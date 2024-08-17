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

struct ScopedSetting
{
	ScopedSetting()
	{
		obj = JSON::parse(getSettingFile().loadFileAsString());
	}

	~ScopedSetting()
	{
		if(changed)
			getSettingFile().replaceWithText(JSON::toString(obj));
	}

	void set(const Identifier& id, const var& newValue)
	{
		if(auto no = obj.getDynamicObject())
		{
			no->setProperty(id, newValue);
			changed = true;
		}
	}

	var get(const Identifier& id, const var& defaultValue=var("")) const
	{
		return obj.getProperty(id, defaultValue);
	}

    static File getSettingFile()
	{
		auto f = File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory);
#if JUCE_MAC
        f = f.getChildFile("Application Support");
#endif
        return f.getChildFile("HISE").getChildFile("multipage.json");
	}

private:

	bool changed = false;
	var obj;

	
};

struct Asset: public ReferenceCountedObject
{
    using Ptr = ReferenceCountedObjectPtr<Asset>;
    using List = ReferenceCountedArray<Asset>;

    enum class TargetOS
    {
        All,
	    Windows,
        macOS,
        Linux,
        Other,
        numTargetOS
    };

	enum class Type
	{
		Image,
        File,
        Font,
        Text,
        Stylesheet,
        Archive,
        numTypes
	};

    enum class Filemode
    {
	    Absolute,
        Relative
    };

    static String getTypeString(Type t);

    static Type getType(const File& f);

    Image toImage() const
    {
	    if(type == Type::Image)
	    {
		    MemoryInputStream mis(data, false);
            return ImageCache::getFromMemory(data.getData(), (int)data.getSize());
	    }
        else
            return {};
    }

    Font toFont() const
    {
        if(type == Type::Font)
        {
	        return Font(Typeface::createSystemTypefaceFor(data.getData(), data.getSize()));
        }

        return Font(13.0f, Font::plain);
    }

    String toReferenceVariable() const
    {
	    return "${" + id + "}";
    }

    String toText(bool reloadIfPossible=false);

    simple_css::StyleSheet::Collection toStyleSheet(const String& additionalStyle) const
    {
        auto code = data.toString();
        code << additionalStyle;
        simple_css::Parser p(code);
        p.parse();
        return p.getCSSValues();
    }

    void writeCppLiteral(OutputStream& c, const String& newLine, ReferenceCountedObject* job) const;

    var toJSON(bool embedData=false, const File& currentRoot=File()) const;

    String getFilePath(const File& currentRoot) const;

    bool writeToFile(const File& targetFile, ReferenceCountedObject* job_) const;

    static String getOSName(TargetOS os);

    static Asset::Ptr fromFile(const File& f)
    {
	    auto type = getType(f);
        return new Asset(f, type);
    }

    static Asset::Ptr fromMemory(MemoryBlock&& mb, Type t, const String& filename, const String& id)
    {
        zstd::ZDefaultCompressor comp;
		comp.expandInplace(mb);
		auto a = new Asset(std::move(mb), t, id);
        a->filename = filename;

        if(mb.getSize() == 1)
            a->os = TargetOS::Other;

        return a;
    }

    static Asset::Ptr fromVar(const var& obj, const File& currentRoot);

    Asset(MemoryBlock&& mb, Type t, const String& id_):
      type(t),
      data(mb),
      id(id_)
    {
	    
    };

    Asset(const File& f, Type t):
      type(t),
      id("asset_" + String(f.getFullPathName().hash())),
      filename(f.getFullPathName())
    {
        
	    loadFromFile();
    }

    void loadFromFile();

    bool matchesOS() const
    {
		if(os == TargetOS::All)
            return true;

        if(os == TargetOS::Other)
            return false;

#if JUCE_WINDOWS
        return os == TargetOS::Windows;
#elif JUCE_MAC
        return os == TargetOS::macOS;
#elif JUCE_LINUX
        return os == TargetOS::Linux;
#else
        return false;
#endif
    }

    TargetOS os = TargetOS::All;
    const Type type;
    MemoryBlock data;
    String id;
    String filename;
    bool useRelativePath = false;
};

class State: public Thread,
			 public ApiProviderBase::Holder,
			 public ReferenceCountedObject,
			 public hlac::HlacArchiver::Listener
{
public:

    using Ptr = ReferenceCountedObjectPtr<State>;
    using WeakPtr = WeakReference<State>;
    
    State(const var& obj, const File& currentRootDirectory=File());;
    ~State();

    void onFinish();
    void run() override;

    void clearCompletedJobs()
    {
        stopThread(1000);
	    jobs.clear();
        completedJobs.clear();
    }

    struct StateProvider;

    void logStatusMessage(const String& message) override
    {
	    logMessage(MessageType::Hlac, message);
    }

    void logVerboseMessage(const String& verboseMessage) override
    {
	    logMessage(MessageType::Hlac, verboseMessage);
    }

	void criticalErrorOccured(const String& message) override
	{
        logMessage(MessageType::Hlac, message);
        jassertfalse;
	}

	ScopedPointer<ApiProviderBase> stateProvider;

    void reset(const var& obj);

    /** Override this method and return a provider if it exists. */
	ApiProviderBase* getProviderBase() override;

    Font loadFont(String fontName) const;

    void onDestroy();

    void addEventListener(const String& eventType, const var& functionObject);
    void removeEventListener(const String& eventType, const var& functionObject);
    void addCurrentEventGroup();
    void clearAndSetGroup(const String& groupId);
    void callEventListeners(const String& eventType, const Array<var>& args);

    simple_css::StyleSheet::Collection getStyleSheet(const String& name, const String& additionalStyle) const;

    JavascriptEngine* createJavascriptEngine();

    String getAssetReferenceList(Asset::Type t) const;

    String loadText(const String& assetVariable, bool forceReload) const;

    static NotificationType getNotificationTypeForCurrentThread()
    {
	    auto notification = sendNotificationSync;

		if(!MessageManager::getInstanceWithoutCreating()->isThisTheMessageThread())
			notification = sendNotificationAsync;

        return notification;
    }

    Image loadImage(const String& assetVariable) const;


    void setLogFile(const File& newLogFile);

    File logFile;
    File currentRootDirectory;

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

        virtual void setMessage(const String& newMessage);
        void updateProgressBar(ProgressBar* b) const;
        Thread& getThread() const { return parent; }

        var getInfoObject() const { return localObj; }

        void setInfoObject(const var& newInfoObject)
        {
            localObj = newInfoObject;
        }

        void setEnableProgressAndMessage(bool shouldEnable)
        {
            enableProgress = shouldEnable;
        }

    protected:

        bool enableProgress = true;

        String message;

        virtual Result run() = 0;
        
        State& parent;
        double progress = 0.0;
        double unusedProgess = 0.0;
        var localObj;
    };

    using HardcodedLambda = std::function<var(Job&, const var&)>;

    bool isFinished(Job::Ptr j) const { return completedJobs.contains(j); }

    Job::Ptr currentJob = nullptr;
    Job::Ptr getJob(const var& obj);
    var getGlobalSubState(const Identifier& id);
    void addJob(Job::Ptr b, bool addFirst=false);

    bool navigateOnFinish = false;

    double totalProgress = 0.0;
    
    Result currentError;
    Array<WeakReference<Dialog>> currentDialogs;

    WeakReference<Dialog> getFirstDialog() { return currentDialogs.getFirst(); }

    var globalState;
    int currentPageIndex = 0;

    void logMessage(MessageType t, const String& m)
    {
	    auto isMessageThread = MessageManager::getInstanceWithoutCreating()->isThisTheMessageThread();
        auto n = isMessageThread ? sendNotificationSync : sendNotificationAsync;
	    eventLogger.sendMessage(n, t, m);
    }

    LambdaBroadcaster<MessageType, String> eventLogger;

    void addTempFile(TemporaryFile* fileToOwnUntilShutdown)
    {
	    tempFiles.add(fileToOwnUntilShutdown);
    }

    var exportAssetsAsJSON(bool embedData) const
    {
	    Array<var> list;

        for(auto a: assets)
	        list.add(a->toJSON(embedData, currentRootDirectory));

        return var(list);
    }

    Asset::List assets;

    String getFileLog() const;

    void addFileToLog(const std::pair<File, bool>& fileOp);

    void bindCallback(const String& functionName, const var::NativeFunction& f);

    bool callNativeFunction(const String& functionName, const var::NativeFunctionArgs& args, var* returnValue);

    bool hasNativeFunction(const String& functionName) const
    {
	    return jsLambdas.find(functionName) != jsLambdas.end();
    }

    String currentEventGroup;
    
    std::map<String, Array<std::pair<String, var>>> eventListeners;

    Asset::Ptr getAsset(const var& infoObjectToUse, const Identifier& id)
    {
	    auto assetId = infoObjectToUse[id].toString().trim();

		if(assetId.startsWith("${"))
		{
			assetId = assetId.substring(2, assetId.length() - 1);

			for(auto a: assets)
			{
				if(a->id == assetId)
					return a;
			}
		}

		return nullptr;
    }


private:

    Job::List jobs;
    Job::List completedJobs;

    std::unique_ptr<JavascriptEngine> javascriptEngine;

    std::map<String, var::NativeFunction> jsLambdas;
    Array<std::pair<File, bool>> fileOperations;

    OwnedArray<TemporaryFile> tempFiles;
    JUCE_DECLARE_WEAK_REFERENCEABLE(State);
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(State);
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

class Dialog;    

struct MonolithData
{
    enum Markers
    {
	    MonolithBeginJSON = 9124,
        MonolithEndJSON,
        MonolithBeginAssets,
        MonolithAssetJSONStart,
        MonolithAssetJSONEnd,
        MonolithAssetNoCompressFlag,
        MonolithAssetStart,
        MonolithAssetEnd,
        MonolithEndAssets,
        MonolithBeginVersion,
        MonolithEndVersion
    };

    static String getMarkerName(Markers m);

    MonolithData(InputStream* input);
    
    multipage::Dialog* create(State& state, bool allowVersionMismatch);
    
    static Result exportMonolith(State& state, OutputStream* output, bool compressAssets=true, State::Job* j=nullptr);
    var getJSON() const;

private:

    int64 expectFlag(Markers m, bool throwIfMismatch=true);
    var readJSON(int64 numToRead);

    InputStream* input;
};

struct HardcodedDialogWithState: public Component
{
    HardcodedDialogWithState():
	  state(var())
	{};

	void setProperty(const Identifier& id, const var& newValue)
	{
		state.globalState.getDynamicObject()->setProperty(id, newValue);
	}

	var getProperty(const Identifier& id) const
	{
		return state.globalState[id];
	}

    /** Override this method and return an item list for the autocomplete popup for the given id*/
    virtual StringArray getAutocompleteItems(const Identifier& textEditorId) { return {}; };
    
    void setOnCloseFunction(const std::function<void()>& f);

    /** Override this method and initialise all default values for the global state. */
	virtual void postInit() {}

	void resized() override;
    
	virtual Dialog* createDialog(State& state) = 0;

    std::function<void()> closeFunction;
	State state;
    ScopedPointer<Dialog> dialog;
};

}
}
