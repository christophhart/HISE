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
using namespace juce;
using namespace hise;

struct ComponentHelpers
{
    static NodeComponent* createDefaultComponent(NodeBase* n);
    static void addExtraComponentToDefault(NodeComponent* nc, Component* c);
        
};


class WrapperNode : public ModulationSourceNode
{
protected:

	WrapperNode(DspNetwork* parent, ValueTree d);;

	NodeComponent* createComponent() override;

	virtual Component* createExtraComponent() { return nullptr; }

	Rectangle<int> getPositionInCanvas(Point<int> topLeft) const override;

	Rectangle<int> createRectangleForParameterSliders(int numColumns) const;

	virtual int getExtraWidth() const { return 0; }
	virtual int getExtraHeight() const { return 0; }
};

    
template <class HiseDspBaseType> class HiseDspNodeBase : public WrapperNode
{
	using WrapperType = bypass::smoothed<HiseDspBaseType, false>;

public:
	HiseDspNodeBase(DspNetwork* parent, ValueTree d) :
		WrapperNode(parent, d)
	{
		wrapper.getObject().initialise(this);

		setDefaultValue(PropertyIds::BypassRampTimeMs, 20.0f);

		d.getOrCreateChildWithName(PropertyIds::Parameters, getUndoManager());

		Array<HiseDspBase::ParameterData> pData;

		wrapper.getObject().createParameters(pData);

		for (auto p : pData)
		{
			auto existingChild = getParameterTree().getChildWithProperty(PropertyIds::ID, p.id);

			if (!existingChild.isValid())
			{
				existingChild = p.createValueTree();
				getParameterTree().addChild(existingChild, -1, getUndoManager());
			}

			auto newP = new Parameter(this, existingChild);
			newP->setCallback(p.db);
			newP->valueNames = p.parameterNames;

			addParameter(newP);
		}

		bypassListener.setCallback(d, { PropertyIds::Bypassed },
			valuetree::AsyncMode::Synchronously,
			std::bind(&WrapperType::setBypassedFromValueTreeCallback,
				&wrapper, std::placeholders::_1, std::placeholders::_2));
	};

    static NodeBase* createNode(DspNetwork* n, ValueTree d) { return new HiseDspNodeBase<HiseDspBaseType>(n, d); }; 
    static Identifier getStaticId() { return HiseDspBaseType::getStaticId(); };
    
	Component* createExtraComponent()
	{
		return wrapper.getObject().createExtraComponent(getScriptProcessor()->getMainController_()->getGlobalUIUpdater());
	}

	int getExtraHeight() const final override
	{
		return wrapper.getObject().getExtraHeight();
	}

	int getExtraWidth() const final override
	{
		return wrapper.getObject().getExtraWidth();
	}

	bool isUsingModulation() const override
	{
		return HiseDspBaseType::isModulationSource;
	}

	void reset()
	{
		wrapper.reset();

		if (HiseDspBaseType::isModulationSource)
		{
			double initValue = 0.0;
			if (getRootNetwork()->isCurrentlyRenderingVoice() && wrapper.getObject().handleModulation(initValue))
				sendValueToTargets(initValue, 0);
		}
	}

	HiseDspBase* getInternalT()
	{
		return wrapper.getInternalT();
	}

	bool isPolyphonic() const override { return wrapper.isPolyphonic(); }

	void postInit() override
    {
        if(wrapper.getInternalT()->needsReinitialisation())
            wrapper.initialise(this);
    }

	void prepare(PrepareSpecs specs) final override
	{
		ModulationSourceNode::prepare(specs);
		wrapper.prepare(specs);
	}

	void processFrame(dyn<float>& data) final override
	{
		wrapper.processFrame(data);

		if (wrapper.allowsModulation())
		{
			double value = 0.0;
			if (wrapper.handleModulation(value))
				sendValueToTargets(value, 1);
		}
	}

	void process(ProcessData& data) noexcept final override
	{
		wrapper.process(data);

		if (wrapper.allowsModulation())
		{
			double value = 0.0;

			if (wrapper.handleModulation(value))
				sendValueToTargets(value, data.getNumSamples());
		}
	}

	NamedValueSet getDefaultProperties() const
	{
		return wrapper.getObject().getDefaultProperties();
	}

	void handleHiseEvent(HiseEvent& e) final override
	{
		wrapper.handleHiseEvent(e);

		if (wrapper.allowsModulation())
		{
			double value = 0.0;
			if (wrapper.handleModulation(value))
				sendValueToTargets(value, 0);
		}
	}

	RestorableNode* getAsRestorableNode() override
	{
		if (auto hc = getAsHardcodedNode())
		{
			return hc;
		}

		auto rn = dynamic_cast<RestorableNode*>(wrapper.getInternalT());

		return rn;
	}

	HardcodedNode* getAsHardcodedNode() override
	{
		return wrapper.getObject().getAsHardcodedNode();
	}

	WrapperType wrapper;
	valuetree::PropertyListener bypassListener;
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
			callback(p.db),
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

	static Array<HiseDspBase::ParameterData> createParametersT(ParameterHolder* d, const String& prefix)
	{
		Array<HiseDspBase::ParameterData> data;
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

	Array<HiseDspBase::ParameterData> internalParameterList;
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
			if (auto hc = typed->getAsHardcodedNode())
			{
				hc->fillInternalParameterList(typed, id);
				internalNodes.addArray(hc->internalNodes);
			}

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

	HardcodedNode* getAsHardcodedNode() override
	{
		return this;
	}

	int getExtraWidth() const override
	{
		if (extraWidth != -1)
			return extraWidth;

		return HiseDspBase::getExtraWidth();
	}

	int getExtraHeight() const override
	{
		if (extraHeight != -1)
			return extraHeight;

		return HiseDspBase::getExtraHeight();
	}

	Component* createExtraComponent(PooledUIUpdater* updater) override
	{
		extraHeight = -1;
		extraWidth = -1;
		StringArray sa = getNodeIdsWithPublicComponent();

		if (sa.isEmpty())
			return nullptr;
		else
		{
			auto c = new HardcodedNodeComponent(this, sa, updater);
			extraHeight = c->getHeight();
			extraWidth = c->getWidth();
			return c;
		}
			
	}

	int extraHeight = -1;
	int extraWidth = -1;
};
#endif

template <class Initialiser, class T, class PropertyClass=properties::none> struct cpp_node : public SingleWrapper<T>
{
	static constexpr bool isModulationSource = T::isModulationSource;
	static constexpr int NumChannels = T::NumChannels;

	

	static Identifier getStaticId() { return Initialiser::getStaticId(); };

	using FixBlockType = snex::Types::ProcessDataFix<NumChannels>;
	using FrameType = snex::Types::span<float, NumChannels>;

	void initialise(NodeBase* n) override
	{
		Initialiser init;

		init.initialise(obj);
		obj.initialise(n);
		props.initWithRoot(n, this, obj);

	}

	template <int P> static void setParameter(void* ptr, double v)
	{
		static_cast<cpp_node*>(ptr)->obj.setParameter<P>(v);
	}

	template <int I, int Limit> struct ParameterAdder
	{
		struct Below
		{
			static constexpr int value = I < Limit;
		};

		ParameterAdder(cpp_node& p, Array<ParameterData>& d)
		{
			add_(p, d, std::integral_constant<bool, Below::value>{});
		}

		void add_(cpp_node& obj, Array<ParameterData>& d, std::true_type)
		{
			obj.addParameter<I>(d);
		}

		void add_(cpp_node& obj, Array<ParameterData>& d, std::false_type)
		{

		}
	};

	void process(FixBlockType& d)
	{
		obj.process(d);
	}

	void process(ProcessData& data) noexcept
	{
		jassert(data.getNumChannels() == NumChannels);
		auto& fd = data.as<FixBlockType>();
		obj.process(fd);
	}

	template <typename FrameDataType> void processFrame(FrameDataType& data) noexcept
	{
		auto& fd = FrameType::as(data.begin());
		obj.processFrame(fd);
	}

	constexpr bool allowsModulation()
	{
		return obj.isModulationSource;
	}

	void prepare(PrepareSpecs ps)
	{
		obj.prepare(ps);
	}

	forcedinline void reset() noexcept { obj.reset(); }

	forcedinline bool handleModulation(double& value) noexcept
	{
		return obj.handleModulation(value);
	}

	constexpr auto& getObject() { return *this; }
	constexpr const auto& getObject() const { return *this; }

	void createParameters(Array<ParameterData>& data) override
	{
		static constexpr int NumParameters = obj.parameters.size;

#define ADD_PARAMETER(x) ParameterAdder<x, NumParameters>(*this, data)

		ADD_PARAMETER(0); ADD_PARAMETER(1); ADD_PARAMETER(2); ADD_PARAMETER(3);
		ADD_PARAMETER(4); ADD_PARAMETER(5); ADD_PARAMETER(6); ADD_PARAMETER(7);

		ADD_PARAMETER(8); ADD_PARAMETER(9); ADD_PARAMETER(10); ADD_PARAMETER(11);
		ADD_PARAMETER(12); ADD_PARAMETER(13); ADD_PARAMETER(14); ADD_PARAMETER(15);

#undef ADD_PARAMETER

		props.initWithRoot(nullptr, nullptr, obj);
	}

	template <int P> void addParameter(Array<ParameterData>& data)
	{
		String pName = "Parameter";

		pName << String(P);


		ParameterData p(pName, obj.parameters.createParameterRange<P>());
		p.db = [this](double v)
		{
			obj.setParameter<P>(v);
		};

		data.add(p);
	}

	PropertyClass props;
};

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
    
    class NodeFactory
    {
    public:
        
        NodeFactory(DspNetwork* n) :
        network(n)
        {};
        
        virtual ~NodeFactory() {};
        
        using CreateCallback = std::function<NodeBase*(DspNetwork*, ValueTree)>;
        using PostCreateCallback = std::function<void(NodeBase* n)>;
        using IdFunction = std::function<Identifier()>;
        
        StringArray getModuleList() const
        {
            StringArray sa;
            String prefix = getId().toString() + ".";
            
            for (const auto& item : monoNodes)
            {
                sa.add(prefix + item.id().toString());
            }
            
            return sa;
        }
        
		template <class MonoT, class PolyT> void registerPolyNodeRaw()
		{
			{
				Item newItem;
				newItem.cb = PolyT::createNode;
				newItem.id = PolyT::getStaticId;

				polyNodes.add(newItem);
			}

			{
				Item newItem;
				newItem.cb = MonoT::createNode;
				newItem.id = MonoT::getStaticId;

				monoNodes.add(newItem);
			}
		}

		template <class T> void registerAnonymousNode(const Identifier& id)
		{
			using WrappedT = HiseDspNodeBase<T>;

			Item newItem;
			newItem.cb = WrappedT::createNode;
			newItem.id = [id]() { return id; };
			
			monoNodes.add(newItem);
		}

        template <class T> void registerNodeRaw(const PostCreateCallback& cb = {})
        {
            Item newItem;
            newItem.cb = T::createNode;
            newItem.id = T::getStaticId;
            newItem.pb = cb;
            
            monoNodes.add(newItem);
        }
        
        template <class T> void registerNode(const PostCreateCallback& cb = {})
        {
            using WrappedT = HiseDspNodeBase<T>;
            
            Item newItem;
            newItem.cb = WrappedT::createNode;
            newItem.id = WrappedT::getStaticId;
            newItem.pb = cb;
            
            monoNodes.add(newItem);
        };
        
        template <class MonoT, class PolyT> void registerPolyNode(const PostCreateCallback& cb = {})
        {
            using WrappedPolyT = HiseDspNodeBase<PolyT>;
            using WrappedMonoT = HiseDspNodeBase<MonoT>;
            
            {
                Item newItem;
                newItem.cb = WrappedPolyT::createNode;
                newItem.id = WrappedPolyT::getStaticId;
                newItem.pb = cb;
                
                polyNodes.add(newItem);
            }
            
            {
                Item newItem;
                newItem.cb = WrappedMonoT::createNode;
                newItem.id = WrappedMonoT::getStaticId;
                newItem.pb = cb;
                
                monoNodes.add(newItem);
            }
        };
        
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
					return first.id().toString().compareNatural(second.id().toString());
				}
			} sorter;

			monoNodes.sort(sorter);
			polyNodes.sort(sorter);
		}

    private:
        
        struct Item
        {
            CreateCallback cb;
            IdFunction id;
            PostCreateCallback pb;
        };
        
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
