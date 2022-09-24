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

#pragma once

namespace hise { using namespace juce;

namespace ScriptingObjects
{

struct ScriptBroadcaster :  public ConstScriptingObject,
							public WeakCallbackHolder::CallableObject,
							public AssignableDotObject,
							private Timer
{
	ScriptBroadcaster(ProcessorWithScriptingContent* p, const var& defaultValue);;

	struct Panel : public PanelWithProcessorConnection
	{
		Panel(FloatingTile* parent);;

		SET_PANEL_NAME("ScriptBroadcasterMap");

		Identifier getProcessorTypeId() const override;

		Component* createContentComponent(int) override;

		void fillModuleList(StringArray& moduleList) override
		{
			fillModuleListWithType<JavascriptProcessor>(moduleList);
		}
	};
	
	Identifier getObjectName() const override { RETURN_STATIC_IDENTIFIER("Broadcaster"); }

	Component* createPopupComponent(const MouseEvent& e, Component* parent) override;

	Result call(HiseJavascriptEngine* engine, const var::NativeFunctionArgs& args, var* returnValue) override;

	int getNumChildElements() const override { return defaultValues.size(); }

	DebugInformationBase* getChildElement(int index) override;

	bool isAutocompleteable() const override { return true; }

    bool isRealtimeSafe() const override { return realtimeSafe; }
        
	void timerCallback() override;

	// =============================================================================== API Methods

	/** Adds a listener that is notified when a message is send. The object can be either a JSON object, a script object or a simple string. */
	bool addListener(var object, var function);

	/** Adds a listener that will be executed with a delay. */
	bool addDelayedListener(int delayInMilliSeconds, var obj, var function);

	/** Removes the listener that was assigned with the given object. */
	bool removeListener(var objectToRemove);

	/** Sends a message to all listeners. the length of args must match the default value list. if isSync is false, then it will be deferred. */
	void sendMessage(var args, bool isSync);

	/** Sends a message to all listeners with a delay. */
	void sendMessageWithDelay(var args, int delayInMilliseconds);

	/** Resets the state. */
	void reset();

	/** Resends the current state. */
	void resendLastMessage(bool isSync);

	/** Sets a unique ID for the broadcaster. */
	void setId(String name);

	/** Registers this broadcaster to be called when one of the properties of the given components change. */
	void attachToComponentProperties(var componentIds, var propertyIds);

	/** Registers this broadcaster to be called when the value of the given components change. */
	void attachToComponentValue(var componentIds);

	/** Registers this broadcaster to be notified for mouse events for the given components. */
	void attachToComponentMouseEvents(var componentIds, var callbackLevel);

    /** Registers this broadcaster to be notified when a complex data object changes. */
    void attachToComplexData(String dataTypeAndEvent, var moduleIds, var dataIndexes);
        
	/** Registers this broadcaster to be notified when a module parameter changes. */
	void attachToModuleParameter(var moduleIds, var parameterIds);

	/** Registers this broadcaster to be notified when a button of a radio group is clicked. */
	void attachToRadioGroup(int radioGroupIndex);

	/** Attaches this broadcaster to another broadcaster(s) to forward the messages. */
	void attachToOtherBroadcaster(var otherBroadcaster, var argTransformFunction, bool async);

	/** Calls a function after a short period of time. This is exclusive, so if you pass in a new function while another is pending, the first will be replaced. */
	void callWithDelay(int delayInMilliseconds, var argArray, var function);

	/** This will control whether the `this` reference for the listener function will be replaced with the object passed into `addListener`. */
	void setReplaceThisReference(bool shouldReplaceThisReference);

	/** If this is enabled, the broadcaster will keep an internal queue of all messages and will guarantee to send them all. */
	void setEnableQueue(bool shouldUseQueue);

	/** Deactivates the broadcaster so that it will not send messages. If sendMessageIfEnabled is true, it will send the last value when unbypassed. */
	void setBypassed(bool shouldBeBypassed, bool sendMessageIfEnabled, bool async);

    /** Guarantees that the synchronous execution of the listener callbacks can be called from the audio thread. */
    void setRealtimeMode(bool enableRealTimeMode);
        
	// ===============================================================================

	void addBroadcasterAsListener(ScriptBroadcaster* targetBroadcaster, const var& transformFunction, bool async);

	bool assign(const Identifier& id, const var& newValue) override;

	var getDotProperty(const Identifier& id) const override;

private:

	bool bypassed = false;
    bool realtimeSafe = false;
	bool enableQueue = false;
	bool forceSend = false;

	struct DelayedFunction : public Timer
	{
		DelayedFunction(ScriptBroadcaster* b, var f, const Array<var>& args_, int milliSeconds):
			c(b->getScriptProcessor(), f, 0),
			bc(b),
			args(args_)
		{
			c.setHighPriority();
			c.incRefCount();
			startTimer(milliSeconds);
		}

		~DelayedFunction()
		{
			stopTimer();
		}

		void timerCallback() override
		{
			if (bc != nullptr && !bc->bypassed)
			{
				ScopedLock sl(bc->delayFunctionLock);

				c.setThisObject(bc.get());
				c.call(args);
			}
				
			stopTimer();
		}

		Array<var> args;
		WeakCallbackHolder c;
		WeakReference<ScriptBroadcaster> bc;
	};

	CriticalSection delayFunctionLock;
	ScopedPointer<DelayedFunction> currentDelayedFunction;

	std::atomic<bool> asyncPending = { false };

	void handleDebugStuff();

	var pendingData;

	Array<Identifier> argumentIds;

	SimpleReadWriteLock lastValueLock;

	String name;

	uint32 lastMessageTime = 0;

	bool triggerBreakpoint = false;

	struct Display;
	static var getArg(const var& v, int idx);

	Result sendInternal(const Array<var>& args);

	bool cancelIfSame = true;

	Array<var> defaultValues;
	Array<var> lastValues;

	var keepers;

	struct Wrapper;
	
	bool replaceThisReference = true;

	struct ProcessorBypassEvent;

	struct ItemBase
	{
		ItemBase(const var& obj_, const var& f) :
			obj(obj_)
		{
			if (auto dl = dynamic_cast<DebugableObjectBase*>(f.getObject()))
			{
				location = dl->getLocation();
			}
		};

		virtual ~ItemBase() {};

		virtual Result callSync(const Array<var>& args) = 0;

		bool operator==(const ItemBase& other) const
		{
			return obj == other.obj;
		}

		var obj;
		bool enabled = true;
		DebugableObjectBase::Location location;

		JUCE_DECLARE_WEAK_REFERENCEABLE(ItemBase);
	};

	struct Item: ItemBase
	{
		Item(ProcessorWithScriptingContent* p, int numArgs, const var& obj_, const var& f);;

		Result callSync(const Array<var>& args) override;
		WeakCallbackHolder callback;
	};

	struct DelayedItem: public ItemBase
	{
		DelayedItem(ScriptBroadcaster* bc, const var& obj_, const var& f, int milliseconds);
		Result callSync(const Array<var>& args) override;

		int ms;
		var f;

		ScopedPointer<DelayedFunction> delayedFunction;
		WeakReference<ScriptBroadcaster> parent;
	};

	struct OtherBroadcasterTarget : public ItemBase
	{
		OtherBroadcasterTarget(ScriptBroadcaster* parent_, ScriptBroadcaster* target_, const var& transformFunction, bool async);

		Result callSync(const Array<var>& args) override;
		
		const bool async;
		WeakReference<ScriptBroadcaster> parent, target;
		WeakCallbackHolder argTransformFunction;
	};

    struct ListenerBase
    {
        virtual ~ListenerBase() {};
     
		virtual Identifier getListenerType() const = 0;

        virtual Result callItem(ItemBase* n) = 0;
    };
    
	struct ModuleParameterListener: public ListenerBase
	{
		struct ProcessorListener;

		ModuleParameterListener(ScriptBroadcaster* b, const Array<WeakReference<Processor>>& processors, const Array<int>& parameterIndexes);

		Identifier getListenerType() const override { RETURN_STATIC_IDENTIFIER("ModuleParameter"); }

		Result callItem(ItemBase* n) override;

		OwnedArray<ProcessorListener> listeners;
	};
        
    struct ComplexDataListener: public ListenerBase
    {
        struct Item;

		Identifier getListenerType() const override;
            
        ComplexDataListener(ScriptBroadcaster* b,
                            Array<WeakReference<ExternalDataHolder>> list,
                            ExternalData::DataType dataType,
                            bool isDisplayListener,
                            Array<int> indexList, 
							const String& typeString);
            
        Result callItem(ItemBase* n) override;
            
        OwnedArray<Item> items;

		Identifier typeId;
    };

	struct ComponentPropertyListener : public ListenerBase
	{
		struct InternalListener;

		ComponentPropertyListener(ScriptBroadcaster* b, var componentIds, const Array<Identifier>& propertyIds);

		Identifier getListenerType() const override { RETURN_STATIC_IDENTIFIER("ComponentProperties"); }

		Result callItem(ItemBase* n) override;

		Identifier illegalId;
		OwnedArray<InternalListener> items;
	};

	struct MouseEventListener : public ListenerBase
	{
		struct InternalMouseListener;

		MouseEventListener(ScriptBroadcaster* parent, var componentIds, MouseCallbackComponent::CallbackLevel level);

		Identifier getListenerType() const override { RETURN_STATIC_IDENTIFIER("MouseEvents"); }

		// We don't need to call this to update it with the current value because the mouse events are non persistent. */
		Result callItem(ItemBase*) override { return Result::ok(); }

		MouseCallbackComponent::CallbackLevel level;
	};

	struct ComponentValueListener : public ListenerBase
	{
		struct InternalListener;

		ComponentValueListener(ScriptBroadcaster* parent, var componentIds);

		Identifier getListenerType() const override { RETURN_STATIC_IDENTIFIER("ComponentValue"); }

		Result callItem(ItemBase* n) override;

		OwnedArray<InternalListener> items;
	};

	struct RadioGroupListener : public ListenerBase
	{
		struct InternalListener;

		RadioGroupListener(ScriptBroadcaster* b, int radioGroup);

		Identifier getListenerType() const override { RETURN_STATIC_IDENTIFIER("RadioGroup"); }

		void setButtonValueFromIndex(int newIndex);

		Result callItem(ItemBase* n) override;

		int currentIndex = -1;

		const int radioGroup;
		OwnedArray<InternalListener> items;
	};
	
	ScopedPointer<ListenerBase> attachedListener;

	OwnedArray<ItemBase> items;

	Result lastResult;

	void throwIfAlreadyConnected();

	JUCE_DECLARE_WEAK_REFERENCEABLE(ScriptBroadcaster);
};



}


} // namespace hise
