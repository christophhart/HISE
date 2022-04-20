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
		SET_HISE_NODE_ID("cable_pack");
		SN_GET_SELF_AS_OBJECT(cable_pack);
		HISE_ADD_SET_VALUE(cable_pack);
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
		SET_HISE_NODE_ID("sliderbank");
		SN_GET_SELF_AS_OBJECT(sliderbank);
		HISE_ADD_SET_VALUE(sliderbank);
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
					this->getParameter().template getParameter<P>().call(v * b[P]);
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
						  public pimpl::parameter_node_base<ParameterClass>
	{
		SET_HISE_NODE_ID("file_analyser");
		SN_GET_SELF_AS_OBJECT(file_analyser);
		SN_TEMPLATED_MODE_PARAMETER_NODE_CONSTRUCTOR(file_analyser, ParameterClass, "file_analysers");
		SN_DESCRIPTION("Extracts file information (pitch, length, etc) and sends it as modulation signal on file load");

		HISE_EMPTY_CREATE_PARAM;

		static constexpr bool isNormalisedModulation() { return false; }

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
															 public pimpl::no_processing
	{
		SET_HISE_NODE_ID("input_toggle");
		SN_GET_SELF_AS_OBJECT(input_toggle);
		SN_PARAMETER_NODE_CONSTRUCTOR(input_toggle, ParameterClass);
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
		PARAMETER_MEMBER_FUNCTION;

		static constexpr bool isNormalisedModulation() { return false; };

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

	class tempo_sync : public mothernode,
					   public hise::TempoListener
	{
	public:

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
		PARAMETER_MEMBER_FUNCTION;

		bool isPolyphonic() const { return false; }

		SET_HISE_NODE_ID("tempo_sync");
		SN_GET_SELF_AS_OBJECT(tempo_sync);
		SN_DESCRIPTION("Sends the tempo duration as modulation signal");

		void prepare(PrepareSpecs ps);

		HISE_EMPTY_INITIALISE;
		HISE_EMPTY_RESET;
		HISE_EMPTY_PROCESS;
		HISE_EMPTY_PROCESS_SINGLE;
		HISE_EMPTY_HANDLE_EVENT;

		tempo_sync();
		~tempo_sync();

		void createParameters(ParameterDataList& data);
		void tempoChanged(double newTempo) override;
		void setTempo(double newTempoIndex);
		bool handleModulation(double& max);

		static constexpr bool isNormalisedModulation() { return false; }

		void setMultiplier(double newMultiplier);

		void setEnabled(double v);
		void setUnsyncedTime(double unsyncedTime);

		static void tempoChangedStatic(void* obj, double newBpm)
		{
			auto s = static_cast<tempo_sync*>(obj);
			s->tempoChanged(newBpm);
		}

		void refresh();

		double currentTempoMilliseconds = 500.0;
		double lastTempoMs = 0.0;
		double bpm = 120.0;

		bool enabled = false;
		double unsyncedTime = false;

		double multiplier = 1.0;
		TempoSyncer::Tempo currentTempo = TempoSyncer::Tempo::Eighth;


		DllBoundaryTempoSyncer* tempoSyncer = nullptr;

		JUCE_DECLARE_WEAK_REFERENCEABLE(tempo_sync);
	};

	template <typename ParameterClass> struct resetter : public mothernode,
														 public pimpl::no_processing,
														 public pimpl::parameter_node_base<ParameterClass>
	{
		SET_HISE_NODE_ID("resetter");
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
				p.callback = parameter::inner<resetter, 0>(*this);
				data.add(std::move(p));
			}
		}

		int flashCounter = 0;

		FORWARD_PARAMETER_TO_MEMBER(resetter);

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
			Aftertouch = 128,
			Pitchbend = 129,
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
		PARAMETER_MEMBER_FUNCTION;

		SET_HISE_NODE_ID("midi_cc");
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
																					 public pimpl::no_processing
	{
		SET_HISE_NODE_ID("cable_expr");
		SN_GET_SELF_AS_OBJECT(cable_expr);
		SN_DESCRIPTION("evaluates an expression for the control value");

		HISE_ADD_SET_VALUE(cable_expr);
		SN_PARAMETER_NODE_CONSTRUCTOR(cable_expr, ParameterClass);

		HISE_DEFAULT_INIT(ExpressionClass);

		void setValue(double input)
		{
			auto v = obj.op(input);

			if (this->getParameter().isConnected())
				this->getParameter().call(v);
		}

		ExpressionClass obj;
		double lastValue = 0.0;
	};

    template <typename ConverterClass, typename ParameterClass>
        struct converter : public mothernode,
                           public pimpl::parameter_node_base<ParameterClass>,
                           public pimpl::no_processing
    {
        SET_HISE_NODE_ID("converter");
        SN_GET_SELF_AS_OBJECT(converter);
        SN_DESCRIPTION("converts a control value");

        HISE_DEFAULT_INIT(ConverterClass);
        HISE_DEFAULT_PREPARE(ConverterClass);
        HISE_ADD_SET_VALUE(converter);
        SN_PARAMETER_NODE_CONSTRUCTOR(converter, ParameterClass);

        static constexpr bool isNormalisedModulation() { return false; }
        
        void setValue(double input)
        {
            auto v = obj.getValue(input);

            if (this->getParameter().isConnected())
                this->getParameter().call(v);
        }

        ConverterClass obj;
    };

	template <typename ParameterClass> struct cable_table : public scriptnode::data::base,
															public pimpl::parameter_node_base<ParameterClass>,
															public pimpl::no_processing
	{
		SET_HISE_NODE_ID("cable_table");
		SN_GET_SELF_AS_OBJECT(cable_table);
		SN_DESCRIPTION("Modify a modulation signal using a lookup table");

		HISE_ADD_SET_VALUE(cable_table);
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
		SET_HISE_NODE_ID("clone_pack");
		
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
		PARAMETER_MEMBER_FUNCTION;

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

				if (auto sp = dynamic_cast<SliderPackData*>(this->externalData.obj))
				{
					auto v = sp->getValue(changedIndex) * lastValue;
					this->p.callEachClone(changedIndex, v);
				}
			}
		}

		void setExternalData(const snex::ExternalData& d, int index) override
		{
			if (auto existing = this->externalData.obj)
				existing->getUpdater().addEventListener(this);

			base::setExternalData(d, index);

			if (auto existing = this->externalData.obj)
				existing->getUpdater().addEventListener(this);

			this->externalData.referBlockTo(sliderData, 0);

			setSliderAmountToNumClones();
		}

		void setValue(double newValue)
		{
			lastValue = newValue;
			jassert(numClones == sliderData.size());

			for (int i = 0; i < numClones; i++)
			{
				auto valueToSend = sliderData[i] * lastValue;
				this->getParameter().callEachClone(i, valueToSend);
			}
		}

        void numClonesChanged(int newSize) override
        {
            setNumClones(newSize);
        }
        
		void setSliderAmountToNumClones()
		{
			if (auto sp = dynamic_cast<SliderPackData*>(this->externalData.obj))
				sp->setNumSliders(numClones);

			setValue(lastValue);
		}

		void setNumClones(double newNumClones)
		{
			if (numClones != newNumClones)
			{
				numClones = jlimit(1, 128, (int)newNumClones);
				setSliderAmountToNumClones();
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
		SET_HISE_NODE_ID("clone_cable");
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
		PARAMETER_MEMBER_FUNCTION;

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
			setNumClones(newNumClones);
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
				this->getParameter().callEachClone(i, valueToSend);
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

	
	template <typename ParameterClass, typename FaderClass> struct xfader: public pimpl::parameter_node_base<ParameterClass>,
																		   public control::pimpl::templated_mode,
																		   public pimpl::no_processing
	{
		SET_HISE_NODE_ID("xfader");
		SN_GET_SELF_AS_OBJECT(xfader);
		SN_TEMPLATED_MODE_PARAMETER_NODE_CONSTRUCTOR(xfader, ParameterClass, "faders");
		SN_DESCRIPTION("Apply a crossfade to multiple outputs");

		HISE_ADD_SET_VALUE(xfader);

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
		struct bipolar
		{
			SET_HISE_NODE_ID("bipolar");
			SN_DESCRIPTION("Creates a bipolar mod signal from a 0...1 range");

			static constexpr bool isNormalisedModulation() { return true; }

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
					p.callback = parameter::inner<NodeType, 0>(n);
					p.setRange({ 0.0, 1.0 });
					p.setDefaultValue(0.0);
					data.add(std::move(p));
				}
				{
					parameter::data p("Scale");
					p.callback = parameter::inner<NodeType, 1>(n);
					p.setRange({ -1.0, 1.0 });
					p.setDefaultValue(0.0);
					data.add(std::move(p));
				}
				{
					parameter::data p("Gamma");
					p.callback = parameter::inner<NodeType, 2>(n);
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

		struct logic_op
		{
			enum class LogicType
			{
				AND,
				OR,
				XOR,
				numLogicTypes
			};

			SET_HISE_NODE_ID("logic_op");
			SN_DESCRIPTION("Combines the (binary) input signals using a logic operator");

			static constexpr bool isNormalisedModulation() { return true; }

			bool operator==(const logic_op& other) const
			{
				return logicType == other.logicType && leftValue == other.leftValue && rightValue == other.rightValue;
			}

			double getValue() const
			{
				dirty = false;

				switch (logicType)
				{
				case LogicType::AND: return leftValue && rightValue ? 1.0 : 0.0;
				case LogicType::OR: return leftValue || rightValue ? 1.0 : 0.0;
				case LogicType::XOR: return (leftValue || rightValue) && !(leftValue == rightValue) ? 1.0 : 0.0;
                default: return 0.0;
				}
                
                return 0.0;
			}

			template <int P> void setParameter(double v)
			{
				if constexpr (P == 0)
					leftValue = v > 0.5;
				if constexpr (P == 1)
					rightValue = v > 0.5;
				if constexpr (P == 2)
					logicType = (LogicType)jlimit(0, 2, (int)v);

				dirty = true;
			}

			template <typename NodeType> static void createParameters(ParameterDataList& data, NodeType& n)
			{
				{
					parameter::data p("Left");
					p.callback = parameter::inner<NodeType, 0>(n);
					p.setRange({ 0.0, 1.0 });
					p.setDefaultValue(0.0);
					data.add(std::move(p));
				}
				{
					parameter::data p("Right");
					p.callback = parameter::inner<NodeType, 1>(n);
					p.setRange({ 0.0, 1.0 });
					p.setDefaultValue(0.0);
					data.add(std::move(p));
				}
				{
					parameter::data p("Operator");
					p.callback = parameter::inner<NodeType, 2>(n);
					p.setParameterValueNames({ "AND", "OR", "XOR" });
					p.setDefaultValue(0.0);
					data.add(std::move(p));
				}
			}

			bool leftValue = false;
			bool rightValue = false;
			LogicType logicType = LogicType::AND;
			mutable bool dirty = false;

		};

		struct minmax
		{
			SET_HISE_NODE_ID("minmax");
			SN_DESCRIPTION("Scales the input value to a modifyable range");

			static constexpr bool isNormalisedModulation() { return false; }

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
					p.callback = parameter::inner<NodeType, 0>(n);
					p.setRange({ 0.0, 1.0 });
					p.setDefaultValue(0.0);
					data.add(std::move(p));
				}
				{
					parameter::data p("Minimum");
					p.callback = parameter::inner<NodeType, 1>(n);
					p.setRange({ 0.0, 1.0 });
					p.setDefaultValue(0.0);
					data.add(std::move(p));
				}
				{
					parameter::data p("Maximum");
					p.callback = parameter::inner<NodeType, 2>(n);
					p.setRange({ 0.0, 1.0 });
					p.setDefaultValue(1.0);
					data.add(std::move(p));
				}
				{
					parameter::data p("Skew");
					p.callback = parameter::inner<NodeType, 3>(n);
					p.setRange({ 0.1, 10.0 });
					p.setSkewForCentre(1.0);
					p.setDefaultValue(1.0);
					data.add(std::move(p));
				}
				{
					parameter::data p("Step");
					p.callback = parameter::inner<NodeType, 4>(n);
					p.setRange({ 0.0, 1.0 });
					p.setDefaultValue(0.0);
					data.add(std::move(p));
				}
                {
                    parameter::data p("Polarity");
                    p.callback = parameter::inner<NodeType, 5>(n);
                    p.setRange({ 0.0, 1.0, 1.0 });
                    p.setParameterValueNames({"Normal", "Inverted"});
                    p.setDefaultValue(0.0);
                    data.add(std::move(p));
                }
			}

			double value = 0.0;
			InvertableParameterRange range;
			mutable bool dirty = false;
		};

		struct pma
		{
			SET_HISE_NODE_ID("pma");
			SN_DESCRIPTION("Scales and offsets a modulation signal");

			static constexpr bool isNormalisedModulation() { return true; }

			double getValue() const 
			{ 
				dirty = false; 
				return jlimit(0.0, 1.0, value * mulValue + addValue); 
			}

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
					p.callback = parameter::inner<NodeType, 0>(n);
					p.setRange({ 0.0, 1.0 });
					p.setDefaultValue(0.0);
					data.add(std::move(p));
				}
				{
					parameter::data p("Multiply");
					p.callback = parameter::inner<NodeType, 1>(n);
					p.setRange({ -1.0, 1.0 });
					p.setDefaultValue(1.0);
					data.add(std::move(p));
				}
				{
					parameter::data p("Add");
					p.callback = parameter::inner<NodeType, 2>(n);
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
	}

	

	template <int NV, class ParameterType, typename DataType> 
		struct multi_parameter : public mothernode,
								 public polyphonic_base,
								 public pimpl::combined_parameter_base<DataType>,
								 public pimpl::parameter_node_base<ParameterType>,
								 public pimpl::no_processing
	{
		static constexpr int NumVoices = NV;

		SET_HISE_POLY_NODE_ID(DataType::getStaticId());
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
        PARAMETER_MEMBER_FUNCTION;

		void sendPending()
		{
			if constexpr (isPolyphonic())
			{
				if (polyHandler == nullptr || 
					!this->getParameter().isConnected() ||
					polyHandler->getVoiceIndex() == -1)
					return;

				auto& d = data.get();

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

		template <typename T> void process(T&)
		{
			sendPending();
		}

		template <typename T> void processFrame(T&)
		{
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
	template <int NV, typename ParameterType> using bipolar = multi_parameter<NV, ParameterType, multilogic::bipolar>;
	template <int NV, typename ParameterType> using minmax = multi_parameter<NV, ParameterType, multilogic::minmax>;
	template <int NV, typename ParameterType> using logic_op = multi_parameter<NV, ParameterType, multilogic::logic_op>;

	struct smoothed_parameter_base: public mothernode
	{
		virtual ~smoothed_parameter_base() {};
		virtual smoothers::base* getSmootherObject() = 0;
	};

	template <int NV, typename SmootherClass> struct smoothed_parameter: public control::pimpl::templated_mode,
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

		smoothed_parameter():
          polyphonic_base(getStaticId(), false),
          templated_mode(getStaticId(), "smoothers")
		{
            cppgen::CustomNodeProperties::setPropertyForObject(*this, PropertyIds::TemplateArgumentIsPolyphonic);
            
			static_assert(std::is_base_of<smoothers::base, SmootherClass>(), "Not a smoother class");
			static_assert(SmootherClass::NumVoices == NumVoices, "Voice amount mismatch");
		}

		SET_HISE_NODE_ID("smoothed_parameter");
		SN_GET_SELF_AS_OBJECT(smoothed_parameter);
		SN_DESCRIPTION("Smoothes an incoming modulation signal");

		DEFINE_PARAMETERS
		{
			DEF_PARAMETER(Value, smoothed_parameter);
			DEF_PARAMETER(SmoothingTime, smoothed_parameter);
			DEF_PARAMETER(Enabled, smoothed_parameter);
		}
		PARAMETER_MEMBER_FUNCTION;


		void initialise(NodeBase* n)
		{
			value.initialise(n);
		}

		HISE_EMPTY_HANDLE_EVENT;

		static constexpr bool isNormalisedModulation() { return true; };

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
				DEFINE_PARAMETERDATA(smoothed_parameter, Value);
				p.setRange({ 0.0, 1.0 });
				data.add(std::move(p));
			}
			{
				DEFINE_PARAMETERDATA(smoothed_parameter, SmoothingTime);
				p.setRange({ 0.1, 1000.0, 0.1 });
				p.setDefaultValue(100.0);
				data.add(std::move(p));
			}
			{
				DEFINE_PARAMETERDATA(smoothed_parameter, Enabled);
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

}
}
