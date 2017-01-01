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


MainController::MacroManager::MacroManager(MainController *mc_) :
	macroIndexForCurrentLearnMode(-1),
	macroChain(nullptr),
	mc(mc_),
	midiControllerHandler(mc_)
{
	for (int i = 0; i < 8; i++)
	{
		macroControllerNumbers[i] = -1;
	};
}


void MainController::MacroManager::removeMacroControlsFor(Processor *p)
{
	if (p == macroChain) return; // it will delete itself

	if (macroChain == nullptr) return;

	for (int i = 0; i < 8; i++)
	{
		macroChain->getMacroControlData(i)->removeAllParametersWithProcessor(p);
	}

	macroChain->sendSynchronousChangeMessage();
}

void MainController::MacroManager::removeMacroControlsFor(Processor *p, Identifier name)
{
	if (p == macroChain) return; // it will delete itself

	if (macroChain == nullptr) return;

	for (int i = 0; i < 8; i++)
	{
		MacroControlBroadcaster::MacroControlData *data = macroChain->getMacroControlData(i);

		for (int j = 0; j < data->getNumParameters(); j++)
		{
			if (data->getParameter(j)->getParameterName() == name.toString() && data->getParameter(j)->getProcessor() == p)
			{
				data->removeParameter(j);
				macroChain->sendChangeMessage();
				return;
			}
		}
	}

	macroChain->sendSynchronousChangeMessage();
}

MidiControllerAutomationHandler * MainController::MacroManager::getMidiControlAutomationHandler()
{
	return &midiControllerHandler;
}

const MidiControllerAutomationHandler * MainController::MacroManager::getMidiControlAutomationHandler() const
{
	return &midiControllerHandler;
}

void MainController::MacroManager::setMidiControllerForMacro(int midiControllerNumber)
{
	if (macroIndexForCurrentMidiLearnMode >= 0 && macroIndexForCurrentMidiLearnMode < 8)
	{
		macroControllerNumbers[macroIndexForCurrentMidiLearnMode] = midiControllerNumber;

		getMacroChain()->getMacroControlData(macroIndexForCurrentMidiLearnMode)->setMidiController(midiControllerNumber);

		macroIndexForCurrentMidiLearnMode = -1;
	}
}

void MainController::MacroManager::setMidiControllerForMacro(int macroIndex, int midiControllerNumber)
{
	if (macroIndex < 8)
	{
		macroControllerNumbers[macroIndex] = midiControllerNumber;
	}
}

bool MainController::MacroManager::midiMacroControlActive() const
{
	for (int i = 0; i < 8; i++)
	{
		if (macroControllerNumbers[i] != -1) return true;
	}

	return false;
}

void MainController::MacroManager::setMacroControlMidiLearnMode(ModulatorSynthChain *chain, int index)
{
	macroChain = chain;
	macroIndexForCurrentMidiLearnMode = index;
}

int MainController::MacroManager::getMacroControlForMidiController(int midiController)
{
	for (int i = 0; i < 8; i++)
	{
		if (macroControllerNumbers[i] == midiController) return i;
	}

	return -1;
}

int MainController::MacroManager::getMidiControllerForMacro(int macroIndex)
{
	if (macroIndex < 8)
	{
		return macroControllerNumbers[macroIndex];
	}
	else
	{
		return -1;
	}
}

bool MainController::MacroManager::midiControlActiveForMacro(int macroIndex) const
{
	if (macroIndex < 8)
	{
		return macroControllerNumbers[macroIndex] != -1;
	}
	else
	{
		jassertfalse;
		return false;
	}
}

void MainController::MacroManager::removeMidiController(int macroIndex)
{
	if (macroIndex < 8)
	{
		macroControllerNumbers[macroIndex] = -1;
	}
}

void MainController::MacroManager::setMacroControlLearnMode(ModulatorSynthChain *chain, int index)
{
	macroChain = chain;
	macroIndexForCurrentLearnMode = index;
}

int MainController::MacroManager::getMacroControlLearnMode() const
{
	return macroIndexForCurrentLearnMode;
}



MainController::SampleManager::SampleManager(MainController *mc_) :
	mc(mc_),
	samplerLoaderThreadPool(new SampleThreadPool()),
	projectHandler(mc_),
	globalSamplerSoundPool(new ModulatorSamplerSoundPool(mc)),
	globalAudioSampleBufferPool(new AudioSampleBufferPool(&projectHandler)),
	globalImagePool(new ImagePool(&projectHandler)),
	sampleClipboard(ValueTree("clipboard")),
	useRelativePathsToProjectFolder(true)
{

}


void MainController::SampleManager::copySamplesToClipboard(const Array<WeakReference<ModulatorSamplerSound>> &soundsToCopy)
{
	sampleClipboard.removeAllChildren(nullptr);

	for (int i = 0; i < soundsToCopy.size(); i++)
	{
		if (soundsToCopy[i].get() != nullptr)
		{
			ValueTree soundTree = soundsToCopy[i]->exportAsValueTree();

			static Identifier duplicate("Duplicate");
			soundTree.setProperty(duplicate, true, nullptr);

			sampleClipboard.addChild(soundTree, -1, nullptr);
		}
	}
}

const ValueTree &MainController::SampleManager::getSamplesFromClipboard() const { return sampleClipboard; }

const ValueTree MainController::SampleManager::getLoadedSampleMap(const String &fileName) const
{
	for (int i = 0; i < sampleMaps.getNumChildren(); i++)
	{
		String childFileName = sampleMaps.getChild(i).getProperty("SampleMapIdentifier", String());
		if (childFileName == fileName) return sampleMaps.getChild(i);
	}

	return ValueTree();
}

void MainController::SampleManager::setDiskMode(DiskMode mode) noexcept
{
	mc->allNotesOff();

	hddMode = mode == DiskMode::HDD;

	const int multplier = hddMode ? 2 : 1;

	Processor::Iterator<ModulatorSampler> it(mc->getMainSynthChain());

	while (ModulatorSampler* sampler = it.getNextProcessor())
	{
		sampler->setPreloadMultiplier(multplier);
	}
}


void MainController::UserPresetHandler::timerCallback()
{
	loadPresetInternal();
	stopTimer();
}

void MainController::UserPresetHandler::loadUserPreset(const ValueTree& presetToLoad)
{
	currentPreset = presetToLoad;
	startTimer(50);
}

void MainController::UserPresetHandler::loadPresetInternal()
{
#if USE_BACKEND
	if (!GET_PROJECT_HANDLER(mc->getMainSynthChain()).isActive()) return;
#endif

	Processor::Iterator<JavascriptMidiProcessor> iter(mc->getMainSynthChain());

	while (JavascriptMidiProcessor *sp = iter.getNextProcessor())
	{
		if (!sp->isFront()) continue;

		ValueTree v;

		for (int i = 0; i < currentPreset.getNumChildren(); i++)
		{
			if (currentPreset.getChild(i).getProperty("Processor") == sp->getId())
			{
				v = currentPreset.getChild(i);
				break;
			}
		}

		if (v.isValid())
		{
			sp->getScriptingContent()->restoreAllControlsFromPreset(v);
		}
	}

	ValueTree autoData = currentPreset.getChildWithName("MidiAutomation");

	if (autoData.isValid())
		mc->getMacroManager().getMidiControlAutomationHandler()->restoreFromValueTree(autoData);

	mc->presetLoadRampFlag.set(1);

}


MainController::EventIdHandler::EventIdHandler(HiseEventBuffer& masterBuffer_) :
	masterBuffer(masterBuffer_),
	currentEventId(1)
{
	for (int i = 0; i < 128; i++)
		realNoteOnEvents[i] = HiseEvent();

	artificialEvents.calloc(HISE_EVENT_ID_ARRAY_SIZE, sizeof(HiseEvent));
}

MainController::EventIdHandler::~EventIdHandler()
{

}


void MainController::EventIdHandler::handleEventIds()
{
	HiseEventBuffer::Iterator it(masterBuffer);

	while (HiseEvent *m = it.getNextEventPointer())
	{
		// This operates on a global level before artificial notes are possible
		jassert(!m->isArtificial());

		if (m->isAllNotesOff())
		{
			memset(realNoteOnEvents, 0, sizeof(HiseEvent) * 128);
		}

		if (m->isNoteOn())
		{
			if (realNoteOnEvents[m->getNoteNumber()].isEmpty())
			{
				m->setEventId(currentEventId);
				realNoteOnEvents[m->getNoteNumber()] = HiseEvent(*m);
				currentEventId++;
			}
			else
			{
				// There is something fishy here so deactivate this event
				m->ignoreEvent(true);
			}
		}
		else if (m->isNoteOff())
		{
			if (!realNoteOnEvents[m->getNoteNumber()].isEmpty())
			{
				uint16 id = realNoteOnEvents[m->getNoteNumber()].getEventId();
				m->setEventId(id);
				realNoteOnEvents[m->getNoteNumber()] = HiseEvent();
			}
			else
			{
				// There is something fishy here so deactivate this event
				m->ignoreEvent(true);
			}
		}
	}
}

uint16 MainController::EventIdHandler::getEventIdForNoteOff(const HiseEvent &noteOffEvent)
{
	jassert(noteOffEvent.isNoteOff());

	const int noteNumber = noteOffEvent.getNoteNumber();

	if (!noteOffEvent.isArtificial())
	{
		return realNoteOnEvents[noteNumber].getEventId();
	}
	else
	{
		const uint16 eventId = noteOffEvent.getEventId();

		if (eventId != 0)
			return eventId;

		else
			return lastArtificialEventIds[noteOffEvent.getNoteNumber()];
	}
}

void MainController::EventIdHandler::pushArtificialNoteOn(HiseEvent& noteOnEvent) noexcept
{
	jassert(noteOnEvent.isNoteOn());
	jassert(noteOnEvent.isArtificial());

	noteOnEvent.setEventId(currentEventId);
	artificialEvents[currentEventId % HISE_EVENT_ID_ARRAY_SIZE] = noteOnEvent;
	lastArtificialEventIds[noteOnEvent.getNoteNumber()] = currentEventId;

	currentEventId++;
}


HiseEvent MainController::EventIdHandler::peekNoteOn(const HiseEvent& noteOffEvent)
{
	if (noteOffEvent.isArtificial())
	{
		if (noteOffEvent.getEventId() != 0)
		{
			return artificialEvents[noteOffEvent.getEventId() % HISE_EVENT_ID_ARRAY_SIZE];
		}
		else
		{
			jassertfalse;

			return HiseEvent();
		}
	}
	else
	{
		return realNoteOnEvents[noteOffEvent.getNoteNumber()];
	}
}

HiseEvent MainController::EventIdHandler::popNoteOnFromEventId(uint16 eventId)
{
	HiseEvent e;
	e.swapWith(artificialEvents[eventId % HISE_EVENT_ID_ARRAY_SIZE]);

	return e;
}

