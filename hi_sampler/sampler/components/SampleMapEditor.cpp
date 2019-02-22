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

	Factory f;

	addAndMakeVisible(warningButton = new MarkdownHelpButton());

	warningButton->setShape(f.createPath("Warning"), false, true, true);

	warningButton->setHelpText(SampleMapEditorHelp::Warning());
	warningButton->setFontSize(14.0f);
	warningButton->setPopupWidth(400);

	addAndMakeVisible(helpButton = new MarkdownHelpButton());

	helpButton->setHelpText<MarkdownParser::PathProvider<Factory>>(SampleMapEditorHelp::Help());
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

	Factory f;

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
	case DuplicateSampleMapAsReference:
		result.setInfo("Duplicate as Reference", "Creates a copy of this samplemap and reuses the current monolith", "SampleMap Handling", 0);
		result.setActive(true);
		break;
	case SaveSampleMapAsMonolith:	result.setInfo("Save as Monolith", "Save the current SampleMap as one big monolith file", "SampleMap Handling", 0);
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
	case DeselectAllSamples:	SampleEditHandler::SampleEditingActions::deselectAllSamples(handler); return true;
	case MergeIntoMultisamples:		SampleEditHandler::SampleEditingActions::mergeIntoMultiSamples(handler, this); return true;
	case CreateMultiMicSampleMap:	SampleEditHandler::SampleEditingActions::createMultimicSampleMap(handler); return true;
	case ExtractToSingleMicSamples:	SampleEditHandler::SampleEditingActions::extractToSingleMicSamples(handler); return true;
	case ReencodeMonolith:	SampleEditHandler::SampleEditingActions::reencodeMonolith(this, handler); return true;
	case EncodeAllMonoliths:	SampleEditHandler::SampleEditingActions::encodeAllMonoliths(this, handler); return true;
	case ZoomIn:			zoom(false); return true;
	case ZoomOut:			zoom(true); return true;
	case Undo:				sampler->getUndoManager()->undo(); return true;
	case Redo:				sampler->getUndoManager()->redo(); return true;
	case ToggleVerticalSize:toggleVerticalSize(); return true;
	case NewSampleMap:		if (PresetHandler::showYesNoWindow("Create new samplemap", "Do you want to create a new sample map? The current samplemap will be discarded", PresetHandler::IconType::Question))
	{
		auto f = [](Processor* p) {dynamic_cast<ModulatorSampler*>(p)->clearSampleMap(sendNotificationAsync); return SafeFunctionCall::OK; };
		sampler->killAllVoicesAndCall(f, true);
	}
	return true;
	case LoadSampleMap:				loadSampleMap(); return true;
	case RevertSampleMap:
	{
		if (PresetHandler::showYesNoWindow("Revert Samplemap", "Do you really want to revert the samplemap"))
		{
			auto ref = sampler->getSampleMap()->getReference();
			sampler->getMainController()->getCurrentSampleMapPool()->loadFromReference(ref, PoolHelpers::ForceReloadStrong);
			auto f = [ref](Processor* p) {dynamic_cast<ModulatorSampler*>(p)->loadSampleMap(ref); return SafeFunctionCall::OK; };
			sampler->killAllVoicesAndCall(f, true);
		}
		
		return true;
	}
	case SaveSampleMap:				sampler->saveSampleMap(); refreshSampleMapPool(); return true;
	case DuplicateSampleMapAsReference:	sampler->saveSampleMapAsReference(); refreshSampleMapPool(); return true;
	case SaveSampleMapAsMonolith:	sampler->saveSampleMapAsMonolith(this); return true;
	case ImportSfz:					importSfz(); return true;

	case ImportFiles:		{

                            AudioFormatManager afm;
                            afm.registerBasicFormats();

                            FileChooser f("Load new samples", GET_PROJECT_HANDLER(sampler).getRootFolder(), afm.getWildcardForAllFormats(), true);
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
	FileChooser f("Import sfz", GET_PROJECT_HANDLER(sampler).getRootFolder(), "*.sfz");

	if (f.browseForFileToOpen())
	{
		try
		{
			sampler->clearSampleMap(dontSendNotification);

			SfzImporter sfz(sampler, f.getResult());
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

	FileChooser f("Load new samplemap", rootFile, "*.xml");
	if (f.browseForFileToOpen())
	{
		PoolReference ref(sampler->getMainController(), f.getResult().getFullPathName(), FileHandlerBase::SampleMaps);

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

	if (sampler->getSampleMap()->hasUnsavedChanges() && PresetHandler::showYesNoWindow("Save " + sampler->getSampleMap()->getId().toString(), "Do you want to save the current sample map"))
	{
		sampler->getSampleMap()->saveAndReloadMap();
	}

	PoolReference r(sampler->getMainController(), t, FileHandlerBase::SampleMaps);

	auto f = [r](Processor* p)
	{
		dynamic_cast<ModulatorSampler*>(p)->loadSampleMap(r);
		return SafeFunctionCall::OK;
	};

	sampler->killAllVoicesAndCall(f);
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
		Font f = GLOBAL_BOLD_FONT();

		g.setFont(f);

		static const String text = "Drop samples or samplemaps from browser";
		const int w = f.getStringWidth(text) + 20;

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

	auto f = [rebuild, cb, s]()
	{
		if (cb.getComponent() == nullptr)
			return;

		if (rebuild)
		{
			cb.getComponent()->clear(dontSendNotification);

			auto pool = s.get()->getMainController()->getCurrentSampleMapPool();
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

	MessageManager::callAsync(f);
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
	Path path;

	if (name == "Warning")
	{
		static const unsigned char pathData[] = { 110,109,0,2,22,67,128,64,223,67,108,0,2,22,67,128,64,223,67,108,52,245,21,67,156,64,223,67,108,111,232,21,67,10,65,223,67,108,184,219,21,67,201,65,223,67,108,25,207,21,67,218,66,223,67,108,153,194,21,67,59,68,223,67,108,65,182,21,67,235,69,223,67,108,
			24,170,21,67,234,71,223,67,108,37,158,21,67,54,74,223,67,108,113,146,21,67,206,76,223,67,108,4,135,21,67,176,79,223,67,108,227,123,21,67,218,82,223,67,108,23,113,21,67,74,86,223,67,108,167,102,21,67,254,89,223,67,108,153,92,21,67,244,93,223,67,108,243,
			82,21,67,41,98,223,67,108,188,73,21,67,154,102,223,67,108,250,64,21,67,69,107,223,67,108,178,56,21,67,38,112,223,67,108,233,48,21,67,58,117,223,67,108,165,41,21,67,127,122,223,67,108,234,34,21,67,241,127,223,67,108,189,28,21,67,140,133,223,67,108,0,27,
			21,67,64,135,223,67,108,0,27,21,67,64,135,223,67,108,0,63,11,67,192,110,233,67,108,0,63,11,67,192,110,233,67,108,151,57,11,67,141,116,233,67,108,196,52,11,67,123,122,233,67,108,139,48,11,67,133,128,233,67,108,237,44,11,67,169,134,233,67,108,238,41,11,
			67,226,140,233,67,108,143,39,11,67,44,147,233,67,108,209,37,11,67,131,153,233,67,108,183,36,11,67,227,159,233,67,108,64,36,11,67,73,166,233,67,108,108,36,11,67,175,172,233,67,108,61,37,11,67,18,179,233,67,108,176,38,11,67,110,185,233,67,108,198,40,11,
			67,190,191,233,67,108,125,43,11,67,255,197,233,67,108,211,46,11,67,45,204,233,67,108,198,50,11,67,68,210,233,67,108,83,55,11,67,63,216,233,67,108,120,60,11,67,27,222,233,67,108,50,66,11,67,213,227,233,67,108,124,72,11,67,104,233,233,67,108,82,79,11,67,
			209,238,233,67,108,177,86,11,67,12,244,233,67,108,148,94,11,67,23,249,233,67,108,245,102,11,67,237,253,233,67,108,207,111,11,67,141,2,234,67,108,28,121,11,67,242,6,234,67,108,215,130,11,67,26,11,234,67,108,250,140,11,67,3,15,234,67,108,125,151,11,67,
			170,18,234,67,108,90,162,11,67,12,22,234,67,108,138,173,11,67,40,25,234,67,108,7,185,11,67,251,27,234,67,108,199,196,11,67,132,30,234,67,108,197,208,11,67,193,32,234,67,108,249,220,11,67,176,34,234,67,108,90,233,11,67,81,36,234,67,108,225,245,11,67,162,
			37,234,67,108,133,2,12,67,162,38,234,67,108,63,15,12,67,82,39,234,67,108,6,28,12,67,175,39,234,67,108,0,36,12,67,192,39,234,67,108,0,36,12,67,192,39,234,67,108,0,220,31,67,192,39,234,67,108,0,220,31,67,192,39,234,67,108,204,232,31,67,150,39,234,67,108,
			143,245,31,67,25,39,234,67,108,66,2,32,67,75,38,234,67,108,220,14,32,67,44,37,234,67,108,85,27,32,67,189,35,234,67,108,166,39,32,67,255,33,234,67,108,198,51,32,67,242,31,234,67,108,174,63,32,67,153,29,234,67,108,86,75,32,67,243,26,234,67,108,183,86,32,
			67,5,24,234,67,108,200,97,32,67,206,20,234,67,108,132,108,32,67,82,17,234,67,108,228,118,32,67,146,13,234,67,108,224,128,32,67,145,9,234,67,108,114,138,32,67,81,5,234,67,108,149,147,32,67,213,0,234,67,108,66,156,32,67,33,252,233,67,108,116,164,32,67,
			54,247,233,67,108,38,172,32,67,25,242,233,67,108,82,179,32,67,204,236,233,67,108,244,185,32,67,82,231,233,67,108,8,192,32,67,177,225,233,67,108,138,197,32,67,234,219,233,67,108,118,202,32,67,1,214,233,67,108,202,206,32,67,251,207,233,67,108,130,210,32,
			67,220,201,233,67,108,156,213,32,67,166,195,233,67,108,22,216,32,67,95,189,233,67,108,239,217,32,67,9,183,233,67,108,37,219,32,67,170,176,233,67,108,184,219,32,67,70,170,233,67,108,167,219,32,67,223,163,233,67,108,242,218,32,67,123,157,233,67,108,154,
			217,32,67,30,151,233,67,108,159,215,32,67,203,144,233,67,108,3,213,32,67,135,138,233,67,108,200,209,32,67,86,132,233,67,108,240,205,32,67,59,126,233,67,108,124,201,32,67,59,120,233,67,108,112,196,32,67,89,114,233,67,108,0,193,32,67,192,110,233,67,108,
			0,193,32,67,192,110,233,67,108,0,229,22,67,64,135,223,67,108,0,229,22,67,64,135,223,67,108,253,222,22,67,153,129,223,67,108,108,216,22,67,27,124,223,67,108,80,209,22,67,200,118,223,67,108,175,201,22,67,165,113,223,67,108,140,193,22,67,180,108,223,67,
			108,238,184,22,67,249,103,223,67,108,217,175,22,67,118,99,223,67,108,84,166,22,67,47,95,223,67,108,100,156,22,67,38,91,223,67,108,16,146,22,67,94,87,223,67,108,95,135,22,67,217,83,223,67,108,87,124,22,67,154,80,223,67,108,0,113,22,67,163,77,223,67,108,
			96,101,22,67,245,74,223,67,108,128,89,22,67,146,72,223,67,108,102,77,22,67,124,70,223,67,108,27,65,22,67,180,68,223,67,108,166,52,22,67,59,67,223,67,108,15,40,22,67,18,66,223,67,108,95,27,22,67,58,65,223,67,108,157,14,22,67,180,64,223,67,108,0,2,22,67,
			128,64,223,67,108,0,2,22,67,128,64,223,67,99,109,0,0,22,67,128,223,224,67,108,128,62,30,67,192,39,233,67,108,128,193,13,67,192,39,233,67,108,0,0,22,67,128,223,224,67,99,109,0,196,20,67,64,5,227,67,108,128,15,21,67,192,242,230,67,108,0,24,23,67,192,242,
			230,67,108,128,103,23,67,64,5,227,67,108,0,196,20,67,64,5,227,67,99,109,0,26,22,67,0,128,231,67,98,211,174,21,67,0,128,231,67,103,85,21,67,204,143,231,67,128,13,21,67,0,175,231,67,98,244,198,20,67,52,206,231,67,128,163,20,67,41,246,231,67,128,163,20,
			67,0,39,232,67,98,128,163,20,67,206,85,232,67,244,198,20,67,195,125,232,67,128,13,21,67,0,159,232,67,98,12,84,21,67,143,191,232,67,119,173,21,67,192,207,232,67,0,26,22,67,192,207,232,67,98,228,135,22,67,192,207,232,67,170,224,22,67,226,191,232,67,128,
			36,23,67,0,160,232,67,98,177,105,23,67,30,128,232,67,0,140,23,67,215,87,232,67,0,140,23,67,0,39,232,67,98,0,140,23,67,214,246,231,67,177,105,23,67,52,207,231,67,128,36,23,67,0,176,231,67,98,79,223,22,67,30,144,231,67,136,134,22,67,0,128,231,67,0,26,22,
			67,0,128,231,67,99,101,0,0 };

		
		path.loadPathFromData(pathData, sizeof(pathData));
	}
	else if (name == "Cut")
	{
		static const unsigned char pathData[] = { 110,109,3,139,29,67,1,163,87,67,98,185,8,29,67,151,230,86,67,60,57,28,67,142,92,86,67,124,81,27,67,12,40,86,67,98,214,179,26,67,68,4,86,67,200,19,26,67,246,10,86,67,128,136,25,67,18,58,86,67,108,244,239,23,67,162,232,83,67,98,1,91,25,67,56,191,81,67,
			212,116,27,67,109,140,78,67,186,184,27,67,249,41,78,67,98,172,94,28,67,12,57,77,67,59,78,27,67,155,45,76,67,135,66,27,67,105,34,76,67,108,101,243,26,67,39,214,75,67,108,49,170,22,67,119,15,82,67,108,233,96,18,67,72,214,75,67,108,199,17,18,67,138,34,76,
			67,98,0,6,18,67,186,45,76,67,142,245,16,67,45,57,77,67,128,155,17,67,26,42,78,67,98,82,223,17,67,127,140,78,67,57,249,19,67,74,191,81,67,90,100,21,67,195,232,83,67,108,187,203,19,67,51,58,86,67,98,95,64,19,67,24,11,86,67,101,160,18,67,103,4,86,67,170,
			2,18,67,45,40,86,67,98,254,26,17,67,159,92,86,67,109,75,16,67,183,230,86,67,54,201,15,67,35,163,87,67,98,133,212,14,67,73,6,89,67,48,55,15,67,169,180,90,67,6,165,16,67,120,98,91,67,98,46,11,17,67,63,147,91,67,107,124,17,67,6,172,91,67,144,245,17,67,6,
			172,91,67,98,16,56,18,67,6,172,91,67,52,125,18,67,165,167,91,67,238,190,18,67,29,149,91,67,98,30,71,20,67,77,38,91,67,5,16,21,67,22,28,90,67,53,112,21,67,218,10,89,67,98,78,116,21,67,39,255,88,67,218,53,22,67,28,116,86,67,230,161,22,67,72,230,85,67,98,
			230,161,22,67,72,230,85,67,58,165,22,67,48,225,85,67,49,170,22,67,156,217,85,67,98,21,175,22,67,54,225,85,67,124,178,22,67,72,230,85,67,124,178,22,67,72,230,85,67,98,135,30,23,67,28,116,86,67,17,224,23,67,36,255,88,67,45,228,23,67,218,10,89,67,98,73,
			68,24,67,22,28,90,67,68,13,25,67,93,38,91,67,117,149,26,67,29,149,91,67,98,47,215,26,67,166,167,91,67,62,28,27,67,6,172,91,67,210,94,27,67,6,172,91,67,98,247,215,27,67,6,172,91,67,32,73,28,67,62,147,91,67,92,175,28,67,120,98,91,67,98,10,29,30,67,136,
			180,90,67,161,127,30,67,40,6,89,67,4,139,29,67,2,163,87,67,99,109,194,187,19,67,146,131,89,67,98,135,106,19,67,68,249,89,67,21,232,18,67,34,82,90,67,112,95,18,67,246,112,90,67,98,122,59,18,67,0,121,90,67,13,24,18,67,31,125,90,67,27,246,17,67,31,125,90,
			67,98,95,189,17,67,31,125,90,67,13,137,17,67,167,113,90,67,180,90,17,67,152,91,90,67,98,86,155,16,67,220,0,90,67,52,117,16,67,176,11,89,67,252,5,17,67,100,57,88,67,98,116,88,17,67,22,194,87,67,125,215,17,67,186,107,87,67,19,98,18,67,99,76,87,67,98,182,
			192,18,67,22,55,87,67,84,28,19,67,230,61,87,67,90,103,19,67,123,97,87,67,98,145,38,20,67,138,188,87,67,119,76,20,67,133,177,88,67,195,187,19,67,142,131,89,67,99,109,114,249,27,67,168,91,90,67,98,4,203,27,67,185,113,90,67,199,150,27,67,47,125,90,67,10,
			94,27,67,47,125,90,67,98,23,60,27,67,47,125,90,67,151,24,27,67,16,121,90,67,181,244,26,67,6,113,90,67,98,16,108,26,67,51,82,90,67,158,233,25,67,85,249,89,67,99,152,25,67,162,131,89,67,98,173,7,25,67,152,177,88,67,149,45,25,67,157,188,87,67,183,236,25,
			67,175,97,87,67,98,210,55,26,67,25,62,87,67,91,147,26,67,71,55,87,67,254,241,26,67,151,76,87,67,98,168,124,27,67,221,107,87,67,176,251,27,67,73,194,87,67,21,78,28,67,152,57,88,67,98,242,222,28,67,211,11,89,67,208,184,28,67,240,0,90,67,113,249,27,67,171,
			91,90,67,99,101,0,0 };


		path.loadPathFromData(pathData, sizeof(pathData));
	}
	else if (name == "Copy")
	{
		static const unsigned char pathData[] = { 110,109,0,0,240,65,0,0,0,0,108,0,0,32,65,0,0,0,0,98,4,86,14,65,0,0,0,0,0,0,0,65,184,30,101,63,0,0,0,65,0,0,0,64,108,0,0,0,65,0,0,0,65,108,0,0,0,64,0,0,0,65,98,66,96,101,63,0,0,0,65,0,0,0,0,236,81,14,65,0,0,0,0,0,0,32,65,108,0,0,0,0,0,0,240,65,98,0,0,
			0,0,254,212,248,65,66,96,101,63,0,0,0,66,0,0,0,64,0,0,0,66,108,0,0,176,65,0,0,0,66,98,10,215,184,65,0,0,0,66,0,0,192,65,254,212,248,65,0,0,192,65,0,0,240,65,108,0,0,192,65,0,0,192,65,108,0,0,240,65,0,0,192,65,98,254,212,248,65,0,0,192,65,0,0,0,66,254,
			212,184,65,0,0,0,66,0,0,176,65,108,0,0,0,66,0,0,0,64,98,0,0,0,66,184,30,101,63,254,212,248,65,0,0,0,0,0,0,240,65,0,0,0,0,99,109,0,0,168,65,0,0,232,65,108,0,0,64,64,0,0,232,65,108,0,0,64,64,0,0,48,65,108,0,0,0,65,0,0,48,65,108,0,0,0,65,0,0,176,65,98,0,
			0,0,65,10,215,184,65,4,86,14,65,0,0,192,65,0,0,32,65,0,0,192,65,108,0,0,168,65,0,0,192,65,108,0,0,168,65,0,0,232,65,99,109,0,0,232,65,0,0,168,65,108,0,0,48,65,0,0,168,65,108,0,0,48,65,0,0,64,64,108,0,0,232,65,0,0,64,64,108,0,0,232,65,0,0,168,65,99,101,
			0,0 };


		path.loadPathFromData(pathData, sizeof(pathData));
	}
	else if (name == "Paste")
	{
		path.loadPathFromData(EditorIcons::pasteIcon, sizeof(EditorIcons::pasteIcon));
	}
	else if (name == "Delete")
	{
		static const unsigned char pathData[] = { 110,109,77,57,87,67,170,11,183,66,108,77,57,45,67,170,11,183,66,98,152,91,41,67,170,11,183,66,77,57,38,67,64,80,189,66,77,57,38,67,170,11,197,66,108,77,57,38,67,84,182,201,66,108,77,57,94,67,84,182,201,66,108,77,57,94,67,170,11,197,66,98,77,57,94,67,
			64,80,189,66,2,23,91,67,170,11,183,66,77,57,87,67,170,11,183,66,99,109,70,255,74,67,85,182,173,66,108,198,6,76,67,0,113,188,66,108,212,107,56,67,0,113,188,66,108,74,115,57,67,85,182,173,66,108,70,255,74,67,85,182,173,66,109,162,142,75,67,0,97,164,66,
			108,248,227,56,67,0,97,164,66,98,43,247,54,67,0,97,164,66,40,44,53,67,162,127,167,66,252,231,52,67,192,79,171,66,108,168,138,51,67,148,215,190,66,98,115,70,51,67,178,167,194,66,214,161,52,67,85,198,197,66,162,142,54,67,85,198,197,66,108,248,227,77,67,
			85,198,197,66,98,197,208,79,67,85,198,197,66,40,44,81,67,178,167,194,66,252,231,80,67,148,215,190,66,108,178,138,79,67,192,79,171,66,98,115,70,79,67,162,127,167,66,111,123,77,67,0,97,164,66,163,142,75,67,0,97,164,66,108,163,142,75,67,0,97,164,66,99,109,
			248,99,88,67,170,11,211,66,108,162,14,44,67,170,11,211,66,98,145,125,41,67,170,11,211,66,166,148,39,67,118,58,215,66,31,208,39,67,55,87,220,66,108,122,162,43,67,110,53,24,67,98,243,221,43,67,197,195,26,67,59,40,46,67,43,219,28,67,77,185,48,67,43,219,
			28,67,108,77,185,83,67,43,219,28,67,98,94,74,86,67,43,219,28,67,166,148,88,67,198,195,26,67,32,208,88,67,110,53,24,67,108,104,162,92,67,56,87,220,66,98,244,221,92,67,119,58,215,66,9,245,90,67,171,11,211,66,248,99,88,67,171,11,211,66,99,109,248,227,56,
			67,213,133,19,67,108,248,227,49,67,213,133,19,67,108,162,142,47,67,84,182,229,66,108,248,227,56,67,84,182,229,66,108,248,227,56,67,213,133,19,67,99,109,248,227,70,67,213,133,19,67,108,162,142,61,67,213,133,19,67,108,162,142,61,67,84,182,229,66,108,248,
			227,70,67,84,182,229,66,108,248,227,70,67,213,133,19,67,99,109,162,142,82,67,213,133,19,67,108,162,142,75,67,213,133,19,67,108,162,142,75,67,84,182,229,66,108,248,227,84,67,84,182,229,66,108,162,142,82,67,213,133,19,67,99,101,0,0 };

		path.loadPathFromData(pathData, sizeof(pathData));
	}
	else if (name == "Duplicate")
	{
		static const unsigned char pathData[] = { 110,109,206,114,185,66,122,177,183,193,108,206,119,137,66,122,177,183,193,98,135,4,138,66,6,165,202,193,183,175,139,66,109,120,220,193,97,121,142,66,178,43,237,193,98,9,67,145,66,240,222,253,193,145,125,150,66,104,202,8,194,250,40,158,66,171,38,20,194,
			98,117,216,162,66,33,29,27,194,185,215,165,66,188,102,32,194,200,38,167,66,127,3,36,194,98,211,117,168,66,56,160,39,194,90,29,169,66,14,14,43,194,92,29,169,66,1,77,46,194,98,90,29,169,66,236,206,49,194,128,119,168,66,48,206,52,194,207,43,167,66,208,74,
			55,194,98,26,224,165,66,99,199,57,194,246,62,164,66,176,5,59,194,100,72,162,66,182,5,59,194,98,180,61,160,66,176,5,59,194,131,146,158,66,86,189,57,194,209,70,157,66,169,44,55,194,98,28,251,155,66,240,155,52,194,77,28,155,66,168,20,48,194,99,170,154,66,
			210,150,41,194,108,90,165,138,66,57,46,44,194,98,45,70,139,66,91,47,53,194,5,109,140,66,149,54,60,194,228,25,142,66,232,67,65,194,98,193,198,143,66,46,81,70,194,134,35,146,66,238,48,74,194,50,48,149,66,43,227,76,194,98,221,60,152,66,88,149,79,194,103,
			117,156,66,115,238,80,194,210,217,161,66,122,238,80,194,98,137,122,167,66,114,238,80,194,72,219,171,66,25,166,79,194,17,252,174,66,109,21,77,194,98,212,28,178,66,178,132,74,194,186,146,180,66,49,148,70,194,194,93,182,66,232,67,65,194,98,196,40,184,66,
			146,243,59,194,71,14,185,66,22,255,53,194,74,14,185,66,115,102,47,194,98,71,14,185,66,141,98,40,194,67,7,184,66,22,175,33,194,62,249,181,66,14,76,27,194,98,51,235,179,66,253,232,20,194,161,46,176,66,28,229,13,194,134,195,170,66,108,64,6,194,98,74,139,
			167,66,60,205,1,194,33,100,165,66,160,95,253,193,11,78,164,66,77,208,249,193,98,241,55,163,66,242,64,246,193,68,241,161,66,41,152,241,193,4,122,160,66,242,213,235,193,108,206,114,185,66,242,213,235,193,99,109,241,134,21,66,230,247,78,194,108,101,182,
			60,66,230,247,78,194,108,52,33,81,66,101,141,43,194,108,48,235,100,66,230,247,78,194,108,16,219,133,66,230,247,78,194,108,27,231,103,66,177,54,23,194,108,72,139,135,66,122,177,183,193,108,20,30,103,66,122,177,183,193,108,71,108,80,66,78,213,0,194,108,
			96,166,57,66,122,177,183,193,108,51,234,17,66,122,177,183,193,108,96,166,57,66,132,215,23,194,99,101,0,0 };

		path.loadPathFromData(pathData, sizeof(pathData));
	}
	else if (name == "Fill Note Gaps")
	{
		static const unsigned char pathData[] = { 110,109,0,0,0,0,39,49,12,65,108,221,36,12,65,26,47,140,65,108,196,32,12,65,200,118,70,65,108,133,235,169,65,249,126,70,65,108,133,235,169,65,26,47,140,65,108,0,0,240,65,39,49,12,65,108,133,235,169,65,0,0,0,0,108,133,235,169,65,217,206,163,64,108,196,
			32,12,65,217,206,163,64,108,221,36,12,65,0,0,0,0,99,101,0,0 };

		path.loadPathFromData(pathData, sizeof(pathData));
	}
	else if (name == "Fill Velocity Gaps")
	{
		static const unsigned char pathData[] = { 110,109,244,253,111,65,203,161,199,192,108,204,161,199,64,222,79,33,64,108,82,184,53,65,124,63,33,64,108,33,176,53,65,37,6,112,65,108,203,161,199,64,37,6,112,65,108,244,253,111,65,142,23,190,65,108,142,23,190,65,38,6,112,65,108,216,35,149,65,38,6,112,
			65,108,216,35,149,65,128,63,33,64,108,142,23,190,65,226,79,33,64,99,101,0,0 };

		path.loadPathFromData(pathData, sizeof(pathData));
	}
	else if (name == "New SampleMap")
	{
		static const unsigned char pathData[] = { 110,109,202,148,45,67,240,190,35,66,108,166,167,28,67,200,20,192,65,98,54,23,28,67,71,145,187,65,170,98,27,67,167,46,184,65,2,138,26,67,167,46,184,65,108,234,246,250,66,167,46,184,65,98,74,148,247,66,167,46,184,65,25,194,244,66,105,119,195,65,25,194,
			244,66,236,1,209,65,108,25,194,244,66,17,31,174,66,98,25,194,244,66,178,129,177,66,74,148,247,66,170,11,180,66,234,246,250,66,170,11,180,66,108,37,119,43,67,170,11,180,66,98,117,40,45,67,170,11,180,66,142,145,46,67,178,129,177,66,142,145,46,67,17,31,
			174,66,108,142,145,46,67,130,53,44,66,98,142,145,46,67,225,210,40,66,58,37,46,67,177,0,38,66,202,148,45,67,240,190,35,66,99,109,189,92,40,67,64,234,167,66,108,221,149,0,67,64,234,167,66,108,221,149,0,67,48,213,233,65,108,252,199,18,67,48,213,233,65,108,
			252,199,18,67,21,179,61,66,98,252,199,18,67,86,120,68,66,20,49,20,67,183,28,74,66,101,226,21,67,183,28,74,66,108,188,92,40,67,183,28,74,66,108,188,92,40,67,64,234,167,66,99,109,205,252,24,67,115,73,49,66,108,205,252,24,67,48,213,233,65,108,5,69,25,67,
			48,213,233,65,108,188,92,40,67,115,73,49,66,108,205,252,24,67,115,73,49,66,99,101,0,0 };


		path.loadPathFromData(pathData, sizeof(pathData));
	}
	else if (name == "Load SampleMap")
	{
		static const unsigned char pathData[] = { 110,109,249,109,142,67,168,198,186,65,98,181,96,143,67,168,198,186,65,200,125,144,67,164,246,176,65,160,229,144,67,138,149,163,65,108,36,201,152,67,123,139,22,193,98,236,65,153,67,185,157,51,193,74,146,152,67,183,237,68,193,18,86,151,67,183,237,68,193,
			108,192,94,143,67,183,237,68,193,108,192,94,143,67,63,138,155,193,98,192,94,143,67,11,61,175,193,86,94,142,67,168,67,191,193,42,35,141,67,168,67,191,193,108,82,69,115,67,168,67,191,193,108,124,96,112,67,102,7,252,193,98,90,19,112,67,129,43,1,194,189,
			81,111,67,57,66,3,194,116,121,110,67,57,66,3,194,108,178,166,94,67,57,66,3,194,98,17,28,94,67,57,66,3,194,232,150,93,67,64,101,2,194,238,52,93,67,63,219,0,194,98,54,211,92,67,125,162,254,193,192,156,92,67,58,121,250,193,136,157,92,67,27,34,246,193,108,
			112,167,92,67,64,138,155,193,108,160,147,92,67,88,40,151,65,98,160,147,92,67,35,219,170,65,116,148,94,67,193,225,186,65,205,10,97,67,193,225,186,65,109,149,85,141,67,142,233,68,193,108,122,237,119,67,142,233,68,193,98,148,83,118,67,142,233,68,193,213,
			207,116,67,88,54,48,193,193,2,116,67,82,135,22,193,108,72,211,101,67,29,79,154,65,108,205,10,97,67,29,79,154,65,98,76,211,96,67,29,79,154,65,244,165,96,67,98,228,152,65,244,165,96,67,89,40,151,65,108,244,165,96,67,45,161,155,193,108,113,165,96,67,182,
			239,229,193,108,96,18,109,67,182,239,229,193,108,54,247,111,67,248,43,169,193,98,88,68,112,67,91,220,162,193,245,5,113,67,237,174,158,193,61,222,113,67,237,174,158,193,108,42,35,141,67,237,174,158,193,98,234,62,141,67,237,174,158,193,150,85,141,67,50,
			68,157,193,150,85,141,67,42,136,155,193,108,150,85,141,67,140,233,68,193,99,101,0,0 };

		path.loadPathFromData(pathData, sizeof(pathData));
	}
	else if (name == "Save SampleMap")
	{
		static const unsigned char pathData[] = { 110,109,0,0,144,65,0,0,0,64,108,0,0,176,65,0,0,0,64,108,0,0,176,65,0,0,48,65,108,0,0,144,65,0,0,48,65,99,109,0,0,192,64,0,0,208,65,108,0,0,208,65,0,0,208,65,108,0,0,208,65,0,0,224,65,108,0,0,192,64,0,0,224,65,99,109,0,0,192,64,0,0,176,65,108,0,0,208,
			65,0,0,176,65,108,0,0,208,65,0,0,192,65,108,0,0,192,64,0,0,192,65,99,109,0,0,192,64,0,0,144,65,108,0,0,208,65,0,0,144,65,108,0,0,208,65,0,0,160,65,108,0,0,192,64,0,0,160,65,99,109,0,0,208,65,0,0,0,0,108,0,0,192,65,0,0,0,0,108,0,0,192,65,0,0,80,65,108,
			0,0,0,65,0,0,80,65,108,0,0,0,65,0,0,0,0,108,0,0,0,0,0,0,0,0,108,0,0,0,0,0,0,0,66,108,0,0,0,66,0,0,0,66,108,0,0,0,66,0,0,192,64,108,0,0,208,65,0,0,0,0,99,109,0,0,224,65,0,0,240,65,108,0,0,128,64,0,0,240,65,108,0,0,128,64,0,0,128,65,108,0,0,224,65,0,0,
			128,65,108,0,0,224,65,0,0,240,65,99,101,0,0 };

		path.loadPathFromData(pathData, sizeof(pathData));
	}
	else if (name == "Import SFZ file format")
	{
		static const unsigned char pathData[] = { 110,109,0,192,152,67,0,120,186,67,98,79,191,150,67,0,120,186,67,0,8,149,67,24,22,188,67,0,8,149,67,0,28,190,67,108,0,8,149,67,0,212,215,67,98,0,8,149,67,232,217,217,67,79,191,150,67,0,120,219,67,0,192,152,67,0,120,219,67,108,0,164,180,67,0,120,219,67,
			98,177,164,182,67,0,120,219,67,0,92,184,67,232,217,217,67,0,92,184,67,0,212,215,67,108,0,92,184,67,0,28,190,67,98,0,92,184,67,24,22,188,67,177,164,182,67,0,120,186,67,0,164,180,67,0,120,186,67,108,0,192,152,67,0,120,186,67,99,109,0,192,152,67,0,160,189,
			67,108,0,164,180,67,0,160,189,67,98,145,3,181,67,0,160,189,67,0,52,181,67,164,216,189,67,0,52,181,67,0,28,190,67,108,0,52,181,67,0,212,215,67,98,0,52,181,67,92,23,216,67,145,3,181,67,0,80,216,67,0,164,180,67,0,80,216,67,108,0,192,152,67,0,80,216,67,98,
			111,96,152,67,0,80,216,67,0,48,152,67,92,23,216,67,0,48,152,67,0,212,215,67,108,0,48,152,67,0,28,190,67,98,0,48,152,67,164,216,189,67,111,96,152,67,0,160,189,67,0,192,152,67,0,160,189,67,99,109,99,184,179,67,236,106,199,67,98,98,184,179,67,61,117,199,
			67,69,150,179,67,208,154,199,67,11,82,179,67,168,219,199,67,98,208,13,179,67,129,28,200,67,236,181,178,67,110,110,200,67,95,74,178,67,112,209,200,67,98,209,222,177,67,115,52,201,67,166,101,177,67,214,160,201,67,219,222,176,67,152,22,202,67,98,16,88,176,
			67,91,140,202,67,27,210,175,67,66,1,203,67,251,76,175,67,76,117,203,67,98,219,199,174,67,87,233,203,67,178,72,174,67,215,84,204,67,127,207,173,67,204,183,204,67,98,76,86,173,67,194,26,205,67,110,242,172,67,63,105,205,67,227,163,172,67,68,163,205,67,108,
			131,184,172,67,149,173,205,67,98,120,27,173,67,144,115,205,67,251,163,173,67,141,86,205,67,11,82,174,67,141,86,205,67,98,229,105,174,67,141,86,205,67,72,144,174,67,59,94,205,67,51,197,174,67,153,109,205,67,98,29,250,174,67,246,124,205,67,149,49,175,67,
			41,141,205,67,155,107,175,67,49,158,205,67,98,160,165,175,67,57,175,205,67,60,220,175,67,108,191,205,67,111,15,176,67,201,206,205,67,98,162,66,176,67,38,222,205,67,126,102,176,67,213,229,205,67,3,123,176,67,213,229,205,67,98,189,153,176,67,213,229,205,
			67,77,185,176,67,48,216,205,67,179,217,176,67,229,188,205,67,98,24,250,176,67,155,161,205,67,74,10,177,67,152,132,205,67,75,10,177,67,221,101,205,67,98,75,10,177,67,200,84,205,67,11,6,177,67,82,58,205,67,143,253,176,67,125,22,205,67,98,18,245,176,67,
			168,242,204,67,211,240,176,67,50,216,204,67,211,240,176,67,29,199,204,67,98,211,240,176,67,152,103,204,67,10,24,177,67,213,55,204,67,123,102,177,67,213,55,204,67,98,154,235,177,67,213,55,204,67,42,46,178,67,101,122,204,67,43,46,178,67,133,255,204,67,
			98,42,46,178,67,122,98,205,67,178,25,178,67,85,192,205,67,195,240,177,67,21,25,206,67,98,210,199,177,67,213,113,206,67,55,145,177,67,125,191,206,67,239,76,177,67,13,2,207,67,98,167,8,177,67,157,68,207,67,71,185,176,67,93,122,207,67,207,94,176,67,77,163,
			207,67,98,87,4,176,67,61,204,207,67,88,167,175,67,181,224,207,67,211,71,175,67,181,224,207,67,98,93,10,175,67,181,224,207,67,55,203,174,67,199,212,207,67,95,138,174,67,237,188,207,67,98,135,73,174,67,18,165,207,67,247,6,174,67,164,138,207,67,175,194,
			173,67,161,109,207,67,98,103,126,173,67,158,80,207,67,74,57,173,67,41,54,207,67,87,243,172,67,65,30,207,67,98,100,173,172,67,89,6,207,67,136,102,172,67,101,250,206,67,195,30,172,67,101,250,206,67,98,67,249,171,67,101,250,206,67,119,207,171,67,232,2,207,
			67,95,161,171,67,241,19,207,67,98,71,115,171,67,249,36,207,67,195,71,171,67,228,54,207,67,211,30,171,67,177,73,207,67,98,227,245,170,67,127,92,207,67,14,210,170,67,112,110,207,67,83,179,170,67,133,127,207,67,98,152,148,170,67,154,144,207,67,203,129,170,
			67,37,153,207,67,235,122,170,67,37,153,207,67,98,151,119,170,67,37,153,207,67,175,107,170,67,110,151,207,67,55,87,170,67,252,147,207,67,98,191,66,170,67,139,144,207,67,107,45,170,67,71,140,207,67,59,23,170,67,44,135,207,67,98,11,1,170,67,17,130,207,67,
			140,236,169,67,239,124,207,67,191,217,169,67,200,119,207,67,98,242,198,169,67,159,114,207,67,139,189,169,67,99,110,207,67,139,189,169,67,12,107,207,67,98,139,189,169,67,151,45,207,67,27,221,169,67,115,232,206,67,59,28,170,67,160,155,206,67,98,91,91,170,
			67,205,78,206,67,42,197,170,67,25,234,205,67,167,89,171,67,132,109,205,67,98,36,238,171,67,239,240,204,67,222,175,172,67,54,82,204,67,211,158,173,67,88,145,203,67,98,200,141,174,67,123,208,202,67,195,182,175,67,156,219,201,67,195,25,177,67,188,178,200,
			67,98,120,254,176,67,243,188,200,67,211,170,176,67,12,194,200,67,211,30,176,67,12,194,200,67,98,136,3,176,67,12,194,200,67,37,221,175,67,128,191,200,67,171,171,175,67,100,186,200,67,98,48,122,175,67,73,181,200,67,253,70,175,67,82,175,200,67,19,18,175,
			67,128,168,200,67,98,40,221,174,67,173,161,200,67,246,169,174,67,176,155,200,67,123,120,174,67,136,150,200,67,98,0,71,174,67,95,145,200,67,144,32,174,67,205,142,200,67,43,5,174,67,205,142,200,67,98,203,216,173,67,205,142,200,67,71,173,173,67,43,158,200,
			67,159,130,173,67,229,188,200,67,98,247,87,173,67,160,219,200,67,7,47,173,67,160,254,200,67,207,7,173,67,229,37,201,67,98,151,224,172,67,43,77,201,67,194,188,172,67,43,112,201,67,79,156,172,67,229,142,201,67,98,220,123,172,67,160,173,201,67,182,95,172,
			67,253,188,201,67,219,71,172,67,253,188,201,67,98,102,10,172,67,253,188,201,67,171,235,171,67,232,171,201,67,171,235,171,67,189,137,201,67,98,171,235,171,67,35,100,201,67,113,254,171,67,101,40,201,67,255,35,172,67,133,214,200,67,98,140,73,172,67,166,
			132,200,67,193,118,172,67,227,49,200,67,159,171,172,67,61,222,199,67,98,124,224,172,67,152,138,199,67,202,24,173,67,54,65,199,67,135,84,173,67,21,2,199,67,98,68,144,173,67,245,194,198,67,83,196,173,67,102,163,198,67,179,240,173,67,101,163,198,67,98,248,
			93,174,67,102,163,198,67,43,180,174,67,13,171,198,67,75,243,174,67,93,186,198,67,98,107,50,175,67,174,201,198,67,40,110,175,67,224,217,198,67,131,166,175,67,245,234,198,67,98,221,222,175,67,11,252,198,67,40,29,176,67,68,12,199,67,99,97,176,67,161,27,
			199,67,98,157,165,176,67,255,42,199,67,205,1,177,67,174,50,199,67,243,117,177,67,173,50,199,67,98,146,148,177,67,174,50,199,67,203,187,177,67,255,42,199,67,155,235,177,67,161,27,199,67,98,107,27,178,67,68,12,199,67,236,76,178,67,11,252,198,67,31,128,
			178,67,245,234,198,67,98,81,179,178,67,224,217,198,67,246,227,178,67,174,201,198,67,15,18,179,67,93,186,198,67,98,38,64,179,67,14,171,198,67,216,100,179,67,102,163,198,67,35,128,179,67,101,163,198,67,98,141,148,179,67,102,163,198,67,8,163,179,67,150,
			185,198,67,147,171,179,67,245,229,198,67,98,29,180,179,67,86,18,199,67,98,184,179,67,213,55,199,67,99,184,179,67,117,86,199,67,108,99,184,179,67,237,106,199,67,99,109,187,197,173,67,100,35,196,67,98,117,88,173,67,101,35,196,67,220,248,172,67,91,6,196,
			67,239,166,172,67,72,204,195,67,98,1,85,172,67,54,146,195,67,10,44,172,67,205,72,195,67,11,44,172,67,12,240,194,67,98,10,44,172,67,226,130,194,67,74,95,172,67,26,25,194,67,203,197,172,67,180,178,193,67,108,99,100,173,67,244,19,193,67,98,98,100,173,67,
			192,9,193,67,181,92,173,67,96,0,193,67,87,77,173,67,212,247,192,67,98,249,61,173,67,74,239,192,67,14,44,173,67,156,231,192,67,151,23,173,67,200,224,192,67,98,30,3,173,67,245,217,192,67,202,237,172,67,215,212,192,67,155,215,172,67,100,209,192,67,98,106,
			193,172,67,246,205,192,67,127,175,172,67,63,204,192,67,219,161,172,67,59,204,192,67,98,85,66,172,67,172,207,192,67,146,239,171,67,91,250,192,67,147,169,171,67,71,76,193,67,98,147,99,171,67,53,158,193,67,144,35,171,67,109,11,194,67,139,233,170,67,239,
			147,194,67,98,133,175,170,67,114,28,195,67,92,118,170,67,44,187,195,67,15,62,170,67,27,112,196,67,98,193,5,170,67,12,37,197,67,40,201,169,67,49,228,197,67,67,136,169,67,139,173,198,67,108,243,79,170,67,139,173,198,67,98,200,185,170,67,140,173,198,67,
			66,14,171,67,112,174,198,67,99,77,171,67,33,176,198,67,98,131,140,171,67,216,177,198,67,40,189,171,67,103,180,198,67,83,223,171,67,201,183,198,67,98,125,1,172,67,45,187,198,67,137,24,172,67,70,192,198,67,119,36,172,67,25,199,198,67,98,101,48,172,67,236,
			205,198,67,90,54,172,67,126,214,198,67,91,54,172,67,205,224,198,67,98,90,54,172,67,62,228,198,67,164,52,172,67,191,242,198,67,50,49,172,67,81,12,199,67,98,193,45,172,67,228,37,199,67,238,38,172,67,47,65,199,67,186,28,172,67,49,94,199,67,98,131,18,172,
			67,52,123,199,67,187,5,172,67,170,149,199,67,94,246,171,67,145,173,199,67,98,0,231,171,67,121,197,199,67,100,211,171,67,109,209,199,67,138,187,171,67,109,209,199,67,108,138,59,169,67,109,209,199,67,98,57,90,168,67,104,93,202,67,221,132,167,67,165,138,
			204,67,118,187,166,67,37,89,206,67,98,14,242,165,67,165,39,208,67,139,35,165,67,134,162,209,67,238,79,164,67,201,201,210,67,98,81,124,163,67,11,241,211,67,227,155,162,67,202,201,212,67,166,174,161,67,5,84,213,67,98,105,193,160,67,63,222,213,67,77,182,
			159,67,93,35,214,67,82,141,158,67,93,35,214,67,98,168,69,158,67,92,35,214,67,102,1,158,67,219,20,214,67,142,192,157,67,217,247,213,67,98,182,127,157,67,214,218,213,67,105,71,157,67,145,179,213,67,166,23,157,67,9,130,213,67,98,228,231,156,67,128,80,213,
			67,129,193,156,67,81,23,213,67,126,164,156,67,121,214,212,67,98,123,135,156,67,161,149,212,67,250,120,156,67,95,81,212,67,250,120,156,67,181,9,212,67,98,250,120,156,67,117,139,211,67,129,158,156,67,15,37,211,67,142,233,156,67,133,214,210,67,98,156,52,
			157,67,250,135,210,67,152,151,157,67,181,96,210,67,130,18,158,67,181,96,210,67,98,120,117,158,67,181,96,210,67,94,199,158,67,75,128,210,67,54,8,159,67,121,191,210,67,98,14,73,159,67,166,254,210,67,122,105,159,67,183,79,211,67,122,105,159,67,173,178,211,
			67,98,122,105,159,67,215,212,211,67,64,95,159,67,242,252,211,67,198,74,159,67,253,42,212,67,98,78,54,159,67,7,89,212,67,73,31,159,67,175,131,212,67,182,5,159,67,245,170,212,67,98,36,236,158,67,58,210,212,67,30,213,158,67,173,242,212,67,166,192,158,67,
			77,12,213,67,98,46,172,158,67,236,37,213,67,242,161,158,67,5,49,213,67,242,161,158,67,149,45,213,67,98,242,161,158,67,201,55,213,67,41,172,158,67,229,60,213,67,146,192,158,67,229,60,213,67,98,29,15,159,67,229,60,213,67,151,134,159,67,133,16,213,67,2,
			39,160,67,197,183,212,67,98,125,158,160,67,138,115,212,67,111,7,161,67,151,10,212,67,218,97,161,67,237,124,211,67,98,69,188,161,67,66,239,210,67,110,24,162,67,110,40,210,67,86,118,162,67,113,40,209,67,98,62,212,162,67,116,40,208,67,55,61,163,67,194,230,
			206,67,66,177,163,67,93,99,205,67,98,76,37,164,67,248,223,203,67,90,182,164,67,168,4,202,67,106,100,165,67,109,209,199,67,108,74,54,163,67,109,209,199,67,98,69,252,162,67,109,209,199,67,66,223,162,67,197,166,199,67,66,223,162,67,117,81,199,67,98,66,223,
			162,67,48,228,198,67,23,3,163,67,142,173,198,67,194,74,163,67,141,173,198,67,108,18,213,165,67,141,173,198,67,98,242,38,166,67,99,180,197,67,123,140,166,67,140,208,196,67,174,5,167,67,9,2,196,67,98,224,126,167,67,135,51,195,67,148,6,168,67,227,130,194,
			67,202,156,168,67,29,240,193,67,98,255,50,169,67,89,93,193,67,33,213,169,67,219,235,192,67,50,131,170,67,165,155,192,67,98,66,49,171,67,113,75,192,67,36,230,171,67,86,35,192,67,218,161,172,67,85,35,192,67,98,121,1,173,67,86,35,192,67,241,91,173,67,31,
			48,192,67,66,177,173,67,177,73,192,67,98,145,6,174,67,69,99,192,67,129,82,174,67,26,135,192,67,18,149,174,67,49,181,192,67,98,161,215,174,67,74,227,192,67,140,12,175,67,194,26,193,67,210,51,175,67,153,91,193,67,98,22,91,175,67,114,156,193,67,185,110,
			175,67,219,229,193,67,186,110,175,67,213,55,194,67,98,185,110,175,67,43,199,194,67,158,70,175,67,238,60,195,67,106,246,174,67,29,153,195,67,98,52,166,174,67,77,245,195,67,164,64,174,67,102,35,196,67,186,197,173,67,101,35,196,67,99,109,139,191,162,67,
			180,96,200,67,98,138,191,162,67,63,175,200,67,208,160,162,67,66,239,200,67,91,99,162,67,188,32,201,67,98,229,37,162,67,55,82,201,67,59,222,161,67,244,106,201,67,91,140,161,67,244,106,201,67,98,117,75,161,67,244,106,201,67,208,26,161,67,151,91,201,67,
			107,250,160,67,220,60,201,67,98,5,218,160,67,34,30,201,67,72,193,160,67,227,246,200,67,51,176,160,67,32,199,200,67,98,29,159,160,67,94,151,200,67,120,145,160,67,7,101,200,67,67,135,160,67,28,48,200,67,98,12,125,160,67,50,251,199,67,104,111,160,67,219,
			200,199,67,83,94,160,67,24,153,199,67,98,61,77,160,67,86,105,199,67,157,51,160,67,23,66,199,67,115,17,160,67,92,35,199,67,98,72,239,159,67,162,4,199,67,21,188,159,67,68,245,198,67,219,119,159,67,68,245,198,67,98,86,140,158,67,68,245,198,67,147,22,158,
			67,245,118,199,67,147,22,158,67,84,122,200,67,98,147,22,158,67,244,217,200,67,16,101,158,67,44,71,201,67,11,2,159,67,252,193,201,67,98,144,162,159,67,87,64,202,67,8,32,160,67,18,165,202,67,115,122,160,67,44,240,202,67,98,221,212,160,67,71,59,203,67,67,
			24,161,67,251,124,203,67,163,68,161,67,72,181,203,67,98,3,113,161,67,150,237,203,67,84,140,161,67,86,35,204,67,151,150,161,67,136,86,204,67,98,216,160,161,67,187,137,204,67,251,165,161,67,68,204,204,67,251,165,161,67,36,30,205,67,98,251,165,161,67,218,
			142,205,67,97,140,161,67,184,242,205,67,47,89,161,67,192,73,206,67,98,252,37,161,67,200,160,206,67,74,222,160,67,7,235,206,67,27,130,160,67,124,40,207,67,98,235,37,160,67,242,101,207,67,136,185,159,67,3,148,207,67,243,60,159,67,176,178,207,67,98,93,192,
			158,67,93,209,207,67,176,56,158,67,180,224,207,67,235,165,157,67,180,224,207,67,98,112,46,157,67,180,224,207,67,23,188,156,67,15,211,207,67,223,78,156,67,196,183,207,67,98,167,225,155,67,121,156,207,67,20,130,155,67,59,117,207,67,39,48,155,67,8,66,207,
			67,98,58,222,154,67,213,14,207,67,134,156,154,67,103,209,206,67,11,107,154,67,188,137,206,67,98,145,57,154,67,18,66,206,67,211,32,154,67,220,241,205,67,211,32,154,67,28,153,205,67,98,211,32,154,67,204,183,204,67,214,131,154,67,36,71,204,67,219,73,155,
			67,36,71,204,67,98,155,162,155,67,36,71,204,67,105,233,155,67,75,99,204,67,71,30,156,67,152,155,204,67,98,36,83,156,67,229,211,204,67,147,109,156,67,108,28,205,67,147,109,156,67,44,117,205,67,98,147,109,156,67,33,141,205,67,120,104,156,67,76,175,205,
			67,67,94,156,67,172,219,205,67,98,12,84,156,67,12,8,206,67,243,78,156,67,113,40,206,67,243,78,156,67,220,60,206,67,98,243,78,156,67,50,129,206,67,128,116,156,67,140,185,206,67,155,191,156,67,236,229,206,67,98,181,10,157,67,76,18,207,67,51,89,157,67,124,
			40,207,67,19,171,157,67,124,40,207,67,98,104,198,158,67,124,40,207,67,19,84,159,67,119,168,206,67,19,84,159,67,108,168,205,67,98,19,84,159,67,215,113,205,67,227,61,159,67,95,58,205,67,131,17,159,67,4,2,205,67,98,35,229,158,67,170,201,204,67,207,172,158,
			67,164,143,204,67,135,104,158,67,244,83,204,67,98,63,36,158,67,68,24,204,67,220,218,157,67,178,219,203,67,95,140,157,67,60,158,203,67,98,225,61,157,67,199,96,203,67,127,244,156,67,232,31,203,67,55,176,156,67,160,219,202,67,98,239,107,156,67,88,151,202,
			67,155,51,156,67,95,81,202,67,59,7,156,67,180,9,202,67,98,219,218,155,67,10,194,201,67,171,196,155,67,239,118,201,67,171,196,155,67,100,40,201,67,98,171,196,155,67,58,187,200,67,104,221,155,67,69,88,200,67,227,14,156,67,132,255,199,67,98,93,64,156,67,
			196,166,199,67,202,131,156,67,170,91,199,67,39,217,156,67,52,30,199,67,98,132,46,157,67,191,224,198,67,128,145,157,67,246,176,198,67,27,2,158,67,216,142,198,67,98,181,114,158,67,187,108,198,67,232,235,158,67,172,91,198,67,179,109,159,67,172,91,198,67,
			98,216,119,160,67,172,91,198,67,5,72,161,67,226,136,198,67,59,222,161,67,76,227,198,67,98,112,116,162,67,183,61,199,67,138,191,162,67,218,188,199,67,139,191,162,67,180,96,200,67,99,101,0,0 };

		path.loadPathFromData(pathData, sizeof(pathData));
	}
	else if (name == "Save as Monolith")
	{
		static const unsigned char pathData[] = { 110,109,166,92,181,67,88,209,231,193,98,31,230,165,67,88,209,231,193,166,92,153,67,88,206,121,192,166,92,153,67,168,46,216,65,98,166,92,153,67,142,203,103,66,31,230,165,67,170,11,166,66,166,92,181,67,170,11,166,66,98,45,211,196,67,170,11,166,66,166,92,
			209,67,142,203,103,66,166,92,209,67,168,46,216,65,98,166,92,209,67,88,206,121,192,45,211,196,67,88,209,231,193,166,92,181,67,88,209,231,193,99,109,166,92,181,67,88,109,101,66,108,70,135,168,67,168,46,216,65,108,230,177,176,67,168,46,216,65,108,230,177,
			176,67,242,42,122,191,108,70,7,186,67,242,42,122,191,108,70,7,186,67,168,46,216,65,108,197,49,194,67,168,46,216,65,108,166,92,181,67,88,109,101,66,99,101,0,0 };

		path.loadPathFromData(pathData, sizeof(pathData));
	}
	else if (name == "Zoom In")
	{
		static const unsigned char pathData[] = { 110,109,0,0,0,65,0,0,128,64,108,0,0,192,64,0,0,128,64,108,0,0,192,64,0,0,192,64,108,0,0,128,64,0,0,192,64,108,0,0,128,64,0,0,0,65,108,0,0,192,64,0,0,0,65,108,0,0,192,64,0,0,32,65,108,0,0,0,65,0,0,32,65,108,0,0,0,65,0,0,0,65,108,0,0,32,65,0,0,0,65,108,
			0,0,32,65,0,0,192,64,108,0,0,0,65,0,0,192,64,108,0,0,0,65,0,0,128,64,99,109,98,16,122,65,63,53,94,65,108,10,215,75,65,0,0,48,65,98,18,131,88,65,10,215,29,65,0,0,96,65,217,206,7,65,0,0,96,65,0,0,224,64,98,0,0,96,65,117,147,72,64,35,219,45,65,0,0,0,0,0,
			0,224,64,0,0,0,0,98,117,147,72,64,0,0,0,0,0,0,0,0,117,147,72,64,0,0,0,0,0,0,224,64,98,0,0,0,0,35,219,45,65,117,147,72,64,0,0,96,65,0,0,224,64,0,0,96,65,98,217,206,7,65,0,0,96,65,10,215,29,65,18,131,88,65,0,0,48,65,10,215,75,65,108,64,53,94,65,74,12,122,
			65,98,109,231,101,65,60,223,128,65,54,94,114,65,60,223,128,65,99,16,122,65,74,12,122,65,98,72,225,128,65,29,90,114,65,60,223,128,65,109,231,101,65,99,16,122,65,64,53,94,65,99,109,0,0,224,64,0,0,64,65,98,227,165,135,64,0,0,64,65,0,0,0,64,14,45,28,65,0,
			0,0,64,0,0,224,64,98,0,0,0,64,227,165,135,64,227,165,135,64,0,0,0,64,0,0,224,64,0,0,0,64,98,14,45,28,65,0,0,0,64,0,0,64,65,228,165,135,64,0,0,64,65,0,0,224,64,98,0,0,64,65,14,45,28,65,14,45,28,65,0,0,64,65,0,0,224,64,0,0,64,65,99,101,0,0 };

		path.loadPathFromData(pathData, sizeof(pathData));
	}
	else if (name == "Zoom Out")
	{
		static const unsigned char pathData[] = { 110,109,0,169,123,64,230,209,197,64,108,64,234,30,65,230,209,197,64,108,64,234,30,65,230,209,255,64,108,0,169,123,64,230,209,255,64,99,109,98,16,122,65,63,53,94,65,108,10,215,75,65,0,0,48,65,98,18,131,88,65,10,215,29,65,0,0,96,65,217,206,7,65,0,0,96,
			65,0,0,224,64,98,0,0,96,65,117,147,72,64,35,219,45,65,0,0,0,0,0,0,224,64,0,0,0,0,98,117,147,72,64,0,0,0,0,0,0,0,0,117,147,72,64,0,0,0,0,0,0,224,64,98,0,0,0,0,35,219,45,65,117,147,72,64,0,0,96,65,0,0,224,64,0,0,96,65,98,217,206,7,65,0,0,96,65,10,215,29,
			65,18,131,88,65,0,0,48,65,10,215,75,65,108,64,53,94,65,74,12,122,65,98,109,231,101,65,60,223,128,65,54,94,114,65,60,223,128,65,99,16,122,65,74,12,122,65,98,72,225,128,65,29,90,114,65,60,223,128,65,109,231,101,65,99,16,122,65,64,53,94,65,99,109,0,0,224,
			64,0,0,64,65,98,227,165,135,64,0,0,64,65,0,0,0,64,14,45,28,65,0,0,0,64,0,0,224,64,98,0,0,0,64,227,165,135,64,227,165,135,64,0,0,0,64,0,0,224,64,0,0,0,64,98,14,45,28,65,0,0,0,64,0,0,64,65,228,165,135,64,0,0,64,65,0,0,224,64,98,0,0,64,65,14,45,28,65,14,
			45,28,65,0,0,64,65,0,0,224,64,0,0,64,65,99,101,0,0 };

		path.loadPathFromData(pathData, sizeof(pathData));
	}
	else if (name == "Toggle Vertical Size")
	{
		static const unsigned char pathData[] = { 110, 109, 128, 109, 151, 67, 0, 185, 239, 66, 98, 102, 225, 114, 67, 0, 185, 239, 66, 225, 122, 59, 67, 13, 29, 13, 67, 0, 32, 18, 67, 128, 197, 50, 67, 98, 130, 21, 197, 66, 251, 25, 94, 67, 0, 164, 138, 66, 174, 38, 140, 67, 0, 164, 138, 66, 0, 31, 175, 67, 98, 0, 164, 138, 66, 154, 216, 197, 67, 86, 242, 152, 66, 79, 0,
			216, 67, 0, 196, 196, 66, 192, 29, 229, 67, 108, 0, 194, 196, 66, 64, 30, 229, 67, 98, 255, 65, 228, 66, 177, 139, 238, 67, 77, 121, 7, 67, 33, 160, 245, 67, 128, 140, 32, 67, 64, 197, 250, 67, 98, 149, 250, 33, 67, 6, 129, 2, 68, 240, 210, 40, 67, 56, 149, 9, 68, 128, 53, 57, 67, 128, 6, 13, 68, 98, 199, 182, 82, 67, 29, 99,
			18, 68, 146, 30, 127, 67, 0, 55, 18, 68, 192, 197, 149, 67, 0, 55, 18, 68, 98, 59, 218, 166, 67, 0, 55, 18, 68, 191, 200, 193, 67, 40, 124, 17, 68, 64, 230, 205, 67, 32, 16, 14, 68, 98, 205, 190, 219, 67, 216, 38, 10, 68, 31, 78, 221, 67, 218, 98, 3, 68, 192, 120, 221, 67, 128, 74, 250, 67, 98, 6, 15, 235, 67, 58, 140, 244,
			67, 170, 128, 246, 67, 0, 147, 236, 67, 64, 180, 254, 67, 64, 0, 226, 67, 98, 59, 89, 4, 68, 84, 30, 213, 67, 64, 25, 6, 68, 158, 190, 196, 67, 64, 25, 6, 68, 0, 31, 175, 67, 98, 64, 25, 6, 68, 133, 106, 145, 67, 116, 209, 252, 67, 62, 245, 104, 67, 128, 251, 234, 67, 128, 101, 63, 67, 98, 46, 147, 213, 67, 109, 130, 13, 67,
			149, 155, 186, 67, 0, 185, 239, 66, 128, 109, 151, 67, 0, 185, 239, 66, 99, 109, 128, 232, 150, 67, 128, 18, 25, 67, 98, 251, 220, 181, 67, 128, 18, 25, 67, 128, 245, 206, 67, 11, 67, 75, 67, 128, 245, 206, 67, 0, 150, 132, 67, 98, 128, 245, 206, 67, 123, 138, 163, 67, 251, 220, 181, 67, 192, 162, 188, 67, 128, 232, 150,
			67, 192, 162, 188, 67, 98, 10, 232, 111, 67, 192, 162, 188, 67, 128, 183, 61, 67, 123, 138, 163, 67, 128, 183, 61, 67, 0, 150, 132, 67, 98, 128, 183, 61, 67, 11, 67, 75, 67, 10, 232, 111, 67, 128, 18, 25, 67, 128, 232, 150, 67, 128, 18, 25, 67, 99, 109, 0, 117, 8, 67, 0, 239, 97, 67, 98, 187, 75, 9, 67, 187, 232, 97, 67, 31,
			25, 10, 67, 174, 255, 97, 67, 0, 220, 10, 67, 0, 52, 98, 67, 98, 109, 10, 23, 67, 215, 119, 101, 67, 255, 101, 22, 67, 10, 95, 134, 67, 128, 59, 10, 67, 192, 18, 157, 67, 98, 83, 34, 252, 66, 250, 197, 179, 67, 248, 248, 213, 66, 10, 241, 196, 67, 0, 156, 189, 66, 192, 79, 195, 67, 108, 0, 155, 189, 66, 192, 79, 195, 67, 98,
			213, 62, 165, 66, 77, 173, 193, 67, 79, 41, 159, 66, 14, 12, 170, 67, 0, 126, 183, 66, 128, 88, 147, 67, 98, 174, 77, 206, 66, 205, 31, 124, 67, 21, 192, 247, 66, 9, 77, 98, 67, 0, 117, 8, 67, 0, 239, 97, 67, 99, 109, 192, 157, 236, 67, 128, 65, 102, 67, 98, 253, 239, 242, 67, 188, 20, 102, 67, 18, 197, 254, 67, 238, 164,
			134, 67, 224, 241, 1, 68, 192, 212, 156, 67, 98, 156, 150, 4, 68, 22, 187, 179, 67, 207, 167, 2, 68, 224, 181, 196, 67, 192, 42, 255, 67, 0, 33, 198, 67, 98, 227, 5, 249, 67, 133, 140, 199, 67, 57, 22, 240, 67, 135, 14, 182, 67, 192, 204, 234, 67, 0, 40, 159, 67, 98, 71, 131, 229, 67, 88, 65, 136, 67, 163, 224, 229, 67, 191,
			59, 105, 67, 128, 5, 236, 67, 128, 101, 102, 67, 108, 192, 5, 236, 67, 128, 101, 102, 67, 98, 233, 54, 236, 67, 199, 78, 102, 67, 140, 105, 236, 67, 242, 66, 102, 67, 192, 157, 236, 67, 128, 65, 102, 67, 99, 109, 192, 109, 151, 67, 0, 140, 213, 67, 98, 208, 229, 169, 67, 0, 140, 213, 67, 184, 205, 178, 67, 154, 98, 214,
			67, 192, 89, 190, 67, 64, 198, 219, 67, 98, 255, 190, 197, 67, 230, 57, 223, 67, 235, 60, 207, 67, 255, 22, 227, 67, 192, 37, 212, 67, 128, 68, 233, 67, 98, 51, 113, 214, 67, 179, 39, 236, 67, 0, 67, 215, 67, 254, 195, 240, 67, 0, 67, 215, 67, 64, 220, 243, 67, 98, 0, 67, 215, 67, 254, 75, 251, 67, 62, 217, 214, 67, 35, 190,
			5, 68, 0, 255, 207, 67, 0, 159, 8, 68, 98, 92, 94, 196, 67, 58, 129, 13, 68, 48, 96, 171, 67, 96, 139, 13, 68, 128, 213, 148, 67, 96, 139, 13, 68, 98, 77, 130, 128, 67, 96, 139, 13, 68, 240, 35, 86, 67, 56, 231, 12, 68, 0, 204, 63, 67, 160, 60, 9, 68, 108, 128, 203, 63, 67, 128, 60, 9, 68, 98, 99, 193, 46, 67, 169, 112, 6, 68,
			0, 246, 40, 67, 95, 137, 252, 67, 0, 246, 40, 67, 64, 220, 243, 67, 98, 0, 246, 40, 67, 80, 252, 240, 67, 225, 89, 38, 67, 183, 169, 237, 67, 0, 246, 40, 67, 0, 88, 235, 67, 98, 197, 230, 47, 67, 31, 45, 229, 67, 129, 109, 71, 67, 19, 116, 223, 67, 0, 200, 90, 67, 64, 198, 219, 67, 98, 143, 234, 114, 67, 211, 47, 215, 67, 110,
			101, 134, 67, 0, 140, 213, 67, 192, 109, 151, 67, 0, 140, 213, 67, 99, 109, 128, 255, 123, 67, 128, 227, 222, 67, 98, 122, 30, 117, 67, 128, 227, 222, 67, 128, 138, 111, 67, 245, 91, 226, 67, 128, 138, 111, 67, 192, 165, 243, 67, 98, 128, 138, 111, 67, 189, 119, 2, 68, 122, 30, 117, 67, 0, 52, 4, 68, 128, 255, 123, 67,
			0, 52, 4, 68, 98, 67, 112, 129, 67, 0, 52, 4, 68, 0, 58, 132, 67, 197, 119, 2, 68, 0, 58, 132, 67, 192, 165, 243, 67, 98, 0, 58, 132, 67, 6, 92, 226, 67, 67, 112, 129, 67, 128, 227, 222, 67, 128, 255, 123, 67, 128, 227, 222, 67, 99, 109, 128, 209, 175, 67, 128, 227, 222, 67, 98, 253, 96, 172, 67, 128, 227, 222, 67, 64, 151,
			169, 67, 245, 91, 226, 67, 64, 151, 169, 67, 192, 165, 243, 67, 98, 64, 151, 169, 67, 189, 119, 2, 68, 253, 96, 172, 67, 0, 52, 4, 68, 128, 209, 175, 67, 0, 52, 4, 68, 98, 3, 66, 179, 67, 0, 52, 4, 68, 192, 11, 182, 67, 197, 119, 2, 68, 192, 11, 182, 67, 192, 165, 243, 67, 98, 192, 11, 182, 67, 6, 92, 226, 67, 3, 66, 179, 67,
			128, 227, 222, 67, 128, 209, 175, 67, 128, 227, 222, 67, 99, 101, 0, 0 };

		path.loadPathFromData(pathData, sizeof(pathData));
	}
	else if (name == "Refresh Velocity Crossfades.")
	{
		static const unsigned char pathData[] = { 110, 109, 0, 0, 220, 66, 64, 174, 223, 67, 108, 0, 0, 47, 67, 32, 151, 2, 68, 108, 0, 0, 47, 67, 64, 174, 223, 67, 108, 0, 0, 220, 66, 64, 174, 223, 67, 99, 109, 0, 0, 220, 66, 64, 174, 228, 67, 108, 0, 0, 220, 66, 32, 23, 5, 68, 108, 0, 0, 47, 67, 32, 23, 5, 68, 108, 0, 0, 220, 66, 64, 174, 228, 67, 99, 101, 0, 0 };

		path.loadPathFromData(pathData, sizeof(pathData));

	}
	else if (name == "Select all Samples")
	{
		static const unsigned char pathData[] = { 110,109,0,128,147,67,128,161,185,67,108,0,128,147,67,128,161,185,67,108,246,120,147,67,173,161,185,67,108,241,113,147,67,53,162,185,67,108,245,106,147,67,23,163,185,67,108,6,100,147,67,81,164,185,67,108,42,93,147,67,228,165,185,67,108,99,86,147,67,206,
			167,185,67,108,184,79,147,67,15,170,185,67,108,43,73,147,67,164,172,185,67,108,193,66,147,67,139,175,185,67,108,127,60,147,67,196,178,185,67,108,104,54,147,67,76,182,185,67,108,128,48,147,67,33,186,185,67,108,203,42,147,67,64,190,185,67,108,76,37,147,
			67,167,194,185,67,108,8,32,147,67,83,199,185,67,108,1,27,147,67,65,204,185,67,108,59,22,147,67,109,209,185,67,108,184,17,147,67,213,214,185,67,108,124,13,147,67,117,220,185,67,108,137,9,147,67,73,226,185,67,108,227,5,147,67,78,232,185,67,108,138,2,147,
			67,127,238,185,67,108,130,255,146,67,218,244,185,67,108,204,252,146,67,89,251,185,67,108,105,250,146,67,249,1,186,67,108,93,248,146,67,181,8,186,67,108,167,246,146,67,137,15,186,67,108,73,245,146,67,113,22,186,67,108,68,244,146,67,104,29,186,67,108,153,
			243,146,67,106,36,186,67,108,71,243,146,67,115,43,186,67,108,64,243,146,67,64,46,186,67,108,64,243,146,67,64,46,186,67,108,64,243,146,67,64,46,191,67,108,192,12,148,67,64,46,191,67,108,192,12,148,67,64,187,186,67,108,0,128,152,67,64,187,186,67,108,0,
			128,152,67,128,161,185,67,108,0,128,147,67,128,161,185,67,99,109,0,128,157,67,128,161,185,67,108,0,128,157,67,64,187,186,67,108,64,243,161,67,64,187,186,67,108,64,243,161,67,64,46,191,67,108,192,12,163,67,64,46,191,67,108,192,12,163,67,64,46,186,67,108,
			192,12,163,67,64,46,186,67,108,147,12,163,67,54,39,186,67,108,11,12,163,67,49,32,186,67,108,41,11,163,67,53,25,186,67,108,239,9,163,67,70,18,186,67,108,92,8,163,67,106,11,186,67,108,114,6,163,67,163,4,186,67,108,49,4,163,67,248,253,185,67,108,156,1,163,
			67,107,247,185,67,108,181,254,162,67,1,241,185,67,108,124,251,162,67,191,234,185,67,108,244,247,162,67,168,228,185,67,108,31,244,162,67,192,222,185,67,108,0,240,162,67,11,217,185,67,108,153,235,162,67,140,211,185,67,108,237,230,162,67,72,206,185,67,108,
			255,225,162,67,65,201,185,67,108,211,220,162,67,123,196,185,67,108,107,215,162,67,248,191,185,67,108,203,209,162,67,188,187,185,67,108,247,203,162,67,201,183,185,67,108,242,197,162,67,35,180,185,67,108,193,191,162,67,202,176,185,67,108,102,185,162,67,
			194,173,185,67,108,231,178,162,67,12,171,185,67,108,71,172,162,67,169,168,185,67,108,139,165,162,67,157,166,185,67,108,183,158,162,67,231,164,185,67,108,207,151,162,67,137,163,185,67,108,216,144,162,67,132,162,185,67,108,214,137,162,67,217,161,185,67,
			108,205,130,162,67,135,161,185,67,108,0,128,162,67,128,161,185,67,108,0,128,162,67,128,161,185,67,108,0,128,157,67,128,161,185,67,99,109,64,7,156,67,0,167,188,67,108,64,7,156,67,0,167,188,67,108,229,0,156,67,39,167,188,67,108,143,250,155,67,159,167,188,
			67,108,65,244,155,67,104,168,188,67,108,254,237,155,67,129,169,188,67,108,204,231,155,67,235,170,188,67,108,174,225,155,67,163,172,188,67,108,168,219,155,67,168,174,188,67,108,189,213,155,67,251,176,188,67,108,242,207,155,67,152,179,188,67,108,75,202,
			155,67,126,182,188,67,108,202,196,155,67,172,185,188,67,108,116,191,155,67,31,189,188,67,108,76,186,155,67,214,192,188,67,108,84,181,155,67,205,196,188,67,108,146,176,155,67,2,201,188,67,108,6,172,155,67,115,205,188,67,108,181,167,155,67,29,210,188,67,
			108,161,163,155,67,253,214,188,67,108,204,159,155,67,15,220,188,67,108,58,156,155,67,80,225,188,67,108,236,152,155,67,190,230,188,67,108,228,149,155,67,84,236,188,67,108,37,147,155,67,15,242,188,67,108,176,144,155,67,235,247,188,67,108,135,142,155,67,
			229,253,188,67,108,171,140,155,67,249,3,189,67,108,30,139,155,67,35,10,189,67,108,223,137,155,67,94,16,189,67,108,241,136,155,67,167,22,189,67,108,84,136,155,67,250,28,189,67,108,8,136,155,67,84,35,189,67,108,0,136,155,67,0,38,189,67,108,0,136,155,67,
			0,38,189,67,108,0,136,155,67,192,35,193,67,108,0,136,155,67,192,35,193,67,108,42,136,155,67,26,42,193,67,108,165,136,155,67,113,48,193,67,108,113,137,155,67,191,54,193,67,108,142,138,155,67,0,61,193,67,108,250,139,155,67,50,67,193,67,108,182,141,155,
			67,79,73,193,67,108,190,143,155,67,84,79,193,67,108,19,146,155,67,62,85,193,67,108,180,148,155,67,7,91,193,67,108,157,151,155,67,173,96,193,67,108,205,154,155,67,44,102,193,67,108,67,158,155,67,129,107,193,67,108,252,161,155,67,167,112,193,67,108,246,
			165,155,67,157,117,193,67,108,46,170,155,67,93,122,193,67,108,161,174,155,67,231,126,193,67,108,77,179,155,67,53,131,193,67,108,47,184,155,67,71,135,193,67,108,67,189,155,67,25,139,193,67,108,134,194,155,67,169,142,193,67,108,245,199,155,67,244,145,193,
			67,108,141,205,155,67,249,148,193,67,108,74,211,155,67,181,151,193,67,108,39,217,155,67,39,154,193,67,108,34,223,155,67,77,156,193,67,108,55,229,155,67,38,158,193,67,108,97,235,155,67,176,159,193,67,108,157,241,155,67,236,160,193,67,108,231,247,155,67,
			214,161,193,67,108,58,254,155,67,113,162,193,67,108,148,4,156,67,186,162,193,67,108,64,7,156,67,192,162,193,67,108,64,7,156,67,192,162,193,67,108,64,118,160,67,192,162,193,67,108,64,118,160,67,192,162,193,67,108,154,124,160,67,150,162,193,67,108,241,
			130,160,67,27,162,193,67,108,63,137,160,67,79,161,193,67,108,128,143,160,67,50,160,193,67,108,178,149,160,67,198,158,193,67,108,207,155,160,67,10,157,193,67,108,212,161,160,67,2,155,193,67,108,190,167,160,67,173,152,193,67,108,135,173,160,67,12,150,193,
			67,108,45,179,160,67,35,147,193,67,108,172,184,160,67,243,143,193,67,108,1,190,160,67,125,140,193,67,108,39,195,160,67,196,136,193,67,108,29,200,160,67,202,132,193,67,108,221,204,160,67,146,128,193,67,108,103,209,160,67,31,124,193,67,108,181,213,160,
			67,115,119,193,67,108,199,217,160,67,145,114,193,67,108,153,221,160,67,125,109,193,67,108,41,225,160,67,58,104,193,67,108,116,228,160,67,203,98,193,67,108,121,231,160,67,51,93,193,67,108,53,234,160,67,119,87,193,67,108,167,236,160,67,153,81,193,67,108,
			205,238,160,67,158,75,193,67,108,166,240,160,67,137,69,193,67,108,48,242,160,67,95,63,193,67,108,108,243,160,67,35,57,193,67,108,86,244,160,67,217,50,193,67,108,241,244,160,67,134,44,193,67,108,58,245,160,67,45,38,193,67,108,64,245,160,67,192,35,193,
			67,108,64,245,160,67,192,35,193,67,108,64,245,160,67,0,38,189,67,108,64,245,160,67,0,38,189,67,108,22,245,160,67,166,31,189,67,108,155,244,160,67,79,25,189,67,108,207,243,160,67,1,19,189,67,108,178,242,160,67,192,12,189,67,108,70,241,160,67,142,6,189,
			67,108,138,239,160,67,113,0,189,67,108,130,237,160,67,108,250,188,67,108,45,235,160,67,130,244,188,67,108,140,232,160,67,185,238,188,67,108,163,229,160,67,19,233,188,67,108,115,226,160,67,148,227,188,67,108,253,222,160,67,63,222,188,67,108,68,219,160,
			67,25,217,188,67,108,74,215,160,67,35,212,188,67,108,18,211,160,67,99,207,188,67,108,159,206,160,67,217,202,188,67,108,243,201,160,67,139,198,188,67,108,17,197,160,67,121,194,188,67,108,253,191,160,67,167,190,188,67,108,186,186,160,67,23,187,188,67,108,
			75,181,160,67,204,183,188,67,108,179,175,160,67,199,180,188,67,108,247,169,160,67,11,178,188,67,108,25,164,160,67,153,175,188,67,108,30,158,160,67,115,173,188,67,108,9,152,160,67,154,171,188,67,108,223,145,160,67,16,170,188,67,108,163,139,160,67,212,
			168,188,67,108,89,133,160,67,234,167,188,67,108,6,127,160,67,79,167,188,67,108,172,120,160,67,6,167,188,67,108,64,118,160,67,0,167,188,67,108,64,118,160,67,0,167,188,67,108,64,7,156,67,0,167,188,67,99,109,192,162,149,67,192,168,188,67,108,192,162,149,
			67,192,168,188,67,108,102,156,149,67,234,168,188,67,108,15,150,149,67,101,169,188,67,108,193,143,149,67,49,170,188,67,108,128,137,149,67,78,171,188,67,108,78,131,149,67,186,172,188,67,108,49,125,149,67,118,174,188,67,108,44,119,149,67,126,176,188,67,
			108,66,113,149,67,211,178,188,67,108,121,107,149,67,116,181,188,67,108,211,101,149,67,93,184,188,67,108,84,96,149,67,141,187,188,67,108,255,90,149,67,3,191,188,67,108,217,85,149,67,188,194,188,67,108,227,80,149,67,182,198,188,67,108,35,76,149,67,238,
			202,188,67,108,153,71,149,67,97,207,188,67,108,75,67,149,67,13,212,188,67,108,57,63,149,67,239,216,188,67,108,103,59,149,67,3,222,188,67,108,215,55,149,67,70,227,188,67,108,140,52,149,67,181,232,188,67,108,135,49,149,67,77,238,188,67,108,203,46,149,67,
			9,244,188,67,108,89,44,149,67,231,249,188,67,108,51,42,149,67,226,255,188,67,108,90,40,149,67,247,5,189,67,108,208,38,149,67,33,12,189,67,108,148,37,149,67,93,18,189,67,108,170,36,149,67,167,24,189,67,108,15,36,149,67,250,30,189,67,108,198,35,149,67,
			83,37,189,67,108,192,35,149,67,0,40,189,67,108,192,35,149,67,0,40,189,67,108,192,35,149,67,128,37,193,67,108,192,35,149,67,128,37,193,67,108,234,35,149,67,218,43,193,67,108,101,36,149,67,49,50,193,67,108,49,37,149,67,127,56,193,67,108,78,38,149,67,192,
			62,193,67,108,186,39,149,67,242,68,193,67,108,118,41,149,67,15,75,193,67,108,126,43,149,67,20,81,193,67,108,212,45,149,67,254,86,193,67,108,116,48,149,67,199,92,193,67,108,93,51,149,67,109,98,193,67,108,141,54,149,67,236,103,193,67,108,3,58,149,67,65,
			109,193,67,108,188,61,149,67,103,114,193,67,108,182,65,149,67,93,119,193,67,108,238,69,149,67,29,124,193,67,108,97,74,149,67,167,128,193,67,108,13,79,149,67,245,132,193,67,108,239,83,149,67,7,137,193,67,108,3,89,149,67,217,140,193,67,108,70,94,149,67,
			105,144,193,67,108,181,99,149,67,180,147,193,67,108,77,105,149,67,185,150,193,67,108,10,111,149,67,117,153,193,67,108,231,116,149,67,231,155,193,67,108,226,122,149,67,13,158,193,67,108,247,128,149,67,230,159,193,67,108,33,135,149,67,112,161,193,67,108,
			93,141,149,67,172,162,193,67,108,167,147,149,67,150,163,193,67,108,250,153,149,67,49,164,193,67,108,84,160,149,67,122,164,193,67,108,192,162,149,67,128,164,193,67,108,192,162,149,67,128,164,193,67,108,0,18,154,67,128,164,193,67,108,0,18,154,67,128,164,
			193,67,108,90,24,154,67,86,164,193,67,108,177,30,154,67,219,163,193,67,108,255,36,154,67,15,163,193,67,108,64,43,154,67,242,161,193,67,108,114,49,154,67,134,160,193,67,108,143,55,154,67,202,158,193,67,108,148,61,154,67,194,156,193,67,108,126,67,154,67,
			109,154,193,67,108,71,73,154,67,204,151,193,67,108,237,78,154,67,227,148,193,67,108,108,84,154,67,179,145,193,67,108,193,89,154,67,61,142,193,67,108,231,94,154,67,132,138,193,67,108,221,99,154,67,138,134,193,67,108,157,104,154,67,82,130,193,67,108,39,
			109,154,67,223,125,193,67,108,117,113,154,67,51,121,193,67,108,135,117,154,67,81,116,193,67,108,89,121,154,67,61,111,193,67,108,233,124,154,67,250,105,193,67,108,52,128,154,67,139,100,193,67,108,57,131,154,67,243,94,193,67,108,245,133,154,67,55,89,193,
			67,108,103,136,154,67,89,83,193,67,108,141,138,154,67,94,77,193,67,108,102,140,154,67,73,71,193,67,108,240,141,154,67,31,65,193,67,108,44,143,154,67,227,58,193,67,108,22,144,154,67,153,52,193,67,108,177,144,154,67,70,46,193,67,108,250,144,154,67,237,
			39,193,67,108,0,145,154,67,128,37,193,67,108,0,145,154,67,128,37,193,67,108,0,145,154,67,0,40,189,67,108,0,145,154,67,0,40,189,67,108,217,144,154,67,165,33,189,67,108,97,144,154,67,79,27,189,67,108,152,143,154,67,1,21,189,67,108,127,142,154,67,190,14,
			189,67,108,21,141,154,67,140,8,189,67,108,93,139,154,67,110,2,189,67,108,88,137,154,67,104,252,188,67,108,5,135,154,67,125,246,188,67,108,104,132,154,67,178,240,188,67,108,130,129,154,67,11,235,188,67,108,84,126,154,67,138,229,188,67,108,225,122,154,
			67,52,224,188,67,108,42,119,154,67,12,219,188,67,108,51,115,154,67,20,214,188,67,108,254,110,154,67,82,209,188,67,108,141,106,154,67,198,204,188,67,108,227,101,154,67,117,200,188,67,108,3,97,154,67,97,196,188,67,108,241,91,154,67,140,192,188,67,108,176,
			86,154,67,250,188,188,67,108,66,81,154,67,172,185,188,67,108,172,75,154,67,164,182,188,67,108,241,69,154,67,229,179,188,67,108,21,64,154,67,112,177,188,67,108,27,58,154,67,71,175,188,67,108,7,52,154,67,107,173,188,67,108,221,45,154,67,222,171,188,67,
			108,162,39,154,67,159,170,188,67,108,89,33,154,67,177,169,188,67,108,6,27,154,67,20,169,188,67,108,172,20,154,67,200,168,188,67,108,0,18,154,67,192,168,188,67,108,0,18,154,67,192,168,188,67,108,192,162,149,67,192,168,188,67,99,109,64,134,156,67,64,165,
			189,67,108,0,247,159,67,64,165,189,67,108,0,247,159,67,128,164,192,67,108,64,134,156,67,128,164,192,67,108,64,134,156,67,64,165,189,67,99,109,0,34,150,67,0,167,189,67,108,192,146,153,67,0,167,189,67,108,192,146,153,67,64,166,192,67,108,0,34,150,67,64,
			166,192,67,108,0,34,150,67,0,167,189,67,99,109,0,1,156,67,128,98,194,67,108,0,1,156,67,128,98,194,67,108,166,250,155,67,170,98,194,67,108,79,244,155,67,37,99,194,67,108,1,238,155,67,241,99,194,67,108,192,231,155,67,14,101,194,67,108,142,225,155,67,122,
			102,194,67,108,113,219,155,67,54,104,194,67,108,108,213,155,67,62,106,194,67,108,130,207,155,67,147,108,194,67,108,185,201,155,67,52,111,194,67,108,19,196,155,67,29,114,194,67,108,148,190,155,67,77,117,194,67,108,63,185,155,67,195,120,194,67,108,25,180,
			155,67,124,124,194,67,108,35,175,155,67,118,128,194,67,108,99,170,155,67,174,132,194,67,108,217,165,155,67,33,137,194,67,108,139,161,155,67,205,141,194,67,108,121,157,155,67,175,146,194,67,108,167,153,155,67,195,151,194,67,108,23,150,155,67,6,157,194,
			67,108,204,146,155,67,117,162,194,67,108,199,143,155,67,13,168,194,67,108,11,141,155,67,201,173,194,67,108,153,138,155,67,167,179,194,67,108,115,136,155,67,162,185,194,67,108,154,134,155,67,183,191,194,67,108,16,133,155,67,225,197,194,67,108,212,131,
			155,67,29,204,194,67,108,234,130,155,67,103,210,194,67,108,79,130,155,67,186,216,194,67,108,6,130,155,67,19,223,194,67,108,0,130,155,67,192,225,194,67,108,0,130,155,67,192,225,194,67,108,0,130,155,67,64,223,198,67,108,0,130,155,67,64,223,198,67,108,42,
			130,155,67,154,229,198,67,108,165,130,155,67,241,235,198,67,108,113,131,155,67,63,242,198,67,108,142,132,155,67,128,248,198,67,108,250,133,155,67,178,254,198,67,108,182,135,155,67,207,4,199,67,108,190,137,155,67,212,10,199,67,108,20,140,155,67,190,16,
			199,67,108,180,142,155,67,135,22,199,67,108,157,145,155,67,45,28,199,67,108,205,148,155,67,172,33,199,67,108,67,152,155,67,1,39,199,67,108,252,155,155,67,39,44,199,67,108,246,159,155,67,29,49,199,67,108,46,164,155,67,221,53,199,67,108,161,168,155,67,
			103,58,199,67,108,77,173,155,67,181,62,199,67,108,47,178,155,67,199,66,199,67,108,67,183,155,67,153,70,199,67,108,134,188,155,67,41,74,199,67,108,245,193,155,67,116,77,199,67,108,141,199,155,67,121,80,199,67,108,74,205,155,67,53,83,199,67,108,39,211,
			155,67,167,85,199,67,108,34,217,155,67,205,87,199,67,108,55,223,155,67,166,89,199,67,108,97,229,155,67,48,91,199,67,108,157,235,155,67,108,92,199,67,108,231,241,155,67,86,93,199,67,108,58,248,155,67,241,93,199,67,108,148,254,155,67,58,94,199,67,108,0,
			1,156,67,64,94,199,67,108,0,1,156,67,64,94,199,67,108,0,112,160,67,64,94,199,67,108,0,112,160,67,64,94,199,67,108,91,118,160,67,25,94,199,67,108,177,124,160,67,161,93,199,67,108,255,130,160,67,216,92,199,67,108,66,137,160,67,191,91,199,67,108,116,143,
			160,67,85,90,199,67,108,146,149,160,67,157,88,199,67,108,152,155,160,67,152,86,199,67,108,131,161,160,67,69,84,199,67,108,78,167,160,67,168,81,199,67,108,245,172,160,67,194,78,199,67,108,118,178,160,67,148,75,199,67,108,204,183,160,67,33,72,199,67,108,
			244,188,160,67,106,68,199,67,108,236,193,160,67,115,64,199,67,108,174,198,160,67,62,60,199,67,108,58,203,160,67,205,55,199,67,108,139,207,160,67,35,51,199,67,108,159,211,160,67,67,46,199,67,108,116,215,160,67,49,41,199,67,108,6,219,160,67,240,35,199,
			67,108,84,222,160,67,130,30,199,67,108,92,225,160,67,236,24,199,67,108,27,228,160,67,49,19,199,67,108,144,230,160,67,85,13,199,67,108,185,232,160,67,91,7,199,67,108,149,234,160,67,71,1,199,67,108,34,236,160,67,30,251,198,67,108,97,237,160,67,226,244,
			198,67,108,79,238,160,67,153,238,198,67,108,236,238,160,67,70,232,198,67,108,56,239,160,67,237,225,198,67,108,64,239,160,67,64,223,198,67,108,64,239,160,67,64,223,198,67,108,64,239,160,67,192,225,194,67,108,64,239,160,67,192,225,194,67,108,25,239,160,
			67,101,219,194,67,108,161,238,160,67,15,213,194,67,108,216,237,160,67,193,206,194,67,108,191,236,160,67,126,200,194,67,108,85,235,160,67,76,194,194,67,108,157,233,160,67,46,188,194,67,108,152,231,160,67,40,182,194,67,108,69,229,160,67,61,176,194,67,108,
			168,226,160,67,114,170,194,67,108,194,223,160,67,203,164,194,67,108,148,220,160,67,74,159,194,67,108,33,217,160,67,244,153,194,67,108,106,213,160,67,204,148,194,67,108,115,209,160,67,212,143,194,67,108,62,205,160,67,18,139,194,67,108,205,200,160,67,134,
			134,194,67,108,35,196,160,67,53,130,194,67,108,67,191,160,67,33,126,194,67,108,49,186,160,67,76,122,194,67,108,240,180,160,67,186,118,194,67,108,130,175,160,67,108,115,194,67,108,236,169,160,67,100,112,194,67,108,49,164,160,67,165,109,194,67,108,85,158,
			160,67,48,107,194,67,108,91,152,160,67,7,105,194,67,108,71,146,160,67,43,103,194,67,108,29,140,160,67,158,101,194,67,108,226,133,160,67,95,100,194,67,108,153,127,160,67,113,99,194,67,108,70,121,160,67,212,98,194,67,108,236,114,160,67,136,98,194,67,108,
			0,112,160,67,128,98,194,67,108,0,112,160,67,128,98,194,67,108,0,1,156,67,128,98,194,67,99,109,192,156,149,67,128,100,194,67,108,192,156,149,67,128,100,194,67,108,102,150,149,67,170,100,194,67,108,15,144,149,67,37,101,194,67,108,193,137,149,67,241,101,
			194,67,108,128,131,149,67,14,103,194,67,108,78,125,149,67,122,104,194,67,108,49,119,149,67,54,106,194,67,108,44,113,149,67,62,108,194,67,108,66,107,149,67,147,110,194,67,108,121,101,149,67,52,113,194,67,108,211,95,149,67,29,116,194,67,108,84,90,149,67,
			77,119,194,67,108,255,84,149,67,195,122,194,67,108,217,79,149,67,124,126,194,67,108,227,74,149,67,118,130,194,67,108,35,70,149,67,174,134,194,67,108,153,65,149,67,33,139,194,67,108,75,61,149,67,205,143,194,67,108,57,57,149,67,175,148,194,67,108,103,53,
			149,67,195,153,194,67,108,215,49,149,67,6,159,194,67,108,140,46,149,67,117,164,194,67,108,135,43,149,67,13,170,194,67,108,203,40,149,67,201,175,194,67,108,89,38,149,67,167,181,194,67,108,51,36,149,67,162,187,194,67,108,90,34,149,67,183,193,194,67,108,
			208,32,149,67,225,199,194,67,108,148,31,149,67,29,206,194,67,108,170,30,149,67,103,212,194,67,108,15,30,149,67,186,218,194,67,108,198,29,149,67,20,225,194,67,108,192,29,149,67,128,227,194,67,108,192,29,149,67,128,227,194,67,108,192,29,149,67,0,225,198,
			67,108,192,29,149,67,0,225,198,67,108,231,29,149,67,91,231,198,67,108,95,30,149,67,177,237,198,67,108,40,31,149,67,255,243,198,67,108,65,32,149,67,66,250,198,67,108,171,33,149,67,116,0,199,67,108,99,35,149,67,146,6,199,67,108,104,37,149,67,152,12,199,
			67,108,187,39,149,67,131,18,199,67,108,88,42,149,67,78,24,199,67,108,62,45,149,67,245,29,199,67,108,108,48,149,67,118,35,199,67,108,223,51,149,67,204,40,199,67,108,150,55,149,67,244,45,199,67,108,141,59,149,67,236,50,199,67,108,194,63,149,67,174,55,199,
			67,108,51,68,149,67,58,60,199,67,108,221,72,149,67,139,64,199,67,108,189,77,149,67,159,68,199,67,108,207,82,149,67,116,72,199,67,108,16,88,149,67,6,76,199,67,108,126,93,149,67,84,79,199,67,108,20,99,149,67,92,82,199,67,108,207,104,149,67,27,85,199,67,
			108,171,110,149,67,144,87,199,67,108,165,116,149,67,185,89,199,67,108,185,122,149,67,149,91,199,67,108,226,128,149,67,34,93,199,67,108,30,135,149,67,97,94,199,67,108,103,141,149,67,79,95,199,67,108,186,147,149,67,236,95,199,67,108,19,154,149,67,56,96,
			199,67,108,192,156,149,67,64,96,199,67,108,192,156,149,67,64,96,199,67,108,192,11,154,67,64,96,199,67,108,192,11,154,67,64,96,199,67,108,27,18,154,67,25,96,199,67,108,113,24,154,67,161,95,199,67,108,191,30,154,67,216,94,199,67,108,2,37,154,67,191,93,
			199,67,108,52,43,154,67,85,92,199,67,108,82,49,154,67,157,90,199,67,108,88,55,154,67,152,88,199,67,108,67,61,154,67,69,86,199,67,108,14,67,154,67,168,83,199,67,108,181,72,154,67,194,80,199,67,108,54,78,154,67,148,77,199,67,108,140,83,154,67,33,74,199,
			67,108,180,88,154,67,106,70,199,67,108,172,93,154,67,115,66,199,67,108,110,98,154,67,62,62,199,67,108,250,102,154,67,205,57,199,67,108,75,107,154,67,35,53,199,67,108,95,111,154,67,67,48,199,67,108,52,115,154,67,49,43,199,67,108,198,118,154,67,240,37,
			199,67,108,20,122,154,67,130,32,199,67,108,28,125,154,67,236,26,199,67,108,219,127,154,67,49,21,199,67,108,80,130,154,67,85,15,199,67,108,121,132,154,67,91,9,199,67,108,85,134,154,67,71,3,199,67,108,226,135,154,67,30,253,198,67,108,33,137,154,67,226,
			246,198,67,108,15,138,154,67,153,240,198,67,108,172,138,154,67,70,234,198,67,108,248,138,154,67,237,227,198,67,108,0,139,154,67,0,225,198,67,108,0,139,154,67,0,225,198,67,108,0,139,154,67,128,227,194,67,108,0,139,154,67,128,227,194,67,108,214,138,154,
			67,38,221,194,67,108,91,138,154,67,207,214,194,67,108,143,137,154,67,129,208,194,67,108,114,136,154,67,64,202,194,67,108,6,135,154,67,14,196,194,67,108,75,133,154,67,241,189,194,67,108,66,131,154,67,236,183,194,67,108,237,128,154,67,2,178,194,67,108,
			76,126,154,67,57,172,194,67,108,99,123,154,67,147,166,194,67,108,51,120,154,67,20,161,194,67,108,189,116,154,67,191,155,194,67,108,4,113,154,67,153,150,194,67,108,10,109,154,67,163,145,194,67,108,210,104,154,67,227,140,194,67,108,95,100,154,67,89,136,
			194,67,108,179,95,154,67,11,132,194,67,108,209,90,154,67,249,127,194,67,108,189,85,154,67,39,124,194,67,108,122,80,154,67,151,120,194,67,108,11,75,154,67,76,117,194,67,108,115,69,154,67,71,114,194,67,108,183,63,154,67,139,111,194,67,108,217,57,154,67,
			25,109,194,67,108,222,51,154,67,243,106,194,67,108,201,45,154,67,26,105,194,67,108,159,39,154,67,144,103,194,67,108,99,33,154,67,84,102,194,67,108,25,27,154,67,106,101,194,67,108,198,20,154,67,207,100,194,67,108,109,14,154,67,134,100,194,67,108,192,11,
			154,67,128,100,194,67,108,192,11,154,67,128,100,194,67,108,192,156,149,67,128,100,194,67,99,109,64,128,156,67,192,96,195,67,108,0,241,159,67,192,96,195,67,108,0,241,159,67,64,96,198,67,108,64,128,156,67,64,96,198,67,108,64,128,156,67,192,96,195,67,99,
			109,0,28,150,67,128,98,195,67,108,192,140,153,67,128,98,195,67,108,192,140,153,67,0,98,198,67,108,0,28,150,67,0,98,198,67,108,0,28,150,67,128,98,195,67,99,109,64,243,146,67,64,46,196,67,108,64,243,146,67,64,46,201,67,108,64,243,146,67,64,46,201,67,108,
			106,243,146,67,74,53,201,67,108,239,243,146,67,79,60,201,67,108,205,244,146,67,76,67,201,67,108,5,246,146,67,59,74,201,67,108,149,247,146,67,24,81,201,67,108,124,249,146,67,224,87,201,67,108,185,251,146,67,140,94,201,67,108,75,254,146,67,26,101,201,67,
			108,48,1,147,67,133,107,201,67,108,102,4,147,67,201,113,201,67,108,235,7,147,67,225,119,201,67,108,189,11,147,67,203,125,201,67,108,218,15,147,67,130,131,201,67,108,62,20,147,67,3,137,201,67,108,231,24,147,67,73,142,201,67,108,211,29,147,67,82,147,201,
			67,108,253,34,147,67,27,152,201,67,108,99,40,147,67,160,156,201,67,108,1,46,147,67,223,160,201,67,108,211,51,147,67,212,164,201,67,108,214,57,147,67,126,168,201,67,108,6,64,147,67,217,171,201,67,108,95,70,147,67,228,174,201,67,108,221,76,147,67,157,177,
			201,67,108,124,83,147,67,2,180,201,67,108,55,90,147,67,18,182,201,67,108,11,97,147,67,203,183,201,67,108,242,103,147,67,44,185,201,67,108,233,110,147,67,52,186,201,67,108,235,117,147,67,227,186,201,67,108,243,124,147,67,56,187,201,67,108,0,128,147,67,
			64,187,201,67,108,0,128,147,67,64,187,201,67,108,0,128,152,67,64,187,201,67,108,0,128,152,67,128,161,200,67,108,192,12,148,67,128,161,200,67,108,192,12,148,67,64,46,196,67,108,64,243,146,67,64,46,196,67,99,109,64,243,161,67,64,46,196,67,108,64,243,161,
			67,128,161,200,67,108,0,128,157,67,128,161,200,67,108,0,128,157,67,64,187,201,67,108,0,128,162,67,64,187,201,67,108,0,128,162,67,64,187,201,67,108,10,135,162,67,19,187,201,67,108,15,142,162,67,139,186,201,67,108,11,149,162,67,169,185,201,67,108,250,155,
			162,67,111,184,201,67,108,214,162,162,67,220,182,201,67,108,157,169,162,67,242,180,201,67,108,72,176,162,67,177,178,201,67,108,213,182,162,67,28,176,201,67,108,63,189,162,67,53,173,201,67,108,129,195,162,67,252,169,201,67,108,152,201,162,67,116,166,201,
			67,108,128,207,162,67,159,162,201,67,108,53,213,162,67,128,158,201,67,108,180,218,162,67,25,154,201,67,108,248,223,162,67,109,149,201,67,108,255,228,162,67,127,144,201,67,108,197,233,162,67,83,139,201,67,108,72,238,162,67,235,133,201,67,108,132,242,162,
			67,75,128,201,67,108,119,246,162,67,119,122,201,67,108,29,250,162,67,114,116,201,67,108,118,253,162,67,65,110,201,67,108,126,0,163,67,230,103,201,67,108,52,3,163,67,103,97,201,67,108,151,5,163,67,199,90,201,67,108,163,7,163,67,11,84,201,67,108,89,9,163,
			67,55,77,201,67,108,183,10,163,67,79,70,201,67,108,188,11,163,67,88,63,201,67,108,103,12,163,67,86,56,201,67,108,185,12,163,67,77,49,201,67,108,192,12,163,67,64,46,201,67,108,192,12,163,67,64,46,201,67,108,192,12,163,67,64,46,196,67,108,64,243,161,67,
			64,46,196,67,99,101,0,0 };

		path.loadPathFromData(pathData, sizeof(pathData));

	}
	else if (name == "Trim Sample Start")
	{
		static const unsigned char pathData[] = { 110,109,0,128,187,67,64,46,201,67,108,0,128,187,67,64,46,211,67,108,0,0,190,67,64,46,207,67,108,0,0,195,67,64,46,206,67,108,0,0,190,67,64,46,205,67,108,0,128,187,67,64,46,201,67,99,109,0,20,185,67,64,61,204,67,108,0,20,185,67,192,53,205,67,108,64,106,
			183,67,192,53,205,67,108,64,106,183,67,0,39,207,67,108,0,20,185,67,0,39,207,67,108,0,20,185,67,128,31,208,67,108,192,189,186,67,64,46,206,67,108,0,20,185,67,64,61,204,67,99,101,0,0 };

		path.loadPathFromData(pathData, sizeof(pathData));
	}
	else if (name == "Deselect all Samples") return ColumnIcons::getPath(EditorIcons::cancelIcon, sizeof(EditorIcons::cancelIcon));
	else if (name == "Undo")	return ColumnIcons::getPath(EditorIcons::undoIcon, sizeof(EditorIcons::undoIcon));
	else if (name == "Redo")	return ColumnIcons::getPath(EditorIcons::redoIcon, sizeof(EditorIcons::redoIcon));
	else if (name == "Rebuild")return ColumnIcons::getPath(ColumnIcons::moveIcon, sizeof(ColumnIcons::moveIcon));
	else
	{
		jassertfalse;
	}
	
	
	return path;
}

} // namespace hise
//[/EndFile]
