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
		GET_MACROCHAIN()->addControlledParameter(currentlyActiveLearnIndex, getProcessor()->getId(), parameter, name, getRange());
			
		return true;
	}

	return false;
}


void MacroControlledObject::enableMidiLearnWithPopup()
{
	if (!canBeMidiLearned())
		return;

#if USE_BACKEND
	auto isOnHiseModuleUI = dynamic_cast<Component*>(this)->findParentComponentOfClass<ProcessorEditorBody>() != nullptr;
#else
	auto isOnHiseModuleUI = false;
#endif

	MidiControllerAutomationHandler *handler = getProcessor()->getMainController()->getMacroManager().getMidiControlAutomationHandler();

	auto mods = ProcessorHelpers::getListOfAllGlobalModulators(getProcessor()->getMainController()->getMainSynthChain());

	const int midiController = handler->getMidiControllerNumber(processor, parameter);
	const bool learningActive = handler->isLearningActive(processor, parameter);

	enum Commands
	{
		Learn = 1,
		Remove,
		AddMPE,
		RemoveMPE,
		RemoveMacroControl,
		AddMacroControlOffset = 50,
		GlobalModAddOffset = 100,
		GlobalModRemoveOffset = 200,
		MidiOffset = 300,
		numCommands
	};

	PopupMenu m;

	auto mc = getProcessor()->getMainController();
	auto& plaf = mc->getGlobalLookAndFeel();

	m.setLookAndFeel(&plaf);


	if (!isOnHiseModuleUI)
	{
		m.addItem(Learn, "Learn MIDI CC", true, learningActive);
		
		auto value = getProcessor()->getMainController()->getMacroManager().getMidiControlAutomationHandler()->getMidiControllerNumber(processor, parameter);

		PopupMenu s;
		for (int i = 1; i < 127; i++)
			s.addItem(i + MidiOffset, "CC #" + String(i), true, i == value);

		m.addSubMenu("Assign MIDI CC", s, true);
	}
		

	auto& data = getProcessor()->getMainController()->getMacroManager().getMidiControlAutomationHandler()->getMPEData();

	const String possibleName = dynamic_cast<Component*>(this)->getName() + "MPE";

	auto possibleConnection = dynamic_cast<MPEModulator*>(ProcessorHelpers::getFirstProcessorWithName(getProcessor()->getMainController()->getMainSynthChain(), possibleName));

	if (data.isMpeEnabled() && possibleConnection != nullptr)
	{
		const bool active = !data.getListOfUnconnectedModulators(false).contains(possibleName);

		if (active)
		{
			m.addItem(RemoveMPE, "Remove MPE Gesture");
		}
		else
			m.addItem(AddMPE, "Add MPE Gesture");
	}

	if (midiController != -1)
	{
		m.addItem(Remove, "Remove CC " + String(midiController));
	}

	if (macroIndex != -1)
	{
		m.addItem(RemoveMacroControl, "Remove Macro control");
	}
	else
	{
		auto& mm = getProcessor()->getMainController()->getMacroManager();
		auto macroChain = mm.getMacroChain();

		if (mm.isMacroEnabledOnFrontend())
		{
			for (int i = 0; i < 8; i++)
			{
				auto name = macroChain->getMacroControlData(i)->getMacroName();

				if (name.isNotEmpty())
				{
					m.addItem((int)AddMacroControlOffset + i, "Add to " + name);
				}
			}
		}
	}

	NormalisableRange<double> rangeWithSkew = getRange();

	if (HiSlider *slider = dynamic_cast<HiSlider*>(this))
	{
		rangeWithSkew.skew = slider->getSkewFactor();
	}

	const int result = m.show();

	if (result == Learn)
	{
		if (!learningActive)
		{
			handler->addMidiControlledParameter(processor, parameter, rangeWithSkew, getMacroIndex());
		}
		else
		{
			handler->deactivateMidiLearning();
		}
	}
	else if (result == Remove)
	{
		handler->removeMidiControlledParameter(processor, parameter, sendNotification);
	}
	else if (result == AddMPE)
	{
		data.addConnection(possibleConnection);
	}
	else if (result == RemoveMPE)
	{
		data.removeConnection(possibleConnection);
	}
	else if (result == RemoveMacroControl)
	{
		getProcessor()->getMainController()->getMacroManager().getMacroChain()->getMacroControlData(macroIndex)->removeParameter(name, getProcessor());
		initMacroControl(sendNotification);
	}
	else if (result >= MidiOffset)
	{
		auto number = result - MidiOffset;

		auto mHandler = getProcessor()->getMainController()->getMacroManager().getMidiControlAutomationHandler();
		
		mHandler->deactivateMidiLearning();
		mHandler->removeMidiControlledParameter(processor, parameter, sendNotificationAsync);
		mHandler->addMidiControlledParameter(processor, parameter, rangeWithSkew, -1);
		mHandler->setUnlearndedMidiControlNumber(number, sendNotificationAsync);
	}
	else if (result >= AddMacroControlOffset)
	{
		int macroIndex = result - AddMacroControlOffset;

		getProcessor()->getMainController()->getMacroManager().getMacroChain()->getMacroControlData(macroIndex)->addParameter(getProcessor(), parameter, getName(), rangeWithSkew);
		initMacroControl(sendNotification);
	}
	
}


void MacroControlledObject::setAttributeWithUndo(float newValue, bool useCustomOldValue/*=false*/, float customOldValue/*=-1.0f*/)
{
	if (useUndoManagerForEvents)
	{
		const float oldValue = useCustomOldValue ? customOldValue : getProcessor()->getAttribute(parameter);

		UndoableControlEvent* newEvent = new UndoableControlEvent(getProcessor(), parameter, oldValue, newValue);

		getProcessor()->getMainController()->getControlUndoManager()->perform(newEvent);
	}
	else
	{
		getProcessor()->setAttribute(parameter, newValue, dontSendNotification);
	}
}

void MacroControlledObject::macroConnectionChanged(int macroIndex, Processor* p, int parameterIndex, bool wasAdded)
{
	if (getProcessor() == p && parameter == parameterIndex)
	{
		if (wasAdded)
			addToMacroController(macroIndex);
		else
			removeFromMacroController();

		if (auto c = dynamic_cast<Component*>(this))
			c->repaint();

		updateValue(dontSendNotification);
	}
}

MacroControlledObject::~MacroControlledObject()
{
	if (auto p = getProcessor())
	{
		p->getMainController()->getMainSynthChain()->removeMacroConnectionListener(this);
	}
}

bool MacroControlledObject::isConnectedToModulator() const
{
	auto chain = getProcessor()->getMainController()->getMainSynthChain();

	if (auto container = ProcessorHelpers::getFirstProcessorWithType<GlobalModulatorContainer>(chain))
	{
		return container->getModulatorForControlledParameter(getProcessor(), parameter) != nullptr;
	}

	return false;
	
}

void MacroControlledObject::setup(Processor *p, int parameter_, const String &name_)
{
	processor = p;
	parameter = parameter_;
	name = name_;

	initMacroControl(dontSendNotification);

	slaf = new ScriptingObjects::ScriptedLookAndFeel::Laf(p->getMainController());
	numberTag->setLookAndFeel(slaf.get());
	
	

	p->getMainController()->getMainSynthChain()->addMacroConnectionListener(this);

	auto mIndex = p->getMainController()->getMainSynthChain()->getMacroControlIndexForProcessorParameter(p, parameter);

	if (mIndex != -1)
		addToMacroController(mIndex);
	else
		removeFromMacroController();
}

void MacroControlledObject::initMacroControl(NotificationType notify)
{
#if 0
	if (getProcessor() == nullptr)
		return;


	auto macroChain = getProcessor()->getMainController()->getMacroManager().getMacroChain();
	
	auto mIndex = -1;

	for (int i = 0; i < 8; i++)
	{
		if (auto p = macroChain->getMacroControlData(i)->getParameterWithProcessorAndIndex(getProcessor(), parameter))
		{
			mIndex = i;
			break;
		}
	}
	
	if (mIndex >= 0)
		addToMacroController(mIndex);
	else

		removeFromMacroController();

	if (notify == sendNotification)
	{
		getProcessor()->getMainController()->getMainSynthChain()->sendChangeMessage();

		
	}
#endif
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

HiSlider::HiSlider(const String &name) :
	Slider(name),
	MacroControlledObject(),
	mode(numModes),
	displayValue(1.0f),
	useModulatedRing(false)
{
	addChildComponent(numberTag);

	setScrollWheelEnabled(true);

	FloatVectorOperations::clear(modeValues, numModes);
	addListener(this);
	setWantsKeyboardFocus(false);

	setColour(HiseColourScheme::ComponentFillTopColourId, Colour(0x66333333));
	setColour(HiseColourScheme::ComponentFillBottomColourId, Colour(0xfb111111));
	setColour(HiseColourScheme::ComponentOutlineColourId, Colours::white.withAlpha(0.3f));
	setColour(TextEditor::highlightColourId, Colour(SIGNAL_COLOUR).withAlpha(0.5f));
	setColour(TextEditor::ColourIds::focusedOutlineColourId, Colour(SIGNAL_COLOUR));
}

void HiSlider::mouseDown(const MouseEvent &e)
{
	if (e.mods.isLeftButtonDown())
	{
		if (e.mods.isShiftDown() && getWidth() > 25)
		{
			addAndMakeVisible(inputLabel = new TextEditor());
			
			inputLabel->centreWithSize(getWidth(), 20);
			inputLabel->addListener(this);
			
			inputLabel->setColour(TextEditor::ColourIds::backgroundColourId, Colours::black.withAlpha(0.6f));
			inputLabel->setColour(TextEditor::ColourIds::textColourId, Colours::white.withAlpha(0.8f));
			inputLabel->setColour(TextEditor::ColourIds::highlightedTextColourId, Colours::black);
			inputLabel->setColour(TextEditor::ColourIds::highlightColourId, Colours::white.withAlpha(0.5f));
			inputLabel->setColour(TextEditor::ColourIds::focusedOutlineColourId, Colours::transparentBlack);
			inputLabel->setColour(CaretComponent::ColourIds::caretColourId, Colours::white);

			inputLabel->setFont(GLOBAL_BOLD_FONT());
			inputLabel->setBorder(BorderSize<int>());
			inputLabel->setJustification(Justification::centred);
			
			inputLabel->setText(getTextFromValue(getValue()), dontSendNotification);
			inputLabel->selectAll();
			inputLabel->grabKeyboardFocus();

		}
		else
		{
			PresetHandler::setChanged(getProcessor());

			checkLearnMode();

			if (!isConnectedToModulator())
			{
				Slider::mouseDown(e);
				startTouch(e.getMouseDownPosition());
			}
		}
		
	}
	else
	{
		enableMidiLearnWithPopup();
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
	enableMidiLearnWithPopup();
}

void HiSlider::updateValueFromLabel(bool shouldUpdateValue)
{
	if (inputLabel == nullptr)
		return;

	auto doubleValue = getValueFromText(inputLabel->getText());

	if (shouldUpdateValue && (getRange().getRange().contains(doubleValue) || doubleValue == getMaximum()))
	{
		setAttributeWithUndo((float)doubleValue);
	}

	inputLabel->removeListener(this);
	inputLabel = nullptr;
}

void HiSlider::textEditorFocusLost(TextEditor&)
{
	updateValueFromLabel(true);
}

void HiSlider::textEditorReturnKeyPressed(TextEditor&)
{
	updateValueFromLabel(true);
}

void HiSlider::textEditorEscapeKeyPressed(TextEditor&)
{
	updateValueFromLabel(false);
}

void HiToggleButton::setLookAndFeelOwned(LookAndFeel *laf_)
{
	laf = laf_;
	setLookAndFeel(laf);
}

void HiToggleButton::touchAndHold(Point<int> /*downPosition*/)
{
	enableMidiLearnWithPopup();
}
    
void HiToggleButton::mouseDown(const MouseEvent &e)
{

    if(e.mods.isLeftButtonDown())
    {
        checkLearnMode();
        
        PresetHandler::setChanged(getProcessor());
        
        startTouch(e.getMouseDownPosition());
        
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
		enableMidiLearnWithPopup();
    }
}

void HiToggleButton::mouseUp(const MouseEvent& e)
{
    abortTouch();
    
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
        
        startTouch(e.getMouseDownPosition());
        
        
        ComboBox::mouseDown(e);
    }
    else
    {
		enableMidiLearnWithPopup();
    }
}

void HiComboBox::touchAndHold(Point<int> /*downPosition*/)
{
	enableMidiLearnWithPopup();
}
    
void HiComboBox::updateValue(NotificationType /*sendAttributeChange*/)
{
	const bool enabled = !isLocked();

	if(enabled) numberTag->setNumber(0);

	setEnabled(enabled);

	setSelectedId(roundToInt(getProcessor()->getAttribute(parameter)), dontSendNotification);

	//addItemsToMenu(*getRootMenu());
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
