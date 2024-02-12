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

    struct CallOnNextAction
    {
	    
    };

    struct Job: public ReferenceCountedObject
    {
        using Ptr = ReferenceCountedObjectPtr<Job>;
        using List = ReferenceCountedArray<Job>;
        using JobFunction = std::function<Result(Job&)>;
        
        Job(State& rt, const var& obj);
        virtual ~Job();;

        void postInit()
        {
	        if(!callOnNextEnabled)
				parent.addJob(this);
        }

        void callOnNext();

        bool matches(const var& obj) const;
        Result runJob();
        double& getProgress();

        void setMessage(const String& newMessage);

        void updateProgressBar(ProgressBar* b)
        {
	        if(message.isNotEmpty())
                b->setTextToDisplay(message);
        }

        Thread& getThread() const { return parent; }

    protected:

        String message;

        virtual Result run() = 0;
        
        State& parent;
        double progress = 0.0;
        var localObj;
        bool callOnNextEnabled = false;
    };

    Job::Ptr currentJob = nullptr;
    Job::Ptr getJob(const var& obj);
    var getGlobalSubState(const Identifier& id);
    void addJob(Job::Ptr b, bool addFirst=false);

    bool navigateOnFinish = false;

    double totalProgress = 0.0;
    Job::List jobs;
    Result currentError;
    WeakReference<Dialog> currentDialog;
    var globalState;
    int currentPageIndex = 0;
};


/** This base class is used to store actions in a var::NativeFunction type.
 *
 *  - subclass from this base clase
 *  - override the perform() method and return the value that is supposed to be stored for the given ID (if applicable)
 *  - throw a Result if there is an error
 */
struct CallableAction
{
    CallableAction(State& s):
      state(s)
    {};

    virtual ~CallableAction() {};

    var operator()(const var::NativeFunctionArgs& args)
    {
        globalState = args.thisObject;

        if(state.currentJob != nullptr)
        	return perform(*state.currentJob);
        
        jassertfalse;
    	return var();
    }

    var get(const Identifier& id) const
    {
	    return globalState[id];
    }

    virtual var perform(multipage::State::Job& t) = 0;
    
protected:

    var globalState;
    multipage::State& state;
};

struct LambdaAction: public CallableAction
{
    using LambdaFunctionWithObject = std::function<var(State::Job&, const var&)>;
    using LambdaFunction = std::function<var(State::Job&)>;

	LambdaAction(State& s, const LambdaFunctionWithObject& of_):
      CallableAction(s),
      of(of_)
	{}

    LambdaAction(State& s, const LambdaFunction& lf_):
      CallableAction(s),
      lf(lf_)
	{}

    LambdaFunctionWithObject of;
    LambdaFunction lf;

    var perform(multipage::State::Job& t) override
    {
        if(lf)
			return lf(t);
        else if (of)
            return of(t, globalState);

        jassertfalse;
        return var();
    }
};

struct JavascriptAction: public CallableAction
{
    JavascriptAction(multipage::State& s, const String& code_):
      CallableAction(s),
      code(code_)
	{
	    
	}

    var perform(multipage::State::Job& t) override
    {
        auto r = Result::ok();
        JavascriptEngine e;

        auto function = e.evaluate(code, &r);

	    if(r.failed())
            throw r;

        auto rv = (int)e.callFunctionObject(globalState.getDynamicObject(), function, var::NativeFunctionArgs(var(), nullptr, 0));
        
        return rv;
    }

    const String code;
    
};


}
}