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

	bool keyPressed(const KeyPress&) override;

    int getNumRows() override;
    void paintRowBackground (Graphics& g, int rowNumber, int /*width*/, int /*height*/, bool rowIsSelected) override;
    void selectedRowsChanged(int /*lastRowSelected*/) override;
    void paintCell (Graphics& g, int rowNumber, int columnId, int width, int height, bool /*rowIsSelected*/) override;
    
	void mouseWheelMove(const MouseEvent&, const MouseWheelDetails&) override;

	void mouseExit(const MouseEvent& e) override;

	static void updateFontSize(ScriptWatchTable& t, float newSize)
	{
		t.table->setRowHeight(roundToInt(newSize / 0.7f));
	}

    String getHeadline() const;
    void resized() override;
    
    void refreshChangeStatus();

	void mouseDown(const MouseEvent& event) override;
    void mouseDoubleClick(const MouseEvent &e) override;
    
	void paint(Graphics &g) override;

	void paintOverChildren(Graphics& g) override;
	

	void providerWasRebuilt() override
	{
		rebuildLines();
	}
    
    void providerCleared() override;

	void buttonClicked(Button* b) override;

	void setLogFunction(const std::function<void(const String&)>& f)
	{
		logFunction = f;
	}

	void setPopupFunction(const std::function<void(Component*, Component*, Point<int>)>& pf)
	{
		popupFunction = pf;
	}

	void setViewDataTypes(const StringArray& names, const Array<int>& typeIds);

	var getStateObject() const { return viewInfo.exportViewSettings(); }

	void fromStateObject(const var& o) { viewInfo.importViewSettings(o); }

	void mouseMove(const MouseEvent& e) override;

private:
    
	struct TooltipInfo : public Timer
	{
		TooltipInfo(ScriptWatchTable& parent_) :
			parent(parent_)
		{
			startTimer(800);
		}

		void timerCallback() override
		{
			ready = true;
			parent.repaint();
			stopTimer();
		}

		ScriptWatchTable& parent;

		void draw(Graphics& g);

		String t;
		Point<int> tablePos;
		Rectangle<int> cellPos;
		bool ready = false;
	};

	ScopedPointer<TooltipInfo> currentTooltip;

	void refreshTimer()
	{
		startTimer(timerspeed);
		fullRefreshCounter = 0;
	}

	int timerspeed = 500;
	int fullRefreshFactor = 0;
	int fullRefreshCounter = 0;

	std::function<void(const String&)> logFunction;

	DebugInformationBase::Ptr getDebugInformationForRow(int rowIndex);

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

		String getValue()
		{
			if (!valueInitialised && source != nullptr)
			{
				value = source->getTextForValue();
				valueInitialised = true;
			}

			return value;
		}

		bool checkValueChange()
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

		ViewInfo(ScriptWatchTable& p) : parent(p) {};

		ScriptWatchTable& parent;

		

		

		struct ViewDataType
		{
			int typeId;
			String name;
			bool on = true;
		};

		

		bool isRoot(Info::Ptr info) const
		{
			return info->name == currentRoot;
		}

		bool matchesRoot(Info::Ptr info) const
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

		void toggleRoot(Info::Ptr info)
		{
			auto newRoot = info->name;

			if (newRoot == currentRoot)
				currentRoot = {};
			else
				currentRoot = newRoot;

			parent.applySearchFilter();
		}

		bool isTypeAllowed(Info::Ptr info) const
		{
			auto t = info->type;

			for (const auto& vt : viewDataTypes)
			{
				if (vt.typeId == t)
					return vt.on;
			}

			return true;
		}

		bool is(Info::Ptr info, ViewProperty p) const
		{
			return d[p]->contains(info->name);
		}

		bool isAny(ViewProperty p) const
		{
			return !d[p]->isEmpty();
		}

		bool is(ViewStates s) const
		{
			return states[s];
		}

		void addDataTypeToPopup(PopupMenu& m)
		{
			bool on = false;
			for (const auto& d : viewDataTypes)
			{
				on |= d.on;
				m.addItem(70000 + d.typeId, d.name, true, d.on);
			}
				
			m.addItem(80000, "Toggle all", true, on);
		}

		bool performPopup(int result)
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

		void set(ViewStates s, bool v)
		{
			states[s] = v;
			parent.applySearchFilter();
		}

		void toggle(ViewStates s)
		{
			states[s] = !states[s];
			parent.applySearchFilter();
		}

		void toggle(Info::Ptr info, ViewProperty p)
		{
			if (is(info, p))
				d[p]->removeString(info->name);
			else
				d[p]->addIfNotAlreadyThere(info->name);
		}

		void clear(ViewProperty p)
		{
			d[p]->clear();
			parent.rebuildLines();
		}

		void clear()
		{
			importViewSettings(var());
		}

		void importViewSettings(var data)
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

		var exportViewSettings() const
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
