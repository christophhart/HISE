/*
  ==============================================================================

    Transposer.cpp
    Created: 5 Jul 2014 6:01:57pm
    Author:  Chrisboy

  ==============================================================================
*/

ProcessorEditorBody *Transposer::createEditor(BetterProcessorEditor *parentEditor)
{
#if USE_BACKEND

	return new TransposerEditor(parentEditor);

#else

	jassertfalse;

	return nullptr;

#endif
}