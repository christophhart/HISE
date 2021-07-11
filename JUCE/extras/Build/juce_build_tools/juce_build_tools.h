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

/*******************************************************************************
 The block below describes the properties of this module, and is read by
 the Projucer to automatically generate project code that uses it.
 For details about the syntax and how to create or use a module, see the
 JUCE Module Format.md file.


 BEGIN_JUCE_MODULE_DECLARATION

  ID:                 juce_build_tools
  vendor:             juce
  version:            6.0.8
  name:               JUCE Build Tools
  description:        Classes for generating intermediate files for JUCE projects.
  website:            http://www.juce.com/juce
  license:            GPL/Commercial

  dependencies:       juce_gui_basics

 END_JUCE_MODULE_DECLARATION

*******************************************************************************/

// This module is shared by juceaide and the Projucer, but should not be
// considered 'public'. That is, its API, functionality, and contents (and
// existence!) may change between releases without warning.

#pragma once
#define JUCE_BUILD_TOOLS_H_INCLUDED

#include <juce_gui_basics/juce_gui_basics.h>

#include "utils/juce_ProjectType.h"
#include "utils/juce_BuildHelperFunctions.h"
#include "utils/juce_BinaryResourceFile.h"
#include "utils/juce_RelativePath.h"
#include "utils/juce_Icons.h"
#include "utils/juce_PlistOptions.h"
#include "utils/juce_ResourceFileHelpers.h"
#include "utils/juce_ResourceRc.h"
#include "utils/juce_VersionNumbers.h"
#include "utils/juce_Entitlements.h"
