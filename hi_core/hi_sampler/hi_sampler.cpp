/* HI Module */

#include "AppConfig.h"

#include "hi_sampler.h"

#include "sampler/StreamingSampler.cpp"

#include "sampler/dywapitchtrack/dywapitchtrack.c"

#include "sampler/ModulatorSamplerData.cpp"
#include "sampler/ModulatorSamplerSound.cpp"
#include "sampler/ModulatorSamplerVoice.cpp"
#include "sampler/ModulatorSampler.cpp"

#if USE_BACKEND

#include "sampler/SampleImporter.cpp"
#include "sampler/SfzImporter.cpp"

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
#include "sampler/components/SamplerBody.cpp"
#include "sampler/components/SampleEditingActions.cpp"

#endif