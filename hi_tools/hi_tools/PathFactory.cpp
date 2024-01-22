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


	juce::Path ChainBarPathFactory::createPath(const String& id) const
	{
		Path p;

#if !HISE_NO_GUI_TOOLS
		auto url = StringSanitizer::get(id);

		LOAD_EPATH_IF_URL("midi", ProcessorIcons::midiIcon);
		LOAD_EPATH_IF_URL("gain", ProcessorIcons::gainIcon);
		LOAD_EPATH_IF_URL("pitch", ProcessorIcons::pitchIcon);
		LOAD_EPATH_IF_URL("fx", ProcessorIcons::fxIcon);
		LOAD_EPATH_IF_URL("sample-start", ProcessorIcons::sampleStartIcon);
		LOAD_EPATH_IF_URL("group-fade", ProcessorIcons::groupFadeIcon);
		LOAD_EPATH_IF_URL("speaker", ProcessorIcons::speaker);
		LOAD_EPATH_IF_URL("fft", ProcessorIcons::fftIcon);
		LOAD_EPATH_IF_URL("stereo", ProcessorIcons::stereoIcon);
		LOAD_EPATH_IF_URL("osc", ProcessorIcons::pitchIcon);
		LOAD_EPATH_IF_URL("cpu", ProcessorIcons::cpuIcon);
		LOAD_EPATH_IF_URL("master-effects", HiBinaryData::SpecialSymbols::masterEffect);
		LOAD_EPATH_IF_URL("script", HiBinaryData::SpecialSymbols::scriptProcessor);
		LOAD_EPATH_IF_URL("polyphonic-effects", ProcessorIcons::polyFX);
		LOAD_EPATH_IF_URL("voice-start-modulator", ProcessorIcons::voiceStart);
		LOAD_EPATH_IF_URL("time-variant-modulator", ProcessorIcons::timeVariant);
		LOAD_EPATH_IF_URL("envelope", ProcessorIcons::envelope);
#endif
		return p;
	}

	HiseShapeButton::HiseShapeButton(const String& name, ButtonListener* listener, const PathFactory& factory,
		const String& offName):
		ShapeButton(name, Colours::white.withAlpha(0.5f), Colours::white.withAlpha(0.8f), Colours::white)
	{
		onShape = factory.createPath(name);

		if (offName.isEmpty())
			offShape = onShape;
		else
			offShape = factory.createPath(offName);

		if (listener != nullptr)
			addListener(listener);

		refreshShape();
		refreshButtonColours();
	}

	void HiseShapeButton::setToggleModeWithColourChange(bool shouldBeEnabled)
	{
		setClickingTogglesState(shouldBeEnabled);

		if (shouldBeEnabled)
			addListener(this);
		else
			removeListener(this);
	}

	void HiseShapeButton::setToggleStateAndUpdateIcon(bool shouldBeEnabled, bool forceUpdate)
	{
		if (forceUpdate || getToggleState() != shouldBeEnabled)
		{
			setToggleState(shouldBeEnabled, dontSendNotification);
			refreshButtonColours();
			refreshShape();
		}
	}

	void HiseShapeButton::buttonClicked(Button*)
	{
		refreshShape();
		refreshButtonColours();
	}


	PathFactory::Description::Description(const String& name, const String& description_) :
		url(StringSanitizer::get(name)),
		description(description_.trim())
	{

	}

	void PathFactory::updateCommandInfoWithKeymapping(juce::ApplicationCommandInfo& info)
	{
		auto mappings = getKeyMapping();

		if (mappings.size() > 0)
		{
			auto url = StringSanitizer::get(info.shortName);

			for (const auto& m : mappings)
			{
				if (m.url == url)
				{
					info.addDefaultKeypress(m.k.getKeyCode(), m.k.getModifiers());
				}
			}
		}

	}

	void PathFactory::scalePath(Path& p, Rectangle<float> f)
	{
		if(!isValid(p, f))
			return;

		p.scaleToFit(f.getX(), f.getY(), f.getWidth(), f.getHeight(), true);
	}

	void PathFactory::scalePath(Path& p, Component* c, float padding)
	{
		auto b = c->getBoundsInParent().toFloat().reduced(padding);
		scalePath(p, b);
	}

	bool PathFactory::isValid(const Path& p, Rectangle<float> area)
	{
		auto isOk = [](float v)
		{
			auto v2 = v;
			FloatSanitizers::sanitizeFloatNumber(v2);
			return v2 == v;
		};

		auto pb = p.getBounds();

		auto pathOk = isOk(pb.getX()) && isOk(pb.getY()) && isOk(pb.getWidth()) && isOk(pb.getHeight());

		auto areaOk = area.isEmpty() || (isOk(area.getX()) && isOk(area.getY()) && isOk(area.getWidth()) && isOk(area.getHeight()));

		return pathOk && areaOk;
	}

	PathFactory::PathFactory()
	{

	}

	String PathFactory::getId() const
	{ return {}; }

	PathFactory::~PathFactory()
	{}

	Array<PathFactory::Description> PathFactory::getDescription() const
	{ return {}; }

	Array<PathFactory::KeyMapping> PathFactory::getKeyMapping() const
	{ return {}; }

	PathFactory::KeyMapping::KeyMapping(const String& name, int keyCode, ModifierKeys::Flags mods)
	{
		url = StringSanitizer::get(name);
		k = KeyPress(keyCode, mods, 0);
	}

	void HiseShapeButton::refreshButtonColours()
	{
		if (getToggleState())
		{
			setColours(onColour.withAlpha(0.8f), onColour, onColour);
		}
		else
		{
			setColours(offColour.withMultipliedAlpha(0.5f), offColour.withMultipliedAlpha(0.8f), offColour);
		}

		repaint();
	}

	void HiseShapeButton::refreshShape()
	{
		if (getToggleState())
		{
			setShape(onShape, false, true, true);
		}
		else
			setShape(offShape, false, true, true);
	}

	void HiseShapeButton::refresh()
	{
		refreshShape();
		refreshButtonColours();
	}

	void HiseShapeButton::toggle()
	{
		setToggleState(!getToggleState(), dontSendNotification);

		refresh();
	}

	void HiseShapeButton::mouseDown(const MouseEvent& e)
	{
		CHECK_MIDDLE_MOUSE_DOWN(e);
		ShapeButton::mouseDown(e);
	}

	void HiseShapeButton::mouseUp(const MouseEvent& e)
	{
		CHECK_MIDDLE_MOUSE_UP(e);
		ShapeButton::mouseUp(e);
	}

	void HiseShapeButton::mouseDrag(const MouseEvent& e)
	{
		CHECK_MIDDLE_MOUSE_DRAG(e);
		ShapeButton::mouseDrag(e);
	}

	void HiseShapeButton::setShapes(Path newOnShape, Path newOffShape)
	{
		onShape = newOnShape;
		offShape = newOffShape;
	}

	void HiseShapeButton::clicked(const ModifierKeys& modifiers)
	{
		lastMods = modifiers;
		ShapeButton::clicked(modifiers);
	}
}
