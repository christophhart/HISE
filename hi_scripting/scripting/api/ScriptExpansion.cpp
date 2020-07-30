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

struct ScriptExpansionHandler::Wrapper
{
	API_VOID_METHOD_WRAPPER_1(ScriptExpansionHandler, setErrorFunction);
	API_VOID_METHOD_WRAPPER_1(ScriptExpansionHandler, setErrorMessage);
	API_VOID_METHOD_WRAPPER_1(ScriptExpansionHandler, setCredentials);
	API_VOID_METHOD_WRAPPER_1(ScriptExpansionHandler, setEncryptionKey);
	API_METHOD_WRAPPER_1(ScriptExpansionHandler, setCurrentExpansion);
	API_METHOD_WRAPPER_0(ScriptExpansionHandler, getExpansionList);
	API_VOID_METHOD_WRAPPER_1(ScriptExpansionHandler, setExpansionCallback);
	API_METHOD_WRAPPER_1(ScriptExpansionHandler, encodeWithCredentials);
};

ScriptExpansionHandler::ScriptExpansionHandler(JavascriptProcessor* jp_) :
	ConstScriptingObject(dynamic_cast<ProcessorWithScriptingContent*>(jp_), 0),
	ControlledObject(dynamic_cast<ControlledObject*>(jp_)->getMainController()),
	jp(jp_)
{
	getMainController()->getExpansionHandler().addListener(this);

	ADD_API_METHOD_1(setErrorFunction);
	ADD_API_METHOD_1(setErrorMessage);
	ADD_API_METHOD_1(setCredentials);
	ADD_API_METHOD_1(setEncryptionKey);
	ADD_API_METHOD_0(getExpansionList);
	ADD_API_METHOD_1(setExpansionCallback);
	ADD_API_METHOD_1(setCurrentExpansion);
	ADD_API_METHOD_1(encodeWithCredentials);
}

ScriptExpansionHandler::~ScriptExpansionHandler()
{
	getMainController()->getExpansionHandler().removeListener(this);
}

void ScriptExpansionHandler::setEncryptionKey(String newKey)
{
	getMainController()->getExpansionHandler().setEncryptionKey(newKey);
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
		errorFunction = newErrorFunction;
}

void ScriptExpansionHandler::setErrorMessage(String errorMessage)
{
	if (HiseJavascriptEngine::isJavascriptFunction(errorFunction))
	{
		var arg(errorMessage);
		var::NativeFunctionArgs args(this, &arg, 1);
		jp->getScriptEngine()->callExternalFunctionRaw(errorFunction, args);
	}
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

void ScriptExpansionHandler::setExpansionCallback(var expansionLoadedCallback)
{
	if (HiseJavascriptEngine::isJavascriptFunction(expansionLoadedCallback))
	{
		loadedCallback = expansionLoadedCallback;
	}
}

var ScriptExpansionHandler::getUninitialisedExpansions()
{
	Array<var> list;

	for (auto e : getMainController()->getExpansionHandler().getListOfUnavailableExpansions())
		list.add(new ScriptExpansionReference(getScriptProcessor(), e));

	return list;
}

void ScriptExpansionHandler::refreshExpansions()
{
	getMainController()->getExpansionHandler().createAvailableExpansions();
}

bool ScriptExpansionHandler::setCurrentExpansion(String expansionName)
{
	return getMainController()->getExpansionHandler().setCurrentExpansion(expansionName);
}

bool ScriptExpansionHandler::encodeWithCredentials(var hxiFile)
{
	if (auto f = dynamic_cast<ScriptingObjects::ScriptFile*>(hxiFile.getObject()))
	{
		return ScriptEncryptedExpansion::encryptIntermediateFile(getMainController(), f->f);
	}
	else
		reportScriptError("argument is not a file");
}

void ScriptExpansionHandler::expansionPackLoaded(Expansion* currentExpansion)
{
	expansionPackCreated(currentExpansion);
}

void ScriptExpansionHandler::expansionPackCreated(Expansion* newExpansion)
{
	if (HiseJavascriptEngine::isJavascriptFunction(loadedCallback))
	{
		var args(new ScriptExpansionReference(getScriptProcessor(), newExpansion));
		auto r = Result::ok();

		jp->getScriptEngine()->callExternalFunction(loadedCallback, var::NativeFunctionArgs(this, &args, 1), &r, true);

		if (r.failed())
		{
			debugError(dynamic_cast<Processor*>(jp.get()), r.getErrorMessage());
		}
	}
}

void ScriptExpansionHandler::logMessage(const String& message, bool isCritical)
{
	if (HiseJavascriptEngine::isJavascriptFunction(errorFunction))
	{
		var args[2];
		args[0] = var(message);
		args[1] = isCritical;

		auto r = Result::ok();

		jp->getScriptEngine()->callExternalFunction(errorFunction, var::NativeFunctionArgs(this, args, 2), &r, true);

		if (r.failed())
		{
			debugError(dynamic_cast<Processor*>(jp.get()), r.getErrorMessage());
		}
	}
}

struct ScriptExpansionReference::Wrapper
{
	API_METHOD_WRAPPER_0(ScriptExpansionReference, getSampleMapList);
	API_METHOD_WRAPPER_0(ScriptExpansionReference, getImageList);
	API_METHOD_WRAPPER_0(ScriptExpansionReference, getAudioFileList);
	API_METHOD_WRAPPER_0(ScriptExpansionReference, getMidiFileList);
	API_METHOD_WRAPPER_0(ScriptExpansionReference, getProperties);
	API_METHOD_WRAPPER_1(ScriptExpansionReference, loadDataFile);
	API_METHOD_WRAPPER_2(ScriptExpansionReference, writeDataFile);
	API_METHOD_WRAPPER_0(ScriptExpansionReference, getRootFolder);
};

ScriptExpansionReference::ScriptExpansionReference(ProcessorWithScriptingContent* p, Expansion* e) :
	ConstScriptingObject(p, 0),
	exp(e)
{
	ADD_API_METHOD_0(getSampleMapList);
	ADD_API_METHOD_0(getImageList);
	ADD_API_METHOD_0(getAudioFileList);
	ADD_API_METHOD_0(getMidiFileList);
	ADD_API_METHOD_0(getProperties);
	ADD_API_METHOD_1(loadDataFile);
	ADD_API_METHOD_2(writeDataFile);
	ADD_API_METHOD_0(getRootFolder);
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

var ScriptExpansionReference::loadDataFile(var relativePath)
{
	auto fileToLoad = exp->getSubDirectory(FileHandlerBase::AdditionalSourceCode).getChildFile(relativePath.toString());

	return JSON::parse(fileToLoad.loadFileAsString());
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

ScriptEncryptedExpansion::ScriptEncryptedExpansion(MainController* mc, const File& f) :
	Expansion(mc, f)
{

}

hise::Expansion::ExpansionType ScriptEncryptedExpansion::getExpansionType() const
{

	return getExpansionTypeFromFolder(getRootFolder());
}

void ScriptEncryptedExpansion::encodeExpansion()
{
	if (getExpansionType() == Expansion::FileBased)
	{
		auto& handler = getMainController()->getExpansionHandler();

		String s;
		s << "Do you want to encode the expansion " << getProperty(ExpansionIds::Name) << "?  \n> The encryption key is `" << handler.getEncryptionKey() << "`.";

		if (PresetHandler::showYesNoWindow("Encode expansion", s))
		{
			auto hxiFile = Expansion::Helpers::getExpansionInfoFile(getRootFolder(), Expansion::Intermediate);

			ValueTree hxiData("Expansion");

			auto metadata = data->v.createCopy();
			metadata.setProperty(ExpansionIds::Hash, handler.getEncryptionKey().hashCode64(), nullptr);

			hxiData.addChild(metadata, -1, nullptr);

			pool->getAdditionalDataPool().loadAllFilesFromProjectFolder();
			pool->getImagePool().loadAllFilesFromProjectFolder();
			pool->getAudioSampleBufferPool().loadAllFilesFromProjectFolder();
			pool->getMidiFilePool().loadAllFilesFromProjectFolder();

			ValueTree poolData(ExpansionIds::PoolData);

			for (auto fileType : getListOfPooledSubDirectories())
				addDataType(poolData, fileType);

			addUserPresets(hxiData);

			hxiData.addChild(poolData, -1, nullptr);
			hxiFile.replaceWithText(hxiData.createXml()->createDocument(""));

			auto h = &getMainController()->getExpansionHandler();

			MessageManager::callAsync([h]()
			{
				h->clearExpansions();
				h->createAvailableExpansions();
			});
		}
	}
	else
	{
		PresetHandler::showMessageWindow("Already encoded", "The expansion " + getProperty(ExpansionIds::Name) + " is already encoded");
	}
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

Result ScriptEncryptedExpansion::initialise()
{
	auto type = getExpansionType();

	if (type == Expansion::FileBased)
		return Expansion::initialise();
	else if (type == Expansion::Intermediate)
	{
		auto& handler = getMainController()->getExpansionHandler();

		if (handler.getEncryptionKey().isEmpty())
			return skipEncryptedExpansionWithoutKey();
			
		if (handler.getCredentials().isObject())
			return Result::fail("Trying to load an intermediate expansion when credentials have been set");

		auto fileToLoad = Helpers::getExpansionInfoFile(getRootFolder(), type);

		ScopedPointer<XmlElement> xml = XmlDocument::parse(fileToLoad);

		if (xml != nullptr)
			return initialiseFromValueTree(ValueTree::fromXml(*xml));

		return Result::fail("Can't parse XML");
	}
	else if (type == ExpansionType::Encrypted)
	{
		auto& handler = getMainController()->getExpansionHandler();

		if (handler.getEncryptionKey().isEmpty() || !handler.getCredentials().isObject())
			return skipEncryptedExpansionWithoutKey();

		zstd::ZDefaultCompressor comp;
		auto f = Helpers::getExpansionInfoFile(getRootFolder(), type);
		ValueTree hxpData;
		auto r = comp.expand(f, hxpData);

		if (r.failed())
			return r;

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

		jassertfalse;
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

bool ScriptEncryptedExpansion::encryptIntermediateFile(MainController* mc, const File& f)
{
	auto& h = mc->getExpansionHandler();

	auto key = h.getEncryptionKey();

	if (key.isEmpty())
		return h.setErrorMessage("Can't encode credentials without encryption key", true);

	ScopedPointer<XmlElement> xml = XmlDocument::parse(f);

	if (xml == nullptr)
		return h.setErrorMessage("Can't parse .hxi file", true);

	ValueTree hxiData = ValueTree::fromXml(*xml);

	if (hxiData.getType() != Identifier("Expansion"))
		return h.setErrorMessage("Invalid .hxi file", true);

	auto hxiName = hxiData.getChildWithName(ExpansionIds::ExpansionInfo).getProperty(ExpansionIds::Name).toString();

	if (hxiName.isEmpty())
		return h.setErrorMessage("Can't get expansion name", true);

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

	auto expRoot = h.getExpansionFolder().getChildFile(hxiName);

	if (!expRoot.isDirectory())
		expRoot.createDirectory();
	
	auto hxpFile = Expansion::Helpers::getExpansionInfoFile(expRoot, Expansion::Encrypted);

	zstd::ZDefaultCompressor comp;
	auto ok = comp.compress(hxiData, hxpFile);

	if (ok.wasOk())
	{
		h.createAvailableExpansions();
		return true;
	}
}

juce::Result ScriptEncryptedExpansion::skipEncryptedExpansionWithoutKey()
{
	ValueTree v(ExpansionIds::ExpansionInfo);
	v.setProperty(ExpansionIds::Name, getRootFolder().getFileName(), nullptr);
	data = new Data(getRootFolder(), v);
	return Result::fail("no encryption key set for scripted encryption");
}

Result ScriptEncryptedExpansion::initialiseFromValueTree(const ValueTree& hxiData)
{
	data = new Data(getRootFolder(), hxiData.getChildWithName(ExpansionIds::ExpansionInfo).createCopy());

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

void ScriptEncryptedExpansion::extractUserPresetsIfEmpty(ValueTree encryptedTree)
{
	auto presetTree = encryptedTree.getChildWithName("UserPresets");

	// the directory might not have been created yet...
	auto targetDirectory = getRootFolder().getChildFile(FileHandlerBase::getIdentifier(FileHandlerBase::UserPresets));

	if (!targetDirectory.isDirectory())
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

} // namespace hise