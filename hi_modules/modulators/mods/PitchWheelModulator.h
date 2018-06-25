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

#ifndef PITCHWHEELMODULATOR_H_INCLUDED
#define PITCHWHEELMODULATOR_H_INCLUDED

namespace hise { using namespace juce;

/**	A PitchwheelModulator is a non polyphonic TimeVariantModulator which processes pitch bend messages.
*
*	@ingroup modulatorTypes
*
*	It uses a simple low pass filter to smooth value changes.  
*/
class PitchwheelModulator: public TimeVariantModulator,
						   public LookupTableProcessor,
						   public MidiControllerAutomationHandler::MPEData::Listener
{
public:

	SET_PROCESSOR_NAME("PitchWheel", "Pitch Wheel Modulator")

	PitchwheelModulator(MainController *mc, const String &id, Modulation::Mode m);

	~PitchwheelModulator();

	/** Special Parameters for the PitchwheelModulator. */
	enum Parameters
	{
		Inverted=0, ///< inverts the modulation.
		UseTable, ///< use a Table object for a look up table
		SmoothTime ///< the smoothing time
	};

	void restoreFromValueTree(const ValueTree &v) override
	{
		TimeVariantModulator::restoreFromValueTree(v);

		loadAttribute(UseTable, "UseTable");
		loadAttribute(Inverted, "Inverted");
		loadAttribute(SmoothTime, "SmoothTime");
		
		if(useTable) loadTable(table, "PitchwheelTableData");
	};

	ValueTree exportAsValueTree() const override
	{
		ValueTree v = TimeVariantModulator::exportAsValueTree();

		saveAttribute(UseTable, "UseTable");
		saveAttribute(Inverted, "Inverted");
		saveAttribute(SmoothTime, "SmoothTime");
		
		if(useTable) saveTable(table, "PitchwheelTableData");

		return v;

	};

	void mpeModeChanged(bool isEnabled) override 
	{
		mpeEnabled = isEnabled;
	}

	void mpeDataReloaded() override {}

	void mpeModulatorAssigned(MPEModulator* /*m*/, bool /*wasAssigned*/) override
	{
		
	}
	

	/** Returns a new editor */
	ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor)  override;

	virtual Processor *getChildProcessor(int /*processorIndex*/) override final {return nullptr;};

	virtual const Processor *getChildProcessor(int /*processorIndex*/) const override final {return nullptr;};

	virtual int getNumChildProcessors() const override final {return 0;};

	float getAttribute (int parameter_index) const override;
	void setInternalAttribute (int parameter_index, float newValue) override;

	void calculateBlock(int startSample, int numSamples) override
	{

		const bool smoothThisBlock = fabsf(targetValue - currentValue) > 0.001f;

		if(smoothThisBlock)
		{
			if(--numSamples >= 0)
			{
				currentValue =  smoother.smooth(targetValue);
				
				internalBuffer.setSample(0, startSample, currentValue);
				++startSample;
				
			}

			while(--numSamples >= 0)
			{
				currentValue =  smoother.smooth(targetValue);
				internalBuffer.setSample(0, startSample, currentValue);
				++startSample;
			}
		}
		else
		{
			currentValue = targetValue;
			FloatVectorOperations::fill(internalBuffer.getWritePointer(0, startSample), currentValue, numSamples);
		}

		if (useTable) sendTableIndexChangeMessage(false, table, inputValue);
		setOutputValue(currentValue);
	};

	/** sets the new target value if the controller number matches. */
	void handleHiseEvent(const HiseEvent &m) override;

	/** returns a pointer to the look up table. Don't delete it! */
	Table *getTable(int=0) const override {return table; };

	/** sets up the smoothing filter. */
	virtual void prepareToPlay(double sampleRate, int samplesPerBlock) override
	{
		TimeVariantModulator::prepareToPlay(sampleRate, samplesPerBlock);
		smoother.prepareToPlay(getControlRate());

		if(sampleRate != -1.0) setInternalAttribute(SmoothTime, smoothTime);
	};

private:

	bool mpeEnabled = false;

	/** Returns a smoothed value of the current control value. 
	*	Don't use this for GUI stuff, since it advances the smoothing! 
	*/
	float calculateNewValue();

	float targetValue;

	int64 uptime;

	float inputValue;

	// the smoothed version of the target value
	float currentValue;

	float lastCurrentValue;

	float intensity;
	
	bool inverted;

	float smoothTime;

	bool useTable;
	
	Smoother smoother;

	ScopedPointer<MidiTable> table;

};



} // namespace hise

#endif  // PITCHWHEELMODULATOR_H_INCLUDED
