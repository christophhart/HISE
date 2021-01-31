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

struct combined_parameter_base
{
	struct Data
	{
		double getPmaValue() const { return value * mulValue + addValue; }
		double getPamValue() const { return (value + addValue) * mulValue; }

		double value = 0.0;
		double mulValue = 1.0;
		double addValue = 0.0;
	};

	virtual Data getUIData() const = 0;

	NormalisableRange<double> currentRange;

	JUCE_DECLARE_WEAK_REFERENCEABLE(combined_parameter_base);
};

template <int NumVoices> struct combined_parameter : public combined_parameter_base
{
	Data getUIData() const override {
		return data.getFirst();
	}

	PolyData<Data, NumVoices> data;
};

namespace core
{

	template <class ParameterType> struct parameter_node_base
	{
		/** This method can be used to connect a target to the combined output of this
			node.
		*/
		template <int I, class T> void connect(T& t)
		{
			this->p.template getParameter<0>().template connect<I>(t);
		}

		auto& getParameter()
		{
			return p;
		}

		ParameterType p;
	};

	template <class ParameterType> struct cable_table : public parameter_node_base<ParameterType>,
		public scriptnode::data::base
	{
		SET_HISE_NODE_ID("cable_table");
		SN_GET_SELF_AS_OBJECT(cable_table);

		enum class Parameters
		{
			Value,
		};

		DEFINE_PARAMETERS
		{
			DEF_PARAMETER(Value, cable_table);
		};
		PARAMETER_MEMBER_FUNCTION;

		HISE_EMPTY_INITIALISE;
		HISE_EMPTY_RESET;
		HISE_EMPTY_PROCESS;
		HISE_EMPTY_PREPARE;
		HISE_EMPTY_PROCESS_SINGLE;
		HISE_EMPTY_HANDLE_EVENT;
		HISE_EMPTY_MOD;

		void setExternalData(const ExternalData& d, int index)
		{
			base::setExternalData(d, index);
			this->externalData.referBlockTo(tableData, 0);
		}

		ExternalData* getExternalData() { return &data; }

		void setValue(double input)
		{
			if (!tableData.isEmpty())
			{
				auto index = (float)input * (float)tableData.size();

				jassert(isPowerOfTwo(tableData.size()));

				int i1 = (int)index & (tableData.size() - 1);
				int i2 = (i1 + 1) & (tableData.size() - 1);

				auto alpha = hmath::fmod(index, 1.0f);
				auto v1 = tableData[i1];
				auto v2 = tableData[i2];

				auto tv = Interpolator::interpolateLinear(v1, v2, alpha);

				externalData.setDisplayedValue(input);

				getParameter().call(tv);
			}
		}

		void createParameters(ParameterDataList& data)
		{
			{
				DEFINE_PARAMETERDATA(cable_table, Value);
				p.setRange({ 0.0, 1.0 });
				p.setDefaultValue(0.0);
				data.add(std::move(p));
			}
		}

		block tableData;
	};


	template <class ParameterType, int NumVoices = 1> struct pma : public combined_parameter<NumVoices>,
		public parameter_node_base<ParameterType>
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

		HISE_EMPTY_RESET;
		HISE_EMPTY_PROCESS;
		HISE_EMPTY_PROCESS_SINGLE;
		HISE_EMPTY_HANDLE_EVENT;
		HISE_EMPTY_INITIALISE;

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

	private:

		void sendParameterChange(combined_parameter_base::Data& d)
		{
			getParameter().call(d.getPmaValue());
		}
	};



}
}