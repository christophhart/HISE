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

#include <cstdint>

#include "CodeGen/X64CodeGenerator.h" // JccType.
#include "ExpressionTreeDecls.h"      // Base class.
#include "Model.h"                    // Parameter.
#include "Nodes/ImmediateNodeDecls.h" // Parameter too cumbersome to forward declare.


namespace NativeJIT
{
    template <JccType JCC>
    class FlagExpressionNode;

    template <typename T>
    class Node;

    class NodeBase;

    class ParameterSlotAllocator;

    template <typename T>
    class ParameterNode;

    class ExpressionNodeFactory : public ExpressionTree
    {
    public:
        ExpressionNodeFactory(Allocators::IAllocator& allocator, FunctionBuffer& code);

        //
        // Leaf nodes
        //
        template <typename T> ImmediateNode<T>& Immediate(T value);
        template <typename T> ParameterNode<T>& Parameter(ParameterSlotAllocator& slotAllocator);

        // See StackVariableNode for important information about stack variable
        // lifetime.
        template <typename T> Node<T&>& StackVariable();


        //
        // Unary operators
        //
        template <typename T> Node<T*>& AsPointer(Node<T&>& reference);
        template <typename T> Node<T&>& AsReference(Node<T*>& pointer);

        template <typename TO, typename FROM> Node<TO>& Cast(Node<FROM>& value);

        // Casts that add or remove const from the node type.
        // Note: enable_if is a workaround for the MSVC bug (falsely reported
        // ambiguity between the two templates).
        template <typename FROM> Node<FROM const>& AddConstCast(Node<FROM>& value);
        template <typename FROM> Node<FROM>& RemoveConstCast(Node<FROM const>& value,
                                                             typename std::enable_if<!std::is_const<FROM>::value>::type* = nullptr);
        template <typename FROM> Node<FROM&>& RemoveConstCast(Node<FROM const &>& value,
                                                              typename std::enable_if<std::is_const<FROM>::value>::type* = nullptr);

        // Casts that add or remove const from the pointed-to target type.
        template <typename FROM> Node<FROM const *>& AddTargetConstCast(Node<FROM*>& value);
        template <typename FROM> Node<FROM*>& RemoveTargetConstCast(Node<FROM const *>& value);

        template <typename T> Node<T>& Deref(Node<T*>& pointer);
        template <typename T> Node<T>& Deref(Node<T*>& pointer, int32_t index);
        template <typename T> Node<T>& Deref(Node<T&>& reference);

        // Note: OBJECT1 is there to allow for template deduction in all cases
        // since OBJECT may or may not be const, but OBJECT1 is never const
        // in FieldPointer(someObjectNode, &SomeObject::m_field) expression.
        // Otherwise, the following code would fail to compile:
        //
        // Node<SomeObject const *>& obj = ...;
        // FieldPointer(obj, &SomeObject::m_field)
        template <typename OBJECT, typename FIELD, typename OBJECT1 = OBJECT>
        Node<FIELD*>& FieldPointer(Node<OBJECT*>& object, FIELD OBJECT1::*field);

        // Note: even though it has two arguments, Dependent node is conceptually
        // unary.
        template <typename T> Node<T>& Dependent(Node<T>& dependentNode,
                                                 NodeBase& prerequisiteNode);

        template <typename T> NodeBase& Return(Node<T>& value);


        //
        // Binary arithmetic operators
        //
        template <typename L, typename R> Node<L>& Add(Node<L>& left, Node<R>& right);
        template <typename L, typename R> Node<L>& And(Node<L>& left, Node<R>& right);
        template <typename L, typename R> Node<L>& Mul(Node<L>& left, Node<R>& right);
		
        template <typename L, typename R> Node<L>& MulImmediate(Node<L>& left, R right);
        template <typename L, typename R> Node<L>& Or(Node<L>& left, Node<R>& right);
        template <typename L, typename R> Node<L>& Rol(Node<L>& left, R right);
        template <typename L, typename R> Node<L>& Shl(Node<L>& left, R right);
        template <typename L, typename R> Node<L>& Shr(Node<L>& left, R right);
        template <typename L, typename R> Node<L>& Sub(Node<L>& left, Node<R>& right);

        template <typename T, size_t SIZE, typename INDEX>
        Node<T*>& Add(Node<T(*)[SIZE]>& array, Node<INDEX>& index);

        template <typename T, typename INDEX> Node<T*>& Add(Node<T*>& array, Node<INDEX>& index);

        //
        // Ternary arithmetic operators
        template <typename T>
        Node<T>& Shld(Node<T>& shiftee, Node<T>& filler, uint8_t bitCount);

        //
        // Model related.
        //
        template <typename PACKED> Node<float>& ApplyModel(Node<Model<PACKED>*>& model, Node<PACKED>& packed);


        //
        // Relational operators
        //
        template <JccType JCC, typename T>
        FlagExpressionNode<JCC>& Compare(Node<T>& left, Node<T>& right);


        //
        // Conditional operators
        //

        // WARNING: Both trueValue and falseValue are evaluated before testing the
        // condition so both must be legal to evaluate regardless of the result
        // of the condition. See the TODO note in ConditionalNode::CodeGenValue.
        template <typename T, JccType JCC>
        Node<T>& Conditional(FlagExpressionNode<JCC>& condition, Node<T>& trueValue, Node<T>& falseValue);

        // WARNING: Both trueValue and falseValue are evaluated before testing the
        // condition so both must be legal to evaluate regardless of the result
        // of the condition. See the TODO note in ConditionalNode::CodeGenValue.
        template <typename CONDT, typename T>
        Node<T>& IfNotZero(Node<CONDT>& conditionValue, Node<T>& trueValue, Node<T>& falseValue);

        // WARNING: Both thenValue and elseValue are evaluated before testing the
        // condition so both must be legal to evaluate regardless of the result
        // of the condition. See the TODO note in ConditionalNode::CodeGenValue.
        template <typename T>
        Node<T>& If(Node<bool>& conditionValue, Node<T>& thenValue, Node<T>& elseValue);


        //
        // Call node
        //
        template <typename R>
        Node<R>& Call(Node<R (*)()>& function);

        template <typename R, typename P1>
        Node<R>& Call(Node<R (*)(P1)>& function, Node<P1>& param1);

        template <typename R, typename P1, typename P2>
        Node<R>& Call(Node<R (*)(P1, P2)>& function, Node<P1>& param1, Node<P2>& param2);

        template <typename R, typename P1, typename P2, typename P3>
        Node<R>& Call(Node<R (*)(P1, P2, P3)>& function,
                      Node<P1>& param1,
                      Node<P2>& param2,
                      Node<P3>& param3);

        template <typename R, typename P1, typename P2, typename P3, typename P4>
        Node<R>& Call(Node<R (*)(P1, P2, P3, P4)>& function,
                      Node<P1>& param1,
                      Node<P2>& param2,
                      Node<P3>& param3,
                      Node<P4>& param4);

        //
        // Packed operators
        //
        // TODO: use traits to ensure PACKED is a Packed type.
        template <typename PACKED>
        Node<PACKED>& PackedMax(Node<PACKED>& left, Node<PACKED>& right);

        template <typename PACKED>
        Node<PACKED>& PackedMin(Node<PACKED>& left, Node<PACKED>& right);

    private:
        template <OpCode OP, typename L, typename R> Node<L>& Binary(Node<L>& left, Node<R>& right);
        template <OpCode OP, typename L, typename R> Node<L>& BinaryImmediate(Node<L>& left, R right);
    };
}
