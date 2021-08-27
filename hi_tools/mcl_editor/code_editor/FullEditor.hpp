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
	
	static juce::Array<juce::Identifier> getAllIds()
	{
		static const juce::Array<juce::Identifier> ids = { MapWidth, LineBreaks, EnableHover };

		return ids;
	}
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

	void setMarkdownMode()
	{
		editor.setCodeTokeniser(new MarkdownParser::Tokeniser());
		editor.colourScheme = editor.tokeniser->getDefaultColourScheme();
		editor.setLineRangeFunction(mcl::FullEditor::createMarkdownLineRange);
		editor.setEnableAutocomplete(false);
		enableBreakpoints(false);
	}

	void loadSettings(const var& s)
	{
		editor.setLineBreakEnabled(s.getProperty(TextEditorSettings::LineBreaks, true));
		mapWidth = s.getProperty(TextEditorSettings::MapWidth, 150);
		resized();
		codeMap.allowHover = s.getProperty(TextEditorSettings::EnableHover, true);
	}

	void saveSettings(DynamicObject::Ptr obj) const
	{
		if (obj != nullptr)
		{
			obj->setProperty(TextEditorSettings::LineBreaks, editor.linebreakEnabled);
			obj->setProperty(TextEditorSettings::MapWidth, mapWidth);
			obj->setProperty(TextEditorSettings::EnableHover, codeMap.allowHover);
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

	void buttonClicked(Button* b) override;

	void resized() override;

	int mapWidth = 150;

	HiseShapeButton mapButton, foldButton;
	CodeMap codeMap;
	FoldMap foldMap;

	using SettingFunction = std::function<void(bool, DynamicObject::Ptr)>;

	SettingFunction settingFunction;

	juce::ComponentBoundsConstrainer constrainer;
	ResizableEdgeComponent edge;

	var settings;
};

}

