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
*   which must be separately licensed for closed source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

#pragma once

namespace scriptnode {
using namespace juce;
using namespace hise;


namespace math
{
namespace Operations
{
#define SET_ID(x) static Identifier getId() { RETURN_STATIC_IDENTIFIER(#x); }
#define SET_DEFAULT(x) static constexpr float defaultValue = x;

	struct mul
	{
		SET_ID(mul); SET_DEFAULT(1.0f);

		static void op(ProcessData& d, float value)
		{
			for (int i = 0; i < d.numChannels; i++)
				FloatVectorOperations::multiply(d.data[i], value, d.size);
		}

		static void opSingle(float* frameData, int numChannels, float value)
		{
			for (int i = 0; i < numChannels; i++)
				*frameData++ *= value;
		}
	};

	struct add
	{
		SET_ID(add); SET_DEFAULT(0.0f);

		static void op(ProcessData& d, float value)
		{
			for (int i = 0; i < d.numChannels; i++)
				FloatVectorOperations::add(d.data[i], value, d.size);
		}

		static void opSingle(float* frameData, int numChannels, float value)
		{
			for (int i = 0; i < numChannels; i++)
				*frameData++ += value;
		}
	};

	struct clear
	{
		SET_ID(clear); SET_DEFAULT(0.0f);

		static void op(ProcessData& d, float)
		{
			for (int i = 0; i < d.numChannels; i++)
				FloatVectorOperations::clear(d.data[i], d.size);
		}

		static void opSingle(float* frameData, int numChannels, float)
		{
			for (int i = 0; i < numChannels; i++)
				*frameData++ = 0.0f;
		}
	};

	struct sub
	{
		SET_ID(sub); SET_DEFAULT(0.0f);

		static void op(ProcessData& d, float value)
		{
			for (int i = 0; i < d.numChannels; i++)
				FloatVectorOperations::add(d.data[i], -value, d.size);
		}

		static void opSingle(float* frameData, int numChannels, float value)
		{
			for (int i = 0; i < numChannels; i++)
				*frameData++ -= value;
		}
	};

	struct div
	{
		SET_ID(div); SET_DEFAULT(1.0f);

		static void op(ProcessData& d, float value)
		{
			auto factor = value > 0.0f ? 1.0f / value : 0.0f;
			for (int i = 0; i < d.numChannels; i++)
				FloatVectorOperations::multiply(d.data[i], factor, d.size);
		}

		static void opSingle(float* frameData, int numChannels, float value)
		{
			for (int i = 0; i < numChannels; i++)
				*frameData++ /= value;
		}
	};

	struct tanh
	{
		SET_ID(tanh); SET_DEFAULT(1.0f);

		static void op(ProcessData& d, float value)
		{
			for (int i = 0; i < d.numChannels; i++)
			{
				auto ptr = d.data[i];

				for (int j = 0; j < d.size; j++)
					ptr[j] = std::tanhf(ptr[j] * value);
			}
		}

		static void opSingle(float* frameData, int numChannels, float value)
		{
			for (int i = 0; i < numChannels; i++)
				frameData[i] = std::tanhf(frameData[i] * value);
		}
	};

	struct pi
	{
		SET_ID(pi); SET_DEFAULT(2.0f);

		static void op(ProcessData& d, float value)
		{
			for (auto ptr : d)
				FloatVectorOperations::multiply(ptr, float_Pi * value, d.size);
		}

		static void opSingle(float* frameData, int numChannels, float value)
		{
			for (int i = 0; i < numChannels; i++)
				*frameData++ *= float_Pi * value;
		}
	};

	struct sin
	{
		SET_ID(sin); SET_DEFAULT(2.0f);

		static void op(ProcessData& d, float)
		{
			for (auto ptr : d)
			{
				for (int i = 0; i < d.size; i++)
					ptr[i] = std::sin(ptr[i]);
			}
		}

		static void opSingle(float* frameData, int numChannels, float)
		{
			for (int i = 0; i < numChannels; i++)
				frameData[i] = std::sin(frameData[i]);
		}
	};

	struct sig2mod
	{
		SET_ID(sig2mod); SET_DEFAULT(0.0f);

		static void op(ProcessData& d, float)
		{
			for (auto& ch : d.channels())
				for (auto& s : ch)
					s = s * 0.5f + 0.5f;
		}

		static void opSingle(float* frameData, int numChannels, float)
		{
			for (int i = 0; i < numChannels; i++)
				frameData[i] = frameData[i] * 0.5f + 0.5f;
		}
	};

	struct clip
	{
		SET_ID(clip); SET_DEFAULT(1.0f);

		static void op(ProcessData& d, float value)
		{
			for (auto ptr : d)
				FloatVectorOperations::clip(ptr, ptr, -value, value, d.size);
		}

		static void opSingle(float* frameData, int numChannels, float value)
		{
			for (int i = 0; i < numChannels; i++)
				frameData[i] = jlimit(-value, value, frameData[i]);
		}
	};

	struct abs
	{
		SET_ID(abs); SET_DEFAULT(0.0f);

		static void op(ProcessData& d, float)
		{
			for (int i = 0; i < d.numChannels; i++)
				FloatVectorOperations::abs(d.data[i], d.data[i], d.size);
		}

		static void opSingle(float* frameData, int numChannels, float)
		{
			for (int i = 0; i < numChannels; i++)
				frameData[i] = frameData[i] > 0.0f ? frameData[i] : frameData[i] * -1.0f;
		}
	};
}

template <class OpType, int V> class OpNode : public HiseDspBase
{
public:

	constexpr static int NumVoices = V;

	SET_HISE_POLY_NODE_ID(OpType::getId());
	SET_HISE_NODE_EXTRA_HEIGHT(0);
	GET_SELF_AS_OBJECT(OpNode);
	SET_HISE_NODE_IS_MODULATION_SOURCE(false);

	bool handleModulation(double&) noexcept { return false; };

	void process(ProcessData& d)
	{
		OpType::op(d, value.get());
	}

	void processSingle(float* frameData, int numChannels)
	{
		OpType::opSingle(frameData, numChannels, value.get());
	}

	forcedinline void reset() noexcept {}

	void prepare(PrepareSpecs ps)
	{
		value.prepare(ps);
	}

	void createParameters(Array<ParameterData>& data) override
	{
		ParameterData p("Value");
		p.range = { 0.0, 1.0, 0.01 };
		p.defaultValue = OpType::defaultValue;

		p.db = BIND_MEMBER_FUNCTION_1(OpNode::setValue);

		data.add(std::move(p));
	}

	void setValue(double newValue)
	{
		if (NumVoices == 1)
		{
			value.getMonoValue() = (float)newValue;
		}
		else
		{
			if (value.isVoiceRenderingActive())
			{
				value.get() = (float)newValue;
			}
			else
			{
				auto nv = (float)newValue;
				value.forEachVoice([nv](float& v)
				{
					v = nv;
				});
			}
		}


	}

	PolyData<float, NumVoices> value = OpType::defaultValue;
};


#if 0
extern template class OpNode<Operations::mul, 1>; 
using mul = OpNode<Operations::mul, 1>; 
extern template class OpNode<Operations::mul, NUM_POLYPHONIC_VOICES>; 
using mul_poly = OpNode<Operations::mul, NUM_POLYPHONIC_VOICES>;
#endif

#define DEFINE_OP_NODE_IMPL(opName) template class OpNode<Operations::opName, 1>; \
template class OpNode<Operations::opName, NUM_POLYPHONIC_VOICES>;

#define DEFINE_OP_NODE(monoName, polyName) extern template class OpNode<Operations::monoName, 1>; \
using monoName = OpNode<Operations::monoName, 1>; \
extern template class OpNode<Operations::monoName, NUM_POLYPHONIC_VOICES>; \
using polyName = OpNode<Operations::monoName, NUM_POLYPHONIC_VOICES>;


#if 0
using add = OpNode<Operations::add, 1>;
using sub = OpNode<Operations::sub, 1>;
using div = OpNode<Operations::div, 1>;
using tanh = OpNode<Operations::tanh, 1>;
using clip = OpNode<Operations::clip, 1>;
using sin = OpNode<Operations::sin, 1>;
using pi = OpNode<Operations::pi, 1>;
using sig2mod = OpNode<Operations::sig2mod, 1>;
using abs = OpNode<Operations::abs, 1>;
using clear = OpNode<Operations::clear, 1>;


using add_poly = OpNode<Operations::add, NUM_POLYPHONIC_VOICES>;
using sub_poly = OpNode<Operations::sub, NUM_POLYPHONIC_VOICES>;
using div_poly = OpNode<Operations::div, NUM_POLYPHONIC_VOICES>;
using tanh_poly = OpNode<Operations::tanh, NUM_POLYPHONIC_VOICES>;
using clip_poly = OpNode<Operations::clip, NUM_POLYPHONIC_VOICES>;
using pi_poly = OpNode<Operations::pi, NUM_POLYPHONIC_VOICES>;
using abs_poly = OpNode<Operations::abs, NUM_POLYPHONIC_VOICES>;
#endif

DEFINE_OP_NODE(mul, mul_poly);
DEFINE_OP_NODE(add, add_poly);
DEFINE_OP_NODE(sub, sub_poly);
DEFINE_OP_NODE(div, div_poly);
DEFINE_OP_NODE(tanh, tanh_poly);
DEFINE_OP_NODE(clip, clip_poly);
DEFINE_OP_NODE(sin, sin_poly);
DEFINE_OP_NODE(pi, pi_poly);
DEFINE_OP_NODE(sig2mod, sig2mod_poly);
DEFINE_OP_NODE(abs, abs_poly);
DEFINE_OP_NODE(clear, clear_poly);

}


}
