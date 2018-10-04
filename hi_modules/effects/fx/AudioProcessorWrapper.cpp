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

AudioProcessorWrapper::AudioProcessorWrapper(MainController *mc, const String &uid):
MasterEffectProcessor(mc, uid),
wetAmountChain(new ModulatorChain(mc, "Wet Amount", 1, Modulation::GainMode, this)),
wetAmountBuffer(1, 0),
loadedProcessorId(Identifier("unused"))
{
    
}



AudioProcessorWrapper::~AudioProcessorWrapper()
{
	for (int i = 0; i < connectedEditors.size(); i++)
	{
		if (connectedEditors[i].getComponent() != nullptr)
		{
			dynamic_cast<WrappedAudioProcessorEditorContent*>(connectedEditors[i].getComponent())->setUnconnected();
		}
	}

	wrappedAudioProcessor = nullptr;
}

void AudioProcessorWrapper::setInternalAttribute(int parameterIndex, float newValue)
{
	ScopedLock sl(wrapperLock);

	if (wrappedAudioProcessor.get() != nullptr)
	{
		wrappedAudioProcessor->setParameter(parameterIndex, newValue);
	}
}

float AudioProcessorWrapper::getAttribute(int parameterIndex) const
{
	ScopedLock sl(wrapperLock);

	if (wrappedAudioProcessor.get() != nullptr)
	{
		return wrappedAudioProcessor->getParameter(parameterIndex);
	}
	else
	{
		return 0.0f;
	}
}

float AudioProcessorWrapper::getDefaultValue(int parameterIndex) const
{
	ScopedLock sl(wrapperLock);

	if (wrappedAudioProcessor.get() != nullptr)
	{
		return wrappedAudioProcessor->getParameterDefaultValue(parameterIndex);
	}
	else
	{
		return 0.0f;
	}
}

void AudioProcessorWrapper::restoreFromValueTree(const ValueTree &v)
{
	MasterEffectProcessor::restoreFromValueTree(v);

	const Identifier processorId = Identifier(v.getProperty("AudioProcessorId", "unused").toString());

	setAudioProcessor(processorId);

	if (wrappedAudioProcessor != nullptr)
	{
		MemoryBlock mb;

		mb.fromBase64Encoding(v.getProperty("AudioProcessorData", "").toString());

		wrappedAudioProcessor->setStateInformation(mb.getData(), (int)mb.getSize());
	}
}

ValueTree AudioProcessorWrapper::exportAsValueTree() const
{
	ValueTree v = MasterEffectProcessor::exportAsValueTree();

	v.setProperty("AudioProcessorId", loadedProcessorId.toString(), nullptr);

	if (wrappedAudioProcessor != nullptr)
	{
		MemoryBlock mb;

		wrappedAudioProcessor->getStateInformation(mb);

		const String dataAsString = mb.toBase64Encoding();

		v.setProperty("AudioProcessorData", dataAsString, nullptr);
	}

	return v;
}

ProcessorEditorBody * AudioProcessorWrapper::createEditor(ProcessorEditor *parentEditor)
{
#if USE_BACKEND
	return new AudioProcessorEditorWrapper(parentEditor);
#else
	ignoreUnused(parentEditor);
	jassertfalse;
	return nullptr;
#endif
}

void AudioProcessorWrapper::applyEffect(AudioSampleBuffer &buffer, int startSample, int numSamples)
{
	jassert(startSample == 0);

	ignoreUnused(startSample);

	ScopedLock sl(wrapperLock);

	MidiBuffer messages;

	if (wrappedAudioProcessor != nullptr)
	{
		if (!wetAmountChain->isBypassed() && wetAmountChain->getNumChildProcessors() != 0)
		{
			

			FloatVectorOperations::copy(tempBuffer.getWritePointer(0), buffer.getReadPointer(0), numSamples);
			FloatVectorOperations::copy(tempBuffer.getWritePointer(1), buffer.getReadPointer(1), numSamples);

			wrappedAudioProcessor->processBlock(tempBuffer, messages);

			float *modValues = wetAmountBuffer.getWritePointer(0);

			FloatVectorOperations::multiply(tempBuffer.getWritePointer(0), modValues, numSamples);
			FloatVectorOperations::multiply(tempBuffer.getWritePointer(1), modValues, numSamples);

			FloatVectorOperations::multiply(modValues, -1.0f, numSamples);
			FloatVectorOperations::add(modValues, 1.0f, numSamples);

			FloatVectorOperations::multiply(buffer.getWritePointer(0), modValues, numSamples);
			FloatVectorOperations::multiply(buffer.getWritePointer(1), modValues, numSamples);

			FloatVectorOperations::add(buffer.getWritePointer(0), tempBuffer.getReadPointer(0), numSamples);
			FloatVectorOperations::add(buffer.getWritePointer(1), tempBuffer.getReadPointer(1), numSamples);
		}
		else
		{
			wrappedAudioProcessor->processBlock(buffer, messages);
		}
	}
}

void AudioProcessorWrapper::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	MasterEffectProcessor::prepareToPlay(sampleRate, samplesPerBlock);

	ScopedLock sl(wrapperLock);

	if (wrappedAudioProcessor != nullptr)
	{
		wrappedAudioProcessor->prepareToPlay(sampleRate, samplesPerBlock);
	}

	tempBuffer = AudioSampleBuffer(2, samplesPerBlock);
}

void AudioProcessorWrapper::addAudioProcessorToList(const Identifier id, createAudioProcessorFunction *function)
{
	// Skip existing Ids...
	for (int i = 0; i < numRegisteredProcessors; i++)
	{
		if (registeredAudioProcessors[i].id == id) return;
	}

	registeredAudioProcessors[numRegisteredProcessors] = ListEntry(id, function);
	numRegisteredProcessors++;
}

void AudioProcessorWrapper::setAudioProcessor(const Identifier& processorId)
{
	ScopedLock sl(wrapperLock);

	for (int i = 0; i < numRegisteredProcessors; i++)
	{
		if (!processorId.isNull() && registeredAudioProcessors[i].id == processorId)
		{
			wrappedAudioProcessor = nullptr;

			createAudioProcessorFunction *createProcessor = registeredAudioProcessors[i].function;

			wrappedAudioProcessor = (*createProcessor)();

			if (wrappedAudioProcessor != nullptr)
			{
				if (getSampleRate() > 0.0)
				{
					wrappedAudioProcessor->prepareToPlay(getSampleRate(), getLargestBlockSize());
				}

				loadedProcessorId = processorId;

				for (int j = 0; j < connectedEditors.size(); j++)
				{
					if (connectedEditors[j].getComponent() != nullptr)
					{
						dynamic_cast<WrappedAudioProcessorEditorContent*>(connectedEditors[j].getComponent())->setAudioProcessor(wrappedAudioProcessor);
					}
					else
					{
						connectedEditors.remove(j);
						j--;
					}
				}
			}
		}
	}
}

AudioProcessorWrapper::ListEntry AudioProcessorWrapper::registeredAudioProcessors[1024];
int AudioProcessorWrapper::numRegisteredProcessors = 0;


#if 0
// this needs at least c++11
Array<Spatializer::SpeakerLayout> Spatializer::speakerPositions =
{
	Spatializer::SpeakerLayout{ AudioChannelSet::stereo(), { SpeakerPosition{ 1.0f, -0.25f * float_Pi }, SpeakerPosition{ 1.0f, 0.25f * float_Pi } } },
	Spatializer::SpeakerLayout{ AudioChannelSet::quadraphonic(), { SpeakerPosition{ 1.0f, -0.25f * float_Pi }, SpeakerPosition{ 1.0f, 0.25f * float_Pi }, SpeakerPosition{ 1.0f, -0.75f * float_Pi }, SpeakerPosition{ 1.0f, 0.75f * float_Pi } } },
	Spatializer::SpeakerLayout{ AudioChannelSet::create5point0(), { SpeakerPosition{ 1.0f, 0.0f }, SpeakerPosition{ 1.0f, -0.25f * float_Pi }, SpeakerPosition{ 1.0f, 0.25f * float_Pi }, SpeakerPosition{ 1.0f, -0.75f * float_Pi }, SpeakerPosition{ 1.0f, 0.75f * float_Pi } } }
};
#endif

} // namespace hise
