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
        
        var toJSON() const;

        void writeJSON(const var& obj_);
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
    
    void* getFunction(const String& name) const;

    LorisManager(const File& hiseRoot_, const std::function<void(String)>& errorFunction_);

    File getRedirectedFolder() const;

    File getLorisDll() const;

    StringArray getList(bool getOptions);

    void analyse(const Array<AnalyseData>& data);

    Array<var> createEnvelope(const File& audioFile, const Identifier& parameter, int index);
    
    
    double get(String command) const;

    bool set(String command, String value);

    Range<double> getEnvelopeRange(const Identifier& id) const;
    
    Path setEnvelope(const var& bf, const Identifier& id);
    
    
    
    static bool processCustomStatic(CustomPOD& data);

    bool processCustom(const File& audioFile, const CustomPOD::Function& cf);

    bool process(const File& audioFile, String command, const String& jsonData);

    Array<var> synthesise(const File& audioFile);
    
    
    bool checkError();

    var getSnapshot(const File& f, double time, const Identifier& parameter);
    
    void setLogFunction(const std::function<void(String)>& logFunction);

    void checkMessages();

    ~LorisManager();

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
