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

//
// Declaration includes - these includes must come before implementation includes.
//
#include "ExpressionTreeDecls.h"

//
// Implementation includes
//
#include <algorithm>    // For std::find.
#include <iostream>     // Debugging output.

#include "BitOperations.h"
#include "CodeGen/CallingConvention.h"
#include "CodeGenHelpers.h"
#include "TypeConverter.h"
#include "TypePredicates.h"
#include "../Temporary/AllocatorOperations.h"
#include "../Temporary/Assert.h"
#include "../Temporary/NonCopyable.h"


namespace Allocators
{
    class IAllocator;
}

namespace NativeJIT
{
    //*************************************************************************
    //
    // Template definitions for ExpressionTree
    //
    //*************************************************************************

    template <>
    class ExpressionTree::FreeListForRegister<false>
    {
    public:
        static auto Get(ExpressionTree& tree) -> decltype(tree.m_rxxFreeList) &
        {
            return tree.m_rxxFreeList;
        }
    };


    template <>
    class ExpressionTree::FreeListForRegister<true>
    {
    public:
        static auto Get(ExpressionTree& tree) -> decltype(tree.m_xmmFreeList) &
        {
            return tree.m_xmmFreeList;
        }
    };


    template <typename T>
    typename Storage<T>::DirectRegister ExpressionTree::GetResultRegister()
    {
        return typename Storage<T>::DirectRegister(0);
    }


    template <typename T, typename... ConstructorArgs>
    T& ExpressionTree::PlacementConstruct(ConstructorArgs&&... constructorArgs)
    {
        return Allocators::PlacementConstruct<T>(m_allocator,
                                                 std::forward<ConstructorArgs>(constructorArgs)...);
    }


    template <typename T>
    ExpressionTree::Storage<T> ExpressionTree::Direct()
    {
        auto & freeList = FreeListForType<T>::Get(*this);
        Storage<T> direct;

        if (freeList.GetFreeCount() > 0)
        {
            direct = Storage<T>::ForAnyFreeRegister(*this);
        }
        else
        {
            const unsigned id = freeList.GetAllocatedSpillable();
            direct = Direct<T>(typename Storage<T>::DirectRegister(id));
        }

        return direct;
    }


    template <typename T>
    ExpressionTree::Storage<T>
    ExpressionTree::Direct(typename Storage<T>::DirectRegister r)
    {
        typedef typename Storage<T>::FullRegister FullRegister;

        auto & code = GetCodeGenerator();
        auto & freeList = FreeListForType<T>::Get(*this);

        LogThrowAssert(!IsPinned(r), "Attempted to obtain the pinned register %s", r.GetName());

        unsigned src = r.GetId();

        if (!freeList.IsAvailable(src))
        {
            typedef typename CanonicalRegisterType<FullRegister>::Type FullType;
            // DESIGN NOTE: If the storage is indirect, FullType may not be the correct
            // size. It will still get the right data due to the little endian
            // architecture, but if the process doesn't have access to the
            // additional bytes (f. ex. at the end of the allocated memory
            // block) it will trigger access violation. See also the comment
            // for the indirect Data constructor.
            auto registerStorage = Storage<FullType>
                ::ForAdditionalReferenceToRegister(*this, FullRegister(src));

            // Use another register if available or a temporary otherwise to
            // bump the current contents of the register.
            if (freeList.GetFreeCount() > 0)
            {
                auto destStorage = Storage<FullType>::ForAnyFreeRegister(*this);
                CodeGenHelpers::Emit<OpCode::Mov>(code,
                                                  destStorage.GetDirectRegister(),
                                                  registerStorage);

                // Swap storages for the target storage and for the register
                // (including all the references to it). Once the destStorage
                // variable goes out of scope, the register will be free.
                registerStorage.Swap(destStorage, Storage<FullType>::SwapType::AllReferences);
            }
            else
            {
                // IMPORTANT: the spilling code must not affect and CPU flags
                // since many nodes (e.g. ConditionalNode) assume that they
                // can modify flags, allocate a register and then act on flags.
                // The MOV instruction does not affect any flags.
                //
                // It is not possible to spill an indirect to stack in one
                // step, so ensure that the source storage is direct.
                // If it was previously indirect, its register will be reused
                // because the conversion is not for modification and because
                // the register is not one of the base registers.
                registerStorage.ConvertToDirect(false);
                auto fullReg = registerStorage.GetDirectRegister();
                LogThrowAssert(fullReg.IsSameHardwareRegister(r),
                               "Converting %s to direct without modification should "
                               "not have moved into a different register (%s)",
                               r.GetName(),
                               fullReg.GetName());

                // Make registerStorage the only owner. After it goes out of
                // scope, the register will be free.
                registerStorage.TakeSoleOwnershipOfDirect();
            }
        }

        return Storage<T>::ForFreeRegister(*this, r);
    }


    template <typename T>
    ExpressionTree::Storage<T> ExpressionTree::RIPRelative(int32_t offset)
    {
        return Storage<T>::ForSharedBaseRegister(*this, rip, offset);
    }


    template <typename T>
    ExpressionTree::Storage<T> ExpressionTree::Temporary()
    {
        static_assert(sizeof(T) <= sizeof(void*),
                      "The size of the variable is too large.");

        uint32_t slot;

        if (m_temporaries.size() > 0)
        {
            // TODO: assert that m_temporaries is > 0?
            slot = static_cast<uint32_t>(m_temporaries.back());
            m_temporaries.pop_back();
        }
        else
        {
            // Note: FunctionSpecification will throw if too much stack gets allocated.
            slot = m_temporaryCount++;
        }

        const int32_t offset = TemporarySlotToOffset(slot);

        return Storage<T>::ForSharedBaseRegister(*this, GetBasePointer(), offset);
    }


    template <typename T>
    ExpressionTree::Storage<T> ExpressionTree::Immediate(T value)
    {
        return Storage<T>::ForImmediate(*this, value);
    }


    template <unsigned SIZE>
    void ExpressionTree::ReleaseRegister(Register<SIZE, false> r)
    {
        m_rxxFreeList.Release(r.GetId());
    }


    template <unsigned SIZE>
    void ExpressionTree::ReleaseRegister(Register<SIZE, true> r)
    {
        m_xmmFreeList.Release(r.GetId());
    }


    template <unsigned SIZE, bool ISFLOAT>
    bool ExpressionTree::IsPinned(Register<SIZE, ISFLOAT> reg)
    {
        auto & freeList = FreeListForRegister<ISFLOAT>::Get(*this);

        return freeList.IsPinned(reg.GetId());
    }


    template <unsigned SIZE>
    bool ExpressionTree::IsAnySharedBaseRegister(Register<SIZE, false> r) const
    {
        return IsBasePointer(PointerRegister(r))
               || r.IsRIP()
               || r.IsStackPointer();
    }


    template <unsigned SIZE>
    bool ExpressionTree::IsAnySharedBaseRegister(Register<SIZE, true> /* r */) const
    {
        return false;
    }


    //*************************************************************************
    //
    // Template definitions for ExpressionTree::Data
    //
    //*************************************************************************
    template <unsigned SIZE, bool ISFLOAT>
    ExpressionTree::Data::Data(ExpressionTree& tree,
                               Register<SIZE, ISFLOAT> r)
        : m_tree(tree),
          m_storageClass(StorageClass::Direct),
          m_isFloat(ISFLOAT),
          m_registerId(r.GetId()),
          m_offset(0),
          m_refCount(0)
    {
        NotifyDataRegisterChange(RegisterChangeType::Initialize);
    }


    template <typename T>
    ExpressionTree::Data::Data(ExpressionTree& tree,
                               T value)
        : m_tree(tree),
          m_storageClass(StorageClass::Immediate),
          m_isFloat(false),
          m_registerId(0),
          m_offset(0),
          m_refCount(0)
    {
        static_assert(CanBeInImmediateStorage<T>::value, "Invalid immediate type");
        static_assert(sizeof(T) <= sizeof(m_immediate), "Unsupported type.");
        m_immediate = convertType<T, size_t>(value);

        // Note: no need to call NotifyDataRegisterChange() as this constructor
        // doesn't apply to registers.
    }


    template <typename T>
    T ExpressionTree::Data::GetImmediate() const
    {
        static_assert(CanBeInImmediateStorage<T>::value, "Invalid immediate type");
        static_assert(sizeof(T) <= sizeof(m_immediate), "Unsupported type.");
        LogThrowAssert(m_storageClass == StorageClass::Immediate, "GetImmediate() called for non-immediate storage!");

        return convertType<size_t, T>(m_immediate);
    }


    template <bool ISFLOAT>
    void ExpressionTree::Data::NotifyDataRegisterChange(RegisterChangeType type)
    {
        LogThrowAssert(m_storageClass != StorageClass::Immediate, "Invalid storage class");
        auto & freeList = FreeListForRegister<ISFLOAT>::Get(m_tree);

        switch (type)
        {
        case RegisterChangeType::Initialize:
            // The free list doesn't need to keep data for indirects relative
            // to shared base registers since there can be many of them for
            // the same register.
            if (!(m_storageClass == StorageClass::Indirect
                  && IsSharedBaseRegister()))
            {
                freeList.InitializeData(m_registerId, this);
            }
            break;

        case RegisterChangeType::Update:
            // Only initialization is allowed for the direct shared registers.
            LogThrowAssert(!(m_storageClass == StorageClass::Direct
                             && IsSharedBaseRegister()),
                           "Cannot update data for shared register %u",
                           m_registerId);

            // At this point, any reference to shared register is indirect
            // and the free list doesn't need to know about such updates.
            if (!IsSharedBaseRegister())
            {
                freeList.UpdateData(m_registerId, this);
            }

            break;

        // clang warns on a default when all enum cases are covered.
        // default:
        //     LogThrowAbort("Unknown register change type %u", type);
        //     break;
        }
    }


    //*************************************************************************
    //
    // Template definitions for Storage<T>
    //
    //*************************************************************************
    template <typename T>
    ExpressionTree::Storage<T>::Storage()
        : m_data(nullptr)
    {
    }


    template <typename T>
    template <typename U>
    ExpressionTree::Storage<T>::Storage(const Storage<U>& other)
        : m_data(nullptr)
    {
        SetData(other.m_data);
    }


    template <typename T>
    ExpressionTree::Storage<T>::Storage(Storage<void*>&& base,
                                        int32_t byteOffset)
        : m_data(nullptr)
    {
        // Load the base pointer into a register.
        base.ConvertToDirect(true);

        // Dereference it.
        base.m_data->ConvertDirectToIndirect(byteOffset);

        // Transfer ownership of datablock to this Storage.
        SetData(base.m_data);
        base.Reset();
    }


    template <typename T>
    Storage<T> ExpressionTree::Storage<T>::ForAdditionalReferenceToRegister(
            ExpressionTree& tree,
            DirectRegister reg)
    {
        auto & freeList = FreeListForType<T>::Get(tree);
        LogThrowAssert(!freeList.IsAvailable(reg.GetId()),
                       "Register %s must already be allocated",
                       reg.GetName());

        return Storage<T>(freeList.GetData(reg.GetId()));
    }


    template <typename T>
    Storage<T> ExpressionTree::Storage<T>::ForAnyFreeRegister(ExpressionTree& tree)
    {
        auto & freeList = FreeListForType<T>::Get(tree);
        Storage<T>::DirectRegister r(freeList.Allocate());

        Data* data = &tree.PlacementConstruct<Data>(tree, r);

        return Storage<T>(data);
    }


    template <typename T>
    Storage<T> ExpressionTree::Storage<T>::ForFreeRegister(ExpressionTree& tree,
                                                           DirectRegister reg)
    {
        auto & freeList = FreeListForType<T>::Get(tree);
        freeList.Allocate(reg.GetId());

        Data* data = &tree.PlacementConstruct<Data>(tree, reg);

        return Storage<T>(data);
    }


    template <typename T>
    Storage<T> ExpressionTree::Storage<T>::ForSharedBaseRegister(
        ExpressionTree& tree,
        BaseRegister base,
        int32_t offset)
    {
        LogThrowAssert(tree.IsAnySharedBaseRegister(base), "Register %s is not a shared base register", base.GetName());

        return Storage<T>(&tree.PlacementConstruct<Data>(tree, base, offset));
    }


    template <typename T>
    Storage<T> ExpressionTree::Storage<T>::ForImmediate(ExpressionTree& tree, T value)
    {
        return Storage<T>(&tree.PlacementConstruct<Data>(tree, value));
    }


    template <typename T>
    template <typename U>
    Storage<T> ExpressionTree::Storage<T>::AddressOfIndirect(Storage<U>&& indirect)
    {
        static_assert(std::is_pointer<T>::value, "T must be a pointer type");
        static_assert(std::is_same<U, typename std::remove_pointer<T>::type>::value,
                      "U must be the type T points to");

        // The storage being created.
        Storage<T> target;

        // Source information.
        auto const base = indirect.GetBaseRegister();
        auto const offset = indirect.GetOffset();

        auto & tree = indirect.m_data->GetTree();
        auto & code = tree.GetCodeGenerator();

        // Reuse the base register if possible. Since the indirect argument
        // is giving away the ownership, the register can be reused if that's
        // the only reference to it and if it's not one of the special
        // reserved/shared registers. Since T is a pointer type, the register
        // it needs is compatible to indirect's base PointerRegister.
        if (indirect.IsSoleDataOwner()
            && !tree.IsAnySharedBaseRegister(base))
        {
            // Get the address and convert the storage to indirect.
            code.template Emit<OpCode::Lea>(base, base, offset);

            indirect.m_data->ConvertIndirectToDirect();
            target.SetData(indirect.m_data);
        }
        else
        {
            // Get a pin for the base register to make sure it doesn't get spilled
            // by the target register allocation.
            ReferenceCounter basePin = indirect.GetPin();
            target = tree.template Direct<T>();
            code.template Emit<OpCode::Lea>(target.GetDirectRegister(), base, offset);
        }

        // Take away ownership from the indirect storage.
        indirect.Reset();

        return target;
    }


    template <typename T>
    ExpressionTree::Storage<T>::Storage(Storage const & other)
        : m_data(nullptr)
    {
        SetData(other.m_data);
    }


    template <typename T>
    ExpressionTree::Storage<T>& ExpressionTree::Storage<T>::operator=(Storage const & other)
    {
        SetData(other.m_data);
        return *this;
    }


    template <typename T>
    ExpressionTree::Storage<T>::Storage(ExpressionTree::Data* data)
        : m_data(data)
    {
        m_data->Increment();
    }


    template <typename T>
    ExpressionTree::Storage<T>::~Storage()
    {
        SetData(nullptr);
    }


    template <typename T>
    void ExpressionTree::Storage<T>::Reset()
    {
        SetData(nullptr);
    }


    template <typename T>
    bool ExpressionTree::Storage<T>::IsNull() const
    {
        return m_data == nullptr;
    }


    template <typename T>
    bool ExpressionTree::Storage<T>::IsSoleDataOwner() const
    {
        return IsNull() || m_data->GetRefCount() == 1;
    }


    template <typename T>
    StorageClass ExpressionTree::Storage<T>::GetStorageClass() const
    {
        return m_data->GetStorageClass();
    }


    template <typename T>
    typename ExpressionTree::Storage<T>::DirectRegister ExpressionTree::Storage<T>::GetDirectRegister() const
    {
        LogThrowAssert(m_data->GetStorageClass() == StorageClass::Direct, "GetDirectRegister(): storage class must be direct.");
        return DirectRegister(m_data->GetRegisterId());
    }


    template <typename T>
    typename ExpressionTree::Storage<T>::BaseRegister ExpressionTree::Storage<T>::GetBaseRegister() const
    {
        LogThrowAssert(m_data->GetStorageClass() == StorageClass::Indirect, "GetBaseRegister(): storage class must be indirect.");
        return BaseRegister(m_data->GetRegisterId());
    }


    template <typename T>
    int32_t ExpressionTree::Storage<T>::GetOffset() const
    {
        LogThrowAssert(m_data->GetStorageClass() == StorageClass::Indirect, "GetOffset(): storage class must be indirect.");
        return m_data->GetOffset();
    }


    template <typename T>
    template <typename U, typename ENABLED>
    U ExpressionTree::Storage<T>::GetImmediate() const
    {
        static_assert(std::is_same<T, U>::value, "U must not be specified explicitly");
        LogThrowAssert(m_data->GetStorageClass() == StorageClass::Immediate, "GetImmediate(): storage class must be immediate.");

        return m_data->GetImmediate<T>();
    }


    template <typename T>
    typename ExpressionTree::Storage<T>::DirectRegister
    ExpressionTree::Storage<T>::ConvertToDirect(bool forModification)
    {
        // DESIGN NOTE: Target the whole target register with MovZX to prevent the partial register stall.

        // IMPORTANT: This method must not affect any CPU flags. See the comment
        // in Direct(reg) method for more information.
        auto & tree = m_data->GetTree();
        auto & code = tree.GetCodeGenerator();

        switch (m_data->GetStorageClass())
        {
        case StorageClass::Immediate:
            ConvertImmediateToDirect(forModification, ImmediateFlavor());
            break;

        case StorageClass::Direct:
            // If the storage is already direct, action is only necessary if it
            // needs to be modified but we're not the sole owner.
            if (!IsSoleDataOwner() && forModification)
            {
                auto dest = tree.Direct<T>();

                // There's a possibility that the current register may get spilled
                // by the allocation, so move from Storage instead of from register.
                CodeGenHelpers::Emit<OpCode::Mov>(code,
                                                  dest.GetDirectRegister(),
                                                  *this);
                SetData(dest);
            }
            break;

        case StorageClass::Indirect:
            {
                BaseRegister base = GetBaseRegister();

                // If we either fully own the storage or don't plan to make
                // modifications, the type of the base register is compatible
                // and it's not one of the reserved (shared) registers, then
                // the base register can be reused.
                if ((IsSoleDataOwner() || !forModification)
                    && BaseRegister::c_isFloat == DirectRegister::c_isFloat
                    && !tree.IsAnySharedBaseRegister(base))
                {
                    code.Emit<OpCode::Mov>(DirectRegister(base), base, GetOffset());
                    m_data->ConvertIndirectToDirect();
                }
                else
                {
                    // Otherwise, move the data to a newly allocated register,
                    // making sure that the base register doesn't get spilled.
                    Storage<T> dest;
                    {
                        ReferenceCounter basePin = GetPin();
                        dest = tree.Direct<T>();
                    }

                    code.Emit<OpCode::Mov>(dest.GetDirectRegister(), base, GetOffset());

                    // Let every owner benefit from moving to direct storage if
                    // possible. This is also necessary for the register to be
                    // fully released during spilling.
                    Swap(dest, forModification
                               ? Storage<T>::SwapType::Single
                               : Storage<T>::SwapType::AllReferences);
                }
            }
            break;

        default:
            LogThrowAbort("ConvertToDirect: invalid storage class.");
            break;
        }

        return GetDirectRegister();
    }


    template <typename T>
    void ExpressionTree::Storage<T>::ConvertImmediateToDirect(bool forModification, ValidImmediateStorage)
    {
        LogThrowAssert(m_data->GetStorageClass() == StorageClass::Immediate, "Unexpected storage class");

        auto & tree = m_data->GetTree();
        auto & code = tree.GetCodeGenerator();

        // Allocate a register and load the immediate into it.
        auto dest = tree.Direct<T>();
        code.EmitImmediate<OpCode::Mov>(dest.GetDirectRegister(), m_data->GetImmediate<T>());

        // Let every owner benefit from moving to direct storage if possible.
        Swap(dest, forModification
                   ? Storage<T>::SwapType::Single
                   : Storage<T>::SwapType::AllReferences);
    }


    template <typename T>
    void ExpressionTree::Storage<T>::ConvertImmediateToDirect(bool /* forModification */, InvalidImmediateStorage)
    {
        // This should never be hit, it's impossible to compile an invalid immediate
        // with StorageClass::Immediate.
        LogThrowAssert(m_data->GetStorageClass() == StorageClass::Immediate, "Unexpected storage class");
        LogThrowAbort("Unexpected occurrence of an invalid immediate storage");
    }


    template <typename T>
    template <typename U>
    void ExpressionTree::Storage<T>::Swap(Storage<U>& other, SwapType type)
    {
        switch (type)
        {
        case SwapType::Single:
            std::swap(m_data, other.m_data);
            break;

        case SwapType::AllReferences:
            m_data->SwapContents(other.m_data);
            break;

        default:
            LogThrowAbort("Unknown swap type %u", type);
            break;
        }
    }


    template <typename T>
    void ExpressionTree::Storage<T>::TakeSoleOwnershipOfDirect()
    {
        // IMPORTANT: This method must not affect any CPU flags. See the comment
        // in Direct(reg) method for more information.
        auto & tree = m_data->GetTree();

        LogThrowAssert(GetStorageClass() == StorageClass::Direct,
                       "Unexpected storage class %u",
                       GetStorageClass());
        LogThrowAssert(!tree.IsAnySharedBaseRegister(GetDirectRegister()),
                       "Cannot take sole ownership of %s",
                       GetDirectRegister().GetName());

        if (!IsSoleDataOwner())
        {
            auto & freeList = FreeListForType<T>::Get(tree);
            auto & code = tree.GetCodeGenerator();
            typedef typename CanonicalRegisterType<FullRegister>::Type FullType;

            // Use another register if available or a temporary otherwise to
            // bump the full contents of the register.
            Storage<FullType> destStorage
                = freeList.GetFreeCount() > 0
                  ? Storage<FullType>::ForAnyFreeRegister(tree)
                  : tree.Temporary<FullType>();

            CodeGenHelpers::Emit<OpCode::Mov>(code,
                                              destStorage,
                                              FullRegister(GetDirectRegister().GetId()));

            // After the swap, the destStorage variable will be the only one
            // still referring to the original register.
            Swap(destStorage, SwapType::AllReferences);

            // Make this instance another owner of the register. The other one
            // in stackStorage will go away after the closing brace.
            *this = Storage<T>(destStorage);
        }
    }


    template <typename T>
    ReferenceCounter ExpressionTree::Storage<T>::GetPin()
    {
        LogThrowAssert(GetStorageClass() != StorageClass::Immediate,
                       "Cannot pin a register for immediate storage");

        if (GetStorageClass() == StorageClass::Direct)
        {
            auto & freeList = FreeListForType<T>::Get(m_data->GetTree());

            return freeList.GetPin(m_data->GetRegisterId());
        }
        else
        {
            auto & freeList = FreeListForRegister<false>::Get(m_data->GetTree());

            return freeList.GetPin(m_data->GetRegisterId());
        }
    }


    template <typename T>
    void ExpressionTree::Storage<T>::SetData(ExpressionTree::Data* data)
    {
        // If the new data is different from the existing data ...
        if (m_data != data)
        {
            // If there is existing data, decrement the ref count and release resources if zero.
            if (!IsNull())
            {
                if (m_data->Decrement() == 0)
                {
                    if (m_data->GetStorageClass() == StorageClass::Direct)
                    {
                        m_data->GetTree().ReleaseRegister(DirectRegister(m_data->GetRegisterId()));
                    }
                    else if (m_data->GetStorageClass() == StorageClass::Indirect)
                    {
                        BaseRegister base(m_data->GetRegisterId());

                        if (base.IsRIP())
                        {
                            // No need to release RIP-relative constant.
                        }
                        else if (m_data->GetTree().IsBasePointer(base))
                        {
                            m_data->GetTree().ReleaseIfTemporary(m_data->GetOffset());
                        }
                        else if (base.IsStackPointer())
                        {
                            // Nothing to release in this case.
                            // Note: this branch must be after the IsBasePointer()
                            // branch to also cover the cases when stack pointer
                            // is chosen to be the base pointer. For such cases,
                            // it's important that temporaries (which are based
                            // off base pointer) are released and the IsBasePointer()
                            // branch handles that.
                        }
                        else
                        {
                            // Release the base register.
                            m_data->GetTree().ReleaseRegister(base);
                        }
                    }

                    // Note: m_data intentionally not released as it's meant to
                    // be allocated and in-place constructed using arena allocator.
                }
            }

            // Set the new data ptr and increment its ref count if applicable.
            m_data = data;
            if (m_data != nullptr)
            {
                m_data->Increment();
            }
        }
    }


    template <typename T>
    void ExpressionTree::Storage<T>::SetData(Storage& other)
    {
        SetData(other.m_data);
    }


    template <typename T>
    void ExpressionTree::Storage<T>::Print(std::ostream& out) const
    {
        switch (GetStorageClass())
        {
        case StorageClass::Direct:
            out << "register " << GetDirectRegister().GetName();
            break;

        case StorageClass::Immediate:
            PrintImmediate(out, ImmediateFlavor());
            break;

        case StorageClass::Indirect:
        {
            IosMiniStateRestorer state(out);

            out << "indirect [" << GetBaseRegister().GetName() << std::uppercase << std::hex;

            if (GetOffset() < 0)
            {
                out << " - " << -GetOffset() << "h";
            }
            else
            {
                out << " + " << GetOffset() << "h";
            }
            out << "]";

            break;
        }

        default:
            out << "[unknown storage type]";
            break;
        }
    }


    template <typename T>
    void ExpressionTree::Storage<T>::PrintImmediate(std::ostream& out, ValidImmediateStorage) const
    {
        // Unary + causes integral promotion and char ([u]int8_t) would be
        // printed as integer, as expected.
        out << "immediate value " << +GetImmediate() << "h";
    }


    template <typename T>
    void ExpressionTree::Storage<T>::PrintImmediate(std::ostream& /* out */, InvalidImmediateStorage) const
    {
        // This should never be hit, it's impossible to compile an invalid immediate
        // with StorageClass::Immediate.
        LogThrowAssert(m_data->GetStorageClass() == StorageClass::Immediate, "Unexpected storage class");
        LogThrowAbort("Unexpected occurrence of an invalid immediate storage");
    }

    template <typename T>
    template <typename U>
    bool ExpressionTree::Storage<T>::operator==(Storage<U> const & other) const
    {
        return this->m_data == other.m_data;
    }


    //*************************************************************************
    //
    // template definitions for FreeList
    //
    //*************************************************************************
    template <unsigned REGISTER_COUNT, bool ISFLOAT>
    ExpressionTree::FreeList<REGISTER_COUNT, ISFLOAT>::FreeList(Allocators::IAllocator& allocator)
        : m_usedMask(0),
          m_lifetimeUsedMask(0),
          m_volatileRegisterMask(ISFLOAT ?
            CallingConvention::c_xmmVolatileRegistersMask :
            CallingConvention::c_rxxVolatileRegistersMask),
          m_nonVolatileRegisterMask(ISFLOAT ?
            CallingConvention::c_xmmNonVolatileRegistersMask :
            CallingConvention::c_rxxNonVolatileRegistersMask),
          m_data(),
          m_allocatedRegisters(Allocators::StlAllocator<uint8_t>(allocator)),
          m_pinCount()
    {
        m_allocatedRegisters.reserve(REGISTER_COUNT);
    }


    template <unsigned REGISTER_COUNT, bool ISFLOAT>
        void ExpressionTree::FreeList<REGISTER_COUNT, ISFLOAT>::AssertValidID(unsigned id) const
    {
        LogThrowAssert(id < REGISTER_COUNT, "Register %u is out of range.", id);
    }


    template <unsigned REGISTER_COUNT, bool ISFLOAT>
    void ExpressionTree::FreeList<REGISTER_COUNT, ISFLOAT>::AssertValidData(unsigned id) const
    {
        AssertValidID(id);
        AssertValidData(id, m_data[id]);
    }


    template <unsigned REGISTER_COUNT, bool ISFLOAT>
    void ExpressionTree::FreeList<REGISTER_COUNT, ISFLOAT>::AssertValidData(unsigned id, Data* data) const
    {
        LogThrowAssert(data != nullptr, "Unexpected null data at/intended for register %u", id);
        LogThrowAssert(data->GetStorageClass() != StorageClass::Immediate,
                       "Invalid storage class %u for data at/intended for register %u",
                       data->GetStorageClass(),
                       id);
        LogThrowAssert(data->GetRegisterId() == id,
                       "Mismatched register ID %u for data at/intended for register %u",
                       data->GetRegisterId(),
                       id);
    }


    template <unsigned REGISTER_COUNT, bool ISFLOAT>
    unsigned ExpressionTree::FreeList<REGISTER_COUNT, ISFLOAT>::GetFreeCount() const
    {
        return BitOp::GetNonZeroBitCount(GetFreeMask());
    }


    template <unsigned REGISTER_COUNT, bool ISFLOAT>
    bool ExpressionTree::FreeList<REGISTER_COUNT, ISFLOAT>::IsAvailable(unsigned id) const
    {
        AssertValidID(id);

        return BitOp::TestBit(GetFreeMask(), id);
    }


    template <unsigned REGISTER_COUNT, bool ISFLOAT>
    unsigned ExpressionTree::FreeList<REGISTER_COUNT, ISFLOAT>::Allocate()
    {
        unsigned id;

        const bool volatileRegisterFound =
            BitOp::GetHighestBitSet(~m_usedMask & m_volatileRegisterMask, &id);

        if (volatileRegisterFound)
        {
            Allocate(id);
            return id;
        }
        else
        {
            const bool nonVolatileRegisterFound =
                BitOp::GetHighestBitSet(~m_usedMask & m_nonVolatileRegisterMask, &id);

            LogThrowAssert(nonVolatileRegisterFound, "No free registers available");

            Allocate(id);
            return id;
        }
    }


    template <unsigned REGISTER_COUNT, bool ISFLOAT>
    void ExpressionTree::FreeList<REGISTER_COUNT, ISFLOAT>::Allocate(unsigned id)
    {
        AssertValidID(id);
        LogThrowAssert(BitOp::TestBit(GetFreeMask(), id), "Register %u must be free", id);
        LogThrowAssert(!IsPinned(id), "Register %u must be unpined when free", id);
        LogThrowAssert(m_data[id] == nullptr, "Data for register %u must be null", id);

        m_allocatedRegisters.push_back(static_cast<uint8_t>(id));

        BitOp::SetBit(&m_usedMask, id);
        BitOp::SetBit(&m_lifetimeUsedMask, id);
    }


    template <unsigned REGISTER_COUNT, bool ISFLOAT>
    void ExpressionTree::FreeList<REGISTER_COUNT, ISFLOAT>::Release(unsigned id)
    {
        AssertValidData(id);
        LogThrowAssert(BitOp::TestBit(GetUsedMask(), id), "Register %u must be allocated", id);
        LogThrowAssert(!IsPinned(id), "Register %u must be unpinned before release", id);
        LogThrowAssert(m_data[id]->GetRefCount() == 0, "Reference count for register %u must be zero", id);

        auto it = std::find(m_allocatedRegisters.begin(),
                            m_allocatedRegisters.end(),
                            static_cast<uint8_t>(id));
        LogThrowAssert(it != m_allocatedRegisters.end(), "Couldn't find allocation record for %u", id);
        m_allocatedRegisters.erase(it);

        m_data[id] = nullptr;
        BitOp::ClearBit(&m_usedMask, id);
    }


    template <unsigned REGISTER_COUNT, bool ISFLOAT>
    void ExpressionTree::FreeList<REGISTER_COUNT, ISFLOAT>::InitializeData(unsigned id, Data* data)
    {
        AssertValidID(id);
        LogThrowAssert(m_data[id] == nullptr, "Data for register %u must be clear", id);
        AssertValidData(id, data);

        m_data[id] = data;
    }


    template <unsigned REGISTER_COUNT, bool ISFLOAT>
    void ExpressionTree::FreeList<REGISTER_COUNT, ISFLOAT>::UpdateData(unsigned id, Data* data)
    {
        AssertValidID(id);
        LogThrowAssert(m_data[id] != nullptr, "Data for register %u must not be clear", id);
        AssertValidData(id, data);

        m_data[id] = data;
    }


    template <unsigned REGISTER_COUNT, bool ISFLOAT>
    ExpressionTree::Data* ExpressionTree::FreeList<REGISTER_COUNT, ISFLOAT>::GetData(unsigned id) const
    {
        AssertValidData(id);

        return m_data[id];
    }


    template <unsigned REGISTER_COUNT, bool ISFLOAT>
    unsigned ExpressionTree::FreeList<REGISTER_COUNT, ISFLOAT>::GetUsedMask() const
    {
        return m_usedMask;
    }

    template <unsigned REGISTER_COUNT, bool ISFLOAT>
    unsigned ExpressionTree::FreeList<REGISTER_COUNT, ISFLOAT>::GetLifetimeUsedMask() const
    {
        return m_lifetimeUsedMask;
    }


    template <unsigned REGISTER_COUNT, bool ISFLOAT>
    unsigned ExpressionTree::FreeList<REGISTER_COUNT, ISFLOAT>::GetFreeMask() const
    {
        return ~m_usedMask & c_fullUsedMask;
    }


    template <unsigned REGISTER_COUNT, bool ISFLOAT>
    unsigned ExpressionTree::FreeList<REGISTER_COUNT, ISFLOAT>::GetAllocatedSpillable() const
    {
        unsigned pinnedCount = 0;
        bool found = false;
        unsigned foundId = 0;

        // Start looking from the oldest allocated register. This is expected
        // to give best results as recently allocated registers are more likely
        // to be needed in the code that's currently being compiled.
        for (unsigned id : m_allocatedRegisters)
        {
            if (IsPinned(id))
            {
                pinnedCount++;
            }
            else
            {
                found = true;
                foundId = id;
                break;
            }
        }

        LogThrowAssert(found,
                       "Couldn't find any registers for spilling: %u registers "
                       "allocated, %u of those are pinned",
                       static_cast<unsigned>(m_allocatedRegisters.size()),
                       pinnedCount);

        return foundId;
    }


    template <unsigned REGISTER_COUNT, bool ISFLOAT>
    ReferenceCounter
    ExpressionTree::FreeList<REGISTER_COUNT, ISFLOAT>::GetPin(unsigned id)
    {
        AssertValidID(id);
        LogThrowAssert(BitOp::TestBit(GetUsedMask(), id), "Register %u must be allocated to be pinned", id);
        AssertValidData(id);

        return ReferenceCounter(m_pinCount[id]);
    }


    template <unsigned REGISTER_COUNT, bool ISFLOAT>
    bool ExpressionTree::FreeList<REGISTER_COUNT, ISFLOAT>::IsPinned(unsigned id) const
    {
        AssertValidID(id);

        return m_pinCount[id] != 0;
    }
}
