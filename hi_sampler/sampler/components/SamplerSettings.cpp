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

#include "SamplerSettings.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
SamplerSettings::SamplerSettings (ModulatorSampler *s)
    : sampler(s)
{
    //[Constructor_pre] You can add your own custom stuff here..
    //[/Constructor_pre]

    addAndMakeVisible (bufferSizeEditor = new Label ("new label",
                                                     TRANS("100000")));
    bufferSizeEditor->setFont (Font ("Khmer UI", 14.00f, Font::plain));
    bufferSizeEditor->setJustificationType (Justification::centredLeft);
    bufferSizeEditor->setEditable (true, true, false);
    bufferSizeEditor->setColour (Label::backgroundColourId, Colour (0x38ffffff));
    bufferSizeEditor->setColour (Label::outlineColourId, Colour (0x38ffffff));
    bufferSizeEditor->setColour (TextEditor::textColourId, Colours::black);
    bufferSizeEditor->setColour (TextEditor::backgroundColourId, Colour (0x00000000));
    bufferSizeEditor->setColour (TextEditor::highlightColourId, Colour (SIGNAL_COLOUR).withAlpha(0.5f));
    bufferSizeEditor->setColour (TextEditor::ColourIds::focusedOutlineColourId, Colour(SIGNAL_COLOUR));
    bufferSizeEditor->addListener (this);

    addAndMakeVisible (preloadBufferEditor = new Label ("new label",
                                                        TRANS("100000")));
    preloadBufferEditor->setFont (Font ("Khmer UI", 14.00f, Font::plain));
    preloadBufferEditor->setJustificationType (Justification::centredLeft);
    preloadBufferEditor->setEditable (true, true, false);
    preloadBufferEditor->setColour (Label::backgroundColourId, Colour (0x38ffffff));
    preloadBufferEditor->setColour (Label::outlineColourId, Colour (0x38ffffff));
    preloadBufferEditor->setColour (TextEditor::textColourId, Colours::black);
    preloadBufferEditor->setColour (TextEditor::backgroundColourId, Colour (0x00000000));
    preloadBufferEditor->setColour (TextEditor::highlightColourId, Colour (0x40750000));
    preloadBufferEditor->setColour (TextEditor::highlightColourId, Colour (SIGNAL_COLOUR).withAlpha(0.5f));
    preloadBufferEditor->setColour (TextEditor::ColourIds::focusedOutlineColourId, Colour(SIGNAL_COLOUR));
    preloadBufferEditor->addListener (this);

    addAndMakeVisible (memoryUsageLabel = new Label ("new label",
                                                     TRANS("0.52MB\n")));
    memoryUsageLabel->setFont (Font ("Arial", 13.00f, Font::plain));
    memoryUsageLabel->setJustificationType (Justification::centredLeft);
    memoryUsageLabel->setEditable (false, false, false);
    memoryUsageLabel->setColour (Label::backgroundColourId, Colour (0x38ffffff));
    memoryUsageLabel->setColour (Label::outlineColourId, Colour (0x38ffffff));
    memoryUsageLabel->setColour (TextEditor::textColourId, Colours::black);
    memoryUsageLabel->setColour (TextEditor::backgroundColourId, Colour (0x00000000));
    memoryUsageLabel->setColour (TextEditor::highlightColourId, Colour (0x40750000));

    addAndMakeVisible (diskSlider = new Slider ("new slider"));
    diskSlider->setRange (0, 100, 0.1);
    diskSlider->setSliderStyle (Slider::LinearBar);
    diskSlider->setTextBoxStyle (Slider::TextBoxLeft, true, 80, 20);
    diskSlider->setColour (Slider::backgroundColourId, Colour (0x38ffffff));
    diskSlider->setColour (Slider::thumbColourId, Colour (SIGNAL_COLOUR).withAlpha(0.5f));
    diskSlider->setColour (Slider::rotarySliderOutlineColourId, Colours::black);
    diskSlider->setColour (Slider::textBoxOutlineColourId, Colour (0x13ffffff));
    diskSlider->addListener (this);

    addAndMakeVisible (voiceAmountEditor = new Label ("new label",
                                                      TRANS("64")));
    voiceAmountEditor->setFont (Font ("Khmer UI", 14.00f, Font::plain));
    voiceAmountEditor->setJustificationType (Justification::centredLeft);
    voiceAmountEditor->setEditable (true, true, false);
    voiceAmountEditor->setColour (Label::backgroundColourId, Colour (0x38ffffff));
    voiceAmountEditor->setColour (Label::outlineColourId, Colour (0x38ffffff));
    voiceAmountEditor->setColour (TextEditor::textColourId, Colours::black);
    voiceAmountEditor->setColour (TextEditor::backgroundColourId, Colour (0x00000000));
    voiceAmountEditor->setColour (TextEditor::highlightColourId, Colour (0x407a0000));
    voiceAmountEditor->setColour (TextEditor::highlightColourId, Colour (SIGNAL_COLOUR).withAlpha(0.5f));
    voiceAmountEditor->setColour (TextEditor::ColourIds::focusedOutlineColourId, Colour(SIGNAL_COLOUR));
    voiceAmountEditor->addListener (this);

    addAndMakeVisible (voiceLimitEditor = new Label ("new label",
                                                     TRANS("16")));
    voiceLimitEditor->setFont (Font ("Khmer UI", 14.00f, Font::plain));
    voiceLimitEditor->setJustificationType (Justification::centredLeft);
    voiceLimitEditor->setEditable (true, true, false);
    voiceLimitEditor->setColour (Label::backgroundColourId, Colour (0x38ffffff));
    voiceLimitEditor->setColour (Label::outlineColourId, Colour (0x38ffffff));
    voiceLimitEditor->setColour (TextEditor::textColourId, Colours::black);
    voiceLimitEditor->setColour (TextEditor::backgroundColourId, Colour (0x00000000));
    voiceLimitEditor->setColour (TextEditor::highlightColourId, Colour (0x40750000));
    voiceLimitEditor->setColour (TextEditor::highlightColourId, Colour (SIGNAL_COLOUR).withAlpha(0.5f));
    voiceLimitEditor->setColour (TextEditor::ColourIds::focusedOutlineColourId, Colour(SIGNAL_COLOUR));
    voiceLimitEditor->addListener (this);

    addAndMakeVisible (fadeTimeEditor = new Label ("new label",
                                                   TRANS("15 ms")));
    fadeTimeEditor->setFont (Font ("Khmer UI", 14.00f, Font::plain));
    fadeTimeEditor->setJustificationType (Justification::centredLeft);
    fadeTimeEditor->setEditable (true, true, false);
    fadeTimeEditor->setColour (Label::backgroundColourId, Colour (0x38ffffff));
    fadeTimeEditor->setColour (Label::outlineColourId, Colour (0x38ffffff));
    fadeTimeEditor->setColour (TextEditor::textColourId, Colours::black);
    fadeTimeEditor->setColour (TextEditor::backgroundColourId, Colour (0x00000000));
    fadeTimeEditor->setColour (TextEditor::highlightColourId, Colour (0x407a0000));
    fadeTimeEditor->setColour (TextEditor::highlightColourId, Colour (SIGNAL_COLOUR).withAlpha(0.5f));
    fadeTimeEditor->setColour (TextEditor::ColourIds::focusedOutlineColourId, Colour(SIGNAL_COLOUR));
    fadeTimeEditor->addListener (this);

    addAndMakeVisible (retriggerEditor = new PopupLabel ("new label",
                                                         TRANS("Kill note")));
    retriggerEditor->setFont (Font ("Khmer UI", 14.00f, Font::plain));
    retriggerEditor->setJustificationType (Justification::centredLeft);
    retriggerEditor->setEditable (false, false, false);
    retriggerEditor->setColour (Label::backgroundColourId, Colour (0x38ffffff));
    retriggerEditor->setColour (Label::outlineColourId, Colour (0x38ffffff));
    retriggerEditor->setColour (TextEditor::textColourId, Colours::black);
    retriggerEditor->setColour (TextEditor::backgroundColourId, Colour (0x00000000));
    retriggerEditor->setColour (TextEditor::highlightColourId, Colour (0x407a0000));

    addAndMakeVisible (rrGroupEditor = new Label ("new label",
                                                  TRANS("0")));
    rrGroupEditor->setFont (Font ("Khmer UI", 14.00f, Font::plain));
    rrGroupEditor->setJustificationType (Justification::centredLeft);
    rrGroupEditor->setEditable (true, true, false);
    rrGroupEditor->setColour (Label::backgroundColourId, Colour (0x38ffffff));
    rrGroupEditor->setColour (Label::outlineColourId, Colour (0x38ffffff));
    rrGroupEditor->setColour (TextEditor::textColourId, Colours::black);
    rrGroupEditor->setColour (TextEditor::backgroundColourId, Colour (0x00000000));
    rrGroupEditor->setColour (TextEditor::highlightColourId, Colour (0x407a0000));
    rrGroupEditor->setColour (TextEditor::highlightColourId, Colour (SIGNAL_COLOUR).withAlpha(0.5f));
    rrGroupEditor->setColour (TextEditor::ColourIds::focusedOutlineColourId, Colour(SIGNAL_COLOUR));
    rrGroupEditor->addListener (this);

    addAndMakeVisible (playbackEditor = new PopupLabel ("new label",
                                                        TRANS("Normal")));
    playbackEditor->setFont (Font ("Khmer UI", 14.00f, Font::plain));
    playbackEditor->setJustificationType (Justification::centredLeft);
    playbackEditor->setEditable (true, true, false);
    playbackEditor->setColour (Label::backgroundColourId, Colour (0x38ffffff));
    playbackEditor->setColour (Label::outlineColourId, Colour (0x38ffffff));
    playbackEditor->setColour (TextEditor::textColourId, Colours::black);
    playbackEditor->setColour (TextEditor::backgroundColourId, Colour (0x00000000));
    playbackEditor->setColour (TextEditor::highlightColourId, Colour (0x407a0000));
    playbackEditor->addListener (this);

    addAndMakeVisible (pitchTrackingEditor = new PopupLabel ("new label",
                                                             TRANS("Enabled")));
    pitchTrackingEditor->setFont (Font ("Khmer UI", 14.00f, Font::plain));
    pitchTrackingEditor->setJustificationType (Justification::centredLeft);
    pitchTrackingEditor->setEditable (true, true, false);
    pitchTrackingEditor->setColour (Label::backgroundColourId, Colour (0x38ffffff));
    pitchTrackingEditor->setColour (Label::outlineColourId, Colour (0x38ffffff));
    pitchTrackingEditor->setColour (TextEditor::textColourId, Colours::black);
    pitchTrackingEditor->setColour (TextEditor::backgroundColourId, Colour (0x00000000));
    pitchTrackingEditor->setColour (TextEditor::highlightColourId, Colour (0x407a0000));
    pitchTrackingEditor->addListener (this);

    addAndMakeVisible (crossfadeGroupEditor = new PopupLabel ("new label",
                                                              TRANS("Enabled")));
    crossfadeGroupEditor->setFont (Font ("Khmer UI", 14.00f, Font::plain));
    crossfadeGroupEditor->setJustificationType (Justification::centredLeft);
    crossfadeGroupEditor->setEditable (true, true, false);
    crossfadeGroupEditor->setColour (Label::backgroundColourId, Colour (0x38ffffff));
    crossfadeGroupEditor->setColour (Label::outlineColourId, Colour (0x38ffffff));
    crossfadeGroupEditor->setColour (TextEditor::textColourId, Colours::black);
    crossfadeGroupEditor->setColour (TextEditor::backgroundColourId, Colour (0x00000000));
    crossfadeGroupEditor->setColour (TextEditor::highlightColourId, Colour (0x407a0000));
    crossfadeGroupEditor->addListener (this);

    addAndMakeVisible (crossfadeEditor = new TableEditor (getProcessor()->getMainController()->getControlUndoManager(), dynamic_cast<ModulatorSampler*>(getProcessor())->getTable(0)));
    crossfadeEditor->setName ("new component");

    addAndMakeVisible (showCrossfadeLabel = new PopupLabel ("new label",
                                                            TRANS("Group 1")));
    showCrossfadeLabel->setFont (Font ("Khmer UI", 14.00f, Font::plain));
    showCrossfadeLabel->setJustificationType (Justification::centredLeft);
    showCrossfadeLabel->setEditable (true, true, false);
    showCrossfadeLabel->setColour (Label::backgroundColourId, Colour (0x38ffffff));
    showCrossfadeLabel->setColour (Label::outlineColourId, Colour (0x38ffffff));
    showCrossfadeLabel->setColour (TextEditor::textColourId, Colours::black);
    showCrossfadeLabel->setColour (TextEditor::backgroundColourId, Colour (0x00000000));
    showCrossfadeLabel->setColour (TextEditor::highlightColourId, Colour (0x407a0000));
    showCrossfadeLabel->addListener (this);

    addAndMakeVisible (purgeSampleEditor = new PopupLabel ("new label",
                                                           TRANS("Enabled")));
    purgeSampleEditor->setFont (Font ("Khmer UI", 14.00f, Font::plain));
    purgeSampleEditor->setJustificationType (Justification::centredLeft);
    purgeSampleEditor->setEditable (true, true, false);
    purgeSampleEditor->setColour (Label::backgroundColourId, Colour (0x38ffffff));
    purgeSampleEditor->setColour (Label::outlineColourId, Colour (0x38ffffff));
    purgeSampleEditor->setColour (TextEditor::textColourId, Colours::black);
    purgeSampleEditor->setColour (TextEditor::backgroundColourId, Colour (0x00000000));
    purgeSampleEditor->setColour (TextEditor::highlightColourId, Colour (0x407a0000));
    purgeSampleEditor->addListener (this);

    addAndMakeVisible (purgeChannelLabel = new PopupLabel ("new label",
                                                           TRANS("NoMultiChannel")));
    purgeChannelLabel->setFont (Font ("Khmer UI", 14.00f, Font::plain));
    purgeChannelLabel->setJustificationType (Justification::centredLeft);
    purgeChannelLabel->setEditable (true, true, false);
    purgeChannelLabel->setColour (Label::backgroundColourId, Colour (0x38ffffff));
    purgeChannelLabel->setColour (Label::outlineColourId, Colour (0x38ffffff));
    purgeChannelLabel->setColour (TextEditor::textColourId, Colours::black);
    purgeChannelLabel->setColour (TextEditor::backgroundColourId, Colour (0x00000000));
    purgeChannelLabel->setColour (TextEditor::highlightColourId, Colour (0x407a0000));
    purgeChannelLabel->addListener (this);


    addAndMakeVisible(timestretchEditor = new PopupLabel("time",
        TRANS("Timestretching")));
    timestretchEditor->setJustificationType(Justification::centredLeft);
    timestretchEditor->setEditable(true, true, false);
    timestretchEditor->setColour(Label::backgroundColourId, Colour(0x38ffffff));
    timestretchEditor->setColour(Label::outlineColourId, Colour(0x38ffffff));
    timestretchEditor->setColour(TextEditor::textColourId, Colours::black);
    timestretchEditor->setColour(TextEditor::backgroundColourId, Colour(0x00000000));
    timestretchEditor->setColour(TextEditor::highlightColourId, Colour(0x407a0000));
    timestretchEditor->addListener(this);

    //[UserPreSize]

	docs = new ModulatorSampler::Documentation();

    bufferSizeEditor->setFont (GLOBAL_FONT());
    preloadBufferEditor->setFont (GLOBAL_FONT());
    memoryUsageLabel->setFont (GLOBAL_FONT());
    timestretchEditor->setFont(GLOBAL_FONT());

    voiceAmountEditor->setFont (GLOBAL_FONT());
    voiceLimitEditor->setFont (GLOBAL_FONT());
    fadeTimeEditor->setFont (GLOBAL_FONT());
    retriggerEditor->setFont (GLOBAL_FONT());
    rrGroupEditor->setFont (GLOBAL_FONT());
    playbackEditor->setFont (GLOBAL_FONT());
    pitchTrackingEditor->setFont(GLOBAL_FONT());
    purgeSampleEditor->setFont(GLOBAL_FONT());
    showCrossfadeLabel->setFont(GLOBAL_FONT());
    crossfadeGroupEditor->setFont(GLOBAL_FONT());
    purgeChannelLabel->setFont(GLOBAL_FONT());

	docs->createHelpButtonForParameter(ModulatorSampler::Parameters::Purged, purgeSampleEditor);
	docs->createHelpButtonForParameter(ModulatorSampler::Parameters::BufferSize, bufferSizeEditor);
	docs->createHelpButtonForParameter(ModulatorSampler::Parameters::CrossfadeGroups, crossfadeGroupEditor);
	docs->createHelpButtonForParameter(ModulatorSampler::Parameters::PitchTracking, pitchTrackingEditor);
	docs->createHelpButtonForParameter(ModulatorSampler::Parameters::SamplerRepeatMode, retriggerEditor);
	docs->createHelpButtonForParameter(ModulatorSampler::Parameters::VoiceAmount, voiceLimitEditor);
	docs->createHelpButtonForParameter(ModulatorSynth::Parameters::VoiceLimit, voiceAmountEditor);
	docs->createHelpButtonForParameter(ModulatorSynth::Parameters::KillFadeTime, fadeTimeEditor);
	docs->createHelpButtonForParameter(ModulatorSampler::Parameters::PreloadSize, preloadBufferEditor);

	retriggerEditor->addListener(this);

	diskSlider->setInterceptsMouseClicks(false, false);
	diskSlider->setTextValueSuffix("%");

    addAndMakeVisible(stretchRatioSlider = new Slider("new slider"));
    stretchRatioSlider->setRange(50, 200.0, 1.0);
    stretchRatioSlider->setSliderStyle(Slider::LinearBar);
    stretchRatioSlider->setTextBoxStyle(Slider::TextBoxLeft, true, 80, 20);
    stretchRatioSlider->setColour(Slider::backgroundColourId, Colour(0x38ffffff));
    stretchRatioSlider->setColour(Slider::thumbColourId, Colour(SIGNAL_COLOUR).withAlpha(0.8f));
    stretchRatioSlider->setColour(Slider::rotarySliderOutlineColourId, Colours::black);
    stretchRatioSlider->setColour(Slider::textBoxOutlineColourId, Colour(0x13ffffff));
    stretchRatioSlider->addListener(this);

    stretchRatioSlider->setTextValueSuffix("%");
    stretchRatioSlider->setSkewFactorFromMidPoint(100.0);
    stretchRatioSlider->addListener(this);

	startTimer(200);

	retriggerEditor->addOption("Kill Note");
	retriggerEditor->addOption("Note off");
	retriggerEditor->addOption("Do nothing");
	retriggerEditor->addOption("Kill Duplicate");

	retriggerEditor->addListener(this);

	playbackEditor->addOption("Normal");
	playbackEditor->addOption("One Shot");
	playbackEditor->addOption("Reverse");
	playbackEditor->addOption("Reverse One Shot");
	playbackEditor->setEditable(false);

	pitchTrackingEditor->addOption("Disabled");
	pitchTrackingEditor->addOption("Enabled");
	pitchTrackingEditor->setEditable(false);

	crossfadeGroupEditor->addOption("Disabled");
	crossfadeGroupEditor->addOption("Enabled");
	crossfadeGroupEditor->setEditable(false);

	purgeSampleEditor->addOption("Disabled");
	purgeSampleEditor->addOption("Enabled");
	purgeSampleEditor->addOption("Lazy Load");
	purgeSampleEditor->setEditable(false);

    timestretchEditor->addOption("Disabled");
	timestretchEditor->addOption("VoiceStart");
    timestretchEditor->addOption("TimeVariant");
    timestretchEditor->addOption("TempoSynced");

    timestretchEditor->setEditable(false);

	for (int i = 0; i < 8; i++)
	{
		showCrossfadeLabel->addOption("Group " + String(i+1));

	}
	showCrossfadeLabel->setEditable(false);


    ProcessorHelpers::connectTableEditor(*crossfadeEditor, getProcessor());
    

	purgeChannelLabel->setEditable(false);

	refreshMicAmount();

    //[/UserPreSize]

    setSize (800, 180);


    //[Constructor] You can add your own custom stuff here..
	h = getHeight();
    //[/Constructor]
}

SamplerSettings::~SamplerSettings()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]

    bufferSizeEditor = nullptr;
    preloadBufferEditor = nullptr;
    memoryUsageLabel = nullptr;
    diskSlider = nullptr;
    voiceAmountEditor = nullptr;
    voiceLimitEditor = nullptr;
    fadeTimeEditor = nullptr;
    retriggerEditor = nullptr;
    rrGroupEditor = nullptr;
    playbackEditor = nullptr;
    pitchTrackingEditor = nullptr;
    crossfadeGroupEditor = nullptr;
    crossfadeEditor = nullptr;
    showCrossfadeLabel = nullptr;
    purgeSampleEditor = nullptr;
    purgeChannelLabel = nullptr;


    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

void SamplerSettings::timerCallback()
{
	const double usage = sampler->getDiskUsage();
	diskSlider->setValue(usage, dontSendNotification);

	auto st = sampler->getCurrentTimestretchRatio();

	stretchRatioSlider->setValue(st * 100.0, dontSendNotification);

    auto enableStretchSlider = (timestretchEditor->getCurrentIndex() == 1 ||
        timestretchEditor->getCurrentIndex() == 2);

    if(enableStretchSlider != stretchRatioSlider->isEnabled())
    {
        stretchRatioSlider->setEnabled(enableStretchSlider);

        stretchRatioSlider->setColour(Slider::thumbColourId, Colour(enableStretchSlider ? SIGNAL_COLOUR : 0xFF999999).withAlpha(0.5f));
    }
}

//==============================================================================
void SamplerSettings::paint (Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced(10.0f);

    auto top = b.removeFromTop(24.0f);

    g.setColour(JUCE_LIVE_CONSTANT_OFF(Colour(0xccffffff)));
    g.setFont(GLOBAL_BOLD_FONT().withHeight(22.0f));
    g.drawText("SAMPLER SETTINGS", top, Justification::right);

    top.removeFromRight(210.0);

    g.setColour(Colour(0x13ffffff));
    g.fillRect(top);

    g.setColour(Colour(0x0fffffff));
    g.drawRect(top, 1.0f);

    auto drawTitle = [](Graphics& g, Rectangle<float> a, const String& text)
    {
        g.setColour(Colours::white.withAlpha(0.05f));

        g.fillRect(a);
        g.drawRect(a, 1.0);

        auto ta = a.removeFromTop(16);

        g.fillRect(ta.reduced(1.0f));

        g.setColour(Colours::white.withAlpha(0.8f));
        g.setFont(GLOBAL_BOLD_FONT());
        g.drawText(text, ta, Justification::centred);
    };

    drawTitle(g, columns[0], "Disk I/O Settings");

    drawTitle(g, columns[1], "Voice Settings");

    auto rc = columns[2];

    auto roc = rc.removeFromTop(rc.getHeight() / 2.0f - 5.0f);
    rc.removeFromTop(10.0f);

    auto rbc = rc;
    
    drawTitle(g, roc, "Group Settings");
    drawTitle(g, rbc, "Playback Settings");
    
    attachLabel(g, *bufferSizeEditor, "Buffer Size");
    attachLabel(g, *preloadBufferEditor, "Preload Size");
    attachLabel(g, *purgeChannelLabel, "Purge Channel");

    attachLabel(g, *memoryUsageLabel, "Memory Usage");
    attachLabel(g, *diskSlider, "Disk Usage");
    attachLabel(g, *purgeSampleEditor, "Purge All");

    attachLabel(g, *voiceAmountEditor, "Amount");
    attachLabel(g, *voiceLimitEditor, "Soft Limit");
    attachLabel(g, *fadeTimeEditor, "Fade Time");

    attachLabel(g, *rrGroupEditor, "RR Groups");
    attachLabel(g, *crossfadeGroupEditor, "Group XF");
    attachLabel(g, *showCrossfadeLabel, "Edit XF");

    attachLabel(g, *pitchTrackingEditor, "Pitch Tracking");
    attachLabel(g, *retriggerEditor, "Retrigger");
    attachLabel(g, *playbackEditor, "Playback");

    attachLabel(g, *timestretchEditor, "Timestretching");
    attachLabel(g, *stretchRatioSlider, "Stretch Ratio");
    

#if 0

    //[UserPrePaint] Add your own custom painting code here..

    int x = 0;
    int y = 2;
    int width = getWidth();
    int height = getHeight()-4;
    
	Rectangle<int> a(x, y, width, height);

	//ProcessorEditorLookAndFeel::drawShadowBox(g, a, Colour(0xFF333333));

    //[/UserPrePaint]

    g.setColour (JUCE_LIVE_CONSTANT_OFF(Colour (0x18ffffff)));
    g.fillRect ((getWidth() / 2) - (222 / 2), 74, 222, 14);

    g.setColour (Colour (0x18ffffff));
    g.fillRect (getWidth() - 9 - 222, 42, 222, 14);

    g.setColour (JUCE_LIVE_CONSTANT_OFF(Colour (0xccffffff)));
    g.setFont (GLOBAL_BOLD_FONT().withHeight(22.0f));
    g.drawText (TRANS("SAMPLER SETTINGS"),
                getWidth() - 8 - 240, 6, 240, 30,
                Justification::centredRight, true);

    g.setColour (Colour (0x13ffffff));
    g.fillRect (8, 8, getWidth() - 210, 24);

    g.setColour (Colour (0x0fffffff));
    g.drawRect (8, 8, getWidth() - 210, 24, 1);

    g.setColour (Colour (0x13ffffff));
    g.fillRect (8, 56, 224, 88);

    g.setColour (Colour (0x0fffffff));
    g.drawRect (8, 56, 224, 88, 1);

    g.setColour (Colour (0x13ffffff));
    g.fillRect (getWidth() - 8 - 224, 41, 224, 55);

    g.setColour (Colour (0x0fffffff));
    g.drawRect (getWidth() - 8 - 224, 41, 224, 55, 1);

    g.setColour (Colour (0x80ffffff));
    g.setFont (GLOBAL_BOLD_FONT());
    g.drawText (TRANS("Disk IO Settings"),
                16, 49, 200, 30,
                Justification::centred, true);

    g.setColour (Colour (0x80ffffff));
	g.setFont(GLOBAL_BOLD_FONT());
    g.drawText (TRANS("Group Settings"),
                getWidth() - 220, 33, 200, 30,
                Justification::centred, true);

    g.setColour (Colour (0x13ffffff));
    g.fillRect ((getWidth() / 2) - (224 / 2), 33, 224, 95);

    g.setColour (Colour (0x0fffffff));
    g.drawRect ((getWidth() / 2) - (224 / 2), 33, 224, 95, 1);

    g.setColour (Colour (0x80ffffff));
	g.setFont(GLOBAL_BOLD_FONT());
    g.drawText (TRANS("Voice Settings"),
                (getWidth() / 2) - (200 / 2), 33, 200, 30,
                Justification::centred, true);

    g.setColour (Colour (0x13ffffff));
    g.fillRect (getWidth() - 8 - 224, 105, 224, 55);

    g.setColour (Colour (0x0fffffff));
    g.drawRect (getWidth() - 8 - 224, 105, 224, 55, 1);

    g.setColour (Colour (0x80ffffff));
	g.setFont(GLOBAL_BOLD_FONT());
    g.drawText (TRANS("Playback Settings"),
                getWidth() - 224, 97, 208, 30,
                Justification::centred, true);

    g.setColour (Colour (0x18ffffff));
    g.fillRect (getWidth() - 9 - 222, 106, 222, 14);

    g.setColour (Colour (0x18ffffff));
    g.fillRect (9, 57, 222, 14);

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
#endif
}

void SamplerSettings::resized()
{
    auto b = getLocalBounds().toFloat().reduced(10.0f);

    b.removeFromTop(10.0f + 24.0);

    auto w = b.getWidth() / 3;

    if(crossfadeEditor->isVisible())
    {
        
        crossfadeEditor->setBounds(b.removeFromBottom(115).toNearestInt());
        b.removeFromBottom(10.0f);
    }

    columns[0] = b.removeFromLeft(w);
    columns[1] = b.removeFromLeft(w);
    columns[2] = b.removeFromLeft(w);

    columns[0].removeFromRight(5.0f);
    columns[1].removeFromRight(5.0f);

    columns[1].removeFromLeft(5.0f);
    columns[2].removeFromLeft(5.0f);

    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    auto placeComponents = [](Rectangle<int> s, Component& c1, Component& c2, Component& c3)
    {
        s = s.withSizeKeepingCentre(s.getWidth() - 20, 16);
        s.translate(0, 5);

        auto w = (s.getWidth() - 20) / 3 ;

        c1.setBounds(s.removeFromLeft(w)); s.removeFromLeft(10);
        c2.setBounds(s.removeFromLeft(w)); s.removeFromLeft(10);
        c3.setBounds(s.removeFromLeft(w)); s.removeFromLeft(10);
    };

    {
        auto r0 = columns[0].toNearestInt();
        r0.removeFromTop(16);

        auto r1 = r0.removeFromBottom(r0.getHeight() / 2);

        placeComponents(r0, *bufferSizeEditor, *preloadBufferEditor, *purgeChannelLabel);
        placeComponents(r1, *memoryUsageLabel, *diskSlider, *purgeSampleEditor);
    }
    {
	    auto r0 = columns[1].toNearestInt();
        r0.removeFromTop(16);

        auto r1 = r0.removeFromBottom(r0.getHeight() / 2);

        placeComponents(r0, *voiceAmountEditor, *voiceLimitEditor, *fadeTimeEditor);

        placeComponents(r1, *timestretchEditor, *stretchRatioSlider, *stretchRatioSlider);
    }
    {
        auto r0 = columns[2].toNearestInt();
        auto r1 = r0.removeFromBottom(r0.getHeight() / 2);

        r0.removeFromBottom(5);
        r1.removeFromTop(5);

    	r0.removeFromTop(16);
        r1.removeFromTop(16);

        placeComponents(r0, *rrGroupEditor, *crossfadeGroupEditor, *showCrossfadeLabel);
        placeComponents(r1, *pitchTrackingEditor, *retriggerEditor, *playbackEditor);
    }
    


#if 0
    bufferSizeEditor->setBounds (17, 90, 64, 16);
    preloadBufferEditor->setBounds (86, 90, 64, 16);
    memoryUsageLabel->setBounds (17, 122, 64, 16);
    diskSlider->setBounds (86, 122, 64, 16);
    voiceAmountEditor->setBounds ((getWidth() / 2) + -40 - 64, 107, 64, 16);
    voiceLimitEditor->setBounds ((getWidth() / 2) + 31 - 64, 107, 64, 16);
    fadeTimeEditor->setBounds ((getWidth() / 2) + 40, 106, 64, 16);
    retriggerEditor->setBounds (getWidth() - 154, 137, 64, 16);
    //voiceAmountLabel2->setBounds ((getWidth() - 223) + -14, 72 + -19, 79, 24);
    rrGroupEditor->setBounds (getWidth() - 223, 72, 64, 16);
    //playbackModeDescription->setBounds ((getWidth() - 18 - 64) + -6, 137 + -19, 82, 24);
    playbackEditor->setBounds (getWidth() - 18 - 64, 137, 64, 16);
    //playbackModeDescription2->setBounds ((getWidth() - 225) + -5, 137 + -19, 82, 24);
    pitchTrackingEditor->setBounds (getWidth() - 225, 137, 64, 16);
    //voiceLimitLabel2->setBounds ((getWidth() - 152) + -17, 72 + -19, 80, 24);
    crossfadeGroupEditor->setBounds (getWidth() - 152, 72, 64, 16);
    crossfadeEditor->setBounds (16, 174, getWidth() - 32, 112);
    //voiceLimitLabel3->setBounds ((getWidth() - 81) + -22, 72 + -19, 80, 24);
    showCrossfadeLabel->setBounds (getWidth() - 81, 72, 64, 16);
    //voiceLimitLabel4->setBounds (86 + 52, 103, 80, 24);
    purgeSampleEditor->setBounds (156, 122, 64, 16);
    //channelAmountLabel3->setBounds (86 + 64, 71, 79, 24);
    purgeChannelLabel->setBounds (86 + 134 - 64, 90, 64, 16);
    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
#endif
}

void SamplerSettings::labelTextChanged (Label* labelThatHasChanged)
{
    //[UserlabelTextChanged_Pre]
    //[/UserlabelTextChanged_Pre]

    if (labelThatHasChanged == bufferSizeEditor)
    {
        //[UserLabelCode_bufferSizeEditor] -- add your label text handling code here..
		sampler->setAttribute(ModulatorSampler::BufferSize, labelThatHasChanged->getText().getFloatValue(), dontSendNotification);
        //[/UserLabelCode_bufferSizeEditor]
    }
    else if (labelThatHasChanged == preloadBufferEditor)
    {
        //[UserLabelCode_preloadBufferEditor] -- add your label text handling code here..
		sampler->setAttribute(ModulatorSampler::PreloadSize, labelThatHasChanged->getText().getFloatValue(), dontSendNotification);
        //[/UserLabelCode_preloadBufferEditor]
    }
    else if (labelThatHasChanged == voiceAmountEditor)
    {
        //[UserLabelCode_voiceAmountEditor] -- add your label text handling code here..
		int value = labelThatHasChanged->getText().getIntValue();

		if(value > 0)
		{
			value = jmin(NUM_POLYPHONIC_VOICES, value);

			sampler->setAttribute(ModulatorSampler::VoiceAmount, (float)value, dontSendNotification);
		}

        //[/UserLabelCode_voiceAmountEditor]
    }
    else if (labelThatHasChanged == voiceLimitEditor)
    {
        //[UserLabelCode_voiceLimitEditor] -- add your label text handling code here..

		int value = labelThatHasChanged->getText().getIntValue();

		if(value > 0)
		{
			value = jmin(NUM_POLYPHONIC_VOICES, value);

			sampler->setAttribute(ModulatorSynth::VoiceLimit, (float)value, dontSendNotification);
		}


        //[/UserLabelCode_voiceLimitEditor]
    }
    else if (labelThatHasChanged == fadeTimeEditor)
    {
        //[UserLabelCode_fadeTimeEditor] -- add your label text handling code here..

		int value = labelThatHasChanged->getText().getIntValue();

		if(value > 0)
		{
			value = jmin(20000, value);

			sampler->setAttribute(ModulatorSampler::KillFadeTime, (float)value, dontSendNotification);
		}

        //[/UserLabelCode_fadeTimeEditor]
    }
    else if (labelThatHasChanged == rrGroupEditor)
    {
        //[UserLabelCode_rrGroupEditor] -- add your label text handling code here..

		int value = labelThatHasChanged->getText().getIntValue();

		if(value >= 1)
		{
			sampler->setAttribute(ModulatorSampler::RRGroupAmount, (float)value, sendNotification);



		}
		else
		{
			labelThatHasChanged->setText(String(sampler->getAttribute(ModulatorSampler::RRGroupAmount), 0), dontSendNotification);
		}

        //[/UserLabelCode_rrGroupEditor]
    }
    else if (labelThatHasChanged == playbackEditor)
    {
        //[UserLabelCode_playbackEditor] -- add your label text handling code here..

		const bool isOneShot = (playbackEditor->getCurrentIndex() % 2) != 0;
		const bool isReverse = playbackEditor->getCurrentIndex() > 1;

		sampler->setAttribute(ModulatorSampler::OneShot, isOneShot ? 1.0f : 0.0f, dontSendNotification);
		sampler->setAttribute(ModulatorSampler::Reversed, isReverse ? 1.0f : 0.0f, dontSendNotification);

        //[/UserLabelCode_playbackEditor]
    }
    else if (labelThatHasChanged == pitchTrackingEditor)
    {
        //[UserLabelCode_pitchTrackingEditor] -- add your label text handling code here..

		sampler->setAttribute(ModulatorSampler::PitchTracking, (float)pitchTrackingEditor->getCurrentIndex(), dontSendNotification);

        //[/UserLabelCode_pitchTrackingEditor]
    }
    else if (labelThatHasChanged == timestretchEditor)
    {
        //[UserLabelCode_pitchTrackingEditor] -- add your label text handling code here..

        using TM = ModulatorSampler::TimestretchOptions::TimestretchMode;

        sampler->setCurrentTimestretchMode(static_cast<TM>(timestretchEditor->getCurrentIndex()));

        //[/UserLabelCode_pitchTrackingEditor]
    }
    else if (labelThatHasChanged == crossfadeGroupEditor)
    {
        //[UserLabelCode_crossfadeGroupEditor] -- add your label text handling code here..

		const float enabled = (float)crossfadeGroupEditor->getCurrentIndex();

		sampler->setAttribute(ModulatorSampler::CrossfadeGroups, enabled, dontSendNotification);

		sampler->setEditorState(sampler->getEditorStateForIndex(ModulatorSampler::CrossfadeTableShown), enabled);
		
		if (enabled)
		{
			int crossfadeShown = sampler->getEditorState(sampler->getEditorStateForIndex(ModulatorSampler::CrossfadeTableShown));

			crossfadeEditor->setEditedTable(sampler->getTable(crossfadeShown));
		}
		else
		{
			crossfadeEditor->setEditedTable(nullptr);
		}

		BACKEND_ONLY(findParentComponentOfClass<ProcessorEditorBody>()->refreshBodySize());

        //[/UserLabelCode_crossfadeGroupEditor]
    }
    else if (labelThatHasChanged == showCrossfadeLabel)
    {
        //[UserLabelCode_showCrossfadeLabel] -- add your label text handling code here..

		sampler->setEditorState(sampler->getEditorStateForIndex(ModulatorSampler::EditorStates::CrossfadeTableShown), showCrossfadeLabel->getCurrentIndex(), dontSendNotification);

		crossfadeEditor->setEditedTable(sampler->getTable(showCrossfadeLabel->getCurrentIndex()));



        //[/UserLabelCode_showCrossfadeLabel]
    }
    else if (labelThatHasChanged == purgeSampleEditor)
    {
        //[UserLabelCode_purgeSampleEditor] -- add your label text handling code here..

		sampler->setAttribute(ModulatorSampler::Purged, (float)purgeSampleEditor->getCurrentIndex(), dontSendNotification);

        //[/UserLabelCode_purgeSampleEditor]
    }
    else if (labelThatHasChanged == purgeChannelLabel)
    {
        //[UserLabelCode_purgeChannelLabel] -- add your label text handling code here..

		const int channelNumber = purgeChannelLabel->getCurrentIndex();

		const bool isEnabled = sampler->getChannelData(channelNumber).enabled;

		sampler->setMicEnabled(channelNumber, !isEnabled);

		refreshTickStatesForPurgeChannel();

        //[/UserLabelCode_purgeChannelLabel]
    }

    //[UserlabelTextChanged_Post]



	else if (labelThatHasChanged == retriggerEditor)
	{
		sampler->setAttribute(ModulatorSampler::SamplerRepeatMode, (float)retriggerEditor->getCurrentIndex(), dontSendNotification);

	}
    //[/UserlabelTextChanged_Post]
}

void SamplerSettings::sliderValueChanged (Slider* sliderThatWasMoved)
{
    //[UsersliderValueChanged_Pre]
    //[/UsersliderValueChanged_Pre]

    if (sliderThatWasMoved == stretchRatioSlider)
    {
        sampler->setTimestretchRatio(stretchRatioSlider->getValue() / 100.0);
        //[UserSliderCode_diskSlider] -- add your slider handling code here..
        //[/UserSliderCode_diskSlider]
    }

    //[UsersliderValueChanged_Post]
    //[/UsersliderValueChanged_Post]
}

void SamplerSettings::attachLabel(Graphics& g, Component& c, const String& text)
{
	auto b = c.getBoundsInParent().translated(0, -15).toFloat();
	g.setFont(GLOBAL_FONT().withHeight(11.0f));
	g.setColour(Colours::white.withAlpha(0.7f));
	g.drawText(text, b, Justification::left);
}


//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...

void SamplerSettings::updateGui()
{
	if(bufferSizeEditor->getCurrentTextEditor() == nullptr)
	{
		bufferSizeEditor->setText(String((int)sampler->getAttribute(ModulatorSampler::BufferSize)), dontSendNotification);
	}

	if(preloadBufferEditor->getCurrentTextEditor() == nullptr)
	{
		preloadBufferEditor->setText(String((int)sampler->getAttribute(ModulatorSampler::PreloadSize)), dontSendNotification);
	}

	memoryUsageLabel->setText(sampler->getMemoryUsage(), dontSendNotification);

	if(voiceLimitEditor->getCurrentTextEditor() == nullptr)
	{
		voiceLimitEditor->setText(String((int)sampler->getAttribute(ModulatorSampler::VoiceLimit)), dontSendNotification);
	}

	if(fadeTimeEditor->getCurrentTextEditor() == nullptr)
	{
		fadeTimeEditor->setText(String((int)sampler->getAttribute(ModulatorSampler::KillFadeTime)), dontSendNotification);
	}

	if(voiceAmountEditor->getCurrentTextEditor() == nullptr)
	{
		voiceAmountEditor->setText(String((int)sampler->getAttribute(ModulatorSampler::VoiceAmount)), dontSendNotification);
	}


	if(retriggerEditor->getCurrentTextEditor() == nullptr)
	{
		retriggerEditor->setItemIndex((int)sampler->getAttribute(ModulatorSampler::SamplerRepeatMode), dontSendNotification);
	}

	if(playbackEditor->getCurrentTextEditor() == nullptr)
	{
		const int oneShot = (int)sampler->getAttribute(ModulatorSampler::OneShot);
		const int reversed = (int)sampler->getAttribute(ModulatorSampler::Reversed);

		playbackEditor->setItemIndex(reversed * 2 + oneShot, dontSendNotification);
	}

	if (showCrossfadeLabel->getCurrentTextEditor() == nullptr)
	{
		showCrossfadeLabel->setItemIndex(sampler->getEditorState(ModulatorSampler::EditorStates::CrossfadeTableShown));
	}

	if (crossfadeGroupEditor->getCurrentTextEditor() == nullptr)
	{
		crossfadeGroupEditor->setItemIndex((int)sampler->getAttribute(ModulatorSampler::CrossfadeGroups), dontSendNotification);
	}

	if(pitchTrackingEditor->getCurrentTextEditor() == nullptr)
	{
		pitchTrackingEditor->setItemIndex((int)sampler->getAttribute(ModulatorSampler::PitchTracking), dontSendNotification);
	}

	if (timestretchEditor->getCurrentTextEditor() == nullptr)
	{
		timestretchEditor->setItemIndex((int)sampler->getTimestretchMode(), dontSendNotification);
	}

	if(rrGroupEditor->getCurrentTextEditor() == nullptr)
	{
		rrGroupEditor->setText(String((int)sampler->getAttribute(ModulatorSampler::RRGroupAmount)), dontSendNotification);
	}

	if (purgeSampleEditor->getCurrentTextEditor() == nullptr)
	{
		purgeSampleEditor->setItemIndex((int)sampler->getAttribute(ModulatorSampler::Purged), dontSendNotification);
	}

    

	refreshMicAmount();
}

void SamplerSettings::refreshMicAmount()
{
	const int channelSize = (int)sampler->getNumMicPositions();

	currentChannelSize = channelSize;

	purgeChannelLabel->setEnabled(channelSize != 1);




	purgeChannelLabel->clearOptions();

	BigInteger state = 0;

	for (int i = 0; i < channelSize; i++)
	{

		purgeChannelLabel->addOption(sampler->getChannelData(i).suffix);

	}

	refreshTickStatesForPurgeChannel();

}

void SamplerSettings::refreshTickStatesForPurgeChannel()
{
	const int channelSize = (int)sampler->getNumMicPositions();

	BigInteger state = 0;

	for (int i = 0; i < channelSize; i++)
	{
		ModulatorSampler::ChannelData data = sampler->getChannelData(i);

		state.setBit(i, data.enabled);
	}

	purgeChannelLabel->setTickedState(state);
}


//[/MiscUserCode]



//[EndFile] You can add extra defines here...
} // namespace hise
//[/EndFile]
