

#pragma once
#include <JuceHeader.h>
namespace hise {
namespace multipage {
using namespace juce;
struct PayloadInstaller: public HardcodedDialogWithState
{
	PayloadInstaller()
	{
		setSize(300, 200);
	}

	void postInit() override
	{
		if(dialog != nullptr)
		{
			setSize(dialog->getWidth(), dialog->getHeight());
		}
	}

	Dialog* createDialog(State& state) override;
	
};
} // namespace multipage
} // namespace hise

using DialogClass = hise::multipage::PayloadInstaller;