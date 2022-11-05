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


START_MARKDOWN(ReverseWavetables)
ML("## Reverse Wavetable order")
ML("Only used in FFT Resynthesis mode. If this is enabled, it will store the wavetables in reversed order.  ")
ML("This is useful if you have decaying samples and want to use the table index to modulate the decay process")
END_MARKDOWN()

START_MARKDOWN(WindowType)
ML("## FFT Window Type")
ML("Only used in FFT Resynthesis mode. This changes the window function that is applied to the signal before the FFT.  ")
ML("The selection of the right window function has a drastic impact on the resulting accuracy and varies between different signal types. ")
ML("Try out different window types and compare the results.");
END_MARKDOWN()

START_MARKDOWN(WindowSize)
ML("## FFT Window Size")
ML("Only used in FFT Resynthesis mode. Changes the size of the chunk that is used for the FFT.  ")
ML("Generally, lower values yield more accurate results (and sizes above 2048 tend to smear the wavetables because their range start to overlap).  ");
ML("However lower frequencies can't be detected with small FFT sizes so it might not work.    ")
ML("> The recommended approach is to choose the **lowest possible FFT size** that still creates a reasonable harmonic spectrum.");
END_MARKDOWN()

END_MARKDOWN_CHAPTER()


static bool canConnectToWebsite(const URL& url)
{
	auto in = url.createInputStream(false, nullptr, nullptr, String(), 2000, nullptr);
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

		auto stream = url.createInputStream(false, &downloadProgress, this);

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

		auto stream = url.createInputStream(false);

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




void XmlBackupFunctions::restoreAllScripts(ValueTree& v, ModulatorSynthChain *masterChain, const String &newId)
{
	static const Identifier pr("Processor");
	static const Identifier scr("Script");
	static const Identifier id("ID");
	static const Identifier typ("Type");

	if (v.getType() == Identifier(pr) && v[typ].toString().contains("Script"))
	{
		auto fileName = getSanitiziedName(v[id]);
		const String t = v[scr];
		
		if (t.startsWith("{EXTERNAL_SCRIPT}"))
		{
			return;
		}

		File scriptDirectory = getScriptDirectoryFor(masterChain, newId);

		auto sf = scriptDirectory.getChildFile(fileName).withFileExtension("js");

		if (!sf.existsAsFile())
		{
			auto isExternal = v.getProperty(scr).toString().startsWith("{EXTERNAL_SCRIPT}");

			if (!isExternal)
			{
				PresetHandler::showMessageWindow("Script not found", "Error loading script " + fileName, PresetHandler::IconType::Error);
			}
		}

        for(auto f: RangedDirectoryIterator(scriptDirectory, false, "*.js", File::findFiles))
		{
			File script = f.getFile();

			if (script.getFileNameWithoutExtension() == fileName)
			{
				v.setProperty(scr, script.loadFileAsString(), nullptr);
				break;
			}
		}
	}

	for (auto c: v)
		restoreAllScripts(c, masterChain, newId);
}

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

		for(auto f: RangedDirectoryIterator(projectDirectory, true, "*", File::findFilesAndDirectories))
		{
			File currentFile = f.getFile();

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
								 public ComboBoxListener
	
{
	enum Mode
	{
		Resample,
		FFTResynthesis,
		numModes
	};

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
		
		auto list = chain->getMainController()->getActiveFileHandler()->pool->getSampleMapPool().getIdList();

		addComboBox("samplemap", list, "Samplemap");
		getComboBoxComponent("samplemap")->addListener(this);
		
		

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


		selectors->addComboBox("mode", { "Resample Wavetables", "FFT Resynthesis"}, "Mode", 130);
		selectors->addComboBox("ReverseTables", {"Yes", "No"}, "Reverse Wavetable order", 90);
		selectors->setInfoTextForLastComponent(WavetableHelp::ReverseWavetables());
		selectors->addComboBox("WindowType", windows, "FFT Window Type", 120);
		
		selectors->setInfoTextForLastComponent(WavetableHelp::WindowType());
		selectors->addComboBox("FFTSize", sizes, "FFT Size");
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
		fileHandling = nullptr;
		preview = nullptr;
		converter = nullptr;
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
		if (getComboBoxComponent("samplemap") == comboBoxThatHasChanged)
		{
			auto& spool = chain->getMainController()->getActiveFileHandler()->pool->getSampleMapPool();

			PoolReference ref(chain->getMainController(), comboBoxThatHasChanged->getText(), FileHandlerBase::SampleMaps);

			currentlyLoadedMap = ref.getFile().getFileNameWithoutExtension();

			if (auto vData = spool.loadFromReference(ref, PoolHelpers::LoadAndCacheWeak))
			{
				converter->parseSampleMap(*vData.getData());

				if (currentMode == FFTResynthesis)
				{
					converter->refreshCurrentWavetable(getProgressCounter());
					
				}

				refreshPreview(); 
			}
		}
		if (comboBoxThatHasChanged->getName() == "mode")
		{
			currentMode = (Mode)comboBoxThatHasChanged->getSelectedItemIndex();

			if (currentMode == FFTResynthesis)
			{
				converter->refreshCurrentWavetable(getProgressCounter());
				refreshPreview();
			}

		}
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
		if (currentMode == FFTResynthesis)
			converter->renderAllWavetablesFromHarmonicMaps(getProgressCounter());
		else
			converter->renderAllWavetablesFromSingleWavetables(getProgressCounter());

		if (threadShouldExit())
			return;

		auto leftTree = converter->getValueTree(true);
		auto rightTree = converter->getValueTree(false);

		auto fileL = currentlyLoadedMap + "_Left.hwt";
		auto fileR = currentlyLoadedMap + "_Right.hwt";

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
	
	Mode currentMode = Resample;
	ScopedPointer<AdditionalRow> fileHandling;
	ScopedPointer<AdditionalRow> selectors;
	ScopedPointer<AdditionalRow> additionalButtons;
	ScopedPointer<SampleMapToWavetableConverter> converter;

	String currentlyLoadedMap;

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

		StringArray sa2;

		sa2.add("No normalisation");
		sa2.add("Normalise every sample");
		sa2.add("Full Dynamics");

		addComboBox("normalise", sa2, "Normalization");

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

		auto& data = dynamic_cast<GlobalSettingManager*>(mc)->getSettingsObject();
		auto project = data.getSetting(HiseSettings::Project::Name).toString();
		auto company = data.getSetting(HiseSettings::User::Company).toString();
		auto targetDirectory = ProjectHandler::getAppDataDirectory().getParentDirectory().getChildFile(company).getChildFile(project);
		auto& handler = mc->getCurrentFileHandler();

		if (!(bool)data.getSetting(HiseSettings::Project::EmbedImageFiles))
		{
			if (PresetHandler::showYesNoWindow("Copy image pool file to App data directory",
				"Do you want to copy the ImageResources.dat file to the app data directory?\nThis is required for the compiled plugin to load the new resources on this machine"))
			{
				auto f = handler.getTempFileForPool(FileHandlerBase::Images);
				jassert(f.existsAsFile());
				f.copyFileTo(targetDirectory.getChildFile(f.getFileName()));
			}
		}

		if (!(bool)data.getSetting(HiseSettings::Project::EmbedAudioFiles))
		{
			if (PresetHandler::showYesNoWindow("Copy audio pool file to App data directory",
				"Do you want to copy the AudioResources.dat file to the app data directory?\nThis is required for the compiled plugin to load the new resources on this machine"))
			{
				auto f = handler.getTempFileForPool(FileHandlerBase::AudioFiles);
				jassert(f.existsAsFile());
				f.copyFileTo(targetDirectory.getChildFile(f.getFileName()));
			}
		}
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



struct ShortcutEditor : public QuasiModalComponent,
						public Component,
					    public PathFactory
{
	ShortcutEditor(BackendRootWindow* t) :
		QuasiModalComponent(),
		editor(t->getKeyPressMappingSet(), true),
		closeButton("close", nullptr, *this)
	{
		addAndMakeVisible(editor);
		setName("Edit Shortcuts");
		setSize(600, 700);

		editor.setLookAndFeel(&alaf);
		editor.setColours(Colours::transparentBlack, alaf.bright);
		setLookAndFeel(&alaf);
		addAndMakeVisible(closeButton);

		closeButton.onClick = [&]()
		{
			destroy();
		};
	};

	Path createPath(const String&) const override
	{
		Path p;
		p.loadPathFromData(HiBinaryData::ProcessorEditorHeaderIcons::closeIcon, sizeof(HiBinaryData::ProcessorEditorHeaderIcons::closeIcon));
		return p;
	}

	void resized() override
	{
		auto b = getLocalBounds();
		b.removeFromTop(32);

		closeButton.setBounds(getLocalBounds().removeFromTop(37).removeFromRight(37).reduced(6));
		editor.setBounds(b.reduced(10));
	}

	void mouseDown(const MouseEvent& e)
	{
		dragger.startDraggingComponent(this, e);
	}

	void mouseDrag(const MouseEvent& e)
	{
		dragger.dragComponent(this, e, nullptr);
	}

	juce::ComponentDragger dragger;

	void paint(Graphics& g) override
	{
		ColourGradient grad(alaf.dark.withMultipliedBrightness(1.4f), 0.0f, 0.0f,
			alaf.dark, 0.0f, (float)getHeight(), false);

		auto a = getLocalBounds().removeFromTop(37).toFloat();

		g.setFont(GLOBAL_BOLD_FONT().withHeight(17.0f));
		g.setGradientFill(grad);
		g.fillAll();
		g.setColour(Colours::white.withAlpha(0.1f));
		g.fillRect(a);
		g.setColour(alaf.bright);

		g.drawRect(getLocalBounds().toFloat());

		g.drawText("Edit Shortcuts", a, Justification::centred);
		
		
	}

	HiseShapeButton closeButton;
	AlertWindowLookAndFeel alaf;
	juce::KeyMappingEditorComponent editor;
};

class SampleMapPropertySaverWithBackup : public DialogWindowWithBackgroundThread,
										 public ControlledObject
{
public:

	enum class Preset
	{
		None,
		All,
		Positions,
		Volume,
		Tables,
		numPresets
	};

	static Array<Identifier> getPropertyIds(Preset p)
	{
		switch (p)
		{
		case Preset::None:			return {};
		case Preset::All:			return { SampleIds::GainTable, SampleIds::PitchTable, SampleIds::LowPassTable,
											 SampleIds::SampleStart, SampleIds::SampleEnd, SampleIds::LoopXFade,
											 SampleIds::Volume, SampleIds::Pitch, SampleIds::Normalized };
		case Preset::Positions:		return { SampleIds::SampleStart, SampleIds::SampleEnd, SampleIds::LoopXFade };
		case Preset::Volume:		return { SampleIds::Volume, SampleIds::Pitch, SampleIds::Normalized };
		case Preset::Tables:		return { SampleIds::GainTable, SampleIds::PitchTable, SampleIds::LowPassTable };
		default:					return {};
		}
	}

	struct PropertySelector: public Component,
							 public ComboBoxListener
	{
		struct Item : public Component
		{
			Item(Identifier& id_):
				id(id_.toString())
			{
				setRepaintsOnMouseActivity(true);
			}

			void mouseDown(const MouseEvent& e) override
			{
				active = !active;
				repaint();
			}

			void paint(Graphics& g) override
			{
				auto b = getLocalBounds().toFloat().reduced(1.0f);

				g.setColour(Colours::white.withAlpha(isMouseOver(true) ? 0.3f : 0.2f));
				g.fillRect(b);
				g.drawRect(b, 1.0f);
				g.setColour(Colours::white.withAlpha(active ? 0.9f : 0.2f));
				g.setFont(GLOBAL_MONOSPACE_FONT());
				g.drawText(id, b, Justification::centred);
			}

			String id;
			bool active = false;
		};

		PropertySelector()
		{
			for (auto id : getPropertyIds(Preset::All))
			{
				auto item = new Item(id);
				addAndMakeVisible(item);
				items.add(item);
			}

			addAndMakeVisible(presets);
			presets.addItemList({ "None", "All", "Positions", "Volume", "Tables" }, 1);
			presets.addListener(this);
			presets.setTextWhenNothingSelected("Presets");
			

			setSize(350, 100);
		}

		void comboBoxChanged(ComboBox*) override
		{
			Preset p = (Preset)presets.getSelectedItemIndex();

			auto selectedIds = getPropertyIds(p);

			for (auto i : items)
			{
				i->active = selectedIds.contains(Identifier(i->id));
				i->repaint();
			}
		}

		void paint(Graphics& g) override
		{
			auto b = getLocalBounds().removeFromTop(24).toFloat();
			g.setFont(GLOBAL_BOLD_FONT());
			g.setColour(Colours::white);
			g.drawText("Properties to apply", b, Justification::centredLeft);
		}

		void resized() override
		{
			auto b = getLocalBounds();
			auto top = b.removeFromTop(24);

			

			static constexpr int NumRows = 3;
			static constexpr int NumCols = 3;

			auto rowHeight = b.getHeight() / 3;
			auto colWidth = b.getWidth() / 3;

			int cellIndex = 0;

			presets.setBounds(top.removeFromRight(colWidth));

			for (int row = 0; row < NumRows; row++)
			{
				auto r = b.removeFromTop(rowHeight);

				for (int col = 0; col < NumCols; col++)
				{
					auto cell = r.removeFromLeft(colWidth);
					items[cellIndex++]->setBounds(cell);
				}
			}
		}

		OwnedArray<Item> items;
		ComboBox presets;
	};

	SampleMapPropertySaverWithBackup(BackendRootWindow* bpe) :
		DialogWindowWithBackgroundThread("Apply Samplemap Properties"),
		ControlledObject(bpe->getMainController()),
		result(Result::ok())
	{
		auto samplemapList = getMainController()->getCurrentFileHandler().pool->getSampleMapPool().getIdList();
		addComboBox("samplemapId", samplemapList, "SampleMap");
		addTextEditor("backup_postfix", "_backup", "Backup folder suffix");

		sampleMapId = getComboBoxComponent("samplemapId");
		sampleMapId->onChange = BIND_MEMBER_FUNCTION_0(SampleMapPropertySaverWithBackup::refresh);
		suffix = getTextEditor("backup_postfix");
		suffix->onTextChange = BIND_MEMBER_FUNCTION_0(SampleMapPropertySaverWithBackup::refresh);

		addCustomComponent(propertySelector = new PropertySelector());

		addBasicComponents(true);

		refresh();
	}

	void refresh()
	{
		if (sampleMapId->getSelectedId() == 0)
		{
			showStatusMessage("Select a samplemap you want to apply");
			return;
		}

		auto bf = getBackupFolder();
		auto exists = bf.isDirectory();

		if (exists)
			showStatusMessage("Press OK to restore the backup from /" + bf.getFileName());
		else
			showStatusMessage("Press OK to move the original files to /" + bf.getFileName() + " and apply the properties");
	}

	File getBackupFolder()
	{
		auto sampleFolder = getMainController()->getCurrentFileHandler().getRootFolder().getChildFile("SampleBackups");
		sampleFolder.createDirectory();
		auto t = getComboBoxComponent("samplemapId")->getText().fromLastOccurrenceOf("}", false, false);
		t << getTextEditor("backup_postfix")->getText();
		return sampleFolder.getChildFile(t);
	}

	File getSampleFolder() const
	{
		return getMainController()->getCurrentFileHandler().getSubDirectory(FileHandlerBase::Samples);
	}

	void threadFinished() override
	{
		getMainController()->getCurrentFileHandler().pool->getSampleMapPool().refreshPoolAfterUpdate();

		if (!result.wasOk())
		{
			PresetHandler::showMessageWindow("Error at applying properties", result.getErrorMessage(), PresetHandler::IconType::Error);
		}
		else
		{
			if (doBackup)
			{
				if (PresetHandler::showYesNoWindow("OK", "The backup was successfully restored. Do you want to delete the backup folder?"))
				{
					getBackupFolder().deleteRecursively();
				}
			}
			else
			{
				String m;

				m << "The samplemap was applied and saved as backup";

				if (wasMonolith)
					m << "  \n> The monolith files were also removed, so you can reencode the samplemap";

				PresetHandler::showMessageWindow("OK", m);
			}
		}
	}

	struct SampleWithPropertyData: public ReferenceCountedObject
	{
		using List = ReferenceCountedArray<SampleWithPropertyData>;

		SampleWithPropertyData() = default;

		void addFileFromValueTree(MainController* mc, const ValueTree& v)
		{
			if (v.hasProperty(SampleIds::FileName))
			{
				PoolReference sRef(mc, v[SampleIds::FileName].toString(), FileHandlerBase::SubDirectories::Samples);

				if (sRef.isAbsoluteFile())
				{
					String s;
					s << "Absolute file reference detected  \n";
					s << "> " << sRef.getFile().getFullPathName() << "\n";
					throw Result::fail(s);
				}

				sampleFiles.add(sRef.getFile());
			}

			for (auto c : v)
				addFileFromValueTree(mc, c);
		}

		bool operator==(const SampleWithPropertyData& other) const
		{
			for (auto& tf : sampleFiles)
			{
				for (auto& otf : other.sampleFiles)
				{
					if (tf == otf)
						return true;
				}
			}

			return false;
		}

		void addDelta(int delta, const Array<Identifier>& otherIds)
		{
			for (auto i : otherIds)
			{
				if (propertyData.hasProperty(i))
				{
					auto newValue = (int)propertyData[i] + delta;
					propertyData.setProperty(i, newValue, nullptr);
				}
			}
		}

		void addFactor(double factor, const Array<Identifier>& otherIds)
		{
			for (auto i : otherIds)
			{
				if (propertyData.hasProperty(i))
				{
					auto newValue = (double)propertyData[i] * factor;
					propertyData.setProperty(i, (int)(newValue), nullptr);
				}
			}
		}

		void apply(const Identifier& id)
		{
			for (auto f : sampleFiles)
			{
				apply(id, f);
			}
		}

		void apply(const Identifier& id, File& fileToUse)
		{
			if (!propertyData.hasProperty(id) || propertyData[id].toString().getIntValue() == 0)
				return;

			auto value = propertyData[id];

			double unused = 0;
			auto ob = hlac::CompressionHelpers::loadFile(fileToUse, unused);

			int numChannels = ob.getNumChannels();
			int numSamples = ob.getNumSamples();

			fileToUse.deleteFile();

			AudioSampleBuffer lut;
			bool isTable = getPropertyIds(Preset::Tables).contains(id);

			if (isTable)
			{
				SampleLookupTable t;
				t.fromBase64String(propertyData[id].toString());
				lut = AudioSampleBuffer(1, numSamples);
				t.fillExternalLookupTable(lut.getWritePointer(0), numSamples);
			}

			if (id == SampleIds::Volume)
			{
				float gainFactor = Decibels::decibelsToGain((float)value);
				ob.applyGain(gainFactor);
			}
			else if (id == SampleIds::Normalized)
			{
				auto gainFactor = (float)propertyData[SampleIds::NormalizedPeak];
				ob.applyGain(gainFactor);
				propertyData.removeProperty(SampleIds::NormalizedPeak, nullptr);
			}
			else if (id == SampleIds::SampleStart)
			{
				int offset = (int)value;
				AudioSampleBuffer nb(numChannels, numSamples - offset);

				for (int i = 0; i < numChannels; i++)
					FloatVectorOperations::copy(nb.getWritePointer(i), ob.getReadPointer(i, offset), nb.getNumSamples());

				addDelta(-offset, { SampleIds::SampleEnd, SampleIds::LoopStart, SampleIds::LoopEnd });

				std::swap(nb, ob);
			}
			else if (id == SampleIds::LoopXFade)
			{
				int xfadeSize = (int)value;
				AudioSampleBuffer loopBuffer(numChannels, xfadeSize);

				loopBuffer.clear();
			
				float fadeOutStart = (int)propertyData[SampleIds::LoopEnd] - xfadeSize;
				auto  fadeInStart = (int)propertyData[SampleIds::LoopStart] - xfadeSize;

				ob.applyGainRamp(fadeOutStart, xfadeSize, 1.0f, 0.0f);

				for (int i = 0; i < numChannels; i++)
					ob.addFromWithRamp(i, fadeOutStart, ob.getReadPointer(i, fadeInStart), xfadeSize, 0.0f, 1.0f);

			}
			else if (id == SampleIds::SampleEnd)
			{
				int length = (int)value;
				AudioSampleBuffer nb(numChannels, length);

				for (int i = 0; i < numChannels; i++)
					FloatVectorOperations::copy(nb.getWritePointer(i), ob.getReadPointer(i, 0), length);

				std::swap(nb, ob);
			}
			else if (id == SampleIds::Pitch || id == SampleIds::PitchTable)
			{
				int newNumSamples = 0;

				auto getPitchFactor = [&](int index)
				{
					if (isTable)
						return (double)ModulatorSamplerSound::EnvelopeTable::getPitchValue(lut.getSample(0, index));
					else
						return scriptnode::conversion_logic::st2pitch().getValue((double)propertyData[id] / 100.0);
				};

				if (isTable)
				{
					double samplesToCalculate = 0.0;

					for (int i = 0; i < numSamples; i++)
						samplesToCalculate += 1.0 / getPitchFactor(i);

					newNumSamples = (int)samplesToCalculate;
				}
				else
					newNumSamples = (int)((double)numSamples / getPitchFactor(0));

				AudioSampleBuffer nb(numChannels, newNumSamples);
				
				ValueTree tableIndexes;

				Array<Identifier> sampleRangeIds = { SampleIds::SampleStart, SampleIds::SampleEnd,
						SampleIds::LoopStart, SampleIds::LoopEnd,
						SampleIds::LoopXFade, SampleIds::SampleStartMod };

				if (isTable)
				{
					tableIndexes = ValueTree("Ranges");

					for (auto id : sampleRangeIds)
					{
						if (propertyData.hasProperty(id))
							tableIndexes.setProperty(id, propertyData[id], nullptr);
					}
				}
				
				double uptime = 0.0;

				for (int i = 0; i < newNumSamples; i++)
				{
					auto i0 = jmin(numSamples-1, (int)uptime);
					auto i1 = jmin(numSamples-1, i0 + 1);
					auto alpha = uptime - (double)i0;
					
					for (int c = 0; c < numChannels; c++)
					{
						auto v0 = ob.getSample(c, i0);
						auto v1 = ob.getSample(c, i1);
						auto v = Interpolator::interpolateLinear(v0, v1, (float)alpha);
						nb.setSample(c, i, v);
					}

					auto uptimeDelta = getPitchFactor(0);

					if (isTable)
					{
						for (int i = 0; i < tableIndexes.getNumProperties(); i++)
						{
							auto id = tableIndexes.getPropertyName(i);

							if ((int)tableIndexes[id] == i)
								tableIndexes.setProperty(id, (int)uptime, nullptr);
						}

						auto pf0 = getPitchFactor(i0);
						auto pf1 = getPitchFactor(i1);
						uptimeDelta = Interpolator::interpolateLinear(pf0, pf1, alpha);
					}

					uptime += uptimeDelta;
				}

				if (isTable)
				{
					for (int i = 0; i < tableIndexes.getNumProperties(); i++)
					{
						auto id = tableIndexes.getPropertyName(i);
						propertyData.setProperty(id, tableIndexes[id], nullptr);
					}
				}
				else
				{
					// Calculate the static offset
					addFactor(1.0 / getPitchFactor(0), sampleRangeIds);
				}

				std::swap(ob, nb);
			}
			else if(id == SampleIds::GainTable)
			{
				for (int c = 0; c < numChannels; c++)
				{
					for (int i = 0; i < numSamples; i++)
					{
						auto gainFactor = ModulatorSamplerSound::EnvelopeTable::getGainValue(lut.getSample(0, i));
						auto value = ob.getSample(c, i);
						ob.setSample(c, i, value * gainFactor);
					}
				}
			}
			else if (id == SampleIds::LowPassTable)
			{
				CascadedEnvelopeLowPass lp(true);

				PrepareSpecs ps;
				ps.blockSize = 16;
				ps.sampleRate = 44100.0;
				ps.numChannels = numChannels;

				lp.prepare(ps);

				snex::PolyHandler::ScopedVoiceSetter svs(lp.polyManager, 0);

				for (int i = 0; i < numSamples; i+= ps.blockSize)
				{
					int numToDo = jmin(ps.blockSize, numSamples - i);
					auto v = lut.getSample(0, i);
					auto freq = ModulatorSamplerSound::EnvelopeTable::getFreqValue(v);
					lp.process(freq, ob, i, numToDo);
				}
			}

			propertyData.removeProperty(id, nullptr);
			hlac::CompressionHelpers::dump(ob, fileToUse.getFullPathName());
		}

		ValueTree propertyData;
		Array<File> sampleFiles;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SampleWithPropertyData);
	};

	File getSampleMapFile(bool fromBackup)
	{
		if (fromBackup)
			return getBackupFolder().getChildFile(sampleMapId->getText()).withFileExtension("xml");
		else
		{
			PoolReference ref(getMainController(), sampleMapId->getText(), FileHandlerBase::SampleMaps);
			return ref.getFile();
		}
	}

	Array<Identifier> getPropertiesToProcess()
	{
		Array<Identifier> ids;
		for (auto i : propertySelector->items)
		{
			if (!i->active)
				continue;

			ids.add(Identifier(i->id));
		}

		return ids;
	}

	SampleWithPropertyData::List saveToBackup()
	{
		SampleWithPropertyData::List fileList;

		showStatusMessage("Saving backup");

		auto smf = getSampleMapFile(false);

		auto v = ValueTree::fromXml(smf.loadFileAsString());

		if (!v.isValid())
			throw Result::fail("Can't load samplemap");

		wasMonolith = (int)v[Identifier("SaveMode")] != 0;

		if (wasMonolith)
		{
			getMainController()->getSampleManager().getModulatorSamplerSoundPool2()->clearUnreferencedMonoliths();

			auto monolist = getSampleFolder().findChildFiles(File::findFiles, false);

			for (auto m : monolist)
			{
				if (m.getFileExtension().startsWith(".ch") && m.getFileNameWithoutExtension() == sampleMapId->getText())
				{
					auto ok = m.deleteFile();

					if (!ok)
						throw Result::fail("Can't delete monolith");
				}
			}
		}

		auto bf = getBackupFolder();
		auto r = bf.createDirectory();

		auto ok = smf.copyFileTo(getSampleMapFile(true));
		
		if (!ok)
			throw Result::fail("Can't copy samplemap file");

		if (!r.wasOk())
			throw r;

		for (auto c : v)
		{
			ReferenceCountedObjectPtr<SampleWithPropertyData> newData = new SampleWithPropertyData();

			newData->propertyData = c;
			newData->addFileFromValueTree(getMainController(), c);
			
			fileList.add(newData);
		}

		showStatusMessage("Copying sample files to backup location");

		double numTodo = (double)fileList.size();
		double numDone = 0.0;

		for (auto sf : fileList)
		{
			setProgress(numDone / numTodo);

			for (auto sourceFile : sf->sampleFiles)
			{
				auto targetFilePath = sourceFile.getRelativePathFrom(getSampleFolder());
				auto targetFile = bf.getChildFile(targetFilePath);

				if (!targetFile.getParentDirectory().isDirectory())
				{
					r = targetFile.getParentDirectory().createDirectory();

					if (!r.wasOk())
						throw r;
				}

				auto ok = sourceFile.copyFileTo(targetFile);

				if (!ok)
					throw Result::fail("Can't copy sample file");
			}
		}

		return fileList;
	}

	void restoreFromBackup()
	{
		auto smf = getSampleMapFile(true);

		if (!smf.existsAsFile())
		{
			throw Result::fail("Can't find samplemap backup file");
		}

		auto ok = smf.copyFileTo(getSampleMapFile(false));

		if (!ok)
			throw Result::fail("Can't copy samplemap backup");
		
		auto fileList = getBackupFolder().findChildFiles(File::findFiles, true, "*");

		for (auto f : fileList)
		{
			if (f == smf)
				continue;

			if (f.isHidden())
				continue;

			auto targetPath = f.getRelativePathFrom(getBackupFolder());
			auto targetFile = getSampleFolder().getChildFile(targetPath);

			if (targetFile.existsAsFile())
			{
				auto ok = targetFile.deleteFile();

				if (!ok)
					throw Result::fail("Can't delete file  \n> " + targetFile.getFullPathName());
			}

			auto ok = f.copyFileTo(targetFile);

			if (!ok)
				throw Result::fail("Can't copy file  \n>" + f.getFullPathName());
		}
	}

	static void removeProperties(ValueTree v, const Array<Identifier>& properties)
	{
		for (auto p : properties)
			v.removeProperty(p, nullptr);

		for (auto c : v)
			removeProperties(c, properties);
	}

	void applyChanges(SampleWithPropertyData::List& sampleList)
	{
		auto propertiesToProcess = getPropertiesToProcess();

		double numTodo = sampleList.size();
		double numDone = 0.0;

		for (auto s : sampleList)
		{
			setProgress(numDone / numTodo);
			numDone += 1.0;

			for (const auto& p : propertiesToProcess)
			{
				s->apply(p);
			}
		}

		auto v = ValueTree::fromXml(getSampleMapFile(true).loadFileAsString());

		
		v.removeAllChildren(nullptr);

		for (auto s : sampleList)
		{
			v.addChild(s->propertyData.createCopy(), -1, nullptr);
		}

		if (wasMonolith)
		{
			// set it to use the files
			v.setProperty("SaveMode", 0, nullptr);

			// remove the monolith info
			removeProperties(v, { Identifier("MonolithLength"), Identifier("MonolithOffset") });
		}

		auto xml = v.createXml();
		
		getSampleMapFile(false).replaceWithText(xml->createDocument(""));
	}

	void run() override
	{
		auto bf = getBackupFolder();

		try
		{
			doBackup = bf.isDirectory();

			if (doBackup)
			{
				restoreFromBackup();
			}
			else
			{
				auto propertiesToProcess = getPropertiesToProcess();

				if (propertiesToProcess.isEmpty())
				{
					throw Result::fail("No properties selected");
				}

				auto sampleList = saveToBackup();
				applyChanges(sampleList);
			}
		}
		catch (Result& r)
		{
			result = r;
		}
	}

	Result result;

	bool doBackup = false;
	bool wasMonolith = false;

	ComboBox* sampleMapId;
	TextEditor* suffix;
	ScopedPointer<PropertySelector> propertySelector;
};

class SVGToPathDataConverter : public Component,
							   public Value::Listener,
							   public QuasiModalComponent,
							   public PathFactory,
							   public juce::FileDragAndDropTarget
{
public:

	enum class OutputFormat
	{
		Base64,
		CppString,
		HiseScriptNumbers,
        Base64SVG,
		numOutputFormats
	};

	SVGToPathDataConverter(BackendRootWindow* bpe):
		QuasiModalComponent(),
		loadClipboard("Load from clipboard"),
		copyClipboard("Copy to clipboard"),
		resizer(this, nullptr),
		closeButton("close", nullptr, *this)
	{
		outputFormatSelector.addItemList({ "Base64 Path", "C++ Path String", "HiseScript Path number array", "Base64 SVG" }, 1);
		
		addAndMakeVisible(outputFormatSelector);
		addAndMakeVisible(inputEditor);
		addAndMakeVisible(outputEditor);
		addAndMakeVisible(variableName);
		addAndMakeVisible(loadClipboard);
		addAndMakeVisible(copyClipboard);
		addAndMakeVisible(resizer);
		addAndMakeVisible(closeButton);

		GlobalHiseLookAndFeel::setTextEditorColours(inputEditor);
		GlobalHiseLookAndFeel::setTextEditorColours(outputEditor);
		inputEditor.setFont(GLOBAL_MONOSPACE_FONT());
		outputEditor.setFont(GLOBAL_MONOSPACE_FONT());
		GlobalHiseLookAndFeel::setTextEditorColours(variableName);
		
		inputEditor.setFont(GLOBAL_MONOSPACE_FONT());
		variableName.setFont(GLOBAL_MONOSPACE_FONT());

		inputEditor.setMultiLine(true);
		outputEditor.setMultiLine(true);

		inputEditor.getTextValue().referTo(inputDoc);
		outputEditor.getTextValue().referTo(outputDoc);
		variableName.getTextValue().referTo(variableDoc);
		variableDoc.addListener(this);

		variableDoc.setValue("pathData");

		outputFormatSelector.setSelectedItemIndex(0);

		copyClipboard.setLookAndFeel(&alaf);
		loadClipboard.setLookAndFeel(&alaf);
		outputFormatSelector.setLookAndFeel(&alaf);

		outputFormatSelector.onChange = [&]()
		{
			currentOutputFormat = (OutputFormat)outputFormatSelector.getSelectedItemIndex();
			update();
		};

		loadClipboard.onClick = [&]()
		{
			inputDoc.setValue(SystemClipboard::getTextFromClipboard());
		};

		copyClipboard.onClick = [&]()
		{
			SystemClipboard::copyTextToClipboard(outputDoc.getValue().toString());
		};

		GlobalHiseLookAndFeel::setDefaultColours(outputFormatSelector);

        inputDoc.setValue("Paste the SVG data here, drop a SVG file or use the Load from Clipboard button.\nThen select the output format xand variable name above, and click Copy to Clipboard to paste the path data.\nYou can also paste an array that you've previously exported to convert it to Base64");
        
		inputDoc.addListener(this);

		closeButton.onClick = [this]()
		{
			this->destroy();
		};

		setSize(800, 600);
	}

	~SVGToPathDataConverter()
	{
		inputDoc.removeListener(this);
		variableDoc.removeListener(this);
	}

	void valueChanged(Value& v) override
	{
		update();
	}

	void mouseDown(const MouseEvent& e) override
	{
		dragger.startDraggingComponent(this, e);
	}

	void mouseDrag(const MouseEvent& e) override
	{
		dragger.dragComponent(this, e, nullptr);
	}

	void mouseUp(const MouseEvent& e) override
	{
		
	}

	static bool isHiseScriptArray(const String& input)
	{
		return input.startsWith("const var") || input.startsWith("[");
	}

	static String parse(const String& input, OutputFormat format)
	{
		String rt = input.trim();

		if (isHiseScriptArray(rt) || format == OutputFormat::Base64SVG)
		{
			return rt;
		}

		if (auto xml = XmlDocument::parse(input))
		{
			auto v = ValueTree::fromXml(*xml);

			cppgen::ValueTreeIterator::forEach(v, snex::cppgen::ValueTreeIterator::Forward, [&](ValueTree& c)
			{
				if (c.hasType("path"))
				{
					rt = c["d"].toString();
					return true;
				}

				return false;
			});
		}

		return rt;
	}

	Path createPath(const String& url) const override
	{
		Path p;
		p.loadPathFromData(HiBinaryData::ProcessorEditorHeaderIcons::closeIcon, sizeof(HiBinaryData::ProcessorEditorHeaderIcons::closeIcon));
		return p;
	}

	Path pathFromPoints(String pointsText)
	{
		auto points = StringArray::fromTokens(pointsText, " ,", "");
		points.removeEmptyStrings();

		jassert(points.size() % 2 == 0);

		Path p;

		for (int i = 0; i < points.size() / 2; i++)
		{
			auto x = points[i * 2].getFloatValue();
			auto y = points[i * 2 + 1].getFloatValue();

			if (i == 0)
				p.startNewSubPath({ x, y });
			else
				p.lineTo({ x, y });
		}

		p.closeSubPath();

		return p;
	}

	bool isInterestedInFileDrag(const StringArray& files) override
	{
		return File(files[0]).getFileExtension() == ".svg";
	}

	void filesDropped(const StringArray& files, int x, int y) override
	{
		File f(files[0]);
		auto filename = f.getFileNameWithoutExtension();
		variableDoc.setValue(filename);
		inputDoc.setValue(f.loadFileAsString());
	}

	void writeDataAsCppLiteral(const MemoryBlock& mb, OutputStream& out,
		bool breakAtNewLines, bool allowStringBreaks, String bracketSet = "{}")
	{
		const int maxCharsOnLine = 250;

		auto data = (const unsigned char*)mb.getData();
		int charsOnLine = 0;

		bool canUseStringLiteral = mb.getSize() < 32768; // MS compilers can't handle big string literals..

		if (canUseStringLiteral)
		{
			unsigned int numEscaped = 0;

			for (size_t i = 0; i < mb.getSize(); ++i)
			{
				auto num = (unsigned int)data[i];

				if (!((num >= 32 && num < 127) || num == '\t' || num == '\r' || num == '\n'))
				{
					if (++numEscaped > mb.getSize() / 4)
					{
						canUseStringLiteral = false;
						break;
					}
				}
			}
		}

		if (!canUseStringLiteral)
		{
			out << bracketSet.substring(0, 1) << " ";

			for (size_t i = 0; i < mb.getSize(); ++i)
			{
				auto num = (int)(unsigned int)data[i];
				out << num << ',';

				charsOnLine += 2;

				if (num >= 10)
				{
					++charsOnLine;

					if (num >= 100)
						++charsOnLine;
				}

				if (charsOnLine >= maxCharsOnLine)
				{
					charsOnLine = 0;
					out << newLine;
				}
			}

			out << "0,0 " << bracketSet.substring(1) << ";";
		}
		
	}

	void update()
	{
        currentSVG = nullptr;
        path = Path();
        
		auto inputText = parse(inputDoc.toString(), currentOutputFormat);

		String result = "No path generated.. Not a valid SVG path string?";

		if (isHiseScriptArray(inputText))
		{
			auto ar = JSON::parse(inputText.fromFirstOccurrenceOf("[", true, true));

			if (ar.isArray())
			{
				MemoryOutputStream mos;

				for (auto v : *ar.getArray())
				{
					auto byte = (uint8)(int)v;
					mos.write(&byte, 1);
				}
				
				mos.flush();

				path.clear();
				path.loadPathFromData(mos.getData(), mos.getDataSize());

				auto b64 = mos.getMemoryBlock().toBase64Encoding();

				result = {};

				if (!inputText.startsWith("["))
					result << inputText.upToFirstOccurrenceOf("[", false, false);

				result << b64.quoted();

				if (inputText.endsWith(";"))
					result << ";";
			}
		}
		else
		{
			auto text = inputText.trim().unquoted().trim();

            if(currentOutputFormat == OutputFormat::Base64SVG)
            {
                if(auto xml = XmlDocument::parse(text))
                {
                    currentSVG = Drawable::createFromSVG(*xml);
                    
                    currentSVG->setTransformToFit(pathArea, RectanglePlacement::centred);
                }
            }
            else
            {
                path = Drawable::parseSVGPath(text);

                if (path.isEmpty())
                    path = pathFromPoints(text);
                
                if(!path.isEmpty())
                    PathFactory::scalePath(path, pathArea);
            }

            auto filename = snex::cppgen::StringHelpers::makeValidCppName(variableDoc.toString());

			if (!path.isEmpty() || currentSVG != nullptr)
			{
				MemoryOutputStream data;
                
                MemoryBlock mb;
                
                if(currentSVG != nullptr)
                {
                    zstd::ZDefaultCompressor comp;
                    
                    comp.compress(text, mb);
                }
                else
                    mb = data.getMemoryBlock();
                
				path.writePathToStream(data);

				MemoryOutputStream out;

				if (currentOutputFormat == OutputFormat::CppString)
				{
					out << "static const unsigned char " << filename << "[] = ";

					writeDataAsCppLiteral(mb, out, false, true, "{}");

					out << newLine
						<< newLine
						<< "Path path;" << newLine
						<< "path.loadPathFromData (" << filename << ", sizeof (" << filename << "));" << newLine;

				}
				else if (currentOutputFormat == OutputFormat::Base64 || currentOutputFormat == OutputFormat::Base64SVG)
				{
					out << "const var " << filename << " = ";
					out << "\"" << mb.toBase64Encoding() << "\"";
				}
				else if (currentOutputFormat == OutputFormat::HiseScriptNumbers)
				{
					out << "const var " << filename << " = ";
					writeDataAsCppLiteral(mb, out, false, true, "[]");

					out << ";";
				}

				result = out.toString();
			}
		}
		
		outputDoc.setValue(result);

		

		repaint();
	}

	void paint(Graphics& g)
	{
		ColourGradient grad(alaf.dark.withMultipliedBrightness(1.4f), 0.0f, 0.0f,
			alaf.dark, 0.0f, (float)getHeight(), false);

		g.setGradientFill(grad);
		g.fillAll();
		g.setColour(Colours::white.withAlpha(0.1f));
		g.fillRect(getLocalBounds().removeFromTop(37).toFloat());
		g.setColour(alaf.bright);

		g.drawRect(getLocalBounds().toFloat());

		g.setFont(GLOBAL_BOLD_FONT().withHeight(17.0f));
		g.drawText("SVG to Path converter", titleArea, Justification::centred);

        if(currentSVG != nullptr)
        {
            currentSVG->draw(g, 1.0f);
        }
        else if(path.isEmpty())
        {
            g.setFont(GLOBAL_BOLD_FONT());
            g.drawText("No valid path", pathArea, Justification::centred);
        }
        else
        {
            g.fillPath(path);
            g.strokePath(path, PathStrokeType(1.0f));
        }
	}

	void resized() override
	{
		auto b = getLocalBounds();
		
		titleArea = b.removeFromTop(37).toFloat();

		b = b.reduced(10);

		auto top = b.removeFromTop(32);


		
		outputFormatSelector.setBounds(top.removeFromLeft(300));

		top.removeFromLeft(5);

		variableName.setBounds(top.removeFromLeft(200));

		auto bottom = b.removeFromBottom(32);

		b.removeFromBottom(5);

		bottom.removeFromRight(15);

		auto w = getWidth() / 3;

		inputEditor.setBounds(b.removeFromLeft(w-5));
		b.removeFromLeft(5);
		outputEditor.setBounds(b.removeFromLeft(w-5));
		b.removeFromLeft(5);
		pathArea = b.toFloat();

		loadClipboard.setBounds(bottom.removeFromLeft(150));
		bottom.removeFromLeft(10);
		copyClipboard.setBounds(bottom.removeFromLeft(150));

		resizer.setBounds(getLocalBounds().removeFromRight(15).removeFromBottom(15));
		closeButton.setBounds(getLocalBounds().removeFromRight(titleArea.getHeight()).removeFromTop(titleArea.getHeight()).reduced(6));

        if(!path.isEmpty())
            scalePath(path, pathArea);
        else if(currentSVG != nullptr)
            currentSVG->setTransformToFit(pathArea, RectanglePlacement::centred);
            
		repaint();
	}

    std::unique_ptr<Drawable> currentSVG;
	Path path;
	Rectangle<float> pathArea;
	Rectangle<float> titleArea;

	Value inputDoc, outputDoc, variableDoc;

	TextEditor inputEditor, outputEditor;
	TextEditor variableName;

	ComboBox outputFormatSelector;

	OutputFormat currentOutputFormat = OutputFormat::Base64;
	TextButton loadClipboard, copyClipboard;

	ResizableCornerComponent resizer;
	HiseShapeButton closeButton;
	AlertWindowLookAndFeel alaf;
	juce::ComponentDragger dragger;
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

		auto stream = downloadLocation.createInputStream(false, &downloadProgress, this, String(), 0, nullptr, &httpStatusCode, 20);

		if (stream == nullptr || stream->getTotalLength() <= 0)
		{
			result = ErrorCodes::URLNotFound;
			targetDirectory.deleteRecursively();
			return;
		}

		File tempFile(File::getSpecialLocation(File::tempDirectory).getChildFile("projectDownload.tmp"));

		tempFile.deleteFile();
		tempFile.create();

		auto fos = tempFile.createOutputStream();

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

		CompileExporter::BuildOption option = CompileExporter::BuildOption::Cancelled;

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
