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
 *   which also must be licensed for commercial applications:
 *
 *   http://www.juce.com
 *
 *   ===========================================================================
 */

namespace scriptnode
{
using namespace juce;
using namespace hise;


ChainNode::ChainNode(DspNetwork* n, ValueTree t) :
	SerialNode(n, t)
{
	initListeners();

	wrapper.getObject().initialise(this);

	setDefaultValue(PropertyIds::BypassRampTimeMs, 20.0);

#if 0
	bypassListener.setCallback(t, { PropertyIds::Bypassed, PropertyIds::BypassRampTimeMs },
		valuetree::AsyncMode::Asynchronously,
		std::bind(&InternalWrapper::setBypassedFromValueTreeCallback, &wrapper, std::placeholders::_1, std::placeholders::_2));
#endif
}



void ChainNode::process(ProcessDataDyn& data)
{
	NodeProfiler np(this);
	wrapper.process(data);
}


void ChainNode::processFrame(NodeBase::FrameType& data)
{
	if (data.size() == 1)
		processMonoFrame(MonoFrameType::as(data.begin()));
	if(data.size() == 2)
		processStereoFrame(StereoFrameType::as(data.begin()));

	wrapper.processFrame(data);
}

void ChainNode::processMonoFrame(MonoFrameType& data)
{
	wrapper.processFrame(data);
}

void ChainNode::processStereoFrame(StereoFrameType& data)
{
	wrapper.processFrame(data);
}

void ChainNode::prepare(PrepareSpecs ps)
{
	ScopedLock sl(getRootNetwork()->getConnectionLock());

	NodeBase::prepare(ps);
	NodeContainer::prepareNodes(ps);
	wrapper.prepare(ps);
}

void ChainNode::handleHiseEvent(HiseEvent& e)
{
	wrapper.handleHiseEvent(e);
}

SplitNode::SplitNode(DspNetwork* root, ValueTree data) :
	ParallelNode(root, data)
{
	initListeners();
}

void SplitNode::prepare(PrepareSpecs ps)
{
	ScopedLock sl(getRootNetwork()->getConnectionLock());

	NodeBase::prepare(ps);
	NodeContainer::prepareNodes(ps);

	if (ps.blockSize > 1)
	{
		DspHelpers::increaseBuffer(original, ps);
		DspHelpers::increaseBuffer(workBuffer, ps);
	}
}

void SplitNode::handleHiseEvent(HiseEvent& e)
{
	for (auto n : nodes)
	{
		HiseEvent copy(e);
		n->handleHiseEvent(e);
	}
}

void SplitNode::process(ProcessDataDyn& data)
{
	if (isBypassed() || original.begin() == nullptr)
		return;

	NodeProfiler np(this);

	float* ptrs[NUM_MAX_CHANNELS];
	int numSamples = data.getNumSamples();

	{
		float* optr = original.begin();
		float* wptr = workBuffer.begin();

		int index = 0;
		for (auto& c : data)
		{
			FloatVectorOperations::copy(optr, c.getRawReadPointer(), numSamples);
			ptrs[index++] = wptr;
			optr += numSamples;
			wptr += numSamples;
		}
	}
	
	int channelCounter = 0;

	for (auto n : nodes)
	{
		if (n->isBypassed())
			continue;

		if (channelCounter++ == 0)
		{
			n->process(data);
		}
		else
		{
			int numToCopy = data.getNumSamples() * data.getNumChannels();

			FloatVectorOperations::copy(workBuffer.begin(), original.begin(), numToCopy);
			ProcessDataDyn cp(ptrs, numSamples, data.getNumChannels());

			n->process(cp);

			int index = 0;
			for (auto& c : data)
				FloatVectorOperations::add(c.getRawWritePointer(), ptrs[index++], numSamples);
		}
	}
}

void SplitNode::processMonoFrame(MonoFrameType& data)
{
	processFrameInternal<1>(data);
}

void SplitNode::processStereoFrame(StereoFrameType& data)
{
	processFrameInternal<2>(data);
}

void SplitNode::reset()
{
	resetNodes();
}

ModulationChainNode::ModulationChainNode(DspNetwork* n, ValueTree t) :
	SerialNode(n, t)
{
	initListeners();
	obj.initialise(this);
}

void ModulationChainNode::processFrame(NodeBase::FrameType& d) noexcept
{
	if (!isBypassed())
		obj.processFrame(d);
}

void ModulationChainNode::process(ProcessDataDyn& data) noexcept
{
	if (isBypassed())
		return;

	NodeProfiler np(this);

	obj.process(data);
	double thisValue = 0.0;
}

void ModulationChainNode::prepare(PrepareSpecs ps)
{
	
	ScopedLock sl(getRootNetwork()->getConnectionLock());

	DspHelpers::setErrorIfFrameProcessing(ps);
	DspHelpers::setErrorIfNotOriginalSamplerate(ps, this);

	NodeBase::prepare(ps);
	NodeContainer::prepareNodes(ps);
	obj.prepare(ps);
}

void ModulationChainNode::handleHiseEvent(HiseEvent& e)
{
	obj.handleHiseEvent(e);
}

void ModulationChainNode::reset()
{
	NodeContainer::resetNodes();
	obj.reset();
}

scriptnode::NodeComponent* ModulationChainNode::createComponent()
{
	return new ModChainNodeComponent(this);
}

int ModulationChainNode::getBlockSizeForChildNodes() const
{
	return jmax(1, originalBlockSize / HISE_EVENT_RASTER);
}

double ModulationChainNode::getSampleRateForChildNodes() const
{
	return originalSampleRate / (double)HISE_EVENT_RASTER;
}

juce::Rectangle<int> ModulationChainNode::getPositionInCanvas(Point<int> topLeft) const
{
	using namespace UIValues;

	const int minWidth = NodeWidth;
	
	int maxW = minWidth;
	int h = 0;

	h += UIValues::NodeMargin;
	h += UIValues::HeaderHeight; // the input

	if (v_data[PropertyIds::ShowParameters])
		h += UIValues::ParameterHeight;

	h += PinHeight; // the "hole" for the cable

	Point<int> childPos(NodeMargin, NodeMargin);

	for (auto n : nodes)
	{
		auto bounds = n->getPositionInCanvas(childPos);

		bounds = n->getBoundsToDisplay(bounds);

		maxW = jmax<int>(maxW, bounds.getWidth());
		h += bounds.getHeight() + NodeMargin;
		childPos = childPos.translated(0, bounds.getHeight());
	}

	h += PinHeight; // the "hole" for the cable

	return { topLeft.getX(), topLeft.getY(), maxW + 2 * NodeMargin, h };
}


template <int OversampleFactor>
OversampleNode<OversampleFactor>::OversampleNode(DspNetwork* network, ValueTree d) :
	SerialNode(network, d)
{
	initListeners();

	obj.initialise(this);

	bypassListener.setCallback(d, { PropertyIds::Bypassed },
		valuetree::AsyncMode::Synchronously,
		BIND_MEMBER_FUNCTION_2(OversampleNode<OversampleFactor>::updateBypassState));
}

template <int OversampleFactor>
void OversampleNode<OversampleFactor>::updateBypassState(Identifier, var)
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

template <int OversampleFactor>
void OversampleNode<OversampleFactor>::prepare(PrepareSpecs ps)
{
	DspHelpers::setErrorIfFrameProcessing(ps);
	DspHelpers::setErrorIfNotOriginalSamplerate(ps, this);

	NodeBase::prepare(ps);
	lastVoiceIndex = ps.voiceIndex;
	prepareNodes(ps);

	if (isBypassed())
		obj.getObject().prepare(ps);
	else
		obj.prepare(ps);
}

template <int OversampleFactor>
double OversampleNode<OversampleFactor>::getSampleRateForChildNodes() const
{
	return isBypassed() ? originalSampleRate : originalSampleRate * OversampleFactor;
}

template <int OversampleFactor>
int OversampleNode<OversampleFactor>::getBlockSizeForChildNodes() const
{
	return isBypassed() ? originalBlockSize : originalBlockSize * OversampleFactor;
}

template <int OversampleFactor>
void OversampleNode<OversampleFactor>::reset()
{
	obj.reset();
}

template <int OversampleFactor>
void OversampleNode<OversampleFactor>::handleHiseEvent(HiseEvent& e)
{
	obj.handleHiseEvent(e);
}

template <int OversampleFactor>
void OversampleNode<OversampleFactor>::process(ProcessDataDyn& d) noexcept
{
	NodeProfiler np(this);

	if (isBypassed())
	{
		obj.getObject().process(d);
	}
	else
	{
		obj.process(d);
	}
}

template class OversampleNode<2>;
template class OversampleNode<4>;
template class OversampleNode<8>;
template class OversampleNode<16>;


template <int B>
FixedBlockNode<B>::FixedBlockNode(DspNetwork* network, ValueTree d):
SerialNode(network, d)
{
	initListeners();

	obj.initialise(this);

	bypassListener.setCallback(d, { PropertyIds::Bypassed },
		valuetree::AsyncMode::Synchronously,
		BIND_MEMBER_FUNCTION_2(FixedBlockNode<B>::updateBypassState));
}



template <int B>
void FixedBlockNode<B>::process(ProcessDataDyn& d)
{
	NodeProfiler np(this);

	if (isBypassed())
	{
		obj.getObject().process(d);
	}
	else
	{
		obj.process(d);
	}
}

template <int B>
void FixedBlockNode<B>::prepare(PrepareSpecs ps)
{
	DspHelpers::setErrorIfFrameProcessing(ps);

	NodeBase::prepare(ps);
	lastVoiceIndex = ps.voiceIndex;
	prepareNodes(ps);

	if (isBypassed())
		obj.getObject().prepare(ps);
	else
		obj.prepare(ps);
}

template <int B>
void FixedBlockNode<B>::reset()
{
	obj.reset();
}


template <int B>
void FixedBlockNode<B>::handleHiseEvent(HiseEvent& e)
{
	obj.handleHiseEvent(e);
}

template class FixedBlockNode<8>;
template class FixedBlockNode<16>;
template class FixedBlockNode<32>;
template class FixedBlockNode<64>;
template class FixedBlockNode<128>;
template class FixedBlockNode<256>;

MultiChannelNode::MultiChannelNode(DspNetwork* root, ValueTree data) :
	ParallelNode(root, data)
{
	initListeners();
}

void MultiChannelNode::channelLayoutChanged(NodeBase* nodeThatCausedLayoutChange)
{
	int numChannelsAvailable = getNumChannelsToProcess();
	int numNodes = nodes.size();

	if (numNodes == 0)
		return;

	// Use the ones with locked channel amounts first
	for (auto n : nodes)
	{
		if (n->hasFixChannelAmount())
		{
			numChannelsAvailable -= n->getNumChannelsToProcess();
			numNodes--;
		}
	}

	if (numNodes > 0)
	{
		int numPerNode = numChannelsAvailable / numNodes;

		for (auto n : nodes)
		{
			if (n->hasFixChannelAmount())
				continue;

			int thisNum = jmax(0, jmin(numChannelsAvailable, numPerNode));

			if (n != nodeThatCausedLayoutChange)
				n->setNumChannels(thisNum);

			numChannelsAvailable -= n->getNumChannelsToProcess();
		}
	}
}

void MultiChannelNode::prepare(PrepareSpecs ps)
{
	ScopedLock sl(getRootNetwork()->getConnectionLock());

	auto numNodes = this->nodes.size();
	auto numChannels = ps.numChannels;

	if (numNodes > numChannels)
	{
		Error e;
		e.error = Error::TooManyChildNodes;
		e.expected = numChannels;
		e.actual = numNodes;
		getRootNetwork()->getExceptionHandler().addError(this, e);
	}
		

	NodeBase::prepare(ps);
	NodeContainer::prepareContainer(ps);
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

		ps.numChannels = numChannelsThisTime;

		nodes[i]->prepare(ps);

		channelRanges[i] = { startChannel, endChannel };
		channelIndex += numChannelsThisTime;
	}
}

void MultiChannelNode::reset()
{
	NodeContainer::resetNodes();
}

void MultiChannelNode::handleHiseEvent(HiseEvent& e)
{
	for (auto n : nodes)
	{
		HiseEvent c(e);
		n->handleHiseEvent(c);
	}
}

void MultiChannelNode::processFrame(NodeBase::FrameType& data)
{
	for (int i = 0; i < nodes.size(); i++)
	{
		auto& r = channelRanges[i];

		if (r.getLength() == 0)
			continue;

		float* d = data.data + r.getStart();
		int numThisThime = r.getLength();
		FrameType md(d, numThisThime);
		nodes[i]->processFrame(md);
	}
}

void MultiChannelNode::process(ProcessDataDyn& d)
{
	NodeProfiler np(this);

	int channelIndex = 0;

	for (auto n : nodes)
	{
		int numChannelsThisTime = n->getNumChannelsToProcess();
		int startChannel = channelIndex;
		int endChannel = startChannel + numChannelsThisTime;

		if (endChannel <= d.getNumChannels())
		{
			for (int i = 0; i < numChannelsThisTime; i++)
				currentChannelData[i] = d[startChannel + i].data;

			ProcessDataDyn td(currentChannelData, d.getNumSamples(), numChannelsThisTime);
			td.copyNonAudioDataFrom(d);
			n->process(td);
		}

		channelIndex += numChannelsThisTime;
	}
}

SingleSampleBlockX::SingleSampleBlockX(DspNetwork* n, ValueTree d) :
	SerialNode(n, d)
{
	initListeners();
	obj.getObject().initialise(this);
}

void SingleSampleBlockX::prepare(PrepareSpecs ps)
{
	ScopedLock sl(getRootNetwork()->getConnectionLock());

	NodeBase::prepare(ps);
	prepareNodes(ps);
}

void SingleSampleBlockX::reset()
{
	obj.reset();
}

void SingleSampleBlockX::process(ProcessDataDyn& data)
{
	NodeProfiler np(this);

	if (isBypassed())
		obj.getObject().process(data);
	else
		obj.process(data);
}

void SingleSampleBlockX::processFrame(NodeBase::FrameType& data)
{
	obj.processFrame(data);
}

int SingleSampleBlockX::getBlockSizeForChildNodes() const
{
	return isBypassed() ? originalBlockSize : 1;
}

void SingleSampleBlockX::handleHiseEvent(HiseEvent& e)
{
	obj.handleHiseEvent(e);
}

MidiChainNode::MidiChainNode(DspNetwork* n, ValueTree t):
	SerialNode(n, t)
{
	initListeners();
	obj.getObject().initialise(this);
}

void MidiChainNode::processFrame(NodeBase::FrameType& data) noexcept
{
	obj.processFrame(data);
}

void MidiChainNode::process(ProcessDataDyn& data) noexcept
{
	if (isBypassed())
	{
		obj.getObject().process(data);
	}
	else
	{
		obj.process(data);
	}
}

void MidiChainNode::prepare(PrepareSpecs ps)
{
	ScopedLock sl(getRootNetwork()->getConnectionLock());

	DspHelpers::setErrorIfFrameProcessing(ps);
	DspHelpers::setErrorIfNotOriginalSamplerate(ps, this);

	NodeBase::prepare(ps);
	prepareNodes(ps);
}

void MidiChainNode::handleHiseEvent(HiseEvent& e)
{
	obj.handleHiseEvent(e);
}

void MidiChainNode::reset()
{
	obj.reset();
}

OfflineChainNode::OfflineChainNode(DspNetwork* n, ValueTree t) :
	SerialNode(n, t)
{
	initListeners();
	obj.initialise(this);
}

void OfflineChainNode::processFrame(FrameType& data) noexcept
{
}

void OfflineChainNode::process(ProcessDataDyn& data) noexcept
{
}

void OfflineChainNode::prepare(PrepareSpecs ps)
{
}

void OfflineChainNode::handleHiseEvent(HiseEvent& e)
{
}

void OfflineChainNode::reset()
{
}

FixedBlockXNode::FixedBlockXNode(DspNetwork* network, ValueTree d) :
	SerialNode(network, d)
{
	initListeners();
	obj.initialise(this);
}

void FixedBlockXNode::process(ProcessDataDyn& data)
{
	NodeProfiler np(this);
	obj.process(data);
}

void FixedBlockXNode::prepare(PrepareSpecs ps)
{
	NodeBase::prepare(ps);
	obj.prepare(ps);
}

void FixedBlockXNode::reset()
{
	obj.reset();
}

void FixedBlockXNode::handleHiseEvent(HiseEvent& e)
{
	obj.handleHiseEvent(e);
}

}
