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

#include <algorithm>    // For std::max

#include "../CodeGen/X64CodeGenerator.h"
#include "../CodeGenHelpers.h"
#include "../ExpressionTree.h"
#include "Node.h"


namespace NativeJIT
{
    class ExpressionTree;

    template <JccType JCC>
    class FlagExpressionNode : public Node<bool>
    {
    public:
        FlagExpressionNode(ExpressionTree& tree);

        //
        // Overrides of NodeBase methods.
        //
        virtual bool CanBeOptimizedAway() const override;

        //
        // Overrides of Node<T> methods.
        //
        virtual void CodeGenFlags(ExpressionTree& tree) = 0;


        // Increments the number of parents that use the node's CodeGenFlags()
        // method rather than the usual CodeGen() method.
        void IncrementFlagsParentCount();

    protected:
        // WARNING: This class is designed to be allocated by an arena allocator,
        // so its destructor will never be called. Therefore, it should hold no
        // resources other than memory from the arena allocator.
        // Would like to make this private and without method body, but it's
        // called implicitly by child class constructors if they throw.
        ~FlagExpressionNode() {}

    private:
        unsigned m_flagsParentCount;
    };


    template <typename T, JccType JCC>
    class ConditionalNode : public Node<T>
    {
    public:
        ConditionalNode(ExpressionTree& tree,
                        FlagExpressionNode<JCC>& condition,
                        Node<T>& trueExpression,
                        Node<T>& falseExpression);


        //
        // Overrides of Node methods.
        //
        virtual void Print(std::ostream& out) const override;

        //
        // Overrides of Node<T> methods.
        //
        virtual ExpressionTree::Storage<T> CodeGenValue(ExpressionTree& tree) override;

    private:
        // WARNING: This class is designed to be allocated by an arena allocator,
        // so its destructor will never be called. Therefore, it should hold no
        // resources other than memory from the arena allocator.
        ~ConditionalNode();

        FlagExpressionNode<JCC>& m_condition;
        Node<T>& m_trueExpression;
        Node<T>& m_falseExpression;
    };


    template <typename T, JccType JCC>
    class RelationalOperatorNode : public FlagExpressionNode<JCC>
    {
    public:
        RelationalOperatorNode(ExpressionTree& tree,
                               Node<T>& left,
                               Node<T>& right);


        //
        // Overrides of Node methods.
        //
        virtual void Print(std::ostream& out) const override;


        //
        // Overrides of Node<T> methods.
        //
        virtual ExpressionTree::Storage<bool> CodeGenValue(ExpressionTree& tree) override;


        //
        // Overrides of FlagExpression methods.
        //
        virtual void CodeGenFlags(ExpressionTree& tree) override;

    private:
        // WARNING: This class is designed to be allocated by an arena allocator,
        // so its destructor will never be called. Therefore, it should hold no
        // resources other than memory from the arena allocator.
        ~RelationalOperatorNode();

        Node<T>& m_left;
        Node<T>& m_right;
    };


    //*************************************************************************
    //
    // Template definitions for FlagExpressionNode
    //
    //*************************************************************************
    template <JccType JCC>
    FlagExpressionNode<JCC>::FlagExpressionNode(ExpressionTree& tree)
        : Node<bool>(tree),
          m_flagsParentCount(0)
    {
    }


    template <JccType JCC>
    bool FlagExpressionNode<JCC>::CanBeOptimizedAway() const
    {
        // The node can be optimized away only if it's unused in both contexts
        // it can be referrenced from (through CodeGenValue() and CodeGenFlags()).
        return Node<bool>::CanBeOptimizedAway() && m_flagsParentCount == 0;
    }


    template <JccType JCC>
    void FlagExpressionNode<JCC>::IncrementFlagsParentCount()
    {
        ++m_flagsParentCount;
        MarkReferenced();
    }


    //*************************************************************************
    //
    // Template definitions for ConditionalNode
    //
    //*************************************************************************
    template <typename T, JccType JCC>
    ConditionalNode<T, JCC>::ConditionalNode(ExpressionTree& tree,
                                             FlagExpressionNode<JCC>& condition,
                                             Node<T>& trueExpression,
                                             Node<T>& falseExpression)
        : Node<T>(tree),
          m_condition(condition),
          m_trueExpression(trueExpression),
          m_falseExpression(falseExpression)
    {
        m_trueExpression.IncrementParentCount();
        m_falseExpression.IncrementParentCount();

        // Use the CodeGenFlags()-related call.
        m_condition.IncrementFlagsParentCount();
    }


    template <typename T, JccType JCC>
    void ConditionalNode<T, JCC>::Print(std::ostream& out) const
    {
        const std::string name = std::string("Conditional(")
            + X64CodeGenerator::JccName(JCC)
            + ") ";
        this->PrintCoreProperties(out, name.c_str());

        out << ", condition = " << m_condition.GetId();
        out << ", trueExpression = " << m_trueExpression.GetId();
        out << ", falseExpression = " << m_falseExpression.GetId();
    }


    template <typename T, JccType JCC>
    typename ExpressionTree::Storage<T> ConditionalNode<T, JCC>::CodeGenValue(ExpressionTree& tree)
    {
        X64CodeGenerator& code = tree.GetCodeGenerator();

        Label conditionIsTrue = code.AllocateLabel();
        Label testCompleted = code.AllocateLabel();

        // TODO: Evaluating both expressions in advance of the test is
        // sub-optimal, but it is currently required to guarantee consistent
        // state: the execution in NativeJIT has a continuous flow regardless
        // of the outcome of the (runtime) condition whereas the generated x64
        // code has two branches and each of them can have independent impact on
        // allocated and spilled registers. To have each expression evaluated
        // only if its branch is hit, the NativeJIT code would have to ensure
        // that the state of the allocated/spilled registers (i.e. all Storages)
        // is consistent once those two x64 branches converge back.
        //
        // A related note: if this is addressed, then it may be possible to
        // also address the fact that common subexpression (CSE) code is
        // inefficient in some cases because some CSEs may not need to be
        // evaluated. For example:
        //   (v == 1)? a : ((v == 2) ? a + b : b + c)
        //
        // Depending on the value of v, either a or c may not need to be
        // evaluated. One possibility is to lazily evaluate CSEs *at runtime*
        // as they are needed.
        // See bug#27

        Storage<T> trueValue;
        Storage<T> falseValue;

        this->CodeGenInOrder(tree,
                             m_trueExpression, trueValue,
                             m_falseExpression, falseValue);

        // Enum that specifies whether the result storage currently holds the
        // true value, false value or neither of them.
        enum class ResultContents { NeitherValue, TrueValue, FalseValue };
        ResultContents resultContents;
        Storage<T> result;

        {
            // Evaluate the condition to update the CPU flags. No code in this
            // block is allowed to modify the flags up until the
            // EmitConditionalJump() call at the end. Spilling (i.e. the MOV
            // instructions used used to copy the spilled value from register
            // to stack) does not affect any flags.
            m_condition.CodeGenFlags(tree);

            // Try to re-use a direct register from true/false expressions if
            // possible, otherwise allocate a register. The allocation must be
            // done before the conditional jump so that any register spills
            // apply to both branches.
            if (trueValue.GetStorageClass() == StorageClass::Direct
                && trueValue.IsSoleDataOwner())
            {
                result = trueValue;
                resultContents = ResultContents::TrueValue;
            }
            else if (falseValue.GetStorageClass() == StorageClass::Direct
                     && falseValue.IsSoleDataOwner())
            {
                result = falseValue;
                resultContents = ResultContents::FalseValue;
            }
            else
            {
                result = tree.Direct<T>();
                resultContents = ResultContents::NeitherValue;
            }

            code.EmitConditionalJump<JCC>(conditionIsTrue);
        }

        // Emit the code for the "condition is false" branch.

        // Move the false value to the result register unless it's already there.
        if (resultContents != ResultContents::FalseValue)
        {
            CodeGenHelpers::Emit<OpCode::Mov>(code, result.GetDirectRegister(), falseValue);
        }

        // Jump behind the true branch, unless the true branch is empty. The true
        // branch is empty only if the true value is already in the result storage.
        if (!(resultContents == ResultContents::TrueValue))
        {
            code.Jmp(testCompleted);
        }

        // Emit the code for the "condition is true" branch.

        code.PlaceLabel(conditionIsTrue);

        // Move the true value in the result register unless it's already there.
        if (resultContents != ResultContents::TrueValue)
        {
            CodeGenHelpers::Emit<OpCode::Mov>(code, result.GetDirectRegister(), trueValue);
        }

        code.PlaceLabel(testCompleted);

        return result;
    }


    //*************************************************************************
    //
    // Template definitions for RelationalOperator
    //
    //*************************************************************************
    template <typename T, JccType JCC>
    RelationalOperatorNode<T, JCC>::RelationalOperatorNode(ExpressionTree& tree,
                                                           Node<T>& left,
                                                           Node<T>& right)
        : FlagExpressionNode<JCC>(tree),
          m_left(left),
          m_right(right)
    {
        m_left.IncrementParentCount();
        m_right.IncrementParentCount();
    }


    template <typename T, JccType JCC>
    void RelationalOperatorNode<T, JCC>::Print(std::ostream& out) const
    {
        const std::string name = std::string("RelationalOperatorNode(")
            + X64CodeGenerator::JccName(JCC)
            + ") ";
        this->PrintCoreProperties(out, name.c_str());

        out << ", left = " << m_left.GetId();
        out << ", right = " << m_right.GetId();
    }


    template <typename T, JccType JCC>
    typename ExpressionTree::Storage<bool> RelationalOperatorNode<T, JCC>::CodeGenValue(ExpressionTree& tree)
    {
        X64CodeGenerator& code = tree.GetCodeGenerator();

        Label conditionIsTrue = code.AllocateLabel();
        Label testCompleted = code.AllocateLabel();

        // Evaluate the condition and react based on it.
        CodeGenFlags(tree);
        // Allocate the result register before the conditional jump so that
        // if any register gets spilled, the spill applies to both branches.
        // The spilling (i.e. the MOV instruction that is used to copy the
        // spilled value from the register onto stack) does not affect any flags.
        auto result = tree.Direct<bool>();
        code.EmitConditionalJump<JCC>(conditionIsTrue);

        code.EmitImmediate<OpCode::Mov>(result.GetDirectRegister(), false);
        code.Jmp(testCompleted);

        code.PlaceLabel(conditionIsTrue);
        code.EmitImmediate<OpCode::Mov>(result.GetDirectRegister(), true);

        code.PlaceLabel(testCompleted);

        return result;
    }


    template <typename T, JccType JCC>
    void RelationalOperatorNode<T, JCC>::CodeGenFlags(ExpressionTree& tree)
    {
        Storage<T> sLeft;
        Storage<T> sRight;

        this->CodeGenInOrder(tree,
                             m_left, sLeft,
                             m_right, sRight);

        CodeGenHelpers::Emit<OpCode::Cmp>(tree.GetCodeGenerator(), sLeft.ConvertToDirect(false), sRight);
    }
}
