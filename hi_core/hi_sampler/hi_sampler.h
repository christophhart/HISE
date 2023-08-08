/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   HISE is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licenses for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licensing:
*
*   http://www.hise.audio/
*
*   HISE is based on the JUCE library,
*   which must be separately licensed for closed source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

/******************************************************************************

BEGIN_JUCE_MODULE_DECLARATION

  ID:               hi_sampler
  vendor:           Hart Instruments
  version:          2.0.0
  name:             HISE Sampler Module
  description:      The sampler module for HISE
  website:          http://hise.audio
  license:          GPL / Commercial

  dependencies:      juce_audio_basics, juce_audio_devices, juce_audio_formats, juce_audio_processors, juce_core, juce_cryptography, juce_data_structures, juce_events, juce_graphics, juce_gui_basics, juce_gui_extra, hi_dsp_library

END_JUCE_MODULE_DECLARATION

******************************************************************************/

#ifndef HI_SAMPLER_INCLUDED
#define HI_SAMPLER_INCLUDED

/** @defgroup sampler Sampler
	@ingroup dsp
	
	All classes related to the sample streaming engine of HISE.
*/


#include "sampler/ModulatorSamplerData.h"
#include "sampler/ModulatorSamplerSound.h"
#include "sampler/ModulatorSamplerVoice.h"
#include "sampler/ModulatorSampler.h"

#include "sampler/SfzImporter.h"
#include "sampler/MultiSampleDataProviders.h"

#if USE_BACKEND || HI_ENABLE_EXPANSION_EDITING
#include "sampler/SampleImporter.h"

#include "sampler/components/FileNamePartComponent.h"
#include "sampler/components/FileNameImporterDialog.h"
#include "sampler/components/FileImportDialog.h"
#include "sampler/components/SfzGroupSelectorComponent.h"
#include "sampler/components/SampleEditorComponents.h"
#include "sampler/components/SamplerSettings.h"
#include "sampler/components/ValueSettingComponent.h"
#include "sampler/components/SamplerToolbar.h"
#include "sampler/components/SampleEditor.h"
#include "sampler/components/SampleMapEditor.h"
#include "sampler/components/SamplerTable.h"
#include "sampler/components/SampleEditHandler.h"
#endif


#if USE_BACKEND
#include "sampler/components/SamplerBody.h"
#endif

#include "sampler/components/SampleMapBrowser.h"



#endif   // HI_SAMPLER_INCLUDED
