/*
  ==============================================================================

    SimpleReverb.h
    Created: 3 Aug 2014 1:25:20pm
    Author:  Chrisboy

  ==============================================================================
*/



#ifndef SIMPLEREVERB_H_INCLUDED
#define SIMPLEREVERB_H_INCLUDED

/** A simple (and cheap sounding) reverb effect
*	@ingroup effectTypes
*
*	This is the Freeverb algorithm found in JUCE wrapped into a HISE effect. It does not sound very good
*	compared to commercially available reverb plugins, but for simple stuff, it is still useful.
*/
class SimpleReverbEffect: public MasterEffectProcessor
{
public:

	SET_PROCESSOR_NAME("SimpleReverb", "Simple Reverb");

	/** The parameters */
	enum Parameters
	{
		RoomSize = 0, ///< the room size
		Damping, ///< the damping
		WetLevel, ///< the wet level
		DryLevel, ///< the dry level
		Width, ///< the stereo width
		FreezeMode, ///< freeze mode (unused)
		numEffectParameters
	};

	SimpleReverbEffect(MainController *mc, const String &id):
		MasterEffectProcessor(mc, id)
	{
		parameterNames.add("RoomSize");
		parameterNames.add("Damping");
		parameterNames.add("WetLevel");
		parameterNames.add("DryLevel");
		parameterNames.add("Width");
		parameterNames.add("FreezeMode");

		parameters.damping = 0.6f;
		parameters.roomSize = 0.8f;
		parameters.wetLevel = 0.2f;
		parameters.dryLevel = 0.8f;
		parameters.width = 0.8f;
		parameters.freezeMode = 0.1f;
		
		reverb.setParameters(parameters);
	};

	float getAttribute(int parameterIndex) const override
	{
		switch ( parameterIndex )
		{
		case RoomSize:		return parameters.roomSize;
		case Damping:		return parameters.damping;
		case WetLevel:		return parameters.wetLevel;
		case DryLevel:		return parameters.dryLevel;
		case Width:			return parameters.width;
		case FreezeMode:	return parameters.freezeMode;
		default:			jassertfalse; return 1.0f;
		}
	};

	void setInternalAttribute(int parameterIndex, float newValue) override 
	{
		switch ( parameterIndex )
		{
		case RoomSize:		parameters.roomSize = newValue; break; 
		case Damping:		parameters.damping = newValue; break;
		case WetLevel:		parameters.wetLevel = newValue; 
							parameters.dryLevel = 1.0f - newValue; break;
		case DryLevel:		break;
		case Width:			parameters.width = newValue; break;
		case FreezeMode:	parameters.freezeMode = newValue; break;
		default:			jassertfalse; 
		}

		reverb.setParameters(parameters);

	};

	void restoreFromValueTree(const ValueTree &v) override
	{
		MasterEffectProcessor::restoreFromValueTree(v);

		loadAttribute(RoomSize, "RoomSize");
		loadAttribute(Damping, "Damping");
		loadAttribute(WetLevel, "WetLevel");
		loadAttribute(DryLevel, "DryLevel");
		loadAttribute(Width, "Width");
		loadAttribute(FreezeMode, "FreezeMode");
		
	};

	ValueTree exportAsValueTree() const override
	{
		ValueTree v = MasterEffectProcessor::exportAsValueTree();

		saveAttribute(RoomSize, "RoomSize");
		saveAttribute(Damping, "Damping");
		saveAttribute(WetLevel, "WetLevel");
		saveAttribute(DryLevel, "DryLevel");
		saveAttribute(Width, "Width");
		saveAttribute(FreezeMode, "FreezeMode");

		return v;

	}

	void prepareToPlay(double sampleRate, int samplesPerBlock) override
	{
		EffectProcessor::prepareToPlay(sampleRate, samplesPerBlock);

		reverb.setSampleRate(sampleRate);
		reverb.reset();
	};

	void applyEffect(AudioSampleBuffer &buffer, int startSample, int numSamples) override
	{
		const bool inputSilent = buffer.getMagnitude(startSample, numSamples) == 0.0f;

		if(!inputSilent || tailActive)
		{
			reverb.processStereo(buffer.getWritePointer(0, startSample), buffer.getWritePointer(1, startSample), numSamples);

			const float outputLevel = buffer.getMagnitude(startSample, numSamples);
			const bool outputSilent = outputLevel <= 0.0001f;
			tailActive = !outputSilent;
		}

	};

	bool hasTail() const override {return false; };


	int getNumChildProcessors() const override { return 0; };

	Processor *getChildProcessor(int /*processorIndex*/) override { return nullptr; };

	const Processor *getChildProcessor(int /*processorIndex*/) const override { return nullptr; };

	ProcessorEditorBody *createEditor(BetterProcessorEditor *parentEditor)  override;

	
private:

	bool tailActive;

	Reverb reverb;
	Reverb::Parameters parameters;
};





#endif  // SIMPLEREVERB_H_INCLUDED
