/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#include "MainComponent.h"

#include "Parser.h"

#include <map>


class HardcodedDspModule
{
public:
    
    HardcodedDspModule()
    {
        init();
    }
    
    double uptime = 0.0;
    double uptimeDelta = 0.01;
    
    float a1;
    float a2;
    float a3;
    float a4;
    
    void init()
    {
        a1 = 1.0f;
        a2 = 0.8f;
        a3 = 0.6f;
        a4 = 0.3f;
    };
    
    float process(float input)
    {
        const float x1 = a1 * sinf((float)uptime);
        const float x2 = a2 * sinf(2.0f * (float)uptime);
        const float x3 = a3 * sinf(3.0f * (float)uptime);
        const float x4 = a4 * sinf(4.0f * (float)uptime);
        
        uptime += uptimeDelta;
        
        return x1 + x2 + x3 + x4;
    };
    
    void processBlock(float* data, int numSamples)
    {
        for (int i = 0; i < numSamples; i++)
        {
            data[i] = process(data[i]);
        }
    };
    
};

class JitDspModule: public DynamicObject
{
public:

	typedef float(*processFunction)(float);

	typedef int(*initFunction)();

	typedef int(*prepareFunction)(double, int);

	JitDspModule(const String& code)
	{
		setProperty("code", code);
		setProperty("scope", new GlobalScope());
	}

	void compile()
	{
		const String code = getProperty("code");

		auto scope = dynamic_cast<GlobalScope*>(getProperty("scope").getDynamicObject());

		GlobalParser parser(code, scope);

		parser.parseStatementList();

		pf = scope->getCompiledFunction1<float, float>("process");
		
		//pp = scope->getCompiledFunction2<int, double, int>("prepareToPlay");
		
		initf = scope->getCompiledFunction0<int>("init");
       
        
        compiledOk = pf != nullptr;// && pp != nullptr;
	}

	void prepareToPlay(double sampleRate, int samplesPerBlock)
	{
		
	}

	void init()
	{
		if (initf != nullptr)
		{
			initf();
		}
	}

	void processBlock(float* data, int numSamples)
	{
		if (compiledOk)
		{
			for (int i = 0; i < numSamples; i++)
			{
				data[i] = pf(data[i]);
			}
		}
	}

private:

	processFunction pf;

	prepareFunction pp;

	initFunction initf;

	bool compiledOk = false;
};

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

struct Buffer
{
    float getSample(Buffer& b, int index)
    {
#if 0
#if !ENABLE_SCRIPTING_SAFECHECKS
        return b.buffer.buffer.getReadPointer()[index];
#else
        return b.buffer.get() != nullptr ? b.getSample(index) : 0.0f;
#endif
#endif
        return 0.0f;
    };
    
    template<typename R> R setSample(Buffer& b, int index, float value)
    {
#if 0
#if !ENABLE_SCRIPTING_SAFECHECKS
        b.buffer.buffer.getWritePointer()[index] = value;
#else
        if(b.buffer.get() != nullptr) b.setSample(index, value);
#endif
#endif
        
        return R();
    }
    
    DynamicObject::Ptr buffer;
};



//==============================================================================
MainContentComponent::MainContentComponent()
{
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

	


	auto& e24 = f1.StackVariable<double>();

	
	auto& e25 = f1.Immediate(storeInStack<double>);


	
	auto& e36 = f1.Call(e25, e24, f1.Immediate(2.0));

#if 0

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

	try
	{
        float data[2048];
        FloatVectorOperations::fill(data, 0.5f, 2048);

        PreprocessorParser preprocessor(code);
        
		JitDspModule module(preprocessor.process());

		
		module.compile();

        module.init();

        
		module.prepareToPlay(44100.0, 128);


		double start = Time::getMillisecondCounterHiRes();

		module.processBlock(data, 2048);

		double end = Time::getMillisecondCounterHiRes();

		double delta = end - start;
        
        HardcodedDspModule hc;
        
        double hstart = Time::getMillisecondCounterHiRes();

        hc.processBlock(data, 2048);
        
        double hend = Time::getMillisecondCounterHiRes();
        
        double hdelta = hend - hstart;
        
        float d = 1.0f;
        
        module.processBlock(&d, 1);
        
        messageBox->setText("Result: " + String(d, 4) + ", JIT: " + String(delta, 5) + " ms, Native: " + String(hdelta, 5) + " ms", dontSendNotification);

	}
	catch (String s)
	{
		messageBox->setText(s, dontSendNotification);
		return;
	}

	

	
}
