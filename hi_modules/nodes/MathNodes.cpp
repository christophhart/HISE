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

namespace scriptnode {
using namespace juce;
using namespace hise;

namespace math
{
using namespace std;

template <class OpType, int V>
void scriptnode::math::OpNode<OpType, V>::reset() noexcept
{

}

template <class OpType, int V>
void scriptnode::math::OpNode<OpType, V>::prepare(PrepareSpecs ps)
{
	value.prepare(ps);
}


template <class OpType, int V>
bool scriptnode::math::OpNode<OpType, V>::handleModulation(double&) noexcept
{
	return false;
}

template <typename T> struct CreateFunctions
{
	static FunctionData* createProcess(const snex::Types::SnexTypeConstructData& cd)
	{
		
		jassertfalse;
		return nullptr;
	}
};

template <class OpType, int V>
snex::Types::DefaultFunctionClass scriptnode::math::OpNode<OpType, V>::createSnexFunctions(const snex::Types::SnexTypeConstructData& cd)
{
	OpNode* ptr = nullptr;
	snex::Types::DefaultFunctionClass f(ptr);
	//f.processFunction = CreateFunctions<OpNode>::createProcess;

	return f;
}


template <class OpType, int V>
void scriptnode::math::OpNode<OpType, V>::createParameters(ParameterDataList& data)
{
	{
		DEFINE_PARAMETERDATA(OpNode, Value);
		p.range = { 0.0, 1.0, 0.01 };
		p.defaultValue = OpType::defaultValue;
		data.add(std::move(p));
	}
}

#pragma warning(push)
#pragma warning(disable: 4127)

template <class OpType, int V>
void scriptnode::math::OpNode<OpType, V>::setValue(double newValue)
{
	if (NumVoices == 1)
	{
		value.getMonoValue() = (float)newValue;
	}
	else
	{
		if (value.isMonophonicOrInsideVoiceRendering())
		{
			value.get() = (float)newValue;
		}
		else
		{
			auto nv = (float)newValue;

			for (auto& v : value)
				v = nv;
		}
	}
}

#pragma warning( pop)

DEFINE_OP_NODE_IMPL(mul);
DEFINE_OP_NODE_IMPL(add);
DEFINE_OP_NODE_IMPL(sub);
DEFINE_OP_NODE_IMPL(div);
DEFINE_OP_NODE_IMPL(tanh);
DEFINE_OP_NODE_IMPL(clip);
DEFINE_MONO_OP_NODE_IMPL(sin);
DEFINE_MONO_OP_NODE_IMPL(pi);
DEFINE_MONO_OP_NODE_IMPL(sig2mod);
DEFINE_MONO_OP_NODE_IMPL(abs);
DEFINE_MONO_OP_NODE_IMPL(clear);
DEFINE_MONO_OP_NODE_IMPL(square);
DEFINE_MONO_OP_NODE_IMPL(sqrt);
DEFINE_MONO_OP_NODE_IMPL(pow);

Factory::Factory(DspNetwork* n) :
	NodeFactory(n)
{
	
	registerPolyNodeCustomSNEX<add, add_poly>();

#if NOT_JUST_OSC
	registerPolyNode<mul, mul_poly>();
	
	registerNode<clear>();
	registerPolyNode<sub, sub_poly>();
	registerPolyNode<div, div_poly>();
	registerPolyNode<tanh, tanh_poly>();
	registerPolyNode<clip, clip_poly>();
	registerNode<sin>();
	registerNode<pi>();
	registerNode<sig2mod>();
	registerNode<abs>();
	registerNode<square>();
	registerNode<sqrt>();
	registerNode<pow>();
#endif

	sortEntries();
}

}

}