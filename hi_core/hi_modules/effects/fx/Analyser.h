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

#ifndef ANALYSEREFFECT_H_INCLUDED
#define ANALYSEREFFECT_H_INCLUDED

namespace hise {
using namespace juce;




/** A analyser that powers one of the available visualisations in HISE (FFT, Oscilloscope or Goniometer.
	@ingroup effectTypes
*/
class AnalyserEffect : public MasterEffectProcessor,
                       public ProcessorWithStaticExternalData
{
public:

	enum Parameters
	{
		PreviewType,
		BufferSize,
		numParameters
	};

    enum class AnalyseType
    {
        Nothing = 1,
        Goniometer,
        Oscilloscope,
        SpectrumAnalyser
    };
    
	SET_PROCESSOR_NAME("Analyser", "Analyser", "A audio analysis module");

	AnalyserEffect(MainController *mc, const String &uid) :
		MasterEffectProcessor(mc, uid),
        ProcessorWithStaticExternalData(mc, 0, 0, 0, 1)
	{
        ringBuffer = getDisplayBuffer(0);
		ringBuffer->setGlobalUIUpdater(mc->getGlobalUIUpdater());
		finaliseModChains();

		parameterNames.add("PreviewType"); 
		parameterDescriptions.add("The index of the visualisation type.");
		parameterNames.add("BufferSize");
		parameterDescriptions.add("The buffer size of the internal ring buffer.");

		updateParameterSlots();

		ringBuffer->setRingBufferSize(2, 8192);
	};

    void updateType(AnalyseType newType)
    {
        if(currentType != newType)
        {
            currentType = newType;
            
            SimpleRingBuffer::ScopedPropertyCreator spc(ringBuffer.get());
            
            switch(currentType)
            {
                case AnalyseType::Nothing: break;
                case AnalyseType::Goniometer:
                    ringBuffer->registerPropertyObject<scriptnode::analyse::Helpers::GonioMeter>();
                    break;
                case AnalyseType::Oscilloscope:
                    ringBuffer->registerPropertyObject<scriptnode::analyse::Helpers::Oscilloscope>();
                    break;
                case AnalyseType::SpectrumAnalyser:
                    ringBuffer->registerPropertyObject<scriptnode::analyse::Helpers::FFT>();
                    break;
            }
        }
    }
    
	void setInternalAttribute(int index, float newValue)
	{
		switch (index)
		{
            case Parameters::PreviewType: updateType((AnalyseType)(int)newValue); break;
            case Parameters::BufferSize:  ringBuffer->setRingBufferSize(2, (int)newValue); break;
		}
	}

	float getAttribute(int index) const override
	{
		switch (index)
		{
		case Parameters::PreviewType: return (float)currentType;
		case Parameters::BufferSize: return (float)ringBuffer->getReadBuffer().getNumSamples();
		default: jassertfalse; return -1;
		}
	}

	~AnalyserEffect()
	{
		
	};

	void restoreFromValueTree(const ValueTree &v) override
	{
		MasterEffectProcessor::restoreFromValueTree(v);

		loadAttribute(BufferSize, "BufferSize");
		loadAttribute(PreviewType, "PreviewType");
	};

	ValueTree exportAsValueTree() const override
	{
		ValueTree v = MasterEffectProcessor::exportAsValueTree();

		saveAttribute(BufferSize, "BufferSize");
		saveAttribute(PreviewType, "PreviewType");

		return v;
	}
	
	bool hasTail() const override { return false; };

	int getNumInternalChains() const override { return 0; };
	int getNumChildProcessors() const override { return 0; };

	Processor *getChildProcessor(int /*processorIndex*/) override
	{
		return nullptr;
	};

	const Processor *getChildProcessor(int /*processorIndex*/) const override
	{
		return nullptr;
	};

	ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor)  override;

    void prepareToPlay(double sr, int blockSize) override
    {
        MasterEffectProcessor::prepareToPlay(sr, blockSize);
        ringBuffer->setSamplerate(sr);
    }
    
	void applyEffect(AudioSampleBuffer &b, int startSample, int numSamples)
	{
		if (ringBuffer != nullptr && ringBuffer->isActive())
			ringBuffer->write(b, startSample, numSamples);
	}

private:

	SimpleRingBuffer::Ptr ringBuffer;
	AnalyseType currentType = AnalyseType::Nothing;

	JUCE_DECLARE_WEAK_REFERENCEABLE(AnalyserEffect);
};


class AudioAnalyserComponent : public Component,
	public Timer
{
public:

	

	AudioAnalyserComponent(Processor* p) :
		processor(p)
	{
		setColour(RingBufferComponentBase::ColourId::bgColour, Colours::transparentBlack);

		startTimer(30);
	};

	Colour getColourForAnalyser(RingBufferComponentBase::ColourId id);

	void timerCallback() override { repaint(); }

	class Panel : public PanelWithProcessorConnection
	{
	public:

		Panel(FloatingTile* parent) :
			PanelWithProcessorConnection(parent)
		{
			setDefaultPanelColour(FloatingTileContent::PanelColourId::bgColour, Colour(0xFF333333));
			setDefaultPanelColour(FloatingTileContent::PanelColourId::itemColour1, Colour(0xFF888888));
			setDefaultPanelColour(FloatingTileContent::PanelColourId::itemColour2, Colour(0xFF444444));
		};

		SET_PANEL_NAME("AudioAnalyser");

		Identifier getProcessorTypeId() const override
		{
			return AnalyserEffect::getClassType();
		}

		Component* createContentComponent(int index) override;

		void fillModuleList(StringArray& moduleList) override
		{
			fillModuleListWithType<AnalyserEffect>(moduleList);
		}

		bool hasSubIndex() const override { return true; }

		void fillIndexList(StringArray& indexList) override;
	};

protected:

	AnalyserEffect * getAnalyser();
	const AnalyserEffect* getAnalyser() const;

	WeakReference<Processor> processor;

	
};


class FFTDisplay : public FFTDisplayBase,
				   public AudioAnalyserComponent
{
public:

	FFTDisplay(Processor* p) :
	   FFTDisplayBase(),
       AudioAnalyserComponent(p)
	{
		setComplexDataUIBase(dynamic_cast<AnalyserEffect*>(p)->getDisplayBufferUnchecked(0));
	};

	void paint(Graphics& g) override
	{
		drawSpectrum(g);
	}

	double getSamplerate() const override { return getAnalyser()->getSampleRate(); }
	Colour getColourForAnalyserBase(int colourId) override { return AudioAnalyserComponent::getColourForAnalyser((RingBufferComponentBase::ColourId)colourId); }
};


class Oscilloscope : public AudioAnalyserComponent,
					 public OscilloscopeBase
{
public:

	Oscilloscope(Processor* p) :
		AudioAnalyserComponent(p),
		OscilloscopeBase()
	{
		setComplexDataUIBase(dynamic_cast<AnalyserEffect*>(p)->getDisplayBufferUnchecked(0));
	};

	Colour getColourForAnalyserBase(int colourId) override { return getColourForAnalyser((RingBufferComponentBase::ColourId)colourId); }

	void paint(Graphics& g) override
	{
		OscilloscopeBase::drawWaveform(g);
	}
};


class Goniometer : public AudioAnalyserComponent,
				   public GoniometerBase
{
public:

	Goniometer(Processor* p) :
		AudioAnalyserComponent(p),
		GoniometerBase()
	{
		setComplexDataUIBase(dynamic_cast<AnalyserEffect*>(p)->getDisplayBufferUnchecked(0));
	}

	Colour getColourForAnalyserBase(int colourId) override { return getColourForAnalyser((RingBufferComponentBase::ColourId)colourId); }

	void paint(Graphics& g) override
	{
		GoniometerBase::paintSpacialDots(g);
	}
	
};


} // namespace hise

#endif  // ANALYSEREFFECT_H_INCLUDED
