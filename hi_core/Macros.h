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

#ifndef MACROS_H_INCLUDED
#define MACROS_H_INCLUDED

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


#if USE_COPY_PROTECTION
#define CHECK_KEY(mainController){ if(FrontendProcessor* fp = dynamic_cast<FrontendProcessor*>(mainController)) fp->checkKey();}
#else
#define CHECK_KEY(mainController) {mainController;}
#endif

#define SCALE_FACTOR() ((float)Desktop::getInstance().getDisplays().getMainDisplay().scale)

#if USE_BACKEND
#define BACKEND_ONLY(x) x 
#else
#define BACKEND_ONLY(x)
#endif

#if USE_BACKEND
#define SET_CHANGED_FROM_PARENT_EDITOR() if(ProcessorEditor *editor = findParentComponentOfClass<ProcessorEditor>()) PresetHandler::setChanged(editor->getProcessor());
#else
#define SET_CHANGED_FROM_PARENT_EDITOR()
#endif


#if JUCE_WINDOWS || JUCE_MAC || JUCE_IOS

static juce::Typeface::Ptr oxygenBoldTypeFace = juce::Typeface::createSystemTypefaceFor(HiBinaryData::FrontendBinaryData::oxygen_bold_ttf, HiBinaryData::FrontendBinaryData::oxygen_bold_ttfSize);
static juce::Typeface::Ptr oxygenTypeFace = juce::Typeface::createSystemTypefaceFor(HiBinaryData::FrontendBinaryData::oxygen_regular_ttf, HiBinaryData::FrontendBinaryData::oxygen_regular_ttfSize);
static juce::Typeface::Ptr sourceCodeProTypeFace = juce::Typeface::createSystemTypefaceFor(HiBinaryData::FrontendBinaryData::SourceCodeProRegular_otf, HiBinaryData::FrontendBinaryData::SourceCodeProRegular_otfSize);
static juce::Typeface::Ptr sourceCodeProBoldTypeFace = juce::Typeface::createSystemTypefaceFor(HiBinaryData::FrontendBinaryData::SourceCodeProBold_otf, HiBinaryData::FrontendBinaryData::SourceCodeProBold_otfSize);

#define GLOBAL_FONT() (Font(oxygenTypeFace).withHeight(13.0f))
#define GLOBAL_BOLD_FONT() (Font(oxygenBoldTypeFace).withHeight(14.0f))
#define GLOBAL_MONOSPACE_FONT() (Font(sourceCodeProTypeFace).withHeight(14.0f))

#else

class LinuxFontHandler
{
    public:

    LinuxFontHandler()
    {
        juce::Typeface::Ptr oxygenBoldTypeFace = juce::Typeface::createSystemTypefaceFor(HiBinaryData::FrontendBinaryData::oxygen_bold_ttf, HiBinaryData::FrontendBinaryData::oxygen_bold_ttfSize);
        juce::Typeface::Ptr oxygenTypeFace = juce::Typeface::createSystemTypefaceFor(HiBinaryData::FrontendBinaryData::oxygen_regular_ttf, HiBinaryData::FrontendBinaryData::oxygen_regular_ttfSize);
        juce::Typeface::Ptr sourceCodeProTypeFace = juce::Typeface::createSystemTypefaceFor(HiBinaryData::FrontendBinaryData::SourceCodeProRegular_otf, HiBinaryData::FrontendBinaryData::SourceCodeProRegular_otfSize);
        juce::Typeface::Ptr sourceCodeProBoldTypeFace = juce::Typeface::createSystemTypefaceFor(HiBinaryData::FrontendBinaryData::SourceCodeProBold_otf, HiBinaryData::FrontendBinaryData::SourceCodeProBold_otfSize);

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

#define RETURN_STATIC_IDENTIFIER(name) static const Identifier id(name); return id;


#define GET_PANEL_NAME(classType) classType::getPanelId()
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

#define SIGNAL_COLOUR 0xFF90FFB1

#define DEBUG_BG_COLOUR 0xff636363



#include "copyProtectionMacros.h"


#endif  // MACROS_H_INCLUDED
