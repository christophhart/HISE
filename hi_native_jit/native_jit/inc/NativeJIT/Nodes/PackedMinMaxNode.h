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


#include "../CodeGen/X64CodeGenerator.h"     // OpCode type.
#include "Node.h"
#include "../Packed.h"


namespace NativeJIT
{
    template <typename PACKED, bool ISMAX>
    class PackedMinMaxNode : public Node<PACKED>
    {
    public:
        PackedMinMaxNode(ExpressionTree& tree,
                         Node<PACKED>& left,
                         Node<PACKED>& right);

        virtual ExpressionTree::Storage<PACKED> CodeGenValue(ExpressionTree& tree) override;

        virtual void Print(std::ostream& out) const override;

    private:
        // WARNING: This class is designed to be allocated by an arena allocator,
        // so its destructor will never be called. Therefore, it should hold no
        // resources other than memory from the arena allocator.
        ~PackedMinMaxNode();

        Node<PACKED>& m_left;
        Node<PACKED>& m_right;
    };


    namespace PackedMinMaxHelper
    {
        // A helper class to emit the code to implement min/max operation
        // for the specified PACKED. It does its work by processing the outer
        // portion of the PACKED and recursively passing the rest of it to
        // the matching MinMaxEmitter.
        template <bool ISMAX, typename REGTYPE, typename PACKED>
        struct MinMaxEmitter
        {
            static void Emit(FunctionBuffer& code, REGTYPE left, REGTYPE right);
        };


        // Specialization for the void PACKED, i.e. the exit from recursion.
        template <bool ISMAX, typename REGTYPE>
        struct MinMaxEmitter<ISMAX, REGTYPE, void>
        {
            static void Emit(FunctionBuffer& code, REGTYPE left, REGTYPE right);
        };
    }


    //*************************************************************************
    //
    // Template definitions for PackedMinMaxNode
    //
    //*************************************************************************
    template <typename PACKED, bool ISMAX>
    PackedMinMaxNode<PACKED, ISMAX>::PackedMinMaxNode(ExpressionTree& tree,
                                                      Node<PACKED>& left,
                                                      Node<PACKED>& right)
        : Node<PACKED>(tree),
          m_left(left),
          m_right(right)
    {
        static_assert(std::is_pod<PACKED>::value, "PACKED must be a POD type.");
        left.IncrementParentCount();
        right.IncrementParentCount();
    }


    template <typename PACKED, bool ISMAX>
    ExpressionTree::Storage<PACKED>
    PackedMinMaxNode<PACKED, ISMAX>::CodeGenValue(ExpressionTree& tree)
    {
        auto & code = tree.GetCodeGenerator();

        ExpressionTree::Storage<PACKED> sLeft;
        ExpressionTree::Storage<PACKED> sRight;

        this->CodeGenInOrder(tree,
                             m_left, sLeft,
                             m_right, sRight);

        // Convert the left and right parameters into direct registers making
        // sure that they don't get spilled.
        // We will be building the result in the left register and destroy the
        // right register in the process.
        {
            auto l = sLeft.ConvertToDirect(true);
            ReferenceCounter lPin = sLeft.GetPin();
            auto r = sRight.ConvertToDirect(true);
            ReferenceCounter rPin = sRight.GetPin();

            // The algorithm works by comparing the left and right register field
            // by field. In each step, the left register will contain the result
            // for the already processed fields. Bits in both registers are rotated
            // after each step so that the fields that are currently being compared
            // are in the leftmost portion of the registers. Once the whole process
            // is completed, the registers will have been rotated the full circle
            // and the left register will contain the final result of the operation.

            // Process the unused portion of the PACKED by clearing the unused bits.
            // This is done by left-shifting towards the most significant bit.
            const auto bitsToShiftToMSB = static_cast<uint8_t>(sizeof(PackedUnderlyingType) * 8
                                                               - PACKED::c_totalBitCount);

            code.EmitImmediate<OpCode::Shl>(l, bitsToShiftToMSB);
            code.EmitImmediate<OpCode::Shl>(r, bitsToShiftToMSB);

            // Start the recursion. The helper does not use any additional registers.
            typedef decltype(l) RegisterType;
            PackedMinMaxHelper::MinMaxEmitter<ISMAX, RegisterType, PACKED>::Emit(code, l, r);
        }

        return sLeft;
    }


    template <bool ISMAX, typename REGTYPE, typename PACKED>
    void PackedMinMaxHelper::MinMaxEmitter<ISMAX, REGTYPE, PACKED>::Emit(
        FunctionBuffer& code,
        REGTYPE left,
        REGTYPE right)
    {
        Label keepLeftDigit = code.AllocateLabel();
        Label bottomOfLoop = code.AllocateLabel();

        // Comparing left to right effectively compares the leftmost fields of
        // the two values.
        code.Emit<OpCode::Cmp>(left, right);

        const JccType jccType = ISMAX ? JccType::JAE : JccType::JB;

        code.EmitConditionalJump<jccType>(keepLeftDigit);

        // Case: Want the keep the digit from the right parameter.
        //       Shift the high order bits from the right parameter into the low
        //       order bits of the left parameter.
        code.EmitImmediate<OpCode::Shld>(left, right, static_cast<uint8_t>(PACKED::c_leftmostBitCount));
        code.Jmp(bottomOfLoop);

        // Case: Want to keep the digit from left parameter.
        //       Just rotate it around to the low order bits.
        code.PlaceLabel(keepLeftDigit);
        code.EmitImmediate<OpCode::Rol>(left, static_cast<uint8_t>(PACKED::c_leftmostBitCount));

        // In either case, shift off the high order bits or the right parameter.
        code.PlaceLabel(bottomOfLoop);
        code.EmitImmediate<OpCode::Shl>(right, static_cast<uint8_t>(PACKED::c_leftmostBitCount));

        // Process the remainder of the PACKED.
        MinMaxEmitter<ISMAX, REGTYPE, typename PACKED::Right>::Emit(code, left, right);
    }


    template <bool ISMAX, typename REGTYPE>
    void PackedMinMaxHelper::MinMaxEmitter<ISMAX, REGTYPE, void>::Emit(
        FunctionBuffer& /* code */,
        REGTYPE /* left */,
        REGTYPE /* right */)
    {
        // End of recursion, do nothing.
    }


    template <typename PACKED, bool ISMAX>
    void PackedMinMaxNode<PACKED, ISMAX>::Print(std::ostream& out) const
    {
        this->PrintCoreProperties(out, "PackedMinMaxNode");

        out << ", isMax = " << (ISMAX ? "true" : "false")
            << ", left = " << m_left.GetId()
            << ", right = " << m_right.GetId();
    }
}
