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

#if USE_BACKEND
void Processor::debugProcessor(const String &t)
{

	if(consoleEnabled) debugToConsole(this, t);

};

#endif

Processor::Processor(MainController *m, const String &id_, int numVoices_) :
	ControlledObject(m),
#if HISE_NEW_PROCESSOR_DISPATCH
	dispatcher(m->getProcessorDispatchHandler(), *this, dispatch::HashedCharPtr(id_)),
#endif
	otherBroadcaster(*this),
	id(id_),
	consoleEnabled(false),
	bypassed(false),
	visible(true),
	samplerate(-1.0),
	largestBlockSize(-1),
	inputValue(0.0f),
	outputValue(0.0f),
	editorState(0),
	numVoices(numVoices_),
	symbol(Path())
{
	WARN_IF_AUDIO_THREAD(true, MainController::KillStateHandler::ProcessorInsertion);

	editorStateIdentifiers.add("Folded");
	editorStateIdentifiers.add("BodyShown");
	editorStateIdentifiers.add("Visible");
	editorStateIdentifiers.add("Solo");

	setEditorState(Processor::BodyShown, true, dontSendNotification);
	setEditorState(Processor::Visible, true, dontSendNotification);
	setEditorState(Processor::Solo, false, dontSendNotification);

	if (Identifier::isValidIdentifier(id))
	{
		idAsIdentifier = Identifier(id);
	}

	otherBroadcaster.enablePooledUpdate(getMainController()->getGlobalUIUpdater());

	//enableAllocationFreeMessages(50);
}

Processor::~Processor()
{
	WARN_IF_AUDIO_THREAD(true, MainController::KillStateHandler::ProcessorDestructor);

	otherBroadcaster.removeAllChangeListeners();

	getMainController()->getMacroManager().removeMacroControlsFor(this);

	

	masterReference.clear();
}

juce::ValueTree Processor::exportAsValueTree() const
{
	WARN_IF_AUDIO_THREAD(true, MainController::KillStateHandler::ValueTreeOperation);

#if USE_OLD_FILE_FORMAT
	ValueTree v(getType());
#else
	ValueTree v("Processor");
	v.setProperty("Type", getType().toString(), nullptr);
#endif

	v.setProperty("ID", getId(), nullptr);
	v.setProperty("Bypassed", isBypassed(), nullptr);

	ScopedPointer<XmlElement> editorValueSet = new XmlElement("EditorStates");
	editorStateValueSet.copyToXmlAttributes(*editorValueSet);

#if USE_OLD_FILE_FORMAT
	v.setProperty("EditorState", editorValueSet->createDocument(""), nullptr);

	for (int i = 0; i < getNumChildProcessors(); i++)
	{
		v.addChild(getChildProcessor(i)->exportAsValueTree(), i, nullptr);
	};

#else
	ValueTree editorStateValueTree = ValueTree::fromXml(*editorValueSet);
	v.addChild(editorStateValueTree, -1, nullptr);

	ValueTree childProcessors("ChildProcessors");

	for (int i = 0; i < getNumChildProcessors(); i++)
	{
		childProcessors.addChild(getChildProcessor(i)->exportAsValueTree(), i, nullptr);
	};

	v.addChild(childProcessors, -1, nullptr);
#endif
	return v;
}

void Processor::restoreFromValueTree(const ValueTree &previouslyExportedProcessorState)
{
	WARN_IF_AUDIO_THREAD(true, MainController::KillStateHandler::ValueTreeOperation);

	const ValueTree &v = previouslyExportedProcessorState;

	jassert(Identifier(v.getProperty("Type", String())) == getType());

	jassert(v.getProperty("ID", String()) == getId());
	setBypassed(v.getProperty("Bypassed", false));

#if 0
	ScopedPointer<XmlElement> editorValueSet = v.getChildWithName("EditorStates").createXml();
	
	if(editorValueSet != nullptr)
	{
		if (!editorValueSet->hasAttribute("Visible") && (dynamic_cast<Chain*>(this) == nullptr || dynamic_cast<ModulatorSynth*>(this) != nullptr)) editorValueSet->setAttribute("Visible", true); // Compatibility for old patches

		editorStateValueSet.setFromXmlAttributes(*editorValueSet);
	}
#endif

	ValueTree childProcessors = v.getChildWithName("ChildProcessors");

	jassert(childProcessors.isValid());

	if(Chain *c = dynamic_cast<Chain*>(this))
	{
		if( !c->restoreChain(childProcessors)) return;
	}

	for(int i = 0; i < getNumChildProcessors(); i++)
	{
		Processor *p = getChildProcessor(i);

		for (int j = 0; j < childProcessors.getNumChildren(); j++)
		{
			if (childProcessors.getChild(j).getProperty("ID") == p->getId())
			{
				p->restoreFromValueTree(childProcessors.getChild(j));
				break;
			}
		}	
	}
}

ProcessorDocumentation* Processor::createDocumentation() const
{ return nullptr; }

const Path Processor::getSymbol() const
{
	if(symbol.isEmpty())
	{
		return getSpecialSymbol();
	}
	else return symbol;
}

void Processor::setSymbol(Path newSymbol)
{symbol = newSymbol;	}

void Processor::setAttribute(int parameterIndex, float newValue, dispatch::DispatchType notifyEditor)
{
	setInternalAttribute(parameterIndex, newValue);

#if HISE_OLD_PROCESSOR_DISPATCH
	if(notifyEditor == dispatch::DispatchType::sendNotification)
	{
		sendOtherChangeMessage(dispatch::library::ProcessorChangeEvent::Attribute, notifyEditor);
	}
#endif
#if HISE_NEW_PROCESSOR_DISPATCH
	dispatcher.setAttribute(parameterIndex, newValue, notifyEditor);
#endif

	
}

float Processor::getDefaultValue(int) const
{ return 1.0f; }

int Processor::getNumInternalChains() const
{ return 0;}

void Processor::enableConsoleOutput(bool shouldBeEnabled)
{consoleEnabled = shouldBeEnabled;}

Colour Processor::getColour() const
{
	return Colours::grey;
}

int Processor::getVoiceAmount() const noexcept
{ return numVoices; }

const String& Processor::getId() const
{return id;}

const String Processor::getName() const
{
	return getType().toString();
}

void Processor::setId(const String& newId, NotificationType notifyChangeHandler)
{
	id = newId;

	if (Identifier::isValidIdentifier(id))
	{
		idAsIdentifier = Identifier(id);
	}
	else
	{
		idAsIdentifier = Identifier();
	}

#if HISE_OLD_PROCESSOR_DISPATCH
	otherBroadcaster.sendChangeMessage();

	if (notifyChangeHandler)
		getMainController()->getProcessorChangeHandler().sendProcessorChangeMessage(this, 
			MainController::ProcessorChangeHandler::EventType::ProcessorRenamed, false);
#endif
#if HISE_NEW_PROCESSOR_DISPATCH
	dispatcher.setId(dispatch::HashedCharPtr(newId));
#endif


}

const Identifier& Processor::getIDAsIdentifier() const
{
	return idAsIdentifier;
}

void Processor::setBypassed(bool shouldBeBypassed, NotificationType notifyChangeHandler) noexcept
{ 
	if (bypassed != shouldBeBypassed)
	{
		bypassed = shouldBeBypassed;
		currentValues.clear();

#if HISE_OLD_PROCESSOR_DISPATCH
#if 0
		sendSynchronousBypassChangeMessage();

		if (notifyChangeHandler)
			getMainController()->getProcessorChangeHandler().sendProcessorChangeMessage(this, MainController::ProcessorChangeHandler::EventType::ProcessorBypassed, false);
#endif
#endif
#if HISE_NEW_PROCESSOR_DISPATCH
		dispatcher.setBypassed(shouldBeBypassed, (dispatch::DispatchType)notifyChangeHandler);
#endif
	}		
}

bool Processor::isBypassed() const noexcept
{ return bypassed; }

void Processor::sendSynchronousBypassChangeMessage()
{
	for (auto l : bypassListeners)
		if (l)
			l->bypassStateChanged(this, bypassed);
}

void Processor::prepareToPlay(double sampleRate_, int samplesPerBlock_)
{
	samplerate = sampleRate_;
	largestBlockSize = samplesPerBlock_;
}

double Processor::getSampleRate() const
{ return samplerate; }

int Processor::getLargestBlockSize() const
{   
	return largestBlockSize;
}

void Processor::sendOtherChangeMessage(dispatch::library::ProcessorChangeEvent eventType, dispatch::DispatchType t)
{
	#if HISE_OLD_PROCESSOR_DISPATCH
	if(t == sendNotificationSync)
		otherBroadcaster.sendSynchronousChangeMessage();
	else
		otherBroadcaster.sendChangeMessage();
	#endif
	#if HISE_NEW_PROCESSOR_DISPATCH

	if(eventType > dispatch::library::ProcessorChangeEvent::numProcessorChangeEvents)
	{
		return;
	}

	dispatcher.sendChangeMessage(eventType, t);
	#endif
}

float Processor::getOutputValue() const
{	return currentValues.outL;	}

float Processor::getInputValue() const
{
	return inputValue;
}

void Processor::setEditorState(int state, bool isOn, NotificationType notifyView)
{
	const Identifier stateId = getEditorStateForIndex(state);

	editorStateValueSet.set(stateId, isOn);
}

const var Processor::getEditorState(Identifier editorStateId) const
{
	return editorStateValueSet[editorStateId];
}

void Processor::setEditorState(Identifier state, var stateValue, NotificationType notifyView)
{
	jassert(state.isValid());

	editorStateValueSet.set(state, stateValue);
}

Identifier Processor::getEditorStateForIndex(int index) const
{
	// Did you forget to add the identifier to the id list in your subtype constructor?
	jassert(index < editorStateIdentifiers.size());

	return editorStateIdentifiers[index];
}

void Processor::toggleEditorState(int index, NotificationType notifyEditor)
{
	bool on = getEditorState(getEditorStateForIndex(index));

	setEditorState(getEditorStateForIndex(index), !on, notifyEditor);
}

bool Processor::getEditorState(int state) const
{
	return editorStateValueSet[getEditorStateForIndex(state)];

	//return editorState[state];
}

XmlElement* Processor::getCompleteEditorState() const
{
	XmlElement *state = new XmlElement("EditorState");

	editorStateValueSet.copyToXmlAttributes(*state);

	return state;
}

void Processor::restoreCompleteEditorState(const XmlElement* storedState)
{
	if(storedState != nullptr)
	{
		editorStateValueSet.setFromXmlAttributes(*storedState);
	}
}

Processor::DisplayValues::DisplayValues():
	inL(0.0f),
	inR(0.0f),
	outL(0.0f),
	outR(0.0f)
{}

void Processor::DisplayValues::clear()
{
	inL = 0.0f;
	inR = 0.0f;
	outL = 0.0f;
	outR = 0.0f;
}

Processor::DisplayValues Processor::getDisplayValues() const
{ return currentValues;}

Processor::BypassListener::BypassListener(dispatch::RootObject& r)
  NEW_PROCESSOR_DISPATCH(:dispatcher(r, *this, BIND_MEMBER_FUNCTION_2(BypassListener::onBypassUpdate)))
{}

Processor::BypassListener::~BypassListener()
{}

void Processor::BypassListener::onBypassUpdate(dispatch::library::Processor* p, bool state)
{
    NEW_PROCESSOR_DISPATCH(bypassStateChanged(&p->getOwner<hise::Processor>(), state));
}

String Processor::getDescriptionForParameters(int parameterIndex)
{
	if (parameterNames.size() == parameterDescriptions.size())
		return parameterDescriptions[parameterIndex];

	return "-";
}

bool Processor::isOnAir() const noexcept
{ return onAir; }

Processor::DeleteListener::~DeleteListener()
{
	masterReference.clear();
}

void Processor::addDeleteListener(DeleteListener* listener)
{
	deleteListeners.addIfNotAlreadyThere(listener);
}

void Processor::setIsWaitingForDeletion()
{
	// Must be called before this method
	jassert(!isOnAir());
	pendingDelete = true;

	for (int i = 0; i < getNumChildProcessors(); i++)
		getChildProcessor(i)->setIsWaitingForDeletion();
}

bool Processor::isWaitingForDeletion() const noexcept
{
	return pendingDelete;
}

void Processor::removeDeleteListener(DeleteListener* listener)
{
	deleteListeners.removeAllInstancesOf(listener);
}

void Processor::addBypassListener(BypassListener* l, dispatch::DispatchType n)
{
#if HISE_NEW_PROCESSOR_DISPATCH
	dispatcher.addBypassListener(&l->dispatcher, n);
#endif
#if HISE_OLD_PROCESSOR_DISPATCH
	bypassListeners.addIfNotAlreadyThere(l);
#endif
}

void Processor::removeBypassListener(BypassListener* l)
{
#if HISE_NEW_PROCESSOR_DISPATCH
	dispatcher.removeBypassListener(&l->dispatcher);
#endif
#if HISE_OLD_PROCESSOR_DISPATCH
	bypassListeners.removeAllInstancesOf(l);
#endif
	
}

void Processor::setParentProcessor(Processor* newParent)
{
	// You must call setParentProcessor before locking and inserting to the processing chain
	jassert(!isOnAir());
	jassert(!LockHelpers::isLockedBySameThread(getMainController(), LockHelpers::Type::IteratorLock));
	jassert(!LockHelpers::isLockedBySameThread(getMainController(), LockHelpers::Type::AudioLock));

	parentProcessor = newParent;

	for (int i = 0; i < getNumChildProcessors(); i++)
		getChildProcessor(i)->setParentProcessor(this);
}

Path Processor::getSpecialSymbol() const
{ return Path(); }

void Processor::setOutputValue(float newValue)
{
	currentValues.outL = newValue;

	//outputValue = (double)newValue;
	//sendChangeMessage();
}

void Processor::setInputValue(float newValue, NotificationType notify)
{
	inputValue = newValue;

	if(notify == sendNotification)
	{
		//otherBroadcaster.sendPooledChangeMessage();
	}
};

void Processor::setConstrainerForAllInternalChains(BaseConstrainer *constrainer)
{
	FactoryType::Constrainer* c = static_cast<FactoryType::Constrainer*>(constrainer);

	for (int i = 0; i < getNumInternalChains(); i++)
	{
		ModulatorChain *mc = dynamic_cast<ModulatorChain*>(getChildProcessor(i));

		if (mc != nullptr)
		{
			mc->getFactoryType()->setConstrainer(c, false);

			for (int j = 0; j < mc->getNumChildProcessors(); j++)
			{
				mc->getChildProcessor(j)->setConstrainerForAllInternalChains(constrainer);
			}
		}
	}
}

Identifier Processor::getIdentifierForParameterIndex(int parameterIndex) const
{
	// Use ProcessorWithScriptingContent
	jassert(dynamic_cast<const ProcessorWithScriptingContent*>(this) == nullptr);

	if (parameterIndex > parameterNames.size()) return Identifier();

	return parameterNames[parameterIndex];
}

int Processor::getParameterIndexForIdentifier(const Identifier& id) const
{
	// Use ProcessorWithScriptingContent
	jassert(dynamic_cast<const ProcessorWithScriptingContent*>(this) == nullptr);

	return parameterNames.indexOf(id);
}


int Processor::getNumParameters() const
{
	if (auto jp = dynamic_cast<const JavascriptProcessor*>(this))
	{
		if (auto n = jp->getActiveOrDebuggedNetwork())
			return n->networkParameterHandler.getNumParameters();
	}

	if (auto pwsc = dynamic_cast<const ProcessorWithScriptingContent*>(this))
		return pwsc->getNumScriptParameters();
	else
		return parameterNames.size();
}

void Processor::setIsOnAir(bool shouldBeOnAir)
{
	// This should be called only with a locked iterator lock during insertion...
	jassert(!shouldBeOnAir || LockHelpers::isLockedBySameThread(getMainController(), LockHelpers::Type::IteratorLock));
	jassert(!shouldBeOnAir || LockHelpers::isLockedBySameThread(getMainController(), LockHelpers::Type::AudioLock));

	// You must call setIsOnAir after setParentProcessor
	isValidAndInitialised(false);

	onAir = shouldBeOnAir;

	for (int i = 0; i < getNumChildProcessors(); i++)
	{
		getChildProcessor(i)->setIsOnAir(shouldBeOnAir);
	}
}


void Processor::sendDeleteMessage()
{
	jassert_message_thread;

	int numListeners = deleteListeners.size();

	for (int i = numListeners - 1; i >= 0; --i)
	{
		if (deleteListeners[i].get() != nullptr)
		{
			deleteListeners[i]->processorDeleted(this);
		}
	}
}

bool Processor::isRebuildMessagePending() const noexcept
{
	if (rebuildMessagePending)
		return true;

	auto parent = getParentProcessor(false, false);

	if (parent != nullptr)
		return parent->isRebuildMessagePending();

	return false;
}

void Processor::cleanRebuildFlagForThisAndParents()
{
	if (rebuildMessagePending)
	{
		rebuildMessagePending = false;

		if (auto parent = getParentProcessor(false))
			parent->cleanRebuildFlagForThisAndParents();
	}
}

void Processor::sendRebuildMessage(bool forceUpdate/*=false*/)
{
	
	if (!isRebuildMessagePending())
	{
		rebuildMessagePending = true;

		auto f = [forceUpdate](Dispatchable* obj)
		{
			auto p = static_cast<Processor*>(obj);
			auto pToUse = p;

			while (auto parent = pToUse->getParentProcessor(false, false))
			{
				if (parent->isRebuildMessagePending())
				{
					DBG("Skipping rebuilding for " + pToUse->getId());
					pToUse = parent;
				}

				else
					break;
			}

			for (auto l : pToUse->deleteListeners)
			{
				if (p->getMainController()->shouldAbortMessageThreadOperation())
					return Status::needsToRunAgain;

				if (l.get() != nullptr)
					l->updateChildEditorList(forceUpdate);
			}

			p->cleanRebuildFlagForThisAndParents();
			return Status::OK;
		};

		getMainController()->getLockFreeDispatcher().callOnMessageThreadAfterSuspension(this, f);
	}
}

const hise::Processor* Processor::getParentProcessor(bool getOwnerSynth, bool assertIfFalse) const
{
	if (getMainController()->isFlakyThreadingAllowed())
		assertIfFalse = false;

	if (parentProcessor == nullptr)
	{
		ASSERT_STRICT_PROCESSOR_STRUCTURE(!assertIfFalse || this == getMainController()->getMainSynthChain());
		return nullptr;
	}
		

	if (getOwnerSynth)
	{
		if (dynamic_cast<ModulatorSynth*>(parentProcessor.get()) != nullptr)
			return parentProcessor;
		else
			return parentProcessor->getParentProcessor(true, assertIfFalse);
	}
	else
	{
		return parentProcessor;
	}
}


hise::Processor* Processor::getParentProcessor(bool getOwnerSynth, bool assertIfFalse)
{
	return const_cast<Processor*>(const_cast<const Processor*>(this)->getParentProcessor(getOwnerSynth, assertIfFalse));
}

bool Chain::restoreChain(const ValueTree &v)
{
	Processor *thisAsProcessor = dynamic_cast<Processor*>(this);

	jassert(thisAsProcessor != nullptr);


	jassert(v.getType().toString() == "ChildProcessors");

	bool wasOnAir = thisAsProcessor->isOnAir();

	getHandler()->clearAsync(nullptr);

	if(thisAsProcessor->isOnAir() != wasOnAir)
	{
		LockHelpers::SafeLock itLock(thisAsProcessor->getMainController(), LockHelpers::Type::IteratorLock);
		LockHelpers::SafeLock audioLock(thisAsProcessor->getMainController(), LockHelpers::Type::AudioLock);

		thisAsProcessor->setIsOnAir(wasOnAir);
	}

	for(int i = 0; i < v.getNumChildren(); i++)
	{
		const bool isFixedInternalChain = i < thisAsProcessor->getNumChildProcessors();

		const bool isNoProcessorChild = v.getChild(i).getType() != Identifier("Processor");

		if (isNoProcessorChild || isFixedInternalChain)
		{
			continue; // They will be restored in Processor::restoreFromValueTree
		}
		else
		{

			Processor *p = MainController::createProcessor(getFactoryType(), v.getChild(i).getProperty("Type", String()).toString(), v.getChild(i).getProperty("ID"));
			
			if(p == nullptr)
			{
				String errorMessage;
				
				errorMessage << "The Processor (" << v.getChild(i).getType().toString() << ") " << v.getChild(i).getProperty("ID").toString() << "could not be generated. Skipping!";

				return false;
			}
			else
			{
				getHandler()->add(p, nullptr);
			}
		}
	}

	jassert(v.getNumChildren() == thisAsProcessor->getNumChildProcessors());

	return (v.getNumChildren() == thisAsProcessor->getNumChildProcessors());
};

bool FactoryType::countProcessorsWithSameId(int &index, const Processor *p, Processor *processorToLookFor, const String &nameToLookFor)
{   
	auto pName = p->getId();

	auto pNumber = String(pName.getTrailingIntValue());

	if (pNumber.isNotEmpty())
		pName = pName.upToLastOccurrenceOf(pNumber, false, false);

	if (pName == nameToLookFor)
	{
		index++;
	}

	if (p == processorToLookFor)
	{
		// Do not look
		return false;
	}
	
	const int numChildren = p->getNumChildProcessors();

	for (int i = 0; i < numChildren; i++)
	{
		bool lookFurther = countProcessorsWithSameId(index, p->getChildProcessor(i), processorToLookFor, nameToLookFor);

		if (!lookFurther) return false;
	}
	
	return true;
}


String FactoryType::getUniqueName(Processor *id, String name/*=String()*/)
{
	ModulatorSynthChain *chain = id->getMainController()->getMainSynthChain();

	if (id == chain) return chain->getId();

	int amount = 0;

	if(name.isEmpty()) name = id->getId();



	auto lastNumbers = String(name.getTrailingIntValue());

	if (lastNumbers.isNotEmpty())
		name = name.upToLastOccurrenceOf(lastNumbers, false, false);

	countProcessorsWithSameId(amount, chain, id, name);

	// If this happens, you haven't added the processor yet.
	jassert(amount != 0);

	name = name + String(amount);
	
	return name;
}


Processor *ProcessorHelpers::getFirstProcessorWithName(const Processor *root, const String &name)
{
    if(root->getId() == name)
        return const_cast<Processor*>(root);
    
	Processor::Iterator<Processor> iter(const_cast<Processor*>(root), false);

	Processor *p;

	while((p = iter.getNextProcessor()) != nullptr)
	{
		if(p->getId() == name) return p;
	}

	return nullptr;
}


Array<WeakReference<Processor>> ProcessorHelpers::getListOfAllGlobalModulators(const Processor* rootProcessor)
{
	Array<WeakReference<Processor>> mods;

	// Only checks the first global modulator container, should be enough...
	if (auto container = getFirstProcessorWithType<GlobalModulatorContainer>(rootProcessor))
	{
		auto chain = container->getChildProcessor(ModulatorSynth::GainModulation);

		for (int i = 0; i < chain->getNumChildProcessors(); i++)
		{
			mods.add(chain->getChildProcessor(i));
		}
	}

	return mods;
}

const Processor *ProcessorHelpers::findParentProcessor(const Processor *childProcessor, bool getParentSynth)
{
	return const_cast<const Processor*>(findParentProcessor(const_cast<Processor*>(childProcessor), getParentSynth));
}



Processor * ProcessorHelpers::findParentProcessor(Processor *childProcessor, bool getParentSynth)
{
	if (childProcessor->getMainController()->getMainSynthChain() == childProcessor)
		return nullptr;

	if (auto p = childProcessor->getParentProcessor(getParentSynth))
		return p;

	

	Processor *root = const_cast<Processor*>(childProcessor)->getMainController()->getMainSynthChain();
	Processor::Iterator<Processor> iter(root, false);

	Processor *p;
	Processor *lastSynth = nullptr;

	if (getParentSynth)
	{
		while ((p = iter.getNextProcessor()) != nullptr)
		{
			if (is<ModulatorSynth>(childProcessor))
			{
				if (is<Chain>(p))
				{
					Chain::Handler *handler = dynamic_cast<Chain*>(p)->getHandler();
					int numChildSynths = handler->getNumProcessors();

					for (int i = 0; i < numChildSynths; i++)
					{
						if (handler->getProcessor(i) == childProcessor) return p;
					}
				}
			}
			else
			{
				if (is<ModulatorSynth>(p)) lastSynth = p;

				if (p == childProcessor) return lastSynth;
			}
		}
	}
	else
	{
		while ((p = iter.getNextProcessor()) != nullptr)
		{
			for (int i = 0; i < p->getNumChildProcessors(); i++)
			{
				if (p->getChildProcessor(i) == childProcessor) return p;
			}
		}
	}

	return nullptr;
}

juce::NotificationType ProcessorHelpers::getAttributeNotificationType()
{
#if USE_FRONTEND && HI_DONT_SEND_ATTRIBUTE_UPDATES
        return dontSendNotification;
#else
	return sendNotificationAsync;
#endif
}

template <class ProcessorType>
int ProcessorHelpers::getAmountOf(const Processor *rootProcessor, const Processor *upTochildProcessor /*= nullptr*/)
{
	Processor::Iterator<ProcessorType> iter(rootProcessor);

	int count = 0;

	while (ProcessorType *p = iter.getNextProcessor())
	{
		
		if (upTochildProcessor != nullptr && p == upTochildProcessor)
		{
			break;
		}

		count++;
	}

	return count;
}



juce::StringArray ProcessorHelpers::getAllIdsForDataType(const Processor* rootProcessor, snex::ExternalData::DataType dataType)
{
	Processor::Iterator<ExternalDataHolder> iter(rootProcessor);

	StringArray sa;

	while (auto p = iter.getNextProcessor())
	{
		if (p->getNumDataObjects(dataType) > 0)
		{
			sa.add(dynamic_cast<Processor*>(p)->getId());
		}
	}

	return sa;
}

hise::MarkdownLink ProcessorHelpers::getMarkdownLink(const Processor* p)
{
	static const String wildcard = "/hise-modules/";

	String s = wildcard;

	if (auto modChain = dynamic_cast<const ModulatorChain*>(p))
	{
		return { File(), "/hise-modules/modulators/" };
	}
	
	if (auto fxChain = dynamic_cast<const EffectProcessorChain*>(p))
	{
		return { File(), "/hise-modules/effects/" };
	}

	if (auto midiChain = dynamic_cast<const MidiProcessorChain*>(p))
	{
		return { File(), "/hise-modules/midi-processors/" };
	}

	if (auto mod = dynamic_cast<const Modulator*>(p))
	{
		s << "modulators/";

		if (auto tv = dynamic_cast<const TimeVariantModulator*>(p))
		{
			s << "time-variant-modulators/";
		}
		else if (auto vs = dynamic_cast<const VoiceStartModulator*>(p))
		{
			s << "voice-start-modulators/";
		}
		else
			s << "envelopes/";
	}
	else if (auto mp = dynamic_cast<const MidiProcessor*>(p))
	{
		s << "midi-processors/";
	}
	else if (auto fx = dynamic_cast<const EffectProcessor*>(p))
	{
		s << "effects/";
	}
	else
	{
		s << "sound-generators/";
	}

	s << "list/";



	s << MarkdownLink::Helpers::getSanitizedFilename(p->getType().toString());

	return MarkdownLink(File(), s);
}

bool ProcessorHelpers::isHiddableProcessor(const Processor *p)
{
	return dynamic_cast<const Chain*>(p) == nullptr ||
		   dynamic_cast<const ModulatorSynthChain*>(p) != nullptr ||
		   dynamic_cast<const ModulatorSynthGroup*>(p) != nullptr;
}

String ProcessorHelpers::getPrettyNameForAutomatedParameter(const Processor* p, int parameterIndex)
{
	if (p == nullptr)
		return String();

	if (auto sp = dynamic_cast<const ProcessorWithScriptingContent*>(p))
	{
		

		if (auto sc = sp->getScriptingContent()->getComponent(parameterIndex))
		{
			auto pluginParameterName = sc->getScriptObjectProperty(ScriptingApi::Content::ScriptComponent::Properties::pluginParameterName).toString();

			if (pluginParameterName.isNotEmpty())
			{
				return pluginParameterName;
			}
		}
	}

	// Fallback...
	return p->getIdentifierForParameterIndex(parameterIndex).toString();
}

String ProcessorHelpers::getScriptVariableDeclaration(const Processor *p, bool copyToClipboard/*=true*/)
{
	String typeName;

	if (is<ModulatorSynth>(p)) typeName = "ChildSynth";
	else if (is<Modulator>(p)) typeName = "Modulator";
	else if (is<MidiProcessor>(p)) typeName = "MidiProcessor";
	else if (is<EffectProcessor>(p)) typeName = "Effect";
	else return String();

	return getTypedScriptVariableDeclaration(p, typeName, copyToClipboard);

}

juce::String ProcessorHelpers::getTypedScriptVariableDeclaration(const Processor* p, String typeName, bool copyToClipboard/*=true*/)
{
	String code;

	String name = p->getId();
	String id = name.removeCharacters(" \n\t\"\'!$%&/()");

	code << "const var " << id << " = Synth.get" << typeName << "(\"" << name << "\");";

	if (copyToClipboard)
	{
		debugToConsole(const_cast<Processor*>(p), "'" + code + "' was copied to Clipboard");
		SystemClipboard::copyTextToClipboard(code);
	}

	return code;
}

String ProcessorHelpers::getBase64String(const Processor* p, bool copyToClipboard/*=true*/, bool exportContentOnly/*=false*/)
{
	if (exportContentOnly)
	{
		if (auto pwsc = dynamic_cast<const ProcessorWithScriptingContent*>(p))
		{
			auto vt = pwsc->getScriptingContent()->exportAsValueTree();
			return ValueTreeHelpers::getBase64StringFromValueTree(vt);
		}
		else
			return String();
	}
	else
	{
		ValueTree v;

		v = p->exportAsValueTree();

		const String c = ValueTreeHelpers::getBase64StringFromValueTree(v);

		if (copyToClipboard)
			SystemClipboard::copyTextToClipboard("\"" + c + "\"");

		return c;
	}

	
}

void ProcessorHelpers::restoreFromBase64String(Processor* p, const String& base64String, bool restoreScriptContentOnly/*=false*/)
{
	if (restoreScriptContentOnly)
	{
		if (auto pwsc = dynamic_cast<const ProcessorWithScriptingContent*>(p))
		{
			auto vt = ProcessorHelpers::ValueTreeHelpers::getValueTreeFromBase64String(base64String);

			if (auto content = pwsc->getScriptingContent())
				content->restoreAllControlsFromPreset(vt);
		}
	}
	else
	{
		ValueTree v = ValueTreeHelpers::getValueTreeFromBase64String(base64String);

		auto newId = v.getProperty("ID", String()).toString();

		auto oldId = p->getId();

		if (newId.isNotEmpty())
			p->setId(newId, dontSendNotification);

		
		

		p->restoreFromValueTree(v);

		p->setId(oldId);

#if USE_BACKEND
		if (auto firstChild = p->getChildProcessor(0))
		{
			
			auto update = [](Dispatchable* obj)
			{
				dynamic_cast<Processor*>(obj)->sendRebuildMessage(true);
				return Dispatchable::Status::OK;
			};

			p->getMainController()->getLockFreeDispatcher().callOnMessageThreadAfterSuspension(p, update);
		}
			
#endif
	}
	
}

void ProcessorHelpers::increaseBufferIfNeeded(AudioSampleBuffer& b, int numSamplesNeeded)
{
    if(numSamplesNeeded <= 0)
        return;
    
	// The channel amount must be set correctly in the constructor
	jassert(b.getNumChannels() > 0);

    if(numSamplesNeeded == b.getNumSamples())
        return;
    
    // On iOS AUv3, always shrink the buffer because of memory
    const bool shouldResize = HiseDeviceSimulator::isAUv3() ||
                              (b.getNumSamples() < numSamplesNeeded);
    
	if (shouldResize)
	{
		b.setSize(b.getNumChannels(), numSamplesNeeded, true, true, false);
		b.clear();
	}
}

void ProcessorHelpers::increaseBufferIfNeeded(hlac::HiseSampleBuffer& b, int numSamplesNeeded)
{
	// The channel amount must be set correctly in the constructor
	jassert(b.getNumChannels() > 0);

	if (b.getNumSamples() < numSamplesNeeded)
	{
		b.setSize(b.getNumChannels(), numSamplesNeeded);
		b.clear();
	}
}

bool Processor::isValidAndInitialised(bool checkOnAir) const
{
	const bool onAir_ = !checkOnAir || isOnAir();
	const bool isMainSynthChain = getMainController()->getMainSynthChain() == this;
	const bool hasParent = getParentProcessor(false) != nullptr;

	const bool isValid = (onAir_ && (isMainSynthChain || hasParent)) || getMainController()->isFlakyThreadingAllowed();

	// Normally you expect this method to be true, so this assertion will fire here
	// so that you can check the reason from the bools above...
	ASSERT_STRICT_PROCESSOR_STRUCTURE(isValid);
	return isValid;
}

StringArray ProcessorHelpers::getListOfAllConnectableProcessors(const Processor* processorToSkip)
{
	const Processor* mainSynthChain = processorToSkip->getMainController()->getMainSynthChain();

	Processor::Iterator<Processor> boxIter(mainSynthChain, false);

	Array<const Processor*> processorList;

	while (const Processor *p = boxIter.getNextProcessor())
	{
		if (p == processorToSkip)
			continue;

		if (is<Chain>(p) && !is<ModulatorSynth>(p)) continue;

		processorList.add(p);
	}

	StringArray processorIdList;

	processorIdList.add(" ");

	for (int i = 0; i < processorList.size(); i++)
	{
		processorIdList.add(processorList[i]->getId());
	}

	return processorIdList;
}

StringArray ProcessorHelpers::getListOfAllParametersForProcessor(Processor* p)
{
	StringArray parameterNames;

	parameterNames.add("Bypass");
	parameterNames.add("Enabled"); // just the opposite of Bypassed

	if (is<Modulator>(p))
		parameterNames.add("Intensity");

	if (p != nullptr)
	{
		for (int i = 0; i < p->getNumParameters(); i++)
			parameterNames.add(p->getIdentifierForParameterIndex(i).toString());
	}
	
	return parameterNames;
}

int ProcessorHelpers::getParameterIndexFromProcessor(Processor* p, const Identifier& id)
{
	static const Identifier intensityId("Intensity");
	static const Identifier bypassId("Bypass");
	static const Identifier enabled("Enabled");

	if (id == intensityId)
		return -2;

	if (id == bypassId)
		return -3;

	if (id == enabled)
		return -4;

	if (p != nullptr)
	{
		for (int i = 0; i < p->getNumParameters(); i++)
		{
			if (p->getIdentifierForParameterIndex(i) == id)
				return i;
		}
	}

	return -1;
}

void AudioSampleProcessor::setLoadedFile(const String &fileName, bool loadThisFile/*=false*/, bool forceReload/*=false*/)
{
	getBuffer().fromBase64String(fileName);

#if 0
	ignoreUnused(forceReload, loadThisFile);

	PoolReference newRef(dynamic_cast<Processor*>(this)->getMainController(), fileName, ProjectHandler::SubDirectories::AudioFiles);

    if(data.getRef() == newRef)
        return;
    
    
	if (!newRef.isValid())
	{
		if (currentPool != nullptr)
		{
			currentPool->removeListener(this);
			currentPool = nullptr;
		}
		
        data.clear();

		length = 0;
		sampleRateOfLoadedFile = -1.0;
		
		setRange(Range<int>(0, 0));

		// A AudioSampleProcessor must also be derived from Processor!
		jassert(dynamic_cast<Processor*>(this) != nullptr);

		dynamic_cast<Processor*>(this)->sendChangeMessage();

		loopRange = {};
		setUseLoop(false);

		newFileLoaded();
	}
    else
	{
		ScopedLock sl(getFileLock());

		currentPool = mc->getCurrentAudioSampleBufferPool();
		
		if (auto e = mc->getExpansionHandler().getExpansionForWildcardReference(newRef.getReferenceString()))
			currentPool = &e->pool->getAudioSampleBufferPool();

		data = currentPool->loadFromReference(newRef, PoolHelpers::LoadAndCacheWeak);
		currentPool->addListener(this);

		sampleRateOfLoadedFile = data.getAdditionalData().getProperty(MetadataIDs::SampleRate, 0.0);

		setRange(Range<int>(0, getTotalLength()));

		// A AudioSampleProcessor must also be derived from Processor!
		jassert(dynamic_cast<Processor*>(this) != nullptr);
		
		dynamic_cast<Processor*>(this)->sendChangeMessage();

		setLoopFromMetadata(data.getAdditionalData());

		newFileLoaded();
	}
#endif
};;


String ProcessorHelpers::ValueTreeHelpers::getBase64StringFromValueTree(const ValueTree& v)
{
	MemoryOutputStream internalMos;
	GZIPCompressorOutputStream gzos(&internalMos, 9, false);
	MemoryOutputStream mos;

	v.writeToStream(mos);

	gzos.write(mos.getData(), mos.getDataSize());
	gzos.flush();

	return internalMos.getMemoryBlock().toBase64Encoding();
}

ValueTree ProcessorHelpers::ValueTreeHelpers::getValueTreeFromBase64String(const String& base64State)
{
	MemoryBlock mb;

	if (mb.fromBase64Encoding(base64State))
		return ValueTree::readFromGZIPData(mb.getData(), mb.getSize());

	return {};
}

void ProcessorHelpers::connectTableEditor(TableEditor& t, Processor* p, int index)
{
	if(auto eh = dynamic_cast<snex::ExternalDataHolder*>(p))
	{
		t.setEditedTable(eh->getTable(index));
	}
	else
	{
		jassertfalse;
	}
}

hise::MarkdownHelpButton* ProcessorDocumentation::createHelpButtonForParameter(int index, Component* componentToAttachTo)
{
	if (index < parameters.size())
	{
		auto doc = parameters[index].createHelpText(2);

		auto b = new MarkdownHelpButton();
		b->setHelpText(doc);

		if (componentToAttachTo != nullptr)
		{
			b->attachTo(componentToAttachTo, MarkdownHelpButton::TopRight);
		}

		return b;
	}

	return nullptr;
}

hise::MarkdownHelpButton* ProcessorDocumentation::createHelpButton()
{
	String t;

	t << "# " << name << "\n";
	t << description << "\n";
	
	t << createHelpText();

	auto b = new MarkdownHelpButton();
	b->setHelpText<PathProvider<ChainBarPathFactory>>(t);

	return b;
}

juce::String ProcessorDocumentation::createHelpText()
{
	String t;

	

	if (parameterOffset < parameters.size())
	{
		t << "## Parameters \n";

		t << "| `#` | ID | Description |\n";
		t << "| - | --- | ----------- |\n";

		int p = 0;

		for (auto& e : parameters)
		{
			if (p++ < parameterOffset)
				continue;

			t << e.getMarkdownLine(false) << "\n";
		}
			
	}

	if (chainOffset < chains.size())
	{
		t << "## Chains \n";

		t << "| `#` | ID | Restriction | Description |\n";
		t << "| - | --- | ----- | ----------- |\n";

		int c = 0;

		for (auto& e : chains)
		{
			if (c++ < chainOffset)
				continue;

			t << e.getMarkdownLine(true) << "\n";
		}
			
	}

	return t;
}

void ProcessorDocumentation::fillMissingParameters(Processor* p)
{
	for (int i = parameterOffset; i < p->getNumParameters(); i++)
	{
		auto id = p->getIdentifierForParameterIndex(i);

		Entry e;
		e.id = id;
		e.helpText = p->getDescriptionForParameters(i);
		e.index = i;
		e.name = id.toString();

		if (!parameters.contains(e))
			parameters.add(e);
	}

	for (int i = chainOffset; i < p->getNumInternalChains(); i++)
	{
		Entry e;

		e.name = p->getChildProcessor(i)->getId();
		e.helpText = "-";
		e.index = i;
		
		auto c = dynamic_cast<Chain*>(p->getChildProcessor(i));
		auto constrainer = c->getFactoryType()->getConstrainer();

		if (constrainer == nullptr)
			e.constrainer = "All types";
		else
			e.constrainer = constrainer->getDescription();

		if (!chains.contains(e))
			chains.add(e);
	}

    Entry::Sorter s;
	parameters.sort(s);
}

int ProcessorDocumentation::Entry::Sorter::compareElements(Entry& first, Entry& second)
{
	if (first.index > second.index)
		return 1;
	else if (first.index < second.index)
		return -1;
	else
		return 0;
}

bool ProcessorDocumentation::Entry::operator==(const Entry& other) const
{ return id == other.id; }

ProcessorDocumentation::~ProcessorDocumentation()
{}

int ProcessorDocumentation::getNumAttributes() const
{ return parameters.size(); }

Identifier ProcessorDocumentation::getAttributeId(int index) const
{ return parameters[index].id; }

void ProcessorDocumentation::setOffset(int pOffset, int cOffset)
{
	parameterOffset = pOffset;
	chainOffset = cOffset;
}

ProcessorDocumentation::ProcessorDocumentation()
{}

void ProcessorDocumentation::addParameter(Entry newParameter)
{
	parameters.add(newParameter);
}

void ProcessorDocumentation::addChain(Entry newChain)
{
	chains.add(newChain);
}

void ProcessorDocumentation::addLine(const String& l)
{
	description << l << "\n";
}

void ProcessorDocumentation::setName(const String& name_)
{
	name = name_;
}

juce::String ProcessorDocumentation::Entry::getMarkdownLine(bool getChain) const
{
	String s;

	s << "| " << String(index) << " | ";
	
	if (getChain)
	{
		s << name << " | " << constrainer << " |";
	}
	else
	{
		s << "`" << id.toString() << "`";
	}
	
	s << " | " << helpText << " |";
	return s;
}

juce::String ProcessorDocumentation::Entry::createHelpText(int headLineLevel) const
{
	String s;

	switch (headLineLevel)
	{
	case 1: s << "# "; break;
	case 2: s << "## "; break;
	case 3: s << "### "; break;
	default:
		break;
	}

	s << " " << name << "\n";
	s << "Scripting ID: `" << id.toString() << "`  \n";
	s << "  \n";
	s << helpText;
	return s;
}

} // namespace hise
