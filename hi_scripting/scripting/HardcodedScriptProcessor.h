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
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

#ifndef HARDCODEDSCRIPTPROCESSOR_H_INCLUDED
#define HARDCODEDSCRIPTPROCESSOR_H_INCLUDED

namespace hise { using namespace juce;

class ModulatorSynth;

/** The base class for all MidiProcessors that are hardcoded scripts.
*	@ingroup midiProcessor
*
*	You can use a ScriptProcessor to develope the functionality and then copy the callbacks from the 
*	script to the virtual methods in order to have a hardcoded C++ Processor.
*	
*	You obviously have to change some language specific stuff (eg. 'var' 'function' ...), but 
*	this class tries to smooth out the process of converting as much as possible.
*/
class HardcodedScriptProcessor: public ScriptBaseMidiProcessor
{
public:

	HardcodedScriptProcessor(MainController *mc, const String &id, ModulatorSynth *ms);
	virtual ~HardcodedScriptProcessor();;

	/** This callback will get called if the object is created or resetted. */
	virtual void onInit() {};

	/** called if a note on message is processed. */
	virtual void onNoteOn() {};

	/** called if a note off message is processed. */
	virtual void onNoteOff() {};

	/** called if a cc message is processed. */
	virtual void onController() {} ;

	/** called periodically if the timer was started. */
	virtual void onTimer(int /*offsetInBuffer*/) {};

	/** called whenever a script control was moved. */
	virtual void onControl(ScriptingApi::Content::ScriptComponent * /*controller*/, var /*value*/) {};

    void restoreFromValueTree(const ValueTree &v) override
    {
        jassert(content.get() != nullptr);
        
        MidiProcessor::restoreFromValueTree(v);
        
		onInit();

		ScriptBaseMidiProcessor::restoreContent(v);

        if(content.get() != nullptr)
        {
            for(int i = 0; i < content->getNumComponents(); i++)
            {
                controlCallback(content->getComponent(i), content->getComponent(i)->getValue());
            }
        }
    }

protected:

	void controlCallback(ScriptingApi::Content::ScriptComponent *component, var controllerValue) override
	{
		onControl(component, controllerValue);
	}

	
	ReferenceCountedObjectPtr<ScriptingApi::Content> refCountedContent;

	ScriptingApi::Message Message;
	ScriptingApi::Synth Synth;
	ScriptingApi::Console Console;
	ScriptingApi::Engine Engine;
	ScriptingApi::Content& Content;
	ScriptingApi::Sampler Sampler;

	void processHiseEvent(HiseEvent &m) override;

	ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor)  override;
	
private:

};



/** This Processor is a demonstration of how to convert a script into a MidiProcessor.
*
*	It is a simple transpose function. Two things had to be changed manually:
*	
*	- make a member variable transposeValue
*	- typecast the var from getValue() 
*/
class DemoHardcodedScriptProcessor: public HardcodedScriptProcessor
{
public:

	DemoHardcodedScriptProcessor(MainController *mc, const String &id, ModulatorSynth *ms):
		HardcodedScriptProcessor(mc, id, ms)
	{
		onInit();	
	};

	void onInit() override
	{
		transposeButton = Content.addKnob("Transpose", 0, 0);
		transposeButton->setRange(-12, 12, 1);
		Content.setHeight(50);

		Synth.startTimer(0.8);
	}

	void onNoteOn() override
	{
		Message.setNoteNumber(Message.getNoteNumber() + (int)transposeButton->getValue());
	};

	void onNoteOff() override
	{
		Message.setNoteNumber(Message.getNoteNumber() + (int)transposeButton->getValue());
	}
	
	void onControl(ScriptingApi::Content::ScriptComponent * , var ) override
	{
		Engine.allNotesOff();
	}

	void onTimer(int /*offsetInBuffer*/)
	{
		Console.print(transposeButton->getValue());
	}

	const Identifier getType() const override { return "DemoProcessor";};
	
private:

	ScriptingApi::Content::ScriptSlider *transposeButton;

};

/** Enables monophonic mode and retriggers the pressed key if there was a key release. 
*	@ingroup midiTypes	
*/
class LegatoProcessor: public HardcodedScriptProcessor
{
public:

	SET_PROCESSOR_NAME("LegatoWithRetrigger", "Legato with Retrigger");

	LegatoProcessor(MainController *mc, const String &id, ModulatorSynth *ms):
		HardcodedScriptProcessor(mc, id, ms)
	{
		onInit();	
	};

	void onInit() override
	{
		lastNote = -1;
		lastEventId = -1;
		possibleRetriggerNote = -1;		
		lastVelo = 0;
	}

	void onNoteOn() override
	{
		Message.ignoreEvent(true);
		
		int newID = Synth.playNote(Message.getNoteNumber(), Message.getVelocity());

		if(lastNote != -1)
		{
			Synth.noteOffByEventId(lastEventId);
		
			possibleRetriggerNote = lastNote;
		}
	
		
		lastEventId = newID;

		lastNote = Message.getNoteNumber();
		lastVelo = Message.getVelocity();
	};

	void onNoteOff() override
	{
		if (Message.getNoteNumber() == lastNote)
		{
			Message.ignoreEvent(true);
			Synth.noteOffByEventId(lastEventId);
		}

		int number = Message.getNoteNumber();
	
		if(number == possibleRetriggerNote)
		{
			possibleRetriggerNote = -1;
		}
	
		if(number == lastNote)
		{
			if(possibleRetriggerNote != -1)
			{
				lastEventId = Synth.playNote(possibleRetriggerNote, lastVelo);
			
				lastNote = possibleRetriggerNote;
				possibleRetriggerNote = -1;
			}
			else
			{
				lastNote = -1;
			}
		}
	}
	


	
	
private:

	int lastNote;
	int lastEventId;
	int possibleRetriggerNote;
	int lastVelo;

};

/** Swaps two control change numbers. @ingroup midiTypes */
class CCSwapper: public HardcodedScriptProcessor
{
public:

	SET_PROCESSOR_NAME("CCSwapper", "CC Swapper");

	CCSwapper(MainController *mc, const String &id, ModulatorSynth *ms):
		HardcodedScriptProcessor(mc, id, ms)
	{
		onInit();	
	};

	void onInit() override
	{
		firstCC = Content.addKnob("FirstCC", 0, 0);
		secondCC = Content.addKnob("SecondCC", 140, 0);

		firstCC->setRange(0, 127, 1);
		secondCC->setRange(0, 127, 1);

		Content.setHeight(50);
		Content.setName("CC Swapper");
	}

	void onController() override
	{
		if(Message.getControllerNumber() == firstCC->getValue())
		{
			Message.setControllerNumber(secondCC->getValue());
		}
		else if(Message.getControllerNumber() == secondCC->getValue())
		{
			Message.setControllerNumber(firstCC->getValue());
		}
	};

	
	
private:

	ScriptingApi::Content::ScriptSlider *firstCC;
	ScriptingApi::Content::ScriptSlider *secondCC;

};

/** allows release trigger functionality with a time variant decrease of the velocity. @ingroup midiTypes */
class ReleaseTriggerScriptProcessor: public HardcodedScriptProcessor
{
public:

	SET_PROCESSOR_NAME("ReleaseTrigger", "Release Trigger");

	ReleaseTriggerScriptProcessor(MainController *mc, const String &id, ModulatorSynth *ms):
		HardcodedScriptProcessor(mc, id, ms)
	{
		onInit();	
	};

	void onInit() override
	{
		Content.setHeight(100);
		Content.setColour(140, 120, 200);
		Content.setName("Release Trigger");

		enableButton = Content.addButton("TimeAttenuate", 0, 0);
		timeKnob = Content.addKnob("Time", 0, 50);
		timeKnob->setRange(0, 20, 0.1);
		table = Content.addTable("TimeTable", 140, 0);

		table->enableAllocationFreeMessages(30);
		table->setPosition(140, 0, 480, 100);

		attenuationLevel = 1.0; 
		timeIndex = 0;

		for(int i = 0; i < 127; i++)
		{
			velocityValues[i] = 0;
			lengthValues[i] = 0.0;
		}
	}

	void onNoteOn() override
	{
		Message.ignoreEvent(true);
		velocityValues[Message.getNoteNumber()] = Message.getVelocity();
		lengthValues[Message.getNoteNumber()] = Engine.getUptime();
	};

	void onNoteOff() override
	{
		Message.ignoreEvent(true);
	
		if((int)enableButton->getValue() == 1)
		{
			double time = Engine.getUptime() - lengthValues[Message.getNoteNumber()];
		
			timeIndex = (int)(time / (double)(timeKnob->getValue()) * 127.0);
		
			if(timeIndex > 127) timeIndex = 127;
			if(timeIndex < 0) timeIndex = 0;
		
			attenuationLevel = table->getTableValue(timeIndex);
		}
		else
		{
			attenuationLevel = 1.0;
		}
	
		int v = (int)(velocityValues[Message.getNoteNumber()] * attenuationLevel);

		if(v > 0)
			Synth.playNote(Message.getNoteNumber(), v);
	}
	
	void onControl(ScriptingApi::Content::ScriptComponent *component , var value ) override
	{
		if(component == enableButton)
		{
			timeKnob->showControl(value);
			table->showControl(value);
		}
	};
	
private:

	ScriptingApi::Content::ScriptButton *enableButton;
	ScriptingApi::Content::ScriptSlider *timeKnob;
	ScriptingApi::Content::ScriptTable *table;
	double attenuationLevel;
	int timeIndex;
	double lengthValues[128];
	int velocityValues[128];
};


class CCToNoteProcessor : public HardcodedScriptProcessor
{
public:

	SET_PROCESSOR_NAME("CC2Note", "MIDI CC to Note Generator");

	CCToNoteProcessor(MainController *mc, const String &id, ModulatorSynth *ms) :
		HardcodedScriptProcessor(mc, id, ms)
	{
		onInit();
	};

	void onInit() override
	{
		lastNote = -1;
        rrAmount = 1;
		lastGroup = 0;
		thisGroup = 0;
		selectorValue = 0;
		updown = false;

		r.setSeedRandomly();

		Content.setHeight(80);

		bypassButton = Content.addButton("Bypass", 0, 0);

		ccSelector = Content.addComboBox("ccSelector", 309, 15);
		

		for (int i = 1; i < 127; i++)
		{
			ccSelector->addItem("CC " + String(i));
		}

		try
        {
            Sampler.enableRoundRobin(false);
        }
        catch(String& errorMessage)
        {
			Logger::writeToLog(errorMessage);
        }

	}

	void onNoteOn() override
	{
		lastNote = Message.getNoteNumber();
		Message.ignoreEvent(true);
	};

	void onNoteOff() override
	{
		Message.ignoreEvent(true);
		if (Message.getNoteNumber() == lastNote)
		{
			lastNote = -1;
		}
	};

	void onController() override
	{
		if (lastNote != -1 && ((int)Message.getControllerNumber() == selectorValue))
		{
			if ((double)bypassButton->getValue() < 0.5)
			{
				updown = !updown;

				if (!updown)
				{
					
					thisGroup = (int)floor(r.nextFloat() * (float)(rrAmount) / 2.0f);

					while (thisGroup == lastGroup) thisGroup = (int)(floor(r.nextFloat() * (float)(rrAmount) / 2.0f));

					lastGroup = thisGroup;
				}

				const int thisGroupNumber = thisGroup * 2 + (updown ? 1 : 0) + 1;

				Sampler.setActiveGroup(thisGroupNumber);
			}

			if (lastNote != -1)
			{
				Synth.playNote(lastNote, Message.getControllerValue());
			}
		}
	};

	void onControl(ScriptingApi::Content::ScriptComponent * controllerThatWasMoved, var value) override
	{
		rrAmount = (int)Synth.getAttribute(7);

		if (controllerThatWasMoved == bypassButton)
		{
			Sampler.enableRoundRobin((double)value > 0.5);
		}
		else if (controllerThatWasMoved == ccSelector)
		{
			selectorValue = (int)value;
		}
	};

private:

	ScriptingApi::Content::ScriptButton *bypassButton;
	ScriptingApi::Content::ScriptComboBox *ccSelector;

	Random r;

	int selectorValue;
	int lastNote;
	int rrAmount;
	int lastGroup;
	int thisGroup;
	bool updown;

	
};

class ChannelFilterScriptProcessor : public HardcodedScriptProcessor
{
public:

	SET_PROCESSOR_NAME("ChannelFilter", "MIDI Channel Filter");

	ChannelFilterScriptProcessor(MainController *mc, const String &id, ModulatorSynth *ms) :
		HardcodedScriptProcessor(mc, id, ms)
	{
		onInit();
	};

	void onInit() override
	{
		Content.setHeight(50);

		channelNumber = Content.addKnob("channelNumber", 0, 0);
		channelNumber->set("text", "MIDI Channel");
		channelNumber->setRange(1, 16, 1);

		channel = 1;
	}

	void onNoteOn() override
	{
		if (Message.getChannel() != channel)
		{
			Message.ignoreEvent(true);
		}
	};

	void onNoteOff() override
	{
		if (Message.getChannel() != channel)
		{
			Message.ignoreEvent(true);
		}
	};

	void onController() override
	{
		if (Message.getChannel() != channel)
		{
			Message.ignoreEvent(true);
		}
	};

	void onControl(ScriptingApi::Content::ScriptComponent *, var value) override
	{
		channel = value;
	};

private:

	ScriptingApi::Content::ScriptSlider *channelNumber;
	
	int channel;

};

class ChannelSetterScriptProcessor : public HardcodedScriptProcessor
{
public:

	SET_PROCESSOR_NAME("ChannelSetter", "MIDI Channel Setter");

	ChannelSetterScriptProcessor(MainController *mc, const String &id, ModulatorSynth *ms) :
		HardcodedScriptProcessor(mc, id, ms)
	{
		onInit();
	};

	void onInit() override
	{
		Content.setHeight(50);

		channelNumber = Content.addKnob("channelNumber", 0, 0);
		channelNumber->set("text", "MIDI Channel");
		channelNumber->setRange(1, 16, 1);

		channel = 1;
	}

	void onNoteOn() override
	{
		Message.setChannel(channel);
	};

	void onNoteOff() override
	{
		Message.setChannel(channel);
	};

	void onController() override
	{
		Message.setChannel(channel);
	};

	void onControl(ScriptingApi::Content::ScriptComponent *, var value) override
	{
		channel = value;
	};

private:

	ScriptingApi::Content::ScriptSlider *channelNumber;

	int channel;

};

class MuteAllScriptProcessor : public HardcodedScriptProcessor
{
public:

	SET_PROCESSOR_NAME("MidiMuter", "MidiMuter");

	MuteAllScriptProcessor(MainController *mc, const String &id, ModulatorSynth *ms) :
		HardcodedScriptProcessor(mc, id, ms)
	{
		onInit();
	};

	void onInit() override
	{
		ignoreButton = Content.addButton("ignoreButton", 10, 0);
		ignoreButton->set("text", "Ignore all events");

		fixStuckNotes = Content.addButton("fixStuckNotes", 140, 0);
		fixStuckNotes->set("text", "Fix stuck notes");

		ignore = false;
		fix = false;

		noteOns = 0;

	}

	void onNoteOn() override
	{
		if (ignore)
		{
			Message.ignoreEvent(true);
		}

		noteOns.setBit(Message.getNoteNumber(), true);
	};

	void onNoteOff() override
	{
		if (ignore)
		{
			if (!fix || !noteOns[Message.getNoteNumber()])
			{
				Message.ignoreEvent(true);
			}
		}

		noteOns.setBit(Message.getNoteNumber(), false);
	}

	void onControl(ScriptingApi::Content::ScriptComponent *number, var value) override
	{
		if (number == ignoreButton)
		{
			ignore = (double)value > 0.5;
		}
		else if (number == fixStuckNotes)
		{
			fix = (double)value > 0.5;
		}
	}

private:

	ScriptingApi::Content::ScriptButton *ignoreButton;
	ScriptingApi::Content::ScriptButton *fixStuckNotes;

	bool ignore = false;
	bool fix = false;

	BigInteger noteOns;
};


class Processor;

class HardcodedScriptFactoryType: public FactoryType
{
	// private enum for handling
	enum
	{
        legatoWithRetrigger = MidiProcessorFactoryType::numMidiProcessors,
		ccSwapper,
		releaseTrigger,
		cc2Note,
		channelFilter,
		channelSetter,
		muteAll,
		arpeggiator,
	};

public:

	HardcodedScriptFactoryType(Processor *p):
		FactoryType(p)
	{
		fillTypeNameList();
	};

	void fillTypeNameList();

	~HardcodedScriptFactoryType()
	{
		typeNames.clear();
	};

	Processor *createProcessor(int typeIndex, const String &id) override;
	
	const Array<ProcessorEntry> & getTypeNames() const override
	{
		return typeNames;
	};
	
private:

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HardcodedScriptFactoryType)

	Array<ProcessorEntry> typeNames;

};


} // namespace hise
#endif  // HARDCODEDSCRIPTPROCESSOR_H_INCLUDED
