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


AudioProcessorWrapper::AudioProcessorWrapper(MainController *mc, const String &uid):
MasterEffectProcessor(mc, uid),
wetAmountChain(new ModulatorChain(mc, "Wet Amount", 1, Modulation::GainMode, this)),
loadedProcessorId(Identifier("unused"))
{

}



AudioProcessorWrapper::~AudioProcessorWrapper()
{
	for (int i = 0; i < connectedEditors.size(); i++)
	{
		if (connectedEditors[i].getComponent() != nullptr)
		{
			dynamic_cast<AudioProcessorEditorWrapper::Content*>(connectedEditors[i].getComponent())->setUnconnected();
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
		MemoryBlock data;

		data.fromBase64Encoding(v.getProperty("AudioProcessorData", "").toString());

		wrappedAudioProcessor->setStateInformation(data.getData(), data.getSize());
	}
}

ValueTree AudioProcessorWrapper::exportAsValueTree() const
{
	ValueTree v = MasterEffectProcessor::exportAsValueTree();

	v.setProperty("AudioProcessorId", loadedProcessorId.toString(), nullptr);

	if (wrappedAudioProcessor != nullptr)
	{
		MemoryBlock data;

		wrappedAudioProcessor->getStateInformation(data);

		const String dataAsString = data.toBase64Encoding();

		v.setProperty("AudioProcessorData", dataAsString, nullptr);
	}

	return v;
}

ProcessorEditorBody * AudioProcessorWrapper::createEditor(ProcessorEditor *parentEditor)
{
	return new AudioProcessorEditorWrapper(parentEditor);
}

void AudioProcessorWrapper::applyEffect(AudioSampleBuffer &buffer, int startSample, int numSamples)
{
	jassert(startSample == 0);

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

void AudioProcessorWrapper::setAudioProcessor(const Identifier id)
{
	ScopedLock sl(wrapperLock);

	for (int i = 0; i < numRegisteredProcessors; i++)
	{
		if (!id.isNull() && registeredAudioProcessors[i].id == id)
		{
			wrappedAudioProcessor = nullptr;

			createAudioProcessorFunction *createProcessor = registeredAudioProcessors[i].function;

			wrappedAudioProcessor = (*createProcessor)();

			if (wrappedAudioProcessor != nullptr)
			{
				if (getSampleRate() > 0.0)
				{
					wrappedAudioProcessor->prepareToPlay(getSampleRate(), getBlockSize());
				}

				loadedProcessorId = id;

				for (int i = 0; i < connectedEditors.size(); i++)
				{
					if (connectedEditors[i].getComponent() != nullptr)
					{
						dynamic_cast<AudioProcessorEditorWrapper::Content*>(connectedEditors[i].getComponent())->setAudioProcessor(wrappedAudioProcessor);
					}
					else
					{
						connectedEditors.remove(i);
						i--;
					}
				}
			}
		}
	}
}

AudioProcessorWrapper::ListEntry AudioProcessorWrapper::registeredAudioProcessors[1024];
int AudioProcessorWrapper::numRegisteredProcessors = 0;

Array<Identifier> ScriptingDsp::Factory::moduleIds;

// this needs at least c++11
Array<Spatializer::SpeakerLayout> Spatializer::speakerPositions =
{
	Spatializer::SpeakerLayout{ AudioChannelSet::stereo(), { SpeakerPosition{ 1.0f, -0.25f * float_Pi }, SpeakerPosition{ 1.0f, 0.25f * float_Pi } } },
	Spatializer::SpeakerLayout{ AudioChannelSet::quadraphonic(), { SpeakerPosition{ 1.0f, -0.25f * float_Pi }, SpeakerPosition{ 1.0f, 0.25f * float_Pi }, SpeakerPosition{ 1.0f, -0.75f * float_Pi }, SpeakerPosition{ 1.0f, 0.75f * float_Pi } } },
	Spatializer::SpeakerLayout{ AudioChannelSet::create5point0(), { SpeakerPosition{ 1.0f, 0.0f }, SpeakerPosition{ 1.0f, -0.25f * float_Pi }, SpeakerPosition{ 1.0f, 0.25f * float_Pi }, SpeakerPosition{ 1.0f, -0.75f * float_Pi }, SpeakerPosition{ 1.0f, 0.75f * float_Pi } } }
};

AudioProcessorEditor* ScriptingAudioProcessor::createEditor()
{
	return new ScriptingAudioProcessorEditor(this);
}
