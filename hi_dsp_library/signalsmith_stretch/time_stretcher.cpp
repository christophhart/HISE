#include "time_stretcher.h"

#define USE_JUCE_FFT 0

#if JUCE_WINDOWS
#define USE_VDSP_COMPLEX_MUL 0
#else
#define USE_VDSP_COMPLEX_MUL 1
#endif

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

void time_stretcher::process(float** input, int numInput, float** originalOutputs, int numOutput)
{
    if(auto sl = hise::SimpleReadWriteLock::ScopedTryReadLock(stretchLock))
    {
        float* outputs[2];

        outputs[0] = originalOutputs[0];
        outputs[1] = originalOutputs[1];

        if (playbackRatio != 1.0)
        {
            jassert(numOutput <= (resampledBuffer.size() / 2));

            outputs[0] = resampledBuffer.begin();
            outputs[1] = resampledBuffer.begin() + numOutput;
        }


        pimpl->process(input, numInput, outputs, numOutput);
        
        static constexpr float minus3dB = 0.707106781186548f;
        
        for(int i = 0; i < numChannels; i++)
            FloatVectorOperations::multiply(outputs[i], minus3dB, numOutput);

        if (playbackRatio != 1.0)
        {
            

            for (int c = 0; c < numChannels; c++)
            {
                block b(outputs[c], numOutput);

                double uptime = 0.0;

                auto ptr = originalOutputs[c];

                auto realNumOutputs = roundToInt((double)numOutput / playbackRatio);

                for (int i = 0; i < realNumOutputs; i++)
                {
                    auto loIndex = (int)uptime;
                    auto alpha = (float)uptime - (float)loIndex;
                    auto hiIndex = jmin(b.size() - 1, loIndex + 1);

                    auto lo = loIndex > 0 ? b[loIndex - 1] : lastValues[c];
                    auto hi = b[hiIndex - 1];

                    auto v = Interpolator::interpolateLinear(b[loIndex], b[hiIndex], alpha);

                    ptr[i] = v;
                    uptime += playbackRatio;
                }

                lastValues[c] = b[numOutput - 1];

                int x = 05;
            }
        }

    }
}

void time_stretcher::setTransposeSemitones(double semiTones, double tonality)
{
    pimpl->setTransposeSemitones(semiTones, tonality);
}

void time_stretcher::setResampleBuffer(double ratio, float* resampleBuffer_, int size)
{
    if (ratio != playbackRatio)
    {
        playbackRatio = ratio;

        if (ratio == 1.0)
            resampledBuffer.referToNothing();
        else
            resampledBuffer.referToRawData(resampleBuffer_, size);
    }
}
}
