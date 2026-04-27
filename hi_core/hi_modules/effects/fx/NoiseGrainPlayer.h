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

#pragma once

namespace hise { using namespace juce;

/** A simple stereo panner which can be modulated using all types of Modulators
*	@ingroup effectTypes
*
*/
class NoiseGrainPlayer: public VoiceEffectProcessor,
					    public AudioSampleProcessor,
						public MultiChannelAudioBuffer::Listener
{
public:

	SET_PROCESSOR_NAME("NoiseGrainPlayer", "Noise Grain Player", "");

	static ProcessorMetadata createMetadata();

	enum InternalChains
	{
		PositionChain = 0,
		PositionBipolar,
		numInternalChains
	};

	enum Parameters
	{
		Position = 0,
		Mix,
		WhiteNoise,
		GrainSize
	};

	enum EditorStates
	{
		PositionChainShown = Processor::numEditorStates,
		PositionBipolarShown = Processor::numEditorStates,
		numEditorStates
	};

	NoiseGrainPlayer(MainController *mc, const String &uid, int numVoices);;

	float getAttribute(int parameterIndex) const override
	{
		switch(parameterIndex)
		{
		case Parameters::Position: return tableIndex;
		case Parameters::Mix: return mixGain;
		case Parameters::WhiteNoise: return whiteNoise;
		case Parameters::GrainSize: return grainSize;
		}

		return 0.0f;
	}

	void setInternalAttribute(int parameterIndex, float newValue) override
	{
		switch(parameterIndex)
		{
		case Parameters::Position: tableIndex = newValue; break;
		case Parameters::Mix: mixGain = newValue; break;
		case Parameters::WhiteNoise: whiteNoise = newValue; break;
		case Parameters::GrainSize: grainSize = jlimit(1, 6, (int)newValue); recalculate(); break;
		}
	}
	
	void restoreFromValueTree(const ValueTree &v) override
	{
		VoiceEffectProcessor::restoreFromValueTree(v);
		AudioSampleProcessor::restoreFromValueTree(v);
		loadAttribute(Position, "Position");
		loadAttribute(Mix, "Mix");
		loadAttribute(WhiteNoise, "WhiteNoise");
		loadAttribute(GrainSize, "GrainSize");
	}

	virtual void bufferWasLoaded() override { recalculate(); }
	virtual void bufferWasModified() override { recalculate(); }

	ValueTree exportAsValueTree() const override
	{
		auto v = VoiceEffectProcessor::exportAsValueTree();
		AudioSampleProcessor::saveToValueTree(v);
		saveAttribute(Position, "Position");
		saveAttribute(Mix, "Mix");
		saveAttribute(WhiteNoise, "WhiteNoise");
		saveAttribute(GrainSize, "GrainSize");
		return v;
	}

	void prepareToPlay(double sampleRate_, int samplesPerBlock_) override
	{
		VoiceEffectProcessor::prepareToPlay(sampleRate_, samplesPerBlock_);
		recalculateUptimeDelta();
	}

	void renderNextBlock(AudioSampleBuffer &buffer, int startSample, int numSamples) override {}

	void startVoice(int voiceIndex, const HiseEvent& e) override
	{
		lastStartedVoiceIndex = voiceIndex;
		VoiceEffectProcessor::startVoice(voiceIndex, e);
		voiceState[voiceIndex].reset();
		voiceState[voiceIndex].noiseAmp = whiteNoise;
	}

	void stopVoice(int voiceIndex) override
	{
		VoiceEffectProcessor::stopVoice(voiceIndex);
	}

	void reset(int voiceIndex) override
	{
		VoiceEffectProcessor::reset(voiceIndex);
	}

	bool hasTail() const override { return false; };

	Processor *getChildProcessor(int processorIndex) override { return modChains[processorIndex].getChain(); };
	const Processor *getChildProcessor(int processorIndex) const override { return modChains[processorIndex].getChain(); };
	int getNumChildProcessors() const override { return modChains.size(); };
	int getNumInternalChains() const override { return modChains.size(); };

	ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor)  override;

	void applyEffect(int voiceIndex, AudioSampleBuffer &b, int startSample, int numSamples) override;

	void referenceShared(ExternalData::DataType type, int index) override
	{
		recalculate();
	}

private:

	bool dirty = false;

	int lastStartedVoiceIndex = -1;

	void recalculateUptimeDelta()
	{
		auto sr = getSampleRate();
		auto fsr = getBuffer().sampleRate;

		if(fsr > 0.0 && sr > 0.0)
			uptimeDelta = sr / fsr;

		for(auto& v: voiceState)
			v.hopSize = hopSize;
	}

	using NoiseGrainCollection = std::vector<AudioSampleBuffer>;

	struct Grain
	{
		const AudioSampleBuffer* grainData = nullptr;
		double uptime = 0.0;
		double noiseAmp = 0.0;
		Random r;

		void reset();
		bool start(const AudioSampleBuffer& gd, double noiseAmplitude);
		bool isActive() const { return grainData != nullptr; }

		int apply(AudioSampleBuffer& b, int startSample, int numSamples, double uptimeDelta, float gain);

	};

	struct VoiceState
	{
		void reset();
		void render(AudioSampleBuffer& b, int startSample, int numSamples, double uptimeDelta, const NoiseGrainCollection& nc, double tableIndex, float mix);

		std::array<Grain, 8> grains;
		double hopSize = 0.0;
		double uptime = 0.0;

		int lastStartedIndex = -1;

		int lastHopIndex = 0;
		int currentHopIndex = 0;
		double noiseAmp = 0.0;
	};

	void recalculate();

	std::array<VoiceState, NUM_POLYPHONIC_VOICES> voiceState;
	
	double uptimeDelta = 1.0;
	NoiseGrainCollection noiseGrains;

	float tableIndex = 0.0f;
	float mixGain = 1.0f;
	int hopSize = 0;

	float whiteNoise = 0.0f;

	int grainSize = 6;

	hise::SimpleReadWriteLock grainLock;

	JUCE_DECLARE_WEAK_REFERENCEABLE(StereoEffect);
};

} // namespace hise
