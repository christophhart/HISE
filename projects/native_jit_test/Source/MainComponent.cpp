/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#include "MainComponent.h"

#include "Parser.h"

#include <map>



struct LP
{
	float lastValue = 0.0f;

	float process(float input)
	{
		const float a = 0.99f;
		const float invA = 1.0f - a;

		const float thisValue = a*lastValue + invA*input;
		lastValue = thisValue;

		return thisValue;
	}
};



//==============================================================================
MainContentComponent::MainContentComponent()
{

	ScopedPointer<GlobalBase> b = GlobalBase::create<double>("dudel");

	GlobalBase::store<double>(b, 24.8);

	jassert(GlobalBase::get<double>(b) == 24.8);

	

#if 0

	LP filter;

	filter.process(1.0f);
	filter.process(0.5f);

	const float realOutput = filter.process(0.75f);

	LP filter2;

	NativeJIT::ExecutionBuffer codeAllocator(8192);
	NativeJIT::Allocator allocator(8192);

	NativeJIT::FunctionBuffer c(codeAllocator, 8192);

	NativeJIT::Function<float, float> f1(allocator, c);

	std::map<Identifier, NativeJIT::NodeBase*> lines;


	auto& e24 = f1.StackVariable<float>();

	
	auto& e25 = f1.Immediate(storeInStack<float>);


	
	auto& e36 = f1.Call(e25, e24, f1.Immediate(2.0f));

	auto& wrapped = f1.Dependent(e36, e24);

	auto& e7 = f1.Mul(f1.Immediate(1.0f), f1.Deref(e24));

	

	auto f = f1.Compile(e7);

	auto v = f(1.0f);



	auto lastInsert = lines.insert(std::make_pair("a", &f1.Immediate(0.99f)));

	

	lastInsert = lines.insert(std::make_pair("b", &f1.Immediate(24.f)));

	lastInsert = lines.insert(std::make_pair("invA", &f1.Sub(f1.Immediate(1.0f), *dynamic_cast<NativeJIT::Node<float>*>(lines["a"]))));


	auto& e3a = f1.Immediate(&filter2);
	auto& e3b = f1.FieldPointer(e3a, &LP::lastValue);
	auto& e3c = f1.Deref(e3b);
	auto& e3d = f1.Mul(e3c, *dynamic_cast<NativeJIT::Node<float>*>(lines["a"]));
	auto& e3e = f1.Mul(*dynamic_cast<NativeJIT::Node<float>*>(lines["invA"]), f1.GetP1());
	auto& e3f = f1.Add(e3d, e3e);

	lastInsert = lines.insert(std::make_pair("thisValue", &f1.Dependent(e3f, *dynamic_cast<NativeJIT::Node<float>*>(lines["invA"]))));
	
	auto& e4 = f1.Call(f1.Immediate(storeInStack<float>), e3b, *dynamic_cast<NativeJIT::Node<float>*>(lines["thisValue"]));

	lastInsert = lines.insert(std::make_pair("l1", &f1.Dependent(e4, *dynamic_cast<NativeJIT::Node<float>*>(lines["thisValue"]))));

	

	jitProcess(1.0f);
	jitProcess(0.5f);
	const float jitOutput = jitProcess(0.75f);

	jassert(realOutput == jitOutput);

	

	Random r;
	
	float realData[88200];

	for (int i = 0; i < 88200; i++)
	{
		realData[i] = r.nextFloat();
	}

	filter.lastValue = 0.0f;
	filter2.lastValue = 0.0f;

	float jitData[88200];

	FloatVectorOperations::copy(jitData, realData, 88200);

	PerformanceCounter pc1("Compiled", 1, File("D:\\p.log"));

	pc1.start();
	for (int i = 0; i < 88200; i++)
	{
		realData[i] = filter.process(realData[i]);
	}
	pc1.stop();
	

	PerformanceCounter pc2("JIT", 1, File("D:\\p.log"));

	pc2.start();
	for (int i = 0; i < 88200; i++)
	{
		jitData[i] = jitProcess(jitData[i]);
	}
	pc2.stop();
	
#endif

	doc = new CodeDocument();

	tokeniser = new CPlusPlusCodeTokeniser();

	addAndMakeVisible(editor = new CodeEditorComponent(*doc, tokeniser));
	addAndMakeVisible(runButton = new TextButton("Compile & Run"));
	addAndMakeVisible(messageBox = new Label());

	runButton->addListener(this);

	messageBox->setFont(Font(Font::getDefaultMonospacedFontName(), 15, Font::plain));

    setSize (600, 400);
}

MainContentComponent::~MainContentComponent()
{
	editor = nullptr;
	doc = nullptr;
	tokeniser = nullptr;

}

void MainContentComponent::paint (Graphics& g)
{
   
}

void MainContentComponent::resized()
{
	editor->setBounds(0, 0, getWidth(), getHeight()-40);
	messageBox->setBounds(0, getHeight() - 40, getWidth(), 20);
	runButton->setBounds(0, getHeight()-20, getWidth(), 20);
}

void MainContentComponent::buttonClicked(Button* b)
{
	const String code = doc->getAllContent();

	
	NativeJIT::ExecutionBuffer codeAllocator(8192);
	NativeJIT::Allocator allocator(8192);
	NativeJIT::FunctionBuffer fb(codeAllocator, 8192);

	try
	{
		FunctionParser<float, float> l1(code, allocator, fb);

		l1.parseFunctionBody();

		auto l = l1.compileFunction();

		const float result = l(2.0f);

		messageBox->setText("Result: " + String(result), dontSendNotification);

	}
	catch (String s)
	{
		messageBox->setText(s, dontSendNotification);
		return;
	}

	

	
}
