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

#include "hi_scripting/scripting/components/ScriptingContentOverlay.h"

namespace hise { using namespace juce;


Component* ScriptComponentList::Panel::createContentComponent(int /*index*/)
{
	auto jp = dynamic_cast<JavascriptProcessor*>(getConnectedProcessor());
	auto c = jp->getContent();

	return new ScriptComponentList(c, defaultOpeness);
}




ScriptComponentListItem::ScriptComponentListItem(ValueTree v, UndoManager& um_, ScriptingApi::Content* c, const String& searchTerm_) : 
	AsyncValueTreePropertyListener(v, c->getUpdateDispatcher()),
	tree(v),
	undoManager(um_),
	content(c),
	searchTerm(searchTerm_)
{
	c->getProcessor()->getMainController()->addScriptListener(this);

	

	static const Identifier coPro("ContentProperties");

	if (tree.getType() == coPro)
		id = "Components";
	else
	{
		id = tree.getProperty("id");
	}
	
	tree.addListener(this);

	startTimer(50);
}

void ScriptComponentListItem::itemDoubleClicked(const MouseEvent& /*e*/)
{
	if (content.get() == nullptr)
		return;

	if (isRootItem())
		return;

	auto scId = var(getUniqueName());

	try
	{
		auto v = content->getComponent(scId);

		if (auto sc = dynamic_cast<ScriptingApi::Content::ScriptComponent*>(v.getObject()))
		{
			ScriptingApi::Content::Helpers::gotoLocation(sc);
		}
	}
	catch (String& /*errorMessage*/)
	{
		jassertfalse;
	}
}

void ScriptComponentListItem::paintItem(Graphics& g, int width, int height)
{
	if (isRootItem())
	{
		g.setColour(Colours::white);

		g.setFont(GLOBAL_BOLD_FONT());

		g.drawText("Root", 2, 0, width - 4, height, Justification::centredLeft, true);

	}
	else
	{
		auto area = FLOAT_RECTANGLE(Rectangle<int>(0, 0, width, height));

        g.setGradientFill(ColourGradient(JUCE_LIVE_CONSTANT_OFF(Colour(0xff303030)), 0.0f, 0.0f,
            JUCE_LIVE_CONSTANT_OFF(Colour(0xff282828)), 0.0f, (float)area.getHeight(), false));

        g.fillRoundedRectangle(area, 2.0f);
        g.setColour(Colours::white.withAlpha(0.1f));
        g.drawRoundedRectangle(area.reduced(1.0f), 2.0f, 1.0f);
        
		if(isSelected())
        {
            g.setColour(Colour(SIGNAL_COLOUR).withAlpha(0.2f));
            g.fillRoundedRectangle(area, 2.0f);
        }

        


		bool saveInPreset = false;

		if (content.get() != nullptr)
		{
			auto scId = var(getUniqueName());
			try
			{
				auto v = content->getComponent(scId);

				if (auto sc = dynamic_cast<ScriptingApi::Content::ScriptComponent*>(v.getObject()))
				{
					saveInPreset = sc->getScriptObjectProperty(ScriptComponent::Properties::saveInPreset);
				}
			}
			catch (String& /*errorMessage*/)
			{
				jassertfalse;
			}
			
		}

		Colour c3 = saveInPreset ? Colours::green : Colours::red;

		c3 = c3.withAlpha(JUCE_LIVE_CONSTANT_OFF(0.3f));

        if(!ContentValueTreeHelpers::isShowing(tree))
            c3 = c3.withMultipliedAlpha(0.5f);
        
		//g.setColour(c3);

		const float offset = JUCE_LIVE_CONSTANT_OFF(8.0f);
		Rectangle<float> circle(offset, offset, (float)ITEM_HEIGHT - 2.0f * offset, (float)ITEM_HEIGHT - 2.0f * offset);

        
        
        if(saveInPreset)
        {
            
            g.setColour(Colour(SIGNAL_COLOUR).withAlpha(0.8f));

            if (!ContentValueTreeHelpers::isShowing(tree))
            {
                g.setColour(Colour(SIGNAL_COLOUR).withAlpha(0.3f));
            }
            
            float v1 = JUCE_LIVE_CONSTANT_OFF(6.0f);
            float v2 = JUCE_LIVE_CONSTANT_OFF(2.5f);
            
            Path p;
            p.addStar({}, 5, v1, v2);
            p.applyTransform(AffineTransform::rotation(float_Pi));
            p.applyTransform(AffineTransform::translation(circle.getCentre()));
            g.fillPath(p);
        }
        
        g.setColour(Colours::white.withAlpha(0.9f));

        if (!ContentValueTreeHelpers::isShowing(tree))
        {
            g.setColour(Colours::white.withAlpha(0.3f));
        }
		
		//g.drawEllipse(circle, 1.0f);

		

		g.setFont(GLOBAL_BOLD_FONT());

		int xOffset = ITEM_HEIGHT + 2;

		g.drawText(id, xOffset, 0, width - 4, height, Justification::centredLeft, true);

		xOffset += GLOBAL_BOLD_FONT().getStringWidth(id) + 10;

		g.setColour(Colours::white.withAlpha(0.2f));

		auto typeName = tree.getProperty("type").toString().replace("Scripted", "").replace("Script", "");

		if (isDefinedInScript)
		{
			typeName << " {...}";
		}

		g.drawText(typeName, 4 + xOffset, 0, width - 4, height, Justification::centredLeft, true);

		auto isLocked = (bool)tree.getProperty("locked");

		if (isLocked)
		{
			ScriptContentPanel::Factory f;

			Rectangle<float> a = area.removeFromRight(area.getHeight()).reduced(1);

			g.setColour(Colours::white.withAlpha(0.8f));

			auto p = f.createPath("lock");
			f.scalePath(p, a);
			g.fillPath(p);
		}
	}

	
	
}

void ScriptComponentListItem::itemSelectionChanged(bool isNowSelected)
{
	if (content.get() == nullptr)
		return;

	if (!fitsSearch)
	{
		setSelected(false, false, sendNotification);
		return;
	}

	auto b = content->getScriptProcessor()->getMainController_()->getScriptComponentEditBroadcaster();

	try
	{
		auto sc = content->getComponentWithName(id);

		if (sc != nullptr)
		{
			auto selectionContainsParent = b->getSelection().contains(sc->getParentScriptComponent());

			if (isNowSelected && !selectionContainsParent)
			{
				b->addToSelection(sc);
			}
			else
			{
				b->removeFromSelection(sc);
			}
		}
	}
	catch (String& /*errorMessage*/)
	{
		jassertfalse;
	}
	
}


void ScriptComponentListItem::itemOpennessChanged(bool isNowOpen)
{
	
	if (isNowOpen && getNumSubItems() == 0)
		refreshSubItems();
	else
		clearSubItems();

	if (auto l = getOwnerView()->findParentComponentOfClass<ScriptComponentList>())
	{
		auto v = l->getFoldStateTree();

		v.setProperty(getUniqueName(), isNowOpen, nullptr);
	}
}

var ScriptComponentListItem::getDragSourceDescription()
{
	return "ScriptComponent";
}

void ScriptComponentListItem::refreshScriptDefinedState()
{
	if (isRootItem())
		return;

	try
	{
		Identifier name(getUniqueName());

		if (content.get() == nullptr)
			return;

		if (auto sc = content->getComponentWithName(name))
			isDefinedInScript = ScriptingApi::Content::Helpers::hasLocation(sc);

		repaintItem();
	}
	catch (String& errorMessage)
	{
        DBG(errorMessage);
        ignoreUnused(errorMessage);
		jassertfalse;
	}

	
}

bool ScriptComponentListItem::isInterestedInDragSource(const DragAndDropTarget::SourceDetails& dragSourceDetails)
{
	return dragSourceDetails.description == "ScriptComponent";
}

void ScriptComponentListItem::itemDropped(const DragAndDropTarget::SourceDetails&, int insertIndex)
{
	OwnedArray<ValueTree> selectedTrees;
	getSelectedTreeViewItems(*getOwnerView(), selectedTrees);

	moveItems(*getOwnerView(), selectedTrees, tree, insertIndex, undoManager);
}

void ScriptComponentListItem::moveItems(TreeView& treeView, const OwnedArray<ValueTree>& items, ValueTree newParent, int insertIndex, UndoManager& undoManager)
{
	static const Identifier pc("parentComponent");

	if (items.size() > 0)
	{
		auto oldOpenness = treeView.getOpennessState(false);

        auto c = dynamic_cast<ScriptComponentListItem*>(treeView.getRootItem())->content.get();
        
        if(c == nullptr)
            return;
        
        ValueTreeUpdateWatcher::ScopedDelayer sd(c->getUpdateWatcher());
        
		for (int i = items.size(); --i >= 0;)
		{
			ValueTree& v = *items.getUnchecked(i);

			if (v.getParent().isValid() && newParent != v && !newParent.isAChildOf(v))
			{
				if (v.getParent() == newParent && newParent.indexOf(v) < insertIndex)
					--insertIndex;

				auto cPos = ContentValueTreeHelpers::getLocalPosition(v);
				ContentValueTreeHelpers::getAbsolutePosition(v, cPos);


				auto nPos = ContentValueTreeHelpers::getLocalPosition(newParent);
				ContentValueTreeHelpers::getAbsolutePosition(newParent, nPos);

				v.getParent().removeChild(v, &undoManager);
				v.setProperty(pc, newParent.getProperty("id"), &undoManager);

				static const Identifier x("x");
				static const Identifier y("y");

				auto delta = cPos - nPos;

				v.setProperty(x, delta.getX(), &undoManager);
				v.setProperty(y, delta.getY(), &undoManager);


				newParent.addChild(v, insertIndex, &undoManager);
			}
		}

		if (oldOpenness != nullptr)
			treeView.restoreOpennessState(*oldOpenness, false);
	}
}

void ScriptComponentListItem::getSelectedTreeViewItems(TreeView& treeView, OwnedArray<ValueTree>& items)
{
	const int numSelected = treeView.getNumSelectedItems();

	for (int i = 0; i < numSelected; ++i)
		if (const ScriptComponentListItem* vti = dynamic_cast<ScriptComponentListItem*> (treeView.getSelectedItem(i)))
			items.add(new ValueTree(vti->tree));
}

void ScriptComponentListItem::updateSelection(ScriptComponentSelection newSelection)
{
	bool select = false;

	for (auto& sc : newSelection)
	{
		if (sc->getPropertyValueTree() == tree)
		{
			select = true;
			break;
		}
	}

	setSelected(select, false, dontSendNotification);

	for (int i = 0; i < getNumSubItems(); i++)
	{
		static_cast<ScriptComponentListItem*>(getSubItem(i))->updateSelection(newSelection);
	}
}

#define ADD_SCRIPT_COMPONENT(cIndex, cClass) case (int)cIndex: ScriptingApi::Content::Helpers::createNewComponentData(content, pTree, cClass::getStaticObjectName().toString(), ScriptingApi::Content::Helpers::getUniqueIdentifier(content, cClass::getStaticObjectName().toString()).toString()); break;

ScriptComponentList::ScriptComponentList(ScriptingApi::Content* c, bool openess) :
	ScriptComponentEditListener(dynamic_cast<Processor*>(c->getScriptProcessor())),
	undoManager(getScriptComponentEditBroadcaster()->getUndoManager()),
	content(c),
	foldState(ValueTree("FoldState")),
    defaultOpeness(openess)
{
	

	addAsScriptEditListener();

	content->addRebuildListener(this);

	addAndMakeVisible(fuzzySearchBox = new TextEditor());
	fuzzySearchBox->addListener(this);
	
	GlobalHiseLookAndFeel::setTextEditorColours(*fuzzySearchBox);

	
	

	addAndMakeVisible(tree = new TreeView());

	tree->setMultiSelectEnabled(true);
	tree->setColour(TreeView::backgroundColourId, Colours::transparentBlack);
	tree->setColour(TreeView::ColourIds::dragAndDropIndicatorColourId, Colour(SIGNAL_COLOUR));
	tree->setColour(TreeView::ColourIds::selectedItemBackgroundColourId, Colours::transparentBlack);
	tree->setColour(TreeView::ColourIds::linesColourId, Colours::white.withAlpha(JUCE_LIVE_CONSTANT_OFF(0.1f)));
	resetRootItem();

	tree->setIndentSize(12);

	tree->setLookAndFeel(&laf);

	//tree->setRootItemVisible(false);
    tree->getViewport()->setScrollBarThickness(13);
    sf.addScrollBarToAnimate(tree->getViewport()->getVerticalScrollBar());
	
	tree->addMouseListener(this, true);

	static const unsigned char searchIcon[] = { 110, 109, 0, 0, 144, 68, 0, 0, 48, 68, 98, 7, 31, 145, 68, 198, 170, 109, 68, 78, 223, 103, 68, 148, 132, 146, 68, 85, 107, 42, 68, 146, 2, 144, 68, 98, 54, 145, 219, 67, 43, 90, 143, 68, 66, 59, 103, 67, 117, 24, 100, 68, 78, 46, 128, 67, 210, 164, 39, 68, 98, 93, 50, 134, 67, 113, 58, 216, 67, 120, 192, 249, 67, 83, 151,
		103, 67, 206, 99, 56, 68, 244, 59, 128, 67, 98, 72, 209, 112, 68, 66, 60, 134, 67, 254, 238, 144, 68, 83, 128, 238, 67, 0, 0, 144, 68, 0, 0, 48, 68, 99, 109, 0, 0, 208, 68, 0, 0, 0, 195, 98, 14, 229, 208, 68, 70, 27, 117, 195, 211, 63, 187, 68, 146, 218, 151, 195, 167, 38, 179, 68, 23, 8, 77, 195, 98, 36, 92, 165, 68, 187, 58,
		191, 194, 127, 164, 151, 68, 251, 78, 102, 65, 0, 224, 137, 68, 0, 0, 248, 66, 98, 186, 89, 77, 68, 68, 20, 162, 194, 42, 153, 195, 67, 58, 106, 186, 193, 135, 70, 41, 67, 157, 224, 115, 67, 98, 13, 96, 218, 193, 104, 81, 235, 67, 243, 198, 99, 194, 8, 94, 78, 68, 70, 137, 213, 66, 112, 211, 134, 68, 98, 109, 211, 138, 67,
		218, 42, 170, 68, 245, 147, 37, 68, 128, 215, 185, 68, 117, 185, 113, 68, 28, 189, 169, 68, 98, 116, 250, 155, 68, 237, 26, 156, 68, 181, 145, 179, 68, 76, 44, 108, 68, 16, 184, 175, 68, 102, 10, 33, 68, 98, 249, 118, 174, 68, 137, 199, 2, 68, 156, 78, 169, 68, 210, 27, 202, 67, 0, 128, 160, 68, 0, 128, 152, 67, 98, 163,
		95, 175, 68, 72, 52, 56, 67, 78, 185, 190, 68, 124, 190, 133, 66, 147, 74, 205, 68, 52, 157, 96, 194, 98, 192, 27, 207, 68, 217, 22, 154, 194, 59, 9, 208, 68, 237, 54, 205, 194, 0, 0, 208, 68, 0, 0, 0, 195, 99, 101, 0, 0 };

	
	searchPath.loadPathFromData(searchIcon, sizeof(searchIcon));
	searchPath.applyTransform(AffineTransform::rotation(float_Pi));

	searchPath.scaleToFit(4.0f, 4.0f, 16.0f, 16.0f, true);

	

	startTimer(500);


}

ScriptComponentList::~ScriptComponentList()
{
	fuzzySearchBox->removeListener(this);

	removeAsScriptEditListener();
	content->removeRebuildListener(this);

	tree->setRootItem(nullptr);
    
    tree = nullptr;
    
    rootItem = nullptr;
    
}

void ScriptComponentList::scriptComponentPropertyChanged(ScriptComponent* sc, Identifier /*idThatWasChanged*/, const var& /*newValue*/)
{
	if (sc != nullptr)
	{
		auto item = tree->findItemFromIdentifierString(sc->name.toString());

		if (item != nullptr)
		{
			item->repaintItem();
		}
	}
}

void ScriptComponentList::scriptComponentSelectionChanged()
{
	if (rootItem != nullptr)
	{
		rootItem->updateSelection(getScriptComponentEditBroadcaster()->getSelection());
	}
}


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
			CreateCppPositionData,
			CopyToAllDevices,
            CollapseAll,
            UnCollapseAll,
			CopyToDeviceOffset,
			EnableConnectionLearn,
			numOptions = CopyToDeviceOffset + 10
		};

		PopupLookAndFeel plaf;
		PopupMenu m;
		m.setLookAndFeel(&plaf);

		const bool somethingSelected = b->getNumSelected() != 0;

		auto lc = b->getCurrentlyLearnedComponent();

		m.addItem(PopupMenuOptions::CreateScriptVariableDeclaration, "Create script variable definition");
		m.addItem(PopupMenuOptions::CreateCustomCallbackDefinition, "Create custom callback definition");

		m.addItem(PopupMenuOptions::CreateCppPositionData, "Copy C++ position data to clipboard", somethingSelected, false);
		m.addItem(PopupMenuOptions::EnableConnectionLearn, "Enable Connection Learn", somethingSelected, lc != nullptr && lc == b->getCurrentlyLearnedComponent());

        m.addItem(PopupMenuOptions::CollapseAll, "Open tree by default", true, defaultOpeness);
		
		const bool isSingleSelection = tree->getNumSelectedItems() == 1;

		
		ValueTree pTree;
		
		if (isSingleSelection)
		{
			pTree = static_cast<ScriptComponentListItem*>(tree->getSelectedItem(0))->tree;

			m.addSectionHeader("Add new Component");
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
			m.addItem((int)ScriptEditHandler::ComponentType::FloatingTile, "Add new FloatingTile");
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
		case CreateCppPositionData:
		{

			raw::Positioner pos(var(componentListToUse.getFirst().get()));

			SystemClipboard::copyTextToClipboard(pos.toString());

			PresetHandler::showMessageWindow("Position data copied", "The position data for " + componentListToUse.getFirst()->getName().toString() + " was copied to the clipboard. Paste it in your C++ files for automatic positioning of equally named component structures");

			break;
		}
		case EnableConnectionLearn:
		{
			b->setCurrentlyLearnedComponent(b->getFirstFromSelection());
			break;
		}
        case CollapseAll:
        {
            defaultOpeness = !defaultOpeness;
            resetRootItem();
            break;
        }

		ADD_SCRIPT_COMPONENT(ScriptEditHandler::ComponentType::Knob, ScriptingApi::Content::ScriptSlider);
		ADD_SCRIPT_COMPONENT(ScriptEditHandler::ComponentType::Button, ScriptingApi::Content::ScriptButton);
		ADD_SCRIPT_COMPONENT(ScriptEditHandler::ComponentType::Label, ScriptingApi::Content::ScriptLabel);
		ADD_SCRIPT_COMPONENT(ScriptEditHandler::ComponentType::AudioWaveform, ScriptingApi::Content::ScriptAudioWaveform);
		ADD_SCRIPT_COMPONENT(ScriptEditHandler::ComponentType::ComboBox, ScriptingApi::Content::ScriptComboBox);
		ADD_SCRIPT_COMPONENT(ScriptEditHandler::ComponentType::FloatingTile, ScriptingApi::Content::ScriptFloatingTile);
		ADD_SCRIPT_COMPONENT(ScriptEditHandler::ComponentType::Image, ScriptingApi::Content::ScriptImage);
		ADD_SCRIPT_COMPONENT(ScriptEditHandler::ComponentType::Panel, ScriptingApi::Content::ScriptPanel);
		ADD_SCRIPT_COMPONENT(ScriptEditHandler::ComponentType::SliderPack, ScriptingApi::Content::ScriptSliderPack);
		ADD_SCRIPT_COMPONENT(ScriptEditHandler::ComponentType::Table, ScriptingApi::Content::ScriptTable);
		ADD_SCRIPT_COMPONENT(ScriptEditHandler::ComponentType::Viewport, ScriptingApi::Content::ScriptedViewport);
		default:
			break;
		}

	}
}

void ScriptComponentList::paint(Graphics& g)
{
	g.fillAll(JUCE_LIVE_CONSTANT_OFF(Colour(0xff262626)));

	g.setColour(Colour(0xff353535));

	auto r = fuzzySearchBox->getBounds().withLeft(0);

	g.fillRect(r);

    PopupLookAndFeel::drawFake3D(g, r);
    
	g.setColour(Colours::white.withAlpha(0.6f));
	g.fillPath(searchPath);
}

void ScriptComponentList::resetRootItem()
{
	auto v = content->getContentProperties();

	tree->setRootItem(nullptr);
	tree->setDefaultOpenness(defaultOpeness);

	rootItem = new ScriptComponentListItem(v, undoManager, content, searchTerm);
    
	tree->setRootItem(rootItem);

	
    if(!defaultOpeness)
        rootItem->setOpen(true);
    
	if (openState != nullptr)
	{
		tree->restoreOpennessState(*openState, false);

		Component::SafePointer<Viewport> tmp = tree->getViewport();
		
		int s = scrollY;

		auto f = [tmp, s]()
		{
			if(tmp.getComponent() != nullptr)
				const_cast<Viewport*>(tmp.getComponent())->setViewPosition(0, s);
		};

		new DelayedFunctionCaller(f, 30);
	}
}

void ScriptComponentList::resized()
{
	Rectangle<int> r(getLocalBounds());

	Rectangle<int> searchBox = r.removeFromTop(24);

	fuzzySearchBox->setBounds(searchBox.withLeft(24));

	r.removeFromBottom(4);
	tree->setBounds(r.reduced(3));
}

#undef ADD_SCRIPT_COMPONENT



bool ScriptComponentList::keyPressed(const KeyPress& key)
{
	if (key == KeyPress::deleteKey || key == KeyPress::backspaceKey)
	{
		deleteSelectedItems();
		return true;
	}

	if (TopLevelWindowWithKeyMappings::matches(this, key, InterfaceDesignerShortcuts::id_deselect_all))
	{
		getScriptComponentEditBroadcaster()->clearSelection(sendNotification);
		return true;
	}

	if (key == KeyPress('z', ModifierKeys::commandModifier, 0))
	{
		getScriptComponentEditBroadcaster()->undo(true);
		
		return true;
	}

	if (key == KeyPress('z', ModifierKeys::commandModifier | ModifierKeys::shiftModifier, 0))
	{
		getScriptComponentEditBroadcaster()->undo(false);

		return true;
	}
	if (TopLevelWindowWithKeyMappings::matches(this, key, InterfaceDesignerShortcuts::id_show_json))
	{
		getScriptComponentEditBroadcaster()->showJSONEditor(this);
		return true;
	}
    if (TopLevelWindowWithKeyMappings::matches(this, key, InterfaceDesignerShortcuts::id_show_panel_data_json))
    {
        return getScriptComponentEditBroadcaster()->showPanelDataJSON(this);
    }
	else if (key.isKeyCode(KeyPress::F2Key))
	{
		auto b = getScriptComponentEditBroadcaster();

		auto sc = b->getFirstFromSelection();

		if (sc == nullptr)
			return false;

		auto oldName = sc->getName().toString();
		auto newName = PresetHandler::getCustomName(oldName);

		if (newName.isNotEmpty() && oldName != newName)
		{
			auto c = sc->getScriptProcessor()->getScriptingContent();

			if (ScriptingApi::Content::Helpers::renameComponent(c, oldName, newName))
			{
				auto f = [b, c, newName]()
				{
					auto nc = c->getComponentWithName(newName);
					b->addToSelection(nc);
				};

				MessageManager::callAsync(f);
			}
		}
	}

	return Component::keyPressed(key);
}


} // namespace hise
