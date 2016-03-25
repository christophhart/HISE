/*
  ==============================================================================

    GainMatcher.h
    Created: 31 Aug 2015 5:55:43pm
    Author:  Christoph

  ==============================================================================
*/

#ifndef GAINMATCHER_H_INCLUDED
#define GAINMATCHER_H_INCLUDED

 

class GainCollector;

class GainMatcherModulator : public LookupTableProcessor
{
public:

	enum Parameters
	{
		UseTable = 0,
		numTotalParameters
	};

	GainMatcherModulator();

	virtual ~GainMatcherModulator() {};

	StringArray getListOfAllGainCollectors();

	String getConnectedCollectorId() const;

	void setConnectedCollectorId(const String &newId);

	Table *getTable(int = 0) const override { return table; };

	const GainCollector *getCollector() const;

protected:

	ScopedPointer<SampleLookupTable> table;

private:

	WeakReference<Processor> connectedCollector;
	
};


class GainMatcherVoiceStartModulator : public VoiceStartModulator,
									   public GainMatcherModulator
{
public:

	SET_PROCESSOR_NAME("GainMatcherVoiceStartModulator", "Voice Start Gain Matcher")

	GainMatcherVoiceStartModulator(MainController *mc, const String &id, int numVoices, Modulation::Mode m);

	void restoreFromValueTree(const ValueTree &v) override;;
	ValueTree exportAsValueTree() const override;

	ProcessorEditorBody *createEditor(BetterProcessorEditor *parentEditor)  override;

	void setInternalAttribute(int parameterIndex, float newValue) override;;
	float getAttribute(int parameterIndex) const override;;

	float calculateVoiceStartValue(const MidiMessage &) override;;

private:

	float currentValue;
	bool useTable;
	Random generator;
	
};


class GainMatcherTimeVariantModulator : public TimeVariantModulator,
										public GainMatcherModulator
{
public:

	SET_PROCESSOR_NAME("GainMatcherTimeVariantModulator", "Time Variant Gain Matcher")

	GainMatcherTimeVariantModulator(MainController *mc, const String &id, Modulation::Mode m);
	~GainMatcherTimeVariantModulator();

	void restoreFromValueTree(const ValueTree &v) override;;
	ValueTree exportAsValueTree() const override;;

	Processor *getChildProcessor(int /*processorIndex*/) override final { return nullptr; };
	const Processor *getChildProcessor(int /*processorIndex*/) const override final { return nullptr; };
	int getNumChildProcessors() const override final { return 0; };

	float getAttribute(int parameter_index) const override;
	void setInternalAttribute(int parameter_index, float newValue) override;

	void calculateBlock(int startSample, int numSamples) override;;
	void handleMidiEvent(const MidiMessage &m) override;
	virtual void prepareToPlay(double sampleRate, int samplesPerBlock) override;;

	ProcessorEditorBody *createEditor(BetterProcessorEditor *parentEditor)  override;

private:

	float calculateNewValue();

	float currentValue;

	Ramper r;

	bool useTable;

};



#endif  // GAINMATCHER_H_INCLUDED
