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

struct NodeContainer : public AssignableObject
{
	struct MacroParameter : public NodeBase::Parameter
	{
		struct Connection
		{
			Connection(NodeBase* parent, ValueTree d);

			DspHelpers::ParameterCallback createCallbackForNormalisedInput();
			bool isValid() const { return p.get() != nullptr || nodeToBeBypassed.get() != nullptr; };

		private:

			

			NodeBase::Ptr nodeToBeBypassed;
			double rangeMultiplerForBypass = 1.0;

			valuetree::PropertySyncer opSyncer;
			valuetree::PropertyListener idUpdater;
			
			valuetree::RemoveListener nodeRemoveUpdater;

			Identifier conversion = ConverterIds::Identity;
			Identifier opType = OperatorIds::SetValue;
			ReferenceCountedObjectPtr<Parameter> p;
			NormalisableRange<double> connectionRange;
			bool inverted = false;
		};

		ValueTree getConnectionTree();

		MacroParameter(NodeBase* parentNode, ValueTree data_);;

		void rebuildCallback();
		void updateRangeForConnection(ValueTree v, Identifier);

		NormalisableRange<double> inputRange;

		valuetree::ChildListener connectionListener;
		valuetree::RecursivePropertyListener rangeListener;

		OwnedArray<Connection> connections;
	};

	NodeContainer();

	void resetNodes()
	{ 
		for (auto n : nodes)
			n->reset();
	}

	NodeBase* asNode() 
	{ 
		auto n = dynamic_cast<NodeBase*>(this); 
		jassert(n != nullptr);
		return n;
	}
	const NodeBase* asNode() const 
	{ 
		auto n = dynamic_cast<const NodeBase*>(this);
		jassert(n != nullptr);
		return n;
	}

	void prepareNodes(PrepareSpecs ps)
	{
		ScopedLock sl(asNode()->getRootNetwork()->getConnectionLock());

		originalSampleRate = ps.sampleRate;
		originalBlockSize = ps.blockSize;
		lastVoiceIndex = ps.voiceIndex;

		ps.sampleRate = getSampleRateForChildNodes();
		ps.blockSize = getBlockSizeForChildNodes();

		for (auto n : nodes)
		{
			n->prepare(ps);
			n->reset();
		}
	}

	bool shouldCreatePolyphonicClass() const
	{
		if (isPolyphonic())
		{
			for (auto n : getNodeList())
			{
				if (auto nc = dynamic_cast<NodeContainer*>(n.get()))
				{
					if (nc->shouldCreatePolyphonicClass())
						return true;
				}

				if (n->isPolyphonic())
					return true;
			}

			return false;
		}

		return false;
	}

	virtual bool isPolyphonic() const;

	virtual void assign(const int index, var newValue) override;

	/** Return the value for the specified index. The parameter passed in must relate to the index created with getCachedIndex. */
	var getAssignedValue(int index) const override
	{
		return var(nodes[index]);
	}

	virtual int getBlockSizeForChildNodes() const { return originalBlockSize; }
	virtual double getSampleRateForChildNodes() const { return originalSampleRate; }

	virtual int getCachedIndex(const var &indexExpression) const override
	{
		return (int)indexExpression;
	}

	// ===================================================================================

	void clear()
	{
		getNodeTree().removeAllChildren(asNode()->getUndoManager());
	}

	String createCppClassForNodes(bool isOuterClass);

	String createTemplateAlias();
	
	NodeBase::List getChildNodesRecursive()
	{
		NodeBase::List l;

		for (auto n : nodes)
		{
			l.add(n);

			if (auto c = dynamic_cast<NodeContainer*>(n.get()))
				l.addArray(c->getChildNodesRecursive());
		}

		return l;
	}

	void fillAccessors(Array<CppGen::Accessor>& accessors, Array<int> currentPath)
	{
		for (int i = 0; i < nodes.size(); i++)
		{
			Array<int> thisPath = currentPath;
			thisPath.add(i);

			if (auto c = dynamic_cast<NodeContainer*>(nodes[i].get()))
			{
				c->fillAccessors(accessors, thisPath);
			}
			else
			{
				accessors.add({ nodes[i]->getId(), thisPath });
			}
		}
	}

	virtual String getCppCode(CppGen::CodeLocation location);

	ValueTree getNodeTree() { return asNode()->getValueTree().getOrCreateChildWithName(PropertyIds::Nodes, asNode()->getUndoManager()); }

	NodeBase::List& getNodeList() { return nodes; }
	const NodeBase::List& getNodeList() const { return nodes; }

protected:

	void initListeners();

	friend class ContainerComponent;

	NodeBase::List nodes;

	double originalSampleRate = 0.0;
	int originalBlockSize = 0;

	virtual void channelLayoutChanged(NodeBase* nodeThatCausedLayoutChange) { ignoreUnused(nodeThatCausedLayoutChange); };

private:

	void nodeAddedOrRemoved(ValueTree v, bool wasAdded);
	void parameterAddedOrRemoved(ValueTree v, bool wasAdded);
	void updateChannels(ValueTree v, Identifier id);

	valuetree::ChildListener nodeListener;
	valuetree::ChildListener parameterListener;
	valuetree::RecursivePropertyListener channelListener;

	int* lastVoiceIndex = nullptr;
	bool channelRecursionProtection = false;
};

class SerialNode : public NodeBase,
				   public NodeContainer
{
public:

	class DynamicSerialProcessor: public HiseDspBase
	{
	public:

		SET_HISE_NODE_EXTRA_HEIGHT(0);
		SET_HISE_NODE_IS_MODULATION_SOURCE(false);

		bool handleModulation(double&) 
		{ 
			return false; 
		}

		void handleHiseEvent(HiseEvent& e) final override
		{
			for (auto n : parent->getNodeList())
				n->handleHiseEvent(e);
		}

		void initialise(NodeBase* p)
		{
			parent = dynamic_cast<NodeContainer*>(p);
		}

		void reset()
		{
			for (auto n : parent->getNodeList())
				n->reset();
		}

		void prepare(PrepareSpecs)
		{
			// do nothing here, the container inits the child nodes.
		}

		void process(ProcessData& d)
		{
			jassert(parent != nullptr);

			for (auto n : parent->getNodeList())
				n->process(d);
		}

		void processSingle(float* frameData, int numChannels)
		{
			jassert(parent != nullptr);

			for (auto n : parent->getNodeList())
				n->processSingle(frameData, numChannels);
		}

		void createParameters(Array<ParameterData>& ) override {};

		DynamicSerialProcessor& getObject() { return *this; }
		const DynamicSerialProcessor& getObject() const { return *this; }

		NodeContainer* parent;
	};

	SerialNode(DspNetwork* root, ValueTree data);

	Identifier getObjectName() const override { return "SerialNode"; };
	NodeComponent* createComponent() override;
	Rectangle<int> getPositionInCanvas(Point<int> topLeft) const override;

	String createCppClass(bool isOuterClass) override
	{
		return createCppClassForNodes(isOuterClass);
	}

	String getCppCode(CppGen::CodeLocation location) override;

};




class ChainNode : public SerialNode
{
	using InternalWrapper = bypass::smoothed<SerialNode::DynamicSerialProcessor, false>;

public:

	SCRIPTNODE_FACTORY(ChainNode, "chain");

	ChainNode(DspNetwork* n, ValueTree t);

	String getCppCode(CppGen::CodeLocation location) override;

	void process(ProcessData& data) final override;
	void processSingle(float* frameData, int numChannels) final override;

	void prepare(PrepareSpecs ps) override;

	void handleHiseEvent(HiseEvent& e) final override
	{
		wrapper.handleHiseEvent(e);
	}

	void reset() final override { wrapper.reset(); }

private:

	InternalWrapper wrapper;
	valuetree::PropertyListener bypassListener;
};


class ModulationChainNode : public ModulationSourceNode,
							public NodeContainer
{
public:

	class DynamicModContainer : public HiseDspBase
	{
	public:

		SET_HISE_NODE_EXTRA_HEIGHT(0);
		SET_HISE_NODE_IS_MODULATION_SOURCE(true);

		bool handleModulation(double& value)
		{
			value = modValue.get();
			return true;
		}

		void initialise(NodeBase* p)
		{
			parent = dynamic_cast<NodeContainer*>(p);
		}

		void reset()
		{
			for (auto n : parent->getNodeList())
				n->reset();

			if (modValue.isVoiceRenderingActive())
				modValue.get() = 0.0;
			else
				modValue.setAll(0.0);
		}

		void handleHiseEvent(HiseEvent& e)
		{
			for (auto n : parent->getNodeList())
				n->handleHiseEvent(e);
		}

		void prepare(PrepareSpecs ps)
		{
			modValue.prepare(ps);
			// do nothing here, the container inits the child nodes.
		}

		void process(ProcessData& d)
		{
			jassert(parent != nullptr);

			for (auto n : parent->getNodeList())
				n->process(d);

			modValue.get() = DspHelpers::findPeak(d);
		}

		void processSingle(float* frameData, int numChannels)
		{
			jassert(parent != nullptr);

			for (auto n : parent->getNodeList())
				n->processSingle(frameData, numChannels);
		}

		void createParameters(Array<ParameterData>&) override {};

		DynamicModContainer& getObject() { return *this; }
		const DynamicModContainer& getObject() const { return *this; }

		NodeContainer* parent = nullptr;
		PolyData<double, NUM_POLYPHONIC_VOICES> modValue = 0.0;
	};

	SCRIPTNODE_FACTORY(ModulationChainNode, "modchain");

	ModulationChainNode(DspNetwork* n, ValueTree t);;

	void processSingle(float* frameData, int numChannels) noexcept final override;
	void process(ProcessData& data) noexcept final override;
	void prepare(PrepareSpecs ps) override
	{
		NodeContainer::prepareNodes(ps);
		obj.prepare(ps);
		lastValue.prepare(ps);
	}

	void handleHiseEvent(HiseEvent& e) final override
	{
		obj.handleHiseEvent(e);
	}

	void reset() final override
	{
		NodeContainer::resetNodes();
		obj.reset();
		
		if (lastValue.isVoiceRenderingActive())
			lastValue.get() = 0.0;
		else
			lastValue.setAll(0.0);
	}

	NodeComponent* createComponent() override;

	String getCppCode(CppGen::CodeLocation location) override;

	String createCppClass(bool isOuterClass) override
	{
		return createCppClassForNodes(isOuterClass);
	}

	virtual int getBlockSizeForChildNodes() const { return jmax(1, originalBlockSize / HISE_EVENT_RASTER); }
	virtual double getSampleRateForChildNodes() const { return originalSampleRate / (double)HISE_EVENT_RASTER; }

	Identifier getObjectName() const override { return getStaticId(); }

	Rectangle<int> getPositionInCanvas(Point<int> topLeft) const override;

private:
	
	container::mod<DynamicModContainer> obj;

	PolyData<double, NUM_POLYPHONIC_VOICES> lastValue = 0.0;
	int numLeft = 0;
};

class PolySynthesiserNode : public SerialNode
{
public:

	class DynamicSynthChain : public HiseDspBase
	{
	public:

		SET_HISE_NODE_EXTRA_HEIGHT(0);
		SET_HISE_NODE_IS_MODULATION_SOURCE(false);

		bool handleModulation(double&)
		{
			return false;
		}

		void handleHiseEvent(HiseEvent& e) final override
		{
			for (auto n : parent->getNodeList())
				n->handleHiseEvent(e);
		}

		void initialise(NodeBase* p)
		{
			parent = dynamic_cast<NodeContainer*>(p);
		}

		void reset()
		{
			for (auto n : parent->getNodeList())
				n->reset();
		}

		void prepare(PrepareSpecs)
		{
			// do nothing here, the container inits the child nodes.
		}

		void process(ProcessData& d)
		{
			jassert(parent != nullptr);

			for (auto n : parent->getNodeList())
				n->process(d);
		}

		void processSingle(float* frameData, int numChannels)
		{
			jassert(parent != nullptr);

			for (auto n : parent->getNodeList())
				n->processSingle(frameData, numChannels);
		}

		void createParameters(Array<ParameterData>&) override {};

		DynamicSynthChain& getObject() { return *this; }
		const DynamicSynthChain& getObject() const { return *this; }

		NodeContainer* parent;
	};

	SCRIPTNODE_FACTORY(PolySynthesiserNode, "synth");

	PolySynthesiserNode(DspNetwork* n, ValueTree t);;

	bool isPolyphonic() const override { return true; }

	void processSingle(float*, int) noexcept final override
	{
		jassertfalse;
	}

	void process(ProcessData& d) noexcept final override
	{
		obj.process(d);
	}

	void prepare(PrepareSpecs ps) override
	{
		ps.voiceIndex = &obj.currentVoiceIndex;
		NodeContainer::prepareNodes(ps);
		obj.prepare(ps);
	}

	void handleHiseEvent(HiseEvent& e) final override
	{
		obj.handleHiseEvent(e);
	}

	void reset() override
	{
		obj.reset();
	}

private:

	wrap::synth<DynamicSynthChain> obj;
};

template <int OversampleFactor> class OversampleNode : public SerialNode
{
public:

	SCRIPTNODE_FACTORY(OversampleNode, "oversample" + String(OversampleFactor) + "x");

	OversampleNode(DspNetwork* network, ValueTree d):
		SerialNode(network, d)
	{
		initListeners();

		obj.initialise(this);

		bypassListener.setCallback(d, { PropertyIds::Bypassed },
			valuetree::AsyncMode::Synchronously,
			BIND_MEMBER_FUNCTION_2(OversampleNode<OversampleFactor>::updateBypassState));
	}

	void updateBypassState(Identifier, var )
	{
		if (originalBlockSize == 0 || originalSampleRate == 0.0)
			return;

		PrepareSpecs ps;
		ps.blockSize = originalBlockSize;
		ps.sampleRate = originalSampleRate;
		ps.numChannels = getNumChannelsToProcess();
		ps.voiceIndex = lastVoiceIndex;

		prepare(ps);
	}

	void prepare(PrepareSpecs ps) override
	{
		lastVoiceIndex = ps.voiceIndex;
		prepareNodes(ps);

		if (isBypassed())
		{
			obj.getObject().prepare(ps);
		}
		else
		{
			obj.prepare(ps);
		}

		
	}

	

	double getSampleRateForChildNodes() const override
	{
		return isBypassed() ? originalSampleRate : originalSampleRate * OversampleFactor;
	}

	int getBlockSizeForChildNodes() const override
	{
		return isBypassed() ? originalBlockSize : originalBlockSize * OversampleFactor;
	}

	void reset() final override
	{
		obj.reset();
	}

	void handleHiseEvent(HiseEvent& e) final override
	{
		obj.handleHiseEvent(e);
	}

	void process(ProcessData& d) noexcept final override
	{
		if (isBypassed())
		{
			obj.getObject().process(d);
		}
		else
		{
			obj.process(d);
		}
	}

	wrap::oversample<OversampleFactor, SerialNode::DynamicSerialProcessor> obj;

	valuetree::PropertyListener bypassListener;
	int* lastVoiceIndex = nullptr;
};


class ParallelNode : public NodeBase,
					 public NodeContainer
{
public:

	ParallelNode(DspNetwork* root, ValueTree data) :
		NodeBase(root, data, 0)
	{}

	String createCppClass(bool isOuterClass) override
	{
		return createCppClassForNodes(isOuterClass);
	}

	NodeComponent* createComponent() override;

	Rectangle<int> getPositionInCanvas(Point<int> topLeft) const override;
};

class SplitNode : public ParallelNode
{
public:

	SplitNode(DspNetwork* root, ValueTree data) :
		ParallelNode(root, data)
	{
		initListeners();
	};

	Identifier getObjectName() const override { return "SplitNode"; };

	SCRIPTNODE_FACTORY(SplitNode, "split");

	void prepare(PrepareSpecs ps) override;

	void reset() final override
	{
		resetNodes();
	}

	String getCppCode(CppGen::CodeLocation location) override;

	

	void handleHiseEvent(HiseEvent& e) final override
	{
		for (auto n : nodes)
		{
			HiseEvent copy(e);
			n->handleHiseEvent(e);
		}
	}

	void process(ProcessData& data) final override;

	void processSingle(float* frameData, int numChannels) override;

	AudioSampleBuffer splitBuffer;
};



class FeedbackContainer : public ParallelNode
{
public:

	class DynamicFeedbackNode : public HiseDspBase
	{
	public:

		SET_HISE_NODE_EXTRA_HEIGHT(0);
		SET_HISE_NODE_IS_MODULATION_SOURCE(false);

		bool handleModulation(double&) { return false; }

		void initialise(NodeBase* p)
		{
			parent = dynamic_cast<NodeContainer*>(p);
		}

		void prepare(PrepareSpecs ps)
		{
			DspHelpers::increaseBuffer(feedbackBuffer, ps);

			auto nl = parent->getNodeList();
			first = nl[0];
			feedbackLoop = nl[1];

			reset();
			// do nothing here, the container inits the child nodes.
		}

		void handleHiseEvent(HiseEvent& e) final override
		{
			for (auto n : parent->getNodeList())
				n->handleHiseEvent(e);
		}

		void reset()
		{
			feedbackBuffer.clear();
			memset(singleData, 0, sizeof(float)* feedbackBuffer.getNumChannels());

			for (auto n : parent->getNodeList())
				n->reset();
		}

		void process(ProcessData& d)
		{
			jassert(parent != nullptr);

			auto fb = d.referTo(feedbackBuffer, 0);

			d += fb;

			if(first != nullptr)
				first->process(d);

			if (feedbackLoop != nullptr)
			{
				d.copyTo(feedbackBuffer, 0);
				feedbackLoop->process(fb);
			}
		}

		void processSingle(float* frameData, int numChannels)
		{
			for (int i = 0; i < numChannels; i++)
				frameData[i] += singleData[i];

			if (first != nullptr)
				first->processSingle(frameData, numChannels);

			if (feedbackLoop != nullptr)
			{
				for (int i = 0; i < numChannels; i++)
					singleData[i] = frameData[i];

				feedbackLoop->processSingle(singleData, numChannels);
			}
		}

		void createParameters(Array<ParameterData>&) override {};

		DynamicFeedbackNode& getObject() { return *this; }
		const DynamicFeedbackNode& getObject() const { return *this; }

		NodeContainer* parent;

		NodeBase::Ptr first;
		NodeBase::Ptr feedbackLoop;

		AudioSampleBuffer feedbackBuffer;
		float singleData[NUM_MAX_CHANNELS];
	};

	FeedbackContainer(DspNetwork* root, ValueTree data):
		ParallelNode(root, data)
	{
		initListeners();
		obj.initialise(this);
	}

	SCRIPTNODE_FACTORY(FeedbackContainer, "feedback");

	void process(ProcessData& d) final override
	{
		obj.process(d);
		
	}

	void prepare(PrepareSpecs ps)
	{
		NodeContainer::prepareNodes(ps);
		obj.prepare(ps);
	}

	void reset() final override
	{
		obj.reset();
		
	}

	void processSingle(float* frameData, int numChannels) final override
	{
		obj.processSingle(frameData, numChannels);
	}

	Identifier getObjectName() const override { return "FeedbackNode"; };

	DynamicFeedbackNode obj;
	
};



class MultiChannelNode : public ParallelNode
{
public:

	MultiChannelNode(DspNetwork* root, ValueTree data) :
		ParallelNode(root, data)
	{
		initListeners();
	};

	SCRIPTNODE_FACTORY(MultiChannelNode, "multi");

	void prepare(PrepareSpecs ps) final override
	{
		NodeContainer::prepareNodes(ps);

		int channelIndex = 0;

		for (int i = 0; i < NUM_MAX_CHANNELS; i++)
		{
			channelRanges[i] = {};
		}

		for (int i = 0; i < jmin(NUM_MAX_CHANNELS, nodes.size()); i++)
		{
			int numChannelsThisTime = nodes[i]->getNumChannelsToProcess();
			int startChannel = channelIndex;
			int endChannel = startChannel + numChannelsThisTime;

			channelRanges[i] = { startChannel, endChannel };
			channelIndex += numChannelsThisTime;
		}
	}

	void reset() final override
	{
		NodeContainer::resetNodes();
	}

	void handleHiseEvent(HiseEvent& e) override
	{
		for (auto n : nodes)
		{
			HiseEvent c(e);
			n->handleHiseEvent(c);
		}
	}

	void processSingle(float* frameData, int numChannels)
	{
		for (int i = 0; i < nodes.size(); i++)
		{
			auto& r = channelRanges[i];

			if (r.getLength() == 0)
				continue;

			float* d = frameData + r.getStart();
			int numThisThime = r.getLength();
			nodes[i]->processSingle(d, numThisThime);
		}
	}

	void process(ProcessData& d) override
	{
		int channelIndex = 0;

		for (auto n : nodes)
		{
			int numChannelsThisTime = n->getNumChannelsToProcess();
			int startChannel = channelIndex;
			int endChannel = startChannel + numChannelsThisTime;

			if (endChannel <= d.numChannels)
			{
				for (int i = 0; i < numChannelsThisTime; i++)
					currentChannelData[i] = d.data[startChannel + i];

				ProcessData thisData;
				thisData.data = currentChannelData;
				thisData.numChannels = numChannelsThisTime;
				thisData.size = d.size;

				n->process(thisData);
			}

			channelIndex += numChannelsThisTime;
		}
	}

	

	void channelLayoutChanged(NodeBase* nodeThatCausedLayoutChange) override;

	float* currentChannelData[NUM_MAX_CHANNELS];

	Range<int> channelRanges[NUM_MAX_CHANNELS];

	Identifier getObjectName() const override { return "MultiChannelNode"; };
};

/*
namespace container
{
using split = SplitNode;
using chain = ChainNode;
using multi = MultiChannelNode;
using mod = ModulationChainNode;
using oversample16x = OversampleNode<16>;
using oversample8x = OversampleNode<8>;
using oversample4x = OversampleNode<4>;
using oversample2x = OversampleNode<2>;

}
*/

class NodeContainerFactory : public NodeFactory
{
public:

	NodeContainerFactory(DspNetwork* parent);

	Identifier getId() const override { RETURN_STATIC_IDENTIFIER("container"); };
};

}
