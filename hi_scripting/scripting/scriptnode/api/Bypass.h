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

	GET_SELF_OBJECT(*this);
	GET_WRAPPED_OBJECT(obj.getWrappedObject());

	template <typename ProcessDataType> void process(ProcessDataType& data) noexcept
	{
		if (shouldSmoothBypass())
			DspHelpers::forwardToFrame16(this, data);
		else if(!bypassed)
			this->obj.process(data);
	}

	void processFrame(snex::Types::dyn<float>& data) noexcept
	{
		DspHelpers::forwardToFixFrame16(this, data);
	}

	template <int N> void processFrame(snex::Types::span<float, N>& data) noexcept
	{
		if (shouldSmoothBypass())
		{
			snex::Types::span<float, N> wet;

			int index = 0;

			for (auto& s : data)
				wet[index++] = s;

			this->obj.processFrame(wet);

			auto rampValue = bypassRamper.getNextValue();
			auto invRampValue = 1.0f - rampValue;

			index = 0;

			for (auto& s : data)
				s = s * invRampValue + rampValue * wet[index++];
		}
		else
		{
			if(!bypassed)
				this->obj.processFrame(data);
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

	void reset()
	{ 
		bypassRamper.setValueWithoutSmoothing(bypassRamper.getTargetValue());
		this->obj.reset(); 
	}

	forcedinline bool handleModulation(double& value) noexcept
	{
		if (!bypassed)
			return this->obj.handleModulation(value);

		return false;
	}

	static void setBypassed(void* obj, double value)
	{
		static_cast<smoothed*>(obj)->setBypassed(value > 0.5);
	}

	void setBypassed(bool shouldBeBypassed)
	{
		bypassed = shouldBeBypassed;
		bypassRamper.setTargetValue(bypassed ? 0.0f : 1.0f);
	}

	void createParameters(Array<HiseDspBase::ParameterData>& data) override
	{
		this->obj.createParameters(data);
	}

private:

	ValueTree data;
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
		this->obj.initialise(n);
	}

	template <typename ProcessDataType> void process(ProcessDataType& data) noexcept
	{
		this->obj.process(data);
	}

	template <typename FrameDataType> void processFrame(FrameDataType& data) noexcept
	{
		this->obj.processFrame(data);
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
		this->obj.handleHiseEvent(e);
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

	template <typename ProcessDataType> void process(ProcessDataType& data) noexcept
	{
		if (!bypassed)
			this->obj.process(data);
	}

	template <typename FrameDataType> void processFrame(FrameDataType& data) noexcept
	{
		this->obj.processFrame(data);
	}

	void process(ProcessData& data) noexcept
	{
		if (!bypassed)
			this->obj.process(data);
	}

	void processFrame(float* frameData, int numChannels) noexcept
	{
		if (bypassed)
			return;

		this->obj.processFrame(frameData, numChannels);
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

}
}
