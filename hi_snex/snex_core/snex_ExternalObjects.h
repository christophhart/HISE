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
namespace Types {
using namespace juce;

template <typename T> struct _ramp
{
	using Type = _ramp<T>;

	struct Wrapper
	{
		JIT_MEMBER_WRAPPER_0(void, Type, reset);
		JIT_MEMBER_WRAPPER_1(void, Type, set, T);
		JIT_MEMBER_WRAPPER_0(T, Type, advance);
		JIT_MEMBER_WRAPPER_0(T, Type, get);
		JIT_MEMBER_WRAPPER_2(void, Type, prepare, double, double);
	};

	void reset()
	{
		stepsToDo = 0;
		value = targetValue;
		delta = T(0);
	}

	void set(T newTargetValue)
	{
		if (numSteps == 0)
		{
			value = targetValue;
			stepsToDo = 0;
		}
		else
		{
			auto d = newTargetValue - value;
			delta = d * stepDivider;
			targetValue = newTargetValue;
			stepsToDo = numSteps;
		}
	}

	T advance()
	{
		if (stepsToDo <= 0)
			return value;

		auto v = value;
		value += delta;
		stepsToDo--;

		return v;
	}

	T get()
	{
		return value;
	}

	void prepare(double samplerate, double timeInMilliseconds)
	{
		auto msPerSample = 1000.0 / samplerate;
		numSteps = timeInMilliseconds * msPerSample;

		if (numSteps > 0)
			stepDivider = T(1) / (T)numSteps;
	}

	static ComplexType::Ptr createComplexType(Compiler& c, const Identifier& id);

	T value = T(0);
	T targetValue = T(0);
	T delta = T(0);
	T stepDivider = T(0);

	int numSteps = 0;
	int stepsToDo = 0;
};

struct EventWrapper
{
	struct Wrapper
	{
		JIT_MEMBER_WRAPPER_0(int, HiseEvent, getNoteNumber);
		JIT_MEMBER_WRAPPER_0(int, HiseEvent, getVelocity);
		JIT_MEMBER_WRAPPER_0(int, HiseEvent, getChannel);
		JIT_MEMBER_WRAPPER_1(void, HiseEvent, setVelocity, int);
		JIT_MEMBER_WRAPPER_1(void, HiseEvent, setChannel, int);
		JIT_MEMBER_WRAPPER_1(void, HiseEvent, setNoteNumber, int);


	};

	static ComplexType::Ptr createComplexType(Compiler& c, const Identifier& id);
};

struct ScriptnodeCallbacks
{
	enum ID
	{
		ResetFunction,
		ProcessFunction,
		ProcessSingleFunction,
		PrepareFunction,
		HandleEventFunction,
		numFunctions
	};

	static Array<FunctionData> getAllPrototypes(Compiler& c, int numChannels);

	static jit::FunctionData getPrototype(Compiler& c, ID id, int numChannels);
};

template struct _ramp<float>;
template struct _ramp<double>;

using sfloat = _ramp<float>;
using sdouble = _ramp<double>;

using namespace Types;


template <int NumChannels> struct ProcessDataFix
{
	

	struct FrameProcessor
	{
		FrameProcessor(ProcessDataFix<NumChannels>& parent)
		{
			frameLimit = parent.data[0].size();

			for (int i = 0; i < NumChannels; i++)
			{
				channels[i] = parent.data[i].begin();
			}

			loadFrame();
		}

		float& operator[](int index)
		{
			return frameData[index];
		}

		const float& operator[](int index) const
		{
			return frameData[index];
		}

		int next()
		{
			return ProcessDataFix::nextFrame(this);
		}

	private:

		friend class ProcessDataFix;

		void loadFrame()
		{
			for (int i = 0; i < NumChannels; i++)
			{
				frameData[i] = channels[i][frameIndex];
			};
		}

		void writeLastFrame()
		{
			jassert(frameIndex > 0);

			for (int i = 0; i < NumChannels; i++)
			{
				channels[i][frameIndex-1] = frameData[i];
			}
		}

		float* channels[NumChannels];
		span<float, NumChannels> frameData;
		int frameLimit = 0;
		int frameIndex = 0;
	};

	static int nextFrame(void* obj)
	{
		auto fp = static_cast<FrameProcessor*>(obj);

		if (fp->frameIndex == 0)
		{
			++fp->frameIndex;
			return fp->frameLimit;
		}

		fp->writeLastFrame();

		if (fp->frameIndex < fp->frameLimit)
		{
			fp->loadFrame();
			++fp->frameIndex;
			return 1;
		}

		return 0;
	}

	dyn<float>* begin() const
	{
		return data.begin();
	}

	dyn<float>* end() const
	{
		return data.end();
	}

	FrameProcessor toFrameData()
	{
		return FrameProcessor(*this);
	}

	Types::span<Types::dyn<float>, NumChannels> data;
	Types::dyn<HiseEvent> events;
	int voiceIndex = -1;
	int shouldReset = 0;
};

struct ProcessDataDyn
{
	float* channelData[NUM_MAX_CHANNELS];
	HiseEventBuffer* b;
	int numChannels = 0;
	int numSamples;
	int voiceIndex = -1;
	int shouldReset = 0;

	template <int NumChannels, int Offset = 0> ProcessDataFix<NumChannels> toFix()
	{
		jassert(Offset + NumChannels <= numChannels);

		ProcessDataFix<NumChannels> f;
		f.events = dyn<HiseEvent>(b->begin(), b->getNumUsed());
		f.voiceIndex = voiceIndex;
		f.shouldReset = shouldReset;

		for (int i = 0; i < NumChannels; i++)
		{
			f.data[i] = block(channelData[i + Offset], numSamples);
		}

		return f;
	}
};

struct PrepareSpecs
{
	double sampleRate = 0.0;
	int blockSize = 0;
	int numChannels = 0;

	static ComplexType::Ptr createComplexType(Compiler& c, const Identifier& id);
};




struct SnexObjectDatabase
{
	static void registerObjects(Compiler& c, int numChannels);

	static void addVariadicGet(VariadicSubType* variadicType);

	static void createProcessData(Compiler& c, const TypeInfo& eventType);
};



}
}