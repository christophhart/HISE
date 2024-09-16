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

#ifndef SATURATOR_H_INCLUDED
#define SATURATOR_H_INCLUDED

namespace hise { using namespace juce;

/** A simple saturator effect. Use the ShapeFX class for more control.
	@ingroup effectTypes.
*/
class SaturatorEffect : public MasterEffectProcessor
{
public:

	SET_PROCESSOR_NAME("Saturator", "Saturator", "Applies a simple saturation effect")

	enum InternalChains
	{
		SaturationChain = 0,
		numInternalChains
	};

	enum EditorStates
	{
		SaturationChainShown = Processor::numEditorStates,
		numEditorStates
	};

	enum Parameters
	{
		Saturation = 0,
		WetAmount,
		PreGain,
		PostGain,
		numParameters
	};

	SaturatorEffect(MainController *mc, const String &uid);;

	~SaturatorEffect()
	{};

	void setInternalAttribute(int parameterIndex, float newValue) override;;
	float getAttribute(int parameterIndex) const override;
	float getDefaultValue(int parameterIndex) const override;

	void restoreFromValueTree(const ValueTree &v) override;;
	ValueTree exportAsValueTree() const override;

	bool hasTail() const override { return false; };

	Processor *getChildProcessor(int /*processorIndex*/) override { return saturationChain; };
	const Processor *getChildProcessor(int /*processorIndex*/) const override { return saturationChain; };
	int getNumInternalChains() const override { return numInternalChains; };
	int getNumChildProcessors() const override { return numInternalChains; };

	ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor)  override;

	void applyEffect(AudioSampleBuffer &buffer, int startSample, int numSamples) override;

private:

	float dry;
	float wet;
	float saturation;
	float preGain;
	float postGain;

	Saturator saturator;

	ModulatorChain* saturationChain;
};


} // namespace hise

#endif  // SATURATOR_H_INCLUDED
