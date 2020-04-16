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






/** This namespace contains template classes that define compile-time range objects which can be passed
	into parameter connections. It has:

	- Identity - no conversion
	- SNEX Expression range - conversion using a custom formula
	- Complex range conversions with skew factor and step sizes
	
	You will most likely use the DECLARE_XXX macros instead of using the classes directly for better readability.
*/
namespace ranges
{

/** The base class for a connection with a custom SNEX expression. 

	You will most likely never use this class directly, but create the parameter
	using the 
	
	DECLARE_PARAMETER_EXPRESSION()
	
	macro that just contains the formula.
*/
struct SnexExpressionBase 
{
	/** Allows `Math.min(a, b)` syntax... */
	static snex::hmath Math;

	/** There's no range conversion going on so we return the identity. */
	static NormalisableRange<double> createNormalisableRange() { return NormalisableRange<double>(0.0, 1.0); };
};


/** @internal A helper class that contains the actual range calculations. 

	It is used by the various range macro definitions below to create classes with compile time range conversions. */
template <typename T> struct RangeBase
{
	static constexpr T from0To1(T min, T max, T value) { return hmath::map(value, min, max); }
	static constexpr T to0To1(T min, T max, T value) { return (value - min) / (max - min); }
	static constexpr T from0To1Skew(T min, T max, T skew, T value) { auto v = hmath::pow(value, skew); return from0To1(min, max, value); }
	static constexpr T to0To1Skew(T min, T max, T skew, T value) { return hmath::pow(to0To1(min, max, value), skew); }
	static constexpr T to0To1Step(T min, T max, T step, T value) { return to0To1(min, max, value - hmath::fmod(value, step)); }
	static constexpr T to0To1StepSkew(T min, T max, T step, T skew, T value) { return to0To1(min, max, skew, value - hmath::fmod(value, step)); }

	static constexpr T from0To1Step(T min, T max, T step, T value)
	{
		auto v = from0To1(min, max, value);
		return v - hmath::fmod(v, step);
	}
	static constexpr T from0To1StepSkew(T min, T max, T step, T skew, T value)
	{
		auto v = from0To1(min, max, skew, value);
		return v - hmath::fmod(v, step);
	}
};

/** A range without any conversion.*/
struct Identity
{
	static constexpr double from0To1(double input) { return input; };
	static constexpr double to0To1(double input) { return input; };
	static NormalisableRange<double> createNormalisableRange() { return NormalisableRange<double>(0.0, 1.0); };
};


/** Declares a identity range. */
#define DECLARE_IDENTITY_RANGE(name) struct name: public Identity {};

/** A shortcut to declare an expression parameter */
#define DECLARE_PARAMETER_EXPRESSION(name, x) struct name : public ranges::SnexExpressionBase { \
static double op(double input) { return x; } }; 

/** Declares a linear range with a custom range. */
#define DECLARE_PARAMETER_RANGE(name, min, max) struct name { \
	static constexpr double to0To1(double input) { return ranges::RangeBase<double>::to0To1(min, max, input); } \
	static constexpr double from0To1(double input){ return ranges::RangeBase<double>::from0To1(min, max, input); };\
	static NormalisableRange<double> createNormalisableRange() { return NormalisableRange<double>(min, max); } };

/** Declares a linear range with discrete steps. */
#define DECLARE_PARAMETER_RANGE_STEP(name, min, max, step) struct name {\
	static constexpr double to0To1(double input) {  return ranges::RangeBase<double>::to0To1Step(min, max, step, input); } \
	static constexpr double from0To1(double input){ return ranges::RangeBase<double>::from0To1Step(min, max, step, input);} \
	static NormalisableRange<double> createNormalisableRange() { return NormalisableRange<double>(min, max, step); } };

/** Declares a skewed range with a settable skew factor. */
#define DECLARE_PARAMETER_RANGE_SKEW(name, min, max, skew) struct name {\
	static constexpr double to0To1(double input) {  return ranges::RangeBase<double>::to0To1Skew(min, max, skew, input); }\
	static constexpr double from0To1(double input){ return ranges::RangeBase<double>::from0To1Skew(min, max, skew, input);} \
	static NormalisableRange<double> createNormalisableRange() { return NormalisableRange<double>(min, max, 0.0, skew); } };

/** Declares a skewed range with discrete steps. */
#define DECLARE_PARAMETER_RANGE_STEP_SKEW(name, min, max, step, skew)struct name {\
	static constexpr double to0To1(double input) {  return ranges::RangeBase<double>::to0To1StepSkew(min, max, step, skew, input); } \
	static constexpr double from0To1(double input){ return ranges::RangeBase<double>::from0To1StepSkew(min, max, step, skew, input);} \
	static NormalisableRange<double> createNormalisableRange() { return NormalisableRange<double>(min, max, step, skew); } };

}

/** This namespace contains different parameter templates that can be used to create compile time callbacks with the same
	behaviour as the connections in scriptnode. 
	
	

*/
namespace parameter
{

/** The base class for all parameters that represent a single connection between two nodes. 

	This class has two template arguments:

	- T: the object that this parameter operates on
	- I: the parameter index

	Because these things are compile-time constants, the compiler should be able to resolve and
	inline the function call to the parameter callback.
*/
template <class T, int P> struct single_base
{
	/** @internal: has to be defined for compatibility with the chained parameter types. */
	static constexpr int size = 1;

	/** A helper class that calls the setParameter function of the T object. */
	struct Caller
	{
		void operator()(void* obj, double v)
		{
			T::setParameter<P>(obj, v);
		}
	} f;

	/** Connects the parameter to the given object instance `o`. */
	template <class MaybeWrapped> constexpr void connect(MaybeWrapped& o)
	{
		obj = reinterpret_cast<void*>(&o.getObject());
	}

	/** @internal */
	template <int P_> constexpr auto& get() noexcept
	{
		static_assert(P_ == 0, "parameter index to single parameter must be zero");
		return *this;
	}

	void* obj = nullptr;
};

/** A dummy class that can be used when the container does not have any macro parameters. 

	Since every container template requires a parameter class, using this class will not generate
	any code if the container does not need to have macro parameters. */
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

	template <int P> NormalisableRange<double> createParameterRange()
	{
		static_assert(P == 0, "not zero");
		return NormalisableRange<double>();
	}
};

/** The most simple parameter type without any range conversion. */
template <class T, int P> struct plain: public single_base<T, P>
{
	template <int P_> void call(double v)
	{
		static_assert(P_ == 0, "not zero");

		jassert(obj != nullptr);
		f(obj, v);
	}

	template <int P_> NormalisableRange<double> createParameterRange()
	{
		static_assert(P_ == 0, "not zero");
		return NormalisableRange<double>(0.0, 1.0);
	}
};

/** A parameter that takes a expression class for converting the value that is passed to the parameter callback. 
	
	the Expression argument should be a class derived by SnexExpressionBase 
	(and best created using the DECLARE_PARAMETER_EXPRESSION macro).
*/
template <class T, int P, class Expression> struct expression: public single_base<T, P>
{
	template <int P_> void call(double v)
	{
		static_assert(P_ == 0, "not zero");
		jassert(obj != nullptr);

		f(obj, Expression()(v));
	}

	template <int P_> NormalisableRange<double> createParameterRange()
	{
		static_assert(P_ == 0, "not zero");
		return NormalisableRange<double>();
	}
};

/** A parameter that converts the input from 0...1 to the given range.

	The RangeType argument must be a class created by one of the 
	
	DECLARE_PARAMETER_RANGE_XXX

	macros that define a helper class with the required function signature.

	This class is usually used in connections from a macro parameter in order to
	convert the range to the respective limits for each connection. */
template <class T, int P, class RangeType> struct from0to1: public single_base<T, P>
{
	template <int P_=0> void call(double v)
	{
		static_assert(P_ == 0, "not zero");
		jassert(obj != nullptr);

		auto converted = RangeType::from0To1(v);
		f(obj, converted);
	}

	template <int P_> NormalisableRange<double> createParameterRange()
	{
		static_assert(P_ == 0, "not zero");

		// Return the default range here...
		return NormalisableRange<double>(0.0, 1.0);
	}
};

/** A parameter that converts the input to 0...1 from the given range.

	The RangeType argument must be a class created by one of the

	DECLARE_PARAMETER_RANGE_XXX

	macros that define a helper class with the required function signature.

	This class is usually used in macro parameters to convert the "public" knob
	range into the normalised range that is sent to each connection.
*/
template <class T, int P, class RangeType> struct to0to1 : public single_base<T, P>
{
	template <int P_> void call(double v)
	{
		static_assert(P_ == 0, "not zero");
		jassert(obj != nullptr);

		f(obj, RangeType::to0to1(v));
	}

	template <int P_> NormalisableRange<double> createParameterRange()
	{
		static_assert(P_ == 0, "not zero");
		return RangeType::createNormalisableRange();
	}
};

/** A parameter chain is a list of parameter callbacks that are processed serially. 

	The InputRange template argument must be a range class and will be used to convert
	the input to 0...1 before it is processed with each parameter in the list.
	
	The Others... variadic template argument is a list of all parameters and can be used
	to control multiple parameters at once.
*/
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
			1, (std::get<Ns>(others).call<0>(v), void(), int{})...
		};
	}

	template <int P_> NormalisableRange<double> createParameterRange()
	{
		static_assert(P_ == 0, "not zero");
		return InputRange::createNormalisableRange();
	}

	template <size_t arg> constexpr auto& get() noexcept
	{
		return std::get<arg>(others);
	}

	std::index_sequence_for<Others...> indexes;
	std::tuple<Others...> others;
};

/** The parameter list is a collection of multiple parameters that can be called individually.

	It can be used to add multiple macro parameters to a chain and supports nested calls.

	The Parameters... argument just contains a list of parameters that will be called by their
	index - so the first parameter can be called using list::call<0>(value).

	If this class is supplied to a container node template, it will forward the calls to 

	T::setParameter<I>() to the call<I>() method, so you can connect it like a hardcoded node.
*/
template <class... Parameters> struct list
{
	static constexpr int size = sizeof...(Parameters);

	template<int P> void call(double v)
	{
		std::get<P>(others).call<0>(v);
	}

	template <int P_> NormalisableRange<double> createParameterRange()
	{
		return get<P_>().createParameterRange<0>();
	}

	template <size_t P> constexpr auto& get() noexcept
	{
		return std::get<P>(others);
	}

	std::tuple<Parameters...> others;
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

	bool isPolyphonic() const { return obj.isPolyphonic(); }

	Component* createExtraComponent(PooledUIUpdater* updater) { return nullptr; };
	int getExtraWidth() const { return 0; };
	int getExtraHeight() const { return 0; };

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

	HardcodedNode* getAsHardcodedNode() { return obj.getAsHardcodedNode(); }

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
			p.template call<0>(modValue);
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
			p.template call<0>(modValue);
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
