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




MARKDOWN_CHAPTER(WavetableHelp)

START_MARKDOWN(FileBrowserDescription)
ML("# Wavetable Conversion File")
ML("You can load / save configuration files that store the current settings of this dialogue.  ")
ML("The settings will be written to the specified file when you press **OK**. In order to load a configuration, just open a previously stored file.")
ML("## Stored information")
ML("- Settings")
ML("- Samplemap. ")
ML("- Analysis progress")
ML("- Harmonic Map of each analysed sample")
ML("- Gain table of each analysed sample")
END_MARKDOWN()

START_MARKDOWN(ReverseWavetables)
ML("# Reverse Wavetable order")
ML("If this is enabled, it will store the wavetables in reversed order.  ")
ML("This is useful if you have decaying samples and want to use the table index to modulate the decay process")
END_MARKDOWN()

START_MARKDOWN(WindowType)
ML("# FFT Window Type")
ML("This changes the window function that is applied to the signal before the FFT.  ")
ML("The selection of the right window function has a drastic impact on the resulting accuracy and varies between different signal types. ")
ML("Try out different window types and compare the results.");
END_MARKDOWN()

START_MARKDOWN(WindowSize)
ML("# FFT Window Size")
ML("Changes the size of the chunk that is used for the FFT.  ")
ML("Generally, lower values yield more accurate results (and sizes above 2048 tend to smear the wavetables because their range start to overlap).  ");
ML("However lower frequencies can't be detected with small FFT sizes so it might not work.    ")
ML("> The recommended approach is to choose the **lowest possible FFT size** that still creates a reasonable harmonic spectrum.");
END_MARKDOWN()

END_MARKDOWN_CHAPTER()


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



class UpdateChecker : public DialogWindowWithBackgroundThread
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
		DialogWindowWithBackgroundThread("Checking for newer version."),
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
				const int64 chunkSize = (int64)jmin<int>((int)stream->getNumBytesRemaining(), 8192);

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
		const String t = xml.getStringAttribute("Script");

		if (!t.startsWith("{EXTERNAL_SCRIPT}"))
		{
			xml.removeAttribute("Script");
		}

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

	DummyUnlocker(MainController *mc_) :
		mc(mc_)
	{

	}

	String getProductID() override
	{
		return dynamic_cast<GlobalSettingManager*>(mc)->getSettingsObject().getSetting(HiseSettings::Project::Name).toString();
	}

	bool doesProductIDMatch(const String & 	returnedIDFromServer)
	{
		return returnedIDFromServer == getProductID();
	}




private:

	MainController* mc;


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

class WavetableConverterDialog : public DialogWindowWithBackgroundThread,
								 public ComboBoxListener,
								 public FilenameComponentListener
	
{
	enum ButtonIds
	{
		Next = 5,
		Previous,
		Rescan,
		Discard,
		Preview,
		Original,
		numButtonIds
	};

public:
	WavetableConverterDialog(ModulatorSynthChain* chain_) :
		DialogWindowWithBackgroundThread("Convert Samplemaps to Wavetable"),
		chain(chain_),
		converter(new SampleMapToWavetableConverter(chain_)),
		r(Result::ok())
	{
		
		sampleMapFile = new FilenameComponent("SampleMap", GET_PROJECT_HANDLER(chain).getWorkDirectory(), true, false, true, "*.wtc", "", "Load Wavetable Conversion File");
		sampleMapFile->addListener(this);
		
		fileHandling = new AdditionalRow(this);
		fileHandling->addCustomComponent(sampleMapFile, "Wavetable File");

		fileHandling->setInfoTextForLastComponent(WavetableHelp::FileBrowserDescription());

		
		fileHandling->setSize(512, 24+16);

		addCustomComponent(fileHandling);

		StringArray windows;
		windows.add("Flat Top");
		windows.add("Blackman Harris");
		windows.add("Hann");
		windows.add("Hamming");
		windows.add("Kaiser");
		windows.add("Gaussian");
		windows.add("Triangle");
		windows.add("Rectangle");
		
		StringArray sizes;
		sizes.add("Lowest possible");
		sizes.add("128");
		sizes.add("256");
		sizes.add("512");
		sizes.add("1024");
		sizes.add("2048");
		sizes.add("4096");
		sizes.add("8192");

		StringArray yesNo;

		yesNo.add("Yes");
		yesNo.add("No");



		selectors = new AdditionalRow(this);



		selectors->addComboBox("ReverseTables", {"Yes", "No"}, "Reverse Wavetable order", 90);
		selectors->setInfoTextForLastComponent(WavetableHelp::ReverseWavetables());
		selectors->addComboBox("WindowType", windows, "FFT Window Type" );
		selectors->setInfoTextForLastComponent(WavetableHelp::WindowType());
		selectors->addComboBox("FFTSize", sizes, "FFT Size", 90);
		selectors->setInfoTextForLastComponent(WavetableHelp::WindowSize());

		selectors->setSize(512, 40);
		addCustomComponent(selectors);

		preview = new CombinedPreview(*converter);
		preview->setSize(512, 300);

		addCustomComponent(preview);

		gainPackData = new SliderPackData(nullptr, nullptr);
		gainPackData->setNumSliders(64);
		gainPackData->setRange(0.0, 1.0, 0.01);
		gainPack = new SliderPack(gainPackData);
		
		
		gainPack->setName("Gain Values");
		gainPack->setSize(512, 40);

		addCustomComponent(gainPack);

		additionalButtons = new AdditionalRow(this);



		additionalButtons->addButton("Previous", KeyPress(KeyPress::leftKey));
		additionalButtons->addButton("Next", KeyPress(KeyPress::rightKey));
		additionalButtons->addButton("Rescan", KeyPress(KeyPress::F5Key));
		additionalButtons->addButton("Discard");
		additionalButtons->addButton("Preview", KeyPress(KeyPress('p')));
		additionalButtons->addButton("Original", KeyPress(KeyPress('o')));

		additionalButtons->setSize(512, 32);

		addCustomComponent(additionalButtons);

		addBasicComponents(true);

        if(auto s = ProcessorHelpers::getFirstProcessorWithType<ModulatorSampler>(chain))
        {
			auto sampleMapTree = s->getSampleMap()->getValueTree();
            
            converter->parseSampleMap(sampleMapTree);
            converter->refreshCurrentWavetable(getProgressCounter());
            refreshPreview();
            
            showStatusMessage("Imported samplemap from " + s->getId());
            
        }
        else
        {
            showStatusMessage("Choose Samplemap to convert");
        }
        
        
		
		
		
	}

	~WavetableConverterDialog()
	{
		sampleMapFile->removeListener(this);
		sampleMapFile = nullptr;
		fileHandling = nullptr;
		preview = nullptr;
		converter = nullptr;
	}

	void filenameComponentChanged(FilenameComponent* /*fileComponentThatHasChanged*/) override
	{
		currentFile = sampleMapFile->getCurrentFile();

		if (currentFile.existsAsFile())
		{
			
		}

		converter->parseSampleMap(sampleMapFile->getCurrentFile());
		converter->refreshCurrentWavetable(getProgressCounter());
		refreshPreview();
	}

	void resultButtonClicked(const String &name)
	{
		if (name == "Previous")
		{
			converter->moveCurrentSampleIndex(false);
			r = converter->refreshCurrentWavetable(getProgressCounter(), false);
		}
		else if (name == "Next")
		{
			converter->moveCurrentSampleIndex(true);
			r = converter->refreshCurrentWavetable(getProgressCounter(), false);
		}
		else if (name == "Rescan")
		{
			r = converter->refreshCurrentWavetable(getProgressCounter(), true);
		}
		else if (name == "Discard")
		{
			r = converter->discardCurrentNoteAndUsePrevious();
		}
		else if (name == "Original" || name == "Preview")
		{
			showStatusMessage("Preview");
			auto b = converter->getPreviewBuffers(name == "Original");

			chain->getMainController()->setBufferToPlay(b);
		}
		

		refreshPreview();

	}

	void refreshPreview()
	{
		converter->sendChangeMessage();

		if (auto gainData = converter->getCurrentGainValues())
		{
			for (int i = 0; i < 64; i++)
			{
				gainPackData->setValue(i, gainData[i]);
			}

			gainPackData->sendChangeMessage();
		}

		if (r.failed())
			showStatusMessage("Fail at " + MidiMessage::getMidiNoteName(converter->getCurrentNoteNumber(), true, true, 3) + ": " + r.getErrorMessage());
		else
			showStatusMessage("OK: " + MidiMessage::getMidiNoteName(converter->getCurrentNoteNumber(), true, true, 3));
	}

	void comboBoxChanged(ComboBox* comboBoxThatHasChanged) override
	{
		if (comboBoxThatHasChanged->getName() == "WindowType")
		{
			converter->windowType = (SampleMapToWavetableConverter::WindowType)comboBoxThatHasChanged->getSelectedItemIndex();
		}
		else if (comboBoxThatHasChanged->getName() == "ReverseTables")
		{
			converter->reverseOrder = comboBoxThatHasChanged->getSelectedItemIndex() == 0;
		}
		else
		{
			if (comboBoxThatHasChanged->getText() == "Lowest possible")
			{
				converter->fftSize = -1;
			}
			else
			{
				auto windowSize = comboBoxThatHasChanged->getText().getIntValue();
				converter->fftSize = windowSize;
			}

			
		}
	}

	void run() override
	{
		converter->renderAllWavetablesFromHarmonicMaps(getProgressCounter());

		if (threadShouldExit())
			return;

		auto leftTree = converter->getValueTree(true);
		auto rightTree = converter->getValueTree(false);

		auto fileL = sampleMapFile->getCurrentFile().getFileName() + "_Left.hwt";
		auto fileR = sampleMapFile->getCurrentFile().getFileName() + "_Right.hwt";

		auto tfl = GET_PROJECT_HANDLER(chain).getSubDirectory(ProjectHandler::SubDirectories::AudioFiles).getChildFile(fileL);
		auto tfr = GET_PROJECT_HANDLER(chain).getSubDirectory(ProjectHandler::SubDirectories::AudioFiles).getChildFile(fileR);

		PresetHandler::writeValueTreeAsFile(leftTree, tfl.getFullPathName());		
		PresetHandler::writeValueTreeAsFile(rightTree, tfr.getFullPathName());
	}

	void threadFinished() override
	{
		if (r.failed())
			PresetHandler::showMessageWindow("Conversion failed", r.getErrorMessage(), PresetHandler::IconType::Error);
		else
			PresetHandler::showMessageWindow("Conversion OK", "The wavetables were created sucessfully", PresetHandler::IconType::Info);
	}

	double wavetableProgress = 0.0;
	int numWavetables = 0;

	Result r;

	ScopedPointer<CombinedPreview> preview;

	
	ScopedPointer<SliderPack> gainPack;
	ScopedPointer<SliderPackData> gainPackData;
	ModulatorSynthChain* chain;
	
	ScopedPointer<AdditionalRow> fileHandling;

	ScopedPointer<AdditionalRow> selectors;

	ScopedPointer<AdditionalRow> additionalButtons;

	Component::SafePointer<FilenameComponent> sampleMapFile;

	ScopedPointer<SampleMapToWavetableConverter> converter;

	File currentFile;

};

class MonolithConverter : public MonolithExporter
{
public:

	enum ExportOptions
	{
		ExportAll,
		ExportSampleMapsAndJSON,
		ExportJSON,
		numExportOptions
	};

	MonolithConverter(BackendRootWindow* bpe_) :
		MonolithExporter("Convert samples to Monolith + Samplemap", bpe_->getMainSynthChain()),
		bpe(bpe_),
		chain(bpe_->getMainSynthChain())
	{

		sampler = dynamic_cast<ModulatorSampler*>(ProcessorHelpers::getFirstProcessorWithName(chain, "Sampler"));
		sampleFolder = GET_PROJECT_HANDLER(chain).getSubDirectory(ProjectHandler::SubDirectories::Samples);

		jassert(sampler != nullptr);

		StringArray directoryDepth;
		directoryDepth.add("1");
		directoryDepth.add("2");
		directoryDepth.add("3");
		directoryDepth.add("4");

		addComboBox("directoryDepth", directoryDepth, "Directory Depth");

		StringArray yesNo;

		yesNo.add("Yes");
		yesNo.add("No"); // best code I've ever written...

		addComboBox("overwriteFiles", yesNo, "Overwrite existing files");
		
		StringArray option;
		option.add("Export all");
		option.add("Skip monolith file creation");
		option.add("Only create JSON data file");

		addComboBox("option", option, "Export Depth");

		addTextEditor("directorySeparator", "::", "Directory separation character");

		StringArray sa;

		sa.add("No compression");
		sa.add("Fast Decompression");
		sa.add("Low file size (recommended)");

		addComboBox("compressionOptions", sa, "HLAC Compression options");

		addBasicComponents(true);
	};

	void getDepth(const File& rootFile, const File& childFile, int& counter)
	{
		jassert(childFile.isAChildOf(rootFile));

		if (childFile.getParentDirectory() == rootFile)
		{
			return;
		}

		counter++;

		getDepth(rootFile, childFile.getParentDirectory(), counter);
	}

	void convertSampleMap(const File& sampleDirectory, bool overwriteExistingData, bool exportSamples, bool exportSampleMap)
	{
		if (!exportSamples && !exportSampleMap) return;

#if JUCE_WINDOWS
		const String slash = "\\";
#else
		const String slash = "/";
#endif

		const String sampleMapPath = sampleDirectory.getRelativePathFrom(sampleFolder);
		const String sampleMapId = sampleMapPath.replace(slash, "_");

		showStatusMessage("Importing " + sampleMapId);

		Array<File> samples;

		sampleDirectory.findChildFiles(samples, File::findFiles, true, "*.wav;*.aif;*.aif;*.WAV;*.AIF;*.AIFF");

		StringArray fileNames;

		for (int i = 0; i < samples.size(); i++)
		{
			if (samples[i].isHidden() || samples[i].getFileName().startsWith("."))
				continue;

			fileNames.add(samples[i].getFullPathName());
		}

        auto& tmpBpe = bpe;
		
        StringArray fileNamesCopy;
        
        fileNamesCopy.addArray(fileNames);
        
        auto f = [tmpBpe, fileNamesCopy](Processor* p)
        {
            if(auto sampler = dynamic_cast<ModulatorSampler*>(p))
            {
                sampler->clearSampleMap(dontSendNotification);
                SampleImporter::loadAudioFilesRaw(tmpBpe, sampler, fileNamesCopy);
                SampleEditHandler::SampleEditingActions::automapUsingMetadata(sampler);
            }
            
            return SafeFunctionCall::Status::OK;
        };
        
        sampler->killAllVoicesAndCall(f);
		
        Thread::sleep(500);
        
		sampler->getSampleMap()->setId(sampleMapId);
		sampler->getSampleMap()->setIsMonolith();

		setSampleMap(sampler->getSampleMap());

        auto sampleMapFolder = GET_PROJECT_HANDLER(chain).getSubDirectory(ProjectHandler::SubDirectories::SampleMaps);
        
		sampleMapFile = sampleMapFolder.getChildFile(sampleMapPath + ".xml");

        //sampleMapFile = sampleMapFolder.getChildFile(sampleMapId + ".xml");
        
		auto& lock = sampler->getMainController()->getSampleManager().getSampleLock();

		while (!lock.tryEnter())
			Thread::sleep(500);

		lock.exit();

		exportCurrentSampleMap(overwriteExistingData, exportSamples, exportSampleMap);
	}

	void run() override
	{		
		generateDirectoryList();

		showStatusMessage("Writing JSON list");

		const String data = JSON::toString(fileNameList);
		File f = GET_PROJECT_HANDLER(bpe->getMainSynthChain()).getSubDirectory(ProjectHandler::SubDirectories::Scripts).getChildFile("samplemaps.js");
		f.replaceWithText(data);

		
        ExportOptions optionIndex = (ExportOptions)getComboBoxComponent("option")->getSelectedItemIndex();
        
        const bool exportSamples = (optionIndex == ExportAll);
        const bool exportSampleMap = (optionIndex == ExportSampleMapsAndJSON) || (optionIndex == ExportAll);
        const bool overwriteData = getComboBoxComponent("overwriteFiles")->getSelectedItemIndex() == 0;
        
        
        
        for (int i = 0; i < fileList.size(); i++)
        {
            if (threadShouldExit())
            {
                break;
            }
            
            setProgress((double)i / (double)fileList.size());
            convertSampleMap(fileList[i], overwriteData, exportSamples, exportSampleMap);
        }
	}

	void generateDirectoryList()
	{
		sampler->getMainController()->getSampleManager().getModulatorSamplerSoundPool()->setUpdatePool(false);

		Array<File> allDirectories;

		const int depth = getComboBoxComponent("directoryDepth")->getText().getIntValue();


		sampleFolder.findChildFiles(allDirectories, File::findDirectories, true);
		const String separator = getTextEditor("directorySeparator")->getText();

		for (int i = 0; i < allDirectories.size(); i++)
		{
			int thisDepth = 1;

			getDepth(sampleFolder, allDirectories[i], thisDepth);

			if (thisDepth == depth)
			{
				fileList.add(allDirectories[i]);
			}
		}

		for (int i = 0; i < fileList.size(); i++)
		{

#if JUCE_WINDOWS
			const String slash = "\\";
#else
			const String slash = "/";
#endif

			String s = fileList[i].getRelativePathFrom(sampleFolder).replace(slash, separator);

			fileNameList.add(var(s));

		}
	}

	void threadFinished() override
	{
        
	};

private:

	

	Array<var> fileNameList;
	Array<File> fileList;

	File sampleFolder;
	BackendRootWindow* bpe;
	ModulatorSampler* sampler;
	ModulatorSynthChain* chain;
};


class PoolExporter : public DialogWindowWithBackgroundThread
{
public:

	PoolExporter(MainController* mc_):
		DialogWindowWithBackgroundThread("Exporting pool resources"),
		mc(mc_)
	{
		addBasicComponents(false);

		runThread();
	}

	void run() override
	{
		showStatusMessage("Exporting pools");

		auto& handler = mc->getCurrentFileHandler();
		handler.exportAllPoolsToTemporaryDirectory(mc->getMainSynthChain(), &logData);
	}

	void threadFinished() override
	{
		PresetHandler::showMessageWindow("Sucessfully exported", "All pools were successfully exported");
	}

private:

	MainController* mc;
};


class CyclicReferenceChecker: public DialogWindowWithBackgroundThread
{
public:

	CyclicReferenceChecker(BackendProcessorEditor* bpe_) :
		DialogWindowWithBackgroundThread("Checking cyclic references"),
		bpe(bpe_)
	{
		StringArray processorList = ProcessorHelpers::getAllIdsForType<JavascriptProcessor>(bpe->getMainSynthChain());

		addComboBox("scriptProcessor", processorList, "ScriptProcessor to analyze");
		addBasicComponents(true);
	}

	void run() override
	{
		
		setProgress(-1.0);
		
		auto id = getComboBoxComponent("scriptProcessor")->getText();
		auto jsp = dynamic_cast<JavascriptProcessor*>(ProcessorHelpers::getFirstProcessorWithName(bpe->getMainSynthChain(), id));

		if (jsp != nullptr)
		{
			
			data.progress = &logData.progress;
			data.thread = this;
			
			showStatusMessage("Recompiling script");

			{
				// don't bother...
				MessageManagerLock mm;

				jsp->compileScriptWithCycleReferenceCheckEnabled();
			}

			showStatusMessage("Checking Cyclic references");

			jsp->getScriptEngine()->checkCyclicReferences(data, Identifier());

		}

	}

	void threadFinished()
	{
		if (data.overflowHit)
		{
			PresetHandler::showMessageWindow("Overflow", "The reference check was cancelled due to a stack overflow", PresetHandler::IconType::Error);
		}
		else if (data.cyclicReferenceString.isNotEmpty())
		{
			if (PresetHandler::showYesNoWindow("Cyclic References found", "The " + data.cyclicReferenceString + " is a cyclic reference. Your script will be leaking memory.\nPres OK to copy this message to the clipboard.",
				PresetHandler::IconType::Error))
			{
				SystemClipboard::copyTextToClipboard(data.cyclicReferenceString);
			}
		}
		else
		{
			PresetHandler::showMessageWindow("No Cyclic References found", 
										     "Your script does not contain cyclic references.\n" + String(data.numChecked) + " references were checked", 
											 PresetHandler::IconType::Info);
		}
	}

private:

	HiseJavascriptEngine::CyclicReferenceCheckBase::ThreadData data;


	BackendProcessorEditor* bpe;
};


class ProjectDownloader : public DialogWindowWithBackgroundThread,
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
		DialogWindowWithBackgroundThread("Download new Project"),
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
			const int64 chunkSize = (int64)jmin<int>((int)stream->getNumBytesRemaining(), 8192);

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
				GET_PROJECT_HANDLER(bpe->getMainSynthChain()).setWorkingProject(targetDirectory, bpe);
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


struct DeviceTypeSanityCheck : public DialogWindowWithBackgroundThread,
							   public ControlledObject
{
	enum TestIndex
	{
		ControlPersistency,
		DefaultValues,
		InitState,
		AllTests
	};

	DeviceTypeSanityCheck(MainController* mc):
		DialogWindowWithBackgroundThread("Checking Device type sanity"),
		ControlledObject(mc)
	{
		StringArray options;

		options.add("iPad only");
		options.add("iPhone only");
		options.add("iPad / iPhone");

		addComboBox("targets", options, "Device Targets");

		StringArray tests;

		tests.add("Check Control persistency");
		tests.add("Check default values");
		tests.add("Check init state matches default values");
		tests.add("All tests");

		addComboBox("tests", tests, "Tests to run");

		addBasicComponents(true);
	}

	void run() override
	{
		ok = true;
		mp = JavascriptMidiProcessor::getFirstInterfaceScriptProcessor(getMainController());
		content = mp->getScriptingContent();
		desktopConnections = createArrayForCurrentDevice();

		auto testIndex = getComboBoxComponent("tests")->getSelectedItemIndex();
		auto index = getComboBoxComponent("targets")->getSelectedItemIndex();
		CompileExporter::BuildOption option;

		if (index == 0) option = CompileExporter::BuildOption::StandaloneiPad;
		if (index == 1) option = CompileExporter::BuildOption::StandaloneiPhone;
		if (index == 2) option = CompileExporter::BuildOption::StandaloneiOS;


		if (testIndex == ControlPersistency || testIndex == AllTests)
		{
			runTest(option, [this](HiseDeviceSimulator::DeviceType t) {this->checkPersistency(t); });
			
		}

		if (testIndex == DefaultValues || testIndex == AllTests)
		{
			runTest(option, [this](HiseDeviceSimulator::DeviceType t) {this->checkDefaultValues(t); });
		}

		if (testIndex == InitState || testIndex == AllTests)
		{
			checkInitState();
		}
	}

	void threadFinished() override
	{
		if (ok)
			PresetHandler::showMessageWindow("Tests passed", "All tests are passed");
		else
			PresetHandler::showMessageWindow("Tests failed", "Some tests failed. Check the console for more info", PresetHandler::IconType::Error);
	}

	struct ProcessorConnection
	{
		bool operator==(const ProcessorConnection& other) const
		{
			return other.id == id;
		}

		Identifier id;
		Processor* p = nullptr;
		int parameterIndex = -1;
		var defaultValue;
	};

	Array<ProcessorConnection> createArrayForCurrentDevice()
	{
		Array<ProcessorConnection> newList;

		debugToConsole(mp, " - " + HiseDeviceSimulator::getDeviceName());

		for (int i = 0; i < content->getNumComponents(); i++)
		{
			auto sc = content->getComponent(i);

			if (!sc->getScriptObjectProperty(ScriptComponent::saveInPreset))
				continue;

			ProcessorConnection pc;
			pc.id = sc->getName();
			pc.p = sc->getConnectedProcessor();
			pc.parameterIndex = sc->getConnectedParameterIndex();
			pc.defaultValue = sc->getScriptObjectProperty(ScriptComponent::Properties::defaultValue);
			newList.add(pc);
		}

		return newList;
	}

	void throwError(const String& message)
	{
		String error = HiseDeviceSimulator::getDeviceName() + ": " + message;

		throw error;
	}

	bool ok = true;

	void setDeviceType(HiseDeviceSimulator::DeviceType type)
	{
		showStatusMessage("Setting device type " + HiseDeviceSimulator::getDeviceName());

		HiseDeviceSimulator::setDeviceType(type);
		auto contentData = content->exportAsValueTree();
		mp->setDeviceTypeForInterface((int)type);

		showStatusMessage("Waiting for compilation");
		waitingForCompilation = true;

		bool* w = &waitingForCompilation;
		auto& tmp = mp;

		auto f = [tmp, w]()
		{
			auto rf = [w](const JavascriptProcessor::SnippetResult&)
			{
				*w = false;
				return;
			};

			tmp->compileScript(rf);
		};

		MessageManager::callAsync(f);
		

		while (waitingForCompilation && !threadShouldExit())
		{
			Thread::sleep(500);
		}

		showStatusMessage("Restoring content values");
		mp->getContent()->restoreFromValueTree(contentData);
	}

	void log(const String& s)
	{
		debugToConsole(mp, s);
	}

	Array<ProcessorConnection> setAndCreateArray(HiseDeviceSimulator::DeviceType type)
	{
		if (threadShouldExit())
			return {};

		setDeviceType(type);

		if (!mp->hasUIDataForDeviceType())
		{
			if (HiseDeviceSimulator::isAUv3())
				showStatusMessage("You need to define a interface for AUv3");

			return {};
		}

		if (threadShouldExit())
			return {};

		return createArrayForCurrentDevice();
	}

	void checkDefaultValues(HiseDeviceSimulator::DeviceType type)
	{
		auto deviceConnections = setAndCreateArray(type);

		showStatusMessage("Checking default values");

		for (const auto& item : deviceConnections)
		{
			auto dItemIndex = desktopConnections.indexOf(item);

			if (dItemIndex == -1)
			{
				log("Missing control found. Run persistency check");
				ok = false;
				return;
			}

			auto expected = desktopConnections[dItemIndex].defaultValue;
			auto actual = item.defaultValue;

			if (expected != actual)
			{
				ok = false;
				log("Default value mismatch for " + item.id);
			}
				
		}
	}

	void checkInitState()
	{
		setDeviceType(HiseDeviceSimulator::DeviceType::Desktop);

		desktopConnections = createArrayForCurrentDevice();

		for (int i = 0; i < content->getNumComponents(); i++)
		{
			auto sc = content->getComponent(i);

			if (!sc->getScriptObjectProperty(ScriptComponent::saveInPreset))
				continue;

			if (sc->getValue() != sc->getScriptObjectProperty(ScriptComponent::defaultValue))
			{
				ok = false;
				log("Init value mismatch for " + sc->getName().toString());
			}
				
		}
	}

	void checkPersistency(HiseDeviceSimulator::DeviceType type)
	{
        log("Testing persistency for " + HiseDeviceSimulator::getDeviceName((int)type));
        
		auto deviceConnections = setAndCreateArray(type);
        
        if(deviceConnections.size() == 0)
        {
            return;
        }
        
		showStatusMessage("Checking UI controls");

		StringArray missingInDesktop;
		StringArray missingInDevice;
		StringArray connectionErrors;

		compareArrays(desktopConnections, deviceConnections, missingInDesktop, connectionErrors);
		compareArrays(deviceConnections, desktopConnections, missingInDevice, connectionErrors);
		
		if (!missingInDesktop.isEmpty())
		{
			log(String("Missing in Desktop: "));

			for (const auto& s : missingInDesktop)
				log(s);
		}

		if (!missingInDevice.isEmpty())
		{
			log("Missing in " + HiseDeviceSimulator::getDeviceName());

			for (const auto& s : missingInDevice)
				log(s);
		}

		if (!connectionErrors.isEmpty())
		{
			log("Connection errors");

			for (const auto& s : connectionErrors)
				log(s);
		}
	}

	void compareArrays(const Array<ProcessorConnection>& testArray, const Array<ProcessorConnection>& compareArray, StringArray& missingNames, StringArray& connectionErrors)
	{
		for (const auto& item : testArray)
		{
			int index = compareArray.indexOf(item);

			if (index == -1)
			{
				missingNames.add(item.id.toString());
				ok = false;
			}
			else
			{
				const bool processorMatches = item.p == compareArray[index].p;
				const bool parameterMatches = item.parameterIndex == compareArray[index].parameterIndex;

				if (!processorMatches || !parameterMatches)
				{
					ok = false;
					connectionErrors.add("Connection mismatch for " + item.id);
				}
			}
		}
	}

	Array<ProcessorConnection> desktopConnections;

	ScriptingApi::Content* content;
	JavascriptMidiProcessor* mp;

public:

	bool waitingForCompilation = false;

	using TestFunction = std::function<void(HiseDeviceSimulator::DeviceType)>;

	void runTest(CompileExporter::BuildOption option, const TestFunction& tf)
	{
		if (HiseDeviceSimulator::getDeviceType() != HiseDeviceSimulator::DeviceType::Desktop)
			log("Device Type must be Desktop for exporting");

		try
		{
			if (CompileExporter::BuildOptionHelpers::isIPad(option))
			{
                tf(HiseDeviceSimulator::DeviceType::iPad);
				tf(HiseDeviceSimulator::DeviceType::iPadAUv3);
			}

			if (CompileExporter::BuildOptionHelpers::isIPhone(option))
			{
				tf(HiseDeviceSimulator::DeviceType::iPhone);
				tf(HiseDeviceSimulator::DeviceType::iPhoneAUv3);
			}
		}
		catch (String& s)
		{
			setDeviceType(HiseDeviceSimulator::DeviceType::Desktop);
			log(s);
		}

		setDeviceType(HiseDeviceSimulator::DeviceType::Desktop);
	}

	
};

} // namespace hise
