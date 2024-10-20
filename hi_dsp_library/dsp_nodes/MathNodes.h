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

#define SET_DESCRIPTION(x) static String getDescription() { return x; };

#define OP_BLOCK(data, value) template <typename PD> static void op(PD& data, float value)
#define OP_SINGLE(data, value) template <typename FD> static void opSingle(FD& data, float value)
#define OP_BLOCK2SINGLE(data, value) OP_BLOCK(data, value) { for (auto ch : data) { block b(data.toChannelData(ch)); opSingle(b, value); }}

	struct mul
	{
		SET_ID(mul);
        SET_DESCRIPTION("Multiplies the signal with a scalar value");

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
        SET_DESCRIPTION("Adds a scalar value to the signal");

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

    struct fill1
    {
        SET_ID(fill1); SET_DEFAULT(0.0f);
        SET_DESCRIPTION("Fills the signal with a constant 1.0");

        OP_BLOCK(data, value)
        {
            for (auto ch : data)
            {
                auto dst = data.toChannelData(ch);
                hmath::vmovs(dst, 1.0);
            }
                
        }

        OP_SINGLE(data, value)
        {
            for (auto& s : data)
                s = 1.0f;
        }
    };

	struct clear
	{
		SET_ID(clear); SET_DEFAULT(0.0f);
        SET_DESCRIPTION("Clears the signal (sets it to zero).");
        
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

    struct intensity
    {
        SET_DESCRIPTION("Applies the HISE intensity formula to the input signal");
        
        SET_ID(intensity); SET_DEFAULT(0.0f);

        OP_BLOCK(data, value)
        {
            const float alpha = value;
            const float invAlpha = 1.0f - alpha;
            
            for (auto ch : data)
            {
                block b(data.toChannelData(ch));
                hmath::vmuls(b, alpha);
                hmath::vadds(b, invAlpha);
            }
        }

        OP_SINGLE(data, value)
        {
            const float alpha = value;
            const float invAlpha = 1.0f - alpha;
            
            for (auto& s : data)
                s = invAlpha + alpha * s;
        }
    };

    struct mod_inv
    {
        SET_DESCRIPTION("Inverts a modulation signal from 0-1 to 1-0");
        SET_ID(mod_inv); SET_DEFAULT(0.0f);

        OP_BLOCK(data, value)
        {
            for (auto ch : data)
            {
                block b(data.toChannelData(ch));
                hmath::vmuls(b, -1.0f);
                hmath::vadds(b, 1.0f);
            }
        }

        OP_SINGLE(data, value)
        {
            for (auto& s : data)
                s = 1.0f - s;
        }
    };

    struct inv
    {
        SET_ID(inv); SET_DEFAULT(0.0f);
        SET_DESCRIPTION("Inverts the phase of a signal");

        OP_BLOCK(data, value)
        {
            for (auto ch : data)
            {
                block b(data.toChannelData(ch));
                hmath::vmuls(b, -1.0f);
            }
        }

        OP_SINGLE(data, value)
        {
            for (auto& s : data)
                s *= -1.0f;
        }
    };

    

	struct sub
	{
        SET_DESCRIPTION("Subtracts a scalar value from the signal");
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
        SET_DESCRIPTION("Divides the signal by scalar value != 0.0");
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

    struct fmod
	{
        SET_DESCRIPTION("Calculates the floating point modulo from the signal.");
		SET_ID(fmod); SET_DEFAULT(1.0f);

		OP_BLOCK(data, value)
		{
            if(value == 0.0f)
                return;

			for (auto ch : data)
			{
				block b(data.toChannelData(ch));

                for (auto& s : b)
					s = hmath::fmod(s, value);
			}
		};

		OP_SINGLE(data, value)
		{
            if(value == 0.0f)
                return;

			for (auto& s : data)
				s = hmath::fmod(s, value);
		}
	};

	struct tanh
	{
        SET_DESCRIPTION("Applies the tanh function on the signal.");
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
        SET_DESCRIPTION("Multiplies the signal with PI (3.13)");
		SET_ID(pi); SET_DEFAULT(2.0f);

		OP_BLOCK(data, value)
		{
			for (auto ch : data)
            {
                block b(data.toChannelData(ch));
				hmath::vmuls(b, float_Pi * value);
            }
		}

		OP_SINGLE(data, value)
		{
			for (auto& s : data)
				s *= (float_Pi * value);
		}
	};

	struct sin
	{
        SET_DESCRIPTION("Applies the sin function on the signal.");
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
        SET_DESCRIPTION("Converts a -1...1 signal to a 0...1 signal.");
		SET_ID(sig2mod); SET_DEFAULT(0.0f);

		OP_BLOCK2SINGLE(data, unused);

		OP_SINGLE(data, unused)
		{
			for (auto& s : data)
				s = s * 0.5f + 0.5f;
		}
	};

    struct mod2sig
    {
        SET_DESCRIPTION("Converts a 0...1 signal to a -1...1 signal.");
        SET_ID(mod2sig); SET_DEFAULT(0.0f);

        OP_BLOCK2SINGLE(data, unused);

        OP_SINGLE(data, unused)
        {
            for (auto& s : data)
                s = s * 2.0f - 1.0f;
        }
    };

    struct rect
    {
        SET_ID(rect); SET_DEFAULT(0.0f);
        SET_DESCRIPTION("Rectifies a normalised signal to 0 or 1");

        OP_BLOCK2SINGLE(data, unused);

        OP_SINGLE(data, unused)
        {
            for (auto& s : data)
                s = (float)s >= 0.5f;
        }
    };

	struct clip
	{
        SET_DESCRIPTION("Truncates the signal range (on both ends) using the Value as limit");
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
        SET_DESCRIPTION("Multiplies the signal with itself.");
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
        SET_DESCRIPTION("Applies the sqrt function on the signal.");
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
        SET_DESCRIPTION("Calculates the pow function of the signal with the Value as exponent.");
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
        SET_DESCRIPTION("Calculates the absolute signal (folds negative values).");
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

template <int DataSize> struct complex_data_lut: public scriptnode::data::base
{
    SN_NODE_ID(DataSize ? "table" : "pack");
    SN_GET_SELF_AS_OBJECT(complex_data_lut);
    SN_DESCRIPTION(DataSize ? "Processes the signal with the table as LUT function" :
                              "Processes the signal with the slider pack as LUT function");

    SN_EMPTY_HANDLE_EVENT;
    SN_EMPTY_SET_PARAMETER;
    SN_EMPTY_INITIALISE;
    SN_EMPTY_RESET;
    SN_EMPTY_CREATE_PARAM;
    SN_EMPTY_PREPARE;

    bool isPolyphonic() const { return false; };

    template <typename ProcessDataType> void process(ProcessDataType& data) noexcept
    {
        DataReadLock l(this);

        if (!tableData.isEmpty())
        {
            auto v = data.getRawDataPointers()[0][0];
            
            if constexpr (DataSize == 0)
                v = jlimit<float>(0.0f, (float)(tableData.size()-1), v * (float)tableData.size());
            else
                v = jlimit(0.0f, 1.0f, v);
            
            for (auto& ch : data)
            {
                for (auto& s : data.toChannelData(ch))
                {
                    processFloat(s);
                }
            }

            externalData.setDisplayedValue(v);
        }
    }

    void processFloat(float& s)
    {
        InterpolatorType ip(s);
        s = tableData[ip];
    }

    void setExternalData(const ExternalData& d, int) override
    {
        base::setExternalData(d, 0);
        d.referBlockTo(tableData, 0);
    }

    template <typename FrameDataType> void processFrame(FrameDataType& data) noexcept
    {
        DataReadLock l(this);
        
        if (!tableData.isEmpty())
        {
            auto v = data[0];
            
            if constexpr (DataSize == 0)
                v = jlimit<float>(0.0f, (float)(tableData.size()-1), v * (float)tableData.size());
            else
                v = jlimit(0.0f, 1.0f, v);
            
            for(auto& s: data)
            {
                processFloat(s);
            }


            externalData.setDisplayedValue(v);
        }
    }

    block tableData;

    using TableClampType = index::clamped<DataSize>;
    using InterpolatorType = index::lerp<index::normalised<float, TableClampType>>;
    
    JUCE_DECLARE_WEAK_REFERENCEABLE(complex_data_lut);
};

using table = complex_data_lut<SAMPLE_LOOKUP_TABLE_SIZE>;
using pack = complex_data_lut<0>;

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
	SN_DESCRIPTION(OpType::getDescription());
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
DEFINE_MONO_OP_NODE(fill1);
DEFINE_OP_NODE(sub);
DEFINE_OP_NODE(div);
DEFINE_OP_NODE(tanh);
DEFINE_OP_NODE(fmod);
DEFINE_OP_NODE(clip);
DEFINE_MONO_OP_NODE(sin);
DEFINE_MONO_OP_NODE(pi);
DEFINE_MONO_OP_NODE(rect);
DEFINE_MONO_OP_NODE(sig2mod);
DEFINE_MONO_OP_NODE(mod2sig);
DEFINE_MONO_OP_NODE(mod_inv);
DEFINE_MONO_OP_NODE(inv);
DEFINE_MONO_OP_NODE(abs);
DEFINE_MONO_OP_NODE(clear);
DEFINE_OP_NODE(square);
DEFINE_OP_NODE(intensity);
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


#if HISE_INCLUDE_RT_NEURAL

/** TODO:
 * - Parameters
 * - ProcessingModes
 */
template <int NV, typename IndexType> struct neural:
public runtime_target::indexable_target<IndexType, runtime_target::RuntimeTarget::NeuralNetwork, hise::NeuralNetwork*>,
public polyphonic_base
{
public:

    SN_NODE_ID("neural");
    
    SN_GET_SELF_AS_OBJECT(neural);
    
    SN_DESCRIPTION("Runs a per-sample inference on the first channel of the signal using a neural network");
    
    SN_EMPTY_INITIALISE;
    
    neural():
      polyphonic_base(getStaticId(), false)
    {};
    
    void prepare(PrepareSpecs ps)
    {
        if(!ps)
            return;

        lastSpecs = ps;

        auto originalNetwork = static_cast<NeuralNetwork*>(this->currentConnection.source);;
        
        if(originalNetwork != nullptr)
        {
            auto numClones = NV;

            if(originalNetwork->context.shouldCloneChannels())
                numClones *= ps.numChannels;

            thisNetwork = originalNetwork->clone(numClones);

            voiceIndexOffsets.prepare(ps);

            int idx = 0;

            for(auto& v: voiceIndexOffsets)
            {
                v = idx;
                idx += ps.numChannels;
            }
        }

        reset();
    }

    PolyData<int, NV> voiceIndexOffsets;

    static constexpr bool isPolyphonic() { return NV > 1; }

    void reset()
    {
        auto currentNetwork = getCurrentNetwork();
        if(currentNetwork != nullptr)
        {
            for(auto v: voiceIndexOffsets)
            {
                for(int c = 0; c < lastSpecs.numChannels; c++)
                    currentNetwork->reset(v + c);
            }
        }
    }

    void onValue(NeuralNetwork*) override {};
    
    void onConnectionChange() override
    {
        prepare(lastSpecs);
    };
    
    int getNumExpectedNetworks() const
    {
        return (isPolyphonic() ? NV : 1) * lastSpecs.numChannels;
    }

    SN_EMPTY_HANDLE_EVENT;
    
    template <typename PD> void process(PD& data)
    {
        auto currentNetwork = getCurrentNetwork();
        
        if(currentNetwork != nullptr && getNumExpectedNetworks() == currentNetwork->getNumNetworks())
        {
            auto offset = voiceIndexOffsets.get();

            int c = 0;
            for(auto& ch: data)
            {
                auto bl = data.toChannelData(ch);

                for(auto& s: bl)
                    currentNetwork->process(offset + c, &s, &s);

                c++;
            }
        }
    }

    template <typename FD> void processFrame(FD& data)
    {
        auto currentNetwork = getCurrentNetwork();
        if(currentNetwork != nullptr && data.size() == currentNetwork->getNumNetworks())
        {
            auto offset = voiceIndexOffsets.get();

            int c = 0;
            for(auto& s: data)
                currentNetwork->process(offset + c++, &s, &s);
        }
    }

    NeuralNetwork* getCurrentNetwork()
    {
        return thisNetwork.get();
    }
    
    NeuralNetwork::Ptr thisNetwork;
    
    PrepareSpecs lastSpecs;
};

#else

template <int NV, typename IndexType> struct neural: public polyphonic_base
{
	SN_NODE_ID("neural");
    
    SN_GET_SELF_AS_OBJECT(neural);
    
    SN_DESCRIPTION("Runs a per-sample inference on the first channel of the signal using a neural network");
    
    neural():
      polyphonic_base(getStaticId(), false)
    {};

    SN_EMPTY_INITIALISE;
    SN_EMPTY_RESET;
    SN_EMPTY_MOD;
    SN_EMPTY_PREPARE;
    SN_EMPTY_PROCESS;
    SN_EMPTY_PROCESS_FRAME;
};

#endif

}
}
