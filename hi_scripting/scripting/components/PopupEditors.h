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

	using EditorType = CommonEditorFunctions::EditorType;

	// ================================================================================================================

	PopupIncludeEditor(JavascriptProcessor *s, const File &fileToEdit);
	PopupIncludeEditor(JavascriptProcessor* s, const Identifier &callback);
	
	void addEditor(CodeDocument& d, bool isJavascript);

	~PopupIncludeEditor();

	void timerCallback();
	bool keyPressed(const KeyPress& key) override;

	static void runTimeErrorsOccured(PopupIncludeEditor& t, Array<ExternalScriptFile::RuntimeError>* errors);

	void sleepStateChanged(const Identifier& id, int lineNumber, bool on) override;

	void resized() override;;

	void gotoChar(int character, int lineNumber = -1);

	void buttonClicked(Button* b) override;

	void breakpointsChanged(mcl::GutterComponent& g) override;

	EditorType* getEditor()
	{ 
		return dynamic_cast<EditorType*>(editor.get());
	}

	void paintOverChildren(Graphics& g) override;

	const EditorType* getEditor() const
	{
		return dynamic_cast<const EditorType*>(editor.get());
	}

	File getFile() const;
	
	JavascriptProcessor* getScriptProcessor() { return sp; }

private:

	void addButtonAndCompileLabel();

	void refreshAfterCompilation(const JavascriptProcessor::SnippetResult& r);

	void compileInternal();

	friend class PopupIncludeEditorWindow;

	bool isCallbackEditor() { return !callback.isNull(); }
	
	int fontSize;

	ExternalScriptFile::Ptr externalFile;

	ScopedPointer<mcl::TextDocument> doc;
	ScopedPointer<JavascriptTokeniser> tokeniser;
	ScopedPointer<Component> editor;
	
	ScopedPointer<TextButton> compileButton;
	ScopedPointer<TextButton> resumeButton;

	bool isHalted = false;

	ScopedPointer<DebugConsoleTextEditor> resultLabel;

	WeakReference<JavascriptProcessor> sp;
	
	const Identifier callback;

	bool lastCompileOk;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PopupIncludeEditor);
	JUCE_DECLARE_WEAK_REFERENCEABLE(PopupIncludeEditor);

	// ================================================================================================================
};

} // namespace hise

#endif  // POPUPEDITORS_H_INCLUDED
