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
#include "hi_lac/hlac/CompressionHelpers.h"

namespace hise {
namespace multipage {
namespace factory {
using namespace juce;

struct Action: public Dialog::PageBase
{
    enum class TriggerType
    {
	    OnPageLoad, // called when page is loaded (in postInit)
        OnPageLoadAsync, // called asynchronously when page is loaded
        OnSubmit, // called when Next is pressed
        OnCall // manual call
    };

    HISE_MULTIPAGE_ID("Action");
    
    Action(Dialog& r, int, const var& obj);

    static String getCategoryId() { return "Actions"; }
    virtual String getDescription() const = 0;

    template <typename T> void createBasicEditor(T& t, Dialog::PageInfo& rootList, const String& helpText);

    void paint(Graphics& g) override;

    virtual void setActive(bool shouldBeActive) {};
    
    void postInit() override;
    void perform();
    Result checkGlobalState(var globalState) override;
    Result r;

    static StringArray getEventTriggerIds()
    {
	    return {
            "OnPageLoad",
            "OnPageLoadAsync",
            "OnSubmit",
            "OnCall"
	    };
    }

    void setTriggerType()
    {
        if(infoObject.hasProperty("CallOnNext"))
        {
	        if((bool)infoObject["ManualAction"])
	        {
		        triggerType = TriggerType::OnCall;
	        }
            else
            {
	            triggerType = (bool)infoObject["CallOnNext"] ? TriggerType::OnSubmit : TriggerType::OnPageLoad;
            }

            infoObject.getDynamicObject()->removeProperty("CallOnNext");
            infoObject.getDynamicObject()->removeProperty("ManualAction");
            infoObject.getDynamicObject()->setProperty(mpid::EventTrigger, getEventTriggerIds()[(int)triggerType]);

            return;
        }

	    const auto typeIds = getEventTriggerIds();
        auto typeName = infoObject[mpid::EventTrigger].toString();

        auto idx = typeIds.indexOf(typeName);

        if(typeName.isNotEmpty() && idx != -1)
        {
            triggerType = (TriggerType)idx;
        }
        else
        {
	        triggerType = TriggerType::OnPageLoad;
        }
    }

    CustomCheckFunction actionCallback;

    TriggerType triggerType = TriggerType::OnPageLoad;

    JUCE_DECLARE_WEAK_REFERENCEABLE(Action);
};

/** A base class for an action that will be performed on page load. */
struct ImmediateAction: public Action
{
    ImmediateAction(Dialog& r,int w, const var& obj);;

    virtual bool skipIfStateIsFalse() const = 0;

	virtual Result onAction() = 0;
};

struct Skip: public ImmediateAction
{
    HISE_MULTIPAGE_ID("Skip");

    Skip(Dialog& r, int w, const var& obj);

    bool skipIfStateIsFalse() const override { return true; }

    CREATE_EDITOR_OVERRIDE;

    Result onAction() override;
    String getDescription() const override { return "skipPage()"; };

    bool invert = false;
};

struct JavascriptFunction: public ImmediateAction
{
	HISE_MULTIPAGE_ID("JavascriptFunction");

    JavascriptFunction(Dialog& r, int w, const var& obj):
      ImmediateAction(r, w, obj)
    {};

    bool skipIfStateIsFalse() const override { return false; }

	CREATE_EDITOR_OVERRIDE;

	Result onAction() override;

	String getDescription() const override { return "function() {...}"; };
};

struct AppDataFileWriter: public ImmediateAction
{
	HISE_MULTIPAGE_ID("AppDataFileWriter");

    AppDataFileWriter(Dialog& r, int w, const var& obj);

    bool skipIfStateIsFalse() const override { return false; }

    void paint(Graphics& g) override;

    CREATE_EDITOR_OVERRIDE;

    Result onAction() override;
	String getDescription() const override { return "writeAppDataFile(" + infoObject[mpid::ID].toString() + ")"; };

    File targetFile;
};

struct RelativeFileLoader: public ImmediateAction
{
	HISE_MULTIPAGE_ID("RelativeFileLoader");

    RelativeFileLoader(Dialog& r, int w, const var& obj);

    bool skipIfStateIsFalse() const override { return false; }

    static String getCategoryId() { return "Constants"; }

    CREATE_EDITOR_OVERRIDE;

	static StringArray getSpecialLocations();

	Result onAction() override;

	String getDescription() const override { return "FileLoader"; };
};

struct Launch: public ImmediateAction
{
    HISE_MULTIPAGE_ID("Launch");

    Launch(Dialog& r, int w, const var& obj);

    bool skipIfStateIsFalse() const override { return true; }

    CREATE_EDITOR_OVERRIDE;
    Result onAction() override;
    String getDescription() const override;;

private:

    String currentLaunchTarget;
    String args;
};

struct BackgroundTask: public Action
{
    using Job = State::Job;
    
    struct WaitJob: public State::Job
    {
        WaitJob(State& r, const var& obj);;
        
        Result run() override;
        
        WeakReference<BackgroundTask> currentPage;
    };
    
    Job::Ptr job;
    
    BackgroundTask(Dialog& r, int w, const var& obj);

    void paint(Graphics& g) override;
    void resized() override;
    void postInit() override;

    bool hasOnSubmitEvent() const override
    {
	    return triggerType == TriggerType::OnSubmit && !finished;
    }

    virtual Result performTask(State::Job& t) = 0;

    Result checkGlobalState(var globalState) override;

    void setActive(bool shouldBeActive) override
    {
        if(!shouldBeActive)
            progress->setTextToDisplay("This step is inactive");

        textLabel->setEnabled(shouldBeActive);
        progress->setEnabled(shouldBeActive);
    }
    
protected:

    void addSourceTargetEditor(Dialog::PageInfo& rootList);

    /** Returns the source as URL. */
    URL getSourceURL() const;

    /** Returns the source as File. */
    File getSourceFile() const { return getFileInternal(mpid::Source); }

    /** Returns the target (directory) as file. */
    File getTargetFile() const { return getFileInternal(mpid::Target); }

    Result abort(const String& message);

    String label;
    Component* textLabel;
    ScopedPointer<ProgressBar> progress;
    HiseShapeButton retryButton, stopButton;

    void abortWithErrorMessage(const String& e)
    {
	    errorMessage = e;
    }

private:

    String errorMessage;
    bool finished = false;

    File getFileInternal(const Identifier& id) const;

    JUCE_DECLARE_WEAK_REFERENCEABLE(BackgroundTask);
};

struct LambdaTask: public BackgroundTask
{
	HISE_MULTIPAGE_ID("LambdaTask");

    LambdaTask(Dialog& r, int w, const var& obj);;

    Result performTask(State::Job& t) override;
	String getDescription() const override;

    CREATE_EDITOR_OVERRIDE;

    var::NativeFunction lambda;
};


struct HttpRequest: public BackgroundTask
{
	HISE_MULTIPAGE_ID("HttpRequest");

    HttpRequest(Dialog& r, int w, const var& obj);;

    Result performTask(State::Job& t) override;
	String getDescription() const override { return "HttpRequest"; }

    CREATE_EDITOR_OVERRIDE;

    URL url;
    bool isPost;
    bool parseJSON;
    String extraHeaders;
    String parameters;
    String requestFunction;
};

struct DownloadTask: public BackgroundTask
{
    HISE_MULTIPAGE_ID("DownloadTask");

    DownloadTask(Dialog& r, int w, const var& obj);;

    ~DownloadTask() override;

    Result performTask(State::Job& t) override;

    CREATE_EDITOR_OVERRIDE;

    String getDescription() const override;

    CriticalSection downloadLock;

    std::unique_ptr<URL::DownloadTask> dt;
    ScopedPointer<TemporaryFile> tempFile;
    String extraHeaders;
    bool usePost = false;
    
};

struct UnzipTask: public BackgroundTask
{
    HISE_MULTIPAGE_ID("UnzipTask");

    UnzipTask(Dialog& r, int w, const var& obj);

    ~UnzipTask() {}

    Result performTask(State::Job& t) override;

    CREATE_EDITOR_OVERRIDE;

    String getDescription() const override { return "Unzip Action"; }

    bool overwrite = true;
};

// TODO: Parses a install log from a file and then removes all files

struct UninstallTask //: public BackgroundTask
{
	
};

// TODO: Writes the list of all file operations to a given file to be picked up the an uninstall task
struct LogFileCreator: public ImmediateAction
{
	
};

struct CopyAsset: public BackgroundTask
{
    HISE_MULTIPAGE_ID("CopyAsset");

    CopyAsset(Dialog& r, int w, const var& obj):
      BackgroundTask(r, w, obj)
    {}

    Result performTask(State::Job& t) override;

    CREATE_EDITOR_OVERRIDE;

    String getDescription() const override { return "CopyAsset"; }

    bool overwrite = true;
};

struct CopySiblingFile: public BackgroundTask
{
    HISE_MULTIPAGE_ID("CopySiblingFile");

    CopySiblingFile(Dialog& r, int w, const var& obj):
      BackgroundTask(r, w, obj)
    {}

    Result performTask(State::Job& t) override;

    CREATE_EDITOR_OVERRIDE;

    String getDescription() const override { return "CopySiblingFile"; }

    bool overwrite = true;
};

struct HlacDecoder: public BackgroundTask,
				    public hlac::HlacArchiver::Listener
{
    HISE_MULTIPAGE_ID("HlacDecoder");

    HlacDecoder(Dialog& r, int w, const var& obj);

    ~HlacDecoder() override;

    Result performTask(State::Job& t) override;

    void logStatusMessage(const String& message) override;

    void logVerboseMessage(const String& verboseMessage) override;;
	void criticalErrorOccured(const String& message) override
	{
        rootDialog.logMessage(MessageType::Hlac, "ERROR: " + message);
		r = Result::fail(message);
	}

    CREATE_EDITOR_OVERRIDE;

    String getDescription() const override;

    State::Job::Ptr currentJob;

    bool supportFullDynamics = false;

    Result r;
    Identifier sourceId;

    bool useTotalProgress = true;
    
};


struct DummyWait: public BackgroundTask
{
    HISE_MULTIPAGE_ID("DummyWait");

    DummyWait(Dialog& r, int w, const var& obj);

	CREATE_EDITOR_OVERRIDE;

    String getDescription() const override;

    Result performTask(State::Job& t) override;

    int waitTime = 30;
    int numTodo = 100;
    int failIndex = 101;
};


struct Constants: public Dialog::PageBase
{
    static String getCategoryId() { return "Constants"; }

	Constants(Dialog& r, int w, const var& obj);

    void postInit() override;

    virtual String getDescription() const = 0;

    void paint(Graphics& g) override;

    Result checkGlobalState(var globalState) override { return Result::ok(); }

protected:

    /** Use this to write the constants. */
    void setConstant(const Identifier& id, const var& newValue);

    /** Overwrite this and use setConstant to store every constant that you need. */
    virtual void loadConstants() = 0;
};

struct CopyProtection: public Constants
{
    HISE_MULTIPAGE_ID("CopyProtection");

	CopyProtection(Dialog& r, int w, const var& obj);;

    String getDescription() const override { return "Copy Protection";}

    void loadConstants() override;
};

struct PluginDirectories: public Constants
{
	HISE_MULTIPAGE_ID("PluginDirectories");

	PluginDirectories(Dialog& r, int w, const var& obj):
      Constants(r, w, obj)
    {};

    String getDescription() const override { return "Plugin Directories";}

    

	void loadConstants() override;
};

struct OperatingSystem: public Constants
{
	HISE_MULTIPAGE_ID("OperatingSystem");

	OperatingSystem(Dialog& r, int w, const var& obj):
      Constants(r, w, obj)
    {};

    String getDescription() const override { return "Operating System";}

	void loadConstants() override;
};

struct FileLogger: public Constants
{
	HISE_MULTIPAGE_ID("FileLogger");

	FileLogger(Dialog& r, int w, const var& obj):
      Constants(r, w, obj)
	{
		
	};

    String getDescription() const override { return "Project Info";}

    CREATE_EDITOR_OVERRIDE;

	void loadConstants() override;
};

struct DirectoryScanner: public Constants
{
	HISE_MULTIPAGE_ID("DirectoryScanner");

	DirectoryScanner(Dialog& r, int w, const var& obj);

	String getDescription() const override { return "Directory Scanner";}

    CREATE_EDITOR_OVERRIDE;

	void loadConstants() override;
};

struct ProjectInfo: public Constants
{
	HISE_MULTIPAGE_ID("ProjectInfo");

	ProjectInfo(Dialog& r, int w, const var& obj);;

    String getDescription() const override { return "Project Info";}

	void loadConstants() override;
};

struct PersistentSettings: public Constants
{
    HISE_MULTIPAGE_ID("PersistentSettings");

    PersistentSettings(Dialog& r, int w, const var& obj);

    String getDescription() const override { return "PersistentSettings";}

    CREATE_EDITOR_OVERRIDE;
    Result checkGlobalState(var globalState) override;
    void loadConstants() override;

private:

    File getSettingFile() const;
    bool useValueChildren() const;
    bool shouldUseJson() const;

    NamedValueSet defaultValues;
    NamedValueSet valuesFromFile;
};

} // factory
} // multipage
} // hise
