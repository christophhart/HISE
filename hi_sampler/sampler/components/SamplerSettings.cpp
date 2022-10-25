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

    addAndMakeVisible (fadeTimeLabel = new Label ("new label",
                                                  TRANS("Fade Time")));
    fadeTimeLabel->setFont (Font ("Khmer UI", 13.00f, Font::plain));
    fadeTimeLabel->setJustificationType (Justification::centred);
    fadeTimeLabel->setEditable (false, false, false);
    fadeTimeLabel->setColour (Label::textColourId, Colours::white);
    fadeTimeLabel->setColour (TextEditor::textColourId, Colours::black);
    fadeTimeLabel->setColour (TextEditor::backgroundColourId, Colour (0x00000000));
    fadeTimeLabel->setColour (TextEditor::highlightColourId, Colour (SIGNAL_COLOUR).withAlpha(0.5f));
    fadeTimeLabel->setColour (TextEditor::ColourIds::focusedOutlineColourId, Colour(SIGNAL_COLOUR));
    
    addAndMakeVisible (voiceAmountLabel = new Label ("new label",
                                                     TRANS("Amount")));
    voiceAmountLabel->setFont (Font ("Arial", 12.00f, Font::plain));
    voiceAmountLabel->setJustificationType (Justification::centredLeft);
    voiceAmountLabel->setEditable (false, false, false);
    voiceAmountLabel->setColour (Label::textColourId, Colours::white);
    voiceAmountLabel->setColour (TextEditor::textColourId, Colours::black);
    voiceAmountLabel->setColour (TextEditor::backgroundColourId, Colour (0x00000000));
    voiceAmountLabel->setColour (TextEditor::highlightColourId, Colour (SIGNAL_COLOUR).withAlpha(0.5f));
    voiceAmountLabel->setColour (TextEditor::ColourIds::focusedOutlineColourId, Colour(SIGNAL_COLOUR));

    addAndMakeVisible (label2 = new Label ("new label",
                                           TRANS("Preload Size")));
    label2->setFont (Font ("Arial", 12.00f, Font::plain));
    label2->setJustificationType (Justification::centredLeft);
    label2->setEditable (false, false, false);
    label2->setColour (Label::textColourId, Colours::white);
    label2->setColour (TextEditor::textColourId, Colours::black);
    label2->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (label = new Label ("new label",
                                          TRANS("Buffer Size")));
    label->setFont (Font ("Arial", 12.00f, Font::plain));
    label->setJustificationType (Justification::centredLeft);
    label->setEditable (false, false, false);
    label->setColour (Label::textColourId, Colours::white);
    label->setColour (TextEditor::textColourId, Colours::black);
    label->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (label4 = new Label ("new label",
                                           TRANS("Disk Usage")));
    label4->setFont (Font ("Arial", 12.00f, Font::plain));
    label4->setJustificationType (Justification::centredLeft);
    label4->setEditable (false, false, false);
    label4->setColour (Label::textColourId, Colours::white);
    label4->setColour (TextEditor::textColourId, Colours::black);
    label4->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (label3 = new Label ("new label",
                                           TRANS("Memory")));
    label3->setFont (Font ("Arial", 12.00f, Font::plain));
    label3->setJustificationType (Justification::centredLeft);
    label3->setEditable (false, false, false);
    label3->setColour (Label::textColourId, Colours::white);
    label3->setColour (TextEditor::textColourId, Colours::black);
    label3->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (fadeTimeLabel2 = new Label ("new label",
                                                   TRANS("Retrigger")));
    fadeTimeLabel2->setFont (Font ("Khmer UI", 13.00f, Font::plain));
    fadeTimeLabel2->setJustificationType (Justification::centredLeft);
    fadeTimeLabel2->setEditable (false, false, false);
    fadeTimeLabel2->setColour (Label::textColourId, Colours::white);
    fadeTimeLabel2->setColour (TextEditor::textColourId, Colours::black);
    fadeTimeLabel2->setColour (TextEditor::backgroundColourId, Colour (0x00000000));
    fadeTimeLabel2->setColour (TextEditor::highlightColourId, Colour (SIGNAL_COLOUR).withAlpha(0.5f));
    fadeTimeLabel2->setColour (TextEditor::ColourIds::focusedOutlineColourId, Colour(SIGNAL_COLOUR));
    
    
    addAndMakeVisible (voiceLimitLabel = new Label ("new label",
                                                    TRANS("Soft Limit")));
    voiceLimitLabel->setFont (Font ("Khmer UI", 13.00f, Font::plain));
    voiceLimitLabel->setJustificationType (Justification::centred);
    voiceLimitLabel->setEditable (false, false, false);
    voiceLimitLabel->setColour (Label::textColourId, Colours::white);
    voiceLimitLabel->setColour (TextEditor::textColourId, Colours::black);
    voiceLimitLabel->setColour (TextEditor::backgroundColourId, Colour (0x00000000));
    voiceLimitLabel->setColour (TextEditor::highlightColourId, Colour (SIGNAL_COLOUR).withAlpha(0.5f));
    voiceLimitLabel->setColour (TextEditor::ColourIds::focusedOutlineColourId, Colour(SIGNAL_COLOUR));
    
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
    diskSlider->setColour (Slider::textBoxOutlineColourId, Colour (0x38ffffff));
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

    addAndMakeVisible (voiceAmountLabel2 = new Label ("new label",
                                                      TRANS("RR Groups")));
    voiceAmountLabel2->setFont (Font ("Khmer UI", 13.00f, Font::plain));
    voiceAmountLabel2->setJustificationType (Justification::centred);
    voiceAmountLabel2->setEditable (false, false, false);
    voiceAmountLabel2->setColour (Label::textColourId, Colours::white);
    voiceAmountLabel2->setColour (TextEditor::textColourId, Colours::black);
    voiceAmountLabel2->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

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

    addAndMakeVisible (playbackModeDescription = new Label ("new label",
                                                            TRANS("Playback")));
    playbackModeDescription->setFont (Font ("Khmer UI", 13.00f, Font::plain));
    playbackModeDescription->setJustificationType (Justification::centredLeft);
    playbackModeDescription->setEditable (false, false, false);
    playbackModeDescription->setColour (Label::textColourId, Colours::white);
    playbackModeDescription->setColour (TextEditor::textColourId, Colours::black);
    playbackModeDescription->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

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

    addAndMakeVisible (playbackModeDescription2 = new Label ("new label",
                                                             TRANS("Pitch Track")));
    playbackModeDescription2->setFont (Font ("Khmer UI", 13.00f, Font::plain));
    playbackModeDescription2->setJustificationType (Justification::centredLeft);
    playbackModeDescription2->setEditable (false, false, false);
    playbackModeDescription2->setColour (Label::textColourId, Colours::white);
    playbackModeDescription2->setColour (TextEditor::textColourId, Colours::black);
    playbackModeDescription2->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

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

    addAndMakeVisible (voiceLimitLabel2 = new Label ("new label",
                                                     TRANS("Group XF")));
    voiceLimitLabel2->setFont (Font ("Arial", 12.00f, Font::plain));
    voiceLimitLabel2->setJustificationType (Justification::centred);
    voiceLimitLabel2->setEditable (false, false, false);
    voiceLimitLabel2->setColour (Label::textColourId, Colours::white);
    voiceLimitLabel2->setColour (TextEditor::textColourId, Colours::black);
    voiceLimitLabel2->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

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

    addAndMakeVisible (voiceLimitLabel3 = new Label ("new label",
                                                     TRANS("Edit XF")));
    voiceLimitLabel3->setFont (Font ("Arial", 12.00f, Font::plain));
    voiceLimitLabel3->setJustificationType (Justification::centred);
    voiceLimitLabel3->setEditable (false, false, false);
    voiceLimitLabel3->setColour (Label::textColourId, Colours::white);
    voiceLimitLabel3->setColour (TextEditor::textColourId, Colours::black);
    voiceLimitLabel3->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

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

    addAndMakeVisible (voiceLimitLabel4 = new Label ("new label",
                                                     TRANS("Purge All")));
    voiceLimitLabel4->setFont (Font ("Arial", 12.00f, Font::plain));
    voiceLimitLabel4->setJustificationType (Justification::centred);
    voiceLimitLabel4->setEditable (false, false, false);
    voiceLimitLabel4->setColour (Label::textColourId, Colours::white);
    voiceLimitLabel4->setColour (TextEditor::textColourId, Colours::black);
    voiceLimitLabel4->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

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

    addAndMakeVisible (channelAmountLabel3 = new Label ("new label",
                                                        TRANS("Purge Channel")));
    channelAmountLabel3->setFont (Font ("Arial", 13.00f, Font::plain));
    channelAmountLabel3->setJustificationType (Justification::centredLeft);
    channelAmountLabel3->setEditable (false, false, false);
    channelAmountLabel3->setColour (Label::textColourId, Colours::white);
    channelAmountLabel3->setColour (TextEditor::textColourId, Colours::black);
    channelAmountLabel3->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

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


    //[UserPreSize]

	docs = new ModulatorSampler::Documentation();

	fadeTimeLabel->setFont (GLOBAL_FONT());
    voiceAmountLabel->setFont (GLOBAL_FONT());
    label2->setFont (GLOBAL_FONT());
    label->setFont (GLOBAL_FONT());
    label4->setFont (GLOBAL_FONT());
    label3->setFont (GLOBAL_FONT());
    fadeTimeLabel2->setFont (GLOBAL_FONT());
    voiceLimitLabel->setFont (GLOBAL_FONT());
    bufferSizeEditor->setFont (GLOBAL_FONT());
    preloadBufferEditor->setFont (GLOBAL_FONT());
    memoryUsageLabel->setFont (GLOBAL_FONT());

    voiceAmountEditor->setFont (GLOBAL_FONT());
    voiceLimitEditor->setFont (GLOBAL_FONT());
    fadeTimeEditor->setFont (GLOBAL_FONT());
    retriggerEditor->setFont (GLOBAL_FONT());
    voiceAmountLabel2->setFont (GLOBAL_FONT());
    rrGroupEditor->setFont (GLOBAL_FONT());
    playbackModeDescription->setFont (GLOBAL_FONT());
    playbackEditor->setFont (GLOBAL_FONT());
    playbackModeDescription2->setFont (GLOBAL_FONT());
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
	purgeSampleEditor->setEditable(false);

	for (int i = 0; i < 8; i++)
	{
		showCrossfadeLabel->addOption("Group " + String(i+1));

	}
	showCrossfadeLabel->setEditable(false);


    ProcessorHelpers::connectTableEditor(*crossfadeEditor, getProcessor());
    

	purgeChannelLabel->setEditable(false);

	refreshMicAmount();

    //[/UserPreSize]

    setSize (800, 170);


    //[Constructor] You can add your own custom stuff here..
	h = getHeight();
    //[/Constructor]
}

SamplerSettings::~SamplerSettings()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]

    fadeTimeLabel = nullptr;
    voiceAmountLabel = nullptr;
    label2 = nullptr;
    label = nullptr;
    label4 = nullptr;
    label3 = nullptr;
    fadeTimeLabel2 = nullptr;
    voiceLimitLabel = nullptr;
    bufferSizeEditor = nullptr;
    preloadBufferEditor = nullptr;
    memoryUsageLabel = nullptr;
    diskSlider = nullptr;
    voiceAmountEditor = nullptr;
    voiceLimitEditor = nullptr;
    fadeTimeEditor = nullptr;
    retriggerEditor = nullptr;
    voiceAmountLabel2 = nullptr;
    rrGroupEditor = nullptr;
    playbackModeDescription = nullptr;
    playbackEditor = nullptr;
    playbackModeDescription2 = nullptr;
    pitchTrackingEditor = nullptr;
    voiceLimitLabel2 = nullptr;
    crossfadeGroupEditor = nullptr;
    crossfadeEditor = nullptr;
    voiceLimitLabel3 = nullptr;
    showCrossfadeLabel = nullptr;
    voiceLimitLabel4 = nullptr;
    purgeSampleEditor = nullptr;
    channelAmountLabel3 = nullptr;
    purgeChannelLabel = nullptr;


    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void SamplerSettings::paint (Graphics& g)
{
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
    g.fillRect ((getWidth() / 2) - (224 / 2), 73, 224, 55);

    g.setColour (Colour (0x0fffffff));
    g.drawRect ((getWidth() / 2) - (224 / 2), 73, 224, 55, 1);

    g.setColour (Colour (0x80ffffff));
	g.setFont(GLOBAL_BOLD_FONT());
    g.drawText (TRANS("Voice Settings"),
                (getWidth() / 2) - (200 / 2), 65, 200, 30,
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
}

void SamplerSettings::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    fadeTimeLabel->setBounds (((getWidth() / 2) + 40) + -14, 106 + -19, 79, 24);
    voiceAmountLabel->setBounds (((getWidth() / 2) + -40 - 64) + -4, 107 + -19, 79, 24);
    label2->setBounds (80, 71, 88, 24);
    label->setBounds (11, 71, 79, 24);
    label4->setBounds (80, 103, 88, 24);
    label3->setBounds (11, 103, 88, 24);
    fadeTimeLabel2->setBounds ((getWidth() - 154) + -6, 137 + -18, 82, 24);
    voiceLimitLabel->setBounds (((getWidth() / 2) + 31 - 64) + -16, 107 + -19, 80, 24);
    bufferSizeEditor->setBounds (17, 90, 64, 16);
    preloadBufferEditor->setBounds (86, 90, 64, 16);
    memoryUsageLabel->setBounds (17, 122, 64, 16);
    diskSlider->setBounds (86, 122, 64, 16);
    voiceAmountEditor->setBounds ((getWidth() / 2) + -40 - 64, 107, 64, 16);
    voiceLimitEditor->setBounds ((getWidth() / 2) + 31 - 64, 107, 64, 16);
    fadeTimeEditor->setBounds ((getWidth() / 2) + 40, 106, 64, 16);
    retriggerEditor->setBounds (getWidth() - 154, 137, 64, 16);
    voiceAmountLabel2->setBounds ((getWidth() - 223) + -14, 72 + -19, 79, 24);
    rrGroupEditor->setBounds (getWidth() - 223, 72, 64, 16);
    playbackModeDescription->setBounds ((getWidth() - 18 - 64) + -6, 137 + -19, 82, 24);
    playbackEditor->setBounds (getWidth() - 18 - 64, 137, 64, 16);
    playbackModeDescription2->setBounds ((getWidth() - 225) + -5, 137 + -19, 82, 24);
    pitchTrackingEditor->setBounds (getWidth() - 225, 137, 64, 16);
    voiceLimitLabel2->setBounds ((getWidth() - 152) + -17, 72 + -19, 80, 24);
    crossfadeGroupEditor->setBounds (getWidth() - 152, 72, 64, 16);
    crossfadeEditor->setBounds (16, 174, getWidth() - 32, 112);
    voiceLimitLabel3->setBounds ((getWidth() - 81) + -22, 72 + -19, 80, 24);
    showCrossfadeLabel->setBounds (getWidth() - 81, 72, 64, 16);
    voiceLimitLabel4->setBounds (86 + 52, 103, 80, 24);
    purgeSampleEditor->setBounds (156, 122, 64, 16);
    channelAmountLabel3->setBounds (86 + 64, 71, 79, 24);
    purgeChannelLabel->setBounds (86 + 134 - 64, 90, 64, 16);
    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
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

    if (sliderThatWasMoved == diskSlider)
    {
        //[UserSliderCode_diskSlider] -- add your slider handling code here..
        //[/UserSliderCode_diskSlider]
    }

    //[UsersliderValueChanged_Post]
    //[/UsersliderValueChanged_Post]
}



//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...

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


//==============================================================================
#if 0
/*  -- Introjucer information section --

    This is where the Introjucer stores the metadata that describe this GUI layout, so
    make changes in here at your peril!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="SamplerSettings" componentName=""
                 parentClasses="public Component, public Timer" constructorParams="ModulatorSampler *s"
                 variableInitialisers="sampler(s)&#10;" snapPixels="8" snapActive="1"
                 snapShown="0" overlayOpacity="0.330" fixedSize="1" initialWidth="800"
                 initialHeight="170">
  <BACKGROUND backgroundColour="383838">
    <RECT pos="0Cc 74 222 14" fill="solid: 18ffffff" hasStroke="0"/>
    <RECT pos="9Rr 42 222 14" fill="solid: 18ffffff" hasStroke="0"/>
    <TEXT pos="8Rr 6 240 30" fill="solid: 70ffffff" hasStroke="0" text="SAMPLER SETTINGS"
          fontname="Arial" fontsize="20" bold="1" italic="0" justification="34"/>
    <RECT pos="8 8 210M 24" fill="solid: 13ffffff" hasStroke="1" stroke="1, mitered, butt"
          strokeColour="solid: fffffff"/>
    <RECT pos="8 56 224 88" fill="solid: 13ffffff" hasStroke="1" stroke="1, mitered, butt"
          strokeColour="solid: fffffff"/>
    <RECT pos="8Rr 41 224 55" fill="solid: 13ffffff" hasStroke="1" stroke="1, mitered, butt"
          strokeColour="solid: fffffff"/>
    <TEXT pos="16 49 200 30" fill="solid: 80ffffff" hasStroke="0" text="Disk IO Settings"
          fontname="Arial" fontsize="13" bold="1" italic="0" justification="36"/>
    <TEXT pos="220R 33 200 30" fill="solid: 80ffffff" hasStroke="0" text="Group Settings"
          fontname="Arial" fontsize="13" bold="1" italic="0" justification="36"/>
    <RECT pos="0Cc 73 224 55" fill="solid: 13ffffff" hasStroke="1" stroke="1, mitered, butt"
          strokeColour="solid: fffffff"/>
    <TEXT pos="0Cc 65 200 30" fill="solid: 80ffffff" hasStroke="0" text="Voice Settings"
          fontname="Arial" fontsize="13" bold="1" italic="0" justification="36"/>
    <RECT pos="8Rr 105 224 55" fill="solid: 13ffffff" hasStroke="1" stroke="1, mitered, butt"
          strokeColour="solid: fffffff"/>
    <TEXT pos="224R 97 208 30" fill="solid: 80ffffff" hasStroke="0" text="Playback Settings"
          fontname="Arial" fontsize="13" bold="1" italic="0" justification="36"/>
    <RECT pos="9Rr 106 222 14" fill="solid: 18ffffff" hasStroke="0"/>
    <RECT pos="9 57 222 14" fill="solid: 18ffffff" hasStroke="0"/>
  </BACKGROUND>
  <LABEL name="new label" id="f18e00eab8404cdf" memberName="fadeTimeLabel"
         virtualName="" explicitFocusOrder="0" pos="-14 -19 79 24" posRelativeX="9747f9d28c74d65d"
         posRelativeY="9747f9d28c74d65d" textCol="ffffffff" edTextCol="ff000000"
         edBkgCol="0" labelText="Fade Time" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Khmer UI" fontsize="13" bold="0"
         italic="0" justification="36"/>
  <LABEL name="new label" id="5836a90d75d1dd4a" memberName="voiceAmountLabel"
         virtualName="" explicitFocusOrder="0" pos="-4 -19 79 24" posRelativeX="fa0dc77af8626dc7"
         posRelativeY="fa0dc77af8626dc7" textCol="ffffffff" edTextCol="ff000000"
         edBkgCol="0" labelText="Amount" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Arial" fontsize="12" bold="0"
         italic="0" justification="33"/>
  <LABEL name="new label" id="27f7495f6d3cb953" memberName="label2" virtualName=""
         explicitFocusOrder="0" pos="80 71 88 24" textCol="ffffffff" edTextCol="ff000000"
         edBkgCol="0" labelText="Preload Size" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Arial"
         fontsize="12" bold="0" italic="0" justification="33"/>
  <LABEL name="new label" id="6e29be3815b724b" memberName="label" virtualName=""
         explicitFocusOrder="0" pos="11 71 79 24" textCol="ffffffff" edTextCol="ff000000"
         edBkgCol="0" labelText="Buffer Size" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Arial"
         fontsize="12" bold="0" italic="0" justification="33"/>
  <LABEL name="new label" id="93e47367da77934" memberName="label4" virtualName=""
         explicitFocusOrder="0" pos="80 103 88 24" textCol="ffffffff"
         edTextCol="ff000000" edBkgCol="0" labelText="Disk Usage" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Arial"
         fontsize="12" bold="0" italic="0" justification="33"/>
  <LABEL name="new label" id="15696cd8cb34fa4c" memberName="label3" virtualName=""
         explicitFocusOrder="0" pos="11 103 88 24" textCol="ffffffff"
         edTextCol="ff000000" edBkgCol="0" labelText="Memory" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Arial"
         fontsize="12" bold="0" italic="0" justification="33"/>
  <LABEL name="new label" id="45354151fdeccf85" memberName="fadeTimeLabel2"
         virtualName="" explicitFocusOrder="0" pos="-6 -18 82 24" posRelativeX="3ca481f4230f2188"
         posRelativeY="3ca481f4230f2188" textCol="ffffffff" edTextCol="ff000000"
         edBkgCol="0" labelText="Retrigger" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Khmer UI" fontsize="13" bold="0"
         italic="0" justification="33"/>
  <LABEL name="new label" id="36855d0c4bd35f7b" memberName="voiceLimitLabel"
         virtualName="" explicitFocusOrder="0" pos="-16 -19 80 24" posRelativeX="d52b7a0ef81e7b8a"
         posRelativeY="d52b7a0ef81e7b8a" textCol="ffffffff" edTextCol="ff000000"
         edBkgCol="0" labelText="Soft Limit" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Khmer UI" fontsize="13" bold="0"
         italic="0" justification="36"/>
  <LABEL name="new label" id="ecf08aed0630701" memberName="bufferSizeEditor"
         virtualName="" explicitFocusOrder="0" pos="17 90 64 16" bkgCol="38ffffff"
         outlineCol="38ffffff" edTextCol="ff000000" edBkgCol="0" hiliteCol="407a0000"
         labelText="100000" editableSingleClick="1" editableDoubleClick="1"
         focusDiscardsChanges="0" fontname="Khmer UI" fontsize="14" bold="0"
         italic="0" justification="33"/>
  <LABEL name="new label" id="706157b8bee823c6" memberName="preloadBufferEditor"
         virtualName="" explicitFocusOrder="0" pos="86 90 64 16" bkgCol="38ffffff"
         outlineCol="38ffffff" edTextCol="ff000000" edBkgCol="0" hiliteCol="40750000"
         labelText="100000" editableSingleClick="1" editableDoubleClick="1"
         focusDiscardsChanges="0" fontname="Khmer UI" fontsize="14" bold="0"
         italic="0" justification="33"/>
  <LABEL name="new label" id="ca5597cc875c60f4" memberName="memoryUsageLabel"
         virtualName="" explicitFocusOrder="0" pos="17 122 64 16" bkgCol="38ffffff"
         outlineCol="38ffffff" edTextCol="ff000000" edBkgCol="0" hiliteCol="40750000"
         labelText="0.52MB&#10;" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Arial" fontsize="13" bold="0"
         italic="0" justification="33"/>
  <SLIDER name="new slider" id="29782626e6a3657a" memberName="diskSlider"
          virtualName="" explicitFocusOrder="0" pos="86 122 64 16" bkgcol="38ffffff"
          thumbcol="b3680000" rotaryslideroutline="ff000000" textboxoutline="38ffffff"
          min="0" max="100" int="0.10000000000000001" style="LinearBar"
          textBoxPos="TextBoxLeft" textBoxEditable="0" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1"/>
  <LABEL name="new label" id="fa0dc77af8626dc7" memberName="voiceAmountEditor"
         virtualName="" explicitFocusOrder="0" pos="-40Cr 107 64 16" bkgCol="38ffffff"
         outlineCol="38ffffff" edTextCol="ff000000" edBkgCol="0" hiliteCol="407a0000"
         labelText="64" editableSingleClick="1" editableDoubleClick="1"
         focusDiscardsChanges="0" fontname="Khmer UI" fontsize="14" bold="0"
         italic="0" justification="33"/>
  <LABEL name="new label" id="d52b7a0ef81e7b8a" memberName="voiceLimitEditor"
         virtualName="" explicitFocusOrder="0" pos="31Cr 107 64 16" bkgCol="38ffffff"
         outlineCol="38ffffff" edTextCol="ff000000" edBkgCol="0" hiliteCol="40750000"
         labelText="16" editableSingleClick="1" editableDoubleClick="1"
         focusDiscardsChanges="0" fontname="Khmer UI" fontsize="14" bold="0"
         italic="0" justification="33"/>
  <LABEL name="new label" id="9747f9d28c74d65d" memberName="fadeTimeEditor"
         virtualName="" explicitFocusOrder="0" pos="40C 106 64 16" bkgCol="38ffffff"
         outlineCol="38ffffff" edTextCol="ff000000" edBkgCol="0" hiliteCol="407a0000"
         labelText="15 ms" editableSingleClick="1" editableDoubleClick="1"
         focusDiscardsChanges="0" fontname="Khmer UI" fontsize="14" bold="0"
         italic="0" justification="33"/>
  <LABEL name="new label" id="3ca481f4230f2188" memberName="retriggerEditor"
         virtualName="PopupLabel" explicitFocusOrder="0" pos="154R 137 64 16"
         bkgCol="38ffffff" outlineCol="38ffffff" edTextCol="ff000000"
         edBkgCol="0" hiliteCol="407a0000" labelText="Kill note" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Khmer UI"
         fontsize="14" bold="0" italic="0" justification="33"/>
  <LABEL name="new label" id="9399aaa6d6cf50f" memberName="voiceAmountLabel2"
         virtualName="" explicitFocusOrder="0" pos="-14 -19 79 24" posRelativeX="1dc59b70597350aa"
         posRelativeY="1dc59b70597350aa" textCol="ffffffff" edTextCol="ff000000"
         edBkgCol="0" labelText="RR Groups" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Khmer UI" fontsize="13" bold="0"
         italic="0" justification="36"/>
  <LABEL name="new label" id="1dc59b70597350aa" memberName="rrGroupEditor"
         virtualName="" explicitFocusOrder="0" pos="223R 72 64 16" bkgCol="38ffffff"
         outlineCol="38ffffff" edTextCol="ff000000" edBkgCol="0" hiliteCol="407a0000"
         labelText="0" editableSingleClick="1" editableDoubleClick="1"
         focusDiscardsChanges="0" fontname="Khmer UI" fontsize="14" bold="0"
         italic="0" justification="33"/>
  <LABEL name="new label" id="6fad51908fbcf5c8" memberName="playbackModeDescription"
         virtualName="" explicitFocusOrder="0" pos="-6 -19 82 24" posRelativeX="fd217ec034194bd"
         posRelativeY="fd217ec034194bd" textCol="ffffffff" edTextCol="ff000000"
         edBkgCol="0" labelText="Playback" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Khmer UI" fontsize="13" bold="0"
         italic="0" justification="33"/>
  <LABEL name="new label" id="fd217ec034194bd" memberName="playbackEditor"
         virtualName="PopupLabel" explicitFocusOrder="0" pos="18Rr 137 64 16"
         bkgCol="38ffffff" outlineCol="38ffffff" edTextCol="ff000000"
         edBkgCol="0" hiliteCol="407a0000" labelText="Normal" editableSingleClick="1"
         editableDoubleClick="1" focusDiscardsChanges="0" fontname="Khmer UI"
         fontsize="14" bold="0" italic="0" justification="33"/>
  <LABEL name="new label" id="458c616ebaf25e48" memberName="playbackModeDescription2"
         virtualName="" explicitFocusOrder="0" pos="-5 -19 82 24" posRelativeX="ed02f63655808fc0"
         posRelativeY="ed02f63655808fc0" textCol="ffffffff" edTextCol="ff000000"
         edBkgCol="0" labelText="Pitch Track" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Khmer UI"
         fontsize="13" bold="0" italic="0" justification="33"/>
  <LABEL name="new label" id="ed02f63655808fc0" memberName="pitchTrackingEditor"
         virtualName="PopupLabel" explicitFocusOrder="0" pos="225R 137 64 16"
         bkgCol="38ffffff" outlineCol="38ffffff" edTextCol="ff000000"
         edBkgCol="0" hiliteCol="407a0000" labelText="Enabled" editableSingleClick="1"
         editableDoubleClick="1" focusDiscardsChanges="0" fontname="Khmer UI"
         fontsize="14" bold="0" italic="0" justification="33"/>
  <LABEL name="new label" id="6bae51db35d24551" memberName="voiceLimitLabel2"
         virtualName="" explicitFocusOrder="0" pos="-17 -19 80 24" posRelativeX="4c996c81e26245e3"
         posRelativeY="4c996c81e26245e3" textCol="ffffffff" edTextCol="ff000000"
         edBkgCol="0" labelText="Group XF" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Arial" fontsize="12" bold="0"
         italic="0" justification="36"/>
  <LABEL name="new label" id="4c996c81e26245e3" memberName="crossfadeGroupEditor"
         virtualName="PopupLabel" explicitFocusOrder="0" pos="152R 72 64 16"
         bkgCol="38ffffff" outlineCol="38ffffff" edTextCol="ff000000"
         edBkgCol="0" hiliteCol="407a0000" labelText="Enabled" editableSingleClick="1"
         editableDoubleClick="1" focusDiscardsChanges="0" fontname="Khmer UI"
         fontsize="14" bold="0" italic="0" justification="33"/>
  <GENERICCOMPONENT name="new component" id="86c524f43e825eb1" memberName="crossfadeEditor"
                    virtualName="" explicitFocusOrder="0" pos="16 174 32M 112" class="TableEditor"
                    params="dynamic_cast&lt;ModulatorSampler*&gt;(getProcessor())-&gt;getTable(0)"/>
  <LABEL name="new label" id="f91720efd0acc16d" memberName="voiceLimitLabel3"
         virtualName="" explicitFocusOrder="0" pos="-22 -19 80 24" posRelativeX="272d2355f207a0ae"
         posRelativeY="272d2355f207a0ae" textCol="ffffffff" edTextCol="ff000000"
         edBkgCol="0" labelText="Edit XF" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Arial" fontsize="12" bold="0"
         italic="0" justification="36"/>
  <LABEL name="new label" id="272d2355f207a0ae" memberName="showCrossfadeLabel"
         virtualName="PopupLabel" explicitFocusOrder="0" pos="81R 72 64 16"
         bkgCol="38ffffff" outlineCol="38ffffff" edTextCol="ff000000"
         edBkgCol="0" hiliteCol="407a0000" labelText="Group 1" editableSingleClick="1"
         editableDoubleClick="1" focusDiscardsChanges="0" fontname="Khmer UI"
         fontsize="14" bold="0" italic="0" justification="33"/>
  <LABEL name="new label" id="1d2be8e421141da0" memberName="voiceLimitLabel4"
         virtualName="" explicitFocusOrder="0" pos="52 103 80 24" posRelativeX="29782626e6a3657a"
         textCol="ffffffff" edTextCol="ff000000" edBkgCol="0" labelText="Purge All"
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Arial" fontsize="12" bold="0" italic="0" justification="36"/>
  <LABEL name="new label" id="a5007a767ca7c32f" memberName="purgeSampleEditor"
         virtualName="PopupLabel" explicitFocusOrder="0" pos="156 122 64 16"
         bkgCol="38ffffff" outlineCol="38ffffff" edTextCol="ff000000"
         edBkgCol="0" hiliteCol="407a0000" labelText="Enabled" editableSingleClick="1"
         editableDoubleClick="1" focusDiscardsChanges="0" fontname="Khmer UI"
         fontsize="14" bold="0" italic="0" justification="33"/>
  <LABEL name="new label" id="18b21c3acc428ff8" memberName="channelAmountLabel3"
         virtualName="" explicitFocusOrder="0" pos="64 71 79 24" posRelativeX="706157b8bee823c6"
         textCol="ffffffff" edTextCol="ff000000" edBkgCol="0" labelText="Purge Channel"
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Arial" fontsize="13" bold="0" italic="0" justification="33"/>
  <LABEL name="new label" id="c939f8106478c094" memberName="purgeChannelLabel"
         virtualName="PopupLabel" explicitFocusOrder="0" pos="134r 90 64 16"
         posRelativeX="706157b8bee823c6" bkgCol="38ffffff" outlineCol="38ffffff"
         edTextCol="ff000000" edBkgCol="0" hiliteCol="407a0000" labelText="Rewrite"
         editableSingleClick="1" editableDoubleClick="1" focusDiscardsChanges="0"
         fontname="Khmer UI" fontsize="14" bold="0" italic="0" justification="33"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
} // namespace hise
//[/EndFile]
