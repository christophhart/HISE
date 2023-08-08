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



namespace hise { using namespace juce;

namespace OverlayIcons
{
	
	namespace data
	{
		static const unsigned char lockShape[] = { 110,109,41,100,31,68,33,48,94,67,98,156,188,33,68,33,48,94,67,248,163,35,68,211,205,101,67,248,163,35,68,92,47,111,67,108,248,163,35,68,223,111,184,67,98,248,163,35,68,164,32,189,67,139,188,33,68,125,239,192,67,41,100,31,68,125,239,192,67,108,37,182,
		213,67,125,239,192,67,98,96,5,209,67,125,239,192,67,135,54,205,67,164,32,189,67,135,54,205,67,223,111,184,67,108,135,54,205,67,92,47,111,67,98,135,54,205,67,211,205,101,67,96,5,209,67,33,48,94,67,37,182,213,67,33,48,94,67,108,41,100,31,68,33,48,94,67,
		99,109,166,91,248,67,68,11,76,67,108,166,171,219,67,68,11,76,67,108,166,171,219,67,160,186,20,67,108,137,129,219,67,160,186,20,67,108,137,129,219,67,184,126,20,67,98,137,129,219,67,254,20,196,66,172,252,239,67,229,80,100,66,84,155,4,68,229,80,100,66,
		98,98,56,17,68,229,80,100,66,227,117,27,68,254,20,196,66,227,117,27,68,184,126,20,67,108,227,117,27,68,160,186,20,67,108,49,112,27,68,160,186,20,67,108,49,112,27,68,193,234,76,67,108,41,28,13,68,193,234,76,67,108,41,28,13,68,160,186,20,67,108,229,24,
		13,68,160,186,20,67,98,229,24,13,68,168,166,20,67,246,24,13,68,176,146,20,67,246,24,13,68,184,126,20,67,98,246,24,13,68,0,192,1,67,242,74,9,68,98,16,229,66,84,155,4,68,98,16,229,66,98,35,235,255,67,98,16,229,66,133,91,248,67,66,128,1,67,231,59,248,67,
		180,8,20,67,108,166,91,248,67,180,8,20,67,108,166,91,248,67,68,11,76,67,99,101,0,0 };

		static const unsigned char penShape[] = { 110,109,96,69,112,67,182,243,141,64,108,154,73,133,67,143,194,240,65,98,158,95,136,67,201,118,16,66,59,111,136,67,92,15,56,66,172,108,133,67,125,191,80,66,108,51,179,122,67,100,123,137,66,108,240,7,74,67,172,28,170,65,108,20,46,90,67,82,184,150,64,98,
			51,51,96,67,12,2,187,191,88,25,106,67,131,192,202,191,96,69,112,67,182,243,141,64,99,109,14,173,62,67,164,240,1,66,108,113,29,111,67,213,120,159,66,108,127,42,171,66,0,32,109,67,108,117,147,20,66,190,223,61,67,108,14,173,62,67,164,240,1,66,99,109,236,
			81,200,65,121,9,75,67,108,123,148,145,66,53,158,121,67,108,0,0,0,0,74,60,138,67,108,236,81,200,65,121,9,75,67,99,101,0,0 };
	}

	DEFINE_DATA(lockShape, 393);
	DEFINE_DATA(penShape, 183);
};

ScriptEditHandler::ScriptEditHandler()
{

}

void ScriptEditHandler::createNewComponent(ComponentType componentType, int x, int y, ScriptComponent* parent/*=nullptr*/)
{
	if (getScriptEditHandlerContent() == nullptr)
		return;

	String componentName;

	switch (componentType)
	{
	case ComponentType::Knob:				componentName = "Knob"; break;
	case ComponentType::Button:				componentName = "Button"; break;
	case ComponentType::Table:				componentName = "Table"; break;
	case ComponentType::ComboBox:			componentName = "ComboBox"; break;
	case ComponentType::Label:				componentName = "Label"; break;
	case ComponentType::Image:				componentName = "Image"; break;
	case ComponentType::Viewport:			componentName = "Viewport"; break;
	case ComponentType::Panel:				componentName = "Panel"; break;
	case ComponentType::AudioWaveform:		componentName = "AudioWaveform"; break;
	case ComponentType::SliderPack:			componentName = "SliderPack"; break;
	case ComponentType::WebView:			componentName = "WebView"; break;
	case ComponentType::FloatingTile:		componentName = "FloatingTile"; break;
	case ComponentType::duplicateComponent:
	{
		auto b = getScriptEditHandlerOverlay()->getScriptComponentEditBroadcaster();

		auto sc = b->getFirstFromSelection();

		componentName = sc->getObjectName().toString();
		componentName = componentName.replace("Scripted", "");
		componentName = componentName.replace("Script", "");
		componentName = componentName.replace("Slider", "Knob");
		break;
	}
	case ComponentType::numComponentTypes: break;
	}

	auto content = getScriptEditHandlerProcessor()->getContent();

	Identifier id = ScriptingApi::Content::Helpers::getUniqueIdentifier(content, componentName);

	ScriptComponent::Ptr newComponent;

	switch (componentType)
	{
	case hise::ScriptEditHandler::ComponentType::Knob: 
		newComponent = content->createNewComponent<ScriptingApi::Content::ScriptSlider>(id, x, y);
		break;
	case hise::ScriptEditHandler::ComponentType::Button:
		newComponent = content->createNewComponent<ScriptingApi::Content::ScriptButton>(id, x, y);
		break;
	case hise::ScriptEditHandler::ComponentType::Table:
		newComponent = content->createNewComponent<ScriptingApi::Content::ScriptTable>(id, x, y);
		break;
	case hise::ScriptEditHandler::ComponentType::ComboBox:
		newComponent = content->createNewComponent<ScriptingApi::Content::ScriptComboBox>(id, x, y);
		break;
	case hise::ScriptEditHandler::ComponentType::Label:
		newComponent = content->createNewComponent<ScriptingApi::Content::ScriptLabel>(id, x, y);
		break;
	case hise::ScriptEditHandler::ComponentType::Image:
		newComponent = content->createNewComponent<ScriptingApi::Content::ScriptImage>(id, x, y);
		break;
	case hise::ScriptEditHandler::ComponentType::Viewport:
		newComponent = content->createNewComponent<ScriptingApi::Content::ScriptedViewport>(id, x, y);
		break;
	case hise::ScriptEditHandler::ComponentType::Panel:
		newComponent = content->createNewComponent<ScriptingApi::Content::ScriptPanel>(id, x, y);
		break;
	case hise::ScriptEditHandler::ComponentType::AudioWaveform:
		newComponent = content->createNewComponent<ScriptingApi::Content::ScriptAudioWaveform>(id, x, y);
		break;
	case hise::ScriptEditHandler::ComponentType::SliderPack:
		newComponent = content->createNewComponent<ScriptingApi::Content::ScriptSliderPack>(id, x, y);
		break;
	case hise::ScriptEditHandler::ComponentType::WebView:
		newComponent = content->createNewComponent<ScriptingApi::Content::ScriptWebView>(id, x, y);
		break;
	case hise::ScriptEditHandler::ComponentType::FloatingTile:
		newComponent = content->createNewComponent<ScriptingApi::Content::ScriptFloatingTile>(id, x, y);
		break;
	case hise::ScriptEditHandler::ComponentType::duplicateComponent:
		jassertfalse;
		break;
	case hise::ScriptEditHandler::ComponentType::numComponentTypes:
		jassertfalse;
		break;
	default:
		break;
	}

	auto b = content->getScriptProcessor()->getMainController_()->getScriptComponentEditBroadcaster();

	if (parent != nullptr && newComponent != nullptr)
	{
		newComponent->getPropertyValueTree().setProperty("parentComponent", parent->getName().toString(), nullptr);
	}

	b->setSelection(content->getComponentWithName(id));

}

void ScriptEditHandler::toggleComponentSelectMode(bool shouldSelectOnClick)
{
	useComponentSelectMode = shouldSelectOnClick;

	getScriptEditHandlerContent()->setInterceptsMouseClicks(false, !useComponentSelectMode);
}

void ScriptEditHandler::compileScript()
{
	Processor* p = dynamic_cast<Processor*>(getScriptEditHandlerProcessor());
	
	PresetHandler::setChanged(p);

	scriptEditHandlerCompileCallback();
}


String ScriptEditHandler::isValidComponentName(const String &id)
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
	handler(handler_),
	smw(*this)
{
	addAsScriptEditListener();

	addAndMakeVisible(dragModeButton = new ShapeButton("Drag Mode", Colours::black.withAlpha(0.6f), Colours::black.withAlpha(0.8f), Colours::black.withAlpha(0.8f)));

	lasso.setColour(LassoComponent<ScriptComponent*>::ColourIds::lassoFillColourId, Colours::white.withAlpha(0.1f));
	lasso.setColour(LassoComponent<ScriptComponent*>::ColourIds::lassoOutlineColourId, Colours::white.withAlpha(0.4f));

	lasso.setLookAndFeel(&llaf);

	Path path;
	path.loadPathFromData(OverlayIcons::lockShape, SIZE_OF_PATH(OverlayIcons::lockShape));

	dragModeButton->setShape(path, true, true, false);

	dragModeButton->addListener(this);

	dragModeButton->setTooltip("Toggle between Edit / Performance mode");

	setEditMode(handler->editModeEnabled());

	setWantsKeyboardFocus(true);

	auto p = dynamic_cast<Processor*>(handler->getScriptEditHandlerProcessor());
	enableMouseDragging = GET_HISE_SETTING(p, HiseSettings::Scripting::EnableMousePositioning);
}


ScriptingContentOverlay::~ScriptingContentOverlay()
{
	removeAsScriptEditListener();
}

void ScriptingContentOverlay::resized()
{
	if (auto fc = findParentComponentOfClass<ZoomableViewport>())
	{
		fc->addMouseListener(this, false);
		
	}

	dragModeButton->setBounds(getWidth() - 28, 12, 16, 16);
}


void ScriptingContentOverlay::buttonClicked(Button* /*buttonThatWasClicked*/)
{
	toggleEditMode();

}

void ScriptingContentOverlay::toggleEditMode()
{
	setEditMode(!dragMode);

	if (!dragMode)
		getScriptComponentEditBroadcaster()->setLearnMode(false);

	handler->toggleComponentSelectMode(dragMode);
}

bool ScriptingContentOverlay::isEditModeEnabled() const
{
	return dragMode;
}

void ScriptingContentOverlay::setEditMode(bool editModeEnabled)
{
	dragMode = editModeEnabled;

	Path p;

	if (dragMode == false)
	{
		p.loadPathFromData(OverlayIcons::lockShape, SIZE_OF_PATH(OverlayIcons::lockShape));
		clearDraggers();
		setInterceptsMouseClicks(false, true);
	}
	else
	{
		p.loadPathFromData(OverlayIcons::penShape, SIZE_OF_PATH(OverlayIcons::penShape));
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
        const bool isInPopup = findParentComponentOfClass<ScriptingEditor>() == nullptr;

		Colour lineColour = isInPopup ? Colours::white : Colours::black;

        GlobalHiseLookAndFeel::draw1PixelGrid(g, this, getLocalBounds(), lineColour);
	}

	if (dragModeButton->isVisible())
	{
		Colour c = Colours::white;

		g.setColour(c.withAlpha(0.2f));

		g.fillRoundedRectangle(getFloatRectangle(dragModeButton->getBounds().expanded(3)), 3.0f);
	}

	if (isDisabledUntilUpdate)
	{
		g.setColour(Colours::red.withAlpha(0.08f));
		g.fillAll();
		g.setColour(Colours::white.withAlpha(0.1f));
		g.setFont(GLOBAL_BOLD_FONT().withHeight(24.0f));
		g.drawText("DISABLED UNTIL UPDATE (Press F5)", getLocalBounds(), Justification::centred);
	}
}




void ScriptingContentOverlay::scriptComponentSelectionChanged()
{
	clearDraggers();

	ScriptComponentEditBroadcaster::Iterator iter(getScriptComponentEditBroadcaster());

	auto content = handler->getScriptEditHandlerContent();
	auto singleSelection = getScriptComponentEditBroadcaster()->getNumSelected() == 1;

	getScriptComponentEditBroadcaster()->setCurrentlyLearnedComponent(nullptr);

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

		getScriptComponentEditBroadcaster()->getLearnBroadcaster().addListener(*d, Dragger::learnComponentChanged);

		if (singleSelection && getScriptComponentEditBroadcaster()->learnModeEnabled())
			getScriptComponentEditBroadcaster()->setCurrentlyLearnedComponent(c);

		addAndMakeVisible(d);

		draggers.add(d);

		auto boundsInParent = content->getLocalArea(draggedComponent->getParentComponent(), draggedComponent->getBoundsInParent());

		d->setBounds(boundsInParent);
	}
}


void ScriptingContentOverlay::scriptComponentPropertyChanged(ScriptComponent* /*sc*/, Identifier /*idThatWasChanged*/, const var& /*newValue*/)
{
	
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
	}
	else if ((keyCode == 'Z' || keyCode == 'z') && key.getModifiers().isCommandDown())
	{
		b->getUndoManager().undo();
		return true;
	}
	else if (TopLevelWindowWithKeyMappings::matches(this, key, InterfaceDesignerShortcuts::id_duplicate))
	{
		if (draggers.size() > 0)
		{
			auto pwsc = dynamic_cast<ProcessorWithScriptingContent*>(handler->getScriptEditHandlerProcessor());

			auto start = draggers.getFirst()->getPosition();

			auto end = getMouseXYRelative();

			auto deltaX = end.x - start.x;
			auto deltaY = end.y - start.y;

			if (key.getModifiers().isShiftDown())
			{
				deltaX = 0;
				deltaY = 0;
			}

			ScriptingApi::Content::Helpers::duplicateSelection(pwsc->getScriptingContent(), b->getSelection(), deltaX, deltaY, &b->getUndoManager());
		}

		return true;
	}
	else if (TopLevelWindowWithKeyMappings::matches(this, key, InterfaceDesignerShortcuts::id_show_json))
	{
		getScriptComponentEditBroadcaster()->showJSONEditor(this);
		return true;
	}
    else if (TopLevelWindowWithKeyMappings::matches(this, key, InterfaceDesignerShortcuts::id_show_panel_data_json))
    {
        return getScriptComponentEditBroadcaster()->showPanelDataJSON(this);
    }
	else if ((keyCode == 'C' || keyCode == 'c') && key.getModifiers().isCommandDown())
	{
		auto s = ScriptingApi::Content::Helpers::createScriptVariableDeclaration(b->getSelection());
		SystemClipboard::copyTextToClipboard(s);
		return true;
	}
	else if (keyCode == KeyPress::deleteKey || keyCode == KeyPress::backspaceKey)
	{
		auto pwsc = dynamic_cast<ProcessorWithScriptingContent*>(handler->getScriptEditHandlerProcessor());

		ScriptingApi::Content::Helpers::deleteSelection(pwsc->getScriptingContent(), b);

		return true;
	}
	
	return false;
}

bool isParent(ScriptComponent* cp, ScriptComponent* possibleParent)
{
	if (cp == nullptr)
		return false;

	auto thisParent = cp->getParentScriptComponent();

	if (thisParent == possibleParent)
		return true;

	return isParent(thisParent, possibleParent);
}

void ScriptingContentOverlay::findLassoItemsInArea(Array<ScriptComponent*> &itemsFound, const Rectangle< int > &area)
{
	auto a = getLocalArea(lasso.getParentComponent(), area);

	itemsFound.clearQuick();

	auto content = handler->getScriptEditHandlerContent();
	content->getScriptComponentsFor(itemsFound, a);

	auto b = getScriptComponentEditBroadcaster();

	

	ScriptComponentSelection newSelection;
	
	for (int i = 0; i < itemsFound.size(); i++)
	{
		auto sc = itemsFound[i];

		for (int j = 0; j < itemsFound.size(); j++)
		{
			if (isParent(sc, itemsFound[j]))
			{
				itemsFound.remove(i--);
				break;
			}
		}
	}

	for (auto i : itemsFound)
	{
		newSelection.addIfNotAlreadyThere(i);
	}
		

	b->setSelection(newSelection, sendNotificationAsync);
}

void ScriptingContentOverlay::mouseDown(const MouseEvent& e)
{
	CHECK_MIDDLE_MOUSE_DOWN(e);
}

void ScriptingContentOverlay::mouseUp(const MouseEvent &e)
{
	CHECK_MIDDLE_MOUSE_UP(e);
		
	if (isDisabledUntilUpdate)
		return;

	if (lasso.isVisible())
	{
		lasso.setVisible(false);
		lasso.endLasso();
	}
	else
	{
		auto content = handler->getScriptEditHandlerContent();
		auto processor = dynamic_cast<Processor*>(handler->getScriptEditHandlerProcessor());
		auto b = getScriptComponentEditBroadcaster();

		if (e.mods.isRightButtonDown())
		{
			enum ComponentOffsets
			{
				createCallbackDefinition = 10000,
                createLocalLookAndFeel,
				addDefinition,
				DeleteSelection,
				showCallback,
				showDefinition,
				showLookAndFeel,
				restoreToData,
				copySnapshot,
				toggleLearnMode,
				editComponentOffset = 20000,

			};

			ScopedPointer<PopupLookAndFeel> luf = new PopupLookAndFeel();
			PopupMenu m;
			
			m.setLookAndFeel(luf);

			m.addSectionHeader("Create new Component");
			m.addItem((int)ScriptEditHandler::ComponentType::Knob, "Add new Slider");
			m.addItem((int)ScriptEditHandler::ComponentType::Button, "Add new Button");
			m.addItem((int)ScriptEditHandler::ComponentType::Table, "Add new Table");
			m.addItem((int)ScriptEditHandler::ComponentType::ComboBox, "Add new ComboBox");
			m.addItem((int)ScriptEditHandler::ComponentType::Label, "Add new Label");
			m.addItem((int)ScriptEditHandler::ComponentType::Image, "Add new Image");
			m.addItem((int)ScriptEditHandler::ComponentType::Viewport, "Add new Viewport");
			m.addItem((int)ScriptEditHandler::ComponentType::Panel, "Add new Panel");
			m.addItem((int)ScriptEditHandler::ComponentType::AudioWaveform, "Add new AudioWaveform");
			m.addItem((int)ScriptEditHandler::ComponentType::SliderPack, "Add new SliderPack");
			m.addItem((int)ScriptEditHandler::ComponentType::WebView, "Add new WebView");
			m.addItem((int)ScriptEditHandler::ComponentType::FloatingTile, "Add new FloatingTile");

			auto components = b->getSelection();

			if (components.size() != 0)
			{
				m.addSeparator();

				m.addItem(DeleteSelection, "Delete Selection");

				m.addSeparator();

				m.addItem(addDefinition, "Create script reference for selection");
				m.addItem(createCallbackDefinition, "Create custom callback for selection");
                m.addItem(createLocalLookAndFeel, "Create local LookAndFeel for selection");

				auto first = components.getFirst();

				

				bool learnable = b->getNumSelected() == 1;

				Identifier id = learnable ? b->getFirstFromSelection()->getObjectName() : Identifier();

				learnable |= (id == ScriptingApi::Content::ScriptSlider::getStaticObjectName());
				learnable |= (id == ScriptingApi::Content::ScriptButton::getStaticObjectName());
				learnable |= (id == ScriptingApi::Content::ScriptComboBox::getStaticObjectName());
				learnable |= (id == ScriptingApi::Content::ScriptPanel::getStaticObjectName());
				
				m.addItem(toggleLearnMode, "Enable Connection Learn", learnable, b->getCurrentlyLearnedComponent() == b->getFirstFromSelection());

				m.addSeparator();

				m.addItem(showDefinition, "Goto first definition of " + first->getName().toString(), true);
				m.addItem(showLookAndFeel, "Goto LookAndFeel for " + first->getName().toString(), first->getLookAndFeelObject().isObject());
				m.addItem(showCallback, "Goto callback for " + first->getName().toString(), first->getCustomControlCallback() != nullptr);
			}

			auto first = components.getFirst();

			int result = m.show();

			if (result == createCallbackDefinition)
			{
				auto code = ScriptingApi::Content::Helpers::createCustomCallbackDefinition(components);

				debugToConsole(processor, String(components.size()) + " callback definitions created and copied to the clipboard");

				SystemClipboard::copyTextToClipboard(code);
			}
            else if (result == createLocalLookAndFeel)
            {
                auto code = ScriptingApi::Content::Helpers::createLocalLookAndFeelForComponents(components);
                
                debugToConsole(processor, "local look and feel was copied to clipboard");
                
                SystemClipboard::copyTextToClipboard(code);
            }
			else if (result == addDefinition)
			{
				auto code = ScriptingApi::Content::Helpers::createScriptVariableDeclaration(components);

				debugToConsole(processor, String(components.size()) + " script component definitions created and copied to the clipboard");

				SystemClipboard::copyTextToClipboard(code);
			}
			else if (result == toggleLearnMode)
			{
				b->setCurrentlyLearnedComponent(b->getFirstFromSelection());
			}
			else if (result >= (int)ScriptEditHandler::ComponentType::Knob && result < (int)ScriptEditHandler::ComponentType::numComponentTypes)
			{
				int insertX = e.getEventRelativeTo(content).getMouseDownPosition().getX();
				int insertY = e.getEventRelativeTo(content).getMouseDownPosition().getY();

				insertX = jlimit(0, getLocalBounds().getWidth() - 100, insertX);
				insertY = jlimit(0, getLocalBounds().getWidth() - 50, insertY);

				auto parent = b->getNumSelected() == 1 ? b->getFirstFromSelection() : nullptr;

				if (parent != nullptr)
				{
					auto parentBounds = ApiHelpers::getRectangleFromVar(parent->getLocalBounds(0)).toNearestInt();

					if (!parentBounds.contains(insertX, insertY ))
						parent = nullptr;
					else if (auto d = draggers.getFirst())
					{
						auto bounds = d->getLocalArea(this, d->getLocalBounds());

						insertX += bounds.getX();
						insertY += bounds.getY();

						insertX = jlimit(0, parentBounds.getWidth() - 100, insertX);
						insertY = jlimit(0, parentBounds.getWidth() - 50, insertY);
					}
				}

				handler->createNewComponent((ScriptEditHandler::ComponentType)result, insertX, insertY, parent);
			}
			else if (result == DeleteSelection)
			{
				auto pwsc = dynamic_cast<ProcessorWithScriptingContent*>(handler->getScriptEditHandlerProcessor());

				ScriptingApi::Content::Helpers::deleteSelection(pwsc->getScriptingContent(), b);
			}
			else if (result == showDefinition)
			{
				auto pwsc = dynamic_cast<ProcessorWithScriptingContent*>(handler->getScriptEditHandlerProcessor());

				pwsc->getScriptingContent()->recompileAndThrowAtDefinition(first.get());
			}
			else if (result == showLookAndFeel)
			{
				if (auto obj = dynamic_cast<DebugableObjectBase*>(first->getLookAndFeelObject().getObject()))
				{
					DebugInformation::Ptr d = new DebugableObjectInformation(obj, "unused", DebugInformation::Type::Constant);
					d->doubleClickCallback(e, this);
				}
			}
			else if (result == showCallback)
			{
				if (auto obj = dynamic_cast<DebugableObjectBase*>(first->getCustomControlCallback()))
				{
					DebugInformation::Ptr d = new DebugableObjectInformation(obj, "unused", DebugInformation::Type::Constant);
					d->doubleClickCallback(e, this);
				}
			}
			else if (result >= editComponentOffset) // EDIT IN PANEL
			{
				auto sc = components[result - editComponentOffset];

				getScriptComponentEditBroadcaster()->updateSelectionBasedOnModifier(sc.get(), e.mods, sendNotification);
			}
		}
		else
		{
			Array<ScriptingApi::Content::ScriptComponent*> components;

			content->getScriptComponentsFor(components, e.getEventRelativeTo(content).getPosition());

			ScriptComponent* sc = nullptr;

			for (auto sc_ : components)
			{
				if (!sc_->isShowing())
					continue;

				sc = sc_;
				break;
			}
			

			if (sc == nullptr)
				b->clearSelection();
			else
				b->updateSelectionBasedOnModifier(sc, e.mods);
			
		}
	}
}


void ScriptingContentOverlay::mouseDrag(const MouseEvent& e)
{
	CHECK_MIDDLE_MOUSE_DRAG(e);
		
	if (isDisabledUntilUpdate)
		return;

	if (lasso.isVisible())
	{
		lasso.dragLasso(e);
	}
	else
	{
		if (e.mouseWasDraggedSinceMouseDown() && isEditModeEnabled())
		{
			lassoSet.deselectAll();

			e.eventComponent->addAndMakeVisible(lasso);
			lasso.beginLasso(e, this);
		}
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

	addAndMakeVisible(rEdge = new ResizableEdgeComponent(this, &constrainer, ResizableEdgeComponent::rightEdge));
	addAndMakeVisible(bEdge = new ResizableEdgeComponent(this, &constrainer, ResizableEdgeComponent::bottomEdge));
	
	rEdge->addMouseListener(this, true);
	bEdge->addMouseListener(this, true);

	resizer->setAlpha(0.0f);
	rEdge->setAlpha(0.0f);
	bEdge->setAlpha(0.0f);

	

	setVisible(true);
	setWantsKeyboardFocus(true);

	setAlwaysOnTop(true);
}

ScriptingContentOverlay::Dragger::~Dragger()
{
	
}

void ScriptingContentOverlay::Dragger::paint(Graphics &g)
{
	g.fillAll(Colours::black.withAlpha(0.2f));
	g.setColour(Colour(SIGNAL_COLOUR).withAlpha(0.5f));

	if (!snapShot.isNull())
		g.drawImage(snapShot, getLocalBounds().toFloat());

	UnblurryGraphics ug(g, *this, true);

	ug.draw1PxRect(getLocalBounds().toFloat());

	if (copyMode)
	{
		g.setColour(Colour(SIGNAL_COLOUR));
		g.setFont(GLOBAL_BOLD_FONT().withHeight(28.0f));
		g.drawText("+", getLocalBounds().withTrimmedLeft(2).expanded(0, 4), Justification::topLeft);
	}

	if (learnModeEnabled)
	{
		g.setColour(Colours::black.withAlpha(0.2f));
		g.fillRect(getLocalBounds().reduced(1));
		Learnable::Factory f;
		auto p = f.createPath("source");
		f.scalePath(p, getLocalBounds().reduced(3).removeFromRight(28).removeFromTop(18).reduced(2).toFloat());
		g.setColour(Colour(SIGNAL_COLOUR));
		g.fillPath(p);
	}
}

void ScriptingContentOverlay::Dragger::mouseDown(const MouseEvent& e)
{
	CHECK_MIDDLE_MOUSE_DOWN(e);

	if (e.mods.isLeftButtonDown())
	{
		auto parent = dynamic_cast<ScriptingContentOverlay*>(getParentComponent());

		if (!parent->enableMouseDragging)
			return;

		constrainer.setStartPosition(getBounds());
		startBounds = getBounds();

		if (e.eventComponent == this && draggedComponent.getComponent() != nullptr)
		{
			setUseSnapShot(true);
			

			dragger.startDraggingComponent(this, e);
			parent->smw.startDragging(this);
		}
		
	}
}

void ScriptingContentOverlay::Dragger::mouseDrag(const MouseEvent& e)
{
	CHECK_MIDDLE_MOUSE_DRAG(e);

	if (e.mods.isRightButtonDown() || e.mods.isMiddleButtonDown())
		return;

	auto parent = dynamic_cast<ScriptingContentOverlay*>(getParentComponent());

	if (!parent->enableMouseDragging)
	{
		static const unsigned char pathData[] = { 110,109,0,128,38,68,192,255,63,67,98,192,156,55,68,192,255,63,67,0,128,69,68,240,143,119,67,0,128,69,68,32,0,158,67,98,0,128,69,68,160,57,192,67,192,156,55,68,40,0,220,67,0,128,38,68,40,0,220,67,98,12,100,21,68,40,0,220,67,2,128,7,68,160,57,192,67,2,
128,7,68,32,0,158,67,98,2,128,7,68,240,143,119,67,12,100,21,68,192,255,63,67,0,128,38,68,192,255,63,67,99,109,132,215,55,68,72,73,136,67,108,12,164,27,68,40,175,192,67,98,238,201,30,68,24,162,196,67,160,130,34,68,64,235,198,67,0,128,38,68,64,235,198,
67,98,116,202,49,68,64,235,198,67,140,245,58,68,8,149,180,67,140,245,58,68,32,0,158,67,98,140,245,58,68,96,5,150,67,252,208,57,68,8,149,142,67,132,215,55,68,72,73,136,67,99,109,220,22,21,68,136,125,179,67,108,180,62,49,68,48,91,118,67,98,92,31,46,68,
160,159,110,67,84,112,42,68,0,41,106,67,0,128,38,68,0,41,106,67,98,76,53,27,68,0,41,106,67,48,10,18,68,64,107,135,67,48,10,18,68,32,0,158,67,98,48,10,18,68,200,224,165,67,248,39,19,68,216,62,173,67,220,22,21,68,136,125,179,67,99,101,0,0 };

		Path path;
		path.loadPathFromData(pathData, sizeof(pathData));

		

		int size = 22;

		Image img(Image::PixelFormat::ARGB, size, size, true);
		
		Graphics g(img);

		PathFactory::scalePath(path, { 1.0f, 1.0f, (float)size-2.0f, (float)size-2.0f });
		g.setColour(Colour(0xFFEEEEEE));
		g.fillPath(path);
		g.setColour(Colours::black.withAlpha(0.5f));
		g.strokePath(path, PathStrokeType(1.0f));

		MouseCursor c(img, size / 2, size / 2);

		setMouseCursor(c);
		return;
	}
		

    if(e.mouseWasDraggedSinceMouseDown())
        constrainer.setRasteredMovement(!e.mods.isCommandDown());
    
	constrainer.setLockedMovement(e.mods.isShiftDown());

	

	copyMode = e.mods.isAltDown();

	setMouseCursor(copyMode ? MouseCursor::CopyingCursor : MouseCursor::DraggingHandCursor);

    dragDistance = constrainer.getPosition().getTopLeft() - startBounds.getTopLeft();
    
	if (e.eventComponent == this)
		dragger.dragComponent(this, e, &constrainer);

	

	if (e.eventComponent != this)
	{
		auto newBounds = getLocalBounds();
		resizeOverlayedComponent(newBounds.getWidth(), newBounds.getHeight());
	}

}

void ScriptingContentOverlay::Dragger::mouseUp(const MouseEvent& e)
{
	CHECK_MIDDLE_MOUSE_UP(e);

	setMouseCursor(MouseCursor::NormalCursor);

	auto parent = dynamic_cast<ScriptingContentOverlay*>(getParentComponent());

	if (!e.mouseWasDraggedSinceMouseDown())
	{
		if (e.mods.isShiftDown())
		{
			parent->getScriptComponentEditBroadcaster()->clearSelection();
			return;
		}

		if (e.mods.isCommandDown())
		{
			parent->getScriptComponentEditBroadcaster()->updateSelectionBasedOnModifier(sc, e.mods, sendNotification);
			return;
		}
	}

	if (e.mods.isRightButtonDown())
	{
		getParentComponent()->mouseUp(e);
		return;
	}

	if (!parent->enableMouseDragging)
		return;

	findParentComponentOfClass<ScriptingContentOverlay>()->smw.endDragging();

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

hise::MarkdownLink ScriptingContentOverlay::Dragger::getLink() const
{
	if (sc != nullptr)
	{
		auto objName = sc->getObjectName().toString();
		objName = objName.replace("Scripting", "");
		objName = objName.replace("Scripted", "");
		objName = objName.replace("Script", "");

		if (objName == "Slider")
			objName = "Knob";
		else if (objName == "AudioWaveform")
			objName = "audio-waveform";
		else if (objName == "FloatingTile")
			objName = "floating-tile";


		String url = "ui-components/plugin-components/" + MarkdownLink::Helpers::getSanitizedFilename(objName);

		return MarkdownLink(File(), url);
	}

	return {};
}

void ScriptingContentOverlay::Dragger::setUseSnapShot(bool shouldUseSnapShot)
{
	if (shouldUseSnapShot)
	{
		auto sf = UnblurryGraphics::getScaleFactorForComponent(this);
		snapShot = draggedComponent->createComponentSnapshot(draggedComponent->getLocalBounds(), true, sf);
	}
	else
	{
		snapShot = {};
	}

	repaint();
}



void ScriptingContentOverlay::Dragger::moveOverlayedComponent(int deltaX, int deltaY)
{
	auto b = dynamic_cast<ScriptComponentEditListener*>(getParentComponent())->getScriptComponentEditBroadcaster();

	static const Identifier x("x");
	static const Identifier y("y");
	static const Identifier pos("position");

	String sizeString = "[" + String(deltaX) + ", " + String(deltaY) + "]";

	auto tName = "Position update: " + sizeString;

	b->setScriptComponentPropertyDeltaForSelection(x, deltaX, sendNotification, false);
	b->setScriptComponentPropertyDeltaForSelection(y, deltaY, sendNotification, false);
}

void ScriptingContentOverlay::Dragger::resizeOverlayedComponent(int newWidth, int newHeight)
{
	auto b = dynamic_cast<ScriptComponentEditListener*>(getParentComponent())->getScriptComponentEditBroadcaster();

	static const Identifier width("width");
	static const Identifier height("height");

	b->setScriptComponentProperty(sc, width, newWidth, sendNotification, false);
	b->setScriptComponentProperty(sc, height, newHeight, sendNotification, false);
}


void ScriptingContentOverlay::Dragger::duplicateSelection(int deltaX, int deltaY)
{
	auto b = dynamic_cast<ScriptComponentEditListener*>(getParentComponent())->getScriptComponentEditBroadcaster();

	if (auto first = b->getFirstFromSelection())
	{
		if (auto content = first->parent)
			ScriptingApi::Content::Helpers::duplicateSelection(content, b->getSelection(), deltaX, deltaY, &b->getUndoManager());
	}
}

void ScriptingContentOverlay::Dragger::learnComponentChanged(Dragger& d, ScriptComponent* newComponent)
{
	d.learnModeEnabled = newComponent != nullptr && d.sc == newComponent;
	d.repaint();
}

void ScriptingContentOverlay::Dragger::MovementWatcher::componentMovedOrResized(bool /*wasMoved*/, bool /*wasResized*/)
{
	auto c = getComponent()->findParentComponentOfClass<ScriptContentComponent>();

	if (c != nullptr)
	{
		auto boundsInParent = c->getLocalArea(getComponent()->getParentComponent(), getComponent()->getBoundsInParent());
		dragComponent->setBounds(boundsInParent);
	}
}


void ScriptingContentOverlay::LassoLaf::drawLasso(Graphics& g, Component& c)
{
	UnblurryGraphics ug(g, c, true);

	g.setColour(Colours::white.withAlpha(0.05f));

	ug.fillUnblurryRect(c.getLocalBounds().toFloat());

	g.setColour(Colours::white.withAlpha(0.2f));

	ug.draw1PxRect(c.getLocalBounds().toFloat());
}

} // namespace hise
