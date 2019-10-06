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


MarkdownPreview::MarkdownPreview(MarkdownDatabaseHolder& holder) :
	MarkdownContentProcessor(holder),
	layoutCache(),
	renderer("", &layoutCache),
	toc(*this),
	viewport(*this),
	internalComponent(*this),
	topbar(*this),
	rootDirectory(holder.getDatabaseRootDirectory())
{
	//jassert(dynamic_cast<MainController*>(&holder)->isFlakyThreadingAllowed());

	renderer.setDatabaseHolder(&holder);
	renderer.setCreateFooter(holder.getDatabase().createFooter);

	setLookAndFeel(&laf);
	viewport.setViewedComponent(&internalComponent, false);
	viewport.addListener(&renderer);
	addAndMakeVisible(viewport);
	addAndMakeVisible(toc);
	addAndMakeVisible(topbar);

	setWantsKeyboardFocus(true);

	topbar.database = &holder.getDatabase();
	
	holder.addContentProcessor(this);

	setNewText(" ", {});
}

MarkdownPreview::~MarkdownPreview()
{
	viewport.removeListener(&renderer);
}

void MarkdownPreview::editCurrentPage(const MarkdownLink& lastLink, bool showExactContent)
{
	
	File f;

	if (!showExactContent)
	{
		for (auto lr : linkResolvers)
		{
			f = lr->getFileToEdit(lastLink);

			if (f.existsAsFile())
			{
				break;
			}
		}

		if (!f.existsAsFile())
		{
			f = lastLink.getMarkdownFile(rootDirectory);

			if (!f.existsAsFile())
			{
				if (PresetHandler::showYesNoWindow("No file found", "Do you want to create the file " + f.getFullPathName()))
				{
					String d = "Please enter a brief description.";
					f = MarkdownHeader::createEmptyMarkdownFileWithMarkdownHeader(f.getParentDirectory(), f.getFileNameWithoutExtension(), d);
				}
				else
				{
					return;
				}
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
		PresetHandler::showMessageWindow("File not found", "The file for the URL " + lastLink.toString(MarkdownLink::Everything) + " + wasn't found.");
	}

}


void MarkdownPreview::enableEditing(bool shouldBeEnabled)
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

bool MarkdownPreview::keyPressed(const KeyPress& k)
{
	if (k.getModifiers().isCommandDown() && k.getKeyCode() == 'C')
	{
		auto s = renderer.getSelectionContent();

		if (s.isNotEmpty())
			SystemClipboard::copyTextToClipboard(s);

		return true;
	}

	if (k.getModifiers().isCommandDown() && k.getKeyCode() == 'F')
	{
		topbar.searchBar.showEditor();
		return true;
	}

	return false;
}

void MarkdownPreview::setNewText(const String& newText, const File& f)
{
	internalComponent.setNewText(newText, f);
}

void MarkdownPreview::resized()
{
	auto ar = getLocalBounds();

	if (shouldDisplay(ViewOptions::Topbar))
	{
		auto topBounds = ar.removeFromTop(46);

		topbar.setBounds(topBounds);
		topbar.resized();
	}

	if (shouldDisplay(ViewOptions::Toc) && toc.isVisible())
	{
		toc.setBounds(ar.removeFromLeft(toc.getPreferredWidth()));
	}

	renderer.updateCreatedComponents();

	ar.removeFromLeft(32);
	ar.removeFromTop(16);
	ar.removeFromRight(16);
	ar.removeFromBottom(16);

	viewport.setBounds(ar);

	auto h = internalComponent.getTextHeight();

	internalComponent.setSize(jmin(800, viewport.getWidth() - viewport.getScrollBarThickness()), h);
}

MarkdownPreview::InternalComponent::InternalComponent(MarkdownPreview& parent_) :
	parent(parent_),
	renderer(parent.renderer)
{
	
}

MarkdownPreview::InternalComponent::~InternalComponent()
{
}

int MarkdownPreview::InternalComponent::getTextHeight()
{
	return (int)renderer.getHeightForWidth((float)getWidth());
}

void MarkdownPreview::InternalComponent::setNewText(const String& s, const File& )
{
	currentSearchResult = {};

	renderer.setStyleData(styleData);
	renderer.addListener(this);
	renderer.setNewText(s);

	for (auto lr : parent.linkResolvers)
		renderer.setLinkResolver(lr->clone(&renderer));

	for (auto ip : parent.imageProviders)
		renderer.setImageProvider(ip->clone(&renderer));

	renderer.parse();
	

	auto result = renderer.getParseResult();

	if (getWidth() > 0)
	{
		renderer.getHeightForWidth((float)getWidth());
		
	}

	if (result.failed())
		errorMessage = result.getErrorMessage();
	else
		errorMessage = {};



    scrollToAnchor(0.0f);
    
	repaint();
}

void MarkdownPreview::InternalComponent::markdownWasParsed(const Result& r)
{
	if (parent.getHolder().nothingInHere())
	{
		parent.viewport.setVisible(false);
	}
	else
	{
		parent.viewport.setVisible(true);
	}

	
	if (getWidth() == 0)
		return;

	if (r.wasOk())
	{
		errorMessage = {};
		currentSearchResult = {};

		parent.toc.scrollToLink(renderer.getLastLink());
		auto h = renderer.getHeightForWidth((float)getWidth());

		renderer.setTargetComponent(this);
		setSize(getWidth(), (int)h);
		renderer.updateCreatedComponents();
        
        if(renderer.getLastLink().toString(MarkdownLink::AnchorWithHashtag).isEmpty())
            scrollToAnchor(0.0f);

		repaint();

	}
	else
	{
		errorMessage = r.getErrorMessage();
		repaint();
	}

	parent.topbar.updateNavigationButtons();
}

void MarkdownPreview::InternalComponent::mouseDown(const MouseEvent& e)
{
	parent.currentSearchResults = nullptr;

	if (renderer.navigateFromXButtons(e))
		return;

	if (enableSelect)
	{
		currentLasso.setPosition(e.getPosition());
		currentLasso.setSize(0, 0);

		renderer.updateSelection({});

		repaint();
	}
	
	if (e.mods.isRightButtonDown())
	{
		PopupMenu m;
		hise::PopupLookAndFeel plaf;
		m.setLookAndFeel(&plaf);

		auto anchor = renderer.getAnchorForY(e.getMouseDownPosition().getY());

		auto link = renderer.getLastLink().withAnchor(anchor);


		m.addItem(1, "Back", renderer.canNavigate(true));
		m.addItem(2, "Forward", renderer.canNavigate(false));
        

#if USE_BACKEND
		m.addItem(3, "Export");
#endif

		parent.addEditingMenuItems(m);

		auto result = m.show();

		if (result == 1)
		{
			renderer.navigate(true);
			repaint();
		}
		if (result == 2)
		{
			renderer.navigate(false);
			repaint();
		}
		if (result == 3)
		{
			auto doc = new DocUpdater(parent.getHolder(), false, parent.editingEnabled);
			doc->setModalBaseWindowComponent(this);
		}
        
		parent.performPopupMenuForEditingIcons(result, link);

	}
}

void MarkdownPreview::InternalComponent::mouseDrag(const MouseEvent& e)
{
	if (enableSelect)
	{
		currentLasso = Rectangle<int>(e.getMouseDownPosition(), e.getPosition());

		renderer.updateSelection(currentLasso.toFloat());
		repaint();
	}
}

void MarkdownPreview::InternalComponent::mouseUp(const MouseEvent& e)
{
	currentLasso = {};

	if (e.mods.isLeftButtonDown())
	{
		clickedLink = {};

		auto l = renderer.getLinkForMouseEvent(e, getLocalBounds().toFloat());

		if(l.isValid())
			renderer.gotoLink(l.withRoot(parent.rootDirectory, true));
	}
	
	repaint();
}

void MarkdownPreview::InternalComponent::mouseMove(const MouseEvent& event)
{
	auto link = renderer.getHyperLinkForEvent(event, getLocalBounds().toFloat());

	if (link.valid)
	{
		if (link.tooltip.isEmpty())
			setTooltip(link.url.toString(MarkdownLink::UrlFull));
		else
			setTooltip(link.tooltip);
	}
	else
		setTooltip("");

	setMouseCursor(link.valid ? MouseCursor::PointingHandCursor : MouseCursor::NormalCursor);
}


void MarkdownPreview::InternalComponent::mouseWheelMove(const MouseEvent& event, const MouseWheelDetails& details)
{
	if (event.mods.isCommandDown())
	{
		float delta = details.deltaY > 0 ? 1.0f : -1.0f;

		styleData.fontSize = jlimit<float>(17.0f, 30.0f, styleData.fontSize + delta);

		renderer.setStyleData(styleData);
	}
	else
	{
		Component::mouseWheelMove(event, details);
	}

}

void MarkdownPreview::InternalComponent::scrollToAnchor(float v)
{
	if (auto viewPort = findParentComponentOfClass<Viewport>())
	{
		viewPort->setViewPosition({ 0, (int)v });
	}
}

void MarkdownPreview::InternalComponent::scrollToSearchResult(Rectangle<float> v)
{
	currentSearchResult = v;
	scrollToAnchor(jmax(0.0f, v.getY() - 32.0f));
	repaint();
}

void MarkdownPreview::InternalComponent::paint(Graphics & g)
{
	g.fillAll(styleData.backgroundColour);

	auto bounds = findParentComponentOfClass<CustomViewport>()->visibleArea;

	if (errorMessage.isNotEmpty())
	{
		g.setColour(Colours::white);
		g.setFont(GLOBAL_BOLD_FONT());
		g.drawText(errorMessage, bounds, Justification::centred);
		return;
	}

	float height = (float)getTextHeight();

	auto ar = Rectangle<float>(0, 0, (float)getWidth(), height);

	renderer.draw(g, ar, bounds);

	if (!currentLasso.isEmpty())
	{
		g.setColour(styleData.headlineColour.withAlpha(0.2f));
		g.fillRect(currentLasso);
	}

	if (!currentSearchResult.isEmpty())
	{
		g.setColour(Colours::red);
		g.drawRoundedRectangle(currentSearchResult, 2.0f, 2.0f);
	}
}

juce::Path MarkdownPreview::Topbar::TopbarPaths::createPath(const String& id) const
{
	auto url = MarkdownLink::Helpers::getSanitizedFilename(id);

	Path p;

	LOAD_PATH_IF_URL("back", EditorIcons::backIcon);
	LOAD_PATH_IF_URL("forward", EditorIcons::forwardIcon);
	LOAD_PATH_IF_URL("search", EditorIcons::searchIcon2);
	LOAD_PATH_IF_URL("home", MainToolbarIcons::home);
	
	LOAD_PATH_IF_URL("drag", EditorIcons::dragIcon);
	LOAD_PATH_IF_URL("select", EditorIcons::selectIcon);
	LOAD_PATH_IF_URL("sun", EditorIcons::sunIcon);
	LOAD_PATH_IF_URL("night", EditorIcons::nightIcon);
	LOAD_PATH_IF_URL("book", EditorIcons::bookIcon);
	LOAD_PATH_IF_URL("rebuild", ColumnIcons::moveIcon);
#if USE_BACKEND
	LOAD_PATH_IF_URL("toc", BackendBinaryData::ToolbarIcons::hamburgerIcon);
	LOAD_PATH_IF_URL("edit", OverlayIcons::penShape);
	LOAD_PATH_IF_URL("lock", OverlayIcons::lockShape);
#endif

	return p;
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

		StringArray sa = { "Update local cached file", "Update docs from server" , "Create local HTML offline docs" };

		addComboBox("action", sa, "Action");

		getComboBoxComponent("action")->addListener(this);

		String help1;
		String nl = "\n";

		help1 << "### Action" << nl;
		help1 << "You can choose to download the latest documentation blob from the HISE doc server." << nl;
		help1 << "> It will check the existing hash code and only download something if there is new content" << nl;

		helpButton1 = MarkdownHelpButton::createAndAddToComponent(getComboBoxComponent("action"), help1);

		if (!editingShouldBeEnabled)
			getComboBoxComponent("action")->setSelectedItemIndex(1, dontSendNotification);

		String help2;
		help2 << "### Add Forum Links" << nl;
		help2 << "This will search the subforum [Documentation Discussion](link) for matches to documentation pages and adds a link to the bottom of each page." << nl;


		StringArray sa2 = { "Yes", "No" };

		addComboBox("forum", sa2, "Add links to forum discussion");

		helpButton2 = MarkdownHelpButton::createAndAddToComponent(getComboBoxComponent("forum"), help2);

		markdownRepository = new FilenameComponent("Markdown Repository", holder.getDatabaseRootDirectory(), false, true, false, {}, {}, "No markdown repository specified");
		markdownRepository->setSize(400, 32);

		auto htmlDir = holder.getDatabaseRootDirectory().getParentDirectory().getChildFile("html");


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

void DocUpdater::comboBoxChanged(ComboBox* comboBoxThatHasChanged)
{
	if (comboBoxThatHasChanged->getSelectedItemIndex() == 2)
	{
		htmlDirectory->setEnabled(true);
	}
}

void DocUpdater::run()
{
	if (fastMode)
	{
		

		showStatusMessage("Rebuilding Documentation Index");
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
			
			addForumLinks();

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

void DocUpdater::addForumLinks()
{
	if (fastMode || getComboBoxComponent("forum")->getSelectedItemIndex() == 1)
		return;

	showStatusMessage("Adding forum links to pages");

	URL baseUrl("https://forum.hise.audio");
	auto forumUrl = baseUrl.getChildURL("api/category/13/");
	auto itemList = holder.getDatabase().getFlatList();

	setTimeoutMs(-1);

	auto response = forumUrl.readEntireTextStream(false);

	setTimeoutMs(6000);

	if (threadShouldExit())
	{
		result = UserCancelled;
		return;
	}

	auto v = JSON::parse(response);

	int numFound = 0;

	if (auto postList = v.getProperty("topics", {}).getArray())
	{
		for (auto p : *postList)
		{
			auto title = p.getProperty("title", {}).toString();

			if (title.isNotEmpty())
			{
				for (const auto& item : itemList)
				{
					if (item.tocString == title)
					{
						auto slug = p.getProperty("slug", {}).toString();
						auto topicUrl = baseUrl.getChildURL("topic/" + slug);

						MarkdownLink forumLink({}, topicUrl.toString(false));

						numFound++;

						showStatusMessage("Found " + String(numFound) + " links");

						holder.getDatabase().addForumDiscussion({ item.url, forumLink });
					}
				}
			}
		}
	}
}

void DocUpdater::updateFromServer()
{
	showStatusMessage("Fetching hash from server");

	auto hashURL = getCacheUrl(Hash);

	setTimeoutMs(-1);
	auto content = hashURL.readEntireTextStream(false);
	setTimeoutMs(6000);

	if (threadShouldExit())
	{
		result = UserCancelled;
		return;
	}

	if (content.isEmpty())
	{
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

	if (webContentHash != localContentHash)
		downloadAndTestFile("content.dat");
	
	if (threadShouldExit())
	{
		result = UserCancelled;
		return;
	}
		

	if (webImageHash != localImageHash)
		downloadAndTestFile("images.dat");

	if (threadShouldExit())
	{
		result = UserCancelled;
		return;
	}

	localFile.replaceWithText(JSON::toString(webHash));

	addForumLinks();

	showStatusMessage("Rebuilding indexes");
	
	holder.rebuildDatabase();
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

	DatabaseCrawler::createImagesInHtmlFolder(htmlDirectory->getCurrentFile(), getHolder(), this, &getProgressCounter());
	DatabaseCrawler::createHtmlFilesInHtmlFolder(htmlDirectory->getCurrentFile(), getHolder(), this, &getProgressCounter());
}

void DocUpdater::downloadAndTestFile(const String& targetFileName)
{
	URL base("http://hise.audio/manual/");

	showStatusMessage("Downloading " + targetFileName);

	auto contentURL = getBaseURL().getChildURL("cache/" + targetFileName);

	auto realFile = holder.getCachedDocFolder().getChildFile(targetFileName);
	auto tmpFile = realFile.getSiblingFile("temp.dat");

	setTimeoutMs(-1);

	currentDownload = contentURL.downloadToFile(tmpFile, {}, this);

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

	showStatusMessage("Check file integrity");

	zstd::ZDefaultCompressor comp;

	ValueTree test;

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


void MarkdownPreview::Topbar::databaseWasRebuild()
{
	if (parent.getHolder().nothingInHere())
	{
		WeakReference<MarkdownDatabaseHolder> h(&parent.getHolder());
		Component::SafePointer<Component> tmp = this;

#if USE_BACKEND
		auto f = [h, tmp]()
		{
			if (h.get() != nullptr && tmp.getComponent() != nullptr)
			{
				PresetHandler::showMessageWindow("Setup documentation", "The documentation is not part of the HISE app but has to be downloaded separately.\nPress OK to load the documentation from the HISE doc server");

				auto n = new DocUpdater(*h, false, false);
				n->setModalBaseWindowComponent(tmp.getComponent());
			}
		};
        
        MessageManager::callAsync(f);
#endif
        
		

	}
}

void MarkdownPreview::Topbar::labelTextChanged(Label* labelThatHasChanged)
{
	if (labelThatHasChanged->getText().startsWith("/"))
	{
		MarkdownLink l(parent.getHolder().getDatabaseRootDirectory(), labelThatHasChanged->getText());
		parent.renderer.gotoLink(l);
	}
}

void MarkdownPreview::Topbar::textEditorTextChanged(TextEditor& ed)
{
	if (parent.currentSearchResults != nullptr)
	{
		parent.currentSearchResults->setSearchString(ed.getText());
	}
}

void MarkdownPreview::Topbar::editorShown(Label*, TextEditor& ed)
{
	ed.addListener(this);
	ed.addKeyListener(this);
	ed.addMouseListener(this, true);
	showPopup();
}

void MarkdownPreview::Topbar::showPopup()
{
	if (parent.currentSearchResults == nullptr)
	{
		parent.addAndMakeVisible(parent.currentSearchResults = new SearchResults(*this));

		

		auto bl = searchBar.getBounds().getBottomLeft();

		auto tl = parent.getLocalPoint(this, bl);

		parent.currentSearchResults->setSize(searchBar.getWidth(), 24);
		parent.currentSearchResults->setTopLeftPosition(tl);
		
		parent.currentSearchResults->setSearchString(searchBar.getText(true));
		parent.currentSearchResults->timerCallback();

		parent.currentSearchResults->grabKeyboardFocus();


	}
}

void MarkdownPreview::Topbar::mouseWheelMove(const MouseEvent& event, const MouseWheelDetails& wheel)
{
	jassertfalse;
}

void MarkdownPreview::Topbar::textEditorEscapeKeyPressed(TextEditor&)
{
	parent.currentSearchResults = nullptr;
}

void MarkdownPreview::Topbar::editorHidden(Label*, TextEditor& ed)
{
	ed.removeListener(this);
}

void MarkdownPreview::Topbar::resized()
{
	Colour c = Colours::white;

	tocButton.setColours(c.withAlpha(0.8f), c, c);

	lightSchemeButton.setColours(c.withAlpha(0.8f), c, c);
	selectButton.setColours(c.withAlpha(0.8f), c, c);

	homeButton.setVisible(false);

	auto ar = getLocalBounds();
	int buttonMargin = 12;
	int margin = 0;
	int height = ar.getHeight();

	if (parent.shouldDisplay(ViewOptions::Toc))
	{
		tocButton.setVisible(true);
		tocButton.setBounds(ar.removeFromLeft(height).reduced(buttonMargin));
		ar.removeFromLeft(margin);
	}
	else
		tocButton.setVisible(false);

	if (parent.shouldDisplay(ViewOptions::Edit))
	{
		refreshButton.setVisible(true);
		refreshButton.setBounds(ar.removeFromLeft(height).reduced(buttonMargin));
		ar.removeFromLeft(margin);
	}
	else
		refreshButton.setVisible(true);

	if (parent.shouldDisplay(ViewOptions::Back))
	{
		backButton.setVisible(true);
		forwardButton.setVisible(true);
		backButton.setBounds(ar.removeFromLeft(height).reduced(buttonMargin));
		forwardButton.setBounds(ar.removeFromLeft(height).reduced(buttonMargin));
		ar.removeFromLeft(margin);
	}
	else
	{
		backButton.setVisible(false);
		forwardButton.setVisible(false);
	}

	if (parent.shouldDisplay(ViewOptions::ColourScheme))
	{
		lightSchemeButton.setVisible(true);
		lightSchemeButton.setBounds(ar.removeFromLeft(height).reduced(buttonMargin));
		ar.removeFromLeft(margin);
	}
	else
		lightSchemeButton.setVisible(false);

	selectButton.setBounds(ar.removeFromLeft(height).reduced(buttonMargin));
	ar.removeFromLeft(margin);

	if (parent.shouldDisplay(ViewOptions::Edit))
	{
		editButton.setVisible(true);
		editButton.setBounds(ar.removeFromRight(height).reduced(buttonMargin));
	}
	else
	{
		editButton.setVisible(false);
	}

	if (parent.shouldDisplay(ViewOptions::Search))
	{
		auto delta = 0; //parent.toc.getWidth() - ar.getX();
		ar.removeFromLeft(delta);

		auto sBounds = ar.removeFromLeft(height).reduced(buttonMargin).toFloat();
		searchPath.scaleToFit(sBounds.getX(), sBounds.getY(), sBounds.getWidth(), sBounds.getHeight(), true);


		searchBar.setVisible(true);
		searchBar.setBounds(ar.reduced(5));
	}
	else
	{
		searchBar.setVisible(false);
		searchPath = {};
	}
}

void MarkdownPreview::MarkdownDatabaseTreeview::databaseWasRebuild()
{
	Component::SafePointer<MarkdownDatabaseTreeview> tmp(this);

	auto f = [tmp]()
	{
		if (tmp.getComponent() == nullptr)
			return;

		auto t = tmp.getComponent();

		if (t != nullptr)
		{
			t->tree.setRootItem(nullptr);
			t->rootItem = new Item(t->parent.getHolder().getDatabase().rootItem, t->parent);



			t->tree.setRootItem(t->rootItem);



			t->resized();

			if (t->rootItem->getNumSubItems() == 1)
				t->rootItem->setOpenness(TreeViewItem::Openness::opennessOpen);

		}
	};

	MessageManager::callAsync(f);
}

}
