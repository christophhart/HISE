#ifndef HI_CORE_INCLUDED
#define HI_CORE_INCLUDED

#include <AppConfig.h>

#include "../JUCE/modules/juce_core/juce_core.h"
#include "../JUCE/modules/juce_audio_basics/juce_audio_basics.h"
#include "../JUCE/modules/juce_gui_basics/juce_gui_basics.h"
#include "../JUCE/modules/juce_audio_devices/juce_audio_devices.h"
#include "../JUCE/modules/juce_audio_utils/juce_audio_utils.h"
#include "../JUCE/modules/juce_gui_extra/juce_gui_extra.h"
#include "../JUCE/modules/juce_tracktion_marketplace/juce_tracktion_marketplace.h"

#include "LibConfig.h"
#include "Macros.h"

//=============================================================================
/** Config: USE_BACKEND

If true, then the plugin uses the backend system including IDE editor & stuff.
*/
#ifndef USE_BACKEND
#define USE_BACKEND 0
#endif


using namespace juce;

/**Appconfig file

Use this file to enable the modules that are needed

For all defined variables:

- 1 if the module is used
- 0 if the module should not be used

*/

// Comment this out if you don't want to use the debug tools
#define USE_HI_DEBUG_TOOLS 1


/** Add new subgroups here and in hi_module.cpp
*
*	New files must be added in the specific subfolder header / .cpp file.
*/

#include "hi_binary_data/hi_binary_data.h"
#include "hi_core/hi_core.h"
#include "hi_components/hi_components.h"
#include "hi_dsp/hi_dsp.h"
#include "hi_scripting/hi_scripting.h"
#include "hi_sampler/hi_sampler.h"


#endif   // HI_CORE_INCLUDED
