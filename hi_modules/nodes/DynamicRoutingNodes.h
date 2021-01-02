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

	HISE_EMPTY_SET_PARAMETER;

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

		frameData = data;
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

	void connect(routing::receive<cable::dynamic>& receiveTarget)
	{
		setConnection(receiveTarget, true);
	}

	void setConnection(routing::receive<cable::dynamic>& receiveTarget, bool addAsConnection);

	span<float, NUM_MAX_CHANNELS> data_;
	dyn<float> frameData;

	heap<float> buffer;
	span<dyn<float>, NUM_MAX_CHANNELS> channels;

	int writeIndex = 0;
	int readIndex = 0;
	int numChannels = 0;

	bool shouldAdd = true;
	bool useFrameDataForDisplay = false;

	WeakReference<NodeBase> parentNode;

	NodePropertyT<String> receiveIds;
	PrepareSpecs currentSpecs;

	JUCE_DECLARE_WEAK_REFERENCEABLE(dynamic);
};
}
using dynamic_send = routing::send<cable::dynamic>;
using dynamic_receive = routing::receive<cable::dynamic>;

struct FunkySendComponent : public ScriptnodeExtraComponent<routing::base>,
	public DragAndDropTarget
{
	FunkySendComponent(routing::base* b, PooledUIUpdater* u);;

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


	dynamic_send* getAsSendNode()
	{
		if (auto c = dynamic_cast<dynamic_send*>(getObject()))
		{
			return c;
		}

		return nullptr;
	}

	dynamic_receive* getAsReceiveNode()
	{
		if (auto c = dynamic_cast<dynamic_receive*>(getObject()))
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

	static Component* createExtraComponent(routing::base* b, PooledUIUpdater* updater)
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

}


}
