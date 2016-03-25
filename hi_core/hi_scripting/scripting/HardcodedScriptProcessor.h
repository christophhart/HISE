#ifndef HARDCODEDSCRIPTPROCESSOR_H_INCLUDED
#define HARDCODEDSCRIPTPROCESSOR_H_INCLUDED

#include "JuceHeader.h"

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
class HardcodedScriptProcessor: public ScriptBaseProcessor
{
public:

	HardcodedScriptProcessor(MainController *mc, const String &id, ModulatorSynth *ms);
	virtual ~HardcodedScriptProcessor() {};

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
        
        ScriptBaseProcessor::restoreFromValueTree(v);
        
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

	void synthTimerCallback (int offsetInBuffer) override
	{
		onTimer(offsetInBuffer);
	};


	ScriptingApi::Message Message;
	ScriptingApi::Synth Synth;
	ScriptingApi::Console Console;
	ScriptingApi::Engine Engine;
	ScriptingApi::Content Content;

	void processMidiMessage (MidiMessage &m) override;

	ProcessorEditorBody *createEditor(BetterProcessorEditor *parentEditor)  override;
	
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
		possibleRetriggerNote = -1;		
		lastVelo = 0;
	}

	void onNoteOn() override
	{
		if(lastNote != -1)
		{
		
			Synth.noteOff(lastNote);
		
			possibleRetriggerNote = lastNote;
		}
	
		lastNote = Message.getNoteNumber();
	
		lastVelo = Message.getVelocity();
	};

	void onNoteOff() override
	{
		int number = Message.getNoteNumber();
	
		if(number == possibleRetriggerNote)
		{
			possibleRetriggerNote = -1;
		}
	
		if(number == lastNote)
		{
			if(possibleRetriggerNote != -1)
			{
				Synth.playNote(possibleRetriggerNote, lastVelo);
			
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


class LegatoIntervallPlayer: public HardcodedScriptProcessor
{
public:

	SET_PROCESSOR_NAME("LegatoIntervalPlayer", "Legato Interval Player");

	LegatoIntervallPlayer(MainController *mc, const String &id, ModulatorSynth *ms):
		HardcodedScriptProcessor(mc, id, ms)
	{
		onInit();	
	};

	void onInit() override
	{
		lastNote = -1;
		lastPlayedNote = 0;
	}

	void onNoteOn() override
	{
		Message.ignoreEvent(true);
	
		if(Synth.isLegatoInterval())
		{
			lastPlayedNote = lastNote;
		
			Synth.playNote(lastNote, Message.getVelocity());
		}
	
		lastNote = Message.getNoteNumber();
	};

	void onNoteOff() override
	{
		Message.ignoreEvent(true);
	
		if(Message.getNoteNumber() == lastNote)
		{
			Synth.noteOff(lastPlayedNote);
		}
	};

private:

	int lastNote, lastPlayedNote;

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
	
		Synth.playNote(Message.getNoteNumber(), (int)(velocityValues[Message.getNoteNumber()] * attenuationLevel));
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

		Content.setHeight(120);

		controllerNumberSlider = Content.addKnob("controllerNumberSlider", 10, 10);
		controllerNumberSlider->set("text", "CC Number");
		controllerNumberSlider->set("min", 0);
		controllerNumberSlider->set("max", 127);
		controllerNumberSlider->set("stepSize", 1);
		controllerNumberSlider->set("middlePosition", 64);


		noteOnOrOff = Content.addButton("noteOnOrOff", 10, 70);
		noteOnOrOff->set("text", "Generate Note On");
		veloTable = Content.addTable("veloTable", 150, 17);
		veloTable->set("width", 400);
		veloTable->set("height", 80);
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
		if (lastNote != -1 && (int)Message.getControllerNumber() == controllerNumber)
		{
			if (noteOn)
			{
				const float velo = veloTable->getTableValue(Message.getControllerValue()) * 127.0f;

				Synth.addNoteOn(1, lastNote, (int)velo, 0);
			}
			else
			{
				Synth.addNoteOff(1, lastNote, 0);
			}
		}
	};

	void onControl(ScriptingApi::Content::ScriptComponent * controller, var value) override
	{
		if (controller == noteOnOrOff)
		{
			if ((float)value > 0.5f)
			{
				noteOnOrOff->set("text", "Generate Note On");
				noteOn = true;
			}
			else
			{
				noteOnOrOff->set("text", "Generate Note Off");
				noteOn = false;
			}
		}
		else if (controller == controllerNumberSlider)
		{
			controllerNumber = value;
		}
	};

private:

	ScriptingApi::Content::ScriptButton * noteOnOrOff;
	ScriptingApi::Content::ScriptSlider *controllerNumberSlider;
	ScriptingApi::Content::ScriptTable *veloTable;

	int lastNote;

	int controllerNumber;
	bool noteOn;
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


class LiveCodingNotePlayer : public HardcodedScriptProcessor
{
public:
    
    SET_PROCESSOR_NAME("Live Coding Note Player", "LiveCodingNotePlayer");
    
	enum PlayMode
	{
		Monophonic = 1,
		Polyphonic,
		Percussion,
		Loop,
		numPlayModes
	};

	LiveCodingNotePlayer(MainController *mc, const String &id, ModulatorSynth *ms) :
		HardcodedScriptProcessor(mc, id, ms),
		noteName("note"),
		velocityName("velo"),
		chordSpeed("chordSpeed"),
		transpose("transpose"),
		shuffleSamples(0)
    {
        onInit();
    };
    
    ValueTree exportAsValueTree() const override
    {
        ValueTree v = ScriptBaseProcessor::exportAsValueTree();
        
        v.setProperty("VelocityName", velocityName.toString(), nullptr);
        v.setProperty("NoteName", noteName.toString(), nullptr);
		v.setProperty("CurrentPlayMode", (int)currentPlayMode, nullptr);
        
        return v;
    }
    
    void restoreFromValueTree(const ValueTree &v) override
    {
        ScriptBaseProcessor::restoreFromValueTree(v);
        
		String n = v.getProperty("NoteName").toString();

		if (n.isNotEmpty())
		{
			noteName = Identifier(n);
		}

		String ve = (v.getProperty("VelocityName").toString());
		if (ve.isNotEmpty())
		{
			velocityName = Identifier(ve);
		}
        
        
        
        velocityLabel->set("text", velocityName.toString());
        noteLabel->set("text", noteName.toString());
		currentPlayMode = (PlayMode)(int)v.getProperty("CurrentPlayMode");
        
    }
    
    void onInit() override
    {
		Content.setHeight(100);

		modeSelector = Content.addComboBox("Mode", 325, 51);
		modeSelector->addItem("Monophonic");
		modeSelector->addItem("Polyphonic");
		modeSelector->addItem("Percussion");
		modeSelector->addItem("Loop");


		baseOctaveSlider = Content.addKnob("baseOctaveSlider", 329, 0);
		baseOctaveSlider->set("text", "Octave");
		baseOctaveSlider->setRange(0, 8, 1);
		baseOctaveSlider->setMidPoint(4);

		shuffleSlider = Content.addKnob("shuffleSlider", 492, 0);
		shuffleSlider->set("text", "Shuffle");
		shuffleSlider->set("mode", "NormalizedPercentage");
		
        noteLabel = Content.addLabel("NoteArrayName", 14, 40);
		noteLabel->set("width", 128);
		noteLabel->set("height", 24);
		noteLabel->set("bgColour", 1409286144);
		noteLabel->set("itemColour", 1744830463);
#if JUCE_WINDOWS
		noteLabel->set("fontName", "Consolas");
#else 
		noteLabel->set("fontName", "Monaco");
#endif

        velocityLabel = Content.addLabel("VelocityArrayName",163, 40);
		velocityLabel->set("width", 128);
		velocityLabel->set("height", 24);
		velocityLabel->set("bgColour", 1409286144);
		velocityLabel->set("itemColour", 1744830463);
#if JUCE_WINDOWS
		velocityLabel->set("fontName", "Consolas");
#else 
		velocityLabel->set("fontName", "Monaco");
#endif

		reloadButton = Content.addButton("Reload", 492, 51);
		reloadButton->set("text", "Reload");


		noteNameLabel = Content.addLabel("noteNameLabel", 10, 24);
		noteNameLabel->set("text", "Note Array ID:");
		noteNameLabel->set("editabl", false);

		velocityNameLabel = Content.addLabel("velocityNameLabel", 159, 24);
		velocityNameLabel->set("text", "Velocity Array ID:");
		velocityNameLabel->set("editable", false);


		noteLabel->set("text", noteName.toString());
		velocityLabel->set("text", velocityName.toString());

		
		parameterNames.add("Mode");
		parameterNames.add("BaseOctave");
		parameterNames.add("Shuffle");

		shuffleSamples = 0;
    }
    
    void onNoteOn() override
    {
		Message.ignoreEvent(true);

		var noteData = getMainController()->getGlobalVariableObject()->getProperty(noteName);

		switch (currentPlayMode)
		{
		case LiveCodingNotePlayer::Monophonic:
		{
			if (noteData.isArray() && noteData.size() != 0)
			{
				const int noteIndex = Message.getNoteNumber() % noteData.size();
				const var noteValue = noteData.getArray()->getUnchecked(noteIndex);

				if (noteValue.isUndefined()) return;

				int veloValue = 127;

				var velocityData = getMainController()->getGlobalVariableObject()->getProperty(velocityName);

				if (velocityData.isArray() && velocityData.size() != 0)
				{
					const int velocityIndex = Message.getNoteNumber() % velocityData.size();
					veloValue = jlimit<int>(0, 127, velocityData.getArray()->getUnchecked(velocityIndex));
				}

				const int transposeValue = getMainController()->getGlobalVariableObject()->getProperty(transpose);

				const int noteNumber = jlimit<int>(0, 127, (int)noteValue + (int)baseOctaveSlider->getValue()*12 + transposeValue);

				if (noteNumber != 0 && veloValue != 0)
				{
					Synth.addNoteOn(1, noteNumber, veloValue, (Message.getNoteNumber()%2 == 0) ? 0 : shuffleSamples );
				}
			}
			break;
		}
			
		case LiveCodingNotePlayer::Polyphonic:
		{
			if (noteData.isArray() && noteData.size() != 0)
			{
				const int chordSpeed = getMainController()->getGlobalVariableObject()->getProperty("chordSpeed"); // Add another field (don't hardcode ?)

				if (Message.getNoteNumber() % chordSpeed == 0)
				{
					const int transposeValue = getMainController()->getGlobalVariableObject()->getProperty(transpose);
					const int base = (int)baseOctaveSlider->getValue()*12;

					const int length = noteData.size();
					const int index = (Message.getNoteNumber() / chordSpeed) % length;
					
					if (noteData.getArray()->getUnchecked(index).isArray())
					{
						var chordData = noteData.getArray()->getUnchecked(index);

						if (chordData.isArray() && chordData.size() != 0)
						{
							for (int i = 0; i < chordData.size(); i++)
							{
								const int noteNumberValue = (int)chordData.getArray()->getUnchecked(i) + base + transposeValue;

								Synth.playNote(jlimit<int>(0,127,noteNumberValue), 127);
							}
						}
					}
				}
			}

			
		}
			break;
		case LiveCodingNotePlayer::Percussion:
		{
			var velocityData = getMainController()->getGlobalVariableObject()->getProperty(velocityName);

			if (velocityData.isArray() && velocityData.size() != 0)
			{
				const int index = Message.getNoteNumber() % velocityData.size();
				const int velocityValue = jlimit<int>(0, 127, velocityData.getArray()->getUnchecked(index));

				if (velocityValue != 0)
				{
					Synth.playNote(36 + (int)baseOctaveSlider->getValue()*12, velocityValue);
				}
			}
		}
			break;
		case LiveCodingNotePlayer::Loop:
		{
			bool playLoop = getMainController()->getGlobalVariableObject()->getProperty(noteName);

			if (playLoop && ((Message.getNoteNumber() % 8) == 0))
			{
				Synth.playNote(36, 127);
			}
		}
			break;
		case LiveCodingNotePlayer::numPlayModes:
			break;
		default:
			break;
		}
    };

	void onNoteOff()
	{
		Message.ignoreEvent(true);
	}
    
    void onControl(ScriptingApi::Content::ScriptComponent *number, var value) override
    {
        if(number == noteLabel)
        {
			noteName = value.toString();
        }
		else if (number == velocityLabel)
        {   
			velocityName = value.toString();
        }
		else if (number == reloadButton)
		{
			int noteArrayFound = getMainController()->getGlobalVariableObject()->getProperty(noteName).size();
			int velocityArrayFound = getMainController()->getGlobalVariableObject()->getProperty(velocityName).size();

			if (noteArrayFound)
			{
				debugToConsole(this, "Note Array found. Length: " + String(noteArrayFound));
				
			}
			if (velocityArrayFound)
			{
				debugToConsole(this, "Velocity Array found. Length: " + String(velocityArrayFound));
			}

			noteLabel->set("bgColour", noteArrayFound ? 0x2200FF00 : 0x22FF0000);
			velocityLabel->set("bgColour", velocityArrayFound ? 0x2200FF00 : 0x22FF0000);

		}
		else if (number == modeSelector)
		{
			if (value == "Monophonic")
			{
				currentPlayMode = Monophonic;
				noteLabel->set("enabled", true);
				velocityLabel->set("enabled", true);
			}
			else if (value == "Polyphonic")
			{
				currentPlayMode = Polyphonic;
				noteLabel->set("enabled", true);
				velocityLabel->set("enabled", true);
			}
			else if (value == "Percussion")
			{
				currentPlayMode = Percussion;
				noteLabel->set("enabled", false);
				velocityLabel->set("enabled", true);
			}
			else if (value == "Loop")
			{
				currentPlayMode = Loop;
				noteLabel->set("enabled", true);
				velocityLabel->set("enabled", false);
			}
		}   
		else if (number == shuffleSlider)
		{
			const double tempo = Engine.getHostBpm();
			const double tempoSamples = Engine.getSamplesForMilliSeconds((30.0 / tempo) * 1000.0);
			shuffleSamples = (int)((double)value * tempoSamples * 0.99);
		}
	};

private:
    
	PlayMode currentPlayMode;

    Identifier noteName;
    Identifier velocityName;
	Identifier chordSpeed;
	Identifier transpose;
    
	ScriptingApi::Content::ScriptLabel *noteNameLabel;
	ScriptingApi::Content::ScriptLabel *velocityNameLabel;

    ScriptingApi::Content::ScriptLabel *noteLabel;
    ScriptingApi::Content::ScriptLabel *velocityLabel;
	ScriptingApi::Content::ScriptButton *reloadButton;
	ScriptingApi::Content::ScriptComboBox *modeSelector;
	ScriptingApi::Content::ScriptSlider *baseOctaveSlider;
	ScriptingApi::Content::ScriptSlider *shuffleSlider;

	int shuffleSamples;
};

class Processor;

class HardcodedScriptFactoryType: public FactoryType
{
	// private enum for handling
	enum
	{
		legatoWithRetrigger = MidiProcessorFactoryType::midiProcessorChain,
		ccSwapper,
		legatoIntervalPlayer,
		releaseTrigger,
		cc2Note,
		channelFilter,
		channelSetter,
		muteAll,
        liveCodingNotePlayer
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



#endif  // HARDCODEDSCRIPTPROCESSOR_H_INCLUDED
