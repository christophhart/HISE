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

#if 0
	GlobalScope m;

	//m.addOptimization(OptimizationIds::ConstantFolding);
	//m.addOptimization(OptimizationIds::BinaryOpOptimisation);
	//m.addOptimization(OptimizationIds::DeadCodeElimination);

	Compiler c(m);

	SnexObjectDatabase::registerObjects(c, 2);
	
	MirObject::setLibraryFunctions(c.getFunctionMap());


	MirObject mobj;

	
	
	String nl = "\n";
	String ml;

#if 0
	ml << "namespace Math {" << nl;
	ml << "int sign(int value) { return -1 + 2 * (value > 0); };" << nl;
	ml << "int abs(int value) { return sign(value) * value; }; " << nl;
	ml << "float sign(float value) { return -1.0f + 2.0f * (float)(value > 0.0f); };" << nl;
	ml << "float abs(float value) { return sign(value) * value; };" << nl;
	//ml << "const double PI = 3.14;" << nl; // enough..
	ml << "};" << nl;
#endif

	


	ml << "int test(HiseEvent& e) { return e.getNoteNumber(); };" << nl;

	auto obj = c.compileJitObject(ml);

	//auto obj = c.compileJitObject("struct X { int y = 90; span<int, 4> data = { 1, 2, 3, 4 }; double z = 20.0;  void change(double v) { z = 70.0 + v; } }; X obj;  double test(double input) { obj.change(input); return obj.z; };");

	//auto obj = c.compileJitObject("int x = 12; int test(float input) { return x; }");
	DBG(c.getCompileResult().getErrorMessage());

	auto mirCode = c.getAST();

	auto b64 = SyntaxTreeExtractor::getBase64SyntaxTree(mirCode);

	DBG(mirCode.createXml()->createDocument(""));

	DBG(c.getAssemblyCode());
	auto ok = mobj.compileMirCode(b64);

	DBG(ok.getErrorMessage());
	
	auto f = obj["test"];

	HiseEvent ev(HiseEvent::Type::NoteOn, 64, 127, 1);

	auto res1 = f.call<int>(&ev);

	if (ok)
	{
		auto f = mobj["test"];

		auto res2 = f.call<int>(&ev);
		int ssdfsdf = 0;
	}
#endif

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

		for (auto o : OptimizationIds::getDefaultIds())
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
