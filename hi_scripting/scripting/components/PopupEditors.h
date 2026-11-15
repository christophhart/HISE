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

struct EditorBottomBar : public Component,
						 public ControlledObject,
					     public Timer,
						 public ButtonListener,
						 public ComboBoxListener
{
	static constexpr int BOTTOM_HEIGHT = 24;

	EditorBottomBar(JavascriptProcessor* jp);

	void resized() override;

	void buttonClicked(Button* b) override;

	void comboBoxChanged(ComboBox* cb) override;

	void recompile();

	void clearError(UriError::SourceType s);

	void setError(const UriError& e);

	void setCurrentlyParsing();

	ScopedPointer<DebugConsoleTextEditor> resultLabel;

	struct Factory : public PathFactory
	{
		Path createPath(const String& url) const override;
	} factory;

	void paint(Graphics& g);

	void setCompileFunction(const std::function<void()>& f)
	{
		compileFunction = f;
	}

	struct ButtonLAF : public LookAndFeel_V4
	{
		void drawButtonText(Graphics &g, TextButton &button, bool isMouseOverButton, bool isButtonDown) override
		{
			float alpha = 0.6f;

			if (isMouseOverButton)
				alpha += 0.2f;

			if (isButtonDown)
				alpha += 0.2f;

			g.setFont(GLOBAL_BOLD_FONT());
			g.setColour(Colours::white.withAlpha(alpha));

			g.drawText(button.getButtonText(), button.getLocalBounds().toFloat(), Justification::centred);
		}

		void drawButtonBackground(Graphics& g, Button& button, const Colour& /*backgroundColour*/,
			bool isMouseOverButton, bool isButtonDown) override
		{
			if (isMouseOverButton)
				g.fillAll(Colours::white.withAlpha(0.04f));
		}

	} blaf;

	void timerCallback();

	ScopedPointer<TextButton> compileButton;
	ScopedPointer<HiseShapeButton> resumeButton;
	ScopedPointer<HiseShapeButton> nextButton;
	ScopedPointer<HiseShapeButton> overButton;

	ScopedPointer<HiseShapeButton> errorButton;

	bool isHalted = false;
	
	void onLocationChange(const UriLocation& l)
	{
		if (callstackSelector.isVisible())
		{
			auto currentError = getCurrentError();

			if (currentError.matches(l))
				callstackSelector.setSelectedItemIndex(0, dontSendNotification);

			currentError.forInvertedCallstack([&](int index, const UriLocation::WithExpression& e)
			{
				if (e.second.matches(l))
				{
					callstackSelector.setSelectedId(index + 2, dontSendNotification);
				}
			});
		}
		
	}

private:

	UriError getCurrentError() const;

	std::map<UriError::SourceType, UriError> errors;

	void gotoLocation(const UriLocation& l);

	std::function<void()> compileFunction;

	struct CallstackLAF : public PopupLookAndFeel
	{
		void drawComboBox(Graphics& g, int width, int height, bool /*isButtonDown*/, int /*buttonX*/, int /*buttonY*/, int /*buttonW*/, int /*buttonH*/, ComboBox& c)
		{
			g.setColour(Colours::white.withAlpha(0.3f));

			g.setFont(GLOBAL_MONOSPACE_FONT());
			g.drawText(c.getText(), c.getLocalBounds(), Justification::left);

			Path arrow;
			arrow.addTriangle({ 0.0f, 0.0f }, { 1.0f, 0.0f }, { 0.5f, 1.0f });
			auto tb = c.getLocalBounds().removeFromRight(c.getHeight()).toFloat();
			PathFactory::scalePath(arrow, tb.withSizeKeepingCentre(10, 8));
			g.fillPath(arrow);
		}

		void drawLabel(Graphics& g, Label& l) {};
	} callstackLaf;

	ComboBox callstackSelector;

	JavascriptProcessor* jp;
};


class PopupIncludeEditor : public Component,
						   public Dispatchable,
                           public GlobalScriptCompileListener,
						   public JavascriptThreadPool::SleepListener,
						   public UriErrorLogger::Listener,
						   public UriBreakpoint::Owner::UriBreakpointListener
{
public:

    void scriptWasCompiled(JavascriptProcessor* p) override;
    
	static bool matchesId(Component* c, const Identifier& id)
	{
		return CommonEditorFunctions::as(c)->findParentComponentOfClass<PopupIncludeEditor>()->callback == id;
	}

	static float getGlobalCodeFontSize(Component* c);

	// ================================================================================================================

	PopupIncludeEditor(JavascriptProcessor *s, const File &fileToEdit);
	PopupIncludeEditor(JavascriptProcessor* s, const Identifier &callback);
	
	~PopupIncludeEditor();

	void onUriBreakpointEvent(const UriBreakpoint& knownEvent, const UriError& breakpoint) override;
	void onError(const UriError& e, int numOccurrences) override;
	void onClear(UriErrorLogger::SourceType s) override;
	void onLocationTree(UriLocationTree::Ptr newTree) override;

	int getSourceTypeMask() const override 
	{ 
		return UriError::SourceType::Runtime |
			   UriError::SourceType::Debug |
			   UriError::SourceType::Include |
			   UriError::SourceType::Compilation |
			   UriError::SourceType::Diagnostics; 
	}

	bool keyPressed(const KeyPress& key) override;

	void resized() override;;

	void onBreakpointTooltip(mcl::TooltipWithArea::Data& d);

	static bool gotoUriLocation(UriLocation::Owner* owner, Component* c, const UriLocation& location);

	void gotoChar(int character, int lineNumber = -1);

	void paintOverChildren(Graphics& g) override;

	static void initKeyPresses(Component* root);

	static void blink(MainController* mc, const UriLocation& location);

	File getFile() const;
	
	JavascriptProcessor* getScriptProcessor() { return jp; }

	static File getPropertyFile()
	{
		return ProjectHandler::getAppDataDirectory(nullptr).getChildFile("code_editor.json");
	}

	using EditorType = CommonEditorFunctions::EditorType;

	bool matchesUriLocation(const UriLocation& loc) const override;

	bool sleepStateChanged(const UriError& location, bool on) override;

	void addEditor(CodeDocument& d, bool isJavascript);

	EditorType* getEditor() { return editor; }
	const EditorType* getEditor() const { return editor; }

	ScopedPointer<EditorType> editor;

	ScopedPointer<mcl::TextDocument> doc;

	Identifier callback;
	WeakReference<JavascriptProcessor> jp;

	ExternalScriptFile::Ptr externalFile;

	ScopedPointer<EditorBottomBar> bottomBar;

	bool isJavascript() const { return t == FileTypes::Javascript; }

private:

	UriLocation baseLocation;

	enum class FileTypes
	{
		Javascript = 0,
		GLSL,
		CSS
	};

	FileTypes t = FileTypes::Javascript;

    void checkUnreferencedExternalFile()
    {
        bool before = unreferencedExternalFile;
        
        auto f = getFile();
        
        if(f.existsAsFile())
        {
            unreferencedExternalFile = true;
            
            for(int i = 0; i < jp->getNumWatchedFiles(); i++)
            {
                if(f == jp->getWatchedFile(i))
                {
                    unreferencedExternalFile = false;
                    break;
                }
            }
        }
        else
            unreferencedExternalFile = false;
        
        if(auto pc = findParentComponentOfClass<PanelWithProcessorConnection>())
        {
            if(before && !unreferencedExternalFile)
            {
                pc->refreshSelectorValue(dynamic_cast<Processor*>(jp.get()), getFile().getFileName());
            }
            else if (unreferencedExternalFile)
            {
                pc->setDynamicTitle(getFile().getFileName() + " (detached)");
            }
        }
        
        
    }

    void addButtonAndCompileLabel();
	void refreshAfterCompilation(const UriError& e);
	void compileInternal();

    bool unreferencedExternalFile = false;
    
	friend class PopupIncludeEditorWindow;

	bool isCallbackEditor() { return !callback.isNull(); }
	
	int fontSize;

	ScopedPointer<JavascriptTokeniser> tokeniser;

#if USE_BACKEND
	void shadowParse(bool printToConsole=true);
#endif
	
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PopupIncludeEditor);
	JUCE_DECLARE_WEAK_REFERENCEABLE(PopupIncludeEditor);

	// ================================================================================================================
};

} // namespace hise

#endif  // POPUPEDITORS_H_INCLUDED
