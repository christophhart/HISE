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
	using dynamic_send = routing::send<dynamic>;
	using dynamic_receive = routing::receive<dynamic>;

	using FrameType = dyn<float>;
	using BlockType = ProcessDataDyn;

	int getNumChannels() const 
	{ 
		return numChannels;
	};

	bool addToSignal() const { return shouldAdd; };
	Colour colour = Colours::blue;

	static constexpr bool allowFrame() { return true; };
	static constexpr bool allowBlock() { return true; };

	static NamespacedIdentifier getReceiveId();

	dynamic();

	SN_EMPTY_SET_PARAMETER;

	void prepare(PrepareSpecs ps);
	void reset();
	void validate(PrepareSpecs receiveSpecs);
	void restoreConnections(Identifier id, var newValue);
	void initialise(NodeBase* n);;

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

	void setIsNull()
	{
		isNull = true;
	}

	void incCounter(bool incReadCounter, int delta);

	String getSendId() const
	{
		jassert(parentNode != nullptr);
		return parentNode->getId();
	}

	void connect(routing::receive<cable::dynamic>& receiveTarget);

	void setConnection(routing::receive<cable::dynamic>& receiveTarget, bool addAsConnection);

	void checkSourceAndTargetProcessSpecs();

	span<float, NUM_MAX_CHANNELS> data_;
	dyn<float> frameData;

	heap<float> buffer;
	span<dyn<float>, NUM_MAX_CHANNELS> channels;

	int writeIndex = 0;
	int readIndex = 0;
	int numChannels = 0;

	bool shouldAdd = true;
	bool useFrameDataForDisplay = false;
	bool isNull = false;

	WeakReference<NodeBase> parentNode;

	NodePropertyT<String> receiveIds;

	PrepareSpecs sendSpecs;
	PrepareSpecs receiveSpecs;
	bool postPrepareCheckActive = false;


	JUCE_DECLARE_WEAK_REFERENCEABLE(dynamic);

	struct editor : public ScriptnodeExtraComponent<routing::base>,
		public DragAndDropTarget
	{
		editor(routing::base* b, PooledUIUpdater* u);;

		void resized() override;

		Error checkConnectionWhileDragging(const SourceDetails& dragSourceDetails);

		bool isValidDragTarget(editor* other);

		bool isInterestedInDragSource(const SourceDetails& dragSourceDetails) override;
		void itemDragEnter(const SourceDetails& dragSourceDetails) override;

		void itemDragExit(const SourceDetails& dragSourceDetails) override;

		void mouseEnter(const MouseEvent&) override { repaint(); }
		void mouseExit(const MouseEvent&) override { repaint(); }

		void mouseDrag(const MouseEvent& event) override;

		void mouseDoubleClick(const MouseEvent& event) override;

		bool isConnected();

		void updatePeakMeter();

		dynamic_send* getAsSendNode();
		dynamic_receive* getAsReceiveNode();

		void itemDropped(const SourceDetails& dragSourceDetails) override;

		void paintOverChildren(Graphics& g) override;

		void timerCallback() override;

		DragAndDropContainer* getDragAndDropContainer();

		static Image createDragImage(const String& m, Colour bgColour);

		void mouseDown(const MouseEvent& event) override;

		void mouseUp(const MouseEvent& e) override;

		static Component* createExtraComponent(void* b, PooledUIUpdater* updater)
		{
			auto typed = static_cast<routing::base*>(b);
			return new editor(typed, updater);
		}

		void paint(Graphics& g) override;

		bool dragOver = false;
		bool dragMode = false;

		Path icon;

		VuMeter levelDisplay;
		Error currentDragError;
	};
};
}
using dynamic_send =  routing::send<cable::dynamic>;
using dynamic_receive = routing::receive<cable::dynamic>;

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
