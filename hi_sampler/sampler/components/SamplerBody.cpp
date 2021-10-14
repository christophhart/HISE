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

#include "SamplerBody.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
SamplerBody::SamplerBody (ProcessorEditor *p)
    : ProcessorEditorBody(p)
{
    //[Constructor_pre] You can add your own custom stuff here..
    //[/Constructor_pre]

    addAndMakeVisible(buttonRow = new Component());
    
    addAndMakeVisible (sampleEditor = new SampleEditor (dynamic_cast<ModulatorSampler*>(getProcessor()), this));
    sampleEditor->setName ("new component");

    addAndMakeVisible (soundTable = new SamplerTable (dynamic_cast<ModulatorSampler*>(getProcessor()), this));
    soundTable->setName ("new component");

    buttonRow->addAndMakeVisible (waveFormButton = new TextButton ("new button"));
    waveFormButton->setButtonText (TRANS("Sample Editor"));
    waveFormButton->setConnectedEdges (Button::ConnectedOnLeft | Button::ConnectedOnRight);
    waveFormButton->addListener (this);
    waveFormButton->setColour (TextButton::buttonColourId, Colours::white);
    waveFormButton->setColour (TextButton::buttonOnColourId, Colour (0xff680000));
    waveFormButton->setColour (TextButton::textColourOnId, Colour (0x99ffffff));
    waveFormButton->setColour (TextButton::textColourOffId, Colours::white);

    buttonRow->addAndMakeVisible (mapButton = new TextButton ("new button"));
    mapButton->setButtonText (TRANS("Map Editor"));
    mapButton->setConnectedEdges (Button::ConnectedOnLeft | Button::ConnectedOnRight);
    mapButton->addListener (this);
    mapButton->setColour (TextButton::buttonColourId, Colour (0xff606060));
    mapButton->setColour (TextButton::buttonOnColourId, Colour (0xff680000));
    mapButton->setColour (TextButton::textColourOnId, Colour (0x99ffffff));
    mapButton->setColour (TextButton::textColourOffId, Colours::white);

    buttonRow->addAndMakeVisible (tableButton = new TextButton ("new button"));
    tableButton->setButtonText (TRANS("Table View"));
    tableButton->setConnectedEdges (Button::ConnectedOnLeft);
    tableButton->addListener (this);
    tableButton->setColour (TextButton::buttonColourId, Colour (0xff606060));
    tableButton->setColour (TextButton::buttonOnColourId, Colour (0xff680000));
    tableButton->setColour (TextButton::textColourOnId, Colour (0x99ffffff));
    tableButton->setColour (TextButton::textColourOffId, Colours::white);

    buttonRow->addAndMakeVisible (settingsView = new TextButton ("new button"));
    settingsView->setButtonText (TRANS("Sampler Settings"));
    settingsView->setConnectedEdges (Button::ConnectedOnRight);
    settingsView->addListener (this);
    settingsView->setColour (TextButton::buttonColourId, Colour (0xff606060));
    settingsView->setColour (TextButton::buttonOnColourId, Colour (0xff680000));
    settingsView->setColour (TextButton::textColourOnId, Colour (0x99ffffff));
    settingsView->setColour (TextButton::textColourOffId, Colours::white);

    addAndMakeVisible (settingsPanel = new SamplerSettings (dynamic_cast<ModulatorSampler*>(getProcessor())));
    addAndMakeVisible (map = new SampleMapEditor (dynamic_cast<ModulatorSampler*>(getProcessor()), this));

    //[UserPreSize]

    buttonRow->setBufferedToImage(true);
    
	waveFormButton->setLookAndFeel(&cblaf);
	settingsView->setLookAndFeel(&cblaf);
	mapButton->setLookAndFeel(&cblaf);
	tableButton->setLookAndFeel(&cblaf);

	internalChange = false;

	settingsPanel->setVisible(getProcessor()->getEditorState(ModulatorSampler::SettingsShown));
	settingsView->setToggleState(getProcessor()->getEditorState(ModulatorSampler::SettingsShown), dontSendNotification);


	sampleEditor->setVisible(getProcessor()->getEditorState(ModulatorSampler::WaveformShown));
	waveFormButton->setToggleState(getProcessor()->getEditorState(ModulatorSampler::WaveformShown), dontSendNotification);

	map->setVisible(getProcessor()->getEditorState(ModulatorSampler::MapPanelShown));
	mapButton->setToggleState(getProcessor()->getEditorState(ModulatorSampler::MapPanelShown), dontSendNotification);

	soundTable->setVisible(getProcessor()->getEditorState(ModulatorSampler::TableShown));
	tableButton->setToggleState(getProcessor()->getEditorState(ModulatorSampler::TableShown), dontSendNotification);

	map->addMouseListener(this, false);
	sampleEditor->addMouseListener(this, false);

	setWantsKeyboardFocus(true);

    setSize (900, 700);

	h = 36;

	Button *t = waveFormButton;

	t->setColour (TextButton::buttonColourId, Colour (0x884b4b4b));
	t->setColour (TextButton::buttonOnColourId, Colour(Colours::white));
	t->setColour (TextButton::textColourOnId, Colour (0xaa000000));
	t->setColour (TextButton::textColourOffId, Colour (0x99ffffff));

	t = mapButton;

	t->setColour (TextButton::buttonColourId, Colour (0x884b4b4b));
	t->setColour (TextButton::buttonOnColourId, Colour(Colours::white));
	t->setColour (TextButton::textColourOnId, Colour (0xaa000000));
	t->setColour (TextButton::textColourOffId, Colour (0x99ffffff));

	t = tableButton;

	t->setColour (TextButton::buttonColourId, Colour (0x884b4b4b));
	t->setColour (TextButton::buttonOnColourId, Colour(Colours::white));
	t->setColour (TextButton::textColourOnId, Colour (0xaa000000));
	t->setColour (TextButton::textColourOffId, Colour (0x99ffffff));

	t = settingsView;

	t->setColour (TextButton::buttonColourId, Colour (0x884b4b4b));
	t->setColour (TextButton::buttonOnColourId, Colour(Colours::white));
	t->setColour (TextButton::textColourOnId, Colour (0xaa000000));
	t->setColour (TextButton::textColourOffId, Colour (0x99ffffff));

    //[/Constructor]
}

SamplerBody::~SamplerBody()
{
    //[Destructor_pre]. You can add your own custom destruction code here..

    //[/Destructor_pre]

    sampleEditor = nullptr;
    soundTable = nullptr;
    waveFormButton = nullptr;
    mapButton = nullptr;
    tableButton = nullptr;
    settingsView = nullptr;
    settingsPanel = nullptr;
    map = nullptr;


    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void SamplerBody::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
	g.fillAll(Colours::transparentBlack);
    
    //[/UserPrePaint]

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void SamplerBody::resized()
{
    //[UserPreResize] Add your own custom resize code here..

    buttonRow->setBounds(0, 0, getWidth(), 26);


    //[/UserPreResize]

    sampleEditor->setBounds ((getWidth() / 2) - ((getWidth() - 32) / 2), 317, getWidth() - 32, 250);
    soundTable->setBounds ((getWidth() / 2) - ((getWidth() - 32) / 2), 350, getWidth() - 32, 300);
    waveFormButton->setBounds ((getWidth() / 2) - 128, 6, 128, 20);
    mapButton->setBounds (((getWidth() / 2) - 128) + 128, 6, 128, 20);
    tableButton->setBounds ((((getWidth() / 2) - 128) + 128) + 128, 6, 128, 20);
    settingsView->setBounds (((getWidth() / 2) - 128) + 0 - 128, 6, 128, 20);
    settingsPanel->setBounds ((getWidth() / 2) - ((getWidth() - 32) / 2), 48, getWidth() - 32, 256);
    map->setBounds ((getWidth() / 2) - ((getWidth() - 32) / 2), 564, getWidth() - 32, 245);
    //[UserResized] Add your own custom resize handling here..


	settingsHeight = getProcessor()->getEditorState(ModulatorSampler::SettingsShown) ? settingsPanel->getPanelHeight(): 0;

	settingsPanel->setSize(settingsPanel->getWidth(), settingsHeight);

	waveFormHeight = getProcessor()->getEditorState(ModulatorSampler::WaveformShown) ? sampleEditor->getHeight(): 0;

	mapHeight = getProcessor()->getEditorState(ModulatorSampler::MapPanelShown) ? map->getHeight(): 0;
	const int additionalMapHeight = getProcessor()->getEditorState(getProcessor()->getEditorStateForIndex(ModulatorSampler::EditorStates::BigSampleMap)) ? 128 : 0;

	if (mapHeight != 0)
	{


		map->setSize(map->getWidth(), mapHeight + additionalMapHeight);

	}

	tableHeight = getProcessor()->getEditorState(ModulatorSampler::TableShown) ? soundTable->getHeight(): 0;

	int y = h;

	if(settingsHeight != 0)
	{
		settingsPanel->setTopLeftPosition(settingsPanel->getX(), y);
		y += settingsHeight;
	}

	if(waveFormHeight != 0)
	{
		sampleEditor->setTopLeftPosition(sampleEditor->getX(), y);
		y += waveFormHeight;
	}

	if(mapHeight != 0)
	{
		map->setTopLeftPosition(map->getX(), y);
		y += mapHeight + additionalMapHeight;
	}

	if(tableHeight != 0)
	{
		soundTable->setTopLeftPosition(soundTable->getX(), y);
		y += tableHeight;
	}
    //[/UserResized]
}

void SamplerBody::buttonClicked (Button* buttonThatWasClicked)
{
    //[UserbuttonClicked_Pre]

	const bool visible = !toggleButton(buttonThatWasClicked);

    //[/UserbuttonClicked_Pre]

    if (buttonThatWasClicked == waveFormButton)
    {
        //[UserButtonCode_waveFormButton] -- add your button handler code here..

		sampleEditor->setVisible(visible);
		getProcessor()->setEditorState(ModulatorSampler::WaveformShown, visible);

        //[/UserButtonCode_waveFormButton]
    }
    else if (buttonThatWasClicked == mapButton)
    {
        //[UserButtonCode_mapButton] -- add your button handler code here..

		map->setVisible(visible);
		getProcessor()->setEditorState(ModulatorSampler::MapPanelShown, visible);

        //[/UserButtonCode_mapButton]
    }
    else if (buttonThatWasClicked == tableButton)
    {
        //[UserButtonCode_tableButton] -- add your button handler code here..

		soundTable->setVisible(visible);
		getProcessor()->setEditorState(ModulatorSampler::TableShown, visible);

        //[/UserButtonCode_tableButton]
    }
    else if (buttonThatWasClicked == settingsView)
    {
        //[UserButtonCode_settingsView] -- add your button handler code here..

		settingsPanel->setVisible(visible);
		getProcessor()->setEditorState(ModulatorSampler::SettingsShown, visible);

        //[/UserButtonCode_settingsView]
    }

    //[UserbuttonClicked_Post]

	resized();
	this->refreshBodySize();

    //[/UserbuttonClicked_Post]
}



//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...



//[/MiscUserCode]


//==============================================================================
#if 0
/*  -- Introjucer information section --

    This is where the Introjucer stores the metadata that describe this GUI layout, so
    make changes in here at your peril!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="SamplerBody" componentName=""
                 parentClasses="public ProcessorEditorBody" constructorParams="ProcessorEditor *p"
                 variableInitialisers="ProcessorEditorBody(p)" snapPixels="8"
                 snapActive="1" snapShown="1" overlayOpacity="0.330" fixedSize="1"
                 initialWidth="900" initialHeight="700">
  <BACKGROUND backgroundColour="ffffff"/>
  <GENERICCOMPONENT name="new component" id="11e50644daffb36a" memberName="sampleEditor"
                    virtualName="" explicitFocusOrder="0" pos="0Cc 317 32M 250" class="SampleEditor"
                    params="dynamic_cast&lt;ModulatorSampler*&gt;(getProcessor()), this"/>
  <GENERICCOMPONENT name="new component" id="6c4bc39d031db517" memberName="soundTable"
                    virtualName="" explicitFocusOrder="0" pos="0Cc 350 32M 300" class="SamplerTable"
                    params="dynamic_cast&lt;ModulatorSampler*&gt;(getProcessor()), this"/>
  <TEXTBUTTON name="new button" id="dd59e008f4585fa5" memberName="waveFormButton"
              virtualName="" explicitFocusOrder="0" pos="0Cr 6 128 20" bgColOff="ff606060"
              bgColOn="ff680000" textCol="99ffffff" textColOn="ffffffff" buttonText="Sample Editor"
              connectedEdges="3" needsCallback="1" radioGroupId="0"/>
  <TEXTBUTTON name="new button" id="e2f768648a24323c" memberName="mapButton"
              virtualName="" explicitFocusOrder="0" pos="0R 6 128 20" posRelativeX="dd59e008f4585fa5"
              bgColOff="ff606060" bgColOn="ff680000" textCol="99ffffff" textColOn="ffffffff"
              buttonText="Map Editor" connectedEdges="3" needsCallback="1"
              radioGroupId="0"/>
  <TEXTBUTTON name="new button" id="ea22abb7d2519bbc" memberName="tableButton"
              virtualName="" explicitFocusOrder="0" pos="0R 6 128 20" posRelativeX="e2f768648a24323c"
              bgColOff="ff606060" bgColOn="ff680000" textCol="99ffffff" textColOn="ffffffff"
              buttonText="Table View" connectedEdges="1" needsCallback="1"
              radioGroupId="0"/>
  <TEXTBUTTON name="new button" id="51c4da4f73142797" memberName="settingsView"
              virtualName="" explicitFocusOrder="0" pos="0r 6 128 20" posRelativeX="dd59e008f4585fa5"
              bgColOff="ff606060" bgColOn="ff680000" textCol="99ffffff" textColOn="ffffffff"
              buttonText="Sampler Settings" connectedEdges="2" needsCallback="1"
              radioGroupId="0"/>
  <JUCERCOMP name="" id="711664091e076bba" memberName="settingsPanel" virtualName=""
             explicitFocusOrder="0" pos="0Cc 48 32M 256" sourceFile="SamplerSettings.cpp"
             constructorParams="dynamic_cast&lt;ModulatorSampler*&gt;(getProcessor())"/>
  <JUCERCOMP name="" id="5aa8cd46870e9477" memberName="map" virtualName=""
             explicitFocusOrder="0" pos="0Cc 564 32M 245" sourceFile="SampleMapEditor.cpp"
             constructorParams="dynamic_cast&lt;ModulatorSampler*&gt;(getProcessor()), this"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...

} // namespace hise
//[/EndFile]
