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

//[Headers] You can add your own extra header files here...
namespace hise { using namespace juce;
//[/Headers]

#include "SampleEditor.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

struct SamplerDisplayWithTimeline : public Component
{
	static constexpr int TimelineHeight = 24;

	enum class TimeDomain
	{
		Samples,
		Milliseconds,
		Seconds
	};

	SamplerSoundWaveform* getWaveform() { return dynamic_cast<SamplerSoundWaveform*>(getChildComponent(0)); }
	const SamplerSoundWaveform* getWaveform() const { return dynamic_cast<SamplerSoundWaveform*>(getChildComponent(0)); }

	void resized() override
	{
		auto b = getLocalBounds();
		b.removeFromTop(TimelineHeight);
		getWaveform()->setBounds(b);
	}

	void mouseDown(const MouseEvent& e) override
	{
		PopupLookAndFeel plaf;
		PopupMenu m;
		m.setLookAndFeel(&plaf);

		m.addItem(1, "Samples", true, currentDomain == TimeDomain::Samples);
		m.addItem(2, "Milliseconds", true, currentDomain == TimeDomain::Milliseconds);
		m.addItem(3, "Seconds", true, currentDomain == TimeDomain::Seconds);

		if (auto r = m.show())
		{
			currentDomain = (TimeDomain)(r - 1);
			repaint();
		}
	}

	String getText(float normalisedX) const
	{
		if (auto s = getWaveform()->getCurrentSound())
		{
			auto sampleValue = roundToInt(normalisedX * sampleLength);

			if(currentDomain == TimeDomain::Samples)
				return String(roundToInt(sampleValue));

			auto msValue = sampleValue / jmax(1.0, sampleRate) * 1000.0;

			if(currentDomain == TimeDomain::Milliseconds)
				return String(roundToInt(msValue)) + " ms";

			String sec;
			sec << Time((int64)msValue).formatted("%M:%S:");

			auto ms = String(roundToInt(msValue) % 1000);

			while (ms.length() < 3)
				ms = "0" + ms;

			sec << ms;
			return sec;
		}

		return {};
	}

	void paint(Graphics& g) override
	{
		auto visibleArea = findParentComponentOfClass<Viewport>()->getViewArea();

		auto b = getLocalBounds().removeFromTop(TimelineHeight);

		g.setFont(GLOBAL_FONT());

		int delta = 200;

		if (auto s = getWaveform()->getCurrentSound())
		{
			sampleLength = s->getReferenceToSound(0)->getLengthInSamples();
			sampleRate = s->getReferenceToSound(0)->getSampleRate();
		}

		for (int i = 0; i < getWidth(); i += delta)
		{
			auto textArea = b.removeFromLeft(delta).toFloat();

			g.setColour(Colours::white.withAlpha(0.1f));
			g.drawVerticalLine(i, 3.0f, (float)TimelineHeight);

			g.setColour(Colours::white.withAlpha(0.4f));

			auto normalisedX = (float)i / (float)getWidth();

			g.drawText(getText(normalisedX), textArea.reduced(5.0f, 0.0f), Justification::centredLeft);
		}
	}

	double sampleLength;
	double sampleRate;

	TimeDomain currentDomain = TimeDomain::Seconds;
	
};

struct VerticalZoomer : public Component,
						public Slider::Listener,
						public SampleMap::Listener,
						public SampleEditHandler::Listener
						
{
	VerticalZoomer(SamplerSoundWaveform* waveform, ModulatorSampler* s):
		display(waveform),
		sampler(s)
	{
		sampler->getSampleMap()->addListener(this);
		sampler->getSampleEditHandler()->addSelectionListener(this);

		addAndMakeVisible(zoomSlider);
		zoomSlider.setRange(1.0, 16.0);
		zoomSlider.setSliderStyle(Slider::LinearBarVertical);
		zoomSlider.addListener(this);
		display->addMouseListener(this, true);
	}

	void mouseWheelMove(const MouseEvent& e, const MouseWheelDetails& d) override
	{
		if (e.eventComponent == display && e.mods.isShiftDown())
		{
			zoomSlider.mouseWheelMove(e, d);
		}
	}

	void soundSelectionChanged(SampleSelection& newSelection) override
	{
		repaint();
	}

	virtual void sampleMapWasChanged(PoolReference) {};

	virtual void samplePropertyWasChanged(ModulatorSamplerSound* s, const Identifier& id, const var& newValue)
	{
		if (s == display->getCurrentSound())
		{
			if (id == SampleIds::Normalized || id == SampleIds::Volume)
			{
				display->refresh(sendNotificationSync);
				repaint();
			}
		}
	};

	~VerticalZoomer()
	{
		sampler->getSampleMap()->removeListener(this);
		sampler->getSampleEditHandler()->removeSelectionListener(this);
	}

	void sliderValueChanged(Slider* s) override
	{
		display->setVerticalZoom(s->getValue());
		repaint();
	};

	void drawLevels(Graphics& g, Rectangle<float> area, float gain)
	{
		auto invGain = 1.0f / gain;
		auto invGainHalf = invGain / 2.0f;

		auto db = String(roundToInt(Decibels::gainToDecibels(invGain)));
		auto dbHalf = String(roundToInt(Decibels::gainToDecibels(invGainHalf)));
		auto silence = "-dB";
		
		g.setFont(GLOBAL_FONT());
		g.setColour(Colours::white.withAlpha(0.5f));
		g.drawText(db, area, Justification::topRight);
		g.drawText(db, area, Justification::bottomRight);

		area = area.withSizeKeepingCentre(area.getWidth(), area.getHeight() / 2.0f + 10.0f);

		g.drawText(dbHalf, area, Justification::topRight);
		g.drawText(dbHalf, area, Justification::bottomRight);

		g.drawText(silence, area, Justification::centredRight);
	}

	void paint(Graphics& g) override
	{
		if (auto s = display->getCurrentSound())
		{
			auto isStereo = s->getReferenceToSound(0)->isStereo();

			auto gain = display->getCurrentSampleGain();

			auto b = getLocalBounds().toFloat();

			if (isStereo)
			{
				drawLevels(g, b.removeFromTop(b.getHeight() / 2.0f), gain);
				drawLevels(g, b, gain);
			}
			else
			{
				drawLevels(g, b, gain);
			}
		}
	}

	void resized() override
	{
		zoomSlider.setBounds(getLocalBounds().removeFromLeft(8));
        zoomSlider.setVisible(false);
	}

	WeakReference<ModulatorSampler> sampler;
	SamplerSoundWaveform* display;
	Slider zoomSlider;
};

struct SamplePositionPainter : public snex::ui::Graph::PeriodicUpdaterBase
{
	SamplePositionPainter(ModulatorSampler* s, snex::ui::Graph& g) :
		PeriodicUpdaterBase(g, s->getMainController()->getGlobalUIUpdater()),
		sampler(s)
	{};

	void paintPosition(Graphics& g, Range<int> range)
	{
		auto s = sampler->getSamplerDisplayValues().currentSamplePos;

		if (parent.getSamplePosition(s))
		{
			g.setColour(Colours::white.withAlpha(0.7f));
			g.drawVerticalLine((int)s, 24, parent.getHeight());
		}
	}

	WeakReference<ModulatorSampler> sampler;
};

//==============================================================================
SampleEditor::SampleEditor (ModulatorSampler *s, SamplerBody *b):
	SamplerSubEditor(s->getSampleEditHandler()),
	sampler(s),
	graphHandler(*this)
{
    //[Constructor_pre] You can add your own custom stuff here..
    //[/Constructor_pre]

    addAndMakeVisible (viewport = new Viewport ("new viewport"));
    viewport->setScrollBarsShown (false, true);

    addAndMakeVisible (volumeSetter = new ValueSettingComponent(s));
    addAndMakeVisible (pitchSetter = new ValueSettingComponent(s));
    addAndMakeVisible (sampleStartSetter = new ValueSettingComponent(s));
    addAndMakeVisible (sampleEndSetter = new ValueSettingComponent(s));
    addAndMakeVisible (loopStartSetter = new ValueSettingComponent(s));
    addAndMakeVisible (loopEndSetter = new ValueSettingComponent(s));
    addAndMakeVisible (loopCrossfadeSetter = new ValueSettingComponent(s));
    addAndMakeVisible (startModulationSetter = new ValueSettingComponent(s));
    addAndMakeVisible (toolbar = new Toolbar());
    toolbar->setName ("new component");

    addAndMakeVisible(spectrumSlider);
    spectrumSlider.setRange(0.0, 1.0, 0.0);
    spectrumSlider.setSliderStyle(Slider::SliderStyle::LinearBar);
	spectrumSlider.onValueChange = [this]()
    {
        auto sAlpha = spectrumSlider.getValue();
        auto wAlpha = 1.0f - sAlpha;
        currentWaveForm->getThumbnail()->setSpectrumAndWaveformAlpha(sAlpha, wAlpha);
    };

    addAndMakeVisible (panSetter = new ValueSettingComponent(s));

    //[UserPreSize]

	viewContent = new SamplerDisplayWithTimeline();

	viewContent->addAndMakeVisible(currentWaveForm = new SamplerSoundWaveform(sampler));

	viewport->setViewedComponent(viewContent, false);
	viewport->setScrollBarThickness(13);

	fader.addScrollBarToAnimate(viewport->getHorizontalScrollBar());
	viewport->getHorizontalScrollBar().setLookAndFeel(&laf);

	addAndMakeVisible(verticalZoomer = new VerticalZoomer(currentWaveForm, sampler));

	addAndMakeVisible(graph = new snex::ui::Graph());
	graph->setVisible(false);

	graph->getBufferFunction = BIND_MEMBER_FUNCTION_0(SampleEditor::getGraphBuffer);
	graph->getSampleRateFunction = [this]()
	{
		if (auto c = currentWaveForm->getCurrentSound())
			return c->getSampleRate();

		return 0.0;
	};

	graph->drawMarkerFunction = [](Graphics& g)
	{
	};

	graph->setPeriodicUpdater(new SamplePositionPainter(sampler, *graph));

	currentWaveForm->setVisible(true);

	zoomFactor = 1.0f;

	body = b;

	samplerEditorCommandManager = new ApplicationCommandManager();

	samplerEditorCommandManager->registerAllCommandsForTarget(this);
	samplerEditorCommandManager->getKeyMappings()->resetToDefaultMappings();

	samplerEditorCommandManager->setFirstCommandTarget(this);

	toolbarFactory = new SampleEditorToolbarFactory(this);

	addButton(SampleMapCommands::ZoomIn, false, "zoom-in");
	addButton(SampleMapCommands::ZoomOut, false, "zoom-out");
    addButton(SampleMapCommands::Analyser, true, "analyse");
    
	addButton(SampleMapCommands::SelectWithMidi, true, "select-midi", "select-mouse");
	addButton(SampleMapCommands::EnablePlayArea, true, "play-area");
	addButton(SampleMapCommands::EnableSampleStartArea, true, "samplestart-area");
	addButton(SampleMapCommands::EnableLoopArea, true, "loop-area");
    addButton(SampleMapCommands::NormalizeVolume, true, "normalise-on", "normalise-off");
    addButton(SampleMapCommands::LoopEnabled, true, "loop-on", "loop-off");
    
    addAndMakeVisible(sampleSelector = new ComboBox());
    addAndMakeVisible(multimicSelector = new ComboBox());
    
    sampleSelector->setLookAndFeel(&claf);
    multimicSelector->setLookAndFeel(&claf);
    
    sampleSelector->addListener(this);
    multimicSelector->addListener(this);
    
    claf.setDefaultColours(*sampleSelector);
    claf.setDefaultColours(*multimicSelector);
    
	toolbar->setStyle(Toolbar::ToolbarItemStyle::iconsOnly);
	toolbar->addDefaultItems(*toolbarFactory);

	toolbar->setColour(Toolbar::ColourIds::backgroundColourId, Colours::transparentBlack);
	toolbar->setColour(Toolbar::ColourIds::buttonMouseOverBackgroundColourId, Colours::white.withAlpha(0.3f));
	toolbar->setColour(Toolbar::ColourIds::buttonMouseDownBackgroundColourId, Colours::white.withAlpha(0.4f));

	panSetter->setPropertyType(SampleIds::Pan);
	volumeSetter->setPropertyType(SampleIds::Volume);
	pitchSetter->setPropertyType(SampleIds::Pitch);
	sampleStartSetter->setPropertyType(SampleIds::SampleStart);
	sampleEndSetter->setPropertyType(SampleIds::SampleEnd);
	startModulationSetter->setPropertyType(SampleIds::SampleStartMod);
	loopStartSetter->setPropertyType(SampleIds::LoopStart);
	loopEndSetter->setPropertyType(SampleIds::LoopEnd);
	loopCrossfadeSetter->setPropertyType(SampleIds::LoopXFade);

	loopStartSetter->setLabelColour(Colours::green.withAlpha(0.1f), Colours::white);
	loopEndSetter->setLabelColour(Colours::green.withAlpha(0.1f), Colours::white);
	loopCrossfadeSetter->setLabelColour(Colours::yellow.withAlpha(0.1f), Colours::white);
	startModulationSetter->setLabelColour(Colours::blue.withAlpha(0.1f), Colours::white);

	sampler->getSampleMap()->addListener(this);

    //[/UserPreSize]

    setSize (800, 250);


    //[Constructor] You can add your own custom stuff here..

	sampleStartSetter->addChangeListener(this);
    sampleEndSetter->addChangeListener(this);
    loopStartSetter->addChangeListener(this);
    loopEndSetter->addChangeListener(this);
    loopCrossfadeSetter->addChangeListener(this);
    startModulationSetter->addChangeListener(this);

	currentWaveForm->addAreaListener(this);


    currentWaveForm->setColour(AudioDisplayComponent::ColourIds::bgColour, Colour(0xff1d1d1d));
    currentWaveForm->setColour(AudioDisplayComponent::ColourIds::outlineColour, Colour(0xff1d1d1d));
    //[/Constructor]
}

SampleEditor::~SampleEditor()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
	samplerEditorCommandManager->setFirstCommandTarget(nullptr);


	if (sampler != nullptr)
	{
		sampler->getSampleMap()->removeListener(this);
	}
    //[/Destructor_pre]

    viewport = nullptr;
    volumeSetter = nullptr;
    pitchSetter = nullptr;
    sampleStartSetter = nullptr;
    sampleEndSetter = nullptr;
    loopStartSetter = nullptr;
    loopEndSetter = nullptr;
    loopCrossfadeSetter = nullptr;
    startModulationSetter = nullptr;
    toolbar = nullptr;
    panSetter = nullptr;


    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

void SampleEditor::samplePropertyWasChanged(ModulatorSamplerSound* s, const Identifier& id, const var& /*newValue*/)
{
	if (s == currentWaveForm->getCurrentSound() && SampleIds::Helpers::isAudioProperty(id))
	{
		currentWaveForm->updateRanges();
	}
		
}

juce::AudioSampleBuffer& SampleEditor::getGraphBuffer()
{
	if (!graphHandler.lastSound)
		graphHandler.graphBuffer.setSize(0, 0);
	else
	{
		auto sound = graphHandler.lastSound->getReferenceToSound(0);

		ScopedPointer<AudioFormatReader> afr;

		if (sound->isMonolithic())
			afr = sound->createReaderForPreview();
		else
			afr = PresetHandler::getReaderForFile(sound->getFileName(true));

		graphHandler.graphBuffer.setSize(afr->numChannels, afr->lengthInSamples);
		afr->read(&graphHandler.graphBuffer, 0, afr->lengthInSamples, 0, true, true);
		afr = nullptr;
	}
	
	return graphHandler.graphBuffer;
}

//==============================================================================
void SampleEditor::paint (Graphics& g)
{
	auto b = getLocalBounds().removeFromTop(24);

    claf.drawFake3D(g, b);
}

void SampleEditor::resized()
{
	auto b = getLocalBounds();

	auto topBar = b.removeFromTop(24);
	
	for (auto b : menuButtons)
		b->setBounds(topBar.removeFromLeft(topBar.getHeight()).reduced(2));

    b = b.reduced(8);
    
    
    
    multimicSelector->setBounds(topBar.removeFromRight(100));
    sampleSelector->setBounds(topBar.removeFromRight(300));
    
    spectrumSlider.setBounds(topBar.removeFromRight(100));
    
	//toolbar->setBounds(topBar);

	b.removeFromTop(20);
	int setterHeight = 32;
	int width = 150;
    
    if(isInWorkspace())
        width = jmin(b.getWidth() / 8, 150);
    
	int halfWidth = width / 2;
	static constexpr int Margin = 5;

	if (isInWorkspace())
	{
		b.removeFromBottom(12);
		auto bottom = b.removeFromBottom(setterHeight);
		bottom = bottom.withSizeKeepingCentre(width * 8, bottom.getHeight());

		volumeSetter->setBounds(bottom.removeFromLeft(width - Margin));
		bottom.removeFromLeft(Margin);

		panSetter->setBounds(bottom.removeFromLeft(halfWidth - Margin));
		bottom.removeFromLeft(Margin);

		pitchSetter->setBounds(bottom.removeFromLeft(halfWidth - Margin));
		bottom.removeFromLeft(Margin);

		sampleStartSetter->setBounds(bottom.removeFromLeft(width - Margin));
		bottom.removeFromLeft(Margin);

		sampleEndSetter->setBounds(bottom.removeFromLeft(width - Margin));
		bottom.removeFromLeft(Margin);

		startModulationSetter->setBounds(bottom.removeFromLeft(width - Margin));
		bottom.removeFromLeft(Margin);

		loopStartSetter->setBounds(bottom.removeFromLeft(width - Margin));
		bottom.removeFromLeft(Margin);

		loopEndSetter->setBounds(bottom.removeFromLeft(width - Margin));
		bottom.removeFromLeft(Margin);

		loopCrossfadeSetter->setBounds(bottom.removeFromLeft(width - Margin));
		bottom.removeFromLeft(Margin);
	}
	else
	{
		auto bottom = b.removeFromBottom(2 * setterHeight + 10);

		bottom = bottom.withSizeKeepingCentre(width * 4, bottom.getHeight());

		auto r1 = bottom.removeFromTop(setterHeight);
		auto r2 = bottom.removeFromBottom(setterHeight);

		volumeSetter->setBounds(r2.removeFromLeft(width - Margin));
		r2.removeFromLeft(Margin);

		panSetter->setBounds(r1.removeFromLeft(halfWidth - Margin));
		r1.removeFromLeft(Margin);

		pitchSetter->setBounds(r1.removeFromLeft(halfWidth - Margin));
		r1.removeFromLeft(Margin);

		sampleEndSetter->setBounds(r2.removeFromLeft(width - Margin));
		r2.removeFromLeft(Margin);

		sampleStartSetter->setBounds(r1.removeFromLeft(width - Margin));
		r1.removeFromLeft(Margin);

		loopEndSetter->setBounds(r2.removeFromLeft(width - Margin));
		r2.removeFromLeft(Margin);

		loopStartSetter->setBounds(r1.removeFromLeft(width - Margin));
		r1.removeFromLeft(Margin);

		startModulationSetter->setBounds(r2.removeFromLeft(width - Margin));
		r2.removeFromLeft(Margin);

		loopCrossfadeSetter->setBounds(r1.removeFromLeft(width - Margin));
		r1.removeFromLeft(Margin);
	}

	b.removeFromBottom(12);

	if (viewport->isVisible())
	{
		if (isInWorkspace())
		{
			auto zb = b.removeFromLeft(18);
            b.removeFromRight(24);
			zb.removeFromBottom(viewport->getScrollBarThickness());
			zb.removeFromTop(SamplerDisplayWithTimeline::TimelineHeight);
			verticalZoomer->setBounds(zb);
			b.removeFromLeft(6);
		}
		else
		{
			verticalZoomer->setVisible(false);
		}

		viewport->setBounds(b);

		viewContent->setSize((int)(viewport->getWidth() * zoomFactor), viewport->getHeight() - (viewport->isHorizontalScrollBarShown() ? viewport->getScrollBarThickness() : 0));
	}
	else
		graph->setBounds(b);
}



void SampleEditor::addButton(CommandID commandId, bool hasState, const String& name, const String& offName /*= String()*/)
{
	SampleEditorToolbarFactory::Factory f;
	auto newButton = new HiseShapeButton(name, nullptr, f, offName);

	if (hasState)
		newButton->setToggleModeWithColourChange(true);

	newButton->setClickingTogglesState(false);
	newButton->setCommandToTrigger(samplerEditorCommandManager, commandId, true);
	menuButtons.add(newButton);
	addAndMakeVisible(newButton);
}

//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...

bool SampleEditor::perform (const InvocationInfo &info)
{

	switch(info.commandID)
	{
	case NormalizeVolume:  SampleEditHandler::SampleEditingActions::normalizeSamples(handler, this); return true;
	case LoopEnabled:	   {for(int i = 0; i < selection.size(); i++)
						   {
							   selection[i]->toggleBoolProperty(SampleIds::LoopEnabled);
						   };

						   const bool isOn = (selection.size() != 0) ? (bool)selection.getLast()->getSampleProperty(SampleIds::LoopEnabled) : false;

						   currentWaveForm->getSampleArea(SamplerSoundWaveform::LoopArea)->setVisible(isOn);
						   currentWaveForm->getSampleArea(SamplerSoundWaveform::LoopCrossfadeArea)->setVisible(isOn);
						   return true;
						   }
	case SelectWithMidi:   sampler->setEditorState(ModulatorSampler::MidiSelectActive, !sampler->getEditorState(ModulatorSampler::MidiSelectActive));
						   samplerEditorCommandManager->commandStatusChanged();
						   return true;
	case EnableSampleStartArea:	currentWaveForm->toggleRangeEnabled(SamplerSoundWaveform::SampleStartArea);
								return true;
	case EnableLoopArea:	currentWaveForm->toggleRangeEnabled(SamplerSoundWaveform::LoopArea); return true;
	case EnablePlayArea:	currentWaveForm->toggleRangeEnabled(SamplerSoundWaveform::PlayArea); return true;
	case ZoomIn:			zoom(false); return true;
	case ZoomOut:			zoom(true); return true;
	case Analyser:			toggleAnalyser(); return true;
	}
	return false;
}


void SampleEditor::toggleAnalyser()
{
	auto graphIsVisible = graph->isVisible();

	graph->setVisible(!graphIsVisible);
	viewport->setVisible(graphIsVisible);

	updateWaveform();
	resized();
}

void SampleEditor::paintOverChildren(Graphics &g)
{
	if (selection.size() != 0)
	{
		g.setColour(Colours::black.withAlpha(0.4f));

		const bool useGain = selection.getLast()->getNormalizedPeak() != 1.0f;

		String fileName = selection.getLast()->getPropertyAsString(SampleIds::FileName);

		PoolReference ref(sampler->getMainController(), fileName, FileHandlerBase::Samples);

		fileName = ref.getReferenceString();

		const String autogain = useGain ? ("Autogain: " + String(Decibels::gainToDecibels(selection.getLast()->getNormalizedPeak()), 1) + " dB") : String();

		int width = jmax<int>(GLOBAL_BOLD_FONT().getStringWidth(autogain), GLOBAL_BOLD_FONT().getStringWidth(fileName)) + 8;

		Rectangle<int> area; 
		
		if (isInWorkspace())
		{
			area = getLocalBounds().reduced(12).removeFromTop(24);
		}
		else
		{
			area = { viewport->getRight() - width - 1, viewport->getY() + 1, width, useGain ? 32 : 16 };
			g.fillRect(area);
		}
		
		g.setColour(Colours::white.withAlpha(0.7f));
		g.setFont(GLOBAL_BOLD_FONT());


		//g.drawText(fileName, area, Justification::topRight, false);

		//if (useGain)
		//	g.drawText(autogain, area, Justification::bottomRight, false);

		if (auto f = selection.getFirst())
		{
			if (f->isMissing())
			{
				g.setColour(Colours::black.withAlpha(0.4f));


				auto b = viewport->getBounds().toFloat();

				g.fillRect(b);

				g.setColour(Colours::white);
				g.drawText(f->getReferenceToSound()->getFileName(true) + " is missing", b, Justification::centred);
			}
		}

	}
}

void SampleEditor::updateWaveform()
{
	samplerEditorCommandManager->commandStatusChanged();

	if(viewport->isVisible())
		currentWaveForm->updateRanges();

	if (graph->isVisible())
		graphHandler.rebuildIfDirty();
    
    repaint();
}

//[/MiscUserCode]


//==============================================================================
#if 0
/*  -- Introjucer information section --

    This is where the Introjucer stores the metadata that describe this GUI layout, so
    make changes in here at your peril!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="SampleEditor" componentName=""
                 parentClasses="public Component, public SamplerSubEditor, public ApplicationCommandTarget, public AudioDisplayComponent::Listener, public SafeChangeListener"
                 constructorParams="ModulatorSampler *s, SamplerBody *b" variableInitialisers="sampler(s)"
                 snapPixels="8" snapActive="1" snapShown="0" overlayOpacity="0.330"
                 fixedSize="1" initialWidth="800" initialHeight="250">
  <BACKGROUND backgroundColour="6f6f6f">
    <RECT pos="8 8 175M 24" fill="solid: 13ffffff" hasStroke="1" stroke="1, mitered, butt"
          strokeColour="solid: fffffff"/>
    <TEXT pos="8Rr 4 232 30" fill="solid: 70ffffff" hasStroke="0" text="SAMPLE EDITOR"
          fontname="Arial" fontsize="20" bold="1" italic="0" justification="34"/>
  </BACKGROUND>
  <VIEWPORT name="new viewport" id="babb35bbad848ac" memberName="viewport"
            virtualName="" explicitFocusOrder="0" pos="8 41 16M 119" vscroll="0"
            hscroll="1" scrollbarThickness="18" contentType="0" jucerFile=""
            contentClass="" constructorParams=""/>
  <JUCERCOMP name="" id="d91d22eeed4f96dc" memberName="volumeSetter" virtualName=""
             explicitFocusOrder="0" pos="31.5%r 162 14% 32" sourceFile="ValueSettingComponent.cpp"
             constructorParams=""/>
  <JUCERCOMP name="" id="6dcb9c7b0159189f" memberName="pitchSetter" virtualName=""
             explicitFocusOrder="0" pos="61 195 10% 32" posRelativeX="d91d22eeed4f96dc"
             sourceFile="ValueSettingComponent.cpp" constructorParams=""/>
  <JUCERCOMP name="" id="dfa58ae047aa2ca6" memberName="sampleStartSetter"
             virtualName="" explicitFocusOrder="0" pos="4Cr 162 14% 32" sourceFile="ValueSettingComponent.cpp"
             constructorParams=""/>
  <JUCERCOMP name="" id="33421ae4da2b4051" memberName="sampleEndSetter" virtualName=""
             explicitFocusOrder="0" pos="0 196 14% 32" posRelativeX="dfa58ae047aa2ca6"
             sourceFile="ValueSettingComponent.cpp" constructorParams=""/>
  <JUCERCOMP name="" id="aecfa63b27e29f3e" memberName="loopStartSetter" virtualName=""
             explicitFocusOrder="0" pos="51.875% 163 14% 32" sourceFile="ValueSettingComponent.cpp"
             constructorParams=""/>
  <JUCERCOMP name="" id="55f3ee71f4c1fc08" memberName="loopEndSetter" virtualName=""
             explicitFocusOrder="0" pos="0 196 14% 32" posRelativeX="aecfa63b27e29f3e"
             sourceFile="ValueSettingComponent.cpp" constructorParams=""/>
  <JUCERCOMP name="" id="410604ce2de30fea" memberName="loopCrossfadeSetter"
             virtualName="" explicitFocusOrder="0" pos="0% 196 14% 32" posRelativeX="28c477b05f890ea4"
             sourceFile="ValueSettingComponent.cpp" constructorParams=""/>
  <JUCERCOMP name="" id="28c477b05f890ea4" memberName="startModulationSetter"
             virtualName="" explicitFocusOrder="0" pos="139C 160 14% 32" sourceFile="ValueSettingComponent.cpp"
             constructorParams=""/>
  <GENERICCOMPONENT name="new component" id="498417b33c43bc3c" memberName="toolbar"
                    virtualName="" explicitFocusOrder="0" pos="12 10 175M 20" class="Toolbar"
                    params=""/>
  <JUCERCOMP name="" id="d3e768374c58ef45" memberName="panSetter" virtualName=""
             explicitFocusOrder="0" pos="23.875%r 195 10% 32" sourceFile="ValueSettingComponent.cpp"
             constructorParams=""/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
} // namespace hise
//[/EndFile]
