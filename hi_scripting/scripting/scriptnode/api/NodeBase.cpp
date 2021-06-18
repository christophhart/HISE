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
	API_METHOD_WRAPPER_1(NodeBase, getParameterReference);
};



NodeBase::NodeBase(DspNetwork* rootNetwork, ValueTree data_, int numConstants_) :
	ConstScriptingObject(rootNetwork->getScriptProcessor(), 8),
	parent(rootNetwork),
	v_data(data_),
	helpManager(this, data_),
	currentId(v_data[PropertyIds::ID].toString()),
	subHolder(rootNetwork->getCurrentHolder())
{
	numChannels.referTo(data_, PropertyIds::NumChannels, nullptr, 2);

	setDefaultValue(PropertyIds::NumChannels, 2);
	setDefaultValue(PropertyIds::NodeColour, 0);
	setDefaultValue(PropertyIds::Comment, "");
	//setDefaultValue(PropertyIds::CommentWidth, 300);

	ADD_API_METHOD_0(reset);
	ADD_API_METHOD_2(set);
	ADD_API_METHOD_1(get);
	ADD_API_METHOD_1(setBypassed);
	ADD_API_METHOD_0(isBypassed);
	ADD_API_METHOD_2(setParent);
	ADD_API_METHOD_1(getParameterReference);

	bypassListener.setCallback(v_data, { PropertyIds::Bypassed }, valuetree::AsyncMode::Synchronously, [this](Identifier id, var newValue)
	{
		if (id == PropertyIds::Bypassed)
		{
			setBypassed((bool)newValue);
		}
	});

	for (auto c : getPropertyTree())
	{
		addConstant(c[PropertyIds::ID].toString(), c[PropertyIds::ID]);
	}

	DBG("Add " + getId());
}

NodeBase::~NodeBase()
{
	DBG("Remove " + getId());
}

void NodeBase::prepare(PrepareSpecs specs)
{
	jassert(!isUINodeOfDuplicate());

	numChannels = specs.numChannels;
	
	lastSpecs = specs;
	cpuUsage = 0.0;

	for (auto p : parameters)
	{
		if (p->isModulated())
			continue;

		auto v = p->getValue();
		p->setValueAndStoreAsync(v);
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

	setNodeProperty(id.toString(), value);
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


bool NodeBase::isBypassed() const noexcept
{
	checkValid();
	return bypassState;
}

int NodeBase::getIndexInParent() const
{
	return v_data.getParent().indexOf(v_data);
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

juce::UndoManager* NodeBase::getUndoManager() const
{
	return getRootNetwork()->getUndoManager();
}

juce::Rectangle<int> NodeBase::getBoundsToDisplay(Rectangle<int> originalHeight) const
{
	if (v_data[PropertyIds::Folded])
		originalHeight = originalHeight.withHeight(UIValues::HeaderHeight).withWidth(UIValues::NodeWidth);
	
	auto helpBounds = helpManager.getHelpSize().toNearestInt();

	if (!helpBounds.isEmpty())
	{
		originalHeight.setWidth(originalHeight.getWidth() + helpBounds.getWidth());
		originalHeight.setHeight(jmax<int>(originalHeight.getHeight(), helpBounds.getHeight()));
	}

	return originalHeight;
}



juce::Rectangle<int> NodeBase::getBoundsWithoutHelp(Rectangle<int> originalHeight) const
{
	auto helpBounds = helpManager.getHelpSize().toNearestInt();

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


NodeBase::Parameter* NodeBase::getParameter(const String& id) const
{
	for (auto p : parameters)
		if (p->getId() == id)
			return p;

	return nullptr;
}


NodeBase::Parameter* NodeBase::getParameter(int index) const
{
	if (isPositiveAndBelow(index, parameters.size()))
	{
		return parameters[index];
	}
    
    return nullptr;
}

struct ParameterSorter
{
	static int compareElements(const Parameter* first, const Parameter* second)
	{
		const auto& ftree = first->getTreeWithValue();
		const auto& stree = second->getTreeWithValue();

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
	auto& f = p->getReferenceToCallback();
	
	f.call(p->getValue());

	
}


void NodeBase::removeParameter(int index)
{
	parameters.remove(index);
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

void NodeBase::updateFrozenState(Identifier id, var newValue)
{
	if (auto n = getEmbeddedNetwork())
	{
		if (n->canBeFrozen())
			n->setUseFrozenNode((bool)newValue);
	}
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

var NodeBase::getParameterReference(var indexOrId) const
{
	Parameter* p = nullptr;

	if (indexOrId.isString())
		p = getParameter(indexOrId.toString());
	else
		p = getParameter((int)indexOrId);

	if (p != nullptr)
		return var(p);
	else
		return {};
}



struct Parameter::Wrapper
{
	API_METHOD_WRAPPER_0(NodeBase::Parameter, getId);
	API_METHOD_WRAPPER_0(NodeBase::Parameter, getValue);
	API_METHOD_WRAPPER_1(NodeBase::Parameter, addConnectionFrom);
	API_VOID_METHOD_WRAPPER_1(NodeBase::Parameter, setValueAndStoreAsync);
};

Parameter::Parameter(NodeBase* parent_, ValueTree& data_) :
	ConstScriptingObject(parent_->getScriptProcessor(), 4),
	parent(parent_),
	data(data_)
{
	auto initialValue = (double)data[PropertyIds::Value];
	auto weakThis = WeakReference<Parameter>(this);

	setTreeWithValue(data);

	ADD_API_METHOD_0(getValue);
	ADD_API_METHOD_1(addConnectionFrom);
	ADD_API_METHOD_1(setValueAndStoreAsync);

#define ADD_PROPERTY_ID_CONSTANT(id) addConstant(id.toString(), id.toString());

	ADD_PROPERTY_ID_CONSTANT(PropertyIds::MinValue);
	ADD_PROPERTY_ID_CONSTANT(PropertyIds::MaxValue);
	ADD_PROPERTY_ID_CONSTANT(PropertyIds::MidPoint);
	ADD_PROPERTY_ID_CONSTANT(PropertyIds::StepSize);

	setValueAndStoreAsync(initialValue);
}



juce::String Parameter::getId() const
{
	return data[PropertyIds::ID].toString();
}


double Parameter::getValue() const
{
	return dbNew.getDisplayValue();
}

void Parameter::setCallbackNew(parameter::dynamic_base* ownedNew)
{
	SimpleReadWriteLock::ScopedWriteLock sl(parent->getRootNetwork()->getConnectionLock());
	dbNew.setParameter(ownedNew);
}

void Parameter::setTreeWithValue(ValueTree v)
{
	treeThatStoresValue = v;
	valuePropertyUpdater.setCallback(treeThatStoresValue, { PropertyIds::Value }, valuetree::AsyncMode::Synchronously,
		std::bind(&NodeBase::Parameter::updateFromValueTree, this, std::placeholders::_1, std::placeholders::_2));
}

void Parameter::setValueAndStoreAsync(double newValue)
{
	if (!connectionSourceTree.isValid()) // prevent overriding all voice parameters
	{
		DspNetwork::NoVoiceSetter nvs(*parent->getRootNetwork());
		dbNew.call(newValue);
	}
	else
		dbNew.setUIValue(newValue);
}

juce::ValueTree Parameter::getConnectionSourceTree(bool forceUpdate)
{
	if (forceUpdate)
	{
		auto pId = getId();
		auto nId = parent->getId();
		auto n = parent->getRootNetwork();

		{
			auto containers = n->getListOfNodesWithType<NodeContainer>(true);

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
			auto modNodes = n->getListOfNodesWithType<ModulationSourceNode>(true);

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

		details->setProperty(PropertyIds::ModulationTarget, isMod);
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

		if ((bool)dragDetails.getProperty(PropertyIds::ModulationTarget, false))
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

void NodeBase::addConnectionToBypass(var dragDetails)
{
	auto sourceParameterTree = DragHelpers::getValueTreeOfSourceParameter(this, dragDetails);

	if (sourceParameterTree.isValid())
	{
		ValueTree newC(PropertyIds::Connection);
		newC.setProperty(PropertyIds::NodeId, getId(), nullptr);
		newC.setProperty(PropertyIds::ParameterId, PropertyIds::Bypassed.toString(), nullptr);

		NormalisableRange<double> r(0.5, 1.1, 0.5);

		RangeHelpers::storeDoubleRange(newC, false, r, nullptr);

		String connectionId = DragHelpers::getSourceNodeId(dragDetails) + "." + 
							  DragHelpers::getSourceParameterId(dragDetails);

		setValueTreeProperty(PropertyIds::DynamicBypass, connectionId);

		ValueTree connectionTree = sourceParameterTree.getChildWithName(PropertyIds::Connections);
		connectionTree.addChild(newC, -1, getUndoManager());
	}
}

var NodeBase::Parameter::addConnectionFrom(var dragDetails)
{
	auto shouldAdd = dragDetails.isObject();

	data.setProperty(PropertyIds::Automated, shouldAdd, parent->getUndoManager());

	if (shouldAdd)
	{
		auto sourceNodeId = DragHelpers::getSourceNodeId(dragDetails);
		auto parameterId = DragHelpers::getSourceParameterId(dragDetails);

		if (auto modSource = DragHelpers::getModulationSource(parent, dragDetails))
			return modSource->addModulationTarget(this);

		if (sourceNodeId == parent->getId() && parameterId == getId())
			return {};

		if (auto sn = parent->getRootNetwork()->getNodeWithId(sourceNodeId))
		{
			if (auto sp = dynamic_cast<NodeContainer::MacroParameter*>(sn->getParameter(parameterId)))
				return sp->addParameterTarget(this);

			if (dragDetails.getProperty(PropertyIds::SwitchTarget, false))
			{
				auto cTree = sn->getValueTree().getChildWithName(PropertyIds::SwitchTargets).getChild(parameterId.getIntValue()).getChildWithName(PropertyIds::Connections);

				if (cTree.isValid())
				{
					ValueTree newC(PropertyIds::Connection);
					newC.setProperty(PropertyIds::NodeId, parent->getId(), nullptr);
					newC.setProperty(PropertyIds::ParameterId, getId(), nullptr);
					RangeHelpers::storeDoubleRange(newC, false, RangeHelpers::getDoubleRange(data), nullptr);
					newC.setProperty(PropertyIds::Expression, "", nullptr);

					cTree.addChild(newC, -1, parent->getUndoManager());
				}
			}
		}

		return {};
	}
	else
	{
		auto c = getConnectionSourceTree(false);

		if (c.isValid())
			c.getParent().removeChild(c, parent->getUndoManager());

		connectionSourceTree = {};

		return {};
	}
}

bool NodeBase::Parameter::matchesConnection(const ValueTree& c) const
{
	auto matchesNode = c[PropertyIds::NodeId].toString() == parent->getId();
	auto matchesParameter = c[PropertyIds::ParameterId].toString() == getId();
	return matchesNode && matchesParameter;
}

juce::Array<NodeBase::Parameter*> NodeBase::Parameter::getConnectedMacroParameters() const
{
	Array<Parameter*> list;

	if (auto n = parent)
	{
		while ((n = n->getParentNode()))
		{
			for (int i = 0; i < n->getNumParameters(); i++)
			{
				if (auto m = dynamic_cast<NodeContainer::MacroParameter*>(n->getParameter(i)))
				{
					if (m->matchesTarget(this))
						list.add(m);
				}
			}
		}
	}

	return list;
}

HelpManager::HelpManager(NodeBase* parent, ValueTree d) :
	ControlledObject(parent->getScriptProcessor()->getMainController_())
{
	commentListener.setCallback(d, { PropertyIds::Comment, PropertyIds::NodeColour}, valuetree::AsyncMode::Asynchronously,
		BIND_MEMBER_FUNCTION_2(HelpManager::update));
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
		area.removeFromLeft(10.0f);
		g.setColour(Colours::black.withAlpha(0.1f));
		g.fillRoundedRectangle(area, 2.0f);
		helpRenderer->draw(g, area.reduced(10.0f));
	}
}

void HelpManager::addHelpListener(Listener* l)
{
	listeners.addIfNotAlreadyThere(l);
	l->helpChanged(lastWidth + 30.0f, lastHeight + 20.0f);
}

void HelpManager::removeHelpListener(Listener* l)
{
	listeners.removeAllInstancesOf(l);
}

juce::Rectangle<float> HelpManager::getHelpSize() const
{
	return { 0.0f, 0.0f, lastHeight > 0.0f ? lastWidth + 30.0f : 0.0f, lastHeight + 20.0f };
}

void HelpManager::rebuild()
{
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

	for (auto l : listeners)
	{
		if (l != nullptr)
			l->helpChanged(lastWidth + 30.0f, lastHeight);
	}
}

struct ConnectionBase::Wrapper
{
	API_METHOD_WRAPPER_1(ConnectionBase, get);
	API_VOID_METHOD_WRAPPER_2(ConnectionBase, set);
	API_METHOD_WRAPPER_0(ConnectionBase, getLastValue);
	API_METHOD_WRAPPER_0(ConnectionBase, isModulationConnection);
	API_METHOD_WRAPPER_0(ConnectionBase, getTarget);
};

ConnectionBase::ConnectionBase(ProcessorWithScriptingContent* p, ValueTree data_) :
	ConstScriptingObject(p, 6),
	data(data_)
{
	addConstant(PropertyIds::Enabled.toString(), PropertyIds::Enabled.toString());
	addConstant(PropertyIds::MinValue.toString(), PropertyIds::MinValue.toString());
	addConstant(PropertyIds::MaxValue.toString(), PropertyIds::MaxValue.toString());
	addConstant(PropertyIds::SkewFactor.toString(), PropertyIds::SkewFactor.toString());
	addConstant(PropertyIds::StepSize.toString(), PropertyIds::StepSize.toString());
	addConstant(PropertyIds::Expression.toString(), PropertyIds::Expression.toString());

	ADD_API_METHOD_0(getLastValue);
	ADD_API_METHOD_1(get);
	ADD_API_METHOD_2(set);
}

scriptnode::parameter::dynamic_chain* ConnectionBase::createParameterFromConnectionTree(NodeBase* n, const ValueTree& connectionTree, bool throwIfNotFound)
{
	jassert(connectionTree.getType() == PropertyIds::Connections);

	ScopedPointer<parameter::dynamic_chain> chain = new parameter::dynamic_chain();

	chain->inputRange2 = RangeHelpers::getDoubleRange(connectionTree.getParent());

	for (auto c : connectionTree)
	{
		auto nId = c[PropertyIds::NodeId].toString();
		auto pId = c[PropertyIds::ParameterId].toString();
		auto tn = n->getRootNetwork()->getNodeWithId(nId);

		if (tn == nullptr && throwIfNotFound)
			throw nId;

		if (pId == PropertyIds::Bypassed.toString())
		{
			auto r = RangeHelpers::getDoubleRange(c).getRange();
			chain->addParameter(new NodeBase::DynamicBypassParameter(tn, r));
		}
		else if (auto param = tn->getParameter(pId))
		{
#if HISE_INCLUDE_SNEX
			if (auto sn = dynamic_cast<SnexSource::SnexParameter*>(param))
			{
				ScopedPointer<parameter::dynamic_base> b;
				b = parameter::dynamic_base::createFromConnectionTree(c, sn->p);
				b->setDataTree(sn->getTreeWithValue());
				chain->addParameter(b.release());
				continue;
			}
#endif
			auto pList = tn->createInternalParameterList();

			for (auto p : pList)
			{
				if (p.info.getId() == pId)
				{
					ScopedPointer<parameter::dynamic_base> b;
					b = parameter::dynamic_base::createFromConnectionTree(c, p.callback);
					b->setDataTree(param->getTreeWithValue());
					chain->addParameter(b.release());
				}
			}
		}
	}

	return chain.release();
}

void ConnectionBase::initRemoveUpdater(NodeBase* parent)
{
	auto nodeId = data[PropertyIds::NodeId].toString();

	if (auto targetNode = dynamic_cast<NodeBase*>(parent->getRootNetwork()->get(nodeId).getObject()))
	{
		auto undoManager = parent->getUndoManager();
		auto d = data;
		auto n = parent->getRootNetwork();

		nodeRemoveUpdater.setCallback(targetNode->getValueTree(), valuetree::AsyncMode::Synchronously, false,
			[n, d, undoManager](ValueTree& v)
		{
			if (auto node = n->getNodeForValueTree(v))
			{
				if(!node->isBeingMoved())
					d.getParent().removeChild(d, undoManager);
			}
		});
	}
}

RealNodeProfiler::RealNodeProfiler(NodeBase* n) :
	enabled(n->getRootNetwork()->getCpuProfileFlag()),
	profileFlag(n->getCpuFlag())
{
	if (enabled)
		start = Time::getMillisecondCounterHiRes();
}

}


