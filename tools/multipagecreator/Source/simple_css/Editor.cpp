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

namespace hise
{
namespace simple_css
{
	CodeEditorComponent::ColourScheme LanguageManager::KeywordDataBase::getColourScheme()
	{
		CodeEditorComponent::ColourScheme scheme;
			
		scheme.set("Type", Colour(0xffDDAADD));
		scheme.set("Properties", Colour(0xffbbbbff));
		scheme.set("PseudoClass", Colour(0xffEEAA00));
		scheme.set("Keyword", Colours::orange);
		scheme.set("Class", Colour(0xff88bec5));
		scheme.set("ID", Colour(0xffDDAAAA));
		scheme.set("SpecialCharacters", Colours::white);
		scheme.set("Value", Colour(0xffCCCCEE));
		scheme.set("Comment", Colour(0xff77CC77));
		scheme.set("Important", Colour(0xffBB3333));

#if 0
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
#endif

		return scheme;
	}

LanguageManager::KeywordDataBase::KeywordDataBase()
{
	keywords[(int)KeywordType::PseudoClass] = { "hover", "active", "focus" };
	keywords[(int)KeywordType::Type] = { "button", "body", "div", "select", "input" };
	keywords[(int)KeywordType::Property] = {
		"::selection",
		"background", "background-color", "background-size", "background-position",
        "border", "border-width", "border-style", "border-color",
        "border-radius", "border-top-left-radius", "border-top-right-radius", "border-bottom-left-radius", "border-bottom-right-radius",
        "box-shadow",
		"color",
        "cursor",
        "ease", "ease-in", "ease-in-out", "linear",
        "font-family", "font-size", "font-weight", "font-stretch",
        "height",
        "letter-spacing",
		"margin", "margin-top", "margin-left", "margin-right", "margin-bottom",
        "opacity",
		"padding", "padding-top", "padding-left", "padding-right", "padding-bottom",
        "text-align",
        "text-transform",
		"transition",
		"transform",
        "vertical-align",
        "width"
	};
}

void LanguageManager::Tokeniser::skipNumberValue(CodeDocument::Iterator& source)
{
	auto c = source.peekNextChar();
	while(!source.isEOF() && (CharacterFunctions::isLetterOrDigit(c) || c == '-'))
	{
		source.skip();
		c = source.peekNextChar();
	}
}

void LanguageManager::Tokeniser::skipToSemicolon(CodeDocument::Iterator& source)
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

void LanguageManager::Tokeniser::skipComment(CodeDocument::Iterator& source)
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

String LanguageManager::Tokeniser::skipWord(CodeDocument::Iterator& source)
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

bool LanguageManager::Tokeniser::isNumber(CodeDocument::Iterator& source)
{
	juce_wchar nextChar = source.peekNextChar();

	return CharacterFunctions::isDigit(nextChar) || nextChar == '-';
}

bool LanguageManager::Tokeniser::isIdentifierStart(CodeDocument::Iterator& source)
{
	juce_wchar nextChar = source.peekNextChar();

	return CharacterFunctions::isLetter(nextChar);
}

int LanguageManager::Tokeniser::readNextToken(CodeDocument::Iterator& source)
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

		if(source.peekNextChar() == ':')
		{
			source.skip();
			skipWord(source);
			return (int)Token::PseudoClass;
		}
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

LanguageManager::LanguageManager(mcl::TextDocument& doc_):
	doc(doc_)
{
		
}

void LanguageManager::CssTokens::addTokens(mcl::TokenCollection::List& tokens)
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

void LanguageManager::setupEditor(mcl::TextEditor* editor)
{
	addTokenProviders(editor->tokenCollection.get());
}

void LanguageManager::addTokenProviders(mcl::TokenCollection* t)
{
	t->addTokenProvider(new mcl::SimpleDocumentTokenProvider(doc.getCodeDocument()));
	t->addTokenProvider(new CssTokens());

	t->updateIfSync();
}

void StyleSheetLookAndFeel::drawButtonBackground(Graphics& g, Button& tb, const Colour& colour, bool cond, bool cond1)
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

void StyleSheetLookAndFeel::drawButtonText(Graphics& g, TextButton& tb, bool over, bool down)
{
	if(auto ed = tb.findParentComponentOfClass<ComponentWithCSS>())
	{
		Renderer r(&tb);

		Selector s_id(SelectorType::ID, tb.getName().toLowerCase());
		Selector s_type(ElementType::Button);

		if(auto ss = ed->css.getOrCreateCascadedStyleSheet({s_type, s_id}))
		{
			auto currentState = Renderer::getPseudoClassFromComponent(&tb);

			auto totalArea = tb.getLocalBounds().toFloat();

			totalArea = ss->getArea(totalArea, { "margin", currentState });
			totalArea = ss->getArea(totalArea, { "padding", currentState });

			auto f = ss->getFont(currentState, totalArea);
			g.setFont(f);
			r.setCurrentBrush(g, ss, totalArea, {"color", currentState }, Colours::black);

			auto j = ss->getJustification(currentState);

			auto text = ss->getText(tb.getButtonText(), currentState);

			g.drawText(text, totalArea, j);

		}
	}
		
}

void StyleSheetLookAndFeel::fillTextEditorBackground(Graphics& g, int width, int height, TextEditor& textEditor)
{
	if(auto ed = textEditor.findParentComponentOfClass<ComponentWithCSS>())
	{
		Renderer r(&textEditor);

		Selector s_id(SelectorType::ID, textEditor.getName().toLowerCase());
		Selector s_type(ElementType::TextInput);

		if(auto ss = ed->css.getOrCreateCascadedStyleSheet({s_type, s_id}))
		{
			auto currentState = Renderer::getPseudoClassFromComponent(&textEditor);
			ed->stateWatcher.checkChanges(&textEditor, ss, currentState);

			ss->setDefaultColour("background-color", textEditor.findColour(TextEditor::backgroundColourId));
			ss->setDefaultColour("color", textEditor.findColour(TextEditor::textColourId));
			
			r.drawBackground(g, textEditor.getLocalBounds().toFloat(), ss);
		}
		else
		{
			LookAndFeel_V3::fillTextEditorBackground(g, width, height, textEditor);
			LookAndFeel_V3::drawTextEditorOutline(g, width, height, textEditor);
		}
	}
}

Editor::Editor():
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
	TopLevelWindowWithKeyMappings::loadKeyPressMap();
	setRepaintsOnMouseActivity(true);
	setSize(1600, 800);

	addAndMakeVisible(editor);
	addAndMakeVisible(list);

	addAndMakeVisible(textInput);

	

		
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
	textInput.setLookAndFeel(&css_laf);

	stateWatcher.registerComponentToUpdate(&textInput);

	auto f = File::getSpecialLocation(File::SpecialLocationType::userDesktopDirectory).getChildFile("current.css");
	jdoc.replaceAllContent(f.loadFileAsString());
	compile();
}

bool Editor::keyPressed(const KeyPress& key)
{
	if(key == KeyPress::F5Key)
	{
		compile();
		return true;
	}

	return false;
}

void Editor::compile()
{
	Parser p(jdoc.getAllContent());

	auto ok = p.parse();
	auto f = File::getSpecialLocation(File::SpecialLocationType::userDesktopDirectory).getChildFile("current.css");
	f.replaceWithText(jdoc.getAllContent());
	editor.setError(ok.getErrorMessage());
	css = p.getCSSValues();
	css.setAnimator(&animator);

	list.setText(css.toString(), dontSendNotification);

	resized();
		
}

void Editor::resized()
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

		pos2.applyMargin = false;
		textInput.setBounds(pos2.removeFromLeft(Selector(ElementType::TextInput), 180.0f).toNearestInt());

		

		if(auto c = css.getOrCreateCascadedStyleSheet({ Selector(ElementType::Button), Selector(SelectorType::ID, "next")}))
		{
			next.setMouseCursor(c->getMouseCursor());
		}
	}

		

	repaint();
}

void Editor::paint(Graphics& g)
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
}
}


