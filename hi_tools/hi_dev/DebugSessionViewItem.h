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

struct DebugSession::ProfileDataSource::ViewComponents::ViewItem: public ReferenceCountedObject
{
	using Ptr = ReferenceCountedObjectPtr<ViewItem>;
	using List = ReferenceCountedArray<ViewItem>;

	struct Position
	{
		double xOffset = 0.0;
		double x;
		double length;
	};

	struct DrawContext
	{
		DrawContext() = default;

		VoiceBitMap<32>* typeFilter;
		ViewItem::Ptr currentlyHoveredItem = nullptr;
		ViewItem::Ptr currentRoot;
		float scaleFactor = 1.0f;
		float transpose = 0.0f;
		int currentIndex = -1;
		int totalWidth = 0;
		bool showTimeline = false;
		Rectangle<float> fullBounds;
		TimeDomain domain = TimeDomain::Absolute;
		double domainContext = 0.0;
		bool drawChildren = true;
		Range<int> depthRange;
		Component* componentToDrawOn = nullptr;
		Range<double> displayRange;
	};

	ViewItem(ProfileInfoBase* pi, double first_, int depth_, int numRuns, int thisRunIndex=0);

	static Ptr createFromProfileInfo(ProfileInfoBase::Ptr info);

	void addLoopRanges(int thisRunIndex);
	void drawTrack(Graphics& g, const DrawContext& ctx, Ptr target) const;
	String getDuration(int currentIndex, bool, TimeDomain domain, double contextValue) const;

	ViewItem::Ptr callRecursive(const std::function<bool(ViewItem&)>& f, bool callFoldedChildren) const;
	ViewItem::Ptr callAllRecursive(const std::function<bool(ViewItem&)>& f) const { return callRecursive(f, true); }
	ViewItem::Ptr callUnfoldedRecursive(const std::function<bool(ViewItem&)>& f) const { return callRecursive(f, false); }

	void setAsRoot();
	ViewItem::Ptr updateSearch(const String& searchTerm);
	int getMaxDepth(int newDepth, const VoiceBitMap<32>& typeFilters);
	bool isFoldedRecursive() const;

	void calculateAverage(ViewItem* rootItem);
	std::vector<std::pair<Ptr, Ptr>> createTracks();
	ViewItem* getChildForData(const Data& d);
	void addRun(double first, ProfileInfo* p, int runIndex);

	/** Returns the rectangle for the given run (or average if index=-1). */
	Rectangle<float> getBounds(float transpose, bool showTimeline, float scaleFactor, int topItemDepth, int index) const;
	void getNames(StringArray& names);
	ViewItem* isHovered(const MouseEvent& e, float transpose, bool showTimeline, float scaleFactor, int topItemDepth, int currentIndex);
	void paintItem(Graphics& g, Rectangle<float> thisBounds, const DrawContext& ctx);
	bool shouldBeDisplayed(const DrawContext& ctx) const;
	void draw(Graphics& g, const DrawContext& ctx);
	int toggleFold(ViewItem::Ptr itemToFold, VoiceBitMap<32> typeFilter);
	void updateEquiDistanceRange(int& currentIndex, VoiceBitMap<32> typeFilter);
	bool isParentOf(const ViewItem* other) const;
	void setUseEquiDistance(bool shouldUseEquiDistance, VoiceBitMap<32> typeFilter);
	TimeDomain getPreferredDomain() const;

	bool folded = false;
	bool useEquiDistance = false;
	Range<int> equiDistanceRange;

	std::vector<Position> runs;
	std::vector<Data> runData;

	Position avg;
	Identifier mode;

	bool multiThreadMode = false;
    bool hasTracks = false;
    
    bool cachedVisibility = true;
    
	ProfileInfoBase::Ptr infoItem;
	SourceType sourceType = SourceType::Undefined;
	ThreadIdentifier::Type threadType = ThreadIdentifier::Type::Undefined;
	SparseSet<int> loopRanges;
	String name;
	int depth = 1;
	double first = 0.0f;
	Colour c;

	WeakReference<ViewItem> parent;
	ReferenceCountedArray<ViewItem> children;

	bool searchEmpty = true;
	bool matchesSearch = false;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ViewItem);
	JUCE_DECLARE_WEAK_REFERENCEABLE(ViewItem);
};

}
