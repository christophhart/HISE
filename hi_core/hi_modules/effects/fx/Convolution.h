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

#pragma once

namespace hise { using namespace juce;




/** @brief A convolution reverb using zero-latency convolution
*	@ingroup effectTypes
*
*/
class ConvolutionEffect: public MasterEffectProcessor,
						 public AudioSampleProcessor,
						 public MultiChannelAudioBuffer::Listener,
						 public ConvolutionEffectBase
{

public:

	static ProcessorMetadata createMetadata();

	SET_PROCESSOR_NAME("Convolution", "Convolution Reverb", "");

	// ============================================================================================= Constructor / Destructor / enums

	enum Parameters
	{
		DryGain = 0,
		WetGain,
		Latency,
		ImpulseLength,
		ProcessInput,
		UseBackgroundThread,
		Predelay,
		HiCut,
		Damping,
		FFTType,
		numEffectParameters
	};

	ConvolutionEffect(MainController *mc, const String &id);;

	~ConvolutionEffect();
	
	// ============================================================================================= Convolution methods

	void bufferWasLoaded() override
	{
		setImpulse(sendNotificationSync);
	}

	void bufferWasModified() override
	{
		setImpulse(sendNotificationSync);
	}

	MultiChannelAudioBuffer& getImpulseBufferBase() override { return getBuffer(); }
	const MultiChannelAudioBuffer& getImpulseBufferBase() const override { return getBuffer(); }

	// ============================================================================================= MasterEffect methods

	float getAttribute(int parameterIndex) const override;;
	void setInternalAttribute(int parameterIndex, float newValue) override;;

	void restoreFromValueTree(const ValueTree &v) override;;
	ValueTree exportAsValueTree() const override;

	void prepareToPlay(double sampleRate, int samplesPerBlock) override;;
	void applyEffect(AudioSampleBuffer &buffer, int startSample, int numSamples) override;;
	bool hasTail() const override {return true; };
	bool isSuspendedOnSilence() const override { return true; }

	void voicesKilled() override;

	int getNumChildProcessors() const override { return 0; };
	Processor *getChildProcessor(int /*processorIndex*/) override { return nullptr; };
	const Processor *getChildProcessor(int /*processorIndex*/) const override { return nullptr; };

	ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor)  override;

};


} // namespace hise



