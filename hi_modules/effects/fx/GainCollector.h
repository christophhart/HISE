/*
  ==============================================================================

    GainCollector.h
    Created: 31 Aug 2015 5:56:06pm
    Author:  Christoph

  ==============================================================================
*/

#ifndef GAINCOLLECTOR_H_INCLUDED
#define GAINCOLLECTOR_H_INCLUDED


class GainCollector : public MasterEffectProcessor
{
public:

	SET_PROCESSOR_NAME("GainCollector", "Gain Collector");

	/** The parameters */
	enum Parameters
	{
		Smoothing,
		Mode,
		Attack,
		Release,
		numEffectParameters
	};

	enum EnvelopeFollowerMode
	{
		SimpleLP = 0,
		AttackRelease,
		numEnvelopeFollowerModes
	};

	GainCollector(MainController *mc, const String &id);;

	float getAttribute(int parameterIndex) const override;;
	void setInternalAttribute(int parameterIndex, float newValue) override;;

	void restoreFromValueTree(const ValueTree &v) override;;
	ValueTree exportAsValueTree() const override;

	void prepareToPlay(double sampleRate, int samplesPerBlock) override;;
	void applyEffect(AudioSampleBuffer &buffer, int startSample, int numSamples) override;;
	bool hasTail() const override { return false; };

	int getNumChildProcessors() const override { return 0; };
	Processor *getChildProcessor(int /*processorIndex*/) override { return nullptr; };
	const Processor *getChildProcessor(int /*processorIndex*/) const override { return nullptr; };

	ProcessorEditorBody *createEditor(BetterProcessorEditor *parentEditor)  override;

	float getCurrentGain() const { return currentGain; }

private:

	Smoother smoother;

	float smoothingTime;
	EnvelopeFollowerMode mode;
	float attack;
	float release;

	float currentGain;

};


#endif  // GAINCOLLECTOR_H_INCLUDED
