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
*   Commercial licences for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licencing:
*
*   http://www.hartinstruments.net/hise/
*
*   HISE is based on the JUCE library,
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

#if STANDALONE_CONVOLUTION
#else
#include <regex>
#endif


#define SEND_MESSAGE(broadcaster) {																												\
								      if (MessageManager::getInstance()->isThisTheMessageThread()) broadcaster->sendSynchronousChangeMessage(); \
									  									  else broadcaster->sendChangeMessage();																	\
								  								  }

#define ADD_TO_TYPE_SELECTOR(x) (ScriptComponentPropertyTypeSelector::addToTypeSelector(ScriptComponentPropertyTypeSelector::x, propertyIds.getLast()))
#define ADD_AS_SLIDER_TYPE(min, max, interval) (ScriptComponentPropertyTypeSelector::addToTypeSelector(ScriptComponentPropertyTypeSelector::SliderSelector, propertyIds.getLast(), min, max, interval))

ScriptingObject::ScriptingObject(ScriptBaseProcessor *p):
	processor(dynamic_cast<Processor*>(p))
	{};	

ScriptBaseProcessor *ScriptingObject::getScriptProcessor() 
{
	return dynamic_cast<ScriptBaseProcessor*>(processor.get()); 
};

const ScriptBaseProcessor *ScriptingObject::getScriptProcessor() const 
{ 
	return dynamic_cast<ScriptBaseProcessor*>(processor.get());
};


bool ScriptingObject::checkArguments(const String &callName, int numArguments, int expectedArgumentAmount)
{
	if(numArguments < expectedArgumentAmount) 
	{
		String x;
		x << "Call to " << callName << " - Too few arguments: " << String(numArguments) << ", (Expected: " << String(expectedArgumentAmount) << ")";

		reportScriptError(x); 
		return false;
	}
		
	return true;
}

int ScriptingObject::checkValidArguments(const var::NativeFunctionArgs &args)
{
	for(int i = 0; i < args.numArguments; i++) 
	{
		if(args.arguments[i].isUndefined()) 
		{
			reportScriptError("Argument " + String(i) + " is undefined!");
			return i;
		} 
	}

	return -1;

};



#pragma warning( push )
#pragma warning( disable : 4100)

void ScriptingObject::reportScriptError(const String &errorMessage) const
{ 
	
#if USE_BACKEND // needs to be customized because of the colour!
	const_cast<MainController*>(getScriptProcessor()->getMainController())->writeToConsole(errorMessage, 1, getScriptProcessor(), getScriptProcessor()->getScriptingContent()->getColour()); 
#endif
}

#pragma warning( pop ) 

bool ScriptingObject::checkIfSynchronous(const Identifier &methodName) const
{ 
	const ScriptProcessor *sp = dynamic_cast<const ScriptProcessor*>(getScriptProcessor());

	if(sp == nullptr) return true; // HardcodedScriptProcessors are always synchronous

	if(sp->isDeferred())
	{
		reportScriptError("Illegal call of " + methodName.toString() + " (Can only be called in synchronous mode)");
	}

	return !sp->isDeferred();
}

void ScriptingObject::reportIllegalCall(const String &callName, const String &allowedCallback) const
{
	String x;
	x << "Call of " << callName << " outside of " << allowedCallback << " callback";

	reportScriptError(x);
};


// ====================================================================================================== Message functions

int ScriptingApi::Message::getNoteNumber() const
{
	if(messageHolder == nullptr || !messageHolder->isNoteOnOrOff())
	{
		reportIllegalCall("getNoteNumber()", "onNoteOn / onNoteOff");
		return -1;
	}

	return messageHolder->getNoteNumber();
};


void ScriptingApi::Message::delayEvent(int samplesToDelay)
{
	if(messageHolder == nullptr)
	{
		reportIllegalCall("delayEvent()", "midi event");
		return;
	}	

	messageHolder->addToTimeStamp(samplesToDelay);
};

void ScriptingApi::Message::setNoteNumber(int newValue)
{
	if(messageHolder == nullptr)
	{
		reportIllegalCall("setNoteNumber()", "midi event");
		return;
	}

	if(!messageHolder->isNoteOnOrOff())
	{
		reportIllegalCall("setNoteNumber()", "noteOn / noteOff");
	}

	messageHolder->setNoteNumber(newValue);
};

void ScriptingApi::Message::setVelocity(int newValue)
{
	if(messageHolder == nullptr)
	{
		reportIllegalCall("setVelocity()", "midi event");
		return;
	}

	if(!messageHolder->isNoteOn())
	{
		reportIllegalCall("setVelocity()", "onNoteOn");
	}

	messageHolder->setVelocity((float)newValue / 127.0f);
};




void ScriptingApi::Message::setControllerNumber(int newValue)
{
	if(messageHolder == nullptr)
	{
		reportIllegalCall("setControllerNumber()", "midi event");
		return;
	}

	if(!messageHolder->isController())
	{
		reportIllegalCall("setControllerNumber()", "onController");
	}

	const int value = messageHolder->getControllerValue();

	*messageHolder = juce::MidiMessage::controllerEvent(1, newValue, value);
};

void ScriptingApi::Message::setControllerValue(int newValue)
{
	if(messageHolder == nullptr)
	{
		reportIllegalCall("setVelocity()", "midi event");
		return;
	}

	if(!messageHolder->isController())
	{
		reportIllegalCall("setControllerValue()", "onController");
	}

	const int number = messageHolder->getControllerNumber();

	*messageHolder = juce::MidiMessage::controllerEvent(1, number, newValue);
};

var ScriptingApi::Message::getControllerNumber() const
{
	if(messageHolder == nullptr || ( !messageHolder->isController() && !messageHolder->isPitchWheel() && !messageHolder->isAftertouch() ))
	{
		reportIllegalCall("getControllerNumber()", "onController");
		return var::undefined();
	}

	if(messageHolder->isController())		  return messageHolder->getControllerNumber();
	else if (messageHolder->isPitchWheel())	  return 128;
	else if (messageHolder->isAftertouch())   return 129;
	else									  return var::undefined();
};


var ScriptingApi::Message::getControllerValue() const
{
	if(messageHolder == nullptr || ( !messageHolder->isController() && !messageHolder->isPitchWheel() && !messageHolder->isAftertouch() ))
	{
		reportIllegalCall("getControllerValue()", "onController");
		return var::undefined();
	}

	if      (messageHolder->isController())	  return messageHolder->getControllerValue();
	else if (messageHolder->isAftertouch())   return messageHolder->getAfterTouchValue();
	else if (messageHolder->isPitchWheel())	  return messageHolder->getPitchWheelValue();
	else									  return var::undefined();
};

int ScriptingApi::Message::getVelocity() const
{
	if(messageHolder == nullptr || (!messageHolder->isNoteOn()))
	{
		reportIllegalCall("getVelocity()", "onNoteOn");
		return -1;
	}

	return messageHolder->getVelocity();
};

int ScriptingApi::Message::getChannel() const
{
	if(messageHolder == nullptr)
	{
		reportScriptError("Can only be called in MIDI callbacks");
		return -1;
	}

	return messageHolder->getChannel();
};

void ScriptingApi::Message::setChannel(int newValue)
{
	if(messageHolder == nullptr)
	{
		reportIllegalCall("setChannel()", "midi event");
		return;
	}

	if(newValue < 1 || newValue > 16)
	{
		reportScriptError("Channel must be between 1 and 16.");
		return;
	}

	messageHolder->setChannel(newValue);
};

int ScriptingApi::Message::getEventId() const
{ 
	if(currentEventId == -1) reportScriptError("Defective Event ID detected!");

	return currentEventId; 
}

void ScriptingApi::Message::setMidiMessage(MidiMessage *m)
{
	messageHolder = m;

	if(m->isNoteOn())
	{
		eventIdCounter++;
		currentEventId = eventIdCounter;

		

		for(int i = 0; i < 1024; i++)
		{
			if(noteOnMessages[i].isVoid())
			{
				noteOnMessages[i] = MidiMessageWithEventId(*m, currentEventId);
				break;
			}
		}

		
	}
	else if(m->isNoteOff())
	{
		currentEventId = -1;

		for(int i = 0; i < 1024; i++)
		{
			MidiMessageWithEventId *mi = &noteOnMessages[i];

			if(!mi->isVoid() && m->getNoteNumber() == mi->getNoteNumber())
			{
				wrongNoteOff = false;
				currentEventId = mi->eventId;
						
				mi->setVoid();

				break;
			}
		}


        DBG("Wrong event ID");
    }
	else
	{
		currentEventId = -1;
	}
}

// ====================================================================================================== Engine functions


void ScriptingApi::Engine::allNotesOff()
{
	getScriptProcessor()->getMainController()->allNotesOff();
};



double ScriptingApi::Engine::getSampleRate() const
{
	return const_cast<MainController*>(getScriptProcessor()->getMainController())->getMainSynthChain()->getSampleRate();
}

void ScriptingApi::Engine::setGlobal(int index, var valueToSave)
{
	if (valueToSave.isMethod() || (valueToSave.isObject() && !valueToSave.isArray()))
	{
		reportScriptError("Methods and Objects can't be stored in the global container");
		return;
	}

	getScriptProcessor()->getMainController()->setGlobalVariable(index, valueToSave);
}

var ScriptingApi::Engine::getGlobal(int index) const
{
	return getScriptProcessor()->getMainController()->getGlobalVariable(index);
}

double ScriptingApi::Engine::getUptime() const
{
	return getScriptProcessor()->getMainController()->getUptime();
}

double ScriptingApi::Engine::getHostBpm() const
{
	return getScriptProcessor()->getMainController()->getBpm();
}

String ScriptingApi::Engine::getMacroName(int index)
{
	if (index >= 1 && index <= 8)
	{
		return getScriptProcessor()->getMainController()->getMainSynthChain()->getMacroControlData(index-1)->getMacroName();
	}
	else
	{
		reportScriptError("Illegal Macro Index");

		return "Undefined";
	}
}

String ScriptingApi::Engine::getOS()
{
#if JUCE_WINDOWS
	return "WIN";
#else
	return "OSX";
#endif
}

int ScriptingApi::Engine::getMidiNoteFromName(String midiNoteName) const
{
	for (int i = 0; i < 127; i++)
	{
		if (getMidiNoteName(i) == midiNoteName)
		{
			return i;
		}
	}
	return -1;
}




void ScriptingApi::Engine::openEditor(int includedFileIndex)
{
	ScriptProcessor *sp = dynamic_cast<ScriptProcessor*>(getScriptProcessor());

	if (sp != nullptr && includedFileIndex >= 0 && includedFileIndex < sp->getNumWatchedFiles())
	{
		dynamic_cast<ScriptProcessor*>(getScriptProcessor())->showPopupForFile(includedFileIndex);
	}
	else
	{
		reportScriptError("Illegal File Index");
	}
}

void ScriptingApi::Engine::setKeyColour(int keyNumber, int colourAsHex)
{
	getScriptProcessor()->getMainController()->setKeyboardCoulour(keyNumber, Colour(colourAsHex));
}

void ScriptingApi::Engine::setLowestKeyToDisplay(int keyNumber)
{
	getScriptProcessor()->getMainController()->setLowestKeyToDisplay(keyNumber);
}

void ScriptingApi::Engine::include(const String &string)
{
	jassertfalse;
}


void ScriptingApi::Engine::createLiveCodingVariables()
{
	ScriptProcessor *sp = dynamic_cast<ScriptProcessor*>(getScriptProcessor());

	if (sp != nullptr)
	{
		NamedValueSet *s = const_cast<NamedValueSet*>(&sp->getScriptEngine()->getRootObjectProperties());

		int C = 0;
		int Db = 1;
		int D = 2;
		int Eb = 3;
		int E = 4;
		int F = 5;
		int Gb = 6;
		int G = 7;
		int Ab = 8;
		int A = 9;
		int Bb = 10;
		int B = 11;

		s->set("C", C);
		s->set("Db", Db);
		s->set("D", D);
		s->set("Eb", Eb);
		s->set("E", E);
		s->set("F", F);
		s->set("Gb", Gb);
		s->set("G", G);
		s->set("A", A);
		s->set("Ab", Ab);
		s->set("Bb", Bb);
		s->set("B", B);

		var Cm[3] = { C, G, Eb + 12 };		
		var G7[3] = { B - 12, F, D + 12 };
		var Fm[3] = { Ab - 12, F, C + 12 };
		var Gm[3] = { Bb - 12, G, D + 12 };
		var Dmb5[4] = { D, F, Ab, C + 12 };
		var Bbmaj[3] = { Bb - 12, F, D + 12 };
		var Ebmaj[3] = { Eb, Bb, G + 12 };
		var Abmaj[3] = { Ab - 12, Eb, C + 12 };

		s->set("Cm", Array<var>(Cm, 3));
		s->set("G7", Array<var>(G7, 3));
		s->set("Fm", Array<var>(Fm, 3));
		s->set("Gm", Array<var>(Gm, 3));
		s->set("Abmaj", Array<var>(Abmaj, 3));
		s->set("Dmb5", Array<var>(Dmb5, 3));
		s->set("Bbmaj", Array<var>(Bbmaj, 3));
		s->set("Ebmaj", Array<var>(Ebmaj, 3));
		s->set("Abmaj", Array<var>(Abmaj, 3));
	}
}

DynamicObject * ScriptingApi::Engine::getPlayHead()
{
	return getScriptProcessor()->getMainController()->getHostInfoObject();
}

ScriptingObjects::MidiList *ScriptingApi::Engine::createMidiList()
{
    return new ScriptingObjects::MidiList(getScriptProcessor());
};


ScriptingObjects::TimerObject* ScriptingApi::Engine::createTimerObject()
{
	return new ScriptingObjects::TimerObject(getScriptProcessor());
}

void ScriptingApi::Engine::dumpAsJSON(var object, String fileName)
{
	if (!object.isObject())
	{
		reportScriptError("Only objects can be exported as JSON");
		return;
	}

	File f;

	if (File::isAbsolutePath(fileName))
	{
		f = File(fileName);
	}
	else
	{
		f = File(GET_PROJECT_HANDLER(getScriptProcessor()).getSubDirectory(ProjectHandler::SubDirectories::UserPresets).getChildFile(fileName));
	}

	f.replaceWithText(JSON::toString(object, false));
	
}

var ScriptingApi::Engine::loadFromJSON(String fileName)
{
	File f;

	if (File::isAbsolutePath(fileName))
	{
		f = File(fileName);
	}
	else
	{
		f = File(GET_PROJECT_HANDLER(getScriptProcessor()).getSubDirectory(ProjectHandler::SubDirectories::UserPresets).getChildFile(fileName));
	}

	if (f.existsAsFile())
	{
		return JSON::parse(f);
	}
	else
	{
		reportScriptError("File not found");
		return var::undefined();
	}
}

var ScriptingApi::Engine::getUserPresetDirectoryContent()
{
#if USE_FRONTEND
    
    const ValueTree v = dynamic_cast<FrontendProcessor*>(getScriptProcessor()->getMainController())->getPresetData();
    
    var returnArray;
    
    for(int i = 0; i < v.getNumChildren(); i++)
    {
        returnArray.append(v.getChild(i).getProperty("FileName"));
    }
    
    return returnArray;
    
#else
	File presetDirectory = GET_PROJECT_HANDLER(getScriptProcessor()).getSubDirectory(ProjectHandler::SubDirectories::UserPresets);

	if (presetDirectory.exists() && presetDirectory.isDirectory())
	{
		DirectoryIterator iter(GET_PROJECT_HANDLER(getScriptProcessor()).getSubDirectory(ProjectHandler::SubDirectories::UserPresets), false, "*", File::findFiles);

		var returnArray;

		while (iter.next())
		{
			returnArray.append(iter.getFile().getFileName());
		}

		return returnArray;
	}
	else
	{
		return var::undefined();
	}
#endif
    
}



void ScriptingApi::Engine::setCompileProgress(var progress)
{
	ScriptProcessor *sp = dynamic_cast<ScriptProcessor*>(getScriptProcessor());

	if (sp != nullptr)
	{
		sp->setCompileProgress((double)progress);
	}
}



bool ScriptingApi::Engine::matchesRegex(String stringToMatch, String wildcard)
{
#if STANDALONE_CONVOLUTION
#else
	try
	{
		std::regex reg(wildcard.toStdString());

		return std::regex_search(stringToMatch.toStdString(), reg);
	}
	catch (std::regex_error e)
	{
		debugError(getScriptProcessor(), e.what());
		return false;
	}
#endif
}

var ScriptingApi::Engine::getRegexMatches(String stringToMatch, String wildcard)
{
#if STANDALONE_CONVOLUTION
#else
    try
    {
        std::string s = stringToMatch.toStdString();
        
        std::regex reg(wildcard.toStdString());
        
        std::smatch match;
        

        
        if (std::regex_search(s, match, reg))
        {
            var returnArray = var();
            
            for (auto x:match)
            {
                returnArray.insert(-1, String(x));
            }
            
            return returnArray;
        }
    }
    catch (std::regex_error e)
    {
        debugError(getScriptProcessor(), e.what());
        return var::undefined();
    }
    
    return var::undefined();
#endif
}

String ScriptingApi::Engine::doubleToString(double value, int digits)
{
    return String(value, digits);
}

// ====================================================================================================== Synth functions


ScriptingApi::Synth::Synth(ScriptBaseProcessor *p, ModulatorSynth *ownerSynth):
	ScriptingObject(p),
	owner(ownerSynth),
	numPressedKeys(0),
	sustainState(false)
{
	jassert(owner != nullptr);

	setMethod("allowChildSynth", Wrapper::allowChildSynth);
	setMethod("getNumChildSynths", Wrapper::getNumChildSynths);
	setMethod("addToFront", Wrapper::addToFront);
	setMethod("deferCallbacks", Wrapper::deferCallbacks);
	setMethod("noteOff", Wrapper::noteOff);
	setMethod("playNote", Wrapper::playNote);
	setMethod("setAttribute", Wrapper::setAttribute);
	setMethod("getAttribute", Wrapper::getAttribute);
	setMethod("addNoteOn", Wrapper::addNoteOn);
	setMethod("addNoteOff", Wrapper::addNoteOff);
	setMethod("addController", Wrapper::addController);
	setMethod("startTimer", Wrapper::startTimer);
	setMethod("stopTimer", Wrapper::stopTimer);
	setMethod("setMacroControl", Wrapper::setMacroControl);
	setMethod("sendController", Wrapper::sendController);
	setMethod("sendControllerToChildSynths", Wrapper::sendControllerToChildSynths);
	setMethod("setModulatorAttribute", Wrapper::setModulatorAttribute);
	setMethod("addModulator", Wrapper::addModulator);
	setMethod("getModulator", Wrapper::getModulator);
	setMethod("getAudioSampleProcessor", Wrapper::getAudioSampleProcessor);
	setMethod("getTableProcessor", Wrapper::getTableProcessor);
	setMethod("getSampler", Wrapper::getSampler);
	setMethod("getEffect", Wrapper::getEffect);
	setMethod("getMidiProcessor", Wrapper::getMidiProcessor);
	setMethod("getChildSynth", Wrapper::getChildSynth);
	setMethod("getModulatorIndex", Wrapper::getModulatorIndex);
	setMethod("getNumPressedKeys", Wrapper::getNumPressedKeys);
	setMethod("isLegatoInterval", Wrapper::isLegatoInterval);
	setMethod("isSustainPedalDown", Wrapper::isSustainPedalDown);
	setMethod("setClockSpeed", Wrapper::setClockSpeed);
	
};

void ScriptingApi::Synth::allowChildSynth(int synthIndex, bool shouldBeAllowed)
{
	if(dynamic_cast<ModulatorSynthGroup*>(owner) == nullptr) 
	{
		reportScriptError("allowChildSynth() can only be called on SynthGroups!");
		return;
	}

	static_cast<ModulatorSynthGroup*>(owner)->allowChildSynth(synthIndex, shouldBeAllowed);
};

int ScriptingApi::Synth::getNumChildSynths() const
{
	if(dynamic_cast<Chain*>(owner) == nullptr) 
	{
		reportScriptError("getNumChildSynths() can only be called on Chains!");
		return -1;
	}

	return dynamic_cast<Chain*>(owner)->getHandler()->getNumProcessors();
};

void ScriptingApi::Synth::noteOff(int noteNumber)
{
	jassert(owner != nullptr);

	// Set the timestamp to the future if this is called in the note off callback to prevent wrong order.
	const int timestamp = 1;

	addNoteOff(1, noteNumber, timestamp);

#if 0
	ModulatorSynthChain *ownerChain = dynamic_cast<ModulatorSynthChain*>(owner);

	if (ownerChain != nullptr)
	{
		const int numProcessors = ownerChain->getHandler()->getNumProcessors();

		for (int i = 0; i < numProcessors; i++)
		{
			ModulatorSynth* childSynth = dynamic_cast<ModulatorSynth*>(ownerChain->getHandler()->getProcessor(i));

			childSynth->noteOff(1, noteNumber, 1.0f, true);
		}
	}
	else
	{
		owner->noteOff(1, noteNumber, 1.0f, true);
	}
#endif
}

void ScriptingApi::Synth::addToFront(bool addToFront)
{
	
	dynamic_cast<ScriptProcessor*>(getScriptProcessor())->addToFront(addToFront);
}

void ScriptingApi::Synth::deferCallbacks(bool deferCallbacks)
{
	dynamic_cast<ScriptProcessor*>(getScriptProcessor())->deferCallbacks(deferCallbacks);
}

void ScriptingApi::Synth::playNote(int noteNumber, int velocity)
{
	if(velocity == 0)
	{
		reportScriptError("A velocity of 0 is not valid!");
		return;
	}

	// Set the timestamp to the future if this is called in the note off callback to prevent wrong order.
	const int timestamp = getScriptProcessor()->getCurrentMidiMessage().isNoteOff(false) ? 1 : 0;

	addNoteOn(1, noteNumber, velocity, timestamp);
}


void ScriptingApi::Synth::startTimer(double intervalInSeconds)
{
	if(intervalInSeconds < 0.04)
	{
		reportScriptError("Go easy on the timer!");
		return;
	}

	ScriptProcessor *p = dynamic_cast<ScriptProcessor*>(getScriptProcessor());

	if(p != nullptr && p->isDeferred())
	{
		owner->stopSynthTimer();
		p->startTimer((int)(intervalInSeconds * 1000));
	}
	else
	{
		owner->startSynthTimer(intervalInSeconds);
	}
}

void ScriptingApi::Synth::stopTimer()
{
	ScriptProcessor *p = dynamic_cast<ScriptProcessor*>(getScriptProcessor());

	if(p != nullptr && p->isDeferred())
	{
		owner->stopSynthTimer();
		p->stopTimer();
	}
	else
	{
		owner->stopSynthTimer();
	}
}

void ScriptingApi::Synth::sendController(int controllerNumber, int controllerValue)
{
	if(controllerNumber == 128) owner->handlePitchWheel(1, controllerValue);

	MidiMessage m = MidiMessage::controllerEvent(1, controllerNumber, controllerValue);

	owner->gainChain->handleMidiEvent(m);
	owner->pitchChain->handleMidiEvent(m);
	owner->effectChain->handleMidiEvent(m);
};

void ScriptingApi::Synth::sendControllerToChildSynths(int controllerNumber, int controllerValue)
{
	

	MidiMessage m = MidiMessage::controllerEvent(1, controllerNumber, controllerValue);

	owner->addGeneratedMidiMessageToNextBlock(m);

};



void ScriptingApi::Synth::setMacroControl(int macroIndex, float newValue)
{
	if(ModulatorSynthChain *chain = dynamic_cast<ModulatorSynthChain*>(owner))
	{
		if(macroIndex > 0 && macroIndex < 8)
		{
			chain->setMacroControl(macroIndex - 1, newValue, sendNotification);
		}
		else reportScriptError("macroIndex must be between 1 and 8!");
	}
	else
	{
		reportScriptError("setMacroControl() can only be called on ModulatorSynthChains");
	}

}

ScriptingObjects::ScriptingModulator *ScriptingApi::Synth::getModulator(const String &name)
{
	if(getScriptProcessor()->objectsCanBeCreated())
	{
		Processor::Iterator<Modulator> it(owner);

		Modulator *m;
		

		while((m = it.getNextProcessor()) != 0)
		{
			if(m->getId() == name)
			{
				debugToConsole(owner, m->getId() + " was found. ");
				return new ScriptingObjects::ScriptingModulator(getScriptProcessor(), m);	
			}
		}

		return new ScriptingObjects::ScriptingModulator(getScriptProcessor(), nullptr);
	}
	else
	{
		reportIllegalCall("getModulator()", "onInit");

		return new ScriptingObjects::ScriptingModulator(getScriptProcessor(), nullptr);

	}	
}


ScriptingObjects::ScriptingMidiProcessor *ScriptingApi::Synth::getMidiProcessor(const String &name)
{
	if(name == getScriptProcessor()->getId())
	{
		reportScriptError("You can't get a reference to yourself!");
	}

	if(getScriptProcessor()->objectsCanBeCreated())
	{
		Processor::Iterator<MidiProcessor> it(owner);

		MidiProcessor *mp;
		
		while((mp = it.getNextProcessor()) != nullptr)
		{
			if(mp->getId() == name)
			{
				debugToConsole(owner, mp->getId() + " was found. ");
				return new ScriptingObjects::ScriptingMidiProcessor(getScriptProcessor(), mp);	
			}
		}

		return new ScriptMidiProcessor(getScriptProcessor(), nullptr);
	}
	else
	{
		reportIllegalCall("getMidiProcessor()", "onInit");

		return new ScriptMidiProcessor(getScriptProcessor(), nullptr);

	}	
}


ScriptingObjects::ScriptingSynth *ScriptingApi::Synth::getChildSynth(const String &name)
{
	if(getScriptProcessor()->objectsCanBeCreated())
	{
		Processor::Iterator<ModulatorSynth> it(owner);

		ModulatorSynth *m;
		

		while((m = it.getNextProcessor()) != 0)
		{
			if(m->getId() == name)
			{
				debugToConsole(owner, m->getId() + " was found. ");
				return new ScriptingObjects::ScriptingSynth(getScriptProcessor(), m);	
			}
		}

		return new ScriptingObjects::ScriptingSynth(getScriptProcessor(), nullptr);
	}
	else
	{
		reportIllegalCall("getChildSynth()", "onInit");

		return new ScriptingObjects::ScriptingSynth(getScriptProcessor(), nullptr);

	}	
}

ScriptingObjects::ScriptingEffect *ScriptingApi::Synth::getEffect(const String &name)
{
	if(getScriptProcessor()->objectsCanBeCreated())
	{
		Processor::Iterator<EffectProcessor> it(owner);

		EffectProcessor *fx;
		

		while((fx = it.getNextProcessor()) != nullptr)
		{
			if(fx->getId() == name)
			{
				debugToConsole(owner, fx->getId() + " was found. ");
				return new ScriptEffect(getScriptProcessor(), fx);	
			}
		}

		return new ScriptEffect(getScriptProcessor(), nullptr);
	}
	else
	{
		reportIllegalCall("getEffect()", "onInit");

		return new ScriptEffect(getScriptProcessor(), nullptr);

	}	
}

ScriptingObjects::ScriptingAudioSampleProcessor * ScriptingApi::Synth::getAudioSampleProcessor(const String &name)
{
	if (getScriptProcessor()->objectsCanBeCreated())
	{
		Processor::Iterator<AudioSampleProcessor> it(owner);

		AudioSampleProcessor *asp;


		while ((asp = it.getNextProcessor()) != nullptr)
		{
			if (dynamic_cast<Processor*>(asp)->getId() == name)
			{
				debugToConsole(owner, dynamic_cast<Processor*>(asp)->getId() + " was found. ");
				return new ScriptAudioSampleProcessor(getScriptProcessor(), asp);
			}
		}

		return new ScriptAudioSampleProcessor(getScriptProcessor(), nullptr);
	}
	else
	{
		reportIllegalCall("getScriptingAudioSampleProcessor()", "onInit");

		return new ScriptAudioSampleProcessor(getScriptProcessor(), nullptr);

	}
}


ScriptingObjects::ScriptingTableProcessor *ScriptingApi::Synth::getTableProcessor(const String &name)
{
	if (getScriptProcessor()->objectsCanBeCreated())
	{
		Processor::Iterator<LookupTableProcessor> it(owner);

		while (LookupTableProcessor *lut = it.getNextProcessor())
		{
			if (dynamic_cast<Processor*>(lut)->getId() == name)
			{
				debugToConsole(owner, dynamic_cast<Processor*>(lut)->getId() + " was found. ");
				return new ScriptTableProcessor(getScriptProcessor(), lut);
			}
		}

		return new ScriptTableProcessor(getScriptProcessor(), nullptr);
	}
	else
	{
		reportIllegalCall("getScriptingTableProcessor()", "onInit");

		return new ScriptTableProcessor(getScriptProcessor(), nullptr);
	}
}

ScriptingApi::Sampler * ScriptingApi::Synth::getSampler(const String &name)
{
	if (getScriptProcessor()->objectsCanBeCreated())
	{
		Processor::Iterator<ModulatorSampler> it(owner);

		while (ModulatorSampler *s = it.getNextProcessor())
		{
			if (s->getId() == name)
			{
				debugToConsole(owner, s->getId() + " was found. ");
				return new Sampler(getScriptProcessor(), s);
			}
		}

		return new Sampler(getScriptProcessor(), nullptr);
	}
	else
	{
		reportIllegalCall("getScriptingAudioSampleProcessor()", "onInit");

		return new Sampler(getScriptProcessor(), nullptr);
	}
}

void ScriptingApi::Synth::setAttribute(int attributeIndex, float newAttribute)
{
	if(owner == nullptr)
	{
		jassertfalse;
		return;
	}

	owner->setAttribute(attributeIndex, newAttribute, sendNotification);
}

float ScriptingApi::Synth::getAttribute(int attributeIndex) const
{
	if (owner == nullptr)
	{
		jassertfalse;
		return -1.0f;
	}

	return owner->getAttribute(attributeIndex);
}

void ScriptingApi::Synth::addNoteOn(int channel, int noteNumber, int velocity, int timeStampSamples)
{
	if (channel > 0 && channel <= 16)
	{
		if (noteNumber >= 0 && noteNumber < 127)
		{
			if (velocity >= 0 && velocity <= 127)
			{
				if (timeStampSamples >= 0)
				{
					MidiMessage m = MidiMessage::noteOn(channel, noteNumber, (uint8)velocity);
					m.setTimeStamp(getScriptProcessor()->getCurrentMidiMessage().getTimeStamp() + timeStampSamples);

					getScriptProcessor()->addMidiMessageToBuffer(m);
				}
				else reportScriptError("Timestamp must be >= 0");
			}
			else reportScriptError("Velocity must be between 0 and 127");
		}
		else reportScriptError("Note number must be between 0 and 127");
	}
	else reportScriptError("Channel must be between 1 and 16.");
}

void ScriptingApi::Synth::addNoteOff(int channel, int noteNumber, int timeStampSamples)
{
	if (channel > 0 && channel <= 16)
	{
		if (noteNumber >= 0 && noteNumber < 127)
		{
			if (timeStampSamples >= 0)
			{
				timeStampSamples = jmax<int>(1, timeStampSamples);

				MidiMessage m = MidiMessage::noteOff(channel, noteNumber);
				m.setTimeStamp(getScriptProcessor()->getCurrentMidiMessage().getTimeStamp() + timeStampSamples);

				getScriptProcessor()->addMidiMessageToBuffer(m);
			}
			else reportScriptError("Timestamp must be > 0");
		}
		else reportScriptError("Note number must be between 0 and 127");
	}
	else reportScriptError("Channel must be between 1 and 16.");
}

void ScriptingApi::Synth::addController(int channel, int number, int value, int timeStampSamples)
{
	if (channel > 0 && channel <= 16)
	{
		if (number >= 0 && number <= 127)
		{
			if (value >= 0 && value <= 127)
			{
				if (timeStampSamples >= 0)
				{
					MidiMessage m = MidiMessage::controllerEvent(channel, number, value);
					m.setTimeStamp(getScriptProcessor()->getCurrentMidiMessage().getTimeStamp() + timeStampSamples);

					getScriptProcessor()->addMidiMessageToBuffer(m);
				}
				else reportScriptError("Timestamp must be > 0");
			}
			else reportScriptError("CC Value must be between 0 and 127");
		}
		else reportScriptError("CC number must be between 0 and 127");
	}
	else reportScriptError("Channel must be between 1 and 16.");
}

void ScriptingApi::Synth::setClockSpeed(int clockSpeed)
{
	switch (clockSpeed)
	{
	case 0:	 owner->setClockSpeed(ModulatorSynth::Inactive); break;
	case 1:  owner->setClockSpeed(ModulatorSynth::ClockSpeed::Bar); break;
	case 2:  owner->setClockSpeed(ModulatorSynth::ClockSpeed::Half); break;
	case 4:  owner->setClockSpeed(ModulatorSynth::ClockSpeed::Quarters); break;
	case 8:  owner->setClockSpeed(ModulatorSynth::ClockSpeed::Eighths); break;
	case 16: owner->setClockSpeed(ModulatorSynth::ClockSpeed::Sixteens); break;
	case 32: owner->setClockSpeed(ModulatorSynth::ThirtyTwos); break;
	default: reportScriptError("Unknown clockspeed. Use 1,2,4,8,16 or 32");
	}
}

void ScriptingApi::Synth::setModulatorAttribute(int chain, int modulatorIndex, int attributeIndex, float newValue)
{
	if(owner == nullptr)
	{
		jassertfalse;
		return;
	}

	ModulatorChain *c = nullptr;

	switch(chain)
	{
	case ModulatorSynth::GainModulation:	c = owner->gainChain; break;
	case ModulatorSynth::PitchModulation:	c = owner->pitchChain; break;
	default: jassertfalse; reportScriptError("No valid chainType - 1= GainModulation, 2=PitchModulation"); return;
	}

	Processor *modulator = c->getHandler()->getProcessor(modulatorIndex);

	if(modulator == nullptr)
	{
		String errorMessage;
		errorMessage << "No Modulator found in " << (chain == 1 ? "GainModulation" : "PitchModulation") << " at index " << String(modulatorIndex);
		reportScriptError(errorMessage);
		return;
	}

	if(attributeIndex == -12)
	{
		if (chain == ModulatorSynth::PitchModulation) 
		{
			const float pitchValue = jlimit(0.5f, 2.0f, powf(2, newValue/12.0f));

			dynamic_cast<Modulation*>(modulator)->setIntensity(pitchValue);
		}
		else
		{
			dynamic_cast<Modulation*>(modulator)->setIntensity(newValue);
			
		}
	}
	else if(attributeIndex == -13) modulator->setBypassed(newValue == 1.0f);
	
	else modulator->setAttribute(attributeIndex, newValue, dontSendNotification);

	modulator->sendChangeMessage();

	
}


ScriptingApi::Sampler::Sampler(ScriptBaseProcessor *p, ModulatorSampler *sampler_) :
CreatableScriptObject(p),
sampler(sampler_)
{
	setMethod("enableRoundRobin", Wrapper::enableRoundRobin);
	setMethod("setActiveGroup", Wrapper::setActiveGroup);
	setMethod("getRRGroupsForMessage", Wrapper::getRRGroupsForMessage);
	setMethod("refreshRRMap", Wrapper::refreshRRMap);

	setMethod("selectSounds", Wrapper::selectSounds);
	setMethod("getNumSelectedSounds", Wrapper::getNumSelectedSounds);
	setMethod("setSoundPropertyForSelection", Wrapper::setSoundPropertyForSelection);
	setMethod("getSoundProperty", Wrapper::getSoundProperty);
	setMethod("setSoundProperty", Wrapper::setSoundProperty);
	setMethod("purgeMicPosition", Wrapper::purgeMicPosition);
	setMethod("getMicPositionName", Wrapper::getMicPositionName);
	setMethod("refreshInterface", Wrapper::refreshInterface);
	setMethod("loadSampleMap", Wrapper::loadSampleMap);

	for (int i = 1; i < ModulatorSamplerSound::numProperties; i++)
	{
		setProperty(Identifier(ModulatorSamplerSound::getPropertyName((ModulatorSamplerSound::Property)i)), i);
	}
}

void ScriptingApi::Sampler::enableRoundRobin(bool shouldUseRoundRobin)
{
	ModulatorSampler *s = dynamic_cast<ModulatorSampler*>(sampler.get());

	if (s != nullptr)
	{
		s->setUseRoundRobinLogic(shouldUseRoundRobin);

		getScriptProcessor()->setScriptProcessorDeactivatedRoundRobin(!shouldUseRoundRobin);

	}
	else
	{
		reportScriptError("setActiveGroup() only works with Samplers.");
	}
}

void ScriptingApi::Sampler::setActiveGroup(int activeGroupIndex)
{
	ModulatorSampler *s = dynamic_cast<ModulatorSampler*>(sampler.get());

	if (s == nullptr)
	{
		reportScriptError("setActiveGroup() only works with Samplers.");
		return;
	}
	
	if (s->isRoundRobinEnabled())
	{
		reportScriptError("Round Robin is not disabled. Call 'Synth.enableRoundRobin(false)' before calling this method.");
		return;
	}

	bool ok = s->setCurrentGroupIndex(activeGroupIndex);

	if (!ok)
	{
		reportScriptError(String(activeGroupIndex) + " is not a valid group index.");
	}
}

int ScriptingApi::Sampler::getRRGroupsForMessage(int noteNumber, int velocity)
{
	ModulatorSampler *s = dynamic_cast<ModulatorSampler*>(sampler.get());

	if (s == nullptr)
	{
		reportScriptError("getRRGroupsForMessage() only works with Samplers.");
		return 0;
	}

	if (s->isRoundRobinEnabled())
	{
		reportScriptError("Round Robin is not disabled. Call 'Synth.enableRoundRobin(false)' before calling this method.");
		return 0;
	}

	return s->getRRGroupsForMessage(noteNumber, velocity);
}

void ScriptingApi::Sampler::refreshRRMap()
{
	ModulatorSampler *s = dynamic_cast<ModulatorSampler*>(sampler.get());

	if (s == nullptr)
	{
		reportScriptError("refreshRRMap() only works with Samplers.");
		return;
	}

	if (s->isRoundRobinEnabled())
	{
		reportScriptError("Round Robin is not disabled. Call 'Synth.enableRoundRobin(false)' before calling this method.");
		return;
	}

	s->refreshRRMap();
}

void ScriptingApi::Sampler::selectSounds(String regexWildcard)
{
	ModulatorSampler *s = dynamic_cast<ModulatorSampler*>(sampler.get());

	if (s == nullptr)
	{
		reportScriptError("selectSamplerSounds() only works with Samplers.");
		return;
	}

	ModulatorSamplerSound::selectSoundsBasedOnRegex(regexWildcard, s, soundSelection);
}

int ScriptingApi::Sampler::getNumSelectedSounds()
{
	ModulatorSampler *s = dynamic_cast<ModulatorSampler*>(sampler.get());

	if (s == nullptr)
	{
		reportScriptError("getNumSelectedSamplerSounds() only works with Samplers.");
		return -1;
	}

	return soundSelection.getNumSelected();
}

void ScriptingApi::Sampler::setSoundPropertyForSelection(int propertyId, var newValue)
{
	ModulatorSampler *s = dynamic_cast<ModulatorSampler*>(sampler.get());

	if (s == nullptr)
	{
		reportScriptError("setSoundsProperty() only works with Samplers.");
		return;
	}

	Array<WeakReference<ModulatorSamplerSound>> sounds = soundSelection.getItemArray();

	const int numSelected = sounds.size();

	for (int i = 0; i < numSelected; i++)
	{
		if (sounds[i].get() != nullptr)
		{
			sounds[i]->setProperty((ModulatorSamplerSound::Property)propertyId, newValue, dontSendNotification);
		}
	}
}

var ScriptingApi::Sampler::getSoundProperty(int propertyIndex, int soundIndex)
{
	ModulatorSampler *s = dynamic_cast<ModulatorSampler*>(sampler.get());

	if (s == nullptr)
	{
		reportScriptError("getSoundProperty() only works with Samplers.");
		return var::undefined();
	}

	ModulatorSamplerSound *sound = soundSelection.getSelectedItem(soundIndex);

	if (sound != nullptr)
	{
		return sound->getProperty((ModulatorSamplerSound::Property)propertyIndex);
	}
	else
	{
		reportScriptError("no sound with index " + String(soundIndex));
		return var::undefined();
	}
	
}

void ScriptingApi::Sampler::setSoundProperty(int soundIndex, int propertyIndex, var newValue)
{
	ModulatorSampler *s = dynamic_cast<ModulatorSampler*>(sampler.get());

	if (s == nullptr)
	{
		reportScriptError("ssetSoundProperty() only works with Samplers.");
	}

	ModulatorSamplerSound *sound = soundSelection.getSelectedItem(soundIndex);

	if (sound != nullptr)
	{
		sound->setProperty((ModulatorSamplerSound::Property)propertyIndex, newValue, dontSendNotification);
	}
	else
	{
		reportScriptError("no sound with index " + String(soundIndex));
	}
}

void ScriptingApi::Sampler::purgeMicPosition(String micName, bool shouldBePurged)
{
	ModulatorSampler *s = dynamic_cast<ModulatorSampler*>(sampler.get());

	if (s == nullptr)
	{
		reportScriptError("purgeMicPosition() only works with Samplers.");
		return;
	}

	if (s->getNumMicPositions() == 1)
	{
		reportScriptError("purgeMicPosition() only works with multi mic Samplers.");
		return;
	}

	for (int i = 0; i < s->getNumMicPositions(); i++)
	{
		if (micName == s->getChannelData(i).suffix)
		{	
			s->setMicEnabled(i, !shouldBePurged);
			
			return;
		}
	}

	reportScriptError("Channel not found. Use getMicPositionName()");
}

String ScriptingApi::Sampler::getMicPositionName(int channelIndex) 
{
	ModulatorSampler *s = dynamic_cast<ModulatorSampler*>(sampler.get());

	if (s == nullptr)
	{
		reportScriptError("purgeMicPosition() only works with Samplers.");
		return "";
	}

	if (s->getNumMicPositions() == 1)
	{
		reportScriptError("purgeMicPosition() only works with multi mic Samplers.");
		return "";
	}

	return s->getChannelData(channelIndex).suffix;
}

void ScriptingApi::Sampler::refreshInterface()
{
	ModulatorSampler *s = dynamic_cast<ModulatorSampler*>(sampler.get());

	if (s == nullptr)
	{
		reportScriptError("purgeMicPosition() only works with Samplers.");
		return ;
	}

	s->sendChangeMessage();
	s->getMainController()->getSampleManager().getModulatorSamplerSoundPool()->sendChangeMessage();
}

void ScriptingApi::Sampler::loadSampleMap(const String &fileName)
{
	ModulatorSampler *s = dynamic_cast<ModulatorSampler*>(sampler.get());

	if (s == nullptr)
	{
		reportScriptError("loadSampleMap() only works with Samplers.");
		return;
	}

	File f = GET_PROJECT_HANDLER(s).getSubDirectory(ProjectHandler::SubDirectories::SampleMaps).getChildFile(fileName + ".xml");

	if (!f.existsAsFile())
	{
		reportScriptError("Samplemap " + f.getFileName() + " not found.");
		return;
	}

	XmlDocument doc(f);

	ScopedPointer<XmlElement> xml = doc.getDocumentElement();

	if (xml != nullptr)
	{
		ValueTree v = ValueTree::fromXml(*xml);

        static const Identifier unused = Identifier("unused");
        const Identifier oldId = s->getSampleMap()->getId();
        const Identifier newId = Identifier(v.getProperty("ID", "unused").toString());
        
        if(newId != unused && newId != oldId)
        {
            s->loadSampleMap(v);
            s->sendChangeMessage();
            s->getMainController()->getSampleManager().getModulatorSamplerSoundPool()->sendChangeMessage();
        }
        
	}
	else
	{
		reportScriptError("Error when loading sample map: " + doc.getLastParseError());
	}

	int maxGroup = 1;

	for (int i = 0; i < s->getNumSounds(); i++)
	{
		maxGroup = jmax<int>(maxGroup, s->getSound(i)->getProperty(ModulatorSamplerSound::RRGroup));
	}

	s->setAttribute(ModulatorSampler::RRGroupAmount, (float)maxGroup, sendNotification);
}

int ScriptingApi::Synth::addModulator(int chain, const String &type, const String &id) const
{
	ModulatorChain *c = nullptr;

	switch(chain)
	{
	case ModulatorSynth::GainModulation:	c = owner->gainChain; break;
	case ModulatorSynth::PitchModulation:	c = owner->pitchChain; break;
	default: jassertfalse; reportScriptError("No valid chainType - 1= GainModulation, 2=PitchModulation"); return -1;
	}

	for(int i = 0; i < c->getHandler()->getNumProcessors(); i++)
	{
		if(c->getHandler()->getProcessor(i)->getId() == id) return i;
	}

	Processor *p = getScriptProcessor()->getMainController()->createProcessor(c->getFactoryType(), type, id);

	if(p == nullptr)
	{
		reportScriptError("Modulator with type " + type + " could not be generated.");
		return -1;
	}

	jassert(dynamic_cast<Modulator*>(p) != nullptr);

	c->getHandler()->add(p, nullptr);

	SEND_MESSAGE(c);

	int index = c->getHandler()->getNumProcessors() - 1;

#if USE_BACKEND
	p->getMainController()->writeToConsole("Modulator " + id + " added to " + c->getId() + " at index " + String(index), 0, p, getScriptProcessor()->getScriptingContent()->getColour());
#endif

	return index;
}

int ScriptingApi::Synth::getModulatorIndex(int chain, const String &id) const
{
	ModulatorChain *c = nullptr;

	switch(chain)
	{
	case ModulatorSynth::GainModulation:	c = owner->gainChain; break;
	case ModulatorSynth::PitchModulation:	c = owner->pitchChain; break;
	default: jassertfalse; reportScriptError("No valid chainType - 1= GainModulation, 2=PitchModulation"); return -1;
	}

	for(int i = 0; i < c->getHandler()->getNumProcessors(); i++)
	{
		if(c->getHandler()->getProcessor(i)->getId() == id) return i;
	}

	reportScriptError("Modulator " + id + " was not found in " + c->getId());

	return -1;

}

// ====================================================================================================== Console functions

void ScriptingApi::Console::print(var x)
{
#if USE_BACKEND
	debugToConsole(getScriptProcessor(), x);
#endif
}

void ScriptingApi::Console::stop()
{
#if USE_BACKEND
	if(startTime == 0.0)
	{
		reportScriptError("The Benchmark was not started!");
		return;
	}

	const double now = Time::highResolutionTicksToSeconds(Time::getHighResolutionTicks());
	const double ms = (now - startTime) * 1000.0;
	startTime = 0.0;

	debugToConsole(getScriptProcessor(), benchmarkTitle + " Benchmark Result: " + String(ms, 3) + " ms");
#endif
}


// ====================================================================================================== Content functions


template <class Subtype> Subtype* ScriptingApi::Content::addComponent(Identifier name, int x, int y, int width, int height)
{
	if( ! allowGuiCreation)
	{
		reportScriptError("Tried to add a component after onInit()");
		return nullptr;
	}

	for(int i = 0; i < components.size(); i++)
	{
		if(components[i]->name == name) return dynamic_cast<Subtype*>(components[i].get());
	}

	Subtype *t = new Subtype(getScriptProcessor(), this, name, x, y, width, height);

	components.add(t);

	var savedValue = getScriptProcessor()->getSavedValue(name);

	if(!savedValue.isUndefined())
	{
		components.getLast()->value = savedValue;
	}

	return t;

}

ScriptingApi::Content::ScriptComponent * ScriptingApi::Content::getComponentWithName(const Identifier &componentName)
{
	for (int i = 0; i < getNumComponents(); i++)
	{
		if (components[i]->name == componentName)
		{
			return components[i];
		}
	}

	return nullptr;
}

const ScriptingApi::Content::ScriptComponent * ScriptingApi::Content::getComponentWithName(const Identifier &componentName) const
{
	for (int i = 0; i < getNumComponents(); i++)
	{
		if (components[i]->name == componentName)
		{
			return components[i];
		}
	}

	return nullptr;
}

ScriptingApi::Content::ScriptComboBox *ScriptingApi::Content::addComboBox(Identifier boxName, int x, int y)
{
	return addComponent<ScriptComboBox>(boxName, x, y, 128, 32); 
}

ScriptingApi::Content::ScriptButton * ScriptingApi::Content::addButton(Identifier buttonName, int x, int y)
{
	return addComponent<ScriptButton>(buttonName, x, y);
};

ScriptingApi::Content::ScriptSlider * ScriptingApi::Content::addKnob(Identifier knobName, int x, int y)
{
	return addComponent<ScriptSlider>(knobName, x, y);
};

ScriptingApi::Content::ScriptImage * ScriptingApi::Content::addImage(Identifier knobName, int x, int y)
{
	return addComponent<ScriptImage>(knobName, x, y, 50, 50);
};

ScriptingApi::Content::ScriptLabel * ScriptingApi::Content::addLabel(Identifier labelName, int x, int y)
{
	return addComponent<ScriptLabel>(labelName, x, y, 100, 50);
};

ScriptingApi::Content::ScriptTable * ScriptingApi::Content::addTable(Identifier labelName, int x, int y)
{
	return addComponent<ScriptTable>(labelName, x, y, 100, 50);
};

ScriptingApi::Content::ModulatorMeter * ScriptingApi::Content::addModulatorMeter(Identifier modulatorName, int x, int y)
{
	ModulatorMeter *m = addComponent<ModulatorMeter>(modulatorName, x, y, 100, 50);

	m->setScriptProcessor(getScriptProcessor());

	return m;
	
};

ScriptingApi::Content::ScriptedPlotter * ScriptingApi::Content::addPlotter(Identifier plotterName, int x, int y)
{
	return addComponent<ScriptedPlotter>(plotterName, x, y, 100, 50);

};

ScriptingApi::Content::ScriptPanel * ScriptingApi::Content::addPanel(Identifier panelName, int x, int y)
{
	return addComponent<ScriptPanel>(panelName, x, y, 100, 50);

};

ScriptingApi::Content::ScriptAudioWaveform * ScriptingApi::Content::addAudioWaveform(Identifier audioWaveformName, int x, int y)
{
	return addComponent<ScriptAudioWaveform>(audioWaveformName, x, y, 100, 50);
}

ScriptingApi::Content::ScriptSliderPack * ScriptingApi::Content::addSliderPack(Identifier sliderPackName, int x, int y)
{
	return addComponent<ScriptSliderPack>(sliderPackName, x, y, 200, 100);
}

ScriptingApi::Content::ScriptPluginEditor * ScriptingApi::Content::addPluginEditor(Identifier pluginEditorName, int x, int y)
{
	return addComponent<ScriptPluginEditor>(pluginEditorName, x, y);
}

ScriptCreatedComponentWrapper * ScriptingApi::Content::ScriptedPlotter::createComponentWrapper(ScriptContentComponent *content, int index)
{
	return new ScriptCreatedComponentWrappers::PlotterWrapper(content, this, index);
}

void ScriptingApi::Content::ScriptedPlotter::addModulatorToPlotter(String processorName, String modulatorName)
{
	Processor::Iterator<ModulatorSynth> synthIter(getScriptProcessor()->getMainController()->getMainSynthChain());

	ModulatorSynth *synth;

	while((synth = synthIter.getNextProcessor()) != nullptr)
	{
		if(synth->getId() == processorName)
		{
			Processor::Iterator<Modulator> modIter(synth);

			Modulator *m;

			while((m = modIter.getNextProcessor()) != nullptr)
			{
				if(m->getId() == modulatorName)
				{
					addModulator(m);
					return;
				}
			}
		}
	}

	reportScriptError(String(modulatorName) + " was not found");

}

void ScriptingApi::Content::ScriptedPlotter::clearModulatorPlotter()
{
	clearModulators();
}

StringArray ScriptingApi::Content::ScriptImage::getOptionsFor(const Identifier &id)
{
	if (id == getIdFor(FileName))
	{
		StringArray sa;

		sa.add("Load new File");

		sa.addArray(getScriptProcessor()->getMainController()->getSampleManager().getImagePool()->getFileNameList());

		return sa;
	}
	else if (id == getIdFor(AllowCallbacks))
	{
		return MouseCallbackComponent::getCallbackLevels();
	}
		
		
	return ScriptComponent::getOptionsFor(id);

	
}

void ScriptingApi::Content::ScriptImage::setScriptObjectPropertyWithChangeMessage(const Identifier &id, var newValue, NotificationType notifyEditor)
{
	CHECK_COPY_AND_RETURN_17(getScriptProcessor());

	if(id == getIdFor(FileName))
	{
		setImageFile(newValue, true);
	}

	ScriptComponent::setScriptObjectPropertyWithChangeMessage(id, newValue, notifyEditor);
}

void ScriptingApi::Content::ScriptImage::setImageFile(const String &absoluteFileName, bool forceUseRealFile)
{
	ignoreUnused(forceUseRealFile);

    const bool imageWasEmpty = (image == nullptr);
    
	CHECK_COPY_AND_RETURN_10(getScriptProcessor());

	if (absoluteFileName.isEmpty())
	{
		setScriptObjectProperty(FileName, absoluteFileName);
		return;
	}

	ImagePool *pool = getScriptProcessor()->getMainController()->getSampleManager().getImagePool();

	pool->releasePoolData(image);

	setScriptObjectProperty(FileName, absoluteFileName);
	
#if USE_FRONTEND

	String poolName = ProjectHandler::Frontend::getSanitiziedFileNameForPoolReference(absoluteFileName);

	image = pool->loadFileIntoPool(poolName, false);

	jassert (image != nullptr);

#else

	File actualFile = getExternalFile(absoluteFileName);

	image = pool->loadFileIntoPool(actualFile.getFullPathName(), forceUseRealFile);
	
#endif

	if(imageWasEmpty && image != nullptr)
	{
		setScriptObjectProperty(ScriptComponent::width, image->getWidth());
		setScriptObjectProperty(ScriptComponent::height, image->getHeight());
	}

	parent->sendChangeMessage();
};

ScriptingApi::Content::ScriptImage::ScriptImage(ScriptBaseProcessor *base, Content *parentContent, Identifier imageName, int x, int y, int width, int height) :
ScriptComponent(base, parentContent, imageName, x, y, width, height),
image(nullptr)
{
	deactivatedProperties.add(getIdFor(ScriptComponent::Properties::bgColour));
	deactivatedProperties.add(getIdFor(ScriptComponent::Properties::itemColour));
	deactivatedProperties.add(getIdFor(ScriptComponent::Properties::itemColour2));
	deactivatedProperties.add(getIdFor(ScriptComponent::Properties::max));
	deactivatedProperties.add(getIdFor(ScriptComponent::Properties::min));
	deactivatedProperties.add(getIdFor(ScriptComponent::Properties::textColour));
	deactivatedProperties.add(getIdFor(ScriptComponent::Properties::macroControl));

	propertyIds.add("alpha");		ADD_AS_SLIDER_TYPE(0.0, 1.0, 0.01);
	propertyIds.add("fileName");	ADD_TO_TYPE_SELECTOR(SelectorTypes::FileSelector);
    propertyIds.add("offset");
    propertyIds.add("scale");
    propertyIds.add("allowCallbacks");		ADD_TO_TYPE_SELECTOR(SelectorTypes::ChoiceSelector);
	propertyIds.add("popupMenuItems");		ADD_TO_TYPE_SELECTOR(SelectorTypes::MultilineSelector);
	propertyIds.add("popupOnRightClick");	ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);

    priorityProperties.add(getIdFor(FileName));
    
	componentProperties->setProperty(getIdFor(Alpha), 0);
	componentProperties->setProperty(getIdFor(FileName), 0);
    componentProperties->setProperty(getIdFor(Offset), 0);
    componentProperties->setProperty(getIdFor(Scale), 1.0);
    componentProperties->setProperty(getIdFor(AllowCallbacks), 0);
	componentProperties->setProperty(getIdFor(PopupMenuItems), "");
	componentProperties->setProperty(getIdFor(PopupOnRightClick), true);

    setDefaultValue(ScriptComponent::Properties::saveInPreset, false);
	setDefaultValue(Alpha, 1.0f);
	setDefaultValue(FileName, String::empty);
    setDefaultValue(Offset, 0);
    setDefaultValue(Scale, 1.0);
    setDefaultValue(AllowCallbacks, false);
	setDefaultValue(PopupMenuItems, "");
	setDefaultValue(PopupOnRightClick, true);

	setMethod("setImageFile", Wrapper::setImageFile);
	setMethod("setAlpha", Wrapper::setImageAlpha);
}

ScriptingApi::Content::ScriptImage::~ScriptImage()
{
	if(getScriptProcessor() != nullptr)
	{
		ImagePool *pool = getScriptProcessor()->getMainController()->getSampleManager().getImagePool();

		pool->releasePoolData(image);
	}
};



ScriptCreatedComponentWrapper * ScriptingApi::Content::ScriptImage::createComponentWrapper(ScriptContentComponent *content, int index)
{
	return new ScriptCreatedComponentWrappers::ImageWrapper(content, this, index);
}

void ScriptingApi::Content::ScriptComponent::addToMacroControl(int macroIndex)
{
	if( ! parent->allowGuiCreation)
	{
		reportScriptError("Tried to change the macro setup after onInit()");
		
	}

	int knobIndex = parent->components.indexOf(this);

	if(knobIndex == -1)
	{
		reportScriptError("Component not valid");
	}

	if(macroIndex == -1)
	{
		getScriptProcessor()->getMainController()->getMacroManager().removeMacroControlsFor(getScriptProcessor(), getName());
	}
	else
	{
		NormalisableRange<double> range(getScriptObjectProperty(Properties::min), getScriptObjectProperty(Properties::max));

		getScriptProcessor()->getMainController()->getMacroManager().getMacroChain()->addControlledParameter(
		macroIndex, getScriptProcessor()->getId(), knobIndex, name.toString(), range, false);
	}

	

	
};

void ScriptingApi::Content::ScriptComponent::showControl(bool shouldBeVisible)
{
	setScriptObjectPropertyWithChangeMessage(getIdFor(visible), shouldBeVisible);
};


void ScriptingApi::Content::ScriptSlider::setValueNormalized(double normalizedValue)
{
	const double minValue = getScriptObjectProperty(min);
	const double maxValue = getScriptObjectProperty(max);
	const double midPoint = getScriptObjectProperty(middlePosition);
	const double step = getScriptObjectProperty(stepSize);

	if (minValue < maxValue &&
		midPoint > minValue &&
		midPoint < maxValue &&
		step > 0.0)
	{
		const double skew = log(0.5) / log((midPoint - minValue) / (maxValue - minValue));

		NormalisableRange<double> range = NormalisableRange<double>(minValue, maxValue, step, skew);

		const double value = range.convertFrom0to1(normalizedValue);

		setValue(value);
	}
	else
	{


#if USE_BACKEND
		String errorMessage;

		errorMessage << "Slider range of " << getName().toString() << " is illegal: min: " << minValue << ", max: " << maxValue << ", middlePoint: " << midPoint << ", step: " << step;

		reportScriptError(errorMessage);
#endif
	}
}

double ScriptingApi::Content::ScriptSlider::getValueNormalized() const
{
	const double minValue = getScriptObjectProperty(min);
	const double maxValue = getScriptObjectProperty(max);
	const double midPoint = getScriptObjectProperty(middlePosition);
	const double step = getScriptObjectProperty(stepSize);

	if (minValue < maxValue &&
		midPoint > minValue &&
		midPoint < maxValue &&
		step > 0.0)
	{
		const double skew = log(0.5) / log((midPoint - minValue) / (maxValue - minValue));

		NormalisableRange<double> range = NormalisableRange<double>(minValue, maxValue, step, skew);

		const double value = getValue();

		return range.convertTo0to1(value);
	}
	else
	{


#if USE_BACKEND
		String errorMessage;

		errorMessage << "Slider range of " << getName().toString() << " is illegal: min: " << minValue << ", max: " << maxValue << ", middlePoint: " << midPoint << ", step: " << step;

		reportScriptError(errorMessage);
#endif

		return 0.0;
	}
}

void ScriptingApi::Content::ScriptSlider::setRange(double min_, double max_, double stepSize_)
{
	setScriptObjectProperty(ScriptComponent::Properties::min, var(min_));
	setScriptObjectProperty(ScriptComponent::Properties::max, var(max_));
	setScriptObjectProperty(Properties::stepSize, stepSize_);

	sendChangeMessage();
}

void ScriptingApi::Content::ScriptSlider::setMode(String mode)
{
	StringArray sa = getOptionsFor(getIdFor(Mode));

	const int index = sa.indexOf(mode);

	if(index == -1)
	{
		reportScriptError("invalid slider mode: " + mode);
		return;
	}

	m = (HiSlider::Mode) index;
	
	NormalisableRange<double> nr =  HiSlider::getRangeForMode(m);
	
	setScriptObjectProperty(Mode, mode);

	if((nr.end - nr.start) != 0)
	{
		setScriptObjectProperty(ScriptComponent::Properties::min, nr.start);
		setScriptObjectProperty(ScriptComponent::Properties::max,  nr.end);
		setScriptObjectProperty(stepSize, nr.interval);
		setScriptObjectProperty(ScriptSlider::Properties::suffix, HiSlider::getSuffixForMode(m, getValue()));
		setMidPoint(getScriptObjectProperty(ScriptSlider::Properties::middlePosition));
	}
}



void ScriptingApi::Content::ScriptComboBox::addItem(const String &itemName)
{
	String name = getScriptObjectProperty(Items);

	name.append("\n", 1);
	name.append(itemName, 128);

	setScriptObjectProperty(Items, name);

	int size = getScriptObjectProperty(max);

	setScriptObjectProperty(ScriptComponent::Properties::min, 1);
	setScriptObjectProperty(ScriptComponent::Properties::max, size+1);
    
    sendChangeMessage();
}


void ScriptingApi::Content::ScriptComponent::setTooltip(const String &newTooltip)
{
	setScriptObjectProperty(Properties::tooltip, newTooltip);
}

File ScriptingApi::Content::ScriptComponent::getExternalFile(var newValue)
{
	if (GET_PROJECT_HANDLER(getScriptProcessor()).isActive())
	{
		return GET_PROJECT_HANDLER(getScriptProcessor()).getFilePath(newValue, ProjectHandler::SubDirectories::Images);
	}
	else
	{
		return dynamic_cast<ScriptProcessor*>(getScriptProcessor())->getFile(newValue, PresetPlayerHandler::ImageResources);
	}
}

ScriptingApi::Content::ScriptComponent::ScriptComponent(ScriptBaseProcessor *base, Content *parentContent, Identifier name_, int x, int y, int width, int height) :
CreatableScriptObject(base),
name(name_),
value(0.0),
parent(parentContent),
skipRestoring(false),
componentProperties(new DynamicObject()),
changed(false)
{
	propertyIds.add(Identifier("text"));
	propertyIds.add(Identifier("visible"));		ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
	propertyIds.add(Identifier("enabled"));		ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
	propertyIds.add(Identifier("x"));			ADD_AS_SLIDER_TYPE(0, 900, 1);
	propertyIds.add(Identifier("y"));			ADD_AS_SLIDER_TYPE(0, MAX_SCRIPT_HEIGHT, 1);
	propertyIds.add(Identifier("width"));		ADD_AS_SLIDER_TYPE(0, 900, 1);
	propertyIds.add(Identifier("height"));		ADD_AS_SLIDER_TYPE(0, MAX_SCRIPT_HEIGHT, 1);
	propertyIds.add(Identifier("min"));
	propertyIds.add(Identifier("max"));
	propertyIds.add(Identifier("tooltip"));
	propertyIds.add(Identifier("bgColour"));	ADD_TO_TYPE_SELECTOR(SelectorTypes::ColourPickerSelector);
	propertyIds.add(Identifier("itemColour"));	ADD_TO_TYPE_SELECTOR(SelectorTypes::ColourPickerSelector);
	propertyIds.add(Identifier("itemColour2")); ADD_TO_TYPE_SELECTOR(SelectorTypes::ColourPickerSelector);
	propertyIds.add(Identifier("textColour"));  ADD_TO_TYPE_SELECTOR(SelectorTypes::ColourPickerSelector);
	propertyIds.add(Identifier("macroControl")); ADD_TO_TYPE_SELECTOR(SelectorTypes::ChoiceSelector);
	propertyIds.add(Identifier("zOrder"));
	propertyIds.add(Identifier("saveInPreset")); ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
	propertyIds.add(Identifier("isPluginParameter")); ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);

	deactivatedProperties.add(getIdFor(isPluginParameter));

	setDefaultValue(Properties::text, name.toString());
	setDefaultValue(Properties::visible, true);
	setDefaultValue(Properties::enabled, true);
	setDefaultValue(Properties::x, x);
	setDefaultValue(Properties::y, y);
	setDefaultValue(Properties::width, width);
	setDefaultValue(Properties::height, height);
	setDefaultValue(Properties::min, 0.0);
	setDefaultValue(Properties::max, 1.0);
	setDefaultValue(Properties::tooltip, "");
	setDefaultValue(Properties::bgColour, (int64)0x55FFFFFF);
	setDefaultValue(Properties::itemColour, (int64)0x66333333);
	setDefaultValue(Properties::itemColour2, (int64)0xfb111111);
	setDefaultValue(Properties::textColour, (int64)0xFFFFFFFF);
	setDefaultValue(Properties::macroControl, -1);
	setDefaultValue(Properties::zOrder, "Normal order");
	setDefaultValue(Properties::saveInPreset, true);
	setDefaultValue(Properties::isPluginParameter, false);

	setMethod("set", Wrapper::set);
	setMethod("get", Wrapper::get);
	setMethod("getValue", Wrapper::getValue);
	setMethod("setValue", Wrapper::setValue);
	setMethod("setValueNormalized", Wrapper::setValueNormalized);
	setMethod("getValueNormalized", Wrapper::getValueNormalized);
	setMethod("setColour", Wrapper::setColour);
	setMethod("setPosition", Wrapper::setPosition);
	setMethod("setTooltip", Wrapper::setTooltip);
	setMethod("showControl", Wrapper::showControl);
	setMethod("addToMacroControl", Wrapper::addToMacroControl);

	setName(name_.toString());

	SEND_MESSAGE(this);
}

StringArray ScriptingApi::Content::ScriptComponent::getOptionsFor(const Identifier &id)
{
	if (id == getIdFor(macroControl))
	{
		StringArray sa;

		sa.add("No MacroControl");
		for (int i = 0; i < 8; i++)
		{
			sa.add("Macro " + String(i + 1));
		}

		return sa;
	}
	else if (id == getIdFor(zOrder))
	{
		StringArray sa;

		sa.add("Normal order");
		sa.add("Always on top");

		return sa;
	}

	return StringArray();
}

ValueTree ScriptingApi::Content::ScriptComponent::exportAsValueTree() const
{
	ValueTree v(name);

	v.setProperty("value", value, nullptr);
	v.setProperty("visible", visible, nullptr);

	return v;
}

void ScriptingApi::Content::ScriptComponent::doubleClickCallback(Component *componentToNotify)
{
#if USE_BACKEND
	getScriptProcessor()->getMainController()->setEditedScriptComponent(this, componentToNotify);
#else
	ignoreUnused(componentToNotify);
#endif
}

void ScriptingApi::Content::ScriptComponent::setScriptObjectPropertyWithChangeMessage(const Identifier &id, var newValue, NotificationType notifyEditor)
{
	jassert(propertyIds.contains(id));

	componentProperties->setProperty(id, newValue);

	if(id == getIdFor(macroControl))
	{
		StringArray sa = getOptionsFor(id);

		const int index = sa.indexOf(newValue.toString())-1;

		if(index >= -1) addToMacroControl(index);
	}

	if(notifyEditor == sendNotification)
	{
		SEND_MESSAGE(this);
	}
}

const var ScriptingApi::Content::ScriptComponent::getScriptObjectProperty(int p) const
{
	jassert(componentProperties.get() != nullptr);

	jassert(componentProperties->hasProperty(getIdFor(p)));

	return componentProperties->getProperty(getIdFor(p));
}

String ScriptingApi::Content::ScriptComponent::getScriptObjectPropertiesAsJSON() const
{
	DynamicObject::Ptr clone = componentProperties->clone();

	for (int i = 0; i < deactivatedProperties.size(); i++)
	{
		clone->removeProperty(deactivatedProperties[i]);
	}

	for (int i = 0; i < defaultValues.size(); i++)
	{
		if (clone->getProperty(getIdFor(i)) == defaultValues[getIdFor(i)])
		{
			clone->removeProperty(getIdFor(i));
		}
	}

	var string = var(clone);

	return JSON::toString(string, false);
}

Rectangle<int> ScriptingApi::Content::ScriptComponent::getPosition() const
{
	const int x = getScriptObjectProperty(Properties::x);
	const int y = getScriptObjectProperty(Properties::y);
	const int w = getScriptObjectProperty(Properties::width);
	const int h = getScriptObjectProperty(Properties::height);

	return Rectangle<int>(x, y, w, h);
}

void ScriptingApi::Content::ScriptComponent::set(String propertyName, var value)
{
	Identifier name = Identifier(propertyName);

	if (!componentProperties->hasProperty(name))
	{
		reportScriptError("the property does not exist");
		return;
	}

	setScriptObjectPropertyWithChangeMessage(name, value, parent->allowGuiCreation ? dontSendNotification : sendNotification);
}

ScriptingApi::Content::ScriptSlider::ScriptSlider(ScriptBaseProcessor *base, Content *parentContent, Identifier name_, int x, int y, int, int) :
ScriptComponent(base, parentContent, name_, x, y, 128, 48),
styleId(Slider::SliderStyle::RotaryHorizontalVerticalDrag),
m(HiSlider::Mode::Linear),
image(nullptr),
minimum(0.0f),
maximum(1.0f)
{
	CHECK_COPY_AND_RETURN_22(base);

	propertyIds.add(Identifier("mode"));			ADD_TO_TYPE_SELECTOR(SelectorTypes::ChoiceSelector);
	propertyIds.add(Identifier("style"));			ADD_TO_TYPE_SELECTOR(SelectorTypes::ChoiceSelector);
	propertyIds.add(Identifier("stepSize"));		
	propertyIds.add(Identifier("middlePosition"));
	propertyIds.add(Identifier("defaultValue"));
	propertyIds.add(Identifier("suffix"));
	propertyIds.add(Identifier("filmstripImage"));	ADD_TO_TYPE_SELECTOR(SelectorTypes::FileSelector);
	propertyIds.add(Identifier("numStrips"));
	propertyIds.add(Identifier("isVertical"));		ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
    propertyIds.add(Identifier("scaleFactor"));
	
	deactivatedProperties.removeAllInstancesOf(getIdFor(isPluginParameter));

	componentProperties->setProperty(getIdFor(Mode), 0);
	componentProperties->setProperty(getIdFor(Style), 0);
	componentProperties->setProperty(getIdFor(stepSize), 0);
	componentProperties->setProperty(getIdFor(middlePosition), 0);
	componentProperties->setProperty(getIdFor(defaultValue), 0);
	componentProperties->setProperty(getIdFor(suffix), 0);
	componentProperties->setProperty(getIdFor(filmstripImage), String::empty);
	componentProperties->setProperty(getIdFor(numStrips), 0);
	componentProperties->setProperty(getIdFor(isVertical), true);

	priorityProperties.add(getIdFor(Mode));

	setDefaultValue(ScriptSlider::Properties::Mode, "Linear");
	setDefaultValue(ScriptSlider::Properties::Style, "Knob");
	setDefaultValue(ScriptSlider::Properties::middlePosition, -1.0);
	setDefaultValue(ScriptSlider::Properties::defaultValue, 0.0);
	setDefaultValue(ScriptSlider::Properties::stepSize, 0.01);
	setDefaultValue(ScriptComponent::min, 0.0);
	setDefaultValue(ScriptComponent::max, 1.0);
	setDefaultValue(ScriptSlider::Properties::suffix, "");
	setDefaultValue(ScriptSlider::Properties::filmstripImage, "Use default skin");
	setDefaultValue(ScriptSlider::Properties::numStrips, 0);
	setDefaultValue(ScriptSlider::Properties::isVertical, true);
    setDefaultValue(ScriptSlider::Properties::scaleFactor, 1.0f);

	setScriptObjectPropertyWithChangeMessage(getIdFor(Mode), "Linear", dontSendNotification);
	setScriptObjectPropertyWithChangeMessage(getIdFor(Style), "Knob", dontSendNotification);
	setScriptObjectPropertyWithChangeMessage(getIdFor(ScriptSlider::Properties::middlePosition), -1.0);
	setScriptObjectPropertyWithChangeMessage(getIdFor(stepSize), 0.01, dontSendNotification);
	setScriptObjectPropertyWithChangeMessage(getIdFor(ScriptComponent::min), 0.0, dontSendNotification);
	setScriptObjectPropertyWithChangeMessage(getIdFor(ScriptComponent::max), 1.0, dontSendNotification);
	setScriptObjectPropertyWithChangeMessage(getIdFor(suffix), "", dontSendNotification);
	setScriptObjectPropertyWithChangeMessage(getIdFor(filmstripImage), "Use default skin", dontSendNotification);

	setMethod("setMidPoint", Wrapper::setMidPoint);
	setMethod("setRange", Wrapper::setRange);
	setMethod("setMode", Wrapper::setMode);
	setMethod("setStyle", Wrapper::setStyle);
	setMethod("setMinValue", Wrapper::setMinValue);
	setMethod("setMaxValue", Wrapper::setMaxValue);
	setMethod("getMinValue", Wrapper::getMinValue);
	setMethod("getMaxValue", Wrapper::getMaxValue);
	setMethod("contains", Wrapper::contains);

	setProperty("Decibel", HiSlider::Mode::Decibel);
	setProperty("Discrete", HiSlider::Mode::Discrete);
	setProperty("Frequency", HiSlider::Mode::Frequency);
}

ScriptingApi::Content::ScriptSlider::~ScriptSlider()
{
	if (getScriptProcessor() != nullptr)
	{
		ImagePool *pool = getScriptProcessor()->getMainController()->getSampleManager().getImagePool();

		pool->releasePoolData(image);
	}
}

ScriptCreatedComponentWrapper * ScriptingApi::Content::ScriptSlider::createComponentWrapper(ScriptContentComponent *content, int index)
{
	return new ScriptCreatedComponentWrappers::SliderWrapper(content, this, index);
}

void ScriptingApi::Content::ScriptSlider::setScriptObjectPropertyWithChangeMessage(const Identifier &id, var newValue, NotificationType notifyEditor)
{
	jassert(propertyIds.contains(id));

	if(id == Identifier("mode"))
	{
		setMode(newValue.toString());
	}
	else if(id == propertyIds[Style])
	{
		setStyle(newValue);
	}
	else if(id == getIdFor(middlePosition))
	{
		setMidPoint(newValue);
		if(notifyEditor==sendNotification) parent->sendChangeMessage(); // skip the rest
		return;
	}
	else if (id == getIdFor(filmstripImage))
	{
		ImagePool *pool = getScriptProcessor()->getMainController()->getSampleManager().getImagePool();

		pool->releasePoolData(image);

		if (newValue == "Use default skin" || newValue == "")
		{
			setScriptObjectProperty(filmstripImage, "Use default skin");
			image = nullptr;
		}
		else
		{
			setScriptObjectProperty(filmstripImage, newValue);


#if USE_FRONTEND

			String poolName = ProjectHandler::Frontend::getSanitiziedFileNameForPoolReference(newValue);

			image = pool->loadFileIntoPool(poolName, false);

			jassert(image != nullptr);

#else


			File actualFile = getExternalFile(newValue);

			image = pool->loadFileIntoPool(actualFile.getFullPathName(), false);

#endif
		}		
	}

	ScriptComponent::setScriptObjectPropertyWithChangeMessage(id, newValue, notifyEditor);
}

StringArray ScriptingApi::Content::ScriptSlider::getOptionsFor(const Identifier &id)
{
	const int index = propertyIds.indexOf(id);

	StringArray sa;

	switch (index)
	{
	case Properties::Mode:	
		sa.add("Frequency");
		sa.add("Decibel");
		sa.add("Time");
		sa.add("TempoSync");
		sa.add("Linear");
		sa.add("Discrete");
		sa.add("Pan");
		sa.add("NormalizedPercentage");
		break;
	case Properties::Style:	sa.add("Knob");
		sa.add("Horizontal");
		sa.add("Vertical");
		sa.add("Range");
		break;
	case filmstripImage:
		sa.add("Load new File");
		sa.add("Use default skin");
		sa.addArray(getScriptProcessor()->getMainController()->getSampleManager().getImagePool()->getFileNameList());
		break;
	default:				sa = ScriptComponent::getOptionsFor(id);
	}

	return sa;
}

void ScriptingApi::Content::ScriptSlider::setMidPoint(double valueForMidPoint)
{
	if (valueForMidPoint == -1.0f)
	{
		setScriptObjectProperty(middlePosition, valueForMidPoint);
		return;
		//valueForMidPoint = range.getStart() + (range.getEnd() - range.getStart()) / 2.0;
	}

	Range<double> range = Range<double>(getScriptObjectProperty(ScriptComponent::Properties::min), getScriptObjectProperty(ScriptComponent::Properties::max));

	const bool illegalMidPoint = !range.contains(valueForMidPoint);
	if(illegalMidPoint)
	{
		reportScriptError("setMidPoint() value must be in the knob range.");
		valueForMidPoint = (range.getEnd() - range.getStart()) / 2.0 + range.getStart();
	}

	CHECK_COPY_AND_RETURN_11(getScriptProcessor());

	setScriptObjectProperty(middlePosition, valueForMidPoint);	
}

void ScriptingApi::Content::ScriptSlider::setStyle(String style)
{
	if (style == "Knob") styleId = Slider::SliderStyle::RotaryHorizontalVerticalDrag;
	else if (style == "Horizontal") styleId = Slider::SliderStyle::LinearBar;
	else if (style == "Vertical") styleId = Slider::SliderStyle::LinearBarVertical;
	else if (style == "Range") styleId = Slider::SliderStyle::TwoValueHorizontal;
}

void ScriptingApi::Content::ScriptSlider::setMinValue(double min) noexcept
{
	if (styleId == Slider::TwoValueHorizontal)
	{
		minimum = min;
		sendChangeMessage();
	}
	else
	{
		reportScriptError("setMinValue() can only be called on sliders in 'Range' mode.");
	}
}

void ScriptingApi::Content::ScriptSlider::setMaxValue(double max) noexcept
{
	if (styleId == Slider::TwoValueHorizontal)
	{
		maximum = max;
		sendChangeMessage();
	}
	else
	{
		reportScriptError("setMaxValue() can only be called on sliders in 'Range' mode.");
	}
}

double ScriptingApi::Content::ScriptSlider::getMinValue() const
{

	if (styleId == Slider::TwoValueHorizontal)
	{
		return minimum;
	}
	else
	{
		reportScriptError("getMinValue() can only be called on sliders in 'Range' mode.");
		return 0.0;
	}
}

double ScriptingApi::Content::ScriptSlider::getMaxValue() const
{
	if (styleId == Slider::TwoValueHorizontal)
	{
		return maximum;
	}
	else
	{
		reportScriptError("getMaxValue() can only be called on sliders in 'Range' mode.");
		return 0.0;
	}
}

bool ScriptingApi::Content::ScriptSlider::contains(double value)
{
	if (styleId == Slider::TwoValueHorizontal)
	{
		return minimum <= value && maximum >= value;
	}
	else
	{
		reportScriptError("contains() can only be called on sliders in 'Range' mode.");
		return false;
	}
}

ScriptingApi::Content::ScriptLabel::ScriptLabel(ScriptBaseProcessor *base, Content *parentContent, Identifier name, int x, int y, int width, int) :
ScriptComponent(base, parentContent, name, x, y, width, 16)
{
	propertyIds.add(Identifier("fontName"));	ADD_TO_TYPE_SELECTOR(SelectorTypes::ChoiceSelector);
	propertyIds.add(Identifier("fontSize"));	ADD_AS_SLIDER_TYPE(1, 200, 1);
	propertyIds.add(Identifier("fontStyle"));	ADD_TO_TYPE_SELECTOR(SelectorTypes::ChoiceSelector);
	propertyIds.add(Identifier("alignment"));	ADD_TO_TYPE_SELECTOR(SelectorTypes::ChoiceSelector);
	propertyIds.add(Identifier("editable"));	ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
	propertyIds.add(Identifier("multiline"));	ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);


	componentProperties->setProperty(getIdFor(FontName), 0);
	componentProperties->setProperty(getIdFor(FontSize), 0);
	componentProperties->setProperty(getIdFor(FontStyle), 0);
	componentProperties->setProperty(getIdFor(Alignment), 0);
	componentProperties->setProperty(getIdFor(Editable), 0);
	componentProperties->setProperty(getIdFor(Multiline), 0);

	setDefaultValue(bgColour, (int64)0x00000000);
	setDefaultValue(itemColour, (int64)0x00000000);
	setDefaultValue(textColour, (int64)0xffffffff);
	setDefaultValue(FontStyle, "plain");
	setDefaultValue(FontSize, 13.0f);
	setDefaultValue(FontName, "Arial");
	setDefaultValue(Editable, true);
	setDefaultValue(Multiline, false);

	deactivatedProperties.add(getIdFor(ScriptComponent::Properties::min));
	deactivatedProperties.add(getIdFor(ScriptComponent::Properties::max));

	setMethod("setEditable", Wrapper::setEditable);
}

ScriptCreatedComponentWrapper * ScriptingApi::Content::ScriptLabel::createComponentWrapper(ScriptContentComponent *content, int index)
{
	return new ScriptCreatedComponentWrappers::LabelWrapper(content, this, index);
}

void ScriptingApi::Content::ScriptLabel::setEditable(bool shouldBeEditable)
{
	if( ! parent->allowGuiCreation)
	{
		reportScriptError("the editable state of a label can't be changed after onInit()");
		return;
	}

	setScriptObjectProperty(Editable, shouldBeEditable);
}

void ScriptingApi::Content::setHeight(int newHeight) noexcept
{
	if( ! allowGuiCreation)
	{
		reportScriptError("the height can't be changed after onInit()");
		return;
	}

	if(newHeight > 800)
	{
		reportScriptError("Go easy on the height! (" + String(800) + "px is enough)");
		return;
	}

	height = newHeight;
};

void ScriptingApi::Content::setWidth(int newWidth) noexcept
{
	if( ! allowGuiCreation)
	{
		reportScriptError("the width can't be changed after onInit()");
		return;
	}

	if(newWidth > 1280)
	{
		reportScriptError("Go easy on the width! (1280px is enough)");
		return;
	}

	width = newWidth;
};


void ScriptingApi::Content::setToolbarProperties(const var &toolbarProperties)
{
	NamedValueSet *newSet = &toolbarProperties.getDynamicObject()->getProperties();

	NamedValueSet *set = &getScriptProcessor()->getMainController()->getToolbarPropertiesObject()->getProperties();

	set->clear();

	for (int i = 0; i < newSet->size(); i++)
	{
		set->set(newSet->getName(i), newSet->getValueAt(i));
	}
}

void ScriptingApi::Content::storeAllControlsAsPreset(const String &fileName)
{
	File f;

	if (File::isAbsolutePath(fileName))
	{
		f = File(fileName);
	}
	else
	{
		f = File(GET_PROJECT_HANDLER(getScriptProcessor()).getSubDirectory(ProjectHandler::SubDirectories::UserPresets).getChildFile(fileName));
	}

	ValueTree v = exportAsValueTree();

	v.setProperty("Processor", getScriptProcessor()->getId(), nullptr);

	if (f.existsAsFile())
	{
		ScopedPointer<XmlElement> existingData = XmlDocument::parse(f);

		ValueTree preset = ValueTree::fromXml(*existingData);

		bool found = false;

		for (int i = 0; i < preset.getNumChildren(); i++)
		{
			if (preset.getChild(i).getProperty("Processor") == getScriptProcessor()->getId())
			{
				preset.getChild(i).copyPropertiesFrom(v, nullptr);
				found = true;
				break;
			}
		}

		if (!found) preset.addChild(v, -1, nullptr);

		existingData = preset.createXml();

		f.replaceWithText(existingData->createDocument(""));
	}
	else
	{
		ValueTree preset = ValueTree("Preset");

		preset.addChild(v, -1, nullptr);

		ScopedPointer<XmlElement> xml = preset.createXml();

		f.replaceWithText(xml->createDocument(""));
	}
}

void ScriptingApi::Content::restoreAllControlsFromPreset(const String &fileName)
{
#if USE_FRONTEND
    
	CHECK_COPY_AND_RETURN_23(getScriptProcessor());

    const ValueTree parent = dynamic_cast<FrontendProcessor*>(getScriptProcessor()->getMainController())->getPresetData();
    
    ValueTree v;
        
    for (int i = 0; i < parent.getNumChildren(); i++)
    {
        ValueTree preset = parent.getChild(i);
        
        if(preset.getProperty("FileName") != fileName) continue;
        
        for(int j = 0; j < preset.getNumChildren(); j++)
        {
            if (preset.getChild(j).getProperty("Processor") == getScriptProcessor()->getId())
            {
                v = preset.getChild(j);
                break;
            }
        }
    }
    
    if (!v.isValid())
    {
        reportScriptError("Preset ID not found");
    }
        
    restoreAllControlsFromPreset(v);
    
#else
    
	File f;

	if (File::isAbsolutePath(fileName))
	{
		f = File(fileName);
	}
	else
	{
		f = File(GET_PROJECT_HANDLER(getScriptProcessor()).getSubDirectory(ProjectHandler::SubDirectories::UserPresets).getChildFile(fileName));
	}

	if (f.existsAsFile())
	{
		ScopedPointer<XmlElement> xml = XmlDocument::parse(f);

		ValueTree parent = ValueTree::fromXml(*xml);

		ValueTree v;

		for (int i = 0; i < parent.getNumChildren(); i++)
		{
			if (parent.getChild(i).getProperty("Processor") == getScriptProcessor()->getId())
			{
				v = parent.getChild(i);
				break;
			}
		}

		if (!v.isValid())
		{
			reportScriptError("Preset ID not found");
		}

		restoreAllControlsFromPreset(v);

		
	}
	else
	{
		reportScriptError("File not found");
	}
    
#endif
}

void ScriptingApi::Content::restoreAllControlsFromPreset(const ValueTree &preset)
{
	restoreFromValueTree(preset);

	StringArray macroNames;

	if (components.size() != 0)
	{
		macroNames = components[0]->getOptionsFor(components[0]->getIdFor(ScriptComponent::macroControl));
	}

	for (int i = 0; i < components.size(); i++)
	{
		if (!components[i]->getScriptObjectProperty(ScriptComponent::Properties::saveInPreset)) continue;

		getScriptProcessor()->setAttribute(i, components[i]->getValue(), sendNotification);

		const String macroName = components[i]->getScriptObjectProperty(ScriptComponent::macroControl).toString();

		const int macroIndex = macroNames.indexOf(macroName) - 1;

		if (macroIndex >= 0)
		{
			NormalisableRange<float> range(components[i]->getScriptObjectProperty(ScriptComponent::min), components[i]->getScriptObjectProperty(ScriptComponent::max));

			getScriptProcessor()->getMainController()->getMacroManager().getMacroChain()->setMacroControl(macroIndex, range.convertTo0to1(components[i]->getValue()) * 127.0f, sendNotification);
		}
	}
}

void ScriptingApi::Content::ScriptComponent::setValue(var controlValue)
{	
	if(parent != nullptr)
	{
		ScopedLock sl(parent->lock);
		value = controlValue;
	}

	if(parent->allowGuiCreation)
	{
		skipRestoring = true;
	}

	sendChangeMessage();
};

void ScriptingApi::Content::ScriptComponent::setColour(int colourId, int colourAs32bitHex)
{
	switch (colourId)
	{
	case 0: setScriptObjectProperty(bgColour, (int64)colourAs32bitHex); break;
	case 1:	setScriptObjectProperty(itemColour, (int64)colourAs32bitHex); break;
	case 2:	setScriptObjectProperty(itemColour2, (int64)colourAs32bitHex); break;
	case 3:	setScriptObjectProperty(textColour, (int64)colourAs32bitHex); break;
	}

	sendChangeMessage();
}

void ScriptingApi::Content::ScriptComponent::setPropertiesFromJSON(const var &jsonData)
{
	if (!jsonData.isUndefined() && jsonData.isObject())
	{
		NamedValueSet dataSet = jsonData.getDynamicObject()->getProperties();

		for (int i = 0; i < priorityProperties.size(); i++)
		{
			if (dataSet.contains(priorityProperties[i]))
			{
				setScriptObjectPropertyWithChangeMessage(priorityProperties[i], dataSet[priorityProperties[i]], dontSendNotification);
			}
		}

		for (int i = 0; i < dataSet.size(); i++)
		{
			Identifier name = dataSet.getName(i);

			if (priorityProperties.contains(name)) continue;

			setScriptObjectPropertyWithChangeMessage(name, dataSet.getValueAt(i), dontSendNotification);
		}
	}

	SEND_MESSAGE(this);
}

void ScriptingApi::Content::ScriptComponent::setPosition(int x, int y, int w, int h)
{
	componentProperties->setProperty("x", x);
	componentProperties->setProperty("y", y);
	componentProperties->setProperty("width", w);
	componentProperties->setProperty("height", h);

	sendChangeMessage();
}

ScriptingApi::Content::ScriptComboBox::ScriptComboBox(ScriptBaseProcessor *base, Content *parentContent, Identifier name, int x, int y, int width, int) :
ScriptComponent(base, parentContent, name, x, y, width, 32)
{
	propertyIds.add(Identifier("items"));	ADD_TO_TYPE_SELECTOR(SelectorTypes::MultilineSelector);
	propertyIds.add(Identifier("isPluginParameter")); ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
    
	deactivatedProperties.add(getIdFor(ScriptComponent::Properties::height));
	deactivatedProperties.add(getIdFor(ScriptComponent::Properties::max));
	deactivatedProperties.add(getIdFor(ScriptComponent::Properties::min));
	deactivatedProperties.add(getIdFor(ScriptComponent::Properties::textColour));

	deactivatedProperties.removeAllInstancesOf(getIdFor(ScriptComponent::Properties::isPluginParameter));

	priorityProperties.add(getIdFor(Items));

	componentProperties->setProperty(getIdFor(Items), 0);

	setDefaultValue(Items, "");
	setDefaultValue(ScriptComponent::min, 1.0f);
	setDefaultValue(isPluginParameter, false);

	setMethod("addItem", Wrapper::addItem);
	setMethod("getItemText", Wrapper::getItemText);
}

ScriptCreatedComponentWrapper * ScriptingApi::Content::ScriptComboBox::createComponentWrapper(ScriptContentComponent *content, int index)
{
	return new ScriptCreatedComponentWrappers::ComboBoxWrapper(content, this, index);
}

String ScriptingApi::Content::ScriptComboBox::getItemText() const
{
	StringArray items = getItemList();

	jassert((int)value <= items.size());

	return items[(int)value - 1];
}

ScriptingApi::Content::ScriptTable::ScriptTable(ScriptBaseProcessor *base, Content *parentContent, Identifier name, int x, int y, int width, int height) :
ScriptComponent(base, parentContent, name, x, y, width, height),
ownedTable(new MidiTable()),
useOtherTable(false),
connectedProcessor(nullptr),
lookupTableIndex(-1)
{
	deactivatedProperties.add(getIdFor(ScriptComponent::Properties::bgColour));
	deactivatedProperties.add(getIdFor(ScriptComponent::Properties::itemColour));
	deactivatedProperties.add(getIdFor(ScriptComponent::Properties::itemColour2));
	deactivatedProperties.add(getIdFor(ScriptComponent::Properties::max));
	deactivatedProperties.add(getIdFor(ScriptComponent::Properties::min));
	deactivatedProperties.add(getIdFor(ScriptComponent::Properties::textColour));
	deactivatedProperties.add(getIdFor(ScriptComponent::Properties::macroControl));

	propertyIds.add("tableIndex");
	propertyIds.add("processorId"); ADD_TO_TYPE_SELECTOR(SelectorTypes::ChoiceSelector);

	componentProperties->setProperty(getIdFor(ProcessorId), 0);
	componentProperties->setProperty(getIdFor(TableIndex), 0);

	setDefaultValue(ScriptTable::Properties::ProcessorId, "");
	setDefaultValue(ScriptTable::Properties::TableIndex, 0);

	setMethod("getTableValue", Wrapper::getTableValue);
	setMethod("connectToOtherTable", Wrapper::connectToOtherTable);
}

ScriptCreatedComponentWrapper * ScriptingApi::Content::ScriptTable::createComponentWrapper(ScriptContentComponent *content, int index)
{
	return new ScriptCreatedComponentWrappers::TableWrapper(content, this, index);
}

float ScriptingApi::Content::ScriptTable::getTableValue(int inputValue)
{
	value = inputValue;

	parent->sendChangeMessage();

	if(useOtherTable)
	{
		if(MidiTable *mt = dynamic_cast<MidiTable*>(referencedTable.get()))
		{
			return mt->get(inputValue);
		}
		else if (SampleLookupTable *st = dynamic_cast<SampleLookupTable*>(referencedTable.get()))
		{
			return st->getInterpolatedValue(inputValue);
		}
		else
		{
			reportScriptError("Connected Table was not found!");
			return -1.0f;
		}
	}
	else
	{
		return ownedTable->get(inputValue);
	}
}

StringArray ScriptingApi::Content::ScriptTable::getOptionsFor(const Identifier &id)
{
	if(id != getIdFor(ProcessorId)) return ScriptComponent::getOptionsFor(id);

	return ProcessorHelpers::getAllIdsForType<LookupTableProcessor>(getScriptProcessor()->getOwnerSynth());
};

void ScriptingApi::Content::ScriptTable::setScriptObjectPropertyWithChangeMessage(const Identifier &id, var newValue, NotificationType notifyEditor)
{
	if(getIdFor(ProcessorId) == id)
	{
		const int tableIndex = getScriptObjectProperty(TableIndex);

		connectToOtherTable(newValue, tableIndex);
	}
	else if (getIdFor(TableIndex) == id)
	{
		connectToOtherTable(getScriptObjectProperty(ProcessorId), newValue);
	}

	ScriptComponent::setScriptObjectPropertyWithChangeMessage(id, newValue, notifyEditor);
}

void ScriptingApi::Content::ScriptTable::connectToOtherTable(const String &otherTableId, int index)
{
	if (otherTableId.isEmpty()) return;

	Processor::Iterator<Processor> it(getScriptProcessor()->getOwnerSynth(), false);

	Processor *p;

	while((p = it.getNextProcessor()) != nullptr)
	{
		if(dynamic_cast<LookupTableProcessor*>(p) != nullptr && p->getId() == otherTableId)
		{
			useOtherTable = true;

			debugToConsole(getScriptProcessor(), otherTableId + " was found.");

			referencedTable = dynamic_cast<LookupTableProcessor*>(p)->getTable(index);
			connectedProcessor = p;

			return;
		}

	}

	useOtherTable = false;
	referencedTable = nullptr;

}

LookupTableProcessor * ScriptingApi::Content::ScriptTable::getTableProcessor() const
{
	return dynamic_cast<LookupTableProcessor*>(connectedProcessor.get());
}

ScriptingApi::Content::ScriptAudioWaveform::ScriptAudioWaveform(ScriptBaseProcessor *base, Content *parentContent, Identifier plotterName, int x, int y, int width, int height) :
ScriptComponent(base, parentContent, plotterName, x, y, width, height),
connectedProcessor(nullptr)
{
	propertyIds.add("processorId");	ADD_TO_TYPE_SELECTOR(SelectorTypes::ChoiceSelector);

	deactivatedProperties.add(getIdFor(text));
	deactivatedProperties.add(getIdFor(min));
	deactivatedProperties.add(getIdFor(max));
	deactivatedProperties.add(getIdFor(bgColour));
	deactivatedProperties.add(getIdFor(itemColour));
	deactivatedProperties.add(getIdFor(itemColour2));
	deactivatedProperties.add(getIdFor(textColour));
	deactivatedProperties.add(getIdFor(macroControl));

	componentProperties->setProperty(getIdFor(processorId), "");

	setMethod("connectToAudioSampleProcessor", Wrapper::connectToAudioSampleProcessor);
}

ScriptCreatedComponentWrapper * ScriptingApi::Content::ScriptAudioWaveform::createComponentWrapper(ScriptContentComponent *content, int index)
{
	return new ScriptCreatedComponentWrappers::AudioWaveformWrapper(content, this, index);
}

void ScriptingApi::Content::ScriptAudioWaveform::connectToAudioSampleProcessor(String processorId)
{
	Processor::Iterator<Processor> it(getScriptProcessor()->getOwnerSynth(), false);

	Processor *p;

	while ((p = it.getNextProcessor()) != nullptr)
	{
		if (dynamic_cast<AudioSampleProcessor*>(p) != nullptr && p->getId() == processorId)
		{
			debugToConsole(getScriptProcessor(), processorId + " was found.");

			connectedProcessor = p;

			return;
		}

	}

	connectedProcessor = nullptr;
}

void ScriptingApi::Content::ScriptAudioWaveform::setScriptObjectPropertyWithChangeMessage(const Identifier &id, var newValue, NotificationType notifyEditor /*= sendNotification*/)
{
	if (id == getIdFor(processorId))
	{
		connectToAudioSampleProcessor(newValue.toString());
	}

	ScriptComponent::setScriptObjectPropertyWithChangeMessage(id, newValue, notifyEditor);
}


ValueTree ScriptingApi::Content::ScriptAudioWaveform::exportAsValueTree() const
{
	ValueTree v = ScriptComponent::exportAsValueTree();

	const AudioSampleProcessor *processor = dynamic_cast<const AudioSampleProcessor*>(connectedProcessor.get());
	
	if (processor != nullptr)
	{
		v.setProperty("Processor", connectedProcessor->getId(), nullptr);

		v.setProperty("rangeStart", processor->getRange().getStart(), nullptr);
		v.setProperty("rangeEnd", processor->getRange().getEnd(), nullptr);
		v.setProperty("fileName", processor->getFileName(), nullptr);
	}

	return v;
}

void ScriptingApi::Content::ScriptAudioWaveform::restoreFromValueTree(const ValueTree &v)
{
	const String id = v.getProperty("Processor", "");



	if (id.isNotEmpty())
	{
		if (connectedProcessor.get() == nullptr || connectedProcessor.get()->getId() != id)
		{
			connectToAudioSampleProcessor(id);
		}

		const String fileName = v.getProperty("fileName", "");

		if (fileName.isNotEmpty())
		{
			getProcessor()->setLoadedFile(fileName, true, false);
			
			Range<int> range(v.getProperty("rangeStart"), v.getProperty("rangeEnd"));

			getProcessor()->setRange(range);

			sendChangeMessage();
		}
	}
}

StringArray ScriptingApi::Content::ScriptAudioWaveform::getOptionsFor(const Identifier &id)
{
	if (id != getIdFor(processorId)) return ScriptComponent::getOptionsFor(id);

	return ProcessorHelpers::getAllIdsForType<AudioSampleProcessor>(getScriptProcessor()->getOwnerSynth());
}

void ScriptingApi::Content::endInitialization()
{
	allowGuiCreation = false;
}

ValueTree ScriptingApi::Content::exportAsValueTree() const
{
	ValueTree v("Content");
			
	for(int i = 0; i < components.size(); i++)
	{
		if (!components[i]->getScriptObjectProperty(ScriptingApi::Content::ScriptComponent::Properties::saveInPreset)) continue;

		ValueTree child = components[i]->exportAsValueTree();
		
		v.addChild(child, -1, nullptr);
	}

	return v;
};

void ScriptingApi::Content::restoreFromValueTree(const ValueTree &v)
{
	jassert (v.getType().toString() == "Content");

	for(int i = 0; i < components.size(); i++)
	{
		if(components[i]->skipRestoring || !components[i]->getScriptObjectProperty(ScriptComponent::Properties::saveInPreset)) continue;

		ValueTree child = v.getChildWithName(components[i]->name);
		if(child.isValid())
		{
			components[i]->restoreFromValueTree(child);
		}
	}

};

bool ScriptingApi::Content::isEmpty()
{
	return components.size() == 0;
}


void ScriptingObjects::ScriptingModulator::doubleClickCallback(Component *componentToNotify) 
{
#if USE_BACKEND
	if (objectExists() && !objectDeleted())
	{
		BackendProcessorEditor *editor = componentToNotify->findParentComponentOfClass<BackendProcessorEditor>();

		Processor *p = ProcessorHelpers::getFirstProcessorWithName(editor->getMainSynthChain(), mod->getId());

		if (p != nullptr)
		{
			editor->setRootProcessorWithUndo(p);
		}
	}
#else 
	ignoreUnused(componentToNotify);
#endif
}

void ScriptingObjects::ScriptingModulator::setIntensity(float newIntensity)
{
	if(checkValidObject())
	{
		Modulation *m = dynamic_cast<Modulation*>(mod.get());

		if(m->getMode() == Modulation::GainMode)
		{
			const float value = jlimit<float>(0.0f, 1.0f, newIntensity);

			m->setIntensity(value);
		}
		else
		{
			const float value = jlimit<float>(-12.0f, 12.0f, newIntensity);

			const float pitchFactor = powf(2.0f, value / 12.0f);

			m->setIntensity(pitchFactor);

		}

		

		mod->sendChangeMessage();
	}
};


#pragma warning( push )
#pragma warning( disable : 4390)

void ScriptingApi::Content::ModulatorMeter::setScriptProcessor(ScriptBaseProcessor *sp)
{
	Processor::Iterator<Modulator> it(sp->getOwnerSynth(), true);

	String name = getScriptObjectProperty(ModulatorId);

	if(name.isEmpty()) return;

	Modulator *m;

	while((m = it.getNextProcessor()) != nullptr)
	{
		if(Identifier(m->getId()) == Identifier(name))
		{
			targetMod = m;
			break;
		}
	}

	if(m == nullptr) debugError(sp, "Modulator " + name + " not found!");
};

#pragma warning( pop )

StringArray ScriptingApi::Content::ModulatorMeter::getOptionsFor(const Identifier &id)
{
	if (id != getIdFor(ModulatorId)) return ScriptComponent::getOptionsFor(id);

	StringArray sa;

	Processor::Iterator<Modulator> iter(getScriptProcessor()->getOwnerSynth());

	Modulator *m;

	while((m = iter.getNextProcessor()) != nullptr)
	{
		sa.add(m->getId());
	}

	return sa;
};

ScriptingApi::Content::ModulatorMeter::ModulatorMeter(ScriptBaseProcessor *base, Content *parentContent, Identifier modulatorName, int x, int y, int width, int height) :
ScriptComponent(base, parentContent, modulatorName, x, y, width, height),
targetMod(nullptr)
{
	deactivatedProperties.add(getIdFor(ScriptComponent::Properties::max));
	deactivatedProperties.add(getIdFor(ScriptComponent::Properties::min));
	deactivatedProperties.add(getIdFor(ScriptComponent::Properties::textColour));
	deactivatedProperties.add(getIdFor(ScriptComponent::Properties::macroControl));

	propertyIds.add("modulatorId");	ADD_TO_TYPE_SELECTOR(SelectorTypes::ChoiceSelector);

	componentProperties->setProperty(getIdFor(ModulatorId), 0);

	setScriptObjectProperty(ModulatorId, "");
}

ScriptCreatedComponentWrapper * ScriptingApi::Content::ModulatorMeter::createComponentWrapper(ScriptContentComponent *content, int index)
{
	return new ScriptCreatedComponentWrappers::ModulatorMeterWrapper(content, this, index);
}

void ScriptingApi::Content::ModulatorMeter::setScriptObjectPropertyWithChangeMessage(const Identifier &id, var newValue, NotificationType notifyEditor)
{
	if(getIdFor(ModulatorId) == id)
	{
		setScriptProcessor(getScriptProcessor());
	}

	ScriptComponent::setScriptObjectPropertyWithChangeMessage(id, newValue, notifyEditor);
}



ScriptingObjects::ScriptingEffect::ScriptingEffect(ScriptBaseProcessor *p, EffectProcessor *fx) :
	CreatableScriptObject(p),
	effect(fx)
{
	if(fx != nullptr)
	{
		setName(fx->getId());

		for(int i = 0; i < fx->getNumParameters(); i++)
		{
			setProperty(fx->getIdentifierForParameterIndex(i), var(i));
		}
	}
	else
	{
		setName("Invalid Effect");
	}

	setMethod("setAttribute", Wrapper::setAttribute);
	setMethod("setBypassed", Wrapper::setBypassed);
};

ScriptingObjects::ScriptingSynth::ScriptingSynth(ScriptBaseProcessor *p, ModulatorSynth *synth_):
	CreatableScriptObject(p),
	synth(synth_)
{
	if(synth != nullptr)
	{
		setName(synth->getId());

		for(int i = 0; i < synth->getNumParameters(); i++)
		{
			setProperty(synth->getIdentifierForParameterIndex(i), var(i));
		}
	}
	else
	{
		setName("Invalid Effect");
	}

	setMethod("setAttribute", Wrapper::setAttribute);
	setMethod("setBypassed", Wrapper::setBypassed);
};

ScriptingObjects::ScriptingAudioSampleProcessor::ScriptingAudioSampleProcessor(ScriptBaseProcessor *p, AudioSampleProcessor *sampleProcessor):
CreatableScriptObject(p),
audioSampleProcessor(dynamic_cast<Processor*>(sampleProcessor))
{
	if (audioSampleProcessor != nullptr)
	{
		setName(audioSampleProcessor->getId());

		for (int i = 0; i < audioSampleProcessor->getNumParameters(); i++)
		{
			setProperty(audioSampleProcessor->getIdentifierForParameterIndex(i), var(i));
		}
	}
	else
	{
		setName("Invalid Processor");
	}

	setMethod("setAttribute", Wrapper::setAttribute);
	setMethod("setBypassed", Wrapper::setBypassed);
	setMethod("getSampleLength", Wrapper::getSampleLength);
    setMethod("setSampleRange", Wrapper::setSampleRange);
	setMethod("setFile", Wrapper::setFile);
}

void ScriptingObjects::ScriptingAudioSampleProcessor::setFile(String fileName)
{
	if (checkValidObject())
	{
		ScopedLock sl(audioSampleProcessor->getMainController()->getLock());
        
#if USE_FRONTEND
        const String nameInPool = fileName.fromFirstOccurrenceOf("}", false, false);
        
        dynamic_cast<AudioSampleProcessor*>(audioSampleProcessor.get())->setLoadedFile(nameInPool, true);
#else
		dynamic_cast<AudioSampleProcessor*>(audioSampleProcessor.get())->setLoadedFile(GET_PROJECT_HANDLER(dynamic_cast<Processor*>(audioSampleProcessor.get())).getFilePath(fileName, ProjectHandler::SubDirectories::AudioFiles), true);
#endif
	}
}

void ScriptingObjects::ScriptingAudioSampleProcessor::setSampleRange(int start, int end)
{
    if (checkValidObject())
    {
        ScopedLock sl(audioSampleProcessor->getMainController()->getLock());
        dynamic_cast<AudioSampleProcessor*>(audioSampleProcessor.get())->setRange(Range<int>(start, end));
        
    }
}

int ScriptingObjects::ScriptingAudioSampleProcessor::getSampleLength() const
{
	if (checkValidObject())
	{
        return dynamic_cast<const AudioSampleProcessor*>(audioSampleProcessor.get())->getTotalLength();	}
	else return 0;
}


ScriptingObjects::ScriptingTableProcessor::ScriptingTableProcessor(ScriptBaseProcessor *p, LookupTableProcessor *tableProcessor_):
  CreatableScriptObject(p),
  tableProcessor(dynamic_cast<Processor*>(tableProcessor_))
{
    if (tableProcessor != nullptr)
    {
        setName(tableProcessor->getId());
        
        for (int i = 0; i < tableProcessor->getNumParameters(); i++)
        {
            setProperty(tableProcessor->getIdentifierForParameterIndex(i), var(i));
        }
    }
    else
    {
        setName("Invalid Processor");
    }
    
    setMethod("addTablePoint", Wrapper::addTablePoint);
    setMethod("reset", Wrapper::reset);
    setMethod("setTablePoint", Wrapper::setTablePoint);
}
    
    
    
void ScriptingObjects::ScriptingTableProcessor::setTablePoint(int tableIndex, int pointIndex, float x, float y, float curve)
{
    if(tableProcessor != nullptr)
    {
        Table *table = dynamic_cast<LookupTableProcessor*>(tableProcessor.get())->getTable(tableIndex);
        
        if(table != nullptr)
        {
			table->setTablePoint(pointIndex, x, y, curve);
			table->sendChangeMessage();
        }
    }
}


void ScriptingObjects::ScriptingTableProcessor::addTablePoint(int tableIndex, float x, float y)
{
	if (tableProcessor != nullptr)
	{
		Table *table = dynamic_cast<LookupTableProcessor*>(tableProcessor.get())->getTable(tableIndex);

		if (table != nullptr)
		{
			table->addTablePoint(x, y);
			table->sendChangeMessage();
		}
	}
}


void ScriptingObjects::ScriptingTableProcessor::reset(int tableIndex)
{
	if (tableProcessor != nullptr)
	{
		Table *table = dynamic_cast<LookupTableProcessor*>(tableProcessor.get())->getTable(tableIndex);

		if (table != nullptr)
		{
			table->reset();
			table->sendChangeMessage();
		}
	}
}

ScriptingApi::Content::ScriptSliderPack::ScriptSliderPack(ScriptBaseProcessor *base, Content *parentContent, Identifier imageName, int x, int y, int width, int height) :
ScriptComponent(base, parentContent, imageName, x, y, width, height),
packData(new SliderPackData()),
existingData(nullptr)
{
	deactivatedProperties.add(getIdFor(ScriptComponent::Properties::macroControl));
	
	

	propertyIds.add("sliderAmount");		ADD_AS_SLIDER_TYPE(0, 128, 1);
	propertyIds.add("stepSize");			
	propertyIds.add("flashActive");			ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
	propertyIds.add("showValueOverlay");	ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
    propertyIds.add("ProcessorId");         ADD_TO_TYPE_SELECTOR(SelectorTypes::ChoiceSelector);

	packData->setNumSliders(16);

	setDefaultValue(bgColour, 0x00000000);
	setDefaultValue(itemColour, 0x77FFFFFF);
	setDefaultValue(itemColour2, 0x77FFFFFF);
	setDefaultValue(textColour, 0x33FFFFFF);

	setDefaultValue(SliderAmount, 0);
	setDefaultValue(StepSize, 0);
	setDefaultValue(FlashActive, true);
	setDefaultValue(ShowValueOverlay, true);
    setDefaultValue(ProcessorId, "");

	setDefaultValue(SliderAmount, 16);
	setDefaultValue(StepSize, 0.01);

	setMethod("setSliderAtIndex", Wrapper::setSliderAtIndex);
	setMethod("getSliderValueAt", Wrapper::getSliderValueAt);
	setMethod("setAllValues", Wrapper::setAllValues);
	setMethod("getNumSliders", Wrapper::getNumSliders);
}

ScriptCreatedComponentWrapper * ScriptingApi::Content::ScriptSliderPack::createComponentWrapper(ScriptContentComponent *content, int index)
{
	return new ScriptCreatedComponentWrappers::SliderPackWrapper(content, this, index);
}

void ScriptingApi::Content::ScriptSliderPack::setSliderAtIndex(int index, double value)
{
    getSliderPackData()->setValue(index, (float)value, sendNotification);
}

double ScriptingApi::Content::ScriptSliderPack::getSliderValueAt(int index)
{
    SliderPackData *dataToUse = getSliderPackData();
    
    jassert(dataToUse != nullptr);
    
    dataToUse->setDisplayedIndex(index);

	return (double)dataToUse->getValue(index);
}

void ScriptingApi::Content::ScriptSliderPack::setAllValues(double value)
{
    SliderPackData *dataToUse = getSliderPackData();
    
    jassert(dataToUse != nullptr);

	for (int i = 0; i < dataToUse->getNumSliders(); i++)
	{
        dataToUse->setValue(i, (float)value, dontSendNotification);
	}

	getSliderPackData()->sendChangeMessage();
}

int ScriptingApi::Content::ScriptSliderPack::getNumSliders() const
{
	return getSliderPackData()->getNumSliders();
}

void ScriptingApi::Content::ScriptSliderPack::connectToOtherSliderPack(const String &otherPackId)
{
    if (otherPackId.isEmpty()) return;
    
    Processor::Iterator<Processor> it(getScriptProcessor()->getOwnerSynth(), false);
    
    Processor *p;
    
    while((p = it.getNextProcessor()) != nullptr)
    {
        if((dynamic_cast<SliderPackProcessor*>(p) != nullptr) && p->getId() == otherPackId)
        {
            existingData = dynamic_cast<SliderPackProcessor*>(p)->getSliderPackData(0);
            
            debugToConsole(getScriptProcessor(), otherPackId + " was found.");
            
            return;
        }
    }
    
    existingData = nullptr;
}

StringArray ScriptingApi::Content::ScriptSliderPack::getOptionsFor(const Identifier &id)
{
    if(id != getIdFor(ProcessorId)) return ScriptComponent::getOptionsFor(id);
    
    return ProcessorHelpers::getAllIdsForType<SliderPackProcessor>(getScriptProcessor()->getOwnerSynth());
};

void ScriptingApi::Content::ScriptSliderPack::setScriptObjectPropertyWithChangeMessage(const Identifier &id, var newValue, NotificationType notifyEditor /*= sendNotification*/)
{
	if (id == getIdFor(SliderAmount))
	{
        if(existingData.get() != nullptr) return;
        packData->setNumSliders(newValue);
	}
	else if (id == getIdFor(ScriptComponent::Properties::min))
	{
        if(existingData.get() != nullptr) return;
		packData->setRange(newValue, packData->getRange().getEnd(), newValue);
	}
	else if (id == getIdFor(ScriptComponent::Properties::max))
	{
        if(existingData.get() != nullptr) return;
		packData->setRange(packData->getRange().getStart(), newValue, newValue);
	}
	else if (id == getIdFor(StepSize))
	{
        if(existingData.get() != nullptr) return;
		packData->setRange(packData->getRange().getStart(), packData->getRange().getEnd(), newValue);
	}
	else if (id == getIdFor(FlashActive))
	{
        if(existingData.get() != nullptr) return;
		packData->setFlashActive((bool)newValue);
	}
	else if (id == getIdFor(ShowValueOverlay))
	{
        if(existingData.get() != nullptr) return;
		packData->setShowValueOverlay((bool)newValue);
	}
    else if (id == getIdFor(ProcessorId))
    {
        connectToOtherSliderPack(newValue.toString());
    }

	ScriptComponent::setScriptObjectPropertyWithChangeMessage(id, newValue, notifyEditor);
}

ScriptingApi::Content::ScriptButton::~ScriptButton()
{
	if (getScriptProcessor() != nullptr)
	{
		ImagePool *pool = getScriptProcessor()->getMainController()->getSampleManager().getImagePool();

		pool->releasePoolData(image);
	}
}

ScriptingApi::Content::ScriptButton::ScriptButton(ScriptBaseProcessor *base, Content *parentContent, Identifier name, int x, int y, int, int) :
ScriptComponent(base, parentContent, name, x, y, 128, 32),
image(nullptr)
{
	propertyIds.add("filmstripImage");	ADD_TO_TYPE_SELECTOR(SelectorTypes::FileSelector);
	propertyIds.add("isVertical");		ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
    propertyIds.add("scaleFactor");
	propertyIds.add("radioGroup");

	deactivatedProperties.add(getIdFor(ScriptComponent::Properties::max));
	deactivatedProperties.add(getIdFor(ScriptComponent::Properties::min));
	deactivatedProperties.add(getIdFor(ScriptComponent::Properties::textColour));
	deactivatedProperties.removeAllInstancesOf(getIdFor(ScriptComponent::Properties::isPluginParameter));

	setDefaultValue(ScriptButton::Properties::filmstripImage, "");
	setDefaultValue(ScriptButton::Properties::isVertical, true);
    setDefaultValue(ScriptButton::Properties::scaleFactor, 1.0f);
	setDefaultValue(ScriptButton::Properties::radioGroup, 0);
}

ScriptCreatedComponentWrapper * ScriptingApi::Content::ScriptButton::createComponentWrapper(ScriptContentComponent *content, int index)
{
	return new ScriptCreatedComponentWrappers::ButtonWrapper(content, this, index);
}

void ScriptingApi::Content::ScriptButton::setScriptObjectPropertyWithChangeMessage(const Identifier &id, var newValue, NotificationType notifyEditor /*= sendNotification*/)
{
	if (id == getIdFor(filmstripImage))
	{
		ImagePool *pool = getScriptProcessor()->getMainController()->getSampleManager().getImagePool();

		pool->releasePoolData(image);

		if (newValue == "Use default skin" || newValue == "")
		{
			setScriptObjectProperty(filmstripImage, "");
			image = nullptr;
		}
		else
		{
			setScriptObjectProperty(filmstripImage, newValue);

#if USE_FRONTEND

			String poolName = ProjectHandler::Frontend::getSanitiziedFileNameForPoolReference(newValue);

			image = pool->loadFileIntoPool(poolName, false);

			jassert(image != nullptr);

#else


			File actualFile = getExternalFile(newValue);

			image = pool->loadFileIntoPool(actualFile.getFullPathName(), false);

#endif
		}
	}

	ScriptComponent::setScriptObjectPropertyWithChangeMessage(id, newValue, notifyEditor);
}

StringArray ScriptingApi::Content::ScriptButton::getOptionsFor(const Identifier &id)
{
	if (id != getIdFor(filmstripImage)) return ScriptComponent::getOptionsFor(id);

	StringArray sa;

	sa.add("Load new File");

	sa.add("Use default skin");
	sa.addArray(getScriptProcessor()->getMainController()->getSampleManager().getImagePool()->getFileNameList());

	return sa;
}

ScriptingApi::Content::ScriptPanel::ScriptPanel(ScriptBaseProcessor *base, Content *parentContent, Identifier panelName, int x, int y, int width, int height) :
ScriptComponent(base, parentContent, panelName, x, y, width, height)
{
	deactivatedProperties.add(getIdFor(ScriptComponent::Properties::max));
	deactivatedProperties.add(getIdFor(ScriptComponent::Properties::min));
	deactivatedProperties.add(getIdFor(ScriptComponent::Properties::bgColour));
	deactivatedProperties.add(getIdFor(ScriptComponent::Properties::macroControl));
	deactivatedProperties.add(getIdFor(ScriptComponent::Properties::enabled));
	deactivatedProperties.add(getIdFor(ScriptComponent::Properties::tooltip));

	propertyIds.add("borderSize");		ADD_AS_SLIDER_TYPE(0, 20, 1);
	propertyIds.add("borderRadius");	ADD_AS_SLIDER_TYPE(0, 20, 1);
	propertyIds.add("allowCallbacks");	ADD_TO_TYPE_SELECTOR(SelectorTypes::ChoiceSelector);

	componentProperties->setProperty(getIdFor(borderSize), 0);
	componentProperties->setProperty(getIdFor(borderRadius), 0);
	componentProperties->setProperty(getIdFor(allowCallbacks), 0);

    setDefaultValue(ScriptComponent::Properties::saveInPreset, false);
	setDefaultValue(textColour, 0x23FFFFFF);
	setDefaultValue(itemColour, 0x30000000);
	setDefaultValue(itemColour2, 0x30000000);
	setDefaultValue(borderSize, 2.0f);
	setDefaultValue(borderRadius, 6.0f);
	setDefaultValue(allowCallbacks, 0);
}

StringArray ScriptingApi::Content::ScriptPanel::getOptionsFor(const Identifier &id)
{
	if (id == getIdFor(allowCallbacks))
	{
		return MouseCallbackComponent::getCallbackLevels();
	}

	return ScriptComponent::getOptionsFor(id);
}

ScriptCreatedComponentWrapper * ScriptingApi::Content::ScriptPanel::createComponentWrapper(ScriptContentComponent *content, int index)
{
	return new ScriptCreatedComponentWrappers::PanelWrapper(content, this, index);
}



void ScriptingApi::Content::PluginParameterConnector::setConnected(ScriptedControlAudioParameter *p)
{
	parameter = p;

	if (parameter != nullptr)
	{
		parameter->setControlledScriptComponent(dynamic_cast<ScriptComponent*>(this));
	}
}

void ScriptingApi::Content::PluginParameterConnector::sendParameterChangeNotification(float newValue)
{
	if (nextUpdateIsDeactivated)
	{
		nextUpdateIsDeactivated = false;
		return;
	}

	if (isConnected() && (parameter->getParameterIndex() != -1))
	{
		parameter->beginChangeGesture();
		parameter->setValueNotifyingHost(newValue);
		parameter->endChangeGesture();
	}
}

ScriptingApi::Content::ScriptPluginEditor::ScriptPluginEditor(ScriptBaseProcessor *base, Content*parentContent, Identifier name, int x, int y, int width, int height):
ScriptComponent(base, parentContent, name, x, y, width, height),
connectedProcessor(nullptr)
{
	propertyIds.add("processorId");	ADD_TO_TYPE_SELECTOR(SelectorTypes::ChoiceSelector);

	deactivatedProperties.add(getIdFor(text));
	deactivatedProperties.add(getIdFor(min));
	deactivatedProperties.add(getIdFor(max));
	deactivatedProperties.add(getIdFor(bgColour));
	deactivatedProperties.add(getIdFor(itemColour));
	deactivatedProperties.add(getIdFor(itemColour2));
	deactivatedProperties.add(getIdFor(textColour));
	deactivatedProperties.add(getIdFor(macroControl));

	componentProperties->setProperty(getIdFor(processorId), "");

	setMethod("connectToAudioSampleProcessor", Wrapper::connectToAudioSampleProcessor);
}

void ScriptingApi::Content::ScriptPluginEditor::connectToAudioProcessorWrapper(String processorId)
{
	Processor::Iterator<AudioProcessorWrapper> it(getScriptProcessor()->getOwnerSynth(), false);

	while (AudioProcessorWrapper *p = it.getNextProcessor())
	{
		if (p->getId() == processorId)
		{
			debugToConsole(getScriptProcessor(), processorId + " was found.");

			connectedProcessor = p;

			return;
		}

	}

	connectedProcessor = nullptr;
}

ScriptCreatedComponentWrapper * ScriptingApi::Content::ScriptPluginEditor::createComponentWrapper(ScriptContentComponent *content, int index)
{
	return new ScriptCreatedComponentWrappers::PluginEditorWrapper(content, this, index);
}

void ScriptingApi::Content::ScriptPluginEditor::setScriptObjectPropertyWithChangeMessage(const Identifier &id, var newValue, NotificationType notifyEditor /*= sendNotification*/)
{
	if (id == getIdFor(processorId))
	{
		connectToAudioProcessorWrapper(newValue.toString());
	}

	ScriptComponent::setScriptObjectPropertyWithChangeMessage(id, newValue, notifyEditor);
}

ValueTree ScriptingApi::Content::ScriptPluginEditor::exportAsValueTree() const
{
	ValueTree v = ScriptComponent::exportAsValueTree();

	const AudioProcessorWrapper *processor = dynamic_cast<const AudioProcessorWrapper*>(connectedProcessor.get());

	if (processor != nullptr)
	{
		v.setProperty("Processor", connectedProcessor->getId(), nullptr);
        
        if(const AudioProcessor *audioProcessor = processor->getWrappedAudioProcessor())
        {
            MemoryBlock data;
            
            const_cast<AudioProcessor*>(audioProcessor)->getStateInformation(data);
            
            v.setProperty("PluginData", data.toBase64Encoding(), nullptr);
        }
            
        
	}

	return v;
}

void ScriptingApi::Content::ScriptPluginEditor::restoreFromValueTree(const ValueTree &v)
{
	const String id = v.getProperty("Processor", "");

	if (id.isNotEmpty())
	{
		if (connectedProcessor.get() == nullptr || connectedProcessor.get()->getId() != id)
		{
			connectToAudioProcessorWrapper(id);
            
        }
        
        AudioProcessorWrapper *processor = dynamic_cast<AudioProcessorWrapper*>(connectedProcessor.get());
        
        if (processor != nullptr)
        {
            if(AudioProcessor *audioProcessor = processor->getWrappedAudioProcessor())
            {
                
                MemoryBlock data;
                
                const String dataString = v.getProperty("PluginData", "").toString();
                
                data.fromBase64Encoding(dataString);
                
                audioProcessor->setStateInformation(data.getData(), (int)data.getSize());
                
            }
        }
        
    }
}

StringArray ScriptingApi::Content::ScriptPluginEditor::getOptionsFor(const Identifier &id)
{
	if (id != getIdFor(processorId)) return ScriptComponent::getOptionsFor(id);

	return ProcessorHelpers::getAllIdsForType<AudioProcessorWrapper>(getScriptProcessor()->getOwnerSynth());
}

AudioProcessorWrapper * ScriptingApi::Content::ScriptPluginEditor::getProcessor()
{
	return dynamic_cast<AudioProcessorWrapper*>(connectedProcessor.get());
}

#undef SEND_MESSAGE
#undef ADD_TO_TYPE_SELECTOR
#undef ADD_AS_SLIDER_TYPE

void ScriptingObjects::TimerObject::timerCallback()
{
	var callback = getProperty("callback");
    var undefinedArgs = var::undefined();
    var::NativeFunctionArgs args(this, &undefinedArgs, 0);

    Result r = Result::ok();
    
    dynamic_cast<ScriptProcessor*>(getScriptProcessor())->getScriptEngine()->callExternalFunction(callback, args);
    
	if(r.failed())
    {
        stopTimer();
        debugError(getScriptProcessor(), r.getErrorMessage());
    }
}
