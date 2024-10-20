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

#ifndef CONTROLMODULATOR_H_INCLUDED
#define CONTROLMODULATOR_H_INCLUDED

 namespace hise { using namespace juce;

/**	A ControlModulator is a non polyphonic TimeVariantModulator which processes midi CC messages.
*
*	@ingroup modulatorTypes
*
*	It uses a simple low pass filter to smooth value changes.  
*/
class ControlModulator: public TimeVariantModulator,
						public LookupTableProcessor,
						public MidiControllerAutomationHandler::MPEData::Listener
{
public:

	SET_PROCESSOR_NAME("MidiController", "Midi Controller", "Creates a modulation signal from MIDI-CC messages.");

	/** Special Parameters for the ControlModulator. */
	enum Parameters
	{
		Inverted = 0, ///< inverts the modulation.
		UseTable, ///< use a Table object for a look up table
		ControllerNumber, ///< the controllerNumber that this controller reacts to.
		SmoothTime, ///< the smoothing time
		DefaultValue, ///< the default value before a control message is received.
		numSpecialParameters
	};

    void referenceShared(ExternalData::DataType, int) override
    {
        table = getTableUnchecked(0);
        table->setXTextConverter(Modulation::getDomainAsMidiRange);
    }
    
	ControlModulator(MainController *mc, const String &id, Modulation::Mode m);
	~ControlModulator();

	void restoreFromValueTree(const ValueTree &v) override;;
	ValueTree exportAsValueTree() const override;;

	Processor *getChildProcessor(int /*processorIndex*/) override final {return nullptr;};
	const Processor *getChildProcessor(int /*processorIndex*/) const override final {return nullptr;};
	int getNumChildProcessors() const override final {return 0;};

	float getAttribute (int parameter_index) const override;
	void setInternalAttribute (int parameter_index, float newValue) override;

	void calculateBlock(int startSample, int numSamples) override;;
	void handleHiseEvent(const HiseEvent &m) override;
	virtual void prepareToPlay(double sampleRate, int samplesPerBlock) override;;

	void enableLearnMode() { learnMode = true; };
	void disableLearnMode() { learnMode = false; sendOtherChangeMessage(dispatch::library::ProcessorChangeEvent::Custom); }
	bool learnModeActive() const { return learnMode; }

	ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor)  override;

	void mpeModeChanged(bool isEnabled) override
	{
		mpeEnabled = isEnabled;
	}

	void mpeDataReloaded() override {}

	void mpeModulatorAssigned(MPEModulator* /*m*/, bool /*wasAssigned*/) override
	{

	}

private:

	bool mpeEnabled = false;

	float calculateNewValue();

	int controllerNumber;
	float defaultValue;
	bool inverted;
	float smoothTime;
	bool useTable;

	float polyValues[128];

	bool learnMode;
	float targetValue;
	int64 uptime;
	float inputValue;
    float lastInputValue = -1.0f;

	// the smoothed version of the target value
	float currentValue;
	float lastCurrentValue;
	float intensity;

	Smoother smoother;

	SampleLookupTable* table;

	JUCE_DECLARE_WEAK_REFERENCEABLE(ControlModulator);
};


} // namespace hise

#endif  // CONTROLMODULATOR_H_INCLUDED
