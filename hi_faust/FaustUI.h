#ifndef __FAUST_UI_H
#define __FAUST_UI_H
#include <memory>
#include <string>
#include <regex>
#include <iostream>
#include <optional>

#ifndef HISE_NUM_MAX_FAUST_MOD_SOURCES
#define HISE_NUM_MAX_FAUST_MOD_SOURCES 4
#endif

#if HISE_NUM_MAX_FAUST_MOD_SOURCES > 16
#error "There is a hard limit of 16 modulation source outputs for Faust nodes"
#endif

namespace scriptnode {
namespace faust {

/** This struct will take a snapshot of the current values and reset the zone pointers
    to this value when it goes out of scope.
 
    It is used by the `faust->init()` call to prevent resetting the internal values everytime
    it is called which happens when changing the processing context.
*/
struct ScopedZoneSetter
{
    struct Item
    {
        Item(float* zone_): zone(zone_), value(*zone) {};
        
        void reset()
        {
            jassert(zone != nullptr);
            *zone = value;
        }
        
        float* zone;
        float value;
    };
    
    Array<Item> items;
    
    template <typename UIClass> ScopedZoneSetter(UIClass& ui)
    {
        for (auto p : ui.parameters)
        {
            for(const auto& z: p->zone)
                items.add({ z });
        }
    }
    
    ~ScopedZoneSetter()
    {
        for(auto& i: items)
            i.reset();
    }
};

/** This UI subclass is templated to handle monophonic / polyphonic faust nodes.

	Depending on the voice amount, the internal Parameter object will either hold a 
	single pointer to the faust zone (float*) or a PolyData object that contains the 
	zone pointers for every faust object of each voice.
*/
template <int NV, class ModParameterClass> struct faust_ui : public ::faust::UI
{
	static constexpr int NumVoices = NV;

	enum ControlType {
		NONE = 0,
		BUTTON,
		CHECK_BUTTON,
		VERTICAL_SLIDER,
		HORIZONTAL_SLIDER,
		NUM_ENTRY,
		HORIZONTAL_BARGRAPH,
		VERTICAL_BARGRAPH,
		// SOUND_FILE, // Handle Soundfile separately
		MIDI,
		OTHER=0xffff,
	};

	/** Faust is using a convention that maps MIDI input to
		"freq", "gate" and "gain" parameters and this will implement
		the logic to do this when the node is placed within a context
		that processes MIDI events.
	*/
	enum class HardcodedMidiZones {
		Frequency,
		Gate,
		Gain,
		numHardcodedMidiZones
	};

	struct Parameter 
	{
		ControlType type;
		String label;
		PolyData<float*, NumVoices> zone;
		float init;
		float min;
		float max;
		float step;
		std::optional<std::string> styleType;
		std::optional<std::unique_ptr<std::map<float,std::string>>> styleMap;

        ModValue modValue;
        
		Parameter(ControlType type,
				  String label,
				  float* zone_,
				  float init,
				  float min,
				  float max,
				  float step) :
			type(type),
			label(label),
			init(init),
			min(min),
			max(max),
			step(step)
		{
			for (auto& z : zone)
				z = nullptr;

			*zone.begin() = zone_;
		}

        static void setParameter(void* obj, double newValue)
        {
			auto typed = static_cast<Parameter*>(obj);

            for(auto z: typed->zone)
				*z = (float)newValue;
        }
        
        bool handleModulation(double& value)
        {
            auto newValue = (double)*(zone.get());
            
            if(modValue.setModValueIfChanged(newValue))
            {
                value = newValue;
                return true;
            }
            
            return false;
        }
        
        /** Skip parameter creation for those, as they are used as draggable modulation source. */
        bool isModParameter() const
        {
            return type == ControlType::HORIZONTAL_BARGRAPH ||
                   type == ControlType::VERTICAL_BARGRAPH;
        }
        
		parameter::data toParameterData() const {
			switch (type) {
			case faust_ui::ControlType::VERTICAL_SLIDER:
			case faust_ui::ControlType::HORIZONTAL_SLIDER:
			case faust_ui::ControlType::NUM_ENTRY:
			{
				parameter::data pd(label, {(double)(min), (double)(max), (double)(step)});
				pd.setDefaultValue((double)(init));
                pd.callback.referTo((void*)this, setParameter);
				return pd;
			}
			break;
            case faust_ui::ControlType::BUTTON:
			case faust_ui::ControlType::CHECK_BUTTON:
			{
				parameter::data pd(label, {0.0, 1.0});
				pd.setDefaultValue((double)(init));
				pd.setParameterValueNames({"off", "on"});
                pd.callback.referTo((void*)this, setParameter);
				return pd;
			}
            default:
			break;
			}
			parameter::data pd("invalid", {0., 1.0});
			return pd;
		}

	};

	struct MidiZones
	{
		MidiZones()
		{
			reset();
		}

		void reset()
		{
			memset(zones, 0, sizeof(float*) * (size_t)HardcodedMidiZones::numHardcodedMidiZones);
		}

		float* zones[(int)HardcodedMidiZones::numHardcodedMidiZones];
        bool sustain = false;
        
        
	};

	faust_ui() 
	{
		reset();
	}

    ModParameterClass modParameters;
    
    /** This holds the zones for all modulation outputs. */
    std::vector<std::shared_ptr<Parameter>> modoutputs;
    
	std::vector<std::shared_ptr<Parameter>> parameters;

	PolyData<MidiZones, NumVoices> midiZones;
    
	bool anyMidiZonesActive = false;

	/** This checks the parameter name and stores the zone pointer to be updated when a MIDI event is received. */
	void addHardcodedMidiZone(const String& parameterName, float* zonePtr)
	{
		auto id = parameterName.toLowerCase();

		static const StringArray zoneNames = { "freq", "gate", "gain"};

		auto idx = zoneNames.indexOf(id);

		if (idx != -1)
		{
			for (auto& mz : midiZones)
			{
				if (mz.zones[idx] == nullptr)
				{
					mz.zones[idx] = zonePtr;
					anyMidiZonesActive = true;
					break;
				}
			}
		}
	}

	void prepare(PrepareSpecs ps)
	{
		for (auto p : parameters)
			p->zone.prepare(ps);

		midiZones.prepare(ps);
	}

    template <int P> void callOutputValue()
    {
        if constexpr (P < HISE_NUM_MAX_FAUST_MOD_SOURCES)
        {
            // In a static compilation context this will evaluate to a
            // compile-time expression to remove the overhead of unnecessary
            // branching
            if constexpr (ModParameterClass::isStaticList())
            {
                if constexpr ((P < ModParameterClass::getNumParameters()))
                {
					double v;

                    if(modoutputs[P]->handleModulation(v))
                        this->modParameters.template call<P>(v);
                }
            }
            else
            {
                // In a dynamic (scriptnode JIT) context this must be resolved by
                // the parameter::dynamic_list
                jassert(modoutputs.size() == this->modParameters.getNumParameters());
                
                double v;
                
                if(isPositiveAndBelow(P, modoutputs.size()))
                {
                    if(modoutputs[P]->handleModulation(v))
                        this->modParameters.template call<P>(v);
                }
            }
        }
    }
    
    void handleModulationOutputs()
    {
        // We need to type this out but it will be checked
        // against HISE_NUM_MAX_FAUST_MOD_SOURCES on compile time
        callOutputValue<0>();
        callOutputValue<1>();
        callOutputValue<2>();
        callOutputValue<3>();
        callOutputValue<4>();
        callOutputValue<5>();
        callOutputValue<6>();
        callOutputValue<7>();
        callOutputValue<8>();
        callOutputValue<9>();
        callOutputValue<10>();
        callOutputValue<11>();
        callOutputValue<12>();
        callOutputValue<13>();
        callOutputValue<14>();
        callOutputValue<15>();
    }
    
	void handleHiseEvent(HiseEvent& e)
	{
        auto isSustain = e.isControllerOfType(64);
        
        // We don't want to bother if there are no zones defined or it's not a note on/off message
		if (!anyMidiZonesActive || (!e.isNoteOnOrOff() && !isSustain))
			return;

		if (e.isNoteOn())
		{
			auto& mz = midiZones.get();

			if (auto gatePtr = mz.zones[(int)HardcodedMidiZones::Gate])
				*gatePtr = 1.0f;
			if (auto freqPtr = mz.zones[(int)HardcodedMidiZones::Frequency])
				*freqPtr = e.getFrequency();
			if (auto gainPtr = mz.zones[(int)HardcodedMidiZones::Gain])
				*gainPtr = e.getFloatVelocity();
		}
        else if(e.isNoteOff())
		{
			auto& mz = midiZones.get();

            if(!mz.sustain)
            {
                if (auto gatePtr = mz.zones[(int)HardcodedMidiZones::Gate])
                    *gatePtr = 0.0f;
            }
		}
        else if (isSustain)
        {
            auto thisSustain = e.getControllerValue() > 64;
            
            for(auto& mz: midiZones)
            {
                if(mz.sustain != thisSustain)
                {
                    mz.sustain = thisSustain;
                    
                    if(!thisSustain)
                    {
                        if (auto gatePtr = mz.zones[(int)HardcodedMidiZones::Gate])
                            *gatePtr = 0.0f;
                    }
                }
            }
        }
	}

	void reset()
	{
        modoutputs.clear();
		parameters.clear();

		for (auto& mz : midiZones)
			mz.reset();

		anyMidiZonesActive = false;
	}

	std::optional<std::shared_ptr<Parameter>> getParameterByLabel(String label, bool getInputParameter=true)
	{
        auto& listToUse = getInputParameter ? parameters : modoutputs;
        
		for (auto p : listToUse)
		{
			if (p->label == label)
				return p;
		}
		return {};
	}

	std::optional<std::shared_ptr<Parameter>> getParameterByZone(float* zone)
	{
		for (auto p : parameters)
		{
			if (p->zone.getFirst() == zone)
				return p;
		}
		return {};
	}

	bool attachZoneVoiceToExistingParameter(const char* label, float* zone, bool getInputParameter=true)
	{
		auto existing = getParameterByLabel(label, getInputParameter);

		if (existing.has_value())
		{
			for (auto& z : (*existing)->zone)
			{
				if (z == nullptr)
				{
					z = zone;
					return true;
				}
			}
		}

		return false;
	}

	std::pair<std::string, std::map<float,std::string>> parseMetaData(std::string style)
	{
		std::map<float,std::string> values;
		std::string type = "";
		std::string s(style);
		std::regex type_re("([^{]+)(\\{(.*)\\})?");
		std::regex kv_re("'([^']*)' *: *([^ ;]*)");
		std::smatch type_match;

		if (std::regex_match(s, type_match, type_re)) {
			if (type_match.size() > 1) {
				type = type_match[1];
			}
			if (type_match.size() == 4) {
				std::string data = type_match[3];
				std::smatch values_match;
				while (std::regex_search(data, values_match, kv_re)) {
					if (values_match.size() == 3) {
						float k = std::stof(values_match[2]);
						std::string v = values_match[1];
						values.insert({k, v});
					}
					data = values_match.suffix();
				}
			}
		}
		return std::make_pair(type, values);
	}

	void applyStyle(std::shared_ptr<Parameter> p, std::string style)
	{
		auto style_ = parseMetaData(style);
		auto style_type = style_.first;
		auto style_map = style_.second;

		p->styleType = std::make_optional(style_type);
		p->styleMap = std::make_optional(std::make_unique<std::map<float, std::string>>(style_map));
		// if (style_type == "menu") {
		//		std::vector<std::string> labels;
		//		std::vector<float> values;


		//		for (auto elm: style_map) {
		//			labels.push_back(elm.second());
		//			values.push_back(elm.first());
		//		}

		//		// p.setParameterValueNames(labels);
		// }
	}

	// -- metadata declarations

	virtual void declare(float* zone, const char* key, const char* val) override
	{
		auto p = getParameterByZone(zone);
		if (p.has_value())
		{
			if (strcmp("style", val) == 0) {
				applyStyle(*p, std::string(val));
			}
		}
	}

	// Faust UI implementation

	virtual void openTabBox(const char* label) override
	{

	}
	virtual void openHorizontalBox(const char* label) override
	{

	}
	virtual void openVerticalBox(const char* label) override
	{

	}
	virtual void closeBox() override { }

	// -- active widgets



	virtual void addButton(const char* label, float* zone) override
	{
		addHardcodedMidiZone(label, zone);

		if (attachZoneVoiceToExistingParameter(label, zone))
			return;

		parameters.push_back(std::make_shared<Parameter>(ControlType::BUTTON,
														 String(label),
														 zone,
														 0.f,
														 0.f,
														 1.f,
														 1.f));
	}
	virtual void addCheckButton(const char* label, float* zone) override
	{
		addHardcodedMidiZone(label, zone);

		if (attachZoneVoiceToExistingParameter(label, zone))
			return;

		parameters.push_back(std::make_shared<Parameter>(ControlType::CHECK_BUTTON,
														 String(label),
														 zone,
														 0.f,
														 0.f,
														 1.f,
														 1.f));

	}
	virtual void addVerticalSlider(const char* label, float* zone, float init, float min, float max, float step) override
	{
		addHardcodedMidiZone(label, zone);

		if (attachZoneVoiceToExistingParameter(label, zone))
			return;

		parameters.push_back(std::make_shared<Parameter>(ControlType::VERTICAL_SLIDER,
														 String(label),
														 zone,
														 init,
														 min,
														 max,
														 step));
	}
	virtual void addHorizontalSlider(const char* label, float* zone, float init, float min, float max, float step) override
	{
		addHardcodedMidiZone(label, zone);

		if (attachZoneVoiceToExistingParameter(label, zone))
			return;

		parameters.push_back(std::make_shared<Parameter>(ControlType::HORIZONTAL_SLIDER,
														 String(label),
														 zone,
														 init,
														 min,
														 max,
														 step));
	}
	virtual void addNumEntry(const char* label, float* zone, float init, float min, float max, float step) override
	{
		addHardcodedMidiZone(label, zone);

		if (attachZoneVoiceToExistingParameter(label, zone))
			return;

		parameters.push_back(std::make_shared<Parameter>(ControlType::NUM_ENTRY,
														 String(label),
														 zone,
														 init,
														 min,
														 max,
														 step));
	}

	// -- passive widgets

	virtual void addHorizontalBargraph(const char* label, float* zone, float min, float max) override
	{
		if (attachZoneVoiceToExistingParameter(label, zone, false))
            return;
        
        modoutputs.push_back(std::make_shared<Parameter>(ControlType::HORIZONTAL_BARGRAPH,
                                                         String(label),
                                                         zone,
                                                         0.0,
                                                         min,
                                                         max,
                                                         0.0));
	}

	virtual void addVerticalBargraph(const char* label, float* zone, float min, float max) override
	{
		if (attachZoneVoiceToExistingParameter(label, zone, false))
            return;
        
        modoutputs.push_back(std::make_shared<Parameter>(ControlType::VERTICAL_BARGRAPH,
                                                         String(label),
                                                         zone,
                                                         0.0,
                                                         min,
                                                         max,
                                                         0.0));
	}

	// -- soundfiles -- TODO

	virtual void addSoundfile(const char* label, const char* filename, ::faust::Soundfile** sf_zone) { }

};

} // namespace faust
} // namespace scriptnode

#endif // __FAUST_UI_H
