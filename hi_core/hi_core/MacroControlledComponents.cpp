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
		if (customId.isValid())
			return GET_MACROCHAIN()->getMacroControlIndexForCustomAutomation(customId);
		else
			return GET_MACROCHAIN()->getMacroControlIndexForProcessorParameter(getProcessor(), parameter);
	}
	else return -1;
};

bool MacroControlledObject::checkLearnMode()
{
#if USE_BACKEND

	if (getProcessor() == nullptr)
		return false;

	auto b = getProcessor()->getMainController()->getScriptComponentEditBroadcaster();

	if (auto l = b->getCurrentlyLearnedComponent())
	{
		LearnData ld;
		ld.processorId = getProcessor()->getId();
		ld.parameterId = getProcessor()->getIdentifierForParameterIndex(parameter).toString();
		ld.range = getRange();
		ld.value = getProcessor()->getAttribute(parameter);
		ld.name = name;

		if (auto s = dynamic_cast<HiSlider*>(this))
		{
			ld.mode = s->getModeId();
		}
		
		else if (auto cb = dynamic_cast<HiComboBox*>(this))
		{
			for (int i = 0; i < cb->getNumItems(); i++)
				ld.items.add(cb->getItemText(i));
		}

		b->setLearnData(ld);
		return true;
	}

#endif


	const int currentlyActiveLearnIndex = getProcessor()->getMainController()->getMacroManager().getMacroControlLearnMode();

	if(currentlyActiveLearnIndex != -1)
	{
		String suffix;

		if(auto asSlider = dynamic_cast<Slider*>(this))
			suffix = asSlider->getTextValueSuffix();

		GET_MACROCHAIN()->addControlledParameter(currentlyActiveLearnIndex, getProcessor()->getId(), parameter, name, getValueToTextConverter(), getRange());
			
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

	auto parameterToUse = getAutomationIndex();

	auto midiController = handler->getMidiControllerNumber(processor, parameterToUse);
	const bool learningActive = handler->isLearningActive(processor, parameterToUse);

	enum Commands
	{
		Learn = 1,
		Remove,
		AddMPE,
		RemoveMPE,
		RemoveMacroControl,
		AddMacroControlOffset = 50,
		MidiOffset = 400,
		numCommands
	};

	PopupMenu m;

	auto mc = getProcessor()->getMainController();
	LookAndFeel* plaf = &mc->getGlobalLookAndFeel();


	auto thisLaf = &asComponent()->getLookAndFeel();
	auto css = dynamic_cast<simple_css::StyleSheetLookAndFeel*>(thisLaf);

	if(css == nullptr)
	{
		if(auto pf = dynamic_cast<ScriptingObjects::ScriptedLookAndFeel::LafBase*>(thisLaf))
			css = pf->getStyleSheetLookAndFeel();
	}

	if(css != nullptr)
		plaf = css;

	m.setLookAndFeel(plaf);

	auto ccName = handler->getCCName();

	if (!isOnHiseModuleUI && getMacroIndex() == -1)
	{
		auto addNumbersToMenu = [&](PopupMenu& mToUse)
		{
			auto value = handler->getMidiControllerNumber(processor, parameterToUse);

			for (int i = 0; i < 128; i++)
			{
				if (handler->shouldAddControllerToPopup(i))
					mToUse.addItem(i + MidiOffset, handler->getControllerName(i), handler->isMappable(i), value.isValid() && i == value.ccNumber);
			}
		};

		auto isUsingCustomModel = getProcessor()->getMainController()->getUserPresetHandler().getNumCustomAutomationData() != 0;

		auto showMidi = !isUsingCustomModel || (isUsingCustomModel && customId.isValid());
		
		if(showMidi)
		{
			if (handler->hasSelectedControllerPopupNumbers())
			{
				m.addSectionHeader("Assign " + ccName);
				addNumbersToMenu(m);
			}
			else
			{
				m.addItem(Learn, "Learn " + ccName, true, learningActive);
				PopupMenu s;
				addNumbersToMenu(s);
				m.addSubMenu("Assign " + ccName, s, true);
			}
		}
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

	if (midiController.isValid())
	{
		m.addItem(Remove, "Remove " + handler->getControllerName(midiController.ccNumber));
	}

	if (macroIndex != -1)
	{
		auto& mm = getProcessor()->getMainController()->getMacroManager();
		auto macroChain = mm.getMacroChain();

		if (mm.isMacroEnabledOnFrontend())
		{
			auto name = macroChain->getMacroControlData(macroIndex)->getMacroName();

			m.addItem(RemoveMacroControl, "Remove " + name);
		}
		else
		{
			m.addItem(RemoveMacroControl, "Remove Macro control");
		}
	}
	else
	{
		auto& mm = getProcessor()->getMainController()->getMacroManager();
		auto macroChain = mm.getMacroChain();

		if (mm.isMacroEnabledOnFrontend())
		{
			auto useMacrosAsParameter = HISE_GET_PREPROCESSOR(mc, HISE_MACROS_ARE_PLUGIN_PARAMETERS);
			auto numMacros = HISE_GET_PREPROCESSOR(mc, HISE_NUM_MACROS);

			auto title = useMacrosAsParameter ? "Assign Automation" : "Assign Macro";

			PopupMenu sub;
			auto mToUse = &m;

			static bool useSubMenu = numMacros > 8;

			if (useSubMenu)
				mToUse = &sub;
			else
				m.addSectionHeader(title);
				
			for (int i = 0; i < numMacros; i++)
			{
				auto name = macroChain->getMacroControlData(i)->getMacroName();

				if (name.isNotEmpty())
				{
					mToUse->addItem((int)AddMacroControlOffset + i, "Connect to " + name);
				}
			}

			if (useSubMenu)
			{
				m.addSeparator();
				m.addSubMenu(title, sub);
			}
		}
	}

	if (modulationData != nullptr)
	{
		modulationData->addToPopupMenu(this, m);
	}

	NormalisableRange<double> rangeWithSkew = getRange();

	if (HiSlider *slider = dynamic_cast<HiSlider*>(this))
	{
		rangeWithSkew.skew = slider->getSkewFactor();
	}

    auto result = PopupLookAndFeel::showAtComponent(m, dynamic_cast<Component*>(this), false);

	if(modulationData != nullptr && modulationData->onPopupMenuResult(this, result))
	{
		return;
	}

	if (result == Learn)
	{
		if (!learningActive)
		{
			handler->addMidiControlledParameter(processor, parameterToUse, rangeWithSkew, getValueToTextConverter(), getMacroIndex());
		}
		else
		{
			handler->deactivateMidiLearning();
		}
	}
	else if (result == Remove)
	{
		handler->removeMidiControlledParameter(processor, parameterToUse, sendNotification);
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
		auto nameToUse = name;

		if (customId.isValid())
			nameToUse = customId.toString();

		getProcessor()->getMainController()->getMacroManager().getMacroChain()->getMacroControlData(macroIndex)->removeParameter(nameToUse, getProcessor(), sendNotificationSync);
		
		initMacroControl(sendNotification);
	}
	else if (result >= MidiOffset)
	{
		auto number = result - MidiOffset;

		auto mHandler = getProcessor()->getMainController()->getMacroManager().getMidiControlAutomationHandler();
		
		mHandler->deactivateMidiLearning();
		mHandler->removeMidiControlledParameter(processor, parameterToUse, sendNotificationAsync);
		mHandler->addMidiControlledParameter(processor, parameterToUse, rangeWithSkew, getValueToTextConverter(), -1);
		mHandler->setUnlearndedMidiControlNumber(MidiControllerAutomationHandler::Key(-1, number), sendNotificationAsync);
	}
	else if (result >= AddMacroControlOffset)
	{
		int macroIndex = result - AddMacroControlOffset;

		auto nameToUse = getName();

		if (customId.isValid())
			nameToUse = customId.toString();

		getProcessor()->getMainController()->getMacroManager().getMacroChain()->getMacroControlData(macroIndex)->addParameter(getProcessor(), parameterToUse, nameToUse, getValueToTextConverter(), rangeWithSkew, false, customId.isValid());

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
		getProcessor()->setAttribute(parameter, newValue, sendNotificationAsync);
	}
}

void MacroControlledObject::macroConnectionChanged(int macroIndex, Processor* p, int parameterIndex, bool wasAdded)
{
	auto parameterToUse = getAutomationIndex();

	if (getProcessor() == p && parameterToUse == parameterIndex)
	{
		auto pd = p->getMainController()->getMainSynthChain()->getMacroControlData(macroIndex)->getParameterWithProcessorAndIndex(p, parameterIndex);

		if(pd != nullptr && pd->isCustomAutomation() != customId.isValid())
		{
			// This might happen if you have a control with a custom automation index
			// that might collide with a normal parameter index...
			return;
		}

		if (wasAdded)
			addToMacroController(macroIndex);
		else
			removeFromMacroController();

		if (auto c = dynamic_cast<Component*>(this))
			c->repaint();

		updateValue(dontSendNotification);
	}
}

bool MacroControlledObject::canBeMidiLearned() const
{
#if HISE_ENABLE_MIDI_LEARN
	
	return midiLearnEnabled;
#else
	return false;
#endif
}

MacroControlledObject::~MacroControlledObject()
{
    numberTag = nullptr;
    slaf = nullptr;
    
	if (auto p = getProcessor())
	{
		if(valueListener != nullptr)
		{
			p->removeAttributeListener(valueListener);
			valueListener = nullptr;
		}

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
	if(valueListener != nullptr)
	{
		p->removeAttributeListener(valueListener);
		valueListener = nullptr;
	}

	processor = p;
	name = name_;

	if(parameter_ != -1)
	{
		valueListener = new dispatch::library::ProcessorHandler::AttributeListener(p->getMainController()->getRootDispatcher(), *this, BIND_MEMBER_FUNCTION_2(MacroControlledObject::onAttributeChange));
		parameter = parameter_;
		auto a = (uint16)parameter;
		p->addAttributeListener(valueListener, &a, 1, dispatch::DispatchType::sendNotificationAsync);
	}

	initMacroControl(dontSendNotification);

	auto newLaf = new ScriptingObjects::ScriptedLookAndFeel::Laf(p->getMainController());

	slaf = newLaf;

	WeakReference<ScriptingObjects::ScriptedLookAndFeel::Laf> safeLaf = newLaf;

	SafeAsyncCall::callAsyncIfNotOnMessageThread<Component>(*numberTag, [safeLaf](Component& c)
	{
		if (safeLaf != nullptr)
			c.setLookAndFeel(safeLaf.get());
	});

	p->getMainController()->getMainSynthChain()->addMacroConnectionListener(this);

    updateValue(sendNotificationSync);
	
}

void MacroControlledObject::connectToCustomAutomation(const Identifier& newCustomId)
{
	customId = newCustomId;

	rebuildPluginParameterConnection();

	updateValue(sendNotificationSync);

	auto mIndex = getMacroIndex();

	if (mIndex != -1)
		addToMacroController(mIndex);
	else
		removeFromMacroController();
}

int MacroControlledObject::getAutomationIndex() const
{
	if (customId.isNull() || processor == nullptr)
		return parameter;
	
	return processor->getMainController()->getUserPresetHandler().getCustomAutomationIndex(customId);
}

void MacroControlledObject::initMacroControl(NotificationType notify)
{
	rebuildPluginParameterConnection();
}

bool  MacroControlledObject::isLocked()
{
	if (!macroControlledComponentEnabled) return true;

	const int index = getMacroIndex();

	if(index == -1) return false;

	return isReadOnly();
}

bool  MacroControlledObject::isReadOnly()
{
	const int index = getMacroIndex();

	if (index == -1)
		return false;

	auto parameterIndexToUse = getAutomationIndex();

	const MacroControlBroadcaster::MacroControlledParameterData *data = GET_MACROCHAIN()->getMacroControlData(index)->getParameterWithProcessorAndIndex(getProcessor(), parameterIndexToUse);

	if(data == nullptr) return true;

	const bool ro = data->isReadOnly();

	return ro;
}

bool SliderWithShiftTextBox::onShiftClick(const MouseEvent& e)
{
	if (asSlider()->getWidth() > 25 && enableShiftTextInput)
	{
		asSlider()->addAndMakeVisible(inputLabel = new TextEditor());

		if(auto root = simple_css::CSSRootComponent::find(*asSlider()))
		{
			if(auto ss = root->css.getForComponent(inputLabel))
			{
				root->stateWatcher.registerComponentToUpdate(inputLabel);
				inputLabel->setBounds(asSlider()->getLocalBounds());
				inputLabel->addListener(this);
				inputLabel->setText(asSlider()->getTextFromValue(asSlider()->getValue()), dontSendNotification);
				inputLabel->selectAll();
				inputLabel->grabKeyboardFocus();
				return true;
			}
		}

		inputLabel->centreWithSize(asSlider()->getWidth(), 20);
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

		inputLabel->setText(asSlider()->getTextFromValue(asSlider()->getValue()), dontSendNotification);
		inputLabel->selectAll();
		inputLabel->grabKeyboardFocus();
        
        return true;
	}

    return false;
}

SliderWithShiftTextBox::~SliderWithShiftTextBox()
{}

void SliderWithShiftTextBox::init()
{
		
}

void SliderWithShiftTextBox::cleanup()
{
	inputLabel = nullptr;
}

void SliderWithShiftTextBox::onTextValueChange(double newValue)
{
	asSlider()->setValue(newValue, sendNotificationAsync);
}

Slider* SliderWithShiftTextBox::asSlider()
{ return dynamic_cast<Slider*>(this); }

const Slider* SliderWithShiftTextBox::asSlider() const
{ return dynamic_cast<const Slider*>(this); }

void SliderWithShiftTextBox::updateValueFromLabel(bool shouldUpdateValue)
{
	if (inputLabel == nullptr)
		return;

	auto doubleValue = asSlider()->getValueFromText(inputLabel->getText());

	if (shouldUpdateValue && (asSlider()->getRange().contains(doubleValue) || doubleValue == asSlider()->getMaximum()))
	{
		onTextValueChange(doubleValue);
	}

	inputLabel->removeListener(this);
	inputLabel = nullptr;
}

void SliderWithShiftTextBox::textEditorFocusLost(TextEditor&)
{
	updateValueFromLabel(true);
}

void SliderWithShiftTextBox::textEditorReturnKeyPressed(TextEditor&)
{
	updateValueFromLabel(true);
}

void SliderWithShiftTextBox::textEditorEscapeKeyPressed(TextEditor&)
{
	updateValueFromLabel(false);
}

void HiSlider::sliderValueChanged(Slider *s)
{
	jassert(s == this);

	if(!sendValueOnDrag)
		return;

	auto useMacrosAsParameter = (bool)HISE_GET_PREPROCESSOR(getProcessor()->getMainController(), HISE_MACROS_ARE_PLUGIN_PARAMETERS);
	
	if(!useMacrosAsParameter)
	{
		const int index = GET_MACROCHAIN()->getMacroControlIndexForProcessorParameter(getProcessor(), parameter);
    
		if (index != -1 && !isReadOnly())
		{
			const float v = (float)getRange().convertTo0to1(s->getValue());
			GET_MACROCHAIN()->setMacroControl(index,v * 127.0f, sendNotification);
		}
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

			getProcessor()->setAttribute(parameter, (float)s->getValue(), sendNotificationAsync);
		}
	}
}




void HiSlider::sliderDragStarted(Slider* s)
{
	checkMouseClickProfiler(true);

	if(auto pp = getConnectedPluginParameter())
		pp->beginChangeGesture();

	dragStartValue = s->getValue();

	Point<int> o;

	startTouch(o);
}

void HiSlider::sliderDragEnded(Slider* s)
{
	checkMouseClickProfiler(false);

	if(auto pp = getConnectedPluginParameter())
		pp->endChangeGesture();

	abortTouch();
	setAttributeWithUndo((float)s->getValue(), true, (float)dragStartValue);
}

bool HiSlider::changePluginParameter(AudioProcessor* p, int macroIndex)
{
	auto ok = HISE_GET_PREPROCESSOR(getProcessor()->getMainController(), HISE_MACROS_ARE_PLUGIN_PARAMETERS);
	jassert(ok);
    ignoreUnused(ok);

	if(auto pp = getConnectedPluginParameter())
	{
		auto hp = dynamic_cast<HisePluginParameterBase*>(pp);
		auto value = getValue();
		value = hp->getNormalisableRange().convertTo0to1(value);
		pp->setValueNotifyingHost(value);
		return true;
	}

	return false;
}

void HiSlider::updateValue(NotificationType /*sendAttributeChange*/)
{
	if(getProcessor() == nullptr)
		return;
	
	const bool enabled = !isLocked();

	setEnabled(enabled);

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

String HiSlider::getTextFromValue(double value)
{
	if (mode == Pan)				  return ValueToTextConverter::ConverterFunctions::Pan(value);
	if (mode == Frequency)			  return ValueToTextConverter::ConverterFunctions::Frequency(value);
	if (mode == TempoSync)			  return ValueToTextConverter::ConverterFunctions::TempoSync(value);
	if (mode == NormalizedPercentage) return ValueToTextConverter::ConverterFunctions::NormalizedPercentage(value);
	else							  return Slider::getTextFromValue(value);
}

void HiSlider::setup(Processor *p, int parameterIndex, const String &parameterName)
{
	modUpdater = new ModUpdater(*this, p->getMainController());

	MacroControlledObject::setup(p, parameterIndex, parameterName);

	p->getMainController()->skin(*this);
	
	for(int i = 0; i < numModes; i++)
	{
		modeValues[i] = 0.0;
	}

	if(parameterIndex != -1)
		setDoubleClickReturnValue(true, (double)p->getDefaultValue(parameterIndex), ModifierKeys());

	setName(parameterName);

	modUpdater->setUpdateFunction(p->getModulationQueryFunction(parameterIndex));
}

double HiSlider::getValueFromText(const String& text)
{
	if (mode == Frequency) return getFrequencyFromTextString(text);
	if(mode == TempoSync) return TempoSyncer::getTempoIndex(text);
	else if (mode == NormalizedPercentage) return text.getDoubleValue() / 100.0;
	else				  return Slider::getValueFromText(text);
}

void HiSlider::setLookAndFeelOwned(LookAndFeel *laf_)
{
	laf = laf_;
	setLookAndFeel(laf);
}

double HiSlider::getSkewFactorFromMidPoint(double minimum, double maximum, double midPoint)
{
	if (maximum > minimum)
		return log(0.5) / log((midPoint - minimum) / (maximum - minimum));
		
	jassertfalse;
	return 1.0;
}

String HiSlider::getSuffixForMode(HiSlider::Mode mode, float panValue)
{
	jassert(mode != numModes);

	switch (mode)
	{
	case Frequency:		return " Hz";
	case Decibel:		return " dB";
	case Time:			return " ms";
	case Pan:			return panValue > 0.0 ? "R" : "L";
	case TempoSync:		return String();
	case Linear:		return String();
	case Discrete:		return String();
	case NormalizedPercentage:	return "%";
	default:			return String();
	}
}

void HiSlider::HoverPopupLookandFeel::PositionData::fromVar(const var& data)
{
	if(auto obj = data.getDynamicObject())
	{
		margin = data.getProperty("margin", margin);
		auto l = data["dragAreas"];
		labelArea = ApiHelpers::getIntRectangleFromVar(data["labelArea"]);

		sensitivity = data.getProperty("mouseSensitivity", sensitivity);

		FloatSanitizers::sanitizeFloatNumber(sensitivity);
		sensitivity = jlimit(0.01f, 100.0f, sensitivity);

		auto dd = data.getProperty("dragDirection", "Diagonal");

		if(dd == "Diagonal")
			s = SliderStyle::RotaryHorizontalVerticalDrag;
		if(dd == "Horizontal")
			s = SliderStyle::LinearBar;
		if(dd == "Vertical")
			s = SliderStyle::LinearBarVertical;

		if(auto ar = l.getArray())
		{
			draggers.clear();

			auto r = Result::ok();

			for(const auto& a: *ar)
			{
				auto newArea = ApiHelpers::getIntRectangleFromVar(a, &r);
				draggers.addWithoutMerging(newArea);
			}
		}
	}
}

var HiSlider::HoverPopupLookandFeel::PositionData::toVar() const
{
	DynamicObject::Ptr obj = new DynamicObject();

	obj->setProperty("margin", margin);
	obj->setProperty("labelArea", ApiHelpers::getVarRectangle(true, labelArea.toFloat()));
	obj->setProperty("mouseSensitivity", sensitivity);

	switch(s)
	{
	case SliderStyle::RotaryHorizontalVerticalDrag:
		obj->setProperty("dragDirection", "Diagonal");
		break;
	case SliderStyle::LinearBarVertical:
		obj->setProperty("dragDirection", "Vertical");
		break;
	case SliderStyle::LinearBar:
		obj->setProperty("dragDirection", "Horizontal");
		break;
	}

	Array<var> dragData;

	for(const auto& d: draggers)
		dragData.add(ApiHelpers::getVarRectangle(true, d.toFloat()));

	obj->setProperty("dragAreas", var(dragData));

	return var(obj.get());
}

HiSlider::HoverPopupLookandFeel::PositionData HiSlider::HoverPopupLookandFeel::getModulatorDragData(HiSlider& s,
                                                                                                        const StringArray& sourceList) const
{
	PositionData pd;

	static constexpr int DRAG_SIZE = 30;

	auto totalWidth = sourceList.size() * DRAG_SIZE + 110;
	auto sb = s.getBoundsInParent();
	auto x = Rectangle(sb.getX(), sb.getBottom() + 5, totalWidth, DRAG_SIZE);
	
	for(auto s: sourceList)
		pd.draggers.addWithoutMerging(x.removeFromLeft(DRAG_SIZE));

	x.removeFromLeft(10);
	pd.labelArea = x;

	return pd;

}

void HiSlider::HoverPopupLookandFeel::drawModulationDragBackground(Graphics& g, HiSlider& s, const DrawData& obj,
	Rectangle<int> labelBounds)
{
	if(obj.isHover)
	{
		auto cornerSize = 3.0f;

		g.setColour(JUCE_LIVE_CONSTANT_OFF(Colour(0xffe0e0e0)));
		g.fillRoundedRectangle(labelBounds.toFloat(), cornerSize);


		g.setFont(GLOBAL_FONT());

		String text;
		text << obj.sourceName << ": " << obj.labelText;

		g.setColour(JUCE_LIVE_CONSTANT_OFF(Colour(0xff313131)));
		g.drawText(text, labelBounds.reduced(3, 0), Justification::left);
			

#if 0










		auto lr = labelBounds.toFloat();

		g.setColour(Colour(0x88161616));
		g.fillRoundedRectangle(lr, 3.0f);
		g.setColour(Colour(0x22FFFFFF));
		g.drawRoundedRectangle(lr, 3.0f, 1.0);

		g.setColour(Colour(0xAAFFFFFF));

		g.setFont(GLOBAL_FONT());

		String text;
		text << obj.sourceName << ": " << obj.labelText;

		g.drawText(text, lr.reduced(4.0f, 0.0f), Justification::left);
#endif
	}
}

void HiSlider::HoverPopupLookandFeel::drawModulationDragger(Graphics& g, HiSlider& s, const DrawData& obj)
{
	auto area = obj.bounds.toFloat();
	Path track, dragPath;

	double ARC = 2.5;

	double start = 0.0;
	double end = 1.0;
	
	switch(obj.targetMode)
	{
	case modulation::TargetMode::Gain:
			start = -ARC;
			end = -ARC + 2.0 * ARC * obj.intensityValue;
			break;
	case modulation::TargetMode::Unipolar:
			start = 0;
			end = ARC * obj.intensityValue;
			break;
	case modulation::TargetMode::Bipolar:
			end = ARC * obj.intensityValue;
			start = - 1.0 * end;
			break;
	}
	
	dragPath.startNewSubPath(area.getTopLeft());
	dragPath.startNewSubPath(area.getBottomRight());

	track.startNewSubPath(area.getTopLeft());
	track.startNewSubPath(area.getBottomRight());

	auto arcBounds = area.reduced(4);

	dragPath.addArc(arcBounds.getX(), arcBounds.getY(), arcBounds.getWidth(), arcBounds.getHeight(), start, end, true);
	track.addArc(arcBounds.getX(), arcBounds.getY(), arcBounds.getWidth(), arcBounds.getHeight(), -ARC, ARC, true);
	
	g.setColour(obj.isHover ? Colour(0x44000000) : Colour(0x22000000));

	PathStrokeType stroke(3.0f, PathStrokeType::curved, PathStrokeType::rounded);

	auto itemColour1 = s.findColour(HiseColourScheme::ColourIds::ComponentFillTopColourId);

	if(itemColour1.isTransparent())
		itemColour1 = Colours::white.withAlpha(0.8f);

	g.setColour(Colour(0x44000000));
	g.strokePath(track, stroke);
	g.setColour(itemColour1.withMultipliedAlpha(obj.isDown ? 1.0f : 0.8f));
	g.strokePath(dragPath, stroke);

#if 0
	g.setColour(Colours::black);
	g.fillRect(dd.bounds);
	g.setColour(Colours::white.withAlpha(0.2f));
	g.fillRect(dd.bounds.withWidth(dd.bounds.getWidth() * dd.intensityValue));
	g.setColour(Colours::white);
	g.drawText(dd.sourceName, dd.bounds.toFloat(), Justification::centred);
#endif
}

HiSlider::HoverPopupLookandFeel& HiSlider::getHoverPopupLookAndFeel()
{
	if(auto laf = dynamic_cast<HoverPopupLookandFeel*>(&getLookAndFeel()))
	{
		return *laf;
	}

	return fallback;
}

void HiSlider::ModUpdater::timerCallback()
{
	if(modFunction != nullptr)
	{
		if(auto p = parent.getProcessor())
		{
			auto nr = parent.getRange();

			auto mv = modFunction->getDisplayValue(p, parent.getValue(), nr);

			auto lastModValue = lastValue.lastModValue;
			auto thisModValue = mv.getNormalisedModulationValue();

			auto shouldSmooth = std::abs(lastModValue - thisModValue) > JUCE_LIVE_CONSTANT(0.01);

			if(lastValue != mv || shouldSmooth)
			{
				mv.lastModValue = lastModValue * 0.9 + thisModValue * 0.1;
				mv.storeToComponent(parent);
				lastValue = mv;
			}
		}
	}
}

void HiSlider::ModUpdater::setUpdateFunction(const ModulationDisplayValue::QueryFunction::Ptr f)
{
	modFunction = f;

	if(modFunction)
	{
		parent.scaleFunction = [this](bool isDown, float delta)
		{
			return modFunction->onScaleDrag(parent.getProcessor(), isDown, delta);
		};

		start();
	}
	else
	{
		parent.scaleFunction = {};

		stop();
	}
				
}

bool HiSlider::ModUpdater::canBeDropped(const var& info) const
{
	auto typeMatches = info["Type"] == "ModulationDrag";

	if(typeMatches)
	{
		auto sourceIndex = (int)info[MatrixIds::SourceIndex];
		auto chain = parent.getProcessor()->getMainController()->getMainSynthChain();

		if(auto gc = ProcessorHelpers::getFirstProcessorWithType<GlobalModulatorContainer>(chain))
		{
			auto md = gc->getMatrixModulatorData();
			auto targetId = parent.getProcessor()->getModulationTargetId(parent.getParameter());
			auto con = MatrixIds::Helpers::getConnection(md, sourceIndex, targetId);

			auto a = !con.isValid() ? GlobalModulatorContainer::DragAction::Hover :
								      GlobalModulatorContainer::DragAction::DisabledHover;

			gc->sendDragMessage(sourceIndex, targetId, a);
			return !con.isValid();
		}
	}

	return false;
}

void HiSlider::ModUpdater::onDrop(const var& info)
{
	auto sourceIndex = (int)info["SourceIndex"];
	auto chain = parent.getProcessor()->getMainController()->getMainSynthChain();

	if(auto gc = ProcessorHelpers::getFirstProcessorWithType<GlobalModulatorContainer>(chain))
	{
		auto targetId = parent.getProcessor()->getModulationTargetId(parent.getParameter());
		gc->sendDragMessage(sourceIndex, targetId, GlobalModulatorContainer::DragAction::Drop);
	}

	parent.getProcessor()->onModulationDrop(parent.getParameter(), sourceIndex);
}


NormalisableRange<double> HiSlider::getRangeForMode(HiSlider::Mode m)
{
	NormalisableRange<double> r;

	switch(m)
	{
	case Frequency:				
		r = NormalisableRange<double>(20.0, 20000.0, 1);
		r.setSkewForCentre(1500.0);
		break;
	case Decibel:				
		r = NormalisableRange<double>(-100.0, 0.0, 0.1);
		r.setSkewForCentre(-18.0);
		break;
	case Time:					
		r = NormalisableRange<double>(0.0, 20000.0, 1);
		r.setSkewForCentre(1000.0);
		break;
	case TempoSync:				
		r = NormalisableRange<double>(0, TempoSyncer::numTempos-1, 1);
		break;
	case Pan:					
		r = NormalisableRange<double>(-100.0, 100.0, 1);
		break;
	case NormalizedPercentage:	
		r = NormalisableRange<double>(0.0, 1.0, 0.01);									
		break;
	case Linear:				
		r = NormalisableRange<double>(0.0, 1.0, 0.01); 
		break;
	case Discrete:				
		r = NormalisableRange<double>();
		r.interval = 1;
		break;
	case numModes: 
	default:					jassertfalse; 
		r = NormalisableRange<double>();
	}

	return r;
}

HiSlider::HiSlider(const String &name) :
	Slider(name),
	MacroControlledObject(),
	mode(numModes)
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

	// init the modulationDragState property so that it's not undefined before the first drag...
	getProperties().set("modulationDragState", 0);
}

HiSlider::~HiSlider()
{
	cleanup();
	setLookAndFeel(nullptr);
}




double HiSlider::getFrequencyFromTextString(const String& t)
{
	if (t.contains("kHz"))
		return t.getDoubleValue() * 1000.0;
	else
		return t.getDoubleValue();
}

void HiSlider::mouseEnter(const MouseEvent& event)
{
	showModHoverPopup(true, true);
	Slider::mouseEnter(event);
}



void HiSlider::mouseDoubleClick(const MouseEvent &e)
{
    performModifierAction(e, true);
}

struct HiSlider::HoverPopup: public Component,
							 public PooledUIUpdater::SimpleTimer
	
{
	HoverPopup(HiSlider& slider, 
	           const ValueTree& matrixData_, 
		       const String& targetId_, 
		       const Array<int>& sourceIndexes_, 
		       const StringArray& sourceNames_, 
		       const HoverPopupLookandFeel::PositionData& pd):
	  SimpleTimer(slider.getProcessor()->getMainController()->getGlobalUIUpdater()),
	  parent(&slider),
	  targetId(targetId_),
	  matrixData(matrixData_),
	  sourceIndexes(sourceIndexes_),
	  sourceNames(sourceNames_),
	  dragAreas(pd.draggers),
	  labelArea(pd.labelArea),
	  sensitivity(pd.sensitivity),
	  sliderStyle(pd.s)
	{
		auto pp = parent->getParentComponent();
		pp->addAndMakeVisible(this);
		auto pb = parent->getBoundsInParent();
		auto b = dragAreas.getBounds();

		if(!labelArea.isEmpty())
			b = b.getUnion(labelArea);
		
		setBounds(b.expanded(pd.margin));
		globalHitbox = getScreenBounds().getUnion(slider.getScreenBounds());

		auto translationToOrigin = AffineTransform::translation(getBoundsInParent().getTopLeft().toFloat() * -1.0f);

		dragAreas.transformAll(translationToOrigin);

		if(!labelArea.isEmpty())
			labelArea = labelArea.transformed(translationToOrigin);

		start();

		gc = ProcessorHelpers::getFirstProcessorWithType<GlobalModulatorContainer>(slider.getProcessor()->getMainController()->getMainSynthChain());

		if(gc != nullptr)
		{
			gc->currentMatrixSourceBroadcaster.addListener(*this, [](HoverPopup& hp, int)
			{
				hp.keepAlive = false;
				hp.clear();
			}, false);
		}

		rebuild();
	}

	WeakReference<GlobalModulatorContainer> gc;

	~HoverPopup()
	{
		if(gc != nullptr)
		{
			gc->currentMatrixSourceBroadcaster.removeListener(*this);
		}

		//jassert(!keepAlive);
	}

	bool hitTest(int x, int y) override
	{
		if(labelArea.contains(x, y))
			return true;

		for(auto r: dragAreas)
		{
			if(r.contains(x, y))
				return true;
		}

		return false;
	}

	void rebuild()
	{
		intensityValues.clear();
		labelConverters.clear();
		intensityDownValues.clear();


		auto tt = MatrixIds::Helpers::getTargetType(parent->getProcessor()->getMainController(), targetId);

		if(tt == MatrixIds::Helpers::TargetType::Modulators)
		{
			auto mod = dynamic_cast<MatrixModulator*>(parent->getProcessor());

			if(auto pwsc = dynamic_cast<ProcessorWithScriptingContent*>(parent->getProcessor()))
			{
				if(auto sc = pwsc->getScriptingContent()->getComponent(parent->getParameter()))
				{
					mod = dynamic_cast<MatrixModulator*>(sc->getConnectedProcessor());
				}

				if(mod == nullptr)
				{
					Processor::Iterator<MatrixModulator> iter(parent->getProcessor()->getMainController()->getMainSynthChain());

					while(auto n = iter.getNextProcessor())
					{
						if(n->getMatrixTargetId() == targetId)
						{
							mod = n;
							break;
						}
					}
				}
			}

			if(mod != nullptr)
			{
				for(int i = 0; i < sourceIndexes.size(); i++)
				{
					MatrixIds::Helpers::IntensityTextConverter::ConstructData cd = mod->getIntensityTextConstructData();
					cd.connectedSlider = parent.getComponent();
					cd.tm = (scriptnode::modulation::TargetMode)(int)getConnectionData(sourceIndexes[i])[MatrixIds::Mode];
					auto c = new MatrixIds::Helpers::IntensityTextConverter(cd);
					labelConverters.add(c);
				}
			}
		}
		else
		{
			if(auto pwsc = dynamic_cast<ProcessorWithScriptingContent*>(parent->getProcessor()))
			{
				using S = ScriptingApi::Content::ScriptSlider;

				if(auto sc = dynamic_cast<S*>(pwsc->getScriptingContent()->getComponent(parent->getParameter())))
				{
					for(int i = 0; i < sourceIndexes.size(); i++)
					{
						auto sourceIndex = sourceIndexes[i];
						auto cd = sc->createIntensityConverter(sourceIndex);
						cd.connectedSlider = parent.getComponent();
						cd.tm = (scriptnode::modulation::TargetMode)(int)getConnectionData(sourceIndex)[MatrixIds::Mode];
						labelConverters.add(new MatrixIds::Helpers::IntensityTextConverter(cd));
					}
				}
			}
		}

		for(int i = 0; i < sourceIndexes.size(); i++)
		{
			auto intensity = getIntensity(sourceIndexes[i], targetId);
			intensityDownValues.push_back(intensity);
			intensityValues.push_back(intensity);
		}

		repaint();
	}

	Rectangle<int> globalHitbox;

	void paint(Graphics& g) override
	{
		auto& laf = parent->getHoverPopupLookAndFeel();

		HoverPopupLookandFeel::DrawData dd;

		dd.isHover = currentHoverIndex != -1;

		auto down = isMouseButtonDown();

		dd.isDown = down;
		dd.bounds = getLocalBounds().toFloat();
		dd.targetName = targetId;

		if(currentHoverIndex != -1)
		{
			dd.sourceIndex = sourceIndexes[currentHoverIndex];
			dd.sourceName = sourceNames[currentHoverIndex];
			dd.targetMode = (scriptnode::modulation::TargetMode)(int)getConnectionData(sourceIndexes[currentHoverIndex])[MatrixIds::Mode];
			dd.intensityRange = Range<double>(dd.targetMode == scriptnode::modulation::TargetMode::Gain ? 0.0f : -1.0f, 1.0f);
			dd.intensityValue = intensityValues[currentHoverIndex];

			if(isPositiveAndBelow(currentHoverIndex, labelConverters.size()))
				dd.labelText = labelConverters[currentHoverIndex]->getText(dd.intensityValue);
		}

		laf.drawModulationDragBackground(g, *parent, dd, labelArea);

		for(int i = 0; i < sourceNames.size(); i++)
		{
			dd.isHover = currentHoverIndex == i;
			dd.isDown = down && dd.isHover;
			dd.bounds = dragAreas.getRectangle(i).toFloat();
			dd.sourceIndex = sourceIndexes[i];
			dd.sourceName = sourceNames[i];
			dd.targetMode = (scriptnode::modulation::TargetMode)(int)getConnectionData(sourceIndexes[i])[MatrixIds::Mode];
			dd.intensityRange = Range<double>(dd.targetMode == scriptnode::modulation::TargetMode::Gain ? 0.0f : -1.0f, 1.0f);
			dd.intensityValue = intensityValues[i];
			parent->getHoverPopupLookAndFeel().drawModulationDragger(g, *parent, dd);
		}
	}

	void clear()
	{
		auto s = parent.getComponent();
		MessageManager::callAsync([s]()
		{
			Desktop::getInstance().getAnimator().fadeOut(s->currentHoverPopup, 150);
			s->currentHoverPopup = nullptr;
		});
	}

	float getIntensity(int sourceIndex, const String& targetId) const
	{
		auto c = getConnectionData(sourceIndex);

		if(c.isValid())
			return (float)c[MatrixIds::Intensity];

		return 0.0f;
	}

	void timerCallback() override
	{
		auto pos = Desktop::getInstance().getMainMouseSource().getScreenPosition().toInt();

		if(!globalHitbox.contains(pos) && !keepAlive)
		{
			auto now = Time::getMillisecondCounter();

			if(now - lastHoverTime > 300)
			{
				clear();
			}
		}
		else
		{
			lastHoverTime = Time::getMillisecondCounter();
		}
	}

	ValueTree getConnectionData(int sourceIndex) const
	{
		for(auto c: matrixData)
		{
			if(MatrixIds::Helpers::matchesTarget(c, targetId) && (int)c[MatrixIds::SourceIndex] == sourceIndex)
				return c;
		}

		return {};
	}

	void mouseDoubleClick(const MouseEvent& event) override
	{
		auto um = parent->getProcessor()->getMainController()->getControlUndoManager();
		auto c = getConnectionData(sourceIndexes[currentHoverIndex]);
		c.getParent().removeChild(c, um);

		removeSource(currentHoverIndex);
	}

	void mouseMove(const MouseEvent& e) override
	{
		currentHoverIndex = getSourceIndexForMouseEvent(e, false);
		repaint();
	}

	void mouseEnter(const MouseEvent& event) override
	{
		keepAlive = false;
		repaint();
	}

	void mouseExit(const MouseEvent& event) override
	{
		repaint();
	}

	void mouseDown(const MouseEvent& e) override
	{
		currentHoverIndex = getSourceIndexForMouseEvent(e, true);

		if(e.mods.isRightButtonDown() && currentHoverIndex != -1)
		{
			PopupMenu m;
			m.setLookAndFeel(&parent->getLookAndFeel());

			auto c = getConnectionData(sourceIndexes[currentHoverIndex]);
			auto tm = (int)c[MatrixIds::Mode];

			static constexpr int ModeOffset = 4;

			m.addItem(1, "Remove connection");
			m.addItem(2, "Enter value");
			m.addSeparator();
			m.addItem(3, "Inverted", true, c[MatrixIds::Inverted]);
			m.addItem(ModeOffset, "Scaled", true, tm == 0);
			m.addItem(ModeOffset+1, "Unipolar", true, tm == 1);
			m.addItem(ModeOffset+2, "Bipolar", true, tm == 2);


			auto um = parent->getProcessor()->getMainController()->getControlUndoManager();

			keepAlive = true;

			auto result = PopupLookAndFeel::showAtComponent(m, dynamic_cast<Component*>(this), false);

			if(result == 0)
			{
				keepAlive = false;
				return;
			}

			switch(result)
			{
			case 1:
				c.getParent().removeChild(c, um);
				removeSource(currentHoverIndex);
				break;
			case 2:
				showEditor();
				break;
			case 3:
				c.setProperty(MatrixIds::Inverted, !(bool)c[MatrixIds::Inverted], um);
				break;
			default:
				c.setProperty(MatrixIds::Mode, result - ModeOffset, um);
				rebuild();
				break;
			}
		}

		if(isPositiveAndBelow(currentHoverIndex, intensityDownValues.size()))
		{
			auto sourceIndex = sourceIndexes[currentHoverIndex];
			intensityDownValues[currentHoverIndex] = getIntensity(sourceIndex, targetId);
		}

		repaint();
	}

	void removeSource(int indexInList)
	{
		if(sourceNames.size() == 1)
		{
			clear();
			return;
		}

		sourceNames.remove(indexInList);
		sourceIndexes.remove(indexInList);

		

		RectangleList<int> newList;

		for(int i = 0; i < dragAreas.getNumRectangles()-1; i++)
		{
			newList.addWithoutMerging(dragAreas.getRectangle(i));
		}

		currentHoverIndex = -1;
		newList.swapWith(dragAreas);
		rebuild();
	}

	void showEditor()
	{
		if(isPositiveAndBelow(currentHoverIndex, labelConverters.size()))
		{
			keepAlive = true;
			addAndMakeVisible(editor = new TextEditor());
			editor->setLookAndFeel(&parent->getLookAndFeel());

			if(auto root = simple_css::CSSRootComponent::find(*parent))
			{
				if(auto ss = root->css.getForComponent(editor))
				{
					ss->setupComponent(root, editor, 0);
				}
			}

			editor->setText(labelConverters[currentHoverIndex]->getText(intensityValues[currentHoverIndex]));
			editor->setBounds(labelArea);
			editor->setSelectAllWhenFocused(true);
			editor->grabKeyboardFocusAsync();

			auto f = [this]()
			{
				
				if(isPositiveAndBelow(currentHoverIndex, labelConverters.size()))
				{
					auto newValue = labelConverters[currentHoverIndex]->getValue(editor->getText());

					auto sourceIndex = sourceIndexes[currentHoverIndex];
					auto cm = getConnectionData(sourceIndex);
					newValue = jlimit((int)cm[MatrixIds::Mode] == 0 ? 0.0 : -1.0, 1.0, newValue);

					intensityValues[currentHoverIndex] = newValue;
					auto um = parent->getProcessor()->getMainController()->getControlUndoManager();
					cm.setProperty(MatrixIds::Intensity, newValue, um);
					repaint();
				};

				keepAlive = false;

				MessageManager::callAsync([this]()
				{
					editor = nullptr;
				});
			};;

			editor->onFocusLost = f;
			editor->onReturnKey = f;
		}
	}

	ScopedPointer<TextEditor> editor;

	int getSourceIndexForMouseEvent(const MouseEvent& e, bool downPos) const
	{
		for(int i = 0; i < dragAreas.getNumRectangles(); i++)
		{
			if(dragAreas.getRectangle(i).contains(downPos ? e.getMouseDownPosition() : e.getPosition()))
				return i;
		}

		return -1;
	}

	void mouseUp(const MouseEvent& e) override
	{
		if(e.mods.isRightButtonDown())
		{
			return;
		}

		keepAlive = false;
		repaint();
	}

	void mouseDrag(const MouseEvent& e) override
	{
		keepAlive = !getLocalBounds().contains(e.getPosition());

		if(isPositiveAndBelow(currentHoverIndex, intensityDownValues.size()))
		{
			auto downValue = intensityDownValues[currentHoverIndex];
			auto deltaX = (float)e.getDistanceFromDragStartX() / parent->getWidth();
			auto deltaY = (float)e.getDistanceFromDragStartY() / parent->getHeight();

			if(sliderStyle == SliderStyle::LinearBarVertical)
				deltaX = 0.0;

			if(sliderStyle == SliderStyle::LinearBar)
				deltaY = 0.0;

			auto sourceIndex = sourceIndexes[currentHoverIndex];
			auto cd = getConnectionData(sourceIndex);

			auto isScale = (int)cd[MatrixIds::Mode] == 0;

			auto newValue = jlimit(isScale ? 0.0f : -1.0f, 1.0f, downValue + (deltaX - deltaY) * sensitivity * 0.25f);

			intensityValues[currentHoverIndex] = newValue;

			auto um = parent->getProcessor()->getMainController()->getControlUndoManager();
			cd.setProperty(MatrixIds::Intensity, newValue, um);
			repaint();
		}
	}

	std::vector<float> intensityDownValues;
	std::vector<float> intensityValues;
	uint32 lastHoverTime = 0;

	bool keepAlive = false;
	Component::SafePointer<HiSlider> parent;
	String targetId;
	ValueTree matrixData;
	Array<int> sourceIndexes;
	StringArray sourceNames;
	RectangleList<int> dragAreas;
	Rectangle<int> labelArea;

	int currentHoverIndex = -1;

	OwnedArray<MatrixIds::Helpers::IntensityTextConverter> labelConverters;

	float sensitivity;
	SliderStyle sliderStyle;

	JUCE_DECLARE_WEAK_REFERENCEABLE(HoverPopup);
};

void HiSlider::showModHoverPopup(bool shouldShow, bool closeOnExit)
{
	if(modUpdater != nullptr && modUpdater->modFunction)
	{
		auto hp = dynamic_cast<HoverPopup*>(currentHoverPopup.get());

		if(hp != nullptr)
		{
			if(!shouldShow && hp->keepAlive)
				return;
		}

		if(shouldShow)
		{
			if(hp != nullptr)
			{
				hp->keepAlive = true;
				return;
			}

			Array<int> connectedSources;

			auto targetId = getProcessor()->getModulationTargetId(getParameter());

			if(auto gc = ProcessorHelpers::getFirstProcessorWithType<GlobalModulatorContainer>(getProcessor()->getMainController()->getMainSynthChain()))
			{
				auto md = gc->getMatrixModulatorData();

				for(auto c: md)
				{
					if(MatrixIds::Helpers::matchesTarget(c, targetId))
					{
						connectedSources.add((int)c[MatrixIds::SourceIndex]);
					}
				}

				if(!connectedSources.isEmpty())
				{
					StringArray allSources, sourceList;
					MatrixIds::Helpers::fillModSourceList(getProcessor()->getMainController(), allSources);

					for(auto idx: connectedSources)
						sourceList.add(allSources[idx]);

					if(auto pd = getHoverPopupLookAndFeel().getModulatorDragData(*this, sourceList))
					{
						callRecursive<HoverPopup>(getTopLevelComponent(), [](HoverPopup* h)
						{
							h->clear();
							return false;
						});

						currentHoverPopup = new HoverPopup(*this, md, targetId, connectedSources, sourceList, pd);
					}
				}
			}
		}
		else
		{
			currentHoverPopup = nullptr;
		}

	}
}

void HiSlider::mouseDown(const MouseEvent &e)
{
	CHECK_MIDDLE_MOUSE_DOWN(e);

	showModHoverPopup(false, false);

    if(performModifierAction(e, false))
    {
        return;
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

void HiSlider::mouseExit(const MouseEvent& e)
{
	if(auto hp = dynamic_cast<HoverPopup*>(currentHoverPopup.get()))
	{
		hp->keepAlive = false;
	}
}

void HiSlider::mouseDrag(const MouseEvent& e)
{
	CHECK_MIDDLE_MOUSE_DRAG(e);

	setDragDistance((float)e.getDistanceFromDragStart());

	if(performModifierAction(e, false, false))
		return;

	Slider::mouseDrag(e);
}

void HiSlider::mouseUp(const MouseEvent& e)
{
	CHECK_MIDDLE_MOUSE_UP(e);

	abortTouch();

	showModHoverPopup(true, false);

	if(performModifierAction(e, false, false))
		return;

	Slider::mouseUp(e);
}

void HiSlider::mouseWheelMove(const MouseEvent& event, const MouseWheelDetails& wheel)
{
	CHECK_VIEWPORT_SCROLL(event, wheel);

	Slider::mouseWheelMove(event, wheel);
}

void HiSlider::touchAndHold(Point<int> /*downPosition*/)
{
	enableMidiLearnWithPopup();
}

void HiSlider::onTextValueChange(double newValue)
{
	setValue(newValue, dontSendNotification);

	setAttributeWithUndo((float)newValue);
}

void HiSlider::resized()
{
	Slider::resized();
	numberTag->setBounds(getLocalBounds());
}

ValueToTextConverter HiSlider::getValueToTextConverter() const
{
	hise::ValueToTextConverter c;
	c.active = true;
	c.suffix = getTextValueSuffix();
	c.stepSize = getRange().interval;

#define FUNC(x) case x: c.valueToTextFunction = ValueToTextConverter::ConverterFunctions::x; c.textToValueFunction = ValueToTextConverter::InverterFunctions::x; break;

	switch (mode)
	{
	FUNC(Frequency);
	FUNC(Time);
	FUNC(TempoSync);
	FUNC(Pan);
	FUNC(NormalizedPercentage);
	default: break;
	}

#undef FUNC

	return c;
}


String HiSlider::getModeId() const
{
	switch (mode)
	{
	case Frequency: return "Frequency";
	case Decibel: return "Decibel";
	case Time: return "Time";
	case TempoSync: return "TempoSync";
	case Linear: return "Linear";
	case Discrete: return "Discrete";
	case Pan: return "Pan";
	case NormalizedPercentage: return "NormalizedPercentage";
	case numModes: return "";
	}

	return "";
}

void HiSlider::setMode(Mode m)
{
	if (mode != m)
	{
		mode = m;

		auto normRange = getRangeForMode(m);
		setTextValueSuffix(getModeSuffix());
		setNormalisableRange(normRange);
		setValue(modeValues[m], dontSendNotification);

		repaint();
	}
}

void HiSlider::setMode(Mode m, NormalisableRange<double> nr)
{ 
	if(mode != m)
	{
		mode = m;
		setNormalisableRange(nr);
		setTextValueSuffix(getModeSuffix());

		setValue(modeValues[m], dontSendNotification);

		repaint();
	}
	else
	{
		setNormalisableRange(nr);
	}
    
    updateValue(sendNotificationSync);
}

HiSlider::Mode HiSlider::getMode() const
{ return mode; }

bool HiSlider::isUsingModulatedRing() const noexcept
{
	return modUpdater != nullptr && (bool)modUpdater->modFunction;
}

float HiSlider::getDisplayValue() const
{
	return isUsingModulatedRing() ? (float)getProperties()["modValue"] : 1.0f;
}

NormalisableRange<double> HiSlider::getRange() const
{
	auto r = Slider::getRange();
	NormalisableRange<double> nr(r.getStart(), r.getEnd());
	nr.interval = Slider::getInterval();
	nr.skew = Slider::getSkewFactor();
	return nr;
}

String HiSlider::getModeSuffix() const
{
	return getSuffixForMode(mode, (float)modeValues[Pan]);
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

void HiToggleButton::mouseDrag(const MouseEvent& e)
{
	CHECK_MIDDLE_MOUSE_DRAG(e);
}

void HiToggleButton::mouseDown(const MouseEvent &e)
{
	checkMouseClickProfiler(true);

	CHECK_MIDDLE_MOUSE_DOWN(e);

    if(e.mods.isLeftButtonDown())
    {
        checkLearnMode();
        
        PresetHandler::setChanged(getProcessor());
        
        startTouch(e.getMouseDownPosition());
        
        MomentaryToggleButton::mouseDown(e);

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
	checkMouseClickProfiler(false);

	CHECK_MIDDLE_MOUSE_UP(e);

    abortTouch();
    MomentaryToggleButton::mouseUp(e);
}

HiComboBox::HiComboBox(const String& name):
	SubmenuComboBox(name),
	MacroControlledObject()
{
	addChildComponent(numberTag);
	font = GLOBAL_FONT();

	addListener(this);

	setWantsKeyboardFocus(false);
        
	setColour(HiseColourScheme::ComponentFillTopColourId, Colour(0x66333333));
	setColour(HiseColourScheme::ComponentFillBottomColourId, Colour(0xfb111111));
	setColour(HiseColourScheme::ComponentOutlineColourId, Colours::white.withAlpha(0.3f));
	setColour(HiseColourScheme::ComponentTextColourId, Colours::white);
}

HiComboBox::~HiComboBox()
{
	setLookAndFeel(nullptr);
}

void HiComboBox::resized()
{
	ComboBox::resized();
	numberTag->setBounds(getLocalBounds());
}

NormalisableRange<double> HiComboBox::getRange() const
{ 
	NormalisableRange<double> r(1.0, (double)getNumItems()); 

	r.interval = 1.0;

	return r;
}

MomentaryToggleButton::MomentaryToggleButton(const String& name):
	ToggleButton(name)
{}

void MomentaryToggleButton::setIsMomentary(bool shouldBeMomentary)
{
	isMomentary = shouldBeMomentary;
}

void MomentaryToggleButton::mouseDown(const MouseEvent& e)
{
	if (e.mods.isRightButtonDown())
		return;

	if (isMomentary)
	{
		setToggleState(true, sendNotification);
	}
	else
	{
		ToggleButton::mouseDown(e);
	}
}

void MomentaryToggleButton::mouseUp(const MouseEvent& e)
{
	if (e.mods.isRightButtonDown())
		return;

	if (isMomentary)
	{
		setToggleState(false, sendNotification);
	}
	else
	{
		ToggleButton::mouseUp(e);
	}
}

HiToggleButton::HiToggleButton(const String& name):
	MomentaryToggleButton(name),
	MacroControlledObject(),
	notifyEditor(dontSendNotification)
{
	addChildComponent(numberTag);
	addListener(this);
	setWantsKeyboardFocus(false);
        
	setColour(HiseColourScheme::ComponentFillTopColourId, Colour(0x66333333));
	setColour(HiseColourScheme::ComponentFillBottomColourId, Colour(0xfb111111));
	setColour(HiseColourScheme::ComponentOutlineColourId, Colours::white.withAlpha(0.3f));
}

HiToggleButton::~HiToggleButton()
{
	setLookAndFeel(nullptr);
}

void HiToggleButton::setNotificationType(NotificationType notify)
{
	notifyEditor = notify;
}

void HiToggleButton::setPopupData(const var& newPopupData, Rectangle<int>& newPopupPosition)
{
	popupData = newPopupData;
	popupPosition = newPopupPosition;
}

void HiToggleButton::resized()
{
	ToggleButton::resized();
	numberTag->setBounds(getLocalBounds());
}

NormalisableRange<double> HiToggleButton::getRange() const
{ 
	NormalisableRange<double> r(0.0, 1.0); 
	r.interval = 1.0;

	return r;
}

void HiComboBox::setup(Processor *p, int parameterIndex, const String &parameterName)
{
	MacroControlledObject::setup(p, parameterIndex, parameterName);

	p->getMainController()->skin(*this);
}

void HiComboBox::mouseDown(const MouseEvent &e)
{
	checkMouseClickProfiler(true);

	if(auto pp = getConnectedPluginParameter())
		pp->endChangeGesture();

	CHECK_MIDDLE_MOUSE_DOWN(e);

    if(e.mods.isLeftButtonDown())
    {
        checkLearnMode();
        PresetHandler::setChanged(getProcessor());
        startTouch(e.getMouseDownPosition());
        
        ComboBox::mouseDown(e);
    }
    else
		enableMidiLearnWithPopup();
}

void HiComboBox::mouseUp(const MouseEvent& e)
{
	checkMouseClickProfiler(false);

	if(auto pp = getConnectedPluginParameter())
		pp->endChangeGesture();

	CHECK_MIDDLE_MOUSE_UP(e);
	abortTouch();
	ComboBox::mouseUp(e);
};


void HiComboBox::mouseDrag(const MouseEvent& e)
{
	CHECK_MIDDLE_MOUSE_DRAG(e);
	ComboBox::mouseDrag(e);
}

ValueToTextConverter HiComboBox::getValueToTextConverter() const
{
	StringArray itemList;
	itemList.add("Nothing");

	for(int i = 0; i < getNumItems(); i++)
		itemList.add(getItemText(i));

	ValueToTextConverter c;
	c.active = true;
	c.itemList = itemList;

	return c;
}

void HiComboBox::touchAndHold(Point<int> /*downPosition*/)
{
	enableMidiLearnWithPopup();
}
    
void HiComboBox::updateValue(NotificationType /*sendAttributeChange*/)
{
	const bool enabled = !isLocked();

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
}




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
    
	if (getProcessor() == nullptr)
		return;

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



#undef GET_MACROCHAIN

	MacroControlledObject::ModulationPopupData::operator bool() const noexcept
	{
		return targetId.isNotEmpty();
	}

	MacroControlledObject::MacroControlledObject():
		parameter(-1),
		processor(nullptr),
		macroIndex(-1),
		name(""),
		numberTag(new NumberTag(3, 14.0f, Colour(SIGNAL_COLOUR))),
		macroControlledComponentEnabled(true)
	{}

	const String MacroControlledObject::getName() const noexcept
	{ return name; }

	void MacroControlledObject::setCanBeMidiLearned(bool shouldBe)
	{
		midiLearnEnabled = shouldBe;
	}

	void MacroControlledObject::setUseUndoManagerForEvents(bool shouldUseUndo)
	{ useUndoManagerForEvents = shouldUseUndo; }

	void MacroControlledObject::addToMacroController(int newMacroIndex)
	{
		if(macroIndex != newMacroIndex)
		{
			numberTag->setNumber(newMacroIndex+1);
			numberTag->setVisible(true);
            
            if(auto asComponent = dynamic_cast<Component*>(this))
            {
#define SET_COLOUR(x) numberTag->setColour(x, asComponent->findColour(x));
                SET_COLOUR(HiseColourScheme::ComponentOutlineColourId);
                SET_COLOUR(HiseColourScheme::ComponentFillTopColourId);
                SET_COLOUR(HiseColourScheme::ComponentFillBottomColourId);
                SET_COLOUR(HiseColourScheme::ComponentTextColourId);
#undef SET_COLOUR
            }
            
			macroIndex = newMacroIndex;
		}
	}

	void MacroControlledObject::removeFromMacroController()
	{
		if(macroIndex != -1)
		{
			numberTag->setNumber(0);
			numberTag->setVisible(false);
			macroIndex = -1;
		}
	}

	void MacroControlledObject::enableMacroControlledComponent(bool shouldBeEnabled) noexcept
	{
		macroControlledComponentEnabled = shouldBeEnabled;
	}

	int MacroControlledObject::getParameter() const
	{ return parameter; }

	void MacroControlledObject::setModulationData(ModulationPopupData::Ptr modData)
	{
		modulationData = modData;
	}

	void MacroControlledObject::onAttributeChange(dispatch::library::Processor* p, uint8 index)
	{
#if HISE_NEW_PROCESSOR_DISPATCH
		if(&p->getOwner<hise::Processor>() == getProcessor())
		{
			if(index == getParameter())
			{
				updateValue(sendNotificationSync);
			}
		}
#endif
	}

	juce::AudioProcessorParameter* MacroControlledObject::getConnectedPluginParameter() const
	{
		return dynamic_cast<juce::AudioProcessorParameter*>(connectedPluginParameter.get());
	}

	void MacroControlledObject::rebuildPluginParameterConnection()
	{
		auto p = getProcessor();

		if(p == nullptr)
			return;

		auto isCustomAutomation = customId.isValid();
		auto useMacrosAsParameter = HISE_GET_PREPROCESSOR(p->getMainController(), HISE_MACROS_ARE_PLUGIN_PARAMETERS);
		auto isMacro = useMacrosAsParameter && getMacroIndex() != -1;

		auto wasMacro = connectedPluginParameter != nullptr && connectedPluginParameter->getType() == HisePluginParameterBase::Type::Macro;
		bool sendPluginParameterUpdate = false;

		HisePluginParameterBase::Type t;
		int slotIndex = -1;

		if (isMacro)
		{
			slotIndex = getMacroIndex();

			t = HisePluginParameterBase::Type::Macro;
		}
		else if(isCustomAutomation)
		{
			t = HisePluginParameterBase::Type::CustomAutomation;

			if(auto d = p->getMainController()->getUserPresetHandler().getCustomAutomationData(customId))
				slotIndex = d->index;
		}
		else
		{
			t = HisePluginParameterBase::Type::ScriptControl;
			slotIndex = parameter;
		}

		connectedPluginParameter = nullptr;

		if(auto jmp = dynamic_cast<JavascriptMidiProcessor*>(getProcessor()))
		{
			if(jmp->isFront())
			{
				auto ap = dynamic_cast<AudioProcessor*>(p->getMainController());
				
				for(auto pp: ap->getParameters())
				{
					if(auto typed = dynamic_cast<HisePluginParameterBase*>(pp))
					{
						auto wrapped = typed->getWrappedParameter();
						auto matchesType = wrapped->getType() == t;

						if(matchesType && wrapped->matchesIndex(slotIndex))
						{
							sendPluginParameterUpdate = connectedPluginParameter != typed;
							connectedPluginParameter = typed;
							break;
						}
					}
				}
			}
		}

		sendPluginParameterUpdate |= connectedPluginParameter == nullptr & wasMacro;
		
		if(sendPluginParameterUpdate && !skipHostDisplayUpdate)
		{
			auto details = AudioProcessorListener::ChangeDetails().withParameterInfoChanged(true);
			dynamic_cast<AudioProcessor*>(getProcessor()->getMainController())->updateHostDisplay(details);
		}
	}

	void MacroControlledObject::checkMouseClickProfiler(bool isDown)
	{
		
	}

	Processor* MacroControlledObject::getProcessor()
	{return processor.get(); }

	const Processor* MacroControlledObject::getProcessor() const
	{return processor.get(); }

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
		processor->setAttribute(parameterIndex, newValue, sendNotificationAsync);
		return true;
	}
	else return false;
}

bool MacroControlledObject::UndoableControlEvent::undo()
{
	if (processor.get() != nullptr)
	{
		processor->setAttribute(parameterIndex, oldValue, sendNotificationAsync);
		return true;
	}
	else return false;
}

namespace LearnableIcons
{
	static const unsigned char destination[] = { 110,109,164,176,141,67,70,246,138,66,98,82,216,144,67,231,251,113,66,123,116,148,67,18,131,80,66,80,125,152,67,180,72,50,66,98,227,133,167,67,162,69,131,65,135,214,187,67,240,167,70,62,94,154,208,67,0,0,0,0,98,207,199,208,67,0,0,0,0,63,245,208,67,0,0,
0,0,176,34,209,67,0,0,0,0,98,8,60,239,67,78,98,144,62,4,70,6,68,215,163,14,66,127,58,13,68,45,178,180,66,98,182,243,18,68,160,90,7,67,193,34,19,68,174,199,62,67,244,157,13,68,92,143,108,67,98,113,29,8,68,219,25,141,67,18,67,250,67,94,90,158,67,225,58,
225,67,195,101,163,67,98,76,71,199,67,131,160,168,67,248,3,171,67,248,131,160,67,129,197,151,67,184,14,142,67,98,78,34,149,67,43,135,139,67,133,171,146,67,203,209,136,67,63,101,144,67,162,245,133,67,108,27,207,160,67,66,192,118,67,98,246,24,175,67,217,
254,139,67,117,83,198,67,88,169,148,67,20,110,220,67,66,144,144,67,98,121,233,245,67,233,214,139,67,12,194,5,68,162,101,108,67,51,171,7,68,182,19,55,67,98,41,76,9,68,59,159,9,67,162,69,4,68,172,92,178,66,127,58,245,67,0,128,119,66,98,180,216,234,67,199,
203,60,66,72,65,222,67,84,99,28,66,133,123,209,67,125,63,27,66,98,195,53,209,67,94,58,27,66,0,240,208,67,88,57,27,66,61,170,208,67,94,58,27,66,98,16,24,189,67,188,244,27,66,213,216,169,67,184,158,106,66,244,253,157,67,143,2,181,66,108,164,176,141,67,
70,246,138,66,99,109,184,30,237,65,119,222,70,67,98,100,59,174,65,119,222,70,67,10,215,99,65,250,190,67,67,96,229,10,65,223,47,62,67,98,59,223,71,64,6,161,56,67,0,0,0,0,10,23,49,67,0,0,0,0,160,58,41,67,98,0,0,0,0,94,58,41,67,0,0,0,0,29,58,41,67,0,0,0,
0,219,57,41,67,98,0,0,0,0,111,178,24,67,102,102,86,65,199,75,11,67,164,112,239,65,199,75,11,67,98,209,98,192,66,199,75,11,67,76,119,121,67,199,75,11,67,76,119,121,67,199,75,11,67,108,76,119,121,67,63,245,167,66,108,246,184,190,67,254,20,41,67,108,76,
119,121,67,158,47,126,67,108,76,119,121,67,119,222,70,67,98,76,119,121,67,119,222,70,67,113,125,191,66,119,222,70,67,184,30,237,65,119,222,70,67,99,109,78,18,174,67,231,123,222,66,98,119,30,182,67,109,231,180,66,240,183,194,67,47,29,154,66,152,222,208,
67,47,29,154,66,98,254,36,233,67,47,29,154,66,94,218,252,67,45,242,232,66,94,218,252,67,37,6,37,67,98,94,218,252,67,242,146,85,67,254,36,233,67,113,253,124,67,152,222,208,67,113,253,124,67,98,250,94,196,67,113,253,124,67,63,21,185,67,127,138,114,67,143,
18,177,67,168,198,97,67,108,61,250,219,67,170,113,42,67,108,78,18,174,67,231,123,222,66,99,101,0,0 };

	static const unsigned char source[] = { 110,109,238,124,158,67,137,193,202,66,108,20,190,136,67,137,193,202,66,98,223,255,131,67,205,12,170,66,8,236,122,67,117,83,141,66,23,217,106,67,137,193,111,66,98,127,106,86,67,94,58,57,66,244,29,62,67,45,178,27,66,231,91,37,67,94,58,27,66,98,4,22,37,
67,88,57,27,66,33,208,36,67,88,57,27,66,127,138,36,67,94,58,27,66,98,29,218,214,66,217,78,28,66,39,177,82,66,137,129,162,66,156,68,36,66,244,189,12,67,98,2,43,4,66,190,223,53,67,115,104,59,66,219,153,98,67,100,59,155,66,244,13,128,67,98,84,99,213,66,
223,255,141,67,129,117,21,67,254,84,148,67,166,59,61,67,248,115,144,67,98,104,49,96,67,68,11,141,67,111,82,127,67,18,163,129,67,88,217,136,67,223,111,100,67,108,49,168,158,67,223,111,100,67,98,76,23,158,67,8,44,103,67,74,124,157,67,137,225,105,67,233,
214,156,67,92,143,108,67,98,80,189,145,67,14,77,141,67,188,244,118,67,174,151,158,67,121,169,68,67,92,127,163,67,98,240,7,17,67,82,136,168,67,186,9,178,66,180,104,160,67,25,4,75,66,184,14,142,67,98,190,159,194,65,111,98,129,67,70,182,211,64,90,100,96,
67,188,116,195,63,170,17,60,67,98,23,217,182,192,12,98,8,67,129,149,81,65,100,59,163,66,72,225,77,66,250,254,52,66,98,147,88,163,66,231,251,132,65,156,132,245,66,131,192,74,62,193,106,36,67,0,0,0,0,98,162,197,36,67,0,0,0,0,131,32,37,67,0,0,0,0,100,123,
37,67,0,0,0,0,98,236,17,88,67,143,194,117,62,254,228,132,67,219,249,198,65,115,72,148,67,143,2,131,66,98,221,116,152,67,74,12,153,66,152,222,155,67,100,59,177,66,238,124,158,67,137,193,202,66,99,109,43,103,37,67,141,55,67,67,98,193,138,29,67,141,55,67,
67,197,0,22,67,16,24,64,67,236,113,16,67,55,137,58,67,98,209,226,10,67,29,250,52,67,84,195,7,67,33,112,45,67,84,195,7,67,182,147,37,67,98,84,195,7,67,117,147,37,67,84,195,7,67,117,147,37,67,84,195,7,67,51,147,37,67,98,84,195,7,67,133,11,21,67,186,41,
21,67,31,165,7,67,104,177,37,67,31,165,7,67,98,254,244,103,67,31,165,7,67,113,157,192,67,31,165,7,67,113,157,192,67,31,165,7,67,108,113,157,192,67,240,167,160,66,108,80,77,1,68,86,110,37,67,108,113,157,192,67,180,136,122,67,108,113,157,192,67,141,55,
67,67,98,113,157,192,67,141,55,67,67,12,130,103,67,141,55,67,67,43,103,37,67,141,55,67,67,99,101,0,0 };

}

TouchAndHoldComponent::TouchAndHoldComponent():
	updateTimer(this)
{

}

TouchAndHoldComponent::~TouchAndHoldComponent()
{
	abortTouch();
}

void TouchAndHoldComponent::startTouch(Point<int> downPosition)
{
	if (isTouchEnabled())
	{
		updateTimer.startTouch(downPosition);
	}
}

void TouchAndHoldComponent::setDragDistance(float newDistance)
{
	updateTimer.setDragDistance(newDistance);
}

void TouchAndHoldComponent::abortTouch()
{
	updateTimer.stopTimer();
}

bool TouchAndHoldComponent::isTouchEnabled() const
{
	return touchEnabled && HiseDeviceSimulator::isMobileDevice();
}

void TouchAndHoldComponent::setTouchEnabled(bool shouldBeEnabled)
{
	touchEnabled = shouldBeEnabled;
}

TouchAndHoldComponent::UpdateTimer::UpdateTimer(TouchAndHoldComponent* parent_):
	parent(parent_),
	dragDistance(0.0f)
{}

void TouchAndHoldComponent::UpdateTimer::startTouch(Point<int>& newDownPosition)
{
	downPosition = newDownPosition;
	startTimer(1000);
}

void TouchAndHoldComponent::UpdateTimer::setDragDistance(float newDistance)
{
	dragDistance = newDistance;
}

void TouchAndHoldComponent::UpdateTimer::timerCallback()
{
	stopTimer();

	if (dragDistance < 8.0f)
	{
		parent->touchAndHold(downPosition);
	}
}

juce::Path Learnable::Factory::createPath(const String& url) const
{
	Path p;
	
	LOAD_PATH_IF_URL("destination", LearnableIcons::destination);
	LOAD_PATH_IF_URL("source", LearnableIcons::source);

	return p;
}

} // namespace hise
