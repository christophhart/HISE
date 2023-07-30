#include "time_stretcher.h"

#define USE_JUCE_FFT 1

#if JUCE_WINDOWS
#define USE_VDSP_COMPLEX_MUL 0
#else
#define USE_VDSP_COMPLEX_MUL 1
#endif

#include "signalsmith-stretch.h"

namespace hise {
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
    ScopedLock sl(stretchLock);
    pimpl->reset();
}

void time_stretcher::configure(int numChannels_, double sourceSampleRate_)
{
    if(numChannels != numChannels_ ||
       sourceSampleRate != sourceSampleRate_)
    {
        ScopedLock sl(stretchLock);
        
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
    ScopedLock sl(stretchLock);
    
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
    juce::ScopedTryLock sl(stretchLock);

    if(sl.isLocked())
    {
        float* outputs[2];

        outputs[0] = originalOutputs[0];
        outputs[1] = originalOutputs[1];

        if (playbackRatio != 1.0)
        {
            jassert(numOutput <= (resampledBuffer.getNumSamples() / 2));

            outputs[0] = resampledBuffer.getWritePointer(0);
            outputs[1] = resampledBuffer.getWritePointer(1);
        }

        pimpl->process(input, numInput, outputs, numOutput);
        
        static constexpr float minus3dB = 0.5;//0.707106781186548f;
        
        for(int i = 0; i < numChannels; i++)
            FloatVectorOperations::multiply(outputs[i], minus3dB, numOutput);

        if (playbackRatio != 1.0)
        {
            for (int c = 0; c < numChannels; c++)
            {
                auto b = outputs[c];
                auto size = numOutput;

                double uptime = 0.0;

                auto ptr = originalOutputs[c];

                auto realNumOutputs = roundToInt((double)numOutput / playbackRatio);

                for (int i = 0; i < realNumOutputs; i++)
                {
                    auto loIndex = (int)uptime;
                    auto alpha = (float)uptime - (float)loIndex;
                    auto hiIndex = jmin(size - 1, loIndex + 1);

                    auto lo = b[loIndex];
                    auto hi = b[hiIndex];

                    auto v = Interpolator::interpolateLinear(b[loIndex], b[hiIndex], alpha);

                    ptr[i] = v;
                    uptime += playbackRatio;
                }
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

        if (playbackRatio != 1.0)
        {
            float* s[2];

            s[0] = resampleBuffer_;
            s[1] = resampleBuffer_ + size;


            resampledBuffer = AudioSampleBuffer(s, 2, size);
        }
        else
        {
            resampledBuffer = {};
        }
    }
}
}
