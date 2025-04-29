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

struct DebugSession::ProfileDataSource::ViewComponents::BaseWithManagerConnection
{
	virtual ~BaseWithManagerConnection();;

	virtual void onNavigation(BaseWithManagerConnection* navigationSource, ViewItem::Ptr totalRoot, ViewItem::Ptr currentRoot) {};

	virtual void onSearchUpdate(const String& searchTerm, bool zoomToFirst) {};

	virtual void onTimeDomainUpdate(TimeDomain newDomain, double context) {};

	virtual void onRunIndexUpdate(int currentRunIndex) = 0;

protected:

	Manager* getManager();

	Manager* initManager();

	

private:

	friend struct SingleThreadPopup;
	WeakReference<Manager> currentManager;

	JUCE_DECLARE_WEAK_REFERENCEABLE(BaseWithManagerConnection);
};

struct DebugSession::ProfileDataSource::ViewComponents::Manager: public Component
{
	struct SearchBar;
	struct NavigationAction;
	struct UndoableTotalRootChange;

	struct Factory: public PathFactory
	{
		Path createPath(const String& url) const override;
	};

	/** Recursively goes up the parent component hierarchy to find the first manager component. */
	static Manager* findManager(Component* c);

	SET_GENERIC_PANEL_ID("ProfilerManager");

	Manager(DebugSession& s);

	static void onRec(Manager& m, bool isEnabled);

#if USE_BACKEND
	void showOptions();
#endif

	static void onOptionUpdate(Manager& m, DebugSession::Options* o)
	{
		if(o != nullptr)
		{
			m.typeFilters.clear();

			for(int i = 0; i < (int)SourceType::numSourceTypes; i++)
				m.typeFilters.setBit(i, true);

			for(auto& ev: o->eventFilter)
				m.typeFilters.setBit((int)ev, false);

			for(auto c: m.connectedComponents)
			{
				if(c != nullptr)
					c->onSearchUpdate(m.lastSearch, false);
			}
		}
	}

	void resized() override;
	void setRecording(bool isRecording);
	void setTimeDomain(TimeDomain newDomain, double newDomainContext);
	void toggleTypeFilter(SourceType source);
	bool showSourceType(SourceType source) const;
	void setPopupMode(bool usePopupMode);
	void setSearchTerm(const String& searchTerm, bool zoomToFirst);

	void setCurrentRunIndex(int newRunIndex)
	{
		if(newRunIndex != runIndex)
		{
			runIndex = newRunIndex;

			for(auto b: connectedComponents)
				b->onRunIndexUpdate(runIndex);
		}
	}

	void zoomToFirstMatch(ViewItem::Ptr itemToZoomTo);
	bool setNewRoot(BaseWithManagerConnection* source, ViewItem::Ptr newRoot, bool useUndoManager);
	bool navigate(BaseWithManagerConnection* source, ViewItem::Ptr newTarget, bool useUndoManager);
	void sendRootChangeMessage(BaseWithManagerConnection* source);
	void setActiveViewer(Viewer* newViewer);

    void paint(Graphics& g) override
    {
        session.initUIThread();
    }
    
	VoiceBitMap<32> typeFilters;

	TimeDomain currentDomain;
	double domainContext;
	String lastSearch;

	WeakReference<Viewer> currentViewer;

	Array<WeakReference<BaseWithManagerConnection>> connectedComponents;

	ViewItem::Ptr totalRoot;
	ViewItem::Ptr currentRoot;
	
	DebugSession& session;
	Factory f;
	HiseShapeButton openButton, saveButton, recordButton, trimButton, moreButton;

	HiseShapeButton undoButton, redoButton;
	ScopedPointer<Component> searchBar;
	int runIndex = -1;

	UndoManager um;

	JUCE_DECLARE_WEAK_REFERENCEABLE(Manager);
};



}
