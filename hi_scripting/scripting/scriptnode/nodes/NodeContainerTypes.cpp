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

	bypassListener.setCallback(t, { PropertyIds::Bypassed, PropertyIds::BypassRampTimeMs },
		valuetree::AsyncMode::Asynchronously,
		std::bind(&InternalWrapper::setBypassedFromValueTreeCallback, &wrapper, std::placeholders::_1, std::placeholders::_2));
}

void ChainNode::process(ProcessData& data)
{
	wrapper.process(data);
}


void ChainNode::processSingle(float* frameData, int numChannels)
{
	wrapper.processSingle(frameData, numChannels);
}

void ChainNode::prepare(PrepareSpecs ps)
{
	ScopedLock sl(getRootNetwork()->getConnectionLock());

	NodeContainer::prepareNodes(ps);
	wrapper.prepare(ps);
}

void ChainNode::handleHiseEvent(HiseEvent& e)
{
	wrapper.handleHiseEvent(e);
}

String ChainNode::getCppCode(CppGen::CodeLocation location)
{
	if (location == CppGen::CodeLocation::Definitions)
	{
		String s = NodeContainer::getCppCode(location);
		CppGen::Emitter::emitDefinition(s, "SET_HISE_NODE_IS_MODULATION_SOURCE", "false", false);

		return s;
	}
	else
		return SerialNode::getCppCode(location);
}

SplitNode::SplitNode(DspNetwork* root, ValueTree data) :
	ParallelNode(root, data)
{
	initListeners();
}

void SplitNode::prepare(PrepareSpecs ps)
{
	ScopedLock sl(getRootNetwork()->getConnectionLock());

	NodeContainer::prepareNodes(ps);

	ps.numChannels *= 2;
	DspHelpers::increaseBuffer(splitBuffer, ps);
}

juce::String SplitNode::getCppCode(CppGen::CodeLocation location)
{
	return ParallelNode::getCppCode(location);
}


void SplitNode::handleHiseEvent(HiseEvent& e)
{
	for (auto n : nodes)
	{
		HiseEvent copy(e);
		n->handleHiseEvent(e);
	}
}

void SplitNode::process(ProcessData& data)
{
	if (isBypassed())
		return;

	auto original = data.copyTo(splitBuffer, 0);
	int channelCounter = 0;

	for (auto n : nodes)
	{
		if (channelCounter++ == 0)
		{
			if (n->isBypassed())
				continue;

			n->process(data);
		}
		else
		{
			if (n->isBypassed())
				continue;

			auto wd = original.copyTo(splitBuffer, 1);
			n->process(wd);
			data += wd;
		}
	}
}


void SplitNode::processSingle(float* frameData, int numChannels)
{
	if (isBypassed())
		return;

	float original[NUM_MAX_CHANNELS];
	memcpy(original, frameData, sizeof(float)*numChannels);
	bool isFirst = true;

	for (auto n : nodes)
	{
		if (isFirst)
		{
			n->processSingle(frameData, numChannels);
			isFirst = false;
		}
		else
		{
			float wb[NUM_MAX_CHANNELS];
			memcpy(wb, original, sizeof(float)*numChannels);
			n->processSingle(wb, numChannels);
			FloatVectorOperations::add(frameData, wb, numChannels);
		}
	}
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

void ModulationChainNode::processSingle(float* frameData, int ) noexcept
{
	if (isBypassed())
		return;

	obj.processSingle(frameData, 1);
}

void ModulationChainNode::process(ProcessData& data) noexcept
{
	if (isBypassed())
		return;

	ProcessData copy(data);
	copy.numChannels = 1;

	obj.process(copy);
	double thisValue = 0.0;
	obj.handleModulation(thisValue);
}

void ModulationChainNode::prepare(PrepareSpecs ps)
{
	ScopedLock sl(getRootNetwork()->getConnectionLock());

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

juce::String ModulationChainNode::createCppClass(bool isOuterClass)
{
	return createCppClassForNodes(isOuterClass);
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
void OversampleNode<OversampleFactor>::process(ProcessData& d) noexcept
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
void FixedBlockNode<B>::updateBypassState(Identifier, var)
{
	if (originalBlockSize == 0)
		return;

	PrepareSpecs ps;
	ps.blockSize = originalBlockSize;
	ps.sampleRate = originalSampleRate;
	ps.numChannels = getNumChannelsToProcess();
	ps.voiceIndex = lastVoiceIndex;

	prepare(ps);
}

template <int B>
void FixedBlockNode<B>::process(ProcessData& d)
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

template <int B>
void FixedBlockNode<B>::prepare(PrepareSpecs ps)
{
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

void MultiChannelNode::processSingle(float* frameData, int )
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

void MultiChannelNode::process(ProcessData& d)
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

SingleSampleBlockX::SingleSampleBlockX(DspNetwork* n, ValueTree d) :
	SerialNode(n, d)
{
	initListeners();
	obj.getObject().initialise(this);
}

void SingleSampleBlockX::prepare(PrepareSpecs ps)
{
	ScopedLock sl(getRootNetwork()->getConnectionLock());
	prepareNodes(ps);
}

void SingleSampleBlockX::reset()
{
	obj.reset();
}

void SingleSampleBlockX::process(ProcessData& data)
{
	if (isBypassed())
		obj.getObject().process(data);
	else
		obj.process(data);
}

void SingleSampleBlockX::processSingle(float* frameData, int numChannels)
{
	obj.processSingle(frameData, numChannels);
}

int SingleSampleBlockX::getBlockSizeForChildNodes() const
{
	return isBypassed() ? originalBlockSize : 1;
}

void SingleSampleBlockX::handleHiseEvent(HiseEvent& e)
{
	obj.handleHiseEvent(e);
}

juce::String SingleSampleBlockX::getCppCode(CppGen::CodeLocation location)
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

MidiChainNode::MidiChainNode(DspNetwork* n, ValueTree t):
	SerialNode(n, t)
{
	initListeners();
	obj.getObject().initialise(this);
}

void MidiChainNode::processSingle(float* frameData, int numChannels) noexcept
{
	obj.processSingle(frameData, numChannels);
}

void MidiChainNode::process(ProcessData& data) noexcept
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

juce::String MidiChainNode::getCppCode(CppGen::CodeLocation location)
{
	if (location == CppGen::CodeLocation::Definitions)
	{
		String s;
		s << SerialNode::getCppCode(location);
		CppGen::Emitter::emitDefinition(s, "SET_HISE_NODE_IS_MODULATION_SOURCE", "false", false);
		return s;
	}
    else
        return SerialNode::getCppCode(location);
}

}
