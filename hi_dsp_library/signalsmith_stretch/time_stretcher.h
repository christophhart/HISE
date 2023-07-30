#pragma once

namespace signalsmith {
namespace stretch {
struct FloatStretcher;
}}

namespace scriptnode
{
using namespace hise;
using namespace juce;

/** A pimpl wrapper around the signalsmith stretcher. */
struct time_stretcher
{
    void reset();
    void process(float** input, int numInput, float** outputs, int numOutput);
    void configure(int numChannels, double sourceSampleRate);
    void setTransposeSemitones(double semitTones, double tonality=0.0);
    
    void setResampleBuffer(double ratio, float* resampleBuffer_, int size);

    double skipLatency(float** input, double ratio);
    
    time_stretcher();
    ~time_stretcher();
    
private:
    
    double playbackRatio = 0.0;
    block resampledBuffer;

    span<float, 2> lastValues;

    int numChannels = 0;
    double sourceSampleRate = 0.0;
    
    hise::SimpleReadWriteLock stretchLock;
    signalsmith::stretch::FloatStretcher* pimpl;
};

}
