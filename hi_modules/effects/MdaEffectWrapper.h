/*
  ==============================================================================

    MdaEffectWrapper.h
    Created: 2 Aug 2014 10:39:44pm
    Author:  Chrisboy

  ==============================================================================
*/

#ifndef MDAEFFECTWRAPPER_H_INCLUDED
#define MDAEFFECTWRAPPER_H_INCLUDED

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
		MasterEffectProcessor(mc, id)
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
			char *name;

			effect->getParameterName(i, name);

			parameterNames.add(name);
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

		inputBuffer = AudioSampleBuffer(2, samplesPerBlock);

		effect->prepareToPlay(sampleRate);
	};

	virtual void applyEffect(AudioSampleBuffer &buffer, int startSample, int numSamples) override
	{
		FloatVectorOperations::copy(inputBuffer.getWritePointer(0, startSample), buffer.getReadPointer(0, startSample), numSamples);
		FloatVectorOperations::copy(inputBuffer.getWritePointer(1, startSample), buffer.getReadPointer(1, startSample), numSamples);

		float *input[2];

		

		input[0] = inputBuffer.getWritePointer(0, startSample);
		input[1] = inputBuffer.getWritePointer(1, startSample);

		

		currentValues.inL = FloatVectorOperations::findMaximum(input[0], numSamples);
		currentValues.inR = FloatVectorOperations::findMaximum(input[1], numSamples);

		float *output[2];

		output[0] = buffer.getWritePointer(0, startSample);
		output[1] = buffer.getWritePointer(1, startSample);

		currentValues.outL = output[0][startSample];
		currentValues.outR = output[1][startSample];

		effect->processReplacing(input, output, numSamples);

		currentValues.outL = FloatVectorOperations::findMaximum(output[0], numSamples);
		currentValues.outR = FloatVectorOperations::findMaximum(output[1], numSamples);

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

	ProcessorEditorBody *createEditor(BetterProcessorEditor *parentEditor)  override;
};


class MdaDegradeEffect: public MdaEffectWrapper
{
public:

	static String getClassType() {return "Degrade";};
	static String getClassName() {return "Degrade";};

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

	AudioSampleBuffer &getBufferForChain(int /*chainIndex*/) { return dryWetBuffer; };

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

	ProcessorEditorBody *createEditor(BetterProcessorEditor *parentEditor)  override;

	virtual void applyEffect(AudioSampleBuffer &buffer, int startSample, int numSamples) override
	{
		FloatVectorOperations::copy(inputBuffer.getWritePointer(0, startSample), buffer.getReadPointer(0, startSample), numSamples);
		FloatVectorOperations::copy(inputBuffer.getWritePointer(1, startSample), buffer.getReadPointer(1, startSample), numSamples);

		float *input[2];

		input[0] = inputBuffer.getWritePointer(0, startSample);
		input[1] = inputBuffer.getWritePointer(1, startSample);

		float *output[2];

		output[0] = buffer.getWritePointer(0, startSample);
		output[1] = buffer.getWritePointer(1, startSample);

		effect->processReplacing(input, output, numSamples);

		float *dryWetModValues = dryWetBuffer.getWritePointer(0, startSample);
		FloatVectorOperations::multiply(dryWetModValues, dryWet, numSamples);

		FloatVectorOperations::multiply(buffer.getWritePointer(0, startSample), dryWetModValues, numSamples);
		FloatVectorOperations::multiply(buffer.getWritePointer(1, startSample), dryWetModValues, numSamples);

		FloatVectorOperations::multiply(dryWetModValues, -1.0f, numSamples);
		FloatVectorOperations::add(dryWetModValues, 1.0f, numSamples);

		FloatVectorOperations::multiply(input[0], dryWetModValues, numSamples);
		FloatVectorOperations::multiply(input[1], dryWetModValues, numSamples);

		FloatVectorOperations::add(buffer.getWritePointer(0, startSample), input[0], numSamples);
		FloatVectorOperations::add(buffer.getWritePointer(1, startSample), input[1], numSamples);

	};

private:

	ScopedPointer<ModulatorChain> dryWetChain;
	AudioSampleBuffer dryWetBuffer;

	float dryWet;

};



#endif  // MDAEFFECTWRAPPER_H_INCLUDED
