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

struct NodeBase::Wrapper
{
	API_VOID_METHOD_WRAPPER_0(NodeBase, reset);
	API_VOID_METHOD_WRAPPER_2(NodeBase, set);
	API_VOID_METHOD_WRAPPER_1(NodeBase, setBypassed);
	API_METHOD_WRAPPER_0(NodeBase, isBypassed);
	API_METHOD_WRAPPER_1(NodeBase, get);
	API_VOID_METHOD_WRAPPER_2(NodeBase, setParent);
	API_METHOD_WRAPPER_2(NodeBase, connectTo);
	API_VOID_METHOD_WRAPPER_1(NodeBase, connectToBypass);
	API_METHOD_WRAPPER_1(NodeBase, getParameter);
    API_METHOD_WRAPPER_3(NodeBase, setComplexDataIndex);
	API_METHOD_WRAPPER_0(NodeBase, getNumParameters);
	API_METHOD_WRAPPER_1(NodeBase, getChildNodes);
};



NodeBase::NodeBase(DspNetwork* rootNetwork, ValueTree data_, int numConstants_) :
	ConstScriptingObject(rootNetwork->getScriptProcessor(), 8),
	parent(rootNetwork),
	v_data(data_),
	helpManager(this, data_),
	currentId(v_data[PropertyIds::ID].toString()),	
	subHolder(rootNetwork->getCurrentHolder())
{
	if (!v_data.hasProperty(PropertyIds::Bypassed))
		v_data.setProperty(PropertyIds::Bypassed, false, getUndoManager());

    if(!v_data.hasProperty(PropertyIds::Name))
        v_data.setProperty(PropertyIds::Name, v_data[PropertyIds::ID], getUndoManager());
    
	bypassListener.setCallback(data_,
							   PropertyIds::Bypassed, 
							   valuetree::AsyncMode::Synchronously, 
							   BIND_MEMBER_FUNCTION_2(NodeBase::updateBypassState));

	setDefaultValue(PropertyIds::NodeColour, 0);
	setDefaultValue(PropertyIds::Comment, "");
	//setDefaultValue(PropertyIds::CommentWidth, 300);

	ADD_API_METHOD_0(reset);
	ADD_API_METHOD_2(set);
	ADD_API_METHOD_1(get);
	ADD_API_METHOD_1(setBypassed);
	ADD_API_METHOD_0(isBypassed);
	ADD_API_METHOD_2(setParent);
	ADD_API_METHOD_1(getParameter);
	ADD_API_METHOD_2(connectTo);
	ADD_API_METHOD_1(connectToBypass);
    ADD_API_METHOD_3(setComplexDataIndex);
	ADD_API_METHOD_0(getNumParameters);
	ADD_API_METHOD_1(getChildNodes);

	for (auto c : getPropertyTree())
		addConstant(c[PropertyIds::ID].toString(), c[PropertyIds::ID]);
}

NodeBase::~NodeBase()
{
	parameters.clear();
}

void NodeBase::prepare(PrepareSpecs specs)
{
	if(lastSpecs.numChannels == 0)
		setBypassed(isBypassed());

	lastSpecs = specs;
	cpuUsage = 0.0;

	for (auto p : parameters)
	{
        if(p == nullptr)
            continue;
        
		if (p->isModulated())
			continue;

		auto v = p->getValue();
		p->setValueAsync(v);
	}
}

DspNetwork* NodeBase::getRootNetwork() const
{
	return static_cast<DspNetwork*>(parent.get());
}

scriptnode::NodeBase::Holder* NodeBase::getNodeHolder() const
{
	if (subHolder != nullptr)
		return subHolder;

	return static_cast<DspNetwork*>(parent.get());
}

void NodeBase::setValueTreeProperty(const Identifier& id, const var value)
{
	v_data.setProperty(id, value, getUndoManager());
}

void NodeBase::setDefaultValue(const Identifier& id, var newValue)
{
	if (!v_data.hasProperty(id))
		v_data.setProperty(id, newValue, nullptr);
}

void NodeBase::setNodeProperty(const Identifier& id, const var& newValue)
{
	auto propTree = getPropertyTree().getChildWithProperty(PropertyIds::ID, id.toString());

	if (propTree.isValid())
		propTree.setProperty(PropertyIds::Value, newValue, getUndoManager());
}

void NodeBase::set(var id, var value)
{
	checkValid();

	Identifier id_(id.toString());

	if (hasNodeProperty(id_))
	{
		setNodeProperty(id.toString(), value);
	}

	if (getValueTree().hasProperty(id_))
	{
		getValueTree().setProperty(id_, value, getUndoManager());
	}
}

var NodeBase::getNodeProperty(const Identifier& id)
{
	auto propTree = getPropertyTree().getChildWithProperty(PropertyIds::ID, id.toString());

	if (propTree.isValid())
		return propTree[PropertyIds::Value];

	return {};
}

bool NodeBase::hasNodeProperty(const Identifier& id) const
{
	auto propTree = v_data.getChildWithName(PropertyIds::Properties);

	if (propTree.isValid())
		return propTree.getChildWithProperty(PropertyIds::ID, id.toString()).isValid();

	return false;
}

juce::Value NodeBase::getNodePropertyAsValue(const Identifier& id)
{
	auto propTree = getPropertyTree().getChildWithProperty(PropertyIds::ID, id.toString());

	if (propTree.isValid())
		return propTree.getPropertyAsValue(PropertyIds::Value, getUndoManager(), true);

	return {};
}

NodeComponent* NodeBase::createComponent()
{
	return new NodeComponent(this);
}


juce::Rectangle<int> NodeBase::getPositionInCanvas(Point<int> topLeft) const
{
	using namespace UIValues;

	Rectangle<int> body(NodeWidth, NodeHeight);

	body = body.withPosition(topLeft).reduced(NodeMargin);
	return body;
}

bool NodeBase::sendResizeMessage(Component* childComponent, bool async)
{
    if(auto p = childComponent->findParentComponentOfClass<DspNetworkGraph>())
    {
        auto f = [](DspNetworkGraph& g)
        {
            g.resizeNodes();
        };
        
        if(async)
            SafeAsyncCall::call<DspNetworkGraph>(*p, f);
        else
            f(*p);
        
        return true;
    }
    
    return false;
}
juce::var NodeBase::addModulationConnection(var source, Parameter* targetParameter)
{
	jassertfalse;
	return var();
}

snex::NamespacedIdentifier NodeBase::getPath() const
{
	auto t = getValueTree()[PropertyIds::FactoryPath].toString();
	return NamespacedIdentifier::fromString(t.replace(".", "::"));
}

void NodeBase::setBypassed(bool shouldBeBypassed)
{
	checkValid();

	bypassState = shouldBeBypassed;

}


var NodeBase::connectTo(var parameterTarget, var sourceInfo)
{
	if (auto p = dynamic_cast<Parameter*>(parameterTarget.getObject()))
	{
		return addModulationConnection(sourceInfo, p);
	}

	return var();
}

bool NodeBase::isBypassed() const noexcept
{
	checkValid();
	return bypassState;
}

int NodeBase::getIndexInParent() const
{
	return v_data.getParent().indexOf(v_data);
}

bool NodeBase::isActive(bool checkRecursively) const
{
	ValueTree p = v_data.getParent();

	if (!checkRecursively)
		return p.isValid();

	while (p.isValid() && p.getType() != PropertyIds::Network)
	{
		p = p.getParent();
	}

	return p.getType() == PropertyIds::Network;
}

void NodeBase::checkValid() const
{
	getRootNetwork()->checkValid();
}

NodeBase* NodeBase::getParentNode() const
{
	if (parentNode != nullptr)
	{
		return parentNode;
	}

	auto v = v_data.getParent().getParent();

	if (v.getType() == PropertyIds::Node)
	{
		return getRootNetwork()->getNodeForValueTree(v);
	}

	return nullptr;
}


juce::ValueTree NodeBase::getValueTree() const
{
	return v_data;
}

juce::String NodeBase::getId() const
{
	return v_data[PropertyIds::ID].toString();
}

juce::UndoManager* NodeBase::getUndoManager(bool returnIfPending) const
{
	return getRootNetwork()->getUndoManager(returnIfPending);
}

juce::Rectangle<int> NodeBase::getBoundsToDisplay(Rectangle<int> originalHeight) const
{
	auto titleWidth = GLOBAL_BOLD_FONT().getStringWidthFloat(getName());
	auto minWidth = jmax<int>(UIValues::NodeWidth, titleWidth + UIValues::HeaderHeight * 4);

	if (v_data[PropertyIds::Folded])
	{
		
		originalHeight = originalHeight.withHeight(UIValues::HeaderHeight).withWidth(minWidth);
	}

	if(originalHeight.getWidth() < minWidth)
		originalHeight.setWidth(minWidth);

	auto helpBounds = helpManager.getHelpSize().toNearestInt();

	if (!helpBounds.isEmpty())
	{
		if(!helpManager.isHelpBelow())
		{
			originalHeight.setWidth(originalHeight.getWidth() + helpBounds.getWidth());
			originalHeight.setHeight(jmax<int>(originalHeight.getHeight(), helpBounds.getHeight()));
		}
		else
		{
			originalHeight.setWidth(jmax<int>(originalHeight.getWidth(), helpBounds.getWidth()));
			originalHeight.setHeight(originalHeight.getHeight() + helpBounds.getHeight());
		}
	}

	if (getRootNetwork()->getExceptionHandler().getErrorMessage(this).isNotEmpty())
		originalHeight.setHeight(jmax(originalHeight.getHeight(), 150));

	return originalHeight;
}



juce::Rectangle<int> NodeBase::getBoundsWithoutHelp(Rectangle<int> originalHeight) const
{
	auto helpBounds = helpManager.getHelpSize().toNearestInt();

	auto isBelow = helpManager.isHelpBelow();

	if(isBelow)
		originalHeight.removeFromBottom(helpBounds.getHeight());
	else
		originalHeight.removeFromRight(helpBounds.getWidth());

	if (v_data[PropertyIds::Folded])
		return originalHeight.withHeight(UIValues::HeaderHeight);
	else
	{
		return originalHeight;
	}
}

int NodeBase::getNumParameters() const
{
	return parameters.size();
}


juce::var NodeBase::getChildNodes(bool recursive)
{
	Array<var> nodes;

	if (auto asContainer = dynamic_cast<NodeContainer*>(this))
	{
		auto list = asContainer->getNodeList();

		for (auto cn : list)
		{
			var childNode(cn);

			nodes.add(childNode);

			if (recursive)
			{
				auto cArray = cn->getChildNodes(true);
				nodes.addArray(*cArray.getArray());
			}
		}
	}

	return var(nodes);
}

NodeBase::Parameter* NodeBase::getParameterFromName(const String& id) const
{
	for (auto p : parameters)
		if (p->getId() == id)
			return p;

	return nullptr;
}


NodeBase::Parameter* NodeBase::getParameterFromIndex(int index) const
{
	if (isPositiveAndBelow(index, parameters.size()))
	{
		return parameters[index].get();
	}
    
    return nullptr;
}

struct ParameterSorter
{
	static int compareElements(const Parameter* first, const Parameter* second)
	{
		const auto& ftree = first->data;
		const auto& stree = second->data;

		int fIndex = ftree.getParent().indexOf(ftree);
		int sIndex = stree.getParent().indexOf(stree);

		if (fIndex < sIndex)
			return -1;

		if (fIndex > sIndex)
			return 1;

		jassertfalse;
		return 0;
	}
};

void NodeBase::addParameter(Parameter* p)
{
	ParameterSorter sorter;

	parameters.addSorted(sorter, p);
}


void NodeBase::removeParameter(int index)
{
	parameters.remove(index);
}

void NodeBase::removeParameter(const String& id)
{
    for (int i=0; i<getNumParameters(); i++)
    {
        if (parameters[i]->getId() == id)
        {
            removeParameter(i);
            return;
        }
    }
}

void NodeBase::setParentNode(Ptr newParentNode)
{
	if (newParentNode == nullptr && getRootNetwork() != nullptr)
	{
		getRootNetwork()->getExceptionHandler().removeError(this);

		if (auto nc = dynamic_cast<NodeContainer*>(this))
		{
			nc->forEachNode([](NodeBase::Ptr b)
			{
				b->getRootNetwork()->getExceptionHandler().removeError(b);
				return false;
			});
		}
	}

	parentNode = newParentNode;
}

void NodeBase::showPopup(Component* childOfGraph, Component* c)
{
	auto g = childOfGraph->findParentComponentOfClass<ZoomableViewport>();
	auto b = g->getLocalArea(childOfGraph, childOfGraph->getLocalBounds());
	g->setCurrentModalWindow(c, b);
}

String NodeBase::getCpuUsageInPercent() const
{
	String s;

	if (lastSpecs.sampleRate > 0.0 && lastSpecs.blockSize > 0)
	{
		auto secondPerBuffer = cpuUsage * 0.001;
		auto bufferDuration = (double)lastSpecs.blockSize / lastSpecs.sampleRate;
		auto bufferRatio = secondPerBuffer / bufferDuration;
		s << " - " << String(bufferRatio * 100.0, 1) << "%";
	}

	return s;
}

bool NodeBase::isClone() const
{
	return findParentNodeOfType<CloneNode>() != nullptr;
}

void NodeBase::setEmbeddedNetwork(NodeBase::Holder* n)
{
	embeddedNetwork = n;

	if (getEmbeddedNetwork()->canBeFrozen())
	{
		setDefaultValue(PropertyIds::Frozen, true);
		frozenListener.setCallback(v_data, { PropertyIds::Frozen }, valuetree::AsyncMode::Synchronously,
			BIND_MEMBER_FUNCTION_2(NodeBase::updateFrozenState));
	}
}

scriptnode::DspNetwork* NodeBase::getEmbeddedNetwork()
{
	return static_cast<DspNetwork*>(embeddedNetwork.get());
}

const scriptnode::DspNetwork* NodeBase::getEmbeddedNetwork() const
{
	return static_cast<const DspNetwork*>(embeddedNetwork.get());
}

ValueTree findBypassConnectionTree(const ValueTree& v, const String& nodeId)
{
	if (v.getType() == PropertyIds::Connection)
	{
		auto thisNode = v[PropertyIds::NodeId].toString();
		auto isParam = v[PropertyIds::ParameterId].toString() == PropertyIds::Bypassed.toString();

		if (isParam && thisNode == nodeId)
			return v;
	}

	for (const auto& c : v)
	{
		auto d = findBypassConnectionTree(c, nodeId);
		if (d.isValid())
			return d;
	}

	return {};
}

ValueTree findParentTreeOfType(const ValueTree& v, const Identifier& t)
{
	if (!v.isValid())
		return v;

	if (v.getType() == t)
		return v;

	return findParentTreeOfType(v.getParent(), t);
}

String NodeBase::getDynamicBypassSource(bool forceUpdate /*= true*/) const
{
	if (!forceUpdate)
		return dynamicBypassId;

	auto cTree = findBypassConnectionTree(getRootNetwork()->getValueTree(), getId());

	dynamicBypassId = {};

	if (cTree.isValid())
	{
		auto srcNode = findParentTreeOfType(cTree, PropertyIds::Node);
		auto pNode = findParentTreeOfType(cTree, PropertyIds::Parameter);

		dynamicBypassId << srcNode[PropertyIds::ID].toString();

		if (pNode.isValid())
			dynamicBypassId << "." << pNode[PropertyIds::ID].toString();
		else
		{
			auto sTree = findParentTreeOfType(cTree, PropertyIds::SwitchTargets);

			if (sTree.isValid())
			{
				auto sChild = findParentTreeOfType(cTree, PropertyIds::SwitchTarget);
				auto idx = sTree.indexOf(sChild);

				dynamicBypassId << "[" << String(idx) << "]";
			}
		}
	}

	return dynamicBypassId;
}

void NodeBase::updateFrozenState(Identifier id, var newValue)
{
	if (auto n = getEmbeddedNetwork())
	{
		try
		{
			if (n->canBeFrozen())
				n->setUseFrozenNode((bool)newValue);
		}
		catch (Error& e)
		{
			getRootNetwork()->getExceptionHandler().addError(this, e);
		}
	}
}

Colour NodeBase::getColour() const
{
    auto value = getValueTree()[PropertyIds::NodeColour];
    auto colour = PropertyHelpers::getColourFromVar(value);
    
    if(getRootNetwork()->getRootNode() == this)
    {
        return dynamic_cast<const Processor*>(getScriptProcessor())->getColour();
    }
    
    if(auto cont = dynamic_cast<const NodeContainer*>(this))
    {
        auto cc = cont->getContainerColour();
        
        if(!cc.isTransparent())
            return cc;
    }
    
    return colour;
}

var NodeBase::get(var id)
{
	checkValid();

	return getNodeProperty(id.toString());
}

void NodeBase::setParent(var parentNode, int indexInParent)
{
	checkValid();

	ScopedValueSetter<bool> svs(isCurrentlyMoved, true);

	auto network = getRootNetwork();

	// allow passing in the root network
	if (parentNode.getObject() == network)
		parentNode = var(network->getRootNode());

	Parameter::ScopedAutomationPreserver sap(this);

	if(getValueTree().getParent().isValid())
		getValueTree().getParent().removeChild(getValueTree(), getUndoManager());

	if (auto pNode = dynamic_cast<NodeContainer*>(network->get(parentNode).getObject()))
	{
		pNode->getNodeTree().addChild(getValueTree(), indexInParent, network->getUndoManager());
	}
		
	else
	{
		if (parentNode.toString().isNotEmpty())
			reportScriptError("parent node " + parentNode.toString() + " not found.");

		if (auto pNode = dynamic_cast<NodeContainer*>(getParentNode()))
		{
			pNode->getNodeTree().removeChild(getValueTree(), getUndoManager());
		}
	}
}

var NodeBase::getParameter(var indexOrId) const
{
	Parameter* p = nullptr;

	if (indexOrId.isString())
		p = getParameterFromName(indexOrId.toString());
	else
		p = getParameterFromIndex((int)indexOrId);

	if (p != nullptr)
		return var(p);
	else
    {
        if(auto nc = dynamic_cast<const NodeContainer*>(this))
        {
            auto name = indexOrId.toString();
            
            ValueTree p(PropertyIds::Parameter);
            p.setProperty(PropertyIds::ID, name, nullptr);
            p.setProperty(PropertyIds::MinValue, 0.0, nullptr);
            p.setProperty(PropertyIds::MaxValue, 1.0, nullptr);

            PropertyIds::Helpers::setToDefault(p, PropertyIds::StepSize);
            PropertyIds::Helpers::setToDefault(p, PropertyIds::SkewFactor);

            p.setProperty(PropertyIds::Value, 1.0, nullptr);
            getValueTree().getChildWithName(PropertyIds::Parameters).addChild(p, -1, getUndoManager());
            
            return var(getParameterFromName(name));
        }
        
        return {};
    }
		
}

struct Parameter::Wrapper
{
	API_METHOD_WRAPPER_0(NodeBase::Parameter, getId);
	API_METHOD_WRAPPER_0(NodeBase::Parameter, getValue);
    API_VOID_METHOD_WRAPPER_2(NodeBase::Parameter, setRangeProperty);
	API_METHOD_WRAPPER_1(NodeBase::Parameter, addConnectionFrom);
	API_VOID_METHOD_WRAPPER_1(NodeBase::Parameter, setValueSync);
	API_VOID_METHOD_WRAPPER_1(NodeBase::Parameter, setValueAsync);
	API_VOID_METHOD_WRAPPER_1(NodeBase::Parameter, setRangeFromObject);
	API_METHOD_WRAPPER_0(NodeBase::Parameter, getRangeObject);
};

Parameter::Parameter(NodeBase* parent_, const ValueTree& data_) :
	ConstScriptingObject(parent_->getScriptProcessor(), 4),
	parent(parent_),
	data(data_),
	dynamicParameter()
{
	auto weakThis = WeakReference<Parameter>(this);

	ADD_API_METHOD_0(getValue);
	ADD_API_METHOD_1(addConnectionFrom);
	ADD_API_METHOD_1(setValueAsync);
	ADD_API_METHOD_1(setValueSync);
    ADD_API_METHOD_2(setRangeProperty);
	ADD_API_METHOD_0(getId);
	ADD_API_METHOD_1(setRangeFromObject);
	ADD_API_METHOD_0(getRangeObject);

#define ADD_PROPERTY_ID_CONSTANT(id) addConstant(id.toString(), id.toString());

	ADD_PROPERTY_ID_CONSTANT(PropertyIds::MinValue);
	ADD_PROPERTY_ID_CONSTANT(PropertyIds::MaxValue);
	ADD_PROPERTY_ID_CONSTANT(PropertyIds::MidPoint);
	ADD_PROPERTY_ID_CONSTANT(PropertyIds::StepSize);

	valuePropertyUpdater.setCallback(data, { PropertyIds::Value }, valuetree::AsyncMode::Synchronously,
		std::bind(&NodeBase::Parameter::updateFromValueTree, this, std::placeholders::_1, std::placeholders::_2));

	rangeListener.setCallback(data, RangeHelpers::getRangeIds(), valuetree::AsyncMode::Synchronously, BIND_MEMBER_FUNCTION_2(NodeBase::Parameter::updateRange));

	automationRemover.setCallback(data, valuetree::AsyncMode::Synchronously, true, BIND_MEMBER_FUNCTION_1(Parameter::updateConnectionOnRemoval));
}

void Parameter::updateRange(Identifier, var)
{
	if (dynamicParameter != nullptr)
		dynamicParameter->updateRange(data);
}

void Parameter::updateConnectionOnRemoval(ValueTree& c)
{
	if (!ScopedAutomationPreserver::isPreservingRecursive(parent) && connectionSourceTree.isValid())
		connectionSourceTree.getParent().removeChild(connectionSourceTree, parent->getUndoManager());
}

NodeBase::Holder::~Holder()
{
	root = nullptr;
	nodes.clear();
}

NodeBase* NodeBase::Holder::getRootNode() const
{ return root.get(); }

void NodeBase::Holder::setRootNode(NodeBase::Ptr newRootNode)
{
	root = newRootNode;
}

NodeBase::DynamicBypassParameter::ScopedUndoDeactivator::ScopedUndoDeactivator(NodeBase* n):
	p(*n)
{
	prevState = p.enableUndo;
	p.enableUndo = false;
}

NodeBase::DynamicBypassParameter::ScopedUndoDeactivator::~ScopedUndoDeactivator()
{
	p.enableUndo = prevState;
}

NodeBase::DynamicBypassParameter::~DynamicBypassParameter()
{
	if (node != nullptr)
		node->dynamicBypassId = prevId;
}

void NodeBase::DynamicBypassParameter::call(double v)
{
	setDisplayValue(v);
	bypassed = !enabledRange.contains(v) && enabledRange.getEnd() != v;

	ScopedUndoDeactivator sns(node);

	node->setBypassed(bypassed);
}

Identifier NodeBase::getObjectName() const
{ return PropertyIds::Node; }

bool NodeBase::forEach(const std::function<bool(NodeBase::Ptr)>& f)
{
	return f(this);
}

void NodeBase::processMonoFrame(MonoFrameType& data)
{
	FrameType dynData(data);
	processFrame(dynData);
}

void NodeBase::processStereoFrame(StereoFrameType& data)
{
	FrameType dynData(data);
	processFrame(dynData);
}

bool NodeBase::isProcessingHiseEvent() const
{ return false; }

void NodeBase::handleHiseEvent(HiseEvent& e)
{
	ignoreUnused(e);
}

String NodeBase::getNodeDescription() const
{ return {}; }

ParameterDataList NodeBase::createInternalParameterList()
{ return {}; }

void NodeBase::processProfileInfo(double cpuUsage, int numSamples)
{
	lastBlockSize = numSamples;
	ignoreUnused(cpuUsage, numSamples);
}

bool NodeBase::setComplexDataIndex(String dataType, int dataSlot, int indexValue)
{
	auto v = getValueTree().getChildWithName(PropertyIds::ComplexData);
        
	if(!v.isValid())
		return false;
        
	auto types = dataType + "s";
	v = v.getChildWithName(Identifier(types));
        
	if(!v.isValid())
		return false;
        
	v = v.getChild(dataSlot);
        
	if(!v.isValid())
		return false;
        
	v.setProperty(PropertyIds::Index, dataSlot, getUndoManager());
        
	return true;
}

bool NodeBase::isPolyphonic() const
{ return false; }

bool NodeBase::isBodyShown() const
{
	if (v_data[PropertyIds::Folded])
		return false;

	if (auto p = getParentNode())
	{
		return p->isBodyShown();
	}

	return true;
}

HelpManager& NodeBase::getHelpManager()
{ return helpManager; }

ValueTree NodeBase::getParameterTree()
{ return v_data.getOrCreateChildWithName(PropertyIds::Parameters, getUndoManager()); }

ValueTree NodeBase::getPropertyTree()
{ return v_data.getOrCreateChildWithName(PropertyIds::Properties, getUndoManager()); }

bool NodeBase::isBeingMoved() const
{
	auto pNode = getParentNode();

	while (pNode != nullptr)
	{
		if (pNode->isCurrentlyMoved)
			return true;

		pNode = pNode->getParentNode();
	}

	return isCurrentlyMoved;
}

NodeBase::ParameterIterator::ParameterIterator(NodeBase& n): node(n), size(n.getNumParameters())
{}

Parameter** NodeBase::ParameterIterator::begin() const
{ return node.parameters.begin(); }

Parameter** NodeBase::ParameterIterator::end() const
{ return node.parameters.end(); }

void NodeBase::setCurrentId(const String& newId)
{
	currentId = newId;
}

String NodeBase::getCurrentId() const
{ return currentId; }

Rectangle<int> NodeBase::getExtraComponentBounds() const
{
	return {};
}

double& NodeBase::getCpuFlag()
{ return cpuUsage; }

bool& NodeBase::getPreserveAutomationFlag()
{ return preserveAutomation; }

int NodeBase::getCurrentChannelAmount() const
{ return lastSpecs.numChannels; }

int NodeBase::getNumChannelsToDisplay() const
{ return getCurrentChannelAmount(); }

int NodeBase::getCurrentBlockRate() const
{ return lastBlockSize; }

void NodeBase::setSignalPeaks(float* p, int numChannels, bool postSignal)
{
	auto& s = signalPeaks[(int)postSignal];

	for(int i = 0; i < numChannels; i++)
	{
		s[i] *= 0.5f;
		s[i] += 0.5f * p[i];
	}
}

float NodeBase::getSignalPeak(int channel, bool post) const
{ return signalPeaks[(int)post][channel]; }

void NodeBase::updateBypassState(Identifier, var newValue)
{
	auto shouldBeBypassed = (bool)newValue;
	setBypassed((bool)newValue);

	ignoreUnused(shouldBeBypassed);
        
	// This needs to be set in the virtual method above
	jassert(shouldBeBypassed == bypassState);
}

juce::String Parameter::getId() const
{
	return data[PropertyIds::ID].toString();
}


double Parameter::getValue() const
{
	if(dynamicParameter != nullptr)
		return dynamicParameter->getDisplayValue();

	return (double)data[PropertyIds::Value];
}

void Parameter::setDynamicParameter(parameter::dynamic_base::Ptr ownedNew)
{
	// We don't need to lock if the network isn't active yet...
	bool useLock = parent->isActive(true) && parent->getRootNetwork()->isInitialised();

	auto ph = parent->getRootNetwork()->getParentHolder();

	if(ph == nullptr)
		return;

	SimpleReadWriteLock::ScopedWriteLock sl(parent->getRootNetwork()->getConnectionLock(), useLock);

	dynamicParameter = ownedNew;

	if (dynamicParameter != nullptr)
	{
		dynamicParameter->updateRange(data);

		if (data.hasProperty(PropertyIds::Value))
			dynamicParameter->call((double)data[PropertyIds::Value]);
	}
}

void Parameter::setValueAsync(double newValue)
{
	if (dynamicParameter != nullptr)
	{
		DspNetwork::NoVoiceSetter nvs(*parent->getRootNetwork());
		dynamicParameter->call(newValue);
	}
}

juce::var Parameter::getRangeObject() const
{
	auto nr = RangeHelpers::getDoubleRange(data);

	auto obj = new DynamicObject();

	obj->setProperty(PropertyIds::MinValue, nr.rng.start);
	obj->setProperty(PropertyIds::MaxValue, nr.rng.end);
	obj->setProperty(PropertyIds::SkewFactor, nr.rng.skew);
	obj->setProperty(PropertyIds::StepSize, nr.rng.interval);
	obj->setProperty(PropertyIds::Inverted, nr.inv);

	return var(obj);
}

void Parameter::setRangeFromObject(var obj)
{
	InvertableParameterRange nr;
	
	nr.rng.start = obj.getProperty(PropertyIds::MinValue, 0.0);
	nr.rng.end = obj.getProperty(PropertyIds::MaxValue, 1.0);
	nr.rng.skew = obj.getProperty(PropertyIds::SkewFactor, 1.0);
	nr.rng.interval = obj.getProperty(PropertyIds::StepSize, 0.0);
	nr.inv = obj.getProperty(PropertyIds::Inverted, false);

	nr.checkIfIdentity();

	RangeHelpers::storeDoubleRange(data, nr, parent->getUndoManager());
}

void Parameter::setRangeProperty(String id, var newValue)
{
	Identifier i(id);

	if (RangeHelpers::isRangeId(i))
	{
		data.setProperty(i, newValue, nullptr);
	}
}

void Parameter::setValueSync(double newValue)
{
	data.setProperty(PropertyIds::Value, newValue, parent->getUndoManager());
}

juce::ValueTree Parameter::getConnectionSourceTree(bool forceUpdate)
{
	if (forceUpdate)
	{
		auto pId = getId();
		auto nId = parent->getId();
		auto n = parent->getRootNetwork();

		{
			auto containers = n->getListOfNodesWithType<NodeContainer>(false);

			for (auto c : containers)
			{
				for (auto p : c->getParameterTree())
				{
					auto cTree = p.getChildWithName(PropertyIds::Connections);

					for (auto c : cTree)
					{
						if (c[PropertyIds::NodeId].toString() == nId &&
							c[PropertyIds::ParameterId].toString() == pId)
						{
							connectionSourceTree = c;
							return c;
						}
					}
				}
			}
		}

		{
			auto modNodes = n->getListOfNodesWithType<WrapperNode>(false);

			for (auto mn : modNodes)
			{
				auto mTree = mn->getValueTree().getChildWithName(PropertyIds::ModulationTargets);

				for (auto mt : mTree)
				{
					if (mt[PropertyIds::NodeId].toString() == nId &&
						mt[PropertyIds::ParameterId].toString() == pId)
					{
						connectionSourceTree = mt;
						return mt;
					}
				}

				auto sTree = mn->getValueTree().getChildWithName(PropertyIds::SwitchTargets);

				for (auto sts : sTree)
				{
					for (auto st : sts.getChildWithName(PropertyIds::Connections))
					{
						if (st[PropertyIds::NodeId].toString() == nId &&
							st[PropertyIds::ParameterId].toString() == pId)
						{
							connectionSourceTree = st;
							return st;
						}
					}
				}
			}
		}

		return {};
	}

	return connectionSourceTree;
	
}

struct DragHelpers
{
	static var createDescription(const String& sourceNodeId, const String& parameterId, bool isMod=false)
	{
		DynamicObject::Ptr details = new DynamicObject();

		details->setProperty(PropertyIds::Automated, isMod);
		details->setProperty(PropertyIds::ID, sourceNodeId);
		details->setProperty(PropertyIds::ParameterId, parameterId);
		
		return var(details.get());
	}

	static String getSourceNodeId(var dragDetails)
	{
		if (dragDetails.isString())
		{
			return dragDetails.toString().upToFirstOccurrenceOf(".", false, false);
		}

		return dragDetails.getProperty(PropertyIds::ID, "").toString();
	}

	static String getSourceParameterId(var dragDetails)
	{
		if (dragDetails.isString())
			return dragDetails.toString().fromFirstOccurrenceOf(".", false, true);

		return dragDetails.getProperty(PropertyIds::ParameterId, "").toString();
	}

	static ModulationSourceNode* getModulationSource(NodeBase* parent, var dragDetails)
	{
		if (dragDetails.isString())
		{
			return dynamic_cast<ModulationSourceNode*>(parent->getRootNetwork()->getNodeWithId(dragDetails.toString()));
		}

		if ((bool)dragDetails.getProperty(PropertyIds::Automated, false))
		{
			auto sourceNodeId = getSourceNodeId(dragDetails);
			auto list = parent->getRootNetwork()->getListOfNodesWithType<ModulationSourceNode>(false);

			for (auto l : list)
			{
				if (l->getId() == sourceNodeId)
					return dynamic_cast<ModulationSourceNode*>(l.get());
			}
		}

		return nullptr;
	}

	static ValueTree getValueTreeOfSourceParameter(NodeBase* parent, var dragDetails)
	{
		auto sourceNodeId = getSourceNodeId(dragDetails);
		auto pId = getSourceParameterId(dragDetails);

		if (dragDetails.getProperty(PropertyIds::SwitchTarget, false))
		{
			auto st = parent->getRootNetwork()->getNodeWithId(sourceNodeId)->getValueTree().getChildWithName(PropertyIds::SwitchTargets);

			jassert(st.isValid());

			auto v = st.getChild(pId.getIntValue());
			jassert(v.isValid());
			return v;
		}

		if (auto sourceContainer = dynamic_cast<NodeContainer*>(parent->getRootNetwork()->get(sourceNodeId).getObject()))
		{
			return sourceContainer->asNode()->getParameterTree().getChildWithProperty(PropertyIds::ID, pId);
		}

		return {};
	}
};

void NodeBase::connectToBypass(var dragDetails)
{
	auto sourceParameterTree = DragHelpers::getValueTreeOfSourceParameter(this, dragDetails);

	if (sourceParameterTree.isValid())
	{
		ValueTree newC(PropertyIds::Connection);
		newC.setProperty(PropertyIds::NodeId, getId(), nullptr);
		newC.setProperty(PropertyIds::ParameterId, PropertyIds::Bypassed.toString(), nullptr);

		String connectionId = DragHelpers::getSourceNodeId(dragDetails) + "." + 
							  DragHelpers::getSourceParameterId(dragDetails);

		ValueTree connectionTree = sourceParameterTree.getChildWithName(PropertyIds::Connections);
		connectionTree.addChild(newC, -1, getUndoManager());
	}
	else
	{
		auto src = getDynamicBypassSource(true);

		if(src.containsChar('.'))
		{
			if (auto srcNode = getRootNetwork()->getNodeWithId(src.upToFirstOccurrenceOf(".", false, false)))
			{
				if (auto srcParameter = srcNode->getParameterFromName(src.fromFirstOccurrenceOf(".", false, false)))
				{
					for (auto c : srcParameter->data.getChildWithName(PropertyIds::Connections))
					{
						if (c[PropertyIds::NodeId] == getId() && c[PropertyIds::ParameterId].toString() == "Bypassed")
						{
							c.getParent().removeChild(c, getUndoManager());
							return;
						}
					}
				}
			}
		}
		else if (src.containsChar('['))
		{
			if (auto srcNode = getRootNetwork()->getNodeWithId(src.upToFirstOccurrenceOf("[", false, false)))
			{
				auto stree = srcNode->getValueTree().getChildWithName(PropertyIds::SwitchTargets);
				auto slotIndex = src.fromFirstOccurrenceOf("[", false, false).getIntValue();

				for (auto c : stree.getChild(slotIndex).getChildWithName(PropertyIds::Connections))
				{
					if (c[PropertyIds::NodeId] == getId() && c[PropertyIds::ParameterId].toString() == "Bypassed")
					{
						c.getParent().removeChild(c, getUndoManager());
						return;
					}
				}
			}
		}
	}
}

var NodeBase::Parameter::addConnectionFrom(var dragDetails)
{
	auto shouldAdd = dragDetails.isObject();

	if (shouldAdd)
	{
		if (data[PropertyIds::Automated])
			return var();

		data.setProperty(PropertyIds::Automated, true, parent->getUndoManager());

		auto sourceNodeId = DragHelpers::getSourceNodeId(dragDetails);
		auto parameterId = DragHelpers::getSourceParameterId(dragDetails);

		if (auto modSource = DragHelpers::getModulationSource(parent, dragDetails))
			return modSource->addModulationConnection(0, this);

		if (sourceNodeId == parent->getId() && parameterId == getId())
			return {};

		if (auto sn = parent->getRootNetwork()->getNodeWithId(sourceNodeId))
			return sn->addModulationConnection(var(parameterId), this);

		return {};
	}
	else
	{
		auto c = getConnectionSourceTree(true);

		if (c.isValid())
		{
			data.setProperty(PropertyIds::Automated, false, parent->getUndoManager());
			c.getParent().removeChild(c, parent->getUndoManager());
		}

		connectionSourceTree = {};

		return {};
	}
}

bool NodeBase::Parameter::matchesConnection(const ValueTree& c) const
{
	if (c.hasType(PropertyIds::Node))
	{
		auto isParent = parent->getValueTree() == c;
		auto isParentOfParent = parent->getValueTree().isAChildOf(c);
		
		return isParent || isParentOfParent;
	}

	auto matchesNode = c[PropertyIds::NodeId].toString() == parent->getId();
	auto matchesParameter = c[PropertyIds::ParameterId].toString() == getId();
	return matchesNode && matchesParameter;
}

juce::Array<NodeBase::Parameter*> NodeBase::Parameter::getConnectedMacroParameters() const
{
	Array<Parameter*> list;

	if (auto n = parent)
	{
		while ((n = n->getParentNode()) != nullptr)
		{
			for (auto m : NodeBase::ParameterIterator(*n))
			{
				if (auto mp = dynamic_cast<NodeContainer::MacroParameter*>(m))
				{
					if(mp->isConnectedToSource(this))
						list.add(m);
				}
			}
		}
	}

	return list;
}

HelpManager::HelpManager(NodeBase* parent_, ValueTree d) :
	ControlledObject(parent_->getScriptProcessor()->getMainController_()),
	parent(*parent_)
{
	commentListener.setCallback(d, { PropertyIds::Comment, PropertyIds::NodeColour}, valuetree::AsyncMode::Asynchronously,
		BIND_MEMBER_FUNCTION_2(HelpManager::update));
}

HelpManager::~HelpManager()
{
	if(commentButton != nullptr && commentButton->getParentComponent() != nullptr)
		commentButton->getParentComponent()->removeChildComponent(commentButton);

	commentButton = nullptr;
}

void HelpManager::setCommentTooltip()
{
	auto firstLine = lastText.upToFirstOccurrenceOf("\n", false, false);

	if(lastText.length() != firstLine.length())
		firstLine << " [...] (click to show full content)";

	if(commentButton != nullptr)
		commentButton->setTooltip(firstLine);
}

void HelpManager::update(Identifier id, var newValue)
{
	if (id == PropertyIds::NodeColour)
	{
		highlightColour = PropertyHelpers::getColourFromVar(newValue);
		if (highlightColour.isTransparent())
			highlightColour = Colour(SIGNAL_COLOUR);

		if (helpRenderer != nullptr)
		{
			helpRenderer->getStyleData().headlineColour = highlightColour;

			helpRenderer->setNewText(lastText);

			for (auto l : listeners)
			{
				if (l != nullptr)
					l->repaintHelp();
			}
		}
	}
	else if (id == PropertyIds::Comment)
	{
		lastText = newValue.toString();

		setCommentTooltip();

		auto f = GLOBAL_BOLD_FONT();

		auto sa = StringArray::fromLines(lastText);
		lastWidth = 0.0f;

		for (auto s : sa)
			lastWidth = jmax(f.getStringWidthFloat(s) + 10.0f, lastWidth);

		lastWidth = jmin(300.0f, lastWidth);
		rebuild();
	}
}

void HelpManager::render(Graphics& g, Rectangle<float> area)
{
	if (helpRenderer != nullptr && !area.isEmpty())
	{
		if(!showCommentButton())
		{
			if(!isHelpBelow())
				area.removeFromLeft(UIValues::NodeMargin);
			else
				area.removeFromTop(UIValues::NodeMargin);

			auto c = highlightColour == Colour(SIGNAL_COLOUR) ? Colours::black : highlightColour;
			g.setColour(c.withAlpha(0.1f));
			g.fillRoundedRectangle(area, 2.0f);
			helpRenderer->draw(g, area.reduced(10.0f));
		}
		else
		{
			auto b = area.toNearestInt().reduced(3);

			if(commentButton != nullptr && commentButton->getBoundsInParent() != b)
				commentButton->setBounds(b);
		}
	}
}

void HelpManager::addHelpListener(Listener* l)
{
	listeners.addIfNotAlreadyThere(l);
	//l->helpChanged(lastWidth + 30.0f, lastHeight + 20.0f);
}

void HelpManager::removeHelpListener(Listener* l)
{
	listeners.removeAllInstancesOf(l);
}

void HelpManager::initCommentButton(Component* parentComponent)
{
	if(commentButton != nullptr)
	{
		if(auto pc = commentButton->getParentComponent())
		pc->removeChildComponent(commentButton);
	}
	
	if(lastText.isNotEmpty())
	{
		auto showComments = (bool)dynamic_cast<NodeComponent*>(parentComponent)->node->getRootNetwork()->getValueTree()[PropertyIds::ShowComments];

		if(commentButton == nullptr)
		{
			commentButton = new HiseShapeButton("comment", nullptr, *this);

			setCommentTooltip();

			commentButton->onClick = [this]()
			{
				setShowComments(true);
				commentButton->findParentComponentOfClass<DspNetworkGraph>()->resizeNodes();
			};
		}
			

		parentComponent->addChildComponent(commentButton);

		setShowComments(showComments);
	}
}

void HelpManager::setShowComments(bool shouldShowComments)
{
	showButton = !shouldShowComments;

	if(commentButton != nullptr)
		commentButton->setVisible(showButton);
}

juce::Rectangle<float> HelpManager::getHelpSize() const
{
	if(showCommentButton())
	{
		if(lastHeight != 0.0f || lastWidth != 0.0f)
			return { 0.0f, 0.0f, 30.0f, 30.0f };

		return {};
	}
	else
	{
		return { 0.0f, 0.0f, lastHeight > 0.0f ? lastWidth + 30.0f : 0.0f, lastHeight > 0.0f ? lastHeight + 20.0f : 0.0f };
	}
}

bool HelpManager::isHelpBelow() const
{
	if(auto pn = dynamic_cast<SerialNode*>(parent.getParentNode()))
	{
		return !pn->isVertical.getValue();
	}

	return false;
}

void HelpManager::rebuild()
{
	if(commentButton != nullptr)
		commentButton->setVisible(showCommentButton());

	if (lastText.isNotEmpty())
	{
		helpRenderer = new MarkdownRenderer(lastText);
		helpRenderer->setDatabaseHolder(dynamic_cast<MarkdownDatabaseHolder*>(getMainController()));
		helpRenderer->getStyleData().headlineColour = highlightColour;
		helpRenderer->setDefaultTextSize(15.0f);
		helpRenderer->parse();
		lastHeight = helpRenderer->getHeightForWidth(lastWidth);
	}
	else
	{
		helpRenderer = nullptr;
		lastHeight = 0.0f;
	}

	if(!showCommentButton())
	{
		for (auto l : listeners)
		{
			if (l != nullptr)
				l->helpChanged(lastWidth + 30.0f, lastHeight);
		}
	}
}

struct ConnectionBase::Wrapper
{
	API_METHOD_WRAPPER_1(ConnectionBase,		getSourceNode);
	API_VOID_METHOD_WRAPPER_0(ConnectionBase,	disconnect);
	API_METHOD_WRAPPER_0(ConnectionBase,		isConnected);
	API_METHOD_WRAPPER_0(ConnectionBase,		getConnectionType);
	API_METHOD_WRAPPER_0(ConnectionBase,		getUpdateRate);
	API_METHOD_WRAPPER_0(ConnectionBase,		getTarget);
};

ConnectionBase::ConnectionBase(DspNetwork* network_, ValueTree data_) :
	ConstScriptingObject(network_->getScriptProcessor(), 0),
	network(network_),
	data(data_)
{
    jassert(data.getType() == PropertyIds::Connection || data.getType() == PropertyIds::ModulationTarget);
    
	ADD_API_METHOD_0(getTarget);
	ADD_API_METHOD_1(getSourceNode);
	ADD_API_METHOD_0(disconnect);
	ADD_API_METHOD_0(isConnected);
	ADD_API_METHOD_0(getConnectionType);
	ADD_API_METHOD_0(getUpdateRate);

	auto nodeId = data[PropertyIds::NodeId].toString();

	auto nodeTree = findParentTreeOfType(data, PropertyIds::Node);
	sourceNode = network->getNodeForValueTree(nodeTree);

	if (auto targetNode = network->getNodeWithId(nodeId))
	{
		for(auto p: NodeBase::ParameterIterator(*targetNode))
		{
			if (p->getId() == data[PropertyIds::ParameterId].toString())
			{
				targetParameter = p;
				break;
			}
		}
	}

	if ((sourceInSignalChain = Helpers::findRealSource(sourceNode)))
	{
		if (targetParameter != nullptr)
		{
			auto containerTree = Helpers::findCommonParent(sourceInSignalChain->getValueTree(), targetParameter->data);
			commonContainer = network->getNodeForValueTree(containerTree.getParent());
		}
	}
}

scriptnode::parameter::dynamic_base::Ptr ConnectionBase::createParameterFromConnectionTree(NodeBase* n, const ValueTree& connectionTree, bool scaleInput)
{
	static const Array<Identifier> validIds =
	{
		PropertyIds::Connections,
		PropertyIds::ModulationTargets,
		PropertyIds::SwitchTargets
	};

	jassert(validIds.contains(connectionTree.getType()));

	parameter::dynamic_base::Ptr chain;
	
	auto numConnections = connectionTree.getNumChildren();

	if (numConnections == 0)
		return nullptr;

	auto inputRange = RangeHelpers::getDoubleRange(connectionTree.getParent());

	for (auto c : connectionTree)
	{
		auto nId = c[PropertyIds::NodeId].toString();
		auto pId = c[PropertyIds::ParameterId].toString();
		auto tn = n->getRootNetwork()->getNodeWithId(nId);

		bool isUnscaledTarget = false;

		if (tn == nullptr)
			return nullptr;

		n->getRootNetwork()->getExceptionHandler().removeError(tn, Error::UnscaledModRangeMismatch);


		parameter::dynamic_base::Ptr p;

		if (pId == PropertyIds::Bypassed.toString())
		{
			if (auto validNode = dynamic_cast<SoftBypassNode*>(tn))
			{
				p = new NodeBase::DynamicBypassParameter(tn, {});
			}
			else
			{
				tn->getRootNetwork()->getExceptionHandler().addCustomError(tn, Error::IllegalBypassConnection, "Can't add a bypass here");
				return nullptr;
			}
		}
		else if (auto param = tn->getParameterFromName(pId))
		{
			p = param->getDynamicParameter();

			isUnscaledTarget = cppgen::CustomNodeProperties::isUnscaledParameter(param->data);
		}
		else
			return nullptr;


		if (numConnections == 1)
		{
			auto sameRange = RangeHelpers::equalsWithError(p->getRange(), inputRange, 0.001);
			
			if (!scaleInput || sameRange || isUnscaledTarget)
				return p;
		}

		if (chain == nullptr)
		{
			if (scaleInput)
				chain = new parameter::dynamic_chain<true>();
			else
				chain = new parameter::dynamic_chain<false>();

			chain->updateRange(connectionTree.getParent());
		}

		if (scaleInput)
			dynamic_cast<parameter::dynamic_chain<true>*>(chain.get())->addParameter(p, isUnscaledTarget);
		else
			dynamic_cast<parameter::dynamic_chain<false>*>(chain.get())->addParameter(p, isUnscaledTarget);
	}

	return chain;
}

juce::var ConnectionBase::getSourceNode(bool getSignalSource) const
{
	if (!isConnected())
		return var();

	if (getSignalSource)
		return sourceInSignalChain != nullptr ? var(sourceInSignalChain) : var();
	else
		return sourceNode != nullptr ? var(sourceNode) : var();
}

void ConnectionBase::disconnect()
{
	data.getParent().removeChild(data, network->getUndoManager());
}

bool ConnectionBase::isConnected() const
{
	return data.getParent().isValid();
}

juce::var ConnectionBase::getTarget() const
{
	return var(targetParameter.get());
}

int ConnectionBase::getConnectionType() const
{
	return (int)type;
}

int ConnectionBase::getUpdateRate() const
{
	if (commonContainer != nullptr)
		return commonContainer->getCurrentBlockRate();

	return 0;
}

juce::ValueTree ConnectionSourceManager::Helpers::getOrCreateConnection(ValueTree connectionTree, const String& nodeId, const String& parameterId, UndoManager* um)
{
	for (const auto& c : connectionTree)
	{
		if (c[PropertyIds::NodeId].toString() == nodeId &&
			c[PropertyIds::ParameterId].toString() == parameterId)
		{
			return c;
		}
	}

	ValueTree newC("Connection");
	newC.setProperty(PropertyIds::NodeId, nodeId, nullptr);
	newC.setProperty(PropertyIds::ParameterId, parameterId, nullptr);

	connectionTree.addChild(newC, -1, um);

	return newC;
}

ProcessDataPeakChecker::ProcessDataPeakChecker(NodeBase* n, ProcessDataDyn& d_) :
	p(*n),
	d(d_)
{
	check(false);
}

ProcessDataPeakChecker::~ProcessDataPeakChecker()
{
	check(true);
}

void ProcessDataPeakChecker::check(bool post)
{
#if USE_BACKEND
	if (!p.getRootNetwork()->isSignalDisplayEnabled())
		return;

	span<float, NUM_MAX_CHANNELS> peaks;

	int index = 0;

	int halfIndex = d.getNumSamples() / 2;

	for (auto& ch : d)
	{
		auto b = d.toChannelData(ch);

		auto first = b[0];
		auto half = b[halfIndex];
		auto peak = jmax(hmath::abs(first), hmath::abs(half));
		peaks[index++] = peak;
	}

	p.setSignalPeaks(peaks.begin(), d.getNumChannels(), post);
#endif
}

RealNodeProfiler::RealNodeProfiler(NodeBase* n, int numSamples_) :
	enabled(n->getRootNetwork()->getCpuProfileFlag()),
	profileFlag(n->getCpuFlag()),
	numSamples(numSamples_),
	node(n)
{
	if (enabled)
		start = Time::getMillisecondCounterHiRes();
}

RealNodeProfiler::~RealNodeProfiler()
{
	if (enabled)
	{
		auto delta = Time::getMillisecondCounterHiRes() - start;
		profileFlag = profileFlag * 0.9 + 0.1 * delta;

		node->processProfileInfo(profileFlag, numSamples);
	}
}

Parameter::ScopedAutomationPreserver::ScopedAutomationPreserver(NodeBase* n) :
	parent(n)
{
	prevValue = parent->getPreserveAutomationFlag();
	parent->getPreserveAutomationFlag() = true;
}

Parameter::ScopedAutomationPreserver::~ScopedAutomationPreserver()
{
	parent->getPreserveAutomationFlag() = prevValue;
}

bool Parameter::ScopedAutomationPreserver::isPreservingRecursive(NodeBase* n)
{
	if (n == nullptr)
		return false;

	if (n->getPreserveAutomationFlag())
		return true;

	return isPreservingRecursive(n->getParentNode());
}

void ConnectionSourceManager::CableRemoveListener::removeCable(ValueTree& v)
{
	if (auto node = parent.n->getNodeForValueTree(v))
	{
		if (!node->isBeingMoved())
			data.getParent().removeChild(data, parent.n->getUndoManager(false));
	}
}

juce::ValueTree ConnectionSourceManager::CableRemoveListener::findTargetNodeData(const ValueTree& recursiveTree)
{
	if (recursiveTree.getType() == PropertyIds::Node)
	{
		auto nodeId = data[PropertyIds::NodeId].toString();

		if (recursiveTree[PropertyIds::ID] == nodeId)
		{
			auto parameterId = data[PropertyIds::ParameterId].toString();

			if (parameterId == PropertyIds::Bypassed.toString())
				return recursiveTree;

			for (auto p : recursiveTree.getChildWithName(PropertyIds::Parameters))
			{
				if (p[PropertyIds::ID] == parameterId)
					return recursiveTree;
			}
		}
	}

	auto nodeList = recursiveTree.getChildWithName(PropertyIds::Nodes);

	for(auto n: nodeList)
	{
		auto d = findTargetNodeData(n);
		if (d.isValid())
			return d;
	}

	return {};
}

ConnectionSourceManager::CableRemoveListener::CableRemoveListener(ConnectionSourceManager& parent_, ValueTree connectionData, ValueTree sourceNodeData) :
	parent(parent_),
	data(connectionData),
	sourceNode(sourceNodeData)
{
	targetNode = findTargetNodeData(parent.n->getValueTree().getChildWithName(PropertyIds::Node));

    if(!sourceNode.isValid() || !targetNode.isValid())
    {
        WeakReference<CableRemoveListener> safeThis(this);
        
        parent.n->addPostInitFunction([safeThis]()
        {
            if(safeThis.get() != nullptr)
                return safeThis->initListeners();
            
            return true;
        });
    }
    
    initListeners();
}

bool ConnectionSourceManager::CableRemoveListener::initListeners()
{
    targetNode = findTargetNodeData(parent.n->getValueTree().getChildWithName(PropertyIds::Node));
    
    if(!targetNode.isValid())
        return false;
    
    jassert(data.hasType(PropertyIds::Connection) || data.hasType(PropertyIds::ModulationTarget));
    jassert(sourceNode.hasType(PropertyIds::Node));
    jassert(targetNode.hasType(PropertyIds::Node));

    RangeHelpers::removeRangeProperties(data, parent.n->getUndoManager(true));

    //targetNode->getValueTree()
    //parent->getValueTree()

    targetRemoveUpdater.setCallback(targetNode,
        valuetree::AsyncMode::Synchronously,
        true,
        BIND_MEMBER_FUNCTION_1(CableRemoveListener::removeCable));

    sourceRemoveUpdater.setCallback(sourceNode,
        valuetree::AsyncMode::Synchronously,
        true,
        BIND_MEMBER_FUNCTION_1(CableRemoveListener::removeCable));

    if (data[PropertyIds::ParameterId].toString() != PropertyIds::Bypassed.toString())
    {
        targetParameterTree = targetNode.getChildWithName(PropertyIds::Parameters).getChildWithProperty(PropertyIds::ID, data[PropertyIds::ParameterId]);

        jassert(targetParameterTree.isValid());

        targetParameterTree.setProperty(PropertyIds::Automated, true, parent.n->getUndoManager());

        targetRangeListener.setCallback(targetParameterTree, RangeHelpers::getRangeIds(false), valuetree::AsyncMode::Synchronously,
            [this](Identifier, var) { this->parent.rebuildCallback(); });
    }
    
    return true;
}

ConnectionSourceManager::CableRemoveListener::~CableRemoveListener()
{
	if (targetParameterTree.isValid())
	{
		auto um = parent.n != nullptr ? parent.n->getUndoManager(false) : nullptr;
		targetParameterTree.setProperty(PropertyIds::Automated, false, um);
	}
}

ConnectionSourceManager::ConnectionSourceManager(DspNetwork* n_, ValueTree connectionsTree_) :
	n(n_),
	connectionsTree(connectionsTree_)
{
	
}

void ConnectionSourceManager::initConnectionSourceListeners()
{
	connectionListener.setCallback(connectionsTree, valuetree::AsyncMode::Synchronously, BIND_MEMBER_FUNCTION_2(ConnectionSourceManager::connectionChanged));

	initialised = true;
}

bool ConnectionSourceManager::isConnectedToSource(const Parameter* target) const
{
	for (auto c : connectionsTree)
	{
		if (target->matchesConnection(c))
			return true;
	}

	return false;
}

juce::var ConnectionSourceManager::addTarget(NodeBase::Parameter* p)
{
	p->data.setProperty(PropertyIds::Automated, true, p->parent->getUndoManager());
	auto newC = Helpers::getOrCreateConnection(connectionsTree, p->parent->getId(), p->getId(), p->parent->getUndoManager());

	return var(new ConnectionBase(n, newC));
}

juce::ValueTree ConnectionSourceManager::Helpers::findParentNodeTree(const ValueTree& v)
{
	if (!v.isValid())
	{
		jassertfalse;
		return {};
	}

	if (v.getType() == PropertyIds::Node)
		return v;

	return findParentNodeTree(v.getParent());
}

void ConnectionSourceManager::connectionChanged(ValueTree v, bool wasAdded)
{
	if (wasAdded)
	{
		connections.add(new CableRemoveListener(*this, v, Helpers::findParentNodeTree(connectionsTree)));
	}
	else
	{
		for (auto c : connections)
		{
			if (c->data == v)
			{
				connections.removeObject(c);
				break;
			}
		}
	}

	rebuildCallback();
}



NodeBase::DynamicBypassParameter::DynamicBypassParameter(NodeBase* n, Range<double> enabledRange_) :
	node(n),
	enabledRange(enabledRange_),
	prevId(n->dynamicBypassId)
{
	enabledRange = { 0.5, 1.0 };

	auto v = n->getRootNetwork()->getValueTree();
	auto id = n->getId();

#if 0
	for (auto& nc : c)
	{
		for (int i = 0; i < nc->getNumParameters(); i++)
		{
			auto p = nc->getParameter(i);

			if (p == nullptr)
				continue;

			for (const auto& con : p->data.getChildWithName(PropertyIds::Connections))
			{
				if (con[PropertyIds::ParameterId].toString() == "Bypassed")
				{
					if (con[PropertyIds::NodeId].toString() == n->getId())
					{
						n->dynamicBypassId = nc->getId() + "." + p->getId();
					}

				}
			}
		}
	}
#endif
}

juce::ValueTree ConnectionBase::Helpers::findCommonParent(ValueTree v1, ValueTree v2)
{
	if (!v1.isValid())
		return v1;

	if (v2.isAChildOf(v1))
		return v1;

	return findCommonParent(v1.getParent(), v2);
}

scriptnode::NodeBase* ConnectionBase::Helpers::findRealSource(NodeBase* source)
{
	if (auto cableNode = dynamic_cast<InterpretedCableNode*>(source))
	{
		source = nullptr;

		auto valueParam = cableNode->getParameterFromIndex(0);

		jassert(valueParam != nullptr);

		if(valueParam != nullptr && valueParam->isModulated())
		{
            source = nullptr;
            
			for (auto allMod : cableNode->getRootNetwork()->getListOfNodesWithType<ModulationSourceNode>(false))
			{
				auto am = dynamic_cast<ModulationSourceNode*>(allMod.get());

				if (am->isConnectedToSource(valueParam))
					return findRealSource(am);
			}
		}
	}

	return source;
}

FrameDataPeakChecker::FrameDataPeakChecker(NodeBase* n, float* d, int s) :
	p(*n),
	b(d, s)
{
	check(false);
}

FrameDataPeakChecker::~FrameDataPeakChecker()
{
	check(true);
}

void FrameDataPeakChecker::check(bool post)
{
#if USE_BACKEND && ALLOW_FRAME_SIGNAL_CHECK
	p.setSignalPeaks(b.begin(), b.size(), post);
#endif
}

}


