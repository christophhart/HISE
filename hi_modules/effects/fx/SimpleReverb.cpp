
ProcessorEditorBody *SimpleReverbEffect::createEditor(BetterProcessorEditor *parentEditor)
{
#if USE_BACKEND

	return new ReverbEditor(parentEditor);

#else 

	jassertfalse;
	return nullptr;

#endif
}

