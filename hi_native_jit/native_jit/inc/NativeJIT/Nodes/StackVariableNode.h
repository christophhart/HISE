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
    // Represents a variable on the stack of the function being compiled.
    // The variable is valid as long as the function it is being compiled for
    // is running. Once the function returns and its stack gets freed, the
    // variable is invalid. This means that StackVariableNode cannot be used as
    // return value from the function.
    template <typename T>
    class StackVariableNode : public Node<T&>
    {
    public:
        StackVariableNode(ExpressionTree& tree);

        //
        // Overrides of Node methods
        //

        virtual void Print(std::ostream& out) const override;
        virtual Storage<T&> CodeGenValue(ExpressionTree& tree) override;

    private:
        // WARNING: This class is designed to be allocated by an arena allocator,
        // so its destructor will never be called. Therefore, it should hold no
        // resources other than memory from the arena allocator.
        ~StackVariableNode();

        Storage<T> m_stackStorage;
    };


    template <typename T>
    StackVariableNode<T>::StackVariableNode(ExpressionTree& tree)
        : Node<T&>(tree)
    {
    }


    template <typename T>
    void StackVariableNode<T>::Print(std::ostream& out) const
    {
        this->PrintCoreProperties(out, "StackVariableNode");

        if (!m_stackStorage.IsNull())
        {
            out << ", offset " << m_stackStorage.GetOffset()
                << " off " << m_stackStorage.GetBaseRegister().GetName();
        }
        else
        {
            out << ", storage not yet assigned";
        }
    }


    template <typename T>
    ExpressionTree::Storage<T&> StackVariableNode<T>::CodeGenValue(ExpressionTree& tree)
    {
        // Allocate temporary storage and store it as a class variable to keep
        // it alive. Make a copy which we can give up ownership on.
        //
        // DESIGN NOTE: The storage returned by CodeGenValue() is a pointer to the
        // stack space allocated by m_stackStorage. Thus, m_stackStorage has to
        // be kept alive for the address to remain valid. Currently the space
        // is kept for the whole lifetime of the expression, which may be an issue
        // if something uses many short-lived stack variables. Consider making it
        // possible for one storage to extend the lifetime of another storage
        // (for example, a concept of parent storage or similar).
        m_stackStorage = tree.Temporary<T>();
        auto storageCopy = m_stackStorage;

        // Transfer ownership of the copy to a direct storage of variable's address.
        auto addressOfStorage = Storage<T*>::AddressOfIndirect(std::move(storageCopy));

        // Convert the pointer to a reference and return it.
        return Storage<T&>(addressOfStorage);
    }
}
