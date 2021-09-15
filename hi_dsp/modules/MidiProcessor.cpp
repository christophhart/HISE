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


struct MidiProcessor::EventLogger
{
    EventLogger():
      queue(1024)
    {};
    
    struct Display: public Component,
                    public PooledUIUpdater::SimpleTimer
    {
        static constexpr int RowHeight = 24;
        
        enum class Columns
        {
            Type,
            Ignored,
            Artificial,
            Number,
            Channel,
            Value,
            Timestamp,
            EventId,
            
            numColumns
        };
        
        Display(MidiProcessor* mp_, EventLogger* l):
          SimpleTimer(mp_->getMainController()->getGlobalUIUpdater()),
          resizer(this, nullptr),
          mp(mp_),
          logger(l)
        {
            addAndMakeVisible(resizer);
            start();
            setSize(400, 400);
            setName("Event Logger: " + mp->getId());
        }
        
        void drawEventColumn(Graphics& g, const HiseEvent& e, Columns c, Rectangle<float> area)
        {
            g.setFont(GLOBAL_MONOSPACE_FONT());
            g.setColour(Colours::black.withAlpha(0.05f));
            g.fillRect(area.reduced(0.5f));
            g.setColour(Colours::white);
            
            auto draw = [&](int v)
            {
                g.drawText(String(v), area, Justification::centred);
            };
            
            switch(c)
            {
                case Columns::Type:
                {
                    if(e.isAllNotesOff())
                    {
                        g.setColour(Colours::red.withSaturation(0.6f));
                        g.drawText("!", area, Justification::centred);
                    }
                    if(e.isNoteOnOrOff())
                    {
                        Path p;
                        p.startNewSubPath(0.0f, 0.0f);
                        p.lineTo(1.0f, 0.0f);
                        p.lineTo(0.5f, 1.0f);
                        p.closeSubPath();
                        
                        Colour c[8];
                        
                        c[0] = Colours::red;
                        c[1] = Colours::violet;
                        c[2] = Colours::brown;
                        c[3] = Colours::green;
                        c[4] = Colours::pink;
                        c[5] = Colours::yellow;
                        c[6] = Colours::orange;
                        c[7] = Colours::blue;
                        
                        auto randomColour = c[e.getEventId() % 8];
                        g.setColour(randomColour.withAlpha(0.7f).withSaturation(0.5f));
                        
                        if(e.isNoteOff())
                            p.applyTransform(AffineTransform::rotation(float_Pi));
                        
                        PathFactory::scalePath(p, area.reduced(6.0f));
                        
                        
                        
                        g.fillPath(p);
                    }
                }
                case Columns::Ignored: if(e.isIgnored()) g.fillEllipse(area.reduced(9)); break;
                case Columns::Artificial: if(e.isArtificial()) g.fillEllipse(area.reduced(9)); break;
                case Columns::Number: draw(e.getNoteNumber()); break;
                case Columns::Channel: draw(e.getChannel()); break;
                case Columns::Value: draw(e.getVelocity()); break;
                case Columns::Timestamp: draw(e.getTimeStamp()); break;
                case Columns::EventId: draw(e.getEventId()); break;
                default: return;
            }
        }
        
        void drawColumnHeader(Graphics& g, Columns c, Rectangle<float> area)
        {
            g.setFont(GLOBAL_BOLD_FONT());
            g.setColour(Colours::black.withAlpha(0.15f));
            g.fillRect(area.reduced(0.5f));
            g.setColour(Colours::white);
            
            switch(c)
            {
                case Columns::Type: g.drawText("T", area, Justification::centred); break;
                case Columns::Ignored: g.drawText("I", area, Justification::centred); break;
                case Columns::Artificial: g.drawText("A", area, Justification::centred); break;
                case Columns::Number: g.drawText("Number", area, Justification::centred); break;
                case Columns::Channel: g.drawText("Channel", area, Justification::centred); break;
                case Columns::Value: g.drawText("Value", area, Justification::centred); break;
                case Columns::Timestamp: g.drawText("Timestamp", area, Justification::centred); break;
                case Columns::EventId: g.drawText("Event ID", area, Justification::centred); break;
                default: return;
            }
        }
        
        int getColumnWidth(Columns c)
        {
            switch(c)
            {
                case Columns::Type: return RowHeight;
                case Columns::Ignored: return RowHeight;
                case Columns::Artificial: return RowHeight;
                case Columns::Number: return (getWidth() - RowHeight * 3) * 0.2;
                case Columns::Channel: return (getWidth() - RowHeight * 3) * 0.15;
                case Columns::Value: return (getWidth() - RowHeight * 3) * 0.15;
                case Columns::Timestamp: return (getWidth() - RowHeight * 3) * 0.25;
                case Columns::EventId: return (getWidth() - RowHeight * 3) * 0.25;
                default: return 0;
            }
        }
        
        void drawEvent(Graphics& g, const HiseEvent& e, Rectangle<float> area)
        {
            g.setFont(GLOBAL_MONOSPACE_FONT());
            g.setColour(Colours::white.withAlpha(0.4f));
            g.drawText(e.toDebugString(), area, Justification::centred);
        }
        
        void paint(Graphics& g) override
        {
            auto b = getLocalBounds();
            
            auto top = b.removeFromTop(30);
            
            for(int i = 0; i < int(Columns::numColumns); i++)
            {
                auto h = top.removeFromLeft(getColumnWidth((Columns)i));
                drawColumnHeader(g, (Columns)i, h.toFloat());
            }
            
            for(auto e: events)
            {
                auto a = b.removeFromTop(RowHeight);
             
                for(int i = 0; i < int(Columns::numColumns); i++)
                {
                    auto h = a.removeFromLeft(getColumnWidth((Columns)i));
                    drawEventColumn(g, e, (Columns)i, h.toFloat());
                }
            }
        }
        
        ~Display()
        {
            mp->setEnableEventLogger(false);
        }
        
        void timerCallback() override
        {
            if(logger != nullptr)
            {
                logger->queue.callForEveryElementInQueue([&](const HiseEvent& e)
                {
                    events.add(e);
                    return true;
                });
                
                int numToDisplay = getHeight() / RowHeight;
                
                if(events.size() > numToDisplay)
                {
                    int numToDelete = events.size() - numToDisplay;
                    events.removeRange(0, numToDelete);
                }
                
                repaint();
            }
        }
        
        void resized() override
        {
            resizer.setBounds(getLocalBounds().removeFromRight(15).removeFromBottom(15));
        }
        
        WeakReference<EventLogger> logger;
        juce::ResizableCornerComponent resizer;
        WeakReference<MidiProcessor> mp;
        
        Array<HiseEvent> events;
    };
    
    hise::LockfreeQueue<HiseEvent> queue;
    
    JUCE_DECLARE_WEAK_REFERENCEABLE(EventLogger);
};

void MidiProcessor::logIfEnabled(const HiseEvent& e)
{
#if USE_BACKEND
    
    SimpleReadWriteLock::ScopedReadLock sl(eventLock);
    
    if(eventLogger != nullptr)
    {
        eventLogger->queue.push(e);
    }
    
#endif
}

void MidiProcessor::setEnableEventLogger(bool shouldBeEnabled)
{
    SimpleReadWriteLock::ScopedWriteLock sl(eventLock);
    
    bool isLoggingEvents = eventLogger != nullptr;
    
    if(isLoggingEvents != shouldBeEnabled)
    {
        if(!shouldBeEnabled)
        {
            eventLogger = nullptr;
        }
        else
        {
            eventLogger = new EventLogger();
        }
    }
}


Component* MidiProcessor::createEventLogComponent()
{
    setEnableEventLogger(true);
    return new EventLogger::Display(this, eventLogger);
}

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

	return false;
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

    
    
	while (HiseEvent* e = it.getNextEventPointer(false, false))
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
