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



struct DebugSession::ProfileDataSource::ViewComponents::Manager::SearchBar: public Component,
                                                                            public TextEditor::Listener
{
	SearchBar():
	  clearButton("clear", nullptr, f)
	{
		
		addAndMakeVisible(editor);
		addAndMakeVisible(clearButton);
		clearButton.setVisible(false);
		clearButton.onClick = [this](){ editor.setText({}, true); };
		clearButton.setColours(Colours::black.withAlpha(0.3f), Colours::black.withAlpha(0.5f), Colours::black.withAlpha(0.7f));
		editor.setTextToShowWhenEmpty("Search timeline...", Colours::black.withAlpha(0.4f));
		editor.setEscapeAndReturnKeysConsumed(true);
		editor.setSelectAllWhenFocused(true);
		GlobalHiseLookAndFeel::setTextEditorColours(editor);
		searchIcon = f.createPath("search");
		editor.setSelectAllWhenFocused(true);
		editor.addListener(this);
	}

	void textEditorTextChanged(TextEditor& te) override
	{
		findParentComponentOfClass<Manager>()->setSearchTerm(te.getText(), false);
		clearButton.setVisible(!te.isEmpty());
	}

	void textEditorEscapeKeyPressed(TextEditor& te) override
	{
		te.setText({}, dontSendNotification);
		clearButton.setVisible(false);
	}

	void textEditorReturnKeyPressed(TextEditor& te) override
	{
		auto m = findParentComponentOfClass<Manager>();
		m->setSearchTerm(te.getText(), true);
		clearButton.setVisible(!te.isEmpty());
	}

	void paint(Graphics& g) override
	{
		g.setColour(Colours::white.withAlpha(0.4f));
		g.fillPath(searchIcon);
	}

	void resized()
	{
		auto b = getLocalBounds();
		PathFactory::scalePath(searchIcon, b.removeFromLeft(b.getHeight()).reduced(4).toFloat());
		editor.setBounds(b.reduced(1));
		clearButton.setBounds(b.removeFromRight(b.getHeight()).reduced(5));
	}

	Factory f;
	Path searchIcon;
	TextEditor editor;
	HiseShapeButton clearButton;
	
};

struct DebugSession::ProfileDataSource::ViewComponents::Manager::NavigationAction: public UndoableAction
{
	NavigationAction(Manager& v, BaseWithManagerConnection* source_, ViewItem::Ptr old, ViewItem::Ptr n):
	  parent(v),
	  source(source_),
	  oldRoot(old),
	  newRoot(n)
	{};

	bool perform() override
	{
		if(newRoot != nullptr)
			return parent.navigate(source, newRoot, false);

		return false;
	}

	bool undo() override
	{
		if(newRoot != nullptr)
			return parent.navigate(source, oldRoot, false);

		return false;
	}

	WeakReference<BaseWithManagerConnection> source;
	Manager& parent;
	ViewItem::Ptr oldRoot;
	ViewItem::Ptr newRoot;
};

struct DebugSession::ProfileDataSource::ViewComponents::Manager::UndoableTotalRootChange: public UndoableAction
{
	UndoableTotalRootChange(BaseWithManagerConnection* source_, Manager& p, ViewItem::Ptr newRoot_):
	  UndoableAction(),
	  parent(p),
	  source(source_),
	  oldRoot(p.totalRoot),
	  newRoot(newRoot_)
	{}

	bool perform() override
	{
		return parent.setNewRoot(source, newRoot.get(), false);
	}

	bool undo() override
	{
		return parent.setNewRoot(source, oldRoot.get(), false);
	}

	Manager& parent;
	WeakReference<BaseWithManagerConnection> source;
	ViewItem::Ptr newRoot;
	ViewItem::Ptr oldRoot;
};

bool DebugSession::ProfileDataSource::ViewComponents::Manager::setNewRoot(BaseWithManagerConnection* source,
	ViewItem::Ptr newRoot, bool useUndoManager)
{
	if(newRoot != totalRoot)
	{
		if(useUndoManager)
		{
			um.beginNewTransaction();
			um.perform(new UndoableTotalRootChange(source, *this, newRoot));
		}
		else
		{
			totalRoot = newRoot;
			currentRoot = totalRoot;
			sendRootChangeMessage(source);
		}
	}

	return true;
}

bool DebugSession::ProfileDataSource::ViewComponents::Manager::navigate(BaseWithManagerConnection* source,
                                                                        ViewItem::Ptr newTarget, bool useUndoManager)
{
	if(newTarget != currentRoot)
	{
		if(useUndoManager)
		{
			um.beginNewTransaction();
			um.perform(new NavigationAction(*this, source, currentRoot, newTarget));
		}
		else
		{
			currentRoot = newTarget;
			sendRootChangeMessage(source);
				
		}
	}

	return true;
}

void DebugSession::ProfileDataSource::ViewComponents::Manager::sendRootChangeMessage(BaseWithManagerConnection* source)
{
	for(int i = 0; i < connectedComponents.size(); i++)
	{
		auto b = connectedComponents[i];
		if(b != nullptr)
			b->onNavigation(source, totalRoot, currentRoot);
	}
}

void DebugSession::ProfileDataSource::ViewComponents::Manager::setActiveViewer(Viewer* newViewer)
{
	if(newViewer != currentViewer || newViewer == nullptr)
	{
		currentViewer = newViewer;

		if(currentViewer != nullptr)
		{
			totalRoot = currentViewer->originalRoot;
			currentRoot = currentViewer->rootItem;
		}
		else
		{
			totalRoot = nullptr;
			currentRoot = nullptr;
		}

		sendRootChangeMessage(currentViewer.get());
	}
}

DebugSession::ProfileDataSource::ViewComponents::BaseWithManagerConnection::~BaseWithManagerConnection()
{
	if(auto m = currentManager.get())
		m->connectedComponents.removeAllInstancesOf(this);
}

DebugSession::ProfileDataSource::ViewComponents::Manager* DebugSession::ProfileDataSource::ViewComponents::
BaseWithManagerConnection::getManager()
{
	initManager();
	return currentManager.get();
}

DebugSession::ProfileDataSource::ViewComponents::Manager* DebugSession::ProfileDataSource::ViewComponents::
BaseWithManagerConnection::initManager()
{
	if(currentManager == nullptr)
	{
		currentManager = Manager::findManager(dynamic_cast<Component*>(this));

		if(currentManager != nullptr)
			currentManager->connectedComponents.addIfNotAlreadyThere(this);

		return currentManager;
	}

	return nullptr;
}

Path DebugSession::ProfileDataSource::ViewComponents::Manager::Factory::createPath(const String& url) const
{
	Path p;

	LOAD_EPATH_IF_URL("open", SampleMapIcons::pasteSamples);
	LOAD_EPATH_IF_URL("save", SampleMapIcons::copySamples);
	LOAD_EPATH_IF_URL("delete", SampleMapIcons::deleteSamples);
	LOAD_EPATH_IF_URL("init", HnodeIcons::jit);
	LOAD_EPATH_IF_URL("trim", EditorIcons::cropIcon);
	LOAD_EPATH_IF_URL("undo", EditorIcons::backIcon);
	LOAD_EPATH_IF_URL("redo", EditorIcons::forwardIcon);
	LOAD_EPATH_IF_URL("home", MainToolbarIcons::home);
	LOAD_EPATH_IF_URL("peak", EditorIcons::peakIcon);
	LOAD_EPATH_IF_URL("favorite", EditorIcons::favoriteIcon);
	LOAD_EPATH_IF_URL("search", EditorIcons::searchIcon);
	LOAD_EPATH_IF_URL("clear", EditorIcons::clearTextEditorIcon);
	LOAD_EPATH_IF_URL("more", ColumnIcons::threeDots);

	if(url == "search")
		p.applyTransform(AffineTransform::rotation(float_Pi));

	if(url == "peak")
		p.applyTransform(AffineTransform::scale(1.0f, 2.0f));

	if (url == "record")
		p.addEllipse({ 0.0f, 0.0f, 1.0f, 1.0f });

	return p;
}

DebugSession::ProfileDataSource::ViewComponents::Manager* DebugSession::ProfileDataSource::ViewComponents::Manager::
findManager(Component* c)
{
	Manager* m = nullptr;

	while(c != nullptr)
	{
		Component::callRecursive<Manager>(c, [&](Manager* cm)
		{
			m = cm;
			return true;
		});

		if(m != nullptr)
			return m;

		c = c->getParentComponent();
	}

	return m;
}

DebugSession::ProfileDataSource::ViewComponents::Manager::Manager(DebugSession& s):
	session(s),
	openButton("open", nullptr, f),
	saveButton("save", nullptr, f),
	recordButton("record", nullptr, f),
	trimButton("trim", nullptr, f),
	moreButton("more", nullptr, f),
	
	undoButton("undo", nullptr, f),
	redoButton("redo", nullptr, f),
	searchBar(new SearchBar()) 
{
	addAndMakeVisible(openButton);
	addAndMakeVisible(saveButton);
	addAndMakeVisible(recordButton);
	addAndMakeVisible(moreButton);
	addAndMakeVisible(trimButton);
	addAndMakeVisible(undoButton);
	addAndMakeVisible(redoButton);
	addAndMakeVisible(searchBar);

    openButton.setTooltip("Import a profile session from the clipboard");
    saveButton.setTooltip("Copy the current profiling session to the clipboard");
    recordButton.setTooltip("Toggle the profiling recording session");
    moreButton.setTooltip("Adjust the recording settings");
    undoButton.setTooltip("Navigate back");
    redoButton.setTooltip("Navigate forward");
    trimButton.setTooltip("Copy the currently displayed range into a new profiling session");
    
	recordButton.onColour = Colour(HISE_ERROR_COLOUR);
	recordButton.setToggleModeWithColourChange(true);

	undoButton.onClick = [this](){ um.undo(); };
	redoButton.onClick = [this](){ um.redo(); };

#if USE_BACKEND
	moreButton.onClick = BIND_MEMBER_FUNCTION_0(Manager::showOptions);
#endif

	trimButton.onClick = [this]()
	{
		if(auto r = currentRoot)
		{
			if(currentViewer != nullptr)
				currentViewer->trimToCurrentRange();
		}
	};

	recordButton.onClick = [this]()
	{
		if(recordButton.getToggleState())
			this->session.startRecording(20000, &this->session);
		else
			this->session.stopRecording();
	};

	saveButton.onClick = [this]()
	{
		if(auto r = currentRoot->infoItem)
		{
			SystemClipboard::copyTextToClipboard(r->toBase64());
		}
	};

	openButton.onClick = [this]()
	{
		auto b64 = SystemClipboard::getTextFromClipboard();

		if(b64.startsWith("HiseProfile"))
		{
			auto n = ProfileDataSource::MultiThreadSession::fromBase64(b64, session);
			ProfileInfoBase::Ptr s = n.get();
			session.recordingFlushBroadcaster.sendMessage(sendNotificationSync, s);
		}
	};

	session.syncRecordingBroadcaster.addListener(*this, onRec, false);

	session.optionBroadcaster.addListener(*this, onOptionUpdate);
}

void DebugSession::ProfileDataSource::ViewComponents::Manager::resized()
{
	auto menuBar = getLocalBounds();

	searchBar->setBounds(menuBar.removeFromRight(300).reduced(5));

	int ButtonMargin = 10;
	undoButton.setBounds(menuBar.removeFromLeft(MultiViewerMenuBarHeight).reduced(ButtonMargin));
	redoButton.setBounds(menuBar.removeFromLeft(MultiViewerMenuBarHeight).reduced(ButtonMargin));
    menuBar.removeFromLeft(ButtonMargin);
    recordButton.setBounds(menuBar.removeFromLeft(MultiViewerMenuBarHeight).reduced(ButtonMargin));
	moreButton.setBounds(menuBar.removeFromLeft(MultiViewerMenuBarHeight).reduced(ButtonMargin));
	menuBar.removeFromLeft(ButtonMargin);
	saveButton.setBounds(menuBar.removeFromLeft(MultiViewerMenuBarHeight).reduced(ButtonMargin));
	openButton.setBounds(menuBar.removeFromLeft(MultiViewerMenuBarHeight).reduced(ButtonMargin));
    trimButton.setBounds(menuBar.removeFromLeft(MultiViewerMenuBarHeight).reduced(ButtonMargin));
}

void DebugSession::ProfileDataSource::ViewComponents::Manager::setRecording(bool isRecording)
{
	recordButton.setToggleStateAndUpdateIcon(isRecording, true);
}

void DebugSession::ProfileDataSource::ViewComponents::Manager::setTimeDomain(TimeDomain newDomain,
                                                                             double newDomainContext)
{
	if(currentDomain != newDomain)
	{
		currentDomain = newDomain;
		domainContext = newDomainContext;

		for(auto b: connectedComponents)
		{
			b->onTimeDomainUpdate(newDomain, newDomainContext);
		}
	}
}

void DebugSession::ProfileDataSource::ViewComponents::Manager::toggleTypeFilter(SourceType source)
{
	typeFilters.setBit((int)source, !typeFilters[(int)source]);

	for(auto c: connectedComponents)
	{
		if(c != nullptr)
			c->onSearchUpdate(lastSearch, false);
	}
}

bool DebugSession::ProfileDataSource::ViewComponents::Manager::showSourceType(SourceType source) const
{
	return !typeFilters[(int)source];
}

void DebugSession::ProfileDataSource::ViewComponents::Manager::setPopupMode(bool usePopupMode)
{
	recordButton.setVisible(!usePopupMode);
	trimButton.setVisible(!usePopupMode);
	moreButton.setVisible(!usePopupMode);
}

void DebugSession::ProfileDataSource::ViewComponents::Manager::onRec(Manager& m, bool isEnabled)
{
	SafeAsyncCall::call<Manager>(m, [isEnabled](Manager& m)
	{
		m.setRecording(isEnabled);
	});
}

void DebugSession::ProfileDataSource::ViewComponents::Manager::setSearchTerm(const String& searchTerm, bool zoomToFirst)
{
	lastSearch = searchTerm;

	for(auto b: connectedComponents)
	{
		if(b != nullptr)
			b->onSearchUpdate(searchTerm, zoomToFirst);
	}
}

void DebugSession::ProfileDataSource::ViewComponents::Manager::zoomToFirstMatch(ViewItem::Ptr itemToZoomTo)
{
	if(currentViewer != nullptr)
	{
		currentViewer->zoomToItem(itemToZoomTo);
	}
}

}
