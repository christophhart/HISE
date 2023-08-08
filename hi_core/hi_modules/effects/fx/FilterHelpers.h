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

#ifndef FILTER_HELPERS_H_INCLUDED
#define FILTER_HELPERS_H_INCLUDED

namespace hise {
using namespace juce;


#define GET_TYPED(Type) case FilterHelpers::FilterSubType::Type: static_cast<FilterBank<Type>*>(this)



class FilterBank
{
public:

	// Remember to keep the ScriptingObjects::ScriptingEffect::FilterListObject 
	enum FilterMode
	{
		LowPass = 0,
		HighPass,
		LowShelf,
		HighShelf,
		Peak,
		ResoLow,
		StateVariableLP,
		StateVariableHP,
		MoogLP,
		OnePoleLowPass,
		OnePoleHighPass,
		StateVariablePeak,
		StateVariableNotch,
		StateVariableBandPass,
		Allpass,
		LadderFourPoleLP,
		LadderFourPoleHP,
		RingMod,
		numFilterModes
	};

	FilterBank(int numVoices_);

	~FilterBank();

	bool isPoly() const noexcept { return numVoices != 1; };

	explicit operator bool() const noexcept { return object.get() != nullptr; };

	void setMode(FilterMode mode);

	void renderPoly(FilterHelpers::RenderData& r);
	void renderMono(FilterHelpers::RenderData& r);

    void setDisplayModValues(int voiceIndex, float freqModValue_, float gainModValue_)
    {
        if(voiceIndex != displayVoiceIndex)
            return;
        
        freqModValue = freqModValue_;
        gainModValue = gainModValue_;
    }
    
    void setDisplayVoiceIndex(int v) const
    {
        displayVoiceIndex = v;
    }
    
	void reset(int voiceIndex);
	void reset();

	IIRCoefficients getCurrentCoefficients() const noexcept;

	FilterMode getMode() const noexcept { return mode; }

	void setQ(double newQ);;

	void setFrequency(double newFrequency)
	{
		frequency = newFrequency;
		object->setFrequency(newFrequency);
	}

	void setGain(float newGain);;

	double getQ() const noexcept { return q; }
	double getFrequency() const noexcept { return frequency; }
	float getGain() const noexcept { return gain; }

	void setSmoothingTime(double newSmoothingTime)
	{
		SpinLock::ScopedLockType sl(lock);
		object->setSmoothingTime(jlimit(0.0, 1.0, newSmoothingTime));
	}

	void setSampleRate(double newSampleRate)
	{
		SpinLock::ScopedLockType sl(lock);

		object->setSampleRate(newSampleRate);
	}

	bool calculateGainModValue = false;

private:

	void setType(FilterHelpers::FilterSubType newType, int filterSubType);

	class InternalBankBase
	{
	public:

		InternalBankBase(FilterHelpers::FilterSubType t) :
			type(t)
		{};

		virtual ~InternalBankBase() {};

		virtual void setSampleRate(double sampleRate) = 0;
		virtual void setType(int subType) = 0;
		virtual void setSmoothingTime(double smoothingTimeSeconds) = 0;

		virtual void setFrequency(double newFrequency) = 0;
		virtual void setQ(double newQ) = 0;
		virtual void setGain(double newGain) = 0;

		const FilterHelpers::FilterSubType type;

	private:

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InternalBankBase)
	};


	template <class FilterType> class InternalMonoBank : public InternalBankBase
	{
	public:
		InternalMonoBank() :
			InternalBankBase(FilterType::getFilterType())
		{};

		void render(FilterHelpers::RenderData& r)
		{
			filter.render(r);
		}

		void setSmoothingTime(double smoothingTimeSeconds) override
		{
			filter.setSmoothingTime(smoothingTimeSeconds);
		}

		void setType(int subType) override
		{
			filter.setType(subType);
		}

		void reset()
		{
			filter.reset();
		}

		void setFrequency(double newFrequency) final override
		{
			filter.setFrequency(newFrequency);
		}

		void setQ(double newFrequency) final override
		{
			filter.setQ(newFrequency);
		}

		void setSampleRate(double newSampleRate) final override
		{
			filter.setSampleRate(newSampleRate);
		}

		void setGain(double newFrequency) final override
		{
			filter.setGain(newFrequency);
		}

	private:

		MultiChannelFilter<FilterType> filter;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InternalMonoBank);
	};

	template <class FilterType> class InternalPolyBank : public InternalBankBase
	{
	public:
		InternalPolyBank(int numVoices) :
			InternalBankBase(FilterType::getFilterType()),
			filters(numVoices)
		{};

		void render(FilterHelpers::RenderData& r)
		{
			filters[r.voiceIndex].render(r);
		}

		void processFrame(float* frameData, int numChannels)
		{
			filters[this->r.voiceIndex].processFrame(frameData, numChannels);
		}

		void setSampleRate(double sampleRate) override
		{
			for (auto &filter : filters)
				filter.setSampleRate(sampleRate);
		}

		void setType(int subType) override
		{
			for (auto& filter : filters)
				filter.setType(subType);
		}

		void reset(int voiceIndex)
		{
			filters[voiceIndex].reset();
		}

		void setSmoothingTime(double smoothingTimeSeconds) final override
		{
			for(auto& filter: filters)
				filter.setSmoothingTime(smoothingTimeSeconds);
		}

		void setFrequency(double newFrequency) final override
		{
			for (auto& filter : filters)
				filter.setFrequency(newFrequency);
		}

		void setQ(double newFrequency) final override
		{
			for (auto& filter : filters)
				filter.setQ(newFrequency);
		}

		void setGain(double newFrequency) final override
		{
			for (auto& filter : filters)
				filter.setGain(newFrequency);
		}

	private:

		FixedVoiceAmountArray<MultiChannelFilter<FilterType>> filters;
		

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InternalPolyBank);
	};


	template <class FilterType> InternalPolyBank<FilterType>* getAsPoly()
	{
		jassert(FilterType::getFilterType() == type);
		return static_cast<InternalPolyBank<FilterType>*>(object.get());
	}

	template <class FilterType> InternalMonoBank<FilterType>* getAsMono()
	{
		jassert(FilterType::getFilterType() == type);
		return static_cast<InternalMonoBank<FilterType>*>(object.get());
	}

	SpinLock lock;

	FilterMode mode;

	double frequency = 20000.0;
	float gain = 1.0f;
	double q = 1.0;
	
    float freqModValue = 1.0f;
    float gainModValue = 1.0f;
    mutable int displayVoiceIndex = -1;
    
	double sampleRate = 44100.0;

	const int numVoices;

	FilterHelpers::FilterSubType type;
	int subType = -1;
	ScopedPointer<InternalBankBase> object = nullptr;
};

#ifndef FILL_PARAMETER_ID
/** Adds a case statement for the enum and returns the name of the enum. */
#define FILL_PARAMETER_ID(enumClass, enumId, size, text) case (int)enumClass::enumId: size = (int)HelperFunctions::writeString(text, #enumId); break;
#endif




class ScriptFilterBank : public StaticDspFactory
{
public:

	class Filter : public DspBaseObject
	{
	public:

		enum Parameters
		{
			Gain = 0,
			Frequency,
			Q,
			Mode,
			Reset,
			SmoothingTime,
			
			numParameters
		};

		Filter() :
			DspBaseObject(),
			internalFilter(1)
		{
			b.setSize(0, 0);
		};

		static Identifier getName() { static const Identifier id("multi_filter"); return id; };
		
		int getNumConstants() const override
		{
			return Parameters::numParameters + hise::FilterBank::FilterMode::numFilterModes;
		}

		void getIdForConstant(int index, char*name, int &size) const noexcept override
		{
			switch (index)
			{
				FILL_PARAMETER_ID(Parameters, Gain, size, name);
				FILL_PARAMETER_ID(Parameters, Frequency, size, name);
				FILL_PARAMETER_ID(Parameters, Q, size, name);
				FILL_PARAMETER_ID(Parameters, Mode, size, name);
				FILL_PARAMETER_ID(Parameters, Reset, size, name);
				FILL_PARAMETER_ID(Parameters, SmoothingTime, size, name);
				FILL_PARAMETER_ID(Parameters::numParameters + hise::FilterBank::FilterMode, LowPass, size, name);
				FILL_PARAMETER_ID(Parameters::numParameters + hise::FilterBank::FilterMode, HighPass, size, name);
				FILL_PARAMETER_ID(Parameters::numParameters + hise::FilterBank::FilterMode, LowShelf, size, name);
				FILL_PARAMETER_ID(Parameters::numParameters + hise::FilterBank::FilterMode, HighShelf, size, name);
				FILL_PARAMETER_ID(Parameters::numParameters + hise::FilterBank::FilterMode, Peak, size, name);
				FILL_PARAMETER_ID(Parameters::numParameters + hise::FilterBank::FilterMode, ResoLow, size, name);
				FILL_PARAMETER_ID(Parameters::numParameters + hise::FilterBank::FilterMode, StateVariableLP, size, name);
				FILL_PARAMETER_ID(Parameters::numParameters + hise::FilterBank::FilterMode, StateVariableHP, size, name);
				FILL_PARAMETER_ID(Parameters::numParameters + hise::FilterBank::FilterMode, MoogLP, size, name);
				FILL_PARAMETER_ID(Parameters::numParameters + hise::FilterBank::FilterMode, OnePoleLowPass, size, name);
				FILL_PARAMETER_ID(Parameters::numParameters + hise::FilterBank::FilterMode, OnePoleHighPass, size, name);
				FILL_PARAMETER_ID(Parameters::numParameters + hise::FilterBank::FilterMode, StateVariablePeak, size, name);
				FILL_PARAMETER_ID(Parameters::numParameters + hise::FilterBank::FilterMode, StateVariableNotch, size, name);
				FILL_PARAMETER_ID(Parameters::numParameters + hise::FilterBank::FilterMode, StateVariableBandPass, size, name);
				FILL_PARAMETER_ID(Parameters::numParameters + hise::FilterBank::FilterMode, Allpass, size, name);
				FILL_PARAMETER_ID(Parameters::numParameters + hise::FilterBank::FilterMode, LadderFourPoleLP, size, name);
				FILL_PARAMETER_ID(Parameters::numParameters + hise::FilterBank::FilterMode, LadderFourPoleHP, size, name);
				FILL_PARAMETER_ID(Parameters::numParameters + hise::FilterBank::FilterMode, RingMod, size, name);
			}
		};

		bool getConstant(int index, int& value) const noexcept override
		{
			if (index < getNumParameters())
			{
				value = index;
				return true;
			}
			else if (index < getNumConstants())
			{
				value = index - Parameters::numParameters;
				return true;
			}

			return false;
		};

		void setParameter(int index, float newValue) override
		{
			switch (index)
			{
			case Parameters::Gain:			internalFilter.setGain(newValue); break;
			case Parameters::Frequency:		internalFilter.setFrequency(newValue); break;
			case Parameters::Q:				internalFilter.setQ(newValue); break;
			case Parameters::Mode:			internalFilter.setMode((FilterBank::FilterMode)(int)newValue); break;
			case Parameters::Reset:			internalFilter.reset(); break;
			case Parameters::SmoothingTime: smoothingTime = newValue;  internalFilter.setSmoothingTime((double)newValue); break;
			default: break;
			}
		}

		float getParameter(int index) const override
		{
			switch (index)
			{
			case Parameters::Gain:			return internalFilter.getGain();
			case Parameters::Frequency:		return (float)internalFilter.getFrequency();
			case Parameters::Q:				return (float)internalFilter.getQ();
			case Parameters::Mode:			return (float)internalFilter.getMode();
			case Parameters::SmoothingTime: return smoothingTime;
			case Parameters::Reset:			
			
			default: return 0.0f;
			}
		}

		int getNumParameters() const override { return Parameters::numParameters; }

		void processBlock(float **data, int numChannels, int numSamples) override
		{
			b.setDataToReferTo(data, numChannels, numSamples);
			FilterHelpers::RenderData r(b, 0, numSamples);
			internalFilter.renderMono(r);
		}

		void prepareToPlay(double sampleRate, int) override
		{
			internalFilter.setSampleRate(sampleRate);
		}

		float smoothingTime = 0.03f;
		hise::FilterBank internalFilter;
		AudioSampleBuffer b;
	};

	ScriptFilterBank() :
		StaticDspFactory()
	{

	}

	Identifier getId() const override { RETURN_STATIC_IDENTIFIER("filters") };

	void registerModules() override
	{
		registerDspModule<Filter>();
	}
};

class FilterEffect
{
public:

	static IIRCoefficients getDisplayCoefficients(FilterBank::FilterMode m, double frequency, double q, float gain, double samplerate);
	static String getTableValueAsGain(float input);


	void setRenderQuality(int powerOfTwo);

	int getSampleAmountForRenderQuality() const;

	virtual ~FilterEffect() {};

	virtual IIRCoefficients getCurrentCoefficients() const = 0;

protected:

	int quality;

};

}



#endif // FILTER_HELPERS_H_INCLUDED
