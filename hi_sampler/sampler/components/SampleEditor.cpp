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

    //[UserPreSize]

	viewContent = new SamplerDisplayWithTimeline();

	viewContent->addAndMakeVisible(currentWaveForm = new SamplerSoundWaveform(sampler));

	viewport->setViewedComponent(viewContent, false);
	viewport->setScrollBarThickness(13);

	fader.addScrollBarToAnimate(viewport->getHorizontalScrollBar());
	viewport->getHorizontalScrollBar().setLookAndFeel(&laf);
	viewport->getHorizontalScrollBar().addListener(this);

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
    analyseButton = addButton(SampleMapCommands::Analyser, true, "analyse");
    
	addButton(SampleMapCommands::SelectWithMidi, true, "select-midi", "select-mouse");
	addButton(SampleMapCommands::EnablePlayArea, true, "play-area");
	addButton(SampleMapCommands::EnableSampleStartArea, true, "samplestart-area");
	addButton(SampleMapCommands::EnableLoopArea, true, "loop-area");
    addButton(SampleMapCommands::NormalizeVolume, true, "normalise-on", "normalise-off");
    addButton(SampleMapCommands::LoopEnabled, true, "loop-on", "loop-off");
	externalButton = addButton(SampleMapCommands::ExternalEditor, false, "external");
    
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



Component* SampleEditor::addButton(CommandID commandId, bool hasState, const String& name, const String& offName /*= String()*/)
{
	SampleEditorToolbarFactory::Factory f;
	auto newButton = new HiseShapeButton(name, nullptr, f, offName);

	if (hasState)
		newButton->setToggleModeWithColourChange(true);

	newButton->setClickingTogglesState(false);
	newButton->setCommandToTrigger(samplerEditorCommandManager, commandId, true);
	menuButtons.add(newButton);
	addAndMakeVisible(newButton);
	return newButton;
}

//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...

struct ExternalFileChangeWatcher : public Timer,
								   public SampleMap::Listener
{
	ExternalFileChangeWatcher(ModulatorSampler* s, Array<File> fileList_):
		fileList(fileList_),
		sampler(s)
	{
		startTimer(1000);

		sampler->getSampleMap()->addListener(this);

		for (const File& f : fileList)
			modificationTimes.add(f.getLastModificationTime());
	}

	virtual void sampleMapWasChanged(PoolReference newSampleMap)
	{
		stopTimer();
		sampler->getSampleMap()->removeListener(this);
	}

	virtual void sampleMapCleared()
	{
		stopTimer();
		sampler->getSampleMap()->removeListener(this);
	};

	void timerCallback() override
	{
		for (int i = 0; i < fileList.size(); i++)
		{
			auto t = fileList[i].getLastModificationTime();

			if (t != modificationTimes[i])
			{
				stopTimer();

				if (PresetHandler::showYesNoWindow("Detected File change", "Press OK to reload the samplemap"))
				{
					sampler->getSampleMap()->saveAndReloadMap();
				}
				
				modificationTimes.clear();

				for (const auto& l : fileList)
					modificationTimes.add(l.getLastModificationTime());
				
				startTimer(1000);
			}
		}
	}

	WeakReference<ModulatorSampler> sampler;
	const Array<File> fileList;
	Array<Time> modificationTimes;
};

void SampleEditor::getCommandInfo(CommandID commandID, ApplicationCommandInfo &result)
{
	const bool isSelected = selection.size() > 0 && selection.getLast() != nullptr;

	switch (commandID)
	{
	case ZoomIn:			result.setInfo("Zoom In", "Zoom in the sample map", "Zooming", 0);
		result.addDefaultKeypress('+', ModifierKeys::commandModifier);
		result.setActive(isSelected && zoomFactor != 16.0f);
		break;
	case ZoomOut:			result.setInfo("Zoom Out", "Zoom out the sample map", "Zooming", 0);
		result.addDefaultKeypress('-', ModifierKeys::commandModifier);
		result.setActive(isSelected && zoomFactor != 1.0f);
		break;
	case EnableSampleStartArea:	result.setInfo("Change SampleStart", "Change SampleStart", "Areas", 0);
		result.setActive(true);
		result.setTicked(currentWaveForm->currentClickArea == SamplerSoundWaveform::AreaTypes::SampleStartArea);
		break;
	case EnableLoopArea:	result.setInfo("Change loop range", "Change loop range", "Areas", 0);
		result.setActive(true);
		result.setTicked(currentWaveForm->currentClickArea == SamplerSoundWaveform::AreaTypes::LoopArea);
		break;
	case EnablePlayArea:	result.setInfo("Change sample range", "Change sample range", "Areas", 0);
		result.setActive(true);
		result.setTicked(currentWaveForm->currentClickArea == SamplerSoundWaveform::AreaTypes::PlayArea);
		break;
	case SelectWithMidi:	result.setInfo("Midi Select", "Autoselect the most recently triggered sound", "Tools", 0);
		result.setActive(true);
		result.setTicked(!sampler->getEditorState(ModulatorSampler::MidiSelectActive));
		break;
	case NormalizeVolume:	result.setInfo("Normalize Volume", "Normalize the sample volume to 0dB", "Properties", 0);
		result.setActive(isSelected);
		result.setTicked(isSelected && (int)selection.getLast()->getSampleProperty(SampleIds::Normalized));
		break;
	case LoopEnabled:		result.setInfo("Loop Enabled", "Enable Loop Playback", "Properties", 0);
		result.setActive(isSelected);
		result.setTicked(isSelected && (int)selection.getLast()->getSampleProperty(SampleIds::LoopEnabled));
		break;
	case Analyser:			result.setInfo("Show Spectrogram properties", "Edit Spectrogram properties", "Properties", 0);
		result.setActive(isSelected);
		break;
	case ExternalEditor:    result.setInfo("Edit in external editor", "External Editor", "Properties", 0);
		result.setActive(isSelected);
		break;
	}
}

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
	case EnableSampleStartArea:	currentWaveForm->setClickArea(SamplerSoundWaveform::SampleStartArea);
		samplerEditorCommandManager->commandStatusChanged();
								return true;
	case EnableLoopArea:	currentWaveForm->setClickArea(SamplerSoundWaveform::LoopArea);
		samplerEditorCommandManager->commandStatusChanged();  return true;
	case EnablePlayArea:	currentWaveForm->setClickArea(SamplerSoundWaveform::PlayArea);
		samplerEditorCommandManager->commandStatusChanged(); return true;
	case ZoomIn:			zoom(false); return true;
	case ZoomOut:			zoom(true); return true;
	case Analyser:			
	{
		auto n = new Spectrum2D::Parameters::Editor(currentWaveForm->getThumbnail()->getSpectrumParameters());
		findParentComponentOfClass<FloatingTile>()->getRootFloatingTile()->showComponentAsDetachedPopup(n, analyseButton, {8, 16});
		return true;
	}
	case ExternalEditor:
	{
		if (sampler->getSampleMap()->isMonolith())
		{
			PresetHandler::showMessageWindow("Monolith file", "You can't edit a monolith file", PresetHandler::IconType::Error);
			return true;
		}

		if (sampler->getSampleMap()->isUsingUnsavedValueTree())
		{
			PresetHandler::showMessageWindow("Unsaved samplemap", "You need to save your samplemap to a file before starting the external editor.  \n> This is required so that you can reload the map after editing");
			return true;
		}

		auto fp = GET_HISE_SETTING(sampler, HiseSettings::Other::ExternalEditorPath).toString();

		if (fp.isEmpty())
		{
			PresetHandler::showMessageWindow("No external editor specified", "You need to set an audio editor you want to use for this operation.  \n> Settings -> Other -> ExternalEditorPath", PresetHandler::IconType::Error);

			return true;
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

		return true;
	}
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

	samplerEditorCommandManager->commandStatusChanged();

	if (selectedSoundList.size() != 0 && selectedSoundList.getLast() != nullptr)
	{
		auto ms = selectedSoundList.getLast();

		auto micIndex = jlimit<int>(0, ms->getNumMultiMicSamples() - 1, multimicSelector->getSelectedItemIndex());

		currentWaveForm->setSoundToDisplay(ms, micIndex);

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
	samplerEditorCommandManager->commandStatusChanged();

	if(viewport->isVisible())
		currentWaveForm->updateRanges();

	if (graph->isVisible())
		graphHandler.rebuildIfDirty();
    
    repaint();
}

void SampleEditor::zoom(bool zoomOut, int mousePos/*=0*/)
{
	auto oldPos = (float)(viewport->getViewPositionX() + mousePos);
	oldPos /= (double)jmax(1, viewport->getViewedComponent()->getWidth());



	if (!zoomOut)
	{
		zoomFactor = jmin(128.0f, zoomFactor * 1.25f); resized();
	}
	else
	{
		zoomFactor = jmax(1.0f, zoomFactor / 1.25f); resized();
	}

	resized();

	scrollBarMoved(&viewport->getHorizontalScrollBar(), 0.0f);

	auto newWidth = viewport->getViewedComponent()->getWidth();

	viewport->setViewPositionProportionately(oldPos, 0.0);
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
