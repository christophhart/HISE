/*
  ==============================================================================

    HardcodedScriptProcessor.cpp
    Created: 21 Apr 2015 8:11:59pm
    Author:  Christoph

  ==============================================================================
*/


HardcodedScriptProcessor::HardcodedScriptProcessor(MainController *mc, const String &id, ModulatorSynth *ms):
	ScriptBaseProcessor(mc, id),
	Message(this),
	Synth(this, ms),
	Engine(this),
	Console(this),
	Content(this)
{
	
	content = &Content;

	jassert(ms != nullptr);

	allowObjectConstructors = true;
	
	onInit();

	allowObjectConstructors = false;

};

ProcessorEditorBody *HardcodedScriptProcessor::createEditor(BetterProcessorEditor *parentEditor)
{
#if USE_BACKEND

	return new HardcodedScriptEditor(parentEditor);

	
#else

	jassertfalse;

	return nullptr;

#endif
};

void HardcodedScriptProcessor::processMidiMessage (MidiMessage &m)
{
	
	currentMessage = m;
	Message.setMidiMessage(&m);
	Message.ignoreEvent(false);

	if(m.isNoteOn())
	{
		Synth.increaseNoteCounter();
		onNoteOn();
		
	}
	else if (m.isNoteOff())
	{
		Synth.decreaseNoteCounter();
		onNoteOff();
	}
	else if (m.isController())
	{
		onController();
	}

	processThisMessage = !Message.ignored;
	

}

void HardcodedScriptFactoryType::fillTypeNameList()
{
	ADD_NAME_TO_TYPELIST(LegatoProcessor);
	ADD_NAME_TO_TYPELIST(CCSwapper);
	ADD_NAME_TO_TYPELIST(LegatoIntervallPlayer);
	ADD_NAME_TO_TYPELIST(ReleaseTriggerScriptProcessor);
	ADD_NAME_TO_TYPELIST(CCToNoteProcessor);
	ADD_NAME_TO_TYPELIST(ChannelFilterScriptProcessor);
	ADD_NAME_TO_TYPELIST(ChannelSetterScriptProcessor);
	ADD_NAME_TO_TYPELIST(MuteAllScriptProcessor);
    ADD_NAME_TO_TYPELIST(LiveCodingNotePlayer);
}

Processor *HardcodedScriptFactoryType::createProcessor(int typeIndex, const String &id) 
{
	MainController *m = getOwnerProcessor()->getMainController();
	ModulatorSynth *owner = dynamic_cast<ModulatorSynth*>(getOwnerProcessor());
	MidiProcessor *mp = nullptr;

	switch(typeIndex)
	{
	case legatoWithRetrigger:	mp = new LegatoProcessor(m, id, owner); break;
	case ccSwapper:				mp = new CCSwapper(m, id, owner); break;
	case legatoIntervalPlayer:	mp = new LegatoIntervallPlayer(m, id, owner); break;
	case releaseTrigger:		mp = new ReleaseTriggerScriptProcessor(m, id, owner); break;
	case cc2Note:				mp = new CCToNoteProcessor(m, id, owner); break;
	case channelFilter:			mp = new ChannelFilterScriptProcessor(m, id, owner); break;
	case channelSetter:			mp = new ChannelSetterScriptProcessor(m, id, owner); break;
	case muteAll:				mp = new MuteAllScriptProcessor(m, id, owner); break;
    case liveCodingNotePlayer:  mp = new LiveCodingNotePlayer(m, id, owner); break;
	default:					jassertfalse; break;
	}

	if(mp != nullptr)
	{
		mp->setOwnerSynth(owner);
	}

	return mp;
}

