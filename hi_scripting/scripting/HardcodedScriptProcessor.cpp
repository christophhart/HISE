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
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

namespace hise { using namespace juce;

HardcodedScriptProcessor::HardcodedScriptProcessor(MainController *mc, const String &id, ModulatorSynth *ms):
	ScriptBaseMidiProcessor(mc, id),
	ProcessorWithDynamicExternalData(mc),
	Message(this),
	Synth(this, &Message, ms),
	Engine(this),
	Console(this),
	refCountedContent(new ScriptingApi::Content(this)),
	Content(*refCountedContent.get()),
	Sampler(this, dynamic_cast<ModulatorSampler*>(ms))
{
	
	content = refCountedContent;

	jassert(ms != nullptr);

	allowObjectConstructors = true;
	
	onInit();

	allowObjectConstructors = false;

};

HardcodedScriptProcessor::~HardcodedScriptProcessor()
{
	refCountedContent = nullptr;
	content = nullptr;
}

void HardcodedScriptProcessor::restoreFromValueTree(const ValueTree& v)
{
	jassert(content.get() != nullptr);
        
	MidiProcessor::restoreFromValueTree(v);
        
	onInit();

	ScriptBaseMidiProcessor::restoreContent(v);

	if(content.get() != nullptr)
	{
		for(int i = 0; i < content->getNumComponents(); i++)
		{
			controlCallback(content->getComponent(i), content->getComponent(i)->getValue());
		}
	}
}

void HardcodedScriptProcessor::controlCallback(ScriptingApi::Content::ScriptComponent* component, var controllerValue)
{
	try
	{
		onControl(component, controllerValue);
	}
	catch (String& s)
	{
		debugToConsole(this, s);
	}
}

ProcessorEditorBody *HardcodedScriptProcessor::createEditor(ProcessorEditor *parentEditor)
{
#if USE_BACKEND

	return new HardcodedScriptEditor(parentEditor);

#else

	ignoreUnused(parentEditor);
	jassertfalse;

	return nullptr;

#endif
};

void HardcodedScriptProcessor::processHiseEvent(HiseEvent &m)
{
	try
	{
		currentEvent = &m;

		Message.setHiseEvent(m);
		Message.ignoreEvent(false);
		Synth.handleNoteCounter(m);

		switch (m.getType())
		{
		case HiseEvent::Type::NoteOn:
			onNoteOn();
			break;
		case HiseEvent::Type::NoteOff:
			onNoteOff();
			break;
		case HiseEvent::Type::Controller:
		case HiseEvent::Type::PitchBend:
		case HiseEvent::Type::Aftertouch: 
			onController();
			break;
		case HiseEvent::Type::TimerEvent:
		{
			if (m.getTimerIndex() == getIndexInChain())
			{
				onTimer(m.getTimeStamp());
				m.ignoreEvent(true);
			}

			break;
		}
		case HiseEvent::Type::AllNotesOff:
		{
			onAllNotesOff();
			break;
		}
		case HiseEvent::Type::Empty:
		case HiseEvent::Type::SongPosition:
		case HiseEvent::Type::MidiStart:
		case HiseEvent::Type::MidiStop:
		case HiseEvent::Type::VolumeFade:
		case HiseEvent::Type::PitchFade:
		case HiseEvent::Type::numTypes:
        case HiseEvent::Type::ProgramChange:
			break;
		}
	}
	catch (String& error)
	{
#if USE_BACKEND
		debugError(this, error);
		
#else
		DBG(error);
		ignoreUnused(error);
#endif
	}
	
}

HardcodedScriptFactoryType::HardcodedScriptFactoryType(Processor* p):
	FactoryType(p)
{
	fillTypeNameList();
}

void HardcodedScriptFactoryType::fillTypeNameList()
{
	ADD_NAME_TO_TYPELIST(LegatoProcessor);
	ADD_NAME_TO_TYPELIST(CCSwapper);
	ADD_NAME_TO_TYPELIST(ReleaseTriggerScriptProcessor);
	ADD_NAME_TO_TYPELIST(CCToNoteProcessor);
	ADD_NAME_TO_TYPELIST(ChannelFilterScriptProcessor);
	ADD_NAME_TO_TYPELIST(ChannelSetterScriptProcessor);
	ADD_NAME_TO_TYPELIST(MuteAllScriptProcessor);
	ADD_NAME_TO_TYPELIST(Arpeggiator);
}

HardcodedScriptFactoryType::~HardcodedScriptFactoryType()
{
	typeNames.clear();
}

Processor *HardcodedScriptFactoryType::createProcessor(int typeIndex, const String &id) 
{
	MainController *m = getOwnerProcessor()->getMainController();
	ModulatorSynth *ownerAsSynth = dynamic_cast<ModulatorSynth*>(getOwnerProcessor());
	MidiProcessor *mp = nullptr;

	switch(typeIndex)
	{
	case legatoWithRetrigger:	mp = new LegatoProcessor(m, id, ownerAsSynth); break;
	case ccSwapper:				mp = new CCSwapper(m, id, ownerAsSynth); break;
	case releaseTrigger:		mp = new ReleaseTriggerScriptProcessor(m, id, ownerAsSynth); break;
	case cc2Note:				mp = new CCToNoteProcessor(m, id, ownerAsSynth); break;
	case channelFilter:			mp = new ChannelFilterScriptProcessor(m, id, ownerAsSynth); break;
	case channelSetter:			mp = new ChannelSetterScriptProcessor(m, id, ownerAsSynth); break;
	case muteAll:				mp = new MuteAllScriptProcessor(m, id, ownerAsSynth); break;
	case arpeggiator:			mp = new Arpeggiator(m, id, ownerAsSynth); break;
	default:					jassertfalse; break;
	}

	if(mp != nullptr)
	{
		mp->setOwnerSynth(ownerAsSynth);
	}

	return mp;
}

const Array<FactoryType::ProcessorEntry>& HardcodedScriptFactoryType::getTypeNames() const
{
	return typeNames;
}
} // namespace hise