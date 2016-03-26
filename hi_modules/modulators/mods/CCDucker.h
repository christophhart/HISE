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
*   along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
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

#ifndef CCDUCKER_H_INCLUDED
#define CCDUCKER_H_INCLUDED

class CCDucker : public TimeVariantModulator,
					   public LookupTableProcessor
{
public:

	SET_PROCESSOR_NAME("CCDucker", "CC Ducker")

		/** The special Parameters for the Modulator. */
	enum SpecialParameters
	{
		CCNumber = 0,
		DuckingTime,
		SmoothingTime,
		numSpecialParameters
	};

	CCDucker(MainController *mc, const String &id, Modulation::Mode m);;

	~CCDucker() { table = nullptr; }

	void restoreFromValueTree(const ValueTree &v) override;;

	ValueTree exportAsValueTree() const override;;

	/** Returns a new ControlEditor */
	ProcessorEditorBody *createEditor(BetterProcessorEditor *parentEditor)  override;

	virtual Processor *getChildProcessor(int /*processorIndex*/) override final { return nullptr; };
	virtual const Processor *getChildProcessor(int /*processorIndex*/) const override final { return nullptr; };
	virtual int getNumChildProcessors() const override final { return 0; };

	float getDefaultValue(int parameterIndex) const override;
	float getAttribute(int parameterIndex) const override;
	void setInternalAttribute(int parameterIndex, float newValue) override;

	/** sets the new target value if the controller number matches. */
	void handleMidiEvent(const MidiMessage &m) override;

	/** returns a pointer to the look up table. Don't delete it! */
	Table *getTable(int = 0) const override { return table; };

	/** sets up the smoothing filter. */
	virtual void prepareToPlay(double sampleRate, int samplesPerBlock) override;

	void calculateBlock(int startSample, int numSamples) override;;
	
private:

	void setDuckingTime(float newValue);
	float getNextValue();
	Smoother smoother;

	float smoothTime;
	float attackTime;
	int ccNumber;

	float currentValue;
	float targetValue;

	float currentUptime;
	float uptimeDelta;

	ScopedPointer<SampleLookupTable> table;

};




#endif  // CCDUCKER_H_INCLUDED
