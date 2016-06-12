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
	const int macroIndex = getProcessor()->getMainController()->getMacroManager().getMacroControlLearnMode();

	if(macroIndex != -1)
	{
		addToMacroController(macroIndex);

		GET_MACROCHAIN()->addControlledParameter(macroIndex, getProcessor()->getId(), parameter, name, getRange());
			
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

bool  MacroControlledObject::isLocked()
{
	if (!macroControlledComponentEnabled) return true;

#if STANDALONE_CONVOLUTION
    return false;
#endif
    
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
 
    
    
#if STANDALONE_CONVOLUTION
    
    NormalisableRange<float> range(-100.0f, 0.0f);
    range.skew = 5.0f;
    
    dynamic_cast<AudioProcessor*>(getProcessor()->getMainController())->setParameterNotifyingHost(parameter, range.convertTo0to1( (float)s->getValue() ));
    
#else
    
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
    
    #endif
}




void HiSlider::updateValue()
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

void HiSlider::setup(Processor *p, int parameter, const String &name)
{
	MacroControlledObject::setup(p, parameter, name);

	p->getMainController()->skin(*this);
	
	for(int i = 0; i < numModes; i++)
	{
		modeValues[i] = 0.0;
	}

	setDoubleClickReturnValue(true, (double)p->getDefaultValue(parameter));

	setName(name);
	
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
	}
	else
	{
#if USE_FRONTEND
		enableMidiLearnWithPopup();
#else
		removeParameterWithPopup();
#endif
	}
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
        
        ToggleButton::mouseDown(e);
    }
    else
    {
#if USE_FRONTEND
        enableMidiLearnWithPopup();
#else
        removeParameterWithPopup();
#endif
    }
}

void HiComboBox::setup(Processor *p, int parameter, const String &name)
{
	MacroControlledObject::setup(p, parameter, name);

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
        removeParameterWithPopup();
#endif
    }
}

void HiComboBox::updateValue()
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

	const int macroIndex = getProcessor()->getMainController()->getMacroManager().getMacroChain()->getMacroControlIndexForProcessorParameter(getProcessor(), parameter);

	if (macroIndex != -1 && !isReadOnly())
	{
		const float v = (float)getRange().convertTo0to1(index);

		GET_MACROCHAIN()->setMacroControl(macroIndex,v * 127.0f, sendNotification);
	}

	if(!checkLearnMode())
	{
		getProcessor()->setAttribute(parameter, (float)index, dontSendNotification);
	}
};


void HiToggleButton::setup(Processor *p, int parameter, const String &name)
{
	MacroControlledObject::setup(p, parameter, name);

	p->getMainController()->skin(*this);
}


void HiToggleButton::updateValue()
{
	const bool enabled = !isLocked();

	setEnabled(enabled);

	const bool state = getProcessor()->getAttribute(parameter) >= 0.5f;

	if(state != getToggleState()) setToggleState(state, notifyEditor);
}

void HiToggleButton::buttonClicked(Button *b)
{
	jassert(b == this);

#if STANDALONE_CONVOLUTION

    // Change this when you need another button
    dynamic_cast<AudioProcessor*>(getProcessor()->getMainController())->setParameterNotifyingHost(2, b->getToggleState());
    
#else
    
	const int index = GET_MACROCHAIN()->getMacroControlIndexForProcessorParameter(getProcessor(), parameter);


    
	if (index != -1 && !isReadOnly())
	{
		const float v = b->getToggleState() ? 127.0f : 0.0f;

		GET_MACROCHAIN()->setMacroControl(index,v, sendNotification);
	}


    
	if(!checkLearnMode())
	{
		getProcessor()->setAttribute(parameter, b->getToggleState(), dontSendNotification);
	}
    
#endif
    
}

StringArray TempoSyncer::tempoNames = StringArray();

float TempoSyncer::tempoFactors[numTempos];

#undef GET_MACROCHAIN