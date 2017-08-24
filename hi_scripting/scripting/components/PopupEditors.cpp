/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   HISE is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licenses for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licensing:
*
*   http://www.hise.audio/
*
*   HISE is based on the JUCE library,
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/



PopupIncludeEditor::PopupIncludeEditor(JavascriptProcessor *s, const File &fileToEdit) :
	sp(s),
	callback(Identifier()),
	tokeniser(new JavascriptTokeniser())
{
	Processor *p = dynamic_cast<Processor*>(sp);

	externalFile = p->getMainController()->getExternalScriptFile(fileToEdit);

	const Identifier snippetId = Identifier("File_" + fileToEdit.getFileNameWithoutExtension());

	tokeniser = new JavascriptTokeniser();
	addAndMakeVisible(editor = new JavascriptCodeEditor(externalFile->getFileDocument(), tokeniser, s, snippetId));

	addButtonAndCompileLabel();
	
}

PopupIncludeEditor::PopupIncludeEditor(JavascriptProcessor* s, const Identifier &callback_) :
	sp(s),
	callback(callback_),
	tokeniser(new JavascriptTokeniser())
{
	addAndMakeVisible(editor = new JavascriptCodeEditor(*sp->getSnippet(callback_), tokeniser, s, callback));

	addButtonAndCompileLabel();
}

void PopupIncludeEditor::addButtonAndCompileLabel()
{
	addAndMakeVisible(resultLabel = new DebugConsoleTextEditor("messageBox", dynamic_cast<Processor*>(sp)));

	addAndMakeVisible(compileButton = new TextButton("new button"));
	compileButton->setButtonText(TRANS("Compile"));
	compileButton->setConnectedEdges(Button::ConnectedOnLeft | Button::ConnectedOnRight);
	compileButton->addListener(this);
	compileButton->setColour(TextButton::buttonColourId, Colour(0xa2616161));

	setSize(800, 800);
}



PopupIncludeEditor::~PopupIncludeEditor()
{
	

	editor = nullptr;
	resultLabel = nullptr;

	tokeniser = nullptr;
	compileButton = nullptr;

	Processor *p = dynamic_cast<Processor*>(sp);

	sp = nullptr;
	p = nullptr;

	externalFile = nullptr;
}

void PopupIncludeEditor::timerCallback()
{
	resultLabel->setColour(TextEditor::backgroundColourId, lastCompileOk ? Colours::green.withBrightness(0.1f) : Colours::red.withBrightness((0.1f)));
	stopTimer();
}

bool PopupIncludeEditor::keyPressed(const KeyPress& key)
{
	if (key.isKeyCode(KeyPress::F5Key))
	{
		compileInternal();
		return true;
	}

	return false;
}

void PopupIncludeEditor::resized()
{
	bool isInPanel = findParentComponentOfClass<FloatingTile>() != nullptr;

	if(isInPanel)
		editor->setBounds(0, 0, getWidth(), getHeight() - 18);
	else
		editor->setBounds(0, 5, getWidth(), getHeight() - 23);

	resultLabel->setBounds(0, getHeight() - 18, getWidth()- 95, 18);
	compileButton->setBounds(getWidth() - 95, getHeight() - 18, 95, 18);
}

void PopupIncludeEditor::gotoChar(int character, int lineNumber/*=-1*/)
{
	CodeDocument::Position pos;

	pos = lineNumber != -1 ? CodeDocument::Position(editor->getDocument(), lineNumber, character) :
		CodeDocument::Position(editor->getDocument(), character);

	editor->scrollToLine(jmax<int>(0, pos.getLineNumber() - 1));
	editor->moveCaretTo(pos, false);
	editor->moveCaretToStartOfLine(false);
	editor->moveCaretToEndOfLine(true);
}

void PopupIncludeEditor::buttonClicked(Button* /*b*/)
{
	compileInternal();
}

File PopupIncludeEditor::getFile() const
{
	return externalFile != nullptr ? externalFile->getFile() : File();
}

void PopupIncludeEditor::compileInternal()
{
	if (externalFile != nullptr)
	{
		externalFile->getFile().replaceWithText(externalFile->getFileDocument().getAllContent());
		externalFile->getFileDocument().setSavePoint();
	}

	sp->compileScript();
	
	lastCompileOk = sp->wasLastCompileOK();

	resultLabel->setColour(TextEditor::ColourIds::backgroundColourId, Colours::white);
	resultLabel->setColour(TextEditor::ColourIds::textColourId, Colours::white);

	startTimer(200);
}
