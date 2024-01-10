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
namespace factory {
using namespace juce;

struct Action: public Dialog::PageBase
{
    enum class CallType
    {
        Asynchronous,
        Synchronous,
        BackgroundThread
    };
    
    SN_NODE_ID("Action");
    
    Action(Dialog& r, int, const var& obj);

    void postInit() override;
    void perform();
    Result checkGlobalState(var globalState) override;

    Result r;
    CallType callType = CallType::Asynchronous;
};

struct Skip: public Action
{
    Skip(Dialog& r, int w, const var& obj);;
};

// TODO: Rename to BackgroundTask and add dynamic logic
struct DummyWait: public Action
{
    SN_NODE_ID("DummyWait");
    
    using Job = State::Job;
    
    struct WaitJob: public State::Job
    {
        WaitJob(State& r, const var& obj);;
        
        Result run() override;
        Result abort(const String& message);

        WeakReference<DummyWait> currentPage;
    };
    
    Job::Ptr job;
    
    DummyWait(Dialog& r, int w, const var& obj);

    void paint(Graphics& g) override;
    void resized() override;

protected:

    String label;
    ScopedPointer<ProgressBar> progress;
    HiseShapeButton retryButton;
    
    JUCE_DECLARE_WEAK_REFERENCEABLE(DummyWait);
};


} // factory
} // multipage
} // hise