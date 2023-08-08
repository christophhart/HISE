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

#ifndef SINESYNTH_H_INCLUDED
#define SINESYNTH_H_INCLUDED

namespace hise { using namespace juce;

class SineSynth;

class SineWaveSound : public ModulatorSynthSound
{
public:
    SineWaveSound() {}

    bool appliesToNote (int /*midiNoteNumber*/) override   { return true; }
    bool appliesToChannel (int /*midiChannel*/) override   { return true; }
	bool appliesToVelocity (int /*midiChannel*/) override  { return true; }
};



class SineSynthVoice: public ModulatorSynthVoice
{
public:

	SineSynthVoice(ModulatorSynth *ownerSynth):
		ModulatorSynthVoice(ownerSynth),
		octaveTransposeFactor(1.0)
	{};

	bool canPlaySound(SynthesiserSound *) override
	{
		return true;
	};

	void startNote (int midiNoteNumber, float /*velocity*/, SynthesiserSound* , int /*currentPitchWheelPosition*/) override
	{
		ModulatorSynthVoice::startNote(midiNoteNumber, 0.0f, nullptr, -1);

		midiNoteNumber += getTransposeAmount();

		voiceUptime = (double)getCurrentHiseEvent().getStartOffset();
        
        const double cyclesPerSecond = MidiMessage::getMidiNoteInHertz (midiNoteNumber);
		const double cyclesPerSample = cyclesPerSecond / getSampleRate();

		uptimeDelta = cyclesPerSample * table->getTableSize() * octaveTransposeFactor;
        
        uptimeDelta *= getOwnerSynth()->getMainController()->getGlobalPitchFactor();
    }

	void calculateBlock(int startSample, int numSamples) override;;

	void setOctaveTransposeFactor(double newFactor)
	{
		octaveTransposeFactor = newFactor;
	}

private:

	SharedResourcePointer<SineLookupTable<2048>> table;

	double octaveTransposeFactor;
};

/** A sine wave generator.
	@ingroup synthTypes.

	This is a rather lightweight module, however if you're planning to do stuff like
	additive synthesis, I'd recommend writing your own module to save CPU since this
	would calculate the pitch modulation / event triggering for each SineSynth instance.
*/
class SineSynth: public ModulatorSynth,
				 public WaveformComponent::Broadcaster
{
public:

	ADD_DOCUMENTATION_WITH_BASECLASS(ModulatorSynth);

	SET_PROCESSOR_NAME("SineSynth", "Sine Wave Generator", "A sine wave generator");

	/** The parameters. */
	enum SpecialParameters
	{
		OctaveTranspose = ModulatorSynth::numModulatorSynthParameters, ///< The octave transpose amount (+- 5)
		SemiTones, ///< the semitones transpose amount (+- 12)
		UseFreqRatio, ///< switches between musical and harmonic-based tuning
		CoarseFreqRatio, ///< the harmonic index (= frequency multiplier)
		FineFreqRatio, ///< the harmonic detune amount
		SaturationAmount, ///< the amount of the inbuilt saturator effect
		numSineSynthParameters
	};

	SineSynth(MainController *mc, const String &id, int numVoices);;

	void restoreFromValueTree(const ValueTree &v) override
	{
		ModulatorSynth::restoreFromValueTree(v);

		loadAttribute(OctaveTranspose, "OctaveTranspose");
		loadAttribute(SemiTones, "SemiTones");
		loadAttribute(UseFreqRatio, "UseFreqRatio");
		loadAttribute(CoarseFreqRatio, "CoarseFreqRatio");
		loadAttribute(FineFreqRatio, "FineFreqRatio");
		loadAttribute(SaturationAmount, "SaturationAmount");

	};

	ValueTree exportAsValueTree() const override
	{
		ValueTree v = ModulatorSynth::exportAsValueTree();

		saveAttribute(OctaveTranspose, "OctaveTranspose");
		saveAttribute(SemiTones, "SemiTones");
		saveAttribute(UseFreqRatio, "UseFreqRatio");
		saveAttribute(CoarseFreqRatio, "CoarseFreqRatio");
		saveAttribute(FineFreqRatio, "FineFreqRatio");
		saveAttribute(SaturationAmount, "SaturationAmount");

		return v;
	}

	float getDefaultValue(int parameterIndex) const override
	{
		if (parameterIndex < ModulatorSynth::numModulatorSynthParameters) return ModulatorSynth::getDefaultValue(parameterIndex);

		switch (parameterIndex)
		{
		case OctaveTranspose:		return 0.0f;
		case SemiTones:				return 0.0f;
		case UseFreqRatio:			return 0.0f;
		case CoarseFreqRatio:		return 1.0f;
		case FineFreqRatio:			return 0.0f;
		case SaturationAmount:		return 0.0f;
		default:					jassertfalse; return -1.0f;
		}
	};

	float getAttribute(int parameterIndex) const override 
	{
		if(parameterIndex < ModulatorSynth::numModulatorSynthParameters) return ModulatorSynth::getAttribute(parameterIndex);

		switch(parameterIndex)
		{
		case OctaveTranspose:		return (float)octaveTranspose;
		case SemiTones:				return (float)semiTones;
		case UseFreqRatio:			return (float)useRatio ? 1.0f : 0.0f;
		case CoarseFreqRatio:		return coarseRatio;
		case FineFreqRatio:			return fineRatio;
		case SaturationAmount:		return saturationAmount;
		default:					jassertfalse; return -1.0f;
		}
	};

	void setInternalAttribute(int parameterIndex, float newValue) override
	{
		if(parameterIndex < ModulatorSynth::numModulatorSynthParameters)
		{
			ModulatorSynth::setInternalAttribute(parameterIndex, newValue);
			return;
		}

		switch(parameterIndex)
		{
		case OctaveTranspose:		octaveTranspose = (int)newValue; break;
									
		case SemiTones:				semiTones = (int)newValue; break;
		case UseFreqRatio:			useRatio = newValue > 0.5f; break;
		case CoarseFreqRatio:		coarseRatio = newValue; break;
		case FineFreqRatio:			fineRatio = newValue; break;
		case SaturationAmount:		saturationAmount = newValue; 
									saturator.setSaturationAmount(newValue); 
									triggerWaveformUpdate();
									return; // skip the calculation of the pitch ratio
		default:					jassertfalse;
									break;
		}

		double factor = 1.0;

		if (useRatio)
		{
			double cToUse = (double)coarseRatio - 1.0;

			if (cToUse == 0)
				factor = 1.0 + (double)fineRatio;
			else if (cToUse > 0)
				factor = 1.0 + cToUse + (double)fineRatio;
			else if (cToUse < 0)
				factor = pow(2, cToUse) + (double)fineRatio;
		}
		else
		{
			factor = pow(2.0, (double)octaveTranspose + (double)semiTones / 12.0);
		}
	
		for (int i = 0; i < getNumVoices(); i++)
		{
			static_cast<SineSynthVoice*>(getVoice(i))->setOctaveTransposeFactor(factor);
		}
	};

	ProcessorEditorBody* createEditor(ProcessorEditor *parentEditor) override;

	float const * getSaturatedTableValues();

	void getWaveformTableValues(int /*displayIndex*/, float const** tableValues, int& numValues, float& normalizeValue) override
	{
		*tableValues = getSaturatedTableValues();
		numValues = 128;
		normalizeValue = 1.0f;
	}

private:

	Saturator saturator;
	
	friend class SineSynthVoice;

	float saturatedTableValues[128];

	int octaveTranspose;
	int semiTones;

	float saturationAmount;
	float coarseRatio;
	float fineRatio;
	bool useRatio;

	JUCE_DECLARE_WEAK_REFERENCEABLE(SineSynth);
};



} // namespace hise

#endif  // SINESYNTH_H_INCLUDED
