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
class HardcodedScriptProcessor: public ScriptBaseMidiProcessor,
							    public ProcessorWithDynamicExternalData
{
public:

	using ScriptSliderPack = WeakReference<ScriptingApi::Content::ScriptSliderPack>;
	using ScriptButton = WeakReference<ScriptingApi::Content::ScriptButton>;
	using ScriptSlider = WeakReference<ScriptingApi::Content::ScriptSlider>;
	using ScriptComboBox = WeakReference<ScriptingApi::Content::ScriptComboBox>;

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

	virtual void onAllNotesOff() {};

    void restoreFromValueTree(const ValueTree &v) override;

protected:

	void controlCallback(ScriptingApi::Content::ScriptComponent *component, var controllerValue) override;


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
	@ingroup midiTypes
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

	SET_PROCESSOR_NAME("LegatoWithRetrigger", "Legato with Retrigger", "Enables monophonic mode and retriggers the pressed key if there was a key release. ");

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
		possibleRetriggerChannel = -1;
		lastVelo = 0;
	}

	void onNoteOn() override
	{
		int newID = Message.makeArtificial();

		if(lastNote != -1)
		{
			Synth.noteOffByEventId(lastEventId);
		
			possibleRetriggerNote = lastNote;
			possibleRetriggerChannel = lastChannel;
		}
	
		
		lastEventId = newID;

		lastNote = Message.getNoteNumber();
		lastVelo = Message.getVelocity();
		lastChannel = Message.getChannel();
	};

	void onNoteOff() override
	{
		if (Message.getNoteNumber() == lastNote && Message.getChannel() == lastChannel)
		{
			Message.ignoreEvent(true);
			Synth.noteOffByEventId(lastEventId);
		}

		int number = Message.getNoteNumber();
	
		int channel = Message.getChannel();

		if(number == possibleRetriggerNote && channel == possibleRetriggerChannel)
		{
			possibleRetriggerNote = -1;
			possibleRetriggerChannel = -1;
		}
	
		if(number == lastNote)
		{
			if(possibleRetriggerNote != -1 && possibleRetriggerChannel != -1)
			{
				lastEventId = Synth.addNoteOn(possibleRetriggerChannel, possibleRetriggerNote, lastVelo, 0);

				lastNote = possibleRetriggerNote;
				lastChannel = possibleRetriggerChannel;
				possibleRetriggerNote = -1;
				possibleRetriggerChannel = -1;
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
	int lastChannel;
	int possibleRetriggerChannel;
	int possibleRetriggerNote;
	int lastVelo;

};

/** Swaps two control change numbers. 
	@ingroup midiTypes 
	*/
class CCSwapper: public HardcodedScriptProcessor
{
public:

	SET_PROCESSOR_NAME("CCSwapper", "CC Swapper", "Swaps two control change numbers.");

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

	JUCE_DECLARE_WEAK_REFERENCEABLE(CCSwapper);
};

/** Allows release trigger functionality with a time variant decrease of the velocity. 
	@ingroup midiTypes 
*/
class ReleaseTriggerScriptProcessor: public HardcodedScriptProcessor,
									 public MidiControllerAutomationHandler::MPEData::Listener
{
public:

	SET_PROCESSOR_NAME("ReleaseTrigger", "Release Trigger", "Allows release trigger functionality with a time variant decrease of the velocity. ");

	ReleaseTriggerScriptProcessor(MainController *mc, const String &id, ModulatorSynth *ms):
		HardcodedScriptProcessor(mc, id, ms)
	{
		onInit();

		getMainController()->getMacroManager().getMidiControlAutomationHandler()->getMPEData().addListener(this);
	};

	~ReleaseTriggerScriptProcessor()
	{
		getMainController()->getMacroManager().getMidiControlAutomationHandler()->getMPEData().removeListener(this);
	}

	void mpeModeChanged(bool isEnabled) override { mpeEnabled = isEnabled; };

	void mpeDataReloaded() override {};

	void mpeModulatorAssigned(MPEModulator* /*m*/, bool /*wasAssigned*/) override {};

	void onInit() override
	{
		currentMessageHolder = Engine.createMessageHolder();
		currentMessageVar = var(currentMessageHolder.get());

		messageHolders.ensureStorageAllocated(128);

		for (int i = 0; i < 128; i++)
		{
			messageHolders.add(Engine.createMessageHolder());
		}

		Content.setHeight(100);
		Content.setColour(140, 120, 200);
		Content.setName("Release Trigger");

		enableButton = Content.addButton("TimeAttenuate", 0, 0);
		timeKnob = Content.addKnob("Time", 0, 50);
		timeKnob->setRange(0, 20, 0.1);
		table = Content.addTable("TimeTable", 140, 0);
		table->registerAtParent(0);

		table->enablePooledUpdate(mc->getGlobalUIUpdater());
		table->setPosition(140, 0, 480, 100);

		attenuationLevel = 1.0; 
		timeIndex = 0;

		for(int i = 0; i < 128; i++)
		{
			lengthValues[i] = 0.0;
		}
	}

	void onNoteOn() override
	{
		Message.ignoreEvent(true);

		const int number = jlimit<int>(0, 127, Message.getNoteNumber());

		messageHolders[number]->setMessage(*getCurrentHiseEvent());

		lengthValues[number] = Engine.getUptime();
	};

	void onNoteOff() override
	{
		Message.ignoreEvent(true);
	
		const int noteNumber = Message.getNoteNumber();

		if((int)enableButton->getValue() == 1)
		{
			double time = Engine.getUptime() - lengthValues[noteNumber];
		
			timeIndex = (double)(time / (double)timeKnob->getValue());
			timeIndex = jlimit<double>(0, 1, timeIndex);
			
			attenuationLevel = table->getTableValue(timeIndex);
			
		}
		else
		{
			attenuationLevel = 1.0f;
		}
	
		auto onEvent = messageHolders[noteNumber]->getMessageCopy();

		auto velocityToUse = mpeEnabled ? Message.getVelocity() : onEvent.getVelocity();

		const int v = (int)((float)velocityToUse * attenuationLevel);

		if (v > 0)
		{
			onEvent.setVelocity((uint8)v);
			onEvent.ignoreEvent(false);
			onEvent.setTimeStamp((int)Message.getTimestamp());
			

			currentMessageHolder->setMessage(onEvent);

			Synth.addMessageFromHolder(currentMessageVar);
		}
			
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

	bool mpeEnabled = false;

	ReferenceCountedArray<ScriptingObjects::ScriptingMessageHolder> messageHolders;

	ReferenceCountedObjectPtr<ScriptingObjects::ScriptingMessageHolder> currentMessageHolder;

	var currentMessageVar;

	ScriptingApi::Content::ScriptButton *enableButton;
	ScriptingApi::Content::ScriptSlider *timeKnob;
	ScriptingApi::Content::ScriptTable *table;
	float attenuationLevel;
	double timeIndex;
	double lengthValues[128];

	JUCE_DECLARE_WEAK_REFERENCEABLE(ReleaseTriggerScriptProcessor);
};


class CCToNoteProcessor : public HardcodedScriptProcessor
{
public:

	SET_PROCESSOR_NAME("CC2Note", "MIDI CC to Note Generator", "deprecated");

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

/** This MIDI processor simply filters messages that do not fit the given channel. 
	@ingroup midiTypes
*/
class ChannelFilterScriptProcessor : public HardcodedScriptProcessor,
	public MidiControllerAutomationHandler::MPEData::Listener
{
public:

	SET_PROCESSOR_NAME("ChannelFilter", "MIDI Channel Filter", "Filters messages that do not fit the given channel.");

	ChannelFilterScriptProcessor(MainController *mc, const String &id, ModulatorSynth *ms) :
		HardcodedScriptProcessor(mc, id, ms)
	{
		onInit();

		mc->getMacroManager().getMidiControlAutomationHandler()->getMPEData().addListener(this);
	};

	~ChannelFilterScriptProcessor()
	{
		getMainController()->getMacroManager().getMidiControlAutomationHandler()->getMPEData().removeListener(this);
	}

	void onInit() override
	{
		Content.setHeight(50);

		channelNumber = Content.addKnob("channelNumber", 0, 0);
		channelNumber->set("text", "MIDI Channel");
		channelNumber->setRange(1, 16, 1);

		mpeStartChannel = Content.addKnob("mpeStart", 150, 0);
		mpeStartChannel->set("width", 170);
		mpeStartChannel->set("text", "MPE Start Channel");
		mpeStartChannel->setRange(2, 16, 1);

		mpeEndChannel = Content.addKnob("mpeEnd", 150 + 190, 0);
		mpeEndChannel->set("width", 170);
		mpeEndChannel->set("text", "MPE End Channel");
		mpeEndChannel->setRange(2, 16, 1);
		mpeEndChannel->setValue(16);

		channel = 1;
		mpeRange = 0;

		mpeRange.setRange(1, 15, true);
	}

	void mpeModeChanged(bool isEnabled) override
	{
		mpeEnabled = isEnabled;
	}

	void mpeDataReloaded() override {};

	void mpeModulatorAmountChanged() override {};

	void mpeModulatorAssigned(MPEModulator* /*m*/, bool /*wasAssigned*/) override {};

	void onNoteOn() override
	{
		if (mpeEnabled)
		{
			if (!mpeRange[Message.getChannel()-1])
				Message.ignoreEvent(true);
		}
		else
		{
			if (Message.getChannel() != channel)
			{
				Message.ignoreEvent(true);
			}
		}
	};

	void onNoteOff() override
	{
		if (mpeEnabled)
		{
			if (!mpeRange[Message.getChannel()-1])
				Message.ignoreEvent(true);
		}
		else
		{
			if (Message.getChannel() != channel)
			{
				Message.ignoreEvent(true);
			}
		}
		
	};

	void onController() override
	{
		if (mpeEnabled)
		{
			if (!mpeRange[Message.getChannel()-1])
				Message.ignoreEvent(true);
		}
		else
		{
			if (Message.getChannel() != channel)
			{
				Message.ignoreEvent(true);
			}
		}
		
	};

	void onControl(ScriptingApi::Content::ScriptComponent *c, var value) override
	{
		if (c == channelNumber)
		{
			channel = value;
		}
		else
		{
			auto startValue = (int)mpeStartChannel->getValue()-1;
			auto endValue = (int)mpeEndChannel->getValue()-1;

			mpeRange.clear();
			mpeRange.setRange(startValue, (endValue - startValue) + 1, true);
			
			// Always allow stuff on the master channel
			mpeRange.setBit(0, true);
		}
	};

private:

	ScriptingApi::Content::ScriptSlider *channelNumber;
	ScriptingApi::Content::ScriptSlider *mpeStartChannel;
	ScriptingApi::Content::ScriptSlider *mpeEndChannel;

	bool mpeEnabled = false;

	int channel;
	BigInteger mpeRange;

	JUCE_DECLARE_WEAK_REFERENCEABLE(ChannelFilterScriptProcessor);
};

/** Changes the MIDI channel of every incoming message.
	@ingroup midiTypes.
	
	Note that you have 256 MIDI channels in HISE, so you can use it for advanced routing
	of messages.
*/
class ChannelSetterScriptProcessor : public HardcodedScriptProcessor
{
public:

	SET_PROCESSOR_NAME("ChannelSetter", "MIDI Channel Setter", "Changes the MIDI channel of every incoming message.");

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

	JUCE_DECLARE_WEAK_REFERENCEABLE(ChannelSetterScriptProcessor);
};

/** Mutes the incoming note-on messages, but leaves everything else through.
	@ingroup midiTypes
	
	This is a more sophisticated version of just ignoring everything, because
	this might lead to stuck notes pretty easily.
*/
class MuteAllScriptProcessor : public HardcodedScriptProcessor
{
public:

	SET_PROCESSOR_NAME("MidiMuter", "MidiMuter", "Mutes the incoming note-on messages, but leaves everything else through.");

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

	JUCE_DECLARE_WEAK_REFERENCEABLE(MuteAllScriptProcessor);
};


class Processor;




} // namespace hise
#endif  // HARDCODEDSCRIPTPROCESSOR_H_INCLUDED
