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

#ifndef WAVETABLESYNTH_H_INCLUDED
#define WAVETABLESYNTH_H_INCLUDED

#include "ClarinetData.h"

#define WAVETABLE_HQ_MODE 1

class WavetableSynth;


/** Converts a directory containing the individual wavetable files into a ValueTree that can be loaded by a WavetableSynth.
*
*	Simply create one of these on the stack and call convertDirectoryToValueTree.
*	It will create a ValueTree with the following structure:
*	
*		<wavetableData>
*			<wavetable noteNumber="X" amount="64" data="dataAsFloat" sampleRate="48000.0"/>
*			...
*		</wavetableData>
*
*	It assumes 48kHz mono files as input.
*
*	@param directoryPath the directory where the samples are. It assumes the following file name convenience:
*						 NOTENAME_WAVETABLEINDEX.wav - with NOTENAME base octave 3 and WAVETABLEINDEX 0 - 63
*/
class WavetableConverter
{
public:

	WavetableConverter()
	{

		data = ValueTree("wavetableData");
		
		//dataBuffers.insertMultiple(0, AudioSampleBuffer(), 128);

		for(int i = 0; i < 128; i++)
		{
			dataBuffers.add(new AudioSampleBuffer());
			
		}

		wavetableSizes.insertMultiple(0, -1, 128);
	};

	ValueTree convertDirectoryToWavetableData(const String &directoryPath_)
	{
		WavAudioFormat waf;

		double sampleRate = -1.0;

		directoryPath = File(directoryPath_);

		jassert(directoryPath.isDirectory());

		DirectoryIterator iter(directoryPath, false, "*.wav");

		while(iter.next())
		{
			File wavFile = iter.getFile();

			const String noteName = wavFile.getFileNameWithoutExtension().upToFirstOccurrenceOf("_", false, false);

			const int noteNumber = getMidiNoteNumber(noteName);

			if(noteNumber != -1)
			{
				juce::AudioSampleBuffer *bufferOfNote = dataBuffers[noteNumber];

				const int wavetableNumber = wavFile.getFileNameWithoutExtension().fromFirstOccurrenceOf("_", false, false).getIntValue();

				jassert(wavetableNumber >= 0);

				reader = waf.createMemoryMappedReader(wavFile);

				reader->mapEntireFile();

				sampleRate = reader->sampleRate;

				const int numSamples = (int)reader->lengthInSamples;

				if(bufferOfNote->getNumSamples() != numSamples * 64)
				{
					jassert(wavetableSizes[noteNumber] == -1);

					bufferOfNote->setSize(2, numSamples * 64);
					bufferOfNote->clear();

				}


				jassert(wavetableSizes[noteNumber] == numSamples || wavetableSizes[noteNumber] == -1);

				wavetableSizes.set(noteNumber, numSamples);

				reader->read(bufferOfNote, numSamples * wavetableNumber, numSamples, 0, true, true);


				
			}
		}
		

		for(int i = 0; i < 128; i++)
		{
			if(wavetableSizes[i] == -1) continue;

			jassert(dataBuffers[i]->getNumSamples() == wavetableSizes[i] * 64);

			jassert(dataBuffers[i]->getMagnitude(0, 0, dataBuffers[i]->getNumSamples()) != 0.0f);

			ValueTree child = ValueTree("wavetable");

			child.setProperty("noteNumber", i, nullptr);

			child.setProperty("amount", 64, nullptr);

			child.setProperty("sampleRate", sampleRate, nullptr);

			MemoryBlock mb(dataBuffers[i]->getReadPointer(0, 0), dataBuffers[i]->getNumSamples() * sizeof(float));

			var binaryData(mb);

			child.setProperty("data", binaryData, nullptr);

			data.addChild(child, -1, nullptr);

		}
		
		return data;

	}

	


	static int getWavetableLength(int noteNumber, double sampleRate)
	{
		const double freq = MidiMessage::getMidiNoteInHertz(noteNumber);
		const int sampleNumber = (int)(sampleRate / freq);

		return sampleNumber;
	};

	static int getMidiNoteNumber(const String &midiNoteName)
	{
		for(int i = 0; i < 127; i++)
		{
			if(MidiMessage::getMidiNoteName(i, true, true, 3) == midiNoteName)
			{
				return i;
			}

		}

		return -1;
	}

private:



	ScopedPointer<MemoryMappedAudioFormatReader> reader;

	File directoryPath;

	ValueTree data;

	OwnedArray<AudioSampleBuffer> dataBuffers;

	Array<int> wavetableSizes;
};


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
	WavetableSound(const ValueTree &wavetableData)
	{
		jassert(wavetableData.getType() == Identifier("wavetable"));

		
		
		MemoryBlock mb = MemoryBlock(*wavetableData.getProperty("data", var::undefined()).getBinaryData());
		const int numSamples = (int)(mb.getSize() / sizeof(float));
		wavetables.setSize(1, numSamples);
		FloatVectorOperations::copy(wavetables.getWritePointer(0, 0), (float*)mb.getData(), numSamples);

		maximum = wavetables.getMagnitude(0, numSamples);

		wavetableAmount = wavetableData.getProperty("amount", 64);

		sampleRate = wavetableData.getProperty("sampleRate", 48000.0);

		midiNotes.setRange(0, 127, false);
		noteNumber = wavetableData.getProperty("noteNumber", 0);
		midiNotes.setBit(noteNumber, true);

		wavetableSize = numSamples / wavetableAmount;		

		emptyBuffer = AudioSampleBuffer(1, wavetableSize);
		emptyBuffer.clear();

		unnormalizedMaximum = 0.0f;

		normalizeTables();

		pitchRatio = 1.0;
	};

	bool appliesToNote (int midiNoteNumber) override   { return midiNotes[midiNoteNumber]; }
    bool appliesToChannel (int /*midiChannel*/) override   { return true; }
	bool appliesToVelocity (int /*midiChannel*/) override  { return true; }

	/** Returns a read pointer to the wavetable with the given index.
	*
	*	Make sure you don't get off bounds, it will return a nullptr if the index is bigger than the wavetable amount.
	*/
	const float *getWaveTableData(int wavetableIndex) const
	{
		if(wavetableIndex < wavetableAmount)
		{
			jassert((wavetableIndex+1) * wavetableSize <= wavetables.getNumSamples());

			return wavetables.getReadPointer(0, wavetableIndex * wavetableSize);

		}
		else
		{
			return nullptr;
		}
	}

	float getUnnormalizedMaximum()
	{
		return unnormalizedMaximum;
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
	void calculatePitchRatio(double playBackSampleRate)
	{
		const double idealCycleLength = playBackSampleRate / MidiMessage::getMidiNoteInHertz(noteNumber);

		pitchRatio = 1.0;

		pitchRatio = (double)wavetableSize / idealCycleLength;

		//pitchRatio *= sampleRate / playBackSampleRate;

	}

	double getPitchRatio()
	{
		return pitchRatio;
	}

	void normalizeTables()
	{
		

		for(int i = 0; i < wavetableAmount; i++)
		{
			float *data = wavetables.getWritePointer(0, i * wavetableSize);

			const float peak = wavetables.getMagnitude(i * wavetableSize, wavetableSize);

			

			unnormalizedGainValues[i] = peak;

			if(peak == 0.0f) continue;

			if(peak > unnormalizedMaximum)
			{
				unnormalizedMaximum = peak;
			}

			FloatVectorOperations::multiply(data, 1.0f / peak, wavetableSize);
		}

		maximum = 1.0f;

	}

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

	void startNote (int midiNoteNumber, float /*velocity*/, SynthesiserSound* s, int /*currentPitchWheelPosition*/) override
	{
		ModulatorSynthVoice::startNote(midiNoteNumber, 0.0f, nullptr, -1);

		midiNoteNumber += getTransposeAmount();

		currentSound = static_cast<WavetableSound*>(s);

        voiceUptime = 0.0;
        
		lowerTable = currentSound->getWaveTableData(0);
		upperTable = lowerTable;

		nextTable = lowerTable;


		nextGainValue = getGainValue(0.0);
		

		nextTableIndex = 0;
		currentTableIndex = 0;

		tableSize = currentSound->getTableSize();
		
		smoothSize = tableSize;

		uptimeDelta = currentSound->getPitchRatio();
        
        uptimeDelta *= getOwnerSynth()->getMainController()->getGlobalPitchFactor();
    };

	const float *getTableModulationValues(int startSample, int numSamples);

	float getGainValue(float modValue);

	void calculateBlock(int startSample, int numSamples) override
	{
		const int startIndex = startSample;
		const int samplesToCopy = numSamples;

		const float *voicePitchValues = getVoicePitchValues();
		const float *modValues = getVoiceGainValues(startSample, numSamples);
		const float *tableValues = getTableModulationValues(startSample, numSamples);

		if(hqMode)
		{
			while(--numSamples >= 0)
			{
				// Get the two indexes for the table position

				int index = (int)voiceUptime;
            
				const int i1 = index % (tableSize);

				int i2 = i1 + 1;

				if(i2 >= tableSize)
				{
					i2 = 0;
				}

				const float tableModValue = tableValues[startSample];

				const float tableValue = jlimit<float>(0.0f, 1.0f, tableModValue) * 63.0f;

				const int lowerTableIndex = (int)(tableValue);
				const int upperTableIndex = jmin(63, lowerTableIndex + 1);
				const float tableDelta = tableValue - (float)lowerTableIndex;
				jassert(0.0f <= tableDelta && tableDelta <= 1.0f);

				lowerTable = currentSound->getWaveTableData(lowerTableIndex);
				upperTable = currentSound->getWaveTableData(upperTableIndex);

				float tableGainValue = tableGainInterpolator.interpolateLinear(currentSound->getUnnormalizedGainValue(lowerTableIndex), currentSound->getUnnormalizedGainValue(upperTableIndex), tableDelta);

				tableGainValue *= getGainValue(tableModValue);

				float u1 = upperTable[i1];
				float u2 = upperTable[i2];

				float l1 = lowerTable[i1];
				float l2 = lowerTable[i2];

				const float alpha = float(voiceUptime) - (float)index;
				//const float invAlpha = 1.0f - alpha;

				const float upperSample = tableGainInterpolator.interpolateLinear(u1, u2, alpha);

				const float lowerSample = tableGainInterpolator.interpolateLinear(l1, l2, alpha);
			
				float sample = lowerTableIndex != upperTableIndex ? tableGainInterpolator.interpolateLinear(lowerSample, upperSample ,tableDelta) : lowerSample;

				sample *= tableGainValue;

				sample *= 1.0f / currentSound->getUnnormalizedMaximum();

				// Stereo mode assumed
				voiceBuffer.setSample (0, startSample, sample);
				voiceBuffer.setSample (1, startSample, sample);

				jassert(voicePitchValues == nullptr || voicePitchValues[startSample] > 0.0f);

				const double delta = (uptimeDelta * (voicePitchValues == nullptr ? 1.0 : voicePitchValues[startSample]));

				voiceUptime += delta;

				++startSample;    

			}
		}
		else
		{


			while (--numSamples >= 0)
			{
				int index = (int)voiceUptime;
            
				const int i1 = index % (tableSize);

				int i2 = i1 + 1;

				if(i2 >= tableSize)
				{
					i2 = 0;
				}

				//const int i2 = (index +1) % (tableSize);

				const int smoothIndex = index % smoothSize;

				if(index == 0 ||  i2 == 0)
				{
					const float tableModValue = tableValues[startSample];

					currentTableIndex = nextTableIndex;
					nextTableIndex = roundToInt(tableModValue * 63);

					//DBG(nextTableIndex);

					currentGainValue = nextGainValue;
					nextGainValue = getGainValue(tableModValue);

					currentTable = nextTable;
					nextTable = currentSound->getWaveTableData(nextTableIndex);
				}


				const float tableModValue = tableValues[startSample];

			

				const int tableGainLowIndex = (int)(tableModValue * 63);
				const int tableGainHighIndex = jmin(63, tableGainLowIndex + 1);
				const float tableGainAlpha = tableModValue * 63 - tableGainLowIndex;

				float tableGainValue = tableGainInterpolator.interpolateLinear(currentSound->getUnnormalizedGainValue(tableGainLowIndex), currentSound->getUnnormalizedGainValue(tableGainHighIndex), tableGainAlpha);

				tableGainValue *= getGainValue(tableModValue);

				float v1 = currentTable[i1];
				float v2 = currentTable[i2];

				float n1 = nextTable[i1];
				float n2 = nextTable[i2];

				const float alpha = float(voiceUptime) - (float)index;
				//const float invAlpha = 1.0f - alpha;

				const float currentSample = tableGainInterpolator.interpolateLinear(v1, v2, alpha);

				const float nextSample = tableGainInterpolator.interpolateLinear(n1, n2, alpha);

				const float tableAlpha = (float)smoothIndex / (float) (smoothSize -1);
			

				currentGainValue = 1.0f;
				nextGainValue = 1.0f;

				float sample = tableGainInterpolator.interpolateLinear(currentSample, nextSample ,tableAlpha);

				sample *= tableGainValue;

				sample *= 1.0f / currentSound->getUnnormalizedMaximum();

				// Stereo mode assumed
				voiceBuffer.setSample (0, startSample, sample);
				voiceBuffer.setSample (1, startSample, sample);

				jassert(voicePitchValues[startSample] > 0.0f);

				const double delta = (uptimeDelta * voicePitchValues[startSample]);

				voiceUptime += delta;


			

				++startSample;    
			}

		}

		getOwnerSynth()->effectChain->renderVoice(voiceIndex, voiceBuffer, startIndex, samplesToCopy);

		FloatVectorOperations::multiply(voiceBuffer.getWritePointer(0, startIndex), modValues + startIndex, samplesToCopy);
		FloatVectorOperations::multiply(voiceBuffer.getWritePointer(1, startIndex), modValues + startIndex, samplesToCopy);
	};

	int getCurrentTableIndex() const
	{
		return currentTableIndex;
	};

	void stopNote(float velocity, bool allowTailoff) override;

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



class WavetableSynth: public ModulatorSynth
{
public:

	SET_PROCESSOR_NAME("WavetableSynth", "Wavetable Synthesiser")

	enum EditorStates
	{
		TableIndexChainShow = ModulatorSynth::numEditorStates
	};

	enum SpecialParameters
	{
		HqMode = ModulatorSynth::numModulatorSynthParameters
	};

	enum InternalChains
	{
		TableIndexModulation = ModulatorSynth::numInternalChains,
		numInternalChains
	};

	WavetableSynth(MainController *mc, const String &id, int numVoices):
		ModulatorSynth(mc, id, numVoices),
		morphSmoothing(700),
		tableIndexChain(new ModulatorChain(mc, "Table Index", numVoices, Modulation::GainMode, this))
	{
		parameterNames.add("HqMode");
		editorStateIdentifiers.add("TableIndexChainShown");

		for(int i = 0; i < numVoices; i++) addVoice(new WavetableSynthVoice(this));
		
		tableIndexChain->setColour(Colour(0xff4D54B3));

		loadWaveTable();

		disableChain((ModulatorSynth::InternalChains)(int)TableIndexModulation, false);

		gainTable = new SampleLookupTable();

		gainTable->setLengthInSamples(512);

	};

	void loadWaveTable()
	{
		clearSounds();
        


		MemoryInputStream mis(ClarinetData::clarinet_wavetables, ClarinetData::clarinet_wavetablesSize, false);
     
        ValueTree v = ValueTree::readFromStream(mis);
        
		jassert(v.isValid());
        
		for(int i = 0; i < v.getNumChildren(); i++)
		{
			addSound(new WavetableSound(v.getChild(i)));
		}
        
		

	}

	Table *getGainTable()
	{
		return gainTable;
	}

	float getGainValueFromTable(float level)
	{
		return gainTable->getInterpolatedValue(level * 512.0);
	}

	
	void restoreFromValueTree(const ValueTree &v) override
	{
		ModulatorSynth::restoreFromValueTree(v);

		loadAttribute(HqMode, "HqMode");

		loadTable(gainTable, "GainTable");

		//loadWaveTable();

	};

	ValueTree exportAsValueTree() const override
	{
		ValueTree v = ModulatorSynth::exportAsValueTree();

		saveAttribute(HqMode, "HqMode");
		
		saveTable(gainTable, "GainTable");

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
			tableBuffer = AudioSampleBuffer(1, samplesPerBlock);


			tableIndexChain->prepareToPlay(newSampleRate, samplesPerBlock);

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

	
	void preHiseEventCallback(const HiseEvent &m) override
	{
		tableIndexChain->handleHiseEvent(m);

		ModulatorSynth::preHiseEventCallback(m);
	}

	void preStartVoice(int voiceIndex, int noteNumber) override
	{
		ModulatorSynth::preStartVoice(voiceIndex, noteNumber);

		tableIndexChain->startVoice(voiceIndex);

	};

	/** This method is called to handle all modulatorchains just before the voice rendering. */
	void preVoiceRendering(int startSample, int numThisTime)
	{
		tableIndexChain->renderNextBlock(tableBuffer, startSample, numThisTime);

		ModulatorSynth::preVoiceRendering(startSample, numThisTime);

		if( !isChainDisabled(EffectChain)) effectChain->preRenderCallback(startSample, numThisTime);
	};

	void calculateTableModulationValuesForVoice(int voiceIndex, int startSample, int numSamples)
	{
		tableIndexChain->renderVoice(voiceIndex, startSample, numSamples);

		float *tableValues = tableIndexChain->getVoiceValues(voiceIndex);

		const float* timeVariantTableValues = tableBuffer.getReadPointer(0);

		FloatVectorOperations::multiply(tableValues, timeVariantTableValues, startSample + numSamples);



	}

	const float *getTableModValues(int voiceIndex) const
	{
		return tableIndexChain->getVoiceValues(voiceIndex);
	}

	float getAttribute(int parameterIndex) const override 
	{
		if(parameterIndex < ModulatorSynth::numModulatorSynthParameters) return ModulatorSynth::getAttribute(parameterIndex);

		switch(parameterIndex)
		{
		case HqMode:		return hqMode ? 1.0f : 0.0f;
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
				ScopedLock sl(getSynthLock());

				hqMode = newValue == 1.0f;

				for(int i = 0; i < getNumVoices(); i++)
				{
					static_cast<WavetableSynthVoice*>(getVoice(i))->setHqMode(hqMode);
				}
				break;
			}
		default:					jassertfalse;
									break;
		}
	};

	ProcessorEditorBody* createEditor(ProcessorEditor *parentEditor) override;

private:

	bool hqMode;

	ScopedPointer<SampleLookupTable> gainTable;

	ScopedPointer<ModulatorChain> tableIndexChain;

	int morphSmoothing;

	AudioSampleBuffer tableBuffer;

};



#endif  // WAVETABLESYNTH_H_INCLUDED
