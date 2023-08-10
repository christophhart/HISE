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

namespace hise { using namespace juce;

ModuleBrowser::ModuleBrowser(BackendRootWindow* rootWindow) :
SearchableListComponent(rootWindow)
{
	setName("Module Browser");
    setFuzzyness(0.6);
}


void ModuleBrowser::ModuleItem::paint(Graphics& g)
{
	float w = (float)getWidth() - 4;
	float h = (float)getHeight();

	Colour c;

	switch (state)
	{
	case ModuleBrowser::ModuleItem::Inactive:
		c = Colours::black;
		break;
	case ModuleBrowser::ModuleItem::Illegal:
		c = Colours::red;
		break;
	case ModuleBrowser::ModuleItem::Legal:
		c = Colours::green;
		break;
	case ModuleBrowser::ModuleItem::numDragStates:
	default:
		c = Colours::black;
		break;
	}

	ColourGradient grad(c.withAlpha(0.1f), 0.0f, .0f, c.withAlpha(0.2f), 0.0f, h, false);

	g.setGradientFill(grad);

	g.fillRoundedRectangle(2.0f, 2.0f, w - 4.0f, h - 4.0f, 3.0f);

	g.setColour(isMouseOver() ? Colours::white : c.withAlpha(0.5f));

	g.drawRoundedRectangle(2.0f, 2.0f, w - 4.0f, h - 4.0f, 3.0f, 2.0f);

	Colour textColour = Colours::white.withAlpha(.7f);
	g.setColour(textColour);

	g.setFont(GLOBAL_BOLD_FONT());

	g.drawText(name, 0, 0, getWidth(), getHeight(), Justification::centred);
}


void ModuleBrowser::ModuleItem::mouseEnter(const MouseEvent&)
{
	repaint();
}

void ModuleBrowser::ModuleItem::mouseUp(const MouseEvent&)
{
	repaint();
}

void ModuleBrowser::ModuleItem::paintPopupBox(Graphics &g) const
{
	g.setColour(Colours::white);

	g.setFont(GLOBAL_BOLD_FONT());

	g.drawText("Parameter List", 0, 0, getWidth(), 20, Justification::centred);

	int h = 30;

	for (int i = 0; i < parameters.size(); i++)
	{
		g.setColour(Colours::white);
		g.setFont(GLOBAL_FONT());
		g.drawText(String(i), 10, h, 30, 20, Justification::centredLeft);

		g.setColour(Colours::white.withAlpha(0.6f));

		g.setFont(GLOBAL_MONOSPACE_FONT());

		g.drawText(parameters[i], 30, h, getWidth() - 30, 20, Justification::centredLeft);

		if (i != parameters.size() - 1)
		{
			g.setColour(Colours::white.withAlpha(0.3f));
			g.drawLine(10.0f, (float)h + 18.0f, getWidth() - 10.0f, (float)h + 18.0f, 0.5f);
		}

		h += 20;
	}
}

ModuleBrowser::ModuleItem::ModuleItem(String name_, Identifier id_) :
Item(name_.toLowerCase().replaceCharacter(' ', ';')),
id(id_),
name(name_),
state(Inactive)
{
	setSize(380 - 16 - 8 - 24, ITEM_HEIGHT);
}

void ModuleBrowser::ModuleItem::setParameters(const ValueTree &v)
{
	parameters.clear();

	if (v.isValid())
	{
		for (int i = 0; i < v.getNumProperties(); i++)
		{
			parameters.add(v.getProperty("id" + String(i), ""));
		}
	}
}

ModuleBrowser::ModuleCollection::ModuleCollection(Types t)
{
	File f = NativeFileHandler::getAppDataDirectory(nullptr).getChildFile("moduleEnums.xml");

	jassert(f.existsAsFile());

	String content = f.loadFileAsString();

	XmlDocument doc(f);

	auto xml = doc.getDocumentElement();

	switch (t)
	{
	case ModuleBrowser::ModuleCollection::MidiProcessors:
		typeName = "MIDI Processors";
		factoryType = new MidiProcessorFactoryType(nullptr);
		p = MidiProcessor::getSymbolPath();
		c = Colour(MIDI_PROCESSOR_COLOUR);
		if (xml != nullptr) vt = ValueTree::fromXml(*xml->getChildByName("MidiProcessors"));
		break;
	case ModuleBrowser::ModuleCollection::VoiceStartModulators:
		typeName = "Voice Start Modulators";
		factoryType = new VoiceStartModulatorFactoryType(NUM_POLYPHONIC_VOICES, Modulation::GainMode, nullptr);
		p = VoiceStartModulator::getSymbolPath();
		c = Colours::black.withAlpha(0.5f);
		if (xml != nullptr) vt = ValueTree::fromXml(*xml->getChildByName("VoiceStartModulators"));
		break;
	case ModuleBrowser::ModuleCollection::TimeVariantModulators:
		typeName = "Timevariant Modulators";
		factoryType = new TimeVariantModulatorFactoryType(Modulation::GainMode, nullptr);
		c = Colours::black.withAlpha(0.5f);
		p = TimeVariantModulator::getSymbolPath();
		if (xml != nullptr) vt = ValueTree::fromXml(*xml->getChildByName("TimeVariantModulators"));
		break;
	case ModuleBrowser::ModuleCollection::EnvelopeModulators:
		typeName = "Envelope Modulators";
		factoryType = new EnvelopeModulatorFactoryType(NUM_POLYPHONIC_VOICES, Modulation::GainMode, nullptr);
		c = Colours::black.withAlpha(0.5f);
		p = EnvelopeModulator::getSymbolPath();
		if (xml != nullptr) vt = ValueTree::fromXml(*xml->getChildByName("EnvelopeModulators"));
		break;
	case ModuleBrowser::ModuleCollection::ModulatorSynths:
		typeName = "Sound Generators";
		factoryType = new ModulatorSynthChainFactoryType(NUM_POLYPHONIC_VOICES, nullptr);
		p.loadPathFromData(BackendBinaryData::ToolbarIcons::keyboard, SIZE_OF_PATH(BackendBinaryData::ToolbarIcons::keyboard));
		c = Colours::white.withAlpha(0.7f);
        
		if (xml != nullptr) vt = ValueTree::fromXml(*xml->getChildByName("ModulatorSynths"));
		break;
	case ModuleBrowser::ModuleCollection::Effects:
		typeName = "Effects";
		factoryType = new EffectProcessorChainFactoryType(NUM_POLYPHONIC_VOICES, nullptr);
		p.loadPathFromData(HiBinaryData::ProcessorIcons::effectChain, SIZE_OF_PATH(HiBinaryData::ProcessorIcons::effectChain));
		c = Colour(EFFECT_PROCESSOR_COLOUR);
		if (xml != nullptr) vt = ValueTree::fromXml(*xml->getChildByName("Effects"));
		break;
	case ModuleBrowser::ModuleCollection::numTypes:
	default:
		jassertfalse;
		break;
	}

	Array<FactoryType::ProcessorEntry> entries = factoryType->getAllowedTypes();

	for (int i = 0; i < entries.size(); i++)
	{
		items.add(new ModuleItem(entries[i].name, entries[i].type));
		dynamic_cast<ModuleItem*>(items.getLast())->setParameters(vt.getChild(i));
		addAndMakeVisible(items.getLast());
	}
}

void ModuleBrowser::ModuleCollection::paint(Graphics &g)
{
	if (getWidth() <= 50)
		return;

	//g.fillAll(Colours::black.withAlpha(0.5f));
	p.scaleToFit(10.0f, 10.0f, 20.0f, 20.0f, true);
	g.setColour(Colours::white.withAlpha(0.4f));
	g.fillPath(p);

    g.setColour(c);

    g.fillRect(40, 5, getWidth() - 50, 30);
	
	g.setColour(Colours::white.withAlpha(0.15f));
	g.drawRect(40, 5, getWidth() - 50, 30);
	
    
    g.setColour(c.getBrightness() > 0.5f ? Colours::black : Colours::white);
	g.setFont(GLOBAL_BOLD_FONT());

	g.drawText(typeName, 50, 0, getWidth() - 50, 40, Justification::centredLeft);
}

} // namespace hise
