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

template <int SmoothingTime, class T> class smoothed: public SingleWrapper<T>
{
public:

	SN_SELF_AWARE_WRAPPER(smoothed, T);

	smoothed()
	{
		smoothingTime = SmoothingTime;

		if (smoothingTime == -1)
			smoothingTime = 20;
	}

	constexpr OPTIONAL_BOOL_CLASS_FUNCTION(isProcessingHiseEvent);
	constexpr OPTIONAL_BOOL_CLASS_FUNCTION(isPolyphonic);

	template <typename ProcessDataType> void process(ProcessDataType& data)
	{
		if (shouldSmoothBypass())
		{
			const int numChannels = data.getNumChannels();
			const int numSamples = data.getNumSamples();

			auto rampData = (float*)alloca(sizeof(float) * numSamples);

			for (int i = 0; i < numSamples; i++)
			{
				rampData[i] = jlimit(0.0f, 1.0f, ramper.advance());
			}

			auto stackBufferData = (float*)alloca(sizeof(float) * numChannels * numSamples);
			float* channels[NUM_MAX_CHANNELS];

			for (int i = 0; i < numChannels; i++)
			{
				channels[i] = stackBufferData + i * numSamples;

				FloatVectorOperations::copy(channels[i], data[i].begin(), numSamples);
				FloatVectorOperations::multiply(channels[i], rampData, numSamples);
			}

			ProcessDataType wetData(channels, numSamples, numChannels);
			wetData.copyNonAudioDataFrom(data);
			
			this->obj.process(wetData);

			for (int i = 0; i < numSamples; i++)
			{
				const auto rampValue = rampData[i];
				const auto invRampValue = 1.0f - rampValue;

				for (int c = 0; c < numChannels; c++)
				{
					auto& dry = data[c][i];
					auto& wet = wetData[c][i];

					dry *= invRampValue; 
					wet *= rampValue;
					dry += wet;
				}
			}
		}
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

        using ObjectTypeT = typename T::ObjectType;
        
		if constexpr (P == ParameterId)
			thisPointer->setBypassed(v > 0.5);
		else
			ObjectTypeT::template setParameterStatic<P>(&thisPointer->obj, v);
	}

	template <typename FrameDataType> void processFrame(FrameDataType& data) noexcept
	{
		if (shouldSmoothBypass())
		{
			const auto rampValue = ramper.advance();
			const auto invRampValue = 1.0f - rampValue;

			FrameDataType wet = data;

			wet *= rampValue;

			this->obj.processFrame(wet);

			
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
		sr = ps.sampleRate;
		ramper.prepare(ps.sampleRate, (double)smoothingTime);
		ramper.set(bypassed ? 0.0f : 1.0f);
		ramper.reset();

		this->obj.prepare(ps);
	}

	void reset()
	{ 
        ramper.reset();
        this->obj.reset();
	}

	bool handleModulation(double& value) noexcept
	{
		if constexpr (prototypes::check::handleModulation<T>::value)
		{
			if (!bypassed)
				return this->obj.handleModulation(value);
		}
		
		return false;
	}

	void createParameters(ParameterDataList& data) override
	{
		this->obj.createParameters(data);
	}

	template <int Index> auto& get()
	{
		return this->obj.template get<Index>();
	}

	void setExternalData(const ExternalData& d, int index)
	{
		this->obj.setExternalData(d, index);
	}

	void setSmoothingTime(int newTime)
	{
		if constexpr (SmoothingTime == -1)
		{
			smoothingTime = jlimit(0, 1000, newTime);
            
            if (sr <= 0.0)
                return;
            
			ramper.prepare(sr, smoothingTime);
			ramper.set(bypassed ? 0.0f : 1.0f);
			ramper.reset();
		}
	}



	void setBypassed(bool shouldBeBypassed)
	{
		if (bypassed != shouldBeBypassed)
		{
			bypassed = shouldBeBypassed;
			ramper.set(bypassed ? 0.0f : 1.0f);

			if (!shouldBeBypassed)
				this->obj.reset();
		}
	}

	private:

	double sr = 0.0;
	int smoothingTime;
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

	constexpr OPTIONAL_BOOL_CLASS_FUNCTION(isProcessingHiseEvent);
	constexpr OPTIONAL_BOOL_CLASS_FUNCTION(isPolyphonic);

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

		if constexpr (P == ParameterId)
			thisPointer->setBypassed(v > 0.5);
		else
			T::ObjectType::template setParameterStatic<P>(&thisPointer->obj, v);
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
