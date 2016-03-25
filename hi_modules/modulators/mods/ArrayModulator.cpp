/*
  ==============================================================================

    ArrayModulator.cpp
    Created: 15 Mar 2016 7:35:52pm
    Author:  Christoph

  ==============================================================================
*/

#include "ArrayModulator.h"

ProcessorEditorBody * ArrayModulator::createEditor(BetterProcessorEditor *parentEditor)
{
#if USE_BACKEND
	return new ArrayModulatorEditor(parentEditor);
#else 
	jassertfalse;
	return nullptr;
#endif
}
