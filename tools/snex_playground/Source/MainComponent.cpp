/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#include "MainComponent.h"





using namespace hise;
using namespace scriptnode;
using namespace snex::Types;
using namespace snex::jit;
using namespace snex;





//==============================================================================
MainComponent::MainComponent() :
	data(new ui::WorkbenchData())
{
	/** TODO:
	
	- split the entire snex compiler into parser & code generator, then test everything

	- chew threw unit tests with MIR
	- implement dyn
	- add all library objects (polydata, osc process data, etc)

	- remove all unnecessary things:
	  - inlining (?)
	  - indexes (?)
	  - 
	

	*/


	bool useValueTrees = false;
	
	laf.setDefaultColours(funkSlider);
    
	if (useValueTrees)
	{
		data->getTestData().setUpdater(&updater);

		auto compileThread = new snex::jit::JitNodeCompileThread(data);
		data->setCompileHandler(compileThread);

		provider = new snex::ui::ValueTreeCodeProvider(data, 2);
		data->setCodeProvider(provider);

		playground = new snex::ui::SnexPlayground(data, true);
		playground->setReadOnly(true);
		
		addAndMakeVisible(parameters = new snex::ui::ParameterList(data));

		addAndMakeVisible(testData = new snex::ui::TestDataComponent(data));

		addAndMakeVisible(graph1 = new snex::ui::Graph());
		addAndMakeVisible(graph2 = new snex::ui::Graph());
		addAndMakeVisible(complexData = new snex::ui::TestComplexDataManager(data));
	}
	else
	{
		auto compileThread = new snex::jit::TestCompileThread(data);
		data->setCompileHandler(compileThread);

		for (auto o : OptimizationIds::Helpers::getDefaultIds())
			data->getGlobalScope().addOptimization(o);

		playground = new snex::ui::SnexPlayground(data, true);
		provider = new snex::ui::SnexPlayground::TestCodeProvider(*playground, {});
		data->setCodeProvider(provider, sendNotification);

		
	}

    //context.attachTo(*playground);
    
    



	
    
    addAndMakeVisible(playground);
    
    playground->toFront(true);
    context.attachTo(*playground);
    
    setSize (1024, 768);
}

MainComponent::~MainComponent()
{
	context.detach();

	
    

}

//==============================================================================
void MainComponent::paint (Graphics& g)
{
	g.fillAll(Colour(0xFF333336));
}

void MainComponent::resized()
{
	auto b = getLocalBounds();

	if (parameters != nullptr)
		parameters->setBounds(b.removeFromTop(50));

	if (graph1 != nullptr)
	{
		//auto gb = b.removeFromTop(400);
		testData->setBounds(b.removeFromTop(150));
		graph1->setBounds(b.removeFromTop(150));// .removeFromLeft(400));
		//graph2->setBounds(b);// .removeFromLeft(400));
		
		
	}
	
    playground->setBounds(b);
}
