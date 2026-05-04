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



namespace hise {
using namespace juce;

HiseAssetInstaller::HiseAssetInstaller(MainController* mc, const var& jsonObject) :
	ControlledObject(mc)
{
	testMode = true;
	initialise(jsonObject);
}

HiseAssetInstaller::HiseAssetInstaller(MainController* mc, ZipFile* archive) :
	ControlledObject(mc),
	zf(archive)
{
	int idx = -1;

	for(int i = 0; i < zf->getNumEntries(); i++)
	{
		auto fn = zf->getEntry(i)->filename;

		if(fn.endsWith("package_install.json"))
		{
			relativePath = fn.upToFirstOccurrenceOf("package_install.json", false, false);
			idx = i;
			break;
		}
	}

	if (idx == -1)
	{
		writeToLog("FATAL: zip does not contain package_install.json");
		isValid = false;
		return;
	}

	ScopedPointer<InputStream> zs = zf->createStreamForEntry(idx);

	if (zs == nullptr)
	{
		writeToLog("FATAL: failed to open package_install.json stream");
		isValid = false;
		return;
	}

	MemoryOutputStream mos;
	mos.writeFromInputStream(*zs, zs->getTotalLength());
	mos.flush();
	auto content = mos.getMemoryBlock().toString();
	var obj;
	auto parseResult = JSON::parse(content, obj);

	if (! parseResult.wasOk())
	{
		writeToLog("FATAL: package_install.json parse error: " + parseResult.getErrorMessage());
		isValid = false;
		return;
	}

	initialise(obj);
}

HiseAssetInstaller::HiseAssetInstaller(MainController* mc, const UninstallInfo& uninstallInfo_):
  ControlledObject(mc),
  uninstallInfo(uninstallInfo_)
{
	if(uninstallInfo.isLocalFolder())
	{
		if(uninstallInfo.localFolder == getRootFolder())
		{
			testMode = true;
		}

		ScopedPointer<InputStream> is = createInputStreamFromSource("package_install.json");

		if(is != nullptr)
		{
			auto data = is->readEntireStreamAsString();
			auto obj = JSON::parse(data);
			initialise(obj);
		}
	}
}

juce::File HiseAssetInstaller::getRootFolder() const
{
	return getMainController()->getCurrentFileHandler().getRootFolder();
}

hise::HiseAssetInstaller::UndoableInstallAction::List HiseAssetInstaller::createInstallList(const UninstallInfo& infoToUse)
{
	if(infoToUse)
		uninstallInfo = infoToUse;

	UndoableInstallAction::List installActions;

	auto sourceInfo = getProjectInfo(false);
	auto targetInfo = getProjectInfo(true);

	if (!preprocessorValues.isEmpty())
	{
		auto sourceDefinitions = getPreprocessors(sourceInfo);
		auto targetDefinitions = getPreprocessors(targetInfo);

		installActions.add(new PreprocessorInstallAction(getMainController(), preprocessorValues, targetDefinitions, sourceDefinitions));
	}

	Array<Identifier> portableSettingIds = {
		HiseSettings::Project::OSXStaticLibs,
		HiseSettings::Project::WindowsStaticLibFolder,
	};

	DynamicObject::Ptr portableSettingValues;

	for(const auto& id: portableSettingIds)
	{
		auto v = sourceInfo[id];

		if(v.isNotEmpty())
		{
			if(portableSettingValues == nullptr)
				portableSettingValues = new DynamicObject();

			portableSettingValues->setProperty(id, v);
		}
	}

	if(portableSettingValues != nullptr)
	{
		installActions.add(new ProjectSettingInstallAction(getMainController(), portableSettingValues));
	}
	
	auto list = getFileList();
	auto rootFolder = getMainController()->getCurrentFileHandler().getRootFolder();

	for (auto& f : list)
	{
		if(zf != nullptr)
			installActions.add(new FileInstallAction(getMainController(), f.second, getInputStream(f.first)));
		else
		{
			jassert(uninstallInfo.isLocalFolder());

			auto relativePath = f.second.getRelativePathFrom(getSourceRootFolder());
			auto targetFile = getRootFolder().getChildFile(relativePath);

			if(auto is = createInputStreamFromSource(relativePath))
				installActions.add(new FileInstallAction(getMainController(), targetFile, is));
		}
	}
	
	if (infoToShow.isNotEmpty())
	{
		installActions.add(new InfoShowAction(getMainController(), infoToShow, infoToUse));
	}

	if(clipboardContent.isNotEmpty())
		installActions.add(new ClipboardInstallAction(getMainController(), clipboardContent));

	return installActions;
}

juce::var HiseAssetInstaller::getInstallInfoFromLog(const UninstallInfo& info)
{
	if(info)
		uninstallInfo = info;

	String packageName;

	if(uninstallInfo)
	{
		packageName = uninstallInfo.packageName;
	}
	else if(info)
	{
		auto packageInfo = getProjectInfo(false);
		packageName = packageInfo[HiseSettings::Project::Name];
	}

	var obj;
	auto ok = JSON::parse(getLogFile().loadFileAsString(), obj);

	if (auto ar = obj.getArray())
	{
		for (auto& package : *ar)
		{
			if (package[HiseSettings::Project::Name].toString() == packageName)
				return package;
		}
	}

	return var();
}

hise::HiseAssetInstaller::UndoableInstallAction::List HiseAssetInstaller::fromInstallLog(const var& obj)
{
	UndoableInstallAction::List installLog;

	if (auto ar = obj["Steps"].getArray())
	{
		for (const auto& step : *ar)
		{
			auto type = Identifier(step["Type"].toString());

			if (type == PreprocessorInstallAction::getStaticId())
				installLog.add(new PreprocessorInstallAction(getMainController(), step));
			if (type == FileInstallAction::getStaticId())
				installLog.add(new FileInstallAction(getMainController(), step));
			if(type == ProjectSettingInstallAction::getStaticId())
				installLog.add(new ProjectSettingInstallAction(getMainController(), step));
		}
	}

	return installLog;
}

juce::var HiseAssetInstaller::toInstallLog(UndoableInstallAction::List installActions, UninstallInfo infoToUse) const
{
	if(!infoToUse)
		infoToUse = uninstallInfo;

	DynamicObject::Ptr installLog = new DynamicObject();

	auto version = getProjectInfo(false)[HiseSettings::Project::Version];

	installLog->setProperty(HiseSettings::Project::Name, infoToUse.packageName);
	installLog->setProperty(HiseSettings::User::Company, infoToUse.vendor);
	installLog->setProperty(HiseSettings::Project::Version, version);
	installLog->setProperty("Date", Time::getCurrentTime().toISO8601(false));

	Array<var> steps;

	for (auto step : installActions)
		steps.add(step->toJSON());

	installLog->setProperty("Steps", var(steps));

	return var(installLog.get());
}

std::map<juce::Identifier, std::map<String, String>> HiseAssetInstaller::getPreprocessors(const std::map<Identifier, String>& p)
{
	std::array<Identifier, 4> keys =
	{
		HiseSettings::Project::ExtraDefinitionsWindows,
		HiseSettings::Project::ExtraDefinitionsOSX,
		HiseSettings::Project::ExtraDefinitionsLinux,
		HiseSettings::Project::ExtraDefinitionsNetworkDll
	};

	std::map<Identifier, std::map<String, String>> rv;

	for (auto& key : keys)
	{
		if (p.find(key) != p.end())
		{
			auto tokens = StringArray::fromLines(p.at(key));

			std::map<String, String> nv;

			for (const auto& t : tokens)
			{
				auto key = t.upToFirstOccurrenceOf("=", false, false);
				auto value = t.fromFirstOccurrenceOf("=", false, false);

				if (key.isNotEmpty() && value.isNotEmpty())
					nv[key] = value;
			}

			rv[key] = nv;
		}
		else
		{
			rv[key] = {};
		}
	}

	return rv;
}

Array<std::pair<int, juce::File>> HiseAssetInstaller::getFileList() const
{
	Array<std::pair<int, File>> results;

	if (isFileBased())
	{
		auto sourceFolder = getSourceRootFolder();

		int index = 0;
		auto allFiles = sourceFolder.findChildFiles(File::findFiles, true, "*");

		for (auto f : allFiles)
		{
			index++;
			auto includeFile = matchesFile(f);

			if (includeFile)
				results.add({ index - 1, f });
		}
	}
	else
	{
		for (int i = 0; i < zf->getNumEntries(); i++)
		{
			auto path = zf->getEntry(i)->filename;

			if (relativePath.isNotEmpty())
				path = path.fromFirstOccurrenceOf(relativePath, false, false);

			auto f = getRootFolder().getChildFile(path);

			auto includeFile = matchesFile(f);

			if (includeFile)
				results.add({ i, f });
		}
	}

	return results;
}

juce::File HiseAssetInstaller::getLogFile() const
{
	return getRootFolder().getChildFile("install_packages_log.json");
}

juce::InputStream* HiseAssetInstaller::getInputStream(int idx) const
{
	if (!isFileBased())
		return zf->createStreamForEntry(idx);

	return nullptr;
}



std::map<juce::Identifier, String> HiseAssetInstaller::getProjectInfo(bool getTarget) const
{
	std::map<Identifier, String> projectValues;

	std::unique_ptr<XmlElement> xml;

	if ((isFileBased() && !uninstallInfo.isLocalFolder()) || getTarget)
	{
		auto projectInfo = getRootFolder().getChildFile("project_info.xml");
		xml = XmlDocument::parse(projectInfo);
	}
	else
	{
		ScopedPointer<InputStream> is = createInputStreamFromSource("project_info.xml");

		if (is != nullptr)
		{
			MemoryOutputStream mos;
			mos.writeFromInputStream(*is, is->getTotalLength());
			mos.flush();
			auto xmlData = mos.toString();
			xml = XmlDocument::parse(xmlData);
		}
	}

	if (xml != nullptr)
	{
		for (auto* c : xml->getChildIterator())
		{
			auto key = c->getTagName();
			auto value = c->getStringAttribute("value");
			projectValues[key] = value;
		}
	};

	return projectValues;
}

juce::StringArray HiseAssetInstaller::getIllegalFilenames()
{
	return {
		".gitignore",
		"expansion_info.xml",
		"project_info.xml",
		"user_info.xml",
		"RSA.xml",
		"package_install.json",
		"install_packages_log.json",
		"Readme.md"
	};
}

void HiseAssetInstaller::writeToLog(const String& message)
{
	DBG(message);

	debugToConsole(getMainController()->getMainSynthChain(), message);
}

juce::StringArray HiseAssetInstaller::createArray(const var& obj, const char* id)
{
	StringArray sa;

	if (auto ar = obj[id].getArray())
	{
		for (auto& v : *ar)
			sa.add(v);
	}

	sa.removeDuplicates(true);
	sa.removeEmptyStrings();
	return sa;
}

void HiseAssetInstaller::initialise(const var& jsonObject)
{
	positiveWildcards = (createArray(jsonObject, "PositiveWildcard"));
	negativeWildcards = (createArray(jsonObject, "NegativeWildcard"));
	preprocessorValues = (createArray(jsonObject, "Preprocessors"));
	fileTypes = (createArray(jsonObject, "FileTypes"));
	infoToShow = (jsonObject["InfoText"].toString());
	clipboardContent = jsonObject["ClipboardContent"].toString();

	auto& fh = getMainController()->getCurrentFileHandler();

	auto getSubDirectory = [&](ProjectHandler::SubDirectories d)
	{
		if(zf != nullptr)
			return fh.getSubDirectory(d);

		auto sd = fh.getSubDirectory(d);
		auto sourceRoot = getSourceRootFolder();
		if(sd.isAChildOf(sourceRoot))
			return sd;
		else
			return sourceRoot.getChildFile(sd.getRelativePathFrom(getRootFolder()));
	};
	

	if(fileTypes.isEmpty())
	{
		validSubdirectories.add(getSubDirectory(ProjectHandler::SubDirectories::AudioFiles));
		validSubdirectories.add(getSubDirectory(ProjectHandler::SubDirectories::AdditionalSourceCode));
		validSubdirectories.add(getSubDirectory(ProjectHandler::SubDirectories::Images));
		validSubdirectories.add(getSubDirectory(ProjectHandler::SubDirectories::Scripts));
		validSubdirectories.add(getSubDirectory(ProjectHandler::SubDirectories::Presets));
		validSubdirectories.add(getSubDirectory(ProjectHandler::SubDirectories::SampleMaps));
		validSubdirectories.add(getSubDirectory(ProjectHandler::SubDirectories::MidiFiles));
		validSubdirectories.add(getSubDirectory(ProjectHandler::SubDirectories::DspNetworks));
		validSubdirectories.add(getSubDirectory(ProjectHandler::SubDirectories::Samples));
	}
	else
	{
		for(auto ft: fileTypes)
		{
			auto sub = fh.getSubDirectoryForIdentifier(Identifier(ft));

			if(sub != ProjectHandler::SubDirectories::numSubDirectories)
				validSubdirectories.add(getSubDirectory(sub));
		}
	}
}

bool HiseAssetInstaller::matchesFile(const File& f) const
{
	if(f.isDirectory())
		return false;

	auto valid = false;

	if (f.getParentDirectory() == getSourceRootFolder())
		valid = true;

	if(f.getParentDirectory().getRelativePathFrom(getSourceRootFolder()).contains("Binaries"))
		return false;

	for (auto& v : validSubdirectories)
	{
		if (valid)
			break;

		if (f.isAChildOf(v))
			valid = true;
	}

	if (!valid)
		return false;

	if (getIllegalFilenames().contains(f.getFileName()))
		return false;

	auto includeFile = positiveWildcards.isEmpty();
	auto path = f.getRelativePathFrom(getRootFolder()).replaceCharacter('\\', '/');

	for (auto& pw : positiveWildcards)
	{
		if (pw.containsChar('*'))
			includeFile |= juce::WildcardFileFilter(pw, {}, {}).isFileSuitable(f);
		else
			includeFile |= path.contains(pw);

		if (includeFile)
			break;
	}

	if (includeFile)
	{
		for (auto& pw : negativeWildcards)
		{
			if (pw.containsChar('*'))
				includeFile &= !juce::WildcardFileFilter(pw, {}, {}).isFileSuitable(f);
			else
				includeFile &= !path.contains(pw);

			if (!includeFile)
				break;
		}
	}

	return includeFile;
}

bool HiseAssetInstaller::install(const UninstallInfo& infoToUse)
{
	if (! isValid)
	{
		writeToLog("FATAL: installer is not valid (missing or invalid package_install.json)");
		return false;
	}

	auto newSteps = createInstallList(infoToUse);

	var obj;

	auto lf = getLogFile();

	auto packageInfo = getProjectInfo(false);

	auto packageName = packageInfo[HiseSettings::Project::Name];
	auto version = packageInfo[HiseSettings::Project::Version];

	if(lf.existsAsFile())
	{
		auto parseResult = JSON::parse(lf.loadFileAsString(), obj);

		if (! parseResult.wasOk())
		{
			writeToLog("FATAL: install_packages_log.json is corrupted: " + parseResult.getErrorMessage());
			return false;
		}

		if (auto ar = obj.getArray())
		{
			for(auto& v: *ar)
			{
				if(v[HiseSettings::Project::Name].toString() == packageName)
				{
					writeToLog("FATAL: package " + packageName + " is already installed. Uninstall first.");
					return false;
				}
			}
		}
	}
	else
	{
		obj = var(Array<var>());
	}

	if (! testMode)
	{
		Array<File> claimedByLog;
		auto root = getRootFolder();

		if (auto ar = obj.getArray())
		{
			for (const auto& pkg : *ar)
			{
				if (auto steps = pkg["Steps"].getArray())
				{
					for (const auto& step : *steps)
					{
						if (step["Type"].toString() == FileInstallAction::getStaticId().toString())
							claimedByLog.add(root.getChildFile(step["Target"].toString()));
					}
				}
			}
		}

		StringArray conflicts;

		for (auto a : newSteps)
		{
			if (auto fa = dynamic_cast<FileInstallAction*>(a))
			{
				if (fa->wouldConflict(claimedByLog))
					conflicts.add(fa->targetFile.getFullPathName());
			}
		}

		if (! conflicts.isEmpty())
		{
			writeToLog("FATAL: install would overwrite " + String(conflicts.size()) + " untracked file(s):");
			for (const auto& c : conflicts)
				writeToLog("  " + c);
			writeToLog("Backup or rename these files and retry.");
			return false;
		}
	}

	if (!newSteps.isEmpty())
	{
		writeToLog(getHeader(getMainController(), infoToUse, "Installing "));
		
		for (auto n : newSteps)
		{
			if(!testMode)
			{
				auto ok = n->perform();

				if (!ok)
					return false;
			}

			writeToLog("TEST: " + n->toString(false));
		}
	}

	if(testMode)
	{
		return true;
	}
	
	auto newObj = toInstallLog(newSteps, infoToUse);

	if (auto ar = obj.getArray())
	{
		ar->add(newObj);
		return getLogFile().replaceWithText(JSON::toString(obj));
	}

	jassertfalse;
	return false;
}

HiseAssetInstaller::UninstallResult HiseAssetInstaller::uninstall(const Error& seekToError)
{
	UninstallResult result;

	auto logInfo = getInstallInfoFromLog({});

	if (! logInfo.getDynamicObject())
	{
		writeToLog("Package " + uninstallInfo.packageName + " is not installed.");
		result.success = false;
		return result;
	}

	auto uninstallSteps = fromInstallLog(logInfo);

	if (!uninstallSteps.isEmpty())
	{
		writeToLog(getHeader(getMainController(), uninstallInfo, "Deinstalling"));

		auto shouldSeek = (bool)seekToError.info;
		shouldSeek &= seekToError.info.packageName == uninstallInfo.packageName;
		shouldSeek &= seekToError.what != Error::OK;

		for (int i = uninstallSteps.size() - 1; i >= 0; i--)
		{
			auto msg = uninstallSteps[i]->toString(true);

			if(shouldSeek && !uninstallSteps[i]->matchesError(seekToError))
			{
				String skipMessage;
				skipMessage << "skip " << msg;
				writeToLog(skipMessage);
				continue;
			}

			writeToLog(msg);

			if(!uninstallSteps[i]->undo())
			{
				// undo() returning false on a FileInstallAction means the file
				// was modified by the user - collect it and continue
				if(auto f = dynamic_cast<FileInstallAction*>(uninstallSteps[i].get()))
				{
					result.skippedFiles.add(f->targetFile.getFullPathName());
					writeToLog("! Skipped (modified): " + f->targetFile.getFullPathName());
					continue;
				}

				// Non-file action failure is a real error
				result.success = false;
				return result;
			}
		}
	}

	var obj;
	auto ok = JSON::parse(getLogFile().loadFileAsString(), obj);

	if (auto ar = obj.getArray())
	{
		for (int i = 0; i < ar->size(); i++)
		{
			if ((*ar)[i][HiseSettings::Project::Name].toString() == uninstallInfo.packageName)
			{
				if (result.hasSkippedFiles())
				{
					// Leave a residual log entry with NeedsCleanup flag
					writeToLog("! " + String(result.skippedFiles.size()) + " modified file(s) skipped. Entering NeedsCleanup state.");

					auto entry = (*ar)[i].getDynamicObject();
					entry->setProperty("NeedsCleanup", true);

					Array<var> skippedList;
					for (const auto& sf : result.skippedFiles)
						skippedList.add(sf);

					entry->setProperty("SkippedFiles", var(skippedList));

					// Remove the Steps array since we've already undone what we could
					entry->removeProperty("Steps");
				}
				else
				{
					writeToLog("> Remove package info ...");
					ar->remove(i);
				}

				break;
			}
		}
	}

	auto ok2 = getLogFile().replaceWithText(JSON::toString(obj));

	if(ok2)
		writeToLog("... done.");

	result.success = true;
	return result;
}

bool HiseAssetInstaller::cleanup()
{
	auto logInfo = getInstallInfoFromLog({});

	if (!logInfo.getDynamicObject())
		return false;

	if (!(bool)logInfo["NeedsCleanup"])
		return false;

	StringArray remainingFiles;

	if (auto skippedArray = logInfo["SkippedFiles"].getArray())
	{
		writeToLog(getHeader(getMainController(), uninstallInfo, "Cleaning up"));

		for (const auto& sf : *skippedArray)
		{
			File f(sf.toString());

			if (f.existsAsFile())
			{
				if (f.deleteFile())
					writeToLog("> Deleted: " + f.getFullPathName());
				else
				{
					writeToLog("! Failed to delete: " + f.getFullPathName());
					remainingFiles.add(f.getFullPathName());
				}
			}
		}
	}

	var obj;
	auto ok = JSON::parse(getLogFile().loadFileAsString(), obj);

	if (auto ar = obj.getArray())
	{
		for (int i = 0; i < ar->size(); i++)
		{
			if ((*ar)[i][HiseSettings::Project::Name].toString() == uninstallInfo.packageName)
			{
				if (remainingFiles.isEmpty())
				{
					// All files deleted - remove the log entry entirely
					ar->remove(i);
				}
				else
				{
					// Some files couldn't be deleted - update the skipped list
					auto entry = (*ar)[i].getDynamicObject();
					Array<var> remaining;
					for (const auto& rf : remainingFiles)
						remaining.add(rf);
					entry->setProperty("SkippedFiles", var(remaining));
				}

				break;
			}
		}
	}

	auto ok2 = getLogFile().replaceWithText(JSON::toString(obj));

	if (ok2 && remainingFiles.isEmpty())
		writeToLog("... cleanup done.");
	else if (!remainingFiles.isEmpty())
		writeToLog("! " + String(remainingFiles.size()) + " file(s) could not be deleted.");

	return ok2 && remainingFiles.isEmpty();
}

// hasLocalChanges and checkLocalChanges removed (poor man's git cleanup)
#if 0

	Array<std::pair<File, LineDiff::HashedDiff::Ptr>> reports;

	for (auto l : list)
	{
		if (auto f = dynamic_cast<FileInstallAction*>(l))
		{
			if(f->isChanged())
				return true;
		}
	}

	return false;
}

Array<std::pair<juce::File, LineDiff::HashedDiff::Ptr>> HiseAssetInstaller::checkLocalChanges()
{
	auto log = getInstallInfoFromLog(uninstallInfo);
	auto list = fromInstallLog(log);

	Array<std::pair<File, LineDiff::HashedDiff::Ptr>> reports;

	for(auto l: list)
	{
		if(auto f = dynamic_cast<FileInstallAction*>(l))
		{
			if(auto report = f->getDiffReport())
				reports.add({f->targetFile, report});
		}
	}

	return reports;
}
#endif

HiseAssetInstaller::PreprocessorInstallAction::PreprocessorInstallAction(MainController* mc, const StringArray& keys, PreprocessorMap& o, const PreprocessorMap& n) :
	UndoableInstallAction(mc),
	changedValues(new DynamicObject())
{
	auto findFirst = [](const PreprocessorMap& m, const String& key) -> var
	{
		for (const auto& slot : m)
		{
			auto it = slot.second.find(key);
			if (it != slot.second.end())
				return var(it->second);
		}
		return var();
	};

	for (const auto& key : keys)
	{
		auto sourceValue = findFirst(n, key);

		if (sourceValue.isVoid())
			continue;

		auto targetValue = findFirst(o, key);

		Array<var> values;
		values.add(targetValue);
		values.add(sourceValue);
		changedValues->setProperty(Identifier(key), var(values));
	}
}

HiseAssetInstaller::PreprocessorInstallAction::PreprocessorInstallAction(MainController* mc, const var& changes) :
	UndoableInstallAction(mc)
{
	jassert(changes["Type"].toString() == getStaticId().toString());
	changedValues = changes["Data"].getDynamicObject()->clone();
}

String HiseAssetInstaller::PreprocessorInstallAction::toString(bool getUndoDescription) const
{
	String t;

	for (auto& nv : changedValues->getProperties())
	{
		t << "> Set ";
		t << nv.name;
		t << ": ";

		if (getUndoDescription)
		{
			if (nv.value[0].isVoid())
				t << "[remove preprocessor]";
			else
				t << nv.value[0].toString();
		}
		else
			t << nv.value[1].toString();

		t << "\n";
	}

	return t;
}

juce::var HiseAssetInstaller::PreprocessorInstallAction::toJSON() const
{
	DynamicObject::Ptr obj = new DynamicObject();
	obj->setProperty("Type", getStaticId().toString());
	obj->setProperty("Data", changedValues->clone().get());
	return var(obj.get());
}

HiseAssetInstaller::FileInstallAction::FileInstallAction(MainController* mc, const File& targetFile_, InputStream* inputStream) :
	UndoableInstallAction(mc),
	is(inputStream),
	targetFile(targetFile_)
{}

HiseAssetInstaller::FileInstallAction::FileInstallAction(MainController* mc, const var& data) :
	UndoableInstallAction(mc),
	is(nullptr),
	targetFile(getRootFolder().getChildFile(data["Target"].toString())),
	modificationTime(Time::fromISO8601(data["Modified"].toString()))
{
	jassert(data["Type"].toString() == getStaticId().toString());

	// Accept BOTH legacy numeric form and new string form. JUCE's (int64) var
	// on a string already routes through getLargeIntValue() so the cast works
	// for both shapes. hashValid distinguishes "field present" (current or
	// legacy entry) from "field absent" (legacy entry from before the file's
	// extension was text-classified - treat as binary on uninstall).
	if (auto obj = data.getDynamicObject())
	{
		if (obj->hasProperty("Hash"))
		{
			const auto hashVar = data["Hash"];

			if (hashVar.isString())
				hash = hashVar.toString().getLargeIntValue();
			else
				hash = (int64)hashVar;

			hashValid = true;
		}
	}
}

bool HiseAssetInstaller::FileInstallAction::perform()
{
	if(is != nullptr && !targetFile.isDirectory() && is->getTotalLength() != 0)
	{
		targetFile.deleteFile();
		targetFile.create();

		int numWritten = 0;

		{
			FileOutputStream fos(targetFile);
			numWritten = fos.writeFromInputStream(*is, (int)is->getTotalLength());
			fos.flush();
		}
		
		auto ok = numWritten == is->getTotalLength();

		if(ok && LineDiff::isTextFile(targetFile))
		{
			hash = targetFile.loadFileAsString().hashCode64();
			hashValid = true;
		}

		return ok;
	}

	return true;
}



String HiseAssetInstaller::FileInstallAction::toString(bool getUndoDescription) const
{
	auto path = targetFile.getFullPathName();
	return (getUndoDescription ? "> Delete " : "> Extract ") + path.replaceCharacter('\\', '/');
}

juce::var HiseAssetInstaller::FileInstallAction::toJSON() const
{
	DynamicObject::Ptr obj = new DynamicObject();
	obj->setProperty("Type", getStaticId().toString());

	auto path = targetFile.getRelativePathFrom(getRootFolder());

	obj->setProperty("Target", path.replaceCharacter('\\', '/'));
	
	if(LineDiff::isTextFile(targetFile))
	{
		auto hashToUse = hashValid ? hash : targetFile.loadFileAsString().hashCode64();

		// Serialize as decimal string: 64-bit hashes can exceed 2^53 and lose
		// precision when read by a standard JSON parser (e.g. JS JSON.parse).
		obj->setProperty("Hash", String(hashToUse));
	}
	
	obj->setProperty("Modified", targetFile.getLastModificationTime().toISO8601(false));
	return var(obj.get());
}

bool HiseAssetInstaller::FileInstallAction::undo()
{
	if(! targetFile.existsAsFile())
		return true;

	// Legacy compat: if the file is text-classified now but the log entry has
	// no recorded hash, the entry was written by pre-fix HISE before this
	// extension was text-classified. Treat as binary - delete unconditionally.
	if(hashValid && LineDiff::isTextFile(targetFile))
	{
		auto currentHash = targetFile.loadFileAsString().hashCode64();

		if(currentHash != hash)
		{
			// File was modified - signal skip by returning false
			return false;
		}
	}

	return targetFile.deleteFile();
}




}
