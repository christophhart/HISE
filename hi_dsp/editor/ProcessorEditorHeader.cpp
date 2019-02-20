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

	setOpaque(true);

    addAndMakeVisible (valueMeter = new VuMeter());
	valueMeter->setType(VuMeter::StereoHorizontal);
	valueMeter->setColour (VuMeter::backgroundColour, Colour (0xFF333333));
	valueMeter->setColour (VuMeter::ledColour, Colours::lightgrey);
	valueMeter->setColour (VuMeter::outlineColour, isHeaderOfModulatorSynth() ? Colour (0x45000000) : Colour (0x45ffffff));
	
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
    idLabel->setColour (Label::textColourId, Colours::black.withAlpha(0.7f));
    idLabel->setColour (Label::outlineColourId, Colour (0x00ffffff));
    idLabel->setColour (Label::textWhenEditingColourId, Colours::black);
    idLabel->setColour (Label::ColourIds::backgroundWhenEditingColourId, Colours::transparentBlack);
	idLabel->setColour(TextEditor::ColourIds::highlightedTextColourId, idLabel->findColour(Label::textWhenEditingColourId));
	idLabel->setColour(TextEditor::ColourIds::highlightColourId, Colour(SIGNAL_COLOUR));
    idLabel->setColour (TextEditor::textColourId, Colours::black);
    idLabel->setColour (TextEditor::backgroundColourId, Colour (0x00000000));
    idLabel->setColour(TextEditor::ColourIds::focusedOutlineColourId, Colours::transparentBlack);
    idLabel->setColour(CaretComponent::ColourIds::caretColourId, isHeaderOfModulatorSynth() ? Colours::black : Colours::white);
    
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
    typeLabel->setColour (Label::textColourId, Colour (0xff220000));
    typeLabel->setColour (Label::outlineColourId, Colour (0x00000000));
    
    typeLabel->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    typeLabel->setBufferedToImage(true);
    
    addAndMakeVisible (debugButton = new TextButton ("Debug Button"));

	debugButton->setButtonText("DBG");

    debugButton->setTooltip (TRANS("Enable debug output to console"));
	
	debugButton->setColour (TextButton::buttonColourId, Colour (0xaa222222));
	debugButton->setColour (TextButton::buttonOnColourId, Colour (0xff680000));
	debugButton->setColour (TextButton::buttonOnColourId, Colour (0xff555555));
	debugButton->setColour (TextButton::textColourOnId, Colour (0xffffffff));
	debugButton->setColour (TextButton::textColourOffId, Colour (0x77ffffff));

	
    debugButton->addListener (this);

	addAndMakeVisible (plotButton = new DrawableButton ("Plotter Button", DrawableButton::ImageOnButtonBackground));
	setupButton(plotButton, ButtonShapes::Plot);
	plotButton->addListener(this);
	plotButton->setTooltip("Open a plot window to display the modulator value.");
    
	Colour buttonColour = isHeaderOfModulatorSynth() ? Colours::black : Colours::white;

	addAndMakeVisible(bypassButton = new HeaderButton("Bypass Button", HiBinaryData::ProcessorEditorHeaderIcons::bypassShape, sizeof(HiBinaryData::ProcessorEditorHeaderIcons::bypassShape), this));

	
	addAndMakeVisible (foldButton = new ShapeButton ("Fold", Colours::white, Colours::white, Colours::white));
	
	
	checkFoldButton();

    foldButton->setTooltip (TRANS("Expand this Processor"));
    
    foldButton->addListener (this);

	addAndMakeVisible(workspaceButton = new ShapeButton("Workspace", Colours::white, Colours::white, Colours::white));
	Path workspacePath = ColumnIcons::getPath(ColumnIcons::openWorkspaceIcon, sizeof(ColumnIcons::openWorkspaceIcon));
	workspaceButton->setShape(workspacePath, true, true, true);
	workspaceButton->addListener(this);
	workspaceButton->setToggleState(true, dontSendNotification);
	workspaceButton->setTooltip("Open " + getProcessor()->getId() + " in workspace");
	refreshShapeButton(workspaceButton);

    
    addAndMakeVisible (deleteButton = new ShapeButton ("Delete Processor", Colours::white, Colours::white, Colours::white));
	Path deletePath;
	deletePath.loadPathFromData(HiBinaryData::ProcessorEditorHeaderIcons::closeIcon, sizeof(HiBinaryData::ProcessorEditorHeaderIcons::closeIcon));
	deleteButton->setShape(deletePath, true, true, true);
	deleteButton->setToggleState(true, dontSendNotification);
	refreshShapeButton(deleteButton);

	deleteButton->addListener(this);

	addAndMakeVisible(retriggerButton = new ShapeButton("Retrigger Envelope", Colours::white, Colours::white, Colours::white));
	retriggerButton->setTooltip("Retrigger envelope in Legato Mode");
	retriggerButton->addListener(this);

	addAndMakeVisible(monophonicButton = new ShapeButton("Monophonic", Colours::white, Colours::white, Colours::white));

	monophonicButton->setTooltip("Toggle between monophonic and polyphonic mode");
	monophonicButton->addListener(this);

	addAndMakeVisible(routeButton = new ShapeButton("Edit Routing Matrix", Colours::white, Colours::white, Colours::white));
	Path routePath;
	routePath.loadPathFromData(HiBinaryData::SpecialSymbols::routingIcon, sizeof(HiBinaryData::SpecialSymbols::routingIcon));
	routeButton->setShape(routePath, true, true, true);
	routeButton->setToggleState(true, dontSendNotification);
	refreshShapeButton(routeButton);

	routeButton->addListener(this);
	routeButton->setVisible(dynamic_cast<RoutableProcessor*>(getProcessor()) != nullptr);


	addAndMakeVisible(bipolarModButton = new ShapeButton("Bipolar Modulation", Colours::white, Colours::white, Colours::white));
	
	Path bipolarPath;
	bipolarPath.loadPathFromData(HiBinaryData::ProcessorEditorHeaderIcons::bipolarIcon, sizeof(HiBinaryData::ProcessorEditorHeaderIcons::bipolarIcon));
	bipolarModButton->setShape(bipolarPath, true, true, true);
	
	addAndMakeVisible (addButton = new ShapeButton ("Add new Processor", Colours::white, Colours::white, Colours::white));
	Path addPath;
	addPath.loadPathFromData(HiBinaryData::ProcessorEditorHeaderIcons::addIcon, sizeof(HiBinaryData::ProcessorEditorHeaderIcons::addIcon));
	addButton->setShape(addPath, true, true, true);
	addButton->setToggleState(true, dontSendNotification);
	
	refreshShapeButton(addButton);

    addButton->addListener (this);

    addAndMakeVisible (intensitySlider = new Slider ("Intensity Slider"));
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

		debugButton->setVisible(false);
		plotButton->setVisible(false);
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

		bipolarModButton->setVisible(false);

		if(getModulatorMode() == Modulation::PitchMode)
		{
			intensitySlider->setRange(-12.0, 12.0, 0.01);
			intensitySlider->setTextValueSuffix(" st");
			intensitySlider->setTextBoxIsEditable(true);
			bipolarModButton->setVisible(!isHeaderOfChain());
			bipolarModButton->addListener(this);
		}
		else if (getModulatorMode() == Modulation::PanMode)
		{
			intensitySlider->setRange(-100.0, 100.0, 1);
			intensitySlider->setTextValueSuffix("%");
			intensitySlider->setTextBoxIsEditable(true);
			bipolarModButton->setVisible(!isHeaderOfChain());
			bipolarModButton->addListener(this);
		}

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

		typeLabel->setColour (Label::textColourId, Colour (0xffFFFFFF));

		plotButton->setVisible(false);
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

		typeLabel->setColour (Label::textColourId, Colour (0xffFFFFFF));
		bipolarModButton->setVisible(false);

		plotButton->setVisible(false);
		debugButton->setVisible(ProcessorHelpers::is<JavascriptProcessor>(getProcessor()));

		addButton->setVisible(isHeaderOfChain());
		deleteButton->setVisible(!isHeaderOfChain());
		intensitySlider->setVisible(false);
	}

	checkSoloLabel();

#if HISE_IOS

	setSize(ProcessorEditorContainer::getWidthForIntendationLevel(getEditor()->getIndentationLevel()), 40);

#else
    setSize(ProcessorEditorContainer::getWidthForIntendationLevel(getEditor()->getIndentationLevel()), 30);
#endif

	update();

	debugButton->setMouseClickGrabsKeyboardFocus(false);
	foldButton->setMouseClickGrabsKeyboardFocus(false);
	deleteButton->setMouseClickGrabsKeyboardFocus(false);
	addButton->setMouseClickGrabsKeyboardFocus(false);
	bypassButton->setMouseClickGrabsKeyboardFocus(false);
	routeButton->setMouseClickGrabsKeyboardFocus(false);

	debugButton->setWantsKeyboardFocus(false);
	foldButton->setWantsKeyboardFocus(false);
	deleteButton->setWantsKeyboardFocus(false);
	addButton->setWantsKeyboardFocus(false);
	bypassButton->setWantsKeyboardFocus(false);
	routeButton->setWantsKeyboardFocus(false);
	plotButton->setWantsKeyboardFocus(false);




}

ProcessorEditorHeader::~ProcessorEditorHeader()
{
    valueMeter = nullptr;
    idLabel = nullptr;
    typeLabel = nullptr;
    debugButton = nullptr;
    plotButton = nullptr;
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
		luf = new ModulatorEditorHeaderLookAndFeel();
		luf->isChain = isHeaderOfChain();
	}
	
	repaint();
};

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

		routeButton->setBounds(x, yOffset, addCloseWidth, addCloseWidth);
		
		x = routeButton->getRight() + 5;
        
	}
	else
	{
        routeButton->setVisible(false);
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
		debugButton->setBounds(x, yOffset2, 30, 20);
		debugButton->setVisible(true);
		x += 40;
	}

	if (IS(TimeModulation))
	{
		plotButton->setBounds(x, yOffset2, 30, 20);
		plotButton->setVisible(true);

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
	else
	{
		//addButton->setBounds (getWidth() - 8 - addCloseWidth, 8, addCloseWidth, addCloseWidth);
	}

	if(isHeaderOfModulator())
	{
		plotButton->setEnabled(( dynamic_cast<TimeVariantModulator*>(getProcessor()) != nullptr ||
									dynamic_cast<EnvelopeModulator*>(getProcessor()) != nullptr )  &&
								!isHeaderOfEmptyChain());
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

		getProcessor()->getMainController()->getControlUndoManager()->beginNewTransaction(undoName);
		getProcessor()->getMainController()->getControlUndoManager()->perform(newEvent);
	}
}

void ProcessorEditorHeader::buttonClicked (Button* buttonThatWasClicked)
{
    if (buttonThatWasClicked == debugButton)
    {
        const bool value = buttonThatWasClicked->getToggleState();
		

		if(dynamic_cast<JavascriptProcessor*>(getProcessor()) != nullptr)
		{
			JavascriptProcessor *sp = value ? nullptr : dynamic_cast<JavascriptProcessor*>(getProcessor());

			getProcessor()->getMainController()->setWatchedScriptProcessor(sp, getEditor()->getBody());

			auto root = GET_ROOT_FLOATING_TILE(this);

			auto p = BackendPanelHelpers::toggleVisibilityForRightColumnPanel<ScriptWatchTablePanel>(root, !buttonThatWasClicked->getToggleState());

			if (p != nullptr)
			{
				p->setContentWithUndo(getProcessor(), 0);
			}
		}


		buttonThatWasClicked->setToggleState(!value, dontSendNotification);
    }
	else if (buttonThatWasClicked == workspaceButton)
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


    else if (buttonThatWasClicked == plotButton)
    {
        const bool value = buttonThatWasClicked->getToggleState();
		setPlotButton(!value);

		auto root = GET_ROOT_FLOATING_TILE(this);

		auto p = BackendPanelHelpers::toggleVisibilityForRightColumnPanel<PlotterPanel>(root,  buttonThatWasClicked->getToggleState());

		if (p != nullptr)
		{
			p->setContentWithUndo(getProcessor(), 0);
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
	else if (buttonThatWasClicked == routeButton)
	{
		dynamic_cast<RoutableProcessor*>(getProcessor())->editRouting(routeButton);
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
        if(dynamic_cast<BackendProcessorEditor*>(getEditor()->getParentComponent()) != nullptr)
		{
			dynamic_cast<BackendProcessorEditor*>(getEditor()->getParentComponent())->clearPopup();
			return;
		}

		if(dynamic_cast<ModulatorSynth*>(getProcessor()) == nullptr || PresetHandler::showYesNoWindow("Delete " + getProcessor()->getId() + "?", "Do you want to delete the Synth module?"))
		{
			auto p = getProcessor();

			auto f = [](Processor* p)
			{
				if (auto c = dynamic_cast<Chain*>(p->getParentProcessor(false)))
				{
					c->getHandler()->remove(p, false);
					jassert(!p->isOnAir());
				}
				
				return SafeFunctionCall::OK;
			};

			p->getMainController()->getGlobalAsyncModuleHandler().removeAsync(p, f);
		}
	}
  
	else if (buttonThatWasClicked == addButton)
	{
        getEditor()->getProcessor()->setEditorState(Processor::Folded, false);
		createProcessorFromPopup();
	}
}

void ProcessorEditorHeader::updateBipolarIcon(bool shouldBeBipolar)
{
	Path p;

	if (shouldBeBipolar)
		p.loadPathFromData(HiBinaryData::ProcessorEditorHeaderIcons::bipolarIcon, sizeof(HiBinaryData::ProcessorEditorHeaderIcons::bipolarIcon));
	else
		p.loadPathFromData(HiBinaryData::ProcessorEditorHeaderIcons::unipolarIcon, sizeof(HiBinaryData::ProcessorEditorHeaderIcons::unipolarIcon));

	bipolarModButton->setShape(p, false, true, true);
}

void ProcessorEditorHeader::updateRetriggerIcon(bool shouldRetrigger)
{
	retriggerButton->setToggleState(shouldRetrigger, dontSendNotification);
	
	Path p;

	if (!shouldRetrigger)
		p.loadPathFromData(HiBinaryData::ProcessorEditorHeaderIcons::retriggerOffPath, sizeof(HiBinaryData::ProcessorEditorHeaderIcons::retriggerOffPath));
	else
		p.loadPathFromData(HiBinaryData::ProcessorEditorHeaderIcons::retriggerOnPath, sizeof(HiBinaryData::ProcessorEditorHeaderIcons::retriggerOnPath));	

	retriggerButton->setShape(p, false, true, true);
}

void ProcessorEditorHeader::updateMonoIcon(bool shouldBeMono)
{
	monophonicButton->setToggleState(shouldBeMono, dontSendNotification);

	if (!monophonicButton->isVisible())
		return;

	Path p;

	if (!shouldBeMono)
	{
		p.loadPathFromData(HiBinaryData::ProcessorEditorHeaderIcons::polyphonicPath, sizeof(HiBinaryData::ProcessorEditorHeaderIcons::polyphonicPath));		
	}
	else
	{
		p.loadPathFromData(HiBinaryData::ProcessorEditorHeaderIcons::monophonicPath, sizeof(HiBinaryData::ProcessorEditorHeaderIcons::monophonicPath));	
	}

	monophonicButton->setShape(p, false, true, true);

	resized();
}

void ProcessorEditorHeader::update()
{
	Processor *p = getProcessor();

	bypassButton->setToggleState(! p->isBypassed(), dontSendNotification);

	if(!idLabel->isBeingEdited()) idLabel->setText(p->getId(), dontSendNotification);

	if(isHeaderOfModulator())
	{
		Modulation *m = dynamic_cast<Modulation*>(p);
		const double intensity = m->getDisplayIntensity();
		intensitySlider->setValue(intensity, dontSendNotification);

		auto mod = dynamic_cast<Modulation*>(p);

		plotButton->setToggleState(mod->isPlotted(), dontSendNotification);

		updateBipolarIcon(m->isBipolar());

		if (isHeaderOfChain())
			return;

		if (dynamic_cast<EnvelopeModulator*>(mod) != nullptr)
		{
			auto retrigger = getProcessor()->getAttribute(EnvelopeModulator::Parameters::Retrigger) > 0.5f;
			auto monophonic = getProcessor()->getAttribute(EnvelopeModulator::Parameters::Monophonic) > 0.5f;

			updateMonoIcon(monophonic);
			updateRetriggerIcon(retrigger);
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
	plotButton->setToggleState(on, dontSendNotification);
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

	

	idLabel->setColour (Label::textColourId, Colour (0x66ffffff).withAlpha(shouldBeEnabled ? 0.7f : 0.4f));
	idLabel->setColour (Label::ColourIds::textWhenEditingColourId, Colours::white);
	typeLabel->setColour (Label::textColourId, Colour (0x66ffffff).withAlpha(shouldBeEnabled ? 0.7f : 0.4f));

	foldButton->setEnabled(shouldBeEnabled);
	debugButton->setEnabled(shouldBeEnabled);
	plotButton->setEnabled(shouldBeEnabled);
	bypassButton->setEnabled(shouldBeEnabled);

	checkFoldButton();
}


void ProcessorEditorHeader::checkSoloLabel()
{
	const bool isInMasterPanel = getEditor()->getIndentationLevel() == 0;
	const bool isRootProcessor = getProcessor()->getMainController()->getMainSynthChain()->getRootProcessor() == getProcessor();

	if(isInMasterPanel && !isRootProcessor)
	{
		isSoloHeader = true;
		parentName = ProcessorHelpers::findParentProcessor(getProcessor(), true)->getId();
		deleteButton->setVisible(false);
			
		repaint();
	}
};

void ProcessorEditorHeader::createProcessorFromPopup(Processor *insertBeforeSibling)
{
	Processor *processorToBeAdded = nullptr;
	
	Chain *c = getEditor()->getProcessorAsChain();
	
	jassert(c != nullptr);
	FactoryType *t = c->getFactoryType();
	StringArray types;
	bool clipBoard = false;
	int result;

	// =================================================================================================================
	// Create the Popup

	{
		ScopedPointer<PopupLookAndFeel> l = new PopupLookAndFeel();
		PopupMenu m;
		
		m.setLookAndFeel(l);

		m.addSectionHeader("Create new Processor ");
		
		t->fillPopupMenu(m);

		m.addSeparator();
		m.addSectionHeader("Add from Clipboard");

		String clipBoardName = PresetHandler::getProcessorNameFromClipboard(t);

		if(clipBoardName != String())  m.addItem(CLIPBOARD_ITEM_MENU_INDEX, "Add " + clipBoardName + " from Clipboard");
		else								m.addItem(-1, "No compatible Processor in clipboard.", false);

		clipBoard = clipBoardName != String();

		result = m.show();
	}

	// =================================================================================================================
	// Create the processor

	if(result == 0)									return;

	else if(result == CLIPBOARD_ITEM_MENU_INDEX && clipBoard) processorToBeAdded = PresetHandler::createProcessorFromClipBoard(getProcessor());

	else
	{
		Identifier type = t->getTypeNameFromPopupMenuResult(result);
		String typeName = t->getNameFromPopupMenuResult(result);

		String name;

		if (isHeaderOfModulatorSynth()) name = typeName; // PresetHandler::getCustomName(typeName);
		else						  name = typeName;


		if (name.isNotEmpty())
		{
			processorToBeAdded = MainController::createProcessor(t, type, name);

			
		}
			else return;
	}

	// =================================================================================================================
	// Add the Editor

	addProcessor(processorToBeAdded, insertBeforeSibling);
};


void ProcessorEditorHeader::addProcessor(Processor *processorToBeAdded, Processor *insertBeforeSibling)
{
	if (processorToBeAdded == nullptr)	{ jassertfalse;	return; }

	auto editor = getEditor();

	auto f = [editor, insertBeforeSibling](Processor* p)
	{
		if (ProcessorHelpers::is<ModulatorSynth>(p) && dynamic_cast<ModulatorSynthGroup*>(editor->getProcessor()) == nullptr)
			dynamic_cast<ModulatorSynth*>(p)->addProcessorsWhenEmpty();

		editor->getProcessorAsChain()->getHandler()->add(p, insertBeforeSibling);

		PresetHandler::setUniqueIdsForProcessor(p);

		if (ProcessorHelpers::is<ModulatorSynth>(editor->getProcessor()))
			p->getMainController()->getMainSynthChain()->compileAllScripts();

		auto update = [](Dispatchable* obj)
		{
			auto editor = static_cast<ProcessorEditor*>(obj);

			editor->changeListenerCallback(editor->getProcessor());
			editor->childEditorAmountChanged();

			BACKEND_ONLY(GET_BACKEND_ROOT_WINDOW(editor)->sendRootContainerRebuildMessage(false));
			PresetHandler::setChanged(editor->getProcessor());

			return Dispatchable::Status::OK;
		};

		p->getMainController()->getLockFreeDispatcher().callOnMessageThreadAfterSuspension(editor, update);

		return SafeFunctionCall::OK;
	};

	editor->getProcessor()->getMainController()->getKillStateHandler().killVoicesAndCall(processorToBeAdded, f, MainController::KillStateHandler::SampleLoadingThread);
	
	return;

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

		enum
		{
			Copy=2,
			InsertBefore,
			CloseAllChains,
			CheckForDuplicate,
			CreateGenericScriptReference,
			CreateTableProcessorScriptReference,
			CreateAudioSampleProcessorScriptReference,
			CreateSamplerScriptReference,
			CreateMidiPlayerScriptReference,
			CreateSliderPackProcessorReference,
			CreateSlotFXReference,
			CreateRoutingMatrixReference,
			ReplaceWithClipboardContent,
			SaveAllSamplesToGlobalFolder,
			OpenAllScriptsInPopup,
			OpenInterfaceInPopup,
			ConnectToScriptFile,
			ReloadFromExternalScript,
			DisconnectFromScriptFile,
			SaveCurrentInterfaceState,
			numMenuItems
		};

		PopupMenu m;
		m.setLookAndFeel(&plaf);

		const bool isMainSynthChain = getProcessor()->getMainController()->getMainSynthChain() == getProcessor();

		m.addSectionHeader("Copy Tools");

		m.addItem(Copy, "Copy " + getProcessor()->getId() + " to Clipboard");

		if ((!isHeaderOfChain() || isHeaderOfModulatorSynth()) && !isMainSynthChain)
		{
			m.addItem(InsertBefore, "Add Processor before this module", true);
		}

		
		
		
		m.addSeparator();
		m.addSectionHeader("Misc Tools");


		if(isHeaderOfModulatorSynth())
		{
			m.addItem(CloseAllChains, "Close All Chains");
		}

		m.addSeparator();

		m.addItem(CreateGenericScriptReference, "Create generic script reference");
		
		if (dynamic_cast<ModulatorSampler*>(getProcessor()) != nullptr)
			m.addItem(CreateSamplerScriptReference, "Create typed Sampler script reference");
		else if (dynamic_cast<MidiFilePlayer*>(getProcessor()) != nullptr)
			m.addItem(CreateMidiPlayerScriptReference, "Create typed MIDI Player script reference");
		else if (dynamic_cast<SlotFX*>(getProcessor()) != nullptr)
			m.addItem(CreateSlotFXReference, "Create typed SlotFX script reference");

		m.addItem(CreateTableProcessorScriptReference, "Create typed Table script reference", dynamic_cast<LookupTableProcessor*>(getProcessor()) != nullptr);
		m.addItem(CreateAudioSampleProcessorScriptReference, "Create typed Audio sample script reference", dynamic_cast<AudioSampleProcessor*>(getProcessor()) != nullptr);
		//m.addItem(CreateSliderPackProcessorReference, "Create typed Slider Pack script reference", dynamic_cast<SliderPackProcessor*>(getProcessor()) != nullptr);
		m.addItem(CreateRoutingMatrixReference, "Create typed Routing matrix script reference", dynamic_cast<RoutableProcessor*>(getProcessor()) != nullptr);

		m.addSeparator();

		if (isMainSynthChain)
		{
			m.addSeparator();
			m.addSectionHeader("Root Container Tools");
			m.addItem(CheckForDuplicate, "Check children for duplicate IDs");
		}

		else if (JavascriptProcessor *sp = dynamic_cast<JavascriptProcessor*>(getProcessor()))
		{
			m.addSeparator();
			m.addSectionHeader("Script Processor Tools");
			m.addItem(OpenInterfaceInPopup, "Open Interface in Popup Window", true, false);
			m.addItem(ConnectToScriptFile, "Connect to external script", true, sp->isConnectedToExternalFile());
			m.addItem(ReloadFromExternalScript, "Reload external script", sp->isConnectedToExternalFile(), false);
			m.addItem(DisconnectFromScriptFile, "Disconnect from external script", sp->isConnectedToExternalFile(), false);
		}

		int result = m.show();

		if(result == 0) return;

		if (result == Copy) PresetHandler::copyProcessorToClipboard(getProcessor());

		else if (result == InsertBefore)
		{
			ProcessorEditor *pEditor = getEditor()->getParentEditor();

			ProcessorEditorHeader *parentHeader = pEditor->getHeader();

			parentHeader->createProcessorFromPopup(getProcessor());
		}

		else if (result == CloseAllChains)
		{
			getEditor()->getChainBar()->closeAll();
		}
		else if (result == CheckForDuplicate)
		{
			PresetHandler::checkProcessorIdsForDuplicates(getEditor()->getProcessor(), false);

		}
		else if (result == CreateGenericScriptReference)
			ProcessorHelpers::getScriptVariableDeclaration(getEditor()->getProcessor());
		else if (result == CreateAudioSampleProcessorScriptReference)
			ProcessorHelpers::getTypedScriptVariableDeclaration(getEditor()->getProcessor(), "AudioSampleProcessor");
		else if (result == CreateMidiPlayerScriptReference)
			ProcessorHelpers::getTypedScriptVariableDeclaration(getEditor()->getProcessor(), "MidiPlayer");
		else if (result == CreateRoutingMatrixReference)
			ProcessorHelpers::getTypedScriptVariableDeclaration(getEditor()->getProcessor(), "RoutingMatrix");
		else if (result == CreateSamplerScriptReference)
			ProcessorHelpers::getTypedScriptVariableDeclaration(getEditor()->getProcessor(), "Sampler");
		else if (result == CreateSlotFXReference)
			ProcessorHelpers::getTypedScriptVariableDeclaration(getEditor()->getProcessor(), "SlotFX");
		else if (result == CreateTableProcessorScriptReference)
			ProcessorHelpers::getTypedScriptVariableDeclaration(getEditor()->getProcessor(), "TableProcessor");
		else if (result == CreateSliderPackProcessorReference)
			ProcessorHelpers::getTypedScriptVariableDeclaration(getEditor()->getProcessor(), "SliderPackProcessor");
		else if (result == OpenInterfaceInPopup)
		{
			dynamic_cast<ScriptingEditor*>(getEditor()->getBody())->openContentInPopup();
		}
		else if (result == ConnectToScriptFile)
		{
			FileChooser fc("Select external script", GET_PROJECT_HANDLER(getProcessor()).getSubDirectory(ProjectHandler::SubDirectories::Scripts));

			if (fc.browseForFileToOpen())
			{
				File scriptFile = fc.getResult();

				const String scriptReference = GET_PROJECT_HANDLER(getProcessor()).getFileReference(scriptFile.getFullPathName(), ProjectHandler::SubDirectories::Scripts);

				dynamic_cast<JavascriptProcessor*>(getProcessor())->setConnectedFile(scriptReference);
			}
		}
		else if (result == ReloadFromExternalScript)
		{
			dynamic_cast<JavascriptProcessor*>(getProcessor())->reloadFromFile();
		}
		else if (result == DisconnectFromScriptFile)
		{
			if (PresetHandler::showYesNoWindow("Disconnect from script file", "Do you want to disconnect the script from the connected file?\nAny changes you make here won't be saved in the file"))
			{
				dynamic_cast<JavascriptProcessor*>(getProcessor())->disconnectFromFile();
			}
		}
		else
		{
			File f = PresetHandler::getPresetFileFromMenu(result - PRESET_MENU_ITEM_DELTA, getProcessor());

			if (!f.existsAsFile()) return;
			
			FileInputStream fis(f);

			ValueTree testTree = ValueTree::readFromStream(fis);

			if (testTree.isValid() && testTree.getProperty("Type") == "SynthChain")
			{
				findParentComponentOfClass<BackendProcessorEditor>()->loadNewContainer(testTree);
			}
			else
			{
				PresetHandler::showMessageWindow("Invalid Preset", "The selected Preset file was not a container", PresetHandler::IconType::Error);
			}
		}

	}
};


void ProcessorEditorHeader::mouseDoubleClick(const MouseEvent&)
{
	findParentComponentOfClass<BackendProcessorEditor>()->setRootProcessorWithUndo(getProcessor());
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
	bool off = !b->getToggleState();

	Colour buttonColour = isHeaderOfModulatorSynth() ? Colours::black.withAlpha(0.8f) : Colours::white;

	Colour shadowColour = isHeaderOfModulatorSynth() ? Colour(SIGNAL_COLOUR).withAlpha(0.25f) : Colours::white.withAlpha(0.6f);

	DropShadowEffect *shadow = dynamic_cast<DropShadowEffect*>(b->getComponentEffect());

	if (shadow != nullptr)
	{
		jassert(shadow != nullptr);

		shadow->setShadowProperties(DropShadow(off ? Colours::transparentBlack : shadowColour, 3, Point<int>()));
	}
		
	buttonColour = off ? Colours::grey.withAlpha(0.7f) : buttonColour;
	buttonColour = buttonColour.withAlpha(b->isEnabled() ? 1.0f : 0.2f);

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


void ProcessorEditorHeader::checkFoldButton()
{
	foldButton->setToggleState(!getProcessor()->getEditorState(Processor::Folded), dontSendNotification);

	const bool on = foldButton->getToggleState();

	Path foldPath;
		
	foldPath.loadPathFromData(HiBinaryData::ProcessorEditorHeaderIcons::foldedIcon, sizeof(HiBinaryData::ProcessorEditorHeaderIcons::foldedIcon));
	if(on) foldPath.applyTransform(AffineTransform::rotation(float_Pi * 0.5f));

	foldButton->setShape(foldPath, false, true, true);

	refreshShapeButton(foldButton);
}

#undef IS

} // namespace hise