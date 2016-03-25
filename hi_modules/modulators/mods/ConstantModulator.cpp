/*
  ==============================================================================

    ConstantModulator.cpp
    Created: 18 Jun 2014 8:27:52pm
    Author:  Chrisboy

  ==============================================================================
*/



ProcessorEditorBody *ConstantModulator::createEditor(BetterProcessorEditor *parentEditor)
{
#if USE_BACKEND

	return new EmptyProcessorEditorBody(parentEditor);

#else 

	jassertfalse;
	return nullptr;

#endif
};
