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


struct WrappedBufferBase: public JitCallableObject
{
	WrappedBufferBase(Identifier id) :
		JitCallableObject(id)
	{}

	

	virtual ~WrappedBufferBase() {};

};

template <class WrapType> class WrappedBuffer : public WrappedBufferBase
{
public:

	WrappedBuffer(Identifier id, block b) :
		WrappedBufferBase(id),
		accessor(b)
	{
		registerAllObjectFunctions(nullptr);
	}

	float getInterpolated(double position)
	{
		auto i1 = std::floor((float)position);

		auto s1 = accessor[(int)i1];
		auto s2 = accessor[(int)i1 + 1];
		auto alpha = (float)position - i1;

		return Interpolator::interpolateLinear(s1, s2, alpha);
	}

	void setAt(int index, float value)
	{
		accessor[index] = value;
	}

	float getAt(int index)
	{
		return accessor[index];
	}

	float getWithDelta(int delta)
	{
		return accessor[accessor.currentIndex + delta];
	}

	void setWithDelta(int delta, float value)
	{
		accessor[accessor.currentIndex + delta] = value;
	}

	void seek(int index)
	{
		accessor = index;
	}

	void set(float value)
	{
		*accessor = value;
	}

	void setAndInc(float value)
	{
		*accessor = value;
		++accessor;
	}

	

	float get()
	{
		return *accessor;
	}

	float getAndInc()
	{
		++accessor;
		return *accessor;
	}

	float getAndPostInc()
	{
		auto v = *accessor;
		++accessor;
		return v;
	}

	void registerAllObjectFunctions(GlobalScope*) override
	{
		{
			auto f = createMemberFunction(Types::ID::Void, "seek", { Types::ID::Integer });
			f->setFunction(Wrapper::seek);
			addFunction(f);
		}

		{
			auto f = createMemberFunction(Types::ID::Void, "set", { Types::ID::Float });
			f->setFunction(Wrapper::set);
			addFunction(f);
		}

		{
			auto f = createMemberFunction(Types::ID::Void, "setAt", { Types::ID::Integer, Types::ID::Float });
			f->setFunction(Wrapper::setAt);
			addFunction(f);
		}

		{
			auto f = createMemberFunction(Types::ID::Void, "setAndInc", { Types::ID::Float });
			f->setFunction(Wrapper::setAndInc);
			addFunction(f);
		}

		{
			auto f = createMemberFunction(Types::ID::Void, "setWithDelta", { Types::ID::Integer, Types::ID::Float });
			f->setFunction(Wrapper::setWithDelta);
			addFunction(f);
		}

		{
			auto f = createMemberFunction(Types::ID::Float, "get", {});
			f->setFunction(Wrapper::get);
			addFunction(f);
		}

		{
			auto f = createMemberFunction(Types::ID::Float, "getAt", { Types::ID::Integer });
			f->setFunction(Wrapper::getAt);
			addFunction(f);
		}

		{
			auto f = createMemberFunction(Types::ID::Float, "getWithDelta", { Types::ID::Integer });
			f->setFunction(Wrapper::getWithDelta);
			addFunction(f);
		}

		{
			auto f = createMemberFunction(Types::ID::Float, "getAndInc", {});
			f->setFunction(Wrapper::getAndInc);
			addFunction(f);
		}

		{
			auto f = createMemberFunction(Types::ID::Float, "getAndPostInc", {});
			f->setFunction(Wrapper::getAndPostInc);
			addFunction(f);
		}

		{
			auto f = createMemberFunction(Types::ID::Float, "getInterpolated", {Types::ID::Double});
			f->setFunction(Wrapper::getInterpolated);
			addFunction(f);
		}
	}

	struct Wrapper
	{
		JIT_MEMBER_WRAPPER_1(void, WrappedBuffer, seek, int);
		JIT_MEMBER_WRAPPER_1(void, WrappedBuffer, set, float);
		JIT_MEMBER_WRAPPER_2(void, WrappedBuffer, setAt, int, float);
		JIT_MEMBER_WRAPPER_1(void, WrappedBuffer, setAndInc, float);
		JIT_MEMBER_WRAPPER_2(void, WrappedBuffer, setWithDelta, int, float);
		JIT_MEMBER_WRAPPER_0(float, WrappedBuffer, get);
		JIT_MEMBER_WRAPPER_1(float, WrappedBuffer, getAt, int);
		JIT_MEMBER_WRAPPER_0(float, WrappedBuffer, getAndInc);
		JIT_MEMBER_WRAPPER_0(float, WrappedBuffer, getAndPostInc);
		JIT_MEMBER_WRAPPER_1(float, WrappedBuffer, getWithDelta, int);
		JIT_MEMBER_WRAPPER_1(float, WrappedBuffer, getInterpolated, double);
	};

	BufferHandler::BlockValue<WrapType> accessor;
};


template <typename T> class SmoothedFloat : public JitCallableObject
{
public:

	void reset(T initValue)
	{
		v.setValueWithoutSmoothing(initValue);
	}

	void prepare(double samplerate, double milliSeconds)
	{
		v.reset(samplerate, milliSeconds * 0.001);
	}

	void set(T newTargetValue)
	{
		v.setTargetValue(newTargetValue);
	}

	T next()
	{
		return v.getNextValue();
	}

	juce::LinearSmoothedValue<T> v;

	SmoothedFloat(const Identifier& id, T initialValue) :
		JitCallableObject(id)
	{
		registerAllObjectFunctions(nullptr);
		reset(initialValue);
	};

	~SmoothedFloat()
	{
		functions.clear();
	}

	struct Wrapper
	{
		JIT_MEMBER_WRAPPER_1(void, SmoothedFloat, reset, T);
		JIT_MEMBER_WRAPPER_2(void, SmoothedFloat, prepare, double, double);
		JIT_MEMBER_WRAPPER_1(void, SmoothedFloat, set, T);
		JIT_MEMBER_WRAPPER_0(T, SmoothedFloat, next);
	};

	void registerAllObjectFunctions(GlobalScope*) override
	{
		Types::ID valueType = Types::Helpers::getTypeFromTypeId<T>();

		{
			auto f = createMemberFunction(Types::ID::Void, "reset", { valueType });
			f->setFunction(Wrapper::reset);
			addFunction(f);
		}

		{
			auto f = createMemberFunction(Types::ID::Void, "prepare", { Types::ID::Double, Types::ID::Double });
			f->setFunction(Wrapper::prepare);
			addFunction(f);
		}

		{
			auto f = createMemberFunction(Types::ID::Void, "set", { valueType });
			f->setFunction(Wrapper::set);
			addFunction(f);
		}

		{
			auto f = createMemberFunction(valueType, "next", {});
			f->setFunction(Wrapper::next);
			addFunction(f);
		}
	}
};


class ConsoleFunctions : public JitCallableObject
{
	struct WrapperInt
	{
		JIT_MEMBER_WRAPPER_1(int, ConsoleFunctions, print, int);
	};

	struct WrapperDouble
	{
		JIT_MEMBER_WRAPPER_1(double, ConsoleFunctions, print, double);
	};

	struct WrapperFloat
	{
		JIT_MEMBER_WRAPPER_1(float, ConsoleFunctions, print, float);
	};

	struct WrapperEvent
	{
		JIT_MEMBER_WRAPPER_1(HiseEvent, ConsoleFunctions, print, HiseEvent);
	};

	
	int print(int value)
	{
		DBG(value);
		
		if (gs != nullptr)
			gs->logMessage(juce::String(value) + "\n");

		return value;
	}
	double print(double value)
	{
		DBG(value);
		
		if (gs != nullptr)
			gs->logMessage(juce::String(value) + "\n");

		return value;
	}
	float print(float value)
	{
		DBG(value);
		
		if (gs != nullptr)
			gs->logMessage(juce::String(value) + "\n");

		return value;
	}
	HiseEvent print(HiseEvent e)
	{
		juce::String s;

		s << e.getTypeAsString();
		s << " channel: " << e.getChannel();
		s << " value: " << e.getNoteNumber();
		s << " value2: " << e.getVelocity();
		s << " timestamp: " << (int)e.getTimeStamp();
		s << "\n";

		DBG(s);

		if(gs != nullptr)
			gs->logMessage(s);

		return e;
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
