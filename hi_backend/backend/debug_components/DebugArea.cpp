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
*   Commercial licences for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licencing:
*
*   http://www.hartinstruments.net/hise/
*
*   HISE is based on the JUCE library,
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

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
	
	

	
	externalFileTable = new ExternalFileTable<AudioSampleBuffer>(mainEditor->getBackendProcessor()->getSampleManager().getAudioSampleBufferPool());

	imageTable = new ExternalFileTable<Image>(mainEditor->getBackendProcessor()->getSampleManager().getImagePool());

	
	ShapeButton *modulesButton = new ShapeButton("Modules", Colour(BACKEND_ICON_COLOUR_OFF), Colour(BACKEND_ICON_COLOUR_OFF), Colour(BACKEND_ICON_COLOUR_OFF));
	modulesButton->setTooltip("Module Collection");
	Path modulesPath;

	modulesPath.loadPathFromData(BackendBinaryData::ToolbarIcons::addIcon, sizeof(BackendBinaryData::ToolbarIcons::addIcon));

	modulesButton->setShape(modulesPath, true, true, false);

	addButtonToToolbar(modulesButton);

	ShapeButton *apiButton = new ShapeButton("API", Colour(BACKEND_ICON_COLOUR_OFF), Colour(BACKEND_ICON_COLOUR_OFF), Colour(BACKEND_ICON_COLOUR_OFF));
	apiButton->setTooltip("API List");
	Path apiPath;

	apiPath.loadPathFromData(BackendBinaryData::ToolbarIcons::apiList, sizeof(BackendBinaryData::ToolbarIcons::apiList));

	apiButton->setShape(apiPath, true, true, false);

	addButtonToToolbar(apiButton);

	ShapeButton *fileBrowserButton = new ShapeButton("Plotter", Colour(BACKEND_ICON_COLOUR_OFF), Colour(BACKEND_ICON_COLOUR_OFF), Colour(BACKEND_ICON_COLOUR_OFF));
	Path fileBrowserPath;

	fileBrowserPath.loadPathFromData(BackendBinaryData::ToolbarIcons::fileBrowser, sizeof(BackendBinaryData::ToolbarIcons::fileBrowser));
	fileBrowserButton->setShape(fileBrowserPath, true, true, false);
	fileBrowserButton->setTooltip("File Browser");

	addButtonToToolbar(fileBrowserButton);

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
	case ExternalFiles:		return "ExternalFiles";
	case ImageTable:		return "ImagePool";
	case ProcessorCollection: return "ModuleBrowser";
	case ApiCollectionEnum:	return "ApiBrowser";
	case FileBrowserShown:	return "FileBrowser";
	case numAreas:			return "Empty";
	default:				jassertfalse; return "";
	}
}

Component *CombinedDebugArea::getComponentForIndex(int i) const
{
	switch(i)
	{
	case SamplePool:		return poolTable;
	case FileBrowserShown:  return fileBrowser;
	case ExternalFiles:		return externalFileTable;
	case ImageTable:		return imageTable;
	case ProcessorCollection: return processorDragList;
	case ApiCollectionEnum:	return apiCollection;
	case numAreas:			return emptyComponent;
	default:				jassertfalse; return nullptr;
	}
}

void BaseDebugArea::paint(Graphics &g)
{

        g.setColour(JUCE_LIVE_CONSTANT_OFF(Colour(0xff373737)));
        g.fillRect(panel->getBounds());
    
		g.setColour(JUCE_LIVE_CONSTANT_OFF(Colour(0xff2c2c2c)));

		g.drawVerticalLine(0, (float)panel->getY(), (float)getHeight());

		g.setColour(JUCE_LIVE_CONSTANT_OFF(Colour(0xff5f5f5f)));

		g.drawVerticalLine(getWidth()-1, (float)panel->getY(), (float)getHeight());
		

        g.setGradientFill(ColourGradient(Colours::black.withAlpha(0.1f),
                                     (float)panel->getX(), (float)panel->getY(),
                                     Colours::transparentBlack,
                                     (float)panel->getX(), (float)panel->getBottom(),
                                     false));
        //g.fillRect(panel->getBounds());
    
        const Colour shadowColour = Colour(0x22000000);
    
        g.setGradientFill(ColourGradient(shadowColour,
									(float)panel->getX(), (float)panel->getY(),
                                     Colour(0),
									 (float)panel->getX(), (float)panel->getY() + 4.0f,
                                     false));
        //g.fillRect(panel->getX(), panel->getY(), panel->getWidth(), 4);
    

        g.setGradientFill(ColourGradient(Colour(0),
			(float)panel->getX(), (float)panel->getBottom() - 4.0f,
                                     shadowColour,
									 (float)panel->getX(), (float)panel->getBottom(),
                                     false));
        //g.fillRect(panel->getX(), panel->getBottom() -4, panel->getWidth(), 4);
    

    
    
        g.setGradientFill(ColourGradient(shadowColour,
                                         0.0f, 0.0f,
                                         Colour(0),
                                         4.0f, 0.0f,
                                         false));
        //g.fillRect(0, panel->getY(), 4, panel->getHeight());
        
        g.setGradientFill(ColourGradient(shadowColour,
										 (float)getWidth(), 0.0f,
                                         Colour(0),
										 (float)getWidth() - 4.0f, 0.0f,
                                         false));
        //g.fillRect(getWidth()-4, panel->getY(), 4, panel->getHeight());
    
    
}

void BaseDebugArea::mouseDown(const MouseEvent& e)
{
	auto c = e.eventComponent->findParentComponentOfClass<AutoPopupDebugComponent>();

	if (c == nullptr)
	{
		c = dynamic_cast<AutoPopupDebugComponent*>(e.eventComponent);
	}

	if (c != nullptr)
	{
		DBG("tut");

		if (e.mods.isAltDown() && e.mods.isCommandDown())
		{
			openInFloatingWindow(getIndexForComponent(dynamic_cast<Component*>(c)), true);
		}
	}

	
}

void BaseDebugArea::showComponent(int index, bool shouldBeShown)
{
	auto c = getComponentForIndex(index);

	if (shouldBeShown != isInPanel(c))
	{
		debugToolbar->showComponent(index, shouldBeShown, true);
	}
}

void BaseDebugArea::openInFloatingWindow(int index, bool shouldBeShown)
{
	auto c = getComponentForIndex(index);

	if (shouldBeShown != isFloating(c))
	{
		if (shouldBeShown)
		{
			showComponent(index, false);

			c->setSize(floatingAreas[index].getWidth(), floatingAreas[index].getHeight());

			floatingWindows.add(new DebugPopout(c, this, floatingAreas[index]));
			floatingWindows.getLast()->setVisible(true);

		}
		else
		{
			for (int i = 0; i < floatingWindows.size(); i++)
			{
				if (floatingWindows[i]->getContentComponent() == c)
				{
					floatingAreas[index] = floatingWindows[i]->getScreenBounds();

					floatingWindows.remove(i);
					break;
				}
			}

			showComponent(index, true);
		}

		dynamic_cast<AutoPopupDebugComponent*>(c)->floatingStateChanged(shouldBeShown);
	}

	repaint();
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
#if 0
	console = new Console(this);
	mainEditor->getBackendProcessor()->setConsole(console);
#endif
	

	
	mainEditor->getBackendProcessor()->setPlotter(plotter);

	macroTable = new MacroParameterTable(this);

	

	mainEditor->getBackendProcessor()->setScriptWatchTable(scriptWatchTable);
	mainEditor->getBackendProcessor()->addScriptComponentEditPanel(scriptComponentPanel);

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

BaseDebugArea::DebugPopout::DebugPopout(Component* c, BaseDebugArea* parent, Rectangle<int>  area) :
	DocumentWindow(c->getName(), Colour(0xFF333333), closeButton, true),
	area(parent)
{
	editor = parent->findParentComponentOfClass<BackendProcessorEditor>();

	setUsingNativeTitleBar(true);

	setContentNonOwned(c, true);

	setResizable(true, true);

	if (area.isEmpty())
	{
		centreWithSize(600, 600);
	}
	else
	{
		setBounds(area);

		
		
	}

	
}

void BaseDebugArea::DebugPopout::closeButtonPressed()
{
	area->openInFloatingWindow(area->getIndexForComponent(getContentComponent()), false);
}
