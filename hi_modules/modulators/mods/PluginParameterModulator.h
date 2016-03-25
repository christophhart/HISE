/*
  ==============================================================================

    ConstantModulator.h
    Created: 15 Jun 2014 1:43:59pm
    Author:  Chrisboy

  ==============================================================================
*/

#ifndef PLUGINPARAMETERMODULATOR_H_INCLUDED
#define PLUGINPARAMETERMODULATOR_H_INCLUDED

/** This modulator simply returns a constant value that can be used to change the gain or something else.
*
*	@ingroup modulatorTypes
*/
class PluginParameterModulator: public TimeVariantModulator
{
public:

	SET_PROCESSOR_NAME("HostParameter", "Host Parameter")

	/// Additional Parameters for the constant modulator
	enum SpecialParameters
	{
		Value = 0,
		numTotalParameters
	};

	enum Mode
	{
		Linear,
		Volume,
		OnOff
	};

	PluginParameterModulator(MainController *mc, const String &id, Modulation::Mode m):
		TimeVariantModulator(mc, id, m),
		Modulation(m),
		value(0.0f),
		slotIndex(-1),
		suffix("%"),
		mode(Linear)
	{
		slotIndex = getMainController()->addPluginParameter(this);
	};

	~PluginParameterModulator()
	{
		getMainController()->removePluginParameter(this);
	}

	void restoreFromValueTree(const ValueTree &v) override
	{
		TimeVariantModulator::restoreFromValueTree(v);

		loadAttribute(Value, "Value");
	};

	ValueTree exportAsValueTree() const override
	{
		ValueTree v = TimeVariantModulator::exportAsValueTree();

		saveAttribute(Value, "Value");

		return v;
	}
	
	void removeFromPluginParameterSlot();

	int getSlotIndex() const { return slotIndex; };

	void handleMidiEvent(const MidiMessage &/*m*/) override {};

	int getNumChildProcessors() const override {return 0;};

	Processor *getChildProcessor(int ) override {jassertfalse; return nullptr;};

	const Processor *getChildProcessor(int ) const override {jassertfalse; return nullptr;};

	ProcessorEditorBody *createEditor(BetterProcessorEditor *parentEditor)  override;

	/// sets the constant value. The only valid parameter_index is Intensity
	void setInternalAttribute(int, float newValue) override
	{
		value = newValue;
	};

	const String getSuffix() const
	{
		return suffix;
	};

	/// returns the constant value
	float getAttribute(int ) const override
	{
		return value;
	};

	

	const String getParameterAsText()
	{
		return String(value*100.0f, 2) + suffix;
	};

	void calculateBlock(int startSample, int numSamples) override
	{
		if(--numSamples >= 0)
		{
			const float value = calculateNewValue();
			internalBuffer.setSample(0, startSample, value);
			++startSample;
			setOutputValue(value); 
		}

		while(--numSamples >= 0)
		{
			internalBuffer.setSample(0, startSample, calculateNewValue());
			++startSample;
		}
	};	

private:

	inline float calculateNewValue() { return value; };

	int slotIndex;
	float value;
	String suffix;
	Mode mode;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginParameterModulator)
};



#endif  // CONSTANTMODULATOR_H_INCLUDED
