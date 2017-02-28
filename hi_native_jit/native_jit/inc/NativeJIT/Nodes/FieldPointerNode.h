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

#include "Node.h"


namespace NativeJIT
{
    template <typename OBJECT, typename FIELD>
    class FieldPointerNode : public Node<FIELD*>
    {
    public:
        FieldPointerNode(ExpressionTree& tree, Node<OBJECT*>& base, FIELD OBJECT::*field);

        //
        // Overrides of Node methods
        //

        virtual ExpressionTree::Storage<FIELD*> CodeGenValue(ExpressionTree& tree) override;
        virtual void Print(std::ostream& out) const override;

        virtual void ReleaseReferencesToChildren() override;

    protected:
        virtual bool GetBaseAndOffset(NodeBase*& base, int32_t& offset) const override;

    private:
        // WARNING: This class is designed to be allocated by an arena allocator,
        // so its destructor will never be called. Therefore, it should hold no
        // resources other than memory from the arena allocator.
        ~FieldPointerNode();

        static int32_t Offset(FIELD OBJECT::*field)
        {
            return static_cast<int32_t>(reinterpret_cast<uint64_t>(&((static_cast<OBJECT*>(nullptr))->*field)));
        }

        NodeBase& m_base;
        const int32_t m_originalOffset;

        // Multiple accesses to the same base object can sometimes be collapsed
        // as an optimization. In such cases, m_collapsedBase/Offset will point
        // to such base object. Otherwise, they will match the base object/offset
        // from the constructor.
        // IMPORTANT: the constructor depends on collapsed base/offset being
        // listed after the original base/offset.
        NodeBase* m_collapsedBase;
        int32_t m_collapsedOffset;
    };


    //*************************************************************************
    //
    // Template definitions for FieldPointerNode
    //
    //*************************************************************************
    template <typename OBJECT, typename FIELD>
    FieldPointerNode<OBJECT, FIELD>::FieldPointerNode(ExpressionTree& tree,
                                                      Node<OBJECT*>& base,
                                                      FIELD OBJECT::*field)
        : Node<FIELD*>(tree),
          m_base(base),
          m_originalOffset(Offset(field)),
          // Note: there is constructor order dependency for these two.
          m_collapsedBase(&m_base),
          m_collapsedOffset(m_originalOffset)
    {
        NodeBase* grandparent;
        int32_t parentOffset;

        // If base can be represented off of another object with an added offset,
        // make the reference off of that object and adjust the offset.
        if (base.GetBaseAndOffset(grandparent, parentOffset))
        {
            m_collapsedBase = grandparent;
            m_collapsedOffset += parentOffset;
            base.MarkReferenced();
        }

        m_collapsedBase->IncrementParentCount();
    }


    template <typename OBJECT, typename FIELD>
    typename ExpressionTree::Storage<FIELD*> FieldPointerNode<OBJECT, FIELD>::CodeGenValue(ExpressionTree& tree)
    {
        auto base = m_collapsedBase->CodeGenAsBase(tree);

        if (m_collapsedOffset == 0)
        {
            base.ConvertToDirect(false);
        }
        else
        {
            base.ConvertToDirect(true);
            tree.GetCodeGenerator()
                .EmitImmediate<OpCode::Add>(base.GetDirectRegister(), m_collapsedOffset);
        }

        // With the added offset, the type changes from void* to FIELD*.
        return ExpressionTree::Storage<FIELD*>(base);
    }


    template <typename OBJECT, typename FIELD>
    bool FieldPointerNode<OBJECT, FIELD>::GetBaseAndOffset(NodeBase*& base, int32_t& offset) const
    {
        base = m_collapsedBase;
        offset = m_collapsedOffset;

        return true;
    }


    template <typename OBJECT, typename FIELD>
    void FieldPointerNode<OBJECT, FIELD>::ReleaseReferencesToChildren()
    {
        m_collapsedBase->DecrementParentCount();
    }


    template <typename OBJECT, typename FIELD>
    void FieldPointerNode<OBJECT, FIELD>::Print(std::ostream& out) const
    {
        this->PrintCoreProperties(out, "FieldPointerNode");

        out << ", base ID = " << m_base.GetId()
                  << ", offset = " << m_originalOffset;

        if (m_base.GetId() != m_collapsedBase->GetId())
        {
            out
               << ", collapsed base ID = " << m_collapsedBase->GetId()
               << ", collapsed offset = " << m_collapsedOffset;
        }
    }
}
