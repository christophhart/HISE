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
//[/Headers]

#include "SampleEditor.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
SampleEditor::SampleEditor (ModulatorSampler *s, SamplerBody *b):
	SamplerSubEditor(s->getSampleEditHandler()),
	sampler(s)
{
    //[Constructor_pre] You can add your own custom stuff here..
    //[/Constructor_pre]

    addAndMakeVisible (viewport = new Viewport ("new viewport"));
    viewport->setScrollBarsShown (false, true);

    addAndMakeVisible (volumeSetter = new ValueSettingComponent());
    addAndMakeVisible (pitchSetter = new ValueSettingComponent());
    addAndMakeVisible (sampleStartSetter = new ValueSettingComponent());
    addAndMakeVisible (sampleEndSetter = new ValueSettingComponent());
    addAndMakeVisible (loopStartSetter = new ValueSettingComponent());
    addAndMakeVisible (loopEndSetter = new ValueSettingComponent());
    addAndMakeVisible (loopCrossfadeSetter = new ValueSettingComponent());
    addAndMakeVisible (startModulationSetter = new ValueSettingComponent());
    addAndMakeVisible (toolbar = new Toolbar());
    toolbar->setName ("new component");

    addAndMakeVisible (panSetter = new ValueSettingComponent());

    //[UserPreSize]

	viewport->setViewedComponent(currentWaveForm = new SamplerSoundWaveform(sampler), false);

	currentWaveForm->setVisible(true);

	zoomFactor = 1.0f;

	body = b;

	samplerEditorCommandManager = new ApplicationCommandManager();

	samplerEditorCommandManager->registerAllCommandsForTarget(this);
	samplerEditorCommandManager->getKeyMappings()->resetToDefaultMappings();

	samplerEditorCommandManager->setFirstCommandTarget(this);

	toolbarFactory = new SampleEditorToolbarFactory(this);

	toolbar->setStyle(Toolbar::ToolbarItemStyle::iconsOnly);
	toolbar->addDefaultItems(*toolbarFactory);

	toolbar->setColour(Toolbar::ColourIds::backgroundColourId, Colours::transparentBlack);
	toolbar->setColour(Toolbar::ColourIds::buttonMouseOverBackgroundColourId, Colours::white.withAlpha(0.3f));
	toolbar->setColour(Toolbar::ColourIds::buttonMouseDownBackgroundColourId, Colours::white.withAlpha(0.4f));

	panSetter->setPropertyType(ModulatorSamplerSound::Pan);
	volumeSetter->setPropertyType(ModulatorSamplerSound::Volume);
	pitchSetter->setPropertyType(ModulatorSamplerSound::Pitch);
	sampleStartSetter->setPropertyType(ModulatorSamplerSound::SampleStart);
	sampleEndSetter->setPropertyType(ModulatorSamplerSound::SampleEnd);
	startModulationSetter->setPropertyType(ModulatorSamplerSound::SampleStartMod);
	loopStartSetter->setPropertyType(ModulatorSamplerSound::LoopStart);
	loopEndSetter->setPropertyType(ModulatorSamplerSound::LoopEnd);
	loopCrossfadeSetter->setPropertyType(ModulatorSamplerSound::LoopXFade);

	loopStartSetter->setLabelColour(Colours::green.withAlpha(0.1f), Colours::white);
	loopEndSetter->setLabelColour(Colours::green.withAlpha(0.1f), Colours::white);
	loopCrossfadeSetter->setLabelColour(Colours::yellow.withAlpha(0.1f), Colours::white);
	startModulationSetter->setLabelColour(Colours::blue.withAlpha(0.1f), Colours::white);


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


    //[/Constructor]
}

SampleEditor::~SampleEditor()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
	samplerEditorCommandManager->setFirstCommandTarget(nullptr);
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

//==============================================================================
void SampleEditor::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..

	int x = 0;
	int y = 2;
	int width = getWidth();
	int height = getHeight() - 4;

	Rectangle<int> a(x, y, width, height);

	ProcessorEditorLookAndFeel::drawShadowBox(g, a, Colour(0xFF333333));

    //[/UserPrePaint]

    g.setColour (Colour (0x13ffffff));
    g.fillRect (8, 8, getWidth() - 175, 24);

    g.setColour (Colour (0x0fffffff));
    g.drawRect (8, 8, getWidth() - 175, 24, 1);

    g.setColour (Colour (0xccffffff));
	g.setFont(GLOBAL_BOLD_FONT().withHeight(22.0f));
    g.drawText (TRANS("SAMPLE EDITOR"),
                getWidth() - 8 - 232, 4, 232, 30,
                Justification::centredRight, true);

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void SampleEditor::resized()
{
    
	int width = 150;
	int halfWidth = 75;
	int setterHeight = 32;

	int y1 = getHeight() - 50;
	int y2 = y1 - 50;

	int viewportY = 41;
	int viewportHeight = y2 - 32;

	int widthOfSetters = width * 3 + halfWidth * 2;

	int x = (getWidth() - widthOfSetters) / 2;

    viewport->setBounds (8, viewportY, getWidth() - 16, viewportHeight);

	volumeSetter->setBounds(x, y1, width-5, setterHeight);
	
	panSetter->setBounds(x, y2, halfWidth-5, setterHeight);
	x += halfWidth;

	pitchSetter->setBounds(x, y2, halfWidth-5, setterHeight);
	x += halfWidth;

	sampleStartSetter->setBounds(x, y1, width-5, setterHeight);
	sampleEndSetter->setBounds(x, y2, width-5, setterHeight);

	x += width;

	loopStartSetter->setBounds(x, y1, width-5, setterHeight);
	loopEndSetter->setBounds(x, y2, width-5, setterHeight);

	x += width;

	startModulationSetter->setBounds(x, y1, width-5, setterHeight);
	loopCrossfadeSetter->setBounds(x, y2, width-5, setterHeight);

#if 0
    volumeSetter->setBounds (proportionOfWidth (0.3150f) - proportionOfWidth (0.1400f), 162, proportionOfWidth (0.1400f), 32);
    pitchSetter->setBounds ((proportionOfWidth (0.3150f) - proportionOfWidth (0.1400f)) + 61, 195, proportionOfWidth (0.1000f), 32);
    sampleStartSetter->setBounds ((getWidth() / 2) + 4 - proportionOfWidth (0.1400f), 162, proportionOfWidth (0.1400f), 32);
    sampleEndSetter->setBounds (((getWidth() / 2) + 4 - proportionOfWidth (0.1400f)) + 0, 196, proportionOfWidth (0.1400f), 32);
    loopStartSetter->setBounds (proportionOfWidth (0.5188f), 163, proportionOfWidth (0.1400f), 32);
    loopEndSetter->setBounds (proportionOfWidth (0.5188f) + 0, 196, proportionOfWidth (0.1400f), 32);
    loopCrossfadeSetter->setBounds (((getWidth() / 2) + 139) + roundFloatToInt (proportionOfWidth (0.1400f) * 0.0000f), 196, proportionOfWidth (0.1400f), 32);
    startModulationSetter->setBounds ((getWidth() / 2) + 139, 160, proportionOfWidth (0.1400f), 32);
	panSetter->setBounds(proportionOfWidth(0.2388f) - proportionOfWidth(0.1000f), 195, proportionOfWidth(0.1000f), 32);
#endif
    toolbar->setBounds (12, 10, getWidth() - 175, 20);
    
    

	currentWaveForm->setSize((int)(viewport->getWidth() * zoomFactor), viewport->getHeight() - (viewport->isHorizontalScrollBarShown() ? viewport->getScrollBarThickness() : 0));

    
}



//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...

bool SampleEditor::perform (const InvocationInfo &info)
{

	switch(info.commandID)
	{
	case NormalizeVolume:  SampleEditHandler::SampleEditingActions::normalizeSamples(handler, this); return true;
	case LoopEnabled:	   {for(int i = 0; i < selection.size(); i++)
						   {
							   selection[i]->toggleBoolProperty(ModulatorSamplerSound::LoopEnabled);
						   };

						   const bool isOn = (selection.size() != 0) ? (bool)selection.getLast()->getProperty(ModulatorSamplerSound::LoopEnabled) : false;

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
	}
	return false;
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
//[/EndFile]
