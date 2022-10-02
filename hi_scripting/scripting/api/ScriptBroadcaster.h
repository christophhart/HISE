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
	/** This object holds the metadata for any broadcaster item. */
	struct Metadata
	{
		Metadata() :
			r(Result::fail("uninitialised")),
			hash(0)
		{};

		Metadata(const var& obj, bool mustBeValid) :
			r(Result::ok())
		{
			if (obj.isString())
			{
				id = Identifier(obj.toString());
				c = Colours::grey;
				return;
			}

			if (mustBeValid)
			{
				if (obj.getDynamicObject() == nullptr)
					r = Result::fail("metadata must be a JSON object with `id`, [`commment` and `colour`]");
				else if (obj["id"].toString().isEmpty())
					r = Result::fail("metadata must have at least a id property");
			}

			comment = obj["comment"];

			auto tags_ = obj["tags"];

			if (tags_.isArray())
			{
				for (auto& t : *tags_.getArray())
					tags.add(Identifier(t.toString()));
			}

			auto idString = obj["id"].toString();

			if(idString.isNotEmpty())
				id = Identifier(idString);

			hash = idString.hashCode64();

			if (!obj.hasProperty("colour"))
			{
				c = Colours::lightgrey;
			}
			else if ((int)obj["colour"] == -1)
			{
				c = Colour((uint32)hash).withBrightness(0.7f).withSaturation(0.6f);
			}
			else
				c = scriptnode::PropertyHelpers::getColourFromVar(obj["colour"]);
		}

		operator bool() const { return hash != 0; }

		bool operator==(const Metadata& other) const { return hash == other.hash; }
		bool operator==(const var& other) const { return Metadata(other, true) == *this; }

		var toJSON() const
		{
			DynamicObject::Ptr obj = new DynamicObject();
			obj->setProperty("id", id.toString());
			obj->setProperty("comment", var(comment));
			obj->setProperty("colour", (int64)c.getARGB());

			Array<var> tags_;

			for (auto& t : tags)
				tags_.add(t.toString());

			obj->setProperty("tags", var(tags_));

			return var(obj.get());
		}

		String getErrorMessage() const { return r.getErrorMessage(); }

		Result r;
		String comment;
		Identifier id;
		int64 hash;
		Colour c;
		Array<Identifier> tags;
	};

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
	bool addListener(var object, var metadata, var function);

	/** Adds a listener that will be executed with a delay. */
	bool addDelayedListener(int delayInMilliSeconds, var obj, var metadata, var function);

	/** Adds a listener that sets the properties of the given components when the broadcaster receives a message. */
	bool addComponentPropertyListener(var object, var propertyList, var metadata, var optionalFunction);

	/** Removes the listener that was assigned with the given object. */
	bool removeListener(var idFromMetadata);

	/** Sends a message to all listeners. the length of args must match the default value list. if isSync is false, then it will be deferred. */
	void sendMessage(var args, bool isSync);

	/** Sends a message to all listeners with a delay. */
	void sendMessageWithDelay(var args, int delayInMilliseconds);

	/** Resets the state. */
	void reset();

	/** Resends the current state. */
	void resendLastMessage(bool isSync);

	/** Sets a unique ID for the broadcaster. */
	void setMetadata(var metadata);

	/** Registers this broadcaster to be called when one of the properties of the given components change. */
	void attachToComponentProperties(var componentIds, var propertyIds, var optionalMetadata);

	/** Registers this broadcaster to be called when the value of the given components change. */
	void attachToComponentValue(var componentIds, var optionalMetadata);

	/** Registers this broadcaster to be notified for mouse events for the given components. */
	void attachToComponentMouseEvents(var componentIds, var callbackLevel, var optionalMetadata);

    /** Registers this broadcaster to be notified when a complex data object changes. */
    void attachToComplexData(String dataTypeAndEvent, var moduleIds, var dataIndexes, var optionalMetadata);
        
	/** Registers this broadcaster to be notified when a module parameter changes. */
	void attachToModuleParameter(var moduleIds, var parameterIds, var optionalMetadata);

	/** Registers this broadcaster to be notified when a button of a radio group is clicked. */
	void attachToRadioGroup(int radioGroupIndex, var optionalMetadata);

	/** Attaches this broadcaster to another broadcaster(s) to forward the messages. */
	void attachToOtherBroadcaster(var otherBroadcaster, var argTransformFunction, bool async, var optionalMetadata);

	/** Calls a function after a short period of time. This is exclusive, so if you pass in a new function while another is pending, the first will be replaced. */
	void callWithDelay(int delayInMilliseconds, var argArray, var function);

	/** This will control whether the `this` reference for the listener function will be replaced with the object passed into `addListener`. */
	void setReplaceThisReference(bool shouldReplaceThisReference);

	/** If this is enabled, the broadcaster will keep an internal queue of all messages and will guarantee to send them all. */
	void setEnableQueue(bool shouldUseQueue);

	/** Deactivates the broadcaster so that it will not send messages. If sendMessageIfEnabled is true, it will send the last value when unbypassed. */
	void setBypassed(bool shouldBeBypassed, bool sendMessageIfEnabled, bool async);

	/** Checks if the broadcaster is bypassed. */
	bool isBypassed() const;

    /** Guarantees that the synchronous execution of the listener callbacks can be called from the audio thread. */
    void setRealtimeMode(bool enableRealTimeMode);
        
	// ===============================================================================

	void addBroadcasterAsListener(ScriptBroadcaster* targetBroadcaster, const var& transformFunction, bool async);

	bool assign(const Identifier& id, const var& newValue) override;

	var getDotProperty(const Identifier& id) const override;

	void addAsSource(DebugableObjectBase* b, const Identifier& callbackId) override;

	bool addLocationForFunctionCall(const Identifier& id, const DebugableObjectBase::Location& location) override;

	

private:

	Metadata metadata;

	friend class ScriptBroadcasterMap;

	bool bypassed = false;
    bool realtimeSafe = false;
	bool enableQueue = false;
	bool forceSend = false;

	struct DelayedFunction : public Timer
	{
		DelayedFunction(ScriptBroadcaster* b, var f, const Array<var>& args_, int milliSeconds, const var& thisObj):
			c(b->getScriptProcessor(), b, f, 0),
			bc(b),
			args(args_)
		{
			c.setHighPriority();
			c.incRefCount();

			if (thisObj.isObject() && thisObj.getObject() != b)
				c.setThisObjectRefCounted(thisObj);

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

				c.call(args.getRawDataPointer(), args.size());
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
		ItemBase(const Metadata& m) :
			metadata(m)
		{};

		virtual ~ItemBase() {};

		virtual Identifier getItemId() const = 0;
		virtual void registerSpecialBodyItems(ComponentWithPreferredSize::BodyFactory& factory) {}
		virtual Array<var> createChildArray() const = 0;

		Metadata metadata;
	};

	void sendErrorMessage(ItemBase* i, const String& message, bool throwError=true);

	LambdaBroadcaster<ItemBase*, String> errorBroadcaster;

	struct TargetBase: public ItemBase
	{
		TargetBase(const var& obj_, const var& f, const var& metadata_) :
			ItemBase(Metadata(metadata_, true)),
			obj(obj_)
		{
			if (auto dl = dynamic_cast<DebugableObjectBase*>(f.getObject()))
			{
				location = dl->getLocation();
			}
		};

		virtual ~TargetBase() {};

		virtual Result callSync(const Array<var>& args) = 0;

		bool operator==(const TargetBase& other) const
		{
			return obj == other.obj;
		}

		Array<var> createChildArray() const override
		{
			Array<var> l;

			if (obj.isArray())
				l.addArray(*obj.getArray());
			else
				l.add(obj);

			return l;
		}

		var obj;
		bool enabled = true;
		DebugableObjectBase::Location location;

		JUCE_DECLARE_WEAK_REFERENCEABLE(TargetBase);
	};

	struct ScriptTarget: TargetBase
	{
		ScriptTarget(ScriptBroadcaster* sb, int numArgs, const var& obj_, const var& f, const var& metadata);;

		Identifier getItemId() const override { RETURN_STATIC_IDENTIFIER("Script Callback"); };

		Result callSync(const Array<var>& args) override;
		WeakCallbackHolder callback;
	};

	struct DelayedItem: public TargetBase
	{
		DelayedItem(ScriptBroadcaster* bc, const var& obj_, const var& f, int milliseconds, const var& metadata);
		Result callSync(const Array<var>& args) override;

		Identifier getItemId() const override { RETURN_STATIC_IDENTIFIER("Delayed Callback"); }

		int ms;
		var f;

		ScopedPointer<DelayedFunction> delayedFunction;
		WeakReference<ScriptBroadcaster> parent;
	};

	struct ComponentPropertyItem : public TargetBase
	{
		struct InternalItem;

		ComponentPropertyItem(ScriptBroadcaster* sb, const var& obj, const Array<Identifier>& properties, const var& f, const var& metadata);

		Identifier getItemId() const override { RETURN_STATIC_IDENTIFIER("ComponentProperties"); }

		void registerSpecialBodyItems(ComponentWithPreferredSize::BodyFactory& factory) override;

		Array<var> createChildArray() const override;

		Result callSync(const Array<var>& args) override;

		Array<Identifier> properties;
		ScopedPointer<WeakCallbackHolder> optionalCallback;
	};

	struct OtherBroadcasterTarget : public TargetBase
	{
		OtherBroadcasterTarget(ScriptBroadcaster* parent_, ScriptBroadcaster* target_, const var& transformFunction, bool async, const var& metadata);

		Result callSync(const Array<var>& args) override;
		
		Identifier getItemId() const override { RETURN_STATIC_IDENTIFIER("Other Broadcaster"); }

		const bool async;
		WeakReference<ScriptBroadcaster> parent, target;
		WeakCallbackHolder argTransformFunction;
	};

    struct ListenerBase: public ItemBase
    {
		ListenerBase(const var& metadata_) :
			ItemBase(Metadata(metadata_, false))
		{};

        virtual ~ListenerBase() {};
     
		virtual Result callItem(TargetBase* n) = 0;

		JUCE_DECLARE_WEAK_REFERENCEABLE(ListenerBase);
    };
    
	struct DebugableObjectListener : public ListenerBase
	{
		DebugableObjectListener(const var& metadata, DebugableObjectBase* obj_, const Identifier& callbackId_);;

		Identifier getItemId() const override { return callbackId; }

		Array<var> createChildArray() const override;

		void registerSpecialBodyItems(ComponentWithPreferredSize::BodyFactory& factory) override;

		WeakReference<DebugableObjectBase> obj;
		Identifier callbackId;

		/** Just a dummy for debugging. */
		virtual Result callItem(TargetBase* n) override { return Result::ok(); }
	};

	struct ModuleParameterListener: public ListenerBase
	{
		struct ProcessorListener;

		ModuleParameterListener(ScriptBroadcaster* b, const Array<WeakReference<Processor>>& processors, const Array<int>& parameterIndexes, const var& metadata);

		Identifier getItemId() const override { RETURN_STATIC_IDENTIFIER("ModuleParameter"); }

		void registerSpecialBodyItems(ComponentWithPreferredSize::BodyFactory& factory) override;

		Array<var> createChildArray() const override;

		Result callItem(TargetBase* n) override;

		OwnedArray<ProcessorListener> listeners;
	};

	struct ScriptCallListener : public ListenerBase
	{
		struct ScriptCallItem;

		ScriptCallListener(ScriptBroadcaster* b, const Identifier& id, DebugableObject::Location location);

		Array<var> createChildArray() const override;

		Result callItem(TargetBase* n) override;

		Identifier getItemId() const override { RETURN_STATIC_IDENTIFIER("ScriptFunctionCalls"); }

		void registerSpecialBodyItems(ComponentWithPreferredSize::BodyFactory& factory) override;

		ReferenceCountedArray<ScriptCallItem> items;
	};
        
    struct ComplexDataListener: public ListenerBase
    {
        struct Item;

		Identifier getItemId() const override;
            
        ComplexDataListener(ScriptBroadcaster* b,
                            Array<WeakReference<ExternalDataHolder>> list,
                            ExternalData::DataType dataType,
                            bool isDisplayListener,
                            Array<int> indexList, 
							const String& typeString,
							const var& metadata);
            
        Result callItem(TargetBase* n) override;

		void registerSpecialBodyItems(ComponentWithPreferredSize::BodyFactory& factory) override;

		Array<var> createChildArray() const override;
            
        OwnedArray<Item> items;

		Identifier typeId;
    };

	struct ComponentPropertyListener : public ListenerBase
	{
		struct InternalListener;

		ComponentPropertyListener(ScriptBroadcaster* b, var componentIds, const Array<Identifier>& propertyIds, const var& metadata);

		Identifier getItemId() const override { RETURN_STATIC_IDENTIFIER("ComponentProperties"); }

		void registerSpecialBodyItems(ComponentWithPreferredSize::BodyFactory& factory) override;

		Result callItem(TargetBase* n) override;

		Array<var> createChildArray() const override;

		Array<Identifier> propertyIds;
		Identifier illegalId;
		OwnedArray<InternalListener> items;
	};

	struct MouseEventListener : public ListenerBase
	{
		struct InternalMouseListener;

		MouseEventListener(ScriptBroadcaster* parent, var componentIds, MouseCallbackComponent::CallbackLevel level, const var& metadata);

		Identifier getItemId() const override { RETURN_STATIC_IDENTIFIER("MouseEvents"); }

		// We don't need to call this to update it with the current value because the mouse events are non persistent. */
		Result callItem(TargetBase*) override { return Result::ok(); }

		Array<var> createChildArray() const override;

		OwnedArray<InternalMouseListener> items;
		MouseCallbackComponent::CallbackLevel level;
	};

	struct ComponentValueListener : public ListenerBase
	{
		struct InternalListener;

		ComponentValueListener(ScriptBroadcaster* parent, var componentIds, const var& metadata);

		Identifier getItemId() const override { RETURN_STATIC_IDENTIFIER("ComponentValue"); }

		void registerSpecialBodyItems(ComponentWithPreferredSize::BodyFactory& factory) override;

		Result callItem(TargetBase* n) override;

		Array<var> createChildArray() const override;

		OwnedArray<InternalListener> items;
	};

	struct RadioGroupListener : public ListenerBase
	{
		struct InternalListener;

		RadioGroupListener(ScriptBroadcaster* b, int radioGroup, const var& metadata);

		Identifier getItemId() const override { RETURN_STATIC_IDENTIFIER("RadioGroup"); }

		void registerSpecialBodyItems(ComponentWithPreferredSize::BodyFactory& factory) override;

		void setButtonValueFromIndex(int newIndex);

		Result callItem(TargetBase* n) override;

		Array<var> createChildArray() const override;

		int currentIndex = -1;

		const int radioGroup;
		OwnedArray<InternalListener> items;
	};
	
	struct OtherBroadcasterListener : public ListenerBase
	{
		OtherBroadcasterListener(const Array<WeakReference<ScriptBroadcaster>>& list, const var& metadata) :
			ListenerBase(metadata),
			sources(list)
		{};

		Result callItem(TargetBase* n) override;
		
		Identifier getItemId() const override { RETURN_STATIC_IDENTIFIER("BroadcasterSource"); }

#if USE_BACKEND
		void registerSpecialBodyItems(ComponentWithPreferredSize::BodyFactory& factory) override;
#endif

		Array<var> createChildArray() const override;

		Array<WeakReference<ScriptBroadcaster>> sources;
	};

	void initItem(TargetBase* n);

	ScopedPointer<ListenerBase> attachedListener;

	OwnedArray<TargetBase> items;

	Result lastResult;

	void throwIfAlreadyConnected();

	JUCE_DECLARE_WEAK_REFERENCEABLE(ScriptBroadcaster);
};



}


} // namespace hise
