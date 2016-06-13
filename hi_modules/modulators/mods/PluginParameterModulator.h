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
*   Commercial licences for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licencing:
*
*   http://www.hartinstruments.net/hise/
*
*   HISE is based on the JUCE library,
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
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

	ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor)  override;

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
