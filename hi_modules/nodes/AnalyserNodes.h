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

#pragma once

namespace scriptnode {
using namespace juce;
using namespace hise;

namespace analyse
{

struct Helpers
{
	struct AnalyserDataProvider
	{
		virtual ~AnalyserDataProvider() {};

		virtual double getSampleRate() const = 0;

		virtual AnalyserRingBuffer& getRingBuffer() = 0;
	};

	struct Base
	{
		Base(AnalyserDataProvider& b) :
			srp(b)
		{}

		Colour getColourBase(int colourId)
		{
			if (colourId == AudioAnalyserComponent::ColourId::bgColour)
				return Colour(0xFF333333);
			if (colourId == AudioAnalyserComponent::ColourId::fillColour)
				return Colours::white.withAlpha(0.7f);
			if (colourId == AudioAnalyserComponent::ColourId::lineColour)
				return Colours::white;

			return Colours::transparentBlack;
		}

		AnalyserDataProvider& srp;
	};

	struct FFT
	{
		SET_HISE_NODE_ID("fft");

		static constexpr int width = 512;
		static constexpr int height = 128;

		struct Display : public Base,
			public FFTDisplayBase,
			public Component,
			public Timer
		{
			Display(AnalyserDataProvider& sr) :
				Base(sr),
				FFTDisplayBase(sr.getRingBuffer())
			{
				sr.getRingBuffer().setAnalyserBufferSize(16384);
				startTimer(30);
				setSize(width, height);
			}

			double getSamplerate() const override
			{
				return srp.getSampleRate();
			}

			Colour getColourForAnalyserBase(int colourId)
			{
				return getColourBase(colourId);
			}

			void timerCallback() override { repaint(); }

			void paint(Graphics& g) override
			{
				FFTDisplayBase::drawSpectrum(g);
			}
		};

		static Component* createComponent(AnalyserDataProvider& p)
		{
			return new Display(p);
		}
	};

	struct Oscilloscope
	{
		SET_HISE_NODE_ID("oscilloscope");

		static constexpr int width = 512;
		static constexpr int height = 128;

		struct Display : public Base,
			public OscilloscopeBase,
			public Component,
			public Timer
		{
			Display(AnalyserDataProvider& sr) :
				Base(sr),
				OscilloscopeBase(sr.getRingBuffer())
			{
				sr.getRingBuffer().setAnalyserBufferSize(2048);
				startTimer(30);
				setSize(width, height);
			}

			Colour getColourForAnalyserBase(int colourId)
			{
				return getColourBase(colourId);
			}

			void timerCallback() override { repaint(); }

			void paint(Graphics& g) override
			{
				OscilloscopeBase::drawWaveform(g);
			}
		};

		static Component* createComponent(AnalyserDataProvider& p)
		{
			return new Display(p);
		}
	};

};

template <class T> class analyse_base : public HiseDspBase,
					 public Helpers::AnalyserDataProvider
{
public:

	SET_HISE_NODE_ID(T::getStaticId());
	SET_HISE_NODE_EXTRA_WIDTH(T::width);
	SET_HISE_NODE_EXTRA_HEIGHT(T::height);
	SET_HISE_NODE_IS_MODULATION_SOURCE(false);

	HISE_EMPTY_MOD;
	HISE_EMPTY_CREATE_PARAM;
	
	AnalyserRingBuffer& getRingBuffer() override
	{
		return buffer;
	}

	double getSampleRate() const override
	{
		return sr;
	}

	void prepare(PrepareSpecs  ps)
	{
		sr = ps.sampleRate;
	}

	void initialise(NodeBase* n)
	{

	}

	Component* createExtraComponent(PooledUIUpdater* updater) override
	{
		return T::createComponent(*this);
	}

	void reset()
	{
		buffer.internalBuffer.clear();
	}

	void process(ProcessData& d)
	{
		AudioSampleBuffer b(d.data, d.numChannels, d.size);
		buffer.pushSamples(b, 0, d.size);
	}

	void processSingle(float* frameData, int numChannels)
	{
		AudioSampleBuffer b(&frameData, numChannels, 1);
		buffer.pushSamples(b, 0, 1);
	}

	double sr = 0.0;
	AnalyserRingBuffer buffer;
};

template class analyse_base<Helpers::FFT>;
using fft = analyse_base<Helpers::FFT>;

template class analyse_base<Helpers::Oscilloscope>;
using oscilloscope = analyse_base<Helpers::Oscilloscope>;


}

}
