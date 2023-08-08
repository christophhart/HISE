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

//==============================================================================
TableEditor::TableEditor(UndoManager* undoManager_, Table *tableToBeEdited):
	domainRange(Range<int>()),
	currentType(DomainType::originalSize),
	displayIndex(0.0f)
{
	setUseFlatDesign(false);

	if (tableToBeEdited != nullptr)
		setEditedTable(tableToBeEdited);
	else
		setEditedTable(&dummyTable);

	setSpecialLookAndFeel(&defaultLaf, false);

    // MUST BE SET!
	jassert(editedTable != nullptr);

	addAndMakeVisible(ruler = new Ruler());

	fontToUse = GLOBAL_BOLD_FONT();
	
	ruler->setAlwaysOnTop(true);

	setColour(ColourIds::bgColour, Colours::transparentBlack);
	setColour(ColourIds::fillColour, Colours::white.withAlpha(0.2f));
	setColour(ColourIds::lineColour, Colours::white);
	setColour(ColourIds::rulerColour, Colours::white);
	setColour(ColourIds::overlayTextId, Colour(0xDD000000));
	setColour(ColourIds::overlayBgColour, Colour(0xBBffffff));

	popupFunction = BIND_MEMBER_FUNCTION_2(TableEditor::getPopupString);

	if (undoManager_ != nullptr)
	{
		getEditedTable()->setUndoManager(undoManager_);
	}
}

TableEditor::~TableEditor()
{
	setEditedTable(nullptr);

	closeTouchOverlay();
}

void TableEditor::refreshGraph()
{	
	if(editedTable.get() != nullptr) editedTable->createPath(dragPath, getTableLookAndFeel()->shouldClosePath());

	auto a = getTableArea();
	dragPath.scaleToFit(a.getX(), a.getY(), a.getWidth(), a.getHeight(), false);
	needsRepaint = true;
	repaint();
}

int TableEditor::snapXValueToGrid(int x) const
{
	auto a = getTableArea();

	if (snapValues.size() == 0)
		return x;

	auto normalizedX = (x - a.getX()) / a.getWidth();

	auto snapRangeHalfWidth = 10.0f / a.getWidth();

	for (int i = 0; i < snapValues.size(); i++)
	{
		auto snapValue = (float)snapValues[i];
		auto snapRange = Range<float>(snapValue - snapRangeHalfWidth, snapValue + snapRangeHalfWidth);

		if (snapRange.contains(normalizedX))
			return a.getX() + (int)(snapValue * a.getWidth());
	}

	return x;
}

void TableEditor::mouseWheelMove(const MouseEvent &e, const MouseWheelDetails &wheel)
{
#if HISE_USE_MOUSE_WHEEL_FOR_TABLE_CURVE

	MouseEvent parentEvent = e.getEventRelativeTo(this);
	int x = parentEvent.getMouseDownPosition().getX();
	int y = parentEvent.getMouseDownPosition().getY();

	DragPoint *dp = getNextPointFor(x);

	int thisIndex = drag_points.indexOf(dp);

	int prevIndex = thisIndex - 1;

	DragPoint* pp = drag_points[prevIndex];

	bool useEvent = dp != nullptr;

	useEvent &= e.mods == scrollModifiers;

	if(useEvent)
	{
		if (getUndoManager(thisIndex != lastEditedPointIndex))
		{
			lastEditedPointIndex = thisIndex;
		}
		
		updateCurve(x, y, wheel.deltaY, true);

		if (pp != nullptr)
		{
			auto curveValue = dp->getCurve();

			auto ar = Rectangle<int>(pp->getPos(), dp->getPos());
			auto middle = ar.getCentre();

			ScopedLock sl(editListeners.getLock());

			for (auto l : editListeners)
			{
				if (l.get() != nullptr)
					l->curveChanged(middle, curveValue);
			}
		}
	}

	else getParentComponent()->mouseWheelMove(e, wheel);
#endif
};


void TableEditor::updateCurve(int x, int y, float newCurveValue, bool useUndoManager)
{
	auto dp = getNextPointFor(x);

	auto pp = getPrevPointFor(x);

	if (dp == nullptr)
		return;

	if (auto um = getUndoManager(useUndoManager))
	{
		um->perform(new TableAction(this, TableAction::Curve, -1, x, y, newCurveValue, x, y, -1.0f * newCurveValue));
	}
	else
	{
		if (pp != nullptr && pp->getGraphPoint().y > dp->getGraphPoint().y)
			newCurveValue *= -1.0f;

		dp->updateCurve(newCurveValue);
		updateTable(true);
		refreshGraph();
	}
}






juce::String TableEditor::getPopupString(float x, float y)
{
	auto xName = editedTable->getXValueText(x);
	auto yName = editedTable->getYValueText(y);

	return xName + " | " + yName;
}

juce::Rectangle<int> TableEditor::getPointAreaBetweenMouse() const
{
	return pointAreaBetweenMouse;
}



void TableEditor::addDragPoint(int x, int y, float curve, bool isStart/*=false*/, bool isEnd/*=false*/, bool useUndoManager/*=false*/)
{
	if (auto um = getUndoManager(useUndoManager))
	{
		um->perform(new TableAction(this, TableAction::Add, -1, x, y, curve, -1, -1, -1.0f));
	}
	else
	{
		DragPoint *dp = new DragPoint(isStart, isEnd);

		dp->setCurve(curve);

		dp->setTableEditorSize(getWidth(), getHeight());
		dp->setPos(Point<int>(x, y));
		addAndMakeVisible(dp);

		DragPointComparator comparator;

		drag_points.addSorted(comparator, dp);

		

		if (!(isEnd || isStart)) currently_dragged_point = nullptr; // fix #59
	}
}

void TableEditor::createDragPoints()
{
	
	//jassert(getWidth() != 0);

	drag_points.clear();

	if(editedTable.get() != nullptr) 
	{
        auto list = editedTable->getCopyOfGraphPoints();
        
        jassert(list.size() >= 2);
        
		addNormalizedDragPoint(list.getFirst(), true, false);
        
		for(int i = 1; i < list.size() - 1; ++i)
            addNormalizedDragPoint(list[i], false, false);
        
		addNormalizedDragPoint(list.getLast(), false, true);
	}
};

void TableEditor::setDisplayedIndex(float newIndex)
{
	lastIndex = newIndex;

	if (ruler != nullptr)
		ruler->setIndex(newIndex);
}

void TableEditor::setEdge(float f, bool setLeftEdge)
{
	auto a = getTableArea();

	if(setLeftEdge)	drag_points.getFirst()->changePos(Point<int>(0, (int)(a.getY() + (1.0 - f) * a.getHeight())));
	else drag_points.getLast()->changePos(Point<int>(getWidth(), (int)(a.getY() + (1.0 - f) * a.getHeight())));

	updateTable(true);
	refreshGraph();
}



void TableEditor::paint (Graphics& g)
{
	if (editedTable.get() == nullptr)
	{
		g.setFont(GLOBAL_BOLD_FONT());
		g.setColour(Colours::white.withAlpha(0.5f));
		g.drawText("No table", getLocalBounds(), Justification::centred);
		return;
	}

	if (auto l = getTableLookAndFeel())
	{
		l->drawTableBackground(g, *this, getTableArea(), this->ruler->getValue());
		l->drawTablePath(g, *this, dragPath, getTableArea(), lineThickness);
	}
		

    if (currently_dragged_point != nullptr)
    {
		auto a = getTableArea();

        DragPoint *dp = currently_dragged_point;
        
		auto text = popupFunction(dp->getGraphPoint().x, dp->getGraphPoint().y);
		int boxWidth = (int)fontToUse.getStringWidth(text) + 10;
		int boxHeight = (int)fontToUse.getHeight() + 10;

		int x_ = jlimit<int>(a.getX(), a.getRight() - boxWidth, dp->getPos().x - boxWidth / 2);
		int y_ = jlimit<int>(a.getY(), a.getBottom() - boxHeight, dp->getPos().y - 20);

		Rectangle<int> area(x_, y_, boxWidth, boxHeight);

		if (auto l = getTableLookAndFeel())
			l->drawTableValueLabel(g, *this, fontToUse, text, area);
    }
    
    g.setOpacity(isEnabled() ? 1.0f : 0.2f);
 
}

void TableEditor::Ruler::paint(Graphics &g)
{
	auto te = findParentComponentOfClass<TableEditor>();

	if (te == nullptr)
		return;

	if (auto l = te->getTableLookAndFeel())
		l->drawTableRuler(g, *te, te->getTableArea(), te->lineThickness, value);
}

void TableEditor::resized()
{
	if (editedTable.get() != nullptr)
	{
		this->removeMouseListener(this);
		this->addMouseListener(this, true);

		ruler->setBounds(0, 0, getWidth(), getHeight());

		if (getHeight() > 0 && getWidth() > 0)
		{
			snapshot = Image(Image::ARGB, getWidth(), getHeight(), true);

			createDragPoints();
			refreshGraph();
		}
	}
}



void TableEditor::graphHasChanged(int point)
{
	SafeAsyncCall::call<TableEditor>(*this, [](TableEditor& t) 
	{	
		if (t.currently_dragged_point == nullptr)
		{
			t.createDragPoints();
			t.refreshGraph();
		}
	});
}

void TableEditor::setDomain(DomainType newDomainType, Range<int> newRange)
{
	currentType = newDomainType;
	if( currentType == DomainType::scaled ) domainRange = newRange;
	else jassert ( newRange.isEmpty() );
};

void TableEditor::setSnapValues(var snapArray)
{
	if (auto ar = snapArray.getArray())
	{
		snapValues.clear();

		for (const auto& v : *ar)
		{
			DBG((float)v);
			snapValues.add((float)v);
		}
	}
}

void TableEditor::mouseDown(const MouseEvent &e)
{
	CHECK_MIDDLE_MOUSE_DOWN(e);

	if (!isEnabled()) return;

	if (e.mods.isAltDown())
		return;

	grabCopyAndPasteFocus();

	MouseEvent parentEvent = e.getEventRelativeTo(this);
	int x = parentEvent.getMouseDownPosition().getX();
	int y = parentEvent.getMouseDownPosition().getY();

	DragPoint *dp = this->getPointUnder(x, y);

	lastEditedPointIndex = drag_points.indexOf(dp);
	
	if(e.mods.isRightButtonDown() || e.mods.isCommandDown())
	{
		if (dp != nullptr)
		{
			removeDragPoint(dp);

			if (editedTable.get() != nullptr)
				editedTable->sendGraphUpdateMessage();
		}

		lastRightDragValue = (float)e.getPosition().getY();
	}
	else
	{
		if (dp != nullptr)
		{
			currently_dragged_point = dp;

			showTouchOverlay();

			ScopedLock sl(editListeners.getLock());

			for (auto l : editListeners)
			{
				if (l.get() != nullptr)
				{
					l->pointDragStarted(dp->getPosition(), dp->getGraphPoint().x, dp->getGraphPoint().y);
				}
			}
		}
		else
		{
			pointAreaBetweenMouse = {};
			x = snapXValueToGrid(x);

			addDragPoint(x, y, 0.5f, false, false, true);
		}
	}

	updateTable(false);
	refreshGraph();

	needsRepaint = true;
	repaint();
}


void TableEditor::removeDragPoint(DragPoint * dp, bool useUndoManager)
{
	if (!dp->isStartOrEnd())
	{
		if (auto um = getUndoManager(useUndoManager))
		{
			int x = dp->getBoundsInParent().getCentreX();
			int y = dp->getBoundsInParent().getCentreY();

			um->perform(new TableAction(this, TableAction::Delete, -1, -1, -1, -1, x, y, dp->getCurve()));
		}
		else
		{
			drag_points.remove(drag_points.indexOf(dp));

			updateTable(true);
			refreshGraph();

			needsRepaint = true;
			repaint();
		}


	}
}

void TableEditor::mouseDoubleClick(const MouseEvent& e)
{
	if (!isEnabled()) return;

	MouseEvent parentEvent = e.getEventRelativeTo(this);
	int x = parentEvent.getMouseDownPosition().getX();
	int y = parentEvent.getMouseDownPosition().getY();

	Component *clickedComponent = this->getComponentAt(x, y);

	if (clickedComponent != this)
	{
		DragPoint *dp = this->getPointUnder(x, y);
		if (!dp->isStartOrEnd())
		{

			drag_points.remove(drag_points.indexOf(dp));

			updateTable(true);
		}
	}
	
	updateTable(false);
	refreshGraph();

	needsRepaint = true;
	repaint();
}

void TableEditor::mouseUp(const MouseEvent& e)
{
	CHECK_MIDDLE_MOUSE_UP(e);

	if (!isEnabled()) return;

	if (e.mods.isAltDown())
		return;

	closeTouchOverlay();

	currently_dragged_point = nullptr;
	updateTable(true);

	needsRepaint = true;
	repaint();

	ScopedLock sl(editListeners.getLock());

	for (auto l : editListeners)
	{
		if (l.get() != nullptr)
			l->pointDragEnded();
	}
}

void TableEditor::mouseDrag(const MouseEvent &e)
{
	CHECK_MIDDLE_MOUSE_DRAG(e);

	if (!isEnabled()) return;

	if (e.mods.isAltDown())
		return;

	MouseEvent parentEvent = e.getEventRelativeTo(this);

	int x = parentEvent.getDistanceFromDragStartX() + parentEvent.getMouseDownPosition().getX();
	int y = parentEvent.getDistanceFromDragStartY() + parentEvent.getMouseDownPosition().getY();

	auto a = getTableArea();

	if (currently_dragged_point == nullptr)
	{
		if (e.mods.isRightButtonDown())
		{
			auto thisPos = (float)parentEvent.getPosition().getY();

			auto delta = thisPos - lastRightDragValue;

			lastRightDragValue = thisPos;

			delta /= (float)getHeight();

			delta *= -4.0f;

			updateCurve(x, y, delta, true);
		}

		return;
	}

	if (parentEvent.mods.isShiftDown()) 
		x = parentEvent.getMouseDownPosition().getX();

	x = jmin(x, (int)a.getWidth() - 1);
	y = jmin(y, (int)a.getHeight());

	x = jmax(x, 1);
	y = jmax(y, 0);

	x = snapXValueToGrid(x);

	auto index = drag_points.indexOf(currently_dragged_point);

	changePointPosition(index, x, y, true);

	ScopedLock sl(editListeners.getLock());

	for (auto l : editListeners)
	{
		if (l.get() != nullptr)
		{
			l->pointDragged(currently_dragged_point->getPosition(), currently_dragged_point->getGraphPoint().x, currently_dragged_point->getGraphPoint().y);
		}
	}
};

void TableEditor::mouseMove(const MouseEvent& e)
{
	if (e.eventComponent != this)
	{
		pointAreaBetweenMouse = {};
	}
	else
	{
		auto pp = getPrevPointFor(e.getPosition().getX());
		auto dp = getNextPointFor(e.getPosition().getX());

		if (pp != nullptr && dp != nullptr)
		{
			pointAreaBetweenMouse = Rectangle<int>(pp->getPos(), dp->getPos());
			pointAreaBetweenMouse = pointAreaBetweenMouse.withY(0).withHeight(getHeight());
		}
		else
		{
			pointAreaBetweenMouse = {};
		}
	}

	repaint();
}

void TableEditor::mouseExit(const MouseEvent& e)
{
	pointAreaBetweenMouse = {};
	repaint();
}

void TableEditor::showTouchOverlay()
{
#if HISE_IOS
	auto mainWindow = getTopLevelComponent();
	mainWindow->addAndMakeVisible(touchOverlay = new TouchOverlay(currently_dragged_point));
	updateTouchOverlayPosition();
#endif
}



void TableEditor::closeTouchOverlay()
{
#if HISE_IOS
	if (touchOverlay != nullptr)
	{
		auto mainWindow = getTopLevelComponent();

		if (mainWindow != nullptr)
		{
			mainWindow->removeChildComponent(touchOverlay);
			touchOverlay = nullptr;
		}
	}
#endif
}

void TableEditor::updateTouchOverlayPosition()
{
#if HISE_IOS
	auto mw = getTopLevelComponent();
	auto pArea = mw->getLocalArea(this, currently_dragged_point->getBoundsInParent());
	auto tl = pArea.getCentre();
	tl.addXY(-100, -100);

	touchOverlay->setTopLeftPosition(tl);
#endif
}

//==============================================================================
TableEditor::DragPoint::DragPoint(bool isStart_, bool isEnd_):
	normalizedGraphPoint(-1, -1, 0.5),
	isStart(isStart_),
	isEnd(isEnd_),
	over(false),
	dragPlotSize(Rectangle<int>()),
	constantValue(-1.0f)
{
	if (HiseDeviceSimulator::isMobileDevice())
	{
		const int size = isStartOrEnd() ? 50 : 35;
		setSize(size, size);
	}
	else
	{
		const int size = isStartOrEnd() ? 20 : 14; 
		setSize(size, size);
	}
    
	
}

TableEditor::DragPoint::~DragPoint()
{
	masterReference.clear();
}

void TableEditor::DragPoint::paint (Graphics& g)
{
	auto te = findParentComponentOfClass<TableEditor>();

	if (te == nullptr)
		return;
	
	if(auto l = te->getTableLookAndFeel())
		l->drawTablePoint(g, *te, getLocalBounds().toFloat(), isStartOrEnd(), over, false);
}

void TableEditor::DragPoint::resized()
{
    // This method is where you should set the bounds of any child
    // components that your component contains..

}

TableEditor::TouchOverlay::TouchOverlay(DragPoint* point)
{
#if !HISE_NO_GUI_TOOLS
	table = point->findParentComponentOfClass<TableEditor>();

	addAndMakeVisible(curveSlider = new Slider());

	curveSlider->setSliderStyle(Slider::SliderStyle::LinearBarVertical);
	curveSlider->setTextBoxStyle(Slider::NoTextBox, false, 0, 0);
	curveSlider->setColour(Slider::ColourIds::backgroundColourId, Colours::transparentBlack);
	curveSlider->setColour(Slider::ColourIds::thumbColourId, Colours::white.withAlpha(0.1f));
	curveSlider->setColour(Slider::ColourIds::trackColourId, Colours::white.withAlpha(0.3f));
	curveSlider->setRange(0.0, 1.0, 0.01);
	curveSlider->setValue(point->getCurve(), dontSendNotification);

	addAndMakeVisible(deletePointButton = new ShapeButton("Delete", Colours::white.withAlpha(0.4f), Colours::white.withAlpha(0.8f), Colours::white));

	curveSlider->addListener(this);
	deletePointButton->addListener(this);

	Path p;
	p.loadPathFromData(HiBinaryData::ProcessorEditorHeaderIcons::closeIcon, sizeof(HiBinaryData::ProcessorEditorHeaderIcons::closeIcon));

	setInterceptsMouseClicks(false, true);

	deletePointButton->setShape(p, false, true, true);

	setSize(200, 200);
#endif
}


void TableEditor::TouchOverlay::resized()
{
	if (auto te = table.getComponent())
	{
		if (auto dp = te->currently_dragged_point)
		{
			deletePointButton->setVisible(!dp->isStartOrEnd());
		}
	}

	auto area = getLocalBounds();
	curveSlider->setBounds(area.removeFromLeft(40));
	deletePointButton->setBounds(area.removeFromRight(50).removeFromTop(50));
}



void TableEditor::TouchOverlay::buttonClicked(Button* /*b*/)
{
	if (auto te = table.getComponent())
	{
		if(auto dp = te->currently_dragged_point)
		{
			te->removeDragPoint(dp);
			te->closeTouchOverlay();
		}
	}
}


void TableEditor::TouchOverlay::sliderValueChanged(Slider* slider)
{
	if (auto te = table.getComponent())
	{
		if (auto dp = te->currently_dragged_point)
		{
			dp->setCurve((float)slider->getValue());
			te->updateTable(true);
			te->refreshGraph();
		}
	}
}


bool TableEditor::TableAction::perform()
{
	if (table.getComponent() == nullptr)
		return false;

	bool refresh = false;

	switch (what)
	{
	case hise::TableEditor::TableAction::Add:
		table->addDragPoint(x, y, curve, false, false, false);
		refresh = true;
		break;
	case hise::TableEditor::TableAction::Delete:
	{
		auto dp = table->getPointUnder(oldX, oldY);
		
		if (dp != nullptr)
			table->removeDragPoint(dp, false);

		refresh = true;
		break;
	}
		
	case hise::TableEditor::TableAction::Drag:
		table->changePointPosition(index, x, y, false);
		break;
	case hise::TableEditor::TableAction::Curve:
		table->updateCurve(x, y, curve, false);
		break;
	case hise::TableEditor::TableAction::numActions:
		break;
	default:
		break;
	}

	if (refresh)
	{
		table->updateTable(false);
		table->refreshGraph();

		table->needsRepaint = true;
		table->repaint();
	}

	return true;
}


bool TableEditor::TableAction::undo()
{
	if (table.getComponent() == nullptr)
		return false;

	bool refresh = false;

	switch (what)
	{
	case hise::TableEditor::TableAction::Add:
	{
		auto dp = table->getPointUnder(x, y);

		refresh = true;

		if (dp != nullptr)
			table->removeDragPoint(dp, false);

		break;
	}
	case hise::TableEditor::TableAction::Delete:
		table->addDragPoint(oldX, oldY, oldCurve, false, false, false);
		refresh = true;
		break;
	case hise::TableEditor::TableAction::Drag:
		table->changePointPosition(index, oldX, oldY, false);
		break;
	case hise::TableEditor::TableAction::Curve:
		table->updateCurve(x, y, oldCurve, false);
		break;
	case hise::TableEditor::TableAction::numActions:
		break;
	default:
		break;
	}

	if (refresh)
	{
		table->updateTable(false);
		table->refreshGraph();

		table->needsRepaint = true;
		table->repaint();
	}

	return true;
}

TableEditor::LookAndFeelMethods::~LookAndFeelMethods()
{}

bool TableEditor::LookAndFeelMethods::shouldClosePath() const
{ return true; }

TableEditor::HiseTableLookAndFeel::~HiseTableLookAndFeel()
{}

TableEditor::EditListener::~EditListener()
{}

void TableEditor::EditListener::pointDragStarted(Point<int> position, float index, float value)
{
	ignoreUnused(position, index, value);
}

void TableEditor::EditListener::pointDragEnded()
{
}

void TableEditor::EditListener::pointDragged(Point<int> position, float index, float value)
{
	ignoreUnused(position, index, value);
}

void TableEditor::EditListener::curveChanged(Point<int> position, float curveValue)
{
	ignoreUnused(position, curveValue);
}

void TableEditor::setComplexDataUIBase(ComplexDataUIBase* newData)
{
	if (auto t = dynamic_cast<Table*>(newData))
		setEditedTable(t);
}

void TableEditor::changeListenerCallback(SafeChangeBroadcaster* b)
{
	if (dynamic_cast<Table*>(b) != nullptr)
	{
		createDragPoints();

		refreshGraph();

		return;
	}
}

Table* TableEditor::getEditedTable() const
{
	return editedTable.get();
}

void TableEditor::setEditedTable(Table* newTable)
{
	if (editedTable != nullptr)
		editedTable->removeRulerListener(this);

	editedTable = newTable;

	if (editedTable != nullptr)
	{
		editedTable->addRulerListener(this);
		createDragPoints();
		refreshGraph();
		setDisplayedIndex(editedTable->getUpdater().getLastDisplayValue());
	}
}

void TableEditor::indexChanged(float newIndex)
{
	setDisplayedIndex(newIndex);
}

String TableEditor::getObjectTypeName()
{ 
	return "Table Data"; 
}

void TableEditor::setDrawTableValueLabel(bool shouldBeDisplayed)
{
	displayPopup = shouldBeDisplayed;
}

bool TableEditor::shouldDrawTableValueLabel() const
{ return displayPopup; }

void TableEditor::copyAction()
{ SystemClipboard::copyTextToClipboard(getEditedTable()->exportData()); }

void TableEditor::pasteAction()
{
	const String data = SystemClipboard::getTextFromClipboard();
	getEditedTable()->restoreData(data);

	createDragPoints();
	refreshGraph();
}

void TableEditor::setReadOnlyEdge(float constantLeftEdge, float constantRightEdge)
{
	drag_points.getFirst()->setConstantValue(constantLeftEdge);
	drag_points.getLast()->setConstantValue(constantRightEdge);
}

void TableEditor::setMargin(float newMargin)
{
	margin = newMargin;
}

Rectangle<float> TableEditor::getTableArea() const
{
	return getLocalBounds().toFloat().reduced(margin);
}

void TableEditor::setUseFlatDesign(bool shouldUseFlatDesign)
{
	useFlatDesign = shouldUseFlatDesign;
		
	repaint();
}

void TableEditor::setLineThickness(float newLineThickness)
{
	lineThickness = newLineThickness;
	repaint();
}

void TableEditor::setFont(Font newFont)
{
	fontToUse = newFont;
}

UndoManager* TableEditor::getUndoManager(bool useUndoManager)
{
	if (!useUndoManager)
		return nullptr;

	if (auto d = getEditedTable())
		return getEditedTable()->getUndoManager();

	return nullptr;
}

void TableEditor::changePointPosition(int index, int x, int y, bool useUndoManager)
{
	if (index == -1 || index >= drag_points.size())
		return;

	if (auto um = getUndoManager(useUndoManager))
	{
		auto oldPos = drag_points[index]->getPos();

		um->perform(new TableAction(this, TableAction::Drag, index, x, y, 0.0f, oldPos.getX(), oldPos.getY(), 0.0f));
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

void TableEditor::setScrollModifiers(ModifierKeys newScrollModifiers)
{
	scrollModifiers = newScrollModifiers;
}

void TableEditor::setScrollWheelEnabled(bool shouldBeEnabled)
{
	ModifierKeys mod;

	if (!shouldBeEnabled)
		mod.withFlags(ModifierKeys::commandModifier);

	setScrollModifiers(mod);
}

void TableEditor::paintOverChildren(Graphics& g)
{
	CopyPasteTarget::paintOutlineIfSelected(g);
}

void TableEditor::addEditListener(EditListener* l)
{
	editListeners.addIfNotAlreadyThere(l);
}

void TableEditor::removeEditListener(EditListener* l)
{
	editListeners.removeAllInstancesOf(l);
}

float TableEditor::getLastIndex() const
{ return jlimit(0.0f, 1.0f, lastIndex); }

TableEditor::LookAndFeelMethods* TableEditor::getTableLookAndFeel()
{
	if (auto lm = getSpecialLookAndFeel<LookAndFeelMethods>())
		return lm;

	return &defaultLaf;
}

TableEditor::Ruler::Ruler():
	value(0.0f)
{
	setInterceptsMouseClicks(false, false);
}

double TableEditor::Ruler::getValue()
{return value;}

void TableEditor::Ruler::setIndex(float newIndex)
{
	if (newIndex != value)
	{
		value = newIndex;

		SafeAsyncCall::repaint(this);
	}
}

void TableEditor::DragPoint::setConstantValue(float newConstantValue)
{
	jassert(isStartOrEnd());

	constantValue = newConstantValue;
}

Point<int> TableEditor::DragPoint::getPos() const
{
	//jassert( !dragPlotSize.isEmpty() );

	const int x_pos = (int)(normalizedGraphPoint.x * dragPlotSize.getWidth());
	const int y_pos = (int)((1.0f - normalizedGraphPoint.y) * dragPlotSize.getHeight());

	return Point<int>(x_pos,y_pos);
}

void TableEditor::DragPoint::changePos(Point<int> newPosition)
{
	jassert( !dragPlotSize.isEmpty() );

	// Check if the x value can be changed
	if(! isStartOrEnd()) normalizedGraphPoint.x = (float)newPosition.getX() / (float)dragPlotSize.getWidth();

	// Check if the y value should be changed.
	if(constantValue == -1.0f) normalizedGraphPoint.y = 1.0f - ((float)newPosition.getY() / (float)dragPlotSize.getHeight());
		
	this->setCentrePosition(getPos().getX(), getPos().getY());
}

void TableEditor::DragPoint::setPos(Point<int> newPosition)
{
	//jassert( !dragPlotSize.isEmpty() );

	normalizedGraphPoint.x = (float)newPosition.getX() / (float)dragPlotSize.getWidth();
	normalizedGraphPoint.y = 1.0f - ((float)newPosition.getY() / (float)dragPlotSize.getHeight());

	this->setCentrePosition(getPos().getX(), getPos().getY());
}

void TableEditor::DragPoint::setPos(Point<float> normalizedPoint)
{
	jassert(!dragPlotSize.isEmpty());

	normalizedGraphPoint.x = normalizedPoint.x;
	normalizedGraphPoint.y = normalizedPoint.y;

	this->setCentrePosition(getPos().getX(), getPos().getY());
}

void TableEditor::DragPoint::updateCurve(float newCurveValue)
{
	normalizedGraphPoint.curve += (newCurveValue * 0.1f);
	normalizedGraphPoint.curve = jlimit<float>(0.0f, 1.0f, normalizedGraphPoint.curve);
}

void TableEditor::DragPoint::setCurve(float newValue)
{
	normalizedGraphPoint.curve = jlimit<float>(0.0f, 1.0f, newValue);
}

float TableEditor::DragPoint::getCurve() const
{
	return normalizedGraphPoint.curve;
}

void TableEditor::DragPoint::setTableEditorSize(int width, int height)
{
	dragPlotSize = Rectangle<int>(width, height);
}

void TableEditor::DragPoint::mouseEnter(const MouseEvent& mouseEvent)
{
	if (auto te = findParentComponentOfClass<TableEditor>())
		te->pointAreaBetweenMouse = {};

	over = true;
	repaint();
}

void TableEditor::DragPoint::mouseExit(const MouseEvent& mouseEvent)
{
	over = false;
	repaint();
}

bool TableEditor::DragPoint::isStartOrEnd() const
{ return ( isStart || isEnd ); }

const Table::GraphPoint& TableEditor::DragPoint::getGraphPoint() const
{ return normalizedGraphPoint; }

TableEditor::DragPointComparator::DragPointComparator()
{}

int TableEditor::DragPointComparator::compareElements(DragPoint* dp1, DragPoint* dp2)
{
	if(dp1->getGraphPoint().x < dp2->getGraphPoint().x) return -1;
	else if(dp1->getGraphPoint().x > dp2->getGraphPoint().x) return 1;
	else return 0;
}

TableEditor::DragPoint* TableEditor::getPointUnder(int x, int y)
{
	DragPoint *dp = static_cast<DragPoint*>(this->getComponentAt(x, y));

	for (int i = 0; i < drag_points.size(); i++)
	{
		if (dp == drag_points[i]) return dp;
	}

	return nullptr;
}

TableEditor::DragPoint* TableEditor::getNextPointFor(int x) const
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

TableEditor::DragPoint* TableEditor::getPrevPointFor(int x) const
{
	for (int i = 0; i < drag_points.size() - 1; i++)
	{
		auto dp = drag_points[i];

		auto next = drag_points[i + 1];

		if (x >= dp->getX() && x <= next->getX())
		{
			return dp;
		}
	}

	return nullptr;
}

void TableEditor::addNormalizedDragPoint(Table::GraphPoint normalizedPoint, bool isStart, bool isEnd)
{
	auto a = getTableArea();
	addDragPoint((int)(a.getX() + normalizedPoint.x * a.getWidth()), (int)(a.getY() + (1.0f - normalizedPoint.y) * a.getHeight()), normalizedPoint.curve, isStart, isEnd);
}

void TableEditor::updateTable(bool refreshLookUpTable)
{
	ScopedPointer<DragPointComparator> dpc = new DragPointComparator();

	drag_points.sort(*dpc);

	Array<Table::GraphPoint> newPoints;

	for(int i = 0; i < drag_points.size(); i++)	newPoints.add(drag_points[i]->getGraphPoint());

	if(editedTable.get() != nullptr) 
		editedTable->setGraphPoints(newPoints, drag_points.size(), refreshLookUpTable);
}

TableEditor::TableAction::TableAction(TableEditor* table_, Action what_, int index_, int x_, int y_, float curve_,
	int oldX_, int oldY_, float oldCurve_):
	table(table_),
	what(what_),
	index(index_),
	x(x_),
	y(y_),
	curve(curve_),
	oldX(oldX_),
	oldY(oldY_),
	oldCurve(oldCurve_)
{}

void TableEditor::LookAndFeelMethods::drawTableBackground(Graphics& g, TableEditor& te, Rectangle<float> area, double rulerPosition)
{
	if(te.useFlatDesign)
	{
		g.setColour(te.findColour(ColourIds::bgColour));
		g.fillAll();
	}
	else
	{
		g.setColour(Colours::lightgrey.withAlpha(0.1f));
		g.drawRect(area, 1);	
	}
}

void TableEditor::LookAndFeelMethods::drawTablePath(Graphics& g, TableEditor& te, Path& p, Rectangle<float> area, float )
{
    if(te.useFlatDesign)
    {
        g.setColour(te.findColour(ColourIds::fillColour));
        g.fillPath(p);
        g.setColour(te.findColour(ColourIds::lineColour));
        g.strokePath(p, PathStrokeType(te.lineThickness));
    }
    else
    {
        GlobalHiseLookAndFeel::fillPathHiStyle(g, p, area.getWidth(), area.getHeight());
    }
}

void TableEditor::LookAndFeelMethods::drawTablePoint(Graphics& g, TableEditor& te, Rectangle<float> tablePoint, bool isEdge, bool isHover, bool isDragged)
{
    const float width = (float)tablePoint.getWidth() - 6.0f;
    const float round = width * 0.2f;
    
    if(te.useFlatDesign)
    {
        g.setColour(te.findColour(TableEditor::ColourIds::lineColour));
        g.fillRoundedRectangle(tablePoint.reduced(3.0f), round);
    }
    else
    {
        if (isEdge)
        {
            g.setColour(Colours::white.withAlpha(0.3f));
            g.drawRoundedRectangle(3.0f, 3.0f, width, width, round, isHover ? 2.0f : 1.0f);
            g.setColour(Colours::white.withAlpha(0.2f));
            g.fillRoundedRectangle(3.0f, 3.0f, width, width, round);
        }
        else
        {
            g.setColour(Colours::white.withAlpha(0.3f));
            g.drawRoundedRectangle(3.0f, 3.0f, width, width, round, isHover ? 2.0f : 1.0f);
            g.setColour(Colours::white.withAlpha(0.2f));
            g.fillRoundedRectangle(3.0f, 3.0f, width, width, round);
        }
    }
	
}

void TableEditor::LookAndFeelMethods::drawTableRuler(Graphics& g, TableEditor& te, Rectangle<float> area, float lineThickness, double rulerPosition)
{
    if(te.useFlatDesign)
    {
        auto c = te.findColour(TableEditor::ColourIds::rulerColour);

        g.setColour(c);
        g.drawLine(Line<float>((float)rulerPosition * area.getWidth(), 0.0f, (float)rulerPosition * area.getWidth(), area.getHeight()), lineThickness);
    }
    else
    {
        g.setColour(Colours::lightgrey.withAlpha(0.05f));
        g.fillRect(jmax(0.0f, (float)rulerPosition * area.getWidth() - 5.0f), 0.0f, rulerPosition == 0.0f ? 5.0f : 10.0f, area.getHeight());
        g.setColour(Colours::white.withAlpha(0.6f));
        g.drawLine(Line<float>(rulerPosition * area.getWidth(), 0.0f, rulerPosition * area.getWidth(), area.getHeight()), 0.5f);
    }
}

FileNameValuePropertyComponent::MyFunkyFilenameComponent::MyFunkyFilenameComponent(FileNameValuePropertyComponent& p,
	File::TypesOfFileToFind fileType_):
	parent(p),
	browseButton("Browse"),
	fileType(fileType_)
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

void FileNameValuePropertyComponent::MyFunkyFilenameComponent::buttonClicked(Button*)
{
	if (fileType == File::findDirectories)
	{
		FileChooser fileChooser("Select Folder");

		if (fileChooser.browseForDirectory())
		{
			parent.v = fileChooser.getResult().getFullPathName();
			parent.refresh();
		}
	}
	else
	{
		FileChooser fileChooser("Select Folder");

		if (fileChooser.browseForFileToOpen())
		{
			parent.v = fileChooser.getResult().getFullPathName();
			parent.refresh();
		}
	}
}

void FileNameValuePropertyComponent::MyFunkyFilenameComponent::textEditorReturnKeyPressed(TextEditor& textEditor)
{
	updateFromTextEditor();
}

void FileNameValuePropertyComponent::MyFunkyFilenameComponent::textEditorFocusLost(TextEditor& textEditor)
{
	updateFromTextEditor();
}

void FileNameValuePropertyComponent::MyFunkyFilenameComponent::textEditorTextChanged(TextEditor& textEditor)
{

}

void FileNameValuePropertyComponent::MyFunkyFilenameComponent::resized()
{
	auto area = getLocalBounds();

	browseButton.setBounds(area.removeFromRight(60));
	area.removeFromRight(5);
	editor.setBounds(area);
}

FileNameValuePropertyComponent::FileNameValuePropertyComponent(const String& name, File initialFile,
	File::TypesOfFileToFind ft, Value v_):
	PropertyComponent(name),
	fc(*this, ft),
	v(v_)
{
	addAndMakeVisible(fc);
}

void FileNameValuePropertyComponent::refresh()
{
	fc.editor.setText(v.getValue().toString(), dontSendNotification);
}

void FileNameValuePropertyComponent::MyFunkyFilenameComponent::updateFromTextEditor()
{
	auto t = editor.getText();

	if (t.isEmpty() || (File::isAbsolutePath(t) && File(t).isDirectory()))
	{
		parent.v = editor.getText();
	}
}

void TableEditor::LookAndFeelMethods::drawTableValueLabel(Graphics& g, TableEditor& te, Font f, const String& text, Rectangle<int> textBox)
{
    if(!te.shouldDrawTableValueLabel())
        return;
    
	g.setFont(f);
	g.setColour(te.findColour(TableEditor::ColourIds::overlayBgColour));
	g.fillRect(textBox);
	g.setColour(te.findColour(TableEditor::ColourIds::overlayTextId));
	g.drawRect(textBox, 1);
	g.drawText(text, textBox, Justification::centred, false);
}

} // namespace hise
