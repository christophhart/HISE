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
*   which must be separately licensed for closed source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

namespace hise {
namespace simple_css
{
using namespace juce;

struct LanguageManager: public mcl::LanguageManager
{
	struct KeywordDataBase
	{
		enum class KeywordType
		{
			Type,
			Property,
			PseudoClass,
			ReservedKeywords,
			numKeywords,
			Undefined
		};

		static CodeEditorComponent::ColourScheme getColourScheme();

		KeywordDataBase();

		const StringArray& getKeywords(KeywordType type) const
		{
			return keywords[(int)type];
		}

		std::array<StringArray, (int)KeywordType::numKeywords> keywords;

		KeywordType getKeywordType(const String& t)
		{
			int idx = 0;
			for(const auto& s: keywords)
			{
				if(s.contains(t))
					return (KeywordType)idx;

				idx++;
			}

			return KeywordType::Undefined;
		}
	};

	struct Tokeniser: public CodeTokeniser
	{
		SharedResourcePointer<KeywordDataBase> database;

		enum class Token
		{
			Type,
			Properties,
			PseudoClass,
			Keyword,
			Class,
			ID,
			SpecialCharacters,
			Value,
			Comment,
			Important,
			numTokens
		};

		Tokeniser() = default;

		static void skipNumberValue(CodeDocument::Iterator& source);
		static void skipToSemicolon(CodeDocument::Iterator& source);
		static void skipComment(CodeDocument::Iterator& source);
		static String skipWord(CodeDocument::Iterator& source);
		static bool isNumber(CodeDocument::Iterator& source);
		static bool isIdentifierStart(CodeDocument::Iterator& source);

		int readNextToken (CodeDocument::Iterator& source) override;

		/** Returns a suggested syntax highlighting colour scheme. */
	    CodeEditorComponent::ColourScheme getDefaultColourScheme() override
		{
			return database->getColourScheme();
		}
	};

	LanguageManager(mcl::TextDocument& doc_);

	mcl::TextDocument& doc;

	void processBookmarkTitle(juce::String& bookmarkTitle) override
	{
		
	}

	SharedResourcePointer<KeywordDataBase> database;

	struct CssTokens: public mcl::TokenCollection::Provider
	{
		void addTokens(mcl::TokenCollection::List& tokens) override;

		KeywordDataBase database;
	};

	void setupEditor(mcl::TextEditor* editor) override;

	/** Add all token providers you want to use for this language. */
    void addTokenProviders(mcl::TokenCollection* t) override;

	Identifier getLanguageId() const override { return "CSS"; }

	CodeTokeniser* createCodeTokeniser() override { return new Tokeniser(); }
};



struct StyleSheetLookAndFeel: public LookAndFeel_V3
{
	void drawButtonBackground(Graphics& g, Button& tb, const Colour&, bool, bool) override;

	void drawButtonText(Graphics& g, TextButton& tb, bool over, bool down) override;

	void fillTextEditorBackground (Graphics&, int width, int height, TextEditor&) override;
	void drawTextEditorOutline (Graphics&, int width, int height, TextEditor&) override {};

	

	
	
};

struct Editor: public Component,
	           public ComponentWithCSS,
		       public TopLevelWindowWithKeyMappings
{
	Editor();

	~Editor()
	{
		TopLevelWindowWithKeyMappings::saveKeyPressMap();
		context.detach();
	}

	File getKeyPressSettingFile() const override { return File::getSpecialLocation(File::SpecialLocationType::userDesktopDirectory).getChildFile("something.js"); }
	bool keyPressed(const KeyPress& key) override;

	void compile();
	void resized() override;
	void paint(Graphics& g) override;

	mcl::TokenCollection::Ptr tokenCollection;
	hise::GlobalHiseLookAndFeel laf;
	Rectangle<float> previewArea;
	Selector body, header, content, footer;
	Rectangle<float> areas[4];
	StyleSheetLookAndFeel css_laf;
	TextButton cancel, prev, next;

	TextEditor textInput;

	juce::CodeDocument jdoc;
	mcl::TextDocument doc;
	mcl::TextEditor editor;
	juce::TextEditor list;
	OpenGLContext context;
};

	
} // namespace simple_css
} // namespace hise