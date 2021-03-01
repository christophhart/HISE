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

#ifndef WAVETABLESYNTH_H_INCLUDED
#define WAVETABLESYNTH_H_INCLUDED

//#include "ClarinetData.h"

namespace hise { using namespace juce;

#define WAVETABLE_HQ_MODE 1

class WavetableSynth;

class WavetableSound: public ModulatorSynthSound
{
public:

	/** Creates a new wavetable sound.
	*
	*	You have to supply a ValueTree with the following properties:
	*
	*	- 'data' the binary data
	*	- 'amount' the number of wavetables
	*	- 'noteNumber' the noteNumber
	*	- 'sampleRate' the sample rate
	*
	*/
	WavetableSound(const ValueTree &wavetableData);;

	bool appliesToNote (int midiNoteNumber) override   { return midiNotes[midiNoteNumber]; }
    bool appliesToChannel (int /*midiChannel*/) override   { return true; }
	bool appliesToVelocity (int /*midiChannel*/) override  { return true; }

	/** Returns a read pointer to the wavetable with the given index.
	*
	*	Make sure you don't get off bounds, it will return a nullptr if the index is bigger than the wavetable amount.
	*/
	const float *getWaveTableData(int wavetableIndex) const;

	float getUnnormalizedMaximum()
	{
		return unnormalizedMaximum;
	}

	int getWavetableAmount() const
	{
		return wavetableAmount;
	}

	int getTableSize() const
	{
		return wavetableSize;
	};

	float getMaxLevel() const
	{
		return maximum;
	}

	/** Call this in the prepareToPlay method to calculate the pitch error. It also resamples if the playback frequency is different. */
	void calculatePitchRatio(double playBackSampleRate);

	double getPitchRatio()
	{
		return pitchRatio;
	}

	void normalizeTables();

	float getUnnormalizedGainValue(int tableIndex)
	{
		jassert(tableIndex < 64);
		jassert(tableIndex >= 0);

		return unnormalizedGainValues[tableIndex];
	}

private:

	float maximum;
	float unnormalizedMaximum;
	float unnormalizedGainValues[64];

	BigInteger midiNotes;
	int noteNumber;

	AudioSampleBuffer wavetables;
	AudioSampleBuffer emptyBuffer;

	double sampleRate;
	double pitchRatio;

	int wavetableSize;
	int wavetableAmount;
};

class WavetableSynth;

class WavetableSynthVoice: public ModulatorSynthVoice
{
public:

	WavetableSynthVoice(ModulatorSynth *ownerSynth);

	bool canPlaySound(SynthesiserSound *) override
	{
		return true;
	};

	int getSmoothSize() const;

	void startNote (int midiNoteNumber, float /*velocity*/, SynthesiserSound* s, int /*currentPitchWheelPosition*/) override;;

	const float *getTableModulationValues();

	float getGainValue(float modValue);

	void calculateBlock(int startSample, int numSamples) override;;

	int getCurrentTableIndex() const
	{
		return currentTableIndex;
	};

	void setHqMode(bool useHqMode)
	{
		hqMode = useHqMode;

		if(hqMode)
		{
			lowerTable = currentTable;
			nextTable = upperTable;
		}
		{
			currentTable = lowerTable;
			nextTable = upperTable;
		}

	};

private:

	WavetableSynth *wavetableSynth;

	int octaveTransposeFactor;

	WavetableSound *currentSound;

	int currentWaveIndex;
	int nextWaveIndex;
	int tableSize;
	int currentTableIndex;
	int nextTableIndex;

	float currentGainValue;
	float nextGainValue;

	Interpolator tableGainInterpolator;

	bool hqMode;

	float const *lowerTable;
	float const *upperTable;



	float const *currentTable;
	float const *nextTable;

	int smoothSize;

};


/** A two-dimensional wavetable synthesiser.
	@ingroup synthTypes
*/
class WavetableSynth: public ModulatorSynth,
					  public SliderPackProcessor,
					  public WaveformComponent::Broadcaster
{
public:

	SET_PROCESSOR_NAME("WavetableSynth", "Wavetable Synthesiser", "A two-dimensional wavetable synthesiser.");

	enum EditorStates
	{
		TableIndexChainShow = ModulatorSynth::numEditorStates
	};

	enum SpecialParameters
	{
		HqMode = ModulatorSynth::numModulatorSynthParameters,
		LoadedBankIndex,
		numSpecialParameters
	};

	enum ChainIndex
	{
		Gain = 0,
		Pitch = 1,
		TableIndex = 2
	};

	enum InternalChains
	{
		TableIndexModulation = ModulatorSynth::numInternalChains,
		numInternalChains
	};

	WavetableSynth(MainController *mc, const String &id, int numVoices);;

	void loadWaveTable(const ValueTree& v)
	{
		clearSounds();
        
		jassert(v.isValid());
        
		for(int i = 0; i < v.getNumChildren(); i++)
		{
			auto s = new WavetableSound(v.getChild(i));

			s->calculatePitchRatio(getSampleRate());

			addSound(s);
		}
        
		

	}

	void getWaveformTableValues(int displayIndex, float const** tableValues, int& numValues, float& normalizeValue) override;

	float getGainValueFromTable(float level)
	{
		int index = roundToInt(128 * level);
		index = jlimit<int>(0, 127, roundToInt(128.0 * index));

		if (lastGainIndex != index)
		{
			lastGainIndex = index;
			pack->setDisplayedIndex(index);
			lastGainValue = pack->getValue(index);
		}

		return lastGainValue;
	}

	
	void restoreFromValueTree(const ValueTree &v) override
	{
		ModulatorSynth::restoreFromValueTree(v);

		loadAttribute(LoadedBankIndex, "LoadedBankIndex");
		loadAttribute(HqMode, "HqMode");


		pack->fromBase64(v.getProperty("SliderPackData"));

		//loadWaveTable();

	};

	ValueTree exportAsValueTree() const override
	{
		ValueTree v = ModulatorSynth::exportAsValueTree();

		saveAttribute(HqMode, "HqMode");
		saveAttribute(LoadedBankIndex, "LoadedBankIndex");

		v.setProperty("SliderPackData", pack->toBase64(), nullptr);

		return v;
	}


	int getNumChildProcessors() const override { return numInternalChains;	};

	int getNumInternalChains() const override {return numInternalChains; };

	virtual Processor *getChildProcessor(int processorIndex) override
	{
		jassert(processorIndex < numInternalChains);

		switch(processorIndex)
		{
		case GainModulation:	return gainChain;
		case PitchModulation:	return pitchChain;
		case TableIndexModulation:	return tableIndexChain;
		case MidiProcessor:		return midiProcessorChain;
		case EffectChain:		return effectChain;
		default:				jassertfalse; return nullptr;
		}
	};

	virtual const Processor *getChildProcessor(int processorIndex) const override
	{
		jassert(processorIndex < numInternalChains);

		switch(processorIndex)
		{
		case GainModulation:	return gainChain;
		case PitchModulation:	return pitchChain;
		case TableIndexModulation:	return tableIndexChain;
		case MidiProcessor:		return midiProcessorChain;
		case EffectChain:		return effectChain;
		default:				jassertfalse; return nullptr;
		}
	};

	void prepareToPlay(double newSampleRate, int samplesPerBlock) override
	{
		if(newSampleRate > -1.0)
		{
			for(int i = 0; i < sounds.size(); i++)
			{
				static_cast<WavetableSound*>(getSound(i))->calculatePitchRatio(newSampleRate);
			}
		}

		ModulatorSynth::prepareToPlay(newSampleRate, samplesPerBlock);

		
	}

	int getMorphSmoothing() const
	{
		return morphSmoothing;
	}

	const float *getTableModValues() const
	{
		return modChains[ChainIndex::TableIndex].getReadPointerForVoiceValues(0);
	}

	float getConstantTableModValue() const noexcept
	{
		return modChains[ChainIndex::TableIndex].getConstantModulationValue();
	}


	float getAttribute(int parameterIndex) const override 
	{
		if(parameterIndex < ModulatorSynth::numModulatorSynthParameters) return ModulatorSynth::getAttribute(parameterIndex);

		switch(parameterIndex)
		{
		case HqMode:		return hqMode ? 1.0f : 0.0f;
		case LoadedBankIndex: return (float)currentBankIndex;
		default:			jassertfalse; return -1.0f;
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
		case HqMode:
			{
				ScopedLock sl(getMainController()->getLock());

				hqMode = newValue == 1.0f;

				for(int i = 0; i < getNumVoices(); i++)
				{
					static_cast<WavetableSynthVoice*>(getVoice(i))->setHqMode(hqMode);
				}
				break;
			}
		case LoadedBankIndex:
		{
			loadWavetableFromIndex((int)newValue);
			break;

		}
		default:					jassertfalse;
									break;
		}
	};

	ProcessorEditorBody* createEditor(ProcessorEditor *parentEditor) override;

	StringArray getWavetableList() const
	{
		auto dir = getMainController()->getCurrentFileHandler().getSubDirectory(ProjectHandler::SubDirectories::AudioFiles);

		Array<File> wavetables;
		dir.findChildFiles(wavetables, File::findFiles, true, "*.hwt");
		wavetables.sort();

		StringArray sa;

		for (auto& f : wavetables)
			sa.add(f.getFileNameWithoutExtension());

		return sa;
	}

	int lastGainIndex = -1;
	float lastGainValue = 1.0f;

	void loadWavetableFromIndex(int index)
	{
		if (currentBankIndex != index)
		{
			currentBankIndex = index;
		}

		if (currentBankIndex == 0)
		{
			clearSounds();
		}

		auto dir = getMainController()->getCurrentFileHandler().getSubDirectory(ProjectHandler::SubDirectories::AudioFiles);

		Array<File> wavetables;

		dir.findChildFiles(wavetables, File::findFiles, true, "*.hwt");
		wavetables.sort();

		if (wavetables[index-1].existsAsFile())
		{
			FileInputStream fis(wavetables[index-1]);

			ValueTree v = ValueTree::readFromStream(fis);

			loadWaveTable(v);
		}
		else
		{
			clearSounds();
		}
	}


private:

	

	int currentBankIndex = 0;

	SliderPackData* pack;
	
	bool hqMode;

	ModulatorChain* tableIndexChain;

	int morphSmoothing;


};


} // namespace hise

#endif  // WAVETABLESYNTH_H_INCLUDED
