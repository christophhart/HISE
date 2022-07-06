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
	tableMetadata(td),
	cellCallback(p, var(), 3),
	pwsc(p)
{
	tableRefreshBroadcaster.enableLockFreeUpdate(p->getMainController_()->getGlobalUIUpdater());
}

Component* ScriptTableListModel::refreshComponentForCell(int rowNumber, int columnId, bool isRowSelected, Component* existingComponentToUpdate)
{
	columnId--;

	auto cellType = getCellType(columnId);

	if (cellType == CellType::numCellTypes || cellType == CellType::Text)
	{
		jassert(existingComponentToUpdate == nullptr);
		return nullptr;
	}

	if (existingComponentToUpdate != nullptr)
	{
		auto value = getCellValue(rowNumber, columnId);

		switch (cellType)
		{
		case CellType::Button:
		{
			if (auto b = dynamic_cast<juce::Button*>(existingComponentToUpdate))
				b->setToggleState((bool)value, dontSendNotification);

			break;
		}
		case CellType::Slider:
		{
			if (auto s = dynamic_cast<juce::Slider*>(existingComponentToUpdate))
				s->setValue(value, dontSendNotification);

			break;
		}
		case CellType::Image:
		{
			jassertfalse;
			break;
		}
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
			auto b = new HiToggleButton(cd.getProperty("Text", "Button"));

			auto momentary = !(bool)cd.getProperty("Toggle", false);

			b->setIsMomentary(momentary);


			fallback.setDefaultColours(*b);

			if (!momentary)
				b->setToggleState((bool)getCellValue(rowNumber, columnId), dontSendNotification);

			return b;
		}
		case CellType::Slider:
		{
			auto s = new Slider();

			auto n = cd[scriptnode::PropertyIds::ID].toString();
			n << String(rowNumber);

			s->setName(n);

			auto r = scriptnode::RangeHelpers::getDoubleRange(cd);

			s->setSliderStyle(Slider::LinearBar);

			fallback.setDefaultColours(*s);

			s->setTextBoxStyle(Slider::NoTextBox, false, 0, 0);
			s->setRange(r.rng.getRange(), r.rng.interval);
			s->setSkewFactor(r.rng.skew);

			s->setValue((double)getCellValue(rowNumber, columnId));
			return s;
		}
		case CellType::Image:
		{
			jassertfalse;
			break;
			return existingComponentToUpdate;
		}
		}


	}

	return existingComponentToUpdate;
}

void ScriptTableListModel::setup(juce::TableListBox* t)
{
	auto& header = t->getHeader();

	t->setLookAndFeel(&fallback);

	int idx = 1;

	if (auto a = columnMetadata.getArray())
	{
		for (auto c : *a)
		{
			auto id = c[scriptnode::PropertyIds::ID].toString();
			auto label = c.getProperty("Label", id);

			auto w = (int)c["Width"];
			auto min = jmax(10, (int)c["MinWidth"]);
			auto max = (int)c.getProperty("MaxWidth", -1);

			if (max != -1)
			{
				auto r = Range<int>(min, max);
				w = r.clipValue(w);
			}
			else
				w = jmax(min, w);

			auto visible = (bool)c.getProperty("Visible", true);

			int flag = 0;
			flag |= TableHeaderComponent::ColumnPropertyFlags::visible;

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

void ScriptTableListModel::LookAndFeelMethods::drawTableCell(Graphics& g, const LookAndFeelData& d, const String& text, int rowNumber, int columnId, int width, int height, bool rowIsSelected)
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
	LookAndFeelData d;

	if (TableListBox* table = h.findParentComponentOfClass<TableListBox>())
	{
		if (auto st = dynamic_cast<ScriptTableListModel*>(table->getModel()))
		{
			d = st->d;
		}
	}

	g.setColour(d.itemColour2);
	g.fillAll();
}



void ScriptTableListModel::LookAndFeelMethods::drawDefaultTableHeaderColumn(Graphics& g, TableHeaderComponent& h, const String& columnName, int columnId, int width, int height, bool isMouseOver, bool isMouseDown, int columnFlags)
{
	LookAndFeelData d;

	if (TableListBox* table = h.findParentComponentOfClass<TableListBox>())
	{
		if (auto st = dynamic_cast<ScriptTableListModel*>(table->getModel()))
		{
			d = st->d;
		}
	}

	g.setFont(d.f);
	Rectangle<int> a(0, 0, width - 1, height);

	g.setColour(d.itemColour1);
	g.fillRect(a);

	g.setColour(d.textColour);
	g.drawText(columnName, a.toFloat().reduced(3.0f), d.c);
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

	if (isPositiveAndBelow(columnId, columnMetadata.size()))
	{
		if (auto obj = columnMetadata[columnId].getDynamicObject())
		{
			auto t = obj->getProperty(scriptnode::PropertyIds::Type).toString();

			static const StringArray types =
			{
				"Text",
				"Button",
				"Image",
				"Slider"
			};

			auto type = (CellType)types.indexOf(t);

			cellTypes.set(columnId, type);
			return type;
		}
	}

	jassertfalse;
	return CellType::numCellTypes;
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

void ScriptTableListModel::cellDoubleClicked(int rowNumber, int columnId, const MouseEvent& e)
{
	TableListBoxModel::cellDoubleClicked(rowNumber, columnId, e);
	sendCallback(rowNumber, 0, rowData[rowNumber], EventType::DoubleClick);
}

void ScriptTableListModel::backgroundClicked(const MouseEvent& e)
{
	TableListBoxModel::backgroundClicked(e);
	sendCallback(-1, -1, var(Array<var>()), EventType::Selection);
}

void ScriptTableListModel::selectedRowsChanged(int lastRowSelected)
{
	TableListBoxModel::selectedRowsChanged(lastRowSelected);
	sendCallback(lastRowSelected, 0, rowData[lastRowSelected], EventType::Selection);
}

void ScriptTableListModel::deleteKeyPressed(int lastRowSelected)
{
	TableListBoxModel::deleteKeyPressed(lastRowSelected);
	sendCallback(lastRowSelected, 0, rowData[lastRowSelected], EventType::DeleteRow);
}

void ScriptTableListModel::returnKeyPressed(int lastRowSelected)
{
	TableListBoxModel::returnKeyPressed(lastRowSelected);
	sendCallback(lastRowSelected, 0, rowData[lastRowSelected], EventType::ReturnKey);
}

void ScriptTableListModel::setRowData(var rd)
{
	rowData = rd;
	tableRefreshBroadcaster.sendMessage(sendNotificationAsync, -1);
}

void ScriptTableListModel::setCallback(var callback)
{
	if (HiseJavascriptEngine::isJavascriptFunction(callback))
	{
		cellCallback = WeakCallbackHolder(pwsc, callback, 1);
		cellCallback.incRefCount();
	}
}

void ScriptTableListModel::sendCallback(int rowId, int columnId, var value, EventType type)
{
	if (cellCallback)
	{
		auto obj = new DynamicObject();

		obj->setProperty("rowIndex", rowId);
		obj->setProperty("columnIndex", columnId - 1);
		obj->setProperty("value", value);

		switch (type)
		{
		case EventType::SliderCallback:
			obj->setProperty("Type", "Slider");
			break;
		case EventType::ButtonCallback:
			obj->setProperty("Type", "Button");
			break;
		case EventType::Selection:
			obj->setProperty("Type", "Selection");
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
		}

		var a(obj);
		cellCallback.call1(a);
	}
}

} 
