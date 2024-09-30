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
#ifndef MACROCONTROLMODULATOR_H_INCLUDED
#define MACROCONTROLMODULATOR_H_INCLUDED

namespace hise { using namespace juce;

/** A Modulator that enhances the macro control system.
*	@ingroup macroControl
*
*	It can be used if the traditional way of controlling parameters via the macro controls are not sufficient.
*	These are the following features:
*
*	- Sample Accurate. While the standard macro control protocoll operates on block size level, this allow sample accurate timing.
*	- Parameter Smoothing. Whenever fast parameter changes are intended, this class should be used to prevent zipping noises.
*	- Lookup Tables. Use a table to modify the control function.
*/
class MacroModulator: public TimeVariantModulator,
					  public MacroControlledObject,
					  public LookupTableProcessor
{
public:

	SET_PROCESSOR_NAME("MacroModulator", "Macro Control Modulator", "A Modulator that enhances the macro control system.")

	/** The special Parameters for the Modulator. */
	enum SpecialParameters
	{
		MacroIndex = 0, ///< the macro index of the target macro control
		SmoothTime, ///< the smoothing time
		UseTable, ///< use a look up table for the value calculation
		MacroValue,
		numSpecialParameters
	};

	MacroModulator(MainController *mc, const String &id, Modulation::Mode m);;

	void restoreFromValueTree(const ValueTree &v) override;;

	ValueTree exportAsValueTree() const override;;
	
	ValueToTextConverter getValueToTextConverter() const override
	{
		return {};
	}

#if USE_BACKEND

	Path getSpecialSymbol() const override;;
		
#endif

	/** Returns a new ControlEditor */
	ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor)  override;

	virtual Processor *getChildProcessor(int /*processorIndex*/) override final {return nullptr;};

	virtual const Processor *getChildProcessor(int /*processorIndex*/) const override final {return nullptr;};

	virtual int getNumChildProcessors() const override final {return 0;};

	float getAttribute (int parameter_index) const override;
	void setInternalAttribute (int parameter_index, float newValue) override;

	/** sets the new target value if the controller number matches. */
	void handleHiseEvent(const HiseEvent &m) override;

	/** sets up the smoothing filter. */
	virtual void prepareToPlay(double sampleRate, int samplesPerBlock) override;;

	void calculateBlock(int startSample, int numSamples) override;;

	void macroControllerMoved(float newValue);

	NormalisableRange<double> getRange() const final override { return NormalisableRange<double>(0.0, 1.0); };

	void referenceShared(ExternalData::DataType, int) override
    {
        // no need to do anything...
    }

private:

	// Do nothing, since the data is updated anyway...
	void updateValue(NotificationType /*sendAttributeChange*/ = sendNotification) override {};

	void addToMacroController(int newMacroIndex) override;
	
	bool learnMode;

	Smoother smoother;

	int macroIndex;

	float currentMacroValue;

	float smoothTime;
	
	bool useTable;

	float inputValue;

	float currentValue;
	float targetValue;

};


} // namespace hise

#endif  // MACROCONTROLMODULATOR_H_INCLUDED
