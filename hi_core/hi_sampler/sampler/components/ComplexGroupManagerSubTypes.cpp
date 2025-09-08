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

#include "ComplexGroupManagerComponent.h"
#include "ComplexGroupManagerComponent.h"

namespace hise {
using namespace juce;

struct ComplexGroupManagerComponent::LayerComponent::KeyboardComponent: public Component,
																	    public ComponentWithGroupManagerConnection
{
	static constexpr int KeyboardHeight = 48;

	struct PopupMenuHandler
	{
		virtual ~PopupMenuHandler() {};

		virtual void fillPopupMenu(PopupMenu& m, int noteNumber) = 0;

		virtual void onPopupResult(int result, int noteNumber) = 0;

		JUCE_DECLARE_WEAK_REFERENCEABLE(PopupMenuHandler);
	};

	KeyboardComponent(LogicTypeComponent& parent):
	  ComponentWithGroupManagerConnection(parent.getSampler()),
	  layerIndex(ComplexGroupManager::Helpers::getLayerIndex(parent.data)),
	  data(parent.data),
	  mapColour(ComplexGroupManagerComponent::Helpers::getLayerColour(parent.data))
	{
		parent.getSampleEditHandler()->noteBroadcaster.addListener(*this, onNote);
		getComplexGroupManager()->addPlaystateListener(*this, layerIndex, onPlaystateChange);
		rebuildKeyboard();
	}

	void rebuildKeyboard()
	{
		mappedKeys = ComplexGroupManager::Helpers::getKeyRange(data, getSampler());
		groupNoteMap.clear();

		if(mappedKeys.isEmpty())
		{
			octaveRange = {};
			noteRange = {};
		}
		else
		{
			auto low = mappedKeys.getFirstSetBit();
			auto high = (int)mappedKeys.getHighestSetBit();

			auto lowOctave = low - (low % 12);
			auto highOctave = high - (high % 12) + 12;

			octaveRange = { lowOctave / 12, highOctave / 12};
			noteRange = { low, high };

			if(octaveRange.getLength() == 1)
				octaveRange.setLength(2);

			uint8 layerValue = 1;

			for(int i = 0; i < 128; i++)
			{
				if(mappedKeys[i])
					groupNoteMap[layerValue++] = i;
			}
		}

		repaint();
	}

	std::map<uint8, int> groupNoteMap;

	VoiceBitMap<128> playedNotes;
	bool learnActive = false;

	int getHeightToUse() const override { return KeyboardHeight; }

	static void onNote(KeyboardComponent& k, int note, int velo)
	{
		if(k.learnActive)
		{
			k.data.setProperty(SampleIds::LoKey, note, k.getUndoManager());
			k.learnActive = false;
			return;
		}

		k.playedNotes.setBit(note, velo != 0);
		k.repaint();
	}

	static void onPlaystateChange(KeyboardComponent& k, uint8 s)
	{
		k.selected = k.groupNoteMap[s];
		k.repaint();
	}

	int getNoteNumber(int position) const
	{
		auto numOctavesToShow = octaveRange.getLength();
		auto b = getLocalBounds().toFloat();
		auto keyWidth = b.getWidth() / (float)(numOctavesToShow * 12) + 1;

		return roundToInt((float)position / (float)keyWidth) + octaveRange.getStart() * 12;
	}

	void showPopupMenu(int position)
	{
		if(popupMenuHandler != nullptr)
		{
			PopupLookAndFeel plaf;
			PopupMenu m;

			auto noteNumber = getNoteNumber(position);

			popupMenuHandler->fillPopupMenu(m, noteNumber);

			if(auto r = m.show())
			{
				popupMenuHandler->onPopupResult(r, noteNumber);
			}
		}
	}

	void mouseDown(const MouseEvent& e) override
	{
		auto position = e.getPosition().x;

		if(e.mods.isRightButtonDown() || mappedKeys.isEmpty())
		{
			showPopupMenu(position);
		}
		else
		{
			auto noteNumber = getNoteNumber(position);

			if(auto cg = findParentComponentOfClass<ComponentWithGroupManagerConnection>())
			{
				currentlyPlayedNote = noteNumber;
				auto& ks = cg->getSampler()->getMainController()->getKeyboardState();
				ks.noteOn(1, currentlyPlayedNote, 1.0);
			}
		}
	}

	void mouseUp(const MouseEvent& e) override
	{
		if(currentlyPlayedNote != -1)
		{
			if(auto cg = findParentComponentOfClass<ComponentWithGroupManagerConnection>())
			{
				auto& ks = cg->getSampler()->getMainController()->getKeyboardState();
				ks.noteOff(1, currentlyPlayedNote, 0.0f);
				currentlyPlayedNote = -1;
			}
		}
	}

	int currentlyPlayedNote = -1;

	void paint(Graphics& g) override
	{
		auto b = getLocalBounds().toFloat();

		

		if(mappedKeys.isEmpty())
		{
			String m;
			m << "No key range assigned. Click to setup the MIDI note range for this layer";
			g.setColour(Colours::white.withAlpha(0.4f));
			g.setFont(GLOBAL_BOLD_FONT());
			g.drawText(m, b, Justification::centred);
		}

		auto top = b.removeFromTop(18);
		auto numOctavesToShow = octaveRange.getLength();

		auto keyWidth = b.getWidth() / (float)(numOctavesToShow * 12);

		static const bool black[12] = { false, true, false, true, false, false, true, false, true, false, true, false };

		constexpr int Gap = 1;

		for(int i = 0; i < numOctavesToShow; i++)
		{
			auto isLowerOctave = i == 0;
			auto isUpperOctave = i == (numOctavesToShow - 1);

			auto o = top.removeFromLeft(keyWidth * 12);
			g.setColour(Colours::white.withAlpha(0.3f));
			g.setFont(GLOBAL_BOLD_FONT());
			g.drawText("C" + String(i + octaveRange.getStart() - 2), o, Justification::left);

			for(int j = 0; j < 12; j++)
			{
				int noteNumber = (i + octaveRange.getStart()) * 12 + j;

				float alpha = 0.2f;

				if(mappedKeys[noteNumber])
					alpha += 0.5f;

				auto c = (black[j] ? Colour(0xFF555555) : Colours::white);

				if(playedNotes[noteNumber])
					c = Colour(SIGNAL_COLOUR);

				g.setColour(c.withAlpha(alpha));
				
				auto k = b.removeFromLeft(keyWidth - Gap);

				if(noteNumber == selected)
				{
					g.setColour(mapColour);
				}

				if(isLowerOctave && j == 0)
				{
					Path p;
					p.addRoundedRectangle(k.getX(), k.getY(), k.getWidth(), k.getHeight(), 4.0f, 4.0f, true, false, true, false);
					g.fillPath(p);
				}
				else if (isUpperOctave && j == 11)
				{
					Path p;
					p.addRoundedRectangle(k.getX(), k.getY(), k.getWidth(), k.getHeight(), 4.0f, 4.0f, false, true, false, true);
					g.fillPath(p);
				}
				else
				{
					g.fillRect(k);
				}

				

				b.removeFromLeft((float)Gap);
			}
		}
	}

	void setKeyboardProperties(const Array<Identifier>& idsToWatch)
	{
		rebuildListener.setCallback(data, idsToWatch, valuetree::AsyncMode::Asynchronously, [this](const Identifier& id, const var& nv)
		{
			rebuildKeyboard();
		});
	}

	valuetree::PropertyListener rebuildListener;

	Range<int> octaveRange;
	Range<int> noteRange;

	VoiceBitMap<128> mappedKeys;
	Colour mapColour = Colours::red;

	int selected = -1;
	const uint8 layerIndex;
	ValueTree data;

	WeakReference<PopupMenuHandler> popupMenuHandler;

	bool initialised = false;

	JUCE_DECLARE_WEAK_REFERENCEABLE(KeyboardComponent);
};

struct ComplexGroupManagerComponent::RRBody: public LogicTypeComponent::BodyBase
{
	static constexpr int CircleWidth = 24;
	static constexpr int CircleMargin = 24;

	RRBody(LogicTypeComponent& parent):
	  BodyBase(parent)
	{
		uint8 idx = 0;

		for(auto t: tokens)
			addAndMakeVisible(buttons.add(new CircleButton(parent.data, idx++)));
	}

	int getHeightToUse() const override { return jmax(100, height + 10); }

	

	struct CircleButton: public ButtonBase
	{
		CircleButton(const ValueTree& layerData, uint8 rrIndex_):
		  ButtonBase(layerData, rrIndex_ + 1),
		  rrIndex(rrIndex_)
		{
			setSize(getWidthToUse(), CircleWidth);
		}

		int getWidthToUse() const override { return CircleWidth; }

		void paint(Graphics& g) override
		{
			g.setColour(getButtonColour(true));

			auto cb = getLocalBounds().toFloat().reduced(3.0f);

			g.drawEllipse(cb, 2.0f);

			if(active)
				g.fillEllipse(cb.reduced(4.0f));
		}

		const uint8 rrIndex = 0;
	};

	void resized() override
	{
		SimpleFlexbox fb;

		fb.padding = CircleMargin;
		fb.justification = Justification::centred;

		//for(auto b: buttons)
			//b->setSize(CircleWidth, CircleWidth);

		auto buttonList = getAllButtonsToShow();

		auto list = fb.createBoundsListFromComponents(buttonList);

		if(auto newHeight = fb.apply(list, getLocalBounds().removeFromTop(100).reduced(5)))
		{
			height = newHeight;

			fb.applyBoundsListToComponents(buttonList, list);
			
			if(getHeight() != getHeightToUse())
			{
				findParentComponentOfClass<Content>()->updateSize();
			}
		}
		
		BodyBase::resized();
	}

	int height = CircleWidth;

	int getCentreXPositionForToken(uint8 tokenIndex) const override
	{
		if(auto b = buttons[tokenIndex])
			return b->getBoundsInParent().getCentreX();

		return 0;
	}
};

struct ComplexGroupManagerComponent::KeyswitchBody: public LogicTypeComponent::BodyBase,
													public LayerComponent::KeyboardComponent::PopupMenuHandler
{
	KeyswitchBody(LogicTypeComponent& parent):
	  BodyBase(parent),
	  keyboard(parent),
	  editButton("settings", nullptr, f)
	{
		keyboard.mapColour = ComplexGroupManagerComponent::Helpers::getLayerColour(parent.data);
		keyboard.popupMenuHandler = this;

		keyboard.setKeyboardProperties({ SampleIds::LoKey, groupIds::isChromatic, groupIds::tokens });

		addAndMakeVisible(keyboard);
		addAndMakeVisible(editButton);

		editButton.onClick = [this]()
		{
			keyboard.showPopupMenu(0);
		};

		uint8 layerValue = 1;

		for(auto t: tokens)
			addAndMakeVisible(buttons.add(new KeyswitchButton(parent.data, layerValue++, t, parent.data[groupIds::purgable])));

		keyboard.rebuildKeyboard();
	}

	int getHeightToUse() const override
	{
		int h = 0;
		h += 2 * LayerComponent::BodyMargin;
		h += jmax(100, height);
		h += LayerComponent::KeyboardComponent::KeyboardHeight;

		return h;
	}

	int getCentreXPositionForToken(uint8 tokenIndex) const override
	{
		if(auto p = buttons[tokenIndex])
			return p->getBoundsInParent().getCentreX();

		return 0;
	}

	int height = LayerComponent::TopBarHeight;

	void fillPopupMenu(PopupMenu& m, int noteNumber) override
	{
		PopupMenu sub;

		auto range = Helpers::getKeyRange(data, getSampler());
		auto firstBit = range.getFirstSetBit();

		for(int i = 0; i < 128; i++)
		{
			sub.addItem(i+1000, MidiMessage::getMidiNoteName(i, true, true, 3), true, i == firstBit);
		}

		m.addSubMenu("Set keyswitch start note", sub);

		m.addItem(2, "Set keyswitch start note to next played MIDI note", true, keyboard.learnActive);
		m.addItem(1, "Use white keys", true, !(bool)data[groupIds::isChromatic]);
	}

	void onPopupResult(int result, int noteNumber) override
	{
		if(result == 1)
		{
			data.setProperty(groupIds::isChromatic, !(bool)data[groupIds::isChromatic], getUndoManager());
		}
		else if (result == 2)
		{
			keyboard.learnActive = !keyboard.learnActive;
		}
		else
		{
			auto startNote = result - 1000;
			data.setProperty(SampleIds::LoKey, startNote, getUndoManager());
		}
	}

	void resized() override
	{
		auto b = getLocalBounds().reduced(LayerComponent::BodyMargin);

		for(auto b: buttons)
			b->setSize(b->getWidthToUse(), LayerComponent::TopBarHeight);

		SimpleFlexbox fb;
		fb.justification = Justification::centred;

		auto buttonList = getAllButtonsToShow();

		auto list = fb.createBoundsListFromComponents(buttonList);

		if(auto h = fb.apply(list, b.removeFromTop(100)))
		{
			height = h;

			fb.applyBoundsListToComponents(buttonList, list);

			if(getHeight() != getHeightToUse())
			{
				findParentComponentOfClass<Content>()->updateSize();
			}
		}

		keyboard.setBounds(b.removeFromBottom(LayerComponent::KeyboardComponent::KeyboardHeight));

		auto tb = keyboard.getBounds();

		tb = tb.removeFromRight(tb.getHeight()).translated(tb.getHeight(), 0).reduced(3).getIntersection(getLocalBounds());
		editButton.setBounds(tb);

		BodyBase::resized();
	}

	LayerComponent::KeyboardComponent keyboard;

	ComplexGroupManagerComponent::Factory f;

	HiseShapeButton editButton;
};

struct ComplexGroupManagerComponent::TableFadeBody: public LogicTypeComponent::BodyBase,
													public ComboBox::Listener
{
	TableFadeBody(LogicTypeComponent& parent):
	  BodyBase(parent),
	  popupButton("edit", nullptr, f)
	{
		addAndMakeVisible(table);
		addAndMakeVisible(tableSelector);
		addAndMakeVisible(popupButton);

		tableSelector.addItemList(tokens, 1);

		tableSelector.addListener(this);

		popupButton.onClick = [this]()
		{
			auto t = new RRDisplayComponent::XFadeEditor(getSampler());
			auto root = findParentComponentOfClass<FloatingTile>()->getRootFloatingTile();
			root->showComponentInRootPopup(t, &popupButton, { getLocalBounds().getCentreX(), 15 }, false);
		};

		tableSelector.setSelectedId(1, sendNotificationSync);

		tableSelector.setLookAndFeel(&laf);

		uint8 layerValue = 1;

		for(auto t: tokens)
			addAndMakeVisible(buttons.add(new KeyswitchButton(parent.data, layerValue++, t, false)));
	}

	int getHeightToUse() const override { return 220 + LogicTypeComponent::TopBarHeight; }

	void comboBoxChanged(ComboBox* comboBoxThatHasChanged) override
	{
		if(comboBoxThatHasChanged == &tableSelector)
		{
			auto id = tableSelector.getSelectedId() - 1;
			auto t = getSampler()->getTable(id);
			t->setGlobalUIUpdater(getMainController()->getGlobalUIUpdater());
			t->setUndoManager(getMainController()->getControlUndoManager());
			table.setComplexDataUIBase(t);
		}
	}

	virtual int getCentreXPositionForToken(uint8 tokenIndex) const
	{
		auto tb = table.getBoundsInParent();

		auto normX = (float)tokenIndex / (float)(tokens.size() - 1);

		return tb.getX() + roundToInt(normX * (float)tb.getWidth());
	}

	void resized() override
	{
		auto b = getLocalBounds().reduced(LogicTypeComponent::BodyMargin);

		b.removeFromRight(LogicTypeComponent::PaddingLeft + 3 * LogicTypeComponent::BodyMargin);
		b.removeFromLeft(3 * LogicTypeComponent::BodyMargin);
		
		auto cb = b.removeFromBottom(28);

		tableSelector.setBounds(cb.removeFromLeft(128));
		popupButton.setBounds(cb.removeFromRight(cb.getHeight()).reduced(4));

		b.removeFromBottom(LogicTypeComponent::BodyMargin);

		b.removeFromLeft(LogicTypeComponent::BodyMargin);

		auto buttonRow = b.removeFromTop(LogicTypeComponent::TopBarHeight);

		b.removeFromTop(LogicTypeComponent::BodyMargin);

		table.setBounds(b);

		Array<int> positions;

		calculateXPositionsForItems(positions);

		auto buttonList = getAllButtonsToShow();

		if(unassignedButton->isVisible())
		{
			jassertfalse;
		}
		if(ignoreButton->isVisible())
		{
			jassertfalse;
		}

		for(int i = 0; i < buttons.size(); i++)
		{
			auto b = buttons[i];
			b->setSize(b->getWidthToUse(), LogicTypeComponent::TopBarHeight - 4);
			b->setCentrePosition(positions[i] - LogicTypeComponent::PaddingLeft, buttonRow.getCentreY());
		}

		BodyBase::resized();
	}

	Factory f;

	TableEditor table;
	ComboBox tableSelector;
	HiseShapeButton popupButton;

	SelectorLookAndFeel laf;

};

struct LayerPropertySelector: public ComboBox,
							  public ComponentWithGroupManagerConnection
{
	enum DataStorageType
	{
		Index,
		Text,
		numDataStorageTypes
	};

	LayerPropertySelector(ComponentWithGroupManagerConnection& parent, DataStorageType type_):
	  ComponentWithGroupManagerConnection(parent.getSampler()),
	  type(type_)
	{
		setLookAndFeel(&laf);
		onChange = [this]()
		{
			auto s = getText();

			if(type == DataStorageType::Text)
				layerData.setProperty(id, s, getUndoManager());
			else
				layerData.setProperty(id, items.indexOf(s), getUndoManager());

			if(additionalCallback)
				additionalCallback();
		};
	}

	int getHeightToUse() const override { return 28; }

	void setup(const ValueTree& v, const Identifier& id_, const StringArray& items_)
	{
		clear(dontSendNotification);
		addItemList(items_, 1);

		id = id_;
		jassert(v.getType() == groupIds::Layer);
		
		items = items_;

		if(!layerData.isValid())
		{
			layerData = v;
			listener.setCallback(layerData, { id }, valuetree::AsyncMode::Asynchronously, BIND_MEMBER_FUNCTION_2(LayerPropertySelector::update));
		}
		
		if(layerData.hasProperty(id))
			update(id, layerData[id]);
	}

	void update(const Identifier& id, const var& newValue)
	{
		if(type == DataStorageType::Text)
			setText(newValue.toString(), dontSendNotification);
		else
			setSelectedItemIndex((int)newValue, dontSendNotification);
	}

	std::function<void()> additionalCallback;

	Identifier id;
	UndoManager* um;
	StringArray items;
	const DataStorageType type;
	ValueTree layerData;
	valuetree::PropertyListener listener;
	ComplexGroupManagerComponent::SelectorLookAndFeel laf;
};

struct ComplexGroupManagerComponent::XFadeBody: public LogicTypeComponent::BodyBase
{
	XFadeBody(LogicTypeComponent& parent):
	  BodyBase(parent),
	  slotSelector(*this, LayerPropertySelector::DataStorageType::Index),
	  modeSelector(*this, LayerPropertySelector::DataStorageType::Text),
	  sourceSelector(*this, LayerPropertySelector::DataStorageType::Text)
	{
		// TODO: implement CC mode
		// TODO: implement smoothing
		// TODO: implement global mods 

		uint8 layerValue = 1;

		for(auto t: tokens)
			addAndMakeVisible(buttons.add(new KeyswitchButton(parent.data, layerValue++, t, false)));

		addAndMakeVisible(slotSelector);
		addAndMakeVisible(modeSelector);
		addAndMakeVisible(sourceSelector);
		
		auto idx = Helpers::getLayerIndex(parent.data);

		if(auto l = dynamic_cast<ComplexGroupManager::XFadeLayer*>(getComplexGroupManager()->layers[idx]))
		{
			l->valueUpdater.addListener(*this, updateValue, false);
		}

		modeSelector.additionalCallback = [this]()
		{
			resized();
		};

		StringArray slots;

		modeSelector.setup(parent.data, groupIds::fader, ComplexGroupManager::XFadeLayer::getFaderNames());
		sourceSelector.setup(parent.data, { groupIds::sourceType }, ComplexGroupManager::XFadeLayer::getSourceNames() );

		sourceSelector.additionalCallback = BIND_MEMBER_FUNCTION_0(XFadeBody::rebuildSlotItems);
		rebuildSlotItems();
	};

	void rebuildSlotItems()
	{
		auto idx = ComplexGroupManager::XFadeLayer::getSourceNames().indexOf(sourceSelector.getText());

		StringArray sa;

		if(idx != -1)
		{
			auto sourceType = (ComplexGroupManager::XFadeLayer::SourceTypes)idx;

			switch (sourceType)
			{
			case ComplexGroupManager::XFadeLayer::SourceTypes::EventData:
				for(int i = 0; i < AdditionalEventStorage::NumDataSlots; i++)
					sa.add("Event Data Slot #" + String((i+1)));
				break;
			case ComplexGroupManager::XFadeLayer::SourceTypes::MidiCC:
				sa.add("PitchBend");
				for(int i = 1; i < 128; i++)
					sa.add("CC #" + String(i));
				break;
			case ComplexGroupManager::XFadeLayer::SourceTypes::GlobalMod: 
			{
				auto chain = getSampler()->getMainController()->getMainSynthChain();
				if(auto container = ProcessorHelpers::getFirstProcessorWithType<GlobalModulatorContainer>(chain))
				{
					auto c = container->getChildProcessor(ModulatorSynth::GainModulation);

					for(int i = 0; i < c->getNumChildProcessors(); i++)
						sa.add(c->getChildProcessor(i)->getId());
				}

				break;
			}
			case ComplexGroupManager::XFadeLayer::SourceTypes::numSourceTypes: break;
			default: ;
			}
			
		}
		

		

		

		slotSelector.setup(data, groupIds::slotIndex, sa);
	}

	static void updateValue(XFadeBody& b, int voiceIndex, double value)
	{
		b.values[voiceIndex] = value;
		b.repaint();
	}

	template <int Index> Path createPath(int numPaths, ComplexGroupManager::XFadeLayer::FaderTypes type) const
	{
		int numPixels = 256;

		Path p;
		p.startNewSubPath(0.0f, 0.0f);

		if (numPixels > 0)
		{
			for (int i = 0; i < numPixels; i++)
			{
				double inputValue = (double)i / (double)numPixels;
				auto output = ComplexGroupManager::XFadeLayer::getFaderValueInternal<Index>(inputValue, numPaths, type);

				p.lineTo((float)i, -1.0f * output);
			}
		}

		p.lineTo(numPixels - 1, 0.0f);
		p.closeSubPath();

		return p;
	}

	int getHeightToUse() const override { return 5 * LayerComponent::BodyMargin + 80 + LayerComponent::TopBarHeight + slotSelector.getHeightToUse(); }

	void resized() override
	{
		BodyBase::resized();

		paths.clear();
		auto numPaths = tokens.size();

		if(numPaths >= 2)
		{
			using FaderType = ComplexGroupManager::XFadeLayer::FaderTypes;

			auto faderNames = ComplexGroupManager::XFadeLayer::getFaderNames();

			auto idx = faderNames.indexOf(data[groupIds::fader].toString());

			if(isPositiveAndBelow(idx, faderNames.size()))
			{
				auto type = (FaderType)idx;

				paths.add(createPath<0>(numPaths, type));
				paths.add(createPath<1>(numPaths, type));

				if(numPaths > 2)
					paths.add(createPath<2>(numPaths, type));
				if(numPaths > 3)
					paths.add(createPath<3>(numPaths, type));
				if(numPaths > 4)
					paths.add(createPath<4>(numPaths, type));
				if(numPaths > 5)
					paths.add(createPath<5>(numPaths, type));
				if(numPaths > 6)
					paths.add(createPath<6>(numPaths, type));
				if(numPaths > 7)
					paths.add(createPath<7>(numPaths, type));
			}

			
		}

		auto b = getLocalBounds();

		b.removeFromTop(LayerComponent::BodyMargin);
		b.removeFromTop(LayerComponent::BodyMargin);

		auto top = b.removeFromTop(LayerComponent::TopBarHeight);

		b.removeFromTop(LayerComponent::BodyMargin);

		auto fb = b.removeFromTop(80).withSizeKeepingCentre(256, 80).toFloat();

		pathArea = fb;

		if(fb.getX() > 0)
		{
			for(auto& p: paths)
				p.scaleToFit(fb.getX(), fb.getY(), fb.getWidth(), fb.getHeight(), false);
		}

		auto buttonList = getAllButtonsToShow();

		if(unassignedButton->isVisible())
		{
			unassignedButton->setBounds(top.removeFromLeft(unassignedButton->getWidthToUse()));
		}

		if(ignoreButton->isVisible())
		{
			ignoreButton->setBounds(top.removeFromRight(unassignedButton->getWidthToUse()));
		}
		
		if(tokens.size() >= 2)
		{
			auto w = 256 / (tokens.size() - 1);
			auto x = fb.getX();

			for(auto kb: buttons)
			{
				kb->setSize(kb->getWidthToUse(), LayerComponent::TopBarHeight);
				kb->setCentrePosition(x, top.getY());
				x += w;
			}
		}

		b.removeFromTop(LayerComponent::BodyMargin);

		auto bottom = b;

		bottom.removeFromBottom(LayerComponent::BodyMargin);

		modeSelector.setBounds(bottom.removeFromLeft(128));
		slotSelector.setBounds(bottom.removeFromRight(128));
		sourceSelector.setBounds(bottom.withSizeKeepingCentre(128, bottom.getHeight()));

		BodyBase::resized();
	}

	int getCentreXPositionForToken(uint8 tokenIndex) const override
	{
		if(auto b = buttons[tokenIndex])
			return b->getBoundsInParent().getCentreX();

		return 0;
	}

	void paint(Graphics& g) override
	{
		BodyBase::paint(g);

		if(paths.isEmpty())
		{
			g.setColour(Colours::white.withAlpha(0.2f));
			g.setFont(GLOBAL_BOLD_FONT());
			g.drawText("No tokens in this layer", getLocalBounds().toFloat(), Justification::centred);
		}
		else
		{
			g.setColour(Colours::white.withAlpha(0.2f));

			for(const auto& p: paths)
			{
				g.fillPath(p);
				g.strokePath(p, PathStrokeType(1.0f));
			}

			auto x = pathArea.getX() + values[0] * pathArea.getWidth();

			g.setColour(Colours::white);
			g.drawVerticalLine(x, pathArea.getY(), pathArea.getBottom());
		}
	}

	span<float, NUM_POLYPHONIC_VOICES> values;

	Rectangle<float> pathArea;

	faders::cosine_half lf;

	Array<Path> paths;

	LayerPropertySelector slotSelector;
	LayerPropertySelector sourceSelector;
	LayerPropertySelector modeSelector;

	JUCE_DECLARE_WEAK_REFERENCEABLE(XFadeBody);
};



struct ComplexGroupManagerComponent::LegatoBody: public LogicTypeComponent::BodyBase
{
	LegatoBody(LogicTypeComponent& parent):
	  BodyBase(parent),
	  keyboard(parent),
	  transition(parent)
	{
		uint8 layerValue = 1;

		for(auto t: tokens)
		{
			auto n = MidiMessage::getMidiNoteName(t.getIntValue(), true, true, 3);
			addAndMakeVisible(buttons.add(new KeyswitchButton(parent.data, layerValue++, n, false)));
			buttons.getLast()->setSize(buttons.getLast()->getWidthToUse(), ButtonHeight);
		}
			
		transition.startOffset = keyboard.noteRange.getStart() - 1;

		addAndMakeVisible(keyboard);
		addAndMakeVisible(transition);

		
		
	}

	

	void paint(Graphics& g) override
	{
		drawLabelsAndRulers(g);
		BodyBase::paint(g);
	}

	int getHeightToUse() const override
	{
		return LayerComponent::BodyMargin + AddComponent::LabelHeight +
			   height + LayerComponent::BodyMargin +
			   keyboard.getHeightToUse() + LayerComponent::BodyMargin +
			   transition.getHeightToUse() + LayerComponent::BodyMargin;
	}

	void resized() override
	{
		clearRulersAndLabels();

		auto b = getLocalBounds().reduced(LayerComponent::BodyMargin);

		addLabel(b.removeFromTop(AddComponent::LabelHeight), "Start note groups:");

		SimpleFlexbox fb;


		//for(auto b: buttons)
		//	b->setSize(b->getWidthToUse(), LayerComponent::TopBarHeight);

		auto buttonList = getAllButtonsToShow();

		auto pos = fb.createBoundsListFromComponents(buttonList);

		if(auto h = fb.apply(pos, b.removeFromTop(height)))
		{
			height = h;

			fb.applyBoundsListToComponents(buttonList, pos);

			if(getHeight() != getHeightToUse())
				findParentComponentOfClass<Content>()->updateSize();
		}

		transition.setBounds(b.removeFromBottom(transition.getHeightToUse()));

		

		b.removeFromBottom(LayerComponent::BodyMargin);
		keyboard.setBounds(b.removeFromBottom(keyboard.getHeightToUse()));
		addRuler(b.removeFromBottom(LayerComponent::BodyMargin));

		BodyBase::resized();
	}

	struct TransitionDisplay: public Component,
							  public ComponentWithGroupManagerConnection
	{
		TransitionDisplay(LogicTypeComponent& parent):
		  ComponentWithGroupManagerConnection(parent.getSampler())
		{
			parent.getComplexGroupManager()->addPlaystateListener(*this, Helpers::getLayerIndex(parent.data), onPlaystate);
			parent.getSampleEditHandler()->noteBroadcaster.addListener(*this, onNote);
		}

		void paint(Graphics& g) override
		{
			auto b = getLocalBounds().toFloat();
			g.setColour(Colours::white.withAlpha(0.05f));
			g.fillRect(b.reduced(4.0f));
			g.setColour(Colours::white.withAlpha(0.3f));
			g.drawRoundedRectangle(b.reduced(1.0f), 4.0f, 1.0f);

			String msg;

			if(currentStart != ComplexGroupManager::IgnoreFlag)
			{
				auto s = MidiMessage::getMidiNoteName(currentStart + startOffset, true, true, 3);
				auto e = MidiMessage::getMidiNoteName(currentNote, true, true, 3);

				msg << s << " -> " << e;
			}
			else
				msg << "no transition";

			Helpers::drawLabel(g, b, msg, Justification::centred);
		}

		static void onNote(TransitionDisplay& t, int note, int velo)
		{
			if(velo != 0)
			{
				t.currentNote = note;
				t.repaint();
			}
		}

		uint8 currentStart = ComplexGroupManager::IgnoreFlag;
		int currentNote = -1;
		int startOffset = 0;

		static void onPlaystate(TransitionDisplay& t, uint8 value)
		{
			t.currentStart = value;
			t.repaint();
		}

		int getHeightToUse() const override { return 20; }

		JUCE_DECLARE_WEAK_REFERENCEABLE(TransitionDisplay);
	} transition;

	LayerComponent::KeyboardComponent keyboard;

	int height = LayerComponent::TopBarHeight;

};

struct ComplexGroupManagerComponent::ChokeBody: public KeyswitchBody
{
	ChokeBody(LogicTypeComponent& parent):
	  KeyswitchBody(parent)
	{}
};

}

