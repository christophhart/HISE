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

struct NodeContainer : public NodeBase,
	public AssignableObject
{
	struct MacroParameter : public NodeBase::Parameter
	{
		struct Connection
		{
			Connection(NodeBase* parent, ValueTree& d);

			DspHelpers::ParameterCallback createCallbackForNormalisedInput();
			bool isValid() const { return p.get() != nullptr || nodeToBeBypassed.get() != nullptr; };

		private:

			NodeBase::Ptr nodeToBeBypassed;
			double rangeMultiplerForBypass = 1.0;

			valuetree::PropertySyncer opSyncer;

			Identifier conversion = ConverterIds::Identity;
			Identifier opType = OperatorIds::SetValue;
			ReferenceCountedObjectPtr<Parameter> p;
			NormalisableRange<double> connectionRange;
			bool inverted = false;
		};

		ValueTree getConnectionTree();

		MacroParameter(NodeBase* parentNode, ValueTree data_);;

		void rebuildCallback();
		void updateRangeForConnection(ValueTree& v, Identifier);

		NormalisableRange<double> inputRange;

		valuetree::ChildListener connectionListener;
		valuetree::RecursivePropertyListener rangeListener;
	};

	NodeContainer(DspNetwork* root, ValueTree data);

	ValueTree getNodeTree() const { return data.getChildWithName(PropertyIds::Nodes); }

	void prepare(double sampleRate, int blockSize) override
	{
		ScopedLock sl(getRootNetwork()->getConnectionLock());

		originalSampleRate = sampleRate;
		originalBlockSize = blockSize;

		for (auto n : nodes)
			n->prepare(getSampleRateForChildNodes(), getBlockSizeForChildNodes());
	}

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
		getNodeTree().removeAllChildren(getUndoManager());
	}

	String createCppClass(bool isOuterClass) override;

	virtual String getCppCode(CppGen::CodeLocation location);

	ValueTree getNodeTree() { return data.getOrCreateChildWithName(PropertyIds::Nodes, getUndoManager()); }

protected:

	void initListeners();

	friend class ContainerComponent;

	List nodes;

	double originalSampleRate = 0.0;
	int originalBlockSize = 0;

	virtual void channelLayoutChanged(NodeBase* nodeThatCausedLayoutChange) {};

private:

	void nodeAddedOrRemoved(ValueTree& v, bool wasAdded);
	void parameterAddedOrRemoved(ValueTree& v, bool wasAdded);
	void updateChannels(ValueTree v, Identifier id);

	valuetree::ChildListener nodeListener;
	valuetree::ChildListener parameterListener;
	valuetree::RecursivePropertyListener channelListener;

	bool channelRecursionProtection = false;
};

class SerialNode : public NodeContainer
{
public:

	SerialNode(DspNetwork* root, ValueTree data);

	Identifier getObjectName() const override { return "SerialNode"; };
	NodeComponent* createComponent() override;
	Rectangle<int> getPositionInCanvas(Point<int> topLeft) const override;

	String getCppCode(CppGen::CodeLocation location) override;

private:

	
};

class ChainNode : public SerialNode
{
public:

	SCRIPTNODE_FACTORY(ChainNode, "chain");

	ChainNode(DspNetwork* n, ValueTree t);

	String getCppCode(CppGen::CodeLocation location) override;

	void process(ProcessData& data) final override;
	void processSingle(float* frameData, int numChannels) final override;
};

class ModulationChainNode : public SerialNode
{
public:

	SCRIPTNODE_FACTORY(ModulationChainNode, "mod");

	ModulationChainNode(DspNetwork* n, ValueTree t);;

	void processSingle(float* frameData, int numChannels) noexcept final override;
	void process(ProcessData& data) noexcept final override;

	String getCppCode(CppGen::CodeLocation location) override;

	virtual int getBlockSizeForChildNodes() const { return jmax(1, originalBlockSize / HISE_EVENT_RASTER); }
	virtual double getSampleRateForChildNodes() const { return originalSampleRate / (double)HISE_EVENT_RASTER; }

private:
	
	int singleCounter = 0;
};


template <int OversampleFactor> class OversampleNode : public SerialNode
{
public:

	using Oversampler = juce::dsp::Oversampling<float>;

	SCRIPTNODE_FACTORY(OversampleNode, "oversample" + String(OversampleFactor) + "x");

	OversampleNode(DspNetwork* network, ValueTree d):
		SerialNode(network, d)
	{
		initListeners();

		bypassListener.setCallback(d, { PropertyIds::Bypassed },
			valuetree::AsyncMode::Synchronously,
			BIND_MEMBER_FUNCTION_2(OversampleNode<OversampleFactor>::updateBypassState));
	}

	void updateBypassState(Identifier, var newValue)
	{
		auto bp = (bool)newValue;
		prepare(originalSampleRate, originalBlockSize);
	}

	void prepare(double sampleRate, int blockSize) override
	{
		NodeContainer::prepare(sampleRate, blockSize);

		ScopedPointer<Oversampler> newOverSampler;
		
		if (!isBypassed())
		{
			newOverSampler = new Oversampler(getNumChannelsToProcess(), std::log2(OversampleFactor), Oversampler::FilterType::filterHalfBandPolyphaseIIR, false);

			if (blockSize > 0)
				newOverSampler->initProcessing(blockSize);
		}

		{
			ScopedLock sl(getRootNetwork()->getConnectionLock());
			oversampler.swapWith(newOverSampler);
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

	void process(ProcessData& d) noexcept final override
	{
		if (isBypassed())
		{
			for (auto n : nodes)
				n->process(d);
		}
		else
		{
			if (oversampler == nullptr)
				return;

			juce::dsp::AudioBlock<float> input(d.data, d.numChannels, d.size);

			auto& output = oversampler->processSamplesUp(input);

			float* data[NUM_MAX_CHANNELS];

			for (int i = 0; i < d.numChannels; i++)
				data[i] = output.getChannelPointer(i);

			ProcessData od;
			od.data = data;
			od.numChannels = d.numChannels;
			od.size = d.size * OversampleFactor;

			for (auto n : nodes)
				n->process(od);

			oversampler->processSamplesDown(input);
		}
	}

	ScopedPointer<Oversampler> oversampler;

	valuetree::PropertyListener bypassListener;
};


class ParallelNode : public NodeContainer
{
public:

	ParallelNode(DspNetwork* root, ValueTree data) :
		NodeContainer(root, data)
	{}

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

	void prepare(double sampleRate, int blockSize) override;

	String getCppCode(CppGen::CodeLocation location) override;

	void process(ProcessData& data) final override;

	void processSingle(float* frameData, int numChannels) override;

	AudioSampleBuffer splitBuffer;
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

	void process(ProcessData& data) override
	{
		int channelIndex = 0;

		for (auto n : nodes)
		{
			int numChannelsThisTime = n->getNumChannelsToProcess();
			int startChannel = channelIndex;
			int endChannel = startChannel + numChannelsThisTime;

			if (endChannel <= data.numChannels)
			{
				for (int i = 0; i < numChannelsThisTime; i++)
					currentChannelData[i] = data.data[startChannel + i];

				ProcessData thisData;
				thisData.data = currentChannelData;
				thisData.numChannels = numChannelsThisTime;
				thisData.size = data.size;

				n->process(thisData);
			}

			channelIndex += numChannelsThisTime;
		}
	}

	void channelLayoutChanged(NodeBase* nodeThatCausedLayoutChange) override;

	float* currentChannelData[NUM_MAX_CHANNELS];

	Identifier getObjectName() const override { return "MultiChannelNode"; };
};

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

class NodeContainerFactory : public NodeFactory
{
public:

	NodeContainerFactory(DspNetwork* parent);

	Identifier getId() const override { RETURN_STATIC_IDENTIFIER("container"); };
};

}
