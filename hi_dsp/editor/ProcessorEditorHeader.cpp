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

#define IS(C) (ProcessorHelpers::is<C>(getProcessor()))

ProcessorEditorHeader::ProcessorEditorHeader(ProcessorEditor *p) :
	ProcessorEditorChildComponent(p),
	isSoloHeader(false)
{
	setLookAndFeel();

	p->getProcessor()->getMainController()->addScriptListener(this);

	setOpaque(true);

	auto drawColour = Colours::white;

	if (isHeaderOfChain() || isHeaderOfModulatorSynth())
		drawColour = Colours::black;

    addAndMakeVisible (valueMeter = new VuMeter());
	valueMeter->setType(VuMeter::StereoHorizontal);
	valueMeter->setColour (VuMeter::backgroundColour, Colour (0xFF333333));
	valueMeter->setColour (VuMeter::ledColour, Colours::lightgrey);
	valueMeter->setColour (VuMeter::outlineColour, drawColour.withAlpha(0.5f));
	
	#if JUCE_DEBUG
	startTimer(150);
#else
	startTimer(30);
#endif

    addAndMakeVisible (idLabel = new Label ("ID Label",
                                            TRANS("ModulatorName")));
    idLabel->setTooltip (TRANS("The Modulator ID"));
	idLabel->setFont (GLOBAL_BOLD_FONT());
    
    idLabel->setJustificationType (Justification::centredLeft);
    idLabel->setEditable(false, true, true);
    idLabel->setColour (Label::backgroundColourId, Colour (0x00000000));
    idLabel->setColour (Label::textColourId, drawColour.withAlpha(0.7f));
    idLabel->setColour (Label::outlineColourId, Colour (0x00ffffff));
    idLabel->setColour (Label::textWhenEditingColourId, drawColour);
    idLabel->setColour (Label::ColourIds::backgroundWhenEditingColourId, Colours::transparentBlack);
	idLabel->setColour(TextEditor::ColourIds::highlightedTextColourId, drawColour.contrasting(1.0f));
	idLabel->setColour(TextEditor::ColourIds::highlightColourId, Colour(SIGNAL_COLOUR));
    idLabel->setColour (TextEditor::textColourId, drawColour);
    idLabel->setColour (TextEditor::backgroundColourId, Colour (0x00000000));
    idLabel->setColour(TextEditor::ColourIds::focusedOutlineColourId, Colours::transparentBlack);
    idLabel->setColour(CaretComponent::ColourIds::caretColourId, drawColour);
    
	idLabel->addListener(this);
	
	idLabel->setEditable(!isHeaderOfChain() || isHeaderOfModulatorSynth());

    idLabel->setBufferedToImage(true);
    
	addAndMakeVisible(chainIcon = new ChainIcon(getProcessor()));

    addAndMakeVisible (typeLabel = new Label ("Type Label",
                                              TRANS("Modulator")));
    typeLabel->setTooltip (TRANS("The Modulator Type"));
    typeLabel->setFont (GLOBAL_FONT());
    typeLabel->setJustificationType (Justification::centredLeft);
    typeLabel->setEditable (false, false, false);
    typeLabel->setColour (Label::backgroundColourId, Colour (0x00000000));
    typeLabel->setColour (Label::textColourId, drawColour);
    typeLabel->setColour (Label::outlineColourId, Colour (0x00000000));
    
    typeLabel->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    
	addAndMakeVisible(bypassButton = new HeaderButton("Bypass Button", (const unsigned char*)HiBinaryData::ProcessorEditorHeaderIcons::bypassShape, SIZE_OF_PATH(HiBinaryData::ProcessorEditorHeaderIcons::bypassShape), this));

	addAndMakeVisible (foldButton = new ShapeButton ("Fold", drawColour, drawColour, drawColour));
	
	checkFoldButton();

    foldButton->setTooltip (TRANS("Expand this Processor"));
    foldButton->addListener (this);

	addAndMakeVisible(workspaceButton = new ShapeButton("Workspace", drawColour, drawColour, drawColour));
    Path workspacePath;
    workspacePath.loadPathFromData(ColumnIcons::openWorkspaceIcon, sizeof(ColumnIcons::openWorkspaceIcon));
	workspaceButton->setShape(workspacePath, true, true, true);
	workspaceButton->addListener(this);
	workspaceButton->setToggleState(true, dontSendNotification);
	workspaceButton->setTooltip("Open " + getProcessor()->getId() + " in workspace");
	refreshShapeButton(workspaceButton);

    
    addAndMakeVisible (deleteButton = new ShapeButton ("Delete Processor", drawColour, drawColour, drawColour));
	Path deletePath;
	deletePath.loadPathFromData(HiBinaryData::ProcessorEditorHeaderIcons::closeIcon, SIZE_OF_PATH(HiBinaryData::ProcessorEditorHeaderIcons::closeIcon));
	deleteButton->setShape(deletePath, true, true, true);
	deleteButton->setToggleState(true, dontSendNotification);
	refreshShapeButton(deleteButton);

	deleteButton->addListener(this);

	addAndMakeVisible(retriggerButton = new ShapeButton("Retrigger Envelope", drawColour, drawColour, drawColour));
	retriggerButton->setTooltip("Retrigger envelope in Legato Mode");
	retriggerButton->addListener(this);

	addAndMakeVisible(monophonicButton = new ShapeButton("Monophonic", drawColour, drawColour, drawColour));

	monophonicButton->setTooltip("Toggle between monophonic and polyphonic mode");
	monophonicButton->addListener(this);

	addAndMakeVisible(bipolarModButton = new ShapeButton("Bipolar Modulation", drawColour, drawColour, drawColour));
	
	Path bipolarPath;
	bipolarPath.loadPathFromData(HiBinaryData::ProcessorEditorHeaderIcons::bipolarIcon, SIZE_OF_PATH(HiBinaryData::ProcessorEditorHeaderIcons::bipolarIcon));
	bipolarModButton->setShape(bipolarPath, true, true, true);
	
	addAndMakeVisible (addButton = new ShapeButton ("Add new Processor", Colours::white, Colours::white, Colours::white));
	Path addPath;
	addPath.loadPathFromData(HiBinaryData::ProcessorEditorHeaderIcons::addIcon, SIZE_OF_PATH(HiBinaryData::ProcessorEditorHeaderIcons::addIcon));
	addButton->setShape(addPath, true, true, true);
	addButton->setToggleState(true, dontSendNotification);
	
	refreshShapeButton(addButton);

    addButton->addListener (this);

    addAndMakeVisible (intensitySlider = new IntensitySlider ("Intensity Slider"));
    intensitySlider->setRange (0, 1, 0.01);
    intensitySlider->setSliderStyle (Slider::LinearBar);
	intensitySlider->setTextBoxStyle (Slider::TextEntryBoxPosition::TextBoxLeft, true, 80, 20);
    intensitySlider->setColour (Slider::backgroundColourId, Colour (0xfb282828));
    intensitySlider->setColour (Slider::thumbColourId, Colour (0xff777777));
    intensitySlider->setColour (Slider::trackColourId, Colour (0xff222222));
    intensitySlider->setColour (Slider::textBoxTextColourId, Colours::white);
    intensitySlider->setColour (Slider::textBoxOutlineColourId, Colour (0x45ffffff));
	intensitySlider->setColour(Slider::textBoxHighlightColourId, Colours::white);
	intensitySlider->setColour(Slider::ColourIds::textBoxBackgroundColourId, Colour(0xfb282828));
	intensitySlider->setColour(Label::ColourIds::outlineWhenEditingColourId, Colours::white);
	
	intensitySlider->setVelocityModeParameters(0.1, 1, 0.0, true);	


	intensitySlider->setScrollWheelEnabled(false);
	intensitySlider->setLookAndFeel(&bpslaf);
    intensitySlider->addListener (this);
	intensitySlider->addMouseListener(this, true);
	

	addAndMakeVisible(balanceSlider = new Slider("Balance"));
	balanceSlider->setLookAndFeel(&bbluf);
	balanceSlider->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);

	balanceSlider->setRange(-1.0, 1.0, 0.01);
	balanceSlider->setDoubleClickReturnValue(true, 0.0);
	balanceSlider->setScrollWheelEnabled(false);

	balanceSlider->addListener(this);

	this->idLabel->setText(getProcessor()->getId(), dontSendNotification);
	this->typeLabel->setText(getProcessor()->getName(), dontSendNotification);
	
	
	
	//idLabel->setInterceptsMouseClicks(false, false);
	typeLabel->setInterceptsMouseClicks(false, false);

	if(isHeaderOfModulatorSynth())
	{
		addButton->setTooltip("Add a new child Processor.");
		balanceSlider->setTooltip("Change the Pan of the Synth.");
		deleteButton->setTooltip("Delete the synth.");
		bypassButton->setTooltip("Bypass the synth and all of its child processors.");

		deleteButton->setVisible(getEditor()->getIndentationLevel() != 0);

		intensitySlider->setRange(-100.0, 0.0, 0.1);
		intensitySlider->setSkewFactorFromMidPoint(-18.0);
		intensitySlider->setTextValueSuffix(" dB");
		intensitySlider->setTooltip("Change the volume of the synth.");

		bipolarModButton->setVisible(false);

		addButton->setVisible(isHeaderOfChain());

		if (dynamic_cast<GlobalModulatorContainer*>(getProcessor()) != nullptr)
		{
			intensitySlider->setVisible(false);
			valueMeter->setVisible(false);
			balanceSlider->setVisible(false);
		}

	}
	else if(isHeaderOfModulator())
	{
		addButton->setTooltip("Add a new child Modulator.");
		
		deleteButton->setTooltip("Delete the Modulator.");
		bypassButton->setTooltip("Bypass the Modulator.");
		bipolarModButton->setTooltip("Use bipolar Modulation (0...1) -> (-1...1)");

		bipolarModButton->setVisible(false);

        dynamic_cast<Modulation*>(getProcessor())->modeBroadcaster.addListener(*this, updateModulationMode, true);
        
		intensitySlider->setTooltip("Set the intensity of the modulation. 0 = no effect, 1 = full range modulation.");

		valueMeter->setType(VuMeter::MonoHorizontal);
		valueMeter->setColour(VuMeter::ledColour, Colours::grey);

		intensitySlider->setVisible(!isHeaderOfChain());
		addButton->setVisible(isHeaderOfChain());
		deleteButton->setVisible(!isHeaderOfChain());

	}

	else if(isHeaderOfMidiProcessor())
	{
		addButton->setTooltip("Add a new Midi Processor.");
		
		deleteButton->setTooltip("Delete the Midi Processor.");
		bypassButton->setTooltip("Bypass the Midi Processor.");

		bipolarModButton->setVisible(false);

		addButton->setVisible(isHeaderOfChain());
		deleteButton->setVisible(!isHeaderOfChain());
		intensitySlider->setVisible(false);
	}

	else if(isHeaderOfEffectProcessor())
	{
		addButton->setTooltip("Add a new Effect.");
		
		deleteButton->setTooltip("Delete the Effect.");
		bypassButton->setTooltip("Bypass the Effect.");

		bipolarModButton->setVisible(false);


		addButton->setVisible(isHeaderOfChain());
		deleteButton->setVisible(!isHeaderOfChain());
		intensitySlider->setVisible(false);

	}

	if (isHeaderOfChain() && !isHeaderOfModulatorSynth())
	{
		valueMeter->setOpaque(false);
		valueMeter->setColour(VuMeter::ColourId::backgroundColour, Colours::black.withAlpha(0.2f));
		valueMeter->setColour(VuMeter::ColourId::outlineColour, JUCE_LIVE_CONSTANT_OFF(Colour(0x50000000)));
		valueMeter->setColour(VuMeter::ColourId::ledColour, JUCE_LIVE_CONSTANT_OFF(Colour(0xb6ffffff)));
	}

	checkSoloLabel();

#if HISE_IOS

	setSize(ProcessorEditorContainer::getWidthForIntendationLevel(getEditor()->getIndentationLevel()), 40);

#else
    setSize(ProcessorEditorContainer::getWidthForIntendationLevel(getEditor()->getIndentationLevel()), 30);
#endif

	update(true);

	foldButton->setMouseClickGrabsKeyboardFocus(false);
	deleteButton->setMouseClickGrabsKeyboardFocus(false);
	addButton->setMouseClickGrabsKeyboardFocus(false);
	bypassButton->setMouseClickGrabsKeyboardFocus(false);

	foldButton->setWantsKeyboardFocus(false);
	deleteButton->setWantsKeyboardFocus(false);
	addButton->setWantsKeyboardFocus(false);
	bypassButton->setWantsKeyboardFocus(false);
}

ProcessorEditorHeader::~ProcessorEditorHeader()
{
	getProcessor()->getMainController()->removeScriptListener(this);

    valueMeter = nullptr;
    idLabel = nullptr;
    typeLabel = nullptr;
    bypassButton = nullptr;
    foldButton = nullptr;
    deleteButton = nullptr;
	retriggerButton = nullptr;
    addButton = nullptr;
    intensitySlider = nullptr;
}

void ProcessorEditorHeader::setLookAndFeel()
{
	if(dynamic_cast<ModulatorSynth*>(getProcessor()) != nullptr) luf = new ModulatorSynthEditorHeaderLookAndFeel();
	else
	{
		auto l = new ModulatorEditorHeaderLookAndFeel();
        luf = l;
		luf->isChain = isHeaderOfChain();
        l->c = getProcessor()->getColour();
	}
	
	repaint();
}

void ProcessorEditorHeader::displayFoldState(bool shouldBeFolded)
{ foldButton->setToggleState(shouldBeFolded, dontSendNotification); };

void ProcessorEditorHeader::updateModulationMode(ProcessorEditorHeader& h, int m_)
{
    auto m = (Modulation::Mode)m_;
    
    if(m == Modulation::GainMode)
    {
        h.bipolarModButton->setVisible(false);
        h.intensitySlider->setTextValueSuffix("");
        h.intensitySlider->setRange (0, 1, 0.01);
    }
    else if(m == Modulation::PitchMode)
    {
        h.intensitySlider->setRange(-12.0, 12.0, 0.01);
        h.intensitySlider->setTextValueSuffix(" st");
        h.intensitySlider->setTextBoxIsEditable(true);
        h.bipolarModButton->setVisible(!h.isHeaderOfChain());
        h.bipolarModButton->addListener(&h);
    }
    else if (m == Modulation::PanMode)
    {
        h.intensitySlider->setRange(-100.0, 100.0, 1);
        h.intensitySlider->setTextValueSuffix("%");
        h.intensitySlider->setTextBoxIsEditable(true);
        h.bipolarModButton->setVisible(!h.isHeaderOfChain());
        h.bipolarModButton->addListener(&h);
    }
    else if (m == Modulation::GlobalMode)
    {
        h.bipolarModButton->setVisible(!h.isHeaderOfChain());
        h.bipolarModButton->addListener(&h);
    }
    
    const double intensity = dynamic_cast<Modulation*>(h.getProcessor())->getDisplayIntensity();
    h.intensitySlider->setValue(intensity, dontSendNotification);
    
    h.resized();
}

bool ProcessorEditorHeader::isHeaderOfModulator() const		{ return dynamic_cast<const Modulator*>(getProcessor()) != nullptr;	};

bool ProcessorEditorHeader::isHeaderOfModulatorSynth() const {return dynamic_cast<const ModulatorSynth*>(getProcessor()) != nullptr;	};

bool ProcessorEditorHeader::isHeaderOfMidiProcessor() const	{ return dynamic_cast<const MidiProcessor*>(getProcessor()) != nullptr; };

bool ProcessorEditorHeader::isHeaderOfEffectProcessor() const		{ return dynamic_cast<const EffectProcessor*>(getProcessor()) != nullptr ||
																	         dynamic_cast<const EffectProcessorChain*>(getProcessor()) != nullptr;	};

bool ProcessorEditorHeader::hasWorkspaceButton() const 
{ 
	return dynamic_cast<const JavascriptProcessor*>(getProcessor()) != nullptr || dynamic_cast<const ModulatorSampler*>(getProcessor()) != nullptr;
}

int ProcessorEditorHeader::getModulatorMode() const			{ jassert(isHeaderOfModulator());
															  return dynamic_cast<const Modulation*>(getProcessor())->getMode(); };

bool ProcessorEditorHeader::isHeaderOfChain() const			{ return dynamic_cast<const Chain*>(getProcessor()) != nullptr; };

bool ProcessorEditorHeader::isHeaderOfEmptyChain() const
{
	if(isHeaderOfChain())
	{
		const Chain *c = getEditor()->getProcessorAsChain();
		jassert(c != nullptr);

		return c->getHandler()->getNumProcessors() == 0;
	};

	return false;
}

//==============================================================================
void ProcessorEditorHeader::paint (Graphics& g)
{
    ProcessorEditor *pEditor = getEditor()->getParentEditor();
    
    if(pEditor != nullptr)
    {
        g.setColour(pEditor->getProcessor()->getColour());
    }
    else
    {
        g.setColour(Colour(BACKEND_BG_COLOUR));
    }
    
    g.fillRect(0,0,getWidth(), 10);
    

    
	Colour c = getProcessor()->getColour();

	if (getEditor()->isSelectedForCopyAndPaste()) c = getEditor()->getProcessorAsChain() ? c.withMultipliedBrightness(1.05f) : c.withMultipliedBrightness(1.05f);

	if (dynamic_cast<ModulatorSynth*>(getProcessor()))
	{
		g.setColour(c);
	}
	else
	{
		g.setGradientFill(ColourGradient(c.withMultipliedBrightness(1.05f),
			0.0f, 30.0f,
			c.withMultipliedBrightness(0.95f),
			0.0f, jmax(30.0f, (float)getEditor()->getHeight()),
			false));

	}

    g.fillRect(0,25,getWidth(), 20);

	g.setGradientFill(ColourGradient(Colour(0x6e000000),
		0.0f, 27.0f,
		Colour(0x00000000),
		0.0f, 35.0f,
		false));
	g.fillRect(0, 30, getWidth(), 30);

	luf->drawBackground(g, (float)getWidth(), (float)getHeight()+5.f, getProcessor()->getEditorState(Processor::Folded));	


	if (storedInPreset)
	{
		Path p;
		static const unsigned char pShape[] = { 110,109,0,144,89,67,0,103,65,67,108,0,159,88,67,0,3,68,67,108,129,106,86,67,0,32,74,67,108,1,38,77,67,0,108,74,67,108,1,121,84,67,0,28,80,67,108,129,227,81,67,255,3,89,67,108,1,144,89,67,127,206,83,67,108,1,60,97,67,255,3,89,67,108,129,166,94,67,0,28,
		80,67,108,129,249,101,67,0,108,74,67,108,1,181,92,67,0,32,74,67,108,1,144,89,67,0,103,65,67,99,109,0,144,89,67,1,76,71,67,108,128,73,91,67,1,21,76,67,108,0,94,96,67,129,62,76,67,108,0,90,92,67,129,92,79,67,108,128,196,93,67,129,62,84,67,108,0,144,89,
		67,129,99,81,67,108,0,91,85,67,1,63,84,67,108,128,197,86,67,129,92,79,67,108,128,193,82,67,129,62,76,67,108,0,214,87,67,1,21,76,67,108,0,144,89,67,1,76,71,67,99,101,0,0 };

		p.loadPathFromData(pShape, sizeof(pShape));

		Rectangle<int> b = { foldButton->getRight() + 4, 0, getHeight(), getHeight() };

		PathFactory::scalePath(p, b.toFloat().reduced(3.0f));

		g.setColour(idLabel->findColour(Label::textColourId));
		g.fillPath(p);
	}

	if(isSoloHeader)
	{
		g.setColour(Colours::white.withAlpha(0.2f));

		g.setFont(GLOBAL_BOLD_FONT());

		g.drawText("from " + parentName, 0, 10, getWidth() - 6, getHeight(), Justification::topRight);
	}
    
}

void ProcessorEditorHeader::resized()
{

#if HISE_IOS

	const int addCloseWidth = 22;

	int yOffset = 6;
	const int yOffset2 = 7;

#else
	const int addCloseWidth = 16;

	int yOffset = 8;
	const int yOffset2 = 5;

#endif

	int x = 8;

	if (parentProcessor == nullptr)
	{
		parentProcessor = ProcessorHelpers::findParentProcessor(getProcessor(), false);
	}

	const bool isInEffectSlot = dynamic_cast<SlotFX*>(parentProcessor.get()) != nullptr;
    
	const bool isInternalChain = isHeaderOfChain() && !isHeaderOfModulatorSynth();

	if (isInternalChain)
	{

	}


	if (isHeaderOfChain())
	{
		addButton->setBounds(x, yOffset, addCloseWidth, addCloseWidth);

		x += addCloseWidth + 8;
	}

	if (getProcessor()->getMainController()->getMainSynthChain() != getProcessor())
	{
		foldButton->setBounds(x, yOffset, addCloseWidth, addCloseWidth);
		x += addCloseWidth + 8;

	}

	if (storedInPreset)
		x += getHeight() + 5;

	if (hasWorkspaceButton())
	{
		workspaceButton->setBounds(x, yOffset, addCloseWidth, addCloseWidth);
		x += addCloseWidth + 8;
	}
	else
	{
		workspaceButton->setVisible(false);
	}

	if(getProcessor()->getMainController()->getMainSynthChain() != getProcessor())
	{

        chainIcon->setBounds(x, yOffset-1, chainIcon->drawIcon() ? 16 : 0, 16);
        
        x = chainIcon->getRight() + 3;
    }

	if (isInternalChain)
	{
		idLabel->setBounds(x, yOffset, proportionOfWidth(0.16f), 18);

		x = idLabel->getRight() + 10;
	}
	else
	{
		if (!isHeaderOfModulator() && !isHeaderOfMidiProcessor() && !isHeaderOfEffectProcessor())
		{
            idLabel->setBounds(x, 0, proportionOfWidth(0.16f), 18);
			typeLabel->setBounds(x, 12, proportionOfWidth(0.16f), 18);
		}
        else
        {
            idLabel->setBounds(x, yOffset, proportionOfWidth(0.16f), 18);

        }
		
		x = idLabel->getRight();

		bypassButton->setBounds(x, yOffset2, 30, 20);

		x = bypassButton->getRight() + 10;
	}
	
	const bool showMonoButton = IS(EnvelopeModulator) && !isHeaderOfChain();

	if (showMonoButton)
	{
		monophonicButton->setBounds(x, yOffset, addCloseWidth, addCloseWidth);
		x += addCloseWidth + 8;
	}
	else
	{
		monophonicButton->setVisible(false);
	}

	const bool showRetriggerButton = monophonicButton->getToggleState() && showMonoButton;

	if (showRetriggerButton)
	{
		retriggerButton->setVisible(true);
		retriggerButton->setBounds(x, yOffset, addCloseWidth, addCloseWidth);
		x += addCloseWidth + 8;
	}
	else
	{
		retriggerButton->setVisible(false);
	}

	const bool showValueMeter = !isHeaderOfMidiProcessor();

	if (showValueMeter)
	{
		valueMeter->setBounds(x, yOffset2, getWidth() / 2 - x, 20);
		x = valueMeter->getRight() + 3;
	}

	bool shouldShowRoutingButton = !isInEffectSlot &&
								   dynamic_cast<RoutableProcessor*>(getProcessor()) &&
								   dynamic_cast<GlobalModulatorContainer*>(getProcessor()) == nullptr;

	if (shouldShowRoutingButton)
	{
		x = valueMeter->getRight() + 3;
		x += addCloseWidth + 5;
	}
	else
	{
        x += 3;
	}
	
	if (bipolarModButton->isVisible())
	{
		bipolarModButton->setBounds(x, 7, 18, 18);
		x = bipolarModButton->getRight() + 5;
	}

	intensitySlider->setBounds(x, yOffset2, 200, 20);

	x = intensitySlider->getRight() + 3;

	if (IS(JavascriptProcessor))
	{
		x += 40;
	}

	if (IS(TimeModulation))
	{
		x += 40;
	}

	

	
    
	if (isInEffectSlot)
	{
        deleteButton->setVisible(false);
	}
	else
	{
		deleteButton->setEnabled(getEditor()->getIndentationLevel() != 0);
		deleteButton->setBounds(getWidth() - 8 - addCloseWidth, yOffset, addCloseWidth, addCloseWidth);
	}
    
	
    
	if(isHeaderOfModulatorSynth())
	{
		balanceSlider->setBounds(x + 2, yOffset2, 28, 24);

		if (getEditor()->getIndentationLevel() == 0)
		{
			//addButton->setBounds(getWidth() - 8 - addCloseWidth, 8, addCloseWidth, addCloseWidth);
		}
		else
		{
			//addButton->setBounds(getWidth() - 3 * addCloseWidth, 8, addCloseWidth, addCloseWidth);
			deleteButton->setEnabled(true);
		}
		
	}

	repaint();
}

void ProcessorEditorHeader::sliderValueChanged (Slider* sliderThatWasMoved)
{
    if (sliderThatWasMoved == intensitySlider)
    {
        PresetHandler::setChanged(getProcessor());
        
		if(isHeaderOfModulator())
		{
			Modulation *mod = dynamic_cast<Modulation*>(getProcessor());
			mod->setIntensityFromSlider((float)intensitySlider->getValue());
		}
		else if(isHeaderOfModulatorSynth())
		{
			const float level = Decibels::decibelsToGain((float)intensitySlider->getValue());
			dynamic_cast<ModulatorSynth*>(getProcessor())->setGain(level);
		}
    }

	else if(sliderThatWasMoved == balanceSlider)
	{
        PresetHandler::setChanged(getProcessor());
        
		dynamic_cast<ModulatorSynth*>(getProcessor())->setBalance((float)balanceSlider->getValue());
	}
}

void ProcessorEditorHeader::sliderDragStarted(Slider* s)
{
	dragStartValue = s->getValue();
}

void ProcessorEditorHeader::sliderDragEnded(Slider* s)
{
	if (isHeaderOfModulatorSynth() && s == intensitySlider)
	{
		const float oldValue = Decibels::decibelsToGain((float)dragStartValue);
		const float newValue = Decibels::decibelsToGain((float)s->getValue());

		MacroControlledObject::UndoableControlEvent* newEvent = new MacroControlledObject::UndoableControlEvent(getProcessor(), ModulatorSynth::Parameters::Gain, oldValue, newValue);

		String undoName = getProcessor()->getId();
		undoName << " - " << "Volume" << ": " << String(s->getValue(), 2);

		getProcessor()->getMainController()->getControlUndoManager()->perform(newEvent);
	}
}

void ProcessorEditorHeader::buttonClicked (Button* buttonThatWasClicked)
{
	if (buttonThatWasClicked == workspaceButton)
	{
		auto rootWindow = GET_BACKEND_ROOT_WINDOW(this);

		if (auto jsp = dynamic_cast<JavascriptProcessor*>(getProcessor()))
		{
			BackendPanelHelpers::ScriptingWorkspace::setGlobalProcessor(rootWindow, jsp);
			BackendPanelHelpers::showWorkspace(rootWindow, BackendPanelHelpers::Workspace::ScriptingWorkspace, sendNotification);
			
		}
		else if (auto sampler = dynamic_cast<ModulatorSampler*>(getProcessor()))
		{
			BackendPanelHelpers::SamplerWorkspace::setGlobalProcessor(rootWindow, sampler);
			BackendPanelHelpers::showWorkspace(rootWindow, BackendPanelHelpers::Workspace::SamplerWorkspace, sendNotification);
		}

	}


    else if (buttonThatWasClicked == bypassButton->button)
    {
        
		bool shouldBeBypassed = bypassButton->getToggleState();
		getProcessor()->setBypassed(shouldBeBypassed, sendNotification);
		bypassButton->setToggleState(!shouldBeBypassed, dontSendNotification);
		
		ProcessorEditorPanel *panel = getEditor()->getPanel();

		for(int i = 0; i < panel->getNumChildEditors(); i++)
		{
			panel->getChildEditor(i)->setEnabled(!shouldBeBypassed);
		}
        
        PresetHandler::setChanged(getProcessor());
        
    }
	else if (buttonThatWasClicked == retriggerButton)
	{
		const bool newValue = !retriggerButton->getToggleState();

		getProcessor()->setAttribute(EnvelopeModulator::Parameters::Retrigger, newValue ? 1.0f : 0.f, sendNotification);
	}
	else if (buttonThatWasClicked == monophonicButton)
	{
		const bool newValue = !monophonicButton->getToggleState();

		getProcessor()->setAttribute(EnvelopeModulator::Parameters::Monophonic, newValue ? 1.0f : 0.f, sendNotification);

	}
    else if (buttonThatWasClicked == foldButton)
    {
		bool shouldBeFolded = toggleButton(buttonThatWasClicked);

		getEditor()->setFolded(shouldBeFolded, true);
		getEditor()->sendResizedMessage();

		checkFoldButton();
    }
	else if (buttonThatWasClicked == bipolarModButton)
	{
		bool shouldBeBipolar = toggleButton(bipolarModButton);

		dynamic_cast<Modulation*>(getProcessor())->setIsBipolar(shouldBeBipolar);
		updateBipolarIcon(shouldBeBipolar);
	}
    else if (buttonThatWasClicked == deleteButton)
	{
        ProcessorEditor::deleteProcessorFromUI(this, getProcessor());
	}
  
	else if (buttonThatWasClicked == addButton)
	{
        getEditor()->getProcessor()->setEditorState(Processor::Folded, false);
		createProcessorFromPopup();
	}
}

void ProcessorEditorHeader::updateBipolarIcon(bool shouldBeBipolar)
{
	bipolar = shouldBeBipolar;

	Path p;

	if (shouldBeBipolar)
		p.loadPathFromData(HiBinaryData::ProcessorEditorHeaderIcons::bipolarIcon, SIZE_OF_PATH(HiBinaryData::ProcessorEditorHeaderIcons::bipolarIcon));
	else
		p.loadPathFromData(HiBinaryData::ProcessorEditorHeaderIcons::unipolarIcon, SIZE_OF_PATH(HiBinaryData::ProcessorEditorHeaderIcons::unipolarIcon));

	bipolarModButton->setShape(p, false, true, true);
}

void ProcessorEditorHeader::updateRetriggerIcon(bool shouldRetrigger)
{
	retrigger = shouldRetrigger;
	retriggerButton->setToggleState(shouldRetrigger, dontSendNotification);
	
	Path p;

	if (!shouldRetrigger)
		p.loadPathFromData(HiBinaryData::ProcessorEditorHeaderIcons::retriggerOffPath, SIZE_OF_PATH(HiBinaryData::ProcessorEditorHeaderIcons::retriggerOffPath));
	else
		p.loadPathFromData(HiBinaryData::ProcessorEditorHeaderIcons::retriggerOnPath, SIZE_OF_PATH(HiBinaryData::ProcessorEditorHeaderIcons::retriggerOnPath));

	retriggerButton->setShape(p, false, true, true);
}

void ProcessorEditorHeader::updateMonoIcon(bool shouldBeMono)
{
	mono = shouldBeMono;

	monophonicButton->setToggleState(shouldBeMono, dontSendNotification);

	if (!monophonicButton->isVisible())
		return;

	Path p;

	if (!shouldBeMono)
	{
		p.loadPathFromData(HiBinaryData::ProcessorEditorHeaderIcons::polyphonicPath, SIZE_OF_PATH(HiBinaryData::ProcessorEditorHeaderIcons::polyphonicPath));		
	}
	else
	{
		p.loadPathFromData(HiBinaryData::ProcessorEditorHeaderIcons::monophonicPath, SIZE_OF_PATH(HiBinaryData::ProcessorEditorHeaderIcons::monophonicPath));
	}

	monophonicButton->setShape(p, false, true, true);

	resized();
}

void ProcessorEditorHeader::update(bool force)
{
	Processor *p = getProcessor();

	bypassButton->setToggleState(!p->isBypassed(), dontSendNotification);

	if(!idLabel->isBeingEdited() && force)
		idLabel->setText(p->getId(), dontSendNotification);

	if(isHeaderOfModulator())
	{
		Modulation *m = dynamic_cast<Modulation*>(p);
		const double intensity = m->getDisplayIntensity();
		intensitySlider->setValue(intensity, dontSendNotification);

		auto mod = dynamic_cast<Modulation*>(p);

		if(bipolar != mod->isBipolar() || force)
			updateBipolarIcon(m->isBipolar());

		if (isHeaderOfChain())
			return;

		if (dynamic_cast<EnvelopeModulator*>(mod) != nullptr)
		{
			auto shouldRetrigger = getProcessor()->getAttribute(EnvelopeModulator::Parameters::Retrigger) > 0.5f;
			auto shouldBeMonophonic = getProcessor()->getAttribute(EnvelopeModulator::Parameters::Monophonic) > 0.5f;

			if(mono != shouldBeMonophonic || force)
				updateMonoIcon(shouldBeMonophonic);

			if(shouldRetrigger != retrigger || force)
				updateRetriggerIcon(shouldRetrigger);
		}
	}
	else if(isHeaderOfModulatorSynth())
	{
		ModulatorSynth *s = dynamic_cast<ModulatorSynth*>(p);

		const double levelGain = Decibels::gainToDecibels(s->getGain());
		intensitySlider->setValue(levelGain, dontSendNotification);
		balanceSlider->setValue(s->getAttribute(ModulatorSynth::Balance), dontSendNotification);
	}
};

void ProcessorEditorHeader::setPlotButton(bool on)
{
	
};

void ProcessorEditorHeader::displayBypassedChain(bool isBypassed)
{
	bypassButton->setEnabled(isBypassed);
	valueMeter->setEnabled(isBypassed);
}

void ProcessorEditorHeader::enableChainHeader()
{
	if(! (isHeaderOfModulator() || isHeaderOfMidiProcessor() || isHeaderOfEffectProcessor() )) return;

	bool shouldBeEnabled = !isHeaderOfEmptyChain();

	if(!shouldBeEnabled)
	{
		getEditor()->setFolded(true, false);
	}
	else
	{
		// Not very nice, but works...
		const bool wasDisabled = !foldButton->isEnabled();

		if (wasDisabled)
		{
			getEditor()->setFolded(false, false);
		}
	}

	foldButton->setEnabled(shouldBeEnabled);
	bypassButton->setEnabled(shouldBeEnabled);

	checkFoldButton();
}


void ProcessorEditorHeader::checkSoloLabel()
{
	
};



void ProcessorEditorHeader::createProcessorFromPopup(Processor *insertBeforeSibling)
{
    ProcessorEditor::createProcessorFromPopup(getEditor(), getProcessor(), insertBeforeSibling);
};


void ProcessorEditorHeader::addProcessor(Processor *processorToBeAdded, Processor *insertBeforeSibling)
{
	

}

void ProcessorEditorHeader::timerCallback()
{
	if (getProcessor() != nullptr)
	{
		if (isHeaderOfModulator())
		{
			const float outputValue = getProcessor()->getOutputValue();

			Modulation* m = dynamic_cast<Modulation*>(getProcessor());

			if (m->getMode() == Modulation::PitchMode)
			{
				const float intensity = m->getIntensity();

				if (m->isBipolar())
				{
					const float value = 0.5f + (outputValue-0.5f) * intensity;
					valueMeter->setPeak(value, -1.0f);
				}
				else
				{
					const float value = 0.5f + 0.5f * (outputValue * intensity);
					valueMeter->setPeak(value, -1.0f);
				}
			}
			else
			{
				valueMeter->setPeak(outputValue, -1.0f);
			}

			
			
		}
		else
		{
			valueMeter->setPeak(getProcessor()->getDisplayValues().outL,
				getProcessor()->getDisplayValues().outR);
		}

		update(false);

		bypassButton->refresh();
		intensitySlider->setEnabled(!getProcessor()->isBypassed());
	}
};


void ProcessorEditorHeader::mouseDown(const MouseEvent &e)
{
#if HISE_IOS

	// Increase the hit area for the buttons

	const Rectangle<int> extendedAddButtonBounds = addButton->getBounds().expanded(10);
	const Rectangle<int> extendedFoldButtonBounds = foldButton->getBounds().expanded(10);
	const Rectangle<int> extendedCloseButtonBounds = deleteButton->getBounds().expanded(10);

	const Point<int> pos = e.getMouseDownPosition();

	if (addButton->isVisible() && extendedAddButtonBounds.contains(pos))
	{
		addButton->triggerClick();
	}
	else if (foldButton->isVisible() && extendedFoldButtonBounds.contains(pos))
	{
		foldButton->triggerClick();
	}
	else if (deleteButton->isVisible() && extendedCloseButtonBounds.contains(pos))
	{
		deleteButton->triggerClick();
	}
		
#endif

	if(e.mods.isRightButtonDown())
	{
		ProcessorEditor::showContextMenu(this, getProcessor());
	}
};


void ProcessorEditorHeader::mouseDoubleClick(const MouseEvent& e)
{
	if (auto pe = findParentComponentOfClass<ProcessorEditorContainer>())
	{
		auto p = getProcessor();
		MessageManager::callAsync([pe, p]()
		{
			pe->setRootProcessorEditor(p);
		});
		
		return;
	}
}

void ProcessorEditorHeader::setupButton(DrawableButton *b, ButtonShapes::Symbol s)
{
	ScopedPointer<Drawable> onPath = ButtonShapes::createSymbol(s, true, true);
	ScopedPointer<Drawable> onPathDisabled = ButtonShapes::createSymbol(s, true, false);

	ScopedPointer<Drawable> offPath = ButtonShapes::createSymbol(s, false, true);
	ScopedPointer<Drawable> offPathDisabled = ButtonShapes::createSymbol(s, false, false);

	b->setImages(offPath,
		offPath,
		offPath,
		offPathDisabled,
		onPath,
		onPath,
		onPath,
		onPathDisabled
		);

	b->setEdgeIndent(1);

	b->setColour(TextButton::buttonColourId, Colour(0xaa222222));
	b->setColour(TextButton::buttonOnColourId, Colour(0xff888888));
}

void ProcessorEditorHeader::refreshShapeButton(ShapeButton *b)
{
	Colour buttonColour = Colours::white;
	
	if(isHeaderOfChain() || isHeaderOfModulatorSynth())
		buttonColour = Colours::black.withAlpha(0.8f);
	
	buttonColour = buttonColour.withAlpha(b->isEnabled() ? 1.0f : 0.2f);

	DropShadowEffect *shadow = dynamic_cast<DropShadowEffect*>(b->getComponentEffect());

	if (shadow != nullptr)
	{
		jassert(shadow != nullptr);

		shadow->setShadowProperties(DropShadow(buttonColour.contrasting(1.0f).withAlpha(0.5f), 3, Point<int>()));
	}

	b->setColours(buttonColour.withMultipliedAlpha(0.5f), buttonColour.withMultipliedAlpha(1.0f), buttonColour.withMultipliedAlpha(1.0f));
	b->repaint();
}

void ProcessorEditorHeader::labelTextChanged(Label *l)
{
	if (l == idLabel)
	{
		getEditor()->getProcessor()->setId(l->getText(), sendNotification);
        
		auto root = GET_BACKEND_ROOT_WINDOW(this);

		if(auto keyboard = root->getKeyboard())
			keyboard->grabKeyboardFocus();

		PresetHandler::setChanged(getProcessor());
	}
}


void ProcessorEditorHeader::scriptWasCompiled(JavascriptProcessor *processor)
{
	if (auto jmp = dynamic_cast<JavascriptMidiProcessor*>(processor))
	{
		if (jmp->isFront())
		{
			storedInPreset = false;
			auto id = getProcessor()->getId();

			for (auto ms : getProcessor()->getMainController()->getModuleStateManager())
			{
				if (id == ms->id)
				{
					storedInPreset = true;
					break;
				}
			}
			
			resized();
		}
	}
}

void ProcessorEditorHeader::checkFoldButton()
{
	foldButton->setToggleState(!getProcessor()->getEditorState(Processor::Folded), dontSendNotification);

	const bool on = foldButton->getToggleState();

	Path foldPath;
		
	foldPath.loadPathFromData(HiBinaryData::ProcessorEditorHeaderIcons::foldedIcon, SIZE_OF_PATH(HiBinaryData::ProcessorEditorHeaderIcons::foldedIcon));
	if(on) foldPath.applyTransform(AffineTransform::rotation(float_Pi * 0.5f));

	foldButton->setShape(foldPath, false, true, true);

	refreshShapeButton(foldButton);
}

#undef IS

} // namespace hise
