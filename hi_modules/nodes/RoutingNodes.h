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

#pragma once

namespace scriptnode {
using namespace juce;
using namespace hise;

namespace routing
{

struct SendBase
{
    virtual ~SendBase() {};
	virtual Colour getColour() const = 0;
	virtual bool isConnected() const = 0;
};

class ReceiveNode : public HiseDspBase,
				   public SendBase
{
public:

	SET_HISE_NODE_ID("receive");
	SET_HISE_NODE_EXTRA_HEIGHT(5);
	SET_HISE_NODE_IS_MODULATION_SOURCE(false);
	GET_SELF_AS_OBJECT(ReceiveNode);
	
	ReceiveNode();

	void initialise(NodeBase* n);
	void prepare(PrepareSpecs ps);
	void createParameters(Array<ParameterData>& d) override;
	void reset();
	void process(ProcessData& data);
	Component* createExtraComponent(PooledUIUpdater* updater) override;
	bool handleModulation(double&) { return false; }
	void processSingle(float* frameData, int numChannels);
	Colour getColour() const override;
	bool isConnected() const override;

	void setGain(double newGain);

	float gainFactor = 0.0f;
	float singleFrameData[NUM_MAX_CHANNELS];

	NodePropertyT<bool> addToSignal;
	AudioSampleBuffer buffer;
	bool connectedOK = false;
	Colour c;

	JUCE_DECLARE_WEAK_REFERENCEABLE(ReceiveNode);
};

class SendNode : public HiseDspBase,
				   public SendBase
{
public:

	SET_HISE_NODE_ID("send");
	SET_HISE_NODE_EXTRA_HEIGHT(5);
	SET_HISE_NODE_IS_MODULATION_SOURCE(false);
	GET_SELF_AS_OBJECT(SendNode);

	SendNode();;
	~SendNode();

	void initialise(NodeBase* n) override;
	int getExtraWidth() const override;
	void reset();
	void prepare(PrepareSpecs);
	Colour getColour() const override;
	bool isConnected() const override;
	Component* createExtraComponent(PooledUIUpdater* updater) override;
	
	void process(ProcessData& data);
	void processSingle(float* frameData, int numChannels);
	bool handleModulation(double&) { return false; }
	void createParameters(Array<ParameterData>&) override;
	void connectTo(HiseDspBase* s);

	struct ConnectionNodeProperty : public NodeProperty
	{
		ConnectionNodeProperty(SendNode& parent);
		void postInit(NodeBase* n) override;
		void update(Identifier, var newValue);

		SendNode& p;
		valuetree::PropertyListener updater;
	} connectionUpdater;


	NodeBase::Ptr parent;
	WeakReference<ReceiveNode> connectedSource;

	JUCE_DECLARE_WEAK_REFERENCEABLE(SendNode);
};

struct MsEncoder : public HiseDspBase
{
	SET_HISE_NODE_ID("ms_encode");
	SET_HISE_NODE_EXTRA_HEIGHT(0);
	SET_HISE_NODE_IS_MODULATION_SOURCE(false);
	GET_SELF_AS_OBJECT(MsEncoder);

	HISE_EMPTY_RESET;
	HISE_EMPTY_PREPARE;
	HISE_EMPTY_CREATE_PARAM;
	
	void process(ProcessData& data);
	void processSingle(float* frameData, int numChannels);
	bool handleModulation(double&) { return false; }
};

struct MsDecoder : public HiseDspBase
{
	SET_HISE_NODE_ID("ms_decode");
	SET_HISE_NODE_EXTRA_HEIGHT(0);
	SET_HISE_NODE_IS_MODULATION_SOURCE(false);
	GET_SELF_AS_OBJECT(MsDecoder);

	HISE_EMPTY_RESET;
	HISE_EMPTY_PREPARE;
	HISE_EMPTY_CREATE_PARAM;

	void process(ProcessData& data);
	void processSingle(float* frameData, int numChannels);
	bool handleModulation(double&) { return false; }
};

struct Matrix : public HiseDspBase,
				public RoutableProcessor
{
#if USE_BACKEND
	struct Editor: public ExtraComponent<hise::RoutableProcessor>
	{
		Editor(RoutableProcessor* r, PooledUIUpdater* updater):
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

	SET_HISE_NODE_ID("matrix");
	SET_HISE_NODE_EXTRA_HEIGHT(200);
	SET_HISE_NODE_IS_MODULATION_SOURCE(false);
	GET_SELF_AS_OBJECT(Matrix);

	HISE_EMPTY_RESET;

	Matrix() :
		internalData(PropertyIds::EmbeddedData, "")
	{
		
	}

	void initialise(NodeBase* n) override
	{
		um = n->getUndoManager();
			
		getMatrix().init(dynamic_cast<Processor*>(n->getScriptProcessor()));


		ScopedValueSetter<bool> svs(recursion, true);

		internalData.setAdditionalCallback(BIND_MEMBER_FUNCTION_2(Matrix::updateFromEmbeddedData));
		internalData.init(n, this);
	}

	Component* createExtraComponent(PooledUIUpdater* updater)
	{
#if USE_BACKEND
		return new Editor(this, updater);
#else
		return nullptr;
#endif
	}

	void prepare(PrepareSpecs specs)
	{
		getMatrix().setNumSourceChannels(specs.numChannels);
		getMatrix().setNumDestinationChannels(specs.numChannels);
	}

	void numSourceChannelsChanged() { updateData(); };
	void numDestinationChannelsChanged() { updateData(); };
	void connectionChanged() { updateData(); };

	void updateFromEmbeddedData(Identifier id, var newValue)
	{
		auto base64Data = newValue.toString();

		if (base64Data.isNotEmpty())
		{
			auto matrixData = ValueTreeConverters::convertBase64ToValueTree(newValue.toString(), true);

			getMatrix().restoreFromValueTree(matrixData);
		}
	}

	void updateData()
	{
		if (recursion)
			return;

		ScopedValueSetter<bool> svs(recursion, true);

		auto data = ValueTreeConverters::convertValueTreeToBase64(getMatrix().exportAsValueTree(), true);

		internalData.storeValue(data, um);

		memset(channelRouting, -1, NUM_MAX_CHANNELS);
		memset(sendRouting, -1, NUM_MAX_CHANNELS);

		for (int i = 0; i < getMatrix().getNumSourceChannels(); i++)
		{
			channelRouting[i] = (int8)getMatrix().getConnectionForSourceChannel(i);
			sendRouting[i] = (int8)getMatrix().getSendForSourceChannel(i);
		}
	}

	void process(ProcessData& data) 
	{
		float frameData[NUM_MAX_CHANNELS];
		float* chData[NUM_MAX_CHANNELS];

		memcpy(chData, data.data, data.numChannels * sizeof(float*));

		ProcessData copy(chData, data.numChannels, data.size);

		if (data.size > 0)
		{
			copy.copyToFrameDynamic(frameData);
			getMatrix().setGainValues(frameData, true);
			processSingle(frameData, data.numChannels);
			getMatrix().setGainValues(frameData, false);
			copy.copyFromFrameAndAdvanceDynamic(frameData);
		}

		for (int i = 1; i < data.size; i++)
		{
			copy.copyToFrameDynamic(frameData);
			processSingle(frameData, data.numChannels);
			copy.copyFromFrameAndAdvanceDynamic(frameData);
		}

		
	};

	void processSingle(float* frameData, int numChannels) 
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
			
			if(index != -1)
				frameData[index] += chData[i];

			auto sendIndex = sendRouting[i];

			if (sendIndex != -1)
				frameData[sendIndex] += chData[i];
		}
	};
	bool handleModulation(double&) { return false; }

	void createParameters(Array<ParameterData>&) override
	{
		internalData.init(nullptr, this);
	}

	bool recursion = false;
	UndoManager* um = nullptr;

	int8 channelRouting[NUM_MAX_CHANNELS];
	int8 sendRouting[NUM_MAX_CHANNELS];

	NodePropertyT<String> internalData;

	JUCE_DECLARE_WEAK_REFERENCEABLE(Matrix);
};

using receive = ReceiveNode;
using send = SendNode;
using ms_encode = MsEncoder;
using ms_decode = MsDecoder;
using matrix = Matrix;

}


}
