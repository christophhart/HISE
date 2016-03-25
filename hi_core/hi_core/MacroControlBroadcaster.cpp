/*
  ==============================================================================

    MacroControlBroadcaster.cpp
    Created: 25 Sep 2014 1:58:07pm
    Author:  Christoph

  ==============================================================================
*/



MacroControlBroadcaster::MacroControlBroadcaster(ModulatorSynthChain *chain):
	thisAsSynth(chain)
{
	for(int i = 0; i < 8; i++)
	{
		macroControls.add(new MacroControlData(i));
		
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
	parameter(xml.getIntAttribute("parameter", parameter)),
	parameterName(xml.getStringAttribute("parameter_name", "")),
	id (xml.getStringAttribute("id", id)),
	readOnly(xml.getBoolAttribute("readonly", true))
{
	parameterRange = NormalisableRange<double>(xml.getDoubleAttribute("low", 0.0), xml.getDoubleAttribute("high", 1.0));
	inverted = xml.getBoolAttribute("inverted", false);
	controlledProcessor = findProcessor(chain, id);
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
		controlledProcessor.get()->setAttribute(parameter, value, readOnly ? sendNotification : dontSendNotification);
	}
	
};

		

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



MacroControlBroadcaster::MacroControlData::MacroControlData(ModulatorSynthChain *chain, XmlElement *xml)
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

void MacroControlBroadcaster::loadMacrosFromValueTree(const ValueTree &v)
{
#if USE_OLD_FILE_FORMAT

	ScopedPointer<XmlElement> data = XmlDocument::parse(v.getProperty("MacroControls", String::empty));
#else
	ScopedPointer<XmlElement> data = v.getChildWithName("macro_controls").createXml();
#endif


	if(data != nullptr && data->getNumChildElements() == 8)
	{
		macroControls.clear();

		for(int i = 0; i < data->getNumChildElements(); i++)
		{
			macroControls.add(new MacroControlData(thisAsSynth, data->getChildElement(i)));

			thisAsSynth->getMainController()->getMacroManager().setMidiControllerForMacro(i, macroControls.getLast()->getMidiController());
		}

		for(int i = 0; i < macroControls.size(); i++)
		{
			setMacroControl(i, macroControls[i]->getCurrentValue(), sendNotification);
		}
			
	}
}

void MacroControlBroadcaster::replaceMacroControlData(int index, MacroControlData *newData, ModulatorSynthChain *parentChain)
{
	if(index < macroControls.size())
	{
		ScopedPointer<XmlElement> xml = newData->exportAsXml();

		MacroControlData *copy = new MacroControlData(parentChain, xml);

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
		if(controlledParameters[i]->getProcessor() == p) controlledParameters.remove(i);
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


void MacroControlBroadcaster::MacroControlData::addParameter(Processor *p, int parameterId, const String &parameterName, NormalisableRange<double> range, bool readOnly)
{
	controlledParameters.add(new MacroControlledParameterData(p,
																parameterId,
																parameterName,
																range,
																readOnly));


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

		p->setParameterNotifyingHost(macroIndex, newValue / 127.0f);
	}
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
	if(controlledParameters[parameterIndex]->getProcessor() != nullptr)
	{
		controlledParameters[parameterIndex]->getProcessor()->sendChangeMessage();
	}

	controlledParameters.remove(parameterIndex);

};

void MacroControlBroadcaster::MacroControlData::removeParameter(const String &parameterName, const Processor *processor)
{ 
	int index = -1;

	for(int i = 0; i < controlledParameters.size(); i++)
	{
		const bool isProcessor = processor == nullptr || controlledParameters[i]->getProcessor() == processor;

		if(controlledParameters[i]->getParameterName() == parameterName && isProcessor)
		{
			index = i;
			break;
		}
	};

	if(index == -1) return;

	removeParameter(index);
};
