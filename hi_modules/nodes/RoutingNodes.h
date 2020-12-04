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

/**  TODO SEND STUFF:


- add a dragging icon if not connected
- add error handling (also while dragging)

- remove old stuff

Refactor ideas:

- check whether the node property needs the HiseDspBase at all...
- remove getAsHardcodedNode() once and for all


*/



/** The classes defined in this namespace can be passed into the send / receive nodes
	as template parameters and define the data model of the connection.
*/
namespace cable
{

template <int C, int A> struct frame
{
	using FrameType = span<float, C>;
	using BlockType = ProcessData<C>;

	Colour colour = Colours::transparentBlack;
	constexpr int  getNumChannels() const { return C; };
	constexpr bool addToSignal() const { return A != 0; };
	static constexpr bool allowFrame() { return true; };
	static constexpr bool allowBlock() { return false; };

	void validate(PrepareSpecs receiveSpecs)
	{
		jassert(receiveSpecs.numChannels == getNumChannels());
		jassert(receiveSpecs.blockSize == 1);
	}

	void prepare(PrepareSpecs ps)
	{
		jassert(ps.numChannels <= C);
	}

	void reset()
	{
		for (auto& d : frameData)
			d = 0.0f;
	}

	void initialise(NodeBase* n) {};

	void processFrame(FrameType& t)
	{
		int index = 0;

		for (auto& d : t)
			frameData[index++] = d;
	}

	void incCounter(bool, int) noexcept {}

	void process(BlockType& unused)
	{
		jassertfalse;
	};

	template <typename T> void connect(T& receiveTarget)
	{
		receiveTarget.source = this;
	}

	span<float, C> frameData;
};


template <int C, int A> struct block
{
	using FrameType = span<float, C>;
	using BlockType = ProcessData<C>;

	Colour colour = Colours::transparentBlack;


	constexpr int  getNumChannels() const { return C; };

	constexpr bool addToSignal() const { return A != 0; };

	static constexpr bool allowFrame() { return false; };
	static constexpr bool allowBlock() { return true; };

	void initialise(NodeBase* n) {};

	void validate(PrepareSpecs receiveSpecs)
	{
		jassert(receiveSpecs.numChannels == getNumChannels());
	}

	void prepare(PrepareSpecs ps)
	{
		jassert(ps.numChannels <= getNumChannels());
		DspHelpers::increaseBuffer(buffer, ps);

		int index = 0;

		auto d = ProcessDataHelpers<C>::makeChannelData(buffer);

		for (auto& ch : d)
			channels[index++].referTo(ch, (size_t)ps.blockSize);
	};

	template <typename T> void connect(T& receiveTarget)
	{
		receiveTarget.source = this;
	}

	void reset()
	{
		for (auto& d : channels)
			hmath::vset(d, 0.0f);
	}

	void processFrame(FrameType& unused)
	{
		ignoreUnused(unused);
		jassertfalse;
	}

	void incCounter(bool incReadCounter, int delta)
	{
		auto& counter = incReadCounter ? readIndex : writeIndex;

		counter += delta;

		if (counter == channels[0].size())
			counter = 0;
	}

	void process(BlockType& data)
	{
		int numThisTime = data.getNumSamples();

		int index = 0;
		for (auto c : data)
		{
			auto src = c.getRawWritePointer();
			auto dst = channels[index++].begin() + writeIndex;

			FloatVectorOperations::copy(dst, src, numThisTime);
		}

		incCounter(false, numThisTime);
	};

	heap<float> buffer;
	span<dyn<float>, C> channels;
	int writeIndex = 0;
	int readIndex = 0;
};

}

namespace routing2
{

struct base
{
	virtual ~base() {};

	virtual Colour getColour() const = 0;

	JUCE_DECLARE_WEAK_REFERENCEABLE(base);
};

template <typename CableType> struct receive: public base
{
	SET_HISE_NODE_ID("receive2");

	GET_SELF_AS_OBJECT();

	enum class Parameters
	{
		Feedback
	};

	constexpr bool isPolyphonic() const { return false; }

	template <int P> static void setParameter(void* obj, double value)
	{
		if (P == (int)Parameters::Feedback) static_cast<receive*>(obj)->setFeedback(value);
		//DEF_PARAMETER(Feedback, receive);
	}

	HISE_EMPTY_RESET;

	bool isConnected() const
	{
		return &null != source;
	}

	void disconnect()
	{
		source = &null;
		currentError = {};
	}

	void handleHiseEvent(HiseEvent& e) {}

	void initialise(NodeBase* n)
	{
		
	}

	void prepare(PrepareSpecs ps)
	{
		currentSpecs = ps;

		if (isConnected())
		{
			source->validate(currentSpecs);
		}
	}

	template <typename ProcessDataType> void process(ProcessDataType& data)
	{
		if (CableType::allowBlock())
		{
			if (auto srcPointer = source->buffer.begin())
			{
				int numToReadThisTime = data.getNumSamples();

				int i = 0;

				for (auto& ch : data)
				{
					auto src = source->channels[i++].begin() + source->readIndex;

					if (source->addToSignal())
						FloatVectorOperations::addWithMultiply(ch.getRawWritePointer(), src, feedback, data.getNumSamples());
					else
						FloatVectorOperations::copy(ch.getRawWritePointer(), src, data.getNumSamples());
				}

				source->incCounter(true, numToReadThisTime);
			}
		}
	}

	template <typename FrameDataType> void processFrame(FrameDataType& data)
	{
		if (CableType::allowFrame())
		{
			jassert(data.size() <= source->frameData.size());

			if (source->addToSignal())
			{
				int index = 0;

				for (auto& d : data)
					d += source->frameData[index++] * feedback;
			}
			else
			{
				int index = 0;

				for (auto& d : data)
					d = source->frameData[index++] * feedback;
			}
		}
		else
		{
			// should never be called...
			jassertfalse;
		}
	}

	void createParameters(Array<ParameterDataImpl>& data)
	{
		DEFINE_PARAMETERDATA(receive, Feedback);
		p.range = { 0.0, 1.0, 0.01 };
		p.defaultValue = 0.0;
		data.add(p);
	}

	Colour getColour() const override 
	{ 
		if(isConnected())
			return source->colour; 

		return Colours::transparentBlack;
	}

	void setFeedback(double value)
	{
		feedback = (float)jlimit(0.0, 1.0, value);
	}

	float feedback = 0.0f;

	PrepareSpecs currentSpecs;

	CableType null;
	CableType* source = &null;
};


template <typename CableType> struct send: public base
{
	SET_HISE_NODE_ID("send2");

	GET_SELF_AS_OBJECT();

	constexpr bool isPolyphonic() const { return false; }

	template <typename Target> void connect(Target& t)
	{
		cable.connect(t);
	}

	void handleHiseEvent(HiseEvent& e) {};

	void createParameters(Array<ParameterDataImpl>&) {};

	void initialise(NodeBase* n)
	{
		cable.initialise(n);
	}

	void prepare(PrepareSpecs ps)
	{
		cable.prepare(ps);
	}

	void reset()
	{
		cable.reset();
	}

	Colour getColour() const override { return cable.colour; }

	template <typename ProcessDataType> void process(ProcessDataType& data)
	{
		cable.process(data);
	}

	template <typename FrameDataType> void processFrame(FrameDataType& data)
	{
		cable.processFrame(data);
	}

	CableType cable;
};

}

namespace cable
{

struct dynamic
{
	using FrameType = dyn<float>;
	using BlockType = ProcessDataDyn;

	int  getNumChannels() const 
	{ 
		return numChannels;
	};

	bool addToSignal() const { return shouldAdd; };
	Colour colour = Colours::blue;

	static constexpr bool allowFrame() { return true; };
	static constexpr bool allowBlock() { return true; };

	dynamic():
		receiveIds(PropertyIds::Connection, "")
	{}

	void prepare(PrepareSpecs ps);

	void reset()
	{
		for (auto& d : frameData)
			d = 0.0f;

		for (auto& v : buffer)
			v = 0.0f;
	}

	void validate(PrepareSpecs receiveSpecs)
	{
		DspHelpers::validate(currentSpecs, receiveSpecs);
	}

	void restoreConnections(Identifier id, var newValue);

	void initialise(NodeBase* n)
	{
		parentNode = n;

		receiveIds.setAdditionalCallback(BIND_MEMBER_FUNCTION_2(dynamic::restoreConnections));
		receiveIds.initialise(n);
	};

	template <typename FrameDataType> void processFrame(FrameDataType& data)
	{
		jassert(data.size() == frameData.size());

		int index = 0;
		for (auto& d : data)
			frameData[index++] = d;
	}

	template <typename ProcessDataType> void process(ProcessDataType& data)
	{
		int numThisTime = data.getNumSamples();

		int index = 0;
		for (auto c : data)
		{
			auto src = c.getRawWritePointer();
			auto dst = channels[index++].begin() + writeIndex;

			FloatVectorOperations::copy(dst, src, numThisTime);
		}

		incCounter(false, numThisTime);
	};

	void incCounter(bool incReadCounter, int delta)
	{
		auto& counter = incReadCounter ? readIndex : writeIndex;

		counter += delta;

		if (counter == channels[0].size())
			counter = 0;
	}

	String getSendId() const
	{
		jassert(parentNode != nullptr);
		return parentNode->getId();
	}

	void connect(routing2::receive<cable::dynamic>& receiveTarget)
	{
		setConnection(receiveTarget, true);
	}

	void setConnection(routing2::receive<cable::dynamic>& receiveTarget, bool addAsConnection);

	span<float, NUM_MAX_CHANNELS> data_;
	dyn<float> frameData;

	heap<float> buffer;
	span<dyn<float>, NUM_MAX_CHANNELS> channels;

	int writeIndex = 0;
	int readIndex = 0;
	int numChannels = 0;

	bool shouldAdd = true;

	WeakReference<NodeBase> parentNode;

	NodePropertyT<String> receiveIds;
	PrepareSpecs currentSpecs;

	JUCE_DECLARE_WEAK_REFERENCEABLE(dynamic);
};
}
using dynamic_send = routing2::send<cable::dynamic>;
using dynamic_receive = routing2::receive<cable::dynamic>;


struct FunkySendComponent : public ScriptnodeExtraComponent<routing2::base>,
	public DragAndDropTarget
{
	FunkySendComponent(routing2::base* b, PooledUIUpdater* u);;

	void resized() override
	{
		auto b = getLocalBounds();
		b.removeFromLeft(7);
		levelDisplay.setBounds(b.reduced(1));
	}

	Error checkConnectionWhileDragging(const SourceDetails& dragSourceDetails);

	bool isValidDragTarget(FunkySendComponent* other)
	{
		if (other == this)
			return false;

		auto srcIsSend = other->getAsSendNode() != nullptr;
		auto thisIsSend = getAsSendNode() != nullptr;

		return srcIsSend != thisIsSend;
	}

	bool isInterestedInDragSource(const SourceDetails& dragSourceDetails) override
	{
		if (auto src = dynamic_cast<FunkySendComponent*>(dragSourceDetails.sourceComponent.get()))
		{
			return isValidDragTarget(src);
		}

		return false;

	}
	void itemDragEnter(const SourceDetails& dragSourceDetails) override;

	void itemDragExit(const SourceDetails& dragSourceDetails) override;

	void mouseEnter(const MouseEvent& ) override { repaint(); }

	void mouseExit(const MouseEvent& ) override { repaint(); }


	routing2::send<cable::dynamic>* getAsSendNode()
	{
		if (auto c = dynamic_cast<routing2::send<cable::dynamic>*>(getObject()))
		{
			return c;
		}

		return nullptr;
	}

	routing2::receive<cable::dynamic>* getAsReceiveNode()
	{
		if (auto c = dynamic_cast<routing2::receive<cable::dynamic>*>(getObject()))
		{
			return c;
		}

		return nullptr;
	}

	void itemDropped(const SourceDetails& dragSourceDetails) override;

	void paintOverChildren(Graphics& g) override;

	void timerCallback() override;

	DragAndDropContainer* getDragAndDropContainer();

	static Image createDragImage(const String& m, Colour bgColour);

	void mouseDown(const MouseEvent& event) override;

	void mouseUp(const MouseEvent& e) override;

	static Component* createExtraComponent(routing2::base* b, PooledUIUpdater* updater)
	{
		return new FunkySendComponent(b, updater);
	}

	void paint(Graphics& g) override;

	bool dragOver = false;
	bool dragMode = false;

	VuMeter levelDisplay;
	Error currentDragError;

};


namespace routing
{
struct SendBase
{
	void validateConnection(const PrepareSpecs& sp, const PrepareSpecs& rp);

    virtual ~SendBase() {};
	virtual Colour getColour() const = 0;
	virtual bool isConnected() const = 0;

	PrepareSpecs receiveSpecs;
	PrepareSpecs sendSpecs;

	JUCE_DECLARE_WEAK_REFERENCEABLE(SendBase)
};

class ReceiveNode : public HiseDspBase,
				   public SendBase
{
public:

	SET_HISE_NODE_ID("receive");

#if RE
	SET_HISE_NODE_EXTRA_HEIGHT(5);
#endif

	SET_HISE_NODE_IS_MODULATION_SOURCE(false);
	GET_SELF_AS_OBJECT(ReceiveNode);
	
	ReceiveNode();

	void initialise(NodeBase* n);
	void prepare(PrepareSpecs ps);
	void createParameters(Array<ParameterData>& d) override;
	void reset();

	template <typename ProcessDataType> void process(ProcessDataType& data)
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

	template <typename FrameDataType> void processFrame(FrameDataType& data)
	{
		if (addToSignal.getValue())
		{
			int i = 0;
			for (auto& s : data)
				s += singleFrameData[i++] * gainFactor;
		}
		else
		{
			int i = 0;
			for (auto& s : data)
				s = singleFrameData[i++] * gainFactor;
		}
	}

	bool handleModulation(double&) { return false; }

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

#if RE
	SET_HISE_NODE_EXTRA_HEIGHT(5);
	SET_HISE_NODE_EXTRA_WIDTH(64);
#endif

	SET_HISE_NODE_IS_MODULATION_SOURCE(false);
	GET_SELF_AS_OBJECT(SendNode);

	SendNode();;
	~SendNode();

	void initialise(NodeBase* n) override;

	void reset();
	void prepare(PrepareSpecs);
	Colour getColour() const override;
	bool isConnected() const override;
	
	template <typename ProcessDataType> void process(ProcessDataType& data)
	{
		if (connectedSource != nullptr)
		{
			int numChannelsToSend = jmin(connectedSource->buffer.getNumChannels(), data.getNumChannels());
			int numSamplesToSend = jmin(connectedSource->buffer.getNumSamples(), data.getNumSamples());

			for (int i = 0; i < numChannelsToSend; i++)
				connectedSource->buffer.copyFrom(i, 0, data[i].begin(), numSamplesToSend);
		}
	}

	template <typename FrameDataType> void processFrame(FrameDataType& data)
	{
		if (connectedSource != nullptr)
		{
			int i = 0;
			for(auto& s: data)
				connectedSource->singleFrameData[i++] = s;
		}
	}

	bool handleModulation(double&) { return false; }
	void createParameters(Array<ParameterData>&) override;
	void connectTo(ReceiveNode* s);

	void connect(ReceiveNode& r)
	{
		connectTo(&r);
	}

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

#if RE
	SET_HISE_NODE_EXTRA_HEIGHT(0);
#endif
	SET_HISE_NODE_IS_MODULATION_SOURCE(false);
	GET_SELF_AS_OBJECT(MsEncoder);

	HISE_EMPTY_RESET;
	HISE_EMPTY_PREPARE;
	HISE_EMPTY_CREATE_PARAM;
	
	template <typename ProcessDataType> void process(ProcessDataType& data)
	{
		DspHelpers::forwardToFrameStereo(this, data);
	}

	template <typename FrameDataType> void processFrame(FrameDataType& data)
	{
		if (data.size() == 2)
		{
			auto& l = data[0];
			auto& r = data[1];

			auto m = (l + r) * 0.5f;
			auto s = (l - r) * 0.5f;

			l = m;
			r = s;
		}
	}

	bool handleModulation(double&) { return false; }
};

struct MsDecoder : public HiseDspBase
{
	SET_HISE_NODE_ID("ms_decode");

#if RE
	SET_HISE_NODE_EXTRA_HEIGHT(0);
#endif

	SET_HISE_NODE_IS_MODULATION_SOURCE(false);
	GET_SELF_AS_OBJECT(MsDecoder);

	HISE_EMPTY_RESET;
	HISE_EMPTY_PREPARE;
	HISE_EMPTY_CREATE_PARAM;

	template <typename ProcessDataType> void process(ProcessDataType& data)
	{
		DspHelpers::forwardToFrameStereo(this, data);
	}

	template <typename FrameDataType> void processFrame(FrameDataType& data)
	{
		if (data.size() == 2)
		{
			auto& m = data[0];
			auto& s = data[1];

			auto l = (m + s);
			auto r = (m - s);

			m = l;
			s = r;
		}
	}


	bool handleModulation(double&) { return false; }
};

struct dynamic_matrix : public RoutableProcessor
{
	static constexpr bool isFixedChannelMatrix() { return false; }
	static constexpr int getNumChannels() { return NUM_MAX_CHANNELS; };

	dynamic_matrix() :
		internalData(PropertyIds::EmbeddedData, "")
	{

	}

	void numSourceChannelsChanged() { updateData(); };
	void numDestinationChannelsChanged() { updateData(); };
	void connectionChanged() { updateData(); };

	void prepare(PrepareSpecs specs)
	{
		getMatrix().setNumSourceChannels(specs.numChannels);
		getMatrix().setNumDestinationChannels(specs.numChannels);
	}

	void updateData()
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

	

	void initialise(NodeBase* n)
	{
		um = n->getUndoManager();

		getMatrix().init(dynamic_cast<Processor*>(n->getScriptProcessor()));
		//ScopedValueSetter<bool> svs(recursion, true);

		internalData.setAdditionalCallback(BIND_MEMBER_FUNCTION_2(dynamic_matrix::updateFromEmbeddedData));
		internalData.initialise(n);
	}

	void updateFromEmbeddedData(Identifier id, var newValue)
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

	int getChannel(int index) const
	{
		return (int)channelRouting[index];
	}

	int getSendChannel(int index) const
	{
		return (int)sendRouting[index];
	}

	bool recursion = false;
	UndoManager* um = nullptr;

	int8 channelRouting[NUM_MAX_CHANNELS];
	int8 sendRouting[NUM_MAX_CHANNELS];

	NodePropertyT<String> internalData;

	
};

template <class MatrixType> struct matrix
{
	SET_HISE_NODE_ID("matrix");

	GET_SELF_AS_OBJECT();

	HISE_EMPTY_RESET;

	void prepare(PrepareSpecs specs)
	{
		m.prepare(specs);
	}

	void initialise(NodeBase* node)
	{
		m.initialise(node);
	}
	
	void handleHiseEvent(HiseEvent& e) {};

	template <typename ProcessDataType> void process(ProcessDataType& data)
	{
		if (MatrixType::isFixedChannelMatrix())
		{
			ProcessDataHelpers<MatrixType::getNumChannels()>::processFix(this, data);
		}
		else
		{
			DspHelpers::forwardToFrame16(this, data);
		}

		
	}
	
	template <typename FrameDataType> void processFrame(FrameDataType& data)
	{
		if (MatrixType::isFixedChannelMatrix())
		{
			auto& fd = span<float, MatrixType::getNumChannels()>::as(data.begin());
			processMatrixFrame(fd);
		}
		else
		{
			processMatrixFrame(data);
		}
	}

	template <typename FrameDataType> void processMatrixFrame(FrameDataType& data)
	{
		constexpr int MaxChannelAmount = MatrixType::getNumChannels();

		float copyData[MaxChannelAmount + 1];
		copyData[0] = 0.0f;
		float* chData = copyData + 1;

		for (int i = 0; i < data.size(); i++)
		{
			chData[i] = data[i];
			data[i] = 0.0f;
		}

		for (int i = 0; i < data.size(); i++)
		{
			auto index = m.getChannel(i);

			if (index != -1)
				data[index] += chData[i];

			auto sendIndex = m.getSendChannel(i);

			if (sendIndex != -1)
				data[sendIndex] += chData[i];
		}
	}

	constexpr bool isPolyphonic() const { return false; }

	void createParameters(Array<ParameterDataImpl>&)
	{
		
	}

	MatrixType m;

	JUCE_DECLARE_WEAK_REFERENCEABLE(matrix);
};

using receive = ReceiveNode;
using send = SendNode;
using ms_encode = MsEncoder;
using ms_decode = MsDecoder;

}


}
