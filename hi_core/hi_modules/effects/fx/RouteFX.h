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

#ifndef ROUTEFX_H_INCLUDED
#define ROUTEFX_H_INCLUDED

 
namespace hise { using namespace juce;


/** A signal chain tool that allows to duplicate and send the signal to other channels to build AUX signal paths.
	@ingroup effectTypes.

*/
class RouteEffect : public MasterEffectProcessor
{
public:

	SET_PROCESSOR_NAME("RouteFX", "Routing Matrix", "A signal chain tool that allows to duplicate and send the signal to other channels to build AUX signal paths.");

	RouteEffect(MainController *mc, const String &uid);;

	float getAttribute(int ) const override { return 1.0f; };
	void setInternalAttribute(int , float ) override {};

	void restoreFromValueTree(const ValueTree &v) override
	{
		MasterEffectProcessor::restoreFromValueTree(v);
	};

	ValueTree exportAsValueTree() const override
	{
		ValueTree v = MasterEffectProcessor::exportAsValueTree();
		
		return v;
	}


	int getNumInternalChains() const override { return 0; };

	bool hasTail() const override { return false; };

	Processor *getChildProcessor(int /*processorIndex*/) override { return nullptr; };

	const Processor *getChildProcessor(int /*processorIndex*/) const override { return nullptr; };

	int getNumChildProcessors() const override { return 0; };

	ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor)  override;

	void prepareToPlay(double sampleRate, int samplesPerBlock)
	{
		MasterEffectProcessor::prepareToPlay(sampleRate, samplesPerBlock);
	}

	void renderWholeBuffer(AudioSampleBuffer &buffer) override;
	
	void setSoftBypass(bool /*shouldBeSoftBypassed*/, bool /*useRamp*//* =true */) override {};

	bool isFadeOutPending() const noexcept override
	{
		return false;
	}

	void applyEffect(AudioSampleBuffer &/*b*/, int /*startSample*/, int /*numSamples*/) override;

private:

	

};

struct SendContainer : public ModulatorSynth
{
	SET_PROCESSOR_NAME("SendContainer", "Send Container", "A signal chain tool that allows to receive the signal from a Send FX");

	SendContainer(MainController* mc, const String& id) :
		ModulatorSynth(mc, id, 1)
	{
		finaliseModChains();
		getMatrix().setAllowResizing(true);
		
	}

	void numSourceChannelsChanged() override
	{
		prepareToPlay(getSampleRate(), getLargestBlockSize());
	}

	float getAttribute(int) const override { return 1.0f; };
	void setInternalAttribute(int, float) override {};

	void addProcessorsWhenEmpty() override
	{

	};

	void restoreFromValueTree(const ValueTree &v) override
	{
		ModulatorSynth::restoreFromValueTree(v);
	};

	ValueTree exportAsValueTree() const override
	{
		ValueTree v = ModulatorSynth::exportAsValueTree();
		return v;
	}

	ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor)  override
	{
#if USE_BACKEND
		return new EmptyProcessorEditorBody(parentEditor);
#else
		return nullptr;
#endif
	}

	void addSendSignal(AudioSampleBuffer& b, int startSample, int numSamples, float startGain, float endGain, int channelOffset)
	{
        channelOffset = jlimit(0, internalBuffer.getNumChannels() - 2, channelOffset);
        
		auto isStereo = b.getNumChannels() == 2;

        if(startGain == endGain)
        {
            internalBuffer.addFrom(channelOffset, startSample, b, 0, startSample, numSamples, startGain);

			if(isStereo)
				internalBuffer.addFrom(channelOffset+1, startSample, b, 1, startSample, numSamples, startGain);
        }
        else
        {
            internalBuffer.addFromWithRamp(channelOffset, startSample, b.getReadPointer(0, startSample), numSamples, startGain, endGain);

			if(isStereo)
				internalBuffer.addFromWithRamp(channelOffset+1, startSample, b.getReadPointer(1, startSample), numSamples, startGain, endGain);
        }
	}

	void prepareToPlay(double sampleRate, int samplesPerBlock) override
	{
		if (samplesPerBlock > 0)
		{
			ModulatorSynth::prepareToPlay(sampleRate, samplesPerBlock);
			internalBuffer.setSize(getMatrix().getNumSourceChannels(), samplesPerBlock);
		}
	}

	void renderNextBlockWithModulators(AudioSampleBuffer& outputAudio, const HiseEventBuffer& inputMidi) override
	{
		processHiseEventBuffer(inputMidi, outputAudio.getNumSamples());

		int numSamplesToProcess;

		if (outputAudio.getNumSamples() < internalBuffer.getNumSamples())
		{
			numSamplesToProcess = outputAudio.getNumSamples();
			AudioSampleBuffer truncatedInternalBuffer(internalBuffer.getArrayOfWritePointers(), internalBuffer.getNumChannels(), numSamplesToProcess);
			effectChain->renderMasterEffects(truncatedInternalBuffer);
		}
		else
		{
			numSamplesToProcess = internalBuffer.getNumSamples();
			effectChain->renderMasterEffects(internalBuffer);
		}

        for(int i = 0; i < internalBuffer.getNumChannels(); i++)
        {
            auto idx = getMatrix().getConnectionForSourceChannel(i);
            
            if(isPositiveAndBelow(idx, outputAudio.getNumChannels()))
                outputAudio.addFrom(idx, 0, internalBuffer, i, 0, numSamplesToProcess);
        }
        
        getMatrix().handleDisplayValues(internalBuffer, outputAudio, true);

        handlePeakDisplay(numSamplesToProcess);
        
        internalBuffer.clear();
	}

	

	JUCE_DECLARE_WEAK_REFERENCEABLE(SendContainer);
};

struct SendEffect : public MasterEffectProcessor
{
	SET_PROCESSOR_NAME("SendFX", "Send Effect", "A signal chain tool that allows to send the signal to a send container");

    enum class InternalChains
    {
        SendLevel,
        numInternalChains
    };
    
	enum Parameters
	{
		Gain,
		ChannelOffset,
        SendIndex,
		numParameters
	};

	SendEffect(MainController *mc, const String &id) :
		MasterEffectProcessor(mc, id)
	{
		getMatrix().setNumAllowedConnections(-1);
        modChains.reserve((int)InternalChains::numInternalChains);

        modChains += {this, "Send Modulation"};
        
        finaliseModChains();

        sendChain = modChains[(int)InternalChains::SendLevel].getChain();
        
#if 0
        auto gainConverter = [tmp](float input)
        {
            if (tmp)
            {
                auto v = Decibels::decibelsToGain(tmp->getAttribute(GainEffect::Parameters::Gain));
                auto dbValue = Decibels::gainToDecibels(v * input);
                return String(dbValue, 1) + " dB";
            }

            return Table::getDefaultTextValue(input);
        };

        sendChain->setTableValueConverter(gainConverter);
#endif
        
        parameterNames.add("Gain");
        parameterNames.add("ChannelOffset");
        parameterNames.add("SendIndex");
	};

    ~SendEffect()
    {
        modChains.clear();
    }
    
	float getAttribute(int index) const override 
	{
		switch (index)
		{
		case Parameters::Gain: return Decibels::gainToDecibels(gain.getTargetValue());
		case Parameters::ChannelOffset: return (float)channelOffset;
        case Parameters::SendIndex: return sendIndex;
		}
        
        return 0.0f;
	};
    
	void setInternalAttribute(int index, float newValue) override 
	{
		switch (index)
		{
		case Parameters::Gain: gain.setValue(Decibels::decibelsToGain(newValue)); break;
        case Parameters::ChannelOffset: channelOffset = (int)newValue; break;
        case Parameters::SendIndex: connect((int)newValue); break;
		}
	};

	void restoreFromValueTree(const ValueTree &v) override
	{
		MasterEffectProcessor::restoreFromValueTree(v);
        
        loadAttribute(Gain, "Gain");
        loadAttribute(ChannelOffset, "ChannelOffset");
        loadAttribute(SendIndex, "SendIndex");
	};

	ValueTree exportAsValueTree() const override
	{
		ValueTree v = MasterEffectProcessor::exportAsValueTree();
        
        saveAttribute(Gain, "Gain");
        saveAttribute(ChannelOffset, "ChannelOffset");
        saveAttribute(SendIndex, "SendIndex");
        
		return v;
	}

	int getNumInternalChains() const override { return 1; };

	bool hasTail() const override { return false; };

	bool isSuspendedOnSilence() const override { return true; }

	Processor *getChildProcessor(int /*processorIndex*/) override { return sendChain; };

	const Processor *getChildProcessor(int /*processorIndex*/) const override { return sendChain; };

	int getNumChildProcessors() const override { return 1; };

#if USE_BACKEND
	struct Editor : public ProcessorEditorBody
	{
		Editor(ProcessorEditor* parent) :
			ProcessorEditorBody(parent),
			gainSlider("Gain"),
			offsetSlider("Offset"),
            connectionBox("SendIndex")
		{
			gainSlider.setup(parent->getProcessor(), Parameters::Gain, "Gain");
			
			gainSlider.setMode(HiSlider::Mode::Decibel);

			offsetSlider.setup(parent->getProcessor(), Parameters::ChannelOffset, "Channel");
			offsetSlider.setMode(HiSlider::Mode::Discrete, 0, NUM_MAX_CHANNELS, DBL_MAX, 2);

			offsetSlider.setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
			offsetSlider.setTextBoxStyle(Slider::TextBoxRight, false, 80, 20);
			gainSlider.setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
			gainSlider.setTextBoxStyle(Slider::TextBoxRight, false, 80, 20);

			addAndMakeVisible(gainSlider);
			addAndMakeVisible(offsetSlider);

			addAndMakeVisible(connectionBox);
			connectionBox.setLookAndFeel(&claf);
			claf.setDefaultColours(connectionBox);
            connectionBox.setup(parent->getProcessor(), Parameters::SendIndex, "SendIndex");

			auto list = ProcessorHelpers::getListOfAllProcessors<SendContainer>(getProcessor()->getMainController()->getMainSynthChain());

			int index = 1;

			for (auto c : list)
				connectionBox.addItem(c->getId(), index++);
		};

		void updateGui() override
		{
			gainSlider.updateValue(dontSendNotification);
			offsetSlider.updateValue(dontSendNotification);
            connectionBox.updateValue(dontSendNotification);
		}

		int getBodyHeight() const override
		{
			return 48;
		}

		void resized() override
		{
			auto b = getLocalBounds().reduced(20, 0);
			
			gainSlider.setBounds(b.removeFromLeft(128));
			b.removeFromLeft(10);
			offsetSlider.setBounds(b.removeFromLeft(128));
			b.removeFromLeft(10);
			connectionBox.setBounds(b.reduced(10));
		}

		HiSlider gainSlider;
		HiSlider offsetSlider;
		HiComboBox connectionBox;
		GlobalHiseLookAndFeel claf;
	};
#endif

	ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor)  override
	{
#if USE_BACKEND
		return new Editor(parentEditor);
#else
		return nullptr;
#endif
	}

	void applyEffect(AudioSampleBuffer &b, int startSample, int numSamples) override
	{
		SimpleReadWriteLock::ScopedReadLock sl(lock);

		if (container != nullptr)
		{
            auto thisGain = gain.getCurrentValue();
            auto nextGain = gain.getNextValue();
            
            const float startModValue = modChains[(int)InternalChains::SendLevel].getOneModulationValue(startSample);
            const float endModValue = modChains[(int)InternalChains::SendLevel].getOneModulationValue(startSample + numSamples - 1);
            
            thisGain *= startModValue;
            nextGain *= endModValue;
            
            if(wasBypassed)
                thisGain = 0.0f;
            
            if(shouldBeBypassed)
                nextGain = 0.0f;
            
            wasBypassed = shouldBeBypassed;
            
            container->addSendSignal(b, startSample, numSamples, thisGain, nextGain, channelOffset);
		}
	}
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override
    {
        MasterEffectProcessor::prepareToPlay(sampleRate, samplesPerBlock);
        
        auto blockRate = sampleRate / (double)jmax(1, samplesPerBlock);
        
        gain.reset(blockRate, 0.08);
        
        if(sendIndex != 0 && container == nullptr)
            connect(sendIndex);
    }
    
    void setSoftBypass(bool shouldBeSoftBypassed, bool useRamp) override
    {
        shouldBeBypassed = shouldBeSoftBypassed;
        MasterEffectProcessor::setSoftBypass(shouldBeSoftBypassed, useRamp);
    };
    
    void connect(int index)
	{
        sendIndex = index;
        auto list = ProcessorHelpers::getListOfAllProcessors<SendContainer>(getMainController()->getMainSynthChain());
        
        if(index == 0)
        {
            SimpleReadWriteLock::ScopedWriteLock sl(lock);
            container = nullptr;
            return;
        }
        
        if(auto c = list[index-1])
        {
            SimpleReadWriteLock::ScopedWriteLock sl(lock);
            container = c;
            return;
        }
        
		SimpleReadWriteLock::ScopedWriteLock sl(lock);
		container = nullptr;
	}

	juce::SmoothedValue<float> gain;
    int channelOffset = 0;
    int sendIndex = 0;

    bool wasBypassed = false;
    bool shouldBeBypassed = false;
    
	SimpleReadWriteLock lock;
	WeakReference<SendContainer> container;
    
    ModulatorChain* sendChain;
};


} // namespace hise

#endif  // ROUTEFX_H_INCLUDED
