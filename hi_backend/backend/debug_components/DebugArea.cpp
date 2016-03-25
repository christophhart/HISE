/*
  ==============================================================================

    DebugArea.cpp
    Created: 1 Nov 2014 11:39:29am
    Author:  Christoph

  ==============================================================================
*/

#include "DebugArea.h"


BaseDebugArea::BaseDebugArea(BackendProcessorEditor *mainEditor_):
  mainEditor(mainEditor_)
{
	addAndMakeVisible(panel = new ConcertinaPanel());

	panel->addMouseListener(this, true);

	emptyComponent = new Component("");

	addAndMakeVisible(debugToolbar = new Toolbar());
	
	panel->addPanel(-1, emptyComponent, false);

    panel->setPanelHeaderSize(emptyComponent, 0);
    
	panel->setLookAndFeel(&laf);

	setOpaque(false);
}


CombinedDebugArea::CombinedDebugArea(BackendProcessorEditor *mainEditor_):
  BaseDebugArea(mainEditor_)
{
	
	poolTable = new SamplePoolTable(mainEditor->getBackendProcessor()->getSampleManager().getModulatorSamplerSoundPool());

	
	externalFileTable = new ExternalFileTable<AudioSampleBuffer>(mainEditor->getBackendProcessor()->getSampleManager().getAudioSampleBufferPool());

	imageTable = new ExternalFileTable<Image>(mainEditor->getBackendProcessor()->getSampleManager().getImagePool());

	fileBrowser = new FileBrowser(mainEditor);

	processorDragList = new ModuleBrowser(this);
	apiCollection = new ApiCollection(this);

	

	ShapeButton *modulesButton = new ShapeButton("Modules", Colour(BACKEND_ICON_COLOUR_OFF), Colour(BACKEND_ICON_COLOUR_OFF), Colour(BACKEND_ICON_COLOUR_OFF));
	modulesButton->setTooltip("Module Collection");
	Path modulesPath;

	modulesPath.loadPathFromData(BackendBinaryData::ToolbarIcons::addIcon, sizeof(BackendBinaryData::ToolbarIcons::addIcon));

	modulesButton->setShape(modulesPath, true, true, false);

	addButtonToToolbar(modulesButton);

	ShapeButton *apiButton = new ShapeButton("API", Colour(BACKEND_ICON_COLOUR_OFF), Colour(BACKEND_ICON_COLOUR_OFF), Colour(BACKEND_ICON_COLOUR_OFF));
	modulesButton->setTooltip("API List");
	Path apiPath;

	apiPath.loadPathFromData(BackendBinaryData::ToolbarIcons::apiList, sizeof(BackendBinaryData::ToolbarIcons::apiList));

	apiButton->setShape(apiPath, true, true, false);

	addButtonToToolbar(apiButton);

	ShapeButton *sampleButton = new ShapeButton("Plotter", Colour(BACKEND_ICON_COLOUR_OFF), Colour(BACKEND_ICON_COLOUR_OFF), Colour(BACKEND_ICON_COLOUR_OFF));
	Path samplePath;

	samplePath.loadPathFromData(BackendBinaryData::ToolbarIcons::sampleTable, sizeof(BackendBinaryData::ToolbarIcons::sampleTable));
	sampleButton->setShape(samplePath, true, true, false);
	sampleButton->setTooltip("Referenced sample files");

	addButtonToToolbar(sampleButton);

	ShapeButton *fileButton = new ShapeButton("Plotter", Colour(BACKEND_ICON_COLOUR_OFF), Colour(BACKEND_ICON_COLOUR_OFF), Colour(BACKEND_ICON_COLOUR_OFF));
	Path filePath;

	filePath.loadPathFromData(BackendBinaryData::ToolbarIcons::fileTable, sizeof(BackendBinaryData::ToolbarIcons::fileTable));
	fileButton->setShape(filePath, true, true, false);
	fileButton->setTooltip("Referenced audio files");

	addButtonToToolbar(fileButton);
	
	ShapeButton *imageButton = new ShapeButton("Plotter", Colour(BACKEND_ICON_COLOUR_OFF), Colour(BACKEND_ICON_COLOUR_OFF), Colour(BACKEND_ICON_COLOUR_OFF));
	Path imagePath;

	imagePath.loadPathFromData(BackendBinaryData::ToolbarIcons::imageTable, sizeof(BackendBinaryData::ToolbarIcons::imageTable));
	imageButton->setShape(imagePath, true, true, false);
	imageButton->setTooltip("Referenced image files");

	addButtonToToolbar(imageButton);
	
	
}

Identifier CombinedDebugArea::getIdForComponent(int i) const
{
	switch (i)
	{
	case SamplePool:		return "SamplePool";
	case ExternalFiles:		return "FileBrowser"; // externalFileTable;
	case ImageTable:		return "ImagePool";
	case ProcessorCollection: return "ModuleBrowser";
	case ApiCollectionEnum:	return "ApiBrowser";
	case numAreas:			return "Empty";
	default:				jassertfalse; return "";
	}
}

Component *CombinedDebugArea::getComponentForIndex(int i) const
{
	switch(i)
	{
	case SamplePool:		return poolTable;
	case ExternalFiles:		return fileBrowser; // externalFileTable;
	case ImageTable:		return imageTable;
	case ProcessorCollection: return processorDragList;
	case ApiCollectionEnum:	return apiCollection;
	case numAreas:			return emptyComponent;
	default:				jassertfalse; return nullptr;
	}
}

void BaseDebugArea::paint(Graphics &g)
{

        g.setColour(Colour(BACKEND_BG_COLOUR_BRIGHT));
        g.fillRect(panel->getBounds());
    
        g.setGradientFill(ColourGradient(Colours::black.withAlpha(0.1f),
                                     (float)panel->getX(), (float)panel->getY(),
                                     Colours::transparentBlack,
                                     (float)panel->getX(), (float)panel->getBottom(),
                                     false));
        g.fillRect(panel->getBounds());
    
        const Colour shadowColour = Colour(0x22000000);
    
        g.setGradientFill(ColourGradient(shadowColour,
									(float)panel->getX(), (float)panel->getY(),
                                     Colour(0),
									 (float)panel->getX(), (float)panel->getY() + 4.0f,
                                     false));
        g.fillRect(panel->getX(), panel->getY(), panel->getWidth(), 4);
    

        g.setGradientFill(ColourGradient(Colour(0),
			(float)panel->getX(), (float)panel->getBottom() - 4.0f,
                                     shadowColour,
									 (float)panel->getX(), (float)panel->getBottom(),
                                     false));
        g.fillRect(panel->getX(), panel->getBottom() -4, panel->getWidth(), 4);
    

    
    
        g.setGradientFill(ColourGradient(shadowColour,
                                         0.0f, 0.0f,
                                         Colour(0),
                                         4.0f, 0.0f,
                                         false));
        g.fillRect(0, panel->getY(), 4, panel->getHeight());
        
        g.setGradientFill(ColourGradient(shadowColour,
										 (float)getWidth(), 0.0f,
                                         Colour(0),
										 (float)getWidth() - 4.0f, 0.0f,
                                         false));
        g.fillRect(getWidth()-4, panel->getY(), 4, panel->getHeight());
    
    
}

void BaseDebugArea::showComponent(int index, bool shouldBeShown)
{
	if (shouldBeShown != isInPanel(getComponentForIndex(index)))
	{
		debugToolbar->showComponent(index, shouldBeShown, true);
	}
}

void BaseDebugArea::addButtonToToolbar(ShapeButton *newButton)
{
	debugToolbar->toolbarButtons.add(newButton);
	addAndMakeVisible(newButton);
	newButton->addListener(debugToolbar);
}

CombinedDebugArea::CombinedDebugToolbar::CombinedDebugToolbar() :
  BaseDebugArea::Toolbar()
{
}


PropertyDebugArea::PropertyDebugArea(BackendProcessorEditor *editor) :
BaseDebugArea(editor)
{
	console = new Console(this);
	mainEditor->getBackendProcessor()->setConsole(console);

	plotter = new Plotter(this);
	plotter->setColour(Plotter::backgroundColour, Colour(DEBUG_AREA_BACKGROUND_COLOUR));
	mainEditor->getBackendProcessor()->setPlotter(plotter);

	macroTable = new MacroParameterTable(this);

	scriptWatchTable = new ScriptWatchTable(mainEditor->getBackendProcessor(), this);

	scriptComponentPanel = new ScriptComponentEditPanel(this);

	moduleList = new PatchBrowser(this, editor);

	mainEditor->getBackendProcessor()->setScriptWatchTable(scriptWatchTable);
	mainEditor->getBackendProcessor()->setScriptComponentEditPanel(scriptComponentPanel);

	ShapeButton *macroButton = new ShapeButton("Macro", Colour(BACKEND_ICON_COLOUR_OFF), Colour(BACKEND_ICON_COLOUR_OFF), Colour(BACKEND_ICON_COLOUR_OFF));
	Path macroPath;

	macroPath.loadPathFromData(BackendBinaryData::ToolbarIcons::macros, sizeof(BackendBinaryData::ToolbarIcons::macros));
	macroButton->setShape(macroPath, true, true, false);
	macroButton->setTooltip("Macro Control Parameter List");

	addButtonToToolbar(macroButton);

	ShapeButton *scriptButton = new ShapeButton("Script Auto Watch", Colour(BACKEND_ICON_COLOUR_OFF), Colour(BACKEND_ICON_COLOUR_OFF), Colour(BACKEND_ICON_COLOUR_OFF));
	Path scriptPath;

	scriptPath.loadPathFromData(HiBinaryData::SpecialSymbols::scriptProcessor, sizeof(HiBinaryData::SpecialSymbols::scriptProcessor));
	scriptButton->setShape(scriptPath, true, true, false);
	scriptButton->setTooltip("Script Debugger: View variable values & objects");

	addButtonToToolbar(scriptButton);


	ShapeButton *editButton = new ShapeButton("Script Auto Watch", Colour(BACKEND_ICON_COLOUR_OFF), Colour(BACKEND_ICON_COLOUR_OFF), Colour(BACKEND_ICON_COLOUR_OFF));
	Path editPath;

	editPath.loadPathFromData(BackendBinaryData::ToolbarIcons::mixer, sizeof(BackendBinaryData::ToolbarIcons::mixer));
	editPath.applyTransform(AffineTransform::rotation(float_Pi / 2.0f));
	editButton->setShape(editPath, true, true, false);
	editButton->setTooltip("Interface Editor: edit script component properties");

	addButtonToToolbar(editButton);

	ShapeButton *plotButton = new ShapeButton("Plotter", Colour(BACKEND_ICON_COLOUR_OFF), Colour(BACKEND_ICON_COLOUR_OFF), Colour(BACKEND_ICON_COLOUR_OFF));
	Path plotPath;

	plotPath.loadPathFromData(BackendBinaryData::ToolbarIcons::plotter, sizeof(BackendBinaryData::ToolbarIcons::plotter));
	plotButton->setShape(plotPath, true, true, false);
	plotButton->setTooltip("Modulator Data Plotter");

	addButtonToToolbar(plotButton);

	ShapeButton *debugButton = new ShapeButton("Debug", Colour(BACKEND_ICON_COLOUR_OFF), Colour(BACKEND_ICON_COLOUR_OFF), Colour(BACKEND_ICON_COLOUR_OFF));
	debugButton->setTooltip("Console");
	
	Path debugPath;
	debugPath.loadPathFromData(BackendBinaryData::ToolbarIcons::debugPanel, sizeof(BackendBinaryData::ToolbarIcons::debugPanel));
	debugButton->setShape(debugPath, true, true, false);
	addButtonToToolbar(debugButton);

	ShapeButton *moduleBrowserButton = new ShapeButton("Module Browser", Colour(BACKEND_ICON_COLOUR_OFF), Colour(BACKEND_ICON_COLOUR_OFF), Colour(BACKEND_ICON_COLOUR_OFF));
	debugButton->setTooltip("Module Browser");

	Path moduleBrowserPath;
	moduleBrowserPath.loadPathFromData(BackendBinaryData::ToolbarIcons::modulatorList, sizeof(BackendBinaryData::ToolbarIcons::modulatorList));
	moduleBrowserButton->setShape(moduleBrowserPath, true, true, false);
	addButtonToToolbar(moduleBrowserButton);

}


Identifier PropertyDebugArea::getIdForComponent(int i) const
{
	switch (i)
	{
	case MacroArea:			return "MacroTable";
	case ScriptTable:		return "ScriptDebugger";
	case ScriptComponentPanel:	return "InterfaceDesigner";
	case ModulationPlotter:	return "Plotter";
	case DebugConsole:		return "Console";
	case ModuleBrowser:		return "PresetOverview";
	case numAreas:			return "empty";
	default:				jassertfalse; return "";
	}
}

Component * PropertyDebugArea::getComponentForIndex(int i) const
{
	switch (i)
	{
	case MacroArea:			return macroTable;
	case ScriptTable:		return scriptWatchTable;
	case ScriptComponentPanel:	return scriptComponentPanel;
	case ModulationPlotter:	return plotter;
	case DebugConsole:		return console;
	case ModuleBrowser:		return moduleList;
	case numAreas:			return emptyComponent;
	default:				jassertfalse; return nullptr;
	}
}

PropertyDebugArea::PropertyDebugToolbar::PropertyDebugToolbar()
{
}
