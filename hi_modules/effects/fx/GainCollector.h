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

#ifndef GAINCOLLECTOR_H_INCLUDED
#define GAINCOLLECTOR_H_INCLUDED

namespace hise { using namespace juce;

class GainCollector : public MasterEffectProcessor
{
public:

	SET_PROCESSOR_NAME("GainCollector", "Gain Collector", "deprecated");

	/** The parameters */
	enum Parameters
	{
		Smoothing,
		Mode,
		Attack,
		Release,
		numEffectParameters
	};

	enum EnvelopeFollowerMode
	{
		SimpleLP = 0,
		AttackRelease,
		numEnvelopeFollowerModes
	};

	GainCollector(MainController *mc, const String &id);;

	float getAttribute(int parameterIndex) const override;;
	void setInternalAttribute(int parameterIndex, float newValue) override;;

	void restoreFromValueTree(const ValueTree &v) override;;
	ValueTree exportAsValueTree() const override;

	void prepareToPlay(double sampleRate, int samplesPerBlock) override;;
	void applyEffect(AudioSampleBuffer &buffer, int startSample, int numSamples) override;;
	bool hasTail() const override { return false; };

	int getNumChildProcessors() const override { return 0; };
	Processor *getChildProcessor(int /*processorIndex*/) override { return nullptr; };
	const Processor *getChildProcessor(int /*processorIndex*/) const override { return nullptr; };

	ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor)  override;

	float getCurrentGain() const { return currentGain; }

private:

	Smoother smoother;

	float smoothingTime;
	EnvelopeFollowerMode mode;
	float attack;
	float release;

	float currentGain;

};

} // namespace hise

#endif  // GAINCOLLECTOR_H_INCLUDED
