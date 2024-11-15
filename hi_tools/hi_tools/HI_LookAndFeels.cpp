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

namespace hise { using namespace juce;

	FilmstripLookAndFeel::FilmstripLookAndFeel():
		imageToUse(Image()),
		isVertical(true),
		numStrips(0),
		scaleFactor(1.0f)
	{}

	void FilmstripLookAndFeel::setFilmstripImage(const Image& imageToUse_, int numStrips_, bool isVertical_)
	{
		imageToUse = imageToUse_;
		numStrips = numStrips_;
		isVertical = isVertical_;

		if (numStrips == 0) return;

		if (isVertical)
		{
			heightOfEachStrip = imageToUse.getHeight() / numStrips;
			widthOfEachStrip = imageToUse.getWidth();
		}
		else
		{
			heightOfEachStrip = imageToUse.getHeight();
			widthOfEachStrip = imageToUse.getWidth() / numStrips;
		}

	}

	void FilmstripLookAndFeel::setScaleFactor(float newScaleFactor) noexcept
	{
#ifdef HI_FIXED_SCALEFACTOR_FOR_FILMSTRIPS
		ignoreUnused(newScaleFactor);

		scaleFactor = HI_FIXED_SCALEFACTOR_FOR_FILMSTRIPS;
#else
		scaleFactor = newScaleFactor;
#endif
	}

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

	g.setColour (b.getToggleState() ? Colours::white.withAlpha(0.9f) : Colours::white.withAlpha(0.4f));
	g.setFont (GLOBAL_FONT());

	String text = b.getButtonText();

	Rectangle<int> textArea(30, 6, b.getWidth() - 36, b.getHeight() - 12);

	if (textArea.getHeight() > 0 && textArea.getWidth() > 0)
	{
		g.drawText(text, textArea, Justification::centredLeft, true);
	}
	
	auto area = b.getLocalBounds().removeFromLeft(b.getHeight()).withSizeKeepingCentre(16, 16).toFloat().reduced(JUCE_LIVE_CONSTANT_OFF(1.0f));

	g.setColour(Colours::black.withAlpha(0.5f));
	g.fillEllipse(area);
	g.setColour(Colours::white.withAlpha(b.getToggleState() ? 0.8f : 0.2f));
	g.fillEllipse(area.reduced(JUCE_LIVE_CONSTANT_OFF(3.0f)));
}

GlobalHiseLookAndFeel::GlobalHiseLookAndFeel()
{
	static const uint8 ringData[] = { 110, 109, 203, 161, 243, 65, 12, 2, 240, 65, 98, 96, 229, 189, 65, 90, 228, 19, 66, 152, 110, 74, 65, 94, 58, 21, 66, 180, 200, 178, 64, 166, 155, 245, 65, 98, 6, 129, 197, 191, 162, 69, 192, 65, 33, 176, 242, 191, 121, 233, 76, 65, 154, 153, 153, 64, 215, 163, 180, 64, 98, 158, 239, 55, 65, 74, 12, 194, 191, 168, 198,
		181, 65, 6, 129, 245, 191, 229, 208, 238, 65, 207, 247, 151, 64, 98, 152, 238, 19, 66, 240, 167, 54, 65, 16, 88, 21, 66, 221, 36, 181, 65, 184, 30, 245, 65, 164, 112, 238, 65, 98, 0, 0, 245, 65, 104, 145, 238, 65, 72, 225, 244, 65, 33, 176, 238, 65, 156, 196, 244, 65, 217, 206, 238, 65, 108, 180, 200, 233, 65, 227, 165,
		185, 65, 98, 92, 143, 252, 65, 250, 126, 146, 65, 109, 231, 244, 65, 236, 81, 68, 65, 215, 163, 211, 65, 172, 28, 6, 65, 98, 104, 145, 170, 65, 125, 63, 101, 64, 242, 210, 83, 65, 119, 190, 119, 64, 33, 176, 6, 65, 176, 114, 16, 65, 98, 63, 53, 102, 64, 170, 241, 98, 65, 201, 118, 118, 64, 209, 34, 178, 65, 143, 194, 15,
		65, 68, 139, 216, 65, 98, 102, 102, 80, 65, 156, 196, 246, 65, 209, 34, 151, 65, 156, 196, 251, 65, 201, 118, 188, 65, 139, 108, 232, 65, 108, 203, 161, 243, 65, 12, 2, 240, 65, 99, 101, 0, 0 };

	ring.loadPathFromData(ringData, sizeof(ringData));

	ring2.startNewSubPath(0.5f, 1.0f);
	ring2.addArc(0.0f, 0.0f, 1.0f, 1.0f, -float_Pi * 0.75f - 0.04f, float_Pi * 0.75f + 0.04f, true);
    
	setColour(PopupMenu::highlightedBackgroundColourId, Colour(SIGNAL_COLOUR));

	Colour dark(0xFF252525);

	Colour bright(0xFF999999);

	setColour(PopupMenu::ColourIds::backgroundColourId, dark);
	setColour(PopupMenu::ColourIds::textColourId, bright);
	setColour(PopupMenu::ColourIds::highlightedBackgroundColourId, bright);
	setColour(PopupMenu::ColourIds::highlightedTextColourId, dark);

	comboBoxFont = GLOBAL_FONT();
}

Slider::SliderLayout GlobalHiseLookAndFeel::getSliderLayout(Slider& s)
{
	auto layout = LookAndFeel_V3::getSliderLayout(s);

	if (s.getSliderStyle() == Slider::RotaryHorizontalVerticalDrag)
	{
		layout.textBoxBounds = layout.textBoxBounds.withBottomY(s.getHeight()-3);
	}

	return layout;
}

Label* GlobalHiseLookAndFeel::createSliderTextBox(Slider& s)
{
	Label *label = new Label("Textbox");
	label->setFont (GLOBAL_FONT());
	label->setEditable (false, false, false);
		
	Colour textColour;
	Colour contrastColour;

	if(s.getSliderStyle() == Slider::RotaryHorizontalVerticalDrag)
	{
			
		label->setJustificationType (Justification::centred);
		label->setEditable (false, false, false);
			
		textColour = Colour(0x66ffffff);
		contrastColour = Colours::black;
	}
	else
	{
			
		label->setJustificationType (Justification::centred);
			
		textColour = s.findColour(Slider::textBoxTextColourId);
		contrastColour = textColour.contrasting();
			
	}

	label->setColour(CaretComponent::ColourIds::caretColourId, Colours::white);
	label->setColour(Label::textColourId, textColour);
	label->setColour(Label::ColourIds::textWhenEditingColourId, textColour);
	label->setColour(TextEditor::ColourIds::highlightColourId, textColour);
	label->setColour(TextEditor::ColourIds::highlightedTextColourId, contrastColour);
	label->setColour(TextEditor::ColourIds::focusedOutlineColourId, textColour);

	return label;
}

int GlobalHiseLookAndFeel::getSliderThumbRadius(Slider& slider)
{ return 0; }

void GlobalHiseLookAndFeel::drawLinearSlider(Graphics& g, int i, int i1, int width, int height, float x,
	float minSliderPos, float maxSliderPos, const Slider::SliderStyle style, Slider& s)
{
	if (style == Slider::TwoValueHorizontal)
	{
		g.fillAll(s.findColour(Slider::backgroundColourId));

		//width += 18;
			
		const float leftBoxPos = minSliderPos;// *(float)(width - 6.0f) + 3.0f;
		const float rightBoxPos = maxSliderPos;// *(float)(width - 6.0f) + 3.0f;

		Rectangle<float> area(leftBoxPos, 0.0f, rightBoxPos - leftBoxPos, (float)height);

		area.reduce(-1.0f, -1.0f);
			
		g.setColour(s.findColour(Slider::ColourIds::thumbColourId));
		g.fillRect(area);

		g.setColour(s.findColour(Slider::ColourIds::trackColourId));
		g.drawRect(0, 0, width, height, 1);

		g.drawLine(leftBoxPos, 0.0f, leftBoxPos, (float)height, 1.0f);
		g.drawLine(rightBoxPos, 0.0f, rightBoxPos, (float)height, 1.0f);

		g.setColour(s.findColour(Slider::ColourIds::textBoxTextColourId));

		const int decimal = jmax<int>(0, (int)-log10(s.getInterval()));

		const String text = String(s.getMinValue(), decimal) + " - " + String(s.getMaxValue(), decimal);
		g.setFont(GLOBAL_FONT());
		g.drawText(text, 0, 0, width, height, Justification::centred, false);
	}
	else
	{
		height = s.getHeight();
		width = s.getWidth();

		if (!s.isEnabled()) g.setOpacity(0.4f);

		//drawHiBackground(g, 12, 6, width-18, 32);

		const double value = s.getValue();
		const double normalizedValue = (value - s.getMinimum()) / (s.getMaximum() - s.getMinimum());
		const double proportion = pow(normalizedValue, s.getSkewFactor());

		g.fillAll(s.findColour(Slider::backgroundColourId));

		g.setColour(s.findColour(Slider::ColourIds::thumbColourId));

		if (style == Slider::SliderStyle::LinearBar)
		{
			g.fillRect(0.0f, 0.0f, (float)proportion * (float)width, (float)height);
		}
		else if (style == Slider::SliderStyle::LinearBarVertical)
		{
			g.fillRect(0.0f, (float)(1.0 - proportion)*height, (float)width, (float)(proportion * height));
		}

		g.setColour(s.findColour(Slider::ColourIds::trackColourId));
		g.drawRect(0, 0, width, height, 1);
	}

		

}

void GlobalHiseLookAndFeel::setDefaultColours(Component& c)
{
	c.setColour(HiseColourScheme::ComponentBackgroundColour, Colours::transparentBlack);
	c.setColour(HiseColourScheme::ComponentFillTopColourId, Colour(0x66333333));
	c.setColour(HiseColourScheme::ComponentFillBottomColourId, Colour(0xfb111111));
	c.setColour(HiseColourScheme::ComponentOutlineColourId, Colours::white.withAlpha(0.3f));
	c.setColour(HiseColourScheme::ComponentTextColourId, Colours::white);
}


void GlobalHiseLookAndFeel::fillPathHiStyle(Graphics &g, const Path &p, int, int, bool drawBorders /*= true*/)
{
	if(!PathFactory::isValid(p))
		return;
	


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
    
    const int R = jlimit<int>(1, 100, HI_RASTER_SIZE);
    
    if (mulAlpha > 0.1f)
    {
        
        for (int x = R; x < bounds.getWidth(); x += R)
        {
            float alpha = (x % (R*10) == 0) ? tenAlpha : oneAlpha;
            alpha *= mulAlpha;
            g.setColour(lineColour.withAlpha(alpha));
            ug.draw1PxVerticalLine(x, 0.0f, (float)bounds.getHeight());
        }

        for (int y = R; y < bounds.getHeight(); y += R)
        {
            float alpha = (y % (R*10) == 0) ? tenAlpha : oneAlpha;
            alpha *= mulAlpha;
            g.setColour(lineColour.withAlpha(alpha));
            ug.draw1PxHorizontalLine(y, 0.0f, (float)bounds.getWidth());
        }
    }
}

Point<float> GlobalHiseLookAndFeel::paintCable(Graphics& g, Rectangle<float> start, Rectangle<float> end, Colour c, float alpha /*= 1.0f*/, Colour holeColour /*= Colour(0xFFAAAAAA)*/, bool returnMidPoint /*= false*/, bool useHangingCable/*=true*/, Point<float> velocity)
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

		controlPoint.setY(controlPoint.getY() - jmin(100.0f, hmath::abs(velocity.getDistanceFromOrigin()) * 4.0f));

		p.quadraticTo(controlPoint, end.getCentre());
		
	}
	else
	{
		Rectangle<float> cableBounds(start.getCentre(), end.getCentre());

		Line<float> cableAsLine(start.getCentre(), end.getCentre());

		auto mid = cableBounds.getCentre();

		Point<float> c1 = { cableAsLine.getPointAlongLineProportionally(0.2f).getX(), start.getCentreY() };
		Point<float> c2 = { cableAsLine.getPointAlongLineProportionally(0.8f).getX(), end.getCentreY() };

		c1 -= velocity;
		c2 += velocity;

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
	ed.setColour(TextEditor::ColourIds::backgroundColourId, Colours::white.withAlpha(0.45f));
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
	return {};
}

Path HiToolbarIcons::createSettingsPath()
{
	static const unsigned char pathData[] = { 110,109,35,219,53,65,18,131,180,65,108,19,131,74,65,86,14,166,65,108,84,227,51,65,119,190,154,65,108,219,249,22,65,111,18,165,65,98,178,157,17,65,117,147,163,65,59,223,11,65,66,96,162,65,10,215,5,65,31,133,161,65,108,0,0,0,65,0,0,144,65,108,0,0,192,64,
		0,0,144,65,108,236,81,180,64,31,133,161,65,98,138,65,168,64,66,96,162,65,156,196,156,64,105,145,163,65,74,12,146,64,111,18,165,65,108,176,114,48,64,119,190,154,65,108,108,231,171,63,86,14,166,65,108,116,147,40,64,18,131,180,65,98,165,155,28,64,38,49,
		183,65,12,2,19,64,98,16,186,65,87,57,12,64,122,20,189,65,108,0,8,131,58,255,255,191,65,108,0,8,131,58,255,255,207,65,108,87,57,12,64,132,235,210,65,98,110,18,19,64,157,239,213,65,165,155,28,64,216,206,216,65,214,163,40,64,236,124,219,65,108,47,8,172,
		63,168,241,233,65,108,18,131,48,64,135,65,245,65,108,123,20,146,64,143,237,234,65,98,205,204,156,64,137,108,236,65,186,73,168,64,188,159,237,65,29,90,180,64,223,122,238,65,108,50,8,192,64,254,255,255,65,108,25,4,0,65,254,255,255,65,108,35,219,5,65,223,
		122,238,65,98,84,227,11,65,188,159,237,65,203,161,17,65,149,110,236,65,244,253,22,65,143,237,234,65,108,109,231,51,65,135,65,245,65,108,44,135,74,65,168,241,233,65,108,60,223,53,65,236,124,219,65,98,48,221,56,65,216,206,216,65,150,67,59,65,156,239,213,
		65,220,249,60,65,132,235,210,65,108,26,4,96,65,255,255,207,65,108,26,4,96,65,255,255,191,65,108,220,249,60,65,122,20,189,65,98,150,67,59,65,97,16,186,65,73,225,56,65,38,49,183,65,60,223,53,65,18,131,180,65,99,109,0,0,224,64,0,0,216,65,98,215,163,188,
		64,0,0,216,65,0,0,160,64,10,215,208,65,0,0,160,64,0,0,200,65,98,0,0,160,64,246,40,191,65,215,163,188,64,0,0,184,65,0,0,224,64,0,0,184,65,98,20,174,1,65,0,0,184,65,0,0,16,65,246,40,191,65,0,0,16,65,0,0,200,65,98,0,0,16,65,10,215,208,65,20,174,1,65,0,0,
		216,65,0,0,224,64,0,0,216,65,99,109,0,0,0,66,0,0,64,65,108,0,0,0,66,0,0,32,65,108,234,38,239,65,59,223,25,65,98,11,215,238,65,34,219,21,65,177,114,238,65,84,227,17,65,207,247,237,65,231,251,13,65,108,41,92,252,65,71,225,246,64,108,113,61,246,65,118,190,
		187,64,108,6,129,229,65,63,53,202,64,98,176,114,228,65,198,75,195,64,235,81,227,65,116,147,188,64,196,32,226,65,122,20,182,64,108,71,225,235,65,186,116,123,64,108,104,145,224,65,192,245,32,64,108,225,122,210,65,216,249,110,64,98,35,219,208,65,161,112,
		101,64,14,45,207,65,124,106,92,64,176,114,205,65,204,247,83,64,108,98,16,209,65,240,40,156,63,108,174,71,194,65,172,245,232,62,108,12,2,185,65,134,65,16,64,98,86,14,183,65,222,122,12,64,110,18,181,65,171,71,9,64,98,16,179,65,177,200,6,64,108,0,0,176,
		65,0,0,64,181,108,0,0,160,65,0,0,64,181,108,158,239,156,65,177,200,6,64,98,146,237,154,65,171,71,9,64,170,241,152,65,124,106,12,64,244,253,150,65,134,65,16,64,108,82,184,141,65,168,245,232,62,108,60,223,125,65,239,40,156,63,108,80,141,130,65,204,247,
		83,64,98,242,210,128,65,124,106,92,64,187,73,126,65,161,112,101,64,62,10,123,65,216,249,110,64,108,48,221,94,65,192,245,32,64,108,114,61,72,65,186,116,123,64,108,120,190,91,65,122,20,182,64,98,42,92,89,65,116,147,188,64,161,26,87,65,198,75,195,64,245,
		253,84,65,63,53,202,64,108,32,133,51,65,118,190,187,64,108,175,71,39,65,71,225,246,64,108,99,16,68,65,231,251,13,65,98,185,30,67,65,84,227,17,65,236,81,66,65,34,219,21,65,46,178,65,65,59,223,25,65,108,1,0,32,65,0,0,32,65,108,1,0,32,65,0,0,64,65,108,46,
		178,65,65,197,32,70,65,98,236,81,66,65,222,36,74,65,161,26,67,65,172,28,78,65,99,16,68,65,25,4,82,65,108,175,71,39,65,93,143,100,65,108,32,133,51,65,99,16,129,65,108,245,253,84,65,98,229,122,65,98,161,26,87,65,30,90,126,65,42,92,89,65,36,219,128,65,120,
		190,91,65,226,122,130,65,108,114,61,72,65,105,145,144,65,108,48,221,94,65,72,225,155,65,108,62,10,123,65,197,32,146,65,98,187,73,126,65,236,81,147,65,242,210,128,65,177,114,148,65,80,141,130,65,7,129,149,65,108,60,223,125,65,114,61,166,65,108,82,184,
		141,65,42,92,172,65,108,244,253,150,65,208,247,157,65,98,170,241,152,65,165,112,158,65,146,237,154,65,11,215,158,65,158,239,156,65,235,38,159,65,108,0,0,160,65,2,0,176,65,108,0,0,176,65,2,0,176,65,108,98,16,179,65,236,38,159,65,98,110,18,181,65,13,215,
		158,65,86,14,183,65,179,114,158,65,12,2,185,65,209,247,157,65,108,174,71,194,65,43,92,172,65,108,98,16,209,65,115,61,166,65,108,176,114,205,65,8,129,149,65,98,14,45,207,65,178,114,148,65,35,219,208,65,237,81,147,65,225,122,210,65,198,32,146,65,108,104,
		145,224,65,73,225,155,65,108,71,225,235,65,106,145,144,65,108,196,32,226,65,227,122,130,65,98,235,81,227,65,37,219,128,65,176,114,228,65,32,90,126,65,6,129,229,65,100,229,122,65,108,112,61,246,65,100,16,129,65,108,40,92,252,65,96,143,100,65,108,206,247,
		237,65,28,4,82,65,98,163,112,238,65,175,28,78,65,9,215,238,65,225,36,74,65,233,38,239,65,200,32,70,65,108,0,0,0,66,3,0,64,65,99,109,0,0,168,65,154,153,117,65,98,180,200,148,65,154,153,117,65,51,51,133,65,152,110,86,65,51,51,133,65,0,0,48,65,98,51,51,
		133,65,104,145,9,65,180,200,148,65,205,204,212,64,0,0,168,65,205,204,212,64,98,76,55,187,65,205,204,212,64,205,204,202,65,104,145,9,65,205,204,202,65,0,0,48,65,98,205,204,202,65,152,110,86,65,76,55,187,65,154,153,117,65,0,0,168,65,154,153,117,65,99,101,
		0,0 };

	Path path;
	path.loadPathFromData (pathData, sizeof (pathData));
		
	return path;
		
}

NumberTag::LookAndFeelMethods::~LookAndFeelMethods()
{}

void NumberTag::LookAndFeelMethods::drawNumberTag(Graphics& g, Component& comp, Colour& c, Rectangle<int> area, int offset, int size,
	int number)
{
	if (number > 0)
	{
		Rectangle<float> rect = area.reduced(offset).removeFromTop(size).removeFromRight(size).toFloat();
		DropShadow d;

		d.colour = c.withAlpha(0.3f);
		d.radius = (int)offset * 2;
		d.offset = Point<int>();
		d.drawForRectangle(g, Rectangle<int>((int)rect.getX(), (int)rect.getY(), (int)rect.getWidth(), (int)rect.getHeight()));

		g.setColour(Colours::black.withAlpha(0.3f));
		g.setColour(Colours::white.withAlpha(0.7f));
		g.drawRoundedRectangle(rect.reduced(1.0f), 4.0f, 1.0f);
		g.setFont(GLOBAL_BOLD_FONT());
		g.drawText(String(number), rect, Justification::centred, false);
	}
}

NumberTag::NumberTag(float offset_, float size_, Colour c_):
	number(0),
	offset(offset_),
	size(size_),
	c(c_)
{
	setInterceptsMouseClicks(false, false);
	setLookAndFeel(&dlaf);
}

NumberTag::~NumberTag()
{
	setLookAndFeel(nullptr);
}

void NumberTag::paint(Graphics& g)
{
	if(number == 0) 
		return;

	if (auto l = dynamic_cast<LookAndFeelMethods*>(&getLookAndFeel()))
		l->drawNumberTag(g, *this, c, getLocalBounds(), roundToInt(offset), roundToInt(size), number);
}

void NumberTag::setNumber(int newNumber)
{
	number = newNumber;
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

void HiPropertyPanelLookAndFeel::setFontForAll(const Font& newFont)
{
	comboBoxFont = newFont;
	textButtonFont = newFont;
	labelFont = newFont;
	popupMenuFont = newFont;
}

HiPropertyPanelLookAndFeel::HiPropertyPanelLookAndFeel()
{
	comboBoxFont = GLOBAL_FONT();
	textButtonFont = GLOBAL_FONT();
	labelFont = GLOBAL_FONT();
	popupMenuFont = GLOBAL_FONT();

	setColour(PopupMenu::highlightedBackgroundColourId, Colour(SIGNAL_COLOUR));

	Colour dark(0xFF333333);

	Colour bright(0xFF999999);

	setColour(PopupMenu::ColourIds::backgroundColourId, dark);
	setColour(PopupMenu::ColourIds::textColourId, bright);
	setColour(PopupMenu::ColourIds::highlightedBackgroundColourId, bright);
	setColour(PopupMenu::ColourIds::highlightedTextColourId, dark);
	setColour(PopupMenu::ColourIds::headerTextColourId, bright);
	setColour (TextEditor::highlightColourId, Colour (SIGNAL_COLOUR).withAlpha(0.5f));
	setColour (TextEditor::ColourIds::focusedOutlineColourId, Colour(SIGNAL_COLOUR));

}

void HiPropertyPanelLookAndFeel::drawPropertyPanelSectionHeader(Graphics& g, const String& name, bool isOpen, int width,
	int height)
{
	g.setColour(JUCE_LIVE_CONSTANT_OFF(Colour(0xff1b1b1b)));

	auto r = Rectangle<int>(0, 0, width, height);
        
	g.fillRect(r);

	PopupLookAndFeel::drawFake3D(g, r);
        
        
        
	const float buttonSize = height * 0.75f;
	const float buttonIndent = (height - buttonSize) * 0.5f;

	drawTreeviewPlusMinusBox (g, Rectangle<float> (buttonIndent, buttonIndent, buttonSize, buttonSize), Colours::black, isOpen, false);

	const int textX = (int) (buttonIndent * 2.0f + buttonSize + 2.0f);

	g.setColour (JUCE_LIVE_CONSTANT_OFF(Colour(0xffa2a2a2)));
	g.setFont (GLOBAL_BOLD_FONT());
	g.drawText (name, textX, 0, width - textX - 4, height, Justification::centredLeft, true);
}

Rectangle<int> HiPropertyPanelLookAndFeel::getPropertyComponentContentPosition(PropertyComponent& component)
{
	const int textW = jmin(labelWidth, component.getWidth() / 3);
	return Rectangle<int>(textW, 1, component.getWidth() - textW - 1, component.getHeight() - 3);
}

void HiPropertyPanelLookAndFeel::setLabelWidth(int newLabelWidth)
{
	labelWidth = newLabelWidth;
}

void HiPropertyPanelLookAndFeel::drawPropertyComponentLabel(Graphics& g, int i, int i1, PropertyComponent& component)
{
	Colour textColour = JUCE_LIVE_CONSTANT_OFF(Colour(0xffdddddd));

	g.setColour (textColour.withMultipliedAlpha (component.isEnabled() ? 1.0f : 0.6f));

	g.setFont (labelFont);

	const Rectangle<int> r (getPropertyComponentContentPosition (component));

	if(r.getX() > 8)
	{
		g.drawFittedText (component.getName(),
		                  3, r.getY(), r.getX() - 8, r.getHeight(),
		                  Justification::centredRight, 2);
	}
}

void HiPropertyPanelLookAndFeel::drawLinearSlider(Graphics& g, int i, int i1, int width, int height, float x, float x1,
	float x2, const Slider::SliderStyle sliderStyle, Slider& s)
{
	const bool isBiPolar = s.getMinimum() < 0.0 && s.getMaximum() > 0.0;

	float leftX;
	float actualWidth;

	const float max = (float)s.getMaximum();
	const float min = (float)s.getMinimum();

	Colour c = s.findColour(Slider::ColourIds::backgroundColourId);

	g.fillAll(c);

	if(isBiPolar)
	{
		const float value = ((float)s.getValue() - min) / (max - min);

		leftX = 2.0f + (value < 0.5f ? value * (float)(width-2) : 0.5f * (float)(width-2));
		actualWidth = fabs(0.5f - value) * (float)(width-2);
	}
	else
	{
		const double value = s.getValue();
		const double normalizedValue = (value - s.getMinimum()) / (s.getMaximum() - s.getMinimum());
		const double proportion = pow(normalizedValue, s.getSkewFactor());

		//const float value = ((float)s.getValue() - min) / (max - min);

		leftX = 2;
		actualWidth = (float)proportion * (float)(width-2);
	}

    if(actualWidth > 0.0f)
    {
        c = s.findColour(Slider::ColourIds::thumbColourId);

        g.setGradientFill (ColourGradient (c.withMultipliedAlpha(s.isEnabled() ? 0.8f : 0.4f),
                                           0.0f, 0.0f,
                                           c.withMultipliedAlpha(s.isEnabled() ? 0.8f : 0.4f),
                                           0.0f, (float)height,
                                           false));
        
        
        
        g.fillRect(leftX, 2.0f, actualWidth , (float)(height-2));
    }
}

Font HiPropertyPanelLookAndFeel::getLabelFont(Label& label)
{
	return labelFont;
}

Font HiPropertyPanelLookAndFeel::getComboBoxFont(ComboBox& comboBox)
{
	return comboBoxFont;
}

Font HiPropertyPanelLookAndFeel::getPopupMenuFont()
{
	return popupMenuFont;
}

void HiPropertyPanelLookAndFeel::drawPopupMenuBackground(Graphics& g, int width, int height)
{
	g.setColour( (findColour (PopupMenu::backgroundColourId)));
	g.fillRect(0.0f, 0.0f, (float)width, (float)height);
}

Font HiPropertyPanelLookAndFeel::getTextButtonFont(TextButton& textButton, int i)
{
	return textButtonFont;
}

void HiPropertyPanelLookAndFeel::drawButtonBackground(Graphics& g, Button& b, const Colour& colour, bool over,
	bool down)
{
	auto ar = b.getLocalBounds().toFloat();

	if (b.getToggleState())
	{
		g.setColour(Colour(SIGNAL_COLOUR).withAlpha(0.4f));
		g.fillRoundedRectangle(ar, 2.0f);
	}

	g.setColour(Colours::white.withAlpha(0.1f));

	if (over)
		g.fillRoundedRectangle(ar, 2.0f);

	if (down)
		g.fillRoundedRectangle(ar, 2.0f);
}

void HiPropertyPanelLookAndFeel::drawButtonText(Graphics& g, TextButton& b, bool cond, bool cond1)
{
	g.setFont(GLOBAL_BOLD_FONT());
	g.setColour(b.getToggleState() ? Colours::black : Colours::white);
	g.drawText(b.getButtonText(), b.getLocalBounds().toFloat(), Justification::centred);
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


void PopupLookAndFeel::drawFake3D(Graphics& g, Rectangle<int> area)
{
	Colour upperLight = JUCE_LIVE_CONSTANT_OFF(Colour(0x10ffffff));
	Colour c1 = JUCE_LIVE_CONSTANT_OFF(Colour(0x06ffffff));
	Colour c2 = JUCE_LIVE_CONSTANT_OFF(Colour(0x10000000));
	Colour shadowLight = JUCE_LIVE_CONSTANT_OFF(Colour(0x58000000));
        
        
	g.setGradientFill(ColourGradient(c1, 0.0f, (float)area.getY(), c2, 0.0f, (float)area.getBottom(), false));
	g.fillRect(area);
        
	g.setColour(upperLight);
        
	g.drawHorizontalLine(area.getY(), (float)area.getX(), (float)area.getRight());
        
	g.setColour(shadowLight);
	g.drawHorizontalLine(area.getBottom()-1, (float)area.getX(), (float)area.getRight());
}

PopupLookAndFeel::PopupLookAndFeel()
{
	comboBoxFont = GLOBAL_BOLD_FONT();

	setColour(PopupMenu::highlightedBackgroundColourId, Colour(SIGNAL_COLOUR));

	Colour dark = JUCE_LIVE_CONSTANT_OFF(Colour(0xff333333));

	Colour bright(0xFFAAAAAA);

	setColour(PopupMenu::ColourIds::backgroundColourId, dark);
	setColour(PopupMenu::ColourIds::textColourId, bright);
	setColour(PopupMenu::ColourIds::highlightedBackgroundColourId, bright);
	setColour(PopupMenu::ColourIds::highlightedTextColourId, dark);
	setColour(PopupMenu::ColourIds::headerTextColourId, bright);
}

int PopupLookAndFeel::showAtComponent(PopupMenu& m, Component* c, bool alignToBottom)
{
        
	PopupMenu::Options options;
	options = options.withTargetComponent (c);
        
	if(!alignToBottom)
	{
		Rectangle<int> r;
            
		r.setPosition(Desktop::getMousePosition());
		options = options.withTargetScreenArea(r);
	}
        
	return m.showMenu(options);
}

void PopupLookAndFeel::setComboBoxFont(Font f)
{
	comboBoxFont = f;
}

void PopupLookAndFeel::getIdealPopupMenuItemSize(const String& text, bool isSeparator, int standardMenuItemHeight,
	int& idealWidth, int& idealHeight)
{
	if (HiseDeviceSimulator::isMobileDevice())
	{
		idealHeight = 28;

		idealWidth = getPopupMenuFont().getStringWidth(text) + 50;
	}
	else
	{
		if (isSeparator)
		{
			idealWidth = 50;
			idealHeight = standardMenuItemHeight > 0 ? standardMenuItemHeight / 2 : 10;
		}
		else
		{
			Font font(getPopupMenuFont());

			if (standardMenuItemHeight > 0 && font.getHeight() > standardMenuItemHeight / 1.3f)
				font.setHeight(standardMenuItemHeight / 1.3f);

			idealHeight = standardMenuItemHeight > 0 ? standardMenuItemHeight : roundToInt(font.getHeight() * 1.3f);

			idealHeight = jmax<int>(idealHeight, 18);

			idealWidth = font.getStringWidth(text) + idealHeight * 2;
		}
	}
}

void PopupLookAndFeel::drawMenuBarItem(Graphics& g, int width, int height, int i, const String& itemText,
	bool isMouseOverItem, bool isMenuOpen, bool cond, MenuBarComponent& menuBar)
{
	if (!menuBar.isEnabled())
	{
		g.setColour(menuBar.findColour(PopupMenu::textColourId));
	}
	else if (isMenuOpen || isMouseOverItem)
	{
		Colour c1 = findColour(PopupMenu::highlightedBackgroundColourId).withMultipliedBrightness(JUCE_LIVE_CONSTANT_OFF(1.4f));
		Colour c2 = findColour(PopupMenu::highlightedBackgroundColourId).withMultipliedBrightness(1.1f);

		g.setGradientFill(ColourGradient(c1, 0.0f, 0.0f, c2, 0.0f, (float)height, false));

		g.fillRect(0, 0, width, height);

		Colour dark(0xFF444444);

			

		g.setColour(dark);
	}
	else
	{
		g.setColour(JUCE_LIVE_CONSTANT_OFF(Colour(0xffbdbdbd)));
	}

	g.setFont(getPopupMenuFont());
		
	g.drawFittedText(itemText, 0, 0, width, height, Justification::centred, 1);

		
}

void PopupLookAndFeel::drawTooltip(Graphics& g, const String& text, int width, int height)
{
	g.fillAll(Colour(0xFF444444));
	g.setColour(Colours::white.withAlpha(0.8f));
	g.drawRect(0, 0, width, height, 1);
	g.setFont(GLOBAL_BOLD_FONT());
	g.drawText(text, 0, 0, width, height, Justification::centred);
}

bool PopupLookAndFeel::shouldPopupMenuScaleWithTargetComponent(const PopupMenu::Options& options)
{
	return true;
}

void PopupLookAndFeel::drawPopupMenuBackground(Graphics& g, int width, int height)
{
	Colour c1 = findColour(PopupMenu::backgroundColourId).withMultipliedBrightness(JUCE_LIVE_CONSTANT_OFF(1.1f));
	Colour c2 = findColour(PopupMenu::backgroundColourId).withMultipliedBrightness(0.9f);

	g.setGradientFill(ColourGradient(c1, 0.0f, 0.0f, c2, 0.0f, (float)height, false));

	g.fillRect(0.0f, 0.0f, (float)width, (float)height);
	(void)width; (void)height;

	g.setColour(JUCE_LIVE_CONSTANT_OFF(Colour(0xff555555)));

	g.drawRect(0.0f, 0.0f, (float)width, (float)height, 1.0f);

#if ! JUCE_MAC
	g.setColour(findColour(PopupMenu::textColourId));
	//g.drawRoundedRectangle(0.0f, 0.0f, (float)width, (float)height, 4.0f, 0.5f);
#endif
}

void PopupLookAndFeel::drawPopupMenuItem(Graphics& g, const Rectangle<int>& area, bool isSeparator, bool isActive,
	bool isHighlighted, bool isTicked, bool hasSubMenu, const String& text, const String& shortcutKeyText,
	const Drawable* icon, const Colour* textColourToUse)
{
	if (isSeparator)
	{
		Rectangle<int> r (area.reduced (0, 0));
		r.removeFromTop (r.getHeight() / 2 - 1);
            
		g.setColour (Colour(0x55999999));
		g.fillRect (r.removeFromTop (1));
	}
	else
	{
		Colour textColour (findColour (PopupMenu::textColourId));
            
		if (textColourToUse != nullptr)
			textColour = *textColourToUse;
            
		Rectangle<int> r (area.reduced (1));
            
		if (isHighlighted)
		{
			Colour c1 = findColour (PopupMenu::highlightedBackgroundColourId).withMultipliedBrightness(JUCE_LIVE_CONSTANT_OFF(1.4f));
			Colour c2 = findColour(PopupMenu::highlightedBackgroundColourId).withMultipliedBrightness(1.1f);
                
			g.setGradientFill(ColourGradient(c1, 0.0f, 0.0f, c2, 0.0f, (float)r.getHeight(), false));

			g.fillRect (r);
                
			g.setColour (findColour (PopupMenu::highlightedTextColourId));
		}
		else
		{
			g.setColour (textColour);
		}
            
		if (! isActive)
			g.setOpacity (0.3f);
            
		Font font (getPopupMenuFont());
            
		const float maxFontHeight = area.getHeight() / 1.3f;
            
		if (font.getHeight() > maxFontHeight)
			font.setHeight (maxFontHeight);
            
		g.setFont (font);
            
		Rectangle<float> iconArea (r.removeFromLeft ((r.getHeight() * 5) / 4).reduced (3).toFloat());
            
		if (icon != nullptr)
		{
			if (auto dp = dynamic_cast<const DrawablePath*>(icon))
			{
				auto p = dp->getPath();

				p.scaleToFit(iconArea.getX(), iconArea.getY(), iconArea.getWidth(), iconArea.getHeight(), true);

				g.fillPath(p);
			}
			else
			{
				icon->drawWithin(g, iconArea, RectanglePlacement::centred | RectanglePlacement::onlyReduceInSize, 1.0f);
			}

				
		}
		else if (isTicked)
		{
			const Path tick (getTickShape (1.0f));
			g.fillPath (tick, tick.getTransformToScaleToFit (iconArea, true));
		}
            
		if (hasSubMenu)
		{
			const float arrowH = 0.6f * getPopupMenuFont().getAscent();
                
			const float x = (float) r.removeFromRight ((int) arrowH).getX();
			const float halfH = (float) r.getCentreY();
                
			Path p;
			p.addTriangle (x, halfH - arrowH * 0.5f,
			               x, halfH + arrowH * 0.5f,
			               x + arrowH * 0.6f, halfH);
                
			g.fillPath (p);
		}
            
		r.removeFromRight (3);
		g.drawFittedText (text, r, Justification::centredLeft, 1);
            
		if (shortcutKeyText.isNotEmpty())
		{
			Font f2 (font);
			f2.setHeight (f2.getHeight() * 0.75f);
			f2.setHorizontalScale (0.95f);
			g.setFont (f2);
                
			g.drawText (shortcutKeyText, r, Justification::centredRight, true);
		}
	}
}

void PopupLookAndFeel::drawPopupMenuSectionHeader(Graphics& g, const Rectangle<int>& area, const String& sectionName)
{
	g.setFont (getPopupMenuFont());
	g.setColour (Colours::white);
        
	g.drawFittedText (sectionName,
	                  area.getX() + 12, area.getY(), area.getWidth() - 16, (int) (area.getHeight() * 0.8f),
	                  Justification::bottomLeft, 1);
}

Rectangle<int> PopupLookAndFeel::getPropertyComponentContentPosition(PropertyComponent& component)
{
	const int textW = jmin(200, component.getWidth() / 3);
	return Rectangle<int>(textW, 1, component.getWidth() - textW - 1 - component.getHeight()-1, component.getHeight() - 3);
}

Component* PopupLookAndFeel::getParentComponentForMenuOptions(const PopupMenu::Options& options)
{
	if (HiseDeviceSimulator::isAUv3())
	{
		if (options.getParentComponent() == nullptr && options.getTargetComponent() != nullptr)
			return options.getTargetComponent()->getTopLevelComponent();
	}
		
	return LookAndFeel_V3::getParentComponentForMenuOptions(options);

}

Font PopupLookAndFeel::getComboBoxFont(ComboBox& comboBox)
{
	return comboBoxFont;
}

void PopupLookAndFeel::drawHiBackground(Graphics &g, int x, int y, int width, int height, Component *c /*= nullptr*/, bool isMouseOverButton /*= false*/)
{
	Colour upperBgColour = (c != nullptr) ? c->findColour(HiseColourScheme::ColourIds::ComponentFillTopColourId, true) :
		Colour(0x66333333);

	Colour lowerBgColour = (c != nullptr) ? c->findColour(HiseColourScheme::ColourIds::ComponentFillBottomColourId, true) :
		Colour(0xfb111111);

	g.setGradientFill(ColourGradient(upperBgColour.withMultipliedBrightness(isMouseOverButton ? 1.6f : 1.1f),
		64.0f, 8.0f,
		lowerBgColour.withMultipliedBrightness(isMouseOverButton ? 1.9f : 1.0f),
		64.0f, (float)(height + 32),
		false));

	g.fillRect((float)x, (float)y, (float)width, (float)height);

	Colour outlineColour = (c != nullptr) ? c->findColour(HiseColourScheme::ColourIds::ComponentOutlineColourId, true) :
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
    auto textColour = c.findColour(HiseColourScheme::ColourIds::ComponentTextColourId, true);
    //jassert(textColour != Colours::transparentBlack);
    
	c.setColour(ComboBox::ColourIds::textColourId, textColour);

	drawHiBackground(g, 0, 0, width, height - 2, dynamic_cast<ComboBox*>(&c), isButtonDown);

	static const unsigned char pathData[] = { 110, 109, 0, 0, 130, 67, 92, 174, 193, 67, 108, 0, 0, 140, 67, 92, 174, 193, 67, 108, 0, 0, 135, 67, 92, 174, 198, 67, 99, 109, 0, 0, 130, 67, 92, 46, 191, 67, 108, 0, 0, 140, 67, 92, 46, 191, 67, 108, 0, 0, 135, 67, 92, 46, 186, 67, 99, 101, 0, 0 };

	Path path;
	path.loadPathFromData(pathData, sizeof(pathData));

	path.scaleToFit((float)width - 20.0f, (float)(height - 12) * 0.5f, 12.0f, 12.0f, true);

	g.setColour(c.findColour(HiseColourScheme::ColourIds::ComponentTextColourId, true));
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

TableHeaderLookAndFeel::TableHeaderLookAndFeel()
{
	f = GLOBAL_BOLD_FONT();

	bgColour = Colour(0xff474747);
	textColour = Colour(0xa2ffffff);
}

void TableHeaderLookAndFeel::drawTableHeaderBackground(Graphics& graphics, TableHeaderComponent& tableHeaderComponent)
{
		
		
}

void TableHeaderLookAndFeel::drawTableHeaderColumn(Graphics& g, TableHeaderComponent& tableHeaderComponent,
	const String& columnName, int i, int width, int height, bool cond, bool cond1, int i1)
{
	if (width > 0)
	{
		g.setColour(bgColour);

		g.fillRect(0.0f, 0.0f, (float)width - 1.0f, (float)height);

		g.setFont(f);
		g.setColour(textColour);

		g.drawText(columnName, 3, 0, width - 3, height, Justification::centredLeft, true);
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

PopupMenu::Options PopupLookAndFeel::getOptionsForComboBoxPopupMenu (ComboBox& box, Label& label)
{
    auto options = LookAndFeel_V3::getOptionsForComboBoxPopupMenu(box, label);
    
    auto alignment = box.getProperties()["popupAlignment"].toString();
    
    if(alignment.isNotEmpty())
    {
        auto area = options.getTargetScreenArea().toFloat();
        
        auto sf = UnblurryGraphics::getScaleFactorForComponent(&box);
        
        if(alignment == "topRight")
            area.translate((float)box.getWidth() * sf, -1.0f * (float)box.getHeight() * sf);
        if(alignment == "bottomRight")
            area.translate((float)box.getWidth() * sf, 0.0f);
        if(alignment == "top")
            area.translate(0.0f, -1.0f * (float)box.getHeight() * sf);
        
        return options.withTargetScreenArea(area.toNearestInt());
    }
    
    return options;
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

void FrontendKnobLookAndFeel::setCustomFilmstripImage(const Image* customImage, int numFilmstrips)
{
	if (numFilmstrips != 0 && customImage->isValid())
	{
		volumeFilmStrip = customImage->createCopy();
		balanceFilmStrip = volumeFilmStrip;

		numStrips = numFilmstrips;
		useCustomStrip = true;
	}
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


BalanceButtonLookAndFeel::BalanceButtonLookAndFeel()
{
}

void BalanceButtonLookAndFeel::drawRotarySlider(Graphics& g, int x, int y, int width, int height,
	float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle, Slider& s)
{
	auto area = Rectangle<int>(x, y, width, height).toFloat();
	area.removeFromBottom(JUCE_LIVE_CONSTANT_OFF(3.0f));
	auto size = jmin(area.getWidth(), area.getHeight());

	if(s.isMouseButtonDown())
		size -= 1.0f;

	area = area.withSizeKeepingCentre(size, size);

	float alpha = 0.5f;

	if(s.isMouseOverOrDragging())
		alpha += 0.3f;

	if(s.isMouseButtonDown(true))
		alpha += 0.2f;

	Path track, thumb;

	float radius = JUCE_LIVE_CONSTANT_OFF(0.6f);

	track.addPieSegment(area, -2.7f, 2.7f, radius);
	thumb.addPieSegment(area, 0.0f, -2.7f + (2.7f * 2.0f * sliderPosProportional), radius);

	g.setColour(Colours::black.withAlpha(sliderPosProportional != 0.5 ? 0.3f : 0.15f));
	g.fillPath(track);
	g.setColour(Colour(0xFF111111).withAlpha(alpha));
	g.fillPath(thumb);
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

MacroKnobLookAndFeel::MacroKnobLookAndFeel()
{
	cachedImage_macroKnob_png = ImageProvider::getImage(ImageProvider::KnobEmpty);// ImageCache::getFromMemory(macroKnob_png, macroKnob_pngSize);
	cachedImage_macroKnob_ring_png = ImageProvider::getImage(ImageProvider::KnobUnmodulated);
}

Label* MacroKnobLookAndFeel::createSliderTextBox(Slider& slider)
{


	Label *label = new Label("Textbox");
	label->setFont (GLOBAL_FONT());
	label->setEditable (false, false, false);
	label->setColour (Label::textColourId, Colours::black);
		
	label->setJustificationType (Justification::centred);

	return label;
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

void BlackTextButtonLookAndFeel::drawButtonBackground(Graphics& g, Button& button, const Colour& colour,
	bool isMouseOverButton, bool isButtonDown)
{
		

	auto area = button.getLocalBounds().toFloat();

	if (button.getToggleState())
	{
		g.setColour(textColour);
		g.drawRoundedRectangle(area.reduced(1.0f), 4.0f, 1.0f);
	}

	float alpha = 0.2f;

	if (isButtonDown)
		alpha += 0.1f;

	if (isMouseOverButton)
		alpha += 0.1f;

	if (!button.isEnabled())
		alpha = 0.1f;
		

	g.setGradientFill(ColourGradient(Colours::white.withAlpha(alpha + 0.1f), 0.0f, 0.0f,
	                                 Colours::white.withAlpha(alpha), 0.0f, (float)button.getHeight(), false));

	g.fillRoundedRectangle(area, 4.0f);
}

void BlackTextButtonLookAndFeel::drawButtonText(Graphics& g, TextButton& b, bool cond, bool cond1)
{
	float alpha = 1.0f;

	if (!b.isEnabled())
		alpha = 0.5f;

	g.setColour(textColour.withMultipliedAlpha(alpha));
	g.setFont(f);
	g.drawText(b.getButtonText(), b.getLocalBounds().toFloat(), Justification::centred);
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

void PresetBrowserLookAndFeelMethods::drawColumnBackground(Graphics& g, Component& column, int columnIndex, Rectangle<int> listArea, const String& emptyText)
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

void PresetBrowserLookAndFeelMethods::drawTag(Graphics& g, Component& tagButton, bool hover, bool blinking, bool active, bool selected, const String& name, Rectangle<int> position)
{
    float alpha = active ? 0.4f : 0.1f;
    alpha += (blinking ? 0.2f : 0.0f);

    auto ar = position.toFloat().reduced(1.0f);

    g.setColour(highlightColour.withAlpha(alpha));
    g.fillRoundedRectangle(ar, 2.0f);
    g.drawRoundedRectangle(ar, 2.0f, 1.0f);
    g.setFont(getTagFont(tagButton));
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


void PresetBrowserLookAndFeelMethods::drawListItem(Graphics& g, Component& column, int columnIndex, int, const String& itemName, Rectangle<int> position, bool rowIsSelected, bool deleteMode, bool hover)
{
#if !HISE_NO_GUI_TOOLS
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
#endif
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

void BiPolarSliderLookAndFeel::drawLinearSlider(Graphics& g, int i, int i1, int width, int height, float x, float x1,
	float x2, const Slider::SliderStyle style, Slider& s)
{

	if (style == Slider::SliderStyle::LinearBarVertical)
	{
		const bool isBiPolar = s.getMinimum() < 0.0 && s.getMaximum() > 0.0;

		float leftY;
		float actualHeight;

		const float max = (float)s.getMaximum();
		const float min = (float)s.getMinimum();

		g.fillAll(s.findColour(Slider::ColourIds::backgroundColourId));

		if (isBiPolar)
		{
			const float value = (-1.0f * (float)s.getValue() - min) / (max - min);

			leftY = (value < 0.5f ? value * (float)(height) : 0.5f * (float)(height));
			actualHeight = fabs(0.5f - value) * (float)(height);
		}
		else
		{
			const double value = s.getValue();
			const double normalizedValue = (value - s.getMinimum()) / (s.getMaximum() - s.getMinimum());
			const double proportion = pow(normalizedValue, s.getSkewFactor());

			//const float value = ((float)s.getValue() - min) / (max - min);

			actualHeight = (float)proportion * (float)(height);
			leftY = (float)height - actualHeight ;
		}

		Colour c = s.findColour(Slider::ColourIds::thumbColourId);

		g.setGradientFill(ColourGradient(c.withMultipliedAlpha(s.isEnabled() ? 1.0f : 0.4f),
		                                 0.0f, 0.0f,
		                                 c.withMultipliedAlpha(s.isEnabled() ? 1.0f : 0.3f).withMultipliedBrightness(0.9f),
		                                 0.0f, (float)height,
		                                 false));

		g.fillRect(0.0f, leftY, (float)(width+1), actualHeight+1.0f);

		if (width > 4)
		{
			//g.setColour(Colours::white.withAlpha(0.3f));
			g.setColour(s.findColour(Slider::trackColourId));
			g.drawRect(0.0f, leftY, (float)(width + 1), actualHeight + 1.0f, 1.0f);
		}

			
	}
	else
	{
		const bool isBiPolar = s.getMinimum() < 0.0 && s.getMaximum() > 0.0;

		float leftX;
		float actualWidth;

		const float max = (float)s.getMaximum();
		const float min = (float)s.getMinimum();

		g.fillAll(Colour(0xfb333333));

		if (isBiPolar)
		{
			const float value = ((float)s.getValue() - min) / (max - min);

			leftX = 2.0f + (value < 0.5f ? value * (float)(width - 2) : 0.5f * (float)(width - 2));
			actualWidth = fabs(0.5f - value) * (float)(width - 2);
		}
		else
		{
			const double value = s.getValue();
			const double normalizedValue = (value - s.getMinimum()) / (s.getMaximum() - s.getMinimum());
			const double proportion = pow(normalizedValue, s.getSkewFactor());

			//const float value = ((float)s.getValue() - min) / (max - min);

			leftX = 2;
			actualWidth = (float)proportion * (float)(width - 2);
		}

		g.setGradientFill(ColourGradient(Colour(0xff888888).withAlpha(s.isEnabled() ? 0.8f : 0.4f),
		                                 0.0f, 0.0f,
		                                 Colour(0xff666666).withAlpha(s.isEnabled() ? 0.8f : 0.4f),
		                                 0.0f, (float)height,
		                                 false));
		g.fillRect(leftX, 2.0f, actualWidth, (float)(height - 2));
	}

		

}

void BiPolarSliderLookAndFeel::drawLinearSliderBackground(Graphics& graphics, int i, int i1, int i2, int i3, float x,
	float x1, float x2, const Slider::SliderStyle sliderStyle, Slider& slider)
{}

Font BiPolarSliderLookAndFeel::getLabelFont(Label& label)
{
	return GLOBAL_FONT();
}


void PresetBrowserLookAndFeelMethods::drawModalOverlay(Graphics& g, Component& modalWindow, Rectangle<int> area, Rectangle<int> labelArea, const String& title, const String& command)
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

void PresetBrowserLookAndFeelMethods::drawSearchBar(Graphics& g, Component& label, Rectangle<int> area)
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

AlertWindowLookAndFeel::AlertWindowLookAndFeel()
{
	dark = Colour(0xFF252525);
	bright = Colour(0xFFAAAAAA);

	setColour(PopupMenu::ColourIds::backgroundColourId, dark);
	setColour(PopupMenu::ColourIds::textColourId, bright);
	setColour(PopupMenu::ColourIds::highlightedBackgroundColourId, bright);
	setColour(PopupMenu::ColourIds::highlightedTextColourId, dark);
	setColour(PopupMenu::ColourIds::headerTextColourId, dark);
        
        
	setColour (TextEditor::highlightColourId, Colour (SIGNAL_COLOUR).withAlpha(0.5f));
	setColour (TextEditor::ColourIds::focusedOutlineColourId, Colour (SIGNAL_COLOUR));
                                  
}

Font AlertWindowLookAndFeel::getAlertWindowMessageFont()
{
	return GLOBAL_BOLD_FONT();
}

Font AlertWindowLookAndFeel::getAlertWindowTitleFont()
{
	if (HiseDeviceSimulator::isMobileDevice())
	{
		return GLOBAL_BOLD_FONT().withHeight(24.0f);
	}
	else
	{
		return GLOBAL_BOLD_FONT().withHeight(17.0f);
	}
}

Font AlertWindowLookAndFeel::getTextButtonFont(TextButton& textButton, int i)
{
	return getAlertWindowFont();
}

Font AlertWindowLookAndFeel::getComboBoxFont(ComboBox& comboBox)
{
	return GLOBAL_FONT();
}

Font AlertWindowLookAndFeel::getPopupMenuFont()
{
	return GLOBAL_BOLD_FONT();
}

void AlertWindowLookAndFeel::drawPopupMenuBackground(Graphics& g, int width, int height)
{
	g.setColour((findColour(PopupMenu::backgroundColourId)));
	g.fillRect(0.0f, 0.0f, (float)width, (float)height);
}

Font AlertWindowLookAndFeel::getAlertWindowFont()
{
	return GLOBAL_BOLD_FONT();
}

void AlertWindowLookAndFeel::setColourIdsForAlertWindow(AlertWindow& window)
{
	window.setColour(AlertWindow::ColourIds::backgroundColourId, dark);
	window.setColour(AlertWindow::ColourIds::textColourId, bright);
	window.setColour(AlertWindow::ColourIds::outlineColourId, bright);
}

void AlertWindowLookAndFeel::drawProgressBar(Graphics& g, ProgressBar& progressBar, int width, int height,
	double progress, const String& textToShow)
{
	const Colour background = bright;
	const Colour foreground = dark;

	g.fillAll (dark);

	if (progress >= 0.0f && progress < 1.0f)
	{
		ColourGradient grad(bright, 0.0f, 0.0f, bright.withAlpha(0.6f), 0.0f, (float)height, false);

		g.setColour(bright);

		g.drawRect(0, 0, width, height);

		int x = 2;
		int y = 2;
		int w = (int)((width - 4) * progress);
		int h = height - 4;

		g.setGradientFill(grad);

		g.fillRect(x,y,w,h);
	}
	else
	{
			
			
	}

	if (textToShow.isNotEmpty())
	{
		g.setColour (Colour::contrasting (background, foreground));
		g.setFont (GLOBAL_FONT());

		g.drawText (textToShow, 0, 0, width, height, Justification::centred, false);
	}

}
} // namespace hise
