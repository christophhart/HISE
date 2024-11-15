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


namespace parameter
{

	struct clone_holder : public dynamic_base_holder
	{
		clone_holder()
		{
			lastValues.ensureStorageAllocated(16);
		}

		static dynamic_base_holder* getParameterFunctionStatic(void* b);

		void callEachClone(int index, double v, bool);

		void setParameter(NodeBase* n, dynamic_base::Ptr b) override;

		void rebuild(NodeBase* targetContainer);

		ReferenceCountedArray<dynamic_base> cloneTargets;
		Array<double> lastValues;

		NodeBase::Ptr connectedCloneContainer;

		void setParentNumClonesListener(scriptnode::wrap::clone_manager::Listener* pl)
		{
			parentListener = pl;
		}

		bool isNormalised = true;
		WeakReference<scriptnode::wrap::clone_manager::Listener> parentListener;

		JUCE_DECLARE_WEAK_REFERENCEABLE(clone_holder);
	};
}

namespace duplilogic
{
	struct dynamic
	{
		enum class DupliMode
		{

			Spread,
			Scale,
			Harmonics,
			Random,
			Triangle,
			Fixed,
			Nyquist,
			Ducker,
			Toggle
		};

		using NodeType = control::clone_cable<parameter::clone_holder, dynamic>;

		dynamic() :
			mode(PropertyIds::Mode, "Spread")
		{};

		void initialise(NodeBase* n)
		{
			mode.initialise(n);
			mode.setAdditionalCallback(BIND_MEMBER_FUNCTION_2(dynamic::updateMode), true);
		}

		void updateMode(Identifier id, var newValue)
		{
			auto m = newValue.toString();
			currentMode = (DupliMode)getSpreadModes().indexOf(m);
		}

		bool shouldUpdateNumClones() const
		{
			switch (currentMode)
			{
			case DupliMode::Spread: return spread::shouldUpdateNumClones();
			case DupliMode::Scale: return scale::shouldUpdateNumClones();
			case DupliMode::Harmonics: return harmonics::shouldUpdateNumClones();
			case DupliMode::Random: return random::shouldUpdateNumClones();
			case DupliMode::Triangle: return triangle::shouldUpdateNumClones();
			case DupliMode::Fixed: return fixed::shouldUpdateNumClones();
			case DupliMode::Nyquist: return nyquist::shouldUpdateNumClones();
			case DupliMode::Ducker: return ducker::shouldUpdateNumClones();
			case DupliMode::Toggle: return toggle::shouldUpdateNumClones();
			default: return false;
			}
		}

		static constexpr bool isProcessingHiseEvent() { return true; }

		bool getMidiValue(HiseEvent& e, double& v)
		{
			switch (currentMode)
			{
			case DupliMode::Random:     return random().getMidiValue(e, v);
			case DupliMode::Fixed:      return fixed().getMidiValue(e, v);
			case DupliMode::Harmonics:  return harmonics().getMidiValue(e, v);
			case DupliMode::Nyquist:    return nyquist().getMidiValue(e, v);
			default:                    return false;
			}
		}

		double getValue(int index, int numDuplicates, double input, double gamma)
		{
			switch (currentMode)
			{
			case DupliMode::Spread: return spread().getValue(index, numDuplicates, input, gamma);
			case DupliMode::Scale: return scale().getValue(index, numDuplicates, input, gamma);
			case DupliMode::Harmonics: return harmonics().getValue(index, numDuplicates, input, gamma);
			case DupliMode::Random: return random().getValue(index, numDuplicates, input, gamma);
			case DupliMode::Triangle: return triangle().getValue(index, numDuplicates, input, gamma);
			case DupliMode::Fixed: return fixed().getValue(index, numDuplicates, input, gamma);
			case DupliMode::Nyquist: return nyquist().getValue(index, numDuplicates, input, gamma);
			case DupliMode::Ducker: return ducker().getValue(index, numDuplicates, input, gamma);
			case DupliMode::Toggle: return toggle().getValue(index, numDuplicates, input, gamma);
			}

			return 0.0;
		}

		static StringArray getSpreadModes()
		{
			return { "Spread", "Scale", "Harmonics", "Random", "Triangle", "Fixed", "Nyquist", "Ducker", "Toggle" };
		}

		NodePropertyT<String> mode;
		DupliMode currentMode;

		struct editor : public ScriptnodeExtraComponent<NodeType>
		{
			editor(NodeType* obj, PooledUIUpdater* u);;

			void timerCallback() override;

			static Component* createExtraComponent(void* obj, PooledUIUpdater* updater)
			{
				auto mn = static_cast<mothernode*>(obj);
				auto typed = dynamic_cast<NodeType*>(mn);
				return new editor(typed, updater);
			}

			void paint(Graphics& g) override;



			void resized() override
			{
				auto b = getLocalBounds();

				mode.setBounds(b.removeFromTop(28));
				b.removeFromTop(UIValues::NodeMargin);

				b.removeFromBottom(UIValues::NodeMargin);
				dragger.setBounds(b.removeFromBottom(28));
				b.removeFromBottom(UIValues::NodeMargin);
				area = b.toFloat();
			}

			Rectangle<float> area;
			ModulationSourceBaseComponent dragger;
			ComboBoxWithModeProperty mode;
			bool initialised = false;
		};
	};
}


namespace cable
{

struct dynamic: public cable::block_base<NUM_MAX_CHANNELS>
{
	using dynamic_send = routing::send<dynamic>;
	using dynamic_receive = routing::receive<dynamic>;

	using FrameType = dyn<float>;
	using BlockType = ProcessDataDyn;

	int getNumChannels() const 
	{ 
		return numChannels;
	};

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
		writeToBuffer(data);
	};

	void setIsNull()
	{
		isNull = true;
	}

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

	int numChannels = 0;
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

struct local_cable_base :
	public mothernode,
	public control::pimpl::parameter_node_base<parameter::dynamic_base_holder>,

	public control::pimpl::no_processing
{
	struct Manager: public ReferenceCountedObject
	{
		using Ptr = ReferenceCountedObjectPtr<Manager>;

		Manager(DspNetwork* n):
		  um(n->getUndoManager())
		{
			updater.enableLockFreeUpdate(n->getScriptProcessor()->getMainController_()->getGlobalUIUpdater());
		}

		~Manager()
		{
			registeredCables.clear();
		}

		void registerCable(WeakReference<local_cable_base> c);
		void deregisterCable(WeakReference<local_cable_base> c);

		void refreshAllConnections(const String& v);

		struct Item
		{
			Item(Manager& p, local_cable_base* c);

			void updateRanges(const Identifier& id, const var& newValue);

			void updateId(const Identifier&, const var& newValue);

			Manager& parent;
			WeakReference<local_cable_base> cable;
			ValueTree propTree, paramTree;
			valuetree::PropertyListener idListener;
			valuetree::PropertyListener rangeListener;

			int variableIndex = -1;
		};

		static constexpr int NumVariableSlots = 64;

		double getCurrentVariableValue(const String& id) const
		{
			auto idx = variables.indexOf(id);

			if(isPositiveAndBelow(idx, NumVariableSlots))
				return variableValues[idx];

			return 0.0;
		}

		void setVariableValue(int variableIndex, double value)
		{
			if(isPositiveAndBelow(variableIndex, NumVariableSlots))
				variableValues[variableIndex] = value;

			updater.sendMessage(sendNotificationAsync, variableIndex);
		}

		LambdaBroadcaster<int> updater;

		StringArray variables;

		span<double, NumVariableSlots> variableValues;

		CriticalSection lock;

		OwnedArray<Item> registeredCables;
		UndoManager* um;
	};

	struct editor: public ScriptnodeExtraComponent<local_cable_base>,
				   public GlobalRoutingManager::RoutingIcons
	{
		editor(local_cable_base* obj, PooledUIUpdater* u);

		void resized() override;
		void timerCallback() override;

		static Component* createExtraComponent(void* obj, PooledUIUpdater* u);

		int counter = 0;

		ModulationSourceBaseComponent dragger;
		ComboBoxWithModeProperty name;
		HiseShapeButton newButton, viewButton;
	};

	local_cable_base(const Identifier& id):
	  control::pimpl::parameter_node_base<scriptnode::parameter::dynamic_base_holder>(id),
	  currentId(PropertyIds::LocalId, {})
	{
		
	}

	using Helpers = LocalCableHelpers;

	~local_cable_base()
	{
		getManager()->deregisterCable(this);
	}

	void setValue(double v);

	

	struct ListItem: public Component
	{
		ListItem(DspNetwork* network_, const String& id):
		  network(network_)
		{
			setName(id);
			connections = Helpers::getListOfConnectedNodes(network, {}, id);
			auto mn = static_cast<Manager*>(network->getLocalCableManager());
			mn->updater.addListener(*this, update);

		}

		static void update(ListItem& item, int index)
		{
			item.repaint();
		}

		void mouseDown(const MouseEvent& e) override
		{
			if(e.mods.isRightButtonDown())
			{
				PopupLookAndFeel plaf;

				PopupMenu m;
				m.setLookAndFeel(&plaf);

				m.addItem(1, "Replace local cable with direct connections");

				auto r = m.show();

				if(r == 1)
				{
					Helpers::explode(connections.getFirst()->getValueTree(), connections.getFirst()->getUndoManager());
				}

				return;
			}
			else
			{
				Helpers::showAllOccurrences(network.get(), getName());
			}
		}

		void paint(Graphics& g) override
		{
			auto b = getLocalBounds().toFloat();

			 

			auto selection = network->getSelection();

			auto isSelected = false;

			for(auto c: connections)
			{
				for(auto s: selection)
				{
					isSelected |= c == s;
					if(isSelected)
						break;
				}

				if(isSelected)
					break;
			}

			if(auto c = connections.getFirst())
			{
				auto cl = GlobalRoutingManager::Helpers::getColourFromId(getName());

				g.setColour(cl.withAlpha(0.1f));
				g.fillRoundedRectangle(b.reduced(1.0f), 2.0f);

				if(isSelected)
				{
					g.setColour(Colour(SIGNAL_COLOUR));
					g.drawRoundedRectangle(b.reduced(1.0f), 2.0f, 1.0f);
				}

				auto rng = RangeHelpers::getDoubleRange(c->getParameterFromIndex(0)->data);

				auto mn = static_cast<Manager*>(network->getLocalCableManager());
				auto v = mn->getCurrentVariableValue(getName());
				auto normValue = rng.convertTo0to1(v, false);

				g.setColour(cl);

				auto l = b.removeFromLeft(b.getHeight()).reduced(JUCE_LIVE_CONSTANT_OFF(5.0f));

				g.drawEllipse(l, 1.0f);
				g.setColour(cl.withAlpha(jlimit<float>(0.0f, 1.0f, normValue)));
				g.fillEllipse(l.reduced(2.0f));
				g.setColour(Colours::white.withAlpha(0.8f));

				String text;
				text << getName() << " (" << String(connections.size()) << "x)";

				b.removeFromLeft(5.0f);

				g.setFont(GLOBAL_BOLD_FONT());
				g.drawText(text, b, Justification::left);
			}
		}

		

		Array<WeakReference<NodeBase>> connections;
		WeakReference<DspNetwork> network;

		JUCE_DECLARE_WEAK_REFERENCEABLE(ListItem);
	};

	void initialise(NodeBase* n);

	Manager::Ptr getManager() const;

	void sendValue(double v);

	void refreshConnection(int newVariableIndex);

	String getVariableId() const { return currentId.getValue(); }

	virtual void createParameters(ParameterDataList&) = 0;

private:

	int variableIndex = -1;

	NodePropertyT<String> currentId;
	WeakReference<NodeBase> node;

	hise::SimpleReadWriteLock lock;

	Array<WeakReference<NodeBase>> connections;

	bool recursion = false;
	
	JUCE_DECLARE_WEAK_REFERENCEABLE(local_cable_base);
};

struct local_cable: public local_cable_base
{
	SN_NODE_ID("local_cable");
	SN_GET_SELF_AS_OBJECT(local_cable);

	SN_ADD_SET_VALUE(local_cable);

	local_cable() :
	  local_cable_base(getStaticId())
	{};;
};

struct local_cable_unscaled: public local_cable_base,
						     public control::pimpl::no_mod_normalisation
{
	SN_NODE_ID("local_cable_unscaled");
	SN_GET_SELF_AS_OBJECT(local_cable_unscaled);

	SN_ADD_SET_VALUE(local_cable_unscaled);

	local_cable_unscaled() :
	  local_cable_base(getStaticId()),
	  no_mod_normalisation(getStaticId(), { "Value" })
	{};

	static constexpr bool isNormalisedModulation() { return false; }
};


struct dynamic_matrix : public RoutableProcessor
{
	static constexpr bool isFixedChannelMatrix() { return false; }
	static constexpr int getNumChannels() { return NUM_MAX_CHANNELS; };
	static constexpr bool hasSendChannels() { return true; }

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

	void updateData();

	static constexpr bool createDisplayValues() { return true; }

	void handleDisplayValues(bool isSource, ProcessDataDyn& data)
	{
		float values[NUM_MAX_CHANNELS];
		
		for(int i = 0; i < data.getNumChannels(); i++)
		{
			auto r = FloatVectorOperations::findMinAndMax(data.getRawChannelPointers()[i], data.getNumSamples());
			values[i] = jmax(r.getStart(), -r.getStart(), r.getEnd(), -r.getEnd());
		}

		getMatrix().setGainValues(values, isSource);
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
