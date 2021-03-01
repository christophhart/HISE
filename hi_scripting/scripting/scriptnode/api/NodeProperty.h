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



#if HISE_INCLUDE_SNEX
using namespace snex::ui;

struct SnexSource: public WorkbenchData::Listener
{
	struct SnexSourceListener
	{
		virtual ~SnexSourceListener() {};
		virtual void wasCompiled(bool ok) = 0;
		virtual void complexDataAdded(snex::ExternalData::DataType t, int index) = 0;
		virtual void parameterChanged(int snexParameterId, double newValue) = 0;

		virtual void complexDataTypeChanged() {};

		JUCE_DECLARE_WEAK_REFERENCEABLE(SnexSourceListener);
	};

	struct SnexParameter : public NodeBase::Parameter
	{
		SnexParameter(SnexSource* n, NodeBase* parent, ValueTree dataTree);
		parameter::dynamic p;
		const int pIndex;
		ValueTree treeInNetwork;

		void sendValueChangeToParentListeners(Identifier id, var newValue);

		valuetree::PropertySyncer syncer;
		valuetree::PropertyListener parentValueUpdater;
		WeakReference<SnexSource> snexSource;
	};

	template <int BSize, int Alignment> struct ObjectStorage
	{
		static constexpr int SmallBufferSize = BSize;

		ObjectStorage()
		{
			memset(smallBuffer, 0, BSize + Alignment);
			setSize(0);
		}

		void initialise(snex::jit::ComplexType::Ptr obj)
		{
			setSize(obj->getRequiredByteSize());

			ComplexType::InitData d;
			d.callConstructor = obj->hasConstructor();
			d.dataPointer = getObjectPtr();
			d.t = ComplexType::InitData::Type::Constructor;
			d.initValues = obj->makeDefaultInitialiserList();

			obj->initialise(d);
		}

		void* getObjectPtr()
		{
			return objPtr;
		}

		void setSize(size_t newSize)
		{
			if (newSize != allocatedSize)
			{
				allocatedSize = newSize;

				if (allocatedSize >= (SmallBufferSize))
				{
					bigBuffer.allocate(newSize + Alignment, true);
					objPtr = bigBuffer.get();
				}
				else
				{
					bigBuffer.free();
					objPtr = &smallBuffer;
				}

				if (Alignment != 0)
				{
					if (auto o = reinterpret_cast<uint64_t>(objPtr) % Alignment)
						objPtr = (static_cast<uint8*>(objPtr) + (Alignment - o));
				}
			}
		}

	private:

		void* objPtr = nullptr;
		size_t allocatedSize = 0;
		uint8 smallBuffer[BSize + Alignment];
		HeapBlock<uint8> bigBuffer;
	};

	struct HandlerBase
	{
		using ObjectStorageType = ObjectStorage<OpaqueNode::SmallObjectSize, 16>;

		HandlerBase(SnexSource& s, ObjectStorageType& obj_) :
			parent(s),
			obj(obj_)
		{};

		virtual void reset() = 0;
		virtual Result recompiledOk(snex::jit::ComplexType::Ptr objectClass) = 0;
		virtual ~HandlerBase() {};

		snex::jit::FunctionData getFunctionAsObjectCallback(const String& id);

		void addObjectPtrToFunction(FunctionData& f);;

		SimpleReadWriteLock& getAccessLock() { return lock; }

	protected:

		NodeBase* getNode() { return parent.parentNode; }
		SnexSource& parent;

		ObjectStorageType& obj;

	private:

		SimpleReadWriteLock lock;
	};


	struct ParameterHandlerLight : public HandlerBase
	{
		ParameterHandlerLight(SnexSource& s, ObjectStorageType& o) :
			HandlerBase(s, o)
		{
			memset(lastValues, 0, sizeof(double)*OpaqueNode::NumMaxParameters);
		};

		virtual ~ParameterHandlerLight() {};

		void reset() override
		{
			SimpleReadWriteLock::ScopedWriteLock sl(getAccessLock());

			for (auto& f : pFunctions)
				f = {};
		}

		void copyLastValuesFrom(ParameterHandlerLight& other)
		{
			memcpy(lastValues, other.lastValues, sizeof(double) *OpaqueNode::NumMaxParameters);
		}

		Result recompiledOk(snex::jit::ComplexType::Ptr objectClass) override;

		template <int P> static void setParameterStatic(void* obj, double v)
		{
			auto typed = static_cast<SnexSource::ParameterHandler*>(obj);
			typed->lastValues[P] = v;

			SimpleReadWriteLock::ScopedReadLock sl(typed->getAccessLock());
			typed->pFunctions[P].callVoid(v);
		}

	protected:

		span<snex::jit::FunctionData, OpaqueNode::NumMaxParameters> pFunctions;
		double lastValues[OpaqueNode::NumMaxParameters];
	};

	struct ParameterHandler: public ParameterHandlerLight
	{
		ParameterHandler(SnexSource& s, ObjectStorageType& o) :
			ParameterHandlerLight(s, o)
		{};

		void updateParameters(ValueTree v, bool wasAdded)
		{
			if (wasAdded)
			{
				auto newP = new SnexParameter(&parent, getNode(), v);
				getNode()->addParameter(newP);
			}
			else
			{
				for (int i = 0; i < getNode()->getNumParameters(); i++)
				{
					if (auto sn = dynamic_cast<SnexParameter*>(getNode()->getParameter(i)))
					{
						if (sn->data == v)
						{
							removeSnexParameter(sn);
							break;
						}
					}
				}
			}
		}

		void updateParametersForWorkbench(bool shouldAdd)
		{
			for (int i = 0; i < getNode()->getNumParameters(); i++)
			{
				if (auto sn = dynamic_cast<SnexParameter*>(getNode()->getParameter(i)))
				{
					removeSnexParameter(sn);
					i--;
				}
			}

			if (shouldAdd)
			{
				parameterTree = getNode()->getRootNetwork()->codeManager.getParameterTree(parent.getTypeId(), parent.classId.getValue());
				parameterListener.setCallback(parameterTree, valuetree::AsyncMode::Synchronously, BIND_MEMBER_FUNCTION_2(ParameterHandler::updateParameters));
			}
		}

		void removeSnexParameter(SnexParameter* p)
		{
			p->treeInNetwork.getParent().removeChild(p->treeInNetwork, getNode()->getUndoManager());

			for (int i = 0; i < getNode()->getNumParameters(); i++)
			{
				if (getNode()->getParameter(i) == p)
				{
					getNode()->removeParameter(i);
					break;
				}
			}
		}

		void addNewParameter(parameter::data p)
		{
			if (auto existing = getNode()->getParameter(p.info.getId()))
				return;

			auto newTree = p.createValueTree();
			parameterTree.addChild(newTree, -1, getNode()->getUndoManager());
		}

		NodeBase* getNode() { return parent.parentNode; }

		void removeLastParameter()
		{
			parameterTree.removeChild(parameterTree.getNumChildren() - 1, getNode()->getUndoManager());
		}

		void addParameterCode(String& code);


	private:

		ValueTree parameterTree;
		valuetree::ChildListener parameterListener;
	};

	struct ComplexDataHandlerLight : public HandlerBase,
									 public scriptnode::data::base
	{
		ComplexDataHandlerLight(SnexSource& parent, ObjectStorageType& o):
			HandlerBase(parent, o)
		{

		}

		virtual ~ComplexDataHandlerLight() {};

		void reset() override
		{
			SimpleReadWriteLock::ScopedWriteLock l(getAccessLock());
			externalFunction = {};
		}

		void setExternalData(const snex::ExternalData& d, int index) override
		{
			base::setExternalData(d, index);

			auto v = (void*)(&d);

			SimpleReadWriteLock::ScopedReadLock l(getAccessLock());
			externalFunction.callVoid(v, index);
		}

		Result recompiledOk(snex::jit::ComplexType::Ptr objectClass) override
		{
			auto newFunction = getFunctionAsObjectCallback("setExternalData");
			auto r = newFunction.validateWithArgs(Types::ID::Void, { Types::ID::Pointer, Types::ID::Integer });

			if (r.wasOk())
			{
				SimpleReadWriteLock::ScopedWriteLock l(getAccessLock());
				std::swap(newFunction, externalFunction);
			}

			

			return r;
		}

	protected:

		snex::jit::FunctionData externalFunction;
	};

	struct ComplexDataHandler: public ComplexDataHandlerLight,
							   public ExternalDataHolder,
							   public hise::ComplexDataUIUpdaterBase::EventListener
	{
		ComplexDataHandler(SnexSource& parent, ObjectStorageType& o) :
			ComplexDataHandlerLight(parent, o)
		{};

		~ComplexDataHandler()
		{
			reset();
		}

		int getNumDataObjects(ExternalData::DataType t) const override;

		Table* getTable(int index) override;
		SliderPackData* getSliderPack(int index) override;
		MultiChannelAudioBuffer* getAudioFile(int index) override;
		bool removeDataObject(ExternalData::DataType t, int index) override;

		ExternalDataHolder* getDynamicDataHolder(snex::ExternalData::DataType t, int index);

		void reset() override
		{
			ComplexDataHandlerLight::reset();

			ExternalData::forEachType([this](ExternalData::DataType t)
			{
				for (int i = 0; i < getNumDataObjects(t); i++)
					getComplexBaseType(t, i)->getUpdater().removeEventListener(this);
			});
		}

		void onComplexDataEvent(ComplexDataUIUpdaterBase::EventType t, var data) override
		{
			if (t != ComplexDataUIUpdaterBase::EventType::DisplayIndex)
			{
				for (auto l : parent.compileListeners)
				{
					if (l != nullptr)
						l->complexDataTypeChanged();
				}
			}
		}

		bool hasComplexData() const
		{
			return !tables.isEmpty() || !sliderPacks.isEmpty() || !audioFiles.isEmpty();
		}

		Result recompiledOk(snex::jit::ComplexType::Ptr objectClass) override;
		void initialise(NodeBase* n);
		void addOrRemoveDataFromUI(ExternalData::DataType t, bool shouldAdd);
		void dataAddedOrRemoved(ValueTree v, bool wasAdded);
		ValueTree getDataRoot() { return dataTree; }

		

		static void callExternalDataForAll(ComplexDataHandler& handler, ComplexDataHandlerLight& target)
		{
			ExternalData::forEachType([&handler, &target](ExternalData::DataType t)
			{
				for (int i = 0; i < handler.getNumDataObjects(t); i++)
				{
					auto absoluteIndex = handler.getAbsoluteIndex(t, i);
					ExternalData ed(handler.getComplexBaseType(t, i), absoluteIndex);
					SimpleReadWriteLock::ScopedWriteLock l(ed.obj->getDataLock());
					target.setExternalData(ed, absoluteIndex);
				}
			});
		}

	private:

		ValueTree dataTree;

		valuetree::ChildListener dataListeners[(int)ExternalData::DataType::numDataTypes];

		OwnedArray<snex::ExternalDataHolder> tables;
		OwnedArray<snex::ExternalDataHolder> sliderPacks;
		OwnedArray<snex::ExternalDataHolder> audioFiles;
	};

	struct CallbackHandlerBase : public HandlerBase
	{
		CallbackHandlerBase(SnexSource& p, ObjectStorageType& o) :
			HandlerBase(p, o)
		{};

		virtual ~CallbackHandlerBase() {};

	protected:

		/** Use this in every callback and it will check that the read lock was
		    acquired and the compilation was ok. */
		struct ScopedCallbackChecker
		{
			ScopedCallbackChecker(CallbackHandlerBase& p) :
				parent(p)
			{
				if (parent.ok)
					holdsLock = p.getAccessLock().enterReadLock();
			}

			operator bool() { return parent.ok && holdsLock; }

			~ScopedCallbackChecker()
			{
				parent.getAccessLock().exitReadLock(holdsLock);
			}

			bool holdsLock = false;
			CallbackHandlerBase& parent;
		};

		std::atomic<bool> ok = { false };
	};

	template <class T> struct Tester
	{
		Tester(SnexSource& s):
			dataHandler(s, obj),
			parameterHandler(s, obj),
			callbacks(s, obj)
		{
			static_assert(std::is_base_of<CallbackHandlerBase, T>(), "not a base of CallbackHandlerBase");

			callbacks.reset();
			dataHandler.reset();
			parameterHandler.reset();
			parameterHandler.copyLastValuesFrom(s.getParameterHandler());

			if (auto wb = s.getWorkbench())
			{
				if (auto ptr = wb->getLastResult().mainClassPtr)
				{
					obj.initialise(ptr);

					callbacks.recompiledOk(ptr);
					parameterHandler.recompiledOk(ptr);
					dataHandler.recompiledOk(ptr);
					ComplexDataHandler::callExternalDataForAll(s.getComplexDataHandler(), dataHandler);
				}
			}
		}

		HandlerBase::ObjectStorageType obj;
		ComplexDataHandlerLight dataHandler;
		ParameterHandlerLight parameterHandler;
		T callbacks;
	};

	SnexSource() :
		classId(PropertyIds::ClassId, ""),
		parameterHandler(*this, object),
		dataHandler(*this, object),
		lastResult(Result::fail("uninitialised"))
	{
	};

	~SnexSource()
	{
		setWorkbench(nullptr);
	}

	virtual Identifier getTypeId() const = 0;

	void initialise(NodeBase* n)
	{
		parentNode = n;

		getComplexDataHandler().initialise(n);

		classId.initialise(n);
		classId.setAdditionalCallback(BIND_MEMBER_FUNCTION_2(SnexSource::updateClassId), true);
	}

	void recompiled(WorkbenchData::Ptr wb) final override;

	bool preprocess(String& code) final override
	{
		jassert(code.contains("setParameter("));

		parameterHandler.addParameterCode(code);

		return true;
	}

	String getId() const
	{
		if (parentNode != nullptr)
		{
			return parentNode->getId();
		}
	}

	StringArray getAvailableClassIds()
	{
		return parentNode->getRootNetwork()->codeManager.getClassList(getTypeId());
	}

	virtual String getEmptyText(const Identifier& id) const = 0;

	void setWorkbench(WorkbenchData::Ptr nb)
	{
		if (wb != nullptr)
			wb->removeListener(this);

		wb = nb;
		
		if (parentNode != nullptr)
			parameterHandler.updateParametersForWorkbench(wb != nullptr);

		if (wb != nullptr)
		{
			if (auto dc = dynamic_cast<snex::ui::WorkbenchData::DefaultCodeProvider*>(wb->getCodeProvider()))
				dc->defaultFunction = [this](const Identifier& id) { return this->getEmptyText(id); };

			wb->addListener(this);
			wb->triggerRecompile();
		}
	}

	void updateClassId(Identifier, var newValue)
	{
		auto s = newValue.toString();

		if (s.isNotEmpty())
		{
			auto nb = parentNode->getRootNetwork()->codeManager.getOrCreate(getTypeId(), Identifier(newValue.toString()));
			setWorkbench(nb);
		}
	}

	WorkbenchData::Ptr getWorkbench() { return wb; }

	void setClass(const String& newClassName)
	{
		classId.storeValue(newClassName, parentNode->getUndoManager());
		updateClassId({}, newClassName);
	}

	NodeBase* getParentNode() { return parentNode; }

	void addCompileListener(SnexSourceListener* l)
	{
		compileListeners.addIfNotAlreadyThere(l);

		if(getWorkbench() != nullptr)
			l->wasCompiled(lastResult.wasOk());
	}

	void removeCompileListener(SnexSourceListener* l)
	{
		compileListeners.removeAllInstancesOf(l);
	}

	ParameterHandler& getParameterHandler() { return parameterHandler; }
	ComplexDataHandler& getComplexDataHandler() { return dataHandler; }
	CallbackHandlerBase& getCallbackHandler() { jassert(callbackHandler != nullptr); return *callbackHandler; }

	const ParameterHandler& getParameterHandler() const { return parameterHandler; }
	const ComplexDataHandler& getComplexDataHandler() const { return dataHandler; }
	const CallbackHandlerBase& getCallbackHandler() const { jassert(callbackHandler != nullptr); *callbackHandler; }

protected:

	void setCallbackHandler(CallbackHandlerBase* nonOwnedHandler)
	{
		callbackHandler = nonOwnedHandler;
	}

	Array<WeakReference<SnexSourceListener>> compileListeners;

	static void addDefaultParameterFunction(String& code)
	{
		code << "template <int P> static void setParameter(double v)\n";
		code << "{\n";
		code << "\t\n";
		code << "}\n";
	}

protected:

	ObjectStorage<OpaqueNode::SmallObjectSize, 16> object;

private:

	ParameterHandler parameterHandler;
	ComplexDataHandler dataHandler;
	CallbackHandlerBase* callbackHandler = nullptr;

	Result lastResult;

	// This keeps the function alive until recompiled
	snex::JitObject lastCompiledObject;
	NodePropertyT<String> classId;
	snex::ui::WorkbenchData::Ptr wb;
	WeakReference<NodeBase> parentNode;

	JUCE_DECLARE_WEAK_REFERENCEABLE(SnexSource);
};
#endif

}
