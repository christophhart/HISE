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

	void process(ProcessDataDyn& data) final override;
	void processFrame(FrameType& data) final override;

	void processMonoFrame(MonoFrameType& data) final override;
	void processStereoFrame(StereoFrameType& data) final override;

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

	void processFrame(FrameType& data) noexcept final override;
	void process(ProcessDataDyn& data) noexcept final override;
	void prepare(PrepareSpecs ps) override;
	void handleHiseEvent(HiseEvent& e) final override;
	void reset() final override;

	NodeComponent* createComponent() override;
	String createCppClass(bool isOuterClass) override;

	int getBlockSizeForChildNodes() const override;
	double getSampleRateForChildNodes() const override;

	Rectangle<int> getPositionInCanvas(Point<int> topLeft) const override;

private:
	
	wrap::fix<1, wrap::control_rate<SerialNode::DynamicSerialProcessor>> obj;
};

class MidiChainNode : public SerialNode
{
public:

	SCRIPTNODE_FACTORY(MidiChainNode, "midichain");

	MidiChainNode(DspNetwork* n, ValueTree t);

	void processFrame(FrameType& data) noexcept final override;
	void process(ProcessDataDyn& data) noexcept final override;
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
	void process(ProcessDataDyn& d) noexcept final override;
	void processFrame(FrameType& data) noexcept final override { jassertfalse; }

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

	void process(ProcessDataDyn& data) final override;

	void processFrame(FrameType& data) noexcept final override { jassertfalse; }

	void prepare(PrepareSpecs ps) final override;
	void reset() final override;
	void handleHiseEvent(HiseEvent& e) final override;

	int getBlockSizeForChildNodes() const override
	{
		return isBypassed() ? originalBlockSize : FixedBlockSize;
	}

	void updateBypassState(Identifier, var);

	wrap::fix_block<FixedBlockSize, DynamicSerialProcessor> obj;

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
	void process(ProcessDataDyn& data) final override;

	void processFrame(FrameType& data) final override
	{
		if (data.size() == 1) processFrameInternal(MonoFrameType::as(data.begin()));
		else if (data.size() == 2) processFrameInternal(StereoFrameType::as(data.begin()));
	}

	template <typename int C> void processFrameInternal(snex::Types::span<float, C>& data)
	{
		if (isBypassed())
			return;

		using ThisFrameType = snex::Types::span<float, C>;

		ThisFrameType original;
		data.copyTo(original);

		bool isFirst = true;

		for (auto n : nodes)
		{
			if (isFirst)
			{
				if (C == 1)
					n->processMonoFrame(MonoFrameType::as(data.begin()));
				if (C == 2)
					n->processStereoFrame(StereoFrameType::as(data.begin()));
					
				isFirst = false;
			}
			else
			{
				ThisFrameType wb;
				original.copyTo(wb);

				if (C == 1)
					n->processMonoFrame(MonoFrameType::as(data.begin()));
				if (C == 2)
					n->processStereoFrame(StereoFrameType::as(data.begin()));

				wb.addTo(data);
			}
		}
	}

	void processMonoFrame(MonoFrameType& data) final override;
	void processStereoFrame(StereoFrameType& data) final override;

	heap<float> original, workBuffer;
	
};


class MultiChannelNode : public ParallelNode
{
public:

	MultiChannelNode(DspNetwork* root, ValueTree data);;

	SCRIPTNODE_FACTORY(MultiChannelNode, "multi");

	void prepare(PrepareSpecs ps) final override;
	void reset() final override;
	void handleHiseEvent(HiseEvent& e) override;
	void processFrame(FrameType& data) final override;
	void process(ProcessDataDyn& d) final override;

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
	void process(ProcessDataDyn& data) final override;
	void processFrame(FrameType& data) final override;
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

	using FixProcessType = snex::Types::ProcessData<NumChannels>;
	using FixFrameType = snex::Types::span<float, NumChannels>;

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

	void process(ProcessDataDyn& data) final override
	{
		if (isBypassed())
			obj.getObject().process(data.as<FixProcessType>());
		else
		{
			
			float* channels[NumChannels];
			int numChannels = jmin(NumChannels, data.getNumChannels());
			memcpy(channels, data.getRawDataPointers(), numChannels * sizeof(float*));
			int numLeftOverChannels = NumChannels - data.getNumChannels();

			if (numLeftOverChannels > 0)
			{
				// fix channel mismatch
				jassert(leftoverBuffer.getNumChannels() == numLeftOverChannels);

				leftoverBuffer.clear();

				for (int i = 0; i < numLeftOverChannels; i++)
					channels[data.getNumChannels() + i] = leftoverBuffer.getWritePointer(i);
			}

			auto& cd = FixProcessType::ChannelDataType::as(channels);
			FixProcessType copy(cd.begin(), data.getNumSamples());
			copy.copyNonAudioDataFrom(data);
			obj.process(copy);
		}
	}

	void processFrame(FrameType& d) final override
	{
		jassert(d.size() == NumChannels);

		auto& s = FixFrameType::as(d.begin());
		obj.processFrame(s);
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
