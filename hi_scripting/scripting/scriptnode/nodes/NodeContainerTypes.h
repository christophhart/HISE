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
	void handleHiseEvent(HiseEvent& e) final override;
	void reset() final override { wrapper.reset(); }

private:

	InternalWrapper wrapper;
	valuetree::PropertyListener bypassListener;
};


class ModulationChainNode : public SerialNode
{
public:

	SCRIPTNODE_FACTORY(ModulationChainNode, "modchain");

	ModulationChainNode(DspNetwork* n, ValueTree t);;

	void processSingle(float* frameData, int numChannels) noexcept final override;
	void process(ProcessData& data) noexcept final override;
	void prepare(PrepareSpecs ps) override;
	void handleHiseEvent(HiseEvent& e) final override;
	void reset() final override;

	NodeComponent* createComponent() override;
	String createCppClass(bool isOuterClass) override;

	int getBlockSizeForChildNodes() const override;
	double getSampleRateForChildNodes() const override;

	Rectangle<int> getPositionInCanvas(Point<int> topLeft) const override;

private:
	
	wrap::control_rate<SerialNode::DynamicSerialProcessor> obj;
};

class MidiChainNode : public SerialNode
{
public:

	SCRIPTNODE_FACTORY(MidiChainNode, "midichain");

	MidiChainNode(DspNetwork* n, ValueTree t);

	void processSingle(float* frameData, int numChannels) noexcept final override;
	void process(ProcessData& data) noexcept final override;
	void prepare(PrepareSpecs ps) override;
	void handleHiseEvent(HiseEvent& e) final override;
	void reset() final override;
	String getCppCode(CppGen::CodeLocation location) override;
	
private:

	wrap::event<SerialNode::DynamicSerialProcessor> obj;
};

template <int OversampleFactor> class OversampleNode : public SerialNode
{
public:

	SCRIPTNODE_FACTORY(OversampleNode, "oversample" + String(OversampleFactor) + "x");

	OversampleNode(DspNetwork* network, ValueTree d);

	double getSampleRateForChildNodes() const override;

	int getBlockSizeForChildNodes() const override;

	void updateBypassState(Identifier, var );

	void prepare(PrepareSpecs ps) override;
	void reset() final override;
	void handleHiseEvent(HiseEvent& e) final override;
	void process(ProcessData& d) noexcept final override;

	wrap::oversample<OversampleFactor, SerialNode::DynamicSerialProcessor> obj;

	valuetree::PropertyListener bypassListener;
	int* lastVoiceIndex = nullptr;
};

template <int B> class FixedBlockNode : public SerialNode
{
public:

	static constexpr int FixedBlockSize = B;

	SCRIPTNODE_FACTORY(FixedBlockNode<B>, "fix" + String(FixedBlockSize) + "_block");

	FixedBlockNode(DspNetwork* network, ValueTree d);

	void process(ProcessData& data) final override;
	void prepare(PrepareSpecs ps) final override;
	void reset() final override;
	void handleHiseEvent(HiseEvent& e) final override;

	int getBlockSizeForChildNodes() const override
	{
		return isBypassed() ? originalBlockSize : FixedBlockSize;
	}

	void updateBypassState(Identifier, var);

	wrap::fix_block<DynamicSerialProcessor, FixedBlockSize> obj;

	valuetree::PropertyListener bypassListener;
	int* lastVoiceIndex = nullptr;
};



class SplitNode : public ParallelNode
{
public:

	SplitNode(DspNetwork* root, ValueTree data);;

	SCRIPTNODE_FACTORY(SplitNode, "split");

	String getCppCode(CppGen::CodeLocation location) override;
	void prepare(PrepareSpecs ps) override;
	void reset() final override;
	void handleHiseEvent(HiseEvent& e) final override;
	void process(ProcessData& data) final override;
	void processSingle(float* frameData, int numChannels) override;

	AudioSampleBuffer splitBuffer;
};


class MultiChannelNode : public ParallelNode
{
public:

	MultiChannelNode(DspNetwork* root, ValueTree data);;

	SCRIPTNODE_FACTORY(MultiChannelNode, "multi");

	void prepare(PrepareSpecs ps) final override;
	void reset() final override;
	void handleHiseEvent(HiseEvent& e) override;
	void processSingle(float* frameData, int numChannels);
	void process(ProcessData& d) override;

	void channelLayoutChanged(NodeBase* nodeThatCausedLayoutChange) override;

	float* currentChannelData[NUM_MAX_CHANNELS];
	Range<int> channelRanges[NUM_MAX_CHANNELS];
};




class SingleSampleBlockX : public SerialNode
{
public:

	SingleSampleBlockX(DspNetwork* n, ValueTree d);

	SCRIPTNODE_FACTORY(SingleSampleBlockX, "framex_block");

	void prepare(PrepareSpecs ps) final override;
	void reset() final override;
	void process(ProcessData& data) final override;
	void processSingle(float* frameData, int numChannels) final override;
	int getBlockSizeForChildNodes() const override;;
	void handleHiseEvent(HiseEvent& e) override;

	String getCppCode(CppGen::CodeLocation location);

	valuetree::PropertyListener bypassListener;
	wrap::frame_x<SerialNode::DynamicSerialProcessor> obj;
	AudioSampleBuffer leftoverBuffer;
};

template <int NumChannels> class SingleSampleBlock : public SerialNode
{
public:

	SingleSampleBlock(DspNetwork* n, ValueTree d) :
		SerialNode(n, d)
	{
		initListeners();
		obj.getObject().initialise(this);
	};

	SCRIPTNODE_FACTORY(SingleSampleBlock<NumChannels>, "frame" + String(NumChannels) + "_block");

	String getCppCode(CppGen::CodeLocation location)
	{
		if (location == CppGen::CodeLocation::Definitions)
		{
			String s;
			s << SerialNode::getCppCode(location);
			CppGen::Emitter::emitDefinition(s, "SET_HISE_NODE_IS_MODULATION_SOURCE", "false", false);
			return s;
		}
		
		return SerialNode::getCppCode(location);
	}

	void reset() final override
	{
		obj.reset();
	}

	void prepare(PrepareSpecs ps) final override
	{
		prepareNodes(ps);

		auto numLeftOverChannels = NumChannels - ps.numChannels;

		if (numLeftOverChannels <= 0)
			leftoverBuffer = {};
		else
			leftoverBuffer.setSize(numLeftOverChannels, ps.blockSize);
	}

	void process(ProcessData& data) final override
	{
		if (isBypassed())
			obj.getObject().process(data);
		else
		{
			float* channels[NumChannels];

			int numChannels = jmin(NumChannels, data.numChannels);

			memcpy(channels, data.data, numChannels * sizeof(float*));

			int numLeftOverChannels = NumChannels - data.numChannels;

			if (numLeftOverChannels > 0)
			{
				// fix channel mismatch
				jassert(leftoverBuffer.getNumChannels() == numLeftOverChannels);

				leftoverBuffer.clear();

				for (int i = 0; i < numLeftOverChannels; i++)
					channels[data.numChannels + i] = leftoverBuffer.getWritePointer(i);
			}

			ProcessData copy(channels, NumChannels, data.size);
			obj.process(copy);
		}
	}

	void processSingle(float* frameData, int numChannels) final override
	{
		jassert(numChannels == NumChannels);

		obj.processSingle(frameData, numChannels);
	}

	void updateBypassState(Identifier, var)
	{
		prepare(originalSampleRate, originalBlockSize);
	}

	int getBlockSizeForChildNodes() const override
	{
		return isBypassed() ? originalBlockSize : 1;
	};

	void handleHiseEvent(HiseEvent& e) override
	{
		obj.handleHiseEvent(e);
	}

	valuetree::PropertyListener bypassListener;

	wrap::frame<NumChannels, SerialNode::DynamicSerialProcessor> obj;

	AudioSampleBuffer leftoverBuffer;
};


}
