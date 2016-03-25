/*
  ==============================================================================

    MidiProcessor.cpp
    Created: 5 Jul 2014 12:09:20pm
    Author:  Chrisboy

  ==============================================================================
*/

MidiProcessor::MidiProcessor(MainController *mc, const String &id):
		Processor(mc, id),
		processThisMessage(true),
		allNotesOffAtNextBuffer(false),
		ownerSynth(nullptr),
		numThisTime(0)
	{

		outputBuffer.ensureSize(1024);
		futureBuffer.ensureSize(1024);
		nextFutureBuffer.ensureSize(1024);

		

	};

MidiProcessor::~MidiProcessor()
{
	ownerSynth = nullptr;
	masterReference.clear();
};


void MidiProcessor::addMidiMessageToBuffer(MidiMessage &m)
{
	const int timeStamp = (int)m.getTimeStamp();

	if (timeStamp > numThisTime)
	{
		ownerSynth->midiProcessorChain->futureBuffer.addEvent(m, timeStamp);
	}
	else
	{
		ownerSynth->generatedMessages.addEvent(m, timeStamp);
	}

#if 0 // adding something to the output buffer will be erased. Check for bugs!
	if (m.getTimeStamp() > numThisTime)
	{
		ownerSynth->midiProcessorChain->futureBuffer.addEvent(m, (int)m.getTimeStamp());
	}
	else
	{
		ownerSynth->midiProcessorChain->outputBuffer.addEvent(m, (int)m.getTimeStamp());
	}
#endif
}

ProcessorEditorBody *MidiProcessor::createEditor(BetterProcessorEditor *parentEditor)
{
#if USE_BACKEND

	return new EmptyProcessorEditorBody(parentEditor);

#else 

	jassertfalse;
	return nullptr;

#endif
};

ProcessorEditorBody *MidiProcessorChain::createEditor(BetterProcessorEditor *parentEditor)
{
#if USE_BACKEND

	return new EmptyProcessorEditorBody(parentEditor);

#else 

	jassertfalse;
	return nullptr;

#endif
};



MidiProcessorFactoryType::MidiProcessorFactoryType(Processor *p):
		FactoryType(p),
		hardcodedScripts(new HardcodedScriptFactoryType(p))
{
	ADD_NAME_TO_TYPELIST(MidiDelay);
	ADD_NAME_TO_TYPELIST(SampleRaster);
	ADD_NAME_TO_TYPELIST(ScriptProcessor);
	ADD_NAME_TO_TYPELIST(Transposer);
	ADD_NAME_TO_TYPELIST(RoundRobinMidiProcessor);

	typeNames.addArray(hardcodedScripts->getAllowedTypes());
};

int MidiProcessorFactoryType::fillPopupMenu(PopupMenu &m, int startIndex)
{
	Array<ProcessorEntry> types = getAllowedTypes();

	int index = startIndex;

	for(int i = 0; i < midiProcessorChain; i++)
	{
		m.addItem(i+startIndex, types[i].name);

		index++;

	}

	PopupMenu hardcodedScriptMenu;

	index = hardcodedScripts->fillPopupMenu(hardcodedScriptMenu, midiProcessorChain);

	m.addSubMenu("Hardcoded Scripts", hardcodedScriptMenu);

	return index;
}


Processor *MidiProcessorFactoryType::createProcessor(int typeIndex, const String &id) 
{
	MainController *m = getOwnerProcessor()->getMainController();

	MidiProcessor *mp = nullptr;

	if(typeIndex >= midiProcessorChain)
	{
		mp = dynamic_cast<MidiProcessor*>(hardcodedScripts->createProcessor(typeIndex, id));
	}
	else
	{
		switch(typeIndex)
		{
			case midiDelay:				mp = new MidiDelay(m, id); break;
			case sampleRaster:			mp = new SampleRaster(m, id); break;
			case scriptProcessor:		mp = new ScriptProcessor(m, id); break;
			case transposer:			mp = new Transposer(m, id); break;
			case roundRobin:			return nullptr;
			case midiProcessorChain:	jassertfalse; mp = new MidiProcessorChain(m, id, getOwnerProcessor()); break;
			default:					jassertfalse; return nullptr;
		}

		mp->setOwnerSynth(dynamic_cast<ModulatorSynth*>(getOwnerProcessor()));

	}

	

	return mp;
};

MidiProcessorChain::MidiProcessorChain(MainController *mc, const String &id, Processor *ownerProcessor):
		MidiProcessor(mc, id),
		parentProcessor(ownerProcessor),
		midiProcessorFactory(new MidiProcessorFactoryType(ownerProcessor)),
		handler(this)
{
	setOwnerSynth(dynamic_cast<ModulatorSynth*>(ownerProcessor));

	setFactoryType(new MidiProcessorFactoryType(ownerProcessor));

	setEditorState(Processor::Visible, false, dontSendNotification);
};



bool MidiProcessorFactoryType::allowType(const Identifier &typeName) const
{
	if (! FactoryType::allowType(typeName) ) return false;

	if(typeName == RoundRobinMidiProcessor::getClassType())
	{
		const bool isChildSynthOfGroup = dynamic_cast<const ModulatorSynthGroup*>(getOwnerProcessor()) != nullptr;// && owner->getGroup() != nullptr;

		if (!isChildSynthOfGroup) return false;

		const MidiProcessorChain* c = dynamic_cast<const MidiProcessorChain*>(owner->getChildProcessor(ModulatorSynth::MidiProcessor));
		jassert(c != nullptr);

		for(int i = 0; i < c->getHandler()->getNumProcessors(); i++)
		{
			if(c->getHandler()->getProcessor(i)->getType() == RoundRobinMidiProcessor::getClassType()) return false;
		}
	}
	
	return true;

}
