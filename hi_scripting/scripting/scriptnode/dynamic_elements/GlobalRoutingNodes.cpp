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

scriptnode::routing::GlobalRoutingManager::Ptr GlobalRoutingManager::Helpers::getOrCreate(MainController* mc)
{
	GlobalRoutingManager::Ptr newP = dynamic_cast<GlobalRoutingManager*>(mc->getGlobalRoutingManager());

	if(newP == nullptr)
	{
		newP = new GlobalRoutingManager();
		mc->setGlobalRoutingManager(newP.get());
		mc->getProcessorChangeHandler().sendProcessorChangeMessage(mc->getMainSynthChain(), MainController::ProcessorChangeHandler::EventType::RebuildModuleList, false);
	}

	return newP;
}



juce::Colour GlobalRoutingManager::Helpers::getColourFromId(const String& id)
{
	auto h = static_cast<uint32>(id.hashCode());
	return Colour(h).withSaturation(0.6f).withAlpha(1.0f).withBrightness(0.7f);
}

void GlobalRoutingManager::Helpers::addGotoTargetCallback(Button* b, SlotBase* slot)
{
#if USE_BACKEND
	b->onClick = [b, slot]()
	{
		PopupLookAndFeel plaf;
		PopupMenu m;
		m.setLookAndFeel(&plaf);

		int i = 1;

		m.addSectionHeader("Goto target");

		auto l = slot->getTargetList();

		for (auto t : l)
		{
			if (auto c = dynamic_cast<GlobalRoutingManager::CableTargetBase*>(t.get()))
			{
				auto dp = new DrawablePath();
				dp->setPath(c->getTargetIcon());
				m.addItem(i++, t->getTargetId(), true, false, std::unique_ptr<Drawable>(dp));
			}
			else
			{
				m.addItem(i++, t->getTargetId());
			}
		}

		if (auto result = m.showAt(b))
		{
			if (auto selectedTarget = l[result - 1])
			{
				auto brw = GET_BACKEND_ROOT_WINDOW(b);
				selectedTarget->selectCallback(brw);
			}
		}
	};
#endif
}

juce::Path GlobalRoutingManager::RoutingIcons::createPath(const String& url) const
{
	Path p;

#if USE_BACKEND
	LOAD_PATH_IF_URL("new", SampleMapIcons::newSampleMap);
	LOAD_PATH_IF_URL("debug", BackendBinaryData::ToolbarIcons::viewPanel);
	LOAD_PATH_IF_URL("goto", ColumnIcons::openWorkspaceIcon);
	LOAD_PATH_IF_URL("global", HiBinaryData::SpecialSymbols::globalCableIcon);
#endif

	return p;
}



struct GlobalRoutingManager::DebugComponent : public Component,
											  public EditorBase,
											  public PooledUIUpdater::SimpleTimer,
											  public ControlledObject
{
	DebugComponent(MainController* mc) :
		EditorBase(dynamic_cast<GlobalRoutingManager*>(mc->getGlobalRoutingManager())),
		SimpleTimer(mc->getGlobalUIUpdater()),
		ControlledObject(mc)
	{
		setName("Global Routing Viewer");

		setSize(500, 300);
		manager->listUpdater.addListener(*this, DebugComponent::listUpdated, true);
	};

	~DebugComponent()
	{
		manager->listUpdater.removeListener(*this);
	}

	struct Item : public Component
	{
		static constexpr int Height = 28;

		Item(SlotBase::Ptr b) :
			gotoButton("goto", nullptr, factory),
			slot(b)
		{
			addAndMakeVisible(gotoButton);

			Helpers::addGotoTargetCallback(&gotoButton, slot.get());
		}

		virtual ~Item() {};

		void drawLed(Graphics& g)
		{
			auto id = slot->id;
			auto numConnections = slot->getTargetList().size();

			auto b = getLocalBounds().toFloat();
			auto led = b.removeFromLeft(b.getHeight()).reduced(7.0f);

			g.setColour(Helpers::getColourFromId(id));
			g.drawEllipse(led, 1.0f);
			g.fillEllipse(led.reduced(3.0f));

			String s;

			s << id << " ";

			if (numConnections > 1)
				s << "(" << String(numConnections) << " connections)";
			else if (numConnections == 1)
				s << "(1 connection)";
			else
				s << "(no connection)";

			g.drawText(s, b, Justification::left);
		}

		void resized() override
		{
			auto b = getLocalBounds();
			gotoButton.setBounds(b.removeFromRight(b.getHeight()).reduced(5));
		}

		SlotBase::Ptr slot;
		RoutingIcons factory;
		HiseShapeButton gotoButton;
	};

	struct SignalItem : public Item
	{
		SignalItem(SlotBase::Ptr p) :
			Item(p)
		{};

		void paint(Graphics& g) override
		{
			auto b = getLocalBounds().toFloat();

			ScriptnodeComboBoxLookAndFeel::drawScriptnodeDarkBackground(g, b.reduced(1.0f), true);

			b.removeFromRight(b.getHeight());

			drawLed(g);
		}

		Signal* getSignal() { return static_cast<Signal*>(slot.get()); }
	};

	struct CableItem : public Item
	{
		CableItem(SlotBase::Ptr c) : Item(c) {};

		void paint(Graphics& g) override
		{
			auto b = getLocalBounds().toFloat();

			ScriptnodeComboBoxLookAndFeel::drawScriptnodeDarkBackground(g, b.reduced(1.0f), true);
			g.setFont(GLOBAL_BOLD_FONT());

			auto c = getCable();

			drawLed(g);

			b = b.reduced(b.getHeight(), 0.0f).toFloat();

			auto valueArea = b.removeFromRight(100.0f).reduced(9.0f);
			g.setColour(Colours::white.withAlpha(0.8f));
			g.drawRoundedRectangle(valueArea, valueArea.getHeight() / 2.0f, 1.0f);

			valueArea = valueArea.reduced(3.0f);
			valueArea = valueArea.removeFromLeft(jmax<float>(valueArea.getHeight(), valueArea.getWidth() * c->lastValue));

			g.fillRoundedRectangle(valueArea, valueArea.getHeight() / 2.0f);
		}

		Cable* getCable() { return static_cast<Cable*>(slot.get()); }
	};

	static void listUpdated(DebugComponent& dc, SlotBase::SlotType type, IdList newList)
	{
		if (type == SlotBase::SlotType::Cable)
		{
			dc.cableItems.clear();

			for (auto s : newList)
			{
				auto c = dc.manager->getSlotBase(s, type);
				auto ci = new CableItem(c);
				dc.addAndMakeVisible(ci);
				dc.cableItems.add(ci);
			}
		}
		else
		{
			dc.signalItems.clear();

			for (auto s : newList)
			{
				auto c = dc.manager->getSlotBase(s, type);
				auto si = new SignalItem(c);
				dc.addAndMakeVisible(si);
				dc.signalItems.add(si);
			}
		}

		dc.resized();
	}

	void timerCallback() override
	{
		repaint();
	}

	void resized() override
	{
		auto b = getLocalBounds();

		if (!cableItems.isEmpty())
		{
			cableTitle = b.removeFromTop(24).toFloat();

			for (auto ci : cableItems)
				ci->setBounds(b.removeFromTop(Item::Height));
		}

		if (!signalItems.isEmpty())
		{
			signalTitle = b.removeFromTop(24).toFloat();

			for (auto si : signalItems)
				si->setBounds(b.removeFromTop(Item::Height));
		}
	}

	void paint(Graphics& g) override
	{
		if (!cableItems.isEmpty())
		{
			g.setFont(GLOBAL_BOLD_FONT());
			g.setColour(Colours::white.withAlpha(0.6f));
			g.drawText("Global Cables", cableTitle, Justification::centred);
		}

		if (!signalItems.isEmpty())
		{
			g.setFont(GLOBAL_BOLD_FONT());
			g.setColour(Colours::white.withAlpha(0.6f));
			g.drawText("Global Signal Slots", signalTitle, Justification::centred);
		}
	}

	OwnedArray<SignalItem> signalItems;
	OwnedArray<CableItem> cableItems;

	Rectangle<float> cableTitle, signalTitle;

	JUCE_DECLARE_WEAK_REFERENCEABLE(DebugComponent);
};

Component* GlobalRoutingManager::Helpers::createDebugViewer(MainController* mc)
{
	auto m = getOrCreate(mc);

	return new DebugComponent(mc);
}

void GlobalRoutingManager::removeUnconnectedSlots(SlotBase::SlotType type)
{
	bool didSomething = false;

	auto isCable = type == SlotBase::SlotType::Cable;

	if (isCable)
	{
		for (int i = 0; i < cables.size(); i++)
		{
			if (cables[i]->cleanup())
			{
				cables.remove(i--);
				didSomething = true;
			}
		}
	}
	else
	{
		for (int i = 0; i < signals.size(); i++)
		{
			if (signals[i]->cleanup())
			{
				signals.remove(i--);
				didSomething = true;
			}
		}
	}

	if (didSomething)
		listUpdater.sendMessage(sendNotificationSync, type, getIdList(type));
}

juce::ReferenceCountedObjectPtr<scriptnode::routing::GlobalRoutingManager::SlotBase> GlobalRoutingManager::getSlotBase(const String& id, SlotBase::SlotType t)
{
	auto isCable = t == SlotBase::SlotType::Cable;

	auto& lToUse = isCable ? cables : signals;

	for (auto c : lToUse)
	{
		if (c->id == id)
			return c;
	}

	ReferenceCountedObjectPtr<SlotBase> newSlot;

	if (isCable)
	{
		newSlot = new Cable(id);
	}
	else
		newSlot = new Signal(id);

	lToUse.add(newSlot);
	listUpdater.sendMessage(sendNotificationSync, t, getIdList(t));

	return newSlot;
}

template <class NodeType> 
	struct SlotBaseEditor : public ScriptnodeExtraComponent<NodeType>,
						    public GlobalRoutingManager::EditorBase,
							public Value::Listener,
							public AsyncUpdater
{
	static constexpr auto SlotTypeId = NodeType::SlotTypeId;

	static constexpr int EditorHeight = NodeType::EditorHeight;

	SlotBaseEditor(NodeType* p) :
		ScriptnodeExtraComponent<NodeType>(p, p->getScriptProcessor()->getMainController_()->getGlobalUIUpdater()),
		EditorBase(p->globalRoutingManager),
		dbgButton("debug", nullptr, *this),
		newButton("new", nullptr, *this)
	{
		this->addAndMakeVisible(newButton);
		this->addAndMakeVisible(slotSelector);
		this->addAndMakeVisible(dbgButton);
		slotSelector.setLookAndFeel(&claf);
		slotSelector.setTextWhenNoChoicesAvailable("No Slots available");
		slotSelector.setTextWhenNothingSelected("No slot selected");
		slotSelector.setColour(ComboBox::ColourIds::textColourId, Colours::white);
		slotSelector.setColour(HiseColourScheme::ColourIds::ComponentTextColourId, Colours::white);

		peakMeter.setInterceptsMouseClicks(false, false);
		peakMeter.setForceLinear(true);
		peakMeter.setColour(VuMeter::backgroundColour, Colours::transparentBlack);
		peakMeter.setOpaque(false);
		peakMeter.setColour(VuMeter::ColourId::ledColour, JUCE_LIVE_CONSTANT_OFF(Colour(0xFFAAAAAA)));
		this->addAndMakeVisible(peakMeter);

		this->getObject()->globalRoutingManager->listUpdater.addListener(*this, listUpdated);

		v.referTo(this->getObject()->getNodePropertyAsValue(PropertyIds::Connection));
		v.addListener(this);
		valueChanged(v);

		

		slotSelector.onChange = [this]()
		{
			v.setValue(slotSelector.getText());
		};

		newButton.onClick = [this]()
		{
			auto newValue = PresetHandler::getCustomName("Slot");
			v.setValue(newValue);
		};

		dbgButton.onClick = [this]()
		{
#if USE_BACKEND
			auto root = GET_BACKEND_ROOT_WINDOW(this)->getRootFloatingTile();

			auto c = GlobalRoutingManager::Helpers::createDebugViewer(root->getMainController());
			root->showComponentAsDetachedPopup(c, &dbgButton, { 10, 10 }, false);
#endif
		};

		this->setSize(256, EditorHeight);
		this->start();

		parentUpdater.setCallback(p->getValueTree(), valuetree::AsyncMode::Asynchronously, BIND_MEMBER_FUNCTION_0(SlotBaseEditor::rebuildSlotList));
		rebuildSlotList();
	}

	void handleAsyncUpdate() override
	{
		rebuildSlotList();
	}

	virtual bool slotMatches(GlobalRoutingManager::SlotBase::Ptr b) const
	{
		return true;
	}

	void rebuildSlotList()
	{
		slotSelector.clear(dontSendNotification);

		auto rm = this->getObject()->globalRoutingManager;
		auto list = rm->getIdList(SlotTypeId);
		int id = 1;
		auto currentName = v.toString();

		slotSelector.setSelectedId(0, dontSendNotification);

		for (auto l : list)
		{
			if (slotMatches(rm->getSlotBase(l, SlotTypeId)))
			{
				slotSelector.addItem(l, id++);

				if (currentName == l)
					slotSelector.setText(l, dontSendNotification);
			}
		}
	}

	static void listUpdated(SlotBaseEditor& e, GlobalRoutingManager::SlotBase::SlotType type, GlobalRoutingManager::IdList list)
	{
		if (type == SlotTypeId)
			e.triggerAsyncUpdate();
	}

	void valueChanged(Value& value) override
	{
		slotSelector.setText(v.toString(), dontSendNotification);
	}

	Rectangle<float> ledBounds;
	Value v;
	scriptnode::ScriptnodeComboBoxLookAndFeel claf;
	ComboBox slotSelector;
	HiseShapeButton newButton, dbgButton;

	VuMeter peakMeter;

	valuetree::ParentListener parentUpdater;

	JUCE_DECLARE_WEAK_REFERENCEABLE(SlotBaseEditor);
};

struct GlobalRoutingNodeBase::Editor : public SlotBaseEditor<GlobalRoutingNodeBase>
{
	Editor(GlobalRoutingNodeBase* b) :
		SlotBaseEditor(b)
	{
		
	};

	void valueChanged(Value& v) override
	{
		SlotBaseEditor::valueChanged(v);

		if (auto c = getObject()->currentSlot)
		{
			auto isMono = c->sourceSpecs.numChannels == 1;

			peakMeter.setType(isMono ? VuMeter::MonoHorizontal : VuMeter::StereoHorizontal);
			peakMeter.setVisible(c->isConnected());
		}
	}

	virtual bool slotMatches(GlobalRoutingManager::SlotBase::Ptr b) const override
	{
		if (getObject()->isSource())
			return true;

		auto sig = dynamic_cast<GlobalRoutingManager::Signal*>(b.get());
		return sig->matchesSourceSpecs(getObject()->lastSpecs).error == Error::OK;
	}

	void timerCallback() override
	{
		SimpleReadWriteLock::ScopedReadLock sl(getObject()->connectionLock);

		if (auto s = getObject()->currentSlot)
		{
			auto l = s->signalPeaks[0];
			auto r = s->signalPeaks[1];

			auto g = getObject()->getGain();

			l *= g;
			r *= g;

			peakMeter.setPeak(l, r);
		}

		repaint();
	}

	void paint(Graphics& g) override
	{
		SimpleReadWriteLock::ScopedReadLock sl(getObject()->connectionLock);

		auto r = getObject()->lastResult;

		auto slot = getObject()->currentSlot;

		auto b = getLocalBounds().toFloat();
		b.removeFromTop(32.0f);
		b = b.reduced(10.0f);
		ScriptnodeComboBoxLookAndFeel::drawScriptnodeDarkBackground(g, b.expanded(5.0f), false);

		b.removeFromTop(30.0f);

		String s;

		if (r.wasOk())
		{
			auto ok = getObject()->currentSlot != nullptr && getObject()->currentSlot->isConnected();

			if (ok)
			{
				if (getObject()->isSource())
				{
					if (auto sl = getObject()->currentSlot)
					{
						int numTargets = sl->targetNodes.size();

						if (numTargets == 1)
							s << "Connected to `" << sl->targetNodes.getFirst()->getId() << "`";
						else
							s << "Connected to " << String(numTargets) << " targets";
					}
				}
				else
					s << "Connected to `" << getObject()->currentSlot->sendNode->getId() << "`";
			}
		}
		else
		{
			s = r.getErrorMessage();
		}

		g.setFont(GLOBAL_BOLD_FONT());

		MarkdownRenderer mp(s);

		mp.getStyleData().fontSize = 13.0f;
		mp.parse();
		mp.getHeightForWidth(b.getWidth());
		mp.draw(g, b.translated(0.0f, -10.0f));
	}

	void resized() override
	{
		auto b = getLocalBounds();

		auto top = b.removeFromTop(32);

		peakMeter.setBounds(b.removeFromTop(30).reduced(8));

		if (getObject()->isSource())
			newButton.setBounds(top.removeFromLeft(top.getHeight()).reduced(5));
		else
			newButton.setVisible(false);

		dbgButton.setBounds(top.removeFromRight(top.getHeight()).reduced(5));

		slotSelector.setBounds(top.reduced(2));
	}

	
};

GlobalRoutingNodeBase::GlobalRoutingNodeBase(DspNetwork* n, ValueTree d) :
	NodeBase(n, d, 0),
	slotId(PropertyIds::Connection, ""),
	lastResult(Result::ok())
{
	globalRoutingManager = GlobalRoutingManager::Helpers::getOrCreate(n->getScriptProcessor()->getMainController_());

	slotId.initialise(this);
}

String GlobalRoutingNodeBase::getTargetId() const
{
	String n;
	n << getRootNetwork()->getId() << "." << getId() << " (Node)";
	return n;
}

void GlobalRoutingNodeBase::selectCallback(Component* rootEditor)
{
#if USE_BACKEND
	auto rootWindow = dynamic_cast<BackendRootWindow*>(rootEditor);

	rootWindow->gotoIfWorkspace(dynamic_cast<Processor*>(getScriptProcessor()));

	SafeAsyncCall::call<GlobalRoutingNodeBase>(*this, [rootEditor](GlobalRoutingNodeBase& n)
	{
		Component::callRecursive<DspNetworkGraph>(rootEditor, [&n](DspNetworkGraph* g)
			{
				Timer::callAfterDelay(200, [g, &n]()
					{
						DspNetworkGraph::Actions::selectAndScrollToNode(*g, &n);
					});

				return true;
			});
	});
#endif
}

void GlobalRoutingNodeBase::updateConnection(Identifier id, var newValue)
{
	{
		SimpleReadWriteLock::ScopedWriteLock sl(connectionLock);

		auto s = newValue.toString();

		auto c = GlobalRoutingManager::Helpers::getColourFromId(s);
		setValueTreeProperty(PropertyIds::NodeColour, (int64)c.getARGB());

		if (currentSlot != nullptr)
		{
			currentSlot->setConnection(this, false, {}, isSource());
		}

		if (s.isEmpty())
		{
			currentSlot = nullptr;
			lastResult = Result::fail("Unconnected");
		}
		else
		{
			currentSlot = dynamic_cast<GlobalRoutingManager::Signal*>(globalRoutingManager->getSlotBase(s, SlotTypeId).get());
			lastResult = currentSlot->setConnection(this, true, lastSpecs, isSource());
		}
	}

	globalRoutingManager->removeUnconnectedSlots(SlotTypeId);
}

void GlobalRoutingNodeBase::initParameters()
{
	auto d = getValueTree();

	d.getOrCreateChildWithName(PropertyIds::Parameters, getUndoManager());

	auto pData = createInternalParameterList();

	for (auto p : pData)
	{
		auto existingChild = getParameterTree().getChildWithProperty(PropertyIds::ID, p.info.getId());

		if (!existingChild.isValid())
		{
			existingChild = p.createValueTree();
			getParameterTree().addChild(existingChild, -1, getUndoManager());
		}

		auto newP = new Parameter(this, existingChild);

		auto ndb = new parameter::dynamic_base(p.callback);

		newP->setDynamicParameter(ndb);
		newP->valueNames = p.parameterNames;

		addParameter(newP);
	}
}

void GlobalRoutingNodeBase::prepare(PrepareSpecs specs)
{
	DspHelpers::throwIfFrame(specs);

	lastSpecs = specs;

	SimpleReadWriteLock::ScopedReadLock sl(connectionLock);

	if (currentSlot != nullptr)
		lastResult = currentSlot->setConnection(this, true, lastSpecs, isSource());
}

juce::Rectangle<int> GlobalRoutingNodeBase::getPositionInCanvas(Point<int> topLeft) const
{
	return Rectangle<int>(topLeft.getX(), topLeft.getY(), 256, UIValues::HeaderHeight + UIValues::ParameterHeight + UIValues::NodeMargin + Editor::EditorHeight);
}

scriptnode::NodeComponent* GlobalRoutingNodeBase::createComponent()
{
	auto nc = ComponentHelpers::createDefaultComponent(this);

	ComponentHelpers::addExtraComponentToDefault(nc, new Editor(this));
	return nc;
}

#if 0
struct GlobalCableNode::Editor : public ScriptnodeExtraComponent<NodeBase>,
								 public GlobalRoutingManager::EditorBase,
								 public Value::Listener,
								 public AsyncUpdater
{
	static constexpr int EditorHeight = 32 + UIValues::NodeMargin + 34 + UIValues::NodeMargin + 28;

	Editor(GlobalCableNode* p) :
		ScriptnodeExtraComponent<GlobalCableNode>(p, p->getScriptProcessor()->getMainController_()->getGlobalUIUpdater()),
		EditorBase(p->globalRoutingManager),
		dragger(p->getScriptProcessor()->getMainController_()->getGlobalUIUpdater()),
		newButton("new", nullptr, *this),
		dbgButton("debug", nullptr, *this)
	{
		addAndMakeVisible(dragger);
		addAndMakeVisible(newButton);
		addAndMakeVisible(dbgButton);
		addAndMakeVisible(slotSelector);
		slotSelector.setLookAndFeel(&claf);
		slotSelector.setTextWhenNoChoicesAvailable("No Cables available");
		slotSelector.setTextWhenNothingSelected("No cable selected");
		slotSelector.setColour(ComboBox::ColourIds::textColourId, Colours::white);
		slotSelector.setColour(HiseColourScheme::ColourIds::ComponentTextColourId, Colours::white);

		peakMeter.setType(VuMeter::MonoHorizontal);
		peakMeter.setInterceptsMouseClicks(false, false);
		peakMeter.setForceLinear(true);
		peakMeter.setColour(VuMeter::backgroundColour, JUCE_LIVE_CONSTANT_OFF(Colour(0xff383838)));
		peakMeter.setColour(VuMeter::ColourId::ledColour, JUCE_LIVE_CONSTANT_OFF(Colour(0xFFAAAAAA)));

		getObject()->globalRoutingManager->listUpdater.addListener(*this, listUpdated);

		v.referTo(getObject()->getNodePropertyAsValue(PropertyIds::Connection));
		v.addListener(this);
		valueChanged(v);

		addAndMakeVisible(peakMeter);

		slotSelector.onChange = [this]()
		{
			v.setValue(slotSelector.getText());
		};

		newButton.onClick = [this]()
		{
			auto newValue = PresetHandler::getCustomName("Cable");
			v.setValue(newValue);
		};

		dbgButton.onClick = [this]()
		{
			auto root = GET_BACKEND_ROOT_WINDOW(this)->getRootFloatingTile();

			auto c = new GlobalRoutingManager::DebugComponent(root->getMainController());
			root->showComponentAsDetachedPopup(c, &dbgButton, { 10, 10 }, false);
		};

		setSize(200, EditorHeight);
		start();
		rebuildSlotList();
	}

	void handleAsyncUpdate() override
	{
		rebuildSlotList();
	}

	void rebuildSlotList()
	{
		slotSelector.clear(dontSendNotification);
		auto rm = getObject()->globalRoutingManager;
		auto currentName = v.toString();

		int i = 1;

		for (auto s : rm->getIdList(slotType))
			slotSelector.addItemList(s, i++);

		slotSelector.setText(currentName, dontSendNotification);
	}

	static void listUpdated(Editor& e, GlobalRoutingManager::SlotBase::SlotType type, GlobalRoutingManager::IdList list)
	{
		if (type == slotType)
			e.triggerAsyncUpdate();
	}

	void valueChanged(Value& value) override
	{
		slotSelector.setText(v.toString(), dontSendNotification);
	}

	void resized() override
	{
		auto b = getLocalBounds();

		auto top = b.removeFromTop(32);

		b.removeFromTop(UIValues::NodeMargin);

		peakMeter.setBounds(b.removeFromTop(34).reduced(UIValues::NodeMargin));

		newButton.setBounds(top.removeFromLeft(top.getHeight()).reduced(5));
		dbgButton.setBounds(top.removeFromRight(top.getHeight()).reduced(5));
		slotSelector.setBounds(top.reduced(2));
		b.removeFromTop(UIValues::NodeMargin);

		dragger.setBounds(b);
	}

	void timerCallback() override
	{
		SimpleReadWriteLock::ScopedReadLock sl(getObject()->connectionLock);

		if (auto nc = findParentComponentOfClass<NodeComponent>())
		{
			auto shouldBeEnabled = getObject()->getParameterHolder()->base == nullptr;

			Component::callRecursive<ParameterSlider>(nc, [shouldBeEnabled](ParameterSlider* s)
				{
					s->setVisible(shouldBeEnabled);
					return true;
				});
		}

		if (auto s = getObject()->currentCable)
		{
			peakMeter.setPeak(s->lastValue, 0.0f);
		}

		repaint();
	}

	void paint(Graphics& g) override
	{
		ScriptnodeComboBoxLookAndFeel::drawScriptnodeDarkBackground(g, peakMeter.getBounds().toFloat().expanded(5.0f), false);
	}

	Rectangle<float> ledBounds;
	Value v;
	scriptnode::ScriptnodeComboBoxLookAndFeel claf;
	ComboBox slotSelector;
	HiseShapeButton newButton;
	HiseShapeButton dbgButton;

	valuetree::ParentListener parentUpdater;

	VuMeter peakMeter;

	ModulationSourceBaseComponent dragger;

	JUCE_DECLARE_WEAK_REFERENCEABLE(Editor);
};
#endif

struct GlobalCableNode::Editor : public SlotBaseEditor<GlobalCableNode>
{
	Editor(GlobalCableNode* ed) :
		SlotBaseEditor(ed),
		dragger(ed->getScriptProcessor()->getMainController_()->getGlobalUIUpdater())
	{
		peakMeter.setType(VuMeter::MonoHorizontal);
		addAndMakeVisible(dragger);
	};

	void paint(Graphics& g) override
	{
		ScriptnodeComboBoxLookAndFeel::drawScriptnodeDarkBackground(g, peakMeter.getBounds().toFloat().expanded(5.0f), false);
	}

	void timerCallback() override
	{
		SimpleReadWriteLock::ScopedReadLock sl(getObject()->connectionLock);

		if (auto nc = findParentComponentOfClass<NodeComponent>())
		{
			auto shouldBeEnabled = getObject()->getParameterHolder()->base == nullptr;

			Component::callRecursive<ParameterSlider>(nc, [shouldBeEnabled](ParameterSlider* s)
				{
					s->setVisible(shouldBeEnabled);
					return true;
				});
		}

		if (auto s = getObject()->currentCable)
		{
			peakMeter.setPeak(s->lastValue, 0.0f);
		}

		repaint();
	}

	void resized() override
	{
		auto b = getLocalBounds();

		auto top = b.removeFromTop(32);

		b.removeFromTop(UIValues::NodeMargin);

		peakMeter.setBounds(b.removeFromTop(34).reduced(UIValues::NodeMargin));

		newButton.setBounds(top.removeFromLeft(top.getHeight()).reduced(5));
		dbgButton.setBounds(top.removeFromRight(top.getHeight()).reduced(5));
		slotSelector.setBounds(top.reduced(2));
		b.removeFromTop(UIValues::NodeMargin);

		dragger.setBounds(b);
	}

	ModulationSourceBaseComponent dragger;
};

GlobalCableNode::GlobalCableNode(DspNetwork* n, ValueTree d) :
	ModulationSourceNode(n, d),
	slotId(PropertyIds::Connection, "")
{
	cppgen::CustomNodeProperties::setPropertyForObject(*this, PropertyIds::UncompileableNode);

	globalRoutingManager = GlobalRoutingManager::Helpers::getOrCreate(n->getScriptProcessor()->getMainController_());

	slotId.initialise(this);
	slotId.setAdditionalCallback(BIND_MEMBER_FUNCTION_2(GlobalCableNode::updateConnection), true);

	initParameters();
}

GlobalCableNode::~GlobalCableNode()
{
	if (currentCable != nullptr)
		currentCable->removeTarget(this);

	if (globalRoutingManager != nullptr)
		globalRoutingManager->removeUnconnectedSlots(SlotTypeId);
}

scriptnode::NodeBase* GlobalCableNode::createNode(DspNetwork* n, ValueTree d)
{
	return new GlobalCableNode(n, d);
}

void GlobalCableNode::prepare(PrepareSpecs ps)
{
	ModulationSourceNode::prepare(ps);
}

String GlobalCableNode::getTargetId() const
{
	String n;
	n << getRootNetwork()->getId() << "." << getId() << " (Node)";
	return n;
}

juce::Path GlobalCableNode::getTargetIcon() const
{
	Path p;
	p.loadPathFromData(ScriptnodeIcons::pinIcon, sizeof(ScriptnodeIcons::pinIcon));
	return p;
}

void GlobalCableNode::selectCallback(Component* rootEditor)
{
#if USE_BACKEND
	auto rootWindow = dynamic_cast<BackendRootWindow*>(rootEditor);

	rootWindow->gotoIfWorkspace(dynamic_cast<Processor*>(getScriptProcessor()));

	SafeAsyncCall::call<GlobalCableNode>(*this, [rootEditor](GlobalCableNode& n)
	{
		Component::callRecursive<DspNetworkGraph>(rootEditor, [&n](DspNetworkGraph* g)
			{
				Timer::callAfterDelay(200, [g, &n]()
					{
						DspNetworkGraph::Actions::selectAndScrollToNode(*g, &n);
					});

				return true;
			});
	});
#endif
}

void GlobalCableNode::updateConnection(Identifier id, var newValue)
{
	{
		SimpleReadWriteLock::ScopedWriteLock sl(connectionLock);

		auto s = newValue.toString();

		auto c = GlobalRoutingManager::Helpers::getColourFromId(s);
		setValueTreeProperty(PropertyIds::NodeColour, (int64)c.getARGB());

		if (currentCable != nullptr)
		{
			currentCable->removeTarget(this);
		}

		if (s.isEmpty())
			currentCable = nullptr;
		else
		{
			currentCable = dynamic_cast<GlobalRoutingManager::Cable*>(globalRoutingManager->getSlotBase(s, SlotTypeId).get());

			if (currentCable->targets.isEmpty())
			{
				currentCable->lastValue = lastValue;
			}

			currentCable->addTarget(this);
		}
	}

	globalRoutingManager->removeUnconnectedSlots(SlotTypeId);
}

void GlobalCableNode::initParameters()
{
	auto d = getValueTree();

	d.getOrCreateChildWithName(PropertyIds::Parameters, getUndoManager());

	auto pData = createInternalParameterList();

	for (auto p : pData)
	{
		auto existingChild = getParameterTree().getChildWithProperty(PropertyIds::ID, p.info.getId());

		if (!existingChild.isValid())
		{
			existingChild = p.createValueTree();
			getParameterTree().addChild(existingChild, -1, getUndoManager());
		}

		auto newP = new Parameter(this, existingChild);

		auto ndb = new parameter::dynamic_base(p.callback);

		newP->setDynamicParameter(ndb);
		newP->valueNames = p.parameterNames;

		addParameter(newP);
	}
}

void GlobalCableNode::processFrame(FrameType& data)
{

}

juce::Rectangle<int> GlobalCableNode::getPositionInCanvas(Point<int> topLeft) const
{
	return Rectangle<int>(topLeft.getX(), topLeft.getY(), 256, UIValues::HeaderHeight + UIValues::ParameterHeight + UIValues::NodeMargin + EditorHeight);
}

void GlobalCableNode::sendValue(double v)
{
	if (auto h = getParameterHolder())
		h->call(v);
}

void GlobalCableNode::setValue(void* obj, double newValue)
{
	auto t = static_cast<GlobalCableNode*>(obj);

	t->lastValue = newValue;

	if (auto s = t->currentCable)
	{
		s->sendValue(t, newValue);
	}
}

scriptnode::ParameterDataList GlobalCableNode::createInternalParameterList()
{
	ParameterDataList d;

	{
		parameter::data p("Value");
		p.setRange({ 0.0, 1.0 });
		p.setDefaultValue(1.0);
		p.callback.referTo(this, setValue);
		d.add(p);
	}

	return d;
}

scriptnode::NodeComponent* GlobalCableNode::createComponent()
{
	auto nc = ComponentHelpers::createDefaultComponent(this);

	ComponentHelpers::addExtraComponentToDefault(nc, new Editor(this));
	return nc;
}

GlobalSendNode::GlobalSendNode(DspNetwork* n, ValueTree d) :
	GlobalRoutingNodeBase(n, d)
{
	cppgen::CustomNodeProperties::setPropertyForObject(*this, PropertyIds::UncompileableNode);

	slotId.setAdditionalCallback(BIND_MEMBER_FUNCTION_2(GlobalRoutingNodeBase::updateConnection), true);
	initParameters();
}

void GlobalSendNode::process(ProcessDataDyn& data)
{
	if (auto sl = SimpleReadWriteLock::ScopedTryReadLock(connectionLock))
	{
		if (currentSlot != nullptr && !isBypassed())
			currentSlot->push(data, value);
	}
}

void GlobalSendNode::reset()
{
	if (auto sl = SimpleReadWriteLock::ScopedTryReadLock(connectionLock))
	{
		if (currentSlot != nullptr)
			currentSlot->clearSignal();
	}
}

void GlobalSendNode::setValue(void* obj, double v)
{
	auto typed = static_cast<GlobalSendNode*>(obj);
	typed->value = v;
}

scriptnode::ParameterDataList GlobalSendNode::createInternalParameterList()
{
	ParameterDataList d;

	{
		parameter::data p("Value");
		p.setRange({ 0.0, 1.0 });
		p.setDefaultValue(1.0);
		p.callback.referTo(this, setValue);
		d.add(p);
	}

	return d;
}

template <int NV> struct GlobalReceiveNode : public GlobalRoutingNodeBase
{
	static constexpr int NumVoices = NV;

	GlobalReceiveNode(DspNetwork* n, ValueTree d) :
		GlobalRoutingNodeBase(n, d)
	{
		cppgen::CustomNodeProperties::setPropertyForObject(*this, PropertyIds::UncompileableNode);

		slotId.setAdditionalCallback(BIND_MEMBER_FUNCTION_2(GlobalRoutingNodeBase::updateConnection), true);
		initParameters();
	};

	SN_NODE_ID("global_receive");

	static NodeBase* createNode(DspNetwork* n, ValueTree d)
	{
		return new GlobalReceiveNode(n, d);
	}

	bool isSource() const override { return false; }

	float getGain() const override { return value.getFirst(); }

	void prepare(PrepareSpecs ps) override
	{
		GlobalRoutingNodeBase::prepare(ps);
		value.prepare(ps);
		offset.prepare(ps);

		reset();
	}

	void process(ProcessDataDyn& data) override
	{
		if (auto sl = SimpleReadWriteLock::ScopedTryReadLock(connectionLock))
		{
			if (currentSlot != nullptr && currentSlot->matchesSourceSpecs(lastSpecs).error == Error::OK && !isBypassed())
			{
				auto& o = offset.get();
				o = currentSlot->pop(data, value.get(), o);
			}
		}
	}

	void handleHiseEvent(HiseEvent& e)
	{
		if (e.isNoteOn() && NumVoices > 1)
		{
			auto startStamp = e.getTimeStamp();

			auto ratio = lastSpecs.sampleRate / getRootNetwork()->getMainController()->getMainSynthChain()->getSampleRate();

			startStamp = roundToInt((double)startStamp * ratio);

			offset.get() = startStamp;
		}
	}

	void reset() override
	{
		for (auto& o : offset)
			o = 0;
	}

	String getNodeDescription() const override { return "Send the signal anywhere in HISE!"; }

	static void setValue(void* obj, double nv)
	{
		auto typed = static_cast<GlobalReceiveNode<NumVoices>*>(obj);

		for (auto& v : typed->value)
			v = nv;
	}

	ParameterDataList createInternalParameterList()
	{
		ParameterDataList d;

		{
			parameter::data p("Value");
			p.setRange({ 0.0, 1.0 });
			p.setDefaultValue(1.0);
			p.callback.referTo(this, setValue);
			d.add(p);
		}

		return d;
	};

	PolyData<float, NumVoices> value = 1.0f;
	PolyData<int, NumVoices> offset;
};

GlobalRoutingManager::Cable::Cable(const String& id_) :
	SlotBase(id_, SlotType::Cable)
{

}

bool GlobalRoutingManager::Cable::cleanup()
{
	for (int i = 0; i < targets.size(); i++)
	{
		if (targets[i] == nullptr)
			targets.remove(i--);
	}

	return targets.isEmpty();
}



scriptnode::routing::GlobalRoutingManager::SelectableTargetBase::List GlobalRoutingManager::Cable::getTargetList() const
{
	SelectableTargetBase::List l;

	for (auto t : targets)
		l.add(t.get());

	return l;
}

void GlobalRoutingManager::Cable::addTarget(CableTargetBase* n)
{
	SimpleReadWriteLock::ScopedWriteLock sl(lock);
	targets.addIfNotAlreadyThere(n);
	n->sendValue(lastValue);
}

void GlobalRoutingManager::Cable::removeTarget(CableTargetBase* n)
{
	SimpleReadWriteLock::ScopedWriteLock sl(lock);
	targets.removeAllInstancesOf(n);
}

void GlobalRoutingManager::Cable::sendValue(CableTargetBase* source, double v)
{
	lastValue = jlimit(0.0, 1.0, v);

	for (auto t : targets)
	{
		if (t == source)
			continue;

		t->sendValue(lastValue);
	}
}

GlobalRoutingManager::Signal::Signal(const String& id_) :
	SlotBase(id_, SlotType::Signal),
	sourceSpecs(),
	lastData(channelData.begin(), 0, 0)
{

}

void GlobalRoutingManager::Signal::removeTarget(NodeBase* targetNode)
{
	targetNodes.removeAllInstancesOf(targetNode);
}

scriptnode::routing::GlobalRoutingManager::SelectableTargetBase::List GlobalRoutingManager::Signal::getTargetList() const
{
	SelectableTargetBase::List l;

	for (auto t : targetNodes)
	{
		if (t != nullptr)
			l.add(dynamic_cast<SelectableTargetBase*>(t.get()));
	}

	if (sendNode != nullptr)
		l.add(dynamic_cast<SelectableTargetBase*>(sendNode.get()));

	return l;
}

Error GlobalRoutingManager::Signal::matchesSourceSpecs(PrepareSpecs targetSpecs)
{
	Error e;
	e.error = Error::OK;

	if (sourceSpecs.sampleRate != targetSpecs.sampleRate)
	{
		e.actual = targetSpecs.sampleRate;
		e.expected = sourceSpecs.sampleRate;
		e.error = Error::SampleRateMismatch;
	}

	if (sourceSpecs.numChannels != targetSpecs.numChannels)
	{
		e.error = Error::ChannelMismatch;
		e.actual = targetSpecs.numChannels;
		e.expected = sourceSpecs.numChannels;
	}

	if (sourceSpecs.blockSize < targetSpecs.blockSize)
	{
		e.error = Error::BlockSizeMismatch;
		e.expected = sourceSpecs.blockSize;
		e.actual = targetSpecs.blockSize;
	}

	return e;
}

Result GlobalRoutingManager::Signal::addTarget(NodeBase* targetNode, PrepareSpecs p)
{
	targetNodes.addIfNotAlreadyThere(targetNode);

	if (isConnected())
	{
		auto e = matchesSourceSpecs(p);

		if (e.error != Error::OK)
			return Result::fail(ScriptnodeExceptionHandler::getErrorMessage(e));
		else
			return Result::ok();
	}
	else
		return Result::fail("Unconnected");

	return Result::ok();
}

bool GlobalRoutingManager::Signal::cleanup()
{
	for (int i = 0; i < targetNodes.size(); i++)
	{
		if (targetNodes[i] == nullptr)
		{
			targetNodes.remove(i--);
		}
	}

	return targetNodes.isEmpty() && sendNode == nullptr;
}

void GlobalRoutingManager::Signal::clearSignal()
{
	if (auto sl = SimpleReadWriteLock::ScopedTryReadLock(lock))
	{
		if (sourceSpecs)
		{
			block b(buffer.begin(), sourceSpecs.blockSize * sourceSpecs.numChannels);
			hmath::vset(b, 0.0f);
		}
	}
}

Result GlobalRoutingManager::Signal::setSource(NodeBase* newSendNode, PrepareSpecs p)
{
	if (sendNode != nullptr && newSendNode != nullptr && sendNode != newSendNode)
		return Result::fail("Slot already has a send node");

	{
		SimpleReadWriteLock::ScopedWriteLock sl(lock);

		sendNode = newSendNode;
		sourceSpecs = p;

		if (sourceSpecs)
		{
			DspHelpers::increaseBuffer(buffer, p);

			switch (p.numChannels)
			{
			case 1: channelData = std::move(ProcessDataHelpers<1>::makeChannelData(buffer)); break;
			case 2: channelData = ProcessDataHelpers<2>::makeChannelData(buffer); break;
			case 3: channelData = ProcessDataHelpers<3>::makeChannelData(buffer); break;
			case 4: channelData = ProcessDataHelpers<4>::makeChannelData(buffer); break;
			case 5: channelData = ProcessDataHelpers<5>::makeChannelData(buffer); break;
			case 6: channelData = ProcessDataHelpers<6>::makeChannelData(buffer); break;
			case 8: channelData = ProcessDataHelpers<8>::makeChannelData(buffer); break;
			}
		}
	}

	clearSignal();

	return Result::ok();
}

void GlobalRoutingManager::Signal::push(ProcessDataDyn& data, float value)
{
	if (auto sl = SimpleReadWriteLock::ScopedTryReadLock(lock))
	{
		if (sourceSpecs)
		{
			jassert(isPositiveAndBelow(data.getNumSamples(), sourceSpecs.blockSize + 1));
			for (int i = 0; i < data.getNumChannels(); i++)
			{
				FloatVectorOperations::copyWithMultiply(channelData[i], data[i].begin(), value, data.getNumSamples());
				signalPeaks[i] = FloatVectorOperations::findMaximum(channelData[i], data.getNumSamples());
			}
		}
	}
}

int GlobalRoutingManager::Signal::pop(ProcessDataDyn& data, float value, int offset)
{
	if (auto sl = SimpleReadWriteLock::ScopedTryReadLock(lock))
	{
		if (sourceSpecs)
		{
			if (sourceSpecs.blockSize == data.getNumSamples())
				offset = 0;

			jassert(isPositiveAndBelow(offset + data.getNumSamples(), sourceSpecs.blockSize + 1));

			for (int i = 0; i < data.getNumChannels(); i++)
			{
				FloatVectorOperations::addWithMultiply(data[i].begin(), channelData[i] + offset, value, data.getNumSamples());
			}

			return (offset + data.getNumSamples()) % sourceSpecs.blockSize;
		}
	}

	return 0;
}

Result GlobalRoutingManager::Signal::setConnection(NodeBase* n, bool shouldAdd, PrepareSpecs ps, bool isSource)
{
	if (isSource)
		return setSource(n, ps);
	else
	{
		if (shouldAdd)
			return addTarget(n, ps);
		else
		{
			removeTarget(n);
			return Result::ok();
		}
	}
}

}

}
