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

StreamingSamplerSound::StreamingSamplerSound(HiseMonolithAudioFormat *info, int channelIndex, int sampleIndex):
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

void StreamingSamplerSound::setPreloadSize(int newPreloadSize, bool forceReload)
{
    const bool preloadSizeChanged = preloadSize == newPreloadSize;
    const bool streamingDeactivated = newPreloadSize == -1 && entireSampleLoaded;
    
	if(!forceReload && (preloadSizeChanged || streamingDeactivated)) return;

	ScopedLock sl(getSampleLock());

    const bool sampleDeactivated = !hasActiveState() || newPreloadSize == 0;
    
	if (sampleDeactivated)
	{
		internalPreloadSize = 0;
		preloadSize = 0;
		preloadBuffer = AudioSampleBuffer();
		preloadBuffer.setSize(2, 0);
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

	preloadBuffer = AudioSampleBuffer();
	
	try
	{
		preloadBuffer.setSize(2, internalPreloadSize, false, false, false);
	}
	catch (std::bad_alloc e)
	{
		preloadBuffer.setSize(2, 0);

		throw StreamingSamplerSound::LoadingError(getFileName(), "Preload error (max memory exceeded).");
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

	fileReader.readFromDisk(preloadBuffer, 0, internalPreloadSize, sampleStart + monolithOffset, true);
}



size_t StreamingSamplerSound::getActualPreloadSize() const
{
	return hasActiveState() ? (size_t)(internalPreloadSize *preloadBuffer.getNumChannels()) * sizeof(float) + (size_t)(loopBuffer.getNumSamples() *loopBuffer.getNumChannels()) * sizeof(float) : 0;
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

			smallLoopBuffer.setSize(2, (int)loopLength);

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
			loopBuffer.setSize(2, (int)crossfadeLength);
			loopBuffer.clear();

			AudioSampleBuffer tempBuffer = AudioSampleBuffer(2, (int)crossfadeLength);

			// Calculate the fade in
			const int startCrossfade = loopStart - crossfadeLength;
			tempBuffer.clear();

			fileReader.openFileHandles();

			fileReader.readFromDisk(tempBuffer, 0, (int)crossfadeLength, startCrossfade + monolithOffset, false);
			
			tempBuffer.applyGainRamp(0, 0, (int)crossfadeLength, 0.0f, 1.0f);
			tempBuffer.applyGainRamp(1, 0, (int)crossfadeLength, 0.0f, 1.0f);

			FloatVectorOperations::copy(loopBuffer.getWritePointer(0, 0), tempBuffer.getReadPointer(0, 0), (int)crossfadeLength);
			FloatVectorOperations::copy(loopBuffer.getWritePointer(1, 0), tempBuffer.getReadPointer(1, 0), (int)crossfadeLength);

			// Calculate the fade out
			tempBuffer.clear();

			const int endCrossfade = loopEnd - crossfadeLength;

			fileReader.readFromDisk(tempBuffer, 0, (int)crossfadeLength, endCrossfade + monolithOffset, false);
			
			tempBuffer.applyGainRamp(0, 0, (int)crossfadeLength, 1.0f, 0.0f);
			tempBuffer.applyGainRamp(1, 0, (int)crossfadeLength, 1.0f, 0.0f);

			FloatVectorOperations::add(loopBuffer.getWritePointer(0, 0), tempBuffer.getReadPointer(0, 0), (int)crossfadeLength);
			FloatVectorOperations::add(loopBuffer.getWritePointer(1, 0), tempBuffer.getReadPointer(1, 0), (int)crossfadeLength);

			fileReader.closeFileHandles();
		}
	}
}

void StreamingSamplerSound::wakeSound() const
{
	fileReader.wakeSound();
}


bool StreamingSamplerSound::hasEnoughSamplesForBlock(int maxSampleIndexInFile) const noexcept
{
	return (loopEnabled && loopLength != 0) || maxSampleIndexInFile < sampleLength;
}

float StreamingSamplerSound::calculatePeakValue()
{
	return fileReader.calculatePeakValue();
}

void StreamingSamplerSound::fillSampleBuffer(AudioSampleBuffer &sampleBuffer, int samplesToCopy, int uptime) const
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

				FloatVectorOperations::copy(sampleBuffer.getWritePointer(0, 0), smallLoopBuffer.getReadPointer(0, startSample), numSamplesBeforeFirstWrap);
				FloatVectorOperations::copy(sampleBuffer.getWritePointer(1, 0), smallLoopBuffer.getReadPointer(1, startSample), numSamplesBeforeFirstWrap);
			}

			int numSamples = samplesToCopy - numSamplesBeforeFirstWrap;
			int indexInSampleBuffer = numSamplesBeforeFirstWrap;

			while (numSamples > (int)loopLength)
			{
				jassert(indexInSampleBuffer < sampleBuffer.getNumSamples());

				FloatVectorOperations::copy(sampleBuffer.getWritePointer(0, indexInSampleBuffer), smallLoopBuffer.getReadPointer(0, 0), loopLength);
				FloatVectorOperations::copy(sampleBuffer.getWritePointer(1, indexInSampleBuffer), smallLoopBuffer.getReadPointer(1, 0), loopLength);

				numSamples -= (int)loopLength;
				indexInSampleBuffer += (int)loopLength;
			}

			FloatVectorOperations::copy(sampleBuffer.getWritePointer(0, indexInSampleBuffer), smallLoopBuffer.getReadPointer(0, 0), numSamples);
			FloatVectorOperations::copy(sampleBuffer.getWritePointer(1, indexInSampleBuffer), smallLoopBuffer.getReadPointer(1, 0), numSamples);
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

void StreamingSamplerSound::fillInternal(AudioSampleBuffer &sampleBuffer, int samplesToCopy, int indexInFile, int offsetInBuffer) const
{
	jassert(indexInFile + samplesToCopy <= sampleEnd);

	// Some samples from the loop crossfade buffer are required
	if(loopEnabled && Range<int>(indexInFile, indexInFile + samplesToCopy).intersects(crossfadeArea))
	{
		const int numSamplesBeforeCrossfade = jmax(0, crossfadeArea.getStart() - indexInFile);

		if(numSamplesBeforeCrossfade > 0)
		{
			fillInternal(sampleBuffer, numSamplesBeforeCrossfade, indexInFile, 0);
		}
		
		const int numSamplesInCrossfade = jmin(samplesToCopy - numSamplesBeforeCrossfade, (int)crossfadeLength);

		if(numSamplesInCrossfade > 0)
		{
			ScopedLock sl(getSampleLock());

			const int indexInLoopBuffer = jmax(0, indexInFile - crossfadeArea.getStart());

			FloatVectorOperations::copy(sampleBuffer.getWritePointer(0, numSamplesBeforeCrossfade), loopBuffer.getReadPointer(0, indexInLoopBuffer), numSamplesInCrossfade);
			FloatVectorOperations::copy(sampleBuffer.getWritePointer(1, numSamplesBeforeCrossfade), loopBuffer.getReadPointer(1, indexInLoopBuffer), numSamplesInCrossfade);		
		}

		// Should be taken care by higher logic (fillSampleBuffer should wrap the loop)
		jassert((samplesToCopy - numSamplesBeforeCrossfade - numSamplesInCrossfade) == 0);
	}

	// All samples can be fetched from the preload buffer
	else if(indexInFile + samplesToCopy < internalPreloadSize)
	{
		// the preload buffer has already the samplestart offset
		const int indexInPreloadBuffer = indexInFile - (int)sampleStart;

		jassert(indexInPreloadBuffer >= 0);

		if (indexInPreloadBuffer + samplesToCopy < preloadBuffer.getNumSamples())
		{
			FloatVectorOperations::copy(sampleBuffer.getWritePointer(0, offsetInBuffer), preloadBuffer.getReadPointer(0, indexInPreloadBuffer), samplesToCopy);
			FloatVectorOperations::copy(sampleBuffer.getWritePointer(1, offsetInBuffer), preloadBuffer.getReadPointer(1, indexInPreloadBuffer), samplesToCopy);
		}
		else
		{
			jassertfalse;
			FloatVectorOperations::clear(sampleBuffer.getWritePointer(0), sampleBuffer.getNumSamples());
			FloatVectorOperations::clear(sampleBuffer.getWritePointer(1), sampleBuffer.getNumSamples());
		}
	}
	
	// Read all samples from disk
	else
	{
		fileReader.readFromDisk(sampleBuffer, offsetInBuffer, samplesToCopy, indexInFile + monolithOffset, true);
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

		fileFormatSupportsMemoryReading = fileExtension.contains("wav") || fileExtension.contains("aif");

		hashCode = loadedFile.hashCode64();
	}
	else
	{
		faultyFileName = fileName;
		loadedFile = File::nonexistent;
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
        faultyFileName = String::empty;
        
		const String fileExtension = loadedFile.getFileExtension();

		fileFormatSupportsMemoryReading = fileExtension.compareIgnoreCase(".wav") || fileExtension.startsWithIgnoreCase(".aif");

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


void StreamingSamplerSound::FileReader::readFromDisk(AudioSampleBuffer &buffer, int startSample, int numSamples, int readerPosition, bool useMemoryMappedReader)
{
	if (!fileHandlesOpen) openFileHandles(sendNotification);

    FloatVectorOperations::clear(buffer.getWritePointer(0, startSample), numSamples);
    FloatVectorOperations::clear(buffer.getWritePointer(1, startSample), numSamples);
    
	if (useMemoryMappedReader)
	{
		if (memoryReader != nullptr && memoryReader->getMappedSection().contains(Range<int64>(readerPosition, readerPosition + numSamples)))
		{
			ScopedReadLock sl(fileAccessLock);

			memoryReader->read(&buffer, startSample, numSamples, readerPosition, true, true);

			return;
		}
	}
	
	if (normalReader != nullptr)
	{
		ScopedReadLock sl(fileAccessLock);

		normalReader->read(&buffer, startSample, numSamples, readerPosition, true, true);
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
#if USE_FALLBACK_READERS_FOR_MONOLITH
     
        auto m = monolithicInfo->createFallbackReader(monolithicIndex, monolithicChannelIndex);
#else
		auto m =  monolithicInfo->createMonolithicReader(monolithicIndex, monolithicChannelIndex);
		//m->mapSectionOfFile(Range<int64>((int64)(sound->sampleStart) + (int64)(sound->monolithOffset), (int64)(sound->sampleEnd)));
#endif
        
        
		return m;
	}
	else
	{
		return nullptr;
	}
}

void StreamingSamplerSound::FileReader::setMonolithicInfo(HiseMonolithAudioFormat * info, int channelIndex, int sampleIndex)
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
readPointerLeft(nullptr),
readPointerRight(nullptr),
diskUsage(0.0),
lastCallToRequestData(0.0)
{
	unmapper.setLoader(this);

//     for(int i = 0; i < NUM_UNMAPPERS; i++)
//     {
//         unmappers[i].setLoader(this);
//     }
//     
	setBufferSize(BUFFER_SIZE_FOR_STREAM_BUFFERS);
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

	const AudioSampleBuffer *localReadBuffer = &s->getPreloadBuffer();
	AudioSampleBuffer *localWriteBuffer = &b1;

	// the read pointer will be pointing directly to the preload buffer of the sample sound
	readBuffer = localReadBuffer;
	writeBuffer = localWriteBuffer;

	readIndex = startTime;
	readIndexDouble = (double)startTime;

	readPointerLeft = localReadBuffer->getReadPointer(0, sampleStartModValue);
	readPointerRight = localReadBuffer->getReadPointer(1, sampleStartModValue);

	isReadingFromPreloadBuffer = true;

	// Set the sampleposition to (1 * bufferSize) because the first buffer is the preload buffer
	positionInSampleFile = (int)localReadBuffer->getNumSamples();
	
    voiceCounterWasIncreased = false;
    
	// The other buffer will be filled on the next free thread pool slot
	requestNewData();
};

StereoChannelData SampleLoader::fillVoiceBuffer(AudioSampleBuffer &voiceBuffer, double numSamples)
{
	const AudioSampleBuffer *localReadBuffer = readBuffer.get();
	AudioSampleBuffer *localWriteBuffer = writeBuffer.get();

	const int numSamplesInBuffer = localReadBuffer->getNumSamples();
	const int maxSampleIndexForFillOperation = (int)(readIndexDouble + numSamples)+ 1; // Round up the samples

	if (maxSampleIndexForFillOperation >= numSamplesInBuffer) // Check because of preloadbuffer style
	{
		const int indexBeforeWrap = jmax<int>(0, (int)readIndexDouble);
		const int numSamplesInFirstBuffer = localReadBuffer->getNumSamples() - indexBeforeWrap;

		jassert(numSamplesInFirstBuffer >= 0);

		if (numSamplesInFirstBuffer > 0)
		{
			voiceBuffer.copyFrom(0, 0, *localReadBuffer, 0, indexBeforeWrap, numSamplesInFirstBuffer);
			voiceBuffer.copyFrom(1, 0, *localReadBuffer, 1, indexBeforeWrap, numSamplesInFirstBuffer);
		}

		const int offset = numSamplesInFirstBuffer;
		//remaining = (int)(numSamples)+1 - offset;

		const int numSamplesAvailableInSecondBuffer = localWriteBuffer->getNumSamples() - offset;

		if ( (numSamplesAvailableInSecondBuffer > 0) && (numSamplesAvailableInSecondBuffer < localWriteBuffer->getNumSamples()))
		{
			const int numSamplesToCopyFromSecondBuffer = jmin<int>(numSamplesAvailableInSecondBuffer, voiceBuffer.getNumSamples() - offset);

			voiceBuffer.copyFrom(0, offset, *localWriteBuffer, 0, 0, numSamplesToCopyFromSecondBuffer);
			voiceBuffer.copyFrom(1, offset, *localWriteBuffer, 1, 0, numSamplesToCopyFromSecondBuffer);
		}
		else
		{
			// The streaming buffers must be greater than the block size!
			jassertfalse;
			FloatVectorOperations::clear(voiceBuffer.getWritePointer(0), voiceBuffer.getNumSamples());
			FloatVectorOperations::clear(voiceBuffer.getWritePointer(1), voiceBuffer.getNumSamples());
		}
		
		StereoChannelData returnData;

		returnData.leftChannel = voiceBuffer.getReadPointer(0);
		returnData.rightChannel = voiceBuffer.getReadPointer(1);

		return returnData;
	}
	else
	{
		const int index = (int)readIndexDouble;

		StereoChannelData returnData;

		returnData.leftChannel = localReadBuffer->getReadPointer(0, index);
		returnData.rightChannel = localReadBuffer->getReadPointer(1, index);

		return returnData;
	}
}

bool SampleLoader::advanceReadIndex(double delta)
{
	const int numSamplesInBuffer = readBuffer.get()->getNumSamples();
	readIndexDouble += delta;

	if (readIndexDouble >= numSamplesInBuffer)
	{
		positionInSampleFile += getNumSamplesForStreamingBuffers();
		readIndexDouble -= (double)numSamplesInBuffer;

		swapBuffers();
		const bool queueIsFree = requestNewData();
        
        return queueIsFree;
	}
    
    return true;
}

bool SampleLoader::requestNewData()
{
    ADD_GLITCH_DETECTOR("Requesting new sample data");

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
	}
};
	
void SampleLoader::refreshBufferSizes()
{
	const int numSamplesToUse = jmax<int>(idealBufferSize, minimumBufferSizeForSamplesPerBlock);

	if (getNumSamplesForStreamingBuffers() != numSamplesToUse)
	{
		b1 = AudioSampleBuffer(2, numSamplesToUse);
		b2 = AudioSampleBuffer(2, numSamplesToUse);

		b1.clear();
		b2.clear();

		readBuffer = &b1;
		writeBuffer = &b2;

		reset();
	}
}

bool SampleLoader::swapBuffers()
{
	const AudioSampleBuffer *localReadBuffer = readBuffer.get();

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

	readPointerLeft = readBuffer.get()->getReadPointer(0, 0);
	readPointerRight = readBuffer.get()->getReadPointer(1, 0);

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

	if(sound->getSampleLength() > 0)
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


void StreamingSamplerVoice::renderNextBlock(AudioSampleBuffer &outputBuffer, int startSample, int numSamples)
{
    
    
	const StreamingSamplerSound *sound = loader.getLoadedSound();

	if(sound != nullptr)
	{
        ADD_GLITCH_DETECTOR("Rendering sample " + sound->getFileName());
        
		const double startAlpha = fmod(voiceUptime, 1.0);
		
		double pitchCounter;

		if (pitchData == nullptr) pitchCounter = uptimeDelta * (double)numSamples;
		else
		{
			pitchCounter = 0.0;
			pitchData += startSample;

			for (int i = 0; i < numSamples; i++)
			{
				pitchCounter += uptimeDelta * (double)*pitchData++;
			}

			pitchData -= numSamples;
		}

		AudioSampleBuffer* tempVoiceBuffer = getTemporaryVoiceBuffer();

		jassert(tempVoiceBuffer != nullptr);

		tempVoiceBuffer->clear();
        
		// Copy the not resampled values into the voice buffer.
		StereoChannelData data = loader.fillVoiceBuffer(*tempVoiceBuffer, pitchCounter + startAlpha);

		const float* const inL = data.leftChannel;
		const float* const inR = data.rightChannel;
		float* outL = outputBuffer.getWritePointer(0, startSample);
		float* outR = outputBuffer.getWritePointer(1, startSample);
		
		const int cs = startSample;
		const int cns = numSamples;

		double indexInBuffer = startAlpha;

		if (pitchData != nullptr)
		{
			float indexInBufferFloat = (float)indexInBuffer;
			const float uptimeDeltaFloat = (float)uptimeDelta;

			while (numSamples > 0)
			{
				for (int i = 0; i < 4; i++)
				{
					const int pos = int(indexInBufferFloat);
					const float alpha = indexInBufferFloat - (float)pos;
					const float invAlpha = 1.0f - alpha;

					float l = (inL[pos] * invAlpha + inL[pos + 1] * alpha);
					float r = (inR[pos] * invAlpha + inR[pos + 1] * alpha);

					*outL++ = l;
					*outR++ = r;

					jassert(r >= -1.0f);

					indexInBufferFloat += (uptimeDeltaFloat * *pitchData++);
				}
				numSamples -= 4;
			}
		}
		else
		{
			float indexInBufferFloat = (float)indexInBuffer;
			const float uptimeDeltaFloat = (float)uptimeDelta;

			while (numSamples > 0)
			{
				for (int i = 0; i < 4; i++)
				{
					const int pos = int(indexInBufferFloat);
					const float alpha = indexInBufferFloat - (float)pos;
					const float invAlpha = 1.0f - alpha;

					float l = (inL[pos] * invAlpha + inL[pos + 1] * alpha);
					float r = (inR[pos] * invAlpha + inR[pos + 1] * alpha);

					*outL++ = l;
					*outR++ = r;

					indexInBufferFloat += uptimeDeltaFloat;
				}
				
				numSamples -= 4;
			}
		}
		
		voiceUptime += pitchCounter;
		
        if(!loader.advanceReadIndex(pitchCounter))
        {
            Logger::writeToLog("Streaming failure with sound " + sound->getFileName());
            resetVoice();
            return;
        }

		const bool enoughSamples = sound->hasEnoughSamplesForBlock((int)(voiceUptime + numSamples * MAX_SAMPLER_PITCH));

		if(!enoughSamples) resetVoice();




	}
    else
    {
        resetVoice();
    }
};