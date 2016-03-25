/*
  ==============================================================================

    hi_dsp.cpp
    Created: 14 Jul 2014 1:47:11pm
    Author:  Chrisboy

  ==============================================================================
*/

#include "hi_dsp.h"


#include "Processor.cpp"

#include "Routing.cpp"



#if USE_BACKEND

#include "RoutingEditor.cpp"

#include "editor/ProcessorEditor.cpp"
#include "editor/ProcessorEditorHeaderExtras.cpp"
#include "editor/ProcessorEditorHeader.cpp"
#include "editor/ProcessorEditorChainBar.cpp"

#include "editor/ProcessorEditorPanel.cpp"
#include "editor/ProcessorViews.cpp"

#endif

#include "modules/Modulators.cpp"
#include "modules/ModulatorChain.cpp"
#include "modules/MidiProcessor.cpp"
#include "modules/EffectProcessor.cpp"
#include "modules/EffectProcessorChain.cpp"
#include "modules/ModulatorSynth.cpp"
#include "modules/ModulatorSynthChain.cpp"

#if INCLUDE_PROTOPLUG
#include "protoplug/hi_protoplug.cpp"
#endif

#include "plugin_parameter/PluginParameterProcessor.cpp"