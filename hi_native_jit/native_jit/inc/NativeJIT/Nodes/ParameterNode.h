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

#include "../ExpressionTree.h"
#include "Node.h"
#include "../../Temporary/Assert.h"


namespace NativeJIT
{
    class ParameterSlotAllocator;

    template <typename T>
    class ParameterNode : public Node<T>
    {
    public:
        ParameterNode(ExpressionTree& tree, ParameterSlotAllocator& slotAllocator);

        unsigned GetPosition() const;

        //
        // Overrides of NodeBase methods.
        //
        virtual void ReleaseReferencesToChildren() override;

        //
        // Overrides of Node methods.
        //
        virtual ExpressionTree::Storage<T> CodeGenValue(ExpressionTree& tree) override;

        virtual void Print(std::ostream& out) const override;

    private:
        // WARNING: This class is designed to be allocated by an arena allocator,
        // so its destructor will never be called. Therefore, it should hold no
        // resources other than memory from the arena allocator.
        ~ParameterNode();

        unsigned m_position;
        unsigned m_logicalRegister;
    };


    // ParameterSlotAllocator allocates parameter index numbers which are used to
    // map parameters to registers. In the Windows ABI, a parameter's index
    // numbers is equal to its position in the parameter list, regardless of
    // parameter type. For example, in the function,
    //    void f(int a, float b, int c, float d)
    // the parameter indexes would be
    //    a:0, b:1, c:2, d:3
    // In the System V ABI, parameters indexes are allocated from two sequences,
    // one for integer types and one for floating point types. Given the
    // function definition above, on System V, the parameter indexes would be
    //    a:0, b:0, c:1, d:1
    class ParameterSlotAllocator
    {
    public:
        ParameterSlotAllocator()
            : m_ints(0),
              m_floats(0),
              m_position(0),
              m_register(0)
        {
        }


        unsigned GetPosition() const
        {
            return m_position;
        }


        unsigned GetLogicalRegister() const
        {
            return m_register;
        }


        template <class T, typename std::enable_if<std::is_floating_point<T>::value>::type * = nullptr>
        void Allocate()
        {
            m_position = m_ints + m_floats;
#ifdef NATIVEJIT_PLATFORM_WINDOWS
            m_register = m_position;
#else
            m_register = m_floats;
#endif
            m_floats++;
        }


        template <class T, typename std::enable_if<!std::is_floating_point<T>::value>::type * = nullptr>
        void Allocate()
        {
            m_position = m_ints + m_floats;
#ifdef NATIVEJIT_PLATFORM_WINDOWS
            m_register = m_position;
#else
            m_register = m_ints;
#endif
            m_ints++;
        }

    private:
        unsigned m_ints;
        unsigned m_floats;
        unsigned m_position;
        unsigned m_register;
    };


    //*************************************************************************
    //
    // Template definitions for ParameterNode
    //
    //*************************************************************************

    template <unsigned SIZE>
    void GetParameterRegister(unsigned id, Register<SIZE, false>& r)
    {
        // For now we only support the four parameters that are passed in registers.
        // No support for memory parameters.
        LogThrowAssert(id < 4, "Exceeded maximum number of register parameters.");

        // Integer parameters are passed in RCX, RDX, R8, and R9.
        // Use constants to encode registers. See #31.
#ifdef NATIVEJIT_PLATFORM_WINDOWS
        const uint8_t idMap[] = {1, 2, 8, 9};
#else
        const uint8_t idMap[] = {7, 6, 2, 1, 8, 9};
#endif

        r = Register<SIZE, false>(idMap[id]);
    }


    template <unsigned SIZE>
    void GetParameterRegister(unsigned id, Register<SIZE, true>& r)
    {
        // For now we only support the four parameters that are passed in registers.
        // No support for memory parameters.
        LogThrowAssert(id < 4, "Exceeded maximum number of register parameters.");

        // Floating point parameters are passed in XMM0-XMM3.
        r = Register<SIZE, true>(id);
    }


    template <typename T>
    ParameterNode<T>::ParameterNode(ExpressionTree& tree, ParameterSlotAllocator& slotAllocator)
        : Node<T>(tree)
    {
        slotAllocator.Allocate<T>();
        m_position = slotAllocator.GetPosition();
        m_logicalRegister = slotAllocator.GetLogicalRegister();

        // Parameter nodes are always considered to be referenced (as a part of
        // the function being compiled) even when they are not referenced
        // explicitly.
        this->MarkReferenced();
        tree.AddParameter(*this, m_position);
    }


    template <typename T>
    unsigned ParameterNode<T>::GetPosition() const
    {
        return m_position;
    }


    template <typename T>
    void ParameterNode<T>::ReleaseReferencesToChildren()
    {
        // No children to release.
    }


    template <typename T>
    typename ExpressionTree::Storage<T> ParameterNode<T>::CodeGenValue(ExpressionTree& tree)
    {
        typename Storage<T>::DirectRegister reg;
        GetParameterRegister(m_logicalRegister, reg);

        return tree.Direct<T>(reg);
    }


    template <typename T>
    void ParameterNode<T>::Print(std::ostream& out) const
    {
        this->PrintCoreProperties(out, "ParameterNode");

        out << ", position = " << m_position;
    }
}
