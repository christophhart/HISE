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

	internalComponent.setSize(viewport.getWidth() - viewport.getScrollBarThickness(), h);
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

}
