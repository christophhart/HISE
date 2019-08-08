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

namespace snex {
namespace jit {
using namespace juce;
using namespace asmjit;


#define HNODE_JIT_ADD_C_FUNCTION_0(rt, ptr, name) addFunction(new FunctionData(FunctionData:: template createWithoutParameters<rt>(name, reinterpret_cast<void*>(ptr))))

#define HNODE_JIT_ADD_C_FUNCTION_1(rt, ptr, argType1, name) addFunction(new FunctionData(FunctionData::template create<rt, argType1>(name, static_cast<rt(*)(argType1)>(ptr))))

#define HNODE_JIT_ADD_C_FUNCTION_2(rt, ptr, argType1, argType2, name) addFunction(new FunctionData(FunctionData::template create<rt, argType1, argType2>(name, static_cast<rt(*)(argType1, argType2)>(ptr))));
#define HNODE_JIT_ADD_C_FUNCTION_3(rt, ptr, argType1, argType2, argType3, name) addFunction(new FunctionData(FunctionData::template create<rt, argType1, argType2, argType3>(name, static_cast<rt(*)(argType1, argType2, argType3)>(ptr))));

class ConsoleFunctions : public JitCallableObject
{
	struct WrapperInt
	{
		JIT_MEMBER_WRAPPER_1(void, ConsoleFunctions, print, int);
	};

	struct WrapperDouble
	{
		JIT_MEMBER_WRAPPER_1(void, ConsoleFunctions, print, double);
	};

	struct WrapperFloat
	{
		JIT_MEMBER_WRAPPER_1(void, ConsoleFunctions, print, float);
	};

	struct WrapperEvent
	{
		JIT_MEMBER_WRAPPER_1(void, ConsoleFunctions, print, HiseEvent);
	};

	
	void print(int value)
	{
		DBG(value);
		
		if (gs != nullptr)
			gs->logMessage(String(value) + "\n");
	}
	void print(double value)
	{
		DBG(value);
		
		if (gs != nullptr)
			gs->logMessage(String(value) + "\n");
	}
	void print(float value)
	{
		DBG(value);
		
		if (gs != nullptr)
			gs->logMessage(String(value) + "\n");
	}
	void print(HiseEvent e)
	{
		String s;

		s << e.getTypeAsString();
		s << " channel: " << e.getChannel();
		s << " value: " << e.getNoteNumber();
		s << " value2: " << e.getVelocity();
		s << " timestamp: " << (int)e.getTimeStamp();
		s << "\n";

		DBG(s);

		if(gs != nullptr)
			gs->logMessage(s);
	}

	void registerAllObjectFunctions(GlobalScope*) override;

public:

	ConsoleFunctions(GlobalScope* scope_) :
		JitCallableObject("Console"),
		gs(scope_)
	{};

	WeakReference<GlobalScope> gs;
};

class BlockFunctions : public FunctionClass
{
public:

	struct Wrapper
	{
		static float getSample(block b, int index)
		{
			if (isPositiveAndBelow(index, b.size()))
				return b[index];

			return 0.0f;
		}

		static void setSample(block b, int index, float newValue)
		{
			if (isPositiveAndBelow(index, b.size()))
				b[index] = newValue;
		}

		static AddressType getWritePointer(block b)
		{
			return reinterpret_cast<AddressType>(b.getData());
		}

		static int size(block b)
		{
			return b.size();
		}

	};

public:

	BlockFunctions();
};

class MessageFunctions : public FunctionClass
{
public:

	struct Wrapper
	{
#define WRAP_MESSAGE_GET_FUNCTION(returnType, x) static returnType x(HiseEvent e) { return static_cast<returnType>(e.x()); };

		WRAP_MESSAGE_GET_FUNCTION(int, getNoteNumber);
		WRAP_MESSAGE_GET_FUNCTION(int, getVelocity);
		WRAP_MESSAGE_GET_FUNCTION(int, getChannel);
		WRAP_MESSAGE_GET_FUNCTION(int, isNoteOn);
		WRAP_MESSAGE_GET_FUNCTION(int, isNoteOnOrOff);
		WRAP_MESSAGE_GET_FUNCTION(double, getFrequency);

#undef WRAP_MESSAGE_FUNCTION
	};

public:

	MessageFunctions();;
};

class MathFunctions : public FunctionClass
{
public:

	MathFunctions();;
};

}
}