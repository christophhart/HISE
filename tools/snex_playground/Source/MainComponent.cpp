/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#include "MainComponent.h"

using namespace snex::Types;
using namespace snex::jit;
using namespace snex;



/** A class that wraps a SNEX compiled object and supplies a predefined
	function list that can be called from C++
*/
struct JitObjectWithPrototype
{
	/** Create an object with the given code and supply a scope. */
	JitObjectWithPrototype(snex::jit::GlobalScope& s_) :
		s(s_)
	{}



	template <int FunctionIndex, typename ReturnType, typename... Parameters>
	ReturnType call(Parameters... ps)
	{
		auto ptr = reinterpret_cast<void*>(data.get());
		return memberFunctions[FunctionIndex].callUnchecked<ReturnType>(ptr, ps...);
	}

	virtual int getNumChannels() const = 0;

	virtual void compiled(snex::jit::Compiler& c, Result r) = 0;

	virtual Array<Identifier> getFunctionList() const = 0;

	/** Register all functions that you want to call. */
	Result compile(const juce::String& code)
	{

		ok = false;
		snex::jit::Compiler c(s);
		SnexObjectDatabase::registerObjects(c, getNumChannels());
		obj = c.compileJitObject(code);

		if (!c.getCompileResult().wasOk())
		{
			auto r = c.getCompileResult();
			compiled(c, r);
			return r;
		}

		if (auto type = c.getComplexType(snex::NamespacedIdentifier("instance"), {}))
		{
			objectSize = type->getRequiredByteSize();

			data.calloc(objectSize);

			snex::jit::ComplexType::InitData d;
			d.dataPointer = data.get();
			d.initValues = type->makeDefaultInitialiserList();

			type->initialise(d);

			snex::jit::FunctionClass::Ptr fc = type->getFunctionClass();

			if (fc == nullptr)
			{
				auto r = Result::fail("Main object does not have member functions");
				compiled(c, r);
				return r;
			}

			for (auto id : getFunctionList())
			{
				auto fId = fc->getClassName().getChildId(id);
				auto fData = fc->getNonOverloadedFunction(fId);

				if (!fData.isResolved())
				{
					auto r = Result::fail("Can't find function " + id.toString());
					compiled(c, r);
					return r;
				}


				fData.object = data;
				memberFunctions.add(fData);
			}
		}
		else
		{
			auto r = Result::fail("Can't find instance type");
			compiled(c, r);
			return r;
		}

		ok = true;

		auto r = Result::ok();
		compiled(c, r);
		return r;
	}

	bool ok = false;

	snex::jit::GlobalScope& s;
	snex::jit::JitObject obj;
	HeapBlock<uint8> data;
	size_t objectSize = 0;

	Array<snex::jit::FunctionData> memberFunctions;
};



template <int NumChannels> struct ScriptNodeClassPrototype : public JitObjectWithPrototype
{
	

	ScriptNodeClassPrototype(snex::jit::GlobalScope& s) :
		JitObjectWithPrototype(s)
	{

	}

	int getNumChannels() const override { return NumChannels; }

	virtual void compiled(snex::jit::Compiler& c, Result r) override
	{

	}

	Array<Identifier> getFunctionList() const override
	{
		return { "reset", "process", "processFrame", "prepare", "handleEvent" };
	}

	void reset()
	{
		if (ok)
			call<ScriptnodeCallbacks::ResetFunction, void>();
	}

	void process(ProcessDataDyn& d)
	{
		if (ok)
		{
			auto f = d.toFix<NumChannels>();
			call<ScriptnodeCallbacks::ProcessFunction, void>(&f);
		}
	}

	void processSingle(float* data, int numChannels)
	{
		if (ok)
		{
			auto& d = span<float, NumChannels>::fromExternalData(data, numChannels);
			call<ScriptnodeCallbacks::ProcessSingleFunction, void>(&d);
		}
	}

	void prepare(PrepareSpecs ps)
	{
		ps.numChannels = NumChannels;

		if (ok)
			call<ScriptnodeCallbacks::PrepareFunction, void>(&ps);
	}

	void handleEvent(HiseEvent& e)
	{
		if (ok)
			call<ScriptnodeCallbacks::HandleEventFunction, void>(&e);
	}
};

juce::String getEmpty(const Identifier& namespaceId)
{
	juce::String s;
	juce::String nl = "\n";

	s << "struct " << namespaceId.toString() << nl;
	s << "{" << nl;
	s << "    void reset() {};" << nl;
	s << "    void prepare(PrepareSpecs ps) {};" << nl;
	s << "    void process(ProcessData& d){};" << nl;
	s << "    void handleEvent(HiseEvent& e){};" << nl;
	s << "};" << nl << nl;
	s << "using instance = " << namespaceId.toString() << ";" << nl;

	return s;
}


struct test_table
{
	SN_GET_SELF_AS_OBJECT(test_table);

	static const int NumTables = 1;
	static const int NumSliderPacks = 0;
	static const int NumAudioFiles = 0;

	HISE_EMPTY_PREPARE;
	HISE_EMPTY_RESET;
	HISE_EMPTY_PROCESS;
	HISE_EMPTY_PROCESS_SINGLE;
	HISE_EMPTY_HANDLE_EVENT;

	void setExternalData(const ExternalData& d, int index)
	{
		d.referBlockTo(internalData, 0);
	}

	block internalData;
};


using namespace scriptnode;

namespace scriptnode 
{
namespace wrap
{

}
}




struct FunkyData
{
	span<float, 128> data =
	{   
		0.98293273f, 0.083770002f, 0.049215005f, 0.46574186f,
		0.3699328f, 0.077760494f, 0.20188127f, 0.45882956f,
		0.79893166f, 0.17817925f, 0.39334161f, 0.9944781f,
		0.16272518f, 0.6708219f, 0.80794162f, 0.89819765f,
		0.88123468f, 0.60708319f, 0.72685037f, 0.14657131f,
		0.88341391f, 0.17934901f, 0.51136695f, 0.21482391f,
		0.24232234f, 0.40386775f, 0.99303175f, 0.20041458f,
		0.4133036f, 0.0010109104f, 0.35801062f, 0.8327572f,
		0.67851717f, 0.39967141f, 0.68135488f, 0.65385875f,
		0.97900148f, 0.3318893f, 0.45139003f, 0.99835871f,
		0.40881919f, 0.42505153f, 0.26178591f, 0.85262836f,
		0.048594839f, 0.58222613f, 0.20306654f, 0.93534866f,
		0.77497217f, 0.19702313f, 0.79881407f, 0.13605712f,
		0.14838836f, 0.44911654f, 0.87511395f, 0.75888721f,
		0.35740533f, 0.032125454f, 0.87351156f, 0.34709749f,
		0.59010001f, 0.77947827f, 0.49466204f, 0.47660084f,
		0.67126548f, 0.2693361f, 0.44683661f, 0.085785189f,
		0.61110946f, 0.74041796f, 0.55350956f, 0.20231732f,
		0.072575202f, 0.5610661f, 0.70852187f, 0.39881603f,
		0.099298452f, 0.22915382f, 0.64505892f, 0.78737352f,
		0.62662255f, 0.9961339f, 0.27327271f, 0.69261933f,
		0.89921753f, 0.90094302f, 0.3363325f, 0.43471232f,
		0.46900852f, 0.35552988f, 0.30963335f, 0.5332882f,
		0.67775931f, 0.6710015f, 0.12458851f, 0.50258624f,
		0.6243484f, 0.66994725f, 0.3167612f, 0.65395415f,
		0.46521332f, 0.055192671f, 0.28606196f, 0.62753681f,
		0.33844392f, 0.64746925f, 0.30048265f, 0.92866954f,
		0.28152311f, 0.25430081f, 0.78861418f, 0.50042064f,
		0.72681286f, 0.47046658f, 0.042386493f, 0.29158823f,
		0.70776144f, 0.45799377f, 0.21058219f, 0.11460438f,
		0.94578363f, 0.92267316f, 0.0043439034f, 0.5812113f,
		0.86928468f, 0.44711893f, 0.78487704f, 0.072052477f
	};
};






using MyProcessor = scriptnode::wrap::data<core::table, data::embedded::table<FunkyData>>;

using MyProcessor2 = scriptnode::wrap::data<core::table, data::external::table<0>>;



/** => move this to hi_scripting (or anywhere where it's wrapped on HISE level. */
template <typename T> struct HardcodedExternalHandler: public snex::ExternalDataProviderBase
{
	GET_SELF_OBJECT(*this);
	GET_WRAPPED_OBJECT(obj.getWrappedObject());

	HardcodedExternalHandler()
	{
		initExternalData();
	}

	int getNumRequiredDataObjects(ExternalData::DataType t) const override
	{
		switch (t)
		{
		case ExternalData::DataType::AudioFile:		return T::NumAudioFiles;
		case ExternalData::DataType::SliderPack:	return T::NumSliderPacks;
		case ExternalData::DataType::Table:			return T::NumTables;
		default:									return 0;
		}
	}

	HISE_DEFAULT_INIT(HardcodedExternalHandler);
	HISE_DEFAULT_RESET(HardcodedExternalHandler);
	HISE_DEFAULT_PREPARE(HardcodedExternalHandler);
	HISE_DEFAULT_PROCESS(HardcodedExternalHandler);
	HISE_DEFAULT_PROCESS_FRAME(HardcodedExternalHandler);
	HISE_DEFAULT_HANDLE_EVENT(HardcodedExternalHandler);

	void setExternalData(const ExternalData& d, int index) override
	{
		obj.setExternalData(d, index);
	}

	T obj;
};





//==============================================================================
MainComponent::MainComponent() :
	data(new ui::WorkbenchData())
{
	bool useValueTrees = true;

	MyProcessor obj;

	SampleLookupTable tableObj;

	MyProcessor2 obj2;

	ExternalData d;

	ExternalData d2(&tableObj, 0);

	//obj2.setExternalData(d2, 0);
	//obj.setExternalData(d, 0);
	

	using T = PolyData<uint8, 1>;

	constexpr int s = sizeof(T);


	if (useValueTrees)
	{
		data->getTestData().setUpdater(&updater);

		auto compileThread = new snex::jit::JitNodeCompileThread(data);
		data->setCompileHandler(compileThread);

		provider = new snex::ui::ValueTreeCodeProvider(data);
		data->setCodeProvider(provider);

		playground = new snex::ui::SnexPlayground(data, true);
		playground->setReadOnly(true);
		
		addAndMakeVisible(parameters = new snex::ui::ParameterList(data));

		addAndMakeVisible(testData = new snex::ui::TestDataComponent(data));

		addAndMakeVisible(graph1 = new snex::ui::Graph(data));
		addAndMakeVisible(graph2 = new snex::ui::Graph(data));
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

		playground->setFullTokenProviders();
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
