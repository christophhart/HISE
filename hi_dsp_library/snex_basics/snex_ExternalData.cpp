/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option any later version.
*
*   HISE is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licences for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licencing:
*
*   http://www.hartinstruments.net/hise/
*
*   HISE is based on the JUCE library,
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/


namespace snex {
	using namespace Types;
	using namespace hise;

ExternalData ExternalDataHolder::getData(ExternalData::DataType t, int index)
{
	using DataType = ExternalData::DataType;

	if (t == DataType::Table)
	{
		if (auto t = getTable(index))
		{
			return ExternalData(t, index);
		}
	}
	if (t == DataType::SliderPack)
	{
		if (auto sp = getSliderPack(index))
		{
			return ExternalData(sp, index);
		}
	}
	if (t == DataType::AudioFile)
	{
		if (auto bf = getAudioFile(index))
		{
			return ExternalData(bf, index);
		}
	}

	return ExternalData();
}

void ExternalDataProviderBase::initExternalData()
{
	int totalIndex = 0;

	if (externalDataHolder == nullptr)
		return;

	auto initAll = [this, &totalIndex](ExternalData::DataType d)
	{
		for (int i = 0; i < getNumRequiredDataObjects(d); i++)
		{
			auto b = externalDataHolder->getData(d, i);
			setExternalData(b, totalIndex++);
		}
	};

	initAll(ExternalData::DataType::Table);
	initAll(ExternalData::DataType::SliderPack);
	initAll(ExternalData::DataType::AudioFile);
}

int ExternalDataHolder::getAbsoluteIndex(ExternalData::DataType t, int dataIndex) const
{
	int offset = 0;

	if (t > ExternalData::DataType::Table)
		offset += getNumDataObjects(ExternalData::DataType::Table);
	 
	if (t > ExternalData::DataType::SliderPack)
		offset += getNumDataObjects(ExternalData::DataType::SliderPack);

	return offset + dataIndex;
}

hise::ComplexDataUIBase* ExternalDataHolder::getComplexBaseType(ExternalData::DataType t, int index)
{
	switch (t)
	{
	case ExternalData::DataType::Table:		 return getTable(index);
	case ExternalData::DataType::SliderPack: return getSliderPack(index);
	case ExternalData::DataType::AudioFile:  return getAudioFile(index);
	}

	return nullptr;
}

ExternalData::ExternalData(ComplexDataUIBase* b, int absoluteIndex) :
	dataType(getDataTypeForClass(b)),
	obj(b)
{
	SimpleReadWriteLock::ScopedReadLock sl(b->getDataLock());

	switch (dataType)
	{
	case DataType::AudioFile:
	{
		auto buffer = dynamic_cast<MultiChannelAudioBuffer*>(obj);
		data = buffer->getDataPtrs();
		numChannels = buffer->getBuffer().getNumChannels();
		numSamples = buffer->getCurrentRange().getLength();
		sampleRate = buffer->sampleRate;
		break;
	}
	case DataType::Table:
	{
		auto t = dynamic_cast<Table*>(obj);
		data = const_cast<float*>(t->getReadPointer());
		numSamples = t->getTableSize();
		numChannels = 1;
		break;
	}
	case DataType::SliderPack:
	{
		auto t = dynamic_cast<SliderPackData*>(obj);
		data = const_cast<float*>(t->getCachedData());
		numSamples = t->getNumSliders();
		numChannels = 1;
		break;
	}
	}
}

String ExternalData::getDataTypeName(DataType t, bool plural)
{
	switch (t)
	{
	case DataType::AudioFile: return plural ? "AudioFiles" : "AudioFile";
	case DataType::SliderPack: return plural ? "SliderPacks" : "SliderPack";
	case DataType::Table: return plural ? "Tables" : "Table";
	case DataType::ConstantLookUp: return "ConstantLookup";
	default: jassertfalse; return {};
	}
}

juce::Identifier ExternalData::getNumIdentifier(DataType t)
{
	String s;

	s << "Num";
	s << getDataTypeName(t);
	s << "s";

	return Identifier(s);
}

void ExternalData::forEachType(const std::function<void(DataType)>& f)
{
	f(DataType::Table);
	f(DataType::SliderPack);
	f(DataType::AudioFile);
}

void ExternalData::referBlockTo(block& b, int channelIndex) const
{
	if (dataType == DataType::AudioFile)
	{
		if (isPositiveAndBelow(channelIndex, numChannels))
			b.referToRawData(((float**)data)[channelIndex], numSamples);
		else
			b.referToNothing();
	}
	else
		b.referToRawData((float*)data, numSamples);
}

void ExternalData::setDisplayedValue(double valueToDisplay)
{
	if (obj != nullptr)
	{
		obj->sendDisplayIndexMessage(valueToDisplay);
	}
}

juce::AudioSampleBuffer ExternalData::toAudioSampleBuffer() const
{
	if (isEmpty())
		return {};

	if (dataType == DataType::AudioFile)
		return AudioSampleBuffer((float**)data, numChannels, numSamples);
	else
		return AudioSampleBuffer((float**)&data, 1, numSamples);
}

snex::ExternalData::DataType ExternalData::getDataTypeForClass(ComplexDataUIBase* d)
{
	if (auto s = dynamic_cast<SliderPackData*>(d))
		return DataType::SliderPack;
	if (auto t = dynamic_cast<Table*>(d))
		return DataType::Table;
	if (auto f = dynamic_cast<MultiChannelAudioBuffer*>(d))
		return DataType::AudioFile;

	return DataType::numDataTypes;
}

hise::ComplexDataUIBase::EditorBase* ExternalData::createEditor(ComplexDataUIBase* dataObject)
{
	hise::ComplexDataUIBase::EditorBase* c;

	if (auto t = dynamic_cast<hise::Table*>(dataObject))
	{
		c = new hise::TableEditor();
	}
	if (auto t = dynamic_cast<hise::SliderPackData*>(dataObject))
	{
		c = new hise::SliderPack();
	}
	if (auto t = dynamic_cast<hise::MultiChannelAudioBuffer*>(dataObject))
	{
		c = new hise::MultiChannelAudioBufferDisplay();
	}

	c->setComplexDataUIBase(dataObject);
	return c;
}

juce::Result InitialiserList::getValue(int index, VariableStorage& v)
{
	if (auto child = root[index])
	{
		try
		{
			if (child->getValue(v))
				return Result::ok();
			else
				return Result::fail("Can't resolve value at index " + juce::String(index));
		}
		catch (...)
		{
			return Result::fail("Expression can't be evaluated");
		}

	}
	else
		return Result::fail("Can't find item at index " + juce::String(index));
}

}