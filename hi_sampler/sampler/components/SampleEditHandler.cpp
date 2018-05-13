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

void SampleEditHandler::moveSamples(SamplerSoundMap::Neighbour direction)
{
	ModulatorSampler *s = sampler;

	s->getUndoManager()->beginNewTransaction("Moving Samples");

	switch (direction)
	{
	case SamplerSoundMap::Right:
	case SamplerSoundMap::Left:
	{
		for (int i = 0; i < selectedSamplerSounds.getNumSelected(); i++)
		{
			ModulatorSamplerSound *sound = selectedSamplerSounds.getSelectedItem(i);

			if (direction == SamplerSoundMap::Right)
			{
				changeProperty(sound, ModulatorSamplerSound::KeyHigh, 1);
				changeProperty(sound, ModulatorSamplerSound::KeyLow, 1);
				changeProperty(sound, ModulatorSamplerSound::RootNote, 1);
			}
			else
			{
				changeProperty(sound, ModulatorSamplerSound::KeyLow, -1);
				changeProperty(sound, ModulatorSamplerSound::KeyHigh, -1);
				changeProperty(sound, ModulatorSamplerSound::RootNote, -1);
			}
		}
		break;
	}
	case SamplerSoundMap::Up:
	case SamplerSoundMap::Down:
	{
		for (int i = 0; i < selectedSamplerSounds.getNumSelected(); i++)
		{
			ModulatorSamplerSound *sound = selectedSamplerSounds.getSelectedItem(i);

			changeProperty(sound, ModulatorSamplerSound::VeloHigh, direction == SamplerSoundMap::Up ? 1 : -1);
			changeProperty(sound, ModulatorSamplerSound::VeloLow, direction == SamplerSoundMap::Up ? 1 : -1);
		}
		break;
	}
	}
}

void SampleEditHandler::handleMidiSelection()
{
	auto& x = sampler->getSamplerDisplayValues();

	if (sampler->getEditorState(ModulatorSampler::MidiSelectActive) && newKeysPressed(x.currentNotes))
	{
		selectedSamplerSounds.deselectAll();

		SelectedItemSet<const ModulatorSamplerSound*> midiSounds;

		for (int i = 0; i < 127; i++)
		{
			if (x.currentNotes[i] != 0)
			{
				const int noteNumber = i;
				const int velocity = x.currentNotes[i];

				ModulatorSampler::SoundIterator sIter(sampler);

				while (auto sound = sIter.getNextSound())
				{
					if (sampler->soundCanBePlayed(sound, 1, noteNumber, (float)velocity / 127.0f))
					{
						selectedSamplerSounds.addToSelection(sound.get());
					}
				}

			}
		}
	}
}

SampleSelection SampleEditHandler::getSanitizedSelection()
{
	auto sounds = selectedSamplerSounds.getItemArray();

	SampleSelection existingSounds;

	existingSounds.ensureStorageAllocated(sounds.size());

	for (int i = 0; i < sounds.size(); i++)
	{
		if (sounds[i].get() != nullptr) existingSounds.add(sounds[i].get());
	}

	return existingSounds;
}

bool SampleEditHandler::newKeysPressed(const uint8 *currentNotes)
{
	for (int i = 0; i < 127; i++)
	{
		if (currentNotes[i] != 0) return true;
	}
	return false;
}

void SampleEditHandler::changeProperty(ModulatorSamplerSound *s, ModulatorSamplerSound::Property p, int delta)
{
	const int v = s->getProperty(p);

	s->setPropertyWithUndo(p, v + delta);
}

juce::File SampleEditHandler::getCurrentSampleMapDirectory() const
{
	return sampler->getMainController()->getCurrentFileHandler().getSubDirectory(ProjectHandler::SubDirectories::SampleMaps);
}

void SampleEditHandler::SampleEditingActions::createMultimicSampleMap(SampleEditHandler* handler)
{
	const String multimicTokens = PresetHandler::getCustomName("Multimic Tokens", "Enter a semicolon separated list of all mic position tokens starting with the existing mic position");

	auto list = StringArray::fromTokens(multimicTokens, ";", "\"");

	if (list.size() == 0)
		return;

	auto firstToken = list[0];

	String listString = "\n";

	for (auto l : list)
		listString << l << "\n";

	if (PresetHandler::showYesNoWindow("Confirm multimic tokens", "You have specified these tokens:" + listString + "\nPress OK to create a multimic samplemap with these mic positions"))
	{
		ValueTree v = handler->getSampler()->getSampleMap()->exportAsValueTree();

		for (int i = 0; i < v.getNumChildren(); i++)
		{
			auto sample = v.getChild(i);

			if (sample.getNumChildren() != 0)
			{
				PresetHandler::showMessageWindow("Already a multimic samplemap", "The samplemap has already multimics", PresetHandler::IconType::Error);
				return;
			}

			auto filename = sample.getProperty("FileName").toString();

			if (!filename.contains(firstToken))
			{
				PresetHandler::showMessageWindow("Wrong first mic position", "You have to specify the current mic position as first mic position.\nSample: " + filename, PresetHandler::IconType::Error);
				return;
			}

			sample.removeProperty("FileName", nullptr);

			for (auto token : list)
			{
				auto fChild = ValueTree("file");
				fChild.setProperty("FileName", filename.replace(firstToken, token, false), nullptr);
				sample.addChild(fChild, -1, nullptr);
			}
		}

		PresetHandler::showMessageWindow("Merge successful", "Press OK to choose a location for the multimic sample map");

		auto sampleMapDirectory = handler->getCurrentSampleMapDirectory();

		FileChooser fc("Save multimic Samplemap", sampleMapDirectory, "*.xml", true);

		v.setProperty("MicPositions", multimicTokens, nullptr);

		if (fc.browseForFileToSave(true))
		{
			auto f = fc.getResult();

			auto path = f.getRelativePathFrom(sampleMapDirectory).upToFirstOccurrenceOf(".xml", false, false).replace("\\", "/");

			v.setProperty("ID", path, nullptr);

			ScopedPointer<XmlElement> xml = v.createXml();
			f.replaceWithText(xml->createDocument(""));
		}
	}
}
} // namespace hise
