namespace hise
{
using namespace juce;

Array<var> LorisManager::synthesise(const File& audioFile)
{
    if(auto numBytesFunc = (GetBufferSizeFunction)getFunction("getRequiredBytes"))
    {
        auto f2 = audioFile.getFullPathName();
        auto file = f2.getCharPointer().getAddress();
        
        auto numBytes = numBytesFunc(state, file);
        
        if(numBytes > 0)
        {
            if(auto f = (SynthesiseFunction)getFunction("loris_synthesize"))
            {
                HeapBlock<uint8> buffer;
                buffer.allocate(numBytes, true);
                
                auto asFloat = reinterpret_cast<float*>(buffer.get());
                
                int numSamples, numChannels;
                
                f(state, file, asFloat, numChannels, numSamples);
                
                if(!checkError())
                    return {};
                
                if(numSamples > 0)
                {
                    Array<var> buffers;
                    
                    for(int i = 0; i < numChannels; i++)
                    {
                        auto b = new VariantBuffer(numSamples);
                        FloatVectorOperations::copy(b->buffer.getWritePointer(0), asFloat, numSamples);
                        asFloat += numSamples;
                        
                        buffers.add(var(b));
                    }
                    
                    return buffers;
                }
                
            }
        }
    }
    
    return {};
}

var LorisManager::CustomPOD::toJSON() const
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

void LorisManager::CustomPOD::writeJSON(const var& obj_)
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

void* LorisManager::getFunction(const String& name) const
{
#if HISE_USE_LORIS_DLL
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
#elif HISE_INCLUDE_LORIS

    if (state != nullptr)
    {
        LorisLibrary::setThreadController(state, threadController.get());
    }
    
#define RETURN_STATIC_FUNCTION(x) if(name == #x) return (void*)LorisLibrary::x;

    RETURN_STATIC_FUNCTION(createLorisState);
	RETURN_STATIC_FUNCTION(destroyLorisState);
	RETURN_STATIC_FUNCTION(getLibraryVersion);
	RETURN_STATIC_FUNCTION(getLorisVersion);
	RETURN_STATIC_FUNCTION(loris_analyze);
	RETURN_STATIC_FUNCTION(loris_process);
	RETURN_STATIC_FUNCTION(loris_process_custom);
	RETURN_STATIC_FUNCTION(loris_set);
	RETURN_STATIC_FUNCTION(loris_get);
	RETURN_STATIC_FUNCTION(getRequiredBytes);
	RETURN_STATIC_FUNCTION(loris_synthesize);
	RETURN_STATIC_FUNCTION(loris_create_envelope);
	RETURN_STATIC_FUNCTION(loris_snapshot);
	RETURN_STATIC_FUNCTION(loris_prepare);
	RETURN_STATIC_FUNCTION(getLastMessage);
	RETURN_STATIC_FUNCTION(getIdList);
	RETURN_STATIC_FUNCTION(getLastError);
	RETURN_STATIC_FUNCTION(setThreadController);
    jassertfalse;

#undef RETURN_STATIC_FUNCTION
    
#endif
        
	return nullptr;
}

LorisManager::LorisManager(const File& hiseRoot_, const std::function<void(String)>& errorFunction_):
	lastError(Result::ok()),
	hiseRoot(hiseRoot_),
	errorFunction(errorFunction_)
{
#if HISE_USE_LORIS_DLL
    dll = new DynamicLibrary();

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
#elif HISE_INCLUDE_LORIS

    lorisVersion = ((GetLorisVersion)getFunction("getLorisVersion"))();
    state = ((LorisCreateFunction)getFunction("createLorisState"))();
    
#else
    
    errorFunction("Loris is disabled");

#endif
}

File LorisManager::getRedirectedFolder() const
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

File LorisManager::getLorisDll() const
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

StringArray LorisManager::getList(bool getOptions)
{
	if(auto f = (LorisGetListFunction)getFunction("getIdList"))
	{
		f(messageBuffer, 2048, getOptions);
		return StringArray::fromTokens(String(messageBuffer), ";", "");
	}
        
	return {};
}

void LorisManager::analyse(const Array<AnalyseData>& data)
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

double LorisManager::get(String command) const
{
	if(auto f = (LorisGetFunction)getFunction("loris_get"))
	{
		return f(state, command.getCharPointer().getAddress());
	}
        
	return 0.0;
}

bool LorisManager::set(String command, String value)
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

bool LorisManager::processCustomStatic(CustomPOD& data)
{
	auto typed = static_cast<LorisManager*>(data.obj);
        
	jassert(typed != nullptr);
	jassert(typed->customFunction);
        
	return typed->customFunction(data);
}

bool LorisManager::processCustom(const File& audioFile, const CustomPOD::Function& cf)
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

bool LorisManager::process(const File& audioFile, String command, const String& jsonData)
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

bool LorisManager::checkError()
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

void LorisManager::setLogFunction(const std::function<void(String)>& logFunction)
{
	lf = logFunction;
}

void LorisManager::checkMessages()
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

LorisManager::~LorisManager()
{
	if(state != nullptr)
	{
		if(auto f = (LorisDestroyFunction)getFunction("destroyLorisState"))
		{
			f(state);
			state = nullptr;
		}
	}

#if HISE_USE_LORIS_DLL
	dll->close();
	dll = nullptr;
#endif
}

Array<juce::var> LorisManager::createEnvelope(const juce::File &audioFile, const juce::Identifier &parameter, int index)
{
    if(auto numBytesFunc = (GetBufferSizeFunction)getFunction("getRequiredBytes"))
    {
        auto f2 = audioFile.getFullPathName();
        auto file = f2.getCharPointer().getAddress();
        
        auto numBytes = numBytesFunc(state, file);
        
        if(numBytes > 0)
        {
            if(auto f = (CreateEnvelopeFunction)getFunction("loris_create_envelope"))
            {
                HeapBlock<uint8> buffer;
                buffer.allocate(numBytes, true);
                
                auto asFloat = reinterpret_cast<float*>(buffer.get());
                
                int numSamples, numChannels;
                
                String p = parameter.toString();
                
                
                f(state, file, p.getCharPointer().getAddress(), index, asFloat, numChannels, numSamples);
                
                if(!checkError())
                    return {};
                
                if(numSamples > 0)
                {
                    Array<var> buffers;
                    
                    for(int i = 0; i < numChannels; i++)
                    {
                        auto b = new VariantBuffer(numSamples);
                        FloatVectorOperations::copy(b->buffer.getWritePointer(0), asFloat, numSamples);
                        asFloat += numSamples;
                        
                        buffers.add(var(b));
                    }
                    
                    return buffers;
                }
                
            }
        }
    }
    
    return {};
}

Range<double> LorisManager::getEnvelopeRange(const Identifier& id) const
{
    if(id == Identifier("rootFrequency") ||
       id == Identifier("frequency"))
    {
        float d = get("freqdrift");
        
        d = std::pow(2.0, d / 1200.0);
        
        const float min = 1.0f / d;
        const float max = 1.0f * d;
        
        return { min, max };
    }
    if(id == Identifier("gain"))
    {
        return { 0.0, 1.0 };
    }
    if(id == Identifier("phase"))
    {
        return { -1.0 * double_Pi, double_Pi };
    }
    if(id == Identifier("bandwidth"))
    {
        return { 0.0, 1.0 };
    }
    
    jassertfalse;
    return {};
}

juce::Path LorisManager::setEnvelope(const juce::var &bf, const juce::Identifier &id)
{
    Path ep;
    
    if(auto b = bf.getBuffer())
    {
        bool stop = false;
        
        auto validRange = getEnvelopeRange(id);
        
        ep.startNewSubPath(0.0f, validRange.getStart());
        ep.startNewSubPath(0.0f, validRange.getEnd());
        ep.startNewSubPath(0.0f, 1.0f);
        
        auto samplesPerPixel = 3 * b->size / 600;
        
        for(float i = 0.0f; i < (float)b->size; i += samplesPerPixel)
        {
            auto numThisTime = jmin<int>(samplesPerPixel, b->size - i);
            
            auto v = b->buffer.getMagnitude(0, i, numThisTime);
            v = validRange.getEnd() - v;
            
            FloatSanitizers::sanitizeFloatNumber(v);
            
            if(validRange.contains(v))
            {
                if(stop)
                {
                    ep.startNewSubPath((float)i, v);
                    stop = false;
                }
                else
                {
                    ep.lineTo({(float)i, v});
                }
            }
            else
            {
                stop = true;
            }
        }
        
        
        ep.lineTo((float)b->size, 1.0f);
        ep.closeSubPath();
    }
    
    return ep;
}

var LorisManager::getSnapshot(const juce::File &audioFile, double time, const juce::Identifier &parameter)
{
    Array<var> list;
    
    if(auto f = (LorisGetSnapshot)getFunction("loris_snapshot"))
    {
        auto f2 = audioFile.getFullPathName();
        auto file = f2.getCharPointer().getAddress();
        
        auto p = parameter.toString();
        
        int numHarmonics, numChannels;
        
        HeapBlock<double> buffer;
        
        // should be enough...
        buffer.calloc(NUM_MAX_CHANNELS * 512);
        
        if(f(state, file, time, p.getCharPointer().getAddress(), buffer.get(), numChannels, numHarmonics))
        {
            auto d = buffer.get();
            
            for(int c = 0; c < numChannels; c++)
            {
                Array<var> channelData;
                
                for(int i = 0; i < numHarmonics; i++)
                    channelData.add(*d++);
                
                list.add(var(channelData));
            }
        }
    }
    
    return var(list);
}




}
