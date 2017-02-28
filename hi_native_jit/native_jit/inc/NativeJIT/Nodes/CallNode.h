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

#include <iostream>                    // Accessed by template definition for Print().

#include "../AllocatorVector.h" // Embedded member.
#include "../CodeGenHelpers.h"
#include "Node.h"      // Base class.
#include "../TypePredicates.h"

// https://software.intel.com/en-us/articles/introduction-to-x64-assembly

namespace NativeJIT
{
    class ExpressionTree;


    class SaveRestoreVolatilesHelper
    {
    protected:
        SaveRestoreVolatilesHelper(Allocators::IAllocator& allocator);

    public:
        // These methods need to be public for access by
        //     CallNodeBase::FunctionChild
        //     CallNodeBase::ParameterChild

        void SaveVolatiles(ExpressionTree& tree);
        void RestoreVolatiles(ExpressionTree& tree);

        template <unsigned SIZE, bool ISFLOAT>
        void RecordCallRegister(Register<SIZE, ISFLOAT> r, bool isSoleOwner);

    protected:
        // WARNING: This class is designed to be allocated by an arena allocator,
        // so its destructor will never be called. Therefore, it should hold no
        // resources other than memory from the arena allocator.
        // Would like to make this private and without method body, but it's
        // called implicitly by child class constructors if they throw.
        ~SaveRestoreVolatilesHelper() {}

    private:
        // Returns the mask for the registers it's necessary to preserve accross
        // the function call that's being made.
        template <bool ISFLOAT>
        unsigned GetRegistersToPreserve(ExpressionTree& tree) const;

        // A bit-mask of registers that are exclusively owned for the function
        // call and thus don't need to be preserved.
        unsigned m_rxxCallExclusiveRegisterMask;
        unsigned m_xmmCallExclusiveRegisterMask;

        // Temporary storage used to preserve volatile registers.
        AllocatorVector<Storage<void*>> m_preservationStorage;
    };


    template <typename R, unsigned PARAMETERCOUNT>
    class CallNodeBase : public Node<R>, public SaveRestoreVolatilesHelper
    {
    public:
        CallNodeBase(ExpressionTree& tree);

        //
        // Overrides of Node methods.
        //
        virtual ExpressionTree::Storage<R> CodeGenValue(ExpressionTree& tree) override;
        virtual void Print(std::ostream& out) const override;

    protected:
        // WARNING: This class is designed to be allocated by an arena allocator,
        // so its destructor will never be called. Therefore, it should hold no
        // resources other than memory from the arena allocator.
        // Would like to make this private and without method body, but it's
        // called implicitly by child class constructors if they throw.
        ~CallNodeBase() {}

    protected:

        class Child : private NonCopyable
        {
        public:
            // Generates the code to evaluate the expression inside the child.
            virtual void Evaluate(ExpressionTree& tree) = 0;

            // Emits the code to place the child's value into the appropriate
            // register or stack location depending on the type and position
            // of the child.
            // The staging must not modify any other registers reserved for the
            // function call by the calling convention, so the child must already
            // be evaluated before it can be staged.
            virtual void EmitStaging(ExpressionTree& tree, SaveRestoreVolatilesHelper& volatiles) = 0;

            // Releases any registers used during the evaluation of the child
            // expression in Evaluate().
            virtual void Release() = 0;

            // Prints the contents of the child to standard output for debugging.
            virtual void Print(std::ostream& out) const = 0;
        };


        template <typename T>
        class TypedChild : public Child
        {
        public:
            TypedChild(Node<T>& expression);

            //
            // Overrides of Child methods.
            //
            virtual void Release();

        protected:
            // Pins the storage register so that it cannot be spilled until
            // the Release() call.
            void PinStorageRegister();

            Node<T>& m_expression;
            ExpressionTree::Storage<T> m_storage;
            ReferenceCounter m_registerPin;
        };


        template <typename T>
        class ParameterChild : public TypedChild<T>
        {
        public:
            ParameterChild(Node<T>& expression, unsigned position);

            typename ExpressionTree::Storage<T>::DirectRegister GetRegister() const;

            //
            // Overrides of Child methods.
            //
            virtual void Evaluate(ExpressionTree& tree) override;
            virtual void EmitStaging(ExpressionTree& tree,
                                     SaveRestoreVolatilesHelper& volatiles) override;
            virtual void Print(std::ostream& out) const override;

        private:
            typename ExpressionTree::Storage<T>::DirectRegister m_destination;
        };


        class FunctionChildBase
        {
        public:
            virtual void EmitCall(ExpressionTree& tree) = 0;
        };


        template <typename T>
        class FunctionChild : public FunctionChildBase, public TypedChild<T>
        {
        public:
            FunctionChild(Node<T>& expression,
                          typename Storage<R>::DirectRegister resultRegister);

            //
            // Overrides of Child methods.
            //
            virtual void Evaluate(ExpressionTree& tree) override;
            virtual void EmitStaging(ExpressionTree& tree,
                                     SaveRestoreVolatilesHelper& volatiles) override;

            //
            // Overrides of FunctionChildBase methods.
            //
            virtual void EmitCall(ExpressionTree& tree) override;
            virtual void Print(std::ostream& out) const override;

        private:
            typename Storage<R>::DirectRegister m_resultRegister;
        };

        // One child for each parameter plus one for the function pointer.
        static const unsigned c_childCount = PARAMETERCOUNT + 1;
        Child* m_children[c_childCount];

        // Pointer to function's two base classes.
        FunctionChildBase* m_functionBase;
        Child* m_functionChild;
    };


    template <typename R, typename P1 = void, typename P2 = void, typename P3 = void, typename P4 = void>
    class CallNode;


    template <typename R>
    class CallNode<R> : public CallNodeBase<R, 0>
    {
    public:
        typedef R (*FunctionPointer)();

        CallNode(ExpressionTree& tree,
                 Node<FunctionPointer>& function);

    private:
        // WARNING: This class is designed to be allocated by an arena allocator,
        // so its destructor will never be called. Therefore, it should hold no
        // resources other than memory from the arena allocator.
        ~CallNode();

        typename CallNodeBase<R, 0>::template FunctionChild<FunctionPointer> m_f;
    };


    template <typename R, typename P1>
    class CallNode<R, P1> : public CallNodeBase<R, 1>
    {
    public:
        typedef R (*FunctionPointer)(P1);

        CallNode(ExpressionTree& tree,
                 Node<FunctionPointer>& function,
                 Node<P1>& p1);

    private:
        // WARNING: This class is designed to be allocated by an arena allocator,
        // so its destructor will never be called. Therefore, it should hold no
        // resources other than memory from the arena allocator.
        ~CallNode();

        typename CallNodeBase<R, 1>::template FunctionChild<FunctionPointer> m_f;
        typename CallNodeBase<R, 1>::template ParameterChild<P1> m_p1;
    };


    template <typename R, typename P1, typename P2>
    class CallNode<R, P1, P2> : public CallNodeBase<R, 2>
    {
    public:
        typedef R (*FunctionPointer)(P1, P2);

        CallNode(ExpressionTree& tree,
                 Node<FunctionPointer>& function,
                 Node<P1>& p1,
                 Node<P2>& p2);

    private:
        // WARNING: This class is designed to be allocated by an arena allocator,
        // so its destructor will never be called. Therefore, it should hold no
        // resources other than memory from the arena allocator.
        ~CallNode();

        typename CallNodeBase<R, 2>::template FunctionChild<FunctionPointer> m_f;
        typename CallNodeBase<R, 2>::template ParameterChild<P1> m_p1;
        typename CallNodeBase<R, 2>::template ParameterChild<P2> m_p2;
    };


    template <typename R, typename P1, typename P2, typename P3>
    class CallNode<R, P1, P2, P3> : public CallNodeBase<R, 3>
    {
    public:
        typedef R (*FunctionPointer)(P1, P2, P3);

        CallNode(ExpressionTree& tree,
                 Node<FunctionPointer>& function,
                 Node<P1>& p1,
                 Node<P2>& p2,
                 Node<P3>& p3);

    private:
        // WARNING: This class is designed to be allocated by an arena allocator,
        // so its destructor will never be called. Therefore, it should hold no
        // resources other than memory from the arena allocator.
        ~CallNode();

        typename CallNodeBase<R, 3>::template FunctionChild<FunctionPointer> m_f;
        typename CallNodeBase<R, 3>::template ParameterChild<P1> m_p1;
        typename CallNodeBase<R, 3>::template ParameterChild<P2> m_p2;
        typename CallNodeBase<R, 3>::template ParameterChild<P3> m_p3;
    };


    template <typename R, typename P1, typename P2, typename P3, typename P4>
    class CallNode : public CallNodeBase<R, 4>
    {
    public:
        typedef R (*FunctionPointer)(P1, P2, P3, P4);

        CallNode(ExpressionTree& tree,
                 Node<FunctionPointer>& function,
                 Node<P1>& p1,
                 Node<P2>& p2,
                 Node<P3>& p3,
                 Node<P4>& p4);

    private:
        // WARNING: This class is designed to be allocated by an arena allocator,
        // so its destructor will never be called. Therefore, it should hold no
        // resources other than memory from the arena allocator.
        ~CallNode();

        typename CallNodeBase<R, 4>::template FunctionChild<FunctionPointer> m_f;
        typename CallNodeBase<R, 4>::template ParameterChild<P1> m_p1;
        typename CallNodeBase<R, 4>::template ParameterChild<P2> m_p2;
        typename CallNodeBase<R, 4>::template ParameterChild<P3> m_p3;
        typename CallNodeBase<R, 4>::template ParameterChild<P4> m_p4;
    };


    //*************************************************************************
    //
    // Template definitions for SaveRestoreVolatilesHelper.
    //
    //*************************************************************************
    template <unsigned SIZE, bool ISFLOAT>
    void SaveRestoreVolatilesHelper::RecordCallRegister(Register<SIZE, ISFLOAT> r, bool isSoleOwner)
    {
        if (isSoleOwner)
        {
            BitOp::SetBit(ISFLOAT ? &m_xmmCallExclusiveRegisterMask : &m_rxxCallExclusiveRegisterMask,
                          r.GetId());
        }
    }


    //*************************************************************************
    //
    // Template definitions for CallNodeBase<R, PARAMETERCOUNT>
    //
    //*************************************************************************
    template <typename R, unsigned PARAMETERCOUNT>
    CallNodeBase<R, PARAMETERCOUNT>::CallNodeBase(ExpressionTree& tree)
        : Node<R>(tree),
          SaveRestoreVolatilesHelper(tree.GetAllocator())
    {
        static_assert(IsValidParameter<R>::c_value, "R is an invalid type.");
        tree.ReportFunctionCallNode(PARAMETERCOUNT);
    }


    template <typename R, unsigned PARAMETERCOUNT>
    ExpressionTree::Storage<R> CallNodeBase<R, PARAMETERCOUNT>::CodeGenValue(ExpressionTree& tree)
    {
        // Make sure that the result register is not pinned at this point.
        auto const resultRegister = tree.GetResultRegister<R>();
        LogThrowAssert(!tree.IsPinned(resultRegister), "The result register must not be pinned before the call");

        // Evaluate the function pointer and each parameter.
        for (Child* child : m_children)
        {
            child->Evaluate(tree);
        }

        for (Child* child : m_children)
        {
            // Stage the parameters first since they need to be placed into
            // fixed registers.
            if (child != m_functionChild)
            {
                child->EmitStaging(tree, *this);
            }
        }

        // Stage the function pointer into a register last since it can be staged
        // staged into any register.
        m_functionChild->EmitStaging(tree, *this);

        // If the result register is still not pinned (and thus unused by the
        // call), enforce ownership over it.
        if (!tree.IsPinned(resultRegister))
        {
            // Release the register right away by not keeping the Storage.
            tree.Direct<R>(resultRegister);
            RecordCallRegister(resultRegister, true);
        }

        SaveVolatiles(tree);
        m_functionBase->EmitCall(tree);
        RestoreVolatiles(tree);

        // Free up registers used for function pointer and parameters.
        for (Child* child : m_children)
        {
            child->Release();
        }

        // At this point, the result register was either used by a parameter or
        // the function pointer and then released, or it was empty after the
        // explicit bump further above so the following call will bump nothing.
        return tree.Direct<R>(resultRegister);
    }


    template <typename R, unsigned PARAMETERCOUNT>
    void CallNodeBase<R, PARAMETERCOUNT>::Print(std::ostream& out) const
    {
        this->PrintCoreProperties(out, "CallNode");

        for (unsigned i = 0 ; i < c_childCount; ++i)
        {
            out << ", ";
            m_children[i]->Print(out);
        }
    }


    //*************************************************************************
    //
    // Template definitions for
    //     CallNodeBase<R, PARAMETERCOUNT>::TypedChild<T>
    //
    //*************************************************************************
    template <typename R, unsigned PARAMETERCOUNT>
    template <typename T>
    CallNodeBase<R, PARAMETERCOUNT>::TypedChild<T>::TypedChild(Node<T>& expression)
        : m_expression(expression)
    {
        m_expression.IncrementParentCount();
    }


    template <typename R, unsigned PARAMETERCOUNT>
    template <typename T>
    void CallNodeBase<R, PARAMETERCOUNT>::TypedChild<T>::Release()
    {
        // Release the register pin and reset the storage to release the register.
        m_registerPin.Reset();
        m_storage.Reset();
    }


    template <typename R, unsigned PARAMETERCOUNT>
    template <typename T>
    void CallNodeBase<R, PARAMETERCOUNT>::TypedChild<T>::PinStorageRegister()
    {
        LogThrowAssert(!m_storage.IsNull(), "Storage must be initialized");

        if (m_storage.GetStorageClass() == StorageClass::Direct)
        {
            m_registerPin = m_storage.GetPin();
        }
    }


    //*************************************************************************
    //
    // Template definitions for
    //     CallNodeBase<R, PARAMETERCOUNT>::FunctionChild<F>
    //
    //*************************************************************************
    template <typename R, unsigned PARAMETERCOUNT>
    template <typename F>
    CallNodeBase<R, PARAMETERCOUNT>::FunctionChild<F>::FunctionChild(
        Node<F>& expression,
        typename Storage<R>::DirectRegister resultRegister)
        : TypedChild<F>(expression),
          m_resultRegister(resultRegister)
    {
    }


    template <typename R, unsigned PARAMETERCOUNT>
    template <typename F>
    void CallNodeBase<R, PARAMETERCOUNT>::FunctionChild<F>::Evaluate(ExpressionTree& tree)
    {
        this->m_storage = this->m_expression.CodeGen(tree);
    }


    template <typename R, unsigned PARAMETERCOUNT>
    template <typename F>
    void CallNodeBase<R, PARAMETERCOUNT>::FunctionChild<F>::EmitStaging(ExpressionTree& /* tree */,
                                                                        SaveRestoreVolatilesHelper& volatiles)
    {
        auto & storage = this->m_storage;

        // The CALL instruction requires a direct register, ensure that's the case.
        // Convert for modification to ensure that the register doesn't need
        // to be preserved accross the call.
        if (storage.GetStorageClass() != StorageClass::Direct)
        {
            storage.ConvertToDirect(true);
        }

        // If function pointer happens to be in the result register and there
        // are other owners, they must be spilled out of the register. Otherwise,
        // the attempt to restore the contents into the result register after
        // the call would overwrite the returned result.
        if (!storage.IsSoleDataOwner()
            && storage.GetDirectRegister().IsSameHardwareRegister(m_resultRegister))
        {
            storage.TakeSoleOwnershipOfDirect();
        }

        volatiles.RecordCallRegister(storage.GetDirectRegister(),
                                     storage.IsSoleDataOwner());

        // Make sure that nothing takes away this register.
        // NOTE: This depends on the fact that the function pointer is staged
        // last to ensure that a fixked parameter register is not picked.
        this->PinStorageRegister();
    }


    template <typename R, unsigned PARAMETERCOUNT>
    template <typename F>
    void CallNodeBase<R, PARAMETERCOUNT>::FunctionChild<F>::EmitCall(ExpressionTree& tree)
    {
        tree.GetCodeGenerator().Emit<OpCode::Call>(this->m_storage.GetDirectRegister());
    }


    template <typename R, unsigned PARAMETERCOUNT>
    template <typename F>
    void CallNodeBase<R, PARAMETERCOUNT>::FunctionChild<F>::Print(std::ostream& out) const
    {
        out << "function(" << this->m_expression.GetId() << ")";
    }


    //*************************************************************************
    //
    // Template definitions for
    //     CallNodeBase<R, PARAMETERCOUNT>::ParameterChild<F>
    //
    //*************************************************************************
    template <typename R, unsigned PARAMETERCOUNT>
    template <typename T>
    CallNodeBase<R, PARAMETERCOUNT>::ParameterChild<T>::ParameterChild(Node<T>& expression, unsigned position)
        : TypedChild<T>(expression)
    {
        GetParameterRegister(position, m_destination);
    }


    template <typename R, unsigned PARAMETERCOUNT>
    template <typename T>
    void CallNodeBase<R, PARAMETERCOUNT>::ParameterChild<T>::Evaluate(ExpressionTree& tree)
    {
        this->m_storage = this->m_expression.CodeGen(tree);
    }


    template <typename R, unsigned PARAMETERCOUNT>
    template <typename T>
    void CallNodeBase<R, PARAMETERCOUNT>::ParameterChild<T>::EmitStaging(ExpressionTree& tree,
                                                                         SaveRestoreVolatilesHelper& volatiles)
    {
        if (this->m_storage.GetStorageClass() != StorageClass::Direct
            || !this->m_storage.GetDirectRegister().IsSameHardwareRegister(m_destination))
        {
            ExpressionTree::Storage<T> regStorage = tree.Direct<T>(m_destination);
            CodeGenHelpers::Emit<OpCode::Mov>(tree.GetCodeGenerator(), m_destination, this->m_storage);
            this->m_storage = regStorage;
        }

        // DESIGN NOTE: There's room for optimization if the data was already in the
        // correct register and shared. If there are some free non-volatile
        // registers, it would be better to enforce sole ownership of m_storage
        // by spilling to such register (the callee may not be using it and may
        // not need to push it to/pop it from the stack). Currently, in such
        // case the register will always get pushed/popped by SaveRestoreVolatilesHelper.

        volatiles.RecordCallRegister(this->m_storage.GetDirectRegister(),
                                     this->m_storage.IsSoleDataOwner());

        // The parameter needs to remain in the specified register.
        this->PinStorageRegister();
    }


    template <typename R, unsigned PARAMETERCOUNT>
    template <typename T>
    void CallNodeBase<R, PARAMETERCOUNT>::ParameterChild<T>::Print(std::ostream& out) const
    {
        out << "parameter(" << this->m_expression.GetId() << ")";
    }


    //*************************************************************************
    //
    // Template definitions for CallNode<R, P1, P2>
    //
    //*************************************************************************
    template <typename R>
    CallNode<R>::CallNode(ExpressionTree& tree,
                          Node<FunctionPointer>& function)
        : CallNodeBase<R, 0>(tree),
          m_f(function, tree.GetResultRegister<R>())
    {
        static_assert(IsValidParameter<R>::c_value, "R is an invalid type.");

        this->m_functionBase = &m_f;
        this->m_functionChild = &m_f;
        this->m_children[0] = this->m_functionChild;
    }


    template <typename R, typename P1>
    CallNode<R, P1>::CallNode(ExpressionTree& tree,
                                  Node<FunctionPointer>& function,
                                  Node<P1>& p1)
        : CallNodeBase<R, 1>(tree),
          m_f(function, tree.GetResultRegister<R>()),
          m_p1(p1, 0)
    {
        static_assert(IsValidParameter<R>::c_value, "R is an invalid type.");
        static_assert(IsValidParameter<P1>::c_value, "P1 is an invalid type.");

        this->m_functionBase = &m_f;
        this->m_functionChild = &m_f;
        this->m_children[0] = this->m_functionChild;
        this->m_children[1] = &m_p1;
    }


    template <typename R, typename P1, typename P2>
    CallNode<R, P1, P2>::CallNode(ExpressionTree& tree,
                                  Node<FunctionPointer>& function,
                                  Node<P1>& p1,
                                  Node<P2>& p2)
        : CallNodeBase<R, 2>(tree),
          m_f(function, tree.GetResultRegister<R>()),
          m_p1(p1, 0),
          m_p2(p2, 1)
    {
        static_assert(IsValidParameter<R>::c_value, "R is an invalid type.");
        static_assert(IsValidParameter<P1>::c_value, "P1 is an invalid type.");
        static_assert(IsValidParameter<P2>::c_value, "P2 is an invalid type.");

        this->m_functionBase = &m_f;
        this->m_functionChild = &m_f;
        this->m_children[0] = this->m_functionChild;
        this->m_children[1] = &m_p1;
        this->m_children[2] = &m_p2;
    }


    template <typename R, typename P1, typename P2, typename P3>
    CallNode<R, P1, P2, P3>::CallNode(ExpressionTree& tree,
                                      Node<FunctionPointer>& function,
                                      Node<P1>& p1,
                                      Node<P2>& p2,
                                      Node<P3>& p3)
        : CallNodeBase<R, 3>(tree),
          m_f(function, tree.GetResultRegister<R>()),
          m_p1(p1, 0),
          m_p2(p2, 1),
          m_p3(p3, 2)
    {
        static_assert(IsValidParameter<R>::c_value, "R is an invalid type.");
        static_assert(IsValidParameter<P1>::c_value, "P1 is an invalid type.");
        static_assert(IsValidParameter<P2>::c_value, "P2 is an invalid type.");
        static_assert(IsValidParameter<P3>::c_value, "P3 is an invalid type.");

        this->m_functionBase = &m_f;
        this->m_functionChild = &m_f;
        this->m_children[0] = this->m_functionChild;
        this->m_children[1] = &m_p1;
        this->m_children[2] = &m_p2;
        this->m_children[3] = &m_p3;
    }


    template <typename R, typename P1, typename P2, typename P3, typename P4>
    CallNode<R, P1, P2, P3, P4>::CallNode(ExpressionTree& tree,
                                          Node<FunctionPointer>& function,
                                          Node<P1>& p1,
                                          Node<P2>& p2,
                                          Node<P3>& p3,
                                          Node<P4>& p4)
        : CallNodeBase<R, 4>(tree),
          m_f(function, tree.GetResultRegister<R>()),
          m_p1(p1, 0),
          m_p2(p2, 1),
          m_p3(p3, 2),
          m_p4(p4, 3)
    {
        static_assert(IsValidParameter<R>::c_value, "R is an invalid type.");
        static_assert(IsValidParameter<P1>::c_value, "P1 is an invalid type.");
        static_assert(IsValidParameter<P2>::c_value, "P2 is an invalid type.");
        static_assert(IsValidParameter<P3>::c_value, "P3 is an invalid type.");
        static_assert(IsValidParameter<P4>::c_value, "P4 is an invalid type.");

        this->m_functionBase = &m_f;
        this->m_functionChild = &m_f;
        this->m_children[0] = this->m_functionChild;
        this->m_children[1] = &m_p1;
        this->m_children[2] = &m_p2;
        this->m_children[3] = &m_p3;
        this->m_children[4] = &m_p4;
    }
}
