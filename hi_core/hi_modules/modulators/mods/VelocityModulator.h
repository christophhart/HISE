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

#ifndef VELOCITYMODULATOR_H_INCLUDED
#define VELOCITYMODULATOR_H_INCLUDED

namespace hise { using namespace juce;

/** This modulator changes the output depending on the velocity of note on messages.
*	@ingroup modulatorTypes
*
*/
class VelocityModulator: public VoiceStartModulator,
						 public LookupTableProcessor
{
public:

	SET_PROCESSOR_NAME("Velocity", "Velocity Modulator", "Creates a modulation value from the velocity of a incoming message.")

	/// Additional parameters
	enum SpecialParameters
	{
		Inverted = 0, ///< On, **Off** | if `true`, then the modulator works inverted, so that high velocity values are damped.
		UseTable, ///< On, **Off** | if `true` then a look up table is used to calculate the value
        DecibelMode,
		numTotalParameters
	};

	VelocityModulator(MainController *mc, const String &id, int voiceAmount, Modulation::Mode m):
		VoiceStartModulator(mc, id, voiceAmount, m),
		Modulation(m),
		LookupTableProcessor(mc, 1),
		tableUsed(false),
		inverted(false),
        decibelMode(false)
	{ 
        referenceShared(ExternalData::DataType::Table, 0);

		parameterNames.add("Inverted");
		parameterNames.add("UseTable");
        parameterNames.add("DecibelMode");

		updateParameterSlots();
	};

    void referenceShared(ExternalData::DataType, int) override
    {
        velocityTable = getTableUnchecked();
        velocityTable->setXTextConverter(Modulation::getDomainAsMidiRange);
    }
    
	void restoreFromValueTree(const ValueTree &v) override
	{
		VoiceStartModulator::restoreFromValueTree(v);

		loadAttribute(UseTable, "UseTable");
		loadAttribute(Inverted, "Inverted");
        loadAttribute(DecibelMode, "DecibelMode");

		if(tableUsed) loadTable(velocityTable, "VelocityTableData");
		
	};

	ValueTree exportAsValueTree() const override
	{
		ValueTree v = VoiceStartModulator::exportAsValueTree();

		saveAttribute(UseTable, "UseTable");
		saveAttribute(Inverted, "Inverted");
        saveAttribute(DecibelMode, "DecibelMode");

		if(tableUsed) saveTable(velocityTable, "VelocityTableData");

		return v;
	}

	void setInternalAttribute(int p, float newValue) override
	{
		switch(p)
		{
		case Inverted:
			inverted = newValue == 1.0f;
			break;
		case UseTable:
			tableUsed = newValue == 1.0f;
			break;
        case DecibelMode:
            decibelMode = newValue >= 0.5f;
            break;
                
		}
	}

	float getAttribute(int p) const
	{
		switch(p)
		{
		case Inverted:	return inverted;
		case UseTable:	return tableUsed;
        case DecibelMode: return decibelMode ? 1.0f : 0.0f;

		// This should not happen!
		default: jassertfalse; return 0.0f;
		}
	}
	
	ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor)  override;


	float calculateVoiceStartValue(const HiseEvent &m) override
	{
		float value = m.getFloatVelocity();

		if(inverted) value = 1.0f - value;

		if (tableUsed)
            value = velocityTable->getInterpolatedValue(value, sendNotificationAsync);
			
		if(decibelMode)
        {
            const float decibelValue = -100.0f + 100.0f * value;
            value = Decibels::decibelsToGain(decibelValue);
        }
        
		return value;
	};

	/// \brief enables the look up table
	void setUseTable(bool enableLookUpTable)
	{
		tableUsed = enableLookUpTable;
	};

private:

	float inputValue;

	SampleLookupTable* velocityTable;

	/// checks if the look up table should be used
	bool tableUsed;

	bool inverted;
    
    bool decibelMode;

	ScopedPointer<XmlElement> tableGraph;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VelocityModulator);
	JUCE_DECLARE_WEAK_REFERENCEABLE(VelocityModulator);
};


} // namespace hise

#endif  // VELOCITYMODULATOR_H_INCLUDED
