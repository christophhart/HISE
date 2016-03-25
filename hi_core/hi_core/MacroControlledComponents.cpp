
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
		checkLearnMode();
		Slider::mouseDown(e);
	}
	else
	{
		removeParameterWithPopup();
	}
}

void HiToggleButton::setLookAndFeelOwned(LookAndFeel *laf_)
{
	laf = laf_;
	setLookAndFeel(laf);
}

void HiComboBox::setup(Processor *p, int parameter, const String &name)
{
	MacroControlledObject::setup(p, parameter, name);

	p->getMainController()->skin(*this);
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