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

#ifndef PROCESSOR_METADATA_H
#define PROCESSOR_METADATA_H

namespace hise {
using namespace juce;

class Processor;
class RoutableProcessor;
class HotswappableProcessor;
class ModulatorSampler;
class MidiPlayer;
class WavetableSynth;
class VoiceStartModulator;
class TimeVariantModulator;
class EnvelopeModulator;
class EffectProcessor;
class MasterEffectProcessor;
class MonophonicEffectProcessor;
class VoiceEffectProcessor;
class MidiProcessor;
class ModulatorSynth;
class Chain;

#define DECLARE_ID(x) static const Identifier x(#x);

namespace ProcessorMetadataIds
{

DECLARE_ID(Modulator);
DECLARE_ID(Effect);
DECLARE_ID(MidiProcessor);
DECLARE_ID(SoundGenerator);
DECLARE_ID(TimeVariantModulator);
DECLARE_ID(VoiceStartModulator);
DECLARE_ID(EnvelopeModulator);

DECLARE_ID(MasterEffect);
DECLARE_ID(MonophonicEffect);
DECLARE_ID(VoiceEffect);

DECLARE_ID(Sampler);
DECLARE_ID(MidiPlayer);
DECLARE_ID(WavetableController);
DECLARE_ID(SlotFX);
DECLARE_ID(RoutingMatrix);

DECLARE_ID(TableProcessor);
DECLARE_ID(SliderPackProcessor); 
DECLARE_ID(AudioSampleProcessor);
DECLARE_ID(DisplayBufferSource);

DECLARE_ID(oscillator);
DECLARE_ID(sample_playback);
DECLARE_ID(container);
DECLARE_ID(sequencing);
DECLARE_ID(note_processing);
DECLARE_ID(dynamics);
DECLARE_ID(filter);
DECLARE_ID(delay);
DECLARE_ID(reverb);
DECLARE_ID(mixing);
DECLARE_ID(input);
DECLARE_ID(generator);
DECLARE_ID(routing);
DECLARE_ID(utility);
DECLARE_ID(custom);

}

#undef DECLARE_ID

/** Unified parameter and module metadata for HISE processors.
*
*	Replaces the scattered metadata across editor constructors, parameterNames,
*	getDefaultValue(), and ProcessorDocumentation with a single declaration per
*	processor type using a fluent builder pattern.
*
*	Each processor class declares a static createMetadata() method that returns
*	a fully populated ProcessorMetadata. This feeds three consumers:
*	1. Editor widget setup (md.setup(slider, processor, paramIndex))
*	2. Factory/registry queries (JSON dump without processor instantiation)
*	3. Documentation extraction (docs.hise.dev enrichment pipeline)
*/
struct ProcessorMetadata
{
	using WildcardFilterList = std::vector<std::pair<Identifier, String>>;

	struct ConstrainerParser
	{
		ConstrainerParser(const String& wildcardPattern);

		/** Check if a processor type is allowed by this constrainer.
		 *  Returns an empty Result on success, or an error message on rejection. */
		Result check(const ProcessorMetadata& md) const;

	private:

		const bool matchAll = false;
		StringArray positivePatterns;  // match against subtype
		StringArray negativePatterns;  // match against processor type id
	};

	enum class DataType
	{
		Undefined = 0,
		Static,
		Dynamic,
	};

	/** Metadata for a single processor parameter. */
	struct ParameterMetadata
	{
		enum class Type
		{
			Unspecified,
			Toggle,
			List,
			Float
		};

		ParameterMetadata():
			parameterIndex(-1)
		{ }

		/** Construct with the parameter's enum index. */
		ParameterMetadata(int parameterIndex_):
		  parameterIndex(parameterIndex_),
		  dataType(DataType::Static)
		{}

		// --- Fluent builder methods (return by value for chaining) ---

		ParameterMetadata withId(const String& name) const
		{
			auto copy = *this;
			copy.id = Identifier(name);
			return copy;
		}

		ParameterMetadata asDynamic() const
		{
			auto copy = *this;
			copy.dataType = DataType::Dynamic;
			return copy;
		}

		ParameterMetadata withDescription(const String& desc) const
		{
			auto copy = *this;
			BACKEND_ONLY(copy.description = desc);
			return copy;
		}

		ParameterMetadata withRange(scriptnode::InvertableParameterRange r) const
		{
			auto copy = *this;
			copy.range = r;
			if (copy.type == Type::Unspecified)
				copy.type = Type::Float;
			return copy;
		}

		/** Convenience: set slider mode and custom range in one call.
		*	This mirrors the common editor pattern: setMode(Mode, NormalisableRange).
		*/
		ParameterMetadata withSliderMode(HiSlider::Mode m, scriptnode::InvertableParameterRange r) const
		{
			auto copy = *this;
			copy.sliderMode = m;
			copy.range = r;

			auto modeString = HiSlider::getModeList()[(int)m];
			copy.vtc = ValueToTextConverter::createForMode(modeString);

			if (copy.type == Type::Unspecified)
				copy.type = Type::Float;
			return copy;
		}

		ParameterMetadata withDynamicDefault(const std::function<float(const Processor*)>& rf) const
		{
			auto copy = *this;
			copy.runtimeDefaultQueryFunction = rf;
			return copy;
		}

		ParameterMetadata withDefault(float value) const
		{
			auto copy = *this;
			copy.defaultValue = value;
			return copy;
		}

		ParameterMetadata withValueToTextConverter(const String& mode) const
		{
			auto copy = *this;
			copy.vtc = ValueToTextConverter::createForMode(mode);
			return copy;
		}

		/** For combo-box / list parameters. Sets type to List and populates valueNames.
		*	startIndex defaults to 1 since HiComboBox passes attribute values straight
		*	through as JUCE ComboBox item IDs (which must be >= 1).
		*/
		ParameterMetadata withValueList(const StringArray& items, int startIndex = 1) const
		{
			auto copy = *this;
			copy.type = Type::List;
            
            if(items.size() == 1)
                copy.range = { (double)startIndex, (double)startIndex + 1.0, 1.0};
            else
                copy.range = { (double)startIndex, (double)(startIndex + items.size() - 1), 1.0 };
            
			copy.vtc = ValueToTextConverter::createForOptions(items);
			return copy;
		}

		/** Marks this parameter as a toggle. Sets range to {0,1,1}. */
		ParameterMetadata asToggle() const
		{
			auto copy = *this;
			copy.type = Type::Toggle;
			copy.range = { 0.0, 1.0, 1.0 };
			copy.vtc = ValueToTextConverter::createForOptions({ "Off", "On" });
			return copy;
		}

		ParameterMetadata asDisabled() const
		{
			auto copy = *this;
			copy.disabled = true;
			return copy;
		}

		/** Marks this parameter as tempo-syncable.
		*
		*	Stores the index of the toggle parameter that controls whether this
		*	parameter operates in tempo-sync or free-running mode. The tempo-sync
		*	range, value names, and VTC are constant data derived at point of use
		*	(from TempoSyncer) rather than stored per-parameter.
		*/
		ParameterMetadata withTempoSyncMode(int tempoSyncParameterIndex) const
		{
			auto copy = *this;
			copy.tempoSyncControllerIndex = tempoSyncParameterIndex;
			return copy;
		}

		/** Convert to JSON for serialisation. */
		var toJSON() const;

		// --- Fields ---

		int parameterIndex = -1;
		Type type = Type::Unspecified;
		DataType dataType = DataType::Undefined;
		Identifier id;
		String description;
		int modulationIndex = -1;
		bool disabled = false;
		scriptnode::InvertableParameterRange range;
		float defaultValue = 0.0f;
		ValueToTextConverter vtc;
		HiSlider::Mode sliderMode = HiSlider::numModes;
		std::function<float(const Processor*)> runtimeDefaultQueryFunction;

		// Index of the toggle parameter controlling tempo-sync mode (-1 = not syncable)
		int tempoSyncControllerIndex = -1;
	};

	/** Metadata for a modulation chain (internal child processor slot). */
	struct ModulationMetadata
	{
		ModulationMetadata() = default;

		/** Construct with the chain's enum index. */
		ModulationMetadata(int chainIndex_):
		  chainIndex(chainIndex_),
		  dataType(DataType::Static)
		{}

		ModulationMetadata withId(const String& name) const
		{
			auto copy = *this;
			copy.id = Identifier(name);
			return copy;
		}

		ModulationMetadata asDynamic() const
		{
			auto copy = *this;
			copy.dataType = DataType::Dynamic;
			return copy;
		}

		ModulationMetadata withDescription(const String& desc) const
		{
			auto copy = *this;
			BACKEND_ONLY(copy.description = desc);
			return copy;
		}

		ModulationMetadata withColour(HiseModulationColours::ColourId colourId) const
		{
			auto copy = *this;
			copy.colour = colourId;
			return copy;
		}

		ModulationMetadata withModulatedParameter(int parameterIndex) const
		{
			auto copy = *this;
			copy.parameterIndex = parameterIndex;
			return copy;
		}

		ModulationMetadata withMode(scriptnode::modulation::ParameterMode mode) const
		{
			auto copy = *this;
			copy.modulationMode = mode;
			return copy;
		}

		template <typename CT> ModulationMetadata withConstrainer() const
		{
			auto copy = *this;
			auto wt = CT::getWildcard();
			bool found = false;

			for (const auto& w : wt)
			{
				if (w.first == ProcessorMetadataIds::Modulator)
				{
					copy.constrainerWildcard = w.second;
					found = true;
					break;
				}
			}
			
			jassert(found);
			
			return copy;
		}

		ModulationMetadata asDisabled() const
		{
			auto copy = *this;
			copy.disabled = true;
			return copy;
		}

		/** Convert to JSON for serialisation. */
		var toJSON() const;

		// --- Fields ---

		int chainIndex = -1;
		DataType dataType = DataType::Undefined;
		Identifier id;
		String description;
		int parameterIndex = -1;
		bool disabled = false;
		String constrainerWildcard = "*";
		HiseModulationColours::ColourId colour = HiseModulationColours::ColourId::Gain;
		scriptnode::modulation::ParameterMode modulationMode = scriptnode::modulation::ParameterMode::ScaleOnly;
	};

	ProcessorMetadata() = default;
	ProcessorMetadata(const Identifier& id_, DataType dt=DataType::Static):
		id(id_),
		dataType(dt)
	{};

	// --- Fluent builder methods for the top-level struct ---

	ProcessorMetadata withId(const Identifier& typeId) const
	{
		auto copy = *this;
		copy.id = typeId;
		return copy;
	}

	ProcessorMetadata withMetadataType(DataType t) const
	{
		auto copy = *this;
		copy.dataType = t;
		return copy;
	}

	ProcessorMetadata asDynamic() const
	{
		return withMetadataType(DataType::Dynamic);
	}

	template <typename ProcessorType> ProcessorMetadata withStandardMetadata() const
	{
		return (*this).withId(ProcessorType::getClassType())
			.withMetadataType(DataType::Static)
			.withPrettyName(ProcessorType::getClassName())
			.template withType<ProcessorType>()
			.template withInterface<ProcessorType>();
	}

	ProcessorMetadata withPrettyName(const String& name) const
	{
		auto copy = *this;
		copy.prettyName = name;
		return copy;
	}

	template <typename ConstrainerType> ProcessorMetadata withModConstrainer(int modChainIndex) const
	{
		auto copy = *this;

		jassert(isPositiveAndBelow(modChainIndex, copy.modulation.size()));

		auto m = copy.modulation[modChainIndex];
		copy.modulation.set(modChainIndex, m.withConstrainer<ConstrainerType>());
		return copy;
	}

	ProcessorMetadata withDisabledChain(int modChainIndex) const
	{
		auto copy = *this;

		jassert(isPositiveAndBelow(modChainIndex, copy.modulation.size()));

		auto m = copy.modulation[modChainIndex];
		copy.modulation.set(modChainIndex, m.asDisabled());
		return copy;
	}

	ProcessorMetadata withDisabledFX() const
	{
		auto copy = *this;
		copy.hasFX = false;
		return copy;
	}

	ProcessorMetadata withDescription(const String& desc) const
	{
		auto copy = *this;
		BACKEND_ONLY(copy.description = desc);
		return copy;
	}

	/** Creates a metadata with a dynamic modulation slot. 
	*   - numMax should be the limit from the preprocessor
	*   - dynamicChainOffset should be the number of static child chains
	*/
	ProcessorMetadata withDynamicModulation(const scriptnode::OpaqueNode::ModulationProperties& mod, int numMax, int dynamicChainOffset) const
	{
		auto copy = *this;

		for (int mi = 0; mi < numMax; mi++)
		{
			ModulationMetadata md;
			md.dataType = DataType::Dynamic;

			if (mod.isUsed(mi))
			{
				auto pi = mod.getParameterIndex(mi);
				auto piWithOffset = pi + copy.getNumParameters(DataType::Static);

				if (isPositiveAndBelow(piWithOffset, copy.parameters.size()))
				{
					md.id = copy.parameters[piWithOffset].id + "Modulation";
				}

				md.chainIndex = mod.getModulationChainIndex(pi) + dynamicChainOffset;
				md.colour = mod.getModulationColourRaw(pi);
				md.parameterIndex = piWithOffset;
				md.modulationMode = mod.getParameterMode(pi);
				md.disabled = mi >= numMax;

				if (isPositiveAndBelow(md.parameterIndex, copy.parameters.size()))
					copy.parameters.getReference(md.parameterIndex).modulationIndex = md.chainIndex;
			}
			else
			{
				md.chainIndex = dynamicChainOffset + mi;
				md.modulationMode = scriptnode::modulation::ParameterMode::ScaleAdd;
				md = md.asDisabled();
			}

			copy.modulation.add(md);
		}

		return copy;
	}

	ProcessorMetadata withDynamicParameter(const scriptnode::parameter::data& p, const String& customDescription = {}) const
	{
		ParameterMetadata pd;
		pd.id = p.info.getId();
		pd.parameterIndex = p.info.index + getNumParameters(DataType::Static);
		pd.range = p.toRange();
		pd.vtc = p.getValueToTextConverter();
		pd.description = customDescription;
		pd.defaultValue = p.info.defaultValue;
		pd.dataType = DataType::Dynamic;

		if (pd.range.rng.interval == 1.0)
		{
			if ((int)pd.range.getRange().getLength() == 1)
				pd.type = ParameterMetadata::Type::Toggle;
			else
				pd.type = ParameterMetadata::Type::List;
		}
		else
		{
			pd.type = ParameterMetadata::Type::Float;

			String n = pd.vtc((double)pd.defaultValue);

			if (n.contains("Hz"))
				pd.sliderMode = HiSlider::Mode::Frequency;
			else if (n.contains("ms"))
				pd.sliderMode = HiSlider::Time;
			else if (n.contains("%"))
				pd.sliderMode = HiSlider::NormalizedPercentage;
			else if (n.contains("/"))
				pd.sliderMode = HiSlider::TempoSync;
			else if (n.contains("dB"))
				pd.sliderMode = HiSlider::Decibel;
			else if (n.endsWithChar('L') || n.endsWithChar('R') || n.endsWithChar('C'))
				pd.sliderMode = HiSlider::Pan;
			else
				pd.sliderMode = HiSlider::Mode::Linear;
		}

		return withParameter(pd);
	}

	ProcessorMetadata withParameter(const ParameterMetadata& pd) const
	{
		auto copy = *this;

		// parameterIndex must equal the position in the array. Use asDisabled() to fill gaps.
		jassert(pd.parameterIndex == copy.parameters.size());

		copy.parameters.add(pd);
		return copy;
	}

	ProcessorMetadata withModulation(const ModulationMetadata& mod) const
	{
		auto copy = *this;
		copy.modulation.add(mod);

		if (isPositiveAndBelow(mod.parameterIndex, copy.parameters.size()))
		{
			auto& pd = copy.parameters.getReference(mod.parameterIndex);
			jassert(pd.dataType == mod.dataType);
			pd.modulationIndex = mod.chainIndex;
		}

		return copy;
	}

	template <typename CT> ProcessorMetadata withFXConstrainer() const
	{
		auto copy = *this;
		auto wt = CT::getWildcard();
		bool found = false;
		copy.hasFX = true;

		for (const auto& w : wt)
		{
			if (w.first == ProcessorMetadataIds::Effect)
			{
				copy.fxConstrainerWildcard = w.second;
				found = true;
				break;
			}
		}

		jassert(found);

		return copy;
	}

	template <typename CT> ProcessorMetadata withChildConstrainer() const
	{
		auto copy = *this;
		auto wt = CT::getWildcard();

		jassert(hasChildren);
		bool found = false;

		for (const auto& w : wt)
		{
			if (w.first == ProcessorMetadataIds::SoundGenerator)
			{
				copy.constrainerWildcard = w.second;
				found = true;
				break;
			}
		}

		jassert(found);

		return copy;
	}

	/** Set the FX constrainer wildcard that will be propagated to children's effect chains.
	*	Used by containers like ModulatorSynthGroup that restrict what effects their children can use.
	*/
	template <typename CT> ProcessorMetadata withChildFXConstrainer() const
	{
		auto copy = *this;
		auto wt = CT::getWildcard();

		copy.hasChildren = true;
		bool found = false;

		for (const auto& w : wt)
		{
			if (w.first == ProcessorMetadataIds::Effect)
			{
				copy.childFXConstrainerWildcard = w.second;
				found = true;
				break;
			}
		}

		jassert(found);

		return copy;
	}

	/** Set the processor type category from the C++ base class.
	*	Uses if-constexpr branching to resolve the type and subtype strings.
	*/
	template <typename T> ProcessorMetadata withType() const
	{
		auto copy = *this;

		if constexpr (std::is_base_of<Chain, T>())
			copy.hasChildren = true;

		if constexpr (std::is_base_of<VoiceStartModulator, T>())
		{
			copy.type = ProcessorMetadataIds::Modulator;
			copy.subtype = ProcessorMetadataIds::VoiceStartModulator;
		}
		else if constexpr (std::is_base_of<TimeVariantModulator, T>())
		{
			copy.type = ProcessorMetadataIds::Modulator;
			copy.subtype = ProcessorMetadataIds::TimeVariantModulator;
		}
		else if constexpr (std::is_base_of<EnvelopeModulator, T>())
		{
			copy.type = ProcessorMetadataIds::Modulator;
			copy.subtype = ProcessorMetadataIds::EnvelopeModulator;
		}
		else if constexpr (std::is_base_of<MasterEffectProcessor, T>())
		{
			copy.type = ProcessorMetadataIds::Effect;
			copy.subtype = ProcessorMetadataIds::MasterEffect;
		}
		else if constexpr (std::is_base_of<MonophonicEffectProcessor, T>())
		{
			copy.type = ProcessorMetadataIds::Effect;
			copy.subtype = ProcessorMetadataIds::MonophonicEffect;
		}
		else if constexpr (std::is_base_of<VoiceEffectProcessor, T>())
		{
			copy.type = ProcessorMetadataIds::Effect;
			copy.subtype = ProcessorMetadataIds::VoiceEffect;
		}
		else if constexpr (std::is_base_of<EffectProcessor, T>())
		{
			copy.type = ProcessorMetadataIds::Effect;
			copy.subtype = ProcessorMetadataIds::Effect;
		}
		else if constexpr (std::is_base_of<MidiProcessor, T>())
		{
			copy.type = ProcessorMetadataIds::MidiProcessor;
			copy.subtype = ProcessorMetadataIds::MidiProcessor;
		}
		else if constexpr (std::is_base_of<ModulatorSynth, T>())
		{
			copy.type = ProcessorMetadataIds::SoundGenerator;
			copy.subtype = ProcessorMetadataIds::SoundGenerator;
			copy.hasFX = true;
		}

		return copy;
	}

	/** Record which mix-in interfaces this processor implements. */
	template <typename T> ProcessorMetadata withInterface() const
	{
		auto copy = *this;

		if constexpr (std::is_same<ModulatorSampler, T>())
			copy.interfaceClasses.add(ProcessorMetadataIds::Sampler);
		if constexpr (std::is_same<MidiPlayer, T>())
			copy.interfaceClasses.add(ProcessorMetadataIds::MidiPlayer);
		if constexpr (std::is_same<WavetableSynth, T>())
			copy.interfaceClasses.add(ProcessorMetadataIds::WavetableController);
		if constexpr (std::is_base_of<HotswappableProcessor, T>() || std::is_same<HotswappableProcessor, T>())
			copy.interfaceClasses.addIfNotAlreadyThere(ProcessorMetadataIds::SlotFX);
		if constexpr (std::is_base_of<RoutableProcessor, T>() || std::is_same<RoutableProcessor, T>())
			copy.interfaceClasses.addIfNotAlreadyThere(ProcessorMetadataIds::RoutingMatrix);
		
		return copy;
	}

	ProcessorMetadata withComplexDataInterface(ExternalData::DataType dt) const
	{
		auto copy = *this;

		if (dt == ExternalData::DataType::Table)
			copy.interfaceClasses.add(ProcessorMetadataIds::TableProcessor);
		if (dt == ExternalData::DataType::SliderPack)
			copy.interfaceClasses.add(ProcessorMetadataIds::SliderPackProcessor);
		if (dt == ExternalData::DataType::AudioFile)
			copy.interfaceClasses.add(ProcessorMetadataIds::AudioSampleProcessor);
		if (dt == ExternalData::DataType::DisplayBuffer)
			copy.interfaceClasses.add(ProcessorMetadataIds::DisplayBufferSource);

		return copy;
	}




	// --- Editor setup methods ---

	/** Configure a HiSlider from the metadata for the given parameter index.
	*	Calls setup(), setMode(), and sets the tooltip from the description.
	*	Returns false if parameterIndex is out of range.
	*/
	bool setup(HiSlider& slider, Processor* p, int parameterIndex) const;

	/** Updates the slider mode based on tempo-sync state.
	*	Reads the processor and parameter index from the slider (set by setup()).
	*	Asserts if called on a parameter without tempoSyncControllerIndex.
	*/
	bool updateTempoSync(HiSlider& slider) const;

	/** Returns the default value for the given parameter. The processor reference is used to find the
		tempo sync value if applicable.
	*/
	float getDefaultValue(const Processor* p, int parameterIndex) const;

	/** Configure a HiComboBox from the metadata for the given parameter index.
	*	Calls setup() and populates the item list from valueNames.
	*	Returns false if parameterIndex is out of range.
	*/
	bool setup(HiComboBox& comboBox, Processor* p, int parameterIndex) const;

	/** Configure a HiToggleButton from the metadata for the given parameter index.
	*	Calls setup() and sets the tooltip from the description.
	*	Returns false if parameterIndex is out of range.
	*/
	bool setup(HiToggleButton& button, Processor* p, int parameterIndex) const;

	/** Convert the full metadata to JSON for serialisation. */
	var toJSON() const;

	/** Look up a ParameterMetadata by its parameter index.
	*	Returns nullptr if not found.
	*/
	const ParameterMetadata* getParameterById(int parameterIndex) const;

	/** Checks if the metadata has been properly initialised. */
	bool isValid() const { return dataType != DataType::Undefined; }

	String getBuilderPath() const
	{
		String b;
		b << "b.";
		b << type.toString() << "s.";
		b << id.toString();
		return b;
	}

	ParameterMetadata operator[](int index) const
	{
		if(isPositiveAndBelow(index, parameters.size()))
			return parameters[index];

		return {};
	}

	/** Returns the number of modulation slots with the defined type. Undefined returns all.*/
	int getNumModulationSlots(DataType dt = DataType::Undefined, bool includeDisabledChains = false) const
	{
		int num = 0;

		for (const auto& m : modulation)
		{
			if (!includeDisabledChains && m.disabled)
				continue;


			if (m.dataType == dt || dt == DataType::Undefined)
				num++;
		}

		return num;
	}

	/** Returns the number of parameters with the defined type. Undefined returns all. */
	int getNumParameters(DataType dt = DataType::Undefined) const
	{
		int num = 0;

		for (const auto& p : parameters)
		{
			if (p.dataType == dt || dt == DataType::Undefined)
				num++;
		}

		return num;
	}

	// --- Fields ---

	Identifier id;                        // e.g. "LFO" (from SET_PROCESSOR_NAME type)
	String prettyName;                    // e.g. "LFO Modulator" (display name)
	String description;                   // brief description
	Identifier type;                      // "Modulator", "Effect", "SoundGenerator", "MidiProcessor"
	Identifier subtype;                   // "TimeVariantModulator", "MasterEffect", etc.
	Array<Identifier> categories;		  // ["oscillator", "routing"]

	Array<Identifier> interfaceClasses;         // ["RoutableProcessor", "AudioSampleProcessor"]

	bool hasFX = false;
	bool hasChildren = false;
	String constrainerWildcard = "*";
	String fxConstrainerWildcard = "*";
	String childFXConstrainerWildcard = "*";

	Array<ParameterMetadata> parameters;
	Array<ModulationMetadata> modulation;

	DataType dataType = DataType::Undefined;
};



} // namespace hise

#endif // PROCESSOR_METADATA_H
