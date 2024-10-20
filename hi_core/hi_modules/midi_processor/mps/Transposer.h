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

#ifndef TRANSPOSER_H_INCLUDED
#define TRANSPOSER_H_INCLUDED

namespace hise { using namespace juce;

class ChokeGroupProcessor : public MidiProcessor,
							public EventIdHandler::ChokeListener
{
public:

	enum SpecialParameters
	{
		ChokeGroup = 0,
		LoKey,
		HiKey,
		KillVoice
	};

	SET_PROCESSOR_NAME("ChokeGroupProcessor", "Choke Group Processor", "Kills notes when another choke group processor receives a note on.");

	ChokeGroupProcessor(MainController *mc, const String &id) :
		MidiProcessor(mc, id)
	{
		mc->getEventHandler().addChokeListener(this);

		parameterNames.add("ChokeGroup");
		parameterNames.add("LoKey");
		parameterNames.add("HiKey");
		parameterNames.add("KillVoice");

		updateParameterSlots();
	};

	~ChokeGroupProcessor()
	{
		getMainController()->getEventHandler().removeChokeListener(this);
	}

	ValueTree exportAsValueTree() const override
	{
		ValueTree v = MidiProcessor::exportAsValueTree();

		saveAttribute(ChokeGroup, "ChokeGroup");
		saveAttribute(LoKey, "LoKey");
		saveAttribute(HiKey, "HiKey");
		saveAttribute(KillVoice, "KillVoice");
		
		return v;
	};

	virtual void restoreFromValueTree(const ValueTree &v) override
	{
		MidiProcessor::restoreFromValueTree(v);

		loadAttribute(ChokeGroup, "ChokeGroup");
		loadAttribute(LoKey, "LoKey");
		loadAttribute(HiKey, "HiKey");
		loadAttribute(KillVoice, "KillVoice");
	}

	ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor)  override;

	float getDefaultValue(int parameterIndex) const override
	{
		switch (parameterIndex)
		{
		case SpecialParameters::ChokeGroup: return 0.0f;
		case SpecialParameters::HiKey: return 127.0f;
		case SpecialParameters::LoKey: return 0.0f;
		case SpecialParameters::KillVoice: return 1.0f;
        default: jassertfalse; return 0.0f;
		}
	};

	float getAttribute(int parameterIndex) const override
	{
		switch (parameterIndex)
		{
		case SpecialParameters::ChokeGroup:  return (float)getChokeGroup();
		case SpecialParameters::HiKey: return (float)midiRange.getEnd();
		case SpecialParameters::LoKey: return (float)midiRange.getStart();
		case SpecialParameters::KillVoice: return (float)(int)killVoice;
        default: jassertfalse; return 0.0f;
		}
	}

	void setInternalAttribute(int parameterIndex, float newAmount) override
	{
		switch (parameterIndex)
		{
		case SpecialParameters::ChokeGroup: 
			setChokeGroup(jlimit(0, 16, (int)newAmount));
			break;
		case SpecialParameters::HiKey: 
			midiRange.setEnd((int)newAmount);
			break;
		case SpecialParameters::LoKey: 
			midiRange.setStart((int)newAmount);
			break;
		case SpecialParameters::KillVoice: 
			killVoice = newAmount > 0.5f;
		}
	};

	void prepareToPlay(double sampleRate_, int samplesPerBlock_) override
	{
		MidiProcessor::prepareToPlay(sampleRate_, samplesPerBlock_);
	}

	void processHiseEvent(HiseEvent &m) noexcept override;

	void chokeMessageSent() override;

private:

	bool sustainPedalPressed = false;
	bool killVoice = true;
	Range<int> midiRange;
	hise::UnorderedStack<HiseEvent, NUM_POLYPHONIC_VOICES> activeEvents;
	hise::UnorderedStack<HiseEvent, NUM_POLYPHONIC_VOICES> sustainedEvents;
};

/** Transposes all midi note messages by the specified amount.
*	@ingroup midiTypes
*/
class Transposer: public MidiProcessor
{
public:

	SET_PROCESSOR_NAME("Transposer", "Transposer", "Transposes all midi note messages by the specified amount.");

	Transposer(MainController *mc, const String &id):
		MidiProcessor(mc, id),
		transposeAmount(0)
	{
		parameterNames.add("TransposeAmount");
		updateParameterSlots();
	};


	~Transposer()
	{
		transposeAmount = 0;
	}

	enum SpecialParameters
	{
		TransposeAmount = 0
	};

	ValueTree exportAsValueTree() const override
	{
		ValueTree v = MidiProcessor::exportAsValueTree(); 

		saveAttribute(TransposeAmount , "TransposeAmount");
		
		return v;
	};

	virtual void restoreFromValueTree(const ValueTree &v) override
	{
		MidiProcessor::restoreFromValueTree(v);
    
		loadAttribute(TransposeAmount , "TransposeAmount");
	}

	ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor)  override;

	

	float getAttribute(int) const
	{
		return (float)transposeAmount;
	};

	void setInternalAttribute(int, float newAmount) override
	{
		transposeAmount = (int)newAmount;
	};

	void processHiseEvent(HiseEvent &m) noexcept override
	{
		if (m.isNoteOn())
		{
			m.setTransposeAmount(m.getTransposeAmount() + transposeAmount);
		}
	}

private:

	int transposeAmount;

};

} // namespace hise

#endif  // TRANSPOSER_H_INCLUDED
