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

NodeBase::NodeBase(DspNetwork* rootNetwork, ValueTree data_, int numConstants_) :
	ConstScriptingObject(rootNetwork->getScriptProcessor(), numConstants_),
	parent(rootNetwork),
	v_data(data_),
	bypassUpdater(rootNetwork->getScriptProcessor()->getMainController_()),
	bypassed(v_data, PropertyIds::Bypassed, getUndoManager(), false)
{
	setDefaultValue(PropertyIds::NumChannels, 2);
	setDefaultValue(PropertyIds::LockNumChannels, false);

	bypassUpdater.setFunction([this]()
	{
		bypassed = pendingBypassState;
	});
}

DspNetwork* NodeBase::getRootNetwork() const
{
	return dynamic_cast<DspNetwork*>(parent.get());
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

	if (v_data[PropertyIds::DynamicBypass].toString().isNotEmpty())
	{
		className = CppGen::Emitter::wrapIntoTemplate(className, "wr::bypass::smoothed");
	};
	
	return className;
}

void NodeBase::setProperty(const Identifier& id, const var value)
{
	v_data.setProperty(id, value, getUndoManager());
}

void NodeBase::setDefaultValue(const Identifier& id, var newValue)
{
	if (!v_data.hasProperty(id))
		v_data.setProperty(id, newValue, nullptr);
}




NodeComponent* NodeBase::createComponent()
{
	return new NodeComponent(this);
}


juce::Rectangle<int> NodeBase::getPositionInCanvas(Point<int> topLeft) const
{
	using namespace UIValues;

	return Rectangle<int>(NodeWidth, NodeHeight)
		.withPosition(topLeft)
		.reduced(NodeMargin);
}

void NodeBase::setBypassed(bool shouldBeBypassed)
{
	if (bypassed != shouldBeBypassed)
	{
		pendingBypassState = shouldBeBypassed;
		bypassUpdater.triggerUpdateWithLambda();
	}
}


bool NodeBase::isBypassed() const noexcept
{
	return bypassed;
}

NodeBase* NodeBase::getParentNode() const
{
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

juce::UndoManager* NodeBase::getUndoManager()
{
	return dynamic_cast<Processor*>(getScriptProcessor())->getMainController()->getControlUndoManager();
}

juce::Rectangle<int> NodeBase::reduceHeightIfFolded(Rectangle<int> originalHeight) const
{
	if (v_data[PropertyIds::Folded])
		return originalHeight.withHeight(UIValues::HeaderHeight);
	else
		return originalHeight;
}



bool NodeBase::hasFixChannelAmount() const
{
	return v_data[PropertyIds::LockNumChannels];
}

int NodeBase::getNumParameters() const
{
	return parameters.size();
}


scriptnode::NodeBase::Parameter* NodeBase::getParameter(const String& id) const
{
	for (auto p : parameters)
		if (p->getId() == id)
			return p;

	return nullptr;
}


scriptnode::NodeBase::Parameter* NodeBase::getParameter(int index) const
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

NodeBase::Parameter::Parameter(NodeBase* parent_, ValueTree& data_) :
	ConstScriptingObject(parent_->getScriptProcessor(), 0),
	parent(parent_),
	valueUpdater(parent_->getScriptProcessor()->getMainController_()),
	data(data_)
{
	currentDoublePrecisionValue = data[PropertyIds::Value];
	auto weakThis = WeakReference<Parameter>(this);

	valueUpdater.setFunction(BIND_MEMBER_FUNCTION_0(NodeBase::Parameter::storeValue));
	
	opTypeListener.setCallback(data, { PropertyIds::OpType },
		valuetree::AsyncMode::Synchronously,
		[this](Identifier, var newValue)
	{
		modMulValue = 1.0;
		modAddValue = 0.0;
	});
	
}



juce::String NodeBase::Parameter::getId() const
{
	return data[PropertyIds::ID].toString();
}


double NodeBase::Parameter::getValue() const
{
	return currentDoublePrecisionValue;
}


scriptnode::DspHelpers::ParameterCallback& NodeBase::Parameter::getReferenceToCallback()
{
	return db;
}


scriptnode::DspHelpers::ParameterCallback NodeBase::Parameter::getCallback() const
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
	return (currentDoublePrecisionValue + modAddValue) * modMulValue;
}

void NodeBase::Parameter::storeValue()
{
	data.setProperty(PropertyIds::Value, getValue(), parent->getUndoManager());
}

void NodeBase::Parameter::setValueAndStoreAsync(double newValue)
{
	if (currentDoublePrecisionValue == newValue)
		return;

	currentDoublePrecisionValue = newValue;

	if (db)
		db(getModValue());
	else
		jassertfalse;

	valueUpdater.triggerUpdateWithLambda();
}



void NodeBase::Parameter::addModulationValue(double newValue)
{
	if (modAddValue != newValue)
	{
		modAddValue = newValue;

		if (db)
			db(currentDoublePrecisionValue + modAddValue);
		else
			jassertfalse;
	}
}


void NodeBase::Parameter::multiplyModulationValue(double newValue)
{
	if (modMulValue != newValue)
	{
		modMulValue = newValue;

		if (db)
			db(currentDoublePrecisionValue * modMulValue);
		else
			jassertfalse;
	}
}

struct DragHelpers
{
	static String getSourceNodeId(var dragDetails)
	{
		return dragDetails.getProperty(PropertyIds::ID, "").toString();
	}

	static String getSourceParameterId(var dragDetails)
	{
		return dragDetails.getProperty(PropertyIds::ParameterId, "").toString();
	}

	static ModulationSourceNode* getModulationSource(NodeBase* parent, var dragDetails)
	{
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

		setProperty(PropertyIds::DynamicBypass, connectionId);

		ValueTree connectionTree = sourceParameterTree.getChildWithName(PropertyIds::Connections);
		connectionTree.addChild(newC, -1, getUndoManager());
	}
}

void NodeBase::Parameter::addConnectionTo(var dragDetails)
{
	auto sourceNodeId = DragHelpers::getSourceNodeId(dragDetails);
	auto parameterId = DragHelpers::getSourceParameterId(dragDetails);

	if (auto modSource = DragHelpers::getModulationSource(parent, dragDetails))
	{
		modSource->addModulationTarget(this);
		return;
	}

	if (sourceNodeId == parent->getId() && parameterId == getId())
		return;

	auto sourcePTree = DragHelpers::getValueTreeOfSourceParameter(parent, dragDetails);

	if (sourcePTree.isValid())
	{
		ValueTree newC(PropertyIds::Connection);
		newC.setProperty(PropertyIds::NodeId, parent->getId(), nullptr);
		newC.setProperty(PropertyIds::ParameterId, getId(), nullptr);
		newC.setProperty(PropertyIds::Converter, ConverterIds::Identity.toString(), nullptr);
		newC.setProperty(PropertyIds::OpType, OperatorIds::SetValue.toString(), nullptr);

		auto r = RangeHelpers::getDoubleRange(data);

		RangeHelpers::storeDoubleRange(newC, false, r, nullptr);

		String connectionId = sourceNodeId + "." + parameterId;

		data.setProperty(PropertyIds::Connection, connectionId, parent->getUndoManager());
		data.setProperty(PropertyIds::OpType, OperatorIds::SetValue.toString(), parent->getUndoManager());

		ValueTree connectionTree = sourcePTree.getChildWithName(PropertyIds::Connections);
		connectionTree.addChild(newC, -1, parent->getUndoManager());
	}
}

}


