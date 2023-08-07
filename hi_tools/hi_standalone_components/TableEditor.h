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
		MyFunkyFilenameComponent(FileNameValuePropertyComponent& p, File::TypesOfFileToFind fileType_);

		void buttonClicked(Button*) override;
		void textEditorReturnKeyPressed(TextEditor&) override;
		void updateFromTextEditor();
		void textEditorFocusLost(TextEditor&) override;
		void textEditorTextChanged(TextEditor&) override;
		void resized() override;

		File::TypesOfFileToFind fileType;
		FileNameValuePropertyComponent& parent;

		TextEditor editor;
		TextButton browseButton;

		AlertWindowLookAndFeel alaf;
	};

	FileNameValuePropertyComponent(const String& name, File initialFile, File::TypesOfFileToFind ft, Value v_);

	void refresh() override;


	MyFunkyFilenameComponent fc;
	Value v;
};


class Processor;

//==============================================================================
/** A editor for the Table object, which represents a curve with different resolutions. 
*	@ingroup hise_ui
*/
class TableEditor : public Component,
	public SettableTooltipClient,
	public CopyPasteTarget,
	public Table::Listener,
	public ComplexDataUIBase::EditorBase
{
public:

	struct LookAndFeelMethods
	{
        virtual ~LookAndFeelMethods();;
     
		virtual bool shouldClosePath() const;

        virtual void drawTableBackground(Graphics& g, TableEditor& te, Rectangle<float> area, double rulerPosition);
		virtual void drawTablePath(Graphics& g, TableEditor& te, Path& p, Rectangle<float> area, float lineThickness);
		virtual void drawTablePoint(Graphics& g, TableEditor& te, Rectangle<float> tablePoint, bool isEdge, bool isHover, bool isDragged);
		virtual void drawTableRuler(Graphics& g, TableEditor& te, Rectangle<float> area, float lineThickness, double rulerPosition);

		virtual void drawTableValueLabel(Graphics& g, TableEditor& te, Font f, const String& text, Rectangle<int> textBox);
	};
    
	struct HiseTableLookAndFeel : public LookAndFeel_V3,
		public LookAndFeelMethods
	{
        virtual ~HiseTableLookAndFeel();;
	};

	/** This listener can be used to react on user interaction to display stuff.
	*
	*	It shouldn't be used for any kind of serious data processing though...
	*/
	struct EditListener
	{
		virtual ~EditListener();;

		/** Will be called when the user starts dragging a point. */
		virtual void pointDragStarted(Point<int> position, float index, float value);

		/** Will be called when the user stops dragging a point. */
		virtual void pointDragEnded();

		/** Called while the point is being dragged. */
		virtual void pointDragged(Point<int> position, float index, float value);

		/** Called when the user changes a curve. The position will be the middle between the points. */
		virtual void curveChanged(Point<int> position, float curveValue);

	private:

		JUCE_DECLARE_WEAK_REFERENCEABLE(EditListener);
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
	explicit TableEditor(UndoManager* undoManager=nullptr, Table *tableToBeEdited=nullptr);

	~TableEditor();

	void setComplexDataUIBase(ComplexDataUIBase* newData) override;

	void paint(Graphics&) override;

	/** If you resize the TableEditor, all internal DragPoints are deleted and new created. */
	void resized() override;

	void changeListenerCallback(SafeChangeBroadcaster *b);

	Table *getEditedTable() const;

	void setEditedTable(Table *newTable);

	void indexChanged(float newIndex) override;

	void graphHasChanged(int point) override;

	String getObjectTypeName() override;;

    void setDrawTableValueLabel(bool shouldBeDisplayed);

	bool shouldDrawTableValueLabel() const;

	void copyAction() override;;

	virtual void pasteAction() override;

	/** Set the display of the domain value to the desired type. If you want a scaled value to be displayed, pass a Range<int> object */
	void setDomain(DomainType newDomainType, Range<int> newRange=Range<int>());

	/** Sets the edges to read only.
	*
	*	If you pass anything else than -1.0f here, the edge will be read only, so it can't be dragged around.
	*/
	void setReadOnlyEdge(float constantLeftEdge, float constantRightEdge);;

	void setMargin(float newMargin);

	Rectangle<float> getTableArea() const;

	void setUseFlatDesign(bool shouldUseFlatDesign);

	void setLineThickness(float newLineThickness);


	void setFont(Font newFont);

	void setSnapValues(var snapArray);

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

	void mouseMove(const MouseEvent& e) override;

	void mouseExit(const MouseEvent& e) override;

	UndoManager* getUndoManager(bool useUndoManager = true);

	void changePointPosition(int index, int x, int y, bool useUndoManager=false);

	/** If you move the mouse wheel over a point, you can adjust the curve to the left of the point */
	void mouseWheelMove(const MouseEvent &e, const MouseWheelDetails &wheel)  override;

	void setScrollModifiers(ModifierKeys newScrollModifiers);

	void setScrollWheelEnabled(bool shouldBeEnabled);

	void updateCurve(int x, int y, float newCurveValue, bool useUndoManager);

    
    
	/** You can set a value which is displayed as input here. If the value is changed, the table will be repainted. 
	*
	*	The range of newIndex is 0.0 - 1.0.
	*/
	void setDisplayedIndex(float newIndex);;

	/** \brief Sets the point at the left or right edge to the new value
	 *  
	 * \param f            the new value     
	 * \param setLeftEdge  change the left edge.
	 */  
	void setEdge(float f, bool setLeftEdge=true);

	void paintOverChildren(Graphics& g);

	void addEditListener(EditListener* l);

	void removeEditListener(EditListener* l);

	String getPopupString(float x, float y);
	std::function<String(float, float)> popupFunction;

	float getLastIndex() const;

	Rectangle<int> getPointAreaBetweenMouse() const;

	LookAndFeelMethods* getTableLookAndFeel();

private:

	float margin = 0.0f;
    
    bool displayPopup = true;

	HiseTableLookAndFeel defaultLaf;

	Rectangle<int> pointAreaBetweenMouse;

	ModifierKeys scrollModifiers;

	float lastIndex = 0.0f;

	class Ruler : public Component
	{
	public:

		Ruler();;

		void paint(Graphics &g);
		
		double getValue();

		void setIndex(float newIndex);;

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
		void setConstantValue(float newConstantValue);

		~DragPoint();

		void paint (Graphics&);
		void resized();
	
		/** Returns the scaled position in the TableEditor. */
		Point<int> getPos() const;;
	
		/** Changes the position of the DragPoint
		*
		*	Use this when you drag the point around so it can check whether a point should be moved and how.
		*/
		void changePos(Point<int> newPosition);

		/** Sets up the position of the DragPoint in the TableEditor. It doesn't check if a point is start or end, so be careful!
		*
		*/
		void setPos(Point<int> newPosition);;

		void setPos(Point<float> normalizedPoint);

		/* sets the curve */
		void updateCurve(float newCurveValue);;

		void setCurve(float newValue);

		float getCurve() const;

		/** Saves the TableEditor size for scaling. Call this method whenever you resize the TableEditor */
		void setTableEditorSize(int width, int height);

		void mouseEnter(const MouseEvent &);;

		void mouseExit(const MouseEvent &);;

		bool isStartOrEnd() const;;

		/** Returns the reference to the internal graph point. This is const, so you can't change it */
		const Table::GraphPoint &getGraphPoint() const;

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
		DragPointComparator();;

		static int compareElements(DragPoint *dp1, DragPoint *dp2);;
	};

	

	// Returns the DragPoint at the position x,y
	TableEditor::DragPoint * getPointUnder(int x, int y);;

	TableEditor::DragPoint* getNextPointFor(int x) const;

	TableEditor::DragPoint* getPrevPointFor(int x) const;


	// Adds a new DragPoint and selects it.
	void addDragPoint(int x, int y, float curve, bool isStart=false, bool isEnd=false, bool useUndoManager=false);

	// Add a drag point directly from a GraphPoint
	void addNormalizedDragPoint(Table::GraphPoint normalizedPoint, bool isStart=false, bool isEnd=false);

	// Recreates the drag points from the given Table. It automatically adds the start / end flags.
	void createDragPoints();

	// Refreshes the internal graph and repaints the TableEditor.
	void refreshGraph();

	// Updates the graph point list in the table this editor refers to. If refreshLookUpTable is true, then the look up table is also recalculated.
	void updateTable(bool refreshLookUpTable);;

	int snapXValueToGrid(int x) const;

	Array<WeakReference<EditListener>, CriticalSection> editListeners;

	Image snapshot;
	bool needsRepaint;

	int lastEditedPointIndex = -1;

	DomainType currentType;
		
	Range<int> domainRange;

	WeakReference<Table> editedTable;

	SampleLookupTable dummyTable;

	float lastRightDragValue = 0.0f;
	
	Font fontToUse;

	bool allowScrollWheel = true;

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
			int oldX_, int oldY_, float oldCurve_);;

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

	ScopedPointer<Ruler> ruler;

	ScopedPointer<TouchOverlay> touchOverlay;

	bool useFlatDesign = false;

	float lineThickness = 2.0f;

	Array<float> snapValues;

    //==============================================================================
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TableEditor)
	JUCE_DECLARE_WEAK_REFERENCEABLE(TableEditor);

	void showTouchOverlay();

	void updateTouchOverlayPosition();

	void closeTouchOverlay();

	void removeDragPoint(DragPoint * dp, bool useUndoManager=false);
};


} // namespace hise

#endif  // __MAINCOMPONENT_H_CE2CB2E__
