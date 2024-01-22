/*  ===========================================================================
 *
 *   This file is part of HISE.
 *   Copyright 2016 Christoph Hart
 *
 *   HISE is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   HISE is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
 *
 *   Commercial licenses for using HISE in an closed source project are
 *   available on request. Please visit the project's website to get more
 *   information about commercial licensing:
 *
 *   http://www.hise.audio/
 *
 *   HISE is based on the JUCE library,
 *   which also must be licenced for commercial applications:
 *
 *   http://www.juce.com
 *
 *   ===========================================================================
 */

#pragma once

namespace scriptnode 
{
using namespace juce;
using namespace hise;




namespace control
{
	template <typename ParameterClass> struct cable_pack : public data::base,
														   public pimpl::parameter_node_base<ParameterClass>,
														   public pimpl::no_processing
	{
		SN_NODE_ID("cable_pack");
		SN_GET_SELF_AS_OBJECT(cable_pack);
		SN_ADD_SET_VALUE(cable_pack);
		SN_PARAMETER_NODE_CONSTRUCTOR(cable_pack, ParameterClass);
		SN_DESCRIPTION("Uses a slider pack to modify a modulation signal");

		void setExternalData(const ExternalData& d, int index) override
		{
			base::setExternalData(d, index);

			if (d.numSamples > 0)
			{
				d.referBlockTo(b, 0);

				setValue(lastValue);
			}
		}

		block b;

		void setValue(double v)
		{
			lastValue = v;

			DataReadLock l(this);

			if (b.size() > 0)
			{
				IndexType index(v);

				v = b[index];

				if(this->getParameter().isConnected())
					this->getParameter().call(v);

				externalData.setDisplayedValue((double)index.getIndex(b.size()));
			}
		}
		
		using IndexType = index::normalised<double, index::clamped<0>>;

		double lastValue = 0.0;
	};

	template <typename ParameterClass> struct sliderbank : public data::base,
														   public pimpl::parameter_node_base<ParameterClass>,
														   public pimpl::no_processing,
														   public hise::ComplexDataUIUpdaterBase::EventListener
	{
		SN_NODE_ID("sliderbank");
		SN_GET_SELF_AS_OBJECT(sliderbank);
		SN_ADD_SET_VALUE(sliderbank);
		SN_PARAMETER_NODE_CONSTRUCTOR(sliderbank, ParameterClass);
		SN_DESCRIPTION("Scale a value with a slider pack and send it to multiple targets");

		void initialise(NodeBase* n)
		{
			this->p.initialise(n);
		}

		void onComplexDataEvent(hise::ComplexDataUIUpdaterBase::EventType t, var data) override
		{
			if (t == ComplexDataUIUpdaterBase::EventType::ContentChange)
			{
				switch ((int)data)
				{
					case 0: callSlider<0>(lastValue); break;
					case 1: callSlider<1>(lastValue); break;
					case 2: callSlider<2>(lastValue); break;
					case 3: callSlider<3>(lastValue); break;
					case 4: callSlider<4>(lastValue); break;
					case 5: callSlider<5>(lastValue); break;
					case 6: callSlider<6>(lastValue); break;
					case 7: callSlider<7>(lastValue); break; 
				}
			}
		}

		void setExternalData(const ExternalData& d, int index) override
		{
			if (this->externalData.obj != nullptr)
			{
				this->externalData.obj->getUpdater().removeEventListener(this);
			}

			base::setExternalData(d, index);

			if (d.numSamples > 0)
			{
				if (auto sp = dynamic_cast<SliderPackData*>(d.obj))
				{
					d.obj->getUpdater().addEventListener(this);

					if constexpr (ParameterClass::isStaticList())
					{
						if (d.numSamples != ParameterClass::getNumParameters())
						{
							sp->setNumSliders(ParameterClass::getNumParameters());
						}
					}
				}

				d.referBlockTo(b, 0);

				setValue(lastValue);
			}
		}

		block b;

		template <int P> void callSlider(double v)
		{
			if constexpr (ParameterClass::isStaticList())
			{
				if constexpr (P <ParameterClass::getNumParameters())
                {
                    auto& pToCall = this->getParameter().template getParameter<P>();
                    
                    if(pToCall.isConnected())
                        pToCall.call(v * b[P]);
                }
			}
			else
			{
				if (P < b.size() && P < this->getParameter().getNumParameters())
					this->getParameter().template getParameter<P>().call(v * b[P]);
			}
		}

		void setValue(double v)
		{
			lastValue = v;

			DataReadLock l(this);

			if (b.size() > 0)
			{
				callSlider<0>(v);
				callSlider<1>(v);
				callSlider<2>(v);
				callSlider<3>(v);
				callSlider<4>(v);
				callSlider<5>(v);
				callSlider<6>(v);
				callSlider<7>(v);
			}
		}

	private:

		double lastValue = 0.0;
	};


	

	template <typename ParameterClass, typename AnalyserType> 
	struct file_analyser: public scriptnode::data::base,
						  public pimpl::no_processing,
						  public pimpl::templated_mode,
						  public pimpl::no_mod_normalisation,
						  public pimpl::parameter_node_base<ParameterClass>
	{
		SN_NODE_ID("file_analyser");
		SN_GET_SELF_AS_OBJECT(file_analyser);

		file_analyser() :
			templated_mode(getStaticId(), "file_analysers"),
			pimpl::parameter_node_base<ParameterClass>(getStaticId()),
			no_mod_normalisation(getStaticId(), {})
		{};

		SN_DESCRIPTION("Extracts file information (pitch, length, etc) and sends it as modulation signal on file load");

		SN_EMPTY_CREATE_PARAM;

		void initialise(NodeBase* n)
		{
			if constexpr (prototypes::check::initialise<AnalyserType>::value)
				analyser.initialise(n);
		}

		void setExternalData(const snex::ExternalData& d, int index) override
		{
			block b;
			d.referBlockTo(b, 0);

			if (b.size() > 0)
			{
				auto v = analyser.getValue(d);

				if (v != 0.0)
					this->getParameter().call(v);
			}
		}

		AnalyserType analyser;
	};

	template <typename ParameterClass> struct input_toggle : public pimpl::parameter_node_base<ParameterClass>,
														     public pimpl::no_mod_normalisation,
															 public pimpl::no_processing
	{
		SN_NODE_ID("input_toggle");
		SN_GET_SELF_AS_OBJECT(input_toggle);

		input_toggle() :
			pimpl::parameter_node_base<ParameterClass>(getStaticId()),
			pimpl::no_mod_normalisation(getStaticId(), {"Value1", "Value2" })
		{};

		SN_DESCRIPTION("Switch between two input values as modulation signal");

		enum class Parameters
		{
			Input,
			Value1,
			Value2
		};

		DEFINE_PARAMETERS
		{
			DEF_PARAMETER(Input, input_toggle);
			DEF_PARAMETER(Value1, input_toggle);
			DEF_PARAMETER(Value2, input_toggle);
		};
		SN_PARAMETER_MEMBER_FUNCTION;

		

		void setInput(double input)
		{
			useValue1 = input < 0.5;

			if (this->getParameter().isConnected())
				this->getParameter().call(useValue1 ? v1 : v2);
		}

		void setValue1(double input)
		{
			v1 = input;

			if (useValue1)
			{
				if (this->getParameter().isConnected())
					this->getParameter().call(v1);
			}
		}

		void setValue2(double input)
		{
			v2 = input;

			if (!useValue1)
			{
				if (this->getParameter().isConnected())
					this->getParameter().call(v2);
			}
		}

		void createParameters(ParameterDataList& data)
		{
			{
				DEFINE_PARAMETERDATA(input_toggle, Input);
				p.setRange({ 0.0, 1.0, 1.0, 1.0 });
				p.setDefaultValue(0.0);
				data.add(std::move(p));
			}
			{
				DEFINE_PARAMETERDATA(input_toggle, Value1);
				p.setRange({ 0.0, 1.0 });
				p.setDefaultValue(0.0);
				data.add(std::move(p));
			}
			{
				DEFINE_PARAMETERDATA(input_toggle, Value2);
				p.setRange({ 0.0, 1.0 });
				p.setDefaultValue(0.0);
				data.add(std::move(p));
			}
		}

		bool useValue1 = false;
		double v1 = 0.0;
		double v2 = 0.0;

		JUCE_DECLARE_WEAK_REFERENCEABLE(input_toggle);
	};

	struct TempoData
	{
		TempoData() = default;

		double currentTempoMilliseconds = 500.0;
		double lastMs = 0.0;
		bool enabled = false;
		double unsyncedTime = false;
		double multiplier = 1.0;
		TempoSyncer::Tempo currentTempo = TempoSyncer::Tempo::Eighth;
		double bpm = 120.0;

		void refresh()
		{
			if (enabled)
				currentTempoMilliseconds = TempoSyncer::getTempoInMilliSeconds(bpm, currentTempo) * multiplier;
			else
				currentTempoMilliseconds = unsyncedTime;
		}

		bool handleModulation(double& modValue)
		{
			if (lastMs != currentTempoMilliseconds)
			{
				lastMs = currentTempoMilliseconds;
				modValue = currentTempoMilliseconds;
				return true;
			}

			return false;
		}
	};

	class clock_base : public mothernode,
					   public hise::TempoListener
	{
	public:

		virtual ~clock_base()
		{
			if (tempoSyncer != nullptr)
				tempoSyncer->deregisterItem(this);
		}

		SN_EMPTY_INITIALISE;
		SN_EMPTY_RESET;
		SN_EMPTY_PROCESS;
		SN_EMPTY_PROCESS_FRAME;
		SN_EMPTY_HANDLE_EVENT

		void prepare(PrepareSpecs ps)
		{
			jassert(ps.voiceIndex != nullptr);

            if(tempoSyncer == nullptr)
            {
                tempoSyncer = ps.voiceIndex->getTempoSyncer();
                tempoSyncer->registerItem(this);
            }
		}

	protected:

		DllBoundaryTempoSyncer* tempoSyncer = nullptr;
	};

	class tempo_sync_base: public clock_base
	{
	public:

		virtual ~tempo_sync_base() {};

		virtual TempoData getUIData() const = 0;


	private:
		JUCE_DECLARE_WEAK_REFERENCEABLE(tempo_sync_base);
	};


	template <int NV> class tempo_sync : public tempo_sync_base,
										 public polyphonic_base,
										 public pimpl::no_mod_normalisation
	{
	public:

		static constexpr int NumVoices = NV;

		enum class Parameters
		{
			Tempo,
			Multiplier,
			Enabled,
			UnsyncedTime
		};

		DEFINE_PARAMETERS
		{
			DEF_PARAMETER(Tempo, tempo_sync);
			DEF_PARAMETER(Multiplier, tempo_sync);
			DEF_PARAMETER(Enabled, tempo_sync);
			DEF_PARAMETER(UnsyncedTime, tempo_sync);
		}
		SN_PARAMETER_MEMBER_FUNCTION;

		tempo_sync() :
			polyphonic_base(getStaticId()),
			no_mod_normalisation(getStaticId(), {})
		{};

		SN_POLY_NODE_ID("tempo_sync");

		SN_GET_SELF_AS_OBJECT(tempo_sync);
		SN_DESCRIPTION("Sends the tempo duration as modulation signal");

		SN_EMPTY_INITIALISE;
		SN_EMPTY_RESET;
		SN_EMPTY_PROCESS;
		SN_EMPTY_PROCESS_FRAME;
		SN_EMPTY_HANDLE_EVENT

		void prepare(PrepareSpecs ps)
		{
			clock_base::prepare(ps);
			data.prepare(ps);
		}

		~tempo_sync()
		{
			
		}

		void createParameters(ParameterDataList& data)
		{
			{
				DEFINE_PARAMETERDATA(tempo_sync, Tempo);
				p.setParameterValueNames(TempoSyncer::getTempoNames());
				data.add(std::move(p));
			}
			{
				DEFINE_PARAMETERDATA(tempo_sync, Multiplier);
				p.setRange({ 1, 16, 1.0 });
				p.setDefaultValue(1.0);
				data.add(std::move(p));
			}
			{
				DEFINE_PARAMETERDATA(tempo_sync, Enabled);
				p.setRange({ 0.0, 1.0, 1.0 });
				p.setDefaultValue(0.0);
				data.add(std::move(p));
			}
			{
				DEFINE_PARAMETERDATA(tempo_sync, UnsyncedTime);
				p.setRange({ 0.0, 1000.0, 0.1 });
				p.setDefaultValue(200.0);
				data.add(std::move(p));
			}
		}

		void tempoChanged(double newTempo) override
		{
			for (auto& t : data)
			{
				t.bpm = newTempo;
				t.refresh();
			}
		}

		void setTempo(double newTempoIndex)
		{
			for (auto& t : data)
			{
				t.currentTempo = (TempoSyncer::Tempo)jlimit<int>(0, TempoSyncer::numTempos - 1, (int)newTempoIndex);
				t.refresh();
			}
		}

		bool handleModulation(double& max)
		{
			if(data.isMonophonicOrInsideVoiceRendering())
				return data.get().handleModulation(max);

			return false;
		}

		void setMultiplier(double newMultiplier)
		{
			for (auto& t : data)
			{
				t.multiplier = jlimit(1.0, 32.0, newMultiplier);
				t.refresh();
			}
		}

		void setEnabled(double v)
		{
			for (auto& t : data)
			{
				t.enabled = v > 0.5;
				t.refresh();
			}
		}

		void setUnsyncedTime(double v)
		{
			for (auto& t : data)
			{
				t.unsyncedTime = v;
				t.refresh();
			}
		}

		static void tempoChangedStatic(void* obj, double newBpm)
		{
			auto s = static_cast<tempo_sync*>(obj);
			s->tempoChanged(newBpm);
		}

		TempoData getUIData() const override { return data.getFirst(); }

		PolyData<TempoData, NV> data;

		JUCE_DECLARE_WEAK_REFERENCEABLE(tempo_sync);
	};

	template <typename T, int NV> class transport_base : public clock_base
	{
	public:

		SN_EMPTY_CREATE_PARAM;

		static constexpr int NumVoices = NV;

		void prepare(PrepareSpecs ps)
		{
			clock_base::prepare(ps);
			polyValue.prepare(ps);
		}

		bool handleModulation(double& v)
		{
			if (polyValue.get() != value)
			{
				v = (double)value;
				polyValue.get() = value;
				return true;
			}

			return false;
		}

	protected:

		PolyData<T, NumVoices> polyValue;

		T value = false;
	};

	template <int NV> class transport : public transport_base<bool, NV>,
									    public polyphonic_base
	{
	public:

		transport():
		  polyphonic_base(getStaticId(), false)
		{};

        static constexpr int NumVoices = NV;
        
		SN_GET_SELF_AS_OBJECT(transport);
		SN_DESCRIPTION("Sends a modulation signal when the transport state changes");
		SN_POLY_NODE_ID("transport");
		
		void onTransportChange(bool isPlaying, double /*ppqPosition*/) override
		{
			this->value = isPlaying;
		}
	};

	template <int NV> class ppq : public transport_base<double, NV>,
								  public polyphonic_base
	{
	public:

        static constexpr int NumVoices = NV;
        
		ppq():
		  polyphonic_base(getStaticId(), false)
		{
			loopLengthQuarters = TempoSyncer::getTempoFactor(TempoSyncer::Tempo::Quarter);
		}

		SN_GET_SELF_AS_OBJECT(ppq);
		SN_DESCRIPTION("Sends a modulation signal with the playback position in quarters when the clock starts.");
		SN_POLY_NODE_ID("ppq");

		void onTransportChange(bool isPlaying, double ppqPosition) override
		{
			if (isPlaying)
			{
				ppqPos = ppqPosition;
				updateValue();
			}
		}
        
        void onResync(double newPos) override
        {
            ppqPos = newPos;
            updateValue();
        }

		template <int P> void setParameter(double v)
		{
			if (P == 0)
				t = (TempoSyncer::Tempo)(int)v;
			else if (P == 1)
				factor = jlimit(1.0, 64.0, v);
			
			loopLengthQuarters = TempoSyncer::getTempoFactor(t) * factor;

			if (loopLengthQuarters == 0.0)
				loopLengthQuarters = 1.0;

			updateValue();
		}

		SN_FORWARD_PARAMETER_TO_MEMBER(ppq);
		
		void createParameters(ParameterDataList& data)
		{
			{
				parameter::data p("Tempo", { 0.0, 1.0 });
				p.setParameterValueNames(TempoSyncer::getTempoNames());
				p.setParameterCallbackWithIndex<ppq, 0>(this);
				p.setDefaultValue((double)TempoSyncer::getTempoIndex("1/4"));
				data.add(std::move(p));
			}
			{
				parameter::data p("Multiplier", { 1.0, 16.0, 1.0 });
				p.setParameterCallbackWithIndex<ppq, 1>(this);
				p.setDefaultValue(1.0);
				data.add(std::move(p));
			}
		}

		void updateValue()
		{
			this->value = hmath::fmod(ppqPos, loopLengthQuarters) / loopLengthQuarters;
		}

		double ppqPos = 0.0;
		TempoSyncer::Tempo t;
		double factor;

		double loopLengthQuarters;
		
	};

    

	template <int NumValues> struct pack_writer: public data::base,
                                                 public pimpl::no_parameter,
                                                 public pimpl::no_processing
    {
        static Identifier getStaticId() { return Identifier("pack" + String(NumValues) + "_writer"); };
        
        SN_GET_SELF_AS_OBJECT(pack_writer);
        
        SN_DESCRIPTION("Writes the values from the parameter sliders into a slider pack");
        
        void setExternalData(const ExternalData& ed, int index) override
        {
            data::base::setExternalData(ed, index);
            
            if(auto sp = dynamic_cast<SliderPackData*>(this->externalData.obj))
            {
                sp->setNumSliders(NumValues);
            }
        }
        
        template <int P> void setParameter(double v)
        {
            if(auto sp = dynamic_cast<SliderPackData*>(this->externalData.obj))
            {
                DataReadLock sl(this);
                sp->setValue(P, v, sendNotificationAsync, false);
            }
        }
        
        template <int P> parameter::data createParameter()
        {
            parameter::data p("Value" + String(P+1), { 0.0, 1.0 });
            registerCallback<P>(p);
            return p;
        }
        
        void createParameters(ParameterDataList& data)
        {
#define ADD_PARAMETER(X) if (NumValues > X) data.add(createParameter<X>());
            ADD_PARAMETER(0); ADD_PARAMETER(1); ADD_PARAMETER(2); ADD_PARAMETER(3);
            ADD_PARAMETER(4); ADD_PARAMETER(5); ADD_PARAMETER(6); ADD_PARAMETER(7);
            ADD_PARAMETER(0+8); ADD_PARAMETER(1+8); ADD_PARAMETER(2+8); ADD_PARAMETER(3+8);
            ADD_PARAMETER(4+8); ADD_PARAMETER(5+8); ADD_PARAMETER(6+8); ADD_PARAMETER(7+8);
#undef ADD_PARAMETER
        }
        
        SN_FORWARD_PARAMETER_TO_MEMBER(pack_writer<NumValues>);
    };

    using pack2_writer = pack_writer<2>;
    using pack3_writer = pack_writer<3>;
    using pack4_writer = pack_writer<4>;
    using pack5_writer = pack_writer<5>;
    using pack6_writer = pack_writer<6>;
    using pack7_writer = pack_writer<7>;
    using pack8_writer = pack_writer<8>;


    struct pack_resizer: public data::base,
                         public pimpl::no_parameter,
                         public pimpl::no_processing
                         
    {
        SN_NODE_ID("pack_resizer");
        SN_GET_SELF_AS_OBJECT(pack_resizer);
        
        SN_DESCRIPTION("Dynamically resizes a slider pack");
        
        template <int P> void setParameter(double v)
        {
            if(auto sp = dynamic_cast<SliderPackData*>(this->externalData.obj))
            {
                DataWriteLock sl(this);
                
                auto newNumSliders = jlimit<int>(1, 128, roundToInt(v));
                
                sp->setNumSliders(roundToInt(newNumSliders));
            }
        }
        
        void createParameters(ParameterDataList& data)
        {
            {
                parameter::data p("NumSliders", { 0.0, 128.0, 1.0 });
                p.callback = parameter::inner<pack_resizer, 0>(*this);
                data.add(std::move(p));
            }
        }
        
        SN_FORWARD_PARAMETER_TO_MEMBER(pack_resizer);
        
        float something = 90.0f;
    };


	template <typename ParameterClass> struct resetter : public mothernode,
														 public pimpl::no_processing,
														 public pimpl::parameter_node_base<ParameterClass>
	{
		SN_NODE_ID("resetter");
		SN_GET_SELF_AS_OBJECT(resetter);
		SN_PARAMETER_NODE_CONSTRUCTOR(resetter, ParameterClass);
		SN_DESCRIPTION("Sends an inverted impulse (0,1) to reset gate-like parameters");

		template <int P> void setParameter(double v)
		{
			flashCounter++;
			this->getParameter().call(0.0);
			this->getParameter().call(1.0);
		}

		void createParameters(ParameterDataList& data)
		{
			{
				parameter::data p("Value", { 0.0, 1.0 });
				registerCallback<0>(p);
				data.add(std::move(p));
			}
		}

		int flashCounter = 0;

		SN_FORWARD_PARAMETER_TO_MEMBER(resetter);

		JUCE_DECLARE_WEAK_REFERENCEABLE(resetter);
	};

	struct MidiCCHelpers
	{
		enum class SpecialControllers
		{
			ModWheel = 1,
			BreathControl = 2,
			Volume = 7,
			Expression = 11,
			Sustain = 64,
			Aftertouch = HiseEvent::AfterTouchCCNumber,
			Pitchbend = HiseEvent::PitchWheelCCNumber,
			Stroke = 130,
			Release = 131
		};

		static bool isMPEProperty(int zeroBasedNumber)
		{
			return zeroBasedNumber == (int)SpecialControllers::Aftertouch ||
				   zeroBasedNumber == (int)SpecialControllers::Stroke ||
				   zeroBasedNumber == (int)SpecialControllers::Pitchbend ||
				   zeroBasedNumber == (int)SpecialControllers::Release;
		}

		static HiseEvent::Type getTypeForNumber(int midiNumber)
		{
			switch (midiNumber)
			{
			case (int)SpecialControllers::Pitchbend:
				return HiseEvent::Type::PitchBend;
			case (int)SpecialControllers::Aftertouch:
				return HiseEvent::Type::Aftertouch;
			case (int)SpecialControllers::Stroke:
				return HiseEvent::Type::NoteOn;
			case (int)SpecialControllers::Release:
				return HiseEvent::Type::NoteOff;
			default:
				return HiseEvent::Type::Controller;
			}
		}

		static StringArray createMidiCCNames()
		{
			StringArray sa;

			for (int i = 0; i < 132; i++)
				sa.add(String("CC " + String(i)));

			sa.set((int)SpecialControllers::ModWheel, "Modwheel");
			sa.set((int)SpecialControllers::BreathControl, "Breath Control");
			sa.set((int)SpecialControllers::Expression, "Expression");
			sa.set((int)SpecialControllers::Sustain, "Sustain");
			sa.set((int)SpecialControllers::Volume, "Volume");
			sa.set((int)SpecialControllers::Aftertouch, "Aftertouch");
			sa.set((int)SpecialControllers::Pitchbend, "Pitchbend");
			sa.set((int)SpecialControllers::Stroke, "Stroke");
			sa.set((int)SpecialControllers::Release, "Release");

			return sa;
		}
	};

	template <typename ParameterClass> struct midi_cc : 
		public mothernode,
		public pimpl::no_processing,
		public pimpl::parameter_node_base<ParameterClass>
	{
		enum Parameters
		{
			CCNumber,
			EnableMPE,
			DefaultValue,
		};

		DEFINE_PARAMETERS
		{
			DEF_PARAMETER(CCNumber, midi_cc);
			DEF_PARAMETER(EnableMPE, midi_cc);
			DEF_PARAMETER(DefaultValue, midi_cc);
		}
		SN_PARAMETER_MEMBER_FUNCTION;

		SN_NODE_ID("midi_cc");
		SN_GET_SELF_AS_OBJECT(midi_cc);
		SN_PARAMETER_NODE_CONSTRUCTOR(midi_cc, ParameterClass);
		SN_DESCRIPTION("sends a MIDI cc value");

		void createParameters(ParameterDataList& data)
		{
			{
				DEFINE_PARAMETERDATA(midi_cc, CCNumber);
				auto sa = MidiCCHelpers::createMidiCCNames();
				p.setParameterValueNames(sa);
                p.setDefaultValue(1.0);
				data.add(std::move(p));
			}

			{
				DEFINE_PARAMETERDATA(midi_cc, EnableMPE);
				p.setParameterValueNames({ "On", "Off" });
				data.add(std::move(p));
			}

			{
				DEFINE_PARAMETERDATA(midi_cc, DefaultValue);
				data.add(std::move(p));
			}
		}

		void prepare(PrepareSpecs ps)
		{
		}

		void handleHiseEvent(HiseEvent& e)
		{
			auto thisType = e.getType();

			if (thisType == HiseEvent::Type::Controller && e.getControllerNumber() != midiNumber)
				return;

			if (thisType == expectedType)
			{
				double v = 0.0;

				switch (thisType)
				{
				case HiseEvent::Type::PitchBend:
					v = (double)e.getPitchWheelValue() / (8192.0 * 2.0);
					break;
				case HiseEvent::Type::Aftertouch:
					v = (double)e.getNoteNumber() / 127.0;
					break;
				case HiseEvent::Type::Controller:
					v = (double)e.getControllerValue() / 127.0;
					break;
				case HiseEvent::Type::NoteOn:
					v = (double)e.getVelocity() / 127.0;
					break;
				case HiseEvent::Type::NoteOff:
					v = (double)e.getVelocity() / 127.0;
					break;
                default:
                    break;
				}

				if (this->getParameter().isConnected())
					this->getParameter().call(v);
			}
		}

		void setCCNumber(double v)
		{
			midiNumber = roundToInt(v);
			expectedType = MidiCCHelpers::getTypeForNumber(midiNumber);
		}

		void setEnableMPE(double shouldBeEnabled)
		{
			enableMpe = shouldBeEnabled;
		}

		void setDefaultValue(double newDefaultValue)
		{
			defaultValue = newDefaultValue;

			if (this->getParameter().isConnected())
				this->getParameter().call(defaultValue);
		}

		double defaultValue = 1.0;

		bool isInPolyphonicContext = false;
		bool enableMpe = false;
		int midiNumber = 1;
		HiseEvent::Type expectedType = HiseEvent::Type::Controller;

		JUCE_DECLARE_WEAK_REFERENCEABLE(midi_cc);
	};

	template <typename ExpressionClass, typename ParameterClass> struct cable_expr : public mothernode,
																					 public pimpl::parameter_node_base<ParameterClass>,
																					 public pimpl::no_mod_normalisation,
																					 public pimpl::no_processing
	{
		

		SN_NODE_ID("cable_expr");
		SN_GET_SELF_AS_OBJECT(cable_expr);
		SN_DESCRIPTION("evaluates an expression for the control value");

		SN_ADD_SET_VALUE(cable_expr);
		
		cable_expr() : 
			pimpl::parameter_node_base<ParameterClass>(getStaticId()),
			no_mod_normalisation(getStaticId(), { "Value" })
		{};

		SN_DEFAULT_INIT(ExpressionClass);

		void setValue(double input)
		{
			auto v = obj.op(input);

			if (this->getParameter().isConnected())
				this->getParameter().call(v);
		}

		ExpressionClass obj;
		double lastValue = 0.0;
	};

	template <typename ParameterClass> struct voice_bang : public mothernode,
														   public pimpl::parameter_node_base<ParameterClass>,
														   public pimpl::no_processing
	{
		SN_NODE_ID("voice_bang");
		SN_GET_SELF_AS_OBJECT(voice_bang);
		SN_DESCRIPTION("sends out the current value when a voice is started (note-on is received)");

		SN_ADD_SET_VALUE(voice_bang);

		voice_bang() :
			pimpl::parameter_node_base<ParameterClass>(getStaticId())
		{};

		void prepare(PrepareSpecs ps)
		{
			if (ps.voiceIndex == nullptr || !ps.voiceIndex->isEnabled())
			{
				scriptnode::Error::throwError(Error::IllegalMonophony);
				return;
			}
		}

		void handleHiseEvent(HiseEvent& e)
		{
			if (e.isNoteOn())
			{
				if (this->getParameter().isConnected())
					this->getParameter().call(value);
			}
		}

		void setValue(double input)
		{
			value = input;
		}

		double value = 0.0;
	};

	

	template <typename ParameterClass> struct normaliser : public mothernode,
														   public pimpl::parameter_node_base<ParameterClass>,
														   public pimpl::no_processing
	{


		SN_NODE_ID("normaliser");
		SN_GET_SELF_AS_OBJECT(normaliser);
		SN_DESCRIPTION("normalises the input value");

		SN_ADD_SET_VALUE(normaliser);

		normaliser() :
			pimpl::parameter_node_base<ParameterClass>(getStaticId())
		{};

		void setValue(double input)
		{
			if (this->getParameter().isConnected())
				this->getParameter().call(input);
		}
	};

    template <typename ParameterClass, typename ConverterClass>
        struct converter : public mothernode,
						   public pimpl::templated_mode,
						   public pimpl::no_mod_normalisation,
                           public pimpl::parameter_node_base<ParameterClass>,
                           public pimpl::no_processing
    {
        SN_NODE_ID("converter");
        SN_GET_SELF_AS_OBJECT(converter);
        SN_DESCRIPTION("converts a control value");

        SN_DEFAULT_INIT(ConverterClass);
        SN_DEFAULT_PREPARE(ConverterClass);
        SN_ADD_SET_VALUE(converter);

		converter() :
			pimpl::templated_mode(getStaticId(), "conversion_logic"),
			pimpl::no_mod_normalisation(getStaticId(), { "Value" }),
			pimpl::parameter_node_base<ParameterClass>(getStaticId())
		{};

        void setValue(double input)
        {
            auto v = obj.getValue(input);

            if (this->getParameter().isConnected())
                this->getParameter().call(v);
        }

        ConverterClass obj;
    };

	template <typename ParameterClass> struct random : public mothernode,
                           public pimpl::parameter_node_base<ParameterClass>,
                           public pimpl::no_processing
    {
        SN_NODE_ID("random");
        SN_GET_SELF_AS_OBJECT(random);
        SN_DESCRIPTION("creates a random value");

        SN_ADD_SET_VALUE(random);

		random() :
			pimpl::parameter_node_base<ParameterClass>(getStaticId())
		{};

        void setValue(double)
        {
            if (this->getParameter().isConnected())
                this->getParameter().call(r.nextDouble());
        }

		Random r;
    };

	template <typename ParameterClass> struct cable_table : public scriptnode::data::base,
															public pimpl::parameter_node_base<ParameterClass>,
															public pimpl::no_processing
	{
		SN_NODE_ID("cable_table");
		SN_GET_SELF_AS_OBJECT(cable_table);
		SN_DESCRIPTION("Modify a modulation signal using a lookup table");

		SN_ADD_SET_VALUE(cable_table);
		SN_PARAMETER_NODE_CONSTRUCTOR(cable_table, ParameterClass);

		void setExternalData(const ExternalData& d, int index) override
		{
			base::setExternalData(d, index);
			this->externalData.referBlockTo(tableData, 0);

			setValue(lastValue);
		}

		void setValue(double input)
		{
			if (!tableData.isEmpty())
			{
				lastValue = input;
				InterpolatorType ip(input);
				auto tv = tableData[ip];

				if(this->getParameter().isConnected())
					this->getParameter().call(tv);

				this->externalData.setDisplayedValue(input);
			}
		}

		using TableClampType = index::clamped<SAMPLE_LOOKUP_TABLE_SIZE>;
		using InterpolatorType = index::lerp<index::normalised<double, TableClampType>>;

		block tableData;
		double lastValue = 0.0;
	};

	template <typename ParameterType> struct clone_pack: public data::base,
														 public control::pimpl::no_processing,
                                                         public wrap::clone_manager::Listener,
														 public pimpl::parameter_node_base<ParameterType>,
														 public hise::ComplexDataUIUpdaterBase::EventListener
	{
		SN_GET_SELF_AS_OBJECT(clone_pack);
		SN_NODE_ID("clone_pack");
		
		enum class Parameters
		{
			NumClones,
			Value,
		};

		DEFINE_PARAMETERS
		{
			DEF_PARAMETER(NumClones, clone_pack);
			DEF_PARAMETER(Value, clone_pack);
		};
		SN_PARAMETER_MEMBER_FUNCTION;

		SN_DESCRIPTION("control cloned parameters with a slider pack");
		
        clone_pack() : 
			pimpl::parameter_node_base<ParameterType>(getStaticId())
		{
			cppgen::CustomNodeProperties::setPropertyForObject(*this, PropertyIds::IsCloneCableNode);
            this->getParameter().setParentNumClonesListener(this);
		};

		void onComplexDataEvent(hise::ComplexDataUIUpdaterBase::EventType t, var data) override
		{
			if (t == ComplexDataUIUpdaterBase::EventType::ContentChange)
			{
				auto changedIndex = (int)data;

				if(isPositiveAndBelow(changedIndex, numClones))
				{
					if (auto sp = dynamic_cast<SliderPackData*>(this->externalData.obj))
					{
						auto v = sp->getValue(changedIndex) * lastValue;
						this->p.callEachClone(changedIndex, v, false);
					}
				}
			}
		}

		void setExternalData(const snex::ExternalData& d, int index) override
		{
			if (auto existing = this->externalData.obj)
				existing->getUpdater().removeEventListener(this);

			base::setExternalData(d, index);

			if (auto existing = this->externalData.obj)
				existing->getUpdater().addEventListener(this);
			
			this->externalData.referBlockTo(sliderData, 0);

			setValue(lastValue);
		}

		void setValue(double newValue)
		{
			lastValue = newValue;
			auto numToIterate = jmin(sliderData.size(), numClones);

			for (int i = 0; i < numToIterate; i++)
			{
				auto valueToSend = sliderData[i] * lastValue;
				this->getParameter().callEachClone(i, valueToSend, false);
			}
		}

        void numClonesChanged(int newSize) override
        {
            setNumClones(newSize);
        }
        
		void setNumClones(double newNumClones)
		{
			if (numClones != newNumClones)
			{
				int oldNumClones = numClones;

				numClones = jlimit(1, 128, (int)newNumClones);
				auto numToIterate = jmin(numClones, sliderData.size());

				for(int i = oldNumClones; i < numToIterate; i++)
				{
					auto v = sliderData[i] * lastValue;
					this->p.callEachClone(i, v, false);
				}
			}
		}

		void createParameters(ParameterDataList& data)
		{
			{
				DEFINE_PARAMETERDATA(clone_pack, NumClones);
				p.setRange({ 1.0, 16.0, 1.0 });
				p.setDefaultValue(1.0);
				data.add(std::move(p));
			}
			{
				DEFINE_PARAMETERDATA(clone_pack, Value);
				p.setRange({ 0.0, 1.0 });
				p.setDefaultValue(1.0);
				data.add(std::move(p));
			}
		}

		double lastValue = 0.0;
		block sliderData;
		int numClones = 1;
	};
		

	template <typename ParameterType, typename LogicType> 
		struct clone_cable : public control::pimpl::no_processing,
							 public pimpl::parameter_node_base<ParameterType>,
							 public mothernode,
							 public wrap::clone_manager::Listener,
							 public control::pimpl::templated_mode								
	{
		SN_GET_SELF_AS_OBJECT(clone_cable);
		SN_NODE_ID("clone_cable");
		SN_DESCRIPTION("Send different values to cloned nodes");

		clone_cable():
            control::pimpl::parameter_node_base<ParameterType>(getStaticId()),
            control::pimpl::templated_mode(getStaticId(), "duplilogic")
		{
			cppgen::CustomNodeProperties::setPropertyForObject(*this, PropertyIds::IsCloneCableNode);
			cppgen::CustomNodeProperties::addNodeIdManually(getStaticId(), PropertyIds::IsProcessingHiseEvent);

			this->getParameter().setParentNumClonesListener(this);
		};

		enum class Parameters
		{
			NumClones,
			Value,
			Gamma,
		};

		DEFINE_PARAMETERS
		{
			DEF_PARAMETER(NumClones, clone_cable);
			DEF_PARAMETER(Value, clone_cable);
			DEF_PARAMETER(Gamma, clone_cable);
		};
		SN_PARAMETER_MEMBER_FUNCTION;

		void initialise(NodeBase* n)
		{
			if constexpr (prototypes::check::initialise<LogicType>::value)
				obj.initialise(n);
		}

		static constexpr bool isProcessingHiseEvent() 
		{ 
			return (bool)prototypes::check::isProcessingHiseEvent<LogicType>::value; 
		}

		void setValue(double v)
		{
 			lastValue = v;
			sendValue();
		}

		void numClonesChanged(int newNumClones) override
		{
			if(shouldUpdateClones())
				setNumClones(newNumClones);
		}

		bool shouldUpdateClones() const
		{
			return obj.shouldUpdateNumClones();
		}

		void handleHiseEvent(HiseEvent& e)
		{
			if constexpr (isProcessingHiseEvent())
			{
				double v = lastValue;

				if (obj.getMidiValue(e, v))
					setValue(v);
			}
		}

		void setGamma(double gamma)
		{
			lastGamma = jlimit(0.0, 1.0, gamma);
			sendValue();
		}

		void setNumClones(double newNumClones)
		{
			if (numClones != newNumClones)
			{
				numClones = jlimit(1, 128, (int)newNumClones);
				sendValue();
			}
		}

		void sendValue()
		{
			for (int i = 0; i < numClones; i++)
			{
				auto valueToSend = obj.getValue(i, numClones, lastValue, lastGamma);
				this->getParameter().callEachClone(i, valueToSend, !obj.shouldUpdateNumClones());
			}
		}

		void createParameters(ParameterDataList& data)
		{
			{
				DEFINE_PARAMETERDATA(clone_cable, NumClones);
				p.setRange({ 1.0, 16.0, 1.0 });
				p.setDefaultValue(1.0);
				data.add(std::move(p));
			}
			{
				DEFINE_PARAMETERDATA(clone_cable, Value);
				p.setRange({ 0.0, 1.0 });
				p.setDefaultValue(0.0);
				data.add(std::move(p));
			}
			{
				DEFINE_PARAMETERDATA(clone_cable, Gamma);
				p.setRange({ 0.0, 1.0 });
				p.setDefaultValue(0.0);
				data.add(std::move(p));
			}
		}

		double lastValue = 0.0;
		double lastGamma = 0.0;
		int numClones;
		LogicType obj;

		JUCE_DECLARE_WEAK_REFERENCEABLE(clone_cable);
	};

	template <typename ParameterType> 
		struct clone_forward : public control::pimpl::no_processing,
							   public pimpl::parameter_node_base<ParameterType>,
							   public mothernode,
							   public wrap::clone_manager::Listener
	{
		SN_GET_SELF_AS_OBJECT(clone_forward);
		SN_NODE_ID("clone_forward");
		SN_DESCRIPTION("forwards the unscaled input parameter to all clones");

		static constexpr bool isNormalisedModulation() { return false; }
		static constexpr bool isProcessingHiseEvent()  { return false; }
		static constexpr bool isPolyphonic()  { return false; }

		clone_forward():
            control::pimpl::parameter_node_base<ParameterType>(getStaticId())
		{
			cppgen::CustomNodeProperties::setPropertyForObject(*this, PropertyIds::IsCloneCableNode);

			cppgen::CustomNodeProperties::setPropertyForObject(*this, PropertyIds::UseUnnormalisedModulation);
			cppgen::CustomNodeProperties::addUnscaledParameter(getStaticId(), "Value");

			this->getParameter().setParentNumClonesListener(this);
			this->getParameter().isNormalised = isNormalisedModulation();
		};

		enum class Parameters
		{
			NumClones,
			Value
		};

		DEFINE_PARAMETERS
		{
			DEF_PARAMETER(NumClones, clone_forward);
			DEF_PARAMETER(Value, clone_forward);
		};
		SN_PARAMETER_MEMBER_FUNCTION;

		SN_EMPTY_INITIALISE;
		SN_EMPTY_HANDLE_EVENT;
		
		void setValue(double v)
		{
 			lastValue = v;
			sendValue();
		}

		void numClonesChanged(int newNumClones) override
		{
			setNumClones(newNumClones);
		}

		bool shouldUpdateClones() const
		{
			return true;
		}
		
		void setNumClones(double newNumClones)
		{
			if (numClones != newNumClones)
			{
				numClones = jlimit(1, 128, (int)newNumClones);
				sendValue();
			}
		}

		void sendValue()
		{
			for (int i = 0; i < numClones; i++)
			{
				this->getParameter().callEachClone(i, lastValue, false);
			}
		}

		void createParameters(ParameterDataList& data)
		{
			{
				DEFINE_PARAMETERDATA(clone_cable, NumClones);
				p.setRange({ 1.0, 16.0, 1.0 });
				p.setDefaultValue(1.0);
				data.add(std::move(p));
			}
			{
				DEFINE_PARAMETERDATA(clone_cable, Value);
				p.setRange({ 0.0, 1.0 });
				p.setDefaultValue(0.0);
				data.add(std::move(p));
			}
		}

		double lastValue = 0.0;
		int numClones;
		
		JUCE_DECLARE_WEAK_REFERENCEABLE(clone_forward);
	};

	
	template <typename ParameterClass, typename FaderClass> struct xfader: public pimpl::parameter_node_base<ParameterClass>,
																		   public control::pimpl::templated_mode,
																		   public pimpl::no_processing
	{
		SN_NODE_ID("xfader");
		SN_GET_SELF_AS_OBJECT(xfader);
		SN_TEMPLATED_MODE_PARAMETER_NODE_CONSTRUCTOR(xfader, ParameterClass, "faders");
		SN_DESCRIPTION("Apply a crossfade to multiple outputs");

		SN_ADD_SET_VALUE(xfader);

		void initialise(NodeBase* n) override
		{
			this->p.initialise(n);
			fader.initialise(n);
		}

		void setValue(double v)
		{
			lastValue.setModValueIfChanged(v);

			using FaderType = faders::switcher;

			callFadeValue<0>(v);
			callFadeValue<1>(v);
			callFadeValue<2>(v);
			callFadeValue<3>(v);
			callFadeValue<4>(v);
			callFadeValue<5>(v);
			callFadeValue<6>(v);
			callFadeValue<7>(v);
			callFadeValue<8>(v);
		}

		template <int P> void callFadeValue(double v)
		{
			if constexpr (ParameterClass::isStaticList())
			{
				if constexpr (P < ParameterClass::getNumParameters())
					this->p.template call<P>(fader.template getFadeValue<P>(this->p.getNumParameters(), v));
			}
			else
			{
				if (P < this->p.getNumParameters())
					this->p.template call<P>(fader.template getFadeValue<P>(this->p.getNumParameters(), v));
			}
		}

		ModValue lastValue;

		FaderClass fader;

		JUCE_DECLARE_WEAK_REFERENCEABLE(xfader);
	};

	namespace multilogic
	{
		struct intensity
		{
			SN_NODE_ID("intensity");
			SN_DESCRIPTION("applies the HISE modulation intensity to the value");

			static constexpr bool isNormalisedModulation() { return true; }
            static constexpr bool needsProcessing() { return false; }

			double intensityValue = 1.0;
			double value = 1.0;
			mutable bool dirty = false;

			bool operator==(const intensity& other) const
			{
				return value == other.value && intensityValue == other.intensityValue;
			}

			double getValue() const
			{
				dirty = false;

				return 1.0 * (1.0 - intensityValue) + intensityValue * value;
			}

			template <int P> void setParameter(double v)
			{
				if constexpr (P == 0)
					value = v;
				if constexpr (P == 1)
					intensityValue = jlimit(0.0, 1.0, v);
				
				dirty = true;
			}

			template <typename NodeType> static void createParameters(ParameterDataList& data, NodeType& n)
			{
				{
					parameter::data p("Value");
					p.template setParameterCallbackWithIndex<NodeType, 0>(&n);
					p.setRange({ 0.0, 1.0 });
					p.setDefaultValue(0.0);
					data.add(std::move(p));
				}
				{
					parameter::data p("Intensity");
					p.template setParameterCallbackWithIndex<NodeType, 1>(&n);
					p.setRange({ 0.0, 1.0 });
					p.setDefaultValue(1.0);
					data.add(std::move(p));
				}
			}
		};

		struct bang: public pimpl::no_mod_normalisation
		{
			SN_NODE_ID("bang");
			SN_DESCRIPTION("send the value when the bang input changes");

			bang() :
				no_mod_normalisation(getStaticId(), { "Value" })
			{};

			static constexpr bool isNormalisedModulation() { return false; }
            static constexpr bool needsProcessing() { return false; }

			bool operator==(const bang& other) const
			{
				return value == other.value;
			}

			double getValue() const
			{
				dirty = false;
				return value;
			}

			template <typename NodeType> static void createParameters(ParameterDataList& data, NodeType& n)
			{
				{
					parameter::data p("Value");
					p.template setParameterCallbackWithIndex<NodeType, 0>(&n);
					p.setRange({ 0.0, 1.0 });
					p.setDefaultValue(0.0);
					data.add(std::move(p));
				}
				{
					parameter::data p("Bang");
					p.template setParameterCallbackWithIndex<NodeType, 1>(&n);
					p.setRange({ 0.0, 1.0, 1.0 });
					p.setDefaultValue(0.0);
					data.add(std::move(p));
				}
			}

			template <int P> void setParameter(double v)
			{
				if constexpr (P == 0)
					value = v;
				if constexpr (P == 1)
					dirty = v > 0.5;
			}

			double value;
			mutable bool dirty = false;
		};

		struct bipolar
		{
			SN_NODE_ID("bipolar");
			SN_DESCRIPTION("Creates a bipolar mod signal from a 0...1 range");

			static constexpr bool isNormalisedModulation() { return true; }
            static constexpr bool needsProcessing() { return false; }

			bool operator==(const bipolar& other) const
			{
				return value == other.value && gamma == other.gamma && scale == other.scale;
			}

			double getValue() const
			{
				dirty = false;
				double v = value - 0.5f;

				if (gamma != 1.0)
					v = hmath::pow(hmath::abs(v * 2.0), gamma) * hmath::sign(v) * 0.5;

				v *= scale;
				v += 0.5;
				return v;
			}

			template <int P> void setParameter(double v)
			{
				if constexpr (P == 0)
					value = v;
				if constexpr (P == 1)
					scale = v;
				if constexpr (P == 2)
					gamma = v;

				dirty = true;
			}

			template <typename NodeType> static void createParameters(ParameterDataList& data, NodeType& n)
			{
				{
					parameter::data p("Value");
					p.template setParameterCallbackWithIndex<NodeType, 0>(&n);
					p.setRange({ 0.0, 1.0 });
					p.setDefaultValue(0.0);
					data.add(std::move(p));
				}
				{
					parameter::data p("Scale");
					p.template setParameterCallbackWithIndex<NodeType, 1>(&n);
					p.setRange({ -1.0, 1.0 });
					p.setDefaultValue(0.0);
					data.add(std::move(p));
				}
				{
					parameter::data p("Gamma");
					p.template setParameterCallbackWithIndex<NodeType, 2>(&n);
					p.setRange({ 0.5, 2.0 });
					p.setSkewForCentre(1.0);
					p.setDefaultValue(1.0);
					data.add(std::move(p));
				}
			}

			double value = 0.5;
			double scale = 0.0;
			double gamma = 1.0;
			mutable bool dirty = false;
		};

		struct blend
		{
			SN_NODE_ID("blend");
			SN_DESCRIPTION("Blends the two input values based on the Alpha parameter.");

			static constexpr bool isNormalisedModulation() { return false; }
            static constexpr bool needsProcessing() { return false; }

			bool operator==(const blend& other) const
			{
				return alpha == other.alpha && value1 == other.value1 && value2 == other.value2;
			}

			double getValue() const
			{
				dirty = false;
				return Interpolator::interpolateLinear(value1, value2, alpha);
			}

			template <int P> void setParameter(double v)
			{
				if constexpr (P == 0)
					alpha = v;
				if constexpr (P == 1)
					value1 = v;
				if constexpr (P == 2)
					value2 = v;

				dirty = true;
			}

			template <typename NodeType> static void createParameters(ParameterDataList& data, NodeType& n)
			{
				{
					parameter::data p("Alpha");
					p.template setParameterCallbackWithIndex<NodeType, 0>(&n);
					p.setDefaultValue(0.0);
					data.add(std::move(p));
				}
				{
					parameter::data p("Value1");
					p.template setParameterCallbackWithIndex<NodeType, 1>(&n);
					p.setDefaultValue(0.0);
					data.add(std::move(p));
				}
				{
					parameter::data p("Value2");
					p.template setParameterCallbackWithIndex<NodeType, 2>(&n);
					p.setDefaultValue(0.0);
					data.add(std::move(p));
				}
			}

			double alpha = 0.0;
			double value1 = 0.0;
			double value2 = 0.0;

			mutable bool dirty = false;
		};

		struct logic_op
		{
			enum class LogicState
			{
				Undefined,
				False,
				True
			};

			enum class LogicType
			{
				AND,
				OR,
				XOR,
				numLogicTypes
			};

			SN_NODE_ID("logic_op");
			SN_DESCRIPTION("Combines the (binary) input signals using a logic operator");

			static constexpr bool isNormalisedModulation() { return true; }
            static constexpr bool needsProcessing() { return false; }

			bool operator==(const logic_op& other) const
			{
				return logicType == other.logicType && leftValue == other.leftValue && rightValue == other.rightValue;
			}

			void reset()
			{
				leftValue = LogicState::Undefined;
				rightValue = LogicState::Undefined;
				dirty = false;
			}

			double getValue() const
			{
				dirty = false;

				auto lv = leftValue == LogicState::True;
				auto rv = rightValue == LogicState::True;

				switch (logicType)
				{
				case LogicType::AND: return lv && rv ? 1.0 : 0.0;
				case LogicType::OR: return lv || rv ? 1.0 : 0.0;
				case LogicType::XOR: return (lv || rv) && !(lv == rv) ? 1.0 : 0.0;
                default: return 0.0;
				}
			}

			template <int P> void setParameter(double v)
			{
				if constexpr (P == 0)
				{
					auto prevValue = leftValue;
					leftValue = LogicState(int(v > 0.5) + 1);
					dirty |= ((prevValue != leftValue) && (rightValue != LogicState::Undefined));
				}
					
				if constexpr (P == 1)
				{
					auto prevValue = rightValue;
					rightValue = LogicState(int(v > 0.5) + 1);
					dirty |= ((prevValue != rightValue) && (leftValue != LogicState::Undefined));
				}
					
				if constexpr (P == 2)
				{
					logicType = (LogicType)jlimit(0, 2, (int)v);
					dirty = true;
				}
			}

			template <typename NodeType> static void createParameters(ParameterDataList& data, NodeType& n)
			{
				{
					parameter::data p("Left");
					p.template setParameterCallbackWithIndex<NodeType, 0>(&n);
					p.setRange({ 0.0, 1.0 });
					p.setDefaultValue(0.0);
					data.add(std::move(p));
				}
				{
					parameter::data p("Right");
					p.template setParameterCallbackWithIndex<NodeType, 1>(&n);
					p.setRange({ 0.0, 1.0 });
					p.setDefaultValue(0.0);
					data.add(std::move(p));
				}
				{
					parameter::data p("Operator");
					p.template setParameterCallbackWithIndex<NodeType, 2>(&n);
					p.setParameterValueNames({ "AND", "OR", "XOR" });
					p.setDefaultValue(0.0);
					data.add(std::move(p));
				}
			}

			LogicState leftValue = LogicState::Undefined;
			LogicState rightValue = LogicState::Undefined;
			LogicType logicType = LogicType::AND;
			mutable bool dirty = false;

		};

		struct change: public pimpl::no_mod_normalisation
		{
			SN_NODE_ID("change");
			SN_DESCRIPTION("Filters out repetitions of the same value");

			change() :
				no_mod_normalisation(getStaticId(), { "Value" })
			{};

            static constexpr bool needsProcessing() { return false; }
            
			bool operator==(const change& other) const { return other.value == value; }

			double getValue() const
			{
				dirty = false;
				return value;
			}

			template <int P> void setParameter(double v)
			{
				if constexpr (P == 0)
				{
					dirty = v != value;
					value = v;
				}
			}

			template <typename NodeType> static void createParameters(ParameterDataList& data, NodeType& n)
			{
				{
					parameter::data p("Value");
					p.template setParameterCallbackWithIndex<NodeType, 0>(&n);
					p.setRange({ 0.0, 1.0 });
					p.setDefaultValue(0.0);
					data.add(std::move(p));
				}
			}

			double value = 0.0;
			mutable bool dirty = false;
		};
    
        struct delay_cable: public pimpl::no_mod_normalisation
        {
            SN_NODE_ID("delay_cable");
            SN_DESCRIPTION("Delays the message by a given amount");

            delay_cable() :
                no_mod_normalisation(getStaticId(), { "Value" })
            {};
            
            static constexpr bool needsProcessing() { return true; }

            bool operator==(const change& other) const { return other.value == value; }

            double getValue() const
            {
                dirty = false;
                return value;
            }

            template <typename T> void process(T& pd)
            {
                auto numSamples = pd.getNumSamples();
                
                if(wait)
                {
                    uptime += numSamples;
                    
                    if(uptime >= delayTimeSamples)
                    {
                        wait = false;
                        uptime = 0.0;
                        dirty = true;
                    }
                }
            }
            
            template <typename T> void processFrame(T&)
            {
                if(wait)
                {
                    if(++uptime >= delayTimeSamples)
                    {
                        wait = false;
                        uptime = 0.0;
                        dirty = true;
                    }
                }
            }
            
            template <int P> void setParameter(double v)
            {
                if constexpr (P == 0)
                {
                    value = v;
                    uptime = 0.0;
                    wait = true;
                    dirty = false;
                }
                if constexpr (P == 1)
                {
                    delayTimeSamples = v;
                }
            }

            template <typename NodeType> static void createParameters(ParameterDataList& data, NodeType& n)
            {
                {
                    parameter::data p("Value");
                    p.template setParameterCallbackWithIndex<NodeType, 0>(&n);
                    p.setRange({ 0.0, 1.0 });
                    p.setDefaultValue(0.0);
                    data.add(std::move(p));
                }
                {
                    parameter::data p("DelayTimeSamples");
                    p.template setParameterCallbackWithIndex<NodeType, 1>(&n);
                    p.setRange({ 0.0, 44100.0 });
                    p.setDefaultValue(0.0);
                    data.add(std::move(p));
                }
            }

            double value = 0.0;
            double delayTimeSamples = 0.0;
            double uptime = 0.0;
            bool wait = false;
            mutable bool dirty = false;
        };

		struct minmax: public pimpl::no_mod_normalisation
		{
			SN_NODE_ID("minmax");
			SN_DESCRIPTION("Scales the input value to a modifyable range");

			minmax() :
				no_mod_normalisation(getStaticId(), {}) // no unscaled input parameters...
			{};
            
            static constexpr bool needsProcessing() { return false; }

			bool operator==(const minmax& other) const { return other.range == range && value == other.value; }

			double getValue() const 
			{ 
				dirty = false; 
				auto v = range.convertFrom0to1(value, true);
				v = range.snapToLegalValue(v);
				return v;
			}

			template <int P> void setParameter(double v)
			{
                if constexpr (P == 0)
					value = v;
				if constexpr (P == 1)
                    range.rng.start = v;
				if constexpr (P == 2)
                    range.rng.end = v;
				if constexpr (P == 3)
					range.rng.skew = jlimit(0.1, 10.0, v);
				if constexpr (P == 4)
					range.rng.interval = v;
                if constexpr (P == 5)
                    range.inv = v > 0.5;
				
				range.checkIfIdentity();

				dirty = true;
			}

			template <typename NodeType> static void createParameters(ParameterDataList& data, NodeType& n)
			{
				{
					parameter::data p("Value");
					p.template setParameterCallbackWithIndex<NodeType, 0>(&n);
					p.setRange({ 0.0, 1.0 });
					p.setDefaultValue(0.0);
					data.add(std::move(p));
				}
				{
					parameter::data p("Minimum");
					p.template setParameterCallbackWithIndex<NodeType, 1>(&n);
					p.setRange({ 0.0, 1.0 });
					p.setDefaultValue(0.0);
					data.add(std::move(p));
				}
				{
					parameter::data p("Maximum");
					p.template setParameterCallbackWithIndex<NodeType, 2>(&n);
					p.setRange({ 0.0, 1.0 });
					p.setDefaultValue(1.0);
					data.add(std::move(p));
				}
				{
					parameter::data p("Skew");
					p.template setParameterCallbackWithIndex<NodeType, 3>(&n);
					p.setRange({ 0.1, 10.0 });
					p.setSkewForCentre(1.0);
					p.setDefaultValue(1.0);
					data.add(std::move(p));
				}
				{
					parameter::data p("Step");
					p.template setParameterCallbackWithIndex<NodeType, 4>(&n);
					p.setRange({ 0.0, 1.0 });
					p.setDefaultValue(0.0);
					data.add(std::move(p));
				}
                {
                    parameter::data p("Polarity");
					p.template setParameterCallbackWithIndex<NodeType, 5>(&n);
                    p.setParameterValueNames({"Normal", "Inverted"});
                    p.setDefaultValue(0.0);
                    data.add(std::move(p));
                }
			}

			double value = 0.0;
			InvertableParameterRange range;
			mutable bool dirty = false;
		};

		struct pma_base
		{
			virtual ~pma_base() {};

            
            
			template <int P> void setParameter(double v)
			{
				if constexpr (P == 0)
					value = v;
				if constexpr (P == 1)
					mulValue = v;
				if constexpr (P == 2)
					addValue = v;

				dirty = true;
			}

			template <typename NodeType> static void createParameters(ParameterDataList& data, NodeType& n)
			{
				{
					parameter::data p("Value");
					p.template setParameterCallbackWithIndex<NodeType, 0>(&n);
					p.setRange({ 0.0, 1.0 });
					p.setDefaultValue(0.0);
					data.add(std::move(p));
				}
				{
					parameter::data p("Multiply");
					p.template setParameterCallbackWithIndex<NodeType, 1>(&n);
					p.setRange({ -1.0, 1.0 });
					p.setDefaultValue(1.0);
					data.add(std::move(p));
				}
				{
					parameter::data p("Add");
					p.template setParameterCallbackWithIndex<NodeType, 2>(&n);
					p.setRange({ -1.0, 1.0 });
					p.setDefaultValue(0.0);
					data.add(std::move(p));
				}
			}

			double value = 0.0;
			double mulValue = 1.0;
			double addValue = 0.0;
			mutable bool dirty = false;
		};

		struct pma_unscaled: public pma_base,
							 public pimpl::no_mod_normalisation
		{
			SN_NODE_ID("pma_unscaled");
			SN_DESCRIPTION("multiplies and adds an offset to an unscaled modulation signal");

			static constexpr bool isNormalisedModulation() { return false; }
            static constexpr bool needsProcessing() { return false; }

			pma_unscaled() :
				no_mod_normalisation(getStaticId(), { "Value", "Add" })
			{};

			double getValue() const
			{
				dirty = false;
				return value * mulValue + addValue;
			}
		};

		struct pma: public pma_base
		{
			SN_NODE_ID("pma");
			SN_DESCRIPTION("Scales and offsets a modulation signal");

			static constexpr bool isNormalisedModulation() { return true; }
            static constexpr bool needsProcessing() { return false; }

			double getValue() const
			{
				dirty = false;
				return jlimit(0.0, 1.0, value * mulValue + addValue);
			}
		};
	}

	

	template <int NV, class ParameterType, typename DataType> 
		struct multi_parameter : public mothernode,
								 public polyphonic_base,
								 public pimpl::combined_parameter_base<DataType>,
								 public pimpl::parameter_node_base<ParameterType>,
								 public pimpl::no_processing
	{
		static constexpr int NumVoices = NV;

		SN_POLY_NODE_ID(DataType::getStaticId());
		SN_GET_SELF_AS_OBJECT(multi_parameter);
		SN_DESCRIPTION(DataType::getDescription());
		
		multi_parameter() :
			polyphonic_base(getStaticId(), false),
			control::pimpl::parameter_node_base<ParameterType>(getStaticId()) 
		{};

		static constexpr bool isNormalisedModulation() { return DataType::isNormalisedModulation(); }

		template <int P> static void setParameterStatic(void* obj, double v)
		{
			auto typed = static_cast<multi_parameter<NumVoices, ParameterType, DataType>*>(obj);

			for (auto& s : typed->data)
				s.template setParameter<P>(v);

			typed->sendPending();
		}
        SN_PARAMETER_MEMBER_FUNCTION;

		void sendPending()
		{
			if constexpr (isPolyphonic())
			{
				if (polyHandler == nullptr || 
					!this->getParameter().isConnected() ||
					polyHandler->getVoiceIndex() == -1)
					return;

				auto& d = data.get();

				if(d.dirty)
					this->getParameter().call(d.getValue());
			}
			else
			{
				auto& d = data.get();

				if (d.dirty && this->getParameter().isConnected())
				{
					auto v = d.getValue();
					this->getParameter().call(v);
				}
			}
		}

		template <typename T> void process(T& pd)
		{
            if constexpr (DataType::needsProcessing())
                data.get().process(pd);
            
			sendPending();
		}
        
		void reset()
		{
			if constexpr (prototypes::check::reset<DataType>::value)
			{
				for (auto& d : data)
					d.reset();
			}
		}

		template <typename T> void processFrame(T& fd)
		{
            if constexpr (DataType::needsProcessing())
                data.get().processFrame(fd);
            
			sendPending();
		}

		void prepare(PrepareSpecs ps)
		{
			this->data.prepare(ps);
			polyHandler = ps.voiceIndex;
		}

		void createParameters(ParameterDataList& data)
		{
			DataType::createParameters(data, *this);
		}

		DataType getUIData() const override {
			return data.getFirst();
		}

	private:

		snex::Types::PolyHandler* polyHandler;
		PolyData<DataType, NumVoices> data;
	};

	template <int NV, typename ParameterType> using pma = multi_parameter<NV, ParameterType, multilogic::pma>;
	template <int NV, typename ParameterType> using pma_unscaled = multi_parameter<NV, ParameterType, multilogic::pma_unscaled>;
	template <int NV, typename ParameterType> using bipolar = multi_parameter<NV, ParameterType, multilogic::bipolar>;
	template <int NV, typename ParameterType> using minmax = multi_parameter<NV, ParameterType, multilogic::minmax>;
	template <int NV, typename ParameterType> using logic_op = multi_parameter<NV, ParameterType, multilogic::logic_op>;
	template <int NV, typename ParameterType> using intensity = multi_parameter<NV, ParameterType, multilogic::intensity>;
	template <int NV, typename ParameterType> using bang = multi_parameter<NV, ParameterType, multilogic::bang>;
    template <int NV, typename ParameterType> using delay_cable = multi_parameter<NV, ParameterType, multilogic::delay_cable>;
	template <int NV, typename ParameterType> using change = multi_parameter<NV, ParameterType, multilogic::change>;
	template <int NV, typename ParameterType> using blend = multi_parameter<NV, ParameterType, multilogic::blend>;

	struct smoothed_parameter_base: public mothernode
	{
		virtual ~smoothed_parameter_base() {};
		virtual smoothers::base* getSmootherObject() = 0;
	};

	template <int NV, typename SmootherClass, bool IsScaled> 
	struct smoothed_parameter_pimpl: public control::pimpl::templated_mode,
                                     public polyphonic_base,
									 public smoothed_parameter_base
	{
		static constexpr int NumVoices = NV;

		enum Parameters
		{
			Value,
			SmoothingTime,
			Enabled
		};

		smoothed_parameter_pimpl():
          polyphonic_base(getStaticId(), false),
          templated_mode(getStaticId(), "smoothers")
		{
			if (!IsScaled)
			{
				cppgen::CustomNodeProperties::addNodeIdManually(getStaticId(), PropertyIds::UseUnnormalisedModulation);
				cppgen::CustomNodeProperties::addUnscaledParameter(getStaticId(), "Value");
			}

            cppgen::CustomNodeProperties::setPropertyForObject(*this, PropertyIds::TemplateArgumentIsPolyphonic);
            
			static_assert(std::is_base_of<smoothers::base, SmootherClass>(), "Not a smoother class");
			static_assert(SmootherClass::NumVoices == NumVoices, "Voice amount mismatch");
		}

		static Identifier getStaticId() { return IsScaled ? "smoothed_parameter" : "smoothed_parameter_unscaled"; };

		SN_GET_SELF_AS_OBJECT(smoothed_parameter_pimpl);
		SN_DESCRIPTION("Smoothes an incoming modulation signal");

		DEFINE_PARAMETERS
		{
			DEF_PARAMETER(Value, smoothed_parameter_pimpl);
			DEF_PARAMETER(SmoothingTime, smoothed_parameter_pimpl);
			DEF_PARAMETER(Enabled, smoothed_parameter_pimpl);
		}
		SN_PARAMETER_MEMBER_FUNCTION;


		void initialise(NodeBase* n)
		{
			value.initialise(n);
		}

		SN_EMPTY_HANDLE_EVENT;

		static constexpr bool isNormalisedModulation() { return IsScaled; };

		static constexpr bool isPolyphonic() { return NumVoices > 1; };

		template <typename ProcessDataType> void process(ProcessDataType& d)
		{
			modValue.setModValueIfChanged(value.advance());
		}

		bool handleModulation(double& v)
		{
			return modValue.getChangedValue(v);
		}

		template <typename FrameDataType> void processFrame(FrameDataType& d)
		{
			modValue.setModValueIfChanged(value.advance());
		}

		void reset()
		{
			value.reset();

			// Force the change flag to true in order to trigger the modulation for polyphonic
			// contexts at voice start
			modValue.setModValue(value.get());
			//modValue.setModValueIfChanged(value.get());
		}

		void prepare(PrepareSpecs ps)
		{
			value.prepare(ps);
		}

		void setValue(double newValue)
		{
			value.set(newValue);
		}

		void createParameters(ParameterDataList& data)
		{
			{
				DEFINE_PARAMETERDATA(smoothed_parameter_pimpl, Value);
				p.setRange({ 0.0, 1.0 });
				data.add(std::move(p));
			}
			{
				DEFINE_PARAMETERDATA(smoothed_parameter_pimpl, SmoothingTime);
				p.setRange({ 0.1, 1000.0, 0.1 });
				p.setDefaultValue(100.0);
				data.add(std::move(p));
			}
			{
				DEFINE_PARAMETERDATA(smoothed_parameter_pimpl, Enabled);
				p.setRange({ 0.0, 1.0, 1.0 });
				p.setDefaultValue(1.0);
				data.add(std::move(p));
			}
		}

		void setEnabled(double v)
		{
			value.setEnabled(v);
		}

		void setSmoothingTime(double newSmoothingTime)
		{
			value.setSmoothingTime(newSmoothingTime);
		}

		smoothers::base* getSmootherObject() override { return &value; }

		SmootherClass value;

	private:

		ModValue modValue;
	};

	template <int NV, typename SmootherClass> using smoothed_parameter = smoothed_parameter_pimpl<NV, SmootherClass, true>;
	template <int NV, typename SmootherClass> using smoothed_parameter_unscaled = smoothed_parameter_pimpl<NV, SmootherClass, false>;
}

}
