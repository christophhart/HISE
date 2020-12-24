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

void CombinedParameterDisplay::paint(Graphics& g)
{
	g.setFont(GLOBAL_BOLD_FONT());

	auto r = obj->currentRange;

	String start, mid, end;

	int numDigits = jmax<int>(1, -1 * roundToInt(log10(r.interval)));

	start = String(r.start, numDigits);

	mid = String(r.convertFrom0to1(0.5), numDigits);

	end = String(r.end, numDigits);

	auto b = getLocalBounds().toFloat();

	float w = JUCE_LIVE_CONSTANT_OFF(55.0f);

	auto midCircle = b.withSizeKeepingCentre(w, w).translated(0.0f, 5.0f);

	float r1 = JUCE_LIVE_CONSTANT_OFF(3.0f);
	float r2 = JUCE_LIVE_CONSTANT_OFF(5.0f);
	
	float startArc = JUCE_LIVE_CONSTANT_OFF(-2.5f);
	float endArc = JUCE_LIVE_CONSTANT_OFF(2.5f);

	Colour trackColour = JUCE_LIVE_CONSTANT_OFF(Colour(0xFF888888));
	
	auto createArc = [startArc, endArc](Rectangle<float> b, float startNormalised, float endNormalised)
	{
		Path p;

		auto s = startArc + jmin(startNormalised, endNormalised) * (endArc - startArc);
		auto e = startArc + jmax(startNormalised, endNormalised) * (endArc - startArc);

		s = jlimit(startArc, endArc, s);
		e = jlimit(startArc, endArc, e);

		p.addArc(b.getX(), b.getY(), b.getWidth(), b.getHeight(), s, e, true);

		return p;
	};

	auto oc = midCircle;
	auto mc = midCircle.reduced(5.0f);
	auto ic = midCircle.reduced(10.0f);

	auto outerTrack = createArc(oc, 0.0f, 1.0f);
	auto midTrack =   createArc(mc, 0.0f, 1.0f);
	auto innerTrack = createArc(ic, 0.0f, 1.0f);

	g.setColour(trackColour);

	g.strokePath(outerTrack, PathStrokeType(r1));
	g.strokePath(midTrack, PathStrokeType(r2));
	g.strokePath(innerTrack, PathStrokeType(r1));
	
	auto data = obj->data.getFirst();

	auto nrm = [r](double v)
	{
		return r.convertTo0to1(v);
	};

	auto mulValue = nrm(data.value * data.mulValue);
	auto totalValue = nrm(data.getPmaValue());

	auto outerRing = createArc(oc, mulValue, totalValue);
	auto midRing =   createArc(mc, 0.0f, totalValue);
	auto innerRing = createArc(ic, 0.0f, mulValue);
	auto valueRing = createArc(ic, 0.0f, nrm(data.value));

	g.setColour(Colour(0xFF0051FF));
	g.strokePath(outerRing, PathStrokeType(r1));
	g.setColour(Colours::white);
	g.strokePath(midRing, PathStrokeType(r2));
	g.setColour(JUCE_LIVE_CONSTANT_OFF(Colour(0x88ff7277)));
	g.strokePath(valueRing, PathStrokeType(r1));
	g.setColour(Colour(0xFFE5353C));
	g.strokePath(innerRing, PathStrokeType(r1));

	b.removeFromTop(18.0f);

	g.setColour(Colours::white.withAlpha(0.7f));

	Rectangle<float> t((float)getWidth() / 2.0f - 35.0f, 0.0f, 70.0f, 15.0f);

	g.drawText(start, t.translated(-50.0f, 60.0f), Justification::centred);
	g.drawText(mid, t, Justification::centred);
	g.drawText(end, t.translated(50.0f, 60.0f), Justification::centred);
}

void NodeFactory::registerSnexTypes(const snex::Types::SnexTypeConstructData& cd)
{
	if (cd.polyphonic)
	{
		for (const auto& n : polyNodes)
		{
			if (n.tc)
				cd.c.registerExternalComplexType(n.tc(cd));
		}
	}

	for (const auto& n : monoNodes)
	{
		if (n.tc)
			cd.c.registerExternalComplexType(n.tc(cd));
	}
}

}

