/*
  ==============================================================================

    RouteFX.h
    Created: 6 Nov 2015 5:42:39pm
    Author:  Christoph

  ==============================================================================
*/

#ifndef ROUTEFX_H_INCLUDED
#define ROUTEFX_H_INCLUDED

 



/** A simple gain effect that allows time variant modulation. */
class RouteEffect : public MasterEffectProcessor
{
public:

	SET_PROCESSOR_NAME("RouteFX", "Routing Matrix");

	RouteEffect(MainController *mc, const String &uid);;

	float getAttribute(int ) const override { return 1.0f; };

	

	void setInternalAttribute(int , float ) override {};


	void restoreFromValueTree(const ValueTree &v) override
	{
		MasterEffectProcessor::restoreFromValueTree(v);
	};

	ValueTree exportAsValueTree() const override
	{
		ValueTree v = MasterEffectProcessor::exportAsValueTree();
		
		return v;
	}


	int getNumInternalChains() const override { return 0; };

	bool hasTail() const override { return false; };

	Processor *getChildProcessor(int /*processorIndex*/) override { return nullptr; };

	const Processor *getChildProcessor(int /*processorIndex*/) const override { return nullptr; };

	int getNumChildProcessors() const override { return 0; };

	ProcessorEditorBody *createEditor(BetterProcessorEditor *parentEditor)  override;

	void prepareToPlay(double sampleRate, int samplesPerBlock)
	{
		MasterEffectProcessor::prepareToPlay(sampleRate, samplesPerBlock);
	}

	virtual void renderNextBlock(AudioSampleBuffer &buffer, int startSample, int numSamples) override;
	

	void applyEffect(AudioSampleBuffer &/*b*/, int /*startSample*/, int /*numSamples*/) override;

private:

	

};



#endif  // ROUTEFX_H_INCLUDED
