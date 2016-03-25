
ProcessorEditorBody *StereoEffect::createEditor(BetterProcessorEditor *parentEditor)
{
#if USE_BACKEND

	return new StereoEditor(parentEditor);

	
#else

	jassertfalse;

	return nullptr;

#endif
}

