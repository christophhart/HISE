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
*   which must be separately licensed for cloused source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

#if USE_BACKEND


Console::ConsoleEditorComponent::ConsoleEditorComponent(CodeDocument &doc, CodeTokeniser* tok) :
CodeEditorComponent(doc, tok)
{
	setReadOnly(true);
	setColour(CodeEditorComponent::ColourIds::backgroundColourId, Colour(0xFF282828));
	getDocument().getUndoManager().setMaxNumberOfStoredUnits(0, 0);

#if JUCE_MAC
    setFont(GLOBAL_MONOSPACE_FONT().withHeight(12.0f)); // other font sizes disappear on OSX Sierra, yeah!
#else
    setFont(GLOBAL_MONOSPACE_FONT());
#endif
    
	setColour(CodeEditorComponent::ColourIds::defaultTextColourId, Colours::white.withBrightness(0.7f));
	setLineNumbersShown(false);
}


Console::Console(MainController* mc_):
	mc(mc_)
{
	setName("Console");

	auto consoleDoc = mc->getConsoleHandler().getConsoleData();

	consoleDoc->addListener(this);

	tokeniser = new ConsoleTokeniser();

	addAndMakeVisible(newTextConsole = new ConsoleEditorComponent(*mc->getConsoleHandler().getConsoleData(), tokeniser));
	newTextConsole->addMouseListener(this, true);
}

Console::~Console()
{
	mc->getConsoleHandler().getConsoleData()->removeListener(this);

	newTextConsole = nullptr;
	tokeniser = nullptr;

	masterReference.clear();
};


void Console::resized()
{
	newTextConsole->setBounds(getLocalBounds());
}



void Console::clear()
{
	mc->getConsoleHandler().clearConsole();

	
}

void Console::mouseDown(const MouseEvent &e)
{

    if(e.mods.isRightButtonDown())
    {
        PopupLookAndFeel plaf;
        PopupMenu m;
        
        m.setLookAndFeel(&plaf);
        
        m.addItem(1, "Clear Console");
        m.addItem(2, "Scroll down");
		
		const String id = newTextConsole->getDocument().getLine(newTextConsole->getCaretPos().getLineNumber()).upToFirstOccurrenceOf(":", false, false);

		JavascriptProcessor *jsp = dynamic_cast<JavascriptProcessor*>(ProcessorHelpers::getFirstProcessorWithName(getMainPanel()->getMainSynthChain(), id));


		const int SNIPPET_OFFSET = 1000;
		const int FILE_OFFSET = 2000;

		if (jsp != nullptr)
		{
			const String selectedText = newTextConsole->getTextInRange(newTextConsole->getHighlightedRegion());

			int snippet = -1;

			for (int i = 0; i < jsp->getNumSnippets(); i++)
			{
				if (jsp->getSnippet(i)->getCallbackName().toString() == selectedText)
				{
					snippet = i;
					break;
				}
			}

			if (snippet != -1)
			{
				m.addItem(SNIPPET_OFFSET + snippet, "Go to callback " + selectedText);
			}

			int fileIndex = -1;

			for (int i = 0; i < jsp->getNumWatchedFiles(); i++)
			{
				if (jsp->getWatchedFile(i).getFileName() == selectedText)
				{
					fileIndex = i;
					break;
				}
			}

			if (fileIndex != -1)
			{
				m.addItem(FILE_OFFSET + fileIndex, "Go to file " + selectedText);
			}
		}

        const int result = m.show();
        
		if (result == 1)
		{
			newTextConsole->getDocument().replaceAllContent("");
			newTextConsole->scrollToLine(0);
			
		}
        else if (result == 2)
        {
			newTextConsole->moveCaretToEnd(false);
        }
		else if (result >= FILE_OFFSET)
		{
			jsp->showPopupForFile(result - FILE_OFFSET);
		}
		else if (result >= SNIPPET_OFFSET)
		{
			Processor *js = dynamic_cast<Processor*>(jsp);

			const int editorStateOffset = dynamic_cast<ProcessorWithScriptingContent*>(js)->getCallbackEditorStateOffset() + 1;

			const int editorStateIndex = (result - SNIPPET_OFFSET);

			for (int i = 0; i < jsp->getNumSnippets(); i++)
			{
				js->setEditorState(editorStateOffset + i, editorStateIndex == i, dontSendNotification);
			}

			findParentComponentOfClass<BackendRootWindow>()->getMainPanel()->setRootProcessorWithUndo(js);
		}
    }
    else if (e.mods.isAltDown())
    {
#if USE_BACKEND
        
		CodeDocument::Position pos = newTextConsole->getCaretPos();

		String name = newTextConsole->getDocument().getLine(pos.getLineNumber()).upToFirstOccurrenceOf(":", false, false);

        if(name.isNotEmpty())
        {
			auto editor = findParentComponentOfClass<BackendRootWindow>()->getMainPanel();

            Processor *p = ProcessorHelpers::getFirstProcessorWithName(editor->getMainSynthChain(), name);
            
            if(p != nullptr)
            {
                editor->setRootProcessorWithUndo(p);
            }
        }
#endif
        
    }
}


void Console::mouseMove(const MouseEvent &e)
{
	if (e.mods.isAltDown())
	{
		setMouseCursor(MouseCursor::PointingHandCursor);
	}
}

void Console::mouseDoubleClick(const MouseEvent& /*e*/)
{
	CodeDocument::Position selectionStart = newTextConsole->getSelectionStart();

	const String line = newTextConsole->getDocument().getLine(selectionStart.getLineNumber());

	const String reg = "(.+):! (\\w*): (.* - )?Line (\\d+), column (\\d+): (\\w+)";

	StringArray matches = RegexFunctions::getFirstMatch(reg, line);
	
	if (matches.size() == 7)
	{
		const String id = matches[1];
		const Identifier callback = matches[2].isNotEmpty() ? Identifier(matches[2]) : Identifier();
		const String fileName = matches[3].upToFirstOccurrenceOf(" - ", false, false);
		const String lineNumber = matches[4];
		const String charNumber = matches[5];

		JavascriptProcessor *jsp = dynamic_cast<JavascriptProcessor*>(ProcessorHelpers::getFirstProcessorWithName(getMainPanel()->getMainSynthChain(), id));

		if (fileName.isNotEmpty())
		{
			for (int i = 0; i < jsp->getNumWatchedFiles(); i++)
			{
				if (jsp->getWatchedFile(i).getFileName() == fileName)
				{
					StringArray lines;
					jsp->getWatchedFile(i).readLines(lines);

					int charIndex = jmax<int>(0, charNumber.getIntValue());
					int lineIndex = jmax<int>(0, lineNumber.getIntValue());

					jsp->showPopupForFile(i, charIndex, lineIndex);
				}
			}
		}
		else
		{
			jassert(!callback.isNull());

			ProcessorEditor* pEditor = getMainPanel()->getRootContainer()->getFirstEditorOf(dynamic_cast<Processor*>(jsp));

			if (pEditor != nullptr)
			{
				ScriptingEditor* scriptEditor = dynamic_cast<ScriptingEditor*>(pEditor->getBody());

				for (int i = 0; i < jsp->getNumSnippets(); i++)
				{
					if (jsp->getSnippet(i)->getCallbackName() == callback)
					{
						scriptEditor->showCallback(i, lineNumber.getIntValue() - 1);
						break;
					}
				}
			}
		}
	}
};;

Console::ConsoleTokeniser::ConsoleTokeniser()
{
	s.set("id", Colours::white);
	s.set("default", Colours::white.withBrightness(0.75f));
	s.set("error", JUCE_LIVE_CONSTANT_OFF(Colour(0xffff3939)));
}

int Console::ConsoleTokeniser::readNextToken(CodeDocument::Iterator& source)
{
	while (source.nextChar() != ':')
	{
		return 0;
	}


	if (source.peekNextChar() == '!')
	{
		source.skipToEndOfLine();

		return 2;
	}
	else
	{
		source.skipToEndOfLine();

		return 1;
	}
}


#endif

BackendProcessorEditor* ComponentWithAccessToMainPanel::getMainPanel()
{
#if USE_BACKEND
	return dynamic_cast<Component*>(this)->findParentComponentOfClass<BackendRootWindow>()->getMainPanel();
#else
	return nullptr;
#endif
}

const BackendProcessorEditor* ComponentWithAccessToMainPanel::getMainPanel() const
{
#if USE_BACKEND
	return dynamic_cast<const Component*>(this)->findParentComponentOfClass<BackendRootWindow>()->getMainPanel();
#else
	return nullptr;
#endif
}
