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

#include <type_traits>

#include "../CodeGen/X64CodeGenerator.h"     // OpCode type.
#include "Node.h"


namespace NativeJIT
{
    // Implements a binary node that requires a constant as a right hand
    // argument. Unlike the regular binary node, the constant cannot end up
    // in a register, be spilled to a temporary etc, it's always a constant.
    template <OpCode OP, typename L, typename R>
    class BinaryImmediateNode : public Node<L>
    {
    public:
        static_assert(std::is_integral<R>::value,
                      "The right side must have an integral immediate type");

        BinaryImmediateNode(ExpressionTree& tree, Node<L>& left, R right);

        virtual Storage<L> CodeGenValue(ExpressionTree& tree) override;

        virtual void Print(std::ostream& out) const override;

    private:
        // WARNING: This class is designed to be allocated by an arena allocator,
        // so its destructor will never be called. Therefore, it should hold no
        // resources other than memory from the arena allocator.
        ~BinaryImmediateNode();

        Node<L>& m_left;
        const R m_right;
    };


    //*************************************************************************
    //
    // Template definitions for BinaryImmediateNode
    //
    //*************************************************************************
    template <OpCode OP, typename L, typename R>
    BinaryImmediateNode<OP, L, R>::BinaryImmediateNode(
        ExpressionTree& tree, Node<L>& left, R right)
        : Node<L>(tree),
          m_left(left),
          m_right(right)
    {
        m_left.IncrementParentCount();
        // m_right is not a Node, so no IncrementParentCount() call.
    }


    template <OpCode OP, typename L, typename R>
    Storage<L> BinaryImmediateNode<OP, L, R>::CodeGenValue(ExpressionTree& tree)
    {
        auto & code = tree.GetCodeGenerator();

        auto left = m_left.CodeGen(tree);
        code.EmitImmediate<OP>(left.ConvertToDirect(true), m_right);

        return left;
    }


    template <OpCode OP, typename L, typename R>
    void BinaryImmediateNode<OP, L, R>::Print(std::ostream& out) const
    {
        const std::string name = std::string("Operation (")
            + X64CodeGenerator::OpCodeName(OP)
            + ") ";
        this->PrintCoreProperties(out, name.c_str());

        out << ", left = " << m_left.GetId()
            << ", right = " << m_right;
    }
}
