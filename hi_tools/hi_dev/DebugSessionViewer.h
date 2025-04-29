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


struct DebugSession::ProfileDataSource::ViewComponents::Viewer: public Component,
                                                                public AbstractZoomableView,
																public BaseWithManagerConnection,
                                                                public TextEditorWithAutocompleteComponent::Parent
{
	struct BreadcrumbComponent;
	struct TimelineComponent;

	static constexpr int getTimelineOffsetY(bool showTimeline)
	{
		return (showTimeline ? (LabelMargin + TopBarHeight) : 0) + BreadcrumbHeight + LabelMargin + FirstItemOffset;
	}

	Viewer(ProfileInfoBase* info, DebugSession& dh, bool multiMode);

	const bool multiMode;

	double getTotalLength() const override
	{
		return rootItem->getBounds(0.0f, showTimeline, 1.0f, 0, currentIndex).getWidth();
	}

	void applyPosition(const Rectangle<int>& screenBoundsOfTooltipClient, Rectangle<int>& t) override;

	ViewItem* getTooltipItem() const;

	String getTooltip() override;

	void refreshPeakButton();

	ProfileInfoBase::Ptr getRootInfo()
	{
		return rootItem->infoItem;
	}

	void updateAdjacentItems(const MouseEvent& e);

	ViewItem* getHoverItem(const MouseEvent& e);

	void setTimeDomain(TimeDomain newDomain);

	void toggleTypeFilter(SourceType source);

	void onTimeDomainUpdate(TimeDomain newDomain, double context)
	{
		domainContext = context;
		currentDomain = newDomain;
		resized();
		repaint();
	}

	void onRunIndexUpdate(int currentRunIndex) override;

	void mouseDown(const MouseEvent& e) override;


	void mouseDrag(const MouseEvent& e) override
	{
		handleMouseEvent(e, MouseEventType::MouseDrag);
	}

	void trimToCurrentRange();

	void combineRuns(ProfileInfoBase::Ptr rootInfo, ProfileInfoBase::Ptr ptr);

	void onNavigation(BaseWithManagerConnection* source, ViewItem::Ptr o, ViewItem::Ptr c) override;

	void setActiveViewer(bool isActive);

	StatisticsComponent* getStatistics();

	void zoomToItem(ViewItem::Ptr ptr);
	void setArrowNavigation(const ViewItem* ptr, Rectangle<float> area);

    void mouseMagnify(const MouseEvent& e, float sf) override;

	void mouseMove(const MouseEvent& e) override;
	void mouseUp(const MouseEvent& e) override;
	void mouseDoubleClick(const MouseEvent& e) override;
	void mouseWheelMove(const MouseEvent& event, const MouseWheelDetails& wheel) override;

	VoiceBitMap<32> getTypeFilter();
	UndoManager* getUndoManager() override;

	void navigateInternal(ViewItem::Ptr newDisplayRoot);
	void setRootItems(ViewItem::Ptr newOriginalRoot, ViewItem::Ptr newDisplayRoot);
	bool keyPressed(const KeyPress& k) override;
	void paint(Graphics& g) override;

	Rectangle<int> getViewportPosition() const;

	String getTextForDistance(float dragWidth) const
	{
		return Helpers::getDuration(dragWidth, currentDomain, domainContext);
	}

	void resized() override;
	void updateRowScrollbar();

	StringArray getAutocompleteItems(const Identifier& id) override
	{
		StringArray n;
		originalRoot->getNames(n);
		return n;
	}

	bool isTopLevel() const override { return true; }

	void onSearchUpdate(const String& s, bool zoomToFirst) override;

	bool onWASDMovement(WASDKeyListener::MovementType t, double delta) override;

private:

	double rowPositionDouble = 0.0;
    bool activeViewer = true;
	bool foldAll = false;
	Rectangle<int> menuBar;
	bool showTimeline = true;
	Identifier mode;
	StringArray allNames;
	DebugSession& session;

	bool navigate(ViewItem::Ptr newRoot, bool useUndoManager);

	std::map<int, std::vector<std::pair<ViewItem::Ptr, Range<double>>>> allRanges;

	TimeDomain currentDomain = TimeDomain::Absolute;
	double domainContext = 0.0;
	ViewItem* currentlyHoveredItem = nullptr;
	ViewItem* currentNavigationItem = nullptr;
	std::pair<std::pair<ViewItem::Ptr, Range<double>>, std::pair<ViewItem::Ptr, Range<double>>> adjacentItems;
	float adjacentAlpha = 0.0f;
	int adjacentDepth = -1;

	std::vector<std::pair<Rectangle<float>, String>> labels;
	int currentIndex = -1;

	ViewItem::Ptr originalRoot;
	ViewItem::Ptr rootItem;
	ScopedPointer<Component> timeline;
	ScopedPointer<Component> breadcrumbs;

	ScrollBar rowScrollBar;

	std::vector<std::pair<ViewItem::Ptr, ViewItem::Ptr>> tracks;
	
	int maxDepth = 0;
	int depth = 0;
	

	struct Factory: public PathFactory
	{
		Path createPath(const String& url) const override
		{
			Path p;
			LOAD_EPATH_IF_URL("undo", EditorIcons::backIcon);
			LOAD_EPATH_IF_URL("redo", EditorIcons::forwardIcon);
			LOAD_EPATH_IF_URL("home", MainToolbarIcons::home);
			LOAD_EPATH_IF_URL("peak", EditorIcons::peakIcon);
			LOAD_EPATH_IF_URL("favorite", EditorIcons::favoriteIcon);

			if(url == "peak")
				p.applyTransform(AffineTransform::scale(1.0f, 2.0f));

			return p;
		}
	} factory;

	friend struct StatisticsComponent;
	friend struct ItemPopup;
	friend struct MultiViewer;
	friend struct Manager;
	friend struct SingleThreadPopup;

	HiseShapeButton peakButton;

	std::map<ViewItem*, Rectangle<float>> arrowNavigations;

	JUCE_DECLARE_WEAK_REFERENCEABLE(Viewer);
};

}
