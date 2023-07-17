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
#define SET_DEFAULT(x) float getDefaultValue() const { return x; }


#define OP_BLOCK(data, value) template <typename PD> static void op(PD& data, float value)
#define OP_SINGLE(data, value) template <typename FD> static void opSingle(FD& data, float value)
#define OP_BLOCK2SINGLE(data, value) OP_BLOCK(data, value) { for (auto ch : data) { block b(data.toChannelData(ch)); opSingle(b, value); }}

	struct mul
	{
		SET_ID(mul);

		OP_BLOCK(data, value)
		{
			for (auto ch : data)
            {
                block b(data.toChannelData(ch));
                hmath::vmuls(b, value);
            }
				
		}

		OP_SINGLE(data, value)
		{
			for (auto& s : data)
				s *= value;
		}
	};

	struct add
	{
		SET_ID(add); SET_DEFAULT(0.0f);

		OP_BLOCK(data, value)
		{
			for (auto ch : data)
			{
				block b(data.toChannelData(ch));
				hmath::vadds(b, value);
			}
				
		}

		OP_SINGLE(data, value)
		{
			for (auto& s : data)
				s += value;
		}
	};

	struct clear
	{
		SET_ID(clear); SET_DEFAULT(0.0f);

		OP_BLOCK(data, value)
		{
			for (auto ch : data)
            {
                auto dst = data.toChannelData(ch);
                hmath::vmovs(dst, 0.0f);
            }
				
		}

		OP_SINGLE(data, value)
		{
			for (auto& s : data)
				s = 0.0f;
		}
	};

	struct sub
	{
		SET_ID(sub); SET_DEFAULT(0.0f);

		OP_BLOCK(data, value)
		{
			for (auto ch : data)
            {
                block b(data.toChannelData(ch));
                hmath::vadds(b, -value);
            }
				
		}

		OP_SINGLE(data, value)
		{
			for (auto& s : data)
				s -= value;
		}
	};

	struct div
	{
		SET_ID(div); SET_DEFAULT(1.0f);

		OP_BLOCK(data, value)
		{
			auto factor = value > 0.0f ? 1.0f / value : 0.0f;

			for (auto ch : data)
            {
                block b(data.toChannelData(ch));
                hmath::vmuls(b, factor);
            }
				
		}

		OP_SINGLE(data, value)
		{
			auto factor = value > 0.0f ? 1.0f / value : 0.0f;

			for (auto& s : data)
				s *= factor;
		}
	};



	struct tanh
	{
		SET_ID(tanh); SET_DEFAULT(1.0f);

		OP_BLOCK2SINGLE(data, value);

		OP_SINGLE(data, value)
		{
			for (auto& s : data)
				s = tanhf(s * value);
		}
	};

	struct pi
	{
		SET_ID(pi); SET_DEFAULT(2.0f);

		OP_BLOCK(data, unused)
		{
			for (auto ch : data)
            {
                block b(data.toChannelData(ch));
				hmath::vmuls(b, float_Pi);
            }
		}

		OP_SINGLE(data, value)
		{
			for (auto& s : data)
				s *= float_Pi;
		}
	};

	struct sin
	{
		SET_ID(sin); SET_DEFAULT(2.0f);

		OP_BLOCK2SINGLE(data, unused);

		OP_SINGLE(data, unused)
		{
			for (auto& s : data)
				s = sinf(s);
		}
	};

	struct sig2mod
	{
		SET_ID(sig2mod); SET_DEFAULT(0.0f);

		OP_BLOCK2SINGLE(data, unused);

		OP_SINGLE(data, unused)
		{
			for (auto& s : data)
				s = s * 0.5f + 0.5f;
		}
	};

	struct clip
	{
		SET_ID(clip); SET_DEFAULT(1.0f);

		OP_BLOCK(data, value)
		{
			for (auto ch : data)
            {
                block b(data.toChannelData(ch));
				hmath::vclip(b, -value, value);
            }
		}

		OP_SINGLE(data, value)
		{
			for (auto& s : data)
				s *= jlimit(-value, value, s);
		}
	};

	struct square
	{
		SET_ID(square); SET_DEFAULT(1.0f);

		OP_BLOCK(data, value)
		{
			for (auto ch : data)
            {
                block b(data.toChannelData(ch));
				hmath::vmul(b, b);
            }
		}

		OP_SINGLE(data, value)
		{
			for (auto& s : data)
				s *= s;
		}
	};

	struct sqrt
	{
		SET_ID(sqrt); SET_DEFAULT(1.0f);

		OP_BLOCK2SINGLE(data, unused);

		OP_SINGLE(data, value)
		{
			for (auto& s : data)
				s = sqrtf(s);
		}
	};

	struct pow
	{
		SET_ID(pow); SET_DEFAULT(1.0f);

		OP_BLOCK2SINGLE(data, unused);

		OP_SINGLE(data, value)
		{
			for (auto& s : data)
				s = powf(s, value);
		}
	};

	struct abs
	{
		SET_ID(abs); SET_DEFAULT(0.0f);

		OP_BLOCK(data, unused)
		{
			for (auto& ch : data)
            {
                block b(data.toChannelData(ch));
                hmath::vabs(b);
            }
				
		}

		OP_SINGLE(data, value)
		{
			for (auto& s : data)
				s = hmath::abs(s);
		}
	};
}

template <class OpType> class OpNodeBase : public mothernode,
										   public polyphonic_base
{
public:

	OpNodeBase():
		polyphonic_base(OpType::getId(), false)
	{

	}

	virtual ~OpNodeBase() {};

	SN_DEFAULT_INIT(OpType);

	float getDefaultValue() const
	{
		if constexpr (prototypes::check::getDefaultValue<OpType>::value)
			return obj.getDefaultValue();
		else
			return 1.0f;
	}

	OpType obj;
};

struct map: public mothernode
{
    enum class Parameters
    {
        InputStart,
        InputEnd,
        OutputStart,
        OutputEnd,
    };
    
    SNEX_NODE(map);
    bool isPolyphonic() const { return false; }
    
    SN_DESCRIPTION("A math operator that maps a signal from one range to another");
    SN_EMPTY_HANDLE_EVENT;
    SN_EMPTY_PREPARE;
    SN_EMPTY_RESET;
    
    template <int P> void setParameter(double v)
    {
        if(P == 0) inStart = (float)v;
        if(P == 1) inEnd = (float)v;
        if(P == 2) outStart = (float)v;
        if(P == 3) outEnd = (float)v;
        
        auto inLengthInv = inEnd == inStart ? 0.0f : (1.0f / (inEnd - inStart));
        auto outLength = (outEnd - outStart);
        
        rangeFactor = inLengthInv * outLength;
        clipMax = hmath::abs(inEnd - inStart);
    }
    
    template <typename T> void op(T& value)
    {
        value -= inStart;
        
        value = hmath::range(value, 0.0f, clipMax);
        
        value *= rangeFactor;
        value += outStart;
    }
    
    template <typename PD> void process(PD& d)
    {
        
        for(auto& ch: d)
        {
            auto b = d.toChannelData(ch);
            op(b);
        }
    }

    template <typename FD> void processFrame(FD& d)
    {
        for(auto& s: d)
            op(s);
    }

    void createParameters(ParameterDataList& data)
    {
        {
            DEFINE_PARAMETERDATA(OpNode, InputStart);
            p.setDefaultValue(0.0f);
            data.add(std::move(p));
        }
        {
            DEFINE_PARAMETERDATA(OpNode, InputEnd);
            p.setDefaultValue(1.0f);
            data.add(std::move(p));
        }
        {
            DEFINE_PARAMETERDATA(OpNode, OutputStart);
            p.setDefaultValue(0.0f);
            data.add(std::move(p));
        }
        {
            DEFINE_PARAMETERDATA(OpNode, OutputEnd);
            p.setDefaultValue(1.0f);
            data.add(std::move(p));
        }
    }
    
private:
    
    float inStart = 0.0f;
    float inEnd = 1.0f;
    
    float outStart = 0.0f;
    float outEnd = 1.0f;
    
    float clipMax = 1.0f;
    float rangeFactor = 1.0f;
};

template <class OpType, int V> class OpNode: public OpNodeBase<OpType>
{
public:

	using OperationType = OpType;
	constexpr static int NumVoices = V;

	enum class Parameters
	{
		Value
	};

	DEFINE_PARAMETERS
	{
		DEF_PARAMETER(Value, OpNode);
	}

	SN_PARAMETER_MEMBER_FUNCTION;

	SN_POLY_NODE_ID(OpType::getId());
	SN_GET_SELF_AS_OBJECT(OpNode);
	SN_DESCRIPTION("A math operator on the input signal");
	SN_EMPTY_HANDLE_EVENT;
	
	OpNode()
	{
		for (auto v : value)
			v = this->getDefaultValue();
	}

	OpNode(const OpNode& other) = default;

	template <typename PD> void process(PD& d)
	{
		this->obj.op(d, value.get());
	}

	template <typename FD> void processFrame(FD& d)
	{
		this->obj.opSingle(d, value.get());
	}

	SN_EMPTY_RESET;

	void prepare(PrepareSpecs ps)
	{
		value.prepare(ps);
	}

	void createParameters(ParameterDataList& data)
	{
		{
			DEFINE_PARAMETERDATA(OpNode, Value);
			p.setDefaultValue(this->getDefaultValue());
			data.add(std::move(p));
		}
	}

	void setValue(double newValue)
	{
		for (auto& v : value)
			v = (float)newValue;
	}

	PolyData<float, NumVoices> value;
};

#define DEFINE_MONO_OP_NODE(monoName) template <int NV> using monoName = OpNode<Operations::monoName, 1>;

#define DEFINE_OP_NODE(monoName) template <int NV> using monoName = OpNode<Operations::monoName, NV>;

DEFINE_OP_NODE(mul);
DEFINE_OP_NODE(add);
DEFINE_OP_NODE(sub);
DEFINE_OP_NODE(div);
DEFINE_OP_NODE(tanh);
DEFINE_OP_NODE(clip);
DEFINE_MONO_OP_NODE(sin);
DEFINE_MONO_OP_NODE(pi);
DEFINE_MONO_OP_NODE(sig2mod);
DEFINE_MONO_OP_NODE(abs);
DEFINE_MONO_OP_NODE(clear);
DEFINE_OP_NODE(square);
DEFINE_OP_NODE(sqrt);
DEFINE_OP_NODE(pow);

template <typename ExpressionClass> struct expression_base
{
	static Identifier getId() { return "expr"; };

	template <typename T> static void op(T& data, float value)
	{
		for (auto& ch : data)
		{
			for (auto& s : data.toChannelData(ch))
				s = ExpressionClass::op(s, value);
		}
	}
	template <typename T> static void opSingle(T& data, float value)
	{
		for (auto& s : data)
			s = ExpressionClass::op(s, value);
	}
};

template <int NV, class ExpressionClass> using expr = OpNode<expression_base<ExpressionClass>, NV>;

}


}
