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

#ifndef SCRIPTWATCHTABLE_H_INCLUDED
#define SCRIPTWATCHTABLE_H_INCLUDED

namespace hise { using namespace juce;

class ScriptComponentEditPanel;
class ScriptComponentEditListener;

class AutoPopupDebugComponent;


class DebugInformation;
class ScriptingEditor;


/** A table component containing all variables and their current value for the currently debugged ScriptProcessor.
 *	@ingroup debugComponents
 */
class ScriptWatchTable      : public Component,
public TableListBoxModel,
public Timer,
public TextEditor::Listener,
public ApiProviderBase::ApiComponentBase
{
public:
    
    enum ColumnId
    {
        Type = 1,
		DataType,
        Name,
        Value,
        numColumns
    };
    
	enum class RefreshEvent
	{
		timerCallback = 0,
		filterTextChanged,
		recompiled,
		numRefreshEvents
	};

    ScriptWatchTable() ;
    
	SET_GENERIC_PANEL_ID("ScriptWatchTable");

    ~ScriptWatchTable();
    
    void timerCallback();
    
    void setHolder(ApiProviderBase::Holder* h);
    
	void textEditorTextChanged(TextEditor& )
	{
		applySearchFilter();
	}

	void setResizeOnChange(bool shouldResize)
	{
		resizeOnChange = shouldResize;
	}

    int getNumRows() override;
    void paintRowBackground (Graphics& g, int rowNumber, int /*width*/, int /*height*/, bool rowIsSelected) override;
    void selectedRowsChanged(int /*lastRowSelected*/) override;
    void paintCell (Graphics& g, int rowNumber, int columnId, int width, int height, bool /*rowIsSelected*/) override;
    
    String getHeadline() const;
    void resized() override;
    
    void refreshChangeStatus();

	void mouseDown(const MouseEvent& event) override;
    void mouseDoubleClick(const MouseEvent &e) override;
    
	void paint(Graphics &g) override;

	DebugInformationBase* getDebugInformationForRow(int rowIndex);

	void providerWasRebuilt() override
	{
		rebuildLines();
	}

private:
    
	bool resizeOnChange = false;

	void rebuildLines();

	struct Rebuilder : public LockfreeAsyncUpdater
	{
		Rebuilder(ScriptWatchTable* parent_):
			parent(parent_)
		{};

		void handleAsyncUpdate() override
		{
			parent->rebuildLines();
		}

		ScriptWatchTable* parent;
	};

	Rebuilder rebuilder;

	void rebuildLinesAsync()
	{
		rebuilder.triggerAsyncUpdate();
	}

	void applySearchFilter();

	ScopedPointer<TextEditor> fuzzySearchBox;

	int numFilteredDebugObjects = 0;

	struct Info
	{
		int type;
		String dataType, name, value;
	};

    TableHeaderLookAndFeel laf;

	Array<Info> allVariableLines;

	Array<int> filteredIndexes;
    BigInteger changed;
    
    ScopedPointer<TableListBox> table;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScriptWatchTable);
};

} // namespace hise

#endif  // SCRIPTWATCHTABLE_H_INCLUDED
