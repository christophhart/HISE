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

MarkdownLayout::MarkdownLayout(const AttributedString& s, float width, bool allInOne)
{
	constexpr float marginBetweenAttributes = 2.0f;

	if (width == 0.0f)
		return;

	float currentX = 0.0f;
	float yPos = 0.0f;
	bool allowedToWrap = false;

	auto completeText = s.getText();

	for (int i = 0; i < s.getNumAttributes(); i++)
	{
		currentX += marginBetweenAttributes;

		const auto& a = s.getAttribute(i);

		auto r = s.getAttribute(i).range;
		auto thisString = completeText.substring(r.getStart(), r.getEnd());

		Array<juce::PositionedGlyph> glyphs;

		if (thisString.isNotEmpty())
		{
			Array<int> newGlyphs;
			Array<float> xOffsets;
			a.font.getGlyphPositions(thisString, newGlyphs, xOffsets);
			auto textLen = newGlyphs.size();
			glyphs.ensureStorageAllocated(glyphs.size() + textLen);

			auto t = thisString.getCharPointer();

			for (int j = 0; j < textLen; ++j)
			{
				auto getXDeltaForWordEnd = [](String::CharPointerType copy, Font f)
				{
					auto end = copy;

					while (*end != 0 && !CharacterFunctions::isWhitespace(*end))
						end++;

					String word(copy, end);

					return f.getStringWidthFloat(word);
				};

				if (CharacterFunctions::isWhitespace(*t))
					allowedToWrap = true;

				auto deltaX = xOffsets.getUnchecked(j + 1) - xOffsets.getUnchecked(j);

				auto wordEndX = currentX + getXDeltaForWordEnd(t, a.font);

				const bool isNewLine = *t == '\n';

				if (allowedToWrap && ((wordEndX > width + 1.0f) || isNewLine))
				{
					yPos += a.font.getHeight() * 1.5f;
					currentX = marginBetweenAttributes;
					allowedToWrap = false;

					if (isNewLine)
					{
						t++;
						continue;
					}
				}

				bool isWhitespace = t.isWhitespace();

				if (*t == '\n' && (thisString.getCharPointer() + thisString.length() - 1 == t))
					break;

				glyphs.add(PositionedGlyph(a.font, t.getAndAdvance(),
					newGlyphs.getUnchecked(j),
					currentX,
					yPos,
					deltaX, isWhitespace));

				currentX += deltaX;
			}
		}

		const bool isLink = a.font.isUnderlined();
		const bool isCode = a.font.getTypefaceName() == GLOBAL_MONOSPACE_FONT().getTypefaceName();

		GlyphArrangement newLink;

		auto& aToUse = allInOne ? normalText : (isLink ? newLink : (isCode ? codeText : normalText));

		RectangleList<float> linkRectangles;

		for (const auto& g : glyphs)
		{
			aToUse.addGlyph(g);

			if (isLink)
				linkRectangles.add(g.getBounds().expanded(marginBetweenAttributes, 0.0f));

			if (isCode)
				codeBoxes.add(g.getBounds().expanded(marginBetweenAttributes, 0.0f));
		}

		if (isLink)
		{
			linkRectangles.consolidate();
			hyperlinkRectangles.add(linkRectangles.getBounds());

			std::tuple<Range<int>, Rectangle<float>> newItem(a.range, linkRectangles.getBounds().toFloat());

			linkRanges.add(newItem);
			linkTexts.add(newLink);
		}

		//currentX = glyphs.getLast().getRight() + 3.0f;
	}

	codeBoxes.consolidate();
	hyperlinkRectangles.consolidate();
}

void MarkdownLayout::addYOffset(float delta)
{
	normalText.moveRangeOfGlyphs(0, -1, 0.0f, delta);

	for (auto& lt : linkTexts)
		lt.moveRangeOfGlyphs(0, -1, 0.0f, delta);

	codeText.moveRangeOfGlyphs(0, -1, 0.0f, delta);
	codeBoxes.offsetAll(0.0f, delta);
	hyperlinkRectangles.offsetAll(0.0f, delta);

	for (auto& linkArea : linkRanges)
		std::get<1>(linkArea).translate(0.0f, delta);
}

void MarkdownLayout::addXOffset(float delta)
{
	normalText.moveRangeOfGlyphs(0, -1, delta, 0.0f);

	for (auto& lt : linkTexts)
		lt.moveRangeOfGlyphs(0, -1, delta, 0.0f);

	codeText.moveRangeOfGlyphs(0, -1, delta, 0.0f);
	codeBoxes.offsetAll(delta, 0.0f);
	hyperlinkRectangles.offsetAll(delta, 0.0f);

	for (auto& linkArea : linkRanges)
		std::get<1>(linkArea).translate(delta, 0.0f);
}

void MarkdownLayout::draw(Graphics& g)
{
	g.setColour(styleData.codebackgroundColour);
	for (auto r : codeBoxes)
		g.fillRoundedRectangle(r, 2.0f);

	g.setColour(styleData.linkBackgroundColour);

	for (auto r : hyperlinkRectangles)
		g.fillRoundedRectangle(r, 2.0f);

	g.setColour(styleData.textColour);

	normalText.draw(g);

	g.setColour(styleData.codeColour);

	codeText.draw(g);

	g.setColour(styleData.linkColour);

	for (auto& lt : linkTexts)
		lt.draw(g);
}

void MarkdownLayout::drawCopyWithOffset(Graphics& g, Rectangle<float> targetArea) const
{
	auto copy = MarkdownLayout(*this);
	copy.addYOffset(targetArea.getY());
	copy.addXOffset(targetArea.getX());
	copy.draw(g);
}

float MarkdownLayout::getHeight() const
{
	float t2 = 0.0f; // Nice one...
	float b2 = 0.0f;

	for (auto lt : linkTexts)
	{
		t2 = jmin<float>(lt.getBoundingBox(0, -1, true).getY(), t2);
		b2 = jmax<float>(lt.getBoundingBox(0, -1, true).getBottom(), b2);
	}



	auto t1 = normalText.getBoundingBox(0, -1, true).getY();
	auto t3 = codeText.getBoundingBox(0, -1, true).getY();

	auto b1 = normalText.getBoundingBox(0, -1, true).getBottom();
	auto b3 = codeText.getBoundingBox(0, -1, true).getBottom();

	return jmax<float>(b1, b2, b3) - jmin<float>(t1, t2, t3);
}

MarkdownLayout::StyleData::StyleData()
{
	backgroundColour = Colour(0xFF333333);
	linkColour = JUCE_LIVE_CONSTANT_OFF(Colour(0xFFAAAAFF));
	textColour = Colours::white;
	headlineColour = Colour(SIGNAL_COLOUR);
	linkBackgroundColour = JUCE_LIVE_CONSTANT_OFF(Colour(0x008888FF));
	codebackgroundColour = JUCE_LIVE_CONSTANT_OFF(Colour(0x33888888));
	codeColour = JUCE_LIVE_CONSTANT_OFF(Colour(0xffffffff));
	tableHeaderBackgroundColour = Colours::grey.withAlpha(0.2f);
	tableBgColour = Colours::grey.withAlpha(0.2f);
	tableLineColour = Colours::grey.withAlpha(0.2f);

	f = GLOBAL_FONT();
	fontSize = 18.0f;
}

MarkdownLayout::StyleData MarkdownLayout::StyleData::createBrightStyle()
{
	MarkdownLayout::StyleData l;
	l.textColour = Colour(0xFF333333);
	l.headlineColour = Colour(0xFF444444);
	l.backgroundColour = Colour(0xFFEEEEEE);
	l.linkColour = Colour(0xFF000044);
	l.codeColour = Colour(0xFF333333);
	l.tableHeaderBackgroundColour = Colours::grey.withAlpha(0.2f);
	l.tableLineColour = Colours::grey.withAlpha(0.2f);

	return l;
}

bool MarkdownLayout::StyleData::fromDynamicObject(var obj, const std::function<Font(const String&)>& fontLoader)
{
	auto fName = obj.getProperty(MarkdownStyleIds::Font, f.getTypefaceName());
	auto bName = obj.getProperty(MarkdownStyleIds::BoldFont, getBoldFont().getTypefaceName());
	useSpecialBoldFont = obj.getProperty(MarkdownStyleIds::UseSpecialBoldFont, useSpecialBoldFont);
	fontSize = obj.getProperty(MarkdownStyleIds::FontSize, fontSize);

	f = fontLoader(fName);
	boldFont = fontLoader(bName);

	auto getColourFromVar = [&](const Identifier& id, Colour defaultColour)
	{
		auto v = (int64)obj.getProperty(id, (int64)defaultColour.getARGB());
		return Colour((uint32)v);
	};

#define GET_COLOUR(x, id) x = getColourFromVar(id, x);
	GET_COLOUR(codebackgroundColour, MarkdownStyleIds::codeBgColour);
	GET_COLOUR(linkBackgroundColour, MarkdownStyleIds::linkBgColour);
	GET_COLOUR(textColour, MarkdownStyleIds::textColour);
	GET_COLOUR(codeColour, MarkdownStyleIds::codeColour);
	GET_COLOUR(linkColour, MarkdownStyleIds::linkColour);
	GET_COLOUR(headlineColour, MarkdownStyleIds::headlineColour);
	GET_COLOUR(backgroundColour, MarkdownStyleIds::bgColour);
	GET_COLOUR(tableBgColour, MarkdownStyleIds::tableBgColour);
	GET_COLOUR(tableHeaderBackgroundColour, MarkdownStyleIds::tableHeaderBgColour);
	GET_COLOUR(tableLineColour, MarkdownStyleIds::tableLineColour);
#undef GET_COLOUR

	return true;
}

juce::var MarkdownLayout::StyleData::toDynamicObject() const
{
	DynamicObject::Ptr obj = new DynamicObject();

	obj->setProperty(MarkdownStyleIds::Font, f.getTypefaceName());
	obj->setProperty(MarkdownStyleIds::BoldFont, boldFont.getTypefaceName());
	obj->setProperty(MarkdownStyleIds::FontSize, fontSize);
	obj->setProperty(MarkdownStyleIds::bgColour, backgroundColour.getARGB());
	obj->setProperty(MarkdownStyleIds::codeBgColour, codebackgroundColour.getARGB());
	obj->setProperty(MarkdownStyleIds::linkBgColour, linkBackgroundColour.getARGB());
	obj->setProperty(MarkdownStyleIds::textColour, textColour.getARGB());
	obj->setProperty(MarkdownStyleIds::codeColour, codeColour.getARGB());
	obj->setProperty(MarkdownStyleIds::linkColour, linkColour.getARGB());
	obj->setProperty(MarkdownStyleIds::tableHeaderBgColour, tableHeaderBackgroundColour.getARGB());
	obj->setProperty(MarkdownStyleIds::tableLineColour, tableLineColour.getARGB());
	obj->setProperty(MarkdownStyleIds::tableBgColour, tableBgColour.getARGB());
	obj->setProperty(MarkdownStyleIds::headlineColour, headlineColour.getARGB());
	obj->setProperty(MarkdownStyleIds::UseSpecialBoldFont, useSpecialBoldFont);

	return var(obj.get());
}

}