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
	int lowestValue = 127;
	int highestValue = 0;

	for (auto sound : selectedSamplerSounds)
	{
		switch (direction)
		{
		case SamplerSoundMap::Right:
		case SamplerSoundMap::Left:
		{
			lowestValue = jmin<int>(lowestValue, sound->getSampleProperty(SampleIds::LoKey));
			highestValue = jmax<int>(highestValue, sound->getSampleProperty(SampleIds::HiKey));
			break;
		}
		case SamplerSoundMap::Up:
		case SamplerSoundMap::Down:
		{
			lowestValue = jmin<int>(lowestValue, sound->getSampleProperty(SampleIds::LoVel));
			highestValue = jmax<int>(highestValue, sound->getSampleProperty(SampleIds::HiVel));
			break;
		}
		}

	}

	if (lowestValue == 0 && (direction == SamplerSoundMap::Left || direction == SamplerSoundMap::Down))
		return;

	if (highestValue == 127 && (direction == SamplerSoundMap::Right || direction == SamplerSoundMap::Up))
		return;

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
				changeProperty(sound, SampleIds::HiKey, 1);
				changeProperty(sound, SampleIds::LoKey, 1);
				changeProperty(sound, SampleIds::Root, 1);
			}
			else
			{
				changeProperty(sound, SampleIds::LoKey, -1);
				changeProperty(sound, SampleIds::HiKey, -1);
				changeProperty(sound, SampleIds::Root, -1);
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

			changeProperty(sound, SampleIds::HiVel, direction == SamplerSoundMap::Up ? 1 : -1);
			changeProperty(sound, SampleIds::LoVel, direction == SamplerSoundMap::Up ? 1 : -1);
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
	auto& sounds = selectedSamplerSounds.getItemArray();

	SampleSelection existingSounds;

	existingSounds.ensureStorageAllocated(sounds.size());

	for (int i = 0; i < sounds.size(); i++)
	{
		if (sounds[i].get() != nullptr) 
			existingSounds.add(sounds[i].get());
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

void SampleEditHandler::changeProperty(ModulatorSamplerSound *s, const Identifier& p, int delta)
{
	const int v = s->getSampleProperty(p);

	const int newValue = jlimit<int>(0, 127, v + delta);

	if(v != newValue)
		s->setSampleProperty(p, newValue);
}

juce::File SampleEditHandler::getCurrentSampleMapDirectory() const
{
	auto handler = &sampler->getMainController()->getCurrentFileHandler();

	if (auto exp = sampler->getMainController()->getExpansionHandler().getCurrentExpansion())
		handler = exp;

	return handler->getSubDirectory(ProjectHandler::SubDirectories::SampleMaps);
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
		auto v = handler->getSampler()->getSampleMap()->getValueTree();

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

		v.setProperty("MicPositions", multimicTokens, nullptr);

		auto ref = handler->getSampler()->getSampleMap()->getReference();
		
		handler->getSampler()->getMainController()->getCurrentSampleMapPool()->sendPoolChangeMessage(PoolBase::Reloaded, sendNotificationAsync, ref);


#if 0
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
#endif
		
	}

}

void SampleEditHandler::SampleEditingActions::reencodeMonolith(Component* childComponentOfMainEditor, SampleEditHandler* handler)
{
	if (PresetHandler::showYesNoWindow("Reencode monolith", "Do you want to reencode the monolith?\nYou need the original files at the same location in order to make this work"))
	{
		auto s = handler->getSampler();

		auto map = s->getSampleMap();

		auto tree = map->getValueTree().createCopy();

		tree.setProperty("SaveMode", 0, nullptr);
		
		for (auto sample : tree)
		{
			sample.removeProperty("MonolithOffset", nullptr);
			sample.removeProperty("MonolithLength", nullptr);
		}

		auto f = [map, tree, childComponentOfMainEditor](Processor* )
		{
			map->loadUnsavedValueTree(tree);

			auto f2 = [map, childComponentOfMainEditor]()
			{
				map->saveAsMonolith(childComponentOfMainEditor);
			};

			MessageManager::callAsync(f2);

			return SafeFunctionCall::OK;
		};

		s->killAllVoicesAndCall(f, true);
	}
}

void SampleEditHandler::SampleEditingActions::encodeAllMonoliths(Component * comp, SampleEditHandler* handler)
{
#if HI_ENABLE_EXPANSION_EDITING
	BatchReencoder *encoder = new BatchReencoder(handler->getSampler());

	encoder->setModalBaseWindowComponent(comp);
#endif
}






} // namespace hise
