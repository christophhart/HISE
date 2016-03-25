
float WaveSynthVoice::sinTable[2048];

Random WaveSynthVoice::noiseGenerator = Random();

ProcessorEditorBody* WaveSynth::createEditor(BetterProcessorEditor *parentEditor)
{
#if USE_BACKEND

	return new WaveSynthBody(parentEditor);

#else

	jassertfalse;
	return nullptr;

#endif
}


