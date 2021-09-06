/** ============================================================================
 *
 * TextEditor.hpp
 *
 * Copyright (C) Jonathan Zrake
 *
 * You may use, distribute and modify this code under the terms of the GPL3
 * license.
 * =============================================================================
 */


/**
 * 
 TODO:




 */


#pragma once

#define DECLARE_ID(x) static const juce::Identifier x(#x);
namespace TextEditorSettings
{
	DECLARE_ID(MapWidth);
	DECLARE_ID(LineBreaks);
	DECLARE_ID(EnableHover);
    DECLARE_ID(AutoAutocomplete);
    DECLARE_ID(FixWeirdTab);
}
#undef DECLARE_ID


namespace mcl
{

struct FullEditor: public Component,
				   public ButtonListener
{
	struct Factory : public PathFactory
	{
		Path createPath(const String& url) const override;
	} factory;

	TextEditor editor;

	FullEditor(TextDocument& d);

	struct TemplateProvider : public TokenCollection::Provider
	{
		void addTokens(TokenCollection::List& tokens) override
		{
			for (int i = 0; i < templateExpressions.size(); i++)
			{

			}
		}

		StringArray templateExpressions;
		StringArray classIds;
	};

	void addAutocompleteTemplate(const String& templateExpression, const String& classId)
	{

	}

	void loadSettings(const File& sFile)
	{
		settingFile = sFile;

		auto s = JSON::parse(settingFile);

		editor.setLineBreakEnabled(s.getProperty(TextEditorSettings::LineBreaks, true));
		mapWidth = s.getProperty(TextEditorSettings::MapWidth, 150);
		resized();
		codeMap.allowHover = s.getProperty(TextEditorSettings::EnableHover, true);
        editor.showAutocompleteAfterDelay = s.getProperty(TextEditorSettings::AutoAutocomplete, true);
	}

	static void saveSetting(Component* c, const Identifier& id, const var& newValue)
	{
		auto pe = c->findParentComponentOfClass<FullEditor>();
		
		auto s = JSON::parse(pe->settingFile);

		if (s.getDynamicObject() == nullptr)
			s = var(new DynamicObject());

		s.getDynamicObject()->setProperty(id, newValue);
		pe->settingFile.replaceWithText(JSON::toString(s));

		if (id == TextEditorSettings::MapWidth)
		{
			pe->mapWidth = (int)newValue;
			pe->resized();
		}
		if (id == TextEditorSettings::EnableHover)
		{
			pe->codeMap.allowHover = (bool)newValue;
		}
		if (id == TextEditorSettings::AutoAutocomplete)
		{
			pe->editor.showAutocompleteAfterDelay = (bool)newValue;
		}
		if (id == TextEditorSettings::LineBreaks)
		{
			pe->editor.setLineBreakEnabled((bool)newValue);
		}
	}

	void setReadOnly(bool shouldBeReadOnly)
	{
		editor.setReadOnly(shouldBeReadOnly);
	}

	bool injectBreakpointCode(String& s)
	{
		return editor.gutter.injectBreakPoints(s);
	}

	void setColourScheme(const juce::CodeEditorComponent::ColourScheme& s)
	{
		editor.colourScheme = s;
	}

	void setCurrentBreakline(int n)
	{
		editor.gutter.setCurrentBreakline(n);
	}

	void sendBlinkMessage(int n)
	{
		editor.gutter.sendBlinkMessage(n);
	}

	void addBreakpointListener(GutterComponent::BreakpointListener* l)
	{
		editor.gutter.addBreakpointListener(l);
	}

	void removeBreakpointListener(GutterComponent::BreakpointListener* l)
	{
		editor.gutter.removeBreakpointListener(l);
	}

	void enableBreakpoints(bool shouldBeEnabled)
	{
		editor.gutter.setBreakpointsEnabled(shouldBeEnabled);
	}

	static mcl::FoldableLineRange::List createMarkdownLineRange(const CodeDocument& doc);

	bool keyPressed(const KeyPress& k) override;

	void buttonClicked(Button* b) override;

	void resized() override;

	void paint(Graphics& g) override;

	int mapWidth = 150;

	bool overlayFoldMap = false;

	HiseShapeButton mapButton, foldButton;
	CodeMap codeMap;
	FoldMap foldMap;

	File settingFile;

	using SettingFunction = std::function<void(bool, DynamicObject::Ptr)>;

	SettingFunction settingFunction;

	juce::ComponentBoundsConstrainer constrainer;

	var settings;
};

struct MarkdownPreviewSyncer : public Timer,
                               public CodeDocument::Listener,
                               public juce::ScrollBar::Listener
{
    void setEnableScrollbarListening(bool shouldListenToScrollBars);

    void synchroniseTabs(bool editorIsSource);
    
    void scrollBarMoved(ScrollBar* scrollBarThatHasMoved, double newRangeStart) override;

    void codeDocumentTextInserted(const String& newText, int insertIndex) override
    {
        startTimer(500);
    }

    void codeDocumentTextDeleted(int startIndex, int endIndex) override
    {
        startTimer(500);
    }
    
    MarkdownPreviewSyncer(mcl::FullEditor& editor, MarkdownPreview& preview) :
        p(preview),
        e(editor)
    {
        e.editor.getTextDocument().getCodeDocument().addListener(this);
    };

    ~MarkdownPreviewSyncer()
    {
        e.editor.getTextDocument().getCodeDocument().removeListener(this);
    }
    
    void timerCallback() override
    {
        {
            MarkdownRenderer::ScopedScrollDisabler sds(p.renderer);
            ScopedValueSetter<bool> svs(recursiveScrollProtector, true);

            if (p.isVisible())
                p.setNewText(e.editor.getTextDocument().getCodeDocument().getAllContent(), {}, false);

            stopTimer();
        }
        
        synchroniseTabs(true);
    }

    bool recursiveScrollProtector = false;
    
    MarkdownPreview& p;
    mcl::FullEditor& e;
};

}

