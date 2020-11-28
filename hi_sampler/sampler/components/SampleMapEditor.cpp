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
namespace hise { using namespace juce;
//[/Headers]

#include "SampleMapEditor.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...

MARKDOWN_CHAPTER(SampleMapEditorHelp)
START_MARKDOWN(Warning)
ML("### Why is icon appearing");
ML("In HISE, a sample map is a encapsulated content type which can be loaded into different samplers.");
ML("During development, sample maps are stored as XML files in their subdirectory and are cached in order to speed up loading times.");
ML("However this means that if you make changes to a samplemap it won't get written to disk until you explicitely save the sample map");
ML("This button will light up as soon as you've made changes indicating that you need to resave the samplemap.");
ML("> If you save the project as XML Backup, all currently loaded sample maps will be saved along with the rest of the preset and if you try to select another sample map using the selector on the left, it will ask whether it should save the changes you made before loading the new map. However if you load another sample map using the scripting call `Sampler.loadSampleMap()`, any changes will be discarded.");
END_MARKDOWN()
START_MARKDOWN(Help)
ML("### Toolbar Functions");
ML("| Icon | Function | Shortcut | Description |");
ML("| -- | ------ | ---- | ------------------- |");
ML("| ![](New SampleMap) | **New SampleMap** | `Cmd + N` | Deletes all samples and creates a new sample map |");
ML("| ![](Save SampleMap) | **Save SampleMap** | `Cmd + S` | Saves the current samplemap as XML file. Use this before saving the sampler. |");
ML("| ![](Save as Monolith) | **Save as Monolith** | `` | Collects all samples of the current samplemap and writes them into one monolithic file. You can compress this file using the HLAC codec to save space. **This is the recommended way of distributing sample content in a finished project**. |");
ML("| ![](Zoom In) | **Zoom In** | `Cmd +` | Zooms the samplemap horizontally. |");
ML("| ![](Zoom Out) | **Zoom Out** | `Cmd -` | Zooms the samplemap horizontally. |");
ML("| ![](Cut) | **Cut** | `Cmd+X` | Cuts the currently selected samples |");
ML("| ![](Copy) | **Copy** | `Cmd+C` | Copies the currently selected samples |");
ML("### Editing Sample Properties")
ML("You can change the MIDI related properties of each sample by setting its property.");
ML("You can either use the **+ / -** buttons to change the values relatively, or directly enter a value (in this case it will be applied to the whole selection).");
ML("| Name | Description |");
ML("| -------- | ---------------------------- |");
ML("| RR Group | the group index for round robin / random group start behaviour. Although it's called RRGroup, it can be used for any other purpose (like dynamic X-Fade or legato sample triggering). Think of this as an additional z-axis in the note/velocity coordinate system. |");
ML("| RootNote | the root note. This is the reference pitch note. |");
ML("| HiKey | the highest mapped key. |");
ML("| LoKey | the lowest mapped key. |");
ML("| LoVel | the lowest mapped velocity. |");
ML("| HiVel | the highest mapped velocity. |");
ML("> Right click on the property to open a big slider that allows finer adjustment");
ML("### SFZ Importer");
ML("SFZ is a free file exchange format for samplers. However, **HISE** is not designed to be a SFZ sample player. The SFZ parser makes it more easy to transfer other sample formats to **HISE**. Although there are opcodes for almost any property of a sampler, only these opcodes are supported:");
ML("> `sample, lokey, hikey, lovel, hivel, offset, end, loop_mode, loopstart, loopend, tune, pitch_keycenter, volume, group_volume, pan, groupName, key`");
ML("These are pretty much all opcodes thich relate to a **HISE** sampler property. If you want to convert NI KONTAKT libraries, check out the Chicken Translator SFZ edition, as this is the preferred way of migrating KONTAKT libraries");
ML("Loading SFZ files is remarkably easy, just use the SFZ button, or drop a .sfz file on the sampler. If there are multiple groups in the SFZ file, you will see a dialog window where you can consolidate the sfz groups to RR Groups or ignore dedicated sfz groups (and drop the same sfz on another sampler with a inverted selection to split the sfz file to two independant samplers")

END_MARKDOWN()
END_MARKDOWN_CHAPTER()

//[/MiscUserDefs]

//==============================================================================
SampleMapEditor::SampleMapEditor (ModulatorSampler *s, SamplerBody *b):
	SamplerSubEditor(s->getSampleEditHandler()),
	sampler(s)
{
    //[Constructor_pre] You can add your own custom stuff here..

	popoutMode = false;

    //[/Constructor_pre]

	s->getMainController()->getExpansionHandler().addListener(this);

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

	addAndMakeVisible(currentRRGroupLabel = new Label("new label",
		TRANS("All")));
	currentRRGroupLabel->setFont(GLOBAL_BOLD_FONT());
	currentRRGroupLabel->setJustificationType(Justification::centred);
	currentRRGroupLabel->setEditable(false, false, false);
	currentRRGroupLabel->setColour(Label::backgroundColourId, Colour(0x44ffffff));
	currentRRGroupLabel->setColour(Label::outlineColourId, Colour(0x38ffffff));
	currentRRGroupLabel->setColour(TextEditor::textColourId, Colours::black);
	currentRRGroupLabel->setColour(TextEditor::backgroundColourId, Colour(0x00000000));
	currentRRGroupLabel->setColour(TextEditor::highlightColourId, Colour(0x407a0000));
	currentRRGroupLabel->setText("1", dontSendNotification);
	currentRRGroupLabel->setTooltip("The current RR group. Click to enable auto-follow");
	currentRRGroupLabel->addMouseListener(this, true);

    addAndMakeVisible (viewport = new Viewport ("new viewport"));
    viewport->setScrollBarsShown (false, true);
    viewport->setScrollBarThickness (12);
    viewport->setViewedComponent (new MapWithKeyboard (sampler));

    addAndMakeVisible (toolbar = new Toolbar());
    toolbar->setName ("new component");

	

    //[UserPreSize]


	addAndMakeVisible(lowXFadeSetter = new ValueSettingComponent());
	addAndMakeVisible(highXFadeSetter = new ValueSettingComponent());

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

	
	lowXFadeSetter->setPropertyType(SampleIds::LowerVelocityXFade);
	highXFadeSetter->setPropertyType(SampleIds::UpperVelocityXFade);

	lowXFadeSetter->setVisible(false);
	highXFadeSetter->setVisible(false);

	groupDisplay->setEditable(false);

	rrGroupSetter->setPropertyType(SampleIds::RRGroup);
	rootNoteSetter->setPropertyType(SampleIds::Root);
	lowKeySetter->setPropertyType(SampleIds::LoKey);
	highKeySetter->setPropertyType(SampleIds::HiKey);
	lowVelocitySetter->setPropertyType(SampleIds::LoVel);
	highVelocitySetter->setPropertyType(SampleIds::HiVel);

	setWantsKeyboardFocus(true);

	body = b;

	verticalBigSize = sampler->getEditorState(sampler->getEditorStateForIndex(ModulatorSampler::EditorStates::BigSampleMap));

	map->setBounds(0, 0, 768, (verticalBigSize ? 256 : 128) + 32);
    
    map->addMouseListener(this, true);

	addAndMakeVisible(sampleMaps = new ComboBox());

	sampleMaps->setTextWhenNoChoicesAvailable("No samplemaps found");
	sampleMaps->setTextWhenNothingSelected("No samplemap loaded");
	sampleMaps->addListener(this);

	sampler->getMainController()->skin(*sampleMaps);
	updateSampleMapSelector(true);
	
	sampler->getSampleMap()->addListener(this);
	sampler->getMainController()->getCurrentSampleMapPool()->addListener(this);

	addAndMakeVisible(warningButton = new MarkdownHelpButton());

	warningButton->setShape(f.createPath("Warning"), false, true, true);

	warningButton->setHelpText(SampleMapEditorHelp::Warning());
	warningButton->setFontSize(14.0f);
	warningButton->setPopupWidth(400);

	addAndMakeVisible(helpButton = new MarkdownHelpButton());

	helpButton->setHelpText<PathProvider<Factory>>(SampleMapEditorHelp::Help());
	helpButton->setFontSize(14.0f);
	helpButton->setPopupWidth(600);

	menuButtons.insertMultiple(0, nullptr, SampleMapCommands::numCommands);

	addMenuButton(ZoomIn);
	addMenuButton(ZoomOut);
	addMenuButton(NewSampleMap);
	addMenuButton(SaveSampleMap);
	addMenuButton(SaveSampleMapAsMonolith);
	addMenuButton(ImportSfz);
	addMenuButton(Undo);
	addMenuButton(Redo);
	addMenuButton(DuplicateSamples);
	addMenuButton(CutSamples);
	addMenuButton(CopySamples);
	addMenuButton(PasteSamples);
	addMenuButton(DeleteSamples);
	addMenuButton(SelectAllSamples);
	addMenuButton(DeselectAllSamples);
	addMenuButton(FillNoteGaps);
	addMenuButton(FillVelocityGaps);
	addMenuButton(RefreshVelocityXFade);

	

    //[/UserPreSize]

    setSize (800, 245);


    //[Constructor] You can add your own custom stuff here..

	getCommandManager()->commandStatusChanged();

    //[/Constructor]
}

SampleMapEditor::~SampleMapEditor()
{
    //[Destructor_pre]. You can add your own custom destruction code here..

	sampler->getSampleMap()->removeListener(this);
	sampler->getMainController()->getCurrentSampleMapPool()->removeListener(this);

	sampler->getMainController()->getExpansionHandler().removeListener(this);

	if (getCommandManager()->getFirstCommandTarget(CopySamples))
	{
		getCommandManager()->setFirstCommandTarget(nullptr);
	}

	sampleMaps->removeListener(this);
	sampleMaps = nullptr;

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

#define PLACE_BUTTON(x) x->setBounds(topBar.removeFromLeft(24).reduced(2));
#define ADD_SPACER(x) topBar.removeFromLeft(x);

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



	toolbar->setVisible(false);

	auto topBar = Rectangle<int>(12, 8, getWidth() - 132, 24);

	PLACE_BUTTON(getButton(NewSampleMap));
	PLACE_BUTTON(getButton(ImportSfz));

	ADD_SPACER(8);

	sampleMaps->setBounds(topBar.removeFromLeft(150));

	if (sampler->getSampleMap()->hasUnsavedChanges())
	{
		warningButton->setVisible(true);

		PLACE_BUTTON(warningButton);
	}
	else
		warningButton->setVisible(false);

	ADD_SPACER(8);

	PLACE_BUTTON(getButton(SaveSampleMap));
	PLACE_BUTTON(getButton(SaveSampleMapAsMonolith));
	
	ADD_SPACER(8);

	if (getWidth() > 750)
	{
		PLACE_BUTTON(getButton(ZoomIn));
		PLACE_BUTTON(getButton(ZoomOut));
	}

	ADD_SPACER(8);

	PLACE_BUTTON(getButton(Undo));
	PLACE_BUTTON(getButton(Redo));

	ADD_SPACER(4);

	PLACE_BUTTON(getButton(SelectAllSamples));
	PLACE_BUTTON(getButton(DeselectAllSamples));

	ADD_SPACER(4);

	PLACE_BUTTON(getButton(CutSamples));
	PLACE_BUTTON(getButton(CopySamples));
	PLACE_BUTTON(getButton(PasteSamples));

	ADD_SPACER(4);

	PLACE_BUTTON(getButton(DuplicateSamples));
	PLACE_BUTTON(getButton(DeleteSamples));
	
	ADD_SPACER(8);

	PLACE_BUTTON(getButton(FillNoteGaps));
	PLACE_BUTTON(getButton(FillVelocityGaps));
	PLACE_BUTTON(getButton(RefreshVelocityXFade));

	ADD_SPACER(8);

	PLACE_BUTTON(helpButton);


	const bool isInMainPanel = findParentComponentOfClass<PanelWithProcessorConnection>() == nullptr;

	int viewportHeight;
	int viewportWidth;
	int viewportY = 42;

	if (isInMainPanel)
	{
		viewportHeight = verticalBigSize ? 256 + 32 : 128 + 32;
		viewportWidth = 768;
	}
	else
	{
		auto wToUse = getWidth() - 10;

		auto w = wToUse - wToUse % 128;

		viewportWidth = w;

		auto hToUse = getHeight() - 132;

		viewportHeight = hToUse - hToUse % 128 + 32;

		viewportY += (hToUse % 128 / 2);
		
	}

	

	viewport->setBounds((getWidth() / 2) - (viewportWidth / 2), viewportY, viewportWidth, viewportHeight);

	updateMapInViewport();

	int y = getHeight() - JUCE_LIVE_CONSTANT_OFF(46);

	rootNoteSetter->setBounds((getWidth() / 2) + -107 - (90 / 2), y, 90, 32);
	lowKeySetter->setBounds((getWidth() / 2) + -11 - (90 / 2), y, 90, 32);
	highKeySetter->setBounds((getWidth() / 2) + 85 - (90 / 2), y, 90, 32);
	lowVelocitySetter->setBounds((getWidth() / 2) + 181 - (90 / 2), y, 90, 32);
	highVelocitySetter->setBounds((getWidth() / 2) + 277 - (90 / 2), y, 90, 32);
	rrGroupSetter->setBounds((getWidth() / 2) + -196 - (76 / 2), y, 76, 32);
	displayGroupLabel->setBounds((getWidth() / 2) + -281 - (82 / 2), y-5, 82, 24);
	groupDisplay->setBounds((getWidth() / 2) + -281 - (80 / 2), y+16, 80, 16);

	
	lowXFadeSetter->setBounds(lowKeySetter->getBounds());
	highXFadeSetter->setBounds(highKeySetter->getBounds());

	
	auto gb = groupDisplay->getBounds();

	currentRRGroupLabel->setBounds(gb.removeFromRight(gb.getHeight()*3/2));

	groupDisplay->setBounds(gb);

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
		sampler->getSamplerDisplayValues().currentlyDisplayedGroup = index;
        //[/UserLabelCode_groupDisplay]
    }

    //[UserlabelTextChanged_Post]
    //[/UserlabelTextChanged_Post]
}





//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...

void SampleMapEditor::addMenuButton(SampleMapCommands commandId)
{
	ApplicationCommandInfo r(commandId);

	getCommandInfo(commandId, r);

	auto b = new HiseShapeButton(r.shortName, nullptr, f);
	b->setCommandToTrigger(getCommandManager(), commandId, true);

	addAndMakeVisible(b);
	ownedMenuButtons.add(b);
	menuButtons.set(commandId, b);
}


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
		BACKEND_ONLY(result.setActive(dynamic_cast<SamplerBody*>(getParentComponent()) != nullptr));
		break;
	case PopOutMap:			result.setInfo("Show Map Editor in popup", "Show Map Editor in popup", "Zooming", 0);
		BACKEND_ONLY(result.setActive(dynamic_cast<SamplerBody*>(getParentComponent()) != nullptr));
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
	case RemoveNormalisationInfo: result.setInfo("Remove Normalisation Info", "Resets the normalisation value", "SampleMap Handling", 0);
		result.setActive(true);
		break;
	case DuplicateSampleMapAsReference:
		result.setInfo("Duplicate as Reference", "Creates a copy of this samplemap and reuses the current monolith", "SampleMap Handling", 0);
		result.setActive(true);
		break;

	case SaveSampleMapAsMonolith:	
#if USE_BACKEND
		result.setInfo("Convert to Monolith", "Convert the current samplemap to HLAC monolith format", "SampleMap Handling", 0);
#else
		result.setInfo("Convert to Monolith", "Encode all samples in this expansion as HLAC monolith", "SampleMap Handling", 0);
#endif
		result.setActive(true);
		break;
	case RevertSampleMap:result.setInfo("Revert sample map", "Discards all changes and reloads the samplemap from disk", "SampleMap Handling", 0);
		result.setActive(sampler->getSampleMap()->hasUnsavedChanges());
		break;
	case ImportSfz:		result.setInfo("Import SFZ file format", "Import SFZ file format", "SampleMap Handling", 0);
		result.setActive(true);
		break;
	case Undo:			result.setInfo("Undo", "Undo", "", 0);
						result.addDefaultKeypress('z', ModifierKeys::commandModifier);
						break;
	case Redo:			result.setInfo("Redo", "Redo", "", 0);
						result.addDefaultKeypress('y', ModifierKeys::commandModifier);
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
	case DeselectAllSamples: result.setInfo("Deselect all Samples", "Deselect all Samples", "Sample Editing", 0);
		result.addDefaultKeypress(KeyPress::escapeKey, ModifierKeys::noModifiers);
		result.setActive(true);
		break;
	case MergeIntoMultisamples:
		result.setInfo("Merge into Multimic samples", "Merge into Multimic samples", "Sample Editing", 0);
		result.setActive(selectionIsNotEmpty);
		break;
	case CreateMultiMicSampleMap:
		result.setInfo("Create Multimic SampleMap from single mic position", "Create Multimic SampleMap from single mic position", "Sample Editing", 0);
		result.setActive(true);
		break;
	case ExtractToSingleMicSamples:
		result.setInfo("Extract to Singlemic samples", "Extract to Singlemic samples", "Sample Editing", 0);
		result.setActive(selectionIsNotEmpty);
		break;
	case ReencodeMonolith:
		result.setInfo("Reencode HLAC monolith", "Reencode HLAC monolith", "Sample Editing", 0);
		result.setActive(true);
		result.addDefaultKeypress(KeyPress::F5Key, ModifierKeys::shiftModifier);
		break;
	case EncodeAllMonoliths:
		result.setInfo("(Re)encode all sample maps as HLAC monolith", "(Re)encode all sample maps as HLAC monolith", "Sample Editing", 0);
		result.setActive(true);
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

			int lowLimit = selectedSoundList[0]->getSampleProperty(SampleIds::LoKey);
			int highLimit = selectedSoundList[0]->getSampleProperty(SampleIds::HiKey);

			for (int i = 1; i < selectedSoundList.size(); i++)
			{
				if (lowLimit != (int)selectedSoundList[i]->getSampleProperty(SampleIds::LoKey) ||
					highLimit != (int)selectedSoundList[i]->getSampleProperty(SampleIds::HiKey))
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
		result.setActive(true);
		break;
	case ExportAiffWithMetadata: result.setInfo("Export AIFF with metadata", "Exports the current samplemap as AIFF files with metadata", "Sample Editing", 0);
		result.setActive(true);
		break;
	case AutomapUsingMetadata:  result.setInfo("Automap using Metadata", "Automaps the sample using the metadata that is found in the sample file.", "Sample Editing", 0);
		result.setActive(selectionIsNotEmpty);
		break;
	case TrimSampleStart: result.setInfo("Trim Sample Start", "Removes the silence at the beginning of samples", "Sample Editing", 0);
		result.setActive(selectionIsNotEmpty);
		break;
	}

	f.updateCommandInfoWithKeymapping(result);

#if 0
	if (auto holder = dynamic_cast<MarkdownDatabaseHolder*>(sampler->getMainController()))
	{
		MarkdownParser::PathDescriptionResolver r(*holder, f);
		r.updateCommandInfoFromArray(result);
	}
#endif
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
	case DeselectAllSamples:	SampleEditHandler::SampleEditingActions::deselectAllSamples(handler); return true;
	case MergeIntoMultisamples:		SampleEditHandler::SampleEditingActions::mergeIntoMultiSamples(handler, this); return true;
	case CreateMultiMicSampleMap:	SampleEditHandler::SampleEditingActions::createMultimicSampleMap(handler); return true;
	case ExtractToSingleMicSamples:	SampleEditHandler::SampleEditingActions::extractToSingleMicSamples(handler); return true;
	case RemoveNormalisationInfo: SampleEditHandler::SampleEditingActions::removeNormalisationInfo(handler); return true;
	case ReencodeMonolith:	SampleEditHandler::SampleEditingActions::reencodeMonolith(this, handler); return true;
	
	case ZoomIn:			zoom(false); return true;
	case ZoomOut:			zoom(true); return true;
	case Undo:				sampler->getUndoManager()->undo(); return true;
	case Redo:				sampler->getUndoManager()->redo(); return true;
	case ToggleVerticalSize:toggleVerticalSize(); return true;
	case NewSampleMap:		if (PresetHandler::showYesNoWindow("Create new samplemap", "Do you want to create a new sample map? The current samplemap will be discarded", PresetHandler::IconType::Question))
	{
		auto f2 = [](Processor* p) {dynamic_cast<ModulatorSampler*>(p)->clearSampleMap(sendNotificationAsync); return SafeFunctionCall::OK; };
		sampler->killAllVoicesAndCall(f2, true);
	}
	return true;
	case LoadSampleMap:				loadSampleMap(); return true;
	case RevertSampleMap:
	{
		if (PresetHandler::showYesNoWindow("Revert Samplemap", "Do you really want to revert the samplemap"))
		{
			auto ref = sampler->getSampleMap()->getReference();
			sampler->getMainController()->getCurrentSampleMapPool()->loadFromReference(ref, PoolHelpers::ForceReloadStrong);
			auto f2 = [ref](Processor* p) {dynamic_cast<ModulatorSampler*>(p)->loadSampleMap(ref); return SafeFunctionCall::OK; };
			sampler->killAllVoicesAndCall(f2, true);
		}
		
		return true;
	}
	case SaveSampleMap:				sampler->saveSampleMap(); refreshSampleMapPool(); return true;
	case DuplicateSampleMapAsReference:	sampler->saveSampleMapAsReference(); refreshSampleMapPool(); return true;
	case ExportAiffWithMetadata:
		SampleEditHandler::SampleEditingActions::writeSamplesWithAiffData(sampler); return true;
	case SaveSampleMapAsMonolith:	
#if USE_BACKEND
		sampler->saveSampleMapAsMonolith(this); return true;
#endif
	case EncodeAllMonoliths:	SampleEditHandler::SampleEditingActions::encodeAllMonoliths(this, handler); return true;

	case ImportSfz:					importSfz(); return true;

	case ImportFiles:		{

                            AudioFormatManager afm;
                            afm.registerBasicFormats();

                            FileChooser fc("Load new samples", GET_PROJECT_HANDLER(sampler).getRootFolder(), afm.getWildcardForAllFormats(), true);
							if(fc.browseForMultipleFilesToOpen())
							{
								StringArray fileNames;
								for(int i = 0; i < fc.getResults().size(); i++)
								{
									fileNames.add(fc.getResults()[i].getFullPathName());
								}

								SampleImporter::importNewAudioFiles(this, sampler, fileNames);

							}
							return true;
							}
	case FillVelocityGaps:
	case FillNoteGaps:		{

							SampleImporter::closeGaps(selectedSoundList, info.commandID == FillNoteGaps);



							return true;
							}
	case AutomapVelocity:	SampleEditHandler::SampleEditingActions::automapVelocity(handler);
							return true;
	case RefreshVelocityXFade:
	{
		const bool showXFade = lowXFadeSetter->isVisible();

		lowXFadeSetter->setVisible(!showXFade);
		highXFadeSetter->setVisible(!showXFade);

		lowKeySetter->setVisible(showXFade);
		highKeySetter->setVisible(showXFade);

		//SampleEditHandler::SampleEditingActions::refreshCrossfades(handler);
		return true;
	}
		
		
	case AutomapUsingMetadata: SampleEditHandler::SampleEditingActions::automapUsingMetadata(sampler);
							return true;
	case TrimSampleStart:	SampleEditHandler::SampleEditingActions::trimSampleStart(this,handler);
							return true;
	}
	return false;
}



void SampleMapEditor::importSfz()
{
	FileChooser fc("Import sfz", GET_PROJECT_HANDLER(sampler).getRootFolder(), "*.sfz");

	if (fc.browseForFileToOpen())
	{
		try
		{
			sampler->clearSampleMap(dontSendNotification);

			SfzImporter sfz(sampler, fc.getResult());
			sfz.importSfzFile();
		}
		catch (SfzImporter::SfzParsingError error)
		{
			debugError(sampler, error.getErrorMessage());
		}
	}
}

void SampleMapEditor::loadSampleMap()
{
	auto rootFile = sampler->getSampleEditHandler()->getCurrentSampleMapDirectory();

	FileChooser fc("Load new samplemap", rootFile, "*.xml");
	if (fc.browseForFileToOpen())
	{
		PoolReference ref(sampler->getMainController(), fc.getResult().getFullPathName(), FileHandlerBase::SampleMaps);

		sampler->loadSampleMap(ref);
	}
}

void SampleMapEditor::refreshRootNotes()
{
	auto& sounds = handler->getSelection().getItemArray();

	if (sounds.size() == 0 && map->selectedRootNotes == 0) return;

	BigInteger previousState = map->selectedRootNotes;

	map->selectedRootNotes.setRange(0, 128, false);
	for (int i = 0; i < sounds.size(); i++)
	{
		if (sounds[i].get() != nullptr)
		{
			map->selectedRootNotes.setBit(sounds[i]->getSampleProperty(SampleIds::Root), true);
		}
	}

	if (map->selectedRootNotes != previousState)
	{
		map->repaint();
	}
}

void SampleMapEditor::expansionPackLoaded(Expansion* /*currentExpansion*/)
{
	updateSampleMapSelector(true);
}

void SampleMapEditor::sampleMapWasChanged(PoolReference newSampleMap)
{
	sampleMaps->setText(newSampleMap.getReferenceString(), dontSendNotification);
	resized();
}

void SampleMapEditor::sampleAmountChanged()
{
	updateWarningButton();
}

void SampleMapEditor::samplePropertyWasChanged(ModulatorSamplerSound* /*s*/, const Identifier& id, const var& newValue)
{
	updateWarningButton();

	if (id == SampleIds::Root)
		refreshRootNotes();

	if (id == SampleIds::RRGroup)
		setCurrentRRGroup((int)newValue);

}

void SampleMapEditor::updateWarningButton()
{
	bool unsavedChanges = sampler->getSampleMap()->hasUnsavedChanges();
	bool warningVisible = warningButton->isVisible();

	if (unsavedChanges != warningVisible)
		resized();
}

void SampleMapEditor::comboBoxChanged(ComboBox* b)
{
	auto t = b->getText();

	if (sampler->getSampleMap()->hasUnsavedChanges())
	{
		if (PresetHandler::showYesNoWindow("Save " + sampler->getSampleMap()->getId().toString(), "Do you want to save the current sample map"))
			sampler->getSampleMap()->saveAndReloadMap();
		else
			sampler->getSampleMap()->discardChanges();
	}

	PoolReference r(sampler->getMainController(), t, FileHandlerBase::SampleMaps);

	auto f2 = [r](Processor* p)
	{
		dynamic_cast<ModulatorSampler*>(p)->loadSampleMap(r);
		return SafeFunctionCall::OK;
	};

	sampler->killAllVoicesAndCall(f2);
}

ApplicationCommandManager * SampleMapEditor::getCommandManager()
{
	return sampleMapEditorCommandManager; // sampler->getMainController()->getCommandManager();
}

bool SampleMapEditor::keyPressed(const KeyPress& k)
{
	if (int commandId = getCommandManager()->getKeyMappings()->findCommandForKeyPress(k))
	{
		getCommandManager()->invokeDirectly(commandId, false);
		return true;
	}

	if (k.getModifiers().isShiftDown())
	{
		if (k.getKeyCode() == k.upKey)
		{
			int index = getCurrentRRGroup();

			if (index == -1) index = 0;

			index++;

			setCurrentRRGroup(index);

			return true;
		}
		else if (k.getKeyCode() == k.downKey)
		{
			int index = getCurrentRRGroup();

			index--;

			setCurrentRRGroup(index);

			return true;
		}
	}
	if (k.getKeyCode() == KeyPress::leftKey)
	{
		if (k.getModifiers().isCommandDown())
			handler->moveSamples(SamplerSoundMap::Left);
		else
			getMapComponent()->selectNeighbourSample(SamplerSoundMap::Left);
		return true;
	}
	else if (k.getKeyCode() == KeyPress::rightKey)
	{
		if (k.getModifiers().isCommandDown())
			handler->moveSamples(SamplerSoundMap::Right);
		else
			getMapComponent()->selectNeighbourSample(SamplerSoundMap::Right);
		return true;
	}
	else if (k.getKeyCode() == KeyPress::upKey)
	{
		if (k.getModifiers().isCommandDown())
			handler->moveSamples(SamplerSoundMap::Up);
		else
			getMapComponent()->selectNeighbourSample(SamplerSoundMap::Up);
		return true;
	}
	else if (k.getKeyCode() == KeyPress::downKey)
	{
		if (k.getModifiers().isCommandDown())
			handler->moveSamples(SamplerSoundMap::Down);
		else
			getMapComponent()->selectNeighbourSample(SamplerSoundMap::Down);
		return true;
	}

	return false;
}

void SampleMapEditor::refreshSampleMapPool()
{
	sampler->getMainController()->getCurrentSampleMapPool()->refreshPoolAfterUpdate();
}

void SampleMapEditor::toggleFollowRRGroup()
{
	followRRGroup = !followRRGroup;

	if (followRRGroup)
	{
		currentRRGroupLabel->setColour(Label::ColourIds::outlineColourId, Colour(SIGNAL_COLOUR));
	}
	else
	{
		currentRRGroupLabel->setColour(Label::ColourIds::outlineColourId, Colour(0x38FFFFFF));
		setCurrentRRGroup(0);
	}
}

void SampleMapEditor::toggleVerticalSize()
{
	verticalBigSize = !verticalBigSize;

	sampler->setEditorState(sampler->getEditorStateForIndex(ModulatorSampler::EditorStates::BigSampleMap), verticalBigSize);

	double midPoint = (double)(viewport->getViewPositionX() + viewport->getViewWidth() / 2) / (double)map->getWidth();

	map->setSize(map->getWidth(), verticalBigSize ? 256 + 32 : 128 + 32);

	viewport->setViewPositionProportionately(midPoint, 0);


	BACKEND_ONLY(body->refreshBodySize());
}

void SampleMapEditor::paintOverChildren(Graphics& g)
{
	if (mapIsHovered)
	{
		g.setColour(Colours::white.withAlpha(0.2f));

		g.drawRect(viewport->getBounds(), 2);
	}
	else if (sampler->getNumSounds() == 0)
	{
		Font font = GLOBAL_BOLD_FONT();

		g.setFont(font);

		static const String text = "Drop samples or samplemaps from browser";
		const int w = font.getStringWidth(text) + 20;

		g.setColour(Colours::white.withAlpha(0.3f));

		g.setColour(Colours::black.withAlpha(0.5f));
		Rectangle<int> r((getWidth() - w) / 2, (getHeight() - 20) / 2, w, 20);
		g.fillRect(r);
		g.setColour(Colours::white.withAlpha(0.5f));
		g.drawRect(r, 1);

		g.drawText(text, getLocalBounds(), Justification::centred);
	}
}

void SampleMapEditor::updateSampleMapSelector(bool rebuild)
{
	Component::SafePointer<ComboBox> cb(sampleMaps.get());
	WeakReference<ModulatorSampler> s(sampler);

	auto f2 = [rebuild, cb, s]()
	{
		if (cb.getComponent() == nullptr)
			return;

		if (rebuild)
		{
			cb.getComponent()->setTextWhenNothingSelected("Select SampleMap");

			cb.getComponent()->clear(dontSendNotification);

			auto mc = s.get()->getMainController();

			SampleMapPool* pool = nullptr;

			if (auto e = mc->getExpansionHandler().getCurrentExpansion())
			{
				if (e->getExpansionType() != Expansion::FileBased)
				{
					cb.getComponent()->setTextWhenNothingSelected("Encrypted Expansion");
					return;
				}

				pool = &e->pool->getSampleMapPool();
			}
				
			else
				pool = &mc->getCurrentFileHandler().pool->getSampleMapPool();

			auto ref = pool->getListOfAllReferences(true);

			PoolReference::Comparator comp;

			ref.sort(comp, true);

			int i = 1;

			for (auto r : ref)
			{
				if (!r.isValid())
					continue;

				cb.getComponent()->addItem(r.getReferenceString(), i++);
			}
				
		}

		if (auto currentRef = s.get()->getSampleMap()->getReference())
			cb.getComponent()->setText(currentRef.getReferenceString(), dontSendNotification);
	};

	MessageManager::callAsync(f2);
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

juce::Path SampleMapEditor::Factory::createPath(const String& name) const
{
	auto url = MarkdownLink::Helpers::getSanitizedFilename(name);

	Path p;

	LOAD_PATH_IF_URL("new-samplemap", EditorIcons::newFile);
	LOAD_PATH_IF_URL("import-sfz-file-format", SampleMapIcons::sfzImport);

	LOAD_PATH_IF_URL("warning", EditorIcons::warningIcon);
	LOAD_PATH_IF_URL("save-samplemap", EditorIcons::saveFile);
	LOAD_PATH_IF_URL("convert-to-monolith", SampleMapIcons::monolith);

	LOAD_PATH_IF_URL("zoom-in", SampleMapIcons::zoomIn);
	LOAD_PATH_IF_URL("zoom-out", SampleMapIcons::zoomOut);

	LOAD_PATH_IF_URL("undo", EditorIcons::undoIcon);
	LOAD_PATH_IF_URL("redo", EditorIcons::redoIcon);

	LOAD_PATH_IF_URL("select-all-samples", SampleMapIcons::selectAll);
	LOAD_PATH_IF_URL("deselect-all-samples", EditorIcons::cancelIcon);

	LOAD_PATH_IF_URL("cut", SampleMapIcons::cutSamples);
	LOAD_PATH_IF_URL("copy", SampleMapIcons::copySamples);
	LOAD_PATH_IF_URL("paste", SampleMapIcons::pasteSamples);
	LOAD_PATH_IF_URL("duplicate", SampleMapIcons::duplicateSamples);
	LOAD_PATH_IF_URL("delete", SampleMapIcons::deleteSamples);
	
	LOAD_PATH_IF_URL("fill-note-gaps", SampleMapIcons::fillNoteGaps);
	LOAD_PATH_IF_URL("fill-velocity-gaps", SampleMapIcons::fillVelocityGaps);

	
	LOAD_PATH_IF_URL("load-samplemap", EditorIcons::openFile);
	
	LOAD_PATH_IF_URL("refresh-velocity-crossfades.", SampleMapIcons::refreshCrossfade);
	
	LOAD_PATH_IF_URL("trim-sample-start", SampleMapIcons::trimSampleStart);
	
	LOAD_PATH_IF_URL("rebuild", ColumnIcons::moveIcon);
	
	return p;
}

} // namespace hise
//[/EndFile]
