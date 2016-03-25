
void PresetContainerProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	for (int i = 0; i < patches.size(); i++)
	{
		patches[i]->prepareToPlay(sampleRate, samplesPerBlock);
	}
}

void PresetContainerProcessor::processBlock(AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
	buffer.clear();

	keyboardState.processNextMidiBuffer(midiMessages, 0, buffer.getNumSamples(), true);

	for (int i = 0; i < patches.size(); i++)
	{
		patches[i]->processBlock(buffer, midiMessages);
	}

	midiMessages.clear();
}

void PresetContainerProcessor::getStateInformation(MemoryBlock &destData)
{
	MemoryOutputStream output(destData, false);

	ValueTree v("Presets");

	for (int i = 0; i < patches.size(); i++)
	{
		ValueTree preset("Preset");
		preset.setProperty("FileName", loadedPresets[i], nullptr);

		ValueTree data = patches[i]->exportAsValueTree();

		preset.addChild(data, -1, nullptr);

		v.addChild(preset, -1, nullptr);

	}

	

	v.writeToStream(output);
}

void PresetContainerProcessor::setStateInformation(const void *data, int sizeInBytes)
{
	ValueTree v = ValueTree::readFromData(data, sizeInBytes);

	if (v.isValid())
	{
		ScopedLock sl(getCallbackLock());

		patches.clear();
	}
	else return;


	for (int i = 0; i < v.getNumChildren(); i++)
	{
		File fileName = File(v.getChild(i).getProperty("FileName").toString());
		ValueTree data = v.getChild(i).getChildWithName("ControlData");

		addNewPreset(fileName, &data);
	}
	
}

AudioProcessorEditor* PresetContainerProcessor::createEditor()
{
	return new PresetContainerProcessorEditor(this);
}

void PresetContainerProcessor::addNewPreset(const File &f, ValueTree *data)
{
	jassert(patches.size() == loadedPresets.size());

	PresetProcessor *pp = new PresetProcessor(f);

	if (data != nullptr)
	{
		pp->restoreFromValueTree(*data);
	}

	ScopedLock sl(getCallbackLock());

	if(getSampleRate() > 0) pp->prepareToPlay(getSampleRate(), getBlockSize());

	loadedPresets.add(f.getFullPathName());
	patches.add(pp);
}

void PresetContainerProcessor::removePreset(PresetProcessor * pp)
{
	ScopedLock sl(getCallbackLock());

	jassert(patches.size() == loadedPresets.size());

	for (int i = 0; i < patches.size(); i++)
	{
		if (patches[i] == pp)
		{
			patches.remove(i);
			loadedPresets.remove(i);
		}
	}
}

