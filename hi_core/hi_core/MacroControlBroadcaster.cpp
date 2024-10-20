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

	MacroControlBroadcaster::MacroConnectionListener::~MacroConnectionListener()
	{}

	MacroControlBroadcaster::~MacroControlBroadcaster()
	{}

	void MacroControlBroadcaster::MacroControlledParameterData::setInverted(bool shouldBeInverted)
	{ inverted = shouldBeInverted; }

	void MacroControlBroadcaster::MacroControlledParameterData::setIsCustomAutomation(bool shouldBeCustomAutomation)
	{ customAutomation = shouldBeCustomAutomation; }

	void MacroControlBroadcaster::MacroControlledParameterData::setReadOnly(bool shouldBeReadOnly)
	{ readOnly = shouldBeReadOnly; }

	bool MacroControlBroadcaster::MacroControlledParameterData::isReadOnly() const
	{ return readOnly;}

	bool MacroControlBroadcaster::MacroControlledParameterData::isInverted() const
	{return inverted; }

	bool MacroControlBroadcaster::MacroControlledParameterData::isCustomAutomation() const
	{ return customAutomation; }

	NormalisableRange<double> MacroControlBroadcaster::MacroControlledParameterData::getTotalRange() const
	{ return range; }

	NormalisableRange<double> MacroControlBroadcaster::MacroControlledParameterData::getParameterRange() const
	{return parameterRange;}

	void MacroControlBroadcaster::MacroControlledParameterData::setRangeStart(double min)
	{parameterRange.start = min; }

	void MacroControlBroadcaster::MacroControlledParameterData::setRangeEnd(double max)
	{	parameterRange.end = max; }

	Processor* MacroControlBroadcaster::MacroControlledParameterData::getProcessor()
	{return controlledProcessor.get(); }

	const Processor* MacroControlBroadcaster::MacroControlledParameterData::getProcessor() const
	{return controlledProcessor.get(); }

	int MacroControlBroadcaster::MacroControlledParameterData::getParameter() const
	{	return parameter; }

	void MacroControlBroadcaster::MacroControlledParameterData::setParameterIndex(int newParameter)
	{
		parameter = newParameter;
	}

	String MacroControlBroadcaster::MacroControlledParameterData::getParameterName() const
	{ return parameterName; }

	void MacroControlBroadcaster::sendMacroConnectionChangeMessage(int macroIndex, Processor* p, int parameterIndex,
		bool wasAdded, NotificationType n)
	{
		if (n == dontSendNotification)
			return;

		if(n == sendNotificationAsync)
		{
			WeakReference<MacroControlBroadcaster> safeParent(this);
			WeakReference<Processor> safeP(p);

			auto f = [safeParent, macroIndex, safeP, parameterIndex, wasAdded]()
			{
				if(safeParent && safeP)
					safeParent->sendMacroConnectionChangeMessage(macroIndex, safeP, parameterIndex, wasAdded, sendNotificationSync);
			};

			MessageManager::callAsync(f);
			return;
		}

		// Should only be called from the message thread
		jassert(MessageManager::getInstance()->isThisTheMessageThread());

        {
            ScopedLock sl(listenerLock);
            
            for (auto l : macroListeners)
            {
                if (l != nullptr)
                    l->macroConnectionChanged(macroIndex, p, parameterIndex, wasAdded);
            }
        }
        
#if HISE_MACROS_ARE_PLUGIN_PARAMETERS
		auto ap = dynamic_cast<AudioProcessor*>(dynamic_cast<ControlledObject*>(this)->getMainController());

		jassert(ap != nullptr);

		AudioProcessorListener::ChangeDetails d;
		ap->updateHostDisplay(d.withParameterInfoChanged(true));

		auto md = getMacroControlData(macroIndex);

        SimpleReadWriteLock::ScopedReadLock sl2(md->parameterLock);
        
		if(wasAdded && md->getNumParameters() == 1)
		{
			float newValue;

			auto pd = md->getParameter(0);

			if(pd->isCustomAutomation())
			{
				auto& uph = dynamic_cast<ControlledObject*>(this)->getMainController()->getUserPresetHandler();

				if (uph.getCustomAutomationIndex(Identifier(pd->getParameterName())) == pd->getParameter())
				{
					newValue = uph.getCustomAutomationData(pd->getParameter())->lastValue;
				}
				else
					newValue = 0.0f;
			}
			else
			{
				newValue = p->getAttribute(parameterIndex);
				
			}

			newValue = pd->getParameterRange().convertTo0to1(newValue);
			
			ap->setParameterNotifyingHost(macroIndex, newValue);
		}

#endif
	}

	void MacroControlBroadcaster::sendMacroConnectionChangeMessageForAll(bool wasAdded)
	{
		struct AsyncData
		{
			int index;
			WeakReference<Processor> p;
			int parameter;
			bool wasAdded;
		};

		Array<AsyncData> data;

		for (auto m : macroControls)
		{
            SimpleReadWriteLock::ScopedReadLock sl(m->parameterLock);
            
			auto index = m->macroIndex;

			for (int i = 0; i < m->getNumParameters(); i++)
			{
				if (auto p = m->getParameter(i))
					data.add({ index, p->getProcessor(), p->getParameter(), wasAdded });
			}
		}

		if (!data.isEmpty())
		{
			WeakReference<MacroControlBroadcaster> safeThis(this);

			auto f = [data, safeThis]()
			{
				if (safeThis != nullptr)
				{
					for (auto d : data)
					{
						if(d.p != nullptr)
							safeThis.get()->sendMacroConnectionChangeMessage(d.index, d.p.get(), d.parameter, d.wasAdded);
					}
				}
				
			};

			MessageManager::callAsync(f);
		}
	}

	void MacroControlBroadcaster::addMacroConnectionListener(MacroConnectionListener* l)
	{
		{
			ScopedLock sl(listenerLock);
			macroListeners.addIfNotAlreadyThere(l);
		}
        

#if USE_BACKEND
		for(int i = 0; i < HISE_NUM_MACROS; i++)
		{
			auto md = getMacroControlData(i);

			for(int j = 0; j < md->getNumParameters(); j++)
			{
				auto pc = md->getParameter(j);

				l->macroConnectionChanged(i, pc->getProcessor(), pc->getParameter(), true);
			}
		}
#endif
	}

	void MacroControlBroadcaster::removeMacroConnectionListener(MacroConnectionListener* l)
	{
        ScopedLock sl(listenerLock);
		macroListeners.removeAllInstancesOf(l);
	}

	MacroControlBroadcaster::MacroControlData::MacroControlData(int index, MacroControlBroadcaster& parent_, MainController* mc):
        ControlledObject(mc),
		macroName("Macro " + String(index + 1)),
		currentValue(0.0),
		midiController(-1),
		parent(parent_),
		macroIndex(index)
	{}

	MacroControlBroadcaster::MacroControlData::~MacroControlData()
	{
		controlledParameters.clear();
	}

	MacroControlBroadcaster::MacroControlledParameterData* MacroControlBroadcaster::MacroControlData::getParameter(
		int parameterIndex)
	{
		return controlledParameters[parameterIndex];
	}

	const MacroControlBroadcaster::MacroControlledParameterData* MacroControlBroadcaster::MacroControlData::
	getParameter(int parameterIndex) const
	{
		return controlledParameters[parameterIndex];
	}

	MacroControlBroadcaster::MacroControlledParameterData* MacroControlBroadcaster::MacroControlData::
	getParameterWithProcessorAndIndex(Processor* p, int parameterIndex)
	{
		for(int i = 0; i < controlledParameters.size(); i++)
		{
			if(controlledParameters[i]->getProcessor() == p && controlledParameters[i]->getParameter() == parameterIndex)
			{
				return controlledParameters[i];
			}
		}

		return nullptr;
	}

	MacroControlBroadcaster::MacroControlledParameterData* MacroControlBroadcaster::MacroControlData::
	getParameterWithProcessorAndName(Processor* p, const String& parameterName)
	{
		for(int i = 0; i < controlledParameters.size(); i++)
		{
			if(controlledParameters[i]->getProcessor() == p && controlledParameters[i]->getParameterName() == parameterName)
			{
				return controlledParameters[i];
			}
		}

		return nullptr;
	}

	String MacroControlBroadcaster::MacroControlData::getMacroName() const
	{return macroName; }

	void MacroControlBroadcaster::MacroControlData::setMacroName(const String& name)
	{
		macroName = name;
	}

	int MacroControlBroadcaster::MacroControlData::getNumParameters() const
	{ return controlledParameters.size(); }

	void MacroControlBroadcaster::MacroControlData::setMidiController(int newControllerNumber)
	{ 
		midiController = newControllerNumber;	
	}

	int MacroControlBroadcaster::MacroControlData::getMidiController() const noexcept
	{ return midiController; }

	MacroControlBroadcaster::MacroControlData* MacroControlBroadcaster::getMacroControlData(int index)
	{
		return macroControls[index];
	}

	const MacroControlBroadcaster::MacroControlData* MacroControlBroadcaster::getMacroControlData(int index) const
	{
		return macroControls[index];
	}

	void MacroControlBroadcaster::clearAllMacroControls()
	{
		const int numMacros = macroControls.size();
        
		for(int i = 0; i < numMacros; i++)
		{
			clearData(i);
		}
	}

	bool MacroControlBroadcaster::hasActiveParameters(int macroIndex)
	{
		return macroControls[macroIndex]->getNumParameters() != 0;
	}

MacroControlBroadcaster::MacroControlBroadcaster(ModulatorSynthChain *chain):
	thisAsSynth(chain)
{
	for(int i = 0; i < HISE_NUM_MACROS; i++)
	{
		macroControls.add(new MacroControlData(i, *this, chain->getMainController()));
	}
}


/** Creates a new Parameter data object. */
MacroControlBroadcaster::MacroControlledParameterData::MacroControlledParameterData(Processor *p, int  parameter_, const String &parameterName_, const ValueToTextConverter& converter_, NormalisableRange<double> range_, bool readOnly):
    ControlledObject(p->getMainController()),
	controlledProcessor(p),
	id(p->getId()),
	parameter(parameter_),
	parameterName(parameterName_),
	textConverter(converter_),
	range(range_),
	parameterRange(range_),
	inverted(false),
	readOnly(readOnly)
{};

MacroControlBroadcaster::MacroControlledParameterData::MacroControlledParameterData(MainController* mc):
  ControlledObject(mc),
  id(""),
  parameter(-1),
  parameterName(""),
  textConverter({}),
  controlledProcessor(nullptr),
  range(0.0, 1.0),
  parameterRange(0.0, 1.0),
  inverted(false),
  readOnly(true),
  customAutomation(false)
{};

/** Allows comparison. This only compares the Processor and the parameter (not the range). */
bool MacroControlBroadcaster::MacroControlledParameterData::operator== (const MacroControlledParameterData& other) const
{
	return other.id == id && other.parameter == parameter;
}

void MacroControlBroadcaster::MacroControlledParameterData::setAttribute(double normalizedInputValue)
{
	const float value = getNormalizedValue(normalizedInputValue);
	
	if(controlledProcessor.get() != nullptr)
	{
		if (customAutomation)
		{
			if (auto d = controlledProcessor->getMainController()->getUserPresetHandler().getCustomAutomationData(parameter))
				d->call(value, dispatch::DispatchType::sendNotificationSync);
		}
		else
			controlledProcessor.get()->setAttribute(parameter, value, readOnly ? sendNotificationSync : dontSendNotification);
	}
	
};

		

bool MacroControlBroadcaster::MacroControlledParameterData::matchesCustomAutomation(const Identifier& id) const
{
	if (controlledProcessor == nullptr)
		return false;

	if (!customAutomation)
		return false;

	if (auto ad = controlledProcessor->getMainController()->getUserPresetHandler().getCustomAutomationData(parameter))
		return Identifier(ad->id) == id;

	return false;
}

double MacroControlBroadcaster::MacroControlledParameterData::getParameterRangeLimit(bool getHighLimit) const
{
	return getHighLimit ? parameterRange.end : parameterRange.start;
};

float MacroControlBroadcaster::MacroControlledParameterData::getNormalizedValue(double normalizedSliderInput)
{
	return (float)(parameterRange.convertFrom0to1 ( inverted ? (1.0 - normalizedSliderInput) : normalizedSliderInput));

	
	//return inverted ? (float)(parameterRange.getEnd() - normalizedSliderInput * parameterRange.getLength()):
	//				  (float)(parameterRange.getStart() + normalizedSliderInput * parameterRange.getLength());
};

		

ValueTree MacroControlBroadcaster::MacroControlledParameterData::exportAsValueTree() const
{
    ValueTree v("controlled_parameter");

	v.setProperty("id", controlledProcessor->getId(), nullptr);
	v.setProperty("parameter", parameter, nullptr);
	v.setProperty("parameter_name", parameterName, nullptr);
	v.setProperty("min", range.start, nullptr);
	v.setProperty("max", range.end, nullptr);
	v.setProperty("low", parameterRange.start, nullptr);
	v.setProperty("high", parameterRange.end, nullptr);
	v.setProperty("skew", parameterRange.skew, nullptr);
	v.setProperty("step", parameterRange.interval, nullptr);
	v.setProperty("inverted", inverted, nullptr);
	v.setProperty("readonly", readOnly, nullptr);
	v.setProperty("converter", textConverter.toString(), nullptr);

	return v;
}

void MacroControlBroadcaster::MacroControlledParameterData::restoreFromValueTree(const ValueTree& v)
{
    id = v.getProperty("id", id).toString();
    parameter = v.getProperty("parameter", -1);
    parameterName = v.getProperty("parameter_name", "").toString();
    
    range = NormalisableRange<double>(
                             (double)v.getProperty("min", 0.0),
                             (double)v.getProperty("max", 1.0));
    
    parameterRange = NormalisableRange<double>(
                                      (double)v.getProperty("low", 0.0),
                                      (double)v.getProperty("high", 1.0));
    
    parameterRange.skew = v.getProperty("skew", 1.0);
    parameterRange.interval = v.getProperty("step", 0.0);
	range.skew = parameterRange.skew;
    inverted = v.getProperty("inverted", false);
    readOnly = v.getProperty("readonly", true);
	textConverter = ValueToTextConverter::fromString(v.getProperty("converter", ""));

    controlledProcessor = findProcessor(getMainController()->getMainSynthChain(), id);

    if(controlledProcessor == nullptr)
        return;
    
    if (controlledProcessor->getIdentifierForParameterIndex(parameter).toString().compare(parameterName) != 0)
    {
        // parameter name vs id mismatch, we'll resolve it now...
        
        Identifier pToLookFor(parameterName);
        auto& uph = controlledProcessor->getMainController()->getUserPresetHandler();

        if (uph.isUsingCustomDataModel())
        {
            if (auto d = uph.getCustomAutomationData(pToLookFor))
            {
                parameter = d->index;
                setIsCustomAutomation(true);
            }
        }
        else
        {
            int numParameters = controlledProcessor->getNumParameters();

            for (int i = 0; i < numParameters; i++)
            {
                if (controlledProcessor->getIdentifierForParameterIndex(i) == pToLookFor)
                {
                    parameter = i;
                    break;
                }
            }
        }
    }
}


void MacroControlBroadcaster::saveMacrosToValueTree(ValueTree &v) const
{
    ValueTree m("macro_controls");

	for (auto macro: macroControls)
        m.addChild(macro->exportAsValueTree(), -1, nullptr);

	v.addChild(m, -1, nullptr);
}

void MacroControlBroadcaster::saveMacroValuesToValueTree(ValueTree &v) const
{
    ValueTree mData("macro_controls");

	for (auto macro: macroControls)
	{
        SimpleReadWriteLock::ScopedReadLock sl(macro->parameterLock);
        
		if (macro->getNumParameters() > 0)
		{
			ValueTree child("macro");
			child.setProperty("value", macro->getCurrentValue(), nullptr);
            mData.addChild(child, -1, nullptr);
		}
	}

	v.addChild(mData, -1, nullptr);
}

void MacroControlBroadcaster::loadMacrosFromValueTree(const ValueTree &v, bool loadMacroValues)
{
	ValueTree macroData = v.getChildWithName("macro_controls");

	if(macroData.isValid())
	{
		sendMacroConnectionChangeMessageForAll(false);

        int numToRestore = jmin(HISE_NUM_MACROS, macroControls.size(), macroData.getNumChildren());
        
        for(int i = 0; i < numToRestore; i++)
        {
            macroControls[i]->restoreFromValueTree(macroData.getChild(i));
        }
        
		sendMacroConnectionChangeMessageForAll(true);

		for(int i = 0; i < macroControls.size(); i++)
		{
			setMacroControl(i, macroControls[i]->getCurrentValue(), sendNotification);
		}	
	}

	if(loadMacroValues)
		loadMacroValuesFromValueTree(v);
}

void MacroControlBroadcaster::loadMacroValuesFromValueTree(const ValueTree &v)
{
    auto data = v.getChildWithName("macro_controls");

    if(!data.isValid())
    {
        // The macro controls could not be found...
        return;
    }
    
    int numToRestore = jmin(HISE_NUM_MACROS, macroControls.size(), data.getNumChildren());
    
	for (int i = 0; i < numToRestore; i++)
	{
        auto value = data.getChild(i).getProperty("value", 0.0);
        setMacroControl(i, value, sendNotification);
	}
}

float MacroControlBroadcaster::MacroControlData::getCurrentValue() const
{
	return currentValue;
}

float MacroControlBroadcaster::MacroControlData::getDisplayValue() const
{
	if(getNumParameters() != 0)
	{
		const double v =  currentValue / 127.0;

		NormalisableRange<double> r = getParameter(0)->getParameterRange();

		return (float)r.convertFrom0to1(v);

	}

	return currentValue;
}

void MacroControlBroadcaster::clearData(int macroIndex)
{
	MacroControlData *data = macroControls[macroIndex];
    MacroControlData copy(macroIndex, *this, dynamic_cast<ControlledObject*>(this)->getMainController());
    
    auto v = copy.exportAsValueTree();
    
    data->restoreFromValueTree(v);
    
	thisAsSynth->sendOtherChangeMessage(dispatch::library::ProcessorChangeEvent::Macro);
}

void MacroControlBroadcaster::MacroControlData::setValue(float newValue)
{
	currentValue = newValue;

    SimpleReadWriteLock::ScopedReadLock sl(parameterLock);
    
	for(auto p: controlledParameters)
		p->setAttribute(newValue / 127.0f);
};

bool MacroControlBroadcaster::MacroControlData::isDanglingProcessor(int parameterIndex) const
{
    SimpleReadWriteLock::ScopedReadLock sl(parameterLock);
    
    if(isPositiveAndBelow(parameterIndex, controlledParameters.size()))
    {
        jassert( controlledParameters[parameterIndex]->getProcessor() != nullptr);
        return controlledParameters[parameterIndex]->getProcessor() == nullptr;
    }
    
    return true;
}

void MacroControlBroadcaster::MacroControlData::removeParametersFromIndexList(const Array<int>& indexList, NotificationType n)
{
    if(indexList.isEmpty())
        return;
    
    OwnedArray<MacroControlledParameterData> pendingDelete;
    pendingDelete.ensureStorageAllocated(indexList.size());
    
    {
        SimpleReadWriteLock::ScopedReadLock sl(parameterLock);
        
        for(auto i: indexList)
        {
            auto cp = controlledParameters[i];
            
            if(cp->getProcessor() == nullptr)
                continue;
            
            cp->getProcessor()->sendOtherChangeMessage(dispatch::library::ProcessorChangeEvent::Macro);
            
            parent.sendMacroConnectionChangeMessage(macroIndex, cp->getProcessor(), cp->getParameter(), false, n);
        }
    }
    
    {
        SimpleReadWriteLock::ScopedWriteLock sl(parameterLock);
        
        for(auto l: indexList)
        {
            pendingDelete.add(controlledParameters[l]);
            controlledParameters.set(l, nullptr, false);
        }
        
        for(int i = 0; i < controlledParameters.size(); i++)
        {
            if(controlledParameters[i] == nullptr)
            {
                controlledParameters.remove(i--);
            }
        }
    }
    
    pendingDelete.clear();
}

void MacroControlBroadcaster::MacroControlData::clearDanglingProcessors()
{
    Array<int> indexList;
    
    {
        SimpleReadWriteLock::ScopedReadLock sl(parameterLock);
        
        for(int i = 0; i < controlledParameters.size(); i++)
        {
            if(isDanglingProcessor(i))
            {
                indexList.add(i);
            }
        }
    }
    
    removeParametersFromIndexList(indexList);
}

void MacroControlBroadcaster::MacroControlData::removeAllParametersWithProcessor(Processor *p)
{
    Array<int> indexList;
    
    {
        SimpleReadWriteLock::ScopedReadLock sl(parameterLock);
        
        for(auto cp: controlledParameters)
        {
            if (cp->getProcessor() == p)
                indexList.add(controlledParameters.indexOf(cp));
        }
    }
    
    removeParametersFromIndexList(indexList);
}

ValueTree MacroControlBroadcaster::MacroControlData::exportAsValueTree() const
{
    ValueTree v("macro");
    v.setProperty("name", macroName, nullptr);
    v.setProperty("value", currentValue, nullptr);
    v.setProperty("midi_cc", midiController, nullptr);

	for(int i = 0; i < controlledParameters.size(); i++)
	{
		if( !isDanglingProcessor(i))
            v.addChild(controlledParameters[i]->exportAsValueTree(), -1, nullptr);
            
	}
	return v;
};

void MacroControlBroadcaster::MacroControlData::restoreFromValueTree(const ValueTree& v)
{
    currentValue = 0.0f;

    jassert(v.getType() == Identifier("macro"));


	auto& mm = dynamic_cast<ModulatorSynthChain*>(&parent)->getMainController()->getMacroManager();

	// Do not overwrite the macro names set by `setFrontendMacros()` ever...
	if(!mm.isMacroEnabledOnFrontend())
		macroName = v.getProperty("name", "Macro " + String(macroIndex+1)).toString();

    setValue((float)v.getProperty("value", 0.0));

    setMidiController(v.getProperty("midi_cc", -1));

    OwnedArray<MacroControlledParameterData> newData;
    
    for(auto c: v)
    {
        auto nd = new MacroControlledParameterData(getMainController());
        nd->restoreFromValueTree(c);
        newData.add(nd);
    }
    
    {
        
        SimpleReadWriteLock::ScopedWriteLock sl(parameterLock);
        std::swap(newData, controlledParameters);
    }
}

bool MacroControlBroadcaster::MacroControlData::hasParameter(Processor *p, int parameterIndex)
{
    SimpleReadWriteLock::ScopedReadLock sl(parameterLock);
    
    for(auto cp: controlledParameters)
	{
		if(cp->getProcessor() == p &&
           cp->getParameter() == parameterIndex)
		{
			return true;
		}
	}

	return false;
}


void MacroControlBroadcaster::MacroControlData::addParameter(Processor *p, int parameterId, const String &parameterName, const ValueToTextConverter& converter, NormalisableRange<double> range, bool readOnly, bool isUsingCustomData, NotificationType n)
{
    if(p->getMainController()->getMacroManager().isExclusive())
    {
        Array<int> indexList;
        
        {
            SimpleReadWriteLock::ScopedReadLock sl(parameterLock);
            
            for(int i = 1; i < controlledParameters.size(); i++)
            {
                indexList.add(i);
            }
        }
        
        removeParametersFromIndexList(indexList);
    }
    
    auto nd = new MacroControlledParameterData(p,
                                               parameterId,
                                               parameterName,
											   converter,
                                               range,
                                               readOnly);
    
    nd->setIsCustomAutomation(isUsingCustomData);
    
    {
        SimpleReadWriteLock::ScopedWriteLock sl(parameterLock);
        controlledParameters.add(nd);
    }
    
	parent.sendMacroConnectionChangeMessage(macroIndex, p, parameterId, true, n);
}

Processor *MacroControlBroadcaster::findProcessor(Processor *p, const String &idToSearch)
{
	if (p->getId() == idToSearch) return p;

	for(int i = 0; i < p->getNumChildProcessors(); i++)
	{
		Processor *c = findProcessor(p->getChildProcessor(i), idToSearch);
				
		if(c != nullptr) return c;
	}

	return nullptr;
}

void MacroControlBroadcaster::setMacroControl(int macroIndex, float newValue, NotificationType notifyEditor)
{
	MacroControlData *data = getMacroControlData(macroIndex);

	data->setValue(newValue);

	if(notifyEditor == sendNotificationAsync)
	{
		thisAsSynth->sendOtherChangeMessage(dispatch::library::ProcessorChangeEvent::Macro);

	}
	else if (notifyEditor == sendNotification)
	{
		thisAsSynth->sendOtherChangeMessage(dispatch::library::ProcessorChangeEvent::Macro);

		AudioProcessor *p = dynamic_cast<AudioProcessor*>(thisAsSynth->getMainController());

		jassert(p != nullptr);

		// Skip sending parameter changes before everything is loaded
		if(macroIndex >= p->getNumParameters()) return;

#if USE_BACKEND
		p->setParameterNotifyingHost(macroIndex, newValue / 127.0f);
#endif
	}
}

int MacroControlBroadcaster::getMacroControlIndexForCustomAutomation(const Identifier& customId) const
{
	for (int i = 0; i < macroControls.size(); i++)
	{
        SimpleReadWriteLock::ScopedReadLock sl(macroControls[i]->parameterLock);
        
		for (int j = 0; j < macroControls[i]->getNumParameters(); j++)
		{
			if (macroControls[i]->getParameter(j)->matchesCustomAutomation(customId))
				return i;
		}
	}

	return -1;
}

int MacroControlBroadcaster::getMacroControlIndexForProcessorParameter(const Processor *p, int parameter) const
{
	for (int i = 0; i < macroControls.size(); i++)
	{
        SimpleReadWriteLock::ScopedReadLock sl(macroControls[i]->parameterLock);
        
		for (int j = 0; j < macroControls[i]->getNumParameters(); j++)
		{
			auto pa = macroControls[i]->getParameter(j);

			if (pa->isCustomAutomation())
				continue;

			if (pa->getProcessor() == p &&
				pa->getParameter() == parameter)
			{
				return i;
			}
		}
	}

	return -1;
}

void MacroControlBroadcaster::addControlledParameter(int macroControllerIndex, 
							const String &processorId, 
							int parameterId, 
							const String& parameterName,
					        const ValueToTextConverter& converter,
							NormalisableRange<double> range,
							bool readOnly)
{
	Processor *p = findProcessor(thisAsSynth, processorId);

	if(p != nullptr)
	{
		//if(macroControls[macroControllerIndex]->hasParameter(p, parameterId)) return;

		for(int i = 0; i < macroControls.size(); i++)
		{
			if(macroControls[i]->hasParameter(p, parameterId))
			{
				macroControls[i]->removeParameter(parameterName);
			}
		}

		macroControls[macroControllerIndex]->addParameter(p, parameterId, parameterName, converter, range, readOnly);

		p->sendOtherChangeMessage(dispatch::library::ProcessorChangeEvent::Macro);
		thisAsSynth->sendOtherChangeMessage(dispatch::library::ProcessorChangeEvent::Macro);
	}
}

void MacroControlBroadcaster::MacroControlData::removeParameter(int parameterIndex, NotificationType n)
{
    removeParametersFromIndexList({parameterIndex}, n);
};

void MacroControlBroadcaster::MacroControlData::removeParameter(const String &parameterName, const Processor *processor, NotificationType n)
{ 
	int index = -1;

	for(int i = 0; i < controlledParameters.size(); i++)
	{
		auto pa = controlledParameters[i];

		if (pa->isCustomAutomation())
		{
			auto& uph = pa->getProcessor()->getMainController()->getUserPresetHandler();

			if (uph.getCustomAutomationIndex(Identifier(parameterName)) == pa->getParameter())
			{
				index = i;
				break;
			}
		}
		else
		{
			const bool isProcessor = processor == nullptr || pa->getProcessor() == processor;

			if (pa->getParameterName() == parameterName && isProcessor)
			{
				index = i;
				break;
			}
		}
	};

	if(index == -1) return;

	removeParameter(index, n);
};

} // namespace hise
