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

//[Headers] You can add your own extra header files here...
//[/Headers]

#include "SampleMapEditor.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
SampleMapEditor::SampleMapEditor (ModulatorSampler *s, SamplerBody *b):
	SamplerSubEditor(s->getSampleEditHandler()),
	sampler(s)
{
    //[Constructor_pre] You can add your own custom stuff here..

	popoutMode = false;

    //[/Constructor_pre]

    addAndMakeVisible (rootNoteSetter = new ValueSettingComponent());
    addAndMakeVisible (lowKeySetter = new ValueSettingComponent());
    addAndMakeVisible (highKeySetter = new ValueSettingComponent());
    addAndMakeVisible (lowVelocitySetter = new ValueSettingComponent());
    addAndMakeVisible (highVelocitySetter = new ValueSettingComponent());
    addAndMakeVisible (rrGroupSetter = new ValueSettingComponent());
    addAndMakeVisible (displayGroupLabel = new Label ("new label",
                                                      TRANS("Display Group")));
    displayGroupLabel->setFont (Font ("Khmer UI", 13.00f, Font::plain));
    displayGroupLabel->setJustificationType (Justification::centredLeft);
    displayGroupLabel->setEditable (false, false, false);
    displayGroupLabel->setColour (Label::textColourId, Colours::white);
    displayGroupLabel->setColour (TextEditor::textColourId, Colours::black);
    displayGroupLabel->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (groupDisplay = new PopupLabel ("new label",
                                                      TRANS("All")));
    groupDisplay->setFont (Font ("Khmer UI", 14.00f, Font::plain));
    groupDisplay->setJustificationType (Justification::centred);
    groupDisplay->setEditable (true, true, false);
    groupDisplay->setColour (Label::backgroundColourId, Colour (0x38ffffff));
    groupDisplay->setColour (Label::outlineColourId, Colour (0x38ffffff));
    groupDisplay->setColour (TextEditor::textColourId, Colours::black);
    groupDisplay->setColour (TextEditor::backgroundColourId, Colour (0x00000000));
    groupDisplay->setColour (TextEditor::highlightColourId, Colour (0x407a0000));
    groupDisplay->addListener (this);

    addAndMakeVisible (viewport = new Viewport ("new viewport"));
    viewport->setScrollBarsShown (false, true);
    viewport->setScrollBarThickness (12);
    viewport->setViewedComponent (new MapWithKeyboard (sampler));

    addAndMakeVisible (toolbar = new Toolbar());
    toolbar->setName ("new component");


    //[UserPreSize]

	groupDisplay->setFont (GLOBAL_FONT());
	displayGroupLabel->setFont (GLOBAL_FONT());

	selectionIsNotEmpty = false;
	zoomFactor = 1.0f;

	sampleMapEditorCommandManager = new ApplicationCommandManager();

	getCommandManager()->registerAllCommandsForTarget(this);
	sampleMapEditorCommandManager->getKeyMappings()->resetToDefaultMappings();

	sampleMapEditorCommandManager->setFirstCommandTarget(this);
    
	toolbarFactory = new SampleMapEditorToolbarFactory(this);

	toolbar->setStyle(Toolbar::ToolbarItemStyle::iconsOnly);
	toolbar->addDefaultItems(*toolbarFactory);

	toolbar->setColour(Toolbar::ColourIds::backgroundColourId, Colours::transparentBlack);
	toolbar->setColour(Toolbar::ColourIds::buttonMouseOverBackgroundColourId, Colours::white.withAlpha(0.3f));
	toolbar->setColour(Toolbar::ColourIds::buttonMouseDownBackgroundColourId, Colours::white.withAlpha(0.4f));

	map = dynamic_cast<MapWithKeyboard*>(viewport->getViewedComponent());

	

	groupDisplay->setEditable(false);

	rrGroupSetter->setPropertyType(ModulatorSamplerSound::RRGroup);
	rootNoteSetter->setPropertyType(ModulatorSamplerSound::RootNote);
	lowKeySetter->setPropertyType(ModulatorSamplerSound::KeyLow);
	highKeySetter->setPropertyType(ModulatorSamplerSound::KeyHigh);
	lowVelocitySetter->setPropertyType(ModulatorSamplerSound::VeloLow);
	highVelocitySetter->setPropertyType(ModulatorSamplerSound::VeloHigh);

	setWantsKeyboardFocus(true);

	body = b;

	verticalBigSize = sampler->getEditorState(sampler->getEditorStateForIndex(ModulatorSampler::EditorStates::BigSampleMap));

	map->setBounds(0, 0, 768, (verticalBigSize ? 256 : 128) + 32);
    
    map->addMouseListener(this, true);

	

    //[/UserPreSize]

    setSize (800, 245);


    //[Constructor] You can add your own custom stuff here..

	getCommandManager()->commandStatusChanged();

    //[/Constructor]
}

SampleMapEditor::~SampleMapEditor()
{
    //[Destructor_pre]. You can add your own custom destruction code here..

	if (getCommandManager()->getFirstCommandTarget(CopySamples))
	{
		getCommandManager()->setFirstCommandTarget(nullptr);
	}
    //[/Destructor_pre]

    rootNoteSetter = nullptr;
    lowKeySetter = nullptr;
    highKeySetter = nullptr;
    lowVelocitySetter = nullptr;
    highVelocitySetter = nullptr;
    rrGroupSetter = nullptr;
    displayGroupLabel = nullptr;
    groupDisplay = nullptr;
    viewport = nullptr;
    toolbar = nullptr;


    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void SampleMapEditor::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..

    int x = 0;
    int y = 2;
    int width = getWidth();
    int height = getHeight()-4;
    
	Rectangle<int> a(x, y, width, height);

	ProcessorEditorLookAndFeel::drawShadowBox(g, a, JUCE_LIVE_CONSTANT_OFF(Colour(0xff333333)));

	//g.drawRect(x, y, width, height);

    //[/UserPrePaint]

    g.setColour (JUCE_LIVE_CONSTANT_OFF(Colour (0x13ffffff)));
    g.fillRect (8, 8, getWidth() - 132, 24);

    g.setColour (Colour (0x0fffffff));
    g.drawRect (8, 8, getWidth() - 132, 24, 1);

    g.setColour (Colour (0xccffffff));
    g.setFont (GLOBAL_BOLD_FONT().withHeight(22.0f));
    g.drawText (TRANS("MAP EDITOR"),
                getWidth() - 12 - 244, 5, 244, 30,
                Justification::centredRight, true);

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void SampleMapEditor::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    rootNoteSetter->setBounds ((getWidth() / 2) + -107 - (90 / 2), 206, 90, 32);
    lowKeySetter->setBounds ((getWidth() / 2) + -11 - (90 / 2), 206, 90, 32);
    highKeySetter->setBounds ((getWidth() / 2) + 85 - (90 / 2), 206, 90, 32);
    lowVelocitySetter->setBounds ((getWidth() / 2) + 181 - (90 / 2), 206, 90, 32);
    highVelocitySetter->setBounds ((getWidth() / 2) + 277 - (90 / 2), 206, 90, 32);
    rrGroupSetter->setBounds ((getWidth() / 2) + -196 - (76 / 2), 206, 76, 32);
    displayGroupLabel->setBounds ((getWidth() / 2) + -281 - (82 / 2), 201, 82, 24);
    groupDisplay->setBounds ((getWidth() / 2) + -281 - (80 / 2), 221, 80, 16);
    viewport->setBounds ((getWidth() / 2) - (640 / 2), 42, 640, 160);
    toolbar->setBounds (12, 10, getWidth() - 140, 20);
    //[UserResized] Add your own custom resize handling here..

	int viewportHeight = verticalBigSize ? 256 + 32 : 128 + 32;




	if (popoutMode)
	{
		viewportHeight = 0;

		while (viewportHeight + 128 + 90 < getHeight())
		{
			viewportHeight += 128;
		}

		int viewportWidth = 0;

		while (viewportWidth + 128 < getWidth())
		{
			viewportWidth += 128;
		}

		viewportHeight += 32;

		map->setSize(viewportWidth, viewportHeight);

		viewport->setBounds((getWidth() / 2) - (viewportWidth / 2), 42, viewportWidth, viewportHeight);
	}
	else
	{
		viewport->setBounds((getWidth() / 2) - (768 / 2), 42, 768, viewportHeight);
	}

	rootNoteSetter->setBounds((getWidth() / 2) + -107 - (90 / 2), viewportHeight + 46, 90, 32);
	lowKeySetter->setBounds((getWidth() / 2) + -11 - (90 / 2), viewportHeight + 46, 90, 32);
	highKeySetter->setBounds((getWidth() / 2) + 85 - (90 / 2), viewportHeight + 46, 90, 32);
	lowVelocitySetter->setBounds((getWidth() / 2) + 181 - (90 / 2), viewportHeight + 46, 90, 32);
	highVelocitySetter->setBounds((getWidth() / 2) + 277 - (90 / 2), viewportHeight + 46, 90, 32);
	rrGroupSetter->setBounds((getWidth() / 2) + -196 - (76 / 2), viewportHeight + 46, 76, 32);
	displayGroupLabel->setBounds((getWidth() / 2) + -281 - (82 / 2), viewportHeight + 41, 82, 24);
	groupDisplay->setBounds((getWidth() / 2) + -281 - (80 / 2), viewportHeight + 61, 80, 16);

	toolbar->setBounds(12, 10, 708, 20);




    //[/UserResized]
}

void SampleMapEditor::labelTextChanged (Label* labelThatHasChanged)
{
    //[UserlabelTextChanged_Pre]
    //[/UserlabelTextChanged_Pre]

    if (labelThatHasChanged == groupDisplay)
    {
        //[UserLabelCode_groupDisplay] -- add your label text handling code here..

		int index = getCurrentRRGroup();

		map->map->soloGroup(index);
        //[/UserLabelCode_groupDisplay]
    }

    //[UserlabelTextChanged_Post]
    //[/UserlabelTextChanged_Post]
}



//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...


void SampleMapEditor::getCommandInfo(CommandID commandID, ApplicationCommandInfo &result)
{
	switch (commandID)
	{
	case ZoomIn:			result.setInfo("Zoom In", "Zoom in the sample map", "Zooming", 0);
		result.addDefaultKeypress('+', ModifierKeys::commandModifier);
		result.setActive(zoomFactor != 6.0f);
		break;
	case ZoomOut:			result.setInfo("Zoom Out", "Zoom out the sample map", "Zooming", 0);
		result.addDefaultKeypress('-', ModifierKeys::commandModifier);
		result.setActive(zoomFactor != 1.0f);
		break;
	case ToggleVerticalSize:result.setInfo("Toggle Vertical Size", "Toggle vertical size", "Zooming", 0);
		result.setActive(dynamic_cast<SamplerBody*>(getParentComponent()) != nullptr);
		break;
	case PopOutMap:			result.setInfo("Show Map Editor in popup", "Show Map Editor in popup", "Zooming", 0);
		result.setActive(dynamic_cast<SamplerBody*>(getParentComponent()) != nullptr);
		break;
	case NewSampleMap:		result.setInfo("New SampleMap", "Create a new SampleMap", "SampleMap Handling", 0);
		result.addDefaultKeypress('n', ModifierKeys::commandModifier);
		break;
	case LoadSampleMap:		result.setInfo("Load SampleMap", "Load a SampleMap from disk.", "SampleMap Handling", 0);
		result.addDefaultKeypress('l', ModifierKeys::commandModifier);
		break;
	case SaveSampleMap:		result.setInfo("Save SampleMap", "Save the current SampleMap", "SampleMap Handling", 0);
		result.addDefaultKeypress('s', ModifierKeys::commandModifier);
		result.setActive(true);
		break;
	case SaveSampleMapAsXml:	result.setInfo("Save as XML", "Save the current SampleMap as XML file", "SampleMap Handling", 0);
		result.setActive(true);
		break;
	case SaveSampleMapAsMonolith:	result.setInfo("Save as Monolith", "Save the current SampleMap as one big monolith file", "SampleMap Handling", 0);
		result.setActive(true);
		break;
	case ImportSfz:		result.setInfo("Import SFZ file format", "Import SFZ file format", "SampleMap Handling", 0);
		result.setActive(true);
		break;
	case ImportFiles:		result.setInfo("Import samples", "Import new audio files", "SampleMap Handling", 0);
		result.addDefaultKeypress('i', ModifierKeys::commandModifier);
		result.setActive(true);
		break;
	case DuplicateSamples:	result.setInfo("Duplicate", "Duplicate all selected samples", "Sample Editing", 0);
		result.addDefaultKeypress('d', ModifierKeys::commandModifier);
		result.setActive(selectionIsNotEmpty);
		break;
	case DeleteDuplicateSamples:	result.setInfo("Delete duplicate samples", "Delete all duplicate samples (Can't be undone)?", "Sample Editing", 0);
		result.setActive(true);
		break;
	case CutSamples:		result.setInfo("Cut", "Cut selected samples", "Sample Editing", 0);
		result.addDefaultKeypress('x', ModifierKeys::commandModifier);
		result.setActive(selectionIsNotEmpty);
		break;
	case CopySamples:		result.setInfo("Copy", "Copy samples to clipboard", "Sample Editing", 0);
		result.addDefaultKeypress('c', ModifierKeys::commandModifier);
		result.setActive(selectionIsNotEmpty);
		break;
	case PasteSamples:		result.setInfo("Paste", "Paste samples from clipboard", "Sample Editing", 0);
		result.addDefaultKeypress('v', ModifierKeys::commandModifier);
		result.setActive(sampler->getMainController()->getSampleManager().clipBoardContainsData());
		break;
	case DeleteSamples:		result.setInfo("Delete", "Delete all selected samples", "Sample Editing", 0);
		result.addDefaultKeypress(KeyPress::deleteKey, ModifierKeys::noModifiers);
		result.setActive(selectionIsNotEmpty);
		break;
	case SelectAllSamples:	result.setInfo("Select all Samples", "Select all Samples", "Sample Editing", 0);
		result.addDefaultKeypress('a', ModifierKeys::commandModifier);
		result.setActive(true);
		break;
	case MergeIntoMultisamples:
		result.setInfo("Merge into Multimic samples", "Merge into Multimic samples", "Sample Editing", 0);
		result.setActive(selectionIsNotEmpty);
		break;
	case ExtractToSingleMicSamples:
		result.setInfo("Extract to Singlemic samples", "Extract to Singlemic samples", "Sample Editing", 0);
		result.setActive(selectionIsNotEmpty);
		break;
	case FillNoteGaps:		result.setInfo("Fill Note Gaps", "Fill note gaps in SampleMap", "Sample Editing", 0);
		result.setActive(selectionIsNotEmpty);
		break;
	case FillVelocityGaps:	result.setInfo("Fill Velocity Gaps", "Fill velocity gaps in SampleMap", "Sample Editing", 0);
		result.setActive(selectionIsNotEmpty);
		break;
	case AutomapVelocity:	{
		result.setInfo("Automap Velocity", "Sort the sounds along the velocity range according to their volume", "Sample Editing", 0);

		bool containsVelocityMaterial = false;

		if (selectedSoundList.size() > 1)
		{

			containsVelocityMaterial = true;

			int lowLimit = selectedSoundList[0]->getProperty(ModulatorSamplerSound::KeyLow);
			int highLimit = selectedSoundList[0]->getProperty(ModulatorSamplerSound::KeyHigh);

			for (int i = 1; i < selectedSoundList.size(); i++)
			{
				if (lowLimit != (int)selectedSoundList[i]->getProperty(ModulatorSamplerSound::KeyLow) ||
					highLimit != (int)selectedSoundList[i]->getProperty(ModulatorSamplerSound::KeyHigh))
				{
					containsVelocityMaterial = false;
					break;
				}

			}


		}

		result.setActive(selectionIsNotEmpty && containsVelocityMaterial);
		break;

	}
	case RefreshVelocityXFade:	result.setInfo("Refresh Velocity Crossfades.", "Adds a crossfade to overlapping sounds in a group.", "Sample Editing", 0);
		result.setActive(selectionIsNotEmpty);
		break;
	case AutomapUsingMetadata:  result.setInfo("Automap using Metadata", "Automaps the sample using the metadata that is found in the sample file.", "Sample Editing", 0);
		result.setActive(selectionIsNotEmpty);
		break;
	case TrimSampleStart: result.setInfo("Trim Sample Start", "Removes the silence at the beginning of samples", "Sample Editing", 0);
		result.setActive(selectionIsNotEmpty);
		break;
	}


}



bool SampleMapEditor::perform (const InvocationInfo &info)
{
	switch(info.commandID)
	{
	case DuplicateSamples:	SampleEditHandler::SampleEditingActions::duplicateSelectedSounds(handler); return true;
	case DeleteDuplicateSamples: SampleEditHandler::SampleEditingActions::removeDuplicateSounds(handler); return true;
	case DeleteSamples:		SampleEditHandler::SampleEditingActions::deleteSelectedSounds(handler); return true;
	case CutSamples:		SampleEditHandler::SampleEditingActions::cutSelectedSounds(handler); return true;
	case CopySamples:		SampleEditHandler::SampleEditingActions::copySelectedSounds(handler); return true;
	case PasteSamples:		SampleEditHandler::SampleEditingActions::pasteSelectedSounds(handler); return true;
	case SelectAllSamples:	SampleEditHandler::SampleEditingActions::selectAllSamples(handler); return true;
	case MergeIntoMultisamples:		SampleEditHandler::SampleEditingActions::mergeIntoMultiSamples(handler, this); return true;
	case ExtractToSingleMicSamples:	SampleEditHandler::SampleEditingActions::extractToSingleMicSamples(handler); return true;
	case ZoomIn:			zoom(false); return true;
	case ZoomOut:			zoom(true); return true;
	case ToggleVerticalSize:toggleVerticalSize(); return true;
	case PopOutMap:			popoutMap(); return true;
	case NewSampleMap:		if(PresetHandler::showYesNoWindow("Clear Sample Map", "Do you want to clear the sample map?", PresetHandler::IconType::Question)) 
								sampler->clearSampleMap(); return true;
	case LoadSampleMap:		{
							FileChooser f("Load new samplemap", GET_PROJECT_HANDLER(sampler).getSubDirectory(ProjectHandler::SubDirectories::SampleMaps), "*.xml;*.m5p");
							if(f.browseForFileToOpen())
							{
								sampler->loadSampleMap(f.getResult());
							}
							return true;
							}
	case SaveSampleMap:				sampler->saveSampleMap(); return true;
	case SaveSampleMapAsXml:		sampler->saveSampleMap(); return true;
	case SaveSampleMapAsMonolith:	sampler->saveSampleMapAsMonolith(findParentComponentOfClass<BackendProcessorEditor>()); return true;
	case ImportSfz:			{
							FileChooser f("Import sfz", GET_PROJECT_HANDLER(sampler).getWorkDirectory(), "*.sfz");



							if(f.browseForFileToOpen())
							{
								try
								{
									//sampler->setBypassed(true);

									sampler->clearSampleMap();


									SfzImporter sfz(sampler, f.getResult());
									sfz.importSfzFile();

									//sampler->setBypassed(false);
								}
								catch(SfzImporter::SfzParsingError error)
								{
									debugError(sampler, error.getErrorMessage());
								}
							}
							return true;
							}
	case ImportFiles:		{

                            AudioFormatManager afm;
                            afm.registerBasicFormats();

                            FileChooser f("Load new samples", GET_PROJECT_HANDLER(sampler).getWorkDirectory(), afm.getWildcardForAllFormats(), true);
							if(f.browseForMultipleFilesToOpen())
							{
								StringArray fileNames;
								for(int i = 0; i < f.getResults().size(); i++)
								{
									fileNames.add(f.getResults()[i].getFullPathName());
								}

								SampleImporter::importNewAudioFiles(this, sampler, fileNames);

							}
							return true;
							}
	case FillVelocityGaps:
	case FillNoteGaps:		{
							sampler->getUndoManager()->beginNewTransaction("Fill Gaps");

							SampleImporter::closeGaps(selectedSoundList, info.commandID == FillNoteGaps);



							return true;
							}
	case AutomapVelocity:	SampleEditHandler::SampleEditingActions::automapVelocity(handler);
							return true;
	case RefreshVelocityXFade:	SampleEditHandler::SampleEditingActions::refreshCrossfades(handler);
							return true;
	case AutomapUsingMetadata: SampleEditHandler::SampleEditingActions::automapUsingMetadata(sampler);
							return true;
	case TrimSampleStart:	SampleEditHandler::SampleEditingActions::trimSampleStart(handler);
							return true;
	}
	return false;
}



void SampleMapEditor::refreshRootNotes()
{
	Array<WeakReference<ModulatorSamplerSound>> sounds = handler->getSelection().getItemArray();

	if (sounds.size() == 0 && map->selectedRootNotes == 0) return;

	BigInteger previousState = map->selectedRootNotes;

	map->selectedRootNotes.setRange(0, 128, false);
	for (int i = 0; i < sounds.size(); i++)
	{
		if (sounds[i].get() != nullptr)
		{
			map->selectedRootNotes.setBit(sounds[i]->getProperty(ModulatorSamplerSound::RootNote), true);
		}
	}

	if (map->selectedRootNotes != previousState)
	{
		map->repaint();
	}
}

ApplicationCommandManager * SampleMapEditor::getCommandManager()
{
	return sampleMapEditorCommandManager; // sampler->getMainController()->getCommandManager();
}

bool SampleMapEditor::keyPressed(const KeyPress& /*key*/)
{
	return false;
}

void SampleMapEditor::toggleVerticalSize()
{
	verticalBigSize = !verticalBigSize;

	sampler->setEditorState(sampler->getEditorStateForIndex(ModulatorSampler::EditorStates::BigSampleMap), verticalBigSize);

	double midPoint = (double)(viewport->getViewPositionX() + viewport->getViewWidth() / 2) / (double)map->getWidth();

	map->setSize(map->getWidth(), verticalBigSize ? 256 + 32 : 128 + 32);

	viewport->setViewPositionProportionately(midPoint, 0);


	body->refreshBodySize();
}


void SampleMapEditor::popoutMap()
{
	popoutCopy = new SampleMapEditor(sampler, body);





	BackendProcessorEditor* editor = findParentComponentOfClass<BackendProcessorEditor>();

	popoutCopy->setSize(337, editor->getHeight() - 150); // 337 is very important to keep the world running.

	Array<WeakReference<ModulatorSamplerSound>> refArray = handler->getSelection().getItemArray();

	Array<ModulatorSamplerSound*> soundArray;

	for (int i = 0; i < refArray.size(); i++)
	{
		if(refArray[i].get() != nullptr) soundArray.add(refArray[i].get());
	}

	popoutCopy->selectSounds(soundArray);

	popoutCopy->enablePopoutMode(this);


	popoutCopy->updateSoundData();

	editor->showPseudoModalWindow(popoutCopy, sampler->getId() + " Sample Map Editor");
}

//[/MiscUserCode]


//==============================================================================
#if 0
/*  -- Introjucer information section --

    This is where the Introjucer stores the metadata that describe this GUI layout, so
    make changes in here at your peril!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="SampleMapEditor" componentName=""
                 parentClasses="public Component, public SamplerSubEditor, public ApplicationCommandTarget, public FileDragAndDropTarget, public DragAndDropTarget"
                 constructorParams="ModulatorSampler *s, SamplerBody *b" variableInitialisers="sampler(s)"
                 snapPixels="8" snapActive="1" snapShown="1" overlayOpacity="0.330"
                 fixedSize="1" initialWidth="800" initialHeight="245">
  <BACKGROUND backgroundColour="303030">
    <RECT pos="8 8 140M 24" fill="solid: 13ffffff" hasStroke="1" stroke="1, mitered, butt"
          strokeColour="solid: fffffff"/>
    <TEXT pos="12Rr 5 244 30" fill="solid: 70ffffff" hasStroke="0" text="MAP EDITOR"
          fontname="Arial" fontsize="20" bold="1" italic="0" justification="34"/>
  </BACKGROUND>
  <JUCERCOMP name="" id="d91d22eeed4f96dc" memberName="rootNoteSetter" virtualName=""
             explicitFocusOrder="0" pos="-107Cc 206 90 32" sourceFile="ValueSettingComponent.cpp"
             constructorParams=""/>
  <JUCERCOMP name="" id="10b17e3eebf94c7b" memberName="lowKeySetter" virtualName=""
             explicitFocusOrder="0" pos="-11Cc 206 90 32" sourceFile="ValueSettingComponent.cpp"
             constructorParams=""/>
  <JUCERCOMP name="" id="1a26f6c542cb0cd0" memberName="highKeySetter" virtualName=""
             explicitFocusOrder="0" pos="85Cc 206 90 32" sourceFile="ValueSettingComponent.cpp"
             constructorParams=""/>
  <JUCERCOMP name="" id="1399684357769aa8" memberName="lowVelocitySetter"
             virtualName="" explicitFocusOrder="0" pos="181Cc 206 90 32" sourceFile="ValueSettingComponent.cpp"
             constructorParams=""/>
  <JUCERCOMP name="" id="3d315426e7471e3b" memberName="highVelocitySetter"
             virtualName="" explicitFocusOrder="0" pos="277Cc 206 90 32" sourceFile="ValueSettingComponent.cpp"
             constructorParams=""/>
  <JUCERCOMP name="" id="103aa29ef26df10b" memberName="rrGroupSetter" virtualName=""
             explicitFocusOrder="0" pos="-196Cc 206 76 32" sourceFile="ValueSettingComponent.cpp"
             constructorParams=""/>
  <LABEL name="new label" id="45354151fdeccf85" memberName="displayGroupLabel"
         virtualName="" explicitFocusOrder="0" pos="-281Cc 201 82 24"
         textCol="ffffffff" edTextCol="ff000000" edBkgCol="0" labelText="Display Group"
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Khmer UI" fontsize="13" bold="0" italic="0" justification="33"/>
  <LABEL name="new label" id="3ca481f4230f2188" memberName="groupDisplay"
         virtualName="PopupLabel" explicitFocusOrder="0" pos="-281Cc 221 80 16"
         bkgCol="38ffffff" outlineCol="38ffffff" edTextCol="ff000000"
         edBkgCol="0" hiliteCol="407a0000" labelText="All" editableSingleClick="1"
         editableDoubleClick="1" focusDiscardsChanges="0" fontname="Khmer UI"
         fontsize="14" bold="0" italic="0" justification="36"/>
  <VIEWPORT name="new viewport" id="4dd9b86d9d2cbf57" memberName="viewport"
            virtualName="" explicitFocusOrder="0" pos="0Cc 42 640 160" vscroll="0"
            hscroll="1" scrollbarThickness="12" contentType="2" jucerFile=""
            contentClass="MapWithKeyboard" constructorParams="sampler, b"/>
  <GENERICCOMPONENT name="new component" id="498417b33c43bc3c" memberName="toolbar"
                    virtualName="" explicitFocusOrder="0" pos="12 10 140M 20" class="Toolbar"
                    params=""/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
//[/EndFile]
