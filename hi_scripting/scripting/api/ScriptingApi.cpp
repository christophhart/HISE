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

KeyPress ApiHelpers::getKeyPress(const var& keyPressInformation, Result* r)
{
	if(keyPressInformation.isString())
	{
		auto x = KeyPress::createFromDescription(keyPressInformation.toString());

		if(x == KeyPress() && r != nullptr)
			*r = Result::fail("not a valid key press");

		return x;
	}
	else if (auto dyn = keyPressInformation.getDynamicObject())
	{
		int mods = 0;

		if(keyPressInformation["shift"])
			mods |= ModifierKeys::shiftModifier;

		if(keyPressInformation["cmd"] || keyPressInformation["ctrl"])
			mods |= (ModifierKeys::ctrlModifier | ModifierKeys::commandModifier);
		
		if(keyPressInformation["alt"])
			mods |= ModifierKeys::altModifier;

		auto keyCode = (int)keyPressInformation["keyCode"];

		if(keyCode == 0 && r != nullptr)
			*r = Result::fail("not a valid key code");

		auto character = keyPressInformation["character"].toString();

		juce_wchar c = character.isEmpty() ? 0 : character[0];

		return KeyPress(keyCode, mods, c);
	}
	else
	{
		if(r != nullptr)
			*r = Result::fail("invalid keypress information, use a JSON or a string");

		return KeyPress();
	}
}

melatonin::ShadowParameters ApiHelpers::getShadowParameters(const var& shadowData, Result* r)
{
	if(auto obj = shadowData.getDynamicObject())
	{
		melatonin::ShadowParameters sp;
		sp.color = getColourFromVar(shadowData.getProperty("Colour", var(0xFF000000)));

		Array<var> zero;
		zero.add(0); zero.add(0);

		sp.offset = getPointFromVar(shadowData.getProperty("Offset", var(zero))).toInt();
		sp.inner = shadowData.getProperty("Inner", false);
		sp.radius = shadowData.getProperty("Radius", 0);
		sp.spread = shadowData.getProperty("Spread", 0);

		return sp;
	}
	else
	{
		if(r != nullptr)
		{
			*r = Result::fail("shadowData needs to be a JSON object with the shadow parameters");
		}

		return {};
	}
}

Colour ApiHelpers::getColourFromVar(const var& value)
{
	int64 colourValue = 0;

	if (value.isInt64() || value.isInt())
		colourValue = (int64)value;
	else if (value.isString())
	{
		auto string = value.toString();

		if (string.startsWith("0x"))
			colourValue = string.getHexValue64();
		else
			colourValue = string.getLargeIntValue();
	}

	return Colour((uint32)colourValue);
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

            if(r != nullptr) *r = Result::ok();
            
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

            if(r != nullptr) *r = Result::ok();
            
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

	API_METHOD_WRAPPER_0(Message, isMonophonicAfterTouch);
	API_METHOD_WRAPPER_0(Message, getMonophonicAftertouchPressure);
	API_VOID_METHOD_WRAPPER_1(Message, setMonophonicAfterTouchPressure);
	API_METHOD_WRAPPER_0(Message, isPolyAftertouch);
	API_METHOD_WRAPPER_0(Message, getPolyAfterTouchNoteNumber);
	API_METHOD_WRAPPER_0(Message, getPolyAfterTouchPressureValue);
	API_VOID_METHOD_WRAPPER_2(Message, setPolyAfterTouchNoteNumberAndPressureValue);

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
	API_METHOD_WRAPPER_0(Message, makeArtificialOrLocal);
	API_METHOD_WRAPPER_0(Message, isArtificial);
	API_VOID_METHOD_WRAPPER_0(Message, sendToMidiOut);
	API_VOID_METHOD_WRAPPER_1(Message, setAllNotesOffCallback);

};


ScriptingApi::Message::Message(ProcessorWithScriptingContent *p) :
ScriptingObject(p),
ApiClass(11),
messageHolder(nullptr),
constMessageHolder(nullptr),
allNotesOffCallback(p, nullptr, var(), 0)
{
	addConstant("PITCH_BEND_CC", HiseEvent::PitchWheelCCNumber);
	addConstant("AFTERTOUC_CC", HiseEvent::AfterTouchCCNumber);

	addConstant("Empty ", (int)HiseEvent::Type::Empty );
	addConstant("NoteOn", (int)HiseEvent::Type::NoteOn);
	addConstant("NoteOff", (int)HiseEvent::Type::NoteOff);
	addConstant("Controller", (int)HiseEvent::Type::Controller);
	addConstant("PitchBend", (int)HiseEvent::Type::PitchBend);
	addConstant("Aftertouch", (int)HiseEvent::Type::Aftertouch);
	addConstant("AllNotesOff", (int)HiseEvent::Type::AllNotesOff);
	addConstant("VolumeFade", (int)HiseEvent::Type::VolumeFade);
	addConstant("PitchFade", (int)HiseEvent::Type::PitchFade);
	
	memset(artificialNoteOnIds, 0, sizeof(uint16) * 128);

	ADD_TYPED_API_METHOD_1(setNoteNumber, VarTypeChecker::Number);
	ADD_TYPED_API_METHOD_1(setVelocity, VarTypeChecker::Number);
	ADD_TYPED_API_METHOD_1(setControllerNumber, VarTypeChecker::Number);
	ADD_TYPED_API_METHOD_1(setControllerValue, VarTypeChecker::Number);
    ADD_TYPED_API_METHOD_1(delayEvent, VarTypeChecker::Number);
    ADD_TYPED_API_METHOD_1(setChannel, VarTypeChecker::Number);
    ADD_TYPED_API_METHOD_1(setGain, VarTypeChecker::Number);
    ADD_TYPED_API_METHOD_1(setTransposeAmount, VarTypeChecker::Number);
    ADD_TYPED_API_METHOD_1(setCoarseDetune, VarTypeChecker::Number);
    ADD_TYPED_API_METHOD_1(setFineDetune, VarTypeChecker::Number);
    ADD_TYPED_API_METHOD_1(setStartOffset, VarTypeChecker::Number);
    ADD_TYPED_API_METHOD_1(store, VarTypeChecker::ScriptObject);
    ADD_TYPED_API_METHOD_1(setAllNotesOffCallback, VarTypeChecker::Function);
    
	ADD_API_METHOD_0(getControllerNumber);
	ADD_API_METHOD_0(getControllerValue);
	ADD_API_METHOD_0(isProgramChange);
	ADD_API_METHOD_0(getProgramChangeNumber);
	ADD_API_METHOD_0(getNoteNumber);
	ADD_API_METHOD_0(getVelocity);
	ADD_API_METHOD_1(ignoreEvent);
	
	ADD_API_METHOD_0(getEventId);
	ADD_API_METHOD_0(getChannel);

	ADD_API_METHOD_0(isMonophonicAfterTouch);
	ADD_API_METHOD_0(getMonophonicAftertouchPressure);
	ADD_API_METHOD_1(setMonophonicAfterTouchPressure);
	ADD_API_METHOD_0(isPolyAftertouch);
	ADD_API_METHOD_0(getPolyAfterTouchNoteNumber);
	ADD_API_METHOD_0(getPolyAfterTouchPressureValue);
	ADD_API_METHOD_2(setPolyAfterTouchNoteNumberAndPressureValue);

	ADD_API_METHOD_0(getGain);
	
	ADD_API_METHOD_0(getTransposeAmount);
	
	ADD_API_METHOD_0(getCoarseDetune);
    
	ADD_API_METHOD_0(getFineDetune);
	ADD_API_METHOD_0(getTimestamp);
	ADD_API_METHOD_0(getStartOffset);
    
	ADD_API_METHOD_0(makeArtificial);
	ADD_API_METHOD_0(makeArtificialOrLocal);
	ADD_API_METHOD_0(isArtificial);
	
	ADD_API_METHOD_0(sendToMidiOut);
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

	return constMessageHolder->getControllerNumber();
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
}

//               ================================================================================================ AFTERTOUCH BEGIN

bool ScriptingApi::Message::isMonophonicAfterTouch() const
{
#if ENABLE_SCRIPTING_SAFE_CHECKS
	if (constMessageHolder == nullptr)
	{
		reportIllegalCall("isMonophonicAfterTouch()", "midi event");
		RETURN_IF_NO_THROW(false);
	}
#endif

	return constMessageHolder->isChannelPressure(); 
}

int ScriptingApi::Message::getMonophonicAftertouchPressure() const
{
#if ENABLE_SCRIPTING_SAFE_CHECKS
	if (constMessageHolder == nullptr || !constMessageHolder->isChannelPressure())
	{
		reportIllegalCall("getMonophonicAftertouchPressure()", "midi event");
		RETURN_IF_NO_THROW(-1)
	}
#endif

	return constMessageHolder->getChannelPressureValue(); 
}

void ScriptingApi::Message::setMonophonicAfterTouchPressure(int pressure)
{
#if ENABLE_SCRIPTING_SAFE_CHECKS
	if (messageHolder == nullptr || !messageHolder->isChannelPressure())
	{
		reportIllegalCall("setMonophonicAfterTouchPressure()", "midi event");
		RETURN_VOID_IF_NO_THROW()
	}
#endif

	messageHolder->setChannelPressureValue((uint8)pressure); 
}

bool ScriptingApi::Message::isPolyAftertouch() const
{
#if ENABLE_SCRIPTING_SAFE_CHECKS
	if (constMessageHolder == nullptr)
	{
		reportIllegalCall("isPolyAftertouch()", "midi event");
		RETURN_IF_NO_THROW(false);
	}
#endif

	return constMessageHolder->isAftertouch(); 
}

int ScriptingApi::Message::getPolyAfterTouchNoteNumber() const
{
#if ENABLE_SCRIPTING_SAFE_CHECKS
	if (constMessageHolder == nullptr || !constMessageHolder->isAftertouch())
	{
		reportIllegalCall("getPolyAfterTouchNoteNumber()", "midi event");
		RETURN_IF_NO_THROW(-1);
	}
#endif

	return messageHolder->getAfterTouchNumber(); 
}

int ScriptingApi::Message::getPolyAfterTouchPressureValue() const
{
#if ENABLE_SCRIPTING_SAFE_CHECKS
	if (constMessageHolder == nullptr || !constMessageHolder->isAftertouch())
	{
		reportIllegalCall("getPolyAfterTouchPressureValue()", "midi event");
		RETURN_IF_NO_THROW(-1);
	}
#endif

	return messageHolder->getAfterTouchValue(); 
}

void ScriptingApi::Message::setPolyAfterTouchNoteNumberAndPressureValue(int noteNumber, int aftertouchAmount)
{
	if (messageHolder == nullptr || !constMessageHolder->isAftertouch())
	{
		reportIllegalCall("setPolyAfterTouchNoteNumberAndPressureValue()", "midi event");
		RETURN_VOID_IF_NO_THROW()
	}

	messageHolder->setAfterTouchValue(noteNumber, aftertouchAmount); 
};

//               ================================================================================================ AFTERTOUCH END

void ScriptingApi::Message::ignoreEvent(bool shouldBeIgnored/*=true*/)
{
	if (messageHolder == nullptr)
	{
		reportIllegalCall("ignoreEvent()", "midi event");
		RETURN_VOID_IF_NO_THROW()
	}

	// If we call make artificial and then later ignore the note off, we need
	// to reintroduce the note on event so that it can be killed later.
	if(shouldBeIgnored && isArtificial() && messageHolder->isNoteOff() && (artificialNoteOnThatWasKilled.getEventId() == messageHolder->getEventId())) 
	{
		getScriptProcessor()->getMainController_()->getEventHandler().reinsertArtificialNoteOn(artificialNoteOnThatWasKilled);
		pushArtificialNoteOn(artificialNoteOnThatWasKilled);
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
	return makeArtificialInternal(false);
}

int ScriptingApi::Message::makeArtificialInternal(bool makeLocal)
{
	artificialNoteOnThatWasKilled = {};

	if (messageHolder != nullptr)
	{
		HiseEvent copy(*messageHolder);

		if (!makeLocal && copy.isArtificial())
			return copy.getEventId();

		copy.setArtificial();

		if (copy.isNoteOn())
		{
			getScriptProcessor()->getMainController_()->getEventHandler().pushArtificialNoteOn(copy);
			pushArtificialNoteOn(copy);
		}
		else if (copy.isNoteOff())
		{
			HiseEvent e = getScriptProcessor()->getMainController_()->getEventHandler().popNoteOnFromEventId(artificialNoteOnIds[copy.getNoteNumber()]);

			// keep this alive
			artificialNoteOnThatWasKilled = e;

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

int ScriptingApi::Message::makeArtificialOrLocal()
{
	return makeArtificialInternal(true);
}

bool ScriptingApi::Message::isArtificial() const
{
	if (constMessageHolder != nullptr)
	{
		return constMessageHolder->isArtificial();
	}

	return false;
}

void ScriptingApi::Message::setAllNotesOffCallback(var onAllNotesOffCallback)
{
	allNotesOffCallback = WeakCallbackHolder(getScriptProcessor(), this, onAllNotesOffCallback, 0);
	allNotesOffCallback.incRefCount();
}

void ScriptingApi::Message::sendToMidiOut()
{
#if USE_BACKEND
    
    auto mc = getScriptProcessor()->getMainController_();
    
    auto midiOutputEnabled = dynamic_cast<GlobalSettingManager*>(mc)->getSettingsObject().getSetting(HiseSettings::Project::EnableMidiOut);
    
    if(!midiOutputEnabled)
    {
        reportScriptError("You need to enable EnableMidiOut in the project settings for this function to work");
    }
#endif
    
	makeArtificial();
	getScriptProcessor()->getMainController_()->sendToMidiOut(*messageHolder);
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

void ScriptingApi::Message::onAllNotesOff()
{
	if (allNotesOffCallback)
		allNotesOffCallback.callSync(nullptr, 0, nullptr);
}

// ====================================================================================================== Engine functions

struct ScriptingApi::Engine::Wrapper
{
	API_VOID_METHOD_WRAPPER_0(Engine, allNotesOff);
	API_METHOD_WRAPPER_0(Engine, getProjectInfo);
	API_METHOD_WRAPPER_0(Engine, getUptime);
	API_METHOD_WRAPPER_0(Engine, getHostBpm);
	API_VOID_METHOD_WRAPPER_1(Engine, setHostBpm);
	API_METHOD_WRAPPER_0(Engine, getCpuUsage);
	API_METHOD_WRAPPER_0(Engine, getNumVoices);
	API_METHOD_WRAPPER_0(Engine, getMemoryUsage);
	API_METHOD_WRAPPER_1(Engine, getTempoName);
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
	API_METHOD_WRAPPER_0(Engine, getBufferSize);
	API_METHOD_WRAPPER_0(Engine, getNumPluginChannels);
	API_METHOD_WRAPPER_1(Engine, setMinimumSampleRate);
	API_VOID_METHOD_WRAPPER_1(Engine, setMaximumBlockSize);
	API_METHOD_WRAPPER_1(Engine, getMidiNoteName);
	API_METHOD_WRAPPER_1(Engine, getMidiNoteFromName);
	API_METHOD_WRAPPER_1(Engine, getMacroName);
  API_VOID_METHOD_WRAPPER_1(Engine, setFrontendMacros)
	API_VOID_METHOD_WRAPPER_2(Engine, setKeyColour);
	API_VOID_METHOD_WRAPPER_3(Engine, showMessageBox);
	API_VOID_METHOD_WRAPPER_2(Engine, showErrorMessage);
	API_VOID_METHOD_WRAPPER_1(Engine, showMessage);
	API_VOID_METHOD_WRAPPER_1(Engine, setLowestKeyToDisplay);
  API_VOID_METHOD_WRAPPER_1(Engine, openWebsite);
	API_METHOD_WRAPPER_0(Engine, createUserPresetHandler);
	API_VOID_METHOD_WRAPPER_1(Engine, loadNextUserPreset);
	API_VOID_METHOD_WRAPPER_1(Engine, loadPreviousUserPreset);
	API_VOID_METHOD_WRAPPER_1(Engine, loadUserPreset);
	API_VOID_METHOD_WRAPPER_1(Engine, setUserPresetTagList);
	API_METHOD_WRAPPER_1(Engine, isUserPresetReadOnly);
	API_METHOD_WRAPPER_0(Engine, getUserPresetList);
	API_METHOD_WRAPPER_0(Engine, getCurrentUserPresetName);
	API_VOID_METHOD_WRAPPER_1(Engine, saveUserPreset);
	API_METHOD_WRAPPER_0(Engine, isMpeEnabled);
	API_METHOD_WRAPPER_1(Engine, createAndRegisterSliderPackData);
	API_METHOD_WRAPPER_1(Engine, createAndRegisterTableData);
	API_METHOD_WRAPPER_1(Engine, createAndRegisterAudioFile);
	API_METHOD_WRAPPER_1(Engine, createAndRegisterRingBuffer);
	API_METHOD_WRAPPER_0(Engine, createMidiList);
	API_METHOD_WRAPPER_0(Engine, createBeatportManager);
	API_METHOD_WRAPPER_0(Engine, createUnorderedStack);
	API_METHOD_WRAPPER_0(Engine, createThreadSafeStorage);
	API_METHOD_WRAPPER_0(Engine, createTimerObject);
	API_METHOD_WRAPPER_0(Engine, createMessageHolder);
	API_METHOD_WRAPPER_0(Engine, createTransportHandler);
	API_METHOD_WRAPPER_0(Engine, createMidiAutomationHandler);
	API_METHOD_WRAPPER_0(Engine, getPlayHead);
	API_VOID_METHOD_WRAPPER_2(Engine, dumpAsJSON);
	API_METHOD_WRAPPER_1(Engine, loadFromJSON);
	API_METHOD_WRAPPER_1(Engine, compressJSON);
	API_METHOD_WRAPPER_1(Engine, uncompressJSON);
	API_VOID_METHOD_WRAPPER_1(Engine, setCompileProgress);
	API_METHOD_WRAPPER_2(Engine, matchesRegex);
	API_METHOD_WRAPPER_2(Engine, getRegexMatches);
	API_METHOD_WRAPPER_2(Engine, doubleToString);
	API_METHOD_WRAPPER_4(Engine, getStringWidth);
	API_METHOD_WRAPPER_1(Engine, intToHexString);
	API_METHOD_WRAPPER_0(Engine, getOS);
	API_METHOD_WRAPPER_0(Engine, getSystemStats);
	API_METHOD_WRAPPER_0(Engine, isPlugin);
	API_METHOD_WRAPPER_0(Engine, isHISE);
	API_VOID_METHOD_WRAPPER_0(Engine, reloadAllSamples);
	API_METHOD_WRAPPER_0(Engine, getPreloadProgress);
	API_METHOD_WRAPPER_0(Engine, getPreloadMessage);
	API_METHOD_WRAPPER_0(Engine, getDeviceType);
	API_METHOD_WRAPPER_0(Engine, getDeviceResolution);
	API_METHOD_WRAPPER_0(Engine, getZoomLevel);
	API_VOID_METHOD_WRAPPER_1(Engine, setZoomLevel);
	API_VOID_METHOD_WRAPPER_1(Engine, setDiskMode);
	API_METHOD_WRAPPER_0(Engine, getVersion);
	API_METHOD_WRAPPER_0(Engine, getName);
	API_METHOD_WRAPPER_3(Engine, getComplexDataReference);
	API_METHOD_WRAPPER_0(Engine, getFilterModeList);
	API_METHOD_WRAPPER_2(Engine, sortWithFunction);
	API_METHOD_WRAPPER_1(Engine, isControllerUsedByAutomation);
	API_METHOD_WRAPPER_0(Engine, getSettingsWindowObject);
	API_METHOD_WRAPPER_1(Engine, getMasterPeakLevel);
	API_METHOD_WRAPPER_0(Engine, getControlRateDownsamplingFactor);
	API_METHOD_WRAPPER_1(Engine, createDspNetwork);
	API_METHOD_WRAPPER_0(Engine, createExpansionHandler);
	API_METHOD_WRAPPER_0(Engine, createFFT);
	API_METHOD_WRAPPER_1(Engine, createNeuralNetwork);
	API_METHOD_WRAPPER_0(Engine, getExpansionList);
	API_METHOD_WRAPPER_1(Engine, setCurrentExpansion);
	API_METHOD_WRAPPER_0(Engine, createGlobalScriptLookAndFeel);
	API_METHOD_WRAPPER_1(Engine, createBackgroundTask);
    API_METHOD_WRAPPER_1(Engine, createFixObjectFactory);
	API_METHOD_WRAPPER_0(Engine, createErrorHandler);
	API_METHOD_WRAPPER_1(Engine, createModulationMatrix);
	API_METHOD_WRAPPER_0(Engine, createMacroHandler);
	API_METHOD_WRAPPER_0(Engine, getWavetableList);
	API_VOID_METHOD_WRAPPER_3(Engine, showYesNoWindow);
	API_VOID_METHOD_WRAPPER_1(Engine, addModuleStateToUserPreset);
	API_VOID_METHOD_WRAPPER_0(Engine, rebuildCachedPools);
	API_VOID_METHOD_WRAPPER_1(Engine, extendTimeOut);
	API_VOID_METHOD_WRAPPER_1(Engine, setAllowDuplicateSamples);
	API_VOID_METHOD_WRAPPER_1(Engine, loadFont);
	API_VOID_METHOD_WRAPPER_2(Engine, loadFontAs);
	API_VOID_METHOD_WRAPPER_1(Engine, setGlobalFont);
	API_VOID_METHOD_WRAPPER_0(Engine, quit);
	API_VOID_METHOD_WRAPPER_0(Engine, undo);
	API_VOID_METHOD_WRAPPER_0(Engine, redo);
    API_VOID_METHOD_WRAPPER_0(Engine, clearUndoHistory);
	API_METHOD_WRAPPER_2(Engine, performUndoAction);
	API_METHOD_WRAPPER_0(Engine, getExtraDefinitionsInBackend);
	API_METHOD_WRAPPER_0(Engine, loadAudioFilesIntoPool);
	API_VOID_METHOD_WRAPPER_1(Engine, loadImageIntoPool);
	API_VOID_METHOD_WRAPPER_0(Engine, clearMidiFilePool);
	API_VOID_METHOD_WRAPPER_0(Engine, clearSampleMapPool);
	API_METHOD_WRAPPER_2(Engine, getSampleFilesFromDirectory);
	API_VOID_METHOD_WRAPPER_1(Engine, setLatencySamples);
	API_VOID_METHOD_WRAPPER_1(Engine, setGlobalPitchFactor);
	API_METHOD_WRAPPER_0(Engine, getGlobalPitchFactor);
	API_METHOD_WRAPPER_0(Engine, getLatencySamples);
	API_METHOD_WRAPPER_2(Engine, getDspNetworkReference);
	API_METHOD_WRAPPER_1(Engine, getSystemTime);
	API_METHOD_WRAPPER_0(Engine, createLicenseUnlocker);
	API_METHOD_WRAPPER_1(Engine, createBroadcaster);
	API_METHOD_WRAPPER_0(Engine, getGlobalRoutingManager);
    API_METHOD_WRAPPER_0(Engine, getLorisManager);
	API_METHOD_WRAPPER_1(Engine, loadAudioFileIntoBufferArray);
	API_METHOD_WRAPPER_0(Engine, getClipboardContent);
	API_VOID_METHOD_WRAPPER_1(Engine, copyToClipboard);
	API_METHOD_WRAPPER_1(Engine, decodeBase64ValueTree);
	API_VOID_METHOD_WRAPPER_2(Engine, renderAudio);
	API_VOID_METHOD_WRAPPER_3(Engine, playBuffer);
	
	
};



ScriptingApi::Engine::Engine(ProcessorWithScriptingContent *p) :
ScriptingObject(p),
ApiClass(0),
parentMidiProcessor(dynamic_cast<ScriptBaseMidiProcessor*>(p))
{
	ADD_API_METHOD_0(getProjectInfo);
	ADD_API_METHOD_0(allNotesOff);
	ADD_API_METHOD_0(getUptime);
	ADD_API_METHOD_0(getHostBpm);
	ADD_TYPED_API_METHOD_1(setHostBpm, VarTypeChecker::Number);
	ADD_API_METHOD_0(getCpuUsage);
	ADD_API_METHOD_0(getNumVoices);
	ADD_API_METHOD_0(getMemoryUsage);
	ADD_API_METHOD_1(getTempoName);
	ADD_TYPED_API_METHOD_1(getMilliSecondsForTempo, VarTypeChecker::Number);
	ADD_TYPED_API_METHOD_1(getSamplesForMilliSeconds, VarTypeChecker::Number);
	ADD_TYPED_API_METHOD_1(getMilliSecondsForSamples, VarTypeChecker::Number);
	ADD_TYPED_API_METHOD_1(getQuarterBeatsForMilliSeconds, VarTypeChecker::Number);
	ADD_TYPED_API_METHOD_1(getQuarterBeatsForSamples, VarTypeChecker::Number);
	ADD_TYPED_API_METHOD_1(getSamplesForQuarterBeats, VarTypeChecker::Number);
	ADD_TYPED_API_METHOD_1(getMilliSecondsForQuarterBeats, VarTypeChecker::Number);
	ADD_API_METHOD_2(getQuarterBeatsForMilliSecondsWithTempo);
	ADD_API_METHOD_2(getQuarterBeatsForSamplesWithTempo);
	ADD_API_METHOD_2(getSamplesForQuarterBeatsWithTempo);
	ADD_API_METHOD_2(getMilliSecondsForQuarterBeatsWithTempo);
	ADD_TYPED_API_METHOD_1(getGainFactorForDecibels, VarTypeChecker::Number);
	ADD_TYPED_API_METHOD_1(getDecibelsForGainFactor, VarTypeChecker::Number);
	ADD_TYPED_API_METHOD_1(getFrequencyForMidiNoteNumber, VarTypeChecker::Number);
	ADD_TYPED_API_METHOD_1(getPitchRatioFromSemitones, VarTypeChecker::Number);
	ADD_TYPED_API_METHOD_1(getSemitonesFromPitchRatio, VarTypeChecker::Number);
	ADD_API_METHOD_1(addModuleStateToUserPreset);
	ADD_API_METHOD_0(getSampleRate);
	ADD_API_METHOD_0(getBufferSize);
	ADD_API_METHOD_0(getNumPluginChannels);
	ADD_TYPED_API_METHOD_1(setMinimumSampleRate, VarTypeChecker::Number);
	ADD_TYPED_API_METHOD_1(setMaximumBlockSize, VarTypeChecker::Number);
	ADD_API_METHOD_0(createThreadSafeStorage);
	ADD_API_METHOD_1(getMidiNoteName);
	ADD_API_METHOD_1(getMidiNoteFromName);
	ADD_API_METHOD_1(getMacroName);
	ADD_API_METHOD_0(getWavetableList);
	ADD_API_METHOD_1(setFrontendMacros);
	ADD_API_METHOD_2(setKeyColour);
	ADD_API_METHOD_2(showErrorMessage);
	ADD_API_METHOD_1(showMessage);
	ADD_API_METHOD_1(setLowestKeyToDisplay);
	ADD_API_METHOD_3(getComplexDataReference);
  ADD_API_METHOD_1(openWebsite);
	ADD_API_METHOD_0(createUserPresetHandler);
	ADD_API_METHOD_0(createMidiAutomationHandler);
	ADD_API_METHOD_0(createMacroHandler);
  ADD_API_METHOD_1(loadNextUserPreset);
	ADD_API_METHOD_1(loadPreviousUserPreset);
	ADD_API_METHOD_1(isUserPresetReadOnly);
	ADD_API_METHOD_0(getExpansionList);
	ADD_API_METHOD_1(setCurrentExpansion);
	ADD_API_METHOD_1(setUserPresetTagList);
	ADD_API_METHOD_0(getCurrentUserPresetName);
	ADD_API_METHOD_0(getGlobalPitchFactor);
	ADD_API_METHOD_1(setGlobalPitchFactor);
	ADD_API_METHOD_1(saveUserPreset);
	ADD_API_METHOD_1(loadUserPreset);
	ADD_API_METHOD_0(getUserPresetList);
	ADD_API_METHOD_0(isMpeEnabled);
	ADD_API_METHOD_0(createMidiList);
	ADD_API_METHOD_0(createUnorderedStack);
	ADD_API_METHOD_1(createBackgroundTask);
	ADD_API_METHOD_0(createFFT);
	ADD_API_METHOD_1(createNeuralNetwork);
	ADD_API_METHOD_1(createBroadcaster);
	ADD_API_METHOD_0(getPlayHead);
	ADD_API_METHOD_2(dumpAsJSON);
	ADD_API_METHOD_1(loadFromJSON);
	ADD_API_METHOD_1(setCompileProgress);
	ADD_API_METHOD_2(matchesRegex);
	ADD_API_METHOD_2(getRegexMatches);
	ADD_API_METHOD_2(doubleToString);
	ADD_API_METHOD_1(intToHexString);
	ADD_API_METHOD_4(getStringWidth);
	ADD_API_METHOD_1(getMasterPeakLevel);
	ADD_API_METHOD_0(getOS);
	ADD_API_METHOD_0(getSystemStats);
	ADD_API_METHOD_0(getDeviceType);
	ADD_API_METHOD_0(getDeviceResolution);
	ADD_API_METHOD_0(isPlugin);
	ADD_API_METHOD_0(getPreloadProgress);
	ADD_API_METHOD_0(getPreloadMessage);
	ADD_API_METHOD_0(getZoomLevel);
	ADD_API_METHOD_1(setZoomLevel);
	ADD_API_METHOD_1(setDiskMode);
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
	ADD_API_METHOD_1(createAndRegisterSliderPackData);
	ADD_API_METHOD_1(createAndRegisterTableData);
	ADD_API_METHOD_1(createAndRegisterAudioFile);
	ADD_API_METHOD_1(createAndRegisterRingBuffer);
	ADD_API_METHOD_0(getGlobalRoutingManager);
    ADD_API_METHOD_0(getLorisManager);
	ADD_API_METHOD_0(createBeatportManager);
	ADD_API_METHOD_1(loadFont);
	ADD_API_METHOD_2(loadFontAs);
	ADD_API_METHOD_1(loadAudioFileIntoBufferArray);
	ADD_API_METHOD_1(setGlobalFont);
	ADD_API_METHOD_1(extendTimeOut);
	ADD_API_METHOD_0(getControlRateDownsamplingFactor);
    ADD_API_METHOD_1(createFixObjectFactory);
	ADD_API_METHOD_0(quit);
	ADD_API_METHOD_0(undo);
	ADD_API_METHOD_0(redo);
    ADD_API_METHOD_0(clearUndoHistory);
	ADD_TYPED_API_METHOD_2(performUndoAction, VarTypeChecker::JSON, VarTypeChecker::Function);
	ADD_API_METHOD_0(getExtraDefinitionsInBackend);
	ADD_API_METHOD_0(loadAudioFilesIntoPool);
	ADD_API_METHOD_0(clearMidiFilePool);
	ADD_API_METHOD_0(clearSampleMapPool);
	ADD_API_METHOD_2(getSampleFilesFromDirectory);
	ADD_API_METHOD_0(rebuildCachedPools);
	ADD_API_METHOD_1(loadImageIntoPool);
	ADD_API_METHOD_1(createDspNetwork);
	ADD_API_METHOD_0(createTransportHandler);
	ADD_API_METHOD_0(createErrorHandler);
	ADD_API_METHOD_1(setLatencySamples);
	ADD_API_METHOD_0(getLatencySamples);
	ADD_API_METHOD_2(getDspNetworkReference);
	ADD_API_METHOD_0(createExpansionHandler);
	ADD_API_METHOD_1(createModulationMatrix);
	ADD_API_METHOD_3(showYesNoWindow);
	ADD_API_METHOD_3(showMessageBox);
	ADD_API_METHOD_1(getSystemTime);
	ADD_API_METHOD_0(createLicenseUnlocker);
	ADD_API_METHOD_0(getClipboardContent);
	ADD_API_METHOD_1(copyToClipboard);
	ADD_API_METHOD_0(isHISE);
	ADD_API_METHOD_0(reloadAllSamples);
	ADD_API_METHOD_1(decodeBase64ValueTree);
	ADD_API_METHOD_2(renderAudio);
	ADD_API_METHOD_3(playBuffer);
	ADD_API_METHOD_1(compressJSON);
	ADD_API_METHOD_1(uncompressJSON);
}


ScriptingApi::Engine::~Engine()
{

}

var ScriptingApi::Engine::getProjectInfo()
{		
	auto obj = new DynamicObject();
	
	String licencee;

	#if USE_BACKEND || USE_COPY_PROTECTION
		if (auto ul = getProcessor()->getMainController()->getLicenseUnlocker())
			licencee = ul->getUserEmail();
	#endif

	# if USE_BACKEND
		obj->setProperty("Company", GET_HISE_SETTING(getProcessor()->getMainController()->getMainSynthChain(), HiseSettings::User::Company).toString());
		obj->setProperty("CompanyURL", GET_HISE_SETTING(getProcessor()->getMainController()->getMainSynthChain(), HiseSettings::User::CompanyURL).toString());
		obj->setProperty("CompanyCopyright", GET_HISE_SETTING(getProcessor()->getMainController()->getMainSynthChain(), HiseSettings::User::CompanyCopyright).toString());
		obj->setProperty("ProjectName", GET_HISE_SETTING(getProcessor()->getMainController()->getMainSynthChain(), HiseSettings::Project::Name).toString());
		obj->setProperty("ProjectVersion", GET_HISE_SETTING(getProcessor()->getMainController()->getMainSynthChain(), HiseSettings::Project::Version).toString());
		obj->setProperty("EncryptionKey", GET_HISE_SETTING(getProcessor()->getMainController()->getMainSynthChain(), HiseSettings::Project::EncryptionKey).toString());
		
	#else 
		obj->setProperty("Company", hise::FrontendHandler::getCompanyName());
		obj->setProperty("CompanyURL", hise::FrontendHandler::getCompanyWebsiteName());
		obj->setProperty("CompanyCopyright", hise::FrontendHandler::getCompanyCopyright());
		obj->setProperty("ProjectName", hise::FrontendHandler::getProjectName());
		obj->setProperty("ProjectVersion", hise::FrontendHandler::getVersionString());
		obj->setProperty("EncryptionKey", hise::FrontendHandler::getExpansionKey());
		
	#endif


	obj->setProperty("HISEBuild", GlobalSettingManager::getHiseVersion());
	
	obj->setProperty("BuildDate", Time::getCompilationDate().toString(true, false, false, true));
	obj->setProperty("LicensedEmail", licencee);
			
	return obj;
}

void ScriptingApi::Engine::allNotesOff()
{
	getProcessor()->getMainController()->allNotesOff();
};

void ScriptingApi::Engine::addModuleStateToUserPreset(var moduleId)
{
	String newId;

	auto& ids = getScriptProcessor()->getMainController_()->getModuleStateManager().modules;

	if (moduleId.isString())
	{
		newId = moduleId.toString();

		if (newId.isEmpty())
		{
			ids.clear();
			debugToConsole(getProcessor(), "Removed all stored modules");
			return;
		}

	}
	else
		newId = moduleId["ID"].toString();

	if (newId.isEmpty())
		reportScriptError("Invalid ID");

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

	bool wasRemoved = false;

	for (auto ms : ids)
	{
		if (ms->id == newId)
		{
			ids.removeObject(ms);
			wasRemoved = true;

			break;
		}
	}

	ids.add(new ModuleStateManager::StoredModuleData(moduleId, p));

	if (!wasRemoved)
	{
		debugToConsole(getProcessor(), "Added " + newId + " to user preset system");
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

	if (FullInstrumentExpansion::isEnabled(getProcessor()->getMainController()))
	{
		// Already loaded???
		if (auto e = FullInstrumentExpansion::getCurrentFullExpansion(getProcessor()->getMainController()))
			return;
	}

	const String absolutePath = GET_PROJECT_HANDLER(getProcessor()).getFilePath(fileName, ProjectHandler::SubDirectories::Images);
	File f(absolutePath);
	auto fis = f.createInputStream();

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

void ScriptingApi::Engine::setMaximumBlockSize(int numSamplesPerBlock)
{
	getProcessor()->getMainController()->setMaximumBlockSize(numSamplesPerBlock);
}

double ScriptingApi::Engine::getSampleRate() const { return const_cast<MainController*>(getProcessor()->getMainController())->getMainSynthChain()->getSampleRate(); }

int ScriptingApi::Engine::getBufferSize() const { return getProcessor()->getLargestBlockSize(); }

double ScriptingApi::Engine::getSamplesForMilliSeconds(double milliSeconds) const { return (milliSeconds / 1000.0) * getSampleRate(); }

String ScriptingApi::Engine::getTempoName(int tempoIndex)
{
	return hise::TempoSyncer::getTempoName(tempoIndex);
}

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
	auto samplesPerQuarter = TempoSyncer::getTempoInSamples(bpm, getSampleRate(), TempoSyncer::Quarter);

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
	dynamic_cast<GlobalSettingManager*>(getProcessor()->getMainController())->globalBPM = newTempo;
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
		
		for (int i = 0; i < HISE_NUM_MACROS; i++)
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
#elif JUCE_LINUX
	return "LINUX";
#else
	return "OSX";
#endif
}

var ScriptingApi::Engine::getSystemStats()
{
	auto obj = new DynamicObject();
	
	obj->setProperty("OperatingSystemName", SystemStats::getOperatingSystemName());
	obj->setProperty("OperatingSystem64Bit", SystemStats::isOperatingSystem64Bit());
	obj->setProperty("LogonName", SystemStats::getLogonName());
	obj->setProperty("FullUserName", SystemStats::getFullUserName());
	obj->setProperty("ComputerName", SystemStats::getComputerName());
	obj->setProperty("UserLanguage", SystemStats::getUserLanguage());
	obj->setProperty("UserRegion", SystemStats::getUserRegion());
	obj->setProperty("DisplayLanguage", SystemStats::getDisplayLanguage());
	obj->setProperty("NumCpus", SystemStats::getNumCpus());
	obj->setProperty("NumPhysicalCpus", SystemStats::getNumPhysicalCpus());
	obj->setProperty("CpuSpeedInMegahertz", SystemStats::getCpuSpeedInMegahertz());
	obj->setProperty("CpuVendor", SystemStats::getCpuVendor());
	obj->setProperty("CpuModel", SystemStats::getCpuModel());
	obj->setProperty("MemorySizeInMegabytes", SystemStats::getMemorySizeInMegabytes());

	return obj;
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

bool ScriptingApi::Engine::isHISE()
{
#if USE_BACKEND
	return true;
#else
	return false;
#endif
}

void ScriptingApi::Engine::reloadAllSamples()
{
	auto f = [](Processor* p)
	{
		Processor::Iterator<ModulatorSampler> iter(p);

		while (auto s = iter.getNextProcessor())
			s->reloadSampleMap();

		return SafeFunctionCall::OK;
	};

	auto mc = getScriptProcessor()->getMainController_();
	
#if USE_BACKEND
	mc->getSampleManager().getProjectHandler().checkSubDirectories();
#else
	mc->getSampleManager().getProjectHandler().checkAllSampleReferences();
#endif

	


	mc->getKillStateHandler().killVoicesAndCall(mc->getMainSynthChain(), f, MainController::KillStateHandler::TargetThread::SampleLoadingThread);
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
	logSettingWarning("getZoomLevel");
	auto gm = dynamic_cast<const GlobalSettingManager*>(getScriptProcessor()->getMainController_());
	
	return gm->getGlobalScaleFactor();
}

void ScriptingApi::Engine::setZoomLevel(double newLevel)
{
	logSettingWarning("setZoomLevel");
	newLevel = jlimit(0.25, 2.0, newLevel);

	auto gm = dynamic_cast<GlobalSettingManager*>(getScriptProcessor()->getMainController_());
	gm->setGlobalScaleFactor(newLevel, sendNotificationAsync);
}

void ScriptingApi::Engine::setDiskMode(int mode)
{
	logSettingWarning("setDiskMode");

	auto mc = dynamic_cast<MainController*>(getScriptProcessor()->getMainController_());

	AudioProcessorDriver* driver = dynamic_cast<AudioProcessorDriver*>(mc);

	driver->diskMode = mode;
	mc->getSampleManager().setDiskMode((MainController::SampleManager::DiskMode)mode);
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

juce::var ScriptingApi::Engine::getExtraDefinitionsInBackend()
{
#if USE_BACKEND

    auto mc = getScriptProcessor()->getMainController_();
    
    return dynamic_cast<GlobalSettingManager*>(mc)->getSettingsObject().getExtraDefinitionsAsObject();
    
#else
	return {};
#endif

	
}

void ScriptingApi::Engine::showMessageBox(String title, String markdownMessage, int type)
{
	MessageManager::callAsync([title, markdownMessage, type]()
	{
		PresetHandler::showMessageWindow(title, markdownMessage, (PresetHandler::IconType)type);
	});
}

void ScriptingApi::Engine::showYesNoWindow(String title, String markdownMessage, var callback)
{
	//auto p = dynamic_cast<JavascriptProcessor*>(getScriptProcessor());
	auto p = getScriptProcessor();

	WeakCallbackHolder cb(p, this, callback, 1);
	cb.incRefCount();

	auto f = [markdownMessage, title, cb]() mutable
	{
		auto ok = PresetHandler::showYesNoWindow(title, markdownMessage);

		std::array<var, 1> args = { var(ok) };
		cb.call({ var(ok) });
	};

	MessageManager::callAsync(f);
}



String ScriptingApi::Engine::decodeBase64ValueTree(const String& b64Data)
{
	zstd::ZDefaultCompressor comp;

	

	auto v = ValueTreeConverters::convertBase64ToValueTree(b64Data, true);

	if (!v.isValid())
	{
		auto r = comp.expand(b64Data, v);

		if (!r.wasOk())
		{
			MemoryBlock mb;
			mb.fromBase64Encoding(b64Data);

			v = ValueTree::readFromData(mb.getData(), mb.getSize());
		}
	}

	

	if (v.isValid())
	{
		auto xml = v.createXml();
		return xml->createDocument("");
	}

	return {};
}

var ScriptingApi::Engine::createGlobalScriptLookAndFeel()
{
	auto mc = getScriptProcessor()->getMainController_();

	if (auto sc = mc->getCurrentScriptLookAndFeel())
		return var(sc);
	else
	{
		auto slaf = new ScriptingObjects::ScriptedLookAndFeel(getScriptProcessor(), true);
		return var(slaf);
	}
}

struct ScriptUndoableAction : public UndoableAction,
							  public ControlledObject
{
	ScriptUndoableAction(ProcessorWithScriptingContent* p, var f, var thisObject_):
		UndoableAction(),
	    ControlledObject(p->getMainController_()),
		callback(p, nullptr, f, 1),
		thisObject(thisObject_)
	{
		// ensure it's called synchronously if possible...
		callback.setHighPriority();
		callback.incRefCount();
		callback.setThisObjectRefCounted(thisObject);
	}

	bool undo() override
	{
		if (callback)
		{
			var a(true);

			callSyncIfScriptingThread(a);
			return true;
		}

		return false;
	}

	bool perform() override
	{
		if (callback)
		{
			var a(false);
			
			callSyncIfScriptingThread(a);
			return true;
		}

		return false;
	}

	void callSyncIfScriptingThread(const var& a)
	{
		auto currentThread = getMainController()->getKillStateHandler().getCurrentThread();

		var::NativeFunctionArgs args(thisObject, &a, 1);

		switch(currentThread)
		{
		case MainController::KillStateHandler::TargetThread::ScriptingThread:
		case MainController::KillStateHandler::TargetThread::SampleLoadingThread:
		{
			auto ok = callback.callSync(args);

			if(!ok.wasOk())
				throw ok.getErrorMessage();

			break;
		}
		case MainController::KillStateHandler::TargetThread::MessageThread:
		{
			callback.call(args);
		}
		}
	}

	var thisObject;
	WeakCallbackHolder callback;
};

bool ScriptingApi::Engine::performUndoAction(var thisObject, var undoAction)
{
	return getScriptProcessor()->getMainController_()->getControlUndoManager()->perform(new ScriptUndoableAction(getScriptProcessor(), undoAction, thisObject));
}

int ScriptingApi::Engine::getNumPluginChannels() const
{
	return HISE_NUM_PLUGIN_CHANNELS;
}

var ScriptingApi::Engine::createFixObjectFactory(var layoutData)
{
    return var(new fixobj::Factory(getScriptProcessor(), layoutData));
}

var ScriptingApi::Engine::createThreadSafeStorage()
{
	return var (new ScriptingObjects::ScriptThreadSafeStorage(getScriptProcessor()));
}

juce::var ScriptingApi::Engine::createLicenseUnlocker()
{
	return var(new ScriptUnlocker::RefObject(getScriptProcessor()));
}

var ScriptingApi::Engine::createBeatportManager()
{
	return var(new BeatportManager(getScriptProcessor()));
}



struct AudioRenderer : public AudioRendererBase
{
	AudioRenderer(ProcessorWithScriptingContent* pwsc, var eventList_, var finishCallback_):
		AudioRendererBase(pwsc->getMainController_()),
		finishCallback(pwsc, nullptr, finishCallback_, 1)
	{
		finishCallback.incRefCount();
		finishCallback.setHighPriority();

		if (auto a = eventList_.getArray())
		{
			eventBuffers.add(new HiseEventBuffer());

			for (const auto& e : *a)
			{
				if (auto me = dynamic_cast<ScriptingObjects::ScriptingMessageHolder*>(e.getObject()))
				{
					eventBuffers.getLast()->addEvent(me->getMessageCopy());

					if(eventBuffers.getLast()->getNumUsed() == HISE_EVENT_BUFFER_SIZE)
					{
						eventBuffers.add(new HiseEventBuffer());
					}
				}
			}
		}

		initAfterFillingEventBuffer();
	}

	~AudioRenderer()
	{
		
	}

	void callUpdateCallback(bool isFinished, double progress) override
	{
		if (finishCallback)
		{
			Array<var> finalChannels;

			for (auto& c : channels)
				finalChannels.add(c.get());

			var args(new DynamicObject());
			
			args.getDynamicObject()->setProperty("channels", finalChannels);
			args.getDynamicObject()->setProperty("finished", isFinished);
			args.getDynamicObject()->setProperty("progress", progress);

			getMainController()->getKillStateHandler().removeThreadIdFromAudioThreadList();
			finishCallback.call(&args, 1);

			if(!isFinished)
				getMainController()->getKillStateHandler().addThreadIdToAudioThreadList();
		}
	}

	WeakCallbackHolder finishCallback;
	
};

void ScriptingApi::Engine::renderAudio(var eventList, var updateCallback)
{
	currentExportThread = new AudioRenderer(getScriptProcessor(), eventList, updateCallback);
}

struct ScriptingApi::Engine::PreviewHandler: public ControlledObject,
											 public AsyncUpdater,
											 public BufferPreviewListener
{
	PreviewHandler(ProcessorWithScriptingContent* p) :
		ControlledObject(p->getMainController_()),
		pwsc(p)
	{
		getMainController()->addPreviewListener(this);
	}

	~PreviewHandler()
	{
		getMainController()->stopBufferToPlay();
		getMainController()->removePreviewListener(this);
	}

	struct Job: public ControlledObject,
				private PooledUIUpdater::SimpleTimer
	{
		Job(ProcessorWithScriptingContent* p, var buffer, var callbackFunction, double fileSampleRate_) :
			ControlledObject(p->getMainController_()),
			SimpleTimer(p->getMainController_()->getGlobalUIUpdater(), true),
			bufferToPreview(buffer),
            fileSampleRate(fileSampleRate_),
			callback(p, nullptr, callbackFunction, 2)
		{
			callback.incRefCount();
			memset(channels, 0, sizeof(float*) * HISE_NUM_PLUGIN_CHANNELS);

			if (buffer.isArray())
			{
				numChannels = buffer.size();
				
				for (int i = 0; i < numChannels; i++)
				{
					if (auto b = buffer[i].getBuffer())
					{
						if (numSamples == -1)
							numSamples = b->buffer.getNumSamples();

						channels[i] = b->buffer.getWritePointer(0);
					}
				}
			}
			else if(auto b = buffer.getBuffer())
			{
				numSamples = b->buffer.getNumSamples();
				numChannels = 1;
				channels[0] = b->buffer.getWritePointer(0);
			}

			if (numChannels == 1)
			{
				numChannels = 2;
				channels[1] = channels[0];
			}
		}

        bool isValid() const { return numChannels > 0 && numSamples > 0; }
        
		void play()
		{
			AudioSampleBuffer b(channels, numChannels, numSamples);

			getMainController()->setBufferToPlay(b, fileSampleRate);

            start();
		}

		void sendCallback(bool isPlaying, double position)
		{
			args[0] = isPlaying;
			args[1] = position;

			if (callback)
				callback.call(args, 2);

			if (!isPlaying)
				stop();
		}

		void timerCallback() override
		{
			auto pos = getMainController()->getPreviewBufferPosition();
			double normPos = (double)pos / (double)numSamples;
			sendCallback(true, normPos);
		}
		
		float* channels[HISE_NUM_PLUGIN_CHANNELS];
		int numChannels;
		int numSamples = -1;

		var args[2];
		var bufferToPreview;
		WeakCallbackHolder callback;
        const double fileSampleRate;
	};

	void addJob(var buffer, var callback, double fileSampleRate)
	{
        jassert(fileSampleRate >= 0.0);
        
		getMainController()->stopBufferToPlay();

		ScopedPointer<Job> nj = new Job(pwsc, buffer, callback, fileSampleRate);
        
        if(nj->isValid())
        {
			nj->play();

            ScopedLock sl(jobLock);
			currentJob.swapWith(nj);
        }
	}

	void handleAsyncUpdate()
	{
		ScopedLock sl(jobLock);

		if (currentJob != nullptr)
		{
			currentJob->sendCallback(false, 1.0);
		}
	}

	void previewStateChanged(bool isPlaying, const AudioSampleBuffer& currentBuffer) override
	{
		if (!isPlaying)
			triggerAsyncUpdate();
	}

	CriticalSection jobLock;
	ScopedPointer<Job> currentJob;
	ProcessorWithScriptingContent* pwsc;
};

void ScriptingApi::Engine::playBuffer(var bufferData, var callback, double fileSampleRate)
{
    if(fileSampleRate <= 0.0)
        fileSampleRate = getSampleRate();
    
	if (previewHandler == nullptr)
		previewHandler = new PreviewHandler(getScriptProcessor());

	previewHandler->addJob(bufferData, callback, fileSampleRate);
}

var ScriptingApi::Engine::createFFT()
{
	return new ScriptingObjects::ScriptFFT(getScriptProcessor());
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

juce::var ScriptingApi::Engine::createMidiAutomationHandler()
{
	return var(new ScriptingObjects::ScriptedMidiAutomationHandler(getScriptProcessor()));
}

var ScriptingApi::Engine::createUserPresetHandler()
{
	return var(new ScriptUserPresetHandler(getScriptProcessor()));
}

juce::var ScriptingApi::Engine::createBroadcaster(var defaultValues)
{
	return var(new ScriptingObjects::ScriptBroadcaster(getScriptProcessor(), defaultValues));
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

juce::var ScriptingApi::Engine::getGlobalRoutingManager()
{
	return var(new ScriptingObjects::GlobalRoutingManagerReference(getScriptProcessor()));
}

juce::var ScriptingApi::Engine::getLorisManager()
{
#if HISE_INCLUDE_LORIS
    return var(new ScriptLorisManager(getScriptProcessor()));
#else
    return var();
#endif
}

juce::var ScriptingApi::Engine::getComplexDataReference(String dataType, String moduleId, int index)
{
	auto p = ProcessorHelpers::getFirstProcessorWithName(getScriptProcessor()->getMainController_()->getMainSynthChain(), moduleId);

	if (auto typed = dynamic_cast<ExternalDataHolder*>(p))
	{
		StringArray dataTypes =
		{
		  "Table",
		  "SliderPack",
		  "AudioFile",
		  "FilterCoefficients",
		  "DisplayBuffer",
		};

		auto idx = dataTypes.indexOf(dataType);

		if (idx == -1)
			reportScriptError("Illegal data type. Must be Table, SliderPack, AudioFile or DisplayBuffer");

		auto dt = (ExternalData::DataType)idx;

		if (auto obj = typed->getComplexBaseType(dt, index))
		{
			auto sp = getScriptProcessor();

			switch (dt)
			{
			case ExternalData::DataType::Table: return var(new ScriptingObjects::ScriptTableData(sp, index, typed));
			case ExternalData::DataType::SliderPack: return var(new ScriptingObjects::ScriptSliderPackData(sp, index, typed));
			case ExternalData::DataType::AudioFile: return var(new ScriptingObjects::ScriptAudioFile(sp, index, typed));
			case ExternalData::DataType::DisplayBuffer: return var(new ScriptingObjects::ScriptRingBuffer(sp, index, typed));
			default: jassertfalse;
			}
		}
		else
		{
			// Don't complain, just return undefined...
			return var();
		}
	}
	else
	{
		reportScriptError("Can't find module with ID " + moduleId);
	}

	RETURN_IF_NO_THROW(var());
}

var ScriptingApi::Engine::createBackgroundTask(String name)
{
	return new ScriptingObjects::ScriptBackgroundTask(getScriptProcessor(), name);
}

void ScriptingApi::Engine::setKeyColour(int keyNumber, int colourAsHex) { getProcessor()->getMainController()->setKeyboardCoulour(keyNumber, Colour(colourAsHex));}

void ScriptingApi::Engine::extendTimeOut(int additionalMilliseconds)
{
	dynamic_cast<JavascriptProcessor*>(getScriptProcessor())->getScriptEngine()->extendTimeout(additionalMilliseconds);
}

void ScriptingApi::Engine::setGlobalPitchFactor(double pitchFactorInSemitones)
{
	pitchFactorInSemitones = jlimit(-12.0, 12.0, pitchFactorInSemitones);
	getScriptProcessor()->getMainController_()->setGlobalPitchFactor(pitchFactorInSemitones);
}

double ScriptingApi::Engine::getGlobalPitchFactor() const
{
	return getScriptProcessor()->getMainController_()->getGlobalPitchFactorSemiTones();
}

void ScriptingApi::Engine::setLowestKeyToDisplay(int keyNumber) { getProcessor()->getMainController()->setLowestKeyToDisplay(keyNumber); }

void ScriptingApi::Engine::showErrorMessage(String message, bool isCritical)
{
	getProcessor()->getMainController()->sendOverlayMessage(isCritical ? DeactiveOverlay::State::CriticalCustomErrorMessage :
		DeactiveOverlay::State::CustomErrorMessage,
		message);
}

void ScriptingApi::Engine::showMessage(String message)
{
	getProcessor()->getMainController()->sendOverlayMessage(DeactiveOverlay::State::CustomInformation, message);
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

void ScriptingApi::Engine::copyToClipboard(String textToCopy)
{
	SystemClipboard::copyTextToClipboard(textToCopy);
}

String ScriptingApi::Engine::getClipboardContent()
{
	return SystemClipboard::getTextFromClipboard();
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
		UserPresetHelpers::saveUserPreset(getProcessor()->getMainController()->getMainSynthChain(), sf->f.getFullPathName());
	}
	else
	{
		getProcessor()->getMainController()->getUserPresetHandler().savePreset(presetName);
	}
}

struct ScriptingApi::Settings::Wrapper
{
	API_METHOD_WRAPPER_0(Settings, getZoomLevel);
	API_VOID_METHOD_WRAPPER_1(Settings, setZoomLevel);
	API_METHOD_WRAPPER_0(Settings, getDiskMode);
	API_VOID_METHOD_WRAPPER_1(Settings, setDiskMode);
	API_METHOD_WRAPPER_0(Settings, getAvailableDeviceTypes);
	API_METHOD_WRAPPER_0(Settings, getCurrentAudioDeviceType)
	API_VOID_METHOD_WRAPPER_1(Settings, setAudioDeviceType)
	API_METHOD_WRAPPER_0(Settings, getAvailableDeviceNames);
	API_METHOD_WRAPPER_0(Settings, getCurrentAudioDevice);
	API_VOID_METHOD_WRAPPER_1(Settings, setAudioDevice);
	
	API_METHOD_WRAPPER_0(Settings, getAvailableOutputChannels);
	API_METHOD_WRAPPER_0(Settings, getCurrentOutputChannel);
	API_VOID_METHOD_WRAPPER_1(Settings, setOutputChannel);
	API_METHOD_WRAPPER_0(Settings, getAvailableBufferSizes);
	API_METHOD_WRAPPER_0(Settings, getCurrentBufferSize);
	API_VOID_METHOD_WRAPPER_1(Settings, setBufferSize);
	API_METHOD_WRAPPER_0(Settings, getAvailableSampleRates);
	API_METHOD_WRAPPER_0(Settings, getCurrentSampleRate);
	API_VOID_METHOD_WRAPPER_1(Settings, setSampleRate);
	API_METHOD_WRAPPER_0(Settings, getCurrentVoiceMultiplier);
	API_VOID_METHOD_WRAPPER_1(Settings, setVoiceMultiplier);
	API_VOID_METHOD_WRAPPER_0(Settings, clearMidiLearn);
	API_METHOD_WRAPPER_0(Settings, getMidiInputDevices);
	API_VOID_METHOD_WRAPPER_2(Settings, toggleMidiInput);
	API_METHOD_WRAPPER_1(Settings, isMidiInputEnabled);
	API_VOID_METHOD_WRAPPER_2(Settings, toggleMidiChannel);
	API_VOID_METHOD_WRAPPER_1(Settings, setSampleFolder);
	API_METHOD_WRAPPER_1(Settings, isMidiChannelEnabled);
	API_METHOD_WRAPPER_0(Settings, getUserDesktopSize);
	API_METHOD_WRAPPER_0(Settings, isOpenGLEnabled);
	API_VOID_METHOD_WRAPPER_1(Settings, setEnableOpenGL);
	API_VOID_METHOD_WRAPPER_1(Settings, setEnableDebugMode);
	API_VOID_METHOD_WRAPPER_0(Settings, startPerfettoTracing);
	API_VOID_METHOD_WRAPPER_1(Settings, stopPerfettoTracing);
	API_VOID_METHOD_WRAPPER_0(Settings, crashAndBurn);
};

ScriptingApi::Settings::Settings(ProcessorWithScriptingContent* s) :
	ScriptingObject(s),
	ApiClass(0)
{
	// Gonna save you some typing...
	mc = dynamic_cast<MainController*>(getScriptProcessor()->getMainController_());
	gm = dynamic_cast<GlobalSettingManager*>(mc);
	driver = dynamic_cast<AudioProcessorDriver*>(mc);

	ADD_API_METHOD_0(getZoomLevel);
	ADD_API_METHOD_1(setZoomLevel);
	ADD_API_METHOD_0(getDiskMode);
	ADD_API_METHOD_1(setDiskMode);
	ADD_API_METHOD_0(getAvailableDeviceTypes);
	ADD_API_METHOD_0(getCurrentAudioDeviceType);
	ADD_API_METHOD_1(setAudioDeviceType);
	ADD_API_METHOD_0(getAvailableDeviceNames);
	ADD_API_METHOD_0(getCurrentAudioDevice);
	ADD_API_METHOD_1(setAudioDevice);
	ADD_API_METHOD_0(getAvailableOutputChannels);
	ADD_API_METHOD_0(getCurrentOutputChannel);
	ADD_API_METHOD_1(setOutputChannel);
	ADD_API_METHOD_0(getAvailableBufferSizes);
	ADD_API_METHOD_0(getCurrentBufferSize);
	ADD_API_METHOD_1(setBufferSize);
	ADD_API_METHOD_0(getAvailableSampleRates);
	ADD_API_METHOD_0(getCurrentSampleRate);
	ADD_API_METHOD_1(setSampleRate);
	ADD_API_METHOD_0(getCurrentVoiceMultiplier);
	ADD_API_METHOD_1(setVoiceMultiplier);
	ADD_API_METHOD_0(clearMidiLearn);
	ADD_API_METHOD_0(getMidiInputDevices);
	ADD_API_METHOD_2(toggleMidiInput);
	ADD_API_METHOD_1(isMidiInputEnabled);
	ADD_API_METHOD_2(toggleMidiChannel);
	ADD_API_METHOD_1(isMidiChannelEnabled);
	ADD_API_METHOD_0(getUserDesktopSize);
	ADD_API_METHOD_0(isOpenGLEnabled);
	ADD_API_METHOD_1(setEnableOpenGL);
	ADD_API_METHOD_1(setEnableDebugMode);
	ADD_API_METHOD_1(setSampleFolder);
	ADD_API_METHOD_0(startPerfettoTracing);
	ADD_API_METHOD_1(stopPerfettoTracing);
	ADD_API_METHOD_0(crashAndBurn);
}

var ScriptingApi::Settings::getUserDesktopSize()
{
	auto area = Desktop::getInstance().getDisplays().getMainDisplay().userArea;

	Array<var> desktopSize;

	desktopSize.add(area.getWidth());
	desktopSize.add(area.getHeight());

	return desktopSize;
}

bool ScriptingApi::Settings::isOpenGLEnabled() const
{
	return driver->useOpenGL;
}

void ScriptingApi::Settings::setEnableOpenGL(bool shouldBeEnabled)
{
	driver->useOpenGL = shouldBeEnabled;
}

void ScriptingApi::Settings::setEnableDebugMode(bool shouldBeEnabled)
{
	shouldBeEnabled ? mc->getDebugLogger().startLogging() : mc->getDebugLogger().stopLogging();	
}

void ScriptingApi::Settings::setSampleFolder(var sampleFolder)
{
	if(auto sf = dynamic_cast<ScriptingObjects::ScriptFile*>(sampleFolder.getObject()))
	{
		auto newLocation = sf->f;

		if(newLocation.isDirectory())
		{
#if USE_BACKEND
			getScriptProcessor()->getMainController_()->getCurrentFileHandler().createLinkFile(FileHandlerBase::Samples, newLocation);
#else
			FrontendHandler::setSampleLocation(newLocation);
#endif
		}
	}
}

void ScriptingApi::Settings::startPerfettoTracing()
{
#if PERFETTO
	auto& mp = MelatoninPerfetto::get();
	mp.beginSession();
#else
	reportScriptError("Perfetto is not enabled, make sure to compile your project / HISE with PERFETTO=1");
#endif
}

void ScriptingApi::Settings::stopPerfettoTracing(var traceFileToUse)
{
#if PERFETTO
	if(auto sf = dynamic_cast<ScriptingObjects::ScriptFile*>(traceFileToUse.getObject()))
	{
		auto extension = sf->f.getFileExtension();

		auto& mp = MelatoninPerfetto::get();

		mp.customFileLocation = sf->f;
		mp.endSession(true);
		mp.customFileLocation = File();

		if(extension != ".pftrace")
		{
			reportScriptError("The file needs the extension .pftrace");
		}
	}
	else
	{
		reportScriptError("Not a valid file supplied");
	}
#else
	reportScriptError("Perfetto is not enabled, make sure to compile your project / HISE with PERFETTO=1");
#endif
}

void ScriptingApi::Settings::crashAndBurn()
{
#if USE_BACKEND
	auto includeSymbols = GET_HISE_SETTING(dynamic_cast<Processor*>(getScriptProcessor()), HiseSettings::Project::CompileWithDebugSymbols);

	if(!includeSymbols)
	{
		// don't crash, you're not ready yet...
		reportScriptError("You need to enable CompileWithDebugSymbols for your project to get a meaningful stack trace");
	}
#endif
	
	// There you go...
	volatile int* x = nullptr;
	*x = 90;

	// If that didn't work, try this...
	abort();
}

double ScriptingApi::Settings::getZoomLevel() const
{
	return gm->getGlobalScaleFactor();
}

void ScriptingApi::Settings::setZoomLevel(double newLevel)
{
	newLevel = jlimit(0.25, 2.0, newLevel);
	gm->setGlobalScaleFactor(newLevel, sendNotificationAsync);
}

int ScriptingApi::Settings::getDiskMode()
{
	return driver->diskMode;
}

void ScriptingApi::Settings::setDiskMode(int mode)
{
	driver->diskMode = mode;
	mc->getSampleManager().setDiskMode((MainController::SampleManager::DiskMode)mode);
}

var ScriptingApi::Settings::getAvailableDeviceTypes()
{
	const OwnedArray<AudioIODeviceType> *devices = &driver->deviceManager->getAvailableDeviceTypes();
	
	Array<var> result;

	for (int i = 0; i < devices->size(); i++)
		result.add(devices->getUnchecked(i)->getTypeName());
	
	return result;
}

String ScriptingApi::Settings::getCurrentAudioDeviceType()
{
	AudioIODevice* currentDevice = driver->deviceManager->getCurrentAudioDevice();
	
	if (currentDevice != nullptr)
		return currentDevice->getTypeName();
	
	return "";
}

void ScriptingApi::Settings::setAudioDeviceType(String deviceName)
{
	driver->setAudioDeviceType(deviceName);
}

var ScriptingApi::Settings::getAvailableDeviceNames()
{
	const OwnedArray<AudioIODeviceType> *devices = &driver->deviceManager->getAvailableDeviceTypes();
	const int thisDevice = devices->indexOf(driver->deviceManager->getCurrentDeviceTypeObject());
	
	AudioIODeviceType *currentDeviceType = devices->getUnchecked(thisDevice);	
	Array<var> result;
	
	if (currentDeviceType != nullptr)
	{
		StringArray soundCardNames = currentDeviceType->getDeviceNames(false);

		for (auto x : soundCardNames)
			result.add(x);
	}	
	
	return result;
}

String ScriptingApi::Settings::getCurrentAudioDevice()
{
	AudioIODevice* currentDevice = driver->deviceManager->getCurrentAudioDevice();
	
	if (currentDevice != nullptr)
		return currentDevice->getName();
		
	return "";
}

void ScriptingApi::Settings::setAudioDevice(String name)
{
	driver->setAudioDevice(name);
}

var ScriptingApi::Settings::getAvailableOutputChannels()
{
	AudioIODevice* currentDevice = driver->deviceManager->getCurrentAudioDevice();
	Array<var> result;	
	
	if (currentDevice != nullptr) 
	{
		StringArray outputPairs = HiseSettings::ConversionHelpers::getChannelPairs(currentDevice);

		for (auto x : outputPairs)
			result.add(x);
	}

	return result;
}

int ScriptingApi::Settings::getCurrentOutputChannel()
{
	AudioIODevice* currentDevice = driver->deviceManager->getCurrentAudioDevice();
	
	if (currentDevice != nullptr)
		return (currentDevice->getActiveOutputChannels().getHighestBit() - 1) / 2;
	
	return 0;
}

void ScriptingApi::Settings::setOutputChannel(int index)
{
	CustomSettingsWindow::flipEnablement(driver->deviceManager, index);
}

var ScriptingApi::Settings::getAvailableBufferSizes()
{
	AudioIODevice* currentDevice = driver->deviceManager->getCurrentAudioDevice();

	Array<var> result;
	
	if (currentDevice != nullptr)
	{
		Array<int> bufferSizes = HiseSettings::ConversionHelpers::getBufferSizesForDevice(currentDevice);
		
		for (auto x : bufferSizes)
			result.add(x);
	}
	
	return result;
}

int ScriptingApi::Settings::getCurrentBufferSize()
{	
	if (driver->deviceManager != nullptr)
		return driver->getCurrentBlockSize();		

	return false;
}

void ScriptingApi::Settings::setBufferSize(int newBlockSize)
{	
	if (driver->deviceManager != nullptr)
		driver->setCurrentBlockSize(newBlockSize);
}

var ScriptingApi::Settings::getAvailableSampleRates()
{
	AudioIODevice* currentDevice = driver->deviceManager->getCurrentAudioDevice();
	Array<var> result;
	
	if (currentDevice != nullptr)
	{
		auto samplerates = HiseSettings::ConversionHelpers::getSampleRates(currentDevice);

		for (auto x : samplerates)
			result.add(String(x, 0));
	}
	
	return result;
}

double ScriptingApi::Settings::getCurrentSampleRate()
{
	AudioIODevice* currentDevice = driver->deviceManager->getCurrentAudioDevice();

	if (currentDevice != nullptr)
		return currentDevice->getCurrentSampleRate();
	
	return -1;
}

void ScriptingApi::Settings::setSampleRate(double sampleRate)
{
	driver->setCurrentSampleRate(sampleRate);
}

int ScriptingApi::Settings::getCurrentVoiceMultiplier()
{
	return driver->voiceAmountMultiplier;	
}

void ScriptingApi::Settings::setVoiceMultiplier(int newVoiceAmount)
{
	driver->voiceAmountMultiplier = newVoiceAmount;
}

void ScriptingApi::Settings::clearMidiLearn()
{
	mc->getMacroManager().getMidiControlAutomationHandler()->clear(sendNotification);
}

var ScriptingApi::Settings::getMidiInputDevices()
{
	return MidiInput::getDevices();
}

void ScriptingApi::Settings::toggleMidiInput(const String &midiInputName, bool enableInput)
{
	driver->toggleMidiInput(midiInputName, enableInput);
}

bool ScriptingApi::Settings::isMidiInputEnabled(const String &midiInputName)
{
	if (driver->deviceManager != nullptr)
		return driver->deviceManager->isMidiInputEnabled(midiInputName);

	return false;
}

void ScriptingApi::Settings::toggleMidiChannel(int index, bool value)
{
	HiseEvent::ChannelFilterData *newData = mc->getMainSynthChain()->getActiveChannelData();
	
	if (index == 0)
		newData->setEnableAllChannels(value);
	else
		newData->setEnableMidiChannel(index - 1, value);	
}

bool ScriptingApi::Settings::isMidiChannelEnabled(int index)
{
	HiseEvent::ChannelFilterData* channelFilterData = mc->getMainSynthChain()->getActiveChannelData();
	
	if (index == 0)
		return channelFilterData->areAllChannelsEnabled();
	else 
		return channelFilterData->isChannelEnabled(index - 1);
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

        userPresetToLoad = userPresetRoot.getChildFile(file.toString());
        
        if(userPresetToLoad.hasFileExtension(".preset"))
            userPresetToLoad = userPresetToLoad.withFileExtension(".preset");
	}

    if(!getProcessor()->getMainController()->isInitialised())
    {
        reportScriptError("Do not load user presets at startup.");
    }
    else if (userPresetToLoad.existsAsFile())
	{
		getProcessor()->getMainController()->getUserPresetHandler().loadUserPreset(userPresetToLoad);
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

bool ScriptingApi::Engine::isUserPresetReadOnly(var optionalFile)
{
#if USE_BACKEND

	// In HISE we just pass the project setting here (since all user presets will be factory presets...
	return GET_HISE_SETTING(getScriptProcessor()->getMainController_()->getMainSynthChain(), HiseSettings::Project::ReadOnlyFactoryPresets);

#elif READ_ONLY_FACTORY_PRESETS
	File fToCheck;

	if (optionalFile.isUndefined())
		fToCheck = getScriptProcessor()->getMainController_()->getUserPresetHandler().getCurrentlyLoadedFile();

	if (auto sf = dynamic_cast<ScriptingObjects::ScriptFile*>(optionalFile.getObject()))
		fToCheck = sf->f;

	if (optionalFile.isString())
	{
		auto root = getScriptProcessor()->getMainController_()->getCurrentFileHandler().getSubDirectory(FileHandlerBase::UserPresets);

		fToCheck = root.getChildFile(optionalFile.toString()).withFileExtension(".preset");
	}

	return getScriptProcessor()->getMainController_()->getUserPresetHandler().isReadOnly(fToCheck);
#else
	return false;
#endif
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
    
    if(!pool->areAllFilesLoaded())
        pool->loadAllFilesFromProjectFolder();

#endif

    auto mc = getScriptProcessor()->getMainController_();
     
    auto poolToLoad = mc->getSampleManager().getProjectHandler().pool.get();
    
    if(FullInstrumentExpansion::isEnabled(mc))
    {
        if(auto e = mc->getExpansionHandler().getCurrentExpansion())
            poolToLoad = e->pool;
    }
    
	auto allList = poolToLoad->getAudioSampleBufferPool().getListOfAllReferences(true);

	Array<var> ar;

	for (auto& ref : allList)
		ar.add(ref.getReferenceString());

	return var(ar);
}

var ScriptingApi::Engine::loadAudioFileIntoBufferArray(String audioFileReference)
{
	PoolReference ref(getScriptProcessor()->getMainController_(), audioFileReference, FileHandlerBase::AudioFiles);

	auto pool = getScriptProcessor()->getMainController_()->getCurrentFileHandler().pool.get();

	if (auto e = getScriptProcessor()->getMainController_()->getExpansionHandler().getExpansionForWildcardReference(ref.getReferenceString()))
	{
		pool = e->pool.get();
	}

	if (auto audioData = pool->getAudioSampleBufferPool().loadFromReference(ref, PoolHelpers::LoadAndCacheStrong))
	{
		auto& b = audioData->data;

		Array<var> channels;

		for (int i = 0; i < b.getNumChannels(); i++)
			channels.add(var(new VariantBuffer(b.getWritePointer(i), b.getNumSamples())));

		return channels;
	}
	else
	{
		reportScriptError("Can't load audio file " + ref.getReferenceString());
		RETURN_IF_NO_THROW(var());
	}
}

juce::var ScriptingApi::Engine::getWavetableList()
{
	if (auto first = ProcessorHelpers::getFirstProcessorWithType<WavetableSynth>(getScriptProcessor()->getMainController_()->getMainSynthChain()))
	{
		Array<var> list;

		for (const auto& w : first->getWavetableList())
			list.add(w);

		return list;
	}

	reportScriptError("You need at least one Wavetable synthesiser in your signal chain for this method");
	RETURN_IF_NO_THROW(var());
}

void ScriptingApi::Engine::loadImageIntoPool(const String& id)
{
#if USE_BACKEND

	HiseJavascriptEngine::TimeoutExtender xt(dynamic_cast<JavascriptProcessor*>(getScriptProcessor())->getScriptEngine());

	auto mc = getScriptProcessor()->getMainController_();

	auto pool = mc->getCurrentImagePool();

	if (auto e = mc->getExpansionHandler().getExpansionForWildcardReference(id))
		pool = &e->pool->getImagePool();

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


hise::ScriptingObjects::ScriptUnorderedStack* ScriptingApi::Engine::createUnorderedStack()
{
	return new ScriptingObjects::ScriptUnorderedStack(getScriptProcessor());
}

hise::ScriptingObjects::ScriptSliderPackData* ScriptingApi::Engine::createAndRegisterSliderPackData(int index)
{
	return new ScriptingObjects::ScriptSliderPackData(getScriptProcessor(), index);
}

hise::ScriptingObjects::ScriptTableData* ScriptingApi::Engine::createAndRegisterTableData(int index)
{
	return new ScriptingObjects::ScriptTableData(getScriptProcessor(), index);
}

hise::ScriptingObjects::ScriptAudioFile* ScriptingApi::Engine::createAndRegisterAudioFile(int index)
{
	return new ScriptingObjects::ScriptAudioFile(getScriptProcessor(), index);
}

hise::ScriptingObjects::ScriptRingBuffer* ScriptingApi::Engine::createAndRegisterRingBuffer(int index)
{
	return new ScriptingObjects::ScriptRingBuffer(getScriptProcessor(), index);
}

ScriptingObjects::TimerObject* ScriptingApi::Engine::createTimerObject() { return new ScriptingObjects::TimerObject(getScriptProcessor()); }

ScriptingObjects::ScriptingMessageHolder* ScriptingApi::Engine::createMessageHolder()
{
	return new ScriptingObjects::ScriptingMessageHolder(getScriptProcessor());
}

ScriptingObjects::ScriptNeuralNetwork* ScriptingApi::Engine::createNeuralNetwork(String id)
{
	return new ScriptingObjects::ScriptNeuralNetwork(getScriptProcessor(), id);
}

var ScriptingApi::Engine::createTransportHandler()
{
	return new TransportHandler(getScriptProcessor());
}

juce::var ScriptingApi::Engine::createModulationMatrix(String containerId)
{
	return new ScriptingObjects::ScriptModulationMatrix(getScriptProcessor(), containerId);
}

var ScriptingApi::Engine::createMacroHandler()
{
	return new ScriptingObjects::ScriptedMacroHandler(getScriptProcessor());
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

String ScriptingApi::Engine::compressJSON(var object)
{
	auto x = JSON::toString(object, true);

	zstd::ZDefaultCompressor comp;
	
	MemoryBlock data;
	comp.compress(x, data);

	return data.toBase64Encoding();
}

var ScriptingApi::Engine::uncompressJSON(const String& b64)
{
	MemoryBlock mb;
	mb.fromBase64Encoding(b64);

	String json;

	zstd::ZDefaultCompressor comp;
	comp.expand(mb, json);

	var value;
	auto r = JSON::parse(json, value);

	if(!r.wasOk())
	{
		reportScriptError(r.getErrorMessage());
	}

	return value;
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

juce::var ScriptingApi::Engine::createErrorHandler()
{
	return new ScriptingObjects::ScriptErrorHandler(getScriptProcessor());
}

var ScriptingApi::Engine::getRegexMatches(String stringToMatch, String wildcard)
{
    try
    {
        std::string s = stringToMatch.toStdString();
        std::regex reg(wildcard.toStdString());
        std::smatch match;

		var returnArray = var();

		// If you're trying to find more than 100000 matches here
		// then you need to step back and evaluate some life choices that
		// lead you to this moment.
		int safeCount = 0;

        while (std::regex_search(s, match, reg) && ++safeCount < 100000)
        {
			for (auto x:match)
                returnArray.insert(-1, String(x));

			s = match.suffix();
        }

		return returnArray;
    }
    catch (std::regex_error e)
    {
        debugError(getProcessor(), e.what());
        return var::undefined();
    }
}

String ScriptingApi::Engine::doubleToString(double value, int digits)
{
    return String(value, digits);
}


float ScriptingApi::Engine::getStringWidth(String text, String fontName, float fontSize, float fontSpacing)
{
	auto mc = getScriptProcessor()->getMainController_();
	return mc->getStringWidthFromEmbeddedFont(text, fontName, fontSize, fontSpacing);
}

void ScriptingApi::Engine::quit()
{
    #if IS_STANDALONE_APP
    JUCEApplication::quit();
	#endif
}
                
String ScriptingApi::Engine::intToHexString(int value)
{
    return String::toHexString(value);
}

void ScriptingApi::Engine::undo()
{
	Array<const juce::UndoableAction*> actions;

	auto um = getScriptProcessor()->getMainController_()->getControlUndoManager();

	if (um->getUndoDescription() == "%SCRIPT_TRANSACTION%")
	{
		um->undo();
		return;
	}

	WeakReference<Processor> p = getProcessor();

	auto f = [p]()
	{
		if(p != nullptr)
			p.get()->getMainController()->getControlUndoManager()->undo();
	};

	MessageManager::callAsync(f);
}

void ScriptingApi::Engine::clearUndoHistory()
{
    auto um = getScriptProcessor()->getMainController_()->getControlUndoManager();
    
    if(um->isPerformingUndoRedo())
    {
        reportScriptError("You can't clear the undo history while performing an undoable operation");
    }
    
    um->clearUndoHistory();
}

void ScriptingApi::Engine::redo()
{
	auto um = getScriptProcessor()->getMainController_()->getControlUndoManager();

	if (um->getRedoDescription() == "%SCRIPT_TRANSACTION%")
	{
		um->redo();
		return;
	}

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

void ScriptingApi::Engine::logSettingWarning(const String& methodName) const
{
	auto p = dynamic_cast<const Processor*>(getScriptProcessor());

	auto unconst = const_cast<Processor*>(p);
    ignoreUnused(unconst);

	String s;
	s << "Engine." << methodName << "() is deprecated. Use Settings." << methodName << "() instead.";
	debugToConsole(unconst, s);
}


// ====================================================================================================== Time functions

struct ScriptingApi::Date::Wrapper
{
	API_METHOD_WRAPPER_1(Date, getSystemTimeISO8601);
	API_METHOD_WRAPPER_0(Date, getSystemTimeMs);
	API_METHOD_WRAPPER_2(Date, millisecondsToISO8601);
	API_METHOD_WRAPPER_1(Date, ISO8601ToMilliseconds);
};

ScriptingApi::Date::Date(ProcessorWithScriptingContent* s) :
	ScriptingObject(s),
	ApiClass(0)
{
	ADD_API_METHOD_1(getSystemTimeISO8601);
	ADD_API_METHOD_0(getSystemTimeMs);
	ADD_API_METHOD_2(millisecondsToISO8601);
	ADD_API_METHOD_1(ISO8601ToMilliseconds);
}

String ScriptingApi::Date::getSystemTimeISO8601(bool includeDividerCharacters)
{
	return Time::getCurrentTime().toISO8601(includeDividerCharacters);
}

int64 ScriptingApi::Date::getSystemTimeMs()
{
	return Time::getCurrentTime().toMilliseconds();
}

String ScriptingApi::Date::millisecondsToISO8601(int64 miliseconds, bool includeDividerCharacters)
{
	return Time(miliseconds).toISO8601(includeDividerCharacters);
}

int64 ScriptingApi::Date::ISO8601ToMilliseconds(String iso8601)
{
    return juce::Time::fromISO8601(iso8601).toMilliseconds();
}


// ====================================================================================================== Sampler functions

struct ScriptingApi::Sampler::Wrapper
{
	API_VOID_METHOD_WRAPPER_1(Sampler, enableRoundRobin);
	API_VOID_METHOD_WRAPPER_1(Sampler, setActiveGroup);
	API_VOID_METHOD_WRAPPER_2(Sampler, setMultiGroupIndex)
	API_METHOD_WRAPPER_0(Sampler, getActiveRRGroup);
	API_VOID_METHOD_WRAPPER_2(Sampler, setActiveGroupForEventId);
	API_METHOD_WRAPPER_1(Sampler, getActiveRRGroupForEventId);
	API_VOID_METHOD_WRAPPER_3(Sampler, setMultiGroupIndexForEventId);
	API_METHOD_WRAPPER_2(Sampler, getRRGroupsForMessage);
	API_VOID_METHOD_WRAPPER_0(Sampler, refreshRRMap);
	API_VOID_METHOD_WRAPPER_1(Sampler, selectSounds);
	API_METHOD_WRAPPER_0(Sampler, getNumSelectedSounds);
	API_VOID_METHOD_WRAPPER_2(Sampler, setSoundPropertyForSelection);
	API_VOID_METHOD_WRAPPER_2(Sampler, setSoundPropertyForAllSamples);
	API_METHOD_WRAPPER_2(Sampler, getSoundProperty);
	API_VOID_METHOD_WRAPPER_3(Sampler, setSoundProperty);
	API_VOID_METHOD_WRAPPER_2(Sampler, purgeMicPosition);
	API_VOID_METHOD_WRAPPER_1(Sampler, purgeSampleSelection);
	API_METHOD_WRAPPER_0(Sampler, getNumMicPositions);
	API_METHOD_WRAPPER_1(Sampler, isMicPositionPurged);
	API_METHOD_WRAPPER_1(Sampler, getMicPositionName);
	API_METHOD_WRAPPER_1(Sampler, isNoteNumberMapped);
	API_VOID_METHOD_WRAPPER_0(Sampler, refreshInterface);
	API_VOID_METHOD_WRAPPER_1(Sampler, loadSampleMap);
	API_METHOD_WRAPPER_0(Sampler, getSampleMapList);
    API_METHOD_WRAPPER_0(Sampler, getCurrentSampleMapId);
    API_VOID_METHOD_WRAPPER_2(Sampler, setAttribute);
	API_VOID_METHOD_WRAPPER_2(Sampler, setRRGroupVolume);
    API_METHOD_WRAPPER_0(Sampler, getNumAttributes);
    API_METHOD_WRAPPER_1(Sampler, getAttribute);
    API_METHOD_WRAPPER_1(Sampler, getAttributeId);
		API_METHOD_WRAPPER_1(Sampler, getAttributeIndex);
	API_VOID_METHOD_WRAPPER_1(Sampler, setUseStaticMatrix);
    API_METHOD_WRAPPER_1(Sampler, loadSampleForAnalysis);
	API_METHOD_WRAPPER_1(Sampler, loadSfzFile);
	API_VOID_METHOD_WRAPPER_1(Sampler, loadSampleMapFromJSON);
	API_VOID_METHOD_WRAPPER_1(Sampler, loadSampleMapFromBase64);
	API_METHOD_WRAPPER_1(Sampler, getAudioWaveformContentAsBase64);
	API_METHOD_WRAPPER_0(Sampler, getSampleMapAsBase64);
	API_VOID_METHOD_WRAPPER_1(Sampler, setTimestretchRatio);
	API_VOID_METHOD_WRAPPER_1(Sampler, setTimestretchOptions);
	API_METHOD_WRAPPER_0(Sampler, getTimestretchOptions);
	API_METHOD_WRAPPER_1(Sampler, createSelection);
	API_METHOD_WRAPPER_1(Sampler, createSelectionFromIndexes);
	API_METHOD_WRAPPER_1(Sampler, createSelectionWithFilter);
	API_METHOD_WRAPPER_0(Sampler, createListFromGUISelection);
	API_METHOD_WRAPPER_0(Sampler, createListFromScriptSelection);
	API_METHOD_WRAPPER_1(Sampler, saveCurrentSampleMap);
	API_METHOD_WRAPPER_2(Sampler, importSamples);
	API_METHOD_WRAPPER_0(Sampler, clearSampleMap);
	API_METHOD_WRAPPER_1(Sampler, parseSampleFile);
	API_VOID_METHOD_WRAPPER_2(Sampler, setGUISelection);
	API_VOID_METHOD_WRAPPER_1(Sampler, setSortByRRGroup);
};


ScriptingApi::Sampler::Sampler(ProcessorWithScriptingContent *p, ModulatorSampler *sampler_) :
ConstScriptingObject(p, SampleIds::numProperties),
sampler(sampler_)
{
	ADD_API_METHOD_1(enableRoundRobin);
	ADD_API_METHOD_1(setActiveGroup);
	ADD_API_METHOD_0(getActiveRRGroup);
	ADD_API_METHOD_2(setActiveGroupForEventId);
	ADD_API_METHOD_1(getActiveRRGroupForEventId);
	ADD_API_METHOD_3(setMultiGroupIndexForEventId);
	ADD_API_METHOD_2(setRRGroupVolume);
	ADD_API_METHOD_2(setMultiGroupIndex);
	ADD_API_METHOD_2(getRRGroupsForMessage);
	ADD_API_METHOD_0(refreshRRMap);
	ADD_API_METHOD_1(selectSounds);
	ADD_API_METHOD_0(getNumSelectedSounds);
	ADD_API_METHOD_2(setSoundPropertyForSelection);
	ADD_API_METHOD_2(setSoundPropertyForAllSamples);
	ADD_API_METHOD_2(getSoundProperty);
	ADD_API_METHOD_3(setSoundProperty);
	ADD_API_METHOD_2(purgeMicPosition);
	ADD_API_METHOD_1(purgeSampleSelection);
	ADD_API_METHOD_1(getMicPositionName);
	ADD_API_METHOD_0(getNumMicPositions);
	ADD_API_METHOD_1(isMicPositionPurged);
	ADD_API_METHOD_0(refreshInterface);
	ADD_API_METHOD_1(loadSampleMap);
    ADD_API_METHOD_0(getCurrentSampleMapId);
	ADD_API_METHOD_0(getSampleMapList);
	ADD_API_METHOD_0(getNumAttributes);
    ADD_TYPED_API_METHOD_1(getAttribute, VarTypeChecker::Number);
    ADD_TYPED_API_METHOD_1(getAttributeId, VarTypeChecker::Number);
    ADD_TYPED_API_METHOD_1(getAttributeIndex, VarTypeChecker::String);
    ADD_TYPED_API_METHOD_2(setAttribute, VarTypeChecker::Number, VarTypeChecker::Number);
	ADD_API_METHOD_1(isNoteNumberMapped);
    ADD_API_METHOD_1(loadSampleForAnalysis);
	ADD_API_METHOD_1(loadSfzFile);
	ADD_API_METHOD_1(setUseStaticMatrix);
	ADD_API_METHOD_1(setSortByRRGroup);
	ADD_API_METHOD_1(createSelection);
	ADD_TYPED_API_METHOD_1(createSelectionFromIndexes, VarTypeChecker::Array);
	ADD_API_METHOD_1(createSelectionWithFilter);
	ADD_API_METHOD_0(createListFromGUISelection);
	ADD_API_METHOD_0(createListFromScriptSelection);
	ADD_API_METHOD_1(saveCurrentSampleMap);
	ADD_API_METHOD_1(parseSampleFile);
	ADD_API_METHOD_2(importSamples);
	ADD_API_METHOD_0(clearSampleMap);
	ADD_API_METHOD_2(setGUISelection);
	ADD_API_METHOD_1(loadSampleMapFromJSON);
	ADD_API_METHOD_1(loadSampleMapFromBase64);
	ADD_API_METHOD_0(getSampleMapAsBase64);
	ADD_API_METHOD_1(getAudioWaveformContentAsBase64);
	ADD_API_METHOD_1(setTimestretchRatio);
	ADD_API_METHOD_1(setTimestretchOptions);
	ADD_API_METHOD_0(getTimestretchOptions);

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
    sampleIds.add(SampleIds::NumQuarters);

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
	setActiveGroupForEventId(-1, activeGroupIndex);
}

void ScriptingApi::Sampler::setActiveGroupForEventId(int eventId, int activeGroupIndex)
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
	
	if(eventId != -1 && s->getMainController()->getKillStateHandler().getCurrentThread() != MainController::KillStateHandler::TargetThread::AudioThread)
	{
		reportScriptError("This method is only available in the onNoteOnCallback");
		return;
	}

	bool ok = s->setCurrentGroupIndex(activeGroupIndex, eventId);

	if (!ok)
	{
		reportScriptError(String(activeGroupIndex) + " is not a valid group index.");
	}
}

void ScriptingApi::Sampler::setMultiGroupIndexForEventId(int eventId, var groupIndex, bool enabled)
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
	
	if (groupIndex.isArray())
	{
		for (const auto& v : *groupIndex.getArray())
		{
			auto gIndex = (int)v;
			auto ok = s->setMultiGroupState(gIndex, enabled, eventId);

			if (!ok)
				reportScriptError(String(gIndex) + " is not a valid group index.");
		}
	}
	else if (groupIndex.isObject())
	{
		if (auto ml = dynamic_cast<ScriptingObjects::MidiList*>(groupIndex.getObject()))
			s->setMultiGroupState(ml->getRawDataPointer(), ml->getNumSetValues(), eventId);
	}
	else
	{
		auto ok = s->setMultiGroupState(groupIndex, enabled, eventId);

		if (!ok)
			reportScriptError(groupIndex.toString() + " is not a valid group index.");
	}
}

void ScriptingApi::Sampler::setMultiGroupIndex(var groupIndex, bool enabled)
{
	setMultiGroupIndexForEventId(-1, groupIndex, enabled);
}

void ScriptingApi::Sampler::setRRGroupVolume(int groupIndex, int gainInDecibels)
{
	ModulatorSampler *s = static_cast<ModulatorSampler*>(sampler.get());

	if (s == nullptr)
	{
		reportScriptError("setRRGroupVolume() only works with Samplers.");
		return;
	}

	s->setRRGroupVolume(groupIndex, Decibels::decibelsToGain((float)gainInDecibels));
}


int ScriptingApi::Sampler::getActiveRRGroupForEventId(int eventId)
{
	ModulatorSampler *s = static_cast<ModulatorSampler*>(sampler.get());

	if (s == nullptr)
	{
		reportScriptError("getActiveRRGroup() only works with Samplers.");
		return 0;
	}

	return s->getCurrentRRGroup(eventId);
}

int ScriptingApi::Sampler::getActiveRRGroup()
{
	return getActiveRRGroupForEventId(-1);
}

int ScriptingApi::Sampler::getNumActiveGroups() const
{
	ModulatorSampler *s = static_cast<ModulatorSampler*>(sampler.get());

	if (s == nullptr)
	{
		reportScriptError("getActiveRRGroup() only works with Samplers.");
		return 0;
	}

	return s->getNumActiveGroups();
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

var ScriptingApi::Sampler::createSelectionWithFilter(var filterFunction)
{
	ModulatorSampler *s = static_cast<ModulatorSampler*>(sampler.get());

	if (s == nullptr)
	{
		reportScriptError("createSelectionWithFilter() only works with Samplers.");
		RETURN_IF_NO_THROW({});
	}

	ReferenceCountedArray<ModulatorSamplerSound> list;

	{
		ModulatorSampler::SoundIterator it(s, false);

		while (auto so = it.getNextSound())
			list.add(so.get());
	}

	Array<var> results;

	if (auto jp = dynamic_cast<JavascriptProcessor*>(getScriptProcessor()))
	{
		auto engine = jp->getScriptEngine();

		

		for (auto so : list)
		{
			var x = var(new ScriptingObjects::ScriptingSamplerSound(getScriptProcessor(), s, so));
			var::NativeFunctionArgs args(x, nullptr, 0);

			auto ok = (int)engine->callExternalFunctionRaw(filterFunction, args);

			if(ok != 0)
				results.add(x);
		}
	}

	

	return var(results);
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

		for (auto sound : *s->getSampleEditHandler())
			newSelection.add(new ScriptingObjects::ScriptingSamplerSound(getScriptProcessor(), s, sound));
	}

	return newSelection;
#else
	return {};
#endif
}

void ScriptingApi::Sampler::setGUISelection(var sampleList, bool addToSelection)
{
	WARN_IF_AUDIO_THREAD(true, ScriptGuard::IllegalApiCall);

#if USE_BACKEND
	ModulatorSampler *s = static_cast<ModulatorSampler*>(sampler.get());

	if (s == nullptr)
	{
		reportScriptError("setGUISelection() only works with Samplers.");
		return;
	}

	auto& selection = s->getSampleEditHandler()->getSelectionReference();

	

	if (!addToSelection)
		selection.deselectAll();

	if (auto ar = sampleList.getArray())
	{
		MessageManagerLock mm;

		for (auto& s : *ar)
		{
			if (auto sound = dynamic_cast<ScriptingObjects::ScriptingSamplerSound*>(s.getObject()))
			{
				selection.addToSelectionBasedOnModifiers(sound->getSoundPtr(), ModifierKeys::commandModifier);
			}
		}
	}
		
	MessageManager::callAsync([s]()
	{
		s->getSampleEditHandler()->setMainSelectionToLast();
	});
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

	if (auto sound = soundSelection.getSelectedItem(soundIndex).get())
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

void ScriptingApi::Sampler::purgeSampleSelection(var selection)
{
	ReferenceCountedArray<ModulatorSamplerSound> soundsToBePurged;
	ReferenceCountedArray<ModulatorSamplerSound> allSounds;

	ModulatorSampler *s = static_cast<ModulatorSampler*>(sampler.get());

	soundsToBePurged.ensureStorageAllocated(s->getNumSounds());
	allSounds.ensureStorageAllocated(s->getNumSounds());

	if (s == nullptr)
	{
		reportScriptError("purgeMicPosition() only works with Samplers.");
		RETURN_VOID_IF_NO_THROW()
	}

	ModulatorSampler::SoundIterator iter(s);

	while (auto sound = iter.getNextSound())
		allSounds.add(sound);

	if (selection.isArray())
	{
		for(auto e: *selection.getArray())
			if (auto sObj = dynamic_cast<ScriptingObjects::ScriptingSamplerSound*>(e.getObject()))
			{
				auto mPtr = sObj->getSoundPtr();

				if (!allSounds.contains(mPtr.get()))
					reportScriptError("the sound " + mPtr->getPropertyAsString(SampleIds::FileName) + " is not loaded into the Sampler");

				if(soundsToBePurged.contains(mPtr.get()))
					reportScriptError("the sound " + mPtr->getPropertyAsString(SampleIds::FileName) + " exists more than once in the selection");

				soundsToBePurged.add(mPtr);
			}
			else
			{
				reportScriptError("the array must contain only Sound objects");
			}
	}

	auto f = [allSounds, soundsToBePurged](Processor* p)
	{
		auto s = static_cast<ModulatorSampler*>(p);

		for (auto sound : allSounds)
		{
			sound->setPurged(soundsToBePurged.contains(sound));
		}

		s->refreshPreloadSizes();
		s->refreshMemoryUsage();

		return SafeFunctionCall::OK;
	};

	s->killAllVoicesAndCall(f, true);
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

	s->sendOtherChangeMessage(dispatch::library::ProcessorChangeEvent::Custom, dispatch::sendNotificationAsync);
	s->getMainController()->getSampleManager().getModulatorSamplerSoundPool2()->sendChangeMessage();
}

void ScriptingApi::Sampler::loadSampleMap(const String &fileName)
{
	WARN_IF_AUDIO_THREAD(true, ScriptGuard::IllegalApiCall);

	jassert(LockHelpers::isLockedBySameThread(getScriptProcessor()->getMainController_(), LockHelpers::Type::ScriptLock));

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



void ScriptingApi::Sampler::loadSampleMapFromJSON(var jsonSampleList)
{
	ModulatorSampler *s = dynamic_cast<ModulatorSampler*>(sampler.get());

	if (s == nullptr)
		reportScriptError("Invalid sampler call");

	auto v = convertJSONListToValueTree(jsonSampleList);

	if (!v.isValid())
		return;

	s->killAllVoicesAndCall([v](Processor* p)
		{
			dynamic_cast<ModulatorSampler*>(p)->getSampleMap()->loadUnsavedValueTree(v);
			return SafeFunctionCall::OK;
		}, true);
}

void ScriptingApi::Sampler::loadSampleMapFromBase64(const String& b64)
{
	ModulatorSampler *s = dynamic_cast<ModulatorSampler*>(sampler.get());

	if (s == nullptr)
		reportScriptError("Invalid sampler call");

	s->killAllVoicesAndCall([b64](Processor* p)
	{
		juce::MemoryBlock mb;
		mb.fromBase64Encoding(b64);

		zstd::ZDefaultCompressor comp;
		ValueTree v;
		comp.expand(mb, v);
		dynamic_cast<ModulatorSampler*>(p)->getSampleMap()->loadUnsavedValueTree(v);
		return SafeFunctionCall::OK;
	}, true);
}

String ScriptingApi::Sampler::getSampleMapAsBase64()
{
	ModulatorSampler *s = dynamic_cast<ModulatorSampler*>(sampler.get());

	if (s == nullptr)
		reportScriptError("Invalid sampler call");

	MemoryBlock mb;
	auto v = s->getSampleMap()->getValueTree();

	zstd::ZDefaultCompressor comp;
	comp.compress(v, mb);
	return mb.toBase64Encoding();
}

var ScriptingApi::Sampler::parseSampleFile(var sampleFile)
{
	ModulatorSampler *s = dynamic_cast<ModulatorSampler*>(sampler.get());

	if (s == nullptr)
		reportScriptError("Invalid sampler call");

	File f;

	if (auto sf = dynamic_cast<ScriptingObjects::ScriptFile*>(sampleFile.getObject()))
		f = sf->f;
	else if (sampleFile.isString() && File::isAbsolutePath(sampleFile.toString()))
		f = File(sampleFile.toString());
	else
		reportScriptError("not a valid file input");

	auto v = s->parseMetadata(f);

	if (v.isValid())
	{
		DynamicObject::Ptr obj = new DynamicObject();

		for (int i = 0; i < v.getNumProperties(); i++)
		{
			auto id = v.getPropertyName(i);
			obj->setProperty(id, v[id]);
		}

		return var(obj.get());
	}

	return var();
}

void ScriptingApi::Sampler::setTimestretchRatio(double newRatio)
{
	ModulatorSampler* s = dynamic_cast<ModulatorSampler*>(sampler.get());

	if (s == nullptr)
		reportScriptError("Invalid sampler call");

	s->setTimestretchRatio(newRatio);
}

var ScriptingApi::Sampler::getTimestretchOptions()
{
	ModulatorSampler* s = dynamic_cast<ModulatorSampler*>(sampler.get());

	if (s == nullptr)
		reportScriptError("Invalid sampler call");

	auto o = s->getTimestretchOptions();

	return o.toJSON();
}

void ScriptingApi::Sampler::setTimestretchOptions(var newOptions)
{
	ModulatorSampler* s = dynamic_cast<ModulatorSampler*>(sampler.get());

	if (s == nullptr)
		reportScriptError("Invalid sampler call");

	ModulatorSampler::TimestretchOptions no;
	no.fromJSON(newOptions);
	s->setTimestretchOptions(no);
}

String ScriptingApi::Sampler::getAudioWaveformContentAsBase64(var presetObj)
{
	auto fileName = presetObj.getProperty("data", "").toString();

	Array<var> data;

	if (File::isAbsolutePath(fileName))
	{
		auto sampleStart = (int)presetObj.getProperty("rangeStart", 0);
		auto sampleEnd = (int)presetObj.getProperty("rangeEnd", 0);

		auto newSample = parseSampleFile(var(fileName));

		if (auto obj = newSample.getDynamicObject())
		{
			if (sampleStart != 0)
				obj->setProperty(SampleIds::SampleStart, sampleStart);

			if (sampleEnd != 0)
				obj->setProperty(SampleIds::SampleEnd, sampleEnd);
		}

		data.add(newSample);
	}

	auto v = convertJSONListToValueTree(var(data));

	MemoryBlock mb;

	zstd::ZDefaultCompressor comp;
	comp.compress(v, mb);
	return mb.toBase64Encoding();
}

var ScriptingApi::Sampler::loadSfzFile(var sfzFile)
{
	WARN_IF_AUDIO_THREAD(true, ScriptGuard::IllegalApiCall);

	jassert(LockHelpers::isLockedBySameThread(getScriptProcessor()->getMainController_(), LockHelpers::Type::ScriptLock));

	ModulatorSampler *s = static_cast<ModulatorSampler*>(sampler.get());

	if (s != nullptr)
	{
		File f;

		if (auto sf = dynamic_cast<ScriptingObjects::ScriptFile*>(sfzFile.getObject()))
			f = sf->f;

		if (sfzFile.isString())
		{
			auto fn = sfzFile.toString();

			if (File::isAbsolutePath(fn))
				f = File(fn);
		}

		if(f.existsAsFile())
		{
			SfzImporter imp(nullptr, f);

			try
			{
				auto delta = Time::getMillisecondCounter();
				auto v = imp.importSfzFile();
				v.setProperty("ID", "CustomSFZ", nullptr);
				delta = Time::getMillisecondCounter() - delta;
				dynamic_cast<JavascriptProcessor*>(getScriptProcessor())->getScriptEngine()->extendTimeout(delta);

				if (v.isValid())
				{
					s->killAllVoicesAndCall([v](Processor* p)
						{
							auto sampler = static_cast<ModulatorSampler*>(p);

							sampler->getSampleMap()->loadUnsavedValueTree(v);
							sampler->refreshPreloadSizes();
							sampler->refreshMemoryUsage();
							return SafeFunctionCall::OK;
						});

					return var();
				}

				return var("No sample content");
			}
			catch (SfzImporter::SfzParsingError& p)
			{
				return var(p.getErrorMessage());
			}
			
		}
	}

	return var("Unknown error");
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

int ScriptingApi::Sampler::getAttributeIndex(String parameterId)
{
    ModulatorSampler *s = static_cast<ModulatorSampler*>(sampler.get());

    if (checkValidObject())
        return s->getParameterIndexForIdentifier(parameterId);

    return -1;
}

void ScriptingApi::Sampler::setAttribute(int index, var newValue)
{
    ModulatorSampler *s = static_cast<ModulatorSampler*>(sampler.get());

    if (s == nullptr)
    {
        reportScriptError("setAttribute() only works with Samplers.");
        RETURN_VOID_IF_NO_THROW()
    }

    s->setAttribute(index, newValue, ProcessorHelpers::getAttributeNotificationType());
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

	auto f = [](Processor* p)
	{
		auto sm = static_cast<ModulatorSampler*>(p)->getSampleMap();
		sm->clear(sendNotificationAsync);
		return SafeFunctionCall::OK;
	};

	s->killAllVoicesAndCall(f);
	return true;
}

juce::ValueTree ScriptingApi::Sampler::convertJSONListToValueTree(var jsonSampleList)
{
	if (auto a = jsonSampleList.getArray())
	{
		auto v = ValueTreeConverters::convertVarArrayToFlatValueTree(jsonSampleList, "samplemap", "sample");
		v.setProperty("ID", "CustomJSON", nullptr);
		v.setProperty("SaveMode", 0, nullptr);
		v.setProperty("RRGroupAmount", 1, nullptr);
		v.setProperty("MicPositions", ";", nullptr);

		auto addMissingProp = [](ValueTree& c, const Identifier& id, var defaultValue)
		{
			if (!c.hasProperty(id))
				c.setProperty(id, defaultValue, nullptr);
		};

		for (auto c : v)
		{
			addMissingProp(c, SampleIds::LoVel, 0);
			addMissingProp(c, SampleIds::HiVel, 127);
			addMissingProp(c, SampleIds::LoKey, 0);
			addMissingProp(c, SampleIds::HiKey, 127);
			addMissingProp(c, SampleIds::Root, 64);
			addMissingProp(c, SampleIds::RRGroup, 1);
		}

		return v;
	}

	return {};
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
    API_VOID_METHOD_WRAPPER_3(Synth, playNoteFromUI);
    API_VOID_METHOD_WRAPPER_2(Synth, noteOffFromUI);
	API_METHOD_WRAPPER_4(Synth, playNoteWithStartOffset);
	API_VOID_METHOD_WRAPPER_2(Synth, setAttribute);
	API_METHOD_WRAPPER_1(Synth, getAttribute);
	API_METHOD_WRAPPER_4(Synth, addNoteOn);
	API_VOID_METHOD_WRAPPER_3(Synth, addNoteOff);
	API_VOID_METHOD_WRAPPER_1(Synth, setFixNoteOnAfterNoteOff);
	API_VOID_METHOD_WRAPPER_3(Synth, addVolumeFade);
	API_VOID_METHOD_WRAPPER_4(Synth, addPitchFade);
	API_VOID_METHOD_WRAPPER_4(Synth, addController);
	API_METHOD_WRAPPER_1(Synth, addMessageFromHolder);
	API_METHOD_WRAPPER_2(Synth, attachNote);
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
	API_METHOD_WRAPPER_1(Synth, getDisplayBufferSource);
	API_METHOD_WRAPPER_1(Synth, getTableProcessor);
	API_METHOD_WRAPPER_1(Synth, getSliderPackProcessor);
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
	API_METHOD_WRAPPER_1(Synth, isArtificialEventActive);
	API_VOID_METHOD_WRAPPER_1(Synth, setClockSpeed);
	API_VOID_METHOD_WRAPPER_1(Synth, setShouldKillRetriggeredNote);
	API_VOID_METHOD_WRAPPER_2(Synth, setUseUniformVoiceHandler);
	API_METHOD_WRAPPER_0(Synth, createBuilder);
	
};


ScriptingApi::Synth::Synth(ProcessorWithScriptingContent *p, Message* messageObject_, ModulatorSynth *ownerSynth) :
	ScriptingObject(p),
	ApiClass(0),
	moduleHandler(dynamic_cast<Processor*>(p), dynamic_cast<JavascriptProcessor*>(p)),
	messageObject(messageObject_),
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
    ADD_API_METHOD_3(playNoteFromUI);
	ADD_API_METHOD_2(attachNote);
    ADD_API_METHOD_2(noteOffFromUI);
	ADD_API_METHOD_1(setFixNoteOnAfterNoteOff);
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
	ADD_API_METHOD_2(setUseUniformVoiceHandler);
	ADD_API_METHOD_3(addModulator);
	ADD_API_METHOD_3(addEffect);
	ADD_API_METHOD_1(getMidiPlayer);
	ADD_API_METHOD_1(removeEffect);
	ADD_API_METHOD_1(removeModulator);
	ADD_API_METHOD_1(getModulator);
	ADD_API_METHOD_1(getAudioSampleProcessor);
	ADD_API_METHOD_1(getDisplayBufferSource);
	ADD_API_METHOD_1(getTableProcessor);
	ADD_API_METHOD_1(getSliderPackProcessor);
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
	ADD_API_METHOD_1(isArtificialEventActive);
	ADD_API_METHOD_1(setClockSpeed);
	ADD_API_METHOD_1(setShouldKillRetriggeredNote);
	ADD_API_METHOD_0(createBuilder);
	
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

		if (getProcessor()->getMainController()->getKillStateHandler().getCurrentThread() == MainController::KillStateHandler::TargetThread::AudioThread)
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
		// The note might already be killed, but you want to change the timestamp afterwards...
		parentMidiProcessor->setArtificialTimestamp(eventId, timestamp);
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

void ScriptingApi::Synth::playNoteFromUI(int channel, int noteNumber, int velocity)
{
    CustomKeyboardState& state = getScriptProcessor()->getMainController_()->getKeyboardState();
    
    state.injectMessage(MidiMessage::noteOn(channel, noteNumber, (float)velocity * 127.0f));
}

void ScriptingApi::Synth::noteOffFromUI(int channel, int noteNumber)
{
    CustomKeyboardState& state = getScriptProcessor()->getMainController_()->getKeyboardState();
    state.injectMessage(MidiMessage::noteOff(channel, noteNumber));
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

bool ScriptingApi::Synth::attachNote(int originalNoteId, int artificialNoteId)
{
	if (parentMidiProcessor != nullptr)
	{
		if(!owner->midiProcessorChain->hasAttachedNoteBuffer())
			reportScriptError("You must call setFixNoteOnAfterNoteOff() before calling this method");

		return owner->midiProcessorChain->attachNote(originalNoteId, artificialNoteId);
	}

	return false;
}

void ScriptingApi::Synth::setFixNoteOnAfterNoteOff(bool shouldBeFixed)
{
	if (parentMidiProcessor != nullptr)
		owner->midiProcessorChain->setFixNoteOnAfterNoteOff(shouldBeFixed);
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
					if (messageObject != nullptr)
						messageObject->pushArtificialNoteOn(e);

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

                if(controllerNumber == HiseEvent::PitchWheelCCNumber)
                {
                    e = HiseEvent(HiseEvent::Type::PitchBend, 0, 0);
                    e.setPitchWheelValue(controllerValue);
                }
				else if (controllerNumber == HiseEvent::AfterTouchCCNumber)
				{
					e = HiseEvent(HiseEvent::Type::Aftertouch, 0, controllerValue);
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

	RETURN_IF_NO_THROW({});
}

ScriptingObjects::ScriptingAudioSampleProcessor * ScriptingApi::Synth::getAudioSampleProcessor(const String &name)
{
	WARN_IF_AUDIO_THREAD(true, ScriptGuard::ObjectCreation);

	Processor::Iterator<ProcessorWithExternalData> it(owner);

	ProcessorWithExternalData *asp;
    
	while ((asp = it.getNextProcessor()) != nullptr)
	{
		if (dynamic_cast<Processor*>(asp)->getId() == name)
		{
			if (asp->getNumDataObjects(ExternalData::DataType::AudioFile) > 0)
			{
				return new ScriptAudioSampleProcessor(getScriptProcessor(), dynamic_cast<Processor*>(asp));
			}
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
		Processor::Iterator<ExternalDataHolder> it(owner);

		while (auto lut = it.getNextProcessor())
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

hise::ScriptingApi::Synth::ScriptSliderPackProcessor* ScriptingApi::Synth::getSliderPackProcessor(const String& name)
{
	WARN_IF_AUDIO_THREAD(true, ScriptGuard::ObjectCreation);

	if (getScriptProcessor()->objectsCanBeCreated())
	{
		Processor::Iterator<ExternalDataHolder> it(owner);

		while (auto sp = it.getNextProcessor())
		{
			if (dynamic_cast<Processor*>(sp)->getId() == name)
			{
				return new ScriptSliderPackProcessor(getScriptProcessor(), sp);
			}
		}

		reportScriptError(name + " was not found. ");
		RETURN_IF_NO_THROW(new ScriptSliderPackProcessor(getScriptProcessor(), nullptr));
	}
	else
	{
		reportIllegalCall("getSliderPackProcessor()", "onInit");
		RETURN_IF_NO_THROW(new ScriptSliderPackProcessor(getScriptProcessor(), nullptr));
	}
}

hise::ScriptingObjects::ScriptDisplayBufferSource* ScriptingApi::Synth::getDisplayBufferSource(const String& name)
{
	using namespace snex;

	WARN_IF_AUDIO_THREAD(true, ScriptGuard::ObjectCreation);

	if (getScriptProcessor()->objectsCanBeCreated())
	{
		Processor::Iterator<ProcessorWithExternalData> it(owner);

		while (auto eh = it.getNextProcessor())
		{
			if (dynamic_cast<Processor*>(eh)->getId() == name)
			{
				if (eh->getNumDataObjects(ExternalData::DataType::DisplayBuffer) > 0)
					return new ScriptingObjects::ScriptDisplayBufferSource(getScriptProcessor(), eh);
				else
					reportScriptError("No display buffer available");
			}
		}

		reportScriptError(name + " was not found. ");
		RETURN_IF_NO_THROW(new ScriptingObjects::ScriptDisplayBufferSource(getScriptProcessor(), nullptr));
	}
	else
	{
		reportIllegalCall("getScriptingTableProcessor()", "onInit");
		RETURN_IF_NO_THROW(new ScriptingObjects::ScriptDisplayBufferSource(getScriptProcessor(), nullptr));
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
		Processor::Iterator<HotswappableProcessor> it(owner);

		while (auto s = dynamic_cast<EffectProcessor*>(it.getNextProcessor()))
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

juce::var ScriptingApi::Synth::createBuilder()
{
	return var(new ScriptingObjects::ScriptBuilder(getScriptProcessor()));
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

						if (getProcessor()->getMainController()->getKillStateHandler().getCurrentThread() == MainController::KillStateHandler::TargetThread::AudioThread)
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

						if (messageObject != nullptr)
							messageObject->pushArtificialNoteOn(m);

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

	modulator->sendOtherChangeMessage(dispatch::library::ProcessorChangeEvent::Attribute);


}


bool ScriptingApi::Synth::isArtificialEventActive(int eventId)
{
	return getScriptProcessor()->getMainController_()->getEventHandler().isArtificialEventId((uint16)eventId);
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



void ScriptingApi::Synth::setUseUniformVoiceHandler(String containerId, bool shouldUseUniformVoiceHandling)
{
	Processor::Iterator<ModulatorSynthChain> iter(getScriptProcessor()->getMainController_()->getMainSynthChain());

	while (auto s = iter.getNextProcessor())
	{
		if (s->getId() == containerId)
		{
			s->setUseUniformVoiceHandler(shouldUseUniformVoiceHandling, nullptr);
			return;
		}
	}

	reportScriptError("Can't find Container with ID " + containerId);
}

// ====================================================================================================== Console functions

struct ScriptingApi::Console::Wrapper
{
	API_VOID_METHOD_WRAPPER_1(Console, print);
	API_VOID_METHOD_WRAPPER_0(Console, startBenchmark);
	API_VOID_METHOD_WRAPPER_0(Console, stopBenchmark);
	API_VOID_METHOD_WRAPPER_1(Console, stop);
	API_VOID_METHOD_WRAPPER_0(Console, clear);
	API_VOID_METHOD_WRAPPER_1(Console, assertTrue);
	API_VOID_METHOD_WRAPPER_2(Console, assertEqual);
	API_VOID_METHOD_WRAPPER_1(Console, assertIsDefined);
	API_VOID_METHOD_WRAPPER_1(Console, assertIsObjectOrArray);
	API_VOID_METHOD_WRAPPER_1(Console, assertLegalNumber);
	API_VOID_METHOD_WRAPPER_1(Console, assertNoString);
	API_VOID_METHOD_WRAPPER_0(Console, breakInDebugger);
	API_VOID_METHOD_WRAPPER_0(Console, blink);
};

ScriptingApi::Console::Console(ProcessorWithScriptingContent *p) :
ScriptingObject(p),
ApiClass(0),
startTime(0.0)
{
	ADD_API_METHOD_1(print);
	ADD_API_METHOD_0(startBenchmark);
	ADD_API_METHOD_0(stopBenchmark);
	ADD_API_METHOD_1(stop);
	ADD_API_METHOD_0(clear);
	ADD_API_METHOD_0(blink);

	ADD_API_METHOD_1(assertTrue);
	ADD_API_METHOD_2(assertEqual);
	ADD_API_METHOD_1(assertIsDefined);
	ADD_API_METHOD_1(assertIsObjectOrArray);
	ADD_API_METHOD_1(assertLegalNumber);

	ADD_API_METHOD_0(breakInDebugger);
	ADD_API_METHOD_1(assertNoString);
}



void ScriptingApi::Console::print(var x)
{
#if USE_BACKEND

	AudioThreadGuard::Suspender suspender;
	ignoreUnused(suspender);
	

    auto jp = dynamic_cast<JavascriptProcessor*>(getScriptProcessor());
    jp->addInplaceDebugValue(id, lineNumber, x.toString());
    
	debugToConsole(getProcessor(), x);
#else
	DBG(x.toString());
#endif
}

void ScriptingApi::Console::stopBenchmark()
{
#if USE_BACKEND
	AudioThreadGuard::Suspender suspender;
	ignoreUnused(suspender);

	if (startTime == 0.0)
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

void ScriptingApi::Console::stop(bool condition)
{
	if (!condition)
		return;

	auto c = getScriptProcessor()->getMainController_()->getKillStateHandler().getCurrentThread();

	if (c == MainController::KillStateHandler::TargetThread::ScriptingThread ||
		c == MainController::KillStateHandler::TargetThread::SampleLoadingThread ||
		c == MainController::KillStateHandler::TargetThread::AudioThread)
	{
		auto n = Time::getMillisecondCounter();

		auto jp = dynamic_cast<JavascriptProcessor*>(getScriptProcessor());
		
		MessageManager::callAsync([jp]()
		{
			ScopedReadLock sl(jp->getDebugLock());
			jp->getScriptEngine()->rebuildDebugInformation();
			jp->rebuild();
		});

		auto& jtp = getScriptProcessor()->getMainController_()->getJavascriptThreadPool();

		JavascriptThreadPool::ScopedSleeper ss(jtp, id, lineNumber);

		n = Time::getMillisecondCounter() - n;
		jp->getScriptEngine()->extendTimeout(n);
	}
	else
	{
		String message;
		message << "Breakpoint in UI Thread";// at " << id << "(Line " << lineNumber << ")";

		reportScriptError(message);
	}
}





void ScriptingApi::Console::blink()
{
#if USE_BACKEND && HISE_USE_NEW_CODE_EDITOR
	if (auto e = getProcessor()->getMainController()->getLastActiveEditor())
	{
        auto i = id;
        auto l = lineNumber;

		MessageManager::callAsync([e, i, l]()
		{
			if (PopupIncludeEditor::matchesId(e, i))
			{
				CommonEditorFunctions::as(e)->sendBlinkMessage(l);
			}
		});
	}
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

void ScriptingApi::Console::assertNoString(var value)
{
	if (value.isString())
	{
		reportScriptError("Assertion failure: " + value.toString());
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
	API_METHOD_WRAPPER_2(Colours, withHue);
	API_METHOD_WRAPPER_2(Colours, withBrightness);
	API_METHOD_WRAPPER_2(Colours, withSaturation);
	API_METHOD_WRAPPER_2(Colours, withMultipliedAlpha);
	API_METHOD_WRAPPER_2(Colours, withMultipliedBrightness);
	API_METHOD_WRAPPER_2(Colours, withMultipliedSaturation);
	API_METHOD_WRAPPER_1(Colours, fromVec4);
	API_METHOD_WRAPPER_1(Colours, toVec4);
	API_METHOD_WRAPPER_3(Colours, mix);
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

	ADD_INLINEABLE_API_METHOD_2(withAlpha);
	ADD_INLINEABLE_API_METHOD_2(withHue);
	ADD_INLINEABLE_API_METHOD_2(withBrightness);
	ADD_INLINEABLE_API_METHOD_2(withSaturation);
	ADD_INLINEABLE_API_METHOD_2(withMultipliedAlpha);
	ADD_INLINEABLE_API_METHOD_2(withMultipliedBrightness);
	ADD_INLINEABLE_API_METHOD_2(withMultipliedSaturation);
	ADD_INLINEABLE_API_METHOD_3(mix);
	ADD_INLINEABLE_API_METHOD_1(toVec4);
	ADD_INLINEABLE_API_METHOD_1(fromVec4);
}

int ScriptingApi::Colours::withAlpha(var colour, float alpha)
{
	auto c = Content::Helpers::getCleanedObjectColour(colour);
	return (int)c.withAlpha(jlimit(0.0f, 1.0f, alpha)).getARGB();
}

int ScriptingApi::Colours::withHue(var colour, float hue)
{
	auto c = Content::Helpers::getCleanedObjectColour(colour);
	return (int)c.withHue(jlimit(0.0f, 1.0f, hue)).getARGB();
}

int ScriptingApi::Colours::withSaturation(var colour, float saturation)
{
	auto c = Content::Helpers::getCleanedObjectColour(colour);
	return (int)c.withSaturation(jlimit(0.0f, 1.0f, saturation)).getARGB();
}

int ScriptingApi::Colours::withBrightness(var colour, float brightness)
{
	auto c = Content::Helpers::getCleanedObjectColour(colour);
	return (int)c.withBrightness(jlimit(0.0f, 1.0f, brightness)).getARGB();
}

int ScriptingApi::Colours::withMultipliedAlpha(var colour, float factor)
{
	auto c = Content::Helpers::getCleanedObjectColour(colour);
	return (int)c.withMultipliedAlpha(jmax(0.0f, factor)).getARGB();
}

int ScriptingApi::Colours::withMultipliedSaturation(var colour, float factor)
{
	auto c = Content::Helpers::getCleanedObjectColour(colour);
	return (int)c.withMultipliedSaturation(jmax(0.0f, factor)).getARGB();
}

int ScriptingApi::Colours::withMultipliedBrightness(var colour, float factor)
{
	auto c = Content::Helpers::getCleanedObjectColour(colour);
	return (int)c.withMultipliedBrightness(jmax(0.0f, factor)).getARGB();
}

var ScriptingApi::Colours::toVec4(var colour)
{
	auto c = Content::Helpers::getCleanedObjectColour(colour);

	Array<var> v4;
	v4.add(c.getFloatRed());
	v4.add(c.getFloatGreen());
	v4.add(c.getFloatBlue());
	v4.add(c.getFloatAlpha());

	return v4;
}

int ScriptingApi::Colours::fromVec4(var vec4)
{
	if (vec4.isArray() && vec4.size() == 4)
	{
		auto r = (uint8)roundToInt((float)vec4[0] * 255.0f);
		auto g = (uint8)roundToInt((float)vec4[1] * 255.0f);
		auto b = (uint8)roundToInt((float)vec4[2] * 255.0f);
		auto a = (uint8)roundToInt((float)vec4[3] * 255.0f);

		return Colour(r, g, b, a).getARGB();
	}

	return 0;
}

int ScriptingApi::Colours::mix(var colour1, var colour2, float alpha)
{
	auto c1 = Content::Helpers::getCleanedObjectColour(colour1);
	auto c2 = Content::Helpers::getCleanedObjectColour(colour2);

	return c1.interpolatedWith(c2, alpha).getARGB();
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
	API_METHOD_WRAPPER_1(FileSystem, descriptionOfSizeInBytes);
	API_METHOD_WRAPPER_1(FileSystem, fromAbsolutePath);
	API_METHOD_WRAPPER_2(FileSystem, fromReferenceString);
	API_VOID_METHOD_WRAPPER_4(FileSystem, browse);
	API_VOID_METHOD_WRAPPER_2(FileSystem, browseForDirectory);
	API_METHOD_WRAPPER_1(FileSystem, getBytesFreeOnVolume);
    API_METHOD_WRAPPER_2(FileSystem, encryptWithRSA);
    API_METHOD_WRAPPER_0(FileSystem, findFileSystemRoots);
    API_METHOD_WRAPPER_2(FileSystem, decryptWithRSA);
	API_VOID_METHOD_WRAPPER_0(FileSystem, loadExampleAssets);
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
	addConstant("Applications", (int)Applications);
	addConstant("Temp", (int)Temp);

	ADD_API_METHOD_1(getFolder);
	ADD_API_METHOD_3(findFiles);
	ADD_API_METHOD_0(getSystemId);
	ADD_API_METHOD_1(descriptionOfSizeInBytes);
	ADD_API_METHOD_4(browse);
	ADD_API_METHOD_2(browseForDirectory);
	ADD_API_METHOD_1(fromAbsolutePath);
	ADD_API_METHOD_2(fromReferenceString);
	ADD_API_METHOD_1(getBytesFreeOnVolume);
    ADD_API_METHOD_2(encryptWithRSA);
    ADD_API_METHOD_2(decryptWithRSA);
    ADD_API_METHOD_0(findFileSystemRoots);
	ADD_API_METHOD_0(loadExampleAssets);
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

var ScriptingApi::FileSystem::fromAbsolutePath(String path)
{
	if (File::isAbsolutePath(path))
		return var(new ScriptingObjects::ScriptFile(getScriptProcessor(), File(path)));

	return var();
}

juce::var ScriptingApi::FileSystem::fromReferenceString(String referenceStringOrFullPath, var locationType)
{
	auto sub = getSubdirectory(locationType);

	PoolReference ref(getScriptProcessor()->getMainController_(), referenceStringOrFullPath, sub);

	// also return a file object for missing files...
	if(ref.isAbsoluteFile())
	{
		return var(new ScriptingObjects::ScriptFile(getScriptProcessor(), File(referenceStringOrFullPath)));
	}
	
	if ((ref.isValid()) && !ref.isEmbeddedReference())
	{
		auto f = ref.getFile();
		return var(new ScriptingObjects::ScriptFile(getScriptProcessor(), File(f)));
	}

	return {};
}

var ScriptingApi::FileSystem::findFiles(var directory, String wildcard, bool recursive)
{
	Array<var> l;

	if (auto root = dynamic_cast<ScriptingObjects::ScriptFile*>(directory.getObject()))
	{
		if (root->isDirectory())
		{
			HiseJavascriptEngine::TimeoutExtender ts(dynamic_cast<JavascriptProcessor*>(getScriptProcessor())->getScriptEngine());

			auto list = root->f.findChildFiles(File::findFilesAndDirectories | File::ignoreHiddenFiles, recursive, wildcard);

			for (auto sf : list)
			{
				if(sf.getFileName() == ".DS_Store")
					continue;

				l.add(new ScriptingObjects::ScriptFile(p, sf));
			}
		}
	}

	return l;
}

String ScriptingApi::FileSystem::descriptionOfSizeInBytes(int64 bytes)
{
	return File::descriptionOfSizeInBytes(bytes);
};

void ScriptingApi::FileSystem::browse(var startFolder, bool forSaving, String wildcard, var callback)
{
	File f;

	if (startFolder.isInt())
		f = getFile((SpecialLocations)(int)startFolder);
	else if (auto sf = dynamic_cast<ScriptingObjects::ScriptFile*>(startFolder.getObject()))
		f = sf->f;

	browseInternally(f, forSaving, false, wildcard, callback);
}

void ScriptingApi::FileSystem::browseForDirectory(var startFolder, var callback)
{
	File f;

	if (startFolder.isInt())
		f = getFile((SpecialLocations)(int)startFolder);
	else if (auto sf = dynamic_cast<ScriptingObjects::ScriptFile*>(startFolder.getObject()))
		f = sf->f;

	browseInternally(f, false, true, "", callback);
}

String ScriptingApi::FileSystem::getSystemId()
{
	return OnlineUnlockStatus::MachineIDUtilities::getLocalMachineIDs()[0];
}

var ScriptingApi::FileSystem::findFileSystemRoots()
{
    Array<File> roots;
    File::findFileSystemRoots(roots);
    
    Array<var> entries;
    
    for(auto r: roots)
        entries.add(var(new ScriptingObjects::ScriptFile(getScriptProcessor(), r)));
    
    return var(entries);
}

int64 ScriptingApi::FileSystem::getBytesFreeOnVolume(var folder)
{
	File f;

	if (folder.isInt())
		f = getFile((SpecialLocations)(int)folder);
	else if (auto sf = dynamic_cast<ScriptingObjects::ScriptFile*>(folder.getObject()))
		f = sf->f;

	auto numBytes = f.getBytesFreeOnVolume();

	return numBytes;
}

void ScriptingApi::FileSystem::browseInternally(File f, bool forSaving, bool isDirectory, String wildcard, var callback)
{
	auto p_ = p;

	WeakCallbackHolder wc(p_, this, callback, 1);
	wc.setHighPriority();
	wc.incRefCount();

	auto cb = [forSaving, f, wildcard, isDirectory, wc, p_]() mutable
	{
		String title;

		if (isDirectory)
			title = "Browse for directory";
		else
			title = !forSaving ? "Open file" : "Save file";

		FileChooser fc(title, f, wildcard);

		var a;

		if (isDirectory)
		{
			if (fc.browseForDirectory())
				a = var(new ScriptingObjects::ScriptFile(p_, fc.getResult()));
		}
		else
		{
			if (forSaving && fc.browseForFileToSave(true))
				a = var(new ScriptingObjects::ScriptFile(p_, fc.getResult()));
			if (!forSaving && fc.browseForFileToOpen())
				a = var(new ScriptingObjects::ScriptFile(p_, fc.getResult()));
		}

		if (a.isObject())
		{
			wc.call(&a, 1);
		}
	};

	MessageManager::callAsync(cb);
}


String ScriptingApi::FileSystem::encryptWithRSA(const String& dataToEncrypt, const String& privateKey)
{
    juce::RSAKey key(privateKey);
    
    MemoryOutputStream text;
    text << dataToEncrypt;

    BigInteger val;
    val.loadFromMemoryBlock (text.getMemoryBlock());

    key.applyToValue (val);

    return val.toString(16);
}


String ScriptingApi::FileSystem::decryptWithRSA(const String& dataToDecrypt, const String& publicKey)
{
    BigInteger val;
    val.parseString (dataToDecrypt, 16);

    RSAKey key (publicKey);
    
    if(key.isValid())
    {
        key.applyToValue (val);

        auto mb = val.toMemoryBlock();

        if (CharPointer_UTF8::isValidString (static_cast<const char*> (mb.getData()), (int) mb.getSize()))
            return mb.toString();
    }
    
    return {};
}

void ScriptingApi::FileSystem::loadExampleAssets()
{
#if USE_BACKEND
	auto am = dynamic_cast<BackendProcessor*>(getMainController())->getAssetManager();
	am->initialise();
#endif
}


juce::File ScriptingApi::FileSystem::getFile(SpecialLocations l)
{
	File f;

	switch (l)
	{
	case Samples:
	
		if(FullInstrumentExpansion::isEnabled(getMainController()))
		{
		  if (auto e = getMainController()->getExpansionHandler().getCurrentExpansion())
		    f = e->getSubDirectory(FileHandlerBase::Samples);
		}
		else 
		{
			f = getMainController()->getCurrentFileHandler().getSubDirectory(FileHandlerBase::Samples);	
		}
		
		break;
	case Expansions: return getMainController()->getExpansionHandler().getExpansionFolder();
#if USE_BACKEND
	case AppData:
	{
		f = ProjectHandler::getAppDataRoot(getMainController());

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
	case Applications: f = File::getSpecialLocation(File::globalApplicationsDirectory); break;
	case Temp: f = File::getSpecialLocation(File::tempDirectory); break;
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

hise::FileHandlerBase::SubDirectories ScriptingApi::FileSystem::getSubdirectory(var locationType)
{
	if (locationType.isInt())
	{
		auto t = (SpecialLocations)(int)locationType;

		switch (t)
		{
		case SpecialLocations::AudioFiles:  return FileHandlerBase::AudioFiles;
		case SpecialLocations::Samples:		return FileHandlerBase::Samples;
		case SpecialLocations::UserPresets: return FileHandlerBase::UserPresets;
		default: 
			reportScriptError(String("\"") + getConstantName((int)locationType) + String("\" is not a valid locationType"));
			RETURN_IF_NO_THROW(FileHandlerBase::numSubDirectories);
		}
	}
	else
	{
		reportScriptError("You need to pass in a constant from FileSystem (eg. FileSystem.AudioFiles) as locationType)");

		RETURN_IF_NO_THROW(FileHandlerBase::SubDirectories::numSubDirectories);
	}
}

struct ScriptingApi::Threads::Wrapper
{
    API_METHOD_WRAPPER_0(ScriptingApi::Threads, getCurrentThread);
    API_METHOD_WRAPPER_0(ScriptingApi::Threads, isAudioRunning);
	API_METHOD_WRAPPER_0(ScriptingApi::Threads, isCurrentlyExporting);
    API_METHOD_WRAPPER_1(ScriptingApi::Threads, isLockedByCurrentThread);
    API_METHOD_WRAPPER_1(ScriptingApi::Threads, getLockerThread);
    API_METHOD_WRAPPER_1(ScriptingApi::Threads, isLocked);
	API_METHOD_WRAPPER_1(ScriptingApi::Threads, toString);
	API_METHOD_WRAPPER_0(ScriptingApi::Threads, getCurrentThreadName);
    API_METHOD_WRAPPER_1(ScriptingApi::Threads, killVoicesAndCall);
};

ScriptingApi::Threads::Threads(ProcessorWithScriptingContent* p):
	ApiClass(6),
	ScriptingObject(p)
{
	addConstant("Audio", (int)LockHelpers::Type::AudioLock);
	addConstant("Scripting", (int)LockHelpers::Type::ScriptLock);
	addConstant("Loading", (int)LockHelpers::Type::SampleLock);
	addConstant("UI", (int)LockHelpers::Type::MessageLock);
	addConstant("Unknown", (int)LockHelpers::Type::numLockTypes);
	addConstant("Free", (int)LockHelpers::Type::unused);

	ADD_API_METHOD_0(getCurrentThread);
    ADD_API_METHOD_0(isAudioRunning);
	ADD_API_METHOD_0(isCurrentlyExporting);
    ADD_API_METHOD_1(isLockedByCurrentThread);
    ADD_API_METHOD_1(getLockerThread);
    ADD_API_METHOD_1(isLocked);
    ADD_API_METHOD_1(killVoicesAndCall);
	ADD_API_METHOD_1(toString);
	ADD_API_METHOD_0(getCurrentThreadName);
}

int ScriptingApi::Threads::getCurrentThread() const
{
	auto s = getScriptProcessor()->getMainController_()->getKillStateHandler().getCurrentThread();

	MainController::KillStateHandler::getLockTypeForThread(s);

	switch(s)
	{
	case TargetThreadId::MessageThread:		  return (int)LockId::MessageLock;
	case TargetThreadId::SampleLoadingThread: return (int)LockId::SampleLock;
	case TargetThreadId::AudioThread:		  return (int)LockId::AudioLock;
	case TargetThreadId::AudioExportThread:	  return (int)LockId::AudioLock;
	case TargetThreadId::ScriptingThread:	  return (int)LockId::ScriptLock;
	case TargetThreadId::UnknownThread:		  return (int)LockId::numLockTypes;
	case TargetThreadId::Free:				  return (int)LockId::unused;
	default:								  return -1;
	}
}

bool ScriptingApi::Threads::isAudioRunning() const
{
	return getScriptProcessor()->getMainController_()->getKillStateHandler().isAudioRunning();
}

bool ScriptingApi::Threads::isCurrentlyExporting() const
{
	return getKillStateHandler().isCurrentlyExporting();
}

bool ScriptingApi::Threads::isLockedByCurrentThread(int thread) const
{
	return getKillStateHandler().currentThreadHoldsLock(getAsLockId(thread));
}

int ScriptingApi::Threads::getLockerThread(int threadThatIsLocked) const
{
	return (int)getKillStateHandler().getLockTypeForThread(getAsThreadId(threadThatIsLocked));
}

bool ScriptingApi::Threads::isLocked(int thread) const
{
	auto t = (LockId)getLockerThread(thread);
	return t != LockId::unused;
}

String ScriptingApi::Threads::toString(int thread) const
{
	switch(getAsLockId(thread))
	{
	case LockHelpers::Type::MessageLock:	return "Message Thread";
	case LockHelpers::Type::ScriptLock:		return "Scripting Thread";
	case LockHelpers::Type::SampleLock:		return "Sample Thread";
	case LockHelpers::Type::IteratorLock:	return "Iterator Thread (never used)";
	case LockHelpers::Type::AudioLock:		return "Audio Thread";
	case LockHelpers::Type::numLockTypes:	return "Unknown Thread";
	case LockHelpers::Type::unused:			return "Free (unlocked)";
	default:								return "Unknown Thread";
	}
}

bool ScriptingApi::Threads::killVoicesAndCall(const var& functionToExecute)
{
	WeakCallbackHolder wc(getScriptProcessor(), this, functionToExecute, 0);

	return getKillStateHandler().killVoicesAndCall(dynamic_cast<Processor*>(getScriptProcessor()), [wc](Processor* p)
	{
		WeakCallbackHolder copy = std::move(wc);

		if(copy)
		{
			LockHelpers::SafeLock sl(p->getMainController(), LockId::ScriptLock);
			auto ok = copy.callSync(nullptr, 0, nullptr);

			if(!ok.wasOk())
			{
				debugError(p, ok.getErrorMessage());
			}

			return SafeFunctionCall::OK;
		}
		else
		{
			return SafeFunctionCall::nullPointerCall;
		}
	}, TargetThreadId::SampleLoadingThread);
}

ScriptingApi::Threads::TargetThreadId ScriptingApi::Threads::getAsThreadId(int x)
{
	return MainController::KillStateHandler::getThreadForLockType(getAsLockId(x));
}

ScriptingApi::Threads::LockId ScriptingApi::Threads::getAsLockId(int x)
{
	return (LockId)x;
}

MainController::KillStateHandler& ScriptingApi::Threads::getKillStateHandler()
{ return getScriptProcessor()->getMainController_()->getKillStateHandler(); }

const MainController::KillStateHandler& ScriptingApi::Threads::getKillStateHandler() const
{ return getScriptProcessor()->getMainController_()->getKillStateHandler(); }

struct ScriptingApi::Server::Wrapper
{
	API_VOID_METHOD_WRAPPER_1(Server, setBaseURL);
	API_VOID_METHOD_WRAPPER_3(Server, callWithPOST);
	API_VOID_METHOD_WRAPPER_3(Server, callWithGET);
	API_METHOD_WRAPPER_4(Server, downloadFile);
	API_VOID_METHOD_WRAPPER_1(Server, setHttpHeader);
    API_VOID_METHOD_WRAPPER_1(Server, setEnforceTrailingSlash);
	API_METHOD_WRAPPER_0(Server, getPendingDownloads);
	API_METHOD_WRAPPER_0(Server, getPendingCalls);
	API_METHOD_WRAPPER_0(Server, isOnline);
	API_VOID_METHOD_WRAPPER_1(Server, setNumAllowedDownloads);
	API_VOID_METHOD_WRAPPER_0(Server, cleanFinishedDownloads);
	API_VOID_METHOD_WRAPPER_1(Server, setServerCallback);
    API_VOID_METHOD_WRAPPER_1(Server, setTimeoutMessageString);
    API_METHOD_WRAPPER_0(Server, resendLastCall);
	API_METHOD_WRAPPER_1(Server, isEmailAddress);
};

ScriptingApi::Server::Server(JavascriptProcessor* jp_):
	ApiClass(4),
	ScriptingObject(dynamic_cast<ProcessorWithScriptingContent*>(jp_)),
	jp(jp_),
	globalServer(*getScriptProcessor()->getMainController_()->getJavascriptThreadPool().getGlobalServer()),
	serverCallback(getScriptProcessor(), this, {}, 1)
{
	globalServer.addListener(this);

	addConstant("StatusNoConnection", StatusNoConnection);
	addConstant("StatusOK", StatusOK);
	addConstant("StatusNotFound", StatusNotFound);
	addConstant("StatusServerError", StatusServerError);
	addConstant("StatusAuthenticationFail", StatusAuthenticationFail);

	ADD_API_METHOD_1(setBaseURL);
	ADD_TYPED_API_METHOD_3(callWithPOST, VarTypeChecker::String, VarTypeChecker::JSON, VarTypeChecker::Function);
	ADD_TYPED_API_METHOD_3(callWithGET, VarTypeChecker::String, VarTypeChecker::JSON, VarTypeChecker::Function);
	ADD_TYPED_API_METHOD_1(setHttpHeader, VarTypeChecker::String);
    ADD_TYPED_API_METHOD_4(downloadFile, VarTypeChecker::String, VarTypeChecker::JSON, VarTypeChecker::ScriptObject, VarTypeChecker::Function);
	ADD_API_METHOD_0(getPendingDownloads);
	ADD_API_METHOD_0(getPendingCalls);
	ADD_API_METHOD_0(isOnline);
    ADD_API_METHOD_0(resendLastCall);
	ADD_API_METHOD_1(setNumAllowedDownloads);
	ADD_API_METHOD_1(setServerCallback);
	ADD_API_METHOD_0(cleanFinishedDownloads);
	ADD_API_METHOD_1(isEmailAddress);
    ADD_API_METHOD_1(setTimeoutMessageString);
    ADD_API_METHOD_1(setEnforceTrailingSlash);
}

void ScriptingApi::Server::setBaseURL(String url)
{
	globalServer.setBaseURL(url);
}

void ScriptingApi::Server::setTimeoutMessageString(String timeoutMessage)
{
    globalServer.setTimeoutMessageString(timeoutMessage);
}

void ScriptingApi::Server::callWithGET(String subURL, var parameters, var callback)
{
	if (HiseJavascriptEngine::isJavascriptFunction(callback))
	{
		GlobalServer::PendingCallback::Ptr p = new GlobalServer::PendingCallback(getScriptProcessor(), callback);
		p->url = getWithParameters(subURL, parameters);
		p->isPost = false;
		globalServer.addPendingCallback(p);
	}
}

void ScriptingApi::Server::setEnforceTrailingSlash(bool shouldAdd)
{
    globalServer.addTrailingSlashes = shouldAdd;
}

void ScriptingApi::Server::callWithPOST(String subURL, var parameters, var callback)
{
	if (HiseJavascriptEngine::isJavascriptFunction(callback))
	{
		GlobalServer::PendingCallback::Ptr p = new GlobalServer::PendingCallback(getScriptProcessor(), callback);
        
        const bool isNotAFile = !subURL.containsChar('.');
        const bool trailingSlashMissing = !subURL.endsWithChar('/');
        
        if(isNotAFile && trailingSlashMissing && globalServer.addTrailingSlashes)
        {
            // We need to append a slash in order to prevent redirecting to a GET call
            subURL << '/';
        }
        
		p->url = getWithParameters(subURL, parameters);
		p->isPost = true;
		globalServer.addPendingCallback(p);
	}
}

void ScriptingApi::Server::setHttpHeader(String newHeader)
{
	globalServer.setHttpHeader(newHeader);
}

bool ScriptingApi::Server::resendLastCall()
{
    if(isOnline())
    {
        return globalServer.resendLastCallback();
    }
    
    return false;
}

var ScriptingApi::Server::downloadFile(String subURL, var parameters, var targetFile, var callback)
{
	if (auto sf = dynamic_cast<ScriptingObjects::ScriptFile*>(targetFile.getObject()))
	{
		if (subURL.contains("?") && parameters.getDynamicObject() != nullptr && parameters.getDynamicObject()->getProperties().isEmpty())
		{
			auto parameterObject = new DynamicObject();
			auto realSubURL = subURL.upToFirstOccurrenceOf("?", false, false);
			auto parameterString = subURL.fromFirstOccurrenceOf("?", false, false);
			auto parameterObjects = StringArray::fromTokens(parameterString, "&", "");

			for (auto po : parameterObjects)
			{
				auto key = po.upToFirstOccurrenceOf("=", false, false);
				auto value = po.fromFirstOccurrenceOf("=", false, false);

				if (!key.isEmpty() && !value.isEmpty())
				{
					parameterObject->setProperty(Identifier(key), var(value));
				}
			}

			parameters = var(parameterObject);
			subURL = realSubURL;
		}

		if (sf->f.isDirectory())
		{
			reportScriptError("target file is a directory");
			return var();
		}

		auto urlToUse = getWithParameters(subURL, parameters);

		if(urlToUse.isWellFormed())
		{
			ScriptingObjects::ScriptDownloadObject::Ptr p = new ScriptingObjects::ScriptDownloadObject(getScriptProcessor(), urlToUse, globalServer.getExtraHeader(), sf->f, callback);
			return globalServer.addDownload(p);
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
	return globalServer.getPendingDownloads();
}

var ScriptingApi::Server::getPendingCalls()
{
	return globalServer.getPendingCallbacks();
}

void ScriptingApi::Server::setNumAllowedDownloads(int maxNumberOfParallelDownloads)
{
	globalServer.setNumAllowedDownloads(maxNumberOfParallelDownloads);
}

bool ScriptingApi::Server::isOnline()
{
	const char* urlsToTry[] = { "http://google.com/generate_204", "https://amazon.com", nullptr };

	for (const char** url = urlsToTry; *url != nullptr; ++url)
	{
		URL u(*url);

		auto ms = Time::getMillisecondCounter();
		std::unique_ptr<InputStream> in(u.createInputStream(false, nullptr, nullptr, String(), HISE_SCRIPT_SERVER_TIMEOUT, nullptr));
		dynamic_cast<JavascriptProcessor*>(getScriptProcessor())->getScriptEngine()->extendTimeout(Time::getMillisecondCounter() - ms);

		if (in != nullptr)
			return true;
	}

	return false;
}

void ScriptingApi::Server::cleanFinishedDownloads()
{
	globalServer.cleanFinishedDownloads();
}

void ScriptingApi::Server::setServerCallback(var callback)
{
	serverCallback = WeakCallbackHolder(getScriptProcessor(), this, callback, 1);
	serverCallback.incRefCount();
}

bool ScriptingApi::Server::isEmailAddress(String email)
{
	URL u("");
	return u.isProbablyAnEmailAddress(email);
}

ScriptingApi::TransportHandler::Callback::Callback(TransportHandler* p, const String& name, const var& f, bool sync, int numArgs_) :
	callback(p->getScriptProcessor(), p, f, numArgs_),
	jp(dynamic_cast<JavascriptProcessor*>(p->getScriptProcessor())),
	synchronous(sync),
	th(p),
	numArgs(numArgs_)
{
	callback.addAsSource(p, name);

	if (synchronous)
	{
		auto fObj = dynamic_cast<HiseJavascriptEngine::RootObject::InlineFunction::Object*>(f.getObject());

		if (fObj == nullptr)
			throw String("Must use inline functions for synchronous callback");

		if (fObj->parameterNames.size() != numArgs)
		{
			throw String("Parameter amount mismatch for callback. Expected " + String(numArgs));
		}
	}

	setHandler(th->getMainController()->getGlobalUIUpdater());
	addPooledChangeListener(th);

	callback.incRefCount();

	if(!sync)
		callback.setHighPriority();
	
}

void ScriptingApi::TransportHandler::Callback::call(var arg1, var arg2, var arg3, bool forceSync)
{
	args[0] = arg1;
	args[1] = arg2;
	args[2] = arg3;

	if (synchronous || forceSync)
		callSync();
	else
		sendPooledChangeMessage();
}

void ScriptingApi::TransportHandler::Callback::callAsync()
{
	callback.call(args, numArgs);
}

bool ScriptingApi::TransportHandler::Callback::matches(const var& f) const
{
	return callback.matches(f);
}

void ScriptingApi::TransportHandler::Callback::callSync()
{
	auto ok = callback.callSync(args, numArgs);

#if USE_BACKEND
	if(!ok.wasOk())
		debugError(dynamic_cast<Processor*>(jp), ok.getErrorMessage());
#endif
}

struct ScriptingApi::TransportHandler::Wrapper
{
    API_VOID_METHOD_WRAPPER_1(TransportHandler, stopInternalClockOnExternalStop);
	API_VOID_METHOD_WRAPPER_2(TransportHandler, setOnTempoChange);
	API_VOID_METHOD_WRAPPER_2(TransportHandler, setOnBeatChange);
	API_VOID_METHOD_WRAPPER_2(TransportHandler, setOnGridChange);
	API_VOID_METHOD_WRAPPER_2(TransportHandler, setOnSignatureChange);
	API_VOID_METHOD_WRAPPER_2(TransportHandler, setOnTransportChange);
	API_VOID_METHOD_WRAPPER_1(TransportHandler, setOnBypass);
	API_VOID_METHOD_WRAPPER_1(TransportHandler, setSyncMode);
	API_VOID_METHOD_WRAPPER_2(TransportHandler, setEnableGrid);
	API_VOID_METHOD_WRAPPER_1(TransportHandler, startInternalClock);
	API_VOID_METHOD_WRAPPER_1(TransportHandler, stopInternalClock);
	API_VOID_METHOD_WRAPPER_0(TransportHandler, sendGridSyncOnNextCallback);
	API_VOID_METHOD_WRAPPER_1(TransportHandler, setLinkBpmToSyncMode);
	API_METHOD_WRAPPER_0(TransportHandler, isNonRealtime);
};

ScriptingApi::TransportHandler::TransportHandler(ProcessorWithScriptingContent* sp) :
	ConstScriptingObject(sp, (int)MasterClock::SyncModes::numSyncModes),
	ControlledObject(sp->getMainController_())
{
	addConstant("Inactive", (int)MasterClock::SyncModes::Inactive);
	addConstant("ExternalOnly", (int)MasterClock::SyncModes::ExternalOnly);
	addConstant("InternalOnly", (int)MasterClock::SyncModes::InternalOnly);
	addConstant("PreferInternal", (int)MasterClock::SyncModes::PreferInternal);
	addConstant("PreferExternal", (int)MasterClock::SyncModes::PreferExternal);
	addConstant("SyncInternal", (int)MasterClock::SyncModes::SyncInternal);

	getMainController()->addTempoListener(this);

	ADD_TYPED_API_METHOD_2(setOnTempoChange, VarTypeChecker::Number, VarTypeChecker::Function);
	ADD_TYPED_API_METHOD_2(setOnBeatChange, VarTypeChecker::Number, VarTypeChecker::Function);
	ADD_TYPED_API_METHOD_2(setOnGridChange, VarTypeChecker::Number, VarTypeChecker::Function);
	ADD_TYPED_API_METHOD_2(setOnSignatureChange, VarTypeChecker::Number, VarTypeChecker::Function);
	ADD_TYPED_API_METHOD_2(setOnTransportChange, VarTypeChecker::Number, VarTypeChecker::Function);
	ADD_TYPED_API_METHOD_1(setOnBypass, VarTypeChecker::Function);
	ADD_API_METHOD_1(setSyncMode);
	ADD_API_METHOD_1(startInternalClock);
	ADD_API_METHOD_1(stopInternalClock);
	ADD_API_METHOD_2(setEnableGrid);
	ADD_API_METHOD_0(sendGridSyncOnNextCallback);
    ADD_API_METHOD_1(stopInternalClockOnExternalStop);
	ADD_API_METHOD_1(setLinkBpmToSyncMode);
	ADD_API_METHOD_0(isNonRealtime);
}

ScriptingApi::TransportHandler::~TransportHandler()
{
	getMainController()->getPluginBypassHandler().listeners.removeListener(*this);
	getMainController()->removeTempoListener(this);
	getMainController()->removeMusicalUpdateListener(this);
}

void ScriptingApi::TransportHandler::setOnTempoChange(var sync, var f)
{
    auto isSync = ApiHelpers::isSynchronous(sync);
    
	if (isSync)
	{
		clearIf(tempoChangeCallbackAsync, f);

		tempoChangeCallback = new Callback(this, "onTempoChange", f, isSync, 1);
		tempoChangeCallback->call(bpm, {}, {}, true);
	}
	else
	{
		clearIf(tempoChangeCallback, f);

		tempoChangeCallbackAsync = new Callback(this, "onTempoChange", f, isSync, 1);
		tempoChangeCallbackAsync->call(bpm, {}, {}, true);
	}
}

void ScriptingApi::TransportHandler::setOnTransportChange(var sync, var f)
{
    auto isSync = ApiHelpers::isSynchronous(sync);
    
	if (isSync)
	{
		clearIf(tempoChangeCallbackAsync, f);

		transportChangeCallback = new Callback(this, "onTransportChange", f, isSync, 1);
		transportChangeCallback->call(play, {}, {}, true);
	}
	else
	{
		clearIf(transportChangeCallback, f);

		transportChangeCallbackAsync = new Callback(this, "onTransportChange", f, isSync, 1);
		transportChangeCallbackAsync->call(play, {}, {}, true);
	}
	
}

void ScriptingApi::TransportHandler::stopInternalClockOnExternalStop(bool shouldStop)
{
    getMainController()->getMasterClock().setStopInternalClockOnExternalStop(shouldStop);
}

void ScriptingApi::TransportHandler::setOnSignatureChange(var sync, var f)
{
    auto isSync = ApiHelpers::isSynchronous(sync);
    
	if (isSync)
	{
		clearIf(timeSignatureCallbackAsync, f);

		timeSignatureCallback = new Callback(this, "onTimeSignatureChange", f, isSync, 2);
		timeSignatureCallback->call(nom, denom, {}, true);
	}
	else
	{
		clearIf(timeSignatureCallback, f);

		timeSignatureCallbackAsync = new Callback(this, "onTimeSignatureChange", f, isSync, 2);
		timeSignatureCallbackAsync->call(nom, denom, {}, true);
	}
}

void ScriptingApi::TransportHandler::tempoChanged(double newTempo)
{
	bpm = newTempo;

	if (tempoChangeCallback != nullptr)
		tempoChangeCallback->call(newTempo);

	if (tempoChangeCallbackAsync != nullptr)
		tempoChangeCallbackAsync->call(newTempo);
}



void ScriptingApi::TransportHandler::onTransportChange(bool isPlaying, double /*ppqPosition*/)
{
	play = isPlaying;

	if (transportChangeCallback != nullptr)
		transportChangeCallback->call(isPlaying);

	if (transportChangeCallbackAsync != nullptr)
		transportChangeCallbackAsync->call(isPlaying);
}

void ScriptingApi::TransportHandler::onBeatChange(int newBeat, bool isNewBar)
{
	beat = newBeat;
	newBar = isNewBar;

	if (beatCallback != nullptr)
		beatCallback->call(newBeat, newBar);

	if (beatCallbackAsync != nullptr)
		beatCallbackAsync->call(newBeat, newBar);
}

void ScriptingApi::TransportHandler::onSignatureChange(int newNominator, int numDenominator)
{
	nom = newNominator;
	denom = numDenominator;

	if (timeSignatureCallback != nullptr)
		timeSignatureCallback->call(newNominator, numDenominator);

	if (timeSignatureCallbackAsync != nullptr)
		timeSignatureCallbackAsync->call(newNominator, numDenominator);
}

void ScriptingApi::TransportHandler::onGridChange(int gridIndex_, uint16 timestamp, bool firstGridInPlayback_)
{
	gridIndex = gridIndex_;
	gridTimestamp = timestamp;
	firstGridInPlayback = firstGridInPlayback_;

	if (gridCallback != nullptr)
		gridCallback->call(gridIndex, gridTimestamp, firstGridInPlayback);

	if (gridCallbackAsync != nullptr)
		gridCallbackAsync->call(gridIndex, gridTimestamp, firstGridInPlayback);
}

void ScriptingApi::TransportHandler::setOnBeatChange(var sync, var f)
{
    auto isSync = ApiHelpers::isSynchronous(sync);
    
	if (f.isUndefined())
		getMainController()->removeMusicalUpdateListener(this);
	else
	{
		getMainController()->addMusicalUpdateListener(this);

		if (isSync)
		{
			clearIf(beatCallbackAsync, f);
			beatCallback = new Callback(this, "onBeatChange", f, isSync, 2);
		}
		else
		{
			clearIf(beatCallback, f);
			beatCallbackAsync = new Callback(this, "onBeatChange", f, isSync, 2);
		}
	}
}

void ScriptingApi::TransportHandler::setOnGridChange(var sync, var f)
{
    auto isSync = ApiHelpers::isSynchronous(sync);
    
	if (f.isUndefined())
		getMainController()->removeMusicalUpdateListener(this);
	else
	{
		getMainController()->addMusicalUpdateListener(this);

		if (isSync)
		{
			clearIf(gridCallbackAsync, f);
			gridCallback = new Callback(this, "onGridChange", f, isSync, 3);
		}
		else
		{
			clearIf(gridCallback, f);
			gridCallbackAsync = new Callback(this, "onGridChange", f, isSync, 3);
		}
	}
}

void ScriptingApi::TransportHandler::setOnBypass(var f)
{
	bypassCallback = new Callback(this, "onGridChange", f, false, 1);

	getMainController()->getPluginBypassHandler().listeners.addListener(*this, TransportHandler::onBypassUpdate, true);
}

void ScriptingApi::TransportHandler::setEnableGrid(bool shouldBeEnabled, int tempoFactor)
{
	if (isPositiveAndBelow(tempoFactor, (int)TempoSyncer::numTempos))
	{
		auto t = (TempoSyncer::Tempo)tempoFactor;
		getMainController()->getMasterClock().setClockGrid(shouldBeEnabled, t);
	}
	else
	{
		reportScriptError("Illegal tempo value. Use 1-18");
	}
}

void ScriptingApi::TransportHandler::startInternalClock(int timestamp)
{
	auto& clock = getMainController()->getMasterClock();

	if(clock.changeState(timestamp, true, true))
	{
		if(getMainController()->isInsideAudioRendering())
		{
			auto gi = clock.processAndCheckGrid(getMainController()->getBufferSizeForCurrentBlock(), {});
			auto ph = clock.createInternalPlayHead();
			getMainController()->handleTransportCallbacks(ph, gi);
		}
	}
}

void ScriptingApi::TransportHandler::stopInternalClock(int timestamp)
{
	auto& clock = getMainController()->getMasterClock();

	if(clock.changeState(timestamp, true, false))
	{
		if(getMainController()->isInsideAudioRendering())
		{
			auto gi = clock.processAndCheckGrid(getMainController()->getBufferSizeForCurrentBlock(), {});
			auto ph = clock.createInternalPlayHead();
			getMainController()->handleTransportCallbacks(ph, gi);
		}
	}
}

void ScriptingApi::TransportHandler::setSyncMode(int syncMode)
{
	getMainController()->getMasterClock().setSyncMode((MasterClock::SyncModes)syncMode);
}

void ScriptingApi::TransportHandler::sendGridSyncOnNextCallback()
{
	getMainController()->getMasterClock().setNextGridIsFirst();
}

void ScriptingApi::TransportHandler::setLinkBpmToSyncMode(bool shouldPrefer)
{
	getMainController()->getMasterClock().setLinkBpmToSyncMode(shouldPrefer);
}

bool ScriptingApi::TransportHandler::isNonRealtime() const
{
	return getScriptProcessor()->getMainController_()->getSampleManager().isNonRealtime();
}

void ScriptingApi::TransportHandler::onBypassUpdate(TransportHandler& handler, bool state)
{
	if(handler.bypassCallback != nullptr)
		handler.bypassCallback->call(state, {}, {}, true);
}
} // namespace hise
