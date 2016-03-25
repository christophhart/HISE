/*
  ==============================================================================

    Chorus.h
    Created: 12 Jul 2015 7:37:21pm
    Author:  Chrisboy

  ==============================================================================
*/

#ifndef CHORUS_H_INCLUDED
#define CHORUS_H_INCLUDED

#define BUFMAX   2048

/** 
*/
class ChorusEffect : public MasterEffectProcessor
{
public:

	SET_PROCESSOR_NAME("Chorus", "Chorus");

	/** The parameters */
	enum Parameters
	{
		Rate,
		Width,
		Feedback,
		Delay,
		numEffectParameters
	};

	ChorusEffect(MainController *mc, const String &id);;

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
	void processReplacing(const float** inputs, float** outputs, int sampleFrames);

	void calculateInternalValues();

private:

	AudioSampleBuffer tempBuffer;

	///global internal variables
	float rat, dep, wet, dry, fb, dem; //rate, depth, wet & dry mix, feedback, mindepth
	float phi, fb1, fb2, deps;         //lfo & feedback buffers, depth change smoothing 
	float *buffer, *buffer2;           //maybe use 2D buffer?
	uint32 size, bufpos;

	float parameterRate;
	float parameterDepth;
	float parameterMix;
	float parameterFeedback;
	float parameterDelay;
};







#endif  
