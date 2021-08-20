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

#if HISE_USE_NEW_CODE_EDITOR
	using EditorType = mcl::FullEditor;
#else
	using EditorType = JavascriptCodeEditor;
#endif

	struct CommonEditorFunctions
	{
		static bool matchesId(Component* c, const Identifier& id)
		{
			return as(c)->findParentComponentOfClass<PopupIncludeEditor>()->callback == id;
		}

		static float getGlobalCodeFontSize(Component* c);

		static EditorType* as(Component* c) 
		{ 
			if (auto pe = dynamic_cast<PopupIncludeEditor*>(c))
				return pe->getEditor();

			if (auto e = dynamic_cast<EditorType*>(c))
				return e;

			return c->findParentComponentOfClass<EditorType>();
		}

		static CodeDocument::Position getCaretPos(Component* c)
		{
			if (auto ed = as(c))
			{
#if HISE_USE_NEW_CODE_EDITOR
				auto pos = ed->editor.getTextDocument().getSelection(0).head;
				return CodeDocument::Position(getDoc(c), pos.x, pos.y);
#else
				return ed->getCaretPos();
#endif
			}

			jassertfalse;
			// make it crash...
			return *static_cast<CodeDocument::Position*>(nullptr);
		}

		static CodeDocument& getDoc(Component* c)
		{
			if (auto ed = as(c))
			{
#if HISE_USE_NEW_CODE_EDITOR
				return ed->editor.getDocument();
#else
				return ed->getDocument();
#endif
			}

			jassertfalse;
			static CodeDocument d;
			return d;
		}

		static String getCurrentToken(Component* c)
		{
			if (auto ed = as(c))
			{
#if HISE_USE_NEW_CODE_EDITOR

				auto& doc = ed->editor.getTextDocument();

				auto cs = doc.getSelection(0);

				doc.navigate(cs.tail, mcl::TextDocument::Target::subword, mcl::TextDocument::Direction::backwardCol);
				doc.navigate(cs.head, mcl::TextDocument::Target::subword, mcl::TextDocument::Direction::forwardCol);
				
				auto d = doc.getSelectionContent(cs);

				return d;
#else
				return ed->getCurrentToken();
#endif
			}
		}

		static void insertTextAtCaret(Component* c, const String& t)
		{
			if (auto ed = as(c))
			{
				
#if HISE_USE_NEW_CODE_EDITOR
				ed->editor.insert(t);
#else
				ed->insertTextAtCaret(t);
#endif
			}
		}

		static String getCurrentSelection(Component* c)
		{
			if (auto ed = as(c))
			{
#if HISE_USE_NEW_CODE_EDITOR
				auto& doc = ed->editor.getTextDocument();
				return doc.getSelectionContent(doc.getSelection(0));
#else
				return ed->getTextInRange(ed->getHighlightedRegion());
#endif
			}

			return {};
		}
	};

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
