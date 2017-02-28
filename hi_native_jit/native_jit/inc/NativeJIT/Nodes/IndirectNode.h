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
    // IndirectNode implements the *(base + index) operation when index is known
    // at compile time.
    template <typename T>
    class IndirectNode : public Node<T>
    {
    public:
        IndirectNode(ExpressionTree& tree, Node<T*>& base, int32_t index);

        //
        // Overrides of Node methods.
        //

        virtual ExpressionTree::Storage<T> CodeGenValue(ExpressionTree& tree) override;
        virtual void Print(std::ostream& out) const override;

        // Note: IndirectNode doesn't implement GetBaseAndOffset() method which
        // allows for base object/offset collapsing optimization because it
        // dereferences the target object, preventing continuation of the chain.

    private:
        // WARNING: This class is designed to be allocated by an arena allocator,
        // so its destructor will never be called. Therefore, it should hold no
        // resources other than memory from the arena allocator.
        ~IndirectNode();

        NodeBase& m_base;
        const int32_t m_index;

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
    // Template definitions for IndirectBase
    //
    //*************************************************************************
    template <typename T>
    IndirectNode<T>::IndirectNode(ExpressionTree& tree, Node<T*>& base, int32_t index)
        : Node<T>(tree),
          m_base(base),
          m_index(index),
          // Note: there is constructor order dependency for these two.
          m_collapsedBase(&m_base),
          m_collapsedOffset(sizeof(T) * m_index)
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


    template <typename T>
    typename ExpressionTree::Storage<T> IndirectNode<T>::CodeGenValue(ExpressionTree& tree)
    {
        // The base node's type ensures that the storage represent a T* rather
        // than the void* returned by CodeGenAsBase(). The local offset calculated
        // from the index skips the required number of T's, so it still represents
        // a T*. Dereference the calculated T* to get to T.
        return ExpressionTree::Storage<T>(m_collapsedBase->CodeGenAsBase(tree),
                                          m_collapsedOffset);
    }


    template <typename T>
    void IndirectNode<T>::Print(std::ostream& out) const
    {
        this->PrintCoreProperties(out, "IndirectNode");

        out << ", base ID = " << m_base.GetId()
            << ", index = " << m_index;

        if (m_base.GetId() != m_collapsedBase->GetId())
        {
            out
                << ", collapsed base ID = " << m_collapsedBase->GetId()
                << ", collapsed offset = " << m_collapsedOffset;
        }
    }
}
