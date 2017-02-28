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
#include <type_traits>

#include "../ExpressionNodeFactory.h"
#include "Node.h"


namespace NativeJIT
{
    class ExpressionNodeFactory;

    // The CastNode implements a cast node from FROM to TO, most closely
    // resembling the C++ static_cast/reinterpret_cast. Requires that FROM
    // is convertible to TO using static_cast or that the sizes of FROM and TO
    // are the same so that reinterpret_cast can semi-safely be used.
    // If register size and type of FROM/TO are the same, the cast is a no-op.
    // Otherwise, the appropriate instructions are emitted to perform the
    // conversion.

    namespace Casting
    {
        // All the possible types of casts. Note that integer applies to register
        // type, i.e. everything that is not a floating point.
        enum class Cast { NoOp, IntToInt, FloatToFloat, IntToFloat, FloatToInt };

        // A class used to specialize code generation for casts that can be
        // implemented directly using the X64 conversion instructions.
        template <Cast TYPE>
        struct OneStepCastGenerator
        {
            template <typename TO, typename FROM>
            static Storage<TO>
            Generate(ExpressionTree& tree, Storage<FROM>& source);
        };


        // A class used to build a composite node needed to perform a cast
        // for casts that cannot be implemented directly using the X64 conversion
        // instructions.
        template <Cast TYPE>
        struct CompositeCastNodeBuilder
        {
            template <typename TO, typename FROM>
            static Node<TO>& Build(ExpressionNodeFactory& tree, Node<FROM>& from);
        };


        // Properties for a cast.
        template <typename TO, typename FROM>
        class Traits
        {
        public:
            // Either neither side is a reference, or if one of them is, the
            // other one must be a pointer.
            static_assert((!std::is_reference<FROM>::value
                           && !std::is_reference<TO>::value)
                          // Const cast is also acceptable.
                          || (std::is_same<typename std::remove_const<FROM>::type,
                                           typename std::remove_const<TO>::type>::value)
                          || (std::is_pointer<FROM>::value
                              || std::is_pointer<TO>::value),
                          "If one side is a reference, the other one must be a pointer");

            // Source and target register types and properties.
            typedef typename Storage<FROM>::DirectRegister FromRegister;
            typedef typename Storage<TO>::DirectRegister ToRegister;

        private:
            static const bool c_isSameRegisterType = std::is_same<FromRegister, ToRegister>::value;
            static const bool c_isSameFloatValue = FromRegister::c_isFloat == ToRegister::c_isFloat;

            // Cast is a no-op if it's from and to the same register size and
            // type (see further for signed/unsigned consideration). Additionally,
            // integer cast from larger to smaller size is a no-op and is achieved
            // by observing the larger size as the smaller size.
            //
            // For integer casts, if the signedness of both sides is same, the
            // expected outcome is clearly achieved by observing just the smaller
            // portion of the register. The same applies for larger signed FROM
            // and smaller unsigned TO since that's how the C++ standard defines
            // such cast when two's complement system is used. Finally, for larger
            // unsigned FROM and smaller signed TO, the behavior is implementation
            // defined and here it's chosen to be bit-preserving, the same as in
            // VC.
            //
            // WARNING: assumption that the int cast from larger to smaller size
            // is a no-op is non-portable for indirect storage since it assumes
            // little endian architecture. If portability is required, the
            // assumption could be removed by converting the indirect storage to
            // direct.
            static const bool c_isNoOp
                = c_isSameRegisterType
                  || (c_isSameFloatValue
                      && !FromRegister::c_isFloat
                      && FromRegister::c_size > ToRegister::c_size);

        public:
            // Determine which type of cast should be performed.
            static const Cast c_castType
                = c_isNoOp
                  ? Cast::NoOp
                  : (c_isSameFloatValue
                     ? (FromRegister::c_isFloat ? Cast::FloatToFloat : Cast::IntToInt)
                     : (FromRegister::c_isFloat ? Cast::FloatToInt : Cast::IntToFloat));

        private:
            // Check whether an int to float cast can be done using a single x64
            // conversion instruction.
            static const bool c_isOneStepIntToFloatCast
                = c_castType == Cast::IntToFloat
                  && (std::is_same<typename std::remove_cv<FROM>::type, int32_t>::value
                      || std::is_same<typename std::remove_cv<FROM>::type, int64_t>::value);

            // Check whether a float to int cast can be done using a single x64
            // conversion instruction.
            static const bool c_isOneStepFloatToIntCast
                = c_castType == Cast::FloatToInt
                  && (std::is_same<typename std::remove_cv<TO>::type, int32_t>::value
                      || std::is_same<typename std::remove_cv<TO>::type, int64_t>::value);

        public:
            static const bool c_isOneStepCast
                = c_castType == Cast::NoOp
                  || c_castType == Cast::IntToInt
                  || c_castType == Cast::FloatToFloat
                  || c_isOneStepIntToFloatCast
                  || c_isOneStepFloatToIntCast;
        };


        // A class used to specialize code generation for casts that need to
        // convert between two immediate storages. The class is needed instead
        // of a simple if/else branch because e.g. if TO type belongs to a
        // RIPRelativeImmediate category, the if/else branch that contained
        // a tree.Immediate() call would fail to compile.
        // Two levels of structs are used to allow different template specializations.
        template <ImmediateCategory FROMCATEGORY>
        struct FromImmediate
        {
            template <Cast CASTTYPE, ImmediateCategory TOCATEGORY>
            struct CastGenerator
            {
                template <typename TO, typename FROM>
                static Storage<TO>
                Generate(ExpressionTree& tree, Storage<FROM>& source);
            };
        };
    }


    // The CastNode is partially specialized depending on whether or not a one-step
    // cast can be used.
    template <typename TO,
              typename FROM,
              bool ISONESTEP = Casting::Traits<TO, FROM>::c_isOneStepCast>
    class CastNode;


    // One-step cast version of the CastNode.
    template <typename TO, typename FROM>
    class CastNode<TO, FROM, true> : public Node<TO>
    {
    public:
        CastNode(ExpressionTree& tree, Node<FROM>& from);

        //
        // Overrides of Node methods.
        //

        virtual Storage<TO> CodeGenValue(ExpressionTree& tree) override;
        virtual void Print(std::ostream& out) const override;

    private:
        // WARNING: This class is designed to be allocated by an arena allocator,
        // so its destructor will never be called. Therefore, it should hold no
        // resources other than memory from the arena allocator.
        ~CastNode();

        typedef Casting::Traits<TO, FROM> Traits;

        Node<FROM>& m_from;
    };


    // Composite cast version of the CastNode.
    template <typename TO, typename FROM>
    class CastNode<TO, FROM, false> : public Node<TO>
    {
    public:
        CastNode(ExpressionNodeFactory& tree, Node<FROM>& from);

        //
        // Overrides of Node methods.
        //

        virtual Storage<TO> CodeGenValue(ExpressionTree& tree) override;
        virtual void Print(std::ostream& out) const override;

    private:
        // WARNING: This class is designed to be allocated by an arena allocator,
        // so its destructor will never be called. Therefore, it should hold no
        // resources other than memory from the arena allocator.
        ~CastNode();

        typedef Casting::Traits<TO, FROM> Traits;

        // The composite node used to perform the cast.
        Node<TO>& m_conversionNode;
    };


    //*************************************************************************
    //
    // Template definitions for the one-step CastNode.
    //
    //*************************************************************************

    template <typename TO, typename FROM>
    CastNode<TO, FROM, true>::CastNode(ExpressionTree& tree, Node<FROM>& from)
        : Node<TO>(tree),
          m_from(from)
    {
        m_from.IncrementParentCount();
    }


    template <typename TO, typename FROM>
    Storage<TO>
    CastNode<TO, FROM, true>::CodeGenValue(ExpressionTree& tree)
    {
        auto source = m_from.CodeGen(tree);

        return Casting
            ::template OneStepCastGenerator<Traits::c_castType>
            ::template Generate<TO, FROM>(tree, source);
    }


    template <typename TO, typename FROM>
    void CastNode<TO, FROM, true>::Print(std::ostream& out) const
    {
        this->PrintCoreProperties(out, "CastNode (one-step)");
        out << ", from = " << m_from.GetId();
    }


    //*************************************************************************
    //
    // Template definitions for composite CastNode.
    //
    //*************************************************************************

    template <typename TO, typename FROM>
    CastNode<TO, FROM, false>::CastNode(ExpressionNodeFactory& tree, Node<FROM>& from)
        : Node<TO>(tree),
          m_conversionNode(Casting
                           ::CompositeCastNodeBuilder<Casting::Traits<TO, FROM>::c_castType>
                           ::template Build<TO, FROM>(tree, from))
    {
        m_conversionNode.IncrementParentCount();
    }


    template <typename TO, typename FROM>
    Storage<TO>
    CastNode<TO, FROM, false>::CodeGenValue(ExpressionTree& tree)
    {
        return m_conversionNode.CodeGen(tree);
    }


    template <typename TO, typename FROM>
    void CastNode<TO, FROM, false>::Print(std::ostream& out) const
    {
        this->PrintCoreProperties(out, "CastNode (composite)");

        out << ", conversionNode = " << m_conversionNode.GetId();
    }


    namespace Casting
    {
        //
        // Template definitions for OneStepCastGenerator.
        //

        // No-op cast.
        template <>
        template <typename TO, typename FROM>
        Storage<TO>
        OneStepCastGenerator<Cast::NoOp>::Generate(ExpressionTree& /* tree */, Storage<FROM>& source)
        {
            return Storage<TO>(source);
        }


        // Cast between two float registers.
        template <>
        template <typename TO, typename FROM>
        Storage<TO>
        OneStepCastGenerator<Cast::FloatToFloat>::Generate(ExpressionTree& tree, Storage<FROM>& source)
        {
            static_assert(Traits<TO, FROM>::c_isOneStepCast, "Invalid one-step cast.");

            auto target = tree.Direct<TO>();

            CodeGenHelpers::Emit<OpCode::CvtFP2FP>(tree.GetCodeGenerator(),
                                                   target.GetDirectRegister(),
                                                   source);

            return target;
        }


        // Cast between two integer registers.
        template <>
        template <typename TO, typename FROM>
        Storage<TO>
        OneStepCastGenerator<Cast::IntToInt>::Generate(ExpressionTree& tree, Storage<FROM>& source)
        {
            typedef Traits<TO, FROM> Traits;

            static_assert(Traits::c_isOneStepCast, "Invalid one-step cast.");
            static_assert(Traits::FromRegister::c_size < Traits::ToRegister::c_size,
                          "Invalid one-step cast.");

            typedef Storage<TO> ToStorage;

            // The cast needs to extend the source storage to a larger target.
            // Depending on the signedness of the source type, use sign extension
            // or zero extension.
            const OpCode opCode = std::is_signed<FROM>::value
                ? OpCode::MovSX
                : OpCode::MovZX;
            auto & code = tree.GetCodeGenerator();
            ToStorage result;

            switch (source.GetStorageClass())
            {
            case StorageClass::Immediate:
                result = Casting
                    ::FromImmediate<ImmediateCategoryOf<FROM>::value>
                    ::template CastGenerator<Cast::IntToInt, ImmediateCategoryOf<TO>::value>
                    ::template Generate<TO, FROM>(tree, source);
                break;

            case StorageClass::Indirect:
            {
                using namespace CodeGenHelpers;

                // Cannot reuse source, allocate a new register and move with
                // extension into it.
                result = tree.Direct<TO>();

                // Target the full register to prevent a partial register stall.
                typename ToStorage::FullRegister targetRegister(result.GetDirectRegister());
                Emitter<RegTypes::Different, ImmediateType::NotAllowed>
                    ::Emit<opCode>(tree.GetCodeGenerator(),
                                   targetRegister,
                                   source);
                break;
            }

            case StorageClass::Direct:
            {
                // May be able to reuse the register if it's fully owned by the storage.
                source.ConvertToDirect(true);
                result = ToStorage(source);

                // Always target the full register to prevent a partial register stall.
                auto sourceRegister = source.GetDirectRegister();
                typename ToStorage::FullRegister fullSourceRegister(sourceRegister);

                code.Emit<opCode>(fullSourceRegister, sourceRegister);
                break;
            }

            default:
                LogThrowAbort("Invalid storage class.");
                break;
            }

            return result;
        }


        // Cast from integer to float register.
        template <>
        template <typename TO, typename FROM>
        Storage<TO>
        OneStepCastGenerator<Cast::IntToFloat>::Generate(ExpressionTree& tree, Storage<FROM>& source)
        {
            static_assert(Traits<TO, FROM>::c_isOneStepCast, "Invalid one-step cast.");

            Storage<TO> result;

            if (source.GetStorageClass() == StorageClass::Immediate)
            {
                result = Casting
                    ::FromImmediate<ImmediateCategoryOf<FROM>::value>
                    ::template CastGenerator<Cast::IntToFloat, ImmediateCategoryOf<TO>::value>
                    ::template Generate<TO, FROM>(tree, source);
            }
            else
            {
                result = tree.Direct<TO>();

                // DESIGN NOTE: Implement xorps/xorpd and add a "xor target, target" call to
                // clear the target FP register before changing its lower 32/64 bits.
                // Doing that might prevent a partial register stall.
                using namespace CodeGenHelpers;
                Emitter<RegTypes::Different, ImmediateType::NotAllowed>
                    ::Emit<OpCode::CvtSI2FP>(tree.GetCodeGenerator(),
                                             result.GetDirectRegister(),
                                             source);
            }

            return result;
        }


        // Cast from float to integer register.
        template <>
        template <typename TO, typename FROM>
        Storage<TO>
        OneStepCastGenerator<Cast::FloatToInt>::Generate(ExpressionTree& tree, Storage<FROM>& source)
        {
            static_assert(Traits<TO, FROM>::c_isOneStepCast, "Invalid one-step cast.");

            auto target = tree.Direct<TO>();

            CodeGenHelpers::Emit<OpCode::CvtFP2SI>(tree.GetCodeGenerator(),
                                                   target.GetDirectRegister(),
                                                   source);

            return target;
        }


        // A helper method to build two cast nodes which would convert from FROM
        // to TO by using one-step casts through an INTERMEDIATE.
        template <typename TO, typename INTERMEDIATE, typename FROM>
        Node<TO>& TwoStepCast(ExpressionNodeFactory& tree, Node<FROM>& from)
        {
            static_assert(Traits<FROM, INTERMEDIATE>::c_isOneStepCast, "Must be a one-step cast.");
            static_assert(Traits<INTERMEDIATE, TO>::c_isOneStepCast, "Must be a one-step cast.");

            return tree.Cast<TO, INTERMEDIATE>(tree.Cast<INTERMEDIATE, FROM>(from));
        }


        //
        // Template definitions for FromImmediate::CastGenerator.
        //

        // Target can be represented as a inline immediate so an immediate Storage
        // can be used.
        template <>
        template <>
        template <typename TO, typename FROM>
        Storage<TO>
        FromImmediate<ImmediateCategory::InlineImmediate>
            ::CastGenerator<Cast::IntToInt, ImmediateCategory::InlineImmediate>
            ::Generate(ExpressionTree& tree, Storage<FROM>& source)
        {
            return tree.Immediate(ForcedCast<TO, FROM>(source.GetImmediate()));
        }


        // Target requires a RIP relative immediate, so a direct register has
        // to be used instead of an immediate Storage.
        template <>
        template <>
        template <typename TO, typename FROM>
        Storage<TO>
        FromImmediate<ImmediateCategory::InlineImmediate>
            ::CastGenerator<Cast::IntToInt, ImmediateCategory::RIPRelativeImmediate>
            ::Generate(ExpressionTree& tree, Storage<FROM>& source)
        {
            Storage<TO> result = tree.Direct<TO>();

            // After the compiler performs the cast, move the value to a direct register.
            auto targetImmediate = ForcedCast<TO, FROM>(source.GetImmediate());
            tree.GetCodeGenerator().EmitImmediate<OpCode::Mov>(result.GetDirectRegister(),
                                                               targetImmediate);

            return result;
        }


        // The integer to floating point conversion instruction doesn't have the
        // flavor which converts from an immediate directly. Use the compiler
        // to get the target floating point value and place it directly into
        // the target register through a temporary.
        template <>
        template <>
        template <typename TO, typename FROM>
        Storage<TO>
        FromImmediate<ImmediateCategory::InlineImmediate>
            ::CastGenerator<Cast::IntToFloat, ImmediateCategory::RIPRelativeImmediate>
            ::Generate(ExpressionTree& tree, Storage<FROM>& source)
        {
            Storage<TO> result = tree.Direct<TO>();

            // NOTE: MovThroughTemporary allocates a temporary *integer*
            // register to perform the move. There is no danger that the
            // register allocated for the "target" Storage above will be
            // spilled since it is a *floating point* register.
            CodeGenHelpers::MovThroughTemporary(tree,
                                                result.GetDirectRegister(),
                                                static_cast<TO>(source.GetImmediate()));

            return result;
        }


        // Fallback case which should never be reachable.
        template <ImmediateCategory FROMCATEGORY>
        template <Cast CASTTYPE, ImmediateCategory TOCATEGORY>
        template <typename TO, typename FROM>
        Storage<TO>
        FromImmediate<FROMCATEGORY>
            ::CastGenerator<CASTTYPE, TOCATEGORY>
            ::Generate(ExpressionTree& /* tree */, Storage<FROM>& /* source */)
        {
            static_assert(Traits<TO, FROM>::c_isOneStepCast, "Invalid one-step cast.");

            // The compile time checks should prevent writing code that will
            // reach this point. Since GetStorageClass() is a runtime method, it's
            // necessary to provide a body for the remaining cases even though
            // they should not be reachable.
            LogThrowAbort("Unexpected immediate type");

            return Storage<TO>();
        }


        //
        // Template definitions for CompositeCastNodeBuilder.
        //

#ifdef _MSC_VER
// Supress warning about constant expression involving template parameters.
#pragma warning(push)
#pragma warning(disable:4127)
#endif

        // Composite node for casting from integer to float.
        template <>
        template <typename TO, typename FROM>
        Node<TO>&
        CompositeCastNodeBuilder<Cast::IntToFloat>::Build(ExpressionNodeFactory& tree, Node<FROM>& from)
        {
            Node<TO>* result;

            if (sizeof(FROM) <= 2)
            {
                // Two one-step casts: [unsigned] int8_t/16 -> int32_t -> float.
                // Note that both uint8_t/16 can fully fit into a signed
                // int32_t.
                result = &TwoStepCast<TO, int32_t, FROM>(tree, from);
            }
            else if (sizeof(FROM) == 4)
            {
                // This path can only be reached for uint32_t (cast from
                // a int32_t is a one-step cast). Cast using two one-step
                // casts: uint32_t -> int64_t -> float.
                result = &TwoStepCast<TO, int64_t, FROM>(tree, from);
            }
            else
            {
                // This path can only be reached for uint64_t (cast from
                // a int64_t is a one-step cast). Cast it to float as if it
                // was a int64_t. If the number observed as int64_t
                // was negative, such a conversion would have produced a number
                // that's 2^64 smaller than expected. Add 2^64 back case to adjust
                // in that case. This is how VC++ does the conversion as well.

                // This test could be done with the group 3 "test" instruction
                // in case it was available.
                auto & isNegative = tree.Compare<JccType::JS>(from, tree.Immediate<FROM>(0));

                auto & floatNode = TwoStepCast<TO, int64_t, FROM>(tree, from);
                auto & twoToThePowerOf64 = tree.Immediate<TO>(1.8446744073709551616e19f);

                result = &tree.Conditional(isNegative,
                                           tree.Add(floatNode, twoToThePowerOf64),
                                           floatNode);
            }

            return *result;
        }


        // Cast from float to integer register.
        template <>
        template <typename TO, typename FROM>
        Node<TO>&
        CompositeCastNodeBuilder<Cast::FloatToInt>::Build(ExpressionNodeFactory& tree, Node<FROM>& from)
        {
            Node<TO>* result;

            // Note: In C++, conversion from a floating point value that is
            // outside of the range of the target integer is undefined behavior
            // (the result can be anything and it doesn't even have to be
            // consistent). Any occurence of undefined behavior makes any further
            // operations also undefined. Thus, the caller is expected to make
            // sure that the input floating point values are inside the target
            // integer range. Because of this, the implementation doesn't take
            // any special steps if negative float is converted to unsigned
            // integer or if the float is too large.
            if (sizeof(TO) <= 2)
            {
                // Two one-step casts: float -> int32_t -> [unsigned] int8_t/16.
                result = &TwoStepCast<TO, int32_t, FROM>(tree, from);
            }
            else if (sizeof(TO) == 4)
            {
                // Two one-step casts: float -> int64_t -> uint32_t.
                result = &TwoStepCast<TO, int64_t, FROM>(tree, from);
            }
            else
            {
                // When converting uint64_t to float, there are two
                // branches: if the float is smaller than 2^63 and it will fit
                // into a int64_t, use essentially a one-step cast.
                // Otherwise, subtract 2^63 from the floating point, convert
                // to int64_t and add (integer) 2^63 to the result.
                uint64_t twoToThePowerOf63 = 1ull << 63;
                auto & twoToThePowerOf63Float
                    = tree.Immediate(static_cast<FROM>(twoToThePowerOf63));

                // Check whether source is smaller than 2^63.
                auto & isSourceWithoutOverflow
                    = tree.Compare<JccType::JB>(from, twoToThePowerOf63Float);

                // The node with a reduced float and the int which gets increased
                // by the same value after the conversion.
                auto & reducedFloat = tree.Sub(from, twoToThePowerOf63Float);
                auto & adjustedConvertedInt
                    = tree.Add(TwoStepCast<TO, int64_t, FROM>(tree, reducedFloat),
                               tree.Immediate(static_cast<TO>(twoToThePowerOf63)));

                result = &tree.Conditional(isSourceWithoutOverflow,
                                           TwoStepCast<TO, int64_t, FROM>(tree, from),
                                           adjustedConvertedInt);
            }

            return *result;
        }

#ifdef _MSC_VER
#pragma warning(pop)
#endif
    }
}
