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


struct spreader
{
	template <int P> void setParameter(double newDelta)
	{
		for (auto& v : delta)
			v = newDelta;
	}

	FORWARD_PARAMETER_TO_MEMBER(spreader);

	double getValue(int index, int numUsed, double inputValue)
	{
		auto n = (double)index / (double)numUsed + 0.5;

		n *= delta.get();
		
		return inputValue * n;
	}

	void createParameters(ParameterDataList& data)
	{
		{
			parameter::data p("Delta");
			p.callback = parameter::inner<spreader, 0>(*this);
			p.setRange({ 0.0, 1.0 });
			p.setDefaultValue(0.0);
			data.add(std::move(p));
		}
	}

	PolyData<double, NUM_POLYPHONIC_VOICES> delta;
};


namespace scriptnode {




namespace parameter {

template <class ParameterClass> struct dupli
{
	ParameterClass p;

	void call(int index, double v)
	{
		jassert(isPositiveAndBelow(index, *sizePtr));

		auto thisPtr = (uint8*)firstObj + index * objectDelta;

		p.setObjPtr(thisPtr);
		p.call(v);
	}

	int getNumVoices() const
	{
		jassert(sizePtr != nullptr);
		return *sizePtr;
	}

	template <int P, typename DupliRefType> void connect(DupliRefType& t)
	{
		sizePtr = t.sizePtr;
		objectDelta = t.objectDelta;
		firstObj = t.firstObj;
	}

	int* sizePtr = nullptr;
	void* firstObj = nullptr;
	size_t objectDelta;
};

}

namespace control
{
	
template <typename ParameterType, typename SpreadType> struct spread : public control::pimpl::no_processing,
	public control::pimpl::parameter_node_base<ParameterType>
{
	SN_GET_SELF_AS_OBJECT(spread);
	SET_HISE_NODE_ID("spread");

	void initialise(NodeBase* n)
	{
		if constexpr (prototypes::check::initialise<SpreadType>::value)
			obj.initialise(n);
	}

	template <int P> void setParameterStatic(void* obj, double v)
	{
		auto typed = static_cast<spread*>(obj);

		if (P == 0)
		{
			typed->setValue(v);
		}
		else
		{
			typed->obj.setParameter<P - 1>(v);

			sendValueToDuplicates(*lastValue.begin());
		}
	}

	PARAMETER_MEMBER_FUNCTION;

	void setValue(double v)
	{
		for (auto& s : lastValue)
			s = v;

		sendValueToDuplicates(v);
	}

	void sendValueToDuplicates(double v)
	{
		int numVoices = p.getNumVoices();

		for (int i = 0; i < numVoices; i++)
		{
			auto valueToSend = obj.getValue(i, numVoices, v);
			getParameter().call(i, valueToSend);
		}
	}

	void createParameters(ParameterDataList& data)
	{
		{
			DEFINE_PARAMETERDATA(spread, Value);
			p.setRange({ 0.0, 1.0 });
			p.setDefaultValue(0.0);
			data.add(std::move(p));
		}
		
		if constexpr (prototypes::check::createParameters<SpreadType>()::value)
			obj.createParameters(data);
	}

	SpreadType obj;
	PolyData<double, NUM_POLYPHONIC_VOICES> lastValue;

	JUCE_DECLARE_WEAK_REFERENCEABLE(spread);
};
		
}
}







//==============================================================================
MainComponent::MainComponent() :
	data(new ui::WorkbenchData())
{

	{
		using DupliType = parameter::duplichain<parameter::dupli<parameter::plain<core::oscillator, 1>>,
												parameter::dupli<parameter::plain<core::oscillator, 1>>>;


		using ChainType = container::chain<parameter::empty, core::oscillator, core::oscillator>;

		control::spread<DupliType, spreader> c;

		wrap::duplichain<ChainType> o;

		auto hn = o.get<0>();

		c.connect<0>(hn);
		c.connect<1>(o.get<1>());

		c.setValue(1000.0);
		c.setParameter<1>(0.4);
		


		int x = 5;
		
#if 0
		float data[1024];
		float* ch[2] = { data, data + 512 };

		ProcessData<2> pd(ch, 512, 2);

		mc.prepare(PrepareSpecs());
		mc.reset();
		mc.process(pd);
#endif
	}
	

	bool useValueTrees = false;
	
	addAndMakeVisible(funkSlider);
	funkSlider.setLookAndFeel(&laf);
	funkSlider.setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
	laf.setDefaultColours(funkSlider);


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

	funkSlider.setBounds(b.removeFromTop(148).removeFromLeft(228).reduced(50));
	funkSlider.setRange(0.0, 2.0, 0.01);
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
