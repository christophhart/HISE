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

namespace hise { using namespace juce;

#define SEND_MESSAGE(broadcaster) {	if (MessageManager::getInstance()->isThisTheMessageThread()) broadcaster->sendSynchronousChangeMessage(); else broadcaster->sendChangeMessage();}
#define ADD_TO_TYPE_SELECTOR(x) (ScriptComponentPropertyTypeSelector::addToTypeSelector(ScriptComponentPropertyTypeSelector::x, propertyIds.getLast()))
#define ADD_AS_SLIDER_TYPE(min, max, interval) (ScriptComponentPropertyTypeSelector::addToTypeSelector(ScriptComponentPropertyTypeSelector::SliderSelector, propertyIds.getLast(), min, max, interval))

#if USE_BACKEND



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
		else return String();
	}
	else return String();
}

#endif


String ApiHelpers::getFileNameFromErrorMessage(const String &message)
{
	if (message.startsWith("Line")) return String();

	String fileName = message.upToFirstOccurrenceOf("-", false, true);

	fileName = fileName.trimEnd();

	return fileName;
}



StringArray ApiHelpers::getJustificationNames()
{
	static StringArray sa;

	if (sa.isEmpty())
	{
		sa.add("left");
		sa.add("right");
		sa.add("top");
		sa.add("bottom");
		sa.add("centred");
		sa.add("centredTop");
		sa.add("centredBottom");
		sa.add("topLeft");
		sa.add("topRight");
		sa.add("bottomLeft");
		sa.add("bottomRight");
	}

	return sa;
}

Justification ApiHelpers::getJustification(const String& justificationName, Result* r/*=nullptr*/)
{
	static Array<Justification::Flags> justifications;

	if (justifications.isEmpty())
	{
		justifications.add(Justification::left);
		justifications.add(Justification::right);
		justifications.add(Justification::top);
		justifications.add(Justification::bottom);
		justifications.add(Justification::centred);
		justifications.add(Justification::centredTop);
		justifications.add(Justification::centredBottom);
		justifications.add(Justification::topLeft);
		justifications.add(Justification::topRight);
		justifications.add(Justification::bottomLeft);
		justifications.add(Justification::bottomRight);
	}

	auto names = getJustificationNames();

	int index = names.indexOf(justificationName);

	if (index != -1)
	{
		return justifications[index];
	}

	if (r != nullptr)
		*r = Result::fail("Justification not found: " + justificationName);

	return Justification::centred;
}

Point<float> ApiHelpers::getPointFromVar(const var& data, Result* r /*= nullptr*/)
{
	if (data.isArray())
	{
		Array<var>* d = data.getArray();

		if (d->size() == 2)
		{
            auto d0 = (float)d->getUnchecked(0);
            auto d1 = (float)d->getUnchecked(1);

            Point<float> p(SANITIZED(d0), SANITIZED(d1));

			return p;
		}
		else
		{
			if (r != nullptr) *r = Result::fail("Point array needs 2 elements");

			return Point<float>();
		}
	}
	else
	{
		if (r != nullptr) *r = Result::fail("Point is not an array");

		return Point<float>();
	}
}

var ApiHelpers::getVarRectangle(Rectangle<float> floatRectangle, Result* r /*= nullptr*/)
{
	ignoreUnused(r);

	Array<var> newRect;

	newRect.add(floatRectangle.getX());
	newRect.add(floatRectangle.getY());
	newRect.add(floatRectangle.getWidth());
	newRect.add(floatRectangle.getHeight());

	return var(newRect);
}



Rectangle<float> ApiHelpers::getRectangleFromVar(const var &data, Result *r/*=nullptr*/)
{
	if (data.isArray())
	{
		Array<var>* d = data.getArray();

		if (d->size() == 4)
		{
            auto d0 = (float)d->getUnchecked(0);
            auto d1 = (float)d->getUnchecked(1);
            auto d2 = (float)d->getUnchecked(2);
            auto d3 = (float)d->getUnchecked(3);

			Rectangle<float> rectangle(SANITIZED(d0), SANITIZED(d1), SANITIZED(d2), SANITIZED(d3));

            if(r != nullptr) *r = Result::ok();

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
	API_METHOD_WRAPPER_0(Message, isProgramChange);
	API_METHOD_WRAPPER_0(Message, getProgramChangeNumber);
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
	API_VOID_METHOD_WRAPPER_1(Message, setGain);
	API_METHOD_WRAPPER_0(Message, getGain);
	API_VOID_METHOD_WRAPPER_1(Message, setStartOffset);
	API_METHOD_WRAPPER_0(Message, getStartOffset)
	API_METHOD_WRAPPER_0(Message, getTimestamp);
	API_VOID_METHOD_WRAPPER_1(Message, store);
	API_METHOD_WRAPPER_0(Message, makeArtificial);
	API_METHOD_WRAPPER_0(Message, isArtificial);

};


ScriptingApi::Message::Message(ProcessorWithScriptingContent *p) :
ScriptingObject(p),
ApiClass(0),
messageHolder(nullptr),
constMessageHolder(nullptr)
{
	memset(artificialNoteOnIds, 0, sizeof(uint16) * 128);

	ADD_API_METHOD_1(setNoteNumber);
	ADD_API_METHOD_1(setVelocity);
	ADD_API_METHOD_1(setControllerNumber);
	ADD_API_METHOD_1(setControllerValue);
	ADD_API_METHOD_0(getControllerNumber);
	ADD_API_METHOD_0(getControllerValue);
	ADD_API_METHOD_0(isProgramChange);
	ADD_API_METHOD_0(getProgramChangeNumber);
	ADD_API_METHOD_0(getNoteNumber);
	ADD_API_METHOD_0(getVelocity);
	ADD_API_METHOD_1(ignoreEvent);
	ADD_API_METHOD_1(delayEvent);
	ADD_API_METHOD_0(getEventId);
	ADD_API_METHOD_0(getChannel);
	ADD_API_METHOD_1(setChannel);
	ADD_API_METHOD_0(getGain);
	ADD_API_METHOD_1(setGain);
	ADD_API_METHOD_1(setTransposeAmount);
	ADD_API_METHOD_0(getTransposeAmount);
	ADD_API_METHOD_1(setCoarseDetune);
	ADD_API_METHOD_0(getCoarseDetune);
	ADD_API_METHOD_1(setFineDetune);
	ADD_API_METHOD_0(getFineDetune);
	ADD_API_METHOD_0(getTimestamp);
	ADD_API_METHOD_0(getStartOffset);
	ADD_API_METHOD_1(setStartOffset);
	ADD_API_METHOD_1(store);
	ADD_API_METHOD_0(makeArtificial);
	ADD_API_METHOD_0(isArtificial);
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
		RETURN_IF_NO_THROW(-1)
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

	messageHolder->setVelocity((uint8)newValue);
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

	messageHolder->setControllerNumber(newValue);
};

void ScriptingApi::Message::setControllerValue(int newValue)
{
#if ENABLE_SCRIPTING_SAFE_CHECKS
	if(messageHolder == nullptr)
	{
		reportIllegalCall("setControllerValue()", "midi event");
		return;
	}

	if(!messageHolder->isController())
	{
		reportIllegalCall("setControllerValue()", "onController");
	}
#endif




	messageHolder->setControllerValue(newValue);
};

bool ScriptingApi::Message::isProgramChange()
{
#if ENABLE_SCRIPTING_SAFE_CHECKS
	if (constMessageHolder == nullptr)
	{
		reportIllegalCall("isProgramChange()", "midi event");
		return false;
	}
#endif

	return constMessageHolder->isProgramChange();
}

int ScriptingApi::Message::getProgramChangeNumber()
{
#if ENABLE_SCRIPTING_SAFE_CHECKS
	if (messageHolder == nullptr)
	{
		reportIllegalCall("setVelocity()", "midi event");
		return -1;
	}
#endif

	if (constMessageHolder->isProgramChange())
		return constMessageHolder->getProgramChangeNumber();
	else
		return -1;
}

var ScriptingApi::Message::getControllerNumber() const
{
#if ENABLE_SCRIPTING_SAFE_CHECKS
	if(constMessageHolder == nullptr || ( !constMessageHolder->isController() && !constMessageHolder->isPitchWheel() && !constMessageHolder->isAftertouch() ))
	{
		reportIllegalCall("getControllerNumber()", "onController");
		RETURN_IF_NO_THROW(var())
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
		RETURN_IF_NO_THROW(var())
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
	if(constMessageHolder == nullptr || (!constMessageHolder->isNoteOnOrOff()))
	{
		reportIllegalCall("getVelocity()", "onNoteOn");
		RETURN_IF_NO_THROW(-1)
	}
#endif

	return constMessageHolder->getVelocity();
};

void ScriptingApi::Message::ignoreEvent(bool shouldBeIgnored/*=true*/)
{
	if (messageHolder == nullptr)
	{
		reportIllegalCall("ignoreEvent()", "midi event");
		RETURN_VOID_IF_NO_THROW()
	}

	messageHolder->ignoreEvent(shouldBeIgnored);
}

int ScriptingApi::Message::getChannel() const
{
#if ENABLE_SCRIPTING_SAFE_CHECKS
	if(constMessageHolder == nullptr)
	{
		reportScriptError("Can only be called in MIDI callbacks");
		RETURN_IF_NO_THROW(-1)
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
		RETURN_VOID_IF_NO_THROW()
	}

	if(newValue < 1 || newValue > 16)
	{
		reportScriptError("Channel must be between 1 and 16.");
		RETURN_VOID_IF_NO_THROW()
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
		FRONTEND_ONLY(return 0;)
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
		FRONTEND_ONLY(return;)
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
		FRONTEND_ONLY(return 0;)
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
		RETURN_IF_NO_THROW(0)
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
		RETURN_VOID_IF_NO_THROW()
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
		RETURN_IF_NO_THROW(0)
	}
#endif

	return constMessageHolder->getFineDetune();
}


void ScriptingApi::Message::setGain(int gainInDecibels)
{
#if ENABLE_SCRIPTING_SAFE_CHECKS
	if (messageHolder == nullptr)
	{
		reportIllegalCall("setGain()", "midi event");
		RETURN_VOID_IF_NO_THROW()
	}
#endif

	messageHolder->setGain(gainInDecibels);
}

int ScriptingApi::Message::getGain() const
{
#if ENABLE_SCRIPTING_SAFE_CHECKS
	if (constMessageHolder == nullptr)
	{
		reportIllegalCall("getGain()", "midi event");
		RETURN_IF_NO_THROW(0)
	}
#endif

	return constMessageHolder->getGain();
}

int ScriptingApi::Message::getTimestamp() const
{
#if ENABLE_SCRIPTING_SAFE_CHECKS
	if (constMessageHolder == nullptr)
	{
		reportIllegalCall("getTimestamp()", "midi event");
		RETURN_IF_NO_THROW(0)
	}
#endif

	return constMessageHolder->getTimeStamp();
}

void ScriptingApi::Message::setStartOffset(int newStartOffset)
{
#if ENABLE_SCRIPTING_SAFE_CHECKS
	if (constMessageHolder == nullptr)
	{
		reportIllegalCall("setStartOffset()", "midi event");
		RETURN_VOID_IF_NO_THROW()
	}

	if (newStartOffset > UINT16_MAX)
		reportScriptError("Max start offset is 65536 (2^16)");

#endif

	messageHolder->setStartOffset((uint16)newStartOffset);
}

int ScriptingApi::Message::getStartOffset() const
{
#if ENABLE_SCRIPTING_SAFE_CHECKS
	if (constMessageHolder == nullptr)
	{
		reportIllegalCall("setStartOffset()", "midi event");
		RETURN_IF_NO_THROW(0)
	}
#endif

	return constMessageHolder->getStartOffset();
}

void ScriptingApi::Message::store(var messageEventHolder) const
{
#if ENABLE_SCRIPTING_SAFE_CHECKS
	if (constMessageHolder == nullptr)
	{
		reportIllegalCall("store()", "midi event");
		RETURN_VOID_IF_NO_THROW()
	}
#endif

	ScriptingObjects::ScriptingMessageHolder* holder = dynamic_cast<ScriptingObjects::ScriptingMessageHolder*>(messageEventHolder.getObject());

	if (holder != nullptr && constMessageHolder != nullptr)
	{
		holder->setMessage(*constMessageHolder);
	}
}

int ScriptingApi::Message::makeArtificial()
{
	if (messageHolder != nullptr)
	{
		if (messageHolder->isArtificial()) return messageHolder->getEventId();

		HiseEvent copy(*messageHolder);

		copy.setArtificial();

		if (copy.isNoteOn())
		{
			getScriptProcessor()->getMainController_()->getEventHandler().pushArtificialNoteOn(copy);
			artificialNoteOnIds[copy.getNoteNumber()] = copy.getEventId();

		}
		else if (copy.isNoteOff())
		{
			HiseEvent e = getScriptProcessor()->getMainController_()->getEventHandler().popNoteOnFromEventId(artificialNoteOnIds[copy.getNoteNumber()]);

			if (e.isEmpty())
			{
				artificialNoteOnIds[copy.getNoteNumber()] = 0;
				copy.ignoreEvent(true);
			}

			copy.setEventId(artificialNoteOnIds[copy.getNoteNumber()]);
		}

		copy.swapWith(*messageHolder);

		return messageHolder->getEventId();
	}

	return 0;
}

bool ScriptingApi::Message::isArtificial() const
{
	if (constMessageHolder != nullptr)
	{
		return constMessageHolder->isArtificial();
	}

	return false;
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

hise::HiseEvent& ScriptingApi::Message::getCurrentEventReference()
{
	if (messageHolder != nullptr)
		return *messageHolder;

	jassertfalse;
	static HiseEvent unused;
	return unused;
}

// ====================================================================================================== Engine functions

struct ScriptingApi::Engine::Wrapper
{
	API_VOID_METHOD_WRAPPER_0(Engine, allNotesOff);
	API_METHOD_WRAPPER_0(Engine, getUptime);
	API_METHOD_WRAPPER_0(Engine, getHostBpm);
	API_VOID_METHOD_WRAPPER_1(Engine, setHostBpm);
	API_METHOD_WRAPPER_0(Engine, getCpuUsage);
	API_METHOD_WRAPPER_0(Engine, getNumVoices);
	API_METHOD_WRAPPER_0(Engine, getMemoryUsage);
	API_METHOD_WRAPPER_1(Engine, getMilliSecondsForTempo);
	API_METHOD_WRAPPER_1(Engine, getSamplesForMilliSeconds);
	API_METHOD_WRAPPER_1(Engine, getMilliSecondsForSamples);
	API_METHOD_WRAPPER_1(Engine, getQuarterBeatsForMilliSeconds);
	API_METHOD_WRAPPER_1(Engine, getQuarterBeatsForSamples);
	API_METHOD_WRAPPER_1(Engine, getSamplesForQuarterBeats);
	API_METHOD_WRAPPER_1(Engine, getMilliSecondsForQuarterBeats);
	API_METHOD_WRAPPER_2(Engine, getQuarterBeatsForMilliSecondsWithTempo);
	API_METHOD_WRAPPER_2(Engine, getQuarterBeatsForSamplesWithTempo);
	API_METHOD_WRAPPER_2(Engine, getSamplesForQuarterBeatsWithTempo);
	API_METHOD_WRAPPER_2(Engine, getMilliSecondsForQuarterBeatsWithTempo);
	API_METHOD_WRAPPER_1(Engine, getGainFactorForDecibels);
	API_METHOD_WRAPPER_1(Engine, getDecibelsForGainFactor);
	API_METHOD_WRAPPER_1(Engine, getFrequencyForMidiNoteNumber);
	API_METHOD_WRAPPER_1(Engine, getPitchRatioFromSemitones);
	API_METHOD_WRAPPER_1(Engine, getSemitonesFromPitchRatio);
	API_METHOD_WRAPPER_0(Engine, getSampleRate);
	API_METHOD_WRAPPER_1(Engine, setMinimumSampleRate);
	API_METHOD_WRAPPER_1(Engine, getMidiNoteName);
	API_METHOD_WRAPPER_1(Engine, getMidiNoteFromName);
	API_METHOD_WRAPPER_1(Engine, getMacroName);
	
	API_VOID_METHOD_WRAPPER_1(Engine, setFrontendMacros)
	API_VOID_METHOD_WRAPPER_2(Engine, setKeyColour);
	API_VOID_METHOD_WRAPPER_2(Engine, showErrorMessage);
	API_VOID_METHOD_WRAPPER_1(Engine, showMessage);
	API_VOID_METHOD_WRAPPER_1(Engine, setLowestKeyToDisplay);
    API_VOID_METHOD_WRAPPER_1(Engine, openWebsite);
	API_VOID_METHOD_WRAPPER_1(Engine, loadNextUserPreset);
	API_VOID_METHOD_WRAPPER_1(Engine, loadPreviousUserPreset);
	API_VOID_METHOD_WRAPPER_1(Engine, loadUserPreset);
	API_VOID_METHOD_WRAPPER_1(Engine, setUserPresetTagList);
	API_METHOD_WRAPPER_0(Engine, getUserPresetList);
	API_METHOD_WRAPPER_0(Engine, getCurrentUserPresetName);
	API_VOID_METHOD_WRAPPER_1(Engine, saveUserPreset);
	API_METHOD_WRAPPER_0(Engine, isMpeEnabled);
	API_METHOD_WRAPPER_0(Engine, createSliderPackData);
	API_METHOD_WRAPPER_1(Engine, createAndRegisterSliderPackData);
	API_METHOD_WRAPPER_1(Engine, createAndRegisterTableData);
	API_METHOD_WRAPPER_1(Engine, createAndRegisterAudioFile);
	API_METHOD_WRAPPER_0(Engine, createMidiList);
	API_METHOD_WRAPPER_0(Engine, createTimerObject);
	API_METHOD_WRAPPER_0(Engine, createMessageHolder);
	API_METHOD_WRAPPER_0(Engine, getPlayHead);
	API_VOID_METHOD_WRAPPER_2(Engine, dumpAsJSON);
	API_METHOD_WRAPPER_1(Engine, loadFromJSON);
	API_VOID_METHOD_WRAPPER_1(Engine, setCompileProgress);
	API_METHOD_WRAPPER_2(Engine, matchesRegex);
	API_METHOD_WRAPPER_2(Engine, getRegexMatches);
	API_METHOD_WRAPPER_2(Engine, doubleToString);
	API_METHOD_WRAPPER_0(Engine, getOS);
	API_METHOD_WRAPPER_0(Engine, isPlugin);
	API_METHOD_WRAPPER_0(Engine, getPreloadProgress);
	API_METHOD_WRAPPER_0(Engine, getPreloadMessage);
	API_METHOD_WRAPPER_0(Engine, getDeviceType);
	API_METHOD_WRAPPER_0(Engine, getDeviceResolution);
	API_METHOD_WRAPPER_0(Engine, getZoomLevel);
	API_METHOD_WRAPPER_0(Engine, getVersion);
	API_METHOD_WRAPPER_0(Engine, getName);
	API_METHOD_WRAPPER_0(Engine, getFilterModeList);
	API_METHOD_WRAPPER_2(Engine, sortWithFunction);
	API_METHOD_WRAPPER_1(Engine, isControllerUsedByAutomation);
	API_METHOD_WRAPPER_0(Engine, getSettingsWindowObject);
	API_METHOD_WRAPPER_1(Engine, getMasterPeakLevel);
	API_METHOD_WRAPPER_0(Engine, getControlRateDownsamplingFactor);
	API_METHOD_WRAPPER_1(Engine, createDspNetwork);
	API_METHOD_WRAPPER_0(Engine, createExpansionHandler);
	API_METHOD_WRAPPER_0(Engine, getExpansionList);
	API_METHOD_WRAPPER_1(Engine, setCurrentExpansion);
	API_METHOD_WRAPPER_0(Engine, createGlobalScriptLookAndFeel);
	API_VOID_METHOD_WRAPPER_3(Engine, showYesNoWindow);
	API_VOID_METHOD_WRAPPER_1(Engine, addModuleStateToUserPreset);
	API_VOID_METHOD_WRAPPER_0(Engine, rebuildCachedPools);
	API_VOID_METHOD_WRAPPER_1(Engine, extendTimeOut);
	API_VOID_METHOD_WRAPPER_1(Engine, setAllowDuplicateSamples);
	API_VOID_METHOD_WRAPPER_1(Engine, loadFont);
	API_VOID_METHOD_WRAPPER_2(Engine, loadFontAs);
	API_VOID_METHOD_WRAPPER_1(Engine, setGlobalFont);
	API_VOID_METHOD_WRAPPER_0(Engine, undo);
	API_VOID_METHOD_WRAPPER_0(Engine, redo);
	API_METHOD_WRAPPER_0(Engine, loadAudioFilesIntoPool);
	API_VOID_METHOD_WRAPPER_1(Engine, loadImageIntoPool);
	API_VOID_METHOD_WRAPPER_0(Engine, clearMidiFilePool);
	API_VOID_METHOD_WRAPPER_0(Engine, clearSampleMapPool);
	API_METHOD_WRAPPER_2(Engine, getSampleFilesFromDirectory);
	API_VOID_METHOD_WRAPPER_1(Engine, setLatencySamples);
	API_METHOD_WRAPPER_0(Engine, getLatencySamples);
	API_METHOD_WRAPPER_2(Engine, getDspNetworkReference);
	API_METHOD_WRAPPER_1(Engine, getSystemTime);
};

ScriptingApi::Engine::Engine(ProcessorWithScriptingContent *p) :
ScriptingObject(p),
ApiClass(0),
parentMidiProcessor(dynamic_cast<ScriptBaseMidiProcessor*>(p))
{
	ADD_API_METHOD_0(allNotesOff);
	ADD_API_METHOD_0(getUptime);
	ADD_API_METHOD_0(getHostBpm);
	ADD_API_METHOD_1(setHostBpm);
	ADD_API_METHOD_0(getCpuUsage);
	ADD_API_METHOD_0(getNumVoices);
	ADD_API_METHOD_0(getMemoryUsage);
	ADD_API_METHOD_1(getMilliSecondsForTempo);
	ADD_API_METHOD_1(getSamplesForMilliSeconds);
	ADD_API_METHOD_1(getMilliSecondsForSamples);
	ADD_API_METHOD_1(getMilliSecondsForSamples);
	ADD_API_METHOD_1(getQuarterBeatsForMilliSeconds);
	ADD_API_METHOD_1(getQuarterBeatsForSamples);
	ADD_API_METHOD_1(getSamplesForQuarterBeats);
	ADD_API_METHOD_1(getMilliSecondsForQuarterBeats);
	ADD_API_METHOD_2(getQuarterBeatsForMilliSecondsWithTempo);
	ADD_API_METHOD_2(getQuarterBeatsForSamplesWithTempo);
	ADD_API_METHOD_2(getSamplesForQuarterBeatsWithTempo);
	ADD_API_METHOD_2(getMilliSecondsForQuarterBeatsWithTempo);
	ADD_API_METHOD_1(getGainFactorForDecibels);
	ADD_API_METHOD_1(getDecibelsForGainFactor);
	ADD_API_METHOD_1(getFrequencyForMidiNoteNumber);
	ADD_API_METHOD_1(getPitchRatioFromSemitones);
	ADD_API_METHOD_1(getSemitonesFromPitchRatio);
	ADD_API_METHOD_1(addModuleStateToUserPreset);
	ADD_API_METHOD_0(getSampleRate);
	ADD_API_METHOD_1(setMinimumSampleRate);
	ADD_API_METHOD_1(getMidiNoteName);
	ADD_API_METHOD_1(getMidiNoteFromName);
	ADD_API_METHOD_1(getMacroName);
	ADD_API_METHOD_1(setFrontendMacros);
	ADD_API_METHOD_2(setKeyColour);
	ADD_API_METHOD_2(showErrorMessage);
	ADD_API_METHOD_1(showMessage);
	ADD_API_METHOD_1(setLowestKeyToDisplay);
    ADD_API_METHOD_1(openWebsite);
	ADD_API_METHOD_1(loadNextUserPreset);
	ADD_API_METHOD_1(loadPreviousUserPreset);
	ADD_API_METHOD_0(getExpansionList);
	ADD_API_METHOD_1(setCurrentExpansion);
	ADD_API_METHOD_1(setUserPresetTagList);
	ADD_API_METHOD_0(getCurrentUserPresetName);
	ADD_API_METHOD_1(saveUserPreset);
	ADD_API_METHOD_1(loadUserPreset);
	ADD_API_METHOD_0(getUserPresetList);
	ADD_API_METHOD_0(isMpeEnabled);
	ADD_API_METHOD_0(createMidiList);
	ADD_API_METHOD_0(getPlayHead);
	ADD_API_METHOD_2(dumpAsJSON);
	ADD_API_METHOD_1(loadFromJSON);
	ADD_API_METHOD_1(setCompileProgress);
	ADD_API_METHOD_2(matchesRegex);
	ADD_API_METHOD_2(getRegexMatches);
	ADD_API_METHOD_2(doubleToString);
	ADD_API_METHOD_1(getMasterPeakLevel);
	ADD_API_METHOD_0(getOS);
	ADD_API_METHOD_0(getDeviceType);
	ADD_API_METHOD_0(getDeviceResolution);
	ADD_API_METHOD_0(isPlugin);
	ADD_API_METHOD_0(getPreloadProgress);
	ADD_API_METHOD_0(getPreloadMessage);
	ADD_API_METHOD_0(getZoomLevel);
	ADD_API_METHOD_0(getVersion);
	ADD_API_METHOD_0(getName);
	ADD_API_METHOD_0(getFilterModeList);
	ADD_API_METHOD_2(sortWithFunction);
	ADD_API_METHOD_0(createGlobalScriptLookAndFeel);
	ADD_API_METHOD_1(setAllowDuplicateSamples);
	ADD_API_METHOD_1(isControllerUsedByAutomation);
	ADD_API_METHOD_0(getSettingsWindowObject);
	ADD_API_METHOD_0(createTimerObject);
	ADD_API_METHOD_0(createMessageHolder);
	ADD_API_METHOD_0(createSliderPackData);
	ADD_API_METHOD_1(createAndRegisterSliderPackData);
	ADD_API_METHOD_1(createAndRegisterTableData);
	ADD_API_METHOD_1(createAndRegisterAudioFile);
	ADD_API_METHOD_1(loadFont);
	ADD_API_METHOD_2(loadFontAs);
	ADD_API_METHOD_1(setGlobalFont);
	ADD_API_METHOD_1(extendTimeOut);
	ADD_API_METHOD_0(getControlRateDownsamplingFactor);
	ADD_API_METHOD_0(undo);
	ADD_API_METHOD_0(redo);
	ADD_API_METHOD_0(loadAudioFilesIntoPool);
	ADD_API_METHOD_0(clearMidiFilePool);
	ADD_API_METHOD_0(clearSampleMapPool);
	ADD_API_METHOD_2(getSampleFilesFromDirectory);
	ADD_API_METHOD_0(rebuildCachedPools);
	ADD_API_METHOD_1(loadImageIntoPool);
	ADD_API_METHOD_1(createDspNetwork);
	ADD_API_METHOD_1(setLatencySamples);
	ADD_API_METHOD_0(getLatencySamples);
	ADD_API_METHOD_2(getDspNetworkReference);
	ADD_API_METHOD_0(createExpansionHandler);
	ADD_API_METHOD_3(showYesNoWindow);
	ADD_API_METHOD_1(getSystemTime);
}


void ScriptingApi::Engine::allNotesOff()
{
	getProcessor()->getMainController()->allNotesOff();
};

void ScriptingApi::Engine::addModuleStateToUserPreset(var moduleId)
{
	if (auto jmp = dynamic_cast<JavascriptMidiProcessor*>(getProcessor()))
	{
		auto newId = moduleId.toString();

		auto& ids = jmp->getListOfModuleIds();

		if (newId.isEmpty())
		{
			ids.clear();
			debugToConsole(getProcessor(), "Removed all stored modules");
			return;
		}

		auto p = ProcessorHelpers::getFirstProcessorWithName(getProcessor()->getMainController()->getMainSynthChain(), newId);

		if (p == nullptr)
			reportScriptError("Can't find processor " + newId);

		auto childList = ProcessorHelpers::getListOfAllProcessors<Processor>(p);

		for (auto c : childList)
		{
			if (c == p)
				continue;

			if (dynamic_cast<Chain*>(c.get()) != nullptr)
			{
				reportScriptError("Can't store modules with child modules");
				return;
			}
		}

		if (!ids.contains(newId))
		{
			ids.add(newId);
			debugToConsole(getProcessor(), "Added " + newId + " to user preset system");
		}
	}
}

void ScriptingApi::Engine::loadFont(const String &fileName)
{
	debugError(getProcessor(), "loadFont is deprecated. Use loadFontAs() instead to prevent cross platform issues");
	loadFontAs(fileName, String());
}

void ScriptingApi::Engine::loadFontAs(String fileName, String fontId)
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
		getProcessor()->getMainController()->loadTypeFace(fileName, mb.getData(), mb.getSize(), fontId);
	}

#else

	// Nothing to do here, it will be loaded on startup...

	ignoreUnused(fileName);

#endif
}

void ScriptingApi::Engine::setGlobalFont(String fontName)
{
	getProcessor()->getMainController()->setGlobalFont(fontName);
}

bool ScriptingApi::Engine::setMinimumSampleRate(double minimumSampleRate)
{
	return getProcessor()->getMainController()->setMinimumSamplerate(minimumSampleRate);
}

double ScriptingApi::Engine::getSampleRate() const { return const_cast<MainController*>(getProcessor()->getMainController())->getMainSynthChain()->getSampleRate(); }
double ScriptingApi::Engine::getSamplesForMilliSeconds(double milliSeconds) const { return (milliSeconds / 1000.0) * getSampleRate(); }



double ScriptingApi::Engine::getQuarterBeatsForSamples(double samples)
{
	return getQuarterBeatsForSamplesWithTempo(samples, getHostBpm());
}

double ScriptingApi::Engine::getQuarterBeatsForMilliSeconds(double milliSeconds)
{
	auto samples = getSamplesForMilliSeconds(milliSeconds);
	return getQuarterBeatsForSamples(samples);
}

double ScriptingApi::Engine::getSamplesForQuarterBeats(double quarterBeats)
{
	return getSamplesForQuarterBeatsWithTempo(quarterBeats, getHostBpm());
}

double ScriptingApi::Engine::getMilliSecondsForQuarterBeats(double quarterBeats)
{
	auto samples = getSamplesForQuarterBeats(quarterBeats);
	return getMilliSecondsForSamples(samples);
}


double ScriptingApi::Engine::getQuarterBeatsForSamplesWithTempo(double samples, double bpm)
{
	auto samplesPerQuarter = (double)TempoSyncer::getTempoInSamples(bpm, getSampleRate(), TempoSyncer::Quarter);
	return (double)samples / samplesPerQuarter;
}

double ScriptingApi::Engine::getQuarterBeatsForMilliSecondsWithTempo(double milliSeconds, double bpm)
{
	auto samples = getSamplesForMilliSeconds(milliSeconds);
	return getQuarterBeatsForSamplesWithTempo(samples, bpm);
}

double ScriptingApi::Engine::getSamplesForQuarterBeatsWithTempo(double quarterBeats, double bpm)
{
	auto samplesPerQuarter = (double)TempoSyncer::getTempoInSamples(bpm, getSampleRate(), TempoSyncer::Quarter);

	return samplesPerQuarter * quarterBeats;
}

double ScriptingApi::Engine::getMilliSecondsForQuarterBeatsWithTempo(double quarterBeats, double bpm)
{
	auto samples = getSamplesForQuarterBeatsWithTempo(quarterBeats, bpm);
	return getMilliSecondsForSamples(samples);
}

double ScriptingApi::Engine::getUptime() const
{
	if (parentMidiProcessor != nullptr && parentMidiProcessor->getCurrentHiseEvent() != nullptr)
	{
		return parentMidiProcessor->getMainController()->getUptime() + parentMidiProcessor->getCurrentHiseEvent()->getTimeStamp() / getSampleRate();
	}

	return getProcessor()->getMainController()->getUptime();
}
double ScriptingApi::Engine::getHostBpm() const		 { return getProcessor()->getMainController()->getBpm(); }

void ScriptingApi::Engine::setHostBpm(double newTempo)
{
	getProcessor()->getMainController()->setHostBpm(newTempo);
}

double ScriptingApi::Engine::getMemoryUsage() const
{
	auto bytes = getProcessor()->getMainController()->getSampleManager().getModulatorSamplerSoundPool2()->getMemoryUsageForAllSamples();
	auto& handler = getProcessor()->getMainController()->getExpansionHandler();

	for (int i = 0; i < handler.getNumExpansions(); i++)
	{
		bytes += handler.getExpansion(i)->pool->getSamplePool()->getMemoryUsageForAllSamples();
	}

	return (double)bytes / 1024.0 / 1024.0;
}

double ScriptingApi::Engine::getCpuUsage() const { return (double)getProcessor()->getMainController()->getCpuUsage(); }
int ScriptingApi::Engine::getNumVoices() const { return getProcessor()->getMainController()->getNumActiveVoices(); }

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

void ScriptingApi::Engine::setFrontendMacros(var nameList)
{
	auto& mm = getProcessor()->getMainController()->getMacroManager();

	if (auto ar = nameList.getArray())
	{
		mm.setEnableMacroOnFrontend(!ar->isEmpty());
		
		for (int i = 0; i < 8; i++)
		{
			auto macroName = (*ar)[i].toString();
			mm.getMacroChain()->getMacroControlData(i)->setMacroName(macroName);
		}
	}
	else
	{
		mm.setEnableMacroOnFrontend(false);
		reportScriptError("Expected an Array of Strings");
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

String ScriptingApi::Engine::getDeviceType()
{
	return HiseDeviceSimulator::getDeviceName();
}

var ScriptingApi::Engine::getDeviceResolution()
{
	auto r = HiseDeviceSimulator::getDisplayResolution();

	Array<var> a = { r.getX(), r.getY(), r.getWidth(), r.getHeight() };

	return a;

}

bool ScriptingApi::Engine::isPlugin() const
{
#if HISE_IOS

    return HiseDeviceSimulator::isAUv3();

#else

#if IS_STANDALONE_APP
	return false;
#else
	return true;
#endif
#endif
}

double ScriptingApi::Engine::getPreloadProgress()
{
	return getScriptProcessor()->getMainController_()->getSampleManager().getPreloadProgress();
}

String ScriptingApi::Engine::getPreloadMessage()
{
	return getScriptProcessor()->getMainController_()->getSampleManager().getPreloadMessage();
}

var ScriptingApi::Engine::getZoomLevel() const
{
	return dynamic_cast<const GlobalSettingManager*>(getScriptProcessor()->getMainController_())->getGlobalScaleFactor();
}

var ScriptingApi::Engine::getFilterModeList() const
{
	return var(new ScriptingObjects::ScriptingEffect::FilterModeObject(getScriptProcessor()));
}

String ScriptingApi::Engine::getVersion()
{
#if USE_BACKEND
	return dynamic_cast<GlobalSettingManager*>(getProcessor()->getMainController())->getSettingsObject().getSetting(HiseSettings::Project::Version);
#else
	return FrontendHandler::getVersionString();
#endif
}

String ScriptingApi::Engine::getName()
{
#if USE_BACKEND
	return dynamic_cast<GlobalSettingManager*>(getProcessor()->getMainController())->getSettingsObject().getSetting(HiseSettings::Project::Name);
#else
	return FrontendHandler::getProjectName();
#endif
}

double ScriptingApi::Engine::getMasterPeakLevel(int channel)
{
	// currently only stereo supported for this method...

	if (channel == 0)
		return getScriptProcessor()->getMainController_()->getMainSynthChain()->getDisplayValues().outL;
	else
		return getScriptProcessor()->getMainController_()->getMainSynthChain()->getDisplayValues().outR;
}

var ScriptingApi::Engine::getSettingsWindowObject()
{
	reportScriptError("Deprecated");
	return var();
}

double ScriptingApi::Engine::getControlRateDownsamplingFactor() const
{
	return (double)HISE_EVENT_RASTER;
}

var ScriptingApi::Engine::getSampleFilesFromDirectory(const String& relativePathFromSampleFolder, bool recursive)
{
#if USE_BACKEND

	auto sampleRoot = getScriptProcessor()->getMainController_()->getCurrentFileHandler().getSubDirectory(FileHandlerBase::Samples);

	auto directoryToSearch = sampleRoot.getChildFile(relativePathFromSampleFolder);

	if (!directoryToSearch.isDirectory())
	{
		reportScriptError("The directory " + directoryToSearch.getFullPathName() + " does not exist");
		RETURN_IF_NO_THROW(var());
	}

	auto list = directoryToSearch.findChildFiles(File::findFiles, recursive);

	// We can be this restrictive here...
	StringArray validExtensions = { ".aif", ".aiff", ".wav", ".AIF", ".AIFF", ".WAV" };

	for (int i = 0; i < list.size(); i++)
	{
		if (list[i].isHidden() || list[i].getFileName().startsWith("."))
			list.remove(i--);

		if (!validExtensions.contains(list[i].getFileExtension()))
			list.remove(i--);
	}

	Array<var> returnList;

	for (auto& l : list)
	{
		PoolReference ref(getScriptProcessor()->getMainController_(), l.getFullPathName(), FileHandlerBase::Samples);

		returnList.add(ref.getReferenceString());
	}

	return returnList;

#else
	return {};
#endif

	
}

void ScriptingApi::Engine::showYesNoWindow(String title, String markdownMessage, var callback)
{
	//auto p = dynamic_cast<JavascriptProcessor*>(getScriptProcessor());
	auto p = getScriptProcessor();

	auto f = [markdownMessage, title, callback, p]
	{
		auto ok = PresetHandler::showYesNoWindow(title, markdownMessage);

		std::array<var, 1> args = { var(ok) };

		//Array<var> args = { var(ok) };



		WeakCallbackHolder cb(p, callback, 1);
		cb.call({ var(ok) });

#if 0
		dynamic_cast<ControlledObject*>(p)->getMainController()->getJavascriptThreadPool().addJob(JavascriptThreadPool::Task::HiPriorityCallbackExecution,
			p, [ok, callback](JavascriptProcessor* p)
		{
			auto r = Result::ok();

			var arg(ok);

			p->getScriptEngine()->callExternalFunction(callback, var::NativeFunctionArgs({}, &arg, 1), &r);

			if (!r.wasOk())
				debugError(dynamic_cast<Processor*>(p), r.getErrorMessage());

			return r;
		});
#endif
	};

	MessageManager::callAsync(f);
}

var ScriptingApi::Engine::createGlobalScriptLookAndFeel()
{
	auto mc = getScriptProcessor()->getMainController_();

	if (auto sc = mc->getCurrentScriptLookAndFeel())
		return var(sc);
	else
	{
		auto slaf = new ScriptingObjects::ScriptedLookAndFeel(getScriptProcessor());
		return var(slaf);
	}
}

int ScriptingApi::Engine::getLatencySamples() const
{
	return dynamic_cast<const AudioProcessor*>(getScriptProcessor()->getMainController_())->getLatencySamples();
}

void ScriptingApi::Engine::setLatencySamples(int latency)
{
	auto ap = dynamic_cast<AudioProcessor*>(getScriptProcessor()->getMainController_());
	ap->setLatencySamples(latency);
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



var ScriptingApi::Engine::createDspNetwork(String id)
{
	if (auto holder = dynamic_cast<scriptnode::DspNetwork::Holder*>(getScriptProcessor()))
	{
		return holder->getOrCreate(id);
	}

	reportScriptError("Not available on this script processor");
	RETURN_IF_NO_THROW({});
}

var ScriptingApi::Engine::createExpansionHandler()
{
	return var(new ScriptExpansionHandler(dynamic_cast<JavascriptProcessor*>(getScriptProcessor())));
}

var ScriptingApi::Engine::getDspNetworkReference(String processorId, String id)
{
	Processor::Iterator<scriptnode::DspNetwork::Holder> iter(getScriptProcessor()->getMainController_()->getMainSynthChain());

	while (auto h = iter.getNextProcessor())
	{
		if (dynamic_cast<Processor*>(h)->getId() == processorId)
		{
			if (h->getIdList().contains(id))
			{
				return var(h->getOrCreate(id));
			}
			else
				reportScriptError("Can't find DSP network with given ID");
		}
	}

	return var();
}

void ScriptingApi::Engine::setKeyColour(int keyNumber, int colourAsHex) { getProcessor()->getMainController()->setKeyboardCoulour(keyNumber, Colour(colourAsHex));}

void ScriptingApi::Engine::extendTimeOut(int additionalMilliseconds)
{
	dynamic_cast<JavascriptProcessor*>(getScriptProcessor())->getScriptEngine()->extendTimeout(additionalMilliseconds);
}

void ScriptingApi::Engine::setLowestKeyToDisplay(int keyNumber) { getProcessor()->getMainController()->setLowestKeyToDisplay(keyNumber); }

void ScriptingApi::Engine::showErrorMessage(String message, bool isCritical)
{
#if USE_FRONTEND

	if (isCritical)
	{
		getProcessor()->getMainController()->sendOverlayMessage(isCritical ? DeactiveOverlay::State::CriticalCustomErrorMessage :
			DeactiveOverlay::State::CustomErrorMessage,
			message);
	}




#else

	ignoreUnused(isCritical);

	reportScriptError(message);

#endif
}

void ScriptingApi::Engine::showMessage(String message)
{
#if USE_FRONTEND

	getProcessor()->getMainController()->sendOverlayMessage(DeactiveOverlay::State::CustomInformation, message);

#else
	debugToConsole(getProcessor(), message);
#endif
}

double ScriptingApi::Engine::getMilliSecondsForTempo(int tempoIndex) const { return (double)TempoSyncer::getTempoInMilliSeconds(getHostBpm(), (TempoSyncer::Tempo)tempoIndex); }

void ScriptingApi::Engine::openWebsite(String url)
{
    URL u(url);

    if (u.isWellFormed())
    {
        auto& tmp = u;

        auto f = [tmp]()
        {
            tmp.launchInDefaultBrowser();
        };

        new DelayedFunctionCaller(f, 300);


    }
    else
    {
        reportScriptError("not a valid URL");
    }
}

var ScriptingApi::Engine::getExpansionList()
{
	auto h = createExpansionHandler();
	return dynamic_cast<ScriptExpansionHandler*>(h.getObject())->getExpansionList();
}

bool ScriptingApi::Engine::setCurrentExpansion(const String& expansionName)
{
	auto& expHandler = getProcessor()->getMainController()->getExpansionHandler();
	return expHandler.setCurrentExpansion(expansionName);
}

void ScriptingApi::Engine::loadNextUserPreset(bool stayInDirectory)
{
	getProcessor()->getMainController()->getUserPresetHandler().incPreset(true, stayInDirectory);
}

void ScriptingApi::Engine::loadPreviousUserPreset(bool stayInDirectory)
{
	getProcessor()->getMainController()->getUserPresetHandler().incPreset(false, stayInDirectory);
}

bool ScriptingApi::Engine::isMpeEnabled() const
{
	return getScriptProcessor()->getMainController_()->getMacroManager().getMidiControlAutomationHandler()->getMPEData().isMpeEnabled();
}

String ScriptingApi::Engine::getCurrentUserPresetName()
{
	return getProcessor()->getMainController()->getUserPresetHandler().getCurrentlyLoadedFile().getFileNameWithoutExtension();
}

void ScriptingApi::Engine::saveUserPreset(var presetName)
{
	if (auto sf = dynamic_cast<ScriptingObjects::ScriptFile*>(presetName.getObject()))
	{
		getProcessor()->getMainController()->getUserPresetHandler().setCurrentlyLoadedFile(sf->f);
		UserPresetHelpers::saveUserPreset(getProcessor()->getMainController()->getMainSynthChain(), sf->f.getFullPathName());
	}
	else
	{
		getProcessor()->getMainController()->getUserPresetHandler().savePreset(presetName);
	}
}

struct DynamicArrayComparator
{
	DynamicArrayComparator(HiseJavascriptEngine* engine_, var sortFunction_, var arrayToSort_):
		engine(engine_),
		sortFunction(sortFunction_),
		arrayToSort(arrayToSort_)
	{
		if (!HiseJavascriptEngine::isJavascriptFunction(sortFunction))
			valid = false;

		if (!arrayToSort.isArray())
			valid = false;
	}

	int compareElements(var a, var b) const
	{
		arg[0] = a;
		arg[1] = b;

		var::NativeFunctionArgs args(arrayToSort, arg, 2);
		
		auto rt = engine->callExternalFunctionRaw(sortFunction, args);

		arg[0] = var();
		arg[1] = var();

		return (int)rt;
	}

	mutable var arg[2];
	bool valid = true;
	HiseJavascriptEngine* engine;
	var sortFunction;
	var arrayToSort;
};

bool ScriptingApi::Engine::sortWithFunction(var arrayToSort, var sortFunction)
{
	if (auto ar = arrayToSort.getArray())
	{
		DynamicArrayComparator dac(dynamic_cast<JavascriptProcessor*>(getScriptProcessor())->getScriptEngine(), sortFunction, arrayToSort);

		if (dac.valid)
		{
			ar->sort(dac, true);
			return true;
		}
	}

	return false;
}

void ScriptingApi::Engine::loadUserPreset(var file)
{
	auto name = ScriptingObjects::ScriptFile::getFileNameFromFile(file);

	File userPresetToLoad;

	if (File::isAbsolutePath(name))
	{
		userPresetToLoad = File(name);
	}
	else
	{
#if USE_BACKEND
		File userPresetRoot = GET_PROJECT_HANDLER(getProcessor()).getSubDirectory(ProjectHandler::SubDirectories::UserPresets);
#else
		File userPresetRoot = FrontendHandler::getUserPresetDirectory();
#endif

		userPresetToLoad = userPresetRoot.getChildFile(file.toString() + ".preset");
	}

    if(!getProcessor()->getMainController()->isInitialised())
    {
        reportScriptError("Do not load user presets at startup.");
    }
    else if (userPresetToLoad.existsAsFile())
	{
		getProcessor()->getMainController()->getUserPresetHandler().loadUserPreset(userPresetToLoad);
		getProcessor()->getMainController()->getUserPresetHandler().setCurrentlyLoadedFile(userPresetToLoad);
	}
	else
	{
		reportScriptError("User preset " + userPresetToLoad.getFullPathName() + " doesn't exist");
	}
}

void ScriptingApi::Engine::setUserPresetTagList(var listOfTags)
{
	if(auto ar = listOfTags.getArray())
	{
		StringArray sa;

		for (auto l : *ar)
			sa.add(l.toString());

		getProcessor()->getMainController()->getUserPresetHandler().getTagDataBase().setTagList(sa);
	}


}

var ScriptingApi::Engine::getUserPresetList() const
{
#if USE_BACKEND
	File userPresetRoot = GET_PROJECT_HANDLER(getProcessor()).getSubDirectory(ProjectHandler::SubDirectories::UserPresets);
#else
	File userPresetRoot = FrontendHandler::getUserPresetDirectory();
#endif

	Array<File> presets;

	userPresetRoot.findChildFiles(presets, File::findFiles, true, "*.preset");

	Array<var> list;

	for (auto& pr : presets)
	{
		auto name = pr.getRelativePathFrom(userPresetRoot).upToFirstOccurrenceOf(".preset", false, true);
		name = name.replaceCharacter('\\', '/');

		list.add(var(name));
	}

	return var(list);
}

void ScriptingApi::Engine::setAllowDuplicateSamples(bool shouldAllow)
{
	getProcessor()->getMainController()->getSampleManager().getModulatorSamplerSoundPool2()->setAllowDuplicateSamples(shouldAllow);
}

var ScriptingApi::Engine::loadAudioFilesIntoPool()
{
#if USE_BACKEND

	HiseJavascriptEngine::TimeoutExtender xt(dynamic_cast<JavascriptProcessor*>(getScriptProcessor())->getScriptEngine());

	auto pool = getScriptProcessor()->getMainController_()->getCurrentAudioSampleBufferPool();
	pool->loadAllFilesFromProjectFolder();

#endif

	auto allList = getScriptProcessor()->getMainController_()->getSampleManager().getProjectHandler().pool->getAudioSampleBufferPool().getListOfAllReferences(true);

	Array<var> ar;

	for (auto& ref : allList)
		ar.add(ref.getReferenceString());

	return var(ar);
}

void ScriptingApi::Engine::loadImageIntoPool(const String& id)
{
#if USE_BACKEND

	HiseJavascriptEngine::TimeoutExtender xt(dynamic_cast<JavascriptProcessor*>(getScriptProcessor())->getScriptEngine());

	auto mc = getScriptProcessor()->getMainController_();

	auto pool = mc->getCurrentImagePool();
	const bool isWildcard = id.contains("*");

	if (isWildcard)
	{
		File root = GET_PROJECT_HANDLER(dynamic_cast<Processor*>(getScriptProcessor())).getSubDirectory(ProjectHandler::Images);

		Array<File> results;

		auto wildcard = id.fromFirstOccurrenceOf("{PROJECT_FOLDER}", false, false).replace("*", "");

		root.findChildFiles(results, File::findFiles, true);

		for (auto r : results)
		{
			auto fileName = r.getRelativePathFrom(root).replace("\\", "/");

			if (wildcard.isEmpty() || fileName.contains(wildcard))
			{
				PoolReference ref(mc, r.getFullPathName(), ProjectHandler::Images);
				pool->loadFromReference(ref, PoolHelpers::LoadAndCacheStrong);
			}
		}
	}
	else
	{
		PoolReference r(mc, id, FileHandlerBase::Images);
		pool->loadFromReference(r, PoolHelpers::LoadAndCacheStrong);
	}
#else

	// We don't need that method in compiled plugins...
	ignoreUnused(id);

#endif
}

void ScriptingApi::Engine::clearSampleMapPool()
{
#if USE_BACKEND
	auto& smPool = getScriptProcessor()->getMainController_()->getCurrentFileHandler().pool->getSampleMapPool();
	auto numFiles = smPool.getNumLoadedFiles();
	smPool.clearData();
	debugToConsole(dynamic_cast<Processor*>(getScriptProcessor()), "SampleMap pool cleared: " + String(numFiles) + " entries removed");
#endif
}

void ScriptingApi::Engine::clearMidiFilePool()
{
#if USE_BACKEND
	auto& midiPool = getScriptProcessor()->getMainController_()->getCurrentFileHandler().pool->getMidiFilePool();
	auto numFiles = midiPool.getNumLoadedFiles();
	midiPool.clearData();
	debugToConsole(dynamic_cast<Processor*>(getScriptProcessor()), "MIDI file pool cleared: " + String(numFiles) + " entries removed");
#endif
}

void ScriptingApi::Engine::rebuildCachedPools()
{
#if USE_BACKEND

	auto& pools = getScriptProcessor()->getMainController_()->getCurrentFileHandler().pool;

	pools->getMidiFilePool().clearData();
	pools->getMidiFilePool().loadAllFilesFromProjectFolder();

	pools->getSampleMapPool().clearData();
	pools->getSampleMapPool().loadAllFilesFromProjectFolder();

#endif
}

DynamicObject * ScriptingApi::Engine::getPlayHead() { return getProcessor()->getMainController()->getHostInfoObject(); }

int ScriptingApi::Engine::isControllerUsedByAutomation(int controllerNumber)
{
	auto handler = getProcessor()->getMainController()->getMacroManager().getMidiControlAutomationHandler();

	for (int i = 0; i < handler->getNumActiveConnections(); i++)
	{
		if (handler->getDataFromIndex(i).ccNumber == controllerNumber)
			return i;
	}

	return -1;
}

ScriptingObjects::MidiList *ScriptingApi::Engine::createMidiList() { return new ScriptingObjects::MidiList(getScriptProcessor()); };

ScriptingObjects::ScriptSliderPackData* ScriptingApi::Engine::createSliderPackData() { return new ScriptingObjects::ScriptSliderPackData(getScriptProcessor()); }

hise::ScriptingObjects::ScriptSliderPackData* ScriptingApi::Engine::createAndRegisterSliderPackData(int index)
{
	if (auto jp = dynamic_cast<JavascriptProcessor*>(getScriptProcessor()))
	{
		return jp->addOrReturnSliderPackObject(index);
	}

	return nullptr;
}

hise::ScriptingObjects::ScriptTableData* ScriptingApi::Engine::createAndRegisterTableData(int index)
{
	if (auto jp = dynamic_cast<JavascriptProcessor*>(getScriptProcessor()))
	{
		return jp->addOrReturnTableObject(index);
	}

	return nullptr;
}

hise::ScriptingObjects::ScriptAudioFile* ScriptingApi::Engine::createAndRegisterAudioFile(int index)
{
	if (auto jp = dynamic_cast<JavascriptProcessor*>(getScriptProcessor()))
	{
		return jp->addOrReturnAudioFile(index);
	}

	return nullptr;
}

ScriptingObjects::TimerObject* ScriptingApi::Engine::createTimerObject() { return new ScriptingObjects::TimerObject(getScriptProcessor()); }

ScriptingObjects::ScriptingMessageHolder* ScriptingApi::Engine::createMessageHolder()
{
	return new ScriptingObjects::ScriptingMessageHolder(getScriptProcessor());
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
		f = File(fileName);
	else
		f = File(GET_PROJECT_HANDLER(getProcessor()).getSubDirectory(ProjectHandler::SubDirectories::UserPresets).getChildFile(fileName));

	f.replaceWithText(JSON::toString(object, false, DOUBLE_TO_STRING_DIGITS));

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
		return {};
}


void ScriptingApi::Engine::setCompileProgress(var progress)
{
	JavascriptProcessor *sp = dynamic_cast<JavascriptProcessor*>(getScriptProcessor());

	if (sp != nullptr)
		sp->setCompileProgress((double)progress);
}



bool ScriptingApi::Engine::matchesRegex(String stringToMatch, String wildcard)
{
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
}

var ScriptingApi::Engine::getRegexMatches(String stringToMatch, String wildcard)
{
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
}

String ScriptingApi::Engine::doubleToString(double value, int digits)
{
    return String(value, digits);
}

void ScriptingApi::Engine::undo()
{
	WeakReference<Processor> p = getProcessor();

	auto f = [p]()
	{
		if(p != nullptr)
			p.get()->getMainController()->getControlUndoManager()->undo();
	};

	MessageManager::callAsync(f);
}

void ScriptingApi::Engine::redo()
{
	WeakReference<Processor> p = getProcessor();

	auto f = [p]()
	{
		if (p != nullptr)
			p.get()->getMainController()->getControlUndoManager()->redo();
	};

	MessageManager::callAsync(f);
}

String ScriptingApi::Engine::getSystemTime(bool includeDividerCharacters)
{
	return Time::getCurrentTime().toISO8601(includeDividerCharacters);
};

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
	API_VOID_METHOD_WRAPPER_2(Sampler, setSoundPropertyForAllSamples);
	API_METHOD_WRAPPER_2(Sampler, getSoundProperty);
	API_VOID_METHOD_WRAPPER_3(Sampler, setSoundProperty);
	API_VOID_METHOD_WRAPPER_2(Sampler, purgeMicPosition);
	API_METHOD_WRAPPER_0(Sampler, getNumMicPositions);
	API_METHOD_WRAPPER_1(Sampler, isMicPositionPurged);
	API_METHOD_WRAPPER_1(Sampler, getMicPositionName);
	API_METHOD_WRAPPER_1(Sampler, isNoteNumberMapped);
	API_VOID_METHOD_WRAPPER_0(Sampler, refreshInterface);
	API_VOID_METHOD_WRAPPER_1(Sampler, loadSampleMap);
	API_METHOD_WRAPPER_0(Sampler, getSampleMapList);
    API_METHOD_WRAPPER_0(Sampler, getCurrentSampleMapId);
    API_VOID_METHOD_WRAPPER_2(Sampler, setAttribute);
    API_METHOD_WRAPPER_0(Sampler, getNumAttributes);
    API_METHOD_WRAPPER_1(Sampler, getAttribute);
    API_METHOD_WRAPPER_1(Sampler, getAttributeId);
	API_VOID_METHOD_WRAPPER_1(Sampler, setUseStaticMatrix);
    API_METHOD_WRAPPER_1(Sampler, loadSampleForAnalysis);
	API_METHOD_WRAPPER_1(Sampler, createSelection);
	API_METHOD_WRAPPER_1(Sampler, createSelectionFromIndexes);
	API_METHOD_WRAPPER_0(Sampler, createListFromGUISelection);
	API_METHOD_WRAPPER_0(Sampler, createListFromScriptSelection);
	API_METHOD_WRAPPER_1(Sampler, saveCurrentSampleMap);
	API_METHOD_WRAPPER_2(Sampler, importSamples);
	API_METHOD_WRAPPER_0(Sampler, clearSampleMap);
	API_VOID_METHOD_WRAPPER_1(Sampler, setSortByRRGroup);
};


ScriptingApi::Sampler::Sampler(ProcessorWithScriptingContent *p, ModulatorSampler *sampler_) :
ConstScriptingObject(p, SampleIds::numProperties),
sampler(sampler_)
{
	ADD_API_METHOD_1(enableRoundRobin);
	ADD_API_METHOD_1(setActiveGroup);
	ADD_API_METHOD_2(getRRGroupsForMessage);
	ADD_API_METHOD_0(refreshRRMap);
	ADD_API_METHOD_1(selectSounds);
	ADD_API_METHOD_0(getNumSelectedSounds);
	ADD_API_METHOD_2(setSoundPropertyForSelection);
	ADD_API_METHOD_2(setSoundPropertyForAllSamples);
	ADD_API_METHOD_2(getSoundProperty);
	ADD_API_METHOD_3(setSoundProperty);
	ADD_API_METHOD_2(purgeMicPosition);
	ADD_API_METHOD_1(getMicPositionName);
	ADD_API_METHOD_0(getNumMicPositions);
	ADD_API_METHOD_1(isMicPositionPurged);
	ADD_API_METHOD_0(refreshInterface);
	ADD_API_METHOD_1(loadSampleMap);
    ADD_API_METHOD_0(getCurrentSampleMapId);
	ADD_API_METHOD_0(getSampleMapList);
	ADD_API_METHOD_0(getNumAttributes);
    ADD_API_METHOD_1(getAttribute);
    ADD_API_METHOD_1(getAttributeId);
    ADD_API_METHOD_2(setAttribute);
	ADD_API_METHOD_1(isNoteNumberMapped);
    ADD_API_METHOD_1(loadSampleForAnalysis);
	ADD_API_METHOD_1(setUseStaticMatrix);
	ADD_API_METHOD_1(setSortByRRGroup);
	ADD_API_METHOD_1(createSelection);
	ADD_API_METHOD_1(createSelectionFromIndexes);
	ADD_API_METHOD_0(createListFromGUISelection);
	ADD_API_METHOD_0(createListFromScriptSelection);
	ADD_API_METHOD_1(saveCurrentSampleMap);
	ADD_API_METHOD_2(importSamples);
	ADD_API_METHOD_0(clearSampleMap);

	sampleIds.add(SampleIds::ID);
	sampleIds.add(SampleIds::FileName);
	sampleIds.add(SampleIds::Root);
	sampleIds.add(SampleIds::HiKey);
	sampleIds.add(SampleIds::LoKey);
	sampleIds.add(SampleIds::LoVel);
	sampleIds.add(SampleIds::HiVel);
	sampleIds.add(SampleIds::RRGroup);
	sampleIds.add(SampleIds::Volume);
	sampleIds.add(SampleIds::Pan);
	sampleIds.add(SampleIds::Normalized);
	sampleIds.add(SampleIds::Pitch);
	sampleIds.add(SampleIds::SampleStart);
	sampleIds.add(SampleIds::SampleEnd);
	sampleIds.add(SampleIds::SampleStartMod);
	sampleIds.add(SampleIds::LoopStart);
	sampleIds.add(SampleIds::LoopEnd);
	sampleIds.add(SampleIds::LoopXFade);
	sampleIds.add(SampleIds::LoopEnabled);
	sampleIds.add(SampleIds::LowerVelocityXFade);
	sampleIds.add(SampleIds::UpperVelocityXFade);
	sampleIds.add(SampleIds::SampleState);
	sampleIds.add(SampleIds::Reversed);

	for (int i = 1; i < sampleIds.size(); i++)
	{
		addConstant(sampleIds[i].toString(), (int)i);
	}
}

void ScriptingApi::Sampler::enableRoundRobin(bool shouldUseRoundRobin)
{
	WARN_IF_AUDIO_THREAD(true, ScriptGuard::IllegalApiCall);

	ModulatorSampler *s = static_cast<ModulatorSampler*>(sampler.get());

	if (s != nullptr)
	{
		s->setUseRoundRobinLogic(shouldUseRoundRobin);
	}
	else
	{
		reportScriptError("enableRoundRobin() only works with Samplers.");
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
		RETURN_IF_NO_THROW(0)
	}

	if (s->isRoundRobinEnabled())
	{
		reportScriptError("Round Robin is not disabled. Call 'Synth.enableRoundRobin(false)' before calling this method.");
		RETURN_IF_NO_THROW(0)
	}

	return s->getRRGroupsForMessage(noteNumber, velocity);
}

void ScriptingApi::Sampler::refreshRRMap()
{
	WARN_IF_AUDIO_THREAD(true, ScriptGuard::IllegalApiCall);



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


var ScriptingApi::Sampler::createSelection(String regex)
{
	WARN_IF_AUDIO_THREAD(true, ScriptGuard::IllegalApiCall);

	ModulatorSampler *s = static_cast<ModulatorSampler*>(sampler.get());

	if (s == nullptr)
	{
		reportScriptError("createSelection() only works with Samplers.");
		RETURN_IF_NO_THROW({});
	}

	SelectedItemSet<ModulatorSamplerSound::Ptr> newSet;

	ModulatorSamplerSound::selectSoundsBasedOnRegex(regex, s, newSet);

	Array<var> newSelection;

	for (auto sound : newSet)
	{
		newSelection.add(new ScriptingObjects::ScriptingSamplerSound(getScriptProcessor(), s, sound));
	}

	return var(newSelection);
}


var ScriptingApi::Sampler::createSelectionFromIndexes(var indexData)
{
	WARN_IF_AUDIO_THREAD(true, ScriptGuard::IllegalApiCall);

	ModulatorSampler *s = static_cast<ModulatorSampler*>(sampler.get());

	if (s == nullptr)
	{
		reportScriptError("selectSounds() only works with Samplers.");
		RETURN_IF_NO_THROW({});
	}

	Array<var> selection;

	if (auto ar = indexData.getArray())
	{
		for (auto index : *ar)
		{
			selection.add(new ScriptingObjects::ScriptingSamplerSound(
				getScriptProcessor(), s,
				dynamic_cast< ModulatorSamplerSound*>(s->getSound(index))));
		}
	}
	else
	{
		if (indexData.isInt() || indexData.isInt64())
		{
			if ((int)indexData == -1)
			{
				for (int i = 0; i < s->getNumSounds(); i++)
				{
					selection.add(new ScriptingObjects::ScriptingSamplerSound(
						getScriptProcessor(), s,
						dynamic_cast<ModulatorSamplerSound*>(s->getSound(i))));
				}
			}
			else
			{
				selection.add(new ScriptingObjects::ScriptingSamplerSound(
					getScriptProcessor(), s,
					dynamic_cast<ModulatorSamplerSound*>(s->getSound(indexData))));
			}
		}
	}

	return var(selection);
}

var ScriptingApi::Sampler::createListFromScriptSelection()
{
	ModulatorSampler *s = static_cast<ModulatorSampler*>(sampler.get());

	if (s == nullptr)
	{
		reportScriptError("createListFromScriptSelection() only works with Samplers.");
		RETURN_IF_NO_THROW({});
	}

	Array<var> newSelection;

	for (auto sound : soundSelection)
		newSelection.add(new ScriptingObjects::ScriptingSamplerSound(getScriptProcessor(), s, sound));

	return newSelection;
}

var ScriptingApi::Sampler::createListFromGUISelection()
{
#if USE_BACKEND
	ModulatorSampler *s = static_cast<ModulatorSampler*>(sampler.get());

	if (s == nullptr)
	{
		reportScriptError("createListFromGUISelection() only works with Samplers.");
		RETURN_IF_NO_THROW({});
	}

	Array<var> newSelection;

	{
		MessageManagerLock mm;
		const auto& selection = s->getSampleEditHandler()->getSelection();

		for (auto sound : selection)
			newSelection.add(new ScriptingObjects::ScriptingSamplerSound(getScriptProcessor(), s, sound));
	}

	return newSelection;
#else
	return {};
#endif
}

void ScriptingApi::Sampler::selectSounds(String regexWildcard)
{
	WARN_IF_AUDIO_THREAD(true, ScriptGuard::IllegalApiCall);

	ModulatorSampler *s = static_cast<ModulatorSampler*>(sampler.get());

	if (s == nullptr)
	{
		reportScriptError("selectSounds() only works with Samplers.");
		return;
	}

	ModulatorSamplerSound::selectSoundsBasedOnRegex(regexWildcard, s, soundSelection);

}

int ScriptingApi::Sampler::getNumSelectedSounds()
{
	WARN_IF_AUDIO_THREAD(true, ScriptGuard::IllegalApiCall);

	ModulatorSampler *s = static_cast<ModulatorSampler*>(sampler.get());



	if (s == nullptr)
	{
		reportScriptError("getNumSelectedSounds() only works with Samplers.");
		RETURN_IF_NO_THROW(-1)
	}

	return soundSelection.getNumSelected();
}

void ScriptingApi::Sampler::setSoundPropertyForSelection(int propertyId, var newValue)
{
	WARN_IF_AUDIO_THREAD(true, ScriptGuard::IllegalApiCall);

	ModulatorSampler *s = static_cast<ModulatorSampler*>(sampler.get());

	if (s == nullptr)
	{
		reportScriptError("setSoundsProperty() only works with Samplers.");
		return;
	}

	auto& sounds = soundSelection.getItemArray();
	auto id = sampleIds[propertyId];

	auto f = [sounds, id, newValue](Processor* /*p*/)
	{
		const int numSelected = sounds.size();

		for (int i = 0; i < numSelected; i++)
		{
			if (sounds[i].get() != nullptr)
				sounds[i]->setSampleProperty(id, newValue, false);
		}

		return SafeFunctionCall::OK;
	};

	s->callAsyncIfJobsPending(f);
}

void ScriptingApi::Sampler::setSoundPropertyForAllSamples(int propertyIndex, var newValue)
{
	WARN_IF_AUDIO_THREAD(true, ScriptGuard::IllegalApiCall);

	ModulatorSampler *s = static_cast<ModulatorSampler*>(sampler.get());

	if (s == nullptr)
	{
		reportScriptError("setSoundsProperty() only works with Samplers.");
		return;
	}

	auto id = sampleIds[propertyIndex];

	auto f = [id, newValue](Processor* p)
	{
		auto s = static_cast<ModulatorSampler*>(p);

		ModulatorSampler::SoundIterator iter(s);

		while (auto sound = iter.getNextSound())
			sound->setSampleProperty(id, newValue, false);

		return SafeFunctionCall::OK;
	};

	s->callAsyncIfJobsPending(f);
}

var ScriptingApi::Sampler::getSoundProperty(int propertyIndex, int soundIndex)
{
	WARN_IF_AUDIO_THREAD(true, ScriptGuard::IllegalApiCall);

	ModulatorSampler *s = static_cast<ModulatorSampler*>(sampler.get());

	if (s == nullptr)
	{
		reportScriptError("getSoundProperty() only works with Samplers.");
		RETURN_IF_NO_THROW(var())
	}

	if (auto sound = s->getSampleMap()->getSound(soundIndex))
	{
		auto id = sampleIds[propertyIndex];
		return sound->getSampleProperty(id);
	}
	else
	{
		reportScriptError("no sound with index " + String(soundIndex));
		RETURN_IF_NO_THROW(var())
	}
}

void ScriptingApi::Sampler::setSoundProperty(int soundIndex, int propertyIndex, var newValue)
{
	WARN_IF_AUDIO_THREAD(true, ScriptGuard::IllegalApiCall);

	ModulatorSampler *s = static_cast<ModulatorSampler*>(sampler.get());

	if (s == nullptr)
	{
		reportScriptError("setSoundProperty() only works with Samplers.");
		RETURN_VOID_IF_NO_THROW()
	}

	if (auto sound = soundSelection.getSelectedItem(soundIndex).get())
	{
		auto id = sampleIds[propertyIndex];
		sound->setSampleProperty(id, newValue, false);
	}
	else
	{
		reportScriptError("no sound with index " + String(soundIndex));
		RETURN_VOID_IF_NO_THROW()
	}
}

void ScriptingApi::Sampler::purgeMicPosition(String micName, bool shouldBePurged)
{
	WARN_IF_AUDIO_THREAD(true, ScriptGuard::IllegalApiCall);

	ModulatorSampler *s = static_cast<ModulatorSampler*>(sampler.get());

    if(micName.isEmpty())
    {
        reportScriptError("Mic position name must not be empty.");
        RETURN_VOID_IF_NO_THROW()
    }

	if (s == nullptr)
	{
		reportScriptError("purgeMicPosition() only works with Samplers.");
		RETURN_VOID_IF_NO_THROW()
	}

	if (!s->isUsingStaticMatrix() && s->getNumMicPositions() == 1)
	{
		reportScriptError("purgeMicPosition() only works with multi mic Samplers.");
		RETURN_VOID_IF_NO_THROW()
	}


	for (int i = 0; i < s->getNumMicPositions(); i++)
	{
		if (micName == s->getChannelData(i).suffix)
		{
			auto f = [i, shouldBePurged](Processor* p)
			{
				auto s = static_cast<ModulatorSampler*>(p);
				s->setMicEnabled(i, !shouldBePurged);
				return SafeFunctionCall::OK;
			};

			s->callAsyncIfJobsPending(f);
			return;
		}
	}

	reportScriptError("Channel not found. Use getMicPositionName()");
    RETURN_VOID_IF_NO_THROW()
}

String ScriptingApi::Sampler::getMicPositionName(int channelIndex)
{
	WARN_IF_AUDIO_THREAD(true, ScriptGuard::IllegalApiCall);

	ModulatorSampler *s = static_cast<ModulatorSampler*>(sampler.get());

	if (s == nullptr)
	{
		reportScriptError("getMicPositionName() only works with Samplers.");
		RETURN_IF_NO_THROW("")
	}

	if (!s->isUsingStaticMatrix() && s->getNumMicPositions() == 1)
	{
		reportScriptError("getMicPositionName() only works with multi mic Samplers.");
		RETURN_IF_NO_THROW("")
	}

	return s->getChannelData(channelIndex).suffix;
}



int ScriptingApi::Sampler::getNumMicPositions() const
{
	ModulatorSampler *s = static_cast<ModulatorSampler*>(sampler.get());

	if (s == nullptr)
	{
		reportScriptError("getNumMicPositions() only works with Samplers.");
		RETURN_IF_NO_THROW(0)
	}

	return s->getNumMicPositions();
}

bool ScriptingApi::Sampler::isMicPositionPurged(int micIndex)
{
	ModulatorSampler *s = static_cast<ModulatorSampler*>(sampler.get());

	if (s == nullptr)
	{
		reportScriptError("isMicPositionPurged() only works with Samplers.");
		RETURN_IF_NO_THROW(false)
	}

	if (micIndex >= 0 && micIndex < s->getNumMicPositions())
	{
		return !s->getChannelData(micIndex).enabled;
	}
	else return false;


}

bool ScriptingApi::Sampler::isNoteNumberMapped(int noteNumber)
{
	ModulatorSampler *s = static_cast<ModulatorSampler*>(sampler.get());

	if (s == nullptr)
	{
		reportScriptError("isNoteNumberMapped() only works with Samplers.");
		RETURN_IF_NO_THROW(false)
	}


	return s->isNoteNumberMapped(noteNumber);
}

void ScriptingApi::Sampler::refreshInterface()
{
	WARN_IF_AUDIO_THREAD(true, ScriptGuard::IllegalApiCall);

	ModulatorSampler *s = static_cast<ModulatorSampler*>(sampler.get());

	if (s == nullptr)
	{
		reportScriptError("refreshInterface() only works with Samplers.");
		RETURN_VOID_IF_NO_THROW()
	}

	s->sendChangeMessage();
	s->getMainController()->getSampleManager().getModulatorSamplerSoundPool2()->sendChangeMessage();
}

void ScriptingApi::Sampler::loadSampleMap(const String &fileName)
{
	WARN_IF_AUDIO_THREAD(true, ScriptGuard::IllegalApiCall);

	jassert(LockHelpers::isLockedBySameThread(getScriptProcessor()->getMainController_(), LockHelpers::ScriptLock));

	if (fileName.isEmpty())
		reportScriptError("Trying to load a empty sample map...");

	ModulatorSampler *s = static_cast<ModulatorSampler*>(sampler.get());

	if (s != nullptr)
	{
		PoolReference ref(s->getMainController(), fileName, FileHandlerBase::SampleMaps);

		if (!ref.isValid())
		{
			jassertfalse;
			reportScriptError("Samplemap " + ref.getReferenceString() + " is not valid");
			return;
		}

		s->killAllVoicesAndCall([ref](Processor* p) {dynamic_cast<ModulatorSampler*>(p)->loadSampleMap(ref); return SafeFunctionCall::OK; }, true);
	}
}

var ScriptingApi::Sampler::importSamples(var fileNameList, bool skipExistingSamples)
{
#if HI_ENABLE_EXPANSION_EDITING
	if (fileNameList.isArray() && fileNameList.getArray()->size() == 0)
		return fileNameList;

	WARN_IF_AUDIO_THREAD(true, ScriptGuard::IllegalApiCall);

	ModulatorSampler *s = static_cast<ModulatorSampler*>(sampler.get());

	if (s != nullptr)
	{
		auto start = Time::getMillisecondCounter();

		SuspendHelpers::ScopedTicket ticket(s->getMainController());

		s->getMainController()->getKillStateHandler().killVoicesAndWait();

		LockHelpers::SafeLock sl(s->getMainController(), LockHelpers::Type::SampleLock);

		auto sampleExists = [s](PoolReference r)
		{
			auto fileName = r.getFile().getFullPathName();

			for (int i = 0; i < s->getNumSounds(); i++)
			{
				if (auto sound = dynamic_cast<ModulatorSamplerSound*>(s->getSound(i)))
				{
					auto f = sound->getPropertyAsString(SampleIds::FileName);

					if (f == fileName)
						return true;
				}
			}

			return false;
		};

		if (auto ar = fileNameList.getArray())
		{
			StringArray list;

			for (auto& newSample : *ar)
			{
				PoolReference ref(s->getMainController(), newSample, FileHandlerBase::Samples);

				if (skipExistingSamples && sampleExists(ref))
					continue;

				list.add(ref.getFile().getFullPathName());
			}

			BigInteger rootNotes;
			// start at note number 0
			rootNotes.setRange(0, jmin(127, list.size()), true);

			int startIndex = s->getNumSounds();

			ScopedValueSetter<bool> syncFlag(s->getSampleMap()->getSyncEditModeFlag(), true);

			{
				//SampleMap::ScopedNotificationDelayer snd(*s->getSampleMap());

				SampleImporter::loadAudioFilesUsingDropPoint(nullptr, s, list, rootNotes);

				debugToConsole(s, "Imported " + String(list.size()) + " samples");
			}

			Array<var> refList;

			for (int i = 0; i < s->getNumSounds(); i++)
			{
				if (auto sound = dynamic_cast<ModulatorSamplerSound*>(s->getSound(i)))
				{
					auto id = sound->getPropertyAsString(SampleIds::ID);

					if(id.getIntValue() >= startIndex)
					{
						auto obj = new ScriptingObjects::ScriptingSamplerSound(getScriptProcessor(), s, sound);
						refList.add(var(obj));
					}
				}
			}

			return refList;
		}

		auto stop = Time::getMillisecondCounter();

		dynamic_cast<JavascriptProcessor*>(getScriptProcessor())->getScriptEngine()->extendTimeout(stop - start);

	}
    
#endif
    
    return {};
}

var ScriptingApi::Sampler::loadSampleForAnalysis(int soundIndex)
{
	WARN_IF_AUDIO_THREAD(true, ScriptGuard::IllegalApiCall);

	ModulatorSampler *s = static_cast<ModulatorSampler*>(sampler.get());

	if (s == nullptr)
	{
		reportScriptError("loadSampleForAnalysis() only works with Samplers.");
		RETURN_IF_NO_THROW({});
	}

    auto& sounds = soundSelection.getItemArray();

    if(auto sound = sounds[soundIndex].get())
    {
		ScopedPointer<ScriptingObjects::ScriptingSamplerSound> ownedSound = new ScriptingObjects::ScriptingSamplerSound(getScriptProcessor(), s, sound);
		return ownedSound->loadIntoBufferArray();
    }

    return {};
}

String ScriptingApi::Sampler::getCurrentSampleMapId() const
{
	WARN_IF_AUDIO_THREAD(true, ScriptGuard::IllegalApiCall);

    ModulatorSampler *s = static_cast<ModulatorSampler*>(sampler.get());

    if (s != nullptr)
    {
        auto map = s->getSampleMap();

        if(map != nullptr)
            return map->getId().toString();
    }

    return String();
}


var ScriptingApi::Sampler::getSampleMapList() const
{
	WARN_IF_AUDIO_THREAD(true, ScriptGuard::IllegalApiCall);

	Array<var> sampleMapNames;

	auto pool = getProcessor()->getMainController()->getCurrentSampleMapPool();
	auto references = pool->getListOfAllReferences(true);

	PoolReference::Comparator comparator;

	references.sort(comparator);

	sampleMapNames.ensureStorageAllocated(references.size());

	for (auto r : references)
		sampleMapNames.add(r.getReferenceString());



	return sampleMapNames;
}

int ScriptingApi::Sampler::getNumAttributes() const
{
    ModulatorSampler *s = static_cast<ModulatorSampler*>(sampler.get());

	if (checkValidObject())
	{
		return s->getNumParameters();
	}

	return 0;
}

var ScriptingApi::Sampler::getAttribute(int index) const
{
    ModulatorSampler *s = static_cast<ModulatorSampler*>(sampler.get());

    if (s == nullptr)
    {
        reportScriptError("getAttribute() only works with Samplers.");
		RETURN_IF_NO_THROW(var());
    }

    return s->getAttribute(index);
}

String ScriptingApi::Sampler::getAttributeId(int parameterIndex)
{
    ModulatorSampler *s = static_cast<ModulatorSampler*>(sampler.get());

    if (checkValidObject())
        return s->getIdentifierForParameterIndex(parameterIndex).toString();    
    
    return String();
}

void ScriptingApi::Sampler::setAttribute(int index, var newValue)
{
    ModulatorSampler *s = static_cast<ModulatorSampler*>(sampler.get());

    if (s == nullptr)
    {
        reportScriptError("setAttribute() only works with Samplers.");
        RETURN_VOID_IF_NO_THROW()
    }

    s->setAttribute(index, newValue, sendNotification);
}

void ScriptingApi::Sampler::setUseStaticMatrix(bool shouldUseStaticMatrix)
{
	WARN_IF_AUDIO_THREAD(true, ScriptGuard::IllegalApiCall);

	ModulatorSampler *s = static_cast<ModulatorSampler*>(sampler.get());

	if (s == nullptr)
	{
		reportScriptError("setAttribute() only works with Samplers.");
		RETURN_VOID_IF_NO_THROW()
	}

	s->setUseStaticMatrix(shouldUseStaticMatrix);
}

void ScriptingApi::Sampler::setSortByRRGroup(bool shouldSort)
{
	WARN_IF_AUDIO_THREAD(true, ScriptGuard::IllegalApiCall);

	ModulatorSampler *s = static_cast<ModulatorSampler*>(sampler.get());

	if (s == nullptr)
	{
		reportScriptError("setSortByRRGroup() only works with Samplers.");
		RETURN_VOID_IF_NO_THROW()
	}

	s->setSortByGroup(shouldSort);
}

bool ScriptingApi::Sampler::saveCurrentSampleMap(String relativePathWithoutXml)
{
	ModulatorSampler *s = static_cast<ModulatorSampler*>(sampler.get());

	if (s == nullptr)
	{
		reportScriptError("setAttribute() only works with Samplers.");
		RETURN_IF_NO_THROW(false);
	}

	if (s->getNumSounds() == 0)
	{
		debugToConsole(s, "Skipping creation of empty samplemap");
		return false;
	}

	if (auto sm = s->getSampleMap())
	{
		auto sampleMapsDirectory = s->getMainController()->getCurrentFileHandler().getSubDirectory(FileHandlerBase::SampleMaps);

		auto targetFile = sampleMapsDirectory.getChildFile(relativePathWithoutXml).withFileExtension("xml");

		if (targetFile.existsAsFile())
		{
			debugToConsole(s, "Overwriting file " + targetFile.getFullPathName());
			targetFile.deleteFile();
			targetFile.create();
		}
		else
		{
			targetFile.create();
		}

		return sm->save(targetFile);
	}
	else
		return false;
}

bool ScriptingApi::Sampler::clearSampleMap()
{
	ModulatorSampler *s = static_cast<ModulatorSampler*>(sampler.get());

	if (s == nullptr)
	{
		reportScriptError("clear() only works with Samplers.");
		RETURN_IF_NO_THROW(false);
	}

	if (auto sm = s->getSampleMap())
	{
		sm->clear(sendNotificationAsync);
		return true;
	}

	return false;
}

// ====================================================================================================== Synth functions



struct ScriptingApi::Synth::Wrapper
{
	API_METHOD_WRAPPER_0(Synth, getNumChildSynths);
	API_VOID_METHOD_WRAPPER_1(Synth, addToFront);
	API_VOID_METHOD_WRAPPER_1(Synth, deferCallbacks);
	API_VOID_METHOD_WRAPPER_1(Synth, noteOff);
	API_VOID_METHOD_WRAPPER_1(Synth, noteOffByEventId);
	API_VOID_METHOD_WRAPPER_2(Synth, noteOffDelayedByEventId);
	API_METHOD_WRAPPER_2(Synth, playNote);
	API_METHOD_WRAPPER_4(Synth, playNoteWithStartOffset);
	API_VOID_METHOD_WRAPPER_2(Synth, setAttribute);
	API_METHOD_WRAPPER_1(Synth, getAttribute);
	API_METHOD_WRAPPER_4(Synth, addNoteOn);
	API_VOID_METHOD_WRAPPER_3(Synth, addNoteOff);
	API_VOID_METHOD_WRAPPER_3(Synth, addVolumeFade);
	API_VOID_METHOD_WRAPPER_4(Synth, addPitchFade);
	API_VOID_METHOD_WRAPPER_4(Synth, addController);
	API_METHOD_WRAPPER_1(Synth, addMessageFromHolder);
	API_VOID_METHOD_WRAPPER_2(Synth, setVoiceGainValue);
	API_VOID_METHOD_WRAPPER_2(Synth, setVoicePitchValue);
	API_VOID_METHOD_WRAPPER_1(Synth, startTimer);
	API_VOID_METHOD_WRAPPER_0(Synth, stopTimer);
	API_METHOD_WRAPPER_0(Synth, isTimerRunning);
	API_METHOD_WRAPPER_0(Synth, getTimerInterval);
	API_VOID_METHOD_WRAPPER_2(Synth, setMacroControl);
	API_VOID_METHOD_WRAPPER_2(Synth, sendController);
	API_VOID_METHOD_WRAPPER_2(Synth, sendControllerToChildSynths);
	API_VOID_METHOD_WRAPPER_4(Synth, setModulatorAttribute);
	API_METHOD_WRAPPER_3(Synth, addModulator);
	API_METHOD_WRAPPER_1(Synth, removeModulator);
	API_METHOD_WRAPPER_3(Synth, addEffect);
	API_METHOD_WRAPPER_1(Synth, removeEffect);
	API_METHOD_WRAPPER_1(Synth, getModulator);
	API_METHOD_WRAPPER_1(Synth, getAudioSampleProcessor);
	API_METHOD_WRAPPER_1(Synth, getTableProcessor);
	API_METHOD_WRAPPER_1(Synth, getRoutingMatrix);
	API_METHOD_WRAPPER_1(Synth, getSampler);
	API_METHOD_WRAPPER_1(Synth, getSlotFX);
	API_METHOD_WRAPPER_1(Synth, getEffect);
	API_METHOD_WRAPPER_1(Synth, getAllEffects);
	API_METHOD_WRAPPER_1(Synth, getMidiProcessor);
	API_METHOD_WRAPPER_1(Synth, getChildSynth);
	API_METHOD_WRAPPER_1(Synth, getChildSynthByIndex);
	API_METHOD_WRAPPER_1(Synth, getMidiPlayer);
	API_METHOD_WRAPPER_1(Synth, getIdList);
	API_METHOD_WRAPPER_2(Synth, getModulatorIndex);
	API_METHOD_WRAPPER_1(Synth, getAllModulators);
	API_METHOD_WRAPPER_0(Synth, getNumPressedKeys);
	API_METHOD_WRAPPER_0(Synth, isLegatoInterval);
	API_METHOD_WRAPPER_0(Synth, isSustainPedalDown);
	API_METHOD_WRAPPER_1(Synth, isKeyDown);
	API_VOID_METHOD_WRAPPER_1(Synth, setClockSpeed);
	API_VOID_METHOD_WRAPPER_1(Synth, setShouldKillRetriggeredNote);
};


ScriptingApi::Synth::Synth(ProcessorWithScriptingContent *p, ModulatorSynth *ownerSynth) :
	ScriptingObject(p),
	ApiClass(0),
	moduleHandler(dynamic_cast<Processor*>(p), dynamic_cast<JavascriptProcessor*>(p)),
	owner(ownerSynth),
	numPressedKeys(0),
	keyDown(0),
	sustainState(false),
	parentMidiProcessor(dynamic_cast<ScriptBaseMidiProcessor*>(p)),
	jp(dynamic_cast<JavascriptMidiProcessor*>(p))
{
	jassert(owner != nullptr);

	keyDown.setRange(0, 128, false);

	ADD_API_METHOD_0(getNumChildSynths);
	ADD_API_METHOD_1(addToFront);
	ADD_API_METHOD_1(deferCallbacks);
	ADD_API_METHOD_1(noteOff);
	ADD_API_METHOD_1(noteOffByEventId);
	ADD_API_METHOD_2(noteOffDelayedByEventId);
	ADD_API_METHOD_2(playNote);
	ADD_API_METHOD_4(playNoteWithStartOffset);
	ADD_API_METHOD_2(setAttribute);
	ADD_API_METHOD_1(getAttribute);
	ADD_API_METHOD_4(addNoteOn);
	ADD_API_METHOD_3(addNoteOff);
	ADD_API_METHOD_3(addVolumeFade);
	ADD_API_METHOD_4(addPitchFade);
	ADD_API_METHOD_4(addController);
	ADD_API_METHOD_1(addMessageFromHolder);
	ADD_API_METHOD_2(setVoiceGainValue);
	ADD_API_METHOD_2(setVoicePitchValue);
	ADD_API_METHOD_1(startTimer);
	ADD_API_METHOD_0(stopTimer);
	ADD_API_METHOD_0(isTimerRunning);
	ADD_API_METHOD_0(getTimerInterval);
	ADD_API_METHOD_2(setMacroControl);
	ADD_API_METHOD_2(sendController);
	ADD_API_METHOD_2(sendControllerToChildSynths);
	ADD_API_METHOD_4(setModulatorAttribute);
	ADD_API_METHOD_3(addModulator);
	ADD_API_METHOD_3(addEffect);
	ADD_API_METHOD_1(getMidiPlayer);
	ADD_API_METHOD_1(removeEffect);
	ADD_API_METHOD_1(removeModulator);
	ADD_API_METHOD_1(getModulator);
	ADD_API_METHOD_1(getAudioSampleProcessor);
	ADD_API_METHOD_1(getTableProcessor);
	ADD_API_METHOD_1(getSampler);
	ADD_API_METHOD_1(getSlotFX);
	ADD_API_METHOD_1(getEffect);
	ADD_API_METHOD_1(getAllEffects);
	ADD_API_METHOD_1(getRoutingMatrix);
	ADD_API_METHOD_1(getMidiProcessor);
	ADD_API_METHOD_1(getChildSynth);
	ADD_API_METHOD_1(getChildSynthByIndex);
	ADD_API_METHOD_1(getIdList);
	ADD_API_METHOD_2(getModulatorIndex);
	ADD_API_METHOD_1(getAllModulators);
	ADD_API_METHOD_0(getNumPressedKeys);
	ADD_API_METHOD_0(isLegatoInterval);
	ADD_API_METHOD_0(isSustainPedalDown);
	ADD_API_METHOD_1(isKeyDown);
	ADD_API_METHOD_1(setClockSpeed);
	ADD_API_METHOD_1(setShouldKillRetriggeredNote);

};


int ScriptingApi::Synth::getNumChildSynths() const
{
	if(dynamic_cast<Chain*>(owner) == nullptr)
	{
		reportScriptError("getNumChildSynths() can only be called on Chains!");
		FRONTEND_ONLY(return -1;)
	}

	return dynamic_cast<Chain*>(owner)->getHandler()->getNumProcessors();
};

void ScriptingApi::Synth::noteOff(int noteNumber)
{
	jassert(owner != nullptr);

	addNoteOff(1, noteNumber, 0);

#if ENABLE_SCRIPTING_SAFE_CHECKS
	reportScriptError("noteOff is deprecated. Use noteOfByEventId instead");
#endif
}

void ScriptingApi::Synth::noteOffByEventId(int eventId)
{
	noteOffDelayedByEventId(eventId, 0);
}

void ScriptingApi::Synth::noteOffDelayedByEventId(int eventId, int timestamp)
{
	if (parentMidiProcessor == nullptr)
		reportScriptError("Can't call this outside of MIDI script processors");

	const HiseEvent e = getProcessor()->getMainController()->getEventHandler().popNoteOnFromEventId((uint16)eventId);



	if (!e.isEmpty())
	{
#if ENABLE_SCRIPTING_SAFE_CHECKS
		if (!e.isArtificial())
		{
			reportScriptError("Hell breaks loose if you kill real events artificially!");
		}
#endif
		const HiseEvent* current = parentMidiProcessor->getCurrentHiseEvent();

#if HISE_USE_BACKWARDS_COMPATIBLE_TIMESTAMPS

		if (getProcessor()->getMainController()->getKillStateHandler().getCurrentThread() == MainController::KillStateHandler::AudioThread)
		{
			// Apparently there was something wrong with the timestamp calculation.
		// This restores the old behaviour by removing one block from the timestamps.
		// By default, it's turned off, but you can enable it if you need backwards
		// compatibility with older patches...
			int blocksize = parentMidiProcessor->getMainController()->getBufferSizeForCurrentBlock();
			timestamp = jmax<int>(0, timestamp - blocksize);
		}
		
#endif

		if (current != nullptr)
		{
			timestamp += current->getTimeStamp();
		}

		HiseEvent noteOff(HiseEvent::Type::NoteOff, (uint8)e.getNoteNumber(), 1, (uint8)e.getChannel());
		noteOff.setEventId((uint16)eventId);
		noteOff.setTimeStamp(timestamp);

		if (e.isArtificial()) noteOff.setArtificial();

		parentMidiProcessor->addHiseEventToBuffer(noteOff);
	}
	else
	{
		if(!parentMidiProcessor->setArtificialTimestamp(eventId, timestamp))
			reportScriptError("NoteOn with ID" + String(eventId) + " wasn't found");
	}
}

void ScriptingApi::Synth::addToFront(bool addToFront)
{
	dynamic_cast<JavascriptMidiProcessor*>(getScriptProcessor())->addToFront(addToFront);
}

void ScriptingApi::Synth::deferCallbacks(bool deferCallbacks)
{
	dynamic_cast<JavascriptMidiProcessor*>(getScriptProcessor())->deferCallbacks(deferCallbacks);
}

int ScriptingApi::Synth::playNote(int noteNumber, int velocity)
{
	if(velocity == 0)
	{
		reportScriptError("A velocity of 0 is not valid!");
		RETURN_IF_NO_THROW(-1)
	}

	return internalAddNoteOn(1, noteNumber, velocity, 0, 0); // the timestamp will be added from the current event
}


int ScriptingApi::Synth::playNoteWithStartOffset(int channel, int number, int velocity, int offset)
{
	if (velocity == 0)
	{
		reportScriptError("A velocity of 0 is not valid!");
		RETURN_IF_NO_THROW(-1)
	}

	return internalAddNoteOn(channel, number, velocity, 0, offset); // the timestamp will be added from the current event
}

void ScriptingApi::Synth::addVolumeFade(int eventId, int fadeTimeMilliseconds, int targetVolume)
{
	if (parentMidiProcessor != nullptr)
	{
		if (eventId > 0)
		{
			if (fadeTimeMilliseconds >= 0)
			{
				HiseEvent e = HiseEvent::createVolumeFade((uint16)eventId, fadeTimeMilliseconds, (uint8)targetVolume);

				if (const HiseEvent* current = parentMidiProcessor->getCurrentHiseEvent())
				{
					e.setTimeStamp((int)current->getTimeStamp());
				}

				parentMidiProcessor->addHiseEventToBuffer(e);

                if(targetVolume == -100)
                {
                    HiseEvent no = getProcessor()->getMainController()->getEventHandler().popNoteOnFromEventId((uint16)eventId);

                    if (!no.isEmpty())
                    {
#if ENABLE_SCRIPTING_SAFE_CHECKS
                        if (!no.isArtificial())
                        {
                            reportScriptError("Hell breaks loose if you kill real events artificially!");
                        }
#endif
                        const int timeStampOffset = (int)(1.0 + (double)fadeTimeMilliseconds / 1000.0 * getProcessor()->getSampleRate());

                        int timestamp = timeStampOffset;

                        const HiseEvent* current = parentMidiProcessor->getCurrentHiseEvent();

                        if (current != nullptr)
                        {
                            timestamp += current->getTimeStamp();
                        }

                        HiseEvent noteOff(HiseEvent::Type::NoteOff, (uint8)no.getNoteNumber(), 1, (uint8)no.getChannel());
                        noteOff.setEventId((uint16)eventId);
                        noteOff.setTimeStamp(timestamp);
                        noteOff.setArtificial();

                        parentMidiProcessor->addHiseEventToBuffer(noteOff);

                    }
                    else
                    {
                        reportScriptError("NoteOn with ID" + String(eventId) + " wasn't found");
                    }
                }

			}
			else reportScriptError("Fade time must be positive");
		}
		else reportScriptError("Event ID must be positive");
	}
	else reportScriptError("Only valid in MidiProcessors");
}

void ScriptingApi::Synth::addPitchFade(int eventId, int fadeTimeMilliseconds, int targetCoarsePitch, int targetFinePitch)
{
	if (parentMidiProcessor != nullptr)
	{
		if (eventId > 0)
		{
			if (fadeTimeMilliseconds >= 0)
			{
				HiseEvent e = HiseEvent::createPitchFade((uint16)eventId, fadeTimeMilliseconds, (uint8)targetCoarsePitch, (uint8)targetFinePitch);

				if(auto ce = parentMidiProcessor->getCurrentHiseEvent())
					e.setTimeStamp((int)ce->getTimeStamp());

				parentMidiProcessor->addHiseEventToBuffer(e);
			}
			else reportScriptError("Fade time must be positive");
		}
		else reportScriptError("Event ID must be positive");
	}
	else reportScriptError("Only valid in MidiProcessors");
}

int ScriptingApi::Synth::addMessageFromHolder(var messageHolder)
{
	if (parentMidiProcessor != nullptr)
	{
		ScriptingObjects::ScriptingMessageHolder* m = dynamic_cast<ScriptingObjects::ScriptingMessageHolder*>(messageHolder.getObject());

		if (m != nullptr)
		{
			HiseEvent e = m->getMessageCopy();

			if (e.getType() != HiseEvent::Type::Empty)
			{
				e.setArtificial();

				if (e.isNoteOn())
				{
					parentMidiProcessor->getMainController()->getEventHandler().pushArtificialNoteOn(e);
					parentMidiProcessor->addHiseEventToBuffer(e);
					return e.getEventId();
				}
				else if (e.isNoteOff())
				{
					e.setEventId(parentMidiProcessor->getMainController()->getEventHandler().getEventIdForNoteOff(e));

					parentMidiProcessor->addHiseEventToBuffer(e);
					return e.getTimeStamp();
				}
				else
				{
					parentMidiProcessor->addHiseEventToBuffer(e);
					return 0;
				}
			}
			else reportScriptError("Event is empty");
		}
		else reportScriptError("Not a message holder");
	}
	else reportScriptError("Only valid in MidiProcessors");

	return 0;
}

void ScriptingApi::Synth::startTimer(double intervalInSeconds)
{
#if ENABLE_SCRIPTING_SAFE_CHECKS
	if(intervalInSeconds < 0.004)
	{
		reportScriptError("Go easy on the timer!");
		return;
	}
#endif






	if (parentMidiProcessor == nullptr)
		reportScriptError("Timers only work in MIDI processors!");

	if(jp != nullptr && jp->isDeferred())
	{
		owner->stopSynthTimer(parentMidiProcessor->getIndexInChain());
		jp->startTimer((int)(intervalInSeconds * 1000));
		parentMidiProcessor->setIndexInChain(-1);
	}
	else
	{
		int freeTimerSlot = parentMidiProcessor->getIndexInChain() != -1 ? parentMidiProcessor->getIndexInChain() : owner->getFreeTimerSlot();

		if (freeTimerSlot == -1)
		{
			reportScriptError("All 4 timers are used");
			return;
		}

		parentMidiProcessor->setIndexInChain(freeTimerSlot);

		auto* e = parentMidiProcessor->getCurrentHiseEvent();

		int timestamp = 0;

		if (e != nullptr)
		{
			timestamp = e->getTimeStamp();
		}

		owner->startSynthTimer(parentMidiProcessor->getIndexInChain(), intervalInSeconds, timestamp);
	}
}

void ScriptingApi::Synth::stopTimer()
{
	if(jp != nullptr && jp->isDeferred())
	{
		owner->stopSynthTimer(jp->getIndexInChain());
		jp->stopTimer();
	}
	else
	{
		if(parentMidiProcessor != nullptr) owner->stopSynthTimer(parentMidiProcessor->getIndexInChain());

		parentMidiProcessor->setIndexInChain(-1);
	}
}

bool ScriptingApi::Synth::isTimerRunning() const
{


	if (jp != nullptr && jp->isDeferred())
	{
		return jp->isTimerRunning();

	}
	else
	{
		if (parentMidiProcessor != nullptr)
			return owner->getTimerInterval(parentMidiProcessor->getIndexInChain()) != 0.0;
		else return false;
	}
}

double ScriptingApi::Synth::getTimerInterval() const
{


	if (jp != nullptr && jp->isDeferred())
	{
		return (double)jp->getTimerInterval() / 1000.0;
	}
	else
	{
		if (parentMidiProcessor != nullptr)
			return owner->getTimerInterval(parentMidiProcessor->getIndexInChain());
		else return 0.0;
	}
}

void ScriptingApi::Synth::sendController(int controllerNumber, int controllerValue)
{
	if (parentMidiProcessor != nullptr)
	{
		if (controllerNumber > 0)
		{
			if (controllerValue >= 0)
			{
                HiseEvent e;

                if(controllerNumber == 129)
                {
                    e = HiseEvent(HiseEvent::Type::PitchBend, 0, 0);
                    e.setPitchWheelValue(controllerValue);
                }
                else
                {
                    e = HiseEvent(HiseEvent::Type::Controller, (uint8)controllerNumber, (uint8)controllerValue);
                }


				if (const HiseEvent* current = parentMidiProcessor->getCurrentHiseEvent())
				{
					e.setTimeStamp((int)current->getTimeStamp());
				}

				parentMidiProcessor->addHiseEventToBuffer(e);
			}
			else reportScriptError("CC value must be positive");
		}
		else reportScriptError("CC number must be positive");
	}
	else reportScriptError("Only valid in MidiProcessors");
};

void ScriptingApi::Synth::sendControllerToChildSynths(int controllerNumber, int controllerValue)
{
	sendController(controllerNumber, controllerValue);
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
		RETURN_IF_NO_THROW(new ScriptingObjects::ScriptingModulator(getScriptProcessor(), nullptr))
	}
	else
	{
		reportIllegalCall("getModulator()", "onInit");
		RETURN_IF_NO_THROW(new ScriptingObjects::ScriptingModulator(getScriptProcessor(), nullptr))
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
				return new ScriptingObjects::ScriptingMidiProcessor(getScriptProcessor(), mp);
			}
		}

        reportScriptError(name + " was not found. ");

		RETURN_IF_NO_THROW(new ScriptMidiProcessor(getScriptProcessor(), nullptr))
	}
	else
	{
		reportIllegalCall("getMidiProcessor()", "onInit");

		RETURN_IF_NO_THROW(new ScriptMidiProcessor(getScriptProcessor(), nullptr))

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
				return new ScriptingObjects::ScriptingSynth(getScriptProcessor(), m);
			}
		}

        reportScriptError(name + " was not found. ");
		RETURN_IF_NO_THROW(new ScriptingObjects::ScriptingSynth(getScriptProcessor(), nullptr))
	}
	else
	{
		reportIllegalCall("getChildSynth()", "onInit");
		RETURN_IF_NO_THROW(new ScriptingObjects::ScriptingSynth(getScriptProcessor(), nullptr))

	}
}

ScriptingApi::Synth::ScriptSynth* ScriptingApi::Synth::getChildSynthByIndex(int index)
{
	if (getScriptProcessor()->objectsCanBeCreated())
	{
		if (Chain* c = dynamic_cast<Chain*>(owner))
		{
			if (index >= 0 && index < c->getHandler()->getNumProcessors())
			{
				return new ScriptingObjects::ScriptingSynth(getScriptProcessor(), dynamic_cast<ModulatorSynth*>(c->getHandler()->getProcessor(index)));
			}
		}

		return new ScriptingObjects::ScriptingSynth(getScriptProcessor(), nullptr);
	}
	else
	{
		reportIllegalCall("getChildSynth()", "onInit");
		RETURN_IF_NO_THROW(new ScriptingObjects::ScriptingSynth(getScriptProcessor(), nullptr))
	}
}

var ScriptingApi::Synth::getIdList(const String &type)
{
	if (getScriptProcessor()->objectsCanBeCreated())
	{
		Processor::Iterator<Processor> it(owner);

		Array<var> idList;

		while (Processor* p = it.getNextProcessor())
		{
			// don't include selfie-boy
			if (dynamic_cast<Processor*>(getScriptProcessor()) == p)
				continue;

			if (p->getName() == type)
				idList.add(p->getId());
		}

		return var(idList);
	}
	else
	{
		return var();
	}
}

ScriptingObjects::ScriptingEffect *ScriptingApi::Synth::getEffect(const String &name)
{
	WARN_IF_AUDIO_THREAD(true, ScriptGuard::ObjectCreation);

	if(getScriptProcessor()->objectsCanBeCreated())
	{
		Processor::Iterator<EffectProcessor> it(owner);

		EffectProcessor *fx;


		while((fx = it.getNextProcessor()) != nullptr)
		{
			if(fx->getId() == name)
			{
				return new ScriptEffect(getScriptProcessor(), fx);
			}
		}

        reportScriptError(name + " was not found. ");
		RETURN_IF_NO_THROW(new ScriptEffect(getScriptProcessor(), nullptr))
	}
	else
	{
		reportIllegalCall("getEffect()", "onInit");
		RETURN_IF_NO_THROW(new ScriptEffect(getScriptProcessor(), nullptr))
	}
}


var ScriptingApi::Synth::getAllEffects(String regex)
{
	WARN_IF_AUDIO_THREAD(true, ScriptGuard::ObjectCreation);

	if(getScriptProcessor()->objectsCanBeCreated())
	{
	    Array<var> list;
	    
		Processor::Iterator<EffectProcessor> it(owner);

		EffectProcessor *fx;

		while((fx = it.getNextProcessor()) != nullptr)
		{
			if (RegexFunctions::matchesWildcard(regex, fx->getId()))
			{
				list.add(new ScriptEffect(getScriptProcessor(), fx));
			}
		}

        return var(list);
    }

	RETURN_IF_NO_THROW(var());
}

ScriptingObjects::ScriptingAudioSampleProcessor * ScriptingApi::Synth::getAudioSampleProcessor(const String &name)
{
	WARN_IF_AUDIO_THREAD(true, ScriptGuard::ObjectCreation);

	Processor::Iterator<AudioSampleProcessor> it(owner);

		AudioSampleProcessor *asp;


		while ((asp = it.getNextProcessor()) != nullptr)
		{
			if (dynamic_cast<Processor*>(asp)->getId() == name)
			{
				return new ScriptAudioSampleProcessor(getScriptProcessor(), asp);
			}
		}

        reportScriptError(name + " was not found. ");
		RETURN_IF_NO_THROW(new ScriptAudioSampleProcessor(getScriptProcessor(), nullptr))
}



ScriptingObjects::ScriptingTableProcessor *ScriptingApi::Synth::getTableProcessor(const String &name)
{
	WARN_IF_AUDIO_THREAD(true, ScriptGuard::ObjectCreation);

	if (getScriptProcessor()->objectsCanBeCreated())
	{
		Processor::Iterator<LookupTableProcessor> it(owner);

		while (LookupTableProcessor *lut = it.getNextProcessor())
		{
			if (dynamic_cast<Processor*>(lut)->getId() == name)
			{

				return new ScriptTableProcessor(getScriptProcessor(), lut);
			}
		}

        reportScriptError(name + " was not found. ");
		RETURN_IF_NO_THROW(new ScriptTableProcessor(getScriptProcessor(), nullptr));
	}
	else
	{
		reportIllegalCall("getScriptingTableProcessor()", "onInit");
		RETURN_IF_NO_THROW(new ScriptTableProcessor(getScriptProcessor(), nullptr));
	}
}

ScriptingApi::Sampler * ScriptingApi::Synth::getSampler(const String &name)
{
	WARN_IF_AUDIO_THREAD(true, ScriptGuard::ObjectCreation);

	if (getScriptProcessor()->objectsCanBeCreated())
	{
		Processor::Iterator<ModulatorSampler> it(owner);

		while (ModulatorSampler *s = it.getNextProcessor())
		{
			if (s->getId() == name)
			{

				return new Sampler(getScriptProcessor(), s);
			}
		}

        reportScriptError(name + " was not found. ");
		RETURN_IF_NO_THROW(new Sampler(getScriptProcessor(), nullptr))
	}
	else
	{
		reportIllegalCall("getScriptingAudioSampleProcessor()", "onInit");
		RETURN_IF_NO_THROW(new Sampler(getScriptProcessor(), nullptr))
	}
}

ScriptingApi::Synth::ScriptSlotFX* ScriptingApi::Synth::getSlotFX(const String& name)
{
	WARN_IF_AUDIO_THREAD(true, ScriptGuard::ObjectCreation);

	if (getScriptProcessor()->objectsCanBeCreated())
	{
		Processor::Iterator<SlotFX> it(owner);

		while (SlotFX *s = it.getNextProcessor())
		{
			if (s->getId() == name)
			{

				return new ScriptSlotFX(getScriptProcessor(), s);
			}
		}

		reportScriptError(name + " was not found. ");
		RETURN_IF_NO_THROW(new ScriptSlotFX(getScriptProcessor(), nullptr))
	}
	else
	{
		reportIllegalCall("getScriptingAudioSampleProcessor()", "onInit");
		RETURN_IF_NO_THROW(new ScriptSlotFX(getScriptProcessor(), nullptr))
	}
}

ScriptingObjects::ScriptedMidiPlayer* ScriptingApi::Synth::getMidiPlayer(const String& playerId)
{
	auto p = ProcessorHelpers::getFirstProcessorWithName(getScriptProcessor()->getMainController_()->getMainSynthChain(), playerId);

	if (p == nullptr)
		reportScriptError(playerId + " was not found");

	if (auto mp = dynamic_cast<MidiPlayer*>(p))
		return new ScriptingObjects::ScriptedMidiPlayer(getScriptProcessor(), mp);
	else
		reportScriptError(playerId + " is not a MIDI Player");

	RETURN_IF_NO_THROW(new ScriptingObjects::ScriptedMidiPlayer(getScriptProcessor(), nullptr));
}

hise::ScriptingApi::Synth::ScriptRoutingMatrix* ScriptingApi::Synth::getRoutingMatrix(const String& processorId)
{
	auto p = ProcessorHelpers::getFirstProcessorWithName(getScriptProcessor()->getMainController_()->getMainSynthChain(), processorId);

	if (p == nullptr)
		reportScriptError(processorId + " was not found");

	if (auto rt = dynamic_cast<RoutableProcessor*>(p))
		return new ScriptingObjects::ScriptRoutingMatrix(getScriptProcessor(), p);
	else
		reportScriptError(processorId + " does not have a routing matrix");

	RETURN_IF_NO_THROW(new ScriptingObjects::ScriptRoutingMatrix(getScriptProcessor(), nullptr));
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

int ScriptingApi::Synth::internalAddNoteOn(int channel, int noteNumber, int velocity, int timeStampSamples, int startOffset)
{
	if (channel > 0 && channel <= 16)
	{
		if (noteNumber >= 0 && noteNumber < 127)
		{
			if (velocity >= 0 && velocity <= 127)
			{
				if (timeStampSamples >= 0)
				{
					if (parentMidiProcessor != nullptr)
					{
						HiseEvent m = HiseEvent(HiseEvent::Type::NoteOn, (uint8)noteNumber, (uint8)velocity, (uint8)channel);


#if HISE_USE_BACKWARDS_COMPATIBLE_TIMESTAMPS

						if (getProcessor()->getMainController()->getKillStateHandler().getCurrentThread() == MainController::KillStateHandler::AudioThread)
						{
							// Apparently there was something wrong with the timestamp calculation.
						// This restores the old behaviour by removing one block from the timestamps.
						// By default, it's turned off, but you can enable it if you need backwards
						// compatibility with older patches...
							int blocksize = parentMidiProcessor->getMainController()->getBufferSizeForCurrentBlock();
							timeStampSamples = jmax<int>(0, timeStampSamples - blocksize);
						}
#endif

						if (auto ce = parentMidiProcessor->getCurrentHiseEvent())
						{
							m.setTimeStamp((int)ce->getTimeStamp() + timeStampSamples);
						}
						else
						{
							m.setTimeStamp(timeStampSamples);
						}

						if (startOffset > UINT16_MAX)
							reportScriptError("Max start offset is 65536 (2^16)");

						m.setStartOffset((uint16)startOffset);

						m.setArtificial();

						parentMidiProcessor->getMainController()->getEventHandler().pushArtificialNoteOn(m);
						parentMidiProcessor->addHiseEventToBuffer(m);

						return m.getEventId();
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

	RETURN_IF_NO_THROW(-1)
}

int ScriptingApi::Synth::addNoteOn(int channel, int noteNumber, int velocity, int timeStampSamples)
{
	return internalAddNoteOn(channel, noteNumber, velocity, timeStampSamples, 0);
}

void ScriptingApi::Synth::addNoteOff(int channel, int noteNumber, int timeStampSamples)
{
	if (channel > 0 && channel <= 16)
	{
		if (noteNumber >= 0 && noteNumber < 127)
		{
			if (timeStampSamples >= 0)
			{
				if (parentMidiProcessor != nullptr)
				{
					timeStampSamples = jmax<int>(1, timeStampSamples);

					HiseEvent m = HiseEvent(HiseEvent::Type::NoteOff, (uint8)noteNumber, 127, (uint8)channel);

					if (auto ce = parentMidiProcessor->getCurrentHiseEvent())
					{
						m.setTimeStamp((int)ce->getTimeStamp() + timeStampSamples);
					}
					else
					{
						m.setTimeStamp(timeStampSamples);
					}

					m.setArtificial();

					const uint16 eventId = parentMidiProcessor->getMainController()->getEventHandler().getEventIdForNoteOff(m);

					m.setEventId(eventId);

					parentMidiProcessor->addHiseEventToBuffer(m);

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
					if (parentMidiProcessor != nullptr)
					{
						HiseEvent m = HiseEvent(HiseEvent::Type::Controller, (uint8)number, (uint8)value, (uint8)channel);

						if (auto ce = parentMidiProcessor->getCurrentHiseEvent())
						{
							m.setTimeStamp((int)ce->getTimeStamp() + timeStampSamples);
						}
						else
						{
							m.setTimeStamp(timeStampSamples);
						}

						m.setArtificial();

						parentMidiProcessor->addHiseEventToBuffer(m);
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

void ScriptingApi::Synth::setShouldKillRetriggeredNote(bool killNote)
{
	if (owner != nullptr)
	{
		owner->setKillRetriggeredNote(killNote);
	}
}

var ScriptingApi::Synth::getAllModulators(String regex)
{
	Processor::Iterator<Modulator> iter(owner->getMainController()->getMainSynthChain());

	Array<var> list;

	while (auto m = iter.getNextProcessor())
	{
		if (RegexFunctions::matchesWildcard(regex, m->getId()))
		{
			auto sm = new ScriptingObjects::ScriptingModulator(getScriptProcessor(), m);
			var smv(sm);

			list.add(smv);
		}
	}

	return var(list);
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


ScriptingObjects::ScriptingModulator* ScriptingApi::Synth::addModulator(int chain, const String &type, const String &id)
{
	ModulatorChain *c = nullptr;

	switch(chain)
	{
	case ModulatorSynth::GainModulation:	c = owner->gainChain; break;
	case ModulatorSynth::PitchModulation:	c = owner->pitchChain; break;
	default: jassertfalse; reportScriptError("No valid chainType - 1= GainModulation, 2=PitchModulation"); return nullptr;
	}

	Processor* p = moduleHandler.addModule(c, type, id, -1);

	if (p == nullptr)
		reportScriptError("Processor with id " + id + " could not be generated");

	return new ScriptingObjects::ScriptingModulator(getScriptProcessor(), dynamic_cast<Modulator*>(p));
}

bool ScriptingApi::Synth::removeModulator(var mod)
{
	if (auto m = dynamic_cast<ScriptingObjects::ScriptingModulator*>(mod.getObject()))
	{
		Modulator* modToRemove = m->getModulator();

		return moduleHandler.removeModule(modToRemove);
	}

	return false;
}

ScriptingApi::Synth::ScriptEffect* ScriptingApi::Synth::addEffect(const String &type, const String &id, int index)
{
	EffectProcessorChain* c = owner->effectChain;
	Processor* p = moduleHandler.addModule(c, type, id, index);

	return new ScriptingObjects::ScriptingEffect(getScriptProcessor(), dynamic_cast<EffectProcessor*>(p));
}

bool ScriptingApi::Synth::removeEffect(var effect)
{
	if (auto fx = dynamic_cast<ScriptingObjects::ScriptingEffect*>(effect.getObject()))
	{
		EffectProcessor* effectToRemove = fx->getEffect();
		return moduleHandler.removeModule(effectToRemove);
	}

	return false;
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

	RETURN_IF_NO_THROW(-1)
}

// ====================================================================================================== Console functions

struct ScriptingApi::Console::Wrapper
{
	API_VOID_METHOD_WRAPPER_1(Console, print);
	API_VOID_METHOD_WRAPPER_0(Console, start);
	API_VOID_METHOD_WRAPPER_0(Console, stop);
	API_VOID_METHOD_WRAPPER_0(Console, clear);
	API_VOID_METHOD_WRAPPER_1(Console, assertTrue);
	API_VOID_METHOD_WRAPPER_2(Console, assertEqual);
	API_VOID_METHOD_WRAPPER_1(Console, assertIsDefined);
	API_VOID_METHOD_WRAPPER_1(Console, assertIsObjectOrArray);
	API_VOID_METHOD_WRAPPER_1(Console, assertLegalNumber);
	API_VOID_METHOD_WRAPPER_0(Console, breakInDebugger);
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

	ADD_API_METHOD_1(assertTrue);
	ADD_API_METHOD_2(assertEqual);
	ADD_API_METHOD_1(assertIsDefined);
	ADD_API_METHOD_1(assertIsObjectOrArray);
	ADD_API_METHOD_1(assertLegalNumber);

	ADD_API_METHOD_0(breakInDebugger);
}



void ScriptingApi::Console::print(var x)
{
#if USE_BACKEND

	AudioThreadGuard::Suspender suspender;
	ignoreUnused(suspender);
	debugToConsole(getProcessor(), x);
#endif
}

void ScriptingApi::Console::stop()
{
#if USE_BACKEND
	AudioThreadGuard::Suspender suspender;
	ignoreUnused(suspender);

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
	getProcessor()->getMainController()->getConsoleHandler().clearConsole();
}



void ScriptingApi::Console::assertTrue(var condition)
{
	AudioThreadGuard::Suspender suspender;
	ignoreUnused(suspender);

	if (!(bool)condition)
		reportScriptError("Assertion failure: condition is false");
}

void ScriptingApi::Console::assertEqual(var v1, var v2)
{
	AudioThreadGuard::Suspender suspender;
	ignoreUnused(suspender);

	if (v1 != v2)
		reportScriptError("Assertion failure: values are unequal");
}

void ScriptingApi::Console::assertIsDefined(var v1)
{
	AudioThreadGuard::Suspender suspender;
	ignoreUnused(suspender);

	if (v1.isUndefined() || v1.isVoid())
		reportScriptError("Assertion failure: value is undefined");
}

struct VarTypeHelpers
{
	static bool isFunction(const var& v) noexcept
	{
		return dynamic_cast<HiseJavascriptEngine::RootObject::FunctionObject*> (v.getObject()) != nullptr;
	}

	static bool isNumeric(const var& v) noexcept
	{
		return v.isInt() || v.isDouble() || v.isInt64() || v.isBool();
	}

	static String getVarType(var v)
	{
		if (v.isVoid())                      return "void";
		if (v.isString())                    return "string";
		if (isNumeric(v))                   return "number";
		if (isFunction(v) || v.isMethod())  return "function";
		if (v.isObject())                    return "object";

		return "undefined";
	}
};



void ScriptingApi::Console::assertIsObjectOrArray(var value)
{
	if (!(value.isObject() || value.isArray()))
	{
		reportScriptError("Assertion failure: value is not object or array. Type: " + VarTypeHelpers::getVarType(value));
	}
}

void ScriptingApi::Console::assertLegalNumber(var value)
{
	if (!VarTypeHelpers::isNumeric(value))
	{
		reportScriptError("Assertion failure: value is not a number. Type: " + VarTypeHelpers::getVarType(value) + " Value: " + value.toString());
	}

	const float v1 = (float)value;
	float v2 = v1;


	if (v1 != FloatSanitizers::sanitizeFloatNumber(v2))
	{
		reportScriptError("Assertion failure: value is not a legal number. Value: " + value.toString());
	}
}

void ScriptingApi::Console::breakInDebugger()
{
	// There you go...
	jassertfalse;
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

ScriptingApi::ModuleIds::ModuleIds(ModulatorSynth* s):
	ApiClass(getTypeList(s).size()),
	ownerSynth(s)
{
	auto list = getTypeList(ownerSynth);

	list.sort();

	for (int i = 0; i < list.size(); i++)
	{
		addConstant(list[i].toString(), list[i].toString());
	}
}

Array<Identifier> ScriptingApi::ModuleIds::getTypeList(ModulatorSynth* s)
{
	Array<Identifier> ids;

	for (int i = 0; i < s->getNumInternalChains(); i++)
	{
		Chain* c = dynamic_cast<Chain*>(s->getChildProcessor(i));



		jassert(c != nullptr);

		if (c != nullptr)
		{
			FactoryType* t = c->getFactoryType();
			auto list = t->getAllowedTypes();

			for (int j = 0; j < list.size(); j++)
			{
				ids.addIfNotAlreadyThere(list[j].type);
			}
		}
	}

	return ids;
}

struct ScriptingApi::FileSystem::Wrapper
{
	API_METHOD_WRAPPER_1(FileSystem, getFolder);
	API_METHOD_WRAPPER_3(FileSystem, findFiles);
	API_METHOD_WRAPPER_0(FileSystem, getSystemId);
	API_VOID_METHOD_WRAPPER_4(FileSystem, browse);
};

ScriptingApi::FileSystem::FileSystem(ProcessorWithScriptingContent* pwsc):
	ApiClass((int)numSpecialLocations),
	ScriptingObject(pwsc),
	ControlledObject(pwsc->getMainController_()),
	p(pwsc)
{
	addConstant("Samples", (int)Samples);
	addConstant("Expansions", (int)Expansions);
	addConstant("AudioFiles", (int)AudioFiles);
	addConstant("UserPresets", (int)UserPresets);
	addConstant("AppData", (int)AppData);
	addConstant("UserHome", (int)UserHome);
	addConstant("Documents", (int)Documents);
	addConstant("Desktop", (int)Desktop);
	addConstant("Downloads", (int)Downloads);

	ADD_API_METHOD_1(getFolder);
	ADD_API_METHOD_3(findFiles);
	ADD_API_METHOD_0(getSystemId);
	ADD_API_METHOD_4(browse);
}

ScriptingApi::FileSystem::~FileSystem()
{

}


var ScriptingApi::FileSystem::getFolder(var locationType)
{
	auto l = (SpecialLocations)(int)locationType;
	auto f = getFile(l);

	if (f.isDirectory())
		return new ScriptingObjects::ScriptFile(p, f);

	return {};
}

var ScriptingApi::FileSystem::findFiles(var directory, String wildcard, bool recursive)
{
	Array<var> l;

	if (auto root = dynamic_cast<ScriptingObjects::ScriptFile*>(directory.getObject()))
	{
		if (root->isDirectory())
		{
			auto list = root->f.findChildFiles(File::findFilesAndDirectories, recursive, wildcard);

			for (auto sf : list)
				l.add(new ScriptingObjects::ScriptFile(p, sf));
		}
	}

	return l;
}

void ScriptingApi::FileSystem::browse(var startFolder, bool forSaving, String wildcard, var callback)
{
	File f;

	if (startFolder.isInt())
		f = getFile((SpecialLocations)(int)startFolder);
	else if (auto sf = dynamic_cast<ScriptingObjects::ScriptFile*>(startFolder.getObject()))
		f = sf->f;

	auto p_ = p;

	auto cb = [forSaving, f, wildcard, callback, p_]()
	{
    FileChooser fc(!forSaving ? "Open file" : "Save file", f, wildcard);

		var a;

		if (forSaving && fc.browseForFileToSave(true))
		{
			a = var(new ScriptingObjects::ScriptFile(p_, fc.getResult()));
		}
		if (!forSaving && fc.browseForFileToOpen())
		{
			a = var(new ScriptingObjects::ScriptFile(p_, fc.getResult()));
		}

		if (a.isObject())
		{
			WeakCallbackHolder cb(p_, callback, 1);
			cb.call(&a, 1);

#if 0
			p_->getMainController_()->getJavascriptThreadPool().addJob(JavascriptThreadPool::Task::HiPriorityCallbackExecution,
				dynamic_cast<JavascriptProcessor*>(p_), [callback, a](JavascriptProcessor* p)
			{
				var::NativeFunctionArgs args({}, &a, 1);
				auto r = Result::ok();
				p->getScriptEngine()->callExternalFunction(callback, args, &r);

				if (!r.wasOk())
					debugError(dynamic_cast<Processor*>(p), r.getErrorMessage());

				return r;
			});
#endif
		}
	};

	MessageManager::callAsync(cb);
}

String ScriptingApi::FileSystem::getSystemId()
{
	return OnlineUnlockStatus::MachineIDUtilities::getLocalMachineIDs()[0];
}

juce::File ScriptingApi::FileSystem::getFile(SpecialLocations l)
{
	File f;

	switch (l)
	{
	case Samples:	f = getMainController()->getActiveFileHandler()->getSubDirectory(FileHandlerBase::Samples);
		break;
	case Expansions: return getMainController()->getExpansionHandler().getExpansionFolder();
#if USE_BACKEND
	case AppData:
	{
		f = ProjectHandler::getAppDataRoot();

		auto company = GET_HISE_SETTING(getMainController()->getMainSynthChain(), HiseSettings::User::Company);
		auto project = GET_HISE_SETTING(getMainController()->getMainSynthChain(), HiseSettings::Project::Name);

		f = f.getChildFile(company.toString()).getChildFile(project.toString());

		if (!f.isDirectory())
			f.createDirectory();

		break;
	}
#else
	case AppData:	f = FrontendHandler::getAppDataDirectory();
		break;
#endif
	case UserPresets:
#if USE_BACKEND
		f = getMainController()->getCurrentFileHandler().getSubDirectory(FileHandlerBase::UserPresets);
#else
		f = FrontendHandler::getUserPresetDirectory();
#endif
		break;
	case UserHome: f = File::getSpecialLocation(File::userHomeDirectory); break;
	case Documents: f = File::getSpecialLocation(File::userDocumentsDirectory); break;
	case Desktop:	f = File::getSpecialLocation(File::userDesktopDirectory); break;
	case Downloads: f = File::getSpecialLocation(File::userHomeDirectory).getChildFile("Downloads"); break;
	case AudioFiles: 
#if USE_BACKEND
		f = getMainController()->getCurrentFileHandler().getSubDirectory(FileHandlerBase::AudioFiles);
#else
#if !USE_RELATIVE_PATH_FOR_AUDIO_FILES
		// You need to set this flag if you want to load audio files from the folder
		jassertfalse;
#endif
		f = FrontendHandler::getAdditionalAudioFilesDirectory();
#endif
	default:
		break;
	}

	return f;
}

struct ScriptingApi::Server::Wrapper
{
	API_VOID_METHOD_WRAPPER_1(Server, setBaseURL);
	API_VOID_METHOD_WRAPPER_3(Server, callWithPOST);
	API_VOID_METHOD_WRAPPER_3(Server, callWithGET);
	API_METHOD_WRAPPER_4(Server, downloadFile);
	API_VOID_METHOD_WRAPPER_1(Server, setHttpHeader);
	API_METHOD_WRAPPER_0(Server, getPendingDownloads);
	API_METHOD_WRAPPER_0(Server, isOnline);
	API_VOID_METHOD_WRAPPER_1(Server, setNumAllowedDownloads);
	API_VOID_METHOD_WRAPPER_0(Server, cleanFinishedDownloads);
};

ScriptingApi::Server::Server(JavascriptProcessor* jp_):
	ApiClass(4),
	ScriptingObject(dynamic_cast<ProcessorWithScriptingContent*>(jp_)),
	jp(jp_),
	internalThread(*this)
{
	addConstant("StatusNoConnection", StatusNoConnection);
	addConstant("StatusOK", StatusOK);
	addConstant("StatusNotFound", StatusNotFound);
	addConstant("StatusServerError", StatusServerError);
	addConstant("StatusAuthenticationFail", StatusAuthenticationFail);

	ADD_API_METHOD_1(setBaseURL);
	ADD_API_METHOD_3(callWithPOST);
	ADD_API_METHOD_3(callWithGET);
	ADD_API_METHOD_1(setHttpHeader);
	ADD_API_METHOD_4(downloadFile);
	ADD_API_METHOD_0(getPendingDownloads);
	ADD_API_METHOD_0(isOnline);
}

void ScriptingApi::Server::setBaseURL(String url)
{
	baseURL = URL(url);
	internalThread.startThread();
}

void ScriptingApi::Server::callWithGET(String subURL, var parameters, var callback)
{
	if (HiseJavascriptEngine::isJavascriptFunction(callback))
	{
		PendingCallback::Ptr p = new PendingCallback(getScriptProcessor(), callback);
		p->url = getWithParameters(subURL, parameters);
		p->isPost = false;
		p->extraHeader = extraHeader;

		internalThread.notify();
		internalThread.pendingCallbacks.add(p);
	}
}

void ScriptingApi::Server::callWithPOST(String subURL, var parameters, var callback)
{
	if (HiseJavascriptEngine::isJavascriptFunction(callback))
	{
		PendingCallback::Ptr p = new PendingCallback(getScriptProcessor(), callback);
		p->url = getWithParameters(subURL, parameters);
		p->extraHeader = extraHeader;
		p->isPost = true;

		internalThread.notify();
		internalThread.pendingCallbacks.add(p);
	}
}

void ScriptingApi::Server::setHttpHeader(String newHeader)
{
	extraHeader = newHeader;
}

var ScriptingApi::Server::downloadFile(String subURL, var parameters, var targetFile, var callback)
{
	if (auto sf = dynamic_cast<ScriptingObjects::ScriptFile*>(targetFile.getObject()))
	{
		if (sf->f.isDirectory())
		{
			reportScriptError("target file is a directory");
			return var();
		}

		auto urlToUse = getWithParameters(subURL, parameters);

		if(urlToUse.isWellFormed())
		{
			ScriptingObjects::ScriptDownloadObject::Ptr p = new ScriptingObjects::ScriptDownloadObject(getScriptProcessor(), urlToUse, sf->f, callback);

			ScopedLock sl(internalThread.queueLock);

			for (auto ep : internalThread.pendingDownloads)
			{
				if (*p == *ep)
					return var(ep);
			}

			internalThread.pendingDownloads.add(p);
			internalThread.notify();

			return var(p);
		}
	}
	else
	{
		reportScriptError("target file is not a file object");
	}

	return var();
}

var ScriptingApi::Server::getPendingDownloads()
{
	Array<var> list;

	for (auto p : internalThread.pendingDownloads)
	{
		list.add(var(p));
	}

	return list;
}

void ScriptingApi::Server::setNumAllowedDownloads(int maxNumberOfParallelDownloads)
{
	internalThread.numMaxDownloads = maxNumberOfParallelDownloads;
}

bool ScriptingApi::Server::isOnline()
{
	const char* urlsToTry[] = { "http://google.com/generate_204", "https://amazon.com", nullptr };

	for (const char** url = urlsToTry; *url != nullptr; ++url)
	{
		URL u(*url);

		auto ms = Time::getMillisecondCounter();
		std::unique_ptr<InputStream> in(u.createInputStream(false, nullptr, nullptr, String(), 2000, nullptr));
		dynamic_cast<JavascriptProcessor*>(getScriptProcessor())->getScriptEngine()->extendTimeout(Time::getMillisecondCounter() - ms);

		if (in != nullptr)
			return true;
	}

	return false;
}

void ScriptingApi::Server::cleanFinishedDownloads()
{
	internalThread.cleanDownloads = true;
}

juce::URL ScriptingApi::Server::getWithParameters(String subURL, var parameters)
{
	auto url = baseURL.getChildURL(subURL);

	if (auto d = parameters.getDynamicObject())
	{
		for (auto& p : d->getProperties())
			url = url.withParameter(p.name.toString(), p.value.toString());
	}

	return url;
}

void ScriptingApi::Server::WebThread::run()
{
	while (!threadShouldExit())
	{
		if (parent.getScriptProcessor()->getMainController_()->getKillStateHandler().initialised())
		{
			{
				ScopedLock sl(queueLock);

				int numActiveDownloads = 0;

				for (int i = 0; i < pendingDownloads.size(); i++)
				{
					auto d = pendingDownloads[i];

					if (d->isWaitingForStart && numActiveDownloads < numMaxDownloads)
						d->start();

					if (d->isWaitingForStop)
						d->stopInternal();

					if (d->isRunning())
					{
						if (numActiveDownloads >= numMaxDownloads)
							d->stop();
						else
							numActiveDownloads++;
					}
					
					if (cleanDownloads && d->isFinished)
						pendingDownloads.remove(i--);
				}

				cleanDownloads = false;
			}

			while (auto job = pendingCallbacks.removeAndReturn(0))
			{
				ScopedPointer<WebInputStream> wis;

				wis = dynamic_cast<WebInputStream*>(job->url.createInputStream(job->isPost, nullptr, nullptr, job->extraHeader, 3000, nullptr, &job->status));

				auto response = wis != nullptr ? wis->readEntireStreamAsString() : "{}";
				std::array<var, 2> args;

				args[0] = job->status;
				auto r = JSON::parse(response, args[1]);

				if (!r.wasOk())
				{
					args[0] = 500;
					args[1] = var(new DynamicObject());
					args[1].getDynamicObject()->setProperty("error", r.getErrorMessage());
				}

				job->f.call(args);
#if 0

				auto& pool = parent.getScriptProcessor()->getMainController_()->getJavascriptThreadPool();

				

				pool.addJob(JavascriptThreadPool::Task::HiPriorityCallbackExecution, dynamic_cast<JavascriptProcessor*>(parent.getScriptProcessor()), [status, response, function](JavascriptProcessor* jp)
				{
					if (auto engine = jp->getScriptEngine())
					{
						auto r = Result::ok();

						var argData[2];
						argData[0] = status;

						

						

						var::NativeFunctionArgs args(var(), argData, 2);
						engine->callExternalFunction(function, args, &r);

						return r;
					}
				});
#endif
			}

			Thread::wait(500);
		}
		else
		{
			// We postpone each server call until the thingie is loaded...
			Thread::wait(200);
		}
	}
}


} // namespace hise
