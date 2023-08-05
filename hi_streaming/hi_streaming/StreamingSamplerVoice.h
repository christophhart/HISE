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

#ifndef STREAMINGSAMPLERVOICE_H_INCLUDED
#define STREAMINGSAMPLERVOICE_H_INCLUDED

namespace hise { using namespace juce;


/** This is a utility class that handles buffered sample streaming in a background thread.
*
*	It is derived from ThreadPoolJob, so whenever you want it to read new samples, add an instance of this
*	to a ThreadPool (but don't delete it!) and it will do what it is supposed to do.
*/
class SampleLoader : public SampleThreadPoolJob
{
public:

	/** Creates a new SampleLoader.
	*
	*	Normally you don't need to call this manually, as a StreamingSamplerVoice automatically creates a instance as member.
	*/
	SampleLoader(SampleThreadPool *pool_);;
	~SampleLoader();

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

	size_t getActualStreamingBufferSize() const;

	void setStreamingBufferDataType(bool shouldBeFloat);

	StereoChannelData fillVoiceBuffer(hlac::HiseSampleBuffer &voiceBuffer, double numSamples) const;

	/** Advances the read index and returns `false` if the streaming thread is blocked. */
	bool advanceReadIndex(double uptime);

	/** Call this whenever a sound was started.
	*
	*	This will set the read pointer to the preload buffer of the StreamingSamplerSound and start the background reading.
	*/
	void startNote(StreamingSamplerSound const *s, int sampleStartModValue);

	/** Returns the loaded sound. */
	inline const StreamingSamplerSound *getLoadedSound() const { return sound.get(); };

	class Unmapper : public SampleThreadPoolJob
	{
	public:

		Unmapper() :
			SampleThreadPoolJob("Unmapper"),
			sound(nullptr),
			loader(nullptr)
		{};

		void setLoader(SampleLoader *loader_);

		void setSoundToUnmap(const StreamingSamplerSound *s);

		JobStatus runJob() override;

	private:

		StreamingSamplerSound::Ptr sound;
		SampleLoader *loader;
	};

	/** Resets the loader (unloads the sound). */
	void reset();

	void clearLoader();

	/** Calculates and returns the disk usage.
	*
	*	It measures the time the background thread needed for the loading operation and divides it with the duration since the last
	*	call to requestNewData().
	*/
	double getDiskUsage() noexcept;;


	void setLogger(DebugLogger* l) { logger = l; }
	const CriticalSection &getLock() const { return lock; }

	/** Sets a function that checks whether the streaming engine should work asynchronously or fetch the data directly
	    in the caller thread. 
		
		This can be used in an offline render context to make sure that the multi-realtime render speed will not make
		the streaming thread choke.
	*/
	void setIsNonRealtime(bool shouldBeNonRealtime)
	{
		nonRealtime = shouldBeNonRealtime;
	}

private:

	bool nonRealtime = false;

	friend class Unmapper;

	// ============================================================================================ internal methods

	int getNumSamplesForStreamingBuffers() const;

	bool requestNewData();

	bool swapBuffers();

	void fillInactiveBuffer();
	void refreshBufferSizes();
	// ============================================================================================ member variables

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

	double lastSwapPosition = 0.0;

	Atomic<StreamingSamplerSound const *> sound;

	int readIndex;
	int idealBufferSize;
	int minimumBufferSizeForSamplesPerBlock;

	int positionInSampleFile;

	bool isReadingFromPreloadBuffer;

	bool entireSampleIsLoaded;

	bool voiceCounterWasIncreased;

	int sampleStartModValue;

	DebugLogger* logger;

	Atomic<hlac::HiseSampleBuffer const *> readBuffer;
	Atomic<hlac::HiseSampleBuffer *> writeBuffer;

	// variables for disk usage measurement

	Atomic<float> diskUsage;
	double lastCallToRequestData;

	// just a pointer to the used pool
	SampleThreadPool *backgroundPool;

	// the internal buffers

	hlac::HiseSampleBuffer b1, b2;

	bool cancelled = false;
};


/** A SamplerVoice that streams the data from a StreamingSamplerSound
*
*	It uses a SampleLoader object to fetch the data and copies the values into an internal buffer, so you
*	don't have to bother with the SampleLoader's internals.
*/
class StreamingSamplerVoice : public SynthesiserVoice
{
public:
	StreamingSamplerVoice(SampleThreadPool *backgroundThreadPool);

	~StreamingSamplerVoice() {};

	/** Always returns true. */
	bool canPlaySound(SynthesiserSound*) { return true; };

	/** starts the streaming of the sound. */
	void startNote(int midiNoteNumber, float velocity, SynthesiserSound* s, int /*currentPitchWheelPosition*/) override;

	const StreamingSamplerSound *getLoadedSound();

	void setLoaderBufferSize(int newBufferSize);;

	/** Clears the note data and resets the loader. */
	void stopNote(float, bool /*allowTailOff*/);;

	void setDebugLogger(DebugLogger* newLogger);

	void interpolateFromStereoData(int startSample, float* outL, float* outR, int numSamplesToCalculate,
	                               const float* pitchDataToUse, double thisUptimeDelta, double startAlpha,
	                               StereoChannelData data, int samplesAvailable);
	/** Adds it's output to the outputBuffer. */
	void renderNextBlock(AudioSampleBuffer &outputBuffer, int startSample, int numSamples) override;

	/** You can pass a pointer with float values containing pitch information for each sample and the delta pitch value for each sample.
	*
	*	The array size should be exactly the number of samples that are calculated in the current renderNextBlock method.
	*/
	void setPitchValues(const float *pitchDataForBlock)
	{ 
		pitchData = pitchDataForBlock; 
	};

	/** Changes the pitch of the voice after the voice start with the given multiplier. */
	void setDynamicPitchFactor(double pitchMultiplier)
	{
		uptimeDelta = constUptimeDelta * pitchMultiplier;
	}

	/** You have to call this before startNote() to calculate the pitch factor.
	*
	*	This is necessary because a StreamingSamplerSound can have multiple instances with different root notes.
	*/
	void setPitchFactor(int midiNote, int rootNote, StreamingSamplerSound *sound, double globalPitchFactor);;

	/** Returns the disk usage of the voice.
	*
	*	To get the disk usage of all voices, simply iterate over the voice list and add all disk usages.
	*/
	double getDiskUsage() { return loader.getDiskUsage(); };

	/** Initializes its sampleBuffer. You have to call this manually, since there is no base class function. */
	void prepareToPlay(double sampleRate, int samplesPerBlock);

	/** Not implemented */
	virtual void controllerMoved(int /*controllerNumber*/, int /*controllerValue*/) override { };

	/** Not implemented */
	virtual void pitchWheelMoved(int /*pitchWheelValue*/) override { };

	/** resets everything. */
	void resetVoice();;

	void setSampleStartModValue(int newValue);

	bool isActive = false;

	/** Returns the sampler's temp buffer. */
	hlac::HiseSampleBuffer *getTemporaryVoiceBuffer();

	/** Gives the voice a reference to the sampler temp buffer. */
	void setTemporaryVoiceBuffer(hlac::HiseSampleBuffer* buffer, AudioSampleBuffer* stretchBuffer);

	/** Call this once for every sampler. */
	static void initTemporaryVoiceBuffer(hlac::HiseSampleBuffer* bufferToUse, int samplesPerBlock, double maxPitchRatio);

	void setPitchCounterForThisBlock(double p) noexcept { pitchCounter = p; }

	double getUptimeDelta() const { return uptimeDelta; }

	/** Set this to false if you're using HLAC compressed monoliths. */
	void setStreamingBufferDataType(bool shouldBeFloat);

	void setEnableTimestretch(bool shouldBeEnabled, const Identifier& engineId={})
	{
		stretcher.setEnabled(shouldBeEnabled, engineId);
	}

	void setTimestretchRatio(double newRatio)
	{
		stretchRatio = jlimit(0.0625, 2.0, newRatio);
	}

	void setSkipLatency(bool shouldSkipLatency)
	{
		skipLatency = shouldSkipLatency;
	}

	void setTimestretchTonality(double tonality)
	{
		timestretchTonality = jlimit(0.0, 1.0, tonality);
	}

private:

	double timestretchTonality = 0.0;

	bool skipLatency = false;

	double pitchCounter = 0.0;

	hlac::HiseSampleBuffer* tvb = nullptr;
	AudioSampleBuffer* stretchBuffer = nullptr;

	time_stretcher stretcher;
	double stretchRatio = 1.0;

	const float *pitchData;

	// This lets the wrapper class access the internal data without annoying get/setters
	friend class ModulatorSamplerVoice;
	friend class MultiMicModulatorSamplerVoice;

	double voiceUptime;
	double uptimeDelta;

	// will be calculated at note on
	double constUptimeDelta = 1.0;

	int sampleStartModValue;

	DebugLogger* logger = nullptr;

	SampleLoader loader;
};

} // namespace hise
#endif  // STREAMINGSAMPLERVOICE_H_INCLUDED
