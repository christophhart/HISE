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

#ifdef JUCE_BUILD_TOOLS_H_INCLUDED
 /* When you add this cpp file to your project, you mustn't include it in a file where you've
    already included any other headers - just put it inside a file on its own, possibly with your config
    flags preceding it, but don't include anything else. That also includes avoiding any automatic prefix
    header files that the compiler may be using.
 */
 #error "Incorrect use of JUCE cpp file"
#endif

#include "juce_build_tools.h"

#include <numeric>

#include "utils/juce_CppTokeniserFunctions.cpp"
#include "utils/juce_BuildHelperFunctions.cpp"
#include "utils/juce_BinaryResourceFile.cpp"
#include "utils/juce_Icons.cpp"
#include "utils/juce_PlistOptions.cpp"
#include "utils/juce_ResourceFileHelpers.cpp"
#include "utils/juce_ResourceRc.cpp"
#include "utils/juce_VersionNumbers.cpp"
#include "utils/juce_Entitlements.cpp"
