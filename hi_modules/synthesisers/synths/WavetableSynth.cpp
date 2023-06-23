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

//#include "ClarinetData.cpp"

namespace hise { using namespace juce;

WavetableSynth::WavetableSynth(MainController *mc, const String &id, int numVoices) :
	ModulatorSynth(mc, id, numVoices),
	SliderPackProcessor(mc, 1),
	morphSmoothing(700)
{
	pack = getSliderPackUnchecked(0);

	modChains += {this, "Table Index"};

	finaliseModChains();

	tableIndexChain = modChains[ChainIndex::TableIndex].getChain();

	modChains[ChainIndex::TableIndex].setIncludeMonophonicValuesInVoiceRendering(true);
	modChains[ChainIndex::TableIndex].setExpandToAudioRate(true);

	parameterNames.add("HqMode");
	parameterNames.add("LoadedBankIndex");
	editorStateIdentifiers.add("TableIndexChainShown");

	for (int i = 0; i < numVoices; i++) addVoice(new WavetableSynthVoice(this));

	tableIndexChain->setColour(Colour(0xff4D54B3));

	pack = new SliderPackData(nullptr, mc->getGlobalUIUpdater());
	pack->setNumSliders(128);
	pack->setRange(0.0, 1.0, 0.01);
	pack->setFlashActive(true);

	for (int i = 0; i < 128; i++)
	{
		pack->setValue(i, 1.0f, dontSendNotification);
	}
}

void WavetableSynth::getWaveformTableValues(int /*displayIndex*/, float const** tableValues, int& numValues, float& normalizeValue)
{
	WavetableSynthVoice *wavetableVoice = dynamic_cast<WavetableSynthVoice*>(getLastStartedVoice());

	if (wavetableVoice != nullptr)
	{
		WavetableSound *wavetableSound = dynamic_cast<WavetableSound*>(wavetableVoice->getCurrentlyPlayingSound().get());

		if (wavetableSound != nullptr)
		{
			const int tableIndex = wavetableVoice->getCurrentTableIndex();
			*tableValues = wavetableSound->getWaveTableData(tableIndex);
			numValues = wavetableSound->getTableSize();
			normalizeValue = 1.0f / wavetableSound->getMaxLevel();
		}
	}
	else
	{
		*tableValues = nullptr;
		numValues = 0;
		normalizeValue = 1.0f;
	}
}

ProcessorEditorBody* WavetableSynth::createEditor(ProcessorEditor *parentEditor)
{
#if USE_BACKEND

	return new WavetableBody(parentEditor);

#else 

	ignoreUnused(parentEditor);
	jassertfalse;
	return nullptr;

#endif
}


WavetableSynthVoice::WavetableSynthVoice(ModulatorSynth *ownerSynth):
	ModulatorSynthVoice(ownerSynth),
	wavetableSynth(dynamic_cast<WavetableSynth*>(ownerSynth)),
	octaveTransposeFactor(1),
	currentSound(nullptr),
	hqMode(true)
{
		
};

float WavetableSynthVoice::getGainValue(float modValue)
{
	return wavetableSynth->getGainValueFromTable(modValue);
}

template <typename Type> static Type interpolateCubic(Type x0, Type x1, Type x2, Type x3, Type alpha)
{
    Type a = ((Type(3) * (x1 - x2)) - x0 + x3) * Type(0.5);
    Type b = x2 + x2 + x0 - (Type(5) *x1 + x3) * Type(0.5);
    Type c = (x2 - x0) * Type(0.5);
    return ((a*alpha + b)*alpha + c)*alpha + x1;
};

bool WavetableSynthVoice::updateSoundFromPitchFactor(double pitchFactor, WavetableSound* soundToUse)
{
    if(soundToUse == nullptr)
    {
        auto thisFreq = startFrequency * pitchFactor;
        
        if(!currentSound->getFrequencyRange().contains(thisFreq))
        {
            auto owner = getOwnerSynth();
            
            for(int i = 0; i < owner->getNumSounds(); i++)
            {
                auto ws = static_cast<WavetableSound*>(owner->getSound(i));
                
                if(ws->getFrequencyRange().contains(thisFreq))
                {
                    soundToUse = ws;
                    break;
                }
            }
        }
    }
    
    if(soundToUse == nullptr)
        return false;
    
    if(currentSound != soundToUse)
    {
        currentSound = soundToUse;
        
        lowerTable = currentSound->getWaveTableData(currentTableIndex);
        upperTable = lowerTable;

        currentTableIndex = 0;

        tableSize = currentSound->getTableSize();
        
        uptimeDelta = currentSound->getPitchRatio(noteNumberAtStart);
        uptimeDelta *= getOwnerSynth()->getMainController()->getGlobalPitchFactor();
        
        return true;
    }
    
    
    return false;
}

void WavetableSynthVoice::calculateBlock(int startSample, int numSamples)
{
	const int startIndex = startSample;
	const int samplesToCopy = numSamples;

	const float *voicePitchValues = getOwnerSynth()->getPitchValuesForVoice();
	
    if(voicePitchValues != nullptr)
    {
        updateSoundFromPitchFactor(voicePitchValues[0], nullptr);
    }
    
	if (auto tableValues = getTableModulationValues())
	{
		while (--numSamples >= 0)
		{
			int index = (int)voiceUptime;

            const auto i1 = index % (tableSize);

            auto i2 = i1 + 1;
            auto i0 = i1 - 1;
            auto i3 = i1 + 2;
            
            auto numTables = currentSound->getWavetableAmount();
            
            if (i2 >= tableSize)
            {
                const float tableModValue = tableValues[startSample];
                currentTableIndex = roundToInt(tableModValue * (double)(numTables-1));
                i2 = 0;
            }
            
            if (i1 == 0)         i0 = tableSize-1;
            if (i2 >= tableSize) i2 = 0;
            if (i3 >= tableSize) i3 = 0;

			const float tableModValue = tableValues[startSample];
			const float tableValue = jlimit<float>(0.0f, 1.0f, tableModValue) * (float)(numTables -1);

			const int lowerTableIndex = (int)(tableValue);
			const int upperTableIndex = jmin(numTables-1, lowerTableIndex + 1);
			const float tableDelta = tableValue - (float)lowerTableIndex;
			jassert(0.0f <= tableDelta && tableDelta <= 1.0f);

			lowerTable = currentSound->getWaveTableData(lowerTableIndex);
			upperTable = currentSound->getWaveTableData(upperTableIndex);

			float tableGainValue = tableGainInterpolator.interpolateLinear(currentSound->getUnnormalizedGainValue(lowerTableIndex), currentSound->getUnnormalizedGainValue(upperTableIndex), tableDelta);

			tableGainValue *= getGainValue(tableModValue);

#if 0
			float u1 = upperTable[i1];
			float u2 = upperTable[i2];

			float l1 = lowerTable[i1];
			float l2 = lowerTable[i2];

			const float alpha = float(voiceUptime) - (float)index;
			//const float invAlpha = 1.0f - alpha;

			const float upperSample = tableGainInterpolator.interpolateLinear(u1, u2, alpha);

			const float lowerSample = tableGainInterpolator.interpolateLinear(l1, l2, alpha);

			float sample = lowerTableIndex != upperTableIndex ? tableGainInterpolator.interpolateLinear(lowerSample, upperSample, tableDelta) : lowerSample;
#else
            float u0 = upperTable[i0];
            float u1 = upperTable[i1];
            float u2 = upperTable[i2];
            float u3 = upperTable[i3];

            float l0 = lowerTable[i0];
            float l1 = lowerTable[i1];
            float l2 = lowerTable[i2];
            float l3 = lowerTable[i3];

            const float alpha = float(voiceUptime) - (float)index;
            
            float sample;
            
            if(lowerTableIndex != upperTableIndex)
            {
                float upperSample, lowerSample;
                
                if(hqMode)
                {
                    upperSample = interpolateCubic(u0, u1, u2, u3, alpha);
                    lowerSample = interpolateCubic(l0, l1, l2, l3, alpha);
                }
                else
                {
                    upperSample = tableGainInterpolator.interpolateLinear(u1, u2, alpha);
                    lowerSample = tableGainInterpolator.interpolateLinear(l1, l2, alpha);
                }
                
                sample = tableGainInterpolator.interpolateLinear(lowerSample, upperSample, tableDelta);
            }
            else
            {
                sample = hqMode ? interpolateCubic(u0, u1, u2, u3, alpha) :
                    tableGainInterpolator.interpolateLinear(u1, u2, alpha);
            }
#endif

			sample *= tableGainValue;

			sample *= 1.0f / currentSound->getUnnormalizedMaximum();

			// Stereo mode assumed
			voiceBuffer.setSample(0, startSample, sample);

			jassert(voicePitchValues == nullptr || voicePitchValues[startSample] > 0.0f);

			const double delta = (uptimeDelta * (voicePitchValues == nullptr ? 1.0 : voicePitchValues[startSample]));

			voiceUptime += delta;

			++startSample;
		}
	}
	else
	{
		const float tableModValue = static_cast<WavetableSynth*>(getOwnerSynth())->getConstantTableModValue();

		while (--numSamples >= 0)
		{
			int index = (int)voiceUptime;

			const auto i1 = index % (tableSize);

			auto i2 = i1 + 1;
            auto i0 = i1 - 1;
            auto i3 = i1 + 2;
            
            if (i1 == 0)         i0 = tableSize-1;
			if (i2 >= tableSize) i2 = 0;
            if (i3 >= tableSize) i3 = 0;

            auto numTables = currentSound->getWavetableAmount();
            
			const float tableValue = jlimit<float>(0.0f, 1.0f, tableModValue) * (float)(numTables - 1);

            
            
			const int lowerTableIndex = (int)(tableValue);
			const int upperTableIndex = jmin(numTables - 1, lowerTableIndex + 1);
			const float tableDelta = tableValue - (float)lowerTableIndex;
			jassert(0.0f <= tableDelta && tableDelta <= 1.0f);

            currentTableIndex = lowerTableIndex;
            
			lowerTable = currentSound->getWaveTableData(lowerTableIndex);
			upperTable = currentSound->getWaveTableData(upperTableIndex);

			float tableGainValue = tableGainInterpolator.interpolateLinear(currentSound->getUnnormalizedGainValue(lowerTableIndex), currentSound->getUnnormalizedGainValue(upperTableIndex), tableDelta);

			tableGainValue *= getGainValue(tableModValue);

            float u0 = upperTable[i0];
			float u1 = upperTable[i1];
			float u2 = upperTable[i2];
            float u3 = upperTable[i3];

            float l0 = lowerTable[i0];
			float l1 = lowerTable[i1];
			float l2 = lowerTable[i2];
            float l3 = lowerTable[i3];

			const float alpha = float(voiceUptime) - (float)index;
			
            float sample;
            
            if(lowerTableIndex != upperTableIndex)
            {
                float upperSample, lowerSample;
                
                if(hqMode)
                {
                    upperSample = interpolateCubic(u0, u1, u2, u3, alpha);
                    lowerSample = interpolateCubic(l0, l1, l2, l3, alpha);
                }
                else
                {
                    upperSample = tableGainInterpolator.interpolateLinear(u1, u2, alpha);
                    lowerSample = tableGainInterpolator.interpolateLinear(l1, l2, alpha);
                }
                
                sample = tableGainInterpolator.interpolateLinear(lowerSample, upperSample, tableDelta);
            }
            else
            {
                sample = hqMode ? interpolateCubic(u0, u1, u2, u3, alpha) :
                    tableGainInterpolator.interpolateLinear(u1, u2, alpha);
            }
            
			sample *= tableGainValue;

			sample *= 1.0f / currentSound->getUnnormalizedMaximum();

			// Stereo mode assumed
			voiceBuffer.setSample(0, startSample, sample);

			jassert(voicePitchValues == nullptr || voicePitchValues[startSample] > 0.0f);

			const double delta = (uptimeDelta * (voicePitchValues == nullptr ? 1.0 : voicePitchValues[startSample]));

			voiceUptime += delta;

			++startSample;
		}
	}

	if (auto modValues = getOwnerSynth()->getVoiceGainValues())
	{
		FloatVectorOperations::multiply(voiceBuffer.getWritePointer(0, startIndex), modValues + startIndex, samplesToCopy);
		FloatVectorOperations::copy(voiceBuffer.getWritePointer(1, startIndex), voiceBuffer.getReadPointer(0, startIndex), samplesToCopy);
	}
	else
	{
		const float constantGain = getOwnerSynth()->getConstantGainModValue();

		FloatVectorOperations::multiply(voiceBuffer.getWritePointer(0, startIndex), constantGain, samplesToCopy);
		FloatVectorOperations::copy(voiceBuffer.getWritePointer(1, startIndex), voiceBuffer.getReadPointer(0, startIndex), samplesToCopy);
	}

	getOwnerSynth()->effectChain->renderVoice(voiceIndex, voiceBuffer, startIndex, samplesToCopy);

	if (getOwnerSynth()->getLastStartedVoice() == this)
		static_cast<WavetableSynth*>(getOwnerSynth())->triggerWaveformUpdate();
}

const float * WavetableSynthVoice::getTableModulationValues()
{
	return dynamic_cast<WavetableSynth*>(getOwnerSynth())->getTableModValues();
}

int WavetableSynthVoice::getSmoothSize() const
{
	return wavetableSynth->getMorphSmoothing();
}



void WavetableSynthVoice::startNote(int midiNoteNumber, float /*velocity*/, SynthesiserSound* s, int /*currentPitchWheelPosition*/)
{
    currentSound = nullptr;
    
	ModulatorSynthVoice::startNote(midiNoteNumber, 0.0f, nullptr, -1);

	midiNoteNumber += getTransposeAmount();
    
    noteNumberAtStart = midiNoteNumber;
    
    startFrequency = MidiMessage::getMidiNoteInHertz(noteNumberAtStart);
    
    updateSoundFromPitchFactor(1.0, static_cast<WavetableSound*>(s));
    
    static_cast<WavetableSynth*>(getOwnerSynth())->lastGainIndex = -1;
}

WavetableSound::WavetableSound(const ValueTree &wavetableData)
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

    if(wavetableData.hasProperty(SampleIds::LoKey))
    {
        auto l = (int)wavetableData[SampleIds::LoKey];
        auto h = (int)wavetableData[SampleIds::HiKey];
        
        midiNotes.setRange(l, h - l, true);
    }
    
	wavetableSize = numSamples / wavetableAmount;

	emptyBuffer = AudioSampleBuffer(1, wavetableSize);
	emptyBuffer.clear();

	unnormalizedMaximum = 0.0f;

	normalizeTables();

	pitchRatio = 1.0;
    
    auto lowDelta = MidiMessage::getMidiNoteInHertz(midiNotes.findNextSetBit(0));
    auto highDelta = MidiMessage::getMidiNoteInHertz(midiNotes.getHighestBit());
                                
    
    frequencyRange = { lowDelta, highDelta };
}

const float * WavetableSound::getWaveTableData(int wavetableIndex) const
{
	if (wavetableIndex < wavetableAmount)
	{
		jassert((wavetableIndex + 1) * wavetableSize <= wavetables.getNumSamples());

		return wavetables.getReadPointer(0, wavetableIndex * wavetableSize);

	}
	else
	{
		return nullptr;
	}
}

void WavetableSound::calculatePitchRatio(double playBackSampleRate_)
{
    playbackSampleRate = playBackSampleRate_;
    
	const double idealCycleLength = playbackSampleRate / MidiMessage::getMidiNoteInHertz(noteNumber);

	pitchRatio = (double)wavetableSize / idealCycleLength;
}

void WavetableSound::normalizeTables()
{
	for (int i = 0; i < wavetableAmount; i++)
	{
		float *data = wavetables.getWritePointer(0, i * wavetableSize);
		const float peak = wavetables.getMagnitude(i * wavetableSize, wavetableSize);

		unnormalizedGainValues[i] = peak;

		if (peak == 0.0f) continue;

		if (peak > unnormalizedMaximum)
			unnormalizedMaximum = peak;

		FloatVectorOperations::multiply(data, 1.0f / peak, wavetableSize);
	}

	maximum = 1.0f;
}

} // namespace hise
