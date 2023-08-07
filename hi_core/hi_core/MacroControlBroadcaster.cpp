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
		bool wasAdded)
	{
		// Should only be called from the message thread
		jassert(MessageManager::getInstance()->isThisTheMessageThread());

		for (auto l : macroListeners)
		{
			if (l != nullptr)
				l->macroConnectionChanged(macroIndex, p, parameterIndex, wasAdded);
		}
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

		SimpleReadWriteLock::ScopedReadLock sl(macroLock);

		for (auto m : macroControls)
		{
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
		macroListeners.addIfNotAlreadyThere(l);
	}

	void MacroControlBroadcaster::removeMacroConnectionListener(MacroConnectionListener* l)
	{
		macroListeners.removeAllInstancesOf(l);
	}

	MacroControlBroadcaster::MacroControlData::MacroControlData(int index, MacroControlBroadcaster& parent_):
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
		SimpleReadWriteLock::ScopedReadLock sl(macroLock);
		return macroControls[index];	
	}

	const MacroControlBroadcaster::MacroControlData* MacroControlBroadcaster::getMacroControlData(int index) const
	{
		SimpleReadWriteLock::ScopedReadLock sl(macroLock);
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
	SimpleReadWriteLock::ScopedWriteLock sl(macroLock);

	for(int i = 0; i < HISE_NUM_MACROS; i++)
	{
		macroControls.add(new MacroControlData(i, *this));
		
	}
}


/** Creates a new Parameter data object. */
MacroControlBroadcaster::MacroControlledParameterData::MacroControlledParameterData(Processor *p, int  parameter_, const String &parameterName_, NormalisableRange<double> range_, bool readOnly):
	controlledProcessor(p),
	id(p->getId()),
	parameter(parameter_),
	parameterName(parameterName_),
	range(range_),
	parameterRange(range_),
	inverted(false),
	readOnly(readOnly)
{};

/** Restores a Parameter object from an exported XML document. 
*
*	You have to supply the ModulatorSynthChain to find the Processor with the exported ID.
*/
MacroControlBroadcaster::MacroControlledParameterData::MacroControlledParameterData(ModulatorSynthChain *chain, XmlElement &xml):
	range(NormalisableRange<double>(xml.getDoubleAttribute("min", 0.0),
							xml.getDoubleAttribute("max", 1.0))),
	parameter(xml.getIntAttribute("parameter", -1)),
	parameterName(xml.getStringAttribute("parameter_name", "")),
	id (xml.getStringAttribute("id", id)),
	readOnly(xml.getBoolAttribute("readonly", true))
{
	parameterRange = NormalisableRange<double>(xml.getDoubleAttribute("low", 0.0), xml.getDoubleAttribute("high", 1.0));
	inverted = xml.getBoolAttribute("inverted", false);
	controlledProcessor = findProcessor(chain, id);

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
				d->call(value);
		}
		else
			controlledProcessor.get()->setAttribute(parameter, value, readOnly ? sendNotification : dontSendNotification);
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

		

XmlElement *MacroControlBroadcaster::MacroControlledParameterData::exportAsXml()
{
	XmlElement *entry = new XmlElement("controlled_parameter");

	

	entry->setAttribute("id", controlledProcessor->getId());
	entry->setAttribute("parameter", parameter);
	entry->setAttribute("parameter_name", parameterName);
	entry->setAttribute("min", range.start);
	entry->setAttribute("max", range.end);
	entry->setAttribute("low", parameterRange.start);
	entry->setAttribute("high", parameterRange.end);
	entry->setAttribute("inverted", inverted);
	entry->setAttribute("readonly", readOnly);

	return entry;
}



MacroControlBroadcaster::MacroControlData::MacroControlData(ModulatorSynthChain *chain, int index, XmlElement *xml):
	parent(*chain),
	macroIndex(index)
{
	currentValue = 0.0f;

	jassert(xml->getTagName() == "macro");

	macroName = xml->getStringAttribute("name");

	setValue((float)xml->getDoubleAttribute("value", 0.0));

	setMidiController(xml->getIntAttribute("midi_cc", -1));

	for(int i = 0; i < xml->getNumChildElements(); i++)
	{
		controlledParameters.add(new MacroControlledParameterData(chain, *xml->getChildElement(i)));
	}

};

void MacroControlBroadcaster::saveMacrosToValueTree(ValueTree &v) const
{
	ScopedPointer<XmlElement> macroControlData = new XmlElement("macro_controls");

	SimpleReadWriteLock::ScopedReadLock sl(macroLock);

	for (int i = 0; i < macroControls.size(); i++)
	{
		macroControlData->addChildElement(macroControls[i]->exportAsXml());
	}

	ValueTree macros = ValueTree::fromXml(*macroControlData);

	v.addChild(macros, -1, nullptr);
}

void MacroControlBroadcaster::saveMacroValuesToValueTree(ValueTree &v) const
{
	ScopedPointer<XmlElement> macroControlData = new XmlElement("macro_controls");

	SimpleReadWriteLock::ScopedReadLock sl(macroLock);

	for (int i = 0; i < macroControls.size(); i++)
	{
		if (getMacroControlData(i)->getNumParameters() > 0)
		{
			XmlElement *child = new XmlElement("macro");

			child->setAttribute("value", getMacroControlData(i)->getCurrentValue());

			macroControlData->addChildElement(child);
		}
	}

	ValueTree macros = ValueTree::fromXml(*macroControlData);

	v.addChild(macros, -1, nullptr);
}

void MacroControlBroadcaster::loadMacrosFromValueTree(const ValueTree &v, bool loadMacroValues)
{
	ValueTree macroData = v.getChildWithName("macro_controls");

	auto data = macroData.createXml();

	if(data != nullptr && data->getNumChildElements() == HISE_NUM_MACROS)
	{
		sendMacroConnectionChangeMessageForAll(false);

		{
			SimpleReadWriteLock::ScopedWriteLock sl(macroLock);

			macroControls.clear();

			for (int i = 0; i < data->getNumChildElements(); i++)
			{
				macroControls.add(new MacroControlData(thisAsSynth, i, data->getChildElement(i)));
				thisAsSynth->getMainController()->getMacroManager().setMidiControllerForMacro(i, macroControls.getLast()->getMidiController());
			}
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
	auto data = v.getChildWithName("macro_controls").createXml();

    if(data == nullptr)
    {
        // The macro controls could not be found...
        jassertfalse;
        return;
    }
    
	SimpleReadWriteLock::ScopedReadLock sl(macroLock);

	for (int i = 0; i < macroControls.size(); i++)
	{
		XmlElement *child = data->getChildElement(i);

		if (child != nullptr)
		{
			const float value = (float)child->getDoubleAttribute("value", 0.0);

			setMacroControl(i, value, sendNotification);
		}
	}
}

void MacroControlBroadcaster::replaceMacroControlData(int index, MacroControlData *newData, ModulatorSynthChain *parentChain)
{
	SimpleReadWriteLock::ScopedReadLock sl(macroLock);

	if(index < macroControls.size())
	{
		ScopedPointer<XmlElement> xml = newData->exportAsXml();

		MacroControlData *copy = new MacroControlData(parentChain, index, xml);

		macroControls.set(index, copy);
	}

	thisAsSynth->sendChangeMessage();
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
	SimpleReadWriteLock::ScopedReadLock sl(macroLock);

	MacroControlData *data = macroControls[macroIndex];

	const int numParams = data->getNumParameters();

	if(data != nullptr)
	{
		for(int i = 0; i < numParams; i++)
		{
			data->removeParameter(0);
		}
	}

	jassert(data->getNumParameters() == 0);

	data->setMacroName("Macro " + String(macroIndex + 1));

	data->setValue(0.0);

	data->setMidiController(-1);

	thisAsSynth->sendChangeMessage();
}

void MacroControlBroadcaster::MacroControlData::setValue(float newValue)
{
	currentValue = newValue;

	for(int i = 0; i < controlledParameters.size(); i++)
	{
		MacroControlledParameterData *pData = controlledParameters[i];
			
		pData->setAttribute(newValue / 127.0f);
	}

};

bool MacroControlBroadcaster::MacroControlData::isDanglingProcessor(int parameterIndex)
{
	jassert( controlledParameters[parameterIndex]->getProcessor() != nullptr);

	return controlledParameters[parameterIndex]->getProcessor() == nullptr;
}

void MacroControlBroadcaster::MacroControlData::clearDanglingProcessors()
{
	for(int i = 0; i < controlledParameters.size(); i++)
	{
		if(isDanglingProcessor(i))
		{
			controlledParameters.remove(i);
		}
	}
}

void MacroControlBroadcaster::MacroControlData::removeAllParametersWithProcessor(Processor *p)
{
	for(int i = 0; i < controlledParameters.size(); i++)
	{
		auto cp = controlledParameters[i];

		if (cp->getProcessor() == p)
		{
			parent.sendMacroConnectionChangeMessage(macroIndex, cp->getProcessor(), cp->getParameter(), false);
			controlledParameters.remove(i--);
		}
	}		
}

XmlElement *MacroControlBroadcaster::MacroControlData::exportAsXml()
{
	XmlElement *macro = new XmlElement("macro");

	macro->setAttribute("name", macroName);

	macro->setAttribute("value", currentValue);
	
	macro->setAttribute("midi_cc", midiController);

	for(int i = 0; i < controlledParameters.size(); i++)
	{
		if( !isDanglingProcessor(i)) macro->addChildElement(controlledParameters[i]->exportAsXml());
	}
	return macro;
};

bool MacroControlBroadcaster::MacroControlData::hasParameter(Processor *p, int parameterIndex)
{
	for(int i = 0; i < controlledParameters.size(); i++)
	{
		if(controlledParameters[i]->getProcessor() == p &&
			controlledParameters[i]->getParameter() == parameterIndex)
		{
			return true;
		}
	}

	return false;
}


void MacroControlBroadcaster::MacroControlData::addParameter(Processor *p, int parameterId, const String &parameterName, NormalisableRange<double> range, bool readOnly, bool isUsingCustomData)
{
	controlledParameters.add(new MacroControlledParameterData(p,
																parameterId,
																parameterName,
																range,
																readOnly));

	controlledParameters.getLast()->setIsCustomAutomation(isUsingCustomData);
		
	parent.sendMacroConnectionChangeMessage(macroIndex, p, parameterId, true);
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
		thisAsSynth->sendChangeMessage();

	}
	else if (notifyEditor == sendNotification)
	{
		thisAsSynth->sendChangeMessage();

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
	SimpleReadWriteLock::ScopedReadLock sl(macroLock);

	for (int i = 0; i < macroControls.size(); i++)
	{
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
	SimpleReadWriteLock::ScopedReadLock sl(macroLock);

	for (int i = 0; i < macroControls.size(); i++)
	{
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
							const String &parameterName,
							NormalisableRange<double> range,
							bool readOnly)
{
	Processor *p = findProcessor(thisAsSynth, processorId);

	if(p != nullptr)
	{
		//if(macroControls[macroControllerIndex]->hasParameter(p, parameterId)) return;

		SimpleReadWriteLock::ScopedReadLock sl(macroLock);

		for(int i = 0; i < macroControls.size(); i++)
		{
			if(macroControls[i]->hasParameter(p, parameterId))
			{
				macroControls[i]->removeParameter(parameterName);
			}
		}


		macroControls[macroControllerIndex]->addParameter(p, parameterId, parameterName, range, readOnly);

		p->sendChangeMessage();
		thisAsSynth->sendChangeMessage();
	}
}

void MacroControlBroadcaster::MacroControlData::removeParameter(int parameterIndex)
{
    
	if(parameterIndex != -1 && parameterIndex < controlledParameters.size() && controlledParameters[parameterIndex]->getProcessor() != nullptr)
	{
		controlledParameters[parameterIndex]->getProcessor()->sendChangeMessage();
	}

    WeakReference<Processor> pToRemove;
	auto indexToRemove = -1;

	if (auto cp = controlledParameters[parameterIndex])
	{
		pToRemove = cp->getProcessor();
		indexToRemove = cp->getParameter();
	}

	controlledParameters.remove(parameterIndex);
    
    WeakReference<MacroControlBroadcaster> safeParent(&parent);
    
    auto mi = macroIndex;
    MessageManager::callAsync([safeParent, pToRemove, mi, indexToRemove]()
    {
        if(safeParent && pToRemove)
        {
            safeParent->sendMacroConnectionChangeMessage(mi, pToRemove, indexToRemove, false);
        }
    });
};

void MacroControlBroadcaster::MacroControlData::removeParameter(const String &parameterName, const Processor *processor)
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

	removeParameter(index);
};

} // namespace hise
