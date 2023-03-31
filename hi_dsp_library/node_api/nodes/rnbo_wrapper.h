#pragma once

#define RNBO_NO_PATCHERFACTORY 1
#define RNBO_USE_FLOAT32 1

#include <hi_dsp_library/hi_dsp_library.h>
#include <RNBO.h>







namespace scriptnode
{
using namespace hise;
using namespace juce;

namespace wrap
{


class rnbo_data_handler: public RNBO::EventHandler
{
    struct Item
    {
        Item(ExternalData::DataType& dt_, const char* id_):
          idString(id_),
          index(RNBO::TAG(id_)),
          dt(dt_)
        {};
        
        
        String idString;
        ExternalData::DataType dt;
        ExternalData d;
        RNBO::ExternalDataIndex index;
        RNBO::UniquePtr<RNBO::ParameterEventInterface> eventInterface;
        
        HeapBlock<float> interleavedData;
    };
    
public:
    
    rnbo_data_handler() = default;
    
    void eventsAvailable() override { drainEvents(); }
    
    void handleMessageEvent(const RNBO::MessageEvent& event) override
    {
        if (event.getType() == RNBO::MessageEvent::Number)
        {
            for(auto& i: items)
            {
                if(i->index == event.getTag())
                {
                    auto displayValue = (double)event.getNumValue();
                    
                    if(i->dt != ExternalData::DataType::Table)
                        displayValue *= (double)i->d.numSamples;
                    
                    i->d.setDisplayedValue(displayValue);
                    break;
                }
            }
        }
    }
    
    void registerDataSlot(ExternalData::DataType dataType, const char* slotId)
    {
        items.add(new Item(dataType, slotId));
    }
    
    void setExternalData(RNBO::CoreObject& o, const ExternalData& d, int index)
    {
        int indexes[(int)ExternalData::DataType::numDataTypes];
        
        memset(indexes, 0, sizeof(indexes));
        
        for(auto i: items)
        {
            if(i->dt == d.dataType)
            {
                if(indexes[(int)i->dt]++ == index)
                {
                    i->d = d;
                    
                    RNBO::ExternalDataId id = i->idString.getCharPointer().getAddress();
                    auto ptr = (char*)d.data;
                    auto numBytes = d.numSamples * sizeof(float) / sizeof(char);
                    
                    if(i->dt == ExternalData::DataType::AudioFile)
                    {
                        i->interleavedData.allocate(d.numSamples * d.numChannels, false);
                        
                        int interIndex = 0;
                        
                        auto rptr = (float**)d.data;
                        
                        for(int s = 0; s < d.numSamples; s++)
                        {
                            for(int c = 0; c < d.numChannels; c++)
                                i->interleavedData[interIndex++] = rptr[c][s];
                        }
                        
                        ptr = reinterpret_cast<char*>(i->interleavedData.get());
                        numBytes = d.numSamples * d.numChannels * sizeof(float);
                    }
                    
                    RNBO::Float32AudioBuffer bufferType(d.numChannels, d.sampleRate);
                    
                    o.setExternalData(id, ptr, numBytes, bufferType);
                    
                    if(!i->eventInterface)
                        i->eventInterface = o.createParameterInterface(RNBO::ParameterEventInterface::Trigger, this);
                    
                    break;
                }
            }
        }
    }
    
private:
    
    OwnedArray<Item> items;
};

template <class RNBOType, int NV> struct rnbo_wrapper:
    public scriptnode::data::base,
    public hise::TempoListener
   
{
    // Metadata Definitions ------------------------------------------------------
    
    rnbo_data_handler dataHandler;
    
    rnbo_wrapper()
    {
        for(RNBO::CoreObject& o: obj)
        {
            o.setPatcher(RNBO::UniquePtr<RNBO::PatcherInterface>(new RNBOType()));
        }
        
        const RNBO::CoreObject& first = obj.getFirst();
        
        for(int i = 0; i < first.getNumExternalDataRefs(); i++)
        {
            auto ei = first.getExternalDataInfo(i);
        }
    }
    
    hise::DllBoundaryTempoSyncer* syncer = nullptr;
    bool useSyncer = false;
    
    void setUseTempo(bool shouldUse)
    {
        useSyncer = true;
    }
    
    virtual ~rnbo_wrapper()
    {
        if(syncer != nullptr)
            syncer->deregisterItem(this);
    }
    
    auto resolveTag(RNBO::MessageTag tag) { return obj.getFirst().resolveTag(tag); };
    
    static constexpr bool isModNode() { return false; };
    
    static constexpr bool hasTail() { return false; };

	static constexpr bool isSuspendedOnSilence() { return false; }

    static constexpr int getFixChannelAmount() { return 2; };
    
    // Define the amount and types of external data slots you want to use
    static constexpr int NumTables = 0;
    static constexpr int NumSliderPacks = 0;
    static constexpr int NumAudioFiles = 0;
    static constexpr int NumFilters = 0;
    static constexpr int NumDisplayBuffers = 0;
    
    bool isProcessingHiseEvent() const
    {
        return obj.getFirst().getNumMidiInputPorts() > 0;
    }
    
    SN_EMPTY_INITIALISE;
    
    // Scriptnode Callbacks ------------------------------------------------------
    
    virtual void prepare(PrepareSpecs specs)
    {
        obj.prepare(specs);
            
        for(auto& o: obj)
        {
            o.prepareToProcess(specs.sampleRate, specs.blockSize, true);
            o.scheduleEvent(RNBO::MessageEvent(RNBO::TAG("blocksize"), specs.blockSize));
        }
        
        if(useSyncer && syncer == nullptr)
        {
            syncer = specs.voiceIndex->getTempoSyncer();
            
            if(syncer != nullptr)
                syncer->registerItem(this);
        }
    }
    
    void reset()
    {
        for(auto& o: obj)
            o.scheduleEvent(RNBO::MessageEvent(RNBO::TAG("reset"), RNBO::RNBOTimeNow));
    }
    
    void tempoChanged(double newBpm) override
    {
        for(auto& o: obj)
            o.scheduleEvent(RNBO::TempoEvent(RNBO::RNBOTimeNow, newBpm));
    }
    
    void onTransportChange(bool isPlaying, double ppqPosition) override
    {
        auto state = isPlaying ? RNBO::TransportState::RUNNING :
                                 RNBO::TransportState::STOPPED;
        
        for(auto& o: obj)
        {
            o.scheduleEvent(RNBO::TransportEvent(RNBO::RNBOTimeNow, state));
            o.scheduleEvent(RNBO::BeatTimeEvent(RNBO::RNBOTimeNow, ppqPosition));
        }
    }
    
    void onResync(double ppqPosition) override
    {
        for(auto& o: obj)
        {
            o.scheduleEvent(RNBO::BeatTimeEvent(RNBO::RNBOTimeNow, ppqPosition));
        }
    }
    
    void handleHiseEvent(HiseEvent& e)
    {
        auto m = e.toMidiMesage();
        
        obj.get().scheduleEvent(RNBO::MidiEvent(RNBO::RNBOTimeNow, 0, m.getRawData(), m.getRawDataSize()));
    }
    
    template <typename T> void process(T& data)
    {
        RNBO::CoreObject& o = obj.get();
        
        auto numChannels = data.getNumChannels();
        auto numSamples = data.getNumSamples();
        auto ptrs = data.getRawDataPointers();
        
        o.process(ptrs, numChannels, ptrs, numChannels, numSamples);
    }
    
    template <typename T> void processFrame(T& data)
    {
        auto ptrs = (float**)alloca(data.size() * sizeof(float*));
        
        for(int i = 0; i < data.size(); i++)
            ptrs[i] = data.begin() + i;
        
        auto& o = obj.get();
        
        o.process(ptrs, data.size(), ptrs, data.size(), 1);
    }
    
    void setExternalData(const ExternalData& data, int index)
    {
        for(auto& o: obj)
            dataHandler.setExternalData(o, data, index);
    }
    // Parameter Functions -------------------------------------------------------
    
    template <int P> void setParameter(double v)
    {
        for(auto& o: obj)
            o.setParameterValue(P, v);
    }
    SN_FORWARD_PARAMETER_TO_MEMBER(rnbo_wrapper);
    
    void createParameters(ParameterDataList& data)
    {
        const RNBO::CoreObject& o = obj.getFirst();
        
        for(int i = 0; i < o.getNumParameters(); i++)
        {
            RNBO::ParameterInfo info;
            o.getParameterInfo(i, &info);
            
            if(info.visible)
            {
                auto name = o.getParameterName(i);
                
                parameter::data pi(name, {(double)info.min, (double)info.max});
                
                if(info.enumValues && info.steps)
                {
                    StringArray sa;
                    
                    for(int j = 0; j < info.steps; j++)
                    {
                        sa.add(info.enumValues[j]);
                    }
                    
                    pi.setParameterValueNames(sa);
                }
                
                pi.setDefaultValue(info.initialValue);
                
#define ADD_CALLBACK(idx) if(i == idx)  pi.setParameterCallbackWithIndex<rnbo_wrapper<RNBOType, NV>, idx>(this);
                
                ADD_CALLBACK(0); ADD_CALLBACK(1); ADD_CALLBACK(2); ADD_CALLBACK(3);
                ADD_CALLBACK(4); ADD_CALLBACK(5); ADD_CALLBACK(6); ADD_CALLBACK(7);
                ADD_CALLBACK(8); ADD_CALLBACK(9); ADD_CALLBACK(10); ADD_CALLBACK(11);
                ADD_CALLBACK(12); ADD_CALLBACK(13); ADD_CALLBACK(14); ADD_CALLBACK(15);
                ADD_CALLBACK(16); ADD_CALLBACK(17); ADD_CALLBACK(18); ADD_CALLBACK(19);

#undef ADD_CALLBACK
                
                data.add(std::move(pi));
            }
        }
    }
    
    PolyData<RNBO::CoreObject, NV> obj;
};

template <class RNBOType, int NV> struct rnbo_wrapper_with_mod:
    public rnbo_wrapper<RNBOType, NV>,
    public RNBO::EventHandler
{
    rnbo_wrapper_with_mod():
      rnbo_wrapper<RNBOType, NV>()
    {
        for(auto& o: this->obj)
            interfaces.push_back(o.createParameterInterface(RNBO::ParameterEventInterface::Trigger, this));
    }
    
    static constexpr bool isModNode() { return true; };
    
    int handleModulation(double& value)
    {
        return modValue.getChangedValue(value);
    }
    
    void eventsAvailable() override { drainEvents(); }

    void handleMessageEvent(const RNBO::MessageEvent& event) override
    {
        if (event.getType() == RNBO::MessageEvent::Number &&
            event.getTag() == RNBO::TAG("mod"))
        {
            modValue.setModValue((double)event.getNumValue());
        }
    }
    
    template <typename ProcessDataType> void process(ProcessDataType& data)
    {
        RNBO::CoreObject& o = this->obj.get();
        
        auto numChannels = data.getNumChannels();
        auto numSamples = data.getNumSamples();
        auto ptrs = data.getRawDataPointers();
        
        o.process(ptrs, numChannels, ptrs, numChannels, numSamples);
        o.scheduleEvent(RNBO::MessageEvent(RNBO::TAG("postrender"), RNBO::RNBOTimeNow));
    }
    
    template <typename T> void processFrame(T& data)
    {
        auto ptrs = (float**)alloca(data.size() * sizeof(float*));
        
        for(int i = 0; i < data.size(); i++)
            ptrs[i] = data.begin() + i;
        
        auto& o = this->obj.get();
        
        o.process(ptrs, data.size(), ptrs, data.size(), 1);
        o.scheduleEvent(RNBO::MessageEvent(RNBO::TAG("postrender"), RNBO::RNBOTimeNow));
    }
    
    scriptnode::ModValue modValue;
    std::vector<RNBO::UniquePtr<RNBO::ParameterEventInterface>> interfaces;
    size_t modMessageIndex = 0;
};

}
}
