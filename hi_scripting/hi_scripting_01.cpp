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
*   Commercial licenses for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licensing:
*
*   http://www.hise.audio/
*
*   HISE is based on the JUCE library,
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

/* HI Module */

#include "JuceHeader.h"

#include <regex>


#include "scripting/api/DspFactory.cpp"
#include "scripting/api/DspInstance.cpp"

#include "scripting/engine/JavascriptApiClass.cpp"
#include "scripting/api/ScriptingBaseObjects.cpp"
#include "scripting/api/FixLayoutObjects.cpp"

#include "scripting/engine/HiseJavascriptEngine.cpp"
#include "scripting/engine/JavascriptEngineExpressions.cpp"
#include "scripting/engine/JavascriptEngineStatements.cpp"
#include "scripting/engine/JavascriptEngineOperators.cpp"
#include "scripting/engine/JavascriptEngineCustom.cpp"
#include "scripting/engine/JavascriptEngineParser.cpp"
#include "scripting/engine/JavascriptEngineObjects.cpp"
#include "scripting/engine/JavascriptEngineMathObject.cpp"
#include "scripting/engine/JavascriptEngineAdditionalMethods.cpp"
#include "scripting/engine/JavascriptEngineCyclicReferenceChecks.cpp"

#include "scripting/api/ScriptingApiObjects.cpp"
#include "scripting/api/ScriptBroadcaster.cpp"
#include "scripting/api/ScriptDrawActions.cpp"
#include "scripting/api/ScriptTableListModel.cpp"
#include "scripting/api/ScriptingGraphics.cpp"
#include "scripting/api/ScriptExpansion.cpp"
#if USE_BACKEND || HISE_ENABLE_LORIS_ON_FRONTEND
#include "scripting/api/ScriptLorisManager.cpp"
#endif
#include "scripting/api/GlobalServer.cpp"
#include "scripting/api/ScriptingApi.cpp"

#include "scripting/api/ScriptingApiWrappers.cpp"
#include "scripting/api/ScriptingApiContent.cpp"