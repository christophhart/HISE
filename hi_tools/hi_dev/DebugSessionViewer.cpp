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

struct DebugSession::ProfileDataSource::ViewComponents::Viewer::BreadcrumbComponent: public Component
{
	BreadcrumbComponent(Viewer& parent_):
	  parent(parent_)
	{
		updateBreadcrumbs();
	}

	struct Breadcrumb: public Component
	{
		Breadcrumb(Viewer& v, ViewItem::Ptr i):
		  parent(v),
		  item(i),
		  f(GLOBAL_BOLD_FONT())
		{
			setRepaintsOnMouseActivity(true);
			bestWidth = f.getStringWidthFloat(item->name) + 20.0f;
			setMouseCursor(MouseCursor::PointingHandCursor);
		}

		void mouseUp(const MouseEvent& e) override
		{
			if(auto m = parent.getManager())
				m->navigate(&parent, item, true);
		}

		void paint(Graphics& g) override
		{
			float alpha = 0.6f;

			if(isMouseOver(true))
				alpha += 0.2f;

			if(isMouseButtonDown(true))
				alpha += 0.2f;

			if(parent.rootItem == item)
				alpha = 0.8f;

			g.setColour(Colours::white.withAlpha(alpha));
			g.setFont(item == parent.rootItem ? f : GLOBAL_FONT());
			g.drawText(item->name, getLocalBounds().toFloat(), Justification::centred);
		}

		float bestWidth;
		Font f;
		Viewer& parent;
		ViewItem::Ptr item;
	};

	void updateBreadcrumbs()
	{
		breadcrumbs.clear();
		auto p = parent.rootItem.get();
		Array<Breadcrumb*> reversedBreadcrumbs;

		while(p != nullptr)
		{
			auto bc = new Breadcrumb(parent, p);
			reversedBreadcrumbs.add(bc);
			p = p->parent;
		}

		for(int i = reversedBreadcrumbs.size() - 1; i >= 0; i--)
		{
			auto bc = reversedBreadcrumbs[i];
			breadcrumbs.add(bc);
			addAndMakeVisible(bc);
		}

		resized();
	}

	void paint(Graphics& g) override
	{
		if(findParentComponentOfClass<MultiViewer>() != nullptr)
			g.fillAll(Colours::white.withAlpha(JUCE_LIVE_CONSTANT_OFF(0.1f)));

		g.setColour(Colours::white.withAlpha(0.2f));

		for(auto q: quotes)
			g.drawText(">", q, Justification::centred);
	}

	void resized() override
	{
		auto b = getLocalBounds();

		quotes.clear();

		for(auto bc: breadcrumbs)
		{
			bc->setBounds(b.removeFromLeft(bc->bestWidth));
			quotes.add(b.removeFromLeft(16).toFloat());
		}

		quotes.removeLast();

		repaint();
	}

	Array<Rectangle<float>> quotes;
	Viewer& parent;
	OwnedArray<Breadcrumb> breadcrumbs;
};

struct DebugSession::ProfileDataSource::ViewComponents::Viewer::TimelineComponent: public Component
{
	TimelineComponent(Viewer& parent_):
	  parent(parent_)
	{}

	void rebuild()
	{
		dataPoints.clear();
		graph.clear();

		numRuns = 0;
		float index = 0.0f;

		auto x = 5.0f;
		auto w = (float)getWidth() - 10.0f;
		auto y = 5.0f;
		auto h = (float)getHeight() - 15.0f;
		auto num = (float)parent.rootItem->runs.size();
		auto deltaX = w / num;

		auto maxLength = 0.0;

		auto minLength = 9000000000.0;

		for(auto r: parent.rootItem->runs)
		{
			maxLength = jmax(maxLength, r.length);
			minLength = jmin(minLength, r.length);
		}

		for(auto r: parent.rootItem->runs)
		{
			auto normLength = ((r.length - minLength) / jmax(0.001, (maxLength - minLength)));
			auto tx = x + index * deltaX;
			auto ty = y + (1.0 - normLength) * h;
			auto w = 5.0f;
			auto h = 5.0f;
				
			dataPoints.addWithoutMerging({tx, (float)ty, w, h});
			index += 1.0f;
			numRuns++;
		}

		graph.startNewSubPath(dataPoints.getRectangle(0).getCentre());

		for(int i = 1; i < dataPoints.getNumRectangles(); i++)
			graph.lineTo(dataPoints.getRectangle(i).getCentre());

		Path ns;

		selection.clear();

		if(parent.currentIndex == -1)
			ns.addRoundedRectangle(getLocalBounds().toFloat().reduced(2.0f), 3.0f);
		else
		{
			auto d = dataPoints.getRectangle(parent.currentIndex);

			auto x = d.getX() - 3.0f;
			auto y = 2.0f;
			auto w = d.getWidth() + 6.0f;
			auto h = getHeight() - 4.0f;
				
			ns.addRoundedRectangle(x, y, w, h, 3.0f);
		}

		float p[2] = { 2.0f, 1.0f };
		PathStrokeType(2.0f).createDashedStroke(selection, ns, p, 2);
			
		repaint();
		parent.repaint();
	}

	void mouseDown(const MouseEvent& e) override
	{
		auto mx = (float)e.getPosition().x;
		auto w = (float)(getWidth() - 10.0f) / (float)dataPoints.getNumRectangles();
		auto idx = -1;

		for(int i = 0; i < dataPoints.getNumRectangles(); i++)
		{
			auto x = dataPoints.getRectangle(i).getCentreX();

			if(std::abs(x - mx) < w * 0.5f)
			{
				idx = i;
				break;
			}
		}

		auto m = parent.getManager();

		auto currentRunIndex = m->runIndex;

		if(currentRunIndex == idx)
			currentRunIndex = -1;
		else
			currentRunIndex = idx;

		m->setCurrentRunIndex(currentRunIndex);

		
		rebuild();
	}
	void resized() override
	{
		rebuild();
	}

	void paint(Graphics& g) override
	{
		g.setColour(Colours::white.withAlpha(0.05f));
		g.fillRect(getLocalBounds().toFloat().reduced(5.0f));
		g.setColour(Colours::white.withAlpha(0.4f));
		g.fillRectList(dataPoints);
		g.strokePath(graph, PathStrokeType(1.0f));

		g.setColour(Colours::white.withAlpha(0.6f));
		g.fillPath(selection);
	}

	Viewer& parent;
	Path selection;
	Path graph;
	int numRuns = 0;
	RectangleList<float> dataPoints;
};

DebugSession::ProfileDataSource::ViewComponents::Viewer::Viewer(ProfileInfoBase* info, DebugSession& dh, bool multiMode_):
	timeline(new TimelineComponent(*this)),
	breadcrumbs(new BreadcrumbComponent(*this)),
	peakButton("peak", nullptr, factory),
	session(dh),
	multiMode(multiMode_),
	rowScrollBar(true) 
{
	setOpaque(true);
	setName("Profiler Timeline");

	setAsParentComponent(this);

	addAndMakeVisible(breadcrumbs);
	addAndMakeVisible(timeline);

	addAndMakeVisible(peakButton);
	addAndMakeVisible(rowScrollBar);

	

	peakButton.onClick = BIND_MEMBER_FUNCTION_0(Viewer::refreshPeakButton);
	peakButton.setToggleModeWithColourChange(true);
	peakButton.setTooltip("Toggle between the run with the longest duration and the averaged runtime");

	auto r = ViewItem::createFromProfileInfo(info);

	getFader().addScrollBarToAnimate(rowScrollBar);
	rowScrollBar.setSingleStepSize(1.0);
	rowScrollBar.addListener(this);
	setRootItems(r, r);
	setSize(DefaultWidth, getTimelineOffsetY(showTimeline) + jmin(12, maxDepth) * (ItemHeight + ItemMargin) + ScrollbarHeight + MenuBarHeight);
}


void DebugSession::ProfileDataSource::ViewComponents::Viewer::applyPosition(
	const Rectangle<int>& screenBoundsOfTooltipClient, Rectangle<int>& t)
{
	if(auto tt = getTooltipItem())
	{
		auto tr = getZoomTranspose();
		auto s = getZoomScale();
		auto tBounds = tt->getBounds(tr, showTimeline, s, (int)rowScrollBar.getCurrentRangeStart(), currentIndex);

		if(tBounds.getWidth() > t.getWidth())
			t.setSize(0, 0);

		auto p = screenBoundsOfTooltipClient.getPosition() + tBounds.getTopRight().toInt();
			
		t = t.withPosition(p.translated(4, 0));
	}
}

DebugSession::ProfileDataSource::ViewComponents::ViewItem* DebugSession::ProfileDataSource::ViewComponents::Viewer::
getTooltipItem() const
{
	if(currentlyHoveredItem != nullptr)
		return currentlyHoveredItem;

	if(adjacentDepth != -1)
	{
		return adjacentAlpha > 0.5f ? adjacentItems.second.first.get() : adjacentItems.first.first.get();
	}

	return nullptr;
}

String DebugSession::ProfileDataSource::ViewComponents::Viewer::getTooltip()
{
	if(auto tt = getTooltipItem())
	{
		return tt->name + " - " + tt->getDuration(currentIndex, showTimeline, currentDomain, domainContext);
	}
	else
		return "";
}

void DebugSession::ProfileDataSource::ViewComponents::Viewer::refreshPeakButton()
{
	if(originalRoot->mode == ProfileIds::multithread)
	{
		currentIndex = 0;
		return;
	}

	auto thisIndex = currentIndex;

	if(peakButton.getToggleState())
	{
		double maxLength = 0.0;
		int idx = 0;
		for(const auto& r: rootItem->runs)
		{

			if(r.length > maxLength)
			{
				maxLength = r.length;
				thisIndex = idx;
			}

			idx++;
		}
	}
	else
	{
		thisIndex = -1;
	}

	if(auto m = getManager())
		getManager()->setCurrentRunIndex(thisIndex);
	else
	{
		currentIndex = thisIndex;
		dynamic_cast<TimelineComponent*>(timeline.get())->rebuild();
	}
}

void DebugSession::ProfileDataSource::ViewComponents::Viewer::updateAdjacentItems(const MouseEvent& e)
{
	if(currentlyHoveredItem != nullptr)
	{
		adjacentDepth = -1;
		adjacentItems = {};
		return;
	}
		
	adjacentDepth = (int)((double)(e.getPosition().getY() - getTimelineOffsetY(showTimeline)) / (double)(ItemHeight + ItemMargin)) + 1;

	if(allRanges.find(adjacentDepth) != allRanges.end())
	{
		const auto& ranges = allRanges.at(adjacentDepth);

		auto nr = getNormalisedZoomRange();

		auto normX = (double)e.getPosition().x / (double)getWidth();
		auto xPos = nr.getStart() + normX * nr.getLength();
		xPos *= totalLength;

		for(int i = 0; i < ranges.size(); i++)
		{
			if(ranges[i].second.getStart() > xPos)
			{
				adjacentItems.second = ranges[i];

				if(i != 0)
					adjacentItems.first = ranges[i-1];

				break;
			}
		}

		xPos -= adjacentItems.first.second.getEnd();
		xPos /= (adjacentItems.second.second.getStart() - adjacentItems.first.second.getEnd());
		adjacentAlpha = jlimit(0.0, 1.0, xPos);
	}
	else
	{
		adjacentDepth = -1;
		adjacentItems = {};
	}

		
}

DebugSession::ProfileDataSource::ViewComponents::ViewItem* DebugSession::ProfileDataSource::ViewComponents::Viewer::
getHoverItem(const MouseEvent& e)
{
	auto t = getZoomTranspose();

	if(auto m = rootItem->isHovered(e, t, showTimeline, scaleFactor * zoomFactor, (int)rowScrollBar.getCurrentRangeStart(), currentIndex))
		return m;

	return nullptr;
}

void DebugSession::ProfileDataSource::ViewComponents::Viewer::setTimeDomain(TimeDomain newDomain)
{
	double newDomainContext = 0.0;

	switch (newDomain)
	{
    case TimeDomain::Undefined:
        break;
    case TimeDomain::Absolute:
		newDomainContext = 0.0;
		break;
	case TimeDomain::Relative:
		newDomainContext = (double)rootItem->getBounds(0.0, showTimeline, 1.0, 0, currentIndex).getWidth();
		break;
	case TimeDomain::CpuUsage:

		jassert(session.getBufferDuration);

		if(session.getBufferDuration)
			newDomainContext = session.getBufferDuration();
				
		break;
	case TimeDomain::FPS60:
		newDomainContext = 0.0;
		break;
	}

	if(auto m = getManager())
		m->setTimeDomain(newDomain, newDomainContext);
}

void DebugSession::ProfileDataSource::ViewComponents::Viewer::toggleTypeFilter(SourceType source)
{
	auto mg = getManager();

	mg->toggleTypeFilter(source);
	
	repaint();
	
	//statistics->updateSearchTerm();
}

void DebugSession::ProfileDataSource::ViewComponents::Viewer::onRunIndexUpdate(int currentRunIndex)
{
	currentIndex = currentRunIndex;
	updateIndexToShow();
	dynamic_cast<TimelineComponent*>(timeline.get())->rebuild();

	if(currentDomain == TimeDomain::Relative)
	{
		if(auto m = getManager())
		{
			// force the update
			m->currentDomain = TimeDomain::Undefined;

			// Refresh the relative timeline
			setTimeDomain(currentDomain);
		}
	}
}

void DebugSession::ProfileDataSource::ViewComponents::Viewer::mouseDown(const MouseEvent& e)
{
	mouseMove(e);

	if(e.mods.isLeftButtonDown())
	{
		if(currentlyHoveredItem != nullptr && currentlyHoveredItem != rootItem.get() && e.mods.isCommandDown())
		{
			navigate(currentlyHoveredItem, true);
			return;
		}

		if(currentNavigationItem != nullptr)
		{
			zoomToItem(currentNavigationItem);
			return;
		}
		

		
	}
	if(e.mods.isRightButtonDown())
	{
		if(currentlyHoveredItem != nullptr)
		{
			auto tr = getZoomTranspose();
			auto s = getZoomScale();
			auto tBounds = currentlyHoveredItem->getBounds(tr, showTimeline, s, (int)rowScrollBar.getCurrentRangeStart(), currentIndex).toNearestInt().getIntersection(getLocalBounds());

			std::unique_ptr<Component> c;
			c.reset(new ItemPopup(this, currentlyHoveredItem));

			currentlyHoveredItem = nullptr;
			
			if(auto t = TopLevelWindowWithOptionalOpenGL::findRoot(this))
			{
				auto b = t->getLocalArea(this, getLocalBounds().getIntersection(tBounds.toNearestInt()));
				CallOutBox::launchAsynchronously(std::move(c), b, t);
				return;
			}
		}
		else
		{
			PopupLookAndFeel plaf;
			PopupMenu m;
			m.setLookAndFeel(&plaf);

			constexpr int TimeDomainOffset = 400;
			constexpr int FilterOffset = 800;

			m.addItem(1, "Fold all", true, foldAll);
			m.addItem(2, "Enable event flow mode", true, rootItem->useEquiDistance);
			

			m.addSectionHeader("Select Time domain");
			m.addItem(TimeDomainOffset + (int)TimeDomain::Absolute, "Absolute", true, currentDomain == TimeDomain::Absolute);
			m.addItem(TimeDomainOffset + (int)TimeDomain::Relative, "Relative", true, currentDomain == TimeDomain::Relative);
			m.addItem(TimeDomainOffset + (int)TimeDomain::CpuUsage, "Audio Buffer CPU %", true, currentDomain == TimeDomain::CpuUsage);
			m.addItem(TimeDomainOffset + (int)TimeDomain::FPS60, "Frametime @ 60FPS", true, currentDomain == TimeDomain::FPS60);

			auto r = m.show();

			if(r == 1)
			{
				foldAll = !foldAll;

				rootItem->callAllRecursive([this](ViewItem& v)
				{
					if(&v != rootItem.get() && !v.children.isEmpty())
					{
						v.folded = foldAll;
					}

					return false;
				});

				updateRowScrollbar();
				resized();
			}
			if(r == 2)
			{
				auto nr = rootItem;
				nr->setUseEquiDistance(!rootItem->useEquiDistance, getTypeFilter());
				rootItem = nullptr;
				navigateInternal(nr);
			}
			if(r >= TimeDomainOffset)
			{
				setTimeDomain((TimeDomain)(r - TimeDomainOffset));
			}

			return;
		}
	}

	if(handleMouseEvent(e, MouseEventType::MouseDown))
		return;
}

void DebugSession::ProfileDataSource::ViewComponents::Viewer::trimToCurrentRange()
{
	auto newItem = ProfileInfoBase::trimToCurrentRange(session, getScaledZoomRange(), rootItem->infoItem);
	session.recordingFlushBroadcaster.sendMessage(sendNotificationSync, newItem);
}

void DebugSession::ProfileDataSource::ViewComponents::Viewer::combineRuns(ProfileInfoBase::Ptr rootInfo,
	ProfileInfoBase::Ptr ptr)
{
	auto newItem = ProfileInfoBase::combineRuns(session, rootInfo, ptr);
	session.recordingFlushBroadcaster.sendMessage(sendNotificationSync, newItem);
}

void DebugSession::ProfileDataSource::ViewComponents::Viewer::onNavigation(BaseWithManagerConnection* source,
                                                                           ViewItem::Ptr o, ViewItem::Ptr c)
{
	if(getManager()->currentViewer.get() == this)
	{
		setRootItems(o, c);
	}
}

void DebugSession::ProfileDataSource::ViewComponents::Viewer::setActiveViewer(bool isActive)
{
	activeViewer = isActive;
	repaint();
}

DebugSession::ProfileDataSource::ViewComponents::StatisticsComponent* DebugSession::ProfileDataSource::ViewComponents::
Viewer::getStatistics()
{
	if(auto m = getManager())
	{
		for(auto b: m->connectedComponents)
		{
			if(auto s = dynamic_cast<StatisticsComponent*>(b.get()))
				return s;
		}
	}

	return nullptr;
}

void DebugSession::ProfileDataSource::ViewComponents::Viewer::zoomToItem(ViewItem::Ptr ptr)
{
	auto b = ptr->getBounds(0.0f, false, 1.0, (int)rowScrollBar.getCurrentRangeStart(), currentIndex);

	NormalisableRange<double> nr(getScaledZoomRange());

	if(nr.getRange().contains(b.getX()) && nr.getRange().contains(b.getRight()))
	{
		auto s = nr.convertTo0to1(b.getX());
		auto e = nr.convertTo0to1(b.getRight());
		auto l = e - s;

		s -= l;
		e += l;

		zoomToRange({ s, e});
	}
	else
	{
		NormalisableRange<double> nr(0.0, totalLength);

		auto s = nr.convertTo0to1(b.getX());
		auto e = nr.convertTo0to1(b.getRight());
		auto l = e - s;
		s -= l;
		e += l;

		zoomToRange({ s, e });
		//moveToRange(ns);
	}
}

void DebugSession::ProfileDataSource::ViewComponents::Viewer::setArrowNavigation(const ViewItem* ptr,
	Rectangle<float> area)
{
	arrowNavigations[const_cast<ViewItem*>(ptr)] = area;
}

void DebugSession::ProfileDataSource::ViewComponents::Viewer::mouseMagnify(const MouseEvent& e, float sf)
{
	hangleMouseMagnify(e, sf);
}

void DebugSession::ProfileDataSource::ViewComponents::Viewer::mouseMove(const MouseEvent& e)
{
	currentNavigationItem = nullptr;
	currentlyHoveredItem = getHoverItem(e);

	if(currentlyHoveredItem != nullptr && currentlyHoveredItem->name == "Profile root")
		currentlyHoveredItem = nullptr;

	if(currentlyHoveredItem == nullptr)
	{
		auto posFloat = e.getPosition().toFloat();

		for(auto& v: arrowNavigations)
		{
			if(v.second.contains(posFloat))
			{
				currentNavigationItem = v.first;
				break;
			}
		}
	}

	updateAdjacentItems(e);
	updateMouseCursorForEvent(e);
	repaint();
}

void DebugSession::ProfileDataSource::ViewComponents::Viewer::mouseUp(const MouseEvent& e)
{
	handleMouseEvent(e, MouseEventType::MouseUp);
}

VoiceBitMap<32> DebugSession::ProfileDataSource::ViewComponents::Viewer::getTypeFilter()
{
	return getManager() != nullptr ? getManager()->typeFilters : VoiceBitMap<32>();
}

void DebugSession::ProfileDataSource::ViewComponents::Viewer::mouseDoubleClick(const MouseEvent& e)
{
	if(e.mods.isRightButtonDown())
		return;

	if(currentlyHoveredItem != nullptr)
	{
		if(auto nd = rootItem->toggleFold(currentlyHoveredItem, getTypeFilter()))
		{
			updateRowScrollbar();
		}
			
		resized();
	}
	else
	{
		handleMouseEvent(e, MouseEventType::MouseDoubleClick);
	}
}

void DebugSession::ProfileDataSource::ViewComponents::Viewer::mouseWheelMove(const MouseEvent& event,
	const MouseWheelDetails& wheel)
{
	handleMouseWheelEvent(event, wheel);
}

UndoManager* DebugSession::ProfileDataSource::ViewComponents::Viewer::getUndoManager()
{
	if(auto m = getManager())
		return &m->um;

	return nullptr;
}

void DebugSession::ProfileDataSource::ViewComponents::Viewer::navigateInternal(ViewItem::Ptr newDisplayRoot)
{
	if(newDisplayRoot != rootItem)
	{
		currentlyHoveredItem = nullptr;
		rootItem = newDisplayRoot;
		rootItem->setAsRoot();

		if(!multiMode)
			breadcrumbs->setVisible(rootItem != originalRoot);
		else
			breadcrumbs->setVisible(true);

		rootItem->callAllRecursive([](ViewItem& vi)
		{
			vi.folded = false;
			return false;
		});

		refreshPeakButton();
		updateRowScrollbar();
		originalRoot->updateSearch({});

		allRanges.clear();

		

		for(auto& r: allRanges)
			std::sort(r.second.begin(), r.second.end(), [](const std::pair<ViewItem::Ptr, Range<double>>& r1, const std::pair<ViewItem::Ptr, Range<double>>& r2) { return r1.second.getStart() < r2.second.getStart(); });

		setTimeDomain(newDisplayRoot->getPreferredDomain());

		dynamic_cast<BreadcrumbComponent*>(breadcrumbs.get())->updateBreadcrumbs();
		updateIndexToShow();
		resized();
		dynamic_cast<TimelineComponent*>(timeline.get())->rebuild();
	}
}

void DebugSession::ProfileDataSource::ViewComponents::Viewer::setRootItems(ViewItem::Ptr newOriginalRoot,
                                                                           ViewItem::Ptr newDisplayRoot)
{
	if(newOriginalRoot != originalRoot)
	{
		mode = newOriginalRoot->mode;

		if(mode == ProfileIds::root)
		{
			currentIndex = -1;
			originalRoot = newOriginalRoot;
			showTimeline = originalRoot->runs.size() != 1;
		}
		else if (mode == ProfileIds::multithread)
		{
			showTimeline = false;
			currentIndex = 0;
			originalRoot = newOriginalRoot;
			tracks = originalRoot->createTracks();
		}
	}

	navigateInternal(newDisplayRoot);
	resized();
}

bool DebugSession::ProfileDataSource::ViewComponents::Viewer::keyPressed(const KeyPress& k)
{
	if(k == KeyPress::backspaceKey)
	{
		if(auto um = getUndoManager())
		{
			if(k.getModifiers().isCtrlDown())
				um->redo();
			else
				um->undo();
		}
			
		return true;
	}
	 
	return false;
}

void DebugSession::ProfileDataSource::ViewComponents::Viewer::paint(Graphics& g)
{
	auto vp = getViewportPosition();
	vp.setTop(vp.getY() - FirstItemOffset);
	drawBackgroundGrid(g, vp);

	ViewItem::DrawContext ctx;

	auto t = getTypeFilter();

	ctx.typeFilter = &t;
	ctx.transpose = getZoomTranspose(); 
	ctx.scaleFactor = getZoomScale();
	ctx.showTimeline = showTimeline;
	ctx.totalWidth = getWidth();
	ctx.fullBounds = getLocalBounds().toFloat();
	ctx.currentIndex = currentIndex;
	ctx.currentlyHoveredItem = currentlyHoveredItem;
	ctx.currentRoot = rootItem;
	ctx.domainContext = domainContext;
	ctx.domain = currentDomain;
	ctx.componentToDrawOn = this;
	ctx.displayRange = getScaledZoomRange();

	auto r = rowScrollBar.getCurrentRange();

	ctx.depthRange = { (int)r.getStart(), (int)r.getEnd() + 1 };

	if(rootItem->useEquiDistance)
	{
		for(const auto& t: tracks)
		{
			t.first->drawTrack(g, ctx, t.second);
		}
	}
		
	rootItem->draw(g, ctx);

	drawDragDistance(g, getViewportPosition());

	for(const auto& l: labels)
		Helpers::drawLabel(g, l);
}

Rectangle<int> DebugSession::ProfileDataSource::ViewComponents::Viewer::getViewportPosition() const
{
	return { 0, getTimelineOffsetY(showTimeline), getWidth(), maxDepth * (ItemHeight + ItemMargin)};
}

void DebugSession::ProfileDataSource::ViewComponents::Viewer::resized()
{
	initManager();
	labels.clear();

	auto b = getLocalBounds();

	if(!getBounds().isEmpty() && totalLength > 0)
	{
		int ButtonMargin = 4;

		if(showTimeline)
		{
			labels.push_back({b.removeFromTop(LabelMargin).toFloat(), "Run performance"});
			auto timelineBounds = b.removeFromTop(TopBarHeight);
			peakButton.setBounds(timelineBounds.removeFromRight(timelineBounds.getHeight()).reduced(ButtonMargin * 2));
			timeline->setBounds(timelineBounds);
				
		}

		String t = "Timeline";

		switch(currentDomain)
		{
		case TimeDomain::Absolute: t << " (Absolute)";
			break;
		case TimeDomain::Relative: t << " (Relative)";
			break;
		case TimeDomain::CpuUsage: t << " (Audio Buffer CPU %)";
			break;
		case TimeDomain::FPS60: t << " (Frametime @ 60FPS)";
			break;
		default: ;
		}

		breadcrumbs->setBounds(b.removeFromTop(BreadcrumbHeight));

		labels.push_back({b.removeFromTop(LabelMargin).toFloat(), t});

		auto numRowsToDisplay = b.getHeight() / (ItemHeight + ItemMargin);

		rowScrollBar.setCurrentRange(rowScrollBar.getCurrentRangeStart(), (double)numRowsToDisplay, sendNotificationAsync);

		onResize(b);

		rowScrollBar.setVisible(true);
		rowScrollBar.setBounds(b.removeFromRight(12));
	}
}

void DebugSession::ProfileDataSource::ViewComponents::Viewer::updateRowScrollbar()
{
	maxDepth = rootItem->getMaxDepth(1, getTypeFilter());
	rowScrollBar.setRangeLimits(0.0, (double)(maxDepth + 1), sendNotificationAsync);
}

void DebugSession::ProfileDataSource::ViewComponents::Viewer::onSearchUpdate(const String& s, bool zoomToFirst)
{
	repaint();

	auto m = rootItem->updateSearch(s);
	updateRowScrollbar();

	if(rootItem->useEquiDistance)
	{
		int idx = 0;
		rootItem->updateEquiDistanceRange(idx, getTypeFilter());

		allRanges.clear();

		rootItem->callAllRecursive([&](ViewItem& p)
		{
			auto b = p.getBounds(0.0f, showTimeline, 1.0f, (int)rowScrollBar.getCurrentRangeStart(), currentIndex);
			allRanges[p.depth].push_back({&p, { b.getX(), b.getRight() }});
			return false;
		});

		totalLength = getTotalLength();
		resized();
	}

	if(zoomToFirst && m != nullptr)
		zoomToItem(m);
}

bool DebugSession::ProfileDataSource::ViewComponents::Viewer::onWASDMovement(WASDKeyListener::MovementType t,
	double delta)
{
	if(AbstractZoomableView::onWASDMovement(t, delta))
		return true;

	if(t == WASDKeyListener::MovementType::Vertical)
	{
		auto before = roundToInt(rowPositionDouble);
		rowPositionDouble += delta * JUCE_LIVE_CONSTANT_OFF(0.25);
		auto after = roundToInt(rowPositionDouble);

		if(before != after)
		{
			auto delta = before - after;
			rowScrollBar.moveScrollbarInSteps(delta, sendNotificationSync);
		}
	}

	return true;
}

bool DebugSession::ProfileDataSource::ViewComponents::Viewer::navigate(ViewItem::Ptr newRoot, bool useUndoManager)
{
	if(auto m = getManager())
		m->navigate(this, newRoot, useUndoManager);

	return true;

#if 0
	if(newRoot == rootItem)
		return false;

	auto um = getUndoManager();

	useUndoManager &= um != nullptr;

	if(useUndoManager)
	{
		um->beginNewTransaction();
		um->perform(new NavigationAction(*this, rootItem, newRoot));
		return false;
	}
	else
	{
		currentlyHoveredItem = nullptr;
		rootItem = newRoot;
		rootItem->setAsRoot();
		
		refreshPeakButton();

		updateRowScrollbar();
		originalRoot->updateSearch({});

		allRanges.clear();

		rootItem->callRecursive([&](ViewItem& p)
		{
			auto b = p.getBounds(0.0f, showTimeline, 1.0f, currentIndex);
			allRanges[p.depth].push_back({&p, { b.getX(), b.getRight() }});
			return false;
		});

		for(auto& r: allRanges)
			std::sort(r.second.begin(), r.second.end(), [](const std::pair<ViewItem::Ptr, Range<double>>& r1, const std::pair<ViewItem::Ptr, Range<double>>& r2) { return r1.second.getStart() < r2.second.getStart(); });

		MessageManager::callAsync([this, um]()
		{
			homeButton.setEnabled(originalRoot != rootItem);
		});

		updateIndexToShow();
		resized();
		timeline.rebuild();
		return true;
	}
#endif
}



}
