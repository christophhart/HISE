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

#ifndef SCRIPTDSPMODULES_H_INCLUDED
#define SCRIPTDSPMODULES_H_INCLUDED

// Some handy macros to save typing...

#define SET_MODULE_NAME(x) static Identifier getName() {static const Identifier id(x); return id; };
#define ARG(x) args.arguments[x]
#define GET_ARG_OBJECT(classType, argumentIndex) dynamic_cast<classType*>(args.arguments[argumentIndex].getObject())

#define ADD_WRAPPER_FUNCTION(className, functionName, ...) static var functionName(const var::NativeFunctionArgs& args) \
{ \
	if (className* thisObject = dynamic_cast<className*>(args.thisObject.getObject())) \
		{ \
		thisObject->functionName(__VA_ARGS__); \
		} \
	return var::undefined(); \
} 

#define ADD_WRAPPER_FUNCTION_WITH_RETURN(className, functionName, ...) static var functionName(const var::NativeFunctionArgs& args) \
{ \
	if (className* thisObject = dynamic_cast<className*>(args.thisObject.getObject())) \
			{ \
		return thisObject->functionName(__VA_ARGS__); \
			} \
	return var::undefined(); \
} 

#define ADD_METHOD(name) setMethod(#name, Wrappers::name);

#define CHECK_CONDITION(condition, errorMessage) if(!condition) throw String(errorMessage);



#include <map>

template <typename BaseClass>
class Factory
{
public:
	template <typename DerivedClass>
	void registerType(const std::string name)
	{
		static_assert(std::is_base_of<BaseClass, DerivedClass>::value, "Factory::registerType doesn't accept this type because doesn't derive from base class");
		_createFuncs[name] = &createFunc<DerivedClass>;
	}

	BaseClass* create(const std::string name) {
		typename std::map<std::string, PCreateFunc>::const_iterator it = _createFuncs.find(name);
		if (it != _createFuncs.end()) {
			return it.value()();
		}
		return nullptr;
	}

	template <typename DerivedClass>
	void registerType()
	{
		if (std::is_base_of<BaseClass, DerivedClass>::value)
		{
			ids.add(DerivedClass::getName());
			functions.add(&createFunc<DerivedClass>);
		}
	}

	BaseClass* createFromId(const Identifier &id) 
	{
		int index = ids.indexOf(id);

		if (index != -1)
		{
			return functions[index]();
		}

		return nullptr;
	}

private:
	template <typename DerivedClass>
	static BaseClass* createFunc()
	{
		return new DerivedClass();
	}

	typedef BaseClass* (*PCreateFunc)();
	std::map<std::string, PCreateFunc> _createFuncs;

	Array<Identifier> ids;
	Array <PCreateFunc> functions;;


};



class ScriptingDsp
{
public:

	class BaseObject : public DynamicObject
	{
	public:

		SET_MODULE_NAME("Base")

	private:

	};

	struct Buffer : public BaseObject
	{
		Buffer(AudioSampleBuffer *externalBuffer = nullptr)
		{
			if (externalBuffer != nullptr)
				buffer.setDataToReferTo(externalBuffer->getArrayOfWritePointers(), externalBuffer->getNumChannels(), externalBuffer->getNumSamples());

			initMethods();
		}

		Buffer(int channels, int samples)
		{
			setSize(channels, samples);
			buffer.clear();
			initMethods();
		}

		void initMethods()
		{
			ADD_METHOD(setSize);
			ADD_METHOD(copyFrom);
			ADD_METHOD(add);
			ADD_METHOD(getSample);
			ADD_METHOD(setSample);
			ADD_METHOD(getNumSamples);
		}

		SET_MODULE_NAME("buffer");

		void referToBuffer(AudioSampleBuffer &b)
		{
			buffer.setDataToReferTo(b.getArrayOfWritePointers(), b.getNumChannels(), b.getNumSamples());
		}

		/** Resizes the buffer and clears it. */
		void setSize(int numChannels, int numSamples)
		{
			buffer.setSize(numChannels, numSamples);
			buffer.clear();
		}

		var getSample(int channelIndex, int sampleIndex)
		{
			CHECK_CONDITION(channelIndex < buffer.getNumChannels(), getName() + ": Invalid channel index");
			CHECK_CONDITION(sampleIndex < buffer.getNumSamples(), getName() + ": Invalid sample index");

			return buffer.getSample(channelIndex, sampleIndex);
		}

		void setSample(int channelIndex, int sampleIndex, float newValue)
		{
			CHECK_CONDITION(channelIndex < buffer.getNumChannels(), getName() + ": Invalid channel index");
			CHECK_CONDITION(sampleIndex < buffer.getNumSamples(), getName() + ": Invalid sample index");

			buffer.setSample(channelIndex, sampleIndex, newValue);
		}

		var getNumSamples() const
		{
			return buffer.getNumSamples();
		}

		void copyFrom(Buffer *otherBuffer)
		{
			if (otherBuffer == nullptr) return;


			buffer.copyFrom(0, 0, otherBuffer->buffer, 0, 0, buffer.getNumSamples());
			buffer.copyFrom(1, 0, otherBuffer->buffer, 1, 0, buffer.getNumSamples());
		}

		void add(Buffer *otherBuffer)
		{
			buffer.addFrom(0, 0, otherBuffer->buffer, 0, 0, buffer.getNumSamples());
			buffer.addFrom(1, 0, otherBuffer->buffer, 1, 0, buffer.getNumSamples());
		}

		AudioSampleBuffer buffer;

		struct Wrappers
		{

			ADD_WRAPPER_FUNCTION(Buffer, setSize, ARG(0), ARG(1))
			ADD_WRAPPER_FUNCTION(Buffer, add, GET_ARG_OBJECT(Buffer, 0))
			ADD_WRAPPER_FUNCTION(Buffer, copyFrom, GET_ARG_OBJECT(Buffer, 0))

			ADD_WRAPPER_FUNCTION(Buffer, setSample, ARG(0), ARG(1), ARG(2))
			ADD_WRAPPER_FUNCTION_WITH_RETURN(Buffer, getSample, ARG(0), ARG(1))
			ADD_WRAPPER_FUNCTION_WITH_RETURN(Buffer, getNumSamples)
		};
	};

	class DspObject : public BaseObject
	{
	public:

		DspObject()
		{
			setMethod("processBlock", Wrappers::processBlock);
			setMethod("prepareToPlay", Wrappers::prepareToPlay);
			setMethod("getBuffer", Wrappers::getBuffer);
		}

		/** Call this to setup the module*/
		virtual void prepareToPlay(double sampleRate, int samplesPerBlock) = 0;

		/** Call this to process the given buffer.
		*
		*	If you don't want inplace processing (eg. for delays), store it in a internal buffer and return this buffer using getBuffer().
		*/
		virtual void processBlock(Buffer &buffer) = 0;

		/** Return the internal buffer of the object.
		*
		*	This should be only used by non inplace calculations.
		*/
		virtual var getBuffer() { return var::undefined(); }

		struct Wrappers
		{
			static var processBlock(const var::NativeFunctionArgs& args)
			{
				if (DspObject* thisObject = dynamic_cast<DspObject*>(args.thisObject.getObject()))
				{
					Buffer *buffer = dynamic_cast<Buffer*>(args.arguments[0].getObject());

					thisObject->processBlock(*buffer);
				}
				return var::undefined();
			}

			static var prepareToPlay(const var::NativeFunctionArgs& args)
			{
				if (DspObject* thisObject = dynamic_cast<DspObject*>(args.thisObject.getObject()))
				{
					thisObject->prepareToPlay(args.arguments[0], args.arguments[1]);
				}
				return var::undefined();
			}

			static var getBuffer(const var::NativeFunctionArgs& args)
			{
				if (DspObject* thisObject = dynamic_cast<DspObject*>(args.thisObject.getObject()))
				{
					return thisObject->getBuffer();
				}
				return var::undefined();
			}
		};
	};

	class Gain : public DspObject
	{
	public:

		Gain() :
			DspObject()
		{
			setMethod("setGain", Wrappers::setGain);
		}

		SET_MODULE_NAME("gain")

			void prepareToPlay(double sampleRate, int samplesPerBlock) override {};

		void processBlock(Buffer &buffer) override
		{
			float **out = buffer.buffer.getArrayOfWritePointers();

			float *l = out[0];
			float *r = out[1];

			const int numSamples = buffer.buffer.getNumSamples();

			FloatVectorOperations::multiply(l, gain, numSamples);
			FloatVectorOperations::multiply(r, gain, numSamples);
		}

		void setGain(float newGain) noexcept{ gain = newGain; };

		struct Wrappers
		{
			static var setGain(const var::NativeFunctionArgs& args)
			{
				if (Gain* thisObject = dynamic_cast<Gain*>(args.thisObject.getObject()))
				{
					thisObject->setGain(args.arguments[0]);
				}
				return var::undefined();
			}
		};

	private:

		float gain = 1.0f;

	};

	class Delay : public DspObject
	{
	public:

		Delay() :
			DspObject()
		{
			setMethod("setDelayTime", Wrappers::setDelayTime);
		}

		SET_MODULE_NAME("delay")

			void setDelayTime(int newDelayInSamples)
		{
			delayL.setDelayTimeSamples(newDelayInSamples);
			delayR.setDelayTimeSamples(newDelayInSamples);
		}

		void prepareToPlay(double sampleRate, int samplesPerBlock) override
		{
			delayedBuffer = new Buffer(2, samplesPerBlock);



			delayL.prepareToPlay(sampleRate);
			delayR.prepareToPlay(sampleRate);
		}

		void processBlock(Buffer &b) override
		{
			const float *inL = b.buffer.getReadPointer(0);
			const float *inR = b.buffer.getReadPointer(1);

			float *l = delayedBuffer->buffer.getWritePointer(0);
			float *r = delayedBuffer->buffer.getWritePointer(1);

			int numSamples = b.buffer.getNumSamples();

			while (--numSamples >= 0)
			{
				*l++ = delayL.getDelayedValue(*inL++);
				*r++ = delayL.getDelayedValue(*inR++);
			}
		}

		var getBuffer() override
		{
			return var(delayedBuffer);
		}

		struct Wrappers
		{
			static var setDelayTime(const var::NativeFunctionArgs& args)
			{
				if (Delay* thisObject = dynamic_cast<Delay*>(args.thisObject.getObject()))
				{
					thisObject->setDelayTime(args.arguments[0]);
				}
				return var::undefined();
			}
		};



	private:

		DelayLine delayL;
		DelayLine delayR;

		ReferenceCountedObjectPtr<Buffer> delayedBuffer;
	};



	class MoogFilter : public DspObject
	{
	public:

		MoogFilter() :
			DspObject()
		{
			ADD_METHOD(setFrequency);


		};

		SET_MODULE_NAME("icst_moog");

		void prepareToPlay(double sampleRate_, int samplesPerBlock) override
		{
			if (sampleRate_ > 0.0)
			{
				sampleRate = sampleRate_;

				moogL = icstdsp::MoogFilter(sampleRate);
				moogL = icstdsp::MoogFilter(sampleRate);
			}
		}

		void setFrequency(double frequency)
		{
			freq = frequency / (0.42 * sampleRate);
		}

		void processBlock(Buffer &buffer) override
		{
			float *l = buffer.buffer.getWritePointer(0);
			float *r = buffer.buffer.getWritePointer(1);

			const int numSamples = buffer.getNumSamples();

			moogL.Update(l, numSamples, 1.0f / float(numSamples), freq, resonance);
			moogR.Update(r, numSamples, 1.0f / float(numSamples), freq, resonance);
		}

	private:

		struct Wrappers
		{

			ADD_WRAPPER_FUNCTION(MoogFilter, setFrequency, args.arguments[0]);
		};

		double sampleRate = 44100.0;
		double freq = 20000.0;
		double resonance = 0.5;

		icstdsp::MoogFilter moogL = icstdsp::MoogFilter(44100.0);
		icstdsp::MoogFilter moogR = icstdsp::MoogFilter(44100.0);
	};

	class Biquad : public DspObject
	{
	public:

		enum class Mode
		{
			LowPass = 0,
			HighPass,
			LowShelf,
			HighShelf,
			Peak,
			numModes
		};

		SET_MODULE_NAME("biquad")

			Biquad() :
			DspObject()
		{
			coefficients = IIRCoefficients::makeLowPass(44100.0, 20000.0);

			setProperty("LowPass", LowPass);
			setProperty("HighPass", HighPass);
			setProperty("LowShelf", LowShelf);
			setProperty("HighShelf", HighShelf);
			setProperty("Peak", Peak);

			setMethod("setMode", Wrappers::setMode);
			setMethod("setFrequency", Wrappers::setFrequency);
			setMethod("setGain", Wrappers::setGain);
			setMethod("setQ", Wrappers::setQ);
		}

		void prepareToPlay(double sampleRate_, int samplesPerBlock) override
		{
			sampleRate = sampleRate_;
		}

		void setMode(Mode newMode)
		{
			m = newMode;
			calcCoefficients();
		}

		void setFrequency(double newFrequency)
		{
			frequency = newFrequency;
			calcCoefficients();
		}

		void setGain(double newGain)
		{
			gain = newGain;
			calcCoefficients();
		}

		void setQ(double newQ)
		{
			q = newQ;
			calcCoefficients();
		}

		void processBlock(Buffer &b) override
		{
			float *inL = b.buffer.getWritePointer(0);
			float *inR = b.buffer.getWritePointer(1);

			const int numSamples = b.buffer.getNumSamples();

			leftFilter.processSamples(inL, numSamples);
			rightFilter.processSamples(inR, numSamples);
		}

	private:

		struct Wrappers
		{
			static var setMode(const var::NativeFunctionArgs& args)
			{
				if (Biquad* thisObject = dynamic_cast<Biquad*>(args.thisObject.getObject()))
				{
					thisObject->setMode((Mode)(int)args.arguments[0]);
				}
				return var::undefined();
			}

			static var setFrequency(const var::NativeFunctionArgs& args)
			{
				if (Biquad* thisObject = dynamic_cast<Biquad*>(args.thisObject.getObject()))
				{
					thisObject->setFrequency((double)args.arguments[0]);
				}
				return var::undefined();
			}

			static var setGain(const var::NativeFunctionArgs& args)
			{
				if (Biquad* thisObject = dynamic_cast<Biquad*>(args.thisObject.getObject()))
				{
					thisObject->setGain((double)args.arguments[0]);
				}
				return var::undefined();
			}

			static var setQ(const var::NativeFunctionArgs& args)
			{
				if (Biquad* thisObject = dynamic_cast<Biquad*>(args.thisObject.getObject()))
				{
					thisObject->setQ((double)args.arguments[0]);
				}
				return var::undefined();
			}
		};

		void calcCoefficients()
		{
			switch (m)
			{
			case Mode::LowPass: coefficients = IIRCoefficients::makeLowPass(sampleRate, frequency); break;
			case Mode::HighPass: coefficients = IIRCoefficients::makeHighPass(sampleRate, frequency); break;
			case Mode::LowShelf: coefficients = IIRCoefficients::makeLowShelf(sampleRate, frequency, q, gain); break;
			case Mode::HighShelf: coefficients = IIRCoefficients::makeHighShelf(sampleRate, frequency, q, gain); break;
			case Mode::Peak:      coefficients = IIRCoefficients::makePeakFilter(sampleRate, frequency, q, gain); break;
			}

			leftFilter.setCoefficients(coefficients);
			rightFilter.setCoefficients(coefficients);
		}

		double sampleRate = 44100.0;

		Mode m = Mode::LowPass;

		double gain = 0.0;
		double frequency = 20000.0;
		double q = 1.0;

		IIRFilter leftFilter;
		IIRFilter rightFilter;

		IIRCoefficients coefficients;

	};



	
};


class DspFactory : public DynamicObject
{
public:

	DspFactory()
	{
		ADD_METHOD(createModule);

		registerTypes(this);
	};

	var createModule(String name);

	

	static void registerTypes(DspFactory *instance);


private:

	struct Wrappers
	{
		ADD_WRAPPER_FUNCTION_WITH_RETURN(DspFactory, createModule, ARG(0).toString());
	};

	Factory<ScriptingDsp::BaseObject> factory;

};

#endif  // SCRIPTDSPMODULES_H_INCLUDED
