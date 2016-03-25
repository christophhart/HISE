#ifndef HI_FRONTEND_INCLUDED
#define HI_FRONENT_INCLUDED

#include <AppConfig.h>

#include "LibConfig.h"
#include "Macros.h"
#include "JuceHeader.h"

#include "../JUCE/modules/juce_core/juce_core.h"
#include "../JUCE/modules/juce_audio_basics/juce_audio_basics.h"
#include "../JUCE/modules/juce_gui_basics/juce_gui_basics.h"
#include "../JUCE/modules/juce_audio_devices/juce_audio_devices.h"
#include "../JUCE/modules/juce_audio_utils/juce_audio_utils.h"
#include "../JUCE/modules/juce_gui_extra/juce_gui_extra.h"
#include "../JUCE/modules/stk_core/stk_core.h"
#include "../JUCE/modules/stk_effects/stk_effects.h"
#include "../JUCE/modules/stk_filters/stk_filters.h"
#include "../JUCE/modules/stk_generators/stk_generators.h"
#include "../JUCE/modules/juce_tracktion_marketplace/juce_tracktion_marketplace.h"



using namespace juce;

#include "frontend/FrontEndProcessor.h"
#include "frontend/FrontendBar.h"
#include "frontend/FrontendProcessorEditor.h"


#endif   // HI_FRONTEND_INCLUDED
