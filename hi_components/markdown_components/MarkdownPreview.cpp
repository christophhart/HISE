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
	renderer.setDatabaseHolder(&holder);
	renderer.setCreateFooter(true);

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
			f = lastLink.getMarkdownFile({});

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

	auto topBounds = ar.removeFromTop(46);

	if (toc.isVisible())
	{
		toc.setBounds(ar.removeFromLeft(toc.getPreferredWidth()));

	}

	renderer.updateCreatedComponents();


	topbar.setBounds(topBounds);

#if JUCE_IOS
    int margin = 3;
#else
    int margin = 32;
#endif

	viewport.setBounds(ar.reduced(margin));

	auto h = internalComponent.getTextHeight();

	internalComponent.setSize(jmin(1300, viewport.getWidth() - viewport.getScrollBarThickness()), h);
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

void MarkdownPreview::InternalComponent::setNewText(const String& s, const File& f)
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

		if(l.isNotEmpty())
		{
			renderer.gotoLink({ parent.rootDirectory, l });
		}

#if 0
		auto oldAnchor = renderer.getLastLink(false, true);

		if(renderer.gotoLink(e, getLocalBounds().toFloat()))
			repaint();

		
		auto newAnchor = renderer.getLastLink(false, true);

		if (oldAnchor != newAnchor)
			renderer.gotoLink(newAnchor);
#endif
	}
	
	repaint();
}

void MarkdownPreview::InternalComponent::mouseMove(const MouseEvent& event)
{
	auto link = renderer.getHyperLinkForEvent(event, getLocalBounds().toFloat());

	if (link.valid)
	{
		if (link.tooltip.isEmpty())
			setTooltip(link.url);
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
	LOAD_PATH_IF_URL("search", EditorIcons::searchIcon);
	LOAD_PATH_IF_URL("home", MainToolbarIcons::home);
	LOAD_PATH_IF_URL("toc", BackendBinaryData::ToolbarIcons::hamburgerIcon);
	LOAD_PATH_IF_URL("drag", EditorIcons::dragIcon);
	LOAD_PATH_IF_URL("select", EditorIcons::selectIcon);
	LOAD_PATH_IF_URL("sun", EditorIcons::sunIcon);
	LOAD_PATH_IF_URL("night", EditorIcons::nightIcon);
	LOAD_PATH_IF_URL("book", EditorIcons::bookIcon);
	LOAD_PATH_IF_URL("rebuild", ColumnIcons::moveIcon);
	LOAD_PATH_IF_URL("edit", OverlayIcons::penShape);
	LOAD_PATH_IF_URL("lock", OverlayIcons::lockShape);

	return p;
}

DocUpdater::DocUpdater(MarkdownDatabaseHolder& holder_, bool fastMode_, bool allowEdit) :
	MarkdownContentProcessor(holder_),
	DialogWindowWithBackgroundThread("Update documentation", false),
	holder(holder_),
	crawler(holder),
	fastMode(fastMode_),
	editingShouldBeEnabled(allowEdit)
{
	holder.addContentProcessor(this);

	if (!fastMode)
	{

		holder.addContentProcessor(&crawler);

		StringArray sa = { "Update local cached file", "Update docs from server" , "Create local HTML offline docs" };

		addComboBox("action", sa, "Action");

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


		htmlDirectory = new FilenameComponent("Target directory", {}, true, true, true, {}, {}, "Select a HTML target directory");
		htmlDirectory->setSize(400, 32);

		htmlDirectory->setEnabled(false);

		addCustomComponent(markdownRepository);

		addCustomComponent(htmlDirectory);

		crawler.setProgressCounter(&getProgressCounter());
		holder.setProgressCounter(&getProgressCounter());

		addBasicComponents(true);
	}
	else
	{
		addBasicComponents(false);
		DialogWindowWithBackgroundThread::runThread();
	}
}

void DocUpdater::run()
{
	if (fastMode)
	{
		showStatusMessage("Rebuilding Documentation Index");
		auto mc = dynamic_cast<MainController*>(&holder);
		mc->setAllowFlakyThreading(true);
		holder.setProgressCounter(&getProgressCounter());
		getHolder().setForceCachedDataUse(!editingShouldBeEnabled);

		addForumLinks();

		mc->setAllowFlakyThreading(false);
	}
	else
	{
		auto b = getComboBoxComponent("action");

		if (b->getSelectedItemIndex() == 0)
		{
			auto mc = dynamic_cast<MainController*>(&holder);

			mc->setAllowFlakyThreading(true);

			showStatusMessage("Rebuilding index");
			holder.setForceCachedDataUse(false);
			
			addForumLinks();

			showStatusMessage("Create Content cache");
			crawler.createContentTree();

			showStatusMessage("Create Image cache");
			crawler.createImageTree();

			crawler.createDataFiles(AutogeneratedDocHelpers::getCachedDocFolder(), true);

			mc->setAllowFlakyThreading(false);
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

		PresetHandler::showMessageWindow("Update finished", s, Helpers::wasOk(result) ? PresetHandler::IconType::Info : PresetHandler::IconType::Error);
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

	auto response = forumUrl.readEntireTextStream(false);

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

	URL base("http://hise.audio/manual/");
	
	auto hashURL = base.getChildURL("Hash.json");

	auto content = hashURL.readEntireTextStream(false);

	if (content.isEmpty())
	{
		result = CantResolveServer;
		return;
	}

	result = NothingUpdated;

	auto localFile = holder.getCachedDocFolder().getChildFile("Hash.json");

	auto webHash = JSON::parse(content);
	auto contentHash = JSON::parse(localFile.loadFileAsString());
	

	auto webContentHash = (int64)webHash.getProperty("content-hash", {});
	auto webImageHash = (int64)webHash.getProperty("image-hash", {});

	auto localContentHash = (int64)contentHash.getProperty("content-hash", {});
	auto localImageHash = (int64)contentHash.getProperty("image-hash", {});

	if (webContentHash != localContentHash)
		downloadAndTestFile("Content.dat");
	
	if (webImageHash != localImageHash)
		downloadAndTestFile("Images.dat");

	localFile.replaceWithText(JSON::toString(webHash));

	addForumLinks();

	showStatusMessage("Rebuilding indexes");
	
	holder.rebuildDatabase();
}

void DocUpdater::downloadAndTestFile(const String& targetFileName)
{
	URL base("http://hise.audio/manual/");

	showStatusMessage("Downloading " + targetFileName);

	auto contentURL = base.getChildURL(targetFileName);

	auto realFile = holder.getCachedDocFolder().getChildFile(targetFileName);
	auto tmpFile = realFile.getSiblingFile("temp.dat");

	currentDownload = contentURL.downloadToFile(tmpFile, {}, this);

	while (!currentDownload->isFinished())
	{
		Thread::sleep(500);
	}

	showStatusMessage("Check file integrity");

	zstd::ZDefaultCompressor comp;

	ValueTree test;

	currentDownload = nullptr;

	auto r = comp.expand(tmpFile, test);

	if (!r.wasOk() || !test.isValid())
		result = Helpers::withError(result);
	else
		tmpFile.copyFileTo(realFile);

	auto ok = tmpFile.deleteFile();
	jassert(ok);

	result |= Helpers::getIndexFromFileName(targetFileName);

}

}
