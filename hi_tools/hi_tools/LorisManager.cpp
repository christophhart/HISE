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
