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



namespace core
{


#if 0
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
            obj.process(data.template as<ProcessData<C>>());
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
            jassertfalse;
#if 0
			int numSamples = file.data[0].size();

			for (int i = 0; i < C; i++)
				displayBuffer.copyFrom(i, 0, file.data[i].begin(), numSamples);

			
			writeFlag.store(true);
			ScopedLock sl(fileLock);
			obj.setFile(file.data, file.sampleRate);
			writeFlag.store(false);
#endif
		}
	};

	properties::file<PropClass, C> fileProperty;
};
#endif

struct file_player : public data::base
{
	using IndexType = index::normalised<float, index::wrapped<0, true>>;
	using InterpolatorType = index::lerp<IndexType>;

	SET_HISE_NODE_ID("file_player");
	SN_GET_SELF_AS_OBJECT(file_player);

	static constexpr bool isPolyphonic() { return false; }
	static constexpr bool isNormalisedModulation() { return false; }

	HISE_EMPTY_HANDLE_EVENT;
	HISE_EMPTY_CREATE_PARAM;
	HISE_EMPTY_INITIALISE;
	
	file_player();;

	bool handleModulation(double& v) 
	{ 
		return lastLength.getChangedValue(v);
	}

	void prepare(PrepareSpecs specs);
	void reset();

	void setExternalData(const snex::ExternalData& d, int index) override
	{
		DataWriteLock l(this);

		base::setExternalData(d, index);

		d.referBlockTo(currentData[0], 0);
		d.referBlockTo(currentData[1], 1);

		if (lastSpecs.sampleRate > 0.0)
			lastLength.setModValueIfChanged((double)d.numSamples / lastSpecs.sampleRate * 1000.0);

		prepare(lastSpecs);
		reset();
	}

	span<dyn<float>, 2> currentData;

	template <typename ProcessDataType> void process(ProcessDataType& data) noexcept
	{
		DataReadLock l(this);

		if (!externalData.isEmpty())
		{
			auto pos = data[0][0];

			if ((data.getNumChannels() == 1 && externalData.numChannels > 0) ||
				externalData.numChannels == 1)
			{
				FrameConverters::forwardToFrameMono(this, data);
			}

			IndexType p(data[0][0]);

			externalData.setDisplayedValue(p.getIndex(externalData.numSamples));

			if (data.getNumChannels() >= 2 && externalData.numChannels >= 2)
			{
				FrameConverters::forwardToFrameStereo(this, data);
			}
		}
	}

	template <typename FrameDataType> void processFrame(FrameDataType& data) noexcept
	{
		DataReadLock l(this);

		InterpolatorType ip(data[0]);

		for (int i = 0; i < data.size(); i++)
			data[i] = currentData[i].interpolate(ip);
	}

private:

	ModValue lastLength;
	PrepareSpecs lastSpecs;
};



}

namespace core
{


}




}
