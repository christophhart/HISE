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

namespace hise {

// ==================================================================================================================================================

/** A SamplerSound which provides buffered disk streaming using memory mapped file access and a preloaded sample start. 
	@ingroup sampler

	This class is not directly used in HISE, but wrapped into a ModulatorSamplerSound which provides extra functionality.
	However, if you roll your own sampler class and just need to reuse the streaming facilities, you can tinker around with this class.
*/
class StreamingSamplerSound : public juce::SynthesiserSound
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
		LoadingError(const juce::String &fileName_, const juce::String &errorDescription_) :
			fileName(fileName_),
			errorDescription(errorDescription_)
		{};

        juce::String fileName;
        juce::String errorDescription;
	};

	// ==============================================================================================================================================

	/** Creates a new StreamingSamplerSound.
	*
	*	@param fileToLoad a stereo wave file that is read as memory mapped file.
	*	@param midiNotes the note map
	*	@param midiNoteForNormalPitch the root note
	*/
	StreamingSamplerSound(const juce::String &fileNameToLoad, StreamingSamplerSoundPool *pool);

	/** Creates a new StreamingSamplerSound from a monolithic file. */
	StreamingSamplerSound(MonolithInfoToUse *info, int channelIndex, int sampleIndex);

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
	int getLoopStart() const noexcept { return loopStart; };

	/** Returns the loop end. */
	int getLoopEnd() const noexcept { return loopEnd; };

	/** Returns the loop length. */
	int getLoopLength() const noexcept { return loopEnd - loopStart; };

	/** This sets the crossfade length. */
	void setLoopCrossfade(int newCrossfadeLength);;

	/** Returns the length of the crossfade. */
	int getLoopCrossfade() const noexcept { return crossfadeLength; };

	/** Loads the entire sample into the preload buffer and reverses it. */
	void setReversed(bool shouldBeReversed);

	/** Sets the basic MIDI mapping data (key-range, velocity-range and root note) from the given data object. */
	void setBasicMappingData(const StreamingHelpers::BasicMappingData& data);

	bool isEntireSampleLoaded() const noexcept { return entireSampleLoaded; };

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

	bool replaceAudioFile(const juce::AudioSampleBuffer& b);

	bool isMonolithic() const;
    juce::AudioFormatReader* createReaderForPreview();

    juce::AudioFormatReader* createReaderForAnalysis();

	int64_t getMonolithOffset() const { return fileReader.getMonolithOffset(); }
	int64_t getMonolithLength() const { return fileReader.getMonolithLength(); }
	double getMonolithSampleRate() const { return fileReader.getMonolithSampleRate(); }

	// ==============================================================================================================================================

    juce::String getFileName(bool getFullPath = false) const;

	int64_t getHashCode();


	void refreshFileInformation();
	void checkFileReference();
	void replaceFileReference(const juce::String &newFileName);

	bool isMissing() const noexcept;
	bool hasActiveState() const noexcept;

    juce::String getSampleStateAsString() const;;

	// ==============================================================================================================================================

	/** Returns the sampleRate of the sample (not the current playback samplerate). */
	double getSampleRate() const noexcept 
	{ 
		// Must be initialised!
		jassert(sampleRate != -1.0);
		return sampleRate; 
	};

	/** Returns the length of the loaded audio file in samples. */
	int64_t getLengthInSamples() const noexcept { return fileReader.getSampleLength(); };

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

	typedef juce::ReferenceCountedObjectPtr<StreamingSamplerSound> Ptr;

	const juce::CriticalSection& getSampleLock() const { return lock; };

    /** Use this in order to skip the preloading before all properties have been set. */
    void setDelayPreloadInitialisation(bool shouldDelay);
    
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

		void setFile(const juce::String &fileName);
		void setMonolithicInfo(MonolithInfoToUse* info, int channelIndex, int sampleIndex);

        juce::String getFileName(bool getFullPath);
		void checkFileReference();
		int64_t getHashCode() { return hashCode; };

		/** Refreshes the information about the file (if it is missing, if it supports memory-mapping). */
		void refreshFileInformation();

		// ==============================================================================================================================================

		/** Returns the best reader for the file. If a memorymapped reader can be used, it will return a MemoryMappedAudioFormatReader. */
        juce::AudioFormatReader *getReader();

		/** Encapsulates all reading operations. It will use the best available reader type and opens the file handle if it is not open yet. */
		void readFromDisk(hlac::HiseSampleBuffer &buffer, int startSample, int numSamples, int readerPosition, bool useMemoryMappedReader);

		/** Call this method if you want to close the file handle. If voices are playing, it won't close it. */
		void closeFileHandles(juce::NotificationType notifyPool = juce::sendNotification);

		/** Call this method if you want to open the file handles. If you just want to read the file, you don't need to call it. */
		void openFileHandles(juce::NotificationType notifyPool = juce::sendNotification);

		// ==============================================================================================================================================

		void increaseVoiceCount() { ++voiceCount; };
        void decreaseVoiceCount() { --voiceCount; }

		// ==============================================================================================================================================

		bool isUsed() const noexcept { return voiceCount.load() != 0; }
		bool isOpened() const noexcept { return fileHandlesOpen; }
		bool isMonolithic() const noexcept { return monolithicInfo != nullptr; }

		bool isStereo() const noexcept;

		bool isMissing() const { return missing; }
		void setMissing() { missing = true; }

		int64_t getMonolithOffset() const
		{
			if (monolithicInfo != nullptr)
				return monolithicInfo->getMonolithOffset(monolithicIndex);

			return 0;
		}

		int64_t getMonolithLength() const
		{
			if (monolithicInfo != nullptr)
				return monolithicInfo->getMonolithLength(monolithicIndex);

			return 0;
		}

		int64_t getSampleLength() const
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

        juce::AudioFormatReader* createMonolithicReaderForPreview();

        juce::AudioFormatWriter* createWriterWithSameFormat(juce::OutputStream* out);

		// ==============================================================================================================================================

	private:

		StreamingSamplerSoundPool *pool;

        juce::ReferenceCountedObjectPtr<MonolithInfoToUse> monolithicInfo = nullptr;
		int monolithicIndex = -1;
		int monolithicChannelIndex = -1;
        juce::String monolithicName;

        juce::CriticalSection readLock2;

        juce::ReadWriteLock fileAccessLock;

		bool stereo = true;

		bool isReading;

		int64_t sampleLength;

        juce::File loadedFile;

        juce::String faultyFileName;

		int64_t hashCode;

		StreamingSamplerSound *sound;

		std::unique_ptr<juce::MemoryMappedAudioFormatReader> memoryReader;
        std::unique_ptr<juce::AudioFormatReader> normalReader;
		bool fileHandlesOpen;

        std::atomic<int> voiceCount {0};

        bool fileFormatSupportsMemoryReading = true;

		bool missing;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FileReader)

	};

	struct ScopedFileHandler
	{
		ScopedFileHandler(StreamingSamplerSound* s, juce::NotificationType notifyPool_ = juce::NotificationType::sendNotification) :
            notifyPool(notifyPool_),
            reader(s->fileReader)
		{
			reader.openFileHandles(notifyPool);
		}

		~ScopedFileHandler()
		{
			reader.closeFileHandles(notifyPool);
		}

        juce::NotificationType notifyPool;
		FileReader& reader;
	};

	// ==============================================================================================================================================

	void loopChanged();
	void lengthChanged();

    void rebuildCrossfadeBuffer(bool preloadContainsLoop);
	void applyCrossfadeToPreloadBuffer();

	/** This fills the supplied AudioSampleBuffer with samples.
	*
	*	It copies the samples either from the preload buffer or reads it directly from the file, so don't call this method from the
	*	audio thread, but use the SampleLoader class which handles the background thread stuff.
	*/
	void fillSampleBuffer(hlac::HiseSampleBuffer &sampleBuffer, int samplesToCopy, int uptime) const;

	// used to wrap the read process for looping
	void fillInternal(hlac::HiseSampleBuffer &sampleBuffer, int samplesToCopy, int uptime, int offsetInBuffer = 0) const;


	// ==============================================================================================================================================


    juce::CriticalSection lock;

	mutable FileReader fileReader;

	bool purged;

    bool delayPreloadInitialisation = false;
    
	friend class SampleLoader;

	hlac::HiseSampleBuffer preloadBuffer;
	double sampleRate;

	int monolithOffset;
	int monolithLength;

	bool reversed = false;

	bool useSmallLoopBuffer = false;

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
	int loopLength;
	int crossfadeLength;

    juce::Range<int> crossfadeArea;

	int8_t rootNote = 0;
	
    juce::BigInteger midiNotes = 0;
    juce::BigInteger velocityRange = 0;
	
	// contains the precalculated crossfade
	hlac::HiseSampleBuffer loopBuffer;

	hlac::HiseSampleBuffer smallLoopBuffer;

	// ==============================================================================================================================================

    JUCE_DECLARE_WEAK_REFERENCEABLE (StreamingSamplerSound)
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StreamingSamplerSound)
};

} // namespace hise
#endif  // STREAMINGSAMPLERSOUND_H_INCLUDED
