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

#ifndef MDAEFFECTWRAPPER_H_INCLUDED
#define MDAEFFECTWRAPPER_H_INCLUDED

namespace hise { using namespace juce;

/** Substitutes the VSTWrapper. */
class MdaEffect
{
public:

	MdaEffect():
		sampleRate(-1.0)
	{};

	virtual ~MdaEffect()
	{};

	virtual void process(float **inputs, float **outputs, int sampleFrames) = 0;
	virtual void processReplacing(float **inputs, float **outputs, int sampleFrames) = 0;
	virtual void setParameter(int index, float value) = 0;
	virtual float getParameter(int index) const = 0;
	virtual void getParameterLabel(int index, char *label) const = 0;
	virtual void getParameterDisplay(int index, char *display) const = 0;
	virtual void getParameterName(int index, char *name) const = 0;

	virtual int getNumParameters() const = 0;

	virtual String getEffectName() const = 0;

	

	void prepareToPlay(double sampleRate_) { sampleRate = sampleRate_; };

protected:

	double getSampleRate() const { return sampleRate; };

private:

	double sampleRate;
};

/** A Wrapper for the MDA effect plugins. */
class MdaEffectWrapper: public MasterEffectProcessor
{
	
public:

	MdaEffectWrapper(MainController *mc, const String &id):
		MasterEffectProcessor(mc, id),
        inputBuffer(2, 0)
	{
		currentValues.inL = 0.0f;
		currentValues.inR = 0.0f;
		currentValues.outL = 0.0f;
		currentValues.outR = 0.0f;

	};

	void initEffectParameterNames()
	{
		for(int i = 0; i < effect->getNumParameters(); i++)
		{
			char *name2 = nullptr;

			effect->getParameterName(i, name2);

			parameterNames.add(name2);
		}
	};

	const Identifier getType() const override {return effect->getEffectName();};

	const MdaEffect *getEffect() const
	{
		return effect;
	};

	virtual void setInternalAttribute(int parameterIndex, float newValue)
	{
		effect->setParameter(parameterIndex, newValue);
	}

	virtual float getAttribute(int parameterIndex) const override
	{
		return effect->getParameter(parameterIndex);
	};

	virtual void restoreFromValueTree(const ValueTree &v) override
	{
		MasterEffectProcessor::restoreFromValueTree(v);

		for(int i = 0; i < effect->getNumParameters(); i++)
		{
			char x[64];
			effect->getParameterName(i, x);
			loadAttribute(i, String(x));
		}
	};

	virtual ValueTree exportAsValueTree() const override
	{
		ValueTree v = MasterEffectProcessor::exportAsValueTree();

		for(int i = 0; i < effect->getNumParameters(); i++)
		{
			char x[32];
			effect->getParameterName(i, x);
			saveAttribute(i, String(x));
		}

		return v;
	}

	virtual void prepareToPlay(double sampleRate, int samplesPerBlock) override
	{
		EffectProcessor::prepareToPlay(sampleRate, samplesPerBlock);

		ProcessorHelpers::increaseBufferIfNeeded(inputBuffer, samplesPerBlock);

		effect->prepareToPlay(sampleRate);
	};

	virtual void applyEffect(AudioSampleBuffer &buffer, int startSample, int numSamples) override
	{
		FloatVectorOperations::copy(inputBuffer.getWritePointer(0, startSample), buffer.getReadPointer(0, startSample), numSamples);
		FloatVectorOperations::copy(inputBuffer.getWritePointer(1, startSample), buffer.getReadPointer(1, startSample), numSamples);

		float *inputData[2];

		inputData[0] = inputBuffer.getWritePointer(0, startSample);
		inputData[1] = inputBuffer.getWritePointer(1, startSample);

		currentValues.inL = FloatVectorOperations::findMaximum(inputData[0], numSamples);
		currentValues.inR = FloatVectorOperations::findMaximum(inputData[1], numSamples);

		float *outputData[2];

		outputData[0] = buffer.getWritePointer(0, startSample);
		outputData[1] = buffer.getWritePointer(1, startSample);

		currentValues.outL = outputData[0][startSample];
		currentValues.outR = outputData[1][startSample];

		effect->processReplacing(inputData, outputData, numSamples);

		currentValues.outL = FloatVectorOperations::findMaximum(outputData[0], numSamples);
		currentValues.outR = FloatVectorOperations::findMaximum(outputData[1], numSamples);

		//sendChangeMessage();
	};

	bool hasTail() const override {return false; };

	virtual int getNumChildProcessors() const override { return 0; };

	virtual Processor *getChildProcessor(int /*processorIndex*/) override { return nullptr; };

	virtual const Processor *getChildProcessor(int /*processorIndex*/) const override { return nullptr; };

	

protected:

	

	ScopedPointer<MdaEffect> effect;

	float **input;
	float **output;

	AudioSampleBuffer inputBuffer;

private:

};


class MdaSlider: public HiSlider
{
public:

	MdaSlider(const String &name):
		HiSlider(name),
		effect(nullptr)
	{
		setMode(HiSlider::Linear, 0.0, 1.0, 0.5);
	};

	void setupSlider(const MdaEffect *effect_, Processor *effectProcessor, int parameterIndex_)
	{
		effect = effect_;
		parameterIndex = parameterIndex_;
		setup(effectProcessor, parameterIndex_, Slider::getName());
	}

	String getTextFromValue	( double /*value*/ )
	{
		if(effect == nullptr) return "undefined";

		effect->getParameterDisplay(parameterIndex, parameter);
		
		effect->getParameterLabel(parameterIndex, suffix);

		String text;

		text << parameter;
		text << " ";
		text << suffix;

		return text;
	};

private:

	char parameter[128];
	char suffix[16];

	const MdaEffect *effect;
	int parameterIndex;
};

class MdaLimiterEffect: public MdaEffectWrapper
{
public:

	static String getClassType() {return "Limiter";};
	static String getClassName() {return "Limiter";};

	String getDescription() const override { return "deprecated"; };

	enum Parameters
	{
		Threshhold = 0,
		Output,
		Attack,
		Release,
		Knee,
		numParameters
	};

	MdaLimiterEffect(MainController *mc, const String &id);

	ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor)  override;
};


class MdaDegradeEffect: public MdaEffectWrapper
{
public:

	static String getClassType() {return "Degrade";};
	static String getClassName() {return "Degrade";};
	String getDescription() const override { return "deprecated"; }

	enum InternalChains
	{
		DryWetChain = 0,
		numInternalChains
	};

	enum EditorStates
	{
		DryWetChainShown = Processor::numEditorStates,
		numEditorStates
	};

	enum Parameters
	{
		Headroom = 0,
		Quant,
		Rate,
		PostFilt,
		NonLin,
		DryWet,
		numParameters
	};

	MdaDegradeEffect(MainController *mc, const String &id);

	int getNumInternalChains() const override { return numInternalChains; };

	virtual int getNumChildProcessors() const override { return numInternalChains; };

	virtual Processor *getChildProcessor(int /*processorIndex*/) override { return dryWetChain; };

	virtual const Processor *getChildProcessor(int /*processorIndex*/) const override { return dryWetChain; };

	void setInternalAttribute(int parameterIndex, float newValue) override
	{
		if(parameterIndex == DryWet) dryWet = newValue;
		else						 effect->setParameter(parameterIndex, newValue);
	}

	float getAttribute(int parameterIndex) const override
	{
		if(parameterIndex == DryWet) return dryWet;
		else					     return effect->getParameter(parameterIndex);
	};

	ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor)  override;

	virtual void applyEffect(AudioSampleBuffer &buffer, int startSample, int numSamples) override;;

private:

	ModulatorChain* dryWetChain;

	float dryWet;

};


} // namespace hise

#endif  // MDAEFFECTWRAPPER_H_INCLUDED
