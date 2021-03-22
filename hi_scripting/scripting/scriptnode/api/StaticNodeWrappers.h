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

namespace scriptnode
{

namespace data { 
namespace ui { namespace pimpl { struct editor_base; } }
namespace pimpl { struct dynamic_base; }
}

namespace scriptnode_initialisers
{
	struct no
	{
		static void initialise(void*, NodeBase*)
		{};
	};

}

using namespace juce;
using namespace hise;

struct ComponentHelpers
{
    static NodeComponent* createDefaultComponent(NodeBase* n);
    static void addExtraComponentToDefault(NodeComponent* nc, Component* c);
        
};

struct NoExtraComponent
{
	static Component* createExtraComponent(void* , PooledUIUpdater*) { return nullptr; }
};



struct OpaqueNodeDataHolder: public data::base
{
	struct Editor : public ScriptnodeExtraComponent<OpaqueNodeDataHolder>
	{
		Editor(OpaqueNodeDataHolder* obj, PooledUIUpdater* u);

		void timerCallback() override {}

		void addEditor(data::pimpl::dynamic_base* d);

		void resized() override;

		OwnedArray<data::ui::pimpl::editor_base> editors;
		PooledUIUpdater* updater;
		int height = 0;
	};

	void setExternalData(const snex::ExternalData& d, int index) override;

	OpaqueNodeDataHolder(OpaqueNode& n, NodeBase* pn);

	OpaqueNode& opaqueNode;
	WeakReference<NodeBase> parentNode;
	OwnedArray<data::pimpl::dynamic_base> data;

	JUCE_DECLARE_WEAK_REFERENCEABLE(OpaqueNodeDataHolder);
};

template <class WType> class InterpretedNodeBase
{
public:

	using WrapperType = WType;

	virtual ~InterpretedNodeBase() {};

	InterpretedNodeBase() = default;

	template <typename T, bool AddDataOffsetToUIPtr> void init()
	{
		obj.getWrappedObject().template create<T>();

		if constexpr (AddDataOffsetToUIPtr && std::is_base_of<data::pimpl::provider_base, T>())
		{
			auto offset = T::getDataOffset();
			asWrapperNode()->setUIOffset(offset);	
		}

		this->obj.initialise(asWrapperNode());
		postInit();
	}

	void initFromDll(dll::HostFactory& f, int index)
	{
		f.initOpaqueNode(&obj.getWrappedObject(), index);
		this->obj.initialise(asWrapperNode());

		OpaqueNode& n = obj.getWrappedObject();

		if (n.numDataObjects[0] + n.numDataObjects[1] + n.numDataObjects[2] != 0)
		{
			opaqueDataHolder = new OpaqueNodeDataHolder(n, asWrapperNode());

			asWrapperNode()->extraComponentFunction = [&](void* , PooledUIUpdater* u)
			{
				return new OpaqueNodeDataHolder::Editor(opaqueDataHolder.get(), u);
			};
		}
		
		postInit();
	}

	virtual void postInit()
	{
		ParameterDataList pData;
		obj.getWrappedObject().createParameters(pData);

		asWrapperNode()->initParameterData(pData);
	}


protected:

	WrapperType obj;

	void* getObjectPtrFromWrapper()
	{
		return obj.getWrappedObject().getObjectPtr();
	}

	ParameterDataList createInternalParameterListFromWrapper()
	{
		ParameterDataList pList;
		obj.getWrappedObject().createParameters(pList);
		return pList;
	}

private:

	ScopedPointer<OpaqueNodeDataHolder> opaqueDataHolder;

	WrapperNode* asWrapperNode()
	{
		return dynamic_cast<WrapperNode*>(this);
	}
};

    
class InterpretedNode : public WrapperNode,
						public InterpretedNodeBase<bypass::smoothed<OpaqueNode>>
{
	using Base = InterpretedNodeBase<bypass::smoothed<OpaqueNode>>;

public:

	

	InterpretedNode(DspNetwork* parent, ValueTree d):
		WrapperNode(parent, d),
		Base()
	{}

	void postInit() override
	{
		setDefaultValue(PropertyIds::BypassRampTimeMs, 20.0f);
		Base::postInit();
	}

    template <typename T, typename ComponentType, bool AddDataOffsetToUIPtr> static NodeBase* createNode(DspNetwork* n, ValueTree d) 
	{ 
		auto newNode = new InterpretedNode(n, d); 

		newNode->template init<T, AddDataOffsetToUIPtr>();
		newNode->extraComponentFunction = ComponentType::createExtraComponent;
		return newNode;
	}; 

	SN_OPAQUE_WRAPPER(InterpretedModNode, WrapperType);

	void* getObjectPtr() override
	{
		return getObjectPtrFromWrapper();

	}

	ParameterDataList createInternalParameterList() override
	{
		return createInternalParameterListFromWrapper();
	}

	void reset();

	bool isPolyphonic() const override { return this->obj.isPolyphonic(); }

	void prepare(PrepareSpecs specs) final override;
	void processFrame(NodeBase::FrameType& data) final override;
	void processMonoFrame(MonoFrameType& data) final override;
	void processStereoFrame(StereoFrameType& data) final override;
	void process(ProcessDataDyn& data) noexcept final override;
	void setBypassed(bool shouldBeBypassed) final override;
	void handleHiseEvent(HiseEvent& e) final override;

	valuetree::PropertyListener bypassListener;
};

class InterpretedModNode : public ModulationSourceNode,
						   public InterpretedNodeBase<wrap::mod<parameter::dynamic_base_holder, OpaqueNode>>
{
	using Base = InterpretedNodeBase<wrap::mod<parameter::dynamic_base_holder, OpaqueNode>>;

public:

	SN_OPAQUE_WRAPPER(InterpretedModNode, WrapperType);

	InterpretedModNode(DspNetwork* parent, ValueTree d):
		ModulationSourceNode(parent, d),
		Base()
	{

	}

	void postInit() override
	{
		getParameterHolder()->setRingBuffer(ringBuffer.get());
		stop();
		Base::postInit();
	}

	template <typename T, typename ComponentType, bool AddDataOffsetToUIPtr> static NodeBase* createNode(DspNetwork* n, ValueTree d)
	{ 
		auto mn = new InterpretedModNode(n, d); 
		mn->init<T, AddDataOffsetToUIPtr>();
		mn->extraComponentFunction = ComponentType::createExtraComponent;

		return mn;
	};

	void* getObjectPtr() override
	{
		return getObjectPtrFromWrapper();

	}

	ParameterDataList createInternalParameterList() override
	{
		return createInternalParameterListFromWrapper();
	}

	void timerCallback() override;

	bool isUsingNormalisedRange() const override;

	parameter::dynamic_base_holder* getParameterHolder() override;

	bool isPolyphonic() const override;

	void reset();
	void prepare(PrepareSpecs specs) final override;
	void writeToRingBuffer(double value, int numSamplesForAnalysis);
	void processFrame(NodeBase::FrameType& data) final override;
	void processMonoFrame(MonoFrameType& data) final override;
	void processStereoFrame(StereoFrameType& data) final override;
	void process(ProcessDataDyn& data) noexcept final override;
	void handleHiseEvent(HiseEvent& e) final override;

	WrapperType wrapper;
};




#if 0
struct ParameterNodeBase : public ModulationSourceNode
{
	ParameterNodeBase(DspNetwork* n, ValueTree d) :
		ModulationSourceNode(n, d)
	{};

	void timerCallback() override
	{
		getParameterHolder()->updateUI();
	}

	bool isUsingNormalisedRange() const override
	{
		return true;
	}

	void reset()
	{

	}

	void prepare(PrepareSpecs ps) final override
	{

	}

	void processFrame(NodeBase::FrameType& data) final override
	{

	}

	void processMonoFrame(MonoFrameType& data) final override
	{
	}

	void processStereoFrame(StereoFrameType& data) final override
	{
	}

	void process(ProcessDataDyn& data) noexcept final override
	{
	}


	void handleHiseEvent(HiseEvent& e) final override
	{}

	virtual parameter::dynamic_base_holder* getParameterHolder() = 0;
};

struct ParameterMultiplyAddNode : public ParameterNodeBase
{
	ParameterMultiplyAddNode(DspNetwork* n, ValueTree d) :
		ParameterNodeBase(n, d)
	{
		ParameterDataList pData;
		obj.createParameters(pData);
		initParameterData(pData);

	};

	parameter::dynamic_base_holder* getParameterHolder() override
	{
		return &obj.getParameter();
	}

	static Identifier getStaticId() { RETURN_STATIC_IDENTIFIER("pma"); };

	void* getObjectPtr() override 
	{
		return &obj; 
	};

	static NodeBase* createNode(DspNetwork* n, ValueTree d) 
	{ 
		auto node =  new ParameterMultiplyAddNode(n, d); 
		node->extraComponentFunction = CombinedParameterDisplay::createExtraComponent;
		return node;
	};

	ParameterDataList createInternalParameterList() override
	{  
		ParameterDataList pData;
		obj.createParameters(pData); 
		return pData; 
	}

	core::pma<parameter::dynamic_base_holder, 1> obj;
};
#endif

namespace parameter
{
    struct dynamic_list;
}

struct NewHero : public ModulationSourceNode,
				 public InterpretedNodeBase<OpaqueNode>
{
	using Base = InterpretedNodeBase<OpaqueNode>;

	NewHero(DspNetwork* n, ValueTree d) :
		ModulationSourceNode(n, d),
		Base()
	{
		
	};

	template <typename T, typename ComponentType, bool AddDataOffsetToUIPtr> static NodeBase* createNode(DspNetwork* n, ValueTree d)
	{
		constexpr bool isBaseOfDynamicParameterHolder = std::is_base_of<control::pimpl::parameter_node_base<parameter::dynamic_base_holder>, typename T::WrappedObjectType>();

		static_assert(std::is_base_of<control::pimpl::no_processing, typename T::WrappedObjectType>(), "not a base of no_processing");

		auto mn = new NewHero(n, d);

		if constexpr (isBaseOfDynamicParameterHolder)
			mn->getParameterFunction = NewHero::getParameterFunctionStatic<T>;
		else
		{
			constexpr bool isBaseOfDynamicList = std::is_base_of<control::pimpl::parameter_node_base<parameter::dynamic_list>, typename T::WrappedObjectType>();
			static_assert(isBaseOfDynamicList, "not a base of dynamic holder or list");
			mn->getParameterFunction = nullptr;
		}

		mn->init<T, AddDataOffsetToUIPtr>();
		mn->extraComponentFunction = ComponentType::createExtraComponent;

		return mn;
	};

	void* getObjectPtr() override
	{
		return getObjectPtrFromWrapper();
	}

	ParameterDataList createInternalParameterList() override
	{
		return createInternalParameterListFromWrapper();
	}

	parameter::dynamic_base_holder* getParameterHolder() override
	{
		if(getParameterFunction)
			return getParameterFunction(getObjectPtr());

		return nullptr;
	}

	void process(ProcessDataDyn& data) final override
	{

	}

	void reset() final override
	{

	}

	void timerCallback() override
	{
		if(auto p = getParameterHolder())
			p->updateUI();
	}

	bool isUsingNormalisedRange() const override
	{
		return this->obj.getWrappedObject().isNormalised;
	}

	void prepare(PrepareSpecs ps) final override;

	void processFrame(FrameType& data) final override
	{

	}

	void handleHiseEvent(HiseEvent& e) final override
	{
		this->obj.handleHiseEvent(e);
	}

private:

	typedef parameter::dynamic_base_holder*(*getParamFunc)(void*);

	template <class Derived> static parameter::dynamic_base_holder* getParameterFunctionStatic(void* b)
	{
		using BaseClass = control::pimpl::parameter_node_base<parameter::dynamic_base_holder>;

		static_assert(std::is_base_of<BaseClass, typename Derived::WrappedObjectType>());

		auto typed = static_cast<Derived*>(b);
		return &typed->getWrappedObject().getParameter();
	};

	getParamFunc getParameterFunction;
};



class RestorableNode
{
public:

	virtual ~RestorableNode()
	{

	}

	virtual String getSnippetText() const { return ""; }
};

#if OLD_SCRIPTNODE_CPP
template<int... Indexes> using NodePath = std::integer_sequence<int, Indexes...>;

template <class T> static auto& findNode(T& t, NodePath<> empty)
{
	ignoreUnused(empty);
	return t.getObject();
}

template <class T, int Index, int... Indexes> static auto& findNode(T& t, NodePath<Index, Indexes...> )
{
	auto& obj = t.template get<Index>().getObject();
	NodePath<Indexes...> seq;
	return findNode(obj, seq);
}

#define FIND_NODE(obj, ...) &findNode(obj.getObject(), NodePath<__VA_ARGS__>())

class HardcodedNode : public RestorableNode
{
public:

    virtual ~HardcodedNode() 
	{
		combinedParameterValues.clear();
	};
    
	struct ParameterInitValue
	{
		ParameterInitValue(const char* id_, double v) :
			id(id_),
			value(v)
		{};

		String id;
		double value;
	};

	struct CombinedParameterValue
	{
		CombinedParameterValue(const HiseDspBase::ParameterData& p):
			id(p.id),
			callback(p.dbNew),
			lastValue(p.defaultValue)
		{

		}

		bool matches(const HiseDspBase::ParameterData& p) const
		{
			return id == p.id;
		}

		void SetValue(double newValue)
		{
			lastValue = newValue;
			update();
		}

		void Add(double newValue)
		{
			addValue = newValue;
			update();
		}

		void Multiply(double newValue)
		{
			mulValue = newValue;
			update();
		}
		
		void updateRangeForOpType(Identifier opType, NormalisableRange<double> newRange)
		{
			if (opType == OperatorIds::Add)
			{
				addRange = newRange;
				useAddRange = !RangeHelpers::isIdentity(newRange);
			}
			if (opType == OperatorIds::SetValue)
			{
				setRange = newRange;
				useSetRange = !RangeHelpers::isIdentity(newRange);
			}
			if (opType == OperatorIds::Multiply)
			{
				mulRange = newRange;
				useMulRange = !RangeHelpers::isIdentity(newRange);
			}
		}

		void addConversion(Identifier converterId, Identifier opType)
		{
			if (opType == OperatorIds::SetValue)
				setConverter = DspHelpers::ConverterFunctions::getFunction(converterId);
			if(opType == OperatorIds::Multiply)
				mulConverter = DspHelpers::ConverterFunctions::getFunction(converterId);
			if(opType == OperatorIds::Add)
				addConverter = DspHelpers::ConverterFunctions::getFunction(converterId);
		}

	private:

		void update()
		{
			auto newValue = getSet() * getMul() + getAdd();
			callback(newValue);
		}

		std::function<void(double)> callback;

		double getSet() const
		{
			return useSetRange ? setRange.convertFrom0to1(getSetConverted()) : getSetConverted();
		}

		double getMul() const
		{
			return useMulRange ? mulRange.convertFrom0to1(getMullConverted()) : getMullConverted();
		}

		double getAdd() const
		{
			return useAddRange ? addRange.convertFrom0to1(getAddConverted()) : getAddConverted();
		}

		double getSetConverted() const
		{
			return setConverter ? setConverter(lastValue) : lastValue;
		}

		double getAddConverted() const
		{
			return addConverter ? addConverter(addValue) : addValue;
		}

		double getMullConverted() const
		{
			return mulConverter ? mulConverter(mulValue) : mulValue;
		}

		String id;
		double lastValue = 0.0;
		double mulValue = 1.0;
		double addValue = 0.0;

		NormalisableRange<double> mulRange;
		NormalisableRange<double> setRange;
		NormalisableRange<double> addRange;

		std::function<double(double)> setConverter;
		std::function<double(double)> mulConverter;
		std::function<double(double)> addConverter;

		bool useSetRange = false;
		bool useMulRange = false;
		bool useAddRange = false;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CombinedParameterValue);
	};

	CombinedParameterValue* getCombinedParameter(const String& id, NormalisableRange<double> range, Identifier opType);

	static constexpr int ExtraHeight = 0;

	static ParameterDataList createParametersT(ParameterHolder* d, const String& prefix)
	{
		ParameterDataList data;
		d->createParameters(data);

		if (prefix.isNotEmpty())
		{
			for (auto& c : data)
				c.id = prefix + "." + c.id;
		}

		return data;
	}

	void fillInternalParameterList(ParameterHolder* obj, const String& name)
	{
		internalParameterList.addArray(createParametersT(obj, name));
	}

	template <class T> void setInternalModulationParameter(T* modObject, const DspHelpers::ParameterCallback& f)
	{
		modObject->setCallback(f);
	}

	void setNodeProperty(const String& id, const var& newValue, bool isPublic);

	var getNodeProperty(const String& id, const var& defaultReturnValue) const;

	void setParameterDefault(const String& parameterId, double value);

	ParameterDataList internalParameterList;
	Array<ParameterInitValue> initValues;

	OwnedArray<CombinedParameterValue> combinedParameterValues;

	HiseDspBase::ParameterData getParameter(const String& id);

	HiseDspBase::ParameterData getParameter(const String& id, NormalisableRange<double> d);

	

	String getNodeId(const HiseDspBase* internalNode) const;

	HiseDspBase* getNode(const String& id) const;

	template <class T> void registerNode(T* obj, const String& id)
	{
		if (auto typed = dynamic_cast<HiseDspBase*>(&obj->getObject()))
		{
			
			internalNodes.add({ typed, id });
			fillInternalParameterList(typed, id);
		}
	}

	void addPublicComponent(const String& nodeId)
	{
		nodesWithPublicComponent.add(nodeId);


		
	}
    
protected:

	StringArray getNodeIdsWithPublicComponent()
	{
		return nodesWithPublicComponent;
	}

	ValueTree nodeData;
	UndoManager* um = nullptr;

private:

	StringArray nodesWithPublicComponent;

	ValueTree getNodePropertyTree(const String& id) const;

	struct InternalNode
	{
		HiseDspBase* node = nullptr;
		String id;
	};

	Array<InternalNode> internalNodes;
};





struct hc
{
	static constexpr int no_modulation = 1;
	static constexpr int with_modulation = 2;
};

#define DEF_CONSTRUCTOR instance(); ~instance();
#define DEF_PIMPL struct Impl; Impl* pimpl;
#define DEF_PREPARE_PIMPL void prepare(PrepareSpecs ps);
#define DEF_INIT_PIMPL void initialise(NodeBase* n);
#define DEF_HANDLE_MOD_PIMPL bool handleModulation(double& value);
#define DEF_HANDLE_EVENT_PIMPL void handleHiseEvent(HiseEvent& e);
#define DEF_PROCESS_PIMPL void process(ProcessData& data) noexcept;
#define DEF_RESET_PIMPL void reset();
#define DEF_PROCESS_SINGLE_PIMPL void processFrame(float* frameData, int numChannels) noexcept;

#define DEFINE_DSP_METHODS_PIMPL DEF_CONSTRUCTOR; DEF_PIMPL; DEF_PREPARE_PIMPL; DEF_INIT_PIMPL; DEF_HANDLE_MOD_PIMPL; DEF_HANDLE_EVENT_PIMPL; DEF_PROCESS_PIMPL; DEF_RESET_PIMPL; DEF_PROCESS_SINGLE_PIMPL;

#define DEFINE_PIMPL_CLASS(NodeType) struct instance::Impl : public NodeType {  };
#define DEFINE_CONSTRUCTOR instance::instance() { pimpl = new Impl(); }
#define DEFINE_DESTRUCTOR instance::~instance() { delete pimpl; }
#define PREPARE_PIMPL void instance::prepare(PrepareSpecs ps) { pimpl->prepare(ps); }
#define INIT_PIMPL void instance::initialise(NodeBase* n) { pimpl->initialise(n); nodeData = n->getValueTree(); um = n->getUndoManager(); }
#define HANDLE_MOD_PIMPL bool instance::handleModulation(double& value) { return pimpl->handleModulation(value); }
#define HANDLE_EVENT_PIMPL void instance::handleHiseEvent(HiseEvent& e) { pimpl->handleHiseEvent(e); }
#define PROCESS_PIMPL void instance::process(ProcessData& data) noexcept { pimpl->process(data); }
#define RESET_PIMPL void instance::reset() { pimpl->reset(); }
#define PROCESS_SINGLE_PIMPL void instance::processFrame(float* frameData, int numChannels) noexcept { pimpl->processFrame(frameData, numChannels); }

#define DSP_METHODS_PIMPL_IMPL(NodeType) DEFINE_PIMPL_CLASS(NodeType); DEFINE_CONSTRUCTOR; DEFINE_DESTRUCTOR; PREPARE_PIMPL; INIT_PIMPL; HANDLE_MOD_PIMPL; HANDLE_EVENT_PIMPL; PROCESS_PIMPL; RESET_PIMPL; PROCESS_SINGLE_PIMPL;

struct HardcodedNodeComponent : public Component
{
public:

	HardcodedNodeComponent(HardcodedNode* hc, const StringArray& nodeIds, PooledUIUpdater* updater)
	{
		int w = 256;
		int h = 0;

#if RE
		for (auto nId : nodeIds)
		{
			if (auto node = hc->getNode(nId))
			{
				auto newComponent = node->createExtraComponent(updater);
				newComponent->setSize(w, newComponent->getHeight());
				components.add(newComponent);
				addAndMakeVisible(newComponent);

				h += newComponent->getHeight();
			}
		}
#endif

		setSize(w, h);
	}

	void resized() override
	{
		int y = 0;

		for (auto c : components)
		{
			c->setTopLeftPosition(0, y);
			y += c->getHeight();
		}
	}

	OwnedArray<Component> components;
};
#endif

#if HISE_INCLUDE_SNEX
    
#define SET_HISE_BLOCK_CALLBACK(s) static constexpr int BlockCallback = CallbackTypes::s;
#define SET_HISE_FRAME_CALLBACK(s) static constexpr int FrameCallback = CallbackTypes::s;

struct jit_base : public RestorableNode
{
	struct Parameter
	{
		String id;
		std::function<void(double)> f;
	};

	virtual ~jit_base() {};

	struct ConsoleDummy
	{
		template <typename T> void print(const T& value)
		{
			DBG(value);
		}
	} Console;

	virtual void createParameters()
	{

	}

	void addParameter(String name, const std::function<void(double)>& f)
	{
		parameters.add({ name, f });
	}

	Array<Parameter> parameters;

	snex::hmath Math;
};
    
#endif

#if OLD_SCRIPTNODE_CPP
struct hardcoded_base : public HiseDspBase,
	public HardcodedNode
{
	virtual ~hardcoded_base() {};

	int extraHeight = -1;
	int extraWidth = -1;
};
#endif



#if OLD_SCRIPTNODE_CPP
struct hardcoded_pimpl : public hardcoded_base
{
	virtual ~hardcoded_pimpl() {};

};


template <class DspProcessorType> struct hardcoded : hardcoded_base
{
public:

	virtual ~hardcoded() {};
	
	void initialise(NodeBase* n) override
	{
		obj.initialise(n);

		nodeData = n->getValueTree();
		um = n->getUndoManager();
	}

	void prepare(PrepareSpecs ps) {
		obj.prepare(ps);
	}

	bool handleModulation(double& value)
	{
		return obj.handleModulation(value);
	}

	void handleHiseEvent(HiseEvent& e) final override
	{
		obj.handleHiseEvent(e);
	}

	template <typename ProcessDataType> void process(ProcessDataType& data) noexcept
	{
		this->obj.process(data);
	}

	void reset()
	{
		obj.reset();
	}

	template <typename FrameDataType> void processFrame(FrameDataType& data) noexcept
	{
		this->obj.processFrame(data);
	}

	DspProcessorType obj;
};
#endif
    
#define SCRIPTNODE_FACTORY(x, id) static NodeBase* createNode(DspNetwork* n, ValueTree d) { return new x(n, d); }; \
static Identifier getStaticId() { return Identifier(id); };
    

#if 0
struct FilterTestNode : public snex::Types::SnexNodeBase
{
	FilterTestNode(const snex::Types::SnexTypeConstructData& cd_) :
		SnexNodeBase(cd_)
	{
		
	}

	template <typename ObjectType> void init()
	{
		cd.id = cd.id.getChildId(ObjectType::getStaticId());
		cd.c.registerExternalComplexType(this);
		functionCreator = DefaultFunctionClass(&obj);
	}

	snex::Types::OpaqueSnexParameter::List getParameterList() override
	{
		ParameterDataList data;
		obj.createParameters(data);

		OpaqueSnexParameter::List l;

		for (auto& p : data)
		{
			OpaqueSnexParameter osp;
			osp.function = p.dbNew.getFunction();
			osp.data.id = p.id;

			l.add(osp);
		}

		return l;
	}

	

	/** Override this and return the size of the object. It will be used by the allocator to create the memory. */
	size_t getRequiredByteSize() const override
	{
		return sizeof(obj);
	}

	Result initialise(InitData d) override
	{
		memcpy(d.dataPointer, &obj, getRequiredByteSize());
		reinterpret_cast<ObjectType*>(d.dataPointer)->initialise(reinterpret_cast<NodeBase*>(cd.nodeParent));
		return Result::ok();
	}
};
#endif

/** This namespace contains functions that initialise nodes.

	It's been put here so that it can acccess the NodeBase type.

*/
namespace init
{

}
    struct NodeFactory
    {
		using CreateCallback = std::function<NodeBase*(DspNetwork*, ValueTree)>;

	protected:

		struct Item
		{
			CreateCallback cb;
			Identifier id;
		};

    public:
        
        NodeFactory(DspNetwork* n) :
        network(n)
        {};
        
        virtual ~NodeFactory() {};
        
        StringArray getModuleList() const
        {
            StringArray sa;
            String prefix = getId().toString() + ".";
            
            for (const auto& item : monoNodes)
            {
                sa.add(prefix + item.id.toString());
            }
            
            return sa;
        }
        
		template <class MonoT, class PolyT> void registerPolyNodeRaw()
		{
			{
				Item newItem;
				newItem.cb = PolyT::createNode;
				newItem.id = PolyT::getStaticId();

				polyNodes.add(newItem);
			}

			{
				Item newItem;
				newItem.cb = MonoT::createNode;
				newItem.id = MonoT::getStaticId();

				monoNodes.add(newItem);
			}
		}

        template <class T> void registerNodeRaw()
        {
            Item newItem;
            newItem.cb = T::createNode;
            newItem.id = T::getStaticId();
            
            monoNodes.add(newItem);
        }
        

        template <class T, 
				  class ComponentType=NoExtraComponent, 
				  typename WrapperType=InterpretedNode, 
				  bool AddDataOffsetToUIPtr=true> 
			void registerNode()
        {
			Item newItem;
            newItem.cb = WrapperType::template createNode<T, ComponentType, AddDataOffsetToUIPtr>;
            newItem.id = T::getStaticId();
			
            monoNodes.add(newItem);
        };

        template <class MonoT, 
				  class PolyT, 
				  class ComponentType=NoExtraComponent, 
				  typename WrapperType=InterpretedNode,
				  bool AddDataOffsetToUIPtr=true>
			void registerPolyNode()
        {
            using WrappedPolyT = InterpretedNode;
            using WrappedMonoT = InterpretedNode;
            
            {
                Item newItem;
                newItem.cb = WrapperType::template createNode<PolyT, ComponentType, AddDataOffsetToUIPtr>;
                newItem.id = PolyT::getStaticId();
                
                polyNodes.add(newItem);
            }
            
            {
                Item newItem;
                newItem.cb = WrapperType::template createNode<MonoT, ComponentType, AddDataOffsetToUIPtr>;
                newItem.id = MonoT::getStaticId();
                
                monoNodes.add(newItem);
            }
        };

		template <class MonoT, class PolyT, class ComponentType=ModulationSourcePlotter, bool AddDataOffsetToUIPtr = true> void registerPolyModNode()
		{
			registerPolyNode<MonoT, PolyT, ComponentType, InterpretedModNode, AddDataOffsetToUIPtr>();
		}

		template <class MonoT, class ComponentType = ModulationSourcePlotter, bool AddDataOffsetToUIPtr=true> void registerModNode()
		{
			registerNode<MonoT, ComponentType, InterpretedModNode, AddDataOffsetToUIPtr>();
		}

		template <class MonoT, class ComponentType = ModulationSourcePlotter, bool AddDataOffsetToUIPtr = true> void registerNoProcessNode()
		{
			registerNode<MonoT, ComponentType, NewHero, AddDataOffsetToUIPtr>();
		}

        virtual Identifier getId() const = 0;
        
        NodeBase* createNode(ValueTree data, bool createPolyIfAvailable) const;
        
        void setNetworkToUse(DspNetwork* n)
        {
            network = n;
        }
        
		void sortEntries()
		{
			struct Sorter
			{
				static int compareElements(Item& first, Item& second)
				{
					return first.id.toString().compareNatural(second.id.toString());
				}
			} sorter;

			monoNodes.sort(sorter);
			polyNodes.sort(sorter);
		}

	protected:


        Array<Item> monoNodes;
        Array<Item> polyNodes;
        
        WeakReference<DspNetwork> network;
		JUCE_DECLARE_WEAK_REFERENCEABLE(NodeFactory);
    };
    
    class SingletonFactory : public NodeFactory
    {
    public:
        
        SingletonFactory() :
        NodeFactory(nullptr)
        {};
    };
    
#define DEFINE_FACTORY_FOR_NAMESPACE NodeFactory* Factory::instance = nullptr; \
NodeFactory* Factory::getInstance(DspNetwork* ) \
{ if (instance == nullptr) instance = new Factory(); return instance; }
    
#define DECLARE_SINGLETON_FACTORY_FOR_NAMESPACE(name) class Factory : private SingletonFactory \
{ \
public: \
Identifier getId() const override { RETURN_STATIC_IDENTIFIER(#name); } \
static NodeFactory* getInstance(DspNetwork* n); \
static NodeFactory* instance; \
}; 
    
    
    template <class FactoryClass, class T> struct RegisterAtFactory
    {
        RegisterAtFactory() { FactoryClass::getInstance(nullptr)->template registerNode<T>(); }
    };
    
    template <class FactoryClass, class T, class PolyT> struct PolyRegisterAtFactory
    {
        PolyRegisterAtFactory() { FactoryClass::getInstance(nullptr)->template registerPolyNode<T, PolyT>(); }
    };
    
#define REGISTER_POLY PolyRegisterAtFactory<Factory, instance<1>, instance<NUM_POLYPHONIC_VOICES>> reg;
#define REGISTER_MONO RegisterAtFactory<Factory, instance> reg;

#define REGISTER_POLY_SNEX PolyRegisterAtFactory<Factory, hardcoded_jit<instance, 1>, hardcoded_jit<instance, NUM_POLYPHONIC_VOICES>> reg;
#define REGISTER_MONO_SNEX RegisterAtFactory<Factory, hardcoded_jit<instance, 1>> reg;


}
