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
	for (int i = 0; i < 128; i++) data[i] = valueToFill;
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
		if (data[i] == valueToCheck) amount++;
	}

	return amount;
}

int ScriptingObjects::MidiList::getIndex(int value) const
{
	if (empty) return -1;
	for (int i = 0; i < 128; i++)
	{
		if (data[i] == value)
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
		data[index] = value;

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
	Base64::convertToBase64(stream, data, sizeof(int) * 128);
	return stream.toString();
}

void ScriptingObjects::MidiList::restoreFromBase64String(String base64encodedValues)
{
	MemoryOutputStream stream(data, sizeof(int) * 128);
	Base64::convertFromBase64(stream, base64encodedValues);
}

void addScriptParameters(ConstScriptingObject* this_, Processor* p)
{
	DynamicObject::Ptr scriptedParameters = new DynamicObject();

	if (ProcessorWithScriptingContent* pwsc = dynamic_cast<ProcessorWithScriptingContent*>(p))
	{
		for (int i = 0; i < pwsc->getScriptingContent()->getNumComponents(); i++)
		{
			scriptedParameters->setProperty(pwsc->getScriptingContent()->getComponent(i)->getName(), var(i));
		}
	}

	this_->addConstant("ScriptParameters", var(scriptedParameters.get()));
}

// ScriptingModulator ===========================================================================================================

struct ScriptingObjects::ScriptingModulator::Wrapper
{
	API_VOID_METHOD_WRAPPER_2(ScriptingModulator, setAttribute);
	API_VOID_METHOD_WRAPPER_1(ScriptingModulator, setBypassed);
	API_VOID_METHOD_WRAPPER_1(ScriptingModulator, setIntensity);
};

ScriptingObjects::ScriptingModulator::ScriptingModulator(ProcessorWithScriptingContent *p, Modulator *m_) :
ConstScriptingObject(p, m_ != nullptr ? m_->getNumParameters() + 1 : 1),
mod(m_),
m(nullptr)
{
	if (mod != nullptr)
	{
		m = dynamic_cast<Modulation*>(m_);

		setName(mod->getId());

		addScriptParameters(this, mod.get());

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



void ScriptingObjects::ScriptingModulator::doubleClickCallback(const MouseEvent &, Component* componentToNotify)
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
			const float pitchFactor = value / 12.0f;

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
ConstScriptingObject(p, fx != nullptr ? fx->getNumParameters()+1 : 1),
effect(fx)
{
	if (fx != nullptr)
	{
		setName(fx->getId());

		addScriptParameters(this, effect.get());

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
ConstScriptingObject(p, synth_ != nullptr ? synth_->getNumParameters()+1: 1),
synth(synth_)
{
	if (synth != nullptr)
	{
		setName(synth->getId());

		addScriptParameters(this, synth.get());

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
ConstScriptingObject(p, mp_ != nullptr ? mp_->getNumParameters()+1 : 1),
mp(mp_)
{
	if (mp != nullptr)
	{
		setName(mp->getId());

		addScriptParameters(this, mp.get());

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


ScriptingObjects::TimerObject::~TimerObject()
{
	removeProperty("callback");
}

void ScriptingObjects::TimerObject::timerCallback()
{
	var undefinedArgs = var::undefined();
	
	var::NativeFunctionArgs args(this, &undefinedArgs, 0);

	Result r = Result::ok();

	dynamic_cast<JavascriptMidiProcessor*>(getScriptProcessor())->getScriptEngine()->callExternalFunction(getProperty("callback"), args, &r);

	if (r.failed())
	{
		stopTimer();
		debugError(getProcessor(), r.getErrorMessage());
	}
}

class PathPreviewComponent: public Component
{
public:

	PathPreviewComponent(Path &p_) : p(p_) { setSize(300, 300); }

	void paint(Graphics &g) override
	{
		g.setColour(Colours::white);
		p.scaleToFit(0.0f, 0.0f, (float)getWidth(), (float)getHeight(), true);
		g.fillPath(p);
	}

private:

	Path p;
};

void ScriptingObjects::PathObject::doubleClickCallback(const MouseEvent &e, Component* componentToNotify)
{
#if USE_BACKEND

	BackendProcessorEditor *editor = componentToNotify->findParentComponentOfClass<BackendProcessorEditor>();

	PathPreviewComponent* content = new PathPreviewComponent(p);
	
	MouseEvent ee = e.getEventRelativeTo(editor);

	Rectangle<int> r = Rectangle<int>(ee.getMouseDownPosition(), ee.getMouseDownPosition());

	CallOutBox::launchAsynchronously(content, r, editor);

#else

	ignoreUnused(e, componentToNotify);

#endif
}


struct ScriptingObjects::PathObject::Wrapper
{
	API_VOID_METHOD_WRAPPER_1(PathObject, loadFromData);
	API_VOID_METHOD_WRAPPER_0(PathObject, closeSubPath);
	API_VOID_METHOD_WRAPPER_2(PathObject, startNewSubPath);
	API_VOID_METHOD_WRAPPER_2(PathObject, lineTo);
	API_VOID_METHOD_WRAPPER_0(PathObject, clear);
	API_VOID_METHOD_WRAPPER_4(PathObject, quadraticTo);
};

ScriptingObjects::PathObject::PathObject(ProcessorWithScriptingContent* p) :
ConstScriptingObject(p, 0)
{
	ADD_API_METHOD_1(loadFromData);
	ADD_API_METHOD_0(closeSubPath);
	ADD_API_METHOD_0(clear);
	ADD_API_METHOD_2(startNewSubPath);
	ADD_API_METHOD_2(lineTo);
	ADD_API_METHOD_4(quadraticTo);
}

ScriptingObjects::PathObject::~PathObject()
{

}


void ScriptingObjects::PathObject::loadFromData(var data)
{
	if (data.isArray())
	{
		p.clear();

		Array<unsigned char> pathData;

		Array<var> *varData = data.getArray();

		const int numElements = varData->size();

		pathData.ensureStorageAllocated(numElements);

		for (int i = 0; i < numElements; i++)
		{
			pathData.add(static_cast<unsigned char>((int)varData->getUnchecked(i)));
		}

		p.loadPathFromData(pathData.getRawDataPointer(), numElements);
	}
}

void ScriptingObjects::PathObject::clear()
{
	p.clear();
}

void ScriptingObjects::PathObject::startNewSubPath(var x, var y)
{
	p.startNewSubPath(x, y);
}

void ScriptingObjects::PathObject::closeSubPath()
{
	p.closeSubPath();
}

void ScriptingObjects::PathObject::lineTo(var x, var y)
{
	p.lineTo(x, y);
}

void ScriptingObjects::PathObject::quadraticTo(var cx, var cy, var x, var y)
{
	p.quadraticTo(cx, cy, x, y);
}

struct ScriptingObjects::GraphicsObject::Wrapper
{
	API_VOID_METHOD_WRAPPER_1(GraphicsObject, fillAll);
	API_VOID_METHOD_WRAPPER_1(GraphicsObject, setColour);
	API_VOID_METHOD_WRAPPER_1(GraphicsObject, setOpacity);
	API_VOID_METHOD_WRAPPER_1(GraphicsObject, fillRect);
	API_VOID_METHOD_WRAPPER_2(GraphicsObject, drawRect);
	API_VOID_METHOD_WRAPPER_3(GraphicsObject, drawRoundedRectangle);
	API_VOID_METHOD_WRAPPER_2(GraphicsObject, fillRoundedRectangle);
	API_VOID_METHOD_WRAPPER_5(GraphicsObject, drawLine);
	API_VOID_METHOD_WRAPPER_3(GraphicsObject, drawHorizontalLine);
	API_VOID_METHOD_WRAPPER_2(GraphicsObject, setFont);
	API_VOID_METHOD_WRAPPER_2(GraphicsObject, drawText);
	API_VOID_METHOD_WRAPPER_1(GraphicsObject, setGradientFill);
	API_VOID_METHOD_WRAPPER_2(GraphicsObject, drawEllipse);
	API_VOID_METHOD_WRAPPER_1(GraphicsObject, fillEllipse);
	API_VOID_METHOD_WRAPPER_4(GraphicsObject, drawImage);
	API_VOID_METHOD_WRAPPER_3(GraphicsObject, drawDropShadow);
	API_VOID_METHOD_WRAPPER_2(GraphicsObject, addDropShadowFromAlpha);
	API_VOID_METHOD_WRAPPER_3(GraphicsObject, drawTriangle);
	API_VOID_METHOD_WRAPPER_2(GraphicsObject, fillTriangle);
	API_VOID_METHOD_WRAPPER_2(GraphicsObject, fillPath);
};

ScriptingObjects::GraphicsObject::GraphicsObject(ProcessorWithScriptingContent *p, ConstScriptingObject* parent_) :
ConstScriptingObject(p, 0),
parent(parent_),
rectangleResult(Result::ok())
{
	ADD_API_METHOD_1(fillAll);
	ADD_API_METHOD_1(setColour);
	ADD_API_METHOD_1(setOpacity);
	ADD_API_METHOD_2(drawRect);
	ADD_API_METHOD_1(fillRect);
	ADD_API_METHOD_3(drawRoundedRectangle);
	ADD_API_METHOD_2(fillRoundedRectangle);
	ADD_API_METHOD_5(drawLine);
	ADD_API_METHOD_3(drawHorizontalLine);
	ADD_API_METHOD_2(setFont);
	ADD_API_METHOD_2(drawText);
	ADD_API_METHOD_1(setGradientFill);
	ADD_API_METHOD_2(drawEllipse);
	ADD_API_METHOD_1(fillEllipse);
	ADD_API_METHOD_4(drawImage);
	ADD_API_METHOD_3(drawDropShadow);
	ADD_API_METHOD_2(addDropShadowFromAlpha);
	ADD_API_METHOD_3(drawTriangle);
	ADD_API_METHOD_2(fillTriangle);
	ADD_API_METHOD_2(fillPath);
}

ScriptingObjects::GraphicsObject::~GraphicsObject()
{
	parent = nullptr;
	imageToDraw = nullptr;
	g = nullptr;
}

void ScriptingObjects::GraphicsObject::fillAll(int colour)
{
	initGraphics();
	Colour c((uint32)colour);

	g->fillAll(c);

	
}

void ScriptingObjects::GraphicsObject::fillRect(var area)
{
	initGraphics();

	g->fillRect(getRectangleFromVar(area));
}

void ScriptingObjects::GraphicsObject::drawRect(var area, float borderSize)
{
	initGraphics();

	g->drawRect(getRectangleFromVar(area), borderSize);
}

void ScriptingObjects::GraphicsObject::fillRoundedRectangle(var area, float cornerSize)
{
	initGraphics();

	g->fillRoundedRectangle(getRectangleFromVar(area), cornerSize);
}

void ScriptingObjects::GraphicsObject::drawRoundedRectangle(var area, float cornerSize, float borderSize)
{
	initGraphics();



	g->drawRoundedRectangle(getRectangleFromVar(area), cornerSize, borderSize);
}

void ScriptingObjects::GraphicsObject::drawHorizontalLine(int y, float x1, float x2)
{
	initGraphics();

	g->drawHorizontalLine(y, x1, x2);
}

void ScriptingObjects::GraphicsObject::setOpacity(float alphaValue)
{
	initGraphics();

	

	g->setOpacity(alphaValue);
}

void ScriptingObjects::GraphicsObject::drawLine(float x1, float x2, float y1, float y2, float lineThickness)
{
	initGraphics();

	g->drawLine(x1, y1, x2, y2, lineThickness);
}

void ScriptingObjects::GraphicsObject::setColour(int colour)
{
	
	currentColour = Colour((uint32)colour);
	g->setColour(currentColour);

	useGradient = false;
}

void ScriptingObjects::GraphicsObject::setFont(String fontName, float fontSize)
{
	MainController *mc = getScriptProcessor()->getMainController_();

	juce::Typeface::Ptr typeface = mc->getFont(fontName);

	if (typeface != nullptr)
	{
		currentFont = Font(typeface).withHeight(fontSize);
	}
	else
	{
		currentFont = Font(fontName, fontSize, Font::plain);
	}

	g->setFont(currentFont);
}

void ScriptingObjects::GraphicsObject::drawText(String text, var area)
{
	initGraphics();

	Rectangle<float> r = getRectangleFromVar(area);

	currentFont.setHeightWithoutChangingWidth(r.getHeight());

	g->setFont(currentFont);

	g->drawText(text, r, Justification::centred);
}

void ScriptingObjects::GraphicsObject::setGradientFill(var gradientData)
{
	if (gradientData.isArray())
	{
		Array<var>* data = gradientData.getArray();

		if (gradientData.getArray()->size() == 6)
		{
			currentGradient = ColourGradient(Colour((uint32)(int64)data->getUnchecked(0)), (float)data->getUnchecked(1), (float)data->getUnchecked(2),
					 					       Colour((uint32)(int64)data->getUnchecked(3)), (float)data->getUnchecked(4), (float)data->getUnchecked(5), false);

			useGradient = true;

			g->setGradientFill(currentGradient);
		}
		else
		{
			reportScriptError("Gradient Data must have six elements");
		}
	}
	else
	{
		reportScriptError("Gradient Data is not sufficient");
	}
}



void ScriptingObjects::GraphicsObject::drawEllipse(var area, float lineThickness)
{
	initGraphics();

	g->drawEllipse(getRectangleFromVar(area), lineThickness);
}

void ScriptingObjects::GraphicsObject::fillEllipse(var area)
{
	initGraphics();

	g->fillEllipse(getRectangleFromVar(area));
}

void ScriptingObjects::GraphicsObject::drawImage(String imageName, var area, int xOffset, int yOffset)
{
	initGraphics();

	auto sc = dynamic_cast<ScriptingApi::Content::ScriptPanel*>(parent);

	const Image *img = sc->getLoadedImage(imageName);

	if (img != nullptr && img->isValid())
	{
		Rectangle<float> r = getRectangleFromVar(area);

		g->drawImage(*img, (int)r.getX(), (int)r.getY(), (int)r.getWidth(), (int)r.getHeight(), xOffset, yOffset, (int)r.getWidth(), (int)r.getHeight());
	}
	else
	{
		reportScriptError("Image not found");
	};
}

void ScriptingObjects::GraphicsObject::drawDropShadow(var area, int colour, int radius)
{
	initGraphics();

	DropShadow shadow;

	shadow.colour = Colour((uint32)colour);
	shadow.radius = radius;

	auto r = getIntRectangleFromVar(area);

	shadow.drawForRectangle(*g, r);
}

void ScriptingObjects::GraphicsObject::drawTriangle(var area, float angle, float lineThickness)
{
	initGraphics();

	Path p;
	p.startNewSubPath(0.5f, 0.0f);
	p.lineTo(1.0f, 1.0f);
	p.lineTo(0.0f, 1.0f);
	p.closeSubPath();
	p.applyTransform(AffineTransform::rotation(angle));
	auto r = getRectangleFromVar(area);
	p.scaleToFit(r.getX(), r.getY(), r.getWidth(), r.getHeight(), false);
	
	PathStrokeType pst(lineThickness);
	g->strokePath(p, pst);
}

void ScriptingObjects::GraphicsObject::fillTriangle(var area, float angle)
{
	initGraphics();

	Path p;
	p.startNewSubPath(0.5f, 0.0f);
	p.lineTo(1.0f, 1.0f);
	p.lineTo(0.0f, 1.0f);
	p.closeSubPath();
	p.applyTransform(AffineTransform::rotation(angle));
	auto r = getRectangleFromVar(area);
	p.scaleToFit(r.getX(), r.getY(), r.getWidth(), r.getHeight(), false);

	g->fillPath(p);
}

void ScriptingObjects::GraphicsObject::addDropShadowFromAlpha(int colour, int radius)
{
	initGraphics();

	DropShadow shadow;

	shadow.colour = Colour((uint32)colour);
	shadow.radius = radius;


	shadow.drawForImage(*g, *imageToDraw);
}

void ScriptingObjects::GraphicsObject::fillPath(var path, var area)
{
	if (PathObject* pathObject = dynamic_cast<PathObject*>(path.getObject()))
	{
		Path p = pathObject->getPath();
		Rectangle<float> r = getRectangleFromVar(area);

		p.scaleToFit(r.getX(), r.getY(), r.getWidth(), r.getHeight(), false);

		g->fillPath(p);
	}
}

Rectangle<float> ScriptingObjects::GraphicsObject::getRectangleFromVar(const var &data)
{
	Rectangle<float>&& f = ApiHelpers::getRectangleFromVar(data, &rectangleResult);

	if (rectangleResult.failed()) reportScriptError(rectangleResult.getErrorMessage());

	return f;
}

Rectangle<int> ScriptingObjects::GraphicsObject::getIntRectangleFromVar(const var &data)
{
	Rectangle<int>&& f = ApiHelpers::getIntRectangleFromVar(data, &rectangleResult);

	if (rectangleResult.failed()) reportScriptError(rectangleResult.getErrorMessage());

	return f;
}

void ScriptingObjects::GraphicsObject::initGraphics()
{
	if (g == nullptr) reportScriptError("Graphics not initialised");

}

