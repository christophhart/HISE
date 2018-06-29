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

#ifndef MACROS_H_INCLUDED
#define MACROS_H_INCLUDED

namespace hise { using namespace juce;

#if (defined (_WIN32) || defined (_WIN64))
#define JUCE_WINDOWS 1
#endif

#define JUCE_LIVE_CONSTANT_OFF(x) x

#if ENABLE_CONSOLE_OUTPUT
#define debugToConsole(p, x) (p->getMainController()->writeToConsole(x, 0, p))
#define debugError(p, x) (p->getMainController()->writeToConsole(x, 1, p))
#define debugMod(text) { if(consoleEnabled) debugProcessor(text); };
#else
#define debugToConsole(p, x) ignoreUnused(x)
#define debugError(p, x) ignoreUnused(x)
#define debugMod(text) ignoreUnused(text)
#endif

#define CONTAINER_WIDTH 900 - 32


#if USE_COPY_PROTECTION
#define CHECK_KEY(mainController){ if(FrontendProcessor* fp = dynamic_cast<FrontendProcessor*>(mainController)) fp->checkKey();}
#else
#define CHECK_KEY(mainController) {mainController;}
#endif

#define SCALE_FACTOR() ((float)Desktop::getInstance().getDisplays().getMainDisplay().scale)

#if JUCE_DEBUG || USE_FRONTEND || JUCE_MAC
#define RETURN_IF_NO_THROW(x) return x;
#define RETURN_VOID_IF_NO_THROW() return;
#else
#define RETURN_IF_NO_THROW(x)
#define RETURN_VOID_IF_NO_THROW()
#endif


#if USE_BACKEND
#ifndef HI_ENABLE_EXPANSION_EDITING
#define HI_ENABLE_EXPANSION_EDITING 1
#endif
#else
#ifndef HI_ENABLE_EXPANSION_EDITING
#define HI_ENABLE_EXPANSION_EDITING 0
#endif
#endif

#if USE_BACKEND
#define BACKEND_ONLY(x) x 
#define FRONTEND_ONLY(x)
#else
#define BACKEND_ONLY(x)
#define FRONTEND_ONLY(x) x
#endif



#if FRONTEND_IS_PLUGIN
#define FX_ONLY(x) x
#define INSTRUMENT_ONLY(x) 
#else
#define FX_ONLY(x)
#define INSTRUMENT_ONLY(x) x
#endif

#if USE_BACKEND
#define SET_CHANGED_FROM_PARENT_EDITOR() if(ProcessorEditor *editor = findParentComponentOfClass<ProcessorEditor>()) PresetHandler::setChanged(editor->getProcessor());
#else
#define SET_CHANGED_FROM_PARENT_EDITOR()
#endif

#ifdef LOG_SYNTH_EVENTS
#define LOG_SYNTH_EVENT(x) DBG(x)
#else
#define LOG_SYNTH_EVENT(x)
#endif

#if HI_RUN_UNIT_TESTS
#define jassert_skip_unit_test(x)
#else
#define jassert_skip_unit_test(x) jassert(x)
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



#define MARKDOWN_CHAPTER(chapter) namespace chapter {
#define START_MARKDOWN(name) static const String name() { String content; static const String nl = "\n";
#define ML(text) content << text << nl;
#define ML_START_CODE() ML("```javascript")
#define ML_END_CODE() ML("```")
#define END_MARKDOWN() return content; };
#define END_MARKDOWN_CHAPTER() }


#define HI_DECLARE_LISTENER_METHODS(x) public: \
	void addListener(x* l) { listeners.addIfNotAlreadyThere(l); }\
    void removeListener(x* l) { listeners.removeAllInstancesOf(l); }\
	private:\
	Array<WeakReference<x>> listeners;

#define RETURN_STATIC_IDENTIFIER(name) static const Identifier id(name); return id;


#define SET_GENERIC_PANEL_ID(x) static Identifier getGenericPanelId() { RETURN_STATIC_IDENTIFIER(x) }

#define GET_PROJECT_HANDLER(x)(x->getMainController()->getSampleManager().getProjectHandler())

#define SET_PANEL_NAME(x) static Identifier getPanelId() { RETURN_STATIC_IDENTIFIER(x) }; Identifier getIdentifierForBaseClass() const override { return getPanelId(); };
#define GET_PANEL_NAME(className) className::getPanelId()	

#define GET_PROCESSOR_TYPE_ID(ProcessorClass) Identifier getProcessorTypeId() const override { return ProcessorClass::getConnectorId(); }
#define SET_PROCESSOR_CONNECTOR_TYPE_ID(name) static Identifier getConnectorId() { RETURN_STATIC_IDENTIFIER(name) };

#define loadTable(tableVariableName, nameAsString) { const var savedData = v.getProperty(nameAsString, var()); tableVariableName->restoreData(savedData); }
#define saveTable(tableVariableName, nameAsString) ( v.setProperty(nameAsString, tableVariableName->exportData(), nullptr) )

#if JUCE_DEBUG
#define START_TIMER() (startTimer(150))
#define IGNORE_UNUSED_IN_RELEASE(x) 
#else
#define START_TIMER() (startTimer(30))
#define IGNORE_UNUSED_IN_RELEASE(x) (ignoreUnused(x))
#endif

#define FLOAT_RECTANGLE(r) Rectangle<float>((float)r.getX(), (float)r.getY(), (float)r.getWidth(), (float)r.getHeight())
#define INT_RECTANGLE(r) Rectangle<int>((int)r.getX(), (int)r.getY(), (int)r.getWidth(), (int)r.getHeight())

#define CONSTRAIN_TO_0_1(x)(jlimit<float>(0.0f, 1.0f, x))

#define RETURN_WHEN_X_BUTTON() if (e.mods.isX1ButtonDown() || e.mods.isX2ButtonDown()) return;

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

#define DEBUG_BG_COLOUR 0xff636363


#if HISE_IOS
#define SET_IMAGE_RESAMPLING_QUALITY() g.setImageResamplingQuality (Graphics::ResamplingQuality::lowResamplingQuality);
#else
#define SET_IMAGE_RESAMPLING_QUALITY()
#endif

#include "copyProtectionMacros.h"

} // namespace hise

#endif  // MACROS_H_INCLUDED
