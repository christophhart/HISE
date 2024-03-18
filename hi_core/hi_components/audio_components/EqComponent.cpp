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



/** a component that plots a collection of filters.
@ingroup floating_tile_objects.

Just connect it to a PolyphonicFilterEffect or a CurveEQ and it will automatically update
the filter graph.
*/
class FilterGraph::Panel : public PanelWithProcessorConnection,
	public Timer
{
public:

	enum SpecialPanelIds
	{
		ShowLines = (int)PanelWithProcessorConnection::SpecialPanelIds::numSpecialPanelIds,
		GainRange,
		numSpecialPanelIds
	};

	SET_PANEL_NAME("FilterDisplay");

	Panel(FloatingTile* parent):
		PanelWithProcessorConnection(parent)
	{
		setDefaultPanelColour(FloatingTileContent::PanelColourId::bgColour, Colour(0xFF333333));
		setDefaultPanelColour(FloatingTileContent::PanelColourId::itemColour1, Colours::white);
		setDefaultPanelColour(FloatingTileContent::PanelColourId::itemColour2, Colours::white.withAlpha(0.5f));
		setDefaultPanelColour(FloatingTileContent::PanelColourId::itemColour3, Colours::white.withAlpha(0.2f));
		setDefaultPanelColour(FloatingTileContent::PanelColourId::textColour, Colours::white);
	};

	~Panel()
	{
		updater = nullptr;
	}

	int getNumDefaultableProperties() const override
	{
		return (int)SpecialPanelIds::numSpecialPanelIds;
	}

	Identifier getDefaultablePropertyId(int index) const override
	{
		if (isPositiveAndBelow(index, PanelWithProcessorConnection::SpecialPanelIds::numSpecialPanelIds))
			return PanelWithProcessorConnection::getDefaultablePropertyId(index);

		RETURN_DEFAULT_PROPERTY_ID(index, SpecialPanelIds::ShowLines, "ShowLines");
		RETURN_DEFAULT_PROPERTY_ID(index, SpecialPanelIds::GainRange, "GainRange");
		
		jassertfalse;
		return {};
	}

	var getDefaultProperty(int index) const override
	{
		if (isPositiveAndBelow(index, PanelWithProcessorConnection::SpecialPanelIds::numSpecialPanelIds))
			return PanelWithProcessorConnection::getDefaultProperty(index);

		RETURN_DEFAULT_PROPERTY(index, SpecialPanelIds::ShowLines, var(false));
		RETURN_DEFAULT_PROPERTY(index, SpecialPanelIds::GainRange, var(24.0));

		jassertfalse;

		return {};
	}

	void fromDynamicObject(const var& object) override
	{
		PanelWithProcessorConnection::fromDynamicObject(object);

		if (auto fd = getContent<FilterGraph>())
		{
			showLines = (bool)getPropertyWithDefault(object, (int)SpecialPanelIds::ShowLines);
			gainRange = (double)getPropertyWithDefault(object, (int)SpecialPanelIds::GainRange);

			fd->setGainRange(gainRange);
			fd->setShowLines(showLines);
		}
	}

	var toDynamicObject() const override
	{
		auto obj = PanelWithProcessorConnection::toDynamicObject();

		if (auto fd = getContent<FilterGraph>())
		{
			storePropertyInObject(obj, (int)SpecialPanelIds::GainRange, gainRange);
			storePropertyInObject(obj, (int)SpecialPanelIds::ShowLines, showLines);
		}

		return obj;
	}

	juce::Identifier getProcessorTypeId() const
	{
		return PolyFilterEffect::getClassType();
	}

	void timerCallback() override
	{
		if (auto filter = dynamic_cast<FilterEffect*>(getProcessor()))
		{
			if (auto filterGraph = getContent<FilterGraph>())
			{
				filterGraph->setBypassed(getProcessor()->isBypassed());

				auto c = filter->getCurrentCoefficients();

				if (!sameCoefficients(c.first, currentCoefficients.first) || c.second != currentCoefficients.second)
				{
					currentCoefficients = c;

					filterGraph->setCoefficients(0, getProcessor()->getSampleRate(), dynamic_cast<FilterEffect*>(getProcessor())->getCurrentCoefficients());
				}
			}
		}
	}

	struct Updater: public Processor::OtherListener
	{
		Updater(FilterGraph::Panel& parent_, Processor* p):
		  OtherListener(p, dispatch::library::ProcessorChangeEvent::Custom),
		  parent(parent_)
		{};

		void otherChange(Processor* p) override
		{
			parent.updateCoefficients();
		}

		FilterGraph::Panel& parent;
	};

	ScopedPointer<Updater> updater;

	Component* createContentComponent(int index) override
	{
		if (auto p = getProcessor())
		{
			updater = nullptr;

			
			auto c = new FilterGraph(1);

			c->useFlatDesign = true;
			c->showLines = false;

			c->setColour(ColourIds::bgColour, findPanelColour(FloatingTileContent::PanelColourId::bgColour));
			c->setColour(ColourIds::lineColour, findPanelColour(FloatingTileContent::PanelColourId::itemColour1));
			c->setColour(ColourIds::fillColour, findPanelColour(FloatingTileContent::PanelColourId::itemColour2));
			c->setColour(ColourIds::gridColour, findPanelColour(FloatingTileContent::PanelColourId::itemColour3));
			c->setColour(ColourIds::textColour, findPanelColour(FloatingTileContent::PanelColourId::textColour));

			c->setOpaque(c->findColour(bgColour).isOpaque());

			if (dynamic_cast<FilterEffect*>(p) != nullptr)
			{
				c->addFilter(FilterType::LowPass);
				startTimer(30);
			}
			else if (auto eq = dynamic_cast<CurveEq*>(p))
			{
				stopTimer();
				updateEq(eq, c);
			}
			else if (auto d = dynamic_cast<ExternalDataHolder*>(p))
			{
				if (auto f = d->getFilterData(index))
				{
					c->setComplexDataUIBase(f);
				}
			}

			updater = new Updater(*this, p);

			return c;
		}

		return nullptr;
	}



	void fillModuleList(StringArray& moduleList)
	{
		fillModuleListWithType<CurveEq>(moduleList);
		fillModuleListWithType<FilterEffect>(moduleList);
	}

private:

	double gainRange = 24.0;
	bool showLines = true;

	bool sameCoefficients(IIRCoefficients c1, IIRCoefficients c2)
	{
		for (int i = 0; i < 5; i++)
		{
			if (c1.coefficients[i] != c2.coefficients[i]) return false;
		}

		return true;
	};

	void updateCoefficients()
	{
		if (auto filterGraph = getContent<FilterGraph>())
		{
			if (auto c = dynamic_cast<CurveEq*>(getProcessor()))
			{
				if (c->getNumFilterBands() != filterGraph->getNumFilterBands())
				{
					updateEq(c, filterGraph);
					return;
				}

				for (int i = 0; i < c->getNumFilterBands(); i++)
				{
					auto ic = c->getCoefficients(i);

					filterGraph->enableBand(i, c->getFilterBand(i)->isEnabled());
					filterGraph->setCoefficients(i, getProcessor()->getSampleRate(), ic);
				}
			}
		}
	}

	void updateEq(CurveEq* eq, FilterGraph* c)
	{
		c->clearBands();

		for (int i = 0; i < eq->getNumFilterBands(); i++)
		{
			auto t = CurveEq::StereoFilter::SubType::getFilterType();
			addFilter(c, i, (int)t);
			c->enableBand(i, eq->getFilterBand(i)->isEnabled());
		}

		int numFilterBands = eq->getNumFilterBands();

		if (numFilterBands == 0)
		{
			c->repaint();
		}
	}

	void addFilter(FilterGraph* filterGraph, int filterIndex, int filterType)
	{
		if (auto c = dynamic_cast<CurveEq*>(getProcessor()))
		{
			switch (filterType)
			{
			case CurveEq::LowPass:		filterGraph->addFilter(FilterType::LowPass); break;
			case CurveEq::HighPass:		filterGraph->addFilter(FilterType::HighPass); break;
			case CurveEq::LowShelf:		filterGraph->addEqBand(BandType::LowShelf); break;
			case CurveEq::HighShelf:	filterGraph->addEqBand(BandType::HighShelf); break;
			case CurveEq::Peak:			filterGraph->addEqBand(BandType::Peak); break;
			}

			filterGraph->setCoefficients(filterIndex, c->getSampleRate(), c->getCoefficients(filterIndex));
		}
	}

	std::pair<IIRCoefficients, int> currentCoefficients;

};

struct FilterDragOverlay::Panel : public PanelWithProcessorConnection
{
	enum class SpecialProperties
	{
		AllowFilterResizing = (int)PanelWithProcessorConnection::SpecialPanelIds::numSpecialPanelIds,
		AllowDynamicSpectrumAnalyser,
		UseUndoManager,
		ResetOnDoubleClick,
		AllowContextMenu,
		GainRange,
		numSpecialProperties
	};

	SET_PANEL_NAME("DraggableFilterPanel");
	
	int getNumDefaultableProperties() const override
	{
		return (int)SpecialProperties::numSpecialProperties;
	}

	Identifier getDefaultablePropertyId(int index) const override
	{
		if (isPositiveAndBelow(index, PanelWithProcessorConnection::SpecialPanelIds::numSpecialPanelIds))
			return PanelWithProcessorConnection::getDefaultablePropertyId(index);

		RETURN_DEFAULT_PROPERTY_ID(index, SpecialProperties::AllowFilterResizing, "AllowFilterResizing");
		RETURN_DEFAULT_PROPERTY_ID(index, SpecialProperties::AllowDynamicSpectrumAnalyser, "AllowDynamicSpectrumAnalyser");
		RETURN_DEFAULT_PROPERTY_ID(index, SpecialProperties::UseUndoManager, "UseUndoManager");
		RETURN_DEFAULT_PROPERTY_ID(index, SpecialProperties::ResetOnDoubleClick, "ResetOnDoubleClick");
		RETURN_DEFAULT_PROPERTY_ID(index, SpecialProperties::AllowContextMenu, "AllowContextMenu");
		RETURN_DEFAULT_PROPERTY_ID(index, SpecialProperties::GainRange, "GainRange");

		jassertfalse;
		return {};
	}

	var getDefaultProperty(int index) const override
	{
		if (isPositiveAndBelow(index, PanelWithProcessorConnection::SpecialPanelIds::numSpecialPanelIds))
			return PanelWithProcessorConnection::getDefaultProperty(index);

		RETURN_DEFAULT_PROPERTY(index, SpecialProperties::AllowFilterResizing, var(true));
		RETURN_DEFAULT_PROPERTY(index, SpecialProperties::AllowDynamicSpectrumAnalyser, var(0));
		RETURN_DEFAULT_PROPERTY(index, SpecialProperties::UseUndoManager, var(false));
		RETURN_DEFAULT_PROPERTY(index, SpecialProperties::ResetOnDoubleClick, var(false));
		RETURN_DEFAULT_PROPERTY(index, SpecialProperties::AllowContextMenu, var(true));
		RETURN_DEFAULT_PROPERTY(index, SpecialProperties::GainRange, var(24.0));

		jassertfalse;

		return {};
	}

	void fromDynamicObject(const var& object) override
	{
		PanelWithProcessorConnection::fromDynamicObject(object);

		if (auto fd = getContent<FilterDragOverlay>())
		{
			auto r = (bool)getPropertyWithDefault(object, (int)SpecialProperties::AllowFilterResizing);
			auto s = (int)getPropertyWithDefault(object, (int)SpecialProperties::AllowDynamicSpectrumAnalyser);

			auto u = (bool)getPropertyWithDefault(object, (int)SpecialProperties::UseUndoManager);
			auto rd = (bool)getPropertyWithDefault(object, (int)SpecialProperties::ResetOnDoubleClick);

			if (u)
				fd->setUndoManager(getMainController()->getControlUndoManager());
			
			auto ctx = (bool)getPropertyWithDefault(object, (int)SpecialProperties::AllowContextMenu);

			fd->setAllowContextMenu(ctx);

			auto gr = getPropertyWithDefault(object, (int)SpecialProperties::GainRange);

			fd->setGainRange(gr);
			fd->setResetOnDoubleClick(rd);
			fd->setAllowFilterResizing(r);
			fd->setSpectrumVisibility((SpectrumVisibility)s);
		}
	}

	var toDynamicObject() const override
	{
		auto obj = PanelWithProcessorConnection::toDynamicObject();

		if (auto fd = getContent<FilterDragOverlay>())
		{
			storePropertyInObject(obj, (int)SpecialProperties::AllowFilterResizing, fd->allowFilterResizing);
			storePropertyInObject(obj, (int)SpecialProperties::AllowDynamicSpectrumAnalyser, (int)fd->fftVisibility);

			storePropertyInObject(obj, (int)SpecialProperties::UseUndoManager, fd->um != nullptr);
			storePropertyInObject(obj, (int)SpecialProperties::ResetOnDoubleClick, fd->shouldResetOnDoubleClick());

			storePropertyInObject(obj, (int)SpecialProperties::GainRange, fd->gainRange);
			storePropertyInObject(obj, (int)SpecialProperties::AllowContextMenu, fd->allowContextMenu);
		}

		return obj;
	}

	Panel(FloatingTile* parent) :
		PanelWithProcessorConnection(parent)
	{
		setDefaultPanelColour(PanelColourId::bgColour, Colours::transparentBlack);
		setDefaultPanelColour(PanelColourId::textColour, Colours::white);
		setDefaultPanelColour(PanelColourId::itemColour1, Colours::white.withAlpha(0.5f));
		setDefaultPanelColour(PanelColourId::itemColour2, Colours::white.withAlpha(0.8f));
		setDefaultPanelColour(PanelColourId::itemColour2, Colours::black.withAlpha(0.2f));
	}

	juce::Identifier getProcessorTypeId() const
	{
		return CurveEq::getClassType();
	}

	Component* createContentComponent(int) override
	{
		if (auto p = getProcessor())
		{
			auto c = new FilterDragOverlay(dynamic_cast<CurveEq*>(p), true);

			c->setColour(ColourIds::bgColour, findPanelColour(PanelColourId::bgColour));
			c->setColour(ColourIds::textColour, findPanelColour(PanelColourId::textColour));
			c->filterGraph.setColour(FilterGraph::ColourIds::fillColour, findPanelColour(PanelColourId::itemColour1));
			c->filterGraph.setColour(FilterGraph::ColourIds::lineColour, findPanelColour(PanelColourId::itemColour2));
			c->fftAnalyser.setColour(RingBufferComponentBase::ColourId::fillColour, findPanelColour(PanelColourId::itemColour3));

			c->setOpaque(c->findColour(ColourIds::bgColour).isOpaque());

			c->font = getFont();

			return c;
		}

		return nullptr;
	}

	void fillModuleList(StringArray& moduleList)
	{
		fillModuleListWithType<CurveEq>(moduleList);
	}

};

FilterDragOverlay::FilterDragOverlay(CurveEq* eq_, bool isInFloatingTile_ /*= false*/) :
	OtherListener(eq_, dispatch::library::ProcessorChangeEvent::Custom),
	eq(eq_),
	fftAnalyser(*this),
	filterGraph(eq_->getNumFilterBands()),
	isInFloatingTile(isInFloatingTile_)
{
	plaf = new PopupLookAndFeel();

	setLookAndFeel(plaf);

	if (!isInFloatingTile)
		setColour(ColourIds::textColour, Colours::white);

	font = GLOBAL_BOLD_FONT().withHeight(11.0f);

	constrainer = new ComponentBoundsConstrainer();

	addAndMakeVisible(fftAnalyser);
	addAndMakeVisible(filterGraph);

	filterGraph.setUseFlatDesign(true);
	filterGraph.setOpaque(false);
	filterGraph.setColour(FilterGraph::ColourIds::bgColour, Colours::transparentBlack);

	//fftAnalyser.setColour(AudioAnalyserComponent::ColourId::fillColour, Colours::black.withAlpha(0.15f));
	fftAnalyser.setInterceptsMouseClicks(false, false);
	filterGraph.setInterceptsMouseClicks(false, false);

	updateFilters();
	updateCoefficients();
	updatePositions(true);

	startTimer(50);
}

FilterDragOverlay::~FilterDragOverlay()
{
}

void FilterDragOverlay::paint(Graphics &g)
{
	if (isInFloatingTile)
	{
		g.fillAll(findColour(bgColour));
	}
	else
	{
		GlobalHiseLookAndFeel::drawHiBackground(g, 0, 0, getWidth(), getHeight());
	}

	
}

void FilterDragOverlay::paintOverChildren(Graphics& g)
{
	auto lafToUse = &defaultLaf;

	if (auto l = dynamic_cast<LookAndFeelMethods*>(&getLookAndFeel()))
		lafToUse = l;

	auto draw = [&](FilterDragComponent* c)
	{
		auto index = c->getIndex();

		if (eq == nullptr)
			return;

		if (auto b = eq->getFilterBand(index))
		{
			DragData d;
			d.selected = c->isSelected();
			d.dragging = c->isDragging();
			d.enabled = b->isEnabled();
			d.hover = c->isOver();
			d.frequency = b->getFrequency();
			d.gain = Decibels::gainToDecibels(b->getGain());
			d.q = b->getQ();
			d.type = b->getModes()[b->getType()];

			lafToUse->drawFilterDragHandle(g, *this, c->getIndex(), c->getBoundsInParent().toFloat(), d);
		}
	};

	FilterDragComponent* selected = nullptr;

	for (auto c : dragComponents)
	{
		auto shouldBeLast = c->isSelected() || c->isOver() || c->isDragging();

		if (shouldBeLast && selected == nullptr)
		{
			// Move it to the end so that
			// it might overdraw other handles
			selected = c; 
			continue;
		}
			
		draw(c);
	}

	if (selected != nullptr)
		draw(selected);
}

void FilterDragOverlay::checkEnabledBands()
{
	if (eq == nullptr)
		return;

	numFilters = eq->getNumFilterBands();

	hise::SimpleReadWriteLock::ScopedReadLock sl(eq->bandLock);

	for (int i = 0; i < numFilters; i++)
		filterGraph.enableBand(i, eq->getFilterBand(i)->isEnabled());
}

void FilterDragOverlay::resized()
{
	constrainer->setMinimumOnscreenAmounts(24, 24, 24, 24);

	fftAnalyser.setBounds(getLocalBounds().reduced(offset));
	filterGraph.setBounds(getLocalBounds().reduced(offset));

	updatePositions(true);
}

void FilterDragOverlay::addFilterDragger(int index)
{
	if (eq == nullptr)
		return;

	if (auto fb = eq->getFilterBand(index))
	{
		FilterDragComponent *dc = new FilterDragComponent(*this, index);

		addAndMakeVisible(dc);

		dc->setConstrainer(constrainer);

		dragComponents.add(dc);

		selectDragger(dragComponents.size() - 1, dontSendNotification);
	}

	updatePositions(true);
}

void FilterDragOverlay::timerCallback()
{
	updateCoefficients();
	fftAnalyser.repaint();
}

void FilterDragOverlay::updateFilters()
{
	if (eq == nullptr)
		return;

	numFilters = eq->getNumFilterBands();

	if (numFilters != dragComponents.size())
	{
		filterGraph.clearBands();
		dragComponents.clear();

		for (int i = 0; i < numFilters; i++)
		{
			auto t = CurveEq::StereoFilter::SubType::getFilterType();
			addFilterToGraph(i, (int)t);
			addFilterDragger(i);
		}
	}

	if (numFilters == 0)
	{
		filterGraph.repaint();
	}
}

void FilterDragOverlay::updateCoefficients()
{
	if (eq == nullptr)
		return;

	for (int i = 0; i < eq->getNumFilterBands(); i++)
	{
		auto ic = eq->getCoefficients(i);
		filterGraph.setCoefficients(i, eq->getSampleRate(), ic);
	}
}

void FilterDragOverlay::addFilterToGraph(int filterIndex, int filterType)
{
	switch (filterType)
	{
	case CurveEq::LowPass:		filterGraph.addFilter(FilterType::LowPass); break;
	case CurveEq::HighPass:		filterGraph.addFilter(FilterType::HighPass); break;
	case CurveEq::LowShelf:		filterGraph.addEqBand(BandType::LowShelf); break;
	case CurveEq::HighShelf:	filterGraph.addEqBand(BandType::HighShelf); break;
	case CurveEq::Peak:			filterGraph.addEqBand(BandType::Peak); break;
	}

	if (eq == nullptr)
		return;

	if(auto b = eq->getFilterBand(filterIndex))
		filterGraph.enableBand(filterIndex, b->isEnabled());

	filterGraph.setCoefficients(filterIndex, eq->getSampleRate(), eq->getCoefficients(filterIndex));
}

void FilterDragOverlay::updatePositions(bool forceUpdate)
{
	if (!forceUpdate && selectedIndex != -1)
		return;

	for (int i = 0; i < dragComponents.size(); i++)
	{
		Point<int> point = getPosition(i);

		Rectangle<int> b(point, point);

		dragComponents[i]->setBounds(b.withSizeKeepingCentre(24, 24));
	}
}

double FilterDragOverlay::getGain(int y)
{
	return (double)filterGraph.yToGain((float)y, gainRange);
}

void FilterDragOverlay::removeFilter(int index)
{
	if (eq == nullptr)
		return;

	eq->removeFilterBand(index);

	for (auto l : listeners)
	{
		if (l != nullptr)
			l->bandRemoved(index);
	}
}


void FilterDragOverlay::addListener(Listener* l)
{
	listeners.addIfNotAlreadyThere(l);
}

void FilterDragOverlay::removeListener(Listener* l)
{
	listeners.removeAllInstancesOf(l);
}

void FilterDragOverlay::setAllowFilterResizing(bool shouldBeAllowed)
{
	allowFilterResizing = shouldBeAllowed;
}

void FilterDragOverlay::setSpectrumVisibility(SpectrumVisibility m)
{
	fftVisibility = m;
	fftAnalyser.setVisible(m != SpectrumVisibility::AlwaysOff);
}

void FilterDragOverlay::setUndoManager(UndoManager* newUndoManager)
{
	um = newUndoManager;
}

void FilterDragOverlay::mouseDrag(const MouseEvent &e)
{
	CHECK_MIDDLE_MOUSE_DRAG(e);

	if (dragComponents[selectedIndex] != nullptr)
	{
		dragComponents[selectedIndex]->mouseDrag(e);
	}
}

Point<int> FilterDragOverlay::getPosition(int index)
{
	if (eq == nullptr)
		return {};

	if (isPositiveAndBelow(index, eq->getNumFilterBands()))
	{
		const int freqIndex = eq->getParameterIndex(index, CurveEq::BandParameter::Freq);
		const int gainIndex = eq->getParameterIndex(index, CurveEq::BandParameter::Gain);

		const int x = (int)filterGraph.freqToX(eq->getAttribute(freqIndex));
		const int y = (int)filterGraph.gainToY(eq->getAttribute(gainIndex), gainRange);

		return Point<int>(x, y).translated(offset, offset);
	}
	else
		return {};
}

void FilterDragOverlay::lookAndFeelChanged()
{
	if (auto l = dynamic_cast<FilterGraph::LookAndFeelMethods*>(&getLookAndFeel()))
	{
		filterGraph.setSpecialLookAndFeel(&getLookAndFeel());
	}

	if (auto l = dynamic_cast<RingBufferComponentBase::LookAndFeelMethods*>(&getLookAndFeel()))
	{
		fftAnalyser.setSpecialLookAndFeel(&getLookAndFeel());
	}
}

void FilterDragOverlay::fillPopupMenu(PopupMenu& m, int handleIndex)
{
	if (eq == nullptr)
		return;

	if (handleIndex != -1)
	{
		StringArray sa = { "Low Pass", "High Pass", "Low Shelf", "High Shelf", "Peak" };
		Factory pf;

		if (auto thisBand = eq->getFilterBand(handleIndex))
		{
			int typeOffset = 8000;

			if(allowFilterResizing)
				m.addItem(9000, "Delete Band", true, false);

			m.addItem(10000, "Enable Band", true, thisBand->isEnabled());
			m.addSeparator();

			m.addSectionHeader("Select Type");

			for (int i = 0; i < sa.size(); i++)
			{
				bool isSelected = thisBand->getType() == i;

				auto p = pf.createPath(sa[i]);

				auto dp = new DrawablePath();
				dp->setPath(p);

				m.addItem(typeOffset + i, sa[i], true, isSelected, std::unique_ptr<Drawable>(dp));
			}

			m.addSeparator();
			m.addItem(3, "Cancel");
		}
	}
	else
	{
		if(allowFilterResizing)
			m.addItem(1, "Delete all bands", true, false);


		if(fftVisibility == SpectrumVisibility::Dynamic)
			m.addItem(2, "Enable Spectrum Analyser", true, eq->getFFTBuffer()->isActive());

		m.addItem(3, "Cancel");
	}
}



void FilterDragOverlay::popupMenuAction(int result, int handleIndex)
{
	if (eq == nullptr)
		return;

	if (handleIndex != -1)
	{
		if (auto thisBand = eq->getFilterBand(handleIndex))
		{
			int typeOffset = 8000;

			if (result == 0 || result == 3)
				return;
			else if (result == 9000)
			{
				if (um != nullptr)
					um->perform(new FilterResizeAction(eq, handleIndex, false));
				else
					eq->removeFilterBand(handleIndex);
			}
			else if (result == 10000)
			{
				setEqAttribute(CurveEq::BandParameter::Enabled, handleIndex, 1.0f - (float)thisBand->isEnabled());
			}
			else
			{
				setEqAttribute(CurveEq::BandParameter::Type, handleIndex, (float)(result - typeOffset));
			}
		}
	}
	else
	{
		if (result == 3)
			return;

		if (result == 1)
		{
			while (eq->getNumFilterBands() > 0)
			{
				if (um != nullptr)
					um->perform(new FilterResizeAction(eq, 0, false));
				else
					eq->removeFilterBand(0);
			}
		}
		else if (result == 2)
			eq->enableSpectrumAnalyser(!eq->getFFTBuffer()->isActive());
	}
}

void FilterDragOverlay::mouseDown(const MouseEvent &e)
{
	CHECK_MIDDLE_MOUSE_DOWN(e);

	if (eq == nullptr)
		return;

	if (e.mods.isRightButtonDown() || e.mods.isCommandDown())
	{
		if (allowContextMenu)
		{
			PopupMenu m;
			m.setLookAndFeel(&getLookAndFeel());

			fillPopupMenu(m, -1);

			auto result = PopupLookAndFeel::showAtComponent(m, this, false);

			popupMenuAction(result, -1);
		}
	}
	else if (allowFilterResizing)
	{
		const double freq = (double)filterGraph.xToFreq((float)e.getPosition().x - offset);
		const double gain = Decibels::decibelsToGain((double)getGain(e.getPosition().y - offset));

		if (um != nullptr)
			um->perform(new FilterResizeAction(eq, -1, true, freq, gain));
		else
			eq->addFilterBand(freq, gain);
	}
	else
	{
		Array<int> distance;

		int minDist = INT_MAX;

		for(int i = 0; i < dragComponents.size(); i++)
		{
			distance.add(hmath::abs(dragComponents[i]->getX() - e.getMouseDownX()));
			minDist = jmin(minDist, distance.getLast());
		}

		for(int i = 0; i < dragComponents.size(); i++)
		{
			if(distance[i] == minDist)
			{
				selectDragger(i);
				dragComponents[i]->mouseDown(e);
			}
		}
	}
}

void FilterDragOverlay::mouseUp(const MouseEvent& e)
{
	CHECK_MIDDLE_MOUSE_UP(e);

	selectDragger(-1);
}

void FilterDragOverlay::mouseMove(const MouseEvent &e)
{
	setTooltip(String(getGain(e.getPosition().y - offset), 1) + " dB / " + String((int)filterGraph.xToFreq((float)e.getPosition().x - offset)) + " Hz");
};

void FilterDragOverlay::selectDragger(int index, NotificationType n)
{
	selectedIndex = index;

	for (int i = 0; i < dragComponents.size(); i++)
	{
		dragComponents[i]->setSelected(index == i);
	}

	if (selectedIndex != -1)
	{
		for (auto l : listeners)
		{
			if (l != nullptr)
				l->filterBandSelected(index);
		}

		eq->sendBroadcasterMessage("BandSelected", index, n);
	}
}


void FilterDragOverlay::FilterDragComponent::mouseDown(const MouseEvent& e)
{
	CHECK_MIDDLE_MOUSE_DOWN(e);

	auto pi = parent.eq->getParameterIndex(index, CurveEq::BandParameter::Q);
	dragQStart = parent.eq->getAttribute(pi);

	if (e.mods.isRightButtonDown() || e.mods.isCommandDown())
	{
		if (parent.allowContextMenu)
		{
			over = false;
			down = false;
			menuActive = true;
			parent.repaint();

			PopupMenu m;
			m.setLookAndFeel(&parent.getLookAndFeel());

			parent.fillPopupMenu(m, index);

			auto result = PopupLookAndFeel::showAtComponent(m, this, false);

			if (result != 0)
				parent.popupMenuAction(result, index);

			menuActive = false;
			over = isMouseOver();
		}
		else
		{
			auto ei = parent.eq->getParameterIndex(index, CurveEq::BandParameter::Enabled);
			auto v = parent.eq->getAttribute(ei);
			parent.setEqAttribute(CurveEq::BandParameter::Enabled, index, 1 - v);
			parent.repaint();
            parent.checkEnabledBands();
		}
	}
	else
	{
		down = true;
		parent.selectDragger(index);
		dragger.startDraggingComponent(this, e);
        
        parent.setEqAttribute(CurveEq::BandParameter::Enabled, index, 1.0f);
        parent.repaint();
        parent.checkEnabledBands();
	}
}

void FilterDragOverlay::FilterDragComponent::mouseUp(const MouseEvent& e)
{
	CHECK_MIDDLE_MOUSE_UP(e);

	down = false;
	draggin = false;
	parent.selectDragger(-1);
}


void FilterDragOverlay::FilterDragComponent::mouseDrag(const MouseEvent& e)
{
	CHECK_MIDDLE_MOUSE_DRAG(e);

	if (e.mods.isShiftDown())
	{
		auto deltaNormalised = (float)e.getDistanceFromDragStartY() / (float)getParentComponent()->getHeight();

		if (parent.eq->getAttribute(parent.eq->getParameterIndex(index, CurveEq::BandParameter::Gain)) < 0.0f)
			deltaNormalised *= -1.0;

		auto qRange = NormalisableRange<double>(0.3, 9.0);
		qRange.setSkewForCentre(1.0);

		auto start = qRange.convertTo0to1(dragQStart);
		auto newValue = jlimit(0.0, 1.0, start + deltaNormalised);

		parent.setEqAttribute(CurveEq::BandParameter::Q, index, qRange.convertFrom0to1(newValue));

		return;
	}
	else
	{
		auto pi = parent.eq->getParameterIndex(index, CurveEq::BandParameter::Q);
		dragQStart = parent.eq->getAttribute(pi);
	}

	auto te = e.getEventRelativeTo(this);

	


	auto constrainerToUse = constrainer;

	

	over = true;
	down = true;

	if (!draggin)
	{
		if (!parent.allowFilterResizing)
		{
			
			parent.setEqAttribute(CurveEq::BandParameter::Enabled, index, 1.0f);
		}

		dragger.startDraggingComponent(this, te);
		draggin = true;
	}

	dragger.dragComponent(this, te, constrainerToUse);

	auto x = getBoundsInParent().getCentreX() - parent.offset;
	auto y = getBoundsInParent().getCentreY() - parent.offset;

	const double freq = jlimit<double>(20.0, 20000.0, (double)parent.filterGraph.xToFreq((float)x));

	const double gain = parent.filterGraph.yToGain((float)y, parent.gainRange);

	parent.setEqAttribute(CurveEq::BandParameter::Freq, index, freq);
	parent.setEqAttribute(CurveEq::BandParameter::Gain, index, gain);
}




void FilterDragOverlay::FilterDragComponent::mouseWheelMove(const MouseEvent &e, const MouseWheelDetails &d)
{
	if (parent.eq == nullptr)
		return;

	if (e.mods.isCtrlDown() || parent.isInFloatingTile)
	{
		double q = parent.eq->getFilterBand(index)->getQ();
        
        double amp = d.deltaY * 4.0;
        
        if(parent.eq->getFilterBand(index)->getGain() > 1.0f)
            amp *= -1.0;
        
        amp += 1.0;
            
        amp = jlimit(0.7, 1.3, amp);
        
        q = jlimit(0.1, 8.0, q * amp);

		parent.setEqAttribute(CurveEq::BandParameter::Q, index, q);
	}
	else
	{
		getParentComponent()->mouseWheelMove(e, d);
	}
}



void FilterDragOverlay::FilterDragComponent::mouseDoubleClick(const MouseEvent& e)
{
	if (parent.eq == nullptr)
		return;

	if (parent.resetOnDoubleClick)
	{
		parent.setEqAttribute(CurveEq::BandParameter::Gain, index, 0.0f);
		parent.setEqAttribute(CurveEq::BandParameter::Enabled, index, 0.0f);
        parent.checkEnabledBands();
	}
	else
	{
		if (parent.allowFilterResizing)
			return;

		auto enableIndex = parent.eq->getParameterIndex(index, CurveEq::BandParameter::Enabled);
		auto v = 1.0f - parent.eq->getAttribute(enableIndex);
		parent.setEqAttribute(CurveEq::BandParameter::Enabled, index, v);
        parent.checkEnabledBands();
	}

	
}

FilterDragOverlay::FilterDragComponent::FilterDragComponent(FilterDragOverlay& parent_, int index_) :
	parent(parent_),
	index(index_)
{

}

void FilterDragOverlay::FilterDragComponent::setConstrainer(ComponentBoundsConstrainer *constrainer_)
{
	constrainer = constrainer_;
}

void FilterDragOverlay::FilterDragComponent::mouseEnter(const MouseEvent& e)
{
	over = true;
	parent.repaint();
}

void FilterDragOverlay::FilterDragComponent::mouseExit(const MouseEvent& e)
{
	over = false;
	parent.repaint();
}

void FilterDragOverlay::FilterDragComponent::setSelected(bool shouldBeSelected)
{
	selected = shouldBeSelected;

	over = Component::isMouseOverOrDragging(true);
	down = Component::isMouseButtonDown(true);

	repaint();
}

void FilterDragOverlay::FilterDragComponent::paint(Graphics &g)
{
	
}

void FilterDragOverlay::FilterDragComponent::setIndex(int newIndex)
{
	index = newIndex;
}

void FilterDragOverlay::setEqAttribute(int bp, int filterIndex, float value)
{
	if (eq == nullptr)
		return;

	auto idx = eq->getParameterIndex(filterIndex, bp);

	if (um != nullptr)
	{
		auto oldValue = eq->getAttribute(idx);
		um->perform(new MacroControlledObject::UndoableControlEvent(eq, idx, oldValue, value));
	}
	else
		eq->setAttribute(idx, value, sendNotification);
}

FilterDragOverlay::FFTDisplay::FFTDisplay(FilterDragOverlay& parent_) :
	FFTDisplayBase(),
	parent(parent_)
{
	setComplexDataUIBase(parent_.eq->getFFTBuffer().get());

	setColour(RingBufferComponentBase::ColourId::fillColour, Colours::white.withAlpha(0.6f));
	fftProperties.freq2x = std::bind(&FilterGraph::freqToX, &parent.filterGraph, std::placeholders::_1);
}

void FilterDragOverlay::FFTDisplay::paint(Graphics& g)
{
	if (parent.eq == nullptr)
		return;

	if (rb != nullptr && rb->isActive() && !parent.eq->isBypassed())
		FFTDisplayBase::drawSpectrum(g);
}

double FilterDragOverlay::FFTDisplay::getSamplerate() const
{
	if (parent.eq == nullptr)
		return 44100.0;

	return parent.eq->getSampleRate();
}

juce::Colour FilterDragOverlay::FFTDisplay::getColourForAnalyserBase(int colourId)
{
	if (colourId == RingBufferComponentBase::bgColour)
		return Colours::transparentBlack;
	if (colourId == RingBufferComponentBase::ColourId::fillColour)
		return findColour(colourId);
	if (colourId == RingBufferComponentBase::ColourId::lineColour)
		return Colours::white.withAlpha(0.2f);

	return Colours::blue;
}

juce::Path FilterDragOverlay::Factory::createPath(const String& url) const
{
	StringArray sa("low-pass", "high-pass", "low-shelf", "high-shelf", "peak");

	auto name = MarkdownLink::Helpers::getSanitizedFilename(url);

	auto index = sa.indexOf(name);

	Path path;

	if (index != -1)
	{
		switch (index)
		{
		case CurveEq::LowPass:
		{
			static const unsigned char pathData[] = { 110,109,0,0,210,66,0,48,204,67,98,0,0,210,66,85,133,205,67,0,0,210,66,171,218,206,67,0,0,210,66,0,48,208,67,98,184,49,227,66,2,50,208,67,22,100,244,66,241,43,208,67,179,202,2,67,22,51,208,67,98,161,92,7,67,235,74,208,67,249,38,11,67,117,227,209,67,133,
			142,13,67,23,186,211,67,98,235,153,15,67,165,60,213,67,185,29,17,67,240,235,214,67,0,32,18,67,1,172,216,67,98,85,181,20,67,86,89,216,67,171,74,23,67,172,6,216,67,0,224,25,67,1,180,215,67,98,73,187,23,67,129,232,211,67,21,186,19,67,66,42,208,67,62,20,
			13,67,100,229,205,67,98,35,105,9,67,120,158,204,67,58,229,4,67,231,20,204,67,152,118,0,67,1,48,204,67,98,202,72,241,66,1,48,204,67,102,164,225,66,1,48,204,67,0,0,210,66,1,48,204,67,99,101,0,0 };

			path.loadPathFromData(pathData, sizeof(pathData));
			break;
		}
		case CurveEq::HighPass:
		{
			static const unsigned char pathData[] = { 110,109,0,0,112,66,0,48,204,67,98,142,227,74,66,0,48,204,67,69,230,49,66,242,81,207,67,0,224,35,66,0,32,210,67,98,187,217,21,66,14,238,212,67,0,128,16,66,0,180,215,67,0,128,16,66,0,180,215,67,108,0,128,47,66,0,172,216,67,98,0,128,47,66,0,172,216,67,69,
			38,52,66,242,109,214,67,0,32,63,66,0,60,212,67,98,187,25,74,66,14,10,210,67,114,28,89,66,0,48,208,67,0,0,112,66,0,48,208,67,108,0,0,170,66,0,48,208,67,108,0,0,170,66,0,48,204,67,108,0,0,112,66,0,48,204,67,99,101,0,0 };

			path.loadPathFromData(pathData, sizeof(pathData));
			break;
		}
		case CurveEq::HighShelf:
		{
			static const unsigned char pathData[] = { 110,109,0,0,92,67,0,48,199,67,98,44,153,88,67,118,34,199,67,167,11,86,67,12,117,200,67,149,34,84,67,225,179,201,67,98,167,166,81,67,71,62,203,67,118,235,79,67,158,20,205,67,172,31,77,67,32,126,206,67,98,91,90,76,67,19,199,206,67,72,65,77,67,149,170,206,
			67,147,30,76,67,0,176,206,67,98,184,105,71,67,0,176,206,67,220,180,66,67,0,176,206,67,1,0,62,67,0,176,206,67,98,1,0,62,67,85,5,208,67,1,0,62,67,171,90,209,67,1,0,62,67,0,176,210,67,98,176,50,67,67,37,174,210,67,184,101,72,67,192,179,210,67,46,152,77,
			67,37,173,210,67,98,21,99,81,67,70,140,210,67,121,214,83,67,128,221,208,67,40,232,85,67,171,120,207,67,98,186,233,87,67,30,30,206,67,99,120,89,67,142,147,204,67,86,224,91,67,225,97,203,67,98,167,165,92,67,238,24,203,67,186,190,91,67,108,53,203,67,111,
			225,92,67,1,48,203,67,98,245,64,99,67,1,48,203,67,123,160,105,67,1,48,203,67,1,0,112,67,1,48,203,67,98,1,0,112,67,172,218,201,67,1,0,112,67,86,133,200,67,1,0,112,67,1,48,199,67,98,86,85,105,67,1,48,199,67,172,170,98,67,1,48,199,67,1,0,92,67,1,48,199,
			67,99,101,0,0 };

			path.loadPathFromData(pathData, sizeof(pathData));
			break;

		}
		case CurveEq::LowShelf:
		{
			static const unsigned char pathData[] = { 110,109,0,0,117,67,92,174,198,67,98,0,0,117,67,177,3,200,67,0,0,117,67,7,89,201,67,0,0,117,67,92,174,202,67,98,171,170,123,67,92,174,202,67,171,42,129,67,92,174,202,67,0,128,132,67,92,174,202,67,98,208,39,132,67,172,192,202,67,179,75,133,67,224,102,203,
			67,42,91,133,67,40,199,203,67,98,2,219,134,67,173,188,205,67,26,242,135,67,189,28,208,67,19,254,137,67,177,149,209,67,98,144,42,139,67,213,105,210,67,132,163,140,67,64,33,210,67,55,251,141,67,91,46,210,67,98,37,210,143,67,91,46,210,67,18,169,145,67,91,
			46,210,67,0,128,147,67,91,46,210,67,98,0,128,147,67,6,217,208,67,0,128,147,67,176,131,207,67,0,128,147,67,91,46,206,67,98,0,0,145,67,91,46,206,67,0,128,142,67,91,46,206,67,0,0,140,67,91,46,206,67,98,48,88,140,67,11,28,206,67,77,52,139,67,215,117,205,
			67,214,36,139,67,143,21,205,67,98,254,164,137,67,10,32,203,67,230,141,136,67,250,191,200,67,237,129,134,67,6,71,199,67,98,112,85,133,67,226,114,198,67,124,220,131,67,119,187,198,67,201,132,130,67,92,174,198,67,98,12,177,127,67,92,174,198,67,134,88,122,
			67,92,174,198,67,0,0,117,67,92,174,198,67,99,101,0,0 };

			path.loadPathFromData(pathData, sizeof(pathData));
			break;
		}
		case CurveEq::Peak:
		{
			static const unsigned char pathData[] = { 110,109,0,0,22,67,0,176,181,67,98,187,189,18,67,27,175,181,67,216,98,16,67,236,14,183,67,212,53,15,67,102,114,184,67,98,140,250,12,67,223,183,186,67,79,140,11,67,108,99,189,67,63,64,7,67,186,241,190,67,98,99,112,4,67,66,246,191,67,19,226,0,67,231,160,
			191,67,25,74,251,66,255,175,191,67,98,57,63,249,66,91,136,191,67,152,54,250,66,72,33,192,67,255,255,249,66,182,110,192,67,98,255,255,249,66,121,132,193,67,255,255,249,66,60,154,194,67,255,255,249,66,255,175,195,67,98,239,13,1,67,139,170,195,67,251,70,
			5,67,5,221,195,67,2,16,9,67,3,249,194,67,98,8,153,14,67,113,211,193,67,116,255,17,67,255,42,191,67,228,79,20,67,174,135,188,67,98,177,239,20,67,88,14,188,67,96,130,21,67,123,192,186,67,205,34,22,67,78,198,186,67,98,241,145,24,67,34,154,189,67,148,50,
			27,67,164,173,192,67,181,166,32,67,153,95,194,67,98,194,252,35,67,241,118,195,67,214,8,40,67,43,196,195,67,202,242,43,67,0,176,195,67,98,49,247,44,67,0,176,195,67,152,251,45,67,0,176,195,67,255,255,46,67,0,176,195,67,98,255,255,46,67,171,90,194,67,255,
			255,46,67,85,5,193,67,255,255,46,67,0,176,191,67,98,48,69,44,67,193,162,191,67,205,121,41,67,176,214,191,67,122,208,38,67,176,117,191,67,98,1,143,34,67,185,180,190,67,246,109,32,67,50,135,188,67,35,182,30,67,72,152,186,67,98,81,69,29,67,110,22,185,67,
			247,82,28,67,166,79,183,67,225,132,25,67,247,69,182,67,98,198,125,24,67,135,234,181,67,204,66,23,67,208,175,181,67,0,0,22,67,1,176,181,67,99,101,0,0 };

			path.loadPathFromData(pathData, sizeof(pathData));

			break;
		}
		case CurveEq::numFilterTypes: break;
		}
	}

	return path;
}



void FilterDragOverlay::LookAndFeelMethods::drawFilterDragHandle(Graphics& g, FilterDragOverlay& o, int index, Rectangle<float> handleBounds, const DragData& d)
{
	handleBounds.reduce(6.0f, 6.0f);
	auto tc = o.findColour(ColourIds::textColour);
	auto fc = tc.contrasting();
	g.setColour(fc.withAlpha(0.3f));
	g.fillRoundedRectangle(handleBounds, 3.0f);
	g.setColour(tc.withAlpha(d.enabled ? 1.0f : 0.3f));
	g.drawRoundedRectangle(handleBounds, 3.0f, d.selected ? 2.0f : 1.0f);
	g.setFont(o.font);
	g.drawText(String(index), handleBounds.expanded(6.0f), Justification::centred, false);
}

FilterDragOverlay::FilterResizeAction::FilterResizeAction(CurveEq* eq_, int index_, bool add, double freq_/*=0.0*/, double gain_/*=0.0*/) :
	eq(eq_),
	index(index_),
	isAddAction(add),
	freq(freq_),
	gain(gain_)
{

}

bool FilterDragOverlay::FilterResizeAction::perform()
{
	if (!eq)
		return false;

	if (isAddAction)
	{
		index = eq->getNumFilterBands();
		eq.get()->addFilterBand(freq, gain);
		return true;
	}
	else
	{
		if (auto b = eq->getFilterBand(index))
		{
			gain = b->getGain();
			freq = b->getFrequency();
			q = b->getQ();
			type = b->getType();
			enabled = b->isEnabled();
		}

		eq.get()->removeFilterBand(index);
		return true;
	}
}

bool FilterDragOverlay::FilterResizeAction::undo()
{
	if (!eq)
		return false;

	if (isAddAction)
	{
		eq.get()->removeFilterBand(index);
		return true;
	}
	else
	{
		index = eq->getNumFilterBands();
		eq->addFilterBand(freq, gain);

		if (auto b = eq->getFilterBand(index))
		{
			b->setType(type);
			b->setQ(q);
			b->setEnabled(enabled);
		}
			
		return true;
	}
}

} // namespace hise;
