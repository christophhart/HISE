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
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

namespace hise { using namespace juce;

ScriptTableListModel::ScriptTableListModel(ProcessorWithScriptingContent* p, const var& td) :
	TableListBoxModel(),
	SimpleTimer(p->getMainController_()->getGlobalUIUpdater()),
	tableMetadata(td),
	cellCallback(p, nullptr, var(), 3),
	sortCallback(p, nullptr, var(), 2),
	pwsc(p)
{
    processSpaceKey = tableMetadata.getProperty("ProcessSpaceKey", false);
    
	tableRefreshBroadcaster.enableLockFreeUpdate(p->getMainController_()->getGlobalUIUpdater());

	eventTypesForCallback.add(EventType::SingleClick);
	eventTypesForCallback.add(EventType::DoubleClick);
	eventTypesForCallback.add(EventType::ReturnKey);
	eventTypesForCallback.add(EventType::SpaceKey);
}

bool ScriptTableListModel::isMultiColumn() const
{
	return tableMetadata.getProperty("MultiColumnMode", false);
}

void ScriptTableListModel::paintCell(Graphics& g, int rowNumber, int columnId, int width, int height, bool rowIsSelected)
{
	auto lafToUse = laf != nullptr ? laf : &fallback;

    auto s = getCellValue(rowNumber, columnId - 1);

    if(s.isUndefined() || s.isVoid())
        return;
    
	auto isClicked = lastClickedCell.y == rowNumber && lastClickedCell.x == columnId;
	auto isHover = hoverPos.y == rowNumber && hoverPos.x == columnId;

	lafToUse->drawTableCell(g, d, s.toString(), rowNumber, columnId - 1, width, height, rowIsSelected, isClicked, isHover);
}



struct ComponentUpdateHelpers
{
	struct ShiftSlider : public juce::Slider,
						 public SliderWithShiftTextBox
	{
		void mouseDown(const MouseEvent& e) override
		{
			if (onShiftClick(e))
				return;
			else
				juce::Slider::mouseDown(e);
		}
	};

	enum class ComboBoxValueMode
	{
		ID,
		Index,
		Text,
		numModes
	};

	static ComboBoxValueMode getValueMode(const var& metadata, int columnId)
	{
		static const StringArray comboboxValueModes = { "ID", "Index", "Text" };
		auto d = metadata[columnId].getProperty("ValueMode", "ID");
		return (ComboBoxValueMode)comboboxValueModes.indexOf(d.toString());
	}

	static bool updateItemList(ComboBox* cb, const var& obj)
	{
		if (obj.isObject())
		{
			auto l = obj["items"];

			if (auto ar = l.getArray())
			{
				StringArray items;

				for (auto& v : *ar)
					items.add(v.toString());

				cb->clear(dontSendNotification);
				cb->addItemList(items, 1);

				return true;
			}
		}

		return false;
	}

	static void updateValue(ComboBox* cb, ComboBoxValueMode mode, const var& valueToUse)
	{
		if (valueToUse.isObject())
		{
			updateValue(cb, mode, valueToUse[scriptnode::PropertyIds::Value]);
			return;
		}

		if (valueToUse.isUndefined())
			cb->setSelectedId(0, dontSendNotification);
		else
		{
			if (mode == ComboBoxValueMode::ID)
			{
				auto id = (int)valueToUse;
				if (id >= 1 && id <= cb->getNumItems())
					cb->setSelectedId(id, dontSendNotification);
			}
			if (mode == ComboBoxValueMode::Index)
			{
				auto index = (int)valueToUse;

				if (index != -1 && index < cb->getNumItems())
					cb->setSelectedItemIndex(index, dontSendNotification);
			}
			if (mode == ComboBoxValueMode::Text)
			{
				auto t = valueToUse.toString();
				cb->setText(t, dontSendNotification);
			}
		}
	}

	static void updateValue(ShiftSlider* s, const var& value)
	{
		if (value.isObject())
			updateValue(s, value[scriptnode::PropertyIds::Value]);
		else if (!value.isUndefined())
		{
			s->setValue((double)value, dontSendNotification);
		}
	}

	static bool updateSliderProperties(ShiftSlider* s, const var& value)
	{
		if (value.isObject())
		{
			auto r = scriptnode::RangeHelpers::getDoubleRange(value);

			s->setRange(r.getRange(), r.rng.interval);
			s->setSkewFactor(r.rng.skew);
			s->setTextValueSuffix(value["suffix"]);
			s->setDoubleClickReturnValue(value.hasProperty("defaultValue"), (double)value["defaultValue"]);

			s->enableShiftTextInput = value.getProperty("showTextBox", true);

			StringArray modeNames = { "Knob", "Horizontal", "Vertical" };
			Slider::SliderStyle modes[3] = { Slider::SliderStyle::RotaryHorizontalVerticalDrag,
											 Slider::SliderStyle::LinearBar,
											 Slider::SliderStyle::LinearBarVertical };

			auto idx = modeNames.indexOf(value["style"].toString());

			if (idx != -1)
				s->setSliderStyle(modes[idx]);

			return true;
		}
		else
		{
			return false;
		}
	}

	static var getValue(ComboBox* cb, ComboBoxValueMode mode)
	{
		switch (mode)
		{
		case ComboBoxValueMode::ID:
			return cb->getSelectedId();
		case ComboBoxValueMode::Index:
			return cb->getSelectedItemIndex();
		case ComboBoxValueMode::Text:
			return cb->getText();
		case ComboBoxValueMode::numModes:
		default:
			jassertfalse;
			return {};
		}
	}
};

Component* ScriptTableListModel::refreshComponentForCell(int rowNumber, int columnId, bool isRowSelected, Component* existingComponentToUpdate)
{
	

	columnId--;

	auto cellType = getCellType(columnId);

	if (cellType == CellType::numCellTypes || cellType == CellType::Text || cellType == CellType::Hidden)
	{
		jassert(existingComponentToUpdate == nullptr);
		return nullptr;
	}

    auto value = getCellValue(rowNumber, columnId);

    if(value.isUndefined() || value.isVoid())
    {
        if(existingComponentToUpdate != nullptr)
            delete existingComponentToUpdate;
        
        return nullptr;
    }
    
	if (existingComponentToUpdate != nullptr)
	{
		switch (cellType)
		{
		case CellType::Button:
		{
			if (auto b = dynamic_cast<MomentaryToggleButton*>(existingComponentToUpdate))
			{
				b->getProperties().set("RowIndex", rowNumber);
				b->setToggleState((bool)value, dontSendNotification);
			}
				

			break;
		}
		case CellType::Slider:
		{
			if (auto s = dynamic_cast<ComponentUpdateHelpers::ShiftSlider*>(existingComponentToUpdate))
			{
				s->getProperties().set("RowIndex", rowNumber);

				ComponentUpdateHelpers::updateSliderProperties(s, value);
				ComponentUpdateHelpers::updateValue(s, value);
			}
			
			break;
		}
		case CellType::ComboBox:
		{
			if (auto s = dynamic_cast<juce::ComboBox*>(existingComponentToUpdate))
			{
				s->getProperties().set("RowIndex", rowNumber);

				auto valueMode = ComponentUpdateHelpers::getValueMode(columnMetadata, columnId);

				ComponentUpdateHelpers::updateItemList(s, value);
				ComponentUpdateHelpers::updateValue(s, valueMode, value);
			}

			break;
		}
		case CellType::Image:
		{
			jassertfalse;
			break;
		}
        default:
            jassertfalse;
            break;    
		}

		return existingComponentToUpdate;
	}
	else
	{
		auto cd = columnMetadata[columnId];

		switch (cellType)
		{
		case CellType::Button:
		{
			auto b = new MomentaryToggleButton(cd.getProperty("Text", "Button"));

			auto momentary = !(bool)cd.getProperty("Toggle", false);

			b->setIsMomentary(momentary);

			auto c = columnId + 1;
			
			b->getProperties().set("RowIndex", rowNumber);

			b->onClick = [c, b, this]()
			{
				auto id = columnMetadata[c - 1][PropertyIds::ID].toString();
				
				auto r = (int)b->getProperties()["RowIndex"];

				if (auto obj = rowData[r].getDynamicObject())
				{
					obj->setProperty(id, b->getToggleState());
				}

				sendCallback(r, c, b->getToggleState(), EventType::ButtonCallback);
			};

			fallback.setDefaultColours(*b);

			if (!momentary)
				b->setToggleState((bool)getCellValue(rowNumber, columnId), dontSendNotification);

			return b;
		}
		case CellType::Slider:
		{
			auto s = new ComponentUpdateHelpers::ShiftSlider();

			auto n = cd[scriptnode::PropertyIds::ID].toString();
			n << String(rowNumber);

			s->getProperties().set("RowIndex", rowNumber);

			s->setName(n);
			s->setScrollWheelEnabled(false);

			auto s_ = s;
			
			auto c = columnId+1;

			s->onValueChange = [s_, c, this]()
			{
				auto id = columnMetadata[c - 1][PropertyIds::ID].toString();

				auto r_ = (int)s_->getProperties()["RowIndex"];

				if (auto obj = rowData[r_].getDynamicObject())
				{
					obj->setProperty(id, s_->getValue());
				}

				sendCallback(r_, c, s_->getValue(), EventType::SliderCallback);
			};

			fallback.setDefaultColours(*s);

			s->setTextBoxStyle(Slider::NoTextBox, false, 0, 0);

			if (!ComponentUpdateHelpers::updateSliderProperties(s, value))
				ComponentUpdateHelpers::updateSliderProperties(s, cd);

			ComponentUpdateHelpers::updateValue(s, value);

			return s;
		}
		case CellType::ComboBox:
		{
			auto cb = new ComboBox();

			auto n = cd[scriptnode::PropertyIds::ID].toString();
			n << String(rowNumber);

			cb->getProperties().set("RowIndex", rowNumber);

			cb->setName(n);
			cb->setTextWhenNothingSelected(cd.getProperty("Text", "No selection"));

			

			auto cb_ = cb;

			auto c = columnId + 1;

			auto valueMode = ComponentUpdateHelpers::getValueMode(columnMetadata, columnId);

			cb->onChange = [cb_, c, this, valueMode]()
			{
				auto id = columnMetadata[c - 1][PropertyIds::ID].toString();

				auto r_ = (int)cb_->getProperties()["RowIndex"];

				var value = ComponentUpdateHelpers::getValue(cb_, valueMode);

				if (auto obj = rowData[r_].getDynamicObject())
				{
					obj->setProperty(id, value);
				}

				sendCallback(r_, c, value, EventType::ComboboxCallback);
			};

			fallback.setDefaultColours(*cb);

			

			if(!ComponentUpdateHelpers::updateItemList(cb, value))
				ComponentUpdateHelpers::updateItemList(cb, cd);

			ComponentUpdateHelpers::updateValue(cb, valueMode, value);

			return cb;
		}
		case CellType::Image:
			jassertfalse;
			break;
        default:
            jassertfalse;
            break;
		}
	}

	return existingComponentToUpdate;
}

void ScriptTableListModel::setup(juce::TableListBox* t)
{
	auto& header = t->getHeader();

	t->setLookAndFeel(&fallback);

	if(isMultiColumn())
		tableRepainters.add(new TableRepainter(t, *this));

	int idx = 1;

	if (auto a = columnMetadata.getArray())
	{
		for (auto c : *a)
		{
			auto id = c[scriptnode::PropertyIds::ID].toString();
			auto label = c.getProperty("Label", id);

			auto w = (int)c["Width"];
			auto min = jmax(1, (int)c["MinWidth"]);
			auto max = (int)c.getProperty("MaxWidth", -1);

			if (max != -1)
			{
				auto r = Range<int>(min, max);
				w = r.clipValue(w);
			}
			else
				w = jmax(min, w);

			int flag = TableHeaderComponent::ColumnPropertyFlags::visible;

			if(tableMetadata.getProperty("Sortable", false))
				flag |= TableHeaderComponent::ColumnPropertyFlags::sortable;

			header.addColumn(label, idx, w, min, max, flag);
			idx++;
		}
	}

	t->setAutoSizeMenuOptionShown(false);
	t->setHeaderHeight(tableMetadata.getProperty("HeaderHeight", 24));
	t->setRowHeight(tableMetadata.getProperty("RowHeight", 20));
	t->setMultipleSelectionEnabled(tableMetadata.getProperty("MultiSelection", false));
	t->getViewport()->setScrollOnDragEnabled(tableMetadata.getProperty("ScrollOnDrag", false));

	t->setModel(this);
}


ScriptTableListModel::LookAndFeelMethods::~LookAndFeelMethods()
{}

int ScriptTableListModel::getNumRows()
{
	return rowData.size();
}

void ScriptTableListModel::paintRowBackground(Graphics& g, int rowNumber, int width, int height, bool rowIsSelected)
{
	auto lafToUse = laf != nullptr ? laf : &fallback;

	lafToUse->drawTableRowBackground(g, d, rowNumber, width, height, rowIsSelected);
}

var ScriptTableListModel::getCellValue(int rowIndex, int columnIndex) const
{
	if(isPositiveAndBelow(columnIndex, columnMetadata.size()))
	{
		auto id = columnMetadata[columnIndex][scriptnode::PropertyIds::ID].toString();
            
		if(isPositiveAndBelow(rowIndex, rowData.size()))
			return rowData[rowIndex][Identifier(id)];
            
		return {};
	}
	else
	{
		jassertfalse;
		return {};
	}
		
}

void ScriptTableListModel::timerCallback()
{
	for (auto r : repaintedColumns)
		tableColumnRepaintBroadcaster.sendMessage(sendNotificationSync, r);
			
}

int ScriptTableListModel::getColumnAutoSizeWidth(int columnId)
{
	return columnMetadata[columnId - 1].getProperty("MaxWidth", 10000);
}

void ScriptTableListModel::setExternalLookAndFeel(LookAndFeelMethods* l)
{
	laf = l;
}

void ScriptTableListModel::addAdditionalCallback(const std::function<void(int columnIndex, int rowIndex)>& f)
{
	additionalCallback = f;
}

var ScriptTableListModel::getRowData() const
{ return rowData.clone(); }

int ScriptTableListModel::defaultSorter(const var& v1, const var& v2)
{
	if (v1 < v2) return -1;
	else if (v1 > v2) return 1;
	else return 0;
}

ScriptTableListModel::TableRepainter::TableRepainter(TableListBox* t_, ScriptTableListModel& parent_):
	t(t_),
	parent(parent_)
{
	t_->addMouseListener(this, true);
	t_->addKeyListener(this);
}

ScriptTableListModel::TableRepainter::~TableRepainter()
{
	if(auto tt = t.getComponent())
	{
		tt->removeMouseListener(this);
		tt->removeKeyListener(this);
	}
				
}

void ScriptTableListModel::TableRepainter::mouseEnter(const MouseEvent& e)
{
	repaintIfCellChange(e);
}

void ScriptTableListModel::TableRepainter::mouseMove(const MouseEvent& e)
{
	repaintIfCellChange(e);
}

void ScriptTableListModel::LookAndFeelMethods::drawTableRowBackground(Graphics& g, const LookAndFeelData& d, int rowNumber, int width, int height, bool rowIsSelected)
{
	Rectangle<int> a(0, 0, width, height);

	g.fillAll(d.bgColour);

	if (rowNumber % 2 == 0)
	{
		g.setColour(Colours::grey.withAlpha(0.05f));
		g.fillRect(a);
	}

	if (rowIsSelected)
	{
		g.setColour(Colours::grey.withAlpha(0.1f));
		g.fillRect(a);
	}
}

void ScriptTableListModel::LookAndFeelMethods::drawTableCell(Graphics& g, const LookAndFeelData& d, const String& text, int rowNumber, int columnId, int width, int height, bool rowIsSelected, bool cellIsClicked, bool cellIsHovered)
{
	g.setColour(d.textColour);
	g.setFont(d.f);
	Rectangle<int> a(0, 0, width, height);
	g.drawText(text, a.toFloat().reduced(3.0f), d.c);
}

void ScriptTableListModel::DefaultLookAndFeel::drawTableHeaderBackground(Graphics& g, TableHeaderComponent& h)
{
	drawDefaultTableHeaderBackground(g, h);
}

void ScriptTableListModel::LookAndFeelMethods::drawDefaultTableHeaderBackground(Graphics& g, TableHeaderComponent& h)
{
	auto d = getDataFromTableHeader(h);

	

	g.setColour(d.itemColour2);
	g.fillAll();
}



void ScriptTableListModel::LookAndFeelMethods::drawDefaultTableHeaderColumn(Graphics& g, TableHeaderComponent& h, const String& columnName, int columnId, int width, int height, bool isMouseOver, bool isMouseDown, int columnFlags)
{
	auto d = getDataFromTableHeader(h);

	g.setFont(d.f);
	Rectangle<int> a(0, 0, width - 1, height);

	g.setColour(d.itemColour1);
	g.fillRect(a);

	g.setColour(d.textColour);
	g.drawText(columnName, a.toFloat().reduced(3.0f), d.c);

	if (d.sortColumnId == columnId)
	{
		auto b = a.toFloat().removeFromRight(a.getHeight()).reduced(8.0f);

		Path p;

		if (!d.sortForward)
			p.addTriangle(b.getBottomLeft(), { b.getCentreX(), b.getY() }, b.getBottomRight());
		else
			p.addTriangle(b.getTopLeft(), { b.getCentreX(), b.getBottom() }, b.getTopRight());
		
		g.fillPath(p);
	}
}

ScriptTableListModel::LookAndFeelData ScriptTableListModel::LookAndFeelMethods::getDataFromTableHeader(TableHeaderComponent& h)
{
	if (TableListBox* table = h.findParentComponentOfClass<TableListBox>())
	{
		if (auto st = dynamic_cast<ScriptTableListModel*>(table->getModel()))
		{
			return st->d;
		}
	}
	
	return {};
}

void ScriptTableListModel::DefaultLookAndFeel::drawTableHeaderColumn(Graphics& g, TableHeaderComponent& h, const String& columnName, int columnId, int width, int height, bool isMouseOver, bool isMouseDown, int columnFlags)
{
	drawDefaultTableHeaderColumn(g, h, columnName, columnId, width, height, isMouseOver, isMouseDown, columnFlags);
}


hise::ScriptTableListModel::CellType ScriptTableListModel::getCellType(int columnId) const
{
	if (isPositiveAndBelow(columnId, cellTypes.size()))
	{
		auto t = cellTypes[columnId];

		if (t != CellType::numCellTypes)
			return t;
	}

	jassertfalse;
	return CellType::numCellTypes;
}

void ScriptTableListModel::setTableColumnData(var cd)
{
	columnMetadata = cd;

	cellTypes.clear();

	if (columnMetadata.isArray())
	{
		int idx = 0;
		repaintedColumns.clear();

		for (auto& v : *columnMetadata.getArray())
		{
			if (v["PeriodicRepaint"])
				repaintedColumns.add(idx+1);

			if (auto obj = v.getDynamicObject())
			{
				auto t = obj->getProperty(scriptnode::PropertyIds::Type).toString();

				if (t.isNotEmpty())
				{
					static const StringArray types =
					{
						"Text",
						"Button",
						"Image",
						"Slider",
						"ComboBox",
						"Hidden"
					};

					auto type = (CellType)types.indexOf(t);
					cellTypes.add(type);
				}
				else
					cellTypes.add(CellType::Text);
			}

			idx++;
		}

		if (repaintedColumns.isEmpty())
			stop();
		else
			start();
	}
}

void ScriptTableListModel::setFont(Font f, Justification c)
{
	d.f = f;
	d.c = c;
}

void ScriptTableListModel::setColours(Colour textColour, Colour bgColour, Colour itemColour1, Colour itemColour2)
{
	d.textColour = textColour;
	d.bgColour = bgColour;
	d.itemColour1 = itemColour1;
	d.itemColour2 = itemColour2;
}

void ScriptTableListModel::sortOrderChanged(int newSortColumnId, bool isForwards)
{
	auto idToUse = Identifier(columnMetadata[newSortColumnId-1]["ID"].toString());

	d.sortColumnId = newSortColumnId;
	d.sortForward = isForwards;

	jassert(idToUse.isValid());

	struct PropertySorter
	{
		PropertySorter(const Identifier& p_, bool sortForward_, const SortFunction& sf_):
			p(p_),
			sortForward(sortForward_),
			sf(sf_)
		{

		}

		int compareElements(const var& first, const var& second) const
		{
			auto v1 = first[p];
			auto v2 = second[p];

			if (!sortForward)
				std::swap(v1, v2);

			return sf(v1, v2);
		}

		const Identifier p;
		const bool sortForward;
		SortFunction sf;
	};

	if (auto a = rowData.getArray())
	{
		PropertySorter sorter(idToUse, isForwards, sortFunction);

		a->sort(sorter);
	}
}

Result ScriptTableListModel::setEventTypesForValueCallback(var eventTypeList)
{
	StringArray eventTypes =
	{
		"SliderCallback",
		"ButtonCallback",
		"Selection",
		"SingleClick",
		"DoubleClick",
		"ReturnKey",
		"SetValue",
		"Undo",
		"DeleteRow"
	};

	Array<EventType> illegalTypes =
	{
		EventType::SliderCallback,
		EventType::ButtonCallback,
		EventType::SetValue,
		EventType::Undo,
		EventType::DeleteRow
	};

	eventTypesForCallback.clear();

	if (eventTypeList.isArray())
	{
		for (const auto& r : *eventTypeList.getArray())
		{
			auto idx = eventTypes.indexOf(r.toString());
			
			if (idx == -1)
				return Result::fail("unknown event type: " + r.toString());

			if (illegalTypes.contains((EventType)idx))
				return Result::fail("illegal event type for value callback: " + r.toString());
			
			eventTypesForCallback.add((EventType)idx);
		}
	}
	else
		return Result::fail("event type list is not an array");

	return Result::ok();
}

int ScriptTableListModel::getOriginalRowIndex(int rowIndex) const
{
	auto item = rowData[rowIndex];
	return originalRowData.indexOf(item);
}

void ScriptTableListModel::cellClicked(int rowNumber, int columnId, const MouseEvent& e)
{
    auto value = getCellValue(rowNumber, columnId-1);

    if(value.isUndefined() || value.isVoid())
    {
        return;
    }
    
	lastClickedCell = { columnId, rowNumber };

    
	TableListBoxModel::cellClicked(rowNumber, columnId, e);
	sendCallback(rowNumber, columnId, rowData[rowNumber], EventType::SingleClick);
}

void ScriptTableListModel::cellDoubleClicked(int rowNumber, int columnId, const MouseEvent& e)
{
	lastClickedCell = { columnId, rowNumber };

	TableListBoxModel::cellDoubleClicked(rowNumber, columnId, e);
	sendCallback(rowNumber, lastClickedCell.x, rowData[rowNumber], EventType::DoubleClick);
}

void ScriptTableListModel::backgroundClicked(const MouseEvent& e)
{
	lastClickedCell = {};

	TableListBoxModel::backgroundClicked(e);
	sendCallback(-1, -1, var(Array<var>()), EventType::Selection);
}

void ScriptTableListModel::selectedRowsChanged(int lastRowSelected)
{
	Point<int> newSelection(lastClickedCell.x, lastRowSelected);

	if (newSelection == lastClickedCell)
		return;

	lastClickedCell.y = lastRowSelected;

	if (lastRowSelected == -1)
		return;

	sendCallback(lastRowSelected, lastClickedCell.x, rowData[lastRowSelected], EventType::Selection);
}

void ScriptTableListModel::deleteKeyPressed(int lastRowSelected)
{
	TableListBoxModel::deleteKeyPressed(lastRowSelected);
	sendCallback(lastRowSelected, 0, rowData[lastRowSelected], EventType::DeleteRow);
}

void ScriptTableListModel::returnKeyPressed(int lastRowSelected)
{
	TableListBoxModel::returnKeyPressed(lastRowSelected);
	sendCallback(lastRowSelected, lastClickedCell.x, rowData[lastRowSelected], EventType::ReturnKey);
}

void ScriptTableListModel::setTableSortFunction(var newSortFunction)
{
	if (HiseJavascriptEngine::isJavascriptFunction(newSortFunction))
	{
		sortCallback = WeakCallbackHolder(pwsc, nullptr, newSortFunction, 2);
		sortCallback.incRefCount();
		

		sortFunction = [this](const var& v1, const var& v2)
		{
			if (!sortCallback)
				return defaultSorter(v1, v2);

			var args[2] = { v1, v2 };
			var rv;
			sortCallback.callSync(args, 2, &rv);

			return (int)rv;
		};
	}
	else
	{
		sortFunction = defaultSorter;
	}
}

void ScriptTableListModel::setRowData(var rd)
{
	originalRowData = rd.clone();

	Array<var> newData;

	if (auto a = originalRowData.getArray())
		newData.addArray(*a);

	rowData = var(newData);

	if (d.sortColumnId != 0)
	{
		sortOrderChanged(d.sortColumnId, d.sortForward);
	}

	tableRefreshBroadcaster.sendMessage(sendNotificationAsync, -1);
}

void ScriptTableListModel::setCallback(var callback)
{
	if (HiseJavascriptEngine::isJavascriptFunction(callback))
	{
		cellCallback = WeakCallbackHolder(pwsc, nullptr, callback, 1);
		cellCallback.incRefCount();
	}
}

void ScriptTableListModel::sendCallback(int rowId, int columnId, var value, EventType type)
{
	if (cellCallback)
	{
		auto obj = new DynamicObject();

		switch (type)
		{
		case EventType::SliderCallback:
			obj->setProperty("Type", "Slider");
			break;
		case EventType::ButtonCallback:
			obj->setProperty("Type", "Button");
			break;
		case EventType::ComboboxCallback:
			obj->setProperty("Type", "ComboBox");
			break;
		case EventType::Selection:
			obj->setProperty("Type", "Selection");
			break;
		case EventType::SingleClick:
			obj->setProperty("Type", "Click");
			break;
		case EventType::DoubleClick:
			obj->setProperty("Type", "DoubleClick");
			break;
		case EventType::ReturnKey:
			obj->setProperty("Type", "ReturnKey");
			break;
		case EventType::DeleteRow:
			obj->setProperty("Type", "DeleteRow");
			break;
		case EventType::SetValue:
			obj->setProperty("Type", "SetValue");
			break;
		case EventType::Undo:
			obj->setProperty("Type", "Undo");
		case EventType::SpaceKey:
			obj->setProperty("Type", "SpaceKey");
		}

		if (type == EventType::SetValue ||
			type == EventType::Undo)
		{
			Point<int> newCell(columnId, rowId);

			if (lastClickedCell == newCell)
				return;

			lastClickedCell = newCell;

			if(rowData.isArray() && isPositiveAndBelow(rowId, rowData.size()))
				value = rowData[rowId];
		}

		bool notifyParent = eventTypesForCallback.contains(type);

		obj->setProperty("rowIndex", rowId);

		if (isPositiveAndBelow(columnId - 1, columnMetadata.size()))
			obj->setProperty("columnID", columnMetadata[columnId - 1][scriptnode::PropertyIds::ID]);
		
		obj->setProperty("value", value);

		var a(obj);
		cellCallback.call1(a);

		if (notifyParent && additionalCallback)
		{
			additionalCallback(columnId - 1, rowId);
		}
	}
}

bool ScriptTableListModel::TableRepainter::keyPressed(const KeyPress& key, Component* originatingComponent)
{
	if (key == KeyPress::leftKey ||
		key == KeyPress::rightKey)
	{
		auto delta = key == KeyPress::leftKey ? -1 : 1;

		auto c = parent.lastClickedCell.x;
		auto fc = c;

		auto isEOF = [&](int columnId)
		{
			return !isPositiveAndBelow(columnId - 1, parent.columnMetadata.size());
		};

		auto canBeFocused = [&](int columnId)
		{
			jassert(!isEOF(columnId));
			return (bool)parent.columnMetadata[columnId - 1].getProperty("Focus", true);
		};

		c += delta;

		while (!isEOF(c))
		{
			if (canBeFocused(c))
			{
				fc = c;
				break;
			}

			c += delta;
		}

		parent.lastClickedCell.x = fc;

		// force an update like this
		auto oldY = parent.lastClickedCell.y;
		parent.lastClickedCell.y = -1;
		parent.selectedRowsChanged(oldY);
		t.getComponent()->repaintRow(oldY);
		return true;
	}
	if (key == KeyPress::spaceKey)
	{
        if(parent.processSpaceKey)
        {
            parent.sendCallback(parent.lastClickedCell.x, parent.lastClickedCell.y, parent.rowData[parent.lastClickedCell.y], EventType::SpaceKey);
            
            return true;
        }
		
	}

	return false;
}

void ScriptTableListModel::TableRepainter::mouseDown(const MouseEvent& e)
{
	if(dynamic_cast<Button*>(e.eventComponent) ||
		dynamic_cast<Slider*>(e.eventComponent))
	{
		return;
	}

	auto send = false;

	if (parent.lastClickedCell.y == parent.hoverPos.y)
		send = true;

	parent.lastClickedCell = parent.hoverPos;

	if (send)
		parent.selectedRowsChanged(parent.lastClickedCell.y);

	t.getComponent()->repaintRow(parent.hoverPos.y);
}

void ScriptTableListModel::TableRepainter::mouseExit(const MouseEvent& event)
{
	t.getComponent()->repaintRow(parent.hoverPos.y);
	parent.hoverPos = {};
}

void ScriptTableListModel::TableRepainter::repaintIfCellChange(const MouseEvent& e)
{
	auto te = e.getEventRelativeTo(t.getComponent());
	auto pos = te.getPosition();

	Point<int> s;

	s.y = t.getComponent()->getRowContainingPosition(pos.x, pos.y);

	for (int i = 0; i < parent.columnMetadata.size(); i++)
	{
		auto c = t.getComponent()->getCellPosition(i + 1, s.y, true);

		if (c.isEmpty())
			break;

		if (c.contains(pos))
		{
			s.x = i + 1;
			break;
		}
	}

	if (parent.hoverPos != s)
	{
		t->repaintRow(parent.hoverPos.y);
		parent.hoverPos = s;
		t->repaintRow(parent.hoverPos.y);
	}
}

} 
