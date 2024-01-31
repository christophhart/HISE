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

namespace multipage {
using namespace juce;

void Dialog::LookAndFeelMethods::drawMultiPageHeader(Graphics& g, Dialog& d, Rectangle<int> area)
{
    var pd;
    
    if(auto pi = d.pages[d.runThread->currentPageIndex])
        pd = pi->getData();
    
	auto pos = getMultiPagePositionInfo(pd);


	auto f = d.styleData.getBoldFont().withHeight(d.styleData.fontSize * 1.7f);

	if(!d.isEditModeEnabled() && d.pages.size() > 1)
	{
		auto progress = area.removeFromBottom(2);

		g.setColour(d.styleData.textColour.withAlpha(0.3f));
		g.fillRect(progress);

		auto normProgress = (float)(d.runThread->currentPageIndex+1) / jmax(1.0f, (float)d.pages.size());

		g.setColour(d.styleData.textColour.withAlpha(0.6f));
		g.fillRect(progress.removeFromLeft(normProgress * (float)area.getWidth()));
	}

	area.removeFromBottom(3);

	g.setFont(f);
	g.setColour(d.styleData.headlineColour);
	g.drawText(d.properties[mpid::Header], area.toFloat(), Justification::topLeft);

	g.setFont(d.styleData.getFont());
	g.setColour(d.styleData.textColour);

	auto sub = d.properties[mpid::Subtitle];

	if(sub == "{PAGE_TEXT}" && d.currentPage != nullptr)
	{
		sub = d.currentPage->getPropertyFromInfoObject(mpid::Text).toString();
	}

	g.setColour(d.styleData.textColour.withAlpha(0.4f));

	area.removeFromBottom(5.0f);

	g.drawText(sub, area.toFloat(), Justification::bottomLeft);

	if(d.pages.size() > 1 || d.isEditModeEnabled())
	{
		String pt;
		pt << "Step " << String(d.runThread->currentPageIndex+1) << " / " << String(d.pages.size());

		g.drawText(pt, area.toFloat(), Justification::bottomRight);
	}
}

void Dialog::LookAndFeelMethods::drawMultiPageButtonTab(Graphics& g, Dialog& d, Rectangle<int> area)
{
	g.setColour(d.findColour(ColourIds::backgroundColour));
	g.fillRect(area);
}

void Dialog::LookAndFeelMethods::drawMultiPageModalBackground(Graphics& g, Rectangle<int> totalBounds,
	Rectangle<int> modalBounds)
{
	g.setColour(Colour(0xFF222222).withAlpha(0.9f));
	g.fillRect(totalBounds);
            
	DropShadow sh(Colours::black.withAlpha(0.7f), 30, { 0, 0 });
	sh.drawForRectangle(g, modalBounds);

	g.setColour(Colour(0xFF333333));
	g.fillRoundedRectangle(modalBounds.toFloat(), 3.0f);
	g.setColour(Colour(0xFFAAAAAA));
	g.drawRoundedRectangle(modalBounds.toFloat(), 3.0f, 1.0f);
}

void Dialog::LookAndFeelMethods::drawMultiPageFoldHeader(Graphics& g, Component& c, Rectangle<float> area,
                                                         const String& title, bool folded)
{
	auto f = Dialog::getDefaultFont(c);
            
	g.setColour(Colours::black.withAlpha(0.2f));
            
	Path bg;
	bg.addRoundedRectangle(area.getX(), area.getY(), area.getWidth(), area.getHeight(), 8.0f, 8.0f, true, true, folded, folded);
            
	g.fillPath(bg);
            
	Path p;
	p.addTriangle({0.5, 0.0}, {1.0, 1.0}, {0.0, 1.0});
            
	p.applyTransform(AffineTransform::rotation(folded ? float_Pi * 0.5f : float_Pi));
            
	g.setFont(f.first.boldened());
	g.setColour(f.second.withAlpha(folded ? 0.7f : 1.0f));
            
	PathFactory::scalePath(p, area.removeFromLeft(area.getHeight()).reduced(10));
            
	g.fillPath(p);
            
	g.drawText(title, area, Justification::centredLeft);
}

void Dialog::LookAndFeelMethods::drawMultiPageBackground(Graphics& g, Dialog& tb, Rectangle<int> errorBounds)
{
	g.fillAll(tb.findColour(backgroundColour));
            
	if(!errorBounds.isEmpty())
	{
		g.setColour(Colour(HISE_ERROR_COLOUR).withAlpha(0.15f));
		auto eb = errorBounds.toFloat().expanded(-1.0f, 3.0f);
		g.fillRoundedRectangle(eb, 3.0f);
		g.setColour(Colour(HISE_ERROR_COLOUR));
		g.drawRoundedRectangle(eb, 3.0f, 2.0f);
	}
}

void Dialog::DefaultLookAndFeel::drawToggleButton(Graphics& g, ToggleButton& tb,
                                                  bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
	auto f = Dialog::getDefaultFont(tb);
	auto c = f.second;
    auto b = tb.getLocalBounds().toFloat();
    
    auto tickArea = b.removeFromLeft(b.getHeight());
	g.setColour(c.withMultipliedAlpha(shouldDrawButtonAsHighlighted ? 1.0f : 0.7f));

	g.drawRoundedRectangle(tickArea.reduced(8).toFloat(), 4.0f, 2.0f);

	if(tb.getToggleState())
	{
		g.fillRoundedRectangle(tickArea.reduced(11).toFloat(), 3.0f);
	}

	auto text = tb.getButtonText();

	

	if(text.isNotEmpty())
	{
		b.removeFromLeft(3);
		g.setColour(f.second);
		g.setFont(f.first);
		g.drawText(text, b.toFloat(), Justification::left);

		if(auto c = tb.findParentComponentOfClass<factory::LabelledComponent>())
		{
			if(c->getId().isValid())
			{
				g.setColour(f.second.withAlpha(0.5f));
				g.setFont(GLOBAL_MONOSPACE_FONT());
				g.drawText(c->getId().toString(), b.toFloat(), Justification::right);
			}
		}
	}
}

Dialog::PositionInfo Dialog::DefaultLookAndFeel::getMultiPagePositionInfo(const var& pageData) const
{
	return defaultPosition;
}

void Dialog::DefaultLookAndFeel::layoutFilenameComponent(FilenameComponent& filenameComp,
	ComboBox* filenameBox, Button* browseButton)
{
	if (browseButton == nullptr || filenameBox == nullptr)
		return;

	auto b = filenameComp.getLocalBounds();
            
	browseButton->setBounds(b.removeFromRight(100));
    b.removeFromRight(getMultiPagePositionInfo({}).OuterPadding);

	filenameBox->setBounds(b);
}

void Dialog::DefaultLookAndFeel::drawProgressBar(Graphics& g, ProgressBar& pb, int width, int height, double progress,
	const String& textToShow)
{
            
	auto f = getDefaultFont(pb);
            
	Rectangle<float> area = pb.getLocalBounds().toFloat();
            
	g.setColour(f.second);
            
	area = area.reduced(1.0f);
            
	g.drawRoundedRectangle(area, area.getHeight() / 2, 1.0f);
            
	area = area.reduced(3.0f);
            
	auto copy = area.reduced(2.0f);
            
	area = area.removeFromLeft(jmax<float>(area.getHeight(), area.getWidth() * progress));
            
	g.fillRoundedRectangle(area, area.getHeight() * 0.5f);
            
	g.setColour(f.second.contrasting().withAlpha(progress > 0.5f ? 0.6f : 0.2f));
	g.fillRoundedRectangle(copy.withSizeKeepingCentre(copy.getHeight() + f.first.getStringWidthFloat(textToShow), copy.getHeight()), copy.getHeight() * 0.5f);
            
	g.setColour(f.second);
	g.setFont(f.first);
	g.drawText(textToShow, copy, Justification::centred);
}


}
}