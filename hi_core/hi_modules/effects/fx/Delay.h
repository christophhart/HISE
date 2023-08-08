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

#ifndef DELAY_H_INCLUDED
#define DELAY_H_INCLUDED

namespace hise { using namespace juce;

/** A delay effect with time synching and dual channel processing.
*	@ingroup effectTypes
*/
class DelayEffect: public MasterEffectProcessor,
				   public TempoListener
{
public:

	SET_PROCESSOR_NAME("Delay", "Delay", "A stereo delay effect");

	/** The parameters*/
	enum Parameters
	{
		DelayTimeLeft = 0, ///< the left channel time in ms or TempoSync::Mode
		DelayTimeRight, ///< the right channel time in ms or TempoSync::Mode
		FeedbackLeft, ///< the left channel feedback in percent
		FeedbackRight, ///< the right channel feedback in percent
		LowPassFreq, ///< the frequency for the low pass
		HiPassFreq, ///< the frequency for the high pass
		Mix, ///< the wet amount
		TempoSync, ///< if enabled, the delay time will sync to the host tempo
		numEffectParameters
	};

	DelayEffect(MainController *mc, const String &id);;

	~DelayEffect()
	{
		getMainController()->removeTempoListener(this);
	}

	void tempoChanged(double newTempo) override
	{
		if(tempoSync)
		{
			delayTimeLeft = TempoSyncer::getTempoInMilliSeconds(newTempo, syncTimeLeft);
			delayTimeRight = TempoSyncer::getTempoInMilliSeconds(newTempo, syncTimeLeft);

			calcDelayTimes();

		}
	}

	float getAttribute(int parameterIndex) const override;;

	void setInternalAttribute(int parameterIndex, float newValue) override;;

	float getDefaultValue(int parameterIndex) const override;

	void restoreFromValueTree(const ValueTree &v) override
	{
		MasterEffectProcessor::restoreFromValueTree(v);

		loadAttribute(TempoSync, "TempoSync");
		loadAttribute(DelayTimeLeft, "DelayTimeLeft");
		loadAttribute(DelayTimeRight, "DelayTimeRight");
		loadAttribute(FeedbackLeft, "FeedbackLeft");
		loadAttribute(FeedbackRight, "FeedbackRight");
		loadAttribute(LowPassFreq, "LowPassFreq");
		loadAttribute(HiPassFreq, "HiPassFreq");
		loadAttribute(Mix, "Mix");
	};

	ValueTree exportAsValueTree() const override
	{
		ValueTree v = MasterEffectProcessor::exportAsValueTree();

		saveAttribute(DelayTimeLeft, "DelayTimeLeft");
		saveAttribute(DelayTimeRight, "DelayTimeRight");
		saveAttribute(FeedbackLeft, "FeedbackLeft");
		saveAttribute(FeedbackRight, "FeedbackRight");
		saveAttribute(LowPassFreq, "LowPassFreq");
		saveAttribute(HiPassFreq, "HiPassFreq");
		saveAttribute(Mix, "Mix");
		saveAttribute(TempoSync, "TempoSync");

		return v;
	}

	void prepareToPlay(double sampleRate, int samplesPerBlock) override
	{
		MasterEffectProcessor::prepareToPlay(sampleRate, samplesPerBlock);
        
        leftDelay.prepareToPlay(sampleRate);
        rightDelay.prepareToPlay(sampleRate);
        
		calcDelayTimes();
	};

	void calcDelayTimes()
	{
		const bool wrongDomain = tempoSync && (syncTimeLeft >= TempoSyncer::Tempo::numTempos || syncTimeRight >= TempoSyncer::Tempo::numTempos);

		if (wrongDomain)
		{
			// This happens when you convert non synced presets to tempo synced values.

			syncTimeLeft = TempoSyncer::getTempoIndexForTime(getMainController()->getBpm(), syncTimeLeft);
			syncTimeRight = TempoSyncer::getTempoIndexForTime(getMainController()->getBpm(), syncTimeRight);
		}
		
		const float actualLeftTime = tempoSync ? TempoSyncer::getTempoInMilliSeconds(getMainController()->getBpm(), syncTimeLeft) :
			delayTimeLeft;

		const float actualRightTime = tempoSync ? TempoSyncer::getTempoInMilliSeconds(getMainController()->getBpm(), syncTimeRight) :
			delayTimeRight;

        leftDelay.setDelayTimeSeconds(actualLeftTime * 0.001);
        rightDelay.setDelayTimeSeconds(actualRightTime * 0.001);
	}

	void applyEffect(AudioSampleBuffer &buffer, int startSample, int numSamples) override;;

	bool hasTail() const override {return true; };

	void voicesKilled() override
	{
		leftDelay.clear();
		rightDelay.clear();
	}

	int getNumChildProcessors() const override { return 0; };

	Processor *getChildProcessor(int /*processorIndex*/) override { return nullptr; };

	const Processor *getChildProcessor(int /*processorIndex*/) const override { return nullptr; };

	ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor)  override;

private:

	// Unsynced
	float delayTimeLeft;
	float delayTimeRight;

	float timeLeft;
	float timeRight;

	TempoSyncer::Tempo syncTimeLeft;
	TempoSyncer::Tempo syncTimeRight;

	float feedbackLeft;
	float feedbackRight;
	float lowPassFreq;
	float hiPassFreq;
	float mix;
	bool tempoSync;

    DelayLine<> leftDelay;
    DelayLine<> rightDelay;

	bool skipFirstBuffer;
};





} // namespace hise

#endif  // DELAY_H_INCLUDED
