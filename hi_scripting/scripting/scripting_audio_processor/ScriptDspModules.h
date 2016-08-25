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


#ifndef FILL_PARAMETER_ID
#define FILL_PARAMETER_ID(enumClass, enumId, size, text) case (int)enumClass::enumId: strcpy(text, #enumId); size = (int)strlen(text); break;
#endif

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


	class Stereo : public DspBaseObject
	{
	public:

		enum class Parameters
		{
			Pan = 0,
			Width,
			numParameters
		};

		Stereo() :
			DspBaseObject(),
			mb(new VariantBuffer(0)),
			sb(new VariantBuffer(0))
		{};

		SET_MODULE_NAME("stereo");
		
		int getNumParameters() const override { return 2; }

		float getParameter(int index) const override
		{
			if (index == 0) return pan / 100.0f;
			else return width;
		}

		void setParameter(int index, float newValue) override
		{
			if (index == 0) pan = newValue*100.0f;
			else width = newValue;
			
		}

		void prepareToPlay(double /*sampleRate*/, int samplesPerBlock)
		{
			mb = new VariantBuffer(samplesPerBlock);
			sb = new VariantBuffer(samplesPerBlock);
		}

		void processBlock(float** data, int numChannels, int numSamples)
		{
			if (numChannels == 2)
			{
				float *l = data[0];
				float *r = data[1];

				const float thisPan = 0.92f * pan + 0.08f * lastPan;
				lastPan = thisPan;

				const float thisWidth = 0.92f * width + 0.08f * lastWidth;
				lastWidth = thisWidth;

				FloatVectorOperations::multiply(l, BalanceCalculator::getGainFactorForBalance(lastPan, true), numSamples);
				FloatVectorOperations::multiply(r, BalanceCalculator::getGainFactorForBalance(lastPan, false), numSamples);

				const float w = 0.5f * lastWidth;

#if 1
				while (--numSamples >= 0)
				{
					const float m = (*l + *r) * 0.5f;
					const float s = (*r - *l) * w;

					*l = (m - s);
					*r = (m + s);

					l++;
					r++;
				}

#else

				float *s = sb->buffer.getWritePointer(0);
				float *m = mb->buffer.getWritePointer(0);

				FloatVectorOperations::copy(m, l, numSamples);
				FloatVectorOperations::add(m, r, numSamples);
				FloatVectorOperations::multiply(m, 0.5f, numSamples);

				FloatVectorOperations::copy(r, s, numSamples);
				FloatVectorOperations::subtract(s, l, numSamples);
				FloatVectorOperations::multiply(s, 0.5f * width, numSamples);

				FloatVectorOperations::copy(l, m, numSamples);
				FloatVectorOperations::subtract(l, s, numSamples);

				FloatVectorOperations::copy(r, m, numSamples);
				FloatVectorOperations::add(r, s, numSamples);
#endif

			}
		}

		int getNumConstants() const override
		{
			return 2;
		}

		void getIdForConstant(int index, char*name, int &size) const noexcept override
		{
			switch (index)
			{
				FILL_PARAMETER_ID(Parameters, Pan, size, name);
				FILL_PARAMETER_ID(Parameters, Width, size, name);
			}
		};

		bool getConstant(int index, int& value) const noexcept override
		{
			if (index < getNumParameters())
			{
				value = index;
				return true;
			}

			return false;
		};

		

	private:

		VariantBuffer::Ptr mb;
		VariantBuffer::Ptr sb;

		float pan = 0.0f;
		float width = 1.0f;

		float lastPan = 0.0f;
		float lastWidth = 1.0f;

	};

	class Delay : public DspBaseObject
	{
	public:

		enum class Parameters
		{
			DelayTime = 0,
			numParameters
		};

		Delay() :
			DspBaseObject()
		{};

		SET_MODULE_NAME("delay")

		void setParameter(int /*index*/, float newValue) override
		{
			delayTimeSamples = newValue;
			delayL.setDelayTimeSamples((int)newValue);
			delayR.setDelayTimeSamples((int)newValue);			
		};

		int getNumParameters() const override { return 1; };

		float getParameter(int /*index*/) const override { return delayTimeSamples; };

		void prepareToPlay(double sampleRate, int samplesPerBlock) override
		{
			delayedBufferL = new VariantBuffer(samplesPerBlock);
			delayedBufferR = new VariantBuffer(samplesPerBlock);

			delayL.prepareToPlay(sampleRate);
			delayR.prepareToPlay(sampleRate);
		}

		void processBlock(float **data, int numChannels, int numSamples) override
		{
			if (numChannels == 2)
			{
				const float *inL = data[0];
				const float *inR = data[1];

				float *l = delayedBufferL->buffer.getWritePointer(0);
				float *r = delayedBufferR->buffer.getWritePointer(0);

				while (--numSamples >= 0)
				{
					*l++ = delayL.getDelayedValue(*inL++);
					*r++ = delayL.getDelayedValue(*inR++);
				}
			}
			else
			{
				const float *inL = data[0];

				float *l = delayedBufferL->buffer.getWritePointer(0);

				while (--numSamples >= 0)
				{
					*l++ = delayL.getDelayedValue(*inL++);
				}
			}
			
		}
		
	private:

		DelayLine delayL;
		DelayLine delayR;

		float delayTimeSamples = 0.0f;

		VariantBuffer::Ptr delayedBufferL;
		VariantBuffer::Ptr delayedBufferR;
	};

	class SignalSmoother : public DspBaseObject
	{
	public:

		enum class Parameters
		{
			SmoothingTime = 0,
			numParameters
		};

		SignalSmoother() :
			DspBaseObject()
		{};

		SET_MODULE_NAME("smoother");

		void setParameter(int /*index*/, float newValue) override
		{
			smoothingTime = newValue;
			smootherL.setSmoothingTime(newValue);
			smootherR.setSmoothingTime(newValue);
		};

		int getNumParameters() const override { return 1; };

		float getParameter(int /*index*/) const override { return smoothingTime; };

		void prepareToPlay(double sampleRate, int /*samplesPerBlock*/) override
		{
			smootherL.prepareToPlay(sampleRate);
			smootherR.prepareToPlay(sampleRate);
		}

		void processBlock(float **data, int numChannels, int numSamples) override
		{
			if (numChannels == 2)
			{
				float *inL = data[0];
				float *inR = data[1];

				while (--numSamples >= 0)
				{
					*inL = smootherL.smooth(*inL);
					*inR = smootherR.smooth(*inR);

					inL++;
					inR++;
				}
			}
			else
			{
				float *inL = data[0];
				
				while (--numSamples >= 0)
				{
					*inL = smootherL.smooth(*inL);
					
					inL++;
				}
			}
		}

	private:

		Smoother smootherL;
		Smoother smootherR;

		float smoothingTime = 0;
	};



	class SineGenerator : public DspBaseObject
	{
	public:

		SineGenerator() :
			DspBaseObject()
		{

		}

		SET_MODULE_NAME("sine");

		void setParameter(int /*index*/, float /*newValue*/) override {};

		int getNumParameters() const override { return 0; };

		float getParameter(int /*index*/) const override { return -1; };

		void prepareToPlay(double /*sampleRate*/, int /*samplesPerBlock*/) override { }

		void processBlock(float **data, int numChannels, int numSamples) override
		{
			if (numChannels == 2)
			{
				float *inL = data[0];
				float *pitch = data[1];
				float phase = pitch[0];

				while (--numSamples >= 0)
				{
					*inL = std::sin(phase);
					pitch++;
					phase += *pitch;
					inL++;
				}
			}
			else if (numChannels == 3)
			{
				float *inL = data[0];
				float *inR = data[1];
				float *pitch = data[2];
				float phase = pitch[0];

				const int samplesToCopy = numSamples;

				while (--numSamples >= 0)
				{
					*inL = std::sin(phase);
					pitch++;
					phase += *pitch;
					inL++;
				}

				FloatVectorOperations::copy(inR, inL, samplesToCopy);
			}
		}

	};


	class MoogFilter : public DspBaseObject
	{
	public:

        enum class Parameters
        {
            Frequency = 0,
            Resonance,
            numParameters
        };
        
		MoogFilter() :
			DspBaseObject()
		{
			
		};

        SET_MODULE_NAME("moog");
        
        void setParameter(int index, float newValue) override
        {
            Parameters p = (Parameters)index;
            
            switch(p)
            {
                case Parameters::Frequency: frequency = newValue;
                                            freq = newValue / (float)(0.42 * sampleRate);
                                            moogL.setFrequency(freq);
                                            moogR.setFrequency(freq);
                                            break;
                case Parameters::Resonance: resonance = newValue;
                                            moogL.setResonance(resonance);
                                            moogR.setFrequency(resonance);
                                            break;
            }
        };
        
        int getNumParameters() const override { return (int)Parameters::numParameters; };
        
        float getParameter(int index) const override
        {
            Parameters p = (Parameters)index;
            
            switch(p)
            {
                case Parameters::Frequency: return frequency;
                case Parameters::Resonance: return resonance;
            }
            
            return -1;
        };
        
        void prepareToPlay(double sampleRate_, int samplesPerBlock) override
		{
			if (sampleRate_ > 0.0)
			{
				sampleRate = sampleRate_;

				moogL.prepareToPlay(sampleRate_, samplesPerBlock);
				moogL.prepareToPlay(sampleRate_, samplesPerBlock);
			}
		}

        void processBlock(float **data, int numChannels, int numSamples) override
        {
            if (numChannels == 2)
            {
                float *l = data[0];
                float *r = data[1];
                
                moogL.processInplace(l, numSamples);
                moogR.processInplace(r, numSamples);
            }
            else
            {
                float *inL = data[0];
                
                moogL.processInplace(inL, numSamples);
            }
        }
        
	private:

		double sampleRate = 44100.0;
		float freq = 20000.0;
        float frequency = 20000.0;
		float resonance = 0.5;

		icstdsp::MoogFilter moogL;
		icstdsp::MoogFilter moogR;
	};

#if 0
    
	class Filter : public DspBaseObject
	{
	public:

		Filter() :
			DspObject()
		{
			ADD_DYNAMIC_METHOD(setFrequency);
			ADD_DYNAMIC_METHOD(setResonance);
			ADD_DYNAMIC_METHOD(setType);

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
			DYNAMIC_METHOD_WRAPPER(Filter, setFrequency, (float)ARG(0));
			DYNAMIC_METHOD_WRAPPER(Filter, setType, (float)ARG(0));
			DYNAMIC_METHOD_WRAPPER(Filter, setResonance, (float)ARG(0));
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

#endif
};



class HiseCoreDspFactory : public StaticDspFactory
{
	Identifier getId() const override { RETURN_STATIC_IDENTIFIER("core") };

	void registerModules() override
	{
		registerDspModule<ScriptingDsp::Delay>();
		registerDspModule<ScriptingDsp::SignalSmoother>();
		registerDspModule<ScriptingDsp::Stereo>();
        registerDspModule<ScriptingDsp::MoogFilter>();
		registerDspModule<ScriptingDsp::SineGenerator>();
	}
};

#undef FILL_PARAMETER_ID

#endif  // SCRIPTDSPMODULES_H_INCLUDED
