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

#include <regex>

namespace hise { using namespace juce;


DebugConsoleTextEditor::DebugConsoleTextEditor(const String& name, Processor* p) :
	TextEditor(name),
	processor(p)
{
	setMultiLine(false);
	setReturnKeyStartsNewLine(false);
	setScrollbarsShown(false);
	setPopupMenuEnabled(false);
	setColour(TextEditor::textColourId, Colours::white);
	setColour(CaretComponent::ColourIds::caretColourId, Colours::white);
	setColour(TextEditor::backgroundColourId, Colour(0x00ffffff));
	setColour(TextEditor::highlightColourId, Colour(0x40ffffff));
	setColour(TextEditor::shadowColourId, Colour(0x00000000));
	setColour(TextEditor::ColourIds::focusedOutlineColourId, Colours::white.withAlpha(0.1f));
	setText(String());

	setFont(GLOBAL_MONOSPACE_FONT());
	setLookAndFeel(&laf2);
	setColour(Label::ColourIds::textColourId, Colours::white);

	addListener(this);

	p->getMainController()->addScriptListener(this);

	scriptWasCompiled(dynamic_cast<JavascriptProcessor*>(p));
}

DebugConsoleTextEditor::~DebugConsoleTextEditor()
{
	setLookAndFeel(nullptr);

	if (processor != nullptr)
	{
		processor->getMainController()->removeScriptListener(this);
	}
}

void DebugConsoleTextEditor::scriptWasCompiled(JavascriptProcessor *jp)
{
	if (dynamic_cast<Processor*>(jp) == processor)
	{
		lastError = jp->getLastErrorMessage();

		if (lastError.wasOk())
			setText("Compiled OK", dontSendNotification);
		else
        {
            setText(lastError.errorMessage, dontSendNotification);
        }

		setColour(TextEditor::backgroundColourId, lastError.wasOk() ? Colours::green.withBrightness(0.1f) : Colours::red.withBrightness((0.1f)));
	}
}

bool DebugConsoleTextEditor::keyPressed(const KeyPress& k)
{
	if (k == KeyPress::upKey)
	{
		currentHistoryIndex = jmax<int>(0, currentHistoryIndex - 1);

		setText(history[currentHistoryIndex], dontSendNotification);

		return true;
	}
	else if (k == KeyPress::downKey)
	{
		currentHistoryIndex = jmin<int>(history.size() - 1, currentHistoryIndex + 1);
		setText(history[currentHistoryIndex], dontSendNotification);
	}

	return TextEditor::keyPressed(k);
}

void DebugConsoleTextEditor::mouseDown(const MouseEvent& e)
{
	if(!getText().containsChar('>'))
        setText("> ", dontSendNotification);

	TextEditor::mouseDown(e);
}

void DebugConsoleTextEditor::mouseDoubleClick(const MouseEvent& /*e*/)
{
	gotoText();
}

void DebugConsoleTextEditor::gotoText()
{
	if(!lastError.wasOk())
		PopupIncludeEditor::gotoUriLocation(dynamic_cast<JavascriptProcessor*>(processor.get()), this, lastError);
}

void DebugConsoleTextEditor::addToHistory(const String& s)
{
	if (!history.contains(s))
	{
		history.add(s);
		currentHistoryIndex = history.size() - 1;
	}
	else
	{
		history.move(history.indexOf(s), history.size() - 1);
	}
}

void DebugConsoleTextEditor::textEditorReturnKeyPressed(TextEditor& /*t*/)
{
	String codeToEvaluate = getText();

	addToHistory(codeToEvaluate);

	if (codeToEvaluate.startsWith("> "))
	{
		codeToEvaluate = codeToEvaluate.substring(2);
	}

	auto jsp = dynamic_cast<JavascriptProcessor*>(processor.get());

	if(codeToEvaluate.startsWith("goto "))
	{
		auto d = codeToEvaluate.substring(5, 1000000);
		auto tokens = StringArray::fromTokens(d, "@", "");

		UriLocation loc;
		loc.charIndex = tokens[1].getIntValue();
		loc.uri = tokens[0];

		PopupIncludeEditor::gotoUriLocation(jsp, this, loc);

		return;
	}
    
#if USE_BACKEND
    constexpr auto taskType = JavascriptThreadPool::Task::ReplEvaluation;
#else
    constexpr auto taskType = JavascriptThreadPool::Task::Compilation;
#endif

    processor->getMainController()->getJavascriptThreadPool().addJob(taskType,
                                                                     jsp, [codeToEvaluate](JavascriptProcessor* p)
    {
        Result r = Result::ok();
        HiseJavascriptEngine* engine = p->getScriptEngine();
        
        var returnValue = engine->evaluate(codeToEvaluate, &r);

        auto pr = dynamic_cast<Processor*>(p);
        
		auto error = UriError::fromResult(r, p);

        if (r.wasOk())
        {
            pr->getMainController()->writeToConsole("> " + returnValue.toString(), 0, nullptr);
        }
        else
        {
            debugToConsole(pr, error.toConsoleMessage());
        }
        
        return error;
    });
}






} // namespace hise
