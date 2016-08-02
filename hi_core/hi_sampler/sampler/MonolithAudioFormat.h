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


#ifndef MONOLITHAUDIOFORMAT_H_INCLUDED
#define MONOLITHAUDIOFORMAT_H_INCLUDED

class MonolithAudioFormatReader : public MemoryMappedAudioFormatReader
{
public:

	class MetadataReader : public AudioFormatReader
	{
	public:

		MetadataReader(const File& f):
			AudioFormatReader(nullptr, "HISE Monolith")
		{
			FileInputStream fis(f);

			sizeOffset = fis.readInt64BigEndian();

			MemoryBlock mb;

			mb.setSize(sizeOffset);

			fis.read(mb.getData(), mb.getSize());

			ValueTree metadata = ValueTree::readFromData(mb.getData(), mb.getSize());

			for (int i = 0; i < metadata.getNumChildren(); i++)
			{
				ValueTree sample = metadata.getChild(i);

				SampleInfo info;

				info.start = sample.getProperty("offset");
				info.length = sample.getProperty("length");
				info.sampleRate = sample.getProperty("rate");
				info.fileName = sample.getProperty("file");

				sampleInformation.add(info);
			}

		};

		bool readSamples(int **destSamples, int numDestChannels, int startOffsetInDestBuffer, int64 startSampleInFile, int numSamples) override
		{
			jassertfalse;
			return false;
		}

		struct SampleInfo
		{
			double sampleRate;
			int64 length;
			int64 start;
			String fileName;
		};

		Array<SampleInfo> sampleInformation;

		int64 sizeOffset;
	};

	MonolithAudioFormatReader(const File &f, AudioFormatReader &details, int64 start, int64 length):
		MemoryMappedAudioFormatReader(f, details, start, length, 4)
	{}

	bool readSamples(int **destSamples, int numDestChannels, int startOffsetInDestBuffer, int64 startSampleInFile, int numSamples) override
	{
		clearSamplesBeyondAvailableLength (destSamples, numDestChannels, startOffsetInDestBuffer,
			startSampleInFile, numSamples, lengthInSamples);

		if (map == nullptr || ! mappedSection.contains (Range<int64> (startSampleInFile, startSampleInFile + numSamples)))
		{
			jassertfalse; // you must make sure that the window contains all the samples you're going to attempt to read.
			return false;
		}

		copySampleData (destSamples, startOffsetInDestBuffer, numDestChannels, sampleToPointer (startSampleInFile), numSamples);
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

		ReadHelper<AudioData::Float32, AudioData::Int16, AudioData::LittleEndian>::read(dest, 0, 1, source, 1, 2);	
	}

	static void copySampleData(int* const* destSamples, int startOffsetInDestBuffer, int numDestChannels, const void* sourceData, int numSamples) noexcept
	{
		ReadHelper<AudioData::Int32, AudioData::Int16, AudioData::LittleEndian>::read(destSamples, startOffsetInDestBuffer, numDestChannels, sourceData, 2, numSamples);
	}

};


class HiseMonolithAudioFormat: public AudioFormat
{
public:

	HiseMonolithAudioFormat(const File& monoFile_):
		AudioFormat("HISE Monolithic Sample Format", "ch1 ch2 ch3 ch4 ch5 ch6"),
		metaDataReader(monoFile_),
		monoFile(monoFile_)
	{
		metaDataReader.numChannels = 2;
		metaDataReader.metadataValues = StringPairArray();
		metaDataReader.bitsPerSample = 16;
		metaDataReader.usesFloatingPointData = false;
	}

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

	MemoryMappedAudioFormatReader* createMemoryMappedReader(const File &embeddedFile) override
	{
		jassertfalse;


		return nullptr;
	}

	

	MemoryMappedAudioFormatReader* createMonolithicReader(int sampleIndex)
	{
		if (sampleIndex < metaDataReader.sampleInformation.size())
		{
			const int64 start = metaDataReader.sampleInformation[sampleIndex].start;
			const int64 length = metaDataReader.sampleInformation[sampleIndex].length;

			metaDataReader.sampleRate = metaDataReader.sampleInformation[sampleIndex].sampleRate;
			metaDataReader.lengthInSamples = length;

			return new MonolithAudioFormatReader(monoFile, metaDataReader, sizeof(int64) + 4 * start + metaDataReader.sizeOffset, 4 * length);
		}
		
		return nullptr;
	}

	AudioFormatWriter* createWriterFor(OutputStream* streamToWriteTo, double sampleRateToUse, unsigned int numberOfChannels, int bitsPerSample, const StringPairArray& metadataValues, int qualityOptionIndex)
	{
		return nullptr;
	}

	AudioFormatReader* createReaderFor(InputStream* sourceStream, bool deleteStreamIfOpeningFails)
	{
		return nullptr;
	}

	String getFileName(int sampleIndex)
	{
		return metaDataReader.sampleInformation[sampleIndex].fileName;
	}

	MonolithAudioFormatReader::MetadataReader metaDataReader;
	
	File monoFile;
};

#endif  // MONOLITHAUDIOFORMAT_H_INCLUDED
