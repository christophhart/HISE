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

#if USE_BACKEND

ApiHelpers::Api::Api()
{
	apiTree = ValueTree(ValueTree::readFromData(XmlApi::apivaluetree_dat, XmlApi::apivaluetree_datSize));
}

void ApiHelpers::getColourAndCharForType(int type, char &c, Colour &colour)
{

	const float alpha = 0.6f;
	const float brightness = 0.8f;


	switch (type)
	{
	case (int)DebugInformation::Type::InlineFunction:	c = 'I'; break;
	case (int)DebugInformation::Type::Callback:			c = 'F'; break;
	case (int)DebugInformation::Type::Variables:		c = 'V'; break;
	case (int)DebugInformation::Type::Globals:			c = 'G'; break;
	case (int)DebugInformation::Type::Constant:			c = 'C'; break;
	case (int)DebugInformation::Type::RegisterVariable:	c = 'R'; break;
	case (int)DebugInformation::Type::ExternalFunction: c = 'E'; break;
	case 7:												c = 'A'; break;
	default:											c = 'V'; break;
	}

	switch (c)
	{
	case 'I': colour = Colours::blue.withAlpha(alpha).withBrightness(brightness); break;
	case 'V': colour = Colours::cyan.withAlpha(alpha).withBrightness(brightness); break;
	case 'G': colour = Colours::green.withAlpha(alpha).withBrightness(brightness); break;
	case 'C': colour = Colours::yellow.withAlpha(alpha).withBrightness(brightness); break;
	case 'R': colour = Colours::red.withAlpha(alpha).withBrightness(brightness); break;
	case 'A': colour = Colours::orange.withAlpha(alpha).withBrightness(brightness); break;
	case 'F': colour = Colours::purple.withAlpha(alpha).withBrightness(brightness); break;
	case 'E': colour = Colours::chocolate.withAlpha(alpha).withBrightness(brightness); break;

	}
}



String ApiHelpers::getValueType(const var &v)
{
	const bool isObject = v.isObject();
	const bool isCreatableScriptObject = dynamic_cast<DynamicScriptingObject*>(v.getDynamicObject()) != nullptr;

	if (v.isBool()) return "bool";
	else if (v.isInt() || v.isInt64()) return "int";
	else if (v.isDouble()) return "double";
	else if (v.isString()) return "String";
	else if (v.isArray()) return "Array";
	else if (v.isMethod()) return "Function";
	else if (isObject && isCreatableScriptObject)
	{
		DynamicScriptingObject * obj = dynamic_cast<DynamicScriptingObject*>(v.getDynamicObject());

		if (obj != nullptr) return obj->getObjectName().toString();
		else return String::empty;
	}
	else return String::empty;
}


AttributedString ApiHelpers::createAttributedStringFromApi(const ValueTree &method, const String &/*className*/, bool multiLine, Colour textColour)
{
	AttributedString help;

	const String name = method.getProperty(Identifier("name")).toString();
	const String arguments = method.getProperty(Identifier("arguments")).toString();
	const String description = method.getProperty(Identifier("description")).toString();
	const String returnType = method.getProperty("returnType", "void");


	help.setWordWrap(AttributedString::byWord);


	if (multiLine)
	{
		help.setJustification(Justification::topLeft);
		help.setLineSpacing(1.5f);
		help.append("Name:\n  ", GLOBAL_BOLD_FONT(), textColour);
		help.append(name, GLOBAL_MONOSPACE_FONT(), textColour.withAlpha(0.8f));
		help.append(arguments + "\n\n", GLOBAL_MONOSPACE_FONT(), textColour.withAlpha(0.6f));
		help.append("Description:\n  ", GLOBAL_BOLD_FONT(), textColour);
		help.append(description + "\n\n", GLOBAL_FONT(), textColour.withAlpha(0.8f));

		help.append("Return Type:\n  ", GLOBAL_BOLD_FONT(), textColour);
		help.append(method.getProperty("returnType", "void"), GLOBAL_MONOSPACE_FONT(), textColour.withAlpha(0.8f));
	}

	else
	{
		help.setJustification(Justification::centredLeft);
		help.append(description, GLOBAL_BOLD_FONT(), textColour.withAlpha(0.8f));

		const String returnType = method.getProperty("returnType", "");

		if (returnType.isNotEmpty())
		{
			help.append("\nReturn Type: ", GLOBAL_BOLD_FONT(), textColour);
			help.append(returnType, GLOBAL_MONOSPACE_FONT(), textColour.withAlpha(0.8f));
		}
	}

	return help;
}



String ApiHelpers::createCodeToInsert(const ValueTree &method, const String &className)
{
	const String name = method.getProperty(Identifier("name")).toString();

	if (name == "setMouseCallback")
	{
		const String argumentName = "event";
		String functionDef = className;
		functionDef << "." << name + "(function(" << argumentName << ")\n";
		functionDef << "{\n\t\n});\n";

		return functionDef;
	}
	else if (name == "setTimerCallback")
	{
		const String argumentName = "";
		String functionDef = className;
		functionDef << "." << name + "(function(" << argumentName << ")\n";
		functionDef << "{\n\t\n});\n";

		return functionDef;
	}
	else if (name == "setPaintRoutine")
	{
		const String argumentName = "g";
		String functionDef = className;
		functionDef << "." << name + "(function(" << argumentName << ")\n";
		functionDef << "{\n\t\n});\n";

		return functionDef;
	}
	else
	{
		const String arguments = method.getProperty(Identifier("arguments")).toString();

		return String(className + "." + name + arguments);
	}
}

#endif


String ApiHelpers::getFileNameFromErrorMessage(const String &message)
{
	if (message.startsWith("Line")) return String();

	String fileName = message.upToFirstOccurrenceOf("-", false, true);

	fileName = fileName.trimEnd();

	return fileName;
}



Rectangle<float> ApiHelpers::getRectangleFromVar(const var &data, Result *r/*=nullptr*/)
{
	if (data.isArray())
	{
		Array<var>* d = data.getArray();

		if (d->size() == 4)
		{
			Rectangle<float> rectangle((float)d->getUnchecked(0), (float)d->getUnchecked(1), (float)d->getUnchecked(2), (float)d->getUnchecked(3));

			return rectangle;
		}
		else
		{
			if (r != nullptr) *r = Result::fail("Rectangle array needs 4 elements");
			return Rectangle<float>();
		}
	}
	else
	{
		if (r != nullptr) *r = Result::fail("Rectangle data is not an array");
		return Rectangle<float>();
	}
}

Rectangle<int> ApiHelpers::getIntRectangleFromVar(const var &data, Result* r/*=nullptr*/)
{
	if (data.isArray())
	{
		Array<var>* d = data.getArray();

		if (d->size() == 4)
		{
			Rectangle<int> rectangle((int)d->getUnchecked(0), (int)d->getUnchecked(1), (int)d->getUnchecked(2), (int)d->getUnchecked(3));

			return rectangle;
		}
		else
		{
			if (r != nullptr) *r = Result::fail("Rectangle array needs 4 elements");
			return Rectangle<int>();
		}
	}
	else
	{
		if (r != nullptr) *r = Result::fail("Rectangle data is not an array");
		return Rectangle<int>();
	}
}

// ====================================================================================================== Message functions


struct ScriptingApi::Message::Wrapper
{
	API_VOID_METHOD_WRAPPER_1(Message, setNoteNumber);
	API_VOID_METHOD_WRAPPER_1(Message, setVelocity);
	API_VOID_METHOD_WRAPPER_1(Message, setControllerNumber);
	API_VOID_METHOD_WRAPPER_1(Message, setControllerValue);
	API_METHOD_WRAPPER_0(Message, getNoteNumber);
	API_METHOD_WRAPPER_0(Message, getVelocity);
	API_METHOD_WRAPPER_0(Message, getControllerNumber);
	API_METHOD_WRAPPER_0(Message, getControllerValue);
	API_VOID_METHOD_WRAPPER_1(Message, ignoreEvent);
	API_VOID_METHOD_WRAPPER_1(Message, delayEvent);
	API_METHOD_WRAPPER_0(Message, getEventId);
	API_METHOD_WRAPPER_0(Message, getChannel);
	API_VOID_METHOD_WRAPPER_1(Message, setChannel);
	API_VOID_METHOD_WRAPPER_1(Message, setTransposeAmount);
	API_METHOD_WRAPPER_0(Message, getTransposeAmount);
	API_VOID_METHOD_WRAPPER_1(Message, setCoarseDetune);
	API_METHOD_WRAPPER_0(Message, getCoarseDetune);
	API_VOID_METHOD_WRAPPER_1(Message, setFineDetune);
	API_METHOD_WRAPPER_0(Message, getFineDetune);
};


ScriptingApi::Message::Message(ProcessorWithScriptingContent *p) :
ScriptingObject(p),
ApiClass(0),
messageHolder(nullptr),
constMessageHolder(nullptr)
{
	ADD_API_METHOD_1(setNoteNumber);
	ADD_API_METHOD_1(setVelocity);
	ADD_API_METHOD_1(setControllerNumber);
	ADD_API_METHOD_1(setControllerValue);
	ADD_API_METHOD_0(getControllerNumber);
	ADD_API_METHOD_0(getControllerValue);
	ADD_API_METHOD_0(getNoteNumber);
	ADD_API_METHOD_0(getVelocity);
	ADD_API_METHOD_1(ignoreEvent);
	ADD_API_METHOD_1(delayEvent);
	ADD_API_METHOD_0(getEventId);
	ADD_API_METHOD_0(getChannel);
	ADD_API_METHOD_1(setChannel);
	ADD_API_METHOD_1(setTransposeAmount);
	ADD_API_METHOD_0(getTransposeAmount);
	ADD_API_METHOD_1(setCoarseDetune);
	ADD_API_METHOD_0(getCoarseDetune);
	ADD_API_METHOD_1(setFineDetune);
	ADD_API_METHOD_0(getFineDetune);
}


ScriptingApi::Message::~Message()
{
	messageHolder = nullptr;
	constMessageHolder = nullptr;
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

	messageHolder->setVelocity((float)newValue);
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

	messageHolder->setControllerNumber(newValue);
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

	
	messageHolder->setControllerValue(newValue);
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

void ScriptingApi::Message::ignoreEvent(bool shouldBeIgnored/*=true*/)
{
	if (messageHolder == nullptr)
	{
		reportIllegalCall("ignoreEvent()", "midi event");
		return;
	}

	messageHolder->ignoreEvent(shouldBeIgnored);
}

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
#if ENABLE_SCRIPTING_SAFE_CHECKS
	if (constMessageHolder == nullptr)
	{
		reportIllegalCall("getEventId()", "midi event");
		return 0;
	}
#endif

	return constMessageHolder->getEventId();
}

void ScriptingApi::Message::setTransposeAmount(int tranposeValue)
{
#if ENABLE_SCRIPTING_SAFE_CHECKS
	if (messageHolder == nullptr)
	{
		reportIllegalCall("setTransposeAmount()", "midi event");
		return;
	}
#endif

	messageHolder->setTransposeAmount(tranposeValue);

}

int ScriptingApi::Message::getTransposeAmount() const
{
#if ENABLE_SCRIPTING_SAFE_CHECKS
	if (constMessageHolder == nullptr)
	{
		reportIllegalCall("getTransposeAmount()", "midi event");
		return 0;
	}
#endif

	return constMessageHolder->getTransposeAmount();
}

void ScriptingApi::Message::setCoarseDetune(int semiToneDetune)
{
#if ENABLE_SCRIPTING_SAFE_CHECKS
	if (messageHolder == nullptr)
	{
		reportIllegalCall("setCoarseDetune()", "midi event");
		return;
	}
#endif

	messageHolder->setCoarseDetune(semiToneDetune);

}

int ScriptingApi::Message::getCoarseDetune() const
{
#if ENABLE_SCRIPTING_SAFE_CHECKS
	if (constMessageHolder == nullptr)
	{
		reportIllegalCall("getCoarseDetune()", "midi event");
		return 0;
	}
#endif

	return constMessageHolder->getCoarseDetune();

}

void ScriptingApi::Message::setFineDetune(int cents)
{
#if ENABLE_SCRIPTING_SAFE_CHECKS
	if (messageHolder == nullptr)
	{
		reportIllegalCall("setFineDetune()", "midi event");
		return;
	}
#endif

	messageHolder->setFineDetune(cents);
}

int ScriptingApi::Message::getFineDetune() const
{
#if ENABLE_SCRIPTING_SAFE_CHECKS
	if (constMessageHolder == nullptr)
	{
		reportIllegalCall("getFineDetune()", "midi event");
		return 0;
	}
#endif

	return constMessageHolder->getFineDetune();
}

void ScriptingApi::Message::setMidiMessage(MidiMessage *m)
{
	jassertfalse;

#if 0

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
        
    }
	else
	{
		currentEventId = -1;
	}
#endif
}

void ScriptingApi::Message::setHiseEvent(HiseEvent &m)
{
	messageHolder = &m;
	constMessageHolder = messageHolder;
}

void ScriptingApi::Message::setHiseEvent(const HiseEvent& m)
{
	messageHolder = nullptr;
	constMessageHolder = &m;
}

// ====================================================================================================== Engine functions

struct ScriptingApi::Engine::Wrapper
{
	API_VOID_METHOD_WRAPPER_0(Engine, allNotesOff);
	API_METHOD_WRAPPER_0(Engine, getUptime);
	API_METHOD_WRAPPER_0(Engine, getHostBpm);
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
	API_METHOD_WRAPPER_0(Engine, getPlayHead);
	API_VOID_METHOD_WRAPPER_2(Engine, dumpAsJSON);
	API_METHOD_WRAPPER_1(Engine, loadFromJSON);
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
	ADD_API_METHOD_0(getPlayHead);
	ADD_API_METHOD_2(dumpAsJSON);
	ADD_API_METHOD_1(loadFromJSON);
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

	ignoreUnused(fileName);

#endif
}

double ScriptingApi::Engine::getSampleRate() const { return const_cast<MainController*>(getProcessor()->getMainController())->getMainSynthChain()->getSampleRate(); }
double ScriptingApi::Engine::getSamplesForMilliSeconds(double milliSeconds) const { return (milliSeconds / 1000.0) * getSampleRate(); }


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



void ScriptingApi::Engine::setKeyColour(int keyNumber, int colourAsHex) { getProcessor()->getMainController()->setKeyboardCoulour(keyNumber, Colour(colourAsHex));}
void ScriptingApi::Engine::setLowestKeyToDisplay(int keyNumber) { getProcessor()->getMainController()->setLowestKeyToDisplay(keyNumber); }
double ScriptingApi::Engine::getMilliSecondsForTempo(int tempoIndex) const { return (double)TempoSyncer::getTempoInMilliSeconds(getHostBpm(), (TempoSyncer::Tempo)tempoIndex); }


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
    API_VOID_METHOD_WRAPPER_2(Sampler, setAttribute);
    API_METHOD_WRAPPER_1(Sampler, getAttribute);
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
    ADD_API_METHOD_1(getAttribute);
    ADD_API_METHOD_2(setAttribute);

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
    if(fileName.isEmpty()) return;
    
    
	ModulatorSampler *s = static_cast<ModulatorSampler*>(sampler.get());

#if USE_BACKEND

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
		return;
	}

#else

	ValueTree v = dynamic_cast<FrontendProcessor*>(sampler->getMainController())->getSampleMap(fileName);

	if (v.isValid())
	{
		static const Identifier unused = Identifier("unused");
		const Identifier oldId = s->getSampleMap()->getId();
		const Identifier newId = Identifier(v.getProperty("ID", "unused").toString());

		if (newId != unused && newId != oldId)
		{
			s->loadSampleMap(v);
		}
	}
	else
	{
		reportScriptError("Error when loading sample map: " + fileName);
		return;
	}

#endif


	int maxGroup = 1;

	for (int i = 0; i < s->getNumSounds(); i++)
	{
		maxGroup = jmax<int>(maxGroup, s->getSound(i)->getProperty(ModulatorSamplerSound::RRGroup));
	}

	s->setAttribute(ModulatorSampler::RRGroupAmount, (float)maxGroup, sendNotification);
}


var ScriptingApi::Sampler::getAttribute(int index) const
{
    ModulatorSampler *s = static_cast<ModulatorSampler*>(sampler.get());
    
    if (s == nullptr)
    {
        reportScriptError("loadSampleMap() only works with Samplers.");
        return var::undefined();
    }
    
    return s->getAttribute(index);
}

void ScriptingApi::Sampler::setAttribute(int index, var newValue)
{
    ModulatorSampler *s = static_cast<ModulatorSampler*>(sampler.get());
    
    if (s == nullptr)
    {
        reportScriptError("loadSampleMap() only works with Samplers.");
        return;
    }
    
    s->setAttribute(index, newValue, sendNotification);
}

// ====================================================================================================== Synth functions



struct ScriptingApi::Synth::Wrapper
{
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

	int timestamp = dynamic_cast<ScriptBaseMidiProcessor*>(getProcessor())->getCurrentHiseEvent()->isNoteOff() ? 1 : 0;

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

	HiseEvent m = HiseEvent(HiseEvent::Type::Controller, controllerNumber, controllerValue, 1);

	owner->gainChain->handleHiseEvent(m);
	owner->pitchChain->handleHiseEvent(m);
	owner->effectChain->handleHiseEvent(m);
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
				return new ScriptingObjects::ScriptingModulator(getScriptProcessor(), m);	
			}
		}

		reportScriptError(name + " was not found. ");

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
						HiseEvent m = HiseEvent(HiseEvent::Type::NoteOn, noteNumber, velocity, channel);
						m.setTimeStamp(sp->getCurrentHiseEvent()->getTimeStamp() + timeStampSamples);
						m.setArtificial();

						sp->addHiseEventToBuffer(m);
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

					HiseEvent m = HiseEvent(HiseEvent::Type::NoteOff, noteNumber, 127, channel);
					m.setTimeStamp(sp->getCurrentHiseEvent()->getTimeStamp() + timeStampSamples);
					m.setArtificial();

					sp->addHiseEventToBuffer(m);

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
						HiseEvent m = HiseEvent(HiseEvent::Type::Controller, number, value, channel);
						m.setTimeStamp(sp->getCurrentHiseEvent()->getTimeStamp() + timeStampSamples);
						m.setArtificial();

						sp->addHiseEventToBuffer(m);
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
	API_VOID_METHOD_WRAPPER_0(Console, start);
	API_VOID_METHOD_WRAPPER_0(Console, stop);
	API_VOID_METHOD_WRAPPER_0(Console, clear);
};

ScriptingApi::Console::Console(ProcessorWithScriptingContent *p) :
ScriptingObject(p),
ApiClass(0),
startTime(0.0)
{
	ADD_API_METHOD_1(print);
	ADD_API_METHOD_0(start);
	ADD_API_METHOD_0(stop);
	ADD_API_METHOD_0(clear);
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

	debugToConsole(getProcessor(), "Benchmark Result: " + String(ms, 3) + " ms");
#endif
}





void ScriptingApi::Console::clear()
{
	getProcessor()->getMainController()->clearConsole();
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

struct ScriptingApi::Colours::Wrapper
{
	API_METHOD_WRAPPER_2(Colours, withAlpha);
};

ScriptingApi::Colours::Colours() :
ApiClass(139)
{
	addConstant("transparentBlack", (int64)0);
	addConstant("transparentWhite", (int64)0x00ffffff);
	addConstant("aliceblue", (int64)0xfff0f8ff);
	addConstant("antiquewhite", (int64)0xfffaebd7);
	addConstant("aqua", (int64)0xff00ffff);
	addConstant("aquamarine", (int64)0xff7fffd4);
	addConstant("azure", (int64)0xfff0ffff);
	addConstant("beige", (int64)0xfff5f5dc);
	addConstant("bisque", (int64)0xffffe4c4);
	addConstant("black", (int64)0xff000000);
	addConstant("blanchedalmond", (int64)0xffffebcd);
	addConstant("blue", (int64)0xff0000ff);
	addConstant("blueviolet", (int64)0xff8a2be2);
	addConstant("brown", (int64)0xffa52a2a);
	addConstant("burlywood", (int64)0xffdeb887);
	addConstant("cadetblue", (int64)0xff5f9ea0);
	addConstant("chartreuse", (int64)0xff7fff00);
	addConstant("chocolate", (int64)0xffd2691e);
	addConstant("coral", (int64)0xffff7f50);
	addConstant("cornflowerblue", (int64)0xff6495ed);
	addConstant("cornsilk", (int64)0xfffff8dc);
	addConstant("crimson", (int64)0xffdc143c);
	addConstant("cyan", (int64)0xff00ffff);
	addConstant("darkblue", (int64)0xff00008b);
	addConstant("darkcyan", (int64)0xff008b8b);
	addConstant("darkgoldenrod", (int64)0xffb8860b);
	addConstant("darkgrey", (int64)0xff555555);
	addConstant("darkgreen", (int64)0xff006400);
	addConstant("darkkhaki", (int64)0xffbdb76b);
	addConstant("darkmagenta", (int64)0xff8b008b);
	addConstant("darkolivegreen", (int64)0xff556b2f);
	addConstant("darkorange", (int64)0xffff8c00);
	addConstant("darkorchid", (int64)0xff9932cc);
	addConstant("darkred", (int64)0xff8b0000);
	addConstant("darksalmon", (int64)0xffe9967a);
	addConstant("darkseagreen", (int64)0xff8fbc8f);
	addConstant("darkslateblue", (int64)0xff483d8b);
	addConstant("darkslategrey", (int64)0xff2f4f4f);
	addConstant("darkturquoise", (int64)0xff00ced1);
	addConstant("darkviolet", (int64)0xff9400d3);
	addConstant("deeppink", (int64)0xffff1493);
	addConstant("deepskyblue", (int64)0xff00bfff);
	addConstant("dimgrey", (int64)0xff696969);
	addConstant("dodgerblue", (int64)0xff1e90ff);
	addConstant("firebrick", (int64)0xffb22222);
	addConstant("floralwhite", (int64)0xfffffaf0);
	addConstant("forestgreen", (int64)0xff228b22);
	addConstant("fuchsia", (int64)0xffff00ff);
	addConstant("gainsboro", (int64)0xffdcdcdc);
	addConstant("gold", (int64)0xffffd700);
	addConstant("goldenrod", (int64)0xffdaa520);
	addConstant("grey", (int64)0xff808080);
	addConstant("green", (int64)0xff008000);
	addConstant("greenyellow", (int64)0xffadff2f);
	addConstant("honeydew", (int64)0xfff0fff0);
	addConstant("hotpink", (int64)0xffff69b4);
	addConstant("indianred", (int64)0xffcd5c5c);
	addConstant("indigo", (int64)0xff4b0082);
	addConstant("ivory", (int64)0xfffffff0);
	addConstant("khaki", (int64)0xfff0e68c);
	addConstant("lavender", (int64)0xffe6e6fa);
	addConstant("lavenderblush", (int64)0xfffff0f5);
	addConstant("lemonchiffon", (int64)0xfffffacd);
	addConstant("lightblue", (int64)0xffadd8e6);
	addConstant("lightcoral", (int64)0xfff08080);
	addConstant("lightcyan", (int64)0xffe0ffff);
	addConstant("lightgoldenrodyellow", (int64)0xfffafad2);
	addConstant("lightgreen", (int64)0xff90ee90);
	addConstant("lightgrey", (int64)0xffd3d3d3);
	addConstant("lightpink", (int64)0xffffb6c1);
	addConstant("lightsalmon", (int64)0xffffa07a);
	addConstant("lightseagreen", (int64)0xff20b2aa);
	addConstant("lightskyblue", (int64)0xff87cefa);
	addConstant("lightslategrey", (int64)0xff778899);
	addConstant("lightsteelblue", (int64)0xffb0c4de);
	addConstant("lightyellow", (int64)0xffffffe0);
	addConstant("lime", (int64)0xff00ff00);
	addConstant("limegreen", (int64)0xff32cd32);
	addConstant("linen", (int64)0xfffaf0e6);
	addConstant("magenta", (int64)0xffff00ff);
	addConstant("maroon", (int64)0xff800000);
	addConstant("mediumaquamarine", (int64)0xff66cdaa);
	addConstant("mediumblue", (int64)0xff0000cd);
	addConstant("mediumorchid", (int64)0xffba55d3);
	addConstant("mediumpurple", (int64)0xff9370db);
	addConstant("mediumseagreen", (int64)0xff3cb371);
	addConstant("mediumslateblue", (int64)0xff7b68ee);
	addConstant("mediumspringgreen", (int64)0xff00fa9a);
	addConstant("mediumturquoise", (int64)0xff48d1cc);
	addConstant("mediumvioletred", (int64)0xffc71585);
	addConstant("midnightblue", (int64)0xff191970);
	addConstant("mintcream", (int64)0xfff5fffa);
	addConstant("mistyrose", (int64)0xffffe4e1);
	addConstant("navajowhite", (int64)0xffffdead);
	addConstant("navy", (int64)0xff000080);
	addConstant("oldlace", (int64)0xfffdf5e6);
	addConstant("olive", (int64)0xff808000);
	addConstant("olivedrab", (int64)0xff6b8e23);
	addConstant("orange", (int64)0xffffa500);
	addConstant("orangered", (int64)0xffff4500);
	addConstant("orchid", (int64)0xffda70d6);
	addConstant("palegoldenrod", (int64)0xffeee8aa);
	addConstant("palegreen", (int64)0xff98fb98);
	addConstant("paleturquoise", (int64)0xffafeeee);
	addConstant("palevioletred", (int64)0xffdb7093);
	addConstant("papayawhip", (int64)0xffffefd5);
	addConstant("peachpuff", (int64)0xffffdab9);
	addConstant("peru", (int64)0xffcd853f);
	addConstant("pink", (int64)0xffffc0cb);
	addConstant("plum", (int64)0xffdda0dd);
	addConstant("powderblue", (int64)0xffb0e0e6);
	addConstant("purple", (int64)0xff800080);
	addConstant("red", (int64)0xffff0000);
	addConstant("rosybrown", (int64)0xffbc8f8f);
	addConstant("royalblue", (int64)0xff4169e1);
	addConstant("saddlebrown", (int64)0xff8b4513);
	addConstant("salmon", (int64)0xfffa8072);
	addConstant("sandybrown", (int64)0xfff4a460);
	addConstant("seagreen", (int64)0xff2e8b57);
	addConstant("seashell", (int64)0xfffff5ee);
	addConstant("sienna", (int64)0xffa0522d);
	addConstant("silver", (int64)0xffc0c0c0);
	addConstant("skyblue", (int64)0xff87ceeb);
	addConstant("slateblue", (int64)0xff6a5acd);
	addConstant("slategrey", (int64)0xff708090);
	addConstant("snow", (int64)0xfffffafa);
	addConstant("springgreen", (int64)0xff00ff7f);
	addConstant("steelblue", (int64)0xff4682b4);
	addConstant("tan", (int64)0xffd2b48c);
	addConstant("teal", (int64)0xff008080);
	addConstant("thistle", (int64)0xffd8bfd8);
	addConstant("tomato", (int64)0xffff6347);
	addConstant("turquoise", (int64)0xff40e0d0);
	addConstant("violet", (int64)0xffee82ee);
	addConstant("wheat", (int64)0xfff5deb3);
	addConstant("white", (int64)0xffffffff);
	addConstant("whitesmoke", (int64)0xfff5f5f5);
	addConstant("yellow", (int64)0xffffff00);
	addConstant("yellowgreen", (int64)0xff9acd32);

	ADD_API_METHOD_2(withAlpha);
}

int ScriptingApi::Colours::withAlpha(int colour, float alpha)
{
	Colour c((uint32)colour);

	return (int)c.withAlpha(alpha).getARGB();
}
