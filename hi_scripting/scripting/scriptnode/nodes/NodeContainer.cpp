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


NodeContainer::NodeContainer()
{

}

void NodeContainer::resetNodes()
{

	for (auto n : nodes)
		n->reset();
}

scriptnode::NodeBase* NodeContainer::asNode()
{

	auto n = dynamic_cast<NodeBase*>(this);
	jassert(n != nullptr);
	return n;
}

const scriptnode::NodeBase* NodeContainer::asNode() const
{

	auto n = dynamic_cast<const NodeBase*>(this);
	jassert(n != nullptr);
	return n;
}

void NodeContainer::prepareContainer(PrepareSpecs& ps)
{
	originalSampleRate = ps.sampleRate;
	originalBlockSize = ps.blockSize;
	lastVoiceIndex = ps.voiceIndex;

	ps.sampleRate = getSampleRateForChildNodes();
	ps.blockSize = getBlockSizeForChildNodes();
}

void NodeContainer::prepareNodes(PrepareSpecs ps)
{
	prepareContainer(ps);

	for (auto n : nodes)
	{
		if (n == nullptr)
			continue;

		

		auto& eHandler = asNode()->getRootNetwork()->getExceptionHandler();

		eHandler.removeError(n);

		try
		{
			n->prepare(ps);
			n->reset();
		}
		catch (Error& e)
		{
			eHandler.addError(n, e);
		}
	}
}

bool NodeContainer::shouldCreatePolyphonicClass() const
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

void NodeContainer::nodeAddedOrRemoved(ValueTree child, bool wasAdded)
{
	auto n = asNode();

	DspNetwork::AnonymousNodeCloner cloner(*n->getRootNetwork(), n->getNodeHolder());
	
	bool useLock = n->getRootNetwork()->isInitialised();

	if (auto nodeToProcess = n->getRootNetwork()->getNodeForValueTree(child))
	{
		if (wasAdded)
		{
			if (nodes.contains(nodeToProcess))
				return;

			nodeToProcess->setParentNode(asNode());

			auto isDuplicate = asNode()->isUINodeOfDuplicate();

			nodeToProcess->setIsUINodeOfDuplicates(isDuplicate);

			int insertIndex = getNodeTree().indexOf(child);

			SimpleReadWriteLock::ScopedWriteLock sl(n->getRootNetwork()->getConnectionLock(), useLock);

			nodes.insert(insertIndex, nodeToProcess);
			updateChannels(n->getValueTree(), PropertyIds::NumChannels);
		}
		else
		{
			nodeToProcess->setParentNode(nullptr);

			nodeToProcess->setIsUINodeOfDuplicates(false);

			SimpleReadWriteLock::ScopedWriteLock sl(n->getRootNetwork()->getConnectionLock(), useLock);
			
			nodes.removeAllInstancesOf(nodeToProcess);
			updateChannels(n->getValueTree(), PropertyIds::NumChannels);
		}
	}
}


void NodeContainer::parameterAddedOrRemoved(ValueTree child, bool wasAdded)
{
	auto n = asNode();

	if (wasAdded)
	{
		auto newParameter = new MacroParameter(asNode(), child);
		n->addParameter(newParameter);
	}
	else
	{
		for (int i = 0; i < n->getNumParameters(); i++)
		{
			if (n->getParameter(i)->data == child)
			{
				n->removeParameter(i);
				break;
			}
		}
	}
}

void NodeContainer::updateChannels(ValueTree v, Identifier id)
{
	if (v == asNode()->getValueTree())
	{
		channelLayoutChanged(nullptr);

		if (originalSampleRate > 0.0 && !asNode()->isUINodeOfDuplicate())
		{
			PrepareSpecs ps;
			ps.numChannels = asNode()->getCurrentChannelAmount();
			ps.blockSize = originalBlockSize;
			ps.sampleRate = originalSampleRate;
			ps.voiceIndex = lastVoiceIndex;

			asNode()->prepare(ps);
		}
	}
	else if (v.getParent() == getNodeTree())
	{
		if (channelRecursionProtection)
			return;

		auto childNode = asNode()->getRootNetwork()->getNodeForValueTree(v);
		ScopedValueSetter<bool> svs(channelRecursionProtection, true);
		channelLayoutChanged(childNode);

		if (originalSampleRate > 0.0)
		{
			PrepareSpecs ps;
			ps.numChannels = asNode()->getCurrentChannelAmount();
			ps.blockSize = originalBlockSize;
			ps.sampleRate = originalSampleRate;
			ps.voiceIndex = lastVoiceIndex;

			asNode()->prepare(ps);
		}
	}
}

bool NodeContainer::isPolyphonic() const
{

	if (auto n = dynamic_cast<NodeContainer*>(asNode()->getParentNode()))
	{
		return n->isPolyphonic();
	}
	else
		return asNode()->getRootNetwork()->isPolyphonic();
}

void NodeContainer::assign(const int index, var newValue)
{
	SimpleReadWriteLock::ScopedWriteLock sl(asNode()->getRootNetwork()->getConnectionLock());

	auto un = asNode()->getUndoManager();

	if (auto node = dynamic_cast<NodeBase*>(newValue.getObject()))
	{
		auto tree = node->getValueTree();

		tree.getParent().removeChild(tree, un);
		getNodeTree().addChild(tree, index, un);
	}
	else
	{
		getNodeTree().removeChild(index, un);
	}
}


scriptnode::NodeBase::List NodeContainer::getChildNodesRecursive()
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

int NodeContainer::getCachedIndex(const var &indexExpression) const
{
	return (int)indexExpression;
}

bool NodeContainer::forEachNode(const std::function<bool(NodeBase::Ptr)> & f)
{
	if (f(asNode()))
		return true;

	for (auto n : nodes)
	{
		if (n->forEach(f))
			return true;
	}

	return false;
}

void NodeContainer::clear()
{
	getNodeTree().removeAllChildren(asNode()->getUndoManager());
}

juce::Rectangle<int> NodeContainer::getContainerPosition(bool isVerticalContainer, Point<int> topLeft) const
{
	using namespace UIValues;
	auto an = asNode();

	if (isVerticalContainer)
	{
		const int minWidth = jmax(NodeWidth, 100 * an->getNumParameters() + 50);
		int maxW = minWidth;
		int h = 0;

		h += UIValues::NodeMargin;
		h += UIValues::HeaderHeight; // the input

		if (asNode()->getValueTree()[PropertyIds::ShowParameters])
			h += UIValues::ParameterHeight;

		h += PinHeight; // the "hole" for the cable

		Point<int> childPos(NodeMargin, NodeMargin);

		for (auto n : nodes)
		{
			auto bounds = n->getPositionInCanvas(childPos);
			//bounds = n->getBoundsToDisplay(bounds);
			maxW = jmax<int>(maxW, bounds.getWidth());
			h += bounds.getHeight() + NodeMargin;
			childPos = childPos.translated(0, bounds.getHeight());
		}

		h += PinHeight; // the "hole" for the cable

		return { topLeft.getX(), topLeft.getY(), maxW + 2 * NodeMargin, h };
	}
	else
	{

		int y = UIValues::NodeMargin;
		y += UIValues::HeaderHeight;
		y += UIValues::PinHeight;

		if (an->getValueTree()[PropertyIds::ShowParameters])
			y += UIValues::ParameterHeight;

		Point<int> startPos(UIValues::NodeMargin, y);

		int maxy = startPos.getY();
		int maxWidth = NodeWidth + NodeMargin;

		for (auto n : nodes)
		{
			auto b = n->getPositionInCanvas(startPos);
			maxy = jmax(b.getBottom(), maxy);
			startPos = startPos.translated(b.getWidth() + UIValues::NodeMargin, 0);
			maxWidth = startPos.getX();
		}

		maxy += UIValues::PinHeight;
		maxy += UIValues::NodeMargin;

		return { topLeft.getX(), topLeft.getY(), maxWidth, maxy };
	}
}

void NodeContainer::initListeners(bool initParameterListener)
{
	nodeListener.setCallback(getNodeTree(),
		valuetree::AsyncMode::Synchronously,
		BIND_MEMBER_FUNCTION_2(NodeContainer::nodeAddedOrRemoved));

	if (initParameterListener)
	{
		parameterListener.setCallback(asNode()->getParameterTree(),
			valuetree::AsyncMode::Synchronously,
			BIND_MEMBER_FUNCTION_2(NodeContainer::parameterAddedOrRemoved));
	}

	channelListener.setCallback(asNode()->getValueTree(), { PropertyIds::NumChannels },
		valuetree::AsyncMode::Synchronously,
		BIND_MEMBER_FUNCTION_2(NodeContainer::updateChannels));
}

SerialNode::SerialNode(DspNetwork* root, ValueTree data) :
	NodeBase(root, data, 0),
	isVertical(PropertyIds::IsVertical, true)
{
	isVertical.initialise(this);
}


scriptnode::NodeComponent* SerialNode::createComponent()
{
	if (!isVertical.getValue())
		return new ParallelNodeComponent(this);
	else
		return new SerialNodeComponent(this);
}

SerialNode::DynamicSerialProcessor::DynamicSerialProcessor(const DynamicSerialProcessor& other)
{
	jassertfalse;
}

bool SerialNode::DynamicSerialProcessor::handleModulation(double&)
{

	return false;
}

void SerialNode::DynamicSerialProcessor::handleHiseEvent(HiseEvent& e)
{
	for (auto n : parent->getNodeList())
		n->handleHiseEvent(e);
}

void SerialNode::DynamicSerialProcessor::initialise(NodeBase* p)
{
	parent = dynamic_cast<NodeContainer*>(p);
}

void SerialNode::DynamicSerialProcessor::reset()
{
	for (auto n : parent->getNodeList())
		n->reset();
}

void SerialNode::DynamicSerialProcessor::prepare(PrepareSpecs)
{
	// do nothing here, the container inits the child nodes.
}


ParallelNode::ParallelNode(DspNetwork* root, ValueTree data) :
	NodeBase(root, data, 0)
{

}

NodeComponent* ParallelNode::createComponent()
{
	return new ParallelNodeComponent(this);
}

juce::Rectangle<int> ParallelNode::getPositionInCanvas(Point<int> topLeft) const
{
	return getBoundsToDisplay(getContainerPosition(false, topLeft));
}


NodeContainerFactory::NodeContainerFactory(DspNetwork* parent) :
	NodeFactory(parent)
{
	registerNodeRaw<ChainNode>();
	registerNodeRaw<SplitNode>();
	registerNodeRaw<MultiChannelNode>();
	registerNodeRaw<ModulationChainNode>();
	registerNodeRaw<MidiChainNode>();
	registerNodeRaw<SingleSampleBlock<1>>();
	registerNodeRaw<SingleSampleBlock<2>>();
	registerNodeRaw<SingleSampleBlockX>();
	registerNodeRaw<OversampleNode<2>>();
	registerNodeRaw<OversampleNode<4>>();
	registerNodeRaw<OversampleNode<8>>();
	registerNodeRaw<OversampleNode<16>>();
	registerNodeRaw<FixedBlockNode<8>>();
	registerNodeRaw<FixedBlockNode<16>>();
	registerNodeRaw<FixedBlockNode<32>>();
	registerNodeRaw<FixedBlockNode<64>>();
	registerNodeRaw<FixedBlockNode<128>>();
	registerNodeRaw<FixedBlockNode<256>>();
	registerNodeRaw<FixedBlockXNode>();
	registerNodeRaw<OfflineChainNode>();
	registerNodeRaw<NoMidiChainNode>();
}


NodeContainer::MacroParameter::Connection::Connection(NodeBase* parent, MacroParameter* pp, ValueTree d):
	ConnectionBase(parent->getScriptProcessor(), d),
	um(parent->getUndoManager()),
	parentParameter(pp)
{
	initRemoveUpdater(parent);

	auto nodeId = d[PropertyIds::NodeId].toString();

	if (auto targetNode = dynamic_cast<NodeBase*>(parent->getRootNetwork()->get(nodeId).getObject()))
	{
		auto parameterId = d[PropertyIds::ParameterId].toString();

		if (parameterId == PropertyIds::Bypassed.toString())
		{
			nodeToBeBypassed = targetNode;
			auto originalRange = RangeHelpers::getDoubleRange(d.getParent().getParent());
			rangeMultiplerForBypass = jlimit(1.0, 9000.0, originalRange.end);
		}
		else
		{
			for (int i = 0; i < targetNode->getNumParameters(); i++)
			{
				if (targetNode->getParameter(i)->getId() == parameterId)
				{
					targetParameter = targetNode->getParameter(i);
					break;
				}
			}
		}
	}
	else
	{
		targetParameter = nullptr;
		return;
	}

	connectionRange = RangeHelpers::getDoubleRange(d);
}




juce::ValueTree NodeContainer::MacroParameter::getConnectionTree()
{
	auto existing = data.getChildWithName(PropertyIds::Connections);

	if (!existing.isValid())
	{
		existing = ValueTree(PropertyIds::Connections);
		data.addChild(existing, -1, parent->getUndoManager());
	}

	return existing;
}


NodeContainer::MacroParameter::MacroParameter(NodeBase* parentNode, ValueTree data_) :
	Parameter(parentNode, data_),
	SimpleTimer(parentNode->getScriptProcessor()->getMainController_()->getGlobalUIUpdater())
{
	rangeListener.setCallback(getConnectionTree(),
		RangeHelpers::getRangeIds(),
		valuetree::AsyncMode::Synchronously,
		BIND_MEMBER_FUNCTION_2(MacroParameter::updateRangeForConnection));

	expressionListener.setCallback(getConnectionTree(),
		{ PropertyIds::Expression },
		valuetree::AsyncMode::Synchronously,
		BIND_MEMBER_FUNCTION_2(MacroParameter::updateRangeForConnection));

	inputRangeListener.setCallback(data, RangeHelpers::getRangeIds(), valuetree::AsyncMode::Synchronously,
		BIND_MEMBER_FUNCTION_2(MacroParameter::updateInputRange));
	
	connectionListener.setCallback(getConnectionTree(),
		valuetree::AsyncMode::Synchronously,
		[this](ValueTree child, bool wasAdded)
	{
		if (!wasAdded)
		{
			auto macroTargetId = child[PropertyIds::NodeId].toString();
			auto parameterId = child[PropertyIds::ParameterId].toString();

			if (auto macroTarget = parent->getRootNetwork()->getNodeWithId(macroTargetId))
			{
				if (parameterId == PropertyIds::Bypassed.toString())
					macroTarget->getValueTree().removeProperty(PropertyIds::DynamicBypass, parent->getUndoManager());
			}
		}

		rebuildCallback();
	});

	auto initialValue = (double)data[PropertyIds::Value];
	getReferenceToCallback().call(initialValue);
}

void NodeContainer::MacroParameter::rebuildCallback()
{
	

	connections.clear();
	auto cTree = data.getChildWithName(PropertyIds::Connections);

	for (auto c : cTree)
	{
		ScopedPointer<Connection> newC = new Connection(parent, this, c);

		if (newC->isValid())
			connections.add(newC.release());

#if 0
		else
		{
			cTree.removeChild(c, nullptr);
			break;
		}
#endif
	}

#if 0
	if (connections.size() > 0)
	{
		Array<DspHelpers::ParameterCallback> connectionCallbacks;

		for (auto c : connections)
			connectionCallbacks.add(c->createCallbackForNormalisedInput());

		if (RangeHelpers::isIdentity(inputRange))
		{
			setCallback([connectionCallbacks](double newValue)
			{
				for (auto& cb : connectionCallbacks)
					cb(newValue);
			});
		}
		else
		{
			auto cp = inputRange;
			setCallback([cp, connectionCallbacks](double newValue)
			{
				auto normedValue = cp.convertTo0to1(newValue);

				for (auto& cb : connectionCallbacks)
					cb(normedValue);
			});
		}
	}
	else
	{
		setCallback({});
	}
#endif

	ScopedPointer<parameter::dynamic_chain> chain = Connection::createParameterFromConnectionTree(parent, cTree);

	if (auto s = chain->getFirstIfSingle())
		setCallbackNew(s);
	else if (!chain->isEmpty())
		setCallbackNew(chain.release());
	else
		setCallbackNew(nullptr);

	start();
}


void NodeContainer::MacroParameter::updateRangeForConnection(ValueTree v, Identifier)
{
	//RangeHelpers::checkInversion(v, &rangeListener, parent->getUndoManager());
	rebuildCallback();
}

void NodeContainer::MacroParameter::updateInputRange(Identifier, var)
{
	rebuildCallback();
}

bool NodeContainer::MacroParameter::matchesTarget(const Parameter* target) const
{
	for (auto c : connections)
		if (c->matchesTarget(target))
			return true;

	return false;
}


}
