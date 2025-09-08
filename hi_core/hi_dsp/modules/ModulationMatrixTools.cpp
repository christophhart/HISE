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

namespace MatrixIds
{


var Helpers::Properties::RangeData::toJSON() const
{
	if(rangePreset.isNotEmpty())
	{
		return var(rangePreset);
	}

	DynamicObject::Ptr obj = new DynamicObject();

	var i(new DynamicObject());
	var o(new DynamicObject());

	scriptnode::RangeHelpers::storeDoubleRange(i, inputRange, IDSET);
	scriptnode::RangeHelpers::storeDoubleRange(o, outputRange, IDSET);

	obj->setProperty(InputRange, i);
	obj->setProperty(OutputRange, o);
	obj->setProperty("mode", converter);
	obj->setProperty(UseMidPositionAsZero, useMidPositionAsZero);

	return var(obj.get());
}

Helpers::Properties::RangeData Helpers::Properties::RangeData::fromValueTrees(const ValueTree& ri, const ValueTree& ro)
{
	RangeData rd;

	rd.inputRange = scriptnode::RangeHelpers::getDoubleRange(ri, IDSET);
	rd.outputRange = scriptnode::RangeHelpers::getDoubleRange(ro, IDSET);
	rd.useMidPositionAsZero = (bool)ro[UseMidPositionAsZero];
	rd.converter = ri[TextConverter].toString();
	return rd;
}

void Helpers::Properties::RangeData::writeToValueTrees(ValueTree& ri, ValueTree& ro)
{
	scriptnode::RangeHelpers::storeDoubleRange(ri, inputRange, nullptr, IDSET);
	scriptnode::RangeHelpers::storeDoubleRange(ro, outputRange, nullptr, IDSET);
	ri.setProperty(TextConverter, converter, nullptr);
	ro.setProperty(UseMidPositionAsZero, useMidPositionAsZero, nullptr);
}

Helpers::Properties::RangeData Helpers::Properties::RangeData::fromJSON(const var& jsonOrPresetName)
{
	if(jsonOrPresetName.isString())
	{
		auto idx = getRangePresetNames().indexOf(jsonOrPresetName.toString());

		if(isPositiveAndBelow(idx, (int)RangePresets::numPresets))
		{
			return getRangePreset((RangePresets)idx);
		}
	}

	RangeData rd;
	auto obj = jsonOrPresetName;

	rd.useMidPositionAsZero = obj.getProperty(UseMidPositionAsZero, false);
	rd.converter = obj.getProperty("mode", "");

	rd.inputRange = scriptnode::RangeHelpers::getDoubleRange(obj.getProperty(InputRange, {}), IDSET);
	rd.outputRange = scriptnode::RangeHelpers::getDoubleRange(obj.getProperty(OutputRange, {}), IDSET);

	return rd;
}

std::pair<bool, Helpers::Properties::RangeData> Helpers::Properties::getRangeData(const String& targetId) const
{
	if(rangeData.find(targetId) != rangeData.end())
		return { true, rangeData.at(targetId) };

	return { false, {} };
}

Helpers::Properties::DefaultInitValue Helpers::Properties::getConvertedIntensityValue(const String& targetId) const
{
	if(initValues.find(targetId) != initValues.end())
	{
		auto dv = initValues.at(targetId);

		if(dv && dv.defaultMode != scriptnode::modulation::TargetMode::Gain)
		{
			if(rangeData.find(targetId) != rangeData.end())
			{
				auto rd = rangeData.at(targetId);

				if(!dv.isNormalised)
					dv.intensity = (float)rd.inputRange.convertTo0to1((double)dv.intensity, false);

				if(rd.useMidPositionAsZero)
				{
					dv.intensity -= 0.5f;
					dv.intensity *= 2.0f;
				}
			}
		}

		return dv;
	}

	return {};
}

void Helpers::Properties::sendChangeMessage(const String& targetId)
{
	propertyUpdateBroadcaster.sendMessage(sendNotificationSync, this, targetId);
}

void Helpers::Properties::fromJSON(const var& obj)
{
	selectableSources = obj.getProperty(SelectableSources, true);
	initValues.clear();
	rangeData.clear();

	static const StringArray modeNames = {
		"Scale",
		"Unipolar",
		"Bipolar"
	};

	if(auto o = obj[DefaultInitValues].getDynamicObject())
	{
		for(auto& nv: o->getProperties())
		{
			using TM = scriptnode::modulation::TargetMode;
			DefaultInitValue v;
			v.intensity = nv.value.getProperty(Intensity, 0.0);

			v.isNormalised = nv.value.getProperty(IsNormalized, true);

			auto idx = modeNames.indexOf(nv.value[Mode].toString());
			v.defaultMode = idx != -1 ? (TM)(int)idx : TM::Raw;

			initValues[nv.name.toString()] = v;
		}
	}
	if(auto o = obj[RangeProperties].getDynamicObject())
	{
		for(const auto& nv: o->getProperties())
		{
			rangeData[nv.name.toString()] = RangeData::fromJSON(nv.value);
		}
	}

	sendChangeMessage({});
}

var Helpers::Properties::toJSON(const MainController* mc) const
{
	using TM = scriptnode::modulation::TargetMode;
	DynamicObject::Ptr obj = new DynamicObject();

	static const StringArray modeNames = {
		"Scale",
		"Unipolar",
		"Bipolar"
	};

	StringArray targetList;

	fillModTargetList(mc, targetList, TargetType::All);

	obj->setProperty(SelectableSources, selectableSources);

	DynamicObject::Ptr iv = new DynamicObject();
	DynamicObject::Ptr rv = new DynamicObject();

	for(const auto& t: targetList)
	{
		DefaultInitValue dv;

		if(initValues.find(t) != initValues.end())
		{
			dv = initValues.at(t);
		}

		DynamicObject::Ptr item = new DynamicObject();
		item->setProperty(Intensity, dv.intensity);

		item->setProperty(IsNormalized, dv.isNormalised);

		if(dv.defaultMode != TM::Raw)
			item->setProperty(Mode, modeNames[(int)dv.defaultMode]);

		iv->setProperty(t, var(item.get()));

		if(rangeData.find(t) != rangeData.end())
		{
			rv->setProperty(t, rangeData.at(t).toJSON());
		}
	}

	obj->setProperty(DefaultInitValues, iv.get());
	obj->setProperty(RangeProperties, rv.get());

	return var(obj.get());
}

StringArray Helpers::getRangePresetNames()
{
	static const StringArray names = {
		"NormalizedPercentage",
		"Gain0dB",
		"Gain6dB",
		"Pitch1Octave",
		"Pitch2Octaves",
		"Pitch1Semitone",
		"FilterFreq",
		"FilterFreqLog",
		"Stereo",
	};

	return names;
}

Helpers::Properties::RangeData Helpers::getRangePreset(RangePresets p)
{
	Properties::RangeData rd;

	switch(p)
	{
	case RangePresets::NormalizedPercentage:
		rd.inputRange = { 0.0, 1.0};
		rd.outputRange = { 0.0, 1.0};
		rd.converter = "NormalizedPercentage";
		break;
	case RangePresets::Gain0dB:
		rd.inputRange = { -100.0, 0.0};
		rd.inputRange.setSkewForCentre(-6.0);
		rd.outputRange = { 0.0, 1.0};
		rd.converter = "Decibel";
		break;
	case RangePresets::Gain6dB:
		rd.inputRange = { -100.0, 6.0};
		rd.inputRange.setSkewForCentre(0.0);
		rd.outputRange = { 0.0, 2.0};
		rd.converter = "Decibel";
		break;
	case RangePresets::Pitch1Octave:
		rd.inputRange = { -12.0, 12.0};
		rd.inputRange.rng.interval = 1.0;
		rd.outputRange = { -1.0, 1.0};
		rd.converter = "Semitones";
		rd.useMidPositionAsZero = true;
		break;
	case RangePresets::Pitch2Octaves:
		rd.inputRange = { -24.0, 24.0};
		rd.inputRange.rng.interval = 1.0;
		rd.outputRange = { -2.0, 2.0};
		rd.converter = "Semitones";
		rd.useMidPositionAsZero = true;
		break;
	case RangePresets::Pitch1Semitone:
		rd.inputRange = { -1.0, 1.0};
		rd.inputRange.rng.interval = 0.01;
		rd.outputRange = { -1.0 / 12.0, 1.0 / 12.0};
		rd.converter = "Semitones";
		rd.useMidPositionAsZero = true;
		break;
	case RangePresets::FilterFreq:
		rd.inputRange = { 20.0, 20000.0};
		rd.outputRange = { 0.0, 1.0};
		rd.converter = "Frequency";
		break;
	case RangePresets::FilterFreqLog:
		rd.inputRange = { 20.0, 20000.0};
		rd.inputRange.rng.skew = 0.2998514912584123; // use 2khz as center point
		rd.outputRange = { 0.0, 1.0};
		rd.outputRange.rng.skew = 0.2998514912584123;
		rd.converter = "Frequency";
		break;
	case RangePresets::Stereo:
		rd.inputRange = { -100.0, 100.0};
		rd.outputRange = { 0.0, 1.0 };
		rd.converter = "Pan";
		rd.useMidPositionAsZero = true;
	case RangePresets::numPresets:
		break;
	default: ;
	}

	return rd;
}

ValueTree Helpers::addConnection(ValueTree& v, MainController* mc, const String& targetId, int sourceIndex)
{
	jassert(v.getType() == MatrixData);

	if(sourceIndex != -1)
	{
		auto existing = getConnection(v, sourceIndex, targetId);

		if(existing.isValid())
			return existing;
	}

	Properties::DefaultInitValue iv;

	

	auto um = mc->getControlUndoManager();


	if(auto gc = ProcessorHelpers::getFirstProcessorWithType<GlobalModulatorContainer>(mc->getMainSynthChain()))
	{
		iv = gc->matrixProperties.getConvertedIntensityValue(targetId);
	}

	if(!iv)
	{
		if(auto mod = dynamic_cast<Modulation*>(ProcessorHelpers::getFirstProcessorWithName(mc->getMainSynthChain(), targetId)))
		{
			auto m = mod->getMode();

			switch(m)
			{
			case Modulation::GainMode:
			case Modulation::GlobalMode:
				iv.intensity = 1.0f;
				iv.defaultMode = modulation::TargetMode::Gain;
				break;
			case Modulation::PitchMode:
			case Modulation::PanMode:
			case Modulation::OffsetMode:
			case Modulation::CombinedMode:
				iv.intensity = 0.0f;
				iv.defaultMode = modulation::TargetMode::Bipolar;
				break;
			case Modulation::numModes:
				break;
			default: ;
			}
		}
	}

	if(sourceIndex != -1)
	{
		for(auto c: v)
		{
			if(matchesTarget(c, targetId) && (int)c[SourceIndex] == -1)
			{
				c.setProperty(Mode, (int)iv.defaultMode, um);
				c.setProperty(SourceIndex, sourceIndex, um);
				c.setProperty(Intensity, iv.intensity, um);
				c.setProperty(AuxIndex, -1.0, um);
				c.setProperty(AuxIntensity, 0.0, um);
				c.setProperty(Inverted, false, um);

				return c;
			}
		}
	}
	
	ValueTree nd(Connection);

	nd.setProperty(TargetId, targetId, nullptr);
	nd.setProperty(Mode, (int)iv.defaultMode, nullptr);
	nd.setProperty(SourceIndex, sourceIndex, nullptr);
	nd.setProperty(Intensity, iv.intensity, nullptr);
	nd.setProperty(AuxIndex, -1.0, nullptr);
	nd.setProperty(AuxIntensity, 0.0, nullptr);
	nd.setProperty(Inverted, false, nullptr);
	v.addChild(nd, -1, um);
	return nd;
}

ValueTree Helpers::getMatrixDataFromGlobalContainer(MainController* mc)
{
	if(auto gc = ProcessorHelpers::getFirstProcessorWithType<GlobalModulatorContainer>(mc->getMainSynthChain()))
	{
		return gc->getMatrixModulatorData();
	}

	return {};
}

bool Helpers::removeConnection(ValueTree& data, UndoManager* um, const String& targetId, int sourceIndex)
{
	for(auto c: data)
	{
		if(c[TargetId].toString() == targetId && (int)c[SourceIndex] == sourceIndex)
		{
			data.removeChild(c, um);
			return true;
		}
	}

	return false;
}

bool Helpers::removeLastConnection(ValueTree& data, UndoManager* um, const String& targetId)
{
	for(int i = data.getNumChildren(); i >= 0; i--)
	{
		if(data.getChild(i)[TargetId].toString() == targetId)
		{
			data.removeChild(i, um);
			return true;
		}
	}

	return false;
}

bool Helpers::removeAllConnections(ValueTree& data, UndoManager* um, const String& targetId)
{
	auto ok = false;

	for(int i = 0; i < data.getNumChildren(); i++)
	{
		if(matchesTarget(data.getChild(i), targetId))
		{
			ok = true;
			data.removeChild(i--, um);
		}
	}

	return ok;
}

bool Helpers::matchesTarget(const ValueTree& data, const String& targetId)
{
	jassert(data.getType() == Connection);

	if(targetId.isEmpty())
		return true;

	return data[TargetId].toString() == targetId;
}

ValueTree Helpers::getConnection(const ValueTree& matrixData, int sourceIndex, const String& targetId)
{
	jassert(matrixData.getType() == MatrixData);

	for(auto c: matrixData)
	{
		if(matchesTarget(c, targetId) && (int)c[SourceIndex] == sourceIndex)
			return c;
	}

	return {};
}

void Helpers::fillModSourceList(const MainController* mc, StringArray& items)
{
	if(auto container = ProcessorHelpers::getFirstProcessorWithType<GlobalModulatorContainer>(mc->getMainSynthChain()))
	{
		auto mc = container->getChildProcessor(ModulatorSynth::InternalChains::GainModulation);
		for(int i = 0; i < mc->getNumChildProcessors(); i++)
			items.add(mc->getChildProcessor(i)->getId());
	}
}

void Helpers::fillModTargetList(const MainController* mc, StringArray& items, TargetType t)
{
	if(t == TargetType::All || t == TargetType::Modulators)
	{
		Processor::Iterator<MatrixModulator> iter(mc->getMainSynthChain());

		while(auto mm = iter.getNextProcessor())
		{
			items.addIfNotAlreadyThere(mm->getMatrixTargetId());
		}
	}
		

	if(t == TargetType::All || t == TargetType::Parameters)
	{
		if(auto jmp = JavascriptMidiProcessor::getFirstInterfaceScriptProcessor(mc))
		{
			auto content = jmp->getScriptingContent();

			valuetree::Helpers::forEach(content->getContentProperties(), [&](ValueTree& c)
			{
				static const Identifier id("matrixTargetId");
				if(c.hasProperty(id))
				{
					auto tid = c[id].toString();

					if(tid.isNotEmpty())
						items.addIfNotAlreadyThere(tid);
				}

				return false;
			});
		}
	}

	items.sortNatural();

	auto noConnectionIdx = items.indexOf("No connection");

	if(noConnectionIdx != -1)
	{
		items.remove(noConnectionIdx);
		items.insert(0, "No connection");
	}
}

int Helpers::getModulationSourceDragIndex(const var& data)
{
	if(data["Type"].toString() == "ModulationDrag")
		return (int)data[MatrixIds::SourceIndex];

	return -1;
}

void Helpers::updateDragState(HiSlider* s, DragTargetType state)
{
	if(s->isUsingModulatedRing())
	{
		s->getProperties().set("modulationDragState", (int)state);
		s->repaint();
	}
}

void Helpers::repaintMatrixSlidersOnDrag(Component* c, const var& data, DragTargetType state)
{
	auto idx = getModulationSourceDragIndex(data);

	if(idx != -1)
	{
		Component::callRecursive<HiSlider>(c, [&](HiSlider* s)
		{
			updateDragState(s, state);
			return false;
		});
	}
}

void Helpers::startDragging(Component* c, int sourceIndex)
{
	if(auto tc = DragAndDropContainer::findParentDragContainerFor(c))
	{
		auto rc = dynamic_cast<Component*>(tc);

		DynamicObject::Ptr dragInfo = new DynamicObject();
		dragInfo->setProperty("Type", "ModulationDrag");
		dragInfo->setProperty(MatrixIds::SourceIndex, sourceIndex);

		var di(dragInfo.get());
		repaintMatrixSlidersOnDrag(rc, di, DragTargetType::Dragging);
		tc->startDragging(di, c);
	}
}

void Helpers::stopDragging(Component* c)
{
	if(auto tc = DragAndDropContainer::findParentDragContainerFor(c))
	{
		auto rc = dynamic_cast<Component*>(tc);

		DynamicObject::Ptr dragInfo = new DynamicObject();
		dragInfo->setProperty("Type", "ModulationDrag");
		dragInfo->setProperty(MatrixIds::SourceIndex, 0);
		var di(dragInfo.get());
		repaintMatrixSlidersOnDrag(rc, di, DragTargetType::Inactive);

		if(auto co = c->findParentComponentOfClass<ControlledObject>())
		{
			auto chain = co->getMainController()->getMainSynthChain();

			if(auto gc = ProcessorHelpers::getFirstProcessorWithType<GlobalModulatorContainer>(chain))
			{
				gc->sendDragMessage(-1, "", GlobalModulatorContainer::DragAction::DragEnd);
			}
		}
	}
}

Helpers::TargetType Helpers::getTargetType(const MainController* mc, const String& targetId)
{
	if(targetId.isEmpty())
		return TargetType::All;

	StringArray items;
	fillModTargetList(mc, items, TargetType::Modulators);

	if(items.contains(targetId))
		return TargetType::Modulators;

	return TargetType::Parameters;
}


Helpers::IntensityTextConverter::IntensityTextConverter(const MatrixIds::Helpers::IntensityTextConverter::ConstructData& cd):
	AttributeListener(cd.p->getMainController()->getRootDispatcher()),
	data(cd),
	prettifier(ValueToTextConverter::createForMode(data.prettifierMode))
{
	if(cd.connectedSlider != nullptr)
	{
		uint16 pi = cd.parameterIndex;
		addToProcessor(cd.p, &pi, 1, dispatch::DispatchType::sendNotificationAsync);
	}
}

Helpers::IntensityTextConverter::~IntensityTextConverter()
{
	if(data.connectedSlider != nullptr)
		removeFromProcessor();
}

void Helpers::IntensityTextConverter::onAttributeUpdate(Processor*, uint16 index)
{
	if(data.connectedSlider != nullptr)
	{
		data.connectedSlider->updateText();
		data.connectedSlider->repaint();
	}
		
}

double Helpers::IntensityTextConverter::getValue(const String& text) const
{
	if(data.tm == modulation::TargetMode::Gain)
		return ValueToTextConverter::InverterFunctions::NormalizedPercentage(text);

	if(data.p != nullptr)
	{
		auto v = prettifier(text);
		FloatSanitizers::sanitizeDoubleNumber(v);

		auto inputRange = data.inputRange;
		// remove the interval to be able to set different values.
		inputRange.interval = 0;
		v = inputRange.convertTo0to1(v);

		if(data.tm != scriptnode::modulation::TargetMode::Gain)
		{
			v -= 0.5;
			v *= 2.0;
		}
		
		return v;
	}

	return 0.0;
}

String Helpers::IntensityTextConverter::getText(double v) const
{
	if(v == 0.0 && showInactive)
		return "Inactive";

	if(data.tm == modulation::TargetMode::Gain)
		return ValueToTextConverter::ConverterFunctions::NormalizedPercentage(v);

	if(data.p != nullptr)
	{
		auto ir = data.inputRange;
		ir.interval = 0;

		if(ir.skew == 1.0)
		{
			v = v * 0.5 + 0.5;
			v = ir.convertFrom0to1(v);

			if(data.tm == modulation::TargetMode::Unipolar)
				return prettifier(v);
			else
				return "+-" + prettifier(v).removeCharacters("+-");
		}
		else
		{
			auto value = data.p->getAttribute(data.parameterIndex);

			auto norm = ir.convertTo0to1(value);
			auto target = ir.convertFrom0to1(jlimit(0.0, 1.0, norm + v));
			auto diff = target - value;

			if(data.tm == modulation::TargetMode::Unipolar)
				return prettifier(diff);
			else
				return "+-" + prettifier(diff).removeCharacters("+-");
		}
	}
	
	return "disconnected";
}

}

} // namespace hise


