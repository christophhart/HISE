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














namespace juce
{



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

#define CHECK_CONDITION(condition, errorMessage) if(!(condition)) throw String(errorMessage);



	/** A buffer of floating point data for use in scripted environments.
	*
	*	This class can be wrapped into a var and used like a primitive value.
	*	It is a one dimensional float array - if you want multiple channels, use an Array<var> with multiple variant buffers.
	*	
	*	It contains some handy overload operators to make working with this type more convenient:
	*
	*		b = b * 2.0f		// Applies the gain factor to all samples in the buffer
	*		b = b * otherBuffer // Multiplies the values of the buffers and store them into 'b'
	*		b = b + 2.0f		// adds 2.0f to all samples
	*		b = b + otherBuffer // adds the other buffer
	*	
	*	Important: Because all operations are inplace, these statements are aquivalent:
	*
	*		(b = b * 2.0f) == (b *= 2.0f) == (b *2.0f);
	*
	*	For copying and filling buffers, the '<<' and '>>' operators are used.
	*
	*		0.5f >> b			// fills the buffer with 0.5f (shovels 0.5f into the buffer...)
	*		b << 0.5f			// same as 0.5f >> b
	*		a >> b				// copies the buffer a into b;
	*		a << b				// copies the buffer b into a;
	*
	*	If the Intel IPP library is used, the data will be allocated using the IPP allocators for aligned data
	*
	*/
class VariantBuffer : public DynamicObject
{
public:

	/** Creates a buffer using preallocated data. */
	VariantBuffer(float *externalData, int size_):
		size((externalData != nullptr) ? size : 0)
	{
		if (externalData != nullptr)
		{
			float *data[1] = { externalData };
			buffer.setDataToReferTo(data, 1, size);
		}
	}

	/** Creates a buffer that operates on another buffer. */
	VariantBuffer(VariantBuffer *otherBuffer, int offset=0, int numSamples=-1)
	{
		referToOtherBuffer(otherBuffer, offset, numSamples);
	}

	/** Creates a new buffer with the given sample size. The data will be initialised to 0.0f. */
	VariantBuffer(int samples):
		size(samples)
	{
		buffer = AudioSampleBuffer(1, size);
		buffer.clear();
	}

	SET_MODULE_NAME("buffer");

	void referToOtherBuffer(VariantBuffer *b, int offset = 0, int numSamples = -1)
	{
		referencedBuffer = b;
		
		size = (numSamples == -1) ? b->size : numSamples;

		referToData(b->buffer.getWritePointer(0, offset), size);
	}

	void referToData(float *data, int numSamples)
	{
		buffer.setDataToReferTo(&data, 1, numSamples);

		size = numSamples;
	}

	/** Resizes the buffer and clears it. */
	void setSize(int numChannels, int numSamples)
	{
		
	}

	void addSum(const VariantBuffer &a, const VariantBuffer &b)
	{
		CHECK_CONDITION((size >= a.size && size >= b.size && a.size == b.size), "Wrong buffer sizes for addSum");

		FloatVectorOperations::add(buffer.getWritePointer(0), a.buffer.getReadPointer(0), b.buffer.getReadPointer(0), size);
	}

	void addMul(const VariantBuffer &a, const VariantBuffer &b)
	{
		CHECK_CONDITION((size >= a.size && size >= b.size && a.size == b.size), "Wrong buffer sizes for addSum");

		FloatVectorOperations::addWithMultiply(buffer.getWritePointer(0), a.buffer.getReadPointer(0), b.buffer.getReadPointer(0), size);
	}


	VariantBuffer operator *(const VariantBuffer &b)
	{
		CHECK_CONDITION((b.size >= size), "second buffer too small: " + String(b.size));

		FloatVectorOperations::multiply(buffer.getWritePointer(0), b.buffer.getReadPointer(0), size);
		
		return *this;
	}

	VariantBuffer& operator *=(const VariantBuffer &b)
	{
		CHECK_CONDITION((b.size >= size), "second buffer too small: " + String(b.size));

		FloatVectorOperations::multiply(buffer.getWritePointer(0), b.buffer.getReadPointer(0), size);
		
		return *this;
	}

	VariantBuffer operator *(float gain)
	{
		FloatVectorOperations::multiply(buffer.getWritePointer(0), gain, size);

		buffer.applyGain(gain);

		return *this;
	}

	VariantBuffer& operator *=(float gain)
	{
		buffer.applyGain(gain);

		return *this;
	}

	VariantBuffer operator +(const VariantBuffer &b)
	{
		CHECK_CONDITION((b.size >= size), "second buffer too small: " + String(size));

		FloatVectorOperations::add(buffer.getWritePointer(0), b.buffer.getReadPointer(0), size);

		return *this;
	}

	VariantBuffer& operator +=(const VariantBuffer &b)
	{
		CHECK_CONDITION((b.buffer.getNumSamples() >= buffer.getNumSamples()), "second buffer too small: " + String(buffer.getNumSamples()));

		FloatVectorOperations::add(buffer.getWritePointer(0), b.buffer.getReadPointer(0), buffer.getNumSamples());

		return *this;
	}

	VariantBuffer operator +(float gain)
	{
		FloatVectorOperations::add(buffer.getWritePointer(0), gain, buffer.getNumSamples());

		buffer.applyGain(gain);

		return *this;
	}

	VariantBuffer& operator +=(float gain)
	{
		buffer.applyGain(gain);

		return *this;
	}

	VariantBuffer& operator <<(const VariantBuffer &b)
	{
		CHECK_CONDITION((size >= b.size), "second buffer too small: " + String(size));

		FloatVectorOperations::copy(buffer.getWritePointer(0), b.buffer.getReadPointer(0), size);

		return *this;
	}

	VariantBuffer& operator << (float f)
	{
		FloatVectorOperations::fill(buffer.getWritePointer(0), f, size);
		
		return *this;
	}

	void operator >>(VariantBuffer &destinationBuffer) const
	{
		FloatVectorOperations::copy(destinationBuffer.buffer.getWritePointer(0), buffer.getReadPointer(0), size);
	}

	var getSample(int sampleIndex)
	{
		CHECK_CONDITION(isPositiveAndBelow(sampleIndex, buffer.getNumSamples()), getName() + ": Invalid sample index" + String(sampleIndex));
		return (*this)[sampleIndex];
	}

	void setSample(int sampleIndex, float newValue)
	{
		CHECK_CONDITION(isPositiveAndBelow(sampleIndex, buffer.getNumSamples()), getName() + ": Invalid sample index" + String(sampleIndex));
		buffer.setSample(0, sampleIndex, newValue);
	}

	float & operator [](int sampleIndex)
	{
		CHECK_CONDITION(isPositiveAndBelow(sampleIndex, buffer.getNumSamples()), getName() + ": Invalid sample index" + String(sampleIndex));
		return buffer.getWritePointer(0)[sampleIndex];
	}

	AudioSampleBuffer buffer;

	int size;

	typedef ReferenceCountedObjectPtr<VariantBuffer> Ptr;

	VariantBuffer::Ptr referencedBuffer;

	class Factory: public DynamicObject
	{
	public:

		Factory(int stackSize_):
			stackSize(stackSize_)
		{
			sectionBufferStack.ensureStorageAllocated(stackSize);

			for (int i = 0; i < stackSize; i++)
			{
				sectionBufferStack.add(new VariantBuffer(0));
			}

			setMethod("create", create);
			setMethod("referTo", referTo);
		}

		static var create(const var::NativeFunctionArgs &args)
		{
			if (args.numArguments == 1)
			{
				VariantBuffer *b = new VariantBuffer((int)args.arguments[0]);
				return var(b);
			}
			else
			{
				VariantBuffer *b = new VariantBuffer(0);
				return var(b);
			}
			
		}

		static var referTo(const var::NativeFunctionArgs &args)
		{
			Factory *f = dynamic_cast<VariantBuffer::Factory*>(args.thisObject.getObject());

			CHECK_CONDITION((f != nullptr), "Factory Object is wrong");

			const var *firstArgs = args.arguments; // nice...

			CHECK_CONDITION((firstArgs->isBuffer()), "Referenced object is not a buffer");
			
			if (!firstArgs->isBuffer()) return var::undefined();

			VariantBuffer *b = f->getFreeVariantBuffer();

			// Don't wrap this into CHECK_CONDITION, because it could happen with working scripts...
			if (b == nullptr) throw String("Buffer stack size reached!");

			switch (args.numArguments)
			{
			case 1: b->referToOtherBuffer(firstArgs->getBuffer()); break;
			case 2: b->referToOtherBuffer(args.arguments[0].getBuffer(), (int)args.arguments[1]); break;
			case 3:	b->referToOtherBuffer(args.arguments[0].getBuffer(), (int)args.arguments[1], (int)args.arguments[2]); break;
			}
			
			return var(b);
		}

	private:

		VariantBuffer *getFreeVariantBuffer()
		{

			for (int i = 0; i < stackSize; i++)
			{
				const int refCount = sectionBufferStack.getUnchecked(i)->getReferenceCount();
				if (refCount == 2)
				{
					VariantBuffer::Ptr p(sectionBufferStack[i]);

					return p;
				}
			}

			return nullptr;
		}

		const int stackSize;

		ReferenceCountedArray<VariantBuffer> sectionBufferStack;
	};
};



#undef ADD_WRAPPER_FUNCTION
#undef ADD_WRAPPER_FUNCTION_WITH_RETURN
#undef GET_ARG_OBJECT
#undef ARG
#undef CHECK_CONDITION
#undef SET_MODULE_NAME
#undef ADD_METHOD









}









// Some handy macros to save typing...

#define SET_MODULE_NAME(x) static Identifier getName() {static const Identifier id(x); return id; }; String getInstanceName() const override { return String(x);};
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

#define CHECK_CONDITION(condition, errorMessage) if(!(condition)) throw String(errorMessage);



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

		

	private:

	};


	class DspObject : public BaseObject
	{
	public:

		DspObject()
		{
		}

		/** Call this to setup the module. It will be called automatically before the first call to the processBlock callback */
		virtual void prepareToPlay(double sampleRate, int samplesPerBlock) = 0;

	protected:

		/** Overwrite this and process the incoming buffer. This will be called when you use the >> operator on a buffer. */
		virtual void processBuffer(VariantBuffer &buffer) = 0;

		/** This will be called when using the >> operator on a array of channels. 
		*
		*	The default implementation just calls processBuffer for every buffer in the array, but if you need special processing, overwrite this method
		*	and implement it.
		*/
		virtual void processMultiChannel(Array<var> &channels)
		{
			for (int i = 0; i < channels.size(); i++)
			{
				var *c = &(channels.getReference(i));
				if (c->isBuffer())
				{
					processBuffer( *(c->getBuffer()) );
				}
			}
		}

	public:

		/** Applies the module on the data.
		*
		*	The incoming data can be either a VariantBuffer or a array of VariantBuffers. */
		void operator >>(var &data)
		{
			process(data);
		}

		/** Copies the modules internal data into either the supplied multichannel array or the buffer. */
		void operator << (var &data) const
		{
			if (data.isArray())
			{
				const Array<var> *internalData = getChannels();
				Array<var> *dest = data.getArray();

				CHECK_CONDITION((dest->size() >= internalData->size()), "Too few channels");

				for (int i = 0; i < internalData->size(); i++)
				{
					VariantBuffer &destChannel = *data[i].getBuffer();
					const VariantBuffer &src = *(internalData->getUnchecked(i)).getBuffer();
					
					src >> destChannel;
				}
			}
			else if (data.isBuffer())
			{
				//CHECK_CONDITION((getBuffer() != nullptr && data.getBuffer() != nullptr), "Buffer error");
				*getBuffer() >> *data.getBuffer();
			}
		}

		/** If you want to support multichannel mode for a module, overwrite this function and return an array containing all buffers.
		*/
		virtual const Array<var> *getChannels() const
		{
#if ENABLE_SCRIPTING_SAFE_CHECKS
			throw String(getInstanceName() + ": No multichannel mode");
#endif
			
			jassertfalse;
			return nullptr;
		}

		virtual String getInstanceName() const = 0;

		/** Return the internal buffer of the object.
		*
		*	This should be only used by non inplace calculations.
		*/
		VariantBuffer *getBuffer()
		{
			return const_cast<VariantBuffer*>(getBuffer());
		}

		virtual const VariantBuffer *getBuffer() const
		{
			throw String("No internal storage");

			return nullptr;
		}

	private:

		void process(var &data)
		{
			if (data.isBuffer())
			{
				processBuffer(*(data.getBuffer()));
			}
			else if (data.isArray())
			{
				Array<var> *channels = data.getArray();
				if (channels != nullptr) processMultiChannel(*channels);
				else jassertfalse; // somethings wrong here...
			}
		}
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

		void processMultiChannel(Array<var> &channels) override
		{

		}

		void processBuffer(VariantBuffer &buffer) override
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
			delayedBufferL = new VariantBuffer(samplesPerBlock);
			delayedBufferR = new VariantBuffer(samplesPerBlock);

			delayL.prepareToPlay(sampleRate);
			delayR.prepareToPlay(sampleRate);
		}

		void processBuffer(VariantBuffer &b) override
		{
			const float *inL = b.buffer.getReadPointer(0);
			const float *inR = b.buffer.getReadPointer(1);

			float *l = delayedBufferL->buffer.getWritePointer(0);
			float *r = delayedBufferR->buffer.getWritePointer(1);

			int numSamples = b.buffer.getNumSamples();

			while (--numSamples >= 0)
			{
				*l++ = delayL.getDelayedValue(*inL++);
				*r++ = delayL.getDelayedValue(*inR++);
			}
		}

		void processMultiChannel(Array<var> &channels) override
		{
			const float *inL = channels[0].getBuffer()->buffer.getWritePointer(0);
			const float *inR = channels[1].getBuffer()->buffer.getWritePointer(1);

			float *l = delayedBufferL->buffer.getWritePointer(0);
			float *r = delayedBufferR->buffer.getWritePointer(1);

			int numSamples = channels[0].getBuffer()->size;

			while (--numSamples >= 0)
			{
				*l++ = delayL.getDelayedValue(*inL++);
				*r++ = delayL.getDelayedValue(*inR++);
			}
		}

		const VariantBuffer *getBuffer() const override
		{
			return delayedBufferL;
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

		VariantBuffer::Ptr delayedBufferL;
		VariantBuffer::Ptr delayedBufferR;
	};

	class MoogFilter : public DspObject
	{
	public:

		MoogFilter() :
			DspObject()
		{
			ADD_METHOD(setFrequency);
		};

		SET_MODULE_NAME("moog");

		void prepareToPlay(double sampleRate_, int samplesPerBlock) override
		{
			if (sampleRate_ > 0.0)
			{
				sampleRate = sampleRate_;

				moogL.prepareToPlay(sampleRate_, samplesPerBlock);
				moogL.prepareToPlay(sampleRate_, samplesPerBlock);
			}
		}

		void setFrequency(double frequency)
		{
			freq = frequency / (0.42 * sampleRate);
		}

		void processBuffer(VariantBuffer &buffer) override
		{
			moogL.processInplace(buffer.buffer.getWritePointer(0), buffer.size);
		}

		void processMultiChannel(Array<var> &channels) override
		{
			float *l = channels[0].getBuffer()->buffer.getWritePointer(0);
			float *r = channels[1].getBuffer()->buffer.getWritePointer(1);

			const int numSamples = channels[0].getBuffer()->size;

			moogL.processInplace(l, numSamples);
			moogR.processInplace(r, numSamples);
		}

	private:

		struct Wrappers
		{

			ADD_WRAPPER_FUNCTION(MoogFilter, setFrequency, args.arguments[0]);
		};

		double sampleRate = 44100.0;
		double freq = 20000.0;
		double resonance = 0.5;

		icstdsp::MoogFilter moogL;
		icstdsp::MoogFilter moogR;
	};

	class Filter : public DspObject
	{
	public:

		Filter() :
			DspObject()
		{
			ADD_METHOD(setFrequency);
			ADD_METHOD(setResonance);
			ADD_METHOD(setType);

			static const Identifier lp("LowPass");
			static const Identifier hp("HighPass");
			static const Identifier bp("BandPass");
			static const Identifier pk("Peak");
			static const Identifier nt("Notch");

			setProperty(lp, (int)icstdsp::ChambFilter::FilterType::LowPass);
			setProperty(hp, (int)icstdsp::ChambFilter::FilterType::HighPass);
			setProperty(bp, (int)icstdsp::ChambFilter::FilterType::BandPass);
			setProperty(pk, (int)icstdsp::ChambFilter::FilterType::Peak);
			setProperty(nt, (int)icstdsp::ChambFilter::FilterType::Notch);
		}

		SET_MODULE_NAME("filter");

		void setFrequency(float frequency)
		{
			filterL.setFrequency(frequency);
			filterR.setFrequency(frequency);
		}

		void setResonance(float resonance)
		{
			filterL.setResonance(resonance);
			filterR.setResonance(resonance);
		}

		void prepareToPlay(double sampleRate, int samplesPerBlock) override
		{
			filterL.prepareToPlay(sampleRate, samplesPerBlock);
			filterR.prepareToPlay(sampleRate, samplesPerBlock);
		}

		void setType(int type)
		{
			filterL.setFilterType((icstdsp::ChambFilter::FilterType)type);
		}

		void processMultiChannel(Array<var> &channels) override
		{
			float *l = channels[0].getBuffer()->buffer.getWritePointer(0);
			float *r = channels[1].getBuffer()->buffer.getWritePointer(1);

			const int numSamples = channels[0].getBuffer()->size;

			filterL.processInplace(l, numSamples);
			filterR.processInplace(r, numSamples);
		}

		void processBuffer(VariantBuffer &buffer) override
		{
			float *l = buffer.buffer.getWritePointer(0);

			const int numSamples = buffer.size;

			filterL.processInplace(l, numSamples);
		}

	private:

		icstdsp::ChambFilter filterL;
		icstdsp::ChambFilter filterR;

		struct Wrappers
		{
			ADD_WRAPPER_FUNCTION(Filter, setFrequency, (float)ARG(0));
			ADD_WRAPPER_FUNCTION(Filter, setType, (float)ARG(0));
			ADD_WRAPPER_FUNCTION(Filter, setResonance, (float)ARG(0));
		};
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

		void processMultiChannel(Array<var> &channels) override
		{
			float *inL = channels[0].getBuffer()->buffer.getWritePointer(0);
			float *inR = channels[1].getBuffer()->buffer.getWritePointer(1);

			const int numSamples = channels[0].getBuffer()->size;

			leftFilter.processSamples(inL, numSamples);
			rightFilter.processSamples(inR, numSamples);
		}

		void processBuffer(VariantBuffer &buffer) override
		{
			leftFilter.processSamples(buffer.buffer.getWritePointer(0), buffer.size);
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

	Factory<DynamicObject> factory;

};

#endif  // SCRIPTDSPMODULES_H_INCLUDED
