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

String MonolithFileReference::getFileExtensionPrefix()
{
	// Change this for the ultimate HISE customization: Make your own file format!
	// Just change it here and you have created your own personal file format that
	// everybody will love! WOW. So much professional!
	return String("ch");
}

String MonolithFileReference::getIdFromValueTree(const ValueTree& v)
{
	if (v.hasProperty(MonolithIds::MonolithReference))
		return v[MonolithIds::MonolithReference].toString();

	return v["ID"].toString();
}

juce::juce_wchar MonolithFileReference::getCharForSplitPart(int partIndex)
{
	if (partIndex == -1)
	{
		jassertfalse;
		return 0;
	}

	return (juce_wchar)jlimit(0, 26, partIndex) + 'a';
}

int MonolithFileReference::getSplitPartFromChar(juce_wchar splitChar)
{
	return (int)splitChar - 'a';
}

juce::File MonolithFileReference::getFile(bool checkIfFileExists)
{
	jassert(referenceString.isNotEmpty());

	auto path = referenceString.replace("/", "_");

	auto extension = getFileExtensionPrefix();

	if (sampleRoots.isEmpty() && fileNotFoundBehaviour == FileNotFoundBehaviour::ThrowException)
		throw Result::fail("No sample directory specified");

	if (isMultimic())
	{
		extension << String(channelIndex + 1);

		if (useSplitIndex())
			extension << getCharForSplitPart(partIndex);
	}
	else if (useSplitIndex())
	{
		extension << String(partIndex + 1);	
	}
	else
		extension << String(1);

	File lastFile;
	path << "." << extension;

	for (const auto& f : sampleRoots)
	{
		auto mf = f.getChildFile(path);

		lastFile = mf;

		if (!checkIfFileExists || mf.existsAsFile())
			return mf;
	}

	if (fileNotFoundBehaviour == FileNotFoundBehaviour::ThrowException)
		throw Result::fail(lastFile.getFullPathName() + " can't be found");

	return File();
}

Array<juce::File> MonolithFileReference::getAllFiles()
{
	channelIndex = 0;
	partIndex = 0;
	
	Array<File> filesToLoad;

	filesToLoad.addIfNotAlreadyThere(getFile(true));

	while (bumpToNextMonolith(true))
	{
		filesToLoad.addIfNotAlreadyThere(getFile(true));
	}

	int numExpected = numChannels * jmax(1, numParts);
    ignoreUnused(numExpected);
	jassert(filesToLoad.size() == numExpected);

	return filesToLoad;
}

bool MonolithFileReference::bumpToNextMonolith(bool allowChannelBump)
{
	if (useSplitIndex())
	{
		if (isPositiveAndBelow(partIndex, numParts-1))
		{
			partIndex++;
			return true;
		}

		if (!isMultimic() || !allowChannelBump)
			return false;

		partIndex = 0;
	}
	
	if (!allowChannelBump)
		return false;

	if (isPositiveAndBelow(channelIndex, numChannels - 1))
	{
		channelIndex++;
		return true;
	}

	return false;
}

void MonolithFileReference::setNumSplitPartsToCurrentIndex()
{
	jassert(numParts == INT_MAX);

	if (partIndex == 0)
		numParts = 0;
	else
		numParts = partIndex + 1;
}

MonolithFileReference::MonolithFileReference(const File& monolithFile, int numChannels_, int numParts_):
	numChannels(numChannels_),
	numParts(numParts_)
{
	auto extension = monolithFile.getFileExtension().substring(1);
	jassert(extension.startsWith(getFileExtensionPrefix()));

	sampleRoots.add(monolithFile.getParentDirectory());
	referenceString = monolithFile.getFileNameWithoutExtension();

	if(isMultimic())
		channelIndex = jlimit(0, 15, extension.fromFirstOccurrenceOf(getFileExtensionPrefix(), false, false).getIntValue() - 1);

	if (useSplitIndex())
	{
		auto lastChar = extension.getLastCharacter();

		jassert(CharacterFunctions::isLetter(lastChar));
		partIndex = getSplitPartFromChar(lastChar);
	}
}

MonolithFileReference::MonolithFileReference(int numChannels_, int numParts_) : 
	numChannels(jmax(1, numChannels_)), 
	numParts(numParts_)
{

}

MonolithFileReference::MonolithFileReference(const ValueTree& v)
{
	numChannels = jmax(1, v.getChild(0).getNumChildren());
	numParts = v.getProperty(MonolithIds::MonolithSplitAmount, 0);
	referenceString = getIdFromValueTree(v);
	isMonolith = (int)v.getProperty("SaveMode") == 2;
}

bool MonolithFileReference::isUsingMonolith() const
{
	return isMonolith;
}

HlacMonolithInfo::HlacMonolithInfo(const Array<File>& monolithicFiles_)
{
	id = monolithicFiles_.getFirst().getFileNameWithoutExtension().replaceCharacter('_', '/');
	monolithicFiles.reserve(monolithicFiles_.size());

	for (int i = 0; i < monolithicFiles_.size(); i++)
	{
		monolithicFiles.push_back(monolithicFiles_[i]);

#if USE_FALLBACK_READERS_FOR_MONOLITH
		ScopedPointer<FileInputStream> fallbackStream = new FileInputStream(monolithicFiles_[i]);
		fallbackReaders.add(new hlac::HiseLosslessAudioFormatReader(fallbackStream.release()));
#endif
	}
}

HlacMonolithInfo::~HlacMonolithInfo()
{
	memoryReaders.clear();
	fallbackReaders.clear();
}

bool HlacMonolithInfo::operator==(const Identifier& sampleMapId) const
{
	auto first = sampleMapId.toString().replaceCharacter('/', '_');
	auto second = id.toString().replaceCharacter('/', '_');
	return first == second;
}

void HlacMonolithInfo::fillMetadataInfo(const ValueTree& sampleMap)
{
	numChannels = sampleMap.getChild(0).getNumChildren();
	
	if (numChannels == 0) 
		numChannels = 1;

	numSplitFiles = (int)sampleMap.getProperty(MonolithIds::MonolithSplitAmount, 0);

	jassert(monolithicFiles.size() == (jmax(1, numSplitFiles) * numChannels));

	sampleInfo.reserve(sampleMap.getNumChildren());

	for (const auto& sample: sampleMap)
	{
		if (!sample.hasProperty(MonolithIds::MonolithLength) || !sample.hasProperty(MonolithIds::MonolithOffset))
		{
			throw StreamingSamplerSound::LoadingError(sample.getProperty("FileName").toString(), "\nhas no monolith metadata (probably an export error)");
		}

		SampleInfo info;

		info.splitIndex = sample.getProperty(MonolithIds::MonolithSplitIndex, 0);
		info.start = sample.getProperty(MonolithIds::MonolithOffset);
		info.length = sample.getProperty(MonolithIds::MonolithLength);
		info.sampleRate = sample.getProperty("SampleRate");
		
		if (numChannels == 1)
		{
			info.fileNames.add(sample.getProperty(MonolithIds::FileName));
		}
		else
		{
			for (int channel = 0; channel < numChannels; channel++)
				info.fileNames.add(sample.getChild(channel).getProperty(MonolithIds::FileName));
		}

		sampleInfo.push_back(info);
	}

	for (auto& mf: monolithicFiles)
	{
        if(mf.getSize() == 0)
        {
            jassertfalse;
            throw StreamingSamplerSound::LoadingError(mf.getFileName(), "File is corrupt");
        }

		ScopedPointer<MemoryMappedAudioFormatReader> reader = hlaf.createMemoryMappedReader(mf);

#if !USE_FALLBACK_READERS_FOR_MONOLITH
		reader->mapEntireFile();

		memoryReaders.add(dynamic_cast<hlac::HlacMemoryMappedAudioFormatReader*>(reader.release()));
		memoryReaders.getLast()->setTargetAudioDataType(AudioDataConverters::DataFormat::int16BE);

		if (memoryReaders.getLast()->getMappedSection().isEmpty())
		{
			jassertfalse;
			throw StreamingSamplerSound::LoadingError(mf.getFileName(), "Error at memory mapping");
		}
#endif
	}
}


String HlacMonolithInfo::getFileName(int channelIndex, int sampleIndex) const
{
	return sampleInfo[sampleIndex].fileNames[channelIndex];
}

juce::int64 HlacMonolithInfo::getMonolithOffset(int sampleIndex) const
{
	return sampleInfo[sampleIndex].start;
}

int HlacMonolithInfo::getNumSamplesInMonolith() const
{
	return (int)sampleInfo.size();
}

juce::int64 HlacMonolithInfo::getMonolithLength(int sampleIndex) const
{
	return (int64)jmax<int>(0, (int)sampleInfo[sampleIndex].length);
}

double HlacMonolithInfo::getMonolithSampleRate(int sampleIndex) const
{
	return sampleInfo[sampleIndex].sampleRate;
}

juce::AudioFormatReader* HlacMonolithInfo::createReader(int sampleIndex, int channelIndex)
{
#if USE_FALLBACK_READERS_FOR_MONOLITH
	return createFallbackReader(sampleIndex, channelIndex);
#else
	return createMonolithicReader(sampleIndex, channelIndex);
#endif
}

int HlacMonolithInfo::getFileIndex(int channelIndex, int sampleIndex) const
{
	if (numSplitFiles == 0)
	{
		jassert(isPositiveAndBelow(channelIndex, monolithicFiles.size()));
		return channelIndex;
	}
	else
	{
		auto splitIndex = sampleInfo[sampleIndex].splitIndex;

		auto fileIndex = channelIndex * numSplitFiles + splitIndex;
		jassert(isPositiveAndBelow(fileIndex, monolithicFiles.size()));
		return fileIndex;
	}
}

juce::File HlacMonolithInfo::getFile(int channelIndex, int sampleIndex) const
{
	auto fileIndex = getFileIndex(channelIndex, sampleIndex);
	return monolithicFiles[fileIndex];
}

juce::AudioFormatReader* HlacMonolithInfo::createUserInterfaceReader(int sampleIndex, int channelIndex)
{
	if (isPositiveAndBelow(sampleIndex, sampleInfo.size()))
	{
		const auto& info = sampleInfo[sampleIndex];

		const int64 start = info.start;
		const int64 length = info.length;

		auto mf = getFile(channelIndex, sampleIndex);

		if (mf.existsAsFile())
		{
			ScopedPointer<FileInputStream> fallbackStream = new FileInputStream(mf);
			ScopedPointer<hlac::HiseLosslessAudioFormatReader> thumbnailReader = new hlac::HiseLosslessAudioFormatReader(fallbackStream.release());

			thumbnailReader->setTargetAudioDataType(AudioDataConverters::float32BE);
			thumbnailReader->sampleRate = info.sampleRate;
			return new AudioSubsectionReader(thumbnailReader.release(), start, length, true);
		}
	}

	return nullptr;
}

juce::AudioFormatReader* HlacMonolithInfo::createMonolithicReader(int sampleIndex, int channelIndex)
{
	if (isPositiveAndBelow(sampleIndex, sampleInfo.size()))
	{
		const auto& info = sampleInfo[sampleIndex];

		const int64 start = info.start;
		const int64 length = info.length;

		auto fileIndex = getFileIndex(channelIndex, sampleIndex);

		if (memoryReaders[fileIndex] != nullptr)
		{
			return new hlac::HlacSubSectionReader(memoryReaders[fileIndex], start, length);
		}
		else
			return nullptr;
	}

	return nullptr;
}

juce::AudioFormatReader* HlacMonolithInfo::createFallbackReader(int sampleIndex, int channelIndex)
{
	if (isPositiveAndBelow(sampleIndex, sampleInfo.size()))
	{
		const auto& info = sampleInfo[sampleIndex];

		const int64 start = info.start;
		const int64 length = info.length;

		auto fileIndex = getFileIndex(channelIndex, sampleIndex);
		fallbackReaders[fileIndex]->sampleRate = info.sampleRate;

		return new hlac::HlacSubSectionReader(fallbackReaders[fileIndex], start, length);
	}

	return nullptr;
}







} // namespace hise
