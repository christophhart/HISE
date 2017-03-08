/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#include "MainComponent.h"

#include <map>


const char* emptyText = "// Uncomment this line to disable bounds checking\n" \
"//#define DISABLE_SAFE_BUFFER_ACCESS 1\n" \
"\n" \
"void init()\n" \
"{\n" \
"    \n" \
"};\n" \
"\n" \
"void prepareToPlay(double sampleRate, int blockSize)\n" \
"{\n" \
"    \n" \
"};\n" \
"\n" \
"float process(float input)\n" \
"{\n" \
"    return 1.0f;\n" \
"};";


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
    
    void prepareToPlay(double sampleRate, int samplesPerBlock)
    {
        
    }
    
    float process(float /*input*/)
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
	
	doc = new CodeDocument();

	tokeniser = new CPlusPlusCodeTokeniser();

	addAndMakeVisible(editor = new CodeEditorComponent(*doc, tokeniser));
	addAndMakeVisible(runButton = new TextButton("Compile & Run"));
	addAndMakeVisible(messageBox = new Label());
	addAndMakeVisible(table = new TableListBox());

    editor->getDocument().replaceAllContent(String(emptyText));
    
	tableModel = new VariableTableModel(this);

	table->setModel(tableModel);
	

	table->getHeader().setSize(getWidth(), 22);

	// give it a border
	table->setOutlineThickness(1);

	table->getViewport()->setScrollBarsShown(true, false, false, false);

	table->getHeader().setInterceptsMouseClicks(true, true);

	table->getHeader().addColumn("Type", 1, 50);
	table->getHeader().addColumn("Name", 2, 100);
	table->getHeader().addColumn("Value", 3, 50);
	

	runButton->addListener(this);

	messageBox->setFont(Font(Font::getDefaultMonospacedFontName(), 15, Font::plain));

    setSize (800, 600);
}

MainContentComponent::~MainContentComponent()
{
	editor = nullptr;
	doc = nullptr;
	tokeniser = nullptr;

}

void MainContentComponent::paint (Graphics& /*g*/)
{
   
}

void MainContentComponent::resized()
{
	editor->setBounds(0, 0, getWidth()-200, getHeight()-40);
	table->setBounds(getWidth() - 200, 0, 200, getHeight() - 40);
	messageBox->setBounds(0, getHeight() - 40, getWidth(), 20);
	runButton->setBounds(0, getHeight()-20, getWidth(), 20);
}

void MainContentComponent::buttonClicked(Button* /*b*/)
{
	const String code = doc->getAllContent();

	compiler = new NativeJITCompiler(code);
	module = new NativeJITDspModule(compiler);

	if(!compiler->wasCompiledOK())
	{
		stopTimer();
		messageBox->setText(compiler->getErrorMessage(), dontSendNotification);
		return;
	}

	startTimer(500);

	float data[2048];
	FloatVectorOperations::fill(data, 0.5f, 2048);

	module->enableOverflowCheck(true);

	try
	{
		module->init();

		module->prepareToPlay(44100.0, 128);

		double start = Time::getMillisecondCounterHiRes();

		module->processBlock(data, 2048);

		double end = Time::getMillisecondCounterHiRes();

		double delta = end - start;

		HardcodedDspModule hc;

		double hstart = Time::getMillisecondCounterHiRes();

		hc.processBlock(data, 2048);

		double hend = Time::getMillisecondCounterHiRes();

		double hdelta = hend - hstart;

		float d = 1.0f;

		module->processBlock(&d, 1);

		messageBox->setText("Result: " + String(d, 4) + ", JIT: " + String(delta, 5) + " ms, Native: " + String(hdelta, 5) + " ms", dontSendNotification);
	}
	catch (String e)
	{
		messageBox->setText(e, dontSendNotification);
	}
}
