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


namespace feedback
{
	class SourceNode : public NodeBase
	{
	public:

		SourceNode(DspNetwork* rootNetwork, ValueTree data) :
			NodeBase(rootNetwork, data, 0)
		{}

		SCRIPTNODE_FACTORY(SourceNode, "output");
		
		Identifier getObjectName() const override { return getStaticId(); }

		void prepare(double sampleRate, int blockSize) override
		{
			if (buffer.getNumChannels() != getNumChannelsToProcess() ||
				buffer.getNumSamples() < blockSize)
			{
				buffer.setSize(getNumChannelsToProcess(), blockSize);
			}

			memset(singleFrameData, 0, sizeof(float) * NUM_MAX_CHANNELS);
		}

		void process(ProcessData& data) override
		{
			if (connectedOK)
			{
				for (int i = 0; i < data.numChannels; i++)
					FloatVectorOperations::copy(data.data[i], buffer.getReadPointer(i), data.size);
			}
		}

		void processSingle(float* frameData, int numChannels) final override
		{
			FloatVectorOperations::copy(frameData, singleFrameData, numChannels);
		}

		float singleFrameData[NUM_MAX_CHANNELS];

		AudioSampleBuffer buffer;
		bool connectedOK = false;

		JUCE_DECLARE_WEAK_REFERENCEABLE(SourceNode);
	};

	class TargetNode : public NodeBase
	{
	public:


		TargetNode(DspNetwork* parent, ValueTree data) :
			NodeBase(parent, data, 0)
		{
			connectionListener.setCallback(data, { PropertyIds::Connection },
				valuetree::AsyncMode::Synchronously,
				BIND_MEMBER_FUNCTION_2(TargetNode::updateConnection));
		};

		SCRIPTNODE_FACTORY(TargetNode, "input");

		Identifier getObjectName() const override { return getStaticId(); }

		void updateConnection(Identifier, var newValue)
		{
			if (auto n = dynamic_cast<NodeBase*>(getRootNetwork()->get(newValue).getObject()))
			{
				if (connectedSource != nullptr)
					connectedSource->connectedOK = false;

				if (connectedSource = dynamic_cast<SourceNode*>(n))
					connectedSource->connectedOK = true;
			}
		}

		~TargetNode()
		{
			if (connectedSource != nullptr)
				connectedSource->connectedOK = false;
		}

		void prepare(double sampleRate, int blockSize) override
		{
			
		}

		NodeComponent* createComponent();

		Rectangle<int> getPositionInCanvas(Point<int> topLeft) const override
		{
			return Rectangle<int>(0, 0, 128, 32 + UIValues::HeaderHeight).withPosition(topLeft);
		}

		void process(ProcessData& data) override
		{
			if (connectedSource != nullptr)
			{
				int numChannelsToSend = jmin(connectedSource->buffer.getNumChannels(), data.numChannels);
				int numSamplesToSend = jmin(connectedSource->buffer.getNumSamples(), data.size);

				for (int i = 0; i < numChannelsToSend; i++)
					connectedSource->buffer.copyFrom(i, 0, data.data[i], numSamplesToSend);
			}
		}

		void processSingle(float* frameData, int numChannels) final override
		{
			if (connectedSource != nullptr)
			{
				FloatVectorOperations::copy(connectedSource->singleFrameData, frameData, numChannels);
			}
		}

		valuetree::PropertyListener connectionListener;

		WeakReference<SourceNode> connectedSource;

		JUCE_DECLARE_WEAK_REFERENCEABLE(TargetNode);
	};

	using input = SourceNode;
	using output = TargetNode;

	class FeedbackFactory : public NodeFactory
	{
	public:
		FeedbackFactory(DspNetwork* n) :
			NodeFactory(n)
		{
			registerNode<input>();
			registerNode<output>();
		}

		Identifier getId() const override { return "feedback"; }
	};
};



class HiseDspBase
{
public:

	virtual ~HiseDspBase() {};

	struct ParameterData
	{
		ParameterData(const String& id_) :
			id(id_)
		{};

		ValueTree createValueTree() const
		{
			ValueTree p(PropertyIds::Parameter);

			RangeHelpers::storeDoubleRange(p, false, range, nullptr);

			p.setProperty(PropertyIds::ID, id, nullptr);
			p.setProperty(PropertyIds::Value, defaultValue, nullptr);

			return p;
		}

		String id;
		NormalisableRange<double> range;
		double defaultValue = 0.0;

		void setParameterValueNames(const StringArray& valueNames)
		{
			parameterNames = valueNames;
			range = { 0.0, (double)valueNames.size() - 1.0, 1.0 };
		}

		void init()
		{
			db(defaultValue);
		}

		std::function<void(double)> db;
		StringArray parameterNames;
	};

	template <class ObjectType> class ExtraComponent : public Component,
		public PooledUIUpdater::SimpleTimer
	{
	protected:

		ExtraComponent(ObjectType* t, PooledUIUpdater* updater) :
			SimpleTimer(updater),
			object(dynamic_cast<HiseDspBase*>(t))
		{};

		ObjectType* getObject() const
		{
			return dynamic_cast<ObjectType*>(object.get());
		}

	private:

		WeakReference<HiseDspBase> object;
	};

	virtual int getExtraWidth() const { return 0; };

	virtual void initialise(ProcessorWithScriptingContent* sp) 
	{
		ignoreUnused(sp);
	};

	virtual Component* createExtraComponent(PooledUIUpdater* updater)
	{
		ignoreUnused(updater);
		return nullptr;
	}

	JUCE_DECLARE_WEAK_REFERENCEABLE(HiseDspBase);

	virtual void createParameters(Array<ParameterData>& data) = 0;
};

template <class HiseDspBaseType> class HiseDspNodeBase : public ModulationSourceNode
{
public:
	HiseDspNodeBase(DspNetwork* parent, ValueTree d) :
		ModulationSourceNode(parent, d)
	{
		obj.initialise(parent->getScriptProcessor());

		d.getOrCreateChildWithName(PropertyIds::Parameters, getUndoManager());

		Array<HiseDspBase::ParameterData> pData;

		obj.createParameters(pData);

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

		
	};

	SCRIPTNODE_FACTORY(HiseDspNodeBase, HiseDspBaseType::getStaticId());

	NodeComponent* createComponent() override
	{
		auto nc = new DefaultParameterNodeComponent(this);

		if (auto extra = obj.createExtraComponent(getScriptProcessor()->getMainController_()->getGlobalUIUpdater()))
		{
			extra->setSize(0, obj.ExtraHeight);
			nc->setExtraComponent(extra);
		}

		return nc;
	}

	Identifier getObjectName() const override { return getStaticId(); }

	Rectangle<int> getPositionInCanvas(Point<int> topLeft) const override
	{
		int numParameters = getNumParameters();
			
		if(numParameters == 0)
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
		h += obj.ExtraHeight;

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

		w = jmax(w, obj.getExtraWidth());

		auto b = Rectangle<int>(0, 0, w, h);
		return b.expanded(UIValues::NodeMargin);
	}

	void prepare(double sampleRate, int blockSize) final override
	{
		ModulationSourceNode::prepare(sampleRate, blockSize);

		obj.prepare(getNumChannelsToProcess(), sampleRate, blockSize);
	}

	void processSingle(float* frameData, int numChannels) final override
	{
		obj.processSingle(frameData, numChannels);

		if (obj.isModulationSource)
		{
			ProcessData d;
			d.data = &frameData;
			d.size = numChannels;
			d.numChannels = 1;

			double value = 0.0;
			if (obj.handleModulation(d, value))
				sendValueToTargets(value, 1);
		}
	}

	void process(ProcessData& data) noexcept final override
	{
		obj.process(data);

		if (obj.isModulationSource)
		{
			double value = 0.0;

			if (obj.handleModulation(data, value))
				sendValueToTargets(value, data.size);
		}
	}

	HiseDspBaseType obj;
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

	static void initParameterData(Array<HiseDspBase::ParameterData>& d, const Array<ParameterInitValue>& initValues)
	{
		for (auto& parameter : d)
		{
			for (auto& initValue : initValues)
			{
				if (parameter.id == initValue.id)
				{
					parameter.db(initValue.value);
					break;
				}
			}
		}
	}

	static HiseDspBase::ParameterData getParameter(const Array<HiseDspBase::ParameterData>& data, const String& id)
	{
		for (auto& c : data)
			if (c.id == id)
				return c;

		return HiseDspBase::ParameterData("undefined");
	}
};




class DspNode : public NodeBase,
	public AssignableObject
{
public:

	DspNode(DspNetwork* root, DspFactory* f_, ValueTree data);

	Identifier getObjectName() const override { return "DspNode"; }

	virtual void assign(const int index, var newValue) override
	{
		if (auto p = getParameter(index))
		{
			auto floatValue = (float)newValue;
			FloatSanitizers::sanitizeFloatNumber(floatValue);

			p->setValueAndStoreAsync(floatValue);
		}
		else
			reportScriptError("Cant' find parameter for index " + String(index));
	}

	/** Return the value for the specified index. The parameter passed in must relate to the index created with getCachedIndex. */
	var getAssignedValue(int index) const override
	{
		if (auto p = getParameter(index))
		{
			return p->getValue();
		}
		else
			reportScriptError("Cant' find parameter for index " + String(index));
	}

	virtual int getCachedIndex(const var &indexExpression) const override
	{
		return (int)indexExpression;
	}

	void prepare(double sampleRate, int blockSize) override
	{
		if (obj != nullptr)
			obj->prepareToPlay(sampleRate, blockSize);
	}

	void process(ProcessData& data) final override
	{
		if (obj != nullptr)
			obj->processBlock(data.data, data.numChannels, data.size);
	}

	~DspNode()
	{
		f->destroyDspBaseObject(obj);
	}

	Rectangle<int> getPositionInCanvas(Point<int> topLeft) const override
	{
		if (obj != nullptr)
		{
			int numParameters = obj->getNumParameters();

			int numRows = std::ceil((float)numParameters / 4.0f);

			auto b = Rectangle<int>(0, 0, jmin(400, numParameters * 100), numRows * (48+18) + UIValues::HeaderHeight);

			return b.expanded(UIValues::NodeMargin).withPosition(topLeft);
		}
	}

	NodeComponent* createComponent() override;

private:

	friend class DefaultParameterNodeComponent;

	String moduleName;

	void initialise();

	DspFactory::Ptr f;
	DspBaseObject* obj = nullptr;

};



}
