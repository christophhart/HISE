#ifndef HI_PLAYER_INCLUDED
#define HI_PLAYER_INCLUDED

#include <AppConfig.h>

#define USE_FRONTEND 0



#define DEBUG_AREA_BACKGROUND_COLOUR 0x11FFFFFF
#define DEBUG_AREA_BACKGROUND_COLOUR_DARK 0x84000000
#define BACKEND_BG_COLOUR 0xFF888888//0xff4d4d4d
#define BACKEND_BG_COLOUR_BRIGHT 0xFF646464

#include "../hi_modules/hi_modules.h"


using namespace juce;

#include "player/PlayerBinaryData.h"
#include "player/PresetProcessor.h"
#include "player/PresetContainerProcessor.h"

#include "player/PresetPlayerComponents.h"
#include "player/PresetProcessorEditor.h"
#include "player/PresetContainerProcessorEditor.h"

#endif   // HI_PLAYER_INCLUDED
