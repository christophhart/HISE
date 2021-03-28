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
		AnalyserDataProvider()
		{
			rb = new SimpleRingBuffer();
		}

		SimpleRingBuffer::Ptr rb;

		SimpleRingBuffer::Ptr getRingBuffer() { return rb; }

		virtual ~AnalyserDataProvider() {};

		void prepare(PrepareSpecs ps)
		{
			sr = ps.sampleRate;

			if (rb != nullptr)
			{
				auto numSamples = rb->getReadBuffer().getNumSamples();
				rb->setRingBufferSize(ps.numChannels, numSamples);
			}
		}

		double getSampleRate() const { return sr; }

		double sr = 0.0;

		JUCE_DECLARE_WEAK_REFERENCEABLE(AnalyserDataProvider);
	};

	static Colour getColourBase(int colourId)
	{
		if (colourId == AudioAnalyserComponent::ColourId::bgColour)
			return Colour(0xFF333333);
		if (colourId == AudioAnalyserComponent::ColourId::fillColour)
			return Colours::white.withAlpha(0.7f);
		if (colourId == AudioAnalyserComponent::ColourId::lineColour)
			return Colours::white;

		return Colours::transparentBlack;
	}

	struct FFT
	{
		SET_HISE_NODE_ID("fft");
	};

	struct Oscilloscope
	{
		SET_HISE_NODE_ID("oscilloscope");
	};

	struct GonioMeter
	{
		SET_HISE_NODE_ID("goniometer");
	};

};

template <class T> class analyse_base : public HiseDspBase,
										public Helpers::AnalyserDataProvider
{
public:

	SET_HISE_NODE_ID(T::getStaticId());
    SN_GET_SELF_AS_OBJECT(analyse_base);
    
	HISE_EMPTY_CREATE_PARAM;

	HISE_EMPTY_HANDLE_EVENT;
	HISE_EMPTY_INITIALISE;
	HISE_EMPTY_MOD;

	void reset()
	{
		if (rb != nullptr)
			rb->clear();
	}

	template <typename ProcessDataType> void process(ProcessDataType& data) noexcept
	{
		if (rb != nullptr && rb->isActive())
			rb->write(const_cast<const float**>(data.getRawDataPointers()), data.getNumChannels(), data.getNumSamples());
	}

	template <typename FrameDataType> void processFrame(FrameDataType& data) noexcept
	{
	
	}

	JUCE_DECLARE_WEAK_REFERENCEABLE(analyse_base);
};

template class analyse_base<Helpers::FFT>;
using fft = analyse_base<Helpers::FFT>;

template class analyse_base<Helpers::Oscilloscope>;
using oscilloscope = analyse_base<Helpers::Oscilloscope>;

template class analyse_base<Helpers::GonioMeter>;
using goniometer = analyse_base<Helpers::GonioMeter>;


namespace ui
{
struct fft_display : public ScriptnodeExtraComponent<fft>,
				public hise::FFTDisplayBase
{
	fft_display(fft* obj, PooledUIUpdater* updater) :
		ScriptnodeExtraComponent<fft>(obj, updater),
		FFTDisplayBase()
	{
		obj->getRingBuffer()->setRingBufferSize(1, 16384);
		obj->getRingBuffer()->setGlobalUIUpdater(updater);
		setComplexDataUIBase(obj->getRingBuffer());
		
		setSize(512, 100);
	}

	double getSamplerate() const override
	{
		return getObject()->getSampleRate();
	}

	Colour getColourForAnalyserBase(int colourId)
	{
		return Helpers::getColourBase(colourId);
	}

	void timerCallback() override { repaint(); }

	void paint(Graphics& g) override
	{
		FFTDisplayBase::drawSpectrum(g);
	}

	static Component* createExtraComponent(void* obj, PooledUIUpdater* updater)
	{
		auto typed = static_cast<fft*>(obj);
		return new fft_display(typed, updater);
	}
};

struct osc_display : public ScriptnodeExtraComponent<oscilloscope>,
					 public OscilloscopeBase
{
	osc_display(oscilloscope* obj, PooledUIUpdater* updater) :
		ScriptnodeExtraComponent<oscilloscope>(obj, updater),
		OscilloscopeBase()
	{
		setComplexDataUIBase(obj->getRingBuffer());
		obj->getRingBuffer()->setGlobalUIUpdater(updater);
		obj->getRingBuffer()->setRingBufferSize(2, 2048);
		setSize(512, 100);
	}

	Colour getColourForAnalyserBase(int colourId)
	{
		return Helpers::getColourBase(colourId);
	}

	void timerCallback() override { repaint(); }

	void paint(Graphics& g) override
	{
		OscilloscopeBase::drawWaveform(g);
	}

	static Component* createExtraComponent(void* obj, PooledUIUpdater* updater)
	{
		auto typed = static_cast<oscilloscope*>(obj);
		return new osc_display(typed, updater);
	}
};

struct gonio_display : public ScriptnodeExtraComponent<goniometer>,
					   public hise::GoniometerBase
{
	gonio_display(goniometer* obj, PooledUIUpdater* updater) :
		ScriptnodeExtraComponent<goniometer>(obj, updater),
		GoniometerBase()
	{
		setComplexDataUIBase(obj->getRingBuffer());
		obj->getRingBuffer()->setRingBufferSize(2, 8192);
		obj->getRingBuffer()->setGlobalUIUpdater(updater);
		setSize(256, 256);
	}

	Colour getColourForAnalyserBase(int colourId) override
	{
		return Helpers::getColourBase(colourId);
	}

	void timerCallback() override { repaint(); }

	void paint(Graphics& g) override
	{
		GoniometerBase::paintSpacialDots(g);
	}

	static Component* createExtraComponent(void* obj, PooledUIUpdater* updater)
	{
		auto typed = static_cast<goniometer*>(obj);
		return new gonio_display(typed, updater);
	}
};

	
}

}

}
