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
	struct AnalyserDataProvider: public data::display_buffer_base<true>
	{
		AnalyserDataProvider()
		{
		}

		SimpleRingBuffer::Ptr getRingBuffer() { return dynamic_cast<SimpleRingBuffer*>(externalData.obj); }

		virtual ~AnalyserDataProvider() {};

		void prepare(PrepareSpecs ps)
		{
			channelsToProcess = ps.numChannels;

			if (getRingBuffer() != nullptr)
			{
				auto numSamples = getRingBuffer()->getReadBuffer().getNumSamples();
				getRingBuffer()->setRingBufferSize(ps.numChannels, numSamples);
				getRingBuffer()->setSamplerate(ps.sampleRate);
			}
		}

		template <typename ProcessDataType> void updateBuffer(ProcessDataType& data)
		{
			if (rb != nullptr && rb->isActive())
				rb->write(const_cast<const float**>(data.getRawDataPointers()), data.getNumChannels(), data.getNumSamples());
		}

		int channelsToProcess = 0;

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
		
		static constexpr int NumChannels = 1;
		static constexpr int NumSamples = 16384;

		static int getValidBufferSize(int desiredSize)
		{
			auto capped = jlimit<int>(512, 32768, desiredSize);
			return nextPowerOfTwo(capped);
		}

		static constexpr int getValidChannelSize(int numChannels)
		{
			return 1;
		}
	};

	struct Oscilloscope
	{
		SET_HISE_NODE_ID("oscilloscope");

		static constexpr int NumChannels = 2;
		static constexpr int NumSamples = 2048;

		static int getValidBufferSize(int desiredSize)
		{
			return jlimit<int>(512, 32768 * 4, desiredSize);
		}

		static constexpr int getValidChannelSize(int numChannels)
		{
			return numChannels;
		}
	};

	struct GonioMeter
	{
		SET_HISE_NODE_ID("goniometer");

		static constexpr int NumChannels = 2;
		static constexpr int NumSamples = 8192;

		static int getValidBufferSize(int desiredSize)
		{
			return 8192;
		}

		static constexpr int getValidChannelSize(int numChannels)
		{
			return 2;
		}
	};

};

template <class T> class analyse_base : public Helpers::AnalyserDataProvider
{
public:

	SET_HISE_NODE_ID(T::getStaticId());
    SN_GET_SELF_AS_OBJECT(analyse_base);
    
	
	HISE_EMPTY_HANDLE_EVENT;
	HISE_EMPTY_INITIALISE;
	HISE_EMPTY_MOD;

	analyse_base()
	{
		setRequiredBufferSize(T::NumChannels, T::NumSamples);
	}

	virtual ~analyse_base() {};

	void reset()
	{
		if (rb != nullptr)
			rb->clear();
	}

	template <typename ProcessDataType> void process(ProcessDataType& data) noexcept
	{
		ProcessData<T::NumChannels> pd(data.getRawDataPointers(), data.getNumSamples());
		updateBuffer(pd);
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

struct simple_osc_display : public OscilloscopeBase,
						    public Component,
							public ComponentWithDefinedSize
{
	Colour getColourForAnalyserBase(int colourId) override
	{
		return Helpers::getColourBase(colourId);
	}
	
	Rectangle<int> getFixedBounds() const override { return { 0, 0, 512, 200 }; }

	void paint(Graphics& g) override
	{
		g.setColour(Colour(0xFF717171));
		g.drawRect(getLocalBounds(), 1.0f);
		OscilloscopeBase::drawWaveform(g);
	}
};


struct simple_fft_display : public FFTDisplayBase,
							public Component,
							public ComponentWithDefinedSize
{
	simple_fft_display();

	Colour getColourForAnalyserBase(int colourId) override
	{
		return Helpers::getColourBase(colourId);

	}

	juce::Rectangle<int> getFixedBounds() const override { return { 0, 0, 512, 100 }; }

	double getSamplerate() const override
	{
		if (rb != nullptr)
			return rb->getSamplerate();

		return 44100.0;
	}

	void paint(Graphics& g) override
	{
		g.setColour(Colour(0xFF717171));
		g.drawRect(getLocalBounds(), 1.0f);
		FFTDisplayBase::drawSpectrum(g);
	}
};

struct simple_gon_display : public GoniometerBase,
						    public Component,
							public ComponentWithDefinedSize
{
	Colour getColourForAnalyserBase(int colourId) override
	{
		return Helpers::getColourBase(colourId);
	}

	Rectangle<int> getFixedBounds() const override { return { 0, 0, 256, 256 }; }

	void paint(Graphics& g) override
	{
		g.setColour(Colour(0xFF717171));
		g.drawRect(getLocalBounds(), 1.0f);
		GoniometerBase::paintSpacialDots(g);
	}
};

using osc_display = data::ui::pimpl::editorT<data::dynamic::displaybuffer, 
											 hise::SimpleRingBuffer, 
											 simple_osc_display,
											 false>; 

using fft_display = data::ui::pimpl::editorT<data::dynamic::displaybuffer, 
											 hise::SimpleRingBuffer, 
											 simple_fft_display,
											 false>; 

using gonio_display = data::ui::pimpl::editorT<data::dynamic::displaybuffer,
											   hise::SimpleRingBuffer,
											   simple_gon_display,
											   false>;

	
}

}

}
