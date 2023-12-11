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

#if PERFETTO
#define OPEN_BROADCASTER_TRACK(item, root){ dispatch::StringBuilder b; b << metadata.id << ": call " << i->metadata.id; \
											item->pendingTrack = root.bumpFlowCounter(); \
											TRACE_EVENT("dispatch", DYNAMIC_STRING_BUILDER(b), perfetto::Flow::ProcessScoped(item->pendingTrack)); }

#define TERMINATE_BROADCASTER_TRACK(x) dispatch::StringBuilder n; n << metadata.id << "(" << getItemId() << ")" << x; \
			TRACE_EVENT("dispatch", DYNAMIC_STRING_BUILDER(n), perfetto::TerminatingFlow::ProcessScoped(pendingTrack)); 

#define CONTINUE_BROADCASTER_TRACK(x) dispatch::StringBuilder n; n << metadata.id << "(" << getItemId() << ")" << x; \
			TRACE_EVENT("dispatch", DYNAMIC_STRING_BUILDER(n), perfetto::Flow::ProcessScoped(pendingTrack));
#else
#define OPEN_BROADCASTER_TRACK(item, root);
#define TERMINATE_BROADCASTER_TRACK(x); 
#define CONTINUE_BROADCASTER_TRACK(x);
#endif

struct ScriptBroadcaster :  public ConstScriptingObject,
							public WeakCallbackHolder::CallableObject,
							public AssignableDotObject,
							private Timer
{
	/** This object holds the metadata for any broadcaster item. */
	struct Metadata
	{
		Metadata();;

		Metadata(const var& obj, bool mustBeValid);

		operator bool() const;

		bool operator==(const Metadata& other) const;
		bool operator==(const var& other) const;

		void attachCommentFromCallableObject(const var& callableObject, bool useDebugInformation=false);

		var toJSON() const;

		String getErrorMessage() const;

		Result r;
		String comment;
		Identifier id;
		int64 hash = 0;
		Colour c;
		int priority = 0;
		Array<Identifier> tags;
		bool visible;
	};

	ScriptBroadcaster(ProcessorWithScriptingContent* p, const var& defaultValue);;
	~ScriptBroadcaster();

	
	
	Identifier getObjectName() const override;

	Component* createPopupComponent(const MouseEvent& e, Component* parent) override;

	Result call(HiseJavascriptEngine* engine, const var::NativeFunctionArgs& args, var* returnValue) override;

	int getNumChildElements() const override;

	DebugInformationBase* getChildElement(int index) override;

	bool isAutocompleteable() const override;

	bool isRealtimeSafe() const override;

	bool allowRefCount() const override;;
        
	void timerCallback() override;

	// =============================================================================== API Methods

	/** Adds a listener that is notified when a message is send. The object can be either a JSON object, a script object or a simple string. */
	bool addListener(var object, var metadata, var function);

	/** Adds a listener that will be executed with a delay. */
	bool addDelayedListener(int delayInMilliSeconds, var obj, var metadata, var function);

	/** Adds a listener that sets the properties of the given components when the broadcaster receives a message. */
	bool addComponentPropertyListener(var object, var propertyList, var metadata, var optionalFunction);

	/** Adds a listener that sets the value of the given components when the broadcaster receives a message. */
	bool addComponentValueListener(var object, var metadata, var optionalFunction);

	/** Adds a listener that will cause a refresh message (eg. repaint(), changed()) to be send out to the given components. */
	bool addComponentRefreshListener(var componentIds, String refreshType, var metadata);

	/** Removes the listener that was assigned with the given object. */
	bool removeListener(var idFromMetadata);

	/** Removes the source with the given metadata. */
	bool removeSource(var metadata);

	/** Removes all listeners. */
	void removeAllListeners();

	/** Removes all sources. */
	void removeAllSources();

	/** deprecated function (use sendSyncMessage / sendAsyncMessage instead). */
	void sendMessage(var args, bool isSync);

	/** Sends a synchronous message to all listeners (same as the dot assignment operator). the length of args must match the default value list. */
	void sendSyncMessage(var args);

	/** Sends an asynchronous message to all listeners. the length of args must match the default value list. */
	void sendAsyncMessage(var args);

	/** Sends a message to all listeners with a delay. */
	void sendMessageWithDelay(var args, int delayInMilliseconds);

	/** Resets the state. */
	void reset();

	/** Resends the current state. */
	void resendLastMessage(var isSync);

	/** Forces every message to be sent synchronously. */
	void setForceSynchronousExecution(bool shouldExecuteSynchronously);

	/** Forces the broadcaster to also send a message when a parameter is undefined. */
	void setSendMessageForUndefinedArgs(bool shouldSendWhenUndefined);

	/** Registers this broadcaster to be called when one of the properties of the given components change. */
	void attachToComponentProperties(var componentIds, var propertyIds, var optionalMetadata);

	/** Registers this broadcaster to be called when the value of the given components change. */
	void attachToComponentValue(var componentIds, var optionalMetadata);

	/** Registers this broadcaster to be called when the visibility of one of the components (or one of its parent component) changes. */
	void attachToComponentVisibility(var componentIds, var optionalMetadata);

	/** Registers this broadcaster to be notified for mouse events for the given components. */
	void attachToComponentMouseEvents(var componentIds, var callbackLevel, var optionalMetadata);

	/** Registers this broadcaster to be notified when a context menu item from the given components was selected. */
	void attachToContextMenu(var componentIds, var stateFunction, var itemList, var optionalMetadata, var useLeftClick);

    /** Registers this broadcaster to be notified when a complex data object changes. */
    void attachToComplexData(String dataTypeAndEvent, var moduleIds, var dataIndexes, var optionalMetadata);
        
	/** Registers this broadcaster to be notified about changes to the EQ (adding / removing / selecting filter bands). */
	void attachToEqEvents(var moduleIds, var eventTypes, var optionalMetadata);

	/** Registers this broadcaster to be notified when a module parameter changes. */
	void attachToModuleParameter(var moduleIds, var parameterIds, var optionalMetadata);

	/** Registers this broadcaster to be notified when a button of a radio group is clicked. */
	void attachToRadioGroup(int radioGroupIndex, var optionalMetadata);

	/** Attaches this broadcaster to another broadcaster(s) to forward the messages. */
	void attachToOtherBroadcaster(var otherBroadcaster, var argTransformFunction, bool async, var optionalMetadata);

	/** Attaches this broadcaster to a routing matrix and listens for changes. */
	void attachToRoutingMatrix(var moduleIds, var optionalMetadata);

	/** Attaches this broadcaster to changes of the audio processing specs (samplerate / buffer size). */
	void attachToProcessingSpecs(var optionalMetadata);

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

	/** If this broadcaster is attached to a context menu, calling this method will update the states for the menu items. */
	void refreshContextMenuState();

	// ===============================================================================

	void addBroadcasterAsListener(ScriptBroadcaster* targetBroadcaster, const var& transformFunction, bool async);

	bool assign(const Identifier& id, const var& newValue) override;

	var getDotProperty(const Identifier& id) const override;

	void addAsSource(DebugableObjectBase* b, const Identifier& callbackId) override;

	bool addLocationForFunctionCall(const Identifier& id, const DebugableObjectBase::Location& location) override;

	const Metadata& getMetadata() const;

	static bool isPrimitiveArray(const var& obj);

private:

	void sendMessageInternal(var args, bool isSync);

	bool forceSync = false;
	bool sendWhenUndefined = false;

	Metadata metadata;

	friend class ScriptBroadcasterMap;

	bool bypassed = false;
    bool realtimeSafe = false;
	bool enableQueue = false;
	bool forceSend = false;

	struct DelayedFunction : public Timer
	{
		DelayedFunction(ScriptBroadcaster* b, var f, const Array<var>& args_, int milliSeconds, const var& thisObj);

		~DelayedFunction();

		void timerCallback() override;

		Array<var> args;
		WeakCallbackHolder c;
		WeakReference<ScriptBroadcaster> bc;
		uint64_t trackIndex = 0;

		bool busy = false;
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
		struct PrioritySorter
		{
			static int compareElements(ItemBase* m1, ItemBase* m2);
		};

		ItemBase(const Metadata& m);;

		virtual ~ItemBase();;

		virtual Identifier getItemId() const = 0;
		virtual void registerSpecialBodyItems(ComponentWithPreferredSize::BodyFactory& factory);
		virtual Array<var> createChildArray() const = 0;

		Metadata metadata;
	};

	void sendErrorMessage(ItemBase* i, const String& message, bool throwError=true);

	LambdaBroadcaster<ItemBase*, String> errorBroadcaster;

	struct TargetBase: public ItemBase
	{
		TargetBase(const var& obj_, const var& f, const var& metadata_);;

		virtual ~TargetBase();;

		virtual Result callSync(const Array<var>& args) = 0;

		bool operator==(const TargetBase& other) const;

		Array<var> createChildArray() const override;

		var obj;
		bool enabled = true;
		DebugableObjectBase::Location location;

		uint64_t pendingTrack = 0;

		JUCE_DECLARE_WEAK_REFERENCEABLE(TargetBase);
	};

	struct ScriptTarget: TargetBase
	{
		ScriptTarget(ScriptBroadcaster* sb, int numArgs, const var& obj_, const var& f, const var& metadata);;

		Identifier getItemId() const override;;

		void registerSpecialBodyItems(ComponentWithPreferredSize::BodyFactory& factory) override;

		Array<var> createChildArray() const override;

		Result callSync(const Array<var>& args) override;
		WeakCallbackHolder callback;
	};

	struct DelayedItem: public TargetBase
	{
		DelayedItem(ScriptBroadcaster* bc, const var& obj_, const var& f, int milliseconds, const var& metadata);
		Result callSync(const Array<var>& args) override;

		Identifier getItemId() const override;

		int ms;
		var f;

		ScopedPointer<DelayedFunction> delayedFunction;
		WeakReference<ScriptBroadcaster> parent;
	};

	struct ComponentPropertyItem : public TargetBase
	{
		struct InternalItem;

		ComponentPropertyItem(ScriptBroadcaster* sb, const var& obj, const Array<Identifier>& properties, const var& f, const var& metadata);

		Identifier getItemId() const override;

		void registerSpecialBodyItems(ComponentWithPreferredSize::BodyFactory& factory) override;

		Array<var> createChildArray() const override;

		Result callSync(const Array<var>& args) override;

		Array<Identifier> properties;
		ScopedPointer<WeakCallbackHolder> optionalCallback;
	};

	struct ComponentRefreshItem : public TargetBase
	{
		enum class RefreshType
		{
			repaint,
			changed,
			updateValueFromProcessorConnection,
			loseFocus,
			resetValueToDefault,
			numRefreshTypes
		};

		ComponentRefreshItem(ScriptBroadcaster* sb, const var& obj, const String refreshMode, const var& metadata);


		Identifier getItemId() const override;

		Array<var> createChildArray() const override;

		void registerSpecialBodyItems(ComponentWithPreferredSize::BodyFactory& factory) override;;

		Result callSync(const Array<var>& args) override;

		struct RefCountedTime : public ReferenceCountedObject
		{
			using List = ReferenceCountedArray<RefCountedTime>;
			using Ptr = ReferenceCountedObjectPtr<RefCountedTime>;

			uint32 lastTime = 0;
		};

		RefCountedTime::List timeSlots;
		String refreshModeString;

		RefreshType refreshMode = RefreshType::numRefreshTypes;
	};

	struct ComponentValueItem : public TargetBase
	{
		ComponentValueItem(ScriptBroadcaster* sb, const var& obj, const var& f, const var& metadata);

		Identifier getItemId() const override;

		void registerSpecialBodyItems(ComponentWithPreferredSize::BodyFactory& factory) override;;
		
		Array<var> createChildArray() const override;

		Result callSync(const Array<var>& args) override;

		ScopedPointer<WeakCallbackHolder> optionalCallback;
	};

	struct OtherBroadcasterTarget : public TargetBase
	{
		OtherBroadcasterTarget(ScriptBroadcaster* parent_, ScriptBroadcaster* target_, const var& transformFunction, bool async, const var& metadata);

		Result callSync(const Array<var>& args) override;
		
		Identifier getItemId() const override;

		const bool async;
		WeakReference<ScriptBroadcaster> parent, target;
		WeakCallbackHolder argTransformFunction;
	};

    struct ListenerBase: public ItemBase
    {
		ListenerBase(const var& metadata_);;

		/** Overwrite this method and return the number of calls to all listeners that should be made
			when the connection is established. */
		virtual int getNumInitialCalls() const;
		// = 0;

		/** Overwrite this method and return the argument array for the initial call when the connection 
			is established. callIndex is guaranteed to be 0 < callIndex < getNumInitialCalls(). */
		virtual Array<var> getInitialArgs(int callIndex) const;;// = 0;

        virtual ~ListenerBase();;
     
		virtual Result callItem(TargetBase* n) = 0;

		JUCE_DECLARE_WEAK_REFERENCEABLE(ListenerBase);
    };
    
	struct DebugableObjectListener : public ListenerBase
	{
		DebugableObjectListener(ScriptBroadcaster* parent_, const var& metadata, DebugableObjectBase* obj_, const Identifier& callbackId_);;

		Identifier getItemId() const override;

		Array<var> createChildArray() const override;

		int getNumInitialCalls() const override;
		Array<var> getInitialArgs(int callIndex) const override;

		void registerSpecialBodyItems(ComponentWithPreferredSize::BodyFactory& factory) override;

		WeakReference<DebugableObjectBase> obj;
		Identifier callbackId;

		/** Just a dummy for debugging. */
		virtual Result callItem(TargetBase* n) override;

		WeakReference<ScriptBroadcaster> parent;
	};


	struct ModuleParameterListener: public ListenerBase
	{
		struct ProcessorListener;

		ModuleParameterListener(ScriptBroadcaster* b, const Array<WeakReference<Processor>>& processors, const Array<uint16>& parameterIndexes, const var& metadata, const Identifier& specialId, bool useIntegerParameters);

		Identifier getItemId() const override;

		void registerSpecialBodyItems(ComponentWithPreferredSize::BodyFactory& factory) override;

		int getNumInitialCalls() const override;
		Array<var> getInitialArgs(int callIndex) const override;

		Array<var> createChildArray() const override;

		Result callItem(TargetBase* n) override;

		OwnedArray<ProcessorListener> listeners;
	};

	struct EqListener : public ListenerBase
	{
		struct InternalListener;
		Identifier getItemId() const override;

		EqListener(ScriptBroadcaster* b, const Array<WeakReference<CurveEq>>& eqs, const StringArray& eventList, const var& metadata);

		int getNumInitialCalls() const override;
		Array<var> getInitialArgs(int callIndex) const override;
		Array<var> createChildArray() const override;

		Result callItem(TargetBase* b) override;

		OwnedArray<InternalListener> listeners;
	};

	struct RoutingMatrixListener : public ListenerBase
	{
		struct MatrixListener;

		RoutingMatrixListener(ScriptBroadcaster* b, const Array<WeakReference<Processor>>& processors, const var& metadata);

		Identifier getItemId() const override;;

		void registerSpecialBodyItems(ComponentWithPreferredSize::BodyFactory& factory) override;

		int getNumInitialCalls() const override;
		Array<var> getInitialArgs(int callIndex) const override;

		Array<var> createChildArray() const override;

		Result callItem(TargetBase* b) override;

		OwnedArray<MatrixListener> listeners;
	};

	struct ScriptCallListener : public ListenerBase
	{
		struct ScriptCallItem;

		ScriptCallListener(ScriptBroadcaster* b, const Identifier& id, DebugableObject::Location location);

		Array<var> createChildArray() const override;

		Result callItem(TargetBase* n) override;

		// Don't need to initialise function calls
		int getNumInitialCalls() const override;
		Array<var> getInitialArgs(int callIndex) const override;

		Identifier getItemId() const override;

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

		int getNumInitialCalls() const override;
		Array<var> getInitialArgs(int callIndex) const override;

		Array<var> createChildArray() const override;
            
        OwnedArray<Item> items;

		Identifier typeId;
    };

	struct ProcessingSpecSource : public ListenerBase
	{
		ProcessingSpecSource(ScriptBroadcaster* b, const var& metadata);

		~ProcessingSpecSource();

		Identifier getItemId() const override;

		void registerSpecialBodyItems(ComponentWithPreferredSize::BodyFactory& factory) override;

		int getNumInitialCalls() const override;
		Array<var> getInitialArgs(int callIndex) const override;

		Array<var> createChildArray() const override;;

		Result callItem(TargetBase* n) override;

		static void prepareCalled(ProcessingSpecSource& p, double sampleRate, int blockSize);

		Array<var> processArgs;

		WeakReference<ScriptBroadcaster> parent;

		JUCE_DECLARE_WEAK_REFERENCEABLE(ProcessingSpecSource);
	};

	struct ComponentPropertyListener : public ListenerBase
	{
		struct InternalListener;

		ComponentPropertyListener(ScriptBroadcaster* b, var componentIds, const Array<Identifier>& propertyIds, const var& metadata);

		Identifier getItemId() const override;

		void registerSpecialBodyItems(ComponentWithPreferredSize::BodyFactory& factory) override;

		int getNumInitialCalls() const override;
		Array<var> getInitialArgs(int callIndex) const override;

		Result callItem(TargetBase* n) override;

		Array<var> createChildArray() const override;

		Array<Identifier> propertyIds;
		Identifier illegalId;
		OwnedArray<InternalListener> items;
	};

	struct ComponentVisibilityListener : public ListenerBase
	{
		struct InternalListener;

		ComponentVisibilityListener(ScriptBroadcaster* b, var componentIds, const var& metadata);

		Identifier getItemId() const override;

		void registerSpecialBodyItems(ComponentWithPreferredSize::BodyFactory& factory) override;

		
		int getNumInitialCalls() const override;
		Array<var> getInitialArgs(int callIndex) const override;

		Result callItem(TargetBase* n) override;

		Array<var> createChildArray() const override;

		OwnedArray<InternalListener> items;

	};

	struct MouseEventListener : public ListenerBase
	{
		struct InternalMouseListener;

		MouseEventListener(ScriptBroadcaster* parent, var componentIds, MouseCallbackComponent::CallbackLevel level, const var& metadata);

		Identifier getItemId() const override;

		// We don't need to call this to update it with the current value because the mouse events are non persistent. */
		Result callItem(TargetBase*) override;

		int getNumInitialCalls() const override;
		Array<var> getInitialArgs(int callIndex) const override;

		Array<var> createChildArray() const override;

		OwnedArray<InternalMouseListener> items;
		MouseCallbackComponent::CallbackLevel level;
	};

	struct ContextMenuListener;

	struct ComponentValueListener : public ListenerBase
	{
		struct InternalListener;

		ComponentValueListener(ScriptBroadcaster* parent, var componentIds, const var& metadata);

		Identifier getItemId() const override;

		void registerSpecialBodyItems(ComponentWithPreferredSize::BodyFactory& factory) override;

		int getNumInitialCalls() const override;
		Array<var> getInitialArgs(int callIndex) const override;

		Result callItem(TargetBase* n) override;

		Array<var> createChildArray() const override;

		OwnedArray<InternalListener> items;

		
	};

	struct RadioGroupListener : public ListenerBase
	{
		struct InternalListener;

		RadioGroupListener(ScriptBroadcaster* b, int radioGroup, const var& metadata);

		Identifier getItemId() const override;

		void registerSpecialBodyItems(ComponentWithPreferredSize::BodyFactory& factory) override;

		int getNumInitialCalls() const override;
		Array<var> getInitialArgs(int callIndex) const override;

		void setButtonValueFromIndex(int newIndex);

		Result callItem(TargetBase* n) override;

		Array<var> createChildArray() const override;

		int currentIndex = -1;

		const int radioGroup;
		OwnedArray<InternalListener> items;
	};
	
	struct OtherBroadcasterListener : public ListenerBase
	{
		OtherBroadcasterListener(const Array<WeakReference<ScriptBroadcaster>>& list, const var& metadata);;

		Result callItem(TargetBase* n) override;
		
		Identifier getItemId() const override;

		int getNumInitialCalls() const override;

		Array<var> getInitialArgs(int callIndex) const override;

#if USE_BACKEND
		void registerSpecialBodyItems(ComponentWithPreferredSize::BodyFactory& factory) override;
#endif

		Array<var> createChildArray() const override;

		Array<WeakReference<ScriptBroadcaster>> sources;
	};

	void initItem(TargetBase* n);

	void checkMetadataAndCallWithInitValues(ItemBase* i);

	OwnedArray<ListenerBase> attachedListeners;

	OwnedArray<TargetBase> items;

	Result lastResult;

	void throwIfAlreadyConnected();

	JUCE_DECLARE_WEAK_REFERENCEABLE(ScriptBroadcaster);
};





}


} // namespace hise
