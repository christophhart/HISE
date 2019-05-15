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

#ifndef DYNAMICS_H_INCLUDED
#define DYNAMICS_H_INCLUDED

namespace hise { using namespace juce;

/** A general purpose dynamics processor based on chunkware's SimpleCompressor.
	@ingroup effectTypes
*/
class DynamicsEffect : public MasterEffectProcessor
{
public:

	SET_PROCESSOR_NAME("Dynamics", "Dynamics", "A general purpose dynamics processor based on chunkware's SimpleCompressor");

		enum Parameters
	{
		GateEnabled,
		GateThreshold,
		GateAttack,
		GateRelease,
		GateReduction,
		CompressorEnabled,
		CompressorThreshold,
		CompressorRatio,
		CompressorAttack,
		CompressorRelease,
		CompressorReduction,
		CompressorMakeup,
		LimiterEnabled,
		LimiterThreshold,
		LimiterAttack,
		LimiterRelease,
		LimiterReduction,
		LimiterMakeup,
		numParameters
	};

	DynamicsEffect(MainController *mc, const String &uid);;

	~DynamicsEffect()
	{};

	void setInternalAttribute(int parameterIndex, float newValue) override;;
	float getAttribute(int parameterIndex) const override;
	float getDefaultValue(int parameterIndex) const override;

	void restoreFromValueTree(const ValueTree &v) override;;
	ValueTree exportAsValueTree() const override;

	bool hasTail() const override { return false; };

	ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor)  override;

	const Processor* getChildProcessor(int /*processorIndex*/) const { return nullptr; };
	Processor* getChildProcessor(int /*processorIndex*/) { return nullptr; };
	int getNumChildProcessors() const { return 0; };

	void applyEffect(AudioSampleBuffer &buffer, int startSample, int numSamples) override;

	void applyLimiter(AudioSampleBuffer &buffer, int startSample, const int numToProcess);

	void prepareToPlay(double sampleRate, int samplesPerBlock) override;

private:

	void updateMakeupValues(bool updateLimiter);

	chunkware_simple::SimpleGate gate;
	chunkware_simple::SimpleComp compressor;
	chunkware_simple::SimpleLimit limiter;

	std::atomic<bool> gateEnabled;
	std::atomic<bool> compressorEnabled;
	std::atomic<bool> limiterEnabled;
	std::atomic<bool> limiterPending;

	std::atomic<bool> compressorMakeup;
	std::atomic<bool> limiterMakeup;

	std::atomic<float> gateReduction;
	std::atomic<float> limiterReduction;
	std::atomic<float> compressorReduction;

	std::atomic<float> compressorMakeupGain;
	std::atomic<float> limiterMakeupGain;
};

class DynamicProcessingLib : public StaticDspFactory
{
public:

	struct Helpers
	{
		static Identifier getName(chunkware_simple::SimpleGate* g) { return "gate"; }
		static Identifier getName(chunkware_simple::SimpleComp* g) { return "compressor"; }
		static Identifier getName(chunkware_simple::SimpleLimit* g) { return "limiter"; }
	};

	class Gate : public DspBaseObject
	{
	public:

		using DynamicProcessorType = chunkware_simple::SimpleGate;

		enum Parameter
		{
			Threshhold,
			Attack,
			Release,
			Ratio,
			Reduction,
			numParameters
		};

		static Identifier getName()
		{
			DynamicProcessorType* t = nullptr;
			return Helpers::getName(t);
		}

		Gate()
		{
		}

		void setParameter(int index, float newValue) override
		{
			Parameter p = (Parameter)index;

			auto v = (SimpleDataType)newValue;

			switch (p)
			{
			case Attack: obj.setAttack(v); break;
			case Release: obj.setRelease(v); break;
			case Threshhold: obj.setThresh(v); break;
			case Ratio: obj.setRatio(v);
			case Reduction: break;
			}
		};

		int getNumParameters() const override { return (int)Parameter::numParameters; };

		float getParameter(int index) const override
		{
			Parameter p = (Parameter)index;

			switch (p)
			{
			case Attack:		return static_cast<float>(obj.getAttack());
			case Release:		return static_cast<float>(obj.getRelease());
			case Threshhold:	return static_cast<float>(obj.getThresh());
			case Ratio:			return static_cast<float>(obj.getRatio());
			case Reduction:		return static_cast<float>(obj.getGainReduction());
			}
		};

		int getNumConstants() const override
		{
			return (int)Parameter::numParameters;
		}

		void getIdForConstant(int index, char*name, int &size) const noexcept override
		{
			switch (index)
			{
				FILL_PARAMETER_ID(Parameter, Threshhold, size, name);
				FILL_PARAMETER_ID(Parameter, Attack, size, name);
				FILL_PARAMETER_ID(Parameter, Release, size, name);
				FILL_PARAMETER_ID(Parameter, Ratio, size, name);
				FILL_PARAMETER_ID(Parameter, Reduction, size, name);
			}
		};

		bool getConstant(int index, int& value) const noexcept override
		{
			if (index < getNumParameters())
			{
				value = index;
				return true;
			}

			return false;
		};

		void prepareToPlay(double sampleRate, int ) override
		{
			obj.setSampleRate(sampleRate);
		}

		void processBlock(float **data, int numChannels, int numSamples) override
		{
			if (numChannels == 2)
			{
				auto l = data[0];
				auto r = data[1];

				while (--numSamples >= 0)
				{
					SimpleDataType lValue = static_cast<SimpleDataType>(*l);
					SimpleDataType rValue = static_cast<SimpleDataType>(*r);

					obj.process(lValue, rValue);

					*l++ = static_cast<float>(lValue);
					*r++ = static_cast<float>(rValue);
				}
			}

			if (numChannels == 1)
			{
				auto l = data[0];

				while (--numSamples >= 0)
				{
					SimpleDataType lValue = static_cast<SimpleDataType>(*l);
					SimpleDataType rValue = static_cast<SimpleDataType>(*l);

					obj.process(lValue, rValue);

					*l++ = static_cast<float>(lValue);
				}
			}
		}

		DynamicProcessorType obj;
		
	};

	DynamicProcessingLib():
		StaticDspFactory()
	{}

	Identifier getId() const override { RETURN_STATIC_IDENTIFIER("dynamics") };

	void registerModules() override
	{
		registerDspModule<Gate>();
	}

};

} // namespace hise

#endif  // DYNAMICS_H_INCLUDED
