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
*   along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
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

#ifndef HI_FRONTEND_INCLUDED
#define HI_FRONTEND_INCLUDED



#include "../hi_modules/hi_modules.h"

#if 0
#if USE_FRONTEND_STATIC_LIB

extern char* getPluginName();
#define JucePlugin_Name getPluginName()

extern char* getVersionString();
#define JucePlugin_VersionString getVersionString()

extern char* getManufacturer();
#define JucePlugin_Manufacturer getManufacturer()

AudioProcessor* createPlugin(ValueTree &presetData, ValueTree &imageData, ValueTree &externalScripts, ValueTree &userPresets);

#endif
#endif


#ifndef USE_FRONTEND
#define USE_FRONTEND 1
#endif

using namespace juce;

#include "frontend/FrontEndProcessor.h"
#include "frontend/FrontendProcessorEditor.h"

#define USER_PRESET_OFFSET 8192

#define CREATE_PLUGIN {ValueTree presetData = ValueTree::readFromData(PresetData::preset, PresetData::presetSize);\
	ValueTree imageData = PresetHandler::loadValueTreeFromData(PresetData::images, PresetData::imagesSize, false);\
	ValueTree externalFiles = PresetHandler::loadValueTreeFromData(PresetData::externalFiles, PresetData::externalFilesSize, true);\
	ValueTree userPresets = PresetHandler::loadValueTreeFromData(PresetData::userPresets, PresetData::userPresetsSize, false);\
	\
	return new FrontendProcessor(presetData, &imageData, nullptr, &externalFiles, &userPresets);\
}

#define CREATE_PLUGIN_WITH_AUDIO_FILES {ValueTree presetData = ValueTree::readFromData(PresetData::preset, PresetData::presetSize);\
	ValueTree imageData = PresetHandler::loadValueTreeFromData(PresetData::images, PresetData::imagesSize, false);\
	ValueTree impulseData = PresetHandler::loadValueTreeFromData(PresetData::impulses, PresetData::impulsesSize, true); \
	ValueTree externalFiles = PresetHandler::loadValueTreeFromData(PresetData::externalFiles, PresetData::externalFilesSize, true);\
	ValueTree userPresets =PresetHandler::loadValueTreeFromData(PresetData::userPresets, PresetData::userPresetsSize, false);\
	\
	return new FrontendProcessor(presetData, &imageData, &impulseData, &externalFiles, &userPresets);\
}


#endif   // HI_FRONTEND_INCLUDED
