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
public Button::Listener,
public ApiProviderBase::ApiComponentBase
{
public:
    
    enum ColumnId
    {
		Expand=1,
        Type,
		DataType,
        Name,
        Value,
        numColumns
    };
    
	var getColumnVisiblilityData() const;

	void restoreColumnVisibility(const var& d);

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
    
	void textEditorTextChanged(TextEditor& );

    void setResizeOnChange(bool shouldResize);

    bool keyPressed(const KeyPress&) override;

    int getNumRows() override;
    void paintRowBackground (Graphics& g, int rowNumber, int /*width*/, int /*height*/, bool rowIsSelected) override;
    void selectedRowsChanged(int /*lastRowSelected*/) override;
    void paintCell (Graphics& g, int rowNumber, int columnId, int width, int height, bool /*rowIsSelected*/) override;
    
	void mouseWheelMove(const MouseEvent&, const MouseWheelDetails&) override;

	void mouseExit(const MouseEvent& e) override;

	static void updateFontSize(ScriptWatchTable& t, float newSize);

    String getHeadline() const;
    void resized() override;
    
    void refreshChangeStatus();

	void mouseDown(const MouseEvent& event) override;
    void mouseDoubleClick(const MouseEvent &e) override;
    
	void paint(Graphics &g) override;

	void paintOverChildren(Graphics& g) override;
	

	void providerWasRebuilt() override;

    void providerCleared() override;

	void buttonClicked(Button* b) override;

	void setLogFunction(const std::function<void(const String&)>& f);

    void setPopupFunction(const std::function<void(Component*, Component*, Point<int>)>& pf);

    void setViewDataTypes(const StringArray& names, const Array<int>& typeIds);

	var getStateObject() const;

    void fromStateObject(const var& o);

    void mouseMove(const MouseEvent& e) override;

private:
    
	struct TooltipInfo : public Timer
	{
		TooltipInfo(ScriptWatchTable& parent_);

		void timerCallback() override;

		ScriptWatchTable& parent;

		void draw(Graphics& g);

		String t;
		Point<int> tablePos;
		Rectangle<int> cellPos;
		bool ready = false;
	};

	ScopedPointer<TooltipInfo> currentTooltip;

	void refreshTimer();

    int timerspeed = 500;
	int fullRefreshFactor = 0;
	int fullRefreshCounter = 0;

	std::function<void(const String&)> logFunction;

	DebugInformationBase::Ptr getDebugInformationForRow(int rowIndex);

	bool resizeOnChange = false;

	void rebuildLines();

	struct Rebuilder : public LockfreeAsyncUpdater
	{
		Rebuilder(ScriptWatchTable* parent_);;

		void handleAsyncUpdate() override;

		ScriptWatchTable* parent;
	};

	Rebuilder rebuilder;

	void rebuildLinesAsync();

    void applySearchFilter();

	ScopedPointer<TextEditor> fuzzySearchBox;

	struct Info: public ReferenceCountedObject
	{
		using Ptr = ReferenceCountedObjectPtr<Info>;
		using List = ReferenceCountedArray<Info>;

		Info(DebugInformationBase::Ptr di, Info* parent=nullptr, int l = 0);

		bool forEachExpandedChildren(const std::function<bool(Ptr)>& f, bool forceChildren, bool skipSelf=false);

		bool forEachParent(const std::function<bool(Ptr)>& f);

		int type;
		String dataType, name;

		DebugInformationBase::Ptr source;

		const int level = 0;

		bool expanded = false;
		List children;
		WeakReference<Info> parent;

		String getValue();

		bool checkValueChange();

	private:

		bool valueInitialised = false;
		String value;

		JUCE_DECLARE_WEAK_REFERENCEABLE(Info);
	};

	struct Factory : public PathFactory
	{
		Path createPath(const String& url) const override;
	} factory;

	String getTextForColumn(ColumnId columnId, Info::Ptr info, bool getFullText);

    TableHeaderLookAndFeel laf;

	Info::List rootValues;
	Info::List filteredFlatList;
	
	struct ViewInfo
	{
		enum ViewProperty
		{
			Debugged,
			Pinned,
			Expanded,
		};

		enum ViewStates
		{
			AllExpanded,
			OnlyPinned,
			numViewStates
		};

		ViewInfo(ScriptWatchTable& p);;

		ScriptWatchTable& parent;

		

		

		struct ViewDataType
		{
			int typeId;
			String name;
			bool on = true;
		};

		

		bool isRoot(Info::Ptr info) const;

		bool matchesRoot(Info::Ptr info) const;

		void toggleRoot(Info::Ptr info);

		bool isTypeAllowed(Info::Ptr info) const;

		bool is(Info::Ptr info, ViewProperty p) const;

		bool isAny(ViewProperty p) const;

		bool is(ViewStates s) const;

		void addDataTypeToPopup(PopupMenu& m);

		bool performPopup(int result);

		void set(ViewStates s, bool v);

		void toggle(ViewStates s);

		void toggle(Info::Ptr info, ViewProperty p);

		void clear(ViewProperty p);

		void clear();

		void importViewSettings(var data);

		var exportViewSettings() const;

		StringArray debugNames;
		StringArray pinnedRows;
		StringArray expandedNames;
		String currentRoot;
		Array<ViewDataType> viewDataTypes;

		bool states[numViewStates] = { false };

		StringArray* d[3] = { &debugNames, &pinnedRows, &expandedNames };
	};

	bool addToFilterListRecursive(Info::Ptr p);

	ViewInfo viewInfo;
	

    BigInteger changed;
    
	std::function<void(Component* c, Component* p, Point<int>)> popupFunction;

    ScopedPointer<TableListBox> table;
    
	Info::List getSelectedInfos() const;

	HiseShapeButton refreshButton;
	HiseShapeButton menuButton;
	HiseShapeButton expandButton;
	HiseShapeButton pinButton;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScriptWatchTable);
	JUCE_DECLARE_WEAK_REFERENCEABLE(ScriptWatchTable);
    
public:
    
    Colour bgColour = Colour(0xFF262626);
};

} // namespace hise

#endif  // SCRIPTWATCHTABLE_H_INCLUDED
