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

struct ExternalFileChangeWatcher;
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
	public SampleMap::Listener,
	public ScrollBar::Listener
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

	void scrollBarMoved(ScrollBar* scrollBarThatHasMoved, double newRangeStart) override;

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
		ExternalEditor,
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
								Analyser,
								ExternalEditor,
								};

		commands.addArray(id, numElementsInArray(id));
	};

	void getCommandInfo (CommandID commandID, ApplicationCommandInfo &result) override;;

	bool perform (const InvocationInfo &info) override;


	void toggleAnalyser();
	

	void soundsSelected(const SampleSelection  &selectedSoundList) override;

	void paintOverChildren(Graphics &g);

	void updateWaveform();

	void zoom(bool zoomOut, int mousePos=0);

	void mouseWheelMove	(const MouseEvent & e, const MouseWheelDetails & 	wheel )	override
	{
		if(e.mods.isCtrlDown())
			zoom(wheel.deltaY < 0, e.getEventRelativeTo(viewport).getPosition().getX());
		else
			getParentComponent()->mouseWheelMove(e, wheel);
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

	Component* addButton(CommandID commandId, bool hasState, const String& name, const String& offName = String());

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

	Component* analyseButton;
	Component* externalButton;

	LookAndFeel_V4 slaf;

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
	ScopedPointer<ExternalFileChangeWatcher> externalWatcher;

	ScopedPointer<Component> verticalZoomer;
    ScopedPointer<ComboBox> sampleSelector;
    ScopedPointer<ComboBox> multimicSelector;
    
	HiseAudioThumbnail overview;

    GlobalHiseLookAndFeel claf;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SampleEditor)
};

//[EndFile] You can add extra defines here...
/** \endcond */
} // namespace hise

//[/EndFile]

#endif   // __JUCE_HEADER_A3E3B1B2DF55A952__
