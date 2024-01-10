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
 *   which must be separately licensed for closed source applications:
 *
 *   http://www.juce.com
 *
 *   ===========================================================================
 */

#pragma once

namespace hise {
namespace multipage {
using namespace juce;

class Dialog;

class State: public Thread
{
public:

    State(const var& obj);;
    ~State();

    void onFinish();
    void run() override;

    struct Job: public ReferenceCountedObject
    {
        using Ptr = ReferenceCountedObjectPtr<Job>;
        using List = ReferenceCountedArray<Job>;
        using JobFunction = std::function<Result(Job&)>;
        
        Job(State& rt, const var& obj);
        virtual ~Job();;
        
        bool matches(const var& obj) const;
        Result runJob();
        double& getProgress();

    protected:
        
        virtual Result run() = 0;
        
        State& parent;
        double progress = 0.0;
        var localObj;
    };
    
    Job::Ptr getJob(const var& obj);
    var getGlobalSubState(const Identifier& id);
    void addJob(Job::Ptr b, bool addFirst=false);

    double totalProgress = 0.0;
    Job::List jobs;
    Result currentError;
    WeakReference<Dialog> currentDialog;
    var globalState;
    int currentPageIndex = 0;
};

}
}