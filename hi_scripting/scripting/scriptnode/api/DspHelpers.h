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

struct VoiceData
{
	bool operator==(const VoiceData& other) const noexcept
	{
		return voiceIndex == other.voiceIndex && noteOnEvent == other.noteOnEvent;
	}

	int voiceIndex = -1;
	HiseEvent noteOnEvent;
};




using PrepareSpecs = snex::Types::PrepareSpecs;

template <typename T, int NumVoices> struct PolyData
{
	PolyData(T initValue)
	{
		setAll(initValue);
	}

	PolyData()
	{
		
	}

	void prepare(PrepareSpecs sp)
	{
		jassert(!isPolyphonic() || sp.voiceIndex != nullptr);
		jassert(isPowerOfTwo(NumVoices));
		voicePtr = sp.voiceIndex;
	}

	static constexpr bool isPolyphonic() { return NumVoices > 1; }

	void setAll(const T& value)
	{
		if (!isPolyphonic() || voicePtr == nullptr)
		{
			*data = value;
		}
		else
		{
			for (int i = 0; i < NumVoices; i++)
				getWithIndex(i) = value;
		}
	}

	T& getMonoValue()
	{
		jassert(!isPolyphonic());
		return *data;
	}

	const T& getMonoValue() const
	{
		jassert(!isPolyphonic());
		return *data;
	}

	T& get()
	{
		if (!isPolyphonic() || voicePtr == nullptr)
			return *data;
		else
			return getWithIndex(getCurrentVoiceIndex());
	}

	const T& get() const
	{
		if (!isPolyphonic() || voicePtr == nullptr)
			return *data;
		else
			return getWithIndex(getCurrentVoiceIndex());
	}

#if 0
	void forEachVoice(const std::function<void(T& v)>& f)
	{
		if (!isPolyphonic() || voicePtr == nullptr)
			f(*data);
		else
		{
			for (int i = 0; i < NumVoices; i++)
				f(getWithIndex(i));
		}
	}
#endif

	T* begin() const
	{
		return const_cast<T*>(data);
	}

	T* end() const
	{
		if (!isPolyphonic() || voicePtr == nullptr)
			return const_cast<T*>(data) + 1;
		else
			return const_cast<T*>(data) + NumVoices;
	}

	bool isMonophonicOrInsideVoiceRendering() const
	{
		if (!isPolyphonic() || voicePtr == nullptr)
			return true;

		return isVoiceRenderingActive();
	}

	bool isVoiceRenderingActive() const
	{
		return isPolyphonic() && 
			   voicePtr != nullptr && *voicePtr != -1 && 
			   !MessageManager::getInstanceWithoutCreating()->currentThreadHasLockedMessageManager();
	}

	T& getCurrentOrFirst()
	{
		if (isVoiceRenderingActive())
			return get();
		else
			return getFirst();
	}

	const T& getCurrentOrFirst() const
	{
		if (isVoiceRenderingActive())
			return get();
		else
			return getFirst();
	}

	const T& getFirst() const
	{
		return *data;
	}

	T& getFirst()
	{
		return *data;
	}

	String getVoiceIndexForDebugging() const
	{
		String s;
		s << "VoiceIndex: ";
		s << (voicePtr != nullptr ? String(*voicePtr) : "inactive");
		return s;
	}

private:

	int getCurrentVoiceIndex() const
	{
		jassert(isPolyphonic());
		jassert(voicePtr != nullptr);
		jassert(isPositiveAndBelow(*voicePtr, NumVoices));
		return *voicePtr;
	}

	int getVoiceIndex(int index) const
	{
		auto rv = index & (NumVoices - 1);
		return rv;
	}

	T& getWithIndex(int index)
	{
		return *(data + getVoiceIndex(index));
	}

	const T& getWithIndex(int index) const
	{
		return *(data + getVoiceIndex(index));
	}

	T data[NumVoices];
	int* voicePtr = nullptr;
	
	
};

using ProcessData = snex::Types::ProcessDataDyn;

#if 0

/** A lightweight object containing the data. */
struct ProcessData
{
	ProcessData(float** d, int c, int s) :
		data(d),
		numChannels(c),
		size(s),
		eventBuffer(nullptr)
	{};

	ProcessData(const ProcessData& other):
		data(other.data),
		numChannels (other.numChannels),
		size(other.size),
		eventBuffer(other.eventBuffer)
	{}
	
	ProcessData() {};

	float** data = nullptr;
	int numChannels = 0;
	int size = 0;
	HiseEventBuffer* eventBuffer = nullptr;
	bool shouldReset = false;


	/** Iterates over the channels. */
	float** begin() const { return data; }
	float** end() const { return data + numChannels; }

	template <int NumChannels> void copyToFrame(float* frame) const
	{
		jassert(modifyPointersAllowed);
		jassert(isPositiveAndBelow(NumChannels, numChannels+1));

		for (int i = 0; i < NumChannels; i++)
			frame[i] = data[i][0];
	}

	template <int NumChannels> void copyFromFrameAndAdvance(const float* frame)
	{
		jassert(modifyPointersAllowed);
		jassert(isPositiveAndBelow(NumChannels, numChannels + 1));

		for (int i = 0; i < NumChannels; i++)
		{
			data[i][0] = frame[i];
			data[i] += 1;
		}
	}

	void copyToFrameDynamic(float* frame) const;

	void copyFromFrameAndAdvanceDynamic(const float* frame);

	void advanceChannelPointers(int sampleAmount=1);

	ProcessData copyToRawArray(float** channelData, float* uninitialisedData, bool clearArray=true);

	ProcessData copyTo(AudioSampleBuffer& buffer, int index);
	ProcessData& operator+=(const ProcessData& other);
	ProcessData referTo(AudioSampleBuffer& buffer, int index) const;

	void allowPointerModification()
	{
		modifyPointersAllowed = true;
	}

	bool modifyPointersAllowed = false;
};
#endif

struct PointerWatcher
{
#if 0 && JUCE_DEBUG
	PointerWatcher(ProcessData& data) :
		dataToWatch(data),
		c(data.numChannels),
		size(data.size)
	{
		memcpy(reinterpret_cast<void*>(d), data.data, data.numChannels * sizeof(float*));
	}

	~PointerWatcher()
	{
		bool sizeMatches = dataToWatch.size = size;
		bool channelMatches = dataToWatch.numChannels = c;

		// You've managed to change the channel amount or block size during processing
		jassert(sizeMatches && channelMatches);

		for (int i = 0; i < c; i++)
		{
			if (dataToWatch.data[i] != d[i])
			{
				// You've managed to change the pointers during processing.
				jassertfalse;
			}
		}
	}

	ProcessData& dataToWatch;
	int c;
	int size;
	float* d[NUM_MAX_CHANNELS];
#else
	PointerWatcher(ProcessData& d)
	{};
#endif
};

struct OscData
{
	void reset()
	{
		uptime = 0.0;
	}

	double tick()
	{
		auto rv = uptime;
		uptime += (uptimeDelta * multiplier);
		return rv;
	}

	double uptime = 0.0;
	double uptimeDelta = 0.0;
	double multiplier = 1.0;
};

struct ModValue
{
	bool getChangedValue(double& d)
	{
		if (changed)
		{
			changed = false;
			d = modValue;
			return true;
		}

		return false;
	}

	double getModValue() const noexcept { return modValue; }

	void setModValue(double newValue)
	{
		modValue = newValue;
		changed = true;
	}

	void setModValueIfChanged(double newValue)
	{
		if (modValue != newValue)
		{
			modValue = newValue;
			changed = true;
		}
	}

private:

	bool changed = false;
	double modValue = 0.0;
};

struct DspHelpers
{
	/** Increases the buffer size to match the process specs. */
	static void increaseBuffer(AudioSampleBuffer& b, const PrepareSpecs& ps);

	static void increaseBuffer(snex::Types::heap<float>& b, const PrepareSpecs& ps);

	using ConverterFunction = std::function<double(double)>;
	using ParameterCallback = std::function<void(double)>;

	struct ConverterFunctions
	{
		static double decibel2Gain(double input);
		static double gain2Decibel(double input);
		static double dryAmount(double input);
		static double wetAmount(double input);
		static double subtractFromOne(double input);

		static ConverterFunction getFunction(const Identifier& id);
	};

	static void setErrorIfFrameProcessing(const PrepareSpecs& ps);

	

	/** Returns a ParameterCallback with the given range. */
	static ParameterCallback getFunctionFrom0To1ForRange(NormalisableRange<double> range, bool inverted, const ParameterCallback& originalFunction);

	/** Wraps the ParameterCallback into a conversion function based on the converterId. */
	static ParameterCallback wrapIntoConversionLambda(const Identifier& converterId,
		const ParameterCallback& originalFunction,
		NormalisableRange<double> range,
		bool inverted);

	forcedinline static double findPeak(const float* data, int numSamples)
	{
		auto r = FloatVectorOperations::findMinAndMax(data, numSamples);
		return jmax<float>(std::abs(r.getStart()), std::abs(r.getEnd()));
	}

	static void validate(PrepareSpecs sp, PrepareSpecs rp);


	template <typename ProcessDataType> forcedinline static double findPeak(const ProcessDataType& data)
	{
		double max = 0.0;

		for (auto ch : data)
			max = jmax(max, findPeak(ch.getRawReadPointer(), data.getNumSamples()));

		return max;
	}

	template <typename DspClass, typename ProcessDataType> static forcedinline void forwardToFrameMono(DspClass* ptr, ProcessDataType& data)
	{
		using namespace snex::Types;
		ProcessDataHelpers<1>::processFix(ptr, data);
	}

	template <typename DspClass, typename ProcessDataType> static forcedinline void forwardToFrameStereo(DspClass* ptr, ProcessDataType& data)
	{
		using namespace snex::Types;

		switch (data.getNumChannels())
		{
		case 1:   ProcessDataHelpers<1>::processFix(ptr, data); break;
		case 2:   ProcessDataHelpers<2>::processFix(ptr, data); break;
		}
	}

	template <typename DspClass, typename FrameDataType> static forcedinline void forwardToFixFrame16(DspClass* ptr, FrameDataType& data)
	{
		using namespace snex::Types;

		jassert(Helpers::isRefArrayType<FrameDataType>(), "unneeded call to forwardToFrameFix");

		switch (data.size())
		{
		case 1:   ptr->processFrame(span<float, 1>::as(data.begin())); break;
		case 2:   ptr->processFrame(span<float, 2>::as(data.begin())); break;
		case 3:   ptr->processFrame(span<float, 3>::as(data.begin())); break;
		case 4:   ptr->processFrame(span<float, 4>::as(data.begin())); break;
		case 5:   ptr->processFrame(span<float, 5>::as(data.begin())); break;
		case 6:   ptr->processFrame(span<float, 6>::as(data.begin())); break;
		case 7:   ptr->processFrame(span<float, 7>::as(data.begin())); break;
		case 8:   ptr->processFrame(span<float, 8>::as(data.begin())); break;
		case 9:   ptr->processFrame(span<float, 9>::as(data.begin())); break;
		case 10:   ptr->processFrame(span<float, 10>::as(data.begin())); break;
		case 11:   ptr->processFrame(span<float, 11>::as(data.begin())); break;
		case 12:   ptr->processFrame(span<float, 12>::as(data.begin())); break;
		case 13:   ptr->processFrame(span<float, 13>::as(data.begin())); break;
		case 14:   ptr->processFrame(span<float, 14>::as(data.begin())); break;
		case 15:   ptr->processFrame(span<float, 15>::as(data.begin())); break;
		case 16:   ptr->processFrame(span<float, 16>::as(data.begin())); break;
		}
	}

	template <typename DspClass, typename ProcessDataType> static forcedinline void forwardToFrame16(DspClass* ptr, ProcessDataType& data)
	{
		using namespace snex::Types;

		switch (data.getNumChannels())
		{
		case 1:   ProcessDataHelpers<1>::processFix(ptr, data); break;
		case 2:   ProcessDataHelpers<2>::processFix(ptr, data); break;
		case 3:   ProcessDataHelpers<3>::processFix(ptr, data); break;
		case 4:   ProcessDataHelpers<4>::processFix(ptr, data); break;
		case 5:   ProcessDataHelpers<5>::processFix(ptr, data); break;
		case 6:   ProcessDataHelpers<6>::processFix(ptr, data); break;
		case 7:   ProcessDataHelpers<7>::processFix(ptr, data); break;
		case 8:   ProcessDataHelpers<8>::processFix(ptr, data); break;
		case 9:   ProcessDataHelpers<9>::processFix(ptr, data); break;
		case 10: ProcessDataHelpers<10>::processFix(ptr, data); break;
		case 11: ProcessDataHelpers<11>::processFix(ptr, data); break;
		case 12: ProcessDataHelpers<12>::processFix(ptr, data); break;
		case 13: ProcessDataHelpers<13>::processFix(ptr, data); break;
		case 14: ProcessDataHelpers<14>::processFix(ptr, data); break;
		case 15: ProcessDataHelpers<15>::processFix(ptr, data); break;
		case 16: ProcessDataHelpers<16>::processFix(ptr, data); break;
		}
	}

};





struct CodeHelpers
{
	

	static void setIncludeDirectory(String filePath);

	static File getIncludeDirectory();

	static void initCustomCodeFolder(Processor* p);

	static void addFileToCustomFolder(const String& filename, const String& content);

	static void addFileToProjectFolder(const String& filename, const String& content);

	static String createIncludeFile(File targetDirectory);

	static bool customFolderIsDefined();

	static bool projectFolderIsDefined();

private:

	static void addFileInternal(const String& filename, const String& content, File targetDirectory);

	static File includeDirectory;
	static File projectIncludeDirectory;
};


}
