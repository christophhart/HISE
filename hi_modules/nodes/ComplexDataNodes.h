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
 *   which also must be licenced for commercial applications:
 *
 *   http://www.juce.com
 *
 *   ===========================================================================
 */

#pragma once

namespace scriptnode
{
using namespace juce;
using namespace hise;

template <int NV> class seq_impl : public HiseDspBase
{
public:

	enum class Parameters
	{
		SliderPack
	};

	DEFINE_PARAMETERS
	{
		DEF_PARAMETER(SliderPack, seq_impl);
	}


	static constexpr int NumVoices = NV;

	SET_HISE_POLY_NODE_ID("seq");
	GET_SELF_AS_OBJECT(seq_impl);

	void createParameters(ParameterDataList& data) override;
	void initialise(NodeBase* n) override;
	bool handleModulation(double& value);
	void reset();
	void prepare(PrepareSpecs ps);

	template <typename FrameDataType> void processFrame(FrameDataType& data) noexcept
	{
		if (packData != nullptr)
		{
			auto peakValue = jlimit(0.0, 0.999999, DspHelpers::findPeak(data.begin(), data.size()));
			auto index = int(peakValue * (double)packData->getNumSliders());

			if (lastIndex.get() != index)
			{
				lastIndex.get() = index;
				modValue.get().setModValue(packData->getValue(index));
			}

			float v = (float)modValue.get().getModValue();

			for (auto& s : data)
				s = v;
		}
	}

	template <typename ProcessDataType> void process(ProcessDataType& data) noexcept
	{
		if (packData != nullptr)
		{
			auto peakValue = jlimit(0.0, 0.999999, DspHelpers::findPeak(data));
			auto index = int(peakValue * (double)(packData->getNumSliders()));

			if (lastIndex.get() != index)
			{
				lastIndex.get() = index;
				modValue.get().setModValue((double)packData->getValue(index));
				packData->setDisplayedIndex(index);
			}

			for (auto c : data)
				FloatVectorOperations::fill(c.getRawWritePointer(), (float)modValue.get().getModValue(), data.getNumSamples());

			String s;
		}
	}

	void setSliderPack(double indexAsDouble);

	WeakReference<SliderPackProcessor> sp;
	WeakReference<SliderPackData> packData;

	PolyData<int, NumVoices> lastIndex = -1;
	PolyData<ModValue, NumVoices> modValue;
};


DEFINE_EXTERN_NODE_TEMPLATE(seq, seq_poly, seq_impl);



struct TableNode : public HiseDspBase
{
	SET_HISE_NODE_ID("table");
	GET_SELF_AS_OBJECT(TableNode);

	enum class Parameters
	{
		TableIndex
	};

	DEFINE_PARAMETERS
	{
		DEF_PARAMETER(TableIndex, TableNode);
	}

	struct TableInterface;

	void createParameters(ParameterDataList& data) override;
	void initialise(NodeBase* n) override;
	bool handleModulation(double& value);
	void prepare(PrepareSpecs);
	void reset() noexcept;

	template <typename ProcessDataType> void process(ProcessDataType& data) noexcept
	{
		if (tableData != nullptr)
		{
			auto peakValue = jlimit(0.0, 1.0, DspHelpers::findPeak(data));
			auto value = tableData->getInterpolatedValue(peakValue * SAMPLE_LOOKUP_TABLE_SIZE);

			changed = currentValue != value;

			if (changed)
				currentValue = value;

			for (auto c : data)
				hmath::vset(data.toChannelData(c), (float)currentValue);
		}
	}

	template <typename FrameDataType> void processFrame(FrameDataType& data) noexcept
	{
		if (tableData != nullptr)
		{
			auto peakValue = jlimit(0.0, 1.0, DspHelpers::findPeak(data.begin(), data.size()));
			auto value = tableData->getInterpolatedValue(peakValue * SAMPLE_LOOKUP_TABLE_SIZE);

			changed = currentValue != value;

			if (changed)
				currentValue = value;

			for (int i = 0; i < data.size(); i++)
				data[i] = (float)currentValue;
		}
	}

	void setTableIndex(double indexAsDouble);

	WeakReference<LookupTableProcessor> tp;
	WeakReference<SampleLookupTable> tableData;

	double currentValue = 0;
	bool changed = true;

	JUCE_DECLARE_WEAK_REFERENCEABLE(TableNode);
};

namespace core
{

struct file_base
{
	virtual ~file_base() {};

	CriticalSection fileLock;
	std::atomic<bool> writeFlag;
	AudioSampleBuffer displayBuffer;

	JUCE_DECLARE_WEAK_REFERENCEABLE(file_base);
};

struct FileDisplay : public ScriptnodeExtraComponent<file_base>
{
	FileDisplay(file_base* b, PooledUIUpdater* u):
		ScriptnodeExtraComponent<file_base>(b, u)
	{
		setSize(400, 100);
	}

	static Component* createExtraComponent(file_base* b, PooledUIUpdater* u)
	{

	}


};

template <typename T, int C> struct file_node: public file_base
{
	GET_SELF_OBJECT(obj);
	GET_WRAPPED_OBJECT(obj);

	void initialise(NodeBase* n)
	{
		fileProperty.initWithRoot(n, nullptr, *this);
		obj.initialise(n);
	}

	static Identifier getStaticId() { return T::getStaticId(); }

	void reset() 
	{ 
		ScopedLock sl(fileLock);
		obj.reset();
	}
	void prepare(PrepareSpecs ps) 
	{ 
		ScopedLock sl(fileLock);
		obj.prepare(ps); 
	}
	
	void handleHiseEvent(HiseEvent& e)
	{
		if(!writeFlag)
			obj.handleHiseEvent(e);
	}

	template <typename ProcessDataType> void process(ProcessDataType& data)
	{
		if(!writeFlag)
			obj.process(data.as<ProcessData<C>>());
	}

	template <typename FrameDataType> void processFrame(FrameDataType& data)
	{
		if(!writeFlag)
			obj.processFrame(data);
	}

	void createParameters(ParameterDataList& data)
	{
		obj.createParameters(data);
	}

	T obj;

	struct PropClass
	{
		using FileType = typename properties::file<PropClass, C>::type;

		static Identifier getId()
		{
			return PropertyIds::File;
		}

		void setFile(file_node& obj, FileType& file)
		{
			int numSamples = file.data[0].size();

			for (int i = 0; i < C; i++)
				displayBuffer.copyFrom(i, 0, file.data[i].begin(), numSamples);

			
			writeFlag.store(true);
			ScopedLock sl(fileLock);
			obj.setFile(file.data, file.sampleRate);
			writeFlag.store(false);
		}
	};

	properties::file<PropClass, C> fileProperty;
};

struct file_player : public AudioFileNodeBase
{
	SET_HISE_NODE_ID("file_player");
	GET_SELF_AS_OBJECT();

	file_player();;

	void prepare(PrepareSpecs specs) override;
	void reset();
	bool handleModulation(double& modValue);

	template <typename AudioDataType> void setFile(AudioDataType& audio, double sampleRate)
	{

	}

	span<dyn<float>, 2> currentData;

	template <typename ProcessDataType> void process(ProcessDataType& data) noexcept
	{
		static_assert(std::is_base_of<snex::Types::InternalData, ProcessDataType>(), "not called with process data type");

		SpinLock::ScopedLockType sl(audioFile->getLock());

		if (currentBuffer->clear)
			return;

		int cIndex = 0;

		for (auto c : data)
		{
			double thisUptime = uptime;

			for (auto& s : data.toChannelData(c))
			{
				s += getSample(thisUptime, cIndex);
				thisUptime += uptimeDelta;
			}

			cIndex++;
		}

		uptime += (double)data.getNumSamples() * uptimeDelta;

		updatePosition();
	}

	template <typename FrameDataType> void processFrame(FrameDataType& data) noexcept
	{
		SpinLock::ScopedLockType sl(lock);

		if (currentBuffer->clear)
			return;

		int index = 0;

		for (auto& s : data)
			s = getSample(uptime, index++);
			
		uptime += uptimeDelta;

		updatePosition();
	}

	void updatePosition();

private:

	float getSample(double pos, int channelIndex);

	double uptime = 0.0;
	double uptimeDelta = 1.0;
};



}

namespace core
{

//using file_player = file_node<file_player_impl, 2>;
using table = TableNode;

}




}
