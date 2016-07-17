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
*   Commercial licences for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licencing:
*
*   http://www.hartinstruments.net/hise/
*
*   HISE is based on the JUCE library,
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

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
	idLabel->setColour (Label::textWhenEditingColourId, isHeaderOfModulatorSynth() ? Colours::black : Colours::white);
	idLabel->setColour (Label::ColourIds::backgroundWhenEditingColourId, idLabel->findColour(Label::textWhenEditingColourId).contrasting());
	idLabel->setColour(TextEditor::ColourIds::highlightedTextColourId, idLabel->findColour(Label::textWhenEditingColourId));
	idLabel->setColour(TextEditor::ColourIds::highlightColourId, Colour(0xFF888888));
    idLabel->setColour (TextEditor::textColourId, Colours::black);
    idLabel->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

	idLabel->addListener(this);
	
	idLabel->setEditable(!isHeaderOfChain() || isHeaderOfModulatorSynth());

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

	
    
    addAndMakeVisible (deleteButton = new ShapeButton ("Delete Processor", Colours::white, Colours::white, Colours::white));
	Path deletePath;
	deletePath.loadPathFromData(HiBinaryData::headerIcons::closeIcon, sizeof(HiBinaryData::headerIcons::closeIcon));
	deleteButton->setShape(deletePath, true, true, true);
	deleteButton->setToggleState(true, dontSendNotification);
	refreshShapeButton(deleteButton);

	deleteButton->addListener(this);

	addAndMakeVisible(routeButton = new ShapeButton("Edit Routing Matrix", Colours::white, Colours::white, Colours::white));
	Path routePath;
	routePath.loadPathFromData(HiBinaryData::SpecialSymbols::routingIcon, sizeof(HiBinaryData::SpecialSymbols::routingIcon));
	routeButton->setShape(routePath, true, true, true);
	routeButton->setToggleState(true, dontSendNotification);
	refreshShapeButton(routeButton);

	routeButton->addListener(this);
	routeButton->setVisible(dynamic_cast<RoutableProcessor*>(getProcessor()) != nullptr);

	addAndMakeVisible (addButton = new ShapeButton ("Add new Processor", Colours::white, Colours::white, Colours::white));
	Path addPath;
	addPath.loadPathFromData(HiBinaryData::headerIcons::addIcon, sizeof(HiBinaryData::headerIcons::addIcon));
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


		if(getModulatorMode() == Modulation::PitchMode)
		{
			intensitySlider->setRange(-12.0, 12.0, 0.01);
			intensitySlider->setTextValueSuffix(" st");
			intensitySlider->setTextBoxIsEditable(true);
			
		};

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

bool ProcessorEditorHeader::isHeaderOfEffectProcessor() const		{ return dynamic_cast<const EffectProcessor*>(getProcessor()) != nullptr;	};

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
    ProcessorEditor *parentEditor = getEditor()->getParentEditor();
    
    if(parentEditor != nullptr)
    {
        g.setColour(parentEditor->getProcessor()->getColour());
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

    
    
	const bool isInternalChain = isHeaderOfChain() && !isHeaderOfModulatorSynth();

	if (isInternalChain)
	{

	}


	if (isHeaderOfChain())
	{
		addButton->setBounds(x, yOffset, addCloseWidth, addCloseWidth);

		x += addCloseWidth + 8;
	}


#if STANDALONE_CONVOLUTION

#else
	foldButton->setBounds(x, yOffset, addCloseWidth, addCloseWidth);

	x += addCloseWidth + 8;
#endif


	if ((getProcessor() == getProcessor()->getMainController()->getMainSynthChain()))
	{
	}
	else
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
	
	const bool showValueMeter = !isHeaderOfMidiProcessor();

	if (showValueMeter)
	{
		valueMeter->setBounds(x, yOffset2, getWidth() / 2 - x, 20);
		x = valueMeter->getRight() + 3;
	}

	if (dynamic_cast<RoutableProcessor*>(getProcessor()))
	{
		x = valueMeter->getRight() + 3;

#if STANDALONE_CONVOLUTION
        
        routeButton->setVisible(false);
        
#else
        
		routeButton->setBounds(x, yOffset, addCloseWidth, addCloseWidth);
		
		x = routeButton->getRight() + 5;
        
#endif
        
	}
	else
	{
		x += 3;
	}
	
	if (IS(JavascriptProcessor))
	{
		debugButton->setBounds(x, yOffset2, 30, 20);
		debugButton->setVisible(true);
	}
	
	intensitySlider->setBounds (x, yOffset2, 200, 20);

    

	
    plotButton->setBounds (getWidth() - 101 - 50, yOffset2, 30, 20);
	plotButton->setVisible(false);
    

	
#if STANDALONE_CONVOLUTION
    
    deleteButton->setVisible(false);
    
#else
    
	deleteButton->setEnabled(getEditor()->getIndentationLevel() != 0);

	
	deleteButton->setBounds (getWidth() - 8 - addCloseWidth, yOffset, addCloseWidth, addCloseWidth);
    
#endif
    
	if(isHeaderOfModulatorSynth())
	{
		balanceSlider->setBounds(intensitySlider->getRight() + 5, yOffset2, 28, 24);

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

void ProcessorEditorHeader::buttonClicked (Button* buttonThatWasClicked)
{
    if (buttonThatWasClicked == debugButton)
    {
        const bool value = buttonThatWasClicked->getToggleState();
		getProcessor()->enableConsoleOutput(!value);

		if(dynamic_cast<JavascriptProcessor*>(getProcessor()) != nullptr)
		{
			JavascriptProcessor *sp = value ? nullptr : dynamic_cast<JavascriptProcessor*>(getProcessor());

			getProcessor()->getMainController()->setWatchedScriptProcessor(sp, getEditor()->getBody());

			AutoPopupDebugComponent *c = findParentComponentOfClass<BackendProcessorEditor>()->getDebugComponent(true, CombinedDebugArea::AreaIds::ApiCollectionEnum);

			if (c != nullptr) c->showComponentInDebugArea(!value);

		}


		buttonThatWasClicked->setToggleState(!value, dontSendNotification);
    }

    else if (buttonThatWasClicked == plotButton)
    {
        const bool value = buttonThatWasClicked->getToggleState();
		setPlotButton(!value);
    }
	else if (buttonThatWasClicked == bypassButton->button)
    {
        
		bool shouldBeBypassed = bypassButton->getToggleState();
		getProcessor()->setBypassed(shouldBeBypassed);
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
		dynamic_cast<RoutableProcessor*>(getProcessor())->editRouting(this);
	}
    else if (buttonThatWasClicked == foldButton)
    {
		bool shouldBeFolded = toggleButton(buttonThatWasClicked);

		getEditor()->setFolded(shouldBeFolded, true);

		getEditor()->sendResizedMessage();

		checkFoldButton();
    }
    else if (buttonThatWasClicked == deleteButton)
	{
        PresetHandler::setChanged(getProcessor());
        
		if(dynamic_cast<BackendProcessorEditor*>(getEditor()->getParentComponent()) != nullptr)
		{
			dynamic_cast<BackendProcessorEditor*>(getEditor()->getParentComponent())->clearPopup();
			return;
		}

		if(dynamic_cast<ModulatorSynth*>(getProcessor()) == nullptr || PresetHandler::showYesNoWindow("Delete " + getProcessor()->getId() + "?", "Do you want to delete the Synth module?"))
		{
            
            PresetHandler::setChanged(getProcessor());
            
			getEditor()->getParentEditor()->getPanel()->removeProcessorEditor(getProcessor());
		}
	}
  
	else if (buttonThatWasClicked == addButton)
	{
        getEditor()->getProcessor()->setEditorState(Processor::Folded, false);
		createProcessorFromPopup();
	}
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

		Modulator *mod = dynamic_cast<Modulator*>(p);

		plotButton->setToggleState(mod->isPlotted(), dontSendNotification);

	
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
	Modulator *mod = dynamic_cast<Modulator*>(getProcessor());

	// the plot button should only be displayed at Modulators...
	jassert(mod != nullptr);

	plotButton->setToggleState(on, dontSendNotification);
	
	if(on)
	{
		getProcessor()->getMainController()->addPlottedModulator(mod);

	}
	else
	{
		getProcessor()->getMainController()->removePlottedModulator(mod);

		
		//plotterWindow = nullptr;
	}
};

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
#if STANDALONE_CONVOLUTION
    return;
#endif
    
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
		PopupMenu m;
		ScopedPointer<PopupLookAndFeel> l = new PopupLookAndFeel();
		m.setLookAndFeel(l);

		m.addSectionHeader("Create new Processor ");
		
		t->fillPopupMenu(m);

		m.addSeparator();
		m.addSectionHeader("Add from Clipboard");

		String clipBoardName = PresetHandler::getProcessorNameFromClipboard(t);

		if(clipBoardName != String::empty)  m.addItem(CLIPBOARD_ITEM_MENU_INDEX, "Add " + clipBoardName + " from Clipboard");
		else								m.addItem(-1, "No compatible Processor in clipboard.", false);

		clipBoard = clipBoardName != String::empty;

		m.addSeparator();
		m.addSectionHeader("Add from Preset-Folder:");
		m.addSubMenu("Add saved Preset", PresetHandler::getAllSavedPresets(PRESET_MENU_ITEM_DELTA, getProcessor()), true);
		
		result = m.show();
	}

	// =================================================================================================================
	// Create the processor

	if(result == 0)									return;

	else if(result == CLIPBOARD_ITEM_MENU_INDEX && clipBoard) processorToBeAdded = PresetHandler::createProcessorFromClipBoard(getProcessor());

	else if (result >= PRESET_MENU_ITEM_DELTA)		processorToBeAdded = PresetHandler::createProcessorFromPreset(result - PRESET_MENU_ITEM_DELTA, getProcessor());

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

	processorToBeAdded->setId(FactoryType::getUniqueName(processorToBeAdded));

	if (ProcessorHelpers::is<ModulatorSynth>(processorToBeAdded) && dynamic_cast<ModulatorSynthGroup*>(getEditor()->getProcessor()) == nullptr)
	{
		dynamic_cast<ModulatorSynth*>(processorToBeAdded)->addProcessorsWhenEmpty();
	}

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

	getEditor()->getProcessorAsChain()->getHandler()->add(processorToBeAdded, insertBeforeSibling);

	getEditor()->changeListenerCallback(getProcessor());

	getEditor()->childEditorAmountChanged();

#if USE_BACKEND

	findParentComponentOfClass<BackendProcessorEditor>()->rebuildModuleList(false);

#endif

	if (dynamic_cast<ModulatorSynth*>(getProcessor()) != nullptr)
	{
		getProcessor()->getMainController()->getMainSynthChain()->compileAllScripts();
	}
	else if (dynamic_cast<JavascriptProcessor*>(processorToBeAdded) != nullptr)
	{
		dynamic_cast<JavascriptProcessor*>(processorToBeAdded)->compileScript();
	}

    PresetHandler::setChanged(getProcessor());
    
	return;

}

void ProcessorEditorHeader::timerCallback()
{

	if (getProcessor() != nullptr)
	{
		if (isHeaderOfModulator())
		{
			const float outputValue = getProcessor()->getOutputValue();
			const float value = dynamic_cast<Modulation*>(getProcessor())->getMode() == Modulation::PitchMode ? outputValue / 2.0f : outputValue;
			valueMeter->setPeak(value, -1.0f);
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
			Save = 1,
			Copy,
			InsertBefore,
			CloseAllChains,
			CheckForDuplicate,
			CreateScriptVariable,
			ReplaceWithClipboardContent,
			SaveAllSamplesToGlobalFolder,
			OpenIncludedFileInPopup,
			numMenuItems
		};


		ScopedPointer<PopupLookAndFeel> luf = new PopupLookAndFeel();
		PopupMenu m;
		m.setLookAndFeel(luf);

		const bool isMainSynthChain = getProcessor()->getMainController()->getMainSynthChain() == getProcessor();

		m.addSectionHeader("Copy Tools");

		m.addItem(Save, "Save " + getProcessor()->getId() + " as Preset");
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

		m.addItem(CreateScriptVariable, "Create script variable declaration");


		if (isMainSynthChain)
		{
			m.addSeparator();
			m.addSectionHeader("Root Container Tools");
			m.addItem(CheckForDuplicate, "Check children for duplicate IDs");
			m.addItem(ReplaceWithClipboardContent, "Replace With Clipboard Content");
			m.addItem(SaveAllSamplesToGlobalFolder, "Save all samples to global folder");
			
			m.addSubMenu("Replace Root Container", PresetHandler::getAllSavedPresets(PRESET_MENU_ITEM_DELTA, getProcessor()), true);
			m.addSeparator();
		}

		else if (JavascriptProcessor *sp = dynamic_cast<JavascriptProcessor*>(getProcessor()))
		{
			m.addSeparator();
			m.addSectionHeader("Script Processor Tools");
			m.addItem(OpenIncludedFileInPopup, "Open Included File in Popup Window", sp->getNumWatchedFiles() != 0);
		}

		int result = m.show();

		if(result == 0) return;

		if(result == Save)
		{
			PresetHandler::saveProcessorAsPreset(getProcessor());
			idLabel->setText(getProcessor()->getId(), dontSendNotification);
		}
		else if (result == Copy) PresetHandler::copyProcessorToClipboard(getProcessor());

		else if (result == InsertBefore)
		{
			ProcessorEditor *parentEditor = getEditor()->getParentEditor();

			ProcessorEditorHeader *parentHeader = parentEditor->getHeader();

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
		else if (result == CreateScriptVariable)
		{
			ProcessorHelpers::getScriptVariableDeclaration(getEditor()->getProcessor());
		}
		else if (result == SaveAllSamplesToGlobalFolder)
		{
			String packageName = PresetHandler::getCustomName("PackageName");

			PresetPlayerHandler::addInstrumentToPackageXml(getProcessor()->getId(), packageName);

			getProcessor()->getMainController()->getSampleManager().saveAllSamplesToGlobalFolder(packageName);

			getProcessor()->getMainController()->getMainSynthChain()->setPackageName(packageName);

			getProcessor()->getMainController()->replaceReferencesToGlobalFolder();

			

			PresetHandler::saveProcessorAsPreset(getProcessor(), PresetPlayerHandler::getSpecialFolder(PresetPlayerHandler::PackageDirectory, packageName));
		}

		else if (result == ReplaceWithClipboardContent)
		{
			
			ScopedPointer<XmlElement> xml = XmlDocument::parse(SystemClipboard::getTextFromClipboard());

			if (xml != nullptr)
			{
				ValueTree v = ValueTree::fromXml(*xml);

				if (v.isValid() && v.getProperty("Type") == "SynthChain")
				{
					findParentComponentOfClass<BackendProcessorEditor>()->loadNewContainer(v);
					return;
				}
			}
			
			PresetHandler::showMessageWindow("Invalid Preset", "The clipboard does not contain a valid container.", PresetHandler::IconType::Warning);
		}
		else if (result == OpenIncludedFileInPopup)
		{
			JavascriptProcessor *sp = dynamic_cast<JavascriptProcessor*>(getProcessor());

			if (sp->getNumWatchedFiles() == 1)
			{
				sp->showPopupForFile(0);

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

	Colour shadowColour = isHeaderOfModulatorSynth() ? Colours::cyan.withAlpha(0.25f) : Colours::white.withAlpha(0.6f);

	DropShadowEffect *shadow = dynamic_cast<DropShadowEffect*>(b->getComponentEffect());

	if (shadow != nullptr)
	{
		jassert(shadow != nullptr);

		shadow->setShadowProperties(DropShadow(off ? Colours::transparentBlack : shadowColour, 3, Point<int>()));

	}
	
		
	buttonColour = off ? Colours::grey.withAlpha(0.7f) : buttonColour;

	buttonColour = buttonColour.withAlpha(b->isEnabled() ? 1.0f : 0.2f);

		

	b->setColours(buttonColour.withMultipliedAlpha(0.7f), buttonColour.withMultipliedAlpha(1.0f), buttonColour.withMultipliedAlpha(1.0f));
	b->repaint();
}

void ProcessorEditorHeader::labelTextChanged(Label *l)
{
	if (l == idLabel)
	{

		getEditor()->getProcessor()->setId(l->getText());
        
        findParentComponentOfClass<BackendProcessorEditor>()->getKeyboard()->grabKeyboardFocus();

        PresetHandler::setChanged(getProcessor());
	}
}


void ProcessorEditorHeader::checkFoldButton()
{
	foldButton->setToggleState(!getProcessor()->getEditorState(Processor::Folded), dontSendNotification);

	const bool on = foldButton->getToggleState();

	Path foldPath;
		
	foldPath.loadPathFromData(HiBinaryData::headerIcons::foldedIcon, sizeof(HiBinaryData::headerIcons::foldedIcon));
	if(on) foldPath.applyTransform(AffineTransform::rotation(float_Pi * 0.5f));

	foldButton->setShape(foldPath, false, true, true);

	refreshShapeButton(foldButton);
}

#undef IS