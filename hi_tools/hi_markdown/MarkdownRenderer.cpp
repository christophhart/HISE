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

MarkdownRenderer::~MarkdownRenderer()
{
	setTargetComponent(nullptr);
}

void MarkdownRenderer::draw(Graphics& g, Rectangle<float> totalArea, Rectangle<int> viewedArea) const
{
	for (auto* e : elements)
	{
		auto heightToUse = e->getHeightForWidthCached(totalArea.getWidth());
		auto topMargin = e->getTopMargin();
		totalArea.removeFromTop((float)topMargin);
		auto ar = totalArea.removeFromTop(heightToUse);

		if (firstDraw || viewedArea.isEmpty() || ar.toNearestInt().intersects(viewedArea))
			e->draw(g, ar);
	}

	firstDraw = false;
}

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

bool MarkdownRenderer::canNavigate(bool back) const
{
	if (back)
		return undoManager.canUndo();
	else
		return undoManager.canRedo();
}

void MarkdownRenderer::navigate(bool back)
{
	if (back)
		undoManager.undo();
	else
		undoManager.redo();
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

RectangleList<float> MarkdownRenderer::searchInContent(const String& searchString)
{
	RectangleList<float> positions;

	float y = 0.0f;

	for (auto e : elements)
	{
		e->searchInContent(searchString);

		y += e->getTopMargin();

		for (auto r : e->searchResults)
			positions.add(r.translated(0.0f, y));

		y += e->getLastHeight();
	}

	return positions;
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

String MarkdownRenderer::getSelectionContent() const
{
	String s;

	for (auto e : elements)
	{
		if (e->selected)
		{
			s << e->getTextToCopy() << "\n";
		}
	}

	return s;
}

void MarkdownRenderer::updateSelection(Rectangle<float> area)
{
	Range<float> yRange(area.getY(), area.getBottom());

	float y = 0.0f;

	for (auto e : elements)
	{
		float h = e->getTopMargin() + e->getLastHeight();

		e->setSelected(Range<float>(y, y + h).intersects(yRange));

		y += h;
	}
}

bool MarkdownRenderer::gotoLink(const MarkdownLink& url)
{
	undoManager.beginNewTransaction("New Link");
	return undoManager.perform(new NavigationAction(this, url));
}

bool MarkdownRenderer::gotoLinkFromMouseEvent(const MouseEvent& e, Rectangle<float> markdownBounds, const File& root)
{
	auto l = getLinkForMouseEvent(e, markdownBounds);

	if (l.isValid())
		return gotoLink(l.withRoot(root, true));
        
	return false;
}

void MarkdownRenderer::setTargetComponent(Component* newTarget)
{
	if (targetComponent == newTarget)
		return;

	if (auto existing = targetComponent.getComponent())
	{
		for (auto e : elements)
		{
			if (auto c = e->createComponent(existing->getWidth()))
				existing->removeChildComponent(c);
		}
	}

	targetComponent = newTarget;
}

void MarkdownRenderer::updateHeight()
{
	float y = currentY;

	getHeightForWidth(lastWidth, true);
		
	for (auto l : listeners)
	{
		if (l != nullptr)
			l->markdownWasParsed(getParseResult());
	}

	scrollToY(y);

}

void MarkdownRenderer::updateCreatedComponents()
{
	if (targetComponent == nullptr)
		return;

	if (targetComponent->getWidth() == 0)
		return;

	float y = (float)childArea.getY();

	auto wToUse = childArea.getWidth();

	if (wToUse == 0)
		wToUse = (int)targetComponent->getWidth();

	for (auto e : elements)
	{
		y += e->getTopMargin();

		if (auto c = e->createComponent(wToUse))
		{
			if (c->getParentComponent() == nullptr)
				targetComponent->addAndMakeVisible(c);

			jassert(c->getWidth() > 0);
			jassert(c->getHeight() > 0);

			c->setTopLeftPosition(childArea.getX(), (int)y);

			y += (float)e->getLastHeight();
		}
		else
		{
			y += e->getLastHeight();
		}
	}

		
}

bool MarkdownRenderer::navigateFromXButtons(const MouseEvent& e)
{
	if (e.mods.isX1ButtonDown())
	{
		navigate(true);
		return true;
	}
	if (e.mods.isX2ButtonDown())
	{
		navigate(false);
		return true;
	}

	return false;
}

void MarkdownRenderer::scrollToY(float y)
{
	if (disableScrolling)
		return;

	currentY = y;

	WeakReference<MarkdownRenderer> r = this;

	auto f = [r, y]()
	{
		if (r != nullptr)
		{
			for (auto l : r->listeners)
			{
				if (l.get() != nullptr)
					l->scrollToAnchor(y);
			}
		}
            
	};

	MessageManager::callAsync(f);
}


void SimpleMarkdownDisplay::InternalComp::mouseDown(const MouseEvent& e)
{
	if(e.mods.isLeftButtonDown())
	{
		auto markdownBounds = getLocalBounds().toFloat();
		parent.r.gotoLinkFromMouseEvent(e, markdownBounds, File());
	}
}

SimpleMarkdownDisplay::SimpleMarkdownDisplay():
	r("", nullptr),
	canvas(*this)
{
	vp.setViewedComponent(&canvas, false);
	addAndMakeVisible(vp);
	vp.setScrollOnDragEnabled(true);
        
	sf.addScrollBarToAnimate(vp.getVerticalScrollBar());
	vp.setScrollBarThickness(14);
}

void SimpleMarkdownDisplay::setText(const String& text)
{
	r.setNewText(text);
	r.setTargetComponent(&canvas);
		

	resized();
	r.updateCreatedComponents();
}

void SimpleMarkdownDisplay::resized()
{
	auto b = getLocalBounds();
	vp.setBounds(b);
		
	auto w = b.getWidth() - vp.getScrollBarThickness();

	totalHeight = r.getHeightForWidth(w, true);

	canvas.setSize(w, totalHeight);
	repaint();
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

void MarkdownPreview::resolversUpdated()
{
	renderer.clearResolvers();

	for (auto l : linkResolvers)
		renderer.setLinkResolver(l->clone(&renderer));

	for (auto ip : imageProviders)
		renderer.setImageProvider(ip->clone(&renderer));
}

void MarkdownPreview::addEditingMenuItems(PopupMenu& m)
{
	m.addItem(EditingMenuCommands::CopyLink, "Copy link");

	if (editingEnabled)
	{
		m.addSectionHeader("Editing Tools");
		m.addItem(EditingMenuCommands::EditCurrentPage, "Edit this page in new editor tab");
		m.addItem(EditingMenuCommands::CreateMarkdownLink, "Create markdown formatted link", true);
		m.addItem(EditingMenuCommands::RevealFile, "Show file");
		m.addItem(EditingMenuCommands::DebugExactContent, "Debug current content");
	}
}

bool MarkdownPreview::performPopupMenuForEditingIcons(int result, const MarkdownLink& linkToUse)
{
	if (result == EditingMenuCommands::EditCurrentPage)
	{
		editCurrentPage(linkToUse);
		return true;
	}
	if (result == EditingMenuCommands::CreateMarkdownLink)
	{
		SystemClipboard::copyTextToClipboard(linkToUse.toString(MarkdownLink::FormattedLinkMarkdown));
		return true;
	}
	if (result == EditingMenuCommands::CopyLink)
	{
		SystemClipboard::copyTextToClipboard(linkToUse.toString(MarkdownLink::Everything));
		return true;
	}
	if (result == EditingMenuCommands::RevealFile)
	{
		auto f = linkToUse.getDirectory({});

		if (f.isDirectory())
		{
			f.revealToUser();
			return true;
		}

		f = linkToUse.getMarkdownFile({});

		if (f.existsAsFile())
		{
			f.revealToUser();
			return true;
		}
	}
	if (result == EditingMenuCommands::DebugExactContent)
	{
		editCurrentPage({}, true);
		return true;
	}

	return false;
}

void MarkdownPreview::setNavigationShown(bool shouldBeShown)
{
	navigationShown = shouldBeShown;
}

void MarkdownPreview::mouseDown(const MouseEvent& e)
{
	currentSearchResults = nullptr;

	if (renderer.navigateFromXButtons(e))
		return;

	if (e.mods.isRightButtonDown())
	{
		PopupLookAndFeel plaf;
		PopupMenu m;
		m.setLookAndFeel(&plaf);

		addEditingMenuItems(m);

		int result = m.show();

		if (performPopupMenuForEditingIcons(result, renderer.getLastLink()))
			return;
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

void MarkdownPreview::setMouseMode(MouseMode newMode)
{
	if (newMode == Drag)
	{
		viewport.setScrollOnDragEnabled(true);
		internalComponent.enableSelect = false;
	}
	else
	{
		viewport.setScrollOnDragEnabled(false);
		internalComponent.enableSelect = true;
	}
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

	LOAD_EPATH_IF_URL("back", EditorIcons::backIcon);
	LOAD_EPATH_IF_URL("forward", EditorIcons::forwardIcon);
	LOAD_EPATH_IF_URL("search", EditorIcons::searchIcon2);
	LOAD_EPATH_IF_URL("home", MainToolbarIcons::home);
	LOAD_EPATH_IF_URL("drag", EditorIcons::dragIcon);
	LOAD_EPATH_IF_URL("select", EditorIcons::selectIcon);
	LOAD_EPATH_IF_URL("sun", EditorIcons::sunIcon);
	LOAD_EPATH_IF_URL("night", EditorIcons::nightIcon);
	LOAD_EPATH_IF_URL("book", EditorIcons::bookIcon);
	LOAD_EPATH_IF_URL("rebuild", EditorIcons::moveIcon);
    
#if USE_BACKEND
	LOAD_EPATH_IF_URL("toc", BackendBinaryData::ToolbarIcons::hamburgerIcon);
#endif
	LOAD_EPATH_IF_URL("edit", EditorIcons::penShape);
	LOAD_EPATH_IF_URL("lock", EditorIcons::lockShape);

	return p;
}

MarkdownPreview::Topbar::SearchResults::ItemComponent::ItemComponent(MarkdownDataBase::Item i,
	const MarkdownLayout::StyleData& l):
	item(i),
	p(i.description),
	style(l)
{
	p.parse();
	setInterceptsMouseClicks(true, true);
}

void MarkdownPreview::Topbar::SearchResults::ItemComponent::mouseEnter(const MouseEvent& mouseEvent)
{
	hover = true;
	setMouseCursor(MouseCursor(MouseCursor::PointingHandCursor));
	repaint();
}

void MarkdownPreview::Topbar::SearchResults::ItemComponent::mouseExit(const MouseEvent& mouseEvent)
{
	hover = false;
	setMouseCursor(MouseCursor(MouseCursor::NormalCursor));
	repaint();
}

void MarkdownPreview::Topbar::SearchResults::ItemComponent::mouseDown(const MouseEvent& e)
{
	down = true;
	repaint();

	if (e.mods.isRightButtonDown())
	{
		PopupLookAndFeel plaf;
		PopupMenu m;
		m.setLookAndFeel(&plaf);

		auto mp = findParentComponentOfClass<MarkdownPreview>();
		mp->addEditingMenuItems(m);

		int result = m.show();

		if (mp->performPopupMenuForEditingIcons(result, item.url))
			return;
	}
}

void MarkdownPreview::Topbar::SearchResults::ItemComponent::gotoLink()
{
	if (auto mp = findParentComponentOfClass<MarkdownPreview>())
	{
		auto& r = mp->renderer;

		r.gotoLink(item.url.withRoot(mp->rootDirectory, true));

		auto f2 = [mp]()
		{
			mp->currentSearchResults = nullptr;
			mp->topbar.searchBar.setText("", dontSendNotification);
		};

		MessageManager::callAsync(f2);
	}
}

void MarkdownPreview::Topbar::SearchResults::ItemComponent::mouseUp(const MouseEvent& e)
{
	down = false;
	repaint();

	if (!e.mouseWasDraggedSinceMouseDown())
		gotoLink();
}

void MarkdownPreview::Topbar::SearchResults::ItemComponent::paint(Graphics& g)
{
	g.fillAll(Colours::grey.withAlpha(down ? 0.6f : (hover ? 0.3f : 0.1f)));

	g.setColour(item.c);

	g.fillRect(0.0f, 0.0f, 3.0f, (float)getHeight());

	auto ar = getLocalBounds();

	auto f = GLOBAL_BOLD_FONT();

	g.setColour(Colours::black.withAlpha(0.1f));
	g.fillRect(kBounds);

	g.setFont(f);
	g.setColour(Colours::white);

	ar.removeFromLeft(kBounds.getWidth());

	g.drawText(item.keywords[0], kBounds.toFloat(), Justification::centred);

	if (!starBounds.isEmpty())
	{
		ar.removeFromLeft(starBounds.getWidth());

		g.setColour(item.c);

		Path starPath;
		starPath.addStar(starBounds.toFloat().getCentre(), 5, 5.0f, 10.0f);
		g.fillPath(starPath);
	}



	p.draw(g, ar.toFloat().reduced(5.0f).translated(0.0f, -5.0f));

	if (isFuzzyMatch)
		g.fillAll(Colours::grey.withAlpha(0.3f));
}

int MarkdownPreview::Topbar::SearchResults::ItemComponent::calculateHeight(int width)
{
	kBounds = { 0, 0, GLOBAL_BOLD_FONT().getStringWidth(item.keywords[0]) + 20, 0 };

	starBounds = {};


	if (height == 0)
	{
		height = (int)p.getHeightForWidth((float)(width - 10.0f - kBounds.getWidth() - starBounds.getWidth()));
	}

	kBounds.setHeight(height);
	starBounds.setHeight(height);

	return height;
}

MarkdownPreview::Topbar::SearchResults::SearchResults(Topbar& parent_):
	parent(parent_),
	nextButton("Forward", this, factory),
	prevButton("Back", this, factory),
	shadower(DropShadow(Colours::black.withAlpha(0.5f), 10, {}))
{
	addAndMakeVisible(nextButton);
	addAndMakeVisible(prevButton);
	addAndMakeVisible(textSearchResults);
	textSearchResults.setEditable(false);
	textSearchResults.setColour(Label::backgroundColourId, Colours::red.withSaturation(0.3f));
	textSearchResults.setFont(parent.parent.internalComponent.styleData.getFont());
	addAndMakeVisible(viewport);
	viewport.setViewedComponent(&content, false);
	viewport.setScrollOnDragEnabled(true);
	shadower.setOwner(this);
}

void MarkdownPreview::Topbar::SearchResults::buttonClicked(Button* b)
{
	if (b == &nextButton)
	{
		currentIndex++;

		if (currentIndex >= currentSearchResultPositions.getNumRectangles())
			currentIndex = 0;


	}
	if (b == &prevButton)
	{
		currentIndex--;

		if (currentIndex == -1)
			currentIndex = currentSearchResultPositions.getNumRectangles() - 1;
	}

	setSize(getWidth(), 32);



	parent.parent.internalComponent.scrollToSearchResult(currentSearchResultPositions.getRectangle(currentIndex));

	refreshTextResultLabel();
}

void MarkdownPreview::Topbar::SearchResults::resized()
{
	auto ar = getLocalBounds();

	if (currentSearchResultPositions.isEmpty())
	{
		nextButton.setVisible(false);
		prevButton.setVisible(false);
		textSearchResults.setVisible(false);
	}
	else
	{
		nextButton.setVisible(true);
		prevButton.setVisible(true);
		textSearchResults.setVisible(true);

		auto top = ar.removeFromTop(32);

		nextButton.setBounds(top.removeFromRight(32).reduced(6));
		prevButton.setBounds(top.removeFromRight(32).reduced(6));
		textSearchResults.setBounds(top);
	}

	viewport.setBounds(ar);
}

void MarkdownPreview::Topbar::SearchResults::refreshTextResultLabel()
{
	if (!currentSearchResultPositions.isEmpty())
	{
		String s;

		s << "Search in current page:" << String(currentIndex + 1) << "/" << String(currentSearchResultPositions.getNumRectangles());

		textSearchResults.setText(s, dontSendNotification);
	}
	else
		textSearchResults.setText("No matches", dontSendNotification);
}

void MarkdownPreview::Topbar::SearchResults::timerCallback()
{
	currentSearchResultPositions = parent.parent.renderer.searchInContent(searchString);

	refreshTextResultLabel();

	parent.parent.repaint();

	int textSearchOffset = currentSearchResultPositions.isEmpty() ? 0 : 32;

	rebuildItems();

	if (viewport.getViewedComponent()->getHeight() > 350)
	{
		setSize(getWidth(), 350 + textSearchOffset);
	}
	else
	{
		setSize(getWidth(), viewport.getViewedComponent()->getHeight() + textSearchOffset);
	}

	stopTimer();
}

void MarkdownPreview::Topbar::SearchResults::gotoSelection()
{
	if (currentSelection != nullptr)
	{
		currentSelection->gotoLink();
	}
}

void MarkdownPreview::Topbar::SearchResults::selectNextItem(bool inc)
{
	if (currentSelection == nullptr)
	{
		itemIndex = 0;
	}
	else
	{
		if (inc)
		{
			itemIndex++;

			if (itemIndex >= displayedItems.size())
				itemIndex = 0;
		}
		else
		{
			itemIndex--;

			if (itemIndex < 0)
				itemIndex = displayedItems.size();
		}
	}

	if ((currentSelection = displayedItems[itemIndex]))
	{
		for (auto s : displayedItems)
		{
			s->hover = s == currentSelection.get();
			s->repaint();
		}

		auto visibleArea = viewport.getViewArea();

		if (!visibleArea.contains(currentSelection->getPosition()))
		{
			if (currentSelection->getY() > visibleArea.getBottom())
			{
				auto y = currentSelection->getBottom() - visibleArea.getHeight();

				viewport.setViewPosition(0, y);
			}
			else
			{
				viewport.setViewPosition(0, currentSelection->getY());
			}
		}
	}
}

void MarkdownPreview::Topbar::SearchResults::rebuildItems()
{
	if (parent.database == nullptr)
		return;

	if (searchString.isEmpty())
	{
		displayedItems.clear();
		exactMatches.clear();
		fuzzyMatches.clear();

		content.setSize(viewport.getMaximumVisibleWidth(), 20);
		return;
	}


	auto allItems = parent.database->getFlatList();

	if (searchString.startsWith("/"))
	{
		displayedItems.clear();
		exactMatches.clear();
		fuzzyMatches.clear();

		MarkdownLink linkURL = { parent.parent.rootDirectory, searchString };

		MarkdownDataBase::Item linkItem;

		for (auto item : allItems)
		{
			if (item.url == linkURL)
			{
				linkItem = item;
				break;
			}
		}

		if (linkItem)
		{
			ScopedPointer<ItemComponent> newItem(new ItemComponent(linkItem, parent.parent.internalComponent.styleData));

			displayedItems.add(newItem);
			exactMatches.add(newItem);
			content.addAndMakeVisible(newItem.release());
		}
	}
	else
	{
		MarkdownDataBase::Item::PrioritySorter sorter(searchString);

		auto sorted = sorter.sortItems(allItems);

		displayedItems.clear();
		exactMatches.clear();
		fuzzyMatches.clear();

		for (const auto& item : sorted)
		{
			int matchLevel = item.fits(searchString);

			if (matchLevel > 0)
			{
				ScopedPointer<ItemComponent> newItem(new ItemComponent(item, parent.parent.internalComponent.styleData));

				if (matchLevel == 1)
				{
					if (exactMatches.size() < 50)
					{
						content.addAndMakeVisible(newItem);
						exactMatches.add(newItem.release());
					}
				}
				else
				{
					if (fuzzyMatches.size() < 10)
					{
						content.addAndMakeVisible(newItem);
						newItem->isFuzzyMatch = true;
						fuzzyMatches.add(newItem.release());
					}
				}

			}
		}


	}

	for (auto i : exactMatches)
		displayedItems.add(i);

	for (auto i : fuzzyMatches)
		displayedItems.add(i);

	content.setSize(viewport.getMaximumVisibleWidth(), 20);

	int y = 0;

	for (auto d : displayedItems)
	{
		auto h = d->calculateHeight(content.getWidth());

		d->setBounds(0, y, content.getWidth(), h);
		y += h;

		if (h == 0)
			continue;

		y += 2;

	}

	content.setSize(content.getWidth(), y);
}

void MarkdownPreview::Topbar::SearchResults::setSearchString(const String& s)
{
	searchString = s;

	startTimer(200);
	itemIndex = 0;
}

void MarkdownPreview::Topbar::SearchResults::paint(Graphics& g)
{
	g.fillAll(Colour(0xFF333333));
	g.fillAll(Colours::black.withAlpha(0.1f));
}


MarkdownPreview::Topbar::Topbar(MarkdownPreview& parent_):
	parent(parent_),
	tocButton("TOC", this, factory),
	homeButton("Home", this, factory),
	backButton("Back", this, factory),
	forwardButton("Forward", this, factory),
	searchPath(factory.createPath("Search")),
	lightSchemeButton("Sun", this, factory, "Night"),
	selectButton("Select", this, factory, "Drag"),
	refreshButton("Rebuild", this, factory),
	editButton("Edit", this, factory, "Lock")
{
	parent.getHolder().addDatabaseListener(this);
	selectButton.setToggleModeWithColourChange(true);
	editButton.setToggleModeWithColourChange(true);

	addAndMakeVisible(homeButton);
	addAndMakeVisible(tocButton);
	addAndMakeVisible(backButton);
	addAndMakeVisible(forwardButton);
	addAndMakeVisible(lightSchemeButton);
	addAndMakeVisible(searchBar);
	addAndMakeVisible(selectButton);
	addAndMakeVisible(editButton);
	addAndMakeVisible(refreshButton);
	lightSchemeButton.setClickingTogglesState(true);

	const auto& s = parent.internalComponent.styleData;

	searchBar.setColour(Label::backgroundColourId, Colour(0x22000000));
	searchBar.setFont(s.getFont());
	searchBar.setEditable(true);
	searchBar.setColour(Label::textColourId, Colours::white);
	searchBar.setColour(Label::ColourIds::textWhenEditingColourId, Colours::white);
	searchBar.setColour(CaretComponent::ColourIds::caretColourId, Colours::white);
	searchBar.setColour(TextEditor::ColourIds::highlightColourId, Colour(SIGNAL_COLOUR));
	searchBar.setColour(TextEditor::ColourIds::highlightedTextColourId, Colours::black);
	searchBar.setColour(TextEditor::ColourIds::focusedOutlineColourId, Colour(SIGNAL_COLOUR));
	searchBar.addListener(this);
	databaseWasRebuild();
}

MarkdownPreview::Topbar::~Topbar()
{
	parent.getHolder().removeDatabaseListener(this);
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

bool MarkdownPreview::Topbar::keyPressed(const KeyPress& key, Component*)
{
	if (key == KeyPress('f') && key.getModifiers().isCommandDown())
	{
		showPopup();
		return true;
	}
	if (key == KeyPress::upKey)
	{
		if (parent.currentSearchResults != nullptr)
			parent.currentSearchResults->selectNextItem(false);

		return true;
	}
	else if (key == KeyPress::downKey)
	{
		if (parent.currentSearchResults != nullptr)
			parent.currentSearchResults->selectNextItem(true);

		return true;
	}
	else if (key == KeyPress::returnKey)
	{
		if (searchBar.getText(true).startsWith("/"))
		{

			parent.renderer.gotoLink({ parent.rootDirectory, searchBar.getText(true) });
			searchBar.hideEditor(false);
			searchBar.setText("", dontSendNotification);
			parent.currentSearchResults = nullptr;
			return true;
		}

		if (parent.currentSearchResults != nullptr)
			parent.currentSearchResults->gotoSelection();

		return true;
	}
	else if (key == KeyPress::tabKey)
	{

		if (parent.currentSearchResults != nullptr)
			parent.currentSearchResults->nextButton.triggerClick();

		return true;
	}

	return false;

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

void MarkdownPreview::Topbar::paint(Graphics& g)
{
	g.fillAll(Colour(0xFF444444));
	//g.fillAll(Colours::white.withAlpha(0.05f));
	g.setColour(Colours::white.withAlpha(0.7f));
	g.fillPath(searchPath);
}

MarkdownPreview::MarkdownDatabaseTreeview::Item::Item(MarkdownDataBase::Item item_, MarkdownPreview& previewParent_):
	TreeViewItem(),
	item(item_),
	previewParent(previewParent_)
{
	previewParent_.toc.tree.addKeyListener(this);
}

MarkdownPreview::MarkdownDatabaseTreeview::Item::~Item()
{
	previewParent.toc.tree.removeKeyListener(this);
}

bool MarkdownPreview::MarkdownDatabaseTreeview::Item::keyPressed(const KeyPress& key, Component*)
{
	if (key.getKeyCode() == KeyPress::returnKey)
	{
		gotoLink();
		return true;
	}

	return false;
}

bool MarkdownPreview::MarkdownDatabaseTreeview::Item::mightContainSubItems()
{ return item.hasChildren(); }

String MarkdownPreview::MarkdownDatabaseTreeview::Item::getUniqueName() const
{ return item.url.toString(MarkdownLink::UrlFull); }

void MarkdownPreview::MarkdownDatabaseTreeview::Item::itemOpennessChanged(bool isNowOpen)
{
	if (item.isAlwaysOpen && !isNowOpen)
		return;

	clearSubItems();

	if (isNowOpen)
	{
		for (auto c : item)
		{
			if (c.tocString.isEmpty())
				continue;

			auto i = new Item(c, previewParent);

			addSubItem(i);

			auto currentLink = previewParent.renderer.getLastLink();

			const bool open = c.isAlwaysOpen || currentLink.isChildOf(c.url);

			if (open)
				i->setOpen(true);


		}

	}

	//previewParent.resized();
}

MarkdownParser* MarkdownPreview::MarkdownDatabaseTreeview::Item::getCurrentParser()
{
	return &previewParent.renderer;
}

MarkdownPreview::MarkdownDatabaseTreeview::Item* MarkdownPreview::MarkdownDatabaseTreeview::Item::selectIfURLMatches(
	const MarkdownLink& url)
{
	if (item.url == url)
	{
		return this;
	}

	for (int i = 0; i < getNumSubItems(); i++)
	{
		if (auto it = dynamic_cast<Item*>(getSubItem(i))->selectIfURLMatches(url))
			return it;
	}

	return nullptr;
}

void MarkdownPreview::MarkdownDatabaseTreeview::Item::gotoLink()
{
	if (auto p = getCurrentParser())
	{
		previewParent.currentSearchResults = nullptr;

		previewParent.renderer.gotoLink(item.url.withRoot(previewParent.rootDirectory, true));

#if 0
					auto link = item.url.upToFirstOccurrenceOf("#", false, false);

					if (p->getLastLink(true, false) != link)
						p->gotoLink(item.url);

					auto anchor = item.url.fromFirstOccurrenceOf("#", true, false);

					if (anchor.isNotEmpty())
					{
						auto mp = &previewParent;

						auto it = this;

						auto f2 = [mp, anchor, it]()
						{
							if (anchor.isNotEmpty())
								mp->renderer.gotoLink(anchor);

							it->setSelected(true, true);
						};

						MessageManager::callAsync(f2);
					}
#endif
	}
}

void MarkdownPreview::MarkdownDatabaseTreeview::Item::itemClicked(const MouseEvent& e)
{
	if (e.mods.isRightButtonDown())
	{
		PopupLookAndFeel plaf;
		PopupMenu m;
		m.setLookAndFeel(&plaf);

		previewParent.addEditingMenuItems(m);

		int result = m.show();

		if (previewParent.performPopupMenuForEditingIcons(result, item.url))
			return;
	}
	else
	{
		setOpen(true);
		gotoLink();
	}


}

bool MarkdownPreview::MarkdownDatabaseTreeview::Item::canBeSelected() const
{
	return true;
}

int MarkdownPreview::MarkdownDatabaseTreeview::Item::getItemHeight() const
{
	return 26;
}

int MarkdownPreview::MarkdownDatabaseTreeview::Item::getItemWidth() const
{
	auto intendation = getItemPosition(false).getX();

	const auto& s = previewParent.internalComponent.styleData;
	auto f = s.getBoldFont().withHeight(16.0f);

	int thisWidth = intendation + f.getStringWidth(item.tocString) + 30;

	int maxWidth = thisWidth;

	for (int i = 0; i < getNumSubItems(); i++)
	{
		maxWidth = jmax<int>(maxWidth, getSubItem(i)->getItemWidth());
	}

	return maxWidth;
}

void MarkdownPreview::MarkdownDatabaseTreeview::Item::paintItem(Graphics& g, int width, int height)
{



	Rectangle<float> area({ 0.0f, 0.0f, (float)width, (float)height });



	if (isSelected())
	{
		g.setColour(Colours::white.withAlpha(0.3f));
		g.fillRoundedRectangle(area, 2.0f);
	}

	auto r = area.removeFromLeft(3.0f).reduced(0.0f, 1.0f);

	area.removeFromLeft(5.0f);


	const auto& s = previewParent.internalComponent.styleData;

	g.setColour(item.c);
	g.fillRect(r);




	g.setColour(Colours::white.withAlpha(0.8f));

	auto f = s.getBoldFont().withHeight(16.0f);

	g.setFont(f);

	g.drawText(item.tocString, area, Justification::centredLeft);
}

MarkdownPreview::MarkdownDatabaseTreeview::MarkdownDatabaseTreeview(MarkdownPreview& parent_):
	parent(parent_)
{
	parent.getHolder().addDatabaseListener(this);
	addAndMakeVisible(tree);

	setBgColour(bgColour);
	tree.setColour(TreeView::ColourIds::selectedItemBackgroundColourId, Colours::transparentBlack);
	tree.setColour(TreeView::ColourIds::linesColourId, Colours::red);
	tree.setRootItemVisible(false);

	tree.getViewport()->setScrollBarsShown(true, false);
	databaseWasRebuild();
}

void MarkdownPreview::MarkdownDatabaseTreeview::setBgColour(Colour c)
{
	bgColour = c;
	tree.setColour(TreeView::ColourIds::backgroundColourId, bgColour);
}

MarkdownPreview::MarkdownDatabaseTreeview::~MarkdownDatabaseTreeview()
{
	parent.getHolder().removeDatabaseListener(this);

	tree.setRootItem(nullptr);
	rootItem = nullptr;
}

void MarkdownPreview::MarkdownDatabaseTreeview::openAll(TreeViewItem* item)
{
	item->setOpen(true);

	for (int i = 0; i < item->getNumSubItems(); i++)
	{
		openAll(item->getSubItem(i));
	}
}

void MarkdownPreview::MarkdownDatabaseTreeview::closeAllExcept(TreeViewItem* item, Array<TreeViewItem*> path)
{
	if (path.contains(item))
		return;

	item->setOpen(false);
}

bool MarkdownPreview::MarkdownDatabaseTreeview::closeIfNoMatch(TreeViewItem* item, const MarkdownLink& id)
{
	if (dynamic_cast<Item*>(item)->item.url == id)
		return true;

	item->setOpen(true);

	bool found = false;

	for (int i = 0; i < item->getNumSubItems(); i++)
	{
		found |= closeIfNoMatch(item->getSubItem(i), id);
	}

	if (!found)
		item->setOpen(false);

	return found;
}

void MarkdownPreview::MarkdownDatabaseTreeview::setCurrentAnchor(const String& s)
{
	if (tree.getRootItem() == nullptr)
		return;

	auto nl = parent.renderer.getLastLink();

	if (auto t = dynamic_cast<Item*>(tree.getRootItem())->selectIfURLMatches(nl.withAnchor(s)))
	{
		t->setSelected(true, true);
		tree.scrollToKeepItemVisible(t);
	}
}

int MarkdownPreview::MarkdownDatabaseTreeview::getPreferredWidth() const
{
	if (fixWidth != -1)
		return fixWidth;

	if (rootItem == nullptr)
		return 300;

	return jmax(300, tree.getRootItem()->getItemWidth());
}

void MarkdownPreview::MarkdownDatabaseTreeview::paint(Graphics& g)
{
	g.fillAll(bgColour);

}

void MarkdownPreview::MarkdownDatabaseTreeview::resized()
{
	tree.setBounds(getLocalBounds());
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
