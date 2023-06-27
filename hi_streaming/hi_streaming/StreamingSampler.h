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

#ifndef STREAMINGSAMPLER_H_INCLUDED
#define STREAMINGSAMPLER_H_INCLUDED

namespace hise { using namespace juce;

class ModulatorSampler;

class StreamingSamplerSound;

/** A utility class for linear interpolation between samples.
*	@ingroup utility
*
*/
class Interpolator
{
public:

	/** A simple linear interpolation.
	*
	*	@param lowValue the value of the lower index.
	*	@param highValue the value of the higher index.
	*	@param delta the sub-integer part between the two indexes (must be between 0.0f and 1.0f)
	*	@returns the interpolated value.
	*/
	template <typename Type> static double interpolateLinear(const Type x1, const Type x2, const Type delta)
	{
		jassert(isPositiveAndNotGreaterThan(delta, Type(1)));

		const Type invDelta = Type(1) - delta;
		return invDelta * x1 + delta * x2;
	}

	template <typename Type> static Type interpolateCubic(const Type x0, const Type x1, const Type x2, const Type x3, const Type alpha)
	{
		Type a = ((Type(3) * (x1 - x2)) - x0 + x3) * Type(0.5);
		Type b = x2 + x2 + x0 - (Type(5) *x1 + x3) * Type(0.5);
		Type c = (x2 - x0) * Type(0.5);
		return ((a*alpha + b)*alpha + c)*alpha + x1;
	};
};

struct StreamingHelpers
{
	/** This contains the minimal MIDI information that can be extracted from a SampleMap. */
	struct BasicMappingData
	{
		int8 lowKey = -1;
		int8 highKey = -1;
		int8 lowVelocity = -1;
		int8 highVelocity = -1;
		int8 rootNote = -1;
	};

	static void increaseBufferIfNeeded(hlac::HiseSampleBuffer& b, int numSamplesNeeded);

	static bool preloadSample(StreamingSamplerSound* s, const int preloadSize, String& errorMessage);

	/** Creates a BasicMappingData object from the given samplemap entry. */
	static BasicMappingData getBasicMappingDataFromSample(const ValueTree& sampleData);
};





class StreamingSamplerSoundPool
{
public:

	StreamingSamplerSoundPool()
	{
		afm.registerBasicFormats();
		afm.registerFormat(new hlac::HiseLosslessAudioFormat(), false);
	}

	virtual ~StreamingSamplerSoundPool()
	{

	}

	virtual void increaseNumOpenFileHandles()
	{
		++numOpenFileHandles;
	}

	virtual void decreaseNumOpenFileHandles()
	{
		--numOpenFileHandles;
		if (numOpenFileHandles < 0) numOpenFileHandles = 0;
	}

	AudioFormatManager afm;

	int getNumOpenFileHandles() const { return numOpenFileHandles; }

private:

	int numOpenFileHandles = 0;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StreamingSamplerSoundPool);
};



class DebugLogger;

// ==================================================================================================================================================

/** A simple data struct for a stereo channel.
*
*	This merges channels from an AudioSampleBuffer into one data structure which is filled by the sample rendering.
*/
struct StereoChannelData
{
	hlac::HiseSampleBuffer const* b;
	int offsetInBuffer = 0;
};

// ==================================================================================================================================================

// If you want to use the streaming engine without anything else from HISE, set this to 1 and implement your own wrapper
#define STANDALONE 0

// Set this to 1 to replace the sample content with indexes for debugging purposes (it will also mute the sound)
#define USE_SAMPLE_DEBUG_COUNTER 0


/*  This number defines the pitch ratio threshhold (8 = three octaves) that decides whether to load the entire sample into memory.
 
    Rationale: If a sample is played with a high pitch ratio, the streaming engine has to fetch more samples and copy it into the temporary
    streaming buffers (which size can be set with the `BufferSize` property of the Sampler module). This opposes a minimum size restriction on the
    streaming buffer size which is dependent on the audio buffer size because there has to be `MAX_SAMPLER_PITCH * AudioBufferSize` available in
    the streaming buffers. If a sample exceeds that pitch ratio, it will be loaded completely into memory in order to avoid the streaming buffer
    overrun - and since it will be most likely one or two samples of a sample map that are expanded to the upper hand, the memory consumption of
    these samples is much lower than having a big streaming buffer available for all voices.
 
    Note: be aware that the pitch ratio detection just takes the *static* pitch ratio into account (HiKey - RootNote). Changing the global pitch,
    adding pitch modulators or changing the `Pitch` property might lead to a pitch ratio that is above the limit, so be careful when you use these
    in combination with a high-pitched sample!
*/
#ifndef MAX_SAMPLER_PITCH
#define MAX_SAMPLER_PITCH 8
#endif

// This is the default preload size. I defined a pretty random value here, but you can change this dynamically.
#define PRELOAD_SIZE 8192

// Same as the preload size.
#define BUFFER_SIZE_FOR_STREAM_BUFFERS 4096

// Deactivate this to use one rounded pitch value for one a buffer (crucial for other interpolation methods than linear interpolation)
#define USE_SAMPLE_ACCURATE_RESAMPLING 0

#if HISE_IOS
#define DEFAULT_BUFFER_TYPE_IS_FLOAT false
#else
#define DEFAULT_BUFFER_TYPE_IS_FLOAT true
#endif
    
// You can set this to 0, if you want to disable background threaded reading. The files will then be read directly in the audio thread,
// which is not the smartest thing to do, but it comes to good use for debugging.
#define USE_BACKGROUND_THREAD 1

// If the streaming background thread is blocked, it will kill the voice to exit gracefully.
#define KILL_VOICES_WHEN_STREAMING_IS_BLOCKED 1

// By default, every voice adds its output to the supplied buffer. Depending on your architecture, it could be more practical to
// set (overwrite) the buffer. In this case, set this to 1.
#if STANDALONE
#define OVERWRITE_BUFFER_WITH_VOICE_DATA 0
#else
#define OVERWRITE_BUFFER_WITH_VOICE_DATA 1
#endif


// You can set a limit of how big the preload buffer can get before the entire sample gets loaded into memory
// (there are a few edge cases where it could result in clicks during playback so if you encounter them, try
// lowering that value. There was an issue that could be "solved" by setting this limit to 28000).
#ifndef HISE_LOAD_ENTIRE_SAMPLE_THRESHHOLD
#define HISE_LOAD_ENTIRE_SAMPLE_THRESHHOLD INT_MAX
#endif


#define NUM_UNMAPPERS 8



} // namespace hise
#endif  // STREAMINGSAMPLER_H_INCLUDED
