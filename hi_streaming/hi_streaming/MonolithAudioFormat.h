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


#ifndef MONOLITHAUDIOFORMAT_H_INCLUDED
#define MONOLITHAUDIOFORMAT_H_INCLUDED

namespace hise { using namespace juce;

#define USE_OLD_MONOLITH_FORMAT 0

#if JUCE_64BIT && !JUCE_IOS
#define USE_FALLBACK_READERS_FOR_MONOLITH 0
#else
#define USE_FALLBACK_READERS_FOR_MONOLITH 1
#endif

#if USE_OLD_MONOLITH_FORMAT // Keep it around for a while

class MonolithAudioFormatReader : public MemoryMappedAudioFormatReader
{
public:


	MonolithAudioFormatReader(const File &f, AudioFormatReader &details, int64 start, int64 length, bool isMono):
		MemoryMappedAudioFormatReader(f, details, start, length, (isMono ? 1 : 2) * sizeof(int16)),
		numChannels(isMono ? 1 : 2)
	{
        usesFloatingPointData = true;
    }

	bool readSamples(int **destSamples, int numDestChannels, int startOffsetInDestBuffer, int64 startSampleInFile, int numSamples) override
	{
		clearSamplesBeyondAvailableLength (destSamples, numDestChannels, startOffsetInDestBuffer,
			startSampleInFile, numSamples, lengthInSamples);

		if (map == nullptr || ! mappedSection.contains (Range<int64> (startSampleInFile, startSampleInFile + numSamples)))
		{
			jassertfalse; // you must make sure that the window contains all the samples you're going to attempt to read.
			return false;
		}

		copySampleData (destSamples, startOffsetInDestBuffer, numDestChannels, sampleToPointer (startSampleInFile), numChannels, numSamples);

		return true;
	}


	void getSample(int64 sample, float* result) const noexcept override
	{
		if (map == nullptr || !mappedSection.contains(sample))
		{
			jassertfalse; // you must make sure that the window contains all the samples you're going to attempt to read.

			zeromem(result, sizeof(float) * (size_t)2);
			return;
		}

		float** dest = &result;
		const void* source = sampleToPointer(sample);

		ReadHelper<AudioData::Float32, AudioData::Int16, AudioData::LittleEndian>::read(dest, 0, 1, source, 2, 1);	
	}

	static void copySampleData(int* const* destSamples, int startOffsetInDestBuffer, int numDestChannels, const void* sourceData, int numChannels, int numSamples) noexcept
	{
		jassert(numDestChannels == numDestChannels);

		if (numChannels == 1)
		{
            ReadHelper<AudioData::Float32, AudioData::Int16, AudioData::LittleEndian>::read(destSamples, startOffsetInDestBuffer, 1, sourceData, 1, numSamples);
			//FloatVectorOperations::copy(reinterpret_cast<float*>(destSamples[1]), reinterpret_cast<float*>(destSamples[0]), numSamples);
		}
		else
		{
			ReadHelper<AudioData::Float32, AudioData::Int16, AudioData::LittleEndian>::read(destSamples, startOffsetInDestBuffer, numDestChannels, sourceData, 2, numSamples);
		}
	}

private:

	const int numChannels;
};


class FallbackMonolithAudioFormatReader: public AudioFormatReader
{
public:
    
    FallbackMonolithAudioFormatReader(InputStream* const input, bool isMono):
      AudioFormatReader(input, "HISE")
    {
        numChannels = isMono ? 1 : 2;
        usesFloatingPointData = true;
        
        const int bytesPerFrame = sizeof(int16) * numChannels;
        
        lengthInSamples = (input->getTotalLength() - 1) / bytesPerFrame;
    };
    
    //==============================================================================
    bool readSamples (int** destSamples, int numDestChannels, int startOffsetInDestBuffer,
                      int64 startSampleInFile, int numSamples) override
    {
        clearSamplesBeyondAvailableLength (destSamples, numDestChannels, startOffsetInDestBuffer,
                                           startSampleInFile, numSamples, lengthInSamples);
        
        if (numSamples <= 0)
            return true;
        
        const int bytesPerFrame = sizeof(int16) * numChannels;
        
        input->setPosition (1 + startSampleInFile * bytesPerFrame);
        
        while (numSamples > 0)
        {
            const int tempBufSize = 480 * 3 * 4; // (keep this a multiple of 3)
            char tempBuffer [tempBufSize];
            
            const int numThisTime = jmin (tempBufSize / bytesPerFrame, numSamples);
            const int bytesRead = input->read (tempBuffer, numThisTime * bytesPerFrame);
            
            if (bytesRead < numThisTime * bytesPerFrame)
            {
                jassert (bytesRead >= 0);
                zeromem (tempBuffer + bytesRead, (size_t) (numThisTime * bytesPerFrame - bytesRead));
            }
            
            copySampleData (destSamples, startOffsetInDestBuffer, numDestChannels,
                            tempBuffer, (int) numChannels, numThisTime);
            
            startOffsetInDestBuffer += numThisTime;
            numSamples -= numThisTime;
        }
        
        return true;
    }
    
    static void copySampleData(int* const* destSamples, int startOffsetInDestBuffer, int numDestChannels, const void* sourceData, int numChannels, int numSamples) noexcept
    {
        if (numChannels == 1)
        {
            ReadHelper<AudioData::Float32, AudioData::Int16, AudioData::LittleEndian>::read(destSamples, startOffsetInDestBuffer, 1, sourceData, 1, numSamples);
            
            if(numDestChannels == 2)
            {
                FloatVectorOperations::copy(reinterpret_cast<float*>(destSamples[1]), reinterpret_cast<float*>(destSamples[0]), numSamples);
            }
        }
        else
        {
            ReadHelper<AudioData::Float32, AudioData::Int16, AudioData::LittleEndian>::read(destSamples, startOffsetInDestBuffer, numDestChannels, sourceData, 2, numSamples);
        }
    }
};

class HiseMonolithAudioFormat: public AudioFormat,
							   public ReferenceCountedObject
{
public:

	HiseMonolithAudioFormat(const Array<File>& monoFiles_):
		AudioFormat("HISE Monolithic Sample Format", "ch1 ch2 ch3 ch4 ch5 ch6")
    
	{
		monolithicFiles.reserve(monoFiles_.size());
		

		for (int i = 0; i < monoFiles_.size(); i++)
		{
			FileInputStream fis(monoFiles_[i]);
			isMonoChannel[i] = fis.readBool();
			monolithicFiles.push_back(monoFiles_[i]);

            ScopedPointer<FileInputStream> fallbackStream = new FileInputStream(monoFiles_[i]);
            fallbackReaders.add(new FallbackMonolithAudioFormatReader(fallbackStream.release(), isMonoChannel[i]));
		}

		dummyReader.numChannels = 2;
		dummyReader.bitsPerSample = 16;
	}

	void fillMetadataInfo(const ValueTree &sampleMap);

	Array<int> getPossibleSampleRates() override
	{
		Array<int> a;
		a.ensureStorageAllocated(5);

		a.add(22050);
		a.add(44100);
		a.add(48000);
		a.add(88200);
		a.add(96000);

		return a;
	}


	Array<int> getPossibleBitDepths() override
	{
		Array<int> a;
		a.ensureStorageAllocated(3);

		a.add(16);
		a.add(24);
		a.add(32);

		return a;
	}

	bool canDoMono() override { return true; };

	bool canDoStereo() override { return true; }

	MemoryMappedAudioFormatReader* createMemoryMappedReader(const File &/*embeddedFile*/) override
	{
		jassertfalse;


		return nullptr;
	}

	AudioFormatReader* createMonolithicReader(int sampleIndex, int channelIndex)
	{
		const int sizeOfFirstChannelList = (int)multiChannelSampleInformation[0].size();
		const int sizeOfChannelList = (int)multiChannelSampleInformation.size();

		if (channelIndex < sizeOfChannelList && sizeOfFirstChannelList > 0 && sampleIndex < sizeOfFirstChannelList)
		{
			auto info = &multiChannelSampleInformation[channelIndex][sampleIndex];

			const int64 start = info->start;
			const int64 length = info->length;

			return new AudioSubsectionReader(memoryReaders[channelIndex], start, length, false);
		}
		
		return nullptr;
	}
    
    AudioFormatReader* createFallbackReader(int sampleIndex, int channelIndex)
    {
        const int sizeOfFirstChannelList = (int)multiChannelSampleInformation[0].size();
        const int sizeOfChannelList = (int)multiChannelSampleInformation.size();
        
        if (channelIndex < sizeOfChannelList && sizeOfFirstChannelList > 0 && sampleIndex < sizeOfFirstChannelList)
        {
            auto info = &multiChannelSampleInformation[channelIndex][sampleIndex];
            
            const int64 start = info->start;
            const int64 length = info->length;
            
            fallbackReaders[channelIndex]->sampleRate = info->sampleRate;
            
            return new AudioSubsectionReader(fallbackReaders[channelIndex], start, length, false);
        }
        
        return nullptr;
    }

	AudioFormatWriter* createWriterFor(OutputStream* /*streamToWriteTo*/, double /*sampleRateToUse*/, unsigned int /*numberOfChannels*/, int /*bitsPerSample*/, const StringPairArray& /*metadataValues*/, int /*qualityOptionIndex*/)
	{
		return nullptr;
	}

	AudioFormatReader* createReaderFor(InputStream* /*sourceStream*/, bool /*deleteStreamIfOpeningFails*/)
	{
		return nullptr;
	}

	String getFileName(int channelIndex, int sampleIndex) const
	{
		return multiChannelSampleInformation[channelIndex][sampleIndex].fileName;
	}
    
    int64 getMonolithOffset(int sampleIndex) const
    {
        return multiChannelSampleInformation[0][sampleIndex].start;
    }

    int64 getMonolithLength(int sampleIndex) const
    {
        return multiChannelSampleInformation[0][sampleIndex].length;
    }
    
    double getMonolithSampleRate(int sampleIndex) const
    {
        return multiChannelSampleInformation[0][sampleIndex].sampleRate;
    }
    
	struct SampleInfo
	{
		double sampleRate;
		int64 length;
		int64 start;
		String fileName;
	};

	private:

	struct DummyReader: public AudioFormatReader
	{
	public:

		DummyReader() :
			AudioFormatReader(nullptr, "HISE Monolith")
		{};

		bool readSamples(int ** /*destSamples*/, int /*numDestChannels*/, int /*startOffsetInDestBuffer*/, int64 /*startSampleInFile*/, int /*numSamples*/) override
		{
			jassertfalse;
			return false;
		}

	};

	DummyReader dummyReader;
	
	std::vector<std::vector<SampleInfo>> multiChannelSampleInformation;

	std::vector<File> monolithicFiles;

	bool isMonoChannel[6];
    
    OwnedArray<FallbackMonolithAudioFormatReader> fallbackReaders;

	OwnedArray<MonolithAudioFormatReader> memoryReaders;
};

typedef HiseMonolithAudioFormat MonolithInfoToUse;

#else

struct HlacMonolithInfo : public ReferenceCountedObject
{
public:

	HlacMonolithInfo(const Array<File>& monolithicFiles_)
	{
		monolithicFiles.reserve(monolithicFiles_.size());


		for (int i = 0; i < monolithicFiles_.size(); i++)
		{
			FileInputStream fis(monolithicFiles_[i]);

			monolithicFiles.push_back(monolithicFiles_[i]);

			ScopedPointer<FileInputStream> fallbackStream = new FileInputStream(monolithicFiles_[i]);
			fallbackReaders.add(new hlac::HiseLosslessAudioFormatReader(fallbackStream.release()));
			isMonoChannel[i] = fallbackReaders.getLast()->numChannels == 1;
		}

		dummyReader.numChannels = 2;
		dummyReader.bitsPerSample = 16;
	}

	void fillMetadataInfo(const ValueTree& sampleMap);

	String getFileName(int channelIndex, int sampleIndex) const
	{
		return multiChannelSampleInformation[channelIndex][sampleIndex].fileName;
	}

	int64 getMonolithOffset(int sampleIndex) const
	{
		return multiChannelSampleInformation[0][sampleIndex].start;
	}

	int64 getMonolithLength(int sampleIndex) const
	{
		return multiChannelSampleInformation[0][sampleIndex].length;
	}

	double getMonolithSampleRate(int sampleIndex) const
	{
		return multiChannelSampleInformation[0][sampleIndex].sampleRate;
	}

	AudioFormatReader* createMonolithicReader(int sampleIndex, int channelIndex)
	{
		const int sizeOfFirstChannelList = (int)multiChannelSampleInformation[0].size();
		const int sizeOfChannelList = (int)multiChannelSampleInformation.size();

		if (channelIndex < sizeOfChannelList && sizeOfFirstChannelList > 0 && sampleIndex < sizeOfFirstChannelList)
		{
			auto info = &multiChannelSampleInformation[channelIndex][sampleIndex];

			const int64 start = info->start;
			const int64 length = info->length;

			return new hlac::HlacSubSectionReader(memoryReaders[channelIndex], start, length);
		}

		return nullptr;
	}

	AudioFormatReader* createFallbackReader(int sampleIndex, int channelIndex)
	{
		const int sizeOfFirstChannelList = (int)multiChannelSampleInformation[0].size();
		const int sizeOfChannelList = (int)multiChannelSampleInformation.size();

		if (channelIndex < sizeOfChannelList && sizeOfFirstChannelList > 0 && sampleIndex < sizeOfFirstChannelList)
		{
			auto info = &multiChannelSampleInformation[channelIndex][sampleIndex];

			const int64 start = info->start;
			const int64 length = info->length;

			fallbackReaders[channelIndex]->sampleRate = info->sampleRate;

			return new hlac::HlacSubSectionReader(fallbackReaders[channelIndex], start, length);
		}

		return nullptr;
	}

	/** Use this for UI rendering stuff to avoid multithreading issues. */
	AudioFormatReader* createThumbnailReader(int sampleIndex, int channelIndex)
	{
		const int sizeOfFirstChannelList = (int)multiChannelSampleInformation[0].size();
		const int sizeOfChannelList = (int)multiChannelSampleInformation.size();

		if (channelIndex < sizeOfChannelList && sizeOfFirstChannelList > 0 && sampleIndex < sizeOfFirstChannelList)
		{
			auto info = &multiChannelSampleInformation[channelIndex][sampleIndex];

			const int64 start = info->start;
			const int64 length = info->length;

			ScopedPointer<FileInputStream> fallbackStream = new FileInputStream(monolithicFiles[channelIndex]);
			
			ScopedPointer<hlac::HiseLosslessAudioFormatReader> thumbnailReader = new hlac::HiseLosslessAudioFormatReader(fallbackStream.release());

			thumbnailReader->setTargetAudioDataType(AudioDataConverters::float32BE);

			thumbnailReader->sampleRate = info->sampleRate;

			return new AudioSubsectionReader(thumbnailReader.release(), start, length, true);

		}

		return nullptr;
	
	}

	struct SampleInfo
	{
		double sampleRate;
		int64 length;
		int64 start;
		String fileName;
	};

	typedef ReferenceCountedObjectPtr<HlacMonolithInfo> Ptr;

private:

	struct DummyReader : public AudioFormatReader
	{
	public:

		DummyReader() :
			AudioFormatReader(nullptr, "HISE Monolith")
		{};

		bool readSamples(int ** /*destSamples*/, int /*numDestChannels*/, int /*startOffsetInDestBuffer*/, int64 /*startSampleInFile*/, int /*numSamples*/) override
		{
			jassertfalse;
			return false;
		}

	};

	hlac::HiseLosslessAudioFormat hlaf;

	DummyReader dummyReader;

	std::vector<std::vector<SampleInfo>> multiChannelSampleInformation;

	std::vector<File> monolithicFiles;

	bool isMonoChannel[6];

	OwnedArray<hlac::HiseLosslessAudioFormatReader> fallbackReaders;

	OwnedArray<hlac::HlacMemoryMappedAudioFormatReader> memoryReaders;

	

};

typedef HlacMonolithInfo MonolithInfoToUse ;
#endif

} // namespace hise
#endif  // MONOLITHAUDIOFORMAT_H_INCLUDED
