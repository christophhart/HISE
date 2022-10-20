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

#if !HISE_NO_GUI_TOOLS
#include "../JuceLibraryCode/BinaryData.h"
#endif

namespace hise { using namespace juce;

void FilmstripLookAndFeel::drawToggleButton(Graphics &g, ToggleButton &b, bool isMouseOverButton, bool isButtonDown)
{
    SET_IMAGE_RESAMPLING_QUALITY();
    
	if (!imageToUse.isValid() || (numStrips != 2 && numStrips != 6))
	{
		GlobalHiseLookAndFeel::drawToggleButton(g, b, isMouseOverButton, isButtonDown);
		return;
	}
	else
	{
		int stripIndex = 0;

		if (numStrips == 2)
		{
			stripIndex = b.getToggleState() ? 1 : 0;

		}
		else if (numStrips == 6)
		{
			const bool on = b.getToggleState();
			const bool hover = isMouseOverButton;
			const bool pressed = isButtonDown;

			if (hover)
				stripIndex = 4;

			if (pressed)
				stripIndex = 2;

			if (on)
				stripIndex += 1;
		}

		Image clip;

		if (isVertical)
		{
			const int offset = stripIndex * heightOfEachStrip;
			clip = imageToUse.getClippedImage(Rectangle<int>(0, offset, widthOfEachStrip, heightOfEachStrip));
		}
		else
		{
			const int offset = stripIndex * widthOfEachStrip;
			clip = imageToUse.getClippedImage(Rectangle<int>(offset, 0, widthOfEachStrip, heightOfEachStrip));
		}

		g.setColour(Colours::black.withAlpha(b.isEnabled() ? 1.0f : 0.5f));



		g.drawImage(clip, 0, 0, (int)((float)widthOfEachStrip * scaleFactor), (int)((float)heightOfEachStrip * scaleFactor), 0, 0, widthOfEachStrip, heightOfEachStrip);
		
		
	}
}

void FilmstripLookAndFeel::drawRotarySlider(Graphics &g, int /*x*/, int /*y*/, int width, int height, float /*sliderPosProportional*/, float /*rotaryStartAngle*/, float /*rotaryEndAngle*/, Slider &s)
{
    SET_IMAGE_RESAMPLING_QUALITY();
    
	if (!imageToUse.isValid() || numStrips == 0)
	{
		GlobalHiseLookAndFeel::drawRotarySlider(g, -1, -1, width, height, -1, -1, -1, s);
		return;
	}
	else
	{
		const double value = s.getValue();
		const double normalizedValue = (value - s.getMinimum()) / (s.getMaximum() - s.getMinimum());
		const double proportion = pow(normalizedValue, s.getSkewFactor());
		const int stripIndex = (int)(proportion * (numStrips-1));

		Image clip;

		if (isVertical)
		{
			const int offset = stripIndex * heightOfEachStrip;
			clip = imageToUse.getClippedImage(Rectangle<int>(0, offset, widthOfEachStrip, heightOfEachStrip));
		}
		else
		{
			const int offset = stripIndex * widthOfEachStrip;
			clip = imageToUse.getClippedImage(Rectangle<int>(offset, 0, widthOfEachStrip, heightOfEachStrip));
		}

		g.setColour(Colours::black.withAlpha(s.isEnabled() ? 1.0f : 0.5f));

        g.drawImage(clip, 0, 0, (int)((float)widthOfEachStrip * scaleFactor), (int)((float)heightOfEachStrip * scaleFactor), 0, 0, widthOfEachStrip, heightOfEachStrip);
	}
}


void VUSliderLookAndFeel::drawLinearSlider(Graphics &g, 
										   int x, 
										   int y, 
										   int width, 
										   int height, 
										   float sliderPos, 
										   float minSliderPos,
										   float maxSliderPos, 
										   const Slider::SliderStyle style, 
										   Slider &slider)
{
	g.fillAll (slider.findColour (Slider::backgroundColourId));

	if (style == Slider::LinearBar || style == Slider::LinearBarVertical)
	{
		const float fx = (float) x, fy = (float) y, fw = (float) width, fh = (float) height;
		Path p;

		if (style == Slider::LinearBarVertical)
			p.addRectangle (fx, sliderPos, fw, 1.0f + fh - sliderPos);
		else
			p.addRectangle (fx, fy, sliderPos - fx, fh);

		Colour baseColour (slider.findColour (Slider::thumbColourId)
								.withMultipliedSaturation (slider.isEnabled() ? 1.0f : 0.5f)
								.withMultipliedAlpha (0.8f));

		g.setGradientFill (ColourGradient (baseColour.brighter (0.08f), 0.0f, 0.0f,
											baseColour.darker (0.08f), 0.0f, (float) height, false));


		g.setGradientFill (ColourGradient (Colour (0x55ffa000),
                                    0.0f, 0.0f,
									Colours::green.withMultipliedAlpha(0.8f),
                                    (float)width, 0.0f,
                                    false));
		g.fillRect (367, 171, 225, 21);
		g.fillPath (p);
		g.setColour (baseColour.darker (0.2f));

		if (style == Slider::LinearBarVertical)
			g.fillRect (fx, sliderPos, fw, 1.0f);
		else
			g.fillRect (sliderPos, fy, 1.0f, fh);

		g.setColour(Colours::white.withAlpha(0.2f));
		for(float i = 0; i < width; i += 3)
		{
			g.drawLine((float)i, 0.0f, (float)i, (float)height, 0.5f);
		};

	}
	else
	{
		drawLinearSliderBackground (g, x, y, width, height, sliderPos, minSliderPos, maxSliderPos, style, slider);
		drawLinearSliderThumb (g, x, y, width, height, sliderPos, minSliderPos, maxSliderPos, style, slider);
	}
};




void GlobalHiseLookAndFeel::drawRotarySlider(Graphics &g, int /*x*/, int /*y*/, int width, int height, float /*sliderPosProportional*/, float /*rotaryStartAngle*/, float /*rotaryEndAngle*/, Slider &s)
{
	s.setTextBoxStyle (Slider::TextBoxRight, false, 80, 28);

			if(!s.isEnabled()) g.setOpacity(0.4f);


	height = s.getHeight();
	width = s.getWidth();

	drawHiBackground(g, 12, 10, width-12, 30, &s);
		
	const double value = s.getValue();
    const double normalizedValue = (value - s.getMinimum()) / (s.getMaximum() - s.getMinimum());
	const double proportion = pow(normalizedValue, s.getSkewFactor());

	auto area = s.getLocalBounds().toFloat();
	area = area.removeFromLeft(area.getHeight());

	auto bipolar = -1.0 * s.getMinimum() == s.getMaximum();

	drawVectorRotaryKnob(g, area.translated(0.0, 2.0f), proportion, bipolar, s.isMouseOverOrDragging(true), s.isMouseButtonDown(), s.isEnabled(), proportion);

#if OLD_FILMSTRIP
	const int stripIndex = (int) (proportion * 127);
	const int filmstripHeight = cachedImage_smalliKnob_png.getHeight() / 128;
    const int offset = stripIndex * filmstripHeight;

	Image clip = cachedImage_smalliKnob_png.getClippedImage(Rectangle<int>(0, offset, filmstripHeight, filmstripHeight));

    g.setColour (Colours::black.withAlpha(s.isEnabled() ? 1.0f : 0.5f));
    g.drawImage (clip, 0, 3, 48, 48, 0, 0, filmstripHeight, filmstripHeight);

	float displayValue = 1.0f;

	Image *imageToUse = &cachedImage_knobRing_png;

	Image clipRing = imageToUse->getClippedImage(Rectangle<int>(0, (int)(stripIndex * displayValue) * filmstripHeight, filmstripHeight, filmstripHeight));
	
    g.setColour (Colours::black.withAlpha(s.isEnabled() ? 1.0f : 0.5f));
	g.drawImage(clipRing, 0, 3, 48, 48, 0, 0, filmstripHeight, filmstripHeight);
#endif

	g.setColour(Colours::white.withAlpha(0.7f));
	g.setFont (GLOBAL_BOLD_FONT());
	g.drawText (s.getName(),
				45 , 13, (int)((0.5391f) * width) + 10, 12,
				Justification::centred, true);

#if OLD_FILMSTRIP
	if(s.getComponentEffect() != nullptr)
	{
		s.getComponentEffect()->applyEffect(clip, g, 1.0f, 1.0f);
	}
#endif
}


void GlobalHiseLookAndFeel::drawToggleButton (Graphics &g, ToggleButton &b, bool isMouseOverButton, bool /*isButtonDown*/)
{
	drawHiBackground(g, 0, 0, b.getWidth(), b.getHeight() - 2, &b, isMouseOverButton);

	const int filmStripHeight = cachedImage_toggle_png.getHeight() / 2;

	Image clip = Image(cachedImage_toggle_png.getClippedImage(Rectangle<int>(0, b.getToggleState() ? filmStripHeight: 0, filmStripHeight, filmStripHeight)));

	g.setColour (b.getToggleState() ? Colours::white.withAlpha(0.9f) : Colours::white.withAlpha(0.4f));
	g.setFont (GLOBAL_FONT());

	String text = b.getButtonText();

	Rectangle<int> textArea(30, 6, b.getWidth() - 36, b.getHeight() - 12);

	if (textArea.getHeight() > 0 && textArea.getWidth() > 0)
	{
		g.drawText(text, textArea, Justification::centredLeft, true);
	}
	
	g.setColour (Colours::black.withAlpha( (b.isEnabled() ? 1.0f : 0.5f) ));
	g.drawImage(clip, 7, (b.getHeight() - 16) / 2, 16, 16, 0, 0, filmStripHeight, filmStripHeight);
}

#ifndef INCLUDE_STOCK_FILMSTRIPS
#if HISE_IOS
#define INCLUDE_STOCK_FILMSTRIPS 0
#else
#define INCLUDE_STOCK_FILMSTRIPS 1
#endif
#endif

GlobalHiseLookAndFeel::GlobalHiseLookAndFeel()
{
	static const uint8 ringData[] = { 110, 109, 203, 161, 243, 65, 12, 2, 240, 65, 98, 96, 229, 189, 65, 90, 228, 19, 66, 152, 110, 74, 65, 94, 58, 21, 66, 180, 200, 178, 64, 166, 155, 245, 65, 98, 6, 129, 197, 191, 162, 69, 192, 65, 33, 176, 242, 191, 121, 233, 76, 65, 154, 153, 153, 64, 215, 163, 180, 64, 98, 158, 239, 55, 65, 74, 12, 194, 191, 168, 198,
		181, 65, 6, 129, 245, 191, 229, 208, 238, 65, 207, 247, 151, 64, 98, 152, 238, 19, 66, 240, 167, 54, 65, 16, 88, 21, 66, 221, 36, 181, 65, 184, 30, 245, 65, 164, 112, 238, 65, 98, 0, 0, 245, 65, 104, 145, 238, 65, 72, 225, 244, 65, 33, 176, 238, 65, 156, 196, 244, 65, 217, 206, 238, 65, 108, 180, 200, 233, 65, 227, 165,
		185, 65, 98, 92, 143, 252, 65, 250, 126, 146, 65, 109, 231, 244, 65, 236, 81, 68, 65, 215, 163, 211, 65, 172, 28, 6, 65, 98, 104, 145, 170, 65, 125, 63, 101, 64, 242, 210, 83, 65, 119, 190, 119, 64, 33, 176, 6, 65, 176, 114, 16, 65, 98, 63, 53, 102, 64, 170, 241, 98, 65, 201, 118, 118, 64, 209, 34, 178, 65, 143, 194, 15,
		65, 68, 139, 216, 65, 98, 102, 102, 80, 65, 156, 196, 246, 65, 209, 34, 151, 65, 156, 196, 251, 65, 201, 118, 188, 65, 139, 108, 232, 65, 108, 203, 161, 243, 65, 12, 2, 240, 65, 99, 101, 0, 0 };

	ring.loadPathFromData(ringData, sizeof(ringData));

	ring2.startNewSubPath(0.5f, 1.0f);
	ring2.addArc(0.0f, 0.0f, 1.0f, 1.0f, -float_Pi * 0.75f - 0.04f, float_Pi * 0.75f + 0.04f, true);

#if INCLUDE_STOCK_FILMSTRIPS
	cachedImage_smalliKnob_png = ImageProvider::getImage(ImageProvider::ImageType::KnobEmpty); // ImageCache::getFromMemory(BinaryData::knob_empty_png, BinaryData::knob_empty_pngSize);
	cachedImage_knobRing_png = ImageProvider::getImage(ImageProvider::ImageType::KnobUnmodulated); // ImageCache::getFromMemory(BinaryData::ring_unmodulated_png, BinaryData::ring_unmodulated_pngSize);
	ring_modulated = ImageProvider::getImage(ImageProvider::ImageType::KnobModulated); //ImageCache::getFromMemory(BinaryData::ring_modulated_png, BinaryData::ring_modulated_pngSize);

	//ring_red = ImageCache::getFromMemory(BinaryData::_1_red_png, BinaryData::_1_red_pngSize);
	//ring_yellow = ImageCache::getFromMemory(BinaryData::_2_yellow_png, BinaryData::_2_yellow_pngSize);
	//ring_blue = ImageCache::getFromMemory(BinaryData::_4_blue_png, BinaryData::_4_blue_pngSize);
	//ring_green = ImageCache::getFromMemory(BinaryData::_3_green_png, BinaryData::_3_green_pngSize);

	cachedImage_toggle_png = ImageProvider::getImage(ImageProvider::ImageType::ToggleButton); // ImageCache::getFromMemory(toggle_png, toggle_pngSize);
	cachedImage_slider_strip2_png = ImageCache::getFromMemory (slider_strip2_png, slider_strip2_pngSize);
	cachedImage_slider2_bipolar_png = ImageCache::getFromMemory (slider2_bipolar_png, slider2_bipolar_pngSize);
#endif
    
    
	setColour(PopupMenu::highlightedBackgroundColourId, Colour(SIGNAL_COLOUR));

	Colour dark(0xFF252525);

	Colour bright(0xFF999999);

	setColour(PopupMenu::ColourIds::backgroundColourId, dark);
	setColour(PopupMenu::ColourIds::textColourId, bright);
	setColour(PopupMenu::ColourIds::highlightedBackgroundColourId, bright);
	setColour(PopupMenu::ColourIds::highlightedTextColourId, dark);

	comboBoxFont = GLOBAL_FONT();
}


void GlobalHiseLookAndFeel::fillPathHiStyle(Graphics &g, const Path &p, int, int, bool drawBorders /*= true*/)
{
	if (drawBorders)
	{
		g.setColour(Colours::lightgrey.withAlpha(0.8f));
		g.strokePath(p, PathStrokeType(1.0f));

		g.setColour(Colours::lightgrey.withAlpha(0.1f));

		g.drawRect(p.getBounds().expanded(1), 1.0f);
	}

	auto pb = p.getBounds();

	g.setGradientFill(ColourGradient(Colour(0x88ffffff),
		pb.getTopLeft(),
		Colour(0x11ffffff),
		pb.getBottomLeft(),
		false));

	g.fillPath(p);

	DropShadow d(Colours::white.withAlpha(drawBorders ? 0.2f : 0.1f), 5, Point<int>());

	d.drawForPath(g, p);
}

void GlobalHiseLookAndFeel::draw1PixelGrid(Graphics& g, Component* c, Rectangle<int> bounds, Colour lineColour)
{
    UnblurryGraphics ug(g, *c, true);

    auto mulAlpha = 1.0f - jlimit(0.0f, 1.0f, (1.0f / 3.0f * ug.getPixelSize()));

    float tenAlpha = JUCE_LIVE_CONSTANT_OFF(0.15f);
    float oneAlpha = JUCE_LIVE_CONSTANT_OFF(.06f);
    
    if (mulAlpha > 0.1f)
    {
        for (int x = 10; x < bounds.getWidth(); x += 10)
        {
            float alpha = (x % 100 == 0) ? tenAlpha : oneAlpha;
            alpha *= mulAlpha;
            g.setColour(lineColour.withAlpha(alpha));
            ug.draw1PxVerticalLine(x, 0.0f, (float)bounds.getHeight());
        }

        for (int y = 10; y < bounds.getHeight(); y += 10)
        {
            float alpha = (y % 100 == 0) ? tenAlpha : oneAlpha;
            alpha *= mulAlpha;
            g.setColour(lineColour.withAlpha(alpha));
            ug.draw1PxHorizontalLine(y, 0.0f, (float)bounds.getWidth());
        }
    }
}

Point<float> GlobalHiseLookAndFeel::paintCable(Graphics& g, Rectangle<float> start, Rectangle<float> end, Colour c, float alpha /*= 1.0f*/, Colour holeColour /*= Colour(0xFFAAAAAA)*/, bool returnMidPoint /*= false*/, bool useHangingCable/*=true*/)
{
	if (start.getCentreY() > end.getCentreY())
		std::swap(start, end);

	if (alpha != 1.0f)
	{
		holeColour = c;
	}

	static const unsigned char pathData[] = { 110,109,233,38,145,63,119,190,39,64,108,0,0,0,0,227,165,251,63,108,0,0,0,0,20,174,39,63,108,174,71,145,63,0,0,0,0,108,174,71,17,64,20,174,39,63,108,174,71,17,64,227,165,251,63,108,115,104,145,63,119,190,39,64,108,115,104,145,63,143,194,245,63,98,55,137,
145,63,143,194,245,63,193,202,145,63,143,194,245,63,133,235,145,63,143,194,245,63,98,164,112,189,63,143,194,245,63,96,229,224,63,152,110,210,63,96,229,224,63,180,200,166,63,98,96,229,224,63,43,135,118,63,164,112,189,63,178,157,47,63,133,235,145,63,178,
157,47,63,98,68,139,76,63,178,157,47,63,84,227,5,63,43,135,118,63,84,227,5,63,180,200,166,63,98,84,227,5,63,14,45,210,63,168,198,75,63,66,96,245,63,233,38,145,63,143,194,245,63,108,233,38,145,63,119,190,39,64,99,101,0,0 };

	Path plug;

	plug.loadPathFromData(pathData, sizeof(pathData));
	PathFactory::scalePath(plug, start.expanded(1.5f));

	g.setColour(Colours::black.withAlpha(alpha));
	g.fillEllipse(start);
	g.setColour(holeColour);

	g.fillPath(plug);

	//g.drawEllipse(start, 2.0f);

	g.setColour(Colours::black.withAlpha(alpha));
	g.fillEllipse(end);
	g.setColour(holeColour);
	PathFactory::scalePath(plug, end.expanded(1.5f));
	g.fillPath(plug);
	//g.drawEllipse(end, 2.0f);

	Path p;

	p.startNewSubPath(start.getCentre());

	if (useHangingCable)
	{
		Point<float> controlPoint(start.getX() + (end.getX() - start.getX()) / 2.0f, end.getY() + 100.0f);
		p.quadraticTo(controlPoint, end.getCentre());
		
	}
	else
	{
		Rectangle<float> cableBounds(start.getCentre(), end.getCentre());

		Line<float> cableAsLine(start.getCentre(), end.getCentre());

		auto mid = cableBounds.getCentre();

		Point<float> c1 = { cableAsLine.getPointAlongLineProportionally(0.2f).getX(), start.getCentreY() };
		Point<float> c2 = { cableAsLine.getPointAlongLineProportionally(0.8f).getX(), end.getCentreY() };

		p.quadraticTo(c1, mid);
		p.quadraticTo(c2, end.getCentre());
	}

	g.setColour(Colours::black.withMultipliedAlpha(alpha));
	g.strokePath(p, PathStrokeType(3.0f, PathStrokeType::curved, PathStrokeType::rounded));
	g.setColour(c.withMultipliedAlpha(alpha));
	g.strokePath(p, PathStrokeType(2.0f, PathStrokeType::curved, PathStrokeType::rounded));

	if (returnMidPoint)
		return p.getPointAlongPath(p.getLength() / 2.0f);

	return {};
}

void GlobalHiseLookAndFeel::setTextEditorColours(TextEditor& ed)
{
	ed.setColour(TextEditor::ColourIds::textColourId, Colours::black);
	ed.setColour(TextEditor::ColourIds::backgroundColourId, Colours::white.withAlpha(0.25f));
	ed.setColour(TextEditor::ColourIds::focusedOutlineColourId, Colour(SIGNAL_COLOUR));
	ed.setColour(Label::ColourIds::outlineWhenEditingColourId, Colour(SIGNAL_COLOUR));
	ed.setColour(TextEditor::ColourIds::outlineColourId, Colours::black.withAlpha(0.8f));
	ed.setColour(TextEditor::ColourIds::highlightColourId, Colour(SIGNAL_COLOUR));
	ed.setFont(GLOBAL_BOLD_FONT());
	ed.setSelectAllWhenFocused(true);
}

void GlobalHiseLookAndFeel::drawVectorRotaryKnob(Graphics& g, Rectangle<float> area, double value, bool bipolar, bool hover, bool down, bool enabled, float modValue)
{
	// Routine

	float displayValue = jlimit(0.0f, 1.0f, (float)(bipolar ? abs(value - 0.5) * 2.0 : value));
	auto ringWidth = area.getWidth() / 16.0f;

	g.setColour(Colour(0x33000000));
	g.fillEllipse(area.reduced(ringWidth));

	g.setGradientFill(ColourGradient(Colour(0xFF666666).withAlpha((float)displayValue * 0.3f + 0.3f + (hover ? 0.2f : 0.0f)), 0.0f, 0.0f,
		Colour(0xFF111111), 0.0, area.getHeight(), false));
	g.fillEllipse(area.reduced(ringWidth * 2.0));

	Path ring3, ring4;
	ring3.startNewSubPath(0.0, 0.0);
	ring3.startNewSubPath(1.0, 1.0);
	ring4.startNewSubPath(0.0, 0.0);
	ring4.startNewSubPath(1.0, 1.0);

	auto start = -double_Pi * 0.75;

	{
		auto s = start;
		auto e1 = start + double_Pi * 1.5 * value;
		auto e2 = start + double_Pi * 1.5 * modValue;

		if (bipolar)
		{
			s = value != 0.5 ? 0.0 : -0.04;
			e1 = value != 0.5 ? e1 : 0.04;
			e2 = value != 0.5 ? e2 : 0.04;
		}

		ring3.addArc(0.0, 0.0, 1.0, 1.0, s, e1, true);
		ring4.addArc(0.0, 0.0, 1.0, 1.0, s, e2, true);
	}

	g.setColour(Colour(0xff111118));
	PathFactory::scalePath(ring2, area.reduced(ringWidth));
	g.strokePath(ring2, PathStrokeType(ringWidth * 2.0));
	auto rColour = (down ? 0xFF9099AA : 0xFF808899);
	g.setColour(Colour(rColour).withAlpha(0.1f));
	PathFactory::scalePath(ring3, area.reduced(ringWidth));
	g.strokePath(ring3, PathStrokeType(ringWidth * (down ? 1.55 : 1.4)));

	auto c = Colour(rColour);


	g.setColour(c.withAlpha(0.5f + displayValue * 0.5f));



	PathFactory::scalePath(ring4, area.reduced(ringWidth));
	g.strokePath(ring4, PathStrokeType(ringWidth * (down ? 1.55 : 1.4)));

	
	if (enabled)
	{
		g.setColour(Colour(hover ? 0xFFb2b2b2 : 0xFFAAAAAA));
		PathFactory::scalePath(ring, area.reduced(ringWidth * (down ? 1.55 : 1.4)));

		Path ringCopy(ring);

		ringCopy.applyTransform(AffineTransform::rotation((1.0 - value) * -1.5 * double_Pi, area.getCentreX(), area.getCentreY()));

		g.fillPath(ringCopy);
	}
	else
	{
		g.setColour(Colour(0xFF888888));
		g.drawEllipse(area.reduced(ringWidth * 2.9), ringWidth * 1.6);
	}
}

const char* GlobalHiseLookAndFeel::smalliKnob_png =  (const char*) HiBinaryData::LookAndFeelBinaryData::knob_mod_bg_png;
const int GlobalHiseLookAndFeel::smalliKnob_pngSize = 16277;

const char* GlobalHiseLookAndFeel::knobRing_png =  (const char*) HiBinaryData::LookAndFeelBinaryData::knob_mod_ring_png;
const int GlobalHiseLookAndFeel::knobRing_size = 17983;

const char* GlobalHiseLookAndFeel::toggle_png = (const char*) HiBinaryData::LookAndFeelBinaryData::resource_Background_toggle_png;
const int GlobalHiseLookAndFeel::toggle_pngSize = 1758;

const char* GlobalHiseLookAndFeel::slider_strip2_png = (const char*) HiBinaryData::LookAndFeelBinaryData::resource_Background_slider_strip2_png;
const int GlobalHiseLookAndFeel::slider_strip2_pngSize = 97907;

const char* GlobalHiseLookAndFeel::slider2_bipolar_png = (const char*) HiBinaryData::LookAndFeelBinaryData::resource_Background_slider2_bipolar_png;
const int GlobalHiseLookAndFeel::slider2_bipolar_pngSize = 87929;

const char* BalanceButtonLookAndFeel::balanceKnob_png = (const char*) HiBinaryData::LookAndFeelBinaryData::resource_Button_balanceKnob_png;
const int BalanceButtonLookAndFeel::balanceKnob_pngSize = 5215;

void MacroKnobLookAndFeel::drawRotarySlider(Graphics &g, int /*x*/, int /*y*/, int /*width*/, int /*height*/, float /*sliderPosProportional*/, float /*rotaryStartAngle*/, float /*rotaryEndAngle*/, Slider &s)
{
	
	float alphaValue = 1.0f;

	if (!s.isEnabled()) alphaValue *= 0.4f;

	const double value = s.getValue();

	if (value == 0.0) alphaValue *= 0.66f;

	const double normalizedValue = (value - s.getMinimum()) / (s.getMaximum() - s.getMinimum());
	const double proportion = pow(normalizedValue, s.getSkewFactor());

    
    
    Rectangle<float> area(0.0f, 0.0f, 48.0f, 48.0f);
    
    
    
    Path p1, p2;
    
    float start = JUCE_LIVE_CONSTANT_OFF(-2.4f);
    
    float end = -1.0f * start;
    
    auto length = -2.0f * start;
    
    
    
    p1.addPieSegment(area, start, end, JUCE_LIVE_CONSTANT_OFF(0.75));
    
    p2.addPieSegment(area, start, start + proportion * length, JUCE_LIVE_CONSTANT_OFF(0.75));
    
    float bgAlpha = 0.1f;
    float fAlpha = 0.7f;
    
    if(s.isEnabled())
    {
        bgAlpha += 0.1f;
    
        if(s.isMouseOver(true))
        {
            fAlpha += 0.1f;
            bgAlpha += 0.1f;
        }
        
        if(s.isMouseButtonDown(true))
        {
            bgAlpha += 0.1f;
            fAlpha += 0.1f;
        }
    }
    
    g.setColour(Colours::white.withAlpha(bgAlpha));
    g.fillPath(p1);
    
    g.setColour(Colour(SIGNAL_COLOUR).withAlpha(fAlpha));
    g.fillPath(p2);

#if 0
	const int stripIndex = (int)(proportion * 127);

	const int filmstripHeight = cachedImage_macroKnob_png.getHeight() / 128;

	const int offset = stripIndex * filmstripHeight;

	Image clip = cachedImage_macroKnob_png.getClippedImage(Rectangle<int>(0, offset, filmstripHeight, filmstripHeight));

	g.setColour(Colours::black.withAlpha(s.isEnabled() ? 1.0f : 0.5f));
	g.drawImage(clip, 0, 0, 48, 48, 0, 0, filmstripHeight, filmstripHeight);

	float displayValue = 1.0f;

	Image *imageToUse = &cachedImage_macroKnob_ring_png;

	
	Image clipRing = imageToUse->getClippedImage(Rectangle<int>(0, (int)(stripIndex * displayValue) * filmstripHeight, filmstripHeight, filmstripHeight));

	g.setColour(Colours::black.withAlpha(s.isEnabled() ? 1.0f : 0.5f));
	g.drawImage(clipRing, 0, 0, 48, 48, 0, 0, filmstripHeight, filmstripHeight);

#endif
	g.setColour(Colours::white.withAlpha(0.3f));

	g.setFont(GLOBAL_BOLD_FONT());
    
    
    
	g.drawText(s.getTextFromValue(value), 0, 0, 48, 48, Justification::centred, false);


	
}

const char* MacroKnobLookAndFeel::macroKnob_png = (const char*)HiBinaryData::LookAndFeelBinaryData::resource_Background_macroKnob2_png;
const int MacroKnobLookAndFeel::macroKnob_pngSize = 241928;


ImageProvider::DisplayScaleFactor ImageProvider::getScaleFactor()
{
	const float scale = (float)Desktop::getInstance().getDisplays().getMainDisplay().scale;

	if (scale == 1.0f) return OneHundred;
	else if (scale == 1.25f) return OneHundredTwentyFive;
	else if (scale == 1.5f) return OneHundredFifty;
	else if (scale == 2.0f) return TwoHundred;
	else return OneHundred;
}

Image ImageProvider::getImage(ImageType type)
{
	

#if HISE_NO_GUI_TOOLS
	return {};
#else
	switch (type)
	{
	case ImageProvider::KnobEmpty:



	case ImageProvider::KnobUnmodulated:


	case ImageProvider::KnobModulated:


	case ImageProvider::MacroKnob:


	case ImageProvider::BalanceKnob:
		return {};
	case ImageProvider::ImageType::ToggleButton:
		
		return ImageCache::getFromMemory(BinaryData::toggle_200_png, BinaryData::toggle_200_pngSize);

	default:
		jassertfalse;
		return Image();
		break;
	}
#endif
}

void ProcessorEditorLookAndFeel::drawBackground(Graphics &g, int width, int height, Colour bgColour, bool folded, int intendationLevel /*= 0*/)
{
	const int xOffset = intendationLevel * 6;

	Colour c = bgColour;

	g.setGradientFill(ColourGradient(c.withMultipliedBrightness(1.05f),
		0.0f, 30.0f,
		c.withMultipliedBrightness(0.95f),
		0.0f, jmax(30.0f, (float)height),
		false));

	float marginBottom = 3.0f;

	int rectangleHeight = folded ? height - 27 : height - 25 - (int)marginBottom;

	g.fillRoundedRectangle(0, 25.0f, static_cast<float> (width), static_cast<float> (rectangleHeight), 3.000f);

	g.setGradientFill(ColourGradient(Colour(0x6e000000),
		0.0f, 27.0f,
		Colour(0x00000000),
		0.0f, 35.0f,
		false));

	if (folded)
	{
		g.fillRect(xOffset, 30, width - 2 * xOffset, 30);
	}
	else
	{
		g.fillRect(0, 30, width, 30);
	}

}


ProcessorEditorLookAndFeel::ProcessorEditorLookAndFeel()
{

}

void ProcessorEditorLookAndFeel::drawShadowBox(Graphics& g, Rectangle<int> area, Colour fillColour)
{
	g.setColour(fillColour);
	g.fillRect(area);

	Colour c1 = JUCE_LIVE_CONSTANT_OFF(Colour(0x77252525));
	Colour c2 = JUCE_LIVE_CONSTANT_OFF(Colour(0x38999999));

	g.setColour(c1);
	g.drawVerticalLine(area.getX(), (float)area.getY(), (float)(area.getBottom()));
	g.drawHorizontalLine(area.getY(), (float)area.getX(), (float)area.getRight());

	g.setColour(c2);
	g.drawVerticalLine(area.getRight() - 1, (float)area.getY(), (float)(area.getBottom()));
	g.drawHorizontalLine(area.getBottom() - 1, (float)area.getX(), (float)area.getRight());
}

void ProcessorEditorLookAndFeel::setupEditorNameLabel(Label* label)
{
	label->setJustificationType(Justification::centredRight);
	label->setEditable(false, false, false);
	label->setColour(Label::textColourId, JUCE_LIVE_CONSTANT_OFF(Colour(0xAAffffff)));
	label->setColour(TextEditor::textColourId, Colours::black);
	label->setColour(TextEditor::backgroundColourId, Colour(0x00000000));

	label->setFont(GLOBAL_BOLD_FONT().withHeight(26.0f));
}

void ProcessorEditorLookAndFeel::fillEditorBackgroundRect(Graphics& g, Component* c, int offsetFromLeftRight /*= 84*/)
{
	g.setColour(JUCE_LIVE_CONSTANT_OFF(Colour(0x27000000)));
	//g.fillRoundedRectangle(static_cast<float> ((c->getWidth() / 2) - ((c->getWidth() - offsetFromLeftRight) / 2)), 3.0f, static_cast<float> (c->getWidth() - offsetFromLeftRight), static_cast<float> (c->getHeight() - 6), 3.000f);
}

void ProcessorEditorLookAndFeel::fillEditorBackgroundRectFixed(Graphics& g, Component* c, int fixedWidth)
{
    g.setColour(JUCE_LIVE_CONSTANT_OFF(Colour(0x27000000)));
    //g.fillRoundedRectangle (static_cast<float> ((c->getWidth() / 2) - (fixedWidth / 2)), 3.0f, (float)fixedWidth, static_cast<float> (c->getHeight() - 6), 3.000f);
    

}

void ProcessorEditorLookAndFeel::drawNoiseBackground(Graphics &g, Rectangle<int> area, Colour /*c*/)
{
    g.setOpacity(JUCE_LIVE_CONSTANT_OFF(0.4f));
    for(int i = area.getY(); i < area.getBottom(); i+=400)
    {
        for(int j = area.getX(); j < area.getRight(); j+=400)
        {
            //g.drawImageAt(ImageCache::getFromMemory(BinaryData::noise_png, BinaryData::noise_pngSize), j,i);
        }
    }
    
    g.setOpacity(1.0f);
}

void FileBrowserLookAndFeel::drawFileBrowserRow(Graphics&g, int width, int height, const File& /*file*/, const String& filename, Image* icon, const String& fileSizeDescription, const String& fileTimeDescription, bool isDirectory, bool isItemSelected, int /*itemIndex*/, DirectoryContentsDisplayComponent& dcc)
{
	Component* const fileListComp = dynamic_cast<Component*> (&dcc);
    


	if (isItemSelected)
	{
		g.setGradientFill(ColourGradient(Colours::black.withAlpha(0.1f), 0.0f, 0.0f,
                                           Colours::black.withAlpha(0.05f), 0.0f, (float)height, false));
		g.fillRoundedRectangle(0.0f, 0.0f, (float)width - 1.0f, (float)height-1.0f, 2.0f);
	}
		

	const int x = 32;
	g.setColour(Colours::black);

	if (icon != nullptr && icon->isValid())
	{
		g.drawImageWithin(*icon, 2, 2, x - 4, height - 4,
			RectanglePlacement::centred | RectanglePlacement::onlyReduceInSize,
			false);
	}
	else
	{
		if (const Drawable* d = isDirectory ? getDefaultFolderImage()
			: getDefaultDocumentFileImage())
			d->drawWithin(g, Rectangle<float>(2.0f, 2.0f, x - 4.0f, height - 4.0f),
			RectanglePlacement::centred | RectanglePlacement::onlyReduceInSize, 1.0f);
	}

	g.setColour(fileListComp != nullptr ? fileListComp->findColour(DirectoryContentsDisplayComponent::textColourId)
		: findColour(DirectoryContentsDisplayComponent::textColourId));
	
	
	g.setFont(!isDirectory ? GLOBAL_BOLD_FONT() : GLOBAL_FONT());
	
	if (width > 450 && !isDirectory)
	{
		const int sizeX = roundToInt(width * 0.7f);
		const int dateX = roundToInt(width * 0.8f);

		g.drawFittedText(filename,
			x, 0, sizeX - x, height,
			Justification::centredLeft, 1);

		
		g.setColour(Colours::darkgrey);

		if (!isDirectory)
		{
			g.drawFittedText(fileSizeDescription,
				sizeX, 0, dateX - sizeX - 8, height,
				Justification::centredRight, 1);

			g.drawFittedText(fileTimeDescription,
				dateX, 0, width - 8 - dateX, height,
				Justification::centredRight, 1);
		}
	}
	else
	{
		g.drawFittedText(filename,
			x, 0, width - x, height,
			Justification::centredLeft, 1);

	}
}

void ConcertinaPanelHeaderLookAndFeel::drawConcertinaPanelHeader(Graphics& g, const Rectangle<int>& area, bool /*isMouseOver*/, bool /*isMouseDown*/, ConcertinaPanel& /*cp*/, Component& panel)
{
#if USE_BACKEND

#if 0
	Path path;

	if (dynamic_cast<MacroParameterTable*>(&panel))
	{
		path.loadPathFromData(BackendBinaryData::ToolbarIcons::macros, sizeof(BackendBinaryData::ToolbarIcons::macros));
	}
	else if (dynamic_cast<ScriptWatchTable*>(&panel))
	{
		path.loadPathFromData(HiBinaryData::SpecialSymbols::scriptProcessor, sizeof(HiBinaryData::SpecialSymbols::scriptProcessor));
	}
	else if (dynamic_cast<ScriptComponentEditPanel* > (&panel))
	{
		path.loadPathFromData(BackendBinaryData::ToolbarIcons::mixer, sizeof(BackendBinaryData::ToolbarIcons::mixer));
	}
	else if (dynamic_cast<SamplePoolTable*>(&panel))
	{
		path.loadPathFromData(BackendBinaryData::ToolbarIcons::sampleTable, sizeof(BackendBinaryData::ToolbarIcons::sampleTable));
	}
	else if (dynamic_cast<FileBrowser*>(&panel))
	{
		path.loadPathFromData(BackendBinaryData::ToolbarIcons::fileBrowser, sizeof(BackendBinaryData::ToolbarIcons::fileBrowser));
	}
	else if (dynamic_cast<PoolTableSubTypes::ImageFilePoolTable*>(&panel))
	{
		path.loadPathFromData(BackendBinaryData::ToolbarIcons::imageTable, sizeof(BackendBinaryData::ToolbarIcons::imageTable));
	}
	else if (dynamic_cast<PoolTableSubTypes::AudioFilePoolTable*>(&panel))
	{
		path.loadPathFromData(BackendBinaryData::ToolbarIcons::fileTable, sizeof(BackendBinaryData::ToolbarIcons::fileTable));
	}
	else if (dynamic_cast<Plotter*>(&panel))
	{
		path.loadPathFromData(BackendBinaryData::ToolbarIcons::plotter, sizeof(BackendBinaryData::ToolbarIcons::plotter));
	}
	else if (dynamic_cast<Console*>(&panel))
	{
		path.loadPathFromData(BackendBinaryData::ToolbarIcons::debugPanel, sizeof(BackendBinaryData::ToolbarIcons::debugPanel));
	}
	else if (dynamic_cast<ModuleBrowser*>(&panel))
	{
		path.loadPathFromData(BackendBinaryData::ToolbarIcons::addIcon, sizeof(BackendBinaryData::ToolbarIcons::addIcon));
	}
	else if (dynamic_cast<ApiCollection*>(&panel))
	{
		path.loadPathFromData(BackendBinaryData::ToolbarIcons::apiList, sizeof(BackendBinaryData::ToolbarIcons::apiList));
	}
	else if (dynamic_cast<PatchBrowser*>(&panel))
	{
		path.loadPathFromData(BackendBinaryData::ToolbarIcons::apiList, sizeof(BackendBinaryData::ToolbarIcons::apiList));
	}
	else
    {
        return;
    }
#endif


    g.setColour(JUCE_LIVE_CONSTANT_OFF(Colour(0xff121212)));

	g.fillAll();    
    g.setColour(Colours::white);
    g.setFont(GLOBAL_BOLD_FONT());
    
    g.drawFittedText(panel.getName(), area.getHeight(), 0, area.getWidth() - 6, area.getHeight(), Justification::centredLeft, 1);
    
    g.setColour(Colours::white);
    
#else

	ignoreUnused(g, area, panel);

#endif

}

void PopupLookAndFeel::drawMenuBarBackground(Graphics& g, int width, int height, bool, MenuBarComponent& /*menuBar*/)
{
	const Colour colour = JUCE_LIVE_CONSTANT_OFF(Colour(0xff282828));

	Rectangle<int> r(width, height);

	g.setColour(colour);
	g.fillRect(r.removeFromTop(1));
	g.fillRect(r.removeFromBottom(1));

	g.setGradientFill(ColourGradient(colour, 0, 0, colour.darker(0.13f), 0, (float)height, false));
	g.fillRect(r);

	g.setColour(JUCE_LIVE_CONSTANT_OFF(Colour(0xff959595)));

	g.drawLine(0.0f, (float)height, (float)width, (float)height);
}


void PopupLookAndFeel::drawHiBackground(Graphics &g, int x, int y, int width, int height, Component *c /*= nullptr*/, bool isMouseOverButton /*= false*/)
{
	Colour upperBgColour = (c != nullptr) ? c->findColour(HiseColourScheme::ColourIds::ComponentFillTopColourId) :
		Colour(0x66333333);

	Colour lowerBgColour = (c != nullptr) ? c->findColour(HiseColourScheme::ColourIds::ComponentFillBottomColourId) :
		Colour(0xfb111111);

	g.setGradientFill(ColourGradient(upperBgColour.withMultipliedBrightness(isMouseOverButton ? 1.6f : 1.1f),
		64.0f, 8.0f,
		lowerBgColour.withMultipliedBrightness(isMouseOverButton ? 1.9f : 1.0f),
		64.0f, (float)(height + 32),
		false));

	g.fillRect((float)x, (float)y, (float)width, (float)height);

	Colour outlineColour = (c != nullptr) ? c->findColour(HiseColourScheme::ColourIds::ComponentOutlineColourId) :
		Colours::white.withAlpha(0.3f);

	g.setColour(outlineColour);

	if (width > 0 && height > 0)
	{
		g.drawRect((float)x, (float)y, (float)width, (float)height, 1.0f);
	}
}


juce::Font PopupLookAndFeel::getPopupMenuFont()
{
	if (HiseDeviceSimulator::isMobileDevice())
	{
		if (comboBoxFont.getTypefaceName() == "Oxygen")
			return comboBoxFont.withHeight(24.0f);
		else
			return comboBoxFont;
	}
	else
	{
		if (comboBoxFont.getTypefaceName() == "Oxygen")
			return comboBoxFont.withHeight(16.0f);
		else
			return comboBoxFont;
	}
}

void PopupLookAndFeel::drawComboBox(Graphics &g, int width, int height, bool isButtonDown, int, int, int, int, ComboBox &c)
{
	c.setColour(ComboBox::ColourIds::textColourId, c.findColour(HiseColourScheme::ColourIds::ComponentTextColourId));

	drawHiBackground(g, 0, 0, width, height - 2, dynamic_cast<ComboBox*>(&c), isButtonDown);

	static const unsigned char pathData[] = { 110, 109, 0, 0, 130, 67, 92, 174, 193, 67, 108, 0, 0, 140, 67, 92, 174, 193, 67, 108, 0, 0, 135, 67, 92, 174, 198, 67, 99, 109, 0, 0, 130, 67, 92, 46, 191, 67, 108, 0, 0, 140, 67, 92, 46, 191, 67, 108, 0, 0, 135, 67, 92, 46, 186, 67, 99, 101, 0, 0 };

	Path path;
	path.loadPathFromData(pathData, sizeof(pathData));

	path.scaleToFit((float)width - 20.0f, (float)(height - 12) * 0.5f, 12.0f, 12.0f, true);

	g.setColour(c.findColour(HiseColourScheme::ColourIds::ComponentTextColourId));
	g.fillPath(path);
}


void ScriptnodeComboBoxLookAndFeel::drawComboBox(Graphics& g, int width, int height, bool isButtonDown, int buttonX, int buttonY, int buttonW, int buttonH, ComboBox& cb)
{
	auto area = cb.getLocalBounds().toFloat();

	drawScriptnodeDarkBackground(g, area, true);

	auto b = area.removeFromRight(area.getHeight()).reduced(area.getHeight() / 3.0f);

	Path p;
	p.addTriangle({ 0.0f, 0.0f }, { 0.5f, 1.0f }, { 1.0f, 0.0f });

	PathFactory::scalePath(p, b);
	g.setColour(cb.findColour(ComboBox::ColourIds::textColourId));
	g.fillPath(p);
}

Label* ScriptnodeComboBoxLookAndFeel::createComboBoxTextBox(ComboBox& c)
{
	auto l = PopupLookAndFeel::createComboBoxTextBox(c);
	l->setColour(Label::ColourIds::textColourId, Colour(0xFFAAAAAA));
	return l;
}

void ScriptnodeComboBoxLookAndFeel::drawScriptnodeDarkBackground(Graphics& g, Rectangle<float> area, bool roundedCorners)
{
	g.setColour(Colour(0xFF262626));

	if (roundedCorners)
	{
		g.fillRoundedRectangle(area, area.getHeight() / 2.0f);
		g.setColour(Colour(0xFF060609));
		g.drawRoundedRectangle(area.reduced(0.5f), area.getHeight() / 2.0f, 1.0f);
	}
	else
	{
		g.fillRect(area);
		g.setColour(Colour(0xFF060609));
		g.drawRect(area, 1.0f);
	}
}

Rectangle<int> getTextBoundsForComboBox(ComboBox& c)
{
	if (c.getHeight() < 20)
		return { 5, 2, c.getWidth() - 25, c.getHeight() - 4 };
	else
		return { 5, 5, c.getWidth() - 25, c.getHeight() - 10 };
}

void PopupLookAndFeel::positionComboBoxText(ComboBox &c, Label &labelToPosition)
{
	labelToPosition.setBounds(getTextBoundsForComboBox(c));

	labelToPosition.setFont(getComboBoxFont(c));
}

void PopupLookAndFeel::drawComboBoxTextWhenNothingSelected(Graphics& g, ComboBox& box, Label& label)
{
	g.setColour(box.findColour(HiseColourScheme::ColourIds::ComponentTextColourId).withMultipliedAlpha(0.5f));

	auto font = label.getLookAndFeel().getLabelFont(label);

	g.setFont(comboBoxFont);

	auto textArea = getTextBoundsForComboBox(box);

	g.drawFittedText(box.getTextWhenNothingSelected(), textArea, label.getJustificationType(), 1);
}

FrontendKnobLookAndFeel::FrontendKnobLookAndFeel():
numStrips(127),
useCustomStrip(false)
{
#if !HISE_NO_GUI_TOOLS
	//volumeFilmStrip = ImageCache::getFromMemory(BinaryData::FrontendKnob_Unipolar_png, BinaryData::FrontendKnob_Unipolar_pngSize);
	//balanceFilmStrip = ImageCache::getFromMemory(BinaryData::FrontendKnob_Bipolar_png, BinaryData::FrontendKnob_Bipolar_pngSize);
#endif
}

void FrontendKnobLookAndFeel::drawRotarySlider(Graphics &g, int /*x*/, int /*y*/, int /*width*/, int /*height*/, float /*sliderPosProportional*/, float /*rotaryStartAngle*/, float /*rotaryEndAngle*/, Slider &s)
{
	const double value = s.getValue();
	const double normalizedValue = (value - s.getMinimum()) / (s.getMaximum() - s.getMinimum());
	const double proportion = pow(normalizedValue, s.getSkewFactor());
	const int stripIndex = (int)(proportion * (numStrips - 1));

	const int filmstripHeight = volumeFilmStrip.getHeight() / numStrips;

	const int offset = stripIndex * filmstripHeight;

	Image *imageToUse;
	
	if (useCustomStrip)
	{
		imageToUse = &volumeFilmStrip;
	}
	else
	{
		if (s.getName() == "Volume")
		{
			imageToUse = &volumeFilmStrip;
		}
		else
		{
			imageToUse = &balanceFilmStrip;
		}
	}

	Image clip = imageToUse->getClippedImage(Rectangle<int>(0, offset, filmstripHeight, filmstripHeight));

	if (!useCustomStrip)
	{
		if (s.isMouseButtonDown())
		{
			g.setColour(Colours::black.withAlpha(1.0f));
		}
		else if (s.isMouseOver())
		{
			g.setColour(Colours::black.withAlpha(0.8f));
		}
		else
		{
			g.setColour(Colours::black.withAlpha(0.5f));
		}
	}
	
	g.drawImageAt(clip, 0, 0);
}

void AlertWindowLookAndFeel::drawButtonText(Graphics &g, TextButton &button, bool /*isMouseOverButton*/, bool /*isButtonDown*/)
{
	Font font(getTextButtonFont(button, button.getHeight()));
	g.setFont(font);

    
    Colour c = dark;
    
    if(button.isColourSpecified(HiseColourScheme::ComponentTextColourId))
        c = button.findColour(HiseColourScheme::ComponentTextColourId);

	g.setColour(c.withMultipliedAlpha(button.isEnabled() ? 1.0f : 0.5f));

	const int yIndent = jmin(4, button.proportionOfHeight(0.3f));
	const int cornerSize = jmin(button.getHeight(), button.getWidth()) / 2;

	const int fontHeight = roundToInt(font.getHeight() * 0.6f);
	const int leftIndent = jmin(fontHeight, 2 + cornerSize / (button.isConnectedOnLeft() ? 4 : 2));
	const int rightIndent = jmin(fontHeight, 2 + cornerSize / (button.isConnectedOnRight() ? 4 : 2));

	g.drawFittedText(button.getButtonText(),
		leftIndent,
		yIndent,
		button.getWidth() - leftIndent - rightIndent,
		button.getHeight() - yIndent * 2,
		Justification::centred, 2);
}

void AlertWindowLookAndFeel::drawButtonBackground(Graphics& g, Button& button, const Colour& /*backgroundColour*/, bool isMouseOverButton, bool isButtonDown)
{
    Colour c = bright;
    
    if(button.isColourSpecified(HiseColourScheme::ComponentBackgroundColour))
        c = button.findColour(HiseColourScheme::ComponentBackgroundColour);
    
	Colour baseColour(c.withMultipliedSaturation(button.hasKeyboardFocus(true) ? 1.3f : 0.9f)
                       .withMultipliedAlpha(button.isEnabled() ? 0.9f : 0.5f));

	if (isButtonDown || isMouseOverButton)
		baseColour = baseColour.contrasting(isButtonDown ? 0.2f : 0.1f);

	g.setColour(baseColour);

	const float width = (float)button.getWidth();
	const float height = (float)button.getHeight();

	g.fillRoundedRectangle(0.f, 0.f, width, height, 3.0f);
}

void AlertWindowLookAndFeel::drawAlertBox(Graphics &g, AlertWindow &alert, const Rectangle< int > &textArea, juce::TextLayout &textLayout)
{
	ColourGradient grad(dark.withMultipliedBrightness(1.4f), 0.0f, 0.0f,
		dark, 0.0f, (float)alert.getHeight(), false);

	g.setGradientFill(grad);
	g.fillAll();
	g.setColour(Colours::white.withAlpha(0.1f));
	g.fillRect(0, 0, alert.getWidth(), 37);
	g.setColour(bright);

	for (int i = 0; i < textLayout.getNumLines(); i++)
		textLayout.getLine(i).runs.getUnchecked(0)->colour = bright;

	textLayout.draw(g, Rectangle<int>(textArea.getX(),
                                      textArea.getY(),
                                      textArea.getWidth(),
                                      textArea.getHeight()).toFloat());

	g.setColour(bright);
	g.drawRect(0, 0, alert.getWidth(), alert.getHeight());
}


void ChainBarButtonLookAndFeel::drawButtonText(Graphics& g, TextButton& button, bool /*isMouseOverButton*/, bool /*isButtonDown*/)
{
	auto alpha = 0.5f;

	if (button.getToggleState())
		alpha += 0.5f;

	if (!button.isEnabled())
		alpha = 0.3f;

	g.setFont(GLOBAL_BOLD_FONT());
	g.setColour(Colours::white.withAlpha(alpha));
	g.drawText(button.getButtonText(), 0, 0, button.getWidth(), button.getHeight(), Justification::centred, false);
}

void ChainBarButtonLookAndFeel::drawButtonBackground(Graphics &g, Button &b, const Colour &backgroundColour, bool isMouseOverButton, bool isButtonDown)
{
	float alpha = 0.05f;
	if (isMouseOverButton || isButtonDown)
		alpha += 0.05f;
	if (isButtonDown)
		alpha += 0.05f;

	if (b.getToggleState())
		alpha += 0.1f;

	auto c = Colours::white.withAlpha(alpha);

	Colour baseColour(c.withMultipliedAlpha(b.isEnabled() ? 1.0f : 0.5f));

	const bool flatOnLeft = b.isConnectedOnLeft();
	const bool flatOnRight = b.isConnectedOnRight();
	const bool flatOnTop = b.isConnectedOnTop();
	const bool flatOnBottom = b.isConnectedOnBottom();

	const float width = (float)b.getWidth() - 1.0f;
	const float height = (float)b.getHeight() - 1.0f;

	if (width > 0 && height > 0)
	{
		const float cornerSize = b.getHeight() / 2.0f;

		Path outline;
		outline.addRoundedRectangle(0.5f, 0.5f, width, height, cornerSize, cornerSize,
			!(flatOnLeft || flatOnTop),
			!(flatOnRight || flatOnTop),
			!(flatOnLeft || flatOnBottom),
			!(flatOnRight || flatOnBottom));

		g.setColour(Colours::white.withAlpha(0.15f));
		g.strokePath(outline, PathStrokeType(1.0f));

		g.setColour(baseColour);

		g.fillPath(outline);	
	}

	g.setColour(Colours::white.withAlpha(b.getToggleState() ? 0.4f : 0.1f));

	ChainBarPathFactory factory;

	Path path = factory.createPath(b.getButtonText());

	path.scaleToFit(4.0f, 4.0f, (float)b.getHeight() - 8.0f, (float)b.getHeight() - 8.0f, true);


	g.setColour(Colours::white.withAlpha(0.7f));


	g.fillPath(path);
}

BlackTextButtonLookAndFeel::BlackTextButtonLookAndFeel()
{
	//================== CheckboxOff.png ==================
	static const unsigned char CheckboxOff_png[] =
	{ 137, 80, 78, 71, 13, 10, 26, 10, 0, 0, 0, 13, 73, 72, 68, 82, 0, 0, 0, 18, 0, 0, 0, 18, 8, 6, 0, 0, 0, 86, 206, 142, 87, 0, 0, 0, 25, 116, 69, 88, 116, 83, 111, 102, 116, 119, 97, 114, 101, 0, 65, 100, 111, 98, 101, 32, 73, 109, 97, 103, 101, 82, 101, 97, 100, 121, 113, 201, 101, 60, 0, 0, 0, 205, 73, 68, 65, 84, 120, 218, 236, 148,
		189, 14, 130, 48, 20, 133, 107, 69, 18, 16, 24, 140, 3, 131, 83, 223, 197, 149, 135, 117, 245, 93, 152, 28, 24, 136, 131, 173, 154, 248, 127, 46, 57, 24, 6, 18, 26, 67, 162, 131, 77, 190, 180, 105, 238, 57, 189, 41, 61, 40, 245, 107, 99, 66, 166, 96, 198, 89, 15, 104, 30, 224, 14, 174, 156, 159, 178, 25, 80, 28, 89, 107,
		215, 73, 146, 44, 125, 78, 119, 206, 213, 105, 154, 110, 177, 60, 131, 91, 107, 36, 157, 100, 52, 217, 128, 106, 192, 39, 71, 109, 33, 26, 154, 52, 70, 154, 29, 197, 44, 170, 60, 26, 106, 107, 98, 106, 85, 107, 36, 132, 31, 220, 111, 216, 189, 79, 61, 214, 87, 251, 27, 125, 209, 40, 247, 208, 244, 214, 4, 204, 206, 165,
		44, 75, 103, 140, 41, 124, 78, 151, 90, 209, 80, 251, 14, 109, 4, 22, 96, 5, 36, 38, 115, 238, 247, 13, 9, 232, 17, 212, 96, 7, 246, 204, 91, 35, 8, 104, 150, 241, 217, 15, 189, 114, 233, 228, 4, 14, 221, 208, 142, 246, 27, 121, 9, 48, 0, 56, 46, 49, 129, 197, 153, 5, 151, 0, 0, 0, 0, 73, 69, 78, 68, 174, 66, 96, 130, 0, 0 };


	//================== CheckboxOn.png ==================
	static const unsigned char CheckboxOn_png[] =
	{ 137, 80, 78, 71, 13, 10, 26, 10, 0, 0, 0, 13, 73, 72, 68, 82, 0, 0, 0, 18, 0, 0, 0, 18, 8, 6, 0, 0, 0, 86, 206, 142, 87, 0, 0, 0, 25, 116, 69, 88, 116, 83, 111, 102, 116, 119, 97, 114, 101, 0, 65, 100, 111, 98, 101, 32, 73, 109, 97, 103, 101, 82, 101, 97, 100, 121, 113, 201, 101, 60, 0, 0, 2, 113, 73, 68, 65, 84, 120, 218, 156, 84,
		65, 139, 82, 81, 20, 126, 62, 223, 60, 181, 244, 133, 58, 211, 128, 14, 20, 34, 227, 54, 221, 21, 4, 22, 182, 16, 35, 177, 101, 180, 40, 92, 184, 168, 69, 8, 45, 102, 221, 174, 31, 33, 173, 2, 107, 35, 173, 220, 136, 11, 197, 34, 6, 74, 73, 51, 68, 161, 154, 160, 73, 81, 28, 199, 158, 227, 123, 79, 251, 142, 220, 27, 78,
		205, 76, 52, 194, 199, 121, 231, 190, 123, 63, 191, 243, 157, 115, 159, 32, 156, 242, 55, 159, 207, 55, 129, 71, 60, 23, 79, 67, 162, 105, 218, 53, 132, 103, 170, 170, 174, 35, 202, 180, 38, 1, 38, 192, 12, 172, 176, 120, 34, 121, 163, 209, 184, 98, 50, 153, 158, 64, 141, 216, 233, 116, 136, 68, 1, 122, 18, 59, 108, 27,
		141, 70, 55, 236, 118, 251, 234, 73, 36, 211, 233, 116, 85, 146, 164, 123, 32, 18, 13, 195, 192, 145, 17, 237, 183, 112, 69, 164, 68, 97, 36, 175, 176, 233, 251, 81, 36, 245, 122, 93, 246, 251, 253, 207, 69, 81, 84, 145, 158, 155, 205, 102, 162, 207, 231, 187, 176, 236, 17, 41, 58, 67, 201, 113, 36, 244, 243, 120, 60,
		41, 89, 150, 47, 146, 69, 140, 88, 79, 165, 82, 101, 118, 126, 161, 72, 228, 134, 45, 117, 228, 42, 194, 93, 224, 1, 200, 245, 106, 181, 186, 225, 112, 56, 238, 240, 247, 165, 82, 73, 139, 70, 163, 223, 198, 227, 241, 46, 210, 25, 39, 90, 38, 56, 15, 201, 143, 241, 72, 93, 17, 134, 195, 225, 109, 132, 151, 94, 175, 247,
		161, 217, 108, 38, 11, 132, 98, 177, 56, 217, 218, 218, 58, 0, 201, 39, 164, 132, 189, 67, 237, 135, 121, 38, 152, 119, 19, 100, 151, 249, 154, 213, 106, 189, 95, 169, 84, 46, 185, 221, 238, 235, 148, 215, 106, 181, 105, 60, 30, 111, 193, 171, 207, 72, 223, 82, 19, 129, 253, 67, 68, 131, 193, 64, 82, 20, 229, 117, 56,
		28, 126, 209, 108, 54, 23, 47, 45, 22, 203, 90, 48, 24, 124, 74, 251, 250, 253, 190, 158, 72, 36, 218, 248, 179, 237, 88, 44, 214, 194, 90, 27, 24, 241, 210, 196, 165, 214, 210, 179, 163, 92, 46, 203, 161, 80, 232, 107, 161, 80, 24, 51, 85, 78, 168, 156, 39, 147, 201, 29, 204, 205, 59, 44, 85, 34, 145, 200, 15, 196, 3,
		114, 227, 175, 201, 134, 26, 29, 225, 11, 73, 198, 196, 190, 135, 153, 237, 124, 62, 191, 168, 63, 147, 201, 244, 114, 185, 92, 13, 143, 111, 128, 150, 203, 229, 210, 255, 236, 234, 111, 179, 49, 71, 6, 2, 117, 129, 148, 236, 225, 26, 168, 40, 197, 200, 102, 179, 155, 233, 116, 186, 137, 181, 109, 128, 226, 24, 198,
		207, 143, 37, 98, 93, 91, 71, 187, 119, 89, 55, 180, 201, 100, 98, 192, 92, 42, 227, 35, 240, 129, 26, 73, 123, 142, 154, 51, 137, 153, 53, 69, 253, 251, 152, 212, 91, 216, 184, 120, 209, 237, 118, 87, 208, 234, 53, 172, 219, 2, 129, 128, 15, 77, 80, 156, 78, 231, 162, 36, 218, 75, 103, 184, 209, 2, 187, 176, 54, 192,
		5, 108, 0, 116, 77, 206, 178, 117, 129, 93, 31, 153, 77, 179, 198, 14, 82, 233, 61, 96, 7, 232, 3, 42, 39, 146, 24, 153, 194, 174, 138, 252, 143, 175, 8, 41, 249, 201, 6, 145, 72, 116, 78, 244, 95, 159, 17, 166, 202, 96, 10, 13, 62, 2, 191, 4, 24, 0, 152, 78, 7, 110, 65, 209, 129, 62, 0, 0, 0, 0, 73, 69, 78, 68, 174, 66, 96,
		130, 0, 0 };

	ticked = ImageCache::getFromMemory(CheckboxOn_png, sizeof(CheckboxOn_png));
	unticked = ImageCache::getFromMemory(CheckboxOff_png, sizeof(CheckboxOff_png));

	f = GLOBAL_BOLD_FONT();
	textColour = Colours::white;
}

void BlackTextButtonLookAndFeel::drawToggleButton(Graphics &g, ToggleButton &b, bool , bool)
{
	g.drawImageAt(b.getToggleState() ? ticked : unticked, 0, 3);

	g.setColour(textColour);
	g.setFont(f);

	const int textX = 24;

	g.drawFittedText(b.getButtonText(),
		textX, 4,
		b.getWidth() - textX - 2, b.getHeight() - 8,
		Justification::centredLeft, 10);

	g.setColour(textColour.withAlpha(0.2f));

	g.drawHorizontalLine(b.getHeight() - 1, 0.0f, (float)b.getWidth());
}

void HiPropertyPanelLookAndFeel::drawPropertyComponentBackground(Graphics& g, int width, int height, PropertyComponent& /*component*/)
{
	g.setColour(propertyBgColour);
	g.fillAll();

	g.setColour(Colours::white.withAlpha(0.03f));
	g.drawHorizontalLine(0, 0.0f, (float)width);
	g.setColour(Colours::black.withAlpha(0.05f));
	g.drawHorizontalLine(height-1, 0.0f, (float)width);

}


void PresetBrowserLookAndFeelMethods::drawPresetBrowserBackground(Graphics& g, Component* p)
{
    if (!backgroundColour.isTransparent())
    {
        g.setGradientFill(ColourGradient(backgroundColour.withMultipliedBrightness(1.2f), 0.0f, 0.0f,
            backgroundColour, 0.0f, (float)p->getHeight(), false));

        g.fillAll();
    }
}

void PresetBrowserLookAndFeelMethods::drawColumnBackground(Graphics& g, int columnIndex, Rectangle<int> listArea, const String& emptyText)
{
    g.setColour(highlightColour.withAlpha(0.1f));
    g.drawRoundedRectangle(listArea.toFloat(), 2.0f, 2.0f);

    if (emptyText.isNotEmpty())
    {
        g.setFont(font);
        g.setColour(textColour.withAlpha(0.3f));
        g.drawText(emptyText, 0, 0, listArea.getWidth(), listArea.getHeight(), Justification::centred);
    }
}

void PresetBrowserLookAndFeelMethods::drawTag(Graphics& g, bool blinking, bool active, bool selected, const String& name, Rectangle<int> position)
{
    float alpha = active ? 0.4f : 0.1f;
    alpha += (blinking ? 0.2f : 0.0f);

    auto ar = position.toFloat().reduced(1.0f);

    g.setColour(highlightColour.withAlpha(alpha));
    g.fillRoundedRectangle(ar, 2.0f);
    g.drawRoundedRectangle(ar, 2.0f, 1.0f);
    g.setFont(font.withHeight(14.0f));
    g.setColour(Colours::white.withAlpha(selected ? 0.9f : 0.6f));

    // Wow, so professional, good bug fix.
    auto nameToUse = (name == "Agressive" ? "Aggressive" : name);

    g.drawText(nameToUse, ar, Justification::centred);

    if (selected)
        g.drawRoundedRectangle(ar, 2.0f, 2.0f);
}




juce::Font PresetBrowserLookAndFeelMethods::getFont(bool fontForTitle)
{
    return fontForTitle ? GLOBAL_BOLD_FONT().withHeight(18.0f) : GLOBAL_BOLD_FONT();
}

void PresetBrowserLookAndFeelMethods::drawPresetBrowserButtonBackground(Graphics& g, Button& button, const Colour&, bool , bool )
{
    if (button.getToggleState())
    {
        auto r = button.getLocalBounds();

        g.setColour(highlightColour.withAlpha(0.1f));
        g.fillRoundedRectangle(r.reduced(3, 1).toFloat(), 2.0f);
    }
}

void PresetBrowserLookAndFeelMethods::drawListItem(Graphics& g, int columnIndex, int, const String& itemName, Rectangle<int> position, bool rowIsSelected, bool deleteMode, bool hover)
{
    float alphaBoost = hover ? 0.1f : 0.0f;

    g.setGradientFill(ColourGradient(highlightColour.withAlpha(0.3f + alphaBoost), 0.0f, 0.0f,
        highlightColour.withAlpha(0.2f + alphaBoost), 0.0f, (float)position.getHeight(), false));

    if (rowIsSelected)
        g.fillRect(position);

    g.setColour(Colours::white.withAlpha(0.9f));

    if (deleteMode)
    {
        Path p;
        p.loadPathFromData(HiBinaryData::ProcessorEditorHeaderIcons::closeIcon, sizeof(HiBinaryData::ProcessorEditorHeaderIcons::closeIcon));

        auto r = position.removeFromRight(position.getHeight()).reduced(3).toFloat();
        p.scaleToFit(r.getX(), r.getY(), r.getWidth(), r.getHeight(), true);

        g.fillPath(p);
    }

    g.setColour(textColour);
    g.setFont(font.withHeight(16.0f));
    g.drawText(itemName, columnIndex == 2 ? 10 + 26 : 10, 0, position.getWidth() - 20, position.getHeight(), Justification::centredLeft);
}

void PresetBrowserLookAndFeelMethods::drawPresetBrowserButtonText(Graphics& g, TextButton& button, bool isMouseOverButton, bool isButtonDown)
{
    g.setColour(highlightColour.withAlpha(isMouseOverButton || button.getToggleState() ? 1.0f : 0.7f));
    g.setFont(font);
    g.drawText(button.getButtonText(), 0, isButtonDown ? 1 : 0, button.getWidth(), button.getHeight(), Justification::centred);

    if (isMouseOverButton)
    {
        auto r = button.getLocalBounds();

        g.setColour(highlightColour.withAlpha(0.1f));
        g.fillRoundedRectangle(r.reduced(3, 1).toFloat(), 2.0f);
    }
}



void PresetBrowserLookAndFeelMethods::drawModalOverlay(Graphics& g, Rectangle<int> area, Rectangle<int> labelArea, const String& title, const String& command)
{
    g.setColour(modalBackgroundColour);
    g.fillAll();

    g.setColour(JUCE_LIVE_CONSTANT_OFF(Colour(0xfa212121)));
    g.fillRoundedRectangle(area.expanded(40).toFloat(), 2.0f);

    g.setColour(JUCE_LIVE_CONSTANT_OFF(Colour(0x228e8e8e)));

    if (!labelArea.isEmpty())
        g.fillRect(labelArea);

    g.setColour(Colours::white.withAlpha(0.8f));
    g.setFont(font.withHeight(18));
    g.drawText(title, area.getX(), labelArea.getY() - 80, area.getWidth(), 30, Justification::centredTop);

    g.setFont(font);

    g.drawText(command, area, Justification::centredTop);
}

juce::Path PresetBrowserLookAndFeelMethods::createPresetBrowserIcons(const String& id)
{
	Path path;

	if (id == "searchIcon")
	{
		static const unsigned char searchIcon[] = { 110, 109, 0, 0, 144, 68, 0, 0, 48, 68, 98, 7, 31, 145, 68, 198, 170, 109, 68, 78, 223, 103, 68, 148, 132, 146, 68, 85, 107, 42, 68, 146, 2, 144, 68, 98, 54, 145, 219, 67, 43, 90, 143, 68, 66, 59, 103, 67, 117, 24, 100, 68, 78, 46, 128, 67, 210, 164, 39, 68, 98, 93, 50, 134, 67, 113, 58, 216, 67, 120, 192, 249, 67, 83, 151,
		103, 67, 206, 99, 56, 68, 244, 59, 128, 67, 98, 72, 209, 112, 68, 66, 60, 134, 67, 254, 238, 144, 68, 83, 128, 238, 67, 0, 0, 144, 68, 0, 0, 48, 68, 99, 109, 0, 0, 208, 68, 0, 0, 0, 195, 98, 14, 229, 208, 68, 70, 27, 117, 195, 211, 63, 187, 68, 146, 218, 151, 195, 167, 38, 179, 68, 23, 8, 77, 195, 98, 36, 92, 165, 68, 187, 58,
		191, 194, 127, 164, 151, 68, 251, 78, 102, 65, 0, 224, 137, 68, 0, 0, 248, 66, 98, 186, 89, 77, 68, 68, 20, 162, 194, 42, 153, 195, 67, 58, 106, 186, 193, 135, 70, 41, 67, 157, 224, 115, 67, 98, 13, 96, 218, 193, 104, 81, 235, 67, 243, 198, 99, 194, 8, 94, 78, 68, 70, 137, 213, 66, 112, 211, 134, 68, 98, 109, 211, 138, 67,
		218, 42, 170, 68, 245, 147, 37, 68, 128, 215, 185, 68, 117, 185, 113, 68, 28, 189, 169, 68, 98, 116, 250, 155, 68, 237, 26, 156, 68, 181, 145, 179, 68, 76, 44, 108, 68, 16, 184, 175, 68, 102, 10, 33, 68, 98, 249, 118, 174, 68, 137, 199, 2, 68, 156, 78, 169, 68, 210, 27, 202, 67, 0, 128, 160, 68, 0, 128, 152, 67, 98, 163,
		95, 175, 68, 72, 52, 56, 67, 78, 185, 190, 68, 124, 190, 133, 66, 147, 74, 205, 68, 52, 157, 96, 194, 98, 192, 27, 207, 68, 217, 22, 154, 194, 59, 9, 208, 68, 237, 54, 205, 194, 0, 0, 208, 68, 0, 0, 0, 195, 99, 101, 0, 0 };

		
		path.loadPathFromData(searchIcon, sizeof(searchIcon));
		path.applyTransform(AffineTransform::rotation(float_Pi));
	}
	else if (id == "favorite_on")
	{
		static const unsigned char onShape[] = { 110,109,109,167,45,67,0,0,0,0,108,227,165,86,67,129,85,252,66,108,109,167,173,67,129,85,252,66,108,231,251,111,67,156,36,76,67,108,47,125,140,67,174,39,165,67,108,109,167,45,67,129,85,124,67,108,246,168,132,66,174,39,165,67,108,227,165,214,66,156,36,
76,67,108,0,0,0,0,129,85,252,66,108,246,168,4,67,129,85,252,66,108,109,167,45,67,0,0,0,0,99,101,0,0 };
		
		path.loadPathFromData(onShape, sizeof(onShape));
	}
	else if (id == "favorite_off")
	{
		static const unsigned char offShape[] = { 110,109,0,144,89,67,0,103,65,67,108,0,159,88,67,0,3,68,67,108,129,106,86,67,0,32,74,67,108,1,38,77,67,0,108,74,67,108,1,121,84,67,0,28,80,67,108,129,227,81,67,255,3,89,67,108,1,144,89,67,127,206,83,67,108,1,60,97,67,255,3,89,67,108,129,166,94,67,0,28,
			80,67,108,129,249,101,67,0,108,74,67,108,1,181,92,67,0,32,74,67,108,1,144,89,67,0,103,65,67,99,109,0,144,89,67,1,76,71,67,108,128,73,91,67,1,21,76,67,108,0,94,96,67,129,62,76,67,108,0,90,92,67,129,92,79,67,108,128,196,93,67,129,62,84,67,108,0,144,89,
			67,129,99,81,67,108,0,91,85,67,1,63,84,67,108,128,197,86,67,129,92,79,67,108,128,193,82,67,129,62,76,67,108,0,214,87,67,1,21,76,67,108,0,144,89,67,1,76,71,67,99,101,0,0 };

		path.loadPathFromData(offShape, sizeof(offShape));
	}

	return path;
}

void PresetBrowserLookAndFeelMethods::drawSearchBar(Graphics& g, Rectangle<int> area)
{
	g.setColour(highlightColour);
	g.drawRoundedRectangle(area.toFloat().reduced(1.0f), 2.0f, 1.0f);

	auto path = createPresetBrowserIcons("searchIcon");

	path.scaleToFit(6.0f, 5.0f, 18.0f, 18.0f, true);

	g.fillPath(path);
}

void ProcessorEditorHeaderLookAndFeel::drawBackground(Graphics &g, float width, float height, bool /*isFolded*/)
{
	g.excludeClipRegion(Rectangle<int>(0, 35, (int)width, 10));

	g.setGradientFill(ColourGradient(getColour(HeaderBackgroundColour),
		288.0f, 8.0f,
		getColour(HeaderBackgroundColour).withMultipliedBrightness(0.9f),
		288.0f, height,
		false));

	if (isChain)
		g.fillRoundedRectangle(0.0f, 0.0f, width, height, 3.0f);
	else
		g.fillAll();
}

void PeriodicScreenshotter::drawGlassSection(Graphics& g, Component* c, Rectangle<int> b, Point<int> offset)
{
    if (b.isEmpty())
        b = c->getLocalBounds();

    auto area = comp->getLocalArea(c, b);
    area = area.getIntersection(comp->getLocalBounds());

    b = c->getLocalArea(comp, area);
    auto clip = img.getClippedImage(area.translated(offset.getX(), offset.getY()).transformedBy(AffineTransform::scale(0.5f)));
    g.setOpacity(1.0f);

    g.saveState();
    g.setImageResamplingQuality(Graphics::lowResamplingQuality);
    g.drawImageWithin(clip, b.getX(), b.getY(), b.getWidth(), b.getHeight(), RectanglePlacement::fillDestination);
    g.restoreState();
}

void PeriodicScreenshotter::PopupGlassLookAndFeel::drawPopupMenuBackgroundWithOptions (Graphics& g,
                                                 int width,
                                                 int height,
                                                 const PopupMenu::Options& o)
{
    g.fillAll(Colour(0xEE404040));
    if(auto s = holder->getScreenshotter())
    {
        auto x = o.getTargetScreenArea().getX();
        auto y = o.getTargetScreenArea().getX();
        s->drawGlassSection(g, dynamic_cast<Component*>(holder.get()), {0, 0, width, height}, {x, y});
    }
    
    g.fillAll(Colour(0x22000000));
    
    g.setColour(Colours::white.withAlpha(0.1f));
    g.drawRect(0, 0, width, height);
}

} // namespace hise
