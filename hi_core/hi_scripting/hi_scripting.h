#ifndef HI_SCRIPTING_INCLUDED
#define HI_SCRIPTING_INCLUDED

#define MAX_SCRIPT_HEIGHT 700

#include "../hi_core/hi_core.h"

#include "scripting/api/XmlApi.h"
#include "scripting/api/ScriptingBaseObjects.h"
#include "scripting/api/ScriptingApi.h"


#include "scripting/ScriptProcessor.h"
#include "scripting/HardcodedScriptProcessor.h"

#include "scripting/components/ScriptingCodeEditor.h"

#include "scripting/api/ScriptComponentWrappers.h"
#include "scripting/components/ScriptingContentComponent.h"

#if USE_BACKEND

#include "scripting/components/ScriptingEditor.h"

#endif 
#endif
