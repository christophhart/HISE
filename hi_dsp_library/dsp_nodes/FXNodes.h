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
using namespace snex;
using namespace snex::Types;

namespace fx
{

template <int V> class sampleandhold : public polyphonic_base
{
public:


	enum class Parameters
	{
		Counter
	};

	DEFINE_PARAMETERS
	{
		DEF_PARAMETER(Counter, sampleandhold);
	}
	SN_PARAMETER_MEMBER_FUNCTION;

	static constexpr int NumVoices = V;

	SN_POLY_NODE_ID("sampleandhold");
	SN_GET_SELF_AS_OBJECT(sampleandhold);
	SN_DESCRIPTION("A sample and hold effect node");

	SN_EMPTY_HANDLE_EVENT;

	sampleandhold();

	void initialise(NodeBase* n);
	void prepare(PrepareSpecs ps);

	template <typename ProcessDataType> void process(ProcessDataType& d)
	{
		Data& v = data.get();

		if (v.counter > d.getNumSamples())
		{
			int i = 0;

			for (auto c : d)
            {
                auto b = d.toChannelData(c);
				hmath::vmovs(b, v.currentValues[i++]);
            }

			v.counter -= d.getNumSamples();
		}
		else
			snex::Types::FrameConverters::forwardToFrame16(this, d);
	}

	template <typename FrameDataType> void processFrame(FrameDataType& d)
	{
		auto& v = data.get();
		int i = 0;

		if (v.counter == 0)
		{
			for (auto& s : d)
				v.currentValues[i++] = s;
				
			v.counter = v.factor;
		}
		else
		{
			for (auto& s : d)
				s = v.currentValues[i++];

			v.counter--;
		}
	}


	void reset() noexcept;;

	
	bool handleModulation(double&) noexcept { return false; };
	void createParameters(ParameterDataList& data);

	void setCounter(double value);

private:

	struct Data
	{
		Data()
		{
			clear();
		}

		void clear(int numChannelsToClear = NUM_MAX_CHANNELS)
		{
			currentValues = 0.0f;
			
			for (auto& s : currentValues)
				s = 0.0f;

			counter = 0;
		}

		int factor = 1;
		int counter = 0;
		span<float, NUM_MAX_CHANNELS> currentValues;
	};

	PolyData<Data, NumVoices> data;
	int lastChannelAmount = NUM_MAX_CHANNELS;
};

template <typename T> static void getBitcrushedValue(T& data, float bitDepth, bool bipolar)
{
	const float invStepSize = hmath::pow(2.0f, bitDepth);
	const float stepSize = 1.0f / invStepSize;

    if(bipolar)
    {
        for(auto& s: data)
        {
            if(s > 0.0f)
                s = stepSize * floor(s * invStepSize);
            else
                s = stepSize * ceil(s * invStepSize);
        }
    }
    else
    {
        for(auto& s: data)
        {
            s = (stepSize * ceil(s * invStepSize)) - 0.5 * stepSize;
        }
    }
    
	
}

template <int V> class bitcrush : public polyphonic_base
{
public:

	enum class Parameters
	{
		BitDepth,
        Mode
	};

	DEFINE_PARAMETERS
	{
		DEF_PARAMETER(BitDepth, bitcrush);
        DEF_PARAMETER(Mode, bitcrush);
	}
	SN_PARAMETER_MEMBER_FUNCTION;

	static constexpr int NumVoices = V;

	SN_POLY_NODE_ID("bitcrush");
	SN_GET_SELF_AS_OBJECT(bitcrush);
	SN_DESCRIPTION("A bitcrusher effect node");

	SN_EMPTY_HANDLE_EVENT;

	bitcrush();

	void initialise(NodeBase* n);
	void prepare(PrepareSpecs ps);

	// ======================================================================================================
	template <typename ProcessDataType> void process(ProcessDataType& d)
	{
		for (auto ch : d)
		{
			auto b = d.toChannelData(ch);
			getBitcrushedValue(b, bitDepth.get(), bipolar);
		}
			
	}

	template <typename FrameDataType> void processFrame(FrameDataType& data)
	{
		getBitcrushedValue(data, bitDepth.get(), bipolar);
	}


	void reset() noexcept;;
	bool handleModulation(double&) noexcept;;
	void createParameters(ParameterDataList& data);

	void setBitDepth(double newBitDepth);
    
    void setMode(double newMode);

private:

	PolyData<float, NumVoices> bitDepth;
    bool bipolar = false;
};

template <int NV> class phase_delay : public polyphonic_base
{
public:


	static constexpr int NumVoices = NV;
	using Delays = span<PolyData<AllpassDelay, NumVoices>, 2>;

	enum class Parameters
	{
		Frequency
	};

	DEFINE_PARAMETERS
	{
		DEF_PARAMETER(Frequency, phase_delay);
	}
	SN_PARAMETER_MEMBER_FUNCTION;

	SN_POLY_NODE_ID("phase_delay");
	SN_GET_SELF_AS_OBJECT(phase_delay);
	SN_DESCRIPTION("A phase delay for comb filtering");
	SN_EMPTY_HANDLE_EVENT;

	phase_delay();

	void initialise(NodeBase* n);
	void prepare(PrepareSpecs ps);

	template <typename ProcessDataType> void process(ProcessDataType& data)
	{
		index::clamped<2> i;

		for (auto ch : data)
		{
			auto& dl = delays[i++].get();

			for (auto& s : data.toChannelData(ch))
				s = dl.getNextSample(s);
		}
	}

	template <typename FrameDataType> void processFrame(FrameDataType& data)
	{
		index::clamped<2> i;

		for (auto& s : data)
			s = delays[i++].get().getNextSample(s);
	}

	void reset() noexcept;;
	
	bool handleModulation(double&) noexcept;;
	void createParameters(ParameterDataList& data);

	void setFrequency(double frequency);

	Delays delays;
	double sr = 44100.0;
};

class reverb : public HiseDspBase
{
public:

	enum class Parameters
	{
		Damping,
		Width,
		Size,
		numParameters
	};

	DEFINE_PARAMETERS
	{
		DEF_PARAMETER(Damping, reverb);
		DEF_PARAMETER(Width, reverb);
		DEF_PARAMETER(Size, reverb);
	}
	SN_PARAMETER_MEMBER_FUNCTION;

	SN_NODE_ID("reverb");
	SN_GET_SELF_AS_OBJECT(reverb);
	SN_DESCRIPTION("The default JUCE reverb implementation");

	bool isPolyphonic() const { return false; }

	SN_EMPTY_HANDLE_EVENT;

	reverb();

	void initialise(NodeBase* n);
	void prepare(PrepareSpecs ps);

	template <typename ProcessDataType> void process(ProcessDataType& d)
	{
		if (d.getNumChannels() == 1)
			r.processMono(d[0].data, d.getNumSamples());
		else
			r.processStereo(d[0].data, d[1].data, d.getNumSamples());
	}

	template <typename FrameDataType> void processFrame(FrameDataType& d)
	{
		if (d.size() == 1)
			r.processMono(d.begin(), 1);
		else
			r.processStereo(d.begin(), d.begin() + 1, 1);
	}

	void reset() noexcept;;
	void createParameters(ParameterDataList& data) override;

	void setDamping(double newDamping);
	void setWidth(double width);
	void setSize(double size);

private:

	juce::Reverb r;

};


template <int V> class haas : public HiseDspBase,
							  public polyphonic_base
{
public:

	static const int NumChannels = 2;
	using DelayType = std::array<DelayLine<2048, DummyCriticalSection>, NumChannels>;
	using FrameType = span<float, NumChannels>;
	using ProcessType = snex::Types::ProcessData<NumChannels>;

	static constexpr int NumVoices = V;

	enum class Parameters
	{
		Position
	};

	haas() : polyphonic_base(haas::getStaticId(), false) {}

	DEFINE_PARAMETERS
	{
		DEF_PARAMETER(Position, haas);
	}
    SN_PARAMETER_MEMBER_FUNCTION;

	SN_POLY_NODE_ID("haas");
	SN_GET_SELF_AS_OBJECT(haas);
	SN_DESCRIPTION("A Haas effect (simulate stereo position using delay)");
	SN_EMPTY_HANDLE_EVENT;

	void createParameters(ParameterDataList& data) override;
	void prepare(PrepareSpecs ps);
	void reset();

	void processFrame(FrameType& data);

	void process(ProcessType& d);

	bool handleModulation(double&);
	void setPosition(double newValue);

	double position = 0.0;
	PolyData<DelayType, NumVoices> delay;
};

}

}
