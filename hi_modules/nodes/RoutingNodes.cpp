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
*   which must be separately licensed for closed source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

namespace scriptnode {
using namespace juce;
using namespace hise;

namespace routing
{


struct SendBaseComponent : public HiseDspBase::ExtraComponent<SendBase>
{
	SendBaseComponent(SendBase* t, PooledUIUpdater* updater) :
		ExtraComponent<SendBase>(t, updater)
	{
		timerCallback();
		setSize(128, 5);
	};

	void timerCallback() override
	{
		if (lastOK != getObject()->isConnected() || lastColour != getObject()->getColour())
		{
			lastOK = getObject()->isConnected();
			lastColour = getObject()->getColour();
			repaint();
		}
	}

	void paint(Graphics& g)
	{
		if (lastOK)
		{
			g.setColour(lastColour);
			g.fillRoundedRectangle(getLocalBounds().toFloat(), 2.5f);
		}
	}

	bool lastOK = false;
	Colour lastColour = Colours::transparentBlack;

};


ReceiveNode::ReceiveNode() :
	addToSignal(PropertyIds::AddToSignal, true)
{
	c = Colour((uint32)Random::getSystemRandom().nextInt64()).withAlpha(1.0f);
}

void ReceiveNode::initialise(NodeBase* n)
{
	addToSignal.init(n, this);
}

void ReceiveNode::prepare(PrepareSpecs ps)
{
	DspHelpers::increaseBuffer(buffer, ps);

	reset();
}

void ReceiveNode::createParameters(Array<ParameterData>& d)
{
	{
		ParameterData p("Feedback");
		p.range = { 0.0, 1.0, 0.01 };
		p.defaultValue = 0.0f;
		p.db = BIND_MEMBER_FUNCTION_1(ReceiveNode::setGain);
		d.add(std::move(p));
	}

	addToSignal.init(nullptr, nullptr);
}

void ReceiveNode::reset()
{
	memset(singleFrameData, 0, sizeof(float) * NUM_MAX_CHANNELS);
	buffer.clear();
}

void ReceiveNode::process(ProcessData& data)
{
	if (connectedOK)
	{
		for (int i = 0; i < data.numChannels; i++)
		{
			if (addToSignal.getValue())
				FloatVectorOperations::addWithMultiply(data.data[i], buffer.getReadPointer(i), gainFactor, data.size);
			else
				FloatVectorOperations::copyWithMultiply(data.data[i], buffer.getReadPointer(i), gainFactor, data.size);
		}

	}
}

juce::Component* ReceiveNode::createExtraComponent(PooledUIUpdater* updater)
{
	return new SendBaseComponent(this, updater);
}

void ReceiveNode::processSingle(float* frameData, int numChannels)
{
	if (addToSignal.getValue())
	{
		for (int i = 0; i < numChannels; i++)
			frameData[i] += singleFrameData[i] * gainFactor;
	}
	else
	{
		for (int i = 0; i < numChannels; i++)
			frameData[i] = singleFrameData[i] * gainFactor;
	}
}

juce::Colour ReceiveNode::getColour() const
{
	return c;
}

bool ReceiveNode::isConnected() const
{
	return connectedOK;
}

void ReceiveNode::setGain(double newGain)
{
	gainFactor = jlimit(0.0f, 1.0f, (float)newGain);
}

juce::StringArray Factory::getSourceNodeList(NodeBase* n)
{
	StringArray sa;
	
	auto list = n->getRootNetwork()->getListOfNodesWithType<HiseDspNodeBase<ReceiveNode>>(false);

	for (auto n : list)
		sa.add(n->getId());

	return sa;
}

Factory::Factory(DspNetwork* n) :
	NodeFactory(n)
{
	registerNode<send>();
	registerNode<receive>();
	registerNode<ms_encode>();
	registerNode<ms_decode>();
	registerNode<matrix>();
}

void SendNode::initialise(NodeBase* n)
{
	parent = n;
	connectionUpdater.init(n, this);
}

int SendNode::getExtraWidth() const
{
	return 128;
}

void SendNode::reset()
{

}

SendNode::SendNode() :
	connectionUpdater(*this)
{

}

SendNode::~SendNode()
{
	if (connectedSource != nullptr)
		connectedSource->connectedOK = false;
}

void SendNode::prepare(PrepareSpecs)
{

}

juce::Colour SendNode::getColour() const
{
	if (connectedSource.get() != nullptr)
		return connectedSource->getColour();

	return Colours::transparentBlack;
}

bool SendNode::isConnected() const
{
	return connectedSource != nullptr;
}

juce::Component* SendNode::createExtraComponent(PooledUIUpdater* updater)
{
	return new SendBaseComponent(this, updater);
}

void SendNode::process(ProcessData& data)
{
	if (connectedSource != nullptr)
	{
		int numChannelsToSend = jmin(connectedSource->buffer.getNumChannels(), data.numChannels);
		int numSamplesToSend = jmin(connectedSource->buffer.getNumSamples(), data.size);

		for (int i = 0; i < numChannelsToSend; i++)
			connectedSource->buffer.copyFrom(i, 0, data.data[i], numSamplesToSend);
	}
}

void SendNode::processSingle(float* frameData, int numChannels)
{
	if (connectedSource != nullptr)
	{
		for (int i = 0; i < numChannels; i++)
			connectedSource->singleFrameData[i] = frameData[i];
	}
}

void SendNode::createParameters(Array<ParameterData>&)
{
	connectionUpdater.init(nullptr, nullptr);
}

void SendNode::connectTo(HiseDspBase* s)
{
	if (connectedSource != nullptr)
		connectedSource->connectedOK = false;

	connectedSource = dynamic_cast<ReceiveNode*>(s);

	if (connectedSource != nullptr)
		connectedSource->connectedOK = true;
}

SendNode::ConnectionNodeProperty::ConnectionNodeProperty(SendNode& parent) :
	NodeProperty(PropertyIds::Connection, "", false),
	p(parent)
{

}

void SendNode::ConnectionNodeProperty::postInit(NodeBase* n)
{
	updater.setCallback(getPropertyTree(), { PropertyIds::Value }, valuetree::AsyncMode::Synchronously,
		BIND_MEMBER_FUNCTION_2(ConnectionNodeProperty::update));
}

void SendNode::ConnectionNodeProperty::update(Identifier, var newValue)
{
	if (auto hc = p.parent->getAsHardcodedNode())
	{
		p.connectTo(hc->getNode(newValue.toString()));
	}
	else
	{
		if (auto n = dynamic_cast<HiseDspNodeBase<ReceiveNode>*>(p.parent->getRootNetwork()->get(newValue).getObject()))
			p.connectTo(n->getInternalT());
		else
			p.connectTo(nullptr);
	}
}

void MsEncoder::process(ProcessData& data)
{
	if (data.numChannels == 2)
	{
		auto l = data.data[0];
		auto r = data.data[1];

		for (int i = 0; i < data.size; i++)
		{
			auto m = (*l + *r) * 0.5f;
			auto s = (*l - *r) * 0.5f;

			*l++ = m;
			*r++ = s;
		}
	}
}

void MsEncoder::processSingle(float* frameData, int numChannels)
{
	if (numChannels == 2)
	{
		auto l = frameData[0];
		auto r = frameData[1];

		auto m = (l + r) * 0.5f;
		auto s = (l - r) * 0.5f;

		frameData[0] = m;
		frameData[1] = s;
	}
}

void MsDecoder::process(ProcessData& data)
{
	if (data.numChannels == 2)
	{
		auto m = data.data[0];
		auto s = data.data[1];

		for (int i = 0; i < data.size; i++)
		{
			auto l = *m + *s; 
			auto r = *m - *s;

			*m++ = l;
			*s++ = r;
		}
	}
}

void MsDecoder::processSingle(float* frameData, int numChannels)
{
	if (numChannels == 2)
	{
		auto m = frameData[0];
		auto s = frameData[1];

		auto l = (m + s);
		auto r = (m - s);

		frameData[0] = l;
		frameData[1] = r;
	}
}



}

}