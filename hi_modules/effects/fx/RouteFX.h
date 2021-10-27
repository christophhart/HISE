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

	void addSendSignal(AudioSampleBuffer& b, int startSample, int numSamples, float gain, int channelOffset)
	{
		sendBuffer.addFrom(channelOffset, startSample, b, 0, startSample, numSamples, gain);
		sendBuffer.addFrom(channelOffset+1, startSample, b, 1, startSample, numSamples, gain);
	}

	void prepareToPlay(double sampleRate, int samplesPerBlock) override
	{
		ModulatorSynth::prepareToPlay(sampleRate, samplesPerBlock);
		sendBuffer.setSize(getMatrix().getNumSourceChannels(), samplesPerBlock);
	}

	void renderNextBlockWithModulators(AudioSampleBuffer& outputAudio, const HiseEventBuffer& inputMidi) override
	{
		effectChain->renderMasterEffects(sendBuffer);
		sendBuffer.clear();
	}

	AudioSampleBuffer sendBuffer;

	JUCE_DECLARE_WEAK_REFERENCEABLE(SendContainer);
};

struct SendEffect : public MasterEffectProcessor
{
	SET_PROCESSOR_NAME("SendFX", "Send Effect", "A signal chain tool that allows to send the signal to a send container");

	enum Parameters
	{
		Gain,
		ChannelOffset,
		numParameters
	};

	SendEffect(MainController *mc, const String &id) :
		MasterEffectProcessor(mc, id)
	{
		finaliseModChains();
	};

	float getAttribute(int index) const override 
	{
		switch (index)
		{
		case Parameters::Gain: return Decibels::gainToDecibels(gain.getTargetValue());
		case Parameters::ChannelOffset: return (float)channelOffset;
		}
	};
	void setInternalAttribute(int index, float newValue) override 
	{
		switch (index)
		{
		case Parameters::Gain: gain.setValue(Decibels::decibelsToGain(newValue)); break;
		case Parameters::ChannelOffset: channelOffset = (int)newValue;
		}
	};

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

#if 1 || USE_BACKEND
	struct Editor : public ProcessorEditorBody,
					public ComboBox::Listener
	{
		Editor(ProcessorEditor* parent) :
			ProcessorEditorBody(parent),
			gainSlider("Gain"),
			offsetSlider("Offset")
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
			connectionBox.addListener(this);

			auto list = ProcessorHelpers::getListOfAllProcessors<SendContainer>(getProcessor()->getMainController()->getMainSynthChain());

			int index = 1;

			for (auto c : list)
				connectionBox.addItem(c->getId(), index++);
		};

		void comboBoxChanged(ComboBox* c) override
		{
			dynamic_cast<SendEffect*>(getProcessor())->connect(c->getText());
		}

		void updateGui() override
		{
			gainSlider.updateValue(dontSendNotification);
			offsetSlider.updateValue(dontSendNotification);

			auto target = dynamic_cast<SendEffect*>(getProcessor())->container;

			if (target == nullptr)
				connectionBox.setText("", dontSendNotification);
			else
				connectionBox.setText(target->getId());
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
		ComboBox connectionBox;
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
			container->addSendSignal(b, startSample, numSamples, gain.getNextValue(), channelOffset);
		}
	}

	void connect(const String& sendId)
	{
		if (auto c = ProcessorHelpers::getFirstProcessorWithName(getMainController()->getMainSynthChain(), sendId))
		{
			if (auto typed = dynamic_cast<SendContainer*>(c))
			{
				SimpleReadWriteLock::ScopedWriteLock sl(lock);
				container = typed;
				return;
			}
		}

		SimpleReadWriteLock::ScopedWriteLock sl(lock);
		container = nullptr;
	}

	juce::SmoothedValue<float> gain;
	int channelOffset;

	SimpleReadWriteLock lock;
	WeakReference<SendContainer> container;
};


} // namespace hise

#endif  // ROUTEFX_H_INCLUDED
