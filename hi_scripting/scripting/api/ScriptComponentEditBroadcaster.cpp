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


ScriptComponentEditListener::ScriptComponentEditListener(GlobalScriptCompileBroadcaster* gscb) :
	broadcaster(gscb->getScriptComponentEditBroadcaster())
{

}

void ScriptComponentEditListener::addAsScriptEditListener()
{
	broadcaster->addScriptComponentEditListener(this);
}

void ScriptComponentEditListener::removeAsScriptEditListener()
{
	broadcaster->removeScriptComponentEditListener(this);
}

void ScriptComponentEditBroadcaster::addToSelection(ScriptComponent* componentToAdd, NotificationType notifyListeners)
{
	for (int i = 0; i < currentSelection.size(); i++)
	{
		if (currentSelection[i]->sc == componentToAdd)
			return;
	}

    ScopedPointer<ScriptComponentWithLocation> n = new ScriptComponentWithLocation(componentToAdd);
    
    if(n->doc != nullptr)
    {
        currentSelection.add(n.release());
        
        if(notifyListeners)
            sendSelectionChangeMessage();
    }
    else
    {
        PresetHandler::showMessageWindow("Can't find UI definition", "The UI definition for the widget " + componentToAdd->getName().toString() + " can't be found.");
    }
    
	
}

void ScriptComponentEditBroadcaster::removeFromSelection(ScriptComponent* componentToRemove, NotificationType nofifyListeners)
{
	for (int i = 0; i < currentSelection.size(); i++)
	{
		if (currentSelection[i]->sc == componentToRemove)
		{
			currentSelection.remove(i, true);
			return;
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

void ScriptComponentEditBroadcaster::setScriptComponentProperty(ScriptComponent* sc, const Identifier& propertyId, const var& newValue, NotificationType notifyListeners/*=sendNotification*/, bool beginNewTransaction/*=true*/)
{
	if (beginNewTransaction)
		manager.beginNewTransaction(getTransactionName(sc, propertyId, newValue));

	manager.perform(new PropertyChange(this, sc, propertyId, newValue, notifyListeners));
}

void ScriptComponentEditBroadcaster::setPropertyInternal(ScriptComponent* sc, const Identifier& propertyId, const var& newValue, NotificationType notifyListeners)
{
	sc->updateContentPropertyInternal(propertyId, newValue);

	if (isPositionId(propertyId))
	{
		for (int i = 0; i < currentSelection.size(); i++)
		{
			if (currentSelection[i]->sc == sc)
			{
				if (currentSelection[i]->doc != nullptr)
				{
					auto loc = currentSelection[i]->location;

					auto coordinates = sc->getPosition().getPosition();

					JavascriptCodeEditor::Helpers::changeXYOfUIDefinition(sc, currentSelection[i]->doc, loc, coordinates.x, coordinates.y);
					break;
				}
			}


		}

	}

	if (notifyListeners)
		sendPropertyChangeMessage(sc, propertyId, newValue);
}

bool ScriptComponentEditBroadcaster::isSelected(ScriptComponent* sc) const
{
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
	manager.beginNewTransaction(getTransactionName(nullptr, propertyId, newValue));

	Iterator iter(this);

	ScriptComponentSelection thisSelection;

	while (auto sc = iter.getNextScriptComponent())
		thisSelection.add(sc);

	manager.perform(new PropertyChange(this, thisSelection, propertyId, newValue, notifyListeners));

	
}

void ScriptComponentEditBroadcaster::setScriptComponentPropertyDeltaForSelection(const Identifier& propertyId, const var& delta, NotificationType notifyListeners /*= sendNotification*/, bool beginNewTransaction /*= true*/)
{
	manager.beginNewTransaction("Multiple");

	Iterator iter(this);

	while (auto sc = iter.getNextScriptComponent())
	{
		setScriptComponentPropertyDelta(sc, propertyId, delta, notifyListeners, false);
	}
}

void ScriptComponentEditBroadcaster::setScriptComponentPropertyDelta(ScriptComponent* sc, const Identifier& propertyId, const var& delta, NotificationType notifyListeners /*= sendNotification*/, bool beginNewTransaction /*= true*/)
{
	if (beginNewTransaction)
		manager.beginNewTransaction("Delta");

	var oldValue = sc->getScriptObjectProperties()->getProperty(propertyId);

	var newValue = (double)oldValue + (double)delta;

	manager.perform(new PropertyChange(this, sc, propertyId, newValue, notifyListeners));
}

bool ScriptComponentEditBroadcaster::isFirstComponentInSelection(ScriptComponent* sc) const
{
	return getFirstFromSelection() == sc;
}

ScriptComponent* ScriptComponentEditBroadcaster::getFirstFromSelection()
{
	return (currentSelection.size() != 0 ? currentSelection.getFirst()->sc.get() : nullptr);
}

const ScriptComponent* ScriptComponentEditBroadcaster::getFirstFromSelection() const
{
	return (currentSelection.size() != 0 ? currentSelection.getFirst()->sc.get() : nullptr);
}

bool ScriptComponentEditBroadcaster::isPositionId(const Identifier& id)
{
	static const Identifier x("x");
	static const Identifier y("y");

	return id == x || id == y;
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
		p << sc->getScriptObjectProperties()->getProperty(id).toString() << " -> " << newValue.toString();
	}

	return p;
}

void ScriptComponentEditBroadcaster::sendPropertyChangeMessage(ScriptComponent* sc, const Identifier& id, const var& newValue)
{
	for (int i = 0; i < listeners.size(); i++)
	{
		if (listeners[i].get() == nullptr)
		{
			listeners.remove(i--);
		}

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

		listeners[i]->scriptComponentSelectionChanged();
	}
}

ScriptComponentEditBroadcaster::ScriptComponentWithLocation::ScriptComponentWithLocation(ScriptComponent* sc_):
	sc(sc_)
{
	doc = JavascriptCodeEditor::Helpers::getPositionOfUIDefinition(sc, location);
}

