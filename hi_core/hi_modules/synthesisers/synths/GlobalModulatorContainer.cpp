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

struct GlobalModulatorContainer::GlobalModulatorCable
{
    WeakReference<Modulator> mod;
    var cable;
    
	

    void send(int voiceIndex, bool isEnvelope=false, int startSample=0)
    {
        if (auto c = cable.getObject())
        {
            auto cable = static_cast<scriptnode::routing::GlobalRoutingManager::Cable*>(c);
            
            double modValue = 1.0;
            
            if(voiceIndex == -1)
            {
                if(auto m = static_cast<TimeVariantModulator*>(mod.get()))
                {
                    modValue = m->getLastConstantValue();
                }
            }
			else if (isEnvelope)
			{
				auto gs = dynamic_cast<GlobalModulatorContainer*>(mod->getParentProcessor(true));

				if (auto data = gs->getEnvelopeValuesForModulator(mod, startSample, voiceIndex))
					modValue = *data;
			}
			else
            {
                if(auto m = static_cast<VoiceStartModulator*>(mod.get()))
                {
                    modValue = m->getVoiceStartValue(voiceIndex);
                }
            }
            
            cable->sendValue(nullptr, modValue);
        }
        
        
    }
    
    bool operator==(const GlobalModulatorCable& other) const
    {
        return other.mod == mod &&
               cable == other.cable;
    }
};

GlobalModulatorContainer::GlobalModulatorContainer(MainController *mc, const String &id, int numVoices) :
ModulatorSynth(mc, id, numVoices)
{
	finaliseModChains();

	gainChain = modChains[BasicChains::GainChain].getChain();
	gainChain->setMode(Modulation::Mode::GlobalMode, dontSendNotification);

	// Do not expand the values, but leave them compressed for the receivers to expand them...
	modChains[BasicChains::GainChain].setExpandToAudioRate(false);

	modChains[BasicChains::GainChain].setScratchBufferFunction([this](int voiceIndex, Modulator* m, float* data, int offset, int numSamples)
	{
		for (auto& ev : envelopeData)
		{
			if (ev.getModulator() == m)
			{
				ev.saveValues(voiceIndex, data, offset, numSamples);
			}
		}
	});

	for (int i = 0; i < numVoices; i++) addVoice(new GlobalModulatorContainerVoice(this));
	addSound(new GlobalModulatorContainerSound());

	disableChain(PitchModulation, true);
	disableChain(ModulatorSynth::EffectChain, true);

	gainChain->setColour(Colour(0xFF88A3A8));
	gainChain->getFactoryType()->setConstrainer(new NoGlobalsConstrainer());
	gainChain->setId("Global Modulators");

	auto f = [](float )
	{
		return "Not assigned";
	};

	gainChain->setTableValueConverter(f);

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

const float* GlobalModulatorContainer::getEnvelopeValuesForModulator(Processor* p, int startIndex, int voiceIndex)
{
	for (auto& tv : envelopeData)
	{
		if (tv.getModulator() == p)
			return tv.getReadPointer(voiceIndex, startIndex);
	}

	return nullptr;
}

float GlobalModulatorContainer::getVoiceStartValueFor(const Processor * /*voiceStartModulator*/)
{
	return 1.0f;
}

const float * GlobalModulatorContainer::getModulationValuesForModulator(Processor *p, int startIndex)
{
	for (auto& tv : timeVariantData)
	{
		if (tv.getModulator() == p)
			return tv.getReadPointer(startIndex);
	}

	return nullptr;
}

float GlobalModulatorContainer::getConstantVoiceValue(Processor *p, int noteNumber)
{
	for (auto& vd : voiceStartData)
	{
		if (vd.getModulator() == p)
			return vd.getConstantVoiceValue(noteNumber);
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

void GlobalModulatorContainer::sendVoiceStartCableValue(Modulator* m, const HiseEvent& e)
{
    if(!e.isNoteOn() || dynamic_cast<VoiceStartModulator*>(m) == nullptr)
        return;
        
    SimpleReadWriteLock::ScopedReadLock sl(cableLock);
    
    for(auto& vc: voiceStartCables)
    {
        if(vc.mod == m)
        {
            auto uv = static_cast<VoiceStartModulator*>(m)->getUnsavedValue();
            
            if (auto c = vc.cable.getObject())
            {
                auto cable = static_cast<scriptnode::routing::GlobalRoutingManager::Cable*>(c);
                cable->sendValue(nullptr, uv);
                break;
            }
        }
    }
}

void GlobalModulatorContainer::preStartVoice(int voiceIndex, const HiseEvent& e)
{
	ModulatorSynth::preStartVoice(voiceIndex, e);

	for (auto& vd : voiceStartData)
	{
		vd.saveValue(e.getNoteNumber(), voiceIndex);
	}
}

void GlobalModulatorContainer::connectToGlobalCable(Modulator* childMod, var cable, bool add)
{
    GlobalModulatorCable c;
    
    c.cable = cable;
    c.mod = childMod;
    
    hise::SimpleReadWriteLock::ScopedWriteLock sl(cableLock);
    
    if(!add)
    {
        timeVariantCables.removeAllInstancesOf(c);
        voiceStartCables.removeAllInstancesOf(c);
		envelopeCables.removeAllInstancesOf(c);
    }
    else
    {
		if (auto vc = dynamic_cast<VoiceStartModulator*>(childMod))
			voiceStartCables.addIfNotAlreadyThere(c);
		else if (auto e = dynamic_cast<EnvelopeModulator*>(childMod))
			envelopeCables.addIfNotAlreadyThere(c);
		else
            timeVariantCables.addIfNotAlreadyThere(c);
    }
    
    if(voiceStartCables.isEmpty())
    {
        gainChain->setPostEventFunction({});
    }
    else
    {
        gainChain->setPostEventFunction(BIND_MEMBER_FUNCTION_2(GlobalModulatorContainer::sendVoiceStartCableValue));
    }
}

bool GlobalModulatorContainer::shouldReset(int voiceIndex)
{
	for (auto& e : envelopeData)
	{
		if (static_cast<EnvelopeModulator*>(e.getModulator())->isPlaying(voiceIndex))
			return false;
	}

	return true;
}

void GlobalModulatorContainer::renderEnvelopeData(int voiceIndex, int startSample, int numSamples)
{
	
}

void GlobalModulatorContainer::preVoiceRendering(int startSample, int numThisTime)
{
	int startSample_cr = startSample / HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR;
	int numSamples_cr = numThisTime / HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR;
	
	auto scratchBuffer = modChains[GainChain].getScratchBuffer();

	for (auto& tv : timeVariantData)
	{
		if (auto mod = tv.getModulator())
		{
			if (!mod->isBypassed())
			{
				auto modBuffer = tv.initialiseBuffer(startSample_cr, numSamples_cr);

				mod->setScratchBuffer(scratchBuffer, startSample_cr + numSamples_cr);
				mod->render(modBuffer, scratchBuffer, startSample_cr, numSamples_cr);
                
                
			}
			else
			{
				tv.clear();
			}
		}
	}
    
    SimpleReadWriteLock::ScopedReadLock sl(cableLock);
    
    for(auto& c: timeVariantCables)
    {
        c.send(-1);
    }
}

void GlobalModulatorContainer::prepareToPlay(double newSampleRate, int samplesPerBlock)
{
	ModulatorSynth::prepareToPlay(newSampleRate, samplesPerBlock);

	for (auto& d : timeVariantData)
		d.prepareToPlay(samplesPerBlock);

	for (auto& d : envelopeData)
		d.prepareToPlay(samplesPerBlock);

	for (int i = 0; i < data.size(); i++)
	{
		data[i]->prepareToPlay(newSampleRate, samplesPerBlock);
	}
}

void GlobalModulatorContainer::addModulatorControlledParameter(const Processor* modulationSource, Processor* processor, int parameterIndex, NormalisableRange<double> range, int /*macroIndex*/)
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

	voiceStartData.clearQuick();
	
	auto handler_ = dynamic_cast<ModulatorChain::ModulatorChainHandler*>(gainChain->getHandler());

	for (auto& mod : handler_->activeVoiceStartList)
	{
        mod->setResetUnsavedValue(true);
		voiceStartData.add(VoiceStartData(mod));
	}

	timeVariantData.clearQuick();

	for (auto& mod : handler_->activeTimeVariantsList)
	{
		timeVariantData.add(TimeVariantData(mod, getLargestBlockSize()));
		//mod->deactivateIntensitySmoothing();
	}

	envelopeData.clearQuick();

	for (auto& mod : handler_->activeEnvelopesList)
	{
		envelopeData.add(EnvelopeData(mod, getLargestBlockSize()));
	}
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

	auto gs = static_cast<GlobalModulatorContainer*>(getOwnerSynth());

	for (auto& e : gs->envelopeCables)
	{
		e.send(getVoiceIndex(), true, startSample);
	}
		

#if 0
	if(gs->shouldReset(getVoiceIndex()))
		resetVoice();
#endif
}

void GlobalModulatorContainerVoice::checkRelease()
{
	auto gc = static_cast<GlobalModulatorContainer*>(getOwnerSynth());

	ModulatorChain *g = static_cast<ModulatorChain*>(gc->getChildProcessor(ModulatorSynth::GainModulation));

	if (killThisVoice && (killFadeLevel < 0.001f))
	{
		resetVoice();
		return;
	}

	if (g->hasActivePolyEnvelopes())
	{
		for (auto& e : gc->envelopeData)
		{
			if (e.getModulator()->isPlaying(getVoiceIndex()))
				return;
		}
	}

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
				auto value = (float)pc->parameterRange.convertFrom0to1(normalizedValue);

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
				auto value = (float)pc->parameterRange.convertFrom0to1(maxValue);

				processor->setAttribute(pc->attribute, value, sendNotification);
			}
		}
	}
}

} // namespace hise
