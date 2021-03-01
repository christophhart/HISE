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

NodeComponent* ComponentHelpers::createDefaultComponent(NodeBase* n)
{
    return  new DefaultParameterNodeComponent(n);
}
 
void ComponentHelpers::addExtraComponentToDefault(NodeComponent* nc, Component* c)
{
    dynamic_cast<DefaultParameterNodeComponent*>(nc)->setExtraComponent(c);
}

#if OLD_SCRIPTNODE_CPP
    
scriptnode::HardcodedNode::CombinedParameterValue* HardcodedNode::getCombinedParameter(const String& id, NormalisableRange<double> range, Identifier opType)
{
	auto p = getParameter(id, range);

	for (auto c : combinedParameterValues)
	{
		if (c->matches(p))
		{
			c->updateRangeForOpType(opType, p.range);
			return c;
		}
	}

	auto c = new CombinedParameterValue(p);
	c->updateRangeForOpType(opType, p.range);
	combinedParameterValues.add(c);
	return c;
}

void HardcodedNode::setNodeProperty(const String& id, const var& newValue, bool isPublic)
{
	auto propTree = getNodePropertyTree(id);

	if (propTree.isValid())
	{
		propTree.setProperty(PropertyIds::Value, newValue, um);
		propTree.setProperty(PropertyIds::Public, isPublic, um);
	}
}

var HardcodedNode::getNodeProperty(const String& id, const var& defaultValue) const
{
	auto propTree = getNodePropertyTree(id);

	if (propTree.isValid())
		return propTree[PropertyIds::Value];

	return defaultValue;
}

juce::ValueTree HardcodedNode::getNodePropertyTree(const String& id) const
{
	return nodeData.getChildWithName(PropertyIds::Properties).getChildWithProperty(PropertyIds::ID, id);
}

void HardcodedNode::setParameterDefault(const String& parameterId, double value)
{
	for (auto& parameter : internalParameterList)
	{
		if (parameter.id == parameterId)
		{
			parameter.setDefaultValue(value);
			parameter.dbNew(value);
			break;
		}
	}
}

scriptnode::HiseDspBase::ParameterData HardcodedNode::getParameter(const String& id)
{
	for (auto& c : internalParameterList)
		if (c.id == id)
			return c;

	return HiseDspBase::ParameterData("undefined");
}


scriptnode::HiseDspBase::ParameterData HardcodedNode::getParameter(const String& id, NormalisableRange<double> d)
{
	return getParameter(id).withRange(d);
}

juce::String HardcodedNode::getNodeId(const HiseDspBase* internalNode) const
{
	for (const auto& n : internalNodes)
	{
		if (n.node == internalNode)
			return n.id;
	}

	return {};
}

scriptnode::HiseDspBase* HardcodedNode::getNode(const String& id) const
{
	for (const auto& n : internalNodes)
	{
		if (n.id == id)
			return n.node;
	}

	return nullptr;
}

#endif

WrapperNode::WrapperNode(DspNetwork* parent, ValueTree d) :
	NodeBase(parent, d, 0)
{

}

scriptnode::NodeComponent* WrapperNode::createComponent()
{
	auto nc = ComponentHelpers::createDefaultComponent(this);

	if (auto extra = createExtraComponent())
	{
		ComponentHelpers::addExtraComponentToDefault(nc, extra);
	}

	return nc;
}

juce::Rectangle<int> WrapperNode::getPositionInCanvas(Point<int> topLeft) const
{
	int numParameters = getNumParameters();

	if (numParameters == 0)
		return createRectangleForParameterSliders(0).withPosition(topLeft);
	if (numParameters % 5 == 0)
		return createRectangleForParameterSliders(5).withPosition(topLeft);
	else if (numParameters % 4 == 0)
		return createRectangleForParameterSliders(4).withPosition(topLeft);
	else if (numParameters % 3 == 0)
		return createRectangleForParameterSliders(3).withPosition(topLeft);
	else if (numParameters % 2 == 0)
		return createRectangleForParameterSliders(2).withPosition(topLeft);
	else if (numParameters == 1)
		return createRectangleForParameterSliders(1).withPosition(topLeft);

	return {};
}

juce::Rectangle<int> WrapperNode::createRectangleForParameterSliders(int numColumns) const
{
	int h = UIValues::HeaderHeight;
	
	auto eb = getExtraComponentBounds();

	h += eb.getHeight();

	int w = 0;

	if (numColumns == 0)
		w = UIValues::NodeWidth * 2;
	else
	{
		int numParameters = getNumParameters();
		int numRows = (int)std::ceil((float)numParameters / (float)numColumns);

		h += numRows * (48 + 18);
		w = jmin(numColumns * 100, numParameters * 100);
	}


	w = jmax(w, eb.getWidth());

	auto b = Rectangle<int>(0, 0, w, h);
	return getBoundsToDisplay(b.expanded(UIValues::NodeMargin));
}



void InterpretedNode::reset()
{
	this->obj.reset();
}

void InterpretedNode::prepare(PrepareSpecs specs)
{
	auto& exceptionHandler = getRootNetwork()->getExceptionHandler();

	exceptionHandler.removeError(this);

	try
	{
		this->obj.prepare(specs);
	}
	catch (Error& s)
	{
		exceptionHandler.addError(this, s);
	}

	NodeBase::prepare(specs);
}

void InterpretedNode::processFrame(NodeBase::FrameType& data)
{
	if (data.size() == 1)
		processMonoFrame(MonoFrameType::as(data.begin()));
	if (data.size() == 2)
		processStereoFrame(StereoFrameType::as(data.begin()));
}

void InterpretedNode::processMonoFrame(MonoFrameType& data)
{
	this->obj.processFrame(data);
}

void InterpretedNode::processStereoFrame(StereoFrameType& data)
{
	this->obj.processFrame(data);
}

void InterpretedNode::process(ProcessDataDyn& data) noexcept
{
	NodeProfiler np(this);
	this->obj.process(data);
}

void InterpretedNode::setBypassed(bool shouldBeBypassed)
{
	WrapperNode::setBypassed(shouldBeBypassed);
	WrapperType::setParameter<bypass::ParameterId>(&this->obj, (double)shouldBeBypassed);
}

void InterpretedNode::handleHiseEvent(HiseEvent& e)
{
	this->obj.handleHiseEvent(e);
}

void InterpretedModNode::timerCallback()
{
	getParameterHolder()->updateUI();
}

bool InterpretedModNode::isUsingNormalisedRange() const
{
	return this->obj.getWrappedObject().isNormalised;
}

scriptnode::parameter::dynamic_base_holder* InterpretedModNode::getParameterHolder()
{
	return &this->obj.p;
}

void InterpretedModNode::reset()
{
	this->obj.reset();
}

bool InterpretedModNode::isPolyphonic() const
{
	return this->obj.isPolyphonic();
}

void InterpretedModNode::prepare(PrepareSpecs specs)
{
	auto& exceptionHandler = getRootNetwork()->getExceptionHandler();

	exceptionHandler.removeError(this);

	try
	{
		ModulationSourceNode::prepare(specs);
		this->obj.prepare(specs);
	}
	catch (Error& s)
	{
		exceptionHandler.addError(this, s);
	}
}

void InterpretedModNode::writeToRingBuffer(double value, int numSamplesForAnalysis)
{
	if (ringBuffer != nullptr &&
		numSamplesForAnalysis > 0 &&
		getRootNetwork()->isRenderingFirstVoice())
	{
		ringBuffer->write(value, (int)(jmax(1.0, sampleRateFactor * (double)numSamplesForAnalysis)));
	}
}

void InterpretedModNode::processFrame(NodeBase::FrameType& data)
{
	if (data.size() == 1)
		processMonoFrame(MonoFrameType::as(data.begin()));
	if (data.size() == 2)
		processStereoFrame(StereoFrameType::as(data.begin()));
}

void InterpretedModNode::processMonoFrame(MonoFrameType& data)
{
	this->obj.p.setSamplesToWrite(1);
	this->obj.processFrame(data);
}

void InterpretedModNode::processStereoFrame(StereoFrameType& data)
{
	this->obj.p.setSamplesToWrite(1);
	this->obj.processFrame(data);
}

void InterpretedModNode::process(ProcessDataDyn& data) noexcept
{
	NodeProfiler np(this);
	this->obj.p.setSamplesToWrite(data.getNumSamples());
	this->obj.process(data);
}

void InterpretedModNode::handleHiseEvent(HiseEvent& e)
{
	this->obj.handleHiseEvent(e);
}

void NewHero::prepare(PrepareSpecs ps)
{
	auto& exceptionHandler = getRootNetwork()->getExceptionHandler();

	exceptionHandler.removeError(this);

	try
	{
		ModulationSourceNode::prepare(ps);
		this->obj.prepare(ps);
	}
	catch (Error& s)
	{
		exceptionHandler.addError(this, s);
	}
}

}

