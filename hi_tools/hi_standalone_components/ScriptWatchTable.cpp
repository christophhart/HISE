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

juce::var ScriptWatchTable::getColumnVisiblilityData() const
{
	Array<var> visibleColumns;

	auto& header = table->getHeader();
	auto numColumns = header.getNumColumns(true);

	for (int i = 0; i < numColumns; i++)
	{
		auto id = header.getColumnIdOfIndex(i, true);
		visibleColumns.add(header.getColumnName(id));
	}

	return var(visibleColumns);
}

void ScriptWatchTable::restoreColumnVisibility(const var& d)
{
	if (auto a = d.getArray())
	{
		auto& header = table->getHeader();
		auto numColumns = header.getNumColumns(false);

		for (int i = 0; i < numColumns; i++)
			header.setColumnVisible(i, false);

		for (const auto& v : *a)
		{
			auto name = v.toString();

			for (int i = 0; i < numColumns; i++)
			{
				auto id = header.getColumnIdOfIndex(i, false);

				if (header.getColumnName(id) == name)
				{
					header.setColumnVisible(id, true);
					break;
				}
			}
		}
	}
}

ScriptWatchTable::ScriptWatchTable() :
	rebuilder(this),
	ApiComponentBase(nullptr),
	refreshButton("refresh", this, factory),
	expandButton("expand", this, factory),
	menuButton("menu", this, factory),
	pinButton("pinned", this, factory),
	viewInfo(*this)
{
	setOpaque(true);

	setName(getHeadline());

	addAndMakeVisible(refreshButton);
	addAndMakeVisible(expandButton);
	addAndMakeVisible(menuButton);
	addAndMakeVisible(pinButton);

	pinButton.setToggleModeWithColourChange(true);
	expandButton.setToggleModeWithColourChange(true);

	pinButton.setTooltip("Show only pinned values");
	expandButton.setTooltip("Expand all values");
	refreshButton.setTooltip("Rebuild all list items");

    addAndMakeVisible (table = new TableListBox());
    table->setModel (this);
	table->getHeader().setLookAndFeel(&laf);
	table->getHeader().setSize(getWidth(), 22);
    table->setOutlineThickness (0);
	table->getViewport()->setScrollBarsShown(true, false, false, false);
    
	table->setMultipleSelectionEnabled(true);
	//table->setClickingTogglesRowSelection(true);

	table->setColour(ListBox::backgroundColourId, JUCE_LIVE_CONSTANT_OFF(Colour(0x04ffffff)));

	table->getHeader().addColumn("", Expand, 30, 30, 30);
	table->getHeader().addColumn("Type", Type, 30, 30, 30);
	table->getHeader().addColumn("Data Type", DataType, 100, 100, -1);
	table->getHeader().addColumn("Name", Name, 100, 60, -1);
	table->getHeader().addColumn("Value", Value, 180, 100, -1);

	table->getHeader().setStretchToFitActive(true);
	

	table->addMouseListener(this, true);

	

	addAndMakeVisible(fuzzySearchBox = new TextEditor());

	GlobalHiseLookAndFeel::setTextEditorColours(*fuzzySearchBox);

	fuzzySearchBox->addListener(this);
	

	rebuildLines();
}


ScriptWatchTable::~ScriptWatchTable()
{
	rebuilder.cancelPendingUpdate();

	rootValues.clear();
	filteredFlatList.clear();

	table = nullptr;
}

void ScriptWatchTable::timerCallback()
{
	if (table != nullptr && isShowing())
	{
		refreshChangeStatus();
	}
		
	if (fullRefreshFactor != 0)
	{
		if (++fullRefreshCounter >= fullRefreshFactor)
		{
			fullRefreshCounter = 0;
			rebuildLines();
		}
	}
}

bool ScriptWatchTable::keyPressed(const KeyPress& k)
{
	if (k == KeyPress::escapeKey)
	{
		table->deselectAllRows();
	}

	return false;
}

int ScriptWatchTable::getNumRows() 
{
	return filteredFlatList.size();
};

void ScriptWatchTable::providerCleared()
{
    rootValues.clear();
    filteredFlatList.clear();
    
    SafeAsyncCall::call<ScriptWatchTable>(*this, [](ScriptWatchTable& t)
    {
        t.table->updateContent();
        t.repaint();
    });
}

void ScriptWatchTable::rebuildLines()
{
	bool rootWasFound = viewInfo.currentRoot.isEmpty();

	rootValues.clear();
	filteredFlatList.clear();

	if (auto pr = getProviderBase())
	{
		for (int i = 0; i < pr->getNumDebugObjects(); i++)
		{
			auto p = pr->getDebugInformation(i);
			
			if (p != nullptr && p->isWatchable())
			{
				rootValues.add(new Info(p));
				rootValues.getLast()->forEachExpandedChildren([this, &rootWasFound](Info::Ptr c)
				{
					rootWasFound |= viewInfo.isRoot(c);

					if (viewInfo.is(c, ViewInfo::Expanded))
						c->expanded = true;

					return false;
				}, true);
			}
		}

		if (!rootWasFound)
			viewInfo.currentRoot = {};

		applySearchFilter();
	}
}

bool ScriptWatchTable::addToFilterListRecursive(Info::Ptr i)
{
	if (viewInfo.is(i, ViewInfo::Pinned))
	{
		i->forEachExpandedChildren([this](Info::Ptr p)
		{
			if (!viewInfo.matchesRoot(p))
				return false;

			filteredFlatList.addIfNotAlreadyThere(p);
			return false;
		}, false);
	}

	return false;
}



hise::ScriptWatchTable::Info::List ScriptWatchTable::getSelectedInfos() const
{
	Info::List l;

	auto s = table->getSelectedRows();

	for (int r = 0; r < s.getNumRanges(); r++)
	{
		auto sr = s.getRange(r);

		for (int i = sr.getStart(); i < sr.getEnd(); i++)
			l.add(filteredFlatList[i]);
	}

	return l;
}

void ScriptWatchTable::applySearchFilter()
{
	filteredFlatList.clear();

	if (viewInfo.isAny(ViewInfo::Pinned))
	{
		for (auto l : rootValues)
			l->forEachExpandedChildren(BIND_MEMBER_FUNCTION_1(ScriptWatchTable::addToFilterListRecursive), true);
	}

	if (!viewInfo.is(ViewInfo::OnlyPinned))
	{
		const String filterText = fuzzySearchBox->getText();

		for (auto l : rootValues)
		{
			l->forEachExpandedChildren([this, filterText](Info::Ptr i)
			{
				if (!viewInfo.isTypeAllowed(i))
					return false;

				if (!viewInfo.matchesRoot(i))
					return false;

				if (filterText.isEmpty() ||
					i->name.containsIgnoreCase(filterText) ||
					i->dataType.containsIgnoreCase(filterText))
					filteredFlatList.add(i);

				return false;
			}, filterText.isNotEmpty() || viewInfo.is(ViewInfo::AllExpanded));
		}
	}

	table->updateContent();

	if (resizeOnChange)
	{
		auto preferredHeight = 24 + table->getHeaderHeight() + table->getNumRows() * table->getRowHeight();

		setSize(getWidth(), preferredHeight);
	}

	repaint();
}


void ScriptWatchTable::refreshChangeStatus()
{
	if(auto provider = getProviderBase())
	{
		BigInteger lastChanged = changed;
		changed = 0;

		for (int i = 0; i < filteredFlatList.size(); i++)
		{
			auto info = filteredFlatList[i];
			
			if (info->checkValueChange())
			{
				if (logFunction && viewInfo.is(info, ViewInfo::Debugged))
				{
					String m;
					m << info->name << ": ";
					m << info->getValue();
					logFunction(m);
				}

				changed.setBit(i, true);
			}
		}

		if (lastChanged != changed || changed != 0)
			repaint();
	}
};


void ScriptWatchTable::mouseDown(const MouseEvent& e)
{
	if (e.eventComponent == &table->getHeader())
		return;

	auto pos = e.getEventRelativeTo(table).getPosition();

	auto idx = table->getRowContainingPosition(pos.x, pos.y);

	if ( pos.getX() < 30)
	{
		if (auto f = filteredFlatList[idx])
		{
			auto shouldBe = !f->expanded;

			f->expanded = shouldBe;

			viewInfo.toggle(f, ViewInfo::Expanded);

			applySearchFilter();
			repaint();
		}

		return;
	}

	if (e.mods.isRightButtonDown())
	{
		PopupLookAndFeel plaf;
		PopupMenu m;
		m.setLookAndFeel(&plaf);

		auto selection = getSelectedInfos();

		bool somethingSelected = !selection.isEmpty();

		auto idx = table->getRowContainingPosition(pos.x, pos.y);

		Component* popup = nullptr;

		if (auto f = filteredFlatList[idx])
		{
			if (auto info = f->source)
			{
				
				popup = info->createPopupComponent(e, table);
			}
		}

		m.addItem(1000 + 9000, "View in popup", popup != nullptr, false);

		m.addItem(1000 + 9001, "Set as root", somethingSelected, somethingSelected ? viewInfo.isRoot(selection.getFirst()): false);

		m.addSeparator();

		m.addItem(1000 + ViewInfo::Pinned * 10, "Pin value", somethingSelected, somethingSelected ? viewInfo.is(selection.getFirst(), ViewInfo::Pinned) : false);
		m.addItem(1000 + ViewInfo::Pinned * 10 + 1, "Clear all pinned values", viewInfo.isAny(ViewInfo::Pinned));

		m.addSeparator();

		m.addItem(1000 + ViewInfo::Debugged * 10, "Log value changes", somethingSelected, somethingSelected ? viewInfo.is(selection.getFirst(), ViewInfo::Debugged) : false);
		m.addItem(1000 + ViewInfo::Debugged * 10 + 1, "Clear all value changes", viewInfo.isAny(ViewInfo::Debugged));

		auto r = m.show() - 1000;

		if (r < 0)
			return;

		if (r == 9001)
		{
			viewInfo.toggleRoot(selection.getFirst());
			return;
		}

		if (r == 9000)
		{
			auto e2 = e.getEventRelativeTo(this);

			Point<int> s(getWidth() / 2, e2.getMouseDownY() + 16);

			if (popupFunction)
				popupFunction(popup, table, s);
			else
				jassertfalse;

			return;
		}

		auto vp = (ViewInfo::ViewProperty)(r / 10);
		bool clear = r % 10 != 0;

		if (clear)
		{
			table->deselectAllRows();

			viewInfo.clear(vp);
		}
		else
		{
			auto l = getSelectedInfos();

			table->deselectAllRows();

			for (auto info : l)
				viewInfo.toggle(info, vp);

			applySearchFilter();
		}
	}
	else if (idx == -1)
	{
		table->deselectAllRows();
	}
}


void ScriptWatchTable::mouseDoubleClick(const MouseEvent &e)
{
	if (auto info = getDebugInformationForRow(table->getSelectedRow(0)))
	{
		info->doubleClickCallback(e, this);
	}
}

void ScriptWatchTable::paint(Graphics &g)
{
	g.setColour(Colour(0xff353535));
	g.fillRect(0.0f, 0.0f, (float)getWidth(), 25.0f);

	g.setGradientFill(ColourGradient(Colours::black.withAlpha(0.5f), 0.0f, 25.0f,
		Colours::transparentBlack, 0.0f, 30.0f, false));
	g.fillRect(0.0f, 25.0f, (float)getWidth(), 25.0f);

	g.setColour(Colour(0xFF3D3D3D));

	g.setColour(bgColour);
	g.fillRect(0, 25, getWidth(), getHeight());

	g.setColour(Colours::white.withAlpha(0.6f));

	

	Path path = factory.createPath("search");
	path.applyTransform(AffineTransform::rotation(float_Pi));

	const float xOffset = 0.0f;

	path.scaleToFit(xOffset + 4.0f, 4.0f, 16.0f, 16.0f, true);

	g.fillPath(path);

	if (filteredFlatList.isEmpty())
	{
		g.setFont(GLOBAL_FONT());
		g.setColour(Colours::white.withAlpha(0.4f));

		String errorMessage;

		if (viewInfo.is(ViewInfo::OnlyPinned))
			errorMessage = "No pinned values";
		else if (rootValues.isEmpty())
			errorMessage = "No data values";
		else if (fuzzySearchBox->getText().isNotEmpty())
			errorMessage = "No matching search results";
		else
			errorMessage = "No data";

		g.drawText(errorMessage, table->getBoundsInParent().toFloat().removeFromTop(80.0f), Justification::centred);
	}
}

void ScriptWatchTable::paintOverChildren(Graphics& g)
{
	if (currentTooltip != nullptr)
		currentTooltip->draw(g);
}

void ScriptWatchTable::buttonClicked(Button* b)
{
	PopupLookAndFeel plaf;
	PopupMenu m;
	m.setLookAndFeel(&plaf);

	if (b == &menuButton)
	{
		PopupMenu sub1, sub2;



		sub1.addItem(50, "50 ms", true, timerspeed == 50);
		sub1.addItem(100, "100 ms", true, timerspeed == 100);
		sub1.addItem(500, "500 ms", true, timerspeed == 500);
		sub1.addItem(1000, "1000 ms", true, timerspeed == 1000);
		sub1.addItem(2000, "2000 ms", true, timerspeed == 2000);

		sub2.addItem(49, "Only on compilation", true, fullRefreshFactor == 0);
		sub2.addItem(501, "50 ms", timerspeed <= 50, fullRefreshFactor != 0 && timerspeed / fullRefreshFactor == 50);
		sub2.addItem(101, "100 ms", timerspeed <= 100, fullRefreshFactor != 0 && timerspeed / fullRefreshFactor == 100);
		sub2.addItem(501, "500 ms", timerspeed <= 500, fullRefreshFactor != 0 && timerspeed / fullRefreshFactor == 500);
		sub2.addItem(1001, "1000 ms", timerspeed <= 1000, fullRefreshFactor != 0 && timerspeed / fullRefreshFactor == 1000);
		sub2.addItem(2001, "2000 ms", timerspeed <= 2000, fullRefreshFactor != 0 && timerspeed / fullRefreshFactor == 2000);
		sub2.addItem(10001, "10 seconds", timerspeed <= 2000, fullRefreshFactor != 0 && timerspeed / fullRefreshFactor == 2000);
		
		m.addSectionHeader("Refresh Rate");
		m.addSubMenu("Value Refresh rate", sub1);
		m.addSubMenu("List refresh rate", sub2);

		m.addSeparator();

		m.addSectionHeader("Displayed Data Types");

		viewInfo.addDataTypeToPopup(m);

		m.addSeparator();

		m.addSectionHeader("Load / Save Configuration");

		m.addItem(4, "Reset view settings");
		m.addItem(1, "Export view settings");
		m.addItem(2, "Import view settings");

		auto r = m.show();

		if (viewInfo.performPopup(r))
			return;

		if (r == 1)
		{
			FileChooser fc("Save watch table configuration", File(), "*.json");

			if (fc.browseForFileToSave(true))
				fc.getResult().replaceWithText(JSON::toString(viewInfo.exportViewSettings()));
		}
		if (r == 2)
		{
			FileChooser fc("Load watch table configuration", File(), "*.json");
			if (fc.browseForFileToOpen())
			{
				auto d = JSON::parse(fc.getResult().loadFileAsString());
				viewInfo.importViewSettings(d);
			}
		}
		if (r == 4)
		{
			viewInfo.clear();
			return;
		}
		if (r == 49)
		{
			fullRefreshFactor = 0;
			refreshTimer();
		}
		if (r >= 50)
		{
			auto fullRefresh = r % 10 != 0;

			if (fullRefresh)
				fullRefreshFactor = roundToInt((float)r / (float)timerspeed);
			else
				timerspeed = r;

			refreshTimer();
		}
	}
	if (b == &expandButton)
	{
		viewInfo.toggle(ViewInfo::AllExpanded);
	}
	if (b == &refreshButton)
	{
		rebuildLines();
	}
	if (b == &pinButton)
	{
		viewInfo.set(ViewInfo::OnlyPinned, pinButton.getToggleState());
	}
}

void ScriptWatchTable::setViewDataTypes(const StringArray& names, const Array<int>& typeIds)
{
	for (int i = 0; i < names.size(); i++)
	{
		ViewInfo::ViewDataType t;
		t.name = names[i];
		t.on = true;
		t.typeId = typeIds[i];
		viewInfo.viewDataTypes.add(t);
	}
}

void ScriptWatchTable::mouseExit(const MouseEvent& e)
{
	auto pos = e.getEventRelativeTo(this).getPosition();

	if (!getLocalBounds().contains(pos))
	{
		currentTooltip = nullptr;
		repaint();
	}
}

String ScriptWatchTable::getCellTooltip (int rowNumber, int columnId)
{
    if(useParentTooltips)
        return getTextForColumn((ColumnId)columnId, filteredFlatList[rowNumber], true);
    else
        return {};
}

void ScriptWatchTable::mouseMove(const MouseEvent& e)
{
    if(useParentTooltips)
        return;
    
	auto ee = e.getEventRelativeTo(table);
	auto p = ee.getPosition();
	auto r = table->getRowContainingPosition(p.getX(), p.getY());

	for (int c = 0; c < (int)ColumnId::numColumns; c++)
	{
		auto pos = table->getCellPosition(c, r, true);

		if (pos.contains(p))
		{
			Point<int> tp(c, r);

			if (currentTooltip != nullptr)
			{
				if (currentTooltip->tablePos != tp)
				{
					currentTooltip = nullptr;
					repaint();
				}
				else
					return;
			}
			
			auto s = getTextForColumn((ColumnId)c, filteredFlatList[r], true);

			auto w = GLOBAL_MONOSPACE_FONT().withHeight((float)table->getRowHeight() * 0.7f).getStringWidth(s);

            if(useParentTooltips)
            {
                

                setTooltip(s);
                return;
            }
			else if ((pos.getWidth() - 30) < w)
			{
                currentTooltip = new TooltipInfo(*this);
                currentTooltip->cellPos = getLocalArea(table, pos);
                currentTooltip->tablePos = { c, r };
                currentTooltip->t = s;
			}

			repaint();
			return;
		}
	}
}

String ScriptWatchTable::getTextForColumn(ScriptWatchTable::ColumnId columnId, Info::Ptr info, bool getFullText)
{
	if (info == nullptr)
		return {};

	String text;

	if (columnId == ColumnId::DataType)
		text << info->dataType;
	else if (columnId == ColumnId::Name)
	{
		text << info->name;

		if (viewInfo.is(info, ViewInfo::Pinned) || getFullText)
			text = text.trimStart();
		else if (text.containsChar('.'))
		{
			auto t = text.fromLastOccurrenceOf(".", false, false);

			text = "";
			for (int i = 0; i < info->level; i++)
				text << ' ';

			text << t;
		}

	}

	else
		text << info->getValue();

	if (!getFullText && columnId == ColumnId::Value && viewInfo.is(info, ViewInfo::Debugged))
		text << "*";

	return text;
}

DebugInformationBase::Ptr ScriptWatchTable::getDebugInformationForRow(int rowIndex)
{
	if (auto r = filteredFlatList[rowIndex])
		return r->source;

	return nullptr;
}

void ScriptWatchTable::setHolder(ApiProviderBase::Holder* h)
{
	deregisterAtHolder();
	holder = h;
	registerAtHolder();

	setName(getHeadline());

	if(holder.get() != nullptr)
	{
		auto fontSize = holder->getCodeFontSize();

		table->setRowHeight(fontSize / 0.7f);

		rebuildLines();
		startTimer(400);
	}
	else
	{
		rootValues.clear();
		filteredFlatList.clear();
		
		table->updateContent();
		stopTimer();
		repaint();
	}

	if(getParentComponent() != nullptr) 
		getParentComponent()->repaint();

}

void ScriptWatchTable::textEditorTextChanged(TextEditor& textEditor)
{
	applySearchFilter();
}

void ScriptWatchTable::setResizeOnChange(bool shouldResize)
{
	resizeOnChange = shouldResize;
}

void ScriptWatchTable::updateFontSize(ScriptWatchTable& t, float newSize)
{
	t.table->setRowHeight(roundToInt(newSize / 0.7f));
}

void ScriptWatchTable::providerWasRebuilt()
{
	rebuildLines();
}

void ScriptWatchTable::setLogFunction(const std::function<void(const String&)>& f)
{
	logFunction = f;
}

void ScriptWatchTable::setPopupFunction(const std::function<void(Component*, Component*, Point<int>)>& pf)
{
	popupFunction = pf;
}

var ScriptWatchTable::getStateObject() const
{ return viewInfo.exportViewSettings(); }

void ScriptWatchTable::fromStateObject(const var& o)
{ viewInfo.importViewSettings(o); }

ScriptWatchTable::TooltipInfo::TooltipInfo(ScriptWatchTable& parent_):
	parent(parent_)
{
	startTimer(800);
}

void ScriptWatchTable::TooltipInfo::timerCallback()
{
	ready = true;
	parent.repaint();
	stopTimer();
}

void ScriptWatchTable::refreshTimer()
{
	startTimer(timerspeed);
	fullRefreshCounter = 0;
}

ScriptWatchTable::Rebuilder::Rebuilder(ScriptWatchTable* parent_):
	parent(parent_)
{}

void ScriptWatchTable::Rebuilder::handleAsyncUpdate()
{
	parent->rebuildLines();
}

void ScriptWatchTable::rebuildLinesAsync()
{
	rebuilder.triggerAsyncUpdate();
}

String ScriptWatchTable::Info::getValue()
{
	if (!valueInitialised && source != nullptr)
	{
		value = source->getTextForValue();
		valueInitialised = true;
	}

	return value;
}

bool ScriptWatchTable::Info::checkValueChange()
{
	if (source == nullptr)
		return false;

	const String oldValue = getValue();

	const String currentValue = source->getTextForValue();

	if (oldValue != currentValue)
	{
		value = currentValue;
		return true;
	}
				
	return false;
}

ScriptWatchTable::ViewInfo::ViewInfo(ScriptWatchTable& p): parent(p)
{}

bool ScriptWatchTable::ViewInfo::isRoot(Info::Ptr info) const
{
	return info->name == currentRoot;
}

bool ScriptWatchTable::ViewInfo::matchesRoot(Info::Ptr info) const
{
	if (currentRoot.isEmpty())
		return true;

	Info* p = info.get();

	while (p != nullptr)
	{
		if (p->name == currentRoot)
			return true;

		p = p->parent;
	}

	return false;
}

void ScriptWatchTable::ViewInfo::toggleRoot(Info::Ptr info)
{
	auto newRoot = info->name;

	if (newRoot == currentRoot)
		currentRoot = {};
	else
		currentRoot = newRoot;

	parent.applySearchFilter();
}

bool ScriptWatchTable::ViewInfo::isTypeAllowed(Info::Ptr info) const
{
	auto t = info->type;

	for (const auto& vt : viewDataTypes)
	{
		if (vt.typeId == t)
			return vt.on;
	}

	return true;
}

bool ScriptWatchTable::ViewInfo::is(Info::Ptr info, ViewProperty p) const
{
	return d[p]->contains(info->name);
}

bool ScriptWatchTable::ViewInfo::isAny(ViewProperty p) const
{
	return !d[p]->isEmpty();
}

bool ScriptWatchTable::ViewInfo::is(ViewStates s) const
{
	return states[s];
}

void ScriptWatchTable::ViewInfo::addDataTypeToPopup(PopupMenu& m)
{
	bool on = false;
	for (const auto& d : viewDataTypes)
	{
		on |= d.on;
		m.addItem(70000 + d.typeId, d.name, true, d.on);
	}
				
	m.addItem(80000, "Toggle all", true, on);
}

bool ScriptWatchTable::ViewInfo::performPopup(int result)
{
	if (result >= 70000)
	{
		for (auto& d : viewDataTypes)
		{
			if (d.typeId == result - 70000 || result == 80000)
			{
				d.on = !d.on;

				if(result != 80000)
					break;
			}
		}

		parent.applySearchFilter();
		return true;
	}

	return false;
				
}

void ScriptWatchTable::ViewInfo::set(ViewStates s, bool v)
{
	states[s] = v;
	parent.applySearchFilter();
}

void ScriptWatchTable::ViewInfo::toggle(ViewStates s)
{
	states[s] = !states[s];
	parent.applySearchFilter();
}

void ScriptWatchTable::ViewInfo::toggle(Info::Ptr info, ViewProperty p)
{
	if (is(info, p))
		d[p]->removeString(info->name);
	else
		d[p]->addIfNotAlreadyThere(info->name);
}

void ScriptWatchTable::ViewInfo::clear(ViewProperty p)
{
	d[p]->clear();
	parent.rebuildLines();
}

void ScriptWatchTable::ViewInfo::clear()
{
	importViewSettings(var());
}

void ScriptWatchTable::ViewInfo::importViewSettings(var data)
{
	debugNames.clear();
	pinnedRows.clear();
	expandedNames.clear();
	currentRoot = {};
			
	for (int i = 0; i < (int)ViewStates::numViewStates; i++)
		states[i] = false;

	for (auto& df : viewDataTypes)
		df.on = true;

	if (auto obj = data.getDynamicObject())
	{
		auto d = obj->getProperty("DebugEntries");
		auto p = obj->getProperty("PinnedEntries");
		auto e = obj->getProperty("ExpandedEntries");
		auto dta = obj->getProperty("DataTypes");

		currentRoot = obj->getProperty("Root").toString();

		if (auto da = d.getArray())
		{
			for (auto& s : *da)
				debugNames.add(s.toString());
		}

		if (auto pa = p.getArray())
		{
			for (auto& s : *pa)
				pinnedRows.add(s.toString());
		}

		if (auto ea = e.getArray())
		{
			for (auto& s : *ea)
				expandedNames.add(s.toString());
		}

		if (auto dt = dta.getArray())
		{
			for (auto& df : viewDataTypes)
				df.on = dt->contains(var(df.name));
		}
	}

	parent.rebuildLines();
}

var ScriptWatchTable::ViewInfo::exportViewSettings() const
{
	auto obj = new DynamicObject();

	Array<var> debugData;
	Array<var> pinnedData;
	Array<var> expandedData;
	Array<var> dataTypeList;

	for (const auto& s : debugNames)
		debugData.add(s);

	for (const auto& s : pinnedRows)
		pinnedData.add(s);

	for (const auto& s : expandedNames)
		expandedData.add(s);

	for (const auto& dt : viewDataTypes)
	{
		if (dt.on)
			dataTypeList.add(dt.name);
	}

	obj->setProperty("Root", currentRoot);
	obj->setProperty("DebugEntries", var(debugData));
	obj->setProperty("PinnedEntries", var(pinnedData));
	obj->setProperty("ExpandedEntries", var(expandedData));
	obj->setProperty("DataTypes", var(dataTypeList));

	return var(obj);
}

void ScriptWatchTable::paintRowBackground (Graphics& g, int rowNumber, int width, int height, bool rowIsSelected) 
{
	if (auto i = filteredFlatList[rowNumber])
	{
		if (viewInfo.is(i, ViewInfo::Pinned))
			g.fillAll(Colour(SIGNAL_COLOUR).withAlpha(0.05f));
	}

	if(rowNumber % 2) g.fillAll(Colours::white.withAlpha(0.01f));

	if (rowIsSelected)
	{
		g.fillAll(Colour(SIGNAL_COLOUR).withAlpha(0.1f));
		Rectangle<float> ar(0.0f, 0.0f, (float)width-1.0f, (float)height - 1.0f);
		g.setColour(Colour(SIGNAL_COLOUR));
		g.drawRect(ar, 1.0f);
	}
        
}

void ScriptWatchTable::selectedRowsChanged(int /*lastRowSelected*/) {};

void ScriptWatchTable::paintCell (Graphics& g, int rowNumber, int columnId,
                int width, int height, bool /*rowIsSelected*/) 
{
	g.setColour(Colours::black.withAlpha(0.1f));

	g.drawHorizontalLine(0, 0.0f, (float)width);
	
	
	g.setColour (Colours::white.withAlpha(.8f));
    g.setFont (GLOBAL_FONT());

	if (auto pr = getProviderBase())
	{
		if (auto info = filteredFlatList[rowNumber])
		{
			String text;

			if (columnId == Expand)
			{
				Rectangle<float> area((float)0.0f, 0.0f, (float)width, (float)height);

				if (!info->children.isEmpty())
				{
					if (viewInfo.isRoot(info))
					{
						g.setColour(Colours::white.withAlpha(0.8f));
						g.setFont(GLOBAL_BOLD_FONT());
						g.drawText("R", area, Justification::centred);
					}
					else
					{
						Path p;

						p = factory.createPath("expand");

						bool isExpanded = info->expanded || viewInfo.is(ViewInfo::AllExpanded);

						if (isExpanded)
							p.applyTransform(AffineTransform::rotation(float_Pi * 0.5f));

						PathFactory::scalePath(p, area.reduced(7.0f));

						g.setColour(isExpanded ? Colours::white.withAlpha(0.8f) : Colours::white.withAlpha(0.4f));
						g.fillPath(p);
					}
				}
				else
				{
					if (viewInfo.is(info, ViewInfo::Pinned))
					{
						Path p = factory.createPath("pinned");

						area.setX(0.0f);

						PathFactory::scalePath(p, area.reduced(3.0f));

						g.setColour(Colours::white.withAlpha(0.8f));
						g.fillPath(p);
						return;
					}
					else
					{
						g.setColour(Colours::white.withAlpha(0.3f));
						g.fillEllipse(area.withSizeKeepingCentre(3.0f, 3.0f));
					}
				}

				return;
			}
			if (columnId == Type)
			{
				Rectangle<float> area(0.0, 0.0, width, height);

				area = area.withSizeKeepingCentre(18.0, 18.0);

				Colour colour;

				char c;

				pr->getColourAndLetterForType(info->type, colour, c);

				g.setColour(colour.withMultipliedSaturation(0.5f));

				g.fillRoundedRectangle(area, 5.0f);
				g.setColour(Colours::white.withAlpha(0.4f));
				g.drawRoundedRectangle(area, 5.0f, 1.0f);
				g.setFont(GLOBAL_BOLD_FONT());
				g.setColour(Colours::white);

				String type;
				type << c;
				g.drawText(type, area, Justification::centred);
			}
			else
			{
				text = getTextForColumn((ColumnId)columnId, info, false);

				Colour c = Colours::white.withAlpha(0.8f);

				auto fat = columnId == ColumnId::Value && changed[rowNumber];

				if (fat)
					c = Colour(0xFFFFFFDD);

				g.setColour(c);

				

				auto ff = GLOBAL_MONOSPACE_FONT().withHeight(table->getRowHeight() * 0.7f);

				g.setFont(ff);
				g.drawText(text, 5, 0, width - 10, height, Justification::centredLeft, true);
			}
		}
	}
}

void ScriptWatchTable::mouseWheelMove(const MouseEvent& e, const MouseWheelDetails& d)
{
	if (e.mods.isCommandDown())
	{
		auto h = table->getRowHeight();
		h += (d.deltaY > 0) ? 1 : -1;
		h = jlimit(24, 60, h);

		table->setRowHeight(h);
	}
}



String ScriptWatchTable::getHeadline() const
{  
	String x;
        
	x << "Watch Script Variable";
	return x;
}

    
void ScriptWatchTable::resized()
{
	table->getHeader().resizeAllColumnsToFit(getWidth());

	table->setBounds(0, 24, getWidth(), jmax<int>(0, getHeight() - 24));

	auto b = getLocalBounds().removeFromTop(23);
	b.removeFromLeft(24);
	menuButton.setBounds(b.removeFromRight(24).reduced(3));
	expandButton.setBounds(b.removeFromRight(24).reduced(3));
	pinButton.setBounds(b.removeFromRight(24).reduced(3));
	refreshButton.setBounds(b.removeFromRight(24).reduced(3));
	fuzzySearchBox->setBounds(b);
}



ScriptWatchTable::Info::Info(DebugInformationBase::Ptr di, Info* p_, int l /*= 0*/) :
	source(di),
	level(l),
	parent(p_),
	type(di->getType()),
	dataType(di->getTextForDataType()),
	name(di->getTextForName())
{
	String ws;

	for (int i = 0; i < level; i++)
		ws << " ";

	name = DebugInformationBase::replaceParentWildcard(name, parent->name);

	name = ws + name.trim();
	
	auto numElements = di->getNumChildElements();

	if (l < 10)
	{
		for (int i = 0; i < numElements; i++)
		{
			if (auto li = di->getChildElement(i))
			{
				if (li->isWatchable())
					children.add(new Info(li, this, l + 1));
			}
		}
	}
}

bool ScriptWatchTable::Info::forEachExpandedChildren(const std::function<bool(Info::Ptr)>& f, bool forceChildren, bool skipSelf)
{
	if (!skipSelf && f(*this))
		return true;

	if (expanded || forceChildren)
	{
		for (auto c : children)
		{
			if (c->forEachExpandedChildren(f, forceChildren))
				return true;
		}
	}

	return false;
}

bool ScriptWatchTable::Info::forEachParent(const std::function<bool(Ptr)>& f)
{
	if (f(this))
		return true;

	if (parent != nullptr)
		return parent->forEachParent(f);

	return false;
}

juce::Path ScriptWatchTable::Factory::createPath(const String& url) const
{
	Path p;

	if (url == "search")
	{
		static const unsigned char searchIcon[] = { 110, 109, 0, 0, 144, 68, 0, 0, 48, 68, 98, 7, 31, 145, 68, 198, 170, 109, 68, 78, 223, 103, 68, 148, 132, 146, 68, 85, 107, 42, 68, 146, 2, 144, 68, 98, 54, 145, 219, 67, 43, 90, 143, 68, 66, 59, 103, 67, 117, 24, 100, 68, 78, 46, 128, 67, 210, 164, 39, 68, 98, 93, 50, 134, 67, 113, 58, 216, 67, 120, 192, 249, 67, 83, 151,
		103, 67, 206, 99, 56, 68, 244, 59, 128, 67, 98, 72, 209, 112, 68, 66, 60, 134, 67, 254, 238, 144, 68, 83, 128, 238, 67, 0, 0, 144, 68, 0, 0, 48, 68, 99, 109, 0, 0, 208, 68, 0, 0, 0, 195, 98, 14, 229, 208, 68, 70, 27, 117, 195, 211, 63, 187, 68, 146, 218, 151, 195, 167, 38, 179, 68, 23, 8, 77, 195, 98, 36, 92, 165, 68, 187, 58,
		191, 194, 127, 164, 151, 68, 251, 78, 102, 65, 0, 224, 137, 68, 0, 0, 248, 66, 98, 186, 89, 77, 68, 68, 20, 162, 194, 42, 153, 195, 67, 58, 106, 186, 193, 135, 70, 41, 67, 157, 224, 115, 67, 98, 13, 96, 218, 193, 104, 81, 235, 67, 243, 198, 99, 194, 8, 94, 78, 68, 70, 137, 213, 66, 112, 211, 134, 68, 98, 109, 211, 138, 67,
		218, 42, 170, 68, 245, 147, 37, 68, 128, 215, 185, 68, 117, 185, 113, 68, 28, 189, 169, 68, 98, 116, 250, 155, 68, 237, 26, 156, 68, 181, 145, 179, 68, 76, 44, 108, 68, 16, 184, 175, 68, 102, 10, 33, 68, 98, 249, 118, 174, 68, 137, 199, 2, 68, 156, 78, 169, 68, 210, 27, 202, 67, 0, 128, 160, 68, 0, 128, 152, 67, 98, 163,
		95, 175, 68, 72, 52, 56, 67, 78, 185, 190, 68, 124, 190, 133, 66, 147, 74, 205, 68, 52, 157, 96, 194, 98, 192, 27, 207, 68, 217, 22, 154, 194, 59, 9, 208, 68, 237, 54, 205, 194, 0, 0, 208, 68, 0, 0, 0, 195, 99, 101, 0, 0 };

		p.loadPathFromData(searchIcon, sizeof(searchIcon));
		return p;
	}

	if (url == "expand")
	{
		Path s;
		s.startNewSubPath(0.0f, 0.0f);
		s.lineTo(1.0f, 0.5f);
		s.lineTo(0.0f, 1.0f);

		PathStrokeType(0.3f, PathStrokeType::curved, PathStrokeType::rounded).createStrokedPath(p, s);
		return p;
	}

	if (url == "pinned")
	{
		static const unsigned char pathData[] = { 110,109,172,28,90,62,0,0,0,0,108,8,172,128,64,0,0,0,0,108,8,172,128,64,139,108,103,63,108,168,198,83,64,139,108,103,63,108,168,198,83,64,246,40,68,64,98,160,26,119,64,186,73,92,64,51,51,135,64,4,86,130,64,238,124,135,64,125,63,153,64,108,172,28,34,64,
			125,63,153,64,108,238,124,7,64,131,192,6,65,108,94,186,217,63,125,63,153,64,108,0,0,0,0,125,63,153,64,98,111,18,3,60,4,86,130,64,219,249,190,62,186,73,92,64,205,204,108,63,246,40,68,64,108,205,204,108,63,139,108,103,63,108,172,28,90,62,139,108,103,63,
			108,172,28,90,62,0,0,0,0,99,101,0,0 };

		p.loadPathFromData(pathData, sizeof(pathData));
		return p;
	}

	if (url == "menu")
	{
		Rectangle<float> a(0.0, 0.0, 1.0, 1.0);
		p.addEllipse(a);
		p.addEllipse(a.translated(0.0f, 2.0f));
		p.addEllipse(a.translated(0.0f, 4.0f));
		return p;
	}

	if (url == "refresh")
	{
		static const unsigned char layoutIcon[] = { 110, 109, 0, 245, 207, 67, 128, 217, 36, 67, 108, 0, 236, 189, 67, 128, 89, 69, 67, 108, 0, 245, 207, 67, 128, 217, 101, 67, 108, 192, 212, 207, 67, 128, 53, 81, 67, 98, 217, 93, 211, 67, 51, 180, 80, 67, 123, 228, 219, 67, 2, 123, 91, 67, 128, 144, 224, 67, 0, 149, 101, 67, 98, 39, 209, 224, 67, 29, 247, 89, 67, 79, 60,
			223, 67, 36, 224, 61, 67, 0, 245, 207, 67, 0, 12, 54, 67, 108, 0, 245, 207, 67, 128, 217, 36, 67, 99, 109, 128, 33, 193, 67, 0, 168, 88, 67, 108, 0, 66, 193, 67, 0, 76, 109, 67, 98, 231, 184, 189, 67, 77, 205, 109, 67, 69, 50, 181, 67, 126, 6, 99, 67, 64, 134, 176, 67, 128, 236, 88, 67, 98, 154, 69, 176, 67, 99, 138, 100, 67,
			49, 218, 177, 67, 174, 80, 128, 67, 128, 33, 193, 67, 192, 58, 132, 67, 108, 128, 33, 193, 67, 0, 212, 140, 67, 108, 192, 42, 211, 67, 0, 40, 121, 67, 108, 128, 33, 193, 67, 0, 168, 88, 67, 99, 101, 0, 0 };

		p.loadPathFromData(layoutIcon, sizeof(layoutIcon));
		return p;
	}
	
    return p;
}

void ScriptWatchTable::TooltipInfo::draw(Graphics& g)
{
	if (!ready)
		return;

	auto delta = cellPos.getHeight() + 6;

	g.setColour(Colours::black.withAlpha(0.7f));
	g.fillRect(cellPos.toFloat());

	
	
	if (cellPos.getY() > parent.getHeight() / 2)
		delta *= -1;

	auto tb = cellPos.translated(0, delta).toFloat().expanded(3.0f);
	auto f = GLOBAL_BOLD_FONT().withHeight(tb.getHeight() * 0.7f);
	tb = tb.withSizeKeepingCentre(f.getStringWidthFloat(t) + 10.0f, tb.getHeight());

	if (tb.getRight() > (float)parent.getWidth())
		tb = tb.withRightX((float)parent.getWidth() - 2.0f);

	if (tb.getX() < 0.0f)
		tb = tb.withX(2.0f);

	g.setFont(f);
	g.setColour(Colours::white.withAlpha(0.9f));
	g.fillRoundedRectangle(tb, 3.0f);
	g.setColour(Colours::black.withAlpha(0.8f));
	g.drawRoundedRectangle(tb, 3.0f, 1.0f);
	g.drawText(t, tb, Justification::centred);
}

} // namespace hise
