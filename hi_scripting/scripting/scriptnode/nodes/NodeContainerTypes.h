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
	using InternalWrapper = bypass::smoothed<SerialNode::DynamicSerialProcessor>;

public:

	SCRIPTNODE_FACTORY(ChainNode, "chain");

	ChainNode(DspNetwork* n, ValueTree t);

	void process(ProcessDataDyn& data) final override;
	void processFrame(FrameType& data) final override;

	void processMonoFrame(MonoFrameType& data) final override;
	void processStereoFrame(StereoFrameType& data) final override;

	void prepare(PrepareSpecs ps) override;
	void handleHiseEvent(HiseEvent& e) final override;
	void reset() final override { wrapper.reset(); }

	Rectangle<int> getPositionInCanvas(Point<int> topLeft) const override
	{
 		return getBoundsToDisplay(getContainerPosition(isVertical.getValue(), topLeft));
	}

	NodeComponent* createComponent() override;

	NodePropertyT<bool> isVertical;

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

	Colour getContainerColour() const override {return JUCE_LIVE_CONSTANT_OFF(Colour(0xffbe952c)); }


	int getBlockSizeForChildNodes() const override;
	double getSampleRateForChildNodes() const override;

	//Rectangle<int> getPositionInCanvas(Point<int> topLeft) const override;

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
	
	Colour getContainerColour() const override {
		return Colour(MIDI_PROCESSOR_COLOUR);
	}

private:

	wrap::event<SerialNode::DynamicSerialProcessor> obj;
};

class NoMidiChainNode : public SerialNode
{
public:

	SCRIPTNODE_FACTORY(NoMidiChainNode, "no_midi");

	NoMidiChainNode(DspNetwork* n, ValueTree t);

	void processFrame(FrameType& data) noexcept final override;
	void process(ProcessDataDyn& data) noexcept final override;
	void prepare(PrepareSpecs ps) override;
	void handleHiseEvent(HiseEvent& e) final override;
	void reset() final override;

private:

	wrap::no_midi<SerialNode::DynamicSerialProcessor> obj;
};

class OfflineChainNode : public SerialNode
{
public:

	SCRIPTNODE_FACTORY(OfflineChainNode, "offline");

	OfflineChainNode(DspNetwork* n, ValueTree t);

	void processFrame(FrameType& data) noexcept final override;
	void process(ProcessDataDyn& data) noexcept final override;
	void prepare(PrepareSpecs ps) override;
	void handleHiseEvent(HiseEvent& e) final override;
	void reset() final override;

private:

	wrap::data<wrap::offline<SerialNode::DynamicSerialProcessor>, scriptnode::data::dynamic::audiofile> obj;
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
	PolyHandler* lastVoiceIndex = nullptr;
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

	void updateBypassState(Identifier, var)
	{
		if (originalBlockSize == 0)
			return;

		PrepareSpecs ps;
		ps.blockSize = originalBlockSize;
		ps.sampleRate = originalSampleRate;
		ps.numChannels = getCurrentChannelAmount();
		ps.voiceIndex = lastVoiceIndex;

		prepare(ps);
	}

	wrap::fix_block<FixedBlockSize, DynamicSerialProcessor> obj;

	valuetree::PropertyListener bypassListener;
	PolyHandler* lastVoiceIndex = nullptr;
};


class FixedBlockXNode : public SerialNode
{
public:

	struct DynamicBlockProperty
	{
		DynamicBlockProperty() :
			blockSizeString(PropertyIds::BlockSize, "64")
		{};

		void initialise(NodeBase* n)
		{
			parent = n;
			blockSizeString.initialise(n);
			blockSizeString.setAdditionalCallback(BIND_MEMBER_FUNCTION_2(DynamicBlockProperty::updateBlockSize), true);
		}

		void updateBlockSize(Identifier id, var newValue)
		{
			blockSize = newValue.toString().getIntValue();

			if (blockSize > 7 && isPowerOfTwo(blockSize))
			{
				SimpleReadWriteLock::ScopedWriteLock sl(parent->getRootNetwork()->getConnectionLock());
				parent->prepare(originalSpecs);
			}
			else
				blockSize = 64;
		}

		void prepare(void* obj, prototypes::prepare f, const PrepareSpecs& ps)
		{
			originalSpecs = ps;
			auto ps_ = ps.withBlockSize(blockSize);
			f(obj, &ps_);
		}

		template <typename ProcessDataType> void process(void* obj, prototypes::process<ProcessDataType> pf, ProcessDataType& data)
		{
			switch (blockSize)
			{
			case 8:   wrap::static_functions::fix_block<8>::process(obj, pf, data); break;
			case 16:  wrap::static_functions::fix_block<16>::process(obj, pf, data); break;
			case 32:  wrap::static_functions::fix_block<32>::process(obj, pf, data); break;
			case 64:  wrap::static_functions::fix_block<64>::process(obj, pf, data); break;
			case 128: wrap::static_functions::fix_block<128>::process(obj, pf, data); break;
			case 256: wrap::static_functions::fix_block<256>::process(obj, pf, data); break;
			case 512: wrap::static_functions::fix_block<512>::process(obj, pf, data); break;
			}
		}

		NodeBase::Ptr parent;
		NodePropertyT<String> blockSizeString;
		int blockSize = 64;
		PrepareSpecs originalSpecs;
	};
		

	SCRIPTNODE_FACTORY(FixedBlockXNode, "fix_blockx");

	FixedBlockXNode(DspNetwork* network, ValueTree d);

	void process(ProcessDataDyn& data) final override;

	void processFrame(FrameType& data) noexcept final override { jassertfalse; }

	void prepare(PrepareSpecs ps) final override;
	void reset() final override;
	void handleHiseEvent(HiseEvent& e) final override;

	int getBlockSizeForChildNodes() const override
	{
		return isBypassed() ? originalBlockSize : obj.fbClass.blockSize;
	}

	void updateBypassState(Identifier, var) 
	{
		if (originalBlockSize == 0)
			return;

		PrepareSpecs ps;
		ps.blockSize = originalBlockSize;
		ps.sampleRate = originalSampleRate;
		ps.numChannels = getCurrentChannelAmount();
		ps.voiceIndex = lastVoiceIndex;

		prepare(ps);
	}

	wrap::fix_blockx<DynamicSerialProcessor, DynamicBlockProperty> obj;

	valuetree::PropertyListener bypassListener;
	PolyHandler* lastVoiceIndex = nullptr;
};


class SplitNode : public ParallelNode
{
public:

	SplitNode(DspNetwork* root, ValueTree data);;

	SCRIPTNODE_FACTORY(SplitNode, "split");

	void prepare(PrepareSpecs ps) override;
	void reset() final override;
	void handleHiseEvent(HiseEvent& e) final override;
	void process(ProcessDataDyn& data) final override;

	void processFrame(FrameType& data) final override
	{
		if (data.size() == 1) processFrameInternal<1>(MonoFrameType::as(data.begin()));
		else if (data.size() == 2) processFrameInternal<2>(StereoFrameType::as(data.begin()));
	}

	template <int C> void processFrameInternal(snex::Types::span<float, C>& data)
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

	void reset() final override
	{
		obj.reset();
	}

	void prepare(PrepareSpecs ps) final override
	{
		NodeBase::prepare(ps);
		prepareNodes(ps);

		auto numLeftOverChannels = NumChannels - ps.numChannels;

		if (numLeftOverChannels <= 0)
			leftoverBuffer = {};
		else
			leftoverBuffer.setSize(numLeftOverChannels, ps.blockSize);
	}

	void process(ProcessDataDyn& data) final override
	{
		NodeProfiler np(this);

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
