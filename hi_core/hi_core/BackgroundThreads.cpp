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

void QuasiModalComponent::setModalBaseWindowComponent(Component * childComponentOfModalBaseWindow, int fadeInTime)
{
	ModalBaseWindow *editor = dynamic_cast<ModalBaseWindow*>(childComponentOfModalBaseWindow);

	if (editor == nullptr) editor = childComponentOfModalBaseWindow->findParentComponentOfClass<ModalBaseWindow>();

	jassert(editor != nullptr);

	if (editor != nullptr)
	{
		editor->setModalComponent(dynamic_cast<Component*>(this), fadeInTime);

		isQuasiModal = true;
	}
}

void QuasiModalComponent::showOnDesktop()
{
	Component *t = dynamic_cast<Component*>(this);

	isQuasiModal = false;
	t->setVisible(true);
	t->setOpaque(true);
	t->addToDesktop(ComponentPeer::windowHasCloseButton);
}

void QuasiModalComponent::destroy()
{
	Component *t = dynamic_cast<Component*>(this);

	if (isQuasiModal)
	{
		t->findParentComponentOfClass<ModalBaseWindow>()->clearModalComponent();
	}
	else
	{
		t->removeFromDesktop();
		delete this;
	}
}


DialogWindowWithBackgroundThread::DialogWindowWithBackgroundThread(const String &title, bool synchronous_) :
AlertWindow(title, String(), AlertWindow::AlertIconType::NoIcon),
progress(0.0),
isQuasiModal(false),
synchronous(synchronous_)
{
	setLookAndFeel(&laf);

	setColour(AlertWindow::backgroundColourId, Colour(0xff222222));
	setColour(AlertWindow::textColourId, Colours::white);

#if USE_FRONTEND && !IS_STANDALONE_APP

	// make sure that there is no automatic key focus grabbing going on in hosts...
	auto f = []()
	{
		Component::unfocusAllComponents();
	};

	new DelayedFunctionCaller(f, 200);
#endif



}

DialogWindowWithBackgroundThread::~DialogWindowWithBackgroundThread()
{
	if (thread != nullptr)
	{
		thread->stopThread(6000);
	}
}

void DialogWindowWithBackgroundThread::addBasicComponents(bool addOKButton)
{
	addTextEditor("state", "", "Status", false);

	getTextEditor("state")->setReadOnly(true);

	addProgressBarComponent(progress);
	
	if (addOKButton)
	{
		addButton("OK", 1, KeyPress(KeyPress::returnKey));
	}
	
	addButton("Cancel", 0, KeyPress(KeyPress::escapeKey));

}

bool DialogWindowWithBackgroundThread::threadShouldExit() const
{
	if (thread != nullptr)
	{
		return thread->threadShouldExit();
	};

	return false;
}

void DialogWindowWithBackgroundThread::handleAsyncUpdate()
{
	threadFinished();
	destroy();
}

void DialogWindowWithBackgroundThread::buttonClicked(Button* b)
{
	if (b->getName() == "OK")
	{
		if (!checkConditionsBeforeStartingThread())
			return;

		if (synchronous)
		{
			runSynchronous();
		}
		else if (thread == nullptr)
		{
			thread = new LoadingThread(this);
			thread->startThread();
		}
		
	}
	else if (b->getName() == "Cancel")
	{
		if (thread != nullptr)
		{
			thread->signalThreadShouldExit();
			thread->notify();
			destroy();
		}
		else
		{
			destroy();
		}
	}
	else
	{
		resultButtonClicked(b->getName());
	}
}

void DialogWindowWithBackgroundThread::runSynchronous()
{
	// Obviously only available in the message loop!
	jassert(MessageManager::getInstance()->isThisTheMessageThread());

	run();
	threadFinished();
	destroy();
}

void DialogWindowWithBackgroundThread::showStatusMessage(const String &message)
{
	MessageManagerLock lock(thread);

	if (lock.lockWasGained())
	{
		if (getTextEditor("state") != nullptr)
		{
			getTextEditor("state")->setText(message, dontSendNotification);
		}
		else
		{
			// Did you just call this method before 'addBasicComponents()' ?
			jassertfalse;

		}
	}
}



void DialogWindowWithBackgroundThread::runThread()
{
	thread = new LoadingThread(this);
	thread->startThread();
}

void DialogWindowWithBackgroundThread::AdditionalRow::addComboBox(const String& name, const StringArray& items, const String& label, int width/*=0*/)
{
	auto listener = dynamic_cast<ComboBoxListener*>(parent);

	auto t = new ComboBox(label);
	t->setName(name);
	t->addItemList(items, 1);

	if (listener != nullptr)
		t->addListener(listener);

	t->setSelectedItemIndex(0, dontSendNotification);

	addCustomComponent(t, label, width);
}

void DialogWindowWithBackgroundThread::AdditionalRow::addCustomComponent(Component* newComponent, const String& name, int width/*=0*/)
{
	auto c = new Column(newComponent, name, width);

	addAndMakeVisible(c);
	columns.add(c);
	
	resized();
}

void DialogWindowWithBackgroundThread::AdditionalRow::addButton(const String& name, const KeyPress& k/*=KeyPress()*/, int width/*=0*/)
{
	auto t = new TextButton(name);

	t->addListener(this);
	t->addShortcut(k);

	if (k.isValid())
		t->setButtonText(t->getButtonText() + " (" + k.getTextDescriptionWithIcons() + ")");

	addCustomComponent(t, "", width);
}

void DialogWindowWithBackgroundThread::AdditionalRow::resized()
{
	if (getWidth() == 0)
		return;

	int totalWidth = getWidth() - (columns.size()-1) * 10;

	int numToDivide = columns.size();

	for (auto* c : columns)
	{
		int w = c->width;

		if (w > 0)
		{
			totalWidth -= w;
			numToDivide--;
		}
	}

	int widthPerComponent = 0;

	if (numToDivide > 0)
	{
		widthPerComponent = totalWidth / numToDivide;
	}

	int x = 0;

	for (auto c: columns)
	{
		int widthToUse = c->width > 0 ? c->width : widthPerComponent;

		c->setBounds(x, 0, widthToUse, getHeight());
		x += widthToUse;
		x += 10;
	}
}



ModalBaseWindow::ModalBaseWindow()
{
	s.colour = Colours::black;
	s.radius = 20;
	s.offset = Point<int>();
}

ModalBaseWindow::~ModalBaseWindow()
{

	shadow = nullptr;
	clearModalComponent();
}

void ModalBaseWindow::setModalComponent(Component *component, int fadeInTime/*=0*/)
{
	if (modalComponent != nullptr)
	{
		shadow = nullptr;
		modalComponent = nullptr;
	}


	shadow = new DropShadower(s);
	modalComponent = component;


	if (fadeInTime == 0)
	{
		dynamic_cast<Component*>(this)->addAndMakeVisible(modalComponent);
		modalComponent->centreWithSize(component->getWidth(), component->getHeight());

	}
	else
	{
		dynamic_cast<Component*>(this)->addChildComponent(modalComponent);
		modalComponent->centreWithSize(component->getWidth(), component->getHeight());
		Desktop::getInstance().getAnimator().fadeIn(modalComponent, fadeInTime);

	}



	shadow->setOwner(modalComponent);
}

bool ModalBaseWindow::isCurrentlyModal() const
{
	return modalComponent != nullptr;
}

void ModalBaseWindow::clearModalComponent()
{
	shadow = nullptr;
	modalComponent = nullptr;
}


const hise::MainController* ModalBaseWindow::getMainController() const
{
#if USE_BACKEND
	return dynamic_cast<const BackendRootWindow*>(this)->getBackendProcessor();
	
#else
	auto fp = dynamic_cast<const FrontendProcessorEditor*>(this)->getAudioProcessor();
	return dynamic_cast<const MainController*>(fp);
#endif
}

hise::MainController* ModalBaseWindow::getMainController()
{
#if USE_BACKEND
	return dynamic_cast<BackendRootWindow*>(this)->getBackendProcessor();
#else
	auto fp = dynamic_cast<FrontendProcessorEditor*>(this)->getAudioProcessor();
	return dynamic_cast<MainController*>(fp);
#endif
}

SampleDataExporter::SampleDataExporter(ModalBaseWindow* mbw) :
	DialogWindowWithBackgroundThread("Export Samples for Installer"),
	modalBaseWindow(mbw),
	synthChain(mbw->getMainController()->getMainSynthChain())
{
	StringArray sa;
	sa.add("Export Monolith files only");

	addComboBox("file_selection", sa, "Select files to export");

	StringArray sa2;

	sa2.add("500 MB");
	sa2.add("1 GB");
	sa2.add("1.5 GB");
	sa2.add("2 GB");

	addComboBox("split", sa2, "Split archive size");

	targetFile = new FilenameComponent("Target directory", File(), true, true, true, "", "", "Choose export directory");
	targetFile->setSize(300, 24);
	addCustomComponent(targetFile);

	totalProgressBar = new ProgressBar(totalProgress);
	totalProgressBar->setName("Total Progress");
	totalProgressBar->setSize(300, 24);
	addCustomComponent(totalProgressBar);

	addBasicComponents(true);

	showStatusMessage("Select the target file and press OK");
}

void SampleDataExporter::logVerboseMessage(const String& verboseMessage)
{
#if USE_BACKEND
	debugToConsole(synthChain, verboseMessage);
#else
	ignoreUnused(verboseMessage);
#endif
}

void SampleDataExporter::logStatusMessage(const String& message)
{
	showStatusMessage(message);
}

void SampleDataExporter::run()
{
	showStatusMessage("Collecting samples");

	showStatusMessage("Exporting");

	hlac::HlacArchiver compressor(getCurrentThread());

	compressor.setListener(this);

	hlac::HlacArchiver::CompressData data;

	data.targetFile = getTargetFile();
	data.metadataJSON = getMetadataJSON();
	data.fileList = collectMonoliths();
	data.progress = &progress;
	data.totalProgress = &totalProgress;
	data.partSize = 1024 * 1024;

	auto partSize = (PartSize)getComboBoxComponent("split")->getSelectedItemIndex();

	switch (partSize)
	{
	case PartSize::HalfGig: data.partSize *= 500; break;
	case PartSize::OneGig: data.partSize *= 1000; break;
	case PartSize::OneAndHalfGig: data.partSize *= 1500; break;
	case PartSize::TwoGig: data.partSize *= 2000; break;
    default: break;
		break;
	}

	compressor.compressSampleData(data);
}

void SampleDataExporter::threadFinished()
{
	PresetHandler::showMessageWindow("Samples successfully exported", "All samples were exported without errors");
}

Array<File> SampleDataExporter::collectMonoliths()
{
	Array<File> sampleMonoliths;

	File sampleDirectory = GET_PROJECT_HANDLER(modalBaseWindow->getMainController()->getMainSynthChain()).getSubDirectory(ProjectHandler::SubDirectories::Samples);

	sampleDirectory.findChildFiles(sampleMonoliths, File::findFiles, false, "*.ch*");

	sampleMonoliths.sort();

	numExported = sampleMonoliths.size();

	return sampleMonoliths;
}

String SampleDataExporter::getMetadataJSON() const
{
	DynamicObject::Ptr d = new DynamicObject();

	d->setProperty("Name", getProjectName());
	d->setProperty("Version", getProjectVersion());
	d->setProperty("Company", getCompanyName());

	var data(d);

	return JSON::toString(data, true);
}

String SampleDataExporter::getProjectName() const
{
	return SettingWindows::getSettingValue((int)SettingWindows::ProjectSettingWindow::Attributes::Name, &GET_PROJECT_HANDLER(synthChain));
}

String SampleDataExporter::getCompanyName() const
{
	return SettingWindows::getSettingValue((int)SettingWindows::UserSettingWindow::Attributes::Company, &GET_PROJECT_HANDLER(synthChain));
}

String SampleDataExporter::getProjectVersion() const
{
	return SettingWindows::getSettingValue((int)SettingWindows::ProjectSettingWindow::Attributes::Version, &GET_PROJECT_HANDLER(synthChain));
}

File SampleDataExporter::getTargetFile() const
{
	auto currentFile = targetFile->getCurrentFile();
	auto name = getProjectName();
	auto version = getProjectVersion();
	version = version.replaceCharacter('.', '_');
	auto fileName = name + "_" + version + "_Samples.hr1";
	return currentFile.getChildFile(fileName);
}


File getDefaultSampleDestination()
{

#if USE_FRONTEND

	const String product = ProjectHandler::Frontend::getProjectName();
	const String company = ProjectHandler::Frontend::getCompanyName();

	const String path = company + "/" + product + "/Samples";

#if JUCE_WINDOWS
	const File sourceD = File::getSpecialLocation(File::globalApplicationsDirectory).getChildFile(path);
#else
	const File sourceD = File::getSpecialLocation(File::userMusicDirectory).getChildFile(path);
#endif

	if (!sourceD.isDirectory())
		sourceD.createDirectory();

	return sourceD;

#else
	return File();

#endif

	


}

SampleDataImporter::SampleDataImporter(ModalBaseWindow* mbw) :
	DialogWindowWithBackgroundThread("Install Sample Archive"),
	modalBaseWindow(mbw),
	synthChain(mbw->getMainController()->getMainSynthChain()),
	result(Result::ok())
{


#if USE_FRONTEND

	const String productName = getProjectName();
	const String version = getProjectVersion();

	PresetHandler::showMessageWindow("Choose the Sample Archive", "Please select the " + productName + " Resources " + version + ".hr1 file that you've downloaded");

	FileChooser fc("Choose the Sample Archive", File::getSpecialLocation(File::userHomeDirectory), "*.hr1", true);

	if (fc.browseForFileToOpen())
	{
		archiveFile = fc.getResult();
	}

	PresetHandler::showMessageWindow("Choose the Sample location folder", "Please select the location where you want to install the samples");

	File sampleDestination = getDefaultSampleDestination();
	FileChooser fc2("Choose the Sample location folder", sampleDestination);

	if (fc2.browseForDirectory())
	{
		sampleDestination = fc2.getResult();
	}


	targetFile = new FilenameComponent("Sample Archive Location", archiveFile, true, false, false, "*.hr1", "", "Choose the Sample Archive");
	targetFile->setSize(300, 24);
	addCustomComponent(targetFile);

	sampleDirectory = new FilenameComponent("Sample Folder", sampleDestination, true, true, true, "", "", "Choose the Sample location folder");

	sampleDirectory->setSize(300, 24);
	addCustomComponent(sampleDirectory);

#endif

	StringArray sa;

	sa.add("Overwrite if newer");
	sa.add("Leave existing files");
	sa.add("Force overwrite");

	addComboBox("overwrite", sa, "Overwrite existing samples");

	StringArray sa2;

	sa2.add("No");
	sa2.add("Yes");

	addComboBox("deleteArchive", sa2, "Delete Sample Archive after extraction");

	getComboBoxComponent("deleteArchive")->setSelectedItemIndex(0, dontSendNotification);

	partProgressBar = new ProgressBar(partProgress);
	partProgressBar->setName("Part Progress");
	partProgressBar->setSize(300, 24);

#if USE_BACKEND
	addCustomComponent(partProgressBar);
#endif

	totalProgressBar = new ProgressBar(totalProgress);
	totalProgressBar->setSize(300, 24);
	totalProgressBar->setName("Total Progress");
	addCustomComponent(totalProgressBar);

	addBasicComponents(true);

#if USE_FRONTEND

	if (archiveFile.existsAsFile() && sampleDestination.isDirectory())
	{
		showStatusMessage("Press OK to extract the samples");
	}
	else if (!archiveFile.existsAsFile())
	{
		showStatusMessage("Please choose the Sample Archive file");
	}
	else if (!sampleDestination.isDirectory())
	{
		showStatusMessage("Please choose the target directory");
	}

#else
	showStatusMessage("Choose a sample archive and press OK.");
#endif

	
}

void SampleDataImporter::logVerboseMessage(const String& verboseMessage)
{
#if USE_BACKEND
	debugToConsole(synthChain, verboseMessage);
#else
	ignoreUnused(verboseMessage);
#endif
	
}

void SampleDataImporter::logStatusMessage(const String& message)
{
	showStatusMessage(message);
}

void SampleDataImporter::run()
{
#if USE_FRONTEND

	if (!sampleDirectory->getCurrentFile().isDirectory())
	{
		result = Result::fail("You haven't specified a valid target directory");
		return;
	}
		

#endif

	result = Result::fail("User pressed cancel");

	showStatusMessage("Reading metadata");

	auto md = getMetadata();

	showStatusMessage("Importing Samples");

	auto option = (hlac::HlacArchiver::OverwriteOption)getComboBoxComponent("overwrite")->getSelectedItemIndex();

	hlac::HlacArchiver::DecompressData data;

	data.option = option;
	data.sourceFile = getSourceFile();
	data.targetDirectory = getTargetDirectory();
	data.progress = &progress;
	data.partProgress = &partProgress;
	data.totalProgress = &totalProgress;

	hlac::HlacArchiver decompressor(getCurrentThread());

	decompressor.setListener(this);

	bool ok = decompressor.extractSampleData(data);

	if (!ok)
	{
		result = Result::fail("Something went wrong during extraction");
		return;
	}
		

#if USE_FRONTEND
	
	auto sampleLocation = sampleDirectory->getCurrentFile();

	ProjectHandler::Frontend::setSampleLocation(sampleLocation);

	
#endif

	result = Result::ok();

}

void SampleDataImporter::threadFinished()
{
	if (!result.wasOk())
	{
		PresetHandler::showMessageWindow("Error during sample installation", result.getErrorMessage());
	}
	else
	{
		PresetHandler::showMessageWindow("Samples imported", "All samples were imported successfully.");

#if USE_FRONTEND

		auto fpe = dynamic_cast<FrontendProcessorEditor*>(modalBaseWindow);

		fpe->setSamplesCorrectlyInstalled(true);

#endif

		const bool deleteArchive = getComboBoxComponent("deleteArchive")->getSelectedItemIndex() != 0;

		if (deleteArchive && archiveFile.existsAsFile())
			archiveFile.deleteFile();

	}
}

bool SampleDataImporter::checkConditionsBeforeStartingThread()
{
	if (!getSourceFile().existsAsFile())
	{
		PresetHandler::showMessageWindow("No Sample Archive selected", "Please select the " + getProjectName() + " Resources " + getProjectVersion() + ".hr1 file that you've downloaded", PresetHandler::IconType::Warning);
		return false;
	}

	if (!getTargetDirectory().isDirectory())
	{
		PresetHandler::showMessageWindow("No Sample Location selected", "Please select the location where you want to install the samples", PresetHandler::IconType::Warning);
		return false;
	}

	return true;
}

String SampleDataImporter::getProjectName() const
{
#if USE_BACKEND
	return SettingWindows::getSettingValue((int)SettingWindows::ProjectSettingWindow::Attributes::Name, &GET_PROJECT_HANDLER(synthChain));
#else
	return ProjectHandler::Frontend::getProjectName();
#endif
}

String SampleDataImporter::getCompanyName() const
{
#if USE_BACKEND
	return SettingWindows::getSettingValue((int)SettingWindows::UserSettingWindow::Attributes::Company, &GET_PROJECT_HANDLER(synthChain));
#else
	return ProjectHandler::Frontend::getCompanyName();
#endif
}

String SampleDataImporter::getProjectVersion() const
{
#if USE_BACKEND
	return SettingWindows::getSettingValue((int)SettingWindows::ProjectSettingWindow::Attributes::Version, &GET_PROJECT_HANDLER(synthChain));
#else

	// TODO: change this later...
	return "1.0.0";

	//return ProjectHandler::Frontend::getVersionString();
#endif
}

File SampleDataImporter::getTargetDirectory() const
{
#if USE_BACKEND
	return GET_PROJECT_HANDLER(synthChain).getSubDirectory(ProjectHandler::SubDirectories::Samples);
#else
	return sampleDirectory->getCurrentFile();
#endif
}

String SampleDataImporter::getMetadata() const
{
	return hlac::HlacArchiver::getMetadataJSON(getSourceFile());
}

File SampleDataImporter::getSourceFile() const
{
	return targetFile->getCurrentFile();
}


juce::Path DialogWindowWithBackgroundThread::AdditionalRow::Column::getPath()
{
	static const unsigned char pathData[] = { 110,109,0,183,97,67,0,111,33,67,98,32,154,84,67,0,111,33,67,128,237,73,67,32,27,44,67,128,237,73,67,0,56,57,67,98,128,237,73,67,224,84,70,67,32,154,84,67,128,1,81,67,0,183,97,67,128,1,81,67,98,224,211,110,67,128,1,81,67,0,128,121,67,224,84,70,67,0,128,
		121,67,0,56,57,67,98,0,128,121,67,32,27,44,67,224,211,110,67,0,111,33,67,0,183,97,67,0,111,33,67,99,109,0,183,97,67,0,111,37,67,98,119,170,108,67,0,111,37,67,0,128,117,67,137,68,46,67,0,128,117,67,0,56,57,67,98,0,128,117,67,119,43,68,67,119,170,108,67,
		128,1,77,67,0,183,97,67,128,1,77,67,98,137,195,86,67,128,1,77,67,128,237,77,67,119,43,68,67,128,237,77,67,0,56,57,67,98,128,237,77,67,137,68,46,67,137,195,86,67,0,111,37,67,0,183,97,67,0,111,37,67,99,109,0,124,101,67,32,207,62,67,108,0,16,94,67,32,207,
		62,67,108,0,16,94,67,32,17,62,67,113,0,16,94,67,32,44,60,67,0,126,94,67,32,0,59,67,113,0,236,94,67,32,207,57,67,0,195,95,67,32,213,56,67,113,0,159,96,67,32,219,55,67,0,151,99,67,32,101,53,67,113,0,44,101,67,32,27,52,67,0,44,101,67,32,8,51,67,113,0,44,
		101,67,32,245,49,67,0,135,100,67,32,95,49,67,113,0,231,99,67,32,196,48,67,0,157,98,67,32,196,48,67,113,0,58,97,67,32,196,48,67,0,79,96,67,32,175,49,67,113,0,105,95,67,32,154,50,67,0,40,95,67,32,227,52,67,108,0,148,87,67,32,243,51,67,113,0,248,87,67,32,
		197,47,67,0,155,90,67,32,59,45,67,113,0,67,93,67,32,172,42,67,0,187,98,67,32,172,42,67,113,0,253,102,67,32,172,42,67,0,155,105,67,32,115,44,67,113,0,41,109,67,32,218,46,67,0,41,109,67,32,219,50,67,113,0,41,109,67,32,132,52,67,0,62,108,67,32,15,54,67,
		113,0,83,107,67,32,154,55,67,0,126,104,67,32,212,57,67,113,0,133,102,67,32,100,59,67,0,254,101,67,32,89,60,67,113,0,124,101,67,32,73,61,67,0,124,101,67,32,207,62,67,99,109,0,207,93,67,32,200,64,67,108,0,194,101,67,32,200,64,67,108,0,194,101,67,32,203,
		71,67,108,0,207,93,67,32,203,71,67,108,0,207,93,67,32,200,64,67,99,101,0,0 };

	Path path;
	path.loadPathFromData(pathData, sizeof(pathData));


	return path;
}

MarkdownParser::MarkdownParser(const String& markdownCode_) :
	markdownCode(markdownCode_)
{
	Font bold = GLOBAL_BOLD_FONT();
	Font normal = GLOBAL_FONT();
	Font code = GLOBAL_MONOSPACE_FONT();
}

juce::AttributedString MarkdownParser::parse()
{
	Iterator it(markdownCode);

	juce::juce_wchar c;

	while(it.next(c))
	{
		switch (c)
		{
		case '*':
		{
			if (it.peek() == '*')
			{
				it.next(c);

				isBold = !isBold;
				currentFont = isBold ? GLOBAL_BOLD_FONT() : GLOBAL_FONT();
			}
			else
			{
				isItalic = !isItalic;
			}

			break;
		}
		case '#':
		{
			headlineLevel = 1;

			while (it.next(c) && c == '#')
			{
				headlineLevel++;
			}

			break;
		}
		case '\n':
		{
			headlineLevel = 0;
		}
		default:
			
			currentFont = currentFont.withHeight(14.0 + 2 * headlineLevel);
			s.append(String(c), currentFont);
		}
	}

	return s;
}

void MarkdownParser::parseBoldBlock()
{
	juce::juce_wchar c;

	while (it.next(c))
	{
		switch (c)
		{
		case '*':
		{
			if (it.peek() == '*')
			{
				it.next(c);
				return;
			}
			else
			{
				parseItalicBlock();
			}
		}
		}
	}
}

void MarkdownParser::parseItalicBlock()
{

}

void MarkdownParser::parseHeadline(int level)
{

}

void MarkdownParser::parseText()
{

}

juce::CharPointer_UTF8 MarkdownParser::parseMarkdownBlock()
{

}

} // namespace hise