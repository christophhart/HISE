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


namespace hise { using namespace juce;


//==============================================================================
CurveEqEditor::CurveEqEditor (ProcessorEditor *p)
    : ProcessorEditorBody(p)
{
	auto eq = dynamic_cast<CurveEq*>(p->getProcessor());

    addAndMakeVisible (typeSelector = new FilterTypeSelector());
    typeSelector->setName ("new component");

    addAndMakeVisible (dragOverlay = new FilterDragOverlay(eq));
    dragOverlay->setName ("new component");

	dragOverlay->addListener(this);

    addAndMakeVisible (enableBandButton = new HiToggleButton ("new toggle button"));
    enableBandButton->setButtonText (TRANS("Enable Band"));
    enableBandButton->addListener (this);
    enableBandButton->setColour (ToggleButton::textColourId, Colours::white);

    addAndMakeVisible (freqSlider = new HiSlider ("Frequency"));
    freqSlider->setTooltip (TRANS("Set the frequency of the selected band"));
    freqSlider->setRange (0, 20000, 1);
    freqSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    freqSlider->setTextBoxStyle (Slider::TextBoxRight, false, 80, 20);
    freqSlider->addListener (this);

    addAndMakeVisible (gainSlider = new HiSlider ("Gain"));
    gainSlider->setRange (-24, 24, 0.1);
    gainSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    gainSlider->setTextBoxStyle (Slider::TextBoxRight, false, 80, 20);
    gainSlider->addListener (this);

    addAndMakeVisible (qSlider = new HiSlider ("Q"));
    qSlider->setRange (0.1, 8, 1);
    qSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    qSlider->setTextBoxStyle (Slider::TextBoxRight, false, 80, 20);
    qSlider->addListener (this);

    addAndMakeVisible (label = new Label ("new label",
                                          TRANS("curve eq")));
    label->setFont (Font ("Arial", 26.00f, Font::bold));
    label->setJustificationType (Justification::centredRight);
    label->setEditable (false, false, false);
    label->setColour (Label::textColourId, Colour (0x52ffffff));
    label->setColour (TextEditor::textColourId, Colours::black);
    label->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    ProcessorEditorLookAndFeel::setupEditorNameLabel(label);
    
    label->setFont(GLOBAL_BOLD_FONT().withHeight(26.0f));
    
	currentlySelectedFilterBand = -1;

	numFilters = 0;

	freqSlider->setup(getProcessor(), -1, "Frequency");
	freqSlider->setMode(HiSlider::Frequency);

	gainSlider->setup(getProcessor(), -1, "Gain");
	gainSlider->setMode(HiSlider::Decibel, -24.0, 24.0, 0.0);

	qSlider->setup(getProcessor(), -1, "Q");
	qSlider->setMode(HiSlider::Linear, 0.1, 8.0, 1.0);

	enableBandButton->setup(getProcessor(), -1, "Enable Band");

	typeSelector->setup(getProcessor(), -1, "Type");

	addAndMakeVisible(fftEnableButton = new ToggleButton("Spectrum Analyser"));
	fftEnableButton->addListener(this);
	fftEnableButton->setTooltip("Enable FFT plotting");
	getProcessor()->getMainController()->skin(*fftEnableButton);

    setSize (800, 320);


	h = getHeight();
}

CurveEqEditor::~CurveEqEditor()
{
    typeSelector = nullptr;
    dragOverlay = nullptr;
    enableBandButton = nullptr;
    freqSlider = nullptr;
    gainSlider = nullptr;
    qSlider = nullptr;
    label = nullptr;

}

//==============================================================================
void CurveEqEditor::paint (Graphics& g)
{
	ProcessorEditorLookAndFeel::fillEditorBackgroundRectFixed(g, this, 700);
}

void CurveEqEditor::resized()
{
	auto b = getLocalBounds().withSizeKeepingCentre(700, getHeight() - 6);

	auto right = b.removeFromRight(148).reduced(10);

	dragOverlay->setBounds(b);

	label->setBounds(right.removeFromTop(20));

	right.removeFromTop(15);
	typeSelector->setBounds(right.removeFromTop(32));
	right.removeFromTop(5);
	enableBandButton->setBounds(right.removeFromTop(32));
	right.removeFromTop(5);
	freqSlider->setBounds(right.removeFromTop(48));
	gainSlider->setBounds(right.removeFromTop(48));
	qSlider->setBounds(right.removeFromTop(48));
	right.removeFromTop(10);
	fftEnableButton->setBounds(right.removeFromTop(32));
}

void CurveEqEditor::buttonClicked (Button* buttonThatWasClicked)
{
    if (buttonThatWasClicked == enableBandButton)
    {
		auto eq = dynamic_cast<CurveEq*>(getProcessor());

		if (currentlySelectedFilterBand != -1)
		{
			auto pIndex = CurveEq::BandParameter::numBandParameters * currentlySelectedFilterBand + CurveEq::BandParameter::Enabled;

			eq->setAttribute(pIndex, enableBandButton->getToggleState(), sendNotification);
		}
    }

	if(buttonThatWasClicked == fftEnableButton)
	{
		const bool on = fftEnableButton->getToggleState();

		auto eq = dynamic_cast<CurveEq*>(getProcessor());

		eq->enableSpectrumAnalyser(on);
	}

}

void CurveEqEditor::sliderValueChanged (Slider* sliderThatWasMoved)
{
    if (sliderThatWasMoved == freqSlider)
    {
    }
    else if (sliderThatWasMoved == gainSlider)
    {
    }
    else if (sliderThatWasMoved == qSlider)
    {
    }

	dragOverlay->updatePositions(false);
}

Point<int> FilterDragOverlay::getPosition(int index)
{
	if (isPositiveAndBelow(index, eq->getNumFilterBands()))
	{
		const int freqIndex = eq->getParameterIndex(index, CurveEq::BandParameter::Freq);
		const int gainIndex = eq->getParameterIndex(index, CurveEq::BandParameter::Gain);

		const int x = (int)filterGraph.freqToX(eq->getAttribute(freqIndex));
		const int y = (int)filterGraph.gainToY(eq->getAttribute(gainIndex), 24.0f);

		return Point<int>(x, y).translated(offset, offset);
	}
	else
		return {};
}

void FilterDragOverlay::mouseDown(const MouseEvent &e)
{
	if (e.mods.isRightButtonDown())
	{
		PopupLookAndFeel plaf;
		PopupMenu m;
		m.setLookAndFeel(&plaf);

		m.addItem(1, "Delete all bands", true, false);
		m.addItem(2, "Enable Spectrum Analyser", true, eq->getFFTBuffer().isActive());
		m.addItem(3, "Cancel");

		auto result = m.show();
		
		if (result == 3)
			return;

		if (result == 1)
		{
			while (eq->getNumFilterBands() > 0)
				eq->removeFilterBand(0);
		}
		else if (result == 2)
		{
			eq->enableSpectrumAnalyser(!eq->getFFTBuffer().isActive());
		}
	}
	else
	{
		const double freq = (double)filterGraph.xToFreq((float)e.getPosition().x-offset);
		const double gain = Decibels::decibelsToGain((double)getGain(e.getPosition().y-offset));

		eq->addFilterBand(freq, gain);
	}
}

void FilterDragOverlay::mouseUp(const MouseEvent& e)
{
	selectDragger(-1);
}

void FilterDragOverlay::FilterDragComponent::mouseDown (const MouseEvent& e)
{
	if(e.mods.isRightButtonDown())
	{
		StringArray sa = { "Low Pass", "High Pass", "Low Shelf", "High Shelf", "Peak" };

		PopupLookAndFeel plaf;
		PopupMenu m;
		m.setLookAndFeel(&plaf);

		FilterTypeSelector::Factory pf;

		auto thisBand = parent.eq->getFilterBand(index);

		int typeOffset = 8000;

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

			m.addItem(typeOffset + i, sa[i], true, isSelected, dp);
		}

		m.addSeparator();
		m.addItem(3, "Cancel");

		auto result = m.show();

		if (result == 0 || result == 3)
			return;
		else if (result == 9000)
		{
			auto tmp = &parent;
			auto index_ = index;
			auto f = [tmp, index_]()
			{
				tmp->removeFilter(index_);
			};

			MessageManager::callAsync(f);
		}
		else if (result == 10000)
		{
			auto enabled = thisBand->isEnabled();

			auto pIndex = parent.eq->getParameterIndex(index, CurveEq::BandParameter::Enabled);
			parent.eq->setAttribute(pIndex, enabled ? 0.0f : 1.0f, sendNotification);
		}
		else
		{
			auto t = result - typeOffset;
			auto pIndex = parent.eq->getParameterIndex(index, CurveEq::BandParameter::Type);
			parent.eq->setAttribute(pIndex, (float)t, sendNotification);
		}

		
	}
	else
	{
		parent.selectDragger(index);
		dragger.startDraggingComponent (this, e);
	}
}

void FilterDragOverlay::FilterDragComponent::mouseUp(const MouseEvent& e)
{
	draggin = false;
	parent.selectDragger(-1);
}

void FilterDragOverlay::removeFilter(int index)
{
	eq->removeFilterBand(index);

	for (auto l : listeners)
	{
		if (l != nullptr)
			l->bandRemoved(index);
	}
}

void FilterDragOverlay::FilterDragComponent::mouseDrag (const MouseEvent& e)
{
	auto te = e.getEventRelativeTo(this);

	if (!draggin)
	{
		dragger.startDraggingComponent(this, te);
		draggin = true;
	}

	dragger.dragComponent (this, te, constrainer);

	auto x = getBoundsInParent().getCentreX() - parent.offset;
	auto y = getBoundsInParent().getCentreY() - parent.offset;

	const double freq = jlimit<double>(20.0, 20000.0, (double)parent.filterGraph.xToFreq((float)x));

	const double gain = parent.filterGraph.yToGain(y, 24.0f);

	const int freqIndex = parent.eq->getParameterIndex(index, CurveEq::BandParameter::Freq);
	const int gainIndex = parent.eq->getParameterIndex(index, CurveEq::BandParameter::Gain);

	parent.eq->setAttribute(freqIndex, (float)freq, sendNotification);
	parent.eq->setAttribute(gainIndex, (float)gain, sendNotification);
}

void FilterDragOverlay::mouseMove(const MouseEvent &e)
{
	setTooltip(String(getGain(e.getPosition().y), 1) + " dB / " + String((int)filterGraph.xToFreq((float)e.getPosition().x)) + " Hz");
};


void FilterDragOverlay::FilterDragComponent::mouseWheelMove(const MouseEvent &e, const MouseWheelDetails &d)
{
	if(e.mods.isCtrlDown())
	{
		double q = parent.eq->getFilterBand(index)->getQ();

		if(d.deltaY > 0)
			q = jmin<double>(8.0, q * 1.3);
		else
			q = jmax<double>(0.1, q / 1.3);

		const int qIndex = parent.eq->getParameterIndex(index, CurveEq::BandParameter::Q);

		parent.eq->setAttribute(qIndex, (float)q, sendNotification);

	}
	else
	{
		getParentComponent()->mouseWheelMove(e, d);
	}
}

void FilterDragOverlay::selectDragger(int index)
{
	selectedIndex = index;

	for(int i = 0; i < dragComponents.size(); i++)
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
	}
}


juce::Path FilterTypeSelector::Factory::createPath(const String& url) const
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

} // namespace hise
