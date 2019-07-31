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
	SET_HISE_NODE_EXTRA_WIDTH(128);
	SET_HISE_NODE_IS_MODULATION_SOURCE(false);
	GET_SELF_AS_OBJECT(SendNode);

	SendNode();;
	~SendNode();

	void initialise(NodeBase* n) override;

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


	SET_HISE_NODE_ID("matrix");
	SET_HISE_NODE_EXTRA_HEIGHT(200);
	SET_HISE_NODE_IS_MODULATION_SOURCE(false);
	GET_SELF_AS_OBJECT(Matrix);

	HISE_EMPTY_RESET;

#if USE_BACKEND
	struct Editor;
#endif

	Matrix();

	void initialise(NodeBase* n) override;

	Component* createExtraComponent(PooledUIUpdater* updater);

	void numSourceChannelsChanged() { updateData(); };
	void numDestinationChannelsChanged() { updateData(); };
	void connectionChanged() { updateData(); };

	void updateFromEmbeddedData(Identifier id, var newValue);
	void updateData();

	void prepare(PrepareSpecs specs);
	void process(ProcessData& data);;
	void processSingle(float* frameData, int numChannels);;
	bool handleModulation(double&) { return false; }
	void createParameters(Array<ParameterData>&) override;

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
