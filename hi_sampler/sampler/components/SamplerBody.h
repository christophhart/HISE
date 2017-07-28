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

#ifndef __JUCE_HEADER_D6854B5362E0277C__
#define __JUCE_HEADER_D6854B5362E0277C__

//[Headers]     -- You can add your own extra header files here --
 


//[/Headers]

#include "SamplerSettings.h"
#include "SampleMapEditor.h"


//==============================================================================
/**
                                                                    //[Comments]
    \cond HIDDEN_SYMBOLS
	An auto-generated component, created by the Introjucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class SamplerBody  : public ProcessorEditorBody,
                     public ButtonListener,
					 public SampleEditHandler::Listener
{
public:
    //==============================================================================
    SamplerBody (ProcessorEditor *p);
    ~SamplerBody();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.

	void updateGui() override
	{
		if (!dynamic_cast<ModulatorSampler*>(getProcessor())->shouldUpdateUI()) return;

		getSampleEditHandler()->handleMidiSelection();

		settingsPanel->updateInterface();
		sampleEditor->updateInterface();
		soundTable->updateInterface();
		map->updateInterface();
	};


	/** This is called whenever the selection changes.
	*
	*	Since the SamplerBody itself is a ChangeBroadcaster for updateGui(), it has to use another callback
	*/

	void soundSelectionChanged(SampleSelection& newSelection) override
	{
		sampleEditor->selectSounds(newSelection);

		map->selectSounds(newSelection);

		soundTable->selectSounds(newSelection);
	}

	SampleEditHandler* getSampleEditHandler()
	{
		return dynamic_cast<ModulatorSampler*>(getProcessor())->getSampleEditHandler();
	}


	int getBodyHeight() const override
	{
		const bool bigMap = getProcessor()->getEditorState(getProcessor()->getEditorStateForIndex(ModulatorSampler::EditorStates::BigSampleMap));

		const int thisMapHeight = (bigMap && mapHeight != 0) ? mapHeight + 128 : mapHeight;

		return h + (settingsHeight != 0 ? settingsPanel->getPanelHeight() : 0) + waveFormHeight + thisMapHeight + tableHeight;
	};


	

	bool keyPressed(const KeyPress &k) override
	{
		if (int commandId = map->getCommandManager()->getKeyMappings()->findCommandForKeyPress(k))
		{
			map->getCommandManager()->invokeDirectly(commandId, false);
			return true;
		}

		if (k.getModifiers().isShiftDown())
		{
			if (k.getKeyCode() == k.upKey)
			{
				int index = map->getCurrentRRGroup();

				if (index == -1) index = 0;

				index++;

				map->setCurrentRRGroup(index);

				return true;
			}
			else if (k.getKeyCode() == k.downKey)
			{
				int index = map->getCurrentRRGroup();

				index--;

				map->setCurrentRRGroup(index);

				return true;
			}
		}
		if (k.getKeyCode() == KeyPress::leftKey)
		{
			if (k.getModifiers().isCommandDown())
				getSampleEditHandler()->moveSamples(SamplerSoundMap::Left);
			else
				map->getMapComponent()->selectNeighbourSample(SamplerSoundMap::Left);
			return true;
		}
		else if (k.getKeyCode() == KeyPress::rightKey)
		{
			if (k.getModifiers().isCommandDown())
				getSampleEditHandler()->moveSamples(SamplerSoundMap::Right);
			else
				map->getMapComponent()->selectNeighbourSample(SamplerSoundMap::Right);
			return true;
		}
		else if (k.getKeyCode() == KeyPress::upKey)
		{
			if (k.getModifiers().isCommandDown())
				getSampleEditHandler()->moveSamples(SamplerSoundMap::Up);
			else
				map->getMapComponent()->selectNeighbourSample(SamplerSoundMap::Up);
			return true;
		}
		else if (k.getKeyCode() == KeyPress::downKey)
		{
			if (k.getModifiers().isCommandDown())
				getSampleEditHandler()->moveSamples(SamplerSoundMap::Down);
			else
				map->getMapComponent()->selectNeighbourSample(SamplerSoundMap::Down);
			return true;
		}

		return false;

	};

    //[/UserMethods]

    void paint (Graphics& g);
    void resized();
    void buttonClicked (Button* buttonThatWasClicked);



private:
    //[UserVariables]   -- You can add your own custom variables in this section.
	int h;
	int settingsHeight;
	int waveFormHeight;
	int mapHeight;
	int tableHeight;

	bool internalChange;
	uint32 timeSinceLastSelectionChange = 0;

#if SAMPLER_DEPRECATED
	ScopedPointer<SelectionListener> selectionListener;
#endif

	ChainBarButtonLookAndFeel cblaf;

    //[/UserVariables]

    //==============================================================================
    ScopedPointer<SampleEditor> sampleEditor;
    ScopedPointer<SamplerTable> soundTable;
    ScopedPointer<Component> buttonRow;
    ScopedPointer<TextButton> waveFormButton;
    ScopedPointer<TextButton> mapButton;
    ScopedPointer<TextButton> tableButton;
    ScopedPointer<TextButton> settingsView;
    ScopedPointer<SamplerSettings> settingsPanel;
    ScopedPointer<SampleMapEditor> map;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SamplerBody)
};

//[EndFile] You can add extra defines here...
/** \endcond */
//[/EndFile]

#endif   // __JUCE_HEADER_D6854B5362E0277C__
