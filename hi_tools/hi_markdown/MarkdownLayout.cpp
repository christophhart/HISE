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
	constexpr float marginBetweenAttributes = 5.0f;

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

			for (int i = 0; i < textLen; ++i)
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

				auto deltaX = xOffsets.getUnchecked(i + 1) - xOffsets.getUnchecked(i);

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

				auto thisX = xOffsets.getUnchecked(i);
				bool isWhitespace = t.isWhitespace();

				if (*t == '\n' && (thisString.getCharPointer() + thisString.length() - 1 == t))
					break;

				glyphs.add(PositionedGlyph(a.font, t.getAndAdvance(),
					newGlyphs.getUnchecked(i),
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
	linkBackgroundColour = JUCE_LIVE_CONSTANT_OFF(Colour(0x228888FF));
	codebackgroundColour = JUCE_LIVE_CONSTANT_OFF(Colour(0x33888888));
	codeColour = JUCE_LIVE_CONSTANT_OFF(Colour(0xffffffff));

	f = GLOBAL_FONT();
	fontSize = 17.0f;
}

}