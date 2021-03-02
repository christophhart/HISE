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

namespace scriptnode {
using namespace juce;
using namespace hise;


/** A NodeProperty is a non-realtime controllable property of a node.

	It is saved as ValueTree property. The actual property ID in the ValueTree
	might contain the node ID as prefix if this node is part of a hardcoded node.
*/
struct NodeProperty
{
	NodeProperty(const Identifier& baseId_, const var& defaultValue_, bool isPublic_) :
		baseId(baseId_),
		defaultValue(defaultValue_),
		isPublic(isPublic_)
	{};

	virtual ~NodeProperty() {};

	/** Call this in the initialise() function of your node as well as in the createParameters() function (with nullptr as argument).

		This will automatically initialise the proper value tree ID at the best time.
	*/
	bool initialise(NodeBase* n);

	/** Callback when the initialisation was successful. This might happen either during the initialise() method or after all parameters
		are created. Use this callback to setup the listeners / the logic that changes the property.
	*/
	virtual void postInit(NodeBase* n) = 0;

	/** Returns the ID in the ValueTree. */
	Identifier getValueTreePropertyId() const;

	ValueTree getPropertyTree() const { return d; }

	juce::Value asJuceValue()
	{
		return d.getPropertyAsValue(PropertyIds::Value, um);
	}

private:

	UndoManager* um = nullptr;
	ValueTree d;
	Identifier valueTreePropertyid;
	Identifier baseId;
	var defaultValue;
	bool isPublic;
};

#if 0
struct ScriptFunctionManager : public hise::GlobalScriptCompileListener,
	public NodeProperty
{
	ScriptFunctionManager() :
		NodeProperty(PropertyIds::Callback, "undefined", true) {};

	~ScriptFunctionManager();

	void scriptWasCompiled(JavascriptProcessor *processor) override;
	void updateFunction(Identifier, var newValue);
	double callWithDouble(double inputValue);

	void postInit(NodeBase* n) override;

	valuetree::PropertyListener functionListener;
	WeakReference<JavascriptProcessor> jp;
	Result lastResult = Result::ok();

	var functionName;
	var function;
	var input[8];
	bool ok = false;

	MainController* mc;
};
#endif

template <class T, int Value> struct StaticProperty
{
	constexpr T getValue() const { return Value; }

};

template <class T> struct NodePropertyT : public NodeProperty
{
	NodePropertyT(const Identifier& id, T defaultValue) :
		NodeProperty(id, defaultValue, false),
		value(defaultValue)
	{};

	void postInit(NodeBase* ) override
	{
		updater.setCallback(getPropertyTree(), { PropertyIds::Value }, valuetree::AsyncMode::Synchronously,
			BIND_MEMBER_FUNCTION_2(NodePropertyT::update));
	}

	void storeValue(const T& newValue, UndoManager* um)
	{
		if(getPropertyTree().isValid())
			getPropertyTree().setPropertyExcludingListener(&updater, PropertyIds::Value, newValue, um);

		value = newValue;
	}

	void update(Identifier id, var newValue)
	{
		value = newValue;

		if (additionalCallback)
			additionalCallback(id, newValue);
	}

	void setAdditionalCallback(const valuetree::PropertyListener::PropertyCallback& c, bool callWithValue=false)
	{
		additionalCallback = c;

		if (callWithValue && additionalCallback)
			additionalCallback(PropertyIds::Value, var(value));
	}

	T getValue() const { return value; }

private:

	valuetree::PropertyListener::PropertyCallback additionalCallback;

	T value;
	valuetree::PropertyListener updater;
};


struct ComboBoxWithModeProperty : public ComboBox,
	public ComboBoxListener
{
	ComboBoxWithModeProperty(String defaultValue) :
		ComboBox(),
		mode(PropertyIds::Mode, defaultValue)
	{
		addListener(this);
		setLookAndFeel(&plaf);
		GlobalHiseLookAndFeel::setDefaultColours(*this);
	}

	void comboBoxChanged(ComboBox* comboBoxThatHasChanged)
	{
		if (initialised)
			mode.storeValue(getText(), um);
	}

	void valueTreeCallback(Identifier id, var newValue)
	{
		setText(newValue.toString(), dontSendNotification);
	}

	void initModes(const StringArray& modes, NodeBase* n)
	{
		if (initialised)
			return;

		clear(dontSendNotification);
		addItemList(modes, 1);

		um = n->getUndoManager();
		mode.initialise(n);
		mode.setAdditionalCallback(BIND_MEMBER_FUNCTION_2(ComboBoxWithModeProperty::valueTreeCallback), true);
		initialised = true;
	}

	bool initialised = false;
	UndoManager* um;
	NodePropertyT<String> mode;
	PopupLookAndFeel plaf;
};


/** This namespace contains templates to declare properties in CPP.

	In order to use it, pass one of the template instances to the cpp_node class as third parameter (default is properties::none).

	It will automatically register and create properties for the given node that can be changed like any other scriptnode property
	(using either the IDE or scripting calls)

*/
namespace properties
{

/** A macro that can by used inside a property class to setup the required members and methods. */
#define DECLARE_SNEX_NATIVE_PROPERTY(className, type, initValue) using DataType = type; \
                                                          static Identifier getId() { return Identifier(#className); } \
                                                          static DataType getDefault() { return initValue; };

#define DECLARE_SNEX_COMPLEX_PROPERTY(className) static Identifier getId() { return Identifier(#className); }

/** This templates takes a class and wraps it into a property with proper data management.

	The PropertyClass you pass in is supposed to meet a few standards:

	- use the DECLARE_SNEX_PROPERTY() macro inside the class to define the name,
	  data type and initial value
	- define a set(T& obj, DataType newValue) method

	A minimal example for a valid property class is:

		struct MyProperty
		{
			DECLARE_SNEX_PROPERTY(MyProperty, double, 4.0);

			void set(MyObject& obj, double newValue)
			{
				obj.doSomethingWith(newValue);
			}
		};

	The set() method will be called whenever the property changes. Be aware that the MyObject class
	is supposed to be the type of the main root object that was passed into the cpp_node template.
	If this is a container, you can use the standard methods to access its members:

		void set(MyChain& obj, double newValue)
		{
			obj.get<1>().setPropertyOfMember(newValue);
		}

	This allows to build up a simple property management that can change any arbitrary member node of
	the root node.
*/
template <class PropertyClass> struct native : public NodePropertyT<typename PropertyClass::DataType>
{
	native() :
    NodePropertyT<typename PropertyClass::DataType>(PropertyClass::getId(), PropertyClass::getDefault())
	{};

	/** This is being called in the initialise method of the cpp_node template class.
		the root object needs to have the same type that is used as object type for the node.
	*/
	template <class RootObject> void initWithRoot(NodeBase* n, RootObject& r)
	{
		setRootObject(r);
		p.set(r, PropertyClass::getDefault());
		this->initialise(n);
	}

private:

	/** @internal: sets up a callback that calls the set method of the given property class. */
	template <class RootObject> void setRootObject(RootObject& r)
	{
		setAdditionalCallback([&r, this](Identifier id, var newValue)
		{
			if (id == PropertyIds::Value)
			{
				auto typedValue = (typename PropertyClass::DataType)newValue;
				p.set(r, typedValue);
			}
		});
	}

	PropertyClass p;
};




/** This node property can be used to use an external file.
	
	The PropertyClass needs a method called setFile(T& obj, file::type& file)	
	and will be called whenever a new file is loaded.
*/
template <class PropertyClass, int NumChannels> struct file : public NodePropertyT<String>
{
	file() :
		NodePropertyT<String>(PropertyClass::getId(), "")
	{};

	/** A POD containing the audio data and metadata for the given channel amount. */
	struct type
	{
		snex::Types::span<snex::Types::dyn<float>, NumChannels> data;
		double sampleRate = 0.0;
	};

	/** This is being called in the initialise method of the cpp_node template class.
		the root object needs to have the same type that is used as object type for the node.
	*/
	template <class RootObject> void initWithRoot(NodeBase* n, RootObject& r)
	{
        jassertfalse;
        
#if 0
		if (n != nullptr)
		{
			mc = n->getScriptProcessor()->getMainController_();
			initialise(n);
			setRootObject(r);
		}
#endif
	}

private:

	/** @internal: sets up a callback that calls the set method of the given property class. */
	template <class RootObject> void setRootObject(RootObject& r)
	{
		setAdditionalCallback([&r, this](Identifier id, var newValue)
		{
			if (id == PropertyIds::Value)
			{
				auto f = newValue.toString();

				PoolReference ref(mc, f, FileHandlerBase::SubDirectories::AudioFiles);
				reference = mc->getCurrentAudioSampleBufferPool()->loadFromReference(ref, PoolHelpers::LoadAndCacheWeak);

				auto& b = reference->data;
				d.sampleRate = (double)reference->additionalData.getProperty("SampleRate", 0.0);

				using namespace snex;
				using namespace snex::Types;

				for (int i = 0; i < b.getNumChannels(); i++)
				{
					d.data[i] = dyn<float>(b.getWritePointer(i), b.getNumSamples());
				}

				PropertyClass p;
				p.setFile(r, d);
			}
		});
	}

	type d;
	MainController* mc;
	AudioSampleBufferPool::ManagedPtr reference;
};




/** This template is being used when you have multiple properties in a node. It simply forwards the
	calls to every child property.
*/
template <class... Properties> struct list: advanced_tuple<Properties...>
{
	tuple_iteratorT2(initWithRoot, RootObject, NodeBase*, n, RootObject&, r);

	template <class RootObject> void initWithRoot(NodeBase* n, RootObject& r)
	{
        jassertfalse;
#if 0
		call_tuple_iterator2(initWithRoot, n, r);
#endif
	}
};
}




}
