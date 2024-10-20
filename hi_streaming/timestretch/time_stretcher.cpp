#include "time_stretcher.h"

#if HISE_ENABLE_RUBBERBAND
#define HISE_DEFAULT_TIMESTRETCH_ENGINE "rubberband_v3"
#else
#define HISE_DEFAULT_TIMESTRETCH_ENGINE "signalsmith"
#endif

#define USE_JUCE_FFT 1

#if JUCE_WINDOWS
#define USE_VDSP_COMPLEX_MUL 0
#else
#define USE_VDSP_COMPLEX_MUL 1
#endif

#include "signalsmith_stretch/signalsmith-stretch.h"

#if HISE_ENABLE_RUBBERBAND

#if USE_IPP
#define HAVE_IPP 1
#endif

// If you want to use rubberband (which isn't supported by default)
// you need to put the files here and set the HISE_ENABLE_RUBBERBAND
// flag to true
// Also make sure that you comply with the GPL license or get a 
// commercial license when using Rubberband...
#include "rubberband/single/RubberBandSingle.cpp"
#endif

namespace hise {
using namespace juce;

struct signal_smith_stretcher: public timestretch_engine_base
{
    static Identifier getStaticId() { return Identifier("signalsmith"); }

    Identifier getEngineId() const override { return getStaticId(); }

    void process(float** input, int numInput, float** output, int numOutput) override
    {
        stretcher.process(input, numInput, output, numOutput);

        static constexpr float minus3dB = 0.5;//0.707106781186548f;

        for (int i = 0; i < numChannels; i++)
            FloatVectorOperations::multiply(output[i], minus3dB, numOutput);
    }

    void reset() override
    {
        stretcher.reset();
    }

    void setFFTSize(int blockSamples_=4096, int intervalSamples_=512)
    {
	    blockSamples = blockSamples_;
        intervalSamples = intervalSamples_;
        configure(numChannels, 44100.0);
    }

    void configure(int numChannels_, double sourceSampleRate) override
    {
        numChannels = numChannels_;
        stretcher.configure(numChannels, blockSamples, intervalSamples);
    }

    void setTransposeSemitones(double semiTones, double tonality = 0.0) override
    {
        stretcher.setTransposeSemitones((float)semiTones, (float)tonality);
    }

    void setTransposeFactor(double pitchFactor, double tonality = 0.0) override
    {
        stretcher.setTransposeFactor((float)pitchFactor, (float)tonality);
    }

    double getLatency(double ratio) const override
	{
    	return  stretcher.outputLatency() + stretcher.inputLatency() * ratio;
    }

    void setEnableOutput(bool shouldBeEnabled)
    {
        stretcher.setEnableOutput(shouldBeEnabled);
    }

    signalsmith::stretch::SignalsmithStretch<float> stretcher;
    int numChannels = 2;

    int blockSamples = 4096;
    int intervalSamples = 512;
};

#if HISE_ENABLE_RUBBERBAND
// this isn't really supported at the moment (currently the first audio buffers will glitch around)
// and I'm not too motivated to fix this given that the other engine works fine...
struct rubberband_stretcher_base: public timestretch_engine_base
{
    virtual ~rubberband_stretcher_base() {}

    void process(float** input, int numInput, float** output, int numOutput) final
    {
        if(stretcher != nullptr)
        {
            auto r = double(numOutput) / double(numInput);

            stretcher->setTimeRatio(r);
            stretcher->process(input, numInput, false);

            auto numAvailable = jmin(stretcher->available(), numOutput);

            stretcher->retrieve(output, numAvailable);
        }
    }

    void reset() final
    {
        if(stretcher != nullptr)
			stretcher->reset();
    }

    virtual RubberBand::RubberBandStretcher::Option getOption() const = 0;

    void configure(int numChannels, double sourceSampleRate) final
    {
        stretcher = new RubberBand::RubberBandStretcher(sourceSampleRate, numChannels, getOption());
        stretcher->setMaxProcessSize(1024);
    }

    void setTransposeSemitones(double semiTones, double tonality = 0.0) final
    {
        auto pr = std::pow(2.0, semiTones / 12.0);
        setTransposeFactor(pr, tonality);
    }

    void setTransposeFactor(double pitchFactor, double tonality = 0.0) final
    {
        if(stretcher != nullptr)
			stretcher->setPitchScale(pitchFactor);
    }

    double getLatency(double ratio) const final
    {
        return stretcher != nullptr ? stretcher->getStartDelay() : 0.0;
    }

    void setEnableOutput(bool shouldBeEnabled)
    {
        
    }

    ScopedPointer<RubberBand::RubberBandStretcher> stretcher;
};

struct rubberband_v3 : public rubberband_stretcher_base
{
    static Identifier getStaticId() { return Identifier("rubberband_v3"); }

    Identifier getEngineId() const override { return getStaticId(); }

    RubberBand::RubberBandStretcher::Option getOption() const override
    {
        int o = 0;

        o |= RubberBand::RubberBandStretcher::Option::OptionProcessRealTime;
        o |= RubberBand::RubberBandStretcher::Option::OptionEngineFiner;
        o |= RubberBand::RubberBandStretcher::Option::OptionChannelsTogether;
        o |= RubberBand::RubberBandStretcher::Option::OptionThreadingNever;
        o |= RubberBand::RubberBandStretcher::Option::OptionWindowShort;

        return static_cast<RubberBand::RubberBandStretcher::Option>(o);
    }
};

struct rubberband_v2: public rubberband_stretcher_base
{
    static Identifier getStaticId() { return Identifier("rubberband_v2"); }

    Identifier getEngineId() const override { return getStaticId(); }

	RubberBand::RubberBandStretcher::Option getOption() const override
	{
        int o = 0;

        o |= RubberBand::RubberBandStretcher::Option::OptionProcessRealTime;
        o |= RubberBand::RubberBandStretcher::Option::OptionEngineFaster;
        o |= RubberBand::RubberBandStretcher::Option::OptionChannelsTogether;
        o |= RubberBand::RubberBandStretcher::Option::OptionThreadingNever;
        o |= RubberBand::RubberBandStretcher::Option::OptionWindowShort;

        return static_cast<RubberBand::RubberBandStretcher::Option>(o);
	}
};
#endif

time_stretcher::time_stretcher(bool enabled):
    engine(nullptr)
{
    registerEngines(*this);

    setEnabled(enabled);
}

time_stretcher::~time_stretcher()
{
    engine = nullptr;
}

bool time_stretcher::isEnabled() const
{
    return engine != nullptr;
}

void time_stretcher::setEnabled(bool shouldBeEnabled, const Identifier& engineToUse)
{
    auto eid = engineToUse.isValid() ? engineToUse : getDefaultEngineId();
    auto switchEngine = engine != nullptr && engine->getEngineId() != eid;

    if (isEnabled() != shouldBeEnabled || switchEngine)
    {
        ScopedLock sl(stretchLock);

        if (!shouldBeEnabled)
        {
            engine = nullptr;
        }
        else
        {
            for(auto f: availableEngines)
            {
	            if(auto e = f(eid))
	            {
                    engine = e;
                    break;
	            }
            }

            if (engine == nullptr && eid != getDefaultEngineId())
            {
                eid = getDefaultEngineId();

                for (auto f : availableEngines)
                {
                    if (auto e = f(eid))
                    {
                        engine = e;
                        break;
                    }
                }
            }
            
            if(engine != nullptr)
            {
                if (numChannels != 0 && sourceSampleRate != 0.0)
                {
                    engine->configure(numChannels, sourceSampleRate);
                }

                engine->reset();
            }
        }
    }
}

template <typename T> static timestretch_engine_base* createEngine(const juce::Identifier& id)
{
    timestretch_engine_base* t = nullptr;

    if (T::getStaticId() == id)
        t = new T();

    return t;
}

void time_stretcher::registerEngines(time_stretcher& t)
{
    t.availableEngines.add(createEngine<signal_smith_stretcher>);

#if HISE_ENABLE_RUBBERBAND
    t.availableEngines.add(createEngine<rubberband_v2>);
    t.availableEngines.add(createEngine<rubberband_v3>);
#endif
}

juce::Identifier time_stretcher::getCurrentEngine() const
{
    return engine != nullptr ? engine->getEngineId() : Identifier();
}

Identifier time_stretcher::getDefaultEngineId()
{
    return Identifier(HISE_DEFAULT_TIMESTRETCH_ENGINE);
}

void time_stretcher::reset()
{
    ScopedLock sl(stretchLock);
    engine->reset();
}

void time_stretcher::configure(int numChannels_, double sourceSampleRate_)
{
    if(numChannels != numChannels_ ||
       sourceSampleRate != sourceSampleRate_)
    {
        ScopedLock sl(stretchLock);
        
        numChannels = numChannels_;
        sourceSampleRate = sourceSampleRate_;
     
        if(isEnabled() && numChannels > 0 && sourceSampleRate > 0)
        {
            engine->configure(numChannels, sourceSampleRate);
            engine->reset();
        }
    }
}

struct Helpers
{
    static bool isSilent(float** data, int numSamples)
    {
        if (numSamples == 0)
            return true;

        float* l = data[0];
        float* r = data[1];

        using SSEFloat = dsp::SIMDRegister<float>;

        auto alignedL = SSEFloat::getNextSIMDAlignedPtr(l);
        auto alignedR = SSEFloat::getNextSIMDAlignedPtr(r);

        jassert(alignedL == l);

        auto numUnaligned = alignedL - l;
        auto numAligned = numSamples - numUnaligned;

        static const auto gain90dB = Decibels::decibelsToGain(-60.0f);
        
        constexpr int sseSize = SSEFloat::SIMDRegisterSize / sizeof(float);

        while (numAligned >= sseSize)
        {
            auto l_ = SSEFloat::fromRawArray(alignedL);
            auto r_ = SSEFloat::fromRawArray(alignedR);

            auto sqL = SSEFloat::abs(l_);
            auto sqR = SSEFloat::abs(r_);

            auto max = SSEFloat::max(sqL, sqR).sum();

            if (max > gain90dB)
                return false;

            alignedL += sseSize;
            alignedR += sseSize;
            numAligned -= sseSize;
        }

        return true;
    }
};

double time_stretcher::skipLatency(float** inputs, double ratio)
{
    TRACE_EVENT("dsp", "skipLatency");

    ScopedLock sl(stretchLock);
    
    engine->reset();
    
    auto numBeforeOutput = roundToInt(getLatency(ratio));
    
    float* thisInputs[2];
    float* outputs[2];
    
    thisInputs[0] = inputs[0];
    thisInputs[1] = inputs[1];
    
    double currentPos = 0.0;

    engine->setEnableOutput(false);
    
    while(numBeforeOutput > 0)
    {
	    const int numInputs = jmin(numBeforeOutput, 512);
        const int numOutputs = static_cast<double>(numInputs) / ratio;
        
        outputs[0] = static_cast<float*>(alloca(numOutputs * sizeof(float)));
        outputs[1] = static_cast<float*>(alloca(numOutputs * sizeof(float)));
        
        engine->process(thisInputs, numInputs, outputs, numOutputs);
        
        currentPos += numInputs;

        if (currentPos >= 1536)
            engine->setEnableOutput(true);

        thisInputs[0] = inputs[0] + static_cast<int>(currentPos);
        thisInputs[1] = inputs[1] + static_cast<int>(currentPos);

        numBeforeOutput -= numInputs;
    }
    
    return currentPos;
}

double time_stretcher::getLatency(double ratio) const
{
    jassert(engine != nullptr);

    return engine->getLatency(ratio);
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
            jassert(numOutput <= (resampledBuffer.getNumSamples()));

            outputs[0] = resampledBuffer.getWritePointer(0);
            outputs[1] = resampledBuffer.getWritePointer(1);
        }

        engine->process(input, numInput, outputs, numOutput);
        
        

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

                    auto v = Interpolator::interpolateLinear(lo, hi, alpha);

                    ptr[i] = v;
                    uptime += playbackRatio;
                }
            }
        }
    }
}

void time_stretcher::setTransposeSemitones(double semiTones, double tonality)
{
    engine->setTransposeSemitones(semiTones, tonality);
}

void time_stretcher::setFFTSize(int blockSamples, int intervalSamples)
{
	engine->setFFTSize(blockSamples, intervalSamples);
}


void time_stretcher::setTransposeFactor(double pitchFactor, double tonality)
{
    engine->setTransposeFactor(pitchFactor, tonality);
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
            s[1] = resampleBuffer_ + size/2;


            resampledBuffer = AudioSampleBuffer(s, 2, size/2);
        }
        else
        {
            resampledBuffer = {};
        }
    }
}
}
