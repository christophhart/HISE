

RouteEffect::RouteEffect(MainController *mc, const String &uid) :
MasterEffectProcessor(mc, uid)
{
	getMatrix().setOnlyEnablingAllowed(false);
}

ProcessorEditorBody *RouteEffect::createEditor(BetterProcessorEditor *parentEditor)
{
#if USE_BACKEND

	return new RouteFXEditor(parentEditor);

#else 

	jassertfalse;
	return nullptr;

#endif
}

void RouteEffect::renderNextBlock(AudioSampleBuffer &b, int /*startSample*/, int numSamples)
{
	for (int i = 0; i < b.getNumChannels(); i++)
	{
		const int j = getMatrix().getSendForSourceChannel(i);

		if (j != -1)
		{
			FloatVectorOperations::add(b.getWritePointer(j), b.getReadPointer(i), numSamples);
		}
	}
}

void RouteEffect::applyEffect(AudioSampleBuffer &, int, int /*numSamples*/)
{
	
}

