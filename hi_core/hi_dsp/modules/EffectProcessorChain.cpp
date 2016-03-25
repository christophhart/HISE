EffectProcessorChain::EffectProcessorChain(Processor *parentProcessor_, const String &id, int numVoices):
		VoiceEffectProcessor(parentProcessor_->getMainController(), id, numVoices),
		handler(this),
		parentProcessor(parentProcessor_)
{
	effectChainFactory = new EffectProcessorChainFactoryType(numVoices, parentProcessor);

	setEditorState(Processor::Visible, false, dontSendNotification);
}

ProcessorEditorBody *::EffectProcessorChain::createEditor(BetterProcessorEditor *parentEditor)
{
#if USE_BACKEND

	return new EmptyProcessorEditorBody(parentEditor);
	
#else 

	jassertfalse;
	return nullptr;

#endif
}


void EffectProcessorChainFactoryType::fillTypeNameList()
{
	ADD_NAME_TO_TYPELIST(MonoFilterEffect);
	ADD_NAME_TO_TYPELIST(PolyFilterEffect);
	ADD_NAME_TO_TYPELIST(HarmonicFilter);
	ADD_NAME_TO_TYPELIST(CurveEq);
	ADD_NAME_TO_TYPELIST(StereoEffect);
	ADD_NAME_TO_TYPELIST(SimpleReverbEffect);
	ADD_NAME_TO_TYPELIST(GainEffect);
	ADD_NAME_TO_TYPELIST(ConvolutionEffect);
	ADD_NAME_TO_TYPELIST(DelayEffect);
	ADD_NAME_TO_TYPELIST(MdaLimiterEffect);
	ADD_NAME_TO_TYPELIST(MdaDegradeEffect);
	ADD_NAME_TO_TYPELIST(ChorusEffect);
	ADD_NAME_TO_TYPELIST(GainCollector);
	ADD_NAME_TO_TYPELIST(RouteEffect);
	ADD_NAME_TO_TYPELIST(SaturatorEffect);

#if INCLUDE_PROTOPLUG
	ADD_NAME_TO_TYPELIST(ProtoplugEffectProcessor);
#endif
};

Processor* EffectProcessorChainFactoryType::createProcessor	(int typeIndex, const String &id)
{
	MainController *m = getOwnerProcessor()->getMainController();

	switch(typeIndex)
	{
	case monophonicFilter:				return new MonoFilterEffect(m, id);
	case polyphonicFilter:				return new PolyFilterEffect(m, id, numVoices);
	case harmonicFilter:				return new HarmonicFilter(m, id, numVoices);
	case curveEq:						return new CurveEq(m, id);
	case stereoEffect:					return new StereoEffect(m, id, numVoices);
	case convolution:					return new ConvolutionEffect(m, id);
	case simpleReverb:					return new SimpleReverbEffect(m, id);
	case simpleGain:					return new GainEffect(m, id);
	case delay:							return new DelayEffect(m, id);
	case limiter:						return new MdaLimiterEffect(m, id);
	case degrade:						return new MdaDegradeEffect(m, id);
	case chorus:						return new ChorusEffect(m, id);
	case gainCollector:					return new GainCollector(m, id);
	case routeFX:						return new RouteEffect(m, id);
	case saturation:					return new SaturatorEffect(m, id);
#if INCLUDE_PROTOPLUG
	case protoPlugEffect:				return new ProtoplugEffectProcessor(m, id);
#endif
	default:					jassertfalse; return nullptr;
	}
};

