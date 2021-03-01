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
				getParameter().call(v);
				lastValue = v;

				externalData.setDisplayedValue((double)index.getIndex(b.size()));
			}
		}
		
		using IndexType = index::normalised<double, index::clamped<0>>;

		double lastValue = 0.0;
	};

	template <typename ParameterClass> struct sliderbank : public data::base,
														   public pimpl::parameter_node_base<ParameterClass>,
														   public pimpl::no_processing
	{
		SET_HISE_NODE_ID("sliderbank");
		SN_GET_SELF_AS_OBJECT(sliderbank);

		HISE_ADD_SET_VALUE(sliderbank);

		void initialise(NodeBase* n)
		{
			p.initialise(n);
		}

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

		template <int P> void callSlider(double v)
		{
			if (P < b.size() && P < getParameter().getNumParameters())
				getParameter().getParameter<P>().call(v * b[P]);
		}

		void setValue(double v)
		{
			lastValue = v;

			DataReadLock l(this);

			callSlider<0>(v);
			callSlider<1>(v);
			callSlider<2>(v);
			callSlider<3>(v);
			callSlider<4>(v);
			callSlider<5>(v);
			callSlider<6>(v);
			callSlider<7>(v);
		}

	private:

		double lastValue = 0.0;
	};

	template <typename ParameterClass> struct cable_table : public scriptnode::data::base,
															public pimpl::parameter_node_base<ParameterClass>,
															public pimpl::no_processing
	{
		SET_HISE_NODE_ID("cable_table");
		SN_GET_SELF_AS_OBJECT(cable_table);

		HISE_ADD_SET_VALUE(cable_table);

		void setExternalData(const ExternalData& d, int index) override
		{
			base::setExternalData(d, index);
			this->externalData.referBlockTo(tableData, 0);
		}

		void setValue(double input)
		{
			if (!tableData.isEmpty())
			{
				InterpolatorType ip(input);

				auto tv = tableData.interpolate(ip);

				getParameter().call(tv);

				this->externalData.setDisplayedValue(input);
			}
		}

		using TableClampType = index::clamped<SAMPLE_LOOKUP_TABLE_SIZE>;
		using InterpolatorType = index::lerp<index::normalised<double, TableClampType>>;

		block tableData;
	};

	
	template <typename ParameterClass, typename FaderClass> struct xfader: public pimpl::parameter_node_base<ParameterClass>,
																		   public pimpl::no_processing
	{
		SET_HISE_NODE_ID("xfader");
		SN_GET_SELF_AS_OBJECT(xfader);

		HISE_ADD_SET_VALUE(xfader);

		void initialise(NodeBase* n) override
		{
			p.initialise(n);
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
			if (P < p.getNumParameters())
				p.call<P>(fader.getFadeValue<P>(p.getNumParameters(), v));
		}

		ModValue lastValue;

		ParameterClass p;
		FaderClass fader;

		JUCE_DECLARE_WEAK_REFERENCEABLE(xfader);
	};


	template <class ParameterType, int NumVoices=1> struct pma : public pimpl::combined_parameter_base,
																 public pimpl::parameter_node_base<ParameterType>,
																 public pimpl::no_processing
	{
		SET_HISE_NODE_ID("pma");
		SN_GET_SELF_AS_OBJECT(pma);

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
				sendParameterChange(s);
			}
		}

		void setAdd(double v)
		{
			for (auto& s : this->data)
			{
				s.addValue = v;
				sendParameterChange(s);
			}
		}

		void prepare(PrepareSpecs ps)
		{
			this->data.prepare(ps);
		}

		void setMultiply(double v)
		{
			for (auto& s : this->data)
			{
				s.mulValue = v;
				sendParameterChange(s);
			}
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
				p.setDefaultValue(1.0);
				data.add(std::move(p));
			}
			{
				DEFINE_PARAMETERDATA(pma, Add);
				p.setDefaultValue(0.0);
				data.add(std::move(p));
			}
		}

		Data getUIData() const override {
			return data.getFirst();
		}

	private:

		PolyData<Data, NumVoices> data;

		void sendParameterChange(control::pimpl::combined_parameter_base::Data& d)
		{
			getParameter().call(d.getPmaValue());
		}
	};

	template <typename SmootherClass> struct smoothed_parameter
	{
		enum Parameters
		{
			Value,
			SmoothingTime
		};

		SET_HISE_NODE_ID("smoothed_parameter");
		SN_GET_SELF_AS_OBJECT(smoothed_parameter);

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