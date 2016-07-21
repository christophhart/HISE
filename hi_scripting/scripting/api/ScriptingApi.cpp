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


#define SEND_MESSAGE(broadcaster) {	if (MessageManager::getInstance()->isThisTheMessageThread()) broadcaster->sendSynchronousChangeMessage(); else broadcaster->sendChangeMessage();}
#define ADD_TO_TYPE_SELECTOR(x) (ScriptComponentPropertyTypeSelector::addToTypeSelector(ScriptComponentPropertyTypeSelector::x, propertyIds.getLast()))
#define ADD_AS_SLIDER_TYPE(min, max, interval) (ScriptComponentPropertyTypeSelector::addToTypeSelector(ScriptComponentPropertyTypeSelector::SliderSelector, propertyIds.getLast(), min, max, interval))



// ====================================================================================================== Message functions


struct ScriptingApi::Message::Wrapper
{
	API_VOID_METHOD_WRAPPER_1(Message, setNoteNumber);
	API_VOID_METHOD_WRAPPER_1(Message, setVelocity);
	API_VOID_METHOD_WRAPPER_1(Message, setControllerNumber);
	API_VOID_METHOD_WRAPPER_1(Message, setControllerValue);
	API_METHOD_WRAPPER_0(Message, getNoteNumber);
	API_METHOD_WRAPPER_0(Message, getVelocity);
	API_VOID_METHOD_WRAPPER_1(Message, ignoreEvent);
	API_VOID_METHOD_WRAPPER_1(Message, delayEvent);
	API_METHOD_WRAPPER_0(Message, getEventId);
	API_METHOD_WRAPPER_0(Message, getChannel);
	API_VOID_METHOD_WRAPPER_1(Message, setChannel);
};


ScriptingApi::Message::Message(ProcessorWithScriptingContent *p) :
ScriptingObject(p),
ApiClass(0),
ignored(false),
wrongNoteOff(false),
messageHolder(nullptr),
currentEventId(0),
eventIdCounter(0)
{
	ADD_API_METHOD_1(setNoteNumber);
	ADD_API_METHOD_1(setVelocity);
	ADD_API_METHOD_1(setControllerNumber);
	ADD_API_METHOD_1(setControllerValue);
	ADD_API_METHOD_0(getNoteNumber);
	ADD_API_METHOD_0(getVelocity);
	ADD_API_METHOD_1(ignoreEvent);
	ADD_API_METHOD_1(delayEvent);
	ADD_API_METHOD_0(getEventId);
	ADD_API_METHOD_0(getChannel);
	ADD_API_METHOD_1(setChannel);
}


ScriptingApi::Message::~Message()
{
	messageHolder = nullptr;
}

int ScriptingApi::Message::getNoteNumber() const
{
#if ENABLE_SCRIPTING_SAFE_CHECKS
	if(constMessageHolder == nullptr || !constMessageHolder->isNoteOnOrOff())
	{
		reportIllegalCall("getNoteNumber()", "onNoteOn / onNoteOff");
		return -1;
	}
#endif

	return constMessageHolder->getNoteNumber();
};


void ScriptingApi::Message::delayEvent(int samplesToDelay)
{
#if ENABLE_SCRIPTING_SAFE_CHECKS
	if(messageHolder == nullptr)
	{
		reportIllegalCall("delayEvent()", "midi event");
		return;
	}	
#endif

	messageHolder->addToTimeStamp(samplesToDelay);
};

void ScriptingApi::Message::setNoteNumber(int newValue)
{
#if ENABLE_SCRIPTING_SAFE_CHECKS
	if(messageHolder == nullptr)
	{
		reportIllegalCall("setNoteNumber()", "midi event");
		return;
	}
#endif

	if(!messageHolder->isNoteOnOrOff())
	{
		reportIllegalCall("setNoteNumber()", "noteOn / noteOff");
	}

	messageHolder->setNoteNumber(newValue);
};

void ScriptingApi::Message::setVelocity(int newValue)
{
#if ENABLE_SCRIPTING_SAFE_CHECKS
	if(messageHolder == nullptr)
	{
		reportIllegalCall("setVelocity()", "midi event");
		return;
	}

	if(!messageHolder->isNoteOn())
	{
		reportIllegalCall("setVelocity()", "onNoteOn");
	}
#endif

	messageHolder->setVelocity((float)newValue / 127.0f);
};




void ScriptingApi::Message::setControllerNumber(int newValue)
{
#if ENABLE_SCRIPTING_SAFE_CHECKS
	if(messageHolder == nullptr)
	{
		reportIllegalCall("setControllerNumber()", "midi event");
		return;
	}

	if(!messageHolder->isController())
	{
		reportIllegalCall("setControllerNumber()", "onController");
	}
#endif

	const int value = messageHolder->getControllerValue();

	*messageHolder = juce::MidiMessage::controllerEvent(1, newValue, value);
};

void ScriptingApi::Message::setControllerValue(int newValue)
{
#if ENABLE_SCRIPTING_SAFE_CHECKS
	if(messageHolder == nullptr)
	{
		reportIllegalCall("setVelocity()", "midi event");
		return;
	}

	if(!messageHolder->isController())
	{
		reportIllegalCall("setControllerValue()", "onController");
	}
#endif

	const int number = messageHolder->getControllerNumber();

	*messageHolder = juce::MidiMessage::controllerEvent(1, number, newValue);
};

var ScriptingApi::Message::getControllerNumber() const
{
#if ENABLE_SCRIPTING_SAFE_CHECKS
	if(constMessageHolder == nullptr || ( !constMessageHolder->isController() && !constMessageHolder->isPitchWheel() && !constMessageHolder->isAftertouch() ))
	{
		reportIllegalCall("getControllerNumber()", "onController");
		return var::undefined();
	}
#endif

	if(constMessageHolder->isController())		  return constMessageHolder->getControllerNumber();
	else if (constMessageHolder->isPitchWheel())	  return 128;
	else if (constMessageHolder->isAftertouch())   return 129;
	else									  return var::undefined();
};


var ScriptingApi::Message::getControllerValue() const
{
#if ENABLE_SCRIPTING_SAFE_CHECKS
	if(constMessageHolder == nullptr || ( !constMessageHolder->isController() && !constMessageHolder->isPitchWheel() && !constMessageHolder->isAftertouch() ))
	{
		reportIllegalCall("getControllerValue()", "onController");
		return var::undefined();
	}
#endif

	if      (constMessageHolder->isController())	  return constMessageHolder->getControllerValue();
	else if (constMessageHolder->isAftertouch())	  return constMessageHolder->getAfterTouchValue();
	else if (constMessageHolder->isPitchWheel())	  return constMessageHolder->getPitchWheelValue();
	else									  return var::undefined();
};

int ScriptingApi::Message::getVelocity() const
{
#if ENABLE_SCRIPTING_SAFE_CHECKS
	if(constMessageHolder == nullptr || (!constMessageHolder->isNoteOn()))
	{
		reportIllegalCall("getVelocity()", "onNoteOn");
		return -1;
	}
#endif

	return constMessageHolder->getVelocity();
};

int ScriptingApi::Message::getChannel() const
{
#if ENABLE_SCRIPTING_SAFE_CHECKS
	if(constMessageHolder == nullptr)
	{
		reportScriptError("Can only be called in MIDI callbacks");
		return -1;
	}
#endif

	return constMessageHolder->getChannel();
};

void ScriptingApi::Message::setChannel(int newValue)
{
#if ENABLE_SCRIPTING_SAFE_CHECKS
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
#endif

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
	constMessageHolder = m;

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

struct ScriptingApi::Engine::Wrapper
{
	API_VOID_METHOD_WRAPPER_0(Engine, allNotesOff);
	API_METHOD_WRAPPER_0(Engine, getUptime);
	API_METHOD_WRAPPER_0(Engine, getHostBpm);
	API_VOID_METHOD_WRAPPER_2(Engine, setGlobal);
	API_METHOD_WRAPPER_1(Engine, getGlobal);
	API_METHOD_WRAPPER_1(Engine, getMilliSecondsForTempo);
	API_METHOD_WRAPPER_1(Engine, getSamplesForMilliSeconds);
	API_METHOD_WRAPPER_1(Engine, getMilliSecondsForSamples);
	API_METHOD_WRAPPER_1(Engine, getGainFactorForDecibels);
	API_METHOD_WRAPPER_1(Engine, getDecibelsForGainFactor);
	API_METHOD_WRAPPER_1(Engine, getFrequencyForMidiNoteNumber);
	API_METHOD_WRAPPER_1(Engine, getPitchRatioFromSemitones);
	API_METHOD_WRAPPER_1(Engine, getSemitonesFromPitchRatio);
	API_METHOD_WRAPPER_0(Engine, getSampleRate);
	API_METHOD_WRAPPER_1(Engine, getMidiNoteName);
	API_METHOD_WRAPPER_1(Engine, getMidiNoteFromName);
	API_METHOD_WRAPPER_1(Engine, getMacroName);
	API_VOID_METHOD_WRAPPER_2(Engine, setKeyColour);
	API_VOID_METHOD_WRAPPER_1(Engine, setLowestKeyToDisplay);
	API_METHOD_WRAPPER_0(Engine, createMidiList);
	API_METHOD_WRAPPER_0(Engine, createTimerObject);
	API_VOID_METHOD_WRAPPER_1(Engine, openEditor);
	API_VOID_METHOD_WRAPPER_0(Engine, createLiveCodingVariables);
	API_VOID_METHOD_WRAPPER_1(Engine, include);
	API_METHOD_WRAPPER_0(Engine, getPlayHead);
	API_VOID_METHOD_WRAPPER_2(Engine, dumpAsJSON);
	API_METHOD_WRAPPER_1(Engine, loadFromJSON);
	API_METHOD_WRAPPER_0(Engine, getUserPresetDirectoryContent);
	API_VOID_METHOD_WRAPPER_1(Engine, setCompileProgress);
	API_METHOD_WRAPPER_2(Engine, matchesRegex);
	API_METHOD_WRAPPER_2(Engine, getRegexMatches);
	API_METHOD_WRAPPER_2(Engine, doubleToString);
	API_METHOD_WRAPPER_0(Engine, getOS);
	API_VOID_METHOD_WRAPPER_1(Engine, loadFont);
};

ScriptingApi::Engine::Engine(ProcessorWithScriptingContent *p) :
ScriptingObject(p),
ApiClass(0)
{
	ADD_API_METHOD_0(allNotesOff);
	ADD_API_METHOD_0(getUptime);
	ADD_API_METHOD_0(getHostBpm);
	ADD_API_METHOD_2(setGlobal);
	ADD_API_METHOD_1(getGlobal);
	ADD_API_METHOD_1(getMilliSecondsForTempo);
	ADD_API_METHOD_1(getSamplesForMilliSeconds);
	ADD_API_METHOD_1(getMilliSecondsForSamples);
	ADD_API_METHOD_1(getGainFactorForDecibels);
	ADD_API_METHOD_1(getDecibelsForGainFactor);
	ADD_API_METHOD_1(getFrequencyForMidiNoteNumber);
	ADD_API_METHOD_1(getPitchRatioFromSemitones);
	ADD_API_METHOD_1(getSemitonesFromPitchRatio);
	ADD_API_METHOD_0(getSampleRate);
	ADD_API_METHOD_1(getMidiNoteName);
	ADD_API_METHOD_1(getMidiNoteFromName);
	ADD_API_METHOD_1(getMacroName);
	ADD_API_METHOD_2(setKeyColour);
	ADD_API_METHOD_1(setLowestKeyToDisplay);
	ADD_API_METHOD_0(createMidiList);
	ADD_API_METHOD_1(openEditor);
	ADD_API_METHOD_0(createLiveCodingVariables);
	ADD_API_METHOD_1(include);
	ADD_API_METHOD_0(getPlayHead);
	ADD_API_METHOD_2(dumpAsJSON);
	ADD_API_METHOD_1(loadFromJSON);
	ADD_API_METHOD_0(getUserPresetDirectoryContent);
	ADD_API_METHOD_1(setCompileProgress);
	ADD_API_METHOD_2(matchesRegex);
	ADD_API_METHOD_2(getRegexMatches);
	ADD_API_METHOD_2(doubleToString);
	ADD_API_METHOD_0(getOS);
	ADD_API_METHOD_0(createTimerObject);
	ADD_API_METHOD_1(loadFont);
}



void ScriptingApi::Engine::allNotesOff()
{
	getProcessor()->getMainController()->allNotesOff();
};



void ScriptingApi::Engine::loadFont(const String &fileName)
{
#if USE_BACKEND

	const String absolutePath = GET_PROJECT_HANDLER(getProcessor()).getFilePath(fileName, ProjectHandler::SubDirectories::Images);
	File f(absolutePath);
	ScopedPointer<FileInputStream> fis = f.createInputStream();

	if (fis == nullptr)
	{
		reportScriptError("File not found");
		return;
	}
	else
	{
		MemoryBlock mb;

		fis->readIntoMemoryBlock(mb);
		getProcessor()->getMainController()->loadTypeFace(fileName, mb.getData(), mb.getSize());
	}

#else

	// Nothing to do here, it will be loaded on startup...

#endif
}

double ScriptingApi::Engine::getSampleRate() const { return const_cast<MainController*>(getProcessor()->getMainController())->getMainSynthChain()->getSampleRate(); }
double ScriptingApi::Engine::getSamplesForMilliSeconds(double milliSeconds) const { return (milliSeconds / 1000.0) * getSampleRate(); }

void ScriptingApi::Engine::setGlobal(int index, var valueToSave)
{
	if (valueToSave.isMethod() || (valueToSave.isObject() && !valueToSave.isArray()))
	{
		reportScriptError("Methods and Objects can't be stored in the global container");
		return;
	}

	getProcessor()->getMainController()->setGlobalVariable(index, valueToSave);
}

var ScriptingApi::Engine::getGlobal(int index) const { return getProcessor()->getMainController()->getGlobalVariable(index); }
double ScriptingApi::Engine::getUptime() const		 { return getProcessor()->getMainController()->getUptime(); }
double ScriptingApi::Engine::getHostBpm() const		 { return getProcessor()->getMainController()->getBpm(); }

String ScriptingApi::Engine::getMacroName(int index)
{
	if (index >= 1 && index <= 8)
		return getProcessor()->getMainController()->getMainSynthChain()->getMacroControlData(index-1)->getMacroName();
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
			return i;
	}
	return -1;
}




void ScriptingApi::Engine::openEditor(int includedFileIndex)
{
	JavascriptProcessor *sp = dynamic_cast<JavascriptProcessor*>(getScriptProcessor());

	if (sp != nullptr && includedFileIndex >= 0 && includedFileIndex < sp->getNumWatchedFiles())
	{
		dynamic_cast<JavascriptProcessor*>(getScriptProcessor())->showPopupForFile(includedFileIndex);
	}
	else
	{
		reportScriptError("Illegal File Index");
	}
}

void ScriptingApi::Engine::setKeyColour(int keyNumber, int colourAsHex) { getProcessor()->getMainController()->setKeyboardCoulour(keyNumber, Colour(colourAsHex));}
void ScriptingApi::Engine::setLowestKeyToDisplay(int keyNumber) { getProcessor()->getMainController()->setLowestKeyToDisplay(keyNumber); }
double ScriptingApi::Engine::getMilliSecondsForTempo(int tempoIndex) const { return (double)TempoSyncer::getTempoInMilliSeconds(getHostBpm(), (TempoSyncer::Tempo)tempoIndex); }
void ScriptingApi::Engine::include(const String &/*string*/) { jassertfalse; }


void ScriptingApi::Engine::createLiveCodingVariables()
{
	JavascriptMidiProcessor *sp = dynamic_cast<JavascriptMidiProcessor*>(getScriptProcessor());

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

DynamicObject * ScriptingApi::Engine::getPlayHead() { return getProcessor()->getMainController()->getHostInfoObject(); }
ScriptingObjects::MidiList *ScriptingApi::Engine::createMidiList() { return new ScriptingObjects::MidiList(getScriptProcessor()); };
ScriptingObjects::TimerObject* ScriptingApi::Engine::createTimerObject() { return new ScriptingObjects::TimerObject(getScriptProcessor()); }

void ScriptingApi::Engine::dumpAsJSON(var object, String fileName)
{
	if (!object.isObject())
	{
		reportScriptError("Only objects can be exported as JSON");
		return;
	}

	File f;

	if (File::isAbsolutePath(fileName))
		f = File(fileName);
	else
		f = File(GET_PROJECT_HANDLER(getProcessor()).getSubDirectory(ProjectHandler::SubDirectories::UserPresets).getChildFile(fileName));

	f.replaceWithText(JSON::toString(object, false));
	
}

var ScriptingApi::Engine::loadFromJSON(String fileName)
{
	File f;

	if (File::isAbsolutePath(fileName))
		f = File(fileName);
	else
		f = File(GET_PROJECT_HANDLER(getProcessor()).getSubDirectory(ProjectHandler::SubDirectories::UserPresets).getChildFile(fileName));

	if (f.existsAsFile())
		return JSON::parse(f);
	else
	{
		reportScriptError("File not found");
		return var::undefined();
	}
}

var ScriptingApi::Engine::getUserPresetDirectoryContent()
{
#if USE_FRONTEND
    
    const ValueTree v = dynamic_cast<FrontendProcessor*>(getProcessor()->getMainController())->getPresetData();
    
    var returnArray;
    
    for(int i = 0; i < v.getNumChildren(); i++)
    {
        returnArray.append(v.getChild(i).getProperty("FileName"));
    }
    
    return returnArray;
    
#else
	File presetDirectory = GET_PROJECT_HANDLER(getProcessor()).getSubDirectory(ProjectHandler::SubDirectories::UserPresets);

	if (presetDirectory.exists() && presetDirectory.isDirectory())
	{
		DirectoryIterator iter(GET_PROJECT_HANDLER(getProcessor()).getSubDirectory(ProjectHandler::SubDirectories::UserPresets), false, "*", File::findFiles);
		var returnArray;

		while (iter.next())
			returnArray.append(iter.getFile().getFileName());

		return returnArray;
	}
	else
		return var::undefined();

#endif    
}


void ScriptingApi::Engine::setCompileProgress(var progress)
{
	JavascriptProcessor *sp = dynamic_cast<JavascriptProcessor*>(getScriptProcessor());

	if (sp != nullptr)
		sp->setCompileProgress((double)progress);
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
		debugError(getProcessor(), e.what());
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
        debugError(getProcessor(), e.what());
        return var::undefined();
    }
    
    return var::undefined();
#endif
}

String ScriptingApi::Engine::doubleToString(double value, int digits)
{
    return String(value, digits);
}

// ====================================================================================================== Sampler functions

struct ScriptingApi::Sampler::Wrapper
{
	API_VOID_METHOD_WRAPPER_1(Sampler, enableRoundRobin);
	API_VOID_METHOD_WRAPPER_1(Sampler, setActiveGroup);
	API_METHOD_WRAPPER_2(Sampler, getRRGroupsForMessage);
	API_VOID_METHOD_WRAPPER_0(Sampler, refreshRRMap);
	API_VOID_METHOD_WRAPPER_1(Sampler, selectSounds);
	API_METHOD_WRAPPER_0(Sampler, getNumSelectedSounds);
	API_VOID_METHOD_WRAPPER_2(Sampler, setSoundPropertyForSelection);
	API_METHOD_WRAPPER_2(Sampler, getSoundProperty);
	API_VOID_METHOD_WRAPPER_3(Sampler, setSoundProperty);
	API_VOID_METHOD_WRAPPER_2(Sampler, purgeMicPosition);
	API_METHOD_WRAPPER_1(Sampler, getMicPositionName);
	API_VOID_METHOD_WRAPPER_0(Sampler, refreshInterface);
	API_VOID_METHOD_WRAPPER_1(Sampler, loadSampleMap);
};


ScriptingApi::Sampler::Sampler(ProcessorWithScriptingContent *p, ModulatorSampler *sampler_) :
ConstScriptingObject(p, ModulatorSamplerSound::Property::numProperties),
sampler(sampler_)
{
	ADD_API_METHOD_1(enableRoundRobin);
	ADD_API_METHOD_1(setActiveGroup);
	ADD_API_METHOD_2(getRRGroupsForMessage);
	ADD_API_METHOD_0(refreshRRMap);
	ADD_API_METHOD_1(selectSounds);
	ADD_API_METHOD_0(getNumSelectedSounds);
	ADD_API_METHOD_2(setSoundPropertyForSelection);
	ADD_API_METHOD_2(getSoundProperty);
	ADD_API_METHOD_3(setSoundProperty);
	ADD_API_METHOD_2(purgeMicPosition);
	ADD_API_METHOD_1(getMicPositionName);
	ADD_API_METHOD_0(refreshInterface);
	ADD_API_METHOD_1(loadSampleMap);

	for (int i = 1; i < ModulatorSamplerSound::numProperties; i++)
	{
		addConstant(ModulatorSamplerSound::getPropertyName((ModulatorSamplerSound::Property)i), i);
	}
}

void ScriptingApi::Sampler::enableRoundRobin(bool shouldUseRoundRobin)
{
	ModulatorSampler *s = static_cast<ModulatorSampler*>(sampler.get());

	if (s != nullptr)
	{
		s->setUseRoundRobinLogic(shouldUseRoundRobin);
	}
	else
	{
		reportScriptError("setActiveGroup() only works with Samplers.");
	}
}

void ScriptingApi::Sampler::setActiveGroup(int activeGroupIndex)
{
	ModulatorSampler *s = static_cast<ModulatorSampler*>(sampler.get());

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
	ModulatorSampler *s = static_cast<ModulatorSampler*>(sampler.get());

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
	ModulatorSampler *s = static_cast<ModulatorSampler*>(sampler.get());

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
	ModulatorSampler *s = static_cast<ModulatorSampler*>(sampler.get());

	if (s == nullptr)
	{
		reportScriptError("selectSamplerSounds() only works with Samplers.");
		return;
	}

	ModulatorSamplerSound::selectSoundsBasedOnRegex(regexWildcard, s, soundSelection);
}

int ScriptingApi::Sampler::getNumSelectedSounds()
{
	ModulatorSampler *s = static_cast<ModulatorSampler*>(sampler.get());

	if (s == nullptr)
	{
		reportScriptError("getNumSelectedSamplerSounds() only works with Samplers.");
		return -1;
	}

	return soundSelection.getNumSelected();
}

void ScriptingApi::Sampler::setSoundPropertyForSelection(int propertyId, var newValue)
{
	ModulatorSampler *s = static_cast<ModulatorSampler*>(sampler.get());

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
	ModulatorSampler *s = static_cast<ModulatorSampler*>(sampler.get());

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
	ModulatorSampler *s = static_cast<ModulatorSampler*>(sampler.get());

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
	ModulatorSampler *s = static_cast<ModulatorSampler*>(sampler.get());

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
	ModulatorSampler *s = static_cast<ModulatorSampler*>(sampler.get());

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
	ModulatorSampler *s = static_cast<ModulatorSampler*>(sampler.get());

	if (s == nullptr)
	{
		reportScriptError("purgeMicPosition() only works with Samplers.");
		return;
	}

	s->sendChangeMessage();
	s->getMainController()->getSampleManager().getModulatorSamplerSoundPool()->sendChangeMessage();
}

void ScriptingApi::Sampler::loadSampleMap(const String &fileName)
{
	ModulatorSampler *s = static_cast<ModulatorSampler*>(sampler.get());

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

		if (newId != unused && newId != oldId)
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

// ====================================================================================================== Synth functions



struct ScriptingApi::Synth::Wrapper
{
	API_VOID_METHOD_WRAPPER_2(Synth, allowChildSynth);
	API_METHOD_WRAPPER_0(Synth, getNumChildSynths);
	API_VOID_METHOD_WRAPPER_1(Synth, addToFront);
	API_VOID_METHOD_WRAPPER_1(Synth, deferCallbacks);
	API_VOID_METHOD_WRAPPER_1(Synth, noteOff);
	API_VOID_METHOD_WRAPPER_2(Synth, playNote);
	API_VOID_METHOD_WRAPPER_2(Synth, setAttribute);
	API_METHOD_WRAPPER_1(Synth, getAttribute);
	API_VOID_METHOD_WRAPPER_4(Synth, addNoteOn);
	API_VOID_METHOD_WRAPPER_3(Synth, addNoteOff);
	API_VOID_METHOD_WRAPPER_4(Synth, addController);
	API_VOID_METHOD_WRAPPER_2(Synth, setVoiceGainValue);
	API_VOID_METHOD_WRAPPER_2(Synth, setVoicePitchValue);
	API_VOID_METHOD_WRAPPER_1(Synth, startTimer);
	API_VOID_METHOD_WRAPPER_0(Synth, stopTimer);
	API_VOID_METHOD_WRAPPER_2(Synth, setMacroControl);
	API_VOID_METHOD_WRAPPER_2(Synth, sendController);
	API_VOID_METHOD_WRAPPER_2(Synth, sendControllerToChildSynths);
	API_VOID_METHOD_WRAPPER_4(Synth, setModulatorAttribute);
	API_VOID_METHOD_WRAPPER_3(Synth, addModulator);
	API_METHOD_WRAPPER_1(Synth, getModulator);
	API_METHOD_WRAPPER_1(Synth, getAudioSampleProcessor);
	API_METHOD_WRAPPER_1(Synth, getTableProcessor);
	API_METHOD_WRAPPER_1(Synth, getSampler);
	API_METHOD_WRAPPER_1(Synth, getEffect);
	API_METHOD_WRAPPER_1(Synth, getMidiProcessor);
	API_METHOD_WRAPPER_1(Synth, getChildSynth);
	API_METHOD_WRAPPER_2(Synth, getModulatorIndex);
	API_METHOD_WRAPPER_0(Synth, getNumPressedKeys);
	API_METHOD_WRAPPER_0(Synth, isLegatoInterval);
	API_METHOD_WRAPPER_0(Synth, isSustainPedalDown);
	API_VOID_METHOD_WRAPPER_1(Synth, setClockSpeed);

};


ScriptingApi::Synth::Synth(ProcessorWithScriptingContent *p, ModulatorSynth *ownerSynth):
	ScriptingObject(p),
	ApiClass(0),
	owner(ownerSynth),
	numPressedKeys(0),
	sustainState(false)
{
	jassert(owner != nullptr);

	

	ADD_API_METHOD_2(allowChildSynth);
	ADD_API_METHOD_0(getNumChildSynths);
	ADD_API_METHOD_1(addToFront);
	ADD_API_METHOD_1(deferCallbacks);
	ADD_API_METHOD_1(noteOff);
	ADD_API_METHOD_2(playNote);
	ADD_API_METHOD_2(setAttribute);
	ADD_API_METHOD_1(getAttribute);
	ADD_API_METHOD_4(addNoteOn);
	ADD_API_METHOD_3(addNoteOff);
	ADD_API_METHOD_4(addController);
	ADD_API_METHOD_2(setVoiceGainValue);
	ADD_API_METHOD_2(setVoicePitchValue);
	ADD_API_METHOD_1(startTimer);
	ADD_API_METHOD_0(stopTimer);
	ADD_API_METHOD_2(setMacroControl);
	ADD_API_METHOD_2(sendController);
	ADD_API_METHOD_2(sendControllerToChildSynths);
	ADD_API_METHOD_4(setModulatorAttribute);
	ADD_API_METHOD_3(addModulator);
	ADD_API_METHOD_1(getModulator);
	ADD_API_METHOD_1(getAudioSampleProcessor);
	ADD_API_METHOD_1(getTableProcessor);
	ADD_API_METHOD_1(getSampler);
	ADD_API_METHOD_1(getEffect);
	ADD_API_METHOD_1(getMidiProcessor);
	ADD_API_METHOD_1(getChildSynth);
	ADD_API_METHOD_2(getModulatorIndex);
	ADD_API_METHOD_0(getNumPressedKeys);
	ADD_API_METHOD_0(isLegatoInterval);
	ADD_API_METHOD_0(isSustainPedalDown);
	ADD_API_METHOD_1(setClockSpeed);
	
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
}

void ScriptingApi::Synth::addToFront(bool addToFront)
{
	
	dynamic_cast<JavascriptMidiProcessor*>(getScriptProcessor())->addToFront(addToFront);
}

void ScriptingApi::Synth::deferCallbacks(bool deferCallbacks)
{
	dynamic_cast<JavascriptMidiProcessor*>(getScriptProcessor())->deferCallbacks(deferCallbacks);
}

void ScriptingApi::Synth::playNote(int noteNumber, int velocity)
{
	if(velocity == 0)
	{
		reportScriptError("A velocity of 0 is not valid!");
		return;
	}

	// Set the timestamp to the future if this is called in the note off callback to prevent wrong order.
	const int timestamp = dynamic_cast<ScriptBaseMidiProcessor*>(getProcessor())->getCurrentMidiMessage().isNoteOff(false) ? 1 : 0;

	addNoteOn(1, noteNumber, velocity, timestamp);
}


void ScriptingApi::Synth::startTimer(double intervalInSeconds)
{
	if(intervalInSeconds < 0.04)
	{
		reportScriptError("Go easy on the timer!");
		return;
	}

	JavascriptMidiProcessor *p = dynamic_cast<JavascriptMidiProcessor*>(getScriptProcessor());

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
	JavascriptMidiProcessor *p = dynamic_cast<JavascriptMidiProcessor*>(getScriptProcessor());

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
	if(name == getProcessor()->getId())
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

void ScriptingApi::Synth::setVoiceGainValue(int voiceIndex, float gainValue)
{
	if (owner == nullptr)
	{
		jassertfalse;
		return;
	}

	owner->setScriptGainValue(voiceIndex, gainValue);
}

void ScriptingApi::Synth::setVoicePitchValue(int voiceIndex, double pitchValue)
{
	if (owner == nullptr)
	{
		jassertfalse;
		return;
	}

	owner->setScriptPitchValue(voiceIndex, pitchValue);
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
					if (ScriptBaseMidiProcessor* sp = dynamic_cast<ScriptBaseMidiProcessor*>(getScriptProcessor()))
					{
						MidiMessage m = MidiMessage::noteOn(channel, noteNumber, (uint8)velocity);
						m.setTimeStamp(sp->getCurrentMidiMessage().getTimeStamp() + timeStampSamples);

						sp->addMidiMessageToBuffer(m);
					}
					else reportScriptError("Only valid in MidiProcessors");
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
				if (ScriptBaseMidiProcessor* sp = dynamic_cast<ScriptBaseMidiProcessor*>(getProcessor()))
				{
					timeStampSamples = jmax<int>(1, timeStampSamples);

					MidiMessage m = MidiMessage::noteOff(channel, noteNumber);
					m.setTimeStamp(sp->getCurrentMidiMessage().getTimeStamp() + timeStampSamples);

					sp->addMidiMessageToBuffer(m);
				}
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
					if (ScriptBaseMidiProcessor* sp = dynamic_cast<ScriptBaseMidiProcessor*>(getProcessor()))
					{
						MidiMessage m = MidiMessage::controllerEvent(channel, number, value);
						m.setTimeStamp(sp->getCurrentMidiMessage().getTimeStamp() + timeStampSamples);

						sp->addMidiMessageToBuffer(m);
					}
					
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

	Processor *p = getProcessor()->getMainController()->createProcessor(c->getFactoryType(), type, id);

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

struct ScriptingApi::Console::Wrapper
{
	API_VOID_METHOD_WRAPPER_1(Console, print);
	API_VOID_METHOD_WRAPPER_1(Console, start);
	API_VOID_METHOD_WRAPPER_0(Console, stop);
};

ScriptingApi::Console::Console(ProcessorWithScriptingContent *p) :
ScriptingObject(p),
ApiClass(0),
startTime(0.0),
benchmarkTitle(String::empty)
{
	ADD_API_METHOD_1(print);
	ADD_API_METHOD_1(start);
	ADD_API_METHOD_0(stop);
}



void ScriptingApi::Console::print(var x)
{
#if USE_BACKEND
	debugToConsole(getProcessor(), x);
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

	debugToConsole(getProcessor(), benchmarkTitle + " Benchmark Result: " + String(ms, 3) + " ms");
#endif
}





#undef SEND_MESSAGE
#undef ADD_TO_TYPE_SELECTOR
#undef ADD_AS_SLIDER_TYPE

ScriptingApi::ModulatorApi::ModulatorApi(Modulator* mod_) :
ApiClass(0),
mod(mod_),
m(dynamic_cast<Modulation*>(mod_))
{
	ADD_API_METHOD_1(setIntensity);
	ADD_API_METHOD_1(setBypassed);
}
