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
#ifndef SCRIPTINGCODEEDITOR_H_INCLUDED
#define SCRIPTINGCODEEDITOR_H_INCLUDED

namespace hise { using namespace juce;


class DebugConsoleTextEditor : public TextEditor,
	public TextEditor::Listener,
	public GlobalScriptCompileListener
{
public:

	DebugConsoleTextEditor(const String& name, Processor* p);;

	~DebugConsoleTextEditor();

	void scriptWasCompiled(JavascriptProcessor *jp);

	bool keyPressed(const KeyPress& k) override;

	void mouseDown(const MouseEvent& e);
	void mouseDoubleClick(const MouseEvent& e) override;

	void addToHistory(const String& s);

	void textEditorReturnKeyPressed(TextEditor& /*t*/);

private:

	LookAndFeel_V2 laf2;

	WeakReference<Processor> processor;

	StringArray history;
	int currentHistoryIndex = 0;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DebugConsoleTextEditor)
};


class CodeEditorWrapper : public Component,
	public Timer
{
public:

	// ================================================================================================================

	CodeEditorWrapper(CodeDocument &document, CodeTokeniser *codeTokeniser, JavascriptProcessor *p, const Identifier& snippetId);
	virtual ~CodeEditorWrapper();

	ScopedPointer<JavascriptCodeEditor> editor;

	void resized() override;;
	void timerCallback();

	void mouseDown(const MouseEvent &m) override;;
	void mouseUp(const MouseEvent &) override;;

	int currentHeight;

	// ================================================================================================================

private:

	ScopedPointer<ResizableEdgeComponent> dragger;

	ComponentBoundsConstrainer restrainer;

	LookAndFeel_V2 laf2;

	// ================================================================================================================

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CodeEditorWrapper);
};




} // namespace hise
#endif  // SCRIPTINGCODEEDITOR_H_INCLUDED
