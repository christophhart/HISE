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

	macroChain->sendChangeMessage();
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

	macroChain->sendChangeMessage();
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


MainController::CodeHandler::CodeHandler(MainController* mc_):
	mc(mc_),
	pendingMessages(8192)
{
	consoleData.setDisableUndo(true);
}


void MainController::CodeHandler::setMainConsole(Console* console)
{
	mainConsole = dynamic_cast<Component*>(console);
}


void MainController::CodeHandler::initialise()
{
	if (!initialised)
	{
		initialised = true;
	}
}


void MainController::CodeHandler::writeToConsole(const String &t, int warningLevel, const Processor *p, Colour c)
{
	pendingMessages.push({ (WarningLevel)warningLevel, const_cast<Processor*>(p), t });
	triggerAsyncUpdate();
}

void MainController::CodeHandler::clearConsole()
{
	clearFlag = true;

	triggerAsyncUpdate();
}

void MainController::CodeHandler::printPendingMessagesFromQueue()
{
#if USE_BACKEND
	if (clearFlag)
	{
		consoleData.replaceAllContent({});
		clearFlag = false;
	}

	String message;

	auto f = [&message](ConsoleMessage& cm)
	{
		auto processor = cm.p.get();

		if (processor == nullptr)
			return MultithreadedQueueHelpers::SkipFurtherExecutions;

		message << processor->getId() << ":";
		message << (cm.warningLevel == WarningLevel::Error ? "! " : " ");
		message << cm.message << "\n";

		return MultithreadedQueueHelpers::OK;
	};

	pendingMessages.clear(f);

	consoleData.insertText(consoleData.getNumCharacters(), message);

	overflowProtection = false;

	WeakReference<Processor> p = mc->getMainSynthChain();

	auto c = mainConsole.getComponent();

	auto toggle = [c](Dispatchable* obj)
	{
		auto p = static_cast<Processor*>(obj);

		if (p->getMainController()->getConsoleHandler().getMainConsole() != nullptr)
		{
			auto rootWindow = GET_BACKEND_ROOT_WINDOW(c);

			if (rootWindow != nullptr)
				BackendPanelHelpers::toggleVisibilityForRightColumnPanel<ConsolePanel>(rootWindow->getRootFloatingTile(), true);
		}
		
		return Status::OK;
	};

	mc->getLockFreeDispatcher().callOnMessageThreadAfterSuspension(p.get(), toggle);
#endif
}


void MainController::CodeHandler::handleAsyncUpdate()
{
	printPendingMessagesFromQueue();

#if 0
#if USE_BACKEND
	consoleData.clearUndoHistory();


	if (clearFlag)
	{
		consoleData.clearUndoHistory();
		consoleData.replaceAllContent({});
		clearFlag = false;
	}

	std::vector<ConsoleMessage> messagesForThisTime;
	messagesForThisTime.reserve(10);

	if (unprintedMessages.size() != 0)
	{
		ScopedLock sl(getLock());
		messagesForThisTime.swap(unprintedMessages);
	}
	else return;

	String message;

	for (size_t i = 0; i < messagesForThisTime.size(); i++)
	{
		const Processor* processor = std::get<(int)ConsoleMessageItems::Processor>(messagesForThisTime[i]).get();

		if (processor == nullptr)
		{
			jassertfalse;
			continue;
		}

		message << processor->getId() << ":";
		message << (std::get<(int)ConsoleMessageItems::WarningLevel>(messagesForThisTime[i]) == WarningLevel::Error ? "! " : " ");
		message << std::get<(int)ConsoleMessageItems::Message>(messagesForThisTime[i]) << "\n";
	}

	consoleData.insertText(consoleData.getNumCharacters(), message);

	overflowProtection = false;

	if (getMainConsole() != nullptr)
	{
		auto rootWindow = GET_BACKEND_ROOT_WINDOW(mainConsole);
		
		if (rootWindow != nullptr)
		{

			BackendPanelHelpers::toggleVisibilityForRightColumnPanel<ConsolePanel>(rootWindow->getRootFloatingTile(), true);
		}
	}
#endif
#endif
}


MainController::EventIdHandler::EventIdHandler(HiseEventBuffer& masterBuffer_) :
	masterBuffer(masterBuffer_),
	currentEventId(1)
{
    firstCC.store(-1);
    secondCC.store(-1);
    
	//for (int i = 0; i < 128; i++)
		//realNoteOnEvents[i] = HiseEvent();

	memset(realNoteOnEvents, 0, sizeof(HiseEvent) * 128);

	artificialEvents.calloc(HISE_EVENT_ID_ARRAY_SIZE, sizeof(HiseEvent));
}

MainController::EventIdHandler::~EventIdHandler()
{

}


void MainController::EventIdHandler::handleEventIds()
{
	if (transposeValue != 0)
	{
		HiseEventBuffer::Iterator transposer(masterBuffer);

		while (HiseEvent* m = transposer.getNextEventPointer())
		{
			if (m->isNoteOnOrOff())
			{
				int newNoteNumber = jlimit<int>(0, 127, m->getNoteNumber() + transposeValue);

				m->setNoteNumber(newNoteNumber);
			}

			
		}
	}

	HiseEventBuffer::Iterator it(masterBuffer);

	while (HiseEvent *m = it.getNextEventPointer())
	{
		// This operates on a global level before artificial notes are possible
		jassert(!m->isArtificial());

		if (m->isAllNotesOff())
		{
			memset(realNoteOnEvents, 0, sizeof(HiseEvent) * 128 * 16);
		}

		if (m->isNoteOn())
		{
			auto channel = jlimit<int>(0, 15, m->getChannel()-1);

			if (realNoteOnEvents[channel][m->getNoteNumber()].isEmpty())
			{
				m->setEventId(currentEventId);
				realNoteOnEvents[channel][m->getNoteNumber()] = HiseEvent(*m);
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
			auto channel = jlimit<int>(0, 15, m->getChannel()-1);

			if (!realNoteOnEvents[channel][m->getNoteNumber()].isEmpty())
			{
                HiseEvent* on = &realNoteOnEvents[channel][m->getNoteNumber()];
                
				uint16 id = on->getEventId();
				m->setEventId(id);
                m->setTransposeAmount(on->getTransposeAmount());
                *on = HiseEvent();
			}
			else
			{
				// There is something fishy here so deactivate this event
				m->ignoreEvent(true);
			}
		}
		else if (firstCC != -1 && m->isController())
		{
			const int ccNumber = m->getControllerNumber();

			if (ccNumber == firstCC)
			{
				m->setControllerNumber(secondCC);
			}
			else if (ccNumber == secondCC)
			{
				m->setControllerNumber(firstCC);
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
		auto channel = jlimit<int>(0, 15, noteOffEvent.getChannel()-1);

		const HiseEvent* e = realNoteOnEvents[channel] + noteNumber;

		return e->getEventId();

		//return realNoteOnEvents +noteNumber.getEventId();
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
		auto channel = jlimit<int>(0, 15, noteOffEvent.getChannel()-1);

		return realNoteOnEvents[channel][noteOffEvent.getNoteNumber()];
	}
}

HiseEvent MainController::EventIdHandler::popNoteOnFromEventId(uint16 eventId)
{
	HiseEvent e;
	e.swapWith(artificialEvents[eventId % HISE_EVENT_ID_ARRAY_SIZE]);

	return e;
}

void MainController::EventIdHandler::setGlobalTransposeValue(int newTransposeValue)
{
	transposeValue = newTransposeValue;
}

void MainController::EventIdHandler::addCCRemap(int firstCC_, int secondCC_)
{
	firstCC = firstCC_;
	secondCC = secondCC_;

	if (firstCC_ == secondCC_)
	{
		firstCC_ = -1;
		secondCC_ = -1;
	}
}

} // namespace hise
