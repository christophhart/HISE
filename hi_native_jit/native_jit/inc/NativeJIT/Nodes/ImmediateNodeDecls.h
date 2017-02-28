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
#include "../TypePredicates.h"

namespace NativeJIT
{
    template <typename T, ImmediateCategory IMMCATEGORY = ImmediateCategoryOf<T>::value>
    class ImmediateNode;


    //*************************************************************************
    //
    // Template specializations for ImmediateNode for InlineImmediate types.
    //
    //*************************************************************************
    template <typename T>
    class ImmediateNode<T, ImmediateCategory::InlineImmediate> : public Node<T>
    {
    public:
        ImmediateNode(ExpressionTree& tree, T value);

        //
        // Overrides of Node methods
        //
        virtual void Print(std::ostream& out) const override;
        virtual ExpressionTree::Storage<T> CodeGenValue(ExpressionTree& tree) override;

    private:
        // WARNING: This class is designed to be allocated by an arena allocator,
        // so its destructor will never be called. Therefore, it should hold no
        // resources other than memory from the arena allocator.
        ~ImmediateNode();

        T m_value;
    };


    //*************************************************************************
    //
    // Template specializations for ImmediateNode for RIPRelativeImmediate types.
    //
    //*************************************************************************

    class RIPRelativeImmediate
    {
    public:
        virtual void EmitStaticData(ExpressionTree& tree) = 0;
    };


    template <typename T>
    class ImmediateNode<T, ImmediateCategory::RIPRelativeImmediate>
        : public Node<T>,
          public RIPRelativeImmediate
    {
    public:
        ImmediateNode(ExpressionTree& tree, T value);

        //
        // Overrides of Node methods
        //
        virtual void Print(std::ostream& out) const override;
        virtual ExpressionTree::Storage<T> CodeGenValue(ExpressionTree& tree) override;


        //
        // Overrides of RIPRelativeImmediate methods
        //
        virtual void EmitStaticData(ExpressionTree& tree) override;

    private:
        // WARNING: This class is designed to be allocated by an arena allocator,
        // so its destructor will never be called. Therefore, it should hold no
        // resources other than memory from the arena allocator.
        ~ImmediateNode();

        T m_value;
        int32_t m_offset;
    };
}
