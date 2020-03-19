/*  ===========================================================================
 *
 *   This file is part of HISE.
 *   Copyright 2016 Christoph Hart
 *
 *   HISE is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   HISE is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
 *
 *   Commercial licenses for using HISE in an closed source project are
 *   available on request. Please visit the project's website to get more
 *   information about commercial licensing:
 *
 *   http://www.hise.audio/
 *
 *   HISE is based on the JUCE library,
 *   which also must be licenced for commercial applications:
 *
 *   http://www.juce.com
 *
 *   ===========================================================================
 */

#pragma once

namespace scriptnode
{
using namespace juce;
using namespace hise;

struct SnexExpressionBase {
	snex::hmath Math;

	static NormalisableRange<double> createNormalisableRange() { return NormalisableRange<double>(0.0, 1.0); };
};

template <typename T> struct RangeBase
{
	static constexpr T from0To1(T min, T max, T value)
	{
		return hmath::map(value, min, max);
	}

	static constexpr T to0To1(T min, T max, T value)
	{
		return (value - min) / (max - min);
	}

	static constexpr T from0To1Skew(T min, T max, T skew, T value)
	{
		auto v = hmath::pow(value, skew);
		return from0To1(min, max, value);
	}

	static constexpr T to0To1Skew(T min, T max, T skew, T value)
	{
		auto v = to0To1(min, max, value);
		return hmath::pow(v, skew);
	}

	static constexpr T from0To1Step(T min, T max, T step, T value)
	{
		auto v = from0To1(min, max, value);
		return v - hmath::fmod(v, step);
	}

	static constexpr T to0To1Step(T min, T max, T step, T value)
	{
		auto v = value - hmath::fmod(value, step);
		return to0To1(min, max, v);
	}

	static constexpr T from0To1StepSkew(T min, T max, T step, T skew, T value)
	{
		auto v = from0To1(min, max, skew, value);
		return v - hmath::fmod(v, step);
	}

	static constexpr T to0To1StepSkew(T min, T max, T step, T skew, T value)
	{
		auto v = value - hmath::fmod(value, step);
		return to0To1(min, max, skew, v);
	}
};

struct Identity
{
	static constexpr double from0To1(double input) { return input; };
	static constexpr double to0To1(double input) { return input; };
	static NormalisableRange<double> createNormalisableRange() { return NormalisableRange<double>(0.0, 1.0); };
};

#define DECLARE_PARAMETER_EXPRESSION(name, x) struct name : public SnexExpressionBase { \
double operator()(double input) { return x; } \
};

#define DECLARE_PARAMETER_RANGE(name, min, max) struct name { \
	static constexpr double to0To1(double input) { return RangeBase<double>::to0To1(min, max, input); } \
	static constexpr double from0To1(double input){ return RangeBase<double>::from0To1(min, max, input); };\
	static NormalisableRange<double> createNormalisableRange() { return NormalisableRange<double>(min, max); } };

#define DECLARE_STEP_PARAMETER_RANGE(name, min, max, step) struct name {\
	static constexpr double to0To1(double input) {  return RangeBase<double>::to0To1Step(min, max, step, input); } \
	static constexpr double from0To1(double input){ return RangeBase<double>::from0To1Step(min, max, step, input);} \
	static NormalisableRange<double> createNormalisableRange() { return NormalisableRange<double>(min, max, step); } };


#define DECLARE_SKEW_PARAMETER_RANGE(name, min, max, skew) struct name {\
	static constexpr double to0To1(double input) {  return RangeBase<double>::to0To1Skew(min, max, skew, input); }\
	static constexpr double from0To1(double input){ return RangeBase<double>::from0To1Skew(min, max, skew, input);} \
	static NormalisableRange<double> createNormalisableRange() { return NormalisableRange<double>(min, max, 0.0, skew); } };

#define DECLARE_STEP_SKEW_PARAMETER_RANGE(name, min, max, step, skew)struct name {\
	static constexpr double to0To1(double input) {  return RangeBase<double>::to0To1StepSkew(min, max, step, skew, input); } \
	static constexpr double from0To1(double input){ return RangeBase<double>::from0To1StepSkew(min, max, step, skew, input);} \
	static NormalisableRange<double> createNormalisableRange() { return NormalisableRange<double>(min, max, step, skew); } };

namespace parameter
{

template <class T, int P> struct single_base
{
	static constexpr int size = 1;

	struct Caller
	{
		void operator()(void* obj, double v)
		{
			T::setParameter<P>(obj, v);
		}
	} f;

	void connect(T& o)
	{
		obj = reinterpret_cast<void*>(&o);
	}

	template <int P> constexpr auto& get() noexcept
	{
		static_assert(P == 0, "not zero");
		return *this;
	}

	void* obj = nullptr;
};

template <class T, int P> struct plain: public single_base<T, P>
{
	template <int P_> void call(double v)
	{
		static_assert(P_ == 0, "not zero");

		jassert(obj != nullptr);
		f(obj, v);
	}
};

template <class T, int P, class Expression> struct expression: public single_base<T, P>
{
	template <int P_> void call(double v)
	{
		static_assert(P_ == 0, "not zero");
		jassert(obj != nullptr);

		f(obj, Expression()(v));
	}
};

template <class T, int P, class RangeType> struct from0to1: public single_base<T, P>
{
	template <int P_> void call(double v)
	{
		static_assert(P_ == 0, "not zero");
		jassert(obj != nullptr);

		f(obj, RangeType::from0to1(v));
	}
};

template <class T, int P, class RangeType> struct to0to1 : public single_base<T, P>
{
	template <int P_> void call(double v)
	{
		static_assert(P_ == 0, "not zero");
		jassert(obj != nullptr);

		f(obj, RangeType::to0to1(v));
	}
};

template <class InputRange, class... Others> struct chain
{
	static constexpr int size = 1;

	template <int P> void call(double v)
	{
		v = InputRange::to0To1(v);

		static_assert(P == 0, "not zero");
		call_each(v, indexes);
	}

	template <std::size_t ...Ns>
	void call_each(double v, std::index_sequence<Ns...>) {
		using swallow = int[];
		(void)swallow {
			1, (std::get<Ns>(others).call(v), void(), int{})...
		};
	}

	template <size_t arg> constexpr auto& get() noexcept
	{
		return std::get<arg>(others);
	}

	std::index_sequence_for<Others...> indexes;
	std::tuple<Others...> others;
};

template <class... Parameters> struct list
{
	static constexpr int size = sizeof...(Parameters);

	template<int P> void call(double v)
	{
		std::get<P>(others).call<0>(v);
	}

	template <size_t P> constexpr auto& get() noexcept
	{
		return std::get<P>(others);
	}

	std::tuple<Parameters...> others;
};

struct empty
{
	static constexpr int size = 0;

	template<int P> void call(double v)
	{
		static_assert(false, "Trying to call a parameter from a empty list");
	}

	template <size_t P> constexpr auto& get() noexcept
	{
		static_assert(false, "Trying to get a parameter from a empty list");
		return *this;
	}
};

}


template <int NumChannels, class T> class fix
{
public:

	static constexpr bool isModulationSource = T::isModulationSource;

	fix() {};

	void initialise(NodeBase* n)
	{
		obj.initialise(n);
	}

	void prepare(PrepareSpecs ps)
	{
		ps.numChannels = getFixChannelAmount();
		obj.prepare(ps);
	}

	forcedinline void reset() noexcept { obj.reset(); }

	forcedinline void process(ProcessData& data) noexcept
	{
		auto dCopy = ProcessData(data);
		dCopy.numChannels = getFixChannelAmount();

		obj.process(dCopy);
	}

	void handleHiseEvent(HiseEvent& e)
	{
		obj.handleHiseEvent(e);
	}

	constexpr static int getFixChannelAmount()
	{
		return NumChannels;
	}

	forcedinline void processSingle(float* frameData, int ) noexcept
	{
		obj.processSingle(frameData, getFixChannelAmount());
	}

	forcedinline bool handleModulation(double& value) noexcept
	{
		return obj.handleModulation(value);
	}

	auto& getObject() { return obj.getObject(); }
	const auto& getObject() const { return obj.getObject(); }

private:

	T obj;
};

template <class T> class skip
{
public:

	static constexpr bool isModulationSource = T::isModulationSource;

	void initialise(NodeBase* n)
	{
		obj.initialise(n);
	}

	void prepare(PrepareSpecs )
	{
		
	}

	forcedinline void reset() noexcept {  }

	forcedinline void process(ProcessData& ) noexcept
	{
		
	}

	void handleHiseEvent(HiseEvent& )
	{
		
	}

	forcedinline void processSingle(float* , int) noexcept
	{
		
	}

	forcedinline bool handleModulation(double& ) noexcept
	{
		return false;
	}

	auto& getObject() { return obj.getObject(); }
	const auto& getObject() const { return obj.getObject(); }

private:

	T obj;
};


namespace wrap
{

template <class T> class event
{
public:

	static constexpr bool isModulationSource = false;

	void initialise(NodeBase* n)
	{
		obj.initialise(n);
	}

	void prepare(PrepareSpecs ps)
	{
		obj.prepare(ps);
	}

	void reset()
	{
		obj.reset();
	}

	void handleHiseEvent(HiseEvent& e)
	{
		obj.handleHiseEvent(e);
	}

	void process(ProcessData& d)
	{
		if (d.eventBuffer != nullptr && !d.eventBuffer->isEmpty())
		{
			float* ptrs[NUM_MAX_CHANNELS];
			int numChannels = d.numChannels;
			memcpy(ptrs, d.data, sizeof(float*) * numChannels);

			auto advancePtrs = [numChannels](float** dt, int numSamples)
			{
				for (int i = 0; i < numChannels; i++)
					dt[i] += numSamples;
			};

			HiseEventBuffer::Iterator iter(*d.eventBuffer);

			int lastPos = 0;
			int numLeft = d.size;
			int samplePos;
			HiseEvent e;

			while (iter.getNextEvent(e, samplePos, true, false))
			{
				int numThisTime = jmin(numLeft, samplePos - lastPos);

				obj.handleHiseEvent(e);

				if (numThisTime > 0)
				{
					ProcessData part(ptrs, numChannels, numThisTime);
					obj.process(part);
					advancePtrs(ptrs, numThisTime);
					numLeft = jmax(0, numLeft - numThisTime);
					lastPos = samplePos;
				}
			}

			if (numLeft > 0)
			{
				ProcessData part(ptrs, numChannels, numLeft);
				obj.process(part);
			}
		}
		else
			obj.process(d);
	}

	void processSingle(float* frameData, int numChannels)
	{
		obj.processSingle(frameData, numChannels);
	}

	bool handleModulation(double& value) noexcept { return false; }

	auto& getObject() { return obj.getObject(); }
	const auto& getObject() const { return obj.getObject(); }

	T obj;
};


template <class T> class frame_x
{
public:

	static constexpr bool isModulationSource = T::isModulationSource;

	void initialise(NodeBase* n)
	{
		obj.initialise(n);
	}

	void prepare(PrepareSpecs ps)
	{
		obj.prepare(ps);
	}

	void handleHiseEvent(HiseEvent& e)
	{
		obj.handleHiseEvent(e);
	}

	forcedinline void reset() noexcept { obj.reset(); }

	forcedinline void process(ProcessData& data) noexcept
	{
		int numToDo = data.size;
		float* frame = (float*)alloca(data.numChannels * sizeof(float));
		float** frameData = (float**)alloca(data.numChannels * sizeof(float*));
		memcpy(frameData, data.data, sizeof(float*)*data.numChannels);
		ProcessData copy(frameData, data.numChannels, data.size);
		copy.allowPointerModification();

		while (--numToDo >= 0)
		{
			copy.copyToFrameDynamic(frame);
			processSingle(frame, data.numChannels);
			copy.copyFromFrameAndAdvanceDynamic(frame);
		}
	}

	forcedinline void processSingle(float* frameData, int numChannels) noexcept
	{
		obj.processSingle(frameData, numChannels);
	}

	bool handleModulation(double& value)
	{
		return obj.handleModulation(value);
	}

	auto& getObject() { return obj.getObject(); }
	const auto& getObject() const { return obj.getObject(); }

private:

	T obj;
};

template <int NumChannels, class T> class frame
{
public:

	static constexpr bool isModulationSource = T::isModulationSource;

	void initialise(NodeBase* n)
	{
		obj.initialise(n);
	}

	void prepare(PrepareSpecs ps)
	{
		ps.numChannels = NumChannels;
		obj.prepare(ps);
	}

	void handleHiseEvent(HiseEvent& e)
	{
		obj.handleHiseEvent(e);
	}

	forcedinline void reset() noexcept { obj.reset(); }

	forcedinline void process(ProcessData& data) noexcept
	{
		jassert(data.numChannels == NumChannels);

		int numToDo = data.size;
		float frame[NumChannels];
		float* frameData[NumChannels];
		memcpy(frameData, data.data, sizeof(float*)*NumChannels);
		ProcessData copy(frameData, NumChannels, data.size);
		copy.allowPointerModification();

		while (--numToDo >= 0)
		{
			copy.copyToFrame<NumChannels>(frame);
			processSingle(frame, NumChannels);
			copy.copyFromFrameAndAdvance<NumChannels>(frame);
		}
	}

	forcedinline void processSingle(float* frameData, int ) noexcept
	{
		obj.processSingle(frameData, NumChannels);
	}

	bool handleModulation(double& value)
	{
		return obj.handleModulation(value);
	}

	auto& getObject() { return obj.getObject(); }
	const auto& getObject() const { return obj.getObject(); }

private:

	T obj;
};




template <int OversamplingFactor, class T> class oversample
{
public:

	static constexpr bool isModulationSource = T::isModulationSource;

	using Oversampler = juce::dsp::Oversampling<float>;

	void prepare(PrepareSpecs ps)
	{
		jassert(lock != nullptr);

		ScopedPointer<Oversampler> newOverSampler;

		auto originalBlockSize = ps.blockSize;

		ps.sampleRate *= (double)OversamplingFactor;
		ps.blockSize *= OversamplingFactor;

		obj.prepare(ps);

		newOverSampler = new Oversampler(ps.numChannels, (int)std::log2(OversamplingFactor), Oversampler::FilterType::filterHalfBandPolyphaseIIR, false);

		if (originalBlockSize > 0)
			newOverSampler->initProcessing(originalBlockSize);

		{
			ScopedLock sl(*lock);
			oversampler.swapWith(newOverSampler);
		}
	}

	forcedinline void reset() noexcept 
	{
		if (oversampler != nullptr)
			oversampler->reset();

		obj.reset(); 
	}

	void handleHiseEvent(HiseEvent& e)
	{
		obj.handleHiseEvent(e);
	}

	forcedinline void processSingle(float* frameData, int numChannels) noexcept
	{
		// Applying oversampling on frame basis is stupid.
		jassertfalse;
	}

	forcedinline void process(ProcessData& d) noexcept
	{
		if (oversampler == nullptr)
			return;

		juce::dsp::AudioBlock<float> input(d.data, d.numChannels, d.size);

		auto output = oversampler->processSamplesUp(input);

		float* data[NUM_MAX_CHANNELS];

		for (int i = 0; i < d.numChannels; i++)
			data[i] = output.getChannelPointer(i);

		ProcessData od;
		od.data = data;
		od.numChannels = d.numChannels;
		od.size = d.size * OversamplingFactor;

		obj.process(od);

		oversampler->processSamplesDown(input);
	}

	bool handleModulation(double& value) noexcept
	{
		return obj.handleModulation(value);
	}

	void initialise(NodeBase* n)
	{
		lock = &n->getRootNetwork()->getConnectionLock();
		obj.initialise(n);
	}

	const auto& getObject() const { return obj.getObject(); }
	auto& getObject() { return obj.getObject(); }

private:

	CriticalSection* lock = nullptr;

	ScopedPointer<Oversampler> oversampler;
	T obj;
};

template <class T, int BlockSize> class fix_block
{
public:

	constexpr static bool isModulationSource = false;

	const auto& getObject() const { return obj.getObject(); }
	auto& getObject() { return obj.getObject(); }

	fix_block() {};

	void initialise(NodeBase* n)
	{
		obj.initialise(n);
	}

	void prepare(PrepareSpecs ps)
	{
		ps.blockSize = BlockSize;
		obj.prepare(ps);
	}

	void reset()
	{
		obj.reset();
	}

	void process(ProcessData& d)
	{
		int numToDo = d.size;

		if (numToDo < BlockSize)
		{
			obj.process(d);
		}
		else
		{
			float* cp[NUM_MAX_CHANNELS];
			memcpy(cp, d.data, d.numChannels * sizeof(float*));

			while (numToDo > 0)
			{
				ProcessData copy(cp, d.numChannels, jmin(BlockSize, numToDo));
				obj.process(copy);

				for (int i = 0; i < d.numChannels; i++)
					cp[i] += copy.size;

				numToDo -= copy.size;
			}
		}
	}

	void processSingle(float* frameData, int numChannels)
	{
		jassertfalse;
	}

	bool handleModulation(double& v)
	{
		return obj.handleModulation(v);
	}

	void handleHiseEvent(HiseEvent& e)
	{
		obj.handleHiseEvent(e);
	}

private:

	T obj;
};


template <class T> class control_rate
{
public:

	constexpr static bool isModulationSource = false;

	void initialise(NodeBase* n)
	{
		obj.initialise(n);
	}

	void prepare(PrepareSpecs ps)
	{
		ps.sampleRate /= (double)HISE_EVENT_RASTER;
		ps.blockSize /= HISE_EVENT_RASTER;
		ps.numChannels = 1;
		this->obj.prepare(ps);
	}

	void reset()
	{
		obj.reset();
		singleCounter = 0;
	}

	void process(ProcessData& data)
	{
		int numToProcess = data.size / HISE_EVENT_RASTER;

		auto d = ALLOCA_FLOAT_ARRAY(numToProcess);
		CLEAR_FLOAT_ARRAY(d, numToProcess);
		ProcessData modData = { &d, 1, numToProcess };
		modData.eventBuffer = data.eventBuffer;
		obj.process(modData);
	}

	void processSingle(float* , int )
	{
		if (--singleCounter <= 0)
		{
			singleCounter = HISE_EVENT_RASTER;
			float lastValue = 0.0f;
			obj.processSingle(&lastValue, 1);
		}
	}

	bool handleModulation(double& )
	{
		return false;
	}

	void handleHiseEvent(HiseEvent& e)
	{
		obj.handleHiseEvent(e);
	}

	auto& getObject() { return obj.getObject(); }
	const auto& getObject() const { return obj.getObject(); }

	T obj;
	int singleCounter = 0;
};


template <class T, class ParameterClass> struct mod: public SingleWrapper<T>
{
	GET_SELF_AS_OBJECT(mod);

	constexpr static bool isModulationSource = false;

	inline void process(ProcessData& data) noexcept
	{
		this->obj.process(data);

		if (this->obj.handleModulation(modValue))
			db(modValue);
	}

	forcedinline void reset() noexcept 
	{
		modValue = 0.0;
		this->obj.reset();
	}

	inline void processSingle(float* frameData, int numChannels) noexcept
	{
		this->obj.processSingle(frameData, numChannels);

		if (this->obj.handleModulation(modValue))
		{
			p.call<0>(modValue);
		}
	}

	void createParameters(Array<HiseDspBase::ParameterData>& data) override
	{
		this->obj.createParameters(data);
	}

	void prepare(PrepareSpecs ps)
	{
		this->obj.prepare(ps);
	}

	bool handleModulation(double& value) noexcept
	{
		return false;
	}

	template <int I, class T> void connect(T& t)
	{
		p.get<I>().connect(t);
	}

#if 0
	void setCallback(const DspHelpers::ParameterCallback& c)
	{
		db = c;
	}
#endif

	ParameterClass p;

	DspHelpers::ParameterCallback db;
	double modValue = 0.0;
};

}





}
