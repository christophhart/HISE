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

}