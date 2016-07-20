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

// MidiList =====================================================================================================================

struct ScriptingObjects::MidiList::Wrapper
{
	API_VOID_METHOD_WRAPPER_1(MidiList, fill);
	API_VOID_METHOD_WRAPPER_0(MidiList, clear);
	API_METHOD_WRAPPER_1(MidiList, getValue);
	API_METHOD_WRAPPER_1(MidiList, getValueAmount);
	API_METHOD_WRAPPER_1(MidiList, getIndex);
	API_METHOD_WRAPPER_0(MidiList, isEmpty);
	API_METHOD_WRAPPER_0(MidiList, getNumSetValues);
	API_VOID_METHOD_WRAPPER_2(MidiList, setValue);
	API_VOID_METHOD_WRAPPER_1(MidiList, restoreFromBase64String);
	API_METHOD_WRAPPER_0(MidiList, getBase64String);
};

ScriptingObjects::MidiList::MidiList(ProcessorWithScriptingContent *p) :
ConstScriptingObject(p, 0)
{
	ADD_API_METHOD_1(fill);
	ADD_API_METHOD_0(clear);
	ADD_API_METHOD_1(getValue);
	ADD_API_METHOD_1(getValueAmount);
	ADD_API_METHOD_1(getIndex);
	ADD_API_METHOD_0(isEmpty);
	ADD_API_METHOD_0(getNumSetValues);
	ADD_API_METHOD_2(setValue);
	ADD_API_METHOD_1(restoreFromBase64String);
	ADD_API_METHOD_0(getBase64String);

	clear();
}

void ScriptingObjects::MidiList::assign(const int index, var newValue)			 { setValue(index, (int)newValue); }
int ScriptingObjects::MidiList::getCachedIndex(const var &indexExpression) const { return (int)indexExpression; }
var ScriptingObjects::MidiList::getAssignedValue(int index) const				 { return getValue(index); }

void ScriptingObjects::MidiList::fill(int valueToFill)
{
	for (int i = 0; i < 128; i++) data[i] = (int16)valueToFill;
	empty = false;
	numValues = 128;
}

void ScriptingObjects::MidiList::clear()
{
	fill(-1);
	empty = true;
	numValues = 0;
}

int ScriptingObjects::MidiList::getValue(int index) const
{
	if (index < 127 && index >= 0) return (int)data[index]; else return -1;
}

int ScriptingObjects::MidiList::getValueAmount(int valueToCheck)
{
	if (empty) return 0;

	int amount = 0;

	for (int i = 0; i < 128; i++)
	{
		if (data[i] == (int16)valueToCheck) amount++;
	}

	return amount;
}

int ScriptingObjects::MidiList::getIndex(int value) const
{
	if (empty) return -1;
	for (int i = 0; i < 128; i++)
	{
		if (data[i] == (int16)value)
		{
			return i;
		}
	}

	return -1;
}

void ScriptingObjects::MidiList::setValue(int index, int value)
{
	if (index >= 0 && index < 128)
	{
		data[index] = (int16)value;

		if (value == -1)
		{
			numValues--;
			if (numValues == 0) empty = true;
		}
		else
		{
			numValues++;
			empty = false;
		}
	}
}

String ScriptingObjects::MidiList::getBase64String() const
{
	MemoryOutputStream stream;
	Base64::convertToBase64(stream, data, sizeof(int16) * 128);
	return stream.toString();
}

void ScriptingObjects::MidiList::restoreFromBase64String(String base64encodedValues)
{
	MemoryOutputStream stream(data, sizeof(int16) * 128);
	Base64::convertFromBase64(stream, base64encodedValues);
}

// ScriptingModulator ===========================================================================================================

struct ScriptingObjects::ScriptingModulator::Wrapper
{
	API_VOID_METHOD_WRAPPER_2(ScriptingModulator, setAttribute);
	API_VOID_METHOD_WRAPPER_1(ScriptingModulator, setBypassed);
	API_VOID_METHOD_WRAPPER_1(ScriptingModulator, setIntensity);
};

ScriptingObjects::ScriptingModulator::ScriptingModulator(ProcessorWithScriptingContent *p, Modulator *m_) :
ConstScriptingObject(p, m_ != nullptr ? m_->getNumParameters() : 0),
mod(m_),
m(nullptr)
{
	if (mod != nullptr)
	{
		m = dynamic_cast<Modulation*>(m_);

		setName(mod->getId());

		for (int i = 0; i < mod->getNumParameters(); i++)
		{
			addConstant(mod->getIdentifierForParameterIndex(i).toString(), var(i));
		}
	}
	else
	{
		setName("Invalid Modulator");
	}

	ADD_API_METHOD_2(setAttribute);
	ADD_API_METHOD_1(setBypassed);
	ADD_API_METHOD_1(setIntensity);
}

String ScriptingObjects::ScriptingModulator::getDebugName() const
{
	if (objectExists() && !objectDeleted())
		return mod->getType().toString();

	return String("Deleted");
}

String ScriptingObjects::ScriptingModulator::getDebugValue() const
{
	if (objectExists() && !objectDeleted())
		return String(mod->getOutputValue(), 2);

	return "0.0";
}

int ScriptingObjects::ScriptingModulator::getCachedIndex(const var &indexExpression) const
{
	if (checkValidObject())
	{
		Identifier id(indexExpression.toString());

		for (int i = 0; i < mod->getNumParameters(); i++)
		{
			if (id == mod->getIdentifierForParameterIndex(i)) return i;
		}
		return -1;
	}
	else
	{
		throw String("Modulator does not exist");
	}
}

void ScriptingObjects::ScriptingModulator::assign(const int index, var newValue)
{
	setAttribute(index, (float)newValue);
}

var ScriptingObjects::ScriptingModulator::getAssignedValue(int /*index*/) const
{
	return 1.0; // Todo...
}

void ScriptingObjects::ScriptingModulator::setAttribute(int index, float value)
{
	if (checkValidObject())
		mod->setAttribute(index, value, sendNotification);
}

void ScriptingObjects::ScriptingModulator::setBypassed(bool shouldBeBypassed)
{
	if (checkValidObject())
	{
		mod->setBypassed(shouldBeBypassed);
		mod->sendChangeMessage();
	}
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
	if (checkValidObject())
	{
		if (m->getMode() == Modulation::GainMode)
		{
			const float value = jlimit<float>(0.0f, 1.0f, newIntensity);
			m->setIntensity(value);

			mod.get()->sendChangeMessage();
		}
		else
		{
			const float value = jlimit<float>(-12.0f, 12.0f, newIntensity);
			const float pitchFactor = powf(2.0f, value / 12.0f);

			m->setIntensity(pitchFactor);

			mod.get()->sendChangeMessage();
		}
	}
};



// ScriptingEffect ==============================================================================================================

struct ScriptingObjects::ScriptingEffect::Wrapper
{
	API_VOID_METHOD_WRAPPER_2(ScriptingEffect, setAttribute);
	API_VOID_METHOD_WRAPPER_1(ScriptingEffect, setBypassed);
};

ScriptingObjects::ScriptingEffect::ScriptingEffect(ProcessorWithScriptingContent *p, EffectProcessor *fx) :
ConstScriptingObject(p, fx != nullptr ? fx->getNumParameters() : 0),
effect(fx)
{
	if (fx != nullptr)
	{
		setName(fx->getId());

		for (int i = 0; i < fx->getNumParameters(); i++)
		{
			addConstant(fx->getIdentifierForParameterIndex(i).toString(), var(i));
		}
	}
	else
	{
		setName("Invalid Effect");
	}

	ADD_API_METHOD_2(setAttribute);
	ADD_API_METHOD_1(setBypassed);
};


void ScriptingObjects::ScriptingEffect::setAttribute(int parameterIndex, float newValue)
{
	if (checkValidObject())
	{
		effect->setAttribute(parameterIndex, newValue, sendNotification);
	}
}

void ScriptingObjects::ScriptingEffect::setBypassed(bool shouldBeBypassed)
{
	if (checkValidObject())
	{
		effect->setBypassed(shouldBeBypassed);
		effect->sendChangeMessage();
	}
}


// ScriptingSynth ==============================================================================================================

struct ScriptingObjects::ScriptingSynth::Wrapper
{
	API_VOID_METHOD_WRAPPER_2(ScriptingSynth, setAttribute);
	API_VOID_METHOD_WRAPPER_1(ScriptingSynth, setBypassed);
};

ScriptingObjects::ScriptingSynth::ScriptingSynth(ProcessorWithScriptingContent *p, ModulatorSynth *synth_) :
ConstScriptingObject(p, synth_ != nullptr ? synth_->getNumParameters(): 0),
synth(synth_)
{
	if (synth != nullptr)
	{
		setName(synth->getId());

		for (int i = 0; i < synth->getNumParameters(); i++)
		{
			addConstant(synth->getIdentifierForParameterIndex(i).toString(), var(i));
		}
	}
	else
	{
		setName("Invalid Effect");
	}

	ADD_API_METHOD_2(setAttribute);
	ADD_API_METHOD_1(setBypassed);
};

void ScriptingObjects::ScriptingSynth::setAttribute(int parameterIndex, float newValue)
{
	if (checkValidObject())
	{
		synth->setAttribute(parameterIndex, newValue, sendNotification);
	}
}

void ScriptingObjects::ScriptingSynth::setBypassed(bool shouldBeBypassed)
{
	if (checkValidObject())
	{
		synth->setBypassed(shouldBeBypassed);
		synth->sendChangeMessage();
	}
}

// ScriptingMidiProcessor ==============================================================================================================

struct ScriptingObjects::ScriptingMidiProcessor::Wrapper
{
	API_VOID_METHOD_WRAPPER_2(ScriptingMidiProcessor, setAttribute);
	API_VOID_METHOD_WRAPPER_1(ScriptingMidiProcessor, setBypassed);
};

ScriptingObjects::ScriptingMidiProcessor::ScriptingMidiProcessor(ProcessorWithScriptingContent *p, MidiProcessor *mp_) :
ConstScriptingObject(p, mp_ != nullptr ? mp_->getNumParameters() : 0),
mp(mp_)
{
	if (mp != nullptr)
	{
		setName(mp->getId());

		for (int i = 0; i < mp->getNumParameters(); i++)
		{
			addConstant(mp->getIdentifierForParameterIndex(i).toString(), var(i));
		}
	}
	else
	{
		setName("Invalid MidiProcessor");
	}

	ADD_API_METHOD_2(setAttribute);
	ADD_API_METHOD_1(setBypassed);
}

int ScriptingObjects::ScriptingMidiProcessor::getCachedIndex(const var &indexExpression) const
{
	if (checkValidObject())
	{
		Identifier id(indexExpression.toString());

		for (int i = 0; i < mp->getNumParameters(); i++)
		{
			if (id == mp->getIdentifierForParameterIndex(i)) return i;
		}
	}

	return -1;
}

void ScriptingObjects::ScriptingMidiProcessor::assign(const int index, var newValue)
{
	setAttribute(index, (float)newValue);
}

var ScriptingObjects::ScriptingMidiProcessor::getAssignedValue(int /*index*/) const
{
	return 1.0; // Todo...
}

void ScriptingObjects::ScriptingMidiProcessor::setAttribute(int index, float value)
{
	if (checkValidObject())
	{
		mp->setAttribute(index, value, sendNotification);
	}
}

void ScriptingObjects::ScriptingMidiProcessor::setBypassed(bool shouldBeBypassed)
{
	if (checkValidObject())
	{
		mp->setBypassed(shouldBeBypassed);
		mp->sendChangeMessage();
	}
}

// ScriptingAudioSampleProcessor ==============================================================================================================

struct ScriptingObjects::ScriptingAudioSampleProcessor::Wrapper
{
	API_VOID_METHOD_WRAPPER_2(ScriptingAudioSampleProcessor, setAttribute);
	API_VOID_METHOD_WRAPPER_1(ScriptingAudioSampleProcessor, setBypassed);
	API_METHOD_WRAPPER_0(ScriptingAudioSampleProcessor, getSampleLength);
	API_VOID_METHOD_WRAPPER_2(ScriptingAudioSampleProcessor, setSampleRange);
	API_VOID_METHOD_WRAPPER_1(ScriptingAudioSampleProcessor, setFile);
};


ScriptingObjects::ScriptingAudioSampleProcessor::ScriptingAudioSampleProcessor(ProcessorWithScriptingContent *p, AudioSampleProcessor *sampleProcessor) :
ConstScriptingObject(p, dynamic_cast<Processor*>(sampleProcessor) != nullptr ? dynamic_cast<Processor*>(sampleProcessor)->getNumParameters() : 0),
audioSampleProcessor(dynamic_cast<Processor*>(sampleProcessor))
{
	if (audioSampleProcessor != nullptr)
	{
		setName(audioSampleProcessor->getId());

		for (int i = 0; i < audioSampleProcessor->getNumParameters(); i++)
		{
			addConstant(audioSampleProcessor->getIdentifierForParameterIndex(i).toString(), var(i));
		}
	}
	else
	{
		setName("Invalid Processor");
	}

	ADD_API_METHOD_2(setAttribute);
	ADD_API_METHOD_1(setBypassed);
	ADD_API_METHOD_0(getSampleLength);
	ADD_API_METHOD_2(setSampleRange);
	ADD_API_METHOD_1(setFile);
}



void ScriptingObjects::ScriptingAudioSampleProcessor::setAttribute(int parameterIndex, float newValue)
{
	if (checkValidObject())
	{
		audioSampleProcessor->setAttribute(parameterIndex, newValue, sendNotification);
	}
}

void ScriptingObjects::ScriptingAudioSampleProcessor::setBypassed(bool shouldBeBypassed)
{
	if (checkValidObject())
	{
		audioSampleProcessor->setBypassed(shouldBeBypassed);
		audioSampleProcessor->sendChangeMessage();
	}
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
		return dynamic_cast<const AudioSampleProcessor*>(audioSampleProcessor.get())->getTotalLength();
	}
	else return 0;
}

// ScriptingTableProcessor ==============================================================================================================

struct ScriptingObjects::ScriptingTableProcessor::Wrapper
{
	API_VOID_METHOD_WRAPPER_3(ScriptingTableProcessor, addTablePoint);
	API_VOID_METHOD_WRAPPER_1(ScriptingTableProcessor, reset);
	API_VOID_METHOD_WRAPPER_5(ScriptingTableProcessor, setTablePoint);
};



ScriptingObjects::ScriptingTableProcessor::ScriptingTableProcessor(ProcessorWithScriptingContent *p, LookupTableProcessor *tableProcessor_) :
ConstScriptingObject(p, dynamic_cast<Processor*>(tableProcessor_) != nullptr ? dynamic_cast<Processor*>(tableProcessor_)->getNumParameters() : 0),
tableProcessor(dynamic_cast<Processor*>(tableProcessor_))
{
	if (tableProcessor != nullptr)
	{
		setName(tableProcessor->getId());

		for (int i = 0; i < tableProcessor->getNumParameters(); i++)
		{
			addConstant(tableProcessor->getIdentifierForParameterIndex(i).toString(), var(i));
		}
	}
	else
	{
		setName("Invalid Processor");
	}

	ADD_API_METHOD_3(addTablePoint);
	ADD_API_METHOD_1(reset);
	ADD_API_METHOD_5(setTablePoint);
}



void ScriptingObjects::ScriptingTableProcessor::setTablePoint(int tableIndex, int pointIndex, float x, float y, float curve)
{
	if (tableProcessor != nullptr)
	{
		Table *table = dynamic_cast<LookupTableProcessor*>(tableProcessor.get())->getTable(tableIndex);

		if (table != nullptr)
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

// TimerObject ==============================================================================================================

struct ScriptingObjects::TimerObject::Wrapper
{
	DYNAMIC_METHOD_WRAPPER(TimerObject, startTimer, (int)ARG(0));
	DYNAMIC_METHOD_WRAPPER(TimerObject, stopTimer);
};

ScriptingObjects::TimerObject::TimerObject(ProcessorWithScriptingContent *p) :
DynamicScriptingObject(p)
{
	ADD_DYNAMIC_METHOD(startTimer);
	ADD_DYNAMIC_METHOD(stopTimer);
}


void ScriptingObjects::TimerObject::timerCallback()
{
	var callback = getProperty("callback");
	var undefinedArgs = var::undefined();
	var::NativeFunctionArgs args(this, &undefinedArgs, 0);

	Result r = Result::ok();

	dynamic_cast<JavascriptMidiProcessor*>(getScriptProcessor())->getScriptEngine()->callExternalFunction(callback, args);

	if (r.failed())
	{
		stopTimer();
		debugError(getProcessor(), r.getErrorMessage());
	}
}
