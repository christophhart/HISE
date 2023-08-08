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

#ifndef GAINEFFECT_H_INCLUDED
#define GAINEFFECT_H_INCLUDED

namespace hise { using namespace juce;

/** A simple effect that does nothing. */
class EmptyFX : public MasterEffectProcessor
{
public:

	SET_PROCESSOR_NAME("EmptyFX", "Empty", "A simple effect that does nothing.");

	EmptyFX(MainController *mc, const String &uid) :
		MasterEffectProcessor(mc, uid)
	{
		finaliseModChains();
	};

	~EmptyFX()
	{};

	void setInternalAttribute(int /*parameterIndex*/, float /*newValue*/) override {};
	float getAttribute(int /*parameterIndex*/) const override { return 0.0f; };

	bool hasTail() const override { return false; };

	int getNumInternalChains() const override { return 0; };
	int getNumChildProcessors() const override { return 0; };

	void setSoftBypass(bool /*shouldBeSoftBypassed*/, bool /*useRamp*//* =true */) override {};

	bool isFadeOutPending() const noexcept override
	{
		return false;
	}

	Processor *getChildProcessor(int /*processorIndex*/) override
	{
		return nullptr;
	};

	const Processor *getChildProcessor(int /*processorIndex*/) const override
	{
		return nullptr;
	};

	ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor)  override;

	void applyEffect(AudioSampleBuffer &/*b*/, int /*startSample*/, int /*numSamples*/)
	{

	}

};


/** A simple effect that does nothing. */
class MidiMetronome : public MasterEffectProcessor
{
public:

	enum Parameters
	{
		Enabled,
		Volume,
		NoiseAmount,
		numParameters
	};

	SET_PROCESSOR_NAME("MidiMetronome", "MidiMetronome", "A simple metronome that connects to a MIDI player.");

	MidiMetronome(MainController *mc, const String &uid) :
		MasterEffectProcessor(mc, uid)
	{
		finaliseModChains();
	};

	~MidiMetronome()
	{};

	void setInternalAttribute(int parameterIndex, float newValue) override
	{
		switch (parameterIndex)
		{
		case Parameters::Enabled: enabled = newValue > 0.5f; break;
		case Parameters::Volume: gain = Decibels::decibelsToGain(newValue); break;
		case Parameters::NoiseAmount: noiseAmount = newValue; break;
		default:
			break;
		}
	};

	float getAttribute(int parameterIndex) const override
	{
		switch (parameterIndex)
		{
		case Parameters::Enabled: return enabled ? 1.0f : 0.0f;
		case Parameters::Volume:  return Decibels::gainToDecibels(gain);
		case Parameters::NoiseAmount: return noiseAmount;
		default:	return 0.0f;
		}
	};

	float getDefaultValue(int parameterIndex) const override
	{
		switch (parameterIndex)
		{
		case Parameters::Enabled:     return 0.0f;
		case Parameters::Volume:      return -12.0f;
		case Parameters::NoiseAmount: return 0.5f;
		default:	return 0.0f;
		}
	}

	void restoreFromValueTree(const ValueTree &v) override
	{
		MasterEffectProcessor::restoreFromValueTree(v);
		connectToPlayer(v.getProperty("PlayerID", ""));

		loadAttribute(Enabled, "Enabled");
		loadAttribute(Volume, "Volume");
		loadAttribute(NoiseAmount, "NoiseAmount");

	}

	ValueTree exportAsValueTree() const override
	{
		auto v = MasterEffectProcessor::exportAsValueTree();
		v.setProperty("PlayerID", getConnectedId(), nullptr);

		saveAttribute(Enabled, "Enabled");
		saveAttribute(Volume, "Volume");
		saveAttribute(NoiseAmount, "NoiseAmount");
		return v;
	}

	bool hasTail() const override { return false; };

	int getNumInternalChains() const override { return 0; };
	int getNumChildProcessors() const override { return 0; };

	void setSoftBypass(bool /*shouldBeSoftBypassed*/, bool /*useRamp*//* =true */) override {};

	bool isFadeOutPending() const noexcept override
	{
		return false;
	}

	Processor *getChildProcessor(int /*processorIndex*/) override
	{
		return nullptr;
	};

	const Processor *getChildProcessor(int /*processorIndex*/) const override
	{
		return nullptr;
	};

	ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor)  override;

	void applyEffect(AudioSampleBuffer &b, int startSample, int numSamples)
	{
		if (enabled && player != nullptr)
		{
			if (auto seq = player->getCurrentSequence())
			{
				nom = (int)seq->getTimeSignaturePtr()->nominator;
				denom = (int)seq->getTimeSignaturePtr()->denominator;
			}

			auto thisQuarter = getBeatFromPosition();

			if (thisQuarter == -1)
			{
				lastQuarter = -1;
				uptimeDelta = 0.0;
				return;
			}

			if (thisQuarter != lastQuarter && thisQuarter >= 0)
			{
				rampValue = 1.0f;

				uptimeDelta = JUCE_LIVE_CONSTANT_OFF(0.1);
				uptime = 0.0;

				if (thisQuarter % nom == 0)
					uptimeDelta *= 2.0;

				lastQuarter = thisQuarter;
			}

			if (rampValue != 0.0f)
			{
				auto l = b.getWritePointer(0, startSample);
				auto r = b.getWritePointer(1, startSample);

				while (--numSamples >= 0)
				{
					rampValue *= JUCE_LIVE_CONSTANT_OFF(0.9988f);

					auto n = (Random::getSystemRandom().nextFloat() * 0.5f - 0.5f) * rampValue;

					auto s = std::sin(uptime) * rampValue;

					uptime += uptimeDelta;

					auto mValue = gain * (noiseAmount * n + (1.0f - noiseAmount) * s);

					*l++ += (float)mValue;
					*r++ += (float)mValue;
				}
			}
		}
	}

	int getBeatFromPosition()
	{
		if (player->getPlayState() == MidiPlayer::PlayState::Stop)
			return -1;

		if (player->getCurrentSequence() == nullptr)
			return -1;

		auto bf = (float)denom / 4.0f;

		lastPos = player->getPlaybackPosition() * player->getCurrentSequence()->getLengthInQuarters() * bf;

        using namespace std;
        
		return (int)floor(lastPos);
	}

	void connectToPlayer(MidiPlayer* p)
	{
		player = p;
		sendChangeMessage();
	}

	void connectToPlayer(const String& playerId)
	{
		connectToPlayer(dynamic_cast<MidiPlayer*>(ProcessorHelpers::getFirstProcessorWithName(getMainController()->getMainSynthChain(), playerId)));
	}

	String getConnectedId() const
	{
		return player != nullptr ? player->getId() : "";
	}

	WeakReference<MidiPlayer> player;

	float rampValue = 0.0f;
	int lastQuarter = -1;

	float noiseAmount = 0.5f;
	float gain = 0.25f;

	bool enabled = false;
	double lastPos = 0.0;
	double uptime = 0.0;
	double uptimeDelta = 0.0;

	int nom = 4;
	int denom = 4;


	JUCE_DECLARE_WEAK_REFERENCEABLE(MidiMetronome);
};



/** A utility effect that allows smooth gain changes, static delays and panning.
	@ingroup effectTypes
	
*/
class GainEffect: public MasterEffectProcessor
{
public:

	SET_PROCESSOR_NAME("SimpleGain", "Simple Gain", "A utility effect that allows smooth gain changes, static delays and panning.")

	enum InternalChains
	{
		GainChain = 0,
        DelayChain,
        WidthChain,
		BalanceChain,
		numInternalChains
	};

	enum EditorStates
	{
		GainChainShown = Processor::numEditorStates,
        DelayChainShown,
        WidthChainShown,
		BalanceChainShown,
		numEditorStates
	};

	enum Parameters
	{
		Gain = 0,
        Delay,
        Width,
		Balance,
		InvertPolarity,
		numParameters
	};

	GainEffect(MainController *mc, const String &uid);;

    ~GainEffect();

	void setInternalAttribute(int parameterIndex, float newValue) override;;
	float getAttribute(int parameterIndex) const override;;
	float getDefaultValue(int ) const override
	{
		return 0.0f;
	}

	void restoreFromValueTree(const ValueTree &v) override;;
	ValueTree exportAsValueTree() const override;

	bool hasTail() const override { return false; };

	Processor *getChildProcessor(int processorIndex) override
    {
        switch(processorIndex)
        {
            case GainChain:	 return gainChain;
            case DelayChain: return delayChain;
            case WidthChain: return widthChain;
			case BalanceChain: return balanceChain;
			default:		 return nullptr;
        }
    };
    
	const Processor *getChildProcessor(int processorIndex) const override
    {
        switch(processorIndex)
        {
            case GainChain:  return gainChain;
            case DelayChain: return delayChain;
            case WidthChain: return widthChain;
			case BalanceChain: return balanceChain;
			default:		 return nullptr;
        }
    };
    
	int getNumInternalChains() const override { return numInternalChains; };
	int getNumChildProcessors() const override { return numInternalChains; };

	ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor)  override;

	void prepareToPlay(double sampleRate, int samplesPerBlock);
	void applyEffect(AudioSampleBuffer &b, int startSample, int numSamples) override;

    void setDelayTime(float newDelayInMilliseconds)
    {
        delay = newDelayInMilliseconds;
        leftDelay.setDelayTimeSeconds(delay/1000.0f);
        rightDelay.setDelayTimeSeconds(delay/1000.0f);
    }
    
private:

	bool invertPolarity = false;
	float gain;
    float delay;
	float balance;

	ModulatorChain* gainChain;
    ModulatorChain* delayChain;
    ModulatorChain* widthChain;
	ModulatorChain* balanceChain;

	LinearSmoothedValue<float> smoothedGainL;
	LinearSmoothedValue<float> smoothedGainR;
	Smoother smoother;
	Smoother balanceSmoother;

    MidSideDecoder msDecoder;
    
    DelayLine<16384> leftDelay;
    DelayLine<16384> rightDelay;
  
	JUCE_DECLARE_WEAK_REFERENCEABLE(GainEffect)
};

} // namespace hise

#endif  // GAINEFFECT_H_INCLUDED
