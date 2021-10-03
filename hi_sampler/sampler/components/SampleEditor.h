/*
  ==============================================================================

  This is an automatically generated GUI class created by the Introjucer!

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Created with Introjucer version: 4.1.0

  ------------------------------------------------------------------------------

  The Introjucer is part of the JUCE library - "Jules' Utility Class Extensions"
  Copyright (c) 2015 - ROLI Ltd.

  ==============================================================================
*/

#ifndef __JUCE_HEADER_A3E3B1B2DF55A952__
#define __JUCE_HEADER_A3E3B1B2DF55A952__

//[Headers]     -- You can add your own extra header files here --
namespace hise {
using namespace juce;

class SamplerBody;
class SampleEditHandler;

//[/Headers]

#include "ValueSettingComponent.h"


//==============================================================================
/**
																	//[Comments]
	\cond HIDDEN_SYMBOLS
	An auto-generated component, created by the Introjucer.

	Describe your class and how it works here!
																	//[/Comments]
*/
class SampleEditor : public Component,
	public SamplerSubEditor,
	public ApplicationCommandTarget,
	public AudioDisplayComponent::Listener,
    public ComboBox::Listener,
	public SafeChangeListener,
	public SampleMap::Listener
{
public:
	//==============================================================================
	SampleEditor(ModulatorSampler *s, SamplerBody *b);
	~SampleEditor();

	void changeListenerCallback(SafeChangeBroadcaster *)
	{
		updateWaveform();
	}

	void updateInterface() override
	{
		updateWaveform();
	}

	bool isInWorkspace() const { return findParentComponentOfClass<ProcessorEditor>() == nullptr; }

	void sampleMapWasChanged(PoolReference r) override
	{
		currentWaveForm->setSoundToDisplay(nullptr);
		updateWaveform();
	}

	void samplePropertyWasChanged(ModulatorSamplerSound* s, const Identifier& id, const var& newValue) override;

	void sampleAmountChanged() override
	{
		if(currentWaveForm->getCurrentSound() == nullptr)
		{
			currentWaveForm->setSoundToDisplay(nullptr);
		}
	}

	AudioSampleBuffer& getGraphBuffer();

	void rangeChanged(AudioDisplayComponent *c, int areaThatWasChanged) override
	{
		SamplerSoundWaveform *waveForm = dynamic_cast<SamplerSoundWaveform*>(c);

		if(waveForm == nullptr) return;

		const ModulatorSamplerSound *currentSound = waveForm->getCurrentSound();

		if(currentSound != nullptr && selection.getLast() == currentSound)
		{
			ModulatorSamplerSound *soundToChange = selection.getLast();

			AudioDisplayComponent::SampleArea *area = c->getSampleArea(areaThatWasChanged);

			const int64 startSample = (int64)jmax<int>(0, (int)area->getSampleRange().getStart());
			const int64 endSample = (int64)(area->getSampleRange().getEnd());

			switch (areaThatWasChanged)
			{
			case SamplerSoundWaveform::SampleStartArea:
			{
				soundToChange->setSampleProperty(SampleIds::SampleStartMod, (int)(endSample - startSample));
				soundToChange->closeFileHandle();
				break;
			}
			case SamplerSoundWaveform::LoopArea:
			{
				if (!area->leftEdgeClicked)
				{
					soundToChange->setSampleProperty(SampleIds::LoopEnd, (int)endSample);
				}
				else
				{
					soundToChange->setSampleProperty(SampleIds::LoopStart, (int)startSample);
				}
				break;
			}
			case SamplerSoundWaveform::PlayArea:
			{
				if (!area->leftEdgeClicked)
				{
					soundToChange->setSampleProperty(SampleIds::SampleEnd, (int)endSample);
				}
				else
				{
					soundToChange->setSampleProperty(SampleIds::SampleStart, (int)startSample);
				}
				break;
			}
			case SamplerSoundWaveform::LoopCrossfadeArea:
			{
				jassert(area->leftEdgeClicked);
				soundToChange->setSampleProperty(SampleIds::LoopXFade, (int)(endSample - startSample));
				break;
			}
			}

			//currentWaveForm->updateRanges();
		}
	};

	/** All application commands are collected here. */
	enum SampleMapCommands
	{
		// Undo / Redo
		ZoomIn = 0x3000,
		ZoomOut,
		EnableSampleStartArea,
		EnableLoopArea,
		EnablePlayArea,
		SelectWithMidi,
		NormalizeVolume,
		LoopEnabled,
		Analyser,
		numCommands
	};

	ApplicationCommandTarget* getNextCommandTarget() override
	{
		return findFirstTargetParentComponent();
	};

	void getAllCommands (Array<CommandID>& commands) override
	{
		const CommandID id[] = {ZoomIn,
								ZoomOut,
								
								EnableSampleStartArea,
								EnableLoopArea,
								EnablePlayArea,
								SelectWithMidi,
								NormalizeVolume,
								LoopEnabled,
								Analyser
								};

		commands.addArray(id, numElementsInArray(id));
	};

	void getCommandInfo (CommandID commandID, ApplicationCommandInfo &result) override
	{
		const bool isSelected = selection.size() > 0 && selection.getLast() != nullptr;

		switch (commandID)
		{
		case ZoomIn:			result.setInfo("Zoom In", "Zoom in the sample map", "Zooming", 0);
								result.addDefaultKeypress('+', ModifierKeys::commandModifier);
								result.setActive(isSelected && zoomFactor != 16.0f);
								break;
		case ZoomOut:			result.setInfo("Zoom Out", "Zoom out the sample map", "Zooming", 0);
								result.addDefaultKeypress('-', ModifierKeys::commandModifier);
								result.setActive(isSelected && zoomFactor != 1.0f);
								break;
		case EnableSampleStartArea:	result.setInfo("Enable SampleStart Dragging", "Enable Sample Start Modulation Area Dragging", "Areas", 0);
									result.setActive(isSelected && (int)selection.getLast()->getSampleProperty(SampleIds::SampleStartMod) != 0);
                                    result.setTicked(isSelected && !currentWaveForm->getSampleArea(SamplerSoundWaveform::AreaTypes::SampleStartArea)->isAreaEnabled());
									break;
		case EnableLoopArea:	result.setInfo("Enable SampleStart Dragging", "Enable Loop Area Dragging", "Areas", 0);
								result.setActive(isSelected && selection.getLast()->getSampleProperty(SampleIds::LoopEnabled));
                                result.setTicked(isSelected && !currentWaveForm->getSampleArea(SamplerSoundWaveform::AreaTypes::LoopArea)->isAreaEnabled());
								break;
		case EnablePlayArea:	result.setInfo("Enable Play Area Dragging", "Enable Playback Area Dragging", "Areas", 0);
								result.setActive(isSelected);
                                result.setTicked(isSelected && !currentWaveForm->getSampleArea(SamplerSoundWaveform::AreaTypes::PlayArea)->isAreaEnabled());
								break;
		case SelectWithMidi:	result.setInfo("Midi Select", "Autoselect the most recently triggered sound", "Tools", 0);
								result.setActive(true);
								result.setTicked(!sampler->getEditorState(ModulatorSampler::MidiSelectActive));
								break;
		case NormalizeVolume:	result.setInfo("Normalize Volume", "Normalize the sample volume to 0dB", "Properties", 0);
								result.setActive(isSelected);
								result.setTicked(isSelected && (int)selection.getLast()->getSampleProperty(SampleIds::Normalized));
								break;
		case LoopEnabled:		result.setInfo("Loop Enabled", "Enable Loop Playback", "Properties", 0);
								result.setActive(isSelected);
								result.setTicked(isSelected && (int)selection.getLast()->getSampleProperty(SampleIds::LoopEnabled));
								break;
		case Analyser:			result.setInfo("Show FFT analyser", "FFT Analyser", "Properties", 0);
								result.setActive(isSelected);
								result.setTicked(isSelected && viewport->isVisible());
								break;
		}
	};

	bool perform (const InvocationInfo &info) override;


	void toggleAnalyser();
	

	void soundsSelected(const SampleSelection  &selectedSoundList) override
	{
		selection.clear();

		for (int i = 0; i < selectedSoundList.size(); i++)
			selection.add(selectedSoundList[i]);

		panSetter->setCurrentSelection(selectedSoundList);
		volumeSetter->setCurrentSelection(selectedSoundList);
		pitchSetter->setCurrentSelection(selectedSoundList);
		sampleStartSetter->setCurrentSelection(selectedSoundList);
		sampleEndSetter->setCurrentSelection(selectedSoundList);
		startModulationSetter->setCurrentSelection(selectedSoundList);
		loopStartSetter->setCurrentSelection(selectedSoundList);
		loopEndSetter->setCurrentSelection(selectedSoundList);
		loopCrossfadeSetter->setCurrentSelection(selectedSoundList);

		samplerEditorCommandManager->commandStatusChanged();

		if(selectedSoundList.size() != 0 && selectedSoundList.getLast() != nullptr) currentWaveForm->setSoundToDisplay(selectedSoundList.getLast());
		else currentWaveForm->setSoundToDisplay(nullptr);

        sampleSelector->clear(dontSendNotification);
        multimicSelector->clear(dontSendNotification);
        int sampleIndex = 1;
        
        for(auto s: selectedSoundList)
        {
            sampleSelector->addItem(s->getSampleProperty(SampleIds::FileName).toString().replace("{PROJECT_FOLDER}", ""), sampleIndex++);
        }
        
        sampleSelector->setSelectedId(selectedSoundList.size(), dontSendNotification);
        
        auto micPositions = StringArray::fromTokens(sampler->getStringForMicPositions(), ";", "");
        micPositions.removeEmptyStrings();
        
        int micIndex = 1;
        
        for(auto t: micPositions)
        {
            multimicSelector->addItem(t, micIndex++);
        }
        
        multimicSelector->setTextWhenNothingSelected("No multimics");
        multimicSelector->setTextWhenNoChoicesAvailable("No multimics");
        
		updateWaveform();
	}

	void paintOverChildren(Graphics &g);

	void updateWaveform();

	void zoom(bool zoomOut)
	{
		if(!zoomOut)
		{
			zoomFactor = jmin(64.0f, zoomFactor * 1.5f); resized();
		}
		else
		{
			zoomFactor = jmax(1.0f, zoomFactor / 1.5f); resized();
		}

		resized();
	}

	void mouseWheelMove	(	const MouseEvent & 	event, const MouseWheelDetails & 	wheel )	override
	{
		if(event.mods.isCtrlDown())
		{
			zoom(wheel.deltaY < 0);
		}
		else
		{
			getParentComponent()->mouseWheelMove(event, wheel);
		}

	};

    void comboBoxChanged(ComboBox* cb) override
    {
        refreshDisplayFromComboBox();
    }
    
    void refreshDisplayFromComboBox()
    {
        auto idx = sampleSelector->getSelectedItemIndex();
        
        if(auto s = selection[idx])
        {
            currentWaveForm->setSoundToDisplay(s);
        }
    }
    
    
    //[/UserMethods]

    void paint (Graphics& g);
    void resized();

	void addButton(CommandID commandId, bool hasState, const String& name, const String& offName = String());

private:
    //[UserVariables]   -- You can add your own custom variables in this section.
	friend class SampleEditorToolbarFactory;

	struct GraphHandler : public AsyncUpdater
	{
		GraphHandler(SampleEditor& parent_) :
			parent(parent_)
		{};

		void handleAsyncUpdate()
		{
			parent.graph->refreshDisplayedBuffer();
		}

		void rebuildIfDirty()
		{
			auto cs = parent.currentWaveForm->getCurrentSound();
			if (lastSound != cs)
			{
				lastSound = const_cast<ModulatorSamplerSound*>(cs);
				triggerAsyncUpdate();
			}
		};

		SampleEditor& parent;
		AudioSampleBuffer graphBuffer;
		ModulatorSamplerSound::WeakPtr lastSound;
	} graphHandler;

	

	float zoomFactor;

	ModulatorSampler *sampler;
	SamplerBody	*body;

    Slider spectrumSlider;
    
	ScrollbarFader fader;
	ScrollbarFader::Laf laf;

	ScopedPointer<snex::ui::Graph> graph;

	ScopedPointer<Component> viewContent;

	ScopedPointer<SamplerSoundWaveform> currentWaveForm;

	ReferenceCountedArray<ModulatorSamplerSound> selection;

	ScopedPointer<SampleEditorToolbarFactory> toolbarFactory;

	ScopedPointer<ApplicationCommandManager> samplerEditorCommandManager;

	OwnedArray<HiseShapeButton> menuButtons;

    //[/UserVariables]

    //==============================================================================
    ScopedPointer<Viewport> viewport;
    ScopedPointer<ValueSettingComponent> volumeSetter;
    ScopedPointer<ValueSettingComponent> pitchSetter;
    ScopedPointer<ValueSettingComponent> sampleStartSetter;
    ScopedPointer<ValueSettingComponent> sampleEndSetter;
    ScopedPointer<ValueSettingComponent> loopStartSetter;
    ScopedPointer<ValueSettingComponent> loopEndSetter;
    ScopedPointer<ValueSettingComponent> loopCrossfadeSetter;
    ScopedPointer<ValueSettingComponent> startModulationSetter;
    ScopedPointer<Toolbar> toolbar;
    ScopedPointer<ValueSettingComponent> panSetter;

	ScopedPointer<Component> verticalZoomer;
    ScopedPointer<ComboBox> sampleSelector;
    ScopedPointer<ComboBox> multimicSelector;
    
    GlobalHiseLookAndFeel claf;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SampleEditor)
};

//[EndFile] You can add extra defines here...
/** \endcond */
} // namespace hise

//[/EndFile]

#endif   // __JUCE_HEADER_A3E3B1B2DF55A952__
