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
 *   which also must be licensed for commercial applications:
 *
 *   http://www.juce.com
 *
 *   ===========================================================================
 */

#pragma once

namespace scriptnode {

namespace jdsp
{

/** This namespace contains interface classes that can be used to convert DSP classes that follow the JUCE::dsp Processor prototype. */
namespace base
{

/** The base class for wrapping a juce::dsp::Processor into a scriptnode node. */
template <typename T, int NV> struct jwrapper
{
	using JuceDspType = T;
	using JuceBaseType = jwrapper<T, NV>;
	using JuceProcessType = juce::dsp::ProcessContextReplacing<float>;

	static constexpr int NumVoices = NV;
	static constexpr bool isPolyphonic() { return NumVoices > 1; };

	virtual ~jwrapper() {};

	void reset()
	{
		for(auto& obj: objects)
			obj.reset();
	}

	virtual void prepare(PrepareSpecs ps)
	{
		juce::dsp::ProcessSpec jps;

		jps.maximumBlockSize = ps.blockSize;
		jps.numChannels = ps.numChannels;
		jps.sampleRate = ps.sampleRate;

		objects.prepare(ps);

		for(auto& obj: objects)
			obj.prepare(jps);
	}

	SN_EMPTY_HANDLE_EVENT;
	SN_EMPTY_INITIALISE;
	
	template <typename ProcessDataType> void process(ProcessDataType& data)
	{
		juce::dsp::AudioBlock<float> b(data.getRawChannelPointers(), data.getNumChannels(), data.getNumSamples());
		objects.get().process(JuceProcessType(b));
	}

	template <typename FrameDataType> void processFrame(FrameDataType& data)
	{
		auto& obj = objects.get();

		if constexpr (prototypes::check::processSample<JuceDspType>::value)
		{
			for (int i = 0; i < data.size(); i++)
				data[i] = obj.processSample(i, data[i]);
		}
		else
		{
			float** ptr = (float**)alloca(data.size() * sizeof(float*));

			for (int i = 0; i < data.size(); i++)
				ptr[i] = data.begin() + i;

			juce::dsp::AudioBlock<float> b(ptr, data.size(), 1);
			obj.process(JuceProcessType(b));
		}
	}

	/** Override this function and create a list of all parameters. */
	virtual void createParameters(ParameterDataList& d) = 0;

protected:

	PolyData<T, NumVoices> objects;
};

/** This template can be used in order to implement a modulation source node from a JUCE processor. 
	Subclass from this interface class, then implement a handleModulation method that creates a modulation value
	and sends it to the display buffer.
	
	Take a look at eg. the jcompressor class for an example implementation. 
*/
template <bool NormalisedModulation=true> struct jmod: public data::display_buffer_base<true>
{
	jmod(const Identifier& id)
	{
		if(!NormalisedModulation)
			cppgen::CustomNodeProperties::addNodeIdManually(id, PropertyIds::UseUnnormalisedModulation);
	}

	virtual ~jmod() {};

	static constexpr bool isNormalisedModulation() { return NormalisedModulation; };

	void prepare(PrepareSpecs ps) override
	{
		lastNumSamples = ps.blockSize;
		data::display_buffer_base<true>::prepare(ps);
	}

	void sendModValue(double v)
	{
		this->updateBuffer(v, lastNumSamples);
	}

	int lastNumSamples = 0;
};
}


struct jcompressor : public jdsp::base::jwrapper<juce::dsp::Compressor<float>, 1>,
					 public jdsp::base::jmod<true>
{
	jcompressor() :
		jmod<true>(getStaticId())
	{};

	SNEX_NODE(jcompressor);

	template <int P> void setParameter(double v)
	{
		for (auto& obj : this->objects)
		{
			if constexpr (P == 0)
				obj.setThreshold(v);
			if constexpr (P == 1)
				obj.setRatio(jmax(1.0, v));
			if constexpr (P == 2)
				obj.setAttack(v);
			if constexpr (P == 3)
				obj.setRelease(v);
		}
	}

	void prepare(PrepareSpecs ps) override
	{
		JuceBaseType::prepare(ps);
		jdsp::base::jmod<true>::prepare(ps);
	}

	bool handleModulation(double& v)
	{
		v = jlimit<double>(0.0, 1.0, 1.0 - (double)this->objects.get().getGainReduction());
		this->sendModValue(v);
		return true;
	}

	void createParameters(ParameterDataList& d) override;
};

struct jlinkwitzriley : public base::jwrapper<juce::dsp::LinkwitzRileyFilter<float>, 1>,
						public data::filter_base,
						public hise::ComplexDataUIUpdaterBase::EventListener
{
	SNEX_NODE(jlinkwitzriley);

	void prepare(PrepareSpecs ps)
	{
		sr = ps.sampleRate;
		JuceBaseType::prepare(ps);
	}

	template <int P> void setParameter(double v)
	{
		for (auto& o : objects)
		{
			if (P == 0)
			{
				if(std::isfinite(v) && v > 20.0)
					o.setCutoffFrequency(v);
			}
			if (P == 1)
				o.setType((JuceDspType::Type)(int)v);
		}

		sendCoefficientUpdateMessage();
	}

	IIRCoefficients getApproximateCoefficients() const override
	{
        if(sr == 0)
            return {};
        
		auto& o = objects.getFirst();

		switch (o.getType())
		{
		case JuceDspType::Type::allpass: return IIRCoefficients::makeAllPass(sr, objects.getFirst().getCutoffFrequency(), 1.0);
		case JuceDspType::Type::lowpass: return IIRCoefficients::makeLowPass(sr, objects.getFirst().getCutoffFrequency(), 1.0);
		case JuceDspType::Type::highpass: return IIRCoefficients::makeHighPass(sr, objects.getFirst().getCutoffFrequency(), 1.0);
		}

		return IIRCoefficients();
	}

	void onComplexDataEvent(hise::ComplexDataUIUpdaterBase::EventType e, var newValue) override
	{
		if (e == ComplexDataUIUpdaterBase::EventType::ContentChange)
		{
			if (auto fd = dynamic_cast<FilterDataObject*>(this->externalData.obj))
			{
				if (sr > 0.0)
					fd->setSampleRate(sr);

				fd->setCoefficients(this, getApproximateCoefficients());
			}
		}
	}

	void sendCoefficientUpdateMessage()
	{
		DataReadLock l(this);

		if (this->externalData.obj != nullptr)
			this->externalData.obj->getUpdater().sendContentChangeMessage(sendNotificationAsync, 0);
	}

	void setExternalData(const ExternalData& d, int index) override
	{
		if (this->externalData.obj != nullptr)
		{
			d.obj->getUpdater().removeEventListener(this);
			
		}

		jassert(d.dataType == ExternalData::DataType::FilterCoefficients);

		filter_base::setExternalData(d, index);

		if (auto fd = dynamic_cast<FilterDataObject*>(d.obj))
		{
			fd->getUpdater().addEventListener(this);

			if (sr > 0.0)
				fd->setSampleRate(sr);
		}
	}

	double sr = 0.0;

	void createParameters(ParameterDataList& d) override;
};

template <int NV> struct jpanner : public base::jwrapper<juce::dsp::Panner<float>, NV>
{
	SNEX_NODE(jpanner);

    jpanner()
    {
        cppgen::CustomNodeProperties::addNodeIdManually(getStaticId(), PropertyIds::IsPolyphonic);
    }
    
	template <int P> void setParameter(double v)
	{
		for (auto& obj : this->objects)
		{
			if (P == 0)
				obj.setPan(v);
			if (P == 1)
				obj.setRule((juce::dsp::Panner<float>::Rule)(int)v);
		}
	}

	void createParameters(ParameterDataList& d) override
	{
		{
			parameter::data p("Pan", { -1.0, 1.0 });
			registerCallback<0>(p);
			p.setDefaultValue(0.0);
			d.add(p);
		}
		{
			parameter::data p("Rule");
			registerCallback<1>(p);
			p.setParameterValueNames({ "Linear", "Balanced", "Sine3dB", "Sine4.5dB", "Sine6dB", "Sqrt3dB", "Sqrt4p5dB" });
			p.setDefaultValue(1.0);
			d.add(p);
		}
	}
};

struct jdelay : public base::jwrapper<juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear>, 1>
{
	SNEX_NODE(jdelay);

	jdelay()
	{
		for (auto& obj : objects)
			obj.setMaxDelaySamples(1024);
	}

	void prepare(PrepareSpecs ps) override
	{
		JuceBaseType::prepare(ps);
		sr = ps.sampleRate;

		if (sr > 0.0)
		{
            if(maxSize != -1.0)
            {
                setParameter<0>(maxSize);
                maxSize = -1.0;
            }
            
            if(currentSize != -1.0)
            {
                setParameter<1>(currentSize);
                currentSize = -1.0;
            }
		}
	}

	template <int P> void setParameter(double v)
	{
        if(sr <= 0.0)
        {
            if constexpr (P == 0)
                maxSize = v;
            if constexpr (P == 1)
                currentSize = v;
            
            return;
        }
        
		auto sampleValue = jmax(0.0f, (float)(v * 0.001 * sr));

		FloatSanitizers::sanitizeFloatNumber(sampleValue);

		for (auto& obj : objects)
		{
			if constexpr (P == 0)
				obj.setMaxDelaySamples(roundToInt(sampleValue));
			if constexpr (P == 1)
                obj.setDelay(sampleValue);
		}
	}

	void createParameters(ParameterDataList& d) override;

	double sr = 0.0;
	double maxSize = -1.0;
    double currentSize = -1.0;
};

struct jchorus: public base::jwrapper<juce::dsp::Chorus<float>, 1>
{
	SNEX_NODE(jchorus);
	
	template <int P> void setParameter(double v)
	{
		for (auto& obj : this->objects)
		{
			if (P == 0)
				obj.setCentreDelay(jmin(v, 99.9));
			if (P == 1)
				obj.setDepth(v);
			if (P == 2)
				obj.setFeedback(v);
			if (P == 3)
				obj.setRate(v);
			if (P == 4)
				obj.setMix(v);
		}
	}

	void createParameters(ParameterDataList& d) override;
};
}

}
