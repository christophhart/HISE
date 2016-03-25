#ifndef HI_SAMPLER_INCLUDED
#define HI_SAMPER_INCLUDED

#include <AppConfig.h>

#include "../hi_core/hi_core.h"
#include "../hi_scripting/hi_scripting.h"
#include "../../hi_modules/hi_modules.h"

using namespace juce;

#include "sampler/StreamingSampler.h"

#include "sampler/dywapitchtrack/dywapitchtrack.h"
#include "sampler/PitchDetection.h"

#include "sampler/ModulatorSamplerData.h"
#include "sampler/ModulatorSamplerSound.h"
#include "sampler/ModulatorSamplerVoice.h"
#include "sampler/ModulatorSampler.h"

#if USE_BACKEND

#include "sampler/SampleImporter.h"
#include "sampler/SfzImporter.h"

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
#include "sampler/components/SamplerBody.h"

#endif

#endif   // HI_SAMPLER_INCLUDED
