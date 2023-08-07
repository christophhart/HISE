
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


MidiControllerAutomationHandler::MidiControllerAutomationHandler(MainController *mc_) :
anyUsed(false),
mpeData(mc_),
mc(mc_),
ccName("MIDI CC")
{
	tempBuffer.ensureSize(2048);

	clear(sendNotification);
}

void MidiControllerAutomationHandler::addMidiControlledParameter(Processor *interfaceProcessor, int attributeIndex, NormalisableRange<double> parameterRange, int macroIndex)
{
	ScopedLock sl(mc->getLock());

	unlearnedData.processor = interfaceProcessor;
	unlearnedData.attribute = attributeIndex;
	unlearnedData.parameterRange = parameterRange;
	unlearnedData.fullRange = parameterRange;
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

void MidiControllerAutomationHandler::setUnlearndedMidiControlNumber(int ccNumber, NotificationType notifyListeners)
{
	jassert(isLearningActive());

	if (!shouldAddControllerToPopup(ccNumber))
	{
		return;
	}

	ScopedLock sl(mc->getLock());

	unlearnedData.ccNumber = ccNumber;

	if (exclusiveMode)
	{
		automationData[ccNumber].clearQuick();
		automationData[ccNumber].add(unlearnedData);
	}
	else
		automationData[ccNumber].addIfNotAlreadyThere(unlearnedData);

	unlearnedData = AutomationData();

	anyUsed = true;

	if (notifyListeners)
		sendChangeMessage();
}

int MidiControllerAutomationHandler::getMidiControllerNumber(Processor *interfaceProcessor, int attributeIndex) const
{
	for (int i = 0; i < 128; i++)
	{
		for (auto& a : automationData[i])
		{
			if (a.processor == interfaceProcessor && a.attribute == attributeIndex)
			{
				return i;
			}
		}
	}

	return -1;
}

void MidiControllerAutomationHandler::refreshAnyUsedState()
{
	AudioThreadGuard::Suspender suspender;
	LockHelpers::SafeLock sl(mc, LockHelpers::AudioLock);

	ignoreUnused(suspender);

	anyUsed = false;

	for (int i = 0; i < 128; i++)
	{
		for (auto& a : automationData[i])
		{
			if (a.used)
			{
				anyUsed = true;
				return;
			}
		}
	}
}

void MidiControllerAutomationHandler::clear(NotificationType notifyListeners)
{
	for (int i = 0; i < 128; i++)
	{
		automationData[i].clearQuick();
	};

	unlearnedData = AutomationData();

	anyUsed = false;
	
	if (notifyListeners == sendNotification)
		sendChangeMessage();
}

void MidiControllerAutomationHandler::removeMidiControlledParameter(Processor *interfaceProcessor, int attributeIndex, NotificationType notifyListeners)
{
	{
		AudioThreadGuard audioGuard(&(mc->getKillStateHandler()));
		LockHelpers::SafeLock sl(mc, LockHelpers::AudioLock);

		for (int i = 0; i < 128; i++)
		{
			for (int j = 0; j < automationData[i].size(); j++)
			{
				auto a = automationData[i][j];

				if (a.processor == interfaceProcessor && a.attribute == attributeIndex)
				{
					automationData[i].remove(j--);
				}
			}
		}
	}

	refreshAnyUsedState();

	if (notifyListeners == sendNotification)
		sendChangeMessage();
}

MidiControllerAutomationHandler::AutomationData::AutomationData() :
processor(nullptr),
attribute(-1),
parameterRange(NormalisableRange<double>()),
fullRange(NormalisableRange<double>()),
macroIndex(-1),
used(false),
inverted(false)
{

}



void MidiControllerAutomationHandler::AutomationData::clear()
{
	processor = nullptr;
	attribute = -1;
	parameterRange = NormalisableRange<double>();
	fullRange = NormalisableRange<double>();
	macroIndex = -1;
	ccNumber = -1;
	inverted = false;
	used = false;
}



bool MidiControllerAutomationHandler::AutomationData::operator==(const AutomationData& other) const
{
	return other.processor == processor && other.attribute == attribute;
}

void MidiControllerAutomationHandler::AutomationData::restoreFromValueTree(const ValueTree &v)
{
	ccNumber = v.getProperty("Controller", 1);;
	processor = ProcessorHelpers::getFirstProcessorWithName(mc->getMainSynthChain(), v.getProperty("Processor"));
	macroIndex = v.getProperty("MacroIndex");

	auto attributeString = v.getProperty("Attribute", attribute).toString();

	const bool isParameterId = attributeString.containsAnyOf("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");

	// The parameter was stored correctly as ID
	if (isParameterId && processor.get() != nullptr)
	{
		auto numCustomAutomationSlots = processor->getMainController()->getUserPresetHandler().getNumCustomAutomationData();

		if (numCustomAutomationSlots != 0)
		{
			for (int j = 0; j < numCustomAutomationSlots; j++)
			{
				if (auto ah = processor->getMainController()->getUserPresetHandler().getCustomAutomationData(j))
				{
					if (ah->id == attributeString)
					{
						attribute = j;
						break;
					}
				}
			}
		}
		else
		{
			const Identifier pId(attributeString);

			for (int j = 0; j < processor->getNumParameters(); j++)
			{
				if (processor->getIdentifierForParameterIndex(j) == pId)
				{
					attribute = j;
					break;
				}
			}
		}
	}
	else
	{
		// This tries to obtain the correct id.
		auto presetVersion = v.getRoot().getProperty("Version").toString();

		const Identifier pId = UserPresetHelpers::getAutomationIndexFromOldVersion(presetVersion, attributeString.getIntValue());

		if (pId.isNull())
		{
			attribute = attributeString.getIntValue();
		}
		else
		{
			for (int j = 0; j < processor->getNumParameters(); j++)
			{
				if (processor->getIdentifierForParameterIndex(j) == pId)
				{
					attribute = j;
					break;
				}
			}
		}
	}

	double start = v.getProperty("Start");
	double end = v.getProperty("End");
	double skew = v.getProperty("Skew", parameterRange.skew);
	double interval = v.getProperty("Interval", parameterRange.interval);

	auto fullStart = v.getProperty("FullStart", start);
	auto fullEnd = v.getProperty("FullEnd", end);

	parameterRange = NormalisableRange<double>(start, end, interval, skew);
	fullRange = NormalisableRange<double>(fullStart, fullEnd, interval, skew);

	used = true;
	inverted = v.getProperty("Inverted", false);
}

juce::ValueTree MidiControllerAutomationHandler::AutomationData::exportAsValueTree() const
{
	ValueTree cc("Controller");

	cc.setProperty("Controller", ccNumber, nullptr);
	cc.setProperty("Processor", processor->getId(), nullptr);
	cc.setProperty("MacroIndex", macroIndex, nullptr);
	cc.setProperty("Start", parameterRange.start, nullptr);
	cc.setProperty("End", parameterRange.end, nullptr);
	cc.setProperty("FullStart", fullRange.start, nullptr);
	cc.setProperty("FullEnd", fullRange.end, nullptr);
	cc.setProperty("Skew", parameterRange.skew, nullptr);
	cc.setProperty("Interval", parameterRange.interval, nullptr);

	if (auto ap = processor->getMainController()->getUserPresetHandler().getCustomAutomationData(attribute))
	{
		cc.setProperty("Attribute", ap->id, nullptr);
	}
	else
	{
		cc.setProperty("Attribute", processor->getIdentifierForParameterIndex(attribute).toString(), nullptr);
	}

	cc.setProperty("Inverted", inverted, nullptr);

	return cc;
}


struct MidiControllerAutomationHandler::MPEData::Data: public Processor::DeleteListener
{
	Data(MPEData& parent_) :
		Processor::DeleteListener(),
		parent(parent_)
	{};

	void add(MPEModulator* m)
	{
		m->addDeleteListener(this);
		connections.addIfNotAlreadyThere(m);
	}

	void remove(MPEModulator* m)
	{
		m->removeDeleteListener(this);
		connections.removeAllInstancesOf(m);
	}

	void processorDeleted(Processor* deletedProcessor) override
	{
		if (auto m = dynamic_cast<MPEModulator*>(deletedProcessor))
		{
			connections.removeAllInstancesOf(m);

            parent.sendAsyncNotificationMessage(m, EventType::MPEModConnectionRemoved);
		}
		else
			jassertfalse;
	}

	void clear()
	{
		for (auto c : connections)
		{
			if (c)
			{
				c->removeDeleteListener(this);
				c->setBypassed(true);
				c->sendChangeMessage();
			}
			else
				jassertfalse;
		}

		connections.clear();
	}

	void updateChildEditorList(bool /*forceUpdate*/) override {};

	MPEData& parent;
	Array<WeakReference<MPEModulator>> connections;
};

MidiControllerAutomationHandler::MPEData::MPEData(MainController* mc) :
	ControlledObject(mc),
	data(new Data(*this)),
	asyncRestorer(*this)
{

}

MidiControllerAutomationHandler::MPEData::~MPEData()
{
	jassert(listeners.size() == 0);
	data = nullptr;
}

void MidiControllerAutomationHandler::MPEData::AsyncRestorer::timerCallback()
{

}

void MidiControllerAutomationHandler::MPEData::restoreFromValueTree(const ValueTree &v)
{
	pendingData = v;

	auto f = [this](Processor* p)
	{
		LockHelpers::noMessageThreadBeyondInitialisation(p->getMainController());

		clear();

		static const Identifier id("ID");

		setMpeMode(pendingData.getProperty("Enabled", false));

		for (auto d : pendingData)
		{
			jassert(d.hasType("Processor"));

			d.setProperty("Type", "MPEModulator", nullptr);
			d.setProperty("Intensity", 1.0f, nullptr);

			ValueTree dummyChild("ChildProcessors");

			d.addChild(dummyChild, -1, nullptr);
			String id_ = d.getProperty(id).toString();

			if (auto mod = findMPEModulator(id_))
			{
				mod->restoreFromValueTree(d);
				addConnection(mod, dontSendNotification);
			}
		}

		

        sendAsyncNotificationMessage(nullptr, EventType::MPEDataReloaded);
        
        return SafeFunctionCall::OK;
	};

	getMainController()->getKillStateHandler().killVoicesAndCall(getMainController()->getMainSynthChain(), f, MainController::KillStateHandler::SampleLoadingThread);

	asyncRestorer.restore(v);
}

juce::ValueTree MidiControllerAutomationHandler::MPEData::exportAsValueTree() const
{
	ValueTree connectionData("MPEData");

	connectionData.setProperty("Enabled", mpeEnabled, nullptr);

	static const Identifier t("Type");
	static const Identifier i_("Intensity");
	

	for (auto mod : data->connections)
	{
		if (mod.get() != nullptr)
		{
			auto child = mod->exportAsValueTree();
			child.removeChild(0, nullptr);
			child.removeChild(0, nullptr);
			jassert(child.getNumChildren() == 0);
			child.removeProperty(t, nullptr);
			child.removeProperty(i_, nullptr);
			
			
			connectionData.addChild(child, -1, nullptr);
		}
	}

	return connectionData;
}

void MidiControllerAutomationHandler::MPEData::sendAsyncNotificationMessage(MPEModulator* mod, EventType type)
{
    WeakReference<MPEModulator> ref(mod);
    
    auto f = [ref, type](Dispatchable* obj)
    {
        if(ref.get() == nullptr && (type == EventType::MPEModConnectionAdded || type == MPEModConnectionRemoved))
            return Dispatchable::Status::OK;
        
        auto d = static_cast<MPEData*>(obj);
        
        jassert_message_thread;
        
        ScopedLock sl(d->listeners.getLock());
        
        for (auto l : d->listeners)
        {
            if (l == ref.get())
                continue;
            
            if (l)
            {
                switch(type)
                {
                    case EventType::MPEModConnectionAdded:   l->mpeModulatorAssigned(ref, true); break;
                    case EventType::MPEModConnectionRemoved: l->mpeModulatorAssigned(ref, false); break;
                    case EventType::MPEModeChanged:          l->mpeModeChanged(d->mpeEnabled); break;
                    case EventType::MPEDataReloaded:         l->mpeDataReloaded(); break;
                    default:                                 jassertfalse; break;
                }
            }
        }
        
        return Dispatchable::Status::OK;
    };
    
    getMainController()->getLockFreeDispatcher().callOnMessageThreadAfterSuspension(this, f);

}
    
void MidiControllerAutomationHandler::MPEData::addConnection(MPEModulator* mod, NotificationType notifyListeners/*=sendNotification*/)
{
    jassert(mod->isOnAir());
    
    jassert(LockHelpers::noMessageThreadBeyondInitialisation(mod->getMainController()));
    
	if (!data->connections.contains(mod))
	{
		data->add(mod);

        mod->mpeModulatorAssigned(mod, true);
        
		if (notifyListeners == sendNotification)
            sendAsyncNotificationMessage(mod, EventType::MPEModConnectionAdded);
	}
}

void MidiControllerAutomationHandler::MPEData::removeConnection(MPEModulator* mod, NotificationType notifyListeners/*=sendNotification*/)
{
    if(mod->isOnAir())
    {
        jassert(LockHelpers::noMessageThreadBeyondInitialisation(mod->getMainController()));
    }
    
	if (data->connections.contains(mod))
	{
		data->remove(mod);

        if(mod->isOnAir())
            mod->mpeModulatorAssigned(mod, false);

		if (notifyListeners == sendNotification)
            sendAsyncNotificationMessage(mod, EventType::MPEModConnectionRemoved);
	}
	else if (mod != nullptr)
	{
		sendAmountChangeMessage();
	}
}

MPEModulator* MidiControllerAutomationHandler::MPEData::getModulator(int index) const
{
	return data->connections[index].get();
}

MPEModulator* MidiControllerAutomationHandler::MPEData::findMPEModulator(const String& modName) const
{
	return dynamic_cast<MPEModulator*>(ProcessorHelpers::getFirstProcessorWithName(getMainController()->getMainSynthChain(), modName));
}

juce::StringArray MidiControllerAutomationHandler::MPEData::getListOfUnconnectedModulators(bool prettyName) const
{
	Processor::Iterator<MPEModulator> iter(getMainController()->getMainSynthChain(), false);

	StringArray sa;

	while (auto m = iter.getNextProcessor())
	{
		if (!data->connections.contains(m))
			sa.add(m->getId());
	}

	if (prettyName)
	{
		for (auto& s : sa)
		{
			s = getPrettyName(s);
		}
	}

	return sa;
}

juce::String MidiControllerAutomationHandler::MPEData::getPrettyName(const String& id)
{
	auto n = id.replace("MPE", "");
	String pretty;
	auto ptr = n.getCharPointer();
	bool lastWasUppercase = true;

	while (!ptr.isEmpty())
	{
		if (ptr.isUpperCase() && !lastWasUppercase)
			pretty << " ";

		lastWasUppercase = ptr.isUpperCase();
		pretty << ptr.getAddress()[0];
		ptr++;
	}

	return pretty;
}

void MidiControllerAutomationHandler::MPEData::clear()
{
	data->clear();

	Processor::Iterator<MPEModulator> iter(getMainController()->getMainSynthChain());

	while (auto m = iter.getNextProcessor())
	{
		m->resetToDefault();
	}
	
}

void MidiControllerAutomationHandler::MPEData::reset()
{
	clear();
	mpeEnabled = false;

    
    sendAsyncNotificationMessage(nullptr, EventType::MPEModeChanged);
}

int MidiControllerAutomationHandler::MPEData::size() const
{
	return data->connections.size();
}

void MidiControllerAutomationHandler::MPEData::setMpeMode(bool shouldBeOn)
{
	getMainController()->getKeyboardState().injectMessage(MidiMessage::controllerEvent(1, 74, 64));
	getMainController()->getKeyboardState().injectMessage(MidiMessage::pitchWheel(1, 8192));
	getMainController()->allNotesOff();

	if (mpeEnabled != shouldBeOn)
	{
		mpeEnabled = shouldBeOn;

		// Do this synchronously
		ScopedLock sl(listeners.getLock());

		for (auto l : listeners)
		{
			if (l != nullptr)
				l->mpeModeChanged(mpeEnabled);
		}
	}
}

bool MidiControllerAutomationHandler::MPEData::contains(MPEModulator* mod) const
{
	return data->connections.contains(mod);
}



ValueTree MidiControllerAutomationHandler::exportAsValueTree() const
{
	if (unloadedData.isValid())
		return unloadedData;

	ValueTree v("MidiAutomation");

	for (int i = 0; i < 128; i++)
	{
		for (auto& a : automationData[i])
		{
			if (a.used && a.processor != nullptr)
			{
				auto cc = a.exportAsValueTree();
				v.addChild(cc, -1, nullptr);
			}
		}
	}

	return v;
}

void MidiControllerAutomationHandler::restoreFromValueTree(const ValueTree &v)
{
	if (v.getType() != Identifier("MidiAutomation")) return;

	clear(sendNotification);

	for (int i = 0; i < v.getNumChildren(); i++)
	{
		ValueTree cc = v.getChild(i);

		int controller = cc.getProperty("Controller", 1);

		auto& aArray = automationData[controller];

		AutomationData a;
		a.mc = mc;

		a.restoreFromValueTree(cc);

		aArray.addIfNotAlreadyThere(a);
	}

    if(mc->getUserPresetHandler().isInternalPresetLoad())
        sendSynchronousChangeMessage();
    else
        sendChangeMessage();

	refreshAnyUsedState();
}

Identifier MidiControllerAutomationHandler::getUserPresetStateId() const
{ return UserPresetIds::MidiAutomation; }

void MidiControllerAutomationHandler::resetUserPresetState()
{ clear(sendNotification); }

MidiControllerAutomationHandler::MPEData::Listener::~Listener()
{}

void MidiControllerAutomationHandler::MPEData::Listener::mpeModulatorAmountChanged()
{}

Identifier MidiControllerAutomationHandler::MPEData::getUserPresetStateId() const
{ return UserPresetIds::MPEData; }

void MidiControllerAutomationHandler::MPEData::resetUserPresetState()
{ reset(); }

bool MidiControllerAutomationHandler::MPEData::isMpeEnabled() const
{ return mpeEnabled; }

void MidiControllerAutomationHandler::MPEData::addListener(Listener* l)
{
	listeners.addIfNotAlreadyThere(l);

	// Fire this once to setup the correct state
	l->mpeModeChanged(mpeEnabled);
}

void MidiControllerAutomationHandler::MPEData::removeListener(Listener* l)
{
	listeners.removeAllInstancesOf(l);
}

void MidiControllerAutomationHandler::MPEData::sendAmountChangeMessage()
{
	ScopedLock sl(listeners.getLock());

	for (auto l : listeners)
	{
		if (l)
			l->mpeModulatorAmountChanged();
	}
}

MidiControllerAutomationHandler::MPEData::AsyncRestorer::AsyncRestorer(MPEData& parent_):
	parent(parent_)
{}

void MidiControllerAutomationHandler::MPEData::AsyncRestorer::restore(const ValueTree& v)
{
	data = v;
	dirty = true;
	startTimer(50);
}

MidiControllerAutomationHandler::AutomationData::~AutomationData()
{ clear(); }

MidiControllerAutomationHandler::MPEData& MidiControllerAutomationHandler::getMPEData()
{ return mpeData; }

const MidiControllerAutomationHandler::MPEData& MidiControllerAutomationHandler::getMPEData() const
{ return mpeData; }

void MidiControllerAutomationHandler::setUnloadedData(const ValueTree& v)
{
	unloadedData = v;
}

void MidiControllerAutomationHandler::loadUnloadedData()
{
	if(unloadedData.isValid())
		restoreFromValueTree(unloadedData);

	unloadedData = {};
}

void MidiControllerAutomationHandler::setControllerPopupNumbers(BigInteger controllerNumberToShow)
{
	controllerNumbersInPopup = controllerNumberToShow;
}

bool MidiControllerAutomationHandler::hasSelectedControllerPopupNumbers() const
{
	return !controllerNumbersInPopup.isZero();
}

bool MidiControllerAutomationHandler::shouldAddControllerToPopup(int controllerValue) const
{
	if (!hasSelectedControllerPopupNumbers())
		return true;

	return controllerNumbersInPopup[controllerValue];
}

bool MidiControllerAutomationHandler::isMappable(int controllerValue) const
{
	if (isPositiveAndBelow(controllerValue, 128))
	{
		if (!exclusiveMode)
			return shouldAddControllerToPopup(controllerValue);
		else
			return shouldAddControllerToPopup(controllerValue) && automationData[controllerValue].isEmpty();
	}
		
	return false;
}

void MidiControllerAutomationHandler::setExclusiveMode(bool shouldBeExclusive)
{
	exclusiveMode = shouldBeExclusive;
}

void MidiControllerAutomationHandler::setConsumeAutomatedControllers(bool shouldConsume)
{
	consumeEvents = shouldConsume;
}

void MidiControllerAutomationHandler::setControllerPopupNames(const StringArray& newControllerNames)
{
	controllerNames = newControllerNames;
}

String MidiControllerAutomationHandler::getControllerName(int controllerIndex)
{
	if (isPositiveAndBelow(controllerIndex, controllerNames.size()))
	{
		return controllerNames[controllerIndex];
	}
	else
	{
		String s;
		s << "CC#" << controllerIndex;
		return s;
	}
}

void MidiControllerAutomationHandler::setCCName(const String& newCCName)
{ ccName = newCCName; }

String MidiControllerAutomationHandler::getCCName() const
{ return ccName; }

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
				setUnlearndedMidiControlNumber(number, sendNotification);
			}

			HiseEvent he(m);

			consumed = handleControllerMessage(he);
		}

		if (!consumed) tempBuffer.addEvent(m, samplePos);
	}

	b.clear();
	b.addEvents(tempBuffer, 0, -1, 0);
}


bool MidiControllerAutomationHandler::handleControllerMessage(const HiseEvent& e)
{
	auto number = e.getControllerNumber();

    bool thisConsumed = false;
    
	for (auto& a : automationData[number])
	{
		if (a.used && a.processor.get() != nullptr)
		{
			jassert(a.processor.get() != nullptr);

			// MIDI events should not be propagated as plugin parameter changes
			ScopedValueSetter<bool> setter(a.processor->getMainController()->getPluginParameterUpdateState(), false);
			
			auto normalizedValue = (double)e.getControllerValue() / 127.0;

			if (a.inverted) normalizedValue = 1.0 - normalizedValue;

			const double value = a.parameterRange.convertFrom0to1(normalizedValue);

			const float snappedValue = (float)a.parameterRange.snapToLegalValue(value);

			if (a.macroIndex != -1)
			{
				a.processor->getMainController()->getMacroManager().getMacroChain()->setMacroControl(a.macroIndex, (float)e.getControllerValue(), sendNotification);
			}
			else
			{
				if (a.lastValue != snappedValue)
				{
					auto& uph = a.processor->getMainController()->getUserPresetHandler();

					if (uph.isUsingCustomDataModel())
					{
						if (auto ad = uph.getCustomAutomationData(a.attribute))
							ad->call(snappedValue);
					}
					else
					{
						a.processor->setAttribute(a.attribute, snappedValue, sendNotification);
					}

					a.lastValue = snappedValue;
				}
			}

            thisConsumed |= consumeEvents;
		}
	}

	return thisConsumed;
}

hise::MidiControllerAutomationHandler::AutomationData MidiControllerAutomationHandler::getDataFromIndex(int index) const
{
	int currentIndex = 0;

	for (int i = 0; i < 128; i++)
	{
		for (const auto& a: automationData[i])
		{
			if (index == currentIndex)
				return AutomationData(a);

			currentIndex++;
		}
	}

	return AutomationData();
}

int MidiControllerAutomationHandler::getNumActiveConnections() const
{
	int numActive = 0;

	for (int i = 0; i < 128; i++)
	{
		numActive += automationData[i].size();
	}

	return numActive;
}

bool MidiControllerAutomationHandler::setNewRangeForParameter(int index, NormalisableRange<double> range)
{
	int currentIndex = 0;

	for (int i = 0; i < 128; i++)
	{
		for (auto& a : automationData[i])
		{
			if (index == currentIndex)
			{
				a.parameterRange = range;
				return true;
			}
			
			currentIndex++;
		}
	}

	return false;
}

bool MidiControllerAutomationHandler::setParameterInverted(int index, bool value)
{
	int currentIndex = 0;

	for (int i = 0; i < 128; i++)
	{
		for (auto& a : automationData[i])
		{
			if (index == currentIndex)
			{
				a.inverted = value;
				return true;
			}

			currentIndex++;
		}
	}

	return false;
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

ControlledObject::ControlledObject(MainController *m, bool notifyOnShutdown) :
	controller(m),
	registerShutdown(notifyOnShutdown)
{
	if(registerShutdown)
		controller->registerControlledObject(this);

	jassert(m != nullptr);
};

ControlledObject::~ControlledObject()
{
	if(registerShutdown)
		controller->removeControlledObject(this);

	// Oops, this ControlledObject was not connected to a MainController
	jassert(controller != nullptr);

	masterReference.clear();
};

class DelayedRenderer::Pimpl
{
public:

	Pimpl() {}

	bool shouldDelayRendering() const 
	{
#if IS_STANDALONE_APP || IS_STANDALONE_FRONTEND || (HISE_EVENT_RASTER == 1)
		return false;
#else
		return hostType.isFruityLoops();
#endif
	}

#if !(IS_STANDALONE_APP || IS_STANDALONE_FRONTEND)
	PluginHostType hostType;
#endif

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Pimpl)
};

DelayedRenderer::DelayedRenderer(MainController* mc_) :
	pimpl(new Pimpl()),
	mc(mc_)
{
	memset(leftOverChannels, 0, sizeof(float)*HISE_NUM_PLUGIN_CHANNELS * HISE_EVENT_RASTER);
}

DelayedRenderer::~DelayedRenderer()
{
	pimpl = nullptr;
}

bool DelayedRenderer::shouldDelayRendering() const
{
	return pimpl->shouldDelayRendering();
}

CircularAudioSampleBuffer::CircularAudioSampleBuffer(int numChannels_, int numSamples) :
	internalBuffer(numChannels_, numSamples),
	numChannels(numChannels_),
	size(numSamples)
{
	internalBuffer.clear();
	internalMidiBuffer.ensureSize(1024);
}

bool CircularAudioSampleBuffer::writeSamples(const AudioSampleBuffer& source, int offsetInSource, int numSamples)
{
	jassert(source.getNumChannels() == internalBuffer.getNumChannels());

	const bool needsWrapping = writeIndex + numSamples > size;

	if (needsWrapping)
	{
		const int numSamplesBeforeWrap = size - writeIndex;

		if (numSamplesBeforeWrap > 0)
		{
			for (int i = 0; i < numChannels; i++)
			{
				auto w = internalBuffer.getWritePointer(i, writeIndex);
				auto r = source.getReadPointer(i, offsetInSource);

				FloatVectorOperations::copy(w, r, numSamplesBeforeWrap);
			}
		}

		const int numSamplesAfterWrap = numSamples - numSamplesBeforeWrap;

		if (numSamplesAfterWrap > 0)
		{
			for (int i = 0; i < numChannels; i++)
			{
				auto w = internalBuffer.getWritePointer(i, 0);
				auto r = source.getReadPointer(i, offsetInSource + numSamplesBeforeWrap);

				FloatVectorOperations::copy(w, r, numSamplesAfterWrap);
			}

			
		}

		writeIndex = numSamplesAfterWrap;
	}
	else
	{
		for (int i = 0; i < numChannels; i++)
		{
			auto w = internalBuffer.getWritePointer(i, writeIndex);
			auto r = source.getReadPointer(i, offsetInSource);

			FloatVectorOperations::copy(w, r, numSamples);
		}

		writeIndex += numSamples;
	}

	numAvailable += numSamples;

	const bool ok = numAvailable <= size;
	jassert(ok);
	return ok;
}

bool CircularAudioSampleBuffer::readSamples(AudioSampleBuffer& destination, int offsetInDestination, int numSamples)
{
	jassert(destination.getNumChannels() == internalBuffer.getNumChannels());

	numAvailable -= numSamples;

	jassert(numAvailable >= 0);

	const bool needsWrapping = readIndex + numSamples > size;

	if (needsWrapping)
	{
		const int numSamplesBeforeWrap = size - readIndex;

		if (numSamplesBeforeWrap > 0)
		{
			for (int i = 0; i < numChannels; i++)
			{
				auto r = internalBuffer.getReadPointer(i, readIndex);
				auto w = destination.getWritePointer(i, offsetInDestination);

				FloatVectorOperations::copy(w, r, numSamplesBeforeWrap);
			}
		}

		const int numSamplesAfterWrap = numSamples - numSamplesBeforeWrap;

		if (numSamplesAfterWrap > 0)
		{
			for (int i = 0; i < numChannels; i++)
			{
				auto r = internalBuffer.getReadPointer(i, 0);
				auto w = destination.getWritePointer(i, offsetInDestination + numSamplesBeforeWrap);

				FloatVectorOperations::copy(w, r, numSamplesAfterWrap);
			}
		}

		readIndex = numSamplesAfterWrap;
	}
	else
	{
		for (int i = 0; i < numChannels; i++)
		{
			auto r = internalBuffer.getReadPointer(i, readIndex);
			auto w = destination.getWritePointer(i, offsetInDestination);

			FloatVectorOperations::copy(w, r, numSamples);
		}

		readIndex += numSamples;
	}

	const bool ok = numAvailable >= 0;
	jassert(ok);
	return ok;
}

bool CircularAudioSampleBuffer::writeMidiEvents(const MidiBuffer& source, int offsetInSource, int numSamples)
{
	const bool needsWrapping = midiWriteIndex + numSamples > size;

	if (source.isEmpty())
	{
		midiWriteIndex = (midiWriteIndex + numSamples) % size;
		return numAvailable <= size;
	}
	
	if (needsWrapping)
	{
		const int numSamplesBeforeWrap = size - midiWriteIndex;

		if (numSamplesBeforeWrap > 0)
		{
			internalMidiBuffer.clear(midiWriteIndex, numSamplesBeforeWrap);
			internalMidiBuffer.addEvents(source, offsetInSource, numSamplesBeforeWrap, midiWriteIndex);
		}

		const int numSamplesAfterWrap = numSamples - numSamplesBeforeWrap;
		const int offsetAfterWrap = offsetInSource + numSamplesBeforeWrap;

		if (numSamplesAfterWrap > 0)
		{
			internalMidiBuffer.clear(0, numSamplesAfterWrap);
			internalMidiBuffer.addEvents(source, offsetAfterWrap, numSamplesAfterWrap, -offsetAfterWrap);
		}

		midiWriteIndex = numSamplesAfterWrap;
	}
	else
	{
		internalMidiBuffer.clear(midiWriteIndex, numSamples);
		internalMidiBuffer.addEvents(source, offsetInSource, numSamples, midiWriteIndex);

		midiWriteIndex += numSamples;
	}

	const bool ok = numAvailable <= size;
	jassert(ok);
	return ok;
}

bool CircularAudioSampleBuffer::readMidiEvents(MidiBuffer& destination, int offsetInDestination, int numSamples)
{
	const bool needsWrapping = midiReadIndex + numSamples > size;

	jassert(destination.isEmpty());

	if (needsWrapping)
	{
		const int numSamplesBeforeWrap = size - midiReadIndex;
		const int numSamplesAfterWrap = numSamples - numSamplesBeforeWrap;
		const int offsetAfterWrap = offsetInDestination + numSamplesBeforeWrap;
		const int offsetBeforeWrap = offsetInDestination - midiReadIndex;

		if (numSamplesAfterWrap > 0)
		{
			destination.addEvents(internalMidiBuffer, 0, numSamplesAfterWrap, offsetAfterWrap);
			internalMidiBuffer.clear(0, numSamplesAfterWrap);
		}


		if (numSamplesBeforeWrap > 0)
		{
			destination.addEvents(internalMidiBuffer, midiReadIndex, numSamplesBeforeWrap, offsetBeforeWrap);
			internalMidiBuffer.clear(midiReadIndex, numSamplesBeforeWrap);
		}
		
		
		midiReadIndex = numSamplesAfterWrap;
	}
	else
	{
		destination.addEvents(internalMidiBuffer, midiReadIndex, numSamples, offsetInDestination - midiReadIndex);
		internalMidiBuffer.clear(midiReadIndex, numSamples);

		midiReadIndex += numSamples;
	}

	const bool ok = numAvailable >= 0;
	jassert(ok);
	return ok;
}

void DelayedRenderer::processWrapped(AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
	// So FL Studio seems to violate the convention of not exceeding the buffer size set
	// in the prepareToPlay() callback
	auto maxBlockSize = jmin(mc->getMaximumBlockSize(), mc->getOriginalBufferSize());
	
	if (buffer.getNumSamples() > maxBlockSize)
	{
		// We'll recursively call this method with a smaller buffer to stay within the
		// buffer size limits...

		int numChannels = buffer.getNumChannels();
		int numToDo = buffer.getNumSamples();

		auto ptrs = (float**)alloca(numChannels * sizeof(float*) * numChannels);
		
		memcpy(ptrs, buffer.getArrayOfWritePointers(), sizeof(float*) * numChannels);

		int start = 0;

		while (numToDo > 0)
		{
			auto numThisTime = jmin(numToDo, maxBlockSize);

			AudioSampleBuffer chunk(ptrs, numChannels, numThisTime);
			
			delayedMidiBuffer.clear();
			delayedMidiBuffer.addEvents(midiMessages, start, numThisTime, -start);
			
			start += numThisTime;
			numToDo -= numThisTime;

			for (int i = 0; i < numChannels; i++)
				ptrs[i] += numThisTime;

			processWrapped(chunk, delayedMidiBuffer);
		}

		return;
	}
	else if (buffer.getNumSamples() % HISE_EVENT_RASTER != 0 || numLeftOvers != 0)
	{
#if FRONTEND_IS_PLUGIN
		jassertfalse;
		buffer.clear();
#else

		int numSamplesToCalculate = buffer.getNumSamples() - numLeftOvers;

		if (numSamplesToCalculate <= 0)
		{
			if (!midiMessages.isEmpty())
			{
				// We need to save the messages for the next block so that
				// they don't get dropped (FL Studio yay...)
				lastBlockSizeForShortBuffer = buffer.getNumSamples();
				MidiBuffer::Iterator it(midiMessages);

				MidiMessage m;
				int pos;

				while (it.getNextEvent(m, pos))
					shortBuffer.addEvent(m, pos);
			}

			// we need to calculate less samples than we have already available, so we can just return
			// the samples and skip the process for this buffer
			int numToCopy = buffer.getNumSamples();

			for (int c = 0; c < buffer.getNumChannels(); c++)
			{
				leftOverChannels[c] = leftOverData + c * HISE_EVENT_RASTER;

				for (int i = 0; i < numLeftOvers; i++)
				{
					if (i < numToCopy)
					{
						// Copy the sample to the buffer
						buffer.setSample(c, i, leftOverChannels[c][i]);
					}
					else
					{
						// shift the sample data in the left over buffer to the left by numCopy samples
						leftOverChannels[c][i - numToCopy] = leftOverChannels[c][i];
					}
				}
			}

			numLeftOvers -= numToCopy;
			return;
		}
		
		// use a temp buffer and pad to the event raster size...
		auto oddSampleAmount = numSamplesToCalculate % HISE_EVENT_RASTER;
		auto thisLeftOver = (HISE_EVENT_RASTER - oddSampleAmount) % HISE_EVENT_RASTER;

		int paddedBufferSize = (numSamplesToCalculate + thisLeftOver);
		int numToCopy = jmin(paddedBufferSize, buffer.getNumSamples());

		auto data = (float*)alloca(sizeof(float) * buffer.getNumChannels() * paddedBufferSize);

        FloatVectorOperations::clear(data, buffer.getNumChannels() * paddedBufferSize);
        
		float* ptrs[HISE_NUM_PLUGIN_CHANNELS];

		for (int i = 0; i < buffer.getNumChannels(); i++)
		{
			leftOverChannels[i] = leftOverData + i * HISE_EVENT_RASTER;

			ptrs[i] = data + i * paddedBufferSize;

			FloatVectorOperations::copy(ptrs[i], buffer.getReadPointer(i), numToCopy);

			if(thisLeftOver > 0 && (numToCopy + thisLeftOver) < paddedBufferSize)
				FloatVectorOperations::clear(ptrs[i] + numToCopy, thisLeftOver);
		}

		if (paddedBufferSize > 0)
		{
			auto lastTimestamp = (int)midiMessages.getLastEventTime();

            ignoreUnused(lastTimestamp);
			jassert(lastTimestamp < buffer.getNumSamples());

			AudioSampleBuffer tempBuffer(ptrs, buffer.getNumChannels(), paddedBufferSize);

			if (lastTimestamp > numSamplesToCalculate || !shortBuffer.isEmpty())
			{
				MidiBuffer::Iterator it(midiMessages);

				delayedMidiBuffer.clear();

				MidiMessage m;
				int pos;

				for (auto& e : shortBuffer)
					delayedMidiBuffer.addEvent(e.toMidiMesage(), e.getTimeStamp());

				while (it.getNextEvent(m, pos))
				{
					delayedMidiBuffer.addEvent(m, jmin(pos + lastBlockSizeForShortBuffer, numSamplesToCalculate));
				}

				delayedMidiBuffer.swapWith(midiMessages);

				shortBuffer.clear();
				lastBlockSizeForShortBuffer = 0;
			}

			mc->setMaxEventTimestamp(numSamplesToCalculate);
			mc->processBlockCommon(tempBuffer, midiMessages);
			mc->setMaxEventTimestamp(0);
		}

		for (int i = 0; i < buffer.getNumChannels(); i++)
		{
			if (numLeftOvers != 0)
				FloatVectorOperations::copy(buffer.getWritePointer(i), leftOverChannels[i], numLeftOvers);

			if(numSamplesToCalculate > 0)
				FloatVectorOperations::copy(buffer.getWritePointer(i, numLeftOvers), ptrs[i], numSamplesToCalculate);

			if(thisLeftOver != 0)
				FloatVectorOperations::copy(leftOverChannels[i], ptrs[i] + numSamplesToCalculate, thisLeftOver);
		}

		numLeftOvers = thisLeftOver;
#endif
	}
	else
	{
		mc->processBlockCommon(buffer, midiMessages);
	}
}

void DelayedRenderer::prepareToPlayWrapped(double sampleRate, int samplesPerBlock)
{
	illegalBufferSize = !(samplesPerBlock % HISE_EVENT_RASTER == 0);

#if HISE_COMPLAIN_ABOUT_ILLEGAL_BUFFER_SIZE
	if (illegalBufferSize)
		mc->sendOverlayMessage(OverlayMessageBroadcaster::IllegalBufferSize);
#endif

    if(samplesPerBlock % HISE_EVENT_RASTER != 0)
        samplesPerBlock += HISE_EVENT_RASTER - (samplesPerBlock % HISE_EVENT_RASTER);

	mc->prepareToPlay(sampleRate, jmin(samplesPerBlock, mc->getMaximumBlockSize()));
}


OverlayMessageBroadcaster::Listener::~Listener()
{
	masterReference.clear();
}

OverlayMessageBroadcaster::OverlayMessageBroadcaster():
	internalUpdater(this)
{

}

OverlayMessageBroadcaster::~OverlayMessageBroadcaster()
{}

void OverlayMessageBroadcaster::addOverlayListener(Listener* listener)
{
	listeners.addIfNotAlreadyThere(listener);
}

void OverlayMessageBroadcaster::removeOverlayListener(Listener* listener)
{
	listeners.removeAllInstancesOf(listener);
}

bool OverlayMessageBroadcaster::isUsingDefaultOverlay() const
{ return useDefaultOverlay; }

void OverlayMessageBroadcaster::setUseDefaultOverlay(bool shouldUseOverlay)
{
	useDefaultOverlay = shouldUseOverlay;
}

OverlayMessageBroadcaster::InternalAsyncUpdater::InternalAsyncUpdater(OverlayMessageBroadcaster* parent_): parent(parent_)
{}

void OverlayMessageBroadcaster::InternalAsyncUpdater::handleAsyncUpdate()
{
	ScopedLock sl(parent->listeners.getLock());

	for (int i = 0; i < parent->listeners.size(); i++)
	{
		if (parent->listeners[i].get() != nullptr)
		{
			parent->listeners[i]->overlayMessageSent(parent->currentState, parent->customMessage);
		}
		else
		{
			parent->listeners.remove(i--);
		}
	}
}

void OverlayMessageBroadcaster::sendOverlayMessage(int newState, const String& newCustomMessage/*=String()*/)
{
	if (currentState == DeactiveOverlay::State::CriticalCustomErrorMessage)
		return;

#if USE_BACKEND

	

	// Just print it on the console
	Logger::getCurrentLogger()->writeToLog("!" + newCustomMessage);
#endif

	currentState = newState;
	customMessage = newCustomMessage;

	internalUpdater.triggerAsyncUpdate();
}

String OverlayMessageBroadcaster::getOverlayTextMessage(State s) const
{
	switch (s)
	{
	case AppDataDirectoryNotFound:
		return "The application directory is not found. (The installation seems to be broken. Please reinstall this software.)";
		break;
	case IllegalBufferSize:
	{
		String s;
		s << "The audio buffer size should be a multiple of " << String(HISE_EVENT_RASTER) << ". Please adjust your audio settings";
		return s;
	}
	case SamplesNotFound:
		return "The sample directory could not be located. \nClick below to choose the sample folder.";
		break;
	case SamplesNotInstalled:
#if HISE_SAMPLE_DIALOG_SHOW_INSTALL_BUTTON && HISE_SAMPLE_DIALOG_SHOW_LOCATE_BUTTON
		return "Please click below to install the samples from the downloaded archive or point to the location where you've already installed the samples.";
#elif HISE_SAMPLE_DIALOG_SHOW_INSTALL_BUTTON
		return "Please click below to install the samples from the downloaded archive.";
#elif HISE_SAMPLE_DIALOG_SHOW_LOCATE_BUTTON
		return "Please click below to point to the location where you've already installed the samples.";
#else
		return "This should never show :)";
		jassertfalse;
#endif

		break;
	case LicenseNotFound:
	{
#if USE_COPY_PROTECTION
#if HISE_ALLOW_OFFLINE_ACTIVATION
		return "This computer is not registered.\nClick below to authenticate this machine using either online authorization or by loading a license key.";
#else
		return "This computer is not registered.";
#endif
#else
		return "";
#endif
	}
	case ProductNotMatching:
		return "The license key is invalid (wrong plugin name / version).\nClick below to locate the correct license key for this plugin / version";
		break;
	case MachineNumbersNotMatching:
		return "The machine ID is invalid / not matching.\nThis might be caused by a major OS / or system hardware update which change the identification of this computer.\nIn order to solve the issue, just repeat the activation process again to register this system with the new specifications.";
		break;
	case UserNameNotMatching:
		return "The user name is invalid.\nThis means usually a corrupt or rogued license key file. Please contact support to get a new license key.";
		break;
	case EmailNotMatching:
		return "The email name is invalid.\nThis means usually a corrupt or rogued license key file. Please contact support to get a new license key.";
		break;
	case LicenseInvalid:
	{
#if USE_COPY_PROTECTION && !USE_SCRIPT_COPY_PROTECTION
		auto ul = &dynamic_cast<const FrontendProcessor*>(this)->unlocker;
		return ul->getProductErrorMessage();
#else
		return "";
#endif
	}
	case LicenseExpired:
	{
#if USE_COPY_PROTECTION
		return "The license key is expired. Press OK to reauthenticate (you'll need to be online for this)";
#else
		return "";
#endif
	}
	case State::CustomErrorMessage:
	case State::CriticalCustomErrorMessage:
	case State::CustomInformation:
	case State::numReasons:
		jassertfalse;
		break;
	default:
		break;
	}

	return String();
}

ScopedSoftBypassDisabler::ScopedSoftBypassDisabler(MainController* mc) :
	ControlledObject(mc)
{
	previousState = mc->shouldUseSoftBypassRamps();
	mc->setAllowSoftBypassRamps(false);
}

ScopedSoftBypassDisabler::~ScopedSoftBypassDisabler()
{
	getMainController()->setAllowSoftBypassRamps(previousState);
}

} // namespace hise
