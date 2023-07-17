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

namespace dynamics
{
    
struct DynamicHelpers
{
    static Identifier getId(chunkware_simple::SimpleGate*)
    {
        RETURN_STATIC_IDENTIFIER("gate");
    }
    
    
    
    static Identifier getId(chunkware_simple::SimpleComp*)
    {
        RETURN_STATIC_IDENTIFIER("comp");
    }
    
    static Identifier getId(chunkware_simple::SimpleCompRms*)
    {
        RETURN_STATIC_IDENTIFIER("comp_rms");
    }
    
    static Identifier getId(chunkware_simple::SimpleLimit*)
    {
        RETURN_STATIC_IDENTIFIER("limiter");
    }
    
    static String getDescription(const chunkware_simple::SimpleGate*)
    {
        return "A gate effect with the ducking amount as modulation signal";
    }
    
    static String getDescription(const chunkware_simple::SimpleComp*)
    {
        return "A compressor with the ducking amount as modulation signal";
    }
    
    static String getDescription(const chunkware_simple::SimpleLimit*)
    {
        return "A limiter with the ducking amount as modulation signal";
    }
};
    
template <class DynamicProcessorType> class dynamics_wrapper : public HiseDspBase,
															   public data::display_buffer_base<true>
{
public:

	enum class Parameters
	{
		Threshhold,
		Attack,
		Release,
		Ratio,
        Sidechain
	};
    
    enum class SidechainMode
    {
        Disabled = 0,
        Original,
        Sidechain,
        numSideChainModes
    };

	DEFINE_PARAMETERS
	{
		DEF_PARAMETER(Threshhold, dynamics_wrapper);
		DEF_PARAMETER(Attack, dynamics_wrapper);
		DEF_PARAMETER(Release, dynamics_wrapper);
		DEF_PARAMETER(Ratio, dynamics_wrapper);
        DEF_PARAMETER(Sidechain, dynamics_wrapper);
	}
	SN_PARAMETER_MEMBER_FUNCTION;

	static Identifier getStaticId()
    {
        DynamicProcessorType* t = nullptr;
        return DynamicHelpers::getId(t);
    }

	static String getDescription()
    {
        DynamicProcessorType* t = nullptr;
        return DynamicHelpers::getDescription(t);
    }

	static constexpr bool isNormalisedModulation() { return true; };

	SN_GET_SELF_AS_OBJECT(dynamics_wrapper);

	dynamics_wrapper()
    {
        
    };

	SN_EMPTY_HANDLE_EVENT;

	void createParameters(ParameterDataList& data)
    {
        {
            DEFINE_PARAMETERDATA(dynamics_wrapper, Threshhold);
            p.setRange({ -100.0, 0.0, 0.1 });
            p.setSkewForCentre(-12.0);
            p.setDefaultValue(0.0);
            data.add(std::move(p));
        }
        
        {
            DEFINE_PARAMETERDATA(dynamics_wrapper, Attack);
            p.setRange({ 0.0, 250.0, 0.1 });
            p.setSkewForCentre(50.0);
            p.setDefaultValue(50.0);
            data.add(std::move(p));
        }
        
        {
            DEFINE_PARAMETERDATA(dynamics_wrapper, Release);
            p.setRange({ 0.0, 250.0, 0.1 });
            p.setSkewForCentre(50.0);
            p.setDefaultValue(50.0);
            data.add(std::move(p));
        }
        
        {
            DEFINE_PARAMETERDATA(dynamics_wrapper, Ratio);
            p.setRange({ 1.0, 32.0, 0.1 });
            p.setSkewForCentre(4.0);
            p.setDefaultValue(1.0);
            data.add(std::move(p));
        }
        
        {
            DEFINE_PARAMETERDATA(dynamics_wrapper, Sidechain);
            p.setParameterValueNames({"Disabled", "Original", "Sidechain"});
            p.setDefaultValue(0.0);
            data.add(std::move(p));
        }
    }
    
	bool handleModulation(double& max) noexcept
    {
		return modValue.getChangedValue(max);
    }
    
	void prepare(PrepareSpecs ps)
    {
		display_buffer_base<true>::prepare(ps);
        obj.setSampleRate(ps.sampleRate);
    }
    
	void reset() noexcept
    {
        obj.initRuntime();
    }

	void updateModValue(int numSamples)
	{
		if (updateOnFrame)
		{
			auto mv = jlimit(0.0, 1.0, 1.0 - obj.getGainReduction());
			modValue.setModValueIfChanged(mv);
			updateBuffer(mv, numSamples);
		}
	}

	template <typename ProcessDataType> void process(ProcessDataType& data)
	{
		{
			ScopedValueSetter<bool> svs(updateOnFrame, false);
			snex::Types::FrameConverters::forwardToFrameStereo(this, data);
		}
		
		updateModValue(data.getNumSamples());
	}

	template <typename FrameDataType> void processFrame(FrameDataType& data) noexcept
	{
        int numSignalChannels = data.size();
        
        if(sidechainMode != SidechainMode::Disabled)
            numSignalChannels /= 2;
        
        
		if (numSignalChannels == 2)
		{
			double values[2] = { data[0], data[1] };
            
            if(sidechainMode == SidechainMode::Sidechain)
            {
                float sideChainValue = hmath::max(hmath::abs(data[2]), hmath::abs(data[3]));
                obj.process(values[0], values[1], sideChainValue);
            }
            else
            {
                obj.process(values[0], values[1]);
            }
            
			
			data[0] = (float)values[0];
			data[1] = (float)values[1];
		}
		else
		{
			double values[2] = { data[0], data[0] };
            
            if(sidechainMode == SidechainMode::Sidechain)
                obj.process(values[0], values[1], data[1]);
            else
                obj.process(values[0], values[1]);
            
			data[0] = (float)values[0];
		}

		updateModValue(1);
	}

	void setThreshhold(double v)
    {
        obj.setThresh(v);
    }
    
	void setAttack(double v)
    {
        obj.setAttack(v);
    }
    
	void setRelease(double v)
    {
        obj.setRelease(v);
    }
    
	void setRatio(double v)
    {
        auto ratio = (v != 0.0) ? 1.0 / v : 1.0;
        obj.setRatio(ratio);
    }
    
    void setSidechain(double newMode)
    {
        sidechainMode = (SidechainMode)(int)newMode;
    }

	DynamicProcessorType obj;
	ModValue modValue;
	bool updateOnFrame = true;
	
    
    SidechainMode sidechainMode = SidechainMode::Disabled;
};

template class dynamics_wrapper<chunkware_simple::SimpleGate>;
template class dynamics_wrapper<chunkware_simple::SimpleComp>;
template class dynamics_wrapper<chunkware_simple::SimpleLimit>;

using gate = dynamics_wrapper<chunkware_simple::SimpleGate>;
using comp = dynamics_wrapper<chunkware_simple::SimpleComp>;
using limiter = dynamics_wrapper<chunkware_simple::SimpleLimit>;

    
    
    
    
    
class envelope_follower: public data::display_buffer_base<true>
{
public:

	enum class Parameters
	{
		Attack,
		Release,
		ProcessSignal
	};

	DEFINE_PARAMETERS
	{
		DEF_PARAMETER(Attack, envelope_follower);
		DEF_PARAMETER(Release, envelope_follower);
		DEF_PARAMETER(ProcessSignal, envelope_follower)
	}
    SN_PARAMETER_MEMBER_FUNCTION;

	SN_NODE_ID("envelope_follower");
	SN_GET_SELF_AS_OBJECT(envelope_follower);

    envelope_follower() :
      envelope(20.0, 50.0)
    {
        
    }

	static constexpr bool isNormalisedModulation() { return true; }

	SN_EMPTY_HANDLE_EVENT;
    SN_EMPTY_INITIALISE;

	bool handleModulation(double& v) noexcept 
	{ 
		updateBuffer(modValue.getModValue(), lastNumSamples);
		return modValue.getChangedValue(v); 
	};

	void prepare(PrepareSpecs ps) override
    {
		display_buffer_base<true>::prepare(ps);

        envelope.setSampleRate(ps.sampleRate);
    }
    
	void reset() noexcept
    {
        envelope.reset();
    }

	template <typename ProcessDataType> void process(ProcessDataType& data)
	{
		if (data.getNumChannels() == 1)
			snex::Types::FrameConverters::forwardToFrameMono(this, data);
		if(data.getNumChannels() == 2)
			snex::Types::FrameConverters::forwardToFrameStereo(this, data);

		lastNumSamples = data.getNumSamples();
	}

	template <typename FrameType> void processFrame(FrameType& data)
	{
		float input = 0.0;

		for (auto& s: data)
			input = Math.max(Math.abs(s), input);

		auto output = envelope.calculateValue(input);
		
		if (processSignal)
		{
			for (auto& s : data)
				s = output;
		}

		modValue.setModValue(output);
		lastNumSamples = 1;
	}

	void setAttack(double v)
    {
        envelope.setAttackDouble(v);
    }
    
	void setRelease(double v)
    {
        envelope.setReleaseDouble(v);
    }

	void setProcessSignal(double v)
	{
		processSignal = v > 0.5;
	}

	void createParameters(ParameterDataList& data)
    {
        {
            DEFINE_PARAMETERDATA(envelope_follower, Attack);
            p.setRange({ 0.0, 1000.0, 0.1 });
            p.setSkewForCentre(50.0);
            p.setDefaultValue(20.0);
            
            data.add(std::move(p));
        }
        
        {
            DEFINE_PARAMETERDATA(envelope_follower, Release);
            p.setRange({ 0.0, 1000.0, 0.1 });
            p.setSkewForCentre(50.0);
            p.setDefaultValue(20.0);
            
            data.add(std::move(p));
        }
        
        {
            DEFINE_PARAMETERDATA(envelope_follower, ProcessSignal);
            data.add(std::move(p));
        }
    }

	EnvelopeFollower::AttackRelease envelope;

	ModValue modValue;
	int lastNumSamples = 0;

	hmath Math;
	
	bool processSignal = false;
};

}


}
