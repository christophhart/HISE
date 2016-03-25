ProcessorEditorBody* NoiseSynth::createEditor(BetterProcessorEditor *parentEditor)
{
#if USE_BACKEND

	return new EmptyProcessorEditorBody(parentEditor);

#else 

	jassertfalse;
	return nullptr;

#endif
}