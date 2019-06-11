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

	SCRIPTNODE_FACTORY(HiseDspNodeBase, HiseDspBaseType::getStaticId());

	Component* createExtraComponent()
	{
		return wrapper.getObject().createExtraComponent(getScriptProcessor()->getMainController_()->getGlobalUIUpdater());
	}

	int getExtraHeight() const override
	{
		return wrapper.getObject().ExtraHeight;
	}

	int getExtraWidth() const override
	{
		return wrapper.getObject().getExtraWidth();
	}

	void reset()
	{
		wrapper.reset();
	}

	bool isPolyphonic() const override { return wrapper.isPolyphonic(); }

	HiseDspBaseType* getReferenceToInternalObject() 
	{ 
		return dynamic_cast<HiseDspBaseType*>(wrapper.createListOfNodesWithSameId().getLast());
	}

	Identifier getObjectName() const override { return getStaticId(); }

	void prepare(PrepareSpecs specs) final override
	{
		ModulationSourceNode::prepare(specs);
		wrapper.prepare(specs);
	}

	void processSingle(float* frameData, int numChannels) final override
	{
		wrapper.processSingle(frameData, numChannels);

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
				sendValueToTargets(value, data.size);
		}
	}

	NamedValueSet getDefaultProperties() const
	{
		return wrapper.getObject().getDefaultProperties();
	}

	void handleHiseEvent(HiseEvent& e) final override
	{
		wrapper.handleHiseEvent(e);
	}

	HardcodedNode* getAsHardcodedNode() override
	{
		return wrapper.getObject().getAsHardcodedNode();
	}

	WrapperType wrapper;

	valuetree::PropertyListener bypassListener;
};


class HardcodedNode
{
public:

	struct ParameterInitValue
	{
		ParameterInitValue(const char* id_, double v) :
			id(id_),
			value(v)
		{};

		String id;
		double value;
	};

	static constexpr int ExtraHeight = 0;

	template <class T> static Array<HiseDspBase::ParameterData> createParametersT(T* d, const String& prefix)
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

	template <class T> void fillInternalParameterList(T* obj, const String& name)
	{
		internalParameterList.addArray(createParametersT(obj, name));
	}

	template <class T> void setInternalModulationParameter(T* modObject, const DspHelpers::ParameterCallback& f)
	{
		modObject->setCallback(f);
	}

	void setParameterDefault(const String& parameterId, double value);

	Array<HiseDspBase::ParameterData> internalParameterList;
	Array<ParameterInitValue> initValues;

	HiseDspBase::ParameterData getParameter(const String& id);

	HiseDspBase::ParameterData getParameter(const String& id, NormalisableRange<double> d);

	virtual String getSnippetText() const { return ""; }

	String getNodeId(const HiseDspBase* internalNode) const;

	HiseDspBase* getNode(const String& id) const
	{
		for (const auto& n : internalNodes)
		{
			if (n.id == id)
				return n.nodes.getFirst();
		}

		return nullptr;
	}

	template <class T> void registerNode(T* obj, const String& id)
	{
		if (auto typed = dynamic_cast<HiseDspBase*>(&obj->getObject()))
		{
			internalNodes.add({ typed->createListOfNodesWithSameId(), id });
			fillInternalParameterList(typed, id);
		}
	}

private:

	struct InternalNode
	{
		Array<HiseDspBase*> nodes;
		String id;
	};

	Array<InternalNode> internalNodes;

	StringArray registeredIds;
};

struct hc
{
	static constexpr int no_modulation = 1;
	static constexpr int with_modulation = 2;
};


template <class DspProcessorType> struct hardcoded : public HiseDspBase,
public HardcodedNode
{
public:

	void initialise(NodeBase* n) override
	{
		obj.initialise(n);

		nodeData = n->getValueTree();
		um = n->getUndoManager();
	}

	void prepare(PrepareSpecs ps)
	{
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

	void process(ProcessData& data) noexcept
	{
		obj.process(data);
	}

	void reset()
	{
		obj.reset();
	}

	void setNodeProperty(const String& id, const var& newValue, bool isPublic)
	{
		auto propTree = nodeData.getChildWithName(PropertyIds::Properties).getChildWithProperty(PropertyIds::ID, id);

		if (propTree.isValid())
		{
			propTree.setProperty(PropertyIds::Value, newValue, um);
			propTree.setProperty(PropertyIds::Public, isPublic, um);
		}
	}

	void processSingle(float* frameData, int numChannels) noexcept
	{
		obj.processSingle(frameData, numChannels);
	}

	HardcodedNode* getAsHardcodedNode() override 
	{ 
		return this; 
	}

	template <int Index1, int Index2, int Index3, int Index4, int Index5, int Index6, class T> auto* get(T& t)
	{
		return get<Index6>(*get<Index5>(*get<Index4>(*get<Index3>(*get<Index2>(*get<Index1>(t))))));
	}

	template <int Index1, int Index2, int Index3, int Index4, int Index5, class T> auto* get(T& t)
	{
		return get<Index5>(*get<Index4>(*get<Index3>(*get<Index2>(*get<Index1>(t)))));
	}

	template <int Index1, int Index2, int Index3, int Index4, class T> auto* get(T& t)
	{
		return get<Index4>(*get<Index3>(*get<Index2>(*get<Index1>(t))));
	}

	template <int Index1, int Index2, int Index3, class T> auto* get(T& t)
	{
		return get<Index3>(*get<Index2>(*get<Index1>(t)));
	}

	template <int Index1, int Index2, class T> auto* get(T& t)
	{
		return get<Index2>(*get<Index1>(t));
	}

	template <int Index, class T> static auto* get(T& t)
	{
        auto* obj1 = &t.getObject();
        auto* obj2 = &obj1->template get<Index>();
        return &obj2->getObject();
	}

	ValueTree nodeData;
	UndoManager* um = nullptr;

	DspProcessorType obj;
};


}
