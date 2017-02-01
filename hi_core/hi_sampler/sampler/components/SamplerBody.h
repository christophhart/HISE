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
#include "JuceHeader.h"


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
                     public ButtonListener
{
public:
    //==============================================================================
    SamplerBody (ProcessorEditor *p);
    ~SamplerBody();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.

	class SelectionListener: public ChangeListener
	{
	public:

		SelectionListener(SamplerBody *body_):
			body(body_)
		{}

		void changeListenerCallback(ChangeBroadcaster *)
		{
			const Array<WeakReference<ModulatorSamplerSound>> newSelection = body->getSelection().getItemArray();

			Array<ModulatorSamplerSound*> existingSounds;

			for(int i = 0; i < newSelection.size(); i++)
			{
				if(newSelection[i].get() != nullptr) existingSounds.add(newSelection[i].get());
			}

			if (existingSounds != lastSelection)
			{
				body->soundSelectionChanged();
				lastSelection.clear();
				lastSelection.addArray(existingSounds);
			}
		}

	private:

		Array<ModulatorSamplerSound*> lastSelection;

		SamplerBody *body;
	};



	bool newKeysPressed(const uint8 *currentNotes)
	{
		for(int i = 0; i < 127; i++)
		{
			if(currentNotes[i] != 0) return true;
		}
		return false;
	}

	void updateGui() override
	{
		if (!dynamic_cast<ModulatorSampler*>(getProcessor())->shouldUpdateUI()) return;

		const ModulatorSampler::SamplerDisplayValues x = dynamic_cast<ModulatorSampler*>(getProcessor())->getSamplerDisplayValues();

		settingsPanel->updateGui();

		sampleEditor->updateWaveform();

		if(getProcessor()->getEditorState(ModulatorSampler::MidiSelectActive) && newKeysPressed(x.currentNotes))
		{
			selectedSamplerSounds.deselectAll();

			SelectedItemSet<const ModulatorSamplerSound*> midiSounds;

			ModulatorSampler *sampler = dynamic_cast<ModulatorSampler*>(getProcessor());

			for(int i = 0; i < 127; i++)
			{
				if(x.currentNotes[i] != 0)
				{
					const int noteNumber = i;
					const int velocity = x.currentNotes[i];

					for(int j = 0; j < sampler->getNumSounds(); j++)
					{
						if (sampler->soundCanBePlayed(sampler->getSound(j), 1, noteNumber, (float)velocity / 127.0f))
						{
							selectedSamplerSounds.addToSelection(sampler->getSound(j));
						}
					}

				}
			}
		}

		soundTable->refreshList();

		map->setPressedKeys(x.currentNotes);

		map->updateSoundData();
	};

	struct SampleEditingActions
	{
		static void deleteSelectedSounds(SamplerBody *body);
		static void duplicateSelectedSounds(SamplerBody *body);
		static void removeDuplicateSounds(SamplerBody *body);
		static void cutSelectedSounds(SamplerBody *body);
		static void copySelectedSounds(SamplerBody *body);
		static void automapVelocity(SamplerBody *body);
		static void pasteSelectedSounds(SamplerBody *body);

		static void checkMicPositionAmountBeforePasting(const ValueTree &v, ModulatorSampler * s);

		static void refreshCrossfades(SamplerBody * body);
		static void selectAllSamples(SamplerBody * body);
		static void mergeIntoMultiSamples(SamplerBody * body);
		static void extractToSingleMicSamples(SamplerBody * body);
		static void normalizeSamples(SamplerBody *body);
		static void automapUsingMetadata(SamplerBody * body, ModulatorSampler* sampler);
		static void trimSampleStart(SamplerBody * body);
	};


	/** This is called whenever the selection changes.
	*
	*	Since the SamplerBody itself is a ChangeBroadcaster for updateGui(), it has to use another callback
	*/
	void soundSelectionChanged()
	{
		const uint32 thisTime = Time::getMillisecondCounter();
		const uint32 interval = 0;

		if ((thisTime - timeSinceLastSelectionChange) > interval)
		{
			const Array<WeakReference<ModulatorSamplerSound>> sounds = selectedSamplerSounds.getItemArray();

			Array<ModulatorSamplerSound*> existingSounds;

			for (int i = 0; i < sounds.size(); i++)
			{
				if (sounds[i].get() != nullptr) existingSounds.add(sounds[i].get());
			}

			sampleEditor->selectSounds(existingSounds);

			map->selectSounds(existingSounds);

			soundTable->selectSounds(existingSounds);

			timeSinceLastSelectionChange = Time::getMillisecondCounter();
		}
	};

	SelectedItemSet<WeakReference<ModulatorSamplerSound>> &getSelection()
	{
		return selectedSamplerSounds;
	}

	int getBodyHeight() const override
	{
		const bool bigMap = getProcessor()->getEditorState(getProcessor()->getEditorStateForIndex(ModulatorSampler::EditorStates::BigSampleMap));

		const int thisMapHeight = (bigMap && mapHeight != 0) ? mapHeight + 128 : mapHeight;

		return h + (settingsHeight != 0 ? settingsPanel->getPanelHeight() : 0) + waveFormHeight + thisMapHeight + tableHeight;
	};

	void changeProperty(ModulatorSamplerSound *s, ModulatorSamplerSound::Property p, int delta)
	{
		const int v = s->getProperty(p);

		s->setPropertyWithUndo(p, v + delta);
	};

	void moveSamples(SamplerSoundMap::Neighbour direction)
	{
		ModulatorSampler *s = dynamic_cast<ModulatorSampler*>(getProcessor());

		s->getUndoManager()->beginNewTransaction("Moving Samples");

		switch(direction)
		{
		case SamplerSoundMap::Right:
		case SamplerSoundMap::Left:
		{
			for(int i = 0; i < selectedSamplerSounds.getNumSelected(); i++)
			{
				ModulatorSamplerSound *sound = selectedSamplerSounds.getSelectedItem(i);

				if(direction == SamplerSoundMap::Right)
				{
					changeProperty(sound, ModulatorSamplerSound::KeyHigh, 1);
					changeProperty(sound, ModulatorSamplerSound::KeyLow, 1);
					changeProperty(sound, ModulatorSamplerSound::RootNote, 1);
				}
				else
				{
					changeProperty(sound, ModulatorSamplerSound::KeyLow, -1);
					changeProperty(sound, ModulatorSamplerSound::KeyHigh, -1);
					changeProperty(sound, ModulatorSamplerSound::RootNote, -1);
				}
			}
			break;
		}
		case SamplerSoundMap::Up:
		case SamplerSoundMap::Down:
		{
			for(int i = 0; i < selectedSamplerSounds.getNumSelected(); i++)
			{
				ModulatorSamplerSound *sound = selectedSamplerSounds.getSelectedItem(i);

				changeProperty(sound, ModulatorSamplerSound::VeloHigh, direction == SamplerSoundMap::Up ? 1 : -1);
				changeProperty(sound, ModulatorSamplerSound::VeloLow, direction == SamplerSoundMap::Up ? 1 : -1);
			}
			break;
		}
		}
	}

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
		if(k.getKeyCode() == KeyPress::leftKey)
		{
			if(k.getModifiers().isCommandDown())
				moveSamples(SamplerSoundMap::Left);
			else
				map->getMapComponent()->selectNeighbourSample(SamplerSoundMap::Left);
			return true;
		}
		else if (k.getKeyCode() == KeyPress::rightKey)
		{
			if(k.getModifiers().isCommandDown())
				moveSamples(SamplerSoundMap::Right);
			else
				map->getMapComponent()->selectNeighbourSample(SamplerSoundMap::Right);
			return true;
		}
		else if (k.getKeyCode() == KeyPress::upKey)
		{
			if(k.getModifiers().isCommandDown())
				moveSamples(SamplerSoundMap::Up);
			else
				map->getMapComponent()->selectNeighbourSample(SamplerSoundMap::Up);
			return true;
		}
		else if( k.getKeyCode() == KeyPress::downKey)
		{
			if(k.getModifiers().isCommandDown())
				moveSamples(SamplerSoundMap::Down);
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

	SelectedItemSet<WeakReference<ModulatorSamplerSound>> selectedSamplerSounds;

	ScopedPointer<SelectionListener> selectionListener;

	ChainBarButtonLookAndFeel cblaf;

	uint32 timeSinceLastSelectionChange;

    //[/UserVariables]

    //==============================================================================
    ScopedPointer<SampleEditor> sampleEditor;
    ScopedPointer<SamplerTable> soundTable;
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
