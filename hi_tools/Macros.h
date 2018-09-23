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

namespace hise { using namespace juce;

#define NUM_MAX_CHANNELS 16

#ifndef NUM_POLYPHONIC_VOICES
#if HISE_IOS
#define NUM_POLYPHONIC_VOICES 128
#else
#define NUM_POLYPHONIC_VOICES 256
#endif
#endif


#if HI_RUN_UNIT_TESTS
#define jassert_skip_unit_test(x)
#else
#define jassert_skip_unit_test(x) jassert(x)
#endif

#ifndef HISE_EVENT_RASTER
#define HISE_EVENT_RASTER 8
#endif

#ifndef HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR
#define HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR HISE_EVENT_RASTER
#endif


#if USE_FRONTEND
#define ENABLE_MARKDOWN false
#else
#define ENABLE_MARKDOWN true
#endif

#if (defined (_WIN32) || defined (_WIN64))
#define JUCE_WINDOWS 1
#endif

#define JUCE_LIVE_CONSTANT_OFF(x) x

#define SCALE_FACTOR() ((float)Desktop::getInstance().getDisplays().getMainDisplay().scale)

#if JUCE_DEBUG || USE_FRONTEND || JUCE_MAC
#define RETURN_IF_NO_THROW(x) return x;
#define RETURN_VOID_IF_NO_THROW() return;
#else
#define RETURN_IF_NO_THROW(x)
#define RETURN_VOID_IF_NO_THROW()
#endif

#if JUCE_WINDOWS || JUCE_MAC || JUCE_IOS

static Typeface::Ptr oxygenBoldTypeFace = Typeface::createSystemTypefaceFor(HiBinaryData::FrontendBinaryData::oxygen_bold_ttf, HiBinaryData::FrontendBinaryData::oxygen_bold_ttfSize);
static Typeface::Ptr oxygenTypeFace = Typeface::createSystemTypefaceFor(HiBinaryData::FrontendBinaryData::oxygen_regular_ttf, HiBinaryData::FrontendBinaryData::oxygen_regular_ttfSize);
static Typeface::Ptr sourceCodeProTypeFace = Typeface::createSystemTypefaceFor(HiBinaryData::FrontendBinaryData::SourceCodeProRegular_otf, HiBinaryData::FrontendBinaryData::SourceCodeProRegular_otfSize);
static Typeface::Ptr sourceCodeProBoldTypeFace = Typeface::createSystemTypefaceFor(HiBinaryData::FrontendBinaryData::SourceCodeProBold_otf, HiBinaryData::FrontendBinaryData::SourceCodeProBold_otfSize);

#define GLOBAL_FONT() (Font(oxygenTypeFace).withHeight(13.0f))
#define GLOBAL_BOLD_FONT() (Font(oxygenBoldTypeFace).withHeight(14.0f))
#define GLOBAL_MONOSPACE_FONT() (Font(sourceCodeProTypeFace).withHeight(14.0f))

#else

class LinuxFontHandler
{
    public:

    LinuxFontHandler()
    {
        Typeface::Ptr oxygenBoldTypeFace = Typeface::createSystemTypefaceFor(HiBinaryData::FrontendBinaryData::oxygen_bold_ttf, HiBinaryData::FrontendBinaryData::oxygen_bold_ttfSize);
        Typeface::Ptr oxygenTypeFace = Typeface::createSystemTypefaceFor(HiBinaryData::FrontendBinaryData::oxygen_regular_ttf, HiBinaryData::FrontendBinaryData::oxygen_regular_ttfSize);
        Typeface::Ptr sourceCodeProTypeFace = Typeface::createSystemTypefaceFor(HiBinaryData::FrontendBinaryData::SourceCodeProRegular_otf, HiBinaryData::FrontendBinaryData::SourceCodeProRegular_otfSize);
        Typeface::Ptr sourceCodeProBoldTypeFace = Typeface::createSystemTypefaceFor(HiBinaryData::FrontendBinaryData::SourceCodeProBold_otf, HiBinaryData::FrontendBinaryData::SourceCodeProBold_otfSize);

        globalFont = Font(oxygenTypeFace).withHeight(13.0f);
        globalBoldFont = Font(oxygenBoldTypeFace).withHeight(14.0f);
        monospaceFont = Font(sourceCodeProTypeFace).withHeight(14.0f);
    }

    

    Font globalFont;
    Font globalBoldFont;
    Font monospaceFont;

    class Instance
    {
    public:

        Instance() {};

        Font getGlobalFont() {return data->globalFont;};
        Font getGlobalBoldFont() {return data->globalBoldFont;};
        Font getGlobalMonospaceFont() {return data->monospaceFont; }

    private:

        SharedResourcePointer<LinuxFontHandler> data;
    };
};

#define GLOBAL_FONT() (LinuxFontHandler::Instance().getGlobalFont())
#define GLOBAL_BOLD_FONT() (LinuxFontHandler::Instance().getGlobalBoldFont())
#define GLOBAL_MONOSPACE_FONT() (LinuxFontHandler::Instance().getGlobalMonospaceFont())

#endif


#if ENABLE_MARKDOWN
#define MARKDOWN_CHAPTER(chapter) namespace chapter {
#define START_MARKDOWN(name) static const String name() { String content; static const String nl = "\n";
#define ML(text) content << text << nl;
#define ML_START_CODE() ML("```javascript")
#define ML_END_CODE() ML("```")
#define END_MARKDOWN() return content; };
#define END_MARKDOWN_CHAPTER() }
#else
#define MARKDOWN_CHAPTER(chapter)
#define START_MARKDOWN(name) 
#define ML(text)
#define ML_START_CODE() 
#define ML_END_CODE() 
#define END_MARKDOWN()
#define END_MARKDOWN_CHAPTER()
#endif

#define RETURN_STATIC_IDENTIFIER(name) static const Identifier id(name); return id;

#define SET_GENERIC_PANEL_ID(x) static Identifier getGenericPanelId() { RETURN_STATIC_IDENTIFIER(x) }

#define SET_PANEL_NAME(x) static Identifier getPanelId() { RETURN_STATIC_IDENTIFIER(x) }; Identifier getIdentifierForBaseClass() const override { return getPanelId(); };
#define GET_PANEL_NAME(className) className::getPanelId()	


struct HiseColourScheme
{
	enum Scheme
	{
		Dark,
		Bright,
		numSchemes
	};

	enum ColourIds
	{
		EditorBackgroundColourId,
		EditorBackgroundColourIdBright,
		ModulatorSynthBackgroundColourId,
		DebugAreaBackgroundColourId,
		ModulatorSynthHeader,
		WidgetBackgroundColour = 0xFF123532,
		WidgetFillTopColourId,
		WidgetFillBottomColourId,
		WidgetOutlineColourId,
		WidgetTextColourId,
		numColourIds
	};

	static Colour getColour(ColourIds id)
	{
		switch (id)
		{
		case HiseColourScheme::EditorBackgroundColourId:
		{
			switch (currentColourScheme)
			{
			case HiseColourScheme::Dark:
				return JUCE_LIVE_CONSTANT_OFF(Colour(0xff515151));
			case HiseColourScheme::Bright:
				return JUCE_LIVE_CONSTANT_OFF(Colour(0xff898989));
			case HiseColourScheme::numSchemes:
				break;
			}
		}
		case HiseColourScheme::EditorBackgroundColourIdBright:
		{
			switch (currentColourScheme)
			{
			case HiseColourScheme::Dark:
				return JUCE_LIVE_CONSTANT_OFF(Colour(0xFF666666));
			case HiseColourScheme::Bright:
				return JUCE_LIVE_CONSTANT_OFF(Colour(0xFF666666));
			case HiseColourScheme::numSchemes:
				break;
			}
		}
		case HiseColourScheme::ModulatorSynthBackgroundColourId:
		{
			switch (currentColourScheme)
			{
			case HiseColourScheme::Dark:
				return JUCE_LIVE_CONSTANT_OFF(Colour(0xff414141));
			case HiseColourScheme::Bright:
				return JUCE_LIVE_CONSTANT_OFF(Colour(0xff5e5e5e));
			case HiseColourScheme::numSchemes:
				break;
			}
		}

		case HiseColourScheme::DebugAreaBackgroundColourId:
		{
			switch (currentColourScheme)
			{
			case HiseColourScheme::Dark:
				return JUCE_LIVE_CONSTANT_OFF(Colour(0xFF3D3D3D));
			case HiseColourScheme::Bright:
				return JUCE_LIVE_CONSTANT_OFF(Colour(0xff5d5d5d));
			case HiseColourScheme::numSchemes:
				break;
			}
		}

		case HiseColourScheme::ModulatorSynthHeader:
		{
			switch (currentColourScheme)
			{
			case HiseColourScheme::Dark:
				return JUCE_LIVE_CONSTANT_OFF(Colour(0xFFEEEEEE));
			case HiseColourScheme::Bright:
				return JUCE_LIVE_CONSTANT_OFF(Colour(0xFFEEEEEE));
			case HiseColourScheme::numSchemes:
				break;
			}
		}

		default:
			break;
		}

		jassertfalse;
		return Colours::transparentBlack;
	}

	static void setColourScheme(Scheme s)
	{
		currentColourScheme = s;
	}

	static Scheme getCurrentColourScheme()
	{
		return currentColourScheme;
	}

private:


	static Scheme currentColourScheme;
};


#define DEBUG_AREA_BACKGROUND_COLOUR_DARK 0x03000000
#define BACKEND_BG_COLOUR 0xFF888888//0xff4d4d4d
#define BACKEND_BG_COLOUR_BRIGHT 0xFF646464

#define BACKEND_ICON_COLOUR_ON 0xCCFFFFFF
#define BACKEND_ICON_COLOUR_OFF 0xFF333333


#ifndef SIGNAL_COLOUR
#define SIGNAL_COLOUR 0xFF90FFB1
#endif

#define FLOAT_RECTANGLE(r) r.toFloat();
#define INT_RECTANGLE(r) r.toInt();

#define CONSTRAIN_TO_0_1(x)(jlimit<float>(0.0f, 1.0f, x))

#define DEBUG_AREA_BACKGROUND_COLOUR_DARK 0x03000000
#define BACKEND_BG_COLOUR 0xFF888888//0xff4d4d4d
#define BACKEND_BG_COLOUR_BRIGHT 0xFF646464

#define BACKEND_ICON_COLOUR_ON 0xCCFFFFFF
#define BACKEND_ICON_COLOUR_OFF 0xFF333333

#define DEBUG_BG_COLOUR 0xff636363

#if HISE_IOS
#define SET_IMAGE_RESAMPLING_QUALITY() g.setImageResamplingQuality (Graphics::ResamplingQuality::lowResamplingQuality);
#else
#define SET_IMAGE_RESAMPLING_QUALITY()
#endif

} // namespace hise
