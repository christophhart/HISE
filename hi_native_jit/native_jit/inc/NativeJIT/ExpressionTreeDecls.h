// The MIT License (MIT)

// Copyright (c) 2016, Microsoft

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.


#pragma once

#include <array>                // For arrays in FreeList.
#include <cstdint>
#include <iosfwd>               // For debugging output.

#include "AllocatorVector.h"                  // Embedded member.
#include "CodeGen/JumpTable.h"                // ExpressionTree embeds Label.
#include "CodeGen/Register.h"
#include "TypePredicates.h"                   // RegisterStorage used in typedef.
#include "../Temporary/NonCopyable.h"


namespace Allocators
{
    class IAllocator;
}


namespace NativeJIT
{
    class ExecutionPreconditionTest;
    class FunctionBuffer;
    class NodeBase;
    class RIPRelativeImmediate;


    // A class which increases reference counter on construction and decreases
    // it on destruction.
    // This class is not thread safe.
    class ReferenceCounter final
    {
    public:
        // Default constructor performs no observable reference counting. Used
        // in cases where an object of this class needs to be constructed before
        // the counter object is known (f. ex. in constructors of classes which
        // contain the ReferenceCounter as a member).
        ReferenceCounter();

        // Increases the reference count in the specified counter.
        ReferenceCounter(unsigned& counter);

        // Uses the counter in the other object as its counter and increases it.
        ReferenceCounter(ReferenceCounter const & other);

        // Decreases the current counter.
        ~ReferenceCounter();

        // Decreases the current counter, replaces it with the one in the other
        // object and increases that counter.
        ReferenceCounter& operator=(ReferenceCounter const & other);

        // Decreases the current counter and disassociates itself from it.
        void Reset();

    private:
        // Pointer to the current counter.
        unsigned* m_counter;

        // Methods to add and remove a reference to the counter.
        void AddReference();
        void RemoveReference();
    };


    enum class StorageClass {Direct, Indirect, Immediate};

    class ExpressionTree : public NonCopyable
    {
    private:
        class Data;

    public:
        template <typename T> class Storage;

        // Returns the function return register for the specified type.
        template <typename T>
        static typename Storage<T>::DirectRegister GetResultRegister();

        ExpressionTree(Allocators::IAllocator& allocator, FunctionBuffer& code);

        Allocators::IAllocator& GetAllocator() const;
        FunctionBuffer& GetCodeGenerator() const;

        void EnableDiagnostics(std::ostream& out);
        void DisableDiagnostics();

        // In-place constructs an object using the class allocator. The object's
        // lifetime cannot be longer than that of the ExpressionTree.
        template <typename T, typename... ConstructorArgs>
        T&
        PlacementConstruct(ConstructorArgs&&... constructorArgs);

        //
        // Tree construction
        //
        unsigned AddNode(NodeBase& node);

        // DESIGN NOTE: This might be better if ParameterNode<T> (and get position from it)
        // to ensure that other nodes can't be passed to AddParameter. To make
        // that possible, a circular include dependency between ExpressionTree.h
        // and Node.h (through ParameterNode.h) needs to be broken. Then forward
        // declaring ParameterNode<T> and using #include further below would work.
        void AddParameter(NodeBase& parameter, unsigned position);

        void AddRIPRelative(RIPRelativeImmediate& node);
        void ReportFunctionCallNode(unsigned parameterCount);
        void Compile();

        //
        // Storage allocation.
        //

        template <typename T>
        Storage<T> Direct();

        template <typename T>
        Storage<T> Direct(typename Storage<T>::DirectRegister r);

        template <typename T>
        Storage<T> RIPRelative(int32_t offset);

        // Returns indirect storage relative to the base pointer for a variable
        // of type T. It is guaranteed that it's legal to access the whole quadword
        // at the target address.
        template <typename T>
        Storage<T> Temporary();

        template <typename T>
        Storage<T> Immediate(T value);

        template <unsigned SIZE>
        void ReleaseRegister(Register<SIZE, false> r);

        template <unsigned SIZE>
        void ReleaseRegister(Register<SIZE, true> r);

        // Given an offset off BaseRegister, checks whether the register
        // belongs to a valid offset describing an already allocated slot and,
        // if so, releases the corresponding temporary slot. Some valid offsets
        // off the base register that don't refer to temporaries include offsets
        // referring to compiled function's parameters.
        void ReleaseIfTemporary(int32_t offset);

        // Returns whether a register is pinned.
        template <unsigned SIZE, bool ISFLOAT>
        bool IsPinned(Register<SIZE, ISFLOAT> reg);

        unsigned GetRXXUsedMask() const;
        unsigned GetXMMUsedMask() const;

        Label GetStartOfEpilogue() const;

    protected:
        bool IsDiagnosticsStreamAvailable() const;

        // Returns the diagnostic stream. Throws if it is not available.
        std::ostream& GetDiagnosticsStream() const;

        // Adds a precondition for executing the expression. See the
        // m_preconditionTests variable for more information.
        void AddExecutionPreconditionTest(ExecutionPreconditionTest& test);

        void const * GetUntypedEntryPoint() const;

    private:
        // Keeps track of used and free registers and the Data* associated with
        // each register.
        //
        // The Data* is kept so that it's easily possible to create another
        // Storage around an allocated register when necessary (f. ex. for
        // spilling. Thus, there's a required 1-1 mapping between a general
        // purpose register and the Data* that refers to it (either as Direct
        // or Indirect).
        //
        // Note that it's neither necessary nor possible to hold information
        // about a singular Data* for *Indirect* references to shared base
        // registers such as RSP and RIP since multiple Data* refer to them by
        // definition and since they don't need to be spilled. The Data* for
        // Direct reference to shared base registers is kept, though.
        template <unsigned REGISTER_COUNT, bool ISFLOAT>
        class FreeList
        {
        public:
            // The bit-mask signifying that all valid registers have been allocated.
            static const unsigned c_fullUsedMask = (1ul << REGISTER_COUNT) - 1;

            FreeList(Allocators::IAllocator& allocator);

            // Returns the number of unallocated registers.
            unsigned GetFreeCount() const;

            bool IsAvailable(unsigned id) const;

            unsigned Allocate();

            void Allocate(unsigned id);

            // Returns a pin for a register. Pinned register cannot be spilled.
            // IMPORTANT: Register pinning should be done in a very limited
            // scope. Otherwise, in a larger scope (f. ex. before a CodeGen()
            // call) there is a risk of pinning some of the registers used for
            // parameter passing, which would cause the compilation of a function
            // call to fail.
            ReferenceCounter GetPin(unsigned id);

            // Returns whether a register is pinned.
            bool IsPinned(unsigned id) const;

            void Release(unsigned id);

            // The methods to set and retrieve the Data* owned by the respective
            // register. The InitializeData() and UpdateData() methods differ
            // only in sanity checks: the former requires that no Data* was
            // previously set for the register whereas the latter requires that
            // a Data* already existed.
            void InitializeData(unsigned id, Data* data);
            void UpdateData(unsigned id, Data* data);
            Data* GetData(unsigned id) const;

            // Returns the bit-mask for used and allocated registers.
            unsigned GetUsedMask() const;
            unsigned GetFreeMask() const;

            // Returns a register mask specifying which registers were touched
            // at any point of time, regardless of whether they were later
            // released or not.
            unsigned GetLifetimeUsedMask() const;

            // Returns the ID of an allocated register that is not pinned and
            // can be spilled. Throws if there are no such registers available.
            unsigned GetAllocatedSpillable() const;

        private:
            // Helper methods to perform sanity check on arguments and data contents.
            void AssertValidID(unsigned id) const;
            void AssertValidData(unsigned id) const;
            void AssertValidData(unsigned id, Data* data) const;

            // The mask tracking registers which are currently allocated.
            unsigned m_usedMask;

            // The mask tracking registers which were touched at any point of
            // time, regardless of whether they were later released or not.
            unsigned m_lifetimeUsedMask;

            const unsigned m_volatileRegisterMask;
            const unsigned m_nonVolatileRegisterMask;

            // See the class description for more details.
            std::array<Data*, REGISTER_COUNT> m_data;

            // An array of currently allocated IDs, oldest at the beginning.
            // DESIGN NOTE: Deque and list better satisfy this variable's usage
            // pattern in general (elements always added at the back, mostly pulled
            // out from the front). However, given the small number of elements
            // and the simplicitly of the vector, it is likely to perform better.
            AllocatorVector<uint8_t> m_allocatedRegisters;

            // Number of active references to a pinned register. The register
            // cannot be spilled while it's pinned.
            std::array<unsigned, REGISTER_COUNT> m_pinCount;
        };


        bool IsBasePointer(PointerRegister r) const;
        PointerRegister GetBasePointer() const;

        // Returns whether the register is one of the reserved/shared base
        // registers (instruction, stack or base pointer).
        template <unsigned SIZE>
        bool IsAnySharedBaseRegister(Register<SIZE, false> r) const;

        template <unsigned SIZE>
        bool IsAnySharedBaseRegister(Register<SIZE, true> r) const;

        // Converts a valid temporary slot index into an offset off base register.
        int32_t TemporarySlotToOffset(unsigned temporarySlot);

        // If the temporary offset off base register describes a valid allocated
        // temporary slot, returns true and fills in the temporary slot out
        // parameter. Returns false otherwise.
        bool TemporaryOffsetToSlot(int32_t temporaryOffset, unsigned& temporarySlot);

        void Pass0();
        void Pass1();
        void Pass2();
        void Pass3();
        void Print() const;

        // The following template and the alias template provide a way to access
        // the free list for a register and a C++ type respectively.

        template <bool ISFLOAT>
        class FreeListForRegister;

        template <typename T>
        using FreeListForType = class FreeListForRegister<RegisterStorage<T>::c_isFloat>;

        // The allocator and STL-compatible wrapper around it.
        Allocators::IAllocator& m_allocator;
        Allocators::StlAllocator<void*> m_stlAllocator;

        FunctionBuffer & m_code;

        // Stream used to print diagnostics or nullptr if disabled.
        std::ostream* m_diagnosticsStream;

        AllocatorVector<NodeBase*> m_topologicalSort;
        AllocatorVector<NodeBase*> m_parameters;
        AllocatorVector<RIPRelativeImmediate*> m_ripRelatives;

        // Preconditions for evaluating the whole expression. The preconditions
        // are evaluated right after the parameters and will cause the function
        // to return early if any of them is not met.
        AllocatorVector<ExecutionPreconditionTest*> m_preconditionTests;

        FreeList<RegisterBase::c_maxIntegerRegisterID + 1, false> m_rxxFreeList;
        FreeList<RegisterBase::c_maxFloatRegisterID + 1, true> m_xmmFreeList;

        AllocatorVector<Storage<void*>> m_reservedRxxRegisterStorages;
        AllocatorVector<Storage<double>> m_reservedXmmRegisterStorages;
        AllocatorVector<ReferenceCounter> m_reservedRegistersPins;

        unsigned m_temporaryCount;
        AllocatorVector<int32_t> m_temporaries;

        // Maximum number of parameters used in function calls done by the tree.
        // Negative value signifies no function calls made.
        int m_maxFunctionCallParameters;

        PointerRegister m_basePointer;

        Label m_startOfEpilogue;
    };


    class ExpressionTree::Data : public NonCopyable
    {
    public:
        template <unsigned SIZE, bool ISFLOAT>
        Data(ExpressionTree& tree, Register<SIZE, ISFLOAT> r);

        // DESIGN NOTE: It would be better if indrect memory knew the size of the memory it
        // points to, otherwise access violation can happen if larger, non-owned
        // memory area is dereferenced.
        // Also applies to ConvertDirectToIndirect and Storage::Direct(Register).
        Data(ExpressionTree& tree, PointerRegister r, int32_t offset);

        // Note: attempt to create an immediate storage will static_assert for
        // invalid immediates.
        template <typename T>
        Data(ExpressionTree& tree, T value);

        ExpressionTree& GetTree() const;

        StorageClass GetStorageClass() const;

        unsigned GetRegisterId() const;
        int32_t GetOffset() const;

        // Note: attempt to GetImmediate() will static_assert for invalid immediates.
        template <typename T>
        T GetImmediate() const;

        void ConvertDirectToIndirect(int32_t offset);
        void ConvertIndirectToDirect();

        unsigned GetRefCount() const;
        unsigned Decrement();
        void Increment();

        // Swaps the targets between two Data objects keeping the reference
        // count unchanged and notifies the free list of the register change.
        // Used when all clients of both data objects need to have the contents
        // exchanged.
        void SwapContents(Data* other);

    private:
        // WARNING: This class is designed to be allocated by an arena allocator,
        // so its destructor will never be called. Therefore, it should hold no
        // resources other than memory from the arena allocator.
        ~Data();

        // The type of the register change that the free list gets notified about.
        enum class RegisterChangeType { Initialize, Update };

        // Returns true if data's storage class is direct/immediate and if the
        // register it refers to is one of the shared registers.
        bool IsSharedBaseRegister() const;

        // Notifies the free list that register ID of this data object has
        // been modified. Calls the templated version of the same method to
        // peform the actual work.
        void NotifyDataRegisterChange(RegisterChangeType type);

        // See above.
        template <bool ISFLOAT>
        void NotifyDataRegisterChange(RegisterChangeType type);

        ExpressionTree& m_tree;

        // How the data is stored.
        StorageClass m_storageClass;

        // Which register.
        bool m_isFloat;
        unsigned m_registerId;
        int32_t m_offset;

        // Holds the immediate value for Data whose storage class is Immediate.
        size_t m_immediate;

        // Who is using it.
        unsigned m_refCount;
    };


    // Storage should be public because it is a return type for Node.
    // Storage should be private so that its constructor can take an m_data.
    template <typename T>
    class ExpressionTree::Storage
    {
    public:
        // All Storages need to be treated as the same class, regardless of
        // the template parameter.
        template <typename U> friend class Storage;

        typedef typename RegisterStorage<T>::RegisterType DirectRegister;
        typedef PointerRegister BaseRegister;
        typedef typename DirectRegister::FullRegister FullRegister;

        // Types of swaps. The Single swap affects only the two storages directly
        // involved by swapping their Data*. The AllReferences swap modifies the
        // underlying Data objects thus swapping the data for the first storage
        // as well as the storages that share the same Data* with the second
        // storage as well as the storages that share its Data*.
        enum class SwapType { Single, AllReferences };

        Storage();

        template <typename U>
        explicit Storage(const Storage<U>& other);

        Storage(Storage const & other);

        // Takes ownership of a base storage, adds the offset to it and
        // dereferences it to produce a Storage<T>. Equivalent to
        // *static_cast<T*>(base + byteOffset).
        Storage(Storage<void*>&& base, int32_t byteOffset);

        // Creates another storage referencing a register. The register must
        // already be allocated. For shared base registers, the Storage will
        // reference the direct Data version which cannot be converted to
        // indirect nor spilled since it's pinned.
        static Storage<T> ForAdditionalReferenceToRegister(
            ExpressionTree& tree,
            DirectRegister reg);

        // Allocates an empty register and creates a direct storage referencing it.
        // Can only be called when there are available registers.
        static Storage<T> ForAnyFreeRegister(ExpressionTree& tree);

        // Allocates a specific empty register and creates a direct storage
        // referencing it. Can only be called when the specified register is free.
        static Storage<T> ForFreeRegister(ExpressionTree& tree,
                                          DirectRegister reg);

        // Creates an indirect storage against a shared base register.
        static Storage<T> ForSharedBaseRegister(ExpressionTree& tree,
                                                BaseRegister base,
                                                int32_t offset);

        // Creates an immediate storage with the specified value.
        static Storage<T> ForImmediate(ExpressionTree& tree, T value);

        // Loads the address of an immediate storage into a newly created direct
        // storage and resets the original storage. If possible, reuses the
        // register from the source storage.
        // Note: do not call this method with a last reference to a Temporary as
        // the Temporary may be released after the source storage is reset and
        // the created storage would point to invalid the memory.
        template <typename U>
        static Storage<T> AddressOfIndirect(Storage<U>&& indirect);

        Storage& operator=(Storage const & other);

        ~Storage();

        void Reset();
        bool IsNull() const;

        // Returns whether this Storage shares ownership over its Data*.
        // Note that for direct storages, IsSoleDataOwner() result of true
        // implies that the storage also exclusively owns the register. For
        // indirect storages, this implication stands only for non-shared base
        // registers.
        bool IsSoleDataOwner() const;

        StorageClass GetStorageClass() const;

        DirectRegister GetDirectRegister() const;

        BaseRegister GetBaseRegister() const;
        int32_t GetOffset() const;

        // Note: The method must be removed for invalid immediates rather than
        // static_assert triggered because for some of them (arrays) the method
        // has invalid declaration otherwise.
        template <typename U = T,
                  typename ENABLED = typename std::enable_if<ImmediateCategoryOf<U>::value
                                                             == ImmediateCategory::InlineImmediate>::type>
        U GetImmediate() const;

        DirectRegister ConvertToDirect(bool forModification);

        // Swaps the contents of the two storages. See SwapType definition for
        // more details.
        template <typename U>
        void Swap(Storage<U>& other, SwapType type);

        // If the storage is not the sole data owner, copies the data to stack
        // and switches the ownership of other owners to it.
        // Does nothing if the storage was already the sole data owner.
        // Requires that the storage is direct and that it does not refer to
        // one of the shared base registers.
        void TakeSoleOwnershipOfDirect();

        // Returns a pin for the storage's register. While the pin is held,
        // the register cannot be spilled. Can only be called if Storage is
        // either direct or if it's indirect and refers to non-shared base
        // registers.
        ReferenceCounter GetPin();

        // Prints the information about the storage's type, value, base register
        // and offset to the output stream.
        void Print(std::ostream& out) const;

        // Returns whether or not two storages point to the same Data object.
        // (as opposed to having the same contents).
        template <typename U>
        bool operator==(Storage<U> const & other) const;


    private:
        // Types used to select the correct flavor of immediate methods. This is
        // necessary because GetStorageClass() is a runtime rather than a compile
        // time property. Even though it is not possible to create a
        // StorageClass::Immediate storage for an invalid immediate, the code
        // that branches depending on various types of storage needs to always
        // compile.
        struct ValidImmediateStorage {};
        struct InvalidImmediateStorage {};
        typedef typename std::conditional<CanBeInImmediateStorage<T>::value,
                                          ValidImmediateStorage,
                                          InvalidImmediateStorage>::type
            ImmediateFlavor;

        Storage(ExpressionTree::Data* data);

        void SetData(ExpressionTree::Data* data);
        void SetData(Storage& other);

        void ConvertImmediateToDirect(bool forModification, ValidImmediateStorage);
        void ConvertImmediateToDirect(bool forModification, InvalidImmediateStorage);

        void PrintImmediate(std::ostream& out, ValidImmediateStorage) const;
        void PrintImmediate(std::ostream& out, InvalidImmediateStorage) const;

        ExpressionTree::Data* m_data;
    };


    template <typename T>
    using Storage = typename ExpressionTree::Storage<T>;
}
