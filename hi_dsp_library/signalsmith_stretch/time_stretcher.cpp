#include "time_stretcher.h"

#define USE_JUCE_FFT 1
#define USE_VDSP_COMPLEX_MUL 1

#include "signalsmith-stretch.h"

namespace scriptnode {
using namespace hise;
using namespace juce;

time_stretcher::time_stretcher()
{
    pimpl = new signalsmith::stretch::FloatStretcher();
}

time_stretcher::~time_stretcher()
{
    delete pimpl;
}

void time_stretcher::reset()
{
    SimpleReadWriteLock::ScopedWriteLock sl(stretchLock);
    pimpl->reset();
}

void time_stretcher::configure(int numChannels_, double sourceSampleRate_)
{
    if(numChannels != numChannels_ ||
       sourceSampleRate != sourceSampleRate_)
    {
        SimpleReadWriteLock::ScopedWriteLock sl(stretchLock);
        
        numChannels = numChannels_;
        sourceSampleRate = sourceSampleRate_;
     
        if(numChannels > 0 && sourceSampleRate > 0)
        {
            //pimpl->presetDefault(numChannels, sourceSampleRate);
            
            
            
            pimpl->configure(numChannels, 4096, 512);
            pimpl->reset();
        }
    }
}

double time_stretcher::skipLatency(float** inputs, double ratio)
{
    hise::SimpleReadWriteLock::ScopedWriteLock sl(stretchLock);
    
    pimpl->reset();
    
    auto numBeforeOutput = 2048;
    
    float* thisInputs[2];
    float* outputs[2];
    
    thisInputs[0] = inputs[0];
    thisInputs[1] = inputs[1];
    
    double currentPos = 0.0;
    
    while(numBeforeOutput > 0)
    {
        int numThisTime = jmin(numBeforeOutput, 1024);
        int numInputs = ratio * (double)numThisTime;
        
        outputs[0] = (float*)alloca(numThisTime * sizeof(float));
        outputs[1] = (float*)alloca(numThisTime * sizeof(float));
        
        pimpl->process(thisInputs, numInputs, outputs, numThisTime);
        
        currentPos += numInputs;
        
        thisInputs[0] = inputs[0] + (int)currentPos;
        thisInputs[1] = inputs[1] + (int)currentPos;
        
        numBeforeOutput -= numThisTime;
    }
    
    return currentPos;
}

void time_stretcher::process(float** input, int numInput, float** output, int numOutput)
{
    if(auto sl = hise::SimpleReadWriteLock::ScopedTryReadLock(stretchLock))
    {
        pimpl->process(input, numInput, output, numOutput);
        
        static constexpr float minus3dB = 0.707106781186548f;
        
        for(int i = 0; i < numChannels; i++)
            FloatVectorOperations::multiply(output[i], minus3dB, numOutput);
    }
}

void time_stretcher::setTransposeSemitones(double semiTones, double tonality)
{
    pimpl->setTransposeSemitones(semiTones, tonality);
}

}
