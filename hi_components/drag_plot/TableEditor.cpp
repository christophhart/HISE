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
	editedTable(tableToBeEdited),
	domainRange(Range<int>()),
	currentType(DomainType::originalSize),
	displayIndex(0.0f),
	undoManager(undoManager_)
{
	if (editedTable == nullptr)
		editedTable = &dummyTable;

    // MUST BE SET!
	jassert(editedTable != nullptr);

	addAndMakeVisible(ruler = new Ruler());

	
	setColour(ColourIds::bgColour, Colours::transparentBlack);
	setColour(ColourIds::fillColour, Colours::white.withAlpha(0.2f));
	setColour(ColourIds::lineColour, Colours::white);

	if(editedTable.get() != nullptr) editedTable->addChangeListener(this);
}

TableEditor::~TableEditor()
{
	if(editedTable.get() != nullptr)
	{
		editedTable->removeChangeListener(this);
	}
	if (LookupTableProcessor *ltp = dynamic_cast<LookupTableProcessor*>(connectedProcessor.get()))
	{
		ltp->removeTableChangeListener(this);
	}

	closeTouchOverlay();
}

void TableEditor::refreshGraph()
{	
	if(editedTable.get() != nullptr) editedTable->createPath(dragPath);

	dragPath.scaleToFit(0.0f, 0.0f, (float)getWidth(), (float)getHeight(), false);

	needsRepaint = true;
	repaint();
}

int TableEditor::snapXValueToGrid(int x) const
{
	if (snapValues.size() == 0)
		return x;

	auto normalizedX = (float)x / (float)getWidth();

	auto snapRangeHalfWidth = 10.0f / (float)getWidth();

	for (int i = 1; i < snapValues.size() - 1; i++)
	{
		auto snapValue = (float)snapValues[i];
		auto snapRange = Range<float>(snapValue - snapRangeHalfWidth, snapValue + snapRangeHalfWidth);

		if (snapRange.contains(normalizedX))
			return (int)(snapValue * (float)getWidth());
	}

	return x;
}

void TableEditor::mouseWheelMove(const MouseEvent &e, const MouseWheelDetails &wheel)
{
	MouseEvent parentEvent = e.getEventRelativeTo(this);
	int x = parentEvent.getMouseDownPosition().getX();
	int y = parentEvent.getMouseDownPosition().getY();

	DragPoint *dp = getNextPointFor(x);

	int thisIndex = drag_points.indexOf(dp);

#if USE_BACKEND
	const bool useEvent = dp != nullptr && (e.mods.isCtrlDown() || findParentComponentOfClass<ProcessorEditorContainer>() == nullptr);
#else
	const bool useEvent = dp != nullptr;
#endif

	if(useEvent)
	{
		if (undoManager != nullptr && thisIndex != lastEditedPointIndex)
		{
			lastEditedPointIndex = thisIndex;
			undoManager->beginNewTransaction("Change Curve");
		}
		
		updateCurve(x, y, wheel.deltaY, true);

		if (editedTable.get() != nullptr)
			editedTable->sendSynchronousChangeMessage();
	}

	else getParentComponent()->mouseWheelMove(e, wheel);
};


void TableEditor::updateCurve(int x, int y, float newCurveValue, bool useUndoManager)
{
	auto dp = getNextPointFor(x);

	if (dp == nullptr)
		return;

	if (undoManager != nullptr && useUndoManager)
	{
		undoManager->perform(new TableAction(this, TableAction::Curve, -1, x, y, newCurveValue, x, y, -1.0f * newCurveValue));
	}
	else
	{
		dp->updateCurve(newCurveValue);
		updateTable(true);
		refreshGraph();
	}
}



void TableEditor::addDragPoint(int x, int y, float curve, bool isStart/*=false*/, bool isEnd/*=false*/, bool useUndoManager/*=false*/)
{
	if (useUndoManager && undoManager != nullptr)
	{
		undoManager->perform(new TableAction(this, TableAction::Add, -1, x, y, curve, -1, -1, -1.0f));
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
	jassert(editedTable->getNumGraphPoints() >= 2);
	//jassert(getWidth() != 0);

	drag_points.clear();

	if(editedTable.get() != nullptr) 
	{
		addNormalizedDragPoint(editedTable->getGraphPoint(0), true, false);
		for(int i = 1; i < editedTable->getNumGraphPoints() - 1; ++i) addNormalizedDragPoint(editedTable->getGraphPoint(i), false, false);
		addNormalizedDragPoint(editedTable->getGraphPoint(editedTable->getNumGraphPoints()-1), false, true);
	}
};

void TableEditor::setEdge(float f, bool setLeftEdge)
{
	if(setLeftEdge)	drag_points.getFirst()->changePos(Point<int>(0, (int)((1.0 - f) * getHeight())));
	else drag_points.getLast()->changePos(Point<int>(getWidth(), (int)((1.0 - f) * getHeight())));

	updateTable(true);
	refreshGraph();
}

void TableEditor::paint (Graphics& g)
{
	if (flatDesign)
	{
		g.setColour(findColour(ColourIds::bgColour));
		g.fillAll();
	}
	else
	{
		g.setColour(Colours::lightgrey.withAlpha(0.1f));
		g.drawRect(getLocalBounds(), 1);
	}
	
    
	if (editedTable.get() == nullptr)
	{
		g.setFont(GLOBAL_BOLD_FONT());
		g.setColour(Colours::white.withAlpha(0.5f));
		g.drawText("No table", getLocalBounds(), Justification::centred);
		return;
	}
	
	if (flatDesign)
	{
		g.setColour(findColour(ColourIds::fillColour));
		g.fillPath(dragPath);
		g.setColour(findColour(ColourIds::lineColour));
		g.strokePath(dragPath, PathStrokeType(2.0f));
	}
	else
	{
		KnobLookAndFeel::fillPathHiStyle(g, dragPath, getWidth(), getHeight());
	}
    
    
    
#if !HISE_IOS
    if (currently_dragged_point != nullptr)
    {
        int index = drag_points.indexOf(currently_dragged_point);
        
        DragPoint *dp = currently_dragged_point;
        
        g.setFont(GLOBAL_MONOSPACE_FONT());
        
        float domainValue = dp->getGraphPoint().x;
        String domainString;
        
        if (currentType == DomainType::originalSize)
        {
            domainValue = domainValue * (this->editedTable->getTableSize() - 1);
            domainString = String((int)domainValue);
        }
        else if (currentType == DomainType::scaled)
        {
            jassert(!domainRange.isEmpty());
            domainValue = domainValue * domainRange.getLength() + domainRange.getStart();
            domainString = String((int)domainValue);
        }
        else
        {
            domainString = String(domainValue, 2);
        }
        
        String text = String("#") + String(index) + ": " + domainString + ", " + String(dp->getGraphPoint().y, 2);
        
        
        g.setColour(Colour(0xBBffffff));
        juce::Rectangle<int> textBox(dp->getPos().x > getWidth() - 80 ? getWidth() - 80 : dp->getPos().x, dp->getPos().y > getHeight() - 12 ? getHeight() - 12 : dp->getPos().y, 80, 12);
        
        g.fillRect(textBox);
        g.setColour(Colour(0xDD000000));
        g.drawRect(textBox, 1);
        g.drawText(text, textBox, Justification::centred, false);
        g.setColour(Colours::darkgrey.withAlpha(0.4f));
        g.drawLine(0.0f, (float)getHeight(), (float)getWidth(), (float)getHeight());
    }
#endif
    
    g.setOpacity(isEnabled() ? 1.0f : 0.2f);
 
}

void TableEditor::Ruler::paint(Graphics &g)
{
	auto te = findParentComponentOfClass<TableEditor>();

	if (te == nullptr)
		return;

	if (te->flatDesign)
	{
		
		g.setColour(te->findColour(TableEditor::ColourIds::lineColour));
		g.drawLine(Line<float>(value * (float)getWidth(), 0.0f, value * (float)getWidth(), (float)getHeight()), 2.0f);
	}
	else
	{
		g.setColour(Colours::lightgrey.withAlpha(0.05f));
		g.fillRect(jmax(0.0f, value * (float)getWidth() - 5.0f), 0.0f, value == 0.0f ? 5.0f : 10.0f, (float)getHeight());
		g.setColour(Colours::white.withAlpha(0.6f));
		g.drawLine(Line<float>(value * (float)getWidth(), 0.0f, value * (float)getWidth(), (float)getHeight()), 0.5f);
	}


	
}

void TableEditor::resized()
{
	if (editedTable.get() != nullptr)
	{
		this->removeMouseListener(this);
		this->addMouseListener(this, true);

		ruler->setBounds(0, 0, getWidth(), getHeight());

		if (getHeight() > 0)
		{
			snapshot = Image(Image::ARGB, getWidth(), getHeight(), true);

			createDragPoints();
			refreshGraph();
		}
	}
}

void TableEditor::changeListenerCallback(SafeChangeBroadcaster *b)
{
	if (dynamic_cast<Table*>(b) != nullptr)
	{
		createDragPoints();

		refreshGraph();
	}
	else if (dynamic_cast<LookupTableProcessor::TableChangeBroadcaster*>(b) != nullptr)
	{
		LookupTableProcessor::TableChangeBroadcaster * tcb = dynamic_cast<LookupTableProcessor::TableChangeBroadcaster*>(b);

		if (tcb->table == editedTable)
		{
			setDisplayedIndex(tcb->tableIndex);
		}
	}
}

void TableEditor::connectToLookupTableProcessor(Processor *p)
{
	if (p == connectedProcessor) return;

	if (p == nullptr)
	{
		connectedProcessor = nullptr;
		createDragPoints();
		refreshGraph();
	}

	if (LookupTableProcessor * ltp = dynamic_cast<LookupTableProcessor*>(p))
	{
		connectedProcessor = p;
		ltp->addTableChangeListener(this);
	}
}

void TableEditor::setDomain(DomainType newDomainType, Range<int> newRange)
{
	currentType = newDomainType;
	if( currentType == DomainType::scaled ) domainRange = newRange;
	else jassert ( newRange.isEmpty() );
};

void TableEditor::mouseDown(const MouseEvent &e)
{
	if (!isEnabled()) return;

	SET_CHANGED_FROM_PARENT_EDITOR()
    
	grabCopyAndPasteFocus();

	MouseEvent parentEvent = e.getEventRelativeTo(this);
	int x = parentEvent.getMouseDownPosition().getX();
	int y = parentEvent.getMouseDownPosition().getY();

	DragPoint *dp = this->getPointUnder(x, y);

	lastEditedPointIndex = drag_points.indexOf(dp);
	
	if(e.mods.isLeftButtonDown())
	{
		if (dp != nullptr)
		{
			if (undoManager != nullptr)
				undoManager->beginNewTransaction("Move graph point");

			currently_dragged_point = dp;

			showTouchOverlay();
		}
		else
		{
			if (undoManager != nullptr)
				undoManager->beginNewTransaction("Add graph point");

			x = snapXValueToGrid(x);

			addDragPoint(x, y, 0.5f, false, false, true);
		}
	}
	else
	{
		if(dp != nullptr)
		{
			if (undoManager != nullptr)
				undoManager->beginNewTransaction("Remove graph point");

			removeDragPoint(dp);

			if (editedTable.get() != nullptr) 
				editedTable->sendSynchronousChangeMessage();
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

		if (undoManager != nullptr && useUndoManager)
		{
			int x = dp->getBoundsInParent().getCentreX();
			int y = dp->getBoundsInParent().getCentreY();

			undoManager->perform(new TableAction(this, TableAction::Delete, -1, -1, -1, -1, x, y, dp->getCurve()));
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

void TableEditor::mouseUp(const MouseEvent &)
{	
	if (!isEnabled()) return;

	closeTouchOverlay();

	currently_dragged_point = nullptr;
	updateTable(true);

	if(editedTable.get() != nullptr) editedTable->sendSynchronousChangeMessage();
	
	needsRepaint = true;
	repaint();
}

void TableEditor::mouseDrag(const MouseEvent &e)
{
	if (!isEnabled()) return;

	MouseEvent parentEvent = e.getEventRelativeTo(this);

	int x = parentEvent.getDistanceFromDragStartX() + parentEvent.getMouseDownPosition().getX();
	int y = parentEvent.getDistanceFromDragStartY() + parentEvent.getMouseDownPosition().getY();

	if (parentEvent.mods.isShiftDown()) x = parentEvent.getMouseDownPosition().getX();

	x = jmin(x, getWidth() - 1);
	y = jmin(y, getHeight());

	x = jmax(x, 1);
	y = jmax(y, 0);

	x = snapXValueToGrid(x);

	auto index = drag_points.indexOf(currently_dragged_point);

	changePointPosition(index, x, y, true);


};

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
	bool useFlatDesign = false;

	auto te = findParentComponentOfClass<TableEditor>();


	if (te != nullptr)
	{
		useFlatDesign = te->flatDesign;
	}
	else
		return;

    const float width = (float)getWidth() - 6.0f;
    const float round = width * 0.2f;
    
	if (useFlatDesign)
	{
		g.setColour(te->findColour(TableEditor::ColourIds::lineColour));
		g.fillRoundedRectangle(3.0f, 3.0f, width, width, round);
	}
	else
	{
		if (isStartOrEnd())
		{
			g.setColour(Colours::white.withAlpha(0.3f));
			g.drawRoundedRectangle(3.0f, 3.0f, width, width, round, over ? 2.0f : 1.0f);

			g.setColour(Colours::white.withAlpha(0.2f));
			g.fillRoundedRectangle(3.0f, 3.0f, width, width, round);

		}
		else
		{
			g.setColour(Colours::white.withAlpha(0.3f));
			g.drawRoundedRectangle(3.0f, 3.0f, width, width, round, over ? 2.0f : 1.0f);
			g.setColour(Colours::white.withAlpha(0.2f));
			g.fillRoundedRectangle(3.0f, 3.0f, width, width, round);
		}
	}

	
}

void TableEditor::DragPoint::resized()
{
    // This method is where you should set the bounds of any child
    // components that your component contains..

}

TableEditor::TouchOverlay::TouchOverlay(DragPoint* point)
{
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

} // namespace hise
