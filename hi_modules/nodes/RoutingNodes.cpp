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
		if (getObject() == nullptr)
			return;

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
		bool shouldAdd = addToSignal.getValue();
		int numSamples = data.getNumSamples();

		int index = 0;

		for (auto ch : data)
		{
			auto dst = ch.getRawWritePointer();
			auto src = buffer.getReadPointer(index++);

			if (shouldAdd)
				FloatVectorOperations::addWithMultiply(dst, src, gainFactor, numSamples);
			else
				FloatVectorOperations::copyWithMultiply(dst, src, gainFactor, numSamples);
		}
	}
}

juce::Component* ReceiveNode::createExtraComponent(PooledUIUpdater* updater)
{
	return new SendBaseComponent(this, updater);
}

void ReceiveNode::processFrame(float* frameData, int numChannels)
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

	for (auto rn : list)
		sa.add(rn->getId());

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

		int numChannelsToSend = jmin(connectedSource->buffer.getNumChannels(), data.getNumChannels());
		int numSamplesToSend = jmin(connectedSource->buffer.getNumSamples(), data.getNumSamples());

		for (int i = 0; i < numChannelsToSend; i++)
			connectedSource->buffer.copyFrom(i, 0, data[i].begin(), numSamplesToSend);
	}
}

void SendNode::processFrame(float* frameData, int numChannels)
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

void SendNode::ConnectionNodeProperty::postInit(NodeBase* )
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



void MsDecoder::processFrame(float* frameData, int numChannels)
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



Matrix::Matrix() :
	internalData(PropertyIds::EmbeddedData, "")
{

}

void Matrix::initialise(NodeBase* n)
{
	um = n->getUndoManager();

	getMatrix().init(dynamic_cast<Processor*>(n->getScriptProcessor()));


	//ScopedValueSetter<bool> svs(recursion, true);

	internalData.setAdditionalCallback(BIND_MEMBER_FUNCTION_2(Matrix::updateFromEmbeddedData));
	internalData.init(n, this);
}

#if USE_BACKEND
struct Matrix::Editor : public ExtraComponent<hise::RoutableProcessor>
{
	Editor(RoutableProcessor* r, PooledUIUpdater* updater) :
		ExtraComponent<hise::RoutableProcessor>(r, updater),
		editor(&r->getMatrix())
	{
		addAndMakeVisible(editor);
		setSize(600, 200);
		stop();
	}

	void timerCallback() override
	{

	}

	void resized() override
	{
		editor.setBounds(getLocalBounds());
	}

	hise::RouterComponent editor;
};
#endif

juce::Component* Matrix::createExtraComponent(PooledUIUpdater* updater)
{
#if USE_BACKEND
	return new Editor(this, updater);
#else
	ignoreUnused(updater);
	return nullptr;
#endif
}

void Matrix::prepare(PrepareSpecs specs)
{
	getMatrix().setNumSourceChannels(specs.numChannels);
	getMatrix().setNumDestinationChannels(specs.numChannels);
}

void Matrix::updateFromEmbeddedData(Identifier id, var newValue)
{
	if (recursion)
		return;

	auto base64Data = newValue.toString();

	if (base64Data.isNotEmpty())
	{
		auto matrixData = ValueTreeConverters::convertBase64ToValueTree(newValue.toString(), true);
		getMatrix().restoreFromValueTree(matrixData);
	}
}

void Matrix::updateData()
{
	if (recursion)
		return;

	ScopedValueSetter<bool> svs(recursion, true);

	auto matrixData = ValueTreeConverters::convertValueTreeToBase64(getMatrix().exportAsValueTree(), true);

	internalData.storeValue(matrixData, um);

	memset(channelRouting, -1, NUM_MAX_CHANNELS);
	memset(sendRouting, -1, NUM_MAX_CHANNELS);

	for (int i = 0; i < getMatrix().getNumSourceChannels(); i++)
	{
		channelRouting[i] = (int8)getMatrix().getConnectionForSourceChannel(i);
		sendRouting[i] = (int8)getMatrix().getSendForSourceChannel(i);
	}
}

void Matrix::process(ProcessData& d)
{
	switch (d.getNumChannels())
	{
	case 1: processFrame_<1>(d); break;
	case 2: processFrame_<2>(d); break;
	case 3: processFrame_<3>(d); break;
	case 4: processFrame_<4>(d); break;
	case 5: processFrame_<5>(d); break;
	case 6: processFrame_<6>(d); break;
	case 7: processFrame_<7>(d); break;
	case 8: processFrame_<8>(d); break;
	case 9: processFrame_<9>(d); break;
	case 10: processFrame_<10>(d); break;
	case 11: processFrame_<11>(d); break;
	case 12: processFrame_<12>(d); break;
	case 13: processFrame_<13>(d); break;
	case 14: processFrame_<14>(d); break;
	case 15: processFrame_<15>(d); break;
	case 16: processFrame_<16>(d); break;
	}
}

void Matrix::processFrame(float* frameData, int numChannels)
{
	float copyData[NUM_MAX_CHANNELS + 1];
	copyData[0] = 0.0f;
	float* chData = copyData + 1;

	for (int i = 0; i < numChannels; i++)
	{
		chData[i] = frameData[i];
		frameData[i] = 0.0f;
	}

	for (int i = 0; i < numChannels; i++)
	{
		auto index = channelRouting[i];

		if (index != -1)
			frameData[index] += chData[i];

		auto sendIndex = sendRouting[i];

		if (sendIndex != -1)
			frameData[sendIndex] += chData[i];
	}
}

void Matrix::createParameters(Array<ParameterData>&)
{
	internalData.init(nullptr, this);
}

}

}