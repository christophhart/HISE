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

class PopupIncludeEditorWindow : public DocumentWindow,
	public ModalBaseWindow
{
public:

	// ================================================================================================================

	PopupIncludeEditorWindow(File f, JavascriptProcessor *s);

	PopupIncludeEditorWindow(const Identifier& callbackName, JavascriptProcessor* s);

	PopupIncludeEditorWindow(Component* scriptingEditor, JavascriptProcessor* s);

	File getFile() const { return file; };
	Identifier getCallback() const { return callback; }

	void paint(Graphics &g) override;
	bool keyPressed(const KeyPress& key);;
	void closeButtonPressed() override;;

	void gotoChar(int character, int lineNumber = -1);

private:

	Component::SafePointer<Component> parentEditorBody;
	ScopedPointer<PopupIncludeEditor> editor;
	const File file;
	const Identifier callback;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PopupIncludeEditorWindow);

	// ================================================================================================================
};



#endif  // POPUPEDITORS_H_INCLUDED
