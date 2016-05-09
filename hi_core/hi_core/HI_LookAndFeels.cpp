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
*   Commercial licences for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licencing:
*
*   http://www.hartinstruments.net/hise/
*
*   HISE is based on the JUCE library,
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

void FilmstripLookAndFeel::drawToggleButton(Graphics &g, ToggleButton &b, bool isMouseOverButton, bool isButtonDown)
{
	if (!imageToUse.isValid() || numStrips != 2)
	{
		KnobLookAndFeel::drawToggleButton(g, b, isMouseOverButton, isButtonDown);
		return;
	}
	else
	{
		const int stripIndex = b.getToggleState() ? 1 : 0;

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

		g.drawImageAt(clip, 0, 0);
		
	}
}

void FilmstripLookAndFeel::drawRotarySlider(Graphics &g, int /*x*/, int /*y*/, int width, int height, float /*sliderPosProportional*/, float /*rotaryStartAngle*/, float /*rotaryEndAngle*/, Slider &s)
{
	if (!imageToUse.isValid() || numStrips == 0)
	{
		KnobLookAndFeel::drawRotarySlider(g, -1, -1, width, height, -1, -1, -1, s);
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

		g.drawImageAt(clip, 0, 0);
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

void KnobLookAndFeel::drawHiBackground(Graphics &g, int x, int y, int width, int height, Component *c, bool isMouseOverButton)
{
    
    
    
    Colour upperBgColour = (c != nullptr) ? c->findColour(MacroControlledObject::HiBackgroundColours::upperBgColour) :
                                            Colour(0x66333333);
    
    Colour lowerBgColour = (c != nullptr) ? c->findColour(MacroControlledObject::HiBackgroundColours::lowerBgColour) :
                                            Colour(0xfb111111);
    
    g.setGradientFill(ColourGradient(upperBgColour.withMultipliedBrightness(isMouseOverButton ? 1.6f : 1.1f),
                                     64.0f, 8.0f,
                                     lowerBgColour.withMultipliedBrightness(isMouseOverButton ? 1.9f : 1.0f),
                                     64.0f, (float)(height+32),
                                     false));
    
    g.fillRect ((float)x, (float)y, (float)width, (float)height);
    
    Colour outlineColour = (c != nullptr) ? c->findColour(MacroControlledObject::HiBackgroundColours::outlineBgColour) :
    Colours::white.withAlpha(0.3f);
    
    g.setColour (outlineColour);
    g.drawRect((float)x, (float)y, (float)width, (float)height, 1.0f);
}

void KnobLookAndFeel::drawComboBox(Graphics &g, int width, int height, bool isButtonDown, int /*buttonX*/, int /*buttonY*/, int /*buttonW*/, int /*buttonH*/, ComboBox &c)
{
	c.setColour(ComboBox::ColourIds::textColourId, Colours::white);

	drawHiBackground(g, 2, 2, width - 4, height - 4, dynamic_cast<ComboBox*>(&c), isButtonDown);

	static const unsigned char pathData[] = { 110, 109, 0, 0, 130, 67, 92, 174, 193, 67, 108, 0, 0, 140, 67, 92, 174, 193, 67, 108, 0, 0, 135, 67, 92, 174, 198, 67, 99, 109, 0, 0, 130, 67, 92, 46, 191, 67, 108, 0, 0, 140, 67, 92, 46, 191, 67, 108, 0, 0, 135, 67, 92, 46, 186, 67, 99, 101, 0, 0 };

	Path path;
	path.loadPathFromData(pathData, sizeof(pathData));

	path.scaleToFit((float)width - 20.0f, (float)(height - 12) * 0.5f, 12.0f, 12.0f, true);

	g.setColour(Colours::white.withAlpha(0.8f));
	g.fillPath(path);
}

void KnobLookAndFeel::drawRotarySlider(Graphics &g, int /*x*/, int /*y*/, int width, int height, float /*sliderPosProportional*/, float /*rotaryStartAngle*/, float /*rotaryEndAngle*/, Slider &s)
{
	s.setTextBoxStyle (Slider::TextBoxRight, false, 80, 24);

			if(!s.isEnabled()) g.setOpacity(0.4f);


	height = s.getHeight();
	width = s.getWidth();

	drawHiBackground(g, 12, 6, width-18, 32, dynamic_cast<HiSlider*>(&s));
		
	const double value = s.getValue();
    const double normalizedValue = (value - s.getMinimum()) / (s.getMaximum() - s.getMinimum());
	const double proportion = pow(normalizedValue, s.getSkewFactor());
	const int stripIndex = (int) (proportion * 127);

	const int filmstripHeight = cachedImage_smalliKnob_png.getHeight() / 128;

    const int offset = stripIndex * filmstripHeight;

	Image clip = cachedImage_smalliKnob_png.getClippedImage(Rectangle<int>(0, offset, filmstripHeight, filmstripHeight));

    g.setColour (Colours::black.withAlpha(s.isEnabled() ? 1.0f : 0.5f));
    g.drawImage (clip, 0, 0, 48, 48, 0, 0, filmstripHeight, filmstripHeight); 

	float displayValue = 1.0f;

	Image *imageToUse = &cachedImage_knobRing_png;

	if(dynamic_cast<HiSlider*>(&s) != nullptr)
	{
		

		HiSlider *hi = dynamic_cast<HiSlider*>(&s);

		displayValue = hi->getDisplayValue();

		imageToUse = hi->isUsingModulatedRing() ? &ring_modulated : &cachedImage_knobRing_png;
		
	}

	Image clipRing = imageToUse->getClippedImage(Rectangle<int>(0, (int)(stripIndex * displayValue) * filmstripHeight, filmstripHeight, filmstripHeight));
	
    g.setColour (Colours::black.withAlpha(s.isEnabled() ? 1.0f : 0.5f));
	g.drawImage(clipRing, 0, 0, 48, 48, 0, 0, filmstripHeight, filmstripHeight); 


	g.setColour(Colours::white.withAlpha(0.7f));
	g.setFont (GLOBAL_BOLD_FONT());
	g.drawText (s.getName(),
				41, 10, (int)((0.5391f) * width) + 10, 12,
				Justification::centred, true);


	if(s.getComponentEffect() != nullptr)
	{
		s.getComponentEffect()->applyEffect(clip, g, 1.0f, 1.0f);
	}

}


void KnobLookAndFeel::drawToggleButton (Graphics &g, ToggleButton &b, bool isMouseOverButton, bool /*isButtonDown*/)
{
	drawHiBackground(g, 6, 4, b.getWidth()-12, b.getHeight() - 8, dynamic_cast<HiToggleButton*>(&b), isMouseOverButton);

	const int filmStripHeight = cachedImage_toggle_png.getHeight() / 2;

	Image clip = Image(cachedImage_toggle_png.getClippedImage(Rectangle<int>(0, b.getToggleState() ? filmStripHeight: 0, filmStripHeight, filmStripHeight)));

	g.setColour (b.getToggleState() ? Colours::white.withAlpha(0.9f) : Colours::white.withAlpha(0.4f));
	g.setFont (GLOBAL_FONT());

	String text = b.getButtonText();

	g.drawText (text,
				30, 6, b.getWidth() - 36, b.getHeight() - 12,
				Justification::centredLeft, true);
		
	g.setColour (Colours::black.withAlpha( (b.isEnabled() ? 1.0f : 0.5f) ));
	g.drawImage(clip, 12, 8, 16, 16, 0, 0, filmStripHeight, filmStripHeight);
}


KnobLookAndFeel::KnobLookAndFeel()
{
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

	setColour(PopupMenu::highlightedBackgroundColourId, Colour(0xff680000));

	Colour dark(0xFF252525);

	Colour bright(0xFF999999);

	setColour(PopupMenu::ColourIds::backgroundColourId, dark);
	setColour(PopupMenu::ColourIds::textColourId, bright);
	setColour(PopupMenu::ColourIds::highlightedBackgroundColourId, bright);
	setColour(PopupMenu::ColourIds::highlightedTextColourId, dark);
}


const char* KnobLookAndFeel::smalliKnob_png =  (const char*) HiBinaryData::LookAndFeelBinaryData::knob_mod_bg_png;
const int KnobLookAndFeel::smalliKnob_pngSize = 16277;

const char* KnobLookAndFeel::knobRing_png =  (const char*) HiBinaryData::LookAndFeelBinaryData::knob_mod_ring_png;
const int KnobLookAndFeel::knobRing_size = 17983;

const char* KnobLookAndFeel::toggle_png = (const char*) HiBinaryData::LookAndFeelBinaryData::resource_Background_toggle_png;
const int KnobLookAndFeel::toggle_pngSize = 1758;

const char* KnobLookAndFeel::slider_strip2_png = (const char*) HiBinaryData::LookAndFeelBinaryData::resource_Background_slider_strip2_png;
const int KnobLookAndFeel::slider_strip2_pngSize = 97907;

const char* KnobLookAndFeel::slider2_bipolar_png = (const char*) HiBinaryData::LookAndFeelBinaryData::resource_Background_slider2_bipolar_png;
const int KnobLookAndFeel::slider2_bipolar_pngSize = 87929;

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


	g.setColour(Colours::white.withAlpha(0.3f));

	g.setFont(GLOBAL_BOLD_FONT());
	g.drawText(String((int)value), 0, 0, 48, 48, Justification::centred, false);


	
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
	switch (type)
	{
	case ImageProvider::KnobEmpty:

		return ImageCache::getFromMemory(BinaryData::knobEmpty_200_png, BinaryData::knobEmpty_200_pngSize);


	case ImageProvider::KnobUnmodulated:

		return ImageCache::getFromMemory(BinaryData::knobUnmodulated_200_png, BinaryData::knobUnmodulated_200_pngSize);

	case ImageProvider::KnobModulated:

		return ImageCache::getFromMemory(BinaryData::knobModulated_200_png, BinaryData::knobModulated_200_pngSize);

	case ImageProvider::MacroKnob:
		

	case ImageProvider::BalanceKnob:

	case ImageProvider::ImageType::ToggleButton:

		return ImageCache::getFromMemory(BinaryData::toggle_200_png, BinaryData::toggle_200_pngSize);

	default:
		jassertfalse;
		return Image();
		break;
	}
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

void ProcessorEditorLookAndFeel::drawNoiseBackground(Graphics &g, Rectangle<int> area, Colour c)
{
    g.setOpacity(JUCE_LIVE_CONSTANT(0.4f));
    for(int i = area.getY(); i < area.getBottom(); i+=400)
    {
        for(int j = area.getX(); j < area.getRight(); j+=400)
        {
            //g.drawImageAt(ImageCache::getFromMemory(BinaryData::noise_png, BinaryData::noise_pngSize), j,i);
        }
    }
    
    g.setOpacity(1.0f);
}

void FileBrowserLookAndFeel::drawFileBrowserRow(Graphics&g, int width, int height, const String& filename, Image* icon, const String& fileSizeDescription, const String& fileTimeDescription, bool isDirectory, bool isItemSelected, int /*itemIndex*/, DirectoryContentsDisplayComponent& dcc)
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

void ConcertinaPanelHeaderLookAndFeel::drawConcertinaPanelHeader(Graphics& g, const Rectangle<int>& area, bool isMouseOver, bool /*isMouseDown*/, ConcertinaPanel& /*cp*/, Component& panel)
{
#if USE_BACKEND
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
	else if (dynamic_cast<ExternalFileTable<Image>*>(&panel))
	{
		path.loadPathFromData(BackendBinaryData::ToolbarIcons::imageTable, sizeof(BackendBinaryData::ToolbarIcons::imageTable));
	}
	else if (dynamic_cast<ExternalFileTable<AudioSampleBuffer>*>(&panel))
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

    const float lowAlpha = 0.25f;
    
    g.setGradientFill(ColourGradient(Colours::black.withAlpha(isMouseOver ? (lowAlpha - 0.1f) : lowAlpha), 0, (float)area.getY(),
                                     Colours::black.withAlpha(lowAlpha + 0.1f), 0, (float)area.getBottom(), false));
    
	
	g.fillAll();

	g.setColour(Colours::black.withAlpha(0.4f));

	g.drawRect(area, 1);

	g.fillRoundedRectangle(0.0f, 0.0f, (float)area.getWidth(), 60.0f, 5.0f);

	//g.fillAll();
    
    g.setColour(Colours::white);
    g.setFont(GLOBAL_BOLD_FONT());
    
    g.drawFittedText(panel.getName(), area.getHeight(), 0, area.getWidth() - 6, area.getHeight(), Justification::centredLeft, 1);
    
	path.scaleToFit(3.0f, 3.0f, (float)area.getHeight() - 6.0f, (float)area.getHeight() - 6.0f, true);
	g.setColour(Colours::white);
	g.fillPath(path);
    
#else

	ignoreUnused(g, area, isMouseOver, panel);

#endif

}

void PopupLookAndFeel::drawMenuBarBackground(Graphics& g, int width, int height, bool, MenuBarComponent& /*menuBar*/)
{
	const Colour colour = Colour(BACKEND_BG_COLOUR_BRIGHT);

	Rectangle<int> r(width, height);

	g.setColour(colour);
	g.fillRect(r.removeFromTop(1));
	g.fillRect(r.removeFromBottom(1));

	g.setGradientFill(ColourGradient(colour, 0, 0, colour.darker(0.13f), 0, (float)height, false));
	g.fillRect(r);

	g.setColour(Colour(0xFF666666));

	g.drawLine(0.0f, (float)height, (float)width, (float)height);
}

FrontendKnobLookAndFeel::FrontendKnobLookAndFeel()
{
	volumeFilmStrip = ImageCache::getFromMemory(BinaryData::FrontendKnob_Unipolar_png, BinaryData::FrontendKnob_Unipolar_pngSize);
	balanceFilmStrip = ImageCache::getFromMemory(BinaryData::FrontendKnob_Bipolar_png, BinaryData::FrontendKnob_Bipolar_pngSize);
}

void FrontendKnobLookAndFeel::drawRotarySlider(Graphics &g, int /*x*/, int /*y*/, int width, int height, float /*sliderPosProportional*/, float /*rotaryStartAngle*/, float /*rotaryEndAngle*/, Slider &s)
{
	const double value = s.getValue();
	const double normalizedValue = (value - s.getMinimum()) / (s.getMaximum() - s.getMinimum());
	const double proportion = pow(normalizedValue, s.getSkewFactor());
	const int stripIndex = (int)(proportion * 126);

	
	const int filmstripHeight = 22;

	const int offset = stripIndex * filmstripHeight;

	Image *imageToUse;
	
	if (s.getName() == "Volume")
	{
		imageToUse = &volumeFilmStrip;
	}
	else 
	{
		imageToUse = &balanceFilmStrip;
	}

	Image clip = imageToUse->getClippedImage(Rectangle<int>(0, offset, filmstripHeight, filmstripHeight));

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

	
	g.drawImageAt(clip, 0, 0);
}
