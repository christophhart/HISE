/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2020 - Raw Material Software Limited

   JUCE is an open source library subject to commercial or open-source
   licensing.

   By using JUCE, you agree to the terms of both the JUCE 6 End-User License
   Agreement and JUCE Privacy Policy (both effective as of the 16th June 2020).

   End User License Agreement: www.juce.com/juce-6-licence
   Privacy Policy: www.juce.com/juce-privacy-policy

   Or: You may also use this code under the terms of the GPL v3 (see
   www.gnu.org/licenses).

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

#pragma once


//==============================================================================
/**
    A namespace to hold all the possible command IDs.
*/
namespace JucerCommandIDs
{
    enum
    {
        test                   = 0xf20009,
        toFront                = 0xf2000a,
        toBack                 = 0xf2000b,

        group                  = 0xf20017,
        ungroup                = 0xf20018,

        showGrid               = 0xf2000e,
        enableSnapToGrid       = 0xf2000f,

        editCompLayout         = 0xf20010,
        editCompGraphics       = 0xf20011,

        bringBackLostItems     = 0xf20012,

        zoomIn                 = 0xf20013,
        zoomOut                = 0xf20014,
        zoomNormal             = 0xf20015,
        spaceBarDrag           = 0xf20016,

        compOverlay0           = 0xf20020,
        compOverlay33          = 0xf20021,
        compOverlay66          = 0xf20022,
        compOverlay100         = 0xf20023,

        newDocumentBase        = 0xf32001,
        newComponentBase       = 0xf30001,
        newElementBase         = 0xf31001,

        alignTop               = 0xf33000,
        alignRight             = 0xf33001,
        alignBottom            = 0xf33002,
        alignLeft              = 0xf33003,
    };
}
