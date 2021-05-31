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

namespace file_analysers
{
struct pitch
{
	double getValue(const ExternalData& d)
	{
		if (d.numSamples > 0)
		{
			block b;
			d.referBlockTo(b, 0);

			return PitchDetection::detectPitch(b.begin(), b.size(), d.sampleRate);
		}

		return 0.0;
	}
};

struct milliseconds
{
	double getValue(const ExternalData& d)
	{
		if (d.numSamples > 0 && d.sampleRate > 0.0)
		{
			return 1000.0 * (double)d.numSamples / d.sampleRate;
		}

		return 0.0;
	}
};

struct peak
{
	double getValue(const ExternalData& d)
	{
		if (d.numSamples > 0)
		{
			auto b = d.toAudioSampleBuffer();
			return (double)b.getMagnitude(0, d.numSamples);
		}

		return 0.0;
	}
};
}


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

				lastValue = v;

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

	template <typename ParameterType, int NV> struct cloud_mod : 
					   public mothernode,
					   public control::pimpl::no_processing,
					   public control::pimpl::duplicate_parameter_node_base<ParameterType>
	{
		SET_HISE_NODE_ID("cloud_mod");
		SN_GET_SELF_AS_OBJECT(cloud_mod);
		SN_DESCRIPTION("Create a mod signal using a perlin noise cloud");

		SN_PARAMETER_NODE_CONSTRUCTOR(cloud_mod, ParameterType);

		struct Vector
		{
			float x, y, z;
		};

		PolyData<Vector, NV> data;

		void prepare(PrepareSpecs ps)
		{
			data.prepare(ps);
		}

		void setXValue(double x)
		{
			x = jlimit(0.0, 1.0, x);

			for (auto& v : data)
				v.x = (float)x;
			
			refreshValue();
		}

		void setYValue(double y)
		{
			y = jlimit(0.0, 1.0, y);

			for (auto& v : data)
				v.y = (float)y;

			refreshValue();
		}

		void setZValue(double z)
		{
			z = jlimit(0.0, 1.0, z);

			for (auto& v : data)
				v.z = (float)z;

			refreshValue();
		}

		void setOctave(double octave)
		{
			cubeData.setOctave(octave);
		}

		void createParameters(ParameterDataList& list)
		{
			{
				parameter::data d("XValue");
				d.callback = parameter::inner<cloud_mod, 0>(*this);
				list.add(d);
			}
			{
				parameter::data d("YValue");
				d.callback = parameter::inner<cloud_mod, 1>(*this);
				list.add(d);
			}
			{
				parameter::data d("ZValue");
				d.callback = parameter::inner<cloud_mod, 2>(*this);
				list.add(d);
			}
			{
				parameter::data d("XValue");
				d.callback = parameter::inner<cloud_mod, 3>(*this);
				d.setRange({ 1.0, 16.0, 1.0 });
				list.add(d);
			}
		}

		void refreshValue()
		{
			auto& v = data.get();

			auto rValue = cubeData.getInterpolated(v.x, v.y, v.z);

			if (this->getParameter().isConnected())
				this->getParameter().call(rValue);
		}

		template <int P> void setParameter(double v)
		{
			if (P == 0) setXValue(v);
			if (P == 1) setYValue(v);
			if (P == 2) setZValue(v);
			if (P == 3) setOctave(v);
		}

		FORWARD_PARAMETER_TO_MEMBER(cloud_mod);

		hise::PerlinCube<64> cubeData;
	};

	template <typename ParameterType> struct dupli_pack: public data::base,
														 public control::pimpl::no_processing,
														 public control::pimpl::duplicate_parameter_node_base<ParameterType>,
														 public hise::ComplexDataUIUpdaterBase::EventListener
	{
		SN_GET_SELF_AS_OBJECT(dupli_pack);
		SET_HISE_NODE_ID("dupli_pack");
		HISE_ADD_SET_VALUE(dupli_pack);		
		SN_DESCRIPTION("Scale unisono values using a slider pack");
		
        dupli_pack() : pimpl::duplicate_parameter_node_base<ParameterType>(getStaticId()) {};

		void onComplexDataEvent(hise::ComplexDataUIUpdaterBase::EventType t, var data) override
		{
			if (t == ComplexDataUIUpdaterBase::EventType::ContentChange)
			{
				auto changedIndex = (int)data;

				if (auto sp = dynamic_cast<SliderPackData*>(this->externalData.obj))
				{
					auto v = sp->getValue(changedIndex) * lastValue;
					this->getParameter().call(changedIndex, v);
				}
			}

			if (t == ComplexDataUIUpdaterBase::EventType::ContentRedirected)
			{
				if (auto sp = dynamic_cast<SliderPackData*>(this->externalData.obj))
				{
					if (this->p.getNumVoices() != sp->getNumSliders())
						jassertfalse; // resize here?
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

			setValue(lastValue);
		}

		void numVoicesChanged(int newNumVoices) override
		{
			setValue(lastValue);

			if (auto sp = dynamic_cast<SliderPackData*>(this->externalData.obj))
			{
				sp->setNumSliders(newNumVoices);
			}
		}

		double lastValue = 0.0;
		block sliderData;

		void setValue(double newValue)
		{
			lastValue = newValue;

			int numVoices = this->p.getNumVoices();

			if (numVoices == sliderData.size())
			{
				for (int i = 0; i < numVoices; i++)
				{
					auto valueToSend = sliderData[i] * lastValue;
					this->getParameter().call(i, valueToSend);
				}
			}
		}
	};
		

	template <typename ParameterType, typename LogicType> struct dupli_cable : public control::pimpl::no_processing,
																			   public control::pimpl::templated_mode,
																			   public control::pimpl::duplicate_parameter_node_base<ParameterType>
	{
		SN_GET_SELF_AS_OBJECT(dupli_cable);
		SET_HISE_NODE_ID("dupli_cable");
		SN_DESCRIPTION("Send different values to unisono nodes");

		dupli_cable():
            control::pimpl::duplicate_parameter_node_base<ParameterType>(getStaticId()),
            control::pimpl::templated_mode(getStaticId(), "duplilogic")
		{};

		enum class Parameters
		{
			Value,
			Gamma,
		};

		DEFINE_PARAMETERS
		{
			DEF_PARAMETER(Value, dupli_cable);
			DEF_PARAMETER(Gamma, dupli_cable);
		};
		PARAMETER_MEMBER_FUNCTION;


		void initialise(NodeBase* n)
		{
			if constexpr (prototypes::check::initialise<LogicType>::value)
				obj.initialise(n);
		}

		void numVoicesChanged(int newNumVoices) override
		{
			sendValue();
		}

		void setValue(double v)
		{
 			lastValue = v;
			sendValue();
		}

		void setGamma(double gamma)
		{
			lastGamma = jlimit(0.0, 1.0, gamma);
			sendValue();
		}

		void sendValue()
		{
			int numVoices = this->p.getNumVoices();

			for (int i = 0; i < numVoices; i++)
			{
				auto valueToSend = obj.getValue(i, numVoices, lastValue, lastGamma);
				this->getParameter().call(i, valueToSend);
			}
		}

		void createParameters(ParameterDataList& data)
		{
			{
				DEFINE_PARAMETERDATA(dupli_cable, Value);
				p.setRange({ 0.0, 1.0 });
				p.setDefaultValue(0.0);
				data.add(std::move(p));
			}
			{
				DEFINE_PARAMETERDATA(dupli_cable, Gamma);
				p.setRange({ 0.0, 1.0 });
				p.setDefaultValue(0.0);
				data.add(std::move(p));
			}
		}

		double lastValue = 0.0;
		double lastGamma = 0.0;
		LogicType obj;

		JUCE_DECLARE_WEAK_REFERENCEABLE(dupli_cable);
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

	

	template <int NV, class ParameterType> struct pma : public mothernode,
														      public polyphonic_base,
															  public pimpl::combined_parameter_base,
															  public pimpl::parameter_node_base<ParameterType>,
															  public pimpl::no_processing
	{
		static constexpr int NumVoices = NV;

		SET_HISE_POLY_NODE_ID("pma");
		SN_GET_SELF_AS_OBJECT(pma);
		SN_DESCRIPTION("Scales and offsets a modulation signal");
		
		pma() : 
			polyphonic_base(getStaticId(), false),
			control::pimpl::parameter_node_base<ParameterType>(getStaticId()) 
		{};

		enum class Parameters
		{
			Value,
			Multiply,
			Add
		};

		DEFINE_PARAMETERS
		{
			DEF_PARAMETER(Value, pma);
			DEF_PARAMETER(Multiply, pma);
			DEF_PARAMETER(Add, pma);
		};
		PARAMETER_MEMBER_FUNCTION;

		void setValue(double v)
		{
			for (auto& s : this->data)
			{
				s.value = v;
				s.dirty = true;
			}

			sendPending();
		}

		void setAdd(double v)
		{
			for (auto& s : this->data)
			{
				s.addValue = v;
				s.dirty = true;
			}
				
			sendPending();
		}

		void setMultiply(double v)
		{
			for (auto& s : this->data)
			{
				s.mulValue = v;
				s.dirty = true;
			}

			sendPending();
		}
		

		void sendPending()
		{
			if constexpr (isPolyphonic())
			{
				if (polyHandler == nullptr || 
					!this->getParameter().isConnected() ||
					polyHandler->getVoiceIndex() == -1)
					return;

				auto& d = data.get();

				this->getParameter().call(d.getPmaValue());
			}
			else
			{
				auto& d = data.get();

				if(d.dirty && this->getParameter().isConnected())
					this->getParameter().call(d.getPmaValue());
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
			{
				DEFINE_PARAMETERDATA(pma, Value);
				p.setRange({ 0.0, 1.0 });
				p.setDefaultValue(0.0);
				data.add(std::move(p));
			}
			{
				DEFINE_PARAMETERDATA(pma, Multiply);
				p.setRange({ -1.0, 1.0 });
				p.setDefaultValue(1.0);
				data.add(std::move(p));
			}
			{
				DEFINE_PARAMETERDATA(pma, Add);
				p.setRange({ -1.0, 1.0 });
				p.setDefaultValue(0.0);
				data.add(std::move(p));
			}
		}

		Data getUIData() const override {
			return data.getFirst();
		}

	private:

		snex::Types::PolyHandler* polyHandler;
		PolyData<Data, NumVoices> data;
	};

	template <typename SmootherClass> struct smoothed_parameter: public control::pimpl::templated_mode
	{
		enum Parameters
		{
			Value,
			SmoothingTime
		};

		smoothed_parameter():
			templated_mode(getStaticId(), "smoothers")
		{
		}

		SET_HISE_NODE_ID("smoothed_parameter");
		SN_GET_SELF_AS_OBJECT(smoothed_parameter);
		SN_DESCRIPTION("Smoothes an incoming modulation signal");

		DEFINE_PARAMETERS
		{
			DEF_PARAMETER(Value, smoothed_parameter);
			DEF_PARAMETER(SmoothingTime, smoothed_parameter);
		}
		PARAMETER_MEMBER_FUNCTION;


		void initialise(NodeBase* n)
		{
			value.initialise(n);
		}

		HISE_EMPTY_HANDLE_EVENT;

		static constexpr bool isNormalisedModulation() { return true; };

		bool isPolyphonic() const { return false; };

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
			modValue.setModValueIfChanged(value.get());
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
		}

		void setSmoothingTime(double newSmoothingTime)
		{
			value.setSmoothingTime(newSmoothingTime);
		}

		SmootherClass value;

	private:

		ModValue modValue;
	};

}
}
