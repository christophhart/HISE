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
*   Commercial licences for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licencing:
*
*   http://www.hartinstruments.net/hise/
*
*   HISE is based on the JUCE library,
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/



static bool canConnectToWebsite(const URL& url)
{
	ScopedPointer<InputStream> in(url.createInputStream(false, nullptr, nullptr, String(), 2000, nullptr));
	return in != nullptr;
}

static bool areMajorWebsitesAvailable()
{
	if (canConnectToWebsite(URL("http://hartinstruments.net")))
		return true;

	return false;
}



class UpdateChecker : public ThreadWithAsyncProgressWindow
{
public:

	class ScopedTempFile
	{
	public:

		ScopedTempFile(const File &f_) :
			f(f_)
		{
			f.deleteFile();

			jassert(!f.existsAsFile());

			f.create();
		}

		~ScopedTempFile()
		{
			jassert(f.existsAsFile());

			jassert(f.deleteFile());
		}


		File f;
	};

	UpdateChecker() :
		ThreadWithAsyncProgressWindow("Checking for newer version."),
		updatesAvailable(false),
		lastUpdate(BUILD_SUB_VERSION)
	{


		String changelog = getAllChangelogs();

		if (updatesAvailable)
		{
			changelogDisplay = new TextEditor("Changelog");
			changelogDisplay->setSize(500, 300);

			changelogDisplay->setFont(GLOBAL_MONOSPACE_FONT());
			changelogDisplay->setMultiLine(true);
			changelogDisplay->setReadOnly(true);
			changelogDisplay->setText(changelog);
			addCustomComponent(changelogDisplay);

			filePicker = new FilenameComponent("Download Location", File::getSpecialLocation(File::SpecialLocationType::userDesktopDirectory), false, true, true, "", "", "Choose Download Location");
			filePicker->setSize(500, 24);

			addCustomComponent(filePicker);

			addBasicComponents();

			showStatusMessage("New build available: " + String(lastUpdate) + ". Press OK to download file to the selected location");
		}
		else
		{
			addBasicComponents(false);

			showStatusMessage("Your HISE build is up to date.");
		}
	}

	static bool downloadProgress(void* context, int bytesSent, int totalBytes)
	{
		const double downloadedMB = (double)bytesSent / 1024.0 / 1024.0;
		const double totalMB = (double)totalBytes / 1024.0 / 1024.0;
		const double percent = downloadedMB / totalMB;

		static_cast<UpdateChecker*>(context)->showStatusMessage("Downloaded: " + String(downloadedMB, 2) + " MB / " + String(totalMB, 2) + " MB");

		static_cast<UpdateChecker*>(context)->setProgress(percent);

		return !static_cast<UpdateChecker*>(context)->threadShouldExit();
	}

	void run()
	{
		static String webFolder("http://hartinstruments.net/hise/download/nightly_builds/");

		static String version = "099";

#if JUCE_WINDOWS
		String downloadFileName = "HISE_" + version + "_build" + String(lastUpdate) + ".exe";
#else
		String downloadFileName = "HISE_" + version + "_build" + String(lastUpdate) + "_OSX.dmg";
#endif

		URL url(webFolder + downloadFileName);

		ScopedPointer<InputStream> stream = url.createInputStream(false, &downloadProgress, this);

		target = File(filePicker->getCurrentFile().getChildFile(downloadFileName));

		if (!target.existsAsFile())
		{
			MemoryBlock mb;

			mb.setSize(8192);

			tempFile = new ScopedTempFile(File(target.getFullPathName() + "temp"));

			ScopedPointer<FileOutputStream> fos = new FileOutputStream(tempFile->f);

			const int64 numBytesTotal = stream->getNumBytesRemaining();

			int64 numBytesRead = 0;

			downloadOK = false;

			while (stream->getNumBytesRemaining() > 0)
			{
				const int64 chunkSize = jmin<int64>(stream->getNumBytesRemaining(), 8192);

				downloadProgress(this, (int)numBytesRead, (int)numBytesTotal);

				if (threadShouldExit())
				{
					fos->flush();
					fos = nullptr;

					tempFile = nullptr;
					return;
				}

				stream->read(mb.getData(), (int)chunkSize);

				numBytesRead += chunkSize;

				fos->write(mb.getData(), (size_t)chunkSize);
			}

			downloadOK = true;
			fos->flush();

			tempFile->f.copyFileTo(target);
		}
	};

	void threadFinished()
	{
		if (downloadOK)
		{
			PresetHandler::showMessageWindow("Download finished", "Quit the app and run the installer to update to the latest version", PresetHandler::IconType::Info);

			target.revealToUser();
		}
	}

private:

	String getAllChangelogs()
	{
		int latestVersion = BUILD_SUB_VERSION + 1;

		String completeLog;

		bool lookFurther = true;

		while (lookFurther)
		{
			String log = getChangelog(latestVersion);

			if (log == "404")
			{
				lookFurther = false;
				continue;
			}

			updatesAvailable = true;

			lastUpdate = latestVersion;

			completeLog << log;
			latestVersion++;
		}

		return completeLog;
	}

	String getChangelog(int i) const
	{
		static String webFolder("http://www.hartinstruments.net/hise/download/nightly_builds/");

		String changelogFile("changelog_" + String(i) + ".txt");

		URL url(webFolder + changelogFile);

		String content;

		ScopedPointer<InputStream> stream = url.createInputStream(false);

		if (stream != nullptr)
		{
			String changes = stream->readEntireStreamAsString();

			if (changes.contains("404"))
			{
				return "404";
			}
			else if (changes.containsNonWhitespaceChars())
			{
				content = "Build " + String(i) + ":\n\n" + changes;
				return content;
			}

			return String();
		}
		else
		{
			return "404";
		}
	}

	bool updatesAvailable;

	int lastUpdate;

	File target;
	ScopedPointer<ScopedTempFile> tempFile;

	bool downloadOK;

	ScopedPointer<FilenameComponent> filePicker;
	ScopedPointer<TextEditor> changelogDisplay;
};


struct XmlBackupFunctions
{
	static void removeEditorStatesFromXml(XmlElement &xml)
	{
		xml.deleteAllChildElementsWithTagName("EditorStates");

		for (int i = 0; i < xml.getNumChildElements(); i++)
		{
			removeEditorStatesFromXml(*xml.getChildElement(i));
		}
	}

	static void removeAllScripts(XmlElement &xml)
	{
		xml.removeAttribute("Script");

		for (int i = 0; i < xml.getNumChildElements(); i++)
		{
			removeAllScripts(*xml.getChildElement(i));
		}
	}

	static void restoreAllScripts(XmlElement &xml, ModulatorSynthChain *masterChain, const String &newId)
	{
		if (xml.hasTagName("Processor") && xml.getStringAttribute("Type").contains("Script"))
		{
			File scriptDirectory = getScriptDirectoryFor(masterChain, newId);

			DirectoryIterator iter(scriptDirectory, false, "*.js", File::findFiles);

			while (iter.next())
			{
				File script = iter.getFile();

				if (script.getFileNameWithoutExtension() == getSanitiziedName(xml.getStringAttribute("ID")))
				{
					xml.setAttribute("Script", script.loadFileAsString());
					break;
				}
			}
		}

		for (int i = 0; i < xml.getNumChildElements(); i++)
		{
			restoreAllScripts(*xml.getChildElement(i), masterChain, newId);
		}
	}

	static File getScriptDirectoryFor(ModulatorSynthChain *masterChain, const String &chainId = String())
	{
		if (chainId.isEmpty())
		{
			return GET_PROJECT_HANDLER(masterChain).getSubDirectory(ProjectHandler::SubDirectories::Scripts).getChildFile("ScriptProcessors/" + getSanitiziedName(masterChain->getId()));
		}
		else
		{
			return GET_PROJECT_HANDLER(masterChain).getSubDirectory(ProjectHandler::SubDirectories::Scripts).getChildFile("ScriptProcessors/" + getSanitiziedName(chainId));
		}
	}

	static File getScriptFileFor(ModulatorSynthChain *masterChain, const String &id)
	{
		return getScriptDirectoryFor(masterChain).getChildFile(getSanitiziedName(id) + ".js");
	}

private:

	static String getSanitiziedName(const String &id)
	{
		return id.removeCharacters(" .()");
	}
};


class DummyUnlocker : public OnlineUnlockStatus
{
public:

	DummyUnlocker(ProjectHandler *handler_) :
		handler(handler_)
	{

	}

	String getProductID() override
	{
		return SettingWindows::getSettingValue((int)SettingWindows::ProjectSettingWindow::Attributes::Name, handler);
	}

	bool doesProductIDMatch(const String & 	returnedIDFromServer)
	{
		return returnedIDFromServer == getProductID();
	}




private:

	ProjectHandler *handler;


};


class ProjectArchiver : public ThreadWithQuasiModalProgressWindow
{
public:

	ProjectArchiver(File &archiveFile_, File &projectDirectory_, ThreadWithQuasiModalProgressWindow::Holder *holder) :
		ThreadWithQuasiModalProgressWindow("Archiving Project", true, true, holder),
		archiveFile(archiveFile_),
		projectDirectory(projectDirectory_)
	{
		getAlertWindow()->setLookAndFeel(&alaf);
	}

	void run()
	{
		ZipFile::Builder builder;

		StringArray ignoredDirectories;

		ignoredDirectories.add("Binaries");
		ignoredDirectories.add("git");

		DirectoryIterator walker(projectDirectory, true, "*", File::findFilesAndDirectories);

		while (walker.next())
		{
			File currentFile = walker.getFile();

			if (currentFile.isDirectory() ||
				currentFile.getFullPathName().contains("git") ||
				currentFile.getParentDirectory().getFullPathName().contains("Binaries"))
			{
				continue;
			}

			builder.addFile(currentFile, 9, currentFile.getRelativePathFrom(projectDirectory));
		}

		setStatusMessage("Creating ZIP archive of project folder");

		archiveFile.deleteFile();

		archiveFile.create();

		FileOutputStream fos(archiveFile);

		builder.writeToStream(fos, getProgressValue());


	}

	void threadComplete(bool userPressedCancel) override
	{
		if (!userPressedCancel && PresetHandler::showYesNoWindow("Successfully exported", "Press OK to show the archive", PresetHandler::IconType::Info))
		{
			archiveFile.revealToUser();
		}

		ThreadWithQuasiModalProgressWindow::threadComplete(userPressedCancel);
	}

private:

	AlertWindowLookAndFeel alaf;

	File archiveFile;

	File projectDirectory;
};

class ProjectDownloader : public ThreadWithAsyncProgressWindow,
	public TextEditor::Listener
{
public:

	enum class ErrorCodes
	{
		OK = 0,
		InvalidURL,
		URLNotFound,
		DirectoryAlreadyExists,
		FileNotAnArchive,
		AbortedByUser,
		numErrorCodes
	};

	ProjectDownloader(BackendProcessorEditor *bpe_) :
		ThreadWithAsyncProgressWindow("Download new Project"),
		bpe(bpe_),
		result(ErrorCodes::OK)
	{
		addTextEditor("url", "http://www.", "URL");

#if HISE_IOS

		addTextEditor("projectName", "Project", "Project Name");

#else

		targetFile = new FilenameComponent("Target folder", File(), true, true, true, "", "", "Choose target folder");
		targetFile->setSize(300, 24);
		addCustomComponent(targetFile);

#endif

		addBasicComponents(true);
		addButton("Paste URL from Clipboard", 2);
	};

	void resultButtonClicked(const String &name)
	{
		if (name == "Paste URL from Clipboard")
		{
			getTextEditor("url")->setText(SystemClipboard::getTextFromClipboard());
		}
	}

	void run() override
	{
#if HISE_IOS
		targetDirectory = File::getSpecialLocation(File::userDocumentsDirectory).getChildFile(getTextEditor("projectName")->getText());

		if (targetDirectory.isDirectory())
		{
			result = ErrorCodes::DirectoryAlreadyExists;
			return;
		}

		targetDirectory.createDirectory();

#else
		targetDirectory = targetFile->getCurrentFile();

		if (targetDirectory.isDirectory() && targetDirectory.getNumberOfChildFiles(File::findFilesAndDirectories) != 0)
		{
			result = ErrorCodes::DirectoryAlreadyExists;
			return;
		}

#endif

		const String enteredURL = getTextEditor("url")->getText();

		const String directURL = replaceHosterLinksWithDirectDownloadURL(enteredURL);

		URL downloadLocation(directURL);

		if (!downloadLocation.isWellFormed())
		{
			result = ErrorCodes::InvalidURL;
			targetDirectory.deleteRecursively();
			return;
		}

		showStatusMessage("Downloading the project");

		ScopedPointer<InputStream> stream = downloadLocation.createInputStream(false, &downloadProgress, this, String(), 0, nullptr, &httpStatusCode, 20);

		if (stream == nullptr || stream->getTotalLength() <= 0)
		{
			result = ErrorCodes::URLNotFound;
			targetDirectory.deleteRecursively();
			return;
		}

		File tempFile(File::getSpecialLocation(File::tempDirectory).getChildFile("projectDownload.tmp"));

		tempFile.deleteFile();
		tempFile.create();

		ScopedPointer<OutputStream> fos = tempFile.createOutputStream();

		MemoryBlock mb;
		mb.setSize(8192);

		const int64 numBytesTotal = stream->getNumBytesRemaining();
		int64 numBytesRead = 0;

		while (stream->getNumBytesRemaining() > 0)
		{
			const int64 chunkSize = jmin<int64>(stream->getNumBytesRemaining(), 8192);

			downloadProgress(this, (int)numBytesRead, (int)numBytesTotal);

			if (threadShouldExit())
			{
				result = ErrorCodes::AbortedByUser;
				fos->flush();
				fos = nullptr;

				tempFile.deleteFile();
				targetDirectory.deleteRecursively();
				return;
			}

			stream->read(mb.getData(), (int)chunkSize);

			numBytesRead += chunkSize;

			fos->write(mb.getData(), (size_t)chunkSize);
		}

		fos->flush();

		showStatusMessage("Extracting...");

		setProgress(-1.0);

		FileInputStream fis(tempFile);

		ZipFile input(&fis, false);

		if (input.getNumEntries() == 0)
		{
			result = ErrorCodes::FileNotAnArchive;
			tempFile.deleteFile();
			targetDirectory.deleteRecursively();
			return;
		}

		const int numFiles = input.getNumEntries();

		for (int i = 0; i < numFiles; i++)
		{
			if (threadShouldExit())
			{
				tempFile.deleteFile();
				targetDirectory.deleteRecursively();
				result = ErrorCodes::AbortedByUser;

				break;
			}

			input.uncompressEntry(i, targetDirectory, true);

			setProgress((double)i / (double)numFiles);
		}

		tempFile.deleteFile();

	}


	static bool downloadProgress(void* context, int bytesSent, int totalBytes)
	{
		const double downloadedMB = (double)bytesSent / 1024.0 / 1024.0;
		const double totalMB = (double)totalBytes / 1024.0 / 1024.0;
		const double percent = (totalMB > 0.0) ? (downloadedMB / totalMB) : 0.0;

		static_cast<ProjectDownloader*>(context)->showStatusMessage("Downloaded: " + String(downloadedMB, 1) + " MB / " + String(totalMB, 2) + " MB");

		static_cast<ProjectDownloader*>(context)->setProgress(percent);

		return !static_cast<ProjectDownloader*>(context)->threadShouldExit();
	}

	void threadFinished() override
	{
		switch (result)
		{
		case ProjectDownloader::ErrorCodes::OK:
			if (PresetHandler::showYesNoWindow("Switch projects", "Do you want to switch to the downloaded project?", PresetHandler::IconType::Question))
			{
				GET_PROJECT_HANDLER(bpe->getMainSynthChain()).setWorkingProject(targetDirectory);
			}
			break;
		case ProjectDownloader::ErrorCodes::InvalidURL:
			PresetHandler::showMessageWindow("Wrong URL", "The entered URL is not valid", PresetHandler::IconType::Error);
			break;
		case ProjectDownloader::ErrorCodes::URLNotFound:
			PresetHandler::showMessageWindow("Error downloading", "The URL could not be opened. HTTP status code: " + String(httpStatusCode), PresetHandler::IconType::Error);
			break;
		case ProjectDownloader::ErrorCodes::DirectoryAlreadyExists:
			PresetHandler::showMessageWindow("Project already exists.", "You'll need to delete the existing project before downloading.", PresetHandler::IconType::Error);
			break;
		case ProjectDownloader::ErrorCodes::FileNotAnArchive:
			PresetHandler::showMessageWindow("Archive corrupt", "The file could not be extracted because it is either corrupt or not an archive.", PresetHandler::IconType::Error);
		case ProjectDownloader::ErrorCodes::AbortedByUser:
			PresetHandler::showMessageWindow("Download cancelled", "The project was not downloaded because the progress was aborted.", PresetHandler::IconType::Error);
		case ProjectDownloader::ErrorCodes::numErrorCodes:
			break;
		default:
			break;
		}


	}

private:

	/** A small helper function that replaces links to cloud content with their direct download URL. */
	static String replaceHosterLinksWithDirectDownloadURL(const String url)
	{
		const bool dropBox = url.containsIgnoreCase("dropbox");
		const bool googleDrive = url.containsIgnoreCase("drive.google.com");

		if (dropBox)
		{
			return url.replace("dl=0", "dl=1");;
		}
		else if (googleDrive)
		{
			const String downloadID = url.fromFirstOccurrenceOf("https://drive.google.com/file/d/", false, true).upToFirstOccurrenceOf("/", false, false);
			const String directLink = "https://drive.google.com/uc?export=download&id=" + downloadID;

			return directLink;
		}
		else return url;
	}

	BackendProcessorEditor *bpe;

	ScopedPointer<FilenameComponent> targetFile;

	File targetDirectory;

	ErrorCodes result;
	int httpStatusCode;
};

