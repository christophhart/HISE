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

GlobalScriptCompileBroadcaster::GlobalScriptCompileBroadcaster() :
	useBackgroundCompiling(false),
	enableGlobalRecompile(true),
	globalEditBroadcaster(new ScriptComponentEditBroadcaster())
{
	createDummyLoader();
}

GlobalScriptCompileBroadcaster::~GlobalScriptCompileBroadcaster()
{
	dummyLibraryLoader = nullptr;
	globalEditBroadcaster = nullptr;
    
	clearIncludedFiles();
}

void GlobalScriptCompileBroadcaster::addScriptListener(GlobalScriptCompileListener* listener, bool insertAtBeginning)
{
	if (insertAtBeginning)
	{
		listenerListStart.addIfNotAlreadyThere(listener);
	}
	else
	{
		listenerListEnd.addIfNotAlreadyThere(listener);
	}

}

void GlobalScriptCompileBroadcaster::removeScriptListener(GlobalScriptCompileListener* listener)
{
	listenerListStart.removeAllInstancesOf(listener);
	listenerListEnd.removeAllInstancesOf(listener);
}

void GlobalScriptCompileBroadcaster::setShouldUseBackgroundThreadForCompiling(bool shouldBeEnabled) noexcept
{ useBackgroundCompiling = shouldBeEnabled; }

bool GlobalScriptCompileBroadcaster::isUsingBackgroundThreadForCompiling() const noexcept
{ return useBackgroundCompiling; }

void GlobalScriptCompileBroadcaster::setEnableCompileAllScriptsOnPresetLoad(bool shouldBeEnabled) noexcept
{ enableGlobalRecompile = shouldBeEnabled; }

bool GlobalScriptCompileBroadcaster::isCompilingAllScriptsOnPresetLoad() const noexcept
{ return enableGlobalRecompile; }

int GlobalScriptCompileBroadcaster::getNumExternalScriptFiles() const
{ return includedFiles.size(); }

ExternalScriptFile::Ptr GlobalScriptCompileBroadcaster::getExternalScriptFile(int index) const
{
	return includedFiles[index];
}

void GlobalScriptCompileBroadcaster::clearIncludedFiles()
{
	includedFiles.clear();
}

ScriptComponentEditBroadcaster* GlobalScriptCompileBroadcaster::getScriptComponentEditBroadcaster()
{
	return globalEditBroadcaster;
}

const ScriptComponentEditBroadcaster* GlobalScriptCompileBroadcaster::getScriptComponentEditBroadcaster() const
{
	return globalEditBroadcaster;
}

ReferenceCountedObject* GlobalScriptCompileBroadcaster::getCurrentScriptLookAndFeel()
{ return currentScriptLaf.get(); }

void GlobalScriptCompileBroadcaster::setCurrentScriptLookAndFeel(ReferenceCountedObject* newLaf)
{
	currentScriptLaf = newLaf;
}

ReferenceCountedObject* GlobalScriptCompileBroadcaster::getGlobalRoutingManager()
{ return routingManager.get(); }

void GlobalScriptCompileBroadcaster::setGlobalRoutingManager(ReferenceCountedObject* newManager)
{ routingManager = newManager; }

void GlobalScriptCompileBroadcaster::clearWebResources()
{
	webviews.clear();
}

void GlobalScriptCompileBroadcaster::setWebViewRoot(File newRoot)
{
	webViewRoot = newRoot;
}


void GlobalScriptCompileBroadcaster::sendScriptCompileMessage(JavascriptProcessor *processorThatWasCompiled)
{
	if (!enableGlobalRecompile) return;

	for (int i = 0; i < listenerListStart.size(); i++)
	{
		if (listenerListStart[i].get() != nullptr)
		{
			listenerListStart[i].get()->scriptWasCompiled(processorThatWasCompiled);
		}
		else
		{
			listenerListStart.remove(i--);
		}
	}

	for (int i = 0; i < listenerListEnd.size(); i++)
	{
		if (listenerListEnd[i].get() != nullptr)
		{
			listenerListEnd[i].get()->scriptWasCompiled(processorThatWasCompiled);
		}
		else
		{
			listenerListEnd.remove(i--);
		}
	}
}

double GlobalScriptCompileBroadcaster::getCompileTimeOut() const noexcept
{
	return jmax(2.0, (double)dynamic_cast<const GlobalSettingManager*>(this)->getSettingsObject().getSetting(HiseSettings::Scripting::CompileTimeout));

}

bool GlobalScriptCompileBroadcaster::isCallStackEnabled() const noexcept
{
	return (bool)dynamic_cast<const GlobalSettingManager*>(this)->getSettingsObject().getSetting(HiseSettings::Scripting::EnableCallstack);
}

void GlobalScriptCompileBroadcaster::updateCallstackSettingForExistingScriptProcessors()
{
	auto shouldBeEnabled = isCallStackEnabled();

	ModulatorSynthChain *mainChain = dynamic_cast<MainController*>(this)->getMainSynthChain();

	Processor::Iterator<JavascriptProcessor> iter(mainChain);

	while (auto jp = iter.getNextProcessor())
	{
		jp->setCallStackEnabled(shouldBeEnabled);
	}
}

void GlobalScriptCompileBroadcaster::fillExternalFileList(Array<File> &files, StringArray &processors)
{
	ModulatorSynthChain *mainChain = dynamic_cast<MainController*>(this)->getMainSynthChain();

	Processor::Iterator<JavascriptProcessor> iter(mainChain);

	while (JavascriptProcessor *sp = iter.getNextProcessor())
	{
		for (int i = 0; i < sp->getNumWatchedFiles(); i++)
		{
			if (!files.contains(sp->getWatchedFile(i)))
			{
				files.add(sp->getWatchedFile(i));
				processors.add(dynamic_cast<Processor*>(sp)->getId());
			}



		}
	}
}

void GlobalScriptCompileBroadcaster::setExternalScriptData(const ValueTree &collectedExternalScripts)
{
	externalScripts = collectedExternalScripts;
}

String GlobalScriptCompileBroadcaster::getExternalScriptFromCollection(const String &fileName)
{
    static const String deviceWildcard = "{DEVICE}";

    String realFileName = fileName;
    
    if(realFileName.contains(deviceWildcard))
    {
        realFileName = realFileName.replace(deviceWildcard, HiseDeviceSimulator::getDeviceName());
    }
    
	for (int i = 0; i < externalScripts.getNumChildren(); i++)
	{
    const String thisName = externalScripts.getChild(i).getProperty("FileName").toString().replace("\\", "/");
        
		if (thisName == realFileName)
		{
			return externalScripts.getChild(i).getProperty("Content").toString();
		}
	}

	// Hitting this assert means you try to get a script that wasn't exported.
	jassertfalse;
	return String();
}

ExternalScriptFile::Ptr GlobalScriptCompileBroadcaster::getExternalScriptFile(const File& fileToInclude)
{
	for (int i = 0; i < includedFiles.size(); i++)
	{
		if (includedFiles[i]->getFile() == fileToInclude)
			return includedFiles[i];
	}

	includedFiles.add(new ExternalScriptFile(fileToInclude));

	return includedFiles.getLast();
}

juce::ValueTree GlobalScriptCompileBroadcaster::exportWebViewResources()
{
	ValueTree v("WebViewResources");

	for (const auto& wv : webviews)
	{
		auto data = std::get<1>(wv);

		auto projectRoot = dynamic_cast<MainController*>(this)->getCurrentFileHandler().getRootFolder();

		// We only embed file resources that are in the project directory (because then we'll assume
		// that they are not likely to be installed on the end user's computer).
		auto shouldEmbedResource = data->getRootDirectory().isAChildOf(projectRoot);

		if (shouldEmbedResource)
		{
			auto id = std::get<0>(wv).toString();
			auto c = data->exportAsValueTree();
			c.setProperty("ID", id, nullptr);
			v.addChild(c, -1, nullptr);
		}
	}

	return v;
}

void GlobalScriptCompileBroadcaster::restoreWebResources(const ValueTree& v)
{
    clearWebResources();

	for (auto c : v)
	{
		auto d = getOrCreateWebView(c["ID"].toString());
		d->restoreFromValueTree(c);
	}
}

hise::WebViewData::Ptr GlobalScriptCompileBroadcaster::getOrCreateWebView(const Identifier& id)
{
	for (const auto& wv : webviews)
	{
		if (std::get<0>(wv) == id)
			return std::get<1>(wv);
	}

	webviews.add({ id, new WebViewData(webViewRoot) });
	return std::get<1>(webviews.getLast());
}

Array<juce::Identifier> GlobalScriptCompileBroadcaster::getAllWebViewIds() const
{
	Array<Identifier> ids;

	for (const auto& wv : webviews)
		ids.add(std::get<0>(wv));

	return ids;
}

void ExternalScriptFile::setRuntimeErrors(const Result& r)
{
	runtimeErrors.clearQuick();

	if (!r.wasOk())
	{
		StringArray sa = StringArray::fromLines(r.getErrorMessage());

		for (const auto& s : sa)
			runtimeErrors.add(RuntimeError(s));
	}

	runtimeErrorBroadcaster.sendMessage(sendNotification, &runtimeErrors);
}

ExternalScriptFile::RuntimeError::RuntimeError(const String& e)
{
	file = e.upToFirstOccurrenceOf("(", false, false);
	lineNumber = e.fromFirstOccurrenceOf("(", false, false).getIntValue();

	auto tokens = StringArray::fromTokens(e.fromFirstOccurrenceOf(")", false, false), ":", "");
	tokens.removeEmptyStrings();
	
	bool isWarning = tokens[0].trim() == "warning";

	errorLevel = isWarning ? ErrorLevel::Warning : ErrorLevel::Error;

	errorMessage = tokens[1].trim();

	if (errorMessage.isEmpty())
		errorLevel = ErrorLevel::Invalid;

}

bool ExternalScriptFile::RuntimeError::matches(const String& fileNameWithoutExtension) const
{
	return file.compareIgnoreCase(fileNameWithoutExtension) == 0;
}

String ExternalScriptFile::RuntimeError::toString() const
{
	/** Invert this: 
	auto s = e.fromFirstOccurrenceOf("Line ", false, false);
	auto l = s.getIntValue() - 1;
	auto c = s.fromFirstOccurrenceOf("(", false, false).upToFirstOccurrenceOf(")", false, false).getIntValue();
	errorMessage = s.fromFirstOccurrenceOf(": ", false, false);
	*/

	String e;
	e << "Line " << String(lineNumber) << "(-1): " << errorMessage;
	return e;
}

ExternalScriptFile::RuntimeError::RuntimeError() = default;

ExternalScriptFile::ExternalScriptFile(const File& file):
	file(file),
	currentResult(Result::ok())
{
#if USE_BACKEND
	content.replaceAllContent(file.loadFileAsString());
	content.setSavePoint();
	content.clearUndoHistory();
#endif
}

ExternalScriptFile::~ExternalScriptFile()
{

}

void ExternalScriptFile::setResult(Result r)
{
	currentResult = r;
}

CodeDocument& ExternalScriptFile::getFileDocument()
{ return content; }

Result ExternalScriptFile::getResult() const
{ return currentResult; }

File ExternalScriptFile::getFile() const
{ return file; }

ExternalScriptFile::RuntimeError::Broadcaster& ExternalScriptFile::getRuntimeErrorBroadcaster()
{ return runtimeErrorBroadcaster; }
} // namespace hise
