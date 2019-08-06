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

namespace bypass
{
template <class T, bool AddAsParameter=true> class smoothed: public SingleWrapper<T>
{
public:

	GET_SELF_AS_OBJECT(smoothed);


	static constexpr bool isModulationSource = T::isModulationSource;

	forcedinline void process(ProcessData& data) noexcept
	{
		if (shouldSmoothBypass())
		{

			for (int c = 0; c < data.numChannels; c++)
				wetBuffer.copyFrom(c, 0, data.data[c], data.size);

			float* wetDataPointers[NUM_MAX_CHANNELS];
			float* dryDataPointers[NUM_MAX_CHANNELS];

			auto bytesToCopy = data.numChannels * sizeof(float*);

			memcpy(wetDataPointers, wetBuffer.getArrayOfWritePointers(), bytesToCopy);
			memcpy(dryDataPointers, data.data, bytesToCopy);

			ProcessData wetData(wetDataPointers, data.numChannels, data.size);
			ProcessData dryData(dryDataPointers, data.numChannels, data.size);
			
			this->obj.process(wetData);

			float wet[NUM_MAX_CHANNELS];
			float dry[NUM_MAX_CHANNELS];

			wetData.allowPointerModification();
			dryData.allowPointerModification();

			for (int i = 0; i < data.size; i++)
			{
				wetData.copyToFrameDynamic(wet);
				dryData.copyToFrameDynamic(dry);

				auto rampValue = bypassRamper.getNextValue();
				auto invRampValue = 1.0f - rampValue;

				FloatVectorOperations::multiply(dry, invRampValue, data.numChannels);
				FloatVectorOperations::multiply(wet, rampValue, data.numChannels);
				FloatVectorOperations::add(dry, wet, data.numChannels);

				dryData.copyFromFrameAndAdvanceDynamic(dry);
				wetData.advanceChannelPointers();
			}

		}
		else
		{
			if (bypassed)
				return;

			this->obj.process(data);
		}
	}

	forcedinline void processSingle(float* frameData, int numChannels) noexcept
	{
		if (shouldSmoothBypass())
		{
			float wet[NUM_MAX_CHANNELS];

			FloatVectorOperations::copy(wet, frameData, numChannels);

			this->obj.processSingle(wet, numChannels);

			auto rampValue = bypassRamper.getNextValue();
			auto invRampValue = 1.0f - rampValue;

			FloatVectorOperations::multiply(frameData, invRampValue, numChannels);
			FloatVectorOperations::addWithMultiply(frameData, wet, rampValue, numChannels);
		}
		else
		{
			if (bypassed)
				return;

			this->obj.processSingle(frameData, numChannels);
		}
	}

	bool shouldSmoothBypass() const { return bypassRamper.isSmoothing(); }

	void prepare(PrepareSpecs ps)
	{
		sr = ps.sampleRate;
		bypassRamper.reset(sr, smoothTimeMs * 0.001);
		DspHelpers::increaseBuffer(wetBuffer, ps);
		this->obj.prepare(ps);
	}

	void reset() { this->obj.reset(); }

	constexpr bool allowsModulation()
	{
		return this->obj.isModulationSource;
	}

	forcedinline bool handleModulation(double& value) noexcept
	{
		if (!bypassed)
			return this->obj.handleModulation(value);

		return false;
	}

	void setBypassedFromRange(double value)
	{
		setBypassed(value > 0.5);
	}

	void setBypassed(bool shouldBeBypassed)
	{
		bypassed = shouldBeBypassed;
		bypassRamper.setTargetValue(bypassed ? 0.0f : 1.0f);
	}

	void setBypassedFromValueTreeCallback(Identifier id, var newValue)
	{
		if (id == PropertyIds::Bypassed)
			setBypassed((bool)newValue);
		else
		{
			smoothTimeMs = jlimit(0.0, 1000.0, (double)newValue);
			bypassRamper.reset(sr, smoothTimeMs * 0.001);
		}
	}

	void createParameters(Array<HiseDspBase::ParameterData>& data) override
	{
		if (AddAsParameter)
		{
			HiseDspBase::ParameterData p("Bypassed");

			using ThisClass = smoothed<T, AddAsParameter>;

			p.defaultValue = 0.0;
			p.db = BIND_MEMBER_FUNCTION_1(ThisClass::setBypassedFromRange);

			data.add(std::move(p));

		}
		
		this->obj.createParameters(data);
	}

private:

	AudioSampleBuffer wetBuffer;

	LinearSmoothedValue<float> bypassRamper;
	double sr = 44100.0;
	double smoothTimeMs = 20.0;
	bool bypassed = false;
};

template <class T> class no
{
public:


	void initialise(NodeBase* n)
	{
		obj.initialise(n);
	}

	forcedinline void process(ProcessData& data) noexcept
	{
		obj.process(data);
	}

	forcedinline void processSingle(float* frameData, int numChannels) noexcept
	{
		obj.processSingle(frameData, numChannels);
	}

	constexpr bool allowsModulation()
	{
		return obj.isModulationSource;
	}

	void prepare(PrepareSpecs ps)
	{
		obj.prepare(ps);
	}

	forcedinline void reset() noexcept { obj.reset(); }

	forcedinline bool handleModulation(double& value) noexcept
	{
		return obj.handleModulation(value);
	}

	void handleHiseEvent(HiseEvent& e)
	{
		obj.handleHiseEvent(e);
	}

	void setBypassedFromRange(double value)
	{

	}

	void setBypassed(bool shouldBeBypassed)
	{

	}

	void setBypassedFromValueTreeCallback(Identifier id, var newValue)
	{

	}

	auto& getObject() { return obj.getObject(); }
	const auto& getObject() const { return obj.getObject(); }

private:

	T obj;
};



template <class T, bool AddAsParameter = true> class yes : public SingleWrapper<T>
{
public:

	GET_SELF_AS_OBJECT(yes);

	

	static constexpr bool isModulationSource = T::isModulationSource;

	void process(ProcessData& data) noexcept
	{
		if (bypassed)
			return;

		this->obj.process(data);
	}

	void processSingle(float* frameData, int numChannels) noexcept
	{
		if (bypassed)
			return;

		this->obj.processSingle(frameData, numChannels);
	}

	void prepare(PrepareSpecs ps)
	{
		this->obj.prepare(ps);
	}

	void reset() 
	{ 
		if(bypassed)
			this->obj.reset(); 
	}

	constexpr bool allowsModulation()
	{
		return this->obj.isModulationSource;
	}

	bool handleModulation(double& value) noexcept
	{
		if (!bypassed)
			return this->obj.handleModulation(value);

		return false;
	}

	void setBypassedFromRange(double value)
	{
		setBypassed(value > 0.5);
	}

	void setBypassed(bool shouldBeBypassed)
	{
		bypassed = shouldBeBypassed;
	}

	void setBypassedFromValueTreeCallback(Identifier id, var newValue)
	{
		if (id == PropertyIds::Bypassed)
			setBypassed((bool)newValue);
	}

	void createParameters(Array<HiseDspBase::ParameterData>& data) override
	{
		if (AddAsParameter)
		{
			HiseDspBase::ParameterData p("Bypassed");

			using ThisClass = yes<T, AddAsParameter>;

			p.defaultValue = 0.0;
			p.db = BIND_MEMBER_FUNCTION_1(ThisClass::setBypassedFromRange);

			data.add(std::move(p));

		}

		this->obj.createParameters(data);
	}

private:

	bool bypassed = false;
};


#if 0
template <class T, bool AddAsParameter> class yes: public SingleWrapper<T>
{
public:

	GET_SELF_AS_OBJECT(yes);

	void initialise(NodeBase* n)
	{
		obj.initialise(n);
	}

	forcedinline void process(ProcessData& data) noexcept
	{
		if (!bypassed)
			obj.process(data);
	}

	forcedinline void processSingle(float* frameData, int numChannels) noexcept
	{
		if (!bypassed)
			obj.processSingle(frameData, numChannels);
	}

	void handleHiseEvent(HiseEvent& e)
	{
		if(!bypassed)
			obj.handleHiseEvent(e);
	}

	constexpr bool allowsModulation()
	{
		return obj.isModulationSource;
	}

	void prepare(PrepareSpecs ps)
	{
		obj.prepare(ps);
	}

	forcedinline void reset() noexcept { obj.reset(); }

	forcedinline bool handleModulation(double& value) noexcept
	{
		if (!bypassed)
			return obj.handleModulation(value);

		return false;
	}

	void setBypassedFromRange(double value)
	{
		setBypassed(activeRange.contains(value));
	}

	void setBypassed(bool shouldBeBypassed)
	{
		bypassed = shouldBeBypassed;
	}

	void setBypassedFromValueTreeCallback(Identifier id, var newValue)
	{
		if (id == PropertyIds::Bypassed)
			bypassed = (bool)newValue;
	}

	auto& getObject() { return obj.getObject(); }
	const auto& getObject() const { return obj.getObject(); }

	Range<double> activeRange;
	std::atomic<bool> bypassed;

private:

	T obj;
};
#endif

}
}
