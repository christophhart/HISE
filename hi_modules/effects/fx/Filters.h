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
*   which must be separately licensed for closed source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

#ifndef FILTERS_H_INCLUDED
#define FILTERS_H_INCLUDED

namespace hise { using namespace juce;

#define USE_STATE_VARIABLE_FILTERS 1


#ifndef MIN_FILTER_FREQ
#define MIN_FILTER_FREQ 20.0
#endif

/** A base class for filters with multiple channels. 
*
*   It exposes an interface for different filter types which have common methods for
*   setting their parameters, initialisation etc.
*/
class MultiChannelFilter
{
public:

	MultiChannelFilter()
	{
		
	}

	virtual ~MultiChannelFilter() {}

    /** Set the filter type. The type is just a plain integer so it's up to your
    *   filter implementation to decide what is happening with the value. */
	void setType(int newType)
	{
		if (type != newType)
		{
			type = newType;
			updateCoefficients();
		}
	}

    /** Set the new frequency (real values from 20Hz to Fs/2). */
	void setFrequency(double newFrequency)
	{
		if (frequency != newFrequency)
		{
			frequency = newFrequency;
			updateCoefficients();
		}
	}

	void setFreqAndQ(double newFrequency, double newQ)
	{
		frequency = newFrequency;
		q = newQ;
		updateCoefficients();
	}

    /** Set the resonance. */
	void setQ(double newQ)
	{
		if (q != newQ)
		{
			q = newQ;
		}
		
		updateCoefficients();
	}

	void setGain(double newGain)
	{
		if (gain != newGain)
		{
			gain = newGain;
			updateCoefficients();
		}
	}

    /** Sets the samplerate. This will be automatically called whenever the sample rate changes. */
	void setSampleRate(double newSampleRate)
	{
		sampleRate = newSampleRate;

		reset();
		updateCoefficients();
	}

    /** Sets the amount of channels. */
	void setNumChannels(int newNumChannels)
	{
		numChannels = jlimit<int>(0, NUM_MAX_CHANNELS, newNumChannels);
	}

    /** Overwrite this method and reset all internal variables like previous values etc. This will be called
    *   when a new voice is started to prevent clicks in the processing. */
	virtual void reset() = 0;

    /** this will be called whenever the frequency, q, or samplerate changes. */
	virtual void updateCoefficients() = 0;

    /** Implement the filter algorithm here. */
	virtual void processSamples(AudioSampleBuffer& buffer, int startSample, int numSamples) = 0;

protected:

	double sampleRate = 44100.0;
	double frequency = 10000.0;
	double q = 1.0;
	double gain = 1.0;
	int type = 0;
	int numChannels = 2;

};


class MoogFilter: public MultiChannelFilter
{
public:

	enum Mode
	{
		OnePole = 0,
		TwoPole,
		FourPole,
		numModes
	};

	MoogFilter()
	{
		in1 = data;
		in2 = in1 + NUM_MAX_CHANNELS;
		in3 = in2 + NUM_MAX_CHANNELS;
		in4 = in3 + NUM_MAX_CHANNELS;
		out1 = in4 + NUM_MAX_CHANNELS;
		out2 = out1 + NUM_MAX_CHANNELS;
		out3 = out2 + NUM_MAX_CHANNELS;
		out4 = out3 + NUM_MAX_CHANNELS;


		reset();

		setFreqAndQ(20000.0, 1.0);
	}

	void reset()
	{
		memset(data, 0, sizeof(double)*NUM_MAX_CHANNELS * 8);
	}

	void setMode(int newMode)
	{
		mode = (Mode)newMode;
	};

	

	void updateCoefficients() override
	{
		auto lFrequency = jlimit<double>(20.0, 20000.0, frequency);

		fc = lFrequency / (0.5 *sampleRate);
		res = q / 2.0;
		if (res > 4.0) res = 4.0;
		f = fc * 1.16;
		fss = (f * f) * (f * f);
		invF = 1.0 - f;
		fb = res * (1.0 - 0.15 * f * f);
	}

	
	void processSamples(AudioSampleBuffer& buffer, int startSample, int numSamples) override
	{
		if (numChannels != buffer.getNumChannels())
		{
			setNumChannels(buffer.getNumChannels());
		}

		for (int c = 0; c < numChannels; c++)
		{
			float* d = buffer.getWritePointer(c, startSample);

			for (int i = 0; i < numSamples; i++)
			{
				double input = (double)d[i];

				input -= out4[c] * fb;
				input *= 0.35013 * fss;
				out1[c] = input + 0.3 * in1[c] + invF * out1[c]; 
				in1[c] = input;
				out2[c] = out1[c] + 0.3 * in2[c] + invF * out2[c]; 
				in2[c] = out1[c];
				out3[c] = out2[c] + 0.3 * in3[c] + invF * out3[c]; 
				in3[c] = out2[c];
				out4[c] = out3[c] + 0.3 * in4[c] + invF * out4[c]; 
				in4[c] = out3[c];
				d[i] = 2.0f * (float)out4[c];
			}
		}
	}

private:

	enum Index
	{
		In1 = 0,
		In2,
		In3,
		In4,
		Out1,
		Out2,
		Out3,
		Out4
	};

	Mode mode;

	double data[NUM_MAX_CHANNELS * 8];

	double* in1;
	double* in2;
	double* in3;
	double* in4;

	double* out1;
	double* out2;
	double* out3;
	double* out4;
	
	double f;
	double fss;
	double invF;
	double fb;
	double fc;
	double res;
	
};


class SimpleOnePole: public MultiChannelFilter
{
public:

	enum FilterType
	{
		LP=0,
		HP,
		numTypes
	};

	SimpleOnePole()
	{
		memset(lastValues, 0, sizeof(float)*NUM_MAX_CHANNELS);
	}

	void reset() override
	{
		memset(lastValues, 0, sizeof(float)*numChannels);
	}

	void updateCoefficients() override
	{
		const double x = exp(-2.0*double_Pi*frequency / sampleRate);

		a0 = (float)(1.0 - x);
		b1 = (float)-x;
	}

	void processSamples(AudioSampleBuffer& buffer, int startSample, int numSamples) override
	{
		if (buffer.getNumSamples() != numChannels)
		{
			setNumChannels(buffer.getNumChannels());
		}
		
		switch (type)
		{
		case FilterType::HP:
		{
			for (int c = 0; c < numChannels; c++)
			{
				float *d = buffer.getWritePointer(c, startSample);

				for (int i = 0; i < numSamples; i++)
				{
					const float tmp = a0*d[i] - b1*lastValues[c];
					lastValues[c] = tmp;
					d[i] = d[i] - tmp;

				}
			}

			break;
		}
		case FilterType::LP:
		{
			for (int c = 0; c < numChannels; c++)
			{
				float *d = buffer.getWritePointer(c, startSample);

				for (int i = 0; i < numSamples; i++)
				{
					d[i] = a0*d[i] - b1*lastValues[c];
					lastValues[c] = d[i];
				}
			}

			break;
		}

		}

		
	}

private:

	float lastValues[NUM_MAX_CHANNELS];

	float a0;
	float b1;
};

class RingmodFilter : public MultiChannelFilter
{

	void updateCoefficients() override
	{
		uptimeDelta = frequency / sampleRate * 2.0 * double_Pi;
		oscGain = jmap<float>((float)q, 0.3f, 9.9f, 0.0f, 1.0f);
		oscGain = jlimit<float>(0.0f, 1.0f, oscGain);
	}

	void reset() override
	{
		uptime = 0.0;
	}

	void processSamples(AudioSampleBuffer& buffer, int startSample, int numSamples) override
	{
		const float invGain = 1.0f - oscGain;

		for (int i = 0; i < numSamples; i++)
		{
			const float oscValue = oscGain * (float)std::sin(uptime);;
			

			for (int c = 0; c < numChannels; c++)
			{
				const float input = buffer.getSample(c, i + startSample);

				buffer.setSample(c, i + startSample, invGain * input + input * oscValue);
			}

			uptime += uptimeDelta;
			
		}


		
	}

private:

	double uptimeDelta = 0.0;
	double uptime = 0.0;
	float oscGain;

	

};

class StaticBiquad : public MultiChannelFilter
{
public:

	enum FilterType
	{
		LowPass,
		HighPass,
		HighShelf,
		LowShelf,
		Peak,
		ResoLow,
		numFilterTypes
	};


	void reset() override
	{
		for (int i = 0; i < numChannels; i++)
		{
			filters[i].reset();
		}
	}

	void processSamples(AudioSampleBuffer& b, int startSample, int numSamples)
	{
		if (numChannels != b.getNumChannels())
		{
			setNumChannels(b.getNumChannels());
		}

		for (int i = 0; i < numChannels; i++)
		{
			float* d = b.getWritePointer(i, startSample);
			filters[i].processSamples(d, numSamples);
		}
	}

	void updateCoefficients() override;

	IIRCoefficients currentCoefficients;

	IIRFilter filters[NUM_MAX_CHANNELS];
};



class PhaseAllpass : public MultiChannelFilter
{
public:

	void processSamples(AudioSampleBuffer& b, int startSample, int numSamples) override
	{
		for (int c = 0; c < b.getNumChannels(); c++)
		{
			for (int i = 0; i < numSamples; i++)
			{
				float* d = b.getWritePointer(c, i + startSample);
				*d = filters[c].getNextSample(*d);
			}
		}
	}
	

	void updateCoefficients() override
	{
		if (sampleRate <= 0.0f)
		{
			return;
		}

		for (int i = 0; i < numChannels; i++)
		{
			filters[i].fMin = (float)frequency;
			filters[i].minDelay = (float)(frequency / (sampleRate / 2.0));
			filters[i].feedback = (float)jmap<double>(q, 0.3, 9.9, 0.0, 0.99);
		}
	}

	void reset() override
	{
		updateCoefficients();
	}

private:

	struct InternalFilter
	{
		
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

			void setDelay(float newDelay) noexcept { delay = newDelay; };

			float getNextSample(float input) noexcept
			{
				float y = input * -delay + currentValue;
				currentValue = y * delay + input;

				return y;
			};

			float delay, currentValue;
		};

		InternalFilter()
		{
			reset();
		}

		void reset()
		{
			currentValue = 0.0f;
		}

		float getNextSample(float input)
		{
			float delayThisSample = minDelay;

			const float delayCoefficient = AllpassDelay::getDelayCoefficient(delayThisSample);

			allpassFilters[0].setDelay(delayCoefficient);
			allpassFilters[1].setDelay(delayCoefficient);
			allpassFilters[2].setDelay(delayCoefficient);
			allpassFilters[3].setDelay(delayCoefficient);
			allpassFilters[4].setDelay(delayCoefficient);
			allpassFilters[5].setDelay(delayCoefficient);

			float output = allpassFilters[0].getNextSample(
				allpassFilters[1].getNextSample(
					allpassFilters[2].getNextSample(
						allpassFilters[3].getNextSample(
							allpassFilters[4].getNextSample(
								allpassFilters[5].getNextSample(input + currentValue * feedback))))));

			currentValue = output;

			return input + output;
		}

		AllpassDelay allpassFilters[6];

		float minDelay, maxDelay;
		float feedback;
		float sampleRate;
		float fMin;
		float fMax;
		float rate;

		float currentValue;

	};

	InternalFilter filters[NUM_MAX_CHANNELS];
	
};

class Ladder : public MultiChannelFilter
{
public:

	enum FilterType
	{
		LP24 = 0,
		numTypes
	};

	void reset() override
	{
		memset(buf, 0, sizeof(float) * NUM_MAX_CHANNELS * 4);
		updateCoefficients();
	}

	void processSamples(AudioSampleBuffer& b, int startSample, int numSamples)
	{
		for (int c = 0; c < b.getNumChannels(); c++)
		{
			for (int i = 0; i < numSamples; i++)
			{
				float* d = b.getWritePointer(c, i + startSample);
				*d = processSample(*d, c);
			}
		}
	}

	void updateCoefficients() override
	{
		float inFreq = jlimit<float>(20.0f, 20000.0f, (float)frequency);

		const float x = 2.0f * float_Pi*inFreq / (float)sampleRate;

		cut = jlimit<float>(0.0f, 0.8f, x);
		res = jlimit<float>(0.3f, 4.0f, (float)q / 2.0f);
	}

private:

	float processSample(float input, int channel)
	{
		float* buffer = buf[channel];

		float resoclip = buffer[3];

		const float in = input - (resoclip * res);
		buffer[0] = ((in - buffer[0]) * cut) + buffer[0];
		buffer[1] = ((buffer[0] - buffer[1]) * cut) + buffer[1];
		buffer[2] = ((buffer[1] - buffer[2]) * cut) + buffer[2];
		buffer[3] = ((buffer[2] - buffer[3]) * cut) + buffer[3];
		return 2.0f * buffer[3];
	}

	float buf[NUM_MAX_CHANNELS][4];

	float cut;
	float res;
};


#define JORDAN_HARRIS_SVF 0




#if USE_STATE_VARIABLE_FILTERS

class StateVariableFilter: public MultiChannelFilter
{


#if JORDAN_HARRIS_SVF

public:

	/** The type of filter that the State Variable Filter will output. */
	enum class FilterType {
		LP = 0,
		BP,
		HP,
		SVFUnitGainBandpass,
		BPShelf,
		Notch,
		Allpass,
		Peak
	};

	StateVariableFilter()
	{
		q = 1.0;

		gCoeff = 1.0f;
		RCoeff = 1.0f;
		KCoeff = 0.0f;

		reset();

	}


	void updateCoefficients() override
	{
		// prewarp the cutoff (for bilinear-transform filters)
		float wd = static_cast<float>(frequency * 2.0f * M_PI);
		float T = 1.0f / (float)sampleRate;
		float wa = (2.0f / T) * tan(wd * T / 2.0f);

		// Calculate g (gain element of integrator)
		gCoeff = wa * T / 2.0f;			// Calculate g (gain element of integrator)

										// Calculate Zavalishin's R from Q (referred to as damping parameter)
		RCoeff = 1.0f / (2.0f * q);

		// Gain for BandShelving filter
		KCoeff = gain;

		x1 = (2.0f * RCoeff + gCoeff);
		x2 = 1.0f / (1.0f + (2.0f * RCoeff * gCoeff) + gCoeff * gCoeff);
	}

	void reset() override
	{
		memset(z1_A, 0, sizeof(float)*numChannels);
		memset(z2_A, 0, sizeof(float)*numChannels);
	}

	void processSamples(AudioSampleBuffer& buffer, int startSample, int numSamples) override
	{
		if (numChannels != buffer.getNumChannels())
		{
			setNumChannels(buffer.getNumChannels());
		}

		for (int i = 0; i < buffer.getNumChannels(); i++)
		{
			processAudioBlock(buffer.getWritePointer(i, startSample), numSamples, i);
		}
	}

private:

	

	void processAudioBlock(float* const samples, const int& numSamples,
		const int& channelIndex)
	{
		switch (type) 
		{
		case FilterType::LP:
		{
			/*
			for (int i = 0; i < numSamples; i++)
			{
				const float v0 = samples[i];
				const float v1z = z1_A[channelIndex];
				const float v2z = z2_A[channelIndex];



				float v3 = v0 + v0z[c] - 2.0f * v2z;
				z1_A[c] += g1 * v3 - g2 * v1z;
				v2[c] += g3 * v3 + g4 * v1z;
				v0z[c] = v0;
				samples[i] = v2[c];
			}
			*/

			for (int i = 0; i < numSamples; ++i)
			{
				const float v0 = samples[i];
				const float v1z = z1_A[channelIndex];
				const float v2z = z2_A[channelIndex];

				const float HP = (v0 - x1 * v1z - v2z) * x2;
				const float BP = HP * gCoeff + v1z;
				const float LP = BP * gCoeff + v2z;

				z1_A[channelIndex] = gCoeff * HP + BP;
				z2_A[channelIndex] = gCoeff * BP + LP;

				samples[i] = LP;
			}
			break;
		}
		case FilterType::BP:
		{
			for (int i = 0; i < numSamples; ++i)
			{
				const float input = samples[i];
				const float HP = (input - x1 * z1_A[channelIndex] - z2_A[channelIndex]) / x2;
				const float BP = HP * gCoeff + z1_A[channelIndex];
				const float LP = BP * gCoeff + z2_A[channelIndex];

				z1_A[channelIndex] = gCoeff * HP + BP;
				z2_A[channelIndex] = gCoeff * BP + LP;

				samples[i] = BP;
			}

			break;
		}
			
		case FilterType::HP:
		{
			for (int i = 0; i < numSamples; ++i)
			{
				const float input = samples[i];
				const float HP = (input - x1 * z1_A[channelIndex] - z2_A[channelIndex]) / x2;
				const float BP = HP * gCoeff + z1_A[channelIndex];
				const float LP = BP * gCoeff + z2_A[channelIndex];

				z1_A[channelIndex] = gCoeff * HP + BP;
				z2_A[channelIndex] = gCoeff * BP + LP;

				samples[i] = HP;
			}

			
			break;
		}
			
		case FilterType::SVFUnitGainBandpass:
		{
			for (int i = 0; i < numSamples; ++i)
			{
				const float input = samples[i];
				const float HP = (input - x1 * z1_A[channelIndex] - z2_A[channelIndex]) / x2;
				const float BP = HP * gCoeff + z1_A[channelIndex];
				const float LP = BP * gCoeff + z2_A[channelIndex];
				const float UBP = 2.0f * RCoeff * BP;

				z1_A[channelIndex] = gCoeff * HP + BP;
				z2_A[channelIndex] = gCoeff * BP + LP;

				samples[i] = UBP;
			}

			break;
		}
			
		case FilterType::BPShelf:
		{
			for (int i = 0; i < numSamples; ++i)
			{
				const float input = samples[i];
				const float HP = (input - x1 * z1_A[channelIndex] - z2_A[channelIndex]) / x2;
				const float BP = HP * gCoeff + z1_A[channelIndex];
				const float LP = BP * gCoeff + z2_A[channelIndex];
				const float UBP = 2.0f * RCoeff * BP;

				z1_A[channelIndex] = gCoeff * HP + BP;
				z2_A[channelIndex] = gCoeff * BP + LP;

				const float BShelf = input + UBP * KCoeff;
				samples[i] = BShelf;
			}

			
			break;
		}
		case FilterType::Notch:
		{
			for (int i = 0; i < numSamples; ++i)
			{
				const float input = samples[i];
				const float HP = (input - x1 * z1_A[channelIndex] - z2_A[channelIndex]) / x2;
				const float BP = HP * gCoeff + z1_A[channelIndex];
				const float LP = BP * gCoeff + z2_A[channelIndex];
				const float UBP = 2.0f * RCoeff * BP;

				z1_A[channelIndex] = gCoeff * HP + BP;
				z2_A[channelIndex] = gCoeff * BP + LP;

				const float Notch = input - UBP;
				samples[i] = Notch;
			}

			break;
		}

		case FilterType::Allpass:
		{
			for (int i = 0; i < numSamples; ++i)
			{
				const float input = samples[i];
				const float HP = (input - x1 * z1_A[channelIndex] - z2_A[channelIndex]) / x2;
				const float BP = HP * gCoeff + z1_A[channelIndex];
				const float LP = BP * gCoeff + z2_A[channelIndex];
				
				z1_A[channelIndex] = gCoeff * HP + BP;
				z2_A[channelIndex] = gCoeff * BP + LP;

				const float AP = input - (4.0f * RCoeff * BP);
				samples[i] = AP;
			}

			break;
		}

		case FilterType::Peak:
		{
			for (int i = 0; i < numSamples; ++i)
			{
				const float input = samples[i];
				const float HP = (input - x1 * z1_A[channelIndex] - z2_A[channelIndex]) / x2;
				const float BP = HP * gCoeff + z1_A[channelIndex];
				const float LP = BP * gCoeff + z2_A[channelIndex];

				z1_A[channelIndex] = gCoeff * HP + BP;
				z2_A[channelIndex] = gCoeff * BP + LP;

				const float Peak = LP - HP;
				samples[i] = Peak;
			}

			break;
		}

		default:
			FloatVectorOperations::clear(samples, numSamples);
			break;
		}


		
	}

	//	Parameters:
	
	

	bool active = true;	// is the filter processing or not

						//	Coefficients:
	float gCoeff;		// gain element 
	float RCoeff;		// feedback damping element
	float KCoeff;		// shelf gain element

	float x1;
	float x2;

	float z1_A[NUM_MAX_CHANNELS];
	float z2_A[NUM_MAX_CHANNELS];		// state variables (z^-1)

	

#else


public:



	enum FilterType
	{
		LP = 0,
		HP,
		BP,
		NOTCH,
		ALLPASS,
		numTypes
	};

	StateVariableFilter()
	{
		memset(v0z, 0, sizeof(float)*NUM_MAX_CHANNELS);
		memset(z1_A, 0, sizeof(float)*NUM_MAX_CHANNELS);
		memset(v2, 0, sizeof(float)*NUM_MAX_CHANNELS);
	}

	void reset() 
	{
		memset(v0z, 0, sizeof(float)*numChannels);
		memset(z1_A, 0, sizeof(float)*numChannels);
		memset(v2, 0, sizeof(float)*numChannels);
	};

	void updateCoefficients() override
	{
		const float scaledQ = jlimit<float>(0.0f, 9.999f, (float)q * 0.1f);

		if (type == FilterType::ALLPASS)
		{
			// prewarp the cutoff (for bilinear-transform filters)
			float wd = static_cast<float>(frequency * 2.0f * float_Pi);
			float T = 1.0f / (float)sampleRate;
			float wa = (2.0f / T) * tan(wd * T / 2.0f);

			// Calculate g (gain element of integrator)
			gCoeff = wa * T / 2.0f;			// Calculate g (gain element of integrator)

											// Calculate Zavalishin's R from Q (referred to as damping parameter)
			RCoeff = 1.0f / (2.0f * (float)q);

			x1 = (2.0f * RCoeff + gCoeff);
			x2 = 1.0f / (1.0f + (2.0f * RCoeff * gCoeff) + gCoeff * gCoeff);
		}
		else
		{
			float g = (float)tan(double_Pi * frequency / sampleRate);
			//float damping = 1.0f / res;
			//k = damping;
			k = 1.0f - 0.99f * scaledQ;
			float ginv = g / (1.0f + g * (g + k));
			g1 = ginv;
			g2 = 2.0f * (g + k) * ginv;
			g3 = g * ginv;
			g4 = 2.0f * ginv;
		}

		
		
	}

	void processSamples(AudioSampleBuffer& buffer, int startSample, int numSamples) override
	{
		if (numChannels != buffer.getNumChannels())
		{
			setNumChannels(buffer.getNumChannels());
		}

		switch (type)
		{
		case LP:
		{
			for (int c = 0; c < numChannels; c++)
			{
				float* d = buffer.getWritePointer(c, startSample);

				for (int i = 0; i < numSamples; i++)
				{
					float v0 = d[i];
					float v1z = z1_A[c];
					float v2z = v2[c];
					float v3 = v0 + v0z[c] - 2.0f * v2z;
					z1_A[c] += g1 * v3 - g2 * v1z;
					v2[c] += g3 * v3 + g4 * v1z;
					v0z[c] = v0;
					d[i] = v2[c];
				}
			}

			break;
		}
		case BP:
		{
			for (int c = 0; c < numChannels; c++)
			{
				float* d = buffer.getWritePointer(c, startSample);

				for (int i = 0; i < numSamples; i++)
				{
					float v0 = d[i];
					float v1z = z1_A[c];
					float v2z = v2[c];
					float v3 = v0 + v0z[c] - 2.0f * v2z;
					z1_A[c] += g1 * v3 - g2 * v1z;
					v2[c] += g3 * v3 + g4 * v1z;
					v0z[c] = v0;
					d[i] = z1_A[c];
				}
			}

			break;
		}

		case HP:
		{
			for (int c = 0; c < numChannels; c++)
			{
				float* d = buffer.getWritePointer(c, startSample);

				for (int i = 0; i < numSamples; i++)
				{
					float v0 = d[i];
					float v1z = z1_A[c];
					float v2z = v2[c];
					float v3 = v0 + v0z[c] - 2.0f * v2z;
					z1_A[c] += g1 * v3 - g2 * v1z;
					v2[c] += g3 * v3 + g4 * v1z;
					v0z[c] = v0;
					d[i] = v0 - k * z1_A[c] - v2[c];
				}
			}

			break;
		}
		case FilterType::ALLPASS:
		{
			for (int c = 0; c < numChannels; c++)
			{
				float* d = buffer.getWritePointer(c, startSample);

				for (int i = 0; i < numSamples; ++i)
				{
					const float input = d[i];
					const float HP = (input - x1 * z1_A[c] - v2[c]) / x2;
					const float BP = HP * gCoeff + z1_A[c];
					const float LP = BP * gCoeff + v2[c];

					z1_A[c] = gCoeff * HP + BP;
					v2[c] = gCoeff * BP + LP;

					const float AP = input - (4.0f * RCoeff * BP);
					d[i] = AP;
				}
			}
			

			

			break;
		}

		case NOTCH:
		{
			for (int c = 0; c < numChannels; c++)
			{
				float* d = buffer.getWritePointer(c, startSample);

				for (int i = 0; i < numSamples; i++)
				{
					float v0 = d[i];
					float v1z = z1_A[c];
					float v2z = v2[c];
					float v3 = v0 + v0z[c] - 2.0f * v2z;
					z1_A[c] += g1 * v3 - g2 * v1z;
					v2[c] += g3 * v3 + g4 * v1z;
					v0z[c] = v0;
					d[i] = v0 - k * z1_A[c];
				}
			}

			break;
		}
		}
	}

private:
	
	float v0z[NUM_MAX_CHANNELS];
	float z1_A[NUM_MAX_CHANNELS];
	float v2[NUM_MAX_CHANNELS];

	float k, g1, g2, g3, g4, x1, x2, gCoeff, RCoeff;

#endif

};

#endif

class FilterEffect
{
public:
    
    
    void setRenderQuality(int powerOfTwo)
    {
        if(powerOfTwo != 0 && isPowerOfTwo(powerOfTwo))
        {
            quality = powerOfTwo;
        }
        
    }
    
    int getSampleAmountForRenderQuality() const
    {
        return quality;
    }
    
    virtual ~FilterEffect() {};

	virtual IIRCoefficients getCurrentCoefficients() const = 0;

protected:

    int quality;

};

class MonoFilterEffect: public MonophonicEffectProcessor,
						public FilterEffect
{
public:

	SET_PROCESSOR_NAME("MonophonicFilter", "Monophonic Filter");

	enum InternalChains
	{
		FrequencyChain = 0,
		GainChain,
		BipolarFrequencyChain,
		numInternalChains
	};

	enum EditorStates
	{
		FrequencyChainShown = Processor::numEditorStates,
		GainChainShown,
		BipolarFrequencyChainShown,
		numEditorStates
	};

	enum Parameters
	{
		Gain = 0,
		Frequency,
		Q,
		Mode,
        Quality,
		BipolarIntensity,
		numEffectParameters
	};

	// Remember to keep the ScriptingObjects::ScriptingEffect::FilterListObject 
	enum FilterMode
	{
		LowPass = 0,
		HighPass,
		LowShelf,
		HighShelf,
		Peak,
		ResoLow,
		StateVariableLP,
		StateVariableHP,
		MoogLP,
		OnePoleLowPass,
		OnePoleHighPass,
		StateVariablePeak,
		StateVariableNotch,
		StateVariableBandPass,
		Allpass,
		LadderFourPoleLP,
		LadderFourPoleHP,
		RingMod,
		numFilterModes
	};

	MonoFilterEffect(MainController *mc, const String &id);;

	void setUseInternalChains(bool shouldBeUsed) { useInternalChains = shouldBeUsed; };
	void setUseFixedFrequency(bool shouldUseFixedFrequency) { useFixedFrequency = shouldUseFixedFrequency; }

	float getAttribute(int parameterIndex) const override;;
	void setInternalAttribute(int parameterIndex, float newValue) override;;
	float getDefaultValue(int parameterIndex) const override;

	void restoreFromValueTree(const ValueTree &v) override;;
	ValueTree exportAsValueTree() const override;

	void prepareToPlay(double sampleRate, int samplesPerBlock) override;;
	void applyEffect(AudioSampleBuffer &buffer, int startSample, int numSamples) override;
    
    void processBlockPartial(AudioSampleBuffer &buffer, int startSample, int numSamples);
	
	bool hasTail() const override {return false; };

	int getNumInternalChains() const override { return numInternalChains; };
	int getNumChildProcessors() const override { return numInternalChains; };
	Processor *getChildProcessor(int processorIndex) override;;
	const Processor *getChildProcessor(int processorIndex) const override;;
	AudioSampleBuffer &getBufferForChain(int chainIndex);;

	ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor)  override;
	
	static IIRCoefficients makeResoLowPass(double sampleRate, double cutoff, double q);;

	IIRCoefficients getCurrentCoefficients() const override
	{
		switch (mode)
		{
		case MonoFilterEffect::OnePoleLowPass:  return IIRCoefficients::makeLowPass(getSampleRate(), currentFreq);
		case MonoFilterEffect::OnePoleHighPass:  return IIRCoefficients::makeHighPass(getSampleRate(), currentFreq);
		case MonoFilterEffect::LowPass:			return IIRCoefficients::makeLowPass(getSampleRate(), currentFreq);
		case MonoFilterEffect::HighPass:		return IIRCoefficients::makeHighPass(getSampleRate(), currentFreq, q);
		case MonoFilterEffect::LowShelf:		return IIRCoefficients::makeLowShelf(getSampleRate(), currentFreq, q, currentGain);
		case MonoFilterEffect::HighShelf:		return IIRCoefficients::makeHighShelf(getSampleRate(), currentFreq, q, currentGain);
		case MonoFilterEffect::Peak:			return IIRCoefficients::makePeakFilter(getSampleRate(), currentFreq, q, currentGain);
		case MonoFilterEffect::ResoLow:			return makeResoLowPass(getSampleRate(), currentFreq, q);
		case MonoFilterEffect::StateVariableLP: return makeResoLowPass(getSampleRate(), currentFreq, q);
		case MonoFilterEffect::StateVariableHP: return IIRCoefficients::makeHighPass(getSampleRate(), currentFreq, q);
		case MonoFilterEffect::LadderFourPoleLP: return makeResoLowPass(getSampleRate(), currentFreq, 2.0*q);
		case MonoFilterEffect::LadderFourPoleHP: return IIRCoefficients::makeHighPass(getSampleRate(), currentFreq, 2.0*q);
		case MonoFilterEffect::MoogLP:			return makeResoLowPass(getSampleRate(), currentFreq, q);
        case MonoFilterEffect::StateVariablePeak:       return IIRCoefficients::makePeakFilter(getSampleRate(), currentFreq, q, currentGain);
        case MonoFilterEffect::StateVariableNotch:      return IIRCoefficients::makeNotchFilter(getSampleRate(), currentFreq, q);
        case MonoFilterEffect::StateVariableBandPass:   return IIRCoefficients::makeBandPass(getSampleRate(), currentFreq, q);
        case MonoFilterEffect::Allpass:                 return IIRCoefficients::makeAllPass(getSampleRate(), currentFreq, q);
        case MonoFilterEffect::RingMod:                 return IIRCoefficients::makeAllPass(getSampleRate(), currentFreq, q);
		default:								return IIRCoefficients();
		}
	}

private:

	void setMode(int filterMode);

	void calcCoefficients();

	bool useInternalChains;
	bool useFixedFrequency;

	ScopedPointer<ModulatorChain> freqChain;
	ScopedPointer<ModulatorChain> gainChain;
	ScopedPointer<ModulatorChain> bipolarFreqChain;

	AudioSampleBuffer freqBuffer;
	AudioSampleBuffer gainBuffer;
	AudioSampleBuffer bipolarFreqBuffer;

	friend class PolyFilterEffect;
	friend class HarmonicFilter;
	friend class HarmonicMonophonicFilter;

	bool changeFlag;

	bool calculateGainModValue = false;

	double currentFreq;
	float currentGain;

	double freq;
	double q;
	float gain;
	FilterMode mode;

	bool useBipolarIntensity = false;
	float bipolarIntensity = 0.0f;

	StaticBiquad staticBiquadFilter;
	MoogFilter moogFilter;
	StateVariableFilter stateFilter;
	SimpleOnePole simpleFilter;
	Ladder ladderFilter;
	PhaseAllpass allpassFilter;
	RingmodFilter ringModFilter;

	MultiChannelFilter* currentFilter = nullptr;

	double lastSampleRate = 0.0;

};


class PolyFilterEffect: public VoiceEffectProcessor,
						public FilterEffect
{
public:

	SET_PROCESSOR_NAME("PolyphonicFilter", "Polyphonic Filter");

	enum InternalChains
	{
		FrequencyChain = 0,
		GainChain,
		BipolarFrequencyChain,
		numInternalChains
	};

	enum EditorStates
	{
		FrequencyChainShown = Processor::numEditorStates,
		GainChainShown,
		BipolarFreqChainShown,
		numEditorStates
	};

	PolyFilterEffect(MainController *mc, const String &uid, int numVoices);;

	float getAttribute(int parameterIndex) const override;;
	void setInternalAttribute(int parameterIndex, float newValue) override;;

	void restoreFromValueTree(const ValueTree &v) override;;
	ValueTree exportAsValueTree() const override;

	int getNumInternalChains() const override { return numInternalChains; };
	int getNumChildProcessors() const override { return numInternalChains; };
	Processor *getChildProcessor(int processorIndex) override;;
	const Processor *getChildProcessor(int processorIndex) const override;;
	AudioSampleBuffer & getBufferForChain(int index);;

	void prepareToPlay(double sampleRate, int samplesPerBlock) override;;
	void renderNextBlock(AudioSampleBuffer &/*b*/, int /*startSample*/, int /*numSample*/) { }
	/** Calculates the frequency chain and sets the q to the current value. */
	void preVoiceRendering(int voiceIndex, int startSample, int numSamples);
	void applyEffect(int voiceIndex, AudioSampleBuffer &b, int startSample, int numSamples) override;
	/** Resets the filter state if a new voice is started. */
	void startVoice(int voiceIndex, int noteNumber) override;
	bool hasTail() const override { return true; };
	
	ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor)  override;

	IIRCoefficients getCurrentCoefficients() const override {return voiceFilters[0]->getCurrentCoefficients();};

private:

	friend class HarmonicFilter;

	bool changeFlag;

	double currentFreq;
	double freq;
	double q;
	float gain;
	float currentGain;
	float bipolarIntensity = 0.0f;

	MonoFilterEffect::FilterMode mode;

	OwnedArray<MonoFilterEffect> voiceFilters;

	ScopedPointer<ModulatorChain> freqChain;
	ScopedPointer<ModulatorChain> gainChain;
	ScopedPointer<ModulatorChain> bipolarFreqChain;

	AudioSampleBuffer timeVariantFreqModulatorBuffer;
	AudioSampleBuffer timeVariantGainModulatorBuffer;
	AudioSampleBuffer timeVariantBipolarFreqModulatorBuffer;

};




} // namespace hise


#endif  // FILTERS_H_INCLUDED
