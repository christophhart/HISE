
#pragma once

namespace hise { 
using namespace juce;

class CurveEqEditor;

class FilterTypeSelector: public Component,
						  public MacroControlledObject,
						  public ButtonListener
{
public:

	struct Factory : public PathFactory
	{
		String getId() const override { return "FilterIcons"; }

		Path createPath(const String& url) const override;
	};


	class Listener
	{
	public:

        virtual ~Listener() {};

		virtual void filterSelectorChanged(FilterTypeSelector *selectorThatWasChanged) = 0;
	};

	void addListener(FilterTypeSelector::Listener *newListener)
	{
		listeners.add(newListener);
	};

	void buttonClicked(Button *b)
	{
		const int buttonIndex = buttons.indexOf(dynamic_cast<ShapeButton*>(b));

		selectButton(buttonIndex);

		typeChanged(buttonIndex);
	}

	void selectButton(int buttonIndex)
	{
		for(int i = 0; i < buttons.size(); i++)
		{
			buttons[i]->setColours(Colours::white.withAlpha(0.5f), Colours::white.withAlpha(0.7f), Colours::white);
			buttons[i]->repaint();
		}

		if(buttonIndex != -1)
		{
			buttons[buttonIndex]->setColours(Colours::white, Colours::white, Colours::white);
			buttons[buttonIndex]->repaint();
		}
	}

	void updateValue(NotificationType /*sendAttributeChange*/ = sendNotification) override
	{
		if(parameter == -1)
		{
			selectButton(-1);
		}
		else
		{
			auto p = getProcessor()->getAttribute(parameter);
			selectButton(p);
		}
	}

	NormalisableRange<double> getRange() const override
	{
		return NormalisableRange<double>(0.0, (double)(int)CurveEq::FilterType::numFilterTypes);
	}

	void typeChanged(int newType)
	{
		if(!checkLearnMode())
		{
			currentType = (CurveEq::FilterType)newType;

			for(int i = 0; i < listeners.size(); i++)
			{
				listeners[i]->filterSelectorChanged(this);
			}

			getProcessor()->setAttribute(parameter, (float)(int)currentType, dontSendNotification);
		}
	}

	FilterTypeSelector()
	{
		Path path;

		Factory f;

		addAndMakeVisible(lowPassButton = new ShapeButton("Low Pass", Colours::white.withAlpha(0.5f), Colours::white.withAlpha(0.7f), Colours::white));
		lowPassButton->setShape(f.createPath("Low Pass"), true, true, true);
		lowPassButton->addListener(this);

		addAndMakeVisible(highPassButton = new ShapeButton("High Pass", Colours::white.withAlpha(0.5f), Colours::white.withAlpha(0.7f), Colours::white));
		highPassButton->setShape(f.createPath("High Pass"), true, true, true);
		highPassButton->addListener(this);

		addAndMakeVisible(lowShelfButton = new ShapeButton("Low Shelf", Colours::white.withAlpha(0.5f), Colours::white.withAlpha(0.7f), Colours::white));
		lowShelfButton->setShape(f.createPath("Low Shelf"), true, true, true);
		lowShelfButton->addListener(this);

		addAndMakeVisible(highShelfButton = new ShapeButton("High Shelf", Colours::white.withAlpha(0.5f), Colours::white.withAlpha(0.7f), Colours::white));
		highShelfButton->setShape(f.createPath("High Shelf"), true, true, true);
		highShelfButton->addListener(this);

		addAndMakeVisible(peakButton = new ShapeButton("Peak", Colours::white.withAlpha(0.5f), Colours::white.withAlpha(0.7f), Colours::white));
		peakButton->setShape(f.createPath("peak"), true, true, true);
		peakButton->addListener(this);

		buttons.add(lowPassButton);
		buttons.add(highPassButton);
		buttons.add(lowShelfButton);
		buttons.add(highShelfButton);
		buttons.add(peakButton);

	};



	

    void mouseDown(const MouseEvent &) override
    {
        ProcessorEditor *editor = findParentComponentOfClass<ProcessorEditor>();
        
        if(editor != nullptr)
        {
            PresetHandler::setChanged(editor->getProcessor());
        }
    }


	void resized()
	{
		auto b = getLocalBounds();

		auto s = jmin(b.getHeight(), b.getWidth() / buttons.size());

		for (int i = 0; i < buttons.size(); i++)
			buttons[i]->setBounds(b.removeFromLeft(s).reduced(2));

	}

	void paint(Graphics &g)
	{
		PopupLookAndFeel::drawHiBackground(g, 0, 0, getWidth(), getHeight(), nullptr, false);
	}

	int getCurrentType() const
	{
		return currentType;
	}

private:

	CurveEq::FilterType currentType;

	ShapeButton *lowPassButton;

	ShapeButton *highPassButton;

	ShapeButton *lowShelfButton;

	ShapeButton *highShelfButton;

	ShapeButton *peakButton;

	OwnedArray<ShapeButton> buttons;

	Array<FilterTypeSelector::Listener*> listeners;

};

class FilterDragOverlay: public Component,
						 public SettableTooltipClient,
						 public SafeChangeListener,
					     public Timer
{
public:

	enum ColourIds
	{
		bgColour = 125160,
		textColour
	};

	struct Panel : public PanelWithProcessorConnection
	{
		SET_PANEL_NAME("DraggableFilterPanel");

		Panel(FloatingTile* parent) :
			PanelWithProcessorConnection(parent)
		{
			setDefaultPanelColour(PanelColourId::bgColour, Colours::transparentBlack);
			setDefaultPanelColour(PanelColourId::textColour, Colours::white);
			setDefaultPanelColour(PanelColourId::itemColour1, Colours::white.withAlpha(0.5f));
			setDefaultPanelColour(PanelColourId::itemColour2, Colours::white.withAlpha(0.8f));
			setDefaultPanelColour(PanelColourId::itemColour2, Colours::black.withAlpha(0.2f));
		};

		juce::Identifier getProcessorTypeId() const
		{
			return CurveEq::getClassType();
		}

		Component* createContentComponent(int /*index*/) override
		{
			if (auto p = getProcessor())
			{
				auto c = new FilterDragOverlay(dynamic_cast<CurveEq*>(p), true);

				c->setColour(ColourIds::bgColour, findPanelColour(PanelColourId::bgColour));
				c->setColour(ColourIds::textColour, findPanelColour(PanelColourId::textColour));
				c->filterGraph.setColour(FilterGraph::ColourIds::fillColour, findPanelColour(PanelColourId::itemColour1));
				c->filterGraph.setColour(FilterGraph::ColourIds::lineColour, findPanelColour(PanelColourId::itemColour2));
				c->fftAnalyser.setColour(AudioAnalyserComponent::ColourId::fillColour, findPanelColour(PanelColourId::itemColour3));

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

	int offset = 12;

	struct Listener
	{
		virtual ~Listener() {};

		virtual void bandRemoved(int index) = 0;

		virtual void filterBandSelected(int index) = 0;

		JUCE_DECLARE_WEAK_REFERENCEABLE(Listener);
	};

	FilterDragOverlay(CurveEq* eq_, bool isInFloatingTile_=false):
		eq(eq_),
		fftAnalyser(*this),
		filterGraph(eq->getNumFilterBands()),
		isInFloatingTile(isInFloatingTile_)
	{
		if (!isInFloatingTile)
			setColour(ColourIds::textColour, Colours::white);

		font = GLOBAL_BOLD_FONT().withHeight(11.0f);

		constrainer = new ComponentBoundsConstrainer();

		addAndMakeVisible(fftAnalyser);
		addAndMakeVisible(filterGraph);
		
		filterGraph.setUseFlatDesign(true);
		filterGraph.setOpaque(false);
		filterGraph.setColour(FilterGraph::ColourIds::bgColour, Colours::transparentBlack);

		fftAnalyser.setColour(AudioAnalyserComponent::ColourId::fillColour, Colours::black.withAlpha(0.15f));
		fftAnalyser.setInterceptsMouseClicks(false, false);
		filterGraph.setInterceptsMouseClicks(false, false);

		updateFilters();
		updateCoefficients();
		updatePositions(true);

		startTimer(50);
		eq->addChangeListener(this);
	}

	bool isInFloatingTile = false;

	~FilterDragOverlay()
	{
		eq->removeChangeListener(this);
	}

	void paint(Graphics &g)
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

	void changeListenerCallback(SafeChangeBroadcaster *b) override
	{
		checkEnabledBands();
		updateFilters();
		updatePositions(false);
	}

	void checkEnabledBands()
	{
		auto numFilters = eq->getNumFilterBands();

		for (int i = 0; i < numFilters; i++)
			filterGraph.enableBand(i, eq->getFilterBand(i)->isEnabled());
	}

	void resized() override
	{
		constrainer->setMinimumOnscreenAmounts(24, 24, 24, 24);

		fftAnalyser.setBounds(getLocalBounds().reduced(offset));
		filterGraph.setBounds(getLocalBounds().reduced(offset));

		updatePositions(true);
	}

	void mouseMove(const MouseEvent &e);

	void mouseDown(const MouseEvent &e);

	void mouseUp(const MouseEvent& e);
	
	void addFilterDragger(int index)
	{
		if (auto fb = eq->getFilterBand(index))
		{
			FilterDragComponent *dc = new FilterDragComponent(*this, index);

			addAndMakeVisible(dc);

			dc->setConstrainer(constrainer);

			dragComponents.add(dc);

			selectDragger(dragComponents.size() - 1);
		}

		updatePositions(true);
	};

	void timerCallback() override
	{
		updateCoefficients();
		fftAnalyser.repaint();
	}

	void updateFilters()
	{
		numFilters = eq->getNumFilterBands();

		if (numFilters != dragComponents.size())
		{
			filterGraph.clearBands();
			dragComponents.clear();

			for (int i = 0; i < numFilters; i++)
			{
				addFilterToGraph(i, (int)eq->getFilterBand(i)->getFilterType());
				addFilterDragger(i);
			}
		}

		if (numFilters == 0)
		{
			filterGraph.repaint();
		}
	}

	void updateCoefficients()
	{
		for (int i = 0; i < eq->getNumFilterBands(); i++)
		{
			IIRCoefficients ic = eq->getCoefficients(i);
			filterGraph.setCoefficients(i, eq->getSampleRate(), ic);
		}
	}

	void addFilterToGraph(int filterIndex, int filterType)
	{
		switch (filterType)
		{
		case CurveEq::LowPass:		filterGraph.addFilter(FilterType::LowPass); break;
		case CurveEq::HighPass:		filterGraph.addFilter(FilterType::HighPass); break;
		case CurveEq::LowShelf:		filterGraph.addEqBand(BandType::LowShelf); break;
		case CurveEq::HighShelf:	filterGraph.addEqBand(BandType::HighShelf); break;
		case CurveEq::Peak:			filterGraph.addEqBand(BandType::Peak); break;
		}

		filterGraph.setCoefficients(filterIndex, eq->getSampleRate(), eq->getCoefficients(filterIndex));
	}

	void updatePositions(bool forceUpdate)
	{
		if (!forceUpdate && selectedIndex != -1)
			return;

		for(int i = 0; i < dragComponents.size(); i++)
		{
			Point<int> point = getPosition(i);

			Rectangle<int> b(point, point);

			dragComponents[i]->setBounds(b.withSizeKeepingCentre(24, 24));
		}
	}

	void mouseDrag(const MouseEvent &e) override
	{
		if( dragComponents[selectedIndex] != nullptr)
		{
			dragComponents[selectedIndex]->mouseDrag(e);
		}
	}

	Font font;

	Point<int> getPosition(int index);

	class FilterDragComponent: public Component
	{
	public:

		FilterDragComponent(FilterDragOverlay& parent_, int index_):
			parent(parent_),
			index(index_)
		{};

		void setConstrainer(ComponentBoundsConstrainer *constrainer_)
		{
			constrainer = constrainer_;
		};

		void mouseDown (const MouseEvent& e);

		void mouseUp(const MouseEvent& e);

		void mouseDrag (const MouseEvent& e);

		void mouseWheelMove(const MouseEvent &e, const MouseWheelDetails &d) override;

		void setSelected(bool shouldBeSelected)
		{
			selected = shouldBeSelected;
			repaint();
		}

		void paint(Graphics &g)
		{
			Rectangle<float> thisBounds = Rectangle<float>(0.0f, 0.0f, (float)getWidth(), (float)getHeight());

			thisBounds.reduce(6.0f, 6.0f);

			auto tc = parent.findColour(ColourIds::textColour);

			auto fc = tc.contrasting();

			g.setColour(fc.withAlpha(0.3f));
			g.fillRoundedRectangle(thisBounds, 3.0f);

			const bool enabled = parent.eq->getFilterBand(index)->isEnabled();

			g.setColour(tc.withAlpha(enabled ? 1.0f : 0.3f));

			g.drawRoundedRectangle(thisBounds, 3.0f, selected ? 2.0f : 1.0f);

			g.setFont(parent.font);
			g.drawText(String(index), getLocalBounds(), Justification::centred, false);
		};
		
		void setIndex(int newIndex)
		{
			index = newIndex;
		};

	private:

		bool draggin = false;

		int index;

		bool selected;

		ComponentBoundsConstrainer *constrainer;

		ComponentDragger dragger;

		FilterDragOverlay& parent;
	};

	void removeFilter(int index);

	double getGain(int y)
	{
		return (double)filterGraph.yToGain((float)y, 24.0f);
	}

	void selectDragger(int index);

	void addListener(Listener* l)
	{
		listeners.addIfNotAlreadyThere(l);
	}

	void removeListener(Listener* l)
	{
		listeners.removeAllInstancesOf(l);
	}

private:

	CurveEq *eq;

	int numFilters = 0;

	struct FFTDisplay : public Component,
						public FFTDisplayBase
	{
		FFTDisplay(FilterDragOverlay& parent_) :
			FFTDisplayBase(parent_.eq->getFFTBuffer()),
			parent(parent_)
		{
			freqToXFunction = std::bind(&FilterGraph::freqToX, &parent.filterGraph, std::placeholders::_1);
		};

		void paint(Graphics& g) override
		{
			if(ringBuffer.isActive())
				FFTDisplayBase::drawSpectrum(g);
		}

		double getSamplerate() const override
		{
			return parent.eq->getSampleRate();
		}

		Colour getColourForAnalyserBase(int colourId)
		{
			if (colourId == AudioAnalyserComponent::bgColour)
				return Colours::transparentBlack;
			if (colourId == AudioAnalyserComponent::ColourId::fillColour)
				return findColour(colourId);
			if(colourId == AudioAnalyserComponent::ColourId::lineColour)
				return Colours::white.withAlpha(0.2f);

			return Colours::blue;
		}

		FilterDragOverlay& parent;
	} fftAnalyser;

	FilterGraph filterGraph;

	Array<WeakReference<Listener>> listeners;

	UpdateMerger repaintUpdater;
	
	int selectedIndex;

	ScopedPointer<ComponentBoundsConstrainer> constrainer;

	OwnedArray<FilterDragComponent> dragComponents;
};

//[/Headers]



//==============================================================================
/**
                                                                    //[Comments]
    An auto-generated component, created by the Introjucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class CurveEqEditor  : public ProcessorEditorBody,
                       public FilterTypeSelector::Listener,
					   public FilterDragOverlay::Listener,
                       public ButtonListener,
                       public SliderListener
{
public:
    //==============================================================================
    CurveEqEditor (ProcessorEditor *p);
    ~CurveEqEditor();

    //==============================================================================

	void filterSelectorChanged(FilterTypeSelector *) override
	{

	}

	void updateGui() override
	{
		CurveEq *eq = dynamic_cast<CurveEq*>(getProcessor());

		numFilters = eq->getNumFilterBands();

		if (isPositiveAndBelow(currentlySelectedFilterBand, numFilters))
		{
			freqSlider->updateValue();
			gainSlider->updateValue();
			qSlider->updateValue();
			enableBandButton->updateValue();
			typeSelector->updateValue();
		}

        freqSlider->setEnabled(freqSlider->isEnabled() && numFilters != 0);
        gainSlider->setEnabled(gainSlider->isEnabled() && numFilters != 0);
        qSlider->setEnabled(qSlider->isEnabled() && numFilters != 0);
        enableBandButton->setEnabled(enableBandButton->isEnabled() && numFilters != 0);
        typeSelector->setEnabled(numFilters != 0);
	};

	int getBodyHeight() const override {return h; };
	
	void filterBandSelected(int currentSelection) override
	{
		currentlySelectedFilterBand = currentSelection;

		CurveEq *eq = dynamic_cast<CurveEq*>(getProcessor());

		freqSlider->setup(eq, eq->getParameterIndex(currentlySelectedFilterBand, CurveEq::BandParameter::Freq), "Frequency " + String(currentlySelectedFilterBand));
		freqSlider->setMode(HiSlider::Frequency);
        freqSlider->updateValue();
		freqSlider->setEnabled(true);

		gainSlider->setup(eq, eq->getParameterIndex(currentlySelectedFilterBand, CurveEq::BandParameter::Gain), "Gain " + String(currentlySelectedFilterBand));
        gainSlider->setMode(HiSlider::Decibel, -24.0, 24.0, 0.0);
		gainSlider->updateValue();

		qSlider->setup(eq, eq->getParameterIndex(currentlySelectedFilterBand, CurveEq::BandParameter::Q), "Q " + String(currentlySelectedFilterBand));
		qSlider->updateValue();
        qSlider->setMode(HiSlider::Linear, 0.1, 8.0, 1.0);

		enableBandButton->setup(eq, eq->getParameterIndex(currentlySelectedFilterBand, CurveEq::BandParameter::Enabled), "Enabled " + String(currentlySelectedFilterBand));
		enableBandButton->updateValue();

		typeSelector->setup(eq, eq->getParameterIndex(currentlySelectedFilterBand, CurveEq::BandParameter::Type), "Type " + String(currentlySelectedFilterBand));
		typeSelector->updateValue();
	}

	void bandRemoved(int bandIndexThatWasRemoved) override
	{
		if(bandIndexThatWasRemoved == currentlySelectedFilterBand)
		{
			freqSlider->setup(getProcessor(), -1, "Disabled");
			freqSlider->setEnabled(false);
			gainSlider->setup(getProcessor(), -1, "Disabled");
			qSlider->setup(getProcessor(), -1, "Disabled");
			enableBandButton->setup(getProcessor(), -1, "Disabled");
			typeSelector->setup(getProcessor(), -1, "Disabled");
            
            freqSlider->setEnabled(false);
            gainSlider->setEnabled(false);
            qSlider->setEnabled(false);
            enableBandButton->setEnabled(false);
            typeSelector->setEnabled(false);
		}
	}

    void paint (Graphics& g);
    void resized();
    void buttonClicked (Button* buttonThatWasClicked);
    void sliderValueChanged (Slider* sliderThatWasMoved);



private:
	int h;

	int numFilters;

	int currentlySelectedFilterBand;

	ScopedPointer<ToggleButton> fftEnableButton;

    //==============================================================================
    ScopedPointer<FilterTypeSelector> typeSelector;
    ScopedPointer<FilterDragOverlay> dragOverlay;
    ScopedPointer<HiToggleButton> enableBandButton;
    ScopedPointer<HiSlider> freqSlider;
    ScopedPointer<HiSlider> gainSlider;
    ScopedPointer<HiSlider> qSlider;
    ScopedPointer<Label> label;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CurveEqEditor)
};

} // namespace hise

