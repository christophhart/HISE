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

/** A wrapper around a task that is supposed to take a while and is not allowed to be executed simultaneously to the audio rendering.

Some tasks that interfere with the audio rendering (eg. adding / removing modules, swapping samples) have to make sure that the audio thread
is not running in parallel. Therefore HISE has a "suspended task" system which kills all voices, waits until the voices have gracefully faded out (including
FX tails for reverb effects), then suspends the audio rendering until the task has been completed on a background thread with a minimal amount of locking.

In HISE, this system is used for each one of these events:

- Compilation of scripts
- Hot swapping of samplemaps
- Loading User Presets
- Adding / Removing Modules

You can see this system in action if you do one of these things in HISE while playing notes.

In order to use that system from your own C++ code, just create a lambda that follows the hise::SafeFunctionCall definition and pass it to the call() function:

\code
auto f = [mySampleMap](hise::Processor* p)
{
    // You know it's a sampler, so no need for a dynamic_cast
	auto sampler = static_cast<hise::ModulatorSampler*>(p);

	// Call whatever you need to do.
	// (Actually this call makes no sense, since it's already wrapped in a suspended execution)
	sampler->loadSampleMap(mySampleMap);

	// The lambda definition requires you to return this.
	return hise::SafeFunctionCall::OK;
};

TaskAfterSuspension::call(mc, f);
\endcode

This makes sure that the lambda will be executed on the given thread after all voices are killed. However this doesn't necessarily mean asynchronous execution:
if you're calling this on the target thread and all voices are already killed, it will be executed synchronously. You can check this with the ReturnType enum passed back from the call() method. This reduces the debugging headaches of asynchronous callbacks wherever possible.
*/
class TaskAfterSuspension
{
public:

	/** The call() method will return one of these to indicate what happened. */
	enum ReturnType
	{
		ExecutedSynchronously, ///< If the audio was already suspended and the method was called from the same thread as the desired target thread, this will be returned from the call.
		DeferredToThread, ///< If the audio needs to be suspended and / or the method was called from another thread than the target thread, this will be returned in order to indicate that the function was not yet executed.
		numReturnTypes
	};

	/** Executes the function passed as lambda and returns the ReturnType depending on the
		execution state of the function. If you want to specify the processor being used, use the other call() method. */
	static ReturnType call(MainController* mc, const ProcessorFunction& f, int TargetThread = IDs::Threads::Loading)
	{
		bool synchronouslyCalled = mc->getKillStateHandler().killVoicesAndCall(mc->getMainSynthChain(), f, (MainController::KillStateHandler::TargetThread)TargetThread);

		return synchronouslyCalled ? ExecutedSynchronously : DeferredToThread;
	}

	/** Executes the function passed as lambda for the given processor and returns the ReturnType depending on the
	execution state of the function. */
	static ReturnType call(Processor* p, const ProcessorFunction& f, int TargetThread = IDs::Threads::Loading)
	{
		bool synchronouslyCalled = p->getMainController()->getKillStateHandler().killVoicesAndCall(p, f, (MainController::KillStateHandler::TargetThread)TargetThread);

		return synchronouslyCalled ? ExecutedSynchronously : DeferredToThread;
	}
};



/** A lightweight wrapper around a Processor in the HISE signal path. 

	This object can be used as a wrapper around one of the Processor modules in your HISE
	project. Upon creation, it will search for the given ID (or use the first module that matches the type if the ID is not supplied).

	If desired, it will also be notified about changes to a parameter which can be used to update the UI.

	\code
	Reference<hise::PolyFilterEffect> polyFilter(mc, "MyFunkyFilter", true);

	ParameterCallback update = [this](float newValue) 
	{
		this->frequencySlider->setValue(newValue, dontSendNotification); 
	};

	polyFilter.addParameterToWatch(hise::MonoFilterEffect::Frequency, update);
	\endcode
	
	If you want a bidirectional connection between UI and the processor, take a look at 
	the UIConnection classes, which offer ready made listeners / updaters for some of the
	basic JUCE widgets (Slider, ComboBox, Button)

	*/
template <class ProcessorType> class Reference : public ControlledObject,
												 private SafeChangeListener
{
public:

	/** A lambda that will be executed when the parameter changes. The newValue argument will contain the current value. */
	using ParameterCallback = std::function<void(float newValue)>;

	/** Creates a reference to the processor with the given ID. If the ID is empty, it just looks for the first processor of the specified type. */
	Reference(MainController* mc, const String& id = String(), bool addAsListener=false);
	~Reference();

	/** Adds a lambda callback to a dedicated parameter that will be fired on changes. */
	void addParameterToWatch(int parameterIndex, const ParameterCallback& callbackFunction);

	/** Returns a raw pointer to the connected Processor. */
	ProcessorType* getProcessor() const noexcept;

	ProcessorType* operator->() noexcept { return getProcessor(); }
	const ProcessorType* operator->() const noexcept { return getProcessor(); }

	operator ProcessorType*() const noexcept { return getProcessor(); }

private:

	void changeListenerCallback(SafeChangeBroadcaster *) override;

	struct ParameterValue
	{
		int index;
		float lastValue;
		ParameterCallback callbackFunction;
	};

	Array<ParameterValue> watchedParameters;

	WeakReference<ProcessorType> processor;
};



/** This class offers a drop-in replacement to the juce::AsyncUpdater class with the following modifications:

	- it doesn't use the OS message queue for posting update messages but a global timer instead.
	  This fixes the issues that occur when many objects are used that might clog the OS message loop.
	- real-time safe. Calling triggerAsyncUpdate() normally allocates a little bit of memory and should not
	  be used from the audio thread. This model uses a lockfree queue that offers allocation free and thread-safe
	  notifications to the UI thread.
	- the updates will not be executed when there is no plugin-editor visible. This improves the UI performance
	  when many instances of the plugin are used. This is trivial if the class that subclasses it is a Component
	  (in this case, the object most likely won't exist), however if your using this class somewhere in your data
	  model, it might have a drastic performance increase.

	You can use it just like the normal juce::AsyncUpdater class: call triggerAsyncUpdate() and override
	handleAsyncUpdate().
*/
struct SuspendableAsyncUpdater : public  hise::ControlledObject,
	private hise::SafeChangeListener,
	private hise::SafeChangeBroadcaster
{
	/** Creates an instance of this. You need to supply the main controller for the global UI timer. */
	SuspendableAsyncUpdater(MainController* mc) :
		ControlledObject(mc)
	{
		enablePooledUpdate(getMainController()->getGlobalUIUpdater());
	}

	virtual ~SuspendableAsyncUpdater()
	{
		removeChangeListener(this);
	}

	/** Override this message. */
	virtual void handleAsyncUpdate() = 0;

	/** Call this to trigger a update that will be executed sometime in the future on the message thread.

		This is completely lockfree and threadsafe and can be called from any thread, including the audio thread.
	*/
	void triggerAsyncUpdate()
	{
		sendPooledChangeMessage();
	}

private:

	void changeListenerCallback(SafeChangeBroadcaster *) override
	{
		handleAsyncUpdate();
	}
};


/** A object that handles the embedded resources (audio files, images and samplemaps). */
class Pool : public hise::ControlledObject
{
public:

	static String getProjectFolderWildcard()
    {
        static const String projectFolderWildcard = "{PROJECT_FOLDER}";
        return projectFolderWildcard;
    }

	Pool(MainController* mc, bool allowLoading = false) :
		ControlledObject(mc),
		allowUnusedSources(allowLoading)
	{};

	/** By default, this object assumes that the data is already loaded (if not, it fails with an assertion).
	Call this to allow the Pool object to actually load the items from the embedded data. */
	void allowLoadingOfUnusedResources();

	/** Loads an audio file into a AudioSampleBuffer. */
	AudioSampleBuffer loadAudioFile(const String& id);

	/** Loads an Image from the given ID. */
	Image loadImage(const String& id);

	PoolReference createSampleMapReference(const String& referenceString);

	PoolReference createMidiFileReference(const String& referenceString);

	/** Returns a list of the references to every embedded resource of the given type. */
	StringArray getListOfEmbeddedResources(FileHandlerBase::SubDirectories directory, bool useExpansionPool=false);

	
private:

	Array<PoolReference> getListOfReferences(FileHandlerBase::SubDirectories directory, FileHandlerBase* handler);

	bool allowUnusedSources = false;
};



/** The base class for all data connections used in the raw namespace.

	This is a powerful and generic class which is the glue between the data of a Processor
	and your raw project. 

	- templated data type for best performance without overhead
	- ready made accessor-classes for the most important data in HISE (Processor attributes, bypass states, samplemaps, table data)
	
	In order to use it, pass one of the subclasses as template argument to one of the more high-level classes (UIConnection and Storage)
	and they will synchronize with the actual data in your Processor tree.

	You can also write your own handler classes - just make sure it has two functions that match the SaveFunction and LoadFunction prototype
*/
template <typename DataType> struct Data
{
	using SaveFunction = std::function<DataType(Processor* p)>;
	using LoadFunction = std::function<void(Processor* p, const DataType&)>;

	struct Dummy
	{
		static DataType save(Processor* p)
		{
			ignoreUnused(p);
			jassertfalse;
			return {};
		}

		static void load(Processor* p, const DataType& newValue)
		{
			ignoreUnused(p, newValue);
			jassertfalse;
		}
	};

	/** Loads / saves a samplemap. Expects a String with the SampleMap ID. */
	struct SampleMap
	{
		static DataType save(Processor* p)
		{
			return DataType(dynamic_cast<ModulatorSampler*>(p)->getSampleMap()->getReference().getReferenceString());
		}

		static void load(Processor* p, const DataType& newValue)
		{
			auto f = [newValue](Processor* p)
			{
				raw::Pool pool(p->getMainController(), true);
				auto ref = pool.createSampleMapReference(newValue);
				dynamic_cast<ModulatorSampler*>(p)->loadSampleMap(ref);
				return SafeFunctionCall::OK;
			};

			TaskAfterSuspension::call(p, f);
		}
	};

	struct Intensity
	{
		static DataType save(Processor* p)
		{
			jassert(dynamic_cast<Modulation*>(p) != nullptr);

			auto v = dynamic_cast<Modulation*>(p)->getIntensity();
			return static_cast<DataType>(v);
		}

		static void load(Processor* p, DataType newValue)
		{
			jassert(dynamic_cast<Modulation*>(p) != nullptr);
			
			auto v = static_cast<float>(newValue);
			dynamic_cast<Modulation*>(p)->setIntensity(v);
		}
	};

	/** Loads / saves the bypass state. Use the template argument to invert the logic (so that true = enabled). */
	template <bool inverted = false> struct Bypassed
	{
		static DataType save(Processor* p)
		{
			if (inverted)
				return static_cast<DataType>(!p->isBypassed());
			else
				return static_cast<DataType>(p->isBypassed());
		}

		static void load(Processor* p, const DataType& newValue)
		{
			if (inverted)
				p->setBypassed(!static_cast<bool>(newValue), sendNotification);
			else
				p->setBypassed(static_cast<bool>(newValue), sendNotification);
		}
	};

	/** Loads / saves a Processor's attribute. */
	template <int attributeIndex> struct Attribute
	{
		static DataType save(Processor* p)
		{
			return static_cast<DataType>(p->getAttribute(attributeIndex));
		}

		static void load(Processor* p, const DataType& newValue)
		{
			float value = static_cast<float>(newValue);
			FloatSanitizers::sanitizeFloatNumber(value);
			p->setAttribute(attributeIndex, value, sendNotification);
		}
	};

	/** Loads / saves a Processor's attribute using 1 - value. */
	template <int attributeIndex> struct InvertedBoolAttribute
	{
		static DataType save(Processor* p)
		{
			static_assert(std::is_same<DataType, bool>(), "only allowed with bool data type");
			float value = p->getAttribute(attributeIndex);
			return value > 0.5f ? false : true;
		}

		static void load(Processor* p, const DataType& newValue)
		{
			static_assert(std::is_same<DataType, bool>(), "only allowed with bool data type");
			float value = newValue ? 0.0f : 1.0f;
			p->setAttribute(attributeIndex, value, sendNotification);
		}
	};

	/** Loads / saves the table data as encoded String. */
	template <int tableIndex = 0> struct Table
	{
		static DataType save(Processor* p)
		{
			if (auto ltp = dynamic_cast<hise::LookupTableProcessor*>(p))
			{
				return DataType(ltp->getTable(tableIndex)->exportData());
			}

			return {};
		}

		static void load(Processor* p, const DataType& newValue)
		{
			if (auto ltp = dynamic_cast<hise::LookupTableProcessor*>(p))
			{
				auto table = ltp->getTable(tableIndex);

				table->restoreData(newValue.toString());
			}
		}
	};

	/** Loads / saves the slider pack data as encoded String. */
	template <int sliderPackIndex = 0> struct SliderPack
	{
		static DataType save(Processor* p)
		{
			if (auto sp = dynamic_cast<hise::SliderPackProcessor*>(p))
			{
				return DataType(sp->getSliderPack(sliderPackIndex)->toBase64());
			}

			return {};
		}

		static void load(Processor* p, const DataType& newValue)
		{
			if (auto sp = dynamic_cast<hise::SliderPackProcessor*>(p))
			{
				if (auto spData = sp->getSliderPack(sliderPackIndex))
				{
					spData->fromBase64(String(newValue));
				}
				else
					jassertfalse; // no sliderpack at the given index.
			}
			else
				jassertfalse; // no slider pack processor.
		}
	};

	Data(const Identifier& id_) :
		id(id_)
	{};

	
protected:

	Identifier id;

private:

	JUCE_DECLARE_WEAK_REFERENCEABLE(Data);
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Data);

};




/** This class offers a bidirectional connection between Data and its UI representation.

	In order to use it, just add one of these as Member variable in your interface class:

\code
class MyInterface
{
	MyInterface(hise::MainController* mc):
	    s("My Funky Slider"),
	    connection(&s, mc, "Sine 1", hise::SineSynth::SaturationAmount)
	{
		addAndMakeVisible(s);
		s.setRange(0.0, 1.0, 0.01);
	}

	juce::Slider s;
	raw::UIConnection::Slider connection;
}
\endcode

	If you just want a more generic, but unidirectional connection (Processor->UI) take a look at the Reference class.
*/
class UIConnection
{
public:

	/** A small helper class that manages the ownership of an UI connection object.
	
		You can use it as member variable and assign an object to it in your editor's constructor.
	*/
	struct Ptr
	{

		/** assigns a new element to this holder. It will be automatically deleted when this goes out of scope.
		*
		*	It uses a pointer of the type hise::ControlledObject as least common denominator to prevent typing 
		*	template arguments more than once.
		*/
		ControlledObject* operator=(ControlledObject* objectToOwn)
		{
			ownedObject = objectToOwn;

			return objectToOwn;
		}

		/** Make sure you call this before deleting the connected UI element. */
		void release()
		{
			ownedObject = nullptr;
		}

	private:

		ScopedPointer<ControlledObject> ownedObject;
	};

	/** The base class for the connection implementation of each UI widget. 
	
		There are three available UI widgets: the ComboBox, the Slider and the Button.

		If you need more UI widgets or hook them up to your own classes, make sure that:

		1. The class is derived from juce::Component
		2. It has a subclass called Listener and the methods addListener() and removeListener()
		3. the Listener subclass has a virtual callback that gets overwritten to change the
		   Processor's parameter (take a look at the implementations to get a feeling how this works).
		4. Overwrite the method updateUI() to reflect the change on your UI widget. This is 
		   important to match the displayed value when the parameter is changed externally,
		   eg. by loading presets or automation.
	*/
	template <class ComponentType, typename ValueType> 
	class Base : public Data<ValueType>,
				 public ControlledObject,
				 public ComponentType::Listener,
				 public SafeChangeListener
	{
	public:

		/** This class can be used to update a processor parameter with undo support.

	If you want a UI connection to be undoable, just set UIConnection::Base::setUndoManager() and
	it will use this class for all changes.
*/
		class UndoableUIAction : public juce::UndoableAction
		{
		public:

			UndoableUIAction(Base& b, const ValueType& newValue_) :
				processor(b.getProcessor()),
				newValue(newValue_),
				f(b.loadFunction)
			{
				prevValue = b.saveFunction(processor);
			}

			bool perform() override
			{
				if (processor.get() == nullptr)
					return false;

				f(processor, newValue);
                return true;
			}

			bool undo() override
			{
				if (processor.get() == nullptr)
					return false;

				f(processor, prevValue);
                return true;
			}

		private:

			ValueType prevValue;
			ValueType newValue;
			typename Data<ValueType>::LoadFunction f;
			WeakReference<Processor> processor;
		};

		Base(ComponentType* c, MainController* mc, const String& id);
		virtual ~Base();

		/** Call this function to specify which data you want. */
		template <class FunctionClass> void setData()
		{
			loadFunction = FunctionClass::load;
			saveFunction = FunctionClass::save;

			if (saveFunction)
			{
				ValueType initialValue = saveFunction(getProcessor());
				updateUI(initialValue);
			}
		}

		/** If this is enabled, it will wrap the execution of the callback into a SafeFunction call. */
		void setUseLoadingThread(bool shouldUseLoadingThread)
		{
			useLoadingThread = shouldUseLoadingThread;
		}

		/** Returns the processor that this connection is operating on. */
		Processor* getProcessor() { return processor.get(); }

		/** Returns the processor that this connection is operating on. */
		const Processor* getProcessor() const { return processor.get(); }

		/** Enables the use of an undomanager for all UI interactions. Pass in nullptr to deactivate. */
		void setUseUndoManager(UndoManager* undoManagerToUse)
		{
			undoManager = undoManagerToUse;
		}

	protected:

		/** Call this method from your subclass's listener callback. */
		void parameterChangedFromUI(ValueType newValue);

		/** Overwrite this method and change the displayed value on your Component. */
		virtual void updateUI(ValueType newValue) = 0;

		/** Returns a reference to the component. It is safe to assume that it's not deleted in the UI callbacks. */
		ComponentType& getComponent() { return *component.getComponent(); }

		void changeListenerCallback(SafeChangeBroadcaster* b) override;

	private:

		UndoManager* undoManager = nullptr;

		bool useLoadingThread = false;

		ValueType lastValue = ValueType(0);

        typename Data<ValueType>::LoadFunction loadFunction;
        typename Data<ValueType>::SaveFunction saveFunction;

		Component::SafePointer<ComponentType> component;
		WeakReference<Processor> processor;
		
		JUCE_DECLARE_NON_COPYABLE(Base);
	};

	/** A connection between a processor and a juce::Slider. 
	
		If you want to connect a processor's parameter, best use the subclass Slider<ParameterIndex>. 
		Otherwise you can use this class directly and set the behaviour with a suitable datatype:

		\code
		// Create a slider base connection
		UIConnection::SliderBase b(mySlider, mc, "MyProcessor");

		// Set it to control the intensity ("MyProcessor" must be modulator then).
		b.setData<raw::Data<float>::Intensity>();
		\endcode
	*/
	class SliderBase : public Base<juce::Slider, float>
	{
	public:

		SliderBase(juce::Slider* s, MainController* mc, const String& processorID):
			Base(s, mc, processorID)
		{}

	private:

		/** Changes the slider's value. */
		void updateUI(float newValue) override {
			getComponent().setValue(newValue, dontSendNotification);
		}

		/** Updates the Processor's parameter. */
		void sliderValueChanged(juce::Slider*) override
		{
			parameterChangedFromUI((float)getComponent().getValue());
		}
	};

	/** A connection between a Processor's parameter and a juce::Slider. */
	template <int parameterIndex> class Slider: public SliderBase
	{
	public:

		Slider(juce::Slider* s, MainController* mc, const String& processorID);
	};

	/** A connection between a Processor's parameter and a juce::Button.
	
		It assumes toggling mode (as it's the case with 99% of all HISE parameters
		connected to buttons)
	*/
	class Button : public Base<juce::Button, bool>
	{
	public:

		Button(juce::Button* b, MainController* mc,
			const String& processorID) :
			Base(b, mc, processorID)
		{
			changeListenerCallback(nullptr);
		}

	private:

		/** Changes the button's toggle state. */
		void updateUI(bool newValue) override
		{
			getComponent().setToggleState(newValue, dontSendNotification);
		}

		void buttonClicked(juce::Button*) override
		{
			parameterChangedFromUI(getComponent().getToggleState());
		}
	};

	/** A connection between a Processor's data (anything) and a juce::ComboBox.
	*/
	class ComboBox : public Base<juce::ComboBox, var>
	{
	public:

		ComboBox(juce::ComboBox* b, MainController* mc,
			const String& processorID):
			Base(b, mc, processorID)
		{
			changeListenerCallback(nullptr);
		}

		enum Mode
		{
			Index = 0,
			Id,
			Text,
			Unspecified,
			numModes
		};

		void setMode(Mode m)
		{
			mode = m;
		}

	private:

		Mode mode = Mode::Unspecified;

		/** Changes the comboboxes's value. */
		void updateUI(var newValue) override
		{
			// Make sure you call setMode() with the expected data type
			jassert(mode != Mode::Unspecified);

			switch (mode)
			{
			case Index: getComponent().setSelectedItemIndex((int)newValue, dontSendNotification); break;
			case Id:	getComponent().setSelectedId((int)newValue, dontSendNotification); break;
			case Text:  getComponent().setText(newValue.toString(), dontSendNotification); break;
			default:	jassertfalse; break;
			}
		}

		void comboBoxChanged(juce::ComboBox*) override
		{
			// Make sure you call setMode() with the expected data type
			jassert(mode != Mode::Unspecified);

			var valueToUse;

			switch (mode)
			{
			case Index: valueToUse = getComponent().getSelectedItemIndex(); break;
			case Id:	valueToUse = getComponent().getSelectedId(); break;
			case Text:	valueToUse = getComponent().getText(); break;
			default:	jassertfalse; break;
			}

			parameterChangedFromUI(valueToUse);
		}
	};
};



/** The base class for storing data as user preset. */
class GenericStorage : public Data<juce::var>,
	public RestorableObject
{
public:

	GenericStorage(const Identifier& id) :
		Data<juce::var>(id)
	{};

	void storeAsProperty(ValueTree& v) const
	{
		v.setProperty(id, var(save()), nullptr);
	}

	ValueTree exportAsValueTree() const override
	{
		ValueTree v("Control");

		v.setProperty("id", id.toString(), nullptr);
		v.setProperty("value", var(save()), nullptr);
		return v;
	}

	void loadFromProperty(const ValueTree& v)
	{
		load(v.getProperty(id));
	}

	void restoreFromValueTree(const ValueTree &previouslyExportedState) override
	{
		jassert(previouslyExportedState.getType() == Identifier("Control"));

		if (previouslyExportedState.getProperty("id") == id.toString())
		{
			load(previouslyExportedState.getProperty("value"));
		}
	}

	virtual var save() const = 0;
	virtual void load(const var& newValue) = 0;

private:
};

template <typename DataType> class LambdaStorage: public GenericStorage
{
public:

	using SaveFunction = std::function<DataType()>;
	using LoadFunction = std::function<void(DataType)>;

	LambdaStorage(const Identifier& id, Processor* p_):
		GenericStorage(id),
		p(p_)
	{}

	var save() const override
	{
		if (saveFunction)
		{
			return var(saveFunction());
		}

		return {};
	}

	void load(const var& newValue) override
	{
		if (loadFunction)
		{
			loadFunction((DataType)newValue);
		}
	}

	WeakReference<Processor> p;

	LoadFunction loadFunction;
	SaveFunction saveFunction;
};

/** An object that automatically handles the loading / saving in a ValueTree. 

Use this to implement your custom user preset system. 
*/
template <class FunctionClass> class Storage : public GenericStorage
{
public:

	Storage(const Identifier& id, Processor* p) :
		GenericStorage(id),
		processor(p->getMainController(), p->getId()),
		saveFunction(FunctionClass::save),
		loadFunction(FunctionClass::load)
	{};

	virtual ~Storage() {};

	void load(const var& newValue) override
	{
		loadFunction(processor, newValue);
	}

	var save() const override
	{
		return saveFunction(processor);
	}

private:

	Reference<Processor> processor;

	SaveFunction saveFunction;
	LoadFunction loadFunction;
};


/** A Helper class that can be used to change a selection of FloatingTile properties.

	This saves you some time because you don't need to setup a DynamicObject yourself.
	Just call the set() method, pass a FloatingTile instance and it will set the properties.

	```cpp
	hise::FloatingTile t;
	t.setNewContent("Keyboard");

	raw::FloatingTileProperties::set(t,
	{
		{ MidiKeyboardPanel::LowKey, 15 },
		{ MidiKeyboardPanel::CustomGraphics, true }
	});
	```

	As you can see, you can use a std::initializer_list for a lean syntax. You have to use
	the property indexes (look them up in the API) for the ID.

	Be aware that this will reset all unspecified properties to the default value.
*/
class FloatingTileProperties
{
	struct Property
	{
		Property(int id_, const var& value_);;

		int id;
		var value;
	};

public:

	/** Sets the given property set to the given FloatingTile reference.

		It assumes that the type is already setup correctly.
	*/
	static void set(FloatingTile& floatingTile, const std::initializer_list<Property>& list);
};


}

} // namespace hise;
