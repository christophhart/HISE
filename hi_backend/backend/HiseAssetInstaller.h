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

#pragma once

namespace hise {
using namespace juce;



/** TODO:

- MOVE Installer to new file & cleanup
- add install / uninstall routine
- add download task
- add project asset info file
- add step system for performing download operations
- add proper change log
- add manage button: uninstall, update, revert, add
- add project listener logic (including display of current project)
- add dummy test project with stuff
- add create install.json from current project
- add file modification logic (warn if overriding custom file)

*/

/** TODO:
	
- make constructor with ZipFile that loads the install package into an memoryInputStream OK
- use common SubDirectories to prefilter junk (only Images, SampleMaps, Samples, AudioFiles, Scripts, AdditionalSourceCode, DspNetworks)
- use those subdirectories instead of the FolderType filter


*/
struct HiseAssetInstaller: public ControlledObject
{
	

	struct UninstallInfo
	{
		struct LocalCollection
		{
			LocalCollection()
			{
				auto list = JSON::parse(getLocalAssetFile());

				if (!list.isArray())
					list = var(Array<var>());

				for (const auto& v : *list.getArray())
				{
					auto path = v.toString();

					if (File::isAbsolutePath(path))
					{
						if (auto info = HiseAssetInstaller::UninstallInfo(File(path)))
						{
							auto exists = std::any_of(assets.begin(), assets.end(),
								[&](const UninstallInfo& a) { return a.localFolder == info.localFolder; });

							if (! exists)
								assets.push_back(info);
						}
					}
				}
			}

			const UninstallInfo* begin() const { return assets.data(); }
			const UninstallInfo* end() const { return assets.data() + assets.size(); }

			File getLocalAssetFile()
			{
				return ProjectHandler::getAppDataDirectory(nullptr).getChildFile("localAssetFolders.js");
			}

			void resave()
			{
				Array<var> newList;

				for (const auto& a : assets)
					newList.add(a.localFolder.getFullPathName());

				getLocalAssetFile().replaceWithText(JSON::toString(var(newList), false));
			}

			std::vector<UninstallInfo> assets;
		};

		enum class Mode
		{
			Undefined,
			UninstallOnly,
			StoreDownload,
			LocalFolder
		};

		static StringArray getModeNames() {
			return {
				"Undefined",
				"UninstallOnly",
				"StoreDownload",
				"LocalFolder"
			};
		}

		UninstallInfo(const var& obj)
		{
			if(obj.hasProperty("owner"))
			{
                m = Mode::StoreDownload;
				packageName = obj["name"].toString();
				vendor = obj["owner"]["username"].toString();
				localFolder = File();
			}
			else
			{
				auto modeString = obj["Mode"].toString();

				packageName = obj[HiseSettings::Project::Name].toString();
				installedVersion = obj[HiseSettings::Project::Version].toString();
				vendor = obj[HiseSettings::User::Company].toString();

				if(modeString == "LocalFolder")
				{
					SharedResourcePointer<LocalCollection> collection;

					for(const auto& lc: *collection)
					{
						if(lc.packageName == packageName)
						{
							localFolder = lc.localFolder;
							break;
						}
					}

					m = Mode::LocalFolder;
				}
				else if (modeString == "StoreDownload")
				{
					m = Mode::StoreDownload;
				}
			}
		}

		UninstallInfo(const File& f):
		  m(Mode::LocalFolder),
		  localFolder(f.getFullPathName())
		{
			if(auto xml = XmlDocument::parse(f.getChildFile("project_info.xml")))
			{
				if(auto pn = xml->getChildByName(HiseSettings::Project::Name))
					packageName = pn->getStringAttribute("value");

				if (auto pn = xml->getChildByName(HiseSettings::Project::Version))
					availableVersion = pn->getStringAttribute("value");
			}
			else
			{
				m = Mode::Undefined;
			}

			if (auto xml = XmlDocument::parse(f.getChildFile("user_info.xml")))
			{
				if (auto pn = xml->getChildByName(HiseSettings::User::Company))
					vendor = pn->getStringAttribute("value");
			}
		}

        UninstallInfo() {};
		UninstallInfo(const UninstallInfo& other) = default;
		UninstallInfo& operator=(const UninstallInfo& other) = default;

		operator bool() const { return m != Mode::Undefined; }

		var toLogInfo() const
		{
			auto no = new DynamicObject();
			no->setProperty(HiseSettings::Project::Name, packageName);
			no->setProperty(HiseSettings::User::Company, vendor);
			no->setProperty("Mode", getModeNames()[(int)m]);

			if(isInstalled())
				no->setProperty(HiseSettings::Project::Version, installedVersion);

			return var(no);
			
		}

		bool isUpdateAvailable() const
		{
			if(!isInstalled())
				return availableVersion.isNotEmpty();

			return SemanticVersionChecker(installedVersion, availableVersion).isUpdate();
		}

		bool matchesVersion(const String& version) const
		{
			return version == installedVersion;
		}

		bool isInstalled() const { return installedVersion.isNotEmpty(); };

		bool isLocalFolder() const { return m == Mode::LocalFolder; }

		Mode m = Mode::Undefined;

		File localFolder;
		String packageName;
		String vendor;
		String installedVersion;
		String availableVersion;
	};

	struct Error
	{
		enum ErrorType
		{
			OK,
			HashMismatch,
			FileWriteError,
			numErrorTypes
		};

        Error() {};

		ErrorType what = ErrorType::OK;
		File file;
		UninstallInfo info;
	};

	struct UndoableInstallAction: public ReferenceCountedObject,
									public ControlledObject
	{
		using List = ReferenceCountedArray<UndoableInstallAction>;
		using Ptr = ReferenceCountedObjectPtr<UndoableInstallAction>;

		UndoableInstallAction(MainController* mc):
			ControlledObject(mc)
		{};

		virtual ~UndoableInstallAction() {};

		virtual String toString(bool getUndoDescription) const = 0;
		virtual var toJSON() const = 0;

		virtual bool matchesError(const Error& e) const = 0;

		virtual bool perform() = 0;
		virtual bool undo() = 0;

		JUCE_DECLARE_WEAK_REFERENCEABLE(UndoableInstallAction);
	};

	HiseAssetInstaller(MainController* mc, const UninstallInfo& uninstallInfo);

	HiseAssetInstaller(MainController* mc, const var& jsonObject);

	HiseAssetInstaller(MainController* mc, ZipFile* archive);

	
	/** This creates a list of installation steps that can be performed and reversed. */
	UndoableInstallAction::List createInstallList(const UninstallInfo& infoToUse);

	bool install(const UninstallInfo& infoToUse);

	struct UninstallResult
	{
		bool success = false;
		StringArray skippedFiles;
		bool hasSkippedFiles() const { return !skippedFiles.isEmpty(); }
	};

	UninstallResult uninstall(const Error& seekToError=Error());

	/** Force-deletes files left over from a partial uninstall and removes the log entry. */
	bool cleanup();

	var getInstallInfoFromLog(const UninstallInfo& infoToUse);

	void addLocalFolder()
	{
		if(getInstallInfoFromLog(uninstallInfo).getDynamicObject() != nullptr)
			return;

		auto list = JSON::parse(getLogFile());

		if(auto ar = list.getArray())
		{
			ar->add(uninstallInfo.toLogInfo());
		}

		getLogFile().replaceWithText(JSON::toString(list));
	}

	void setTestMode() { testMode = true; }

private:

	File getRootFolder() const;

	var toInstallLog(UndoableInstallAction::List installActions, UninstallInfo infoToUse) const;

	

	UndoableInstallAction::List fromInstallLog(const var& obj);

	UninstallInfo uninstallInfo;

	struct ProjectSettingInstallAction: public UndoableInstallAction
	{
		static Identifier getStaticId() { RETURN_STATIC_IDENTIFIER("ProjectSetting"); }

		ProjectSettingInstallAction(MainController* mc, DynamicObject::Ptr newValues_):
		  UndoableInstallAction(mc),
		  newValues(newValues_),
		  oldValues(new DynamicObject())
		{
			auto root = getMainController()->getCurrentFileHandler().getRootFolder();
			auto projectInfo = root.getChildFile("project_info.xml");

			if (auto xml = XmlDocument::parse(projectInfo))
			{
				for (const auto& nv : newValues->getProperties())
				{
					if(auto c = xml->getChildByName(nv.name.toString()))
						oldValues->setProperty(nv.name, c->getStringAttribute("value"));
					else
						oldValues->setProperty(nv.name, var(""));
				}
			}
			else
			{
				for(auto& nv: newValues->getProperties())
					oldValues->setProperty(nv.name, var(""));
			}
		}

		ProjectSettingInstallAction(MainController* mc, const var& changes):
		  UndoableInstallAction(mc),
		  oldValues(changes["oldValues"].getDynamicObject()),
		  newValues(changes["newValues"].getDynamicObject())
		{}

		bool matchesError(const Error& e) const override { return false; }

		bool applyChanges(bool shouldUndo)
		{
			auto root = getMainController()->getCurrentFileHandler().getRootFolder();

			auto projectInfo = root.getChildFile("project_info.xml");
			
			auto xml = XmlDocument::parse(projectInfo);

			if (xml == nullptr)
				xml.reset(new XmlElement("ProjectSettings"));

			auto obj = shouldUndo ? oldValues : newValues;

			for(const auto& nv: obj->getProperties())
			{
				auto c = xml->getChildByName(nv.name.toString());

				if(c == nullptr)
				{
					c = new XmlElement(nv.name.toString());
					xml->addChildElement(c);
				}

				c->setAttribute("value", nv.value.toString());
			}

			if(projectInfo.replaceWithText(xml->createDocument("")))
				dynamic_cast<GlobalSettingManager*>(getMainController())->getSettingsObject().refreshProjectData();

			return true;
		}

		bool perform() override
		{
			return applyChanges(false);
		}

		bool undo() override
		{
			return applyChanges(true);
		}

		String toString(bool getUndoDescription) const override
		{
			auto obj = getUndoDescription ? oldValues : newValues;

			String msg;

			for(const auto& nv: obj->getProperties())
				msg << "> Set " << nv.name << ": " << nv.value.toString() << "\n";

			return msg;
		}

		var toJSON() const override 
		{
			DynamicObject::Ptr no = new DynamicObject();

			no->setProperty("Type", getStaticId().toString());

			auto ov = oldValues->clone();
			auto nv = newValues->clone();

			no->setProperty("oldValues", var(ov.get()));
			no->setProperty("newValues", var(nv.get()));

			return var(no.get());

		}

		DynamicObject::Ptr oldValues, newValues;
	};

	struct PreprocessorInstallAction : public UndoableInstallAction
	{
		using PreprocessorMap = std::map<Identifier, std::map<String, String>>;

		static Identifier getStaticId() { RETURN_STATIC_IDENTIFIER("Preprocessor"); }

		PreprocessorInstallAction(MainController* mc, const StringArray& keys, PreprocessorMap& o, const PreprocessorMap& n);

		PreprocessorInstallAction(MainController* mc, const var& changes);

		String toString(bool getUndoDescription) const override;

		bool matchesError(const Error& e) const override { return false; }

		bool applyChanges(int valueIndex)
		{
			std::map<juce::Identifier, String> projectValues;
			
			auto root = getMainController()->getCurrentFileHandler().getRootFolder();

			auto projectInfo = root.getChildFile("project_info.xml");
			auto xml = XmlDocument::parse(projectInfo);

			if(xml == nullptr)
				xml.reset(new XmlElement("ProjectSettings"));

			for (auto* c : xml->getChildIterator())
			{
				auto key = c->getTagName();
				auto value = c->getStringAttribute("value");
				projectValues[key] = value;
			}
			
			auto platforms = getPreprocessors(projectValues);

			for(auto& pl: platforms)
			{
				for(const auto& nv: changedValues->getProperties())
				{
					auto key = nv.name.toString();
					auto v = nv.value[valueIndex];

					if(v.isVoid() || v.isUndefined())
						pl.second.erase(key);
					else
						pl.second[key] = v.toString();
				}

				String lines;
				for(const auto& v: pl.second)
				{
					lines << v.first << "=" << v.second << "\n";
				}

				projectValues[pl.first] = lines;

				if(auto existing = xml->getChildByName(pl.first.toString()))
					existing->setAttribute("value", lines);
				else
				{
					auto ne = new XmlElement(pl.first.toString());
					ne->setAttribute("value", lines);
					xml->addChildElement(ne);
				}
			}

			auto ok = projectInfo.replaceWithText(xml->createDocument(""));

			dynamic_cast<GlobalSettingManager*>(getMainController())->getSettingsObject().refreshProjectData();

			return ok;
		}

		bool perform() override
		{
			return applyChanges(1);
		}

		bool undo() override
		{
			return applyChanges(0);
		}

		var toJSON() const override;

		DynamicObject::Ptr changedValues;
	};

	struct InfoShowAction: public UndoableInstallAction
	{
		static Identifier getStaticId() { RETURN_STATIC_IDENTIFIER("Info"); }

		InfoShowAction(MainController* mc, const String& message, const UninstallInfo& info_):
		  UndoableInstallAction(mc),
		  msg(message),
		  info(info_)
		{};

		bool matchesError(const Error& e) const override
		{
			return false;
		}

		bool perform() override
		{
			return true;
		}

		bool undo() override { return true; }

		UninstallInfo info;
		String msg;

		var toJSON() const override {
			auto no = new DynamicObject();
			no->setProperty("Type", getStaticId().toString());
			return var(no);
		}

		String toString(bool getUndo) const override 
		{ 
			String s;

			if(!getUndo)
			{
				s << getHeader(getMainController(), info, "Install instructions for");
				s << msg;
			}

			return s;
		}
	};

	static String getHeader(const MainController* mc, const UninstallInfo& info, const String& prefix)
	{
		String s;
		s << "\n";
		s << "\n\t" << prefix << " " << info.packageName + " " + info.installedVersion;
		auto length = s.length() - 3;
		s << "\n\t";

		for (int i = 0; i < length; i++)
			s << '=';

		s << "\n";
		return s;

	}

	struct FileInstallAction : public UndoableInstallAction
	{
		static Identifier getStaticId() { RETURN_STATIC_IDENTIFIER("File"); }

		FileInstallAction(MainController* mc, const File& targetFile_, InputStream* inputStream);

		FileInstallAction(MainController* mc, const var& data);

		bool matchesError(const Error& e) const override
		{
			auto typeMatches = e.what == Error::FileWriteError || e.what == Error::HashMismatch;
			auto fileMatches = e.file == targetFile;
			return typeMatches && fileMatches;
		}

		bool perform() override;

		bool undo() override;

		File getRootFolder() const
		{
			return getMainController()->getCurrentFileHandler().getRootFolder();
		}

		String toString(bool getUndoDescription) const override;

		var toJSON() const override;

		bool wouldConflict(const Array<File>& claimedByLog) const
		{
			return targetFile.existsAsFile() && ! claimedByLog.contains(targetFile);
		}

		File targetFile;
		ScopedPointer<InputStream> is;
		Time modificationTime;
		int64 hash = 0;
		bool hashValid = false;
	};

	struct ClipboardInstallAction: public UndoableInstallAction
	{
		static Identifier getStaticId() { RETURN_STATIC_IDENTIFIER("Clipboard"); }

		ClipboardInstallAction(MainController* mc, const String& clipboardContent):
		  UndoableInstallAction(mc),
		  content(clipboardContent)
		{
		}

		bool perform() override
		{
			SystemClipboard::copyTextToClipboard(content);
			return true;
		}

		bool matchesError(const Error& e) const override { return false; }

		bool undo() override { return true; }

		String toString(bool getUndoDescription) const override
		{
			return {};
		}

		var toJSON() const override {
			auto no = new DynamicObject();
			no->setProperty("Type", getStaticId().toString());
			return var(no);
		}

		String content;
	};

	static std::map<Identifier, std::map<String, String>> getPreprocessors(const std::map<Identifier, String>& p);

	Array<std::pair<int, File>> getFileList() const;

	File getLogFile() const;

	InputStream* getInputStream(int idx) const;

	InputStream* createInputStreamFromSource(const String& relativeFilePath) const
	{
		if(uninstallInfo.isLocalFolder())
		{
			auto sourcePath = uninstallInfo.localFolder;
			
			if(relativePath.isNotEmpty())
				sourcePath = sourcePath.getChildFile(relativePath);
				
			sourcePath = sourcePath.getChildFile(relativeFilePath);

			if(sourcePath.existsAsFile())
				return new FileInputStream(sourcePath);
		}

		if(zf != nullptr)
		{
			auto idx = zf->getIndexOfFileName(relativePath + "project_info.xml");
			return zf->createStreamForEntry(idx);
		}

		return nullptr;
	}

	std::map<Identifier, String> getProjectInfo(bool getTarget) const;

	/** Checks whether the installer payload is based on a project directory or a Zip file. */
	bool isFileBased() const { return zf == nullptr; }

	File getSourceRootFolder() const
	{
		if(zf != nullptr)
			return getRootFolder();

		if(uninstallInfo.localFolder.isDirectory())
			return uninstallInfo.localFolder;
		else
		{
			jassert(testMode);
			return getRootFolder();
		}
	}

	bool testMode = false;
	bool isValid = true;

	static StringArray getIllegalFilenames();

	void writeToLog(const String& message);

	static StringArray createArray(const var& obj, const char* id);

	void initialise(const var& jsonObject);

	bool matchesFile(const File& f) const;

	ZipFile* zf = nullptr;

	Array<File> validSubdirectories;

	StringArray positiveWildcards;
	StringArray negativeWildcards;
	StringArray preprocessorValues;
	StringArray fileTypes;

	String relativePath;
	String clipboardContent;
	String infoToShow;

	
};



}
