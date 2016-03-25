/*
  ==============================================================================

    ConstantModulator.cpp
    Created: 18 Jun 2014 8:27:52pm
    Author:  Chrisboy

  ==============================================================================
*/



ProcessorEditorBody *PluginParameterModulator::createEditor(BetterProcessorEditor *parentEditor)
{
#if USE_BACKEND

	return new PluginParameterEditorBody(parentEditor);

	
#else

	jassertfalse;

	return nullptr;

#endif
};



