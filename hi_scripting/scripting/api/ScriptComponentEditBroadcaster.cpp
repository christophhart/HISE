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
*/

namespace hise { using namespace juce;

ScriptComponentEditListener::ScriptComponentEditListener(Processor* p) :
	broadcaster(p->getMainController()->getScriptComponentEditBroadcaster()),
	editedProcessor(p)
{

}

ScriptComponentEditListener::~ScriptComponentEditListener()
{
	masterReference.clear();
}

ScriptComponentEditBroadcaster* ScriptComponentEditListener::getScriptComponentEditBroadcaster()
{ return broadcaster; }

const ScriptComponentEditBroadcaster* ScriptComponentEditListener::getScriptComponentEditBroadcaster() const
{ return broadcaster; }

const Processor* ScriptComponentEditListener::getProcessor() const
{
	return editedProcessor.get();
}

Processor* ScriptComponentEditListener::getProcessor()
{
	return editedProcessor.get();
}

void ScriptComponentEditListener::updateUndoDescription()
{}

void ScriptComponentEditListener::addAsScriptEditListener()
{
	broadcaster->addScriptComponentEditListener(this);
}

void ScriptComponentEditListener::removeAsScriptEditListener()
{
	broadcaster->removeScriptComponentEditListener(this);
}

ScriptComponentEditBroadcaster::ScriptComponentEditBroadcaster()
{}

ScriptComponentEditBroadcaster::~ScriptComponentEditBroadcaster()
{
	clearSelection(dontSendNotification);
	manager.clearUndoHistory();
}

ScriptComponentEditBroadcaster::Iterator::Iterator(ScriptComponentEditBroadcaster* parent):
	broadcaster(parent)
{

}

ScriptComponentEditBroadcaster::Iterator::Iterator(const ScriptComponentEditBroadcaster* parent):
	broadcaster(const_cast<ScriptComponentEditBroadcaster*>(parent))
{

}

ScriptComponent* ScriptComponentEditBroadcaster::Iterator::getNextScriptComponent()
{
	while (auto r = getNextInternal())
		return r;

	return nullptr;
}

const ScriptComponent* ScriptComponentEditBroadcaster::Iterator::getNextScriptComponent() const
{
	while (auto r = getNextInternal())
		return r;

	return nullptr;
}

ScriptComponent* ScriptComponentEditBroadcaster::Iterator::getNextInternal()
{
	while (index < broadcaster->currentSelection.size())
	{
		if (auto r = broadcaster->currentSelection[index++])
			return r.get();
	}

	return nullptr;
}

const ScriptComponent* ScriptComponentEditBroadcaster::Iterator::getNextInternal() const
{
	while (index < broadcaster->currentSelection.size())
	{
		if (auto r = broadcaster->currentSelection[index++])
			return r.get();
	}

	return nullptr;
}

void ScriptComponentEditBroadcaster::addScriptComponentEditListener(ScriptComponentEditListener* listenerToAdd)
{
	listeners.addIfNotAlreadyThere(listenerToAdd);
}

void ScriptComponentEditBroadcaster::removeScriptComponentEditListener(ScriptComponentEditListener* listenerToRemove)
{
	listeners.removeAllInstancesOf(listenerToRemove);
}

ScriptComponentSelection ScriptComponentEditBroadcaster::getSelection()
{ return currentSelection; }

int ScriptComponentEditBroadcaster::getNumSelected() const
{ return currentSelection.size(); }

bool ScriptComponentEditBroadcaster::isBeingEdited(const Processor* p) const
{
	return currentlyEditedProcessor.get() == p;
}

ScriptComponentEditBroadcaster::PropertyChange::PropertyChange(ScriptComponentEditBroadcaster* b_, ScriptComponent* sc,
	const Identifier& id_, const var& newValue_, NotificationType notifyListeners_):
	b(b_),
	id(id_),
	newValue(newValue_),
	notifyListeners(notifyListeners_)
{
	selection.add(sc);
}

ScriptComponentEditBroadcaster::PropertyChange::PropertyChange(ScriptComponentEditBroadcaster* b_,
	ScriptComponentSelection selection_, const Identifier& id_, const var& newValue_, NotificationType notifyListeners_):
	b(b_),
	id(id_),
	newValue(newValue_),
	notifyListeners(notifyListeners_)
{
	selection = selection_;
}

bool ScriptComponentEditBroadcaster::PropertyChange::perform()
{
	for (auto sc : selection)
	{
		if (sc != nullptr)
		{
			oldValues.add(sc->getScriptObjectProperty(id));
			b->setPropertyInternal(sc, id, newValue, notifyListeners);
		}
		else
		{
			return false;
		}
	}

	return true;
}

bool ScriptComponentEditBroadcaster::PropertyChange::undo()
{
	for (int i = 0; i < selection.size(); i++)
	{
		if (auto sc = selection[i])
		{
			var oldValue = oldValues[i];
			b->setPropertyInternal(sc.get(), id, oldValue, notifyListeners);
		}
		else
		{
			return false;
		}
	}

	return true;
}

UndoManager& ScriptComponentEditBroadcaster::getUndoManager()
{
	return manager;
}

bool ScriptComponentEditBroadcaster::learnModeEnabled() const
{ return learnMode; }

ScriptComponent* ScriptComponentEditBroadcaster::getCurrentlyLearnedComponent()
{ return currentlyLearnedComponent.get(); }

ScriptComponentEditBroadcaster::LearnBroadcaster& ScriptComponentEditBroadcaster::getLearnBroadcaster()
{ return learnBroadcaster; }

void ScriptComponentEditBroadcaster::addToSelection(ScriptComponent* componentToAdd, NotificationType notifyListeners)
{
	if (componentToAdd == nullptr)
		return;

	for (int i = 0; i < currentSelection.size(); i++)
	{
		if (currentSelection[i].get() == componentToAdd)
			return;

		if (componentToAdd->getParentScriptComponent() == currentSelection[i].get())
			return;

		if (currentSelection[i]->getParentScriptComponent() == componentToAdd)
			currentSelection.remove(i--);
	}

	currentSelection.add(componentToAdd);

	currentlyEditedProcessor = dynamic_cast<Processor*>(componentToAdd->getScriptProcessor());

	if (notifyListeners)
		sendSelectionChangeMessage();
	
}

void ScriptComponentEditBroadcaster::removeFromSelection(ScriptComponent* componentToRemove, NotificationType nofifyListeners)
{
	if (componentToRemove == nullptr)
		return;

	for (int i = 0; i < currentSelection.size(); i++)
	{
		if (currentSelection[i].get() == componentToRemove)
		{
			currentSelection.remove(i);
			break;
		}
			
	}
	
	if(nofifyListeners)
		sendSelectionChangeMessage();
}

void ScriptComponentEditBroadcaster::clearSelection(NotificationType notifyListeners)
{
	currentSelection.clear();

	if(notifyListeners)
		sendSelectionChangeMessage();
}

void ScriptComponentEditBroadcaster::setSelection(ScriptComponent* componentToSet, NotificationType notifyListeners /*= sendNotification*/)
{
	if (componentToSet == nullptr)
	{
		clearSelection(notifyListeners);
	}
	else
	{
		clearSelection(dontSendNotification);
		addToSelection(componentToSet, notifyListeners);
	}

	
}

void ScriptComponentEditBroadcaster::setSelection(ScriptComponentSelection newSelection, NotificationType notifyListeners /*= sendNotification*/)
{
	bool isEqual = newSelection.size() == currentSelection.size();

	if (isEqual)
	{
		for (int i = 0; i < newSelection.size(); i++)
		{
			if (newSelection[i] != currentSelection[i])
			{
				isEqual = false;
				break;
			}
		}
	}

	if (!isEqual)
	{
		currentSelection.swapWith(newSelection);

		if (notifyListeners)
			sendSelectionChangeMessage();
	}
}

void ScriptComponentEditBroadcaster::updateSelectionBasedOnModifier(ScriptComponent* componentToUpdate, const ModifierKeys& mods, NotificationType notifyListeners /*= sendNotification*/)
{
	if (mods.isCommandDown())
	{
		if(isSelected(componentToUpdate))
		{
			removeFromSelection(componentToUpdate, notifyListeners);
		}
		else
		{
			addToSelection(componentToUpdate, notifyListeners);
		}
	}
	else
	{
		setSelection(componentToUpdate, notifyListeners);
	}
}

struct ScriptComponentSorter
{
	int compareElements(ScriptComponent* first, ScriptComponent* second)
	{
		jassert(first->parent == second->parent);

		auto firstIndex = first->parent->getComponentIndex(first->getName());
		auto secondIndex = second->parent->getComponentIndex(second->getName());

		if (firstIndex < secondIndex)
			return -1;

		if (firstIndex > secondIndex)
			return 1;

		return 0;

	}

#if 0
	@endcode

		..and this method must return:
	-a value of < 0 if the first comes before the second
		- a value of 0 if the two objects are equivalent
		- a value of > 0 if the second comes before the first
#endif
};

void addChildrenToSelection(ScriptComponentEditBroadcaster* /*b*/, ScriptComponent* /*sc*/)
{
	jassertfalse;
}

void ScriptComponentEditBroadcaster::prepareSelectionForDragging(ScriptComponent* source)
{
	addToSelection(source, dontSendNotification);

	Iterator iter(this);

	while (auto sc = iter.getNextScriptComponent())
	{
		addChildrenToSelection(this, sc);
	}

	ScriptComponentSorter sorter;

	currentSelection.sort(sorter, false);

	sendSelectionChangeMessage();
}

void ScriptComponentEditBroadcaster::setScriptComponentProperty(ScriptComponent* sc, const Identifier& propertyId, const var& newValue, NotificationType notifyListeners/*=sendNotification*/, bool /*beginNewTransaction*//*=true*/)
{
	manager.perform(new PropertyChange(this, sc, propertyId, newValue, notifyListeners));
}

void ScriptComponentEditBroadcaster::setPropertyInternal(ScriptComponent* sc, const Identifier& propertyId, const var& newValue, NotificationType notifyListeners)
{
	sc->updateContentPropertyInternal(propertyId, newValue);

	if (notifyListeners)
		sendPropertyChangeMessage(sc, propertyId, newValue);
}

bool ScriptComponentEditBroadcaster::isSelected(ScriptComponent* sc) const
{
	if (sc == nullptr)
		return false;

	Iterator iter(this);

	while (auto sc_ = iter.getNextScriptComponent())
	{
		if (sc == sc_)
			return true;
	}

	return false;
}

void ScriptComponentEditBroadcaster::setScriptComponentPropertyForSelection(const Identifier& propertyId, const var& newValue, NotificationType notifyListeners)
{
	Iterator iter(this);

	ScriptComponentSelection thisSelection;

	while (auto sc = iter.getNextScriptComponent())
		thisSelection.add(sc);

    try
    {
        manager.perform(new PropertyChange(this, thisSelection, propertyId, newValue, notifyListeners));
    }
    catch(String& s)
    {
        if(currentlyEditedProcessor.get())
        {
            debugError(currentlyEditedProcessor, s);
        }
    }
	

	
}

void ScriptComponentEditBroadcaster::setScriptComponentPropertyDeltaForSelection(const Identifier& propertyId, const var& delta, NotificationType notifyListeners /*= sendNotification*/, bool /*beginNewTransaction*/ /*= true*/)
{
	Iterator iter(this);

	while (auto sc = iter.getNextScriptComponent())
	{
		setScriptComponentPropertyDelta(sc, propertyId, delta, notifyListeners, false);
	}
}

void ScriptComponentEditBroadcaster::setScriptComponentPropertyDelta(ScriptComponent* sc, const Identifier& propertyId, const var& delta, NotificationType notifyListeners /*= sendNotification*/, bool /*beginNewTransaction*/ /*= true*/)
{
	var oldValue = sc->getScriptObjectProperty(propertyId);

	var newValue = (double)oldValue + (double)delta;

	manager.perform(new PropertyChange(this, sc, propertyId, newValue, notifyListeners));
}

bool ScriptComponentEditBroadcaster::isFirstComponentInSelection(ScriptComponent* sc) const
{
	return getFirstFromSelection() == sc;
}

ScriptComponent* ScriptComponentEditBroadcaster::getFirstFromSelection()
{
	return currentSelection.getFirst().get();
}

const ScriptComponent* ScriptComponentEditBroadcaster::getFirstFromSelection() const
{
	return currentSelection.getFirst().get();
}

bool ScriptComponentEditBroadcaster::isPositionId(const Identifier& id)
{
	static const Identifier x("x");
	static const Identifier y("y");

	return id == x || id == y;
}

void ScriptComponentEditBroadcaster::undo(bool shouldUndo)
{
	auto jp = dynamic_cast<JavascriptProcessor*>(currentlyEditedProcessor.get());

	if (jp == nullptr)
		return;

	auto content = jp->getContent();

	ValueTreeUpdateWatcher::ScopedDelayer sd(content->getUpdateWatcher());

	if (shouldUndo)
		manager.undo();
	else
		manager.redo();

	for (auto l : listeners)
	{
		if (l.get() != nullptr)
			l->updateUndoDescription();
	}
}

void ScriptComponentEditBroadcaster::showJSONEditor(Component* t)
{
	ScriptingApi::Content* content = nullptr;

	if (auto sc = getFirstFromSelection())
		content = sc->getScriptProcessor()->getScriptingContent();
	else
		return;

	Array<var> list;

	for (auto sc : getSelection())
	{
		auto v = ValueTreeConverters::convertContentPropertiesToDynamicObject(sc->getPropertyValueTree());
		list.add(v);
	}

	JSONEditor* editor = new JSONEditor(var(list));

	editor->setEditable(true);

	auto callback = [content, this](const var& newData)
	{
		if (auto ar = newData.getArray())
		{
			auto selection = this->getSelection();

			jassert(ar->size() == selection.size());

			auto undoManager = &this->getUndoManager();

			ValueTreeUpdateWatcher::ScopedDelayer sd(content->getUpdateWatcher());

			for (int i = 0; i < selection.size(); i++)
			{
				auto sc = selection[i];

				auto newJson = ar->getUnchecked(i);

				ScriptingApi::Content::Helpers::setComponentValueTreeFromJSON(content, sc->name, newJson, undoManager);
			}
		}

		return;
	};

	editor->setCallback(callback, true);
	editor->setName("Editing JSON");
	editor->setSize(400, 400);

	t->findParentComponentOfClass<FloatingTile>()->showComponentInRootPopup(editor, t, t->getLocalBounds().getCentre());

	editor->grabKeyboardFocus();
}

bool ScriptComponentEditBroadcaster::showPanelDataJSON(juce::Component *t)
{
    auto fc = getFirstFromSelection();
    
    var pd;
    
    JSONEditor* editor = nullptr;
    
    if(auto sp = dynamic_cast<ScriptingApi::Content::ScriptPanel*>(fc))
    {
        auto pd = sp->getConstantValue(0);
        
        editor = new JSONEditor(pd);

        auto callback = [sp, pd](const var& newData)
        {
            auto copy = newData.clone();
            auto& prop = copy.getDynamicObject()->getProperties();
            pd.getDynamicObject()->swapProperties(std::move(prop));
            sp->repaint();
            return;
        };
        
        editor->setCallback(callback, false);
        
        editor->setName("Editing Panel.data (non-persistent!)");
    }
    if(auto vp = dynamic_cast<ScriptingApi::Content::ScriptedViewport*>(fc))
    {
        if(auto tm = vp->getTableModel())
        {
            auto pd = tm->getRowData();
            editor = new JSONEditor(pd);
            
            
            
            auto callback = [vp](const var& newData)
            {
                auto mc = vp->getScriptProcessor()->getMainController_();
                
                {
                    LockHelpers::SafeLock(mc, LockHelpers::ScriptLock);
                    vp->getTableModel()->setRowData(newData);
                }
                vp->sendRepaintMessage();
                
                return;
            };
            
            editor->setCallback(callback, false);
            
            editor->setName("Editing Viewport table rows (non-persistent!)");
        }
    }
    
    if(editor != nullptr)
    {
        editor->setEditable(true);
        
        
        editor->setSize(400, 400);

        t->findParentComponentOfClass<FloatingTile>()->showComponentInRootPopup(editor, t, t->getLocalBounds().getCentre());

        editor->grabKeyboardFocus();
        
        return true;
    }
    
    return false;
}

String ScriptComponentEditBroadcaster::getTransactionName(ScriptComponent* sc, const Identifier& id, const var& newValue)
{
	String p;

	if (sc == nullptr) // all
	{
		p << "Property Change for selection: " << id.toString() << " -> " << newValue.toString();
	}
	else
	{
		p << sc->getName().toString() << "." << id.toString() << ": ";
		p << sc->getScriptObjectProperty(id).toString() << " -> " << newValue.toString();
	}

	return p;
}

void ScriptComponentEditBroadcaster::sendPropertyChangeMessage(ScriptComponent* /*sc*/, const Identifier& id, const var& newValue)
{
	for (int i = 0; i < listeners.size(); i++)
	{
		if (listeners[i].get() == nullptr)
		{
			listeners.remove(i--);
		}

		if (listeners[i]->getProcessor() != currentlyEditedProcessor.get())
			continue;

		Iterator iter(this);

		while (auto sc = iter.getNextScriptComponent())
		{
			listeners[i]->scriptComponentPropertyChanged(sc, id, newValue);
		}
	}
}

void ScriptComponentEditBroadcaster::sendSelectionChangeMessage()
{
	for (int i = 0; i < listeners.size(); i++)
	{
		if (listeners[i].get() == nullptr)
		{
			listeners.remove(i--);
		}

		if (listeners[i]->getProcessor() != currentlyEditedProcessor.get())
			continue;

		listeners[i]->scriptComponentSelectionChanged();
	}
}


void ScriptComponentEditBroadcaster::setCurrentlyLearnedComponent(ScriptComponent* c)
{
	if (c != currentlyLearnedComponent)
	{
		currentlyLearnedComponent = c;
		learnBroadcaster.sendMessage(sendNotificationSync, currentlyLearnedComponent);
	}
}

void ScriptComponentEditBroadcaster::setLearnMode(bool shouldBeEnabled)
{
	learnMode = shouldBeEnabled;

	if (!learnMode)
		setCurrentlyLearnedComponent(nullptr);
	else if (getNumSelected() == 1)
		setCurrentlyLearnedComponent(getFirstFromSelection());
}

void ScriptComponentEditBroadcaster::setLearnData(const MacroControlledObject::LearnData& d)
{
	if (currentlyLearnedComponent != nullptr)
	{
		auto c = currentlyLearnedComponent.get();

		c->setControlCallback(var());

		if (!d.mode.isEmpty() && dynamic_cast<ScriptingApi::Content::ScriptSlider*>(c) != nullptr)
			c->setScriptObjectPropertyWithChangeMessage(c->getIdFor(ScriptingApi::Content::ScriptSlider::Mode), d.mode);

		if (dynamic_cast<ScriptingApi::Content::ScriptComboBox*>(c) != nullptr)
		{
			c->setScriptObjectPropertyWithChangeMessage(c->getIdFor(ScriptingApi::Content::ScriptComboBox::Items), d.items.joinIntoString("\n"));
		}

		c->setScriptObjectPropertyWithChangeMessage(c->getIdFor(ScriptComponent::Properties::text), d.name);
		c->setScriptObjectPropertyWithChangeMessage(c->getIdFor(ScriptComponent::Properties::min), d.range.start);
		c->setScriptObjectPropertyWithChangeMessage(c->getIdFor(ScriptComponent::Properties::max), d.range.end);

		if (dynamic_cast<ScriptingApi::Content::ScriptSlider*>(c) != nullptr)
		{
			if (d.range.skew != 1.0)
			{
				auto end = d.range.end;
				auto start = d.range.start;
				auto skew = d.range.skew;

				auto centrePointValue  = std::exp(std::log(0.5) / skew) * (end - start) + start;
				c->setScriptObjectPropertyWithChangeMessage(c->getIdFor(ScriptingApi::Content::ScriptSlider::middlePosition), centrePointValue);
			}
		}
			
			

		c->setValue(d.value);
		c->setScriptObjectPropertyWithChangeMessage(c->getIdFor(ScriptComponent::Properties::processorId), d.processorId);
		c->setScriptObjectPropertyWithChangeMessage(c->getIdFor(ScriptComponent::Properties::parameterId), d.parameterId);

		setCurrentlyLearnedComponent(nullptr);
	}
}

} // namespace hise

using namespace hise;


