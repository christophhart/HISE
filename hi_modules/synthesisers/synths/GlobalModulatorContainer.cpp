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

namespace hise { using namespace juce;

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

	gainChain->getHandler()->addListener(this);
}

GlobalModulatorContainer::~GlobalModulatorContainer()
{
	gainChain->getHandler()->removeListener(this);

	data.clear();
	allParameters.clear();
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

void GlobalModulatorContainer::preStartVoice(int voiceIndex, int noteNumber)
{
	ModulatorSynth::preStartVoice(voiceIndex, noteNumber);

	for (int i = 0; i < data.size(); i++)
	{
		if (data[i]->getType() == GlobalModulator::VoiceStart)
		{
			data[i]->saveValuesToBuffer(0, 0, voiceIndex, noteNumber);
			data[i]->handleVoiceStartControlledParameters(noteNumber);
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
			data[i]->handleTimeVariantControlledParameters(startSample, numThisTime);
		}
	}
}

void GlobalModulatorContainer::prepareToPlay(double newSampleRate, int samplesPerBlock)
{
	ModulatorSynth::prepareToPlay(newSampleRate, samplesPerBlock);

	for (int i = 0; i < data.size(); i++)
	{
		data[i]->prepareToPlay(newSampleRate, samplesPerBlock);
	}
}

void GlobalModulatorContainer::addModulatorControlledParameter(const Processor* modulationSource, Processor* processor, int parameterIndex, NormalisableRange<double> range, int macroIndex)
{
	for (auto d : data)
	{
		if (d->getProcessor() == modulationSource)
		{
			auto newConnection = d->addConnectedParameter(processor, parameterIndex, range);

			allParameters.add(newConnection);

			return;
		}
	}
}

void GlobalModulatorContainer::removeModulatorControlledParameter(const Processor* modulationSource, Processor* processor, int parameterIndex)
{
	for(auto pc: allParameters)
		

	for (auto d : data)
	{
		if (d->getProcessor() == modulationSource)
		{
			d->removeConnectedParameter(processor, parameterIndex);
			return;
		}
	}
}

bool GlobalModulatorContainer::isModulatorControlledParameter(Processor* processor, int parameterIndex) const
{
	return getModulatorForControlledParameter(processor, parameterIndex) != nullptr;
}

const hise::Processor* GlobalModulatorContainer::getModulatorForControlledParameter(const Processor* processor, int parameterIndex) const
{
	for (auto d : data)
	{
		if (auto pc = d->getParameterConnection(processor, parameterIndex))
			return d->getProcessor();
	}

	return nullptr;
}

juce::ValueTree GlobalModulatorContainer::exportModulatedParameters() const
{
	ValueTree v("ModulatedParameters");

	for (auto d : data)
	{
		auto tree = d->exportAllConnectedParameters();

		if (tree.isValid())
			v.addChild(tree, -1, nullptr);
	}

	return v;
}

void GlobalModulatorContainer::restoreModulatedParameters(const ValueTree& v)
{
	for (const auto& c : v)
	{
		auto pId = c.getProperty("id");

		for (auto d : data)
		{
			if (pId == d->getProcessor()->getId())
			{
				d->restoreParameterConnections(c);
			}
		}
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

	resetVoice();
}

GlobalModulatorData::GlobalModulatorData(Processor *modulator_):
modulator(modulator_),
valuesForCurrentBuffer(1, 0)
{
	

	if (getTimeVariantModulator() != nullptr)
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

	if (modulator->getSampleRate() > 0) prepareToPlay(modulator->getSampleRate(), modulator->getLargestBlockSize());

}

void GlobalModulatorData::prepareToPlay(double /*sampleRate*/, int blockSize)
{
	switch (type)
	{
    case GlobalModulator::VoiceStart:	valuesForCurrentBuffer.setSize(1, 0); return;
	case GlobalModulator::TimeVariant:	ProcessorHelpers::increaseBufferIfNeeded(valuesForCurrentBuffer, blockSize); break;
    case GlobalModulator::numTypes: break;
    default: break;
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
    default: break;
	}
}

const float * GlobalModulatorData::getModulationValues(int startIndex, int /*voiceIndex*/) const
{
	switch (type)
	{
	case GlobalModulator::VoiceStart:	jassertfalse; return nullptr;
	case GlobalModulator::TimeVariant:	return valuesForCurrentBuffer.getReadPointer(0, startIndex);
    case GlobalModulator::numTypes: return nullptr;
    default: break;
	}

	return nullptr;
}

float GlobalModulatorData::getConstantVoiceValue(int noteNumber)
{
	return constantVoiceValues[noteNumber];
}

void GlobalModulatorData::handleVoiceStartControlledParameters(int noteNumber)
{
	if (connectedParameters.size() != 0)
	{
		auto normalizedValue = getConstantVoiceValue(noteNumber);

		for (auto pc : connectedParameters)
		{
			if (auto processor = pc->processor)
			{
				auto value = pc->parameterRange.convertFrom0to1(normalizedValue);

				if (pc->lastValue != value)
				{
					processor->setAttribute(pc->attribute, value, sendNotification);
					pc->lastValue = value;
				}
			}
		}
	}

	


}

void GlobalModulatorData::handleTimeVariantControlledParameters(int startSample, int numThisTime) const
{
	if (connectedParameters.size() > 0)
	{
		auto modData = getModulationValues(startSample);

		auto maxValue = FloatVectorOperations::findMaximum(modData, numThisTime);

		for (auto pc : connectedParameters)
		{
			if (auto processor = pc->processor)
			{
				auto value = pc->parameterRange.convertFrom0to1(maxValue);

				processor->setAttribute(pc->attribute, value, sendNotification);
			}
		}
	}
}

} // namespace hise
