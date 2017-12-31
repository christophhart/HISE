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
*   Commercial licenses for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licensing:
*
*   http://www.hise.audio/
*
*   HISE is based on the JUCE library,
*   which must be separately licensed for closed source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

namespace hise { using namespace juce;

CodeEditorPanel::CodeEditorPanel(FloatingTile* parent) :
	PanelWithProcessorConnection(parent)
{
	tokeniser = new JavascriptTokeniser();

	getMainController()->addScriptListener(this);

}

CodeEditorPanel::~CodeEditorPanel()
{
	tokeniser = nullptr;
	getMainController()->removeScriptListener(this);
}


Component* CodeEditorPanel::createContentComponent(int index)
{
	auto p = dynamic_cast<JavascriptProcessor*>(getProcessor());

	const int numSnippets = p->getNumSnippets();
	const int numFiles = p->getNumWatchedFiles();

	const bool isCallback = index < p->getNumSnippets();

	const bool isJSONData = index == (numSnippets + numFiles);

	if (isCallback)
	{
		auto pe = new PopupIncludeEditor(p, p->getSnippet(index)->getCallbackName());
		pe->addMouseListener(this, true);
		getProcessor()->getMainController()->setLastActiveEditor(pe->getEditor(), CodeDocument::Position());
		return pe;
	}
	else if (isJSONData)
	{
		auto pe = new PopupIncludeEditor(p, Identifier("JsonData"));
		pe->addMouseListener(this, true);
		getProcessor()->getMainController()->setLastActiveEditor(pe->getEditor(), CodeDocument::Position());
		return pe;
	}
	else
	{
		const int fileIndex = index - p->getNumSnippets();

		auto pe = new PopupIncludeEditor(p, p->getWatchedFile(fileIndex));
		pe->addMouseListener(this, true);
		getProcessor()->getMainController()->setLastActiveEditor(pe->getEditor(), CodeDocument::Position());

		return pe;
	}
}


void CodeEditorPanel::fillModuleList(StringArray& moduleList)
{
	Processor::Iterator<JavascriptProcessor> iter(getMainSynthChain(), false);

	while (auto p = iter.getNextProcessor())
	{
		if (p->isConnectedToExternalFile())
			continue;

		moduleList.add(dynamic_cast<Processor*>(p)->getId());
	}
}


void CodeEditorPanel::scriptWasCompiled(JavascriptProcessor *processor)
{
	if (getProcessor() == dynamic_cast<Processor*>(processor))
		refreshIndexList();
}

void CodeEditorPanel::mouseDown(const MouseEvent& event)
{
	if (auto tab = findParentComponentOfClass<FloatingTabComponent>())
	{
		if (tab->getNumTabs() > 1)
			return;
	}

	if (event.mods.isX2ButtonDown())
	{
		incIndex(true);
	}
	else if (event.mods.isX1ButtonDown())
	{
		incIndex(false);
	}
}

var CodeEditorPanel::getAdditionalUndoInformation() const
{
	auto pe = getContent<PopupIncludeEditor>();

	if (pe != nullptr && pe->getEditor() != nullptr)
	{
		

		return var(pe->getEditor()->getFirstLineOnScreen());
	}

	return var(0);
}

void CodeEditorPanel::performAdditionalUndoInformation(const var& undoInformation)
{
	if (!undoInformation.isInt())
		return;

	auto pe = getContent<PopupIncludeEditor>();

	if (pe != nullptr)
	{
		const int lineIndex = (int)undoInformation;

		if (lineIndex > 0)
		{
			pe->getEditor()->scrollToLine(lineIndex);
		}
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

		indexList.add("UI JSON Data");
	}
}

void CodeEditorPanel::gotoLocation(Processor* p, const String& fileName, int charNumber)
{
	if (fileName.isEmpty())
	{
		setContentWithUndo(p, 0);
	}
	else if (fileName.contains("()"))
	{
		auto jp = dynamic_cast<JavascriptProcessor*>(p);

		auto snippetId = Identifier(fileName.upToFirstOccurrenceOf("()", false, false));

		

		for (int i = 0; i < jp->getNumSnippets(); i++)
		{
			if (jp->getSnippet(i)->getCallbackName() == snippetId)
			{
				setContentWithUndo(p, i);
				break;
			}
		}
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
	addAndMakeVisible(console = new Console(parent->getMainController()));
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
	var obj = PanelWithProcessorConnection::toDynamicObject();

	auto editor = getContent<Editor>();
	
	if (editor != nullptr)
	{
		storePropertyInObject(obj, SpecialPanelIds::ZoomAmount, editor->getZoomAmount(), 1.0);
		storePropertyInObject(obj, SpecialPanelIds::EditMode, editor->isEditModeEnabled(), false);
	}
	else
	{
		storePropertyInObject(obj, SpecialPanelIds::ZoomAmount, 1.0, 1.0);
		storePropertyInObject(obj, SpecialPanelIds::EditMode, false, false);
	}

	

	return obj;
}

void ScriptContentPanel::fromDynamicObject(const var& object)
{
	PanelWithProcessorConnection::fromDynamicObject(object);

	auto editor = getContent<Editor>();

	if (editor != nullptr)
	{
		editor->setZoomAmount(getPropertyWithDefault(object, SpecialPanelIds::ZoomAmount));
		editor->setEditMode(getPropertyWithDefault(object, SpecialPanelIds::EditMode));
	}
}

Identifier ScriptContentPanel::getProcessorTypeId() const
{
	return JavascriptProcessor::getConnectorId();
}

Component* ScriptContentPanel::createContentComponent(int /*index*/)
{
	return new Editor(getConnectedProcessor());
}


void ScriptContentPanel::fillModuleList(StringArray& moduleList)
{
	fillModuleListWithType<JavascriptProcessor>(moduleList);
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
		bool isInContent = findParentComponentOfClass<ScriptContentComponent>() != nullptr;

		if (!isInContent)
		{
			g.setColour(JUCE_LIVE_CONSTANT_OFF(Colour(0xFF1b1b1b)));

			auto overlayBounds = FLOAT_RECTANGLE(getLocalArea(overlay, overlay->getLocalBounds()));

			g.fillRect(overlayBounds);

			g.setColour(Colours::white.withAlpha(0.5f));

			

			g.drawHorizontalLine((int)overlayBounds.getY(), overlayBounds.getX()-5.0f, overlayBounds.getX());
			g.drawHorizontalLine((int)overlayBounds.getY(), overlayBounds.getRight(), overlayBounds.getRight() + 5);

			g.drawHorizontalLine((int)overlayBounds.getBottom(), overlayBounds.getX() - 5.0f, overlayBounds.getX());
			g.drawHorizontalLine((int)overlayBounds.getBottom(), overlayBounds.getRight(), overlayBounds.getRight() + 5);

			g.drawVerticalLine((int)overlayBounds.getX(), overlayBounds.getY() - 5.0f, overlayBounds.getY());
			g.drawVerticalLine((int)overlayBounds.getX(), overlayBounds.getBottom(), overlayBounds.getBottom() + 5);

			g.drawVerticalLine((int)overlayBounds.getRight(), overlayBounds.getY() - 5.0f, overlayBounds.getY());
			g.drawVerticalLine((int)overlayBounds.getRight(), overlayBounds.getBottom(), overlayBounds.getBottom() + 5.0f);
		}

		
	}

	void selectOnInitCallback() override
	{

	}

	void setZoomLevel(float zoomAmount)
	{
		if (zoomLevel != zoomAmount)
		{
			zoomLevel = zoomAmount;

			auto s1 = AffineTransform::scale((float)zoomAmount);
			auto s2 = AffineTransform::scale((float)zoomAmount);

			content->setTransform(s1);
			overlay->setTransform(s2);

			refreshContent();
		}

		

		

	}

	void scriptEditHandlerCompileCallback()
	{
		if (getScriptEditHandlerProcessor())
			getScriptEditHandlerProcessor()->compileScript();
	}

	void refreshContent()
	{
		auto unscaledWidth = content->getContentWidth();
		auto unscaledHeight = content->getContentHeight();

		auto vp = findParentComponentOfClass<Viewport>();

		auto vpw = vp != nullptr ? vp->getWidth() - vp->getScrollBarThickness() : 0;
		auto vph = vp != nullptr ? vp->getHeight() - vp->getScrollBarThickness() : 0;

		auto w = (int)((float)unscaledWidth * zoomLevel) + 20;
		auto h = (int)((float)unscaledHeight * zoomLevel) + 20;

		setSize(jmax<int>(w, vpw), jmax<int>(h, vph));
		resized();
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
		bool isInContent = findParentComponentOfClass<ScriptContentComponent>() != nullptr;

		auto total = getLocalBounds();

		if (isInContent)
		{
			content->setBounds(0, 0, getWidth(), getHeight());
			overlay->setVisible(false);
		}
		else
		{
			int w = content->getContentWidth();
			int h = content->getContentHeight();

			int scaledWidth = (int)((float)w * zoomLevel);
			int scaledHeight = (int)((float)h * zoomLevel);

			float centreX = (float)(getWidth() - w*zoomLevel) / 2;
			float centreY = (float)(getHeight() - h*zoomLevel) / 2;

			if (getWidth() < scaledWidth)
			{
				centreX = 10.0f;
			}

			if (getHeight() < scaledHeight)
			{
				centreY = 10.0f;
			}
			
			int offsetX = (int)(centreX / zoomLevel);
			int offsetY = (int)(centreY / zoomLevel);
			


			content->setBounds(offsetX, offsetY, w, h);
			overlay->setBounds(offsetX, offsetY, w, h);
		}
	}

private:

	float zoomLevel = 1.0f;

	friend class Editor;

	ScopedPointer<ScriptContentComponent> content;
	ScopedPointer<ScriptingContentOverlay> overlay;
	WeakReference<Processor> processor;

};

namespace EditorIcons
{
	static const unsigned char compileIcon[] = { 110,109,0,0,22,67,15,2,192,67,108,0,0,27,67,54,88,195,67,98,0,0,27,67,54,88,195,67,110,3,35,67,195,174,188,67,0,0,37,67,232,171,188,67,98,0,0,37,67,232,171,188,67,110,99,39,67,232,171,188,67,0,0,42,67,232,171,188,67,98,183,253,39,67,232,171,188,67,0,
		0,27,67,93,174,198,67,0,0,27,67,93,174,198,67,108,0,0,17,67,15,2,192,67,99,101,0,0 };

	static const unsigned char cancelIcon[] = { 110,109,116,110,45,66,184,32,152,67,108,215,146,59,66,43,92,150,67,108,0,0,102,66,208,169,155,67,108,148,54,136,66,43,92,150,67,108,198,72,143,66,184,32,152,67,108,99,36,116,66,93,110,157,67,108,198,72,143,66,2,188,162,67,108,148,54,136,66,143,128,164,
		67,108,0,0,102,66,233,50,159,67,108,215,146,59,66,142,128,164,67,108,116,110,45,66,2,188,162,67,108,157,219,87,66,93,110,157,67,99,101,0,0 };

	static const unsigned char undoIcon[] = { 110,109,0,93,96,67,64,87,181,67,98,169,116,87,67,119,74,181,67,238,53,75,67,247,66,184,67,128,173,59,67,64,229,191,67,108,0,0,47,67,64,46,186,67,108,0,0,47,67,64,174,203,67,108,0,0,82,67,64,174,203,67,108,0,86,71,67,128,123,197,67,98,221,255,111,67,79,
		174,178,67,128,164,101,67,210,215,207,67,128,228,102,67,64,179,210,67,98,201,215,119,67,101,133,198,67,205,117,117,67,136,117,181,67,0,93,96,67,64,87,181,67,99,101,0,0 };

	static const unsigned char redoIcon[] = { 110,109,90,186,64,67,64,87,181,67,98,176,162,73,67,118,74,181,67,108,225,85,67,247,66,184,67,218,105,101,67,64,229,191,67,108,90,23,114,67,64,46,186,67,108,90,23,114,67,64,174,203,67,108,90,23,79,67,64,174,203,67,108,90,193,89,67,128,123,197,67,98,125,
		23,49,67,79,174,178,67,218,114,59,67,211,215,207,67,218,50,58,67,64,179,210,67,98,145,63,41,67,101,133,198,67,141,161,43,67,136,117,181,67,90,186,64,67,64,87,181,67,99,101,0,0 };
};


ScriptContentPanel::Editor::Editor(Processor* p):
	ScriptComponentEditListener(p)
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

	
	addAndMakeVisible(cancelButton = new HiseShapeButton("Cancel", this, ColumnIcons::getPath(EditorIcons::cancelIcon, sizeof(EditorIcons::cancelIcon))));

	addAndMakeVisible(undoButton = new HiseShapeButton("Undo", this, ColumnIcons::getPath(EditorIcons::undoIcon, sizeof(EditorIcons::undoIcon))));
	undoButton->setTooltip("Undo last item change");

	addAndMakeVisible(redoButton = new HiseShapeButton("Redo", this, ColumnIcons::getPath(EditorIcons::redoIcon, sizeof(EditorIcons::redoIcon))));

	addAndMakeVisible(rebuildButton = new HiseShapeButton("Rebuild", this, ColumnIcons::getPath(ColumnIcons::moveIcon, sizeof(ColumnIcons::moveIcon))));

	addAndMakeVisible(viewport = new Viewport());

	viewport->setViewedComponent(new Canvas(p), true);

	zoomSelector->setTooltip("Select Update Level for Refreshing the interface (Cmd +/-)");
	zoomSelector->setTooltip("Select Zoom Level");
	editSelector->setTooltip("Toggle Edit / Presentation Mode (F4)");
	cancelButton->setTooltip("Deselect current item (Escape)");
	
	redoButton->setTooltip("Redo last item change");
	rebuildButton->setTooltip("Rebuild Interface (F5)");

	


	addAndMakeVisible(verticalAlignButton = new HiseShapeButton("Vertical Align", this, ColumnIcons::getPath(ColumnIcons::verticalAlign, sizeof(ColumnIcons::verticalAlign))));
	verticalAlignButton->setTooltip("Align the selection vertically on the left edge");
	addAndMakeVisible(horizontalAlignButton = new HiseShapeButton("Horizontal Align", this, ColumnIcons::getPath(ColumnIcons::horizontalAlign, sizeof(ColumnIcons::horizontalAlign))));
	horizontalAlignButton->setTooltip("Align the selection horizontally on the top edge");
	addAndMakeVisible(verticalDistributeButton = new HiseShapeButton("Vertical Distribute", this, ColumnIcons::getPath(ColumnIcons::verticalDistribute, sizeof(ColumnIcons::verticalDistribute))));
	verticalDistributeButton->setTooltip("Distribute the selection vertically with equal space");
	addAndMakeVisible(horizontalDistributeButton = new HiseShapeButton("Horizontal Distribute", this, ColumnIcons::getPath(ColumnIcons::horizontalDistribute, sizeof(ColumnIcons::horizontalDistribute))));
	horizontalDistributeButton->setTooltip("Distribute the selection horizontally with equal space");

	setWantsKeyboardFocus(true);

	updateUndoDescription();

	startTimer(2000);
}


void ScriptContentPanel::Editor::scriptComponentSelectionChanged()
{
	updateUndoDescription();
}


void ScriptContentPanel::Editor::scriptComponentPropertyChanged(ScriptComponent* /*sc*/, Identifier /*idThatWasChanged*/, const var& /*newValue*/)
{
	updateUndoDescription();
}

void ScriptContentPanel::Editor::resized()
{
	const bool isInContent = findParentComponentOfClass<ScriptContentComponent>() != nullptr;

	if (isInContent)
	{
		viewport->setBounds(getLocalBounds());

		viewport->getViewedComponent()->resized();
		viewport->setScrollBarsShown(false, false, false, false);

		editSelector->setVisible(false);
		cancelButton->setVisible(false);
		zoomSelector->setVisible(false);
		undoButton->setVisible(false);
		redoButton->setVisible(false);
		verticalAlignButton->setVisible(false);
		horizontalAlignButton->setVisible(false);
		verticalDistributeButton->setVisible(false);
		horizontalDistributeButton->setVisible(false);
	}
	else
	{
		auto total = getLocalBounds();

		auto topRow = total.removeFromTop(24);

		editSelector->setBounds(topRow.removeFromLeft(24).reduced(2));
		cancelButton->setBounds(topRow.removeFromLeft(24).reduced(2));
		zoomSelector->setBounds(topRow.removeFromLeft(84).reduced(2));

		topRow.removeFromLeft(20);

		undoButton->setBounds(topRow.removeFromLeft(24).reduced(2));
		redoButton->setBounds(topRow.removeFromLeft(24).reduced(2));

		topRow.removeFromLeft(20);

		rebuildButton->setBounds(topRow.removeFromLeft(24).reduced(2));

		topRow.removeFromLeft(50);

		verticalAlignButton->setBounds(topRow.removeFromLeft(24).reduced(2));
		horizontalAlignButton->setBounds(topRow.removeFromLeft(24).reduced(2));
		verticalDistributeButton->setBounds(topRow.removeFromLeft(24).reduced(2));
		horizontalDistributeButton->setBounds(topRow.removeFromLeft(24).reduced(2));

		auto canvas = dynamic_cast<Canvas*>(viewport->getViewedComponent());
		
		viewport->setBounds(total);

		canvas->refreshContent();
	}

}

void ScriptContentPanel::Editor::refreshContent()
{
	if (viewport != nullptr && viewport->getViewedComponent() != nullptr)
		dynamic_cast<Canvas*>(viewport->getViewedComponent())->refreshContent();
}

void ScriptContentPanel::Editor::buttonClicked(Button* b)
{
	auto overlay = dynamic_cast<Canvas*>(viewport->getViewedComponent())->overlay.get();

	if (b == editSelector)
	{
		editSelector->toggle();
		overlay->toggleEditMode();
	}
	if (b == cancelButton)
	{
		Actions::deselectAll(this);
	}
	if (b == undoButton)
	{
		auto& undoManager = getScriptComponentEditBroadcaster()->getUndoManager();
		
		undoManager.undo();

		updateUndoDescription();

	}
	if (b == redoButton)
	{
		auto& undoManager = getScriptComponentEditBroadcaster()->getUndoManager();
		undoManager.redo();

		updateUndoDescription();
	}
	if (b == rebuildButton)
	{
		Actions::rebuildAndRecompile(this);
	}
	if (b == verticalAlignButton)
	{
		Actions::align(this,true);
	}
	if (b == horizontalAlignButton)
	{
		Actions::align(this,false);
	}
	if (b == verticalDistributeButton)
	{
		Actions::distribute(this,true);
	}
	if (b == horizontalDistributeButton)
	{
		Actions::distribute(this,false);
	}
	
}


void ScriptContentPanel::Editor::updateUndoDescription()
{
	auto& undoManager = getScriptComponentEditBroadcaster()->getUndoManager();
	undoButton->setTooltip("Undo " + undoManager.getUndoDescription());
	redoButton->setTooltip("Redo " + undoManager.getRedoDescription());
}

void ScriptContentPanel::Editor::comboBoxChanged(ComboBox* c)
{
	if (c == zoomSelector)
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
	
}

void ScriptContentPanel::Editor::setZoomAmount(double newZoomAmount)
{
	if (zoomAmount != newZoomAmount)
	{
		zoomAmount = newZoomAmount;

		auto canvas = dynamic_cast<Canvas*>(viewport->getViewedComponent());

		canvas->setZoomLevel((float)zoomAmount);

		
		
		if(zoomAmount == 0.5)   zoomSelector->setSelectedId(1, dontSendNotification);
		if (zoomAmount == 0.75) zoomSelector->setSelectedId(2, dontSendNotification);
		if (zoomAmount == 1.0)  zoomSelector->setSelectedId(3, dontSendNotification);
		if (zoomAmount == 1.25) zoomSelector->setSelectedId(4, dontSendNotification);
		if (zoomAmount == 1.5)  zoomSelector->setSelectedId(5, dontSendNotification);
		if (zoomAmount == 2.0)  zoomSelector->setSelectedId(6, dontSendNotification);
		
		viewport->setScrollBarsShown(true, true, true, true);
		

		refreshContent();
		resized();
	}

	
}


double ScriptContentPanel::Editor::getZoomAmount() const
{
	return zoomAmount;
}

void ScriptContentPanel::Editor::setEditMode(bool editModeEnabled)
{
	if (editModeEnabled != editSelector->getToggleState())
	{
		buttonClicked(editSelector);
	}
}

bool ScriptContentPanel::Editor::isEditModeEnabled() const
{
	return editSelector->getToggleState();
}


void ScriptContentPanel::Editor::Actions::deselectAll(Editor* e)
{
	e->getScriptComponentEditBroadcaster()->clearSelection();
}

void ScriptContentPanel::Editor::Actions::rebuild(Editor* e)
{
	auto jp = dynamic_cast<JavascriptProcessor*>(e->getProcessor());
	auto content = jp->getContent();

	content->resetContentProperties();
}

void ScriptContentPanel::Editor::Actions::rebuildAndRecompile(Editor* e)
{
	auto jp = dynamic_cast<JavascriptProcessor*>(e->getProcessor());
	auto content = jp->getContent();

	content->resetContentProperties();

	jp->compileScript();


	e->getScriptComponentEditBroadcaster()->clearSelection();
}

void ScriptContentPanel::Editor::Actions::zoomIn(Editor* e)
{
	int currentId = e->zoomSelector->getSelectedId();
	int numIds = e->zoomSelector->getNumItems();

	int newId = jlimit<int>(1, numIds, currentId + 1);

	e->zoomSelector->setSelectedId(newId, sendNotification);
}

void ScriptContentPanel::Editor::Actions::zoomOut(Editor* e)
{

	int currentId = e->zoomSelector->getSelectedId();
	int numIds = e->zoomSelector->getNumItems();

	int newId = jlimit<int>(1, numIds, currentId - 1);

	e->zoomSelector->setSelectedId(newId, sendNotification);
}

void ScriptContentPanel::Editor::Actions::toggleEditMode(Editor* e)
{
	e->editSelector->triggerClick();
}

struct ComponentPositionComparator
{
	ComponentPositionComparator(bool isVertical_) :
		isVertical(isVertical_)
	{};

	int compareElements(ScriptComponent* first, ScriptComponent* second)
	{
		int firstValue = isVertical ? first->getPosition().getY() : first->getPosition().getX();
		int secondValue = isVertical ? second->getPosition().getY() : second->getPosition().getX();

		return firstValue > secondValue ? 1 : -1;
	}

	bool isVertical;
};

void ScriptContentPanel::Editor::Actions::distribute(Editor* editor, bool isVertical)
{
	auto b = editor->getScriptComponentEditBroadcaster();

	b->getUndoManager().beginNewTransaction("Distribute " + isVertical ? "vertically" : "horizontally");

	auto selection = b->getSelection();

	if (selection.size() < 3)
		return;

	float minV = 100000.0f;
	float maxV = -1.0f;

	ComponentPositionComparator comparator(isVertical);

	selection.sort(comparator);

	for (const auto& sc : selection)
	{
		minV = jmin<float>(minV, (float)(isVertical ? sc->getPosition().getY() : sc->getPosition().getX()));
		maxV = jmax<float>(maxV, (float)(isVertical ? sc->getPosition().getY() : sc->getPosition().getX()));
	}

	const auto delta = (maxV - minV) / float(selection.size()-1);

	auto id = isVertical ? Identifier("y") : Identifier("x");

	

	for (auto& sc : selection)
	{
		b->setScriptComponentProperty(sc, id, (int)minV, sendNotification, false);
		minV += delta;
	}


}

void ScriptContentPanel::Editor::Actions::align(Editor* editor, bool isVertical)
{
	auto b = editor->getScriptComponentEditBroadcaster();
	
	auto selection = b->getSelection();

	int minV = INT_MAX;

	for (const auto& sc : selection)
	{
		minV = jmin<int>(minV, isVertical ? sc->getPosition().getX() : sc->getPosition().getY());
	}

	auto id = isVertical ? Identifier("x") : Identifier("y");

	b->setScriptComponentPropertyForSelection(id, minV, sendNotification);
}

bool ScriptContentPanel::Editor::keyPressed(const KeyPress& key)
{
	if (key == KeyPress::F4Key)
	{
		Actions::toggleEditMode(this);
		return true;
	}
	if (key == KeyPress::escapeKey)
	{
		Actions::deselectAll(this);
		return true;
	}
	else if (key == KeyPress::F5Key && key.getModifiers().isCommandDown())
	{
		Actions::rebuildAndRecompile(this);
		return true;
	}
	else if (key.getKeyCode() == '+' && key.getModifiers().isCommandDown())
	{
		Actions::zoomIn(this);
		return true;
	}
	else if (key.getKeyCode() == '-' && key.getModifiers().isCommandDown())
	{
		Actions::zoomOut(this);
		return true;
	}

	return false;
}

Identifier ScriptWatchTablePanel::getProcessorTypeId() const
{
	return JavascriptProcessor::getConnectorId();
}

Component* ScriptWatchTablePanel::createContentComponent(int /*index*/)
{
	setStyleProperty("showConnectionBar", false);

	auto swt = new ScriptWatchTable(getRootWindow());

	swt->setScriptProcessor(dynamic_cast<JavascriptProcessor*>(getConnectedProcessor()), nullptr);

	return swt;
}

void ConnectorHelpers::tut(PanelWithProcessorConnection* connector, const Identifier &idToSearch)
{
    auto parentContainer = connector->getParentShell()->getParentContainer();
    
	if (parentContainer != nullptr)
	{
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
}


} // namespace hise