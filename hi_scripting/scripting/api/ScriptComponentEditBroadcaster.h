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

#ifndef SCRIPTCOMPONENTEDITBROADCASTER_H_INCLUDED
#define SCRIPTCOMPONENTEDITBROADCASTER_H_INCLUDED

namespace hise { using namespace juce;

class ScriptComponentEditBroadcaster;

/** A class that will get updated whenever the selection of currently selected components changes.
*
*	
*
*/
class ScriptComponentEditListener
{
public:

	ScriptComponentEditListener(Processor* p);;

	virtual ~ScriptComponentEditListener();

	/** Overwrite this method and update the component to reflect the selection change. */
	virtual void scriptComponentSelectionChanged() = 0;

	/** Overwrite this method and update the component to reflect the property change. */
	virtual void scriptComponentPropertyChanged(ScriptComponent* sc, Identifier idThatWasChanged, const var& newValue) = 0;

	ScriptComponentEditBroadcaster* getScriptComponentEditBroadcaster();

	const ScriptComponentEditBroadcaster* getScriptComponentEditBroadcaster() const;

	const Processor* getProcessor() const;

	Processor* getProcessor();

	/** Overwrite this and update the undo description. */
	virtual void updateUndoDescription();;

protected:

	/** Call this from the constructor and it will register itself. */
	void addAsScriptEditListener();

	/** Call this from the destructor and it will deregister itself. */
	void removeAsScriptEditListener();

private:

	WeakReference<Processor> editedProcessor;

	friend class WeakReference<ScriptComponentEditListener>;
	WeakReference<ScriptComponentEditListener>::Master masterReference;

	ScriptComponentEditBroadcaster* broadcaster;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScriptComponentEditListener)
};




/** This class handles the communication between ScriptComponentEditListeners.
*
*	It holds a list of selected components and updates it's registered listeners
*	whenever the selection or a property changes.
*
*/
class ScriptComponentEditBroadcaster
{
public:

	ScriptComponentEditBroadcaster();

	~ScriptComponentEditBroadcaster();


	class Iterator
	{
	public:

		Iterator(ScriptComponentEditBroadcaster* parent);

		Iterator(const ScriptComponentEditBroadcaster* parent);

		ScriptComponent* getNextScriptComponent();

		const ScriptComponent* getNextScriptComponent() const;

	private:

		ScriptComponent* getNextInternal();

		const ScriptComponent* getNextInternal() const;

		mutable int index = 0;

		ScriptComponentEditBroadcaster* broadcaster;
	};

	/** Call this whenever you add a ScriptComponent to the selection.
	*
	*	It will send a message to all listeners so make sure you don't call this from the callback.
	*/
	void addToSelection(ScriptComponent* componentToAdd, NotificationType notifyListeners=sendNotification);

	/** Call this when you want to remove a component from the selection. */
	void removeFromSelection(ScriptComponent* componentToRemove, NotificationType nofifyListeners = sendNotification);

	/** Call this when you need to clear the selection. */
	void clearSelection(NotificationType notifyListeners = sendNotification);

	/** Sets the selection to the given ScriptComponent. */
	void setSelection(ScriptComponent* componentToSet, NotificationType notifyListeners = sendNotification);

	void setSelection(ScriptComponentSelection newSelection, NotificationType notifyListeners = sendNotification);

	void updateSelectionBasedOnModifier(ScriptComponent* componentToUpdate, const ModifierKeys& mods, NotificationType notifyListeners = sendNotification);

	void addScriptComponentEditListener(ScriptComponentEditListener* listenerToAdd);

	void removeScriptComponentEditListener(ScriptComponentEditListener* listenerToRemove);

	void prepareSelectionForDragging(ScriptComponent* source);

	void setScriptComponentProperty(ScriptComponent* sc, const Identifier& propertyId, const var& newValue, NotificationType notifyListeners=sendNotification, bool beginNewTransaction=true);

	bool isSelected(ScriptComponent* sc) const;

	void setScriptComponentPropertyForSelection(const Identifier& propertyId, const var& newValue, NotificationType notifyListeners);

	void setScriptComponentPropertyDeltaForSelection(const Identifier& propertyId, const var& delta, NotificationType notifyListeners = sendNotification, bool beginNewTransaction = true);

	void setScriptComponentPropertyDelta(ScriptComponent* sc, const Identifier& propertyId, const var& delta, NotificationType notifyListeners = sendNotification, bool beginNewTransaction = true);

	bool isFirstComponentInSelection(ScriptComponent* sc) const;

	ScriptComponentSelection getSelection();

	int getNumSelected() const;

	ScriptComponent* getFirstFromSelection();

	const ScriptComponent* getFirstFromSelection() const;

	static bool isPositionId(const Identifier& id);

	bool isBeingEdited(const Processor* p) const;

	void undo(bool shouldUndo);

	void showJSONEditor(Component* t);

    bool showPanelDataJSON(Component* t);
    
	class PropertyChange : public UndoableAction
	{
	public:

		PropertyChange(ScriptComponentEditBroadcaster* b_, ScriptComponent* sc, const Identifier& id_, const var& newValue_, NotificationType notifyListeners_);

		PropertyChange(ScriptComponentEditBroadcaster* b_, ScriptComponentSelection selection_, const Identifier& id_, const var& newValue_, NotificationType notifyListeners_);

		bool perform() override;

		bool undo() override;

	private:

		ScriptComponentSelection selection;

		ScriptComponentEditBroadcaster* b;
		
		Identifier id;
		Array<var> oldValues;
		var newValue;
		NotificationType notifyListeners;
	};

	UndoManager& getUndoManager();

	static String getTransactionName(ScriptComponent* sc, const Identifier& id, const var& newValue);

	/* @internal. */
	void sendSelectionChangeMessage();

	void setCurrentlyLearnedComponent(ScriptComponent* c);

	void setLearnMode(bool shouldBeEnabled);

	bool learnModeEnabled() const;

	ScriptComponent* getCurrentlyLearnedComponent();

	using LearnBroadcaster = LambdaBroadcaster<ScriptComponent*>;

	LearnBroadcaster& getLearnBroadcaster();

	void setLearnData(const MacroControlledObject::LearnData& d);

private:

	bool learnMode = false;

	WeakReference<ScriptComponent> currentlyLearnedComponent;
	LearnBroadcaster learnBroadcaster;
	

	ScopedPointer<ValueTreeUpdateWatcher> updateWatcher;

	WeakReference<Processor> currentlyEditedProcessor;

	void setPropertyInternal(ScriptComponent* sc, const Identifier& propertyId, const var& newValue, NotificationType notifyListeners);

	UndoManager manager;


	/* @internal. */
	void sendPropertyChangeMessage(ScriptComponent* sc, const Identifier& id, const var& newValue);

	


	Array<WeakReference<ScriptComponentEditListener>> listeners;

	ScriptComponentSelection currentSelection;
	

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScriptComponentEditBroadcaster)
};


} // namespace hise
#endif  // SCRIPTCOMPONENTEDITBROADCASTER_H_INCLUDED
