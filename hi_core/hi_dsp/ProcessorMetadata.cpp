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

namespace hise {
using namespace juce;

// ============================================================================
// ParameterMetadata::toJSON
// ============================================================================

var ProcessorMetadata::ParameterMetadata::toJSON() const
{
	DynamicObject::Ptr obj = new DynamicObject();

	obj->setProperty("parameterIndex", parameterIndex);
	obj->setProperty("id", id.toString());

	switch (dataType)
	{
	
	case DataType::Static:
		obj->setProperty("metadataType", "static");
		break;
	case DataType::Dynamic:
		obj->setProperty("metadataType", "dynamic");
		break;
	case DataType::Undefined:
	default:
		jassertfalse;
		break;
	}

	obj->setProperty("description", description);
	

	// Type
	switch (type)
	{
	case Type::Toggle: obj->setProperty("type", "Button"); break;
	case Type::List:   obj->setProperty("type", "ComboBox"); break;
	case Type::Float:  obj->setProperty("type", "Slider"); break;
	default:           obj->setProperty("type", "Unspecified"); break;
	}

	var o(obj.get());

	var r(new DynamicObject());

	scriptnode::RangeHelpers::storeDoubleRange(r, range, scriptnode::RangeHelpers::IdSet::ScriptComponents);
	r.getDynamicObject()->removeProperty(scriptnode::PropertyIds::Inverted);

	obj->setProperty("disabled", disabled);

	obj->setProperty("range", r);
	
	if (runtimeDefaultQueryFunction)
		obj->setProperty("defaultValue", "dynamic");
	else
		obj->setProperty("defaultValue", defaultValue);

	if (modulationIndex != -1)
		obj->setProperty("chainIndex", modulationIndex);

	// Value names (for List type, stored in vtc.itemList)
	if (!vtc.itemList.isEmpty())
	{
		Array<var> names;
		for (auto& n : vtc.itemList)
			names.add(n);
		obj->setProperty("items", var(names));
	}

	// Slider mode (if set)
	if (sliderMode != HiSlider::numModes)
	{
		if (isPositiveAndBelow(sliderMode, (int)HiSlider::numModes))
			obj->setProperty("mode", HiSlider::getModeList()[sliderMode]);

		obj->setProperty("unit", HiSlider::getSuffixForMode(sliderMode, 0.0).trim());
	}

	// Tempo sync dual-mode info (range/names are constant, derived from TempoSyncer)
	if (tempoSyncControllerIndex >= 0)
	{
		obj->setProperty("tempoSyncIndex", tempoSyncControllerIndex);
	}

	return o;
}

// ============================================================================
// ModulationMetadata::toJSON
// ============================================================================

var ProcessorMetadata::ModulationMetadata::toJSON() const
{
	DynamicObject::Ptr obj = new DynamicObject();

	obj->setProperty("chainIndex", chainIndex);
	obj->setProperty("id", id.toString());

	switch (dataType)
	{

	case DataType::Static:
		obj->setProperty("metadataType", "static");
		break;
	case DataType::Dynamic:
		obj->setProperty("metadataType", "dynamic");
		break;
	case DataType::Undefined:
	default:
		jassertfalse;
		break;
	}

	if (parameterIndex != -1)
		obj->setProperty("parameterIndex", parameterIndex);

	obj->setProperty("disabled", disabled);
	obj->setProperty("description", description);
	
	obj->setProperty("constrainer", constrainerWildcard);

	switch (modulationMode)
	{
	case scriptnode::modulation::ParameterMode::AddOnly:
		obj->setProperty("modulationMode", "offset");
		break;
	case scriptnode::modulation::ParameterMode::ScaleAdd:
		obj->setProperty("modulationMode", "combined");
		break;
	case scriptnode::modulation::ParameterMode::ScaleOnly:
		obj->setProperty("modulationMode", "gain");
		break;
	case scriptnode::modulation::ParameterMode::Pan:
		obj->setProperty("modulationMode", "pan");
		break;
	case scriptnode::modulation::ParameterMode::Pitch:
		obj->setProperty("modulationMode", "pitch");
		break;
	case scriptnode::modulation::ParameterMode::Disabled:
	case scriptnode::modulation::ParameterMode::numModulationModes:
	default:
		break;

	}

	HiseModulationColours hc;
	
	obj->setProperty("colour", "#" + hc.getColour(disabled ? HiseModulationColours::ColourId::ExtraMod : colour).toDisplayString(false));

	return var(obj.get());
}

// ============================================================================
// ProcessorMetadata::toJSON
// ============================================================================

var ProcessorMetadata::toJSON() const
{
	DynamicObject::Ptr obj = new DynamicObject();

	obj->setProperty("id", id.toString());
	obj->setProperty("prettyName", prettyName);
	obj->setProperty("description", description);
	obj->setProperty("type", type.toString());
	obj->setProperty("subtype", subtype.toString());

	Array<var> c;

	for (auto& cat : categories)
		c.add(cat.toString());

	obj->setProperty("category", var(c));
	obj->setProperty("builderPath", getBuilderPath());

	obj->setProperty("hasChildren", hasChildren);
	obj->setProperty("hasFX", hasFX);

	if (hasChildren)
		obj->setProperty("constrainer", constrainerWildcard);

	if(hasFX)
		obj->setProperty("fx_constrainer", fxConstrainerWildcard);

	if (hasChildren && childFXConstrainerWildcard != "*")
		obj->setProperty("child_fx_constrainer", childFXConstrainerWildcard);

	switch (dataType)
	{
	case DataType::Undefined:
		obj->setProperty("metadataType", "undefined");
		break;
	case DataType::Static:
		obj->setProperty("metadataType", "static");
		break;
	case DataType::Dynamic:
		obj->setProperty("metadataType", "dynamic");
		break;
	default:
		break;
	}

	Array<var> interfaces;
	for (auto& i : interfaceClasses)
		interfaces.add(i.toString());
	obj->setProperty("interfaces", var(interfaces));
	

	Array<var> params;
	for (auto& p : parameters)
		params.add(p.toJSON());
	
	obj->setProperty("parameters", var(params));
	

	Array<var> mods;
	for (auto& m : modulation)
		mods.add(m.toJSON());
	obj->setProperty("modulation", var(mods));
	
	return var(obj.get());
}

// ============================================================================
// ProcessorMetadata::getParameterById
// ============================================================================

const ProcessorMetadata::ParameterMetadata* ProcessorMetadata::getParameterById(int parameterIndex) const
{
	for (auto& p : parameters)
	{
		if (p.parameterIndex == parameterIndex)
			return &p;
	}

	return nullptr;
}

// ============================================================================
// ProcessorMetadata::setup (HiSlider)
// ============================================================================

bool ProcessorMetadata::setup(HiSlider& slider, Processor* p, int parameterIndex) const
{
	auto pd = getParameterById(parameterIndex);

	if (pd == nullptr)
		return false;

	slider.setup(p, parameterIndex, pd->id.toString());

	// Set mode with custom range if we have both
	if (pd->sliderMode != HiSlider::numModes)
	{
		if (pd->tempoSyncControllerIndex != -1)
		{
			updateTempoSync(slider);
			slider.updateValue();
		}
		else
		{
			auto nr = NormalisableRange<double>(pd->range.rng.start, pd->range.rng.end, pd->range.rng.interval);
			nr.skew = pd->range.rng.skew;
			slider.setMode(pd->sliderMode, nr);
		}
	}

	if (!pd->description.isEmpty())
		slider.setTooltip(pd->description);

	return true;
}

// ============================================================================
// ProcessorMetadata::setup (HiComboBox)
// ============================================================================

bool ProcessorMetadata::setup(HiComboBox& comboBox, Processor* p, int parameterIndex) const
{
	auto pd = getParameterById(parameterIndex);

	if (pd == nullptr)
		return false;

	comboBox.setup(p, parameterIndex, pd->id.toString());

	// Populate combo box items with IDs matching the attribute value range
	comboBox.clear(dontSendNotification);
	int startIndex = (int)pd->range.rng.start;
	for (int i = 0; i < pd->vtc.itemList.size(); i++)
		comboBox.addItem(pd->vtc.itemList[i], startIndex + i);

	auto v = (int)p->getAttribute(parameterIndex);

	comboBox.setSelectedId(v, dontSendNotification);

	if (!pd->description.isEmpty())
		comboBox.setTooltip(pd->description);

	return true;
}

// ============================================================================
// ProcessorMetadata::setup (HiToggleButton)
// ============================================================================

bool ProcessorMetadata::setup(HiToggleButton& button, Processor* p, int parameterIndex) const
{
	auto pd = getParameterById(parameterIndex);

	if (pd == nullptr)
		return false;

	button.setup(p, parameterIndex, pd->id.toString());

	if (!pd->description.isEmpty())
		button.setTooltip(pd->description);

	return true;
}

bool ProcessorMetadata::updateTempoSync(HiSlider& slider) const
{
	auto p = slider.getProcessor();
	auto parameterIndex = slider.getParameter();

	if (auto pd = getParameterById(parameterIndex))
	{
		auto ts = pd->tempoSyncControllerIndex;

		if (ts == -1)
		{
			// Only call this with tempo-syncable parameters
			jassertfalse;
			return false;
		}

		if (p->getAttribute(ts) > 0.5f)
		{
			slider.setValue(p->getAttribute(parameterIndex));
			slider.setMode(HiSlider::Mode::TempoSync);
		}
		else
		{
			auto nr = NormalisableRange<double>(pd->range.rng.start, pd->range.rng.end, pd->range.rng.interval);
			nr.skew = pd->range.rng.skew;
			slider.setMode(pd->sliderMode, nr);
		}

		return true;
	}

	return false;
}

float ProcessorMetadata::getDefaultValue(const Processor* p, int parameterIndex) const
{
	if (isPositiveAndBelow(parameterIndex, parameters.size()))
	{
		const auto& pd = parameters.getReference(parameterIndex);
		
		if (pd.runtimeDefaultQueryFunction)
			return pd.runtimeDefaultQueryFunction(p);

		if (pd.tempoSyncControllerIndex != -1)
		{
			if (p->getAttribute(pd.tempoSyncControllerIndex) > 0.5)
				return (float)TempoSyncer::Tempo::Eighth;
		}

		return pd.defaultValue;
	}

	jassertfalse;
	return 0.0f;
}

ProcessorMetadata::ConstrainerParser::ConstrainerParser(const String& wildcardPattern):
  matchAll(wildcardPattern == "*")
{
	if (!matchAll)
	{
		auto tokens = StringArray::fromTokens(wildcardPattern, "|", "");

		for (const auto& t : tokens)
		{
			if (t.startsWithChar('!'))
				negativePatterns.add(t.substring(1));
			else
				positivePatterns.add(t);
		}
	}
	
}

Result ProcessorMetadata::ConstrainerParser::check(const ProcessorMetadata& md) const
{
	if (matchAll)
		return Result::ok();

	auto typeString = md.id.toString();

	for (auto& n : negativePatterns)
	{
		if (typeString.matchesWildcard(n, false))
			return Result::fail("negative match");
	}

	auto isFilter = md.id == PolyFilterEffect::getClassType();

	for (auto p : positivePatterns)
	{
		// special rule: allow PolyFilterEffect to be used
		// as positive wildcard - this is required by NoMidiInputConstrainer
		if (isFilter && md.id.toString() == p)
			return Result::ok();


		if (p.matchesWildcard(md.subtype, false))
			return Result::ok();
	}

	if (!positivePatterns.isEmpty())
		return Result::fail("no positive match");
	else
		return Result::ok();
}

} // namespace hise
