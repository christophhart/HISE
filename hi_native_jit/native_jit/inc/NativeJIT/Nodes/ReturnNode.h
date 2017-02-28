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

#include "../CodeGenHelpers.h"
#include "Node.h"


namespace NativeJIT
{
    template <typename T>
    class ReturnNode : public Node<T>
    {
    public:
        ReturnNode(ExpressionTree& tree, Node<T>& child);

        //
        // Overrides of Node methods.
        //
        virtual ExpressionTree::Storage<T> CodeGenValue(ExpressionTree& tree) override;
        virtual void CompileAsRoot(ExpressionTree& tree) override;
        virtual void Print(std::ostream& out) const override;

    private:
        // WARNING: This class is designed to be allocated by an arena allocator,
        // so its destructor will never be called. Therefore, it should hold no
        // resources other than memory from the arena allocator.
        ~ReturnNode();

        Node<T>& m_child;
    };


    //*************************************************************************
    //
    // Template definitions for ReturnNode
    //
    //*************************************************************************
    template <typename T>
    ReturnNode<T>::ReturnNode(ExpressionTree& tree,
                              Node<T>& child)
        : Node<T>(tree),
          m_child(child)
    {
        // There's an implicit parent to the return node: the function it's used by.
        this->IncrementParentCount();
        child.IncrementParentCount();
    }


    template <typename T>
    typename ExpressionTree::Storage<T> ReturnNode<T>::CodeGenValue(ExpressionTree& tree)
    {
        LogThrowAssert(this->GetParentCount() == 1,
                       "Unexpected parent count for the root node: %u",
                       this->GetParentCount());

        return m_child.CodeGen(tree);
    }


    template <typename T>
    void ReturnNode<T>::CompileAsRoot(ExpressionTree& tree)
    {
        ExpressionTree::Storage<T> s = this->CodeGen(tree);

        auto resultRegister = tree.GetResultRegister<T>();

        // Move result into the result register unless already there.
        if (!(s.GetStorageClass() == StorageClass::Direct
              && s.GetDirectRegister().IsSameHardwareRegister(resultRegister)))
        {
            CodeGenHelpers::Emit<OpCode::Mov>(tree.GetCodeGenerator(), resultRegister, s);
        }
    }


    template <typename T>
    void ReturnNode<T>::Print(std::ostream& out) const
    {
        this->PrintCoreProperties(out, "ReturnNode");
    }
}
