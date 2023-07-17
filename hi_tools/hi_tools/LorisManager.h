namespace hise
{
using namespace juce;

#ifndef HISE_LORIS_LIBRARY_MAJOR_VERSION
#define HISE_LORIS_LIBRARY_MAJOR_VERSION 0
#define HISE_LORIS_LIBRARY_MINOR_VERSION 2
#define HISE_LORIS_LIBRARY_PATCH_VERSION 2
#endif

struct LorisManager: public ReferenceCountedObject
{
    struct CustomPOD
    {
        using FunctionType = bool(*)(CustomPOD&);
        using Function = std::function<bool(CustomPOD&)>;
        
        // Constants
        int channelIndex = 0;
        int partialIndex = 0;
        double sampleRate = 44100.0;
        double rootFrequency = 0.0;
        void* obj = nullptr;
        
        // Variable properties
        double time = 0.0;
        double frequency = 0.0;
        double phase = 0.0;
        double gain = 1.0;
        double bandwidth = 0.0;
        
        var toJSON() const
        {
            DynamicObject::Ptr obj = new DynamicObject();
            
            obj->setProperty("channelIndex", channelIndex);
            obj->setProperty("partialIndex", partialIndex);
            obj->setProperty("sampleRate", sampleRate);
            obj->setProperty("rootFrequency", rootFrequency);
            obj->setProperty("time", time);
            obj->setProperty("frequency", frequency);
            obj->setProperty("phase", phase);
            obj->setProperty("gain", gain);
            obj->setProperty("bandwidth", bandwidth);
            
            return var(obj.get());
        }
        
        void writeJSON(const var& obj_)
        {
            auto obj = obj_.getDynamicObject();
            
            //channelIndex = obj->getProperty("channelIndex");
            //partialIndex = obj->getProperty("partialIndex");
            //sampleRate = obj->getProperty("sampleRate");
            //rootFrequency = obj->getProperty("rootFrequency");
            
            time = obj->getProperty("time");
            frequency = obj->getProperty("frequency");
            phase = obj->getProperty("phase");
            gain = obj->getProperty("gain");
            bandwidth = obj->getProperty("bandwidth");
        }
    };
    
    using Ptr = ReferenceCountedObjectPtr<LorisManager>;
    
    using GetLorisVersion = char*(*)();
    using LorisAnalyseFunction = bool(*)(void*, char*, double);
    using LorisCreateFunction = void*(*)(void);
    using LorisDestroyFunction = void(*)(void*);
    using LorisErrorFunction = char*(*)(void*);
    using LorisMessageFunction = bool(*)(void*, char*, int);
    using GetBufferSizeFunction = size_t(*)(void*, const char*);
    using SynthesiseFunction = bool(*)(void*, const char*, float*, int&, int&);
    using CreateEnvelopeFunction = bool(*)(void*, const char*, const char*, int, float*, int&, int&);
    using LorisProcessFunction = bool(*)(void*, const char*, const char*, const char*);
    using LorisGetFunction = double(*)(void*, const char*);
    using LorisSetFunction = bool(*)(void*, const char*, const char*);
    using LorisGetListFunction = void(*)(char*, int, bool);
    using LorisCustomFunction = void(*)(void*, const char*, void*, void*);
    using LorisGetSnapshot = bool(*)(void*, const char*, double,const char*,double*,int&,int&);
	using LorisSetThreadController = void(*)(void*, void*);

    struct AnalyseData
    {
        File file;
        double rootFrequency;
    };
    
    void* getFunction(const String& name) const
    {
        if(dll != nullptr)
        {
            if(auto f = dll->getFunction(name))
			{
				auto tf = (LorisSetThreadController)dll->getFunction("setThreadController");

				if (state != nullptr && tf != nullptr)
				{
					tf(state, threadController.get());
				}

				return f;
			}
            else
                errorFunction("Can't find function pointer for " + name + "()");
        }
        
        return nullptr;
    }
    
    LorisManager(const File& hiseRoot_, const std::function<void(String)>& errorFunction_):
      lastError(Result::ok()),
      hiseRoot(hiseRoot_),
      dll(new DynamicLibrary()),
      errorFunction(errorFunction_)
    {
        auto dllFile = getLorisDll();
        
        if(!dllFile.existsAsFile())
            
        {
            errorFunction("Can't find Loris DLL. Make sure that you have copied the dynamic library file to " + getLorisDll().getFullPathName());
            
            return;
        }
        
        auto ok = dll->open(dllFile.getFullPathName());
        
        if(ok)
        {
            if(auto f = (GetLorisVersion)getFunction("getLorisVersion"))
            {
                auto r = f();
                lorisVersion = String(r);
            }
            
            if(auto f = (GetLorisVersion)getFunction("getLibraryVersion"))
            {
                auto libraryVersion = String(f());
                
                String thisVersion;
                
                thisVersion << String(HISE_LORIS_LIBRARY_MAJOR_VERSION) << ".";
                thisVersion << String(HISE_LORIS_LIBRARY_MINOR_VERSION) << ".";
                thisVersion << String(HISE_LORIS_LIBRARY_PATCH_VERSION);
                
                if(libraryVersion != thisVersion)
                {
                    lorisVersion = "Loris DLL Library version mismatch: ";
                    lorisVersion << libraryVersion << " vs. " << thisVersion;
                    errorFunction(lorisVersion);
                }
                
                
            }
            
            if(auto f = (LorisCreateFunction)getFunction("createLorisState"))
            {
                state = f();
            }
        }
        else
        {
            errorFunction("Can't open Loris DLL...");
        }
    }
    
    File getRedirectedFolder() const
    {
        
#if JUCE_WINDOWS
        String name = "LinkWindows";
#elif JUCE_MAC
        String name = "LinkOSX";
#else
        String name = "LinkLinux";
#endif
        
#if JUCE_DEBUG
        name << "Debug";
#else
		name << "Release";
#endif
        
        auto linkFile = hiseRoot.getChildFile(name);
        
        if(linkFile.existsAsFile())
            return File(linkFile.loadFileAsString());
        
        return hiseRoot;
    }
    
    File getLorisDll() const
    {
        auto root = getRedirectedFolder();
        
#if JUCE_DEBUG
        String config = "debug";
#else
        String config = "release";
#endif
        
#if JUCE_WINDOWS
        String ext = ".dll";
#elif JUCE_MAC
        String ext = ".dylib";
#else
        String ext = ".so";
#endif
        
        String fileName = "loris_library_";
        fileName << config << ext;
        
        return getRedirectedFolder().getChildFile(fileName);
    }
    
    StringArray getList(bool getOptions)
    {
        if(auto f = (LorisGetListFunction)getFunction("getIdList"))
        {
            f(messageBuffer, 2048, getOptions);
            return StringArray::fromTokens(String(messageBuffer), ";", "");
        }
        
        return {};
    }
    
    void analyse(const Array<AnalyseData>& data)
    {
        if(auto f = (LorisAnalyseFunction)getFunction("loris_analyze"))
        {
            for(const auto& ad: data)
            {
                auto f2 = ad.file.getFullPathName();
                auto file = f2.getCharPointer().getAddress();
                f(state, file, ad.rootFrequency);
             
                if(!checkError())
                    return;
            }
        }
    }
    
    Array<var> createEnvelope(const File& audioFile, const Identifier& parameter, int index);
    
    
    double get(String command) const
    {
        if(auto f = (LorisGetFunction)getFunction("loris_get"))
        {
            return f(state, command.getCharPointer().getAddress());
        }
        
        return 0.0;
    }
    
    bool set(String command, String value)
    {
        if(auto f = (LorisSetFunction)getFunction("loris_set"))
        {
            f(state, command.getCharPointer().getAddress(), value.getCharPointer().getAddress());
            
            if(!checkError())
                return false;
            
            return true;
        }
        
        return false;
    }
    
    Range<double> getEnvelopeRange(const Identifier& id) const;
    
    Path setEnvelope(const var& bf, const Identifier& id);
    
    
    
    static bool processCustomStatic(CustomPOD& data)
    {
        auto typed = static_cast<LorisManager*>(data.obj);
        
        jassert(typed != nullptr);
        jassert(typed->customFunction);
        
        return typed->customFunction(data);
    }
    
    bool processCustom(const File& audioFile, const CustomPOD::Function& cf)
    {
        customFunction = cf;
        
        if(auto f = (LorisCustomFunction)getFunction("loris_process_custom"))
        {
            auto f2 = audioFile.getFullPathName();
            auto file = f2.getCharPointer().getAddress();
            
            f(state, file, this, (void*)LorisManager::processCustomStatic);
            
            return true;
        }
        
        return false;
        
    }
    
    bool process(const File& audioFile, String command, const String& jsonData)
    {
        if(command.isEmpty())
        {
            lastError = Result::fail("Can't find command");
            return false;
        }
        
        var data;
        
        auto ok = JSON::parse(jsonData, data);
        
        if(!ok.wasOk())
        {
            lastError = ok;
            return false;
        }
            
        auto c = juce::JSON::toString(data);
        
        if(auto f = (LorisProcessFunction)getFunction("loris_process"))
        {
            auto f2 = audioFile.getFullPathName();
            auto file = f2.getCharPointer().getAddress();
            
            auto ok = f(state,
                        file,
                        command.getCharPointer().getAddress(),
                        c.getCharPointer().getAddress());
            
            if(!checkError())
                return false;
            
            return ok;
        }
        
        return false;
    }
    
    Array<var> synthesise(const File& audioFile);
    
    
    bool checkError()
    {
        checkMessages();
        
        if(auto f = (LorisErrorFunction)getFunction("getLastError"))
        {
            auto msg = f(state);
            
            String e(msg);
            
            if(e.isEmpty())
                lastError = Result::ok();
            else
            {
                lastError = Result::fail(e);
            }
            
            if(lastError.failed())
                errorFunction(e);
        }
        
        return lastError.wasOk();
    }
    
    var getSnapshot(const File& f, double time, const Identifier& parameter);
    
    void setLogFunction(const std::function<void(String)>& logFunction)
    {
        lf = logFunction;
    }
    
    void checkMessages()
    {
        if(auto f = (LorisMessageFunction)getFunction("getLastMessage"))
        {
            while(f(state, messageBuffer, 2048))
            {
				if (lf)
				{
					String nm(messageBuffer);
					lf(nm);
				}
            }
        }
    }
    
    ~LorisManager()
    {
        if(state != nullptr)
        {
            if(auto f = (LorisDestroyFunction)getFunction("destroyLorisState"))
            {
                f(state);
                state = nullptr;
            }
        }
        
        dll->close();
        dll = nullptr;
    }
    
    char messageBuffer[2048];
    String lorisVersion;
    
	mutable ThreadController::Ptr threadController;

    std::function<void(String)> lf;
    std::function<void(String)> errorFunction;
    
    Result lastError;
    StringArray notifications;
    
    CustomPOD::Function customFunction;
    
    ScopedPointer<DynamicLibrary> dll;
    
    File hiseRoot;
    
    void* state = nullptr;
};

}
