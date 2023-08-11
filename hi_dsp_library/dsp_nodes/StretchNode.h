#pragma once

namespace scriptnode {
namespace core {
using namespace juce;
using namespace hise;



template <int NV> struct stretch_player: public data::base,
                                         public polyphonic_base
{
    struct tempo_syncer: public hise::TempoListener
    {
        tempo_syncer() = default;

        void prepare(PrepareSpecs ps)
        {
            tempoSyncer = ps.voiceIndex->getTempoSyncer();
            tempoSyncer->registerItem(this);

            state.prepare(ps);
        }

        DllBoundaryTempoSyncer* tempoSyncer = nullptr;

        ~tempo_syncer() override
        {
            if (tempoSyncer != nullptr)
                tempoSyncer->deregisterItem(this);
        }

        void updateFromPPQ(double ppq)
        {
            for (auto& s : state)
            {
                auto normed = hmath::fmod(ppq, s.numQuarters) / s.numQuarters;

                normed *= s.numSamples;
                normed += s.numSamples;
                normed = hmath::fmod(normed, s.numSamples);

                s.resyncPosition.setModValueIfChanged(normed);
            }
        }

        void onTransportChange(bool isPlaying_, double ppqPosition) override
        {
            isPlaying = isPlaying_;

            if (isPlaying)
                updateFromPPQ(ppqPosition);
        }



        void onResync(double ppqPosition) override
        {
            updateFromPPQ(ppqPosition);
        }

        bool updatePlayback(bool& shouldPlay)
        {
            if (enabled)
            {
                if (shouldPlay != isPlaying)
                {
                    shouldPlay = isPlaying;

                    if (isPlaying)
                        state.get().resyncPosition.changed = 1;

                    return shouldPlay;
                }

                return false;
            }

            return false;
        }

        bool resync(double& pos) const
        {
            if (enabled)
            {
                return state.get().resyncPosition.getChangedValue(pos);
            }

            return false;
        }

        void setSource(double sourceSamplerate, int numSourceSamples, double numQuarters = 0.0)
        {
            const auto numSeconds = static_cast<double>(numSourceSamples) / sourceSamplerate;

            if (numQuarters == 0.0)
            {
                // Try to guess the duration by picking the nearest numQuarters that matches a bar
                const auto durationPerQuarterForCurrentBpm = 60.0 / bpm;

                const auto exp = std::log2(numSeconds / durationPerQuarterForCurrentBpm);
                numQuarters = std::pow(2.0, hmath::round(exp));
            }

            const auto durationPerQuarter = numSeconds / numQuarters;

            for (auto& s : state)
            {
                s.sourceBpm = 60.0 / durationPerQuarter;
                s.numSamples = numSourceSamples;
                s.numQuarters = numQuarters;
            }
        }

        double getRatio(double fallbackRatio) const
        {
            if (enabled)
            {
                for(auto& s: state)
                {
                    if(s.sourceBpm != 0.0)
                    {
                        auto bpmRatio = bpm / s.sourceBpm;
                        return jmin(bpmRatio, 2.0);
                    }
                }
            }

            return fallbackRatio;
        }

        void tempoChanged(double newTempo) override
        {
            bpm = newTempo;
        }

        struct State
        {
            double sourceBpm = 120.0;
            double numSamples = 0.0;
            double numQuarters = 0.0;
            ModValue resyncPosition;
        };

        void setEnabled(bool shouldBeEnabled)
        {
            enabled = shouldBeEnabled;

            if (enabled)
            {
                for (auto& s : state)
                {
                    s.resyncPosition.changed = 1;
                }
            }
        }

        PolyData<State, NV> state;

        double bpm = 120.0;

        bool enabled = true;

        bool isPlaying = false;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(tempo_syncer)
    };

    constexpr static int NumVoices = NV;
    
    SN_NODE_ID("stretch_player");
    SN_GET_SELF_AS_OBJECT(stretch_player);
    SN_DESCRIPTION("A buffer player with timestretching");
    
    stretch_player():
      polyphonic_base(getStaticId())
    {};
    
    static constexpr bool isModNode() { return false; };
    static constexpr bool isPolyphonic() { return NV > 1; };
    static constexpr bool hasTail() { return true; };
    static constexpr bool isSuspendedOnSilence() { return false; };
    static constexpr int getFixChannelAmount() { return 2; };
    
    static constexpr int NumTables = 0;
    static constexpr int NumSliderPacks = 0;
    static constexpr int NumAudioFiles = 1;
    static constexpr int NumFilters = 0;
    static constexpr int NumDisplayBuffers = 0;
    
    // Scriptnode Callbacks ------------------------------------------------------------------------
    
    void prepare(PrepareSpecs specs)
    {
        lastSpecs = specs;
        refreshQuality();
        refreshResampling();
        
        state.prepare(specs);
        syncer.prepare(specs);
    }
    
    void reset()
    {
        for(auto& s: state)
        {
            s.stretcher.reset();
            s.currentPosition = 0.0;
            s.leftOver = 0.0;
        }
        
    }
    
    SN_EMPTY_PROCESS_FRAME;
    SN_EMPTY_MOD;
    SN_EMPTY_INITIALISE;
    SN_EMPTY_HANDLE_EVENT;
    
    using InterpolatorType = index::hermite<index::unscaled<double, index::clamped<0, false>>>;
    using InterpolatorTypeWrapped = index::hermite<index::unscaled<double, index::wrapped<0, false>>>;
    
    
    
    void processFix(ProcessData<2>& data)
    {
        TRACE_DSP();
        
        DataReadLock sl(ed, true);
        
        auto numSourceSamples = stereoData[0].size();
        
        if(numSourceSamples > 0)
        {
            auto& s = state.get();

            auto add4096 = syncer.updatePlayback(s.gate) && enabled;

            if(!s.gate)
                return;

            syncer.resync(s.currentPosition);

            if (add4096)
            {
                s.stretcher.reset();
                s.currentPosition += 4096.0;
            }
                

            if(enabled)
            {
                float* inputs[2];
                float* outputs[2];
                
                outputs[0] = data[0].begin();
                outputs[1] = data[1].begin();
                
                auto numSamplesToProduce = roundToInt(static_cast<double>(data.getNumSamples()) * playbackRatio);

                auto ratio = syncer.getRatio(s.timeRatio);

                auto numInputs = static_cast<double>(numSamplesToProduce) * ratio + s.leftOver;
                auto numSamplesInLoop = numSourceSamples;
                auto rounded = hmath::round(numInputs);
                
                s.leftOver = rounded - numInputs;
                numInputs = rounded;

                const auto currentLeft = stereoData[0].begin() + static_cast<int>(s.currentPosition);
                const auto currentRight = stereoData[1].begin() + static_cast<int>(s.currentPosition);
                
                if(s.currentPosition + numInputs > numSamplesInLoop)
                {
	                const int numBeforeWrap = stereoData[0].size() - static_cast<int>(s.currentPosition);
	                const int numAfterWrap = static_cast<int>(numInputs) - numBeforeWrap;
                    
                    inputs[0] = loopBuffer.begin();;
                    inputs[1] = loopBuffer.begin() + static_cast<int>(numInputs);;
                    
                    FloatVectorOperations::copy(inputs[0], currentLeft, numBeforeWrap);
                    FloatVectorOperations::copy(inputs[1], currentRight, numBeforeWrap);
                    
                    FloatVectorOperations::copy(inputs[0]+numBeforeWrap, stereoData[0].begin(), numAfterWrap);
                    FloatVectorOperations::copy(inputs[1]+numBeforeWrap, stereoData[1].begin(), numAfterWrap);
                    
                    s.stretcher.process(inputs, int(numInputs), outputs, numSamplesToProduce);
                    s.currentPosition += numInputs - numSamplesInLoop;
                }
                else
                {
                    inputs[0] = currentLeft;
                    inputs[1] = currentRight;
                    
                    s.stretcher.process(inputs, static_cast<int>(numInputs), outputs, numSamplesToProduce);
                    s.currentPosition += numInputs;
                }
            }
            else
            {
                auto fd = data.toFrameData();
                
                while(fd.next())
                {
                    auto& frame = fd.toSpan();
                    
                    if(playbackRatio != 1.0)
                    {
                        InterpolatorTypeWrapped idx(s.currentPosition);
                        
                        frame[0] = stereoData[0][idx];
                        frame[1] = stereoData[1][idx];
                    }
                    else
                    {
                        frame[0] = stereoData[0][(int)s.currentPosition];
                        frame[1] = stereoData[1][(int)s.currentPosition];
                    }
                    
                    s.currentPosition += playbackRatio;
                    
                    if(s.currentPosition >= numSourceSamples)
                        s.currentPosition -= (double)numSourceSamples;
                }
            }
            
            ed.setDisplayedValue((double)s.currentPosition);

            syncer.state.get().resyncPosition.modValue = (double)s.currentPosition;
            syncer.state.get().resyncPosition.changed = 0;

        }
    }
    
    template <typename T> void process(T& data)
    {
        if(data.getNumChannels() != 2)
            return;
        
        using StereoProcessData = ProcessData<2>;
        auto& sd = data.template as<StereoProcessData>();
        
        processFix(sd);
    }
    
    void setExternalData(const ExternalData& data, int index)
    {
        if(auto af = dynamic_cast<MultiChannelAudioBuffer*>(data.obj))
        {
            af->setDisabledXYZProviders({ Identifier("SampleMap"), Identifier("SFZ") });
        }
        
        ed = data;
        
        if(ed.numSamples > 0)
        {
            ed.referBlockTo(stereoData[0], 0);
            ed.referBlockTo(stereoData[1], 1);
            
            refreshQuality();
            refreshResampling();

            syncer.setSource(ed.sampleRate, ed.numSamples, 0.0);
        }
        else
        {
            stereoData[0].referToNothing();
            stereoData[1].referToNothing();
        }
        
        reset();
    }
    // Parameter Functions -------------------------------------------------------------------------

    void seek(double position = 0.0)
    {
        auto& s = state.get();


        if (ed.numSamples > 0 && enabled)
        {
            float* inputs[2];

            inputs[0] = stereoData[0].begin() + roundToInt(position);
            inputs[1] = stereoData[1].begin() + roundToInt(position);

            auto ratio = syncer.getRatio(s.timeRatio);

            s.currentPosition = position + s.stretcher.skipLatency(inputs, ratio);
        }
        else
            s.currentPosition = jmin((double)ed.numSamples, position);
    }
    
    template <int P> void setParameter(double v)
    {
        if (P == 0)
        {
            auto thisGate = v > 0.5;
            
            for(auto& s: state)
            {
                if(thisGate != s.gate)
                {
                    s.gate = thisGate;
                    
                    if(thisGate)
                    {
                        seek(0.0);
                    }
                }
            }
        }
        if (P == 1)
        {
            for(auto& s: state)
                s.timeRatio = jlimit(0.5, 2.0, v);
        }
        if(P == 2)
        {
            auto thisPitch = jlimit(-24.0, 24.0, v);
            
            for(auto& s: state)
            {
                if(s.pitchRatio != thisPitch)
                {
                    s.pitchRatio = thisPitch;
                    s.stretcher.setTransposeSemitones(s.pitchRatio, 0.17);
                }
            }
        }
        if(P == 3)
        {
            enabled = v > 0.5;
        }
        if(P == 4)
        {
            syncer.setEnabled(v > 0.5);
        }
    }
    SN_FORWARD_PARAMETER_TO_MEMBER(stretch_player);
    
    void refreshResampling()
    {
        if(lastSpecs.sampleRate > 0.0 && ed.sampleRate != 0 && lastSpecs.blockSize > 0)
        {
            auto thisRatio = ed.sampleRate / lastSpecs.sampleRate;
            
            if(thisRatio != playbackRatio)
            {
                playbackRatio = thisRatio;
                
                auto numSamplesResampled = (int)hmath::ceil(lastSpecs.blockSize * playbackRatio);
                
                int maxTimeFactor = 4.0;
                
                loopBuffer.setSize(lastSpecs.numChannels * numSamplesResampled * maxTimeFactor);
                
                if(playbackRatio == 1.0)
                    numSamplesResampled = 0;
                
                resampledBuffer.setSize(numSamplesResampled * lastSpecs.numChannels);
                
                for (auto& s : state)
                    s.stretcher.setResampleBuffer(playbackRatio, resampledBuffer.begin(), resampledBuffer.size());
            }
        }
    }
    
    void refreshQuality()
    {
        if(ed.sampleRate > 0.0 && lastSpecs.numChannels > 0 && lastSpecs.blockSize > 0)
        {
            for(auto& s: state)
            {
                s.stretcher.configure(lastSpecs.numChannels, ed.sampleRate);
            }
        }
    }
    
    void createParameters(ParameterDataList& data)
    {
        {
            parameter::data p("Gate", { 0.0, 1.0});
            p.setParameterValueNames({"Off", "On"});
            registerCallback<0>(p);
            p.setDefaultValue(1.0);
            data.add(std::move(p));
        }
        {
            parameter::data p("TimeRatio", { 0.5, 2.0 });
            
            registerCallback<1>(p);
            p.setSkewForCentre(1.0);
            p.setDefaultValue(1.0);
            data.add(std::move(p));
        }
        {
            parameter::data p("Pitch", { -12, 12.0 });
            
            registerCallback<2>(p);
            p.setDefaultValue(0.0);
            data.add(std::move(p));
        }
        {
            parameter::data p("Enable", { 0.0, 1.0});
            p.setParameterValueNames({"Off", "On"});
            registerCallback<3>(p);
            p.setDefaultValue(1.0);
            data.add(std::move(p));
        }
        {
            parameter::data p("ClockSync", { 0.0, 1.0 });
            p.setParameterValueNames({ "Off", "On" });
            registerCallback<4>(p);
            p.setDefaultValue(0.0);
            data.add(std::move(p));
        }
    }
    
    struct State
    {
        State() :
            stretcher(true)
        {};

        double pitchRatio = 0.0;
        double timeRatio = 1.0;
        double currentPosition = 0.0;
        double leftOver = 0.0;
        time_stretcher stretcher;
        bool gate = true;
    };
    
    bool enabled = true;
    ExternalData ed;
    
    span<dyn<float>, 2> stereoData;
    
    heap<float> resampledBuffer;
    
    heap<float> loopBuffer;
    
    double playbackRatio = 0.0;
    
    double windowSize = 100.0;
    PrepareSpecs lastSpecs;
    
    PolyData<State, NV> state;

    tempo_syncer syncer;
};
}
}


