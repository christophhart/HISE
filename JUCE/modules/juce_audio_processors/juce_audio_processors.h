/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2020 - Raw Material Software Limited

   JUCE is an open source library subject to commercial or open-source
   licensing.

   By using JUCE, you agree to the terms of both the JUCE 6 End-User License
   Agreement and JUCE Privacy Policy (both effective as of the 16th June 2020).

   End User License Agreement: www.juce.com/juce-6-licence
   Privacy Policy: www.juce.com/juce-privacy-policy

   Or: You may also use this code under the terms of the GPL v3 (see
   www.gnu.org/licenses).

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/


/*******************************************************************************
 The block below describes the properties of this module, and is read by
 the Projucer to automatically generate project code that uses it.
 For details about the syntax and how to create or use a module, see the
 JUCE Module Format.md file.


 BEGIN_JUCE_MODULE_DECLARATION

  ID:                 juce_audio_processors
  vendor:             juce
  version:            6.0.8
  name:               JUCE audio processor classes
  description:        Classes for loading and playing VST, AU, LADSPA, or internally-generated audio processors.
  website:            http://www.juce.com/juce
  license:            GPL/Commercial

  dependencies:       juce_gui_extra, juce_audio_basics
  OSXFrameworks:      CoreAudio CoreMIDI AudioToolbox
  iOSFrameworks:      AudioToolbox

 END_JUCE_MODULE_DECLARATION

*******************************************************************************/


#pragma once
#define JUCE_AUDIO_PROCESSORS_H_INCLUDED

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_basics/juce_audio_basics.h>

//==============================================================================
/** Config: JUCE_PLUGINHOST_VST
    Enables the VST audio plugin hosting classes. You will need to have the VST2 SDK files in your header search paths. You can obtain the VST2 SDK files from on older version of the VST3 SDK.

    @see VSTPluginFormat, VST3PluginFormat, AudioPluginFormat, AudioPluginFormatManager, JUCE_PLUGINHOST_AU, JUCE_PLUGINHOST_VST3, JUCE_PLUGINHOST_LADSPA
*/
#ifndef JUCE_PLUGINHOST_VST
 #define JUCE_PLUGINHOST_VST 0
#endif

/** Config: JUCE_PLUGINHOST_VST3
    Enables the VST3 audio plugin hosting classes.

    @see VSTPluginFormat, VST3PluginFormat, AudioPluginFormat, AudioPluginFormatManager, JUCE_PLUGINHOST_VST, JUCE_PLUGINHOST_AU, JUCE_PLUGINHOST_LADSPA
*/
#ifndef JUCE_PLUGINHOST_VST3
 #define JUCE_PLUGINHOST_VST3 0
#endif

/** Config: JUCE_PLUGINHOST_AU
    Enables the AudioUnit plugin hosting classes. This is Mac-only, of course.

    @see AudioUnitPluginFormat, AudioPluginFormat, AudioPluginFormatManager, JUCE_PLUGINHOST_VST, JUCE_PLUGINHOST_VST3, JUCE_PLUGINHOST_LADSPA
*/
#ifndef JUCE_PLUGINHOST_AU
 #define JUCE_PLUGINHOST_AU 0
#endif

/** Config: JUCE_PLUGINHOST_LADSPA
    Enables the LADSPA plugin hosting classes. This is Linux-only, of course.

    @see LADSPAPluginFormat, AudioPluginFormat, AudioPluginFormatManager, JUCE_PLUGINHOST_VST, JUCE_PLUGINHOST_VST3, JUCE_PLUGINHOST_AU
 */
#ifndef JUCE_PLUGINHOST_LADSPA
 #define JUCE_PLUGINHOST_LADSPA 0
#endif

/** Config: JUCE_CUSTOM_VST3_SDK
    If enabled, the embedded VST3 SDK in JUCE will not be added to the project and instead you should
    add the path to your custom VST3 SDK to the project's header search paths. Most users shouldn't
    need to enable this and should just use the version of the SDK included with JUCE.
*/
#ifndef JUCE_CUSTOM_VST3_SDK
 #define JUCE_CUSTOM_VST3_SDK 0
#endif

#if ! (JUCE_PLUGINHOST_AU || JUCE_PLUGINHOST_VST || JUCE_PLUGINHOST_VST3 || JUCE_PLUGINHOST_LADSPA)
// #error "You need to set either the JUCE_PLUGINHOST_AU and/or JUCE_PLUGINHOST_VST and/or JUCE_PLUGINHOST_VST3 and/or JUCE_PLUGINHOST_LADSPA flags if you're using this module!"
#endif

#if ! (defined (JUCE_SUPPORT_CARBON) || JUCE_64BIT || JUCE_IOS)
 #define JUCE_SUPPORT_CARBON 1
#endif

#ifndef JUCE_SUPPORT_LEGACY_AUDIOPROCESSOR
 #define JUCE_SUPPORT_LEGACY_AUDIOPROCESSOR 1
#endif

//==============================================================================
#include "processors/juce_AudioProcessorEditor.h"
#include "processors/juce_AudioProcessorListener.h"
#include "processors/juce_AudioProcessorParameter.h"
#include "processors/juce_AudioProcessorParameterGroup.h"
#include "processors/juce_AudioProcessor.h"
#include "processors/juce_PluginDescription.h"
#include "processors/juce_AudioPluginInstance.h"
#include "processors/juce_AudioProcessorGraph.h"
#include "processors/juce_GenericAudioProcessorEditor.h"
#include "format/juce_AudioPluginFormat.h"
#include "format/juce_AudioPluginFormatManager.h"
#include "scanning/juce_KnownPluginList.h"
#include "format_types/juce_AudioUnitPluginFormat.h"
#include "format_types/juce_LADSPAPluginFormat.h"
#include "format_types/juce_VSTMidiEventList.h"
#include "format_types/juce_VSTPluginFormat.h"
#include "format_types/juce_VST3PluginFormat.h"
#include "scanning/juce_PluginDirectoryScanner.h"
#include "scanning/juce_PluginListComponent.h"
#include "utilities/juce_AudioProcessorParameterWithID.h"
#include "utilities/juce_RangedAudioParameter.h"
#include "utilities/juce_AudioParameterFloat.h"
#include "utilities/juce_AudioParameterInt.h"
#include "utilities/juce_AudioParameterBool.h"
#include "utilities/juce_AudioParameterChoice.h"
#include "utilities/juce_ParameterAttachments.h"
#include "utilities/juce_AudioProcessorValueTreeState.h"
#include "utilities/juce_PluginHostType.h"
