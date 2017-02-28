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


#include "../../inc/NativeJIT/CodeGen/CallingConvention.h"
#include "../../inc/NativeJIT/CodeGen/FunctionBuffer.h"
#include "../../inc/NativeJIT/CodeGen/FunctionSpecification.h"
#include "../../inc/NativeJIT/ExecutionPreconditionTest.h"
#include "../../inc/NativeJIT/ExpressionTree.h"
#include "../../inc/NativeJIT/Nodes/ImmediateNode.h"
#include "../../inc/NativeJIT/Nodes/ParameterNode.h"
#include "../../inc/Temporary/Assert.h"


namespace NativeJIT
{
    //*************************************************************************
    //
    // ExpressionTree
    //
    //*************************************************************************
    ExpressionTree::ExpressionTree(Allocators::IAllocator& allocator, FunctionBuffer& code)
        : m_allocator(allocator),
          m_stlAllocator(allocator),
          m_code(code),
          m_diagnosticsStream(nullptr),
          // Note: there is a member initialization order dependency on
          // m_stlAllocator for multiple members below.
          m_topologicalSort(m_stlAllocator),
          m_parameters(m_stlAllocator),
          m_ripRelatives(m_stlAllocator),
          m_preconditionTests(m_stlAllocator),
          m_rxxFreeList(allocator),
          m_xmmFreeList(allocator),
          m_reservedRxxRegisterStorages(m_stlAllocator),
          m_reservedXmmRegisterStorages(m_stlAllocator),
          m_reservedRegistersPins(m_stlAllocator),
          m_temporaryCount(0),
          m_temporaries(m_stlAllocator),
          m_maxFunctionCallParameters(-1),
          m_basePointer(rbp)
          // m_startOfEpilogue intentionally left uninitialized, see Compile().
    {
        m_reservedRxxRegisterStorages.reserve(RegisterBase::c_maxIntegerRegisterID + 1);
        m_reservedXmmRegisterStorages.reserve(RegisterBase::c_maxFloatRegisterID + 1);
        m_reservedRegistersPins.reserve(RegisterBase::c_maxIntegerRegisterID
                                        + RegisterBase::c_maxFloatRegisterID
                                        + 2);

        for (unsigned i = 0 ; i <= RegisterBase::c_maxIntegerRegisterID; ++i)
        {
            const PointerRegister reg(i);

            if (IsAnySharedBaseRegister(reg)
                || !BitOp::TestBit(CallingConvention::c_rxxWritableRegistersMask, i))
            {
                auto s = Direct<void*>(reg);

                m_reservedRxxRegisterStorages.push_back(s);
                m_reservedRegistersPins.push_back(s.GetPin());
            }
        }

        for (unsigned i = 0 ; i <= RegisterBase::c_maxFloatRegisterID; ++i)
        {
            Register<8, true> reg(i);

            if (IsAnySharedBaseRegister(reg)
                || !BitOp::TestBit(CallingConvention::c_xmmWritableRegistersMask, i))
            {
                auto s = Direct<double>(reg);

                m_reservedXmmRegisterStorages.push_back(s);
                m_reservedRegistersPins.push_back(s.GetPin());
            }
        }
    }


    Allocators::IAllocator& ExpressionTree::GetAllocator() const
    {
        return m_allocator;
    }


    FunctionBuffer& ExpressionTree::GetCodeGenerator() const
    {
        return m_code;
    }


    void ExpressionTree::EnableDiagnostics(std::ostream& out)
    {
        m_diagnosticsStream = &out;
    }


    void ExpressionTree::DisableDiagnostics()
    {
        m_diagnosticsStream = nullptr;
    }


    bool ExpressionTree::IsDiagnosticsStreamAvailable() const
    {
        return m_diagnosticsStream != nullptr;
    }


    std::ostream& ExpressionTree::GetDiagnosticsStream() const
    {
        LogThrowAssert(m_diagnosticsStream != nullptr, "Diagnostics are disabled");

        return *m_diagnosticsStream;
    }


    void ExpressionTree::ReleaseIfTemporary(int32_t offset)
    {
        unsigned slot;

        if (TemporaryOffsetToSlot(offset, slot))
        {
            m_temporaries.push_back(slot);
        }
    }


    unsigned ExpressionTree::GetRXXUsedMask() const
    {
        return m_rxxFreeList.GetUsedMask();
    }


    unsigned ExpressionTree::GetXMMUsedMask() const
    {
        return m_xmmFreeList.GetUsedMask();
    }


    bool ExpressionTree::IsBasePointer(PointerRegister r) const
    {
        return r.GetId() == m_basePointer.GetId();
    }


    PointerRegister ExpressionTree::GetBasePointer() const
    {
        return m_basePointer;
    }

    Label ExpressionTree::GetStartOfEpilogue() const
    {
        return m_startOfEpilogue;
    }


    unsigned ExpressionTree::AddNode(NodeBase& node)
    {
        m_topologicalSort.push_back(&node);
        return static_cast<unsigned>(m_topologicalSort.size() - 1);
    }


    void ExpressionTree::AddParameter(NodeBase& parameter, unsigned position)
    {
        LogThrowAssert(position == m_parameters.size(),
                       "Parameters must be added in order. Previously added %u parameters, "
                       "adding parameter with index %u",
                       static_cast<unsigned>(m_parameters.size()),
                       position);
        m_parameters.push_back(&parameter);
    }


    void ExpressionTree::AddRIPRelative(RIPRelativeImmediate& node)
    {
        m_ripRelatives.push_back(&node);
    }


    void ExpressionTree::AddExecutionPreconditionTest(ExecutionPreconditionTest& test)
    {
        m_preconditionTests.push_back(&test);
    }


    void ExpressionTree::ReportFunctionCallNode(unsigned parameterCount)
    {
        if (static_cast<int>(parameterCount) > m_maxFunctionCallParameters)
        {
            m_maxFunctionCallParameters = parameterCount;
        }
    }


    void ExpressionTree::Compile()
    {
        // Note: the call to Reset() clears all allocated labels, so start of
        // epilogue label must be allocated after that point.
        m_code.Reset();
        m_startOfEpilogue = m_code.AllocateLabel();

        // Generate constants.
        Pass0();

        // Generate code.
        m_code.BeginFunctionBodyGeneration();

        Pass1();
        Pass2();
        Print();
        Pass3();

        const FunctionSpecification spec(m_allocator,
                                         m_maxFunctionCallParameters,
                                         m_temporaryCount,
                                         m_rxxFreeList.GetLifetimeUsedMask()
                                            & CallingConvention::c_rxxNonVolatileRegistersMask
                                            & CallingConvention::c_rxxWritableRegistersMask,
                                         m_xmmFreeList.GetLifetimeUsedMask()
                                            & CallingConvention::c_xmmNonVolatileRegistersMask
                                            & CallingConvention::c_xmmWritableRegistersMask,
                                         FunctionSpecification::BaseRegisterType::SetRbpToOriginalRsp,
                                         m_code.IsDiagnosticsStreamAvailable()
                                         ? &m_code.GetDiagnosticsStream()
                                         : nullptr);

        m_code.PlaceLabel(m_startOfEpilogue);
        m_code.EndFunctionBodyGeneration(spec);

        // Release the reserved registers.
        m_reservedRegistersPins.clear();
        m_reservedXmmRegisterStorages.clear();
        m_reservedRxxRegisterStorages.clear();
        Print();

        LogThrowAssert(GetRXXUsedMask() == 0,
                       "Some integer registers have not been released: 0x%x",
                       GetRXXUsedMask());

        LogThrowAssert(GetXMMUsedMask() == 0,
                       "Some floating point registers have not been released: 0x%x",
                       GetXMMUsedMask());
    }


    void const * ExpressionTree::GetUntypedEntryPoint() const
    {
        return m_code.GetEntryPoint();
    }


    int32_t ExpressionTree::TemporarySlotToOffset(unsigned temporarySlot)
    {
        LogThrowAssert(temporarySlot < m_temporaryCount,
                       "Invalid temporary slot %u (total slots %u)",
                       temporarySlot,
                       m_temporaryCount);

        // Expression tree asks for BaseRegisterType::SetRbpToOriginalRsp. That
        // means that [rbp] holds return address, [rbp + 8] home for function's
        // first argument etc, whereas [rbp - 8] holds the first temporary etc.
        return -static_cast<int32_t>(temporarySlot + 1)
               * sizeof(void*);
    }


    bool ExpressionTree::TemporaryOffsetToSlot(int32_t temporaryOffset, unsigned& temporarySlot)
    {
        bool isTemporary = false;

        if (temporaryOffset < 0
            && -temporaryOffset % sizeof(void*) == 0)
        {
            const unsigned slot = -temporaryOffset / sizeof(void*) - 1;

            if (slot < m_temporaryCount)
            {
                temporarySlot = slot;
                isTemporary = true;
            }
        }

        return isTemporary;
    }


    void ExpressionTree::Pass0()
    {
        if (IsDiagnosticsStreamAvailable())
        {
            GetDiagnosticsStream() << "=== Pass0 ===" << std::endl;
        }

        // Emit RIP-relative constants.
        for (unsigned i = 0 ; i < m_ripRelatives.size(); ++i)
        {
            m_ripRelatives[i]->EmitStaticData(*this);
        }

        // Walk the nodes in reverse order of creation (i.e. in potential order
        // of execution) to see whether they can be optimized away.
        //
        // Note: currently it is not important whether the optimize-away pass
        // traverses the nodes in order or in reverse order. This is because the
        // only existing optimization technique (base pointer collapsing) is
        // performed when node is constructed and removing references from
        // children in such case will not create an opportunity for additional
        // optimizations.
        for (auto nodeIt = m_topologicalSort.rbegin();
             nodeIt != m_topologicalSort.rend();
             ++nodeIt)
        {
            auto node = *nodeIt;

            if (!node->IsReferenced())
            {
				continue;

                Print();
                LogThrowAssert(node->IsReferenced(),
                               "Node with ID %u has been created but not referenced by nodes in the tree",
                               node->GetId());
            }

            // If there's a way for node's children to be evaluated without
            // evaluating this node first (f. ex. collapsing of pointers to the
            // same base object), release the references that will be unused.
            // Otherwise, the lifetime of the Storage returned by this node's
            // CodeGenValue() will be too long.
            if (node->CanBeOptimizedAway())
            {
                node->ReleaseReferencesToChildren();
            }
        }
    }


    void ExpressionTree::Pass1()
    {
        if (IsDiagnosticsStreamAvailable())
        {
            GetDiagnosticsStream() << "=== Pass1 ===" << std::endl;
        }

        // Reserve registers used to pass in parameters.
        for (auto param : m_parameters)
        {
            if (param->GetParentCount() > 0)
            {
                param->CodeGenCache(*this);
            }
        }

        // Execute any return-early tests before compiling the expression further.
        for (auto test : m_preconditionTests)
        {
            test->Evaluate(*this);
        }
    }


    void ExpressionTree::Pass2()
    {
        if (IsDiagnosticsStreamAvailable())
        {
            GetDiagnosticsStream() << "=== Pass2 ===" << std::endl;
        }

        for (unsigned i = 0 ; i < m_topologicalSort.size(); ++i)
        {
            NodeBase& node = *m_topologicalSort[i];

            if (node.GetParentCount() > 1 && !node.HasBeenEvaluated())
            {
                node.CodeGenCache(*this);
            }
        }
    }


    void ExpressionTree::Pass3()
    {
        if (IsDiagnosticsStreamAvailable())
        {
            GetDiagnosticsStream() << "=== Pass3 ===" << std::endl;
        }

        NodeBase& root = *m_topologicalSort.back();

        // LogThrowAssert(Root node is return, "Root must be a return node.");

        root.CompileAsRoot(*this);
    }


    void ExpressionTree::Print() const
    {
        if (!IsDiagnosticsStreamAvailable())
        {
            return;
        }

        std::ostream& out = GetDiagnosticsStream();

        out << "Parameters:" << std::endl;
        for (unsigned i = 0 ; i < m_parameters.size(); ++i)
        {
            m_parameters[i]->Print(out);
            out << std::endl;
        }
        out << std::endl;

        out << "Topological sort:" << std::endl;
        for (unsigned i = 0 ; i < m_topologicalSort.size(); ++i)
        {
            m_topologicalSort[i]->Print(out);
            out << std::endl;
        }
        out << std::endl;

        out << "RXX Registers:" << std::endl;
        for (unsigned i = 0 ; i <= RegisterBase::c_maxIntegerRegisterID; ++i)
        {
            out << Register<8, false>(i).GetName()
                      << (m_rxxFreeList.IsAvailable(i) ? " free" : " in use")
                      << std::endl;
        }

        out << std::endl;

        out << "XMM Registers:" << std::endl;
        for (unsigned i = 0 ; i <= RegisterBase::c_maxFloatRegisterID; ++i)
        {
            out << Register<8, true>(i).GetName()
                      << (m_xmmFreeList.IsAvailable(i) ? " free" : " in use")
                      << std::endl;
        }

        out << "Temporaries used: " << m_temporaryCount << std::endl;
        out << "Temporaries still in use: " << m_temporaryCount - m_temporaries.size() << std::endl;

        out << std::endl;
    }


    //*************************************************************************
    //
    // ExpressionTree::Data
    //
    //*************************************************************************
    ExpressionTree::Data::Data(ExpressionTree& tree,
                               PointerRegister base,
                               int32_t offset)
        : m_tree(tree),
          m_storageClass(StorageClass::Indirect),
          m_isFloat(base.c_isFloat),
          m_registerId(base.GetId()),
          m_offset(offset),
          m_refCount(0)
    {
        NotifyDataRegisterChange(RegisterChangeType::Initialize);
    }


    ExpressionTree& ExpressionTree::Data::GetTree() const
    {
        return m_tree;
    }


    StorageClass ExpressionTree::Data::GetStorageClass() const
    {
        return m_storageClass;
    }


    unsigned ExpressionTree::Data::GetRegisterId() const
    {
        return m_registerId;
    }


    int32_t ExpressionTree::Data::GetOffset() const
    {
        LogThrowAssert(m_storageClass == StorageClass::Indirect, "StorageClass must be Indirect, found %u", m_storageClass);
        return m_offset;
    }


    void ExpressionTree::Data::ConvertDirectToIndirect(int32_t offset)
    {
        LogThrowAssert(m_storageClass == StorageClass::Direct,
                       "StorageClass must be Direct, found %u",
                       m_storageClass);
        LogThrowAssert(!IsSharedBaseRegister(),
                       "Cannot change type of shared register %u from direct to indirect",
                       m_registerId);

        m_storageClass = StorageClass::Indirect;
        m_offset = offset;
    }


    void ExpressionTree::Data::ConvertIndirectToDirect()
    {
        LogThrowAssert(m_storageClass == StorageClass::Indirect,
                       "StorageClass must be Indirect, found %u",
                       m_storageClass);
        LogThrowAssert(!IsSharedBaseRegister(),
                       "Cannot change type of shared register %u from indirect to direct",
                       m_registerId);

        m_storageClass = StorageClass::Direct;
        m_offset = 0;
    }


    unsigned ExpressionTree::Data::GetRefCount() const
    {
        return m_refCount;
    }


    unsigned ExpressionTree::Data::Decrement()
    {
         LogThrowAssert(m_refCount > 0, "Attempting to decrement zero ref count.");
        --m_refCount;

        return m_refCount;
    }


    void ExpressionTree::Data::Increment()
    {
        ++m_refCount;
    }


    void ExpressionTree::Data::SwapContents(Data* other)
    {
        std::swap(m_storageClass, other->m_storageClass);
        std::swap(m_isFloat, other->m_isFloat);
        std::swap(m_registerId, other->m_registerId);
        std::swap(m_offset, other->m_offset);

        std::swap(m_immediate, other->m_immediate);

        // Both Data objects keep their m_refCount.

        NotifyDataRegisterChange(RegisterChangeType::Update);
        other->NotifyDataRegisterChange(RegisterChangeType::Update);
    }


    void ExpressionTree::Data::NotifyDataRegisterChange(RegisterChangeType type)
    {
        if (m_storageClass != StorageClass::Immediate)
        {
            if (m_isFloat)
            {
                NotifyDataRegisterChange<true>(type);
            }
            else
            {
                NotifyDataRegisterChange<false>(type);
            }
        }
    }


    bool ExpressionTree::Data::IsSharedBaseRegister() const
    {
        return m_storageClass != StorageClass::Immediate
            && !m_isFloat
            && m_tree.IsAnySharedBaseRegister(PointerRegister(m_registerId));
    }


    ReferenceCounter::ReferenceCounter()
        : m_counter(nullptr)
    {
        // No-op, for symmetry only.
        AddReference();
    }


    ReferenceCounter::ReferenceCounter(unsigned& counter)
        : m_counter(&counter)
    {
        AddReference();
    }


    ReferenceCounter::ReferenceCounter(ReferenceCounter const & other)
        : ReferenceCounter(*other.m_counter)
    {
    }


    ReferenceCounter::~ReferenceCounter()
    {
        RemoveReference();
    }


    ReferenceCounter& ReferenceCounter::operator=(ReferenceCounter const & other)
    {
        if (m_counter != other.m_counter)
        {
            RemoveReference();

            m_counter = other.m_counter;
            AddReference();
        }

        return *this;
    }


    void ReferenceCounter::Reset()
    {
        RemoveReference();
        m_counter = nullptr;
    }


    void ReferenceCounter::AddReference()
    {
        if (m_counter != nullptr)
        {
            ++(*m_counter);
        }
    }


    void ReferenceCounter::RemoveReference()
    {
        if (m_counter != nullptr)
        {
            if (*m_counter == 0)
            {
                throw std::logic_error("Tried to remove a reference with zero reference count");
            }

            --(*m_counter);
        }
    }
}
