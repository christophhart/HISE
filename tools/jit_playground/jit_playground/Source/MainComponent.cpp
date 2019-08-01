/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent()
{
	UnitTestRunner runner;
	runner.setAssertOnFailure(false);
	runner.runAllTests();

	hnode::jit::GlobalScope pool;
	hnode::jit::Compiler compiler(pool); 

	String code = "float member = 8.0f; float square(float input){member = input; return (float)input * input; }";

	if (auto obj = compiler.compileJitObject(code))
	{
		auto f = obj["square"];
		auto returnValue = f.call<float>(12.0f);
		DBG(returnValue);
	}

	addAndMakeVisible(playground);
    setSize (1024, 768);
}

MainComponent::~MainComponent()
{
}

//==============================================================================
void MainComponent::paint (Graphics& g)
{
    
}

void MainComponent::resized()
{
    // This is called when the MainComponent is resized.
    // If you add any child components, this is where you should
    // update their positions.
	playground.setBounds(getLocalBounds());
}
