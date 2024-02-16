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

class State: public Thread,
			 public ApiProviderBase::Holder
{
public:

    State(const var& obj);;
    ~State();

    void onFinish();
    void run() override;

    struct StateProvider;

	ScopedPointer<ApiProviderBase> stateProvider;

    /** Override this method and return a provider if it exists. */
	ApiProviderBase* getProviderBase() override;

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

        void postInit();
        void callOnNext();

        bool matches(const var& obj) const;
        Result runJob();
        double& getProgress();

        void setMessage(const String& newMessage);
        void updateProgressBar(ProgressBar* b) const;
        Thread& getThread() const { return parent; }

    protected:

        String message;

        virtual Result run() = 0;
        
        State& parent;
        double progress = 0.0;
        var localObj;
        bool callOnNextEnabled = false;
    };

    using HardcodedLambda = std::function<var(Job&, const var&)>;

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

    void addTempFile(TemporaryFile* fileToOwnUntilShutdown)
    {
	    tempFiles.add(fileToOwnUntilShutdown);
    }

private:
    
    OwnedArray<TemporaryFile> tempFiles;
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

    var operator()(const var::NativeFunctionArgs& args);

    var get(const Identifier& id) const;

    virtual var perform(multipage::State::Job& t) = 0;
    
protected:

    var globalState;
    multipage::State& state;
};

struct LambdaAction: public CallableAction
{
    using LambdaFunctionWithObject = State::HardcodedLambda;
    using LambdaFunction = std::function<var(State::Job&)>;

	LambdaAction(State& s, const LambdaFunctionWithObject& of_);

    LambdaAction(State& s, const LambdaFunction& lf_);

    LambdaFunctionWithObject of;
    LambdaFunction lf;

    var perform(multipage::State::Job& t) override;
};

struct UndoableVarAction: public UndoableAction
{
    enum class Type
	{
		SetProperty,
        RemoveProperty,
        AddChild,
        RemoveChild,
	};

    UndoableVarAction(const var& parent_, const Identifier& id, const var& newValue_);

    UndoableVarAction(const var& parent_, int index_, const var& newValue_);;
        
    bool perform() override;
    bool undo() override;

    const Type actionType;
    var parent;
    const Identifier key;
    const int index;
    var oldValue;
    var newValue;
};


}
}