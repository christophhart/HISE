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

//
// Declaration includes - these includes must come before implementation includes.
//
#include "ExpressionNodeFactoryDecls.h"

//
// Implementation includes
//
#include <cstdint>

#include "BitOperations.h"
#include "Nodes/BinaryImmediateNode.h"
#include "Nodes/BinaryNode.h"
#include "Nodes/CallNode.h"
#include "Nodes/CastNode.h"
#include "Nodes/ConditionalNode.h"
#include "Nodes/DependentNode.h"
#include "Nodes/FieldPointerNode.h"
#include "Nodes/ImmediateNode.h"
#include "Nodes/IndirectNode.h"
#include "Nodes/Node.h"
#include "Nodes/PackedMinMaxNode.h"
#include "Nodes/ParameterNode.h"
#include "Nodes/ReturnNode.h"
#include "Nodes/ShldNode.h"
#include "Nodes/StackVariableNode.h"
#include "../Temporary/Allocator.h"


namespace NativeJIT
{
    //*************************************************************************
    //
    // Template definitions for ExpressionNodeFactory.
    //
    //*************************************************************************

    //
    // Leaf nodes
    //
    template <typename T>
    ImmediateNode<T>& ExpressionNodeFactory::Immediate(T value)
    {
        return PlacementConstruct<ImmediateNode<T>>(*this, value);
    }


    template <typename T>
    ParameterNode<T>& ExpressionNodeFactory::Parameter(ParameterSlotAllocator& slotAllocator)
    {
        return PlacementConstruct<ParameterNode<T>>(*this, slotAllocator);
    }


    template <typename T>
    Node<T&>& ExpressionNodeFactory::StackVariable()
    {
        return PlacementConstruct<StackVariableNode<T>>(*this);
    }


    //
    // Unary operators
    //

    template <typename T>
    Node<T*>& ExpressionNodeFactory::AsPointer(Node<T&>& reference)
    {
        return Cast<T*>(reference);
    }


    template <typename T>
    Node<T&>& ExpressionNodeFactory::AsReference(Node<T*>& pointer)
    {
        return Cast<T&>(pointer);
    }


    template <typename TO, typename FROM>
    Node<TO>& ExpressionNodeFactory::Cast(Node<FROM>& source)
    {
        return PlacementConstruct<CastNode<TO, FROM>>(*this, source);
    }


    template <typename FROM>
    Node<FROM const>&
    ExpressionNodeFactory::AddConstCast(Node<FROM>& value)
    {
        return Cast<FROM const>(value);
    }


    template <typename FROM>
    Node<FROM>&
    ExpressionNodeFactory::RemoveConstCast(Node<FROM const>& value,
                                           typename std::enable_if<!std::is_const<FROM>::value>::type*)
    {
        return Cast<FROM>(value);
    }


    template <typename FROM>
    Node<FROM&>&
    ExpressionNodeFactory::RemoveConstCast(Node<FROM const &>& value,
                                           typename std::enable_if<std::is_const<FROM>::value>::type*)
    {
        return Cast<FROM&>(value);
    }


    template <typename FROM>
    Node<FROM const *>&
    ExpressionNodeFactory::AddTargetConstCast(Node<FROM*>& value)
    {
        return Cast<FROM const *>(value);
    }


    template <typename FROM>
    Node<FROM*>& ExpressionNodeFactory::RemoveTargetConstCast(Node<FROM const *>& value)
    {
        return Cast<FROM*>(value);
    }


    template <typename T>
    Node<T>& ExpressionNodeFactory::Deref(Node<T*>& pointer)
    {
        return Deref(pointer, 0);
    }


    template <typename T>
    Node<T>& ExpressionNodeFactory::Deref(Node<T*>& pointer, int32_t index)
    {
        return PlacementConstruct<IndirectNode<T>>(*this, pointer, index);
    }


    template <typename T>
    Node<T>& ExpressionNodeFactory::Deref(Node<T&>& reference)
    {
        return Deref(AsPointer(reference));
    }


    template <typename OBJECT, typename FIELD, typename OBJECT1>
    Node<FIELD*>&
    ExpressionNodeFactory::FieldPointer(Node<OBJECT*>& object, FIELD OBJECT1::*field)
    {
        static_assert(std::is_same<typename std::remove_const<OBJECT>::type,
                                   typename std::remove_const<OBJECT1>::type>::value,
                      "Mismatch between the provided object type and field's parent object type");

        return PlacementConstruct<FieldPointerNode<OBJECT, FIELD>>(*this, object, field);
    }


    template <typename T>
    Node<T>& ExpressionNodeFactory::Dependent(Node<T>& dependentNode,
                                              NodeBase& prerequisiteNode)
    {
        return PlacementConstruct<DependentNode<T>>(*this, dependentNode, prerequisiteNode);
    }


    template <typename T>
    NodeBase& ExpressionNodeFactory::Return(Node<T>& value)
    {
        return PlacementConstruct<ReturnNode<T>>(*this, value);
    }


    //
    // Binary arithmetic operators
    //
    template <typename L, typename R>
    Node<L>& ExpressionNodeFactory::Add(Node<L>& left, Node<R>& right)
    {
        return Binary<OpCode::Add>(left, right);
    }


    template <typename L, typename R>
    Node<L>& ExpressionNodeFactory::And(Node<L>& left, Node<R>& right)
    {
        return Binary<OpCode::And>(left, right);
    }


    template <typename L, typename R>
    Node<L>& ExpressionNodeFactory::Sub(Node<L>& left, Node<R>& right)
    {
        return Binary<OpCode::Sub>(left, right);
    }

	


    template <typename L, typename R>
    Node<L>& ExpressionNodeFactory::Mul(Node<L>& left, Node<R>& right)
    {
        return Binary<OpCode::IMul>(left, right);
    }


    template <typename L, typename R>
    Node<L>& ExpressionNodeFactory::MulImmediate(Node<L>& left, R right)
    {
        Node<L>* result;

        if (right == 0)
        {
            result = &Immediate<L>(0);
        }
        else if (right == 1)
        {
            result = &left;
        }
        else if (BitOp::GetNonZeroBitCount(right) == 1)
        {
            // Note: not checking return value of GetLowestBitSet() as it's
            // guaranteed to return an index when a bit is set.
            unsigned bitIndex;
            BitOp::GetLowestBitSet(right, &bitIndex);

            result = &Shl(left, static_cast<uint8_t>(bitIndex));
        }
        else
        {
            result = &BinaryImmediate<OpCode::IMul>(left, right);
        }

        return *result;
    }


    template <typename L, typename R>
    Node<L>& ExpressionNodeFactory::Or(Node<L>& left, Node<R>& right)
    {
        return Binary<OpCode::Or>(left, right);
    }


    template <typename L, typename R>
    Node<L>& ExpressionNodeFactory::Rol(Node<L>& left, R right)
    {
        return BinaryImmediate<OpCode::Rol>(left, right);
    }


    template <typename L, typename R>
    Node<L>& ExpressionNodeFactory::Shl(Node<L>& left, R right)
    {
        return BinaryImmediate<OpCode::Shl>(left, right);
    }


    template <typename L, typename R>
    Node<L>& ExpressionNodeFactory::Shr(Node<L>& left, R right)
    {
        return BinaryImmediate<OpCode::Shr>(left, right);
    }


    template <typename T>
    Node<T>& ExpressionNodeFactory::Shld(Node<T>& shiftee, Node<T>& filler, uint8_t bitCount)
    {
        return PlacementConstruct<ShldNode<T>>(*this, shiftee, filler, bitCount);
    }


    template <typename T, typename INDEX>
    Node<T*>& ExpressionNodeFactory::Add(Node<T*>& array, Node<INDEX>& index)
    {
        // Cast the index to UInt64 to make sure that the calculated offset
        // will not overflow. This will also make it possible to use OpCode::Add
        // on the result regardless of sizeof(INDEX) since both T* and UInt64
        // use the same register size.
        auto & index64 = Cast<uint64_t>(index);

        // The IMul instruction doesn't suport 64-bit immediates, but there's
        // also no need to support types whose size is larger than UINT32_MAX.
        static_assert(sizeof(T) <= UINT32_MAX, "Unsupported type");
        auto & offset = MulImmediate(index64, static_cast<uint32_t>(sizeof(T)));

        return Binary<OpCode::Add>(array, offset);
    }


    template <typename T, size_t SIZE, typename INDEX>
    Node<T*>& ExpressionNodeFactory::Add(Node<T(*)[SIZE]>& array, Node<INDEX>& index)
    {
        return Add(Cast<T*>(array), index);
    }


    //
    // Model related
    //
    template <typename PACKED>
    Node<float>& ExpressionNodeFactory::ApplyModel(Node<Model<PACKED>*>& model, Node<PACKED>& packed)
    {
        auto & array = FieldPointer(model, &Model<PACKED>::m_data);
        return Deref(Add(array, packed));
    }


    //
    // Relational operators
    //
    template <JccType JCC, typename T>
    FlagExpressionNode<JCC>&
    ExpressionNodeFactory::Compare(Node<T>& left, Node<T>& right)
    {
        return PlacementConstruct<RelationalOperatorNode<T, JCC>>(*this, left, right);
    }


    //
    // Conditional operators
    //
    template <typename T, JccType JCC>
    Node<T>& ExpressionNodeFactory::Conditional(FlagExpressionNode<JCC>& condition,
                                                Node<T>& trueValue,
                                                Node<T>& falseValue)
    {
        return PlacementConstruct<ConditionalNode<T, JCC>>(*this, condition, trueValue, falseValue);
    }


    template <typename CONDT, typename T>
    Node<T>& ExpressionNodeFactory::IfNotZero(Node<CONDT>& conditionValue, Node<T>& trueValue, Node<T>& falseValue)
    {
        auto & conditionNode = Compare<JccType::JNE>(conditionValue, Immediate<CONDT>(0));

        return Conditional(conditionNode, trueValue, falseValue);
    }


    template <typename T>
    Node<T>& ExpressionNodeFactory::If(Node<bool>& conditionValue, Node<T>& thenValue, Node<T>& elseValue)
    {
        return IfNotZero(conditionValue, thenValue, elseValue);
    }


    //
    // Call external function
    //
    template <typename R>
    Node<R>& ExpressionNodeFactory::Call(Node<R (*)()>& function)
    {
        return PlacementConstruct<CallNode<R>>(*this, function);
    }


    template <typename R, typename P1>
    Node<R>& ExpressionNodeFactory::Call(Node<R (*)(P1)>& function,
                                         Node<P1>& param1)
    {
        return PlacementConstruct<CallNode<R, P1>>(*this, function, param1);
    }


    template <typename R, typename P1, typename P2>
    Node<R>& ExpressionNodeFactory::Call(Node<R (*)(P1, P2)>& function,
                                         Node<P1>& param1,
                                         Node<P2>& param2)
    {
        return PlacementConstruct<CallNode<R, P1, P2>>(*this, function, param1, param2);
    }


    template <typename R, typename P1, typename P2, typename P3>
    Node<R>& ExpressionNodeFactory::Call(Node<R (*)(P1, P2, P3)>& function,
                                         Node<P1>& param1,
                                         Node<P2>& param2,
                                         Node<P3>& param3)
    {
        return PlacementConstruct<CallNode<R, P1, P2, P3>>(*this, function, param1, param2, param3);
    }


    template <typename R, typename P1, typename P2, typename P3, typename P4>
    Node<R>& ExpressionNodeFactory::Call(Node<R (*)(P1, P2, P3, P4)>& function,
                                         Node<P1>& param1,
                                         Node<P2>& param2,
                                         Node<P3>& param3,
                                         Node<P4>& param4)
    {
        return PlacementConstruct<CallNode<R, P1, P2, P3, P4>>(
            *this, function, param1, param2, param3, param4);
    }


    //
    // PackedMinMax
    //
    template <typename PACKED>
    Node<PACKED>& ExpressionNodeFactory::PackedMax(Node<PACKED>& left, Node<PACKED>& right)
    {
        return PlacementConstruct<PackedMinMaxNode<PACKED, true>>(*this, left, right);
    }


    template <typename PACKED>
    Node<PACKED>& ExpressionNodeFactory::PackedMin(Node<PACKED>& left, Node<PACKED>& right)
    {
        return PlacementConstruct<PackedMinMaxNode<PACKED, false>>(*this, left, right);
    }


    //
    // Private methods.
    //
    template <OpCode OP, typename L, typename R>
    Node<L>& ExpressionNodeFactory::Binary(Node<L>& left, Node<R>& right)
    {
        return PlacementConstruct<BinaryNode<OP, L, R>>(*this, left, right);
    }


    template <OpCode OP, typename L, typename R>
    Node<L>& ExpressionNodeFactory::BinaryImmediate(Node<L>& left, R right)
    {
        return PlacementConstruct<BinaryImmediateNode<OP, L, R>>(*this, left, right);
    }
}
