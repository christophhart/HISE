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

#include "ExecutionPreconditionTest.h"
#include "ExpressionNodeFactory.h"
#include "TypePredicates.h"


namespace NativeJIT
{
    template <typename R>
    class FunctionBase : public ExpressionNodeFactory
    {
    public:
        FunctionBase(Allocators::IAllocator& allocator, FunctionBuffer& code);

        template <JccType JCC>
        void AddExecuteOnlyIfStatement(FlagExpressionNode<JCC>& condition,
                                       ImmediateNode<R>& otherwiseValue);

    private:
        Allocators::IAllocator& m_allocator;
    };


    template <typename R, typename P1 = void, typename P2 = void, typename P3 = void, typename P4 = void>
    class Function : public FunctionBase<R>
    {
    public:
        Function(Allocators::IAllocator& allocator, FunctionBuffer& code);

        ParameterNode<P1>& GetP1() const;
        ParameterNode<P2>& GetP2() const;
        ParameterNode<P3>& GetP3() const;
        ParameterNode<P4>& GetP4() const;

        typedef R (*FunctionType)(P1, P2, P3, P4);

        FunctionType Compile(Node<R>& expression);

        FunctionType GetEntryPoint() const;

    private:
        ParameterNode<P1>* m_p1;
        ParameterNode<P2>* m_p2;
        ParameterNode<P3>* m_p3;
        ParameterNode<P4>* m_p4;
    };


    template <typename R, typename P1, typename P2, typename P3>
    class Function<R, P1, P2, P3> : public FunctionBase<R>
    {
    public:
        Function(Allocators::IAllocator& allocator, FunctionBuffer& code);

        ParameterNode<P1>& GetP1() const;
        ParameterNode<P2>& GetP2() const;
        ParameterNode<P3>& GetP3() const;
		ParameterNode<P1>& GetP4() const { return *m_p1; };

        typedef R (*FunctionType)(P1, P2, P3);

        FunctionType Compile(Node<R>& expression);

        FunctionType GetEntryPoint() const;

    private:
        ParameterNode<P1>* m_p1;
        ParameterNode<P2>* m_p2;
        ParameterNode<P3>* m_p3;
    };


    template <typename R, typename P1, typename P2>
    class Function<R, P1, P2> : public FunctionBase<R>
    {
    public:
        Function(Allocators::IAllocator& allocator, FunctionBuffer& code);

        ParameterNode<P1>& GetP1() const;
        ParameterNode<P2>& GetP2() const;
		ParameterNode<P1>& GetP3() const { return *m_p1; };
		ParameterNode<P1>& GetP4() const { return *m_p1; };

        typedef R (*FunctionType)(P1, P2);

        FunctionType Compile(Node<R>& expression);

        FunctionType GetEntryPoint() const;

    private:
        ParameterNode<P1>* m_p1;
        ParameterNode<P2>* m_p2;
    };


    template <typename R, typename P1>
    class Function<R, P1> : public FunctionBase<R>
    {
    public:
        Function(Allocators::IAllocator& allocator, FunctionBuffer& code);

        ParameterNode<P1>& GetP1() const;
		ParameterNode<P1>& GetP2() const { return *m_p1; };
		ParameterNode<P1>& GetP3() const { return *m_p1; };
		ParameterNode<P1>& GetP4() const { return *m_p1; };

        typedef R (*FunctionType)(P1);

        FunctionType Compile(Node<R>& expression);

        FunctionType GetEntryPoint() const;

    private:
        ParameterNode<P1>* m_p1;
    };


    template <typename R>
    class Function<R> : public FunctionBase<R>
    {
    public:
        Function(Allocators::IAllocator& allocator, FunctionBuffer& code);

		ParameterNode<R>& GetP1() const { return *m_p1; };
		ParameterNode<R>& GetP2() const { return *m_p1; };
		ParameterNode<R>& GetP3() const { return *m_p1; };
		ParameterNode<R>& GetP4() const { return *m_p1; };

        typedef R (*FunctionType)();

        FunctionType Compile(Node<R>& expression);

        FunctionType GetEntryPoint() const;

		ParameterNode<R>* m_p1;
    };


    //*************************************************************************
    //
    // FunctionBase<R> template definitions.
    //
    //*************************************************************************
    template <typename R>
    FunctionBase<R>::FunctionBase(Allocators::IAllocator& allocator,
                                  FunctionBuffer& code)
        : ExpressionNodeFactory(allocator, code),
          m_allocator(allocator)
    {
        static_assert(IsValidParameter<R>::c_value, "R is an invalid type.");
    }


    template <typename R>
    template <JccType JCC>
    void FunctionBase<R>::AddExecuteOnlyIfStatement(FlagExpressionNode<JCC>& condition,
                                                    ImmediateNode<R>& otherwiseValue)
    {
        auto & test = PlacementConstruct<ExecuteOnlyIfStatement<R, JCC>>(condition, otherwiseValue);

        AddExecutionPreconditionTest(test);
    }


    //*************************************************************************
    //
    // Function<R, P1, P2, P3, P4> template definitions.
    //
    //*************************************************************************
    template <typename R, typename P1, typename P2, typename P3, typename P4>
    Function<R, P1, P2, P3, P4>::Function(Allocators::IAllocator& allocator,
                                          FunctionBuffer& code)
        : FunctionBase<R>(allocator, code)
    {
        static_assert(IsValidParameter<P1>::c_value, "P1 is an invalid type.");
        static_assert(IsValidParameter<P2>::c_value, "P2 is an invalid type.");
        static_assert(IsValidParameter<P3>::c_value, "P3 is an invalid type.");
        static_assert(IsValidParameter<P4>::c_value, "P4 is an invalid type.");

        ParameterSlotAllocator slotAllocator;
        m_p1 = &this->template Parameter<P1>(slotAllocator);
        m_p2 = &this->template Parameter<P2>(slotAllocator);
        m_p3 = &this->template Parameter<P3>(slotAllocator);
        m_p4 = &this->template Parameter<P4>(slotAllocator);
    }


    template <typename R, typename P1, typename P2, typename P3, typename P4>
    ParameterNode<P1>& Function<R, P1, P2, P3, P4>::GetP1() const
    {
        return *m_p1;
    }


    template <typename R, typename P1, typename P2, typename P3, typename P4>
    ParameterNode<P2>& Function<R, P1, P2, P3, P4>::GetP2() const
    {
        return *m_p2;
    }


    template <typename R, typename P1, typename P2, typename P3, typename P4>
    ParameterNode<P3>& Function<R, P1, P2, P3, P4>::GetP3() const
    {
        return *m_p3;
    }


    template <typename R, typename P1, typename P2, typename P3, typename P4>
    ParameterNode<P4>& Function<R, P1, P2, P3, P4>::GetP4() const
    {
        return *m_p4;
    }


    template <typename R, typename P1, typename P2, typename P3, typename P4>
    typename Function<R, P1, P2, P3, P4>::FunctionType
    Function<R, P1, P2, P3, P4>::Compile(Node<R>& value)
    {
        this->template Return<R>(value);
        ExpressionTree::Compile();
        return GetEntryPoint();
    }


    template <typename R, typename P1, typename P2, typename P3, typename P4>
    typename Function<R, P1, P2, P3, P4>::FunctionType
    Function<R, P1, P2, P3, P4>::GetEntryPoint() const
    {
        return reinterpret_cast<FunctionType>(const_cast<void*>(this->GetUntypedEntryPoint()));
    }


    //*************************************************************************
    //
    // Function<R, P1, P2, P3> template definitions.
    //
    //*************************************************************************
    template <typename R, typename P1, typename P2, typename P3>
    Function<R, P1, P2, P3>::Function(Allocators::IAllocator& allocator,
                                      FunctionBuffer& code)
        : FunctionBase<R>(allocator, code)
    {
        static_assert(IsValidParameter<P1>::c_value, "P1 is an invalid type.");
        static_assert(IsValidParameter<P2>::c_value, "P2 is an invalid type.");
        static_assert(IsValidParameter<P3>::c_value, "P3 is an invalid type.");

        ParameterSlotAllocator slotAllocator;
        m_p1 = &this->template Parameter<P1>(slotAllocator);
        m_p2 = &this->template Parameter<P2>(slotAllocator);
        m_p3 = &this->template Parameter<P3>(slotAllocator);
    }


    template <typename R, typename P1, typename P2, typename P3>
    ParameterNode<P1>& Function<R, P1, P2, P3>::GetP1() const
    {
        return *m_p1;
    }


    template <typename R, typename P1, typename P2, typename P3>
    ParameterNode<P2>& Function<R, P1, P2, P3>::GetP2() const
    {
        return *m_p2;
    }


    template <typename R, typename P1, typename P2, typename P3>
    ParameterNode<P3>& Function<R, P1, P2, P3>::GetP3() const
    {
        return *m_p3;
    }


    template <typename R, typename P1, typename P2, typename P3>
    typename Function<R, P1, P2, P3>::FunctionType
    Function<R, P1, P2, P3>::Compile(Node<R>& value)
    {
        this->template Return<R>(value);
        ExpressionTree::Compile();
        return GetEntryPoint();
    }


    template <typename R, typename P1, typename P2, typename P3>
    typename Function<R, P1, P2, P3>::FunctionType
    Function<R, P1, P2, P3>::GetEntryPoint() const
    {
        return reinterpret_cast<FunctionType>(const_cast<void*>(this->GetUntypedEntryPoint()));
    }


    //*************************************************************************
    //
    // Function<R, P1, P2> template definitions.
    //
    //*************************************************************************
    template <typename R, typename P1, typename P2>
    Function<R, P1, P2>::Function(Allocators::IAllocator& allocator,
                                  FunctionBuffer& code)
        : FunctionBase<R>(allocator, code)
    {
        static_assert(IsValidParameter<P1>::c_value, "P1 is an invalid type.");
        static_assert(IsValidParameter<P2>::c_value, "P2 is an invalid type.");

        ParameterSlotAllocator slotAllocator;
        m_p1 = &this->template Parameter<P1>(slotAllocator);
        m_p2 = &this->template Parameter<P2>(slotAllocator);
    }


    template <typename R, typename P1, typename P2>
    ParameterNode<P1>& Function<R, P1, P2>::GetP1() const
    {
        return *m_p1;
    }


    template <typename R, typename P1, typename P2>
    ParameterNode<P2>& Function<R, P1, P2>::GetP2() const
    {
        return *m_p2;
    }


    template <typename R, typename P1, typename P2>
    typename Function<R, P1, P2>::FunctionType
    Function<R, P1, P2>::Compile(Node<R>& value)
    {
        this->template Return<R>(value);
        ExpressionTree::Compile();
        return GetEntryPoint();
    }


    template <typename R, typename P1, typename P2>
    typename Function<R, P1, P2>::FunctionType
    Function<R, P1, P2>::GetEntryPoint() const
    {
        return reinterpret_cast<FunctionType>(const_cast<void*>(this->GetUntypedEntryPoint()));
    }


    //*************************************************************************
    //
    // Function<R, P1> template definitions.
    //
    //*************************************************************************
    template <typename R, typename P1>
    Function<R, P1>::Function(Allocators::IAllocator& allocator,
                              FunctionBuffer& code)
        : FunctionBase<R>(allocator, code)
    {
        static_assert(IsValidParameter<P1>::c_value, "P1 is an invalid type.");

        ParameterSlotAllocator slotAllocator;
        m_p1 = &this->template Parameter<P1>(slotAllocator);
    }


    template <typename R, typename P1>
    ParameterNode<P1>& Function<R, P1>::GetP1() const
    {
        return *m_p1;
    }


    template <typename R, typename P1>
    typename Function<R, P1>::FunctionType
    Function<R, P1>::Compile(Node<R>& value)
    {
        this->template Return<R>(value);
        ExpressionTree::Compile();
        return GetEntryPoint();
    }


    template <typename R, typename P1>
    typename Function<R, P1>::FunctionType
    Function<R, P1>::GetEntryPoint() const
    {
        return reinterpret_cast<FunctionType>(const_cast<void*>(this->GetUntypedEntryPoint()));
    }


    //*************************************************************************
    //
    // Function<R> template definitions.
    //
    //*************************************************************************
    template <typename R>
    Function<R>::Function(Allocators::IAllocator& allocator,
                          FunctionBuffer& code)
        : FunctionBase<R>(allocator, code)
    {
    }


    template <typename R>
    typename Function<R>::FunctionType  Function<R>::Compile(Node<R>& value)
    {
        this->template Return<R>(value);
        ExpressionTree::Compile();
        return GetEntryPoint();
    }


    template <typename R>
    typename Function<R>::FunctionType Function<R>::GetEntryPoint() const
    {
        return reinterpret_cast<FunctionType>(const_cast<void*>(this->GetUntypedEntryPoint()));
    }
}
