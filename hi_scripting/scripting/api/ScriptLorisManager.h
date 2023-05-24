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

#pragma once

namespace hise { using namespace juce;


class ScriptLorisManager: public ConstScriptingObject,
                          public ControlledObject
{
public:
    
    ScriptLorisManager(ProcessorWithScriptingContent* p);
      
    Identifier getObjectName() const override { return "LorisManager"; }
    
    // =================================================================
    
    /** set a option for the Loris algorithm. */
    void set(String optionId, var newValue)
    {
        if(lorisManager != nullptr)
            lorisManager->set(optionId, newValue.toString());
        else
            reportScriptError("Loris is not available");
    }
    
    /** Returns the setting value for the Loris algorithm. */
    var get(String optionId);
    
    /** Analyse a file. */
    bool analyse(var file, double estimatedRootFrequency);
    
    /** Processes the partial list using predefined commands. */
    void process(var file, String command, var data);
    
    /** Processes the partial list using the given function. */
    void processCustom(var file, var processCallback);
    
    /** Resynthesise the file from the partial lists. Returns an array of variant buffers. */
    var synthesise(var file);
    
    
    
    /** Creates an audio rate envelope from the given parameter and harmonic index. */
    var createEnvelopes(var file, String parameter, int harmonicIndex);
    
    /** Creates a list of path of every channel from the envelope of the given parameter and harmonic index. */
    var createEnvelopePaths(var file, String parameter, int harmonicIndex);
    
    /** Creates a parameter value list for each harmonic at the given time. */
    var createSnapshot(var file, String parameter, double time);
    
    // =================================================================
    
private:
    
    struct Wrapper;
    
    WeakCallbackHolder logFunction;
    WeakCallbackHolder processFunction;
    
    LorisManager::Ptr lorisManager;
};



} // namespace hise
