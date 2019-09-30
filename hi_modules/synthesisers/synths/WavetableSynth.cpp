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
	morphSmoothing(700)
{
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

void WavetableSynthVoice::calculateBlock(int startSample, int numSamples)
{
	const int startIndex = startSample;
	const int samplesToCopy = numSamples;

	const float *voicePitchValues = getOwnerSynth()->getPitchValuesForVoice();
		
	if (auto tableValues = getTableModulationValues())
	{
		while (--numSamples >= 0)
		{
			int index = (int)voiceUptime;

			const int i1 = index % (tableSize);

			int i2 = i1 + 1;

			auto numTables = currentSound->getWavetableAmount();

			if (i2 >= tableSize)
			{
				const float tableModValue = tableValues[startSample];
				currentTableIndex = roundToInt(tableModValue * (double)(numTables-1));
				i2 = 0;
			}
			
			const float tableModValue = tableValues[startSample];
			const float tableValue = jlimit<float>(0.0f, 1.0f, tableModValue) * (double)(numTables -1);

			const int lowerTableIndex = (int)(tableValue);
			const int upperTableIndex = jmin(numTables-1, lowerTableIndex + 1);
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

			float sample = lowerTableIndex != upperTableIndex ? tableGainInterpolator.interpolateLinear(lowerSample, upperSample, tableDelta) : lowerSample;

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

			const int i1 = index % (tableSize);

			int i2 = i1 + 1;

			auto numTables = currentSound->getWavetableAmount();

			if (i2 >= tableSize)
			{
				i2 = 0;
			}

			const float tableValue = jlimit<float>(0.0f, 1.0f, tableModValue) * (double)(numTables - 1);

			const int lowerTableIndex = (int)(tableValue);
			const int upperTableIndex = jmin(numTables - 1, lowerTableIndex + 1);
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

			float sample = lowerTableIndex != upperTableIndex ? tableGainInterpolator.interpolateLinear(lowerSample, upperSample, tableDelta) : lowerSample;

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

	static_cast<WavetableSynth*>(getOwnerSynth())->lastGainIndex = -1;

	tableSize = currentSound->getTableSize();
	smoothSize = tableSize;
	uptimeDelta = currentSound->getPitchRatio();
	uptimeDelta *= getOwnerSynth()->getMainController()->getGlobalPitchFactor();
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

	wavetableSize = numSamples / wavetableAmount;

	emptyBuffer = AudioSampleBuffer(1, wavetableSize);
	emptyBuffer.clear();

	unnormalizedMaximum = 0.0f;

	normalizeTables();

	pitchRatio = 1.0;
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

void WavetableSound::calculatePitchRatio(double playBackSampleRate)
{
	const double idealCycleLength = playBackSampleRate / MidiMessage::getMidiNoteInHertz(noteNumber);

	pitchRatio = 1.0;

	pitchRatio = (double)wavetableSize / idealCycleLength;

	//pitchRatio *= sampleRate / playBackSampleRate;
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
