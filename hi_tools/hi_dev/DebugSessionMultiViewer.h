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

#pragma once

namespace hise { using namespace juce;

struct DebugSession::ProfileDataSource::ViewComponents::MultiViewer: public Component,
																	 public BaseWithManagerConnection
{
	SET_GENERIC_PANEL_ID("ProfilerViewer");

	MultiViewer(DebugSession& s):
	  session(s)
	{
		session.recordingFlushBroadcaster.addListener(*this, onFlush, false);
	}

	void paint(Graphics& g) override
	{
		g.fillAll(Colour(0xFF1D1D1D));
	}

	static void onFlush(MultiViewer& m, ProfileDataSource::ProfileInfoBase::Ptr p);

	void resized() override;

	struct TabButton: public Component
	{
		TabButton(const String& name_);

		void paint(Graphics& g) override;

		void mouseDown(const MouseEvent& e) override
		{
			findParentComponentOfClass<MultiViewer>()->setCurrentTab(this);
		}

		void resized() override
		{
			auto b = getLocalBounds();
			closeButton.setBounds(b.removeFromRight(getHeight()).reduced(2));
		}

		void setActive(bool shouldBeActive)
		{
			active = shouldBeActive;
			repaint();
		}

		bool active = false;
		Manager::Factory factory;
		Font f;

		HiseShapeButton closeButton;
		String name;
	};

    void mouseDown(const MouseEvent& e) override;
    
	void onNavigation(BaseWithManagerConnection* source, ViewItem::Ptr r, ViewItem::Ptr cr);

	void onRunIndexUpdate(int currentRunIndex) override {};

	void setCurrentTab(TabButton* b);

	void removeTab(TabButton* b, NotificationType s=sendNotificationAsync);

	int currentTabIndex = 0;
	DebugSession& session;
	OwnedArray<TabButton> tabButtons;
	OwnedArray<Viewer> currentViewers;

	JUCE_DECLARE_WEAK_REFERENCEABLE(MultiViewer);
};

struct DebugSession::ProfileDataSource::ViewComponents::SingleThreadPopup: public Component
{
	SingleThreadPopup(DebugSession& s, DebugInformationBase::Ptr root);

	void resized() override;

	ScopedPointer<Manager> manager;
	ScopedPointer<Viewer> viewer;
	ScopedPointer<StatisticsComponent> statistics;
};

}
