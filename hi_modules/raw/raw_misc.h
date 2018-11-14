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

#include "JuceHeader.h"

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

/** This class offers a bidirectional connection between a parameter and its UI representation.

	If you just want to connect one UI element to a Processor's parameter, this lets
	you do it with the minimal amount of code. It automatically registers as listener
	to both the slider and the Processor and updates each other when one of them changes.

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
	template <class ComponentType> class Base : public ControlledObject,
												public ComponentType::Listener
	{
	public:

		Base(ComponentType* c, MainController* mc, const String& id, int parameterIndex);
		virtual ~Base();

	protected:

		/** Call this method from your subclass's listener callback. */
		void parameterChangedFromUI(float newValue);

		/** Overwrite this method and change the displayed value on your Component. */
		virtual void updateUI(float newValue) = 0;

		/** Returns a reference to the component. It is safe to assume that it's not deleted in the UI callbacks. */
		ComponentType& getComponent() { return *component.getComponent(); }
		
	private:

		Component::SafePointer<ComponentType> component;
		raw::Reference<Processor> processor;
		int index;
		
		JUCE_DECLARE_NON_COPYABLE(Base);
	};

	/** A connection between a Processor's parameter and a juce::Slider. */
	class Slider: private Base<juce::Slider>
	{
	public:

		Slider(juce::Slider* s, MainController* mc, 
					  const String& processorID, int parameterIndex);

	private:

		/** Changes the slider's value. */
		void updateUI(float newValue) override;

		/** Updates the Processor's parameter. */
		void sliderValueChanged(juce::Slider*) override;
	};

	/** A connection between a Processor's parameter and a juce::Button.
	
		It assumes toggling mode (as it's the case with 99% of all HISE parameters
		connected to buttons)
	*/
	class Button : private Base<juce::Button>
	{
	public:

		Button(juce::Button* b, MainController* mc,
			const String& processorID, int parameterIndex);

	private:

		/** Changes the button's toggle state. */
		void updateUI(float newValue) override;

		void buttonClicked(juce::Button*) override;
	};

	/** A connection between a Processor's parameter and a juce::ComboBox.
	
		It uses the index (starting with zero) for the value.
	*/
	class ComboBox : private Base<juce::ComboBox>
	{
	public:

		ComboBox(juce::ComboBox* b, MainController* mc,
			const String& processorID, int parameterIndex);

	private:

		/** Changes the comboboxes's value. */
		void updateUI(float newValue) override;

		void comboBoxChanged(juce::ComboBox*) override;
	};
};

/** A object that handles the embedded resources (audio files, images and samplemaps). */
class Pool : public hise::ControlledObject
{
public:

	Pool(MainController* mc, bool allowLoading=false) :
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

	StringArray getSampleMapList() const;

	PoolReference createSampleMapReference(const String& referenceString);

private:

	bool allowUnusedSources = false;
};

/** A wrapper around a plugin parameter (not yet functional). */
class Parameter : private SafeChangeListener
{
public:

	/** A connection to a internal attribute of a Processor. */
	struct Connection
	{
		WeakReference<Processor> processor;
		int index;
		NormalisableRange<float> parameterRange;
		float lastValue;
	};

	/** Creates a parameter. */
	Parameter(MainController* mc)
	{

	};

	/** Adds a parameter connection. */
	void addConnection(const Connection& c)
	{

	}

private:

	void changeListenerCallback(SafeChangeBroadcaster *b) override
	{

	}

	Array<Connection> connections;
};



struct Data : public RestorableObject
{
	using SaveFunction = std::function<var(Processor* p)>;
	using LoadFunction = std::function<void(Processor* p, const var&)>;

	struct SampleMap
	{
		static var save(Processor* p)
		{
			return var(dynamic_cast<ModulatorSampler*>(p)->getSampleMap()->getReference().getReferenceString());
		}

		static void load(Processor* p, const var& newValue)
		{
			raw::Pool pool(p->getMainController(), true);
			auto ref = pool.createSampleMapReference(newValue.toString());
			dynamic_cast<ModulatorSampler*>(p)->loadSampleMap(ref);
		}
	};

	template <int attributeIndex> struct Attribute
	{
		static var save(Processor* p)
		{
			return p->getAttribute(attributeIndex);
		}

		static void load(Processor* p, const var& newValue)
		{
			float value = (float)newValue;
			FloatSanitizers::sanitizeFloatNumber(value);
			p->setAttribute(attributeIndex, value, sendNotification);
		}
	};

	template <int tableIndex = 0> struct Table
	{
		static var save(Processor* p)
		{
			if (auto ltp = dynamic_cast<hise::LookupTableProcessor*>(p))
			{
				return var(ltp->getTable(tableIndex)->exportData());
			}

			return {};
		}

		static void load(Processor* p, const var& newValue)
		{
			if (auto ltp = dynamic_cast<hise::LookupTableProcessor*>(p))
			{
				auto table = ltp->getTable(tableIndex);
				
				table->restoreData(newValue.toString());
				table->sendChangeMessage();
			}
		}
	};

	Data(const Identifier& id_) :
		id(id_)
	{};

	ValueTree exportAsValueTree() const override
	{
		ValueTree v("Control");
		
		v.setProperty("id", id.toString(), nullptr);
		v.setProperty("value", save(), nullptr);
		return v;
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

	Identifier id;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Data);

};


template <class FunctionClass> class Storage : public Data
{
public:

	Storage(const Identifier& id, Processor* p) :
		Data(id),
		processor(p->getMainController(), p->getId()),
		saveFunction(FunctionClass::save),
		loadFunction(FunctionClass::load)
	{};

	~Storage() {};

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


}

} // namespace hise;