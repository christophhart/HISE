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

namespace hise {
using namespace juce;


hise::MoogFilterSubType::MoogFilterSubType()
{
	in1 = data;
	in2 = in1 + NUM_MAX_CHANNELS;
	in3 = in2 + NUM_MAX_CHANNELS;
	in4 = in3 + NUM_MAX_CHANNELS;
	out1 = in4 + NUM_MAX_CHANNELS;
	out2 = out1 + NUM_MAX_CHANNELS;
	out3 = out2 + NUM_MAX_CHANNELS;
	out4 = out3 + NUM_MAX_CHANNELS;

	reset(NUM_MAX_CHANNELS);

	updateCoefficients(44100.0, 20000.0, 1.0, 0.0);
}


void MoogFilterSubType::updateCoefficients(double sampleRate, double frequency, double q, double /*gain*/)
{
	auto lFrequency = FilterLimits::limitFrequency(frequency);

	fc = lFrequency / (0.5 *sampleRate);
	res = q / 2.0;
	if (res > 4.0) res = 4.0;
	f = fc * 1.16;
	fss = (f * f) * (f * f);
	invF = 1.0 - f;
	fb = res * (1.0 - 0.15 * f * f);
}

void MoogFilterSubType::processSamples(AudioSampleBuffer& buffer, int startSample, int numSamples)
{
	for (int c = 0; c < buffer.getNumChannels(); c++)
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

void MoogFilterSubType::processSingle(float* frameData, int numChannels)
{
	for (int c = 0; c < numChannels; c++)
	{
		double input = (double)frameData[c];

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
		frameData[c] = 2.0f * (float)out4[c];
	}
}

void SimpleOnePoleSubType::processSamples(AudioSampleBuffer& buffer, int startSample, int numSamples)
{
	lastChannelAmount = buffer.getNumChannels();

	switch (onePoleType)
	{
	case FilterType::HP:
	{
		for (int c = 0; c < lastChannelAmount; c++)
		{
			float *d = buffer.getWritePointer(c, startSample);

			for (int i = 0; i < numSamples; i++)
			{
				const float tmp = a0 * d[i] - b1 * lastValues[c];
				lastValues[c] = tmp;
				d[i] = d[i] - tmp;

			}
		}

		break;
	}
	case FilterType::LP:
	{
		for (int c = 0; c < lastChannelAmount; c++)
		{
			float *d = buffer.getWritePointer(c, startSample);

			for (int i = 0; i < numSamples; i++)
			{
				d[i] = a0 * d[i] - b1 * lastValues[c];
				lastValues[c] = d[i];
			}
		}

		break;
	}
    default:
        jassertfalse;
        break;
	}
}

void SimpleOnePoleSubType::processSingle(float* d, int numChannels)
{
	switch (onePoleType)
	{
	case FilterType::HP:
	{
		for (int c = 0; c < numChannels; c++)
		{
			const float tmp = a0 * d[c] - b1 * lastValues[c];
			lastValues[c] = tmp;
			d[c] = d[c] - tmp;
		}

		break;
	}
	case FilterType::LP:
	{
		for (int c = 0; c < numChannels; c++)
		{
			d[c] = a0 * d[c] - b1 * lastValues[c];
			lastValues[c] = d[c];
		}

		break;
	}
	default:
		jassertfalse;
		break;
	}
}

void RingmodFilterSubType::updateCoefficients(double sampleRate, double frequency, double q, double /*gain*/)
{
	uptimeDelta = frequency / sampleRate * 2.0 * double_Pi;
	oscGain = jmap<float>((float)q, 0.3f, 9.9f, 0.0f, 1.0f);
	oscGain = jlimit<float>(0.0f, 1.0f, oscGain);
}

void RingmodFilterSubType::processSamples(AudioSampleBuffer& buffer, int startSample, int numSamples)
{
	const float invGain = 1.0f - oscGain;

	int numChannels = buffer.getNumChannels();

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

void RingmodFilterSubType::processSingle(float* d, int numChannels)
{
	const float invGain = 1.0f - oscGain;
	const float oscValue = oscGain * (float)std::sin(uptime);;

	for (int c = 0; c < numChannels; c++)
	{
		const float input = d[c];
		d[c] = invGain * input + input * oscValue;
	}

	uptime += uptimeDelta;
}

void StaticBiquadSubType::reset(int numNewChannels)
{
	numChannels = numNewChannels;

	for (int i = 0; i < numChannels; i++)
	{
		filters[i].reset();
	}
}

void StaticBiquadSubType::processSamples(AudioSampleBuffer& b, int startSample, int numSamples)
{
	int channelAmount = b.getNumChannels();

	for (int i = 0; i < channelAmount; i++)
	{
		float* d = b.getWritePointer(i, startSample);
		filters[i].processSamples(d, numSamples);
	}
}

void StaticBiquadSubType::processSingle(float* d, int numChannels)
{
	for (int i = 0; i < numChannels; i++)
	{
		d[i] = filters[i].processSingleSampleRaw(d[i]);
	}
}

void PhaseAllpassSubType::processSamples(AudioSampleBuffer& b, int startSample, int numSamples)
{
	for (int c = 0; c < numFilters; c++)
	{
		for (int i = 0; i < numSamples; i++)
		{
			float* d = b.getWritePointer(c, i + startSample);
			*d = filters[c].getNextSample(*d);
		}
	}
}

void PhaseAllpassSubType::processSingle(float* d, int numChannels)
{
	for (int i = 0; i < numChannels; i++)
		d[i] = filters[i].getNextSample(d[i]);
}

void PhaseAllpassSubType::updateCoefficients(double sampleRate, double frequency, double q, double /*gain*/)
{
	if (sampleRate <= 0.0f)
	{
		return;
	}

	for (int i = 0; i < numFilters; i++)
	{
		filters[i].fMin = (float)frequency;
		filters[i].minDelay = (float)(frequency / (sampleRate / 2.0));
		filters[i].feedback = (float)jmap<double>(q, 0.3, 9.9, 0.0, 0.99);
	}
}

float PhaseAllpassSubType::InternalFilter::getNextSample(float input)
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

void LadderSubType::processSamples(AudioSampleBuffer& b, int startSample, int numSamples)
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

void LadderSubType::processSingle(float* d, int numChannels)
{
	for (int c = 0; c < numChannels; c++)
	{
		d[c] = processSample(d[c], c);
	}
}

void LadderSubType::updateCoefficients(double sampleRate, double frequency, double q, double /*gain*/)
{
	float inFreq = (float)FilterLimits::limitFrequency(frequency);

	const float x = 2.0f * float_Pi*inFreq / (float)sampleRate;

	cut = jlimit<float>(0.0f, 0.8f, x);
	res = jlimit<float>(0.3f, 4.0f, (float)q / 2.0f);
}

float LadderSubType::processSample(float input, int channel)
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

void StateVariableFilterSubType::updateCoefficients(double sampleRate, double frequency, double q, double /*gain*/)
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

void StateVariableFilterSubType::processSamples(AudioSampleBuffer& buffer, int startSample, int numSamples)
{
	auto numChannels = buffer.getNumChannels();

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
        default:
            jassertfalse;
            break;
	}
}

void StateVariableFilterSubType::processSingle(float* d, int numChannels)
{
	switch (type)
	{
	case LP:
	{
		for (int c = 0; c < numChannels; c++)
		{
			float v0 = d[c];
			float v1z = z1_A[c];
			float v2z = v2[c];
			float v3 = v0 + v0z[c] - 2.0f * v2z;
			z1_A[c] += g1 * v3 - g2 * v1z;
			v2[c] += g3 * v3 + g4 * v1z;
			v0z[c] = v0;
			d[c] = v2[c];
		}

		break;
	}
	case BP:
	{
		for (int c = 0; c < numChannels; c++)
		{
			float v0 = d[c];
			float v1z = z1_A[c];
			float v2z = v2[c];
			float v3 = v0 + v0z[c] - 2.0f * v2z;
			z1_A[c] += g1 * v3 - g2 * v1z;
			v2[c] += g3 * v3 + g4 * v1z;
			v0z[c] = v0;
			d[c] = z1_A[c];
		}

		break;
	}

	case HP:
	{
		for (int c = 0; c < numChannels; c++)
		{
			float v0 = d[c];
			float v1z = z1_A[c];
			float v2z = v2[c];
			float v3 = v0 + v0z[c] - 2.0f * v2z;
			z1_A[c] += g1 * v3 - g2 * v1z;
			v2[c] += g3 * v3 + g4 * v1z;
			v0z[c] = v0;
			d[c] = v0 - k * z1_A[c] - v2[c];
		}

		break;
	}
	case FilterType::ALLPASS:
	{
		for (int c = 0; c < numChannels; c++)
		{
			const float input = d[c];
			const float HP = (input - x1 * z1_A[c] - v2[c]) / x2;
			const float BP = HP * gCoeff + z1_A[c];
			const float LP = BP * gCoeff + v2[c];

			z1_A[c] = gCoeff * HP + BP;
			v2[c] = gCoeff * BP + LP;

			const float AP = input - (4.0f * RCoeff * BP);
			d[c] = AP;
		}
		break;
	}

	case NOTCH:
	{
		for (int c = 0; c < numChannels; c++)
		{
			float v0 = d[c];
			float v1z = z1_A[c];
			float v2z = v2[c];
			float v3 = v0 + v0z[c] - 2.0f * v2z;
			z1_A[c] += g1 * v3 - g2 * v1z;
			v2[c] += g3 * v3 + g4 * v1z;
			v0z[c] = v0;
			d[c] = v0 - k * z1_A[c];
		}

		break;
	}
	default:
		jassertfalse;
		break;
	}
}

}
