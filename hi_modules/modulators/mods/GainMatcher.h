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

	ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor)  override;

	void setInternalAttribute(int parameterIndex, float newValue) override;;
	float getAttribute(int parameterIndex) const override;;

	float calculateVoiceStartValue(const HiseEvent& ) override;;

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
	void handleHiseEvent(const HiseEvent &m) override;
	virtual void prepareToPlay(double sampleRate, int samplesPerBlock) override;;

	ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor)  override;

private:

	float calculateNewValue();

	float currentValue;

	Ramper r;

	bool useTable;

};



#endif  // GAINMATCHER_H_INCLUDED
