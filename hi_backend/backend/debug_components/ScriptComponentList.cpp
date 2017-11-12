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





#if 0
ScriptComponentList::ScriptComponentList(BackendRootWindow* window, JavascriptProcessor* p) :
	SearchableListComponent(window),
	ScriptComponentEditListener(dynamic_cast<Processor*>(p)),
	jp(p)
{
	p->getContent()->addRebuildListener(this);

	hideButton = new ShapeButton("hide", Colours::white.withAlpha(0.5f), Colours::white, Colours::white);


	addAndMakeVisible(hideButton);
	Path hidePath;

	hidePath.loadPathFromData(BackendBinaryData::ToolbarIcons::viewPanel, sizeof(BackendBinaryData::ToolbarIcons::viewPanel));
	hideButton->setShape(hidePath, false, true, true);
	hideButton->addListener(this);

	addCustomButton(hideButton);

	window->getBackendProcessor()->addScriptListener(this);
	addAsScriptEditListener();

	setOpaque(true);
	setName("Script Component Browser");

	setFuzzyness(0.6);
}

ScriptComponentList::~ScriptComponentList()
{
	jp->getContent()->removeRebuildListener(this);

	dynamic_cast<Processor*>(jp)->getMainController()->removeScriptListener(this);
	removeAsScriptEditListener();
}

void ScriptComponentList::buttonClicked(Button* b)
{
	showOnlyVisibleItems = !b->getToggleState();

	b->setToggleState(showOnlyVisibleItems, dontSendNotification);

	Colour c = showOnlyVisibleItems ? Colour(SIGNAL_COLOUR) : Colours::white;
	
	hideButton->setColours(c.withAlpha(0.5f), c, c);

	rebuildModuleList(true);
}

void getChildDepth(const ValueTree& data, int& depth)
{
	static const Identifier co("Component");

	if (data.getParent().getType() == co)
	{
		depth++;

		getChildDepth(data.getParent(), depth);
	}
}




void getSearchTermInternal(ValueTree& v, String& term)
{
	static const Identifier co("Component");
	static const Identifier id_("id");

	term << v.getProperty(id_).toString();

	if (v.getParent().getType() == co)
	{
		getSearchTermInternal(v.getParent(), term);
	}
}


String getSearchTerm(ScriptingApi::Content* content, const Identifier& id)
{
	String term;

	auto v = content->getValueTreeForComponent(id);

	getSearchTermInternal(v, term);

	return term.toLowerCase();

}


ScriptComponentList::ScriptComponentItem::ScriptComponentItem(ScriptingApi::Content* content_, const Identifier& id_) :
	Item(getSearchTerm(content_, id_)),
	idAsId(id_),
	content(content_),
	connectedScriptComponent(content_->getComponentWithName(id_)),
	data(content_->getValueTreeForComponent(id_))
{
	setOpaque(true);

	setWantsKeyboardFocus(true);

	if (connectedScriptComponent != nullptr)
	{
		connectedScriptComponent->addChangeListener(this);
	}

	id = idAsId.toString();

	typeOffset = GLOBAL_BOLD_FONT().getStringWidthFloat(id) + 5.0f;

	const Identifier ty("type");

	typeName = " (" +  data.getProperty("type").toString() + ")";
	typeName = typeName.replace("Scripting", "");
	typeName = typeName.replace("Scripted", "");
	typeName = typeName.replace("Script", "");

	childDepth = 0;

	getChildDepth(data, childDepth);
	
	setSize(380 - 16 - 8 - 24, ITEM_HEIGHT);
}

ScriptComponentList::ScriptComponentItem::~ScriptComponentItem()
{
	data = ValueTree();

	if (connectedScriptComponent != nullptr)
	{
		connectedScriptComponent->removeChangeListener(this);
		connectedScriptComponent = nullptr;
	}
}

void ScriptComponentList::ScriptComponentItem::changeListenerCallback(SafeChangeBroadcaster* /*b*/)
{
	repaint();
}

void ScriptComponentList::ScriptComponentItem::mouseEnter(const MouseEvent&)
{
	repaint();
}

void ScriptComponentList::ScriptComponentItem::mouseExit(const MouseEvent&)
{
	repaint();
}



void ScriptComponentList::ScriptComponentItem::mouseDoubleClick(const MouseEvent&)
{
	if(connectedScriptComponent)
		ScriptingApi::Content::Helpers::gotoLocation(connectedScriptComponent);
}



bool ScriptComponentList::ScriptComponentItem::isInterestedInDragSource(const SourceDetails& dragSourceDetails)
{
	static const Identifier id_("id");

	if (auto ar = dragSourceDetails.description.getArray())
	{
		var name(data.getProperty(id_));

		if (ar->contains(name))
			return false;

		return true;
	}

	return false;

#if 0
	if (connectedScriptComponent == nullptr)
		return false;

	if (auto ar = dragSourceDetails.description.getArray())
	{
		var d(connectedScriptComponent);
		return !ar->contains(d);
	}

	return false;
#endif

	

void ScriptComponentList::ScriptComponentItem::itemDragEnter(const SourceDetails& dragSourceDetails)
{
	insertDragAfterComponent = dragSourceDetails.localPosition.getY() > (ITEM_HEIGHT / 2);
	insertDragAsParentComponent = !insertDragAfterComponent;

	repaint();
}

void ScriptComponentList::ScriptComponentItem::itemDragExit(const SourceDetails& /*dragSourceDetails*/)
{
	insertDragAfterComponent = false;
	insertDragAsParentComponent = false;

	repaint();
}

void ScriptComponentList::ScriptComponentItem::itemDragMove(const SourceDetails& dragSourceDetails)
{
	insertDragAfterComponent = dragSourceDetails.localPosition.getY() > (ITEM_HEIGHT / 2);
	insertDragAsParentComponent = !insertDragAfterComponent;

	repaint();
}

void ScriptComponentList::ScriptComponentItem::itemDropped(const SourceDetails& dragSourceDetails)
{
	insertDragAfterComponent = dragSourceDetails.localPosition.getY() > (ITEM_HEIGHT / 2);
	insertDragAsParentComponent = !insertDragAfterComponent;

	auto processor = findParentComponentOfClass<PanelWithProcessorConnection>()->getConnectedProcessor();
	auto content = dynamic_cast<JavascriptProcessor*>(processor)->getContent();

	auto name = data.getProperty("id");
	auto list = dragSourceDetails.description;

	if (insertDragAsParentComponent)
	{
		ScriptingApi::Content::Helpers::setParentComponent(content, name, list);

		
		
	}
	else
	{
		//ScriptingApi::Content::Helpers::moveComponentsAfter(connectedScriptComponent, dragSourceDetails.description);
	}

	insertDragAfterComponent = false;
	insertDragAsParentComponent = false;

	repaint();
}


void ScriptComponentList::ScriptComponentItem::mouseUp(const MouseEvent& event)
{
	if (connectedScriptComponent == nullptr)
		return;

	if (!event.mods.isRightButtonDown() && event.mouseWasDraggedSinceMouseDown())
		return;

	auto b = findParentComponentOfClass<ScriptComponentEditListener>()->getScriptComponentEditBroadcaster();



	if (event.mods.isRightButtonDown())
	{
		enum PopupMenuOptions
		{
			DeleteComponent = 1,
			DeleteSelection,
			RenameComponent,
			CreateScriptVariableDeclaration,
			CreateCustomCallbackDefinition,
			CopyProperties,
			PasteProperties,
			editJson,
			numOptions
		};

		PopupLookAndFeel plaf;
		PopupMenu m;
		m.setLookAndFeel(&plaf);

		m.addItem(PopupMenuOptions::DeleteComponent, "Delete Component", true);
		m.addItem(PopupMenuOptions::RenameComponent, "Rename Component", true);
		m.addItem(PopupMenuOptions::DeleteSelection, "Delete selected Components", b->getSelection().size() > 0);
		m.addItem(PopupMenuOptions::CreateScriptVariableDeclaration, "Create script variable definition");
		m.addItem(PopupMenuOptions::CreateCustomCallbackDefinition, "Create custom callback definition");

		auto clipboardData = JSON::parse(SystemClipboard::getTextFromClipboard());

		m.addItem(PopupMenuOptions::CopyProperties, "Copy properties");
		m.addItem(PopupMenuOptions::PasteProperties, "Paste properties to selection", clipboardData.isObject());

		ScriptComponentSelection componentListToUse;
		
		if (b->getNumSelected() == 0)
			componentListToUse.add(connectedScriptComponent);
		else
			componentListToUse = b->getSelection();

		


		const PopupMenuOptions result = (PopupMenuOptions)m.show();

		switch (result)
		{
		case DeleteComponent:
		{
			ScriptingApi::Content::Helpers::deleteComponent(connectedScriptComponent->parent, connectedScriptComponent->name);
			break;
		}
		case DeleteSelection:
		{
			ScriptingApi::Content::Helpers::deleteSelection(connectedScriptComponent->parent, b);
			break;
		}
		case RenameComponent:
		{
			auto newId = Identifier(PresetHandler::getCustomName(connectedScriptComponent->name.toString(), "Enter the new ID for the component"));

			if (connectedScriptComponent->parent->getComponentWithName(newId) == nullptr)
			{
				ScriptingApi::Content::Helpers::renameComponent(connectedScriptComponent->parent, connectedScriptComponent->name, newId);
			}
			break;
		}
		case CreateScriptVariableDeclaration:
		{
			auto st = ScriptingApi::Content::Helpers::createScriptVariableDeclaration(componentListToUse);

			debugToConsole(dynamic_cast<Processor*>(connectedScriptComponent->parent->getScriptProcessor()), String(b->getNumSelected()) + " script definitions created and copied to the clipboard");

			SystemClipboard::copyTextToClipboard(st);
			break;
		}
		case CreateCustomCallbackDefinition:
		{
			auto st = ScriptingApi::Content::Helpers::createCustomCallbackDefinition(componentListToUse);

			

			debugToConsole(dynamic_cast<Processor*>(connectedScriptComponent->parent->getScriptProcessor()), String(b->getNumSelected()) + " callback definitions created and copied to the clipboard");

			SystemClipboard::copyTextToClipboard(st);
			break;
		}
		case CopyProperties:
		{
			SystemClipboard::copyTextToClipboard(connectedScriptComponent->getScriptObjectPropertiesAsJSON());
			break;
		}
		case PasteProperties:
		{
			ScriptingApi::Content::Helpers::pasteProperties(b->getSelection(), clipboardData);
			break;
		}
		default:
			break;
		}

	}
	else
	{
		auto l = findParentComponentOfClass<ScriptComponentList>();
		
		if (event.mods.isShiftDown() && l->lastClickedComponent != nullptr)
		{
			auto content = connectedScriptComponent->parent;

			const int index1 = content->getComponentIndex(connectedScriptComponent->name);
			const int index2 = content->getComponentIndex(l->lastClickedComponent->name);

			const int start = jmin<int>(index1, index2);
			const int end = jmax<int>(index1, index2);

			for (int i = start; i <= end; i++)
			{
				bool isLast = (i == end);
				b->addToSelection(content->getComponent(i), isLast ? sendNotification : dontSendNotification);
			}

			l->lastClickedComponent = connectedScriptComponent;
		}
		else
		{


			b->updateSelectionBasedOnModifier(connectedScriptComponent, event.mods, sendNotification);
			repaint();
		}

		l->lastClickedComponent = connectedScriptComponent;

	}
}

void ScriptComponentList::ScriptComponentItem::mouseDrag(const MouseEvent& event)
{
	if (connectedScriptComponent == nullptr)
		return;

	if (event.mods.isRightButtonDown())
		return;

	auto b = findParentComponentOfClass<ScriptComponentEditListener>()->getScriptComponentEditBroadcaster();

	auto container = DragAndDropContainer::findParentDragContainerFor(this);

	if (container->isDragAndDropActive())
		return;

	if (event.mouseWasDraggedSinceMouseDown())
	{
		//b->prepareSelectionForDragging(c);

		b->addToSelection(connectedScriptComponent);

		ScriptComponentEditBroadcaster::Iterator iter(b);

		Array<var> list;

		while (auto sc = iter.getNextScriptComponent())
		{
			list.add(var(sc->getName().toString()));
		}

		container->startDragging(list, this);
		repaint();
	}
}




bool ScriptComponentList::ScriptComponentItem::keyPressed(const KeyPress& key)
{
	if (connectedScriptComponent == nullptr)
		return false;

	if (key.getKeyCode() == 'J')
	{
		auto b = findParentComponentOfClass<ScriptComponentEditListener>()->getScriptComponentEditBroadcaster();

		ScriptComponentEditBroadcaster::Iterator iter(b);

		auto content = connectedScriptComponent->parent;

		Array<var> list;

		while(auto sc = iter.getNextScriptComponent())
		{
			auto tree = content->getValueTreeForComponent(sc->name);

			auto v = ValueTreeConverters::convertContentPropertiesToDynamicObject(tree);

			list.add(v);
		}

		
		JSONEditor* editor = new JSONEditor(var(list));

		editor->setEditable(true);
		
		auto id = connectedScriptComponent->name;

		auto callback = [content, this](const var& newData)
		{
			auto b = this->findParentComponentOfClass<ScriptComponentEditListener>()->getScriptComponentEditBroadcaster();

			if (auto ar = newData.getArray())
			{
				auto selection = b->getSelection();

				jassert(ar->size() == selection.size());

				for (int i = 0; i < selection.size(); i++)
				{
					auto sc = selection[i];

					auto newJson = ar->getUnchecked(i);

					ScriptingApi::Content::Helpers::setComponentValueTreeFromJSON(content, sc->name, newJson);
				}

				content->updateAndSetLevel(ScriptingApi::Content::UpdateLevel::FullRecompile);
			}

			
			return;
		};

		editor->setCallback(callback);

		editor->setName("Editing JSON data of " + connectedScriptComponent->name);

		editor->setSize(400, 400);

		findParentComponentOfClass<FloatingTile>()->showComponentInRootPopup(editor, this, getLocalBounds().getCentre());

		editor->grabKeyboardFocus();
	}

	return false;
}

void ScriptComponentList::ScriptComponentItem::paint(Graphics& g)
{
	if (getWidth() <= 0)
		return;

	g.fillAll(HiseColourScheme::getColour(HiseColourScheme::ColourIds::DebugAreaBackgroundColourId));

	Colour co(0xFF000000);

	const float h = (float)getHeight();
	const float w = (float)getWidth() - 4;

	ColourGradient grad(co.withAlpha(0.1f), 0.0f, .0f, co.withAlpha(0.2f), 0.0f, h, false);

	g.setGradientFill(grad);

	g.fillRect(2, 2, (int)w - 4, (int)h - 4);

	auto b = findParentComponentOfClass<ScriptComponentList>()->getScriptComponentEditBroadcaster();

	co = b->isSelected(connectedScriptComponent) ? Colour(SIGNAL_COLOUR) : Colour(0xFF666666);

	co = co.withAlpha((!isMouseButtonDown() && isMouseOver(true)) ? 1.0f : 0.3f);

	g.setColour(co);

	g.drawRect(2, 2, (int)w, (int)h - 4, 1);
    
	g.setColour(Colours::black.withAlpha(0.1f));

	g.fillRect(3, 3, (int)h - 6, (int)h - 6);

	if (connectedScriptComponent != nullptr)
	{
		static const Identifier sip("saveInPreset");

		const bool saveInPreset = connectedScriptComponent->getScriptObjectProperties()->getProperty(sip);

		Colour c3 = saveInPreset ? Colours::green : Colours::red;

		c3 = c3.withAlpha(JUCE_LIVE_CONSTANT_OFF(0.3f));

		g.setColour(c3);

		const float offset = JUCE_LIVE_CONSTANT_OFF(8.0f);
		Rectangle<float> circle(offset, offset, (float)ITEM_HEIGHT - 2.0f * offset, (float)ITEM_HEIGHT - 2.0f * offset);

		g.fillEllipse(circle);

		g.drawEllipse(circle, 1.0f);
	}


	const bool isShowing = (connectedScriptComponent != nullptr && connectedScriptComponent->isShowing());

	g.setOpacity(isShowing ? 1.0f : 0.4f);

	Colour textColour = Colours::white.withAlpha(isShowing ? 1.0f : 0.4f);

	g.setColour(textColour);
	g.setFont(GLOBAL_BOLD_FONT());
	
	float textOffset = h + 2.0f + (float)(childDepth * 10);
	
	g.drawText(id, (int)textOffset, 0, (int)typeOffset, ITEM_HEIGHT, Justification::centredLeft);

	g.setColour(textColour.withMultipliedBrightness(0.4f));
	g.setFont(GLOBAL_FONT());

	g.drawText(typeName, (int)(textOffset + typeOffset), 0, getWidth() - (int)(textOffset + typeOffset), ITEM_HEIGHT, Justification::centredLeft);

	if (insertDragAfterComponent)
	{
		g.setColour(Colour(SIGNAL_COLOUR));
		g.fillRect(0, getHeight() - 3, getWidth(), 3);
	}
	else if (insertDragAsParentComponent)
	{
		g.setColour(Colour(SIGNAL_COLOUR).withAlpha(0.3f));
		g.fillRect(2, 2, (int)w - 4, (int)h - 4);
	}


}

using ValueTreeIteratorFunction = std::function<bool(ValueTree&)>;

bool executeRecursively(ValueTree& v, const ValueTreeIteratorFunction& f)
{
	if (f(v))
		return true;

	for (int i = 0; i < v.getNumChildren(); i++)
	{
		if (executeRecursively(v.getChild(i), f))
			return true;
	}

	return false;
}

bool isValueTreeVisible(const ValueTree & v)
{
	const bool isVisible = (bool)v.getProperty("visible", true);

	if (!isVisible)
		return false;

	if (v.getParent().isValid())
		return isValueTreeVisible(v.getParent());

	return true;
}

ScriptComponentList::AllCollection::AllCollection(JavascriptProcessor* p, bool showOnlyVisibleItems)
{
	
	auto content = p->getContent();
	auto v = content->getContentProperties();

	static const Identifier id_("id");

	auto f = [this, showOnlyVisibleItems, p](ValueTree& v)
	{
		const bool isVisible = isValueTreeVisible(v);

		if (showOnlyVisibleItems && !isVisible)
			return false;

		auto content = p->getContent();

		const String idString = v.getProperty(id_).toString();

		static const Identifier coPro("ContentProperties");

		if (idString.isEmpty())
		{
			jassert(v.getType() == coPro);
			return false;
		}
			

		Identifier componentId = Identifier(idString);

		this->items.add(new ScriptComponentItem(content, componentId));

		this->addAndMakeVisible(items.getLast());

		return false; 
	};
	
	executeRecursively(v, f);

#if 0
	for (int i = 0; i < content->getNumComponents(); i++)
	{
		if (showOnlyVisibleItems && !content->getComponent(i)->isShowing())
			continue;

		items.add(new ScriptComponentItem(content, content->getComponent(i)->getName()));
		addAndMakeVisible(items.getLast());
	}
#endif
}

void ScriptComponentList::AllCollection::paint(Graphics& g)
{
	g.fillAll(HiseColourScheme::getColour(HiseColourScheme::ColourIds::DebugAreaBackgroundColourId));

	if (isDropTarget)
	{
		g.setColour(Colour(SIGNAL_COLOUR).withAlpha(0.2f));
		g.fillAll();
		g.setColour(Colour(SIGNAL_COLOUR));
		g.drawRect(getLocalBounds(), 1);
	}

	g.setColour(Colours::white);
	g.setFont(GLOBAL_BOLD_FONT());

	String t;

	t << getNumItems(false) << " Components (" << getNumItems(true) << " visible)";

	g.drawText(t, getLocalBounds().removeFromTop(30), Justification::centred);
}

bool ScriptComponentList::AllCollection::isInterestedInDragSource(const SourceDetails& dragSourceDetails)
{
	if (auto ar = dragSourceDetails.description.getArray())
	{
		return ar->size() > 0;
	}

	return false;
}

void ScriptComponentList::AllCollection::itemDragEnter(const SourceDetails& /*dragSourceDetails*/)
{
	isDropTarget = true;
	repaint();
}

void ScriptComponentList::AllCollection::itemDragExit(const SourceDetails& /*dragSourceDetails*/)
{
	isDropTarget = false;
	repaint();
}

void ScriptComponentList::AllCollection::itemDragMove(const SourceDetails& /*dragSourceDetails*/)
{

}

void ScriptComponentList::AllCollection::itemDropped(const SourceDetails& dragSourceDetails)
{
	items.clear();

	auto processor = findParentComponentOfClass<PanelWithProcessorConnection>()->getConnectedProcessor();
	auto content = dynamic_cast<JavascriptProcessor*>(processor)->getContent();

	const var list = dragSourceDetails.description;

	auto f = [=]()
	{
		ScriptingApi::Content::Helpers::setParentComponent(content, "root", list);
	};
	
	new DelayedFunctionCaller(f, 200);
	
	isDropTarget = false;
	repaint();
}

Component* ScriptComponentList::Panel::createContentComponent(int /*index*/)
{
	return new ScriptComponentList(getRootWindow(), dynamic_cast<JavascriptProcessor*>(getConnectedProcessor()));
}





#endif


Component* ScriptComponentList::Panel::createContentComponent(int /*index*/)
{
	auto jp = dynamic_cast<JavascriptProcessor*>(getConnectedProcessor());
	auto c = jp->getContent();

	return new ScriptComponentList(c);
}




void ScriptComponentListItem::itemSelectionChanged(bool isNowSelected)
{
	auto b = content->getScriptProcessor()->getMainController_()->getScriptComponentEditBroadcaster();

	auto sc = content->getComponentWithName(id);

	if (sc != nullptr)
	{
		if (isNowSelected)
		{
			b->addToSelection(sc);
		}
		else
		{
			b->removeFromSelection(sc);
		}
	}
}


#define ADD_WIDGET(widgetIndex, widgetClass) case (int)widgetIndex: ScriptingApi::Content::Helpers::createNewComponentData(content, pTree, widgetClass::getStaticObjectName().toString(), ScriptingApi::Content::Helpers::getUniqueIdentifier(content, widgetClass::getStaticObjectName().toString()).toString()); break;

void ScriptComponentList::mouseUp(const MouseEvent& event)
{
	

	if(event.mods.isRightButtonDown())
	{
		auto b = getScriptComponentEditBroadcaster();

		enum PopupMenuOptions
		{
			CreateScriptVariableDeclaration = 1,
			CreateCustomCallbackDefinition,
			CopyProperties,
			PasteProperties,
			numOptions
		};

		PopupLookAndFeel plaf;
		PopupMenu m;
		m.setLookAndFeel(&plaf);

		m.addItem(PopupMenuOptions::CreateScriptVariableDeclaration, "Create script variable definition");
		m.addItem(PopupMenuOptions::CreateCustomCallbackDefinition, "Create custom callback definition");

		auto clipboardData = JSON::parse(SystemClipboard::getTextFromClipboard());

		const bool isSingleSelection = tree.getNumSelectedItems() == 1;

		m.addItem(PopupMenuOptions::CopyProperties, "Copy properties", isSingleSelection);
		m.addItem(PopupMenuOptions::PasteProperties, "Paste properties to selection", clipboardData.isObject());

		ValueTree pTree;
		
		if (isSingleSelection)
		{
			pTree = static_cast<ScriptComponentListItem*>(tree.getSelectedItem(0))->tree;

			m.addSectionHeader("Add new widget");
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
		}
		

		ScriptComponentSelection componentListToUse = b->getSelection();

		const int result = m.show();

		switch (result)
		{
		
		case CreateScriptVariableDeclaration:
		{
			auto st = ScriptingApi::Content::Helpers::createScriptVariableDeclaration(componentListToUse);

			debugToConsole(dynamic_cast<Processor*>(content->getScriptProcessor()), String(b->getNumSelected()) + " script definitions created and copied to the clipboard");

			SystemClipboard::copyTextToClipboard(st);
			break;
		}
		case CreateCustomCallbackDefinition:
		{
			auto st = ScriptingApi::Content::Helpers::createCustomCallbackDefinition(componentListToUse);



			debugToConsole(dynamic_cast<Processor*>(content->getScriptProcessor()), String(b->getNumSelected()) + " callback definitions created and copied to the clipboard");

			SystemClipboard::copyTextToClipboard(st);
			break;
		}
		case CopyProperties:
		{
			if (tree.getNumSelectedItems() == 1)
			{
				auto sc = static_cast<ScriptComponentListItem*>(tree.getSelectedItem(0))->connectedComponent;

				if (sc != nullptr)
				{
					SystemClipboard::copyTextToClipboard(sc->getScriptObjectPropertiesAsJSON());
				}
			}

			
			break;
		}
		case PasteProperties:
		{
			ScriptingApi::Content::Helpers::pasteProperties(b->getSelection(), clipboardData);
			break;
		}
		ADD_WIDGET(ScriptEditHandler::Widgets::Knob, ScriptingApi::Content::ScriptSlider);
		ADD_WIDGET(ScriptEditHandler::Widgets::Button, ScriptingApi::Content::ScriptButton);
		ADD_WIDGET(ScriptEditHandler::Widgets::Label, ScriptingApi::Content::ScriptLabel);
		ADD_WIDGET(ScriptEditHandler::Widgets::AudioWaveform, ScriptingApi::Content::ScriptAudioWaveform);
		ADD_WIDGET(ScriptEditHandler::Widgets::ComboBox, ScriptingApi::Content::ScriptComboBox);
		ADD_WIDGET(ScriptEditHandler::Widgets::FloatingTile, ScriptingApi::Content::ScriptFloatingTile);
		ADD_WIDGET(ScriptEditHandler::Widgets::Image, ScriptingApi::Content::ScriptImage);
		ADD_WIDGET(ScriptEditHandler::Widgets::ModulatorMeter, ScriptingApi::Content::ModulatorMeter);
		ADD_WIDGET(ScriptEditHandler::Widgets::Plotter, ScriptingApi::Content::ScriptedPlotter);
		ADD_WIDGET(ScriptEditHandler::Widgets::Panel, ScriptingApi::Content::ScriptPanel);
		ADD_WIDGET(ScriptEditHandler::Widgets::SliderPack, ScriptingApi::Content::ScriptSliderPack);
		ADD_WIDGET(ScriptEditHandler::Widgets::Table, ScriptingApi::Content::ScriptTable);
		ADD_WIDGET(ScriptEditHandler::Widgets::Viewport, ScriptingApi::Content::ScriptedViewport);
		default:
			break;
		}

	}
}


#undef ADD_WIDGET

bool ScriptComponentList::keyPressed(const KeyPress& key)
{
	if (key == KeyPress::deleteKey)
	{
		deleteSelectedItems();
		return true;
	}

	if (key == KeyPress::escapeKey)
	{
		tree.clearSelectedItems();
		return true;
	}

	if (key == KeyPress('z', ModifierKeys::commandModifier, 0))
	{
		undoButton.triggerClick();
		return true;
	}

	if (key == KeyPress('z', ModifierKeys::commandModifier | ModifierKeys::shiftModifier, 0))
	{
		redoButton.triggerClick();
		return true;
	}
	if (key.getKeyCode() == 'J')
	{
		Array<var> list;

		for (int i = 0; i < tree.getNumSelectedItems(); i++)
		{
			auto t = static_cast<ScriptComponentListItem*>(tree.getSelectedItem(i))->tree;
			auto v = ValueTreeConverters::convertContentPropertiesToDynamicObject(t);
			list.add(v);
		}

		JSONEditor* editor = new JSONEditor(var(list));

		editor->setEditable(true);

		auto& tmpContent = content;

		auto callback = [tmpContent, this](const var& newData)
		{
			auto b = this->getScriptComponentEditBroadcaster();

			if (auto ar = newData.getArray())
			{
				auto selection = b->getSelection();

				jassert(ar->size() == selection.size());

				for (int i = 0; i < selection.size(); i++)
				{
					auto sc = selection[i];

					auto newJson = ar->getUnchecked(i);

					ScriptingApi::Content::Helpers::setComponentValueTreeFromJSON(content, sc->name, newJson);
				}

				content->updateAndSetLevel(ScriptingApi::Content::UpdateLevel::FullRecompile);
			}



			return;
		};

		editor->setCallback(callback, true);

		editor->setName("Editing JSON");

		editor->setSize(400, 400);

		findParentComponentOfClass<FloatingTile>()->showComponentInRootPopup(editor, this, getLocalBounds().getCentre());

		editor->grabKeyboardFocus();

		return true;
	}

	return Component::keyPressed(key);
}


} // namespace hise
