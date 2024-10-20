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
	for (int i = 0; i < HISE_NUM_MACROS; i++)
	{
		macroControllerNumbers[i] = -1;
	};
}

MainController::MacroManager::~MacroManager()
{}

ModulatorSynthChain* MainController::MacroManager::getMacroChain()
{ return macroChain; }

const ModulatorSynthChain* MainController::MacroManager::getMacroChain() const
{return macroChain; }

void MainController::MacroManager::setMacroChain(ModulatorSynthChain* chain)
{ macroChain = chain; }

bool MainController::MacroManager::macroControlMidiLearnModeActive()
{ return macroIndexForCurrentMidiLearnMode != -1; }

bool MainController::MacroManager::isMacroEnabledOnFrontend() const
{ return enableMacroOnFrontend; }

void MainController::MacroManager::setEnableMacroOnFrontend(bool shouldBeEnabled)
{ enableMacroOnFrontend = shouldBeEnabled; }


void MainController::MacroManager::removeMacroControlsFor(Processor *p)
{
	if (p == macroChain) return; // it will delete itself

	if (macroChain == nullptr) return;

	for (int i = 0; i < HISE_NUM_MACROS; i++)
	{
		macroChain->getMacroControlData(i)->removeAllParametersWithProcessor(p);
	}

	macroChain->sendOtherChangeMessage(dispatch::library::ProcessorChangeEvent::Macro);
}

void MainController::MacroManager::removeMacroControlsFor(Processor *p, Identifier name)
{
	if (p == macroChain) return; // it will delete itself

	if (macroChain == nullptr) return;

	for (int i = 0; i < HISE_NUM_MACROS; i++)
	{
		MacroControlBroadcaster::MacroControlData *data = macroChain->getMacroControlData(i);

		for (int j = 0; j < data->getNumParameters(); j++)
		{
			if (data->getParameter(j)->getParameterName() == name.toString() && data->getParameter(j)->getProcessor() == p)
			{
				data->removeParameter(j);
				macroChain->sendOtherChangeMessage(dispatch::library::ProcessorChangeEvent::Macro);
				return;
			}
		}
	}

	macroChain->sendOtherChangeMessage(dispatch::library::ProcessorChangeEvent::Macro);
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
	if (isPositiveAndBelow(macroIndex, HISE_NUM_MACROS))
	{
		macroControllerNumbers[macroIndex] = midiControllerNumber;
	}
}

bool MainController::MacroManager::midiMacroControlActive() const
{
	for (int i = 0; i < HISE_NUM_MACROS; i++)
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
	for (int i = 0; i < HISE_NUM_MACROS; i++)
	{
		if (macroControllerNumbers[i] == midiController) return i;
	}

	return -1;
}

int MainController::MacroManager::getMidiControllerForMacro(int macroIndex)
{
	if (isPositiveAndBelow(macroIndex, HISE_NUM_MACROS))
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
	if (isPositiveAndBelow(macroIndex, HISE_NUM_MACROS))
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
	if (isPositiveAndBelow(macroIndex, HISE_NUM_MACROS))
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
#if USE_BACKEND
	suspend(false);
#endif

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


void MainController::CodeHandler::writeToConsole(const String &t, int warningLevel, const Processor *p, Colour /*c*/)
{
#if USE_BACKEND
	if (CompileExporter::isExportingFromCommandLine())
	{
#if !JUCE_MAC
        DBG(t);
#endif
        std::cout << t << "\n";
		return;
	}
#endif

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

		if (processor != nullptr)
		{
			message << processor->getId() << ":";
			message << (cm.warningLevel == WarningLevel::Error ? "! " : " ");
		}
		
		message << cm.message << "\n";

		return MultithreadedQueueHelpers::OK;
	};

	pendingMessages.clear(f);

	consoleData.insertText(consoleData.getNumCharacters(), message);

	overflowProtection = false;

	WeakReference<Processor> p = mc->getMainSynthChain();
	
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

} // namespace hise
