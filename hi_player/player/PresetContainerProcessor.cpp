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
*   along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
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

