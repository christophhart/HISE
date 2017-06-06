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



CodeEditorPanel::CodeEditorPanel(FloatingTile* parent) :
	PanelWithProcessorConnection(parent)
{
	tokeniser = new JavascriptTokeniser();


}

CodeEditorPanel::~CodeEditorPanel()
{
	tokeniser = nullptr;
}


Component* CodeEditorPanel::createContentComponent(int index)
{
	auto p = dynamic_cast<JavascriptProcessor*>(getProcessor());

	const bool isCallback = index < p->getNumSnippets();

	if (isCallback)
	{
		auto pe = new PopupIncludeEditor(p, p->getSnippet(index)->getCallbackName());
		getProcessor()->getMainController()->setLastActiveEditor(pe->getEditor(), CodeDocument::Position());
		return pe;
	}
	else
	{
		const int fileIndex = index - p->getNumSnippets();

		auto pe = new PopupIncludeEditor(p, p->getWatchedFile(fileIndex));
		getProcessor()->getMainController()->setLastActiveEditor(pe->getEditor(), CodeDocument::Position());

		return pe;
	}
}

Identifier CodeEditorPanel::getProcessorTypeId() const
{
	return JavascriptProcessor::getConnectorId();
}

void CodeEditorPanel::fillIndexList(StringArray& indexList)
{
	auto p = dynamic_cast<JavascriptProcessor*>(getConnectedProcessor());

	if (p != nullptr)
	{
		for (int i = 0; i < p->getNumSnippets(); i++)
		{
			indexList.add(p->getSnippet(i)->getCallbackName().toString());
		}

		for (int i = 0; i < p->getNumWatchedFiles(); i++)
		{
			indexList.add(p->getWatchedFile(i).getFileName());
		}
	}
}

void CodeEditorPanel::gotoLocation(Processor* p, const String& fileName, int charNumber)
{
	if (fileName.isEmpty())
	{
		setContentWithUndo(p, 0);
	}
	else
	{
		auto jp = dynamic_cast<JavascriptProcessor*>(p);

		int fileIndex = -1;

		for (int i = 0; i < jp->getNumWatchedFiles(); i++)
		{
			if (jp->getWatchedFile(i).getFullPathName() == fileName)
			{
				fileIndex = i;
				break;
			}
		}

		if (fileIndex != -1)
		{
			setContentWithUndo(p, jp->getNumSnippets() + fileIndex);
		}
		else
			return;
	}

	auto editor = getContent<PopupIncludeEditor>()->getEditor();

	CodeDocument::Position pos(editor->getDocument(), charNumber);
	editor->scrollToLine(jmax<int>(0, pos.getLineNumber()));
}

ConsolePanel::ConsolePanel(FloatingTile* parent) :
	FloatingTileContent(parent)
{
	setInterceptsMouseClicks(false, true);
	addAndMakeVisible(console = new Console(getRootWindow()->getBackendProcessor()));
}

void ConsolePanel::resized()
{
	console->setBounds(getParentShell()->getContentBounds());
}




void ScriptContentPanel::scriptWasCompiled(JavascriptProcessor *processor)
{
	if (processor == dynamic_cast<JavascriptProcessor*>(getConnectedProcessor()))
	{
		
		resized();
		if (getContent<Editor>() != nullptr)
			getContent<Editor>()->refreshContent();
	}
}

var ScriptContentPanel::toDynamicObject() const
{
	var obj = FloatingTileContent::toDynamicObject();

	auto editor = getContent<Editor>();
	
	if (editor != nullptr)
	{
		storePropertyInObject(obj, SpecialPanelIds::ZoomAmount, editor->getZoomAmount(), 1.0);
	}
	else
	{
		storePropertyInObject(obj, SpecialPanelIds::ZoomAmount, 1.0, 1.0);
	}

	

	return obj;
}

void ScriptContentPanel::fromDynamicObject(const var& object)
{
	FloatingTileContent::fromDynamicObject(object);

	
}

Identifier ScriptContentPanel::getProcessorTypeId() const
{
	return JavascriptProcessor::getConnectorId();
}

Component* ScriptContentPanel::createContentComponent(int /*index*/)
{
	return new Editor(getConnectedProcessor());
}

struct ScriptContentPanel::Canvas : public ScriptEditHandler,
	public Component
{

	Canvas(Processor* p) :
		processor(p)
	{
		addAndMakeVisible(content = new ScriptContentComponent(dynamic_cast<ProcessorWithScriptingContent*>(p)));
		addAndMakeVisible(overlay = new ScriptingContentOverlay(this));

		overlay->setShowEditButton(false);

		refreshContent();
	}

	void paint(Graphics& g) override
	{
		g.fillAll(Colours::black);

		g.setColour(Colours::white.withAlpha(0.5f));

		const int edgeWidth = 10;

		g.drawHorizontalLine(10, 5.0f, 10.0f);
		g.drawHorizontalLine(10, (float)getWidth() - 10.0f, (float)getWidth() - 5.0f);

		g.drawHorizontalLine(getHeight() - 10, 5.0f, 10.0f);
		g.drawHorizontalLine(getHeight() - 10, (float)getWidth() - 10.0f, (float)getWidth() - 5.0f);

		g.drawVerticalLine(10, 5.0f, 10.0f);
		g.drawVerticalLine(10, (float)getHeight() - 10.0f, (float)getHeight() - 5.0f);

		g.drawVerticalLine(getWidth() - 10, 5.0f, 10.0f);
		g.drawVerticalLine(getWidth() - 10, (float)getHeight() - 10.0f, (float)getHeight() - 5.0f);
	}

	void selectOnInitCallback() override
	{

	}


	void scriptEditHandlerCompileCallback()
	{
		if (getScriptEditHandlerProcessor())
			getScriptEditHandlerProcessor()->compileScript();
	}

	void refreshContent()
	{
		auto scaleFactor = content->getTransform().getScaleFactor();

		setSize(content->getContentWidth()*scaleFactor + 20, content->getContentHeight()*scaleFactor + 20);
	}

public:

	virtual ScriptContentComponent* getScriptEditHandlerContent() { return content; }

	virtual ScriptingContentOverlay* getScriptEditHandlerOverlay() { return overlay; }

	virtual JavascriptCodeEditor* getScriptEditHandlerEditor()
	{
		if (processor.get())
		{
			return dynamic_cast<JavascriptCodeEditor*>(processor->getMainController()->getLastActiveEditor());
		}

		return nullptr;
	}

	virtual JavascriptProcessor* getScriptEditHandlerProcessor() { return dynamic_cast<JavascriptProcessor*>(processor.get()); }

	void resized() override
	{
		content->setBounds(10, 10, getWidth()-20, getHeight()-20);
		overlay->setBounds(10, 10, getWidth() - 20, getHeight() - 20);
	}

private:

	friend class Editor;

	ScopedPointer<ScriptContentComponent> content;
	ScopedPointer<ScriptingContentOverlay> overlay;
	WeakReference<Processor> processor;

};

namespace EditorIcons
{
	static const unsigned char compileIcon[] = { 110,109,0,0,22,67,15,2,192,67,108,0,0,27,67,54,88,195,67,98,0,0,27,67,54,88,195,67,110,3,35,67,195,174,188,67,0,0,37,67,232,171,188,67,98,0,0,37,67,232,171,188,67,110,99,39,67,232,171,188,67,0,0,42,67,232,171,188,67,98,183,253,39,67,232,171,188,67,0,
		0,27,67,93,174,198,67,0,0,27,67,93,174,198,67,108,0,0,17,67,15,2,192,67,99,101,0,0 };

	static const unsigned char cancelIcon[] = { 110,109,0,0,62,67,93,174,188,67,108,0,0,67,67,166,104,192,67,108,0,0,62,67,239,34,196,67,108,0,0,67,67,239,34,196,67,108,0,0,70,67,93,230,193,67,108,0,0,73,67,239,34,196,67,108,0,0,78,67,239,34,196,67,108,0,0,73,67,166,104,192,67,108,0,0,78,67,93,174,
		188,67,108,0,0,73,67,93,174,188,67,108,0,0,70,67,203,169,191,67,108,0,0,67,67,93,174,188,67,99,101,0,0 };
};


ScriptContentPanel::Editor::Editor(Processor* p)
{
	addAndMakeVisible(zoomSelector = new ComboBox("Zoom"));
	zoomSelector->addListener(this);
	zoomSelector->addItem("50%", 1);
	zoomSelector->addItem("75%", 2);
	zoomSelector->addItem("100%", 3);
	zoomSelector->addItem("125%", 4);
	zoomSelector->addItem("150%", 5);
	zoomSelector->addItem("200%", 6);

	zoomSelector->setSelectedId(3, dontSendNotification);
	zoomSelector->setLookAndFeel(&klaf);

	zoomSelector->setColour(MacroControlledObject::HiBackgroundColours::upperBgColour, Colours::black.withAlpha(0.4f));
	zoomSelector->setColour(MacroControlledObject::HiBackgroundColours::lowerBgColour, Colours::black.withAlpha(0.4f));
	zoomSelector->setColour(MacroControlledObject::HiBackgroundColours::outlineBgColour, Colours::transparentBlack);
	zoomSelector->setColour(MacroControlledObject::HiBackgroundColours::textColour, Colours::white.withAlpha(0.8f));

	addAndMakeVisible(editSelector = new HiseShapeButton("Edit", this, ColumnIcons::getPath(OverlayIcons::penShape, sizeof(OverlayIcons::penShape)),
															     ColumnIcons::getPath(OverlayIcons::lockShape, sizeof(OverlayIcons::lockShape))));

	
	addAndMakeVisible(compileButton = new HiseShapeButton("Compile", this, ColumnIcons::getPath(EditorIcons::compileIcon, sizeof(EditorIcons::compileIcon))));

	addAndMakeVisible(cancelButton = new HiseShapeButton("Cancel", this, ColumnIcons::getPath(EditorIcons::cancelIcon, sizeof(EditorIcons::cancelIcon))));


	addAndMakeVisible(viewport = new Viewport());

	viewport->setViewedComponent(new Canvas(p), true);
}


void ScriptContentPanel::Editor::resized()
{
	int x = 4;
	
	zoomSelector->setBounds(x, 2, 80, 20);

	x = zoomSelector->getRight() + 8;

	editSelector->setBounds(x, 2, 20, 20);

	x = editSelector->getRight() + 8;

	compileButton->setBounds(x, 2, 20, 20);

	viewport->setBounds(getLocalBounds().withTrimmedTop(24));


	

}

void ScriptContentPanel::Editor::refreshContent()
{
	if (viewport != nullptr && viewport->getViewedComponent() != nullptr)
		dynamic_cast<Canvas*>(viewport->getViewedComponent())->refreshContent();
}

void ScriptContentPanel::Editor::buttonClicked(Button* b)
{
	auto content = dynamic_cast<Canvas*>(viewport->getViewedComponent())->content.get();
	auto overlay = dynamic_cast<Canvas*>(viewport->getViewedComponent())->overlay.get();

	if (b == editSelector)
	{
		editSelector->toggle();
		overlay->toggleEditMode();
	}
	if (b == compileButton)
	{
		//content->getScriptProcessor()->compileScript();
	}
}

void ScriptContentPanel::Editor::comboBoxChanged(ComboBox* comboBoxThatHasChanged)
{
	switch (zoomSelector->getSelectedId())
	{
	case 1: setZoomAmount(0.5); break;
	case 2: setZoomAmount(0.75); break;
	case 3: setZoomAmount(1.0); break;
	case 4: setZoomAmount(1.25); break;
	case 5: setZoomAmount(1.5); break;
	case 6: setZoomAmount(2.0); break;
	}
}

void ScriptContentPanel::Editor::setZoomAmount(double newZoomAmount)
{
	if (zoomAmount != newZoomAmount)
	{
		zoomAmount = newZoomAmount;

		auto content = dynamic_cast<Canvas*>(viewport->getViewedComponent())->content.get();
		auto overlay = dynamic_cast<Canvas*>(viewport->getViewedComponent())->overlay.get();

		auto s = AffineTransform::scale((float)zoomAmount);

		content->setTransform(s);
		overlay->setTransform(s);

		refreshContent();
	}

	
}


double ScriptContentPanel::Editor::getZoomAmount() const
{
	return zoomAmount;
}

Identifier ScriptWatchTablePanel::getProcessorTypeId() const
{
	return JavascriptProcessor::getConnectorId();
}

Component* ScriptWatchTablePanel::createContentComponent(int /*index*/)
{
	setStyleProperty(showConnectionBar, false);

	auto swt = new ScriptWatchTable(getRootWindow());

	swt->setScriptProcessor(dynamic_cast<JavascriptProcessor*>(getConnectedProcessor()), nullptr);

	return swt;
}

void ConnectorHelpers::tut(PanelWithProcessorConnection* connector, const Identifier &idToSearch)
{
    auto parentContainer = connector->getParentShell()->getParentContainer();
    
    
    FloatingTile::Iterator<PanelWithProcessorConnection> iter(parentContainer->getParentShell());
    
    while (auto p = iter.getNextPanel())
    {
        if (p == connector)
            continue;
        
        if (p->getProcessorTypeId() != idToSearch)
            continue;
        
        p->setContentWithUndo(connector->getProcessor(), 0);
    }
}

