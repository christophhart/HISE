/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option any later version.
*
*   HISE is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licences for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licencing:
*
*   http://www.hartinstruments.net/hise/
*
*   HISE is based on the JUCE library,
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

#pragma once

#define JIT_MEMBER_WRAPPER_0(R, C, N)					  static R N(void* o) { return static_cast<C*>(o)->N(); };
#define JIT_MEMBER_WRAPPER_1(R, C, N, T1)				  static R N(void* o, T1 a1) { return static_cast<C*>(o)->N(a1); };
#define JIT_MEMBER_WRAPPER_2(R, C, N, T1, T2)			  static R N(void* o, T1 a1, T2 a2) { return static_cast<C*>(o)->N(a1, a2); };
#define JIT_MEMBER_WRAPPER_3(R, C, N, T1, T2, T3)		  static R N(void* o, T1 a1, T2 a2, T3 a3) { return static_cast<C*>(o)->N(a1, a2, a3); };
#define JIT_MEMBER_WRAPPER_4(R, C, N, T1, T2, T3, T4)	  static R N(void* o, T1 a1, T2 a2, T3 a3, T4 a4) { return static_cast<C*>(o)->N(a1, a2, a3, a4); };
#define JIT_MEMBER_WRAPPER_5(R, C, N, T1, T2, T3, T4, T5) static R N(void* o, T1 a1, T2 a2, T3 a3, T4 a4, T5 a5) { return static_cast<C*>(o)->N(a1, a2, a3, a4, a5); };

namespace snex
{
namespace jit
{
#define DECLARE_ID(x) static const juce::Identifier x(#x);

namespace OptimizationIds
{
DECLARE_ID(ConstantFolding);
DECLARE_ID(Inlining);
DECLARE_ID(DeadCodeElimination);
DECLARE_ID(BinaryOpOptimisation);
}

#undef DECLARE_ID
}
}

namespace snex
{
struct ApiHelpers
{
	enum DebugObjectTypes
	{
		LocalFunction = 9000,
		ApiCall,
		Template,
		Constants,
		BasicTypes,
		numDebugObjectTypes
	};

	static void getColourAndLetterForType(int type, Colour& colour, char& letter)
	{
		auto typedType = (Types::ID)type; // type;

		if (typedType < Types::ID::Dynamic)
		{
			colour = Types::Helpers::getColourForType(typedType);
			letter = Types::Helpers::getTypeChar(typedType);
		}

		if (type == ApiHelpers::DebugObjectTypes::Template)
		{
			colour = Colours::yellow.withSaturation(0.3f);
			letter = 'T';
		}

		if (type == ApiHelpers::DebugObjectTypes::Constants)
		{
			colour = Colours::blanchedalmond;
			letter = 'C';
		}

		if (type == ApiHelpers::DebugObjectTypes::BasicTypes)
		{
			colour = Colours::white;
			letter = 'T';
		}


		if (type == ApiHelpers::DebugObjectTypes::ApiCall)
		{
			colour = Colours::aqua;
			letter = 'A';
		}

		if (type == ApiHelpers::DebugObjectTypes::LocalFunction)
		{
			colour = Colours::dodgerblue;
			letter = 'F';
		}
	}
};
}


#include "snex_jit_Functions.h"
#include "snex_jit_NamespaceHandler.h"
#include "snex_jit_BaseScope.h"
#include "snex_jit_GlobalScope.h"
#include "snex_jit_JitCallableObject.h"
#include "snex_jit_JitCompiledFunctionClass.h"
#include "snex_jit_JitCompiler.h"

namespace snex {
namespace jit {
using namespace juce;

#if HNODE_BOOL_IS_NOT_INT
using BooleanType = unsigned char
#else
using BooleanType = int;
#endif

using PointerType = uint64_t;

#if JUCE_64BIT
typedef uint64_t AddressType;
#else
typedef uint32_t AddressType;
#endif




} // end namespace jit
} // end namespace snex

