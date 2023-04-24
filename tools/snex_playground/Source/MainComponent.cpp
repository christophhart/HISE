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
	
	- Function calls!
	- global data OK
	- Classes with object pointers
	- local stack data
	- dyn & span
	- iterator loop
	
	
	*/


	bool useValueTrees = false;
	
	laf.setDefaultColours(funkSlider);

	GlobalScope m;

	//m.addOptimization(OptimizationIds::ConstantFolding);
	//m.addOptimization(OptimizationIds::BinaryOpOptimisation);
	//m.addOptimization(OptimizationIds::DeadCodeElimination);

	Compiler c(m);

	
	MirObject mobj;
	
	auto obj = c.compileJitObject("int test(int input) { return input == 0 ? input * 12 : input * 3; }");

	//auto obj = c.compileJitObject("int x = 12; int test(float input) { return x; }");
	DBG(c.getCompileResult().getErrorMessage());

	auto mirCode = c.getAST();

	auto b64 = SyntaxTreeExtractor::getBase64SyntaxTree(mirCode);

	DBG(mirCode.createXml()->createDocument(""));

	DBG(c.getAssemblyCode());
	auto ok = mobj.compileMirCode(b64);

	DBG(ok.getErrorMessage());
	
	auto f = obj["test"];

	

	auto res1 = f.call<int>(14);

	if (ok)
	{
		auto f = mobj["test"];



		auto res2 = f.call<int>(14);
		int ssdfsdf = 0;
	}


	
	

	

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

		for (auto o : OptimizationIds::getAllIds())
			data->getGlobalScope().addOptimization(o);

		playground = new snex::ui::SnexPlayground(data, true);
		provider = new snex::ui::SnexPlayground::TestCodeProvider(*playground, {});
		data->setCodeProvider(provider, sendNotification);

		
	}

	context.attachTo(*this);
	
	addAndMakeVisible(playground);

    setSize (1024, 768);
}

MainComponent::~MainComponent()
{
	data = nullptr;

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
