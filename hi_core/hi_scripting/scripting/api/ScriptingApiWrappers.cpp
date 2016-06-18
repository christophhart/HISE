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

#define GET_OBJECT(x) (dynamic_cast<ScriptingApi::x*> (args.thisObject.getObject()))

#if ENABLE_SCRIPTING_SAFE_CHECKS

#define CHECK_VALID_ARGUMENTS() if(thisObject->checkValidArguments(args) != -1) return var::undefined();
#define CHECK_ARGUMENTS(callName, expectedArguments) if (!thisObject->checkArguments(callName, args.numArguments, expectedArguments)) return var::undefined(); CHECK_VALID_ARGUMENTS();
#define CHECK_IF_DEFERRED(x) if(!thisObject->checkIfSynchronous(x))return var::undefined();

#else

#define CHECK_VALID_ARGUMENTS() 
#define CHECK_ARGUMENTS(callName, expectedArguments)
#define CHECK_IF_DEFERRED(x)

#endif


// =================================================================================================== Message Wrappers

var CreatableScriptObject::Wrappers::checkExists(const var::NativeFunctionArgs& args)
{
	if(CreatableScriptObject* thisObject = dynamic_cast<CreatableScriptObject*>(args.thisObject.getObject()))
	{
		return thisObject->checkValidObject();
	}
	return var::undefined();
}


var ScriptingApi::Message::Wrapper::getNoteNumber(const var::NativeFunctionArgs& args)
{
	if(Message* thisObject = GET_OBJECT(Message))
	{
		return thisObject->getNoteNumber();
	}
	return var::undefined();
};

var ScriptingApi::Message::Wrapper::ignoreEvent(const var::NativeFunctionArgs& args)
{
	if(Message* thisObject = GET_OBJECT(Message))
	{
		CHECK_IF_DEFERRED("ignoreEvent");

		CHECK_ARGUMENTS("ignoreEvent()", 1);

		thisObject->ignoreEvent((bool)args.arguments[0]);
	}
	return var::undefined();
};



var ScriptingApi::Message::Wrapper::delayEvent(const var::NativeFunctionArgs& args)
{
	if(Message* thisObject = GET_OBJECT(Message))
	{
		CHECK_ARGUMENTS("delayEvent()", 1);
		
		CHECK_IF_DEFERRED("delayEvent");

		thisObject->delayEvent((int)args.arguments[0]);
	}
	return var::undefined();
};

var ScriptingApi::Message::Wrapper::setNoteNumber(const var::NativeFunctionArgs& args)
{
	if(Message* thisObject = GET_OBJECT(Message))
	{
		CHECK_ARGUMENTS("setNoteNumber()", 1);

		CHECK_IF_DEFERRED("setNoteNumber");
		
		thisObject->setNoteNumber((int)args.arguments[0]);
	}
	return var::undefined();
};

var ScriptingApi::Message::Wrapper::setVelocity(const var::NativeFunctionArgs& args)
{
	if(Message* thisObject = GET_OBJECT(Message))
	{
		CHECK_ARGUMENTS("setVelocity()", 1);

		CHECK_IF_DEFERRED("setVelocity");
		
		thisObject->setVelocity((int)args.arguments[0]);
	}
	return var::undefined();
};

var ScriptingApi::Message::Wrapper::setControllerNumber(const var::NativeFunctionArgs& args)
{
	if(Message* thisObject = GET_OBJECT(Message))
	{
		CHECK_ARGUMENTS("setControllerNumber()", 1);

		CHECK_IF_DEFERRED("setControllerNumber");
		
		thisObject->setControllerNumber((int)args.arguments[0]);
	}
	return var::undefined();
};


var ScriptingApi::Message::Wrapper::setControllerValue(const var::NativeFunctionArgs& args)
{
	if(Message* thisObject = GET_OBJECT(Message))
	{
		CHECK_ARGUMENTS("setControllerValue()", 1);
		CHECK_IF_DEFERRED("setControllerValue");
		
		thisObject->setControllerValue((int)args.arguments[0]);
	}
	return var::undefined();
};


var ScriptingApi::Message::Wrapper::getChannel(const var::NativeFunctionArgs& args)
{
	if(Message* thisObject = GET_OBJECT(Message))
	{
		return thisObject->getChannel();
	}
	return var::undefined();
};

var ScriptingApi::Message::Wrapper::setChannel(const var::NativeFunctionArgs& args)
{
	if(Message* thisObject = GET_OBJECT(Message))
	{
		CHECK_ARGUMENTS("setChannel()", 1);
		CHECK_IF_DEFERRED("setChannel");
		
		thisObject->setChannel((int)args.arguments[0]);
	}
	return var::undefined();
};

var ScriptingApi::Message::Wrapper::getEventId(const var::NativeFunctionArgs& args)
{
	if(Message* thisObject = GET_OBJECT(Message))
	{
		return thisObject->getEventId();
	}
	return var::undefined();
};

var ScriptingApi::Message::Wrapper::getVelocity(const var::NativeFunctionArgs& args)
{
	if(Message* thisObject = GET_OBJECT(Message))
	{
		return thisObject->getVelocity();
	}
	return var::undefined();
};

var ScriptingApi::Message::Wrapper::getControllerNumber(const var::NativeFunctionArgs& args)
{
	if(Message* thisObject = GET_OBJECT(Message))
	{
		return thisObject->getControllerNumber();
	}
	return var::undefined();
};

var ScriptingApi::Message::Wrapper::getControllerValue(const var::NativeFunctionArgs& args)
{
	if(Message* thisObject = GET_OBJECT(Message))
	{
		return thisObject->getControllerValue();
	}
	return var::undefined();
};



// =================================================================================================== Engine Wrappers

var ScriptingApi::Engine::Wrapper::getSamplesForMilliSeconds(const var::NativeFunctionArgs& args)
{
	if(Engine* thisObject = GET_OBJECT(Engine))
	{
		CHECK_ARGUMENTS("getSamplesForMilliSeconds()", 1);

		return thisObject->getSamplesForMilliSeconds((int)args.arguments[0]);
	}
	return var::undefined();
}

var ScriptingApi::Engine::Wrapper::getMilliSecondsForSamples(const var::NativeFunctionArgs& args)
{
	if(Engine* thisObject = GET_OBJECT(Engine))
	{
		CHECK_ARGUMENTS("getMilliSecondsForSamples()", 1);

		return thisObject->getMilliSecondsForSamples((double)args.arguments[0]);
	}
	return var::undefined();
}

var ScriptingApi::Engine::Wrapper::getGainFactorForDecibels(const var::NativeFunctionArgs& args)
{
	if(Engine* thisObject = GET_OBJECT(Engine))
	{
		CHECK_ARGUMENTS("getGainFactorForDecibel()", 1);

		return thisObject->getGainFactorForDecibels((double)args.arguments[0]);
	}
	return var::undefined();
}

var ScriptingApi::Engine::Wrapper::getDecibelsForGainFactor(const var::NativeFunctionArgs& args)
{
	if(Engine* thisObject = GET_OBJECT(Engine))
	{
		CHECK_ARGUMENTS("getDecibelForGainFactor()", 1);

		return thisObject->getDecibelsForGainFactor((double)args.arguments[0]);
	}
	return var::undefined();
}

var ScriptingApi::Engine::Wrapper::getFrequencyForMidiNoteNumber(const var::NativeFunctionArgs& args)
{
	if(Engine* thisObject = GET_OBJECT(Engine))
	{
		CHECK_ARGUMENTS("getFrequencyForMidiNoteNumber()", 1);

		return thisObject->getFrequencyForMidiNoteNumber((int)args.arguments[0]);
	}
	return var::undefined();
}

var ScriptingApi::Engine::Wrapper::getMidiNoteName(const var::NativeFunctionArgs& args)
{
	if(Engine* thisObject = GET_OBJECT(Engine))
	{
		CHECK_ARGUMENTS("getMidiNoteName()", 1);

		return thisObject->getMidiNoteName((int)args.arguments[0]);
	}
	return var::undefined();
}


var ScriptingApi::Engine::Wrapper::getMidiNoteFromName(const var::NativeFunctionArgs& args)
{
	if (Engine* thisObject = GET_OBJECT(Engine))
	{
		CHECK_ARGUMENTS("getMidiNoteFromName()", 1);

		return thisObject->getMidiNoteFromName(args.arguments[0].toString());
	}
	return var::undefined();
}


var ScriptingApi::Engine::Wrapper::getSampleRate(const var::NativeFunctionArgs& args)
{
	if(Engine* thisObject = GET_OBJECT(Engine))
	{
		CHECK_ARGUMENTS("getSampleRate()", 0);

		return thisObject->getSampleRate();
	}
	return var::undefined();
}

var ScriptingApi::Engine::Wrapper::allNotesOff(const var::NativeFunctionArgs& args)
{
	if(Engine* thisObject = GET_OBJECT(Engine))
	{
		thisObject->allNotesOff();
	}
	return var::undefined();
};

var ScriptingApi::Engine::Wrapper::getUptime(const var::NativeFunctionArgs& args)
{
	if(Engine* thisObject = GET_OBJECT(Engine))
	{
		return thisObject->getUptime();
	}
	return var::undefined();
};

var ScriptingApi::Engine::Wrapper::getHostBpm(const var::NativeFunctionArgs& args)
{
	if(Engine* thisObject = GET_OBJECT(Engine))
	{
		return thisObject->getHostBpm();
	}
	return var::undefined();
};

var ScriptingApi::Engine::Wrapper::getMilliSecondsForTempo(const var::NativeFunctionArgs& args)
{
	if(Engine* thisObject = GET_OBJECT(Engine))
	{
		CHECK_ARGUMENTS("getMilliSecondsFromTempo()", 1);

		return thisObject->getMilliSecondsForTempo((int)args.arguments[0]);
	}
	return var::undefined();
};



var ScriptingApi::Engine::Wrapper::setKeyColour(const var::NativeFunctionArgs& args)
{
	if(Engine* thisObject = GET_OBJECT(Engine))
	{
		CHECK_ARGUMENTS("setKeyColour()", 2);

		thisObject->setKeyColour((int)args.arguments[0], (int)args.arguments[1]);
	}
	return var::undefined();
};

var ScriptingApi::Engine::Wrapper::setLowestKeyToDisplay(const var::NativeFunctionArgs& args)
{
	if (Engine* thisObject = GET_OBJECT(Engine))
	{
		CHECK_ARGUMENTS("setLowestKeyToDisplay()", 1);

		thisObject->setLowestKeyToDisplay((int)args.arguments[0]);
	}
	return var::undefined();
}



var ScriptingApi::Engine::Wrapper::getMacroName(const var::NativeFunctionArgs& args)
{
	if (Engine* thisObject = GET_OBJECT(Engine))
	{
		CHECK_ARGUMENTS("getMacroName()", 1);

		return thisObject->getMacroName((int)args.arguments[0]);
	}
	return var::undefined();
}

var ScriptingApi::Engine::Wrapper::getGlobal(const var::NativeFunctionArgs& args)
{
	if (Engine* thisObject = GET_OBJECT(Engine))
	{
		CHECK_ARGUMENTS("getGlobal()", 1);

		return thisObject->getGlobal((int)args.arguments[0]);
	}
	return var::undefined();
}


var ScriptingApi::Engine::Wrapper::setGlobal(const var::NativeFunctionArgs& args)
{
	if (Engine* thisObject = GET_OBJECT(Engine))
	{
		if (!thisObject->checkArguments("setGlobal()", args.numArguments, 2)) return var::undefined(); // It must be possible to set a invalid argument as global value

		thisObject->setGlobal((int)args.arguments[0], args.arguments[1]);
	}
	return var::undefined();
}

var ScriptingApi::Engine::Wrapper::include(const var::NativeFunctionArgs& args)
{
	if (Engine* thisObject = GET_OBJECT(Engine))
	{
		CHECK_ARGUMENTS("include()", 1);

		thisObject->includeFile(args.arguments[0].toString());
		
	}
	return var::undefined();
}

var ScriptingApi::Engine::Wrapper::createMidiList(const var::NativeFunctionArgs& args)
{
    if (Engine* thisObject = GET_OBJECT(Engine))
    {
        CHECK_ARGUMENTS("createMidiList()", 0);
        
        return thisObject->createMidiList();
        
    }
    return var::undefined();
}


var ScriptingApi::Engine::Wrapper::openEditor(const var::NativeFunctionArgs& args)
{
	if (Engine* thisObject = GET_OBJECT(Engine))
	{
		CHECK_ARGUMENTS("openEditor()", 1);

		thisObject->openEditor((int)args.arguments[0]);

	}
	return var::undefined();
}


var ScriptingApi::Engine::Wrapper::createLiveCodingVariables(const var::NativeFunctionArgs& args)
{
	if (Engine* thisObject = GET_OBJECT(Engine))
	{
		CHECK_ARGUMENTS("createLiveCodingVariables()", 0);

		thisObject->createLiveCodingVariables();

	}
	return var::undefined();
}

var ScriptingApi::Engine::Wrapper::getPlayHead(const var::NativeFunctionArgs& args)
{
	if (Engine* thisObject = GET_OBJECT(Engine))
	{
		CHECK_ARGUMENTS("getPlayHead()", 0);

		ReferenceCountedObject * r = thisObject->getPlayHead();

		return var(r);

	}
	return var::undefined();
}

var ScriptingApi::Engine::Wrapper::dumpAsJSON(const var::NativeFunctionArgs& args)
{
	if (Engine* thisObject = GET_OBJECT(Engine))
	{
		CHECK_ARGUMENTS("dumpAsJSON()", 2);

		thisObject->dumpAsJSON(args.arguments[0], args.arguments[1]);

	}
	return var::undefined();
}

var ScriptingApi::Engine::Wrapper::loadFromJSON(const var::NativeFunctionArgs& args)
{
	if (Engine* thisObject = GET_OBJECT(Engine))
	{
		CHECK_ARGUMENTS("loadFromJSON()", 1);

		return thisObject->loadFromJSON(args.arguments[0].toString());

	}
	return var::undefined();
}


var ScriptingApi::Engine::Wrapper::getUserPresetDirectoryContent(const var::NativeFunctionArgs& args)
{
	if (Engine* thisObject = GET_OBJECT(Engine))
	{
		CHECK_ARGUMENTS("getUserPresetDirectoryContent()", 0);

		return thisObject->getUserPresetDirectoryContent();

	}
	return var::undefined();
}


var ScriptingApi::Engine::Wrapper::setCompileProgress(const var::NativeFunctionArgs& args)
{
	if (Engine* thisObject = GET_OBJECT(Engine))
	{
		CHECK_ARGUMENTS("setCompileTimeOut()", 1);

		thisObject->setCompileProgress((double)args.arguments[0]);

	}
	return var::undefined();
}


var ScriptingApi::Engine::Wrapper::matchesRegex(const var::NativeFunctionArgs& args)
{
	if (Engine* thisObject = GET_OBJECT(Engine))
	{
		CHECK_ARGUMENTS("matchesRegex()", 2);

		return thisObject->matchesRegex(args.arguments[0].toString(), args.arguments[1].toString());

	}
	return var::undefined();
}

var ScriptingApi::Engine::Wrapper::getRegexMatches(const var::NativeFunctionArgs& args)
{
    if (Engine* thisObject = GET_OBJECT(Engine))
    {
        CHECK_ARGUMENTS("getRegexMatches()", 2);
        
        return thisObject->getRegexMatches(args.arguments[0].toString(), args.arguments[1].toString());
        
    }
    return var::undefined();
}


var ScriptingApi::Engine::Wrapper::doubleToString(const var::NativeFunctionArgs& args)
{
    if (Engine* thisObject = GET_OBJECT(Engine))
    {
        CHECK_ARGUMENTS("doubleToString()", 2);
        
        return thisObject->doubleToString(args.arguments[0], args.arguments[1]);
    }
    
    return var::undefined();
}



var ScriptingApi::Engine::Wrapper::getOS(const var::NativeFunctionArgs& args)
{
	if (Engine* thisObject = GET_OBJECT(Engine))
	{
		CHECK_ARGUMENTS("getOS()", 0);

		return thisObject->getOS();
	}

	return var::undefined();
}


// =================================================================================================== Synth Wrappers

var ScriptingApi::Synth::Wrapper::allowChildSynth(const var::NativeFunctionArgs& args)
{
	if(Synth* thisObject = GET_OBJECT(Synth))
	{
		CHECK_ARGUMENTS("allowChildSynth()", 2);

		thisObject->allowChildSynth((int)args.arguments[0], (bool)args.arguments[1]);
	}
	return var::undefined();
};

var ScriptingApi::Synth::Wrapper::getNumChildSynths(const var::NativeFunctionArgs& args)
{
	if(Synth* thisObject = GET_OBJECT(Synth))
	{
		return thisObject->getNumChildSynths();
	}
	return var::undefined();
};

var ScriptingApi::Synth::Wrapper::getNumPressedKeys(const var::NativeFunctionArgs& args)
{
	if(Synth* thisObject = GET_OBJECT(Synth))
	{
		return thisObject->getNumPressedKeys();
	}
	return var::undefined();
};

var ScriptingApi::Synth::Wrapper::isLegatoInterval(const var::NativeFunctionArgs& args)
{
	if(Synth* thisObject = GET_OBJECT(Synth))
	{
		return thisObject->isLegatoInterval();
	}
	return var::undefined();
};

var ScriptingApi::Synth::Wrapper::isSustainPedalDown(const var::NativeFunctionArgs& args)
{
	if(Synth* thisObject = GET_OBJECT(Synth))
	{
		return thisObject->isSustainPedalDown();
	}
	return var::undefined();
};

var ScriptingApi::Synth::Wrapper::noteOff(const var::NativeFunctionArgs& args)
{
	if(Synth* thisObject = GET_OBJECT(Synth))
	{
		CHECK_ARGUMENTS("noteOff()", 1);

		thisObject->noteOff((int)args.arguments[0]);
	}
	return var::undefined();
};

var ScriptingApi::Synth::Wrapper::sendController(const var::NativeFunctionArgs& args)
{
	if(Synth* thisObject = GET_OBJECT(Synth))
	{
		CHECK_ARGUMENTS("sendController()", 2);

		thisObject->sendController((int)args.arguments[0], (int)args.arguments[1]);
	}
	return var::undefined();
};

var ScriptingApi::Synth::Wrapper::sendControllerToChildSynths(const var::NativeFunctionArgs& args)
{
	if(Synth* thisObject = GET_OBJECT(Synth))
	{
		CHECK_ARGUMENTS("sendControllerToChildSynths()", 2);

		thisObject->sendControllerToChildSynths((int)args.arguments[0], (int)args.arguments[1]);
	}
	return var::undefined();
};


var ScriptingApi::Synth::Wrapper::setMacroControl(const var::NativeFunctionArgs& args)
{
	if(Synth* thisObject = GET_OBJECT(Synth))
	{
		CHECK_ARGUMENTS("setMacroControl()", 2);

		thisObject->setMacroControl((int)args.arguments[0], (float)args.arguments[1]);
	}
	return var::undefined();
};

var ScriptingApi::Synth::Wrapper::startTimer(const var::NativeFunctionArgs& args)
{
	if(Synth* thisObject = GET_OBJECT(Synth))
	{
		CHECK_ARGUMENTS("startTimer()", 1);

		thisObject->startTimer((double)args.arguments[0]);
	}
	return var::undefined();
};

var ScriptingApi::Synth::Wrapper::stopTimer(const var::NativeFunctionArgs& args)
{
	if(Synth* thisObject = GET_OBJECT(Synth))
	{
		thisObject->stopTimer();
	}
	return var::undefined();
};

var ScriptingApi::Synth::Wrapper::playNote(const var::NativeFunctionArgs& args)
{
	if(Synth* thisObject = GET_OBJECT(Synth))
	{
		CHECK_ARGUMENTS("playNote()", 2);

		thisObject->playNote((int)args.arguments[0], (int)args.arguments[1]);
	}
	return var::undefined();
};

var ScriptingApi::Synth::Wrapper::deferCallbacks(const var::NativeFunctionArgs& args)
{
	if(Synth* thisObject = GET_OBJECT(Synth))
	{
		CHECK_ARGUMENTS("deferCallbacks()", 1);

		thisObject->deferCallbacks((bool)args.arguments[0]);
	}
	return var::undefined();
};

var ScriptingApi::Synth::Wrapper::addToFront(const var::NativeFunctionArgs& args)
{
	if(Synth* thisObject = GET_OBJECT(Synth))
	{
		CHECK_ARGUMENTS("addToFront()", 1);

		thisObject->addToFront((bool)args.arguments[0]);
	}
	return var::undefined();
};

var ScriptingApi::Synth::Wrapper::getModulator(const var::NativeFunctionArgs& args)
{
	if(Synth* thisObject = GET_OBJECT(Synth))
	{
		CHECK_ARGUMENTS("getModulator()", 1);

		return thisObject->getModulator(args.arguments[0].toString());
	}
	return var::undefined();
};

var ScriptingApi::Synth::Wrapper::getEffect(const var::NativeFunctionArgs& args)
{
	if(Synth* thisObject = GET_OBJECT(Synth))
	{
		CHECK_ARGUMENTS("getEffect()", 1);

		return thisObject->getEffect(args.arguments[0].toString());
	}
	return var::undefined();
};


var ScriptingApi::Synth::Wrapper::getAudioSampleProcessor(const var::NativeFunctionArgs& args)
{
	if (Synth* thisObject = GET_OBJECT(Synth))
	{
		CHECK_ARGUMENTS("getScriptingAudioSampleProcessor()", 1);

		return thisObject->getAudioSampleProcessor(args.arguments[0].toString());
	}
	return var::undefined();
}


var ScriptingApi::Synth::Wrapper::getSampler(const var::NativeFunctionArgs& args)
{
	if (Synth* thisObject = GET_OBJECT(Synth))
	{
		CHECK_ARGUMENTS("getSampler()", 1);

		return thisObject->getSampler(args.arguments[0].toString());
	}
	return var::undefined();
}


var ScriptingApi::Synth::Wrapper::getMidiProcessor(const var::NativeFunctionArgs& args)
{
	if(Synth* thisObject = GET_OBJECT(Synth))
	{
		CHECK_ARGUMENTS("getMidiProcessor()", 1);

		return thisObject->getMidiProcessor(args.arguments[0].toString());
	}
	return var::undefined();
};

var ScriptingApi::Synth::Wrapper::getChildSynth(const var::NativeFunctionArgs& args)
{
	if(Synth* thisObject = GET_OBJECT(Synth))
	{
		CHECK_ARGUMENTS("getChildSynth()", 1);

		return thisObject->getChildSynth(args.arguments[0].toString());
	}
	return var::undefined();
};

var ScriptingApi::Synth::Wrapper::setModulatorAttribute(const var::NativeFunctionArgs& args)
{
	if(Synth* thisObject = GET_OBJECT(Synth))
	{
		CHECK_ARGUMENTS("setModulatorAttribute()", 4);

		thisObject->setModulatorAttribute((int)args.arguments[0], 
										  (int)args.arguments[1], 
										  (int)args.arguments[2], 
										  (float)args.arguments[3]);
	}
	return var::undefined();
};

var ScriptingApi::Synth::Wrapper::setAttribute(const var::NativeFunctionArgs& args)
{
	if(Synth* thisObject = GET_OBJECT(Synth))
	{
		CHECK_ARGUMENTS("setModulatorAttribute()", 2);

		thisObject->setAttribute((int)args.arguments[0], 
										  (float)args.arguments[1]);
	}
	return var::undefined();
};

var ScriptingApi::Synth::Wrapper::getAttribute(const var::NativeFunctionArgs& args)
{
	if (Synth* thisObject = GET_OBJECT(Synth))
	{
		CHECK_ARGUMENTS("getModulatorAttribute()", 1);

		return thisObject->getAttribute((int)args.arguments[0]);
	}
	return var::undefined();
}

var ScriptingApi::Synth::Wrapper::addNoteOn(const var::NativeFunctionArgs& args)
{
	if (Synth* thisObject = GET_OBJECT(Synth))
	{
		CHECK_ARGUMENTS("addNoteOn()", 4);

		thisObject->addNoteOn((int)args.arguments[0], (int)args.arguments[1], (int)args.arguments[2], (int)args.arguments[3]);
	}
	return var::undefined();
}

var ScriptingApi::Synth::Wrapper::addNoteOff(const var::NativeFunctionArgs& args)
{
	if (Synth* thisObject = GET_OBJECT(Synth))
	{
		CHECK_ARGUMENTS("addNoteOff()", 3);

		thisObject->addNoteOff((int)args.arguments[0], (int)args.arguments[1], (int)args.arguments[2]);
	}
	return var::undefined();
}

var ScriptingApi::Synth::Wrapper::addController(const var::NativeFunctionArgs& args)
{
	if (Synth* thisObject = GET_OBJECT(Synth))
	{
		CHECK_ARGUMENTS("addController()", 4);

		thisObject->addController((int)args.arguments[0], (int)args.arguments[1], (int)args.arguments[2], (int)args.arguments[3]);
	}
	return var::undefined();
}


var ScriptingApi::Synth::Wrapper::addModulator(const var::NativeFunctionArgs& args)
{
	if(Synth* thisObject = GET_OBJECT(Synth))
	{
		CHECK_ARGUMENTS("addModulator()", 3);

		return thisObject->addModulator((int)args.arguments[0], 
										  args.arguments[1].toString(),
										  args.arguments[2].toString());
	}
	return var::undefined();
};

var ScriptingApi::Synth::Wrapper::getModulatorIndex(const var::NativeFunctionArgs& args)
{
	if(Synth* thisObject = GET_OBJECT(Synth))
	{
		CHECK_ARGUMENTS("addModulator()", 2);

		return thisObject->getModulatorIndex((int)args.arguments[0], 
                                             args.arguments[1].toString());
	}
	return var::undefined();
};


var ScriptingApi::Synth::Wrapper::setClockSpeed(const var::NativeFunctionArgs& args)
{
	if (Synth* thisObject = GET_OBJECT(Synth))
	{
		CHECK_ARGUMENTS("setClockSpeed()", 1);

		thisObject->setClockSpeed((int)args.arguments[0]);
	}
	return var::undefined();
}


// ============================================================================================= SAMPLER Wrapper methods


var ScriptingApi::Sampler::Wrapper::enableRoundRobin(const var::NativeFunctionArgs& args)
{
	if (Sampler* thisObject = GET_OBJECT(Sampler))
	{
		CHECK_ARGUMENTS("enableRoundRobin()", 1);

		thisObject->enableRoundRobin((bool)args.arguments[0]);
	}
	return var::undefined();
}

var ScriptingApi::Sampler::Wrapper::setActiveGroup(const var::NativeFunctionArgs& args)
{
	if (Sampler* thisObject = GET_OBJECT(Sampler))
	{
		CHECK_ARGUMENTS("setActiveGroup", 1);

		thisObject->setActiveGroup((int)args.arguments[0]);
	}
	return var::undefined();
}


var ScriptingApi::Sampler::Wrapper::refreshRRMap(const var::NativeFunctionArgs& args)
{
	if (Sampler* thisObject = GET_OBJECT(Sampler))
	{
		CHECK_ARGUMENTS("refreshRRMap", 0);

		thisObject->refreshRRMap();
	}
	return var::undefined();
}

var ScriptingApi::Sampler::Wrapper::getRRGroupsForMessage(const var::NativeFunctionArgs& args)
{
	if (Sampler* thisObject = GET_OBJECT(Sampler))
	{
		CHECK_ARGUMENTS("getRRGroupsForMessage", 2);

		return thisObject->getRRGroupsForMessage((int)args.arguments[0], (int)args.arguments[1]);
	}
	return var::undefined();
}

var ScriptingApi::Sampler::Wrapper::selectSounds(const var::NativeFunctionArgs& args)
{
	if (Sampler* thisObject = GET_OBJECT(Sampler))
	{
		CHECK_ARGUMENTS("selectSamplerSounds", 1);

		thisObject->selectSounds(args.arguments[0].toString());
	}
	return var::undefined();
}

var ScriptingApi::Sampler::Wrapper::getNumSelectedSounds(const var::NativeFunctionArgs& args)
{
	if (Sampler* thisObject = GET_OBJECT(Sampler))
	{
		CHECK_ARGUMENTS("getNumSelectedSounds", 0);

		return thisObject->getNumSelectedSounds();
	}
	return var::undefined();
}

var ScriptingApi::Sampler::Wrapper::setSoundPropertyForSelection(const var::NativeFunctionArgs& args)
{
	if (Sampler* thisObject = GET_OBJECT(Sampler))
	{
		CHECK_ARGUMENTS("setSoundPropertyForSelection", 2);

		thisObject->setSoundPropertyForSelection((int)args.arguments[0], args.arguments[1]);
	}
	return var::undefined();
}



var ScriptingApi::Sampler::Wrapper::getSoundProperty(const var::NativeFunctionArgs& args)
{
	if (Sampler* thisObject = GET_OBJECT(Sampler))
	{
		CHECK_ARGUMENTS("getSoundProperty", 2);

		return thisObject->getSoundProperty((int)args.arguments[0], (int)args.arguments[1]);
	}
	return var::undefined();
}


var ScriptingApi::Sampler::Wrapper::setSoundProperty(const var::NativeFunctionArgs& args)
{
	if (Sampler* thisObject = GET_OBJECT(Sampler))
	{
		CHECK_ARGUMENTS("setSoundProperty", 3);

		thisObject->setSoundProperty((int)args.arguments[0], (int)args.arguments[1], args.arguments[2]);
	}
	return var::undefined();
}


var ScriptingApi::Sampler::Wrapper::purgeMicPosition(const var::NativeFunctionArgs& args)
{
	if (Sampler* thisObject = GET_OBJECT(Sampler))
	{
		CHECK_ARGUMENTS("purgeMicPosition", 2);

		thisObject->purgeMicPosition(args.arguments[0].toString(), (bool)args.arguments[1]);
	}
	return var::undefined();
}

var ScriptingApi::Sampler::Wrapper::getMicPositionName(const var::NativeFunctionArgs& args)
{
	if (Sampler* thisObject = GET_OBJECT(Sampler))
	{
		CHECK_ARGUMENTS("getMicPositionName", 1);

		return thisObject->getMicPositionName((int)args.arguments[0]);
	}
	return var::undefined();
}


var ScriptingApi::Sampler::Wrapper::refreshInterface(const var::NativeFunctionArgs& args)
{
	if (Sampler* thisObject = GET_OBJECT(Sampler))
	{
		CHECK_ARGUMENTS("refreshInterface", 0);

		thisObject->refreshInterface();
	}
	return var::undefined();
}


var ScriptingApi::Sampler::Wrapper::loadSampleMap(const var::NativeFunctionArgs& args)
{
	if (Sampler* thisObject = GET_OBJECT(Sampler))
	{
		CHECK_ARGUMENTS("loadSampleMap", 1);

		thisObject->loadSampleMap(args.arguments[0].toString());
	}
	return var::undefined();
}


// =================================================================================================== Console Wrappers

var ScriptingApi::Console::Wrapper::print (const var::NativeFunctionArgs& args)
{
	
	if (ScriptingApi::Console* thisObject = GET_OBJECT(Console))
	{
		CHECK_ARGUMENTS("print()", 1);

		if (!thisObject->checkArguments("print()", args.numArguments, 1)) return var::undefined(); // It must be possible to set a invalid argument as global value
			
		thisObject->print(args.arguments[0]);
	}

	return var::undefined();
};

var ScriptingApi::Console::Wrapper::start (const var::NativeFunctionArgs& args)
{		
	if (ScriptingApi::Console* thisObject = GET_OBJECT(Console))
	{
		if (args.numArguments > 0)	thisObject->start(args.arguments[0]);
		else						thisObject->start();
	}

	return var::undefined();
};

var ScriptingApi::Console::Wrapper::stop (const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Console* thisObject = GET_OBJECT(Console))
	{
		thisObject->stop();
	}

	return var::undefined();
};


// =================================================================================================== Content Wrappers

var ScriptingApi::Content::Wrapper::addButton (const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content* thisObject = GET_OBJECT(Content))
	{
		if(args.numArguments == 1)
		{
			return thisObject->addButton(Identifier(args.arguments[0]), 0, 0);
		}

		CHECK_ARGUMENTS("addButton()", 3);

		return thisObject->addButton(Identifier(args.arguments[0]), args.arguments[1], args.arguments[2]);
	}

	return var::undefined();
};

var ScriptingApi::Content::Wrapper::addKnob (const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content* thisObject = GET_OBJECT(Content))
	{
		if(args.numArguments == 1)
		{
			return thisObject->addKnob(Identifier(args.arguments[0]), 0, 0);
		}

		CHECK_ARGUMENTS("addKnob()", 3);

		return thisObject->addKnob(Identifier(args.arguments[0]), args.arguments[1], args.arguments[2]);
	}

	return var::undefined();
};

var ScriptingApi::Content::Wrapper::addLabel (const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content* thisObject = GET_OBJECT(Content))
	{
		if(args.numArguments == 1)
		{
			return thisObject->addLabel(Identifier(args.arguments[0]), 0, 0);
		}

		CHECK_ARGUMENTS("addLabel()", 3);

		return thisObject->addLabel(Identifier(args.arguments[0]), args.arguments[1], args.arguments[2]);
	}

	return var::undefined();
};

var ScriptingApi::Content::Wrapper::addComboBox (const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content* thisObject = GET_OBJECT(Content))
	{
		if(args.numArguments == 1)
		{
			return thisObject->addComboBox(Identifier(args.arguments[0]), 0, 0);
		}

		CHECK_ARGUMENTS("addComboBox()", 3);

		return thisObject->addComboBox(Identifier(args.arguments[0]), args.arguments[1], args.arguments[2]);
	}

	return var::undefined();
};

var ScriptingApi::Content::Wrapper::addTable (const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content* thisObject = GET_OBJECT(Content))
	{
		if(args.numArguments == 1)
		{
			return thisObject->addTable(Identifier(args.arguments[0]), 0,0);
		}

		CHECK_ARGUMENTS("addTable()", 3);

		return thisObject->addTable(Identifier(args.arguments[0]), args.arguments[1], args.arguments[2]);
	}

	return var::undefined();
};

var ScriptingApi::Content::Wrapper::addImage (const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content* thisObject = GET_OBJECT(Content))
	{
		if(args.numArguments == 1)
		{
			return thisObject->addImage(Identifier(args.arguments[0]), 0, 0);
		}

		CHECK_ARGUMENTS("addImage()", 3);

		return thisObject->addImage(Identifier(args.arguments[0]), args.arguments[1], args.arguments[2]);
	}

	return var::undefined();
};

var ScriptingApi::Content::Wrapper::addModulatorMeter (const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content* thisObject = GET_OBJECT(Content))
	{
		if(args.numArguments == 1)
		{
			return thisObject->addModulatorMeter(Identifier(args.arguments[0]), 0, 0);
		}

		CHECK_ARGUMENTS("addModulatorMeter()", 3);

		return thisObject->addModulatorMeter(Identifier(args.arguments[0]), args.arguments[1], args.arguments[2]);
	}

	return var::undefined();
};

var ScriptingApi::Content::Wrapper::addPlotter (const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content* thisObject = GET_OBJECT(Content))
	{
		if(args.numArguments == 1)
		{
			return thisObject->addPlotter(Identifier(args.arguments[0]), 0, 0);
		}

		CHECK_ARGUMENTS("addPlotter()", 3);

		return thisObject->addPlotter(Identifier(args.arguments[0]), args.arguments[1], args.arguments[2]);
	}

	return var::undefined();
};


var ScriptingApi::Content::Wrapper::addPanel (const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content* thisObject = GET_OBJECT(Content))
	{
		if(args.numArguments == 1)
		{
			return thisObject->addPanel(Identifier(args.arguments[0]), 0, 0);
		}

		CHECK_ARGUMENTS("addPanel()", 3);

		return thisObject->addPanel(Identifier(args.arguments[0]), args.arguments[1], args.arguments[2]);
	}

	return var::undefined();
};


var ScriptingApi::Content::Wrapper::addAudioWaveform(const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content* thisObject = GET_OBJECT(Content))
	{
		if (args.numArguments == 1)
		{
			return thisObject->addAudioWaveform(Identifier(args.arguments[0]), 0, 0);
		}

		CHECK_ARGUMENTS("addAudioWaveform()", 3);

		return thisObject->addAudioWaveform(Identifier(args.arguments[0]), args.arguments[1], args.arguments[2]);
	}

	return var::undefined();
}

var ScriptingApi::Content::Wrapper::addSliderPack(const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content* thisObject = GET_OBJECT(Content))
	{
		if (args.numArguments == 1)
		{
			return thisObject->addSliderPack(Identifier(args.arguments[0]), 0, 0);
		}

		CHECK_ARGUMENTS("addSliderPack()", 3);

		return thisObject->addSliderPack(Identifier(args.arguments[0]), args.arguments[1], args.arguments[2]);
	}

	return var::undefined();

}

var ScriptingApi::Content::Wrapper::storeAllControlsAsPreset(const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content* thisObject = GET_OBJECT(Content))
	{
		CHECK_ARGUMENTS("storeAllControlsAsPreset()", 1);

		thisObject->storeAllControlsAsPreset(args.arguments[0].toString());
	}

	return var::undefined();
}

var ScriptingApi::Content::Wrapper::restoreAllControlsFromPreset(const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content* thisObject = GET_OBJECT(Content))
	{
		CHECK_ARGUMENTS("restoreAllControlsFromPreset()", 1);

		thisObject->restoreAllControlsFromPreset(args.arguments[0].toString());
	}

	return var::undefined();
}


var ScriptingApi::Content::Wrapper::set (const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content::ScriptComponent* thisObject = GET_OBJECT(Content::ScriptComponent))
	{
		CHECK_ARGUMENTS("set()", 2);

		thisObject->set(args.arguments[0], args.arguments[1]);
	}

	return var::undefined();
};

var ScriptingApi::Content::Wrapper::get (const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content::ScriptComponent* thisObject = GET_OBJECT(Content::ScriptComponent))
	{
		CHECK_ARGUMENTS("get()", 1);

		return thisObject->get(args.arguments[0]);
	}

	return var::undefined();
};

var ScriptingApi::Content::Wrapper::addToMacroControl (const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content::ScriptComponent* thisObject = GET_OBJECT(Content::ScriptComponent))
	{
		CHECK_ARGUMENTS("addToMacroControl()", 1);

		thisObject->addToMacroControl(args.arguments[0]);
	}

	return var::undefined();
};

var ScriptingApi::Content::Wrapper::setHeight (const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content* thisObject = GET_OBJECT(Content))
	{
		CHECK_ARGUMENTS("setHeight()", 1);

		thisObject->setHeight((int)args.arguments[0]);
	}

	return var::undefined();
};

var ScriptingApi::Content::Wrapper::setContentTooltip (const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content* thisObject = GET_OBJECT(Content))
	{
		CHECK_ARGUMENTS("setContentTooltip()", 1);

		thisObject->setContentTooltip(args.arguments[0].toString());
	}

	return var::undefined();
};


var ScriptingApi::Content::Wrapper::setToolbarProperties(const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content* thisObject = GET_OBJECT(Content))
	{
		CHECK_ARGUMENTS("setToolbarProperties()", 1);

		thisObject->setToolbarProperties(args.arguments[0]);
	}

	return var::undefined();
}


var ScriptingApi::Content::Wrapper::setWidth (const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content* thisObject = GET_OBJECT(Content))
	{
		CHECK_ARGUMENTS("setWidth()", 1);

		thisObject->setWidth((int)args.arguments[0]);
	}

	return var::undefined();
};

var ScriptingApi::Content::Wrapper::setName (const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content* thisObject = GET_OBJECT(Content))
	{
		CHECK_ARGUMENTS("setName()", 1);

		thisObject->setName(args.arguments[0].toString());
	}

	return var::undefined();
};

var ScriptingApi::Content::Wrapper::setPropertiesFromJSON (const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content* thisObject = GET_OBJECT(Content))
	{
		CHECK_ARGUMENTS("setPropertiesFromJSON()", 1);

		thisObject->setPropertiesFromJSON(Identifier(args.arguments[0]), args.arguments[1]);
	}

	return var::undefined();
};

var ScriptingApi::Content::Wrapper::setColour (const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content* thisObject = GET_OBJECT(Content))
	{
		CHECK_ARGUMENTS("setColour()", 3);

		thisObject->setColour((int)args.arguments[0], (int)args.arguments[1], (int)args.arguments[2]);
	}
	else if (ScriptingApi::Content::ScriptComponent* thisObject = GET_OBJECT(Content::ScriptComponent))
	{
		CHECK_ARGUMENTS("setColour()", 2);

		thisObject->setColour((int)args.arguments[0], (int)args.arguments[1]);
	}

	return var::undefined();
};



// =================================================================================================== Content Component Wrappers

var ScriptingApi::Content::Wrapper::addModulatorToPlotter (const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content::ScriptedPlotter* thisObject = GET_OBJECT(Content::ScriptedPlotter))
	{
		CHECK_ARGUMENTS("addModulatorToPlotter()", 2);

		thisObject->addModulatorToPlotter(args.arguments[0].toString(), args.arguments[1].toString());
	}

	return var::undefined();
};

var ScriptingApi::Content::Wrapper::clearModulatorToPlotter (const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content::ScriptedPlotter* thisObject = GET_OBJECT(Content::ScriptedPlotter))
	{
		CHECK_ARGUMENTS("addModulatorToPlotter()", 0);

		thisObject->clearModulatorPlotter();
	}

	return var::undefined();
};

var ScriptingApi::Content::Wrapper::addItem (const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content::ScriptComboBox* thisObject = GET_OBJECT(Content::ScriptComboBox))
	{
		CHECK_ARGUMENTS("addItem()", 1);

		thisObject->addItem(args.arguments[0].toString());
	}

	return var::undefined();
};

var ScriptingApi::Content::Wrapper::setTooltip (const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content::ScriptComponent* thisObject = GET_OBJECT(Content::ScriptComponent))
	{
		CHECK_ARGUMENTS("setTooltip()", 1);

		thisObject->setTooltip(args.arguments[0].toString());
	}

	return var::undefined();
};



var ScriptingApi::Content::Wrapper::setImageFile (const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content::ScriptImage* thisObject = GET_OBJECT(Content::ScriptImage))
	{
		CHECK_ARGUMENTS("setImageFile()", 2);

		thisObject->setImageFile(args.arguments[0].toString(), (bool)args.arguments[1]);
	}

	return var::undefined();
};

var ScriptingApi::Content::Wrapper::setImageAlpha (const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content::ScriptImage* thisObject = GET_OBJECT(Content::ScriptImage))
	{
		CHECK_ARGUMENTS("setAlpha()", 1);

		thisObject->setAlpha((float)args.arguments[0]);
	}

	return var::undefined();
};

var ScriptingApi::Content::Wrapper::setRange (const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content::ScriptSlider* thisObject = GET_OBJECT(Content::ScriptSlider))
	{
		CHECK_ARGUMENTS("setRange()", 3);

		thisObject->setRange((double)args.arguments[0], (double)args.arguments[1], (double)args.arguments[2] );
	}

	return var::undefined();
};

var ScriptingApi::Content::Wrapper::setMode (const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content::ScriptSlider* thisObject = GET_OBJECT(Content::ScriptSlider))
	{
		CHECK_ARGUMENTS("setMode()", 1);

		thisObject->setMode(args.arguments[0].toString());
	}

	return var::undefined();
};

var ScriptingApi::Content::Wrapper::setStyle (const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content::ScriptSlider* thisObject = GET_OBJECT(Content::ScriptSlider))
	{
		CHECK_ARGUMENTS("setStyle()", 1);

		thisObject->setStyle(args.arguments[0].toString());
	}

	return var::undefined();
};

var ScriptingApi::Content::Wrapper::setMidPoint (const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content::ScriptSlider* thisObject = GET_OBJECT(Content::ScriptSlider))
	{
		CHECK_ARGUMENTS("setMidPoint()", 1);

		thisObject->setMidPoint((double)args.arguments[0]);
	}

	return var::undefined();
};


var ScriptingApi::Content::Wrapper::setEditable (const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content::ScriptLabel* thisObject = GET_OBJECT(Content::ScriptLabel))
	{
		CHECK_ARGUMENTS("setEditable()", 1);

		thisObject->setEditable((bool)args.arguments[0]);
	}

	return var::undefined();
};



var ScriptingApi::Content::Wrapper::setValue (const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content::ScriptComponent* thisObject = GET_OBJECT(Content::ScriptComponent))
	{
		CHECK_ARGUMENTS("setValue()", 1);

		thisObject->setValue(args.arguments[0]);
	}

	return var::undefined();
};



var ScriptingApi::Content::Wrapper::setValueNormalized(const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content::ScriptComponent* thisObject = GET_OBJECT(Content::ScriptComponent))
	{
		CHECK_ARGUMENTS("setValueNormalized()", 1);

		thisObject->setValueNormalized((double)args.arguments[0]);
	}

	return var::undefined();
}



var ScriptingApi::Content::Wrapper::getValueNormalized(const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content::ScriptComponent* thisObject = GET_OBJECT(Content::ScriptComponent))
	{
		CHECK_ARGUMENTS("getValueNormalized()", 0);

		return thisObject->getValueNormalized();
	}

	return var::undefined();
}


var ScriptingApi::Content::Wrapper::setPosition (const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content::ScriptComponent* thisObject = GET_OBJECT(Content::ScriptComponent))
	{
		CHECK_ARGUMENTS("setPosition()", 4);

		thisObject->setPosition((int)args.arguments[0], (int)args.arguments[1], (int)args.arguments[2], (int)args.arguments[3]);
	}

	return var::undefined();
};


var ScriptingApi::Content::Wrapper::getItemText (const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content::ScriptComboBox* thisObject = GET_OBJECT(Content::ScriptComboBox))
	{
		CHECK_ARGUMENTS("setItemText()", 0);

		return thisObject->getItemText();
	}

	return var::undefined();
};



var ScriptingApi::Content::Wrapper::showControl (const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content::ScriptComponent* thisObject = GET_OBJECT(Content::ScriptComponent))
	{
		CHECK_ARGUMENTS("showControl()", 1);

		thisObject->showControl((bool)args.arguments[0]);
	}

	return var::undefined();
};



var ScriptingApi::Content::Wrapper::getValue (const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content::ScriptComponent* thisObject = GET_OBJECT(Content::ScriptComponent))
	{
		CHECK_ARGUMENTS("getValue()", 0);

		return thisObject->getValue();
	}

	return var::undefined();
};

var ScriptingApi::Content::Wrapper::getTableValue (const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content::ScriptTable* thisObject = GET_OBJECT(Content::ScriptTable))
	{
		CHECK_ARGUMENTS("getTableValue()", 1);

		return thisObject->getTableValue((int)args.arguments[0]);
	}

	return var::undefined();
};

var ScriptingApi::Content::Wrapper::connectToOtherTable (const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content::ScriptTable* thisObject = GET_OBJECT(Content::ScriptTable))
	{
		CHECK_ARGUMENTS("connectToOtherTable()", 2);

		thisObject->connectToOtherTable(args.arguments[0].toString(), (int)args.arguments[1]);
	}

	return var::undefined();
};

var ScriptingApi::Content::Wrapper::connectToAudioSampleProcessor(const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content::ScriptAudioWaveform* thisObject = GET_OBJECT(Content::ScriptAudioWaveform))
	{
		CHECK_ARGUMENTS("connectToAudioSampleProcessor()", 1);

		thisObject->connectToAudioSampleProcessor(args.arguments[0].toString());
	}

	return var::undefined();
}


var ScriptingApi::Content::Wrapper::clear (const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content* thisObject = dynamic_cast<ScriptingApi::Content*> (args.thisObject.getObject()))
	{
		thisObject->clear();
	}

	return var::undefined();
};

var ScriptingApi::Content::Wrapper::setSliderAtIndex(const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content::ScriptSliderPack* thisObject = GET_OBJECT(Content::ScriptSliderPack))
	{
		CHECK_ARGUMENTS("setSliderAtIndex()", 2);

		thisObject->setSliderAtIndex((int)args.arguments[0], (double)args.arguments[1]);
	}

	return var::undefined();
}

var ScriptingApi::Content::Wrapper::getSliderValueAt(const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content::ScriptSliderPack* thisObject = GET_OBJECT(Content::ScriptSliderPack))
	{
		CHECK_ARGUMENTS("getSliderValueAt()", 1);

		return thisObject->getSliderValueAt((int)args.arguments[0]);
	}

	return var::undefined();
}

var ScriptingApi::Content::Wrapper::setAllValues(const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content::ScriptSliderPack* thisObject = GET_OBJECT(Content::ScriptSliderPack))
	{
		CHECK_ARGUMENTS("setAllValues()", 1);

		thisObject->setAllValues((double)args.arguments[0]);
	}

	return var::undefined();
}

var ScriptingApi::Content::Wrapper::getNumSliders(const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content::ScriptSliderPack* thisObject = GET_OBJECT(Content::ScriptSliderPack))
	{
		CHECK_ARGUMENTS("getNumSliders()", 0);

		return thisObject->getNumSliders();
	}

	return var::undefined();
}


var ScriptingApi::Content::Wrapper::setMinValue(const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content::ScriptSlider* thisObject = GET_OBJECT(Content::ScriptSlider))
	{
		CHECK_ARGUMENTS("setMinValue()", 1);

		thisObject->setMinValue((double)args.arguments[0]);
	}

	return var::undefined();
}

var ScriptingApi::Content::Wrapper::setMaxValue(const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content::ScriptSlider* thisObject = GET_OBJECT(Content::ScriptSlider))
	{
		CHECK_ARGUMENTS("setMaxValue()", 1);

		thisObject->setMaxValue((double)args.arguments[0]);
	}

	return var::undefined();
}

var ScriptingApi::Content::Wrapper::getMinValue(const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content::ScriptSlider* thisObject = GET_OBJECT(Content::ScriptSlider))
	{
		CHECK_ARGUMENTS("getMinValue()", 0);

		return thisObject->getMinValue();
	}

	return var::undefined();
}

var ScriptingApi::Content::Wrapper::getMaxValue(const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content::ScriptSlider* thisObject = GET_OBJECT(Content::ScriptSlider))
	{
		CHECK_ARGUMENTS("getMaxValue()", 0);

		return thisObject->getMaxValue();
	}

	return var::undefined();
}

var ScriptingApi::Content::Wrapper::contains(const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content::ScriptSlider* thisObject = GET_OBJECT(Content::ScriptSlider))
	{
		CHECK_ARGUMENTS("contains()", 1);

		return thisObject->contains((double)args.arguments[0]);
	}

	return var::undefined();
}



// =================================================================================================== Content Wrappers


var ScriptingObjects::ScriptingModulator::Wrapper::setBypassed (const var::NativeFunctionArgs& args)
{
	if (ScriptingObjects::ScriptingModulator* thisObject = dynamic_cast<ScriptingObjects::ScriptingModulator*> (args.thisObject.getObject()))
	{
		CHECK_ARGUMENTS("setBypassed()", 1);

		thisObject->setBypassed((bool)args.arguments[0]);
	}

	return var::undefined();
};

var ScriptingObjects::ScriptingModulator::Wrapper::setAttribute (const var::NativeFunctionArgs& args)
{
	if (ScriptingObjects::ScriptingModulator* thisObject = dynamic_cast<ScriptingObjects::ScriptingModulator*> (args.thisObject.getObject()))
	{
		CHECK_ARGUMENTS("setBypassed()", 2);

		thisObject->setAttribute((int)args.arguments[0], (float)args.arguments[1]);
	}

	return var::undefined();
};


var ScriptingObjects::ScriptingModulator::Wrapper::setIntensity (const var::NativeFunctionArgs& args)
{
	if (ScriptingObjects::ScriptingModulator* thisObject = dynamic_cast<ScriptingObjects::ScriptingModulator*> (args.thisObject.getObject()))
	{
		CHECK_ARGUMENTS("setIntensity()", 1);

		thisObject->setIntensity((float)args.arguments[0]);
	}

	return var::undefined();
};

var ScriptingObjects::ScriptingEffect::Wrapper::setBypassed (const var::NativeFunctionArgs& args)
{
	if (ScriptingObjects::ScriptingEffect* thisObject = dynamic_cast<ScriptingObjects::ScriptingEffect*> (args.thisObject.getObject()))
	{
		CHECK_ARGUMENTS("setBypassed()", 1);

		thisObject->setBypassed((bool)args.arguments[0]);
	}

	return var::undefined();
};

var ScriptingObjects::ScriptingEffect::Wrapper::setAttribute (const var::NativeFunctionArgs& args)
{
	if (ScriptingObjects::ScriptingEffect* thisObject = dynamic_cast<ScriptingObjects::ScriptingEffect*> (args.thisObject.getObject()))
	{
		CHECK_ARGUMENTS("setBypassed()", 2);

		thisObject->setAttribute((int)args.arguments[0], (float)args.arguments[1]);
	}

	return var::undefined();
};


var ScriptingObjects::ScriptingMidiProcessor::Wrapper::setBypassed (const var::NativeFunctionArgs& args)
{
	if (ScriptingObjects::ScriptingMidiProcessor* thisObject = dynamic_cast<ScriptingObjects::ScriptingMidiProcessor*> (args.thisObject.getObject()))
	{
		CHECK_ARGUMENTS("setBypassed()", 1);

		thisObject->setBypassed((bool)args.arguments[0]);
	}

	return var::undefined();
};

var ScriptingObjects::ScriptingMidiProcessor::Wrapper::setAttribute (const var::NativeFunctionArgs& args)
{
	if (ScriptingObjects::ScriptingMidiProcessor* thisObject = dynamic_cast<ScriptingObjects::ScriptingMidiProcessor*> (args.thisObject.getObject()))
	{
		CHECK_ARGUMENTS("setBypassed()", 2);

		thisObject->setAttribute((int)args.arguments[0], (float)args.arguments[1]);
	}

	return var::undefined();
};


var ScriptingObjects::ScriptingSynth::Wrapper::setBypassed (const var::NativeFunctionArgs& args)
{
	if (ScriptingObjects::ScriptingSynth* thisObject = dynamic_cast<ScriptingObjects::ScriptingSynth*> (args.thisObject.getObject()))
	{
		CHECK_ARGUMENTS("setBypassed()", 1);

		thisObject->setBypassed((bool)args.arguments[0]);
	}

	return var::undefined();
};

var ScriptingObjects::ScriptingSynth::Wrapper::setAttribute (const var::NativeFunctionArgs& args)
{
	if (ScriptingObjects::ScriptingSynth* thisObject = dynamic_cast<ScriptingObjects::ScriptingSynth*> (args.thisObject.getObject()))
	{
		CHECK_ARGUMENTS("setBypassed()", 2);

		thisObject->setAttribute((int)args.arguments[0], (float)args.arguments[1]);
	}

	return var::undefined();
};


var ScriptingObjects::ScriptingAudioSampleProcessor::Wrapper::setBypassed(const var::NativeFunctionArgs& args)
{
	if (ScriptingObjects::ScriptingAudioSampleProcessor* thisObject = dynamic_cast<ScriptingObjects::ScriptingAudioSampleProcessor*> (args.thisObject.getObject()))
	{
		CHECK_ARGUMENTS("setBypassed()", 1);

		thisObject->setBypassed((bool)args.arguments[0]);
	}

	return var::undefined();
};

var ScriptingObjects::ScriptingAudioSampleProcessor::Wrapper::setAttribute(const var::NativeFunctionArgs& args)
{
	if (ScriptingObjects::ScriptingAudioSampleProcessor* thisObject = dynamic_cast<ScriptingObjects::ScriptingAudioSampleProcessor*> (args.thisObject.getObject()))
	{
		CHECK_ARGUMENTS("setBypassed()", 2);

		thisObject->setAttribute((int)args.arguments[0], (float)args.arguments[1]);
	}

	return var::undefined();
};


var ScriptingObjects::ScriptingAudioSampleProcessor::Wrapper::getSampleLength(const var::NativeFunctionArgs& args)
{
	if (ScriptingObjects::ScriptingAudioSampleProcessor *thisObject = dynamic_cast<ScriptingObjects::ScriptingAudioSampleProcessor*> (args.thisObject.getObject()))
	{
		CHECK_ARGUMENTS("getSampleLength()", 0);

		return thisObject->getSampleLength();
	}

	return var::undefined();
}


var ScriptingObjects::ScriptingAudioSampleProcessor::Wrapper::setFile(const var::NativeFunctionArgs& args)
{
	if (ScriptingObjects::ScriptingAudioSampleProcessor *thisObject = dynamic_cast<ScriptingObjects::ScriptingAudioSampleProcessor*> (args.thisObject.getObject()))
	{
		CHECK_ARGUMENTS("setFile()", 1);

		thisObject->setFile(args.arguments[0].toString());
	}

	return var::undefined();
}

var ScriptingObjects::ScriptingAudioSampleProcessor::Wrapper::setSampleRange(const var::NativeFunctionArgs& args)
{
    if (ScriptingObjects::ScriptingAudioSampleProcessor *thisObject = dynamic_cast<ScriptingObjects::ScriptingAudioSampleProcessor*> (args.thisObject.getObject()))
    {
        CHECK_ARGUMENTS("setSampleRange()", 2);
        
        thisObject->setSampleRange((int)args.arguments[0], (int)args.arguments[1]);
    }
    
    return var::undefined();
}


var ScriptingObjects::MidiList::Wrapper::fill(const var::NativeFunctionArgs& args)
{
    if (ScriptingObjects::MidiList *thisObject = dynamic_cast<ScriptingObjects::MidiList*> (args.thisObject.getObject()))
    {
        CHECK_ARGUMENTS("fill", 1);
        
        thisObject->fill((int)args.arguments[0]);
    }
    
    return var::undefined();
}


var ScriptingObjects::MidiList::Wrapper::clear(const var::NativeFunctionArgs& args)
{
    if (ScriptingObjects::MidiList *thisObject = dynamic_cast<ScriptingObjects::MidiList*> (args.thisObject.getObject()))
    {
        CHECK_ARGUMENTS("clear", 0);
        
        thisObject->clear();
    }
    
    return var::undefined();
}

var ScriptingObjects::MidiList::Wrapper::getValue(const var::NativeFunctionArgs& args)
{
    if (ScriptingObjects::MidiList *thisObject = dynamic_cast<ScriptingObjects::MidiList*> (args.thisObject.getObject()))
    {
        CHECK_ARGUMENTS("getValue", 1);
        
        return thisObject->getValue((int)args.arguments[0]);
    }
    
    return var::undefined();
}

var ScriptingObjects::MidiList::Wrapper::getValueAmount(const var::NativeFunctionArgs& args)
{
    if (ScriptingObjects::MidiList *thisObject = dynamic_cast<ScriptingObjects::MidiList*> (args.thisObject.getObject()))
    {
        CHECK_ARGUMENTS("getValueAmount", 1);
        
        return thisObject->getValueAmount((int)args.arguments[0]);
    }
    
    return var::undefined();
}

var ScriptingObjects::MidiList::Wrapper::getIndex(const var::NativeFunctionArgs& args)
{
    if (ScriptingObjects::MidiList *thisObject = dynamic_cast<ScriptingObjects::MidiList*> (args.thisObject.getObject()))
    {
        CHECK_ARGUMENTS("getIndex", 1);
        
        return thisObject->getIndex((int)args.arguments[0]);
    }
    
    return var::undefined();
}

var ScriptingObjects::MidiList::Wrapper::isEmpty(const var::NativeFunctionArgs& args)
{
    if (ScriptingObjects::MidiList *thisObject = dynamic_cast<ScriptingObjects::MidiList*> (args.thisObject.getObject()))
    {
        CHECK_ARGUMENTS("isEmpty", 0);
        
        return thisObject->isEmpty();
    }
    
    return var::undefined();
}

var ScriptingObjects::MidiList::Wrapper::getNumSetValues(const var::NativeFunctionArgs& args)
{
    if (ScriptingObjects::MidiList *thisObject = dynamic_cast<ScriptingObjects::MidiList*> (args.thisObject.getObject()))
    {
        CHECK_ARGUMENTS("getNumSetValues", 0);
        
        return thisObject->getNumSetValues();
    }
    
    return var::undefined();
}

var ScriptingObjects::MidiList::Wrapper::setValue(const var::NativeFunctionArgs& args)
{
    if (ScriptingObjects::MidiList *thisObject = dynamic_cast<ScriptingObjects::MidiList*> (args.thisObject.getObject()))
    {
        CHECK_ARGUMENTS("setValue", 1);
        
        thisObject->setValue((int)args.arguments[0], (int)args.arguments[1]);
    }
    
    return var::undefined();
}


var ScriptingObjects::MidiList::Wrapper::getBase64String(const var::NativeFunctionArgs& args)
{
	if (ScriptingObjects::MidiList *thisObject = dynamic_cast<ScriptingObjects::MidiList*> (args.thisObject.getObject()))
	{
		CHECK_ARGUMENTS("getBase64String", 0);

		return thisObject->getBase64String();
	}

	return var::undefined();
}

var ScriptingObjects::MidiList::Wrapper::restoreFromBase64String(const var::NativeFunctionArgs& args)
{
	if (ScriptingObjects::MidiList *thisObject = dynamic_cast<ScriptingObjects::MidiList*> (args.thisObject.getObject()))
	{
		CHECK_ARGUMENTS("restoreFromBase64String", 1);

		thisObject->restoreFromBase64String(args.arguments[0].toString());
	}

	return var::undefined();
}


#undef GET_OBJECT

#undef CHECK_ARGUMENTS

#undef CHECK_IF_DEFERRED