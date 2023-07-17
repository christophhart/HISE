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

#ifndef MACROS_H_INCLUDED
#define MACROS_H_INCLUDED

namespace hise { using namespace juce;

#if (defined (_WIN32) || defined (_WIN64))
#define JUCE_WINDOWS 1
#endif


#if ENABLE_CONSOLE_OUTPUT
#define debugToConsole(p, x) (p->getMainController()->writeToConsole(x, 0, p))
#define debugError(p, x) (p->getMainController()->writeToConsole(x, 1, p))
#define debugMod(text) { if(consoleEnabled) debugProcessor(text); };
#else
#define debugToConsole(p, x) ignoreUnused(x)
#define debugError(p, x) ignoreUnused(x)
#define debugMod(text) ignoreUnused(text)
#endif

#define CONTAINER_WIDTH 900 - 32

#if USE_COPY_PROTECTION
#define CHECK_KEY(mainController){ if(FrontendProcessor* fp = dynamic_cast<FrontendProcessor*>(mainController)) fp->checkKey();}
#else
#define CHECK_KEY(mainController) {mainController;}
#endif

#ifndef RETURN_IF_NO_THROW
#define RETURN_IF_NO_THROW(x) return x;
#define RETURN_VOID_IF_NO_THROW() return;
#endif


#if USE_BACKEND
#ifndef HI_ENABLE_EXPANSION_EDITING
#define HI_ENABLE_EXPANSION_EDITING 1
#endif
#else
#ifndef HI_ENABLE_EXPANSION_EDITING
#define HI_ENABLE_EXPANSION_EDITING 0
#endif
#endif

#if USE_COPY_PROTECTION
#ifndef HISE_ALLOW_OFFLINE_ACTIVATION
#define HISE_ALLOW_OFFLINE_ACTIVATION 1
#endif
#endif

#if USE_BACKEND
#define BACKEND_ONLY(x) x 
#define FRONTEND_ONLY(x)
#else
#define BACKEND_ONLY(x)
#define FRONTEND_ONLY(x) x
#endif



#if FRONTEND_IS_PLUGIN
#define FX_ONLY(x) x
#define INSTRUMENT_ONLY(x) 
#else
#define FX_ONLY(x)
#define INSTRUMENT_ONLY(x) x
#endif

#if USE_BACKEND
#define SET_CHANGED_FROM_PARENT_EDITOR() if(ProcessorEditor *editor = findParentComponentOfClass<ProcessorEditor>()) PresetHandler::setChanged(editor->getProcessor());
#else
#define SET_CHANGED_FROM_PARENT_EDITOR()
#endif

#define LOG_SYNTH_EVENTS 0

#if LOG_SYNTH_EVENTS
#define LOG_SYNTH_EVENT(x) DBG(x)
#else
#define LOG_SYNTH_EVENT(x)
#endif


#define ENABLE_KILL_LOG 0

#if ENABLE_KILL_LOG
#define KILL_LOG(x) DBG(x)
#else
#define KILL_LOG(x)
#endif

#define LOG_KILL_EVENTS KILL_LOG

#if USE_BACKEND
#define export_ci() CompileExporter::isExportingFromCommandLine()
#else
#define export_ci() false
#endif

#define jassert_message_thread jassert(export_ci() || MessageManager::getInstance()->currentThreadHasLockedMessageManager())
#define jassert_locked_script_thread(mc) jassert(export_ci() ||LockHelpers::isLockedBySameThread(mc, LockHelpers::ScriptLock));
#define jassert_dispatched_message_thread(mc) jassert_message_thread; jassert(export_ci() || mc->getLockFreeDispatcher().isInDispatchLoop());
#define jassert_sample_loading_thread(mc) jassert(export_ci() || !mc->isInitialised() || mc->getKillStateHandler().getCurrentThread() == MainController::KillStateHandler::SampleLoadingThread || LockHelpers::isLockedBySameThread(mc, LockHelpers::SampleLock));
#define jassert_sample_loading_or_global_lock(mc) jassert(export_ci() || mc->getKillStateHandler().getCurrentThread() == MainController::KillStateHandler::SampleLoadingThread || mc->getKillStateHandler().globalLockIsActive());
#define jassert_processor_idle jassert(export_ci() || !isOnAir() || !getMainController()->getKillStateHandler().isAudioRunning());
#define jassert_global_lock(mc) jassert(export_ci() || mc->getKillStateHandler().globalLockIsActive());


#define LOCK_PROCESSING_CHAIN(parent) LockHelpers::SafeLock itLock(parent->getMainController(), LockHelpers::IteratorLock, parent->isOnAir()); \
								  LockHelpers::SafeLock audioLock(parent->getMainController(), LockHelpers::AudioLock, parent->isOnAir());



#define HI_DECLARE_LISTENER_METHODS(x) public: \
	void addListener(x* l) { listeners.addIfNotAlreadyThere(l); }\
    void removeListener(x* l) { listeners.removeAllInstancesOf(l); }\
	private:\
	Array<WeakReference<x>> listeners;



#define GET_PROJECT_HANDLER(x)(x->getMainController()->getSampleManager().getProjectHandler())


#define GET_PROCESSOR_TYPE_ID(ProcessorClass) Identifier getProcessorTypeId() const override { return ProcessorClass::getConnectorId(); }
#define SET_PROCESSOR_CONNECTOR_TYPE_ID(name) static Identifier getConnectorId() { RETURN_STATIC_IDENTIFIER(name) };

#define loadTable(tableVariableName, nameAsString) { const var savedData = v.getProperty(nameAsString, var()); tableVariableName->restoreData(savedData); }
#define saveTable(tableVariableName, nameAsString) ( v.setProperty(nameAsString, tableVariableName->exportData(), nullptr) )

#if JUCE_DEBUG
#define START_TIMER() (startTimer(150))
#define IGNORE_UNUSED_IN_RELEASE(x) 
#else
#define START_TIMER() (startTimer(30))
#define IGNORE_UNUSED_IN_RELEASE(x) (ignoreUnused(x))
#endif

#define RETURN_WHEN_X_BUTTON() if (e.mods.isX1ButtonDown() || e.mods.isX2ButtonDown()) return;

#if JUCE_ENABLE_AUDIO_GUARD
#define LOCK_WITH_GUARD(mc) AudioThreadGuard guard(&(mc->getKillStateHandler())); GuardedScopedLock sl(mc->getLock());
#else
#define LOCK_WITH_GUARD(mc) ScopedLock sl(mc->getLock());
#endif

#define GET_HISE_SETTING(processor, settingId) dynamic_cast<const GlobalSettingManager*>(processor->getMainController())->getSettingsObject().getSetting(settingId)

#include "copyProtectionMacros.h"

} // namespace hise

#endif  // MACROS_H_INCLUDED
