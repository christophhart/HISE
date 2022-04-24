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

		template <typename ProcessDataType> void updateBuffer(ProcessDataType& data)
		{
			if (rb != nullptr && rb->isActive())
				rb->write(const_cast<const float**>(data.getRawDataPointers()), data.getNumChannels(), data.getNumSamples());
		}

		PrepareSpecs lastSpecs;

		JUCE_DECLARE_WEAK_REFERENCEABLE(AnalyserDataProvider);
	};

	static Colour getColourBase(int colourId)
	{
		if (colourId == RingBufferComponentBase::ColourId::bgColour)
			return Colour(0xFF333333);
		if (colourId == RingBufferComponentBase::ColourId::fillColour)
			return Colours::white.withAlpha(0.7f);
		if (colourId == RingBufferComponentBase::ColourId::lineColour)
			return Colours::white;

		return Colours::transparentBlack;
	}

	struct FFT: public SimpleRingBuffer::PropertyObject
	{
		static constexpr int PropertyIndex = 3001;

		int getClassIndex() const override { return PropertyIndex; }

		SN_NODE_ID("fft");

		static String getDescription() { return "A FFT analyser"; };

		static constexpr int NumChannels = 1;
		
		FFT(SimpleRingBuffer::WriterBase* w) : PropertyObject(w) {};

		RingBufferComponentBase* createComponent();

		bool validateInt(const Identifier& id, int& desiredSize) const override
		{
			if (id == RingBufferIds::BufferLength)
			{
				auto capped = jlimit<int>(8192, 32768, desiredSize);
				auto ns = nextPowerOfTwo(capped);

				if (ns != desiredSize)
				{
					desiredSize = ns;
					return true;
				}

				return false;
			}
			if (id == RingBufferIds::NumChannels)
				return SimpleRingBuffer::toFixSize<1>(desiredSize);
			
			return false;
		}

		void transformReadBuffer(AudioSampleBuffer& b) override
		{
			// put the window function stuff here...
		}
	};

	struct Oscilloscope: public SimpleRingBuffer::PropertyObject
	{
		static constexpr int PropertyIndex = 3002;

		int getClassIndex() const override { return PropertyIndex; }

		SN_NODE_ID("oscilloscope");

		static String getDescription() { return "an oscilloscope with optional MIDI input sync"; };

		

		Oscilloscope(SimpleRingBuffer::WriterBase* w) : PropertyObject(w) {};

		static constexpr int NumChannels = 2;

		RingBufferComponentBase* createComponent() override;

		bool validateInt(const Identifier& id, int& v) const override
		{
			if(id == RingBufferIds::BufferLength)
				return SimpleRingBuffer::withinRange<128, 32768*2>(v);
			if(id == RingBufferIds::NumChannels)
				return SimpleRingBuffer::withinRange<1, 2>(v);

			return false;
		}
	};

	struct GonioMeter : public SimpleRingBuffer::PropertyObject
	{
		static constexpr int PropertyIndex = 3003;

		int getClassIndex() const override { return PropertyIndex; }

		static String getDescription() { return "A goniometer (stereo correlation display)."; };

		SN_NODE_ID("goniometer");

		GonioMeter(SimpleRingBuffer::WriterBase* w) : PropertyObject(w) {};

		static constexpr int NumChannels = 2;

		RingBufferComponentBase* createComponent() override;

		bool validateInt(const Identifier& id, int& v) const override
		{
			if(id == RingBufferIds::BufferLength)
				return SimpleRingBuffer::withinRange<512, 32768>(v);
			if (id == RingBufferIds::NumChannels)
				return SimpleRingBuffer::toFixSize<2>(v);

			return false;
		}
	};

};

template <class T> class analyse_base : public Helpers::AnalyserDataProvider,
				                        public AsyncUpdater
{
public:

	SN_NODE_ID(T::getStaticId());
    SN_GET_SELF_AS_OBJECT(analyse_base);
    
	SN_EMPTY_MOD;
	
	constexpr bool isProcessingHiseEvent() const { return std::is_same<T, Helpers::Oscilloscope>(); }

	analyse_base()
	{
		
	}

	SN_DESCRIPTION(T::getDescription());

	virtual ~analyse_base() {};

	void reset()
	{
		if (rb != nullptr)
			rb->clear();
	}

	void registerPropertyObject(SimpleRingBuffer::Ptr rb) override
	{
		rb->registerPropertyObject<T>();
	}

	void handleHiseEvent(HiseEvent& e)
	{
		if(isProcessingHiseEvent() && rb != nullptr && e.isNoteOn())
		{
			auto sr = rb->getSamplerate();
			
			auto numSamplesForCycle = 1.0 / e.getFrequency() * sr;

			while (numSamplesForCycle < 128.0 && numSamplesForCycle != 0.0)
				numSamplesForCycle *= 2.0;

			syncSamples = roundToInt(numSamplesForCycle);

			triggerAsyncUpdate();
		}
	}

	void handleAsyncUpdate() override
	{
		rb->setRingBufferSize(2, syncSamples);
	}

	template <typename ProcessDataType> void process(ProcessDataType& data) noexcept
	{
		ProcessData<T::NumChannels> pd(data.getRawDataPointers(), data.getNumSamples());
		updateBuffer(pd);
	}

	template <typename FrameDataType> void processFrame(FrameDataType& data) noexcept
	{
	
	}

	int syncSamples = 0;

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

struct simple_osc_display : public hise::OscilloscopeBase,
						    public Component,
							public ComponentWithDefinedSize
{
	simple_osc_display() = default;

	Colour getColourForAnalyserBase(int colourId) override
	{
		return Helpers::getColourBase(colourId);
	}
	
	Rectangle<int> getFixedBounds() const override { return { 0, 0, 512, 200 }; }

	void paint(Graphics& g) override
	{
		OscilloscopeBase::drawWaveform(g);
	}

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(simple_osc_display);
};


struct simple_fft_display : public hise::FFTDisplayBase,
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

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(simple_fft_display);
};

struct simple_gon_display : public hise::GoniometerBase,
						    public Component,
							public ComponentWithDefinedSize
{
	simple_gon_display() = default;

	Colour getColourForAnalyserBase(int colourId) override
	{
		return Helpers::getColourBase(colourId);
	}

	Rectangle<int> getFixedBounds() const override { return { 0, 0, 256, 256 }; }

	void paint(Graphics& g) override
	{
		auto laf = getSpecialLookAndFeel<LookAndFeelMethods>();
		laf->drawOscilloscopeBackground(g, *this, getLocalBounds().toFloat());

		GoniometerBase::paintSpacialDots(g);
	}

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(simple_gon_display);
};
	
}

}

}
