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

		static CodeEditorComponent::ColourScheme getColourScheme() 
		{
			CodeEditorComponent::ColourScheme scheme;
			
			scheme.set("Type", Colour(0xFFAAB5B0));
			scheme.set("Properties", Colours::white);
			scheme.set("PseudoClass", Colour(0xFF4CC834));
			scheme.set("Keyword", Colours::orange);
			scheme.set("Class", Colour(0xFF41806C));
			scheme.set("ID", Colour(0xFFF57DAD));
			scheme.set("SpecialCharacters", Colour(0xFFB13532));
			scheme.set("Value", Colour(0xFFF9E2D4));
			scheme.set("Comment", Colour(0xFF555555));
			scheme.set("Important", Colours::pink);

			return scheme;
		}

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

		Tokeniser()
		{
			
		}

		StringArray pseudoClasses;
		StringArray typeIds;
		StringArray properties;

		static void skipNumberValue(CodeDocument::Iterator& source)
		{
			auto c = source.peekNextChar();
			while(!source.isEOF() && (CharacterFunctions::isLetterOrDigit(c) || c == '-'))
			{
				source.skip();
				c = source.peekNextChar();
			}
		}

		static void skipToSemicolon(CodeDocument::Iterator& source)
		{
			auto nextChar = source.peekNextChar();
			while(!source.isEOF() && nextChar != ';')
			{
				if(nextChar == '!')
				{
					break;
				}
				
				if(nextChar == '/')
				{
					source.skip();

					if(source.peekNextChar() == '*')
					{
						source.previousChar();
						break;
					}
				}

				source.skip();
				nextChar = source.peekNextChar();
			}
			
		}

		static void skipComment(CodeDocument::Iterator& source)
		{
			while(!source.isEOF())
			{
				auto n = source.nextChar();

				if(n == '*')
				{
					if(!source.isEOF())
					{
						if(source.nextChar() == '/')
							break;
					}
				}
			}
		}

		static String skipWord(CodeDocument::Iterator& source)
		{
			String s;

			auto c = source.peekNextChar();

			while(!source.isEOF() && CharacterFunctions::isLetterOrDigit(c) || c == '-')
			{
				s << c;

				source.skip();
				c = source.peekNextChar();
			}

			return s;
		}

		static bool isNumber(CodeDocument::Iterator& source)
		{
			juce_wchar nextChar = source.peekNextChar();

			return CharacterFunctions::isDigit(nextChar) || nextChar == '-';
		}

		static bool isIdentifierStart(CodeDocument::Iterator& source)
		{
			juce_wchar nextChar = source.peekNextChar();

			return CharacterFunctions::isLetter(nextChar);
		}


		int readNextToken (CodeDocument::Iterator& source) override
		{
			source.skipWhitespace();

			auto c = source.peekNextChar();

			bool isClass = false;
			bool isID = false;
			bool wasProperty = false;

			if(String("{};").containsChar(c))
			{
				source.skip();
				return (int)Token::SpecialCharacters;
			}

			if(c == '!')
			{
				source.skip();
				auto w = skipWord(source);

				if(w == "important")
					return (int)Token::Important;
				else
					return (int)Token::Value;
			}
			if(c == '/')
			{
				source.skip();

				if(source.peekNextChar() == '*')
				{
					skipComment(source);
					return (int)Token::Comment;
				}
			}

			if(c == '.')
			{
				isClass = true;
				source.skip();
			}
			if(c == '#')
			{
				isID = true;
				source.skip();
			}
			if(c == ':')
			{
				wasProperty = true;
				source.skip();
			}

			if(isIdentifierStart(source))
			{
				auto word = skipWord(source);

				if(isClass)
					return (int)Token::Class;

				if(isID)
					return (int)Token::ID;

				auto keyType = database->getKeywordType(word);

				if(keyType != KeywordDataBase::KeywordType::Undefined)
					return (int)keyType;

				if (wasProperty)
				{
					skipToSemicolon(source);
					return (int)Token::Value;
				}
				
				return (int)Token::Value;
			}

			if(wasProperty)
			{
				skipToSemicolon(source);
				return (int)Token::Value;
			}

			if(isNumber(source))
			{
				skipNumberValue(source);
				return (int)Token::Value;
			}

			source.skip();
			return (int)Token::SpecialCharacters;
			
		}

	    /** Returns a suggested syntax highlighting colour scheme. */
	    CodeEditorComponent::ColourScheme getDefaultColourScheme() override
		{
			return database->getColourScheme();
		}
	};

	LanguageManager(mcl::TextDocument& doc_):
	  doc(doc_)
	{
		
	}

	mcl::TextDocument& doc;

	void processBookmarkTitle(juce::String& bookmarkTitle) override
	{
		
	}

	SharedResourcePointer<KeywordDataBase> database;

	struct CssTokens: public mcl::TokenCollection::Provider
	{
		void addTokens(mcl::TokenCollection::List& tokens) override
		{
			StringArray names({
				"Type",
				"Property",
				"PseudoClass",
				"ReservedKeywords",
			});

			auto colours = database.getColourScheme();

			for(int i = 0; i < (int)KeywordDataBase::KeywordType::numKeywords; i++)
			{
				for(auto& s: database.getKeywords((KeywordDataBase::KeywordType)i))
				{
					auto d = new mcl::TokenCollection::Token(s);

					d->c = colours.types[i].colour;
					d->priority = i;
					d->markdownDescription << "`" << s << "` (" << names[i] << ")";

					tokens.add(d);
				}
			}
		}

		KeywordDataBase database;
	};

	void setupEditor(mcl::TextEditor* editor) override
	{
		addTokenProviders(editor->tokenCollection.get());
	}

	/** Add all token providers you want to use for this language. */
    void addTokenProviders(mcl::TokenCollection* t) override
	{
		t->addTokenProvider(new mcl::SimpleDocumentTokenProvider(doc.getCodeDocument()));
		t->addTokenProvider(new CssTokens());

		t->updateIfSync();
	}

	Identifier getLanguageId() const override { return "CSS"; }

	CodeTokeniser* createCodeTokeniser() override { return new Tokeniser(); }
};



struct StyleSheetLookAndFeel: public LookAndFeel_V3
{
	void drawButtonBackground(Graphics& g, Button& tb, const Colour&, bool, bool) override
	{
		if(auto ed = tb.findParentComponentOfClass<ComponentWithCSS>())
		{
			Renderer r(&tb);

			Selector s_id(SelectorType::ID, tb.getName().toLowerCase());
			Selector s_type(ElementType::Button);

			if(auto ss = ed->css.getOrCreateCascadedStyleSheet({s_type, s_id}))
			{
				auto currentState = Renderer::getPseudoClassFromComponent(&tb);
				ed->stateWatcher.checkChanges(&tb, ss, currentState);
				r.drawBackground(g, tb.getLocalBounds().toFloat(), ss);
			}
		}

		

		
	}
};

struct Editor: public Component,
	           public ComponentWithCSS,
		       public TopLevelWindowWithKeyMappings
{
	Editor():
	  ComponentWithCSS(),
	  doc(jdoc),
	  editor(doc),
	  tokenCollection(new mcl::TokenCollection(Identifier("CSS"))),
	  body(ElementType::Body),
	  header(SelectorType::Class, "header"),
	  content(SelectorType::Class, "content"),
	  footer(SelectorType::Class, "footer"),
	  cancel("Cancel"),
	  prev("Previous"),
	  next("Next")
	{
		setRepaintsOnMouseActivity(true);
		setSize(1600, 800);

		addAndMakeVisible(editor);
		addAndMakeVisible(list);

		
		editor.tokenCollection = tokenCollection;
		tokenCollection->setUseBackgroundThread(false);
		editor.setLanguageManager(new LanguageManager(doc));

		

		mcl::FullEditor::initKeyPresses(this);
		

		list.setLookAndFeel(&laf);
		laf.setTextEditorColours(list);

		list.setMultiLine(true);
		list.setReadOnly(true);
		list.setFont(GLOBAL_MONOSPACE_FONT());

		context.attachTo(*this);

		addAndMakeVisible(cancel);
		addAndMakeVisible(prev);
		addAndMakeVisible(next);

		cancel.setLookAndFeel(&css_laf);
		prev.setLookAndFeel(&css_laf);
		next.setLookAndFeel(&css_laf);

		auto f = File::getSpecialLocation(File::SpecialLocationType::userDesktopDirectory).getChildFile("current.css");
		jdoc.replaceAllContent(f.loadFileAsString());
		compile();
	}

	~Editor()
	{
		context.detach();
	}

	File getKeyPressSettingFile() const override { return File(); }

	mcl::TokenCollection::Ptr tokenCollection;

	hise::GlobalHiseLookAndFeel laf;

	bool keyPressed(const KeyPress& key) override
	{
		if(key == KeyPress::F5Key)
		{
			compile();
			return true;
		}

		return false;
	}

	void compile()
	{
		Parser p(jdoc.getAllContent());

		auto ok = p.parse();

		auto f = File::getSpecialLocation(File::SpecialLocationType::userDesktopDirectory).getChildFile("current.css");

		f.replaceWithText(jdoc.getAllContent());

		editor.setError(ok.getErrorMessage());

		css = p.getCSSValues();

		css.setAnimator(&animator);

		auto listContent = css.toString();

		list.setText(listContent, dontSendNotification);

		resized();
		
	}

	Rectangle<float> previewArea;

	Selector body, header, content, footer;

	Rectangle<float> areas[4];



	void resized() override
	{
		auto b = getLocalBounds();

		previewArea = b.removeFromRight(b.getWidth() / 3).toFloat();

		previewArea.removeFromRight(10);

		simple_css::Positioner pos(css, previewArea);

		areas[0] = pos.bodyArea;
		areas[1] = pos.removeFromTop(header);
		areas[3] = pos.removeFromBottom(footer);
		areas[2] = pos.removeFromTop(content);
		
		list.setBounds(b.removeFromRight(b.getWidth() / 3));

		editor.setBounds(b);

		auto footerArea = areas[3];

		if(auto fss = css[footer])
		{
			footerArea = fss->getArea(footerArea, { "margin"});
			footerArea = fss->getArea(footerArea, { "padding"});

			simple_css::Positioner pos2(css, footerArea);
			pos2.applyMargin = true;
			
			cancel.setBounds(pos2.removeFromLeft(Selector(SelectorType::ID, "cancel"), 100.0f).toNearestInt());
			next.setBounds(pos2.removeFromRight(Selector(SelectorType::ID, "next"), 100.0f).toNearestInt());
			prev.setBounds(pos2.removeFromRight(Selector(SelectorType::ID, "prev"), 100.0f).toNearestInt());
		}

		

		repaint();
	}

	StyleSheetLookAndFeel css_laf;

	TextButton cancel, prev, next;

	void paint(Graphics& g) override
	{
		g.fillAll(Colours::black);
		
		Renderer r(this);
		
		if(auto ss = css[body])
		{
			auto currentState = Renderer::getPseudoClassFromComponent(this);
			stateWatcher.checkChanges(this, ss, currentState);
			r.drawBackground(g, areas[0], ss);
		}

		if(auto ss = css[header])
			r.drawBackground(g, areas[1], ss);

		if(auto ss = css[content])
			
			r.drawBackground(g, areas[2], ss);

		if(auto ss = css[footer])
			r.drawBackground(g, areas[3], ss);

		auto b = getLocalBounds().removeFromRight(10).toFloat();

		if(auto i = animator.items.getFirst())
		{
			g.setColour(Colours::white.withAlpha(0.4f));
			g.fillRect(b);
			b = b.removeFromBottom(i->currentProgress * b.getHeight());
			g.fillRect(b);
		}
		

	}

	juce::CodeDocument jdoc;
	mcl::TextDocument doc;
	mcl::TextEditor editor;
	juce::TextEditor list;
	OpenGLContext context;
};

	
} // namespace simple_css
} // namespace hise