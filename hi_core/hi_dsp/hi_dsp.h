/*
  ==============================================================================

    hi_dsp.h
    Created: 21 Jun 2014 1:28:20pm
    Author:  Chrisboy

  ==============================================================================
*/

#ifndef HI_DSP_H_INCLUDED
#define HI_DSP_H_INCLUDED



/** @defgroup utility Utility Classes
*
*	A collection of small helper classes.
*/

/** @defgroup dsp DSP classes
*
*	Contains all classes for DSP.
*/

/**	@defgroup dspEditor DSP Editors
*	@ingroup dsp
*
*	Contains all classes related to the editors of DSP modules
*/



#include "Processor.h"

#include "Routing.h"

#if USE_BACKEND
#include "RoutingEditor.h"
#endif

/**	@defgroup views View Configuration classes
*	@ingroup core
*
*	Contains all classes related to storing / recalling different view configurations.
*
*	By default, all existing processors are displayed within a tree structure and can be folded.
*	The view configuration system allows to change the processor that will be displayed as root, add designated processors on the root level
*	and save the exact state of every processor to create customized view options within a patch.
*
*	This is incredibly useful for larger patches, where you can define different views 
*	(eg. all interface scripts, or all modulators that alter the pitch, or all sample maps of the release trigger samplers)
*	and toggle these views within one mouse click.
*/



#if USE_BACKEND

#include "editor/ProcessorEditor.h"
#include "editor/ProcessorEditorHeaderExtras.h"
#include "editor/ProcessorEditorHeader.h"
#include "editor/ProcessorEditorChainBar.h"
#include "editor/ProcessorEditorPanel.h"
#include "editor/ProcessorViews.h"

#else

// Return type for all empty createEditor methods
class ProcessorEditor;

#endif




/**	@defgroup modulator Modulator classes
*	@ingroup dsp
*
*	Contains all classes related to Modulators
*/

#include "modules/Modulators.h"
#include "modules/ModulatorChain.h"


/**	@defgroup midiProcessor MidiProcessor classes
*	@ingroup dsp
*
*	Contains all classes related to Midi-Processors
*/

#include "modules/MidiProcessor.h"

/**	@defgroup effect Effect classes
*	@ingroup dsp
*
*	Contains all classes related to Audio FX classes.
*/

#include "modules/EffectProcessor.h"
#include "modules/EffectProcessorChain.h"

/**	@defgroup modulatorSynth ModulatorSynth classes
*	@ingroup dsp
*
*	Contains all classes related to Modulator-Synthesisers.
*/

/**	@defgroup macroControl Macro Control classes
*	@ingroup dsp
*
*	The Macro Control System
*
*	The macro control system is a powerful and easy to use system to map every possible parameter to one of eight slots within
*	a ModulatorSynthChain.
*
*	<b>Features:</b>
*
*	- map any parameter to one of eight macro controls with a customizable and invertible range.
*	- control those macro controls via custom interfaces or ScriptProcessor scripts.
*	- enhanced parameter control (sample accurate, table editing, smoothing) for all modulatable parameters with the MacroModulator class.
*	- the macro controls are saved on ModulatorSynthChain level, so that multiple macro control configurations can be used.
*	- simple learn mode (open the panel and move a parameter and it gets mapped automatically)
*/


#include "modules/ModulatorSynth.h"

#include "modules/ModulatorSynthChain.h"

#if INCLUDE_PROTOPLUG
#include "protoplug/hi_protoplug.h"
#endif

// Plugin Parameters

#include "plugin_parameter/PluginParameterProcessor.h"

#endif  // HI_DSP_H_INCLUDED
