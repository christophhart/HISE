#pragma once

/*
BEGIN_JUCE_MODULE_DECLARATION

ID:               melatonin_blur
vendor:           Sudara
version:          1.3.0
name:             Optimized CPU vector blurring and JUCE drop shadowing with tests and benchmarks
description:      Blurry Life
license:          MIT
minimumCppStandard: 17
dependencies:     juce_graphics,juce_gui_basics,juce_audio_basics

END_JUCE_MODULE_DECLARATION
*/

#include "juce_graphics/juce_graphics.h"
#include "juce_gui_basics/juce_gui_basics.h"

#include "melatonin/cached_blur.h"
#include "melatonin/shadows.h"

