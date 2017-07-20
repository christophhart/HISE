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
*   Commercial licenses for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licensing:
*
*   http://www.hartinstruments.net/hise/
*
*   HISE is based on the JUCE library,
*   which must be separately licensed for cloused source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

/******************************************************************************

BEGIN_JUCE_MODULE_DECLARATION

  ID:               hi_frontend
  vendor:           Hart Instruments
  version:          1.0
  name:             HISE Frontend Module
  description:      use this module for compiled plugins
  website:          http://hise.audio
  license:          GPL / Commercial

  dependencies:      juce_audio_basics, juce_audio_devices, juce_audio_formats, juce_audio_plugin_client, juce_audio_processors, juce_core, juce_cryptography, juce_data_structures, juce_events, juce_graphics, juce_gui_basics, juce_gui_extra, juce_opengl, hi_core, hi_modules

END_JUCE_MODULE_DECLARATION

******************************************************************************/

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

#if DONT_EMBED_FILES_IN_FRONTEND

#define CREATE_PLUGIN(deviceManager, callback) {ValueTree presetData = ValueTree::readFromData(PresetData::preset, PresetData::presetSize);\
	ValueTree externalFiles = PresetHandler::loadValueTreeFromData(PresetData::externalFiles, PresetData::externalFilesSize, true);\
	\
	FrontendProcessor* fp = new FrontendProcessor(presetData, deviceManager, callback, nullptr, nullptr, &externalFiles, nullptr);\
	AudioProcessorDriver::restoreSettings(fp);\
	GlobalSettingManager::restoreGlobalSettings(fp); \
	fp->loadSamplesAfterSetup();\
	return fp;\
}

#define CREATE_PLUGIN_WITH_AUDIO_FILES CREATE_PLUGIN // same same

#else
#define CREATE_PLUGIN(deviceManager, callback) {ValueTree presetData = ValueTree::readFromData(PresetData::preset, PresetData::presetSize);\
	ValueTree externalFiles = PresetHandler::loadValueTreeFromData(PresetData::externalFiles, PresetData::externalFilesSize, true);\
	\
	FrontendProcessor* fp = new FrontendProcessor(presetData, deviceManager, callback, nullptr, nullptr, &externalFiles, nullptr);\
	AudioProcessorDriver::restoreSettings(fp);\
	GlobalSettingManager::restoreGlobalSettings(fp); \
	fp->loadSamplesAfterSetup();\
	return fp;\
}

#define CREATE_PLUGIN_WITH_AUDIO_FILES(deviceManager, callback) {ValueTree presetData = ValueTree::readFromData(PresetData::preset, PresetData::presetSize);\
	ValueTree imageData = PresetHandler::loadValueTreeFromData(PresetData::images, PresetData::imagesSize, false);\
	ValueTree impulseData = PresetHandler::loadValueTreeFromData(PresetData::impulses, PresetData::impulsesSize, false); \
	ValueTree externalFiles = PresetHandler::loadValueTreeFromData(PresetData::externalFiles, PresetData::externalFilesSize, true);\
	\
	FrontendProcessor* fp = new FrontendProcessor(presetData, deviceManager, callback, &imageData, &impulseData, &externalFiles, nullptr);\
	AudioProcessorDriver::restoreSettings(fp);\
	GlobalSettingManager::restoreGlobalSettings(fp); \
	fp->loadSamplesAfterSetup();\
	return fp;\
}
#endif



#endif   // HI_FRONTEND_INCLUDED
