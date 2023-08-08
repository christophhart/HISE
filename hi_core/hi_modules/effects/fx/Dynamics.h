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

#ifndef DYNAMICS_H_INCLUDED
#define DYNAMICS_H_INCLUDED

namespace hise { using namespace juce;

/** A general purpose dynamics processor based on chunkware's SimpleCompressor.
	@ingroup effectTypes
*/
class DynamicsEffect : public MasterEffectProcessor
{
public:

	SET_PROCESSOR_NAME("Dynamics", "Dynamics", "A general purpose dynamics processor based on chunkware's SimpleCompressor");

		enum Parameters
	{
		GateEnabled,
		GateThreshold,
		GateAttack,
		GateRelease,
		GateReduction,
		CompressorEnabled,
		CompressorThreshold,
		CompressorRatio,
		CompressorAttack,
		CompressorRelease,
		CompressorReduction,
		CompressorMakeup,
		LimiterEnabled,
		LimiterThreshold,
		LimiterAttack,
		LimiterRelease,
		LimiterReduction,
		LimiterMakeup,
		numParameters
	};

	DynamicsEffect(MainController *mc, const String &uid);;

	~DynamicsEffect()
	{};

	void setInternalAttribute(int parameterIndex, float newValue) override;;
	float getAttribute(int parameterIndex) const override;
	float getDefaultValue(int parameterIndex) const override;

	void restoreFromValueTree(const ValueTree &v) override;;
	ValueTree exportAsValueTree() const override;

	bool hasTail() const override { return false; };

	ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor)  override;

	const Processor* getChildProcessor(int /*processorIndex*/) const { return nullptr; };
	Processor* getChildProcessor(int /*processorIndex*/) { return nullptr; };
	int getNumChildProcessors() const { return 0; };

	void applyEffect(AudioSampleBuffer &buffer, int startSample, int numSamples) override;

	void applyLimiter(AudioSampleBuffer &buffer, int startSample, const int numToProcess);

	void prepareToPlay(double sampleRate, int samplesPerBlock) override;

private:

	void updateMakeupValues(bool updateLimiter);

	chunkware_simple::SimpleGate gate;
	chunkware_simple::SimpleComp compressor;
	chunkware_simple::SimpleLimit limiter;

	std::atomic<bool> gateEnabled;
	std::atomic<bool> compressorEnabled;
	std::atomic<bool> limiterEnabled;
	std::atomic<bool> limiterPending;

	std::atomic<bool> compressorMakeup;
	std::atomic<bool> limiterMakeup;

	std::atomic<float> gateReduction;
	std::atomic<float> limiterReduction;
	std::atomic<float> compressorReduction;

	std::atomic<float> compressorMakeupGain;
	std::atomic<float> limiterMakeupGain;
};

} // namespace hise

#endif  // DYNAMICS_H_INCLUDED
