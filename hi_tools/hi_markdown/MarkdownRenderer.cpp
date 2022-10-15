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

float MarkdownRenderer::getHeightForWidth(float width, bool forceUpdate/*=false*/)
{
	if (width == lastWidth && !forceUpdate)
		return lastHeight;

	float height = 0.0f;

	for (auto* e : elements)
	{
		if (auto h = dynamic_cast<MarkdownParser::Headline*>(e))
		{
			h->anchorY = height;
		}

		height += e->getTopMargin();
		height += e->getHeightForWidthCached(width, forceUpdate);
	}

	lastWidth = width;
	lastHeight = height;
	firstDraw = true;

	return height;
}

void MarkdownRenderer::parse()
{
	lastWidth = -1.0f;
	firstDraw = true;

	MarkdownParser::parse();

	for (auto l : listeners)
	{
		if (l.get() != nullptr)
			l->markdownWasParsed(getParseResult());
	}
}

void MarkdownRenderer::jumpToCurrentAnchor()
{
	if (lastWidth == -1.0)
		return;

	auto thisAnchor = getLastLink().toString(MarkdownLink::AnchorWithHashtag);

	if (thisAnchor.isEmpty())
	{
		scrollToY(0.0f);
		return;
	}
	
	getHeightForWidth(lastWidth, true);

	for (auto e : elements)
	{
		if (auto headLine = dynamic_cast<Headline*>(e))
		{
			if (thisAnchor == headLine->anchorURL)
			{
				scrollToY(headLine->anchorY);
			}

		}
	}
}

juce::String MarkdownRenderer::getAnchorForY(int y) const
{
	int thisY = 0;

	Headline* lastHeadline = nullptr;

	for (auto e : elements)
	{
		if (auto h = dynamic_cast<Headline*>(e))
		{
			lastHeadline = h;
		}

		thisY += e->getTopMargin();
		thisY += (int)e->getLastHeight();

		if (y <= thisY)
			break;
	}

	if (lastHeadline != nullptr && lastHeadline != elements.getFirst())
		return lastHeadline->anchorURL;

	return {};
}


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

void MarkdownPreview::setNewText(const String& newText, const File& f, bool scrollToStart)
{
	internalComponent.setNewText(newText, f, scrollToStart);
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

	internalComponent.setSize(jmin(MarkdownParser::DefaultLineWidth, viewport.getWidth() - viewport.getScrollBarThickness()), h);
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

void MarkdownPreview::InternalComponent::setNewText(const String& s, const File&, bool scrollToStart)
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

	if(scrollToStart)
		scrollToAnchor(0.0f);

	repaint();
}

void MarkdownPreview::InternalComponent::markdownWasParsed(const Result& r)
{
	if (parent.getHolder().nothingInHere() && renderer.getCurrentText(true).isEmpty())
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

		if (renderer.getLastLink().toString(MarkdownLink::AnchorWithHashtag).isEmpty())
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
			parent.showDoc();
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

        auto markdownBounds = getLocalBounds().toFloat();
        auto root = parent.rootDirectory;
        
        renderer.gotoLinkFromMouseEvent(e, markdownBounds, root);
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

void MarkdownPreview::InternalComponent::setScaleFactor(float newScaleFactor)
{
    newScaleFactor = jlimit(0.5f, 3.0f, newScaleFactor);
    
    if(scaleFactor != newScaleFactor)
    {
        scaleFactor = newScaleFactor;
        
        auto topLine = parent.renderer.getLineNumberForY(parent.viewport.getViewPositionY());
        
        auto newWidth = jmin(parent.viewport.getWidth() - parent.viewport.getScrollBarThickness(), roundToInt((float)MarkdownParser::DefaultLineWidth * scaleFactor));

        MarkdownRenderer::ScopedScrollDisabler sds(parent.renderer);

        styleData.fontSize = 18.0f * scaleFactor;
        
        if(newWidth != getWidth())
            setSize(newWidth, getHeight());

        renderer.setStyleData(styleData);
        
        auto newY = parent.renderer.getYForLineNumber(topLine);
        parent.renderer.scrollToY(newY);
    }
}


void MarkdownPreview::InternalComponent::mouseWheelMove(const MouseEvent& event, const MouseWheelDetails& details)
{
	if (event.mods.isCommandDown())
	{
        auto thisScale = scaleFactor + (float)details.deltaY * 0.2;
        setScaleFactor(thisScale);
        
	}
	else
	{
		Component::mouseWheelMove(event, details);
	}

}

void MarkdownPreview::InternalComponent::scrollToAnchor(float v)
{
	if (renderer.disableScrolling)
		return;

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
	
	LOAD_PATH_IF_URL("rebuild", EditorIcons::moveIcon);
    
#if USE_BACKEND
	LOAD_PATH_IF_URL("toc", BackendBinaryData::ToolbarIcons::hamburgerIcon);
#endif
	LOAD_PATH_IF_URL("edit", EditorIcons::penShape);
	LOAD_PATH_IF_URL("lock", EditorIcons::lockShape);

	return p;
}



void MarkdownPreview::Topbar::databaseWasRebuild()
{
	if (parent.getHolder().nothingInHere())
	{
		WeakReference<MarkdownDatabaseHolder> h(&parent.getHolder());
		Component::SafePointer<MarkdownPreview> tmp = &parent;

		auto f = [h, tmp]()
		{
			if (h.get() != nullptr && tmp.getComponent() != nullptr)
				tmp->showDoc();
		};

		MessageManager::callAsync(f);
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

void MarkdownPreview::Topbar::mouseWheelMove(const MouseEvent&, const MouseWheelDetails&)
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

void MarkdownPreview::Topbar::buttonClicked(Button* b)
{
	if (b == &refreshButton)
	{
		parent.showDoc();
	}
	if (b == &editButton)
	{
		bool on = b->getToggleState();

		parent.enableEditing(on);

	}
	if (b == &forwardButton)
	{
		parent.renderer.navigate(false);
	}
	if (b == &backButton)
	{
		parent.renderer.navigate(true);
	}
	if (b == &tocButton)
	{
		parent.toc.setVisible(!parent.toc.isVisible());
		parent.resized();
	}
	if (b == &lightSchemeButton)
	{
		if (b->getToggleState())
			parent.internalComponent.styleData = MarkdownLayout::StyleData::createBrightStyle();
		else
			parent.internalComponent.styleData = MarkdownLayout::StyleData::createDarkStyle();

		parent.renderer.setStyleData(parent.internalComponent.styleData);

		parent.repaint();

		lightSchemeButton.refreshShape();
	}
	if (b == &selectButton)
	{
		parent.setMouseMode(b->getToggleState() ? Select : Drag);
	}
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


void MarkdownPreview::MarkdownDatabaseTreeview::scrollToLink(const MarkdownLink& l)
{
	if (l.isInvalid())
		return;

	auto root = tree.getRootItem();

	if (root == nullptr)
	{
		pendingLink = l;
		return;
	}

	bool found = false;

	for (int i = 0; i < root->getNumSubItems(); i++)
		found |= closeIfNoMatch(root->getSubItem(i), l);

	if (found)
	{
		if (auto t = dynamic_cast<Item*>(tree.getRootItem())->selectIfURLMatches(l))
		{
			t->setSelected(true, true);
			t->setOpen(true);
			tree.scrollToKeepItemVisible(t);
		}

		pendingLink = {};
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
			{
				auto actualRoot = t->rootItem->getSubItem(0);

				actualRoot->setOpen(false);
				actualRoot->setOpen(true);
			}

			auto pl = tmp->pendingLink;

			if (pl.isValid())
			{
				// This prevents the default link from being automatically opened
				if (pl.toString(MarkdownLink::Format::UrlFull) == "/")
					return;

				auto anchor = pl.toString(MarkdownLink::Format::AnchorWithoutHashtag);
				tmp.getComponent()->scrollToLink(pl);
				tmp.getComponent()->setCurrentAnchor(anchor);
				tmp.getComponent()->repaint();
			}
		}
	};

	MessageManager::callAsync(f);
}


}
