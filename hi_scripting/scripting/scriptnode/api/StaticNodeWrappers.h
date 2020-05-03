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

struct NoExtraComponent
{
	using ObjectType = HiseDspBase;

	static Component* createExtraComponent(ObjectType* , PooledUIUpdater*) { return nullptr; }
};

    
template <class HiseDspBaseType, class ComponentType=NoExtraComponent> class HiseDspNodeBase : public WrapperNode
{
	using WrapperType = bypass::smoothed<HiseDspBaseType, false>;

public:
	HiseDspNodeBase(DspNetwork* parent, ValueTree d) :
		WrapperNode(parent, d)
	{
		wrapper.getObject().initialise(this);

		setDefaultValue(PropertyIds::BypassRampTimeMs, 20.0f);

		Array<HiseDspBase::ParameterData> pData;
		wrapper.getWrappedObject().createParameters(pData);

		initParameterData(pData);
	};


	Array<ParameterDataImpl> createInternalParameterList() override
	{
		Array<ParameterDataImpl> pList;
		wrapper.getWrappedObject().createParameters(pList);
		return pList;
	}

    static NodeBase* createNode(DspNetwork* n, ValueTree d) { return new HiseDspNodeBase<HiseDspBaseType, ComponentType>(n, d); }; 
	static Identifier getStaticId() { return HiseDspBaseType::getStaticId(); };

	Component* createExtraComponent()
	{
		auto obj = static_cast<ComponentType::ObjectType*>(&wrapper.getWrappedObject());
		auto updater = getScriptProcessor()->getMainController_()->getGlobalUIUpdater();
		auto c =  ComponentType::createExtraComponent(obj, updater);
		
		return c;
	}

	GET_SELF_OBJECT(wrapper);
	GET_WRAPPED_OBJECT(wrapper.getWrappedObject());

	void reset()
	{
		wrapper.reset();
	}

	bool isPolyphonic() const override { return wrapper.isPolyphonic(); }

	void prepare(PrepareSpecs specs) final override
	{
		auto& exceptionHandler = getRootNetwork()->getExceptionHandler();

		exceptionHandler.removeError(this);

		try
		{
			wrapper.prepare(specs);
		}
		catch (Error& s)
		{
			exceptionHandler.addError(this, s);
		}

		NodeBase::prepare(specs);
	}

	void processFrame(NodeBase::FrameType& data) final override
	{
		if (data.size() == 1)
			processMonoFrame(MonoFrameType::as(data.begin()));
		if (data.size() == 2)
			processStereoFrame(StereoFrameType::as(data.begin()));
	}

	void processMonoFrame(MonoFrameType& data) final override
	{
		wrapper.processFrame(data);
	}

	void processStereoFrame(StereoFrameType& data) final override
	{
		wrapper.processFrame(data);
	}

	void process(ProcessData& data) noexcept final override
	{
		wrapper.process(data);
	}


	void setBypassed(bool shouldBeBypassed) final override
	{
		WrapperNode::setBypassed(shouldBeBypassed);
		wrapper.setBypassed(shouldBeBypassed);
	}

	void handleHiseEvent(HiseEvent& e) final override
	{
		wrapper.handleHiseEvent(e);
	}

	WrapperType wrapper;
	valuetree::PropertyListener bypassListener;
};


template <class HiseDspBaseType, class ComponentType = ModulationSourcePlotter> class HiseDspNodeBaseWithModulation : public ModulationSourceNode
{
	using WrapperType = wrap::mod<HiseDspBaseType, parameter::dynamic_base_holder>;

public:
	HiseDspNodeBaseWithModulation(DspNetwork* parent, ValueTree d) :
		ModulationSourceNode(parent, d)
	{
		wrapper.getObject().initialise(this);
		wrapper.p.setRingBuffer(ringBuffer.get());

		stop();

		setDefaultValue(PropertyIds::BypassRampTimeMs, 20.0f);
		
		Array<HiseDspBase::ParameterData> pData;
		wrapper.getWrappedObject().createParameters(pData);
		initParameterData(pData);
	};

	static NodeBase* createNode(DspNetwork* n, ValueTree d) { return new HiseDspNodeBaseWithModulation<HiseDspBaseType, ComponentType>(n, d); };
	static Identifier getStaticId() { return HiseDspBaseType::getStaticId(); };

	Component* createExtraComponent()
	{
		auto obj = static_cast<ComponentType::ObjectType*>(&wrapper.getWrappedObject());
		auto updater = getScriptProcessor()->getMainController_()->getGlobalUIUpdater();

		auto c = ComponentType::createExtraComponent(obj, updater);

		c->getProperties().set("circleOffsetX", -10.0f);

		return c;
	}

	void timerCallback() override
	{
		wrapper.p.updateUI();
	}

	bool isUsingNormalisedRange() const override
	{
		return wrapper.getWrappedObject().isNormalisedModulation();
	}

	parameter::dynamic_base_holder* getParameterHolder() override
	{
		return &wrapper.p;
	}
	
	

	GET_WRAPPED_OBJECT(wrapper.getWrappedObject());

	void reset()
	{
		wrapper.reset();
	}

	bool isPolyphonic() const override { return wrapper.isPolyphonic(); }

	void prepare(PrepareSpecs specs) final override
	{
		auto& exceptionHandler = getRootNetwork()->getExceptionHandler();

		exceptionHandler.removeError(this);

		try
		{
			ModulationSourceNode::prepare(specs);
			wrapper.prepare(specs);
		}
		catch (Error& s)
		{
			exceptionHandler.addError(this, s);
		}

		ModulationSourceNode::prepare(specs);
	}

	void writeToRingBuffer(double value, int numSamplesForAnalysis)
	{
		if (ringBuffer != nullptr &&
			numSamplesForAnalysis > 0 &&
			getRootNetwork()->isRenderingFirstVoice())
		{
			ringBuffer->write(value, (int)(jmax(1.0, sampleRateFactor * (double)numSamplesForAnalysis)));
		}
	}

	void processFrame(NodeBase::FrameType& data) final override
	{
		if (data.size() == 1)
			processMonoFrame(MonoFrameType::as(data.begin()));
		if (data.size() == 2)
			processStereoFrame(StereoFrameType::as(data.begin()));
	}

	void processMonoFrame(MonoFrameType& data) final override
	{
		wrapper.p.setSamplesToWrite(1);
		wrapper.processFrame(data);
	}

	void processStereoFrame(StereoFrameType& data) final override
	{
		wrapper.p.setSamplesToWrite(1);
		wrapper.processFrame(data);
	}

	void process(ProcessData& data) noexcept final override
	{
		wrapper.p.setSamplesToWrite(data.getNumSamples());
		wrapper.process(data);
	}

	NamedValueSet getDefaultProperties() const
	{
		return wrapper.getObject().getDefaultProperties();
	}

	void handleHiseEvent(HiseEvent& e) final override
	{
		wrapper.handleHiseEvent(e);
	}

	//parameter::data_pool parameterDataPool;

	WrapperType wrapper;

};


struct combined_parameter_base
{
	struct Data
	{
		double getPmaValue() const { return value * mulValue + addValue; }

		double getPamValue() const { return (value + addValue) * mulValue; }

		double value = 0.0;
		double mulValue = 1.0;
		double addValue = 0.0;
	} data;

	NormalisableRange<double> currentRange;

	JUCE_DECLARE_WEAK_REFERENCEABLE(combined_parameter_base);
};

struct CombinedParameterDisplay : public ModulationSourceBaseComponent
{
	CombinedParameterDisplay(combined_parameter_base* b, PooledUIUpdater* u) :
		ModulationSourceBaseComponent(u),
		obj(b)
	{
		setSize(140, 80);
	};

	void timerCallback() override
	{
		repaint();
	}

	void paint(Graphics& g) override;

	WeakReference<combined_parameter_base> obj;
};

template <class T> struct ParameterMultiplyAddNode : public ModulationSourceNode
{
	ParameterMultiplyAddNode(DspNetwork* n, ValueTree d) :
		ModulationSourceNode(n, d)
	{
		Array<ParameterDataImpl> pData;
		obj.createParameters(pData);

		initParameterData(pData);

		valueRangeUpdater.setCallback(getModulationTargetTree(), valuetree::AsyncMode::Asynchronously, [this](ValueTree v, bool wasAdded)
		{
			auto firstChild = getModulationTargetTree().getChild(0);

			if (!firstChild.isValid())
			{
				NormalisableRange<double> defaultRange(0.0, 1.0);
				auto thisValue = getParameter("Value")->data;
				obj.currentRange = defaultRange;
				RangeHelpers::storeDoubleRange(thisValue, false, defaultRange, getUndoManager());
			}
			else if (auto p = getParameterData(firstChild))
			{
				auto thisValue = getParameter("Value")->data;
				RangeHelpers::storeDoubleRange(thisValue, false, p.range, getUndoManager());
				obj.currentRange = p.range;

				auto v = obj.data.getPmaValue();
				getParameterHolder()->call(v);
			}
		});
	};

	void timerCallback() override
	{
		obj.p.updateUI();
	}

	bool isUsingNormalisedRange() const override
	{
		return false;
	}

	parameter::dynamic_base_holder* getParameterHolder() override
	{
		return &obj.p;
	}

	

	static NodeBase* createNode(DspNetwork* n, ValueTree d) { return new ParameterMultiplyAddNode<T>(n, d); };
	static Identifier getStaticId() { return T::getStaticId(); };

	Component* createExtraComponent()
	{
		auto b = dynamic_cast<combined_parameter_base*>(&obj.getWrappedObject());
		auto u = getScriptProcessor()->getMainController_()->getGlobalUIUpdater();
		auto cp =  new CombinedParameterDisplay(b, u);
		cp->getProperties().set("circleOffsetY", -40.0f);
		return cp;
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

	void process(ProcessData& data) noexcept final override
	{
	}

	Array<ParameterDataImpl> createInternalParameterList() override
	{  
		Array<ParameterDataImpl> pData;
		obj.createParameters(pData); 
		return pData; 
	}

	void handleHiseEvent(HiseEvent& e) final override
	{}

	T obj;

	valuetree::ChildListener valueRangeUpdater;
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

template <class Initialiser, class T, class PropertyClass=properties::none> struct cpp_node: public HiseDspBase
{
	static constexpr bool isModulationSource = T::isModulationSource;
	static constexpr int NumChannels = T::NumChannels;

	// We treat everything in this node as opaque...
	GET_SELF_AS_OBJECT();

	static Identifier getStaticId() { return Initialiser::getStaticId(); };

	using FixBlockType = snex::Types::ProcessDataFix<NumChannels>;
	using FrameType = snex::Types::span<float, NumChannels>;

	void initialise(NodeBase* n)
	{
		Initialiser init;

		init.initialise(obj);
		obj.initialise(n);
		props.initWithRoot(n, this, obj);
	}

	template <int P> static void setParameter(void* ptr, double v)
	{
		auto* objPtr = &static_cast<cpp_node*>(ptr)->obj;
		T::setParameter<P>(objPtr, v);
	}

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

	void prepare(PrepareSpecs ps)
	{
		obj.prepare(ps);
	}

	void handleHiseEvent(HiseEvent& e)
	{
		obj.handleHiseEvent(e);
	}

	bool isPolyphonic() const
	{
		return false;
	}

	void reset() noexcept { obj.reset(); }

	bool handleModulation(double& value) noexcept
	{
		return obj.handleModulation(value);
	}

	void createParameters(Array<ParameterDataImpl>& data)
	{
		obj.parameters.addToList(data);
		props.initWithRoot(nullptr, nullptr, obj);
	}

	T obj;
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
        
		template <class T, class ComponentType=ModulationSourcePlotter> void registerModNode(const PostCreateCallback& cb = {})
		{
			using WrappedT = HiseDspNodeBaseWithModulation<T, ComponentType>;

			Item newItem;
			newItem.cb = WrappedT::createNode;
			newItem.id = WrappedT::getStaticId;
			newItem.pb = cb;

			monoNodes.add(newItem);
		};

		template <class MonoT, class PolyT, class ComponentType = NoExtraComponent> void registerPolyModNode()
		{
			using WrappedPolyT = HiseDspNodeBaseWithModulation<PolyT, ComponentType>;
			using WrappedMonoT = HiseDspNodeBaseWithModulation<MonoT, ComponentType>;

			{
				Item newItem;
				newItem.cb = WrappedPolyT::createNode;
				newItem.id = WrappedPolyT::getStaticId;
				polyNodes.add(newItem);
			}

			{
				Item newItem;
				newItem.cb = WrappedMonoT::createNode;
				newItem.id = WrappedMonoT::getStaticId;
				monoNodes.add(newItem);
			}
		};

        template <class T, class ComponentType=NoExtraComponent> void registerNode(const PostCreateCallback& cb = {})
        {
            using WrappedT = HiseDspNodeBase<T, ComponentType>;
            
            Item newItem;
            newItem.cb = WrappedT::createNode;
            newItem.id = WrappedT::getStaticId;
            newItem.pb = cb;
            
            monoNodes.add(newItem);
        };
        
        template <class MonoT, class PolyT, class ComponentType=NoExtraComponent> void registerPolyNode(const PostCreateCallback& cb = {})
        {
            using WrappedPolyT = HiseDspNodeBase<PolyT, ComponentType>;
            using WrappedMonoT = HiseDspNodeBase<MonoT, ComponentType>;
            
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

    protected:
        
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
