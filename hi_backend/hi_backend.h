#ifndef HI_BACKEND_INCLUDED
#define HI_BACKEND_INCLUDED

#include "../hi_modules/hi_modules.h"

using namespace juce;


#define DEBUG_AREA_BACKGROUND_COLOUR 0x11FFFFFF
#define DEBUG_AREA_BACKGROUND_COLOUR_DARK 0x84000000
#define BACKEND_BG_COLOUR 0xFF888888//0xff4d4d4d
#define BACKEND_BG_COLOUR_BRIGHT 0xFF646464

#define BACKEND_ICON_COLOUR_ON 0xCCFFFFFF
#define BACKEND_ICON_COLOUR_OFF 0xFF333333

#define DEBUG_BG_COLOUR 0xff636363

#include "backend/BackendBinaryData.h"

#include "backend/debug_components/SamplePoolTable.h"
#include "backend/debug_components/MacroEditTable.h"
#include "backend/debug_components/ScriptWatchTable.h"
#include "backend/debug_components/ProcessorCollection.h"
#include "backend/debug_components/ApiBrowser.h"
#include "backend/debug_components/ModuleBrowser.h"
#include "backend/debug_components/PatchBrowser.h"
#include "backend/debug_components/FileBrowser.h"
#include "backend/debug_components/DebugArea.h"

#include "backend/BackendProcessor.h"
#include "backend/BackendComponents.h"
#include "backend/BackendToolbar.h"
#include "backend/ProcessorPopupList.h"
#include "backend/BackendApplicationCommands.h"
#include "backend/BackendEditor.h"



#endif   // HI_BACKEND_INCLUDED
