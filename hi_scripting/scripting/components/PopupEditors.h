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

namespace hise { using namespace juce;


class JavascriptCodeEditor;
class DebugConsoleTextEditor;


class PopupIncludeEditor : public Component,
						   public Timer,
						   public ButtonListener,
						   public Dispatchable, 
						   public mcl::GutterComponent::BreakpointListener,
						   public JavascriptThreadPool::SleepListener
{
public:

	static bool matchesId(Component* c, const Identifier& id)
	{
		return CommonEditorFunctions::as(c)->findParentComponentOfClass<PopupIncludeEditor>()->callback == id;
	}

	static float getGlobalCodeFontSize(Component* c);

	// ================================================================================================================

	PopupIncludeEditor(JavascriptProcessor *s, const File &fileToEdit);
	PopupIncludeEditor(JavascriptProcessor* s, const Identifier &callback);
	
	~PopupIncludeEditor();

	void timerCallback();
	bool keyPressed(const KeyPress& key) override;

	static void runTimeErrorsOccured(PopupIncludeEditor& t, Array<ExternalScriptFile::RuntimeError>* errors);

	void resized() override;;

	void gotoChar(int character, int lineNumber = -1);

	void buttonClicked(Button* b) override;

	void breakpointsChanged(mcl::GutterComponent& g) override;

	void paintOverChildren(Graphics& g) override;

	static void initKeyPresses(Component* root);

	File getFile() const;
	
	JavascriptProcessor* getScriptProcessor() { return jp; }

	static File getPropertyFile()
	{
		return ProjectHandler::getAppDataDirectory().getChildFile("code_editor.json");
	}

	using EditorType = CommonEditorFunctions::EditorType;

	void sleepStateChanged(const Identifier& id, int lineNumber, bool on) override;

	void addEditor(CodeDocument& d, bool isJavascript);

	EditorType* getEditor() { return editor; }
	const EditorType* getEditor() const { return editor; }

	ScopedPointer<EditorType> editor;

	ScopedPointer<mcl::TextDocument> doc;

	Identifier callback;
	WeakReference<JavascriptProcessor> jp;

	ExternalScriptFile::Ptr externalFile;

	ScopedPointer<DebugConsoleTextEditor> resultLabel;

private:

	void addButtonAndCompileLabel();
	void refreshAfterCompilation(const JavascriptProcessor::SnippetResult& r);
	void compileInternal();

	friend class PopupIncludeEditorWindow;

	bool isCallbackEditor() { return !callback.isNull(); }
	
	int fontSize;

	ScopedPointer<JavascriptTokeniser> tokeniser;
	ScopedPointer<TextButton> compileButton;
	ScopedPointer<TextButton> resumeButton;

	bool isHalted = false;
	bool lastCompileOk;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PopupIncludeEditor);
	JUCE_DECLARE_WEAK_REFERENCEABLE(PopupIncludeEditor);

	// ================================================================================================================
};

} // namespace hise

#endif  // POPUPEDITORS_H_INCLUDED
