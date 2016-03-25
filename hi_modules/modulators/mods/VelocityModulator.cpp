/*
  ==============================================================================

    VelocityModulator.cpp
    Created: 18 Jun 2014 8:27:35pm
    Author:  Chrisboy

  ==============================================================================
*/

ProcessorEditorBody *VelocityModulator::createEditor(BetterProcessorEditor *parentEditor)
{

#if USE_BACKEND

	return new VelocityEditorBody(parentEditor);

#else

	jassertfalse;

	return nullptr;

#endif

};