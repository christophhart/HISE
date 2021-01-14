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

#pragma once

namespace snex {

using namespace juce;
using namespace hise;
using namespace Types;

/** A wrapper around one of the complex data types with a update message. */
struct ExternalData
{
	struct Wrapper
	{
		static void updateTable(void* obj, double value)
		{
			auto table = static_cast<Table*>(obj);
			table->setNormalisedIndexSync((float)value);
		}

		static void updateSliderPack(void* obj, double value)
		{
			auto spd = static_cast<SliderPackData*>(obj);
			spd->setDisplayedIndex((int)value);
		}

		static void updateAudioFile(void* obj, double value)
		{
			auto mcb = static_cast<MultiChannelAudioBuffer*>(obj);
			mcb->sentDisplayIndexChangeMessage(value);
		}
	};

	ExternalData():
		dataType(DataType::numDataTypes),
		data(nullptr),
		numSamples(0),
		numChannels(0),
		obj(nullptr),
		f(nullptr)
	{}

	ExternalData(Table* t, int absoluteIndex_):
		dataType(DataType::Table),
		obj(t),
		f(Wrapper::updateTable)
	{
		data = const_cast<float*>(t->getReadPointer());
		numSamples = t->getTableSize();
		numChannels = 1;
	}

	ExternalData(SliderPackData* t, int absoluteIndex_) :
		dataType(DataType::SliderPack),
		obj(t),
		f(Wrapper::updateSliderPack)
	{
		data = const_cast<float*>(t->getCachedData());
		numSamples = t->getNumSliders();
		numChannels = 1;
	}

	ExternalData(MultiChannelAudioBuffer* buffer, int absoluteIndex_) :
		dataType(DataType::SliderPack),
		obj(buffer),
		f(Wrapper::updateAudioFile)
	{
		data = buffer->buffer.getArrayOfWritePointers();
		numSamples = buffer->buffer.getNumSamples();
		numChannels = buffer->buffer.getNumChannels();
	}

	/** Creates an external data object from a constant value class. */
	template <typename T> ExternalData(T& other):
		dataType(DataType::ConstantLookUp),
		obj(nullptr),
		f(nullptr)
	{
		data = other.begin();
		numSamples = other.size();
		numChannels = 1;
	}

	/** A function pointer to an update function. */
	typedef void(*UpdateFunction)(void*, double);

	enum class DataType
	{
		Table,
		SliderPack,
		AudioFile,
		numDataTypes,
		ConstantLookUp
	};

	static String getDataTypeName(DataType t)
	{
		switch (t)
		{
		case DataType::AudioFile: return "AudioFile";
		case DataType::SliderPack: return "SliderPack";
		case DataType::Table: return "Table";
		case DataType::ConstantLookUp: return "ConstantLookup";
		default: jassertfalse; return {};
		}
	}

	static void forEachType(const std::function<void(DataType)>& f)
	{
		f(DataType::Table);
		f(DataType::SliderPack);
		f(DataType::AudioFile);
	}

	void referBlockTo(block& b, int channelIndex) const
	{
		if (dataType == DataType::AudioFile)
		{
			if (isPositiveAndBelow(channelIndex, numChannels))
				b.referToRawData(((float**)data)[channelIndex], numSamples);
			else
				b.referToRawData(nullptr, 0);
		}
		else
			b.referToRawData((float*)data, numSamples);
	}

	void setDisplayedValue(double valueToDisplay)
	{
		if (f != nullptr)
			f(obj, valueToDisplay);
	}

	DataType dataType;
	int numSamples;
	int numChannels;
	void* data;
	void* obj;
	UpdateFunction f;
};

/** A interface class that handles the communication between a SNEX node and externally defined complex data types of HISE.

	This is the lowest common denominator for all the different data management situations in HISE and is used
	by the ExternalDataProviderBase to fetch the data required by the internal processing.

*/
struct ExternalDataHolder
{
	virtual ~ExternalDataHolder() {};

	/** Converts any data type to a float array for the given index.
	
		Be aware that the index is the index of each slot, not the total index.
	*/
	ExternalData getData(ExternalData::DataType t, int index);

	/** Converts the given index of each data type to an absolute index. */
	int getAbsoluteIndex(ExternalData::DataType t, int dataIndex) const;

	virtual int getNumDataObjects(ExternalData::DataType t) const = 0;

	virtual Table* getTable(int index) = 0;
	virtual SliderPackData* getSliderPack(int index) = 0;
	virtual MultiChannelAudioBuffer* getAudioFile(int index) = 0;

	/** Override this method and remove the object in question. Return true if successful. */
	virtual bool removeDataObject(ExternalData::DataType t, int index) = 0;

	/** Call this to clear all data objects. */
	void clearAllDataObjects()
	{
		ExternalData::forEachType([this](ExternalData::DataType t)
		{
			int numObjects = getNumDataObjects(t);

			for (int i = 0; i < numObjects; i++)
				removeDataObject(t, i);
		});
	}

	JUCE_DECLARE_WEAK_REFERENCEABLE(ExternalDataHolder);
};



/** A base class that fetches the data from the ExternalDataHolder and forwards it to
    its inner structure (either JIT compiled, hardcoded C++ or interpreted nodes)
*/
struct ExternalDataProviderBase
{
	/** Override this method and return the amount of data types that this provider requires.

		It will use these values at initialisation.
	*/
	virtual int getNumRequiredDataObjects(ExternalData::DataType t) const = 0;

	/** Override this method and forward the block with the given index to the internal data. */
	virtual void setExternalData(const ExternalData& data, int index) = 0;

	virtual ~ExternalDataProviderBase() {};

	void setExternalDataHolder(ExternalDataHolder* newHolder)
	{
		externalDataHolder = newHolder;
		initExternalData();
	}
	
	void initExternalData();

protected:

	WeakReference<ExternalDataHolder> externalDataHolder;
};


}


namespace scriptnode
{

struct NodeBase;

namespace data
{

using namespace snex;

namespace pimpl
{

	

template <int Index, ExternalData::DataType DType> struct base
{
	static constexpr int getNum(ExternalData::DataType t) { return t == DType ? 1 : 0; }

	static constexpr int NumTables = getNum(ExternalData::DataType::Table);
	static constexpr int NumSliderPacks = getNum(ExternalData::DataType::SliderPack);
	static constexpr int NumAudioFiles = getNum(ExternalData::DataType::AudioFile);

	void initialise(NodeBase* n) {};
};

template <int Index, ExternalData::DataType DType> struct plain : base<Index, DType>
{
	// Needs to be there for compatibility with the wrap::init class
	template <typename NodeType> plain(NodeType& n) {};

	template <typename NodeType> void setExternalData(NodeType& n, const ExternalData& b, int index)
	{
		if (index == Index && b.dataType == DType)
			n.setExternalData(b, 0);
	}
};

template <typename T, ExternalData::DataType DType> struct embedded : public base<-1, DType>
{
	static constexpr int getNum(ExternalData::DataType t) { return t == DType ? 1 : 0; }

	static constexpr int NumTables = getNum(ExternalData::DataType::Table);
	static constexpr int NumSliderPacks = getNum(ExternalData::DataType::SliderPack);
	static constexpr int NumAudioFiles = getNum(ExternalData::DataType::AudioFile);

	template <typename NodeType> embedded(NodeType& n) {};

	template <typename NodeType> void setExternalData(NodeType& n, const ExternalData& b, int index)
	{
		ExternalData d(obj.data);
		n.setExternalData(d, 0);
	}

	T obj;
};
}

namespace external
{
	template <int Index> using table =		 pimpl::plain<Index, ExternalData::DataType::Table>;
	template <int Index> using sliderpack = pimpl::plain<Index, ExternalData::DataType::SliderPack>;
	template <int Index> using audiofile =  pimpl::plain<Index, ExternalData::DataType::AudioFile>;
}

namespace embedded
{
	template <typename DataClass> using table =		   pimpl::embedded<DataClass, ExternalData::DataType::Table>;
	template <typename DataClass> using sliderpack =  pimpl::embedded<DataClass, ExternalData::DataType::SliderPack>;
	template <typename DataClass> using audiofile =   pimpl::embedded<DataClass, ExternalData::DataType::AudioFile>;
}

}
}