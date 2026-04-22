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

namespace hise
{
using namespace juce;

struct PageTabComponent : public Component,
	public ButtonListener,
	public PathFactory
{
	static constexpr int GroupHeight = 20;
	static constexpr int TabHeight = 20;

	PageTabComponent(UndoManager* um_) : um(um_),
	  expandButton("expand", this, *this)
	{
		addAndMakeVisible(expandButton);
		expandButton.setTooltip("Expand and show all sliders at once");
	};

	virtual ~PageTabComponent() {};

	Path createPath(const String& url) const override
	{
		static const unsigned char pathData[] = { 110,109,0,128,38,68,14,5,143,67,98,78,162,42,68,14,5,143,67,149,253,45,68,114,187,149,67,149,253,45,68,0,0,158,67,98,149,253,45,68,144,68,166,67,78,162,42,68,242,250,172,67,0,128,38,68,242,250,172,67,98,184,93,34,68,242,250,172,67,135,2,31,68,144,68,
166,67,135,2,31,68,0,0,158,67,98,135,2,31,68,114,187,149,67,184,93,34,68,14,5,143,67,0,128,38,68,14,5,143,67,99,109,203,198,50,68,60,182,149,67,108,47,159,43,68,248,102,135,67,108,134,179,55,68,124,124,94,67,108,194,14,49,68,56,233,67,67,108,0,128,69,
68,56,233,67,67,108,0,128,69,68,38,215,138,67,108,60,219,62,68,6,27,123,67,108,203,198,50,68,60,182,149,67,99,109,46,57,26,68,60,182,149,67,108,209,36,14,68,6,27,123,67,108,0,128,7,68,38,215,138,67,108,0,128,7,68,56,233,67,67,108,62,241,27,68,56,233,
67,67,108,116,76,21,68,124,124,94,67,108,208,96,33,68,248,102,135,67,108,46,57,26,68,60,182,149,67,99,109,203,198,50,68,196,73,166,67,108,60,219,62,68,126,114,190,67,108,0,128,69,68,220,40,177,67,108,0,128,69,68,102,11,218,67,108,194,14,49,68,102,11,
218,67,108,134,179,55,68,194,193,204,67,108,47,159,43,68,10,153,180,67,108,203,198,50,68,196,73,166,67,99,109,46,57,26,68,196,73,166,67,108,208,96,33,68,10,153,180,67,108,116,76,21,68,194,193,204,67,108,62,241,27,68,102,11,218,67,108,0,128,7,68,102,11,
218,67,108,0,128,7,68,220,40,177,67,108,209,36,14,68,126,114,190,67,108,46,57,26,68,196,73,166,67,99,101,0,0 };

		Path path;
		path.loadPathFromData(pathData, sizeof(pathData));

		return path;
	}

	struct Group
	{
		Group(const String& groupName) :
			name(groupName)
		{};

		void drawGroup(Graphics& g, Component& parent, bool drawOutline)
		{
			if (groupBounds.isEmpty())
			{
				RectangleList<int> bounds;

				for (auto& c : groupComponents)
				{
					jassert(c->getParentComponent() == &parent);
					bounds.addWithoutMerging(c->getBoundsInParent());
				}

				groupBounds = bounds.getBounds();
			}

			g.setColour(Colours::white.withAlpha(0.1f));

			if(drawOutline)
				g.drawRoundedRectangle(groupBounds.reduced(2.0f).toFloat(), 4.0f, 2.0f);

			auto title = groupBounds.expanded(0, GroupHeight).removeFromTop(GroupHeight).toFloat();

			g.fillRoundedRectangle(title.reduced(2.0f), 4.0f);

			g.setColour(Colours::white.withAlpha(0.4f));
			g.setFont(GLOBAL_BOLD_FONT());
			g.drawText(name, title, Justification::centred);
		}

		Rectangle<int> groupBounds;

		using PageHeightFunction = std::function<int(const std::vector<Group>&)>;
		using LayoutFunction = std::function<void(Rectangle<int>, const std::vector<Group>&, bool)>;

		String name;
		Array<Component::SafePointer<Component>> groupComponents;
	};

	struct Laf : public GlobalHiseLookAndFeel
	{
		void drawButtonBackground(Graphics& g, Button& button, const Colour&, bool isMouseOverButton, bool isButtonDown)
		{
			auto tb = button.getLocalBounds().toFloat();

			auto on = button.getToggleState();

			auto c1 = Colours::black.withAlpha(0.2f);
			auto c2 = Colours::white.withAlpha(0.1f);
			auto c3 = Colours::white.withAlpha(0.4f);

			g.setColour(on ? c2 : c1);
			g.fillRoundedRectangle(tb, 4.0f);
			g.setFont(GLOBAL_BOLD_FONT());
			g.setColour(on ? c3 : c2);
			g.drawText(button.getButtonText(), tb, Justification::centred);
		}

		void drawButtonText(Graphics& g, TextButton& button, bool, bool) {}
	} laf;

	struct TabComponent : public Component
	{
		TabComponent(const String& page, bool drawOutline) :
			Component(page),
			outline(drawOutline)
		{};

		

		void addSlider(Component* c)
		{
			groups.back().groupComponents.add(c);
			addAndMakeVisible(c);
		}

		void addGroup(String groupName)
		{
			groups.push_back(Group(groupName));
		}

		void paint(Graphics& g) override
		{
			for (auto& gr : groups)
				gr.drawGroup(g, *this, outline);
		}

		void resized() override
		{
			auto drawGroups = false;

			for (auto& g : groups)
				drawGroups |= g.name.isNotEmpty();

			if (auto pc = findParentComponentOfClass<PageTabComponent>())
				pc->setTabLayout(this, drawGroups);
		}

		std::vector<Group> groups;
		const bool outline;
	};

	using ParameterBuilder = std::function<Component* (int)>;

	bool drawOutline = false;

	void addPage(String p)
	{
		if (p.isEmpty())
			p = "Unassigned";

		auto nb = new TextButton(p);
		nb->setRadioGroupId(2155);
		nb->setLookAndFeel(&laf);
		nb->setClickingTogglesState(true);
		nb->addListener(this);
		addAndMakeVisible(nb);
		tabButtons.add(nb);

		auto tab = new TabComponent(p, drawOutline);
		addAndMakeVisible(tab);
		pages.add(tab);
	}

	void addSlider(Component* p)
	{
		if (pages.isEmpty())
			addPage("");

		pages.getLast()->addSlider(p);
	}

	void addGroup(const String& group)
	{
		jassert(!pages.isEmpty());

		hasGroups |= group.isNotEmpty();

		pages.getLast()->addGroup(group);
	}

	Group::LayoutFunction layoutFunction;
	Group::PageHeightFunction pageHeightFunction;

	virtual int getPageHeight(TabComponent* tc) const 
	{
		jassert(pageHeightFunction);
		return pageHeightFunction(tc->groups);
	}

	virtual void setTabLayout(TabComponent* t, bool drawGroups)
	{
		jassert(layoutFunction);
		auto b = t->getLocalBounds();
		layoutFunction(b, t->groups, drawGroups);
	}

	int getMaxHeight() const
	{
		int h = 0;

		for (auto p : pages)
			h = jmax(h, getPageHeight(p));

		h += TabHeight;

		return h;
	}

	virtual ValueTree getNodeTree() = 0;

	void buildParameters(const PageInfo::Tree& tree, const ParameterBuilder& cp)
	{
		for (const auto& p : tree.getPageNames())
		{
			addPage(p);

			for (const auto& g : tree.getGroups(p))
			{
				addGroup(g);

				for (auto& l : tree.getList(p, g))
				{
					auto s = cp(l.info.index);
					addSlider(s);
				}
			}
		}

		if (getNodeTree().isValid())
		{
			auto pageIndex = (int)getNodeTree()[scriptnode::PropertyIds::CurrentPageIndex];
			setTab(pageIndex);
		}
		else
			setTab(0);
	}

	virtual void onExpandTabs() {}

	void buttonClicked(Button* b) override
	{
		if(b == &expandButton)
		{
			if(getNodeTree().isValid())
				getNodeTree().setProperty(PropertyIds::CurrentPageIndex, -1, um);

			onExpandTabs();

			return;
		}

		auto pageIndex = tabButtons.indexOf(dynamic_cast<TextButton*>(b));

		auto nt = getNodeTree();

		if (nt.isValid())
			nt.setProperty(scriptnode::PropertyIds::CurrentPageIndex, pageIndex, um);

		for (int i = 0; i < pages.size(); i++)
		{
			pages[i]->setVisible(i == pageIndex);
		}
	}

	void resized() override
	{
		auto b = getLocalBounds();

		auto topRow = b.removeFromTop(TabHeight);
		topRow.removeFromLeft(2.0f);
		auto f = GLOBAL_BOLD_FONT();


		for (auto t : tabButtons)
		{
			auto bw = f.getStringWidthFloat(t->getButtonText()) + 20;
			t->setBounds(topRow.removeFromLeft(bw));
			topRow.removeFromLeft(4);
		}

		expandButton.setBounds(topRow.removeFromLeft(topRow.getHeight()).reduced(3));

		for (auto p : pages)
		{
			p->setBounds(b);
		}
	}

	void setTab(int tabIndex)
	{
		if (auto tb = tabButtons[tabIndex])
		{
			tb->setToggleState(true, sendNotificationSync);
		}
	}

	bool hasGroups = false;

	UndoManager* um = nullptr;

	OwnedArray<TextButton> tabButtons{};
	OwnedArray<TabComponent> pages{};
	HiseShapeButton expandButton;
};

class HardcodedSwappableEffect : public HotswappableProcessor,
							     public ProcessorWithExternalData,
								 public ProcessorWithCustomFilterStatistics,
								 public RuntimeTargetHolder
{
public:

	static Identifier getSanitizedParameterId(const String& id);

	~HardcodedSwappableEffect() override;

	template <typename T> static ProcessorMetadata withHardcodedMetadata(const ProcessorMetadata& b)
	{
		return b.asDynamic()
			.withId(T::getClassType())
			.withPrettyName(T::getClassName())
			.template withType<T>()
			.template withInterface<T>()
			.withComplexDataInterface(ExternalData::DataType::Table)
			.withComplexDataInterface(ExternalData::DataType::SliderPack)
			.withComplexDataInterface(ExternalData::DataType::AudioFile)
			.withComplexDataInterface(ExternalData::DataType::DisplayBuffer);
	}

	// ===================================================================================== Complex Data API calls

	int getNumDataObjects(ExternalData::DataType t) const override;
	Table* getTable(int index) override { return getOrCreate<Table>(tables, index); }
	SliderPackData* getSliderPack(int index) override { return getOrCreate<SliderPackData>(sliderPacks, index); }
	MultiChannelAudioBuffer* getAudioFile(int index) override { return getOrCreate<MultiChannelAudioBuffer>(audioFiles, index); }
	FilterDataObject* getFilterData(int index) override { return getOrCreate<FilterDataObject>(filterData, index);; }
	SimpleRingBuffer* getDisplayBuffer(int index) override { return getOrCreate<SimpleRingBuffer>(displayBuffers, index); }

	LambdaBroadcaster<String> errorBroadcaster;

#if USE_BACKEND
	static void onDllReload(HardcodedSwappableEffect& fx, const std::pair<scriptnode::dll::ProjectDll*, scriptnode::dll::ProjectDll*>& update);
#endif

	// ===================================================================================== Custom hardcoded API calls

	virtual const ModulatorChain::Collection* getModulationChains() const { return nullptr; }
	virtual ModulatorChain::Collection* getModulationChains() { return nullptr; }

	Processor* getCurrentEffect() { return dynamic_cast<Processor*>(this); }
	const Processor* getCurrentEffect() const override { return dynamic_cast<const Processor*>(this); }

	StringArray getModuleList() const override;
	bool setEffect(const String& factoryId, bool /*unused*/) override;
	bool swap(HotswappableProcessor* other) override;
	bool isPolyphonic() const { return polyHandler.isEnabled(); }

	virtual StringArray getIllegalParameterIds() const;

	virtual int getParameterOffset() const { return 0; }

    String getCurrentEffectId() const override { return currentEffect; }
    
	Processor& asProcessor() { return *dynamic_cast<Processor*>(this); }
	const Processor& asProcessor() const { return *dynamic_cast<const Processor*>(this); }

	// ===================================================================================== Processor API tool functions

	/** Call this in every derived destructor. */
	void shutdown()
	{
		mc_->removeTempoListener(&tempoSyncer);
		shutdownCalled = true;
		effectUpdater.shutdown();

		listeners.clear();
		disconnectRuntimeTargets(&asProcessor());

		if(opaqueNode != nullptr)
		{
			factory->deinitOpaqueNode(opaqueNode);
			opaqueNode = nullptr;
		}

		tables.clear();
		audioFiles.clear();
		filterData.clear();
		sliderPacks.clear();
		modProperties.reset();
	}

	Result sanityCheck();

	void setHardcodedAttribute(int index, float newValue);
	float getHardcodedAttribute(int index) const;
	Path getHardcodedSymbol() const;
	ProcessorEditorBody* createHardcodedEditor(ProcessorEditor* parent);
	void restoreHardcodedData(const ValueTree& v);
	ValueTree writeHardcodedData(ValueTree& v) const;

	virtual bool checkHardcodedChannelCount();
	bool processHardcoded(AudioSampleBuffer& b, HiseEventBuffer* e, int startSample, int numSamples);
	virtual void renderData(ProcessDataDyn& data);
	bool hasHardcodedTail() const;
    var getParameterProperties() const override;

	ProcessorMetadata withDynamicMetadata(const ProcessorMetadata& pd, int numMods, int modOffset) const
	{
		auto mb = pd;

		if (opaqueNode != nullptr)
		{
			for (parameter::data& p : OpaqueNode::ParameterIterator(*opaqueNode))
				mb = mb.withDynamicParameter(p);
		}

		if(numMods != -1)
			return mb.withDynamicModulation(modProperties, numMods, modOffset);
		return
			mb;
	}

	virtual int getExtraModulationIndex(int modulationSlotIndexWithoutOffset) const
	{
		return modulationSlotIndexWithoutOffset;
	}

	void setupChannelData(float** data, AudioSampleBuffer& b, int startSample)
	{
		for (int i = 0; i < numChannelsToRender; i++)
		{
			data[i] = b.getWritePointer(channelIndexes[i], startSample);
		}
	}

    void disconnectRuntimeTargets(Processor* p) override;
    void connectRuntimeTargets(Processor* p) override;

#if USE_BACKEND
	void preallocateUnloadedParameters(Array<Identifier> unloadedParameters_);
#endif

	ModulatorChain::ExtraModulatorRuntimeTargetSource::ParameterInitData getParameterInitData(int pIndex);

	String getOpaqueNodeParameterId(int opaqueNodeParameterIndex) const
	{
		if (opaqueNode != nullptr)
		{
			if (auto p = opaqueNode->getParameter(opaqueNodeParameterIndex))
				return p->info.getId();
		}

		return "";
	}

protected:

	ModulatorChain* getPitchChain()
	{
		if(auto synth = dynamic_cast<ModulatorSynth*>(&asProcessor()))
		{
			auto mc = synth->getChildProcessor(ModulatorSynth::InternalChains::PitchModulation);
			return dynamic_cast<ModulatorChain*>(mc);
		}

		auto pp = asProcessor().getParentProcessor(true);

		if(pp != nullptr)
		{
			auto mc = pp->getChildProcessor(ModulatorSynth::InternalChains::PitchModulation);
			return dynamic_cast<ModulatorChain*>(mc);
		}

		return nullptr;
	}

	bool hasLoadedButUncompiledEffect() const;

	HardcodedSwappableEffect(MainController* mc, bool isPolyphonic);

	ModulatorChain::ModChainWithBuffer* getModulationChainForParameter(int parameterIndex) const;

	struct DataWithListener : public ComplexDataUIUpdaterBase::EventListener
	{
		DataWithListener(HardcodedSwappableEffect& parent, ComplexDataUIBase* p, int index_, OpaqueNode* nodeToInitialise);;
		~DataWithListener() override;

		void updateData();
		void onComplexDataEvent(ComplexDataUIUpdaterBase::EventType t, var newValue) override;

		OpaqueNode* node;
		const int index = 0;
		ComplexDataUIBase::Ptr data;
	};

	float* getParameterPtr(int index) const;
	virtual Result prepareOpaqueNode(OpaqueNode* n);

	template <typename T> T* getOrCreate(ReferenceCountedArray<T>& list, int index)
	{
		if (isPositiveAndBelow(index, list.size()))
			return list[index].get();

		auto t = createAndInit(ExternalData::getDataTypeForClass<T>());
		list.add(dynamic_cast<T*>(t));
		return list.getLast().get();
	}


	OwnedArray<DataWithListener> listeners;
	LambdaBroadcaster<String, int, bool> effectUpdater;
	
	ReferenceCountedArray<Table> tables;
	ReferenceCountedArray<SliderPackData> sliderPacks;
	ReferenceCountedArray<MultiChannelAudioBuffer> audioFiles;
	ReferenceCountedArray<SimpleRingBuffer> displayBuffers;
	ReferenceCountedArray<FilterDataObject> filterData;

#if USE_BACKEND
	ValueTree previouslySavedTree;
#endif

	String currentEffect = "No network";

	bool shutdownCalled = false;

	int numParameters = 0;
	ObjectStorage<sizeof(float) * OpaqueNode::NumMaxParameters, 8> lastParameters;
	
	snex::Types::ModValue modValue;
	snex::Types::DllBoundaryTempoSyncer tempoSyncer;
	scriptnode::PolyHandler polyHandler;
	mutable SimpleReadWriteLock lock;
	ScopedPointer<scriptnode::OpaqueNode> opaqueNode;
	ScopedPointer<scriptnode::dll::FactoryBase> factory;

	bool channelCountMatches = false;
	int channelIndexes[NUM_MAX_CHANNELS];
	int numChannelsToRender = 0;
	int hash = -1;

    Array<scriptnode::InvertableParameterRange> parameterRanges;
	Array<Identifier> unloadedParameters;

	OpaqueNode::ModulationProperties modProperties;

private:

	bool disconnected = false;
	MainController* mc_;
	friend class HardcodedMasterEditor;

	JUCE_DECLARE_WEAK_REFERENCEABLE(HardcodedSwappableEffect);
};

}
