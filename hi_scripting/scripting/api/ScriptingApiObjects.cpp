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
        if (value == -1)
		{
            if(data[index] != -1)
            {
                numValues--;
                if (numValues == 0) empty = true;
            }
		}
		else
		{
            if(data[index] == -1)
            {
                numValues++;
                empty = false;
            }
            
            data[index] = value;
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


struct ScriptingObjects::ScriptSliderPackData::Wrapper
{
	API_VOID_METHOD_WRAPPER_2(ScriptSliderPackData, setValue);
	API_VOID_METHOD_WRAPPER_1(ScriptSliderPackData, setNumSliders);
	API_METHOD_WRAPPER_1(ScriptSliderPackData, getValue);
	API_METHOD_WRAPPER_0(ScriptSliderPackData, getNumSliders);
	API_VOID_METHOD_WRAPPER_3(ScriptSliderPackData, setRange);
	
};

ScriptingObjects::ScriptSliderPackData::ScriptSliderPackData(ProcessorWithScriptingContent* pwsc) :
	ConstScriptingObject(pwsc, 0)
{
	data.setNumSliders(16);

	ADD_API_METHOD_2(setValue);
	ADD_API_METHOD_1(setNumSliders);
	ADD_API_METHOD_1(getValue);
	ADD_API_METHOD_0(getNumSliders);
	ADD_API_METHOD_3(setRange);
}

void ScriptingObjects::ScriptSliderPackData::rightClickCallback(const MouseEvent& e, Component *c)
{
#if USE_BACKEND
	SliderPack *s = new SliderPack(&data);

	const int numSliders = getNumSliders();



	int widthPerSlider = 16;

	if (numSliders > 64)
		widthPerSlider = 8;

	s->setSize((int)getNumSliders() * 16, 200);

	auto editor = GET_BACKEND_ROOT_WINDOW(c);

	MouseEvent ee = e.getEventRelativeTo(editor);

	editor->getRootFloatingTile()->showComponentInRootPopup(s, editor, ee.getMouseDownPosition());
#else
    ignoreUnused(e, c);
#endif
}

var ScriptingObjects::ScriptSliderPackData::getStepSize() const
{
	return data.getStepSize();
}

void ScriptingObjects::ScriptSliderPackData::setNumSliders(var numSliders)
{
	data.setNumSliders(numSliders);
}

int ScriptingObjects::ScriptSliderPackData::getNumSliders() const
{
	return data.getNumSliders();
}

void ScriptingObjects::ScriptSliderPackData::setValue(int sliderIndex, float value)
{
	data.setValue(sliderIndex, value, sendNotification);
}

float ScriptingObjects::ScriptSliderPackData::getValue(int index) const
{
	return data.getValue((int)index);
}

void ScriptingObjects::ScriptSliderPackData::setRange(double minValue, double maxValue, double stepSize)
{
	data.setRange(minValue, maxValue, stepSize);
}

// ScriptingModulator ===========================================================================================================

struct ScriptingObjects::ScriptingModulator::Wrapper
{
	API_VOID_METHOD_WRAPPER_2(ScriptingModulator, setAttribute);
    API_METHOD_WRAPPER_1(ScriptingModulator, getAttribute);
	API_METHOD_WRAPPER_0(ScriptingModulator, getNumAttributes);
	API_VOID_METHOD_WRAPPER_1(ScriptingModulator, setBypassed);
	API_VOID_METHOD_WRAPPER_1(ScriptingModulator, setIntensity);
	API_METHOD_WRAPPER_0(ScriptingModulator, getCurrentLevel);
	API_METHOD_WRAPPER_0(ScriptingModulator, exportState);
	API_VOID_METHOD_WRAPPER_1(ScriptingModulator, restoreState);
	API_VOID_METHOD_WRAPPER_1(ScriptingModulator, restoreScriptControls);
	API_METHOD_WRAPPER_0(ScriptingModulator, exportScriptControls);
	API_METHOD_WRAPPER_3(ScriptingModulator, addModulator);
	API_METHOD_WRAPPER_3(ScriptingModulator, addGlobalModulator);
	API_METHOD_WRAPPER_0(ScriptingModulator, asTableProcessor);

};

ScriptingObjects::ScriptingModulator::ScriptingModulator(ProcessorWithScriptingContent *p, Modulator *m_) :
ConstScriptingObject(p, m_ != nullptr ? m_->getNumParameters() + 1 : 1),
mod(m_),
m(nullptr),
moduleHandler(m_)
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
    ADD_API_METHOD_1(getAttribute);
	ADD_API_METHOD_0(getCurrentLevel);
	ADD_API_METHOD_0(exportState);
	ADD_API_METHOD_1(restoreState);
	ADD_API_METHOD_0(getNumAttributes);
	ADD_API_METHOD_1(restoreScriptControls);
	ADD_API_METHOD_0(exportScriptControls);
	ADD_API_METHOD_3(addModulator);
	ADD_API_METHOD_3(addGlobalModulator);
	ADD_API_METHOD_0(asTableProcessor);
}

String ScriptingObjects::ScriptingModulator::getDebugName() const
{
	if (objectExists() && !objectDeleted())
		return mod->getId();

	return String("Invalid");
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

float ScriptingObjects::ScriptingModulator::getAttribute(int parameterIndex)
{
    if (checkValidObject())
    {
        return mod->getAttribute(parameterIndex);
    }

	return 0.0f;
}

int ScriptingObjects::ScriptingModulator::getNumAttributes() const
{
	if (checkValidObject())
	{
		return mod->getNumParameters();
	}

	return 0;
}

void ScriptingObjects::ScriptingModulator::setBypassed(bool shouldBeBypassed)
{
	if (checkValidObject())
	{
		mod->setBypassed(shouldBeBypassed, sendNotification);
		mod->sendChangeMessage();
	}
}



void ScriptingObjects::ScriptingModulator::doubleClickCallback(const MouseEvent &, Component* componentToNotify)
{
#if USE_BACKEND
	if (objectExists() && !objectDeleted())
	{
		auto *editor = GET_BACKEND_ROOT_WINDOW(componentToNotify);

		Processor *p = ProcessorHelpers::getFirstProcessorWithName(editor->getMainSynthChain(), mod->getId());

		if (p != nullptr)
		{
			editor->getMainPanel()->setRootProcessorWithUndo(p);
		}
	}
#else 
	ignoreUnused(componentToNotify);
#endif
}

void ScriptingObjects::ScriptingModulator::rightClickCallback(const MouseEvent& e, Component* t)
{
	Helpers::showProcessorEditorPopup(e, t, mod);
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



float ScriptingObjects::ScriptingModulator::getCurrentLevel()
{
	if (checkValidObject())
	{
		return m->getProcessor()->getDisplayValues().outL;
	}
	
	return 0.f;
}

String ScriptingObjects::ScriptingModulator::exportState()
{
	if (checkValidObject())
	{
		return ProcessorHelpers::getBase64String(mod);
	}

	return String();
}

void ScriptingObjects::ScriptingModulator::restoreState(String base64State)
{
	if (checkValidObject())
	{
		ProcessorHelpers::restoreFromBase64String(mod, base64State);
	}
}

String ScriptingObjects::ScriptingModulator::exportScriptControls()
{
	if (dynamic_cast<ProcessorWithScriptingContent*>(mod.get()) == nullptr)
	{
		reportScriptError("exportScriptControls can only be used on Script Processors");
	}

	if (checkValidObject())
	{
		return ProcessorHelpers::getBase64String(mod, false, true);
	}

	return String();
}

void ScriptingObjects::ScriptingModulator::restoreScriptControls(String base64Controls)
{
	if (dynamic_cast<ProcessorWithScriptingContent*>(mod.get()) == nullptr)
	{
		reportScriptError("restoreScriptControls can only be used on Script Processors");
	}

	if (checkValidObject())
	{
		ProcessorHelpers::restoreFromBase64String(mod, base64Controls, true);
	}
}

var ScriptingObjects::ScriptingModulator::addModulator(var chainIndex, var typeName, var modName)
{
	if (checkValidObject())
	{
		ModulatorChain *c = dynamic_cast<ModulatorChain*>(mod->getChildProcessor(chainIndex));

		if (c == nullptr)
			reportScriptError("Modulator Chain with index " + chainIndex.toString() + " does not exist");
		
		Processor* p = moduleHandler.addModule(c, typeName, modName, -1);

		if (p != nullptr)
		{
			auto newMod = new ScriptingObjects::ScriptingModulator(getScriptProcessor(), dynamic_cast<Modulator*>(p));
			return var(newMod);
		}
		
	}

	return var();
}

var ScriptingObjects::ScriptingModulator::addGlobalModulator(var chainIndex, var globalMod, String modName)
{
	if (checkValidObject())
	{
		if (auto gm = dynamic_cast<ScriptingModulator*>(globalMod.getObject()))
		{
			ModulatorChain *c = dynamic_cast<ModulatorChain*>(mod->getChildProcessor(chainIndex));

			if (c == nullptr)
				reportScriptError("Modulator Chain with index " + chainIndex.toString() + " does not exist");

			auto p = moduleHandler.addAndConnectToGlobalModulator(c, gm->getModulator(), modName);

			if (p != nullptr)
			{
				auto newMod = new ScriptingObjects::ScriptingModulator(getScriptProcessor(), p);
				return var(newMod);
			}
			
		}
	}

	return var();
}

var ScriptingObjects::ScriptingModulator::asTableProcessor()
{
	if (checkValidObject())
	{
		auto ltp = dynamic_cast<LookupTableProcessor*>(mod.get());

		if (ltp == nullptr)
			return var(); // don't complain here, handle it on scripting level

		auto t = new ScriptingTableProcessor(getScriptProcessor(), ltp);
			return var(t);
	}

	auto t = new ScriptingObjects::ScriptingTableProcessor(getScriptProcessor(), nullptr);
	return var(t);
}

// ScriptingEffect ==============================================================================================================

struct ScriptingObjects::ScriptingEffect::Wrapper
{
	API_VOID_METHOD_WRAPPER_2(ScriptingEffect, setAttribute);
    API_METHOD_WRAPPER_1(ScriptingEffect, getAttribute);
	API_METHOD_WRAPPER_0(ScriptingEffect, getNumAttributes);
	API_VOID_METHOD_WRAPPER_1(ScriptingEffect, setBypassed);
	API_METHOD_WRAPPER_0(ScriptingEffect, exportState);
	API_METHOD_WRAPPER_1(ScriptingEffect, getCurrentLevel);
	API_VOID_METHOD_WRAPPER_1(ScriptingEffect, restoreState);
	API_VOID_METHOD_WRAPPER_1(ScriptingEffect, restoreScriptControls);
	API_METHOD_WRAPPER_0(ScriptingEffect, exportScriptControls);
	API_METHOD_WRAPPER_3(ScriptingEffect, addModulator);
	API_METHOD_WRAPPER_3(ScriptingEffect, addGlobalModulator);
};

ScriptingObjects::ScriptingEffect::ScriptingEffect(ProcessorWithScriptingContent *p, EffectProcessor *fx) :
ConstScriptingObject(p, fx != nullptr ? fx->getNumParameters()+1 : 1),
effect(fx),
moduleHandler(fx)
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
    ADD_API_METHOD_1(getAttribute);
	ADD_API_METHOD_1(getCurrentLevel);
	ADD_API_METHOD_0(exportState);
	ADD_API_METHOD_1(restoreState);
	ADD_API_METHOD_1(restoreScriptControls);
	ADD_API_METHOD_0(exportScriptControls);
	ADD_API_METHOD_0(getNumAttributes);
	ADD_API_METHOD_3(addModulator);
	ADD_API_METHOD_3(addGlobalModulator);
};


void ScriptingObjects::ScriptingEffect::rightClickCallback(const MouseEvent& e, Component* t)
{
	Helpers::showProcessorEditorPopup(e, t, effect.get());
}

void ScriptingObjects::ScriptingEffect::setAttribute(int parameterIndex, float newValue)
{
	if (checkValidObject())
	{
		effect->setAttribute(parameterIndex, newValue, sendNotification);
	}
}

float ScriptingObjects::ScriptingEffect::getAttribute(int parameterIndex)
{
    if (checkValidObject())
    {
        return effect->getAttribute(parameterIndex);
    }

	return 0.0f;
}

int ScriptingObjects::ScriptingEffect::getNumAttributes() const
{
	if (checkValidObject())
	{
		return effect->getNumParameters();
	}

	return 0;
}

void ScriptingObjects::ScriptingEffect::setBypassed(bool shouldBeBypassed)
{
	if (checkValidObject())
	{
		effect->setBypassed(shouldBeBypassed, sendNotification);
		effect->sendChangeMessage();
	}
}



String ScriptingObjects::ScriptingEffect::exportState()
{
	if (checkValidObject())
	{
		return ProcessorHelpers::getBase64String(effect);
	}

	return String();
}

void ScriptingObjects::ScriptingEffect::restoreState(String base64State)
{
	if (checkValidObject())
	{
		ProcessorHelpers::restoreFromBase64String(effect, base64State);
	}
}

String ScriptingObjects::ScriptingEffect::exportScriptControls()
{
	if (dynamic_cast<ProcessorWithScriptingContent*>(effect.get()) == nullptr)
	{
		reportScriptError("exportScriptControls can only be used on Script Processors");
	}

	if (checkValidObject())
	{
		return ProcessorHelpers::getBase64String(effect, false, true);
	}

	return String();
}

void ScriptingObjects::ScriptingEffect::restoreScriptControls(String base64Controls)
{
	if (dynamic_cast<ProcessorWithScriptingContent*>(effect.get()) == nullptr)
	{
		reportScriptError("restoreScriptControls can only be used on Script Processors");
	}

	if (checkValidObject())
	{
		ProcessorHelpers::restoreFromBase64String(effect, base64Controls, true);
	}
}

float ScriptingObjects::ScriptingEffect::getCurrentLevel(bool leftChannel)
{
	if (checkValidObject())
	{
		return leftChannel ? effect->getDisplayValues().outL : effect->getDisplayValues().outR;
	}

	return 0.0f;
}

var ScriptingObjects::ScriptingEffect::addModulator(var chainIndex, var typeName, var modName)
{
	if (checkValidObject())
	{
		ModulatorChain *c = dynamic_cast<ModulatorChain*>(effect->getChildProcessor(chainIndex));

		if (c == nullptr)
			reportScriptError("Modulator Chain with index " + chainIndex.toString() + " does not exist");

		Processor* p = moduleHandler.addModule(c, typeName, modName, -1);

		if (p != nullptr)
		{
			auto mod = new ScriptingObjects::ScriptingModulator(getScriptProcessor(), dynamic_cast<Modulator*>(p));
			return var(mod);
		}

		return var();
	}
	else
	{
		return var();
	}
	
}

var ScriptingObjects::ScriptingEffect::addGlobalModulator(var chainIndex, var globalMod, String modName)
{
	if (checkValidObject())
	{
		if (auto gm = dynamic_cast<ScriptingModulator*>(globalMod.getObject()))
		{
			ModulatorChain *c = dynamic_cast<ModulatorChain*>(effect->getChildProcessor(chainIndex));

			if (c == nullptr)
				reportScriptError("Modulator Chain with index " + chainIndex.toString() + " does not exist");

			auto p = moduleHandler.addAndConnectToGlobalModulator(c, gm->getModulator(), modName);

			if (p != nullptr)
			{
				auto mod = new ScriptingObjects::ScriptingModulator(getScriptProcessor(), p);
				return var(mod);
			}

			return var();
		}
	}

	return var();
}

// ScriptingSlotFX ==============================================================================================================


struct ScriptingObjects::ScriptingSlotFX::Wrapper
{
    API_METHOD_WRAPPER_1(ScriptingSlotFX, setEffect);
    API_VOID_METHOD_WRAPPER_0(ScriptingSlotFX, clear);
	API_VOID_METHOD_WRAPPER_1(ScriptingSlotFX, swap);
    
};

ScriptingObjects::ScriptingSlotFX::ScriptingSlotFX(ProcessorWithScriptingContent *p, EffectProcessor *fx) :
ConstScriptingObject(p, fx != nullptr ? fx->getNumParameters()+1 : 1),
slotFX(fx)
{
    if (fx != nullptr)
    {
        setName(fx->getId());
        
        addScriptParameters(this, slotFX.get());
        
        for (int i = 0; i < fx->getNumParameters(); i++)
        {
            addConstant(fx->getIdentifierForParameterIndex(i).toString(), var(i));
        }
    }
    else
    {
        setName("Invalid Effect");
    }
    
    ADD_API_METHOD_1(setEffect);
    ADD_API_METHOD_0(clear);
	ADD_API_METHOD_1(swap);
    
};



void ScriptingObjects::ScriptingSlotFX::clear()
{
	if (auto slot = getSlotFX())
	{
        slot->reset();
	}
	else
	{
		reportScriptError("Invalid Slot");
	}
}


ScriptingObjects::ScriptingEffect* ScriptingObjects::ScriptingSlotFX::setEffect(String effectName)
{
	if(auto slot = getSlotFX())
    {
        if(slot->setEffect(effectName))
        {
			return new ScriptingEffect(getScriptProcessor(), slot->getCurrentEffect());
        }
        else
        {
            reportScriptError("Invalid Effect Type");
            RETURN_IF_NO_THROW(new ScriptingEffect(getScriptProcessor(), nullptr))
        }
    }
	else
	{
		reportScriptError("Invalid Slot");
		RETURN_IF_NO_THROW(new ScriptingEffect(getScriptProcessor(), nullptr))
	}
}

void ScriptingObjects::ScriptingSlotFX::swap(var otherSlot)
{
	if (auto t = getSlotFX())
	{
		if (auto sl = dynamic_cast<ScriptingSlotFX*>(otherSlot.getObject()))
		{
			if (auto other = sl->getSlotFX())
			{
				t->swap(other);
			}
			else
			{
				reportScriptError("Target Slot is invalid");
			}
		}
		else
		{
			reportScriptError("Target Slot does not exist");
		}
	}
	else
	{
		reportScriptError("Source Slot is invalid");
	}
}

SlotFX* ScriptingObjects::ScriptingSlotFX::getSlotFX()
{
	return dynamic_cast<SlotFX*>(slotFX.get());
}

// ScriptingSynth ==============================================================================================================

struct ScriptingObjects::ScriptingSynth::Wrapper
{
	API_VOID_METHOD_WRAPPER_2(ScriptingSynth, setAttribute);
    API_METHOD_WRAPPER_1(ScriptingSynth, getAttribute);
	API_METHOD_WRAPPER_0(ScriptingSynth, getNumAttributes);
	API_VOID_METHOD_WRAPPER_1(ScriptingSynth, setBypassed);
	API_METHOD_WRAPPER_1(ScriptingSynth, getChildSynthByIndex);
	API_METHOD_WRAPPER_0(ScriptingSynth, exportState);
	API_METHOD_WRAPPER_1(ScriptingSynth, getCurrentLevel);
	API_VOID_METHOD_WRAPPER_1(ScriptingSynth, restoreState);
	API_METHOD_WRAPPER_3(ScriptingSynth, addModulator);
	API_METHOD_WRAPPER_3(ScriptingSynth, addGlobalModulator);
	API_METHOD_WRAPPER_0(ScriptingSynth, asSampler);
};

ScriptingObjects::ScriptingSynth::ScriptingSynth(ProcessorWithScriptingContent *p, ModulatorSynth *synth_) :
	ConstScriptingObject(p, synth_ != nullptr ? synth_->getNumParameters() + 1 : 1),
	synth(synth_),
	moduleHandler(synth_)
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
    ADD_API_METHOD_1(getAttribute);
	ADD_API_METHOD_1(setBypassed);
	ADD_API_METHOD_1(getChildSynthByIndex);
	ADD_API_METHOD_1(getCurrentLevel);
	ADD_API_METHOD_0(exportState);
	ADD_API_METHOD_1(restoreState);
	ADD_API_METHOD_0(getNumAttributes);
	ADD_API_METHOD_3(addModulator);
	ADD_API_METHOD_3(addGlobalModulator);
	ADD_API_METHOD_0(asSampler);
};


void ScriptingObjects::ScriptingSynth::rightClickCallback(const MouseEvent& e, Component* t)
{
	Helpers::showProcessorEditorPopup(e, t, synth);
}

void ScriptingObjects::ScriptingSynth::setAttribute(int parameterIndex, float newValue)
{
	if (checkValidObject())
	{
		synth->setAttribute(parameterIndex, newValue, sendNotification);
	}
}

float ScriptingObjects::ScriptingSynth::getAttribute(int parameterIndex)
{
    if (checkValidObject())
    {
        return synth->getAttribute(parameterIndex);
    }

	return 0.0f;
}

int ScriptingObjects::ScriptingSynth::getNumAttributes() const
{
	if (checkValidObject())
	{
		return synth->getNumParameters();
	}

	return 0;
}

void ScriptingObjects::ScriptingSynth::setBypassed(bool shouldBeBypassed)
{
	if (checkValidObject())
	{
		synth->setBypassed(shouldBeBypassed, sendNotification);
		synth->sendChangeMessage();
	}
}

ScriptingObjects::ScriptingSynth* ScriptingObjects::ScriptingSynth::getChildSynthByIndex(int index)
{
	if (getScriptProcessor()->objectsCanBeCreated())
	{
		if (Chain* c = dynamic_cast<Chain*>(synth.get()))
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

String ScriptingObjects::ScriptingSynth::exportState()
{
	if (checkValidObject())
	{
		return ProcessorHelpers::getBase64String(synth);
	}

	return String();
}

void ScriptingObjects::ScriptingSynth::restoreState(String base64State)
{
	if (checkValidObject())
	{
		ProcessorHelpers::restoreFromBase64String(synth, base64State);
	}
}

float ScriptingObjects::ScriptingSynth::getCurrentLevel(bool leftChannel)
{
	if (checkValidObject())
	{
		return leftChannel ? synth->getDisplayValues().outL : synth->getDisplayValues().outR;
	}

	return 0.0f;
}

var ScriptingObjects::ScriptingSynth::addModulator(var chainIndex, var typeName, var modName)
{
	if (checkValidObject())
	{
		ModulatorChain *c = dynamic_cast<ModulatorChain*>(synth->getChildProcessor(chainIndex));

		if (c == nullptr)
			reportScriptError("Modulator Chain with index " + chainIndex.toString() + " does not exist");

		Processor* p = moduleHandler.addModule(c, typeName, modName, -1);

		if (p != nullptr)
		{
			auto mod = new ScriptingObjects::ScriptingModulator(getScriptProcessor(), dynamic_cast<Modulator*>(p));
			return var(mod);
		}
	}

	return var();
}

var ScriptingObjects::ScriptingSynth::addGlobalModulator(var chainIndex, var globalMod, String modName)
{
	if (checkValidObject())
	{
		if (auto gm = dynamic_cast<ScriptingModulator*>(globalMod.getObject()))
		{
			ModulatorChain *c = dynamic_cast<ModulatorChain*>(synth->getChildProcessor(chainIndex));

			if (c == nullptr)
				reportScriptError("Modulator Chain with index " + chainIndex.toString() + " does not exist");

			auto p = moduleHandler.addAndConnectToGlobalModulator(c, gm->getModulator(), modName);

			if (p != nullptr)
			{
				auto mod = new ScriptingObjects::ScriptingModulator(getScriptProcessor(), p);
				return var(mod);
			}
		}
	}

	return var();
}

var ScriptingObjects::ScriptingSynth::asSampler()
{
	if (checkValidObject())
	{
		auto sampler = dynamic_cast<ModulatorSampler*>(synth.get());

		if (sampler == nullptr)
			return var(); // don't complain here, handle it on scripting level

		auto t = new ScriptingApi::Sampler(getScriptProcessor(), sampler);
		return var(t);
	}

	auto t = new ScriptingApi::Sampler(getScriptProcessor(), nullptr);
	return var(t);
}

// ScriptingMidiProcessor ==============================================================================================================

struct ScriptingObjects::ScriptingMidiProcessor::Wrapper
{
	API_VOID_METHOD_WRAPPER_2(ScriptingMidiProcessor, setAttribute);
    API_METHOD_WRAPPER_1(ScriptingMidiProcessor, getAttribute);
	API_METHOD_WRAPPER_0(ScriptingMidiProcessor, getNumAttributes);
	API_VOID_METHOD_WRAPPER_1(ScriptingMidiProcessor, setBypassed);
	API_METHOD_WRAPPER_0(ScriptingMidiProcessor, exportState);
	API_VOID_METHOD_WRAPPER_1(ScriptingMidiProcessor, restoreState);
	API_VOID_METHOD_WRAPPER_1(ScriptingMidiProcessor, restoreScriptControls);
	API_METHOD_WRAPPER_0(ScriptingMidiProcessor, exportScriptControls);
	
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
    ADD_API_METHOD_1(getAttribute);
	ADD_API_METHOD_1(setBypassed);
	ADD_API_METHOD_0(exportState);
	ADD_API_METHOD_1(restoreState);
	ADD_API_METHOD_1(restoreScriptControls);
	ADD_API_METHOD_0(exportScriptControls);
	ADD_API_METHOD_0(getNumAttributes);
}

void ScriptingObjects::ScriptingMidiProcessor::rightClickCallback(const MouseEvent& e, Component* t)
{
	Helpers::showProcessorEditorPopup(e, t, mp);
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

float ScriptingObjects::ScriptingMidiProcessor::getAttribute(int parameterIndex)
{
    if (checkValidObject())
    {
        return mp->getAttribute(parameterIndex);
    }

	return 0.0f;
}

int ScriptingObjects::ScriptingMidiProcessor::getNumAttributes() const
{
	if (checkValidObject())
	{
		return mp->getNumParameters();
	}

	return 0;
}

void ScriptingObjects::ScriptingMidiProcessor::setBypassed(bool shouldBeBypassed)
{
	if (checkValidObject())
	{
		mp->setBypassed(shouldBeBypassed, sendNotification);
		mp->sendChangeMessage();
	}
}

String ScriptingObjects::ScriptingMidiProcessor::exportState()
{
	if (checkValidObject())
	{
		return ProcessorHelpers::getBase64String(mp, false, false);
	}

	return String();
}

void ScriptingObjects::ScriptingMidiProcessor::restoreState(String base64State)
{
	if (checkValidObject())
	{
		ProcessorHelpers::restoreFromBase64String(mp, base64State, false);
	}
}

String ScriptingObjects::ScriptingMidiProcessor::exportScriptControls()
{
	if (dynamic_cast<ProcessorWithScriptingContent*>(mp.get()) == nullptr)
	{
		reportScriptError("exportScriptControls can only be used on Script Processors");
	}

	if (checkValidObject())
	{
		return ProcessorHelpers::getBase64String(mp, false, true);
	}

	return String();
}

void ScriptingObjects::ScriptingMidiProcessor::restoreScriptControls(String base64Controls)
{
	if (dynamic_cast<ProcessorWithScriptingContent*>(mp.get()) == nullptr)
	{
		reportScriptError("restoreScriptControls can only be used on Script Processors");
	}

	if (checkValidObject())
	{
		ProcessorHelpers::restoreFromBase64String(mp, base64Controls, true);
	}
}

// ScriptingAudioSampleProcessor ==============================================================================================================

struct ScriptingObjects::ScriptingAudioSampleProcessor::Wrapper
{
	API_VOID_METHOD_WRAPPER_2(ScriptingAudioSampleProcessor, setAttribute);
    API_METHOD_WRAPPER_1(ScriptingAudioSampleProcessor, getAttribute);
	API_METHOD_WRAPPER_0(ScriptingAudioSampleProcessor, getNumAttributes);
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
    ADD_API_METHOD_1(getAttribute);
	ADD_API_METHOD_0(getNumAttributes);
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

float ScriptingObjects::ScriptingAudioSampleProcessor::getAttribute(int parameterIndex)
{
    if (checkValidObject())
    {
        return audioSampleProcessor->getAttribute(parameterIndex);
    }

	return 0.0f;
}

int ScriptingObjects::ScriptingAudioSampleProcessor::getNumAttributes() const
{
	if (checkValidObject())
	{
		return audioSampleProcessor->getNumParameters();
	}

	return 0;
}

void ScriptingObjects::ScriptingAudioSampleProcessor::setBypassed(bool shouldBeBypassed)
{
	if (checkValidObject())
	{
		audioSampleProcessor->setBypassed(shouldBeBypassed, sendNotification);
		audioSampleProcessor->sendChangeMessage();
	}
}


void ScriptingObjects::ScriptingAudioSampleProcessor::setFile(String fileName)
{
	if (checkValidObject())
	{
		ScopedLock sl(dynamic_cast<AudioSampleProcessor*>(audioSampleProcessor.get())->getFileLock());

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
	DYNAMIC_METHOD_WRAPPER(TimerObject, setTimerCallback, ARG(0));
};

ScriptingObjects::TimerObject::TimerObject(ProcessorWithScriptingContent *p) :
	DynamicScriptingObject(p),
	it(this)
{
	ADD_DYNAMIC_METHOD(startTimer);
	ADD_DYNAMIC_METHOD(stopTimer);
	ADD_DYNAMIC_METHOD(setTimerCallback);
}


ScriptingObjects::TimerObject::~TimerObject()
{
	it.stopTimer();
	removeProperty("callback");
}

void ScriptingObjects::TimerObject::timerCallback()
{
	var undefinedArgs;
	var thisObject(this);
	var::NativeFunctionArgs args(thisObject, &undefinedArgs, 0);

	Result r = Result::ok();

    auto engine = dynamic_cast<JavascriptMidiProcessor*>(getScriptProcessor())->getScriptEngine();
    
    engine->maximumExecutionTime = RelativeTime(0.5);
    
	auto callback = getProperty("callback");

	engine->callExternalFunction(callback, args, &r);

	if (r.failed())
	{
		stopTimer();
		debugError(getProcessor(), r.getErrorMessage());
	}
}

void ScriptingObjects::TimerObject::startTimer(int intervalInMilliSeconds)
{
	if (intervalInMilliSeconds > 10)
	{
		it.startTimer(intervalInMilliSeconds);
	}
	else
		throw String("Go easy on the timer");
}

void ScriptingObjects::TimerObject::stopTimer()
{
	it.stopTimer();
}

void ScriptingObjects::TimerObject::setTimerCallback(var callbackFunction)
{
	if (dynamic_cast<HiseJavascriptEngine::RootObject::FunctionObject*>(callbackFunction.getObject()))
	{
		setProperty("callback", callbackFunction);
	}
	else
		throw String("You need to pass in a function for the timer callback");
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

void ScriptingObjects::PathObject::rightClickCallback(const MouseEvent &e, Component* componentToNotify)
{
#if USE_BACKEND

	auto *editor = GET_BACKEND_ROOT_WINDOW(componentToNotify);

	PathPreviewComponent* content = new PathPreviewComponent(p);
	
	MouseEvent ee = e.getEventRelativeTo(editor);

	editor->getRootFloatingTile()->showComponentInRootPopup(content, editor, ee.getMouseDownPosition());

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
	API_VOID_METHOD_WRAPPER_3(PathObject, addArc);
	API_METHOD_WRAPPER_1(PathObject, getBounds);
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
	ADD_API_METHOD_3(addArc);
	ADD_API_METHOD_1(getBounds);
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

void ScriptingObjects::PathObject::addArc(var area, var fromRadians, var toRadians)
{
	auto rect = ApiHelpers::getRectangleFromVar(area);

	p.addArc(rect.getX(), rect.getY(), rect.getWidth(), rect.getHeight(), fromRadians, toRadians, true);
}

var ScriptingObjects::PathObject::getBounds(var scaleFactor)
{
	auto r = p.getBoundsTransformed(AffineTransform::scale(scaleFactor));

	Array<var> area;

	area.add(r.getX());
	area.add(r.getY());
	area.add(r.getWidth());
	area.add(r.getHeight());

	return var(area);
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
	API_VOID_METHOD_WRAPPER_3(GraphicsObject, drawAlignedText);
	API_VOID_METHOD_WRAPPER_1(GraphicsObject, setGradientFill);
	API_VOID_METHOD_WRAPPER_2(GraphicsObject, drawEllipse);
	API_VOID_METHOD_WRAPPER_1(GraphicsObject, fillEllipse);
	API_VOID_METHOD_WRAPPER_4(GraphicsObject, drawImage);
	API_VOID_METHOD_WRAPPER_3(GraphicsObject, drawDropShadow);
	API_VOID_METHOD_WRAPPER_2(GraphicsObject, addDropShadowFromAlpha);
	API_VOID_METHOD_WRAPPER_3(GraphicsObject, drawTriangle);
	API_VOID_METHOD_WRAPPER_2(GraphicsObject, fillTriangle);
	API_VOID_METHOD_WRAPPER_2(GraphicsObject, fillPath);
	API_VOID_METHOD_WRAPPER_3(GraphicsObject, drawPath);
	API_VOID_METHOD_WRAPPER_2(GraphicsObject, rotate);
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
	ADD_API_METHOD_3(drawAlignedText);
	ADD_API_METHOD_1(setGradientFill);
	ADD_API_METHOD_2(drawEllipse);
	ADD_API_METHOD_1(fillEllipse);
	ADD_API_METHOD_4(drawImage);
	ADD_API_METHOD_3(drawDropShadow);
	ADD_API_METHOD_2(addDropShadowFromAlpha);
	ADD_API_METHOD_3(drawTriangle);
	ADD_API_METHOD_2(fillTriangle);
	ADD_API_METHOD_2(fillPath);
	ADD_API_METHOD_3(drawPath);
	ADD_API_METHOD_2(rotate);
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

	currentFont = mc->getFontFromString(fontName, fontSize);

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

void ScriptingObjects::GraphicsObject::drawAlignedText(String text, var area, String alignment)
{
	initGraphics();

	Rectangle<float> r = getRectangleFromVar(area);

	Result re = Result::ok();

	auto just= ApiHelpers::getJustification(alignment, &re);

	if (re.failed())
		reportScriptError(re.getErrorMessage());


	g->setFont(currentFont);

	g->drawText(text, r, just);
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

void ScriptingObjects::GraphicsObject::drawImage(String imageName, var area, int /*xOffset*/, int yOffset)
{
	initGraphics();

	auto sc = dynamic_cast<ScriptingApi::Content::ScriptPanel*>(parent);

	const Image img = sc->getLoadedImage(imageName);

	if (img.isValid())
	{
		Rectangle<float> r = getRectangleFromVar(area);

        if(r.getWidth() != 0)
        {
            const double scaleFactor = (double)img.getWidth() / (double)r.getWidth();
            
            g->drawImage(img, (int)r.getX(), (int)r.getY(), (int)r.getWidth(), (int)r.getHeight(), 0, yOffset, (int)img.getWidth(), (int)((double)r.getHeight() * scaleFactor));
        }        
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

    Graphics g2(*imageToDraw);
    
#if JUCE_MAC || HISE_IOS
    const double scaleFactor = dynamic_cast<ScriptingApi::Content::ScriptPanel*>(parent)->parent->usesDoubleResolution() ? 2.0 : 1.0;
    
    // don't ask why...
    g2.addTransform(AffineTransform::scale((float)(1.0 / scaleFactor)));
#endif
    
	shadow.drawForImage(g2, *imageToDraw);
}

void ScriptingObjects::GraphicsObject::fillPath(var path, var area)
{
	if (PathObject* pathObject = dynamic_cast<PathObject*>(path.getObject()))
	{
		Path p = pathObject->getPath();

		if (area.isArray())
		{
			Rectangle<float> r = getRectangleFromVar(area);
			p.scaleToFit(r.getX(), r.getY(), r.getWidth(), r.getHeight(), false);
		}

		g->fillPath(p);
	}
}

void ScriptingObjects::GraphicsObject::drawPath(var path, var area, var thickness)
{
	if (PathObject* pathObject = dynamic_cast<PathObject*>(path.getObject()))
	{
		Path p = pathObject->getPath();
		
		if (area.isArray())
		{
			Rectangle<float> r = getRectangleFromVar(area);
			p.scaleToFit(r.getX(), r.getY(), r.getWidth(), r.getHeight(), false);
		}

		g->strokePath(p, PathStrokeType(thickness));
	}
}

void ScriptingObjects::GraphicsObject::rotate(var angleInRadian, var center)
{
	initGraphics();

	Point<float> c = getPointFromVar(center);

	auto a = AffineTransform::rotation(angleInRadian, c.getX(), c.getY());

	g->addTransform(a);
}

Point<float> ScriptingObjects::GraphicsObject::getPointFromVar(const var& data)
{
	Point<float>&& f = ApiHelpers::getPointFromVar(data, &rectangleResult);

	if (rectangleResult.failed()) reportScriptError(rectangleResult.getErrorMessage());

	return f;
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

struct ScriptingObjects::ScriptingMessageHolder::Wrapper
{
	API_VOID_METHOD_WRAPPER_1(ScriptingMessageHolder, setNoteNumber);
	API_VOID_METHOD_WRAPPER_1(ScriptingMessageHolder, setVelocity);
	API_VOID_METHOD_WRAPPER_1(ScriptingMessageHolder, setControllerNumber);
	API_VOID_METHOD_WRAPPER_1(ScriptingMessageHolder, setControllerValue);
	API_METHOD_WRAPPER_0(ScriptingMessageHolder, getNoteNumber);
	API_METHOD_WRAPPER_0(ScriptingMessageHolder, getVelocity);
	API_METHOD_WRAPPER_0(ScriptingMessageHolder, getControllerNumber);
	API_METHOD_WRAPPER_0(ScriptingMessageHolder, getControllerValue);
	API_VOID_METHOD_WRAPPER_1(ScriptingMessageHolder, ignoreEvent);
	API_METHOD_WRAPPER_0(ScriptingMessageHolder, getEventId);
	API_METHOD_WRAPPER_0(ScriptingMessageHolder, getChannel);
	API_VOID_METHOD_WRAPPER_1(ScriptingMessageHolder, setChannel);
	API_VOID_METHOD_WRAPPER_1(ScriptingMessageHolder, setTransposeAmount);
	API_METHOD_WRAPPER_0(ScriptingMessageHolder, getTransposeAmount);
	API_VOID_METHOD_WRAPPER_1(ScriptingMessageHolder, setCoarseDetune);
	API_METHOD_WRAPPER_0(ScriptingMessageHolder, getCoarseDetune);
	API_VOID_METHOD_WRAPPER_1(ScriptingMessageHolder, setFineDetune);
	API_METHOD_WRAPPER_0(ScriptingMessageHolder, getFineDetune);
	API_VOID_METHOD_WRAPPER_1(ScriptingMessageHolder, setGain);
	API_METHOD_WRAPPER_0(ScriptingMessageHolder, getGain);
	API_METHOD_WRAPPER_0(ScriptingMessageHolder, getTimestamp);
	API_METHOD_WRAPPER_0(ScriptingMessageHolder, dump);
};

ScriptingObjects::ScriptingMessageHolder::ScriptingMessageHolder(ProcessorWithScriptingContent* pwsc) :
	ConstScriptingObject(pwsc, (int)HiseEvent::Type::numTypes)
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
	ADD_API_METHOD_0(dump);

	addConstant("Empty", 0);
	addConstant("NoteOn", 1);
	addConstant("NoteOff", 2);
	addConstant("Controller", 3);
	addConstant("PitchBend", 4);
	addConstant("Aftertouch", 5);
	addConstant("AllNotesOff", 6);
	addConstant("SongPosition", 7);
	addConstant("MidiStart", 8);
	addConstant("MidiStop", 9);
	addConstant("VolumeFade", 10);
	addConstant("PitchFade", 11);
	addConstant("TimerEvent", 12);
}

int ScriptingObjects::ScriptingMessageHolder::getNoteNumber() const { return (int)e.getNoteNumber(); }
var ScriptingObjects::ScriptingMessageHolder::getControllerNumber() const { return (int)e.getControllerNumber(); }
var ScriptingObjects::ScriptingMessageHolder::getControllerValue() const { return (int)e.getControllerNumber(); }
int ScriptingObjects::ScriptingMessageHolder::getChannel() const { return (int)e.getChannel(); }
void ScriptingObjects::ScriptingMessageHolder::setChannel(int newChannel) { e.setChannel(newChannel); }
void ScriptingObjects::ScriptingMessageHolder::setNoteNumber(int newNoteNumber) { e.setNoteNumber(newNoteNumber); }
void ScriptingObjects::ScriptingMessageHolder::setVelocity(int newVelocity) { e.setVelocity((uint8)newVelocity); }
void ScriptingObjects::ScriptingMessageHolder::setControllerNumber(int newControllerNumber) { e.setControllerNumber(newControllerNumber);}
void ScriptingObjects::ScriptingMessageHolder::setControllerValue(int newControllerValue) { e.setControllerValue(newControllerValue); }
int ScriptingObjects::ScriptingMessageHolder::getVelocity() const { return e.getVelocity(); }
void ScriptingObjects::ScriptingMessageHolder::ignoreEvent(bool shouldBeIgnored /*= true*/) { e.ignoreEvent(shouldBeIgnored); }
int ScriptingObjects::ScriptingMessageHolder::getEventId() const { return (int)e.getEventId(); }
void ScriptingObjects::ScriptingMessageHolder::setTransposeAmount(int tranposeValue) { e.setTransposeAmount(tranposeValue); }
int ScriptingObjects::ScriptingMessageHolder::getTransposeAmount() const { return (int)e.getTransposeAmount(); }
void ScriptingObjects::ScriptingMessageHolder::setCoarseDetune(int semiToneDetune) { e.setCoarseDetune(semiToneDetune); }
int ScriptingObjects::ScriptingMessageHolder::getCoarseDetune() const { return (int)e.getCoarseDetune(); }
void ScriptingObjects::ScriptingMessageHolder::setFineDetune(int cents) { e.setFineDetune(cents); }
int ScriptingObjects::ScriptingMessageHolder::getFineDetune() const { return (int)e.getFineDetune(); }
void ScriptingObjects::ScriptingMessageHolder::setGain(int gainInDecibels) { e.setGain(gainInDecibels); }
int ScriptingObjects::ScriptingMessageHolder::getGain() const { return (int)e.getGain(); }
int ScriptingObjects::ScriptingMessageHolder::getTimestamp() const { return (int)e.getTimeStamp(); }
void ScriptingObjects::ScriptingMessageHolder::setTimestamp(int timestampSamples) { e.setTimeStamp((uint16)timestampSamples);}
void ScriptingObjects::ScriptingMessageHolder::addToTimestamp(int deltaSamples) { e.addToTimeStamp((int16)deltaSamples); }

String ScriptingObjects::ScriptingMessageHolder::dump() const
{
	String x;
	x << "Type: " << e.getTypeAsString() << ", ";
	x << "Number: " << String(e.getNoteNumber()) << ", ";
	x << "Value: " << String(e.getVelocity()) << ", ";
	x << "Channel: " << String(e.getChannel()) << ", ";
	x << "EventId: " << String(e.getEventId()) << ", ";
	x << "Timestamp: " << String(e.getTimeStamp()) << ", ";

	return x;
}



ApiHelpers::ModuleHandler::ModuleHandler(Processor* parent_) :
	parent(parent_)
{
#if USE_BACKEND

	auto console = parent->getMainController()->getConsoleHandler().getMainConsole();

	if (console)
		mainEditor = GET_BACKEND_ROOT_WINDOW(console);

#else
	mainEditor = nullptr;
#endif
}

ApiHelpers::ModuleHandler::~ModuleHandler()
{
	
}




bool ApiHelpers::ModuleHandler::removeModule(Processor* p)
{
	if (p->getMainController()->getKillStateHandler().getCurrentThread() == MainController::KillStateHandler::AudioThread)
	{
		throw String("Effects can't be removed from the audio thread!");
	}

	if (p != nullptr)
	{
		parent->getMainController()->getGlobalAsyncModuleHandler().removeAsync(p, mainEditor);
		return true;
	}
	else
		return false;
}

Processor* ApiHelpers::ModuleHandler::addModule(Chain* c, const String& type, const String& id, int index /*= -1*/)
{
	

	if (dynamic_cast<Processor*>(c)->getMainController()->getKillStateHandler().getCurrentThread() == MainController::KillStateHandler::AudioThread)
	{
		throw String("Modules can't be added from the audio thread!");
	}

	for (int i = 0; i < c->getHandler()->getNumProcessors(); i++)
	{
		if (c->getHandler()->getProcessor(i)->getId() == id)
		{
			return c->getHandler()->getProcessor(i);
		}
	}

	ScopedPointer<Processor> newProcessor = parent->getMainController()->createProcessor(c->getFactoryType(), type, id);

	if (newProcessor == nullptr)
		throw String("Module with type " + type + " could not be generated.");

	// Now we're safe...
	Processor* pFree = newProcessor.release();

	parent->getMainController()->getGlobalAsyncModuleHandler().addAsync(c, pFree, mainEditor, type, id, index);

	// will be owned by the job, than by the handler...
	return pFree;
}

Modulator* ApiHelpers::ModuleHandler::addAndConnectToGlobalModulator(Chain* c, Modulator* globalModulator, const String& modName)
{
	if (globalModulator == nullptr)
		throw String("Global Modulator does not exist");

	if (auto container = dynamic_cast<GlobalModulatorContainer*>(ProcessorHelpers::findParentProcessor(globalModulator, true)))
	{
		GlobalModulator* m = nullptr;

		if (dynamic_cast<VoiceStartModulator*>(globalModulator) != nullptr)
		{
			auto vMod = addModule(c, GlobalVoiceStartModulator::getClassType().toString(), modName);
			m = dynamic_cast<GlobalModulator*>(vMod);
		}
		else if (dynamic_cast<TimeVariantModulator*>(globalModulator) != nullptr)
		{
			auto tMod = addModule(c, GlobalTimeVariantModulator::getClassType().toString(), modName);
			m = dynamic_cast<GlobalModulator*>(tMod);
		}
		else
			throw String("Not a global modulator");

		if (m == nullptr)
			throw String("Global modulator can't be created");

		auto entry = container->getId() + ":" + globalModulator->getId();

		m->connectToGlobalModulator(entry);

		if (!m->isConnected())
		{
			throw String("Can't connect to global modulator");
		}

		auto returnMod = dynamic_cast<Modulator*>(m);

#if USE_BACKEND
		returnMod->sendChangeMessage();
#endif

		return returnMod;
	}
	else
		throw String("The modulator you passed in is not a global modulator. You must specify a modulator in a Global Modulator Container");
}
