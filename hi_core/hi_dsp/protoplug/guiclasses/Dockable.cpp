/*
  ==============================================================================

    Dockable.cpp
    Created: 13 Apr 2014 4:37:52pm
    Author:  pac

  ==============================================================================
*/

#include "Dockable.h"



void DockablePopout::closeButtonPressed()
{ dad->postCommandMessage(1); }

