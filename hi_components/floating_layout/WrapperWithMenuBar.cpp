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

namespace hise
{
using namespace juce;

Component* WrapperWithMenuBarBase::showPopup(FloatingTile* ft, Component* parent, const std::function<Component*()>& createFunc, bool show)
{
	if (!show)
	{
		ft->showComponentInRootPopup(nullptr, parent, {});
		return nullptr;
	}
	else
	{
		auto p = createFunc();
		ft->showComponentInRootPopup(p, parent, { parent->getWidth() / 2, parent->getHeight() });
		return p;
	}
}

WrapperWithMenuBarBase::WrapperWithMenuBarBase(Component* contentComponent):
	canvas(contentComponent)
{
	addAndMakeVisible(canvas);
	canvas.addZoomListener(this);
	canvas.setMaxZoomFactor(1.5f);

	canvas.contentFunction = [this](Component* c)
	{
		this->setContentComponent(c);
	};

	startTimer(100);
}

void WrapperWithMenuBarBase::setContentComponent(Component* newContent)
{
	actionButtons.clear();
	bookmarkBox = nullptr;
	rebuildAfterContentChange();
	resized();
}

void WrapperWithMenuBarBase::timerCallback()
{
	for (auto a : actionButtons)
	{
		if (!isValid())
			break;

		if (auto asB = dynamic_cast<ButtonWithStateFunction*>(a))
		{
			if (asB->hasChanged())
				a->repaint();
		}
	}
}

void WrapperWithMenuBarBase::addSpacer(int width)
{
	auto p = new Spacer(width);
	actionButtons.add(p);
	addAndMakeVisible(p);
}

void WrapperWithMenuBarBase::updateBookmarks(ValueTree, bool)
{
	StringArray sa;

	for (auto b : bookmarkUpdater.getParentTree())
	{
		sa.add(b["ID"].toString());
	}

	sa.add("Add new bookmark");
        
	auto currentIdx = bookmarkBox->getSelectedId();
	bookmarkBox->clear(dontSendNotification);
	bookmarkBox->addItemList(sa, 1);
	bookmarkBox->setSelectedId(currentIdx, dontSendNotification);
}

void WrapperWithMenuBarBase::comboBoxChanged(ComboBox* c)
{
	auto isLastEntry = c->getSelectedItemIndex() == c->getNumItems() - 1;
        
	if(isLastEntry)
	{
		auto idx = bookmarkAdded();
            
		if(idx == -1)
			c->setSelectedId(0, dontSendNotification);
		else
			c->setSelectedItemIndex(idx, dontSendNotification);
            
		return;
	}
        
	auto bm = bookmarkUpdater.getParentTree().getChildWithProperty("ID", bookmarkBox->getText());

	if (bm.isValid())
	{
		auto l = StringArray::fromTokens(bm["Value"].toString(), ";", "");
		bookmarkUpdated(l);
	}
}

void WrapperWithMenuBarBase::addBookmarkComboBox()
{
	bookmarkBox = new ComboBox();

	bookmarkBox->setLookAndFeel(&glaf);
	bookmarkBox->addListener(this);

	glaf.setDefaultColours(*bookmarkBox);

	auto cTree = getBookmarkValueTree();

	bookmarkUpdater.setCallback(cTree, valuetree::AsyncMode::Asynchronously, BIND_MEMBER_FUNCTION_2(WrapperWithMenuBarBase::updateBookmarks));
        
	updateBookmarks({}, true);
	bookmarkBox->setSize(100, 24);
	actionButtons.add(bookmarkBox);
	addAndMakeVisible(bookmarkBox);
}

void WrapperWithMenuBarBase::paint(Graphics& g)
{
	auto top = getLocalBounds().removeFromTop(MenuHeight);

	g.setColour(JUCE_LIVE_CONSTANT_OFF(Colour(0xFF333333)));
	g.fillRect(top);
	GlobalHiseLookAndFeel::drawFake3D(g, top);
}

void WrapperWithMenuBarBase::addCustomComponent(Component* c)
{
	// must be sized before passing in here
	jassert(!c->getBounds().isEmpty());

	addAndMakeVisible(c);
	actionButtons.add(c);
}

void WrapperWithMenuBarBase::setPostResizeFunction(const std::function<void(Component*)>& f)
{
	resizeFunction = f;
}

void WrapperWithMenuBarBase::resized()
{
	auto b = getLocalBounds();
	auto menuBar = b.removeFromTop(MenuHeight);

	for (auto ab : actionButtons)
	{
		ab->setTopLeftPosition(menuBar.getTopLeft());
		menuBar.removeFromLeft(ab->getWidth() + 3);
	}

	canvas.setBounds(b);

	if (resizeFunction)
		resizeFunction(canvas.getContentComponent());
}
}
