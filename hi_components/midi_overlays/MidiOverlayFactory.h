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

namespace hise {
using namespace juce;


class MidiOverlayFactory: public DeletedAtShutdown
{
public:

	MidiOverlayFactory()
	{
		registerDefaultOverlays();

#if HI_ENABLE_ADDITIONAL_MIDI_OVERLAYS
		registerAdditionalMidiOverlays();
#endif
	}

#if HI_ENABLE_ADDITIONAL_MIDI_OVERLAYS
	// Overwrite this
	static void registerAdditionalMidiOverlays();
#endif

	static MidiOverlayFactory& getInstance()
	{
		if (instance == nullptr)
		{
			instance = new MidiOverlayFactory();
		}

		return *instance;
	}

	MidiPlayerBaseType* create(const Identifier& id, MidiPlayer* player)
	{
		for (const auto& item : items)
		{
			if (item.id == id)
				return item.f(player);
		}

		return nullptr;
	}

	Array<Identifier> getIdList() const
	{
		Array<Identifier> ids;
		
		for (const auto& item : items)
			ids.add(item.id);

		return ids;
	}

private:

	template <class T> void registerType()
	{
		items.add({ T::create, T::getId() });
	}

	void registerDefaultOverlays()
	{
		registerType<hise::MidiFileDragAndDropper>();
		registerType<hise::SimpleMidiViewer>();
		registerType<hise::MidiLooper>();
		registerType<hise::SimpleCCViewer>();
	}

	using CreateFunction = std::function<MidiPlayerBaseType*(MidiPlayer*)>;

	struct Item
	{
		CreateFunction f;
		Identifier id;
	};

	Array<Item> items;

	static MidiOverlayFactory* instance;
};


class MidiOverlayPanel : public PanelWithProcessorConnection
{
public:

	MidiOverlayPanel(FloatingTile* parent) :
		PanelWithProcessorConnection(parent)
	{
		setDefaultPanelColour(FloatingTileContent::PanelColourId::bgColour, Colour(0x25000000));
		setDefaultPanelColour(FloatingTileContent::PanelColourId::itemColour1, Colour(0xFF888888));
		setDefaultPanelColour(FloatingTileContent::PanelColourId::itemColour2, Colour(0xFF444444));
		setDefaultPanelColour(FloatingTileContent::PanelColourId::itemColour3, Colours::white);
		setDefaultPanelColour(FloatingTileContent::PanelColourId::textColour, Colours::white);
	};

	SET_PANEL_NAME("MidiOverlayPanel");

	Component* createContentComponent(int index) override
	{
		if (auto mp = dynamic_cast<MidiPlayer*>(getProcessor()))
		{
			auto id = MidiOverlayFactory::getInstance().getIdList()[index];

			if (auto newOverlay = MidiOverlayFactory::getInstance().create(id, mp))
			{
				newOverlay->setFont(getFont());

				auto asComponent = dynamic_cast<Component*>(newOverlay);

				asComponent->setColour(HiseColourScheme::ComponentBackgroundColour, findPanelColour(FloatingTileContent::PanelColourId::bgColour));
				asComponent->setColour(HiseColourScheme::ComponentFillTopColourId, findPanelColour(FloatingTileContent::PanelColourId::itemColour1));
				asComponent->setColour(HiseColourScheme::ComponentFillBottomColourId, findPanelColour(FloatingTileContent::PanelColourId::itemColour2));
				asComponent->setColour(HiseColourScheme::ComponentTextColourId, findPanelColour(FloatingTileContent::PanelColourId::textColour));
				asComponent->setColour(HiseColourScheme::ComponentOutlineColourId, findPanelColour(FloatingTileContent::PanelColourId::itemColour3));

				return asComponent;
			}
		}

		return nullptr;
	}

	int getFixedHeight() const override
	{
		if (auto mfpb = getContent<MidiPlayerBaseType>())
		{
			return mfpb->getPreferredHeight();
		}

		return 0;
	}

	void fillModuleList(StringArray& moduleList) override
	{
		fillModuleListWithType<MidiPlayer>(moduleList);
	}

	Identifier getProcessorTypeId() const override
	{
		return MidiPlayer::getClassType();
	}

	bool hasSubIndex() const override { return true; }

	void fillIndexList(StringArray& indexList) override
	{
		auto idList = MidiOverlayFactory::getInstance().getIdList();

		for (auto id : idList)
			indexList.add(id.toString());
	}
};

}