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
*   Commercial licences for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licencing:
*
*   http://www.hartinstruments.net/hise/
*
*   HISE is based on the JUCE library,
*   which also must be licenced for commercial applications:
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
	setColour(CodeEditorComponent::ColourIds::backgroundColourId, Colour(0xFF393939));
	getDocument().getUndoManager().setMaxNumberOfStoredUnits(0, 0);

	setFont(GLOBAL_MONOSPACE_FONT());
	setColour(CodeEditorComponent::ColourIds::defaultTextColourId, Colours::white.withBrightness(0.9f));
	setLineNumbersShown(false);
}


Console::Console(BaseDebugArea *area) :
		AutoPopupDebugComponent(area),
		overflowProtection(false)
{
	setName("Console");

	doc = new CodeDocument();
	tokeniser = new ConsoleTokeniser();

	addAndMakeVisible(newTextConsole = new ConsoleEditorComponent(*doc, tokeniser));
	newTextConsole->addMouseListener(this, true);

}


Console::~Console()
{
	newTextConsole = nullptr;
	doc = nullptr;
	tokeniser = nullptr;

	masterReference.clear();
};


void Console::resized()
{
	newTextConsole->setBounds(getLocalBounds());
}



void Console::clear()
{
	clearFlag = true;

	triggerAsyncUpdate();
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

		BackendProcessorEditor *editor = findParentComponentOfClass<BackendProcessorEditor>();
		JavascriptProcessor *jsp = dynamic_cast<JavascriptProcessor*>(ProcessorHelpers::getFirstProcessorWithName(editor->getMainSynthChain(), id));


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

			editor->setRootProcessorWithUndo(js);

		}
    }
    else if (e.mods.isAltDown())
    {
		

#if USE_BACKEND
        
		CodeDocument::Position pos = newTextConsole->getCaretPos();

		String name = newTextConsole->getDocument().getLine(pos.getLineNumber()).upToFirstOccurrenceOf(":", false, false);

        if(name.isNotEmpty())
        {
            BackendProcessorEditor *editor = findParentComponentOfClass<BackendProcessorEditor>();
            
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

		BackendProcessorEditor *editor = findParentComponentOfClass<BackendProcessorEditor>();
		JavascriptProcessor *jsp = dynamic_cast<JavascriptProcessor*>(ProcessorHelpers::getFirstProcessorWithName(editor->getMainSynthChain(), id));

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

			ProcessorEditor* pEditor = editor->getRootContainer()->getFirstEditorOf(dynamic_cast<Processor*>(jsp));

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
}

void Console::logMessage(const String &t, WarningLevel warningLevel, const Processor *p, Colour c)
{
	if(overflowProtection) return;

	else
	{
		ScopedLock sl(getLock());

		if(unprintedMessages.size() > 10)
		{
			unprintedMessages.push_back(ConsoleMessage(Error, const_cast<Processor*>(p), "Console Overflow"));
			overflowProtection = true;
			return;
		}
		else
		{
			unprintedMessages.push_back(ConsoleMessage(warningLevel, const_cast<Processor*>(p), t));
		}
	}

	if (MessageManager::getInstance()->isThisTheMessageThread())
	{
		handleAsyncUpdate();
	}
	else
	{
		triggerAsyncUpdate();
	}
};


void Console::handleAsyncUpdate()
{
	if (clearFlag)
	{
		newTextConsole->getDocument().replaceAllContent("");
		clearFlag = false;
	}

	std::vector<ConsoleMessage> messagesForThisTime;
	messagesForThisTime.reserve(10);

	if(unprintedMessages.size() != 0)
	{
		ScopedLock sl(getLock());
		messagesForThisTime.swap(unprintedMessages);
	}
	else return;

	String message;

	for(size_t i = 0; i < messagesForThisTime.size(); i++)
	{
        const Processor* processor = std::get<(int)ConsoleMessageItems::Processor>(messagesForThisTime[i]).get();
        
        if(processor == nullptr)
        {
            jassertfalse;
            continue;
        }
        
		message << processor->getId() << ":";
		message << (std::get<(int)ConsoleMessageItems::WarningLevel>(messagesForThisTime[i]) == WarningLevel::Error ? "! " : " ");
		message << std::get<(int)ConsoleMessageItems::Message>(messagesForThisTime[i]) << "\n";
	}

	doc->insertText(doc->getNumCharacters(), message);

	int numLinesVisible = jmax<int>(0, doc->getNumLines() - (int)((float)newTextConsole->getHeight() / GLOBAL_MONOSPACE_FONT().getHeight()));

	newTextConsole->scrollToLine(numLinesVisible);
	overflowProtection = false;

	return;
};

Console::ConsoleTokeniser::ConsoleTokeniser()
{
	s.set("id", Colours::white);
	s.set("default", Colours::white.withBrightness(0.75f));
	s.set("error", Colours::red.withBrightness(0.9f));
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