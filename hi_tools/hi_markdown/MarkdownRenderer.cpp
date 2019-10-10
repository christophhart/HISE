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

float MarkdownRenderer::getHeightForWidth(float width, bool forceUpdate/*=false*/)
{
	if (width == lastWidth && !forceUpdate)
		return lastHeight;

	float height = 0.0f;

	for (auto* e : elements)
	{
		if (auto h = dynamic_cast<MarkdownParser::Headline*>(e))
		{
			h->anchorY = height;
		}

		height += e->getTopMargin();
		height += e->getHeightForWidthCached(width, forceUpdate);
	}

	lastWidth = width;
	lastHeight = height;
	firstDraw = true;

	return height;
}

void MarkdownRenderer::parse()
{
	lastWidth = -1.0f;
	firstDraw = true;

	MarkdownParser::parse();

	for (auto l : listeners)
	{
		if (l.get() != nullptr)
			l->markdownWasParsed(getParseResult());
	}
}

void MarkdownRenderer::jumpToCurrentAnchor()
{
	if (lastWidth == -1.0)
		return;

	auto thisAnchor = getLastLink().toString(MarkdownLink::AnchorWithHashtag);

	if (thisAnchor.isEmpty())
	{
		scrollToY(0.0f);
		return;
	}
	
	getHeightForWidth(lastWidth, true);

	for (auto e : elements)
	{
		if (auto headLine = dynamic_cast<Headline*>(e))
		{
			if (thisAnchor == headLine->anchorURL)
			{
				scrollToY(headLine->anchorY);
			}

		}
	}
}

juce::String MarkdownRenderer::getAnchorForY(int y) const
{
	int thisY = 0;

	Headline* lastHeadline = nullptr;

	for (auto e : elements)
	{
		if (auto h = dynamic_cast<Headline*>(e))
		{
			lastHeadline = h;
		}

		thisY += e->getTopMargin();
		thisY += (int)e->getLastHeight();

		if (y <= thisY)
			break;
	}

	if (lastHeadline != nullptr && lastHeadline != elements.getFirst())
		return lastHeadline->anchorURL;

	return {};
}

}