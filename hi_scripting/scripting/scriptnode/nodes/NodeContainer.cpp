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

scriptnode::ParameterDataList NodeContainer::createInternalParametersForMacros()
{
	jassertfalse;
	return {};
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

void NodeContainer::addFixedParameters()
{
	if (!hasFixedParameters())
		return;

	auto an = asNode();

	auto pData = an->createInternalParameterList();

	auto d = an->getValueTree();

	d.getOrCreateChildWithName(PropertyIds::Parameters, an->getUndoManager());

	for (auto p : pData)
	{
		auto existingChild = an->getParameterTree().getChildWithProperty(PropertyIds::ID, p.info.getId());

		if (!existingChild.isValid())
		{
			existingChild = p.createValueTree();
			an->getParameterTree().addChild(existingChild, -1, an->getUndoManager());
		}

		auto newP = new Parameter(an, existingChild);

		auto ndb = new parameter::dynamic_base(p.callback);

		newP->setDynamicParameter(ndb);
		newP->valueNames = p.parameterNames;

		an->addParameter(newP);
	}
}

Component* NodeContainer::createLeftTabComponent() const
{
	return new ContainerComponent::MacroToolbar();
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

	bool useLock = n->getRootNetwork()->isInitialised();

	if (auto nodeToProcess = n->getRootNetwork()->getNodeForValueTree(child))
	{
		if (wasAdded)
		{
			if (nodes.contains(nodeToProcess))
				return;

			nodeToProcess->setParentNode(asNode());

			int insertIndex = getNodeTree().indexOf(child);

			SimpleReadWriteLock::ScopedWriteLock sl(n->getRootNetwork()->getConnectionLock(), useLock);

			nodes.insert(insertIndex, nodeToProcess);
            updateChannels(n->getValueTree(), Identifier());
		}
		else
		{
			nodeToProcess->setParentNode(nullptr);

			SimpleReadWriteLock::ScopedWriteLock sl(n->getRootNetwork()->getConnectionLock(), useLock);
			
			nodes.removeAllInstancesOf(nodeToProcess);
			updateChannels(n->getValueTree(), Identifier());
		}

		n->getRootNetwork()->runPostInitFunctions();

		//auto cs = n->getRootNetwork()->getCurrentSpecs();
		//n->getRootNetwork()->prepareToPlay(cs.sampleRate, cs.blockSize);
	}

	ownedReference.clear();
	for (auto n : nodes)
		ownedReference.add(n.get());
}


void NodeContainer::parameterAddedOrRemoved(ValueTree child, bool wasAdded)
{
	auto n = asNode();

    n->getRootNetwork()->getExceptionHandler().removeError(n, Error::CloneMismatch);
    
	if (wasAdded)
	{
        if(auto cn = dynamic_cast<CloneNode*>(asNode()->getParentNode()))
        {
            cn->getRootNetwork()->getExceptionHandler().addCustomError(asNode(), Error::CloneMismatch, "A cloned container must not have any parameters of its own");
        }
        
		auto newParameter = new MacroParameter(asNode(), child);
		n->addParameter(newParameter);
	}
	else
	{
		for (int i = 0; i < n->getNumParameters(); i++)
		{
			if (n->getParameterFromIndex(i)->data == child)
			{
				n->removeParameter(i);
				break;
			}
		}
	}
}

void NodeContainer::updateChannels(ValueTree v, Identifier id)
{
	try
	{
		if (v == asNode()->getValueTree())
		{
			channelLayoutChanged(nullptr);

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
	catch (scriptnode::Error& e)
	{
		asNode()->getRootNetwork()->getExceptionHandler().addError(asNode(), e);
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

	int minWidth = 0;

	if (ScopedPointer<Component> c = createLeftTabComponent())
		minWidth += c->getWidth();

	minWidth += 100 * an->getNumParameters();

	minWidth = jmax(UIValues::NodeWidth, minWidth);

	auto titleWidth = GLOBAL_BOLD_FONT().getStringWidthFloat(asNode()->getName());

	minWidth = jmax<int>(minWidth, titleWidth + UIValues::HeaderHeight * 4);

	if (isVerticalContainer)
	{
		int h = 0;

		h += UIValues::NodeMargin;
		h += UIValues::HeaderHeight; // the input

		if (asNode()->getValueTree()[PropertyIds::ShowParameters])
			h += UIValues::ParameterHeight + UIValues::MacroDragHeight;

		h += PinHeight; // the "hole" for the cable

		Point<int> childPos(NodeMargin, NodeMargin);

		int numHidden = 0;
		int index = 0;

		for (auto n : nodes)
		{
			if (auto cn = dynamic_cast<const CloneNode*>(this))
			{
				if (!cn->shouldCloneBeDisplayed(index++))
				{
					numHidden++;
					continue;
				}
			}

			auto bounds = n->getPositionInCanvas(childPos);
			//bounds = n->getBoundsToDisplay(bounds);
			minWidth = jmax<int>(minWidth, bounds.getWidth());
			h += bounds.getHeight() + NodeMargin;
			childPos = childPos.translated(0, bounds.getHeight());
	
		}

		if (numHidden > 0 || (!an->getValueTree()[PropertyIds::ShowClones] && an->getValueTree().hasProperty(PropertyIds::DisplayedClones)))
			h += UIValues::DuplicateSize;

		h += PinHeight; // the "hole" for the cable

		return { topLeft.getX(), topLeft.getY(), minWidth + 2 * NodeMargin, h };
	}
	else
	{
		int y = UIValues::NodeMargin;
		y += UIValues::HeaderHeight;
		y += UIValues::PinHeight;

		if (an->getValueTree()[PropertyIds::ShowParameters])
			y += UIValues::ParameterHeight + UIValues::MacroDragHeight;

		Point<int> startPos(UIValues::NodeMargin, y);

		int maxy = startPos.getY();

		int index = 0;
		int numHidden = 0;
		
		for (auto n : nodes)
		{
			if (auto cn = dynamic_cast<const CloneNode*>(this))
			{
				if (!cn->shouldCloneBeDisplayed(index++))
				{
					numHidden++;
					continue;
				}
			}

			auto b = n->getPositionInCanvas(startPos);
			maxy = jmax(b.getBottom(), maxy);
			startPos = startPos.translated(b.getWidth() + UIValues::NodeMargin, 0);
			minWidth = jmax(minWidth, startPos.getX());
		}

		if (numHidden > 0 || (!an->getValueTree()[PropertyIds::ShowClones] && an->getValueTree().hasProperty(PropertyIds::DisplayedClones)))
			minWidth += UIValues::DuplicateSize;

		maxy += UIValues::PinHeight;
		maxy += UIValues::NodeMargin;

		return { topLeft.getX(), topLeft.getY(), minWidth, maxy };
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
}

SerialNode::SerialNode(DspNetwork* root, ValueTree data) :
	NodeBase(root, data, 0),
	isVertical(PropertyIds::IsVertical, true)
{
	isVertical.initialise(this);
}

struct LockedContainerExtraComponent: public ScriptnodeExtraComponent<NodeBase>,
								      public PathFactory
{
	LockedContainerExtraComponent(NodeBase* nc):
	  ScriptnodeExtraComponent<scriptnode::NodeBase>(nc, nc->getScriptProcessor()->getMainController_()->getGlobalUIUpdater()),
	  gotoButton("goto", nullptr, *this),
	  lockIcon(createPath("lock"))
	{
		gotoButton.onClick = [this]()
		{
			findParentComponentOfClass<DspNetworkGraph>()->setCurrentRootNode(this->getObject());
		};

		addAndMakeVisible(gotoButton);
		stop();
		

		
		initConnections();

		auto w = 128;
		auto h = 22;

		if(wantsModulationDragger())
		{
			addAndMakeVisible(dragger = new ModulationSourceBaseComponent(nc->getScriptProcessor()->getMainController_()->getGlobalUIUpdater()));
			w += 128;
			h += 28 + UIValues::NodeMargin;
		}

		setSize(w, h);
	}

	ScopedPointer<ModulationSourceBaseComponent> dragger;

	bool wantsModulationDragger() const
	{
		return dynamic_cast<NodeContainer*>(getObject())->getLockedModNode() != nullptr;
	}

	void timerCallback() override
	{
		
	}

	Array<ValueTree> externalConnections;

	void initConnections()
	{
		auto vt = getObject()->getValueTree();
		auto root = vt.getRoot();

		struct Con
		{
			String node;
			String parameter;
		};

		Array<Con> connections;

		valuetree::Helpers::forEach(vt, [&](const ValueTree& v)
		{
			if(v.getType() == PropertyIds::Parameter && v[PropertyIds::Automated])
			{
				if(v.getParent().getParent() == vt)
					return false;

				connections.add({ v.getParent().getParent()[PropertyIds::ID].toString(), v[PropertyIds::ID].toString()});
			}

			return false;
		});

		for(const auto& c: connections)
		{
			valuetree::Helpers::forEach(root, [&](const ValueTree& v)
			{
				if(v.getType() == PropertyIds::Connection)
				{
					auto match = v[PropertyIds::NodeId].toString() == c.node && 
						         v[PropertyIds::ParameterId].toString() == c.parameter;

					if(match && !v.isAChildOf(vt))
					{
						DBG(v.createXml()->createDocument(""));
						externalConnections.add(v);
					}
						
				}

				return false;
			});
		}
	}

	void paint(Graphics& g) override
	{
		auto b = getLocalBounds();

		if(dragger != nullptr)
			b.removeFromBottom(dragger->getHeight() + UIValues::NodeMargin);

		b.removeFromLeft(b.getHeight() * 2);
		b.removeFromRight(b.getHeight() * 2);

		String text = "Locked " + getObject()->getPath().toString();

		auto w = GLOBAL_FONT().getStringWidth(text) + 10.0f;

		g.setColour(Colours::white.withAlpha(0.2f));

		if(w < b.getWidth())
		{
			g.setFont(GLOBAL_FONT());
			g.drawText(text, b.toFloat(), Justification::centred);
		}

		g.fillPath(lockIcon);
		
	}

	Path createPath(const String& url) const override
	{
		Path p;

		LOAD_PATH_IF_URL("goto", ColumnIcons::openWorkspaceIcon);
		LOAD_PATH_IF_URL("lock", ColumnIcons::lockIcon);

		return p;
	}

	void resized() override
	{
		auto b = getLocalBounds();

		if(dragger != nullptr)
		{
			dragger->setBounds(b.removeFromBottom(28));
			b.removeFromBottom(UIValues::NodeMargin);
		}
			

		gotoButton.setBounds(b.removeFromLeft(b.getHeight()).reduced(3));

		scalePath(lockIcon, b.removeFromRight(b.getHeight()).reduced(5).toFloat());

		getProperties().set("circleOffsetX", -1 * getWidth() / 2 + b.getHeight() + UIValues::NodeMargin);
		getProperties().set("circleOffsetY", -1 * getHeight() + JUCE_LIVE_CONSTANT_OFF(9));
	}

	HiseShapeButton gotoButton;
	Path lockIcon;
};

scriptnode::NodeComponent* SerialNode::createComponent()
{
	if(isLockedContainer())
	{
		auto s = new DefaultParameterNodeComponent(this);
		s->setExtraComponent(new LockedContainerExtraComponent(this));
		return s;
	}

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


Rectangle<int> SerialNode::getPositionInCanvas(Point<int> topLeft) const
{
	if(isLockedContainer())
	{
		auto b = getBoundsToDisplay(NodeComponent::PositionHelpers::getPositionInCanvasForStandardSliders(this, topLeft));
		return b;
	}
	else
		return getBoundsToDisplay(getContainerPosition(isVertical.getValue(), topLeft));
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
	if(isLockedContainer())
	{
		auto b = getBoundsToDisplay(NodeComponent::PositionHelpers::getPositionInCanvasForStandardSliders(this, topLeft));
		return b;
	}
		
	else
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
	registerNodeRaw<OversampleNode<-1>>();
	registerNodeRaw<FixedBlockNode<8>>();
	registerNodeRaw<FixedBlockNode<16>>();
	registerNodeRaw<FixedBlockNode<32>>();
	registerNodeRaw<FixedBlockNode<64>>();
	registerNodeRaw<FixedBlockNode<128>>();
	registerNodeRaw<FixedBlockNode<256>>();
	registerNodeRaw<FixedBlockXNode>();
	registerNodeRaw<DynamicBlockSizeNode>();
	registerNodeRaw<OfflineChainNode>();
    registerNodeRaw<RepitchNode>();
	registerNodeRaw<CloneNode>();
	registerNodeRaw<NoMidiChainNode>();
	registerNodeRaw<SoftBypassNode>();
    registerNodeRaw<SidechainNode>();
	registerNodeRaw<BranchNode>();
}

juce::ValueTree NodeContainer::MacroParameter::getConnectionTree()
{
	auto existing = data.getChildWithName(PropertyIds::Connections);

	if (!existing.isValid())
	{
		existing = ValueTree(PropertyIds::Connections);
		data.addChild(existing, -1, parent->getUndoManager(true));
	}

	return existing;
}


NodeContainer::MacroParameter::MacroParameter(NodeBase* parentNode, ValueTree data_) :
	Parameter(parentNode, data_),
	ConnectionSourceManager(parentNode->getRootNetwork(), getConnectionTree()),
	pholder(new parameter::dynamic_base_holder())
{
	inputRangeListener.setCallback(data, RangeHelpers::getRangeIds(), valuetree::AsyncMode::Synchronously,
		BIND_MEMBER_FUNCTION_2(MacroParameter::updateInputRange));
	
	initConnectionSourceListeners();
}

void NodeContainer::MacroParameter::rebuildCallback()
{
	auto nc = ConnectionBase::createParameterFromConnectionTree(parent, getConnectionTree(), true);
	setDynamicParameter(nc);
}

void NodeContainer::MacroParameter::setDynamicParameter(parameter::dynamic_base::Ptr ownedNew)
{
	pholder->setParameter(parent, ownedNew);
	Parameter::setDynamicParameter(pholder);
}

void NodeContainer::MacroParameter::updateInputRange(Identifier, var)
{
	rebuildCallback();
}


}
