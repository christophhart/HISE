/*
  ==============================================================================

    Delay.cpp
    Created: 8 Aug 2014 11:33:54pm
    Author:  Christoph

  ==============================================================================
*/

#include "Delay.h"

ProcessorEditorBody *DelayEffect::createEditor(BetterProcessorEditor *parentEditor)
{
#if USE_BACKEND

	return new DelayEditor(parentEditor);

#else

	jassertfalse;

	return nullptr;

#endif
}