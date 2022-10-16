#ifndef __FAUST_UI_H
#define __FAUST_UI_H
#include <memory>
#include <string>
#include <regex>
#include <iostream>
#include <optional>

namespace scriptnode {
namespace faust {

struct faust_ui : public ::faust::UI {
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
        
        ScopedZoneSetter(faust_ui& ui)
        {
            for(auto p: ui.parameters)
                items.add({p->zone});
        }
        
        ~ScopedZoneSetter()
        {
            for(auto& i: items)
                i.reset();
        }
    };
    
	struct Parameter {
		ControlType type;
		String label;
		float* zone;
		float init;
		float min;
		float max;
		float step;
		std::optional<std::string> styleType;
		std::optional<std::unique_ptr<std::map<float,std::string>>> styleMap;

		Parameter(ControlType type,
				  String label,
				  float* zone,
				  float init,
				  float min,
				  float max,
				  float step) :
			type(type),
			label(label),
			zone(zone),
			init(init),
			min(min),
			max(max),
			step(step) {}

        static void setParameter(void* obj, double newValue)
        {
            *static_cast<Parameter*>(obj)->zone = (float)newValue;
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
			case faust_ui::ControlType::CHECK_BUTTON:
			{
				parameter::data pd(label, {0.0, 1.0});
				pd.setDefaultValue((double)(init));
				pd.setParameterValueNames({"off", "on"});
                pd.callback.referTo((void*)this, setParameter);
				return pd;
			}
			break;
			}
			parameter::data pd("invalid", {0., 0.});
			return pd;
		}

	};

	faust_ui() 
	{
		reset();
	}

	std::vector<std::shared_ptr<Parameter>> parameters;

	float* midiZones[(int)HardcodedMidiZones::numHardcodedMidiZones];
	bool anyMidiZonesActive = false;
    
    float* modZone = nullptr;
    ModValue modValue;

	/** This checks the parameter name and stores the zone pointer to be updated when a MIDI event is received. */
	void addHardcodedMidiZone(const String& parameterName, float* zonePtr)
	{
		auto id = parameterName.toLowerCase();

		static const StringArray zoneNames = { "freq", "gate", "gain"};

		auto idx = zoneNames.indexOf(id);

		if (idx != -1)
		{
			midiZones[idx] = zonePtr;
			anyMidiZonesActive = true;
		}
	}

    bool handleModulation(double& v)
    {
        if(modZone != nullptr)
        {
            if(modValue.setModValueIfChanged((double)*modZone))
            {
                v = modValue.getModValue();
                return true;
            }
        }
        
        return false;
    }
    
	void handleHiseEvent(HiseEvent& e)
	{
		// We don't want to bother if there are no zones defined or it's not a note on/off message
		if (!anyMidiZonesActive || !e.isNoteOnOrOff())
			return;

		if (e.isNoteOn())
		{
			if (auto gatePtr = midiZones[(int)HardcodedMidiZones::Gate])
				*gatePtr = 1.0f;
			if (auto freqPtr = midiZones[(int)HardcodedMidiZones::Frequency])
				*freqPtr = e.getFrequency();
			if (auto gainPtr = midiZones[(int)HardcodedMidiZones::Gain])
				*gainPtr = e.getFloatVelocity();
		}
		else
		{
			if (auto gatePtr = midiZones[(int)HardcodedMidiZones::Gate])
				*gatePtr = 0.0f;
		}
	}

	void reset()
	{
		parameters.clear();
		memset(midiZones, 0, sizeof(float*)*(int)HardcodedMidiZones::numHardcodedMidiZones);
		anyMidiZonesActive = false;
        modZone = nullptr;
	}

	std::optional<std::shared_ptr<Parameter>> getParameterByLabel(String label)
	{
		for (auto p : parameters)
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
			if (p->zone == zone)
				return p;
		}
		return {};
	}

	std::vector<String> getParameterLabels()
	{
		std::vector<String> res;
		res.reserve(parameters.size());

		for (auto p : parameters)
		{
			res.push_back(p->label);
		}

		return res;
	}

	float* getZone(String label) {
		for (auto p : parameters)
		{
			if (p->label == label)
				return p->zone;
		}
		return nullptr;
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
		addHardcodedMidiZone(label, zone);

        // only a single mod value output is supported
        jassert(modZone != nullptr);
        
        modZone = zone;
        
		parameters.push_back(std::make_shared<Parameter>(ControlType::HORIZONTAL_BARGRAPH,
														 String(label),
														 zone,
														 0.f,
														 min,
														 max,
														 1.f));
	}
	virtual void addVerticalBargraph(const char* label, float* zone, float min, float max) override
	{
		addHardcodedMidiZone(label, zone);

        // only a single mod value output is supported
        jassert(modZone != nullptr);
        
        modZone = zone;
        
		parameters.push_back(std::make_shared<Parameter>(ControlType::VERTICAL_BARGRAPH,
														 String(label),
														 zone,
														 0.f,
														 min,
														 max,
														 1.f));
	}

	// -- soundfiles -- TODO

	virtual void addSoundfile(const char* label, const char* filename, ::faust::Soundfile** sf_zone) { }

};

} // namespace faust
} // namespace scriptnode

#endif // __FAUST_UI_H
