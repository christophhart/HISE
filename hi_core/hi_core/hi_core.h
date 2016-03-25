/*
  ==============================================================================

    hi_core.h
    Created: 21 Jun 2014 1:28:11pm
    Author:  Chrisboy

  ==============================================================================
*/

#ifndef HI_CORE_H_INCLUDED
#define HI_CORE_H_INCLUDED



/** @defgroup core Core Classes
*	A collection of basic classes.
*/

#include "HI_LookAndFeels.h"
#include "Popup.h"
#include "Tables.h"
#include "UpdateMerger.h"
#include "ExternalFilePool.h"
#include "BackgroundThreads.h"
#include "SampleThreadPool.h"
#include "PresetHandler.h"
#include "MainController.h"

#include "SampleExporter.h"
#include "Console.h"


#ifdef INCLUDE_INSTALLER

#include "Installer.h"

#endif


#include "JavascriptTokeniser.h"
#include "JavascriptTokeniserFunctions.h"



#include "MacroControlledComponents.h"
#include "MacroControlBroadcaster.h"

#endif  // HI_CORE_H_INCLUDED
