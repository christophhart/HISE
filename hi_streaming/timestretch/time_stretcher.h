#pragma once

#ifndef HISE_ENABLE_RUBBERBAND
#define HISE_ENABLE_RUBBERBAND 0
#endif



struct timestretch_engine_base
{
    virtual ~timestretch_engine_base() {};

    virtual juce::Identifier getEngineId() const = 0;

    virtual void process(float** input, int numInput, float** output, int numOutput) = 0;
    virtual void reset() = 0;
    virtual void configure(int numChannels, double sourceSampleRate) = 0;

    virtual void setFFTSize(int blockSamples, int intervalSamples) = 0;

    virtual void setTransposeSemitones(double semiTones, double tonality = 0.0) = 0;
    virtual void setTransposeFactor(double pitchFactor, double tonality = 0.0) = 0;
    virtual void setEnableOutput(bool shouldBeEnabled) = 0;

    virtual double getLatency(double ratio) const = 0;
};

namespace hise
{
using namespace juce;

/** A pimpl wrapper around the signalsmith stretcher. */
struct time_stretcher
{
    using EngineFactoryFunction = std::function<timestretch_engine_base* (const Identifier& id)>;

    time_stretcher(bool enabled = true);

    void reset();
    void process(float** input, int numInput, float** outputs, int numOutput);
    void configure(int numChannels, double sourceSampleRate);
    void setTransposeSemitones(double semiTones, double tonality=0.0);
    void setTransposeFactor(double pitchFactor, double tonality = 0.0);

    void setFFTSize(int blockSamples, int intervalSamples);

    void setResampleBuffer(double ratio, float* resampleBuffer_, int totalNumFloats);

    double skipLatency(float** input, double ratio);

    double getLatency(double ratio) const;

    time_stretcher();
    ~time_stretcher();
    
    bool isEnabled() const;

    void setEnabled(bool shouldBeEnabled, const Identifier& engineToUse={});
    
    static void registerEngines(time_stretcher& t);

private:

    Identifier getCurrentEngine() const;

    static Identifier getDefaultEngineId();

    Array<EngineFactoryFunction> availableEngines;

    double playbackRatio = 0.0;
    AudioSampleBuffer resampledBuffer;

    int numChannels = 0;
    double sourceSampleRate = 0.0;
    
    CriticalSection stretchLock;

    ScopedPointer<timestretch_engine_base> engine;
};

}
