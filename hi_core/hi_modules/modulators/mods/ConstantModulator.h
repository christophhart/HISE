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

#ifndef CONSTANTMODULATOR_H_INCLUDED
#define CONSTANTMODULATOR_H_INCLUDED

namespace hise { using namespace juce;

/** This modulator simply returns a constant value that can be used to change the gain or something else.
*
*	@ingroup modulatorTypes
*/
class ConstantModulator: public VoiceStartModulator
{
public:

	SET_PROCESSOR_NAME("Constant", "Constant", "creates a constant modulation signal (1.0).");

	/// Additional Parameters for the constant modulator
	enum SpecialParameters
	{
		numTotalParameters
	};

	ConstantModulator(MainController *mc, const String &id, int numVoices, Modulation::Mode m):
		VoiceStartModulator(mc, id, numVoices, m),
		Modulation(m)
	{ };

	ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor)  override;

	/// sets the constant value. The only valid parameter_index is Intensity
	void setInternalAttribute(int, float ) override
	{
	};

	/// returns the constant value
	float getAttribute(int ) const override
	{
		return 0.0f;
	};

	

	/** Returns the 0.0f and let the intensity do it's job. */
	float calculateVoiceStartValue(const HiseEvent& ) override
	{
		return (getMode() == GainMode) ? 0.0f : 1.0f;
	};

private:

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ConstantModulator);
	JUCE_DECLARE_WEAK_REFERENCEABLE(ConstantModulator);

};


} // namespace hise

#endif  // CONSTANTMODULATOR_H_INCLUDED
