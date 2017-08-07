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
*   which must be separately licensed for cloused source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

#define LOG_SAMPLE_RENDERING 1

// ==================================================================================================== StreamingSamplerSound methods

StreamingSamplerSound::StreamingSamplerSound(const String &fileNameToLoad, ModulatorSamplerSoundPool *pool):
	fileReader(this, pool),
    sampleRate(-1.0),
    purged(false),
    monolithOffset(0),
    monolithLength(0),
    preloadSize(0),
    internalPreloadSize(0),
    entireSampleLoaded(false),
    sampleStart(0),
    sampleEnd(INT_MAX),
    sampleLength(INT_MAX),
    sampleStartMod(0),
    loopEnabled(false),
    loopStart(0),
    loopEnd(INT_MAX),
    loopLength(0),
    crossfadeLength(0),
    crossfadeArea(Range<int>())
{
	fileReader.setFile(fileNameToLoad);

    setPreloadSize(0);
}

StreamingSamplerSound::StreamingSamplerSound(MonolithInfoToUse *info, int channelIndex, int sampleIndex):
	fileReader(this, nullptr),
	sampleRate(-1.0),
	purged(false),
	monolithOffset(0),
	monolithLength(0),
	preloadSize(0),
	internalPreloadSize(0),
	entireSampleLoaded(false),
	sampleStart(0),
	sampleEnd(INT_MAX),
	sampleLength(INT_MAX),
	sampleStartMod(0),
	loopEnabled(false),
	loopStart(0),
	loopEnd(INT_MAX),
	loopLength(0),
	crossfadeLength(0),
	crossfadeArea(Range<int>())
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

void StreamingSamplerSound::setPreloadSize(int newPreloadSize, bool forceReload)
{
	if (reversed)
	{
		return;
	}

	const bool preloadSizeChanged = preloadSize == newPreloadSize;
    const bool streamingDeactivated = newPreloadSize == -1 && entireSampleLoaded;
    
	if(!forceReload && (preloadSizeChanged || streamingDeactivated)) return;

	ScopedLock sl(getSampleLock());

    const bool sampleDeactivated = !hasActiveState() || newPreloadSize == 0;
    
	if (sampleDeactivated)
	{
		internalPreloadSize = 0;
		preloadSize = 0;

		preloadBuffer = hlac::HiseSampleBuffer(!fileReader.isMonolithic(), 2, 0);

		return;
	}
    
	preloadSize = newPreloadSize;

	if(newPreloadSize == -1 || (preloadSize + sampleStartMod) > sampleLength)
	{
		internalPreloadSize = (int)sampleLength;
		entireSampleLoaded = true;
	}
	else
	{
		internalPreloadSize = preloadSize + (int)sampleStartMod;
		entireSampleLoaded = false;
	}

	internalPreloadSize = jmax(preloadSize, internalPreloadSize, 2048);

	preloadBuffer = hlac::HiseSampleBuffer(!fileReader.isMonolithic(), 2, 0);
	
	try
	{
		preloadBuffer.setSize(2, internalPreloadSize);
	}
	catch (std::exception e)
	{
		preloadBuffer.setSize(2, 0);

		throw StreamingSamplerSound::LoadingError(getFileName(), "Preload error (max memory exceeded).");
	}
	
	if (preloadBuffer.getNumSamples() == 0)
	{
		return;
	}

	preloadBuffer.clear();
	
	if (sampleRate <= 0.0)
	{
		if (AudioFormatReader *reader = fileReader.getReader())
		{
			sampleRate = reader->sampleRate;
			sampleEnd = jmin<int>(sampleEnd, (int)reader->lengthInSamples);
			sampleLength = sampleEnd - sampleStart;
			loopEnd = jmin(loopEnd, sampleEnd);
		}
	}

	if (loopEnabled && (loopEnd - loopStart > 0) && sampleLength < internalPreloadSize)
	{
		int samplesToFill = internalPreloadSize;
		int offsetInPreloadBuffer = 0;

		fileReader.readFromDisk(preloadBuffer, 0, sampleLength, sampleStart + monolithOffset, true);

		const int samplesPerFillOp = (loopEnd - loopStart);

		if (samplesPerFillOp > 0)
		{
			offsetInPreloadBuffer += sampleLength;
			samplesToFill -= sampleLength;

			while (samplesToFill > 0)
			{
				const int samplesThisTime = jmin<int>(samplesToFill, samplesPerFillOp);

				fileReader.readFromDisk(preloadBuffer, offsetInPreloadBuffer, samplesThisTime, loopStart, true);

				offsetInPreloadBuffer += samplesThisTime;
				samplesToFill -= samplesThisTime;
			}

			jassert(samplesToFill == 0);
		}
		else
		{
			jassertfalse;
		}
	}
	else
	{
		fileReader.readFromDisk(preloadBuffer, 0, internalPreloadSize, sampleStart + monolithOffset, true);
	}
}



size_t StreamingSamplerSound::getActualPreloadSize() const
{
	auto bytesPerSample = fileReader.isMonolithic() ? sizeof(int16) : sizeof(float);

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

bool StreamingSamplerSound::isMonolithic() const
{
	return fileReader.isMonolithic();
}

AudioFormatReader* StreamingSamplerSound::createReaderForAnalysis()
{
	return fileReader.getReader();
}

String StreamingSamplerSound::getSampleStateAsString() const
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

String StreamingSamplerSound::getFileName(bool getFullPath /*= false*/) const {	return fileReader.getFileName(getFullPath); }

int64 StreamingSamplerSound::getHashCode() { return fileReader.getHashCode(); }


void StreamingSamplerSound::checkFileReference()
{
	fileReader.checkFileReference();
}


void StreamingSamplerSound::replaceFileReference(const String &newFileName)
{
	fileReader.setFile(newFileName);

	if (isMissing()) return;

	fileReader.openFileHandles();

	AudioFormatReader *reader = fileReader.getReader();

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

void StreamingSamplerSound::setSampleStart(int newSampleStart)
{
	if(sampleStart != newSampleStart && 
	   (! loopEnabled || (loopEnabled && loopStart > newSampleStart)))
	{
		sampleStart = newSampleStart;
		lengthChanged();
	}
}

void StreamingSamplerSound::setSampleStartModulation(int newModulationDelta)
{
	if(sampleStartMod != newModulationDelta)
	{
		ScopedLock sl(getSampleLock());

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
		loopStart = jmax(sampleStart, newLoopStart);
		loopChanged();
	}
}

void StreamingSamplerSound::setLoopEnd(int newLoopEnd)
{
	if (loopEnd != newLoopEnd)
	{
		loopEnd = jmin(sampleEnd, newLoopEnd);
		crossfadeArea = Range<int>((int)(loopEnd - crossfadeLength), (int)loopEnd);
		loopChanged();
	}
}

void StreamingSamplerSound::setLoopCrossfade(int newCrossfadeLength)
{
	if (crossfadeLength != newCrossfadeLength)
	{
		crossfadeLength = newCrossfadeLength;
		crossfadeArea = Range<int>((int)(loopEnd - crossfadeLength), (int)loopEnd);
		loopChanged();
	}
}


void StreamingSamplerSound::setSampleEnd(int newSampleEnd)
{
	if(sampleEnd != newSampleEnd && 
	   (! loopEnabled || (loopEnabled && loopEnd < newSampleEnd)))
	{
		sampleEnd = newSampleEnd;
		lengthChanged();
	}
}

void StreamingSamplerSound::lengthChanged()
{
	ScopedLock sl(getSampleLock());

	sampleLength = sampleEnd - sampleStart;

	setPreloadSize(preloadSize, true);
}

void StreamingSamplerSound::loopChanged()
{
	ScopedLock sl(getSampleLock());

	if(loopEnabled)
	{
		loopStart = jmax<int>(loopStart, sampleStart);
		loopEnd = jmin<int>(loopEnd, sampleEnd);
		loopLength = jmax<int>(0, loopEnd - loopStart);

		if (loopLength < 8192)
		{
			useSmallLoopBuffer = true;

			fileReader.openFileHandles();

			smallLoopBuffer = hlac::HiseSampleBuffer(!fileReader.isMonolithic(), 2, (int)loopLength);

			fileReader.readFromDisk(smallLoopBuffer, 0, loopLength, loopStart, false);

			closeFileHandle();

		}
		else
		{
			useSmallLoopBuffer = false;
			smallLoopBuffer.setSize(2, 0);
		}

		if(crossfadeLength != 0)
		{
			loopBuffer = hlac::HiseSampleBuffer(!fileReader.isMonolithic(), 2, (int)crossfadeLength);

			hlac::HiseSampleBuffer tempBuffer(!fileReader.isMonolithic(), 2, (int)crossfadeLength);

			// Calculate the fade in
			const int startCrossfade = loopStart - crossfadeLength;
			tempBuffer.clear();

			fileReader.openFileHandles();

			fileReader.readFromDisk(tempBuffer, 0, (int)crossfadeLength, startCrossfade + monolithOffset, false);
			
			tempBuffer.applyGainRamp(0, 0, (int)crossfadeLength, 0.0f, 1.0f);
			tempBuffer.applyGainRamp(1, 0, (int)crossfadeLength, 0.0f, 1.0f);

			hlac::HiseSampleBuffer::copy(loopBuffer, tempBuffer, 0, 0, (int)crossfadeLength);

			//FloatVectorOperations::copy(loopBuffer.getWritePointer(0, 0), tempBuffer.getReadPointer(0, 0), (int)crossfadeLength);
			//FloatVectorOperations::copy(loopBuffer.getWritePointer(1, 0), tempBuffer.getReadPointer(1, 0), (int)crossfadeLength);

			// Calculate the fade out
			tempBuffer.clear();

			const int endCrossfade = loopEnd - crossfadeLength;

			fileReader.readFromDisk(tempBuffer, 0, (int)crossfadeLength, endCrossfade + monolithOffset, false);
			
			tempBuffer.applyGainRamp(0, 0, (int)crossfadeLength, 1.0f, 0.0f);
			tempBuffer.applyGainRamp(1, 0, (int)crossfadeLength, 1.0f, 0.0f);

			hlac::HiseSampleBuffer::add(loopBuffer, tempBuffer, 0, 0, crossfadeLength);

			//FloatVectorOperations::add(loopBuffer.getWritePointer(0, 0), tempBuffer.getReadPointer(0, 0), (int)crossfadeLength);
			//FloatVectorOperations::add(loopBuffer.getWritePointer(1, 0), tempBuffer.getReadPointer(1, 0), (int)crossfadeLength);

			fileReader.closeFileHandles();
		}
	}
}

void StreamingSamplerSound::wakeSound() const {	fileReader.wakeSound(); }


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
	ScopedLock sl(getSampleLock());

	if (!fileReader.isUsed()) return;

	const bool wrapLoop = (uptime + samplesToCopy + sampleStart) > loopEnd;

	if(loopEnabled && loopLength != 0 && wrapLoop)
	{
		const int indexInLoop = (uptime + sampleStart - loopStart) % loopLength;

		const int numSamplesInThisLoop = (int)(loopLength - indexInLoop);

		if (useSmallLoopBuffer)
		{
			int numSamplesBeforeFirstWrap = 0;

			if (indexInLoop < 0)
			{
				numSamplesBeforeFirstWrap = loopStart - (uptime+sampleStart);
				fillInternal(sampleBuffer, numSamplesBeforeFirstWrap, uptime + (int)sampleStart, 0);
			}
			else
			{
				numSamplesBeforeFirstWrap = numSamplesInThisLoop;
				int startSample = indexInLoop;

				hlac::HiseSampleBuffer::copy(sampleBuffer, smallLoopBuffer, 0, startSample, numSamplesBeforeFirstWrap);

				//FloatVectorOperations::copy(sampleBuffer.getWritePointer(0, 0), smallLoopBuffer.getReadPointer(0, startSample), numSamplesBeforeFirstWrap);
				//FloatVectorOperations::copy(sampleBuffer.getWritePointer(1, 0), smallLoopBuffer.getReadPointer(1, startSample), numSamplesBeforeFirstWrap);
			}

			int numSamples = samplesToCopy - numSamplesBeforeFirstWrap;
			int indexInSampleBuffer = numSamplesBeforeFirstWrap;

			while (numSamples > (int)loopLength)
			{
				jassert(indexInSampleBuffer < sampleBuffer.getNumSamples());

				hlac::HiseSampleBuffer::copy(sampleBuffer, smallLoopBuffer, indexInSampleBuffer, 0, loopLength);

				//FloatVectorOperations::copy(sampleBuffer.getWritePointer(0, indexInSampleBuffer), smallLoopBuffer.getReadPointer(0, 0), loopLength);
				//FloatVectorOperations::copy(sampleBuffer.getWritePointer(1, indexInSampleBuffer), smallLoopBuffer.getReadPointer(1, 0), loopLength);

				numSamples -= (int)loopLength;
				indexInSampleBuffer += (int)loopLength;
			}

			hlac::HiseSampleBuffer::copy(sampleBuffer, smallLoopBuffer, indexInSampleBuffer, 0, numSamples);

			//FloatVectorOperations::copy(sampleBuffer.getWritePointer(0, indexInSampleBuffer), smallLoopBuffer.getReadPointer(0, 0), numSamples);
			//FloatVectorOperations::copy(sampleBuffer.getWritePointer(1, indexInSampleBuffer), smallLoopBuffer.getReadPointer(1, 0), numSamples);
		}

		// Loop is smaller than streaming buffers
		else if(loopLength < samplesToCopy)
		{
			const int numSamplesBeforeFirstWrap = numSamplesInThisLoop;
			
			int numSamples = samplesToCopy - numSamplesBeforeFirstWrap;
			int startSample = numSamplesBeforeFirstWrap;

			const int indexToUse = indexInLoop > 0 ? ((int)indexInLoop + (int)loopStart) : uptime + (int)sampleStart;
			fillInternal(sampleBuffer, numSamplesBeforeFirstWrap, indexToUse, 0);

			while(numSamples > (int)loopLength)
			{
				fillInternal(sampleBuffer, (int)loopLength, (int)loopStart, startSample);
				numSamples -= (int)loopLength;
				startSample += (int)loopLength;
			}

			fillInternal(sampleBuffer, numSamples, (int)loopStart, startSample);
		}

		// loop is bigger than streaming buffers and does not get wrapped
		else if(numSamplesInThisLoop > samplesToCopy)
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
	if(loopEnabled && Range<int>(uptime, uptime + samplesToCopy).intersects(crossfadeArea))
	{
		const int numSamplesBeforeCrossfade = jmax(0, crossfadeArea.getStart() - uptime);

		if(numSamplesBeforeCrossfade > 0)
		{
			fillInternal(sampleBuffer, numSamplesBeforeCrossfade, uptime, 0);
		}
		
		const int numSamplesInCrossfade = jmin(samplesToCopy - numSamplesBeforeCrossfade, (int)crossfadeLength);

		if(numSamplesInCrossfade > 0)
		{
			const int indexInLoopBuffer = jmax(0, uptime - crossfadeArea.getStart());

			hlac::HiseSampleBuffer::copy(sampleBuffer, loopBuffer, numSamplesBeforeCrossfade, indexInLoopBuffer, numSamplesInCrossfade);

			//FloatVectorOperations::copy(sampleBuffer.getWritePointer(0, numSamplesBeforeCrossfade), loopBuffer.getReadPointer(0, indexInLoopBuffer), numSamplesInCrossfade);
			//FloatVectorOperations::copy(sampleBuffer.getWritePointer(1, numSamplesBeforeCrossfade), loopBuffer.getReadPointer(1, indexInLoopBuffer), numSamplesInCrossfade);		
		}

		// Should be taken care by higher logic (fillSampleBuffer should wrap the loop)
		jassert((samplesToCopy - numSamplesBeforeCrossfade - numSamplesInCrossfade) == 0);
	}

	// All samples can be fetched from the preload buffer
	else if(uptime + samplesToCopy < internalPreloadSize)
	{
		// the preload buffer has already the samplestart offset
		const int indexInPreloadBuffer = uptime - (int)sampleStart;

		jassert(indexInPreloadBuffer >= 0);

		if (indexInPreloadBuffer + samplesToCopy < preloadBuffer.getNumSamples())
		{
			hlac::HiseSampleBuffer::copy(sampleBuffer, preloadBuffer, offsetInBuffer, indexInPreloadBuffer, samplesToCopy);

			//FloatVectorOperations::copy(sampleBuffer.getWritePointer(0, offsetInBuffer), preloadBuffer.getReadPointer(0, indexInPreloadBuffer), samplesToCopy);
			//FloatVectorOperations::copy(sampleBuffer.getWritePointer(1, offsetInBuffer), preloadBuffer.getReadPointer(1, indexInPreloadBuffer), samplesToCopy);
		}
		else
		{
			jassertfalse;

			sampleBuffer.clear();

			//FloatVectorOperations::clear(sampleBuffer.getWritePointer(0), sampleBuffer.getNumSamples());
			//FloatVectorOperations::clear(sampleBuffer.getWritePointer(1), sampleBuffer.getNumSamples());
		}
	}
	
	// Read all samples from disk
	else
	{
		fileReader.readFromDisk(sampleBuffer, offsetInBuffer, samplesToCopy, uptime + monolithOffset, true);
    }
}

// =============================================================================================================================================== StreamingSamplerSound::FileReader methods


StreamingSamplerSound::FileReader::FileReader(StreamingSamplerSound *soundForReader, ModulatorSamplerSoundPool *pool_) :
pool(pool_),
sound(soundForReader),
missing(true),
hashCode(0),
voiceCount(0),
fileHandlesOpen(false)
{}

StreamingSamplerSound::FileReader::~FileReader()
{
	ScopedWriteLock sl(fileAccessLock);

	memoryReader = nullptr;
	normalReader = nullptr;
}

void StreamingSamplerSound::FileReader::setFile(const String &fileName)
{
	monolithicInfo = nullptr;

	if (File::isAbsolutePath(fileName))
	{
		loadedFile = File(fileName);

		const String fileExtension = loadedFile.getFileExtension();

        fileFormatSupportsMemoryReading = fileExtension.contains("wav") || fileExtension.contains("aif");// || fileExtension.contains("hlac");

		hashCode = loadedFile.hashCode64();
	}
	else
	{
		faultyFileName = fileName;
		loadedFile = File();
	}
}

String StreamingSamplerSound::FileReader::getFileName(bool getFullPath)
{
	if (monolithicInfo != nullptr)
	{
		return monolithicName;
	}

    if(faultyFileName.isNotEmpty())
    {
        if(getFullPath)
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

	if(missing) missing = !loadedFile.existsAsFile();// && getReader() == nullptr;
}

void StreamingSamplerSound::FileReader::refreshFileInformation()
{
	checkFileReference();

	if (!missing)
	{
        faultyFileName = String();
        
		const String fileExtension = loadedFile.getFileExtension();

        fileFormatSupportsMemoryReading = fileExtension.compareIgnoreCase(".wav") || fileExtension.startsWithIgnoreCase(".aif");// || fileExtension.startsWithIgnoreCase("hlac");

		hashCode = loadedFile.hashCode64();
	}
}



AudioFormatReader * StreamingSamplerSound::FileReader::getReader()
{
	if (!fileHandlesOpen) openFileHandles();

	if (memoryReader != nullptr) return memoryReader;
	else if (normalReader != nullptr) return normalReader;
	else return nullptr;
}

void StreamingSamplerSound::FileReader::wakeSound()
{
	if (!fileFormatSupportsMemoryReading) return;

	if (memoryReader != nullptr && !memoryReader->getMappedSection().isEmpty()) memoryReader->touchSample(sound->sampleStart + sound->monolithOffset);
}

    
void StreamingSamplerSound::FileReader::openFileHandles(NotificationType notifyPool)
{


	if (fileHandlesOpen)
	{
		jassert(memoryReader != nullptr || normalReader != nullptr);
		return;
	}
	else
	{
		jassert(memoryReader == nullptr || normalReader == nullptr);

		ScopedWriteLock sl(fileAccessLock);

		fileHandlesOpen = true;

		memoryReader = nullptr;
		normalReader = nullptr;

		if (monolithicInfo != nullptr)
		{
#if USE_FALLBACK_READERS_FOR_MONOLITH
            
            normalReader = monolithicInfo->createFallbackReader(monolithicIndex, monolithicChannelIndex);
#else
            
			normalReader = monolithicInfo->createMonolithicReader(monolithicIndex, monolithicChannelIndex);

			//if (memoryReader != nullptr)
			//{
			//	memoryReader->mapSectionOfFile(Range<int64>((int64)(sound->sampleStart) + (int64)(sound->monolithOffset), (int64)(sound->sampleEnd)));
			//}
#endif
            
            sampleLength = getMonolithLength();
            
		}
		else
		{
			if (fileFormatSupportsMemoryReading)
			{

				AudioFormat* format = pool->afm.findFormatForFileExtension(loadedFile.getFileExtension());

				if (format != nullptr)
				{
					memoryReader = format->createMemoryMappedReader(loadedFile);

					if (memoryReader != nullptr)
					{
						memoryReader->mapSectionOfFile(Range<int64>((int64)(sound->sampleStart) + (int64)(sound->monolithOffset), (int64)(sound->sampleEnd)));
                        
                        sampleLength = memoryReader->getMappedSection().getLength();
                        
					}
				}
			}
            
            
			normalReader = pool->afm.createReaderFor(loadedFile);
            
            sampleLength = normalReader != nullptr ? normalReader->lengthInSamples : 0;
            
		}

#if USE_BACKEND
		if(monolithicInfo == nullptr && notifyPool == sendNotification) pool->increaseNumOpenFileHandles();
#else
		ignoreUnused(notifyPool);
#endif
	}
}


void StreamingSamplerSound::FileReader::closeFileHandles(NotificationType notifyPool)
{
	if (monolithicIndex != -1) return; // don't close the reader for monolithic files...

	if (voiceCount.get() == 0)
	{
		ScopedWriteLock sl(fileAccessLock);

		fileHandlesOpen = false;

		memoryReader = nullptr;
		normalReader = nullptr;

		if (monolithicInfo == nullptr && notifyPool == sendNotification) pool->decreaseNumOpenFileHandles();
	}
}


    

void StreamingSamplerSound::FileReader::readFromDisk(hlac::HiseSampleBuffer &buffer, int startSample, int numSamples, int readerPosition, bool useMemoryMappedReader)
{
	if (!fileHandlesOpen) openFileHandles(sendNotification);

    
#if USE_SAMPLE_DEBUG_COUNTER
    
    float* l = buffer.getWritePointer(0, startSample);
    float* r = buffer.getWritePointer(1, startSample);
    
    int v = readerPosition;
    
    for(int i = 0; i < numSamples; i++)
    {
        l[i] = (float)v;
        r[i] = (float)v;
        
        v++;
    }
    
    return;
#endif
    
    
	buffer.clear(startSample, numSamples);
    
    //FloatVectorOperations::clear(buffer.getWritePointer(0, startSample), numSamples);
    //FloatVectorOperations::clear(buffer.getWritePointer(1, startSample), numSamples);
    
	if (useMemoryMappedReader)
	{
		if (memoryReader != nullptr && memoryReader->getMappedSection().contains(Range<int64>(readerPosition, readerPosition + numSamples)))
		{
			ScopedReadLock sl(fileAccessLock);

			if (buffer.isFloatingPoint())
			{
				memoryReader->read(buffer.getFloatBufferForFileReader(), startSample, numSamples, readerPosition, true, true);
			}
			else
			{
				jassertfalse;
			}

			return;
		}
	}
	
	if (normalReader != nullptr)
	{
		ScopedReadLock sl(fileAccessLock);


		if (buffer.isFloatingPoint())
			normalReader->read(buffer.getFloatBufferForFileReader(), startSample, numSamples, readerPosition, true, true);
		else
			normalReader->read(buffer.getArrayOfFixedBufferPointers(startSample), buffer.getNumChannels(), readerPosition, numSamples, false);

	}
	else
	{
		// Something is wrong so clear the buffer to be safe...
		buffer.clear(startSample, numSamples);
	}
}


float StreamingSamplerSound::FileReader::calculatePeakValue()
{
	float l1, l2, r1, r2;

	openFileHandles();

	AudioFormatReader *readerToUse = getReader();

	if (readerToUse != nullptr) readerToUse->readMaxLevels(sound->sampleStart + sound->monolithOffset, sound->sampleLength, l1, l2, r1, r2);
	else return 0.0f;

	closeFileHandles();

	const float maxLeft = jmax<float>(-l1, l2);
	const float maxRight = jmax<float>(-r1, r2);

	return jmax<float>(maxLeft, maxRight);
}


AudioFormatReader* StreamingSamplerSound::FileReader::createMonolithicReaderForPreview()
{
	if (monolithicInfo != nullptr)
	{
		auto m = monolithicInfo->createFallbackReader(monolithicIndex, monolithicChannelIndex);

		return m;

#if 0
#if USE_FALLBACK_READERS_FOR_MONOLITH
     
        auto m = monolithicInfo->createFallbackReader(monolithicIndex, monolithicChannelIndex);
#else
		auto m =  monolithicInfo->createMonolithicReader(monolithicIndex, monolithicChannelIndex);
		//m->mapSectionOfFile(Range<int64>((int64)(sound->sampleStart) + (int64)(sound->monolithOffset), (int64)(sound->sampleEnd)));
#endif
        
        
		return m;
#endif
	}
	else
	{
		return nullptr;
	}
}

void StreamingSamplerSound::FileReader::setMonolithicInfo(MonolithInfoToUse* info, int channelIndex, int sampleIndex)
{
	monolithicInfo = info;
	monolithicIndex = sampleIndex;
	missing = (sampleIndex == -1);
	monolithicName = info->getFileName(channelIndex, sampleIndex);
	monolithicChannelIndex = channelIndex;
}

// =============================================================================================================================================== SampleLoader methods

SampleLoader::SampleLoader(SampleThreadPool *pool_) :
SampleThreadPoolJob("SampleLoader"),
backgroundPool(pool_),
writeBufferIsBeingFilled(false),
sound(0),
readIndex(0),
readIndexDouble(0.0),
idealBufferSize(0),
minimumBufferSizeForSamplesPerBlock(0),
positionInSampleFile(0),
isReadingFromPreloadBuffer(true),
sampleStartModValue(0),
readBuffer(nullptr),
writeBuffer(nullptr),
diskUsage(0.0),
lastCallToRequestData(0.0),
b1(true, 2, 0),
b2(true, 2, 0)
{
	unmapper.setLoader(this);

//     for(int i = 0; i < NUM_UNMAPPERS; i++)
//     {
//         unmappers[i].setLoader(this);
//     }
//     
	setBufferSize(BUFFER_SIZE_FOR_STREAM_BUFFERS);
}

SampleLoader::~SampleLoader()
{
	b1.setSize(2, 0);
	b2.setSize(2, 0);
}

/** Sets the buffer size in samples. */
void SampleLoader::setBufferSize(int newBufferSize)
{
	ScopedLock sl(getLock());

	idealBufferSize = newBufferSize;

	refreshBufferSizes();
}

bool SampleLoader::assertBufferSize(int minimumBufferSize)
{
	minimumBufferSizeForSamplesPerBlock = minimumBufferSize;

	refreshBufferSizes();

	return true;
}

void SampleLoader::startNote(StreamingSamplerSound const *s, int startTime)
{
	diskUsage = 0.0;

	sound = s;
	
	s->wakeSound();

	sampleStartModValue = (int)startTime;

	auto localReadBuffer = &s->getPreloadBuffer();
	auto localWriteBuffer = &b1;

	// the read pointer will be pointing directly to the preload buffer of the sample sound
	readBuffer = localReadBuffer;
	writeBuffer = localWriteBuffer;

    lastSwapPosition = 0.0;
    
	readIndex = startTime;
	readIndexDouble = (double)startTime;
    
	isReadingFromPreloadBuffer = true;

	// Set the sampleposition to (1 * bufferSize) because the first buffer is the preload buffer
	positionInSampleFile = (int)localReadBuffer->getNumSamples();
	
    voiceCounterWasIncreased = false;
    
	// The other buffer will be filled on the next free thread pool slot
	requestNewData();
};

void SampleLoader::setStreamingBufferDataType(bool shouldBeFloat)
{
	ScopedLock sl(getLock());

	b1 = hlac::HiseSampleBuffer(shouldBeFloat, 2, 0);
	b2 = hlac::HiseSampleBuffer(shouldBeFloat, 2, 0);

	refreshBufferSizes();
}

StereoChannelData SampleLoader::fillVoiceBuffer(hlac::HiseSampleBuffer &voiceBuffer, double numSamples) const
{
	auto localReadBuffer = readBuffer.get();
	auto localWriteBuffer = writeBuffer.get();

#if 0

	logger->checkAssertion(nullptr, DebugLogger::Location::SampleLoaderPreFillVoiceBufferRead, localReadBuffer != nullptr, 1013);
	logger->checkAssertion(nullptr, DebugLogger::Location::SampleLoaderPreFillVoiceBufferWrite, localWriteBuffer != nullptr, 1014);
	logger->checkAssertion(nullptr, DebugLogger::Location::SampleLoaderPreFillVoiceBufferRead, localReadBuffer->getNumSamples() != 0, 1015);
	logger->checkAssertion(nullptr, DebugLogger::Location::SampleLoaderPreFillVoiceBufferWrite, localWriteBuffer->getNumSamples() != 0, 1016);

	const bool leftROK = logger->checkSampleData(nullptr, DebugLogger::Location::SampleLoaderPreFillVoiceBufferRead, true, localReadBuffer->getReadPointer(0), localReadBuffer->getNumSamples());
	const bool rightROK = logger->checkSampleData(nullptr, DebugLogger::Location::SampleLoaderPreFillVoiceBufferRead, false, localReadBuffer->getReadPointer(1), localReadBuffer->getNumSamples());
	const bool leftWOK = logger->checkSampleData(nullptr, DebugLogger::Location::SampleLoaderPreFillVoiceBufferWrite, true, localWriteBuffer->getReadPointer(0), localWriteBuffer->getNumSamples());
	const bool rightWOK = logger->checkSampleData(nullptr, DebugLogger::Location::SampleLoaderPreFillVoiceBufferWrite, false, localWriteBuffer->getReadPointer(1), localWriteBuffer->getNumSamples());

	if (!leftROK || !rightROK)
	{
		String c;
		NewLine nl;

		c << "Sample Error (Read Buffer Access). Dumping Variables:  " << nl;
		c << "- Samples for fill operation : " << String(numSamples, 2) << nl;
		c << "- Read index: " << String(readIndexDouble, 3) << nl;
		c << "- numSamples in ReadBuffer: " << String(localReadBuffer->getNumSamples()) << nl;
		c << "- numChannels in ReadBuffer: " << String(localReadBuffer->getNumChannels()) << nl;
		c << "- Sample length: " << String(sound.get()->getSampleLength()) << nl;
		c << "- Position in sample file: " << positionInSampleFile << nl;
		c << "- isUsingPreloadBuffer: " << (isReadingFromPreloadBuffer ? "yes" : "no") << nl;

		logger->logMessage(c);

	}

	if (!leftWOK || !rightWOK)
	{
		String c;
		NewLine nl;

		c << "Sample Error (Write Buffer Access). Dumping Variables:  " << nl;
		c << "- Samples for fill operation : " << String(numSamples, 2) << nl;
		c << "- Thread is busy: " << (writeBufferIsBeingFilled ? "yes" : "no") << nl;
		c << "- Read index: " << String(readIndexDouble, 3) << nl;
		c << "- numSamples in WriteBuffer: " << String(localWriteBuffer->getNumSamples()) << nl;
		c << "- numChannels in WriteBuffer: " << String(localWriteBuffer->getNumChannels()) << nl;
		c << "- Sample length: " << String(sound.get()->getSampleLength()) << nl;
		c << "- Position in sample file: " << positionInSampleFile << nl;
		c << "- isUsingPreloadBuffer: " << (isReadingFromPreloadBuffer ? "yes" : "no") << nl;

		logger->logMessage(c);

	}

#endif

	const int numSamplesInBuffer = localReadBuffer->getNumSamples();
	const int maxSampleIndexForFillOperation = (int)(readIndexDouble + numSamples)+ 1; // Round up the samples

	if (maxSampleIndexForFillOperation >= numSamplesInBuffer) // Check because of preloadbuffer style
	{
		const int indexBeforeWrap = jmax<int>(0, (int)(readIndexDouble));
		const int numSamplesInFirstBuffer = localReadBuffer->getNumSamples() - indexBeforeWrap;

		jassert(numSamplesInFirstBuffer >= 0);

		if (numSamplesInFirstBuffer > 0)
		{
			hlac::HiseSampleBuffer::copy(voiceBuffer, *localReadBuffer, 0, indexBeforeWrap, numSamplesInFirstBuffer);

			//voiceBuffer.copyFrom(0, 0, *localReadBuffer, 0, indexBeforeWrap, numSamplesInFirstBuffer);
			//voiceBuffer.copyFrom(1, 0, *localReadBuffer, 1, indexBeforeWrap, numSamplesInFirstBuffer);
		}

		const int offset = numSamplesInFirstBuffer;
		//remaining = (int)(numSamples)+1 - offset;

		const int numSamplesAvailableInSecondBuffer = localWriteBuffer->getNumSamples() - offset;

		if ( (numSamplesAvailableInSecondBuffer > 0) && (numSamplesAvailableInSecondBuffer < localWriteBuffer->getNumSamples()))
		{
			const int numSamplesToCopyFromSecondBuffer = jmin<int>(numSamplesAvailableInSecondBuffer, voiceBuffer.getNumSamples() - offset);

			if (writeBufferIsBeingFilled)
			{
				voiceBuffer.clear(offset, numSamplesToCopyFromSecondBuffer);

				//voiceBuffer.clear(0, offset, numSamplesToCopyFromSecondBuffer);
				//voiceBuffer.clear(1, offset, numSamplesToCopyFromSecondBuffer);
			}
			else
			{
				hlac::HiseSampleBuffer::copy(voiceBuffer, *localWriteBuffer, offset, 0, numSamplesToCopyFromSecondBuffer);

				//voiceBuffer.copyFrom(0, offset, *localWriteBuffer, 0, 0, numSamplesToCopyFromSecondBuffer);
				//voiceBuffer.copyFrom(1, offset, *localWriteBuffer, 1, 0, numSamplesToCopyFromSecondBuffer);
			}
		}
		else
		{
			// The streaming buffers must be greater than the block size!
			jassertfalse;

			voiceBuffer.clear();

			//FloatVectorOperations::clear(voiceBuffer.getWritePointer(0), voiceBuffer.getNumSamples());
			//FloatVectorOperations::clear(voiceBuffer.getWritePointer(1), voiceBuffer.getNumSamples());
		}
		
		StereoChannelData returnData;

		returnData.isFloatingPoint = localReadBuffer->isFloatingPoint();
		returnData.leftChannel = voiceBuffer.getReadPointer(0);
		returnData.rightChannel = voiceBuffer.getReadPointer(1);

#if 0 && LOG_SAMPLE_RENDERING

		const bool leftOK = logger->checkSampleData(nullptr, DebugLogger::Location::SampleLoaderPostFillVoiceBufferWrapped, true, returnData.leftChannel, voiceBuffer.getNumSamples());
		const bool rightOK = logger->checkSampleData(nullptr, DebugLogger::Location::SampleLoaderPostFillVoiceBufferWrapped, true, returnData.rightChannel, voiceBuffer.getNumSamples());

		if (!leftOK || !rightOK)
		{
			String c;
			NewLine nl;

			c << "Sample Error (Wrapped). Dumping Variables:  " << nl;
			c << "- Samples for fill operation : " << String(numSamples, 2) << nl;
			c << "- Read index: " << String(readIndexDouble, 3) << nl;
			c << "- numSamples in VoiceBuffer: " << String(voiceBuffer.getNumSamples()) << nl;
			c << "- Index before wrap: " << String(indexBeforeWrap) << nl;
			c << "- Sample length: " << String(sound.get()->getSampleLength()) << nl;
			c << "- Position in sample file: " << positionInSampleFile << nl;
			c << "- isUsingPreloadBuffer: " << (isReadingFromPreloadBuffer ? "yes" : "no") << nl;

			logger->logMessage(c);
		}

#endif

#if USE_SAMPLE_DEBUG_COUNTER
      
        const float *l = voiceBuffer.getReadPointer(0, 0);
        const float *r = voiceBuffer.getReadPointer(1, 0);
        
        float ll = l[0];
        float lr = r[0];
        
        for(int i = 1; i < voiceBuffer.getNumSamples(); i++)
        {
            const float tl = l[i];
            const float tr = r[i];
            
            jassert(tl == tr);
            jassert(tl - ll == 1.0f);
            ll = tl;
            lr = tr;
        }
        
        
#endif
        
		return returnData;
	}
	else
	{
		const int index = (int)readIndexDouble;

		StereoChannelData returnData;

		returnData.isFloatingPoint = localReadBuffer->isFloatingPoint();
		returnData.leftChannel = localReadBuffer->getReadPointer(0, index);
		returnData.rightChannel = localReadBuffer->getReadPointer(1, index);


#if 0 && LOG_SAMPLE_RENDERING

		bool leftOK = logger->checkSampleData(nullptr, DebugLogger::Location::SampleLoaderPostFillVoiceBuffer, true, returnData.leftChannel, localReadBuffer->getNumSamples() - index);
		bool rightOK = logger->checkSampleData(nullptr, DebugLogger::Location::SampleLoaderPostFillVoiceBuffer, true, returnData.rightChannel, localReadBuffer->getNumSamples() - index);

		if (!leftOK || !rightOK)
		{
			String c;
			NewLine nl;

			c << "Sample Error (Unwrapped). Dumping Variables:  " << nl;
			c << "- Samples for fill operation : " << String(numSamples, 2) << nl;
			c << "- Read index: " << String(index) << nl;
			c << "- numSamples: " << String(localReadBuffer->getNumSamples() - index) << nl;
			c << "- Sample length: " << String(sound.get()->getSampleLength()) << nl;
			c << "- Position in sample file: " << positionInSampleFile << nl;
			c << "- isUsingPreloadBuffer: " << (isReadingFromPreloadBuffer ? "yes" : "no") << nl;
			
			logger->logMessage(c);
		}

#endif

		return returnData;
	}
}

bool SampleLoader::advanceReadIndex(double uptime)
{
	const int numSamplesInBuffer = readBuffer.get()->getNumSamples();
    readIndexDouble = uptime - lastSwapPosition;
    
    if(readIndexDouble >= numSamplesInBuffer)
	{
        lastSwapPosition = (double)positionInSampleFile;
		positionInSampleFile += getNumSamplesForStreamingBuffers();
        readIndexDouble = uptime - lastSwapPosition;
        
        swapBuffers();
		const bool queueIsFree = requestNewData();
        
        return queueIsFree;
	}
    
    return true;
}

bool SampleLoader::requestNewData()
{
    //ADD_GLITCH_DETECTOR("Requesting new sample data");

#if KILL_VOICES_WHEN_STREAMING_IS_BLOCKED
    if(this->isQueued())
    {
        writeBuffer.get()->clear();
		
        cancelled = true;
        backgroundPool->notify();
        return false;
    }
    else
    {
        backgroundPool->addJob(this, false);
        return true;
    }
#else
    backgroundPool->addJob(this, false);
    return true;
#endif
};


SampleThreadPoolJob::JobStatus SampleLoader::runJob()
{
    if(cancelled)
    {
        cancelled = false;
        return SampleThreadPoolJob::jobHasFinished;
    }
    
    const double readStart = Time::highResolutionTicksToSeconds(Time::getHighResolutionTicks());

	if (writeBufferIsBeingFilled)
	{
		return SampleThreadPoolJob::jobNeedsRunningAgain;
	}

	writeBufferIsBeingFilled = true; // A poor man's mutex but gets the job done.

	const StreamingSamplerSound *localSound = sound.get();

    if(!voiceCounterWasIncreased && localSound != nullptr)
    {
        localSound->increaseVoiceCount();
        voiceCounterWasIncreased = true;
    }
    
    fillInactiveBuffer();
    
    writeBufferIsBeingFilled = false;
    
    const double readStop = Time::highResolutionTicksToSeconds(Time::getHighResolutionTicks());
    const double readTime = (readStop - readStart);
    const double timeSinceLastCall = readStop - lastCallToRequestData;
	const float diskUsageThisTime = jmax<float>(diskUsage.get(), (float)(readTime / timeSinceLastCall));
    diskUsage = diskUsageThisTime;
    lastCallToRequestData = readStart;
    
    return SampleThreadPoolJob::JobStatus::jobHasFinished;
}

void SampleLoader::fillInactiveBuffer()
{
	const StreamingSamplerSound *localSound = sound.get();

	if (localSound == nullptr) return;

	if(localSound != nullptr)
	{
		if(localSound->hasEnoughSamplesForBlock(positionInSampleFile + getNumSamplesForStreamingBuffers()))
		{
			localSound->fillSampleBuffer(*writeBuffer.get(), getNumSamplesForStreamingBuffers(), (int)positionInSampleFile);
		}
		else if (localSound->hasEnoughSamplesForBlock(positionInSampleFile))
		{
			const int numSamplesToFill = (int)localSound->getSampleLength() - positionInSampleFile;
			const int numSamplesToClear = getNumSamplesForStreamingBuffers() - numSamplesToFill;

			localSound->fillSampleBuffer(*writeBuffer.get(), numSamplesToFill, (int)positionInSampleFile);



			writeBuffer.get()->clear(numSamplesToFill, numSamplesToClear);
		}
		else
		{
			writeBuffer.get()->clear();
		}
        
		logger->checkAssertion(nullptr, DebugLogger::Location::SampleLoaderReadOperation, localSound != nullptr, 1174);

#if 0 && LOG_SAMPLE_RENDERING

		logger->checkSampleData(nullptr, DebugLogger::Location::SampleLoaderReadOperation, true, writeBuffer.get()->getReadPointer(0, 0), writeBuffer.get()->getNumSamples());
		logger->checkSampleData(nullptr, DebugLogger::Location::SampleLoaderReadOperation, false, writeBuffer.get()->getReadPointer(1, 0), writeBuffer.get()->getNumSamples());

#endif

#if USE_SAMPLE_DEBUG_COUNTER
      
        DBG(positionInSampleFile);
        
        const float *l = writeBuffer.get()->getReadPointer(0);
        const float *r = writeBuffer.get()->getReadPointer(1);
        
        int co = (int)positionInSampleFile;
        
        for(int i = 0; i < writeBuffer.get()->getNumSamples(); i++)
        {
            
            const float tl = l[i];
            const float tr = r[i];
            const float expected = (float)co;
            
            jassert(tl == tr);
            jassert(tl == 0.0f || (abs(expected-tl) < 0.00001f));
            
            co++;
            
        }
        
#endif
        
	}
};
	
void SampleLoader::refreshBufferSizes()
{
	const int numSamplesToUse = jmax<int>(idealBufferSize, minimumBufferSizeForSamplesPerBlock);

	if (getNumSamplesForStreamingBuffers() < numSamplesToUse)
	{
		ProcessorHelpers::increaseBufferIfNeeded(b1, numSamplesToUse);
		ProcessorHelpers::increaseBufferIfNeeded(b2, numSamplesToUse);

		readBuffer = &b1;
		writeBuffer = &b2;

		reset();
	}
}

bool SampleLoader::swapBuffers()
{
	auto localReadBuffer = readBuffer.get();

	if(localReadBuffer == &b1)
	{
		readBuffer = &b2;
		writeBuffer = &b1;
	}
	else // This condition will also be true if the read pointer points at the preload buffer
	{
		readBuffer = &b1;
		writeBuffer = &b2;
	}

	isReadingFromPreloadBuffer = false;
	sampleStartModValue = 0;

	return writeBufferIsBeingFilled == false;
};

// ==================================================================================================== StreamingSamplerVoice methods

StreamingSamplerVoice::StreamingSamplerVoice(SampleThreadPool *pool):
loader(pool),
sampleStartModValue(0)
{
	pitchData = nullptr;
};

void StreamingSamplerVoice::startNote (int /*midiNoteNumber*/, 
									   float /*velocity*/, 
									   SynthesiserSound* s, 
									   int /*currentPitchWheelPosition*/)
{
	StreamingSamplerSound *sound = dynamic_cast<StreamingSamplerSound*>(s);

	if(sound != nullptr && sound->getSampleLength() > 0)
	{
		loader.startNote(sound, sampleStartModValue);

		jassert(sound != nullptr);
		sound->wakeSound();

		voiceUptime = (double)sampleStartModValue;

		// You have to call setPitchFactor() before startNote().
		jassert(uptimeDelta != 0.0);

		// Resample if sound has different samplerate than the audio sample rate
		uptimeDelta *= (sound->getSampleRate() / getSampleRate());

		uptimeDelta = jmin<double>((double)MAX_SAMPLER_PITCH, uptimeDelta);

        isActive = true;
        
	}
	else
	{
		resetVoice();
	}
}

#define INTERPOLATE_SSE 1


template <typename SignalType> void interpolateStereoSamples(const SignalType* inL, const SignalType* inR, const float* pitchData, float* outL, float* outR, int startSample, double indexInBuffer, double uptimeDelta, int numSamples, bool isFloat)
{
	const float gainFactor = isFloat ? 1.0 :  (1.0f / (float)INT16_MAX);

	if (pitchData != nullptr)
	{
		pitchData += startSample;

		float indexInBufferFloat = (float)indexInBuffer;

		for(int i = 0; i < numSamples; i++)
		{
			const int pos = int(indexInBufferFloat);
			const float alpha = indexInBufferFloat - (float)pos;
			const float invAlpha = 1.0f - alpha;

			float l = ((float)inL[pos] * invAlpha + (float)inL[pos + 1] * alpha);
			float r = ((float)inR[pos] * invAlpha + (float)inR[pos + 1] * alpha);

			outL[i] = l * gainFactor;
			outR[i] = r * gainFactor;

			jassert(*pitchData <= (float)MAX_SAMPLER_PITCH);

			indexInBufferFloat += pitchData[i];
		}
	}
	else
	{

		float indexInBufferFloat = (float)indexInBuffer;
		const float uptimeDeltaFloat = (float)uptimeDelta;

		while (numSamples > 0)
		{
			const int pos = int(indexInBufferFloat);
			const float alpha = indexInBufferFloat - (float)pos;
			const float invAlpha = 1.0f - alpha;

			float l = ((float)inL[pos] * invAlpha + (float)inL[pos + 1] * alpha);
			float r = ((float)inR[pos] * invAlpha + (float)inR[pos + 1] * alpha);

			*outL++ = l * gainFactor;
			*outR++ = r * gainFactor;

			indexInBufferFloat += uptimeDeltaFloat;

			numSamples--;
		}
	}
}


void StreamingSamplerVoice::renderNextBlock(AudioSampleBuffer &outputBuffer, int startSample, int numSamples)
{
	const StreamingSamplerSound *sound = loader.getLoadedSound();

#if USE_SAMPLE_DEBUG_COUNTER
    const int startDebug = startSample;
    const int numDebug = numSamples;
#endif
    
	if(sound != nullptr)
	{
		const double startAlpha = fmod(voiceUptime, 1.0);
		
		jassert(pitchCounter != 0);

		auto tempVoiceBuffer = getTemporaryVoiceBuffer();

		jassert(tempVoiceBuffer != nullptr);

		tempVoiceBuffer->clear();
        
		// Copy the not resampled values into the voice buffer.
		StereoChannelData data = loader.fillVoiceBuffer(*tempVoiceBuffer, pitchCounter + startAlpha);

#if 0 && LOG_SAMPLE_RENDERING

		logger->checkAssertion(nullptr, DebugLogger::Location::SampleVoiceBufferFill, tempVoiceBuffer != nullptr, 1336);
		logger->checkAssertion(nullptr, DebugLogger::Location::SampleVoiceBufferFill, pitchCounter != 0, 1337);
		logger->checkAssertion(nullptr, DebugLogger::Location::SampleVoiceBufferFill, sound != nullptr, 1338);

		logger->checkSampleData(nullptr, DebugLogger::Location::SampleVoiceBufferFill, true, data.leftChannel, (int)(pitchCounter + startAlpha));
		logger->checkSampleData(nullptr, DebugLogger::Location::SampleVoiceBufferFill, false, data.rightChannel, (int)(pitchCounter + startAlpha));
#endif

		
		float* outL = outputBuffer.getWritePointer(0, startSample);
		float* outR = outputBuffer.getWritePointer(1, startSample);
		
		const int startFixed = startSample;
		const int numSamplesFixed = numSamples;


#if USE_SAMPLE_DEBUG_COUNTER
        jassert((int)voiceUptime == data.leftChannel[0]);
#endif
        
		double indexInBuffer = startAlpha;

		if (data.isFloatingPoint)
		{
			const float* const inL = static_cast<const float*>(data.leftChannel);
			const float* const inR = static_cast<const float*>(data.rightChannel);

			interpolateStereoSamples(inL, inR, pitchData, outL, outR, startSample, indexInBuffer, uptimeDelta, numSamples, true);
		}
		else
		{
			const int16* const inL = static_cast<const int16*>(data.leftChannel);
			const int16* const inR = static_cast<const int16*>(data.rightChannel);

			interpolateStereoSamples(inL, inR, pitchData, outL, outR, startSample, indexInBuffer, uptimeDelta, numSamples, false);
			
		}

		

#if USE_SAMPLE_DEBUG_COUNTER 
        
        for(int i = startDebug; i < numDebug; i++)
        {
            const float l = outputBuffer.getSample(0, i);
            const float r = outputBuffer.getSample(1, i);
            
            jassert(l == r);
            jassert((abs(l - voiceUptime) < 0.000001) || l == 0.0f);
            
            voiceUptime += uptimeDelta;
            
        }
        
        outputBuffer.clear();
#else
        voiceUptime += pitchCounter;
#endif
        
        if(!loader.advanceReadIndex(voiceUptime))
        {
			logger->addStreamingFailure(voiceUptime);

			outputBuffer.clear(startFixed, numSamplesFixed);

            resetVoice();
            return;
        }

		const bool enoughSamples = sound->hasEnoughSamplesForBlock((int)(voiceUptime));// +numSamples * MAX_SAMPLER_PITCH));

#if LOG_SAMPLE_RENDERING
		logger->checkSampleData(nullptr, DebugLogger::Location::SampleVoiceBufferFillPost, true, outputBuffer.getReadPointer(0, startFixed), numSamplesFixed);
		logger->checkSampleData(nullptr, DebugLogger::Location::SampleVoiceBufferFillPost, false, outputBuffer.getReadPointer(1, startFixed), numSamplesFixed);
#endif

		if(!enoughSamples) resetVoice();
	}
    else
    {
        resetVoice();
    }
};
