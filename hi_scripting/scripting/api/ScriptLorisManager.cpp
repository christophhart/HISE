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

namespace hise {
using namespace juce;

struct ScriptLorisManager::Wrapper
{
    API_VOID_METHOD_WRAPPER_2(ScriptLorisManager, set);
    API_METHOD_WRAPPER_1(ScriptLorisManager, get);
    API_METHOD_WRAPPER_2(ScriptLorisManager, analyse);
    API_METHOD_WRAPPER_1(ScriptLorisManager, synthesise);
    API_VOID_METHOD_WRAPPER_3(ScriptLorisManager, process);
    API_VOID_METHOD_WRAPPER_2(ScriptLorisManager, processCustom);
    
    API_METHOD_WRAPPER_3(ScriptLorisManager, createEnvelopes);
    API_METHOD_WRAPPER_3(ScriptLorisManager, createEnvelopePaths);
    API_METHOD_WRAPPER_3(ScriptLorisManager, createSnapshot);
};

ScriptLorisManager::ScriptLorisManager(ProcessorWithScriptingContent* p):
  ConstScriptingObject(p, 0),
  ControlledObject(p->getMainController_()),
  logFunction(p, nullptr, var(), 0),
  processFunction(p, nullptr, var(), 0)
{
    lorisManager = getMainController()->getLorisManager();
    
    if(lorisManager != nullptr)
    {
        lorisManager->setLogFunction([&](String m)
        {
            debugToConsole(dynamic_cast<Processor*>(getScriptProcessor()), m);
        });
    }
    
    ADD_API_METHOD_2(set);
    ADD_API_METHOD_1(get);
    ADD_API_METHOD_2(analyse);
    ADD_API_METHOD_1(synthesise);
    ADD_API_METHOD_3(process);
    ADD_API_METHOD_2(processCustom);
    
    ADD_API_METHOD_3(createEnvelopes);
    ADD_API_METHOD_3(createEnvelopePaths);
    ADD_API_METHOD_3(createSnapshot);
}



bool ScriptLorisManager::analyse(var file, double rootFrequency)
{
    if(lorisManager == nullptr)
        reportScriptError("Loris is not available");
    
    if(auto sf = dynamic_cast<ScriptingObjects::ScriptFile*>(file.getObject()))
    {
        lorisManager->analyse({{sf->f, rootFrequency}});
        return true;
    }
    
    return false;
}

var ScriptLorisManager::synthesise(var file)
{
    if(lorisManager == nullptr)
        reportScriptError("Loris is not available");
    
    if(auto sf = dynamic_cast<ScriptingObjects::ScriptFile*>(file.getObject()))
    {
        return var(lorisManager->synthesise(sf->f));
    }
    
    return {};
}

void ScriptLorisManager::process(var file, String command, var data)
{
    if(lorisManager == nullptr)
        reportScriptError("Loris is not available");
    
    if(auto sf = dynamic_cast<ScriptingObjects::ScriptFile*>(file.getObject()))
    {
        lorisManager->process(sf->f, command, JSON::toString(data));
    }
}
              
              
void ScriptLorisManager::processCustom(var file, var processCallback)
{
    processFunction = WeakCallbackHolder(getScriptProcessor(), this, processCallback, 1);
    
    if(lorisManager == nullptr)
        reportScriptError("Loris is not available");
        
    if(auto sf = dynamic_cast<ScriptingObjects::ScriptFile*>(file.getObject()))
    {
        lorisManager->processCustom(sf->f, [&](LorisManager::CustomPOD& data)
        {
            auto obj = data.toJSON();
            auto ok = processFunction.callSync(&obj, 1);
            
            if(!ok.wasOk())
                reportScriptError(ok.getErrorMessage());
            
            data.writeJSON(obj);
            
            return false;
        });
    }
}

juce::var ScriptLorisManager::createEnvelopes(juce::var file, snex::jit::String parameter, int harmonicIndex)
{
    if(lorisManager == nullptr)
        reportScriptError("Loris is not available");
    
    if(auto sf = dynamic_cast<ScriptingObjects::ScriptFile*>(file.getObject()))
    {
        return var(lorisManager->createEnvelope(sf->f, Identifier(parameter), harmonicIndex));
    }
    
    return {};
}

juce::var ScriptLorisManager::createEnvelopePaths(juce::var file, snex::jit::String parameter, int harmonicIndex)
{
    if(lorisManager == nullptr)
        reportScriptError("Loris is not available");
    
    if(auto sf = dynamic_cast<ScriptingObjects::ScriptFile*>(file.getObject()))
    {
        auto envelopes = createEnvelopes(file, parameter, harmonicIndex);
        
        Array<var> paths;
        
        for(auto e: *envelopes.getArray())
        {
            auto p = lorisManager->setEnvelope(e, Identifier(parameter));
            
            auto np = new ScriptingObjects::PathObject(getScriptProcessor());
            
            np->getPath() = p;
            
            paths.add(var(np));
        }
        
        return var(paths);
    }
    
    return {};
}

var ScriptLorisManager::createSnapshot(juce::var file, String parameter, double time)
{
    if(lorisManager == nullptr)
        reportScriptError("Loris is not available");
    
    if(auto sf = dynamic_cast<ScriptingObjects::ScriptFile*>(file.getObject()))
    {
        return var(lorisManager->getSnapshot(sf->f, time, Identifier(parameter)));
    }
    
    return {};
}


juce::var ScriptLorisManager::get(String optionId)
{
    if(lorisManager == nullptr)
        reportScriptError("Loris is not available");
    
    return var(lorisManager->get(optionId));
}






} // namespace hise
