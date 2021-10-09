
namespace hise { using namespace juce;

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

//==============================================================================
SampleEditor::SampleEditor (ModulatorSampler *s, SamplerBody *b):
	SamplerSubEditor(s->getSampleEditHandler()),
	sampler(s)
{
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
    
	addAndMakeVisible(overview);

    addAndMakeVisible(spectrumSlider);
    spectrumSlider.setRange(-1.0, 1.0, 0.0);
    spectrumSlider.setSliderStyle(Slider::SliderStyle::LinearHorizontal);
    spectrumSlider.setTextBoxStyle(Slider::TextEntryBoxPosition::NoTextBox, true, 0, 0);
	spectrumSlider.setLookAndFeel(&slaf);
	spectrumSlider.setValue(-1.0f, dontSendNotification);

	spectrumSlider.onValueChange = [this]()
    {
        auto nAlpha = spectrumSlider.getValue() * 0.5f + 0.5f;
		auto sAlpha = nAlpha;
        auto wAlpha = sAlpha;
        
        sAlpha = scriptnode::faders::overlap().getFadeValue<0>(2, sAlpha);
        wAlpha = scriptnode::faders::overlap().getFadeValue<1>(2, wAlpha);
        
		spectrumSlider.setColour(Slider::trackColourId, Colours::orange.withSaturation(wAlpha).withAlpha(0.5f));

        currentWaveForm->getThumbnail()->setSpectrumAndWaveformAlpha(sAlpha, wAlpha);
    };
	spectrumSlider.setColour(Slider::backgroundColourId, Colours::white.withAlpha(0.2f));
	spectrumSlider.setColour(Slider::trackColourId, Colours::orange.withSaturation(0.0f).withAlpha(0.5f));

	spectrumSlider.setColour(Slider::thumbColourId, Colour(0xFFDDDDDD));
	spectrumSlider.setTooltip("Blend spectrogram over waveform");

    addAndMakeVisible (panSetter = new ValueSettingComponent(s));

    viewContent = new SamplerDisplayWithTimeline();

	viewContent->addAndMakeVisible(currentWaveForm = new SamplerSoundWaveform(sampler));
    currentWaveForm->setIsSamplerWorkspacePreview();
    
	viewport->setViewedComponent(viewContent, false);
	viewport->setScrollBarThickness(13);

	fader.addScrollBarToAnimate(viewport->getHorizontalScrollBar());
	viewport->getHorizontalScrollBar().setLookAndFeel(&laf);
	viewport->getHorizontalScrollBar().addListener(this);

	addAndMakeVisible(verticalZoomer = new VerticalZoomer(currentWaveForm, sampler));

	currentWaveForm->setVisible(true);

	zoomFactor = 1.0f;

	body = b;

	addButton(SampleMapCommands::ZoomIn, false);
	addButton(SampleMapCommands::ZoomOut, false);
    addButton(SampleMapCommands::SelectWithMidi, true);
    
    analyseButton = addButton(SampleMapCommands::Analyser, true);
    
	addButton(SampleMapCommands::EnablePlayArea, true);
	addButton(SampleMapCommands::EnableSampleStartArea, true);
	addButton(SampleMapCommands::EnableLoopArea, true);
    addButton(SampleMapCommands::ZeroCrossing, false);
    improveButton = addButton(SampleMapCommands::ImproveLoopPoints, false);
    
    externalButton = addButton(SampleMapCommands::ExternalEditor, false);
    
    addButton(SampleMapCommands::NormalizeVolume, true);
    addButton(SampleMapCommands::LoopEnabled, true);
	
    
    
    addAndMakeVisible(sampleSelector = new ComboBox());
    addAndMakeVisible(multimicSelector = new ComboBox());
    
    sampleSelector->setLookAndFeel(&claf);
    multimicSelector->setLookAndFeel(&claf);
    
    sampleSelector->addListener(this);
    multimicSelector->addListener(this);
    
    claf.setDefaultColours(*sampleSelector);
    claf.setDefaultColours(*multimicSelector);
    
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

	viewport->setScrollOnDragEnabled(true);

    //[Constructor] You can add your own custom stuff here..

	sampleStartSetter->addChangeListener(this);
    sampleEndSetter->addChangeListener(this);
    loopStartSetter->addChangeListener(this);
    loopEndSetter->addChangeListener(this);
    loopCrossfadeSetter->addChangeListener(this);
    startModulationSetter->addChangeListener(this);

	currentWaveForm->addAreaListener(this);

	overview.setShouldScaleVertically(true);
	overview.setColour(AudioDisplayComponent::ColourIds::bgColour, Colour(0xFF333333));
    overview.setBufferedToImage(false);
    
    currentWaveForm->setColour(AudioDisplayComponent::ColourIds::bgColour, Colour(0xff1d1d1d));
    currentWaveForm->setColour(AudioDisplayComponent::ColourIds::outlineColour, Colour(0xff1d1d1d));
    
    startTimer(200);
    //[/Constructor]
}

String SampleEditor::getNameForCommand(SampleMapCommands c, bool on)
{
    switch(c)
    {
        case SampleMapCommands::ZoomIn: return on ? "zoom-in" : "";
        case SampleMapCommands::ZoomOut: return on ? "zoom-out" : "";
        case SampleMapCommands::Analyser: return on ? "analyse" : "";
        case SampleMapCommands::SelectWithMidi: return on ?"select-midi" : "select-mouse";
        case SampleMapCommands::EnablePlayArea: return on ? "play-area" : "";
        case SampleMapCommands::EnableSampleStartArea: return on ? "samplestart-area" : "";
        case SampleMapCommands::EnableLoopArea: return on ? "loop-area" : "";
        case SampleMapCommands::NormalizeVolume: return on ? "normalise-on" : "normalise-off";
        case SampleMapCommands::LoopEnabled: return on ? "loop-on" : "loop-off";
        case SampleMapCommands::ExternalEditor: return on ? "external" : "";
        case SampleMapCommands::ZeroCrossing: return on ? "zero" : "";
		case SampleMapCommands::ImproveLoopPoints: return on ? "improve-loop" : "";
        default: return "";
    }
}

SampleEditor::SampleMapCommands SampleEditor::getCommandIdForName(const String& n)
{
    for(int i = 0; i < (int)SampleMapCommands::numCommands; i++)
    {
        if(getNameForCommand((SampleMapCommands)i) == n)
            return (SampleMapCommands)i;
    }
    
    return {};
}

String SampleEditor::getTooltipForCommand(SampleMapCommands c)
{
    switch(c)
    {
        case SampleMapCommands::ZoomIn:                 return "Zoom in the waveform";
        case SampleMapCommands::ZoomOut:                return "Zoom out the waveform";
        case SampleMapCommands::Analyser:               return "Edit spectrogram properties";
        case SampleMapCommands::SelectWithMidi:         return "Enable MIDI selection";
        case SampleMapCommands::EnablePlayArea:         return "Enable Sample range editing";
        case SampleMapCommands::EnableSampleStartArea:  return "Enable SampleStartMod editing";
        case SampleMapCommands::EnableLoopArea:         return "Enable loop range editing";
        case SampleMapCommands::NormalizeVolume:        return "Normalise selected samples";
        case SampleMapCommands::LoopEnabled:            return "Enable looping for selection";
        case SampleMapCommands::ExternalEditor:         return "Open current sample selection in external audio editor";
        case SampleMapCommands::ZeroCrossing:           return "Enable zero crossing";
		case SampleMapCommands::ImproveLoopPoints:		return "Open the Loop Finder Popup";
            
        default: return "";
    }
}

bool SampleEditor::getState(SampleMapCommands c) const
{
    bool isSelected = !selection.isEmpty();
    
    switch(c)
    {
        case SampleMapCommands::ZoomIn:         return false;
        case SampleMapCommands::ZoomOut:        return false;
        case SampleMapCommands::Analyser:       return false;
        case SampleMapCommands::SelectWithMidi: return sampler->getEditorState(ModulatorSampler::MidiSelectActive);
        case SampleMapCommands::EnablePlayArea: return currentWaveForm->currentClickArea == SamplerSoundWaveform::AreaTypes::PlayArea;
        case SampleMapCommands::EnableSampleStartArea: return currentWaveForm->currentClickArea == SamplerSoundWaveform::AreaTypes::SampleStartArea;
        case SampleMapCommands::EnableLoopArea: return currentWaveForm->currentClickArea == SamplerSoundWaveform::AreaTypes::LoopArea;
        case SampleMapCommands::NormalizeVolume: return isSelected && (int)selection.getLast()->getSampleProperty(SampleIds::Normalized);
        case SampleMapCommands::LoopEnabled:    return isSelected && (int)selection.getLast()->getSampleProperty(SampleIds::LoopEnabled);
        case SampleMapCommands::ExternalEditor: return false;
        case SampleMapCommands::ZeroCrossing:   return currentWaveForm->zeroCrossing;
		case SampleMapCommands::ImproveLoopPoints: return false;
        default: return false;
    }
}

namespace LoopIcons
{
static const unsigned char apply[] = { 110,109,6,161,63,67,31,93,227,68,108,80,141,144,67,31,61,245,68,108,221,52,169,67,215,211,243,68,98,221,52,169,67,215,211,243,68,195,117,177,67,20,222,235,68,170,209,192,67,205,60,228,68,98,180,56,202,67,72,145,223,68,248,35,214,67,123,236,218,68,8,12,
229,67,174,199,216,68,108,162,213,241,67,246,240,214,68,108,219,105,229,67,184,94,207,68,108,33,160,216,67,113,53,209,68,98,195,37,197,67,61,2,212,68,53,174,180,67,51,211,217,68,31,101,168,67,195,237,223,68,98,121,41,161,67,113,133,227,68,207,87,155,
67,102,46,231,68,207,7,151,67,102,54,234,68,98,207,7,151,67,102,54,234,68,106,188,125,67,236,89,225,68,106,188,125,67,236,89,225,68,108,197,224,107,67,133,19,222,68,108,6,161,63,67,31,93,227,68,99,101,0,0 };

static const unsigned char find[] = { 110,109,106,44,20,68,225,154,169,68,108,119,246,35,68,225,154,169,68,98,147,224,39,68,0,136,181,68,88,233,58,68,205,236,190,68,61,226,82,68,225,178,192,68,108,61,226,82,68,215,147,200,68,98,141,71,50,68,205,180,198,68,174,79,24,68,236,217,185,68,106,
44,20,68,225,154,169,68,99,109,0,8,145,68,225,154,169,68,98,154,1,143,68,31,133,185,68,82,128,130,68,10,47,198,68,219,81,101,68,133,115,200,68,108,219,81,101,68,246,136,192,68,98,133,91,124,68,82,104,190,68,133,59,135,68,41,52,181,68,225,34,137,68,225,
154,169,68,108,0,8,145,68,225,154,169,68,99,109,47,181,54,68,225,154,169,68,108,193,50,71,68,225,154,169,68,98,90,132,73,68,225,34,172,68,113,181,77,68,20,38,174,68,61,226,82,68,133,51,175,68,108,61,226,82,68,123,92,183,68,98,236,17,69,68,174,207,181,
68,49,32,58,68,51,115,176,68,47,181,54,68,225,154,169,68,99,109,10,135,127,68,225,154,169,68,98,195,69,124,68,92,31,176,68,111,50,114,68,133,75,181,68,219,81,101,68,123,28,183,68,108,219,81,101,68,20,190,174,68,98,211,149,105,68,195,157,173,68,254,4,
109,68,102,206,171,68,104,9,111,68,225,154,169,68,108,10,135,127,68,225,154,169,68,99,109,61,226,82,68,92,135,129,68,108,61,226,82,68,82,104,137,68,98,57,252,58,68,205,44,139,68,250,254,39,68,41,132,148,68,16,0,36,68,51,99,160,68,108,231,51,20,68,51,
99,160,68,98,195,109,24,68,143,50,144,68,170,89,50,68,113,101,131,68,61,226,82,68,92,135,129,68,99,109,61,226,82,68,184,190,146,68,108,61,226,82,68,174,231,154,68,98,88,201,77,68,246,240,155,68,172,164,73,68,246,232,157,68,162,77,71,68,51,99,160,68,108,
215,195,54,68,51,99,160,68,98,217,62,58,68,164,152,153,68,129,37,69,68,246,72,148,68,61,226,82,68,184,190,146,68,99,109,219,81,101,68,102,254,146,68,98,141,31,114,68,31,205,148,68,82,40,124,68,123,236,153,68,82,120,127,68,51,99,160,68,108,135,238,110,
68,51,99,160,68,98,152,230,108,68,205,60,158,68,225,130,105,68,82,120,156,68,219,81,101,68,31,93,155,68,108,219,81,101,68,102,254,146,68,99,109,219,81,101,68,174,167,129,68,98,20,118,130,68,61,234,131,68,61,242,142,68,20,134,144,68,41,4,145,68,51,99,
160,68,108,20,30,137,68,51,99,160,68,98,41,44,135,68,174,215,148,68,82,72,124,68,72,177,139,68,219,81,101,68,61,146,137,68,108,219,81,101,68,174,167,129,68,99,101,0,0 };

static const unsigned char preview[] = { 110,109,164,232,154,68,188,68,221,67,108,215,131,181,68,242,34,134,67,108,215,131,181,68,217,78,68,68,108,164,232,154,68,244,189,24,68,108,246,248,132,68,244,189,24,68,108,246,248,132,68,188,68,221,67,108,164,232,154,68,188,68,221,67,99,101,0,0 };
}

struct LoopImproveWindow: public Component,
                          public ControlledObject,
                          public ComboBox::Listener,
                          public ButtonListener,
                          public SampleMap::Listener,
                          public PathFactory
{
    int getDisplaySize() const { return amountSlider.getValue(); }
    
    LoopImproveWindow(ModulatorSamplerSound* s, ModulatorSampler* sampler_):
      ControlledObject(sampler_->getMainController()),
      sound(s),
      sampler(sampler_),
      applyButton("apply", this, *this),
      previewButton("preview", this, *this),
      findButton("find", this, *this),
      dragger(this, nullptr),
	  findThread(*this)
    {
        addAndMakeVisible(dragger);
        
        addAndMakeVisible(loopStart);
        addAndMakeVisible(loopEnd);
        addAndMakeVisible(&amountSlider);
        
        addAndMakeVisible(applyButton);
        addAndMakeVisible(findButton);
        addAndMakeVisible(previewButton);
        
        
        previewButton.setToggleModeWithColourChange(true);
        
        sampler->getSampleMap()->addListener(this);
        
        addAndMakeVisible(sizeSelector);
        sizeSelector.setLookAndFeel(&claf);
        GlobalHiseLookAndFeel::setDefaultColours(sizeSelector);
        sizeSelector.addItemList({"256", "512", "1024", "2048", "4096", "8192", "16384" }, 1);
        sizeSelector.setText("1024", dontSendNotification);
        sizeSelector.addListener(this);
        
        amountSlider.setRange(256, 8192, 1.0);
        amountSlider.setSkewFactor(0.3f);
        amountSlider.setSliderStyle(Slider::SliderStyle::LinearHorizontal);
        amountSlider.setTextBoxStyle(Slider::TextEntryBoxPosition::NoTextBox, true, 0, 0);
        amountSlider.setLookAndFeel(&laf);
        amountSlider.setValue(1024.0, dontSendNotification);
        
        amountSlider.onValueChange = [this]()
        {
            this->refreshThumbnails();
        };
        
        amountSlider.setColour(Slider::backgroundColourId, Colours::white.withAlpha(0.2f));
        amountSlider.setColour(Slider::trackColourId, Colours::orange.withSaturation(0.0f).withAlpha(0.5f));

        amountSlider.setColour(Slider::thumbColourId, Colour(0xFFDDDDDD));
        amountSlider.setTooltip("Change waveform zoom");
        
        
        previewButton.setTooltip("Preview the current loop");
        applyButton.setTooltip("Write the current loop to the sample properties");
        sizeSelector.setTooltip("Select a zoom size");
        findButton.setTooltip("Find the best loop point");
        
        loopStart.setDisplayMode(HiseAudioThumbnail::DisplayMode::DownsampledCurve);
        loopEnd.setDisplayMode(HiseAudioThumbnail::DisplayMode::DownsampledCurve);
        
        loopStart.setColour(AudioDisplayComponent::ColourIds::bgColour, Colours::transparentBlack);
        loopStart.setColour(AudioDisplayComponent::ColourIds::fillColour, Colours::transparentBlack);
        loopStart.setColour(AudioDisplayComponent::ColourIds::outlineColour, Colours::red.withSaturation(0.43f).withAlpha(0.6f));
        loopEnd.setColour(AudioDisplayComponent::ColourIds::bgColour, Colours::transparentBlack);
        loopEnd.setColour(AudioDisplayComponent::ColourIds::fillColour, Colours::transparentBlack);
        loopEnd.setColour(AudioDisplayComponent::ColourIds::outlineColour, Colours::blue.withSaturation(0.43f).withAlpha(0.6f));
        
        setName("Loop Finder");
        setSize(600, 300);
        
        selectionChanged(*this, sound, 0);
    }
    
    ~LoopImproveWindow()
    {
        sampler->getSampleMap()->removeListener(this);
    }
    
    void mouseDown(const MouseEvent& e)
    {
        mouseDownLoop = currentLoop;
        dragStart = e.getPosition().getX() > getWidth() / 2;
    }
    
    void mouseDrag(const MouseEvent& e)
    {
        auto ds = (float)getDisplaySize();
        
        auto deltaNorm = (float)e.getDistanceFromDragStartX() / (float)getWidth();
        
        if(e.mods.isShiftDown())
            deltaNorm *= 0.25f;
        
        auto delta = roundToInt(deltaNorm * ds);
        
        if(dragStart)
            currentLoop.setStart(mouseDownLoop.getStart() - delta);
        else
            currentLoop.setEnd(mouseDownLoop.getEnd() - delta);
        
		unappliedChanges = true;

        refreshThumbnails();
    }
    
    Path createPath(const String& url) const override
    {
        Path p;
        LOAD_PATH_IF_URL("preview", LoopIcons::preview);
        LOAD_PATH_IF_URL("apply", LoopIcons::apply);
        LOAD_PATH_IF_URL("find", LoopIcons::find);
        
        return p;
    }
    
    void comboBoxChanged(ComboBox* cb) override
    {
        amountSlider.setValue(cb->getText().getIntValue(), sendNotificationSync);
    }
    
	Range<int> getLoopEdgeRange(bool getEnd)
	{
		auto DisplaySize = getDisplaySize();
		auto os = jmax(0, currentLoop.getStart() - DisplaySize / 2);
		auto nums = jmin(DisplaySize, fullBuffer.getNumSamples() - os);

		auto oe = jmax(0, currentLoop.getEnd() - DisplaySize / 2);
		auto nume = jmin(DisplaySize, fullBuffer.getNumSamples() - oe);

		if(getEnd)
			return { oe, oe + nume };
		else
			return { os, os + nums };
	}

	var getLoopEdgesAsChannelBuffers(Range<int> startRange = {}, Range<int> endRange = {})
	{
		if (sound == nullptr || !sound->getSampleProperty(SampleIds::LoopEnabled))
		{
			ok = false;
			return {};
		}

		ok = true;

		auto DisplaySize = getDisplaySize();

		VariantBuffer::Ptr ls = new VariantBuffer(DisplaySize);
		VariantBuffer::Ptr rs = new VariantBuffer(DisplaySize);

		VariantBuffer::Ptr le = new VariantBuffer(DisplaySize);
		VariantBuffer::Ptr re = new VariantBuffer(DisplaySize);

		if(startRange.isEmpty())
			startRange = getLoopEdgeRange(false);

		if(endRange.isEmpty())
			endRange = getLoopEdgeRange(true);

		auto copyTo = [&](VariantBuffer::Ptr dst, int channel, Range<int> r)
		{
			FloatVectorOperations::copy(dst->buffer.getWritePointer(0), fullBuffer.getReadPointer(channel, r.getStart()), r.getLength());
		};

		copyTo(ls, 0, startRange);
		copyTo(rs, 1, startRange);
		copyTo(le, 0, endRange);
		copyTo(re, 1, endRange);

		Array<var> channels;
		channels.add(var(ls));
		channels.add(var(rs));
		channels.add(var(le));
		channels.add(var(re));

		return channels;
	}

	struct FindThread : public Thread
	{
		FindThread(LoopImproveWindow& w) :
			Thread("Find best loop"),
			parent(w)
		{};

		void run() override
		{
			auto DisplaySize = parent.getDisplaySize();

			auto endRange = parent.getLoopEdgeRange(true);
			auto startRange = parent.getLoopEdgeRange(false);

			Array<ErrorStats> allStats;

			allStats.ensureStorageAllocated(DisplaySize);

			{
				ScopedLock sl(parent.fullBufferLock);

				for (int i = 0; i < DisplaySize; i++)
				{
					if (threadShouldExit())
						return;

					auto delta = -1 * DisplaySize / 2 + i;
					ErrorStats thisStats;
					thisStats.loopRange = parent.currentLoop;
					thisStats.loopRange.setStart(thisStats.loopRange.getStart() + delta);
					auto thisStart = startRange + delta;
					thisStats.calculate(parent.fullBuffer, thisStart, endRange, parent.getDisplaySize());
					allStats.add(thisStats);
				}
			}
			

			ErrorStats::Comparator comparator;
			allStats.sort(comparator);

			auto best = allStats.getFirst();

			auto r = best.loopRange;

			if (!threadShouldExit() && !r.isEmpty())
			{
				WeakReference<LoopImproveWindow> p = &parent;

				MessageManager::callAsync([r, p]()
				{
					p->currentLoop = r;
					p->unappliedChanges = true;
					p->refreshThumbnails();
					p->repaint();
				});
			}
		}

		LoopImproveWindow& parent;
	} findThread;

	struct ErrorStats
	{
		String toString() const
		{
			String s;
			s << "Average Diff: " << String(maxdiff, 1) << "dB, ";
			s << "Error: " << String(error, 1) << "dB";
			return s;
		}

		struct Comparator
		{
			static int compareElements(const ErrorStats& first, const ErrorStats& second)
			{
				auto firstScore = first.getScore();
				auto secondScore = second.getScore();

				if (firstScore == secondScore)
					return 0;
				else if (firstScore < secondScore)
					return 1;
				else
					return -1;
			}
		};

		float getScore() const
		{
			return -1.0f * error + -1.0f * maxdiff * 2.0f;
		}

		void calculate(AudioSampleBuffer& fullBuffer, Range<int> startRange, Range<int> endRange, int DisplaySize)
		{
			float diff[2] = { 0.0f };

			auto getSample = [&](bool getEnd, bool getRight, int sampleIndex)
			{
				auto index = (getEnd ? endRange.getStart() : startRange.getStart()) + sampleIndex;

                if(isPositiveAndBelow(index, fullBuffer.getNumSamples()))
                    return fullBuffer.getSample((int)getRight, index);
                
                return 0.0f;
			};

			for (int i = DisplaySize / 4; i < DisplaySize * 3 / 4; i++)
			{
				float window = (float)(i - DisplaySize / 4) / (float)(DisplaySize / 4);

				if (i > DisplaySize / 2)
					window = 2.0f - window;

				diff[0] += window * std::abs(getSample(false, false, i) - getSample(true, false, i));
				diff[1] += window * std::abs(getSample(false, true, i) - getSample(true, true, i));
			}

			auto p2le = getSample(true, false, DisplaySize / 2 - 2);
			auto p1le = getSample(true, false, DisplaySize / 2 - 1);
			auto p2re = getSample(true, true, DisplaySize / 2 - 2);
			auto p1re = getSample(true, true, DisplaySize / 2 - 1);

            auto p2ls = getSample(false, false, DisplaySize / 2 - 2);
            auto p1ls = getSample(false, false, DisplaySize / 2 - 1);
            auto p2rs = getSample(false, true, DisplaySize / 2 - 2);
            auto p1rs = getSample(false, true, DisplaySize / 2 - 1);
            
            auto n2le = getSample(true, false, DisplaySize / 2 + 2);
            auto n1le = getSample(true, false, DisplaySize / 2 + 1);
            auto n2re = getSample(true, true, DisplaySize / 2 + 2);
            auto n1re = getSample(true, true, DisplaySize / 2 + 1);

            auto n2ls = getSample(false, false, DisplaySize / 2 + 2);
            auto n1ls = getSample(false, false, DisplaySize / 2 + 1);
            auto n2rs = getSample(false, true, DisplaySize / 2 + 2);
            auto n1rs = getSample(false, true, DisplaySize / 2 + 1);
            
            auto errL = std::abs(p2le - p2ls) + std::abs(p1le - p2ls) +
                        std::abs(n1le - n1ls) + std::abs(n2le - n2ls);
            
            auto errR = std::abs(p2re - p2rs) + std::abs(p1re - p2rs) +
                        std::abs(n1re - n1rs) + std::abs(n2re - n2rs);
            
            
			auto expectedl = p1le + (p1le - p2le);
			auto expectedr = p1re + (p1re - p2re);

			auto actuall = getSample(false, false, DisplaySize / 2);
			auto actualr = getSample(false, true, DisplaySize / 2);

			error = jmax(std::abs(expectedl - actuall), std::abs(expectedr - actualr));
            
            error = jmax(errL, errR);
			error = Decibels::gainToDecibels(error);

			diff[0] /= (float)(DisplaySize / 4);
			diff[1] /= (float)(DisplaySize / 4);

			maxdiff = jmax(diff[0], diff[1]);
			maxdiff = Decibels::gainToDecibels(maxdiff);
		}

		Range<int> loopRange;
		float maxdiff = 0.0f;
		float error = 0.0f;
	};

	void refreshThumbnails()
	{
		ScopedLock sl(fullBufferLock);
		auto channels = getLoopEdgesAsChannelBuffers();

		if (channels.size() != 4)
		{
			loopStart.clear();
			loopEnd.clear();
			statistics = {};

			repaint();
			return;
		}

		auto startRange = getLoopEdgeRange(false);
		auto endRange = getLoopEdgeRange(true);

		auto DisplaySize = getDisplaySize();

		auto maxS = 1.0f / jmax(0.001f, fullBuffer.getMagnitude(startRange.getStart(), startRange.getLength()));
		auto maxE = 1.0f / jmax(0.001f, fullBuffer.getMagnitude(endRange.getStart(), endRange.getLength()));

		statistics.loopRange = currentLoop;
		statistics.calculate(fullBuffer, startRange, endRange, DisplaySize);

		auto gain = jmin(maxS, maxE);

        loopStart.setDisplayGain(gain, dontSendNotification);
        loopEnd.setDisplayGain(gain, dontSendNotification);
		loopStart.setBuffer(channels[0], channels[1]);
		loopEnd.setBuffer(channels[2], channels[3]);
        
        repaint();
    }
    
    void sampleMapWasChanged(PoolReference newSampleMap) {}

    void samplePropertyWasChanged(ModulatorSamplerSound* s, const Identifier& id, const var& newValue) override
    {
        if(s == sound.get())
        {
            if(id == SampleIds::LoopStart || id == SampleIds::LoopEnd || id == SampleIds::LoopEnabled)
            {
                refreshThumbnailAndRanges();
            }
        }
    };
    
    static void selectionChanged(LoopImproveWindow& w, ModulatorSamplerSound::Ptr newSound, int micIndex)
    {
		
        w.sound = newSound;
        
        if(newSound == nullptr)
        {
            w.fullBuffer = {};
            w.refreshThumbnailAndRanges();
            return;
        }
        
        StreamingSamplerSound::Ptr ss = newSound->getReferenceToSound(micIndex);

        if (ss == nullptr)
        {
            jassertfalse;
            return;
        }

        ScopedPointer<AudioFormatReader> afr;

        if (ss->isMonolithic())
            afr = ss->createReaderForPreview();
        else
            afr = PresetHandler::getReaderForFile(ss->getFileName(true));

        if (afr != nullptr)
        {
			ScopedLock sl(w.fullBufferLock);
            w.fullBuffer.setSize(2, afr->lengthInSamples);
            afr->read(&w.fullBuffer, 0, afr->lengthInSamples, 0, true, true);
        }
        
        w.refreshThumbnailAndRanges();
    }
    
    void refreshThumbnailAndRanges()
    {
        if(sound != nullptr)
        {
            originalLoop = { (int)sound->getSampleProperty(SampleIds::LoopStart), (int)sound->getSampleProperty(SampleIds::LoopEnd)};
            currentLoop = originalLoop;
        }
        else
        {
            originalLoop = {};
            currentLoop = {};
        }
        
        refreshThumbnails();
    }
    
    void mouseWheelMove(const MouseEvent& e, const MouseWheelDetails& d) override
    {
        amountSlider.mouseWheelMove(e, d);
    }
    
    void buttonClicked(Button* b) override
    {
        if(b == &previewButton && fullBuffer.getNumSamples() > 0)
        {
			int PreviewLength = 44100 * 2;

			AudioSampleBuffer b(2, PreviewLength);

			auto useMultipleLoops = currentLoop.getLength() < (PreviewLength / 2);

			if (useMultipleLoops)
			{
				auto l = fullBuffer.getReadPointer(0, currentLoop.getStart());
				auto r = fullBuffer.getReadPointer(0, currentLoop.getStart());

				for (int i = 0; i < PreviewLength; i += currentLoop.getLength())
				{
					auto numToCopy = jmin(currentLoop.getLength(), PreviewLength - i);
					b.copyFrom(0, i, fullBuffer, 0, currentLoop.getStart(), numToCopy);
					b.copyFrom(1, i, fullBuffer, 1, currentLoop.getStart(), numToCopy);
				}
			}
			else
			{
				auto o1 = currentLoop.getEnd() - (PreviewLength / 2);
				
				b.copyFrom(0, 0, fullBuffer, 0, o1, PreviewLength / 2);
				b.copyFrom(1, 0, fullBuffer, 1, o1, PreviewLength / 2);

				b.copyFrom(0, PreviewLength / 2, fullBuffer, 0, currentLoop.getStart(), PreviewLength / 2);
				b.copyFrom(1, PreviewLength / 2, fullBuffer, 1, currentLoop.getStart(), PreviewLength / 2);
			}

			b.applyGainRamp(0, 1024, 0.0f, 1.0f);
			b.applyGainRamp(PreviewLength - 1024, 1024, 1.0f, 0.0f);

            getMainController()->setBufferToPlay(b);
        }
        if(b == &findButton)
        {
			findThread.stopThread(1000);
			findThread.startThread();
			repaint();
        }
        if(b == &applyButton && sound != nullptr)
        {
            sound->setSampleProperty(SampleIds::LoopStart, currentLoop.getStart(), true);
            sound->setSampleProperty(SampleIds::LoopEnd, currentLoop.getEnd(), true);
			unappliedChanges = false;
			repaint();
        }
    }
    
    void resized() override
    {
        auto b = getLocalBounds();
        auto top = b.removeFromTop(24);
        applyButton.setBounds(top.removeFromRight(24).reduced(2));
        
        previewButton.setBounds(top.removeFromRight(24).reduced(2));
        
        findButton.setBounds(top.removeFromRight(24).reduced(2));
        
        auto bottom = b.removeFromBottom(24);
        
        dragger.setBounds(bottom.removeFromRight(15).removeFromBottom(15));
        
        sizeSelector.setBounds(bottom.removeFromLeft(150));
        
        amountSlider.setBounds(bottom);
        
        b.removeFromBottom(10);
        
        loopStart.setBounds(b);//.removeFromTop(b.getHeight()/2));
        loopEnd.setBounds(b);
        
        loopStart.setRange(getWidth()/2, getWidth());
        loopEnd.setRange(0, getWidth() / 2);
    }
    
    void paint(Graphics& g) override
    {
		if (findThread.isThreadRunning())
		{
			g.setColour(Colours::white.withAlpha(0.6f));
			g.setFont(GLOBAL_BOLD_FONT());
			g.drawText("Searching best loop point in area", getLocalBounds().toFloat(), Justification::centred);
			return;
		}

        auto b = getLocalBounds().toFloat();
        
        b.removeFromTop(24);
        
        
        
        g.setColour(Colours::white.withAlpha(0.2f));
        g.drawVerticalLine(getWidth() / 2, 28, getHeight() - 32.0f);
        
        auto lb = loopStart.getBounds();
        
        g.setColour(Colours::black.withAlpha(0.2f));
        g.fillRoundedRectangle(lb.toFloat(), 3.0f);
        
        g.drawHorizontalLine(lb.getY() + lb.getHeight() / 4, 0.0f, (float)getWidth());
        g.drawHorizontalLine(lb.getY() + 3 * lb.getHeight() / 4, 0.0f, (float)getWidth());
        
        g.setFont(GLOBAL_BOLD_FONT());
        g.setColour(Colours::white.withAlpha(0.9f));
        
        if(!ok)
        {
			if(sound == nullptr)
				g.drawText("No sample selected", getLocalBounds().toFloat(), Justification::centred);
			else
				g.drawText("Loop is disabled for current sample", getLocalBounds().toFloat(), Justification::centred);
            return;
        }
        
		String s;
		s << statistics.toString();
		s << (unappliedChanges ? "*" : "");

        g.drawText(s, getLocalBounds().toFloat().removeFromTop(24.0f).reduced(10.0f, 0.0f), Justification::centredLeft);
    }
    
    bool ok;
    
	ErrorStats statistics;

	CriticalSection fullBufferLock;
    
    AudioSampleBuffer fullBuffer;
    
    HiseAudioThumbnail loopStart;
    HiseAudioThumbnail loopEnd;
    
    HiseShapeButton applyButton;
    HiseShapeButton findButton;
    HiseShapeButton previewButton;
    
    Range<int> originalLoop;
    Range<int> currentLoop;
    Range<int> mouseDownLoop;
    bool dragStart = false;
	bool unappliedChanges = false;
    
    Slider amountSlider;
    ModulatorSamplerSound::Ptr sound;
    
    ComboBox sizeSelector;
    GlobalHiseLookAndFeel claf;
    
    WeakReference<ModulatorSampler> sampler;
    
    LookAndFeel_V4 laf;
    
    juce::ResizableCornerComponent dragger;
    
    JUCE_DECLARE_WEAK_REFERENCEABLE(LoopImproveWindow);
};

void SampleEditor::perform(SampleMapCommands c)
{
    switch(c)
    {
    case SampleMapCommands::NormalizeVolume:  SampleEditHandler::SampleEditingActions::normalizeSamples(handler, this); return;
    case SampleMapCommands::LoopEnabled:
    {
        for(int i = 0; i < selection.size(); i++)
        {
           selection[i]->toggleBoolProperty(SampleIds::LoopEnabled);
        };

        const bool isOn = (selection.size() != 0) ? (bool)selection.getLast()->getSampleProperty(SampleIds::LoopEnabled) : false;

        currentWaveForm->getSampleArea(SamplerSoundWaveform::LoopArea)->setVisible(isOn);
        currentWaveForm->getSampleArea(SamplerSoundWaveform::LoopCrossfadeArea)->setVisible(isOn);
        return;
    }
    case SampleMapCommands::SelectWithMidi:   sampler->setEditorState(ModulatorSampler::MidiSelectActive, !sampler->getEditorState(ModulatorSampler::MidiSelectActive));
        return;
    case SampleMapCommands::EnableSampleStartArea:
        currentWaveForm->setClickArea(SamplerSoundWaveform::SampleStartArea);
        return;
    case SampleMapCommands::EnableLoopArea:
        currentWaveForm->setClickArea(SamplerSoundWaveform::LoopArea);
        return;
    case SampleMapCommands::EnablePlayArea:
        currentWaveForm->setClickArea(SamplerSoundWaveform::PlayArea);
        return;
    case SampleMapCommands::ZeroCrossing:
        currentWaveForm->zeroCrossing = !currentWaveForm->zeroCrossing;
        return;
    case SampleMapCommands::ZoomIn:
        zoom(false);
        return;
    case SampleMapCommands::ZoomOut:
        zoom(true);
        return;
    case SampleMapCommands::Analyser:
    {
        auto n = new Spectrum2D::Parameters::Editor(currentWaveForm->getThumbnail()->getSpectrumParameters());
        findParentComponentOfClass<FloatingTile>()->getRootFloatingTile()->showComponentAsDetachedPopup(n, analyseButton, {8, 16});
        return;
    }
	case SampleMapCommands::ImproveLoopPoints:
	{
        auto n = new LoopImproveWindow(const_cast<ModulatorSamplerSound*>(currentWaveForm->getCurrentSound()), sampler);
        
        handler->selectionBroadcaster.addListener(*n, LoopImproveWindow::selectionChanged);
        
        findParentComponentOfClass<FloatingTile>()->getRootFloatingTile()->showComponentAsDetachedPopup(n, improveButton, {8, 16});
        
        return;
	}
    case SampleMapCommands::ExternalEditor:
    {
        if (sampler->getSampleMap()->isMonolith())
        {
            PresetHandler::showMessageWindow("Monolith file", "You can't edit a monolith file", PresetHandler::IconType::Error);
            return;
        }

        if (sampler->getSampleMap()->isUsingUnsavedValueTree())
        {
            PresetHandler::showMessageWindow("Unsaved samplemap", "You need to save your samplemap to a file before starting the external editor.  \n> This is required so that you can reload the map after editing");
            return;
        }

        auto fp = GET_HISE_SETTING(sampler, HiseSettings::Other::ExternalEditorPath).toString();

        if (fp.isEmpty())
        {
            PresetHandler::showMessageWindow("No external editor specified", "You need to set an audio editor you want to use for this operation.  \n> Settings -> Other -> ExternalEditorPath", PresetHandler::IconType::Error);

            return;
        }

        File editor(fp);

        String args;

        Array<File> editedFiles;

        for (auto s : selection)
        {
            for (int i = 0; i < s->getNumMultiMicSamples(); i++)
            {
                editedFiles.add(File(s->getReferenceToSound(i)->getFileName(true)));
                s->getReferenceToSound(i)->closeFileHandle();
            }
        }

#if JUCE_WINDOWS
        for(auto& f: editedFiles)
            args << "\"" << f.getFullPathName() << "\" ";
#else
        for(auto& f: editedFiles)
            args << f.getFullPathName() << " ";
#endif

        externalWatcher = new ExternalFileChangeWatcher(sampler, editedFiles);
        editor.startAsProcess(args);

        return;
    }
    }
    return;
}

SampleEditor::~SampleEditor()
{
	if (sampler != nullptr)
	{
		sampler->getSampleMap()->removeListener(this);
	}
    

    viewport = nullptr;
    volumeSetter = nullptr;
    pitchSetter = nullptr;
    sampleStartSetter = nullptr;
    sampleEndSetter = nullptr;
    loopStartSetter = nullptr;
    loopEndSetter = nullptr;
    loopCrossfadeSetter = nullptr;
    startModulationSetter = nullptr;
    panSetter = nullptr;
}

void SampleEditor::scrollBarMoved(ScrollBar* scrollBarThatHasMoved, double newRangeStart)
{
	auto s = scrollBarThatHasMoved->getCurrentRange();

	auto start = s.getStart();
	auto end = s.getEnd();

	start /= (double)jmax(1, currentWaveForm->getWidth());
	end /= (double)jmax(1, currentWaveForm->getWidth());

	overview.setRange(start * overview.getWidth(), end * overview.getWidth());
}

void SampleEditor::samplePropertyWasChanged(ModulatorSamplerSound* s, const Identifier& id, const var& /*newValue*/)
{
	if (s == currentWaveForm->getCurrentSound() && SampleIds::Helpers::isAudioProperty(id))
	{
		currentWaveForm->updateRanges();
	}
		
}


void SampleEditor::refreshDisplayFromComboBox()
{
	auto idx = sampleSelector->getSelectedItemIndex();

	if (auto s = selection[idx])
	{
		handler->selectionBroadcaster.sendMessage(sendNotification, s, multimicSelector->getSelectedItemIndex());

		currentWaveForm->setSoundToDisplay(s);
	}
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
    
    
	externalButton->setBounds(topBar.removeFromRight(topBar.getHeight()).reduced(3));

    multimicSelector->setBounds(topBar.removeFromRight(100));
    sampleSelector->setBounds(topBar.removeFromRight(300));
    
	topBar.removeFromRight(10);
	analyseButton->setBounds(topBar.removeFromRight(topBar.getHeight()).reduced(3));
    spectrumSlider.setBounds(topBar.removeFromRight(100).reduced(2));
	

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

	if (isInWorkspace())
	{
		auto zb = b.removeFromLeft(18);
		b.removeFromRight(24);
		zb.removeFromBottom(viewport->getScrollBarThickness());
		
		zb.removeFromTop(32);
		zb.removeFromTop(SamplerDisplayWithTimeline::TimelineHeight);
		verticalZoomer->setBounds(zb);
		b.removeFromLeft(6);

		overview.setBounds(b.removeFromTop(32));

	}
	else
	{
		verticalZoomer->setVisible(false);

		overview.setVisible(false);

	}

	viewport->setBounds(b);

	viewContent->setSize((int)(viewport->getWidth() * zoomFactor), viewport->getHeight() - (viewport->isHorizontalScrollBarShown() ? viewport->getScrollBarThickness() : 0));
}



Component* SampleEditor::addButton(SampleMapCommands commandId, bool hasState)
{
	SampleEditorToolbarFactory::Factory f;
    auto newButton = new HiseShapeButton(getNameForCommand(commandId, true), nullptr, f, getNameForCommand(commandId, false));

	if (hasState)
		newButton->setToggleModeWithColourChange(true);

    newButton->setTooltip(getTooltipForCommand(commandId));
	newButton->setClickingTogglesState(false);
	
    newButton->onClick = [commandId, this]()
    {
        perform(commandId);
    };
    
	menuButtons.add(newButton);
	addAndMakeVisible(newButton);
	return newButton;
}


void SampleEditor::soundsSelected(const SampleSelection &selectedSoundList)
{
    selection.clear();

	for (int i = 0; i < selectedSoundList.size(); i++)
		selection.add(selectedSoundList[i]);

	panSetter->setCurrentSelection(selectedSoundList);
	volumeSetter->setCurrentSelection(selectedSoundList);
	pitchSetter->setCurrentSelection(selectedSoundList);
	sampleStartSetter->setCurrentSelection(selectedSoundList);
	sampleEndSetter->setCurrentSelection(selectedSoundList);
	startModulationSetter->setCurrentSelection(selectedSoundList);
	loopStartSetter->setCurrentSelection(selectedSoundList);
	loopEndSetter->setCurrentSelection(selectedSoundList);
	loopCrossfadeSetter->setCurrentSelection(selectedSoundList);

	if (selectedSoundList.size() != 0 && selectedSoundList.getLast() != nullptr)
	{
		auto ms = selectedSoundList.getLast();

		auto micIndex = jlimit<int>(0, ms->getNumMultiMicSamples() - 1, multimicSelector->getSelectedItemIndex());

		currentWaveForm->setSoundToDisplay(ms, micIndex);

		handler->selectionBroadcaster.sendMessage(sendNotificationSync, ms, micIndex);
        
		auto sound = ms->getReferenceToSound(micIndex);

		ScopedPointer<AudioFormatReader> afr;

		if (sound->isMonolithic())
		{
			afr = sound->createReaderForPreview();
		}
		else
		{
			afr = PresetHandler::getReaderForFile(sound->getFileName(true));
		}

		overview.setReader(afr.release());
	}
	else
	{
		handler->selectionBroadcaster.sendMessage(sendNotificationSync, nullptr, 0);

		currentWaveForm->setSoundToDisplay(nullptr);
		overview.setReader(nullptr);
	}

	sampleSelector->clear(dontSendNotification);
	multimicSelector->clear(dontSendNotification);
	int sampleIndex = 1;

	for (auto s : selectedSoundList)
	{
		sampleSelector->addItem(s->getSampleProperty(SampleIds::FileName).toString().replace("{PROJECT_FOLDER}", ""), sampleIndex++);
	}

	sampleSelector->setSelectedId(selectedSoundList.size(), dontSendNotification);

	auto micPositions = StringArray::fromTokens(sampler->getStringForMicPositions(), ";", "");
	micPositions.removeEmptyStrings();

	int micIndex = 1;

	for (auto t : micPositions)
	{
		multimicSelector->addItem(t, micIndex++);
	}

	multimicSelector->setTextWhenNothingSelected("No multimics");
	multimicSelector->setTextWhenNoChoicesAvailable("No multimics");

	updateWaveform();
    
    
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
	if(viewport->isVisible())
		currentWaveForm->updateRanges();

    repaint();
}

void SampleEditor::zoom(bool zoomOut, int mousePos/*=0*/)
{
	auto oldPos = (double)(viewport->getViewPositionX());
    auto oldMousePos = oldPos + mousePos;
    auto oldWidth = (double)currentWaveForm->getWidth();
    auto normDelta = mousePos / oldWidth;
    auto normPos = (double)oldPos / (double)currentWaveForm->getWidth();
    auto oldMousePosNorm = normDelta + normPos;
    
#if JUCE_WINDOWS || JUCE_LINUX
	auto factor = 1.25f;
#else
	auto factor = 1.125f;
#endif

	if (!zoomOut)
		zoomFactor = jmin(128.0f, zoomFactor * factor);
	else
		zoomFactor = jmax(1.0f, zoomFactor / factor);
    
	resized();

	scrollBarMoved(&viewport->getHorizontalScrollBar(), 0.0f);

	auto newWidth = (double)viewport->getViewedComponent()->getWidth();
    
    auto newMousePos = oldMousePosNorm * newWidth;
    auto newPos = newMousePos - mousePos;
    
    
	viewport->setViewPosition(roundToInt(newPos), 0.0);
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
