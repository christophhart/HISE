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

Component* DebugSession::createMultiViewer()
{
	return new ProfileDataSource::ViewComponents::MultiViewer(*this);
}

void DebugSession::ProfileDataSource::ViewComponents::MultiViewer::onFlush(MultiViewer& m,
	ProfileDataSource::ProfileInfoBase::Ptr p)
{
	if(p != nullptr)
	{
		String name = "Profile " + String(m.tabButtons.size() + 1);
		m.tabButtons.add(new TabButton(name));
		m.addAndMakeVisible(m.tabButtons.getLast());

		auto v = dynamic_cast<Viewer*>(p->createComponent());

		m.currentViewers.add(v);
		m.addAndMakeVisible(v);

		if(auto mm = m.getManager())
		{
			mm->setActiveViewer(v);
		}
	}
}

void DebugSession::ProfileDataSource::ViewComponents::MultiViewer::resized()
{
	auto b = getLocalBounds();

	if(b.isEmpty())
		return;

#if 0
	if(currentViewers.size() == 1)
	{
		for(auto tb: tabButtons)
			tb->setVisible(false);
		currentViewers[0]->setVisible(true);
		currentViewers[0]->setBounds(b);
		return;
	}
#endif

	auto tabRow = b.removeFromTop(20);

	for(auto tb: tabButtons)
	{
		tb->setVisible(true);
		tb->setBounds(tabRow.removeFromLeft(tb->getWidth()));
	}
		

	int idx = 0;

	for(auto v: currentViewers)
	{
		auto visible = idx++ == currentTabIndex;
		v->setVisible(visible);

		if(visible)
			v->setBounds(b);
	}
}

DebugSession::ProfileDataSource::ViewComponents::MultiViewer::TabButton::TabButton(const String& name_):
	name(name_),
	closeButton("delete", nullptr, factory),
	f(GLOBAL_BOLD_FONT())
{
	auto w = f.getStringWidthFloat(name) + 30.0f;
	setSize(w, 20);
	addAndMakeVisible(closeButton);

			
	closeButton.onClick = [this](){ findParentComponentOfClass<MultiViewer>()->removeTab(this); };
}

void DebugSession::ProfileDataSource::ViewComponents::MultiViewer::TabButton::paint(Graphics& g)
{
	auto b = getLocalBounds().toFloat();

	b.removeFromRight(2.0f);

	g.setColour(Colours::white.withAlpha(active ? 0.1f : 0.05f));

	Path p;
	p.addRoundedRectangle(b.getX(), b.getY(), b.getWidth(), b.getHeight(), 3.0, 3.0, true, true, false, false);

	g.fillPath(p);

	g.setFont(f);
	b.removeFromLeft(5.0f);
	g.setColour(Colours::white.withAlpha(active ? 0.8f : 0.4f));
	g.drawText(name, b, Justification::left);
}

void DebugSession::ProfileDataSource::ViewComponents::MultiViewer::onNavigation(BaseWithManagerConnection* source,
                                                                                ViewItem::Ptr r, ViewItem::Ptr cr)
{
	if(auto d = dynamic_cast<Viewer*>(source))
	{
		currentTabIndex = currentViewers.indexOf(d);

		int idx = 0;
		for(auto b : tabButtons)
			b->setActive(idx++ == currentTabIndex);

		resized();
	}
}

void DebugSession::ProfileDataSource::ViewComponents::MultiViewer::setCurrentTab(TabButton* b)
{
	auto idx = tabButtons.indexOf(b);

	if(idx != currentTabIndex)
	{
		if(auto m = getManager())
			m->setActiveViewer(currentViewers[idx]);
	}
}

void DebugSession::ProfileDataSource::ViewComponents::MultiViewer::mouseDown(const MouseEvent& e)
{
    if(e.mods.isRightButtonDown())
    {
        PopupLookAndFeel plaf;
        PopupMenu m;
        m.setLookAndFeel(&plaf);
        
        m.addItem(1, "Close all tabs");
        m.addItem(2, "Close all but this");

        auto r = m.show();
        
        if(r == 1)
        {
            while(!tabButtons.isEmpty())
                removeTab(tabButtons.getFirst(), sendNotificationSync);
        }
        if(r == 2)
        {
            for(int i = 0; i < tabButtons.size(); i++)
            {
                if(!tabButtons[i]->active)
                {
                    removeTab(tabButtons[i--], sendNotificationSync);
                }
            }
        }
        
    }
}

void DebugSession::ProfileDataSource::ViewComponents::MultiViewer::removeTab(TabButton* b, NotificationType s)
{
	auto indexToRemove = tabButtons.indexOf(b);

	auto f = [indexToRemove, this]()
	{
		auto m = getManager();
		

		tabButtons.remove(indexToRemove);
		currentViewers.remove(indexToRemove);

		currentTabIndex = currentViewers.indexOf(m->currentViewer.get());

		if(currentTabIndex == -1)
		{
			currentTabIndex = currentViewers.size()-1;
			m->setActiveViewer(currentViewers.getLast());
		}

		resized();
	};

    if(s == sendNotificationAsync)
        MessageManager::callAsync(f);
    else
        f();
}

DebugSession::ProfileDataSource::ViewComponents::SingleThreadPopup::SingleThreadPopup(DebugSession& s,
	DebugInformationBase::Ptr root)
{
	setName("Profiler");

	addAndMakeVisible(manager = new Manager(s));
	manager->setPopupMode(true);
	addAndMakeVisible(viewer = new Viewer(dynamic_cast<ProfileInfoBase*>(root.get()), s, false));
	addAndMakeVisible(statistics = new StatisticsComponent(s));
	statistics->initManager();
	manager->setActiveViewer(viewer.get());
		
	viewer->setTimeDomain(viewer->originalRoot->getPreferredDomain());

	setSize(DefaultWidth, viewer->getHeight() + MultiViewerMenuBarHeight + 200);
}

void DebugSession::ProfileDataSource::ViewComponents::SingleThreadPopup::resized()
{
	auto b = getLocalBounds();
	statistics->setBounds(b.removeFromBottom(200));
	manager->setBounds(b.removeFromTop(MultiViewerMenuBarHeight));
	viewer->setBounds(b);
}
}
