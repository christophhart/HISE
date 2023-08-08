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
#ifndef SIMPLEREVERB_H_INCLUDED
#define SIMPLEREVERB_H_INCLUDED

namespace hise { using namespace juce;

/** A simple (and cheap sounding) reverb effect
*	@ingroup effectTypes
*
*	This is the Freeverb algorithm found in JUCE wrapped into a HISE effect. It does not sound very good
*	compared to commercially available reverb plugins, but for simple stuff, it is still useful.
*/
class SimpleReverbEffect: public MasterEffectProcessor
{
public:

	SET_PROCESSOR_NAME("SimpleReverb", "Simple Reverb", "a algorithmic reverb based on Freeverb.");

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
		finaliseModChains();

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
		MasterEffectProcessor::prepareToPlay(sampleRate, samplesPerBlock);

		reverb.setSampleRate(sampleRate);
		reverb.reset();
	};

	void voicesKilled() override
	{
		KILL_LOG("Kill Reverb");
		reverb.reset();
	}



	void applyEffect(AudioSampleBuffer &buffer, int startSample, int numSamples) override
	{
		reverb.processStereo(buffer.getWritePointer(0, startSample), buffer.getWritePointer(1, startSample), numSamples);

		buffer.applyGain(0.5f);
	};

	bool hasTail() const override {return true; };

	bool isSuspendedOnSilence() const final override { return true; }
	
	int getNumChildProcessors() const override { return 0; };

	Processor *getChildProcessor(int /*processorIndex*/) override { return nullptr; };

	const Processor *getChildProcessor(int /*processorIndex*/) const override { return nullptr; };

	ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor)  override;

	
private:

	Reverb reverb;
	Reverb::Parameters parameters;
};




} // namespace hise

#endif  // SIMPLEREVERB_H_INCLUDED
