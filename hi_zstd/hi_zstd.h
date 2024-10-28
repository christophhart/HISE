/*  ===========================================================================

BSD License

For Zstandard software

Copyright (c) 2016-present, Facebook, Inc. All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name Facebook nor the names of its contributors may be used to
   endorse or promote products derived from this software without specific
   prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*   ===========================================================================
*/

/******************************************************************************

BEGIN_JUCE_MODULE_DECLARATION

  ID:               hi_zstd
  vendor:           Facebook
  version:          4.0.0
  name:             ZSTD compression module
  description:      a wrapper around Facebook's zstd compression algorithm
  website:          https://github.com/facebook/zstd
  license:          BSD / GPL

  dependencies:     juce_core, juce_cryptography, juce_data_structures, juce_events

END_JUCE_MODULE_DECLARATION

******************************************************************************/

#pragma once

#ifndef HI_ZSTD_INCLUDED
#define HI_ZSTD_INCLUDED 1
#endif



#include "../JUCE/modules/juce_core/juce_core.h"
#include "../JUCE/modules/juce_data_structures/juce_data_structures.h"



#include "hi_zstd/ZstdHelpers.h"
#include "hi_zstd/ZstdInputStream.h"
#include "hi_zstd/ZstdOutputStream.h"
#include "hi_zstd/ZstdDictionaries.h"
#include "hi_zstd/ZstdDictionaries_Impl.h"
#include "hi_zstd/ZstdCompressor.h"
#include "hi_zstd/ZstdCompressor_Impl.h"

