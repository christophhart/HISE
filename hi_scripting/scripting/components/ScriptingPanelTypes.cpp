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
#if HISE_USE_NEW_CODE_EDITOR
	if (auto pe = getContent<PopupIncludeEditor>())
	{
		scaleFactor = pe->getEditor()->editor.transform.getScaleFactor();
	}
#endif

	auto p = dynamic_cast<JavascriptProcessor*>(getProcessor());

	int numSnippets = p->getNumSnippets();
	int numFiles = p->getNumWatchedFiles();

	const bool isCallback = scriptPath.isEmpty() && index < numSnippets;
	const bool isExternalFile = scriptPath.isNotEmpty() || (index >= numSnippets && (index-numSnippets) < numFiles);

	if (isCallback)
	{
		auto pe = new PopupIncludeEditor(p, p->getSnippet(index)->getCallbackName());

#if HISE_USE_NEW_CODE_EDITOR
		if(scaleFactor != -1.0f)
			pe->getEditor()->editor.setScaleFactor(scaleFactor);
#endif

		pe->addMouseListener(this, true);
		getProcessor()->getMainController()->setLastActiveEditor(pe->getEditor(), CodeDocument::Position());
		pe->grabKeyboardFocusAsync();
		return pe;
	}
	else if (isExternalFile)
	{
        File f;
        
        auto scriptFolder = f = GET_PROJECT_HANDLER(dynamic_cast<Processor*>(p)).getSubDirectory(FileHandlerBase::Scripts);
        
        if(scriptPath.isNotEmpty())
        {
            f = scriptFolder.getChildFile(scriptPath);
            refreshSelectorValue(getProcessor(), f.getFileName());
        }
        else
        {
            const int fileIndex = index - p->getNumSnippets();
            f = p->getWatchedFile(fileIndex);
            scriptPath = f.getRelativePathFrom(scriptFolder);
        }
        
		if (f.getFileExtension() == ".h")
		{
			snex::ui::WorkbenchData* wb = new snex::ui::WorkbenchData();
			wb->setUseFileAsContentSource(f);
			return new snex::SnexPlayground(wb);
		}

		auto pe = new PopupIncludeEditor(p, f);
		pe->addMouseListener(this, true);

#if HISE_USE_NEW_CODE_EDITOR
        if(scaleFactor != -1.0f)
            pe->getEditor()->editor.setScaleFactor(scaleFactor);
#endif
        
		if(auto ed = pe->getEditor())
			getProcessor()->getMainController()->setLastActiveEditor(pe->getEditor(), CodeDocument::Position());

		pe->grabKeyboardFocusAsync();

		return pe;
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


void CodeEditorPanel::contentChanged()
{
	refreshIndexList();

	StringArray indexList;
	fillIndexList(indexList);
	auto titleToShow = indexList[getCurrentIndex()];

	setCustomTitle(titleToShow);

	if (titleToShow.isNotEmpty())
		setDynamicTitle(titleToShow);
}

void CodeEditorPanel::fromDynamicObject(const var& object)
{
    scriptPath = object.getProperty("ScriptFile", "").toString();
	PanelWithProcessorConnection::fromDynamicObject(object);
}

var CodeEditorPanel::toDynamicObject() const
{
	var obj = PanelWithProcessorConnection::toDynamicObject();
    
    obj.getDynamicObject()->setProperty("ScriptFile", scriptPath);
    
    return obj;
}

CodeEditorPanel* CodeEditorPanel::showOrCreateTab(FloatingTabComponent* parentTab, JavascriptProcessor* jp, int index)
{
	for (int tabIndex = 0; tabIndex < parentTab->getNumTabs(); tabIndex++)
	{
		if (auto tc = dynamic_cast<FloatingTile*>(parentTab->getTabContentComponent(tabIndex)))
		{
			if (auto cep = dynamic_cast<CodeEditorPanel*>(tc->getCurrentFloatingPanel()))
			{
				auto processorMatches = cep->getConnectedProcessor() == dynamic_cast<Processor*>(jp);
				auto indexMatches = cep->getCurrentIndex() == index;

				if (processorMatches && indexMatches)
				{
					parentTab->setCurrentTabIndex(tabIndex);

					return cep;
				}
			}
		}
	}

	FloatingInterfaceBuilder ib(parentTab->getParentShell());

	auto newEditor = ib.addChild<CodeEditorPanel>(0);

	auto ed = ib.getContent<CodeEditorPanel>(newEditor);
	
	ib.finalizeAndReturnRoot();

	ed->setContentWithUndo(dynamic_cast<Processor*>(jp), index);
	parentTab->setCurrentTabIndex(parentTab->getNumTabs() - 1);
	return ed;
}

void CodeEditorPanel::scriptWasCompiled(JavascriptProcessor *processor)
{
	if (getProcessor() == dynamic_cast<Processor*>(processor))
		refreshIndexList();
}

void CodeEditorPanel::mouseDown(const MouseEvent& e)
{
	if (e.mods.isX1ButtonDown())
		getMainController()->getLocationUndoManager()->undo();
	else if (e.mods.isX2ButtonDown())
		getMainController()->getLocationUndoManager()->redo();
}

var CodeEditorPanel::getAdditionalUndoInformation() const
{
	auto pe = getContent<PopupIncludeEditor>();

#if HISE_USE_NEW_CODE_EDITOR
	if (pe != nullptr && pe->getEditor() != nullptr)
	{
		return var(pe->getEditor()->editor.getFirstLineOnScreen());
	}
#endif

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
			//pe->getEditor()->scrollToLine(lineIndex);
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
			if (auto network = h->getActiveOrDebuggedNetwork())
			{
				network->fillSnexObjects(indexList);
			}
#endif
		}
	}
}

void CodeEditorPanel::gotoLocation(Processor* p, const String& fileName, int charNumber)
{
	if (fileName.isEmpty() || fileName == "onInit")
	{
		changeContentWithUndo(0);
	}
	else if (fileName.contains("()"))
	{
		auto jp = dynamic_cast<JavascriptProcessor*>(p);
		auto snippetId = Identifier(fileName.upToFirstOccurrenceOf("()", false, false));

		for (int i = 0; i < jp->getNumSnippets(); i++)
		{
			if (jp->getSnippet(i)->getCallbackName() == snippetId)
			{
				changeContentWithUndo(i);
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
			auto f = jp->getWatchedFile(i);

			if (f.getFullPathName() == fileName ||
				f.getFileName() == fileName)
			{
				fileIndex = i;
				break;
			}
		}

		if (fileIndex != -1)
		{
			changeContentWithUndo(jp->getNumSnippets() + fileIndex);
		}
		else
			return;
	}

	if (auto c = getContent<PopupIncludeEditor>())
	{
		PopupIncludeEditor::EditorType* editor = c->getEditor();

		CodeDocument::Position pos(editor->editor.getDocument(), charNumber);

		editor->editor.scrollToLine(pos.getLineNumber(), true);
		mcl::Selection newSelection(pos.getLineNumber(), pos.getIndexInLine(), pos.getLineNumber(), pos.getIndexInLine());
		editor->editor.getTextDocument().setSelection(0, newSelection, false);
	}
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
	}
}

Identifier ScriptContentPanel::getProcessorTypeId() const
{
	return JavascriptProcessor::getConnectorId();
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
		auto sp = dynamic_cast<ProcessorWithScriptingContent*>(p);

		addAndMakeVisible(content = new ScriptContentComponent(sp));
		addAndMakeVisible(overlay = new ScriptingContentOverlay(this));

		auto shouldEdit = sp->getScriptingContent()->getNumComponents() == 0;
		shouldEdit &= !ScriptEditHandler::editModeEnabled();

		if (shouldEdit)
			toggleComponentSelectMode(true);
		
		overlay->setEditMode(shouldEdit);
		overlay->setShowEditButton(false);
	}


#if USE_BACKEND
	void mouseDown(const MouseEvent& e) override
	{
		if (e.mods.isMiddleButtonDown())
		{
			if (auto zp = findParentComponentOfClass<ZoomableViewport>())
			{
				setMouseCursor(MouseCursor::DraggingHandCursor);
				auto ze = e.getEventRelativeTo(zp);
				zp->mouseDown(ze);
				return;
			}
		}
	}

	void mouseUp(const MouseEvent& e) override
	{
		if (e.mods.isMiddleButtonDown())
		{
			if (auto zp = findParentComponentOfClass<ZoomableViewport>())
			{
				setMouseCursor(MouseCursor::NormalCursor);
				auto ze = e.getEventRelativeTo(zp);
				zp->mouseUp(ze);
				return;
			}
		}
	}

	void mouseDrag(const MouseEvent& e) override
	{
		if (e.mods.isMiddleButtonDown())
		{
			if (auto zp = findParentComponentOfClass<ZoomableViewport>())
			{
				auto ze = e.getEventRelativeTo(zp);
				zp->mouseDrag(ze);
				return;
			}
		}
	}
#endif

	void setEnablePositioningWithMouse(bool shouldBeEnabled)
	{
		overlay->setEnablePositioningWithMouse(shouldBeEnabled);
	}
	
	bool isMousePositioningEnabled() const
	{
		return overlay->isMousePositioningEnabled();
	}

	void paint(Graphics& g) override;

	void selectOnInitCallback() override
	{

	}

	static void centreInViewport(Component* c)
	{
		if (auto vp = c->findParentComponentOfClass<ZoomableViewport>())
		{
			auto cBounds = c->getLocalBounds();

			cBounds = cBounds.withSizeKeepingCentre(vp->getWidth() / vp->zoomFactor, vp->getHeight() / vp->zoomFactor);

			// do not center a zoomed in viewport...
			if (cBounds.getX() < 0 || cBounds.getY() < 0)
				vp->zoomToRectangle(cBounds);

		}
	}

	void scriptEditHandlerCompileCallback()
	{
		if (getScriptEditHandlerProcessor())
			getScriptEditHandlerProcessor()->compileScript();
	}

public:

	virtual ScriptContentComponent* getScriptEditHandlerContent() { return content; }

	virtual ScriptingContentOverlay* getScriptEditHandlerOverlay() { return overlay; }

	CommonEditorFunctions::EditorType* getScriptEditHandlerEditor() override
	{
		if (processor.get())
		{
			return dynamic_cast<CommonEditorFunctions::EditorType*>(processor->getMainController()->getLastActiveEditor());
		}

		return nullptr;
	}

	virtual JavascriptProcessor* getScriptEditHandlerProcessor() { return dynamic_cast<JavascriptProcessor*>(processor.get()); }

	void refreshContent()
	{
		setSize(content->getContentWidth() + 10, content->getContentHeight() + 10);
	}

	void resized() override
	{
		auto b = getLocalBounds().reduced(5);

		content->setBounds(b);
		overlay->setBounds(b);
	}

private:

	float zoomLevel = 1.0f;

	friend class Editor;

	ScopedPointer<ScriptContentComponent> content;
	ScopedPointer<ScriptingContentOverlay> overlay;
	WeakReference<Processor> processor;

};

void ScriptContentPanel::Canvas::paint(Graphics& g)
{
	g.setColour(JUCE_LIVE_CONSTANT_OFF(Colour(0xFF1b1b1b)));

	auto overlayBounds = FLOAT_RECTANGLE(getLocalArea(overlay, overlay->getLocalBounds()));

	g.fillRect(overlayBounds);

	g.setColour(Colours::white.withAlpha(0.5f));

	UnblurryGraphics ug(g, *this, true);

	

	ug.draw1PxHorizontalLine((int)overlayBounds.getY(), overlayBounds.getX() - 5.0f, overlayBounds.getX());
	ug.draw1PxHorizontalLine((int)overlayBounds.getY(), overlayBounds.getRight(), overlayBounds.getRight() + 5);

	ug.draw1PxHorizontalLine((int)overlayBounds.getBottom(), overlayBounds.getX() - 5.0f, overlayBounds.getX());
	ug.draw1PxHorizontalLine((int)overlayBounds.getBottom(), overlayBounds.getRight(), overlayBounds.getRight() + 5);

	ug.draw1PxVerticalLine((int)overlayBounds.getX(), overlayBounds.getY() - 5.0f, overlayBounds.getY());
	ug.draw1PxVerticalLine((int)overlayBounds.getX(), overlayBounds.getBottom(), overlayBounds.getBottom() + 5);

	ug.draw1PxVerticalLine((int)overlayBounds.getRight(), overlayBounds.getY() - 5.0f, overlayBounds.getY());
	ug.draw1PxVerticalLine((int)overlayBounds.getRight(), overlayBounds.getBottom(), overlayBounds.getBottom() + 5.0f);
}




Component* ScriptContentPanel::createContentComponent(int /*index*/)
{
	auto c = new Canvas(getConnectedProcessor());

	return new Editor(c);
}


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


ScriptContentPanel::Editor::Editor(Canvas* c):
	WrapperWithMenuBarBase(c),
	ScriptComponentEditListener(dynamic_cast<Processor*>(c->getScriptEditHandlerProcessor()))
{
	zoomSelector = new ComboBox("Zoom");
	zoomSelector->addListener(this);
	zoomSelector->addItemList({ "50%", "75%", "100%", "125%", "150%", "200%" }, 1);
	zoomSelector->setSelectedId(3, dontSendNotification);
	zoomSelector->setLookAndFeel(&klaf);
	zoomSelector->setSize(100, 24);
	klaf.setDefaultColours(*zoomSelector);

	auto shouldDrag = !canvas.getContent<Canvas>()->overlay->isEditModeEnabled();

	canvas.setScrollOnDragEnabled(shouldDrag);
	canvas.setMouseWheelScrollEnabled(!shouldDrag);
	//canvas.setColour(ZoomableViewport::ColourIds::backgroundColourId, Colour(0xff262626));

	rebuildAfterContentChange();

	canvas.setMaxZoomFactor(4.0);

	setPostResizeFunction(Canvas::centreInViewport);
}


void ScriptContentPanel::Editor::rebuildAfterContentChange()
{
	addButton("showall");

	addCustomComponent(zoomSelector);

	addButton("edit");
	addButton("learn");
	addButton("cancel");
	addButton("rebuild");

	addSpacer(10);

	addButton("undo");
	addButton("redo");

	addSpacer(10);

	addButton("lock");
	addButton("move");

	addSpacer(10);

	addButton("vertical-align");
	addButton("horizontal-align");
	addButton("vertical-distribute");
	addButton("horizontal-distribute");

	setWantsKeyboardFocus(true);
	updateUndoDescription();
	refreshContent();
}

void ScriptContentPanel::Editor::addButton(const String& name)
{
	using ActionButton = WrapperWithMenuBarBase::ActionButtonBase<Editor, Factory>;
	auto b = new ActionButton(this, name);

	if (name == "edit")
	{
		b->actionFunction = Actions::toggleEditMode;
		b->stateFunction = [](Editor& e) { return e.isEditModeEnabled(); };
		b->setTooltip("Toggle Edit / Presentation Mode (F4)");
	}
	if(name == "cancel")
	{
		b->enabledFunction = isSelected;
		b->actionFunction = Actions::deselectAll;
		b->setTooltip("Deselect current item (Escape)");
	}
	if (name == "showall")
	{
		b->actionFunction = [](Editor& e)
		{
			e.canvas.zoomToRectangle(e.canvas.getContentComponent()->getLocalBounds().expanded(20));
			return false;
		};

		b->setTooltip("Zoom to fit");
	}
	if (name == "move")
	{
		b->actionFunction = Actions::move;
		b->setTooltip("Allow dragging components by mouse");
		b->stateFunction = [](Editor& e)
		{
			return e.canvas.getContent<Canvas>()->isMousePositioningEnabled();
		};
	}
	if (name == "rebuild")
	{
		b->actionFunction = Actions::rebuild;
		b->setTooltip("Rebuild Interface (F5)");
	}
	if (name == "learn")
	{
		b->actionFunction = [](Editor& e)
		{
			auto bc = e.getScriptComponentEditBroadcaster();
			bc->setLearnMode(!bc->learnModeEnabled());
			return true;
		};
		b->stateFunction = [](Editor& e)
		{
			return e.getScriptComponentEditBroadcaster()->learnModeEnabled();
		};

		b->enabledFunction = isSingleSelection;
	}
	if (name == "lock")
	{
		b->actionFunction = Actions::lockSelection;
		b->stateFunction = [](Editor& e)
		{
			if (auto f = e.getScriptComponentEditBroadcaster()->getFirstFromSelection())
			{
				return f->isLocked();
			}

			return false;
		};
		b->enabledFunction = isSelected;
		b->setTooltip("Lock the selected components");
	}
	if (name == "undo")
	{
		b->actionFunction = [](Editor& e) { return Actions::undo(&e, true);};
		b->setTooltip("Undo last item change");
	}
	if (name == "redo")
	{
		b->actionFunction = [](Editor& e) { return Actions::undo(&e, false); };
		b->setTooltip("Redo last item change");
	}
	if (name == "vertical-align")
	{
		b->actionFunction = [](Editor& e) { return Actions::align(&e, true); };
		b->setTooltip("Align the selection vertically on the left edge");
		b->enabledFunction = isSelected;
	}
	if(name == "horizontal-align")
	{
		b->actionFunction = [](Editor& e) { return Actions::align(&e, false); };
		b->setTooltip("Align the selection horizontally on the top edge");
		b->enabledFunction = isSelected;
	}
	if (name == "vertical-distribute")
	{
		b->actionFunction = [](Editor& e) { return Actions::distribute(&e, true); };
		b->setTooltip("Distribute the selection vertically with equal space");
		b->enabledFunction = isSelected;
	}
	if (name == "horizontal-distribute")
	{
		b->actionFunction = [](Editor& e) { return Actions::distribute(&e, false); };
		b->setTooltip("Distribute the selection horizontally with equal space");
		b->enabledFunction = isSelected;
	}

	addAndMakeVisible(b);
	actionButtons.add(b);
}

void ScriptContentPanel::Editor::scriptComponentSelectionChanged()
{
	updateUndoDescription();
}


void ScriptContentPanel::Editor::scriptComponentPropertyChanged(ScriptComponent* /*sc*/, Identifier /*idThatWasChanged*/, const var& /*newValue*/)
{
	updateUndoDescription();
}

void ScriptContentPanel::Editor::zoomChanged(float newScalingFactor)
{
	String s;
	s << roundToInt(newScalingFactor * 100) << "%";

	zoomSelector->setText(s, dontSendNotification);
}

#if 0
void ScriptContentPanel::Editor::resized()
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
}
#endif

void ScriptContentPanel::Editor::refreshContent()
{
	canvas.getContent<Canvas>()->refreshContent();

	auto newBounds = canvas.getContent<Canvas>()->getBounds();



	if (lastBounds.getWidth() != newBounds.getWidth() ||
		lastBounds.getHeight() != newBounds.getHeight())
	{
		canvas.centerCanvas();
	}

	lastBounds = newBounds;
}

void ScriptContentPanel::Editor::buttonClicked(Button* b)
{
}


void ScriptContentPanel::Editor::updateUndoDescription()
{
	
}

void ScriptContentPanel::Editor::comboBoxChanged(ComboBox* c)
{
	switch (c->getSelectedId())
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
	auto c = canvas.getContent<Canvas>()->getLocalBounds().toFloat();

	canvas.setZoomFactor(newZoomAmount, c.getCentre());
}


double ScriptContentPanel::Editor::getZoomAmount() const
{
	return canvas.zoomFactor;
}

bool ScriptContentPanel::Editor::isEditModeEnabled() const
{
	return canvas.getContent<Canvas>()->overlay->isEditModeEnabled();
}


bool ScriptContentPanel::Editor::Actions::deselectAll(Editor& e)
{
	e.getScriptComponentEditBroadcaster()->clearSelection();
	return true;
}

bool ScriptContentPanel::Editor::Actions::rebuild(Editor& e_)
{
	auto e = &e_;
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

	return true;
}

bool ScriptContentPanel::Editor::Actions::rebuildAndRecompile(Editor& e_)
{
	auto e = &e_;

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

	return true;
}

bool ScriptContentPanel::Editor::Actions::zoomIn(Editor& e)
{
	return e.canvas.changeZoom(true);
}

bool ScriptContentPanel::Editor::Actions::zoomOut(Editor& e)
{
	return e.canvas.changeZoom(false);
}

bool ScriptContentPanel::Editor::Actions::toggleEditMode(Editor& e)
{
	e.canvas.getContent<Canvas>()->overlay->toggleEditMode();

	auto shouldDrag = !e.canvas.getContent<Canvas>()->overlay->isEditModeEnabled();

	e.canvas.setScrollOnDragEnabled(shouldDrag);
	e.canvas.setMouseWheelScrollEnabled(true);// !shouldDrag);

	return true;
}

bool ScriptContentPanel::Editor::Actions::lockSelection(Editor& e)
{
	auto sl = e.getScriptComponentEditBroadcaster();

	bool wasLocked = false;

	if (auto first = sl->getFirstFromSelection())
		wasLocked = first->isLocked();

	sl->setScriptComponentPropertyForSelection("locked", !wasLocked, sendNotification);

	return true;
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

bool ScriptContentPanel::Editor::Actions::distribute(Editor* editor, bool isVertical)
{
	auto b = editor->getScriptComponentEditBroadcaster();

	auto selection = b->getSelection();

	if (selection.size() < 3)
		return false;

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

	return selection.isEmpty();
}

bool ScriptContentPanel::Editor::Actions::align(Editor* editor, bool isVertical)
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

	return selection.isEmpty();
}

bool ScriptContentPanel::Editor::Actions::undo(Editor* e, bool shouldUndo)
{
	e->getScriptComponentEditBroadcaster()->undo(shouldUndo);
	rebuild(*e);

	return true;
}

bool ScriptContentPanel::Editor::Actions::move(Editor& e)
{
	auto c = e.canvas.getContent<Canvas>();

	jassert(c != nullptr);

	c->setEnablePositioningWithMouse(!c->isMousePositioningEnabled());

	return true;
}

void ScriptContentPanel::initKeyPresses(Component* root)
{
	using namespace InterfaceDesignerShortcuts;

	String cat = "Interface Designer";

	TopLevelWindowWithKeyMappings::addShortcut(root, cat, id_deselect_all, "Deselect all", KeyPress(KeyPress::escapeKey));

	TopLevelWindowWithKeyMappings::addShortcut(root, cat, id_toggle_edit, "Toggle Edit mode", KeyPress(KeyPress::F4Key));

	TopLevelWindowWithKeyMappings::addShortcut(root, cat, id_rebuild, "Rebuild & Recompile", KeyPress(KeyPress::F5Key));

	TopLevelWindowWithKeyMappings::addShortcut(root, cat, id_lock_selection, "Lock selected components", KeyPress('l', ModifierKeys::commandModifier, 'l'));

	TopLevelWindowWithKeyMappings::addShortcut(root, cat, id_duplicate, "Duplicate selection at cursor", KeyPress('d', ModifierKeys::commandModifier, 'd'));

	TopLevelWindowWithKeyMappings::addShortcut(root, cat, id_show_json, "Show JSON properties", KeyPress('j'));
    TopLevelWindowWithKeyMappings::addShortcut(root, cat, id_show_panel_data_json, "Show Panel.data as JSON", KeyPress('p'));
}

bool ScriptContentPanel::Editor::keyPressed(const KeyPress& key)
{
	using namespace InterfaceDesignerShortcuts;

	if (TopLevelWindowWithKeyMappings::matches(this, key, id_toggle_edit))
		return Actions::toggleEditMode(*this);
	if (TopLevelWindowWithKeyMappings::matches(this, key, id_deselect_all))
		return Actions::deselectAll(*this);
	else if (TopLevelWindowWithKeyMappings::matches(this, key, id_rebuild))
		return Actions::rebuildAndRecompile(*this);
	else if (key.getKeyCode() == '+' && key.getModifiers().isCommandDown())
		return Actions::zoomIn(*this);
	else if (key.getKeyCode() == '-' && key.getModifiers().isCommandDown())
		return Actions::zoomOut(*this);
	else if (TopLevelWindowWithKeyMappings::matches(this, key, id_lock_selection))
		return Actions::lockSelection(*this);

	return false;
}

bool ScriptContentPanel::Editor::isSelected(Editor& e)
{
	return !e.canvas.getContent<Canvas>()->overlay->draggers.isEmpty();
}

bool ScriptContentPanel::Editor::isSingleSelection(Editor& e)
{
	return e.canvas.getContent<Canvas>()->overlay->draggers.size() == 1;
}

juce::Identifier ServerControllerPanel::getProcessorTypeId() const
{
	return JavascriptProcessor::getConnectorId();
}

namespace ServerIcons
{
static const unsigned char response[] = { 110,109,25,132,45,66,51,179,228,66,108,25,132,45,66,199,139,14,67,108,0,0,0,0,4,86,198,66,108,25,132,45,66,240,39,95,66,108,25,132,45,66,82,248,167,66,108,188,52,235,66,82,248,167,66,108,188,52,235,66,51,179,228,66,108,25,132,45,66,51,179,228,66,99,109,
229,208,89,66,78,98,240,64,98,223,207,89,66,190,159,238,64,229,208,89,66,47,221,236,64,236,209,89,66,111,18,235,64,98,246,40,90,66,143,194,117,64,74,140,101,66,199,75,23,63,197,160,115,66,53,94,186,61,98,252,169,118,66,150,67,139,188,100,59,120,66,10,
215,35,188,66,96,121,66,111,18,131,60,108,70,150,65,67,111,18,131,60,98,68,203,65,67,205,204,204,60,66,0,66,67,2,43,7,61,254,52,66,67,49,8,44,61,98,213,248,69,67,229,208,2,63,164,240,72,67,106,188,108,64,23,25,73,67,47,221,240,64,108,23,25,73,67,254,
148,213,66,98,217,238,72,67,147,152,221,66,90,164,69,67,156,68,228,66,70,150,65,67,35,155,228,66,108,92,15,255,66,35,155,228,66,108,92,15,255,66,92,143,198,66,108,117,19,58,67,92,143,198,66,108,117,19,58,67,205,204,182,65,108,164,176,254,66,102,102,141,
66,108,51,243,138,66,244,253,186,65,108,51,243,138,66,127,234,147,66,108,223,207,89,66,127,234,147,66,108,223,207,89,66,47,221,240,64,108,229,208,89,66,78,98,240,64,99,109,72,225,43,67,190,159,112,65,108,168,6,166,66,190,159,112,65,108,219,185,254,66,
100,59,77,66,108,72,225,43,67,190,159,112,65,99,101,0,0 };

static const unsigned char downloads[] = { 110,109,25,4,114,66,63,53,246,65,108,143,194,59,66,63,53,246,65,108,59,223,97,65,41,156,191,66,108,117,51,24,67,41,156,191,66,108,137,193,238,66,63,53,246,65,108,12,66,210,66,63,53,246,65,108,12,66,210,66,113,61,177,65,108,53,94,249,66,113,61,177,65,
108,150,163,36,67,72,161,202,66,98,49,168,37,67,104,81,203,66,90,100,38,67,250,62,205,66,90,100,38,67,25,132,207,66,108,90,100,38,67,100,123,254,66,98,90,100,38,67,252,169,0,67,172,60,37,67,170,209,1,67,98,208,35,67,170,209,1,67,108,145,237,36,64,170,
209,1,67,98,10,215,147,63,170,209,1,67,0,0,0,0,252,169,0,67,0,0,0,0,100,123,254,66,108,0,0,0,0,25,132,207,66,98,0,0,0,0,98,80,205,66,96,229,48,63,27,111,203,66,47,221,212,63,45,178,202,66,108,55,137,38,66,113,61,177,65,108,25,4,114,66,113,61,177,65,108,
25,4,114,66,63,53,246,65,99,109,16,24,198,66,29,90,44,66,108,119,254,233,66,29,90,44,66,108,143,66,166,66,225,58,174,66,108,80,13,69,66,29,90,44,66,108,14,109,134,66,29,90,44,66,108,14,109,134,66,0,0,0,0,108,16,24,198,66,0,0,0,0,108,16,24,198,66,29,90,
44,66,99,101,0,0 };

static const unsigned char requests[] = { 110,109,51,243,249,66,137,1,211,66,108,51,243,249,66,55,73,158,66,108,197,160,22,66,55,73,158,66,108,197,160,22,66,2,171,90,66,108,0,0,0,0,96,165,184,66,108,197,160,22,66,225,250,1,67,108,197,160,22,66,137,1,211,66,108,51,243,249,66,137,1,211,66,99,109,
193,202,105,66,16,88,75,66,108,193,202,105,66,229,208,195,65,108,106,188,12,67,229,208,195,65,108,106,188,12,67,0,0,0,0,108,90,100,50,67,197,160,22,66,108,106,188,12,67,66,160,150,66,108,106,188,12,67,16,88,75,66,108,193,202,105,66,16,88,75,66,99,101,
0,0 };

static const unsigned char pause[] = { 110,109,231,59,6,67,115,104,145,61,98,41,28,29,67,207,247,147,62,135,86,50,67,133,235,159,65,236,145,50,67,197,160,49,66,98,61,170,50,67,176,178,148,66,61,170,50,67,129,149,208,66,236,145,50,67,231,59,6,67,98,29,90,50,67,41,28,29,67,168,166,30,67,135,
86,50,67,231,59,6,67,236,145,50,67,98,129,149,208,66,61,170,50,67,176,178,148,66,61,170,50,67,197,160,49,66,236,145,50,67,98,125,63,172,65,29,90,50,67,209,34,155,62,168,166,30,67,115,104,145,61,231,59,6,67,98,166,155,196,188,129,149,208,66,166,155,196,
188,176,178,148,66,115,104,145,61,197,160,49,66,98,207,247,147,62,125,63,172,65,133,235,159,65,209,34,155,62,197,160,49,66,115,104,145,61,98,176,178,148,66,166,155,196,188,129,149,208,66,166,155,196,188,231,59,6,67,115,104,145,61,99,109,96,229,50,66,
43,135,144,65,98,137,65,247,65,72,225,144,65,215,163,145,65,121,233,238,65,43,135,144,65,135,22,50,66,98,82,184,143,65,168,198,148,66,82,184,143,65,137,129,208,66,43,135,144,65,119,30,6,67,98,104,145,145,65,178,221,19,67,154,153,240,65,33,112,32,67,129,
21,50,66,51,147,32,67,98,37,198,148,66,8,172,32,67,12,130,208,66,8,172,32,67,184,30,6,67,51,147,32,67,98,84,227,19,67,236,113,32,67,92,111,32,67,14,141,20,67,51,147,32,67,250,30,6,67,98,55,169,32,67,12,130,208,66,55,169,32,67,37,198,148,66,51,147,32,
67,123,20,50,66,98,229,112,32,67,55,137,245,65,61,138,20,67,190,159,145,65,184,30,6,67,43,135,144,65,98,90,164,208,66,156,196,143,65,199,11,149,66,43,135,144,65,96,229,50,66,43,135,144,65,99,109,252,105,162,66,213,152,3,67,108,137,65,77,66,213,152,3,
67,108,137,65,77,66,14,45,60,66,108,252,105,162,66,14,45,60,66,108,252,105,162,66,213,152,3,67,99,109,109,167,254,66,213,152,3,67,108,53,222,194,66,213,152,3,67,108,53,222,194,66,14,45,60,66,108,109,167,254,66,14,45,60,66,108,109,167,254,66,213,152,3,
67,99,101,0,0 };

static const unsigned char play[] = { 110,109,231,59,6,67,115,104,145,61,98,41,28,29,67,207,247,147,62,135,86,50,67,133,235,159,65,236,145,50,67,197,160,49,66,98,61,170,50,67,176,178,148,66,61,170,50,67,129,149,208,66,236,145,50,67,231,59,6,67,98,29,90,50,67,41,28,29,67,168,166,30,67,135,
86,50,67,231,59,6,67,236,145,50,67,98,129,149,208,66,61,170,50,67,176,178,148,66,61,170,50,67,197,160,49,66,236,145,50,67,98,125,63,172,65,29,90,50,67,209,34,155,62,168,166,30,67,115,104,145,61,231,59,6,67,98,166,155,196,188,129,149,208,66,166,155,196,
188,176,178,148,66,115,104,145,61,197,160,49,66,98,207,247,147,62,125,63,172,65,133,235,159,65,209,34,155,62,197,160,49,66,115,104,145,61,98,176,178,148,66,166,155,196,188,129,149,208,66,166,155,196,188,231,59,6,67,115,104,145,61,99,109,96,229,50,66,
43,135,144,65,98,39,49,247,65,72,225,144,65,203,161,145,65,68,139,239,65,43,135,144,65,123,20,50,66,98,180,200,143,65,37,198,148,66,180,200,143,65,12,130,208,66,43,135,144,65,250,30,6,67,98,117,147,145,65,121,233,19,67,154,153,240,65,33,112,32,67,129,
21,50,66,51,147,32,67,98,37,198,148,66,8,172,32,67,12,130,208,66,8,172,32,67,184,30,6,67,51,147,32,67,98,178,253,19,67,170,113,32,67,223,111,32,67,184,94,20,67,51,147,32,67,119,30,6,67,98,68,171,32,67,137,129,208,66,68,171,32,67,168,198,148,66,51,147,
32,67,135,22,50,66,98,39,113,32,67,76,55,246,65,33,144,20,67,203,161,145,65,184,30,6,67,43,135,144,65,98,90,164,208,66,156,196,143,65,199,11,149,66,43,135,144,65,96,229,50,66,43,135,144,65,99,109,244,221,7,67,90,164,178,66,108,8,44,101,66,147,184,8,67,
108,8,44,101,66,20,174,39,66,108,244,221,7,67,90,164,178,66,99,101,0,0 };

static const unsigned char resend[] = { 110,109,63,53,160,65,6,129,27,65,98,82,184,148,65,78,98,240,64,16,88,128,65,180,200,194,64,86,14,85,65,248,83,195,64,98,82,184,234,64,170,241,198,64,2,43,63,64,84,227,107,65,84,227,253,64,141,151,156,65,98,104,145,55,65,113,61,184,65,205,204,148,65,246,
40,170,65,72,225,165,65,223,79,134,65,108,78,98,216,65,223,79,134,65,98,193,202,214,65,244,253,142,65,49,8,212,65,203,161,151,65,207,247,207,65,98,16,160,65,98,119,190,177,65,119,190,222,65,61,10,37,65,100,59,242,65,92,143,114,64,188,116,192,65,98,217,
206,119,192,12,2,134,65,217,206,247,62,217,206,119,62,242,210,83,65,111,18,131,58,98,57,180,86,65,0,0,0,0,33,176,86,65,0,0,0,0,104,145,89,65,111,18,131,58,98,147,24,142,65,10,215,163,61,240,167,172,65,182,243,21,64,145,237,192,65,197,32,180,64,108,250,
126,233,65,23,217,14,63,108,45,178,234,65,131,192,102,65,108,0,0,172,65,90,100,101,65,108,168,198,170,65,90,100,101,65,108,168,198,170,65,66,96,101,65,108,104,145,119,65,29,90,100,65,108,63,53,160,65,6,129,27,65,99,101,0,0 };

static const unsigned char parameters[] = { 110,109,160,26,92,66,121,233,3,66,108,160,26,92,66,236,81,82,65,108,25,132,139,66,236,81,82,65,108,117,211,135,66,0,0,0,0,108,51,243,181,66,250,126,184,65,108,117,211,135,66,244,125,56,66,108,25,132,139,66,121,233,3,66,108,160,26,92,66,121,233,3,66,99,
109,205,204,141,65,141,23,36,66,108,76,55,249,64,141,23,36,66,108,76,55,249,64,55,9,1,66,108,205,204,141,65,55,9,1,66,108,205,204,141,65,141,23,36,66,99,109,162,197,71,66,137,65,4,66,108,16,88,3,66,137,65,4,66,108,16,88,3,66,152,110,201,65,108,162,197,
71,66,152,110,201,65,108,162,197,71,66,137,65,4,66,99,109,98,16,139,65,16,88,238,65,108,20,174,1,65,16,88,238,65,108,20,174,1,65,133,235,230,65,98,20,174,1,65,186,73,218,65,92,143,4,65,61,10,208,65,186,73,10,65,246,40,200,65,98,25,4,16,65,174,71,192,
65,68,139,24,65,135,22,185,65,59,223,35,65,117,147,178,65,98,51,51,47,65,98,16,172,65,240,167,72,65,166,155,160,65,113,61,112,65,51,51,144,65,98,252,169,130,65,166,155,135,65,158,239,135,65,188,116,127,65,158,239,135,65,209,34,113,65,98,158,239,135,65,
229,208,98,65,242,210,133,65,20,174,87,65,141,151,129,65,119,190,79,65,98,82,184,122,65,193,202,71,65,84,227,109,65,242,210,67,65,57,180,92,65,242,210,67,65,98,63,53,74,65,242,210,67,65,121,233,58,65,182,243,73,65,205,204,46,65,39,49,86,65,98,33,176,
34,65,152,110,98,65,170,241,26,65,168,198,119,65,104,145,23,65,197,32,139,65,108,0,0,0,0,131,192,129,65,98,121,233,166,62,168,198,75,65,227,165,203,63,121,233,30,65,170,241,114,64,35,219,249,64,98,98,16,192,64,84,227,181,64,197,32,22,65,109,231,147,64,
61,10,95,65,109,231,147,64,98,121,233,139,65,109,231,147,64,242,210,162,65,178,157,171,64,162,69,180,65,12,2,219,64,98,207,247,203,65,55,137,13,65,242,210,215,65,113,61,56,65,242,210,215,65,203,161,109,65,98,242,210,215,65,72,225,129,65,143,194,212,65,
92,143,140,65,215,163,206,65,10,215,150,65,98,31,133,200,65,197,32,161,65,37,6,188,65,45,178,173,65,221,36,169,65,55,137,188,65,98,244,253,155,65,182,243,198,65,33,176,147,65,236,81,207,65,113,61,144,65,215,163,213,65,98,193,202,140,65,182,243,219,65,
98,16,139,65,27,47,228,65,98,16,139,65,16,88,238,65,99,109,162,197,71,66,229,208,168,65,108,16,88,3,66,229,208,168,65,108,16,88,3,66,8,172,82,65,108,162,197,71,66,8,172,82,65,108,162,197,71,66,229,208,168,65,99,101,0,0 };

}

struct ServerController: public Component,
						 public ControlledObject,
						 public PooledUIUpdater::SimpleTimer,
						 public GlobalServer::Listener,
						 public ButtonListener
{
	struct StateComponent : public Component
	{
		StateComponent(ServerController& p) :
			parent(p)
		{};

		void refresh()
		{
			if (auto s = parent.getServerClass())
			{
				auto thisState = s->getServerState();

				if (thisState != currentState)
				{
					currentState = thisState;
					repaint();
				}
			}
		}

		void paint(Graphics& g) override
		{
			Colour stateColours[(int)GlobalServer::State::numStates+1] = 
			{ 
				Colours::grey,				// Inactive
				Colours::yellow,			// Pause
				Colours::green,				// Idle
				Colours::blue,				// Pending
				Colours::transparentBlack	// uninitialised
			};

			auto c = stateColours[(int)currentState];

			auto circle = getLocalBounds().withSizeKeepingCentre(10, 10).toFloat();

			g.setColour(c);
			g.fillEllipse(circle);
			g.setColour(Colours::white.withAlpha(0.3f));
			g.drawEllipse(circle, 1.0f);
		}

		ServerController& parent;
		GlobalServer::State currentState = GlobalServer::State::numStates;
	};

	struct Factory : public PathFactory
	{
		String getId() const override { return {}; }
		Path createPath(const String& url) const override
		{
			Path p;

			LOAD_EPATH_IF_URL("showall", ScriptnodeIcons::zoomFit);
			LOAD_EPATH_IF_URL("clear", SampleMapIcons::deleteSamples);
			LOAD_PATH_IF_URL("edit", ServerIcons::parameters);
			LOAD_EPATH_IF_URL("web", MainToolbarIcons::web);
			LOAD_PATH_IF_URL("response", ServerIcons::response);
			LOAD_PATH_IF_URL("resend", ServerIcons::resend);
			LOAD_PATH_IF_URL("downloads", ServerIcons::downloads);
			LOAD_PATH_IF_URL("requests", ServerIcons::requests);
			LOAD_PATH_IF_URL("start", ServerIcons::play);
			LOAD_PATH_IF_URL("stop", ServerIcons::pause);
			LOAD_EPATH_IF_URL("file", SampleMapIcons::loadSampleMap);

			return p;
		}
	};

	struct DownloadModel : public TableListBoxModel,
					       public ButtonListener
	{
		using DataPtr = ScriptingObjects::ScriptDownloadObject::Ptr;

		enum class Columns
		{
			StatusLed = 1,
			Status,
			URL,
			DownloadSize,
			DownloadSpeed,
			Pause,
			Abort,
			ShowFile,
			numColumns
		};

		DownloadModel(ServerController& p) :
			parent(p)
		{};

		int getNumRows() override
		{
			auto l = parent.getServerClass()->getPendingDownloads();

			SimpleReadWriteLock::ScopedWriteLock sl(dataLock);

			downloads.clear();

			for (auto i : *l.getArray())
				downloads.add(dynamic_cast<ScriptingObjects::ScriptDownloadObject*>(i.getObject()));

			return downloads.size();
		}

		DataPtr getData(int index)
		{
			SimpleReadWriteLock::ScopedReadLock sl(dataLock);
			return downloads[index];
		}

		Component* refreshComponentForCell(int rowNumber, int columnId, bool isRowSelected,
			Component* existingComponentToUpdate) override
		{
			if (existingComponentToUpdate != nullptr)
				return existingComponentToUpdate;

			if (columnId == (int)Columns::Pause)
			{
				auto b = new HiseShapeButton("start", this, parent.f, "stop");
				b->setToggleModeWithColourChange(true);
				return b;
			}
			if (columnId == (int)Columns::Abort)
			{
				return new HiseShapeButton("clear", this, parent.f);
			}
			if (columnId == (int)Columns::ShowFile)
			{
				return new HiseShapeButton("file", this, parent.f);
			}

			return nullptr;
		}

		void buttonClicked(Button* b) override
		{
			auto n = b->getName();

			auto index = parent.downloadTable.getRowNumberOfComponent(b->getParentComponent()->getParentComponent());

			if (auto d = getData(index))
			{
				if (n == "start")
				{
					if (!b->getToggleState())
						d->resume();
					else
						d->stop();
				}

				if (n == "file")
					d->getTargetFile().revealToUser();

				if (n == "clear")
					d->abort();
			}
		}

		void paintRowBackground(Graphics& g, int rowNumber, int width, int height, bool rowIsSelected)
		{
			Rectangle<float> area(0.0f, 0.0f, (float)width, (float)height);

			if (rowIsSelected)
			{
				g.setColour(Colours::white.withAlpha(0.05f));
				g.fillRect(area);
			}
		}

		void paintCell(Graphics& g, int rowNumber, int columnId, int width, int height, bool rowIsSelected)
		{
			Rectangle<float> area(0.0f, 0.0f, (float)width, (float)height);

			if (auto obj = getData(rowNumber))
			{
				auto alpha = obj->isFinished ? 0.9f : 0.2f;

				switch ((Columns)columnId)
				{
				case Columns::URL:
				{
					String text = obj->getURL().toString(false);
					g.setFont(GLOBAL_MONOSPACE_FONT());
					g.setColour(Colours::white.withAlpha(alpha));
					g.drawText(text, area, Justification::centredLeft);
					break;
				}
				case Columns::Status:
				{
					auto text = obj->getStatusText();
					g.setFont(GLOBAL_BOLD_FONT());
					g.setColour(Colours::white.withAlpha(alpha));
					g.drawText(text, area, Justification::centredLeft);
					break;
				}
				case Columns::DownloadSize:
				{
					if (auto s = parent.getServerClass())
					{
						String text;
						text << String((double)obj->getNumBytesDownloaded() / 1024.0 / 1024.0, 1) << "MB";
						text << " / ";
						text << String((double)obj->getDownloadSize() / 1024.0 / 1024.0, 2) << "MB";
						g.setFont(GLOBAL_BOLD_FONT());
						g.setColour(Colours::white.withAlpha(alpha));
						g.drawText(text, area, Justification::centredLeft);
						break;
					}
				}
				case Columns::DownloadSpeed:
				{
					String text;
					text << String((double)obj->getDownloadSpeed() / 1024.0 / 1024.0, 1) << "MB/s";
					g.setFont(GLOBAL_BOLD_FONT());
					g.setColour(Colours::white.withAlpha(alpha));
					g.drawText(text, area, Justification::centredLeft);
					break;
				}
				case Columns::StatusLed:
				{
					g.setColour(Colours::green);
					auto circle = area.withSizeKeepingCentre(12.0f, 12.0f);
					g.fillEllipse(circle);
					g.setColour(Colours::white.withAlpha(0.4f));
					g.drawEllipse(circle, 1.0f);
					break;
				}
                default: break;
				}
			}
		}

		SimpleReadWriteLock dataLock;
		ReferenceCountedArray<ScriptingObjects::ScriptDownloadObject> downloads;
		ServerController& parent;
	};

	struct RequestModel : public TableListBoxModel
	{
		enum class Columns
		{
			StatusLed = 1,
			Status,
			URL,
			CreationTime,
			Duration,
			Parameters,
			Response,
			Resend,
			numColumns
		};

		RequestModel(ServerController& p):
			parent(p)
		{

		}

		Component* refreshComponentForCell(int rowNumber, int columnId, bool isRowSelected,
			Component* existingComponentToUpdate) override
		{
			if (existingComponentToUpdate != nullptr)
				return existingComponentToUpdate;

			if (columnId == (int)Columns::Response)
			{
				return new HiseShapeButton("response", &parent, parent.f);
			}
			if (columnId == (int)Columns::Parameters)
			{
				return new HiseShapeButton("edit", &parent, parent.f);
			}
			if (columnId == (int)Columns::Resend)
			{
				return new HiseShapeButton("resend", &parent, parent.f);
			}

			return nullptr;
		}

		void clearUnusedData()
		{
			for (int i = 0; i < objects.size(); i++)
			{
				if (auto d = getData(i))
				{
					if (!d->f)
					{
						objects.remove(i--);
						continue;
					}
				}
			}
		}

		void deleteKeyPressed(int lastRowSelected) override
		{
			auto selected = parent.requestTable.getSelectedRows();

			for (int i = selected.getNumRanges() - 1; i >= 0; i--)
			{
				auto r = selected.getRange(i);

				objects.removeRange(r.getStart(), r.getLength());
			}

			parent.requestTable.updateContent();
		}

		int getNumRows() override
		{
			return objects.size();
		}

		void paintRowBackground(Graphics& g, int rowNumber, int width, int height, bool rowIsSelected)
		{
			Rectangle<float> area(0.0f, 0.0f, (float)width, (float)height);

			if (rowIsSelected)
			{
				g.setColour(Colours::white.withAlpha(0.05f));
				g.fillRect(area);
			}
		}

		void paintCell(Graphics& g, int rowNumber, int columnId, int width, int height, bool rowIsSelected)
		{
			Rectangle<float> area(0.0f, 0.0f, (float)width, (float)height);

			if (auto obj = getData(rowNumber))
			{
				auto alpha = obj->f ? 0.9f : 0.2f;

				switch ((Columns)columnId)
				{
				case Columns::URL:
				{
					String text = obj->url.toString(false);
					g.setFont(GLOBAL_MONOSPACE_FONT());
					g.setColour(Colours::white.withAlpha(alpha));
					g.drawText(text, area, Justification::centredLeft);
					break;
				}
				case Columns::Status:
				{
					String text = String(obj->status);
					g.setFont(GLOBAL_BOLD_FONT());
					g.setColour(Colours::white.withAlpha(alpha));
					g.drawText(text, area, Justification::centredLeft);
					break;
				}
				case Columns::CreationTime:
				{
					if (auto s = parent.getServerClass())
					{
						String text;
						text << String(obj->requestTimeMs - s->startTime) << "ms";
						g.setFont(GLOBAL_BOLD_FONT());
						g.setColour(Colours::white.withAlpha(alpha));
						g.drawText(text, area, Justification::centredLeft);
						break;
					}
				}
				case Columns::Duration:
				{
					String text;
					text << String(obj->completionTimeMs - obj->requestTimeMs) << "ms";
					g.setFont(GLOBAL_BOLD_FONT());
					g.setColour(Colours::white.withAlpha(alpha));
					g.drawText(text, area, Justification::centredLeft);
					break;
				}
				case Columns::StatusLed:
				{
					g.setColour(getColourForState(obj.get()).withMultipliedSaturation(0.7f));
					auto circle = area.withSizeKeepingCentre(12.0f, 12.0f);
					g.fillEllipse(circle);
					g.setColour(Colours::white.withAlpha(0.4f));
					g.drawEllipse(circle, 1.0f);
					break;
				}
                default: break;
				}
			}
		}

		GlobalServer::PendingCallback::Ptr getData(int index)
		{
			SimpleReadWriteLock::ScopedReadLock sl(dataLock);
			return objects[index];
		}

		SimpleReadWriteLock dataLock;
		ServerController& parent;
		ReferenceCountedArray<GlobalServer::PendingCallback> objects;
	};

	ServerController(JavascriptProcessor* p) :
		ControlledObject(dynamic_cast<Processor*>(p)->getMainController()),
		SimpleTimer(getMainController()->getGlobalUIUpdater()),
		jp(p),
		state(*this),
		requestModel(*this),
		downloadModel(*this),
		downloadButton("downloads", this, f),
		requestButton("requests", this, f),
		pauseButton("stop", this, f, "start"),
		clearButton("clear", this, f)
	{
		addAndMakeVisible(requestTable);
		

		getMainController()->getJavascriptThreadPool().getGlobalServer()->addListener(this);

		addAndMakeVisible(state);
		addButton(requestButton);
		addButton(downloadButton);
		addButton(pauseButton);
		addButton(clearButton);

		requestButton.setToggleStateAndUpdateIcon(true);
		downloadButton.setToggleStateAndUpdateIcon(true);

		requestTable.getHeader().addColumn("StatusLED", (int)RequestModel::Columns::StatusLed, 30, 30, 30, TableHeaderComponent::notResizable);
		requestTable.getHeader().addColumn("Status", (int)RequestModel::Columns::Status, 50, 50, 50, TableHeaderComponent::notResizable);
		requestTable.getHeader().addColumn("URL", (int)RequestModel::Columns::URL, 200, 200, 9000);
		requestTable.getHeader().addColumn("Timestamp", (int)RequestModel::Columns::CreationTime, 120, 120, 120, TableHeaderComponent::notResizable);
		requestTable.getHeader().addColumn("Duration", (int)RequestModel::Columns::Duration, 70, 70, 70, TableHeaderComponent::notResizable);
		requestTable.getHeader().addColumn("Parameters", (int)RequestModel::Columns::Parameters, 60, 60, 60, TableHeaderComponent::notResizable);
		requestTable.getHeader().addColumn("Response", (int)RequestModel::Columns::Response, 60, 60, 60, TableHeaderComponent::notResizable);
		requestTable.getHeader().addColumn("Resend", (int)RequestModel::Columns::Resend, 60, 60, 60, TableHeaderComponent::notResizable);
		requestTable.setModel(&requestModel);
		skinTable(requestTable);

		downloadTable.getHeader().addColumn("StatusLED", (int)DownloadModel::Columns::StatusLed, 30, 30, 30, TableHeaderComponent::notResizable);
		downloadTable.getHeader().addColumn("Status", (int)DownloadModel::Columns::Status, 50, 50, 50, TableHeaderComponent::notResizable);
		downloadTable.getHeader().addColumn("URL", (int)DownloadModel::Columns::URL, 200, 200, 9000);
		downloadTable.getHeader().addColumn("Size", (int)DownloadModel::Columns::DownloadSize, 120, 120, 120, TableHeaderComponent::notResizable);
		downloadTable.getHeader().addColumn("Speed", (int)DownloadModel::Columns::DownloadSpeed, 70, 70, 70, TableHeaderComponent::notResizable);
		downloadTable.getHeader().addColumn("Pause", (int)DownloadModel::Columns::Pause, 60, 60, 60, TableHeaderComponent::notResizable);
		downloadTable.getHeader().addColumn("Abort", (int)DownloadModel::Columns::Abort, 60, 60, 60, TableHeaderComponent::notResizable);
		downloadTable.getHeader().addColumn("Show File", (int)DownloadModel::Columns::ShowFile, 60, 60, 60, TableHeaderComponent::notResizable);
		downloadTable.setModel(&downloadModel);
		skinTable(downloadTable);

		addAndMakeVisible(downloadTable);

		start();
	};

	void skinTable(TableListBox& table)
	{
		table.getHeader().setStretchToFitActive(true);
		table.setColour(TableListBox::ColourIds::backgroundColourId, Colours::transparentBlack);
		table.getViewport()->setScrollBarsShown(true, false, true, false);
		table.setMultipleSelectionEnabled(true);
		table.setLookAndFeel(&tlaf);
	}

	void timerCallback() override
	{
		if (requestDirty)
		{
			requestTable.updateContent();
			requestDirty.store(false);
		}

		if (downloadsDirty)
		{
			downloadTable.updateContent();
			downloadsDirty.store(false);
		}
		
		state.refresh();

		if (requestTable.isVisible())
			requestTable.repaint();

		if (downloadTable.isVisible())
			downloadTable.repaint();
	}
	
	static Colour getColourForState(GlobalServer::PendingCallback* data)
	{
		if (data->completionTimeMs == 0)
		{
			return Colours::grey;
		}

		if (data->status == 200)
			return Colours::green;

		if (data->status == 0 && data->requestTimeMs != 0)
		{
			return Colours::blue;
		}

		return Colours::red;
	}

	~ServerController()
	{
		getMainController()->getJavascriptThreadPool().getGlobalServer()->removeListener(this);
	}
	
	void queueChanged(int numItemsInQueue) override
	{
		if (auto s = getServerClass())
		{
			for (int i = 0; i < s->getNumPendingRequests(); i++)
			{
				auto r = s->getPendingCallback(i);

				SimpleReadWriteLock::ScopedWriteLock sl(requestModel.dataLock);
				requestModel.objects.addIfNotAlreadyThere(r.get());
			}
		}

		requestDirty.store(true);
	}

	void downloadQueueChanged(int numItemsToDownload) override
	{
		downloadsDirty.store(true);
	}

	void buttonClicked(Button* b) override
	{
		auto name = b->getName();

		if (name == "response" || name == "edit")
		{
			auto index = requestTable.getRowNumberOfComponent(b->getParentComponent()->getParentComponent());

			if (auto d = requestModel.getData(index))
			{
				var obj = d->responseObj;

				String title = "JSON Response Viewer";
				bool isEditable = false;

				if (name == "edit")
				{
					auto n = new DynamicObject();
					auto keys = d->url.getParameterNames();
					auto values = d->url.getParameterValues();

					for (int i = 0; i < keys.size(); i++)
					{
						n->setProperty(keys[i], values[i]);
					}

					obj = var(n);
					title = "URL Parameter Editor";
					isEditable = true;
				}

				auto ed = new JSONEditor(obj);
				ed->setEditable(isEditable);
				ed->setName(title);
				ed->setSize(500, 500);

				ed->setCallback([d](const var& o)
				{
					if (auto obj = o.getDynamicObject())
					{
						StringPairArray newParameters;

						for (auto nv : obj->getProperties())
							newParameters.set(nv.name.toString(), nv.value.toString());

						juce::URL newURL(d->url.toString(false));
						newURL = newURL.withParameters(newParameters);
						d->url = newURL;
					}
				});

				findParentComponentOfClass<FloatingTile>()->showComponentInRootPopup(ed, b, { b->getWidth() / 2, b->getHeight() });
			}
		}

		if (name == "resend")
		{
			if (auto s = getServerClass())
			{
				auto index = requestTable.getRowNumberOfComponent(b->getParentComponent()->getParentComponent());

				if (auto data = requestModel.getData(index))
				{
					auto ok = s->resendCallback(data.get());

					if (!ok.wasOk())
					{
						PresetHandler::showMessageWindow("Resend error", ok.getErrorMessage(), PresetHandler::IconType::Error);
					}
				}
			}
		}

		if (b == &pauseButton)
		{
			if (auto s = getServerClass())
			{
				if (b->getToggleState())
					s->stop();
				else
					s->resume();
			}
		}
		if (b == &requestButton || b == &downloadButton)
		{
			resized();
		}
		if (b == &clearButton)
		{
			if (auto s = getServerClass())
			{
				s->cleanFinishedDownloads();
				requestModel.clearUnusedData();
				requestDirty.store(true);
			}
		}
	}

	void addButton(HiseShapeButton& b)
	{
		addAndMakeVisible(b);
		b.setToggleModeWithColourChange(true);
		HiseColourScheme::setDefaultColours(b);
	}

	void paint(Graphics& g) override
	{
		g.fillAll(Colour(0xFF262626));
		auto b = getLocalBounds();
		auto topRow = b.removeFromTop(28);
		GlobalHiseLookAndFeel::drawFake3D(g, topRow);
	}

	void resized() override
	{
		auto b = getLocalBounds();

		auto topRow = b.removeFromTop(28);
		
		

		state.setBounds(topRow.removeFromLeft(28).reduced(2));

		bool showRequests = requestButton.getToggleState();
		bool showDownloads = downloadButton.getToggleState();

		downloadTable.setVisible(showDownloads);
		requestTable.setVisible(showRequests);

		if (showDownloads && showRequests)
		{
			requestTable.setBounds(b.removeFromTop(b.getHeight() / 2));
			downloadTable.setBounds(b);
		}
		else if (showDownloads)
			downloadTable.setBounds(b);
		else if (showRequests)
			requestTable.setBounds(b);
		

		pauseButton.setBounds(topRow.removeFromLeft(28).reduced(2));
		clearButton.setBounds(topRow.removeFromLeft(28).reduced(2));

		topRow.removeFromLeft(20);

		requestButton.setBounds(topRow.removeFromLeft(24));
		downloadButton.setBounds(topRow.removeFromLeft(24));
	}

	GlobalServer* getServerClass()
	{
		return getMainController()->getJavascriptThreadPool().getGlobalServer();
	}

	std::atomic<bool> requestDirty = { true };
	std::atomic<bool> downloadsDirty = { true };

	Factory f;

	WeakReference<JavascriptProcessor> jp;

	GlobalHiseLookAndFeel glaf;

	DownloadModel downloadModel;
	RequestModel requestModel;
	TableListBox requestTable, downloadTable;
	
	TableHeaderLookAndFeel tlaf;

	HiseShapeButton downloadButton, requestButton, pauseButton, clearButton;

	StateComponent state;

};

Component* ServerControllerPanel::createContentComponent(int)
{
	return new ServerController(dynamic_cast<JavascriptProcessor*>(getProcessor()));
}

ScriptWatchTablePanel::ScriptWatchTablePanel(FloatingTile* parent) :
PanelWithProcessorConnection(parent)
{
    dynamic_cast<BackendProcessor*>(getMainController())->workbenches.addListener(this);
};

ScriptWatchTablePanel::~ScriptWatchTablePanel()
{
    dynamic_cast<BackendProcessor*>(getMainController())->workbenches.removeListener(this);
}

Identifier ScriptWatchTablePanel::getProcessorTypeId() const
{
	return JavascriptProcessor::getConnectorId();
}

Component* ScriptWatchTablePanel::createContentComponent(int /*index*/)
{
	if (auto sw = getContent<ScriptWatchTable>())
	{
		columnData = sw->getColumnVisiblilityData();
	}

	setStyleProperty("showConnectionBar", false);

	auto swt = new ScriptWatchTable();

	swt->restoreColumnVisibility(columnData);

	auto f = [this](Component* p, Component* c, Point<int> s)
	{
		findParentComponentOfClass<FloatingTile>()->getRootFloatingTile()->showComponentInRootPopup(p, c, s, true);
	};

	swt->setPopupFunction(f);
	 
	getMainController()->getFontSizeChangeBroadcaster().addListener(*swt, ScriptWatchTable::updateFontSize);

	Array<int> typeIds =
	{
		(int)DebugInformation::Type::Callback,
		(int)DebugInformation::Type::Constant,
		(int)DebugInformation::Type::ExternalFunction,
		(int)DebugInformation::Type::Globals,
		(int)DebugInformation::Type::InlineFunction,
		(int)DebugInformation::Type::Namespace,
		(int)DebugInformation::Type::RegisterVariable,
		(int)DebugInformation::Type::Variables
	};
	
	StringArray sa =
	{
		"Callbacks",
		"Constants",
		"Functions",
		"Globals",
		"Inline Functions",
		"Namespaces",
		"Register Variables",
		"Variables"
	};

	swt->setViewDataTypes(sa, typeIds);


	WeakReference<Processor> c = getConnectedProcessor();

	swt->setLogFunction([c](const String& m)
	{
		if (c != nullptr)
		{
			debugToConsole(c, m);
		}		
	});

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

	LOAD_EPATH_IF_URL("showall", ScriptnodeIcons::zoomFit);
	LOAD_EPATH_IF_URL("edit", OverlayIcons::penShape);
	LOAD_EPATH_IF_URL("editoff", OverlayIcons::lockShape);
	LOAD_EPATH_IF_URL("lock", OverlayIcons::lockShape);
	LOAD_EPATH_IF_URL("move", EditorIcons::resizeIcon);
	LOAD_EPATH_IF_URL("cancel", EditorIcons::cancelIcon);
	LOAD_EPATH_IF_URL("undo", EditorIcons::undoIcon);
	LOAD_EPATH_IF_URL("redo", EditorIcons::redoIcon);
	LOAD_PATH_IF_URL("rebuild", ColumnIcons::moveIcon);
	LOAD_EPATH_IF_URL("learn", EditorIcons::connectIcon);
	LOAD_PATH_IF_URL("vertical-align", ColumnIcons::verticalAlign);
	LOAD_PATH_IF_URL("horizontal-align", ColumnIcons::horizontalAlign);
	LOAD_PATH_IF_URL("vertical-distribute", ColumnIcons::verticalDistribute);
	LOAD_PATH_IF_URL("horizontal-distribute", ColumnIcons::horizontalDistribute);

	return p;
}

juce::Identifier ComplexDataManager::getProcessorTypeId() const
{
	return JavascriptProcessor::getConnectorId();
}

struct ComplexDataViewer : public Component,
						   public ComboBox::Listener
{
	ComplexDataViewer(JavascriptProcessor* h_, ExternalData::DataType t) :
		type(t),
		h(h_)
	{
		slotSelector.setLookAndFeel(&claf);
		addAndMakeVisible(slotSelector);
		refreshSelector();
		slotSelector.setTextWhenNothingSelected("Select slot");

		GlobalHiseLookAndFeel::setDefaultColours(slotSelector);
	}

	void refreshSelector()
	{
		int currentId = slotSelector.getSelectedId();
		int numEntries;

		slotSelector.clear(dontSendNotification);

		numEntries = h->getNumDataObjects(type);

		for (int i = 0; i < numEntries; i++)
		{
			slotSelector.addItem("Slot" + String(i), i + 1);
		}

		slotSelector.addItem("Add new slot", numEntries+1);
		slotSelector.setSelectedId(currentId, dontSendNotification);
		slotSelector.addListener(this);
	}

	void paint(Graphics& g) override
	{
		if (currentComponent == nullptr)
		{
			g.setColour(Colours::white.withAlpha(0.7f));
			g.setFont(GLOBAL_BOLD_FONT());
			g.drawText("Select slot or create new slot above", getLocalBounds().toFloat(), Justification::centred);
		}
	}

	void comboBoxChanged(ComboBox* c)
	{
		setContent(c->getSelectedItemIndex());
		refreshSelector();
	}

	void setContent(int slotIndex)
	{
		auto mc = dynamic_cast<Processor*>(h.get())->getMainController();

		auto c = h->getComplexBaseType(type, slotIndex);
		c->setUndoManager(mc->getControlUndoManager());
		c->setGlobalUIUpdater(mc->getGlobalUIUpdater());

		switch (type)
		{
		case ExternalData::DataType::AudioFile:
		{
			auto tn = new MultiChannelAudioBufferDisplay();
			currentComponent = tn;
			break;
		}
		case ExternalData::DataType::SliderPack:
		{
			auto sp = new SliderPack();
			currentComponent = sp;
			break;
		}
		case snex::ExternalData::DataType::Table:
		{
			auto te = new TableEditor();
			currentComponent = te;
			break;
		}
        default: break;
		}

		dynamic_cast<ComplexDataUIBase::EditorBase*>(currentComponent.get())->setComplexDataUIBase(c);

		addAndMakeVisible(currentComponent);
		resized();
	}

	void resized() override
	{
		auto b = getLocalBounds();
		slotSelector.setBounds(b.removeFromTop(24));

		if(currentComponent != nullptr)
			currentComponent->setBounds(b);
	}

	GlobalHiseLookAndFeel claf;
	ComboBox slotSelector;
	ScopedPointer<Component> currentComponent;
	ExternalData::DataType type;
	WeakReference<JavascriptProcessor> h;
};

juce::Component* ComplexDataManager::createContentComponent(int index)
{
	if (auto d = dynamic_cast<JavascriptProcessor*>(getConnectedProcessor()))
	{
		return new ComplexDataViewer(d, (snex::ExternalData::DataType)index);
	}

	return nullptr;
	

	
}

juce::Path OSCLogger::createPath(const String& url) const
{
	Path p;

	LOAD_PATH_IF_URL("filter", ColumnIcons::filterIcon);
	LOAD_EPATH_IF_URL("clear", SampleMapIcons::deleteSamples);
	LOAD_EPATH_IF_URL("pause", HiBinaryData::ProcessorEditorHeaderIcons::bypassShape);
	LOAD_EPATH_IF_URL("scale", ScriptnodeIcons::scaleIcon);
	LOAD_EPATH_IF_URL("script", HiBinaryData::SpecialSymbols::scriptProcessor);

	return p;
}

OSCLogger::OSCLogger(FloatingTile* parent) :
	FloatingTileContent(parent),
	rm(scriptnode::routing::GlobalRoutingManager::Helpers::getOrCreate(parent->getMainController())),
	clearButton("clear", nullptr, *this),
	filterButton("filter", nullptr, *this),
	pauseButton("pause", nullptr, *this)
{
	addAndMakeVisible(searchBox);

	GlobalHiseLookAndFeel::setTextEditorColours(searchBox);

	rm->oscListeners.addListener(*this, OSCLogger::updateConnection, false);

	updateConnection(*this, rm->lastData);

	clearButton.onClick = [this]()
	{
		oscLogList.clear();
		triggerAsyncUpdate();
	};

	pauseButton.setToggleModeWithColourChange(true);
	pauseButton.setToggleStateAndUpdateIcon(true);

	searchBox.onReturnKey = [this]()
	{
		searchPattern = nullptr;

		auto content = searchBox.getText();

		if (content.isNotEmpty())
		{
			try
			{
				searchPattern = new OSCAddressPattern(content);
			}
			catch (String& e)
			{
				PresetHandler::showMessageWindow("OSC Address Pattern error", e, PresetHandler::IconType::Error);
			}
		}

		triggerAsyncUpdate();
	};

	filterButton.onClick = BIND_MEMBER_FUNCTION_0(OSCLogger::triggerAsyncUpdate);

	list.setColour(ListBox::ColourIds::backgroundColourId, Colours::transparentBlack);
	list.setModel(this);
	addAndMakeVisible(list);

	addAndMakeVisible(filterButton);
	addAndMakeVisible(clearButton);
	addAndMakeVisible(pauseButton);

	filterButton.setToggleModeWithColourChange(true);

	fader.addScrollBarToAnimate(list.getVerticalScrollBar());
	list.getViewport()->setScrollBarThickness(12);
}

OSCLogger::~OSCLogger()
{
	if (rm != nullptr && rm->receiver != nullptr)
		dynamic_cast<OSCReceiver*>(rm->receiver.get())->removeListener(this);
}

void OSCLogger::paintListBoxItem(int row, Graphics& g, int width, int height, bool rowIsSelected)
{
	ignoreUnused(rowIsSelected);

	if (isPositiveAndBelow(row, displayedItems.size()))
	{
		auto& data = displayedItems.getReference(row);

		auto b = Rectangle<float>((float)width, (float)height);
		auto circle = b.removeFromLeft(b.getHeight()).reduced(4.0f);

		if (data.hasScriptCallback)
		{
			g.setColour(data.c);
			auto p = createPath("script");
			scalePath(p, circle);
			g.fillPath(p);
		}
		else
		{
			g.setColour(data.c);
			g.drawEllipse(circle, 2.0f);
			g.fillEllipse(circle.reduced(3.0f));
		}

		

		if (data.scaled)
		{
			auto p = createPath("scale");
			scalePath(p, b.withWidth(b.getHeight()).reduced(2.0f));

			g.setColour(Colours::white.withAlpha(0.2f));
			g.fillPath(p);
		}

		g.setColour(Colours::white.withAlpha(data.matchesDomain ? 0.8f : 0.3f));

		g.setFont(GLOBAL_MONOSPACE_FONT());

		g.drawText(data.message,
			b.toFloat(),
			Justification::centredLeft, true);
	}
}

void OSCLogger::addOSCMessage(const OSCMessage& message, int level /*= 0*/)
{
	if (!pauseButton.getToggleState())
		return;

	MessageItem m;
	
	auto pattern = message.getAddressPattern();

	if (!pattern.containsWildcards())
	{
		m.address = OSCAddress(pattern.toString());
	}
	

	m.message = getIndentationString(level)
		+ "- osc message, address = '"
		+ message.getAddressPattern().toString()
		+ "', "
		+ String(message.size())
		+ " argument(s)";

	m.isError = false;
	m.c = Colours::white.withAlpha(0.3f);

	

	if (lastData != nullptr)
	{
		auto id = message.getAddressPattern().toString().fromFirstOccurrenceOf(lastData->domain, false, false);

		for (auto c : rm->cables)
		{
			if (c->id == id)
			{
				m.c = scriptnode::routing::GlobalRoutingManager::Helpers::getColourFromId(id);
				break;
			}
				
		}

		m.matchesDomain = message.getAddressPattern().toString().startsWith(lastData->domain);
	}

	oscLogList.add(m);

	if (!message.isEmpty())
	{
		auto cableIds = scriptnode::routing::GlobalRoutingManager::Helpers::getCableIds(message, lastData->domain);

		int index = 0;

		for (auto& arg : message)
			addOSCMessageArgument(m, arg, level + 1, cableIds[index++]);
	}

	triggerAsyncUpdate();
}

void OSCLogger::addOSCBundle(const OSCBundle& bundle, int level /*= 0*/)
{
	if (!pauseButton.getToggleState())
		return;

	OSCTimeTag timeTag = bundle.getTimeTag();

	MessageItem m;
	
	m.message = getIndentationString(level)
		+ "- osc bundle, time tag = "
		+ timeTag.toTime().toString(true, true, true, true);

	for (auto& element : bundle)
	{
		if (element.isMessage())
			addOSCMessage(element.getMessage(), level + 1);
		else if (element.isBundle())
			addOSCBundle(element.getBundle(), level + 1);
	}

	triggerAsyncUpdate();
}

void OSCLogger::addOSCMessageArgument(const MessageItem& m, const OSCArgument& arg, int level, const String& cableId)
{
	String typeAsString;
	String valueAsString;

	if (arg.isFloat32())
	{
		typeAsString = "float32";
		valueAsString = String(arg.getFloat32());
	}
	else if (arg.isInt32())
	{
		typeAsString = "int32";
		valueAsString = String(arg.getInt32());
	}
	else if (arg.isString())
	{
		typeAsString = "string";
		valueAsString = arg.getString();
	}
	else if (arg.isBlob())
	{
		typeAsString = "blob";
		auto& blob = arg.getBlob();
		valueAsString = String::fromUTF8((const char*)blob.getData(), (int)blob.getSize());
	}
	else
	{
		typeAsString = "(unknown)";
	}

	

	MessageItem am;
	
	OSCAddress address(lastData->domain + cableId.upToFirstOccurrenceOf("[", false, false));

	for (const auto& a : rm->scriptCallbackPatterns)
	{
		if (a.matches(address))
		{
			am.hasScriptCallback = true;
			break;
		}
	}

	for (const auto& nr : lastData->inputRanges)
	{
		if (nr.id == cableId)
		{
			am.scaled = true;
			auto inputValue = valueAsString.getDoubleValue();
			valueAsString << " -> " << String((float)nr.rng.convertTo0to1(inputValue, true));

			valueAsString << " (Input Range: " << scriptnode::RangeHelpers::toDisplayString(nr.rng) << ")";

			break;
		}
	}

	am.address = m.address;
	am.message = getIndentationString(level + 1) + "- " + cableId + ": " + typeAsString.paddedRight(' ', 12) + valueAsString;
	am.c = m.c;
	am.matchesDomain = m.matchesDomain;

	if (lastData != nullptr)
	{
		for (auto c : rm->cables)
		{
			if (c->id == cableId)
			{
				am.c = scriptnode::routing::GlobalRoutingManager::Helpers::getColourFromId(cableId);
				am.hasCableConnection = true;
				break;
			}

		}
	}

	oscLogList.add(am);
}

void OSCLogger::addInvalidOSCPacket(const char* /* data */, int dataSize)

{
	MessageItem i;
	i.message = "(" + String(dataSize) + "bytes with invalid format)";
	i.matchesDomain = false;
	i.isError = true;

	oscLogList.add(i);
}

void OSCLogger::clear()
{
	oscLogList.clear();
	triggerAsyncUpdate();
}

void OSCLogger::paint(Graphics& g)
{
	g.fillAll(Colour(0xFF222222));

	Path searchIcon;
	searchIcon.loadPathFromData(EditorIcons::searchIcon, sizeof(EditorIcons::searchIcon));
	searchIcon.applyTransform(AffineTransform::rotation(float_Pi));

	auto c = topRow.toFloat();
	scalePath(searchIcon, c.removeFromLeft(c.getHeight()).reduced(2.0f));

	GlobalHiseLookAndFeel::drawFake3D(g, topRow);

	g.setColour(Colours::white.withAlpha(0.5f));
	g.fillPath(searchIcon);

	const float labelAlpha = 0.6f;
	const float valueAlpha = 0.8f;

	if (lastData != nullptr)
	{
		AttributedString stats;
		auto lf = GLOBAL_BOLD_FONT();
		auto vf = GLOBAL_MONOSPACE_FONT();

		auto receiveOK = rm != nullptr && rm->receiver != nullptr && rm->receiver->ok;
		auto sendOk = rm != nullptr && rm->sender != nullptr && rm->sender->ok;

		stats.append("Domain: ", lf, Colours::white.withAlpha(labelAlpha));
		stats.append(lastData->domain, vf, Colours::white.withAlpha(valueAlpha));
		stats.append(", Input Port: ", lf, Colours::white.withAlpha(labelAlpha));
		stats.append(String(lastData->sourcePort), vf, Colour(receiveOK ? HISE_OK_COLOUR : HISE_ERROR_COLOUR));
		stats.append(", Output Port: ", lf, Colours::white.withAlpha(labelAlpha));
		stats.append(String(lastData->targetPort), vf, Colour(sendOk ? HISE_OK_COLOUR : HISE_ERROR_COLOUR));
		stats.setJustification(Justification::centredLeft);

		auto copy = topRow.toFloat();
		copy.removeFromLeft(clearButton.getRight() + 15.0f);

		stats.draw(g, copy);
	}

	if (oscLogList.isEmpty())
	{
		g.setColour(Colours::white.withAlpha(0.2f));
		g.setFont(GLOBAL_BOLD_FONT());
		g.drawText("No OSC messages received", getLocalBounds().toFloat(), Justification::centred);
	}
}

void OSCLogger::resized()
{
	auto b = getParentShell()->getContentBounds();

	topRow = b.removeFromTop(24);

	auto copy = topRow;

	copy.removeFromLeft(copy.getHeight());

	searchBox.setBounds(copy.removeFromLeft(150));

	pauseButton.setBounds(copy.removeFromLeft(copy.getHeight()).reduced(2));
	filterButton.setBounds(copy.removeFromLeft(copy.getHeight()).reduced(2));
	clearButton.setBounds(copy.removeFromLeft(copy.getHeight()).reduced(2));

	

	list.setBounds(b);
}

void OSCLogger::updateConnection(OSCLogger& logger, OSCConnectionData::Ptr data)
{
	logger.lastData = data;

	if (logger.rm->receiver != nullptr)
	{
		dynamic_cast<OSCReceiver*>(logger.rm->receiver.get())->addListener(&logger);
		logger.repaint();
	}
}

void OSCLogger::handleAsyncUpdate()
{
	displayedItems.clear();

	int startIndex = jmax(0, oscLogList.size() - 128);

	for(int i = startIndex; i < oscLogList.size(); i++)
	{
		auto item = oscLogList[i];

		auto isActive = !filterButton.getToggleState() || item.matchesDomain;

		auto matchesWildcard = searchPattern == nullptr || searchPattern->matches(item.address);

		if (isActive && matchesWildcard)
			displayedItems.add(item);
	}
	
	list.updateContent();
	list.scrollToEnsureRowIsOnscreen(displayedItems.size() - 1);
	repaint();
}



} // namespace hise
