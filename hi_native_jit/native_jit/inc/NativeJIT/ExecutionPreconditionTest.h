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

#include "CodeGen/X64CodeGenerator.h"
#include "ExpressionTree.h"
#include "Nodes/ConditionalNode.h"
#include "Nodes/ImmediateNode.h"


namespace NativeJIT
{
    // A base class for statements that check whether a precondition for executing
    // the full expression has been met and cause a fixed value to be returned
    // if not.
    class ExecutionPreconditionTest : private NonCopyable
    {
    public:
        // Executes its test and allows the regular flow to continue if test's
        // condition is satisfied. Otherwise, places an alternative fixed value
        // into the return register and jumps to function's epilog.
        virtual void Evaluate(ExpressionTree& tree) = 0;
    };


    // A class implementing a statement that causes the function either to start
    // executing if a precondition has been met or to return an immediate value
    // otherwise.
    //
    // DESIGN NOTE: something similar can be achieved with the If() node-based
    // expression at the root of the expression tree, but that may be very
    // inefficient: the expression tree will pre-evaluate nodes with multiple
    // parents first and then start evaluating the expression. Almost all of the
    // pre-evaluated nodes are irrelevant to the early return test, so their X64
    // code may be executed needlessly at runtime. This is an issue in cases
    // when the function is executed a large number of times for different
    // inputs where the vast majority of them is expected to return early.
    template <typename T, JccType JCC>
    class ExecuteOnlyIfStatement : public ExecutionPreconditionTest
    {
    public:
        ExecuteOnlyIfStatement(FlagExpressionNode<JCC>& condition,
                               ImmediateNode<T>& otherwiseValue);

        //
        // Overrides of ExecutionPreconditionTest.
        //
        virtual void Evaluate(ExpressionTree& tree) override;

    private:
        FlagExpressionNode<JCC>& m_condition;
        ImmediateNode<T>& m_otherwiseValue;
    };


    //
    // Template definitions for ExecuteOnlyIfStatement.
    //

    template <typename T, JccType JCC>
    ExecuteOnlyIfStatement<T, JCC>::ExecuteOnlyIfStatement(
        FlagExpressionNode<JCC>& condition,
        ImmediateNode<T>& otherwiseValue)
        : m_condition(condition),
          m_otherwiseValue(otherwiseValue)
    {
        m_otherwiseValue.IncrementParentCount();

        // Use the CodeGenFlags()-related call.
        m_condition.IncrementFlagsParentCount();
    }


    template <typename T, JccType JCC>
    void ExecuteOnlyIfStatement<T, JCC>::Evaluate(ExpressionTree& tree)
    {
        X64CodeGenerator& code = tree.GetCodeGenerator();
        Label continueWithRegularFlow = code.AllocateLabel();

        // Evaluate the condition to update the CPU flags. If condition is
        // satisfied, continue with the regular flow.
        m_condition.CodeGenFlags(tree);
        code.EmitConditionalJump<JCC>(continueWithRegularFlow);

        // Otherwise, return early with the constant value: move the constant
        // into the return register and jump to epilog.
        auto resultRegister = tree.GetResultRegister<T>();

        // IMPORTANT: CodeGen() for ImmediateNode will not cause any registers
        // to be spilled or modified. Otherwise, NativeJIT's view of the register
        // allocation would get out of sync with the actual state due to the
        // conditional jump above. See the comment in
        // ConditionalNode::CodeGenValue for more details.
        auto otherwiseValue = m_otherwiseValue.CodeGen(tree);

        // Move the value into the result register unless already there.
        if (!(otherwiseValue.GetStorageClass() == StorageClass::Direct
              && otherwiseValue.GetDirectRegister() == resultRegister))
        {
            CodeGenHelpers::Emit<OpCode::Mov>(tree.GetCodeGenerator(),
                                              resultRegister,
                                              otherwiseValue);
        }

        // There are no issues to jumping directly to the start of epilog with
        // no regard for the instructions generated before Evaluate() as there
        // is nothing in them that would need to be undone. Most notably, the
        // stack pointer is supposed to be kept constant during evaluation
        // of the function (except temporarily for function calls).
        code.Jmp(tree.GetStartOfEpilogue());

        code.PlaceLabel(continueWithRegularFlow);
    }
}
