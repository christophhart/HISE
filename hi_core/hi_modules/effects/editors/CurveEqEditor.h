
#pragma once

namespace hise { 
using namespace juce;

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

	void updateValue(NotificationType /*sendAttributeChange*/ = sendNotification) override
	{
		if(parameter == -1)
		{
			selectButton(-1);
		}
		else
		{
			auto p = getProcessor()->getAttribute(parameter);
			selectButton((int)p);
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

        FilterDragOverlay::Factory f;

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

		initMacroControl(dontSendNotification);
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

