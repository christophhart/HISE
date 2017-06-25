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

#ifndef POPUPEDITORS_H_INCLUDED
#define POPUPEDITORS_H_INCLUDED



class JavascriptCodeEditor;


class PopupIncludeEditor : public Component,
						   public Timer
{
public:

	// ================================================================================================================

	PopupIncludeEditor(JavascriptProcessor *s, const File &fileToEdit);
	PopupIncludeEditor(JavascriptProcessor* s, const Identifier &callback);
	PopupIncludeEditor(JavascriptProcessor* s);
	~PopupIncludeEditor();

	void timerCallback();
	bool keyPressed(const KeyPress& key) override;
	void resized() override;;

	void gotoChar(int character, int lineNumber = -1);

	JavascriptCodeEditor* getEditor() { return editor.get(); }

	
private:

	friend class PopupIncludeEditorWindow;

	bool isCallbackEditor() { return !callback.isNull(); }
	bool isWholeScriptEditor() { return callback.isNull() && !file.existsAsFile(); }

	int fontSize;

	ScopedPointer<JavascriptTokeniser> tokeniser;
	ScopedPointer < JavascriptCodeEditor > editor;
	OptionalScopedPointer<CodeDocument> doc;

	ScopedPointer<Label> resultLabel;

	JavascriptProcessor *sp;
	File file;
	const Identifier callback;

	bool lastCompileOk;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PopupIncludeEditor);

	// ================================================================================================================
};

#endif  // POPUPEDITORS_H_INCLUDED
