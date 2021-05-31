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

namespace hise { using namespace juce;

struct GLSLKeywordProvider : public mcl::TokenCollection::Provider
{
	struct GLSLKeyword : public mcl::TokenCollection::Token
	{
		GLSLKeyword(const String& name) :
			Token(name)
		{
			c = Colours::lightcoral;
			priority = 40;
		}
	};

	struct GLSLToken : public mcl::TokenCollection::Token
	{
		GLSLToken(const String& codeToInsert, const String& typeString) :
			Token(typeString + " " + codeToInsert),
			code(codeToInsert)
		{
			c = Colours::magenta;
			priority = 50;
		}

		bool matches(const String& input, const String& previousToken, int lineNumber) const
		{
			return previousToken.isEmpty() && matchesInput(input, code);
		}

		String getCodeToInsert(const String& input) const override { return code; }

		String code;
	};

	void addKeyword(mcl::TokenCollection::List& tokens, const String& word, const String& description)
	{
		auto c = new GLSLKeyword(word);
		c->markdownDescription = description;
		tokens.add(c);
	}

	void addGlobalVariable(mcl::TokenCollection::List& tokens, const String& word, const String& type, const String& description)
	{
		auto c = new GLSLToken(word, type);

		c->markdownDescription = description;

		tokens.add(c);
	}

	void addTokens(mcl::TokenCollection::List& tokens) override
	{
		addGlobalVariable(tokens, "iResolution", "vec2", "The actual pixel size of the canvas");
		addGlobalVariable(tokens, "pixelPos", "vec2", "The unscaled pixel position on the monitor");
		addGlobalVariable(tokens, "fragCoord", "vec2", "The scaled pixel coordinate relative to the bottom left");
		addGlobalVariable(tokens, "iTime", "float", "The time in seconds since compilation");
		addGlobalVariable(tokens, "fragColor", "vec4", "The output colour for the given pixel");
		addGlobalVariable(tokens, "pixelAlpha", "float", "The alpha value that needs to be multiplied with the output colour");

		addKeyword(tokens, "vec2", "A two dimensional vector");
		addKeyword(tokens, "vec3", "A three dimensional vector");
		addKeyword(tokens, "vec4", "A four dimensional vector");
		addKeyword(tokens, "float", "A single precision float number");

		addKeyword(tokens, "uniform", "A keyword for specifying uniform data");
		addKeyword(tokens, "main", "The main entry function");
	}
};

PopupIncludeEditor::PopupIncludeEditor(JavascriptProcessor *s, const File &fileToEdit) :
	sp(s),
	callback(Identifier()),
	tokeniser(new JavascriptTokeniser())
{
	Processor *p = dynamic_cast<Processor*>(sp);

	externalFile = p->getMainController()->getExternalScriptFile(fileToEdit);

	

	if (externalFile->getFile().hasFileExtension("glsl"))
	{
		

		doc = new mcl::TextDocument(externalFile->getFileDocument());
		
		auto med = new mcl::TextEditor(*doc);

		med->tokenCollection.addTokenProvider(new GLSLKeywordProvider());

		editor = med;

		med->setLineRangeFunction(snex::debug::Helpers::createLineRanges);

		addAndMakeVisible(editor);

		externalFile->addRuntimeErrorListener(this);

	}
	else
	{
		const Identifier snippetId = Identifier("File_" + fileToEdit.getFileNameWithoutExtension());
		tokeniser = new JavascriptTokeniser();
		addAndMakeVisible(editor = new JavascriptCodeEditor(externalFile->getFileDocument(), tokeniser, s, snippetId));
	}

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
	if (externalFile != nullptr)
		externalFile->removeRuntimeErrorListener(this);

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
	repaint();
	stopTimer();
}

bool PopupIncludeEditor::keyPressed(const KeyPress& key)
{
	if (key.isKeyCode(KeyPress::F5Key) && !key.getModifiers().isShiftDown())
	{
		compileInternal();
		return true;
	}

	return false;
}

void PopupIncludeEditor::runTimeErrorsOccured(const Array<ExternalScriptFile::RuntimeError>& errors)
{
	if (auto asmcl = dynamic_cast<mcl::TextEditor*>(editor.get()))
	{
		for (const auto& e : errors)
		{
			if (e.errorLevel == ExternalScriptFile::RuntimeError::ErrorLevel::Warning)
				asmcl->addWarning(e.toString(), true);
			if (e.errorLevel == ExternalScriptFile::RuntimeError::ErrorLevel::Error)
			{
				asmcl->addWarning(e.toString(), false);
				lastCompileOk = false;
				startTimer(200);

				if(resultLabel != nullptr)
					resultLabel->setText("GLSL Compile Error", dontSendNotification);
			}
		}

		asmcl->repaint();
	}

	
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

	if (auto ed = getEditor())
	{
		pos = lineNumber != -1 ? CodeDocument::Position(ed->getDocument(), lineNumber, character) :
			CodeDocument::Position(ed->getDocument(), character);

		ed->scrollToLine(jmax<int>(0, pos.getLineNumber() - 1));
		ed->moveCaretTo(pos, false);
		ed->moveCaretToStartOfLine(false);
		ed->moveCaretToEndOfLine(true);
	}
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

	auto rf = [this](const JavascriptProcessor::SnippetResult& r)
	{
		lastCompileOk = r.r.wasOk();
		resultLabel->setColour(TextEditor::ColourIds::backgroundColourId, Colours::white);
		resultLabel->setColour(TextEditor::ColourIds::textColourId, Colours::white);
		startTimer(200);
	};

	sp->compileScript(rf);

	if (auto asmcl = dynamic_cast<mcl::TextEditor*>(editor.get()))
	{
		asmcl->clearWarningsAndErrors();
	}
}

} // namespace hise