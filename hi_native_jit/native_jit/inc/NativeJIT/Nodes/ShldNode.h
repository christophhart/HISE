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

#include "../CodeGen/X64CodeGenerator.h"     // OpCode type.
#include "Node.h"


namespace NativeJIT
{
    // Implements a node for the SHLD instruction.
    template <typename T>
    class ShldNode : public Node<T>
    {
    public:
        ShldNode(ExpressionTree& tree, Node<T>& shiftee, Node<T>& filler, uint8_t bitCount);

        virtual Storage<T> CodeGenValue(ExpressionTree& tree) override;

        virtual void Print(std::ostream& out) const override;

    private:
        // WARNING: This class is designed to be allocated by an arena allocator,
        // so its destructor will never be called. Therefore, it should hold no
        // resources other than memory from the arena allocator.
        ~ShldNode();

        Node<T>& m_shiftee;
        Node<T>& m_filler;
        const uint8_t m_bitCount;
    };


    //*************************************************************************
    //
    // Template definitions for BinaryNode
    //
    //*************************************************************************
    template <typename T>
    ShldNode<T>::ShldNode(ExpressionTree& tree,
                          Node<T>& shiftee,
                          Node<T>& filler,
                          uint8_t bitCount)
        : Node<T>(tree),
          m_shiftee(shiftee),
          m_filler(filler),
          m_bitCount(bitCount)
    {
        m_shiftee.IncrementParentCount();
        m_filler.IncrementParentCount();
    }


    template <typename T>
    Storage<T> ShldNode<T>::CodeGenValue(ExpressionTree& tree)
    {
        auto & code = tree.GetCodeGenerator();

        Storage<T> shiftee;
        Storage<T> filler;

        this->CodeGenInOrder(tree,
                             m_shiftee, shiftee,
                             m_filler, filler);

        // Convert both arguments to direct registers. The filler value will
        // not be touched.
        {
            auto shifteeReg = shiftee.ConvertToDirect(true);
            ReferenceCounter shifteePin = shiftee.GetPin();
            auto fillerReg = filler.ConvertToDirect(false);

            code.EmitImmediate<OpCode::Shld>(shifteeReg, fillerReg, m_bitCount);
        }

        return shiftee;
    }


    template <typename T>
    void ShldNode<T>::Print(std::ostream& out) const
    {
        this->PrintCoreProperties(out, "Shld");

        out << ", shiftee = " << m_shiftee.GetId()
            << ", filler = " << m_filler.GetId()
            << ", bitCount = " << m_bitCount;
    }
}
