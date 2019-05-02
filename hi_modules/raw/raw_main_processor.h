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

#pragma once

namespace hise {
using namespace juce;

namespace raw
{

/** This is the base class for the processor you're going to use as main data object.

	If you are working in HISE without using this C++ API, you will use a ScriptProcessor in the root Container
	as main interface processor (this is done via the Scripting API call Content.makeFrontInterface()).

	This module will then be responsible for the following things:

	1. Defining the user interface
	2. Managing the data that is used for restoring the plugin state (or user presets).
	3. Contains the logic for your UI to react on MIDI events.

	You can do all these things on the C++ side by creating a derived class from this class and use the following
	tools:

	### Create the UI

	The UI is a separate class that needs to be used as main editor - you just need to create an instance of your
	Component class and return it from RawDataBase::createEditor(). You probably want to derive your main editor
	class from the ConnectedObject subclass which offers a convenient way to access this module once it's added to
	the signal path.

	### Data Logic

	The recommended way for storing data that can be represented as float numbers is by adding parameters to this
	processor using the addParameter() function. Other processors can then register themselves with a lambda to be
	notified about changes using the registerCallback() function.
	This way you can benefit from all the other helper classes in the raw namespace: UIConnection, Reference, etc.

	### MIDI Processing

	This class is derived from MidiProcessor so you can react on MIDI events. Other than the scripted version, which
	allows either a synchronous or a deferred execution of the MIDI callback, this class offers both options at once,
	so you can do synchronous tasks in the audio thread as well as handling UI tasks in a asynchronous callback that
	will be executed on the message thread.

	## Initialisation

	The recommended way of using this class is to create a subclassed instance of it and pass it to the helper
	function addAndSetup(), which takes care of the entire initialisation for you:

	\code{.cpp}
	class MyData : public hise::FrontendProcessor::RawDataBase,
				   hise::raw::MainProcessor::ConnectedObject
	{
	public:
		MyData(MainController* mc) :
			RawDataBase(mc)
		{
			// This should be the first thing you do, everything else
			// will probably need the main processor.
			raw::MainProcessor::addAndSetup(this, new MyDataProcessor(getMainController()));

			// Do more initialisation here...
		};
	};
	\endcode
*/
class MainProcessor : public MidiProcessor
{
public:

	/** The function prototype for the lambda callbacks that are executed whenever a parameters changes. */
	using Callback = std::function<void(float)>;

	/** The execution type for the given lambda. */
	enum ExecutionType
	{
		Synchronously, ///< will be executed synchronously on the audio thread.
		Asynchronously, ///< will be deferred and executed on the UI thread sometime in the future.
		numExecutionTypes
	};

	/** A object that holds a reference to the MainProcessor.

		This class gives you a very convenient way to access the MainProcessor in your
		project: just subclass it and call setup, it will search the signal path for
		a raw::MainProcessor which you can then access with getMainInterface() from then on.

		This class is a close relative to the ControlledObject class, which offers the exact
		same functionality, but with a connection to the MainController instead.

		It can be templated so that you can pass in your derived class and don't need to cast
		it everytime you want to use it.
	*/
	template <class ProcessorClass=MainProcessor> struct ConnectedObject
	{
		ConnectedObject() {};

		virtual ~ConnectedObject() {};

		/** Call this with a pointer to your main controller and it will search for a
			MainProcessor object in the signal path.

			You obviously have to create the MainProcessor before calling this method.
		*/
		void setup(MainController* mc)
		{
			mainProcessor = ProcessorHelpers::getFirstProcessorWithType<ProcessorClass>(mc->getMainSynthChain());

			// Oops, if this is still nullptr, something went wrong.
			jassert(mainProcessor != nullptr);
		}

		/** Returns a pointer to the main processor. */
		ProcessorClass* getMainProcessor()
		{
			// if you hit this assertion, you have either:
			// - not called setup() yet
			// - deleted the MainProcessor
			// - didn't add the MainProcessor in the first place
			jassert(mainProcessor != nullptr);

			return mainProcessor.get();
		}

		/** Returns a read-only pointer to the main processor. */
		const ProcessorClass* getMainProcessor() const
		{
			// if you hit this assertion, you have either:
			// - not called setup() yet
			// - deleted the MainProcessor
			// - didn't add the MainProcessor in the first place
			return mainProcessor.get();
		}

	private:

		WeakReference<ProcessorClass> mainProcessor;

		JUCE_DECLARE_WEAK_REFERENCEABLE(ConnectedObject);
		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ConnectedObject);
	};

	/** This interface class can be used to implement UI logic that reacts on MIDI events.

		It automatically registeres itself on creation and destruction, so in order to use
		it, just override the hiseEventCallback() method.
	*/
	struct AsyncHiseEventListener : public ConnectedObject<>
	{
		/** The constructor will setup the connection to the MainProcessor and register
			itself as listener. */
		AsyncHiseEventListener(MainController* mc);

		/** The destructor deregisters itself from the MainProcessor. */
		virtual ~AsyncHiseEventListener();;

		/** Override this method and implement the logic that you need to react on MIDI events.

			This method is guaranteed to be executed on the message thread, however it won't be
			called if your plugin editor is not visible, so make sure you don't do vital data processing
			in here.
		*/
		virtual void hiseEventCallback(HiseEvent e) = 0;

	private:

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AsyncHiseEventListener);
		JUCE_DECLARE_WEAK_REFERENCEABLE(AsyncHiseEventListener);
	};

	SET_PROCESSOR_NAME("Interface", "Interface", "the main interface");

	/** The destructor of the main processor.
	
		Be aware that this might be called while other modules are still alive - the order
		of destruction of HISE modules in the module tree is top down, and chances are that this is one
		of the first modules.
		
		So if you have modules that use a reference to this class (eg. everything derived from ConnectedObject,
		make sure you check if the object still exists for operations that might be called after the destruction.
	*/
	virtual ~MainProcessor();

	/** This method will be called after the processor was correctly initialised.

		You can use this to build up the module tree.
	*/
	virtual void init() {}

	/** Registers a lambda to the given parameter.

		Use this class to add parameters to your main interface that other classes can listen to.

		Since it uses the standard parameter system of a hise::Processor, this is limited to float numbers.
		Boolean and discrete values can obviously also be handled, however if you have more complex data
		types, you need to add them as Data subclass
	*/
	void registerCallback(Processor* p, int parameterIndex, const Callback& f, ExecutionType executionType = Synchronously);

	Identifier getIdentifierForParameterIndex(int parameterIndex) const override;

	void setInternalAttribute(int parameterIndex, float newValue) override;

	float getAttribute(int parameterIndex) const override;

	template <class InterfaceClass> static void addAndSetup(ConnectedObject<InterfaceClass>* object, InterfaceClass* newObject);

	void processHiseEvent(HiseEvent &e) final override;

	ValueTree exportAsValueTree() const override
	{
		ValueTree v("Preset");

		for (auto p : parameters)
			p->storeAsProperty(v);

		return v;
	}

	void restoreFromValueTree(const ValueTree &v) override
	{
		for (auto p : parameters)
			p->loadFromProperty(v);
	}

protected:

	MainProcessor(MainController* mc);

	/** Adds a parameter to the MainProcessor.

		You should call this during construction of your processor and it will create a parameter
		object and add it to the list. Since everything from then on everything will be using this
		index to access the parameter, you are obliged to supply a integer value that will be matched
		against the index to make sure you don't mix things up.

		The best practice is to use a enum on your end:

		
		\code{.cpp}
		class MyClass
		{
			enum Parameters
			{
				MyFirstParameter,
				AnotherParameter,
				numParameters
			};

			MyClass()
			{
				addParameter(MyFirstParameter, "MyFirstParameter");
				addParameter(AnotherParameter, "AnotherParameter");
			}
		};
		\endcode
		
		*/
	template <int Index> void addParameter(const Identifier& id);

	/** If you want to do synchronous event processing, override this method and implement the logic there.
		Be aware that this will be called on the audio thread, so if you are planning to do non-time critical
		stuff, consider sub-classing AsyncHiseEventListener and defer the task to the UI thread instead. */
	virtual void processSync(HiseEvent& /*e*/) {};

private:

	void addAsyncHiseEventListener(AsyncHiseEventListener* l);

	void removeAsyncHiseEventListener(AsyncHiseEventListener* l);

	struct ParameterBase : private SuspendableAsyncUpdater,
						   public LambdaStorage<float>
	{
		ParameterBase(MainProcessor* mc, const Identifier& id_);;

		virtual ~ParameterBase();;

		void update(float newValue);

		Identifier getId() const { return id; }

		void registerCallback(Processor* p, const Callback& f, ExecutionType type);

		/** Returns the current value. */
		float getCurrentValue() const { return currentValue; }

	private:

		struct CallbackWithProcessor;
		
		void handleAsyncUpdate() override;

		const Identifier id;

		float currentValue;

		OwnedArray<CallbackWithProcessor> synchronousCallbacks;
		OwnedArray<CallbackWithProcessor> asynchronousCallbacks;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ParameterBase);
	};

	template <int Index> struct Parameter : public ParameterBase
	{
		Parameter(MainProcessor* parent, const Identifier& id_) :
			ParameterBase(parent, id_)
		{}

		~Parameter() {}
	};

	Array<WeakReference<AsyncHiseEventListener>> eventListeners;

	struct AsyncMessageHandler : public SafeChangeBroadcaster,
		public SafeChangeListener
	{
		AsyncMessageHandler(MainProcessor& parent_);

		~AsyncMessageHandler();

		void pushEvent(HiseEvent& e);

		void changeListenerCallback(SafeChangeBroadcaster *) override;

		MainProcessor& parent;
		hise::LockfreeQueue<HiseEvent> events;
	} asyncHandler;

	OwnedArray<ParameterBase> parameters;

	JUCE_DECLARE_WEAK_REFERENCEABLE(MainProcessor);
};



/** This class is supposed to hold all data defined by your C++ project.

	If you're using a MainProcessor and MainEditor for your project, you won't need to use this class
	directly, but create a instance of the templated derived class DataHolder instead.
*/
class DataHolderBase : public ControlledObject,
	public RestorableObject
{
public:

	DataHolderBase(MainController* mc) :
		ControlledObject(mc)
	{};

	/** Overwrite this method and return your plugin's main editor. */
	virtual Component* createEditor() = 0;

	virtual ~DataHolderBase() {};

private:

	JUCE_DECLARE_WEAK_REFERENCEABLE(DataHolderBase);
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DataHolderBase);
};

/** This class acts as glue to link your MainProcessor and MainEditor instances into your HISE project.

	If you use the raw API to create HISE projects, you just need to create two classes:

	1. A raw::MainProcessor object that manages all the data / logic
	2. A raw::MainEditor object that defines your user interface.

	Then you need to return an instance of this class with the two classes as template arguments from
	the FrontendProcessor::createPresetRaw() method - or just use the CREATE_RAW_PROCESSOR macro that
	takes care of this for you:

	\code{.cpp}
	class MyProcessor: public raw::MainProcessor
	{
		// [...]
	};

	class MyInterface: public raw::MainEditor
	{
		// [...]
	};

	CREATE_RAW_PROCESSOR(MyProcessor, MyInterface);
	\endcode
*/
template <class MainProcessorClass, class EditorClass>
class DataHolder : public hise::raw::DataHolderBase,
	public hise::raw::MainProcessor::ConnectedObject<MainProcessorClass>
{
public:

	DataHolder(MainController* mc) :
		DataHolderBase(mc)
	{
		raw::MainProcessor::addAndSetup(this, new MainProcessorClass(getMainController()));
	}

	Component* createEditor() override
	{
		return new EditorClass(this);
	}

	ValueTree exportAsValueTree() const override
	{
        return MainProcessor::ConnectedObject<MainProcessorClass>::getMainProcessor()->exportAsValueTree();
	}

	void restoreFromValueTree(const ValueTree &previouslyExportedState) override
	{
		MainProcessor::ConnectedObject<MainProcessorClass>::getMainProcessor()->restoreFromValueTree(previouslyExportedState);
	}

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DataHolder);
};

#define CREATE_RAW_PROCESSOR(ProcessorClass, InterfaceClass) hise::raw::DataHolderBase* hise::FrontendProcessor::createPresetRaw() \
{ \
	return new raw::DataHolder<ProcessorClass, InterfaceClass>(this); \
}

}
}
