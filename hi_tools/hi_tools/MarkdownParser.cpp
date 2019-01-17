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
	currentColour = textColour.withAlpha(0.8f);

	parseText();

	while (!Helpers::isNewElement(it.peek()))
	{
		parseText();
	}

	elements.add(new TextBlock(this, currentlyParsedBlock));
}

void MarkdownParser::resetForNewLine()
{
	currentFont = normalFont.withHeight(defaultFontSize);
	currentFont.setBold(false);
	currentColour = textColour;
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

	currentFont = headlineFont.withHeight(defaultFontSize + 5 * headlineLevel);
	currentFont.setBold(true);

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


	currentFont = normalFont.withHeight(defaultFontSize);

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
						currentFont = boldFont.withHeight(size);
					else
						currentFont = normalFont.withHeight(size);
				}
				else
				{
					isItalic = !isItalic;
					currentFont.setItalic(isItalic);
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

			currentFont = isCode ? codeFont : normalFont;

			currentColour = isCode ? textColour : textColour.withAlpha(0.8f);

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

			if (ok)
			{
				auto text = currentlyParsedBlock.getText();

				auto numReturns = StringArray::fromLines(text).size() - 1;

				auto start = text.length();

				currentFont.setUnderline(true);
				currentlyParsedBlock.append(urlId, currentFont, Colour(SIGNAL_COLOUR));
				currentFont.setUnderline(false);

				auto stop = start + urlId.length();

				HyperLink hyperLink;

				hyperLink.url = url;
				hyperLink.urlRange = { start, stop };
				hyperLink.valid = true;

				currentLinks.add(std::move(hyperLink));
			}
			else
				throw String("Error at parsing URL link");

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
			currentlyParsedBlock.setFont(boldFont.withHeight(defaultFontSize));
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

const juce::TextLayout& MarkdownParser::getTextLayoutForString(const AttributedString& s, float width)
{
	if (layoutCache != nullptr)
		return layoutCache->getLayout(s, width);

	uncachedLayout = TextLayout();
	uncachedLayout.createLayoutWithBalancedLineLengths(s, width);

	return uncachedLayout;
}


}

