/** ============================================================================
 *
 * mcl_editor JUCE module
 *
 * Copyright (C) Jonathan Zrake, Christoph Hart
 *
 * You may use, distribute and modify this code under the terms of the GPL3
 * license.
 * =============================================================================
 */

#pragma once


/** CONFIG: MCL_ENABLE_OPEN_GL
*
*	Disable this if your project doesn't include the open gl module
*/
#ifndef MCL_ENABLE_OPEN_GL
#define MCL_ENABLE_OPEN_GL 1
#endif

/** Config: ENABLE_CARET_BLINK
*
*	Enable this to make the caret blink (disabled by default)
*/
#ifndef ENABLE_CARET_BLINK
#define ENABLE_CARET_BLINK 1
#endif


// I'd suggest to split up this big file to multiple files per class and include them here one by one
#include "code_editor/Helpers.h"
#include "code_editor/Selection.h"
#include "code_editor/GlyphArrangementArray.h"
#include "code_editor/TextDocument.h"
#include "code_editor/LanguageManager.h"
#include "code_editor/DocTree.h"
#include "code_editor/CodeMap.h"
#include "code_editor/CaretComponent.h"
#include "code_editor/HighlightComponent.h"
#include "code_editor/Gutter.h"
#include "code_editor/Autocomplete.h"
#include "code_editor/TextEditor.hpp"
#include "code_editor/FullEditor.hpp"


