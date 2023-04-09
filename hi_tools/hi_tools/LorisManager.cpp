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

}
