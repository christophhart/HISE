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

#pragma once

namespace hise {
using namespace juce;



struct MarkdownLayout
{
	MarkdownLayout(const AttributedString& s, float width, bool allInOne=false);

	struct StyleData
	{
		StyleData();

		Font f;
		float fontSize;
		Colour codebackgroundColour;
		Colour linkBackgroundColour;
		Colour textColour;
		Colour codeColour;
		Colour linkColour;
		Colour headlineColour;
		Colour backgroundColour;

		Font getFont() const { return f.withHeight(fontSize); }
	};



	void addYOffset(float delta);

	void addXOffset(float delta);

	void draw(Graphics& g);

	void drawCopyWithOffset(Graphics& g, Rectangle<float> targetArea) const;

	float getHeight() const;

	StyleData styleData;

	juce::GlyphArrangement normalText;
	juce::GlyphArrangement codeText;
	Array<juce::GlyphArrangement> linkTexts;
	RectangleList<float> codeBoxes;
	RectangleList<float> hyperlinkRectangles;
	Array<std::tuple<Range<int>, Rectangle<float>>> linkRanges;
};


}