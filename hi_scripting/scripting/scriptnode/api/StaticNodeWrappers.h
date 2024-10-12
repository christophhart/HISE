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

namespace control
{
	/** Oh boy, this is ugly. The mothernode system does not work with dynamic data types, so the UI
		offset crashes the component creation. Therefore we need to roll an own class .*/
	struct dynamic_dupli_pack : public wrap::data<control::clone_pack<parameter::clone_holder>, data::dynamic::sliderpack>
	{
		static parameter::dynamic_base_holder* getParameterFunction(void* obj);
	};
}

namespace data { 
namespace ui { namespace pimpl { struct editor_base; } }
namespace pimpl { struct dynamic_base; }
}

namespace parameter { struct dynamic_list; }

using namespace juce;
using namespace hise;

struct ComponentHelpers
{
    static NodeComponent* createDefaultComponent(NodeBase* n);
    static void addExtraComponentToDefault(NodeComponent* nc, Component* c);
};

struct OpaqueNodeDataHolder: public data::base,
						     public ExternalDataHolderWithForcedUpdate,
							 public ExternalDataHolderWithForcedUpdate::ForcedUpdateListener
{
	struct Editor : public ScriptnodeExtraComponent<OpaqueNodeDataHolder>
	{
		Editor(OpaqueNodeDataHolder* obj, PooledUIUpdater* u, bool addDragger);

		void timerCallback() override {}

		void addEditor(data::pimpl::dynamic_base* d);

		void resized() override;

		OwnedArray<data::ui::pimpl::editor_base> editors;
		PooledUIUpdater* updater;
		int height = 0;
		int width = 0;
		ScopedPointer<ModulationSourceBaseComponent> dragger;
	};

	void setExternalData(const snex::ExternalData& d, int index) override;

	OpaqueNodeDataHolder(OpaqueNode& n, NodeBase* pn);

	~OpaqueNodeDataHolder();

	void forceRebuild(ExternalData::DataType dt, int index) override
	{
		sendForceUpdateMessage(this, dt, index);
	}

	data::pimpl::dynamic_base* create(ExternalData::DataType dt, int index);

	void createDataType(ExternalData::DataType dt);

	OpaqueNode& opaqueNode;
	WeakReference<NodeBase> parentNode;
	OwnedArray<data::pimpl::dynamic_base> data;

	virtual int getNumDataObjects(ExternalData::DataType t) const override;
	virtual Table* getTable(int index) override;
	virtual SliderPackData* getSliderPack(int index) override;
	virtual MultiChannelAudioBuffer* getAudioFile(int index) override;
	virtual FilterDataObject* getFilterData(int index) override;
	virtual SimpleRingBuffer* getDisplayBuffer(int index) override;
	virtual bool removeDataObject(ExternalData::DataType t, int index) override;

	JUCE_DECLARE_WEAK_REFERENCEABLE(OpaqueNodeDataHolder);
};

template <class WType> class InterpretedNodeBase
{
public:

	using WrapperType = WType;

	virtual ~InterpretedNodeBase() 
	{
		if (nodeFactory != nullptr)
		{
			nodeFactory->deinitOpaqueNode(&obj.getWrappedObject());
		}
	};

	InterpretedNodeBase() = default;

	template <typename T, bool AddDataOffsetToUIPtr, bool UseNodeBaseAsUI=false> void init()
	{
		obj.getWrappedObject().template create<T>();

		if constexpr (AddDataOffsetToUIPtr && std::is_base_of<data::pimpl::provider_base, typename T::ObjectType>())
		{
			auto offset = T::ObjectType::getDataOffset();
			asWrapperNode()->setUIOffset(offset);	
		}
		else if constexpr (UseNodeBaseAsUI)
		{
			auto asUint8Ptr = reinterpret_cast<uint8*>(this);
			auto objPtr = reinterpret_cast<uint8*>(getObjectPtrFromWrapper());

			auto offset = asUint8Ptr - objPtr;
			asWrapperNode()->setUIOffset(offset);
		}

		this->obj.initialise(asWrapperNode());
		postInit();
	}

	ExternalDataHolder* setOpaqueDataEditor(bool addDragger)
	{
		OpaqueNode& n = obj.getWrappedObject();

		if (n.hasComplexData())
		{
			opaqueDataHolder = new OpaqueNodeDataHolder(n, asWrapperNode());

			asWrapperNode()->extraComponentFunction = [this, addDragger](void*, PooledUIUpdater* u)
			{
				return new OpaqueNodeDataHolder::Editor(this->opaqueDataHolder.get(), u, addDragger);
			};

			return opaqueDataHolder.get();
		}
		else if (addDragger)
		{
			asWrapperNode()->extraComponentFunction = [&](void*, PooledUIUpdater* u)
			{
				auto c = new ModulationSourceBaseComponent(u);
				c->setSize(256, 32);
				return c;
			};

		}

		return nullptr;
	}

	void initFromDll(dll::FactoryBase* f, int index, bool addDragger)
	{
		nodeFactory = f;

		f->initOpaqueNode(&obj.getWrappedObject(), index, asWrapperNode()->getRootNetwork()->isPolyphonic());
		this->obj.initialise(asWrapperNode());

		setOpaqueDataEditor(addDragger);
		
		postInit();
		auto mc = asWrapperNode()->getScriptProcessor()->getMainController_();
		mc->connectToRuntimeTargets(obj.getWrappedObject(), true);
	}

	virtual void postInit()
	{
		ParameterDataList pData;
		obj.getWrappedObject().createParameters(pData);

		asWrapperNode()->initParameterData(pData);
	}

	WrapperType& getWrapperType() { return obj; }

	

protected:

	WrapperType obj;

	void* getObjectPtrFromWrapper()
	{
		if (auto o = obj.getWrappedObject().getObjectAsMotherNode())
			return o;

		return obj.getWrappedObject().getObjectPtr();
	}

	ParameterDataList createInternalParameterListFromWrapper()
	{
		ParameterDataList pList;
		obj.getWrappedObject().createParameters(pList);
		return pList;
	}

private:

	dll::FactoryBase* nodeFactory = nullptr;

	ScopedPointer<OpaqueNodeDataHolder> opaqueDataHolder;

	WrapperNode* asWrapperNode()
	{
		return dynamic_cast<WrapperNode*>(this);
	}
};

    
class InterpretedNode : public WrapperNode,
						public InterpretedNodeBase<bypass::simple<OpaqueNode>>
{
	using Base = InterpretedNodeBase<bypass::simple<OpaqueNode>>;

public:

	InterpretedNode(DspNetwork* parent, ValueTree d):
		WrapperNode(parent, d),
		Base()
	{}

	void postInit() override
	{
		Base::postInit();
	}

	void connectToRuntimeTarget(bool shouldConnect) override
	{
		getScriptProcessor()->getMainController_()->connectToRuntimeTargets(obj.getWrappedObject(), shouldConnect);
	}

    template <typename T, typename ComponentType, bool AddDataOffsetToUIPtr, bool UseNodeBaseAsUI> static NodeBase* createNode(DspNetwork* n, ValueTree d) 
	{ 
		auto newNode = new InterpretedNode(n, d); 

		newNode->template init<T, AddDataOffsetToUIPtr, UseNodeBaseAsUI>();
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

	String getNodeDescription() const override { return obj.getWrappedObject().getDescription(); }

	void reset();

	bool isPolyphonic() const override { return this->obj.isPolyphonic(); }

	bool isProcessingHiseEvent() const override
	{
		return this->obj.isProcessingHiseEvent();
	}

	void prepare(PrepareSpecs specs) final override;
	void processFrame(NodeBase::FrameType& data) final override;
	void processMonoFrame(MonoFrameType& data) final override;
	void processStereoFrame(StereoFrameType& data) final override;
	void process(ProcessDataDyn& data) noexcept final override;
	void setBypassed(bool shouldBeBypassed) final override;
	void handleHiseEvent(HiseEvent& e) final override;

	valuetree::PropertyListener bypassListener;
};

using ModWrapType_ = bypass::simple<wrap::mod<parameter::dynamic_base_holder, OpaqueNode>>;

class InterpretedModNode : public ModulationSourceNode,
						   public InterpretedNodeBase<ModWrapType_>
{
	using Base = InterpretedNodeBase<ModWrapType_>;

public:

	SN_OPAQUE_WRAPPER(InterpretedModNode, WrapperType);

	InterpretedModNode(DspNetwork* parent, ValueTree d);

	void postInit() override;

	bool isProcessingHiseEvent() const override
	{
		return obj.isProcessingHiseEvent();
	}

	String getNodeDescription() const override { return obj.getWrappedObject().getDescription(); }

	template <typename T, typename ComponentType, bool AddDataOffsetToUIPtr, bool Unused> static NodeBase* createNode(DspNetwork* n, ValueTree d)
	{ 
		auto mn = new InterpretedModNode(n, d); 
		mn->init<T, AddDataOffsetToUIPtr>();
		mn->extraComponentFunction = ComponentType::createExtraComponent;

		return mn;
	};

	static Component* createModPlotter(void* obj, PooledUIUpdater* updater)
	{
		auto b = static_cast<data::base*>(obj);
		auto n = new ModPlotter();
		n->setComplexDataUIBase(b->getUIPointer());
		return n;
	}

	void* getObjectPtr() override;

	ParameterDataList createInternalParameterList() override;

	bool isUsingNormalisedRange() const override;

	parameter::dynamic_base_holder* getParameterHolder() override;

	bool isPolyphonic() const override;

	void reset();
	void prepare(PrepareSpecs specs) final override;
	void processFrame(NodeBase::FrameType& data) final override;
	void processMonoFrame(MonoFrameType& data) final override;
	void processStereoFrame(StereoFrameType& data) final override;
	void process(ProcessDataDyn& data) noexcept final override;
	void handleHiseEvent(HiseEvent& e) final override;

    void setBypassed(bool shouldBeBypassed) final override;
    
	WrapperType wrapper;
};



struct InterpretedCableNode : public ModulationSourceNode,
							  public InterpretedNodeBase<OpaqueNode>
{
	using Base = InterpretedNodeBase<OpaqueNode>;

	InterpretedCableNode(DspNetwork* n, ValueTree d) :
		ModulationSourceNode(n, d),
		Base()
	{
		
	};

	template <typename T, typename ComponentType, bool AddDataOffsetToUIPtr, bool Unused> static NodeBase* createNode(DspNetwork* n, ValueTree d)
	{
		constexpr bool isBaseOfDynamicParameterHolder = std::is_base_of<control::pimpl::parameter_node_base<parameter::dynamic_base_holder>, typename T::WrappedObjectType>();

		constexpr bool isBaseOfDynamicDupliHolder = std::is_base_of<control::pimpl::parameter_node_base<parameter::clone_holder>, typename T::WrappedObjectType>();

        constexpr bool isBaseOfNoParameterHolder = std::is_base_of<control::pimpl::no_parameter, typename T::WrappedObjectType>();
        
		static_assert(std::is_base_of<control::pimpl::no_processing, typename T::WrappedObjectType>(), "not a base of no_processing");

		auto mn = new InterpretedCableNode(n, d);

		if constexpr (std::is_same<T, control::dynamic_dupli_pack>())
			mn->getParameterFunction = control::dynamic_dupli_pack::getParameterFunction;
		else if constexpr (isBaseOfDynamicDupliHolder)
			mn->getParameterFunction = parameter::clone_holder::getParameterFunctionStatic;
		else if constexpr (isBaseOfDynamicParameterHolder)
			mn->getParameterFunction = InterpretedCableNode::getParameterFunctionStatic<T>;
        else if constexpr (isBaseOfNoParameterHolder)
            mn->getParameterFunction = nullptr;
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

	String getNodeDescription() const override { return obj.getWrappedObject().getDescription(); }

	ParameterDataList createInternalParameterList() override
	{
		return createInternalParameterListFromWrapper();
	}

	parameter::dynamic_base_holder* getParameterHolder() override;

	bool isProcessingHiseEvent() const override
	{
		return this->obj.isProcessingHiseEvent();
	}

	void process(ProcessDataDyn& data) final override
	{
		ProcessDataPeakChecker fd(this, data);
		this->obj.process(data);
	}

	void reset() final override
	{

	}

	bool isUsingNormalisedRange() const override
	{
		return this->obj.getWrappedObject().isNormalised;
	}

	void prepare(PrepareSpecs ps) final override;

	void processFrame(FrameType& data) final override
	{
		FrameDataPeakChecker fd(this, data.begin(), data.size());
		this->obj.processFrame(*reinterpret_cast<span<float, 1>*>(data.begin()));
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

#define SCRIPTNODE_FACTORY(x, id) static NodeBase* createNode(DspNetwork* n, ValueTree d) { return new x(n, d); }; \
static Identifier getStaticId() { return Identifier(id); };
    
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

	void registerNodeWithLambda(const Identifier& id, const CreateCallback& f)
    {
	    Item newItem;
		newItem.cb = f;
		newItem.id = id;

		monoNodes.add(newItem);
    }

    template <class T> void registerNodeRaw()
    {
        Item newItem;
        newItem.cb = T::createNode;
        newItem.id = T::getStaticId();
            
        monoNodes.add(newItem);
    }
        

    template <class T, 
             class ComponentType=HostHelpers::NoExtraComponent,
				typename WrapperType=InterpretedNode, 
				bool AddDataOffsetToUIPtr=true,
				bool UseNodeBaseAsUI=false> 
		void registerNode()
    {
		Item newItem;
        newItem.cb = WrapperType::template createNode<T, ComponentType, AddDataOffsetToUIPtr, UseNodeBaseAsUI>;
        newItem.id = T::getStaticId();
			
        monoNodes.add(newItem);
    };

    template <class MonoT, 
				class PolyT, 
				class ComponentType=HostHelpers::NoExtraComponent, 
				typename WrapperType=InterpretedNode,
				bool AddDataOffsetToUIPtr=true,
				bool UseFullNodeAsUIPtr=false>
		void registerPolyNode()
    {
		// enable this static_assert(std::is_base_of<PolyT, polyphonic_base>(), "not a base of scriptnode::polyphonic_base");
		// or this
		//
		static_assert(std::is_base_of<polyphonic_base, typename PolyT::WrappedObjectType>(), "not a base of scriptnode::polyphonic_base");

        using WrappedPolyT = InterpretedNode;
        using WrappedMonoT = InterpretedNode;
            
        {
            Item newItem;
            newItem.cb = WrapperType::template createNode<PolyT, ComponentType, AddDataOffsetToUIPtr, UseFullNodeAsUIPtr>;
            newItem.id = PolyT::getStaticId();
                
            polyNodes.add(newItem);
        }
            
        {
            Item newItem;
            newItem.cb = WrapperType::template createNode<MonoT, ComponentType, AddDataOffsetToUIPtr, UseFullNodeAsUIPtr>;
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
		registerNode<MonoT, ComponentType, InterpretedModNode, AddDataOffsetToUIPtr, false>();
	}

	template <class MonoT, class ComponentType = ModulationSourcePlotter, bool AddDataOffsetToUIPtr = true> void registerNoProcessNode()
	{
		registerNode<MonoT, ComponentType, InterpretedCableNode, AddDataOffsetToUIPtr, false>();
	}

	template <class MonoT, class PolyT, class ComponentType = ModulationSourcePlotter, bool AddDataOffsetToUIPtr = true> void registerPolyNoProcessNode()
	{
		
		registerPolyNode<MonoT, PolyT, ComponentType, InterpretedCableNode, AddDataOffsetToUIPtr, false>();
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

struct TemplateNodeFactory : public NodeFactory
{
	struct Builder
	{
		static constexpr int BypassIndex = -1;

		Builder(DspNetwork* n, ValueTree v):
			network(n)
		{
			jassert(!v.getParent().isValid());
			nodes.add(v);
			existingIds.addArray(n->getListOfUnusedNodeIds());
			existingIds.addArray(n->getListOfUsedNodeIds());
		}

		WeakReference<DspNetwork> network;

		StringArray existingIds;

		Array<ValueTree> nodes;

		int addNode(int parent, const String& path, const String& id, int index=-1);
		void addParameter(int nodeIndex, const String& name, InvertableParameterRange r);

		bool connectSendReceive(int sendIndex, Array<int> receiveIndexes);

		bool connect(int nodeIndex, const Identifier sourceType, int sourceIndex, int targetNodeIndex, int targetParameterIndex);

		void setNodeColour(Array<int> nodeIndexes, Colour c);

		void setProperty(Array<int> nodeIndexes, const Identifier& id, const var& value);

		void setNodeProperty(Array<int> nodeIndexes, const NamedValueSet& properties);

		void setParameterValues(Array<int> nodeIndexes, StringArray parameterNames, Array<double> values);

		void setComplexDataIndex(Array<int> nodeIndexes, ExternalData::DataType type, int index);

		void addComment(Array<int> nodeIndexes, const String& comment);

		Colour getRandomColour() const
		{
			return Colour(Random::getSystemRandom().nextFloat(), 0.33f, 0.6f, 1.0f);
		}

		void setFolded(Array<int> nodeIndexes);

		void fillValueTree(int nodeIndex);

		void setRootType(const String& rootPath)
		{
			nodes[0].setProperty(PropertyIds::FactoryPath, rootPath, nullptr);
		}

		NodeBase* flush()
		{
			return network->createFromValueTree(network->isPolyphonic(), nodes[0], true);
		}
	};

	TemplateNodeFactory(DspNetwork* n);;

	virtual Identifier getId() const override { RETURN_STATIC_IDENTIFIER("template"); }
};

    
}
