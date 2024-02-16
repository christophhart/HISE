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
    HISE_MULTIPAGE_ID("Action");
    
    Action(Dialog& r, int, const var& obj);

    static String getCategoryId() { return "Actions"; }
    virtual String getDescription() const = 0;


    template <typename T> void createBasicEditor(T& t, Dialog::PageInfo& rootList, const String& helpText);

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
    ImmediateAction(Dialog& r,int w, const var& obj);;

    virtual bool skipIfStateIsFalse() const = 0;

	virtual Result onAction() = 0;
};

struct Skip: public ImmediateAction
{
    HISE_MULTIPAGE_ID("Skip");

    Skip(Dialog& r, int w, const var& obj);

    bool skipIfStateIsFalse() const override { return true; }

    void createEditor(Dialog::PageInfo& rootList) override;
    Result onAction() override;
    String getDescription() const override { return "skipPage()"; };
};

struct LinkFileWriter: public ImmediateAction
{
	HISE_MULTIPAGE_ID("LinkFileWriter");

    LinkFileWriter(Dialog& r, int w, const var& obj);

    bool skipIfStateIsFalse() const override { return false; }

    void paint(Graphics& g) override;

    void createEditor(Dialog::PageInfo& rootList) override;

    Result onAction() override;
	String getDescription() const override { return "writeLinkFile(" + infoObject[mpid::ID].toString() + ")"; };

    File targetFile;
};

struct RelativeFileLoader: public ImmediateAction
{
	HISE_MULTIPAGE_ID("RelativeFileLoader");

    RelativeFileLoader(Dialog& r, int w, const var& obj);

    bool skipIfStateIsFalse() const override { return false; }

	void createEditor(Dialog::PageInfo& rootList) override;
	

	static StringArray getSpecialLocations();

	Result onAction() override;

	String getDescription() const override { return "FileLoader"; };
};



struct Launch: public ImmediateAction
{
    HISE_MULTIPAGE_ID("Launch");

    Launch(Dialog& r, int w, const var& obj);

    bool skipIfStateIsFalse() const override { return true; }

    void createEditor(Dialog::PageInfo& rootList) override;
    Result onAction() override;
    String getDescription() const override;;

private:

    bool isFinished = false;
    String currentLaunchTarget;
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
    void postInit() override;

    virtual Result performTask(State::Job& t) = 0;

    Result checkGlobalState(var globalState) override;

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
    ScopedPointer<ProgressBar> progress;
    HiseShapeButton retryButton;

private:

    File getFileInternal(const Identifier& id) const;

    JUCE_DECLARE_WEAK_REFERENCEABLE(BackgroundTask);
};

struct LambdaTask: public BackgroundTask
{
	HISE_MULTIPAGE_ID("LambdaTask");

    LambdaTask(Dialog& r, int w, const var& obj);;

    Result performTask(State::Job& t) override;
	String getDescription() const override;
	void createEditor(Dialog::PageInfo& rootList) override;

    var::NativeFunction lambda;
};


struct HttpRequest: public BackgroundTask
{
	HISE_MULTIPAGE_ID("HttpRequest");

    HttpRequest(Dialog& r, int w, const var& obj);;

    Result performTask(State::Job& t) override;
	String getDescription() const override { return "HttpRequest"; }
	void createEditor(Dialog::PageInfo& rootList) override;

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

    void createEditor(Dialog::PageInfo& rootList) override;

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

    void createEditor(Dialog::PageInfo& rootList) override;

    String getDescription() const override { return "Unzip Action"; }

    bool overwrite = true;
};



struct HlacDecoder: public BackgroundTask,
				    public hlac::HlacArchiver::Listener
{
    HISE_MULTIPAGE_ID("HlacDecoder");

    HlacDecoder(Dialog& r, int w, const var& obj);

    ~HlacDecoder() override;

    Result performTask(State::Job& t) override;

    void logStatusMessage(const String& message) override { currentJob->setMessage(message); }
	void logVerboseMessage(const String& verboseMessage) override {};
	void criticalErrorOccured(const String& message) override { r = Result::fail(message); }

    void createEditor(Dialog::PageInfo& rootList) override;

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

	void createEditor(Dialog::PageInfo& rootList) override;

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

struct ProjectInfo: public Constants
{
	HISE_MULTIPAGE_ID("ProjectInfo");

	ProjectInfo(Dialog& r, int w, const var& obj):
      Constants(r, w, obj)
    {};

    String getDescription() const override { return "Project Info";}

	void loadConstants() override;
};

} // factory
} // multipage
} // hise
