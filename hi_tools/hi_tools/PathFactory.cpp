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
		auto url = StringSanitizer::get(id);

		Path p;

		LOAD_PATH_IF_URL("midi", ProcessorIcons::midiIcon);
		LOAD_PATH_IF_URL("gain", ProcessorIcons::gainIcon);
		LOAD_PATH_IF_URL("pitch", ProcessorIcons::pitchIcon);
		LOAD_PATH_IF_URL("fx", ProcessorIcons::fxIcon);
		LOAD_PATH_IF_URL("sample-start", ProcessorIcons::sampleStartIcon);
		LOAD_PATH_IF_URL("group-fade", ProcessorIcons::groupFadeIcon);
		LOAD_PATH_IF_URL("speaker", ProcessorIcons::speaker);
		
		LOAD_PATH_IF_URL("master-effects", HiBinaryData::SpecialSymbols::masterEffect);
		LOAD_PATH_IF_URL("script", HiBinaryData::SpecialSymbols::scriptProcessor);
		LOAD_PATH_IF_URL("polyphonic-effects", ProcessorIcons::polyFX);
		LOAD_PATH_IF_URL("voice-start-modulator", ProcessorIcons::voiceStart);
		LOAD_PATH_IF_URL("time-variant-modulator", ProcessorIcons::timeVariant);
		LOAD_PATH_IF_URL("envelope", ProcessorIcons::envelope);

		return p;
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

	PathFactory::KeyMapping::KeyMapping(const String& name, int keyCode, ModifierKeys::Flags mods)
	{
		url = StringSanitizer::get(name);
		k = KeyPress(keyCode, mods, 0);
	}

}