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

#ifndef STREAMINGSAMPLER_H_INCLUDED
#define STREAMINGSAMPLER_H_INCLUDED


class ModulatorSampler;
class ModulatorSamplerSoundPool;

// ==================================================================================================================================================

/** A simple data struct for a stereo channel.
*
*	This merges channels from an AudioSampleBuffer into one data structure which is filled by the sample rendering.
*/
struct StereoChannelData
{
	const float *leftChannel;
	const float *rightChannel;
};

// ==================================================================================================================================================

// This class is a spin off of my upcoming sampler framework, so in order to use it in another project, leave this at '1'
#define STANDALONE 0

#if STANDALONE
#include <JuceHeader.h>
#endif

// This is the maximum value for sample pitch manipulation (this means 3 octaves, which should be more than enough
#define MAX_SAMPLER_PITCH 4

// This is the default preload size. I defined a pretty random value here, but you can change this dynamically.
#define PRELOAD_SIZE 8192

// Same as the preload size.
#define BUFFER_SIZE_FOR_STREAM_BUFFERS 8192

// Deactivate this to use one rounded pitch value for one a buffer (crucial for other interpolation methods than linear interpolation)
#define USE_SAMPLE_ACCURATE_RESAMPLING 0

// You can set this to 0, if you want to disable background threaded reading. The files will then be read directly in the audio thread,
// which is not the smartest thing to do, but it comes to good use for debugging.
#define USE_BACKGROUND_THREAD 1

// By default, every voice adds its output to the supplied buffer. Depending on your architecture, it could be more practical to
// set (overwrite) the buffer. In this case, set this to 1.
#if STANDALONE
#define OVERWRITE_BUFFER_WITH_VOICE_DATA 0
#else
#define OVERWRITE_BUFFER_WITH_VOICE_DATA 1
#endif

// ==================================================================================================================================================

/** A SamplerSound which provides buffered disk streaming using memory mapped file access and a preloaded sample start. */
class StreamingSamplerSound: public SynthesiserSound
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
	struct LoadingError: public std::exception
	{
		/** Create one of this, if the sound fails to load.
		*
		*	@param fileName the file that caused the error.
		*	@param errorDescription a description of what went wrong.
		*/
		LoadingError(const String &fileName_, const String &errorDescription_):
			fileName(fileName_),
			errorDescription(errorDescription_)
		{};

		String fileName;
		String errorDescription;
	};

	// ==============================================================================================================================================

	/** Creates a new StreamingSamplerSound.
	*
	*	@param fileToLoad a stereo wave file that is read as memory mapped file.
	*	@param midiNotes the note map
	*	@param midiNoteForNormalPitch the root note
	*/
	StreamingSamplerSound(const String &fileNameToLoad, BigInteger midiNotes, int midiNoteForNormalPitch, ModulatorSamplerSoundPool *pool);

	/** Creates a new StreamingSamplerSound from a monolithic file. */
	StreamingSamplerSound(HiseMonolithAudioFormat *info, int channelIndex, int sampleIndex);


	

	~StreamingSamplerSound();

	// ===============================================================================================================================================

	/** Checks if the note is mapped to the supplied note number. */
	bool appliesToNote(int midiNoteNumber) noexcept override {jassertfalse; return false; };

	/** Always returns true ( can be implemented if used, but I don't need it) */
	bool appliesToChannel(int /*midiChannel*/) noexcept override {jassertfalse; return false;};

	/** Returns the pitch factor for the note number. */
	double getPitchFactor(int noteNumberToPitch, int rootNote) const noexcept{ return pow(2.0, (noteNumberToPitch - rootNote) / 12.0); };

	// ===============================================================================================================================================

	/** Sets the sample start. This reloads the PreloadBuffer, so make sure you don't call it in the audio thread.
	*
	*	If the sample start is the same, it will do nothing.
	*/
	void setSampleStart(int sampleStart);

	/** Sets the sample end. This can be used to truncate a sample. */
	void setSampleEnd(int sampleEnd);

    /** Returns the sample start index. */
	int getSampleStart() const noexcept {return sampleStart;};

    /** Returns the sample end index. */
	int getSampleEnd() const noexcept {return sampleEnd;};

    /** Returns the length of the sample. */
	int getSampleLength() const noexcept {return sampleLength;};

	/** Sets the sample start modulation. The preload buffer will be enhanced by this amount in order to ensure immediate playback. */
	void setSampleStartModulation(int maxSampleStartDelta);

	/** Returns the amount of samples that can be skipped at voice start.
	*
	*   Changing this value means that the preload buffer will be increased to this amount of samples + the preload size.
	*/
	int getSampleStartModulation() const noexcept{ return sampleStartMod; };

	// ==============================================================================================================================================

	/** Enables the loop. If the loop points are beyond the loaded sample area, they will be truncated. */
	void setLoopEnabled(bool shouldBeEnabled);;

	/** Checks if the looping is enabled. */
	bool isLoopEnabled() const noexcept{ return loopEnabled; };

    /** Sets the start of the loop section.
    *
    *   If crossfading is enabled, it will reload the crossfade area. */
	void setLoopStart(int newLoopStart);;

    /** Sets the end point of the looping section.
    *
    *   If crossfading is enabled, it will reload the crossfade area. */
	void setLoopEnd(int newLoopEnd);;

	/** Returns the loop start. */
	int getLoopStart() const noexcept{ return loopStart; };

	/** Returns the loop end. */
	int getLoopEnd() const noexcept{ return loopEnd; };

	/** Returns the loop length. */
	int getLoopLength() const noexcept{ return loopEnd - loopStart; };

	/** This sets the crossfade length. */
	void setLoopCrossfade(int newCrossfadeLength);;

	/** Returns the length of the crossfade. */
	int getLoopCrossfade() const noexcept{ return crossfadeLength; };

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

	bool isMonolithic() const;
	AudioFormatReader* createReaderForPreview() { return fileReader.createMonolithicReaderForPreview(); }

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
	double getSampleRate() const noexcept { return sampleRate; };

	/** Returns the length of the loaded audio file in samples. */
	int getLengthInSamples() const noexcept { return monolithLength; };

	/** Gets the sound into active memory.
	*
	*	This is a wrapper around MemoryMappedAudioFormatReader::touchSample(), and I didn't check if it is necessary. 
	*/
	void wakeSound() const;

	/** Checks if the file is mapped and has enough samples.
	*
	*	Call this before you call fillSampleBuffer() to check if the audio file has enough samples.
	*/
	bool hasEnoughSamplesForBlock(int maxSampleIndexInFile) const noexcept ;

	/** Returns read only access to the preload buffer.
	*
	*	This is used by the SampleLoader class to fetch the samples from the preloaded buffer until the disk streaming
	*	thread fills the other buffer.
	*/
	const AudioSampleBuffer &getPreloadBuffer() const
	{
		// This should not happen (either its unloaded or it has some samples)...
		jassert(preloadBuffer.getNumSamples() != 0);

		return preloadBuffer;
	}

	// ==============================================================================================================================================

	/** Scans the file for the max level. */
	float calculatePeakValue();

	void setPurged(bool shouldBePurged) { purged = shouldBePurged; };
	bool isPurged() const noexcept { return purged; }
	
	// ==============================================================================================================================================

	typedef ReferenceCountedObjectPtr<StreamingSamplerSound> Ptr;

private:

	// ==============================================================================================================================================

	/** Encapsulates all reading operations. */
	class FileReader
	{
	public:

		// ==========================================================================================================================================

		FileReader(StreamingSamplerSound *soundForReader, ModulatorSamplerSoundPool *pool);

		~FileReader();

		// ==============================================================================================================================================

		void setFile(const String &fileName);

		void setMonolithicInfo(HiseMonolithAudioFormat * info, int channelIndex, int sampleIndex);


		String getFileName(bool getFullPath);
		void checkFileReference();
		int64 getHashCode() { return hashCode; };

		/** Refreshes the information about the file (if it is missing, if it supports memory-mapping). */
		void refreshFileInformation();

		// ==============================================================================================================================================

		/** Returns the best reader for the file. If a memorymapped reader can be used, it will return a MemoryMappedAudioFormatReader. */
		AudioFormatReader *getReader();

		/** Encapsulates all reading operations. It will use the best available reader type and opens the file handle if it is not open yet. */
		void readFromDisk(AudioSampleBuffer &buffer, int startSample, int numSamples, int readerPosition, bool useMemoryMappedReader);

		/** Call this method if you want to close the file handle. If voices are playing, it won't close it. */
		void closeFileHandles(NotificationType notifyPool=sendNotification);

		/** Call this method if you want to open the file handles. If you just want to read the file, you don't need to call it. */
		void openFileHandles(NotificationType notifyPool=sendNotification);

		// ==============================================================================================================================================

		void increaseVoiceCount() { ++voiceCount; };
		void decreaseVoiceCount() { --voiceCount; voiceCount.compareAndSetBool(0, -1); }

		// ==============================================================================================================================================

		bool isUsed() const noexcept { return voiceCount.get() != 0; }
		bool isOpened() const noexcept { return fileHandlesOpen; }
		bool isMonolithic() const noexcept{ return monolithicInfo != nullptr; }

		bool isMissing() const { return missing; }
		void setMissing() { missing = true; }

		

		// ==============================================================================================================================================

		void wakeSound();

		float calculatePeakValue();
		
		AudioFormatReader* createMonolithicReaderForPreview();

		// ==============================================================================================================================================

	private:

		ModulatorSamplerSoundPool *pool;

		HiseMonolithAudioFormat* monolithicInfo = nullptr;
		int monolithicIndex = -1;
		int monolithicChannelIndex = -1;
		String monolithicName;

		CriticalSection readLock;

		bool isReading;

        
        
		File loadedFile;
        
        String faultyFileName;
        
		int64 hashCode;

		StreamingSamplerSound *sound;

		ScopedPointer<MemoryMappedAudioFormatReader> memoryReader;
		ScopedPointer<AudioFormatReader> normalReader;
		bool fileHandlesOpen;
		
        Atomic<int> voiceCount;

		bool fileFormatSupportsMemoryReading;

		bool missing;
		
		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FileReader)

	};

	// ==============================================================================================================================================

	void loopChanged();
	void lengthChanged();

	/** This fills the supplied AudioSampleBuffer with samples.
	*
	*	It copies the samples either from the preload buffer or reads it directly from the file, so don't call this method from the 
	*	audio thread, but use the SampleLoader class which handles the background thread stuff.
	*/
	void fillSampleBuffer(AudioSampleBuffer &sampleBuffer, int samplesToCopy, int uptime) const;

	// used to wrap the read process for looping
	void fillInternal(AudioSampleBuffer &sampleBuffer, int samplesToCopy, int uptime, int offsetInBuffer=0) const;


	// ==============================================================================================================================================

	CriticalSection lock;

	mutable FileReader fileReader;

	bool purged;
	
	friend class SampleLoader;

	AudioSampleBuffer preloadBuffer;	
	double sampleRate;

	int monolithOffset;
	int monolithLength;

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
        
	Range<int> crossfadeArea;


	// contains the precalculated crossfade
	AudioSampleBuffer loopBuffer;

	// ==============================================================================================================================================

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StreamingSamplerSound)
};

#if JUCE_32BIT
#define NUM_UNMAPPERS 8
#else
#define NUM_UNMAPPERS 8
#endif



/** This is a utility class that handles buffered sample streaming in a background thread.
*
*	It is derived from ThreadPoolJob, so whenever you want it to read new samples, add an instance of this 
*	to a ThreadPool (but don't delete it!) and it will do what it is supposed to do.
*/
class SampleLoader: public SampleThreadPoolJob
{
public:

	/** Creates a new SampleLoader.
	*
	*	Normally you don't need to call this manually, as a StreamingSamplerVoice automatically creates a instance as member.
	*/
	SampleLoader(SampleThreadPool *pool_);;

	/** Sets the buffer size in samples. */
	void setBufferSize(int newBufferSize);

	/** This checks the buffer size and increases it if it is too low. 
	*
	*	This is used to make sure that the buffer size is at least 3x bigger than the current block size to avoid switching the buffers within one processBlock.
	*
	*	@return 'true' if the buffer needed resizing (because it was either too big or to small).
	*/
	bool assertBufferSize(int minimumBufferSize);

	/** This fills the currently inactive buffer with samples from the SamplerSound.
	*
	*	The write buffer will be locked for the time of the read operation. Also it measures the time for getDiskUsage();
	*/
	JobStatus runJob() override;

	size_t getActualStreamingBufferSize() const
	{
		return b1.getNumSamples() * 2 * 2;
	}

	StereoChannelData fillVoiceBuffer(AudioSampleBuffer &voiceBuffer, double numSamples);

	void advanceReadIndex(double delta);

	/** Call this whenever a sound was started.
	*
	*	This will set the read pointer to the preload buffer of the StreamingSamplerSound and start the background reading.
	*/
	void startNote(StreamingSamplerSound const *s, int sampleStartModValue);

	/** Returns the loaded sound. */
	inline const StreamingSamplerSound *getLoadedSound() const { return sound.get();	};

	class Unmapper : public SampleThreadPoolJob
	{
	public:

		Unmapper() :
			SampleThreadPoolJob("Unmapper"),
			sound(nullptr),
            loader(nullptr)
		{};

        void setLoader(SampleLoader *loader_)
        {
            loader = loader_;
        }
        
		void setSoundToUnmap(const StreamingSamplerSound *s)
		{
			jassert(sound == nullptr);
			sound = const_cast<StreamingSamplerSound *>(s);
		}

		JobStatus runJob() override
		{
            jassert(sound != nullptr);

			if (loader->isRunning())
			{
				while(!waitForJobToFinish(loader, 50));
			}
            
			if (sound != nullptr)
			{
                sound->decreaseVoiceCount();
				sound->closeFileHandle();
				
				sound = nullptr;
			}

			return SampleThreadPoolJob::jobHasFinished;
		}

	private:

		StreamingSamplerSound *sound;
        SampleLoader *loader;
	};

	/** Resets the loader (unloads the sound). */
	void reset()
	{
		const StreamingSamplerSound *currentSound = sound.get();

		if (currentSound != nullptr)
		{
			unmapper.setSoundToUnmap(currentSound);

			backgroundPool->addJob(&unmapper, false);
            
            clearLoader();
		}
	}
    
    void clearLoader()
    {   
        sound = nullptr;
        diskUsage = 0.0f;
        readPointerLeft = nullptr;
        readPointerRight = nullptr;
    }

	/** Calculates and returns the disk usage.
	*
	*	It measures the time the background thread needed for the loading operation and divides it with the duration since the last
	*	call to requestNewData().
	*/
	double getDiskUsage() noexcept
	{
		const double returnValue = (double)diskUsage.get();
		diskUsage = 0.0f;
		return returnValue;
	};

	Unmapper *getFreeUnmapper()
	{
		for (int i = 0; i < NUM_UNMAPPERS; i++)
		{
			if (unmappers[i].isRunning()) continue;

			return &unmappers[i];
		}

		// Increase NUM_MAPPERS
		jassertfalse;
        
		return nullptr;
	}
	
private:

    friend class Unmapper;
    
	// ============================================================================================ internal methods

	int getNumSamplesForStreamingBuffers() const
	{
		jassert(b1.getNumSamples() == b2.getNumSamples());

		return b1.getNumSamples();
	}

	void requestNewData();
	
	bool swapBuffers();

	void fillInactiveBuffer();
	void refreshBufferSizes();
	// ============================================================================================ member variables


    
	Unmapper unmappers[NUM_UNMAPPERS];

	Unmapper unmapper;

	/** The class tries to be as lock free as possible (it only locks the buffer that is filled 
	*	during the read operation, but I have to lock everything for a few calls, so that's why
	*	there is a critical section.	
	*/
	CriticalSection lock;

	/** A mutex for the buffer that is being used for loading. */
	bool writeBufferIsBeingFilled;

	// variables for handling of the internal buffers

	double readIndexDouble;

	Atomic<StreamingSamplerSound const *> sound;

	int readIndex;
	int idealBufferSize;
	int minimumBufferSizeForSamplesPerBlock;

	int positionInSampleFile;

	bool isReadingFromPreloadBuffer;

    bool voiceCounterWasIncreased;
    
	int sampleStartModValue;

	Atomic<AudioSampleBuffer const *> readBuffer;
	Atomic<AudioSampleBuffer *> writeBuffer;

	Atomic<const float*> readPointerLeft;
	Atomic<const float*> readPointerRight;

	// variables for disk usage measurement

	Atomic<float> diskUsage;
	double lastCallToRequestData;

	// just a pointer to the used pool
	SampleThreadPool *backgroundPool;

	// the internal buffers

	AudioSampleBuffer b1, b2;
};


/** A SamplerVoice that streams the data from a StreamingSamplerSound
*
*	It uses a SampleLoader object to fetch the data and copies the values into an internal buffer, so you
*	don't have to bother with the SampleLoader's internals.
*/
class StreamingSamplerVoice: public SynthesiserVoice
{
public:
	StreamingSamplerVoice(SampleThreadPool *backgroundThreadPool);
	
	~StreamingSamplerVoice() {};

	/** Always returns true. */
	bool canPlaySound (SynthesiserSound*) { return true; };

	/** starts the streaming of the sound. */
	void startNote (int midiNoteNumber, float velocity, SynthesiserSound* s, int /*currentPitchWheelPosition*/) override;
	
	const StreamingSamplerSound *getLoadedSound()
	{
		return loader.getLoadedSound();
	}

	void setLoaderBufferSize(int newBufferSize)
	{
		loader.setBufferSize(newBufferSize);
	};

	/** Clears the note data and resets the loader. */
	void stopNote (float , bool /*allowTailOff*/)
	{ 
		clearCurrentNote();
		loader.reset();
	};

	/** Adds it's output to the outputBuffer. */
	void renderNextBlock(AudioSampleBuffer &outputBuffer, int startSample, int numSamples) override;

	/** You can pass a pointer with float values containing pitch information for each sample.
	*
	*	The array size should be exactly the number of samples that are calculated in the current renderNextBlock method.
	*/
	void setPitchValues(const float *pitchDataForBlock)	{ pitchData = pitchDataForBlock; };

	/** You have to call this before startNote() to calculate the pitch factor.
	*
	*	This is necessary because a StreamingSamplerSound can have multiple instances with different root notes.
	*/
	void setPitchFactor(int midiNote, int rootNote, StreamingSamplerSound *sound, double globalPitchFactor)
	{
		if(midiNote == rootNote)
		{
			uptimeDelta = globalPitchFactor;
		}
		else
		{
			uptimeDelta = jmin(sound->getPitchFactor(midiNote, rootNote) * globalPitchFactor, (double)MAX_SAMPLER_PITCH);
		}
		
	};

	/** Returns the disk usage of the voice. 
	*
	*	To get the disk usage of all voices, simply iterate over the voice list and add all disk usages.
	*/
	double getDiskUsage() {	return loader.getDiskUsage(); };

	/** Initializes its sampleBuffer. You have to call this manually, since there is no base class function. */
	void prepareToPlay(double sampleRate, int samplesPerBlock)
	{
		if(sampleRate != -1.0)
		{
			samplesForThisBlock = AudioSampleBuffer(2, samplesPerBlock * MAX_SAMPLER_PITCH);
			samplesForThisBlock.clear();

			loader.assertBufferSize(samplesPerBlock * MAX_SAMPLER_PITCH);

			setCurrentPlaybackSampleRate(sampleRate);
		}
	}

	/** Not implemented */
	virtual void controllerMoved(int /*controllerNumber*/, int /*controllerValue*/) override { };

	/** Not implemented */
	virtual void pitchWheelMoved(int /*pitchWheelValue*/) override { };

	/** resets everything. */
	void resetVoice()
	{
		voiceUptime = 0.0;
		uptimeDelta = 0.0;
		loader.reset();
		clearCurrentNote();
	};

	void setSampleStartModValue(int newValue)
	{
		jassert(newValue >= 0);

		sampleStartModValue = newValue;
	}

	

private:

	const float *pitchData;

	// This lets the wrapper class access the internal data without annoying get/setters
	friend class ModulatorSamplerVoice; 
	friend class MultiMicModulatorSamplerVoice;

	double voiceUptime;
	double uptimeDelta;

	int sampleStartModValue;

	AudioSampleBuffer samplesForThisBlock;

	SampleLoader loader;
};

#endif  // STREAMINGSAMPLER_H_INCLUDED
