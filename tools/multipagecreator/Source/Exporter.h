

#pragma once
#include <JuceHeader.h>
namespace hise {
namespace multipage {
using namespace juce;

struct projucer_exporter: public HardcodedDialogWithState
{
	projucer_exporter(const File& rootDir_, State& appState_): appState(appState_), rootDir(rootDir_) { setSize(700, 600); };

	var exportProjucerProject(State::Job& t, const var& stateObject);

	void postInit() override;

	File rootDir;

	// not the dialogs state but the global one!
	State& appState;

	Dialog* createDialog(State& state) override;

	var exportObj;
	
};
} // namespace multipage
} // namespace hise

