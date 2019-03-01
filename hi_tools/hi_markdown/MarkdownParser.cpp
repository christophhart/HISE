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
using namespace juce;


void MarkdownParser::parse()
{
	lastWidth = -1.0f;
	firstDraw = true;

	try
	{
		while (it.peek() != 0)
			parseBlock();

		currentParseResult = Result::ok();
	}
	catch (String& s)
	{
		currentParseResult = Result::fail(s);
	}

	for (auto l : listeners)
	{
		if (l.get() != nullptr)
			l->markdownWasParsed(currentParseResult);
	}
}


void MarkdownParser::parseLine()
{
	resetForNewLine();
	currentColour = styleData.textColour.withAlpha(0.8f);

	parseText();

	while (!Helpers::isNewElement(it.peek()))
	{
		parseText();
	}

	elements.add(new TextBlock(this, currentlyParsedBlock));
}

void MarkdownParser::resetForNewLine()
{
	currentFont = styleData.getFont();
	currentColour = styleData.textColour;
	resetCurrentBlock();
}

void MarkdownParser::parseHeadline()
{
	resetCurrentBlock();

	currentColour = Colour(SIGNAL_COLOUR);

	juce::juce_wchar c = it.peek();
	int headlineLevel = 3;

	while (it.next(c) && c == '#' && headlineLevel > 0)
	{
		headlineLevel--;
	}

	headlineLevel = jmax(0, headlineLevel);

	currentFont = styleData.f.withHeight(styleData.fontSize * 3 / 2 + 7 * headlineLevel);

	currentFont = FontHelpers::getFontBoldened(currentFont);


	if (it.peek() == ' ')
		it.advance();

	parseText();

	isBold = false;

	elements.add(new Headline(this, headlineLevel, currentlyParsedBlock, elements.size() == 0));

}

void MarkdownParser::parseBulletList()
{
	Array<AttributedString> bulletpoints;

	while (it.peek() == '-')
	{
		skipTagAndTrailingSpace();

		resetCurrentBlock();
		resetForNewLine();

		parseText();

		bulletpoints.add(currentlyParsedBlock);
	}

	elements.add(new BulletPointList(this, bulletpoints));


	currentFont = styleData.getFont();

}

void MarkdownParser::parseEnumeration()
{
	Array<AttributedString> listItems;

	while (CharacterFunctions::isDigit(it.peek()))
	{
		while(CharacterFunctions::isDigit(it.peek()))
			skipTagAndTrailingSpace();
		
		if (it.peek() == '.')
		{
			skipTagAndTrailingSpace(); // the dot

			resetCurrentBlock();
			resetForNewLine();

			while (!Helpers::isNewElement(it.peek()))
			{
				parseText();
			}

			listItems.add(currentlyParsedBlock);
		}
		else
		{
			parseLine();
			return;
		}
	}

	elements.add(new EnumerationList(this, listItems));

	currentFont = styleData.getFont();
}




void MarkdownParser::parseText(bool stopAtEndOfLine)
{
	juce_wchar c;

	it.next(c);



	//while (!Helpers::isEndOfLine(c))

	while (Helpers::belongsToTextBlock(c, isCode, stopAtEndOfLine))
	{
		switch (c)
		{
		case '*':
		{
			if (!isCode)
			{
				if (it.peek() == '*')
				{
					it.next(c);

					isBold = !isBold;

					float size = currentFont.getHeight();

					if (isBold)
						currentFont = FontHelpers::getFontBoldened(styleData.getFont().withHeight(size));
					else
						currentFont = styleData.getFont().withHeight(size);
				}
				else
				{
					isItalic = !isItalic;

					if (isItalic)
						currentFont = FontHelpers::getFontItalicised(currentFont);
					else
						currentFont = FontHelpers::getFontNormalised(currentFont);
				}
			}
			else
			{
				addCharacterToCurrentBlock(c);
			}

			break;
		}
		case '`':
		{
			isCode = !isCode;

			auto size = currentFont.getHeight();
			auto b = currentFont.isBold();
			auto i = currentFont.isItalic();
			auto u = currentFont.isUnderlined();

			currentFont = isCode ? GLOBAL_MONOSPACE_FONT() : styleData.getFont();

			currentColour = isCode ? styleData.textColour : styleData.textColour.withAlpha(0.8f);

			currentFont.setHeight(size);
			currentFont.setBold(b);
			currentFont.setItalic(i);
			currentFont.setUnderline(u);

			break;
		}
		case ' ':
		{
			if (it.peek() == ' ')
			{
				it.next(c);
				addCharacterToCurrentBlock('\n');
			}
			else
			{
				addCharacterToCurrentBlock(c);
			}

			break;
		}
		case '[':
		{
			if (isCode)
			{
				addCharacterToCurrentBlock(c);
			}
			else
			{
				String urlId;
				String url;

				bool ok = false;

				while (it.next(c))
				{
					if (c == ']')
					{
						ok = true;
						break;
					}
					else
						urlId << c;
				}

				if (ok)
				{
					it.match('(');

					ok = false;

					while (it.next(c))
					{
						if (c == ')')
						{
							ok = true;
							break;
						}
						else
							url << c;
					}
				}

				if (urlId.toLowerCase().startsWith("button: "))
				{
					urlId = urlId.fromFirstOccurrenceOf("Button: ", false, true);


				}

				if (ok)
				{
					auto text = currentlyParsedBlock.getText();

					auto start = text.length();

					// This is extremely annoying, but the glyph's string range will duplicate space characters...
					auto duplicateSpaces = [](int& number, const String& s)
					{
						auto c = s.getCharPointer();

						while (*c != 0)
						{
							if (*c == ' ')
								number++;
							if (*c == '\n')
								number++;

							c++;
						}
					};

					//duplicateSpaces(start, text);

					currentFont.setUnderline(true);
					currentlyParsedBlock.append(urlId, currentFont, Colour(SIGNAL_COLOUR));
					currentFont.setUnderline(false);

					auto stop = start + urlId.length();

					HyperLink hyperLink;

					hyperLink.url = url;
					hyperLink.urlRange = { start, stop };
					hyperLink.displayString = urlId;
					hyperLink.valid = true;

					currentLinks.add(std::move(hyperLink));
				}
				else
					throw String("Error at parsing URL link");
			}

			

			break;
		}
		case '|':
			if (!isCode)
			{
				return;
			}
		case '\n':
		{
			if (!stopAtEndOfLine)
				it.advanceIfNotEOF();
		}
		default:
			addCharacterToCurrentBlock(c);

		}

		if (!it.next(c))
			return;
	}
}



void MarkdownParser::parseBlock()
{
	juce_wchar c = it.peek();

	switch (c)
	{
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9': parseEnumeration(); break;
	case '#': parseHeadline();
		break;
	case '-': parseBulletList();
		break;
	case '>': parseComment();
		break;
	case '`': if (isJavascriptBlock())
		parseJavascriptBlock();
			  else
				  parseLine();
		break;
	case '|': parseTable();
		break;
	case '$': parseButton();
		break;
	case '!': if (isImageLink())
		elements.add(parseImage());
			  else
				  parseLine();
		break;
	case '\n': it.match('\n');
		break;
	default:  parseLine();
		break;

	};
}

void MarkdownParser::parseJavascriptBlock()
{
	auto code = it.getRestString();

	code = code.fromFirstOccurrenceOf("```", false, false);

	auto type = CodeBlock::SyntaxType::Undefined;

	int numToSkip = 0;

	if (code.startsWith("cpp"))
	{
		type = CodeBlock::SyntaxType::Cpp;
		numToSkip = 3;

	}
	else if (code.startsWith("javascript"))
	{
		type = CodeBlock::SyntaxType::Javascript;
		numToSkip = 10;
	}
	else if (code.startsWith("!javascript"))
	{
		type = CodeBlock::SyntaxType::LiveJavascript;
		numToSkip = 11;
	}
	else if (code.startsWith("xml"))
	{
		type = CodeBlock::SyntaxType::XML;
		numToSkip = 3;
	}
	else if (code.startsWith("snippet"))
	{
		type = CodeBlock::SyntaxType::Snippet;
		numToSkip = 7;
	}

	code = code.substring(numToSkip);

	code = code.upToFirstOccurrenceOf("```", true, false);

	if (!code.endsWith("```"))
		throw String("missing end tag for code block. ```");

	code = code.upToFirstOccurrenceOf("```", false, false);

	if (!it.advanceIfNotEOF(code.length() + numToSkip + 6))
		throw String("End tag missing");

	elements.add(new CodeBlock(this, code, type));
}

void MarkdownParser::parseButton()
{
	it.match('$');
	it.match('[');

	String urlId;
	String link;

	juce_wchar c;

	bool ok = false;

	while (it.next(c))
	{
		if (c == ']')
		{
			ok = true;
			break;
		}
		else
			urlId << c;
	}

	if (ok)
	{
		it.match('(');

		while (it.next(c))
		{
			if (c == ')')
			{
				ok = true;
				break;
			}
			else
				link << c;
		}

		if (!ok)
		{
			it.match(')');
			jassertfalse;
		}
	}
	else
	{
		// should throw
		it.match('[');

		jassertfalse;
	}

	elements.add(new ActionButton(this, urlId, link));
}



void MarkdownParser::parseTable()
{
	RowContent headerItems;


	it.peek();

	while (!Helpers::isEndOfLine(it.peek()))
	{
		skipTagAndTrailingSpace();

		resetForNewLine();
		resetCurrentBlock();

		CellContent newCell;

		if (isImageLink())
		{
			ScopedPointer<ImageElement> e = parseImage();
			newCell.imageURL = e->getImageURL();
		}
		else
		{
			parseText();
			currentlyParsedBlock.setFont(FontHelpers::getFontBoldened(styleData.getFont()));
			newCell.s = currentlyParsedBlock;
		}


		if (!newCell.isEmpty())
			headerItems.add(newCell);
	}

	if (!it.advanceIfNotEOF())
		throw String("Table lines needed");

	Array<int> lengths;

	while (!Helpers::isEndOfLine(it.peek()))
	{
		resetCurrentBlock();

		parseText();

		if (!currentlyParsedBlock.getText().containsOnly("-=_ "))
		{
			throw String("Table lines illegal text: " + currentlyParsedBlock.getText());
		}

		auto text = currentlyParsedBlock.getText().trim();

		if (text.isNotEmpty())
			lengths.add(text.length());
	}

	Array<RowContent> rows;

	if (!it.advanceIfNotEOF())
		throw String("No table rows defined");

	while (it.peek() == '|')
	{
		rows.add(parseTableRow());
	}

	elements.add(new MarkdownTable(this, headerItems, lengths, rows));

}

MarkdownParser::ImageElement* MarkdownParser::parseImage()
{
	it.match('!');
	it.match('[');

	auto imageName = it.getRestString().upToFirstOccurrenceOf("]", false, false);

	it.advance(imageName);

	it.match(']');
	it.match('(');

	auto imageLink = it.getRestString().upToFirstOccurrenceOf(")", false, false);

	it.advance(imageLink);

	it.match(')');

	return new ImageElement(this, imageName, imageLink);


}

Array<MarkdownParser::CellContent> MarkdownParser::parseTableRow()
{
	Array<CellContent> entries;

	while (!Helpers::isEndOfLine(it.peek()))
	{
		skipTagAndTrailingSpace();
		resetCurrentBlock();
		resetForNewLine();

		CellContent c;

		if (isImageLink())
		{
			ScopedPointer<ImageElement> e = parseImage();
			c.imageURL = e->getImageURL();
		}
		else
		{
			parseText();
			c.s = currentlyParsedBlock;
			c.cellLinks = currentLinks;
		}

		if (!c.isEmpty())
			entries.add(c);
	}


	if (!it.advanceIfNotEOF())
		return entries;

	return entries;
}

bool MarkdownParser::isJavascriptBlock() const
{
	auto restString = it.getRestString();
	return restString.startsWith("```");
}

bool MarkdownParser::isImageLink() const
{
	auto restString = it.getRestString();
	return restString.startsWith("![");
}

void MarkdownParser::addCharacterToCurrentBlock(juce_wchar c)
{
	currentlyParsedBlock.append(String::charToString(c), currentFont, currentColour);
}

void MarkdownParser::resetCurrentBlock()
{
	currentlyParsedBlock = AttributedString();
	currentlyParsedBlock.setLineSpacing(8.0f);
	currentLinks.clearQuick();
}

void MarkdownParser::skipTagAndTrailingSpace()
{
	if (it.peek() == 0)
		return;

	it.advance();

	if (it.peek() == ' ')
		it.advance();
}

void MarkdownParser::parseComment()
{
	resetForNewLine();

	skipTagAndTrailingSpace();

	parseText();

	elements.add(new Comment(this, currentlyParsedBlock));


}

const MarkdownLayout& MarkdownParser::getTextLayoutForString(const AttributedString& s, float width)
{
	if (layoutCache.get() != nullptr)
		return layoutCache->getLayout(s, width);

	uncachedLayout = { s, width };
	return  uncachedLayout;
}


}

