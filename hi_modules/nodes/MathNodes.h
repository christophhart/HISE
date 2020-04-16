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

		static void op(ProcessData& d, float value);
		static void opSingle(float* frameData, int numChannels, float value);
	};

	struct add
	{
		SET_ID(add); SET_DEFAULT(0.0f);

		static void op(ProcessData& d, float value);
		static void opSingle(float* frameData, int numChannels, float value);
	};

	struct clear
	{
		SET_ID(clear); SET_DEFAULT(0.0f);

		static void op(ProcessData& d, float);
		static void opSingle(float* frameData, int numChannels, float);
	};

	struct sub
	{
		SET_ID(sub); SET_DEFAULT(0.0f);

		static void op(ProcessData& d, float value);
		static void opSingle(float* frameData, int numChannels, float value);
	};

	struct div
	{
		SET_ID(div); SET_DEFAULT(1.0f);

		static void op(ProcessData& d, float value);
		static void opSingle(float* frameData, int numChannels, float value);
	};

	struct tanh
	{
		SET_ID(tanh); SET_DEFAULT(1.0f);

		static void op(ProcessData& d, float value);
		static void opSingle(float* frameData, int numChannels, float value);
	};

	struct pi
	{
		SET_ID(pi); SET_DEFAULT(2.0f);

		static void op(ProcessData& d, float value);
		static void opSingle(float* frameData, int numChannels, float value);
	};

	struct sin
	{
		SET_ID(sin); SET_DEFAULT(2.0f);

		static void op(ProcessData& d, float);
		static void opSingle(float* frameData, int numChannels, float);
	};

	struct sig2mod
	{
		SET_ID(sig2mod); SET_DEFAULT(0.0f);

		static void op(ProcessData& d, float);
		static void opSingle(float* frameData, int numChannels, float);
	};

	struct clip
	{
		SET_ID(clip); SET_DEFAULT(1.0f);

		static void op(ProcessData& d, float value);
		static void opSingle(float* frameData, int numChannels, float value);
	};

	struct square
	{
		SET_ID(square); SET_DEFAULT(1.0f);

		static void op(ProcessData& d, float value);
		static void opSingle(float* frameData, int numChannels, float value);
	};

	struct sqrt
	{
		SET_ID(sqrt); SET_DEFAULT(1.0f);

		static void op(ProcessData& d, float value);
		static void opSingle(float* frameData, int numChannels, float value);
	};

	struct pow
	{
		SET_ID(pow); SET_DEFAULT(1.0f);

		static void op(ProcessData& d, float value);
		static void opSingle(float* frameData, int numChannels, float value);
	};

	struct abs
	{
		SET_ID(abs); SET_DEFAULT(0.0f);

		static void op(ProcessData& d, float);
		static void opSingle(float* frameData, int numChannels, float);
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

	bool handleModulation(double&) noexcept;;
	void process(ProcessData& d);
	void processSingle(float* frameData, int numChannels);
	void reset() noexcept;
	void prepare(PrepareSpecs ps);
	void createParameters(Array<ParameterData>& data) override;
	void setValue(double newValue);

	STATIC_TO_MEMBER_PARAMETER(OpNode);

	template <int P> void setParameter(double v)
	{
		setValue(v);
	}

	PolyData<float, NumVoices> value = OpType::defaultValue;
};

#define DEFINE_OP_NODE_IMPL(opName) template class OpNode<Operations::opName, 1>; \
template class OpNode<Operations::opName, NUM_POLYPHONIC_VOICES>;

#define DEFINE_MONO_OP_NODE_IMPL(opName) template class OpNode<Operations::opName, 1>; \

#define DEFINE_MONO_OP_NODE(monoName) extern template class OpNode<Operations::monoName, 1>; \
using monoName = OpNode<Operations::monoName, 1>;

#define DEFINE_OP_NODE(monoName, polyName) extern template class OpNode<Operations::monoName, 1>; \
using monoName = OpNode<Operations::monoName, 1>; \
extern template class OpNode<Operations::monoName, NUM_POLYPHONIC_VOICES>; \
using polyName = OpNode<Operations::monoName, NUM_POLYPHONIC_VOICES>;

DEFINE_OP_NODE(mul, mul_poly);
DEFINE_OP_NODE(add, add_poly);
DEFINE_OP_NODE(sub, sub_poly);
DEFINE_OP_NODE(div, div_poly);
DEFINE_OP_NODE(tanh, tanh_poly);
DEFINE_OP_NODE(clip, clip_poly);
DEFINE_MONO_OP_NODE(sin);
DEFINE_MONO_OP_NODE(pi);
DEFINE_MONO_OP_NODE(sig2mod);
DEFINE_MONO_OP_NODE(abs);
DEFINE_MONO_OP_NODE(clear);
DEFINE_MONO_OP_NODE(square);
DEFINE_MONO_OP_NODE(sqrt);
DEFINE_MONO_OP_NODE(pow);

}


}
