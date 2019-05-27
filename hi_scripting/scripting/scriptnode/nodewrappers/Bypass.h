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
template <class T, bool AddAsParameter=true> class smoothed
{
public:

	static constexpr int ExtraHeight = T::ExtraHeight;

	int getExtraWidth() const { return obj.getExtraWidth(); }
	static constexpr bool isModulationSource = T::isModulationSource;

	void initialise(NodeBase* n)
	{
		obj.initialise(n);
	}

	forcedinline void process(ProcessData& data) noexcept
	{
		if (shouldSmoothBypass())
		{
			for (int c = 0; c < data.numChannels; c++)
				wetBuffer.copyFrom(c, 0, data.data[c], data.size);

			memcpy(stackChannels, wetBuffer.getArrayOfWritePointers(), data.numChannels * sizeof(float*));

			ProcessData stackData(stackChannels, data.numChannels, data.size);

			obj.process(stackData);

			float wet[NUM_MAX_CHANNELS];
			float dry[NUM_MAX_CHANNELS];

			for (int i = 0; i < data.size; i++)
			{
				data.copyToFrameDynamic(dry);
				stackData.copyToFrameDynamic(wet);

				auto rampValue = bypassRamper.getNextValue();
				auto invRampValue = 1.0f - rampValue;

				FloatVectorOperations::multiply(dry, invRampValue, data.numChannels);
				FloatVectorOperations::multiply(wet, rampValue, data.numChannels);
				FloatVectorOperations::add(dry, wet, data.numChannels);

				data.copyFromFrameAndAdvanceDynamic(dry);
				stackData.advanceChannelPointers();
			}
		}
		else
		{
			if (bypassed)
				return;

			obj.process(data);
		}
	}

	forcedinline void processSingle(float* frameData, int numChannels) noexcept
	{
		if (shouldSmoothBypass())
		{
			FloatVectorOperations::copy(wetFrameData, frameData, numChannels);

			obj.processSingle(wetFrameData, numChannels);

			auto rampValue = bypassRamper.getNextValue();
			auto invRampValue = 1.0f - rampValue;

			FloatVectorOperations::multiply(frameData, invRampValue, numChannels);
			FloatVectorOperations::addWithMultiply(frameData, wetFrameData, rampValue, numChannels);
		}
		else
		{
			if (bypassed)
				return;

			obj.processSingle(frameData, numChannels);
		}
	}

	bool shouldSmoothBypass() const { return bypassRamper.isSmoothing(); }

	void prepare(int numChannels, double sampleRate, int samplesPerBlock)
	{
		sr = sampleRate;
		bypassRamper.reset(sr, smoothTimeMs * 0.001);

		DspHelpers::increaseBuffer(wetBuffer, numChannels, samplesPerBlock);

		obj.prepare(numChannels, sampleRate, samplesPerBlock);
	}

	void reset() { obj.reset(); }

	constexpr bool allowsModulation()
	{
		return obj.isModulationSource;
	}

	forcedinline bool handleModulation(double& value) noexcept
	{
		if (!bypassed)
			return obj.handleModulation(value);

		return false;
	}

	void setBypassedFromRange(double value)
	{
		setBypassed(value > 0.5);
	}

	void setBypassed(bool shouldBeBypassed)
	{
		bypassed = shouldBeBypassed;
		bypassRamper.setTargetValue(bypassed ? 0.0 : 1.0);
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

	void createParameters(Array<HiseDspBase::ParameterData>& data)
	{
		if (AddAsParameter)
		{
			HiseDspBase::ParameterData p("Bypassed");

			using ThisClass = smoothed<T, AddAsParameter>;

			p.defaultValue = 0.0;
			p.db = BIND_MEMBER_FUNCTION_1(ThisClass::setBypassedFromRange);

			data.add(std::move(p));

		}
		
		obj.createParameters(data);
	}

	Component* createExtraComponent(PooledUIUpdater* updater)
	{
		return obj.createExtraComponent(updater);
	}

	auto& getObject() { return *this; }
	const auto& getObject() const { return *this; }

private:

	T obj;

	AudioSampleBuffer wetBuffer;

	float* stackChannels[NUM_MAX_CHANNELS];
	float wetFrameData[NUM_MAX_CHANNELS];

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

	void prepare(int numChannels, double sampleRate, int blockSize)
	{
		obj.prepare(numChannels, sampleRate, blockSize);
	}

	forcedinline void reset() noexcept { obj.reset(); }

	forcedinline bool handleModulation(double& value) noexcept
	{
		return obj.handleModulation(value);
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

template <class T> class yes
{
public:

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

	constexpr bool allowsModulation()
	{
		return obj.isModulationSource;
	}

	void prepare(int numChannels, double sampleRate, int blockSize)
	{
		obj.prepare(numChannels, sampleRate, blockSize);
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

}
}