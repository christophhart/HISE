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

#define MAX_SAMPLE_NUMBER 2147483647

// ==================================================================================================== StreamingSamplerSound methods

StreamingSamplerSound::StreamingSamplerSound(const juce::String &fileNameToLoad, StreamingSamplerSoundPool *pool) :
	fileReader(this, pool),
	sampleRate(-1.0),
	purged(false),
	monolithOffset(0),
	monolithLength(0),
	preloadSize(0),
	internalPreloadSize(0),
	entireSampleLoaded(false),
	sampleStart(0),
	sampleEnd(MAX_SAMPLE_NUMBER),
	sampleLength(MAX_SAMPLE_NUMBER),
	sampleStartMod(0),
	loopEnabled(false),
	loopStart(0),
	loopEnd(MAX_SAMPLE_NUMBER),
	loopLength(0),
	crossfadeLength(0),
	crossfadeArea(juce::Range<int>())
{
	fileReader.setFile(fileNameToLoad);

	setPreloadSize(0);
}

StreamingSamplerSound::StreamingSamplerSound(MonolithInfoToUse *info, int channelIndex, int sampleIndex) :
	fileReader(this, nullptr),
	sampleRate(-1.0),
	purged(false),
	monolithOffset(0),
	monolithLength(0),
	preloadSize(0),
	internalPreloadSize(0),
	entireSampleLoaded(false),
	sampleStart(0),
	sampleEnd(MAX_SAMPLE_NUMBER),
	sampleLength(MAX_SAMPLE_NUMBER),
	sampleStartMod(0),
	loopEnabled(false),
	loopStart(0),
	loopEnd(MAX_SAMPLE_NUMBER),
	loopLength(0),
	crossfadeLength(0),
	crossfadeArea(juce::Range<int>())
{
	fileReader.setMonolithicInfo(info, channelIndex, sampleIndex);

	setPreloadSize(0);
}

StreamingSamplerSound::~StreamingSamplerSound()
{
	masterReference.clear();
	fileReader.closeFileHandles();
}

void StreamingSamplerSound::setReversed(bool shouldBeReversed)
{
	if (reversed != shouldBeReversed)
	{
		if (shouldBeReversed)
		{
			loadEntireSample();
			preloadBuffer.reverse(0, preloadBuffer.getNumSamples());
			reversed = true;
		}
		else
		{
			reversed = false;
			setPreloadSize(preloadSize, true);
		}
	}
}

void StreamingSamplerSound::setBasicMappingData(const StreamingHelpers::BasicMappingData& data)
{
	rootNote = data.rootNote;

	midiNotes.clear();
	midiNotes.setRange(data.lowKey, (data.highKey - data.lowKey) + 1, 1);
	velocityRange.setRange(data.lowVelocity, (data.highVelocity - data.lowVelocity) + 1, 1);
}

void StreamingSamplerSound::setDelayPreloadInitialisation(bool shouldDelay)
{
    if(delayPreloadInitialisation != shouldDelay)
    {
        delayPreloadInitialisation = shouldDelay;
        
        if(!shouldDelay)
        {
            loopChanged();
            setPreloadSize(preloadSize, true);
        }
    }
}
    
void StreamingSamplerSound::setPreloadSize(int newPreloadSize, bool forceReload)
{
    if(delayPreloadInitialisation)
    {
        preloadSize = newPreloadSize;
        return;
    }
    
	if (reversed)
		return;

	const bool preloadSizeChanged = preloadSize == newPreloadSize;
	const bool streamingDeactivated = newPreloadSize == -1 && entireSampleLoaded;

	if (!forceReload && (preloadSizeChanged || streamingDeactivated)) return;

    juce::ScopedLock sl(getSampleLock());

	const bool sampleDeactivated = !hasActiveState() || newPreloadSize == 0;

	if (sampleDeactivated)
	{
		internalPreloadSize = 0;
		preloadSize = 0;

		preloadBuffer = hlac::HiseSampleBuffer(!fileReader.isMonolithic(), fileReader.isStereo() ? 2 : 1, 0);

		return;
	}

	preloadSize = newPreloadSize;

	if (sampleLength == MAX_SAMPLE_NUMBER)
	{
		// this hasn't been initialised, so we need to do it here...

		fileReader.openFileHandles(juce::dontSendNotification);
		sampleLength = (int)fileReader.getSampleLength();
		loopLength = std::min<int>(loopLength, sampleLength);
		loopEnd = std::min<int>(loopEnd, sampleLength);
	}

	if (newPreloadSize == -1 || (preloadSize + sampleStartMod) > sampleLength)
	{
		internalPreloadSize = (int)sampleLength;
		entireSampleLoaded = true;
	}
	else
	{
		internalPreloadSize = preloadSize + (int)sampleStartMod;
		entireSampleLoaded = false;
	}

	// Check if we should simply load the entire sample
	if (internalPreloadSize > HISE_LOAD_ENTIRE_SAMPLE_THRESHHOLD)
	{
		internalPreloadSize = (int)sampleLength;
		entireSampleLoaded = true;
	}

	internalPreloadSize = juce::jmax(preloadSize, internalPreloadSize, 2048);

	fileReader.openFileHandles();

	preloadBuffer = hlac::HiseSampleBuffer(!fileReader.isMonolithic(), fileReader.isStereo() ? 2 : 1, 0);

	try
	{
		preloadBuffer.setSize(fileReader.isStereo() ? 2 : 1, internalPreloadSize);
	}
	catch (std::exception e)
	{
		preloadBuffer.setSize(fileReader.isStereo() ? 2 : 1, 0);

		throw StreamingSamplerSound::LoadingError(getFileName(), "Preload error (max memory exceeded).");
	}

	if (preloadBuffer.getNumSamples() == 0)
	{
		return;
	}

	preloadBuffer.clear();
	preloadBuffer.allocateNormalisationTables(sampleStart);

	if (sampleRate <= 0.0)
	{
		if (auto* reader = fileReader.getReader())
		{
			sampleRate = reader->sampleRate;
			sampleEnd = std::min<int>(sampleEnd, (int)reader->lengthInSamples);
			sampleLength = std::max<int>(0, sampleEnd - sampleStart);
			loopEnd = std::min(loopEnd, sampleEnd);
		}
	}

	if (loopEnabled && (loopEnd - loopStart > 0) && (loopEnd - sampleStart) < internalPreloadSize)
	{
		//entireSampleLoaded = false;
		
		int pos = loopEnd - sampleStart;

		fileReader.readFromDisk(preloadBuffer, 0, pos, sampleStart + monolithOffset, true);
		const int samplesPerFillOp = (loopEnd - loopStart);

		int numTodo = internalPreloadSize - (loopEnd - sampleStart);

		while (numTodo > 0)
		{
			int numThisTime = std::min<int>(numTodo, samplesPerFillOp);

			hlac::HiseSampleBuffer::copy(preloadBuffer, preloadBuffer, pos, loopStart - sampleStart, numThisTime);

			numTodo -= numThisTime;
			pos += numThisTime;
		}
	}
	else
	{
		auto samplesToRead = std::min<int>(sampleLength, internalPreloadSize);

		if(samplesToRead > 0)
			fileReader.readFromDisk(preloadBuffer, 0, samplesToRead, sampleStart + monolithOffset, true);
	}

	applyCrossfadeToPreloadBuffer();
}



size_t StreamingSamplerSound::getActualPreloadSize() const
{
	auto bytesPerSample = fileReader.isMonolithic() ? sizeof(int16_t) : sizeof(float);

	return hasActiveState() ? (size_t)(internalPreloadSize *preloadBuffer.getNumChannels()) * bytesPerSample + (size_t)(loopBuffer.getNumSamples() *loopBuffer.getNumChannels()) * bytesPerSample : 0;
}

void StreamingSamplerSound::loadEntireSample() { setPreloadSize(-1); }

void StreamingSamplerSound::increaseVoiceCount() const { fileReader.increaseVoiceCount(); }
void StreamingSamplerSound::decreaseVoiceCount() const { fileReader.decreaseVoiceCount(); }

void StreamingSamplerSound::closeFileHandle()
{
	fileReader.closeFileHandles();
}

void StreamingSamplerSound::openFileHandle()
{
	fileReader.openFileHandles();
}

bool StreamingSamplerSound::isOpened()
{
	return fileReader.isOpened();
}

bool StreamingSamplerSound::isStereo() const
{
	return fileReader.isStereo();
}

int StreamingSamplerSound::getBitRate() const
{
    juce::ScopedPointer<juce::AudioFormatReader> reader = fileReader.createMonolithicReaderForPreview();
	
	if (reader != nullptr)
		return reader->bitsPerSample;

	return -1;
}

bool StreamingSamplerSound::replaceAudioFile(const juce::AudioSampleBuffer& b)
{
	if (b.getNumChannels() == (fileReader.isStereo() ? 2 : 1))
	{
        juce::TemporaryFile tmp(juce::File(fileReader.getFileName(true)));

		tmp.getFile().create();

		auto fos = new juce::FileOutputStream(tmp.getFile());

        juce::ScopedPointer<juce::AudioFormatWriter> writer = fileReader.createWriterWithSameFormat(fos);

		if (writer != nullptr)
		{
			bool ok = writer->writeFromAudioSampleBuffer(b, 0, b.getNumSamples());

			if (ok)
				ok = writer->flush();

			writer = nullptr;

			fileReader.closeFileHandles(juce::sendNotification);

			if (ok)
				return tmp.overwriteTargetFileWithTemporary();

			return ok;
		}

		return false;
	}
    
    return false;
}

bool StreamingSamplerSound::isMonolithic() const
{
	return fileReader.isMonolithic();
}

juce::AudioFormatReader* StreamingSamplerSound::createReaderForPreview()
{
	return fileReader.createMonolithicReaderForPreview();
}

juce::AudioFormatReader* StreamingSamplerSound::createReaderForAnalysis()
{
	return fileReader.getReader();
}

juce::String StreamingSamplerSound::getSampleStateAsString() const
{
	if (isMissing())
	{
		if (purged) return "Purged+Missing";
		else return "Missing";
	}
	else
	{
		if (purged) return "Purged";
		else return "Normal";
	}
}

juce::String StreamingSamplerSound::getFileName(bool getFullPath /*= false*/) const { return fileReader.getFileName(getFullPath); }

int64_t StreamingSamplerSound::getHashCode() { return fileReader.getHashCode(); }


void StreamingSamplerSound::checkFileReference()
{
	fileReader.checkFileReference();
}


void StreamingSamplerSound::replaceFileReference(const juce::String &newFileName)
{
	fileReader.setFile(newFileName);

	if (isMissing()) return;

	fileReader.openFileHandles();

	auto *reader = fileReader.getReader();

	if (reader != nullptr)
	{
		monolithLength = (int)reader->lengthInSamples;
		sampleRate = reader->sampleRate;

		setPreloadSize(PRELOAD_SIZE, true);
	}
	else
	{
		throw LoadingError(fileReader.getFileName(true), "Error at normal reading");
	}

	fileReader.closeFileHandles();
}

bool StreamingSamplerSound::isMissing() const noexcept { return fileReader.isMissing(); }

bool StreamingSamplerSound::hasActiveState() const noexcept { return !isMissing() && !purged; }

double StreamingSamplerSound::getPitchFactor(int noteNumberToPitch, int rootNoteForPitchFactor) noexcept
{
    // If this fires, you are using uninitialised MIDI values
    jassert(noteNumberToPitch >= 0);
    jassert(rootNoteForPitchFactor >= 0);
    
	return pow(2.0, (noteNumberToPitch - rootNoteForPitchFactor) / 12.0);
}

void StreamingSamplerSound::setSampleStart(int newSampleStart)
{
	if (sampleStart != newSampleStart &&
		(!loopEnabled || (loopEnabled && loopStart > newSampleStart)))
	{
		sampleStart = newSampleStart;
		lengthChanged();
	}
}

void StreamingSamplerSound::setSampleStartModulation(int newModulationDelta)
{
	if (sampleStartMod != newModulationDelta)
	{
        juce::ScopedLock sl(getSampleLock());

		sampleStartMod = newModulationDelta;
		lengthChanged();
	}
}

void StreamingSamplerSound::setLoopEnabled(bool shouldBeEnabled)
{
	if (loopEnabled != shouldBeEnabled)
	{
		loopEnabled = shouldBeEnabled;

		if (shouldBeEnabled && loopStart < sampleStart)
		{
			setLoopStart(sampleStart);
			return;
		}
		if (shouldBeEnabled && loopEnd > sampleEnd)
		{
			setLoopEnd(sampleEnd);
			return;
		}

		loopChanged();
	}
}

void StreamingSamplerSound::setLoopStart(int newLoopStart)
{
	if (loopStart != newLoopStart)
	{
		loopStart = std::max(sampleStart, newLoopStart);
		loopChanged();
	}
}

void StreamingSamplerSound::setLoopEnd(int newLoopEnd)
{
	if (loopEnd != newLoopEnd)
	{
		loopEnd = std::min(sampleEnd, newLoopEnd);
		crossfadeArea = juce::Range<int>((int)(loopEnd - crossfadeLength), (int)loopEnd);
		loopChanged();
	}
}

void StreamingSamplerSound::setLoopCrossfade(int newCrossfadeLength)
{
	if (crossfadeLength != newCrossfadeLength)
	{
		crossfadeLength = newCrossfadeLength;
		crossfadeArea = juce::Range<int>((int)(loopEnd - crossfadeLength), (int)loopEnd);
		loopChanged();
	}
}


void StreamingSamplerSound::setSampleEnd(int newSampleEnd)
{
	if (sampleEnd != newSampleEnd &&
		(!loopEnabled || (loopEnabled && loopEnd < newSampleEnd)))
	{
		sampleEnd = newSampleEnd;
		lengthChanged();
	}
}

void StreamingSamplerSound::lengthChanged()
{
    juce::ScopedLock sl(getSampleLock());

	if (sampleEnd != MAX_SAMPLE_NUMBER)
	{
		sampleLength = std::max<int>(0, sampleEnd - sampleStart);
		setPreloadSize(preloadSize, true);
	}
}

void StreamingSamplerSound::applyCrossfadeToPreloadBuffer()
{
	if (loopEnabled && crossfadeLength > 0 && loopLength > 0)
	{
		auto fadePos = loopEnd - sampleStart - crossfadeLength;
		auto numInBuffer = preloadBuffer.getNumSamples();
        
        if(loopBuffer.getNumSamples() == 0)
        {
            bool preloadContainsLoop = loopEnd <= preloadBuffer.getNumSamples() - sampleStart;
            rebuildCrossfadeBuffer(preloadContainsLoop);
        }
        
		if (fadePos < numInBuffer)
		{
			preloadBuffer.burnNormalisation();

			while (fadePos < numInBuffer)
			{
				int numToCopy = std::min(crossfadeLength, numInBuffer - fadePos);

				hlac::HiseSampleBuffer::copy(preloadBuffer, loopBuffer, fadePos, 0, numToCopy);
				fadePos += loopLength;
			}
		}
	}
}

void StreamingSamplerSound::loopChanged()
{
    if(delayPreloadInitialisation)
        return;
    
    juce::ScopedLock sl(getSampleLock());

	if (sampleEnd == MAX_SAMPLE_NUMBER && loopEnabled)
	{
		fileReader.openFileHandles();
		sampleEnd = (int)fileReader.getSampleLength();
	}

	loopStart  = std::max<int>(loopStart, sampleStart);
	loopEnd    = std::min<int>(loopEnd, sampleEnd);
	loopLength = std::max<int>(0, loopEnd - loopStart);

	if (loopEnabled)
	{
		bool preloadContainsLoop = loopEnd <= preloadBuffer.getNumSamples() - sampleStart;

		if (preloadContainsLoop)
		{
			useSmallLoopBuffer = false;
			smallLoopBuffer.setSize(1, 0);
			setPreloadSize(preloadSize, true);
		}
		else if (loopLength < 8192)
		{
			useSmallLoopBuffer = true;

			ScopedFileHandler sfh(this);
			smallLoopBuffer = hlac::HiseSampleBuffer(!fileReader.isMonolithic(), fileReader.isStereo() ? 2 : 1, (int)loopLength);
			fileReader.readFromDisk(smallLoopBuffer, 0, loopLength, loopStart, false);
		}
		else
		{
			useSmallLoopBuffer = false;
			smallLoopBuffer.setSize(2, 0);
		}

        if(crossfadeLength != 0)
        {
            rebuildCrossfadeBuffer(preloadContainsLoop);
            applyCrossfadeToPreloadBuffer();
        }
	}
	else
	{
		if (loopEnd < internalPreloadSize)
		{
			useSmallLoopBuffer = false;
			smallLoopBuffer.setSize(1, 0);
			setPreloadSize(preloadSize, true);
		}
	}
}

void StreamingSamplerSound::rebuildCrossfadeBuffer(bool preloadContainsLoop)
{
    // If we're copying the crossfade to the preload buffer we will need to take the sample start into account
    // (otherwise it will be compensated during playback)
    auto offsetInPreloadBuffer = preloadContainsLoop ? sampleStart : 0;
    
    const int startCrossfade = offsetInPreloadBuffer + loopStart - crossfadeLength;
    
    if (startCrossfade < 0)
        return;
    
    auto isHlac = fileReader.isMonolithic();
    
    loopBuffer = hlac::HiseSampleBuffer(!isHlac, 2, (int)crossfadeLength);
    loopBuffer.clear();
    
    hlac::HiseSampleBuffer tempBuffer(!isHlac, 2, (int)crossfadeLength);
    
    // Calculate the fade in
    
    tempBuffer.clear();
    
    ScopedFileHandler sfh(this);
    
    fileReader.readFromDisk(loopBuffer, 0, (int)crossfadeLength, startCrossfade + monolithOffset, false);
    
    loopBuffer.burnNormalisation();
    
    loopBuffer.applyGainRamp(0, 0, (int)crossfadeLength, 0.0f, 1.0f);
    loopBuffer.applyGainRamp(1, 0, (int)crossfadeLength, 0.0f, 1.0f);
    
    // Calculate the fade out
    tempBuffer.clear();
    
    const int endCrossfade = offsetInPreloadBuffer + loopEnd - crossfadeLength;
    
    fileReader.readFromDisk(tempBuffer, 0, (int)crossfadeLength, endCrossfade + monolithOffset, false);
    
    tempBuffer.burnNormalisation();
    tempBuffer.applyGainRamp(0, 0, (int)crossfadeLength, 1.0f, 0.0f);
    tempBuffer.applyGainRamp(1, 0, (int)crossfadeLength, 1.0f, 0.0f);
    
    hlac::HiseSampleBuffer::add(loopBuffer, tempBuffer, 0, 0, crossfadeLength);
}
    
void StreamingSamplerSound::wakeSound() const { fileReader.wakeSound(); }


bool StreamingSamplerSound::hasEnoughSamplesForBlock(int maxSampleIndexInFile) const
{
	return (loopEnabled && loopLength != 0) || maxSampleIndexInFile < sampleLength;
}

float StreamingSamplerSound::calculatePeakValue()
{
	return fileReader.calculatePeakValue();
}

void StreamingSamplerSound::fillSampleBuffer(hlac::HiseSampleBuffer &sampleBuffer, int samplesToCopy, int uptime) const
{
    juce::ScopedLock sl(getSampleLock());

	if (sampleBuffer.getNumSamples() == samplesToCopy)
		sampleBuffer.clearNormalisation({});

	if (!fileReader.isUsed()) return;

	const bool wrapLoop = (uptime + samplesToCopy + sampleStart) > loopEnd;

	if (loopEnabled && loopLength != 0 && wrapLoop)
	{
		const int indexInLoop = (uptime + sampleStart - loopStart) % loopLength;

		const int numSamplesInThisLoop = (int)(loopLength - indexInLoop);

		if (useSmallLoopBuffer)
		{
			int numSamplesBeforeFirstWrap = 0;

			if (indexInLoop < 0)
			{
				numSamplesBeforeFirstWrap = std::min<int>(samplesToCopy, loopStart - (uptime + sampleStart));

				fillInternal(sampleBuffer, numSamplesBeforeFirstWrap, uptime + (int)sampleStart, 0);
			}
			else
			{
				numSamplesBeforeFirstWrap = std::min<int>(samplesToCopy, numSamplesInThisLoop);
				int startSample = indexInLoop;

				hlac::HiseSampleBuffer::copy(sampleBuffer, smallLoopBuffer, 0, startSample, numSamplesBeforeFirstWrap);
			}

			int numSamples = samplesToCopy - numSamplesBeforeFirstWrap;
			int indexInSampleBuffer = numSamplesBeforeFirstWrap;

			if (numSamples < 0)
			{
				jassertfalse;
				return;
			}

			while (numSamples > (int)loopLength)
			{
				jassert(indexInSampleBuffer < sampleBuffer.getNumSamples());

				hlac::HiseSampleBuffer::copy(sampleBuffer, smallLoopBuffer, indexInSampleBuffer, 0, loopLength);

				numSamples -= (int)loopLength;
				indexInSampleBuffer += (int)loopLength;
			}

			hlac::HiseSampleBuffer::copy(sampleBuffer, smallLoopBuffer, indexInSampleBuffer, 0, numSamples);
		}

		// Loop is smaller than streaming buffers
		else if (loopLength < samplesToCopy)
		{
			const int numSamplesBeforeFirstWrap = numSamplesInThisLoop;

			int numSamples = samplesToCopy - numSamplesBeforeFirstWrap;
			int startSample = numSamplesBeforeFirstWrap;

			const int indexToUse = indexInLoop > 0 ? ((int)indexInLoop + (int)loopStart) : uptime + (int)sampleStart;
			fillInternal(sampleBuffer, numSamplesBeforeFirstWrap, indexToUse, 0);

			while (numSamples > (int)loopLength)
			{
				fillInternal(sampleBuffer, (int)loopLength, (int)loopStart, startSample);
				numSamples -= (int)loopLength;
				startSample += (int)loopLength;
			}

			fillInternal(sampleBuffer, numSamples, (int)loopStart, startSample);
		}

		// loop is bigger than streaming buffers and does not get wrapped
		else if (numSamplesInThisLoop > samplesToCopy)
		{
			fillInternal(sampleBuffer, samplesToCopy, (int)(loopStart + indexInLoop));
		}

		// loop is bigger than streaming buffers and needs some wrapping
		else
		{
			const int numSamplesBeforeWrap = numSamplesInThisLoop;
			const int numSamplesAfterWrap = samplesToCopy - numSamplesBeforeWrap;

			fillInternal(sampleBuffer, numSamplesBeforeWrap, (int)(loopStart + indexInLoop), 0);
			fillInternal(sampleBuffer, numSamplesAfterWrap, (int)loopStart, numSamplesBeforeWrap);
		}
	}
	else
	{
		jassert(((int)sampleStart + uptime + samplesToCopy) <= sampleEnd);

		fillInternal(sampleBuffer, samplesToCopy, uptime + (int)sampleStart);
	}
};

void StreamingSamplerSound::fillInternal(hlac::HiseSampleBuffer &sampleBuffer, int samplesToCopy, int uptime, int offsetInBuffer/*=0*/) const
{
	jassert(uptime + samplesToCopy <= sampleEnd);

	// Some samples from the loop crossfade buffer are required
	if (loopEnabled && juce::Range<int>(uptime, uptime + samplesToCopy).intersects(crossfadeArea))
	{
		const int numSamplesBeforeCrossfade = std::max(0, crossfadeArea.getStart() - uptime);

		if (numSamplesBeforeCrossfade > 0)
		{
			fillInternal(sampleBuffer, numSamplesBeforeCrossfade, uptime, 0);
		}

		const int numSamplesInCrossfade = std::min(samplesToCopy - numSamplesBeforeCrossfade, (int)crossfadeLength);

		if (numSamplesInCrossfade > 0)
		{
			const int indexInLoopBuffer = std::max(0, uptime - crossfadeArea.getStart());

			hlac::HiseSampleBuffer::copy(sampleBuffer, loopBuffer, numSamplesBeforeCrossfade, indexInLoopBuffer, numSamplesInCrossfade);
		}

		// Should be taken care by higher logic (fillSampleBuffer should wrap the loop)
		jassert((samplesToCopy - numSamplesBeforeCrossfade - numSamplesInCrossfade) == 0);
	}

	// All samples can be fetched from the preload buffer
	else if (uptime + samplesToCopy < internalPreloadSize)
	{
		// the preload buffer has already the samplestart offset
		const int indexInPreloadBuffer = uptime - (int)sampleStart;

		jassert(indexInPreloadBuffer >= 0);

		jassert(!crossfadeArea.contains(indexInPreloadBuffer));

		if (indexInPreloadBuffer + samplesToCopy < preloadBuffer.getNumSamples())
		{
			hlac::HiseSampleBuffer::copy(sampleBuffer, preloadBuffer, offsetInBuffer, indexInPreloadBuffer, samplesToCopy);
		}
		else
		{
			jassertfalse;

			sampleBuffer.clear();
		}
	}

	// Read all samples from disk
	else
	{
		fileReader.readFromDisk(sampleBuffer, offsetInBuffer, samplesToCopy, uptime + monolithOffset, true);
	}
}

// =============================================================================================================================================== StreamingSamplerSound::FileReader methods


StreamingSamplerSound::FileReader::FileReader(StreamingSamplerSound *soundForReader, StreamingSamplerSoundPool *pool_) :
	pool(pool_),
	sound(soundForReader),
	missing(true),
	hashCode(0),
	voiceCount(0),
	fileHandlesOpen(false)
{}

StreamingSamplerSound::FileReader::~FileReader()
{
    juce::ScopedWriteLock sl(fileAccessLock);

	memoryReader = nullptr;
	normalReader = nullptr;
}

void StreamingSamplerSound::FileReader::setFile(const juce::String &fileName)
{
	monolithicInfo = nullptr;

	if (juce::File::isAbsolutePath(fileName))
	{
		loadedFile = juce::File(fileName);

		const juce::String fileExtension = loadedFile.getFileExtension();

		fileFormatSupportsMemoryReading = fileExtension.contains("wav") || fileExtension.contains("aif");// || fileExtension.contains("hlac");

		hashCode = loadedFile.hashCode64();
	}
	else
	{
		faultyFileName = fileName;
		loadedFile = juce::File();
	}
}

juce::String StreamingSamplerSound::FileReader::getFileName(bool getFullPath)
{
	if (monolithicInfo != nullptr)
	{
		return monolithicName;
	}

	if (faultyFileName.isNotEmpty())
	{
		if (getFullPath)
		{
			return faultyFileName;
		}
		else
		{
#if JUCE_WINDOWS
			return faultyFileName.fromLastOccurrenceOf("/", false, false);
#else
			return faultyFileName.fromLastOccurrenceOf("\\", false, false);
#endif
		}
	}
	else return getFullPath ? loadedFile.getFullPathName() : loadedFile.getFileName();
}

void StreamingSamplerSound::FileReader::checkFileReference()
{
	if (monolithicInfo != nullptr) return;

	if (missing) missing = !loadedFile.existsAsFile();// && getReader() == nullptr;
}

void StreamingSamplerSound::FileReader::refreshFileInformation()
{
	checkFileReference();

	if (!missing)
	{
		faultyFileName = juce::String();

		const juce::String fileExtension = loadedFile.getFileExtension();

		fileFormatSupportsMemoryReading = fileExtension.compareIgnoreCase(".wav") || fileExtension.startsWithIgnoreCase(".aif");// || fileExtension.startsWithIgnoreCase("hlac");

		hashCode = loadedFile.hashCode64();
	}
}



juce::AudioFormatReader * StreamingSamplerSound::FileReader::getReader()
{
	if (!fileHandlesOpen) 
		openFileHandles();

	if (memoryReader) return memoryReader.get();
	else if (normalReader) return normalReader.get();
	else return nullptr;
}

void StreamingSamplerSound::FileReader::wakeSound()
{
	if (!fileFormatSupportsMemoryReading) return;

	if (memoryReader != nullptr && !memoryReader->getMappedSection().isEmpty()) memoryReader->touchSample(sound->sampleStart + sound->monolithOffset);
}


void StreamingSamplerSound::FileReader::openFileHandles(juce::NotificationType notifyPool)
{
	if (fileHandlesOpen)
	{
		jassert(memoryReader != nullptr || normalReader != nullptr);
		return;
	}
	else
	{
		jassert(memoryReader == nullptr || normalReader == nullptr);

        juce::ScopedWriteLock sl(fileAccessLock);

		fileHandlesOpen = true;

		memoryReader = nullptr;
		normalReader = nullptr;

		if (monolithicInfo != nullptr)
		{
#if USE_FALLBACK_READERS_FOR_MONOLITH
			normalReader = monolithicInfo->createFallbackReader(monolithicIndex, monolithicChannelIndex);
#else
			normalReader.reset (monolithicInfo->createMonolithicReader (monolithicIndex, monolithicChannelIndex));
#endif

			if (normalReader != nullptr)
				stereo = normalReader->numChannels > 1;

			sampleLength = getMonolithLength();

		}
		else
		{
			if (fileFormatSupportsMemoryReading)
			{
				auto* format = pool->afm.findFormatForFileExtension(loadedFile.getFileExtension());

				if (format != nullptr)
				{
					memoryReader.reset (format->createMemoryMappedReader(loadedFile));

					if (memoryReader != nullptr)
					{
						memoryReader->mapSectionOfFile(juce::Range<int64_t>((int64_t)(sound->sampleStart) + (int64_t)(sound->monolithOffset), (int64_t)(sound->sampleEnd)));
						sampleLength = std::max<int64_t>(0, memoryReader->getMappedSection().getLength());
						stereo = memoryReader->numChannels > 1;
					}
				}
			}


			normalReader.reset (pool->afm.createReaderFor(loadedFile));
			sampleLength = normalReader != nullptr ? normalReader->lengthInSamples : 0;
			stereo = (normalReader != nullptr) ? (normalReader->numChannels > 1) : false;
		}

#if USE_BACKEND
		if (monolithicInfo == nullptr && notifyPool == sendNotification) pool->increaseNumOpenFileHandles();
#else
		ignoreUnused(notifyPool);
#endif
	}
}


bool StreamingSamplerSound::FileReader::isStereo() const noexcept
{
	return stereo;
}

void StreamingSamplerSound::FileReader::closeFileHandles(juce::NotificationType notifyPool)
{
	if (monolithicIndex != -1) return; // don't close the reader for monolithic files...

	if (voiceCount.load() == 0)
	{
        juce::ScopedWriteLock sl(fileAccessLock);

		fileHandlesOpen = false;

		memoryReader = nullptr;
		normalReader = nullptr;

		if (monolithicInfo == nullptr && notifyPool == juce::sendNotification) pool->decreaseNumOpenFileHandles();
	}
}




void StreamingSamplerSound::FileReader::readFromDisk(hlac::HiseSampleBuffer &buffer, int startSample, int numSamples, int readerPosition, bool useMemoryMappedReader)
{
	if (!fileHandlesOpen) openFileHandles(juce::sendNotification);

#if USE_SAMPLE_DEBUG_COUNTER

	float* l = buffer.getWritePointer(0, startSample);
	float* r = buffer.getWritePointer(1, startSample);

	int v = readerPosition;

	for (int i = 0; i < numSamples; i++)
	{
		l[i] = (float)v;
		r[i] = (float)v;

		v++;
	}

	return;
#endif


	buffer.clear(startSample, numSamples);

	if (!isMonolithic() && useMemoryMappedReader)
	{
		if (memoryReader != nullptr && memoryReader->getMappedSection().contains(juce::Range<int64_t>(readerPosition, readerPosition + numSamples)))
		{
            juce::ScopedReadLock sl(fileAccessLock);

			if (buffer.isFloatingPoint())
				memoryReader->read(buffer.getFloatBufferForFileReader(), startSample, numSamples, readerPosition, true, true);
			else
				jassertfalse;

			return;
		}
	}

	if (normalReader != nullptr)
	{
        juce::ScopedReadLock sl(fileAccessLock);

		if (buffer.isFloatingPoint())
			normalReader->read(buffer.getFloatBufferForFileReader(), startSample, numSamples, readerPosition, true, true);
		else
			dynamic_cast<hlac::HlacSubSectionReader*>(normalReader.get())->readIntoFixedBuffer(buffer, startSample, numSamples, readerPosition);
	}
	else
	{
		// Something is wrong so clear the buffer to be safe...
		buffer.clear(startSample, numSamples);
	}
}

float getAbsoluteValue(float input)
{
    return input > 0.0f ? input : input * -1.0f;
}

float StreamingSamplerSound::FileReader::calculatePeakValue()
{
#if !HI_ENABLE_EXPANSION_EDITING

	// If you hit this assertion, it means that you haven't saved the normalised gain value into the samplemap.
	// Please resave the samplemap in order to speed up loading times
	jassertfalse;

#endif

	float l1, l2, r1, r2;

	openFileHandles();

    juce::ScopedPointer<juce::AudioFormatReader> readerToUse = createMonolithicReaderForPreview();// getReader();
	
	if (readerToUse != nullptr) 
		readerToUse->readMaxLevels(sound->sampleStart + sound->monolithOffset, sound->sampleLength, l1, l2, r1, r2);
	else return 0.0f;

	closeFileHandles();

    // so tired of std::abs...
    const float maxLeft = std::max<float>(getAbsoluteValue(l1), getAbsoluteValue(l2));
	const float maxRight = std::max<float>(getAbsoluteValue(r1), getAbsoluteValue(r2));

	return std::max<float>(maxLeft, maxRight);
}


juce::AudioFormatReader* StreamingSamplerSound::FileReader::createMonolithicReaderForPreview()
{
	if (monolithicInfo != nullptr)
		return monolithicInfo->createThumbnailReader(monolithicIndex, monolithicChannelIndex);
	else
		return pool->afm.createReaderFor(loadedFile);
}


juce::AudioFormatWriter* StreamingSamplerSound::FileReader::createWriterWithSameFormat(juce::OutputStream* out)
{
    juce::ScopedPointer<juce::OutputStream> ownedOutput = out;

	if (monolithicInfo != nullptr)
	{
		// Can't use this method with monoliths...
		jassertfalse;
		return nullptr;
	}

	auto extension = loadedFile.getFileExtension();

    juce::ScopedPointer<juce::AudioFormatReader> r = createMonolithicReaderForPreview();

	for (int i = 0; i < pool->afm.getNumKnownFormats(); i++)
	{
		if (pool->afm.getKnownFormat(i)->getFileExtensions().contains(extension))
		{
			auto writer = pool->afm.getKnownFormat(i)->createWriterFor(ownedOutput, r->sampleRate, r->numChannels, r->bitsPerSample, r->metadataValues, 9);

			if (writer != nullptr)
				ownedOutput.release();

			return writer;
		}
	}

	return nullptr;
}

void StreamingSamplerSound::FileReader::setMonolithicInfo(MonolithInfoToUse* info, int channelIndex, int sampleIndex)
{
	monolithicInfo = info;
	monolithicIndex = sampleIndex;
	missing = (sampleIndex == -1);
	monolithicName = info->getFileName(channelIndex, sampleIndex);

	hashCode = monolithicName.hashCode64();

	monolithicChannelIndex = channelIndex;
}

} // namespace hise
