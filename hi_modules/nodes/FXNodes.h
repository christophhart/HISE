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

namespace fx
{

template <int V> class sampleandhold_impl : public HiseDspBase
{
public:

	static constexpr int NumVoices = V;

	SET_HISE_NODE_EXTRA_HEIGHT(0);
	SET_HISE_POLY_NODE_ID("sampleandhold");
	GET_SELF_AS_OBJECT(sampleandhold_impl);
	SET_HISE_NODE_IS_MODULATION_SOURCE(false);

	sampleandhold_impl();

	void initialise(NodeBase* n);
	void prepare(PrepareSpecs ps);
	void process(ProcessData& d);
	void reset() noexcept;;
	void processSingle(float* numFrames, int numChannels);
	bool handleModulation(double&) noexcept { return false; };
	void createParameters(Array<ParameterData>& data) override;

	void setFactor(double value);

private:

	struct Data
	{
		Data()
		{
			clear();
		}

		void clear(int numChannelsToClear = NUM_MAX_CHANNELS)
		{
			memset(currentValues, 0, sizeof(float) * numChannelsToClear);
			counter = 0;
		}

		int factor = 1;
		int counter = 0;
		float currentValues[NUM_MAX_CHANNELS];
	};

	PolyData<Data, NumVoices> data;
	int lastChannelAmount = NUM_MAX_CHANNELS;
};

DEFINE_EXTERN_NODE_TEMPLATE(sampleandhold, sampleandhold_poly, sampleandhold_impl);

template <int V> class bitcrush_impl : public HiseDspBase
{
public:

	static constexpr int NumVoices = V;

	SET_HISE_NODE_EXTRA_HEIGHT(0);
	SET_HISE_POLY_NODE_ID("bitcrush");
	GET_SELF_AS_OBJECT(bitcrush_impl);
	SET_HISE_NODE_IS_MODULATION_SOURCE(false);

	bitcrush_impl();

	void initialise(NodeBase* n);
	void prepare(PrepareSpecs ps);
	void process(ProcessData& d);
	void reset() noexcept;;
	void processSingle(float* numFrames, int numChannels);
	bool handleModulation(double&) noexcept;;
	void createParameters(Array<ParameterData>& data) override;

	void setBitDepth(double newBitDepth);

private:

	PolyData<float, NumVoices> bitDepth;

};

template <int V> class phase_delay_impl : public HiseDspBase
{
public:

	static constexpr int NumVoices = V;

	SET_HISE_NODE_EXTRA_HEIGHT(0);
	SET_HISE_POLY_NODE_ID("phase_delay");
	GET_SELF_AS_OBJECT(phase_delay_impl);
	SET_HISE_NODE_IS_MODULATION_SOURCE(false);

	phase_delay_impl();

	void initialise(NodeBase* n);
	void prepare(PrepareSpecs ps);
	void process(ProcessData& d);
	void reset() noexcept;;
	void processSingle(float* numFrames, int numChannels);
	bool handleModulation(double&) noexcept;;
	void createParameters(Array<ParameterData>& data) override;

	void setFrequency(double frequency);

	PolyData<AllpassDelay, NumVoices> delays[2];
	double sr = 44100.0;
};

DEFINE_EXTERN_NODE_TEMPLATE(phase_delay, phase_delay_poly, phase_delay_impl);

DEFINE_EXTERN_NODE_TEMPLATE(bitcrush, bitcrush_poly, bitcrush_impl);

class reverb : public HiseDspBase
{
public:

	SET_HISE_NODE_EXTRA_HEIGHT(0);
	SET_HISE_NODE_ID("reverb");
	GET_SELF_AS_OBJECT(reverb);
	SET_HISE_NODE_IS_MODULATION_SOURCE(false);

	reverb();

	void initialise(NodeBase* n);
	void prepare(PrepareSpecs ps);
	void process(ProcessData& d);
	void reset() noexcept;;
	void processSingle(float* numFrames, int numChannels);
	bool handleModulation(double&) noexcept;;
	void createParameters(Array<ParameterData>& data) override;

	void setDamping(double newDamping);
	void setWidth(double width);
	void setSize(double size);

private:

	juce::Reverb r;

};


template <int V> class haas_impl : public HiseDspBase
{
public:

	using DelayType = DelayLine<2048, DummyCriticalSection>;

	static constexpr int NumVoices = V;

	SET_HISE_NODE_EXTRA_HEIGHT(0);
	SET_HISE_POLY_NODE_ID("haas");
	GET_SELF_AS_OBJECT(haas_impl);
	SET_HISE_NODE_IS_MODULATION_SOURCE(false);

	void createParameters(Array<ParameterData>& data) override;
	void prepare(PrepareSpecs ps);
	void reset();
	void processSingle(float* data, int numChannels);
	void process(ProcessData& d);
	bool handleModulation(double&);
	void setPosition(double newValue);

	double position = 0.0;
	PolyData<DelayType, NumVoices> delayL;
	PolyData<DelayType, NumVoices> delayR;
};

DEFINE_EXTERN_NODE_TEMPLATE(haas, haas_poly, haas_impl);

}

}
