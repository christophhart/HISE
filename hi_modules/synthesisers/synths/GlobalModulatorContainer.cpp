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
GlobalModulatorContainer::GlobalModulatorContainer(MainController *mc, const String &id, int numVoices) :
ModulatorSynth(mc, id, numVoices)
{
	for (int i = 0; i < numVoices; i++) addVoice(new GlobalModulatorContainerVoice(this));
	addSound(new GlobalModulatorContainerSound());

	disableChain(PitchModulation, true);
	disableChain(ModulatorSynth::EffectChain, true);

	gainChain->setColour(Colour(0xFF88A3A8));
	gainChain->getFactoryType()->setConstrainer(new NoGlobalsConstrainer());
	gainChain->setId("Global Modulators");

	gainChain->getHandler()->addChangeListener(this);

	
}

void GlobalModulatorContainer::restoreFromValueTree(const ValueTree &v)
{
	ModulatorSynth::restoreFromValueTree(v);

	refreshList();
}

float GlobalModulatorContainer::getVoiceStartValueFor(const Processor * /*voiceStartModulator*/)
{
	return 1.0f;
}

const float * GlobalModulatorContainer::getModulationValuesForModulator(Processor *p, int startIndex, int voiceIndex /*= 0*/)
{
	for (int i = 0; i < data.size(); i++)
	{
		if (data[i]->getProcessor() == p)
		{
			return data[i]->getModulationValues(startIndex, voiceIndex);
		}
	}

	jassertfalse;

	return nullptr;
}

float GlobalModulatorContainer::getConstantVoiceValue(Processor *p, int noteNumber)
{
	for (int i = 0; i < data.size(); i++)
	{
		if (data[i]->getProcessor() == p)
		{
			return data[i]->getConstantVoiceValue(noteNumber);
		}
	}

	jassertfalse;

	return 1.0f;
}

ProcessorEditorBody* GlobalModulatorContainer::createEditor(ProcessorEditor *parentEditor)
{

#if USE_BACKEND
	return new EmptyProcessorEditorBody(parentEditor);
#else 

	ignoreUnused(parentEditor);
	jassertfalse;
	return nullptr;
#endif
}

void GlobalModulatorContainer::addChangeListenerToHandler(SafeChangeListener *listener)
{
	gainChain->getHandler()->addChangeListener(listener);
}

void GlobalModulatorContainer::removeChangeListenerFromHandler(SafeChangeListener *listener)
{
	gainChain->getHandler()->removeChangeListener(listener);
}

void GlobalModulatorContainer::preStartVoice(int voiceIndex, int noteNumber)
{
	ModulatorSynth::preStartVoice(voiceIndex, noteNumber);

	for (int i = 0; i < data.size(); i++)
	{
		if (data[i]->getType() == GlobalModulator::VoiceStart)
		{
			data[i]->saveValuesToBuffer(0, 0, voiceIndex, noteNumber);
		}
	}
}

void GlobalModulatorContainer::postVoiceRendering(int startSample, int numThisTime)
{
	gainChain->renderNextBlock(gainBuffer, startSample, numThisTime);

	for (int i = 0; i < data.size(); i++)
	{
		if (data[i]->getType() == GlobalModulator::TimeVariant)
		{
			data[i]->saveValuesToBuffer(startSample, numThisTime, 0);
		}
	}
}

void GlobalModulatorContainer::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	ModulatorSynth::prepareToPlay(sampleRate, samplesPerBlock);

	for (int i = 0; i < data.size(); i++)
	{
		data[i]->prepareToPlay(sampleRate, samplesPerBlock);
	}
}

void GlobalModulatorContainer::refreshList()
{
	// Delete all old datas

	for (int i = 0; i < data.size(); i++)
	{
		if (data[i]->getProcessor() == nullptr)
		{
			data.remove(i--);
		}
	}

	for (int i = 0; i < gainChain->getHandler()->getNumProcessors(); i++)
	{
		if (i >= data.size() || gainChain->getHandler()->getProcessor(i) != data[i]->getProcessor())
		{
			data.insert(i, new GlobalModulatorData(gainChain->getHandler()->getProcessor(i)));
		}
	}

	jassert(data.size() == gainChain->getHandler()->getNumProcessors());
}

void GlobalModulatorContainerVoice::startNote(int midiNoteNumber, float /*velocity*/, SynthesiserSound*, int /*currentPitchWheelPosition*/)
{
	ModulatorSynthVoice::startNote(midiNoteNumber, 0.0f, nullptr, -1);

	voiceUptime = 0.0;

	uptimeDelta = 1.0;
}

void GlobalModulatorContainerVoice::calculateBlock(int startSample, int numSamples)
{
	
	FloatVectorOperations::fill(voiceBuffer.getWritePointer(0, startSample), 0.0f, numSamples);
	FloatVectorOperations::fill(voiceBuffer.getWritePointer(1, startSample), 0.0f, numSamples);
}

GlobalModulatorData::GlobalModulatorData(Processor *modulator_):
modulator(modulator_)
{
	if (getEnvelopeModulator() != nullptr)
	{
		type = GlobalModulator::Envelope;
		constantVoiceValues = Array<float>();

		numVoices = getEnvelopeModulator()->polyManager.getVoiceAmount();

	}
	else if (getTimeVariantModulator() != nullptr)
	{
		type = GlobalModulator::TimeVariant;
		numVoices = 1;
		constantVoiceValues = Array<float>();
	}
	else if (getVoiceStartModulator() != nullptr)
	{
		type = GlobalModulator::VoiceStart;
		constantVoiceValues = Array<float>();
	
		numVoices = getVoiceStartModulator()->polyManager.getVoiceAmount();

		constantVoiceValues.insertMultiple(0, 1.0f, 128);

	}
	else
	{
		jassertfalse;
	}

	if (modulator->getSampleRate() > 0) prepareToPlay(modulator->getSampleRate(), modulator->getBlockSize());

}

void GlobalModulatorData::prepareToPlay(double /*sampleRate*/, int blockSize)
{
	switch (type)
	{
	case GlobalModulator::VoiceStart:	valuesForCurrentBuffer = AudioSampleBuffer(); return;
	case GlobalModulator::TimeVariant:	valuesForCurrentBuffer = AudioSampleBuffer(1, blockSize);
    case GlobalModulator::numTypes: break;
	}
}

void GlobalModulatorData::saveValuesToBuffer(int startIndex, int numSamples, int voiceIndex /*= 0*/, int noteNumber/*=-1*/ )
{
	if (modulator.get() == nullptr) return;

	switch (type)
	{
	case GlobalModulator::VoiceStart:	jassert(noteNumber != -1);  constantVoiceValues.set(noteNumber, static_cast<VoiceStartModulator*>(modulator.get())->getVoiceStartValue(voiceIndex)); break;
	case GlobalModulator::TimeVariant:	FloatVectorOperations::copy(valuesForCurrentBuffer.getWritePointer(0, startIndex), static_cast<TimeVariantModulator*>(modulator.get())->getCalculatedValues(0) + startIndex, numSamples); break;
    case GlobalModulator::numTypes: break;
	}
}

const float * GlobalModulatorData::getModulationValues(int startIndex, int /*voiceIndex*/)
{
	switch (type)
	{
	case GlobalModulator::VoiceStart:	jassertfalse; return nullptr;
	case GlobalModulator::TimeVariant:	return valuesForCurrentBuffer.getReadPointer(0, startIndex);
    case GlobalModulator::numTypes: return nullptr;
	}

	return nullptr;
}

float GlobalModulatorData::getConstantVoiceValue(int noteNumber)
{
	return constantVoiceValues[noteNumber];
}
