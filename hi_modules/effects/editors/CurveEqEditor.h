/*
  ==============================================================================

  This is an automatically generated GUI class created by the Introjucer!

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Created with Introjucer version: 4.1.0

  ------------------------------------------------------------------------------

  The Introjucer is part of the JUCE library - "Jules' Utility Class Extensions"
  Copyright (c) 2015 - ROLI Ltd.

  ==============================================================================
*/

#ifndef __JUCE_HEADER_53931F8EE608D824__
#define __JUCE_HEADER_53931F8EE608D824__

//[Headers]     -- You can add your own extra header files here --

class CurveEqEditor;

class FilterTypeSelector: public Component,
						  public MacroControlledObject,
						  public ButtonListener
{
public:

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

	void updateValue() override
	{
		if(parameter == -1)
		{
			selectButton(-1);
		}
		else
		{
			selectButton((int)getProcessor()->getAttribute(parameter));
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

		addAndMakeVisible(lowPassButton = new ShapeButton("LowPass", Colours::white.withAlpha(0.5f), Colours::white.withAlpha(0.7f), Colours::white));
		lowPassButton->setShape(getPath(CurveEq::LowPass), true, true, true);
		lowPassButton->addListener(this);

		addAndMakeVisible(highPassButton = new ShapeButton("HighPass", Colours::white.withAlpha(0.5f), Colours::white.withAlpha(0.7f), Colours::white));
		highPassButton->setShape(getPath(CurveEq::HighPass), true, true, true);
		highPassButton->addListener(this);

		addAndMakeVisible(lowShelfButton = new ShapeButton("LowShelf", Colours::white.withAlpha(0.5f), Colours::white.withAlpha(0.7f), Colours::white));
		lowShelfButton->setShape(getPath(CurveEq::LowShelf), true, true, true);
		lowShelfButton->addListener(this);

		addAndMakeVisible(highShelfButton = new ShapeButton("HighShelf", Colours::white.withAlpha(0.5f), Colours::white.withAlpha(0.7f), Colours::white));
		highShelfButton->setShape(getPath(CurveEq::HighShelf), true, true, true);
		highShelfButton->addListener(this);

		addAndMakeVisible(peakButton = new ShapeButton("Peak", Colours::white.withAlpha(0.5f), Colours::white.withAlpha(0.7f), Colours::white));
		peakButton->setShape(getPath(CurveEq::Peak), true, true, true);
		peakButton->addListener(this);

		buttons.add(lowPassButton);
		buttons.add(highPassButton);
		buttons.add(lowShelfButton);
		buttons.add(highShelfButton);
		buttons.add(peakButton);

	};



	Path getPath(CurveEq::FilterType type)
	{
		Path path;

		switch(type)
		{
		case CurveEq::LowPass:
			{
				static const unsigned char pathData[] = { 110,109,0,0,210,66,0,48,204,67,98,0,0,210,66,85,133,205,67,0,0,210,66,171,218,206,67,0,0,210,66,0,48,208,67,98,184,49,227,66,2,50,208,67,22,100,244,66,241,43,208,67,179,202,2,67,22,51,208,67,98,161,92,7,67,235,74,208,67,249,38,11,67,117,227,209,67,133,
				142,13,67,23,186,211,67,98,235,153,15,67,165,60,213,67,185,29,17,67,240,235,214,67,0,32,18,67,1,172,216,67,98,85,181,20,67,86,89,216,67,171,74,23,67,172,6,216,67,0,224,25,67,1,180,215,67,98,73,187,23,67,129,232,211,67,21,186,19,67,66,42,208,67,62,20,
				13,67,100,229,205,67,98,35,105,9,67,120,158,204,67,58,229,4,67,231,20,204,67,152,118,0,67,1,48,204,67,98,202,72,241,66,1,48,204,67,102,164,225,66,1,48,204,67,0,0,210,66,1,48,204,67,99,101,0,0 };

				path.loadPathFromData (pathData, sizeof (pathData));
				break;
			}
		case CurveEq::HighPass:
			{
				static const unsigned char pathData[] = { 110,109,0,0,112,66,0,48,204,67,98,142,227,74,66,0,48,204,67,69,230,49,66,242,81,207,67,0,224,35,66,0,32,210,67,98,187,217,21,66,14,238,212,67,0,128,16,66,0,180,215,67,0,128,16,66,0,180,215,67,108,0,128,47,66,0,172,216,67,98,0,128,47,66,0,172,216,67,69,
				38,52,66,242,109,214,67,0,32,63,66,0,60,212,67,98,187,25,74,66,14,10,210,67,114,28,89,66,0,48,208,67,0,0,112,66,0,48,208,67,108,0,0,170,66,0,48,208,67,108,0,0,170,66,0,48,204,67,108,0,0,112,66,0,48,204,67,99,101,0,0 };

				path.loadPathFromData (pathData, sizeof (pathData));
				break;
			}
		case CurveEq::HighShelf:
			{
				static const unsigned char pathData[] = { 110,109,0,0,92,67,0,48,199,67,98,44,153,88,67,118,34,199,67,167,11,86,67,12,117,200,67,149,34,84,67,225,179,201,67,98,167,166,81,67,71,62,203,67,118,235,79,67,158,20,205,67,172,31,77,67,32,126,206,67,98,91,90,76,67,19,199,206,67,72,65,77,67,149,170,206,
				67,147,30,76,67,0,176,206,67,98,184,105,71,67,0,176,206,67,220,180,66,67,0,176,206,67,1,0,62,67,0,176,206,67,98,1,0,62,67,85,5,208,67,1,0,62,67,171,90,209,67,1,0,62,67,0,176,210,67,98,176,50,67,67,37,174,210,67,184,101,72,67,192,179,210,67,46,152,77,
				67,37,173,210,67,98,21,99,81,67,70,140,210,67,121,214,83,67,128,221,208,67,40,232,85,67,171,120,207,67,98,186,233,87,67,30,30,206,67,99,120,89,67,142,147,204,67,86,224,91,67,225,97,203,67,98,167,165,92,67,238,24,203,67,186,190,91,67,108,53,203,67,111,
				225,92,67,1,48,203,67,98,245,64,99,67,1,48,203,67,123,160,105,67,1,48,203,67,1,0,112,67,1,48,203,67,98,1,0,112,67,172,218,201,67,1,0,112,67,86,133,200,67,1,0,112,67,1,48,199,67,98,86,85,105,67,1,48,199,67,172,170,98,67,1,48,199,67,1,0,92,67,1,48,199,
				67,99,101,0,0 };

				path.loadPathFromData (pathData, sizeof (pathData));
				break;

			}
		case CurveEq::LowShelf:
			{
				static const unsigned char pathData[] = { 110,109,0,0,117,67,92,174,198,67,98,0,0,117,67,177,3,200,67,0,0,117,67,7,89,201,67,0,0,117,67,92,174,202,67,98,171,170,123,67,92,174,202,67,171,42,129,67,92,174,202,67,0,128,132,67,92,174,202,67,98,208,39,132,67,172,192,202,67,179,75,133,67,224,102,203,
				67,42,91,133,67,40,199,203,67,98,2,219,134,67,173,188,205,67,26,242,135,67,189,28,208,67,19,254,137,67,177,149,209,67,98,144,42,139,67,213,105,210,67,132,163,140,67,64,33,210,67,55,251,141,67,91,46,210,67,98,37,210,143,67,91,46,210,67,18,169,145,67,91,
				46,210,67,0,128,147,67,91,46,210,67,98,0,128,147,67,6,217,208,67,0,128,147,67,176,131,207,67,0,128,147,67,91,46,206,67,98,0,0,145,67,91,46,206,67,0,128,142,67,91,46,206,67,0,0,140,67,91,46,206,67,98,48,88,140,67,11,28,206,67,77,52,139,67,215,117,205,
				67,214,36,139,67,143,21,205,67,98,254,164,137,67,10,32,203,67,230,141,136,67,250,191,200,67,237,129,134,67,6,71,199,67,98,112,85,133,67,226,114,198,67,124,220,131,67,119,187,198,67,201,132,130,67,92,174,198,67,98,12,177,127,67,92,174,198,67,134,88,122,
				67,92,174,198,67,0,0,117,67,92,174,198,67,99,101,0,0 };

				path.loadPathFromData (pathData, sizeof (pathData));
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

				path.loadPathFromData (pathData, sizeof (pathData));

				break;
			}
            case CurveEq::numFilterTypes: break;
		}

		return path;
	}



	void resized()
	{
		lowPassButton->setBounds(5, 4, 20, 20);
		highPassButton->setBounds(lowPassButton->getRight() + 4, 4, 20, 20);
		lowShelfButton->setBounds(highPassButton->getRight() + 2, 4, 20, 20);
		highShelfButton->setBounds(lowShelfButton->getRight() + 2, 4, 20, 20);
		peakButton->setBounds(highShelfButton->getRight() + 2, 4, 20, 20);
	}

	void paint(Graphics &g)
	{
		KnobLookAndFeel::drawHiBackground(g, 1, 1, getWidth() - 2, getHeight() - 2, nullptr, false);
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
						 public Timer,
						 public SettableTooltipClient
{
public:

	FilterDragOverlay():
		fftRange(-80)
	{
		constrainer = new ComponentBoundsConstrainer();

		repaintUpdater.setManualCountLimit(4);


        FloatVectorOperations::clear(fftAmpData, FFT_SIZE_FOR_EQ);

        FloatVectorOperations::clear(gainValues, FFT_SIZE_FOR_EQ);

        FloatVectorOperations::clear(currentGainValues, FFT_SIZE_FOR_EQ);
	}

	~FilterDragOverlay()
	{
		stopTimer();
	}

	void clearFFTDisplay()
	{
		p.clear();
		repaint();
	}

	void timerCallback() override;

	void paint(Graphics &g);

	void setEq(CurveEqEditor* editor, CurveEq* eq);

	void resized() override
	{
		constrainer->setMinimumOnscreenAmounts(15, 15, 15, 15);
	}

	void mouseMove(const MouseEvent &e);

	void mouseDown(const MouseEvent &e);

	void addFilter(int x, int y, const MouseEvent* event=nullptr)
	{
		FilterDragComponent *dc = new FilterDragComponent(editor, eq, dragComponents.size());

		addAndMakeVisible(dc);

		dc->setConstrainer(constrainer);

		dc->setBounds(x - 12, y - 12, 24, 24);

		if(event != nullptr)
		{
			dc->startDragging(event);
		}

		dragComponents.add(dc);

		selectDragger(dragComponents.size() - 1);
	};

	void updatePositions()
	{
		for(int i = 0; i < dragComponents.size(); i++)
		{
			Point<int> p = getPosition(i);

			dragComponents[i]->setTopLeftPosition(p.x - 12, p.y - 12);
		}
	}

	void mouseDrag(const MouseEvent &e) override
	{
		if( dragComponents[selectedIndex] != nullptr)
		{
			dragComponents[selectedIndex]->mouseDrag(e);
		}
	}

	Point<int> getPosition(int index);

	class FilterDragComponent: public Component
	{
	public:

		FilterDragComponent(CurveEqEditor *editor_, CurveEq *eq_, int index_):
			editor(editor_),
			eq(eq_),
			index(index_)
		{};

		void setConstrainer(ComponentBoundsConstrainer *constrainer_)
		{
			constrainer = constrainer_;
		};

		void mouseDown (const MouseEvent& e);

		void mouseDrag (const MouseEvent& e);

		void mouseWheelMove(const MouseEvent &e, const MouseWheelDetails &d) override;

		void setSelected(bool shouldBeSelected)
		{
			selected = shouldBeSelected;
			repaint();
		}

		void paint(Graphics &g)
		{
			Rectangle<float> bounds = Rectangle<float>(0.0f, 0.0f, (float)getWidth(), (float)getHeight());

			bounds.reduce(6.0f, 6.0f);

			g.setColour(Colours::black.withAlpha(0.3f));
			g.fillRoundedRectangle(bounds, 3.0f);

			const bool enabled = eq->getFilterBand(index)->isEnabled();

			g.setColour(Colours::white.withAlpha(enabled ? 1.0f : 0.3f));





			g.drawRoundedRectangle(bounds, 3.0f, selected ? 2.0f : 1.0f);

			g.setFont(GLOBAL_BOLD_FONT().withHeight(11.0f));
			g.drawText(String(index), getLocalBounds(), Justification::centred, false);


		};

		void startDragging(const MouseEvent *e)
		{
			jassert(e != nullptr);

			MouseEvent newEvent = e->getEventRelativeTo(this);

			mouseDown(newEvent);


			//dragger.startDraggingComponent (this, newEvent);
		}

		void setIndex(int newIndex)
		{
			index = newIndex;
		};

	private:

		int index;

		bool selected;

		ComponentBoundsConstrainer *constrainer;

		ComponentDragger dragger;

		CurveEq *eq;
		CurveEqEditor *editor;

	};

	void removeFilter(FilterDragComponent *componentToRemove);

	double getGain(int y)
	{
		const double gainPosition = (double)y / (double)getHeight();

		const double decibel = (1.0 - gainPosition) * 48.0 - 24.0;

		//const double gain = Decibels::decibelsToGain(decibel);

		return jlimit<double>(-24.0, 24.0, decibel);
	}

	void selectDragger(int index);


	void setFFTRange(double newRange)
	{
		fftRange = newRange;
	};

private:

	UpdateMerger repaintUpdater;

	Path p;

	CurveEq *eq;
	CurveEqEditor *editor;

	double fftRange;

	std::complex<double> fftData[FFT_SIZE_FOR_EQ];

	double fftAmpData[FFT_SIZE_FOR_EQ];

	float gainValues[FFT_SIZE_FOR_EQ];

	float currentGainValues[FFT_SIZE_FOR_EQ];

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
                       public Timer,
                       public FilterTypeSelector::Listener,
                       public ButtonListener,
                       public SliderListener
{
public:
    //==============================================================================
    CurveEqEditor (BetterProcessorEditor *p);
    ~CurveEqEditor();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.

	void filterSelectorChanged(FilterTypeSelector *) override
	{

	}

	void updateGui() override
	{
		CurveEq *c = dynamic_cast<CurveEq*>(getProcessor());

		if(numFilters != c->getNumFilterBands())
		{
			updateFilters();

		}

		checkEnabledBands();

		freqSlider->updateValue();
		gainSlider->updateValue();
		qSlider->updateValue();
		enableBandButton->updateValue();
		typeSelector->updateValue();
	};

	void checkEnabledBands()
	{
		CurveEq *c = dynamic_cast<CurveEq*>(getProcessor());

		for(int i = 0; i < numFilters; i++)
		{
			filterGraph->enableBand(i, c->getFilterBand(i)->isEnabled());
		}
	}

	void updateFilters()
	{
		CurveEq *c = dynamic_cast<CurveEq*>(getProcessor());

		filterGraph->clearBands();

		for(int i = 0; i < c->getNumFilterBands(); i++)
		{
			addFilter(i, c->getFilterBand(i)->getFilterType());
		}

		numFilters = c->getNumFilterBands();

		if(numFilters == 0)
		{
			filterGraph->repaint();
		}
	}

	void updateCoefficients()
	{
		CurveEq *c = dynamic_cast<CurveEq*>(getProcessor());

		for(int i = 0; i < c->getNumFilterBands(); i++)
		{
			IIRCoefficients ic = c->getCoefficients(i);

			filterGraph->setCoefficients(i, getProcessor()->getSampleRate(), ic);
		}
	}

	void addFilter(int filterIndex, int filterType)
	{
		CurveEq *c = dynamic_cast<CurveEq*>(getProcessor());

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

	int getBodyHeight() const override {return h; };

	void timerCallback() override
	{
		CurveEq *c = dynamic_cast<CurveEq*>(getProcessor());

		if(numFilters == c->getNumFilterBands())
		{
			updateCoefficients();
		}
	}

	FilterGraph *getFilterGraph()
	{
		return filterGraph;
	}

	void setSelectedFilterBand(int currentSelection)
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

	void bandRemoved(int bandIndexThatWasRemoved)
	{
		if(bandIndexThatWasRemoved == currentlySelectedFilterBand)
		{
			freqSlider->setup(getProcessor(), -1, "Disabled");
			freqSlider->setEnabled(false);
			gainSlider->setup(getProcessor(), -1, "Disabled");
			qSlider->setup(getProcessor(), -1, "Disabled");
			enableBandButton->setup(getProcessor(), -1, "Disabled");
			typeSelector->setup(getProcessor(), -1, "Disabled");
		}
	}

    //[/UserMethods]

    void paint (Graphics& g);
    void resized();
    void buttonClicked (Button* buttonThatWasClicked);
    void sliderValueChanged (Slider* sliderThatWasMoved);



private:
    //[UserVariables]   -- You can add your own custom variables in this section.
	int h;

	int numFilters;

	int currentlySelectedFilterBand;

	ScopedPointer<ShapeButton> fftEnableButton;

    //[/UserVariables]

    //==============================================================================
    ScopedPointer<FilterGraph> filterGraph;
    ScopedPointer<FilterTypeSelector> typeSelector;
    ScopedPointer<FilterDragOverlay> dragOverlay;
    ScopedPointer<HiToggleButton> enableBandButton;
    ScopedPointer<HiSlider> freqSlider;
    ScopedPointer<HiSlider> gainSlider;
    ScopedPointer<HiSlider> qSlider;
    ScopedPointer<Slider> fftRangeSlider;
    ScopedPointer<Label> label;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CurveEqEditor)
};

//[EndFile] You can add extra defines here...
//[/EndFile]

#endif   // __JUCE_HEADER_53931F8EE608D824__
