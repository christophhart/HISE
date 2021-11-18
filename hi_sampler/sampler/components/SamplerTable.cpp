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

#include "SamplerTable.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...

MARKDOWN_CHAPTER(SamplerTableHelp)
START_MARKDOWN(RegexHelp)
ML("# RegEx Cheatsheet");
ML("- Use the regex parser as a normal wildcard search engine until it does not do what you want.");
ML("- Select all with `.`");
ML("- if you want to subtract something from the selection, use the prefix `sub:` (this is not official regex, but I added it for simpler handling)");
ML("- if you want to add something to the selection but keep the other sounds selected, use the prefix `add:`");
ML("- you can use logical operators (and: `&`, or: `|`) with multiple keywords");
ML("- the end of the filename can be checked with `$`, the start of the filename can be checked with `^`.");

ML("## Isolate a token in the file name")
ML("If you want to check one specific token only (tokens are supposed to be parts of the filename that are divided by the separation character (eg. `_`), you can use this expression:");

ML_START_CODE();
ML("^.*_.*[Interesting part].*_.*");
ML("1  2                      3");
ML_END_CODE();

ML("Whatever you enter into `[Interesting part]` will be checked only against the second token (if `_` is our separation character of course). If you have a (rather bad) filename structure like this:");

ML_START_CODE()
ML("Sample_3_50_1_127.wav");
ML_END_CODE()

ML("where there are only numbers, you can use this trick to still get the samples you want.  ");
ML("Let's say the first number is the round robin group and we want to change the volume of the second round robin group by -6db. Just using `2` would cause all kinds of mayhem. Using `_2_` would be a little bit better, but there is no guarantee that the other tokens are not this value. The solution for this case would be:");

ML_START_CODE();
ML("^.*_2_.*");
ML_END_CODE();

ML("There are two regex elements used in this expression: the beginning of the filename character `^` and the combination `.*` which simply means");
ML("> an arbitrary amount of any character.");
ML("Translated into english this expression would be:");
ML("> Any amount of characters after the beginning of the file up to the first underscore, then exactly `2`, then another underscore followed by anything.");
END_MARKDOWN()
END_MARKDOWN_CHAPTER()

//[/MiscUserDefs]

//==============================================================================
SamplerTable::SamplerTable (ModulatorSampler *s, SamplerBody *b):
	SamplerSubEditor(s->getSampleEditHandler()),
	sampler(s),
    body(b)
{
    //[Constructor_pre] You can add your own custom stuff here..
    //[/Constructor_pre]

    addAndMakeVisible (table = new SamplerSoundTable (s, handler));
    table->setName ("new component");

    addAndMakeVisible (searchLabel = new RetriggerLabel ("new label",
                                                TRANS("Search...")));
    searchLabel->setTooltip (TRANS("Search for wildcard pattern using RegEx"));
    searchLabel->setFont (Font (15.00f, Font::plain));
    searchLabel->setJustificationType (Justification::centredLeft);
    searchLabel->setEditable (true, true, false);
    searchLabel->setColour (Label::backgroundColourId, Colour (0x13ffffff));
    searchLabel->setColour (TextEditor::textColourId, Colours::black);
    searchLabel->setColour (TextEditor::backgroundColourId, Colour (0x00000000));
    searchLabel->setColour (TextEditor::highlightColourId, Colour (SIGNAL_COLOUR).withAlpha(0.5f));
    searchLabel->setColour (TextEditor::ColourIds::focusedOutlineColourId, Colour(SIGNAL_COLOUR));
    searchLabel->addListener (this);


    //[UserPreSize]

	searchLabel->setFont(GLOBAL_FONT());

	searchLabel->setNotifyIfNothingChanges(true);

	numSelected = 0;

	s->getSampleMap()->addListener(this);

	addKeyListener(s->getSampleEditHandler());

	
	addAndMakeVisible(helpButton = new MarkdownHelpButton());

	helpButton->setPopupWidth(600);
	helpButton->setHelpText(SamplerTableHelp::RegexHelp());

    //[/UserPreSize]

    setSize (800, 350);


    //[Constructor] You can add your own custom stuff here..
    //[/Constructor]
}

SamplerTable::~SamplerTable()
{
    //[Destructor_pre]. You can add your own custom destruction code here..

	if(sampler != nullptr)
		sampler->getSampleMap()->removeListener(this);

	if (sampler != nullptr)
		removeKeyListener(sampler->getSampleEditHandler());

    //[/Destructor_pre]

    table = nullptr;
    searchLabel = nullptr;


    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void SamplerTable::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..

    int x = 0;
    int y = 2;
    int width = getWidth();
    int height = getHeight()-4;
    
	Rectangle<int> a(x, y, width, height);

	//ProcessorEditorLookAndFeel::drawShadowBox(g, a, Colour(0xFF333333));

    //[/UserPrePaint]

    g.setColour (Colour (0x13ffffff));
    g.fillRect (8, 8, getWidth() - 163, 24);

    g.setColour (Colour (0x0fffffff));
    g.drawRect (8, 8, getWidth() - 163, 24, 1);

    g.setColour (Colour (0xCCffffff));
	g.setFont(GLOBAL_BOLD_FONT().withHeight(22.0f));
    g.drawText (TRANS("SAMPLE TABLE"),
                getWidth() - 8 - 184, 4, 184, 30,
                Justification::centredRight, true);

    //[UserPaint] Add your own custom painting code here..

	
	
	if (numSelected != 0)
	{
		g.setColour(Colours::white);
		g.setFont(GLOBAL_BOLD_FONT());
		g.drawText(String(numSelected) + " selected samples", helpButton->getRight() + 20, searchLabel->getY(), 150, searchLabel->getHeight(), Justification::centredLeft);
	}
	

	g.setColour(Colours::white.withAlpha(0.8f));

	static const unsigned char searchIcon[] = { 110, 109, 0, 0, 144, 68, 0, 0, 48, 68, 98, 7, 31, 145, 68, 198, 170, 109, 68, 78, 223, 103, 68, 148, 132, 146, 68, 85, 107, 42, 68, 146, 2, 144, 68, 98, 54, 145, 219, 67, 43, 90, 143, 68, 66, 59, 103, 67, 117, 24, 100, 68, 78, 46, 128, 67, 210, 164, 39, 68, 98, 93, 50, 134, 67, 113, 58, 216, 67, 120, 192, 249, 67, 83, 151,
		103, 67, 206, 99, 56, 68, 244, 59, 128, 67, 98, 72, 209, 112, 68, 66, 60, 134, 67, 254, 238, 144, 68, 83, 128, 238, 67, 0, 0, 144, 68, 0, 0, 48, 68, 99, 109, 0, 0, 208, 68, 0, 0, 0, 195, 98, 14, 229, 208, 68, 70, 27, 117, 195, 211, 63, 187, 68, 146, 218, 151, 195, 167, 38, 179, 68, 23, 8, 77, 195, 98, 36, 92, 165, 68, 187, 58,
		191, 194, 127, 164, 151, 68, 251, 78, 102, 65, 0, 224, 137, 68, 0, 0, 248, 66, 98, 186, 89, 77, 68, 68, 20, 162, 194, 42, 153, 195, 67, 58, 106, 186, 193, 135, 70, 41, 67, 157, 224, 115, 67, 98, 13, 96, 218, 193, 104, 81, 235, 67, 243, 198, 99, 194, 8, 94, 78, 68, 70, 137, 213, 66, 112, 211, 134, 68, 98, 109, 211, 138, 67,
		218, 42, 170, 68, 245, 147, 37, 68, 128, 215, 185, 68, 117, 185, 113, 68, 28, 189, 169, 68, 98, 116, 250, 155, 68, 237, 26, 156, 68, 181, 145, 179, 68, 76, 44, 108, 68, 16, 184, 175, 68, 102, 10, 33, 68, 98, 249, 118, 174, 68, 137, 199, 2, 68, 156, 78, 169, 68, 210, 27, 202, 67, 0, 128, 160, 68, 0, 128, 152, 67, 98, 163,
		95, 175, 68, 72, 52, 56, 67, 78, 185, 190, 68, 124, 190, 133, 66, 147, 74, 205, 68, 52, 157, 96, 194, 98, 192, 27, 207, 68, 217, 22, 154, 194, 59, 9, 208, 68, 237, 54, 205, 194, 0, 0, 208, 68, 0, 0, 0, 195, 99, 101, 0, 0 };


	Path path;
	path.loadPathFromData(searchIcon, sizeof(searchIcon));

	path.applyTransform(AffineTransform::rotation(float_Pi));

	path.scaleToFit(12.0f, 12.0f, 16.0f, 16.0f, true);

	g.fillPath(path);

    //[/UserPaint]
}

void SamplerTable::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    table->setBounds (8, 40, getWidth() - 16, getHeight() - 45);
    searchLabel->setBounds (32, 8, 192, 24);
    //[UserResized] Add your own custom resize handling here..

	helpButton->setBounds(searchLabel->getRight() + 4 , 10, 20, 20);

    //[/UserResized]
}

void SamplerTable::labelTextChanged (Label* labelThatHasChanged)
{
    //[UserlabelTextChanged_Pre]
    //[/UserlabelTextChanged_Pre]

    if (labelThatHasChanged == searchLabel)
    {
        //[UserLabelCode_searchLabel] -- add your label text handling code here..
		String wildcard = searchLabel->getText(false);

		ModulatorSamplerSound::selectSoundsBasedOnRegex(wildcard, sampler, handler->getSelectionReference());

        //[/UserLabelCode_searchLabel]
    }

    //[UserlabelTextChanged_Post]
    //[/UserlabelTextChanged_Post]
}



//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...
//[/MiscUserCode]


//==============================================================================
#if 0
/*  -- Introjucer information section --

    This is where the Introjucer stores the metadata that describe this GUI layout, so
    make changes in here at your peril!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="SamplerTable" componentName=""
                 parentClasses="public Component, public SamplerSubEditor" constructorParams="ModulatorSampler *s, SamplerBody *b"
                 variableInitialisers="sampler(s),&#10;body(b)" snapPixels="8"
                 snapActive="1" snapShown="1" overlayOpacity="0.330" fixedSize="1"
                 initialWidth="800" initialHeight="350">
  <BACKGROUND backgroundColour="ffffff">
    <RECT pos="8 8 163M 24" fill="solid: 13ffffff" hasStroke="1" stroke="1, mitered, butt"
          strokeColour="solid: fffffff"/>
    <TEXT pos="8Rr 4 184 30" fill="solid: 52ffffff" hasStroke="0" text="SAMPLE TABLE"
          fontname="Arial" fontsize="20" bold="1" italic="0" justification="34"/>
  </BACKGROUND>
  <GENERICCOMPONENT name="new component" id="ca3a16aa55511202" memberName="table"
                    virtualName="SamplerSoundTable" explicitFocusOrder="0" pos="8 40 16M 45M"
                    class="SamplerSoundTable" params="s, b"/>
  <LABEL name="new label" id="d6ed5b0a6794f45d" memberName="searchLabel"
         virtualName="RetriggerLabel" explicitFocusOrder="0" pos="32 8 192 24" tooltip="Search for wildcard pattern using RegEx"
         bkgCol="13ffffff" edTextCol="ff000000" edBkgCol="0" labelText="Search..."
         editableSingleClick="1" editableDoubleClick="1" focusDiscardsChanges="0"
         fontname="Default font" fontsize="15" bold="0" italic="0" justification="33"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
} // namespace hise
//[/EndFile]
