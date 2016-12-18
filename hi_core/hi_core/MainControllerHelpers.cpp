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

MidiControllerAutomationHandler::MidiControllerAutomationHandler(MainController *mc_) :
anyUsed(false),
mc(mc_)
{
	tempBuffer.ensureSize(2048);

	clear();
}

void MidiControllerAutomationHandler::addMidiControlledParameter(Processor *interfaceProcessor, int attributeIndex, NormalisableRange<double> parameterRange, int macroIndex)
{
	ScopedLock sl(mc->getLock());

	unlearnedData.processor = interfaceProcessor;
	unlearnedData.attribute = attributeIndex;
	unlearnedData.parameterRange = parameterRange;
	unlearnedData.macroIndex = macroIndex;
	unlearnedData.used = true;
}

bool MidiControllerAutomationHandler::isLearningActive() const
{
	return unlearnedData.used;
}

bool MidiControllerAutomationHandler::isLearningActive(Processor *interfaceProcessor, int attributeIndex) const
{
	return unlearnedData.processor == interfaceProcessor && unlearnedData.attribute == attributeIndex;
}

void MidiControllerAutomationHandler::deactivateMidiLearning()
{
	ScopedLock sl(mc->getLock());

	unlearnedData = AutomationData();
}

void MidiControllerAutomationHandler::setUnlearndedMidiControlNumber(int ccNumber)
{
	jassert(isLearningActive());

	ScopedLock sl(mc->getLock());

	automationData[ccNumber] = unlearnedData;
	unlearnedData = AutomationData();

	anyUsed = true;
}

int MidiControllerAutomationHandler::getMidiControllerNumber(Processor *interfaceProcessor, int attributeIndex) const
{
	for (int i = 0; i < 128; i++)
	{
		const AutomationData *a = automationData + i;

		if (a->processor == interfaceProcessor && a->attribute == attributeIndex)
		{
			return i;
		}
	}

	return -1;
}

void MidiControllerAutomationHandler::refreshAnyUsedState()
{
	ScopedLock sl(mc->getLock());

	anyUsed = false;

	for (int i = 0; i < 128; i++)
	{
		AutomationData *a = automationData + i;

		if (a->used)
		{
			anyUsed = true;
			break;
		}
	}
}

void MidiControllerAutomationHandler::clear()
{
	for (int i = 0; i < 128; i++)
	{
		automationData[i] = AutomationData();
	};

	unlearnedData = AutomationData();

	anyUsed = false;
}

void MidiControllerAutomationHandler::removeMidiControlledParameter(Processor *interfaceProcessor, int attributeIndex)
{
	ScopedLock sl(mc->getLock());

	for (int i = 0; i < 128; i++)
	{
		AutomationData *a = automationData + i;

		if (a->processor == interfaceProcessor && a->attribute == attributeIndex)
		{
			*a = AutomationData();
			break;
		}
	}

	refreshAnyUsedState();
}

MidiControllerAutomationHandler::AutomationData::AutomationData() :
processor(nullptr),
attribute(-1),
parameterRange(NormalisableRange<double>()),
macroIndex(-1),
used(false)
{

}



ValueTree MidiControllerAutomationHandler::exportAsValueTree() const
{
	ValueTree v("MidiAutomation");

	for (int i = 0; i < 128; i++)
	{
		const AutomationData *a = automationData + i;
		if (a->used && a->processor != nullptr)
		{
			ValueTree cc("Controller");

			cc.setProperty("Controller", i, nullptr);
			cc.setProperty("Processor", a->processor->getId(), nullptr);
			cc.setProperty("MacroIndex", a->macroIndex, nullptr);
			cc.setProperty("Start", a->parameterRange.start, nullptr);
			cc.setProperty("End", a->parameterRange.end, nullptr);
			cc.setProperty("Skew", a->parameterRange.skew, nullptr);
			cc.setProperty("Interval", a->parameterRange.interval, nullptr);
			cc.setProperty("Attribute", a->attribute, nullptr);

			v.addChild(cc, -1, nullptr);
		}
	}

	return v;
}

void MidiControllerAutomationHandler::restoreFromValueTree(const ValueTree &v)
{
	if (v.getType() != Identifier("MidiAutomation")) return;

	clear();

	for (int i = 0; i < v.getNumChildren(); i++)
	{
		ValueTree cc = v.getChild(i);

		int controller = cc.getProperty("Controller", i);

		AutomationData *a = automationData + controller;

		a->processor = ProcessorHelpers::getFirstProcessorWithName(mc->getMainSynthChain(), cc.getProperty("Processor"));
		a->macroIndex = cc.getProperty("MacroIndex");
		a->attribute = cc.getProperty("Attribute", a->attribute);

		double start = cc.getProperty("Start");
		double end = cc.getProperty("End");
		double skew = cc.getProperty("Skew", a->parameterRange.skew);
		double interval = cc.getProperty("Interval", a->parameterRange.interval);

		a->parameterRange = NormalisableRange<double>(start, end, interval, skew);

		a->used = true;
	}

	refreshAnyUsedState();
}

void MidiControllerAutomationHandler::handleParameterData(MidiBuffer &b)
{
	const bool bufferEmpty = b.isEmpty();
	const bool noCCsUsed = !anyUsed && !unlearnedData.used;

	if (bufferEmpty || noCCsUsed) return;

	tempBuffer.clear();

	MidiBuffer::Iterator mb(b);
	MidiMessage m;

	int samplePos;

	while (mb.getNextEvent(m, samplePos))
	{
		bool consumed = false;

		if (m.isController())
		{
			const int number = m.getControllerNumber();

			if (isLearningActive())
			{
				setUnlearndedMidiControlNumber(number);
			}

			AutomationData *a = automationData + number;

			if (a->used)
			{
				jassert(a->processor.get() != nullptr);

				const float value = (float)a->parameterRange.convertFrom0to1((double)m.getControllerValue() / 127.0);

				if (a->macroIndex != -1)
				{
					a->processor->getMainController()->getMacroManager().getMacroChain()->setMacroControl(a->macroIndex, (float)m.getControllerValue(), sendNotification);
				}
				else
				{
					a->processor->setAttribute(a->attribute, value, sendNotification);
				}

				consumed = true;
			}
		}

		if (!consumed) tempBuffer.addEvent(m, samplePos);
	}

	b.clear();
	b.addEvents(tempBuffer, 0, -1, 0);
}


void ConsoleLogger::logMessage(const String &message)
{
	if (message.startsWith("!"))
	{
		debugError(processor, message.substring(1));
	}
	else
	{
		debugToConsole(processor, message);
	}

	
}

ControlledObject::ControlledObject(MainController *m) :
	controller(m) {
	jassert(m != nullptr);
};

ControlledObject::~ControlledObject()
{
	// Oops, this ControlledObject was not connected to a MainController
	jassert(controller != nullptr);

	masterReference.clear();
};
