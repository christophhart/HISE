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

#if USE_OLD_MONOLITH_FORMAT

void HiseMonolithAudioFormat::fillMetadataInfo(const juce::ValueTree &sampleMap)
{
	int numChannels = sampleMap.getChild(0).getNumChildren();
	if (numChannels == 0) numChannels = 1;

	multiChannelSampleInformation.reserve(numChannels);

	for (int i = 0; i < numChannels; i++)
	{
		std::vector<SampleInfo> newVector;
		newVector.reserve(sampleMap.getNumChildren());
		multiChannelSampleInformation.push_back(newVector);
	}


	for (int i = 0; i < sampleMap.getNumChildren(); i++)
	{
		ValueTree sample = sampleMap.getChild(i);

		if (numChannels == 1)
		{
			SampleInfo info;

			info.start = sample.getProperty("MonolithOffset");
			info.length = sample.getProperty("MonolithLength");
			info.sampleRate = sample.getProperty("SampleRate");
			info.fileName = sample.getProperty("FileName");

			multiChannelSampleInformation[0].push_back(info);
		}
		else
		{
			for (int channel = 0; channel < numChannels; channel++)
			{
				SampleInfo info;

				info.start = sample.getProperty("MonolithOffset");
				info.length = sample.getProperty("MonolithLength");
				info.sampleRate = sample.getProperty("SampleRate");
				info.fileName = sample.getChild(channel).getProperty("FileName");

				multiChannelSampleInformation[channel].push_back(info);
			}
		}
	}

	for (int i = 0; i < numChannels; i++)
	{
		dummyReader.numChannels = isMonoChannel[i] ? 1 : 2;
		dummyReader.sampleRate = multiChannelSampleInformation[i][0].sampleRate;

		const int bytesPerFrame = sizeof(int16) * dummyReader.numChannels;
		FileInputStream fis(monolithicFiles[i]);
		dummyReader.lengthInSamples = (fis.getTotalLength() - 1) / bytesPerFrame;

		ScopedPointer<MonolithAudioFormatReader> reader = new MonolithAudioFormatReader(monolithicFiles[i], dummyReader, 1, fis.getTotalLength() - 1, isMonoChannel[i]);
		
#if !USE_FALLBACK_READERS_FOR_MONOLITH
		reader->mapEntireFile();

		memoryReaders.add(reader.release());

		if (memoryReaders.getLast()->getMappedSection().isEmpty())
		{
			jassertfalse;
			throw StreamingSamplerSound::LoadingError(monolithicFiles[i].getFileName(), "Error at memory mapping");
		}
#endif
	}
}

#else

void HlacMonolithInfo::fillMetadataInfo(const juce::ValueTree& sampleMap)
{
	int numChannels = sampleMap.getChild(0).getNumChildren();
	if (numChannels == 0) numChannels = 1;

	int numSplitFiles = (int)sampleMap.getProperty("MonolithSplitAmount", 0);

	numChannels = std::max(numSplitFiles, numChannels);

	multiChannelSampleInformation.reserve(numChannels);

	for (int i = 0; i < numChannels; i++)
	{
		std::vector<SampleInfo> newVector;
		newVector.reserve(sampleMap.getNumChildren());
		multiChannelSampleInformation.push_back(newVector);
	}


	for (int i = 0; i < sampleMap.getNumChildren(); i++)
	{
        auto sample = sampleMap.getChild(i);

		if (!sample.hasProperty("MonolithLength") || !sample.hasProperty("MonolithOffset"))
		{
			throw StreamingSamplerSound::LoadingError(sample.getProperty("FileName").toString(), "\nhas no monolith metadata (probably an export error)");
		}

		if (numChannels == 1)
		{
			SampleInfo info;

			info.start = sample.getProperty("MonolithOffset");
			info.length = sample.getProperty("MonolithLength");
			info.sampleRate = sample.getProperty("SampleRate");
			info.fileName = sample.getProperty("FileName");

			int splitIndex = sample.getProperty("MonolithSplitIndex", 0);

			multiChannelSampleInformation[splitIndex].push_back(info);
		}
		else
		{
			for (int channel = 0; channel < numChannels; channel++)
			{
				SampleInfo info;

				info.start = sample.getProperty("MonolithOffset");
				info.length = sample.getProperty("MonolithLength");
				info.sampleRate = sample.getProperty("SampleRate");
				info.fileName = sample.getChild(channel).getProperty("FileName");

				multiChannelSampleInformation[channel].push_back(info);
			}
		}
	}

	for (size_t i = 0; i < (size_t)numChannels; i++)
	{
		dummyReader.numChannels = isMonoChannel[i] ? 1 : 2;
        
        if(multiChannelSampleInformation.size() < i)
        {
            jassertfalse;
            dummyReader.sampleRate = 44100.0;
        }
        else if(multiChannelSampleInformation[i].size() <= 0)
        {
            jassertfalse;
            dummyReader.sampleRate = 44100.0;
        }
        else
            dummyReader.sampleRate = multiChannelSampleInformation[i][0].sampleRate;

		const int bytesPerFrame = sizeof(int16_t) * dummyReader.numChannels;
        juce::FileInputStream fis(monolithicFiles[i]);
        
        if(fis.getTotalLength() == 0)
        {
            jassertfalse;
            throw StreamingSamplerSound::LoadingError(monolithicFiles[i].getFileName(), "File is corrupt");
        }
        
        
		dummyReader.lengthInSamples = (fis.getTotalLength() - 1) / bytesPerFrame;

        juce::ScopedPointer<juce::MemoryMappedAudioFormatReader> reader = hlaf.createMemoryMappedReader(monolithicFiles[i]);

		

#if !USE_FALLBACK_READERS_FOR_MONOLITH
		reader->mapEntireFile();

		memoryReaders.add(dynamic_cast<hlac::HlacMemoryMappedAudioFormatReader*>(reader.release()));

		memoryReaders.getLast()->setTargetAudioDataType(juce::AudioDataConverters::DataFormat::int16BE);

		if (memoryReaders.getLast()->getMappedSection().isEmpty())
		{
			jassertfalse;
			throw StreamingSamplerSound::LoadingError(monolithicFiles[i].getFileName(), "Error at memory mapping");
		}
#endif
	}
}

#endif

} // namespace hise
