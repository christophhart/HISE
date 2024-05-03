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
	DECLARE_ID(EnableMap);
	DECLARE_ID(LineBreaks);
	DECLARE_ID(EnableHover);
    DECLARE_ID(AutoAutocomplete);
    DECLARE_ID(ShowStickyLines);
    DECLARE_ID(FixWeirdTab);
}

namespace TextEditorShortcuts
{
	DECLARE_ID(show_fold_map);
	DECLARE_ID(show_autocomplete);
	DECLARE_ID(goto_definition);
	DECLARE_ID(goto_file);
	DECLARE_ID(show_search);
	DECLARE_ID(show_full_search);
	DECLARE_ID(breakpoint_resume);
	DECLARE_ID(show_search_replace);
	DECLARE_ID(add_autocomplete_template);
	DECLARE_ID(clear_autocomplete_templates);
	DECLARE_ID(select_token);
    DECLARE_ID(comment_line);
    DECLARE_ID(goto_undo);
    DECLARE_ID(goto_redo);
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
		void addTokens(TokenCollection::List& tokens) override;

		StringArray templateExpressions;
		StringArray classIds;
	};

	void addAutocompleteTemplate(const String& templateExpression, const String& classId);

	void loadSettings(const File& sFile);

	static void saveSetting(Component* c, const Identifier& id, const var& newValue);

	void setReadOnly(bool shouldBeReadOnly);

	bool injectBreakpointCode(String& s);

	void setColourScheme(const juce::CodeEditorComponent::ColourScheme& s);

	void setCurrentBreakline(int n);

	void sendBlinkMessage(int n);

	void addBreakpointListener(GutterComponent::BreakpointListener* l);

	void removeBreakpointListener(GutterComponent::BreakpointListener* l);

	void enableBreakpoints(bool shouldBeEnabled);

	static void initKeyPresses(Component* root);

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

struct XmlEditor : public Component
{
	XmlEditor(const File& xmlFile, const String& content = {});

	void resized() override;

	std::function<void()> closeCallback;

	CodeDocument doc;
	mcl::TextDocument tdoc;
	mcl::FullEditor editor;
	juce::ResizableCornerComponent resizer;
};

struct MarkdownPreviewSyncer : public Timer,
                               public CodeDocument::Listener,
                               public juce::ScrollBar::Listener
{
    void setEnableScrollbarListening(bool shouldListenToScrollBars);

    void synchroniseTabs(bool editorIsSource);
    
    void scrollBarMoved(ScrollBar* scrollBarThatHasMoved, double newRangeStart) override;

    void codeDocumentTextInserted(const String& newText, int insertIndex) override;

    void codeDocumentTextDeleted(int startIndex, int endIndex) override;

    MarkdownPreviewSyncer(mcl::FullEditor& editor, MarkdownPreview& preview);;

    ~MarkdownPreviewSyncer();

    void timerCallback() override;

    bool recursiveScrollProtector = false;
    
    Component::SafePointer<MarkdownPreview> p;
    Component::SafePointer<mcl::FullEditor> e;
};

}

