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

	int numSnippets = p->getNumSnippets();
	int numFiles = p->getNumWatchedFiles();

	const bool isCallback = index < numSnippets;
	const bool isExternalFile = index >= numSnippets && index < (numSnippets + numFiles);
	

	if (isCallback)
	{
		auto pe = new PopupIncludeEditor(p, p->getSnippet(index)->getCallbackName());
		pe->addMouseListener(this, true);
		getProcessor()->getMainController()->setLastActiveEditor(pe->getEditor(), CodeDocument::Position());
		return pe;
	}
	else if (isExternalFile)
	{
		const int fileIndex = index - p->getNumSnippets();

		auto pe = new PopupIncludeEditor(p, p->getWatchedFile(fileIndex));
		pe->addMouseListener(this, true);
		getProcessor()->getMainController()->setLastActiveEditor(pe->getEditor(), CodeDocument::Position());

		return pe;
	}
	else
	{
#if HISE_INCLUDE_SNEX
		int jitNodeIndex = index - numSnippets - numFiles;

		if (auto h = dynamic_cast<scriptnode::DspNetwork::Holder*>(p))
		{
			if (auto network = h->getActiveNetwork())
			{
				auto list = network->getListOfNodesWithType<scriptnode::JitNode>(true);

				if (auto n = dynamic_cast<scriptnode::JitNode*>(list[jitNodeIndex].get()))
				{
					auto v = n->getNodePropertyAsValue(scriptnode::PropertyIds::Code);
					auto pg = new snex::SnexPlayground(v, nullptr); // add the buffer later...
					return pg;
				}
			}
		}
#endif
	}

	return nullptr;
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

		if (auto h = dynamic_cast<scriptnode::DspNetwork::Holder*>(p))
		{
#if HISE_INCLUDE_SNEX
			if (auto network = h->getActiveNetwork())
			{
				auto list = network->getListOfNodesWithType<scriptnode::JitNode>(true);

				for (auto l : list)
					indexList.add("SNEX Node: " + l->getId());
			}
#endif
		}
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







MARKDOWN_CHAPTER(InterfaceDesignerHelp)
START_MARKDOWN(Help)
ML("# The Canvas");
ML("In the middle of the window is the Canvas, which is a preview of the interface that can be edited graphically.");
ML("## Toolbar");
ML("At the top of the canvas you'll find a toolbar that contains a few important tools for the entire Interface designer.");

ML("| Icon | Function | Shortcut | Description |");
ML("| -- | ------ | ---- | ------------------- |");
ML("| ![](Edit) | **Toggle Edit / Play mode** | `F4` | Toggles the canvas between play mode(where you can actually use the Components) and edit mode where you can edit them and drag them around. |");
ML("| ![](Cancel) | **Deselect current item** | `Escape` | Deselects all items |");
ML("| - | **Zoom selector** | `Cmd + / Cmd -` | Change the zoom amount from 50 % to 200 % .|");
ML("| ![](Undo) | **Undo** | `Cmd + Z` | Undo the last property change.This is using the global undo manager, so it also undoes drag operations in the Component List or property changes in the Property Editor. |");
ML("| ![](Redo) | **Redo** | `Cmd + Y` | Same as undo says Captain Obvious. |");
ML("| ![](Rebuild) | **Rebuild Interface** | `F5` | Refreshes the UI model. |");
ML("| ![](Vertical Align) | **Vertical Align** | `` | Align the selection vertically on the left edge. |");
ML("| ![](Horizontal Align) | **Horizontal Align** | `` | Align the selection horizontally on the top edge. |");
ML("| ![](Vertical Distribute) | **Vertical Distribute** | `` | Distribute the selection vertically with equal space. |");
ML("| ![](Horizontal Distribute) | **Horizontal Distribute** | `` | Distribute the selection horizontally with equal space. |");


ML("> Basically HISE uses an additional layer called ScriptComponent, which are wrappers around the data model for each control.  " \
	"They store a reference to the data and communicate between the actual GUI you see and the script / UI model.  " \
	"However, in some cases, their connections breaks up and you wind up with a broken interface which looks pretty bad, but can easily be solved by simply deleting all of those wrappers and recreating them from the UI model, which is precisely what the Rebuild Button is doing.  " \
	"If you rebuild the interface, the script will not automatically be recompiled, so in order to make a full reset, you need to rebuild the interface first, then hit recompile.");

ML("## Selecting elements");
ML("You can select an element either by directly clicking on it or drag a lasso around all controls you want to modify.  ");
ML("If you hold down the Command key while clicking on an element, it will add it to the selection. In order to deselect all elements, just press the Escape key.");
ML("> If you click on an already selected item, it will not be deselected. Instead it will grab the keyboard focus so that you can use the arrow keys to nudge it around (see below).");

ML("## Changing the position / size");
ML("You can change the position and size of the currently selected items by either just dragging them around or resizing them although resizing doesn't work here with multiple elements.");
ML("> If you need to resize multiple elements at once, you'll need the property panel or the arrow keys");
ML("When the canvas has the keyboard focus, you can use the arrow keys for nudging them around. The modifier keys can be used here to change the action. Their effect is similar for both keyboard and mouse interaction, but here is a detailed overview :");

ML("| Modifier Key | Effect with Keystroke | Effect when dragging |");
ML("| ------------ | -------------------- - | -------------------- |");
ML("| **Cmd / Ctrl * * | Use a 10px raster | Use a 10px raster |");
ML("| **Shift** | Change the size instead of the position | Restrict the movement to horizontal / vertical only |");
ML("| **Alt** | nothing | Duplicating the current control |");

ML("> These modifier keys can be combined, so eg. pressing `Shift + Cmd + Right-Key` will increase the width by 10 pixel.");
ML("The changes you make with the arrow keys are completely relative(so you can move / resize a selection without messing up their relation), which makes it the go - to solution for changing the position of multiple elements at once.");
END_MARKDOWN()
END_MARKDOWN_CHAPTER()


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

	zoomSelector->setColour(HiseColourScheme::ComponentFillTopColourId, Colours::black.withAlpha(0.4f));
	zoomSelector->setColour(HiseColourScheme::ComponentFillBottomColourId, Colours::black.withAlpha(0.4f));
	zoomSelector->setColour(HiseColourScheme::ComponentOutlineColourId, Colours::transparentBlack);
	zoomSelector->setColour(HiseColourScheme::ComponentTextColourId, Colours::white.withAlpha(0.8f));

	Factory f;

	addAndMakeVisible(editSelector = new HiseShapeButton("Edit", this, f, "EditOff"));
	addAndMakeVisible(cancelButton = new HiseShapeButton("Cancel", this, f));

	addAndMakeVisible(undoButton = new HiseShapeButton("Undo", this, f));
	undoButton->setTooltip("Undo last item change");

	addAndMakeVisible(redoButton = new HiseShapeButton("Redo", this, f));
	addAndMakeVisible(rebuildButton = new HiseShapeButton("Rebuild", this, f));

	addAndMakeVisible(viewport = new Viewport());

	viewport->setViewedComponent(new Canvas(p), true);

	zoomSelector->setTooltip("Select Update Level for Refreshing the interface (Cmd +/-)");
	zoomSelector->setTooltip("Select Zoom Level");
	editSelector->setTooltip("Toggle Edit / Presentation Mode (F4)");
	cancelButton->setTooltip("Deselect current item (Escape)");
	
	redoButton->setTooltip("Redo last item change");
	rebuildButton->setTooltip("Rebuild Interface (F5)");

	addAndMakeVisible(verticalAlignButton = new HiseShapeButton("Vertical Align", this, f));
	verticalAlignButton->setTooltip("Align the selection vertically on the left edge");
	addAndMakeVisible(horizontalAlignButton = new HiseShapeButton("Horizontal Align", this, f));
	horizontalAlignButton->setTooltip("Align the selection horizontally on the top edge");
	addAndMakeVisible(verticalDistributeButton = new HiseShapeButton("Vertical Distribute", this, f));
	verticalDistributeButton->setTooltip("Distribute the selection vertically with equal space");
	addAndMakeVisible(horizontalDistributeButton = new HiseShapeButton("Horizontal Distribute", this, f));
	horizontalDistributeButton->setTooltip("Distribute the selection horizontally with equal space");

	addAndMakeVisible(helpButton = new MarkdownHelpButton());
	helpButton->setPopupWidth(600);
	
	helpButton->setHelpText<PathProvider<Factory>>(InterfaceDesignerHelp::Help());

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
		helpButton->setVisible(false);
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

		topRow.removeFromLeft(50);

		helpButton->setBounds(topRow.removeFromLeft(24).reduced(2));


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
		Actions::undo(this, true);
	}
	if (b == redoButton)
	{
		Actions::undo(this, false);
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

	Array<Identifier> ids;

	auto b = e->getScriptComponentEditBroadcaster();

	for (auto sc : b->getSelection())
	{
		ids.add(sc->getName());
	}

	content->resetContentProperties();

	for (auto id : ids)
	{
		auto sc = content->getComponentWithName(id);

		if (sc != nullptr)
			b->addToSelection(sc, id == ids.getLast() ? sendNotification : dontSendNotification);
	}
}

void ScriptContentPanel::Editor::Actions::rebuildAndRecompile(Editor* e)
{
    auto jp = dynamic_cast<JavascriptProcessor*>(e->getProcessor());
    auto content = jp->getContent();
    
    content->setIsRebuilding(true);
    
    Array<Identifier> ids;
    
    auto b = e->getScriptComponentEditBroadcaster();
    
    for (auto sc : b->getSelection())
    {
        ids.add(sc->getName());
    }

    auto f = [ids, b](Processor* p)
    {
		auto content = dynamic_cast<JavascriptProcessor*>(p)->getContent();

		content->resetContentProperties();

		auto jp = dynamic_cast<JavascriptProcessor*>(content->getProcessor());

        jp->compileScript();
        

		auto f2 = [ids, b](Dispatchable* p)
		{
			auto content = dynamic_cast<JavascriptProcessor*>(p)->getContent();

			for (auto id : ids)
			{
				auto sc = content->getComponentWithName(id);

				if (sc != nullptr)
					b->addToSelection(sc, id == ids.getLast() ? sendNotification : dontSendNotification);
			}

			content->setIsRebuilding(false);

			return Dispatchable::Status::OK;
		};

		p->getMainController()->getLockFreeDispatcher().callOnMessageThreadAfterSuspension(p, f2);

		return SafeFunctionCall::OK;
    };
    
	auto p = e->getProcessor();

	p->getMainController()->getKillStateHandler().killVoicesAndCall(p, f, MainController::KillStateHandler::ScriptingThread);
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

void ScriptContentPanel::Editor::Actions::undo(Editor * e, bool shouldUndo)
{
	e->getScriptComponentEditBroadcaster()->undo(shouldUndo);

	rebuild(e);

	
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
	else if (key == KeyPress::F5Key)
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

	auto swt = new ScriptWatchTable();

	swt->setHolder(dynamic_cast<JavascriptProcessor*>(getConnectedProcessor()));

	return swt;
}


Array<PathFactory::KeyMapping> ScriptContentPanel::Factory::getKeyMapping() const
{
	Array<KeyMapping> km;

	km.add({ "edit", KeyPress::F4Key });
	km.add({ "editoff", KeyPress::F4Key });
	km.add({ "cancel", KeyPress::escapeKey });
	km.add({ "Compile", KeyPress::F5Key });
	km.add({ "Rebuild", KeyPress::F5Key, ModifierKeys::commandModifier });
	km.add({ "Zoom in", '+', ModifierKeys::commandModifier });
	km.add({ "Zoom out", '-', ModifierKeys::commandModifier });
	km.add({ "Undo", 'z', ModifierKeys::commandModifier });
	km.add({ "Redo", 'y', ModifierKeys::commandModifier });

	return km;
}


juce::Path ScriptContentPanel::Factory::createPath(const String& id) const
{
	auto url = MarkdownLink::Helpers::getSanitizedFilename(id);
	Path p;

	LOAD_PATH_IF_URL("edit", OverlayIcons::penShape);
	LOAD_PATH_IF_URL("editoff"				, OverlayIcons::lockShape);
	LOAD_PATH_IF_URL("cancel"				, EditorIcons::cancelIcon);
	LOAD_PATH_IF_URL("undo"				, EditorIcons::undoIcon);
	LOAD_PATH_IF_URL("redo"				, EditorIcons::redoIcon);
	LOAD_PATH_IF_URL("rebuild"				, ColumnIcons::moveIcon);
	LOAD_PATH_IF_URL("vertical-align"		, ColumnIcons::verticalAlign);
	LOAD_PATH_IF_URL("horizontal-align"	, ColumnIcons::horizontalAlign);
	LOAD_PATH_IF_URL("vertical-distribute" , ColumnIcons::verticalDistribute);
	LOAD_PATH_IF_URL("horizontal-distribute", ColumnIcons::horizontalDistribute);

	return p;
}

} // namespace hise
