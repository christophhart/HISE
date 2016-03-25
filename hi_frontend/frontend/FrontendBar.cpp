
FrontendBar::FrontendBar (FrontendProcessor *p)
    : fp(p)
{

    addAndMakeVisible (outMeter = new VuMeter (0.0, 0.0, VuMeter::StereoHorizontal));
    outMeter->setName ("new component");

	Colour dark(0xFF333333);
	Colour bright(0xFF999999);

	addAndMakeVisible(voiceLabel = new Label());

	addAndMakeVisible(panicButton = new ShapeButton("Panic", Colours::white.withAlpha(0.5f), Colours::white.withAlpha(0.8f), Colours::white));

	Path path;

	path.loadPathFromData(HiBinaryData::FrontendBinaryData::panicButtonShape, sizeof(HiBinaryData::FrontendBinaryData::panicButtonShape));

	panicButton->setShape(path, true, true, true);


	addAndMakeVisible(tooltipBar = new TooltipBar());

	addAndMakeVisible(infoButton = new ShapeButton("Info", Colours::white.withAlpha(0.5f), Colours::white.withAlpha(0.8f), Colours::white));

	Path path2;

	path2.loadPathFromData(HiBinaryData::FrontendBinaryData::infoButtonShape, sizeof(HiBinaryData::FrontendBinaryData::infoButtonShape));

	infoButton->setShape(path2, true, true, true);

	infoButton->addListener(this);

	infoButton->setTooltip("About");

	panicButton->setTooltip("All notes off");

	panicButton->addListener(this);

	voiceLabel->setFont(Font("Khmer UI", 13.0f, Font::plain));
	voiceLabel->setColour(Label::textColourId, Colour(0x44FFFFFF) );
	voiceLabel->setTooltip("Displays the amount of currently active voices");

	addAndMakeVisible(cpuSlider = new VuMeter(0.0f, 0.0f, VuMeter::MonoHorizontal));

	cpuSlider->setColour (VuMeter::backgroundColour, Colour (0xff333333));
	cpuSlider->setColour (VuMeter::ledColour, Colours::lightgrey.withAlpha(0.5f));
	cpuSlider->setColour (VuMeter::outlineColour, Colour (0x45000000));
	cpuSlider->setTooltip("Displays the CPU peak usage of this instance");


	cpuPeak = 0;
	cpuUpdater.setManualCountLimit(10);

	outMeter->setColour (VuMeter::backgroundColour, Colour (0xFF333333));
	outMeter->setColour (VuMeter::ledColour, Colours::lightgrey);
	outMeter->setColour (VuMeter::outlineColour, Colour (0x45000000));
	outMeter->setTooltip("Master output meter");
	

	tooltipBar->setColour(TooltipBar::ColourIds::backgroundColour, dark);
	tooltipBar->setColour(TooltipBar::ColourIds::iconColour, bright.withAlpha(0.6f));
	tooltipBar->setColour(TooltipBar::ColourIds::textColour, bright);

	voiceLabel->setFont(GLOBAL_FONT());

    setSize (800, 30);

}

FrontendBar::~FrontendBar()
{
    outMeter = nullptr;
}


void FrontendBar::paint (Graphics& g)
{
    g.fillAll (Colours::white);

    g.setColour (Colour (0xffbcbcbc));
    g.fillRect (0, 0, proportionOfWidth (1.0000f), proportionOfHeight (1.0000f));

    //g.setColour (Colour (0xff5a5a5a));
    //g.drawRect (0, 0, proportionOfWidth (1.0000f), proportionOfHeight (1.0000f), 1);

    
	g.setColour(Colour(0xFF252525 ));

	g.fillAll();

	g.setColour(Colour(0xFF222222));

	g.drawRect(getLocalBounds(), 1);

	g.setColour(Colours::white.withAlpha(0.7f));

	g.setFont(GLOBAL_FONT());

	//g.drawText("Registered to: " + fp->unlocker.getUserEmail(), getLocalBounds().reduced(3).translated(-34, 0), Justification::right);

	g.setColour(Colours::white.withAlpha(0.3f));



	g.drawText("CPU", cpuSlider->getBounds(), Justification::centred);

}

void FrontendBar::resized()
{
    outMeter->setBounds (29, 3, 150, 24);

	int toolWidth = getWidth() - 55 - 55 - 155 - 2*getHeight() - 6;

	

	tooltipBar->setBounds(3, 3, toolWidth, getHeight() - 6);

	panicButton->setBounds(tooltipBar->getRight() + 5, 2, getHeight()-4, getHeight()-4);

	cpuSlider->setBounds(panicButton->getRight() + 5, 3, 50, getHeight() - 6);

	voiceLabel->setBounds(cpuSlider->getRight() + 5, 3, 50, getHeight() - 6);

	outMeter->setBounds (voiceLabel->getRight() + 5, 3, 150, getHeight() - 6);

	infoButton->setBounds(outMeter->getRight() + 5, 2, getHeight() -4, getHeight() -4);
}



void FrontendBar::buttonClicked(Button *b)
{
	if(b == infoButton)
	{
		FrontendProcessorEditor *fpe = dynamic_cast<FrontendProcessorEditor*>(getParentComponent());

		if(fpe != nullptr)
		{
			fpe->aboutPage->showAboutPage();
		}
	}
	else if(b == panicButton)
	{
		
		fp->allNotesOff();
		fp->resetVoiceCounter();
		fp->compileAllScripts();

		FrontendProcessorEditor *fpe = dynamic_cast<FrontendProcessorEditor*>(getParentComponent());

		if(fpe != nullptr)
		{
			fpe->resetInterface();
		}
	}
}
