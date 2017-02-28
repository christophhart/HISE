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

#include "../CodeGen/ValuePredicates.h"
#include "ImmediateNodeDecls.h"


namespace NativeJIT
{
    //*************************************************************************
    //
    // Template specializations for ImmediateNode for InlineImmediate types.
    //
    //*************************************************************************
    template <typename T>
    ImmediateNode<T, ImmediateCategory::InlineImmediate>::ImmediateNode(ExpressionTree& tree, T value)
        : Node<T>(tree),
          m_value(value)
    {
    }


    template <typename T>
    void ImmediateNode<T, ImmediateCategory::InlineImmediate>::Print(std::ostream& out) const
    {
        this->PrintCoreProperties(out, "ImmediateNode");

        out << ", value = " << m_value;
    }


    template <typename T>
    Storage<T>
    ImmediateNode<T, ImmediateCategory::InlineImmediate>::CodeGenValue(ExpressionTree& tree)
    {
        return tree.Immediate(m_value);
    }


    //*************************************************************************
    //
    // Template specializations for ImmediateNode for RIPRelativeImmediate types.
    //
    //*************************************************************************

    template <typename T>
    ImmediateNode<T, ImmediateCategory::RIPRelativeImmediate>::ImmediateNode(ExpressionTree& tree, T value)
        : Node<T>(tree),
          m_value(value)
    {
        tree.AddRIPRelative(*this);

        // m_offset will be initialized with the correct value during pass0
        // of compilation in the call to EmitStaticData().
        m_offset = 0;
    }


    template <typename T>
    void ImmediateNode<T, ImmediateCategory::RIPRelativeImmediate>::Print(std::ostream& out) const
    {
        this->PrintCoreProperties(out, "ImmediateNode (RIP-indirect)");

        out << ", value = " << m_value;
    }


    template <typename T>
    Storage<T>
    ImmediateNode<T, ImmediateCategory::RIPRelativeImmediate>::CodeGenValue(ExpressionTree& tree)
    {
        return tree.RIPRelative<T>(m_offset);
    }


    template <typename T>
    void ImmediateNode<T, ImmediateCategory::RIPRelativeImmediate>::EmitStaticData(ExpressionTree& tree)
    {
        auto & code = tree.GetCodeGenerator();
        code.AdvanceToAlignment<T>();
        m_offset = code.CurrentPosition();

        // Emit the value using a canonical type since the EmitValueBytes
        // method intentionally has a limited number of input types. Basic
        // types will be unchanged, but f. ex. function pointers will be
        // emitted as uint64_t.
        code.EmitBytes(ForcedCast<typename CanonicalRegisterStorageType<T>::Type>(m_value));
    }
}
