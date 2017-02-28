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


#include "../../inc/NativeJIT/BitOperations.h"
#include "../../inc/NativeJIT/CodeGen/CallingConvention.h"
#include "../../inc/NativeJIT/CodeGen/FunctionBuffer.h"
#include "../../inc/NativeJIT/Nodes/CallNode.h"

namespace NativeJIT
{
    //*************************************************************************
    //
    // SaveRestoreVolatilesHelper
    //
    //*************************************************************************
    SaveRestoreVolatilesHelper::SaveRestoreVolatilesHelper(Allocators::IAllocator& allocator)
        : m_rxxCallExclusiveRegisterMask(0),
          m_xmmCallExclusiveRegisterMask(0),
          m_preservationStorage(Allocators::StlAllocator<void*>(allocator))
    {
        m_preservationStorage.reserve(RegisterBase::c_maxIntegerRegisterID + 1
                                      + RegisterBase::c_maxIntegerRegisterID + 1);
    }


    template<>
    unsigned SaveRestoreVolatilesHelper::GetRegistersToPreserve<false>(ExpressionTree& tree) const
    {
        // Save all used volatiles, except those used in and *fully* owned by
        // the call itself.
        return CallingConvention::c_rxxVolatileRegistersMask
               & CallingConvention::c_rxxWritableRegistersMask
               & tree.GetRXXUsedMask()
               & ~m_rxxCallExclusiveRegisterMask;
    }


    template<>
    unsigned SaveRestoreVolatilesHelper::GetRegistersToPreserve<true>(ExpressionTree& tree) const
    {
        // Save all used volatiles, except those used in and *fully* owned by
        // the call itself.
        return CallingConvention::c_xmmVolatileRegistersMask
               & CallingConvention::c_xmmWritableRegistersMask
               & tree.GetXMMUsedMask()
               & ~m_xmmCallExclusiveRegisterMask;
    }


    void SaveRestoreVolatilesHelper::SaveVolatiles(ExpressionTree& tree)
    {
        auto & code = tree.GetCodeGenerator();

        unsigned rxxVolatiles = GetRegistersToPreserve<false>(tree);

        unsigned r = 0;
        while (BitOp::GetLowestBitSet(rxxVolatiles, &r))
        {
            m_preservationStorage.push_back(tree.Temporary<void*>());
            auto const & s = m_preservationStorage.back();

            code.Emit<OpCode::Mov>(s.GetBaseRegister(),
                                   s.GetOffset(),
                                   Register<8, false>(r));

            BitOp::ClearBit(&rxxVolatiles, r);
        }

        unsigned xmmVolatiles = GetRegistersToPreserve<true>(tree);

        while (BitOp::GetLowestBitSet(xmmVolatiles, &r))
        {
            // DESIGN NOTE: This preserves only the lower 64 bits of the XMM register.
            // That is currently fine as NativeJIT only uses floats and doubles
            // which don't overlap with the upper 64-bits. Also, the full 128
            // bits of XMM nonvolatiles are preserved in the function prolog
            // by FunctionBuffer. To be fully correct, this should preserve
            // all 128 bits (f. ex. allow Temporary() to return 16-byte
            // aligned 16-byte space and use movaps to save the whole register).
            // RestoreVolatiles() needs to be modified accordingly as well.
            m_preservationStorage.push_back(tree.Temporary<void*>());
            auto const & s = m_preservationStorage.back();

            code.Emit<OpCode::Mov>(s.GetBaseRegister(),
                                   s.GetOffset(),
                                   Register<8, true>(r));


            BitOp::ClearBit(&xmmVolatiles, r);
        }
    }


    void SaveRestoreVolatilesHelper::RestoreVolatiles(ExpressionTree& tree)
    {
        auto & code = tree.GetCodeGenerator();

        // Do everything in the reverse order, including popping XMM registers first.
        unsigned xmmVolatiles = GetRegistersToPreserve<true>(tree);

        unsigned r = 0;
        while (BitOp::GetHighestBitSet(xmmVolatiles, &r))
        {
            LogThrowAssert(!m_preservationStorage.empty(), "Logic error");
            auto const & s = m_preservationStorage.back();

            code.Emit<OpCode::Mov>(Register<8, true>(r),
                                   s.GetBaseRegister(),
                                   s.GetOffset());

            m_preservationStorage.pop_back();
            BitOp::ClearBit(&xmmVolatiles, r);
        }

        unsigned rxxVolatiles = GetRegistersToPreserve<false>(tree);

        while (BitOp::GetHighestBitSet(rxxVolatiles, &r))
        {
            LogThrowAssert(!m_preservationStorage.empty(), "Logic error");
            auto const & s = m_preservationStorage.back();

            code.Emit<OpCode::Mov>(Register<8, false>(r),
                                   s.GetBaseRegister(),
                                   s.GetOffset());

            m_preservationStorage.pop_back();
            BitOp::ClearBit(&rxxVolatiles, r);
        }
    }
}
