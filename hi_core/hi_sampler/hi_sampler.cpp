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

/* HI Module */

#include <regex>

#include "sampler/ModulatorSamplerData.cpp"
#include "sampler/ModulatorSamplerSound.cpp"
#include "sampler/ModulatorSamplerVoice.cpp"
#include "sampler/ModulatorSampler.cpp"

#include "sampler/MultiSampleDataProviders.cpp"
#include "sampler/SfzImporter.cpp"

#if USE_BACKEND || HI_ENABLE_EXPANSION_EDITING
#include "sampler/SampleImporter.cpp"

#include "sampler/components/FileNamePartComponent.cpp"
#include "sampler/components/FileNameImporterDialog.cpp"
#include "sampler/components/FileImportDialog.cpp"
#include "sampler/components/SfzGroupSelectorComponent.cpp"

#include "sampler/components/SampleEditorComponents.cpp"
#include "sampler/components/SamplerSettings.cpp"
#include "sampler/components/ValueSettingComponent.cpp"
#include "sampler/components/SamplerToolbar.cpp"
#include "sampler/components/SampleEditor.cpp"
#include "sampler/components/SampleMapEditor.cpp"
#include "sampler/components/SamplerTable.cpp"
#include "sampler/components/SampleEditHandler.cpp"
#include "sampler/components/SampleEditingActions.cpp"
#endif

#if USE_BACKEND

#include "sampler/components/SamplerBody.cpp"


#endif

#include "sampler/components/SampleMapBrowser.cpp"

