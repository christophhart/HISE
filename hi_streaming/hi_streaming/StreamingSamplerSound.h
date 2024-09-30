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


#ifndef STREAMINGSAMPLERSOUND_H_INCLUDED
#define STREAMINGSAMPLERSOUND_H_INCLUDED

namespace hise { using namespace juce;

// ==================================================================================================================================================

/** A SamplerSound which provides buffered disk streaming using memory mapped file access and a preloaded sample start. 
	@ingroup sampler

	This class is not directly used in HISE, but wrapped into a ModulatorSamplerSound which provides extra functionality.
	However, if you roll your own sampler class and just need to reuse the streaming facilities, you can tinker around with this class.
*/
class StreamingSamplerSound : public SynthesiserSound
{
public:

	enum SampleStates
	{
		Normal = 0, ///< the default state for existing samples
		Disabled, ///< if samples are not found on the disk, they are disabled
		Purged, ///< you can purge samples (unload them from memory)
		numSampleStates
	};

	/** An object of this class will be thrown if the loading of the sound fails.
	*/
	struct LoadingError
	{
		/** Create one of this, if the sound fails to load.
		*
		*	@param fileName the file that caused the error.
		*	@param errorDescription a description of what went wrong.
		*/
		LoadingError(const String &fileName_, const String &errorDescription_) :
			fileName(fileName_),
			errorDescription(errorDescription_)
		{};

		String fileName;
		String errorDescription;
	};


	enum class ReleasePlayState
	{
		Inactive,
#if HISE_SAMPLER_ALLOW_RELEASE_START
		Playing,
#endif
		numReleasePlayStates
	};


	// ==============================================================================================================================================

	/** Creates a new StreamingSamplerSound.
	*
	*	@param fileToLoad a stereo wave file that is read as memory mapped file.
	*	@param midiNotes the note map
	*	@param midiNoteForNormalPitch the root note
	*/
	StreamingSamplerSound(const String &fileNameToLoad, StreamingSamplerSoundPool *pool);

	/** Creates a new StreamingSamplerSound from a monolithic file. */
	StreamingSamplerSound(HlacMonolithInfo::Ptr info, int channelIndex, int sampleIndex);

	~StreamingSamplerSound();

	// ===============================================================================================================================================

	/** Checks if the note is mapped to the supplied note number. */
	bool appliesToNote(int midiNoteNumber) noexcept override { return midiNotes[midiNoteNumber];};
	bool appliesToVelocity(int velocity) noexcept { return velocityRange[velocity]; }

	/** Always returns true ( can be implemented if used, but I don't need it) */
	bool appliesToChannel(int /*midiChannel*/) noexcept override { return true; };

	/** Returns the pitch factor for the note number. */
	static double getPitchFactor(int noteNumberToPitch, int rootNoteForPitchFactor) noexcept;;

	int getRootNote() const { return (int)rootNote; }

	// ===============================================================================================================================================

	/** Sets the sample start. This reloads the PreloadBuffer, so make sure you don't call it in the audio thread.
	*
	*	If the sample start is the same, it will do nothing.
	*/
	void setSampleStart(int sampleStart);

	/** Sets the sample end. This can be used to truncate a sample. */
	void setSampleEnd(int sampleEnd);

	/** Returns the sample start index. */
	int getSampleStart() const noexcept { return sampleStart; };

	/** Returns the sample end index. */
	int getSampleEnd() const noexcept { return sampleEnd; };

	/** Returns the length of the sample. */
	int getSampleLength() const noexcept { return sampleLength; };

	/** Sets the sample start modulation. The preload buffer will be enhanced by this amount in order to ensure immediate playback. */
	void setSampleStartModulation(int maxSampleStartDelta);

	/** Returns the amount of samples that can be skipped at voice start.
	*
	*   Changing this value means that the preload buffer will be increased to this amount of samples + the preload size.
	*/
	int getSampleStartModulation() const noexcept { return sampleStartMod; };

	// ==============================================================================================================================================

	/** Enables the loop. If the loop points are beyond the loaded sample area, they will be truncated. */
	void setLoopEnabled(bool shouldBeEnabled);;

	/** Checks if the looping is enabled. */
	bool isLoopEnabled() const noexcept { return loopEnabled; };

	/** Sets the start of the loop section.
	*
	*   If crossfading is enabled, it will reload the crossfade area. */
	void setLoopStart(int newLoopStart);;

	/** Sets the end point of the looping section.
	*
	*   If crossfading is enabled, it will reload the crossfade area. */
	void setLoopEnd(int newLoopEnd);;

	/** Returns the loop start. */
	int getLoopStart(bool getReverseLoopPoint=false) const noexcept;;

	/** Returns the loop end. */
	int getLoopEnd(bool getReverseLoopPoint=false) const noexcept;;

	/** Returns the loop length. */
	int getLoopLength() const noexcept { return loopEnd - loopStart; };

	/** This sets the crossfade length. */
	void setLoopCrossfade(int newCrossfadeLength);;

	/** Returns the length of the crossfade (might not be the actual crossfade length if the sample is reversed). */
	int getLoopCrossfade() const noexcept { return crossfadeLength; };

	/** Loads the entire sample into the preload buffer and reverses it. */
	void setReversed(bool shouldBeReversed);

	/** Checks if the sound is reversed. */
	bool isReversed() const { return fileReader.isReversed(); }

	/** Sets the basic MIDI mapping data (key-range, velocity-range and root note) from the given data object. */
	void setBasicMappingData(const StreamingHelpers::BasicMappingData& data);

	bool isEntireSampleLoaded() const noexcept { return entireSampleLoaded; };
	
	

#if HISE_SAMPLER_ALLOW_RELEASE_START

	bool isReleaseStartEnabled() const noexcept { return releaseStart != 0; }

	/** Sets the point where the sample should seek to if the note is released (0 = disabled). */
	void setReleaseStart(int newReleaseStart);

	int getReleaseStart() const { return releaseStart; }

	StreamingHelpers::ReleaseStartOptions::Ptr getReleaseStartOptions() const noexcept { return releaseStartOptions; }

	void setReleaseStartOptions(StreamingHelpers::ReleaseStartOptions::Ptr newOptions)
	{
		if(releaseStartOptions != newOptions)
		{
			releaseStartOptions = newOptions;
			rebuildReleaseStartBuffer();
		}
	}

	const hlac::HiseSampleBuffer *getReleaseStartBuffer() const { return releaseStartData != nullptr ? &releaseStartData->preloadBuffer : nullptr; }

	float getReleaseAttenuation() const
	{
		return releaseStartData != nullptr ? releaseStartData->getReleaseAttenuation() : 1.0f;
	}

	void resetReleaseData() const
	{
		if(releaseStartData != nullptr)
			releaseStartData->resetReleaseData();
	}

	void calculateReleasePeak(float* calculatedValues, int numSamples) const
	{
		if(releaseStartData != nullptr)
			releaseStartData->calculateReleasePeak(calculatedValues, numSamples, releaseStartOptions);
	}

#else

	static constexpr bool isReleaseStartEnabled() { return false; }

#endif
	
	// ==============================================================================================================================================

	/** Set the preload size.
	*
	*	If the preload size is not changed, it will do nothing, but you can force it to reload it with 'forceReload'.
	*	You can also tell the sound to load everything into memory by calling loadEntireSample().
	*/
	void setPreloadSize(int newPreloadSizeInSamples, bool forceReload = false);

	/** Returns the size of the preload buffer in bytes. You can use this method to check how much memory the sound uses. It also includes the memory used for the crossfade buffer. */
	size_t getActualPreloadSize() const;

	/** Tell the sound to load everything into memory.
	*
	*   It will also close the file handle.
	*/
	void loadEntireSample();

	/** increases the voice counter. */
	void increaseVoiceCount() const;

	/** decreases the voice counter. The file handle will be kept open until no voice is played. */
	void decreaseVoiceCount() const;;

	void closeFileHandle();
	void openFileHandle();
	bool isOpened();

	bool isStereo() const;

	int getBitRate() const;

	bool replaceAudioFile(const AudioSampleBuffer& b);

	bool isMonolithic() const;
	AudioFormatReader* createReaderForPreview();

	AudioFormatReader* createReaderForAnalysis();

	int64 getMonolithOffset() const { return fileReader.getMonolithOffset(); }
	int64 getMonolithLength() const { return fileReader.getMonolithLength(); }
	double getMonolithSampleRate() const { return fileReader.getMonolithSampleRate(); }

	// ==============================================================================================================================================

	String getFileName(bool getFullPath = false) const;

	int64 getHashCode();


	void refreshFileInformation();
	void checkFileReference();
	void replaceFileReference(const String &newFileName);

	bool isMissing() const noexcept;
	bool hasActiveState() const noexcept;

	String getSampleStateAsString() const;;

	// ==============================================================================================================================================

	/** Returns the sampleRate of the sample (not the current playback samplerate). */
	double getSampleRate() noexcept;;

	/** Returns the length of the loaded audio file in samples. */
	int64 getLengthInSamples() const noexcept { return fileReader.getSampleLength(); };

	/** Gets the sound into active memory.
	*
	*	This is a wrapper around MemoryMappedAudioFormatReader::touchSample(), and I didn't check if it is necessary.
	*/
	void wakeSound() const;

	/** Checks if the file is mapped and has enough samples.
	*
	*	Call this before you call fillSampleBuffer() to check if the audio file has enough samples.
	*/
	bool hasEnoughSamplesForBlock(int maxSampleIndexInFile) const;

	/** Returns read only access to the preload buffer.
	*
	*	This is used by the SampleLoader class to fetch the samples from the preloaded buffer until the disk streaming
	*	thread fills the other buffer.
	*/
	const hlac::HiseSampleBuffer &getPreloadBuffer() const
	{
		// This should not happen (either its unloaded or it has some samples)...
		//jassert(preloadBuffer.getNumSamples() != 0);

		return preloadBuffer;
	}

	// ==============================================================================================================================================

	/** Scans the file for the max level. */
	float calculatePeakValue();

	void setPurged(bool shouldBePurged) { purged = shouldBePurged; };
	bool isPurged() const noexcept { return purged; }

	// ==============================================================================================================================================

	typedef ReferenceCountedObjectPtr<StreamingSamplerSound> Ptr;

	const CriticalSection& getSampleLock() const { return lock; };

    /** Use this in order to skip the preloading before all properties have been set. */
    void setDelayPreloadInitialisation(bool shouldDelay);
    
	void setCrossfadeGammaValue(float newGammaValue);

private:

	

	// ==============================================================================================================================================

	/** Encapsulates all reading operations. */
	class FileReader
	{
	public:

		// ==========================================================================================================================================

		FileReader(StreamingSamplerSound *soundForReader, StreamingSamplerSoundPool *pool);
		~FileReader();

		// ==============================================================================================================================================

		void setFile(const String &fileName);
		void setMonolithicInfo(HlacMonolithInfo::Ptr info, int channelIndex, int sampleIndex);

		String getFileName(bool getFullPath);
		void checkFileReference();
		int64 getHashCode() { return hashCode; };

		/** Refreshes the information about the file (if it is missing, if it supports memory-mapping). */
		void refreshFileInformation();

		// ==============================================================================================================================================

		/** Returns the best reader for the file. If a memorymapped reader can be used, it will return a MemoryMappedAudioFormatReader. */
		AudioFormatReader *getReader();

		/** Encapsulates all reading operations. It will use the best available reader type and opens the file handle if it is not open yet. */
		void readFromDisk(hlac::HiseSampleBuffer &buffer, int startSample, int numSamples, int readerPosition, bool useMemoryMappedReader);

		/** Call this method if you want to close the file handle. If voices are playing, it won't close it. */
		void closeFileHandles(NotificationType notifyPool = sendNotification);

		/** Call this method if you want to open the file handles. If you just want to read the file, you don't need to call it. */
		void openFileHandles(NotificationType notifyPool = sendNotification);

		// ==============================================================================================================================================

		void increaseVoiceCount() { ++voiceCount; };
		void decreaseVoiceCount() { --voiceCount; voiceCount.compareAndSetBool(0, -1); }

		// ==============================================================================================================================================

		bool isUsed() const noexcept { return voiceCount.get() != 0; }
		bool isOpened() const noexcept { return fileHandlesOpen; }
		bool isMonolithic() const noexcept { return monolithicInfo != nullptr; }

		bool isStereo() const noexcept;

		bool isMissing() const { return missing; }
		void setMissing() { missing = true; }

		int64 getMonolithOffset() const
		{
			if (monolithicInfo != nullptr)
				return monolithicInfo->getMonolithOffset(monolithicIndex);

			return 0;
		}

		int64 getMonolithLength() const
		{
			if (monolithicInfo != nullptr)
				return monolithicInfo->getMonolithLength(monolithicIndex);

			return 0;
		}

		int64 getSampleLength() const
		{
			return sampleLength;
		}

		double getMonolithSampleRate() const
		{
			if (monolithicInfo != nullptr)
			{
				return monolithicInfo->getMonolithSampleRate(monolithicIndex);
			}

			return 0.0;
		}

		// ==============================================================================================================================================

		void wakeSound();

		float calculatePeakValue();

		AudioFormatReader* createMonolithicReaderForPreview();

		AudioFormatWriter* createWriterWithSameFormat(OutputStream* out);

		void setReversed(bool shouldBeReversed)
		{
			reversed = shouldBeReversed;
		}

		bool isReversed() const { return reversed; }

		// ==============================================================================================================================================

	private:

		bool reversed = false;

		StreamingSamplerSoundPool *pool;

		HlacMonolithInfo::Ptr monolithicInfo;
		int monolithicIndex = -1;
		int monolithicChannelIndex = -1;
		String monolithicName;

		ReadWriteLock fileAccessLock;

		bool stereo = true;

		bool isReading;

		int64 sampleLength;

		File loadedFile;

		String faultyFileName;

		int64 hashCode;

		StreamingSamplerSound *sound;

		ScopedPointer<MemoryMappedAudioFormatReader> memoryReader;
		ScopedPointer<AudioFormatReader> normalReader;
		bool fileHandlesOpen;

		Atomic<int> voiceCount;

        bool fileFormatSupportsMemoryReading = true;

		bool missing = false;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FileReader)

	};

	struct ScopedFileHandler
	{
		ScopedFileHandler(StreamingSamplerSound* s, NotificationType notifyPool_ = NotificationType::sendNotification) :
			reader(s->fileReader),
			notifyPool(notifyPool_)
		{
			reader.openFileHandles(notifyPool);
		}

		~ScopedFileHandler()
		{
			reader.closeFileHandles(notifyPool);
		}

		NotificationType notifyPool;
		FileReader& reader;
	};

	// ==============================================================================================================================================

	void loopChanged();
	void lengthChanged();

	void calculateCrossfadeArea();
    void rebuildCrossfadeBuffer();
	void applyCrossfadeToInternalBuffers();

	/** This fills the supplied AudioSampleBuffer with samples.
	*
	*	It copies the samples either from the preload buffer or reads it directly from the file, so don't call this method from the
	*	audio thread, but use the SampleLoader class which handles the background thread stuff.
	*/
	void fillSampleBuffer(hlac::HiseSampleBuffer &sampleBuffer, int samplesToCopy, int uptime, ReleasePlayState releaseState) const;

	// used to wrap the read process for looping
	void fillInternal(hlac::HiseSampleBuffer &sampleBuffer, int samplesToCopy, int uptime, ReleasePlayState releaseState, int offsetInBuffer = 0) const;

	// ==============================================================================================================================================

	CriticalSection lock;

	mutable FileReader fileReader;

	bool purged;

    bool delayPreloadInitialisation = false;
    
	friend class SampleLoader;

	hlac::HiseSampleBuffer preloadBuffer;
	double sampleRate;

	int preloadSize;
	int internalPreloadSize;

	bool entireSampleLoaded;

	int sampleStart;
	int sampleEnd;
	int sampleLength;
	int sampleStartMod;

	bool loopEnabled;
	int loopStart;
	int loopEnd;
	int crossfadeLength;

	float crossfadeGamma = 1.0f;

	Range<int> crossfadeArea;

	int8 rootNote = 0;
	
	BigInteger midiNotes = 0;
	BigInteger velocityRange = 0;
	
	// contains the precalculated crossfade
	ScopedPointer<hlac::HiseSampleBuffer> loopBuffer;
	ScopedPointer<hlac::HiseSampleBuffer> smallLoopBuffer;

#if HISE_SAMPLER_ALLOW_RELEASE_START

	int releaseStart = 0;

	struct ReleaseStartData
	{
		ReleaseStartData(bool isFloat, int numSamples):
		  preloadBuffer(isFloat, 2, numSamples)
		{
			preloadBuffer.clear();
		}

		float getReleaseAttenuation() const
		{
			if(releasePreloadPeak != 0.0f)
				return jlimit(0.0f, 4.0f, currentAttenuationPeak / releasePreloadPeak);

			return 1.0f;
		}

		void calculateReleasePeak(float* calculatedValues, int numSamples, StreamingHelpers::ReleaseStartOptions::Ptr options)
		{
			auto thisRange = FloatVectorOperations::findMinAndMax(calculatedValues, numSamples);
			auto thisMax = jmax(std::abs(thisRange.getStart()), std::abs(thisRange.getEnd()));


			if(thisMax > currentAttenuationPeak)
				currentAttenuationPeak = thisMax;
			else
			{
				float alpha = options->smoothing;
				currentAttenuationPeak = thisMax * alpha + currentAttenuationPeak * (1.0f - alpha);
			}
		}

		void resetReleaseData()
		{
			currentAttenuationPeak = 0.0f;
		}

		void calculateZeroCrossings()
		{
			float lastValue = 0.0f;
			int lastIndex = 0;

			for(int i = 0; i < preloadBuffer.getNumSamples(); i++)
			{
				float value;

				if(preloadBuffer.isFloatingPoint())
					value = *((float*)preloadBuffer.getReadPointer(0, i));
				else
					value = static_cast<float>(*((int16*)preloadBuffer.getReadPointer(0, i))) / (float)INT_MAX;

				if(lastValue < 0.0f && value > 0.0f)
				{
					auto delta = i - lastIndex;

					float cyclePeak = 0.0f;

					if(delta > 0)
					{
						cyclePeak = preloadBuffer.getMagnitude(lastIndex, delta);
					}

					zeroCrossingIndexes.push_back({i, cyclePeak});
					lastIndex = i;
				}

				lastValue = value;
			}
		}

		std::vector<std::pair<int, float>> zeroCrossingIndexes;
		hlac::HiseSampleBuffer preloadBuffer;
		float releasePreloadPeak = 0.0f;
		float currentAttenuationPeak = 0.0f;
	};

	void rebuildReleaseStartBuffer();
	
	StreamingHelpers::ReleaseStartOptions::Ptr releaseStartOptions;
	ScopedPointer<ReleaseStartData> releaseStartData;
#endif

	// ==============================================================================================================================================

	JUCE_DECLARE_WEAK_REFERENCEABLE(StreamingSamplerSound);
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StreamingSamplerSound)
};

} // namespace hise
#endif  // STREAMINGSAMPLERSOUND_H_INCLUDED
