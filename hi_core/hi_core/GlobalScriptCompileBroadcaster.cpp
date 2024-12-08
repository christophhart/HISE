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

void GlobalScriptCompileBroadcaster::removeIncludedFile(int index)
{
	includedFiles.remove(index);
}

void GlobalScriptCompileBroadcaster::restoreIncludedScriptFilesFromSnippet(const ValueTree& snippetTree)
{
#if USE_BACKEND
	auto mc = dynamic_cast<MainController*>(this);
	auto scriptRootFolder = mc->getActiveFileHandler()->getSubDirectory(FileHandlerBase::Scripts);

	if(!scriptRootFolder.isDirectory())
		return;

	auto snexRootFolder = BackendDllManager::getSubFolder(mc, BackendDllManager::FolderSubType::CodeLibrary);

	auto restoreFromChild = [&](const Identifier& id, const File& rootDirectory)
	{
		auto v = snippetTree.getChildWithName(id);
		jassert(v.isValid());
		
		auto chain = dynamic_cast<const MainController*>(this)->getMainSynthChain();

		for(auto c: v)
		{
			debugToConsole(const_cast<ModulatorSynthChain*>(chain), "loaded embedded file " + c["filename"].toString() + " from HISE snippet");
			auto n = new ExternalScriptFile(rootDirectory, c);
			includedFiles.add(n);
		}
	};
	
	restoreFromChild("embeddedScripts", scriptRootFolder);
	restoreFromChild("embeddedSnexFiles", snexRootFolder);
#endif
}

ValueTree GlobalScriptCompileBroadcaster::collectIncludedScriptFilesForSnippet(const Identifier& id, const File& root) const
{
#if USE_BACKEND
	ValueTree d(id);
	
	auto chain = dynamic_cast<const MainController*>(this)->getMainSynthChain();

	auto scriptProcessorList = ProcessorHelpers::getListOfAllProcessors<JavascriptProcessor>(chain);

	for(auto f: includedFiles)
	{
		if(!f->getFile().isAChildOf(root))
			continue;

		bool included = false;

		auto extension = f->getFile().getFileExtension();

		// Include faust & snex files but check the others
		// whether they are actually used or "(detached)"
		if(extension == ".dsp" || extension == ".h" || extension == ".xml")
			included = true;

		for(auto jp: scriptProcessorList)
		{
			for(int i = 0; i < jp->getNumWatchedFiles(); i++)
			{
				if(jp->getWatchedFile(i) == f->getFile())
				{
					included = true;
					break;
				}
			}

			if(included)
				break;
		}

		auto c = f->toEmbeddableValueTree(root);

		if(!included)
		{
			debugToConsole(const_cast<ModulatorSynthChain*>(chain), "skip detached file " + c["filename"].toString() + " from embedding into HISE snippet");
			continue;
		}
		
		debugToConsole(const_cast<ModulatorSynthChain*>(chain), "embedded " + c["filename"].toString() + " into HISE snippet");
		d.addChild(c, -1, nullptr);
	}

	return d;
#else
	jassertfalse;
	return {};
#endif
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

void GlobalScriptCompileBroadcaster::saveAllExternalFiles()
{
	for(int i = 0; i < getNumExternalScriptFiles(); i++)
	{		
		auto ef = getExternalScriptFile(i);

		if (!ef->getFile().exists())
		{
				removeIncludedFile(i);
				continue;
		}

		if(ef->getResourceType() == ExternalScriptFile::ResourceType::EmbeddedInSnippet)
		{
			debugToConsole(dynamic_cast<MainController*>(this)->getMainSynthChain(), "Skip writing embedded file " + ef->getFile().getFileName() + " to disk...");
			continue;
		}
			
		ef->saveFile();
	}
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

ExternalScriptFile::Ptr GlobalScriptCompileBroadcaster::getExternalScriptFile(const File& fileToInclude, bool createIfNotFound)
{
	for (int i = 0; i < includedFiles.size(); i++)
	{
		if (includedFiles[i]->getFile() == fileToInclude)
			return includedFiles[i];
	}

	if(createIfNotFound)
		return includedFiles.add(new ExternalScriptFile(fileToInclude));

	return nullptr;
}

juce::ValueTree GlobalScriptCompileBroadcaster::exportWebViewResources()
{
	ValueTree v("WebViewResources");

	for (const auto& wv : webviews)
	{
		auto projectRoot = dynamic_cast<MainController*>(this)->getCurrentFileHandler().getRootFolder();

		auto data = std::get<1>(wv);

		// We only embed file resources that are in the project directory (because then we'll assume
		// that they are not likely to be installed on the end user's computer).
		auto shouldEmbedResource = data->getRootDirectory().isAChildOf(projectRoot);

		if (shouldEmbedResource)
		{
			auto id = std::get<0>(wv).toString();

			auto poolDir = projectRoot.getChildFile("Images").getChildFile("exported_webviews");

#if JUCE_WINDOWS
			poolDir = poolDir.getChildFile("Windows");
#else
			poolDir = poolDir.getChildFile("macOS");
#endif

			poolDir.createDirectory();

			auto wvFile = poolDir.getChildFile(id).withFileExtension(".dat");

			zstd::ZDefaultCompressor comp;


#if USE_BACKEND
		if(CompileExporter::isExportingFromCommandLine())
		{
			if(wvFile.existsAsFile())
			{
				ValueTree c;
				comp.expand(wvFile, c);
				v.addChild(c, -1, nullptr);
			}
			else
			{
				throw Result::fail("Can't find preexported web resource for " + id);
			}
		}
		else
		{
			auto c = data->exportAsValueTree();
			c.setProperty("ID", id, nullptr);

			comp.compress(c, wvFile);

			v.addChild(c, -1, nullptr);
		}
#endif
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
	resourceType(ResourceType::FileBased),
	currentResult(Result::ok())
{
	lastEditTime = getFile().getLastModificationTime();

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

bool ExternalScriptFile::extractEmbedded()
{
	if(resourceType == ResourceType::EmbeddedInSnippet)
	{
		if(!file.existsAsFile() || PresetHandler::showYesNoWindow("Overwrite local file", "The file " + getFile().getFileName() + " from the snippet already exists. Do you want to overwrite your local file?"))
		{
			file.getParentDirectory().createDirectory();
			file.replaceWithText(content.getAllContent());
			resourceType = ResourceType::FileBased;
			return true;
		}
	}

	return false;
}
} // namespace hise
