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

namespace hise { using namespace juce;

using namespace snex;
using namespace snex::Types;

/** This class contains different EnvelopeFollower algorithm as subclasses and some helper methods for preparing the in / output.
*/
class EnvelopeFollower
{
public:

	/** This prepares the input (normalises the input using 'max' and return the absolute value. */
	static inline float prepareAudioInput(float input, float max) { return fabs(input * 1.0f / max); }

	/** This makes sure that the value does not exceed the 0.0 ... 1.0 limits. */
	static inline float constrainTo0To1(float input) { return jlimit<float>(0.0f, 1.0f, input); };

	/** This envelope following algorithm stores values in a temporary buffer and ramps between the magnitudes.
	*
	*	The output is the nicest one (almost no ripple), but you will get a latency of the ramp length
	*/
	class MagnitudeRamp
	{
	public:

		MagnitudeRamp();;

		/** Set the length of the ramp (also the size of the temporary buffer and thus the delay of the ramping). */
		void setRampLength(int newRampLength);;

		/** Returns the calculated value. */
		float getEnvelopeValue(float inputValue);;

		/** The size of the buffer */
		int size;

	private:

		AudioSampleBuffer rampBuffer;
		int indexInBufferedArray;
		float currentPeak;
		float rampedValue;
		Ramper bufferRamper;
	};

	/** This algorithm uses two different times for attack and decay. */
	class AttackRelease
	{
	public:

		/** Creates a new envelope follower using the supplied parameters.
		*
		*	You have to call setSampleRate before you can use it.
		*/
		AttackRelease(float attackTime=20.0, float releaseTime=50.0);;

		/** Returns the envelope value. */
		float calculateValue(float input);;

		/** You have to call this before any call to calculateValue. */
		void setSampleRate(double sampleRate_);

		void setAttackDouble(double newAttack)
		{
			setAttack((float)newAttack);
		}

		void setReleaseDouble(double newRelease)
		{
			setRelease((float)newRelease);
		}

		void reset()
		{
			lastValue = 0.0;
		}

		void setAttack(float newAttack)
		{
			attack = newAttack;

			if (attack == 0.0f)
				attackCoefficient = 0.0;

			else
				calculateCoefficients();
		};

		void setRelease(float newRelease);;

		float getAttack() const noexcept { return (float)attack; };
		float getRelease() const noexcept { return (float)release; };

		double getSampleRate() const { return sampleRate; }

	private:

		void calculateCoefficients();

		float attack, release;

		double sampleRate;
		double attackCoefficient, releaseCoefficient;
		double lastValue;
	};

};


template <int NumChannels> struct InterpolatingDelay
{
	// This is the max buffer size. Making it a power of two
	// lets the compiler optimize the modulo operation of the
	// index wrapping to a simple bitwise AND op.
	static const int MaxBufferSize = 32768;

	// We'll use an interleaved buffer to avoid jumping around
	// in the memory too much. The interpolator class supports operation
	// with a span as data type.
	using BufferType = span<span<float, NumChannels>, MaxBufferSize>;

	// The write access will be wrapped around the buffer size
	using WriterType = index::wrapped<MaxBufferSize, true>;

	// The read access will use linear interpolation
	using ReaderType = index::lerp<index::unscaled<double, WriterType>>;

	void reset()
	{
		// clear the entire buffer
		buffer.clear();

		// stop the smoothing
		smoothedDelayTime.reset();
	}

	void prepare(PrepareSpecs ps)
	{
		samplerate = ps.sampleRate;
		refreshFadeTime();
		refreshDelayTime();
	}

	void setDelayTime(double delayInMilliseconds)
	{
		delayMs = delayInMilliseconds;
		refreshDelayTime();
	}

	void setDelayFadeTime(double fadeTimeMilliseconds)
	{
		fadeMs = fadeTimeMilliseconds;
		refreshFadeTime();
	}

	void processFrame(span<float, NumChannels>& data)
	{
		// Copy the frame to the writer position
		buffer[writer] = data;

		// Calculate the read index using the smoothed delay time
		const double readIndex = (double)(int)(writer) + smoothedDelayTime.advance();

		// bump the writer by a sample
		++writer;

		// assign the read index to the reader.
		reader = readIndex;

		// Interpolate the reader
		data = buffer.interpolate(reader);
	}

	void process(ProcessData<NumChannels>& data)
	{
		auto fd = data.toFrameData();

		while (fd.next())
			processFrame(fd.toSpan());
	}

private:

	void refreshDelayTime()
	{
		auto delayTimeSamples = delayMs * 0.001 * samplerate;
		smoothedDelayTime.set(delayTimeSamples);
	}

	void refreshFadeTime()
	{
		smoothedDelayTime.prepare(samplerate, fadeMs);
	}

	double fadeMs = 20.0;
	double delayMs = 0.0;
	double samplerate = 0.0;

	BufferType buffer;

	ReaderType reader;
	WriterType writer;

	sdouble smoothedDelayTime;

	hmath Math;
};

} 
