/*

    IMPORTANT! This file is auto-generated each time you save your
    project - if you alter its contents, your changes may be overwritten!

    This is the header file that your files should include in order to get all the
    JUCE library headers. You should avoid including the JUCE headers directly in
    your own source files, because that wouldn't pick up the correct configuration
    options for your app.

*/

#pragma once

#include "AppConfig.h"

#include <hi_backend/hi_backend.h>
#include <hi_core/hi_core.h>
#include <hi_dsp_library/hi_dsp_library.h>
#include <hi_faust/hi_faust.h>
#include <hi_faust_jit/hi_faust_jit.h>
#include <hi_faust_lib/hi_faust_lib.h>
#include <hi_faust_types/hi_faust_types.h>
#include <hi_lac/hi_lac.h>
#include <hi_loris/hi_loris.h>
#include <hi_rlottie/hi_rlottie.h>
#include <hi_scripting/hi_scripting.h>
#include <hi_snex/hi_snex.h>
#include <hi_streaming/hi_streaming.h>
#include <hi_tools/hi_tools.h>
#include <hi_zstd/hi_zstd.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_core/juce_core.h>
#include <juce_cryptography/juce_cryptography.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_dsp/juce_dsp.h>
#include <juce_events/juce_events.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_opengl/juce_opengl.h>
#include <juce_osc/juce_osc.h>
#include <juce_product_unlocking/juce_product_unlocking.h>
#include <melatonin_blur/melatonin_blur.h>

#include "BinaryData.h"

#if defined (JUCE_PROJUCER_VERSION) && JUCE_PROJUCER_VERSION < JUCE_VERSION
 /** If you've hit this error then the version of the Projucer that was used to generate this project is
     older than the version of the JUCE modules being included. To fix this error, re-save your project
     using the latest version of the Projucer or, if you aren't using the Projucer to manage your project,
     remove the JUCE_PROJUCER_VERSION define.
 */
 #error "This project was last saved using an outdated version of the Projucer! Re-save this project with the latest version to fix this error."
#endif

#if ! DONT_SET_USING_JUCE_NAMESPACE
 // If your code uses a lot of JUCE classes, then this will obviously save you
 // a lot of typing, but can be disabled by setting DONT_SET_USING_JUCE_NAMESPACE.
 using namespace juce;
#endif

#if ! JUCE_DONT_DECLARE_PROJECTINFO
namespace ProjectInfo
{
    const char* const  projectName    = "HISE Standalone";
    const char* const  companyName    = "Hart Instruments";
    const char* const  versionString  = "4.0.0";
    const int          versionNumber  = 0x40000;
}
#endif
