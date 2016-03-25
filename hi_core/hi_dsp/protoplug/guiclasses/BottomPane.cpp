#include "BottomPane.h"
#include "ProtoWindow.h"


//==============================================================================
BottomPane::BottomPane (ProtoWindow *_protoWin):
	compileButton("Compile", "It's not pink, it's like, ah... lightish red")
{
	protoWin = _protoWin;
	
	// Log area
    addAndMakeVisible (&log);
	log.setMultiLine(true);
	log.setReadOnly(true);
	log.setCaretVisible(false);
	log.setFont(Font(Font::getDefaultMonospacedFontName(), 12, 0));
	log.setColour(TextEditor::backgroundColourId, Colour(0xffe0e0e0));

	// Compile button
	addChildComponent(&compileButton);
	compileButton.setColour(TextButton::buttonColourId, Colour(0xffff8d8d));
	tooltip.setLookAndFeel(&protoWin->newFeel);
	compileButton.addListener(this);
	
	// Input area
	addChildComponent(&input);
	input.setFont(Font(Font::getDefaultMonospacedFontName(), 12, 0));
	input.setColour(TextEditor::backgroundColourId, Colours::transparentWhite);
	input.setIndents(16, 4);
	input.addListener(this);

	// Prompt label
    addAndMakeVisible (&prompt);
	prompt.setTarget(&input);
	prompt.setFont(Font(Font::getDefaultMonospacedFontName(), 14, Font::bold));
	prompt.setColour(Label::textColourId, Colours::red);
	prompt.setText(">", dontSendNotification);
}

void BottomPane::resized()
{
	if (getHeight()>35) {
		input.setVisible(true);
		prompt.setVisible(true);
		input.setBounds(0, getHeight()-22, getWidth()-103, 22);
		prompt.setBounds(0, getHeight()-22, 20, 22);
		log.setBounds(0, 0, getWidth()-16, getHeight()-22);
	} else {
		input.setVisible(false);
		prompt.setVisible(false);
		log.setBounds(0, 0, getWidth()-16, getHeight());
	}
    compileButton.setBounds (getWidth()-100, getHeight()-22, 80, 22);
}

void BottomPane::buttonClicked (Button *b)
{
	if (b==&compileButton)
		protoWin->compile();
}

void BottomPane::setCompileVisible(bool hue)
{
	compileButton.setVisible(hue);
}

void BottomPane::scrollLog()
{
	log.moveCaretToEndOfLine(false);
	log.moveCaretToStartOfLine(false);
}

void BottomPane::updateLog()
{
	log.clear();
	log.setText(protoWin->processor->luli->log);
	log.moveCaretToEnd();
	log.moveCaretToEndOfLine(false);
	log.moveCaretToStartOfLine(false);
}

void BottomPane::textEditorReturnKeyPressed (TextEditor &t)
{
	protoWin->processor->luli->runStringInteractive(input.getText());
	input.clear();
}