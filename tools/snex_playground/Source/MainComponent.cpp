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

		for (auto o : OptimizationIds::getDefaultIds())
			data->getGlobalScope().addOptimization(o);

		playground = new snex::ui::SnexPlayground(data, true);
		provider = new snex::ui::SnexPlayground::TestCodeProvider(*playground, {});
		data->setCodeProvider(provider, sendNotification);

		
	}

    //context.attachTo(*playground);
    
    WebViewData::Ptr data = new WebViewData();



	auto root = File::getSpecialLocation(File::userDesktopDirectory).getChildFile("webtest");

	
	auto cacheFile = root.getChildFile("cached.dat");

	if (cacheFile.existsAsFile())
	{
		zstd::ZDefaultCompressor comp;
		
		ValueTree v;
		comp.expand(cacheFile, v);
		data->restoreFromValueTree(v);
	}
	else
	{
		data->setRootDirectory(root);
		data->setEnableCache(true);
	}
		
	data->setIndexFile("/index.html");

    data->addCallback("getX", [](const var& v)
    {
        return JUCE_LIVE_CONSTANT(0.01);
    });
    
    data->setErrorLogger([](const String& d)
    {
        DBG(d);
    });
    
	

	
    addAndMakeVisible(webViewWrapper = new WebViewWrapper(data));
    
    addAndMakeVisible(playground);
    
    //playground->toFront(true);
    //context.attachTo(*playground);
    
	addAndMakeVisible(funkSlider);
	
	funkSlider.onValueChange = [data, this]()
	{
		auto v = funkSlider.getValue();

		data->call("something", v);
	};


    setSize (1024, 768);
}

MainComponent::~MainComponent()
{
	auto wv = dynamic_cast<WebViewWrapper*>(webViewWrapper.get());

	auto data2 = wv->getData();

	auto v = data2->exportAsValueTree();

	auto root = File::getSpecialLocation(File::userDesktopDirectory).getChildFile("webtest");
	auto cacheFile = root.getChildFile("cached.dat");
	cacheFile.deleteFile();

	//FileOutputStream fos(cacheFile);
	//v.writeToStream(fos);
	
	zstd::ZDefaultCompressor comp;

	comp.compress(v, cacheFile);

	//fos.flush();

	webViewWrapper = nullptr;

	data = nullptr;

	//context.detach();

	
    

}

//==============================================================================
void MainComponent::paint (Graphics& g)
{
	//g.fillAll(Colour(0xFF333336));
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
	
	funkSlider.setBounds(b.removeFromBottom(100));
    
    webViewWrapper->setBounds(b);
    playground->setBounds(b.removeFromBottom(200));
    
    
	
}
