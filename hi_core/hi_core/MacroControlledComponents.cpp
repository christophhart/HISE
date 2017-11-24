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
*   which must be separately licensed for closed source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

namespace hise { using namespace juce;

#define GET_MACROCHAIN() (getProcessor()->getMainController()->getMacroManager().getMacroChain())

int MacroControlledObject::getMacroIndex() const
{
	if(getProcessor() != nullptr)
	{
		return GET_MACROCHAIN()->getMacroControlIndexForProcessorParameter(getProcessor(), parameter);
	}
	else return -1;
};

bool MacroControlledObject::checkLearnMode()
{
	const int currentlyActiveLearnIndex = getProcessor()->getMainController()->getMacroManager().getMacroControlLearnMode();

	if(currentlyActiveLearnIndex != -1)
	{
		addToMacroController(currentlyActiveLearnIndex);

		GET_MACROCHAIN()->addControlledParameter(currentlyActiveLearnIndex, getProcessor()->getId(), parameter, name, getRange());
			
		return true;
	}

	return false;
}

void MacroControlledObject::removeParameterWithPopup()
{
	if (macroIndex != -1)
	{
		ScopedPointer<PopupLookAndFeel> plaf = new PopupLookAndFeel();

		PopupMenu m;

		m.setLookAndFeel(plaf);

		m.addItem(1, "Remove Macro control");

		const int result = m.show();

		if (result == 1)
		{
			getProcessor()->getMainController()->getMacroManager().getMacroChain()->getMacroControlData(macroIndex)->removeParameter(name, getProcessor());
			removeFromMacroController();
		}
	}
}

void MacroControlledObject::enableMidiLearnWithPopup()
{
	if (!canBeMidiLearned())
		return;

	MidiControllerAutomationHandler *handler = getProcessor()->getMainController()->getMacroManager().getMidiControlAutomationHandler();

	const int midiController = handler->getMidiControllerNumber(processor, parameter);
	const bool learningActive = handler->isLearningActive(processor, parameter);

	PopupLookAndFeel plaf;
	PopupMenu m;

	m.setLookAndFeel(&plaf);

	m.addItem(1, "Learn MIDI CC", true, learningActive);
	
	if (midiController != -1)
	{
		m.addItem(2, "Remove CC " + String(midiController));
	}
	
	const int result = m.show();

	if (result == 1)
	{
		if (!learningActive)
		{
			NormalisableRange<double> rangeWithSkew = getRange();

			if (HiSlider *slider = dynamic_cast<HiSlider*>(this))
			{
				rangeWithSkew.skew = slider->getSkewFactor();
			}

			handler->addMidiControlledParameter(processor, parameter, rangeWithSkew, getMacroIndex());
		}
		else
		{
			handler->deactivateMidiLearning();
		}
	}
	else if (result == 2)
	{
		handler->removeMidiControlledParameter(processor, parameter);
	}
}

void MacroControlledObject::setAttributeWithUndo(float newValue, bool useCustomOldValue/*=false*/, float customOldValue/*=-1.0f*/)
{
	if (useUndoManagerForEvents)
	{
		const float oldValue = useCustomOldValue ? customOldValue : getProcessor()->getAttribute(parameter);

		UndoableControlEvent* newEvent = new UndoableControlEvent(getProcessor(), parameter, oldValue, newValue);

		String undoName = getProcessor()->getId();
		undoName << " - " << getProcessor()->getIdentifierForParameterIndex(parameter).toString() << ": " << String(newValue, 2);

		getProcessor()->getMainController()->getControlUndoManager()->beginNewTransaction(undoName);
		getProcessor()->getMainController()->getControlUndoManager()->perform(newEvent);
	}
	else
	{
		getProcessor()->setAttribute(parameter, newValue, dontSendNotification);
	}
}

bool  MacroControlledObject::isLocked()
{
	if (!macroControlledComponentEnabled) return true;


    
	const int index = GET_MACROCHAIN()->getMacroControlIndexForProcessorParameter(getProcessor(), parameter);

	if(index == -1) return false;

	return isReadOnly();
}

bool  MacroControlledObject::isReadOnly()
{
	const int index = GET_MACROCHAIN()->getMacroControlIndexForProcessorParameter(getProcessor(), parameter);

	const MacroControlBroadcaster::MacroControlledParameterData *data = GET_MACROCHAIN()->getMacroControlData(index)->getParameterWithProcessorAndName(getProcessor(), name);

	if(data == nullptr) return true;

	const bool ro = data->isReadOnly();

	return ro;
}


void HiSlider::sliderValueChanged(Slider *s)
{
	jassert(s == this);
 

	const int index = GET_MACROCHAIN()->getMacroControlIndexForProcessorParameter(getProcessor(), parameter);


    
	if (index != -1 && !isReadOnly())
	{
		const float v = (float)normRange.convertTo0to1(s->getValue());

		GET_MACROCHAIN()->setMacroControl(index,v * 127.0f, sendNotification);
	}


    
	if(!checkLearnMode())
	{
		if (getSliderStyle() == Slider::TwoValueHorizontal)
		{
			//setMinAndMaxValues(minValue, maxValue, dontSendNotification);
		}
		else
		{
			modeValues[mode] = s->getValue();

			getProcessor()->setAttribute(parameter, (float)s->getValue(), dontSendNotification);
		}
	}
}




void HiSlider::sliderDragStarted(Slider* s)
{
	dragStartValue = s->getValue();

	Point<int> o;

	startTouch(o);
}

void HiSlider::sliderDragEnded(Slider* s)
{
	abortTouch();
	setAttributeWithUndo((float)s->getValue(), true, (float)dragStartValue);
}

void HiSlider::updateValue(NotificationType /*sendAttributeChange*/)
{

	const bool enabled = !isLocked();

	setEnabled(enabled);

	numberTag->setNumber(enabled ? 0 : getMacroIndex()+1);
	

	const double value = (double)getProcessor()->getAttribute(parameter);

	modeValues[mode] = value;

	if (getSliderStyle() == Slider::TwoValueHorizontal)
	{
		//setMinAndMaxValues(minValue, maxValue, dontSendNotification);
	}
	else
	{
		setValue(modeValues[mode], dontSendNotification);
	}
	
}

void HiSlider::setup(Processor *p, int parameterIndex, const String &parameterName)
{
	MacroControlledObject::setup(p, parameterIndex, parameterName);

	p->getMainController()->skin(*this);
	
	for(int i = 0; i < numModes; i++)
	{
		modeValues[i] = 0.0;
	}

	setDoubleClickReturnValue(true, (double)p->getDefaultValue(parameterIndex));

	setName(parameterName);
}

void HiSlider::setLookAndFeelOwned(LookAndFeel *laf_)
{
	laf = laf_;
	setLookAndFeel(laf);
}

void HiSlider::mouseDown(const MouseEvent &e)
{
	if (e.mods.isLeftButtonDown())
	{
        PresetHandler::setChanged(getProcessor());
        
		checkLearnMode();
		Slider::mouseDown(e);
		startTouch(e.getMouseDownPosition());
	}
	else
	{
#if USE_FRONTEND
		enableMidiLearnWithPopup();
#else
		const bool isOnPreview = findParentComponentOfClass<FloatingTilePopup>() != nullptr;

		if (isOnPreview)
			enableMidiLearnWithPopup();

		else
			removeParameterWithPopup();
#endif
	}
}

void HiSlider::mouseDrag(const MouseEvent& e)
{
	setDragDistance((float)e.getDistanceFromDragStart());
	Slider::mouseDrag(e);
}

void HiSlider::mouseUp(const MouseEvent& e)
{
	abortTouch();
	Slider::mouseUp(e);
}

void HiSlider::touchAndHold(Point<int> /*downPosition*/)
{
#if USE_FRONTEND
	enableMidiLearnWithPopup();
#else

	const bool isOnPreview = findParentComponentOfClass<FloatingTilePopup>() != nullptr;

	if (isOnPreview)
		enableMidiLearnWithPopup();
	else
		removeParameterWithPopup();
#endif
}

void HiToggleButton::setLookAndFeelOwned(LookAndFeel *laf_)
{
	laf = laf_;
	setLookAndFeel(laf);
}

void HiToggleButton::mouseDown(const MouseEvent &e)
{

    if(e.mods.isLeftButtonDown())
    {
        checkLearnMode();
        
        PresetHandler::setChanged(getProcessor());
        
		if (isMomentary)
		{
			setToggleState(true, sendNotification);
		}
		else
		{
			ToggleButton::mouseDown(e);
		}

		if (popupData.isObject())
		{
			if (findParentComponentOfClass<FloatingTilePopup>() == nullptr) // Deactivate this function in popups...
			{
				if (currentPopup.getComponent() != nullptr)
				{
					findParentComponentOfClass<FloatingTile>()->showComponentInRootPopup(nullptr, this, popupPosition.getPosition());
					currentPopup = nullptr;
				}
				else
				{
#if USE_BACKEND
					auto mc = GET_BACKEND_ROOT_WINDOW(this)->getBackendProcessor();
#else
					auto mc = dynamic_cast<MainController*>(findParentComponentOfClass<FrontendProcessorEditor>()->getAudioProcessor());
#endif

					FloatingTile *t = new FloatingTile(mc, nullptr, popupData);
					t->setOpaque(false);

					t->setName(t->getCurrentFloatingPanel()->getBestTitle());

					t->setSize(popupPosition.getWidth(), popupPosition.getHeight());
					currentPopup = findParentComponentOfClass<FloatingTile>()->showComponentInRootPopup(t, this, popupPosition.getPosition());
				}

			}
		}
    }
    else
    {
#if USE_FRONTEND
        enableMidiLearnWithPopup();
#else

		const bool isOnPreview = findParentComponentOfClass<FloatingTilePopup>() != nullptr;

		if (isOnPreview)
			enableMidiLearnWithPopup();
		else
			removeParameterWithPopup();
#endif
    }
}

void HiToggleButton::mouseUp(const MouseEvent& e)
{
	if (isMomentary)
	{
		setToggleState(false, sendNotification);
	}
	else
	{
		ToggleButton::mouseUp(e);
	}
}

void HiComboBox::setup(Processor *p, int parameterIndex, const String &parameterName)
{
	MacroControlledObject::setup(p, parameterIndex, parameterName);

	p->getMainController()->skin(*this);
}

void HiComboBox::mouseDown(const MouseEvent &e)
{
    if(e.mods.isLeftButtonDown())
    {
        checkLearnMode();
        
        PresetHandler::setChanged(getProcessor());
        
        ComboBox::mouseDown(e);
    }
    else
    {
#if USE_FRONTEND
        
        enableMidiLearnWithPopup();
        
#else
		const bool isOnPreview = findParentComponentOfClass<FloatingTilePopup>() != nullptr;

		if (isOnPreview)
			enableMidiLearnWithPopup();
		else
			removeParameterWithPopup();
#endif
    }
}

void HiComboBox::updateValue(NotificationType /*sendAttributeChange*/)
{
	const bool enabled = !isLocked();

	if(enabled) numberTag->setNumber(0);

	setEnabled(enabled);

	setSelectedId(roundFloatToInt(getProcessor()->getAttribute(parameter)), dontSendNotification);
}

void HiComboBox::comboBoxChanged(ComboBox *c)
{
	const int index = c->getSelectedId();
    
	if(index == 0) return;

	const int thisMacroIndex = getProcessor()->getMainController()->getMacroManager().getMacroChain()->getMacroControlIndexForProcessorParameter(getProcessor(), parameter);

	if (thisMacroIndex != -1 && !isReadOnly())
	{
		const float v = (float)getRange().convertTo0to1(index);

		GET_MACROCHAIN()->setMacroControl(thisMacroIndex,v * 127.0f, sendNotification);
	}

	if(!checkLearnMode())
	{
		setAttributeWithUndo((float)index);

		//getProcessor()->setAttribute(parameter, (float)index, dontSendNotification);
	}
};


void HiToggleButton::setup(Processor *p, int parameterIndex, const String &parameterName)
{
	MacroControlledObject::setup(p, parameterIndex, parameterName);

	p->getMainController()->skin(*this);
}


void HiToggleButton::updateValue(NotificationType /*sendAttributeChange*/)
{
	const bool enabled = !isLocked();

	setEnabled(enabled);

	const bool state = getProcessor()->getAttribute(parameter) >= 0.5f;

	if(state != getToggleState()) setToggleState(state, notifyEditor);
}

void HiToggleButton::buttonClicked(Button *b)
{
	jassert(b == this);
    
	const int index = GET_MACROCHAIN()->getMacroControlIndexForProcessorParameter(getProcessor(), parameter);


    
	if (index != -1 && !isReadOnly())
	{
		const float v = b->getToggleState() ? 127.0f : 0.0f;

		GET_MACROCHAIN()->setMacroControl(index,v, sendNotification);
	}


	if(!checkLearnMode())
	{
		const float newValue = b->getToggleState() ? 1.0f : 0.0f;
		
		setAttributeWithUndo(newValue);
	}
}

StringArray TempoSyncer::tempoNames = StringArray();

float TempoSyncer::tempoFactors[numTempos];

#undef GET_MACROCHAIN

MacroControlledObject::UndoableControlEvent::UndoableControlEvent(Processor* p_, int parameterIndex_, float oldValue_, float newValue_) :
	processor(p_),
	parameterIndex(parameterIndex_),
	oldValue(oldValue_),
	newValue(newValue_)
{

}

bool MacroControlledObject::UndoableControlEvent::perform()
{
	if (processor.get() != nullptr)
	{
		processor->setAttribute(parameterIndex, newValue, sendNotification);
		return true;
	}
	else return false;
}

bool MacroControlledObject::UndoableControlEvent::undo()
{
	if (processor.get() != nullptr)
	{
		processor->setAttribute(parameterIndex, oldValue, sendNotification);
		return true;
	}
	else return false;
}

} // namespace hise