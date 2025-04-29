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

DebugSession::ProfileDataSource::ViewComponents::ViewItem::ViewItem(ProfileInfoBase* pi, double first_, int depth_,
	int numRuns, int thisRunIndex):
	depth(depth_),
	first(first_),
	mode(pi->getProfileId()),
	infoItem(pi)
{
	if(mode == ProfileIds::multithread)
	{
		name = "Profile root";

		double start = std::numeric_limits<double>::max();
		double end = 0.0;

		for(auto c: pi->children)
		{
			auto pi = dynamic_cast<ProfileInfo*>(c);

			start = jmin(start, pi->data.start);
			end = jmax(end, pi->data.start + pi->data.delta);
		}

		Range<double> totalRange(start, end);

		first = totalRange.getStart();

		Position newRun;
		newRun.xOffset = 0.0;
		newRun.x = 0.0f;
		newRun.length = totalRange.getLength();

		Data nd;
		nd.source = nullptr;
		nd.parent = nullptr;
		nd.start = totalRange.getStart();
		nd.delta = totalRange.getLength();
		nd.holder = &pi->d;

		runs.push_back(newRun);
		runData.push_back(nd);

		for(auto c: pi->children)
		{
			auto ni = new ViewItem(c, first, 1, 1, 0);
			children.add(ni);
		}

		callAllRecursive([](ViewItem& i)
		{
			i.multiThreadMode = true;
			return false;
		});
	}
	else
	{
		ProfileInfo* p;

		if(mode == ProfileIds::root)
			p = dynamic_cast<ProfileInfo*>(pi->children.getFirst().get());
		else
			p = dynamic_cast<ProfileInfo*>(pi);

		sourceType = p->data.source->sourceType;

		threadType = p->data.threadType;

		name = p->getTextForValue();

		c = p->data.source->colour;

        hasTracks = p->data.trackSource != -1 || p->data.trackTarget != -1;
        
		if(first == -1.0f)
			first = p->data.start;

		Position newRun;
		newRun.xOffset = 0.0;
		newRun.x = p->data.start - first;
		newRun.length = p->data.delta;

		runs.reserve(numRuns);
		runData.reserve(numRuns);

		for(int i = 0; i < numRuns; i++)
		{
			runs.push_back({0.0, 0.0});
			runData.push_back({});
		}
			
		jassert(isPositiveAndBelow(thisRunIndex, numRuns));

		runData[thisRunIndex] = p->data;
		runs[thisRunIndex] = newRun;

		for(auto c: p->children)
			children.add(new ViewItem(c, first, depth + 1, numRuns, thisRunIndex));

		if(mode == ProfileIds::root)
		{
			jassert(numRuns == pi->getNumChildElements());

			for(int i = 1; i < numRuns; i++)
			{
				auto thisRun = dynamic_cast<ProfileInfo*>(pi->getChildElement(i).get());
				jassert(thisRun->data.parent == nullptr);
					
				auto firstInRun = thisRun->data.start;
				addRun(firstInRun, thisRun, i);
			}

			for(auto c: children)
			{
				auto numUsed = 0.0;
				auto s = 0.0;

				for(int i = 0; i < numRuns; i++)
				{
					if(c->runData[i].source != nullptr)
					{
						s += c->runs[i].x;
						numUsed++;
					}
				}

				jassert(numUsed != 0.0);
				s /= numUsed;

				for(int i = 0; i < numRuns; i++)
				{
					if(c->runData[i].source == nullptr)
						c->runs[i].x = s;
				}
			}

			struct Comp
			{
				static int compareElements(ViewItem* i1, ViewItem* i2)
				{
					auto numRuns = (int)i1->runs.size();

					for(int i = 0; i < numRuns; i++)
					{
						if(i1->runData[i].source != nullptr &&
							i2->runData[i].source != nullptr)
						{
							auto s1 = i1->runData[i].start;
							auto s2 = i1->runData[i].start;

							if(s1 < s2)
								return -1;
							else if(s1 > s2)
								return 1;
							else 
								return 0;
						}
					}

					return 0;
				}
					  
			} comp;

			children.sort(comp);

			addLoopRanges(thisRunIndex);
		}
	}

	for(auto c: children)
		c->parent = this;
}

void DebugSession::ProfileDataSource::ViewComponents::ViewItem::addLoopRanges(int thisRunIndex)
{
	for(int i = 1; i < children.size(); i++)
	{
		auto ps = children[i-1]->runData[thisRunIndex].source;
		auto ts = children[i]->runData[thisRunIndex].source;

		if(ps != nullptr && ps->loopCounter != -1)
		{
			if(ps == ts)
				loopRanges.addRange({i-1, i});
		}
	}

	for(auto c: children)
		c->addLoopRanges(thisRunIndex);
}

void DebugSession::ProfileDataSource::ViewComponents::ViewItem::drawTrack(Graphics& g, const DrawContext& ctx,
	Ptr target) const
{
	if(auto v = dynamic_cast<Viewer*>(ctx.componentToDrawOn))
	{
		v->setArrowNavigation(target.get(), {});
		v->setArrowNavigation(this, {});
	}

	if(!shouldBeDisplayed(ctx) || !target->shouldBeDisplayed(ctx))
		return;

	auto b1 = getBounds(ctx.transpose, ctx.showTimeline, ctx.scaleFactor, ctx.depthRange.getStart(), ctx.currentIndex);
	auto b2 = target->getBounds(ctx.transpose, ctx.showTimeline, ctx.scaleFactor, ctx.depthRange.getStart(), ctx.currentIndex);

	Point<float> start(b1.getRight(), b1.getCentreY());
	Point<float> end(b2.getX(), b2.getCentreY());

	if(isParentOf(target.get()))
		start = { b1.getX(), b1.getCentreY() };

	if(ctx.fullBounds.intersects(Rectangle<float>(start, end)))
	{
		auto cstart = ctx.fullBounds.getConstrainedPoint(start);
		auto cend = ctx.fullBounds.getConstrainedPoint(end);

		auto d1 = cstart.getDistanceFrom(start);
		auto d2 = cend.getDistanceFrom(end);
		auto d3 = start.getDistanceFrom(end);

		auto ratio = jlimit(0.05f, 1.0f, 1.0f - (d1 + d2) / d3);

		

		std::swap(cstart, start);
		std::swap(cend, end);

		g.setGradientFill(ColourGradient(c.withAlpha(ratio), start.getX(), 0.0f, target->c.withAlpha(ratio), end.getX(), 0.0f, false));
		g.fillRect(Rectangle<float>(start, start).expanded(3.0f));

		Path head;
		head.addTriangle(end.translated(-10.0f, -5.0f), end.translated(-10.0f, 5.0f), end);
		g.fillPath(head);

		end = end.translated(-10.0f, 0.0f);

		Path l;

		l.startNewSubPath(start);

		auto mid = Line<float>(start, end).getPointAlongLineProportionally(0.5);

		auto d = (mid.getX() - start.getX()) * 0.5f;
		auto c1 = mid.withY(start.getY()).translated(-d, 0.0f);
		auto c2 = mid.withY(end.getY()).translated(d, 0.0f);
			
		l.quadraticTo(c1, mid);
		l.quadraticTo(c2, end);

		g.strokePath(l, PathStrokeType(2.0f));

		auto startVisible = ctx.fullBounds.intersects(b1);
		auto endVisible = ctx.fullBounds.intersects(b2);

		

		if(startVisible && !endVisible)
		{
			auto area = b1.removeFromRight(b1.getHeight()).translated(b1.getHeight(), 0.0f);
			Path icon;
			icon.loadPathFromData(EditorIcons::forwardIcon, EditorIcons::forwardIcon_Size);

			if(PathFactory::scalePath(icon, area.reduced(3.0f)))
			{
				g.setColour(Colour(0xFF777777));
				g.fillPath(icon);

				if(auto v = dynamic_cast<Viewer*>(ctx.componentToDrawOn))
				{
					v->setArrowNavigation(target.get(), area);
				}
			}
		}
		if(!startVisible && endVisible)
		{
			auto area = b2.removeFromLeft(b2.getHeight()).translated(-1.0f * b2.getHeight(), 0.0f);
			Path icon;
			icon.loadPathFromData(EditorIcons::backIcon, EditorIcons::backIcon_Size);

			if(PathFactory::scalePath(icon, area.reduced(3.0f)))
			{
				g.setColour(Colour(0xFF777777));
				g.fillPath(icon);

				if(auto v = dynamic_cast<Viewer*>(ctx.componentToDrawOn))
				{
					v->setArrowNavigation(this, area);
				}
			}
		}
	}
}

String DebugSession::ProfileDataSource::ViewComponents::ViewItem::getDuration(int currentIndex, bool, TimeDomain domain,
	double contextValue) const
{
	auto w = getBounds(0.0f, false, 1.0f, 0, currentIndex).getWidth();
	return Helpers::getDuration(w, domain, contextValue);
}

DebugSession::ProfileDataSource::ViewComponents::ViewItem::Ptr DebugSession::ProfileDataSource::ViewComponents::ViewItem
::callRecursive(const std::function<bool(ViewItem&)>& f, bool callFoldedChildren) const
{
	if(f(*const_cast<ViewItem*>(this)))
		return const_cast<ViewItem*>(this);

	if(!folded || callFoldedChildren)
	{
		for(auto c: children)
		{
			if(auto match = c->callRecursive(f, callFoldedChildren))
				return match;
		}
	}

	return nullptr;
}

void DebugSession::ProfileDataSource::ViewComponents::ViewItem::setAsRoot()
{
	for(auto& r: runs)
		r.xOffset = r.x;
		
	calculateAverage(this);
}

DebugSession::ProfileDataSource::ViewComponents::ViewItem::Ptr DebugSession::ProfileDataSource::ViewComponents::ViewItem
::updateSearch(const String& searchTerm)
{
	searchEmpty = searchTerm.length() < 3;
	matchesSearch = !searchEmpty && name.toLowerCase().contains(searchTerm.toLowerCase());

	ViewItem::Ptr match;

	for(auto c: children)
	{
		if(auto m = c->updateSearch(searchTerm))
			match = m;
	}

	if(matchesSearch)
	{
		auto p = parent.get();

		while(p != nullptr)
		{
			p->folded = false;
			p = p->parent;
		}
	}

	if(match == nullptr && searchTerm == name)
		return this;

	return match;
}

int DebugSession::ProfileDataSource::ViewComponents::ViewItem::getMaxDepth(int newDepth, const VoiceBitMap<32>& typeFilters)
{
	depth = newDepth;

	auto maxDepth = depth;

	if(folded)
		return maxDepth;

	if(typeFilters[(int)sourceType])
		depth -= 1;

	if(mode == ProfileIds::multithread)
	{
		depth -= 1;
		auto childDepth = depth;

		for(auto c: children)
		{
			childDepth = c->getMaxDepth(childDepth + 1, typeFilters);
			maxDepth = childDepth;
		}
	}
	else
	{
		for(auto c: children)
			maxDepth = jmax(maxDepth, c->getMaxDepth(depth+1, typeFilters));
	}
		
	return maxDepth;
}

bool DebugSession::ProfileDataSource::ViewComponents::ViewItem::isFoldedRecursive() const
{
	if(folded)
		return true;

	if(parent != nullptr)
		return parent->isFoldedRecursive();

	return false;
}

void DebugSession::ProfileDataSource::ViewComponents::ViewItem::calculateAverage(ViewItem* rootItem)
{
	double w = 0.0;
	double x = 0.0;

	jassert(runs.size() == rootItem->runs.size());

	for(int i = 0; i < runs.size(); i++)
		runs[i].xOffset = rootItem->runs[i].xOffset;

	auto numRuns = 0.0;

	for(auto r: runs)
	{
		x += r.x;
		w += r.length;
		numRuns += 1.0;
	}

	w *= 1.0 / numRuns;
	x *= 1.0 / numRuns;
		
	avg.x = x;
	avg.length = w;
	avg.xOffset = (rootItem == this) ? x : rootItem->avg.xOffset;

	auto l = 0.0;

	for(auto i: children)
	{
		i->calculateAverage(rootItem);
		l += i->avg.length;
	}

	avg.length = jmax(avg.length, l - avg.x );
}

std::vector<std::pair<DebugSession::ProfileDataSource::ViewComponents::ViewItem::Ptr, DebugSession::ProfileDataSource::
ViewComponents::ViewItem::Ptr>> DebugSession::ProfileDataSource::ViewComponents::ViewItem::createTracks()
{
	std::vector<Ptr> closeTracks;
	std::vector<Ptr> openTracks;

	callUnfoldedRecursive([&](ViewItem& i)
	{
		auto ts = i.runData[0].trackSource;
		auto tt = i.runData[0].trackTarget;


		if(ts != -1)
			openTracks.push_back(&i);
		if(tt != -1)
			closeTracks.push_back(&i);

		return false;
	});

	std::vector<std::pair<Ptr, Ptr>> tracks;
	tracks.reserve(openTracks.size());

	for(const auto& o: openTracks)
	{
		auto idx = o->runData[0].trackSource;

		for(const auto& c: closeTracks)
		{
			if(c == o)
				continue;

			if(c->runData[0].trackTarget == idx)
			{
				tracks.push_back({o, c});
				break;
			}
		}
	}

	return tracks;
}

DebugSession::ProfileDataSource::ViewComponents::ViewItem* DebugSession::ProfileDataSource::ViewComponents::ViewItem::
getChildForData(const Data& d)
{
	for(auto c: children)
	{
		const auto& cd = c->runData[0];

		auto loopMatches = cd.loopIndex == d.loopIndex;
		auto sourceMatches = d.source == cd.source;
		auto parentMatches = d.parent == cd.parent;

		if(loopMatches && sourceMatches && parentMatches)
			return c;
	}

	return nullptr;
}

void DebugSession::ProfileDataSource::ViewComponents::ViewItem::addRun(double first, ProfileInfo* p, int runIndex)
{
	runs[runIndex].xOffset = 0.0;
	runs[runIndex].x = p->data.start - first;
	runs[runIndex].length = p->data.delta;
	runData[runIndex] = p->data;

	for(auto c: p->children)
	{
		auto tc = dynamic_cast<ProfileInfo*>(c);

		if(auto existing = getChildForData(tc->data))
			existing->addRun(first, tc, runIndex);
		else
			children.add(new ViewItem(tc, first, depth+1, (int)runs.size(), runIndex));
	}
}

Rectangle<float> DebugSession::ProfileDataSource::ViewComponents::ViewItem::getBounds(float transpose, bool showTimeline,
                                                                                      float scaleFactor, int topItemDepth, int index) const
{

	auto thisDepth = depth - 1 - topItemDepth;

	auto y = (float)(Viewer::getTimelineOffsetY(showTimeline) + thisDepth * (ItemHeight + ItemMargin));
	auto h = (float)ItemHeight-1.0f;

	float x = 0.0f;
	float w = 0.0f;

	if(useEquiDistance)
	{
		x = (double)equiDistanceRange.getStart();
		w = (double)equiDistanceRange.getLength();
	}
	else
	{
		if(index == -1)
		{
			x = avg.x - avg.xOffset;
			w = avg.length;
		}
		else if (isPositiveAndBelow(index, runs.size()))
		{
			x = (float)runs[index].x - runs[index].xOffset;
			w = (float)runs[index].length;
		}
	}

	x -= transpose;
	x *= scaleFactor;
	w *= scaleFactor;

	return { (float)x, y, (float)w, h};
}

void DebugSession::ProfileDataSource::ViewComponents::ViewItem::getNames(StringArray& names)
{
	names.addIfNotAlreadyThere(name);
	for(auto c: children)
		c->getNames(names);
}

DebugSession::ProfileDataSource::ViewComponents::ViewItem* DebugSession::ProfileDataSource::ViewComponents::ViewItem::
isHovered(const MouseEvent& e, float transpose, bool showTimeline, float scaleFactor, int topItemDepth, int currentIndex)
{
	auto a = getBounds(transpose, showTimeline, scaleFactor, topItemDepth, currentIndex);

	if(a.contains(e.getPosition().toFloat()))
		return this;

	for(auto i: children)
	{
		if(auto m = i->isHovered(e, transpose, showTimeline, scaleFactor, topItemDepth, currentIndex))
			return m;
	}

	return nullptr;
}

void DebugSession::ProfileDataSource::ViewComponents::ViewItem::paintItem(Graphics& g, Rectangle<float> thisBounds,
	const DrawContext& ctx)
{
	float searchAlpha = 1.0f;

	if(!searchEmpty && !matchesSearch)
		searchAlpha = 0.6f;

	if(matchesSearch)
	{
		g.setColour(Colours::white);
		g.drawRoundedRectangle(thisBounds.expanded(2.0), 3.0f, 2.0f);
	}

	if(multiThreadMode && thisBounds.getWidth() <= 4.1f)
	{
		g.setColour(c.withMultipliedAlpha(searchAlpha).withMultipliedBrightness(0.5f));
		g.fillRect(thisBounds.reduced(1.0f));

		return;
	}

	auto tc = c.withMultipliedBrightness(ctx.currentlyHoveredItem.get() == this ? 1.3f : 1.0f);

	g.setColour(tc.withMultipliedAlpha(searchAlpha).withMultipliedBrightness(0.5f));
	g.fillRect(thisBounds.reduced(1.0f));
	g.setColour(tc.withMultipliedAlpha(searchAlpha));
	g.drawRect(thisBounds.reduced(1.0f), 1.0f);
	g.setFont(GLOBAL_MONOSPACE_FONT());

	auto title = name;
	auto gb = Rectangle<float>(0.0, thisBounds.getY(), ctx.totalWidth, thisBounds.getHeight());
	auto textBounds = gb.getIntersection(thisBounds);

	if(textBounds.getWidth() > 5.0f)
	{
		g.setColour(Colours::white.withAlpha(0.8f * searchAlpha));

		if(!children.isEmpty())
			textBounds.removeFromLeft(textBounds.getHeight());
		else
			textBounds.removeFromLeft(5.0f);

		auto t = title;

		if(ctx.currentlyHoveredItem.get() == this && !useEquiDistance)
			t << " (" << getDuration(ctx.currentIndex, ctx.showTimeline, ctx.domain, ctx.domainContext) << ")";

		g.drawText(t, textBounds, Justification::left);
		//g.setFont(GLOBAL_BOLD_FONT());
		//g.drawText(getDuration(ctx.currentIndex, ctx.showTimeline, ctx.domain, ctx.domainContext), textBounds.reduced(5.0f), Justification::centredBottom);
	}
}

bool DebugSession::ProfileDataSource::ViewComponents::ViewItem::shouldBeDisplayed(const DrawContext& ctx) const
{
    if(!cachedVisibility)
        return false;

	if(!ctx.depthRange.isEmpty() && !ctx.depthRange.contains(depth-1))
		return false;

	auto shouldBeDisplayed = ctx.typeFilter != nullptr && !(*ctx.typeFilter)[(int)sourceType];
    
	auto p = parent.get();

	auto isChildOfRoot = (ctx.currentRoot == this) || (ctx.currentRoot.get() == p);

	auto isFolded = false;

	while(p != nullptr && !isFolded)
	{
		isChildOfRoot |= p == ctx.currentRoot.get();
		isFolded |= p->folded;
		p = p->parent.get();
	}

	if(isFolded || !isChildOfRoot)
		return false;

	return shouldBeDisplayed;
}

void DebugSession::ProfileDataSource::ViewComponents::ViewItem::draw(Graphics& g, const DrawContext& ctx)
{
	if(mode == ProfileIds::multithread)
	{
		for(auto c: children)
			c->draw(g, ctx);

		return;
	}

	auto normBounds = getBounds(0.0, false, 1.0f, ctx.depthRange.getStart(), ctx.currentIndex);

	Range<double> normRange = { normBounds.getX(), normBounds.getRight() };

	if(!ctx.displayRange.intersects(normRange))
		return;

	if(!shouldBeDisplayed(ctx))
	{
		for(auto c: children)
			c->draw(g, ctx);

		return;
	}
			
	auto thisBounds = getBounds(ctx.transpose, ctx.showTimeline, ctx.scaleFactor, ctx.depthRange.getStart(), ctx.currentIndex);

	thisBounds = thisBounds.getIntersection(ctx.fullBounds);

	if(matchesSearch || multiThreadMode)
		thisBounds.setWidth(jmax(thisBounds.getWidth(), 4.0f));

	if(thisBounds.getX() > ctx.totalWidth || thisBounds.getRight() < 0 || (!matchesSearch && thisBounds.getWidth() < 1))
		return;

	paintItem(g, thisBounds, ctx);

	if(!folded)
	{
		for(auto c: children)
			c->draw(g, ctx);
	}

	float searchAlpha = 1.0f;

	if(!searchEmpty && !matchesSearch)
		searchAlpha = 0.6f;

	for(auto lr: loopRanges.getRanges())
	{
		auto first = children[lr.getStart()]->getBounds(ctx.transpose, ctx.showTimeline, ctx.scaleFactor, ctx.depthRange.getStart(), ctx.currentIndex);
		auto last = children[lr.getEnd()]->getBounds(ctx.transpose, ctx.showTimeline, ctx.scaleFactor, ctx.depthRange.getStart(), ctx.currentIndex);
		auto loopArea = first.getUnion(last);

		Path p;

		p.startNewSubPath(loopArea.getX() + 5, loopArea.getY());
		p.lineTo(loopArea.getTopLeft());
		p.lineTo(loopArea.getBottomLeft());
		p.lineTo(loopArea.getX() + 5, loopArea.getBottom());
		p.startNewSubPath(loopArea.getRight() - 5, loopArea.getY());
		p.lineTo(loopArea.getTopRight());
		p.lineTo(loopArea.getBottomRight());
		p.lineTo(loopArea.getRight() - 5, loopArea.getBottom());

		g.setColour(Colours::white.withAlpha(searchAlpha));
		g.strokePath(p, PathStrokeType(2.0f));
	}

	if(!children.isEmpty() && thisBounds.getWidth() > thisBounds.getHeight())
	{
		Path p;
		p.addTriangle({0.0f, 0.0f}, {0.0f, 1.0f}, { 1.0f, 0.5f});

		if(!folded)
			p.applyTransform(AffineTransform::rotation(float_Pi / 2.0f));

		auto copy = thisBounds;

		PathFactory::scalePath(p, copy.removeFromLeft(thisBounds.getHeight()).withSizeKeepingCentre(10.0f, 10.0f));

		g.setColour(Colours::white.withAlpha(ctx.currentlyHoveredItem.get() == this ? 0.4f : 0.1f));
		g.fillPath(p);
	}
}

int DebugSession::ProfileDataSource::ViewComponents::ViewItem::toggleFold(ViewItem::Ptr itemToFold, VoiceBitMap<32> typeFilter)
{
	if(auto s = dynamic_cast<ProfileInfo*>(itemToFold->infoItem.get()))
	{
		auto shouldBeFolded = !itemToFold->folded;
		auto sourceToFold = s->data.source;

		callUnfoldedRecursive([sourceToFold, shouldBeFolded](ViewItem& v)
		{
			if(auto m = dynamic_cast<ProfileInfo*>(v.infoItem.get()))
			{
				if(m->data.source == sourceToFold)
					v.folded = shouldBeFolded;
			}

			return false;
		});

		return getMaxDepth(1, typeFilter);
	}

	return 0;
}

void DebugSession::ProfileDataSource::ViewComponents::ViewItem::updateEquiDistanceRange(int& currentIndex, VoiceBitMap<32> typeFilter)
{
	if(!useEquiDistance)
    {
        callAllRecursive([](ViewItem& v) { v.cachedVisibility = true; return false; });
        return;
    }

	auto thisStart = currentIndex;
	auto thisEnd = 0;

	if(infoItem->getProfileId() == ProfileIds::multithread)
	{
		thisStart = 0;

		std::vector<ViewItem::Ptr> topLevelItems;

		DrawContext ctx;
		ctx.currentRoot = this;
		ctx.typeFilter = &typeFilter;
		ctx.currentlyHoveredItem = nullptr;

        if(useEquiDistance)
        {

        }

		for(auto thread: children)
		{
			for(auto topLevelEvent: thread->children)
			{
				if(!topLevelEvent->shouldBeDisplayed(ctx))
					continue;
                
                auto hasTracks = topLevelEvent->callAllRecursive([](ViewItem& v) { return v.hasTracks; });
                
                
                if(!hasTracks)
                {
                    topLevelEvent->callAllRecursive([](ViewItem& v) { v.cachedVisibility = false; return false; });
                    
                    continue;
                }
                else
                {
                    topLevelEvent->callAllRecursive([](ViewItem& v) { v.cachedVisibility = v.hasTracks; return false; });
                }
                    
				topLevelItems.push_back(topLevelEvent);
			}
		}

		std::sort(topLevelItems.begin(), topLevelItems.end(), [](ViewItem::Ptr v1, ViewItem::Ptr v2) { return v1->avg.x < v2->avg.x; });

		for(auto s: topLevelItems)
		{
			s->updateEquiDistanceRange(currentIndex, typeFilter);
			currentIndex++;
		}

		for(auto thread: topLevelItems)
		{
			auto e = thread->equiDistanceRange.getEnd();
			thisEnd = jmax(thisEnd, e);
		}

		for(auto thread: children)
		{
			thread->equiDistanceRange = { 0, thisEnd };
		}
	}
	else
	{
        int before = currentIndex;
        
		for(auto c: children)
			c->updateEquiDistanceRange(currentIndex, typeFilter);

		if(cachedVisibility && children.isEmpty())
			currentIndex++;
        else if(cachedVisibility && !children.isEmpty() && currentIndex == before)
            currentIndex++;

		thisEnd = currentIndex;
	}

	equiDistanceRange = { thisStart, thisEnd };
}

bool DebugSession::ProfileDataSource::ViewComponents::ViewItem::isParentOf(const ViewItem* other) const
{
	if(other == nullptr)
		return false;
		
	if(other->parent == this)
		return true;

	if(other->parent != nullptr)
		return isParentOf(other->parent.get());

	return false;
}

void DebugSession::ProfileDataSource::ViewComponents::ViewItem::setUseEquiDistance(bool shouldUseEquiDistance,
                                                                                   VoiceBitMap<32> typeFilter)
{
	if(useEquiDistance != shouldUseEquiDistance)
	{
		useEquiDistance = shouldUseEquiDistance;

		callAllRecursive([shouldUseEquiDistance](ViewItem& vi){ vi.useEquiDistance = shouldUseEquiDistance; return false; });

		int currentEquiDistanceIndex = 0;
		updateEquiDistanceRange(currentEquiDistanceIndex, typeFilter);
	}
}

DebugSession::ProfileDataSource::TimeDomain DebugSession::ProfileDataSource::ViewComponents::ViewItem::
getPreferredDomain() const
{
	if(runData.empty())
		return TimeDomain::Absolute;
	else if (auto s = runData[0].source)
		return s->preferredDomain;
	else
		return TimeDomain::Absolute;
}

DebugSession::ProfileDataSource::ViewComponents::ViewItem::Ptr DebugSession::ProfileDataSource::ViewComponents::ViewItem
::createFromProfileInfo(ProfileInfoBase::Ptr info)
{
	auto mode = info->getRoot()->getProfileId();

	if(mode == ProfileIds::root)
	{
		auto root = info->getRoot();
		return new ViewItem(root, -1.0f, 0, root->getNumChildElements(), 0);
	}
	if (mode == ProfileIds::multithread)
	{
		return new ViewItem(info.get(), -1.0f, 0, 1, 0);
	}

	jassertfalse;
	return nullptr;
}


}
