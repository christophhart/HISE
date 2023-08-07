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
}



void ChainNode::process(ProcessDataDyn& data)
{
	NodeProfiler np(this, data.getNumSamples());
	ProcessDataPeakChecker pd(this, data);
    TRACE_DSP();
    
	if (isBypassed())
		return;

	wrapper.process(data);
}


void ChainNode::processFrame(NodeBase::FrameType& data)
{
	if (isBypassed())
		return;

	FrameDataPeakChecker fd(this, data.begin(), data.size());

	if (data.size() == 1)
		processMonoFrame(MonoFrameType::as(data.begin()));
	if(data.size() == 2)
		processStereoFrame(StereoFrameType::as(data.begin()));
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
	NodeBase::prepare(ps);
	NodeContainer::prepareNodes(ps);
	wrapper.prepare(ps);
}

void ChainNode::handleHiseEvent(HiseEvent& e)
{
	if (isBypassed())
		return;

	wrapper.handleHiseEvent(e);
}

SplitNode::SplitNode(DspNetwork* root, ValueTree data) :
	ParallelNode(root, data)
{
	initListeners();
}

void SplitNode::prepare(PrepareSpecs ps)
{
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
	if (isBypassed())
		return;

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

	NodeProfiler np(this, data.getNumSamples());
    ProcessDataPeakChecker pd(this, data);
    TRACE_DSP();
    
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

			cp.copyNonAudioDataFrom(data);

			n->process(cp);

			int index = 0;
			for (auto& c : data)
				FloatVectorOperations::add(c.getRawWritePointer(), ptrs[index++], numSamples);
		}
	}
}

void SplitNode::processMonoFrame(MonoFrameType& data)
{
	FrameDataPeakChecker fd(this, data.begin(), data.size());
	processFrameInternal<1>(data);
}

void SplitNode::processStereoFrame(StereoFrameType& data)
{
	FrameDataPeakChecker fd(this, data.begin(), data.size());
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

void ModulationChainNode::processFrame(NodeBase::FrameType& data) noexcept
{
	FrameDataPeakChecker fd(this, data.begin(), data.size());

	if (!isBypassed())
		obj.processFrame(data);
}

void ModulationChainNode::process(ProcessDataDyn& data) noexcept
{
	if (isBypassed())
		return;

	NodeProfiler np(this, data.getNumSamples());
    TRACE_DSP();
    
	obj.process(data);
}

void ModulationChainNode::prepare(PrepareSpecs ps)
{
	isProcessingFrame = ps.blockSize == 1;

	ps.numChannels = 1;

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



int ModulationChainNode::getBlockSizeForChildNodes() const
{
	return isProcessingFrame ? originalBlockSize : jmax(1, originalBlockSize / HISE_EVENT_RASTER);
}

double ModulationChainNode::getSampleRateForChildNodes() const
{
	return isProcessingFrame ? originalSampleRate : originalSampleRate / (double)HISE_EVENT_RASTER;
}

template <int OversampleFactor>
OversampleNode<OversampleFactor>::OversampleNode(DspNetwork* network, ValueTree d) :
	SerialNode(network, d)
{
	initListeners(false);

	addFixedParameters();

	

	

	obj.initialise(this);
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
		obj.getWrappedObject().prepare(ps);
	else
		obj.prepare(ps);
}

template <int OversampleFactor>
void OversampleNode<OversampleFactor>::setBypassed(bool shouldBeBypassed)
{
	SerialNode::setBypassed(shouldBeBypassed);

	if (originalBlockSize == 0 || originalSampleRate == 0.0)
		return;

	PrepareSpecs ps;
	ps.blockSize = originalBlockSize;
	ps.sampleRate = originalSampleRate;
	ps.numChannels = getCurrentChannelAmount();
	ps.voiceIndex = lastVoiceIndex;

	prepare(ps);

	getRootNetwork()->runPostInitFunctions();
}

template <int OversampleFactor>
double OversampleNode<OversampleFactor>::getSampleRateForChildNodes() const
{
	auto os = isBypassed() ? 1 : obj.getOverSamplingFactor();
	return originalSampleRate * os;
}

template <int OversampleFactor>
int OversampleNode<OversampleFactor>::getBlockSizeForChildNodes() const
{
	auto os = isBypassed() ? 1 : obj.getOverSamplingFactor();
	return originalBlockSize * os;
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
	ProcessDataPeakChecker pd(this, d);
    TRACE_DSP();
    
	if (isBypassed())
	{
		NodeProfiler np(this, d.getNumSamples());
		obj.getWrappedObject().process(d);
	}
	else
	{
		if (hasFixedParameters())
		{
#if USE_BACKEND
			auto os = jlimit(1, 16, (int)std::pow(2.0, asNode()->getParameterFromIndex(0)->getValue()));
			NodeProfiler np(this, d.getNumSamples() * os);
#endif
			obj.process(d);
		}
		else
		{
			NodeProfiler np(this, d.getNumSamples() * OversampleFactor);
			obj.process(d);
		}
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
}



template <int B>
void FixedBlockNode<B>::process(ProcessDataDyn& d)
{
    TRACE_DSP();
    
	if (isBypassed())
	{
		NodeProfiler np(this, d.getNumSamples());
		ProcessDataPeakChecker pd(this, d);
		
		obj.getObject().process(d);
	}
	else
	{
		NodeProfiler np(this, B);
		ProcessDataPeakChecker pd(this, d);
		
		obj.process(d);
	}
}



template <int B>
void scriptnode::FixedBlockNode<B>::processFrame(FrameType& data) noexcept
{
	FrameDataPeakChecker fd(this, data.begin(), data.size());

	if (data.size() == 1)
		processMonoFrame(MonoFrameType::as(data.begin()));
	if (data.size() == 2)
		processStereoFrame(StereoFrameType::as(data.begin()));
}


template <int B>
void scriptnode::FixedBlockNode<B>::processStereoFrame(StereoFrameType& data)
{
	obj.processFrame(data);
}

template <int B>
void scriptnode::FixedBlockNode<B>::processMonoFrame(MonoFrameType& data)
{
	obj.processFrame(data);
}


template <int B>
void FixedBlockNode<B>::setBypassed(bool shouldBeBypassed)
{
	SerialNode::setBypassed(shouldBeBypassed);

	if (originalBlockSize == 0)
		return;

	PrepareSpecs ps;
	ps.blockSize = originalBlockSize;
	ps.sampleRate = originalSampleRate;
	ps.numChannels = getCurrentChannelAmount();
	ps.voiceIndex = lastVoiceIndex;

	prepare(ps);
	getRootNetwork()->runPostInitFunctions();
}

template <int B>
void FixedBlockNode<B>::prepare(PrepareSpecs ps)
{
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
	int numNodes = nodes.size();

	if (numNodes == 0)
		return;
}

void MultiChannelNode::prepare(PrepareSpecs ps)
{
	auto numNodes = this->nodes.size();
	auto numChannels = ps.numChannels;

	getRootNetwork()->getExceptionHandler().removeError(this);

	if (numNodes > numChannels)
		Error::throwError(Error::TooManyChildNodes, numChannels, numNodes);

	int numPerChildren = jmax(1,  numNodes > 0 ? numChannels / numNodes : 0);
	
	NodeBase::prepare(ps);
	NodeContainer::prepareContainer(ps);
	int channelIndex = 0;

	for (int i = 0; i < NUM_MAX_CHANNELS; i++)
	{
		channelRanges[i] = {};
	}

	for (int i = 0; i < jmin(NUM_MAX_CHANNELS, nodes.size()); i++)
	{
		int numChannelsThisTime = numPerChildren;
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
	FrameDataPeakChecker fd(this, data.begin(), data.size());

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
	NodeProfiler np(this, d.getNumSamples());
    ProcessDataPeakChecker pd(this, d);
    TRACE_DSP();
    
	int channelIndex = 0;

	for (auto n : nodes)
	{
		int numChannelsThisTime = n->getCurrentChannelAmount();
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

void SingleSampleBlockX::setBypassed(bool shouldBeBypassed)
{
	SerialNode::setBypassed(shouldBeBypassed);

	if (originalBlockSize == 0 || originalSampleRate == 0.0)
		return;

	PrepareSpecs ps;
	ps.blockSize = originalBlockSize;
	ps.sampleRate = originalSampleRate;
	ps.numChannels = getCurrentChannelAmount();
	ps.voiceIndex = lastVoiceIndex;

	prepare(ps);

	getRootNetwork()->runPostInitFunctions();
}

void SingleSampleBlockX::prepare(PrepareSpecs ps)
{
	NodeBase::prepare(ps);
	prepareNodes(ps);
}

void SingleSampleBlockX::reset()
{
	obj.reset();
}

void SingleSampleBlockX::process(ProcessDataDyn& data)
{
	NodeProfiler np(this, isBypassed() ? data.getNumSamples() : 1);
	ProcessDataPeakChecker pd(this, data);


	if (isBypassed())
		obj.getObject().process(data);
	else
		obj.process(data);
}

void SingleSampleBlockX::processFrame(NodeBase::FrameType& data)
{
	FrameDataPeakChecker fd(this, data.begin(), data.size());
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

SidechainNode::SidechainNode(DspNetwork* n, ValueTree d) :
    SerialNode(n, d)
{
    initListeners();
    obj.getObject().initialise(this);
}

void SidechainNode::prepare(PrepareSpecs ps)
{
    obj.prepare(ps);
    NodeBase::prepare(ps);
    ps.numChannels *= 2;
    prepareNodes(ps);
}

void SidechainNode::reset()
{
    obj.reset();
}

void SidechainNode::process(ProcessDataDyn& data)
{
    NodeProfiler np(this, isBypassed() ? data.getNumSamples() : 1);
    ProcessDataPeakChecker pd(this, data);
    obj.process(data);
}

void SidechainNode::processFrame(NodeBase::FrameType& data)
{
    FrameDataPeakChecker fd(this, data.begin(), data.size());
    obj.processFrame(data);
}

int SidechainNode::getBlockSizeForChildNodes() const
{
    return originalBlockSize;
}

void SidechainNode::handleHiseEvent(HiseEvent& e)
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
	NodeProfiler np(this, isBypassed() ? data.getNumSamples() : 1);
	ProcessDataPeakChecker pd(this, data);

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
	FrameDataPeakChecker pd(this, data.begin(), data.size());
}

void OfflineChainNode::process(ProcessDataDyn& data) noexcept
{
	NodeProfiler np(this, isBypassed() ? data.getNumSamples() : 1);
	ProcessDataPeakChecker pd(this, data);
}

void OfflineChainNode::prepare(PrepareSpecs ps)
{
	NodeBase::prepare(ps);
	NodeContainer::prepareNodes(ps);
}

void OfflineChainNode::handleHiseEvent(HiseEvent& e)
{
}

void OfflineChainNode::reset()
{
	obj.reset();
}

struct CloneOptionComponent : public Component,
	public PathFactory,
	public ButtonListener
{
	CloneOptionComponent(CloneNode* n) :
		parent(n),
		hideButton("hide", this, *this),
		duplicateButton("duplicate", this, *this),
		deleteButton("delete", this, *this)
	{
		hideButton.setToggleModeWithColourChange(true);

		hideButton.setToggleStateAndUpdateIcon(parent->getValueTree()[PropertyIds::ShowClones]);

		addAndMakeVisible(hideButton);
		addAndMakeVisible(duplicateButton);
		addAndMakeVisible(deleteButton);

		setSize(24, 90);
	}

	void buttonClicked(Button* b) override
	{
		if (b == &hideButton)
		{
			parent->getValueTree().setProperty(PropertyIds::ShowClones, hideButton.getToggleState(), parent->getUndoManager());
		}
		if (b == &deleteButton)
		{
			
			auto network = parent->getRootNetwork();
			parent->getValueTree().removeProperty(PropertyIds::DisplayedClones, parent->getUndoManager());

			SimpleReadWriteLock::ScopedWriteLock sl(network->getConnectionLock());

			auto nt = dynamic_cast<NodeContainer*>(parent.get())->getNodeTree();
			StringArray nodesToRemove;

			while (nt.getNumChildren() > 1)
			{
				nodesToRemove.add(nt.getChild(1)[PropertyIds::ID].toString());
				nt.removeChild(1, nullptr);
			}

			MessageManager::callAsync([nodesToRemove, network]()
			{
				for (auto nid : nodesToRemove)
					network->deleteIfUnused(nid);
			});

		}
		if (b == &duplicateButton)
		{
			auto parentNode = parent.get();

			deleteButton.triggerClick(sendNotificationSync);

			auto numToCloneString = PresetHandler::getCustomName("NumClones", "Enter the number of clones you want to create");

			SimpleReadWriteLock::ScopedWriteLock sl(parentNode->getRootNetwork()->getConnectionLock());

            Array<DspNetwork::IdChange> changes;
            
			auto numToAdd = jlimit(1, 128, numToCloneString.getIntValue());

			while (numToAdd > 1)
			{
				auto nt = dynamic_cast<NodeContainer*>(parentNode)->getNodeTree();

				auto copy = parentNode->getRootNetwork()->cloneValueTreeWithNewIds(nt.getChild(0), changes, true);
				parentNode->getRootNetwork()->createFromValueTree(true, copy, true);
				nt.addChild(copy, -1, parentNode->getUndoManager());
				numToAdd--;
			}
		}
	}

	Path createPath(const String& url) const override
	{
		Path p;

#if USE_BACKEND
		LOAD_EPATH_IF_URL("hide", BackendBinaryData::ToolbarIcons::viewPanel);
		LOAD_EPATH_IF_URL("duplicate", SampleMapIcons::duplicateSamples);
		LOAD_EPATH_IF_URL("delete", SampleMapIcons::deleteSamples);
#endif

		return p;
	}

	void resized() override
	{
		auto b = getLocalBounds();
		duplicateButton.setBounds(b.removeFromTop(getWidth()).reduced(2));
		hideButton.setBounds(b.removeFromTop(getWidth()).reduced(2));
		deleteButton.setBounds(b.removeFromTop(getWidth()).reduced(2));
	}

	void paint(Graphics& g) override
	{

	}

	NodeBase::Ptr parent;

	HiseShapeButton hideButton, duplicateButton, deleteButton;
};


CloneNode::CloneIterator::CloneIterator(CloneNode& n, const ValueTree& v, bool skipOriginal) :
	cn(n),
	original(v),
	path(cn.getPathForValueTree(original))
{
	auto root = n.getNodeTree();

	for (int i = 0; i < root.getNumChildren(); i++)
	{
		Array<int> tp;
		tp.addArray(path, 1, path.size() - 1);
		auto ch = cn.getValueTreeForPath(root.getChild(i), tp);

		if (!skipOriginal || ch != original)
			cloneSiblings.add(ch);
	}

	for (const auto& c : cloneSiblings)
	{
		jassert(c.getType() == original.getType());
        ignoreUnused(c);
	}
}

scriptnode::Parameter* CloneNode::CloneIterator::getParameterForValueTree(const ValueTree& pTree, NodeBase::Ptr root) const
{
	if (root == nullptr)
		root = &cn;

	for (auto p: NodeBase::ParameterIterator(*root))
	{
		if (p->data == pTree)
			return p;
	}

	if (auto cont = dynamic_cast<NodeContainer*>(root.get()))
	{
		for (auto& cn : cont->getNodeList())
		{
			if (auto p = getParameterForValueTree(pTree, cn))
				return p;
		}
	}

	return nullptr;
}

void CloneNode::CloneIterator::throwError(const String& e)
{
	cn.getRootNetwork()->getExceptionHandler().addCustomError(&cn, Error::CloneMismatch, e);
}

void CloneNode::CloneIterator::resetError()
{
	cn.getRootNetwork()->getExceptionHandler().removeError(&cn, Error::CloneMismatch);
}

CloneNode::CloneNode(DspNetwork* n, ValueTree d) :
	SerialNode(n, d)
{
    obj.cloneData.setCloneNode(this);
    
	if (!d.hasProperty(PropertyIds::ShowClones))
		d.setProperty(PropertyIds::ShowClones, true, getUndoManager());

	showClones.referTo(d, PropertyIds::ShowClones, getUndoManager(), true);

	

	initListeners(false);
	
	NodeContainer::addFixedParameters();

	numVoicesListener.setCallback(getNodeTree(), valuetree::AsyncMode::Synchronously, [this](const ValueTree&, bool wasAdded)
	{
		auto numMax = jmax(1, getNodeTree().getNumChildren());
		auto numTree = getParameterTree().getChildWithProperty(PropertyIds::ID, PropertyIds::NumClones.toString());
		numTree.setProperty(PropertyIds::MaxValue, numMax, getUndoManager());
	});

	auto syncedParameterIds = RangeHelpers::getRangeIds(true);
	syncedParameterIds.add(PropertyIds::Automated);
    syncedParameterIds.add(PropertyIds::Bypassed);

	valueSyncer.setCallback(getNodeTree(), syncedParameterIds, valuetree::AsyncMode::Synchronously, BIND_MEMBER_FUNCTION_2(CloneNode::syncCloneProperty));

	cloneWatcher.setTypeToWatch(PropertyIds::Nodes);
	cloneWatcher.setCallback(getNodeTree(), valuetree::AsyncMode::Synchronously, BIND_MEMBER_FUNCTION_2(CloneNode::checkValidClones));

	Array<Identifier> uiIds = { PropertyIds::NodeColour, PropertyIds::Folded, PropertyIds::ShowParameters, PropertyIds::IsVertical, PropertyIds::Comment, PropertyIds::Frozen };

	uiSyncer.setCallback(getNodeTree(), uiIds, valuetree::AsyncMode::Synchronously, BIND_MEMBER_FUNCTION_2(CloneNode::syncCloneProperty));

	connectionListener.setCallback(getNodeTree(), valuetree::AsyncMode::Synchronously, BIND_MEMBER_FUNCTION_2(CloneNode::updateConnections));

	// prevent the initial execution
	connectionListener.setTypesToWatch({ PropertyIds::Connections, PropertyIds::ModulationTargets });

	displayCloneRangeListener.setCallback(d, { PropertyIds::DisplayedClones }, valuetree::AsyncMode::Synchronously, BIND_MEMBER_FUNCTION_2(CloneNode::updateDisplayedClones));
    
    complexDataSyncer.setCallback(getNodeTree(), { PropertyIds::Index, PropertyIds::EmbeddedData }, valuetree::AsyncMode::Synchronously, BIND_MEMBER_FUNCTION_2(CloneNode::syncCloneProperty));
    
    if(getNodeTree().getNumChildren() == 0)
    {
        var s = getRootNetwork()->create("container.chain", getId() + "_child");
        auto ct = dynamic_cast<NodeBase*>(s.getObject())->getValueTree();
        
        ct.setProperty(PropertyIds::NodeColour, 0xFF949494, getUndoManager());
        
        auto cp = ct.getParent();
        getNodeTree().addChild(ct, -1, getUndoManager());
    }
}

scriptnode::ParameterDataList CloneNode::createInternalParameterList()
{
	ParameterDataList data;

	{
		DEFINE_PARAMETERDATA(CloneNode, NumClones);
		p.setRange({ 1.0, 16.0, 1.0 });
		p.setDefaultValue(1.0);
		data.add(std::move(p));
	}
	{
		DEFINE_PARAMETERDATA(CloneNode, SplitSignal);
		p.setRange({ 0.0, 2.0, 1.0 });
        p.setParameterValueNames({"Serial", "Parallel", "Copy"});
		p.setDefaultValue(2.0);
		data.add(std::move(p));
	}

	return data;
}

void CloneNode::processFrame(FrameType& data) noexcept
{
    // implement compile time channel count
    jassertfalse;
}

void CloneNode::process(ProcessDataDyn& data) noexcept
{
	NodeProfiler np(this, data.getNumSamples());
	ProcessDataPeakChecker pd(this, data);

	if (isBypassed() && !nodes.isEmpty())
		nodes.getFirst()->process(data);
	else
        obj.process(data);
}

void CloneNode::prepare(PrepareSpecs ps)
{
	NodeBase::prepare(ps);
	prepareNodes(ps);

    obj.prepare(ps);
}

void CloneNode::handleHiseEvent(HiseEvent& e)
{
    obj.handleHiseEvent(e);
}

void CloneNode::reset()
{
    obj.reset();
}


void CloneNode::setNumClones(double newSize)
{
	for (int i = 0; i < nodes.size(); i++)
	{
		DynamicBypassParameter::ScopedUndoDeactivator sds(nodes[i]);
		nodes[i]->setBypassed(i >= newSize);
	}

	obj.setNumClones(newSize);
}

void CloneNode::setSplitSignal(double shouldSplit)
{
    isVertical.storeValue(shouldSplit < 1.0, getUndoManager());
    obj.setCloneProcessType(shouldSplit);
}

int CloneNode::getCloneIndex(NodeBase* n)
{
	auto cn = n->findParentNodeOfType<CloneNode>();

	if (cn == nullptr)
		return -1;

	return cn->getPathForValueTree(n->getValueTree()).getFirst();
}

void CloneNode::syncCloneProperty(const ValueTree& v, const Identifier& id)
{
    // do not sync the top level bypass state
    if(id == PropertyIds::Bypassed && v.getParent() == getNodeTree())
        return;
    
    if(currentlySyncedIds.contains(id))
        return;
    
    currentlySyncedIds.addIfNotAlreadyThere(id);
    
    auto value = v[id];
	
	for(auto& cv: CloneIterator(*this, v, true))
		cv.setProperty(id, value, getUndoManager());
    
    currentlySyncedIds.removeAllInstancesOf(id);
}

Component* CloneNode::createLeftTabComponent() const
{
	return new CloneOptionComponent(const_cast<CloneNode*>(this));
}

void CloneNode::updateConnections(const ValueTree& v, bool wasAdded)
{
	if (connectionRecursion)
		return;

	ScopedValueSetter<bool> svs(connectionRecursion, true);

	if (!wasAdded)
	{
		CloneIterator cit(*this, connectionListener.getCurrentParent(), true);

		for (auto& cv : cit)
			cv.removeChild(connectionListener.getRemoveIndex(), getUndoManager());
	}
	else
	{
		CloneIterator cit(*this, connectionListener.getCurrentParent(), true);

		for (auto& cv : cit)
		{
			auto copy = v.createCopy();


			auto originalId = v[PropertyIds::NodeId];
			auto originalTree = getRootNetwork()->getNodeWithId(originalId)->getValueTree();
			auto originalPath = getPathForValueTree(originalTree);

			auto thisCloneIndex = getPathForValueTree(cv).getFirst();

			originalPath.set(0, thisCloneIndex);
			auto newPath = getValueTreeForPath(getNodeTree(), originalPath);
			auto newId = newPath[PropertyIds::ID].toString();
			copy.setProperty(PropertyIds::NodeId, newId, nullptr);

			cv.addChild(copy, -1, getUndoManager());
		}
	}
}

void CloneNode::checkValidClones(const ValueTree& v, bool wasAdded)
{
	getRootNetwork()->getExceptionHandler().removeError(this, Error::CloneMismatch);

	auto firstTree = getNodeTree().getChild(0);

    if(firstTree.isValid() && !firstTree[PropertyIds::FactoryPath].toString().startsWith("container."))
        getRootNetwork()->getExceptionHandler().addCustomError(this, Error::CloneMismatch, "clone root element must be a container");
    
	for (int i = 1; i < getNodeTree().getNumChildren(); i++)
	{
		if (!sameNodes(firstTree, getNodeTree().getChild(i)))
			getRootNetwork()->getExceptionHandler().addCustomError(this, Error::CloneMismatch, "clone doesn't match");
	}

	cloneChangeBroadcaster.sendMessage(sendNotificationSync, this);

	auto firstParameter = getParameterFromIndex(0);

	if (wasAdded && firstParameter->getValue() == getNodeTree().getNumChildren() - 1)
		firstParameter->setValueSync(getNodeTree().getNumChildren());
	if (!wasAdded && firstParameter->getValue() == getNodeTree().getNumChildren() + 1)
		firstParameter->setValueSync(getNodeTree().getNumChildren());

	updateDisplayedClones({}, getValueTree()[PropertyIds::DisplayedClones]);
}

void CloneNode::updateDisplayedClones(const Identifier&, const var& v)
{
	auto s = v.toString();
	s = s.replace(";", ",");

	auto tokens = StringArray::fromTokens(s, ",", "");
	tokens.removeEmptyStrings();

	displayedCloneState.clear();

	if (tokens.isEmpty())
		displayedCloneState.setBit(0, true);
		
	for (auto t : tokens)
	{
		if (t.contains("-"))
		{
			auto range = StringArray::fromTokens(t, "-", "");
			range.removeEmptyStrings();
			int start = range[0].getIntValue()-1;
			auto end = range[1].getIntValue();
			displayedCloneState.setRange(start, end - start, true);
		}
		else
		{
			if (auto value = t.getIntValue())
				displayedCloneState.setBit(value-1);
		}
	}

	if (displayedCloneState.findNextClearBit(0) > nodes.size())
		displayedCloneState.setBit(nodes.size() - 1, false);
}

bool CloneNode::sameNodes(const ValueTree& n1, const ValueTree& n2)
{
	if (n1[PropertyIds::FactoryPath] != n2[PropertyIds::FactoryPath])
		return false;

	auto c1 = n1.getChildWithName(PropertyIds::Nodes);
	auto c2 = n2.getChildWithName(PropertyIds::Nodes);

	if (c1.getNumChildren() != c2.getNumChildren())
		return false;

	for (int i = 0; i < c1.getNumChildren(); i++)
	{
		if (!sameNodes(c1.getChild(i), c2.getChild(i)))
			return false;
	}

	return true;
}

juce::ValueTree CloneNode::getValueTreeForPath(const ValueTree& v, Array<int>& path)
{
	if (path.isEmpty())
		return v;

	auto firstIndex = path.removeAndReturn(0);
	return getValueTreeForPath(v.getChild(firstIndex), path);
}

juce::Array<int> CloneNode::getPathForValueTree(const ValueTree& v)
{
	auto p = v;

	Array<int> path;

	while (p != getNodeTree() && p.isValid())
	{
		path.insert(0, p.getParent().indexOf(p));
		p = p.getParent();
	}

	return path;
}

bool CloneNode::shouldCloneBeDisplayed(int index) const
{
	if (getValueTree()[PropertyIds::ShowClones])
		return true;

	if (displayedCloneState.isZero())
		return index == 0;

	return displayedCloneState[index];
}

FixedBlockXNode::FixedBlockXNode(DspNetwork* network, ValueTree d) :
	SerialNode(network, d)
{
	initListeners();
	obj.initialise(this);
}

void FixedBlockXNode::process(ProcessDataDyn& data)
{
	NodeProfiler np(this, getBlockSizeForChildNodes());
	ProcessDataPeakChecker pd(this, data);
    TRACE_DSP();
    
	obj.process(data);
}

void FixedBlockXNode::processFrame(FrameType& data) noexcept
{
	FrameDataPeakChecker fd(this, data.begin(), data.size());
	obj.processFrame(data);
}

void FixedBlockXNode::prepare(PrepareSpecs ps)
{
	NodeBase::prepare(ps);
	NodeContainer::prepareNodes(ps);
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

struct FixBlockXComponent : public Component
{
	FixBlockXComponent(NodeBase* n) :
		mode("64", PropertyIds::BlockSize)
	{
		addAndMakeVisible(mode);

		mode.initModes({ "8", "16", "32", "64", "128", "256" }, n);
		setSize(128 + 2 * UIValues::NodeMargin, 32);
	};

	void resized() override
	{
		auto b = getLocalBounds().withSizeKeepingCentre(128, 32);
		mode.setBounds(b);
	}

	ComboBoxWithModeProperty mode;
};

Component* FixedBlockXNode::createLeftTabComponent() const
{
	return new FixBlockXComponent(const_cast<FixedBlockXNode*>(this));
}

void FixedBlockXNode::setBypassed(bool shouldBeBypassed)
{
	SerialNode::setBypassed(shouldBeBypassed);

	if (originalBlockSize == 0)
		return;

	PrepareSpecs ps;
	ps.blockSize = originalBlockSize;
	ps.sampleRate = originalSampleRate;
	ps.numChannels = getCurrentChannelAmount();
	ps.voiceIndex = lastVoiceIndex;

	prepare(ps);

	getRootNetwork()->runPostInitFunctions();
}

NoMidiChainNode::NoMidiChainNode(DspNetwork* n, ValueTree t):
	SerialNode(n, t)
{
	initListeners();
	obj.getObject().initialise(this);
}

void NoMidiChainNode::processFrame(FrameType& data) noexcept
{
	FrameDataPeakChecker fd(this, data.begin(), data.size());
	obj.processFrame(data);
}

void NoMidiChainNode::process(ProcessDataDyn& data) noexcept
{
	obj.process(data);
}

void NoMidiChainNode::prepare(PrepareSpecs ps)
{
	NodeBase::prepare(ps);
	NodeContainer::prepareNodes(ps);
	obj.prepare(ps);
}

void NoMidiChainNode::handleHiseEvent(HiseEvent& e)
{
	// let the wrapper send it to nirvana
	obj.handleHiseEvent(e);
}

void NoMidiChainNode::reset()
{
	obj.reset();
}

SoftBypassNode::SoftBypassNode(DspNetwork* n, ValueTree t):
	SerialNode(n, t),
	smoothingTime(PropertyIds::SmoothingTime, 20)
{
	initListeners();
	obj.initialise(this);

	smoothingTime.initialise(this);
	smoothingTime.setAdditionalCallback(BIND_MEMBER_FUNCTION_2(SoftBypassNode::updateSmoothingTime), true);
}

void SoftBypassNode::processFrame(FrameType& data) noexcept
{
	FrameDataPeakChecker fd(this, data.begin(), data.size());
	obj.processFrame(data);
}

void SoftBypassNode::process(ProcessDataDyn& data) noexcept
{
	NodeProfiler np(this, getBlockSizeForChildNodes());
	ProcessDataPeakChecker pd(this, data);
    TRACE_DSP();
    
	obj.process(data);
}

void SoftBypassNode::prepare(PrepareSpecs ps)
{
	NodeBase::prepare(ps);
	NodeContainer::prepareNodes(ps);
	obj.prepare(ps);
}

void SoftBypassNode::handleHiseEvent(HiseEvent& e)
{
	obj.handleHiseEvent(e);
}

void SoftBypassNode::reset()
{
	obj.reset();
}

void SoftBypassNode::updateSmoothingTime(Identifier id, var newValue)
{
	if (id == PropertyIds::Value)
	{
		auto newTime = (int)newValue;
		obj.setSmoothingTime(newTime);
	}
}

void SoftBypassNode::setBypassed(bool shouldBeBypassed)
{
	SerialNode::setBypassed(shouldBeBypassed);
	WrapperType::setParameter<bypass::ParameterId>(&this->obj, (double)shouldBeBypassed);
}








}
