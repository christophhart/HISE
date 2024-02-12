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
    SN_NODE_ID("Action");
    
    Action(Dialog& r, int, const var& obj);

    static String getCategoryId() { return "Actions"; }

    virtual String getDescription() const = 0;

    template <typename T> void createBasicEditor(T& t, Dialog::PageInfo& rootList, const String& helpText)
    {
        rootList.addChild<Type>({
	        { mpid::ID, "Type"},
	        { mpid::Type, T::getStaticId().toString() },
	        { mpid::Help, helpText }
	    });

        StringArray allGlobalStates;

        for(const auto& nv: rootDialog.getState().globalState.getDynamicObject()->getProperties())
        {
	        allGlobalStates.add(nv.name.toString());
        }

        rootList.addChild<TextInput>({
            { mpid::ID, "ID" },
            { mpid::Text, "ID" },
            { mpid::Value, id.toString() },
            { mpid::Items, allGlobalStates.joinIntoString("\n") },
            { mpid::Help, "The ID of the action. This will determine whether the action is tied to a global state value. If not empty, the action will only be performed if the value is not zero." }
        });

        rootList.addChild<Button>({
            { mpid::ID, "CallOnNext" },
            { mpid::Text, "CallOnNext" },
            { mpid::Help, "If enabled, the action will launched when you press the next button (otherwise it will be executed on page load." },
            { mpid::Value, callOnNext }
        });
    }

    void paint(Graphics& g) override;

    void postInit() override;
    void perform();
    Result checkGlobalState(var globalState) override;

    void editModeChanged(bool isEditMode) override;

    Result r;

	bool callOnNext = false;
};

/** A base class for an action that will be performed on page load. */
struct ImmediateAction: public Action
{
    ImmediateAction(Dialog& r,int w, const var& obj):
      Action(r, w, obj)
    {
	    setCustomCheckFunction([this](Dialog::PageBase* pb, const var& obj)
		{
            if(id.isValid())
            {
	            if(!obj[id])
                    return Result::ok();
            }

            if(rootDialog.isEditModeEnabled())
				return Result::ok();

			return this->onAction();
		});
    };
    
	virtual Result onAction() = 0;
};

struct Skip: public ImmediateAction
{
    SN_NODE_ID("Skip");

    Skip(Dialog& r, int w, const var& obj):
	    ImmediateAction(r, w, obj)
	{
		
	}

    void createEditor(Dialog::PageInfo& rootList) override
    {
	    createBasicEditor(*this, rootList, "An action element that simply skips the page that contains this element. This can be used in order to skip a page with a branch (eg. if one of the options doesn't require additional information.)");
    }

    Result onAction() override
    {
	    auto rt = &rootDialog;

        auto direction = rt->getCurrentNavigationDirection();
        
        MessageManager::callAsync([rt, direction]()
        {
	        rt->navigate(direction);
        });
        
        return Result::ok();
    }

    String getDescription() const override { return "skipPage()"; };
};

struct Launch: public ImmediateAction
{
    SN_NODE_ID("Launch");

    void createEditor(Dialog::PageInfo& rootList) override
    {
	    createBasicEditor(*this, rootList, "Shows either a file in the OS file browser or opens an internet browser to load a URL");

        rootList.addChild<TextInput>({
			{ mpid::ID, "Text"},
            { mpid::Text, "Text" },
            { mpid::Required, true },
            { mpid::Value, currentLaunchTarget },
            { mpid::Help, "The target to be launched. If this is a URL, it will launch the internet browser, if it's a file, it will open the file" }
        });
    }

    Launch(Dialog& r, int w, const var& obj):
	    ImmediateAction(r, w, obj)
	{
        if(!obj.hasProperty(mpid::CallOnNext))
            callOnNext = true;

        currentLaunchTarget = obj[mpid::Text].toString();
	}

    bool isFinished = false;

    Result onAction() override
    {
        if(isFinished)
            return Result::ok();

	    auto t = MarkdownText::getString(currentLaunchTarget, rootDialog);

        if(URL::isProbablyAWebsiteURL(t))
        {
            isFinished = true;
	        URL(t).launchInDefaultBrowser();
            return Result::ok();
        }

        if(File::isAbsolutePath(t))
        {
	        File f(t);

            if(f.existsAsFile() || f.isDirectory())
            {
                isFinished = true;
	            f.revealToUser();
				return Result::ok();
            }
            else
            {
	            return Result::fail("The file does not exist");
            }
        }

        return Result::ok();
    }

    String currentLaunchTarget;

    String getDescription() const override { return "launch(" + MarkdownText::getString(currentLaunchTarget, rootDialog).quoted() + ")"; };
};


struct BackgroundTask: public Action
{
    using Job = State::Job;
    
    struct WaitJob: public State::Job
    {
        WaitJob(State& r, const var& obj);;
        
        Result run() override;
        Result abort(const String& message);

        WeakReference<BackgroundTask> currentPage;
    };
    
    Job::Ptr job;
    
    BackgroundTask(Dialog& r, int w, const var& obj);

    void paint(Graphics& g) override;
    void resized() override;

    void postInit() override
    {
	    Action::postInit();

        if(job != nullptr)
            job->postInit();
    }

    virtual Result performTask(State::Job& t) = 0;

    Result checkGlobalState(var globalState) override
    {
        if(callOnNext)
        {
            // make it go through the next time
            callOnNext = false;
	        job->callOnNext();
        }
            
	    return Action::checkGlobalState(globalState);
    }
    
protected:

    Result abort(const String& message)
	{
		SafeAsyncCall::call<BackgroundTask>(*this, [](BackgroundTask& w)
		{
			w.rootDialog.setCurrentErrorPage(&w);
			w.retryButton.setVisible(true);
			w.resized();
		});
	            
		return Result::fail(message);
	}

    String label;
    ScopedPointer<ProgressBar> progress;
    HiseShapeButton retryButton;

    JUCE_DECLARE_WEAK_REFERENCEABLE(BackgroundTask);
};

struct LambdaTask: public BackgroundTask
{
	SN_NODE_ID("LambdaTask");

    LambdaTask(Dialog& r, int w, const var& obj):
      BackgroundTask(r, w, obj)
    {
	    lambda = obj[mpid::Function].getNativeFunction();
    };

    var::NativeFunction lambda;

    Result performTask(State::Job& t) override
	{
        if(rootDialog.isEditModeEnabled())
            return Result::ok();

        if(!lambda)
        {
            t.setMessage("Empty lambda, simulating...");

            for(int i = 0; i < 30; i++)
            {
                t.getProgress() = (double)i / 30.0;
	            t.getThread().wait(50);
            }

            t.getProgress() = 1.0;
            t.setMessage("Done");
            
	        return Result::ok();
        }
        
        try
        {
            var::NativeFunctionArgs args(rootDialog.getState().globalState, nullptr, 0);

            auto rv = lambda(args);

            if(!rv.isUndefined())
                writeState(rv);

            return Result::ok();
        }
        catch(Result& r)
        {
            return r;
        }
    }

    String getDescription() const override
    {
        return "Lambda Task";
    }

    void createEditor(Dialog::PageInfo& rootList) override;
};

struct DummyWait: public BackgroundTask
{
    SN_NODE_ID("DummyWait");

    DummyWait(Dialog& r, int w, const var& obj):
      BackgroundTask(r, w, obj)
    {
	    numTodo = (int)obj[mpid::NumTodo];

        if(numTodo == 0)
            numTodo = 100;

        waitTime = (int)obj[mpid::WaitTime];

        if(waitTime < 4)
            waitTime = 30;

        failIndex = (int)obj[mpid::FailIndex];

        if(failIndex == 0)
            failIndex = numTodo + 2;
    }
    
	void createEditor(Dialog::PageInfo& rootList) override;

    String getDescription() const override
    {
        return "Dummy Wait";
    }

	Result performTask(State::Job& t) override
	{
        if(rootDialog.isEditModeEnabled())
            return Result::ok();
        
		for(int i = 0; i < numTodo; i++)
		{
			if(t.getThread().threadShouldExit())
				return Result::fail("aborted");
	                
			t.getProgress() = (double)i / jmax(1.0, (double)(numTodo-1));
			t.getThread().wait(waitTime);
	                
			if(i == failIndex)
				return abort("**Lost connection**.  \nPlease ensure that your internet connection is stable and click the retry button to resume the download process.");
		}
	            
		return Result::ok();
	}

    int waitTime = 30;
    int numTodo = 100;
    int failIndex = 101;
};




} // factory
} // multipage
} // hise
