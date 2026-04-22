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

struct PreprocessorDataBase
{
	enum class Category
	{
		AudioProcessing,
		AutomationAndMacros,
		BackwardsCompatibility,
		DebugAndProfiling,
		DspAndFilters,
		LicensingAndExpansions,
		ModulatorSlots,
		PluginType,
		PolyphonyAndChannels,
		PresetAndState,
		SamplerAndStreaming,
		ThirdPartyModules,
		UiAndGraphics,
		numCategories
	};

	enum LinkType
	{
		UI,
		Module,
		Scriptnode,
		ScriptingApi,
		Language,
		Preprocessor
	};

	struct CrossReference
	{
		String toString() const
		{
			String u;
			u << "$";

			switch (type)
			{
			case UI:           u << "UI."; break;
			case Module:	   u << "MODULES."; break;
			case Scriptnode:   u << "SN."; break;
			case ScriptingApi: u << "API."; break;
			case Language:     u << "LANG."; break;
			case Preprocessor: u << "PP."; break;
			default:
				break;
			}

			u << url.replaceCharacter(' ', '-').replaceCharacter('/', '.');
			u << "$";

			u << " -- " << description;

			return u;
		}

		LinkType type;
		String url;
		String description;
	};

	

	static StringArray getCategoryNames()
	{
		return {
			"Audio Processing",
			"Automation & Macros",
			"Backwards Compatibility",
			"Debug & Profiling",
			"DSP & Filters",
			"Licensing & Expansions",
			"Modulator Slots",
			"Plugin Type",
			"Polyphony & Channels",
			"Preset & State",
			"Sampler & Streaming",
			"Third-Party Modules",
			"UI & Graphics"
		};
	}

	static StringArray getCategorySlugs()
	{
		return {
			"audio-processing",
			"automation-and-macros",
			"backwards-compatibility",
			"debug-and-profiling",
			"dsp-and-filters",
			"licensing-and-expansions",
			"modulator-slots",
			"plugin-type",
			"polyphony-and-channels",
			"preset-and-state",
			"sampler-and-streaming",
			"third-party-modules",
			"ui-and-graphics"
		};
	}

	struct Entry
	{
		var toJSON(MainController* mc, const String& preprocessor) const
		{
			DynamicObject::Ptr obj = new DynamicObject();

			if (mc != nullptr && supportsHotReload)
			{
				auto v = mc->getExtraDefinitionsValue(preprocessor, value);
				obj->setProperty("value", v);
			}
			else
			{
				obj->setProperty("value", value);
			}

			obj->setProperty("defaultValue", defaultValue);
			obj->setProperty("autoConfig", autoConfig);
			obj->setProperty("supportsHotReload", supportsHotReload);

			obj->setProperty("category", getCategoryNames()[(int)category]);
			obj->setProperty("category-slug", getCategorySlugs()[(int)category]);
			obj->setProperty("brief", brief);
			obj->setProperty("description", description);
			obj->setProperty("valueRange", valueRange.isEmpty() ? String() : (String(valueRange.getStart()) + "-" + String(valueRange.getEnd())));
			obj->setProperty("vestigal", vestigal);

			Array<var> urls;

			for (const auto& l : links)
				urls.add(l.toString());

			obj->setProperty("crossRefs", var(urls));

			return var(obj.get());
		}

		Category category;

		// A brief description. one sentence
		String brief;

		// 3-4 sentences explaining what the preprocessor does, what requirements
		// the preprocessor has and what implications it has for the project
		// if it affects audio / UI performance, explain the CPU load implications.
		// if changing this breaks the functionality of projects, mention it.
		// Don't repeat information that is written into any of the other fields.
		// Use authoring guideline at tools/api generator/style-guide/general.md
		// (minus the preprocessor gate obviously).
		String description;

		// the range of possible values (if applicable)
		Range<int> valueRange;

		// A list of links to other reference pages.
		Array<CrossReference> links;

		// the default value when not set manually
		int defaultValue = 0;

		// The overriden value
		int value = 0;

		// whether HISE uses the HISE_GET_PREPROCESSOR system
		// to dynamically hotload the values.
		bool supportsHotReload = false;

		// whether the hise::CompileExporter automatically sets those during export
		// (checks the projucer templates)
		// if true you'll probably need to leave them alone...
		bool autoConfig = false;

		// whether the preprocessor is actually a noop. Some preprocessors still stick around
		// in the codebase but don't do anything so this flags them.
		bool vestigal = false;

		Entry withVestigal() const {
			auto copy = *this;
			copy.vestigal = true;
			return copy;
		}

		Entry withAutoConfig() const {
			auto copy = *this;
			copy.autoConfig = true;
			return copy;
		}

		Entry withHotReload() const {
			auto copy = *this;
			copy.supportsHotReload = true;
			return copy;
		}

		Entry withDefault(int v) const {
			auto copy = *this;
			copy.defaultValue = v;
			return copy;
		}

		Entry withValue(int v) const {
			auto copy = *this;
			copy.value = v;
			return copy;
		}

		Entry withDescriptionLine(const String& desc) const {
			auto copy = *this;

			if (copy.description.isNotEmpty())
				copy.description << "\n";
			copy.description << desc;
			return copy;
		}

		Entry withBrief(const String& b) const {
			auto copy = *this;
			copy.brief = b;
			return copy;
		}

		Entry withCategory(Category c) const {
			auto copy = *this;
			copy.category = c;
			return copy;
		}

		Entry withCrossReference(LinkType l, const String& id, const String& description)
		{
			auto copy = *this;
			copy.links.add({ l, id, description });
			return copy;
		}
	};

	PreprocessorDataBase();

	std::map<String, Entry> data;

	int getValue(MainController* mc, const String& preprocessor) const
	{
		if (data.find(preprocessor) != data.end())
		{
			const auto& item = data.at(preprocessor);
			
			if (item.supportsHotReload && mc != nullptr)
				return mc->getExtraDefinitionsValue(preprocessor, item.value);

			return item.value;
		}

		return 0;
	}

	var toJSON(MainController* mc, bool verbose, bool skipDefaults = false) const
	{
		DynamicObject::Ptr obj = new DynamicObject();

		for (const auto& d : data)
		{
			const int runtimeValue = getValue(mc, d.first);

			if (skipDefaults && runtimeValue == d.second.defaultValue)
				continue;

			if (verbose)
				obj->setProperty(d.first, d.second.toJSON(mc, d.first));
			else
				obj->setProperty(d.first, runtimeValue);
		}

		return var(obj.get());
	}
};

}