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


void DocUpdater::comboBoxChanged(ComboBox* comboBoxThatHasChanged)
{
	if (comboBoxThatHasChanged->getSelectedItemIndex() == 2)
	{
		htmlDirectory->setEnabled(true);
	}
}

static bool canConnectToWebsite(const URL& url)
{
	std::unique_ptr<InputStream> in(url.createInputStream(false, nullptr, nullptr, String(), 2000, nullptr));
	return in != nullptr;
}

static bool areMajorWebsitesAvailable()
{
	const char* urlsToTry[] = { "http://google.com",  "http://bing.com",  "http://amazon.com",
		"https://google.com", "https://bing.com", "https://amazon.com", nullptr };

	for (const char** url = urlsToTry; *url != nullptr; ++url)
		if (canConnectToWebsite(URL(*url)))
			return true;

	return false;
}

DocUpdater::DocUpdater(MarkdownDatabaseHolder& holder_, bool fastMode_, bool allowEdit) :
	MarkdownContentProcessor(holder_),
	DialogWindowWithBackgroundThread("Update documentation", false),
	holder(holder_),
	crawler(new DatabaseCrawler(holder)),
	fastMode(fastMode_),
	editingShouldBeEnabled(allowEdit)
{

	holder.addContentProcessor(this);

	if (!fastMode)
	{

		holder.addContentProcessor(crawler);

		StringArray sa = { "Update local cached file", "Update docs from server" , "Create local HTML offline docs", "Create Snippet database files" };

		addComboBox("action", sa, "Action");

		getComboBoxComponent("action")->addListener(this);

		String help1;
		String nl = "\n";

		help1 << "### Action" << nl;
		help1 << "There are three actions available here:  " << nl;
		help1 << "- You can create the cached files from the markdown files on your system" << nl;
		help1 << "- You can choose to download the cached files from the server." << nl;
		help1 << "- You can create an HTML version of your documentation using the supplied templates" << nl;
		help1 << "- creates a list of markdown files from the HISE Snippet database" << nl;

		helpButton1 = MarkdownHelpButton::createAndAddToComponent(getComboBoxComponent("action"), help1);

		if (!editingShouldBeEnabled)
			getComboBoxComponent("action")->setSelectedItemIndex(1, dontSendNotification);


		String help2;

		help2 << "### BaseURL" << nl;
		help2 << "You can specify a Base URL that will be used in the generated HTML files to resolve relative links.  " << nl;
		help2 << "If you want it to work on your local computer, leave it empty to use the html link to your specified html folder:  " << nl;
		help2 << "`file::///{PATH}/`  " << nl;
		help2 << "otherwise just add your root URL for the online docs, eg.:  " << nl;
		help2 << "`https://docs.hise.dev/`  " << nl;
		help2 << "> Important: The Base URL **must** end with a slash (`/`), otherwise the links won't work.  " << nl;
		help2 << "Also your template header has to have this wildcard (which will be replaced before creating the HTML files...:  " << nl << nl;
		help2 << "```" << nl;
		help2 << "<base href=\"{BASE_URL}\"/>" << nl;
		help2 << "```" << nl;

		addTextEditor("baseURL", "https://docs.hise.dev/", "Base URL");

		helpButton2 = MarkdownHelpButton::createAndAddToComponent(getTextEditor("baseURL"), help2);

		markdownRepository = new FilenameComponent("Markdown Repository", holder.getDatabaseRootDirectory(), false, true, false, {}, {}, "No markdown repository specified");
		markdownRepository->setSize(400, 32);

		auto htmlDir = holder.getDatabaseRootDirectory().getParentDirectory().getChildFile("html_build");


		htmlDirectory = new FilenameComponent("Target directory", htmlDir, true, true, true, {}, {}, "Select a HTML target directory");
		htmlDirectory->setSize(400, 32);

		htmlDirectory->setEnabled(false);

		addCustomComponent(markdownRepository);

		addCustomComponent(htmlDirectory);

		crawler->setProgressCounter(&getProgressCounter());
		holder.setProgressCounter(&getProgressCounter());

		addBasicComponents(true);
	}
	else
	{
		addBasicComponents(false);
		DialogWindowWithBackgroundThread::runThread();
	}
}


DocUpdater::~DocUpdater()
{
	MessageManagerLock mm;

	if (auto t = getCurrentThread())
	{
		t->stopThread(6000);
	}


	currentDownload = nullptr;



	holder.setProgressCounter(nullptr);
	crawler->setLogger(nullptr, true);
	holder.removeContentProcessor(this);
	holder.removeContentProcessor(crawler);

	crawler = nullptr;

}


void DocUpdater::run()
{
	if (fastMode)
	{
		holder.sendServerUpdateMessage(true, true);

		if (!areMajorWebsitesAvailable())
		{
			if (editingShouldBeEnabled)
				getHolder().setForceCachedDataUse(false);

			holder.sendServerUpdateMessage(false, false);
			return;
		}


		holder.setProgressCounter(&getProgressCounter());
		updateFromServer();
		getHolder().setForceCachedDataUse(!editingShouldBeEnabled);



		//addForumLinks();
	}
	else
	{
		auto b = getComboBoxComponent("action");

		if (b->getSelectedItemIndex() == 0)
		{
			showStatusMessage("Rebuilding index");
			holder.setForceCachedDataUse(false);

			showStatusMessage("Create Content cache");


			crawler->clearResolvers();

			holder.addContentProcessor(crawler);

			crawler->createContentTree();


			showStatusMessage("Create Image cache");
			crawler->createImageTree();

#if USE_BACKEND
			crawler->createDataFiles(holder.getCachedDocFolder(), true);
#endif
		}
		if (b->getSelectedItemIndex() == 2)
		{
			createLocalHtmlFiles();
		}

		if (b->getSelectedItemIndex() == 1)
		{
			updateFromServer();
		}
		if(b->getSelectedItemIndex() == 3)
		{
#if USE_BACKEND
			auto mc = GET_BACKEND_ROOT_WINDOW(this)->getBackendProcessor();

			bool done = false;

			mc->getKillStateHandler().killVoicesAndCall(mc->getMainSynthChain(), [&](Processor* p)
			{
				this->createSnippetDatabase();

				done = true;
				return SafeFunctionCall::OK;
			}, MainController::KillStateHandler::TargetThread::SampleLoadingThread);

			while(!done)
			{
				wait(500);
			}
#endif
		}

	}
}

void DocUpdater::threadFinished()
{
	auto b = getComboBoxComponent("action");

	if (!fastMode && b->getSelectedItemIndex() == 0)
	{
		PresetHandler::showMessageWindow("Cache was updated", "Press OK to rebuild the indexes");
		holder.setForceCachedDataUse(true);
	}

	if (result != NotExecuted)
	{
		String s;

		switch (result)
		{
		case DownloadResult::NotExecuted:			break;
		case DownloadResult::UserCancelled:			s = "Operation aborted by user"; break;
		case DownloadResult::ImagesUpdated:			s = "Updated Image blob"; break;
		case DownloadResult::ContentUpdated:		s = "Updated Content blob"; break;
		case DownloadResult::EverythingUpdated:		s = "Updated Content and Image blob"; break;
		case DownloadResult::NothingUpdated:		s = "Everything is up to date"; break;
		case DownloadResult::CantResolveServer:		s = "Can't connect to server"; break;
		case DownloadResult::FileErrorContent:		s = "The Content.dat file is corrupt"; break;
		case DownloadResult::FileErrorImage:		s = "The Image.dat file is corrupt"; break;
		default:
			break;
		}

		if (!fastMode)
		{


			PresetHandler::showMessageWindow("Update finished", s, Helpers::wasOk(result) ? PresetHandler::IconType::Info : PresetHandler::IconType::Error);
		}
	}
}



void DocUpdater::updateFromServer()
{
	if (!fastMode)
		showStatusMessage("Fetching hash from server");

	auto hashURL = getCacheUrl(Hash);

	setTimeoutMs(-1);
	auto content = hashURL.readEntireTextStream(false);
	setTimeoutMs(6000);

	if (threadShouldExit())
	{
		holder.sendServerUpdateMessage(false, false);
		result = UserCancelled;
		return;
	}

	if (content.isEmpty())
	{
		holder.sendServerUpdateMessage(false, false);
		result = CantResolveServer;
		return;
	}

	result = NothingUpdated;

	auto localFile = holder.getCachedDocFolder().getChildFile("hash.json");

	auto webHash = JSON::parse(content);
	auto contentHash = JSON::parse(localFile.loadFileAsString());


	auto webContentHash = (int64)webHash.getProperty("content-hash", {});
	auto webImageHash = (int64)webHash.getProperty("image-hash", {});

	auto localContentHash = (int64)contentHash.getProperty("content-hash", {});
	auto localImageHash = (int64)contentHash.getProperty("image-hash", {});

	if (webContentHash != localContentHash || !localFile.getSiblingFile("content.dat").existsAsFile())
		downloadAndTestFile("content.dat");

	if (threadShouldExit())
	{
		holder.sendServerUpdateMessage(false, false);
		result = UserCancelled;
		return;
	}


	if (webImageHash != localImageHash || !localFile.getSiblingFile("images.dat").existsAsFile())
		downloadAndTestFile("images.dat");

	if (threadShouldExit())
	{
		holder.sendServerUpdateMessage(false, false);
		result = UserCancelled;
		return;
	}

	localFile.replaceWithText(JSON::toString(webHash));

	if (!fastMode)
		showStatusMessage("Rebuilding indexes");

	holder.rebuildDatabase();
	holder.sendServerUpdateMessage(false, true);
}



juce::URL DocUpdater::getCacheUrl(CacheURLType type) const
{
	switch (type)
	{
	case Hash:		return getBaseURL().getChildURL("cache/hash.json");
	case Content:	return getBaseURL().getChildURL("cache/content.dat");
	case Images:	return getBaseURL().getChildURL("cache/images.dat");
	default:
		jassertfalse;
		return {};
	}
}

juce::URL DocUpdater::getBaseURL() const
{
	return holder.getBaseURL();
}

void DocUpdater::createLocalHtmlFiles()
{
	showStatusMessage("Create local HTML files");

	auto htmlDir = htmlDirectory->getCurrentFile();

	String htmlBaseLink = getTextEditorContents("baseURL");

	if (htmlBaseLink.isEmpty())
	{
		htmlBaseLink << "file:///" << htmlDir.getFullPathName();

		htmlBaseLink = htmlBaseLink.replace("\\", "/");

		if (!htmlBaseLink.endsWith("/"))
			htmlBaseLink << "/";
	}

	if (!htmlBaseLink.endsWith("/"))
	{
		showStatusMessage("The base URL needs to end with a slash!");
		reset();
		setProgress(0.0);
		return;

	}


	auto templateDir = getHolder().getDatabaseRootDirectory().getChildFile("template");
	auto templateTarget = htmlDir.getChildFile("template");
	templateDir.copyDirectoryTo(templateTarget);

	auto headerFile = templateTarget.getChildFile("header.html");

	auto headerContent = headerFile.loadFileAsString();

	if (!headerContent.contains("{BASE_URL}"))
	{
		showStatusMessage("Your header file doesn't contain the {BASE_URL} wildcard");
		reset();
		setProgress(0.0);
		return;
	}

	headerContent = headerContent.replace("{BASE_URL}", htmlBaseLink);
	headerFile.replaceWithText(headerContent);

	DatabaseCrawler::createImagesInHtmlFolder(htmlDir, getHolder(), this, &getProgressCounter());
	DatabaseCrawler::createHtmlFilesInHtmlFolder(htmlDir, getHolder(), this, &getProgressCounter());
}

void DocUpdater::createSnippetDatabase()
{
#if USE_BACKEND
	showStatusMessage("Create Snippet database");

	auto root = GET_BACKEND_ROOT_WINDOW(this);

	auto am = dynamic_cast<BackendProcessor*>(root->getBackendProcessor())->getAssetManager();
	am->initialise();

	auto appDataFolder = NativeFileHandler::getAppDataDirectory(nullptr);

	auto f = appDataFolder.getChildFile("snippetBrowser.xml");

	String snippetDirectory;

	if(auto xml = XmlDocument::parse(f))
	{
		if(auto c = xml->getChildByName("snippetDirectory"))
			snippetDirectory = c->getStringAttribute("value");
	}

	if(snippetDirectory.isNotEmpty())
	{
		File snippetRoot(snippetDirectory);

		auto snippetList = snippetRoot.findChildFiles(File::findFiles, false, "*.md");

		struct Item
		{
			Item(const MarkdownLink& l, const String& name_):
			  name(name_)
			{
				auto h = l.getHeaderFromFile({});

				description = l.toString(MarkdownLink::Format::ContentWithoutHeader);
				priority = h.getKeyValue("priority").getIntValue();

				auto catIndex = getCategories().indexOf(h.getKeyValue("category"));

				if(catIndex != -1)
					cat = (Category)catIndex;

				snippet = h.getKeyValue("HiseSnippet");
			}

			static StringArray getCategories()
			{
				return { "All", "Modules", "MIDI", "Scripting", "Scriptnode", "UI" };
			}

			enum class Category
			{
				All,
				Modules,
				Midi,
				Scripting,
				Scriptnode,
				UI,
				numCategories
			};

			Image createScreenshot(BackendRootWindow* rw) const
			{
				
				if(cat == Category::Scriptnode)
				{
					MainController::ScopedBadBabysitter sbb(rw->getBackendProcessor());

					{
						MessageManagerLock mm;
						BackendCommandTarget::Actions::loadSnippet(rw, snippet);
					}
					
					Processor::Iterator<DspNetwork::Holder> iter(rw->getMainSynthChain(), false);

					while(auto holder = iter.getNextProcessor())
					{
						if(auto nw = holder->getActiveNetwork())
						{
							MessageManagerLock mm;

							ScopedPointer<DspNetworkGraphPanel> c;

							c = new scriptnode::DspNetworkGraphPanel(rw->getRootFloatingTile());

							c->setContentWithUndo(dynamic_cast<Processor*>(holder), 0);


							

							if(auto graph = c->getContent<WrapperWithMenuBarBase>()->canvas.getContent<juce::Component>())
							{
								auto comp = dynamic_cast<Component*>(graph);

								while(comp != nullptr)
								{
									comp->setVisible(true);
									comp = comp->getParentComponent();
								}

								auto img = graph->createComponentSnapshot(graph->getLocalBounds(), true);
								return img;
							}
						}
					}
				}
				else
				{
					SuspendHelpers::ScopedTicket ss(rw->getBackendProcessor());

					MainController::ScopedBadBabysitter sbb(rw->getBackendProcessor());

					{
						MessageManagerLock mm;
						BackendCommandTarget::Actions::loadSnippet(rw, snippet);
					}

					if(auto jmp = JavascriptMidiProcessor::getFirstInterfaceScriptProcessor(rw->getBackendProcessor()))
					{
						auto cs = Point<int>(jmp->getScriptingContent()->getContentWidth(), jmp->getScriptingContent()->getContentHeight());

						if(cs == Point<int>(600, 500) || jmp->getScriptingContent()->getNumComponents() == 0)
						{
							Thread::getCurrentThread()->wait(200);
							// skip the interface if its the default value
							return {};
						}

						MessageManagerLock mm;

						ScriptContentComponent content(jmp);
						content.setNewContent(jmp->getScriptingContent());

						content.setSize(content.getContentWidth(), content.getContentHeight());

						return content.createComponentSnapshot(content.getLocalBounds(), true);
					}
				}

				return {};
			}

			String createMarkdown(bool includeImage) const
			{
				String s;
				String nl = "\n";

				s << "## " << name << nl << nl;

				if(includeImage)
				{
					s << "![](/images/snippets/" << MarkdownLink::Helpers::getSanitizedFilename(name) + ".png)" << nl;
				}

				s << description << nl << nl;

				s << "```" << nl;
				s << snippet << nl;
				s << "```" << nl << nl;

				return s;
			}

			bool operator<(const Item& other) const { return priority > other.priority; }
			bool operator>(const Item& other) const { return priority < other.priority; }
			bool operator==(const Item& other) const { return priority == other.priority; }

			Category cat;
			String description;
			String snippet;
			int priority = 3;
			String name;
		};

		std::map<Item::Category, std::vector<Item>> list;

		for(int i = 0; i < (int)Item::Category::numCategories; i++)
		{
			list[(Item::Category)i] = {};
		}

		for(auto sn: snippetList)
		{
			MarkdownLink l(snippetRoot, sn.getFileName());

			Item i(l, sn.getFileNameWithoutExtension());
			list[i.cat].push_back(i);
		}

		String nl = "\n";

		auto targetDirectory = holder.getDatabaseRootDirectory().getChildFile("tutorials/");
		auto imgDirectory = holder.getDatabaseRootDirectory().getChildFile("images").getChildFile("snippets");

		if(imgDirectory.isDirectory())
			imgDirectory.deleteRecursively();

		imgDirectory.createDirectory();

		auto brw = GET_BACKEND_ROOT_WINDOW(this);

		targetDirectory.createDirectory();

		PNGImageFormat format;

		int listIndex = 0;

		StringArray ignoreNames({ "_Template", "SnippetDB", "README"});

		for(auto& l: list)
		{
			String s;

			auto catName = Item::getCategories()[(int)l.first];

			s << "---" << nl;
			s << "keywords: " << catName << " Snippets" << nl;
			s << "summary: Examples in the category " << catName << nl;
			s << "index: " << String(++listIndex) << nl;
			s << "author: various" << nl;
			s << "modified: 01.01.2024" << nl;
			s << "---" << nl << nl;
			
			std::sort(l.second.begin(), l.second.end());

			for(const auto& i: l.second)
			{
				if(ignoreNames.indexOf(i.name) != -1)
					continue;

				showStatusMessage("Create docs for " + i.name);

				auto img = i.createScreenshot(brw);

				if(threadShouldExit())
					return;

				if(img.isValid())
				{
					auto imageFile = imgDirectory.getChildFile(MarkdownLink::Helpers::getSanitizedFilename(i.name)).withFileExtension("png");

					imageFile.deleteFile();

					FileOutputStream fos(imageFile);
					format.writeImageToStream(img, fos);
				}

				s << i.createMarkdown(img.isValid());
			}

			auto targetFile = targetDirectory.getChildFile(MarkdownLink::Helpers::getSanitizedFilename(catName)).getChildFile("readme.md");

			targetFile.getParentDirectory().createDirectory();

			targetFile.replaceWithText(s);
		}
	}
	else
	{
		showStatusMessage("Can't find snippet directory");
	}
#endif
}

void DocUpdater::downloadAndTestFile(const String& targetFileName)
{
	if (!fastMode)
		showStatusMessage("Downloading " + targetFileName);

	auto contentURL = getBaseURL().getChildURL("cache/" + targetFileName);

	auto docDir = holder.getCachedDocFolder();

	if (!docDir.isDirectory())
		docDir.createDirectory();

	auto realFile = holder.getCachedDocFolder().getChildFile(targetFileName);
	auto tmpFile = realFile.getSiblingFile("temp.dat");

	setTimeoutMs(-1);

	currentDownload = contentURL.downloadToFile(tmpFile, {}, this).release();

	if (threadShouldExit())
	{
		result = UserCancelled;
		currentDownload = nullptr;
		tmpFile.deleteFile();
		return;
	}

	while (currentDownload != nullptr && !currentDownload->isFinished())
	{
		if (threadShouldExit())
		{
			result = UserCancelled;
			currentDownload = nullptr;
			tmpFile.deleteFile();
			return;
		}

		Thread::sleep(500);
	}

	currentDownload = nullptr;
	setTimeoutMs(6000);

	if (threadShouldExit())
	{
		result = UserCancelled;
		currentDownload = nullptr;
		tmpFile.deleteFile();
		return;
	}

	if (!fastMode)
		showStatusMessage("Check file integrity");

	zstd::ZDefaultCompressor comp;

	ValueTree test;

	//tmpFile.deleteFile();
	//tmpFile.create();

	auto r = comp.expand(tmpFile, test);

	if (!r.wasOk() || !test.isValid())
		result = Helpers::withError(result);
	else
		tmpFile.copyFileTo(realFile);

	auto ok = tmpFile.deleteFile();
	ignoreUnused(ok);
	jassert(ok);

	result |= Helpers::getIndexFromFileName(targetFileName);
}



void HiseMarkdownPreview::enableEditing(bool shouldBeEnabled)
{
	if (editingEnabled != shouldBeEnabled)
	{
		if (shouldBeEnabled && !getHolder().databaseDirectoryInitialised())
		{
			if (PresetHandler::showYesNoWindow("Setup documentation repository for editing", "You haven't setup a folder for the hise_documentation repository. Do you want to do this now?\nIf you want to edit this documentation, you have to clone the hise_documentation repository and select the folder here."))
			{
				FileChooser fc("Select hise_documentation repository folder", {}, {}, true);

				if (fc.browseForDirectory())
				{
					auto d = fc.getResult();

					bool ok = d.isDirectory() && d.getChildFile("hise-modules").isDirectory();

					if (ok)
					{
						auto& dataObject = dynamic_cast<GlobalSettingManager*>(&getHolder())->getSettingsObject();
						auto vt = dataObject.data;

						if (vt.isValid())
						{
							auto c = vt.getChildWithName(HiseSettings::SettingFiles::DocSettings);

							ValueTree cProp = c.getChildWithName(HiseSettings::Documentation::DocRepository);
							cProp.setProperty("value", d.getFullPathName(), nullptr);

							dataObject.settingWasChanged(HiseSettings::Documentation::DocRepository, d.getFullPathName());

							ScopedPointer<XmlElement> xml = HiseSettings::ConversionHelpers::getConvertedXml(c);

							auto f = dataObject.getFileForSetting(HiseSettings::SettingFiles::DocSettings);

							xml->writeToFile(f, "");

							PresetHandler::showMessageWindow("Success", "You've setup the documentation folder successfully. You can start editing the files and make pull requests to improve this documentation.");
						}

					}
					else
					{
						PresetHandler::showMessageWindow("Invalid folder", "The directory you specified isn't the repository root folder.\nPlease pull the latest state and select the root folder", PresetHandler::IconType::Error);
						topbar.editButton.setToggleStateAndUpdateIcon(false);
						return;
					}
				}
			}
			else
			{
				topbar.editButton.setToggleStateAndUpdateIcon(false);
				return;
			}
		}

		editingEnabled = shouldBeEnabled;

		if (!editingEnabled && PresetHandler::showYesNoWindow("Update local cached documentation", "Do you want to update the local cached documentation from your edited files"))
		{
			auto d = new DocUpdater(getHolder(), false, editingEnabled);
			d->setModalBaseWindowComponent(this);
		}
		else
		{
			auto d = new DocUpdater(getHolder(), true, editingEnabled);
			d->setModalBaseWindowComponent(this);
		}

		if (auto ft = findParentComponentOfClass<FloatingTile>())
		{
			ft->getCurrentFloatingPanel()->setCustomTitle(editingEnabled ? "Preview" : "HISE Documentation");

			if (auto c = ft->getParentContainer())
			{
				c->getComponent(0)->getLayoutData().setVisible(editingEnabled);
				c->getComponent(1)->getLayoutData().setVisible(editingEnabled);
				ft->refreshRootLayout();
			}
		}
	}
}

void HiseMarkdownPreview::editCurrentPage(const MarkdownLink& link, bool showExactContent /*= false*/)
{
	File f;

	if (!showExactContent)
	{
		for (auto lr : linkResolvers)
		{
			f = lr->getFileToEdit(link);

			if (f.existsAsFile())
				break;
		}

		if (!f.existsAsFile())
		{
			f = link.getMarkdownFile(rootDirectory);

			if (!f.existsAsFile())
			{
				if (PresetHandler::showYesNoWindow("No file found", "Do you want to create the file " + f.getFullPathName()))
				{
					String d = "Please enter a brief description.";
					f = MarkdownHeader::createEmptyMarkdownFileWithMarkdownHeader(f.getParentDirectory(), f.getFileNameWithoutExtension(), d);
				}
				else
					return;
			}
		}
	}

	if (showExactContent || f.existsAsFile())
	{
		auto rootWindow = findParentComponentOfClass<ComponentWithBackendConnection>();
		auto tile = rootWindow->getRootFloatingTile();

		FloatingTile::Iterator<FloatingTabComponent> it(tile);

		if (auto tab = it.getNextPanel())
		{
			FloatingInterfaceBuilder ib(tab->getParentShell());

			auto eIndex = ib.addChild<MarkdownEditorPanel>(0);

			auto editor = ib.getContent<MarkdownEditorPanel>(eIndex);
			editor->setPreview(this);


			if (showExactContent)
				editor->loadText(renderer.getCurrentText(true));
			else
				editor->loadFile(f);
		}
	}
	else
	{
		PresetHandler::showMessageWindow("File not found", "The file for the URL " + link.toString(MarkdownLink::Everything) + " + wasn't found.");
	}
}

}
