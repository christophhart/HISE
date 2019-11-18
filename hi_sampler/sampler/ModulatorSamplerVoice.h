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

#ifndef MODULATORSAMPLERVOICE_H_INCLUDED
#define MODULATORSAMPLERVOICE_H_INCLUDED

namespace hise { using namespace juce;

/** A ModulatorSamplerVoice is a wrapper around a StreamingSamplerVoice with logic for modulation stuff
*	@ingroup sampler
*
*/
class ModulatorSamplerVoice : public ModulatorSynthVoice
{
public:

	// ================================================================================================================

	ModulatorSamplerVoice(ModulatorSynth *ownerSynth);
	~ModulatorSamplerVoice() {};

	// ================================================================================================================

	bool canPlaySound(SynthesiserSound*) { return true; };
	void startNote(int midiNoteNumber, float velocity, SynthesiserSound* s, int /*currentPitchWheelPosition*/) override;

	void stopNote(float velocity, bool allowTailoff) override;

	// ================================================================================================================

	void prepareToPlay(double sampleRate, int samplesPerBlock) override;
	void calculateBlock(int startSample, int numSamples) override;
	void resetVoice() override;

	virtual void setNonRealtime(bool isNonRealtime)
	{
		wrappedVoice.loader.setIsNonRealtime(isNonRealtime);
	}

	void handlePlaybackPosition(const StreamingSamplerSound * sound);

	static double limitPitchDataToMaxSamplerPitch(float * pitchData, double uptimeDelta, int startSample, int numSamples);

	// ================================================================================================================

	virtual void setLoaderBufferSize(int newBufferSize);
	virtual double getDiskUsage();
	virtual size_t getStreamingBufferSize() const;

	virtual void setStreamingBufferDataType(bool shouldBeFloat);

	// ================================================================================================================

	float getConstantCrossfadeModulationValue() const noexcept;

	const float *getCrossfadeModulationValues(int startSample, int numSamples);
	void setSampleStartModValue(float modValue) { sampleStartModValue = modValue; };
	void enablePitchModulation(bool shouldBeEnabled);
	
	/** returns the sound that is played by the voice.
	*
	*	use this method instead of SynthesiserVoice::getCurrentlyPlayingSound(), because ModulatorSynthGroups
	*	do not access this member at startNote() and it will return nullptr.
	*/
	ModulatorSamplerSound *getCurrentlyPlayingSamplerSound() const 	{ return currentlyPlayingSamplerSound; }
	
	// ================================================================================================================

protected:
	
	ModulatorSamplerSound *currentlyPlayingSamplerSound;


	ModulatorSampler *sampler;


	float velocityXFadeValue;
	float sampleStartModValue;

	// ================================================================================================================

private:

	StreamingSamplerVoice wrappedVoice;

	DebugLogger* logger;

	// ================================================================================================================

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulatorSamplerVoice)
};


/** A ModulatorSamplerVoice is a wrapper around a StreamingSamplerVoice with logic for modulation stuff
*	@ingroup sampler
*
*/
class MultiMicModulatorSamplerVoice : public ModulatorSamplerVoice
{
public:

	// ================================================================================================================

	MultiMicModulatorSamplerVoice(ModulatorSynth *ownerSynth, int numMicPositions);
	~MultiMicModulatorSamplerVoice() {};

	// ================================================================================================================

	void startNote(int midiNoteNumber, float velocity, SynthesiserSound* s, int /*currentPitchWheelPosition*/) override;
	void calculateBlock(int startSample, int numSamples) override;
	void prepareToPlay(double sampleRate, int samplesPerBlock);

	// ================================================================================================================

	void setLoaderBufferSize(int newBufferSize) override;
	double getDiskUsage() override;
	size_t getStreamingBufferSize() const override;

	void setStreamingBufferDataType(bool shouldBeFloat) override;

	/** Resets the display value for the current note. */
	void resetVoice() override;

	void setNonRealtime(bool isNonRealtime) override
	{
		for(auto v: wrappedVoices)
			v->loader.setIsNonRealtime(isNonRealtime);
	}

	// ================================================================================================================
private:

	OwnedArray<StreamingSamplerVoice> wrappedVoices;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MultiMicModulatorSamplerVoice)
};

} // namespace hise
#endif  // MODULATORSAMPLERVOICE_H_INCLUDED
