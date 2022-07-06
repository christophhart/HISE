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

#pragma once

namespace hise { using namespace juce;

struct ScriptTableListModel : public juce::TableListBoxModel,
	public ReferenceCountedObject
{
	using Ptr = ReferenceCountedObjectPtr<ScriptTableListModel>;

	enum class EventType
	{
		SliderCallback,
		ButtonCallback,
		Selection,
		DoubleClick,
		ReturnKey,
		DeleteRow
	};

	enum class CellType
	{
		Text,
		Button,
		Image,
		Slider,
		numCellTypes
	};

	struct LookAndFeelData
	{
		Font f = GLOBAL_BOLD_FONT();
		Justification c = Justification::centredLeft;
		Colour textColour, bgColour, itemColour1, itemColour2;
	};

	struct LookAndFeelMethods
	{
		virtual ~LookAndFeelMethods() {};

		virtual void drawTableRowBackground(Graphics& g, const LookAndFeelData& d, int rowNumber, int width, int height, bool rowIsSelected);

		virtual void drawTableCell(Graphics& g, const LookAndFeelData& d, const String& text, int rowNumber, int columnId, int width, int height, bool rowIsSelected);

		void drawDefaultTableHeaderBackground(Graphics& g, TableHeaderComponent& h);

		void drawDefaultTableHeaderColumn(Graphics& g, TableHeaderComponent&, const String& columnName, int columnId, int width, int height, bool isMouseOver, bool isMouseDown, int columnFlags);
	};

	struct DefaultLookAndFeel : public GlobalHiseLookAndFeel,
		public LookAndFeelMethods
	{
		void drawTableHeaderBackground(Graphics& g, TableHeaderComponent& h) override;

		void drawTableHeaderColumn(Graphics& g, TableHeaderComponent&, const String& columnName, int columnId, int width, int height, bool isMouseOver, bool isMouseDown, int columnFlags) override;

	};

	ScriptTableListModel(ProcessorWithScriptingContent* p, const var& td);;

	int getNumRows() override
	{
		return rowData.size();
	}

	void paintCell(Graphics& g, int rowNumber, int columnId, int width, int height, bool rowIsSelected) override
	{
		columnId--;

		auto lafToUse = laf != nullptr ? laf : &fallback;

		auto s = getCellValue(rowNumber, columnId).toString();

		lafToUse->drawTableCell(g, d, s, rowNumber, columnId, width, height, rowIsSelected);
	}

	void paintRowBackground(Graphics& g, int rowNumber, int width, int height, bool rowIsSelected)
	{
		auto lafToUse = laf != nullptr ? laf : &fallback;

		lafToUse->drawTableRowBackground(g, d, rowNumber, width, height, rowIsSelected);
	};

	var getCellValue(int rowIndex, int columnIndex) const
	{
		jassert(isPositiveAndBelow(columnIndex, columnMetadata.size()));
		auto id = columnMetadata[columnIndex][scriptnode::PropertyIds::ID].toString();
		return rowData[rowIndex][Identifier(id)];
	}

	Component* refreshComponentForCell(int rowNumber, int columnId, bool isRowSelected,
		Component* existingComponentToUpdate) override;

	void setup(juce::TableListBox* t);

	CellType getCellType(int columnId) const;

	void setTableColumnData(var cd)
	{
		columnMetadata = cd;
	}

	void setRowData(var rd);

	void setCallback(var callback);

	void sendCallback(int rowId, int columnId, var value, EventType type);

	void setFont(Font f, Justification c);

	void setColours(Colour textColour, Colour bgColour, Colour itemColour1, Colour itemColour2);

	LambdaBroadcaster<int> tableRefreshBroadcaster;

	int getColumnAutoSizeWidth(int columnId) override
	{
		return columnMetadata[columnId - 1].getProperty("MaxWidth", 10000);
	}

	void cellDoubleClicked(int rowNumber, int columnId, const MouseEvent& e) override;
	void backgroundClicked(const MouseEvent&) override;
	void selectedRowsChanged(int lastRowSelected) override;
	void deleteKeyPressed(int lastRowSelected) override;
	void returnKeyPressed(int lastRowSelected) override;

private:

	mutable Array<CellType> cellTypes;

	LookAndFeelData d;

	DefaultLookAndFeel fallback;
	LookAndFeelMethods* laf = nullptr;

	var tableMetadata;
	var columnMetadata;
	var rowData;
	WeakCallbackHolder cellCallback;
	ProcessorWithScriptingContent* pwsc;
};

} // namespace hise
