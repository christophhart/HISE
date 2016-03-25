#ifndef SATURATOR_H_INCLUDED
#define SATURATOR_H_INCLUDED

/** A simple gain effect that allows time variant modulation. */
class SaturatorEffect : public MasterEffectProcessor
{
public:

	SET_PROCESSOR_NAME("Saturator", "Saturator")

	enum InternalChains
	{
		SaturationChain = 0,
		numInternalChains
	};

	enum EditorStates
	{
		SaturationChainShown = Processor::numEditorStates,
		numEditorStates
	};

	enum Parameters
	{
		Saturation = 0,
		WetAmount,
		PreGain,
		PostGain,
		numParameters
	};

	SaturatorEffect(MainController *mc, const String &uid);;

	~SaturatorEffect()
	{};

	void setInternalAttribute(int parameterIndex, float newValue) override;;
	float getAttribute(int parameterIndex) const override;
	float getDefaultValue(int parameterIndex) const override;

	void restoreFromValueTree(const ValueTree &v) override;;
	ValueTree exportAsValueTree() const override;

	bool hasTail() const override { return false; };

	Processor *getChildProcessor(int /*processorIndex*/) override { return saturationChain; };
	const Processor *getChildProcessor(int /*processorIndex*/) const override { return saturationChain; };
	int getNumInternalChains() const override { return numInternalChains; };
	int getNumChildProcessors() const override { return numInternalChains; };

	ProcessorEditorBody *createEditor(BetterProcessorEditor *parentEditor)  override;

	virtual void renderNextBlock(AudioSampleBuffer &buffer, int startSample, int numSamples);;
	void prepareToPlay(double sampleRate, int samplesPerBlock);
	void applyEffect(AudioSampleBuffer &/*b*/, int /*startSample*/, int /*numSamples*/) override {};

private:

	float dry;
	float wet;
	float saturation;
	float preGain;
	float postGain;

	Saturator saturator;

	ScopedPointer<ModulatorChain> saturationChain;

	AudioSampleBuffer saturationBuffer;
};



#endif  // SATURATOR_H_INCLUDED
