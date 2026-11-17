#include "ScriptProcessor.h"
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
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

namespace hise { using namespace juce;

	ProcessorWithScriptingContent::ProcessorWithScriptingContent(MainController* mc_):
		restoredContentValues(ValueTree("Content")),
		mc(mc_),
		contentParameterHandler(*this)
	{}

	hise::ProcessorMetadata ProcessorWithScriptingContent::withDynamicScriptParameters(const ProcessorMetadata& pd) const
	{
		auto md = pd;

		auto content = getScriptingContent();

		if (content->getNumComponents() == 0 && this->restoredContentValues.getNumChildren() != 0)
		{
			content->resetContentProperties();
		}

		for (int i = 0; i < content->getNumComponents(); i++)
		{
			auto sc = content->getComponent(i);

			md = md.withParameter(sc->createParameterMetadata(i));
		}

		return md;
	}

	void ProcessorWithScriptingContent::setAllowObjectConstruction(bool shouldBeAllowed)
	{
		allowObjectConstructors = shouldBeAllowed;
	}

	bool ProcessorWithScriptingContent::objectsCanBeCreated() const
	{
		return allowObjectConstructors;
	}

	int ProcessorWithScriptingContent::getCallbackEditorStateOffset() const
	{ return Processor::EditorState::numEditorStates; }

	ScriptingApi::Content* ProcessorWithScriptingContent::getScriptingContent() const
	{
		return content.get();
	}

	Identifier ProcessorWithScriptingContent::getContentParameterIdentifier(int parameterIndex) const
	{
		if (auto sc = content->getComponent(parameterIndex))
			return sc->name.toString();

		auto child = content->getContentProperties().getChild(parameterIndex);

		if (child.isValid())
			return Identifier(child.getProperty("id").toString());

		return Identifier();
	}

	int ProcessorWithScriptingContent::getContentParameterIdentifierIndex(const Identifier& id) const
	{
		for (int i = 0; i < content->getNumComponents(); i++)
			if (content->getComponent(i)->getName() == id)
				return i;

		return -1;
	}

	var ProcessorWithScriptingContent::getSavedValue(Identifier name)
	{
		return restoredContentValues.getChildWithName(name).getProperty("value", var::undefined());
	}

	MainController* ProcessorWithScriptingContent::getMainController_()
	{ return mc; }

	const MainController* ProcessorWithScriptingContent::getMainController_() const
	{ return mc; }

	void ProcessorWithScriptingContent::initContent()
	{
		content = new ScriptingApi::Content(this);

		callbackProfile.setSourceType(DebugSession::ProfileDataSource::SourceType::ScriptCallback);
		callbackProfile.setHolder(dynamic_cast<ApiProviderBase::Holder*>(this), true);
		callbackProfile.setPrefix(dynamic_cast<Processor*>(this)->getId() + ".");
	}

	ProcessorWithScriptingContent::ContentParameterHandler::ContentParameterHandler(
		ProcessorWithScriptingContent& parent):
		p(parent)
	{}

	void ProcessorWithScriptingContent::ContentParameterHandler::setParameter(int index, float newValue)
	{
		p.setControlValue(index, newValue);
	}

	float ProcessorWithScriptingContent::ContentParameterHandler::getParameter(int index) const
	{
		return p.getControlValue(index);
	}

	int ProcessorWithScriptingContent::ContentParameterHandler::getNumParameters() const
	{
		return p.getNumScriptParameters();
	}

	Identifier ProcessorWithScriptingContent::ContentParameterHandler::getParameterId(int parameterIndex) const
	{
		return p.getContentParameterIdentifier(parameterIndex);
	}

ProcessorWithScriptingContent::~ProcessorWithScriptingContent()
{
	if (content != nullptr)
	{
		content->removeAllScriptComponents();
	}
	
}

void ProcessorWithScriptingContent::suspendStateChanged(bool shouldBeSuspended)
{
	if(content != nullptr)
		content->suspendPanelTimers(shouldBeSuspended);
}

void ProcessorWithScriptingContent::setControlValue(int index, float newValue)
{
	if (content != nullptr && isPositiveAndBelow(index, content->getNumComponents()))
	{
		ScriptingApi::Content::ScriptComponent *c = content->getComponent(index);

		if (auto lc = c->getLinkedComponent())
		{
			c = lc;
		}

		if (c != nullptr)
		{
			c->setValue(newValue);

			if (auto b = dynamic_cast<ScriptingApi::Content::ScriptButton*>(c))
			{
				if (int group = b->getScriptObjectProperty(ScriptingApi::Content::ScriptButton::Properties::radioGroup))
				{
					if (newValue > 0.5f)
					{
						for (int i = 0; i < content->getNumComponents(); i++)
						{
							if (i == index) continue;

							if (auto other = dynamic_cast<ScriptingApi::Content::ScriptButton*>(content->getComponent(i)))
							{
								if ((int)other->getScriptObjectProperty(ScriptingApi::Content::ScriptButton::Properties::radioGroup) == group)
									other->setValue(0);
							}
						}
					}
				}
			}
			controlCallback(c, newValue);
		}
	}
}

float ProcessorWithScriptingContent::getControlValue(int index) const
{
	if (content != nullptr && index < content->getNumComponents())
		return content->getComponent(index)->getValue();

	else return 1.0f;
}

void ProcessorWithScriptingContent::controlCallback(ScriptingApi::Content::ScriptComponent *component, var controllerValue)
{
	if (thisAsJavascriptProcessor == nullptr)
		thisAsJavascriptProcessor = dynamic_cast<JavascriptProcessor*>(this);

	Processor* thisAsProcessor = dynamic_cast<Processor*>(this);

	auto sp = getScriptingContent()->contentProfile.profile(component->pSetAttribute);

    if (component->isConnectedToMacroControll())
    {
        
        float v = jlimit<float>(0.0, 127.0, (float)component->getValue());
        
        component->setMacroRecursionProtection(true);
        
        getMainController_()->getMainSynthChain()->setMacroControl(component->getMacroControlIndex(), v, sendNotification);
        
        component->setMacroRecursionProtection(false);
    }
	else if (component->isConnectedToProcessor())
	{
		float v = (float)controllerValue;
		FloatSanitizers::sanitizeFloatNumber(v);
		
		auto index = component->getConnectedParameterIndex();

		if (index == -2) // intensity
		{
			if (auto mod = dynamic_cast<Modulation*>(component->getConnectedProcessor()))
			{
				mod->setIntensity(v);
				BACKEND_ONLY(component->getConnectedProcessor()->sendOtherChangeMessage(dispatch::library::ProcessorChangeEvent::Intensity));
			}
		}
		else if (index == -3) // bypassed
		{
			component->getConnectedProcessor()->setBypassed(v > 0.5f, sendNotification);
			BACKEND_ONLY(component->getConnectedProcessor()->sendOtherChangeMessage(dispatch::library::ProcessorChangeEvent::Bypassed));
		}
		else if (index == -4) // enabled
		{
			component->getConnectedProcessor()->setBypassed(v < 0.5f, sendNotification);
			BACKEND_ONLY(component->getConnectedProcessor()->sendOtherChangeMessage(dispatch::library::ProcessorChangeEvent::Bypassed));

		}
		else
		{
			component->getConnectedProcessor()->setAttribute(index, v, sendNotificationSync);
		}

		if (auto sp = dynamic_cast<ScriptingApi::Content::ScriptPanel*>(component))
		{
			sp->repaintWrapped();
		}
	}
	else if (auto customAuto = component->getCustomAutomation())
	{
		customAuto->call((float)controllerValue, dispatch::DispatchType::sendNotificationSync);
	}
	else if (auto callback = component->getCustomControlCallback())
	{
		if (MessageManager::getInstance()->isThisTheMessageThread() || component->shouldDeferControlCallback())
		{
			auto f = [component, controllerValue](JavascriptProcessor* p)
			{
				auto& r = p->lastResult;
				dynamic_cast<ProcessorWithScriptingContent*>(p)->customControlCallbackIdle(component, controllerValue, r);
				return r;
			};

			getMainController_()->getJavascriptThreadPool().addJob(JavascriptThreadPool::Task::HiPriorityCallbackExecution,
				dynamic_cast<JavascriptProcessor*>(this),
				f);
		}
		else
		{
			customControlCallbackIdle(component, controllerValue, thisAsJavascriptProcessor->lastResult);
		}

	}
	else if (component->isConnectedToGlobalCable())
	{
		component->sendGlobalCableValue(controllerValue);
	}
	else
	{
		int callbackIndex = getControlCallbackIndex();

		getMainController_()->getDebugLogger().logParameterChange(thisAsJavascriptProcessor, component, controllerValue);

		JavascriptProcessor::SnippetDocument* onControlCallback = thisAsJavascriptProcessor->getSnippet(callbackIndex);

		if (!onControlCallback->isSnippetEmpty())
		{
			if (MessageManager::getInstance()->isThisTheMessageThread())
			{
				auto f = [component, controllerValue](JavascriptProcessor* p)
				{
					auto& r = p->lastResult;
					dynamic_cast<ProcessorWithScriptingContent*>(p)->defaultControlCallbackIdle(component, controllerValue, r);
					return r;
				};

				getMainController_()->getJavascriptThreadPool().addJob(JavascriptThreadPool::Task::HiPriorityCallbackExecution, 
																	   dynamic_cast<JavascriptProcessor*>(this), 
																	   f);
			}
			else
			{
				defaultControlCallbackIdle(component, controllerValue, thisAsJavascriptProcessor->lastResult);
			}
		}
	}

	thisAsProcessor->sendOtherChangeMessage(dispatch::library::ProcessorChangeEvent::Attribute, MessageManager::getInstance()->isThisTheMessageThread() ? dispatch::sendNotificationSync : dispatch::sendNotificationAsync);
}

void ProcessorWithScriptingContent::defaultControlCallbackIdle(ScriptingApi::Content::ScriptComponent *component, const var& controllerValue, UriError& r)
{
	getScriptingContent()->contentProfile.profile(component->pControlCallback);
	getScriptingContent()->contentProfile.closeTrack(component->pControlCallback);
	ScopedValueSetter<bool> objectConstructorSetter(allowObjectConstructors, true);

	int callbackIndex = getControlCallbackIndex();

	if (auto scriptEngine = thisAsJavascriptProcessor->getScriptEngine())
	{
		LockHelpers::SafeLock sl(getMainController_(), LockHelpers::Type::ScriptLock);

		scriptEngine->maximumExecutionTime = HiseJavascriptEngine::getDefaultTimeOut();

		scriptEngine->setCallbackParameter(callbackIndex, 0, component);
		scriptEngine->setCallbackParameter(callbackIndex, 1, controllerValue);
		scriptEngine->executeCallback(callbackIndex, &r);
	}

	LOG_RUNTIME_ERROR(r, thisAsJavascriptProcessor);
}



void ProcessorWithScriptingContent::customControlCallbackIdle(ScriptingApi::Content::ScriptComponent *component, const var& controllerValue, UriError& r)
{
	ScopedValueSetter<bool> objectConstructorSetter(allowObjectConstructors, true);

	auto sp = getScriptingContent()->contentProfile.profile(component->pControlCallback);
	getScriptingContent()->contentProfile.closeTrack(component->pControlCallback);

	getMainController_()->getDebugLogger().logParameterChange(thisAsJavascriptProcessor, component, controllerValue);

	auto callback = component->getCustomControlCallback();

	var fVar(callback);
	var args[2] = { var(component), controllerValue };

	if (auto scriptEngine = thisAsJavascriptProcessor->getScriptEngine())
	{
		LockHelpers::SafeLock sl(getMainController_(), LockHelpers::Type::ScriptLock);

		scriptEngine->maximumExecutionTime = HiseJavascriptEngine::getDefaultTimeOut();
		scriptEngine->executeInlineFunction(fVar, args, &r);

		LOG_RUNTIME_ERROR(r, thisAsJavascriptProcessor);
	}
}

ScriptBaseMidiProcessor::ScriptBaseMidiProcessor(MainController* mc, const String& id): 
	MidiProcessor(mc, id), 
	ProcessorWithScriptingContent(mc),
	currentEvent(nullptr)
{}

void countChildren(const ValueTree& t, int& numChildren)
{
	numChildren++;

	for (auto child : t)
	{
		countChildren(child, numChildren);
	}
}


int ProcessorWithScriptingContent::getNumScriptParameters() const
{
	if (content != nullptr)
	{
		auto c = content->getContentProperties();
		int numChildren = -1; // the root tree doesn't count

		countChildren(c, numChildren);

		return numChildren;
	}
	else
		return 0;
}

void ProcessorWithScriptingContent::restoreContent(const ValueTree &restoredState)
{
	auto& uph = getMainController_()->getUserPresetHandler();

	if (uph.isUsingCustomDataModel())
	{
		if (uph.isUsingPersistentObject())
		{
			restoredContentValues = restoredState;

			getMainController_()->getUserPresetHandler().restoreStateManager(restoredState, UserPresetIds::CustomJSON, UserPresetStateManager::StateTarget::PluginState);
		}
	}
	else
	{
		restoredContentValues = restoredState.getChildWithName("Content");

		if (content.get() != nullptr)
			content->restoreFromValueTree(restoredContentValues);
	}

	dynamic_cast<Processor*>(this)->updateParameterSlots(-1);
}

void ProcessorWithScriptingContent::saveContent(ValueTree &savedState) const
{
	if (content.get() != nullptr)
		savedState.addChild(content->exportAsValueTree(), -1, nullptr);
}

FileChangeListener::~FileChangeListener()
{
	watchers.clear();
	masterReference.clear();
}

ExternalScriptFile::Ptr FileChangeListener::addFileWatcher(const File &file)
{
	auto p = dynamic_cast<Processor*>(this)->getMainController()->getExternalScriptFile(file, true);

#if USE_BACKEND
	if(reloader == nullptr)
		reloader = new ExternalReloader(*this);
#endif

	watchers.addIfNotAlreadyThere(p);

	return p;
}

void FileChangeListener::setFileResult(const UriError& e)
{
	jassertfalse;
}

juce::CodeDocument::Position FileChangeListener::getLastPosition(CodeDocument& docToLookFor) const
{
	for (const auto& pos : lastPositions)
	{
		if (pos.getOwner() == &docToLookFor)
			return pos;
	}

	return CodeDocument::Position(docToLookFor, 0);
}

void FileChangeListener::setWatchedFilePosition(CodeDocument::Position& newPos)
{
	for (auto& p : lastPositions)
	{
		if (p.getOwner() == newPos.getOwner())
		{
			p = newPos;
			return;
		}
	}

	lastPositions.add(newPos);
}

void FileChangeListener::clearFileWatchers()
{
	watchers.clear();
}

int FileChangeListener::getNumWatchedFiles() const noexcept
{
	return watchers.size();
}

void FileChangeListener::setCurrentPopup(DocumentWindow* window)
{
	currentPopups.add(window);
}

void FileChangeListener::deleteAllPopups()
{
	if (currentPopups.size() != 0)
	{
		for (int i = 0; i < currentPopups.size(); i++)
		{
			if (currentPopups[i].getComponent() != nullptr)
			{
				currentPopups[i]->closeButtonPressed();
			}

		}

		currentPopups.clear();
	}
}

File FileChangeListener::getWatchedFile(int index) const
{
	if (index < watchers.size())
	{
		return watchers[index]->getFile();

	}
	else return File();
}

bool FileChangeListener::isEmbeddedSnippetFile(int index) const
{
	if(isPositiveAndBelow(index, watchers.size()))
	{
		return watchers[index]->getResourceType() == ExternalScriptFile::ResourceType::EmbeddedInSnippet;
	}

	return false;
}

CodeDocument& FileChangeListener::getWatchedFileDocument(int index) const
{
	if(index < watchers.size())
		return watchers[index]->getFileDocument();

	jassertfalse;
	return const_cast<FileChangeListener*>(this)->emptyDoc;
}

void FileChangeListener::showPopupForFile(int index, int charNumberToDisplay/*=0*/, int /*lineNumberToDisplay=-1*/)
{
#if USE_BACKEND

	auto mc = dynamic_cast<Processor*>(this)->getMainController();
	mc->getCommandManager()->invoke(BackendCommandTarget::MenuViewAddFloatingWindow, false);

	auto rw = GET_BACKEND_ROOT_WINDOW(mc->getConsoleHandler().getMainConsole());

	auto window = rw->getLastPopup();

	window->setName(watchers[index]->getFile().getFileName());
	window->centreWithSize(1000, 800);
	

	auto root = window->getRootFloatingTile();
	
	root->setNewContent(GET_PANEL_NAME(CodeEditorPanel));

	auto editor = dynamic_cast<CodeEditorPanel*>(root->getCurrentFloatingPanel());

	UriLocation loc;
	loc.uri = watchers[index]->getFile().getFullPathName();
	loc.charIndex = charNumberToDisplay;

	editor->gotoLocation(dynamic_cast<Processor*>(this), loc);

#else
	ignoreUnused(index, charNumberToDisplay);
#endif
}

void FileChangeListener::showPopupForFile(const File& f, int charNumberToDisplay /*= 0*/, int lineNumberToDisplay /*= -1*/)
{
	for (int i = 0; i < watchers.size(); i++)
	{
		if (watchers[i]->getFile() == f)
			showPopupForFile(i, charNumberToDisplay, lineNumberToDisplay);
	}
}

void JavascriptProcessor::showPopupForCallback(const Identifier& callback, int /*charNumberToDisplay*/, int /*lineNumberToDisplay*/)
{
#if USE_BACKEND

	auto mc = dynamic_cast<Processor*>(this)->getMainController();
	mc->getCommandManager()->invoke(BackendCommandTarget::MenuViewAddFloatingWindow, false);

	auto rw = GET_BACKEND_ROOT_WINDOW(mc->getConsoleHandler().getMainConsole());
	auto root = rw->getLastPopup()->getRootFloatingTile();

	root->setNewContent(GET_PANEL_NAME(CodeEditorPanel));

	auto editor = dynamic_cast<CodeEditorPanel*>(root->getCurrentFloatingPanel());

	auto jp = dynamic_cast<JavascriptProcessor*>(this);
	
	for (int i = 0; i < jp->getNumSnippets(); i++)
	{
		if (jp->getSnippet(i)->getCallbackName() == callback)
		{
			editor->setContentWithUndo(dynamic_cast<Processor*>(this), i);
			break;
		}
	}

#else
	ignoreUnused(callback);

#endif
}


void JavascriptProcessor::cleanupEngine()
{
	inplaceValues.clear();
	mainController->getScriptComponentEditBroadcaster()->clearSelection(sendNotification);
	scriptEngine = nullptr;
	dynamic_cast<ProcessorWithScriptingContent*>(this)->content = nullptr;
}

void JavascriptProcessor::setCallStackEnabled(bool shouldBeEnabled)
{
	callStackEnabled = shouldBeEnabled;
	
	if (scriptEngine != nullptr)
		scriptEngine->setCallStackEnabled(shouldBeEnabled);
}

ValueTree FileChangeListener::collectAllScriptFiles(ModulatorSynthChain *chainToExport)
{
	Processor::Iterator<JavascriptProcessor> iter(chainToExport);

	ValueTree externalScriptFiles = ValueTree("ExternalScripts");

	while (JavascriptProcessor *sp = iter.getNextProcessor())
	{
		if (sp->isConnectedToExternalFile())
		{
			const String fileNameReference = sp->getConnectedFileReference();
			
			bool exists = false;

			for (int j = 0; j < externalScriptFiles.getNumChildren(); j++)
			{
				if (externalScriptFiles.getChild(j).getProperty("FileName").toString() == fileNameReference)
				{
					exists = true;
					break;
				}
			}

			if (!exists)
			{
				String code;

				sp->mergeCallbacksToScript(code);

				ValueTree script("Script");
				script.setProperty("FileName", fileNameReference, nullptr);
				script.setProperty("Content", code, nullptr);

				externalScriptFiles.addChild(script, -1, nullptr);
			}
		}

        for (int i = 0; i < sp->getNumWatchedFiles(); i++)
		{
			File scriptFile = sp->getWatchedFile(i);

			addFileContentToValueTree(sp, externalScriptFiles, scriptFile, chainToExport);
		}

		Array<File> allScriptFiles;

		auto scriptDirectory = GET_PROJECT_HANDLER(chainToExport).getSubDirectory(ProjectHandler::SubDirectories::Scripts);

		scriptDirectory.findChildFiles(allScriptFiles, File::findFiles, true, "*.js");

		for (auto f : allScriptFiles)
		{
			if (HiseDeviceSimulator::fileNameContainsDeviceWildcard(f))
			{
				addFileContentToValueTree(sp, externalScriptFiles, f, chainToExport);
			}
		}
	}

	return externalScriptFiles;
}

void FileChangeListener::ExternalReloader::timerCallback()
{
	auto changed = false;

	for(auto w: parent.watchers)
	{
		changed |= w->reloadIfChanged();
	}

	if(changed)
		parent.fileChanged();
}

void FileChangeListener::addFileContentToValueTree(JavascriptProcessor* jp, ValueTree externalScriptFiles, File scriptFile, ModulatorSynthChain* chainToExport)
{
	String fileName = scriptFile.getRelativePathFrom(GET_PROJECT_HANDLER(chainToExport).getSubDirectory(ProjectHandler::SubDirectories::Scripts));

	File globalScriptFolder = PresetHandler::getGlobalScriptFolder(chainToExport);

	if (globalScriptFolder.isDirectory() && scriptFile.isAChildOf(globalScriptFolder))
	{
		fileName = "{GLOBAL_SCRIPT_FOLDER}" + scriptFile.getRelativePathFrom(globalScriptFolder);
	}

	// Wow, much cross-platform, very OSX, totally Windows
	fileName = fileName.replace("\\", "/");

	for (int j = 0; j < externalScriptFiles.getNumChildren(); j++)
	{
		if (externalScriptFiles.getChild(j).getProperty("FileName").toString() == fileName)
		{
			return;
		}
	}

    
	String fileContent = scriptFile.loadFileAsString();
    
	HiseJavascriptPreprocessor::ScopedBackendDeactivator sps(jp);

    auto ok = jp->getScriptEngine()->preprocessor->process(fileContent, fileName);
    
    jassert(ok.wasOk());
    
	ValueTree script("Script");

	script.setProperty("FileName", fileName, nullptr);
	script.setProperty("Content", fileContent, nullptr);

	externalScriptFiles.addChild(script, -1, nullptr);
}

JavascriptProcessor::ScopedPreprocessorMerger::ScopedPreprocessorMerger(MainController* mc)
{
	Processor::Iterator<JavascriptProcessor> iter(mc->getMainSynthChain());
            
	while(auto jp = iter.getNextProcessor())
	{
		jp->usePreprocessorAtMerge = true;
		list.add(jp);
	}
}

JavascriptProcessor::ScopedPreprocessorMerger::~ScopedPreprocessorMerger()
{
	for(auto jp: list)
		jp->usePreprocessorAtMerge = false;
}

JavascriptProcessor::SnippetDocument::~SnippetDocument()
{
	SpinLock::ScopedLockType sl(pendingLock);
	notifier.cancelPendingUpdate();
	pendingNewContent = {};
}

const Identifier& JavascriptProcessor::SnippetDocument::getCallbackName() const
{ return callbackName; }

bool JavascriptProcessor::SnippetDocument::isSnippetEmpty() const
{ return !isActive; }

int JavascriptProcessor::SnippetDocument::getNumArgs() const
{ return numArgs; }

void JavascriptProcessor::SnippetDocument::replaceContentAsync(String s, bool shouldBeAsync)
{
            
#if USE_FRONTEND
            // Not important when using compiled plugins because there will be no editor component
            // being resized...
            replaceAllContent(s);
#else
	{
		// Makes sure that this won't be accessed during replacement...
		SpinLock::ScopedLockType sl(pendingLock);
		pendingNewContent.swapWith(s);
	}
            
	if(shouldBeAsync)
	{
		notifier.notify();
	}
	else
	{   
		notifier.handleAsyncUpdate();
	}
#endif
}

JavascriptProcessor::SnippetDocument::Notifier::Notifier(SnippetDocument& parent_):
	parent(parent_)
{

}

void JavascriptProcessor::SnippetDocument::Notifier::notify()
{
	triggerAsyncUpdate();
}

void JavascriptProcessor::SnippetDocument::Notifier::handleAsyncUpdate()
{
	String text;
				
	{
		SpinLock::ScopedLockType sl(parent.pendingLock);
		parent.pendingNewContent.swapWith(text);
	}

	parent.setDisableUndo(true);
	parent.replaceAllContent(text);
	parent.setDisableUndo(false);
                
	parent.pendingNewContent = String();
}

ValueTree JavascriptProcessor::createApiTree()
{
#if USE_BACKEND
	return ApiHelpers::getApiTree();
#else
		return {};
#endif
}

int JavascriptProcessor::getCodeFontSize() const
{ return (int)dynamic_cast<const Processor*>(this)->getMainController()->getGlobalCodeFontSize(); }

bool JavascriptProcessor::isConnectedToExternalFile() const
{ return connectedFileReference.isNotEmpty(); }

const String& JavascriptProcessor::getConnectedFileReference() const
{ return connectedFileReference; }

bool JavascriptProcessor::wasLastCompileOK() const
{ return lastCompileWasOK; }

const UriError& JavascriptProcessor::getLastErrorMessage() const
{ return lastResult; }

ApiProviderBase* JavascriptProcessor::getProviderBase()
{
#if HISE_INCLUDE_PROFILING_TOOLKIT
	auto& dh = dynamic_cast<Processor*>(this)->getMainController()->getDebugSession();

	if(auto currentSession = dh.getProviderBase())
		return currentSession;
#endif

	return scriptEngine.get();
}

DebugSession* JavascriptProcessor::getDebugSession()
{
	return &mainController->getDebugSession();
}

HiseJavascriptEngine* JavascriptProcessor::getScriptEngine()
{ return scriptEngine; }

void JavascriptProcessor::logRuntimeError(const UriError & r)
{
	auto p = dynamic_cast<Processor*>(this);

	auto copy = r;
	copy.callstack = {};
	p->getMainController()->getJavascriptThreadPool().attachAsyncCallstack(copy);
	
	for (auto& ss : r.callstack)
		copy.callstack.push_back(ss);

	debugError(p, copy.toConsoleMessage());
	errorProvider.logUriError(UriError::SourceType::Runtime, copy);
}

#if USE_BACKEND
WeakCallbackHolder::CallableObject::StrictnessLevel JavascriptProcessor::getStrictnessLevel() const
{
	using SL = WeakCallbackHolder::CallableObject::StrictnessLevel;

	if (callScopeOverride != SL::Unset)
		return callScopeOverride;

	auto s = GET_HISE_SETTING(dynamic_cast<const Processor*>(this),
	                          HiseSettings::Scripting::CallScopeWarnings).toString();

	if (s == "Strict") return SL::Strict;
	if (s == "Warn")   return SL::Warn;
	if (s == "Unsafe") return SL::Unsafe;
	return SL::Unset;
}
#endif

void JavascriptProcessor::clearPreprocessorFunctions()
{
	preprocessorFunctions.clear();
}

void JavascriptProcessor::addPreprocessorFunction(const PreprocessorFunction& pf)
{
	preprocessorFunctions.add(pf);
}

ScriptingApi::Content* JavascriptProcessor::getContent()
{
	return dynamic_cast<ProcessorWithScriptingContent*>(this)->getScriptingContent();
}

const ScriptingApi::Content* JavascriptProcessor::getContent() const
{
	return dynamic_cast<const ProcessorWithScriptingContent*>(this)->getScriptingContent();
}

void JavascriptProcessor::clearContentPropertiesDoc()
{
	contentPropertyDocument = nullptr;
}

void JavascriptProcessor::setOptimisationReport(const String& report)
{
	lastOptimisationReport = report;
}

void JavascriptProcessor::postCompileCallback()
{}

JavascriptProcessor::JavascriptProcessor(MainController *mc) :
	ProcessorWithDynamicExternalData(mc),
	mainController(mc),
	scriptEngine(new HiseJavascriptEngine(this, mc)),
	lastCompileWasOK(false),
	lastResult(UriError::ok()),
	callStackEnabled(mc->isCallStackEnabled()),
	repaintDispatcher(mc),
	errorProvider(*this)
{
	initialiseProjectDll(mc);

	compileProfile.setHolder(this, true);
	compileProfile.setPrefix("later");
	compileProfile.setSourceType(DebugSession::ProfileDataSource::SourceType::Script);
	pCompileScript = compileProfile.add("compileScript()");
	pCreateDebugInfo = compileProfile.add("createDebugInfo()");
	pControlCallback = compileProfile.add("controlCallbacks()");

	allInterfaceData = ValueTree("UIData");
	auto defaultContent = ValueTree("ContentProperties");
	defaultContent.setProperty("DeviceType", "Desktop", nullptr);
	allInterfaceData.addChild(defaultContent, -1, nullptr);
}



JavascriptProcessor::~JavascriptProcessor()
{
	inplaceValues.clear();


	deleteAllPopups();
	scriptEngine = nullptr;


}


void JavascriptProcessor::addPopupMenuItems(PopupMenu &menu, Component* c, const MouseEvent& e)
{
#if USE_BACKEND
	String s = CommonEditorFunctions::getCurrentSelection(c);

	const String selection = CommonEditorFunctions::getCurrentSelection(c).trimEnd().trimStart();
	const bool isUIDefinitionSelected = selection.startsWith("const var");
	
	menu.addSectionHeader("Import / Export");
	menu.addItem(ScriptContextActions::SaveScriptFile, "Save Script To File");
	menu.addItem(ScriptContextActions::LoadScriptFile, "Load Script From File");
	menu.addSeparator();
	menu.addItem(ScriptContextActions::SaveScriptClipboard, "Save Script to Clipboard");
	menu.addItem(ScriptContextActions::LoadScriptClipboard, "Load Script from Clipboard");
	menu.addSeparator();
	menu.addItem(ScriptContextActions::ExportAsCompressedScript, "Export as compressed script");
	menu.addItem(ScriptContextActions::ImportCompressedScript, "Import compressed script");
	menu.addSeparator();
	menu.addItem(ScriptContextActions::MoveToExternalFile, "Move selection to external file");
	menu.addItem(ScriptContextActions::CreateUiFactoryMethod, "Create UI factory method from selection", isUIDefinitionSelected);
	menu.addSeparator();

	menu.addItem(ScriptContextActions::AddCodeBookmark, "Add code bookmark");
	menu.addSeparator();

	menu.addItem(ScriptContextActions::JumpToDefinition, "Jump to definition", true, false);
	menu.addItemWithShortcut(ScriptContextActions::FindAllOccurences, "Find all occurrences", KeyPress('f', ModifierKeys::commandModifier | ModifierKeys::shiftModifier, 'F'));
	menu.addItemWithShortcut(ScriptContextActions::SearchAndReplace, "Search & replace", KeyPress('g', ModifierKeys::commandModifier, 'G'));

	menu.addItemWithShortcut(ScriptContextActions::AddAutocompleteTemplate, "Add autocomplete template", KeyPress(KeyPress::F8Key), s.isNotEmpty());
	menu.addItemWithShortcut(ScriptContextActions::ClearAutocompleteTemplates, "Clear autocomplete templates", KeyPress(KeyPress::F8Key, ModifierKeys::commandModifier, 0));
#endif
}


bool JavascriptProcessor::jumpToDefinition(const UriLocation::WithExpression& locationAndToken)
{
	if (locationAndToken.first.isEmpty())
		return false;

	auto data = HiseJavascriptEngine::SimpleExpressionData::parse(getScriptEngine(), locationAndToken);

	if (auto t = data.getResolvedNode())
		return PopupIncludeEditor::gotoUriLocation(this, nullptr, t->getTokenLocation());

	return false;
}

void JavascriptProcessor::setActiveEditor(JavascriptCodeEditor* e, CodeDocument::Position pos)
{
#if USE_BACKEND
	dynamic_cast<Processor*>(this)->getMainController()->setLastActiveEditor(e, pos);
#endif
}

std::pair<bool, String> JavascriptProcessor::getUriContent(const String& uri) const
{
	UriLocation l;
	l.processorId = getUriProcessorId();
	l.uri = uri;

	if (l.getUriType() != UriLocation::UriType::ExternalFile)
	{
		if (auto snippet = getSnippet(Identifier(l.toLocationString(UriLocation::UriFormat::CallbackId))))
			return { true, snippet->getAllContent() };

		return { false, "Unknown callback: " + uri };
	}

	for (int i = 0; i < getNumWatchedFiles(); i++)
	{
		if (l.matchesFile(getWatchedFile(i)))
			return { true, getWatchedFileDocument(i).getAllContent() };
	}

#if USE_BACKEND
	auto p = dynamic_cast<const Processor*>(this);
	auto root = p->getMainController()->getActiveFileHandler()->getSubDirectory(FileHandlerBase::Scripts);
	auto f = root.getChildFile(uri);

	if (f.existsAsFile())
		return { true, f.loadFileAsString() };
#endif

	return { false, "Can't resolve include " + uri };
}

hise::JavascriptCodeEditor* JavascriptProcessor::getActiveEditor()
{
#if USE_BACKEND
	return dynamic_cast<JavascriptCodeEditor*>(dynamic_cast<Processor*>(this)->getMainController()->getLastActiveEditor());
#else
	return nullptr;
#endif
}

void JavascriptProcessor::addInplaceDebugValue(const UriLocation& loc, const String& value, DebugInformationBase::Ptr info)
{
	bool initialised = false;

	auto lineNumber = loc.line;
	auto callback = loc.toLocationString(UriLocation::UriFormat::CallbackId);
	auto docToUse = getSnippetOrExternalFile(loc);
	
	lineNumber--;

	if(docToUse == nullptr)
	{
		if(scriptEngine != nullptr && docToUse == nullptr)
		{
			for(int i = 0; i < scriptEngine->getNumIncludedFiles(); i++)
			{
				if(loc.matchesFile(scriptEngine->getIncludedFile(i)))
				{
					std::pair<String, mcl::LanguageManager::InplaceDebugValue::Ptr> d;

					d.second = new mcl::LanguageManager::InplaceDebugValue();

					d.first = callback;
					d.second->initialised = false;
					d.second->info = info;
					d.second->originalLineNumber = lineNumber;
					d.second->value = value;

					deferredValues.add(d);
				}
			}
		}

		return;
	}

	inplaceBroadcaster.sendMessage(sendNotificationAsync, callback, lineNumber);

	for (auto v : inplaceValues)
	{
		if (v->location.getOwner() == docToUse &&
			(v->location.getLineNumber() == lineNumber || lineNumber == v->originalLineNumber))
		{
			v->value = value;
			v->info = info;
			return;
		}
	}

	auto newValue = new mcl::LanguageManager::InplaceDebugValue();
	newValue->location = CodeDocument::Position(*docToUse, lineNumber, 99);
	newValue->originalLineNumber = lineNumber;
	newValue->value = value;
	newValue->info = info;
	newValue->initialised = initialised;
	newValue->location.setPositionMaintained(true);
	
	inplaceValues.add(newValue);
}

void JavascriptProcessor::fileChanged()
{
#if USE_BACKEND

	auto p = dynamic_cast<Processor*>(this);
	auto shouldRecompile = GET_HISE_SETTING(p, HiseSettings::Scripting::RecompileOnFileChange);

	if(shouldRecompile)
		compileScript();
#endif
}




void JavascriptProcessor::clearExternalWindows()
{
	if (callbackPopups.size() != 0)
	{
		for (int i = 0; i < callbackPopups.size(); i++)
		{
			if (callbackPopups[i].getComponent() != nullptr)
			{
				callbackPopups[i]->closeButtonPressed();
			}
		}

		callbackPopups.clear();
	}
}

UriError JavascriptProcessor::compileInternal()
{
	compileProfile.setPrefix(dynamic_cast<Processor*>(this)->getId() + ".");

	auto mc = dynamic_cast<Processor*>(this)->getMainController();
	LockHelpers::freeToGo(mc);

	errorProvider.currentBlinkEvents.clear();
	errorProvider.currentBreakpoint = {};
	errorProvider.clearUriErrors(UriError::SourceType::All, sendNotificationAsync);

	Array<UriError> diagnosticErrors;

	{
		initialiseEngine();

		auto cd = createFullShadowParseData();

		shadowParse(cd, [&](const Array<UriError>& diagnostics)
		{
			diagnosticErrors.addArray(diagnostics);
		}, sendNotificationSync);

		// clear the diagnostics again, we'll send them after we know if the compilation went through fine...
		errorProvider.clearUriErrors(UriErrorLogger::SourceType::Diagnostics, sendNotificationAsync);
	}

	SUSPEND_GLOBAL_DISPATCH(mc, "compile script");
	auto sp1 = compileProfile.profile(0);

	ProcessorWithScriptingContent* thisAsScriptBaseProcessor = dynamic_cast<ProcessorWithScriptingContent*>(this);

	ScriptingApi::Content* content = thisAsScriptBaseProcessor->getScriptingContent();

	const bool saveThisContent = lastCompileWasOK && content != nullptr && !useStoredContentData;

	auto& uph = thisAsScriptBaseProcessor->getMainController_()->getUserPresetHandler();

    bool useCustomPreset = false;
    
    if(auto asJmp = dynamic_cast<JavascriptMidiProcessor*>(this))
    {
        useCustomPreset = asJmp->isFront() && uph.isUsingCustomDataModel();
    }
    
	if (saveThisContent)
	{
		if (useCustomPreset)
		{
			if (uph.isUsingPersistentObject())
			{
				thisAsScriptBaseProcessor->restoredContentValues = ValueTree("Content");

				uph.saveStateManager(thisAsScriptBaseProcessor->restoredContentValues, UserPresetIds::CustomJSON, UserPresetStateManager::StateTarget::PluginState);
			}
		}
		else
			thisAsScriptBaseProcessor->restoredContentValues = content->exportAsValueTree();
	}

	auto thisAsProcessor = dynamic_cast<Processor*>(this);

	{
		CompileDebugLock compileLock(*this);
		scriptEngine->clearDebugInformation();
		content->resetLafRegistry();
	}

	content->beginInitialization();

	setupApi();

	content = thisAsScriptBaseProcessor->getScriptingContent();

	scriptEngine->setIsInitialising(true);

	if (cycleReferenceCheckEnabled)
		scriptEngine->setUseCycleReferenceCheckForNextCompilation();

	thisAsScriptBaseProcessor->allowObjectConstructors = true;

	const static Identifier onInit("onInit");

#if HISE_INCLUDE_PROFILING_TOOLKIT
	auto enableProfiling = dynamic_cast<Processor*>(this)->getMainController()->getDebugSession().getTriggerType() == DebugSession::TriggerType::Compilation;
	scriptEngine->setEnableOnInitProfiling(enableProfiling);
#endif

	for (int i = 0; i < getNumSnippets(); i++)
	{
		getSnippet(i)->checkIfScriptActive();

		if (!getSnippet(i)->isSnippetEmpty() || !preprocessorFunctions.isEmpty())
		{
			const Identifier callbackId = getSnippet(i)->getCallbackName();

			auto codeToCompile = getSnippet(i)->getSnippetAsFunction();

			for (const auto& pf : preprocessorFunctions)
			{
				pf(callbackId, codeToCompile);
			}

			if (codeToCompile.isEmpty())
				continue;

			lastResult = scriptEngine->execute(codeToCompile, callbackId == onInit, callbackId);

			errorProvider.logUriError(UriErrorLogger::SourceType::Compilation, lastResult, sendNotificationAsync);

			if (lastResult.wasOk())
			{
				for (auto& d : diagnosticErrors)
				{
					// compilation went through => demote to warning
					if (d.errorType == UriError::ErrorType::Error)
						d.errorType = UriError::ErrorType::Warning;

					errorProvider.logUriError(UriErrorLogger::SourceType::Diagnostics, d);
				}
			}
			else
			{
				for (auto& d : diagnosticErrors)
				{
					errorProvider.logUriError(UriErrorLogger::SourceType::Diagnostics, d);
				}

				debugError(thisAsProcessor, lastResult.toConsoleMessage());

				content->endInitialization();
				scriptEngine->setIsInitialising(false);
				thisAsScriptBaseProcessor->allowObjectConstructors = false;

				// Check the rest of the snippets or they will be deleted on failed compile...
				for (int j = i; j < getNumSnippets(); j++)
				{
					getSnippet(j)->checkIfScriptActive();
				}

				lastCompileWasOK = false;

				scriptEngine->rebuildDebugInformation();
				return lastResult;
			}
		}
	}

	{
		auto sp2 = compileProfile.profile(2);
		CompileDebugLock compileLock(*this);
		scriptEngine->rebuildDebugInformation();
	}

	try
	{
		auto sp2 = compileProfile.profile(3);

		if (useCustomPreset)
		{
			// We need to reinitialise the automation ID property here because
			// it might not find the automation data before
			for (int i = 0; i < getContent()->getNumComponents(); i++)
			{
				auto sc = getContent()->getComponent(i);
				
				auto id = sc->getIdFor(ScriptComponent::Properties::automationId);
				auto idValue = sc->getScriptObjectProperty(id);

				sc->setScriptObjectPropertyWithChangeMessage(id, idValue, sendNotificationAsync);
			}

			if (uph.isUsingPersistentObject())
			{
				uph.restoreStateManager(thisAsScriptBaseProcessor->restoredContentValues, UserPresetIds::CustomJSON, UserPresetStateManager::StateTarget::PluginState);
			}
		}
		else
			content->restoreAllControlsFromPreset(thisAsScriptBaseProcessor->restoredContentValues);
	}
	catch (String& s)
	{
		debugError(thisAsProcessor, "Error at content restoring: " + s);
	}

	useStoredContentData = false; // From now on it's normal;

	content->endInitialization();

	scriptEngine->setIsInitialising(false);

	thisAsScriptBaseProcessor->allowObjectConstructors = false;

	lastCompileWasOK = true;

	if (thisAsProcessor->getMainController()->getScriptComponentEditBroadcaster()->isBeingEdited(thisAsProcessor))
	{
		debugToConsole(thisAsProcessor, "Compiled OK");
	}

	thisAsProcessor->updateParameterSlots();

	postCompileCallback();

	return {};
}

void JavascriptProcessor::initialiseEngine()
{
	if (engineInitialised)
		return;

	scriptEngine->setCallStackEnabled(callStackEnabled);
	scriptEngine->maximumExecutionTime = RelativeTime(mainController->getCompileTimeOut());

	scriptEngine->setIncludeAndScopeTree({ lastIncludeTree, lastScopeTree });
	scriptEngine->setOnInitBreakpoints(resetAttachmentAndCreateBreakpointList());

	registerApiClasses();

	scriptEngine->registerNativeObject("Globals", mainController->getGlobalVariableObject());
	scriptEngine->registerGlobalStorge(mainController->getGlobalVariableObject());

	registerCallbacks();

	engineInitialised = true;
}

void JavascriptProcessor::compileScript(const ResultFunction& rf /*= ResultFunction()*/)
{
    inplaceValues.clearQuick();

	

	bool stopAfterCompilation = false;

#if HISE_INCLUDE_PROFILING_TOOLKIT
    auto& ds = dynamic_cast<Processor*>(this)->getMainController()->getDebugSession();
	if(ds.getTriggerType() == DebugSession::TriggerType::Compilation && !ds.isRecordingMultithread())
	{
		ds.startRecording(10000.0, this);
		stopAfterCompilation = true;
	}
#endif

	clearCallableObjects();
    
	auto f = [rf, stopAfterCompilation](Processor* p)
	{
		auto jp = dynamic_cast<JavascriptProcessor*>(p);

		auto result = jp->compileInternal();

		auto postCompile = [result, rf, stopAfterCompilation](Dispatchable* obj)
		{
			auto jp = static_cast<JavascriptProcessor*>(obj);
			jp->stuffAfterCompilation(result);
			
			if(rf)
				rf(result);

			PROFILE_ONLY(if(stopAfterCompilation) dynamic_cast<Processor*>(jp)->getMainController()->getDebugSession().stopRecording());

			return Dispatchable::Status::OK;
		};

		jp->mainController->getLockFreeDispatcher().callOnMessageThreadAfterSuspension(jp, postCompile);

		return SafeFunctionCall::OK;
	};

	mainController->getJavascriptThreadPool().deactivateSleepUntilCompilation();

	mainController->getKillStateHandler().killVoicesAndCall(dynamic_cast<Processor*>(this), f, MainController::KillStateHandler::TargetThread::ScriptingThread);
}

#if USE_BACKEND

DiagnosticBase::ShadowParseData JavascriptProcessor::createFullShadowParseData()
{
	jassert(engineInitialised);

	DiagnosticBase::ShadowParseData sd;

	auto [includeTree, scope] = getScriptEngine()->parseIncludeTree();

	if (includeTree != nullptr)
	{
		Array<File> alreadyIncludedFiles;
		Array<File> newIncludedFiles;

		for (int i = 0; i < getNumWatchedFiles(); i++)
			alreadyIncludedFiles.add(getWatchedFile(i));

		includeTree->forEach([&](const UriLocationTree& t)
		{
			if (t.category == Identifier("include"))
			{
				UriLocation l;
				l.processorId = getUriProcessorId();
				l.uri = t.id.toString();

				if(l.getUriType() == UriLocation::UriType::ExternalFile)
				{
					auto mc = dynamic_cast<Processor*>(this)->getMainController();
					auto root = mc->getActiveFileHandler()->getSubDirectory(FileHandlerBase::Scripts);
					newIncludedFiles.add(l.getFile(root));
				}
			}
		
			return false;
		});

		auto changeDetected = alreadyIncludedFiles.size() != newIncludedFiles.size();

		if(!changeDetected)
		{
			for (int i = 0; i < alreadyIncludedFiles.size(); i++)
				changeDetected |= alreadyIncludedFiles[i] != newIncludedFiles[i];
		}

		if (changeDetected)
		{
			clearFileWatchers();
			
			for (const auto& nf : newIncludedFiles)
				addFileWatcher(nf);
		}

		scope->cleanup([](const UriLocationTree& v)
		{
			return v.id.isNull() && v.getNumChildren() == 0;
		});

		lastIncludeTree = includeTree;
		lastScopeTree = scope;

		sd.options.includeTree = includeTree;
		sd.options.scopeTree = scope;

		for (int i = 0; i < getNumSnippets(); i++)
		{
			UriLocation l;
			l.processorId = getUriProcessorId();
			l.uri = getSnippet(i)->getCallbackName().toString().removeCharacters("()") + "()";
			sd.codeToParse.push_back({ getSnippet(i)->getAllContent(), l });
		}
	}

	return sd;
}

void JavascriptProcessor::shadowParse(const DiagnosticBase::ShadowParseData& cd, const DiagnosticCallback& callback,
										   NotificationType notificationType)
{
	

	jassert(engineInitialised);

	if (cd.options.includeTree == nullptr)
	{
		jassertfalse;
		return;
	}
	
	if (cd.codeToParse.empty())
	{
		jassertfalse;
		return;
	}

	if (notificationType == sendNotificationSync)
	{
		PerformanceCounter pc("shadowParse", 1);
		pc.start();

		errorProvider.clearUriErrors(UriErrorLogger::SourceType::Diagnostics, sendNotificationAsync);

		Array<UriError> diagnostics;

		{
			SimpleReadWriteLock::ScopedReadLock sl(
				mainController->getJavascriptThreadPool().getLookAndFeelRenderLock());

			diagnostics = scriptEngine->shadowParse(cd);
		}

		if (diagnostics.isEmpty())
		{
			errorProvider.logUriError(UriErrorLogger::SourceType::Diagnostics, UriError::ok(), sendNotificationAsync);
		}
		else
		{
			for (const auto& d : diagnostics)
				errorProvider.logUriError(UriErrorLogger::SourceType::Diagnostics, d, sendNotificationAsync);
		}

		errorProvider.currentIncludeTree = cd.options.includeTree;
		errorProvider.logLocationTree(cd.options.scopeTree);

		if(callback)
			callback(diagnostics);

		pc.stop();
		return;
	}

	// prevent the last compilation from spamming the console.
	lastResult = UriError::ok();

	errorProvider.clearUriErrors(UriErrorLogger::SourceType::Diagnostics, sendNotificationAsync);

	// Async path (IDE F7): defer to scripting thread, callback on message thread.
	auto f = [cd, callback](Processor* p)
	{
		auto jp = dynamic_cast<JavascriptProcessor*>(p);
		auto diagnostics = jp->scriptEngine->shadowParse(cd);

		if (diagnostics.isEmpty())
		{
			jp->errorProvider.logUriError(UriErrorLogger::SourceType::Diagnostics, UriError::ok(), sendNotificationAsync);
		}
		else
		{
			for (const auto& d : diagnostics)
				jp->errorProvider.logUriError(UriErrorLogger::SourceType::Diagnostics, d, sendNotificationAsync);
		}
		
		jp->errorProvider.currentIncludeTree = cd.options.includeTree;
		jp->errorProvider.logLocationTree(cd.options.scopeTree);

		auto postParse = [diagnostics, callback](Dispatchable* obj)
		{
			if(callback)
				callback(diagnostics);

			return Dispatchable::Status::OK;
		};

		jp->mainController->getLockFreeDispatcher().callOnMessageThreadAfterSuspension(jp, postParse);

		return SafeFunctionCall::OK;
	};

	mainController->getKillStateHandler().killVoicesAndCall(
		dynamic_cast<Processor*>(this), f,
		MainController::KillStateHandler::TargetThread::ScriptingThread);
}

void JavascriptProcessor::onUriBreakpointAttachTypeChange(const UriBreakpoint& bp, AttachState newAttachState)
{
	if (newAttachState == AttachState::Stale)
		getScriptEngine()->setRuntimeBreakpoint(bp, UriBreakpointListener::ChangeType::Removed);
}

std::pair<UriBreakpoint::Owner::AttachState, UriBreakpoint> JavascriptProcessor::attachBreakpoint(const UriBreakpoint& bp)
{
	return getScriptEngine()->attachBreakpoint(bp);
}

void JavascriptProcessor::stopOnBreakpoint(const UriError& location)
{
	jassert(location.errorType | UriError::ErrorType::Breakpoint);

	auto mc = dynamic_cast<Processor*>(this)->getMainController();
	auto c = mc->getKillStateHandler().getCurrentThread();

	if (c == MainController::KillStateHandler::TargetThread::ScriptingThread ||
		c == MainController::KillStateHandler::TargetThread::SampleLoadingThread ||
		c == MainController::KillStateHandler::TargetThread::AudioThread)
	{
		auto n = Time::getMillisecondCounter();

		MessageManager::callAsync([this]()
			{
				ScopedReadLock sl(getDebugLock());
				getScriptEngine()->rebuildDebugInformation();
				rebuild();
			});

		auto& jtp = mc->getJavascriptThreadPool();

		JavascriptThreadPool::ScopedSleeper ss(jtp, this, location);

		n = Time::getMillisecondCounter() - n;
		getScriptEngine()->extendTimeout(n);
	}
	else
	{
		String message;
		message << "Breakpoint in UI Thread";// at " << id << "(Line " << lineNumber << ")";
		throw message;
	}
}
void JavascriptProcessor::resumeExecution(NextBreakpointAction n)
{
	auto& jt = dynamic_cast<Processor*>(this)->getMainController()->getJavascriptThreadPool();

	jt.resume(n);


}
void JavascriptProcessor::onUriBreakpointEvent(const UriBreakpoint& resolvedBreakpoint, const UriError& breakpointEvent)
{
	if (breakpointEvent.errorType == UriError::Blink)
	{
		bool found = false;

		for (auto& cp : errorProvider.currentBlinkEvents)
		{
			if (cp.first.matches(breakpointEvent))
			{
				cp.second++;
				found = true;
				break;
			}
		}

		if (!found)
			errorProvider.currentBlinkEvents.push_back({ breakpointEvent, 1 });
	}
	else
	{
		if (breakpointEvent.wasOk())
		{
			errorProvider.currentBreakpoint = { breakpointEvent, 0 };
		}
		else
		{
			if (breakpointEvent.matches(errorProvider.currentBreakpoint.first))
				errorProvider.currentBreakpoint.second++;
			else
				errorProvider.currentBreakpoint = { breakpointEvent, 1 };
		}
	}

	errorProvider.triggerAsyncUpdate();
}
#endif

void JavascriptProcessor::setupApi()
{
    sendClearMessage();

	inplaceValues.clearQuick();
	inplaceBroadcaster.sendMessage(sendNotificationAsync, {}, -1);

	dynamic_cast<ProcessorWithScriptingContent*>(this)->getScriptingContent()->cleanJavascriptObjects();

	auto mc =  dynamic_cast<Processor*>(this)->getMainController();

	PROFILE_ONLY(mc->getDebugSession().clearData(this));
	PROFILE_ONLY(mc->getDebugSession().sendClearMessage());
	scriptEngine = new HiseJavascriptEngine(this, mc);

	engineInitialised = false;

	initialiseEngine();
}


void JavascriptProcessor::registerCallbacks()
{
	const Processor* p = dynamic_cast<Processor*>(this);

	const double bufferTime = (double)p->getLargestBlockSize() / p->getSampleRate() * 1000.0;

	for (int i = 0; i < getNumSnippets(); i++)
	{
		scriptEngine->registerCallbackName(getSnippet(i)->getCallbackName(), getSnippet(i)->getNumArgs(), bufferTime);
	}
}

std::pair<UriLocationTree::Ptr, UriLocationTree::Ptr> JavascriptProcessor::getIncludeAndScopeTree()
{
	return { lastIncludeTree, lastScopeTree };
}


JavascriptProcessor::SnippetDocument * JavascriptProcessor::getSnippet(const Identifier& id)
{
	for (int i = 0; i < getNumSnippets(); i++)
	{
		if (getSnippet(i)->getCallbackName() == id) return getSnippet(i);
	}

	return nullptr;
}

const JavascriptProcessor::SnippetDocument * JavascriptProcessor::getSnippet(const Identifier& id) const
{
	for (int i = 0; i < getNumSnippets(); i++)
	{
		if (getSnippet(i)->getCallbackName() == id) return getSnippet(i);
	}

	return nullptr;
}

CodeDocument* JavascriptProcessor::getSnippet(const UriLocation& loc)
{
	auto uriType = loc.getUriType();

	switch (uriType)
	{
	case UriLocation::UriType::onInit:
	case UriLocation::UriType::OtherCallback:
		return getSnippet(Identifier(loc.toLocationString(UriLocation::UriFormat::CallbackId)));
	case UriLocation::UriType::ExternalFile:
	{
		auto scriptFolder = GET_PROJECT_HANDLER(dynamic_cast<Processor*>(this)).getSubDirectory(FileHandlerBase::Scripts);
		auto f = loc.getFile(scriptFolder);

		for (int i = 0; i < getNumWatchedFiles(); i++)
		{
			if (getWatchedFile(i) == f)
				return &getWatchedFileDocument(i);
		}

		jassertfalse;
		return nullptr;
	}
	default:
		jassertfalse;
		return nullptr;
	}
}

void JavascriptProcessor::saveScript(ValueTree &v) const
{
	saveComplexDataTypeAmounts(v);
	saveNetworks(v);

	String x;

	if (isConnectedToExternalFile())
	{
		x = "{EXTERNAL_SCRIPT}" + connectedFileReference;
	}
	else
	{
		mergeCallbacksToScript(x);
	}

	v.addChild(allInterfaceData.createCopy(), -1, nullptr);

	v.setProperty("Script", x, nullptr);
}

void JavascriptProcessor::restoreScript(const ValueTree &v)
{
	restoreComplexDataTypes(v);

	restoreNetworks(v);

	String x = v.getProperty("Script", String());

	auto contentPropertyChild = v.getChildWithName("ContentProperties");
	auto uiData = v.getChildWithName("UIData");

	static const Identifier deviceType("DeviceType");

	if (contentPropertyChild.isValid())
	{
		allInterfaceData = ValueTree("UIData");

		auto deviceName = HiseDeviceSimulator::getDeviceName((int)HiseDeviceSimulator::DeviceType::Desktop);

		auto copy = contentPropertyChild.createCopy();

		ScriptingApi::Content::Helpers::sanitizeNumberProperties(copy);

		copy.setProperty(deviceType, deviceName, nullptr);

		allInterfaceData.addChild(copy, -1, nullptr);

		restoreInterfaceData(copy);
	}
	if (uiData.isValid())
	{
		allInterfaceData = uiData;

		ScriptingApi::Content::Helpers::sanitizeNumberProperties(allInterfaceData);

		auto deviceIndex = (int)HiseDeviceSimulator::getDeviceType();

		setDeviceTypeForInterface(deviceIndex);
	}

	if (x.startsWith("{EXTERNAL_SCRIPT}"))
	{
		String fileReference = x.fromFirstOccurrenceOf("{EXTERNAL_SCRIPT}", false, false);

		setConnectedFile(fileReference, false);
	}
	else
	{
		parseSnippetsFromString(x, true);
	}

	if (Processor* parent = ProcessorHelpers::findParentProcessor(dynamic_cast<Processor*>(this), true))
	{
		if (parent->getMainController()->shouldSkipCompiling())
		{
			dynamic_cast<ProcessorWithScriptingContent*>(this)->restoredContentValues = v.getChildWithName("Content");
			useStoredContentData = true;
		}
		else
		{
			compileScript();
		}
	}
}

void JavascriptProcessor::setDeviceTypeForInterface(int deviceIndex)
{
	static const Identifier deviceType("DeviceType");

	auto deviceName = HiseDeviceSimulator::getDeviceName(deviceIndex);

	auto ct = allInterfaceData.getChildWithProperty(deviceType, deviceName);

	if (!ct.isValid())
		ct = allInterfaceData.getChild(0);

	jassert(ct.isValid());

	restoreInterfaceData(ct);
}



ValueTree JavascriptProcessor::getContentPropertiesForDevice(int deviceIndex)
{
	static const Identifier deviceType("DeviceType");

	auto desktopName = HiseDeviceSimulator::getDeviceName((int)HiseDeviceSimulator::DeviceType::Desktop);
	auto deviceName = HiseDeviceSimulator::getDeviceName(deviceIndex);

	auto ct = allInterfaceData.getChildWithProperty(deviceType, deviceName);

	if (!ct.isValid())
	{
		ct = allInterfaceData.getChildWithProperty(deviceType, desktopName);
	}

	jassert(ct.isValid());

	return ct;
}

bool JavascriptProcessor::hasUIDataForDeviceType(int type) const
{
	static const Identifier deviceType("DeviceType");

	auto deviceName = HiseDeviceSimulator::getDeviceName(type);

	auto ct = allInterfaceData.getChildWithProperty(deviceType, deviceName);

	return ct.isValid();
}

void JavascriptProcessor::createUICopyFromDesktop()
{
	static const Identifier deviceType("DeviceType");

	auto desktopName = HiseDeviceSimulator::getDeviceName((int)HiseDeviceSimulator::DeviceType::Desktop);
	auto deviceName = HiseDeviceSimulator::getDeviceName();

	if (desktopName == deviceName)
	{
		jassertfalse;
		return;
	}

	auto ct = allInterfaceData.getChildWithProperty(deviceType, deviceName);

	if (ct.isValid())
	{
		if (!PresetHandler::showYesNoWindow("Overwrite existing data", "There is already a UI model for this device type.\nThe current data will be merciless overwritten", PresetHandler::IconType::Warning))
		{
			return;
		}
	}
	
	ValueTree copy = allInterfaceData.getChildWithProperty(deviceType, desktopName).createCopy();

	jassert(copy.isValid());

	copy.setProperty(deviceType, deviceName, nullptr);

	allInterfaceData.addChild(copy, -1, nullptr);

	restoreInterfaceData(copy);
}



void JavascriptProcessor::restoreInterfaceData(ValueTree propertyData)
{
	auto buildComponents = !mainController->shouldSkipCompiling();

	auto r = dynamic_cast<ProcessorWithScriptingContent*>(this)->getScriptingContent()->createComponentsFromValueTree(propertyData, buildComponents);

	int numComponents = 0;

	valuetree::Helpers::forEach(propertyData, [&numComponents](ValueTree& v)
	{
		if(v.getType() == Identifier("Component"))
			numComponents++;

		return false;
	});

	dynamic_cast<Processor*>(this)->updateParameterSlots(numComponents);

	if (r.failed())
	{
		debugError(dynamic_cast<Processor*>(this), r.getErrorMessage());
	}
}

String JavascriptProcessor::Helpers::resolveIncludeStatements(String& x, Array<File>& includedFiles, const JavascriptProcessor* p)
{
	String regex("include\\(\"(?:\\{GLOBAL_SCRIPT_FOLDER\\})?([/\\w\\s]+\\.\\w+)\"\\);");
	StringArray results = RegexFunctions::search(regex, x, 0);
	StringArray fileNames = RegexFunctions::search(regex, x, 1);
	StringArray includedContents;

	File globalScriptFolder = PresetHandler::getGlobalScriptFolder(dynamic_cast<Processor*>(const_cast<JavascriptProcessor*>(p)));

	static const String globalWildcard("{GLOBAL_SCRIPT_FOLDER}");

	for (int i = 0; i < fileNames.size(); i++)
	{				
		File f;
		
		auto isGlobalScript = results[i].contains(globalWildcard);

		if (isGlobalScript)
		{
			f = globalScriptFolder.getChildFile(fileNames[i]).getFullPathName();
		}
		else
		{
			f = File::isAbsolutePath(fileNames[i]) ? File(fileNames[i]) :
				GET_PROJECT_HANDLER(dynamic_cast<const Processor*>(p)).getSubDirectory(ProjectHandler::SubDirectories::Scripts).getChildFile(fileNames[i]);	
		}

		if (includedFiles.contains(f)) // skip multiple inclusions...
		{
			includedContents.add("");
			continue;
		}

		includedFiles.add(f);
		String content = f.loadFileAsString();

		content = resolveIncludeStatements(content, includedFiles, p);

		String thisContent;

		String fileRef;

		if (isGlobalScript)
			fileRef << globalWildcard;

		fileRef << fileNames[i];

		thisContent << "\n//{BEGIN}" + fileRef << "\n";
		thisContent << content;
		thisContent << "\n//{END}" + fileRef << "\n";

		includedContents.add(thisContent);
	}

	for (int i = 0; i < results.size(); i++)
	{
		x = x.replace(results[i], includedContents[i]);
	}

	return x;
}


Array<hise::JavascriptProcessor::Helpers::ExternalScript> JavascriptProcessor::Helpers::desolveIncludeStatements(String& x, const File& scriptRoot, MainController* mc)
{
	Array<ExternalScript> scripts;

	auto lines = StringArray::fromLines(x);
	String nl = "\n";

	String newCode;

	static String beginWildcard = "//{BEGIN}";
	static String endWildcard = "//{END}";

	bool somethingChanged = false;

	File globalScriptFolder = PresetHandler::getGlobalScriptFolder(mc->getMainSynthChain());

	static const String globalWildcard("{GLOBAL_SCRIPT_FOLDER}");

	for (int i = 0; i < lines.size(); i++)
	{
		String currentLine = lines[i];

		if (lines[i].startsWith(beginWildcard))
		{
			somethingChanged = true;
			auto j = i+1;

			String fileReference = lines[i].fromFirstOccurrenceOf(beginWildcard, false, false);

			auto thisEnd = endWildcard + fileReference;

			ExternalScript es;

			if (fileReference.contains(globalWildcard))
				es.f = globalScriptFolder.getChildFile(fileReference.fromFirstOccurrenceOf(globalWildcard, false, false));
			else
				es.f = scriptRoot.getChildFile(fileReference);

			while (j < lines.size())
			{
				if (lines[j].startsWith(thisEnd))
					break;

				es.content << lines[j] << nl;

				j++;
			}

			newCode << "include(" << fileReference.quoted() << ");" << nl;

			i = j;

			auto nested = desolveIncludeStatements(es.content, scriptRoot, mc);

			if(!nested.isEmpty())
				scripts.addArray(nested);

			scripts.add(es);
		}
		else
		{
			newCode << lines[i] << nl;
		}
	}

	if (somethingChanged)
		x = newCode;

	return scripts;
}

String JavascriptProcessor::collectScript(bool silent) const
{
	String x;
	mergeCallbacksToScript(x, NewLine::getDefault());

	int counter = 0;

	Array<File> includedFiles;
	String everything = Helpers::resolveIncludeStatements(x, includedFiles, this);

	return everything;
}



String JavascriptProcessor::getBase64CompressedScript(bool silent) const
{
	auto stripped = collectScript(silent);

	MemoryOutputStream mos;
	GZIPCompressorOutputStream gos(&mos, 9);
	gos.writeString(stripped);
	gos.flush();

	return mos.getMemoryBlock().toBase64Encoding();
}

bool JavascriptProcessor::restoreBase64CompressedScript(const String &base64compressedScript)
{
	MemoryBlock mb;

	mb.fromBase64Encoding(base64compressedScript);

	MemoryInputStream mis(mb, false);

	GZIPDecompressorInputStream gis(mis);

	String script = gis.readEntireStreamAsString();

	return parseSnippetsFromString(script);
}

void JavascriptProcessor::setConnectedFile(const String& fileReference, bool compileScriptAfterLoad/*=true*/)
{
	if (fileReference.isNotEmpty())
	{
		connectedFileReference = fileReference;

#if USE_BACKEND

	File f = File();
	
	if (fileReference.contains("{GLOBAL_SCRIPT_FOLDER}"))
	{
		File globalScriptFolder = PresetHandler::getGlobalScriptFolder(dynamic_cast<Processor*>(this));
		const String filePath = fileReference.fromFirstOccurrenceOf("{GLOBAL_SCRIPT_FOLDER}", false, false);
		f = globalScriptFolder.getChildFile(filePath);
	}
	else
	{
		f = GET_PROJECT_HANDLER(dynamic_cast<const Processor*>(this)).getFilePath(fileReference, ProjectHandler::SubDirectories::Scripts);
	}
	
	const String code = f.loadFileAsString();
#else
	const String code = dynamic_cast<Processor*>(this)->getMainController()->getExternalScriptFromCollection(fileReference);

#endif

		if (fileReference.endsWith(".cjs"))
		{
			restoreBase64CompressedScript(code);
		}
		else
		{
			parseSnippetsFromString(code, true);
		}

		if(compileScriptAfterLoad)
			compileScript();

		dynamic_cast<Processor*>(this)->sendOtherChangeMessage(dispatch::library::ProcessorChangeEvent::Custom);
	}
}

void JavascriptProcessor::disconnectFromFile()
{
	connectedFileReference = String();

	dynamic_cast<Processor*>(this)->sendOtherChangeMessage(dispatch::library::ProcessorChangeEvent::Custom);
}

void JavascriptProcessor::reloadFromFile()
{
	if (isConnectedToExternalFile())
	{
		setConnectedFile(connectedFileReference);
	}
}

void JavascriptProcessor::mergeCallbacksToScript(String &x, const String& sepString/*=String()*/) const
{
	for (int i = 0; i < getNumSnippets(); i++)
	{
		const SnippetDocument *s = getSnippet(i);

		auto code = s->getSnippetAsFunction();

		for (const auto& pf : preprocessorFunctions)
			pf(s->getCallbackName(), code);

        if(usePreprocessorAtMerge)
        {
            if(auto e = const_cast<JavascriptProcessor*>(this)->getScriptEngine())
            {
                auto ok = e->preprocessor->process(code, s->getCallbackName().toString());

				if(!ok.wasOk())
					throw ok;

                jassert(ok.wasOk());
            }
        }
            
		x << code << sepString;
	}
}

Result JavascriptProcessor::MergedCallbackDocument::parse(const JavascriptProcessor& processor, const String& newText)
{
	const int numSnippets = processor.getNumSnippets();
	if (numSnippets == 0)
		return Result::fail("The processor has no callback snippets");

	Array<int> newStarts;
	Array<String> newSnippets;
	for (int i = 0; i < numSnippets; ++i)
	{
		newStarts.add(0);
		newSnippets.add(String());
	}

	// Match the legacy reverse split: each callback starts at the last marker
	// in the remaining prefix. Validate every split before mutating a document.
	String remaining = newText;
	for (int i = numSnippets - 1; i > 0; --i)
	{
		const auto marker = "function " + processor.getSnippet(i)->getCallbackName().toString() + "(";
		const int start = remaining.lastIndexOf(marker);
		if (start < 0)
			return Result::fail("The callback " + marker + " is not defined.");

		newStarts.set(i, start);
		newSnippets.set(i, remaining.substring(start));
		remaining = remaining.substring(0, start);
	}

	newSnippets.set(0, remaining);
	text = newText;
	starts.swapWith(newStarts);
	snippets.swapWith(newSnippets);
	return Result::ok();
}

#if USE_BACKEND
int JavascriptProcessor::MergedCallbackDocument::getSnippetIndex(int wholeFileIndex) const
{
	if (starts.isEmpty() || wholeFileIndex < 0 || wholeFileIndex > text.length())
		return -1;

	for (int i = starts.size(); --i >= 0;)
		if (wholeFileIndex >= starts[i])
			return i;

	return -1;
}

int JavascriptProcessor::MergedCallbackDocument::toSnippetIndex(int snippetIndex, int wholeFileIndex) const
{
	if (!isPositiveAndBelow(snippetIndex, starts.size()) || getSnippetIndex(wholeFileIndex) != snippetIndex)
		return -1;

	return wholeFileIndex - starts[snippetIndex];
}

int JavascriptProcessor::MergedCallbackDocument::toFileIndex(int snippetIndex, int snippetCharIndex) const
{
	if (!isPositiveAndBelow(snippetIndex, snippets.size())
		|| snippetCharIndex < 0 || snippetCharIndex > snippets[snippetIndex].length())
		return -1;

	return starts[snippetIndex] + snippetCharIndex;
}

int JavascriptProcessor::MergedCallbackDocument::toCharacterIndex(const String& content, int line, int utf16Column)
{
	if (line < 0 || utf16Column < 0)
		return -1;

	int currentLine = 0;
	int column = 0;
	const int length = content.length();
	auto cursor = content.getCharPointer();

	for (int i = 0; i < length; ++i, ++cursor)
	{
		const auto c = *cursor;

		if (currentLine == line)
		{
			if (column == utf16Column)
				return i;

			if (c == '\n' || c == '\r')
				return -1;

			column += c > 0xffff ? 2 : 1;
			if (column > utf16Column)
				return -1; // Do not split a surrogate pair.
		}

		if (c == '\r' || c == '\n')
		{
			++currentLine;
			column = 0;
			if (c == '\r' && i + 1 < length && cursor[1] == '\n')
			{
				++i;
				++cursor;
			}
		}
	}

	return currentLine == line && column == utf16Column ? length : -1;
}

std::pair<int, int> JavascriptProcessor::MergedCallbackDocument::toLspPosition(
	const String& content, int characterIndex)
{
	if (characterIndex < 0 || characterIndex > content.length())
		return { -1, -1 };

	int line = 0;
	int column = 0;
	auto cursor = content.getCharPointer();

	for (int i = 0; i < characterIndex; ++i, ++cursor)
	{
		const auto c = *cursor;
		if (c == '\r' || c == '\n')
		{
			if (c == '\r' && i + 1 < characterIndex && cursor[1] == '\n')
			{
				++i;
				++cursor;
			}
			++line;
			column = 0;
		}
		else
			column += c > 0xffff ? 2 : 1;
	}

	return { line, column };
}

Result JavascriptProcessor::applyMergedCallbackDocument(const String& text)
{
	jassert(MessageManager::getInstance()->currentThreadHasLockedMessageManager());

	MergedCallbackDocument next;
	auto valid = next.parse(*this, text);
	if (valid.failed())
		return valid;

	for (int i = 0; i < next.snippets.size(); ++i)
		getSnippet(i)->replaceContentAsync(next.snippets[i], false);

	mergedCallbackDocument = std::move(next);
	return Result::ok();
}

bool JavascriptProcessor::ensureMergedCallbackMapping()
{
	if (mergedCallbackReloader.lspOpen && !mergedCallbackDocument.starts.isEmpty())
		return true; // The accepted editor buffer, not disk, owns the open file.

	String current;
	for (int i = 0; i < getNumSnippets(); ++i)
		current << getSnippet(i)->getAllContent();

	if (mergedCallbackDocument.text == current && !mergedCallbackDocument.starts.isEmpty())
		return true;

	const auto file = getMergedCallbackFile();
	if (!file.existsAsFile())
		return false;

	MergedCallbackDocument candidate;
	if (candidate.parse(*this, file.loadFileAsString()).failed()
		|| candidate.snippets.size() != getNumSnippets())
		return false;

	for (int i = 0; i < candidate.snippets.size(); ++i)
		if (candidate.snippets[i] != getSnippet(i)->getAllContent())
			return false;

	mergedCallbackDocument = std::move(candidate);
	return true;
}

File JavascriptProcessor::getMergedCallbackFile() const
{
	const auto* p = dynamic_cast<const Processor*>(this);
	if (connectedFileReference.isNotEmpty())
	{
		if (connectedFileReference.endsWithIgnoreCase(".cjs"))
			return {};

		if (connectedFileReference.contains("{GLOBAL_SCRIPT_FOLDER}"))
		{
			const auto folder = PresetHandler::getGlobalScriptFolder(const_cast<Processor*>(p));
			const auto relative = connectedFileReference.fromFirstOccurrenceOf("{GLOBAL_SCRIPT_FOLDER}", false, false);
			return folder.getChildFile(relative);
		}

		return GET_PROJECT_HANDLER(p).getFilePath(connectedFileReference, ProjectHandler::SubDirectories::Scripts);
	}

	const auto scripts = GET_PROJECT_HANDLER(p).getSubDirectory(ProjectHandler::SubDirectories::Scripts);
	const auto chainId = mainController->getMainSynthChain()->getId().removeCharacters(" .()");
	const auto processorId = p->getId().removeCharacters(" .()");
	return scripts.getChildFile("ScriptProcessors/" + chainId + "/" + processorId + ".js");
}

JavascriptProcessor::MergedCallbackReloader::MergedCallbackReloader(JavascriptProcessor& owner_) : owner(owner_)
{
	startTimer(3000);
}

void JavascriptProcessor::MergedCallbackReloader::timerCallback()
{
	if (lspOpen)
		return;

	const auto file = owner.getMergedCallbackFile();
	if (file != lastFile)
	{
		lastFile = file;
		lastDiskText.clear();
		hasBaseline = false;
		sawMissingFile = false;
	}

	if (!file.existsAsFile())
	{
		sawMissingFile = true;
		return;
	}

	const auto diskText = file.loadFileAsString();
	if (!hasBaseline)
	{
		hasBaseline = true;
		if (!sawMissingFile)
		{
			lastDiskText = diskText;
			return;
		}
	}

	if (diskText == lastDiskText)
		return;

	lastDiskText = diskText;
	String currentSnippets;
	for (int i = 0; i < owner.getNumSnippets(); ++i)
		currentSnippets << owner.getSnippet(i)->getAllContent();

	if (diskText == currentSnippets)
		return; // XML export of the current snippets is not an external edit.

	const auto result = owner.applyMergedCallbackDocument(diskText);
	if (result.failed())
	{
		debugError(dynamic_cast<Processor*>(&owner), result.getErrorMessage());
		return;
	}

	if (owner.engineInitialised)
	{
		auto data = owner.createFullShadowParseData();
		if (data.options.includeTree != nullptr && !data.codeToParse.empty())
			owner.shadowParse(data, [](const JavascriptProcessor::DiagnosticList&) {}, sendNotificationSync);
	}
}

void JavascriptProcessor::setMergedCallbackFileOpen(bool isOpen)
{
	jassert(MessageManager::getInstance()->currentThreadHasLockedMessageManager());
	if (isOpen && !mergedCallbackReloader.hasBaseline)
	{
		mergedCallbackReloader.lastFile = getMergedCallbackFile();
		mergedCallbackReloader.sawMissingFile = !mergedCallbackReloader.lastFile.existsAsFile();
		if (!mergedCallbackReloader.sawMissingFile)
		{
			mergedCallbackReloader.lastDiskText = mergedCallbackReloader.lastFile.loadFileAsString();
			mergedCallbackReloader.hasBaseline = true;
		}
	}

	mergedCallbackReloader.lspOpen = isOpen;
	if (!isOpen)
		mergedCallbackReloader.timerCallback();
}
#endif

bool JavascriptProcessor::parseSnippetsFromString(const String& x, bool clearUndoHistory /*= false*/)
{
	MergedCallbackDocument parsed;
	const auto result = parsed.parse(*this, x);
	if (result.failed())
	{
		if (MessageManager::getInstance()->isThisTheMessageThread())
			PresetHandler::showMessageWindow("Invalid script",
				"The script you are trying to load is not a valid HISE script file.\n" + result.getErrorMessage(),
				PresetHandler::IconType::Error);

		debugError(dynamic_cast<Processor*>(this), result.getErrorMessage());
		return false;
	}

	for (int i = getNumSnippets() - 1; i > 0; --i)
		getSnippet(i)->replaceContentAsync(parsed.snippets[i], !clearUndoHistory);

	// Preserve the legacy onInit notification behavior.
	getSnippet(0)->replaceContentAsync(parsed.snippets[0]);
	return true;
}


void JavascriptProcessor::compileScriptWithCycleReferenceCheckEnabled()
{
	ScopedValueSetter<bool> ss(cycleReferenceCheckEnabled, true);
	compileScript();
}

void JavascriptProcessor::stuffAfterCompilation(const UriError& result)
{
	mainController->getScriptComponentEditBroadcaster()->clearSelection(sendNotification);

	if (lastCompileWasOK && lastOptimisationReport.isNotEmpty())
	{
		debugToConsole(dynamic_cast<Processor*>(this), lastOptimisationReport);

		String x;
		mergeCallbacksToScript(x);
	}

	mainController->checkAndAbortMessageThreadOperation();

#if USE_BACKEND
	if (isConnectedToExternalFile())
	{
		auto shouldSave = (bool)GET_HISE_SETTING(dynamic_cast<Processor*>(this), HiseSettings::Scripting::SaveConnectedFilesOnCompile);

		if (shouldSave)
		{
			String x;
			mergeCallbacksToScript(x);
		
			const File f = GET_PROJECT_HANDLER(dynamic_cast<const Processor*>(this)).getFilePath(connectedFileReference, ProjectHandler::SubDirectories::Scripts);

			f.replaceWithText(x);
		}
	}
#endif
	
	for(auto & d: deferredValues)
	{
		for (int i = 0; i < getNumWatchedFiles(); i++)
		{
			if (getWatchedFile(i).getFileName() == d.first)
			{
				d.second->location = CodeDocument::Position(getWatchedFileDocument(i), d.second->originalLineNumber, 90);
				d.second->init();

				inplaceBroadcaster.sendMessage(sendNotificationAsync, Identifier(d.first), d.second->originalLineNumber);
				inplaceValues.add(d.second);
			}
		}
	}

	deferredValues.clear();

	mainController->sendScriptCompileMessage(this);
	rebuild();
}

JavascriptProcessor::SnippetDocument::SnippetDocument(const Identifier &callbackName_, const String &parameters_) :
CodeDocument(),
isActive(false),
callbackName(callbackName_),
notifier(*this)
{
	parameters = StringArray::fromTokens(parameters_, " ", "");
	numArgs = parameters.size();
	
	if (callbackName != Identifier("onInit"))
	{
		emptyText << "function " << callbackName_.toString() << "(";
		
		for (int i = 0; i < numArgs; i++)
		{
			emptyText << parameters[i];
			if (i != numArgs - 1) emptyText << ", ";
		}
		
		emptyText << ")\n";
		emptyText << "{\n";
		emptyText << "\t\n";
		emptyText << "}\n";
	};

	emptyText << " ";
	
	setDisableUndo(true);

	replaceAllContent(emptyText);
    
	setDisableUndo(false);
}

void JavascriptProcessor::SnippetDocument::checkIfScriptActive()
{
	isActive = true;
    auto text = getSnippetAsFunction();
    
	if (!text.containsNonWhitespaceChars()) isActive = false;

	String trimmedText = text.removeCharacters(" \t\n\r");

	String trimmedEmptyText = emptyText.removeCharacters(" \t\n\r");

	if (trimmedEmptyText == trimmedText)	isActive = false;
}

String JavascriptProcessor::SnippetDocument::getSnippetAsFunction() const
{
	SpinLock::ScopedLockType sl(pendingLock);

	if (isSnippetEmpty()) return emptyText;
	else if (pendingNewContent.isNotEmpty()) return pendingNewContent;
	else				  return getAllContent();
}


JavascriptThreadPool::JavascriptThreadPool(MainController* mc) :
	Thread("Javascript Thread", HISE_DEFAULT_STACK_SIZE),
	ControlledObject(mc),
	ProfiledRecordingSession(mc->getDebugSession(), DebugSession::ThreadIdentifier::Type::ScriptingThread),
	lowPriorityQueue(8192),
	highPriorityQueue(2048),
	compilationQueue(128),
	deferredPanels(1024),
#if USE_BACKEND
    replQueue(128),
#endif
	globalServer(new GlobalServer(mc))
{
	memset(numTasks, 0, sizeof(numTasks));
	taskNames[Task::Compilation] = "Compilation Count";
	taskNames[Task::ReplEvaluation] = "REPL Evaluation Counter";
	taskNames[Task::HiPriorityCallbackExecution] = "Hi Priority Callback Counter";
	taskNames[Task::LowPriorityCallbackExecution] = "Low Priority Callback Counter";
	taskNames[Task::DeferredPanelRepaintJob] = "Deferred Paint Routine Counter";

	scriptThreadData.setPrefix("");
	scriptThreadData.setColour(Colour(0xFF666666));
	scriptThreadData.setDurationThreshold(0.05);
	scriptThreadData.setSourceType(DebugSession::ProfileDataSource::SourceType::ScriptCallback);
	scriptThreadData.setHolder(&mc->getDebugSession(), true);


	pCompile = scriptThreadData.add("Compilation");
	pHigh = scriptThreadData.add("High Priority Callbacks");
	pLow = scriptThreadData.add("Low Priority Callbacks");
	pRepaint = scriptThreadData.add("Deferred Paint Routines");

}

JavascriptThreadPool::~JavascriptThreadPool()
{
	globalServer = nullptr;
	stopThread(1000);
}

void JavascriptThreadPool::cancelAllJobs(bool shouldStopThread)
{
	if (isSleeping)
	{
		shouldWakeUp = true;
		LockHelpers::waitUntil(getMainController(), [this]() { return this->isSleeping == false; });
	}
		

	LockHelpers::SafeLock ss(getMainController(), LockHelpers::Type::ScriptLock);

	if(shouldStopThread)
		stopThread(1000);

	compilationQueue.clear();
	lowPriorityQueue.clear();
	highPriorityQueue.clear();
#if USE_BACKEND
	replQueue.clear();
#endif
	deferredPanels.clear();
}

JavascriptThreadPool::Task::Task() noexcept:
	type(Free),
	f(),
	jp(nullptr)
{}

JavascriptThreadPool::Task::Task(Type t, JavascriptProcessor* jp_, const Function& functionToExecute) noexcept:
	type(t),
	f(functionToExecute),
	jp(jp_)
{}

JavascriptProcessor* JavascriptThreadPool::Task::getProcessor() const noexcept
{ return jp.get(); }

JavascriptThreadPool::Task::Type JavascriptThreadPool::Task::getType() const noexcept
{ return type; }

bool JavascriptThreadPool::Task::isValid() const noexcept
{ return (bool)f; }

bool JavascriptThreadPool::Task::isHiPriority() const noexcept
{ return type == Compilation || type == HiPriorityCallbackExecution; }

const CriticalSection& JavascriptThreadPool::getLock() const noexcept
{ return scriptLock; }

bool JavascriptThreadPool::isBusy() const noexcept
{ return busy; }

JavascriptThreadPool::Task::Type JavascriptThreadPool::getCurrentTask() const noexcept
{ return currentType; }

SimpleReadWriteLock& JavascriptThreadPool::getLookAndFeelRenderLock()
{
	return lookAndFeelRenderLock;
}

GlobalServer* JavascriptThreadPool::getGlobalServer()
{ return globalServer.get(); }

void JavascriptThreadPool::resume(UriBreakpoint::Owner::NextBreakpointAction stepToNext_)
{
	stepToNext = stepToNext_;
	shouldWakeUp = true;

	notify();
}

void JavascriptThreadPool::deactivateSleepUntilCompilation()
{
	if (isSleeping)
	{
		resume(UriBreakpoint::Owner::NextBreakpointAction::Resume);
		auto mm = MessageManager::getInstanceWithoutCreating();

		if (mm->isThisTheMessageThread())
		{
			while (isSleeping)
			{
				mm->runDispatchLoopUntil(10);
			}
		}
	}

	allowSleep = false;
}

JavascriptThreadPool::ScopedSleeper::ScopedSleeper(JavascriptThreadPool& p_, JavascriptProcessor* jp, const UriError& loc):
	p(p_),
	wasSleeping(p.isSleeping),
	location(loc),
	prevJp(p.currentSleeper)
{
	p.currentSleeper = jp;

	if (!p.allowSleep)
	{
		sendMessage(false);
		return;
	}
		

	sendMessage(true);

	p.isSleeping = true;
	p.shouldWakeUp = false;

	auto cThread = Thread::getCurrentThread();
	std::function<bool()> shouldExit;

	if (cThread != nullptr)
		shouldExit = [cThread](){ return cThread->threadShouldExit(); };
	else
	{
		auto currentThread = p.getMainController()->getKillStateHandler().getCurrentThread();

		if (currentThread == MainController::KillStateHandler::TargetThread::AudioThread)
		{
			auto mc = p.getMainController();
			shouldExit = [mc]() { return mc->getKillStateHandler().hasRequestedQuit(); };
		}
	}

	jassert(shouldExit);

	if (!shouldExit)
		shouldExit = []() { return true; };

	SimpleReadWriteLock::ScopedDisabler sd(p.getLookAndFeelRenderLock());

	auto currentThread = p.getMainController()->getKillStateHandler().getCurrentThread();

	while (p.allowSleep && !p.shouldWakeUp && !shouldExit())
	{
		PendingCompilationList l;
		auto r = p.executeQueue(Task::ReplEvaluation, l);

		if (currentThread == MainController::KillStateHandler::TargetThread::ScriptingThread)
		{
			p.wait(200);
		}
		else
		{
			Thread::sleep(200);
		}
	}

	sendMessage(false);
}

void JavascriptThreadPool::ScopedSleeper::sendMessage(bool on)
{
	auto jp = p.currentSleeper;

	if (messageSent || jp == nullptr)
		return;

	jassert(jp != nullptr);

	if (on)
	{
		jp->sendUriBreakpointEvent(location);
		
		if (p.customOnBreakFunction)
			p.customOnBreakFunction();
	}
		
	else
	{
		jp->sendUriBreakpointEvent(UriError::ok());
		jp->getScriptEngine()->stopAtNextStatement(p.stepToNext);
		p.stepToNext = UriBreakpoint::Owner::NextBreakpointAction::Resume;
	}

	JavascriptThreadPool* p_ = &p;
	auto l = location;

	auto f = [p_, l, on]()
	{
		for (const auto& s : p_->sleepListeners)
		{
			if (s != nullptr)
			{
				if (s.get()->sleepStateChanged(l, on))
					break;
			}
				
		}
	};

	MessageManager::callAsync(f);

	if(!on)
		messageSent = true;
}

JavascriptThreadPool::ScopedSleeper::~ScopedSleeper()
{
	p.isSleeping = wasSleeping;
	p.currentSleeper = prevJp;
	// These are deactivated so that other events are added to the queue, time will tell if that's a good idea...
	//p.lowPriorityQueue.clear();
	//p.highPriorityQueue.clear();
			
	sendMessage(false);
}

void JavascriptThreadPool::addSleepListener(SleepListener* s)
{
	sleepListeners.addIfNotAlreadyThere(s);
}

void JavascriptThreadPool::removeSleepListener(SleepListener* s)
{
	sleepListeners.removeAllInstancesOf(s);
}

bool JavascriptThreadPool::isCurrentlySleeping() const
{ return isSleeping; }

void JavascriptThreadPool::addJob(Task::Type t, JavascriptProcessor* p, const Task::Function& f)
{
	WARN_IF_AUDIO_THREAD(true, IllegalAudioThreadOps::StringCreation);

	auto currentThread = getMainController()->getKillStateHandler().getCurrentThread();

	//if (t != Task::Type::Compilation && t != Task::Type::ReplEvaluation && isSleeping)
	//	return;

	switch (currentThread)
	{
	case MainController::KillStateHandler::TargetThread::SampleLoadingThread:
	{
		jassert(!getMainController()->getKillStateHandler().isAudioRunning());
		
		if (t == Task::Type::LowPriorityCallbackExecution)
		{
			pushToQueue(t, p, f);
		}
		else
		{

			// We're calling a high-priority task from the sample loading thread.
			// This can be executed synchronously

			auto r = executeNow(t, p, f);

			if (r.failed())
				getMainController()->getConsoleHandler().writeToConsole(r.toConsoleMessage(), 1, dynamic_cast<Processor*>(p), Colours::blue);
		}

		break;
	}
	case MainController::KillStateHandler::TargetThread::ScriptingThread:
	{
		jassert(isBusy());
		
        if (t == currentType)
		{
			// Same priority, just run it
			executeNow(t, p, f);
		}
		else
		{
			if (t == Task::Type::LowPriorityCallbackExecution)
			{
				// We're calling a low priority task from a high priority task
				// In this case, we defer the task to later (it's just a timer
				// callback or a repaint function).
				pushToQueue(t, p, f);
			}
			else
			{
				// We're calling a high priority task from a low priority
				// callback, so we need to execute it synchronously.
				executeNow(t, p, f);
			}
		}

		break;
	}
	case MainController::KillStateHandler::TargetThread::MessageThread:
	{
		if (getMainController()->isInitialised() && !getMainController()->isFlakyThreadingAllowed())
		{
			pushToQueue(t, p, f);
			notifyIfNotInBreakpoint();
		}
		else
		{
			executeNow(t, p, f);
		}
		break;
	}
	case MainController::KillStateHandler::TargetThread::AudioThread:
	{
		pushToQueue(t, p, f);
		break;
	}
    default:
		// We're calling any task from an unspecified thread (eg. server download thread).
		pushToQueue(t, p, f);
        break;
	};
}


void JavascriptThreadPool::addDeferredPaintJob(ScriptingApi::Content::ScriptPanel* sp)
{
	bumpCounter(Task::DeferredPanelRepaintJob);
	
	WeakReference<ScriptingApi::Content::ScriptPanel> spWeak(sp);
	deferredPanels.push(std::move(spWeak));
}

UriError JavascriptThreadPool::executeQueue(const Task::Type& t, PendingCompilationList& pendingCompilations)
{
	UriError r = UriError::ok();

	auto alreadyCompiled = [pendingCompilations](const CallbackTask& t)
	{
		return pendingCompilations.contains(t.getFunction().getProcessor());
	};

	switch (t)
	{
	case Task::Type::Compilation:
	{
		CompilationTask ct;
		
		allowSleep = true;

		TRACE_EVENT("scripting", "compile queue");//, perfetto::Track(CompilationTrackId));
		auto sp = scriptThreadData.profile(pCompile);

		while (compilationQueue.pop(ct))
		{
            SimpleReadWriteLock::ScopedWriteLock sl(getLookAndFeelRenderLock());
			SuspendHelpers::ScopedTicket ticket;

			clearCounter(Task::LowPriorityCallbackExecution);
			clearCounter(Task::HiPriorityCallbackExecution);
			clearCounter(Task::DeferredPanelRepaintJob);
#if USE_BACKEND
			clearCounter(Task::ReplEvaluation);
#endif

			lowPriorityQueue.clear();
			highPriorityQueue.clear();
#if USE_BACKEND
			replQueue.clear();
#endif

#if PERFETTO
			dispatch::StringBuilder b;
			b << "compile " << dynamic_cast<Processor*>(ct.getFunction().getProcessor())->getId();
			TRACE_DYNAMIC_SCRIPTING(b);
#endif

			killVoicesAndExtendTimeOut(ct.getFunction().getProcessor());

			r = ct.call();

			pendingCompilations.addIfNotAlreadyThere(ct.getFunction().getProcessor());
		}

		clearCounter(t);
		

		return r;
	}
    case Task::ReplEvaluation:
    {
        r = executeQueue(Task::Compilation, pendingCompilations);
        
#if USE_BACKEND
        CallbackTask hpt;

        while (r.wasOk() && replQueue.pop(hpt))
        {
            r = hpt.call();
        }
#endif
		clearCounter(t);

        return r;
    }
	case Task::HiPriorityCallbackExecution:
	{
		r = executeQueue(Task::ReplEvaluation, pendingCompilations);

		TRACE_EVENT("scripting", "high priority queue");//, perfetto::Track(HighPriorityTrackId));
		auto sp = scriptThreadData.profile(pHigh);

		CallbackTask hpt;

		while (r.wasOk() && highPriorityQueue.pop(hpt))
		{
			jassert(hpt.getFunction().isHiPriority());

			if (alreadyCompiled(hpt))
				continue;

#if PERFETTO
			dispatch::StringBuilder b;

			if(auto p = dynamic_cast<Processor*>(hpt.getFunction().getProcessor()))
			{
				b << "hi priority callback " << p->getId();
			}
			
			TRACE_DYNAMIC_SCRIPTING(b);
#endif

			r = hpt.call();

			if (!r.wasOk())
			{
				auto jp = hpt.getFunction().getProcessor();
				LOG_RUNTIME_ERROR(r, jp);
			}
		}

		clearCounter(t);

		if (!r.wasOk())
		{
			lowPriorityQueue.clear();
			clearCounter(Task::LowPriorityCallbackExecution);
		}
			

		return r;
	}
	case Task::LowPriorityCallbackExecution:
	{
		r = executeQueue(Task::HiPriorityCallbackExecution, pendingCompilations);

		CallbackTask lpt;

#if PERFETTO
		auto tr = perfetto::Track(LowPriorityTrackId);
		PerfettoHelpers::setTrackNameName(tr, "low priority queue");
#endif

		{
			TRACE_EVENT_BEGIN("scripting", "low priority queue");//, t);
			auto sp = scriptThreadData.profile(pLow);

			while (r.wasOk() && lowPriorityQueue.pop(lpt))
			{
	            // We're trying to leave this unlocked here as the
	            // localised inline function scope might resolve all
	            // multithreading issues (???)
				//SimpleReadWriteLock::ScopedWriteLock sl(getLookAndFeelRenderLock());

				jassert(!lpt.getFunction().isHiPriority());

				if (alreadyCompiled(lpt))
					continue;

	#if PERFETTO
				dispatch::StringBuilder b;
				b << "low priority callback " << dynamic_cast<Processor*>(lpt.getFunction().getProcessor())->getId();
				TRACE_DYNAMIC_SCRIPTING(b);
	#endif

				r = lpt.call();

				if (!r.wasOk())
				{
					auto jp = lpt.getFunction().getProcessor();
					LOG_RUNTIME_ERROR(r, jp);
				}
			}

			TRACE_EVENT_END("scripting");//, t);
		}
		

		if (!r.wasOk())
			lowPriorityQueue.clear();

		clearCounter(t);

		WeakReference<ScriptingApi::Content::ScriptPanel> sp;

		auto sp2 = scriptThreadData.profile(pRepaint);

		if (r.wasOk())
		{
			while (deferredPanels.pop(sp))
			{
				ScopedValueSetter<bool> svs(busy, true);

				if (sp.get() != nullptr)
				{
#if PERFETTO
					dispatch::StringBuilder b;
					b << "repaint panel " << sp->getName();
					TRACE_DYNAMIC_SCRIPTING(b);
#endif

					sp->repaint();
				}
					
			}
		}
		else
		{
			deferredPanels.clear();
		}

		clearCounter(Task::DeferredPanelRepaintJob);

		return r;
	}
	default:
		jassertfalse;
	}

	return r;
}



void JavascriptThreadPool::run()
{
	while (!threadShouldExit())
	{
		{
			PROFILE_ONLY(initIfEmpty(DebugSession::ThreadIdentifier::getCurrent()));
			PROFILE_ONLY(checkRecording());

			PerfettoHelpers::setCurrentThreadName("Scripting Thread");
			
			TRACE_SCRIPTING("script thread execution");

			Array<WeakReference<JavascriptProcessor>> compiledProcessors;
			compiledProcessors.ensureStorageAllocated(16);

			auto r = executeQueue(Task::LowPriorityCallbackExecution, compiledProcessors);
			
			if (!r.wasOk() && r.toConsoleMessage() != "Engine is dangling")
			{
				debugError(getMainController()->getMainSynthChain(), r.toConsoleMessage());
			}

			PROFILE_ONLY(checkRecording());
		}

		wait(500);
	}
}

void JavascriptThreadPool::killVoicesAndExtendTimeOut(JavascriptProcessor* jp, int milliseconds)
{
	if (!getMainController()->isInitialised())
		return;

	getMainController()->getKillStateHandler().killVoicesAndWait(&milliseconds);

	if (auto engine = jp->getScriptEngine())
	{
		engine->extendTimeout(milliseconds);
	}
}

void JavascriptThreadPool::pushToQueue(const Task::Type& t, JavascriptProcessor* p, const Task::Function& f)
{
	bumpCounter(t);

	switch (t)
	{
	case Task::LowPriorityCallbackExecution:
	{
		lowPriorityQueue.push({ Task(t, p, f), getMainController() });
		break;
	}
	case Task::HiPriorityCallbackExecution:
	{
		highPriorityQueue.push({ Task(t, p, f), getMainController() });
		break;
	}
	case Task::ReplEvaluation:
	{
#if USE_BACKEND
		replQueue.push({ Task(t, p, f), getMainController() });
#else
		jassertfalse;
		return;
#endif
		break;
	}
	case Task::Compilation:
	{
		compilationQueue.push({ Task(t, p, f), getMainController() });
		break;
	}
	default:
		jassertfalse;
		return;
	}

	notifyIfNotInBreakpoint();
}

UriError JavascriptThreadPool::executeNow(const Task::Type& t, JavascriptProcessor* p, const Task::Function& f)
{
	return Task(t, p, f).callWithResult();
}


UriError JavascriptThreadPool::Task::callWithResult()
{
	if (getProcessor() == nullptr)
		return UriError::fail("Processor deleted");

	auto& parent = dynamic_cast<Processor*>(getProcessor())->getMainController()->getJavascriptThreadPool();

	if (parent.threadShouldExit())
		return UriError::fail("Aborted");

	if (jp != nullptr && f)
	{
		jassert(type != Free);

		if(type == Compilation)
			LockHelpers::freeToGo(parent.getMainController());

		LockHelpers::SafeLock sl(parent.getMainController(), LockHelpers::Type::ScriptLock);

		ScopedValueSetter<bool> svs(parent.busy, true);
		ScopedValueSetter<Task::Type> svs2(parent.currentType, type);

		try
		{
			return f(jp.get());
		}
		catch (Result& r)
		{
			jassertfalse;
			return UriError::fail(r.getErrorMessage());
		}
		catch (UriError& r)
		{
			return r;
		}
		catch (String& errorMessage)
		{
			jassertfalse;
			return UriError::fail(errorMessage);
		}
	};

	return UriError::fail("invalid function");
}

void JavascriptProcessor::EditorHelpers::applyChangesFromActiveEditor(JavascriptProcessor* p)
{
	auto activeEditor = getActiveEditor(p);

	if (activeEditor == nullptr)
		return;

#if USE_BACKEND
	if (auto pe = activeEditor->findParentComponentOfClass<PopupIncludeEditor>())
	{
		auto f = pe->getFile();

		if (f.existsAsFile())
			f.replaceWithText(activeEditor->getDocument().getAllContent());
	}
#endif
}

hise::JavascriptCodeEditor* JavascriptProcessor::EditorHelpers::getActiveEditor(Processor* p)
{
	auto activeEditor = p->getMainController()->getLastActiveEditor();
	return dynamic_cast<JavascriptCodeEditor*>(activeEditor);
}

hise::JavascriptCodeEditor* JavascriptProcessor::EditorHelpers::getActiveEditor(JavascriptProcessor* p)
{
	auto processor = dynamic_cast<Processor*>(p);
	auto activeEditor = processor->getMainController()->getLastActiveEditor();

	return dynamic_cast<JavascriptCodeEditor*>(activeEditor);
}

} // namespace hise
