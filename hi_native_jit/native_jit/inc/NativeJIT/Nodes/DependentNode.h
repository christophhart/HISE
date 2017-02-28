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
    // The DependentNode declares that a prerequisite node must be evaluated
    // before a dependent node can be evaluated. It is useful in scenarios with
    // side effects, f. ex. if a CallNode places an output value into a
    // StackVariableNode, dereferencing of the stack variable must be wrapped
    // into a DependentNode.
    template <typename T>
    class DependentNode : public Node<T>
    {
    public:
        DependentNode(ExpressionTree& tree,
                      Node<T>& dependentNode,
                      NodeBase& prerequisiteNode);

        //
        // Overrides of Node methods.
        //

        virtual Storage<T> CodeGenValue(ExpressionTree& tree) override;
        virtual void Print(std::ostream& out) const override;

    private:
        // WARNING: This class is designed to be allocated by an arena allocator,
        // so its destructor will never be called. Therefore, it should hold no
        // resources other than memory from the arena allocator.
        ~DependentNode();

        Node<T>& m_dependentNode;
        NodeBase& m_prerequisiteNode;
    };


    //*************************************************************************
    //
    // Template definitions for the DependentNode.
    //
    //*************************************************************************

    template <typename T>
    DependentNode<T>::DependentNode(ExpressionTree& tree,
                                    Node<T>& dependentNode,
                                    NodeBase& prerequisiteNode)
        : Node<T>(tree),
          m_dependentNode(dependentNode),
          m_prerequisiteNode(prerequisiteNode)
    {
        m_dependentNode.IncrementParentCount();
        // Note: not increasing parent count on prerequisite node as DependentNode
        // is not using its value but only ensuring that it has been evaluated.
    }


    template <typename T>
    Storage<T>
    DependentNode<T>::CodeGenValue(ExpressionTree& tree)
    {
        if (!m_prerequisiteNode.HasBeenEvaluated())
        {
            m_prerequisiteNode.CodeGenCache(tree);
        }

        return m_dependentNode.CodeGen(tree);
    }


    template <typename T>
    void DependentNode<T>::Print(std::ostream& out) const
    {
        this->PrintCoreProperties(out, "DependentNode");

        out << ", dependent = " << m_dependentNode.GetId();
        out << ", prerequisite = " << m_prerequisiteNode.GetId();
    }
}
