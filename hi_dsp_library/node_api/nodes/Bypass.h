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
using namespace snex::Types;

namespace bypass
{

static constexpr int ParameterId = 9000;

template <class T> class smoothed: public SingleWrapper<T>
{
public:

	SN_SELF_AWARE_WRAPPER(smoothed, T);

	template <typename ProcessDataType> void process(ProcessDataType& data)
	{
		if (shouldSmoothBypass())
			FrameConverters::forwardToFrame16(this, data);
		else if(!bypassed)
			this->obj.process(data);
	}

	void processFrame(snex::Types::dyn<float>& data) noexcept
	{
		FrameConverters::forwardToFixFrame16(this, data);
	}

	template <int P> static void setParameter(void* obj, double v)
	{
		auto thisPointer = static_cast<smoothed*>(obj);

		if (P == ParameterId)
			thisPointer->setBypassed(v > 0.5);
		else
			T::ObjectType::setParameterStatic<P>(&thisPointer->obj, v);
	}

	template <typename FrameDataType> void processFrame(FrameDataType& data) noexcept
	{
		if (shouldSmoothBypass())
		{
			FrameDataType wet = data;

			this->obj.processFrame(wet);

			const auto rampValue = ramper.advance();
			const auto invRampValue = 1.0f - rampValue;
			data *= invRampValue;
			wet *= rampValue;
			data += wet;
		}
		else
		{
			if(!bypassed)
				this->obj.processFrame(data);
		}
	}

	bool shouldSmoothBypass() const { return ramper.isActive(); }

	void prepare(PrepareSpecs ps)
	{
		ramper.prepare(ps.sampleRate, 20.0);
		ramper.set(bypassed ? 1.0f : 0.0f);
		ramper.reset();

		this->obj.prepare(ps);
	}

	void reset()
	{ 
		if (!bypassed)
		{
			ramper.reset();
			this->obj.reset();
		}
	}

	bool handleModulation(double& value) noexcept
	{
		if (!bypassed)
			return this->obj.handleModulation(value);

		return false;
	}

	void createParameters(ParameterDataList& data) override
	{
		this->obj.createParameters(data);
	}

private:

	void setBypassed(bool shouldBeBypassed)
	{
		bypassed = shouldBeBypassed;
		ramper.set(bypassed ? 0.0f : 1.0f);
	}

	sfloat ramper;
	bool bypassed = false;
};

#if 0
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
#endif


template <class T> class simple : public SingleWrapper<T>
{
public:

	SN_SELF_AWARE_WRAPPER(simple, T);

	template <typename ProcessDataType> void process(ProcessDataType& data) noexcept
	{
		if (!bypassed)
			this->obj.process(data);
	}

	template <typename FrameDataType> void processFrame(FrameDataType& data) noexcept
	{
		if(!bypassed)
			this->obj.processFrame(data);
	}

	template <int P> static void setParameter(void* obj, double v)
	{
		auto thisPointer = static_cast<simple*>(obj);

		if (P == ParameterId)
			thisPointer->setBypassed(v > 0.5);
		else
			T::ObjectType::setParameter<P>(&thisPointer->obj, v);
	}

	void prepare(PrepareSpecs ps)
	{
		this->obj.prepare(ps);
	}

	void reset() 
	{ 
		if(!bypassed)
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

		if (bypassed)
			reset();
	}

	void setBypassedFromValueTreeCallback(Identifier id, var newValue)
	{
		if (id == PropertyIds::Bypassed)
			setBypassed((bool)newValue);
	}

	void createParameters(ParameterDataList& data) override
	{
		this->obj.createParameters(data);
	}

private:

	bool bypassed = false;
};

}
}
