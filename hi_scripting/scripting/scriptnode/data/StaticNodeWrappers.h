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

template <class HiseDspBaseType> class HiseDspNodeBase : public ModulationSourceNode
{
	using WrapperType = bypass::smoothed<HiseDspBaseType, false>;

public:
	HiseDspNodeBase(DspNetwork* parent, ValueTree d) :
		ModulationSourceNode(parent, d)
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

	NodeComponent* createComponent() override
	{
		auto nc = new DefaultParameterNodeComponent(this);

		if (auto extra = wrapper.getObject().createExtraComponent(getScriptProcessor()->getMainController_()->getGlobalUIUpdater()))
		{
			extra->setSize(0, wrapper.getObject().ExtraHeight);
			nc->setExtraComponent(extra);
		}

		return nc;
	}

	void reset()
	{
		wrapper.reset();
	}

	HiseDspBaseType* getReferenceToInternalObject() 
	{ 
		return dynamic_cast<HiseDspBaseType*>(wrapper.createListOfNodesWithSameId().getLast());
	}

	Identifier getObjectName() const override { return getStaticId(); }

	Rectangle<int> getPositionInCanvas(Point<int> topLeft) const override
	{
		int numParameters = getNumParameters();

		if (numParameters == 0)
			return createRectangleForParameterSliders(0).withPosition(topLeft);
		if (numParameters % 5 == 0)
			return createRectangleForParameterSliders(5).withPosition(topLeft);
		else if (numParameters % 4 == 0)
			return createRectangleForParameterSliders(4).withPosition(topLeft);
		else if (numParameters % 3 == 0)
			return createRectangleForParameterSliders(3).withPosition(topLeft);
		else if (numParameters % 2 == 0)
			return createRectangleForParameterSliders(2).withPosition(topLeft);
		else if (numParameters == 1)
			return createRectangleForParameterSliders(1).withPosition(topLeft);

		return {};
	}

	Rectangle<int> createRectangleForParameterSliders(int numColumns) const
	{
		int h = UIValues::HeaderHeight;
		h += wrapper.getObject().ExtraHeight;

		int w = 0;

		if (numColumns == 0)
			w = UIValues::NodeWidth * 2;
		else
		{
			int numParameters = getNumParameters();
			int numRows = std::ceil((float)numParameters / (float)numColumns);

			h += numRows * (48 + 18);
			w = jmin(numColumns * 100, numParameters * 100);
		}

		w = jmax(w, wrapper.getObject().getExtraWidth());

		auto b = Rectangle<int>(0, 0, w, h);
		return b.expanded(UIValues::NodeMargin);
	}

	void prepare(double sampleRate, int blockSize) final override
	{
		ModulationSourceNode::prepare(sampleRate, blockSize);

		wrapper.prepare(getNumChannelsToProcess(), sampleRate, blockSize);
	}

	void processSingle(float* frameData, int numChannels) final override
	{
		wrapper.processSingle(frameData, numChannels);

		if (wrapper.allowsModulation())
		{
			ProcessData d;
			d.data = &frameData;
			d.size = numChannels;
			d.numChannels = 1;

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

	HardcodedNode* getAsHardcodedNode() override
	{
		return wrapper.getObject().getAsHardcodedNode();
	}

	WrapperType wrapper;

	valuetree::PropertyListener bypassListener;
};

struct HardcodedNode
{
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

	void setParameterDefault(const String& parameterId, double value)
	{
		for (auto& parameter : internalParameterList)
		{
			if (parameter.id == parameterId)
			{
				parameter.db(value);
				break;
			}
		}
	}

	Array<HiseDspBase::ParameterData> internalParameterList;
	Array<ParameterInitValue> initValues;

	HiseDspBase::ParameterData getParameter(const String& id);

	HiseDspBase::ParameterData getParameter(const String& id, NormalisableRange<double> d);

	String getNodeId(const HiseDspBase* internalNode) const
	{
		for (const auto& n : internalNodes)
		{
			for (const auto& in_ : n.nodes)
			{
				if (in_ == internalNode)
					return n.id;
			}
		}

		return {};
	}

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


template <class SubType, class DspProcessorType, int ModulationType> struct hardcoded : public HiseDspBase,
public HardcodedNode
{
public:

	GET_SELF_AS_OBJECT(SubType);

	static constexpr bool isModulationSource = (ModulationType == hc::with_modulation);

	void initialise(NodeBase* n) override
	{
		obj.initialise(n);
	}

	void prepare(int numChannels, double sampleRate, int blockSize)
	{
		obj.prepare(numChannels, sampleRate, blockSize);
	}

	bool handleModulation(double& value)
	{
		return obj.handleModulation(value);
	}

	void process(ProcessData& data) noexcept
	{
		obj.process(data);
	}

	void reset()
	{
		obj.reset();
	}

	void processSingle(float* frameData, int numChannels) noexcept
	{
		obj.processSingle(frameData, numChannels);
	}

	HardcodedNode* getAsHardcodedNode() override 
	{ 
		return this; 
	}

	template <int Index1, int Index2, int Index3, int Index4, int Index5, int Index6, class T> auto get(T& t)
	{
		return get<Index6>(*get<Index5>(*get<Index4>(*get<Index3>(*get<Index2>(*get<Index1>(t))))));
	}

	template <int Index1, int Index2, int Index3, int Index4, int Index5, class T> auto get(T& t)
	{
		return get<Index5>(*get<Index4>(*get<Index3>(*get<Index2>(*get<Index1>(t)))));
	}

	template <int Index1, int Index2, int Index3, int Index4, class T> auto get(T& t)
	{
		return get<Index4>(*get<Index3>(*get<Index2>(*get<Index1>(t))));
	}

	template <int Index1, int Index2, int Index3, class T> auto get(T& t)
	{
		return get<Index3>(*get<Index2>(*get<Index1>(t)));
	}

	template <int Index1, int Index2, class T> constexpr auto get(T& t)
	{
		return get<Index2>(*get<Index1>(t));
	}

	template <int Index, class T> auto get(T& t)
	{
		return &t.getObject().get<Index>().getObject();
	}





	DspProcessorType obj;
};

template <typename SubType, typename ProcessorType> using no_mod = hardcoded<SubType, ProcessorType, hc::no_modulation>;

}
