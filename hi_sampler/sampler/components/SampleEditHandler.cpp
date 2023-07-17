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

SampleEditHandler::SampleEditHandler(ModulatorSampler* sampler_) :
	sampler(sampler_),
	internalSelectionListener(*this, sampler_->getMainController()),
	previewer(sampler_)
{
	noteBroadcaster.enableLockFreeUpdate(sampler->getMainController()->getGlobalUIUpdater());
	noteBroadcaster.setEnableQueue(true, NUM_POLYPHONIC_VOICES);
	groupBroadcaster.enableLockFreeUpdate(sampler->getMainController()->getGlobalUIUpdater());
	noteBroadcaster.addListener(*this, handleMidiSelection);

	selectionBroadcaster.addListener(*this, updateMainSound);

}

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
		default:
			break;
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
			auto sound = selectedSamplerSounds.getSelectedItem(i);

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
			auto sound = selectedSamplerSounds.getSelectedItem(i);

			changeProperty(sound, SampleIds::HiVel, direction == SamplerSoundMap::Up ? 1 : -1);
			changeProperty(sound, SampleIds::LoVel, direction == SamplerSoundMap::Up ? 1 : -1);
		}
		break;
	}
	default:
		break;
	}
}

void SampleEditHandler::resizeSamples(SamplerSoundMap::Neighbour direction)
{
	bool increase = (direction == SamplerSoundMap::Neighbour::Right) || (direction == SamplerSoundMap::Up);
	bool velocity = (direction == SamplerSoundMap::Neighbour::Up) || (direction == SamplerSoundMap::Down);

	for (auto sound : selectedSamplerSounds)
	{
		changeProperty(sound, velocity ? SampleIds::HiVel : SampleIds::HiKey, increase ? 1 : -1);
	}

}

void SampleEditHandler::handleMidiSelection(SampleEditHandler& handler, int noteNumber, int velocity)
{
	auto sampler = handler.sampler;
	
	if (velocity > 0 && sampler->getEditorState(ModulatorSampler::MidiSelectActive))
	{
		handler.selectedSamplerSounds.deselectAll();

		SelectedItemSet<const ModulatorSamplerSound*> midiSounds;

		ModulatorSampler::SoundIterator sIter(sampler);

		while (auto sound = sIter.getNextSound())
		{
			if (sampler->soundCanBePlayed(sound, 1, noteNumber, (float)velocity / 127.0f))
			{
				handler.selectedSamplerSounds.addToSelection(sound.get());
			}
		}

		handler.setMainSelectionToLast();
	}
}

void SampleEditHandler::cycleMainSelection(int indexToUse, int micIndexToUse, bool back)
{
	

	auto s = getNumSelected();

	if (s == 0)
		return;

	if (micIndexToUse == -1)
	{
		micIndexToUse = currentMicIndex;
	}

	if (indexToUse == -1)
	{
		indexToUse = selectedSamplerSounds.getItemArray().indexOf(currentMainSound);

		if (back)
			indexToUse = hmath::wrap(indexToUse - 1, s);
		else
			indexToUse = hmath::wrap(indexToUse +1, s);
	}

	auto sound = selectedSamplerSounds.getItemArray()[indexToUse];
	selectionBroadcaster.sendMessage(sendNotificationAsync, sound, micIndexToUse);
}

hise::SampleSelection SampleEditHandler::getSelectionOrMainOnlyInTabMode()
{
	SampleSelection s;

	if (applyToMainSelection && getNumSelected() > 0)
		s.add(currentMainSound);
	else
		s = selectedSamplerSounds.getItemArray();

	return s;
}

bool SampleEditHandler::keyPressed(const KeyPress& k, Component* originatingComponent)
{
	if (k == KeyPress('z', ModifierKeys::commandModifier, 'z'))
	{
		return getSampler()->getUndoManager()->undo();
	}
	if (k == KeyPress('z', ModifierKeys::commandModifier | ModifierKeys::shiftModifier, 'z'))
	{
		return getSampler()->getUndoManager()->redo();
	}
	if (k == KeyPress::F9Key)
	{
		SampleEditingActions::toggleFirstScriptButton(this);
		return true;
	}

	if (k.getKeyCode() == KeyPress::escapeKey)
	{
		SampleEditingActions::deselectAllSamples(this);
		return true;
	}
	if (k.getKeyCode() == KeyPress::leftKey)
	{
		if (k.getModifiers().isCommandDown())
		{
			if (k.getModifiers().isShiftDown())
				resizeSamples(SamplerSoundMap::Left);
			else
				moveSamples(SamplerSoundMap::Left);
		}
		else
			SampleEditingActions::selectNeighbourSample(this, SamplerSoundMap::Left, k.getModifiers());
		return true;
	}
	else if (k.getKeyCode() == KeyPress::rightKey)
	{
		if (k.getModifiers().isCommandDown())
		{
			if (k.getModifiers().isShiftDown())
				resizeSamples(SamplerSoundMap::Right);
			else
				moveSamples(SamplerSoundMap::Right);
		}
		else
			SampleEditingActions::selectNeighbourSample(this, SamplerSoundMap::Right, k.getModifiers());
		return true;
	}
	else if (k.getKeyCode() == KeyPress::upKey)
	{
		if (k.getModifiers().isCommandDown())
		{
			if (k.getModifiers().isShiftDown())
				resizeSamples(SamplerSoundMap::Up);
			else
				moveSamples(SamplerSoundMap::Up);
		}
		else
			SampleEditingActions::selectNeighbourSample(this, SamplerSoundMap::Up, k.getModifiers());
		return true;
	}
	else if (k.getKeyCode() == KeyPress::downKey)
	{
		if (k.getModifiers().isCommandDown())
		{
			if (k.getModifiers().isShiftDown())
				resizeSamples(SamplerSoundMap::Down);
			else
				moveSamples(SamplerSoundMap::Down);
		}
		else
			SampleEditingActions::selectNeighbourSample(this, SamplerSoundMap::Down, k.getModifiers());
		return true;
	}
	if (k == KeyPress::deleteKey)
	{
		SampleEditingActions::deleteSelectedSounds(this);
		return true;
	}
	if (k == KeyPress::tabKey)
	{
		if (!applyToMainSelection)
		{
			if (PresetHandler::showYesNoWindow("Enable Tab cycle mode", "Do you want to enable the tab key cycle mode?  \nIf this is enabled, all changes will only be applied to the single sample that is highlighted in the map editor."))
			{
				applyToMainSelection = true;
				selectionBroadcaster.resendLastMessage(sendNotificationAsync);
			}
		}

		cycleMainSelection(-1, -1, k.getModifiers().isShiftDown());

		return true;
	}

	return false;
}

void SampleEditHandler::setMainSelectionToLast()
{
	sampler->getMainController()->stopBufferToPlay();
	selectionBroadcaster.sendMessage(sendNotificationSync, selectedSamplerSounds.getItemArray().getLast(), currentMicIndex);
}

void SampleEditHandler::updateMainSound(SampleEditHandler& s, ModulatorSamplerSound::Ptr sound, int micIndex)
{
	s.currentMainSound = sound;
	s.currentMicIndex = micIndex;

	s.previewer.setPreviewStart(-1);

	if (sound != nullptr && s.getNumSelected() == 0)
	{
		s.selectedSamplerSounds.addToSelection(sound);
	}
}

bool SampleEditHandler::newKeysPressed(const uint8 *currentNotes)
{
	for (int i = 0; i < 127; i++)
	{
		if (currentNotes[i] != 0) return true;
	}
	return false;
}

void SampleEditHandler::changeProperty(ModulatorSamplerSound::Ptr s, const Identifier& p, int delta)
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


void SampleEditHandler::SampleEditingActions::toggleFirstScriptButton(SampleEditHandler* handler)
{
	if (auto jsp = ProcessorHelpers::getFirstProcessorWithType<JavascriptMidiProcessor>(handler->sampler))
	{
		auto v = jsp->getAttribute(0);

		if (v == 0.0f)
		{
			jsp->setAttribute(0, 1.0f, sendNotificationAsync);

			Timer::callAfterDelay(500, [jsp]()
				{
					jsp->setAttribute(0, 0.0f, sendNotificationAsync);
				});

			return;
		}
	}
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






hise::ModulatorSamplerSound* SampleEditHandler::SampleEditingActions::getNeighbourSample(SampleEditHandler* handler, SamplerSoundMap::Neighbour direction)
{
	if (direction == SamplerSoundMap::Neighbour::numNeighbours)
		direction = handler->currentDirection;

	if (handler->getNumSelected() > 0)
	{
		auto sound = *handler->begin();

		if (sound == nullptr)
			return nullptr;

		Array<int> lowKeys;
		Array<int> hiKeys;
		Array<int> lowVelos;
		Array<int> hiVelos;

		for (auto ts : *handler)
		{
			const int lowKey = ts->getSampleProperty(SampleIds::LoKey);
			const int lowVelo = ts->getSampleProperty(SampleIds::LoVel);

			const int hiKey = ts->getSampleProperty(SampleIds::HiKey);
			const int hiVelo = ts->getSampleProperty(SampleIds::HiVel);

			lowKeys.add(lowKey);
			lowVelos.add(lowVelo);
			hiKeys.add(hiKey);
			hiVelos.add(hiVelo);
		}

		const int group = sound->getSampleProperty(SampleIds::RRGroup);

		ModulatorSampler::SoundIterator iter(handler->getSampler());

		while (auto s = iter.getNextSound())
		{
			const int thisLowKey = s->getSampleProperty(SampleIds::LoKey);
			const int thisLowVelo = s->getSampleProperty(SampleIds::LoVel);

			const int thisHiKey = s->getSampleProperty(SampleIds::HiKey);
			const int thisHiVelo = s->getSampleProperty(SampleIds::HiVel);

			const int thisGroup = s->getSampleProperty(SampleIds::RRGroup);

			if (thisGroup != group) continue;

			if ((direction == SamplerSoundMap::Left || direction == SamplerSoundMap::Right) &&
				!hiVelos.contains(thisHiVelo) && !lowVelos.contains(thisLowVelo)) continue;

			if ((direction == SamplerSoundMap::Up || direction == SamplerSoundMap::Down) &&
				!hiKeys.contains(thisHiKey) && !lowKeys.contains(thisLowKey)) continue;

			bool selectThisComponent = false;

			switch (direction)
			{
			case SamplerSoundMap::Left:		selectThisComponent = lowKeys.contains(thisHiKey + 1); break;
			case SamplerSoundMap::Right:	selectThisComponent = hiKeys.contains(thisLowKey - 1); break;
			case SamplerSoundMap::Up:		selectThisComponent = hiVelos.contains(thisLowVelo - 1); break;
			case SamplerSoundMap::Down:		selectThisComponent = lowVelos.contains(thisHiVelo + 1); break;
			default:
				break;
			}

			if (selectThisComponent)
			{
				return s;
			}
		}
	}

	return nullptr;
}

void SampleEditHandler::SampleEditingActions::selectNeighbourSample(SampleEditHandler* handler, SamplerSoundMap::Neighbour direction, ModifierKeys mods)
{
	handler->currentDirection = direction;

	if (auto s = getNeighbourSample(handler, direction))
	{
		handler->getSelectionReference().addToSelectionBasedOnModifiers(s, mods);
		handler->setMainSelectionToLast();
	}
}

SampleEditHandler::PrivateSelectionUpdater::PrivateSelectionUpdater(SampleEditHandler& parent_, MainController* mc) :
	parent(parent_)
{
	if(MessageManager::getInstanceWithoutCreating()->isThisTheMessageThread())
		parent.selectedSamplerSounds.addChangeListener(this);
	else
	{
		WeakReference<PrivateSelectionUpdater> safeThis(this);

		MessageManager::callAsync([safeThis]()
		{
			if (safeThis != nullptr)
				safeThis->parent.selectedSamplerSounds.addChangeListener(safeThis);
		});
	}
}

void SampleEditHandler::PrivateSelectionUpdater::changeListenerCallback(ChangeBroadcaster*)
{
	parent.allSelectionBroadcaster.sendMessage(sendNotificationSync, parent.getNumSelected());
}

SampleEditHandler::SubEditorTraverser::SubEditorTraverser(Component* sub) :
	component(sub)
{
	if (dynamic_cast<SamplerSubEditor*>(sub) == nullptr)
	{
		component = dynamic_cast<Component*>(sub->findParentComponentOfClass<SamplerSubEditor>());
	}
}

} // namespace hise
