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
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/





ScriptEditHandler::ScriptEditHandler()
{

}

void ScriptEditHandler::createNewComponent(Widgets componentType, int x, int y)
{
	if (getScriptEditHandlerContent() == nullptr)
		return;

	if (getScriptEditHandlerEditor() == nullptr)
	{
		return;
	}

	String widgetType;

	switch (componentType)
	{
	case Widgets::Knob:				widgetType = "Knob"; break;
	case Widgets::Button:			widgetType = "Button"; break;
	case Widgets::Table:				widgetType = "Table"; break;
	case Widgets::ComboBox:			widgetType = "ComboBox"; break;
	case Widgets::Label:				widgetType = "Label"; break;
	case Widgets::Image:				widgetType = "Image"; break;
	case Widgets::Viewport:			widgetType = "Viewport"; break;
	case Widgets::Plotter:			widgetType = "Plotter"; break;
	case Widgets::ModulatorMeter:	widgetType = "ModulatorMeter"; break;
	case Widgets::Panel:				widgetType = "Panel"; break;
	case Widgets::AudioWaveform:		widgetType = "AudioWaveform"; break;
	case Widgets::SliderPack:		widgetType = "SliderPack"; break;
	case Widgets::FloatingTile:		widgetType = "FloatingTile"; break;
	case Widgets::duplicateWidget:
	{
		auto b = getScriptEditHandlerOverlay()->getScriptComponentEditBroadcaster();

		auto sc = b->getFirstFromSelection();

		widgetType = sc->getObjectName().toString();
		widgetType = widgetType.replace("Scripted", "");
		widgetType = widgetType.replace("Script", "");
		widgetType = widgetType.replace("Slider", "Knob");
		break;
	}
	case Widgets::numWidgets: break;
	}

	auto content = getScriptEditHandlerProcessor()->getContent();

	Identifier id = ScriptingApi::Content::Helpers::getUniqueIdentifier(content, widgetType);

	ScriptComponent::Ptr newComponent;

	switch (componentType)
	{
	case hise::ScriptEditHandler::Widgets::Knob: 
		newComponent = content->createNewComponent<ScriptingApi::Content::ScriptSlider>(id, x, y, 128, 48);
		break;
	case hise::ScriptEditHandler::Widgets::Button:
		newComponent = content->createNewComponent<ScriptingApi::Content::ScriptButton>(id, x, y, 128, 28);
		break;
	case hise::ScriptEditHandler::Widgets::Table:
		newComponent = content->createNewComponent<ScriptingApi::Content::ScriptTable>(id, x, y, 100, 50);
		break;
	case hise::ScriptEditHandler::Widgets::ComboBox:
		newComponent = content->createNewComponent<ScriptingApi::Content::ScriptComboBox>(id, x, y, 128, 32);
		break;
	case hise::ScriptEditHandler::Widgets::Label:
		newComponent = content->createNewComponent<ScriptingApi::Content::ScriptLabel>(id, x, y, 128, 28);
		break;
	case hise::ScriptEditHandler::Widgets::Image:
		newComponent = content->createNewComponent<ScriptingApi::Content::ScriptImage>(id, x, y, 50, 50);
		break;
	case hise::ScriptEditHandler::Widgets::Viewport:
		newComponent = content->createNewComponent<ScriptingApi::Content::ScriptedViewport>(id, x, y, 200, 100);
		break;
	case hise::ScriptEditHandler::Widgets::Plotter:
		newComponent = content->createNewComponent<ScriptingApi::Content::ScriptedPlotter>(id, x, y, 100, 50);
		break;
	case hise::ScriptEditHandler::Widgets::ModulatorMeter:
		newComponent = content->createNewComponent<ScriptingApi::Content::ModulatorMeter>(id, x, y, 100, 50);
		break;
	case hise::ScriptEditHandler::Widgets::Panel:
		newComponent = content->createNewComponent<ScriptingApi::Content::ScriptPanel>(id, x, y, 100, 50);
		break;
	case hise::ScriptEditHandler::Widgets::AudioWaveform:
		newComponent = content->createNewComponent<ScriptingApi::Content::ScriptAudioWaveform>(id, x, y, 200, 100);
		break;
	case hise::ScriptEditHandler::Widgets::SliderPack:
		newComponent = content->createNewComponent<ScriptingApi::Content::ScriptSliderPack>(id, x, y, 200, 100);
		break;
	case hise::ScriptEditHandler::Widgets::FloatingTile:
		newComponent = content->createNewComponent<ScriptingApi::Content::ScriptFloatingTile>(id, x, y, 200, 100);
		break;
	case hise::ScriptEditHandler::Widgets::duplicateWidget:
		jassertfalse;
		break;
	case hise::ScriptEditHandler::Widgets::numWidgets:
		jassertfalse;
		break;
	default:
		break;
	}

	compileScript();

	auto b = content->getScriptProcessor()->getMainController_()->getScriptComponentEditBroadcaster();

	b->setSelection(newComponent);

#if 0
	switch (componentType)
	{
	case Widgets::Knob:				widgetType = "Knob"; break;
	case Widgets::Button:			widgetType = "Button"; break;
	case Widgets::Table:				widgetType = "Table"; break;
	case Widgets::ComboBox:			widgetType = "ComboBox"; break;
	case Widgets::Label:				widgetType = "Label"; break;
	case Widgets::Image:				widgetType = "Image"; break;
	case Widgets::Viewport:			widgetType = "Viewport"; break;
	case Widgets::Plotter:			widgetType = "Plotter"; break;
	case Widgets::ModulatorMeter:	widgetType = "ModulatorMeter"; break;
	case Widgets::Panel:				widgetType = "Panel"; break;
	case Widgets::AudioWaveform:		widgetType = "AudioWaveform"; break;
	case Widgets::SliderPack:		widgetType = "SliderPack"; break;
	case Widgets::FloatingTile:		widgetType = "FloatingTile"; break;
	case Widgets::duplicateWidget:
	{
		widgetType = getScriptEditHandlerContent()->getEditedComponent()->getObjectName().toString();
		widgetType = widgetType.replace("Scripted", "");
		widgetType = widgetType.replace("Script", "");
		widgetType = widgetType.replace("Slider", "Knob");
		break;
	}
	case Widgets::numWidgets: break;
	}

	String id = PresetHandler::getCustomName(widgetType);

	String errorMessage = isValidWidgetName(id);

	while (errorMessage.isNotEmpty() && PresetHandler::showYesNoWindow("Wrong variable name", errorMessage + "\nPress 'OK' to re-enter a valid variable name or 'Cancel' to abort", PresetHandler::IconType::Warning))
	{
		id = PresetHandler::getCustomName(widgetType);
		errorMessage = isValidWidgetName(id);
	}

	errorMessage = isValidWidgetName(id);

	if (errorMessage.isNotEmpty()) return;

	String textToInsert;

	textToInsert << "\nconst var " << id << " = Content.add" << widgetType << "(\"" << id << "\", " << x << ", " << y << ");\n";

	if (componentType == Widgets::duplicateWidget)
	{
		const int xOfOriginal = getScriptEditHandlerContent()->getEditedComponent()->getScriptObjectProperty(ScriptingApi::Content::ScriptComponent::x);
		const int yOfOriginal = getScriptEditHandlerContent()->getEditedComponent()->getScriptObjectProperty(ScriptingApi::Content::ScriptComponent::y);

		const String originalId = getScriptEditHandlerContent()->getEditedComponent()->getName().toString();

		if (getScriptEditHandlerEditor()->componentIsDefinedWithFactoryMethod(originalId))
		{
			textToInsert = getScriptEditHandlerEditor()->createNewDefinitionWithFactoryMethod(originalId, id, x, y);
		}
	}

	auto onInit = getScriptEditHandlerProcessor()->getSnippet("onInit");

	CodeDocument::Position end(*onInit, onInit->getNumCharacters());

	onInit->insertText(end, textToInsert);

	compileScript();
#endif
}

void ScriptEditHandler::toggleComponentSelectMode(bool shouldSelectOnClick)
{
	useComponentSelectMode = shouldSelectOnClick;

	getScriptEditHandlerContent()->setInterceptsMouseClicks(false, !useComponentSelectMode);
}

void ScriptEditHandler::compileScript()
{
	ProcessorWithScriptingContent *s = dynamic_cast<ProcessorWithScriptingContent*>(getScriptEditHandlerProcessor());
	
	Processor* p = dynamic_cast<Processor*>(getScriptEditHandlerProcessor());
	Component* thisAsComponent = dynamic_cast<Component*>(this);

	PresetHandler::setChanged(p);

	scriptEditHandlerCompileCallback();
}


String ScriptEditHandler::isValidWidgetName(const String &id)
{
	if (id.isEmpty()) return "Identifier must not be empty";

	if (!Identifier::isValidIdentifier(id)) return "Identifier must not contain whitespace or weird characters";

	ScriptingApi::Content* content = dynamic_cast<ProcessorWithScriptingContent*>(getScriptEditHandlerProcessor())->getScriptingContent();

	for (int i = 0; i < content->getNumComponents(); i++)
	{
		if (content->getComponentWithName(Identifier(id)) != nullptr)
			return  "Identifier " + id + " already exists";
	}

	return String();
}

ScriptingContentOverlay::ScriptingContentOverlay(ScriptEditHandler* handler_) :
	ScriptComponentEditListener(dynamic_cast<Processor*>(handler_->getScriptEditHandlerProcessor())),
	handler(handler_)
{
	addAsScriptEditListener();

	addAndMakeVisible(dragModeButton = new ShapeButton("Drag Mode", Colours::black.withAlpha(0.6f), Colours::black.withAlpha(0.8f), Colours::black.withAlpha(0.8f)));

	Path path;
	path.loadPathFromData(OverlayIcons::lockShape, sizeof(OverlayIcons::lockShape));

	dragModeButton->setShape(path, true, true, false);

	dragModeButton->addListener(this);

	dragModeButton->setTooltip("Toggle between Edit / Performance mode");

	setEditMode(handler->editModeEnabled());

	setWantsKeyboardFocus(true);
}


ScriptingContentOverlay::~ScriptingContentOverlay()
{
	removeAsScriptEditListener();
}

void ScriptingContentOverlay::resized()
{
	dragModeButton->setBounds(getWidth() - 28, 12, 16, 16);
}


void ScriptingContentOverlay::buttonClicked(Button* /*buttonThatWasClicked*/)
{
	toggleEditMode();

}

void ScriptingContentOverlay::toggleEditMode()
{
	setEditMode(!dragMode);

	handler->toggleComponentSelectMode(dragMode);
}

void ScriptingContentOverlay::setEditMode(bool editModeEnabled)
{
	dragMode = editModeEnabled;

	Path p;

	if (dragMode == false)
	{
		p.loadPathFromData(OverlayIcons::lockShape, sizeof(OverlayIcons::lockShape));
		clearDraggers();
		setInterceptsMouseClicks(false, true);
	}
	else
	{
		p.loadPathFromData(OverlayIcons::penShape, sizeof(OverlayIcons::penShape));
		setInterceptsMouseClicks(true, true);
	}

	dragModeButton->setShape(p, true, true, false);
	dragModeButton->setToggleState(dragMode, dontSendNotification);

	resized();
	repaint();
}

Rectangle<float> getFloatRectangle(const Rectangle<int> &r)
{
	return Rectangle<float>((float)r.getX(), (float)r.getY(), (float)r.getWidth(), (float)r.getHeight());
}

void ScriptingContentOverlay::paint(Graphics& g)
{
	if (dragMode)
	{
        g.setColour(Colours::white.withAlpha(0.05f));
		g.fillAll();

		const bool isInPopup = findParentComponentOfClass<ScriptingEditor>() == nullptr;

		Colour lineColour = isInPopup ? Colours::white : Colours::black;

		for (int x = 10; x < getWidth(); x += 10)
		{
			g.setColour(lineColour.withAlpha((x % 100 == 0) ? 0.12f : 0.05f));
            g.drawVerticalLine(x, 0.0f, (float)getHeight());
		}

		for (int y = 10; y < getHeight(); y += 10)
		{
			g.setColour(lineColour.withAlpha(((y) % 100 == 0) ? 0.1f : 0.05f));
			g.drawHorizontalLine(y, 0.0f, (float)getWidth());
		}
	}

	if (dragModeButton->isVisible())
	{
		Colour c = Colours::white;

		g.setColour(c.withAlpha(0.2f));

		g.fillRoundedRectangle(getFloatRectangle(dragModeButton->getBounds().expanded(3)), 3.0f);
	}
}




void ScriptingContentOverlay::scriptComponentSelectionChanged()
{
	clearDraggers();

	ScriptComponentEditBroadcaster::Iterator iter(getScriptComponentEditBroadcaster());

	auto content = handler->getScriptEditHandlerContent();

	while (auto c = iter.getNextScriptComponent())
	{
		auto draggedComponent = content->getComponentFor(c);

		if (draggedComponent == nullptr)
		{
			PresetHandler::showMessageWindow("Can't select component", "The component " + c->getName() + " can't be selected", PresetHandler::IconType::Error);
			clearDraggers();
			return;
		}

		auto d = new Dragger(c, draggedComponent);

		addAndMakeVisible(d);

		draggers.add(d);

		auto boundsInParent = content->getLocalArea(draggedComponent->getParentComponent(), draggedComponent->getBoundsInParent());

		d->setBounds(boundsInParent);
	}
}


void ScriptingContentOverlay::scriptComponentPropertyChanged(ScriptComponent* sc, Identifier idThatWasChanged, const var& newValue)
{
	var x = 2;
}


bool ScriptingContentOverlay::keyPressed(const KeyPress &key)
{
	auto b = getScriptComponentEditBroadcaster();

	static const Identifier x("x");
	static const Identifier y("y");
	static const Identifier w("width");
	static const Identifier h("height");

	const int keyCode = key.getKeyCode();
	const int sign = (keyCode == KeyPress::leftKey || keyCode == KeyPress::upKey) ? -1 : 1;
	const int delta = sign * (key.getModifiers().isCommandDown() ? 10 : 1);
	const bool resizeComponent = key.getModifiers().isShiftDown();
	
	if (keyCode == KeyPress::leftKey || keyCode == KeyPress::rightKey)
	{
		if (resizeComponent)
		{
			b->setScriptComponentPropertyDeltaForSelection(w, delta, sendNotification, true);
			return true;
		}
		else
		{
			b->setScriptComponentPropertyDeltaForSelection(x, delta, sendNotification, true);
			return true;
		}

		return true;
	}
	else if (keyCode == KeyPress::upKey || keyCode == KeyPress::downKey)
	{
		if (resizeComponent)
		{
			b->setScriptComponentPropertyDeltaForSelection(h, delta, sendNotification, true);
			return true;
		}
		else
		{
			b->setScriptComponentPropertyDeltaForSelection(y, delta, sendNotification, true);
			return true;
		}

		return true;
	}
	else if (keyCode == 'Z' && key.getModifiers().isCommandDown())
	{
		b->getUndoManager().undo();
		return true;
	}
	else if (keyCode == 'D' && key.getModifiers().isCommandDown())
	{
		if (draggers.size() > 0)
		{
			auto pwsc = dynamic_cast<ProcessorWithScriptingContent*>(handler->getScriptEditHandlerProcessor());

			auto start = draggers.getFirst()->getPosition();

			auto end = getMouseXYRelative();

			auto deltaX = end.x - start.x;
			auto deltaY = end.y - start.y;

			ScriptingApi::Content::Helpers::duplicateSelection(pwsc->getScriptingContent(), b->getSelection(), deltaX, deltaY);
		}

		return true;
	}
	else if (keyCode == 'C' && key.getModifiers().isCommandDown())
	{
		auto s = ScriptingApi::Content::Helpers::createScriptVariableDeclaration(b->getSelection());
		SystemClipboard::copyTextToClipboard(s);
		return true;
	}
	else if (keyCode == KeyPress::deleteKey)
	{
		auto pwsc = dynamic_cast<ProcessorWithScriptingContent*>(handler->getScriptEditHandlerProcessor());

		ScriptingApi::Content::Helpers::deleteSelection(pwsc->getScriptingContent(), b);

		return true;
	}

	return false;
}


void ScriptingContentOverlay::findLassoItemsInArea(Array<ScriptComponent*> &itemsFound, const Rectangle< int > &area)
{
	auto content = handler->getScriptEditHandlerContent();
	content->getScriptComponentsFor(itemsFound, area);

}

void ScriptingContentOverlay::mouseDown(const MouseEvent& e)
{
	if (e.mods.isShiftDown())
	{
		lassoSet.deselectAll();
		addAndMakeVisible(lasso);
		lasso.beginLasso(e, this);
		return;
	}

	auto content = handler->getScriptEditHandlerContent();
	auto processor = dynamic_cast<Processor*>(handler->getScriptEditHandlerProcessor());
	
	jassert(content != nullptr);

	if (e.mods.isLeftButtonDown() && handler->editModeEnabled())
	{
		Array<ScriptingApi::Content::ScriptComponent*> components;

		content->getScriptComponentsFor(components, e.getEventRelativeTo(content).getPosition());

		auto mc = processor->getMainController();

		if (components.size() > 1)
		{
			PopupMenu m;
			ScopedPointer<PopupLookAndFeel> luf = new PopupLookAndFeel();
			m.setLookAndFeel(luf);

			m.addSectionHeader("Choose Control to Edit");


			for (int i = 0; i < components.size(); i++)
			{
				auto name = components[i]->getName().toString();

				if (!components[i]->isShowing())
					continue;

				m.addItem(i + 1, name);
			}

			for (int i = 0; i < components.size(); i++)
			{
				auto name = components[i]->getName().toString();

				if (!components[i]->isShowing())
				{
					name << " (Hidden)";
					m.addItem(i + 1, name);
				}
			}

			const int result = m.show();

			if (result > 0)
			{
				auto sc = components[result - 1];
				
				

				getScriptComponentEditBroadcaster()->updateSelectionBasedOnModifier(sc, e.mods, sendNotification);

				
				auto root = GET_ROOT_FLOATING_TILE(this);
				BackendPanelHelpers::toggleVisibilityForRightColumnPanel<ScriptComponentEditPanel::Panel>(root, sc != nullptr);
			}

		}
		else
		{
			ScriptingApi::Content::ScriptComponent *sc = content->getScriptComponentFor(e.getEventRelativeTo(content).getPosition());
			
			getScriptComponentEditBroadcaster()->updateSelectionBasedOnModifier(sc, e.mods, sendNotification);

			

			auto root = GET_ROOT_FLOATING_TILE(this);
			BackendPanelHelpers::toggleVisibilityForRightColumnPanel<ScriptComponentEditPanel::Panel>(root, sc != nullptr);
		}
	}

	if (e.mods.isRightButtonDown())
	{
		enum ComponentOffsets
		{
			addCallbackOffset = 10000,
			showCallbackOffset = 15000,
			editComponentOffset = 20000
		};

		PopupMenu m;
		ScopedPointer<PopupLookAndFeel> luf = new PopupLookAndFeel();
		m.setLookAndFeel(luf);

		Array<ScriptingApi::Content::ScriptComponent*> components;

		content->getScriptComponentsFor(components, e.getEventRelativeTo(content).getPosition());

		if (handler->editModeEnabled())
		{
			m.addSectionHeader("Create new widget");
			m.addItem((int)ScriptEditHandler::Widgets::Knob, "Add new Slider");
			m.addItem((int)ScriptEditHandler::Widgets::Button, "Add new Button");
			m.addItem((int)ScriptEditHandler::Widgets::Table, "Add new Table");
			m.addItem((int)ScriptEditHandler::Widgets::ComboBox, "Add new ComboBox");
			m.addItem((int)ScriptEditHandler::Widgets::Label, "Add new Label");
			m.addItem((int)ScriptEditHandler::Widgets::Image, "Add new Image");
			m.addItem((int)ScriptEditHandler::Widgets::Viewport, "Add new Viewport");
			m.addItem((int)ScriptEditHandler::Widgets::Plotter, "Add new Plotter");
			m.addItem((int)ScriptEditHandler::Widgets::ModulatorMeter, "Add new ModulatorMeter");
			m.addItem((int)ScriptEditHandler::Widgets::Panel, "Add new Panel");
			m.addItem((int)ScriptEditHandler::Widgets::AudioWaveform, "Add new AudioWaveform");
			m.addItem((int)ScriptEditHandler::Widgets::SliderPack, "Add new SliderPack");
			m.addItem((int)ScriptEditHandler::Widgets::FloatingTile, "Add new FloatingTile");

			m.addItem((int)ScriptEditHandler::Widgets::duplicateWidget, "Duplicate selected component", components.size() > 0);

			if (components.size() != 0)
			{
				m.addSeparator();

				if (components.size() == 1)
				{
					m.addItem(editComponentOffset, "Edit \"" + components[0]->getName().toString() + "\" in Panel");
					m.addItem(addCallbackOffset, "Add custom callback for " + components[0]->getName().toString(), components[0]->getCustomControlCallback() == nullptr);
					m.addItem(showCallbackOffset, "Show custom callback for " + components[0]->getName().toString(), components[0]->getCustomControlCallback() != nullptr);
				}
				else
				{
					PopupMenu editSub;
					PopupMenu addSub;
					PopupMenu showSub;

					for (int i = 0; i < components.size(); i++)
					{
						editSub.addItem(editComponentOffset + i, components[i]->getName().toString());
						addSub.addItem(addCallbackOffset + i, components[i]->getName().toString(), components[0]->getCustomControlCallback() == nullptr);
						showSub.addItem(showCallbackOffset + i, components[i]->getName().toString(), components[0]->getCustomControlCallback() != nullptr);
					}

					m.addSubMenu("Edit in Panel", editSub, components.size() != 0);
					m.addSubMenu("Add custom callback for", addSub, components.size() != 0);
					m.addSubMenu("Show custom callback for", showSub, components.size() != 0);
				}
			}
		}
		else
		{
			return;
		}

		int result = m.show();

		if (result >= (int)ScriptEditHandler::Widgets::Knob && result < (int)ScriptEditHandler::Widgets::numWidgets)
		{
			const int insertX = e.getEventRelativeTo(content).getMouseDownPosition().getX();
			const int insertY = e.getEventRelativeTo(content).getMouseDownPosition().getY();

			handler->createNewComponent((ScriptEditHandler::Widgets)result, insertX, insertY);
		}
		else if (result >= editComponentOffset) // EDIT IN PANEL
		{
			auto sc = components[result - editComponentOffset];

			getScriptComponentEditBroadcaster()->updateSelectionBasedOnModifier(sc, e.mods, sendNotification);

		}
		else if (result >= showCallbackOffset)
		{
			auto componentToUse = components[result - showCallbackOffset];

			auto func = dynamic_cast<DebugableObject*>(componentToUse->getCustomControlCallback());

			
			if (func != nullptr)
				func->doubleClickCallback(e, dynamic_cast<Component*>(handler));
		}
		else if (result >= addCallbackOffset)
		{
			auto componentToUse = components[result - addCallbackOffset];

			auto name = componentToUse->getName();

			NewLine nl;
			String code;


			String callbackName = "on" + name.toString() + "Control";

			code << nl;
			code << "inline function " << callbackName << "(component, value)" << nl;
			code << "{" << nl;
			code << "\t//Add your custom logic here..." << nl;
			code << "};" << nl;
			code << nl;
			code << name.toString() << ".setControlCallback(" << callbackName << ");" << nl;

			
			auto doc = JavascriptCodeEditor::Helpers::gotoAndReturnDocumentWithDefinition(processor, componentToUse);

			if (doc != nullptr)
			{
				int insertPos = JavascriptCodeEditor::Helpers::getPositionAfterDefinition(*doc, name).getPosition();

				doc->insertText(insertPos, code);
			}

			handler->compileScript();
		}
	}
}

static void removeChildComponentsFromArray(Array<ScriptComponent*>& arrayToClean)
{
	for (int i = 0; i < arrayToClean.size(); i++)
	{
		auto sc = arrayToClean[i];

		for (int j = 0; j < sc->getNumChildComponents(); j++)
		{
			arrayToClean.removeAllInstancesOf(sc->getChildComponent(j));
		}
	}
}

void ScriptingContentOverlay::mouseUp(const MouseEvent &e)
{
	if (lasso.isVisible())
	{
		lasso.setVisible(false);
		lasso.endLasso();

		auto itemsFound = lassoSet.getItemArray();

		// Remove all child components from the new selection.
		removeChildComponentsFromArray(itemsFound);

		auto b = getScriptComponentEditBroadcaster();

		b->clearSelection(dontSendNotification);

		for (int i = 0; i < itemsFound.size(); i++)
		{
			b->addToSelection(itemsFound[i], (i == itemsFound.size() - 1 ? sendNotification : dontSendNotification));
		}
	}
}


void ScriptingContentOverlay::mouseDrag(const MouseEvent& e)
{
	if (lasso.isVisible())
	{
		lasso.dragLasso(e);

	}
}

ScriptingContentOverlay::Dragger::Dragger(ScriptComponent* sc_, Component* componentToDrag):
	sc(sc_),
	draggedComponent(componentToDrag)
{
	currentMovementWatcher = new MovementWatcher(componentToDrag, this);

	
	constrainer.setMinimumOnscreenAmounts(0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF);

	addAndMakeVisible(resizer = new ResizableCornerComponent(this, &constrainer));

	resizer->addMouseListener(this, true);

	setVisible(true);
	setWantsKeyboardFocus(true);

	setAlwaysOnTop(true);
	grabKeyboardFocus();

	
}

ScriptingContentOverlay::Dragger::~Dragger()
{
	
}

void ScriptingContentOverlay::Dragger::paint(Graphics &g)
{
	g.fillAll(Colours::black.withAlpha(0.2f));
	g.setColour(Colour(SIGNAL_COLOUR).withAlpha(0.5f));

	if (!snapShot.isNull()) g.drawImageAt(snapShot, 0, 0);

	g.drawRect(getLocalBounds(), 1);

	if (copyMode)
	{
		g.setColour(Colour(SIGNAL_COLOUR));
		g.setFont(GLOBAL_BOLD_FONT().withHeight(28.0f));
		g.drawText("+", getLocalBounds().withTrimmedLeft(2).expanded(0, 4), Justification::topLeft);
	}
}

void ScriptingContentOverlay::Dragger::mouseDown(const MouseEvent& e)
{
	auto parent = dynamic_cast<ScriptingContentOverlay*>(getParentComponent());

	constrainer.setStartPosition(getBounds());

	if (e.eventComponent == this && draggedComponent.getComponent() != nullptr)
	{
		snapShot = draggedComponent->createComponentSnapshot(draggedComponent->getLocalBounds());

		startBounds = getBounds();

		dragger.startDraggingComponent(this, e);
	}

	if (e.mods.isRightButtonDown())
	{
		parent->getScriptComponentEditBroadcaster()->clearSelection(sendNotification);
	}
}

void ScriptingContentOverlay::Dragger::mouseDrag(const MouseEvent& e)
{
	constrainer.setRasteredMovement(e.mods.isCommandDown());
	constrainer.setLockedMovement(e.mods.isShiftDown());

	copyMode = e.mods.isAltDown();

	if (e.eventComponent == this)
		dragger.dragComponent(this, e, &constrainer);
}

void ScriptingContentOverlay::Dragger::mouseUp(const MouseEvent& e)
{
	snapShot = Image();

	Rectangle<int> newBounds = getBounds();

	int deltaX = newBounds.getX() - startBounds.getX();
	int deltaY = newBounds.getY() - startBounds.getY();

	if (copyMode)
	{
		duplicateSelection(deltaX, deltaY);
		return;
	}

	repaint();

	const bool wasResized = newBounds.getWidth() != startBounds.getWidth() || newBounds.getHeight() != startBounds.getHeight();

	if (wasResized)
	{
		resizeOverlayedComponent(newBounds.getWidth(), newBounds.getHeight());
	}
	else
	{
		

		moveOverlayedComponent(deltaX, deltaY);
	}
}

void ScriptingContentOverlay::Dragger::moveOverlayedComponent(int deltaX, int deltaY)
{
	auto b = dynamic_cast<ScriptComponentEditListener*>(getParentComponent())->getScriptComponentEditBroadcaster();

	static const Identifier x("x");
	static const Identifier y("y");
	static const Identifier pos("position");

	String sizeString = "[" + String(deltaX) + ", " + String(deltaY) + "]";

	auto tName = ScriptComponentEditBroadcaster::getTransactionName(sc, pos, var(sizeString));

	b->getUndoManager().beginNewTransaction(tName);

	b->setScriptComponentPropertyDeltaForSelection(x, deltaX, sendNotification, false);
	b->setScriptComponentPropertyDeltaForSelection(y, deltaY, sendNotification, false);
}

void ScriptingContentOverlay::Dragger::resizeOverlayedComponent(int newWidth, int newHeight)
{
	auto b = dynamic_cast<ScriptComponentEditListener*>(getParentComponent())->getScriptComponentEditBroadcaster();

	static const Identifier width("width");
	static const Identifier height("height");
	static const Identifier size("size");

	String sizeString = "[" + String(newWidth) + ", " + String(newHeight) + "]";

	auto tName = ScriptComponentEditBroadcaster::getTransactionName(sc, size, var(sizeString));

	b->getUndoManager().beginNewTransaction(tName);

	b->setScriptComponentProperty(sc, width, newWidth, sendNotification, false);
	b->setScriptComponentProperty(sc, height, newHeight, sendNotification, false);
}


void ScriptingContentOverlay::Dragger::duplicateSelection(int deltaX, int deltaY)
{
	auto b = dynamic_cast<ScriptComponentEditListener*>(getParentComponent())->getScriptComponentEditBroadcaster();

	auto content = b->getFirstFromSelection()->parent;

	ScriptingApi::Content::Helpers::duplicateSelection(content, b->getSelection(), deltaX, deltaY);
	
}

void ScriptingContentOverlay::Dragger::MovementWatcher::componentMovedOrResized(bool /*wasMoved*/, bool wasResized)
{
	auto c = getComponent()->findParentComponentOfClass<ScriptContentComponent>();

	if (c != nullptr)
	{
		auto boundsInParent = c->getLocalArea(getComponent()->getParentComponent(), getComponent()->getBoundsInParent());
		dragComponent->setBounds(boundsInParent);
	}
}
