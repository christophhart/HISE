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

MidiProcessor::MidiProcessor(MainController *mc, const String &id):
		Processor(mc, id, 1),
		processThisMessage(true),
		ownerSynth(nullptr),
		numThisTime(0)
	{

		
		

	};

MidiProcessor::~MidiProcessor()
{
	ownerSynth = nullptr;
	masterReference.clear();
};


bool MidiProcessor::setArtificialTimestamp(uint16 eventId, int newTimestamp)
{
	return ownerSynth->midiProcessorChain->setArtificialTimestamp(eventId, newTimestamp);
}

void MidiProcessor::addHiseEventToBuffer(const HiseEvent &m)
{
	ownerSynth->midiProcessorChain->addArtificialEvent(m);

	
}

ProcessorEditorBody *MidiProcessor::createEditor(ProcessorEditor *parentEditor)
{
#if USE_BACKEND

	return new EmptyProcessorEditorBody(parentEditor);

#else 

	ignoreUnused(parentEditor);
	jassertfalse;
	return nullptr;

#endif
};

ProcessorEditorBody *MidiProcessorChain::createEditor(ProcessorEditor *parentEditor)
{
#if USE_BACKEND

	return new EmptyProcessorEditorBody(parentEditor);

#else 

	ignoreUnused(parentEditor);
	jassertfalse;
	return nullptr;

#endif
};



void MidiProcessorChain::addArtificialEvent(const HiseEvent& m)
{
	artificialEvents.addEvent(m);
}

bool MidiProcessorChain::setArtificialTimestamp(uint16 eventId, int newTimestamp)
{
	for (auto& e : artificialEvents)
	{
		if (e.getEventId() == eventId)
		{
			e.setTimeStamp(newTimestamp);
			return true;
		}
	}

	for (auto& e : futureEventBuffer)
	{
		if (e.getEventId() == eventId)
		{
			e.setTimeStamp(newTimestamp);
			return true;
		}
	}
}

void MidiProcessorChain::renderNextHiseEventBuffer(HiseEventBuffer &buffer, int numSamples)
{
	if (allNotesOffAtNextBuffer)
	{
		buffer.clear();
		buffer.addEvent(HiseEvent(HiseEvent::Type::AllNotesOff, 0, 0, 1));
		allNotesOffAtNextBuffer = false;
	}

	if (!wholeBufferProcessors.isEmpty())
	{
		for (auto wmp : wholeBufferProcessors)
		{
			wmp->preprocessBuffer(buffer, numSamples);
			buffer.alignEventsToRaster<HISE_EVENT_RASTER>(numSamples);
		}
			

		
	}

	if (buffer.isEmpty() && futureEventBuffer.isEmpty() && artificialEvents.isEmpty()) return;

	HiseEventBuffer::Iterator it(buffer);
	
	jassert(buffer.timeStampsAreSorted());

	while (HiseEvent* e = it.getNextEventPointer(true, false))
	{
		processHiseEvent(*e);
	}

	buffer.sortTimestamps();
	artificialEvents.sortTimestamps();

	jassert(buffer.timeStampsAreSorted());

	artificialEvents.moveEventsBelow(buffer, numSamples);
	buffer.moveEventsAbove(artificialEvents, numSamples);
	artificialEvents.subtractFromTimeStamps(numSamples);
}

MidiProcessorFactoryType::MidiProcessorFactoryType(Processor *p) :
		FactoryType(p),
		hardcodedScripts(new HardcodedScriptFactoryType(p))
{
	ADD_NAME_TO_TYPELIST(JavascriptMidiProcessor);
	ADD_NAME_TO_TYPELIST(Transposer);
	ADD_NAME_TO_TYPELIST(MidiPlayer);

	typeNames.addArray(hardcodedScripts->getAllowedTypes());
};

int MidiProcessorFactoryType::fillPopupMenu(PopupMenu &m, int startIndex)
{
	Array<ProcessorEntry> types = getAllowedTypes();

	int index = startIndex;

	for(int i = 0; i < numMidiProcessors; i++)
	{
		m.addItem(i+startIndex, types[i].name);

		index++;

	}

	PopupMenu hardcodedScriptMenu;

	index = hardcodedScripts->fillPopupMenu(hardcodedScriptMenu, numMidiProcessors + startIndex);

	m.addSubMenu("Hardcoded Scripts", hardcodedScriptMenu);

	return index;
}


Processor *MidiProcessorFactoryType::createProcessor(int typeIndex, const String &id) 
{
	MainController *m = getOwnerProcessor()->getMainController();

	MidiProcessor *mp = nullptr;

	if(typeIndex >= numMidiProcessors)
	{
		mp = dynamic_cast<MidiProcessor*>(hardcodedScripts->createProcessor(typeIndex, id));
	}
	else
	{
		auto ms = dynamic_cast<ModulatorSynth*>(getOwnerProcessor());

		switch(typeIndex)
		{
			case scriptProcessor:		mp = new JavascriptMidiProcessor(m, id); break;
			case transposer:			mp = new Transposer(m, id); break;
			case midiFilePlayer:		mp = new MidiPlayer(m, id, ms); break;
			default:					jassertfalse; return nullptr;
		}

		mp->setOwnerSynth(ms);
	}

	return mp;
};

MidiProcessorChain::MidiProcessorChain(MainController *mc, const String &id, Processor *ownerProcessor):
		MidiProcessor(mc, id),
		parentProcessor(ownerProcessor),
		midiProcessorFactory(new MidiProcessorFactoryType(ownerProcessor)),
		allNotesOffAtNextBuffer(false),
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

void MidiProcessorChain::MidiProcessorChainHandler::add(Processor *newProcessor, Processor *siblingToInsertBefore)
{
	{
		MidiProcessor *m = dynamic_cast<MidiProcessor*>(newProcessor);

		jassert(m != nullptr);

		const int index = siblingToInsertBefore == nullptr ? -1 : chain->processors.indexOf(dynamic_cast<MidiProcessor*>(siblingToInsertBefore));

		newProcessor->prepareToPlay(chain->getSampleRate(), chain->getLargestBlockSize());
		newProcessor->setParentProcessor(chain);

		{
			LOCK_PROCESSING_CHAIN(chain);

			newProcessor->setIsOnAir(chain->isOnAir());
			chain->processors.insert(index, m);

			if (m->isProcessingWholeBuffer())
				chain->addWholeBufferProcessor(m);
		}

		if (JavascriptMidiProcessor* sp = dynamic_cast<JavascriptMidiProcessor*>(newProcessor))
		{
			sp->compileScript();
		}
	}
	
	notifyListeners(Listener::ProcessorAdded, newProcessor);
}

} // namespace hise