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


#ifndef AUDIOPROCESSORWRAPPER_H_INCLUDED
#define AUDIOPROCESSORWRAPPER_H_INCLUDED

namespace hise { using namespace juce;

typedef AudioProcessor *(createAudioProcessorFunction)();

/** This module is a wrapper for the general purpose AudioProcessor class from JUCE.
*
*	This allows to embed other plugin code directly into HISE.
*	To do this, create a method that creates an instance of your AudioProcessor and add this to the list of available AudioProcessors 
*	by calling AudioProcessorWrapper::addAudioProcessorToList("YourAudioProcessorIdentifier", &yourCreateAudioProcessorFunction);
*
*	The editor of your plugin will be automatically created. You can even display one in your scripted interface using "Content.addAudioProcessorEditor("id");"
*/
class AudioProcessorWrapper : public MasterEffectProcessor
{
public:

	SET_PROCESSOR_NAME("AudioProcessorWrapper", "Plugin Wrapper")

	enum InternalChains
	{
		WetAmountChain = 0,
		numInternalChains
	};

	enum EditorStates
	{
		WetAmountChainShown = Processor::numEditorStates,
		numEditorStates
	};

	AudioProcessorWrapper(MainController *mc, const String &uid);;

	~AudioProcessorWrapper();;

	void setInternalAttribute(int parameterIndex, float newValue) override;;
	float getAttribute(int parameterIndex) const override;
	float getDefaultValue(int parameterIndex) const override;

	void restoreFromValueTree(const ValueTree &v) override;;
	ValueTree exportAsValueTree() const override;

	bool hasTail() const override { return false; };

	Processor *getChildProcessor(int /*processorIndex*/) override { return wetAmountChain; };
	const Processor *getChildProcessor(int /*processorIndex*/) const override { return wetAmountChain; };
	int getNumInternalChains() const override { return numInternalChains; };
	int getNumChildProcessors() const override { return numInternalChains; };

	ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor)  override;

	void applyEffect(AudioSampleBuffer &buffer, int startSample, int numSamples) override;
	void prepareToPlay(double sampleRate, int samplesPerBlock) override;

	static void addAudioProcessorToList(const Identifier id, createAudioProcessorFunction *function);


	AudioProcessor *getWrappedAudioProcessor()
	{
		return wrappedAudioProcessor.get();
	}

    const AudioProcessor *getWrappedAudioProcessor() const
    {
        return wrappedAudioProcessor.get();
    }
    
	/** Returns a list of all registered processors. */
	static StringArray getRegisteredProcessorList()
	{
		StringArray sa;

		for (int i = 0; i < numRegisteredProcessors; i++)
		{
			sa.add(registeredAudioProcessors[i].id.toString());
		}

		return sa;
	}

	/** Loads a registered AudioProcessor and starts processing it. */
	void setAudioProcessor(const Identifier& processorId);

	void addEditor(Component *editor)
	{
		connectedEditors.add(editor);
	}

	void removeEditor(Component *editor)
	{
		connectedEditors.removeAllInstancesOf(editor);
	}

	struct ListEntry
	{
		ListEntry(const Identifier id_, createAudioProcessorFunction *function_) :
			id(id_),
			function(function_)
		{}

		ListEntry() :
			id(Identifier()),
			function(nullptr)
		{}

		Identifier id;
		createAudioProcessorFunction *function;
	};

private:

	CriticalSection wrapperLock;

	ScopedPointer<AudioProcessor> wrappedAudioProcessor;

	Identifier loadedProcessorId;

	ScopedPointer<ModulatorChain> wetAmountChain;

	AudioSampleBuffer wetAmountBuffer;

	AudioSampleBuffer tempBuffer;

	Array<Component::SafePointer<Component>> connectedEditors;

	static ListEntry registeredAudioProcessors[1024];

	static int numRegisteredProcessors;

};



class GenericEditor : public AudioProcessorEditor,
	public SliderListener,
	private Timer
{
public:
	enum
	{
		kParamSliderHeight = 50,
		kParamLabelWidth = 80,
		kParamSliderWidth = 128
	};

	GenericEditor(AudioProcessor& parent)
		: AudioProcessorEditor(parent),
		noParameterLabel("noparam", "No parameters available")
	{
		const OwnedArray<AudioProcessorParameter>& params = parent.getParameters();
		for (int i = 0; i < params.size(); ++i)
		{
			if (const AudioParameterFloat* param = dynamic_cast<AudioParameterFloat*>(params[i]))
			{
				Slider* aSlider;

				paramControllers.add(aSlider = new Slider(param->name));
				aSlider->setRange(param->range.start, param->range.end);
				aSlider->setSliderStyle(Slider::SliderStyle::RotaryHorizontalVerticalDrag);
				aSlider->setValue(dynamic_cast<const AudioProcessorParameter*>(param)->getValue());

				aSlider->addListener(this);
				addAndMakeVisible(aSlider);

				Label* aLabel;
				paramLabels.add(aLabel = new Label(param->name, param->name));

				aLabel->setFont(GLOBAL_BOLD_FONT());

				addAndMakeVisible(aLabel);
			}
#if 0 // Add this when the time comes...
			else if (const AudioParameterBool *param = dynamic_cast<AudioParameterBool*>(params[i]))
			{

			}
			else if (const AudioParameterChoice *param = dynamic_cast<AudioParameterChoice*>(params[i]))
			{

			}
#endif
		}

		noParameterLabel.setJustificationType(Justification::horizontallyCentred | Justification::verticallyCentred);
		noParameterLabel.setFont(noParameterLabel.getFont().withStyle(Font::italic));

		setSize(kParamSliderWidth + kParamLabelWidth,
			jmax(1, kParamSliderHeight * paramControllers.size()));

		if (paramControllers.size() == 0)
			addAndMakeVisible(noParameterLabel);
		else
			startTimer(100);
	}

	~GenericEditor()
	{
	}

	void resized() override
	{
		Rectangle<int> r = getLocalBounds();
		noParameterLabel.setBounds(r);

		for (int i = 0; i < paramControllers.size(); ++i)
		{
			Rectangle<int> paramBounds = r.removeFromTop(kParamSliderHeight);
			Rectangle<int> labelBounds = paramBounds.removeFromLeft(kParamLabelWidth);

			paramLabels[i]->setBounds(labelBounds);
			paramControllers[i]->setBounds(paramBounds);
		}

		setOpaque(false);
	}

	

	//==============================================================================
	void sliderValueChanged(Slider* slider) override
	{
		if (AudioProcessorParameter* param = getParameterForSlider(slider))
			param->setValueNotifyingHost((float)slider->getValue());
	}

	void sliderDragStarted(Slider* slider) override
	{
		if (AudioProcessorParameter* param = getParameterForSlider(slider))
			param->beginChangeGesture();
	}

	void sliderDragEnded(Slider* slider) override
	{
		if (AudioProcessorParameter* param = getParameterForSlider(slider))
			param->endChangeGesture();
	}

private:
	void timerCallback() override
	{
		const OwnedArray<AudioProcessorParameter>& params = getAudioProcessor()->getParameters();
		for (int i = 0; i < params.size(); ++i)
		{
			if (const AudioProcessorParameter* param = params[i])
			{
				if (i < paramControllers.size())
					dynamic_cast<Slider*>(paramControllers[i])->setValue(param->getValue());
			}
		}
	}

	AudioProcessorParameter* getParameterForSlider(Slider* slider)
	{
		const OwnedArray<AudioProcessorParameter>& params = getAudioProcessor()->getParameters();
		return params[paramControllers.indexOf(slider)];
	}

	Label noParameterLabel;
	OwnedArray<Component> paramControllers;
	OwnedArray<Label> paramLabels;
};

#if 0
//==============================================================================
/**
*/
class GainProcessor : public AudioProcessor
{
public:

	//==============================================================================
	GainProcessor()
	{
		addParameter(gain = new AudioParameterFloat("gain", "Gain", 0.0f, 1.0f, 0.5f));
	}

	~GainProcessor() {}

	//==============================================================================
	void prepareToPlay(double /*sampleRate*/, int /*samplesPerBlock*/) override {}
	void releaseResources() override {}

	void processBlock(AudioSampleBuffer& buffer, MidiBuffer&) override
	{
		buffer.applyGain(*gain);
	}

	//==============================================================================
	AudioProcessorEditor* createEditor() override { return new GenericEditor(*this); }
	bool hasEditor() const override               { return true; }

	//==============================================================================
	const String getName() const override               { return "Gain PlugIn"; }
	bool acceptsMidi() const override                   { return false; }
	bool producesMidi() const override                  { return false; }
	double getTailLengthSeconds() const override        { return 0; }

	//==============================================================================
	int getNumPrograms() override                          { return 1; }
	int getCurrentProgram() override                       { return 0; }
	void setCurrentProgram(int) override                  {}
	const String getProgramName(int) override             { return String(); }
	void changeProgramName(int, const String&) override { }

	//==============================================================================
	void getStateInformation(MemoryBlock& destData) override
	{
		MemoryOutputStream(destData, true).writeFloat(*gain);
	}

	void setStateInformation(const void* data, int sizeInBytes) override
	{
		gain->setValueNotifyingHost(MemoryInputStream(data, sizeInBytes, false).readFloat());
	}

	//==============================================================================
	bool setPreferredBusArrangement(bool isInputBus, int busIndex,
		const AudioChannelSet& preferred) override
	{
		const int numChannels = preferred.size();

		// do not allow disabling channels
		if (numChannels == 0) return false;

		// always have the same channel layout on both input and output on the main bus
		if (!AudioProcessor::setPreferredBusArrangement(!isInputBus, busIndex, preferred))
			return false;

		return AudioProcessor::setPreferredBusArrangement(isInputBus, busIndex, preferred);
	}

	static AudioProcessor *create() { return new GainProcessor(); }

private:
	//==============================================================================
	AudioParameterFloat* gain;

	//==============================================================================
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GainProcessor)
};


//==============================================================================
/**
*/
class Spatializer : public AudioProcessor
{
public:

	struct SpeakerPosition
	{
		float radius, phi;
	};

	struct SpeakerLayout
	{
		AudioChannelSet set;
		Array<SpeakerPosition> positions;
	};

	// this needs at least c++11
	static Array<SpeakerLayout> speakerPositions;

	//==============================================================================
	Spatializer() : currentSpeakerLayout(0)
	{
		// clear the default bus arrangements which were created by the base class
		busArrangement.inputBuses.clear();
		busArrangement.outputBuses.clear();

		// add mono in and default out
		busArrangement.inputBuses.add(AudioProcessorBus("Input", AudioChannelSet::mono()));
		busArrangement.outputBuses.add(AudioProcessorBus("Output", speakerPositions[currentSpeakerLayout].set));

		addParameter(radius = new AudioParameterFloat("radius", "Radius", 0.0f, 1.0f, 0.5f));
		addParameter(phi = new AudioParameterFloat("phi", "Phi", 0.0f, 1.0f, 0.0f));
	}

	~Spatializer() {}

	//==============================================================================
	bool setPreferredBusArrangement(bool isInputBus, int busIndex,
		const AudioChannelSet& preferred) override
	{
		// we only allow mono in
		if (isInputBus && preferred != AudioChannelSet::mono())
			return false;

		// the output must be one of the supported speaker layouts
		if (!isInputBus)
		{
			int i;

			for (i = 0; i < speakerPositions.size(); ++i)
				if (speakerPositions[i].set == preferred) break;

			if (i >= speakerPositions.size())
				return false;

			currentSpeakerLayout = i;
		}

		return AudioProcessor::setPreferredBusArrangement(isInputBus, busIndex, preferred);
	}

	//==============================================================================
	void prepareToPlay(double /*sampleRate*/, int samplesPerBlock) override
	{
		scratchBuffer.setSize(1, samplesPerBlock);
	}

	void releaseResources() override {}

	void processBlock(AudioSampleBuffer& buffer, MidiBuffer&) override
	{
		// copy the input into a scratch buffer
		AudioSampleBuffer scratch(scratchBuffer.getArrayOfWritePointers(), 1, buffer.getNumSamples());
		scratch.copyFrom(0, 0, buffer, 0, 0, buffer.getNumSamples());

		const Array<SpeakerPosition>& positions = speakerPositions.getReference(currentSpeakerLayout).positions;
		const float* inputBuffer = scratch.getReadPointer(0);
		const float kMaxDistanceGain = -20.0f;

		for (int speakerIdx = 0; speakerIdx < positions.size(); ++speakerIdx)
		{
			const SpeakerPosition& speakerPos = positions.getReference(speakerIdx);
			float fltDistance = distance(polarToCartesian(speakerPos.radius, speakerPos.phi), polarToCartesian(*radius, (*phi) * 2.0f * float_Pi));
			float gainInDb = kMaxDistanceGain * (fltDistance / 2.0f);
			float gain = std::pow(10.0f, (gainInDb / 20.0f));

			busArrangement.getBusBuffer(buffer, false, 0).copyFrom(speakerIdx, 0, inputBuffer, buffer.getNumSamples(), gain);
		}
	}

	//==============================================================================
	AudioProcessorEditor* createEditor() override { return new GenericEditor(*this); }
	bool hasEditor() const override               { return true; }

	//==============================================================================
	const String getName() const override               { return "Gain PlugIn"; }

	bool acceptsMidi() const override                   { return false; }
	bool producesMidi() const override                  { return false; }
	double getTailLengthSeconds() const override        { return 0; }

	//==============================================================================
	int getNumPrograms() override                          { return 1; }
	int getCurrentProgram() override                       { return 0; }
	void setCurrentProgram(int) override                  {}
	const String getProgramName(int) override             { return String(); }
	void changeProgramName(int, const String&) override { }

	//==============================================================================
	void getStateInformation(MemoryBlock& destData) override
	{
		MemoryOutputStream stream(destData, true);

		stream.writeFloat(*radius);
		stream.writeFloat(*phi);
	}

	void setStateInformation(const void* data, int sizeInBytes) override
	{
		MemoryInputStream stream(data, sizeInBytes, false);

		radius->setValueNotifyingHost(stream.readFloat());
		phi->setValueNotifyingHost(stream.readFloat());
	}

	static AudioProcessor *create() { return new Spatializer(); }

private:
	//==============================================================================
	AudioParameterFloat* radius;
	AudioParameterFloat* phi;
	int currentSpeakerLayout;
	AudioSampleBuffer scratchBuffer;

	static Point<float> polarToCartesian(float r, float phi) noexcept
	{
		return Point<float>(r * std::cos(phi), r * std::sin(phi));
	}

		static float distance(Point<float> a, Point<float> b) noexcept
	{
		return std::sqrt(std::pow(a.x - b.x, 2.0f) + std::pow(a.y - b.y, 2.0f));
	}

		//==============================================================================
		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Spatializer)
};
#endif

} // namespace hise


#endif  // AUDIOPROCESSORWRAPPER_H_INCLUDED
