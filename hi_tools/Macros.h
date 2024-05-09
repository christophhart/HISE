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

namespace juce
{
using ButtonListener = Button::Listener;
using SliderListener = Slider::Listener;
using ComboBoxListener = ComboBox::Listener;
using TextEditorListener = TextEditor::Listener;
using LabelListener = Label::Listener;
}

namespace hise { using namespace juce;


/** Change this value if you need more than 8 stereo channels in HISE routing. */
#ifndef NUM_MAX_CHANNELS
#define NUM_MAX_CHANNELS 32
#endif

#if NUM_MAX_CHANNELS % 2 != 0
#error "The channel amount must be a multiple of 2"
#endif

#define NUM_MIC_POSITIONS NUM_MAX_CHANNELS / 2

#ifndef NUM_POLYPHONIC_VOICES
#if HISE_IOS
#define NUM_POLYPHONIC_VOICES 128
#else
#define NUM_POLYPHONIC_VOICES 256
#endif
#endif

/** This can be used to change the threshold of what is considered "silence" in HISE.

    This value is used at different places to figure out whether to stop smoothing, stop a decaying envelope curve, etc.
 
    It tries to use a sensible default here, but you can override this (as a positive value, it will calculate the gain factor by taking the
    number as negative dB value and convert it to a gain factor).
*/
#ifndef HISE_SILENCE_THRESHOLD_DB
#define HISE_SILENCE_THRESHOLD_DB 60
#endif

/** This can be used to allow a frame processing context in scriptnode to use more than two channels.
 *  It's set to two channels as a default value because 99% of all projects will not use more channels, but if you use the frame container
 *	with more than two channels, bump this value to ensure it keeps working.
 */
#ifndef HISE_NUM_MAX_FRAME_CONTAINER_CHANNELS
#define HISE_NUM_MAX_FRAME_CONTAINER_CHANNELS 2
#endif

#if HI_RUN_UNIT_TESTS
#define jassert_skip_unit_test(x)
#else
#define jassert_skip_unit_test(x) jassert(x)
#endif

#ifndef HISE_EVENT_RASTER
#if FRONTEND_IS_PLUGIN
#define HISE_EVENT_RASTER 1 // Do not downsample the control rate for effect plugins
#else
#define HISE_EVENT_RASTER 8
#endif
#endif

#ifndef HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR
#define HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR HISE_EVENT_RASTER
#endif

/** If enabled, this will try to retain as much pitch modulation resolution as possible (it will still get downsampled to the control rate).
*/
#ifndef HISE_ENABLE_FULL_CONTROL_RATE_PITCH_MOD
#define HISE_ENABLE_FULL_CONTROL_RATE_PITCH_MOD 0
#endif

#if (HISE_EVENT_RASTER != 1)
#define HISE_USE_CONTROLRATE_DOWNSAMPLING 1
#else
#define HISE_USE_CONTROLRATE_DOWNSAMPLING 0
#endif

#if HI_ENABLE_EXPANSION_EDITING || USE_BACKEND
#define ENABLE_MARKDOWN true
#else
#define ENABLE_MARKDOWN false
#endif

#if (defined (_WIN32) || defined (_WIN64))
#define JUCE_WINDOWS 1
#endif

#define JUCE_LIVE_CONSTANT_OFF(x) x

#define SCALE_FACTOR() ((float)Desktop::getInstance().getDisplays().getMainDisplay().scale)

#if RETURN_IF_NO_THROW
#define RETURN_IF_NO_THROW(x) return x;
#define RETURN_VOID_IF_NO_THROW() return;
#endif

#if JUCE_DEBUG
#define DEBUG_ONLY(x) x
#define RETURN_DEBUG_ONLY(x) return x
#else
#define DEBUG_ONLY(x)
#define RETURN_DEBUG_ONLY(x)
#endif

#if JUCE_WINDOWS || JUCE_MAC || JUCE_IOS

#if HISE_NO_GUI_TOOLS

#define GLOBAL_FONT() (Font().withHeight(15.0f))
#define GLOBAL_BOLD_FONT() (Font().withHeight(15.0f))
#define GLOBAL_MONOSPACE_FONT() (Font().withHeight(15.0f))

#else

#if USE_LATO_AS_DEFAULT
static Typeface::Ptr oxygenBoldTypeFace = Typeface::createSystemTypefaceFor(HiBinaryData::FrontendBinaryData::LatoBold_ttf, HiBinaryData::FrontendBinaryData::LatoBold_ttfSize);
static Typeface::Ptr oxygenTypeFace = Typeface::createSystemTypefaceFor(HiBinaryData::FrontendBinaryData::LatoRegular_ttf, HiBinaryData::FrontendBinaryData::LatoRegular_ttfSize);
#else
static Typeface::Ptr oxygenBoldTypeFace = Typeface::createSystemTypefaceFor(HiBinaryData::FrontendBinaryData::oxygen_bold_ttf, HiBinaryData::FrontendBinaryData::oxygen_bold_ttfSize);
static Typeface::Ptr oxygenTypeFace = Typeface::createSystemTypefaceFor(HiBinaryData::FrontendBinaryData::oxygen_regular_ttf, HiBinaryData::FrontendBinaryData::oxygen_regular_ttfSize);
#endif




static Typeface::Ptr sourceCodeProTypeFace = Typeface::createSystemTypefaceFor(HiBinaryData::FrontendBinaryData::SourceCodeProRegular_otf, HiBinaryData::FrontendBinaryData::SourceCodeProRegular_otfSize);
static Typeface::Ptr sourceCodeProBoldTypeFace = Typeface::createSystemTypefaceFor(HiBinaryData::FrontendBinaryData::SourceCodeProBold_otf, HiBinaryData::FrontendBinaryData::SourceCodeProBold_otfSize);

#define GLOBAL_FONT() (Font(oxygenTypeFace).withHeight(13.0f))
#define GLOBAL_BOLD_FONT() (Font(oxygenBoldTypeFace).withHeight(14.0f))
    
#if JUCE_IOS
#define GLOBAL_MONOSPACE_FONT() Font(Font::getDefaultMonospacedFontName(), 14.0f, Font::plain)
#else
#if JUCE_WINDOWS
#define GLOBAL_MONOSPACE_FONT() (Font("Consolas", 14.0f, Font::plain))
#elif JUCE_MAC
#define GLOBAL_MONOSPACE_FONT() (Font("Menlo", 14.0f, Font::plain))
#else
#define GLOBAL_MONOSPACE_FONT() (Font(sourceCodeProTypeFace).withHeight(14.0f))
#endif
#endif
#endif

#elif JUCE_LINUX && !HISE_NO_GUI_TOOLS

class LinuxFontHandler
{
    public:

    LinuxFontHandler()
    {
#if USE_LATO_AS_DEFAULT
		Typeface::Ptr oxygenBoldTypeFace = Typeface::createSystemTypefaceFor(HiBinaryData::FrontendBinaryData::LatoBold_ttf, HiBinaryData::FrontendBinaryData::LatoBold_ttfSize);
		Typeface::Ptr oxygenTypeFace = Typeface::createSystemTypefaceFor(HiBinaryData::FrontendBinaryData::LatoRegular_ttf, HiBinaryData::FrontendBinaryData::LatoRegular_ttfSize);
#else
		Typeface::Ptr oxygenBoldTypeFace = Typeface::createSystemTypefaceFor(HiBinaryData::FrontendBinaryData::oxygen_bold_ttf, HiBinaryData::FrontendBinaryData::oxygen_bold_ttfSize);
		Typeface::Ptr oxygenTypeFace = Typeface::createSystemTypefaceFor(HiBinaryData::FrontendBinaryData::oxygen_regular_ttf, HiBinaryData::FrontendBinaryData::oxygen_regular_ttfSize);
#endif

        Typeface::Ptr sourceCodeProTypeFace = Typeface::createSystemTypefaceFor(HiBinaryData::FrontendBinaryData::SourceCodeProRegular_otf, HiBinaryData::FrontendBinaryData::SourceCodeProRegular_otfSize);
        Typeface::Ptr sourceCodeProBoldTypeFace = Typeface::createSystemTypefaceFor(HiBinaryData::FrontendBinaryData::SourceCodeProBold_otf, HiBinaryData::FrontendBinaryData::SourceCodeProBold_otfSize);

        globalFont = Font(oxygenTypeFace).withHeight(13.0f);
        globalBoldFont = Font(oxygenBoldTypeFace).withHeight(14.0f);
        monospaceFont = Font(sourceCodeProTypeFace).withHeight(14.0f);
        monospaceBoldFont = Font(sourceCodeProBoldTypeFace).withHeight(14.0f);
    }

    Font globalFont;
    Font globalBoldFont;
    Font monospaceFont;
    Font monospaceBoldFont;

    class Instance
    {
    public:

        Instance() {};

        Font getGlobalFont() {return data->globalFont;};
        Font getGlobalBoldFont() {return data->globalBoldFont;};
        Font getGlobalMonospaceFont() {return data->monospaceFont; }
        Font getGlobalMonospaceBoldFont() { return data->monospaceBoldFont; }

    private:

        SharedResourcePointer<LinuxFontHandler> data;
    };
};


#define GLOBAL_FONT() (LinuxFontHandler::Instance().getGlobalFont())
#define GLOBAL_BOLD_FONT() (LinuxFontHandler::Instance().getGlobalBoldFont())
#define GLOBAL_MONOSPACE_FONT() (LinuxFontHandler::Instance().getGlobalMonospaceFont())
#define GLOBAL_BOLD_MONOSPACE_FONT() (LinuxFontHandler::Instance().getGlobalMonospaceBoldFont())

#else

#define GLOBAL_FONT() (Font())
#define GLOBAL_BOLD_FONT() (Font())
#define GLOBAL_MONOSPACE_FONT() (Font())
#define GLOBAL_BOLD_MONOSPACE_FONT() (Font())

#endif

struct StringSanitizer
{
	static String get(const String& s)
	{
		auto p = s.removeCharacters("():,;?");

		if (!p.isEmpty() && p.endsWith("/"))
			p = p.upToLastOccurrenceOf("/", false, false);

		p = p.replace(".md", "");

		return p.replaceCharacter(' ', '-').toLowerCase();
	}
};

struct FontHelpers
{
	static Font getFontBoldened(const Font& f)
	{
		if (f.isBold())
			return f;

		if (f.getTypefaceName().startsWith("Oxygen"))
			return GLOBAL_BOLD_FONT().withHeight(f.getHeight());

		if (f.getTypefaceName().startsWith("Source"))
			return GLOBAL_MONOSPACE_FONT().withHeight(f.getHeight());

		return f.boldened();
	}

	static Font getFontNormalised(Font f)
	{
		if (f.getTypefaceName().startsWith("Oxygen"))
			return GLOBAL_FONT().withHeight(f.getHeight());

		if (f.getTypefaceName().startsWith("Source"))
			return GLOBAL_MONOSPACE_FONT().withHeight(f.getHeight());

		if (f.isBold() || f.isItalic())
		{
			f.setBold(false);
			f.setItalic(false);
			return f;
		}

		return f;
	}

	static Font getFontItalicised(Font f)
	{
		if (f.isItalic())
			return f;

		if (f.getTypefaceName().startsWith("Oxygen"))
			return GLOBAL_BOLD_FONT().withHeight(f.getHeight());

		if (f.getTypefaceName().startsWith("Source"))
			return GLOBAL_MONOSPACE_FONT().withHeight(f.getHeight());

		return f.italicised();
	}
};


#if ENABLE_MARKDOWN
#define MARKDOWN_CHAPTER(chapter) struct chapter {
#define START_MARKDOWN(name) static const String name() { String content; static const String nl = "\n";
#define ML(text) content << text << nl;
#define ML_START_CODE() ML("```javascript")
#define ML_END_CODE() ML("```")
#define END_MARKDOWN() return content; };
#define END_MARKDOWN_CHAPTER() };
#else
#define MARKDOWN_CHAPTER(chapter)
#define START_MARKDOWN(name) 
#define ML(text)
#define ML_START_CODE() 
#define ML_END_CODE() 
#define END_MARKDOWN()
#define END_MARKDOWN_CHAPTER()
#endif

#ifndef RETURN_STATIC_IDENTIFIER
#define RETURN_STATIC_IDENTIFIER(x) const static Identifier id_(x); return id_;
#endif

#define SET_GENERIC_PANEL_ID(x) static Identifier getGenericPanelId() { RETURN_STATIC_IDENTIFIER(x) }

#define SET_PANEL_NAME(x) static Identifier getPanelId() { RETURN_STATIC_IDENTIFIER(x) }; Identifier getIdentifierForBaseClass() const override { return getPanelId(); };
#define GET_PANEL_NAME(className) className::getPanelId()	

/** Override this and supply a custom implementation of HiseColourScheme::createAlertWindowLookAndFeel. */
#ifndef HISE_USE_CUSTOM_ALERTWINDOW_LOOKANDFEEL
#define HISE_USE_CUSTOM_ALERTWINDOW_LOOKANDFEEL 0
#endif

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
		ComponentBackgroundColour = 0xFF123532,
		ComponentFillTopColourId,
		ComponentFillBottomColourId,
		ComponentOutlineColourId,
		ComponentTextColourId,
		numColourIds
	};

	// Override this and return the look and feel to be used by all alert windows
	static LookAndFeel* createAlertWindowLookAndFeel(void* mainController);

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

	static void setDefaultColours(Component& c, bool recursive=false)
	{
		c.setColour(ComponentBackgroundColour, Colours::transparentBlack);
		c.setColour(ComponentFillTopColourId, Colour(0x66333333));
		c.setColour(ComponentFillBottomColourId, Colour(0xfb111111));
		c.setColour(ComponentOutlineColourId, Colours::white.withAlpha(0.3f));
		c.setColour(ComponentTextColourId, Colours::white);

		if (recursive)
		{
			for (int i = 0; i < c.getNumChildComponents(); i++)
				setDefaultColours(*c.getChildComponent(i), true);
		}
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

#ifndef HISE_OK_COLOUR
#define HISE_OK_COLOUR 0xFF4E8E35
#endif

#ifndef HISE_WARNING_COLOUR
#define HISE_WARNING_COLOUR 0xFFFFBA00
#endif

#ifndef HISE_ERROR_COLOUR
#define HISE_ERROR_COLOUR 0xFFBB3434
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

#define BIND_MEMBER_FUNCTION_0(x) std::bind(&x, this)
#define BIND_MEMBER_FUNCTION_1(x) std::bind(&x, this, std::placeholders::_1)
#define BIND_MEMBER_FUNCTION_2(x) std::bind(&x, this, std::placeholders::_1, std::placeholders::_2)
#define BIND_MEMBER_FUNCTION_3(x) std::bind(&x, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)

#ifndef jassertEqual
#if JUCE_DEBUG
#define jassertEqual(expr1, expr2) jassert(expr1 == expr2)
#else
#define jassertEqual(expr1, expr2) ignoreUnused(expr1, expr2)
#endif
#endif

/** This is set by the HISE projects to figure out the default VS version for the export. */
#ifndef HISE_USE_VS2022
#define HISE_USE_VS2022 0
#endif

/** This sets the global raster size for dragging components. */
#ifndef HI_RASTER_SIZE
#define HI_RASTER_SIZE 10
#endif


} // namespace hise
