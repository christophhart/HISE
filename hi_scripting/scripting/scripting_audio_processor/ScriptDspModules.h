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

#ifndef SCRIPTDSPMODULES_H_INCLUDED
#define SCRIPTDSPMODULES_H_INCLUDED

namespace hise { using namespace juce;

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
            throw String("No internal storage");
            
            return nullptr;
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



	class SmoothedGainer : public DspBaseObject
	{
	public:

		enum class Parameters
		{
			Gain = 0,
			SmoothingTime,
			FastMode,
            TargetValue,
			numParameters
		};

		SmoothedGainer() :
			DspBaseObject(),
			gain(1.0f),
			smoothingTime(200.0f),
			fastMode(true),
			lastValue(0.0f)
		{
			smoother.setDefaultValue(1.0f);
		};

		SET_MODULE_NAME("smoothed_gainer");

		int getNumParameters() const override { return (int)Parameters::numParameters; }

		float getParameter(int index) const override
		{
			if (index == 0) return gain;
			else if (index == 1) return smoothingTime;
            else if (index == 2) return smoother.getDefaultValue();
			else return -1;
		}

		void setParameter(int index, float newValue) override
		{
            if (index == (int)Parameters::Gain) gain = newValue;
            else if (index == (int)Parameters::SmoothingTime)
            {
                smoothingTime = newValue;
                smoother.setSmoothingTime(smoothingTime);
            }
            else if (index == (int)Parameters::FastMode)
            {
                fastMode = newValue > 0.5f;
            }
            else if (index == (int)Parameters::TargetValue)
            {
                smoother.setDefaultValue(newValue);
            }
            
		}

		void prepareToPlay(double sampleRate, int /*samplesPerBlock*/)
		{
			smoother.prepareToPlay(sampleRate);
			smoother.setSmoothingTime(smoothingTime);
		}

		void processBlock(float** data, int numChannels, int numSamples)
		{
			if (numChannels == 1)
			{
				float *l = data[0];
				
				if (fastMode)
				{
					const float a = 0.99f;
					const float invA = 1.0f - a;

					while (--numSamples >= 0)
					{
						const float smoothedGain = lastValue * a + gain * invA;
						lastValue = smoothedGain;

						*l++ *= smoothedGain;
					}
				}
				else
				{
					while (--numSamples >= 0)
					{
						const float smoothedGain = smoother.smooth(gain);

						*l++ *= smoothedGain;
					}
				}

				
			}

			else if (numChannels == 2)
			{
				if (fastMode)
				{
					const float a = 0.99f;
					const float invA = 1.0f - a;

					float *l = data[0];
					float *r = data[1];

					while (--numSamples >= 0)
					{
						const float smoothedGain = lastValue * a + gain * invA;
						lastValue = smoothedGain;

						*l++ *= smoothedGain;
						*r++ *= smoothedGain;
					}
				}
				else
				{
					float *l = data[0];
					float *r = data[1];

					while (--numSamples >= 0)
					{
						const float smoothedGain = smoother.smooth(gain);

						*l++ *= smoothedGain;
						*r++ *= smoothedGain;
					}
				}
				
			}
		}

		int getNumConstants() const override
		{
			return (int)Parameters::numParameters;
		}

		void getIdForConstant(int index, char*name, int &size) const noexcept override
		{
			switch (index)
			{
				FILL_PARAMETER_ID(Parameters, Gain, size, name);
				FILL_PARAMETER_ID(Parameters, SmoothingTime, size, name);
				FILL_PARAMETER_ID(Parameters, FastMode, size, name);
                FILL_PARAMETER_ID(Parameters, TargetValue, size, name);
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

		float gain;
		float smoothingTime;
		bool fastMode;

		float lastValue;

		Smoother smoother;
	};


	

	// Don't use this for anything else than to check the Debug Logger!
	class GlitchCreator : public DspBaseObject
	{
	public:

		enum class Parameters
		{
			EnableGlitches = 0,
			SlowParameter,
			numParameters
		};

		GlitchCreator() : DspBaseObject() {};

		SET_MODULE_NAME("glitch_creator");

		int getNumParameters() const override { return (int)Parameters::numParameters; }

		float getParameter(int /*index*/) const override
		{
			return -1;
		}

		void setParameter(int index, float newValue) override
		{
			if (index == (int)Parameters::EnableGlitches)
			{
				enabled = newValue > 0.5f;
			}
			else
			{
				for (int i = 0; i < 100; i++)
				{
					doSomethingSlow();
				}
			}
		}

		void doSomethingSlow()
		{
			for (int i = 0; i < 8192; i++)
			{
				randomBuffer[i] = r.nextFloat() * sinf(2.0f + randomBuffer[i]);
			}
		}

		void prepareToPlay(double /*sampleRate*/, int /*samplesPerBlock*/)
		{
			
		}

		void processBlock(float** data, int numChannels, int numSamples)
		{
			if (!enabled)
				return;

			if (numChannels == 1)
			{
				DebugLogger::fillBufferWithJunk(data[0], numSamples);

			}

			else if (numChannels == 2)
			{
				DebugLogger::fillBufferWithJunk(data[0], numSamples);
				DebugLogger::fillBufferWithJunk(data[1], numSamples);
			}
		}

		int getNumConstants() const override
		{
			return (int)Parameters::numParameters;
		}

		void getIdForConstant(int index, char*name, int &size) const noexcept override
		{
			switch (index)
			{
				FILL_PARAMETER_ID(Parameters, EnableGlitches, size, name);
				FILL_PARAMETER_ID(Parameters, SlowParameter, size, name);
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

		bool enabled = true;

		float randomBuffer[8192];

		Random r;
	};

    class AdditiveSynthesiser: public DspBaseObject
    {
    public:

        AdditiveSynthesiser() :
        DspBaseObject()
        {
            FloatVectorOperations::clear(lastValues, 6);
            FloatVectorOperations::clear(b, 6);
        };
        
        SET_MODULE_NAME("additive_synth");
        
        int getNumParameters() const override { return 6; }
        
        float getParameter(int index) const override
        {
            if(index >= 0 && index < 6) return b[index];
            return 0.0f;
        }
        
        void setParameter(int index, float newValue) override
        {
            if(index >= 0 && index < 6)
                b[index] = newValue;
        }
        
        void prepareToPlay(double /*sampleRate*/, int /*samplesPerBlock*/)
        {
            
        }
        
        void processBlock(float** data, int numChannels, int numSamples)
        {
            float* l = data[0];
            
            for(int i = 0; i < numSamples; i++)
            {
                l[i] = process(0.f);
            }
            
            if(numChannels == 2)
                FloatVectorOperations::copy(data[1], l, numSamples);
        }
    
        float process(float /*input*/)
        {
            const float uptimeFloat = (float)uptime;
            
            const float a0 = (lastValues[0]*a + b[0]*invA);
            const float a1 = (lastValues[1]*a + b[1]*invA);
            const float a2 = (lastValues[2]*a + b[2]*invA);
            const float a3 = (lastValues[3]*a + b[3]*invA);
            const float a4 = (lastValues[4]*a + b[4]*invA);
            const float a5 = (lastValues[5]*a + b[5]*invA);
            
            const float v0 = a0 * sinf(uptimeFloat);
            const float v1 = a1 * sinf(2.0f*uptimeFloat);
            const float v2 = a2 * sinf(3.0f*uptimeFloat);
            const float v3 = a3 * sinf(4.0f*uptimeFloat);
            const float v4 = a4 * sinf(5.0f*uptimeFloat);
            const float v5 = a5 * sinf(6.0f*uptimeFloat);
            
            lastValues[0] = a0;
            lastValues[1] = a1;
            lastValues[2] = a2;
            lastValues[3] = a3;
            lastValues[4] = a4;
            lastValues[5] = a5;
            
            uptime += uptimeDelta;
            
            return v0+v1+v2+v3+v4+v5;
        };
        
    private:
        
        double uptime = 0.0;
        double uptimeDelta = 0.03;
        
        float b[6];
        
        float lastValues[6];
        
        const float a = 0.999f;
        const float invA = 0.001f;
  
    };
    
	class Allpass : public DspBaseObject
	{
	public:

		enum class Parameters
		{
			DelayLeft,
			DelayRight,
			SmoothingTime,
			numParameters
		};

		Allpass() :
			DspBaseObject(),
			dl(0.0f),
			dr(0.0f)
		{
			delayL.setDelay(0.0f);
			delayR.setDelay(0.0f);

			
		};

		SET_MODULE_NAME("allpass");

		int getNumParameters() const override { return (int)Parameters::numParameters; }

		float getParameter(int index) const override
		{
			if (index == 0) return dl;
			else if (index == 1) return dr;
			else return -1;
		}

		void setParameter(int index, float newValue) override
		{
			if (index == 0)
			{
				l.setValue(newValue);
				dl = newValue; 
			}
			else if (index == 1)
			{
				r.setValue(newValue);
				dr = newValue;
			}
			else if (index == 2)
			{
				smoothingTime = newValue;
				smootherL.setSmoothingTime(smoothingTime);
				smootherR.setSmoothingTime(smoothingTime);
			}
		}

		void prepareToPlay(double sampleRate, int samplesPerBlock)
		{
			smootherL.prepareToPlay(sampleRate);
			smootherR.prepareToPlay(sampleRate);

			smootherL.setSmoothingTime(smoothingTime);
			smootherR.setSmoothingTime(smoothingTime);

			l.reset(sampleRate / (double)samplesPerBlock, 0.3);
			r.reset(sampleRate / (double)samplesPerBlock, 0.3);
		}

		void processBlock(float** data, int numChannels, int numSamples)
		{
			if (numChannels == 1)
			{
				float *ld = data[0];

				delayL.setDelay(dl);

				while (--numSamples >= 0)
				{
					*ld = delayL.getNextSample(*ld);
                    ld++;
				}
			}

			else if (numChannels == 2)
			{
				float *ld = data[0];
				float *rd = data[1];

				delayL.setDelay(l.getNextValue());
				delayR.setDelay(r.getNextValue());

				while (--numSamples >= 0)
				{
					*ld = delayL.getNextSample(*ld);
					*rd = delayR.getNextSample(*rd);
                    
                    ld++;
                    rd++;
				}
			}
		}

		int getNumConstants() const override
		{
			return (int)Parameters::numParameters;
		}

		void getIdForConstant(int index, char*name, int &size) const noexcept override
		{
			switch (index)
			{
				FILL_PARAMETER_ID(Parameters, DelayLeft, size, name);
				FILL_PARAMETER_ID(Parameters, DelayRight, size, name);
				FILL_PARAMETER_ID(Parameters, SmoothingTime, size, name);
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

		class AllpassDelay
		{
		public:
			AllpassDelay() :
				delay(0.f),
				currentValue(0.f)
			{}

			static float getDelayCoefficient(float delaySamples)
			{
				return (1.f - delaySamples) / (1.f + delaySamples);
			}

			void setDelay(float newDelay) noexcept { delay = jmin<float>(0.999f, newDelay); };

			float getNextSample(float input) noexcept
			{
				float y = input * -delay + currentValue;
				currentValue = y * delay + input;

				return y;
			}

		private:
			float delay, currentValue;
		};

		AllpassDelay delayL;
		AllpassDelay delayR;

		Smoother smootherL;
		Smoother smootherR;

		LinearSmoothedValue<float> l;
		LinearSmoothedValue<float> r;

		float dl, dr, smoothingTime;
	};

	class MidSideEncoder : public DspBaseObject
	{
	public:

		enum Parameters
		{
			Width,
			numParameters
		};

		MidSideEncoder() :
			width(1.0f)
		{}

		/** Overwrite this method and return the name of this module.
		*
		*   This will be used to identify the module within your library so it must be unique. */
		static Identifier getName() { RETURN_STATIC_IDENTIFIER("ms_encoder"); }

		// ================================================================================================================

		void prepareToPlay(double /*sampleRate*/, int /*blockSize*/) override {}

		/** Overwrite this method and do your processing on the given sample data. */
		void processBlock(float **data, int numChannels, int numSamples) override
		{
			if (numChannels == 2)
			{
				float* l = data[0];
				float* r = data[1];

				FloatVectorOperations::multiply(l, 0.5f, numSamples);
				FloatVectorOperations::multiply(r, 0.5f, numSamples);

				while (--numSamples >= 0)
				{
					const float m = *l + *r;
					const float s = width * (*r - *l);

					*l++ = m - s;
					*r++ = m + s;

				}
			}

		}

		// =================================================================================================================

		int getNumParameters() const override { return (int)Parameters::numParameters; }
		float getParameter(int index) const override
		{
			switch ((Parameters)index)
			{
			case Parameters::Width: return width;
			
			case Parameters::numParameters: return 0.0f;
			}

			return 0.0f;
		}

		void setParameter(int index, float newValue) override
		{
			switch ((Parameters)index)
			{
			case Parameters::Width: width = newValue; break;
			
			case Parameters::numParameters: break;
			}
		}

		// =================================================================================================================


		int getNumConstants() const { return (int)Parameters::numParameters; };

		void getIdForConstant(int index, char*name, int &size) const noexcept override
		{
			switch (index)
			{
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

		bool getConstant(int /*index*/, float** /*data*/, int &/*size*/) noexcept override
		{
			return false;
		};

	private:

		float width;

		

	};

	class PeakMeter : public DspBaseObject
	{

	public:

		enum Parameters
		{
			EnablePeak,
			EnableRMS,
			StereoMode,
			PeakDecayFactor,
			RMSDecayFactor,
			PeakLevelLeft,
			PeakLevelRight,
			RMSLevelLeft,
			RMSLevelRight,
			numParameters
		};

		PeakMeter() {};

		/** Overwrite this method and return the name of this module.
		*
		*   This will be used to identify the module within your library so it must be unique. */
		static Identifier getName() { RETURN_STATIC_IDENTIFIER("peak_meter"); }

		// ================================================================================================================

		void prepareToPlay(double sampleRate, int blockSize) override
		{
			if (blockSize != 0)
			{
				bufferLength = (double)blockSize / sampleRate;
			}
			
			recalcDecayCoefficents();
		}

		/** Overwrite this method and do your processing on the given sample data. */
		void processBlock(float **data, int numChannels, int numSamples) override
		{
			AudioSampleBuffer b(data, numChannels, numSamples);

			if (enablePeak)
			{
				const float thisPeakL = b.getMagnitude(0, 0, numSamples);

				if (thisPeakL > peakLevelLeft)
					peakLevelLeft = thisPeakL;
				else
					peakLevelLeft = jmax<float>(peakLevelLeft * internalPeakDecay, thisPeakL);

				if (stereoMode && numChannels == 2)
				{
					const float thisPeakR = b.getMagnitude(1, 0, numSamples);

					if (thisPeakR > peakLevelRight)
						peakLevelRight = thisPeakR;
					else
						peakLevelRight = jmax<float>(peakLevelRight * internalPeakDecay, thisPeakR);
				}
			}
			if (enableRMS)
			{
				const float thisRMSL = b.getRMSLevel(0, 0, numSamples);

				if (thisRMSL > rmsLevelLeft)
					rmsLevelLeft = thisRMSL;
				else
					rmsLevelLeft = jmax<float>(rmsLevelLeft * internalRmsDecay, thisRMSL);

				if (stereoMode && numChannels == 2)
				{
					const float thisRMSR = b.getRMSLevel(1, 0, numSamples);

					if (thisRMSR > rmsLevelRight)
						rmsLevelRight = thisRMSR;
					else
						rmsLevelRight = jmax<float>(rmsLevelRight * internalRmsDecay, thisRMSR);
				}
			}
		}

		// =================================================================================================================

		int getNumParameters() const override { return (int)Parameters::numParameters; }

		float getParameter(int index) const override
		{
			switch ((Parameters)index)
			{
			case Parameters::EnablePeak:		return enablePeak;
			case Parameters::EnableRMS:			return enableRMS;
			case Parameters::PeakLevelLeft:		return peakLevelLeft;
			case Parameters::PeakLevelRight:	return peakLevelRight;
			case Parameters::RMSLevelLeft:		return rmsLevelLeft;
			case Parameters::RMSLevelRight:		return rmsLevelRight;
			case Parameters::StereoMode:		return stereoMode;
			case Parameters::RMSDecayFactor:	return rmsDecayFactor;
			case Parameters::PeakDecayFactor:	return peakLevelDecayFactor;
            case Parameters::numParameters:     jassertfalse; break;
			}

			return 0.0f;
		}

		void setParameter(int index, float newValue) override
		{
			switch ((Parameters)index)
			{
			case Parameters::EnablePeak:		enablePeak = newValue > 0.5f;
												peakLevelLeft = 0.0f; 
												peakLevelRight = 0.0f;
												break;
			case Parameters::EnableRMS:			enableRMS = newValue > 0.5f;
												rmsLevelLeft = 0.0f;
												rmsLevelRight = 0.0f;
												break;
			case Parameters::PeakLevelLeft:			
			case Parameters::PeakLevelRight:		
			case Parameters::RMSLevelLeft:			
			case Parameters::RMSLevelRight:		break;
			case Parameters::StereoMode:		stereoMode = newValue > 0.5f; break;
			case Parameters::RMSDecayFactor:	rmsDecayFactor = newValue; recalcDecayCoefficents(); break;
			case Parameters::PeakDecayFactor:	peakLevelDecayFactor = newValue; recalcDecayCoefficents(); break;
			case Parameters::numParameters:		break;
			}
		}

		// =================================================================================================================


		int getNumConstants() const { return getNumParameters(); };

		void getIdForConstant(int index, char*name, int &size) const noexcept override
		{
			switch (index)
			{
				FILL_PARAMETER_ID(Parameters, EnablePeak, size, name);
				FILL_PARAMETER_ID(Parameters, EnableRMS, size, name);
				FILL_PARAMETER_ID(Parameters, StereoMode, size, name);
				FILL_PARAMETER_ID(Parameters, PeakDecayFactor, size, name);
				FILL_PARAMETER_ID(Parameters, RMSDecayFactor, size, name);
				FILL_PARAMETER_ID(Parameters, PeakLevelLeft, size, name);
				FILL_PARAMETER_ID(Parameters, PeakLevelRight, size, name);
				FILL_PARAMETER_ID(Parameters, RMSLevelLeft, size, name);
				FILL_PARAMETER_ID(Parameters, RMSLevelRight, size, name);
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

		bool getConstant(int /*index*/, float** /*data*/, int &/*size*/) noexcept override
		{
			return false;
		};

	private:

		void recalcDecayCoefficents()
		{
			if (bufferLength > 0.0)
			{
				var inputFactor = 0.8;

				const double baseBufferLength = 512.0 / 44100.0;
				const double baseCoef = log(baseBufferLength) / log(2.0);
				const double coef = log(bufferLength) / log(2.0);

				const double diff = coef - baseCoef;
				const double exp = pow(2.0, diff);
				
				internalPeakDecay = powf(peakLevelDecayFactor, (float)exp);
				internalRmsDecay =  powf(rmsDecayFactor, (float)exp);
			}
		}

		bool enablePeak = true;
		bool enableRMS = false;
		bool stereoMode = true;
		float rmsDecayFactor = 0.5f;
		float peakLevelDecayFactor = 0.3f;
		float peakLevelLeft = 0.0f;
		float peakLevelRight = 0.0f;
		float rmsLevelLeft = 0.0f;
		float rmsLevelRight = 0.0f;
		
		float internalPeakDecay = 0.0f;
		float internalRmsDecay = 0.0f;
			
		double bufferLength = 0.0;
		
	};

	class StereoWidener : public DspBaseObject
	{
	public:

		enum class Parameters
		{
			Width = 0,
			PseudoStereoAmount,
			numParameters
		};

		StereoWidener() :
			width(0.5f),
			pseudoStereo(0.0f)
		{
			f1 = f2 = f3 = f4 = f5 = f6 = 0.0f;

			

			delay1.setParameter(0, 0.0f);
			delay1.setParameter(1, 0.0f);
			delay2.setParameter(0, 0.0f);
			delay2.setParameter(1, 0.0f);
			delay3.setParameter(0, 0.0f);
			delay3.setParameter(1, 0.0f);
			delay1.setParameter(2, 20.0f);
			delay2.setParameter(2, 20.0f);
			delay3.setParameter(2, 20.0f);
		}

		/** Overwrite this method and return the name of this module.
		*
		*   This will be used to identify the module within your library so it must be unique. */
		static Identifier getName() { RETURN_STATIC_IDENTIFIER("stereo"); }

		// ================================================================================================================

		void prepareToPlay(double sampleRate_, int blockSize) override
		{
			sampleRate = sampleRate_;

			delay1.prepareToPlay(sampleRate, blockSize);
			delay2.prepareToPlay(sampleRate, blockSize);
			delay3.prepareToPlay(sampleRate, blockSize);
			msEncoder.prepareToPlay(sampleRate, blockSize);
		}

		/** Overwrite this method and do your processing on the given sample data. */
		void processBlock(float **data, int numChannels, int numSamples) override
		{
			if (numChannels == 2)
			{
				VariantBuffer::sanitizeFloatArray(data, numChannels, numSamples);

				uptime += (double)numSamples / sampleRate;

				delay1.setParameter(0, f1 + (float)sin(uptime * 0.84) * sinAmount);
				delay1.setParameter(1, f2 + (float)sin(uptime * 0.53) * sinAmount);
				delay2.setParameter(0, f3 + (float)sin(uptime * 0.74) * sinAmount);
				delay2.setParameter(1, f4 + (float)sin(uptime * 0.33) * sinAmount);
				delay3.setParameter(0, f5 + (float)sin(uptime * 0.24) * sinAmount);
				delay3.setParameter(1, f6 + (float)sin(uptime * 0.07) * sinAmount);

				delay1.processBlock(data, numChannels, numSamples);
				delay2.processBlock(data, numChannels, numSamples);
				delay3.processBlock(data, numChannels, numSamples);
				msEncoder.processBlock(data, numChannels, numSamples);
			}

		}

		// =================================================================================================================

		int getNumParameters() const override { return (int)Parameters::numParameters; }
		float getParameter(int index) const override
		{
			switch ((Parameters)index)
			{
			case Parameters::Width: return width;
			case Parameters::PseudoStereoAmount: return pseudoStereo;
            case Parameters::numParameters: return 0.0f;
			}

			return 0.0f;
		}

		void setParameter(int index, float newValue) override
		{
			switch ((Parameters)index)
			{
			case Parameters::Width: 
				width = newValue;
				msEncoder.setParameter(0, newValue);
				break;
			case Parameters::PseudoStereoAmount: 
				pseudoStereo = jlimit<float>(0.0f, 1.0f, newValue);
				f1 = pseudoStereo * 0.4f;
				f2 = pseudoStereo * 0.87f;
				f3 = pseudoStereo * 0.93f;
				f4 = pseudoStereo * 0.83f;
				f5 = pseudoStereo * 0.23f;
				f6 = pseudoStereo * 0.7f;
				sinAmount = pseudoStereo * 0.013f;
				break;

            case Parameters::numParameters: break;
			}
		}

		// =================================================================================================================


		int getNumConstants() const { return (int)Parameters::numParameters; };

		void getIdForConstant(int index, char*name, int &size) const noexcept override
		{
			switch (index)
			{
				FILL_PARAMETER_ID(Parameters, Width, size, name);
				FILL_PARAMETER_ID(Parameters, PseudoStereoAmount, size, name);
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

		bool getConstant(int /*index*/, float** /*data*/, int &/*size*/) noexcept override
		{
			return false;
		};

	private:

		Allpass delay1;
		Allpass delay2;
		Allpass delay3;
		MidSideEncoder msEncoder;

		double sampleRate;

		float width;
		float pseudoStereo;

		float f1, f2, f3, f4, f5, f6;

		double uptime = 0.0;

		float sinAmount = 0.0f;

	};

#if 0
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

				while (--numSamples >= 0)
				{
					const float m = (*l + *r) * 0.5f;
					const float s = (*r - *l) * w;

					*l = (m - s);
					*r = (m + s);

					l++;
					r++;
				}
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

			return false;
		};

		

				value = index;
				return true;
			}
	private:

		VariantBuffer::Ptr mb;
		VariantBuffer::Ptr sb;

		float pan = 0.0f;
		float width = 1.0f;

		float lastPan = 0.0f;
		float lastWidth = 1.0f;

	};
#endif

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

		DelayLine<> delayL;
		DelayLine<> delayR;

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

				smootherL.smoothBuffer(inL, numSamples);
				smootherR.smoothBuffer(inR, numSamples);

			}
			else
			{
				float *inL = data[0];
				
				smootherL.smoothBuffer(inL, numSamples);
			}
		}

	private:

		Smoother smootherL;
		Smoother smootherR;

		float smoothingTime = 0;
	};

	class NoiseGenerator : public DspBaseObject
	{
	public:

		enum Parameters
		{
			Gain,
			numParameters
		};

		NoiseGenerator() :
			DspBaseObject()
		{
			gain.reset(44100.0, 0.02f);
			gain.setValue(1.0f);
		}

		SET_MODULE_NAME("noise");

		void setParameter(int /*index*/, float newValue) override
		{
			gain.setValue(newValue);
		};

		int getNumParameters() const override { return (int)Parameters::numParameters; };

		float getParameter(int /*index*/) const override { return gain.getTargetValue(); };

		void prepareToPlay(double sampleRate_, int /*samplesPerBlock*/) override
		{
			gain.reset(sampleRate_, 0.2f);
		}

		void processBlock(float **data, int numChannels, int numSamples) override
		{
			float* inL = data[0];

			if (numChannels == 2)
			{
				float* inR = data[1];

				while (--numSamples >= 0)
				{
					const float v = (r.nextFloat() * 2.0f - 1.0f) * gain.getNextValue();
					*inL++ += v;
					*inR++ += v;
				}
			}
			else
			{
				while (--numSamples >= 0)
				{
					*inL++ = (r.nextFloat() * 2.0f - 1.0f) * gain.getNextValue();
				}
			}
		}

		int getNumConstants() const { return (int)Parameters::numParameters; };

		void getIdForConstant(int index, char*name, int &size) const noexcept override
		{
			switch (index)
			{
				FILL_PARAMETER_ID(Parameters, Gain, size, name);
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

		bool getConstant(int /*index*/, float** /*data*/, int &/*size*/) noexcept override
		{
			return false;
		};

	private:

		Random r;

		LinearSmoothedValue<float> gain;
	};

	class SineGenerator : public DspBaseObject
	{
	public:

		enum class Parameters
		{
			ResetPhase = 0,
			Frequency,
			Phase,
			Amplitude,
			GlideTime,
			numParameters
		};

		SineGenerator() :
			DspBaseObject(),
			uptime(0.0),
			uptimeDelta(0.0),
			gain(1.0),
			phaseOffset(0.0),
			frequency(220.0),
			glideTime(0.0f)
		{

		}

		SET_MODULE_NAME("sine");

		void setParameter(int index, float newValue) override 
		{
			Parameters p = (Parameters)index;

			switch (p)
			{
			case ScriptingDsp::SineGenerator::Parameters::ResetPhase: uptime = 0.0;
				break;
			case ScriptingDsp::SineGenerator::Parameters::Frequency: 
				frequency = newValue;
				updateFrequency();
				break;
			case ScriptingDsp::SineGenerator::Parameters::Phase: phaseOffset = newValue;
				break;
			case ScriptingDsp::SineGenerator::Parameters::Amplitude: gain.setValue(newValue);
				break;
			case ScriptingDsp::SineGenerator::Parameters::GlideTime:
				glideTime = newValue;
				if(sampleRate > 0.0)
					uptimeDelta.reset(sampleRate, glideTime);
				break;
			case ScriptingDsp::SineGenerator::Parameters::numParameters:
				break;
			default:
				break;
			}
		};

		int getNumParameters() const override { return (int)Parameters::numParameters; };

		float getParameter(int /*index*/) const override { return -1; };

		void prepareToPlay(double sampleRate_, int /*samplesPerBlock*/) override 
		{
			sampleRate = sampleRate_;

			gain.reset(sampleRate, 0.02f);

			uptimeDelta.reset(sampleRate, 0.0);

			updateFrequency();

			uptimeDelta.reset(sampleRate, glideTime);
			
			updateFrequency();
		}

		void processBlock(float **data, int numChannels, int numSamples) override
		{
			float* inL = data[0];
			

			const int samplesToCopy = numSamples;

			if (numChannels == 2)
			{
				float* inR = data[1];

				while (--numSamples >= 0)
				{
					float v = (float)std::sin(uptime + phaseOffset) * gain.getNextValue();

					*inL++ += v;
					*inR++ += v;

					uptime += uptimeDelta.getNextValue();
				}
			}
			else
			{
				while (--numSamples >= 0)
				{
					*inL++ += (float)std::sin(uptime + phaseOffset) * gain.getNextValue();
					uptime += uptimeDelta.getNextValue();
				}
			}

			

			if (numChannels == 2)
			{
				FloatVectorOperations::copy(data[1], data[0], samplesToCopy);
			}
		}

		int getNumConstants() const { return (int)Parameters::numParameters; };

		void getIdForConstant(int index, char*name, int &size) const noexcept override
		{
			switch (index)
			{
				FILL_PARAMETER_ID(Parameters, ResetPhase, size, name);
				FILL_PARAMETER_ID(Parameters, Frequency, size, name);
				FILL_PARAMETER_ID(Parameters, Phase, size, name);
				FILL_PARAMETER_ID(Parameters, Amplitude, size, name);
				FILL_PARAMETER_ID(Parameters, GlideTime, size, name);
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

		bool getConstant(int /*index*/, float** /*data*/, int &/*size*/) noexcept override
		{
			return false;
		};

	private:

		
		

		LinearSmoothedValue<float> gain;
		LinearSmoothedValue<double> uptimeDelta;

		float frequency;
		
		float glideTime;

		double phaseOffset;

		double uptime;
		
		double sampleRate;

		void updateFrequency()
		{
			uptimeDelta.setValue(frequency / sampleRate * 2.0 * double_Pi);
		}
	};


	class Biquad : public DspBaseObject
	{
	public:

		enum class Parameters
		{
			Frequency,
			Q,
			Gain,
			Mode,
			numParameters
		};

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
			DspBaseObject()
		{
			coefficients = IIRCoefficients::makeLowPass(44100.0, 20000.0);

		}

		void prepareToPlay(double sampleRate_, int /*samplesPerBlock*/) override
		{
			sampleRate = sampleRate_;

			leftFilter.reset();
			rightFilter.reset();
		}

		void setParameter(int index, float newValue) override
		{
			Parameters p = (Parameters)index;

			switch (p)
			{
			case Parameters::Frequency: setFrequency(newValue); break;
			case Parameters::Q:			setQ(newValue); break;
			case Parameters::Gain:		setGain(newValue); break;
			case Parameters::Mode:		setMode((Mode)(int)newValue); break;
			case Parameters::numParameters: break;
			}
		};

		int getNumParameters() const override { return (int)Parameters::numParameters; };

		float getParameter(int index) const override
		{
			Parameters p = (Parameters)index;

			switch (p)
			{
			case Parameters::Frequency: return (float)frequency;
			case Parameters::Q: return (float)q;
			case Parameters::Gain:		return (float)gain;
			case Parameters::Mode:		return (float)(int)m;
            default: break;
			}

			return -1;
		};

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

		void processBlock(float **data, int numChannels, int numSamples) override
		{
			float *inL = data[0];
			leftFilter.processSamples(inL, numSamples);

			if (numChannels == 2)
			{
				float *inR = data[1];
				rightFilter.processSamples(inR, numSamples);
			}
		}

		int getNumConstants() const override
		{
			return (int)Parameters::numParameters;
		}

		void getIdForConstant(int index, char*name, int &size) const noexcept override
		{
			switch (index)
			{
				FILL_PARAMETER_ID(Parameters, Gain, size, name);
				FILL_PARAMETER_ID(Parameters, Frequency, size, name);
				FILL_PARAMETER_ID(Parameters, Q, size, name);
				FILL_PARAMETER_ID(Parameters, Mode, size, name);
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


		void calcCoefficients()
		{
			switch (m)
			{
			case Mode::LowPass: coefficients = IIRCoefficients::makeLowPass(sampleRate, frequency); break;
			case Mode::HighPass: coefficients = IIRCoefficients::makeHighPass(sampleRate, frequency); break;
			case Mode::LowShelf: coefficients = IIRCoefficients::makeLowShelf(sampleRate, frequency, q, (float)gain); break;
			case Mode::HighShelf: coefficients = IIRCoefficients::makeHighShelf(sampleRate, frequency, q, (float)gain); break;
			case Mode::Peak:      coefficients = IIRCoefficients::makePeakFilter(sampleRate, frequency, q, (float)gain); break;
            default: break;
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

// Disable this until we're jumping to C++14
#define ENABLE_JUCE_DSP 0

#if ENABLE_JUCE_DSP
struct JuceDspModuleFactory: public StaticDspFactory
{
    template <class ModuleType> class BaseModule: public DspBaseObject
    {
    public:

        BaseModule() = default;

        virtual ~BaseModule() = default;

        void prepareToPlay(double sampleRate, int blockSize) override
        {
            dsp::ProcessSpec spec;
            spec.maximumBlockSize = blockSize;
            spec.numChannels = 2;
            spec.sampleRate = sampleRate;

            module.prepare(spec);

            module.reset();
        }

        void processBlock(float** data, int numChannels, int numSamples) override
        {
            dsp::AudioBlock<float> b(data, numChannels, numSamples);
            dsp::ProcessContextReplacing<float> c(b);
            
            module.process(c);
        }

    protected:

        ModuleType module;
    };


    class GainModule: public BaseModule<dsp::Gain<float>>
    {
    public:

        SET_MODULE_NAME("gain");

        enum class ParameterIds
        {
            Gain = 0,
            SmoothingTime,
            numParameterIds
        };

        int getNumParameters() const override
        {
            return (int)ParameterIds::numParameterIds;
        }

        float getParameter(int index) const override
        {
            auto p = ParameterIds(index);

            switch(p)
            {
            case ParameterIds::Gain: return module.getGainLinear();
            case ParameterIds::SmoothingTime: return module.getRampDurationSeconds();
            case ParameterIds::numParameterIds: break;
            default: ;
            }
        }

        void setParameter(int index, float newValue) override
        {
            auto p = ParameterIds(index);

            switch (p)
            {
            case ParameterIds::Gain: module.setGainLinear(newValue); break;
            case ParameterIds::SmoothingTime: module.setRampDurationSeconds(newValue); break;
            case ParameterIds::numParameterIds: break;
            default:;
            }
        }
    };

    Identifier getId() const override { RETURN_STATIC_IDENTIFIER("juce") };

    void registerModules() override
    {
        registerDspModule<GainModule>();
    }
};
#endif

void HiseCoreDspFactory::registerModules()
{
	registerDspModule<ScriptingDsp::Delay>();
	registerDspModule<ScriptingDsp::SignalSmoother>();
		
	registerDspModule<ScriptingDsp::SmoothedGainer>();
	registerDspModule<ScriptingDsp::StereoWidener>();
	registerDspModule<ScriptingDsp::SineGenerator>();
	registerDspModule<ScriptingDsp::NoiseGenerator>();
	registerDspModule<ScriptingDsp::Allpass>();
	registerDspModule<ScriptingDsp::MidSideEncoder>();
	registerDspModule<ScriptingDsp::PeakMeter>();
	registerDspModule<ScriptingDsp::AdditiveSynthesiser>();
	registerDspModule<ScriptingDsp::GlitchCreator>();
	registerDspModule<ScriptingDsp::Biquad>();
}

#undef FILL_PARAMETER_ID

} // namespace hise
#endif  // SCRIPTDSPMODULES_H_INCLUDED
