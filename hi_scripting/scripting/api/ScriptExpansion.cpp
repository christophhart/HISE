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

namespace hise {
using namespace juce;

struct ScriptExpansionHandler::Wrapper
{
	API_VOID_METHOD_WRAPPER_1(ScriptExpansionHandler, setErrorFunction);
	API_VOID_METHOD_WRAPPER_1(ScriptExpansionHandler, setErrorMessage);
	API_VOID_METHOD_WRAPPER_1(ScriptExpansionHandler, setCredentials);
	API_VOID_METHOD_WRAPPER_1(ScriptExpansionHandler, setEncryptionKey);
	API_METHOD_WRAPPER_1(ScriptExpansionHandler, setCurrentExpansion);
	API_METHOD_WRAPPER_0(ScriptExpansionHandler, getExpansionList);
	API_METHOD_WRAPPER_1(ScriptExpansionHandler, getExpansion);
	API_METHOD_WRAPPER_0(ScriptExpansionHandler, getCurrentExpansion);
	API_VOID_METHOD_WRAPPER_1(ScriptExpansionHandler, setExpansionCallback);
	API_VOID_METHOD_WRAPPER_1(ScriptExpansionHandler, setInstallCallback);
	API_METHOD_WRAPPER_1(ScriptExpansionHandler, encodeWithCredentials);
	API_METHOD_WRAPPER_0(ScriptExpansionHandler, refreshExpansions);
	API_VOID_METHOD_WRAPPER_1(ScriptExpansionHandler, setAllowedExpansionTypes);
	API_METHOD_WRAPPER_2(ScriptExpansionHandler, installExpansionFromPackage);
	API_METHOD_WRAPPER_1(ScriptExpansionHandler, getExpansionForInstallPackage);
};

ScriptExpansionHandler::ScriptExpansionHandler(JavascriptProcessor* jp_) :
	ConstScriptingObject(dynamic_cast<ProcessorWithScriptingContent*>(jp_), 3),
	ControlledObject(dynamic_cast<ControlledObject*>(jp_)->getMainController()),
	jp(jp_),
	expansionCallback(dynamic_cast<ProcessorWithScriptingContent*>(jp_), var(), 1),
	errorFunction(dynamic_cast<ProcessorWithScriptingContent*>(jp_), var(), 2),
	installCallback(dynamic_cast<ProcessorWithScriptingContent*>(jp_), var(), 1)
{
	getMainController()->getExpansionHandler().addListener(this);

	ADD_API_METHOD_1(setErrorFunction);
	ADD_API_METHOD_1(setErrorMessage);
	ADD_API_METHOD_1(setCredentials);
	ADD_API_METHOD_1(setEncryptionKey);
	ADD_API_METHOD_0(getExpansionList);
	ADD_API_METHOD_1(getExpansion);
	ADD_API_METHOD_1(setExpansionCallback);
	ADD_API_METHOD_1(setCurrentExpansion);
	ADD_API_METHOD_1(encodeWithCredentials);
	ADD_API_METHOD_0(refreshExpansions);
	ADD_API_METHOD_2(installExpansionFromPackage);
	ADD_API_METHOD_1(setAllowedExpansionTypes);
	ADD_API_METHOD_0(getCurrentExpansion);
	ADD_API_METHOD_1(setInstallCallback);
	ADD_API_METHOD_1(getExpansionForInstallPackage);

	

	addConstant(Expansion::Helpers::getExpansionTypeName(Expansion::FileBased), Expansion::FileBased);
	addConstant(Expansion::Helpers::getExpansionTypeName(Expansion::Intermediate), Expansion::Intermediate);
	addConstant(Expansion::Helpers::getExpansionTypeName(Expansion::Encrypted), Expansion::Encrypted);
}

ScriptExpansionHandler::~ScriptExpansionHandler()
{
	getMainController()->getExpansionHandler().removeListener(this);
}

void ScriptExpansionHandler::setEncryptionKey(String newKey)
{
	reportScriptError("This function is deprecated. Use the project settings to setup the project's blowfish key");
	//getMainController()->getExpansionHandler().setEncryptionKey(newKey);
}

void ScriptExpansionHandler::setCredentials(var newCredentials)
{
	if (newCredentials.getDynamicObject() == nullptr)
	{
		setErrorMessage("credentials must be an object");
		return;
	}

	getMainController()->getExpansionHandler().setCredentials(newCredentials);
}

void ScriptExpansionHandler::setErrorFunction(var newErrorFunction)
{
	if (HiseJavascriptEngine::isJavascriptFunction(newErrorFunction))
		errorFunction = WeakCallbackHolder(getScriptProcessor(), newErrorFunction, 1);

	errorFunction.setHighPriority();
}

void ScriptExpansionHandler::setErrorMessage(String errorMessage)
{
	logMessage(errorMessage, false);

}

var ScriptExpansionHandler::getExpansionList()
{
	auto& h = getMainController()->getExpansionHandler();

	Array<var> list;

	for (int i = 0; i < h.getNumExpansions(); i++)
	{
		list.add(new ScriptExpansionReference(dynamic_cast<ProcessorWithScriptingContent*>(jp.get()), h.getExpansion(i)));
	}

	return var(list);
}

var ScriptExpansionHandler::getExpansion(var name)
{
	if (auto e = getMainController()->getExpansionHandler().getExpansionFromName(name.toString()))
	{
		return new ScriptExpansionReference(getScriptProcessor(), e);
	}

	return var();
}

var ScriptExpansionHandler::getCurrentExpansion()
{
	if (auto e = getMainController()->getExpansionHandler().getCurrentExpansion())
		return new ScriptExpansionReference(getScriptProcessor(), e);

	return {};
}

void ScriptExpansionHandler::setExpansionCallback(var expansionLoadedCallback)
{
	if (HiseJavascriptEngine::isJavascriptFunction(expansionLoadedCallback))
	{
		expansionCallback = WeakCallbackHolder(getScriptProcessor(), expansionLoadedCallback, 1);
		expansionCallback.setThisObject(this);
		expansionCallback.incRefCount();
	}

	expansionCallback.setHighPriority();
}

void ScriptExpansionHandler::setInstallCallback(var installationCallback)
{
	if (HiseJavascriptEngine::isJavascriptFunction(installationCallback))
	{
		installCallback = WeakCallbackHolder(getScriptProcessor(), installationCallback, 1);
		installCallback.setThisObject(this);
		installCallback.incRefCount();
	}
}

var ScriptExpansionHandler::getUninitialisedExpansions()
{
	Array<var> list;

	for (auto e : getMainController()->getExpansionHandler().getListOfUnavailableExpansions())
		list.add(new ScriptExpansionReference(dynamic_cast<ProcessorWithScriptingContent*>(jp.get()), e));

	return list;
}

bool ScriptExpansionHandler::refreshExpansions()
{
	return getMainController()->getExpansionHandler().createAvailableExpansions();
}

bool ScriptExpansionHandler::setCurrentExpansion(var expansionName)
{
	if(expansionName.isString())
		return getMainController()->getExpansionHandler().setCurrentExpansion(expansionName);
	
	if (auto se = dynamic_cast<ScriptExpansionReference*>(expansionName.getObject()))
		return setCurrentExpansion(se->exp->getProperty(ExpansionIds::Name));

	

	reportScriptError("can't find expansion");
	RETURN_IF_NO_THROW(false);
}

bool ScriptExpansionHandler::encodeWithCredentials(var hxiFile)
{
	if (auto f = dynamic_cast<ScriptingObjects::ScriptFile*>(hxiFile.getObject()))
	{
		if (!f->f.existsAsFile())
			reportScriptError(f->toString(0) + " doesn't exist");

		return ScriptEncryptedExpansion::encryptIntermediateFile(getMainController(), f->f);
	}
	else
	{
		reportScriptError("argument is not a file");
		RETURN_IF_NO_THROW(false);
	}
}

bool ScriptExpansionHandler::installExpansionFromPackage(var packageFile, var sampleDirectory)
{
	if (auto f = dynamic_cast<ScriptingObjects::ScriptFile*>(packageFile.getObject()))
	{
		File targetFolder;

		if (sampleDirectory.isInt())
		{
			auto target = (ScriptingApi::FileSystem::SpecialLocations)(int)sampleDirectory;

			if (target == ScriptingApi::FileSystem::Expansions)
				targetFolder = getMainController()->getExpansionHandler().getExpansionFolder();
			if (target == ScriptingApi::FileSystem::Samples)
				targetFolder = getMainController()->getCurrentFileHandler().getSubDirectory(FileHandlerBase::Samples);
		}
		else if (auto sf = dynamic_cast<ScriptingObjects::ScriptFile*>(sampleDirectory.getObject()))
		{
			targetFolder = sf->f;
		}

		if (!targetFolder.isDirectory())
			reportScriptError("The sample directory does not exist");
		
		if (installCallback)
		{
			currentInstaller = new InstallState(*this);
		}

		return getMainController()->getExpansionHandler().installFromResourceFile(f->f, targetFolder);
	}
	else
	{
		reportScriptError("argument is not a file");
		RETURN_IF_NO_THROW(false);
	}
}

var ScriptExpansionHandler::getExpansionForInstallPackage(var packageFile)
{
	if (auto sf = dynamic_cast<ScriptingObjects::ScriptFile*>(packageFile.getObject()))
	{
		auto& eh = getMainController()->getExpansionHandler();
		auto tf = eh.getExpansionTargetFolder(sf->f);

		if (tf == File())
			logMessage("Can't read metadata of package", false);

		if (auto e = eh.getExpansionFromRootFile(tf))
		{
			//  In order to simulate the end user process, we do not detect file based expansions
			//  with this call.
			if (e->getExpansionType() != Expansion::FileBased)
			{
				return var(new ScriptExpansionReference(getScriptProcessor(), e));
			}
		}

		return var();
	}

	reportScriptError("getExpansionForInstallPackage requires a file as parameter");
	RETURN_IF_NO_THROW(var());
}

void ScriptExpansionHandler::setAllowedExpansionTypes(var typeList)
{
	Array<Expansion::ExpansionType> l;

	if (auto ar = typeList.getArray())
	{
		for (auto& v : *ar)
			l.add((Expansion::ExpansionType)(int)v);

		getMainController()->getExpansionHandler().setAllowedExpansions(l);
	}
	else
		reportScriptError("Argument must be an array");
}

void ScriptExpansionHandler::expansionPackLoaded(Expansion* currentExpansion)
{
	expansionPackCreated(currentExpansion);
}

void ScriptExpansionHandler::expansionPackCreated(Expansion* newExpansion)
{
	if (expansionCallback)
	{
		if (newExpansion != nullptr)
		{
			var args(new ScriptExpansionReference(getScriptProcessor(), newExpansion));
			expansionCallback.call(&args, 1);
		}
		else
		{
			var args;
			expansionCallback.call(&args, 1);
		}

		
	}
}

void ScriptExpansionHandler::logMessage(const String& message, bool isCritical)
{

	if (errorFunction)
	{
		var args[2];
		args[0] = message;
		args[1] = isCritical;
		errorFunction.call(args, 2);
	}
}

hise::ProcessorWithScriptingContent* ScriptExpansionHandler::getScriptProcessor()
{
	return dynamic_cast<ProcessorWithScriptingContent*>(jp.get());
}


ScriptExpansionHandler::InstallState::InstallState(ScriptExpansionHandler& parent_) :
	parent(parent_),
	status(-1)
{
	parent.getMainController()->getExpansionHandler().addListener(this);
}

ScriptExpansionHandler::InstallState::~InstallState()
{
	parent.getMainController()->getExpansionHandler().removeListener(this);
}

void ScriptExpansionHandler::InstallState::expansionPackCreated(Expansion* newExpansion)
{
	
}

void ScriptExpansionHandler::InstallState::expansionInstallStarted(const File& targetRoot, const File& packageFile, const File& sampleDirectory)
{
	sourceFile = packageFile;
	targetFolder = targetRoot;
	sampleFolder = sampleDirectory;
	createdExpansion = nullptr;
	status = 0;
	call();
	startTimer(300);
}

void ScriptExpansionHandler::InstallState::expansionInstalled(Expansion* newExpansion)
{
	SimpleReadWriteLock::ScopedWriteLock sl(timerLock);

	stopTimer();
	status = 2;

	if (newExpansion != nullptr && newExpansion->getRootFolder() == targetFolder)
		createdExpansion = newExpansion;

	call();

	WeakReference<ScriptExpansionHandler> safeParent(&parent);

	auto f = [safeParent]()
	{
		if (safeParent != nullptr)
			safeParent.get()->currentInstaller = nullptr;
	};
}

void ScriptExpansionHandler::InstallState::call()
{
	parent.installCallback.call1(getObject());
}

void ScriptExpansionHandler::InstallState::timerCallback()
{
	SimpleReadWriteLock::ScopedTryReadLock sl(timerLock);

	if (sl.hasLock())
	{
		status = 1;
		call();
	}
}

var ScriptExpansionHandler::InstallState::getObject()
{
	DynamicObject::Ptr newObj = new DynamicObject();
	newObj->setProperty("Status", status);
	newObj->setProperty("Progress", getProgress());
	newObj->setProperty("SourceFile", new ScriptingObjects::ScriptFile(parent.getScriptProcessor(), sourceFile));
	newObj->setProperty("TargetFolder", new ScriptingObjects::ScriptFile(parent.getScriptProcessor(), targetFolder));
	newObj->setProperty("SampleFolder", new ScriptingObjects::ScriptFile(parent.getScriptProcessor(), sampleFolder));
	newObj->setProperty("Expansion", (createdExpansion != nullptr) ? var(new ScriptExpansionReference(parent.getScriptProcessor(), createdExpansion)) : var());

	return var(newObj);
}

double ScriptExpansionHandler::InstallState::getProgress()
{
	return parent.getMainController()->getSampleManager().getPreloadProgress();
}


struct ScriptExpansionReference::Wrapper
{
	API_METHOD_WRAPPER_0(ScriptExpansionReference, getSampleMapList);
	API_METHOD_WRAPPER_0(ScriptExpansionReference, getImageList);
	API_METHOD_WRAPPER_0(ScriptExpansionReference, getAudioFileList);
	API_METHOD_WRAPPER_0(ScriptExpansionReference, getMidiFileList);
	API_METHOD_WRAPPER_0(ScriptExpansionReference, getDataFileList);
	API_METHOD_WRAPPER_0(ScriptExpansionReference, getProperties);
	API_METHOD_WRAPPER_1(ScriptExpansionReference, loadDataFile);
	API_METHOD_WRAPPER_2(ScriptExpansionReference, writeDataFile);
	API_METHOD_WRAPPER_0(ScriptExpansionReference, getRootFolder);
	API_METHOD_WRAPPER_0(ScriptExpansionReference, getExpansionType);
	API_METHOD_WRAPPER_1(ScriptExpansionReference, getWildcardReference);
	API_METHOD_WRAPPER_0(ScriptExpansionReference, getSampleFolder);
	API_METHOD_WRAPPER_1(ScriptExpansionReference, setSampleFolder);
	API_METHOD_WRAPPER_0(ScriptExpansionReference, rebuildUserPresets);
};

ScriptExpansionReference::ScriptExpansionReference(ProcessorWithScriptingContent* p, Expansion* e) :
	ConstScriptingObject(p, 0),
	exp(e)
{
	ADD_API_METHOD_0(getSampleMapList);
	ADD_API_METHOD_0(getImageList);
	ADD_API_METHOD_0(getAudioFileList);
	ADD_API_METHOD_0(getMidiFileList);
	ADD_API_METHOD_0(getDataFileList);
	ADD_API_METHOD_0(getProperties);
	ADD_API_METHOD_1(loadDataFile);
	ADD_API_METHOD_2(writeDataFile);
	ADD_API_METHOD_0(getRootFolder);
	ADD_API_METHOD_0(getExpansionType);
	ADD_API_METHOD_1(getWildcardReference);
	ADD_API_METHOD_1(setSampleFolder);
	ADD_API_METHOD_0(getSampleFolder);
	ADD_API_METHOD_0(rebuildUserPresets);
}

juce::BlowFish* ScriptExpansionReference::createBlowfish()
{
	if (auto se = dynamic_cast<ScriptEncryptedExpansion*>(exp.get()))
	{
		return se->createBlowfish();
	}

	return nullptr;
}

var ScriptExpansionReference::getRootFolder()
{
	if (objectExists())
		return var(new ScriptingObjects::ScriptFile(getScriptProcessor(), exp->getRootFolder()));

	reportScriptError("Expansion was deleted");
	RETURN_IF_NO_THROW({});
}

var ScriptExpansionReference::getSampleMapList() const
{
	if (objectExists())
	{
		auto refList = exp->pool->getSampleMapPool().getListOfAllReferences(true);

		Array<var> list;

		for (auto& ref : refList)
			list.add(ref.getReferenceString().upToFirstOccurrenceOf(".xml", false, true));

		return list;
	}

	reportScriptError("Expansion was deleted");
	RETURN_IF_NO_THROW({});
}

var ScriptExpansionReference::getImageList() const
{
	if (objectExists())
	{
		exp->pool->getImagePool().loadAllFilesFromProjectFolder();
		auto refList = exp->pool->getImagePool().getListOfAllReferences(true);


		Array<var> list;

		for (auto& ref : refList)
			list.add(ref.getReferenceString());

		return list;
	}

	reportScriptError("Expansion was deleted");
	RETURN_IF_NO_THROW({});
}

var ScriptExpansionReference::getAudioFileList() const
{
	if (objectExists())
	{
		exp->pool->getAudioSampleBufferPool().loadAllFilesFromProjectFolder();
		auto refList = exp->pool->getAudioSampleBufferPool().getListOfAllReferences(true);



		Array<var> list;

		for (auto& ref : refList)
			list.add(ref.getReferenceString());

		return list;
	}

	reportScriptError("Expansion was deleted");
	RETURN_IF_NO_THROW({});
}

var ScriptExpansionReference::getMidiFileList() const
{
	if (objectExists())
	{
		auto refList = exp->pool->getMidiFilePool().getListOfAllReferences(true);

		Array<var> list;

		for (auto& ref : refList)
			list.add(ref.getReferenceString());

		return list;
	}

	reportScriptError("Expansion was deleted");
	RETURN_IF_NO_THROW({});
}

var ScriptExpansionReference::getDataFileList() const
{
	if (objectExists())
	{
		auto refList = exp->pool->getAdditionalDataPool().getListOfAllReferences(true);

		Array<var> list;

		for (auto& ref : refList)
			list.add(ref.getReferenceString());

		return list;
	}

	reportScriptError("Expansion was deleted");
	RETURN_IF_NO_THROW({});
}

var ScriptExpansionReference::getSampleFolder()
{
	File sampleFolder = exp->getSubDirectory(FileHandlerBase::Samples);

	return new ScriptingObjects::ScriptFile(getScriptProcessor(), sampleFolder);
}

bool ScriptExpansionReference::setSampleFolder(var newSampleFolder)
{
	if (auto f = dynamic_cast<ScriptingObjects::ScriptFile*>(newSampleFolder.getObject()))
	{
		auto newTarget = f->f;

		if (!newTarget.isDirectory())
			reportScriptError(newTarget.getFullPathName() + " is not an existing directory");

		if (newTarget != exp->getSubDirectory(FileHandlerBase::Samples))
		{
			exp->createLinkFile(FileHandlerBase::Samples, newTarget);
			exp->checkSubDirectories();
			return true;
		}
	}

	return false;
}

var ScriptExpansionReference::loadDataFile(var relativePath)
{
	if (objectExists())
	{
		if (exp->getExpansionType() == Expansion::FileBased)
		{
			auto fileToLoad = exp->getSubDirectory(FileHandlerBase::AdditionalSourceCode).getChildFile(relativePath.toString());

			if(fileToLoad.existsAsFile())
				return JSON::parse(fileToLoad.loadFileAsString());

			reportScriptError("Can't find data file " + fileToLoad.getFullPathName());
		}
		else
		{
			String rs;

			auto wc = exp->getWildcard();
			auto path = relativePath.toString();

			if (!path.contains(wc))
				rs << wc;

			rs << path;

			PoolReference ref(getScriptProcessor()->getMainController_(), rs, FileHandlerBase::AdditionalSourceCode);
			
			if (auto o = exp->pool->getAdditionalDataPool().loadFromReference(ref, PoolHelpers::LoadAndCacheStrong))
			{
				var obj;
				auto ok = JSON::parse(o->data.getFile(), obj);

				if (ok.wasOk())
					return obj;

				reportScriptError("Error at parsing JSON: " + ok.getErrorMessage());
			}
			else
			{
				reportScriptError("Can't find data file " + ref.getReferenceString());
			}
		}
	}

	return {};
}

String ScriptExpansionReference::getWildcardReference(var relativePath)
{
	if (objectExists())
		return exp->getWildcard() + relativePath.toString();

	return {};
}

bool ScriptExpansionReference::writeDataFile(var relativePath, var dataToWrite)
{
	auto content = JSON::toString(dataToWrite);

	auto targetFile = exp->getSubDirectory(FileHandlerBase::AdditionalSourceCode).getChildFile(relativePath.toString());

	return targetFile.replaceWithText(content);
}

var ScriptExpansionReference::getProperties() const
{
	if (objectExists())
		return exp->getPropertyObject();

	return {};
}

int ScriptExpansionReference::getExpansionType() const
{
	if (objectExists())
		return (int)exp->getExpansionType();

	return -1;
}

bool ScriptExpansionReference::rebuildUserPresets()
{
	if (auto sf = dynamic_cast<ScriptEncryptedExpansion*>(exp.get()))
	{
		ValueTree v;
		auto ok = sf->loadValueTree(v);

		if (ok.wasOk())
		{
			sf->extractUserPresetsIfEmpty(v, true);
			return true;
		}
		else
		{
			debugError(dynamic_cast<Processor*>(getScriptProcessor()), "Error at extracting user presets: ");
			debugError(dynamic_cast<Processor*>(getScriptProcessor()), ok.getErrorMessage());
		}
	}

	return false;
}

ScriptEncryptedExpansion::ScriptEncryptedExpansion(MainController* mc, const File& f) :
	Expansion(mc, f)
{

}


hise::Expansion::ExpansionType ScriptEncryptedExpansion::getExpansionType() const
{

	return getExpansionTypeFromFolder(getRootFolder());
}

Result ScriptEncryptedExpansion::encodeExpansion()
{
	if (getExpansionType() == Expansion::FileBased)
	{
		auto& handler = getMainController()->getExpansionHandler();

		if (handler.getEncryptionKey().isEmpty())
		{
			return Result::fail("You have to set an encryption key using `ExpansionHandler.setEncryptionKey()` before using this method.");
		}

		String s;
		s << "Do you want to encode the expansion " << getProperty(ExpansionIds::Name) << "?  \n> The encryption key is `" << handler.getEncryptionKey() << "`.";

		if (true)//PresetHandler::showYesNoWindow("Encode expansion", s))
		{
			auto hxiFile = Expansion::Helpers::getExpansionInfoFile(getRootFolder(), Expansion::Intermediate);

			ValueTree hxiData("Expansion");

			auto metadata = data->v.createCopy();
			metadata.setProperty(ExpansionIds::Hash, handler.getEncryptionKey().hashCode64(), nullptr);

			hxiData.addChild(metadata, -1, nullptr);
			encodePoolAndUserPresets(hxiData, false);

#if HISE_USE_XML_FOR_HXI
			ScopedPointer<XmlElement> xml = hxiData.createXml();
			hxiFile.replaceWithText(xml->createDocument(""));
#else
			hxiFile.deleteFile();
			FileOutputStream fos(hxiFile);
			hxiData.writeToStream(fos);
			fos.flush();
#endif
			auto h = &getMainController()->getExpansionHandler();

			h->forceReinitialisation();
		}
	}
	else
	{
		return Result::fail("The expansion " + getProperty(ExpansionIds::Name) + " is already encoded");
	}

	return Result::ok();
}

juce::Array<hise::FileHandlerBase::SubDirectories> ScriptEncryptedExpansion::getSubDirectoryIds() const
{
	if (getExpansionType() == Expansion::FileBased)
		return Expansion::getSubDirectoryIds();
	else
	{
		if (getRootFolder().getChildFile("UserPresets").isDirectory())
			return { FileHandlerBase::UserPresets, FileHandlerBase::Samples };
		else
			return { FileHandlerBase::Samples };
	}

}


juce::Result ScriptEncryptedExpansion::loadValueTree(ValueTree& v)
{
	if(getExpansionType() == Expansion::Intermediate)
	{
		auto& handler = getMainController()->getExpansionHandler();

		if (handler.getEncryptionKey().isEmpty())
		{
			v = ValueTree(ExpansionIds::ExpansionInfo);
			v.setProperty(ExpansionIds::Name, getRootFolder().getFileName(), nullptr);
			return Result::ok(); // will fail later
		}

		auto fileToLoad = Helpers::getExpansionInfoFile(getRootFolder(), Intermediate);

	#if HISE_USE_XML_FOR_HXI
		ScopedPointer<XmlElement> xml = XmlDocument::parse(fileToLoad);

		if (xml != nullptr)
		{
			v = ValueTree::fromXml(*xml);
			return Result::ok();
		}

		return Result::fail("Can't parse XML");
	#else
		FileInputStream fis(fileToLoad);
		v = ValueTree::readFromStream(fis);

		if (v.isValid())
			return Result::ok();
		else
			return Result::fail("Can't parse ValueTree");
	#endif
	}
	if (getExpansionType() == Expansion::Encrypted)
	{
		auto& handler = getMainController()->getExpansionHandler();

		if (handler.getEncryptionKey().isEmpty() || !handler.getCredentials().isObject())
		{
			v = ValueTree(ExpansionIds::ExpansionInfo);
			v.setProperty(ExpansionIds::Name, getRootFolder().getFileName(), nullptr);
			return Result::ok(); // will fail later
		}

		zstd::ZDefaultCompressor comp;
		auto f = Helpers::getExpansionInfoFile(getRootFolder(), Encrypted);

		FileInputStream fis(f);

		v = ValueTree::readFromStream(fis);

		if (!v.isValid())
			return Result::fail("Can't parse expansion data file");

		return Result::ok();
	}
}





Result ScriptEncryptedExpansion::initialise()
{
	auto type = getExpansionType();

	if (type == Expansion::FileBased)
		return Expansion::initialise();
	else if (type == Expansion::Intermediate)
	{
		ValueTree v;
		auto ok = loadValueTree(v);

		if (v.isValid())
			return initialiseFromValueTree(v);

		return ok;



#if 0
		auto& handler = getMainController()->getExpansionHandler();

		if (handler.getEncryptionKey().isEmpty())
			return skipEncryptedExpansionWithoutKey();

		auto fileToLoad = Helpers::getExpansionInfoFile(getRootFolder(), type);

#if HISE_USE_XML_FOR_HXI
		ScopedPointer<XmlElement> xml = XmlDocument::parse(fileToLoad);

		if (xml != nullptr)
			return initialiseFromValueTree(ValueTree::fromXml(*xml));

		return Result::fail("Can't parse XML");
#else
		FileInputStream fis(fileToLoad);
		auto v = ValueTree::readFromStream(fis);

		if (v.isValid())
			return initialiseFromValueTree(v);
		else
			return Result::fail("Can't parse ValueTree");
#endif
#endif

	}
	else if (type == ExpansionType::Encrypted)
	{
		auto& handler = getMainController()->getExpansionHandler();
#if 0
		

		if (handler.getEncryptionKey().isEmpty() || !handler.getCredentials().isObject())
			return skipEncryptedExpansionWithoutKey();

		zstd::ZDefaultCompressor comp;
		auto f = Helpers::getExpansionInfoFile(getRootFolder(), type);

		FileInputStream fis(f);

		auto hxpData = ValueTree::readFromStream(fis);

		if (!hxpData.isValid())
			return Result::fail("Can't parse expansion data file");
#endif

		ValueTree hxpData;

		auto ok = loadValueTree(hxpData);

		if (hxpData.getNumChildren() == 0)
		{
			data = new Data(getRootFolder(), hxpData, getMainController());
			return Result::fail("no encryption key set for scripted encryption");
		}

		auto credTree = hxpData.getChildWithName(ExpansionIds::Credentials);
		auto base64Obj = credTree[ExpansionIds::Data].toString();

		if (ScopedPointer<BlowFish> bf = createBlowfish())
		{
			MemoryBlock mb;
			mb.fromBase64Encoding(base64Obj);
			bf->decrypt(mb);
			base64Obj = mb.toBase64Encoding();
		}

		if (base64Obj.hashCode64() != (int64)credTree[ExpansionIds::Hash])
			return Result::fail("Credential hash don't match");

		auto obj = ValueTreeConverters::convertBase64ToDynamicObject(base64Obj, true);

		if (!ExpansionHandler::Helpers::equalJSONData(obj, handler.getCredentials()))
			return Result::fail("Credentials don't match");

		return initialiseFromValueTree(hxpData);
	}

	return Result::ok();
}

juce::BlowFish* ScriptEncryptedExpansion::createBlowfish()
{
	return createBlowfish(getMainController());
}

juce::BlowFish* ScriptEncryptedExpansion::createBlowfish(MainController* mc)
{
	auto d = mc->getExpansionHandler().getEncryptionKey();

	if (d.isNotEmpty())
		return new BlowFish(d.getCharPointer().getAddress(), d.length());
	else
		return nullptr;
}

bool ScriptEncryptedExpansion::encryptIntermediateFile(MainController* mc, const File& f, File expRoot)
{
	auto& h = mc->getExpansionHandler();

	auto key = h.getEncryptionKey();

	if (key.isEmpty())
		return h.setErrorMessage("Can't encode credentials without encryption key", true);

#if HISE_USE_XML_FOR_HXI
	ScopedPointer<XmlElement> xml = XmlDocument::parse(f);

	if (xml == nullptr)
		return h.setErrorMessage("Can't parse XML", true);

	auto hxiData = ValueTree::fromXml(*xml);
#else
	FileInputStream fis(f);
	auto hxiData = ValueTree::readFromStream(fis);
#endif

	if (hxiData.getType() != Identifier("Expansion"))
		return h.setErrorMessage("Invalid .hxi file", true);

	if (expRoot == File())
	{
		auto hxiName = hxiData.getChildWithName(ExpansionIds::ExpansionInfo).getProperty(ExpansionIds::Name).toString();

		if (hxiName.isEmpty())
			return h.setErrorMessage("Can't get expansion name", true);

		expRoot = h.getExpansionFolder().getChildFile(hxiName);
	}

	if (!expRoot.isDirectory())
		expRoot.createDirectory();

	auto hash = (int64)hxiData.getChildWithName(ExpansionIds::ExpansionInfo)[ExpansionIds::Hash];

	if (hash != key.hashCode64())
		return h.setErrorMessage("embedded key does not match encryption key", true);

	auto obj = h.getCredentials();

	if (!obj.isObject())
		return h.setErrorMessage("No credentials set for encryption", true);

	auto c = ValueTreeConverters::convertDynamicObjectToBase64(var(obj), "Credentials", true);
	auto credentialsHash = c.hashCode64();

	ValueTree credTree(ExpansionIds::Credentials);

	MemoryBlock mb;
	mb.fromBase64Encoding(c);

	if (ScopedPointer<BlowFish> bf = createBlowfish(mc))
		bf->encrypt(mb);
	else
		return h.setErrorMessage("Can't create blowfish key", true);

	credTree.setProperty(ExpansionIds::Hash, credentialsHash, nullptr);
	credTree.setProperty(ExpansionIds::Data, mb.toBase64Encoding(), nullptr);

	hxiData.addChild(credTree, 1, nullptr);

	auto hxpFile = Expansion::Helpers::getExpansionInfoFile(expRoot, Expansion::Encrypted);

	hxpFile.deleteFile();
	hxpFile.create();

	FileOutputStream fos(hxpFile);
	hxiData.writeToStream(fos);
	fos.flush();
	h.createAvailableExpansions();
	return true;
}

void ScriptEncryptedExpansion::encodePoolAndUserPresets(ValueTree &hxiData, bool projectExport)
{
	auto& h = getMainController()->getExpansionHandler();

	h.setErrorMessage("Preparing pool export", false);

	if (!projectExport)
	{
		pool->getAdditionalDataPool().loadAllFilesFromProjectFolder();
		pool->getImagePool().loadAllFilesFromProjectFolder();
		pool->getAudioSampleBufferPool().loadAllFilesFromProjectFolder();
		pool->getMidiFilePool().loadAllFilesFromProjectFolder();
	}
	else
	{
		auto nativePool = getMainController()->getCurrentFileHandler().pool.get();

		pool->getMidiFilePool().loadAllFilesFromProjectFolder();

		auto& nip = nativePool->getImagePool();
		auto& nap = nativePool->getAudioSampleBufferPool();

		BACKEND_ONLY(ExpansionHandler::ScopedProjectExporter sps(getMainController(), true));

		for (int i = 0; i < nip.getNumLoadedFiles(); i++)
		{
			PoolReference ref(getMainController(), nip.getReference(i).getFile().getFullPathName(), FileHandlerBase::Images);
			pool->getImagePool().loadFromReference(ref, PoolHelpers::LoadAndCacheStrong);
		}

		for (int i = 0; i < nap.getNumLoadedFiles(); i++)
		{
			PoolReference ref(getMainController(), nap.getReference(i).getFile().getFullPathName(), FileHandlerBase::AudioFiles);
			pool->getAudioSampleBufferPool().loadFromReference(ref, PoolHelpers::LoadAndCacheStrong);
		}
	}

	ValueTree poolData(ExpansionIds::PoolData);

	for (auto fileType : getListOfPooledSubDirectories())
	{
		h.setErrorMessage("Exporting " + FileHandlerBase::getIdentifier(fileType), false);

		if (!projectExport || fileType != FileHandlerBase::AdditionalSourceCode)
			addDataType(poolData, fileType);
	}

	h.setErrorMessage("Embedding user presets", false);

	addUserPresets(hxiData);

	hxiData.addChild(poolData, -1, nullptr);
}

juce::Result ScriptEncryptedExpansion::skipEncryptedExpansionWithoutKey()
{
	ValueTree v(ExpansionIds::ExpansionInfo);
	v.setProperty(ExpansionIds::Name, getRootFolder().getFileName(), nullptr);
	data = new Data(getRootFolder(), v, getMainController());
	return Result::fail("no encryption key set for scripted encryption");
}

Result ScriptEncryptedExpansion::initialiseFromValueTree(const ValueTree& hxiData)
{
	if (hxiData.getNumChildren() == 0)
	{
		data = new Data(getRootFolder(), hxiData, getMainController());
		return Result::fail("no encryption key set for scripted encryption");
	}

	data = new Data(getRootFolder(), hxiData.getChildWithName(ExpansionIds::ExpansionInfo).createCopy(), getMainController());

	extractUserPresetsIfEmpty(hxiData);

	auto hash = getProperty(ExpansionIds::Hash).getLargeIntValue();

	if (getMainController()->getExpansionHandler().getEncryptionKey().hashCode64() != hash)
		return Result::fail("Wrong hash code");

	for (auto fileType : getListOfPooledSubDirectories())
	{
		setCompressorForPool(fileType, true);
		restorePool(hxiData, fileType);
	}

	pool->getSampleMapPool().loadAllFilesFromDataProvider();
	pool->getMidiFilePool().loadAllFilesFromDataProvider();
	pool->getAdditionalDataPool().loadAllFilesFromDataProvider();
	checkSubDirectories();
	return Result::ok();
}

hise::PoolBase::DataProvider::Compressor* ScriptEncryptedExpansion::createCompressor(bool createEncrypted)
{
	if (createEncrypted)
	{
		auto key = createBlowfish();
		return new EncryptedCompressor(key);
	}
	else
	{
		return new hise::PoolBase::DataProvider::Compressor();
	}
}

void ScriptEncryptedExpansion::setCompressorForPool(SubDirectories fileType, bool createEncrypted)
{
	pool->getPoolBase(fileType)->getDataProvider()->setCompressor(createCompressor(createEncrypted));
}

void ScriptEncryptedExpansion::addDataType(ValueTree& parent, SubDirectories fileType)
{
	MemoryBlock mb;

	ScopedPointer<MemoryOutputStream> mos = new MemoryOutputStream(mb, false);

	auto p = pool->getPoolBase(fileType);

	setCompressorForPool(fileType, true);

	p->getDataProvider()->writePool(mos.release());

	auto id = getIdentifier(fileType).removeCharacters("/");

	ValueTree vt(id);
	vt.setProperty(ExpansionIds::Data, mb.toBase64Encoding(), nullptr);

	parent.addChild(vt, -1, nullptr);
}

void ScriptEncryptedExpansion::restorePool(ValueTree encryptedTree, SubDirectories fileType)
{
	auto poolData = encryptedTree.getChildWithName(ExpansionIds::PoolData);

	MemoryBlock mb;

	auto childName = getIdentifier(fileType).removeCharacters("/");
	auto c = poolData.getChildWithName(childName);
	auto d = c.getProperty(ExpansionIds::Data).toString();

	mb.fromBase64Encoding(d);

	ScopedPointer<MemoryInputStream> mis = new MemoryInputStream(mb, true);

	pool->getPoolBase(fileType)->getDataProvider()->restorePool(mis.release());
}

void ScriptEncryptedExpansion::addUserPresets(ValueTree encryptedTree)
{
	auto userPresets = UserPresetHelpers::collectAllUserPresets(getMainController()->getMainSynthChain(), this);

	MemoryBlock mb;

	zstd::ZCompressor<hise::UserPresetDictionaryProvider> comp;
	comp.compress(userPresets, mb);
	ValueTree v("UserPresets");
	v.setProperty("Data", mb.toBase64Encoding(), nullptr);
	encryptedTree.addChild(v, -1, nullptr);
}

void ScriptEncryptedExpansion::extractUserPresetsIfEmpty(ValueTree encryptedTree, bool forceExtraction)
{
	auto presetTree = encryptedTree.getChildWithName("UserPresets");

	// the directory might not have been created yet...
	auto targetDirectory = getRootFolder().getChildFile(FileHandlerBase::getIdentifier(FileHandlerBase::UserPresets));

	if (!targetDirectory.isDirectory() || forceExtraction)
	{
		MemoryBlock mb;
		mb.fromBase64Encoding(presetTree.getProperty(ExpansionIds::Data).toString());
		ValueTree p;
		zstd::ZCompressor<hise::UserPresetDictionaryProvider> comp;
		comp.expand(mb, p);

		if (p.getNumChildren() != 0)
		{
			targetDirectory.createDirectory();
			UserPresetHelpers::extractDirectory(p, targetDirectory);
		}
	}
}

void FullInstrumentExpansion::setNewDefault(MainController* mc, ValueTree t)
{
	if (isEnabled(mc))
		mc->setDefaultPresetHandler(new DefaultHandler(mc, t));
}

bool FullInstrumentExpansion::isEnabled(const MainController* mc)
{
#if USE_BACKEND
	return dynamic_cast<const GlobalSettingManager*>(mc)->getSettingsObject().getSetting(HiseSettings::Project::ExpansionType) == "Full";
#else
	ignoreUnused(mc);
	return FrontendHandler::getExpansionType() == "Full";
#endif
}

Expansion* FullInstrumentExpansion::getCurrentFullExpansion(const MainController* mc)
{
	if (!isEnabled(mc))
		return nullptr;

	return mc->getExpansionHandler().getCurrentExpansion();
}

void FullInstrumentExpansion::expansionPackLoaded(Expansion* e)
{

	if (e == this)
	{
		if (fullyLoaded)
		{
			auto pr = presetToLoad.createCopy();

			getMainController()->getKillStateHandler().killVoicesAndCall(getMainController()->getMainSynthChain(), [pr](Processor* p)
			{
				p->getMainController()->loadPresetFromValueTree(pr);
				return SafeFunctionCall::OK;
			}, MainController::KillStateHandler::SampleLoadingThread);
		}
		else
		{
			getMainController()->getKillStateHandler().killVoicesAndCall(getMainController()->getMainSynthChain(), [this](Processor* p)
			{
				getMainController()->getSampleManager().setCurrentPreloadMessage("Initialising expansion");
				auto r = lazyLoad();

				if (r.wasOk())
				{
					auto pr = presetToLoad.createCopy();
					p->getMainController()->loadPresetFromValueTree(pr);
				}

				return SafeFunctionCall::OK;
			}, MainController::KillStateHandler::SampleLoadingThread);
		}


	}
}

struct PublicIconProvider : public PoolBase::DataProvider
{
	PublicIconProvider(PoolBase* pool, const String& baseString) :
		DataProvider(pool)
	{
		mb.fromBase64Encoding(baseString);
	}

	MemoryInputStream* createInputStream(const String& referenceString) override
	{
		if (referenceString.fromLastOccurrenceOf("}", false, false).toUpperCase() == "ICON.PNG")
		{
			return new MemoryInputStream(mb, false);
		}

		jassertfalse;
		return nullptr;
	}


	MemoryBlock mb;
};

juce::Result FullInstrumentExpansion::initialise()
{
	auto type = getExpansionType();

	if (type == Expansion::FileBased)
	{
		return Expansion::initialise();
	}
	if (type == Expansion::Intermediate)
	{
		auto& handler = getMainController()->getExpansionHandler();

		if (handler.getEncryptionKey().isEmpty())
			return Result::fail("The encryption key for a Full expansion must be set already");

		auto allData = getValueTreeFromFile(type);

		if (!allData.isValid())
			return Result::fail("Error parsing hxi file");

		jassert(allData.isValid() && allData.getType() == ExpansionIds::FullData);

		data = new Data(getRootFolder(), allData.getChildWithName(ExpansionIds::ExpansionInfo).createCopy(), getMainController());

		auto iconData = allData.getChildWithName(ExpansionIds::HeaderData).getChildWithName(ExpansionIds::Icon)[ExpansionIds::Data].toString();

		if(iconData.isNotEmpty())
			pool->getImagePool().setDataProvider(new PublicIconProvider(&pool->getImagePool(), iconData));

		fullyLoaded = false;

		getMainController()->getExpansionHandler().addListener(this);

		checkSubDirectories();

		return Result::ok();
	}

	return Expansion::initialise();
}


juce::ValueTree FullInstrumentExpansion::getValueTreeFromFile(Expansion::ExpansionType type)
{
	auto hxiFile = Helpers::getExpansionInfoFile(getRootFolder(), type);

	FileInputStream fis(hxiFile);

	if (fis.readByte() == '<')
	{
		ScopedPointer<XmlElement> xml = XmlDocument::parse(hxiFile);

		if (xml == nullptr)
			return ValueTree();

		return ValueTree::fromXml(*xml);
	}
	else
	{
		fis.setPosition(0);
		return ValueTree::readFromStream(fis);
	}
}

Result FullInstrumentExpansion::lazyLoad()
{
	auto allData = getValueTreeFromFile(getExpansionType());

	if (!allData.isValid())
		return Result::fail("Can't parse ValueTree");

	auto presetData = allData.getChildWithName(ExpansionIds::Preset)[ExpansionIds::Data].toString();

	ScopedPointer<BlowFish> bf = createBlowfish();

	MemoryBlock mb;
	mb.fromBase64Encoding(presetData);

	bf->decrypt(mb);

	zstd::ZCompressor<hise::PresetDictionaryProvider> comp;

	comp.expand(mb, presetToLoad);

	auto scriptTree = allData.getChildWithName(ExpansionIds::Scripts);

	if (presetToLoad.isValid())
	{
		auto bfCopy = bf.get();

		ScriptingApi::Content::Helpers::callRecursive(presetToLoad, [scriptTree, bfCopy](ValueTree& v)
		{
			if (v.hasProperty(ExpansionIds::Script))
			{
				auto hashToUse = (int)v[ExpansionIds::Script];
				auto scriptToUse = scriptTree.getChildWithProperty(ExpansionIds::Hash, hashToUse)[ExpansionIds::Data].toString();
				MemoryBlock mb;
				mb.fromBase64Encoding(scriptToUse);
				bfCopy->decrypt(mb);

				zstd::ZCompressor<hise::JavascriptDictionaryProvider> comp;
				String s;
				comp.expand(mb, s);
				v.setProperty(ExpansionIds::Script, s, nullptr);
			}

			return true;
		});
	}

	bf = nullptr;

	pool->getImagePool().setDataProvider(new PoolBase::DataProvider(&pool->getImagePool()));

	auto r = initialiseFromValueTree(allData);

	if (r.wasOk())
	{
		fullyLoaded = true;
	}

	return r;
}

juce::Result ScriptEncryptedExpansion::returnFail(const String& errorMessage)
{
	auto& h = getMainController()->getExpansionHandler();
	h.setErrorMessage(errorMessage, false);
	return Result::fail(errorMessage);
}



Result FullInstrumentExpansion::encodeExpansion()
{
	ValueTree allData(ExpansionIds::FullData);

	auto& h = getMainController()->getExpansionHandler();
	auto key = h.getEncryptionKey();

	if (key.isEmpty())
	{
		return returnFail("The encryption key has not been set");
	}

	auto hxiFile = Expansion::Helpers::getExpansionInfoFile(getRootFolder(), Expansion::Intermediate);

	auto metadata = data->v.createCopy();
	metadata.setProperty(ExpansionIds::Hash, key.hashCode64(), nullptr);

	allData.addChild(metadata, -1, nullptr);

	{
		h.setErrorMessage("Encoding Fonts and Icons", false);

		ValueTree hd(ExpansionIds::HeaderData);

		{
			ValueTree fonts(ExpansionIds::Fonts);

			zstd::ZDefaultCompressor d;
			MemoryBlock fontData;
			d.compress(getMainController()->exportCustomFontsAsValueTree(), fontData);
			fonts.setProperty(ExpansionIds::Data, fontData.toBase64Encoding(), nullptr);

			hd.addChild(fonts, -1, nullptr);
		}

		PoolReference icon(getMainController(), String(isProjectExport ? "{PROJECT_FOLDER}" : getWildcard()) + "Icon.png", FileHandlerBase::Images);

		if (icon.getFile().existsAsFile())
		{
			MemoryBlock mb;
			icon.getFile().loadFileAsData(mb);
			ValueTree iconData(ExpansionIds::Icon);
			iconData.setProperty(ExpansionIds::Data, mb.toBase64Encoding(), nullptr);

			hd.addChild(iconData, -1, nullptr);
		}

		allData.addChild(hd, -1, nullptr);
	}


	h.setErrorMessage("Collecting scripts", false);

	ScopedPointer<BlowFish> bf = createBlowfish();
	ValueTree scripts(ExpansionIds::Scripts);
	Processor::Iterator<JavascriptProcessor> iter(getMainController()->getMainSynthChain());

	while (auto jp = iter.getNextProcessor())
	{
		auto s = jp->collectScript(true);
		auto idHash = dynamic_cast<Processor*>(jp)->getId().hashCode();

		zstd::ZCompressor<hise::JavascriptDictionaryProvider> comp;
		MemoryBlock mb;
		comp.compress(s, mb);
		bf->encrypt(mb);

		ValueTree c(ExpansionIds::Script);
		c.setProperty(ExpansionIds::Hash, idHash, nullptr);
		c.setProperty(ExpansionIds::Data, mb.toBase64Encoding(), nullptr);

		scripts.addChild(c, -1, nullptr);
	}

	allData.addChild(scripts, -1, nullptr);

	h.setErrorMessage("Embedding currently loaded project", false);

	{
		auto mTree = getMainController()->getMainSynthChain()->exportAsValueTree();

		ScriptingApi::Content::Helpers::callRecursive(mTree, [scripts](ValueTree& v)
		{
			// Replace the scripts with their hash
			if (v.hasProperty(ExpansionIds::Script))
			{
				auto hash = v["ID"].toString().hashCode();
				jassert(scripts.getChildWithProperty(ExpansionIds::Hash, hash).isValid());
				v.setProperty(ExpansionIds::Script, hash, nullptr);
			}

			return true;
		});

#if 0
		if (isProjectExport)
		{
			auto wc = getWildcard();
			ScriptingApi::Content::Helpers::callRecursive(mTree, [scripts, wc](ValueTree& v)
			{
				// Replace any project folder wildcard reference with the expansion
				for (int i = 0; i < v.getNumProperties(); i++)
				{
					auto id = v.getPropertyName(i);

					auto value = v[id];

					if (value.isString() && value.toString().contains("{PROJECT_FOLDER}"))
						v.setProperty(id, value.toString().replace("{PROJECT_FOLDER}", wc), nullptr);
				}

				return true;
			});
		}
#endif

		zstd::ZCompressor<hise::PresetDictionaryProvider> comp;
		MemoryBlock mb;
		comp.compress(mTree, mb);
		ValueTree preset(ExpansionIds::Preset);
		bf->encrypt(mb);

		preset.setProperty(ExpansionIds::Data, mb.toBase64Encoding(), nullptr);
		allData.addChild(preset, -1, nullptr);
	}

	encodePoolAndUserPresets(allData, isProjectExport);
	
	h.setErrorMessage("Writing file", false);

#if HISE_USE_XML_FOR_HXI
	ScopedPointer<XmlElement> xml = allData.createXml();
	hxiFile.replaceWithText(xml->createDocument(""));
#else
	hxiFile.deleteFile();
	FileOutputStream fos(hxiFile);
	allData.writeToStream(fos);
#endif

	h.setErrorMessage("Done", false);

	if(!isProjectExport)
		h.forceReinitialisation();

	return Result::ok();
}

ExpansionEncodingWindow::ExpansionEncodingWindow(MainController* mc, Expansion* eToEncode, bool isProjectExport) :
	DialogWindowWithBackgroundThread(isProjectExport ? "Encode project as Full Expansion" : "Encode Expansion"),
	ControlledObject(mc),
	e(eToEncode),
	encodeResult(Result::ok()),
	projectExport(isProjectExport)
{
	if (isProjectExport)
	{
	}
	else
	{
		StringArray expList;
		auto l = getMainController()->getExpansionHandler().getListOfAvailableExpansions();

		for (auto v : *l.getArray())
			expList.add(v.toString());

		addComboBox("expansion", expList, "Expansion to encode");

		getComboBoxComponent("expansion")->addItem("All expansions", AllExpansionId);

		if (e != nullptr)
			getComboBoxComponent("expansion")->setText(e->getProperty(ExpansionIds::Name), dontSendNotification);
	}

	getMainController()->getExpansionHandler().addListener(this);
	addBasicComponents(true);

	showStatusMessage("Press OK to encode the expansion");
}

ExpansionEncodingWindow::~ExpansionEncodingWindow()
{
	getMainController()->getExpansionHandler().removeListener(this);
}

void ExpansionEncodingWindow::run()
{
#if USE_BACKEND
	if (projectExport)
	{
		auto& h = GET_PROJECT_HANDLER(getMainController()->getMainSynthChain());
		auto f = Expansion::Helpers::getExpansionInfoFile(h.getWorkDirectory(), Expansion::FileBased);

		ValueTree mData(ExpansionIds::ExpansionInfo);

		mData.setProperty(ExpansionIds::Name, GET_HISE_SETTING(getMainController()->getMainSynthChain(), HiseSettings::Project::Name), nullptr);
		mData.setProperty(ExpansionIds::Version, GET_HISE_SETTING(getMainController()->getMainSynthChain(), HiseSettings::Project::Version), nullptr);
		mData.setProperty(HiseSettings::User::Company, GET_HISE_SETTING(getMainController()->getMainSynthChain(), HiseSettings::User::Company), nullptr);
		mData.setProperty(HiseSettings::User::Company, GET_HISE_SETTING(getMainController()->getMainSynthChain(), HiseSettings::User::CompanyURL), nullptr);
		mData.setProperty(ExpansionIds::UUID, GET_HISE_SETTING(getMainController()->getMainSynthChain(), HiseSettings::Project::BundleIdentifier), nullptr);
		mData.setProperty(ExpansionIds::HiseVersion, HISE_VERSION, nullptr);

		ScopedPointer<XmlElement> xml = mData.createXml();
		f.replaceWithText(xml->createDocument(""));
		ScopedPointer<FullInstrumentExpansion> e = new FullInstrumentExpansion(getMainController(), h.getWorkDirectory());
		e->initialise();
		e->setIsProjectExporter();
		encodeResult = e->encodeExpansion();
		f.deleteFile();
	}
	else
	{
		if (getComboBoxComponent("expansion")->getSelectedId() == AllExpansionId)
		{
			auto& h = getMainController()->getExpansionHandler();

			for (int i = 0; i < h.getNumExpansions(); i++)
			{
				if (auto e = h.getExpansion(i))
				{
					showStatusMessage("Encoding " + e->getProperty(ExpansionIds::Name));
					setProgress((double)i / (double)h.getNumExpansions());

					encodeResult = e->encodeExpansion();

					if (encodeResult.failed())
						break;
				}
			}

			return;
		}

		if (e == nullptr)
		{
			auto n = getComboBoxComponent("expansion")->getText();
			e = getMainController()->getExpansionHandler().getExpansionFromName(n);
		}

		if (e != nullptr)
			encodeResult = e->encodeExpansion();
		else
			encodeResult = Result::fail("No expansion to encode");
	}
#endif
}

void ExpansionEncodingWindow::threadFinished()
{
#if USE_BACKEND
	if (projectExport)
	{
		auto& h = GET_PROJECT_HANDLER(getMainController()->getMainSynthChain());
		Expansion::Helpers::getExpansionInfoFile(h.getWorkDirectory(), Expansion::Intermediate).revealToUser();
		return;
	}

	if (encodeResult.wasOk())
		PresetHandler::showMessageWindow("Expansion encoded", "The expansion was encoded successfully");
	else
		PresetHandler::showMessageWindow("Expansion encoding failed", encodeResult.getErrorMessage(), PresetHandler::IconType::Error);
#endif
}

} // namespace hise