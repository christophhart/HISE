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

#ifndef __MAINCOMPONENT_H_CE2CB2E__
#define __MAINCOMPONENT_H_CE2CB2E__


namespace hise { using namespace juce;



class FileNameValuePropertyComponent : public PropertyComponent
{
public:

	struct MyFunkyFilenameComponent : public Component,
		public ButtonListener,
		public TextEditor::Listener
	{
		MyFunkyFilenameComponent(FileNameValuePropertyComponent& p) :
			parent(p),
			browseButton("Browse")
		{
			addAndMakeVisible(&editor);
			editor.addListener(this);
			editor.setFont(GLOBAL_BOLD_FONT());
			editor.setSelectAllWhenFocused(true);

			editor.setTextToShowWhenEmpty("No folder selected", Colours::grey);

			addAndMakeVisible(&browseButton);
			browseButton.addListener(this);
			browseButton.setLookAndFeel(&alaf);
		}

		void buttonClicked(Button*) override
		{
			FileChooser fileChooser("Select Folder");

			if (fileChooser.browseForDirectory())
			{
				parent.v = fileChooser.getResult().getFullPathName();
				parent.refresh();
			}
		}

		void textEditorReturnKeyPressed(TextEditor&) override
		{
			updateFromTextEditor();
		}

		void updateFromTextEditor();

		void textEditorFocusLost(TextEditor&) override
		{
			updateFromTextEditor();
		}

		void textEditorTextChanged(TextEditor&) override
		{

		}

		void resized() override
		{
			auto area = getLocalBounds();

			browseButton.setBounds(area.removeFromRight(60));
			area.removeFromRight(5);
			editor.setBounds(area);
		}

		FileNameValuePropertyComponent& parent;

		TextEditor editor;
		TextButton browseButton;

		AlertWindowLookAndFeel alaf;
	};

	FileNameValuePropertyComponent(const String& name, File initialFile, Value v_) :
		PropertyComponent(name),
		fc(*this),
		v(v_)
	{
		addAndMakeVisible(fc);
	}

	void refresh() override
	{
		fc.editor.setText(v.getValue().toString(), dontSendNotification);
	}



	MyFunkyFilenameComponent fc;
	Value v;
};


class TableSlider: public Slider
{
public:
	TableSlider(int index):
		Slider(),
		indexMod(index % 12)
	{
		bool isBlack = false;

		switch(indexMod)
		{
		case 1:
		case 3:
		case 6:
		case 8:
		case 10: isBlack = true; 
				 break;
		default: break;
		}
		

		setRange (0, 1, 0);
		setSliderStyle (Slider::LinearBarVertical);
		setTextBoxStyle (Slider::NoTextBox, true, 80, 20);
		setColour (Slider::thumbColourId, isBlack ? Colour (0x93000000) : Colour (0x73ffffff));
		setColour (Slider::backgroundColourId, isBlack ? Colour (0x03000000) : Colour (0x15ffffff));
		setColour (Slider::ColourIds::textBoxOutlineColourId, Colours::white.withAlpha(0.05f));
		setColour (Slider::ColourIds::rotarySliderOutlineColourId, Colours::white.withAlpha(0.05f));
	};

	void mouseWheelMove (const MouseEvent& e, const MouseWheelDetails& wheel) override
    {
		if (e.mods.isCtrlDown())
		{
			Slider::mouseWheelMove(e, wheel);

		}

    }


private:

	int indexMod;

};

struct TableColumn: public Component
{
	TableColumn(int index_):
		index(index_),
		selected(false),
		pressed(false)
	{

		

		addAndMakeVisible(slider = new TableSlider(index));
	};

	void paint(Graphics &g)
	{
		g.setColour(selected ? Colours::white.withAlpha(0.1f) : Colours::transparentBlack);
		g.fillAll();

		g.setColour(pressed ? Colours::red.withAlpha(0.1f) : Colours::transparentBlack);
		g.fillRoundedRectangle(0.0f, 0.0f, (float)getWidth(), (float)getHeight(), 4.0f);

		g.setColour (Colour (0x63ffffff));
		g.setFont (GLOBAL_FONT());
		
		static String noteNames[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };

		g.drawText(String(noteNames[index % 12]), 0, 5, getWidth(), getHeight(), Justification::centredTop, false);
	}

	
	void setSelected(bool shouldBeSelected)
	{
		selected = shouldBeSelected;
		repaint();
	};

	void setPressed(bool isPressed)
	{
		pressed = isPressed;
		repaint();
	};

	void resized()
	{
		
		slider->setBounds(0, 20, getWidth(), getHeight() - 20);
	};

	int index;

	bool selected;
	bool pressed;

	ScopedPointer<TableSlider> slider;
};



class DiscreteTableEditor: public Component,
						   public SliderListener,
						   public SafeChangeBroadcaster
{
public:

	DiscreteTableEditor(Table *tableToBeEdited):
		table(static_cast<DiscreteTable*>(tableToBeEdited))
	{
		// Only Discrete Tables are allowed!
		jassert(table != nullptr);

		addAndMakeVisible(viewport = new Viewport());
		viewport->setViewedComponent(content = new Component(), false);

		viewport->setScrollBarsShown(false, true, false, false);

		

		

		setSliderAmount(128);

		
	}

	void sliderValueChanged(Slider *s) override
	{
		for(int i = 0; i < valueSliders.size(); i++)
		{
			if(s == valueSliders[i]->slider)
			{
				table->setValue(i, (float)s->getValue());
			};
		}

		sendChangeMessage();
	};

	void paint(Graphics &g) override
	{
		g.setColour(Colours::lightgrey.withAlpha(0.1f));
		g.drawRect(getLocalBounds(), 1);
	}

	float getMaximumInSelection() const
	{
		float x = 0.0f;

		for(int i = selectedRange.getStart(); i < selectedRange.getEnd(); i++)
		{
			x = jmax<float>(x, (float)valueSliders[i]->slider->getValue());
		}

		return x;

	};

	void setDeltaValue(float delta)
	{
		jassert(dragStartValues.size() == selectedRange.getLength());

		for(int i = 0; i < dragStartValues.size(); i++)
		{
			const float newValue = jlimit<float>(0.0f, 1.0f, dragStartValues[i] + delta);

			valueSliders[i + selectedRange.getStart()]->slider->setValue(newValue, sendNotificationSync);
		}

	}

	/** Saves all values at the drag start. */
	void startDrag()
	{
		dragStartValues.clear();
		dragStartValues.ensureStorageAllocated(selectedRange.getLength());

		for(int i = selectedRange.getStart(); i < selectedRange.getEnd(); i++)
		{
			dragStartValues.add((float)valueSliders[i]->slider->getValue());
		}

	}

	void endDrag()
	{
		dragStartValues.clear();
	}


	void setSelected(Range<int> newSelection)
	{
		selectedRange = newSelection;
		updateSelection();
	}

	void updateSelection()
	{
		

		for(int i = 0; i < valueSliders.size(); i++)
		{
			valueSliders[i]->setSelected(selectedRange.contains(i));
		}
	}

	void setSliderAmount(int amount)
	{
		valueSliders.clear();

		for(int i = 0; i < amount; i++)
		{
			TableColumn *s = new TableColumn(i);
			content->addAndMakeVisible(s);
			s->slider->addListener (this);
			valueSliders.add(s);
			s->slider->setValue(table->get(i), dontSendNotification);
		}
	}

	void resized() override
	{
		int x = 0;
		//int w = getWidth() / valueSliders.size();
		int w = 20;



		for (int i = 0; i < valueSliders.size(); i++)
		{
			valueSliders[i]->setBounds(x, 0, w + 1, getHeight() - 20);

			x += w;
		}

		content->setSize(x, getHeight());
		viewport->setSize(getWidth(), getHeight());
	}

	void setCurrentKey(int newKey)
	{
		valueSliders[abs(newKey)]->setPressed(newKey >= 0);
	}

	void scrollToKey(int newKey)
	{
		if (newKey > 0) viewport->setViewPositionProportionately((double)newKey / 127.0f, 0.0);
	}

private:

	Array<float> dragStartValues;

	Range<int> selectedRange;

	DiscreteTable *table;

	OwnedArray<TableColumn> valueSliders;

	ScopedPointer<Component> content;

	ScopedPointer<Viewport> viewport;

};

class Processor;

//==============================================================================
/** A editor for the Table object, which represents a curve with different resolutions. 
*	@ingroup hise_ui
*/
class TableEditor : public Component,
	public SettableTooltipClient,
	public SafeChangeListener,
	public CopyPasteTarget,
	public Table::Updater::Listener
{
public:

	struct LookAndFeelMethods
	{
        virtual ~LookAndFeelMethods() {};
        
		virtual void drawTablePath(Graphics& g, TableEditor& te, Path& p, Rectangle<float> area, float lineThickness) = 0;
		virtual void drawTablePoint(Graphics& g, TableEditor& te, Rectangle<float> tablePoint, bool isEdge, bool isHover, bool isDragged) = 0;
		virtual void drawTableRuler(Graphics& g, TableEditor& te, Rectangle<float> area, float lineThickness, double rulerPosition) = 0;
	};

	struct HiseTableLookAndFeel : public LookAndFeel_V3,
		public LookAndFeelMethods
	{
        virtual ~HiseTableLookAndFeel() {};
        
		void drawTablePath(Graphics& g, TableEditor& te, Path& p, Rectangle<float> area, float lineThickness) override;
		void drawTablePoint(Graphics& g, TableEditor& te, Rectangle<float> tablePoint, bool isEdge, bool isHover, bool isDragged) override;
		void drawTableRuler(Graphics& g, TableEditor& te, Rectangle<float> area, float lineThickness, double rulerPosition) override;
	};

	struct FlatTableLookAndFeel : public LookAndFeel_V3,
								  public LookAndFeelMethods
	{
        virtual ~FlatTableLookAndFeel() {};
        
		void drawTablePath(Graphics& g, TableEditor& te, Path& p, Rectangle<float> area, float lineThickness) override;
		void drawTablePoint(Graphics& g, TableEditor& te, Rectangle<float> tablePoint, bool isEdge, bool isHover, bool isDragged) override;
		void drawTableRuler(Graphics& g, TableEditor& te, Rectangle<float> area, float lineThickness, double rulerPosition) override;
	};

	/** This listener can be used to react on user interaction to display stuff.
	*
	*	It shouldn't be used for any kind of serious data processing though...
	*/
	struct Listener
	{
		virtual ~Listener() {};

		/** Will be called when the user starts dragging a point. */
		virtual void pointDragStarted(Point<int> position, float index, float value) = 0;

		/** Will be called when the user stops dragging a point. */
		virtual void pointDragEnded() = 0;

		/** Called while the point is being dragged. */
		virtual void pointDragged(Point<int> position, float index, float value) = 0;

		/** Called when the user changes a curve. The position will be the middle between the points. */
		virtual void curveChanged(Point<int> position, float curveValue) = 0;

	private:

		JUCE_DECLARE_WEAK_REFERENCEABLE(Listener);
	};

	enum ColourIds
	{
		bgColour = 1024,
		lineColour,
		fillColour,
		rulerColour,
		overlayTextId,
		overlayBgColour,
		numColourIds
	};

	/** This allows different domain types, eg. time */
	enum DomainType
	{
		originalSize = 0, ///< the original table size is used
		normalized, ///< the table size is normalized to float between 0.0 and 1.0
		scaled ///< the domain is scaled with a given Range<int> object
	};

	SET_GENERIC_PANEL_ID("TableEditor");

	/** Creates a editor for an existing table.
	*
	*	The lifetime of the table must be longer than the editor's lifetime.
	*/
	explicit TableEditor(UndoManager* undoManager, Table *tableToBeEdited=nullptr);

	~TableEditor();

	void paint(Graphics&) override;

	/** If you resize the TableEditor, all internal DragPoints are deleted and new created. */
	void resized() override;

	Table *getEditedTable() const
	{
		return editedTable.get();
	}

	void setEditedTable(Table *newTable)
	{
		editedTable = newTable;

		if (editedTable != nullptr)
		{
			editedTable->addChangeListener(this);

			createDragPoints();
			refreshGraph();
		}
	}

	void indexChanged(float newIndex) override
	{
		setDisplayedIndex(newIndex);
	}

	String getObjectTypeName() override
	{ 
		return "Table Data"; 
	};

	void copyAction() override { SystemClipboard::copyTextToClipboard(getEditedTable()->exportData()); };
	virtual void pasteAction() override
	{
		const String data = SystemClipboard::getTextFromClipboard();
		getEditedTable()->restoreData(data);

		createDragPoints();
		refreshGraph();
	}

	void changeListenerCallback(SafeChangeBroadcaster *b) override;;

	void connectToLookupTableProcessor(Processor *p, int tableIndex=-1);;

	/** Set the display of the domain value to the desired type. If you want a scaled value to be displayed, pass a Range<int> object */
	void setDomain(DomainType newDomainType, Range<int> newRange=Range<int>());

	/** Sets the edges to read only.
	*
	*	If you pass anything else than -1.0f here, the edge will be read only, so it can't be dragged around.
	*/
	void setReadOnlyEdge(float constantLeftEdge, float constantRightEdge)
	{
		drag_points.getFirst()->setConstantValue(constantLeftEdge);
		drag_points.getLast()->setConstantValue(constantRightEdge);
	};


	void setUseFlatDesign(bool shouldUseFlatDesign)
	{
		if (shouldUseFlatDesign)
			currentLAF = new FlatTableLookAndFeel();
		else
			currentLAF = new HiseTableLookAndFeel();

		setLookAndFeel(currentLAF.get());
		setTableLookAndFeel(dynamic_cast<LookAndFeelMethods*>(currentLAF.get()), false);

		repaint();
	}

	void setLineThickness(float newLineThickness)
	{
		lineThickness = newLineThickness;
		repaint();
	}


	void setFont(Font newFont)
	{
		fontToUse = newFont;
	}

	void setSnapValues(var snapArray)
	{
		if (auto ar = snapArray.getArray())
		{
			snapValues.clear();

			for (const auto& v : *ar)
			{
				snapValues.add((float)v);
			}
		}
	}

	/** A left mouse click creates a new DragPoint or selects the DragPoint under the mouse which can be dragged.
	*	
	*	A right mouse click on a point deletes it.
	*/
	void mouseDown(const MouseEvent &e) override;

	void mouseDoubleClick(const MouseEvent& event) override;

	/** If the table size is bigger than 5000, it only recalculates the lookup table here to save some processing */
	void mouseUp(const MouseEvent &e) override;

	/** Updates the graph and the points. If the table size is smaller than 5000, it also refreshes the look up table. */
	void mouseDrag(const MouseEvent &e) override;

	void changePointPosition(int index, int x, int y, bool useUndoManager=false)
	{
		if (index == -1 || index >= drag_points.size())
			return;

		if (undoManager != nullptr && useUndoManager)
		{
			auto oldPos = drag_points[index]->getPos();

			undoManager->perform(new TableAction(this, TableAction::Drag, index, x, y, 0.0f, oldPos.getX(), oldPos.getY(), 0.0f));
		}
		else
		{
			drag_points[index]->changePos(Point<int>(x, y));

			updateTouchOverlayPosition();

			updateTable(false); // Only update small tables while dragging
			refreshGraph();

			needsRepaint = true;
			repaint();
		}
	}

	/** If you move the mouse wheel over a point, you can adjust the curve to the left of the point */
	void mouseWheelMove(const MouseEvent &e, const MouseWheelDetails &wheel)  override;

	void setScrollWheelEnabled(bool shouldBeEnabled)
	{
		allowScrollWheel = shouldBeEnabled;
	}

	void updateCurve(int x, int y, float newCurveValue, bool useUndoManager);

	void updateFromProcessor(SafeChangeBroadcaster* b);

	void removeProcessorConnection();

	bool isInMainPanel() const;

#if !HI_REMOVE_HISE_DEPENDENCY_FOR_TOOL_CLASSES
	bool isInMainPanelInternal() const;
#endif

	/** You can set a value which is displayed as input here. If the value is changed, the table will be repainted. 
	*
	*	The range of newIndex is 0.0 - 1.0.
	*/
	void setDisplayedIndex(float newIndex)
	{
		ruler->setIndex(newIndex);
		
	};

	/** \brief Sets the point at the left or right edge to the new value
	 *  
	 * \param f            the new value     
	 * \param setLeftEdge  change the left edge.
	 */  
	void setEdge(float f, bool setLeftEdge=true);

	void paintOverChildren(Graphics& g)
	{
		CopyPasteTarget::paintOutlineIfSelected(g);
	}

	void addListener(Listener* l)
	{
		listeners.addIfNotAlreadyThere(l);
	}

	void removeListener(Listener* l)
	{
		listeners.removeAllInstancesOf(l);
	}


	String getPopupString(float x, float y);
	std::function<String(float, float)> popupFunction;

	void setTableLookAndFeel(LookAndFeelMethods* lm, bool isExternalLaf)
	{
		if(isExternalLaf || !externalLaf)
		{
			lafToUse = lm;
			externalLaf = isExternalLaf;
		}
	}

private:

	bool externalLaf = false;
	LookAndFeelMethods* lafToUse = nullptr;

	ScopedPointer<LookAndFeel> currentLAF;

	class Ruler : public Component
	{
	public:

		Ruler() :
			value(0.0f)
		{
			setInterceptsMouseClicks(false, false);
		};

		void paint(Graphics &g);
		
		void setIndex(float newIndex)
		{
			if (newIndex != value)
			{
				value = newIndex;

				repaint();
			}
		};

	private:

		GUIUpdater updater;

		float value;

	};

	//==============================================================================
	/** This is a point with a handle that can be dragged around to edit the plot.
	*
	*	It has a Table::GraphPoint member and some GUI functions (test for mouse-over and scaling)
	*/
	class DragPoint    : public Component
	{
	public:

		/** Initializes a new DragPoint. The curve is linear */
		DragPoint(bool isStart, bool isEnd);

		/** This makes a read only point which can't be dragged around anymore. It only makes sense for start or end points.
		*	
			@param newConstantValue the new constant value. If you want to make it variable again, pass -1.
		*/
		void setConstantValue(float newConstantValue)
		{
			jassert(isStartOrEnd());

			constantValue = newConstantValue;
		}

		~DragPoint();

		void paint (Graphics&);
		void resized();
	
		/** Returns the scaled position in the TableEditor. */
		Point<int> getPos() const
		{
			//jassert( !dragPlotSize.isEmpty() );

			const int x_pos = (int)(normalizedGraphPoint.x * dragPlotSize.getWidth());
			const int y_pos = (int)((1.0f - normalizedGraphPoint.y) * dragPlotSize.getHeight());

			return Point<int>(x_pos,y_pos);
		};
	
		/** Changes the position of the DragPoint
		*
		*	Use this when you drag the point around so it can check whether a point should be moved and how.
		*/
		void changePos(Point<int> newPosition)
		{
			jassert( !dragPlotSize.isEmpty() );

			// Check if the x value can be changed
			if(! isStartOrEnd()) normalizedGraphPoint.x = (float)newPosition.getX() / (float)dragPlotSize.getWidth();

			// Check if the y value should be changed.
			if(constantValue == -1.0f) normalizedGraphPoint.y = 1.0f - ((float)newPosition.getY() / (float)dragPlotSize.getHeight());
		
			this->setCentrePosition(getPos().getX(), getPos().getY());
		}

		/** Sets up the position of the DragPoint in the TableEditor. It doesn't check if a point is start or end, so be careful!
		*
		*/
		void setPos(Point<int> newPosition)
		{
			//jassert( !dragPlotSize.isEmpty() );

			normalizedGraphPoint.x = (float)newPosition.getX() / (float)dragPlotSize.getWidth();
			normalizedGraphPoint.y = 1.0f - ((float)newPosition.getY() / (float)dragPlotSize.getHeight());

			this->setCentrePosition(getPos().getX(), getPos().getY());
		};

		void setPos(Point<float> normalizedPoint)
		{
			jassert(!dragPlotSize.isEmpty());

			normalizedGraphPoint.x = normalizedPoint.x;
			normalizedGraphPoint.y = normalizedPoint.y;

			this->setCentrePosition(getPos().getX(), getPos().getY());
		}

		/* sets the curve */
		void updateCurve(float newCurveValue)
		{
			normalizedGraphPoint.curve += (newCurveValue * 0.1f);
			normalizedGraphPoint.curve = jlimit<float>(0.0f, 1.0f, normalizedGraphPoint.curve);
		};

		void setCurve(float newValue)
		{
			normalizedGraphPoint.curve = jlimit<float>(0.0f, 1.0f, newValue);
		}

		float getCurve() const
		{
			return normalizedGraphPoint.curve;
		}

		/** Saves the TableEditor size for scaling. Call this method whenever you resize the TableEditor */
		void setTableEditorSize(int width, int height)
		{
			dragPlotSize = Rectangle<int>(width, height);
		}

		void mouseEnter(const MouseEvent &){over = true;repaint();};
		void mouseExit(const MouseEvent &){over = false;repaint();};

		bool isStartOrEnd() const { return ( isStart || isEnd ); };

		/** Returns the reference to the internal graph point. This is const, so you can't change it */
		const Table::GraphPoint &getGraphPoint() const { return normalizedGraphPoint; }

	private:
		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DragPoint)
	
		WeakReference<DragPoint>::Master masterReference;
		friend class WeakReference<DragPoint>;

		Rectangle<int> dragPlotSize;

		bool over;

		float constantValue;

		bool isStart;
		bool isEnd;

		Table::GraphPoint normalizedGraphPoint;

	};

	class DragPointComparator
	{
	public:
		DragPointComparator(){};

		static int compareElements(DragPoint *dp1, DragPoint *dp2) 
		{
			if(dp1->getGraphPoint().x < dp2->getGraphPoint().x) return -1;
			else if(dp1->getGraphPoint().x > dp2->getGraphPoint().x) return 1;
			else return 0;
		};
	};

	

	// Returns the DragPoint at the position x,y
	TableEditor::DragPoint * getPointUnder(int x, int y)
	{
		DragPoint *dp = static_cast<DragPoint*>(this->getComponentAt(x, y));

		for (int i = 0; i < drag_points.size(); i++)
		{
			if (dp == drag_points[i]) return dp;
		}

		return nullptr;
	};

	TableEditor::DragPoint* getNextPointFor(int x) const
	{
		for (int i = 0; i < drag_points.size() - 1; i++)
		{
			auto dp = drag_points[i];

			auto next = drag_points[i + 1];

			if (x >= dp->getX() && x <= next->getX())
			{
				return next;
			}
		}

		return nullptr;
	}

	

	// Adds a new DragPoint and selects it.
	void addDragPoint(int x, int y, float curve, bool isStart=false, bool isEnd=false, bool useUndoManager=false);

	// Add a drag point directly from a GraphPoint
	void addNormalizedDragPoint(Table::GraphPoint normalizedPoint, bool isStart=false, bool isEnd=false)
	{
		addDragPoint((int)(normalizedPoint.x * getWidth()), (int)((1.0f - normalizedPoint.y) * getHeight()), normalizedPoint.curve, isStart, isEnd);
	}

	// Recreates the drag points from the given Table. It automatically adds the start / end flags.
	void createDragPoints();

	// Refreshes the internal graph and repaints the TableEditor.
	void refreshGraph();

	// Updates the graph point list in the table this editor refers to. If refreshLookUpTable is true, then the look up table is also recalculated.
	void updateTable(bool refreshLookUpTable)
	{
		ScopedPointer<DragPointComparator> dpc = new DragPointComparator();

		drag_points.sort(*dpc);

		Array<Table::GraphPoint> newPoints;

		for(int i = 0; i < drag_points.size(); i++)	newPoints.add(drag_points[i]->getGraphPoint());

		if(editedTable.get() != nullptr) editedTable->setGraphPoints(newPoints, drag_points.size());

		if(refreshLookUpTable)
		{
			if(editedTable.get() != nullptr) editedTable->fillLookUpTable();
		}
	};

	int snapXValueToGrid(int x) const;

	

	Image snapshot;
	bool needsRepaint;

	int lastEditedPointIndex = -1;

	DomainType currentType;
		
	Range<int> domainRange;

	WeakReference<Table> editedTable;

	MidiTable dummyTable;

	
	Font fontToUse;

	bool allowScrollWheel = true;

	WeakReference<Processor> connectedProcessor;

	class TableAction : public UndoableAction
	{
	public:

		enum Action
		{
			Add,
			Delete,
			Drag,
			Curve,
			numActions
		};

		TableAction(TableEditor* table_, Action what_, int index_, int x_, int y_, float curve_,
			int oldX_, int oldY_, float oldCurve_) :
			table(table_),
			what(what_),
			index(index_),
			x(x_),
			y(y_),
			curve(curve_),
			oldX(oldX_),
			oldY(oldY_),
			oldCurve(oldCurve_)
		{};

		bool perform() override;

		bool undo() override;

		Component::SafePointer<TableEditor> table;
		Action what;
		int index, x, y, oldX, oldY;
		float curve, oldCurve;
	};

	class TouchOverlay : public Component,
						 public ButtonListener,
						 public SliderListener
	{
	public:

		TouchOverlay(DragPoint* point);

		void resized() override;

		void buttonClicked(Button* b) override;

		void sliderValueChanged(Slider* slider) override;

	private:

		Component::SafePointer<TableEditor> table;

		ScopedPointer<ShapeButton> deletePointButton;
		ScopedPointer<Slider> curveSlider;
	};

	

	float displayIndex;

	Path dragPath;
	
	OwnedArray<DragPoint> drag_points;

	WeakReference<DragPoint> currently_dragged_point;

	UndoManager* undoManager;

	ScopedPointer<Ruler> ruler;

	ScopedPointer<TouchOverlay> touchOverlay;

	//bool flatDesign = false;

	float lineThickness = 2.0f;

	Array<float> snapValues;

	Array<WeakReference<Listener>, CriticalSection> listeners;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TableEditor)
		void showTouchOverlay();

	void updateTouchOverlayPosition();

	void closeTouchOverlay();

	void removeDragPoint(DragPoint * dp, bool useUndoManager=false);
};


} // namespace hise

#endif  // __MAINCOMPONENT_H_CE2CB2E__
