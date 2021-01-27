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
	API_METHOD_WRAPPER_0(NodeBase, createRingBuffer);
	API_VOID_METHOD_WRAPPER_1(NodeBase, writeModulationSignal);
};



NodeBase::NodeBase(DspNetwork* rootNetwork, ValueTree data_, int numConstants_) :
	ConstScriptingObject(rootNetwork->getScriptProcessor(), 8),
	parent(rootNetwork),
	v_data(data_),
	bypassUpdater(rootNetwork->getScriptProcessor()->getMainController_()->getGlobalUIUpdater()),
	bypassed(v_data, PropertyIds::Bypassed, getUndoManager(), false),
	helpManager(*this, data_),
	currentId(v_data[PropertyIds::ID].toString())
{
	setDefaultValue(PropertyIds::NumChannels, 2);
	setDefaultValue(PropertyIds::LockNumChannels, false);
	setDefaultValue(PropertyIds::NodeColour, 0);
	setDefaultValue(PropertyIds::Comment, "");
	setDefaultValue(PropertyIds::CommentWidth, 300);

	ADD_API_METHOD_0(reset);
	ADD_API_METHOD_2(set);
	ADD_API_METHOD_1(get);
	ADD_API_METHOD_1(setBypassed);
	ADD_API_METHOD_0(isBypassed);
	ADD_API_METHOD_2(setParent);
	ADD_API_METHOD_0(createRingBuffer);
	ADD_API_METHOD_1(writeModulationSignal);
	ADD_API_METHOD_1(getParameterReference);

	bypassUpdater.setFunction([this]()
	{
		bypassed = pendingBypassState;
	});

	for (auto c : getPropertyTree())
	{
		addConstant(c[PropertyIds::ID].toString(), c[PropertyIds::ID]);
	}
}

DspNetwork* NodeBase::getRootNetwork() const
{
	return static_cast<DspNetwork*>(parent.get());
}


void NodeBase::prepareParameters(PrepareSpecs specs)
{
	for (int i = 0; i < getNumParameters(); i++)
		getParameter(i)->prepare(specs);
}

void NodeBase::processSingle(float* frameData, int numChannels)
{
	ProcessData d;
	d.data = &frameData;
	d.numChannels = numChannels;
	d.size = 1;

	process(d);
}


juce::String NodeBase::createCppClass(bool isOuterClass)
{
	ignoreUnused(isOuterClass);

	auto className = v_data[PropertyIds::FactoryPath].toString().replace(".", "::");

	if (isPolyphonic())
		className << "_poly";

	return className;
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


juce::Rectangle<int> NodeBase::getPositionInCanvas(juce::Point<int> topLeft) const
{
	using namespace UIValues;

	Rectangle<int> body(NodeWidth, NodeHeight);

	body = body.withPosition(topLeft).reduced(NodeMargin);
	return body;
}

void NodeBase::setBypassed(bool shouldBeBypassed)
{
	checkValid();

	if (bypassed != shouldBeBypassed)
	{
		pendingBypassState = shouldBeBypassed;
		bypassUpdater.triggerUpdateWithLambda();
	}
}


bool NodeBase::isBypassed() const noexcept
{
	checkValid();

	return bypassed;
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
		return originalHeight.withHeight(UIValues::HeaderHeight).withWidth(UIValues::NodeWidth);
	else
	{ 
		auto helpBounds = helpManager.getHelpSize().toNearestInt();

		originalHeight.setWidth(originalHeight.getWidth() + helpBounds.getWidth());
		originalHeight.setHeight(jmax<int>(originalHeight.getHeight(), helpBounds.getHeight()));

		return originalHeight;
	}
}



juce::Rectangle<int> NodeBase::getBoundsWithoutHelp(Rectangle<int> originalHeight) const
{
	if (v_data[PropertyIds::Folded])
		return originalHeight.withHeight(UIValues::HeaderHeight);
	else
	{
		return originalHeight;
	}
}

bool NodeBase::hasFixChannelAmount() const
{
	return v_data[PropertyIds::LockNumChannels];
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

void NodeBase::addParameter(Parameter* p)
{
	parameters.add(p);
	auto& f = p->getReferenceToCallback();
	
	if(f) f(p->getValue());
}


void NodeBase::removeParameter(int index)
{
	parameters.remove(index);
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

void NodeBase::writeModulationSignal(var buffer)
{
	checkValid();

	if (auto b = buffer.getBuffer())
	{
		if (b->size == ModulationSourceNode::RingBufferSize)
		{
			if (auto modSource = dynamic_cast<ModulationSourceNode*>(this))
			{
				modSource->fillAnalysisBuffer(b->buffer);
			}
			else
				reportScriptError("No modulation node");
		}
		else
			reportScriptError("buffer size mismatch. Expected: " + String(ModulationSourceNode::RingBufferSize));
	}
	else
		reportScriptError("the argument is not a buffer.");
}

var NodeBase::createRingBuffer()
{
	VariantBuffer::Ptr p = new VariantBuffer(ModulationSourceNode::RingBufferSize);
	return var(p);
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



struct NodeBase::Parameter::Wrapper
{
	API_METHOD_WRAPPER_0(NodeBase::Parameter, getId);
	API_METHOD_WRAPPER_0(NodeBase::Parameter, getValue);
	API_METHOD_WRAPPER_0(NodeBase::Parameter, getModValue);
	API_METHOD_WRAPPER_1(NodeBase::Parameter, addConnectionFrom);
	API_VOID_METHOD_WRAPPER_1(NodeBase::Parameter, setValueAndStoreAsync);
};

NodeBase::Parameter::Parameter(NodeBase* parent_, ValueTree& data_) :
	ConstScriptingObject(parent_->getScriptProcessor(), 4),
	parent(parent_),
	valueUpdater(parent_->getScriptProcessor()->getMainController_()->getGlobalUIUpdater()),
	data(data_)
{
	auto initialValue = (double)data[PropertyIds::Value];

	value.forEachVoice([initialValue](ParameterValue& v)
	{
		v.value = initialValue;
	});

	auto weakThis = WeakReference<Parameter>(this);

	valueUpdater.setFunction(BIND_MEMBER_FUNCTION_0(NodeBase::Parameter::storeValue));
	
	valuePropertyUpdater.setCallback(data, { PropertyIds::Value }, valuetree::AsyncMode::Synchronously,
		std::bind(&NodeBase::Parameter::updateFromValueTree, this, std::placeholders::_1, std::placeholders::_2));

	modulationStorageBypasser.setCallback(data, {PropertyIds::ModulationTarget},
		valuetree::AsyncMode::Synchronously,
		[this](Identifier, var newValue)
	{
		if ((bool)newValue)
			this->valueUpdater.setFunction({});
		else
			valueUpdater.setFunction(BIND_MEMBER_FUNCTION_0(NodeBase::Parameter::storeValue));
	});

	ADD_API_METHOD_0(getValue);
	ADD_API_METHOD_0(getModValue);
	ADD_API_METHOD_0(getModValue);
	ADD_API_METHOD_1(addConnectionFrom);
	ADD_API_METHOD_1(setValueAndStoreAsync);

#define ADD_PROPERTY_ID_CONSTANT(id) addConstant(id.toString(), id.toString());

	ADD_PROPERTY_ID_CONSTANT(PropertyIds::MinValue);
	ADD_PROPERTY_ID_CONSTANT(PropertyIds::MaxValue);
	ADD_PROPERTY_ID_CONSTANT(PropertyIds::MidPoint);
	ADD_PROPERTY_ID_CONSTANT(PropertyIds::StepSize);
}



juce::String NodeBase::Parameter::getId() const
{
	return data[PropertyIds::ID].toString();
}


double NodeBase::Parameter::getValue() const
{
	return value.getCurrentOrFirst().value;
}


DspHelpers::ParameterCallback& NodeBase::Parameter::getReferenceToCallback()
{
	return db;
}


DspHelpers::ParameterCallback NodeBase::Parameter::getCallback() const
{
	return db;
}

void NodeBase::Parameter::setCallback(const DspHelpers::ParameterCallback& newCallback)
{
	ScopedLock sl(parent->getRootNetwork()->getConnectionLock());

	if (!newCallback)
		db = nothing;
	else
		db = newCallback;
}


double NodeBase::Parameter::getModValue() const
{
	return value.getCurrentOrFirst().getModValue();
}

void NodeBase::Parameter::storeValue()
{
	data.setProperty(PropertyIds::Value, getValue(), parent->getUndoManager());
}

void NodeBase::Parameter::setValueAndStoreAsync(double newValue)
{
	if (value.isMonophonicOrInsideVoiceRendering())
	{
		value.getCurrentOrFirst().value = newValue;

		lastValue = getModValue();

		if (db)
			db(lastValue);
		else
			jassertfalse;
	}
	else
	{
		value.forEachVoice([this, newValue](ParameterValue& v)
		{
			v.value = newValue;
			if (v.updateLastValue() && db)
				db(v.lastValue);
		});
	}

#if USE_BACKEND
	valueUpdater.triggerUpdateWithLambda();
#endif
}


void NodeBase::Parameter::addModulationValue(double newValue)
{
	if (value.isMonophonicOrInsideVoiceRendering())
	{
		auto& v = value.get();
		v.modAddValue = newValue;
		
		if (v.updateLastValue() && db)
			db(v.lastValue);
	}
	else
	{
		value.forEachVoice([this, newValue](ParameterValue& v)
		{
			v.modAddValue = newValue;

			if (v.updateLastValue())
				db(v.lastValue);
		});
	}
}


void NodeBase::Parameter::multiplyModulationValue(double newValue)
{
	if (value.isMonophonicOrInsideVoiceRendering())
	{
		auto& v = value.get();
		v.modMulValue = newValue;

		if (v.updateLastValue() && db)
			db(v.lastValue);
	}
	else
	{
		value.forEachVoice([this, newValue](ParameterValue& v)
		{
			v.modMulValue = newValue;

			if (v.updateLastValue())
				db(v.lastValue);
		});
	}
}

void NodeBase::Parameter::clearModulationValues()
{
	value.forEachVoice([](ParameterValue& v)
	{
		v.modAddValue = 0.0;
		v.modMulValue = 1.0;
	});
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
		newC.setProperty(PropertyIds::Converter, ConverterIds::Identity.toString(), nullptr);
		newC.setProperty(PropertyIds::OpType, OperatorIds::SetValue.toString(), nullptr);

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
	DBG(JSON::toString(dragDetails));

	auto sourceNodeId = DragHelpers::getSourceNodeId(dragDetails);
	auto parameterId = DragHelpers::getSourceParameterId(dragDetails);

	if (auto modSource = DragHelpers::getModulationSource(parent, dragDetails))
	{
		return modSource->addModulationTarget(this);
	}

	if (sourceNodeId == parent->getId() && parameterId == getId())
		return {};

	if (auto sn = parent->getRootNetwork()->getNodeWithId(sourceNodeId))
	{
		if (auto sp = dynamic_cast<NodeContainer::MacroParameter*>(sn->getParameter(parameterId)))
		{
			return sp->addParameterTarget(this);

			
		}
	}
	

	return {};
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

NodeBase::HelpManager::HelpManager(NodeBase& parent, ValueTree d) :
	ControlledObject(parent.getScriptProcessor()->getMainController_())
{
	commentListener.setCallback(d, { PropertyIds::Comment, PropertyIds::NodeColour, PropertyIds::CommentWidth }, valuetree::AsyncMode::Asynchronously,
		BIND_MEMBER_FUNCTION_2(NodeBase::HelpManager::update));
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

	opTypeListener.setCallback(data, { PropertyIds::OpType }, valuetree::AsyncMode::Asynchronously,
		[this](Identifier, var newValue)
	{
		auto id = Identifier(newValue.toString());

		setOpTypeFromId(id);
	});

	ADD_API_METHOD_0(getLastValue);
	ADD_API_METHOD_1(get);
	ADD_API_METHOD_2(set);
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

}


