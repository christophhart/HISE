/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#include "MainComponent.h"

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




const char* emptyText = "class NativeJitClass\n" \
"{\n" \
"public:\n" \
"\n" \
"	void init()\n" \
"	{\n" \
"		// Setup the variables here.\n" \
"	};\n" \
"\n" \
"	void prepareToPlay(double sampleRate, int blockSize)\n" \
"	{\n" \
"		// Setup the playback configuration here\n" \
"	};\n" \
"\n" \
"	float process(float input)\n" \
"	{\n" \
"		// Define the processing here\n" \
"		return input;\n" \
"	};\n" \
"\n" \
"private:\n" \
"\n" \
"	// Define private variables here\n" \
"\n" \
"};";


//==============================================================================
MainContentComponent::MainContentComponent()
{

	String code;
	NewLine nl;

	code << "float lastValue = 0.0f;" << nl;
	code << "float a = 0.9f;" << nl;
	code << "float invA = 0.1f;" << nl;
	code << "float test(float input) {" << nl;
	code << "const float x1 = sinf(input);" << nl;
	code << "return x1 + input;};";

	ScopedPointer<NativeJITCompiler> compiler = new NativeJITCompiler(code, false);

	ScopedPointer<NativeJITScope> scope = compiler->compileAndReturnScope();




	if (scope != nullptr)
	{
		const Identifier test("test");

		auto f = scope->getCompiledFunction<float, float>(test);

		float t = f(1.0f);

		int x = 5;
	}
	else
	{
		const String m = compiler->getErrorMessage();
		int x = 5;
	}

	scope = nullptr;





#if !JUCE_DEBUG

	fileLogger = new FileLogger(File::getSpecialLocation(File::currentApplicationFile).getParentDirectory().getChildFile("UnitTests.log"), "Unit Test Log");
	Logger::setCurrentLogger(fileLogger);
#endif

	inBuffer.setSize(1, 2048);
	inFFTBuffer.setSize(1, 1024);
	tempBuffer.setSize(1, 4096);
	outBuffer.setSize(1, 2048);
	outFFTBuffer.setSize(1, 1024);

	inBuffer.clear();
	outBuffer.clear();

	commandManager = new ApplicationCommandManager();

	commandManager->setFirstCommandTarget(this);

	commandManager->registerAllCommandsForTarget(this);
	
	commandManager->getKeyMappings()->resetToDefaultMappings();

	addKeyListener(commandManager->getKeyMappings());

	setApplicationCommandManagerToWatch(commandManager);

	UnitTestRunner runner;
    
    runner.setAssertOnFailure(false);
    runner.runAllTests();
    
	doc = new CodeDocument();
	doc->replaceAllContent(emptyText);

	tokeniser = new CPlusPlusCodeTokeniser();

	addAndMakeVisible(editor = new CodeEditorComponent(*doc, tokeniser));
	addAndMakeVisible(messageBox = new Label());
	addAndMakeVisible(table = new TableListBox());
	addAndMakeVisible(testSignalSelector = new ComboBox());
	addAndMakeVisible(inDisplay = new BufferDisplay("Input"));
	addAndMakeVisible(inFFTDisplay = new BufferDisplay("Input FFT"));
	addAndMakeVisible(outDisplay = new BufferDisplay("Output"));
	addAndMakeVisible(outFFTDisplay = new BufferDisplay("Output FFT"));

	inFFTDisplay->setIsFFT();
	outFFTDisplay->setIsFFT();
	
    editor->setFont(Font(Font::getDefaultMonospacedFontName(), 15.0f, Font::plain));

#if JUCE_MAC
    MenuBarModel::setMacMainMenu(this);
#else
	addAndMakeVisible(menuBar = new MenuBarComponent(this));
#endif
    


    //editor->getDocument().replaceAllContent(String(emptyText));
    
	commandManager->registerAllCommandsForTarget(editor);

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
	

	messageBox->setFont(Font(Font::getDefaultMonospacedFontName(), 15, Font::plain));

    setSize (800, 700);
}

MainContentComponent::~MainContentComponent()
{
	commandManager = nullptr;

	editor = nullptr;
	doc = nullptr;
	tokeniser = nullptr;

	Logger::setCurrentLogger(nullptr);

	fileLogger = nullptr;


}

void MainContentComponent::paint (Graphics& /*g*/)
{
   
}

void MainContentComponent::resized()
{
#if JUCE_MAC
    const int menuOffset = 0;
#else
	const int menuOffset = 22;

	menuBar->setBounds(0, 0, getWidth(), menuOffset);
#endif
    
	const int editorHeight = showAnalysis ? getHeight() / 2 : getHeight();

	editor->setBounds(0, menuOffset, getWidth()-200, editorHeight - 20 - menuOffset);
	table->setBounds(getWidth() - 200, menuOffset, 200, editorHeight - 20- menuOffset);
	messageBox->setBounds(0, editorHeight - 20, getWidth(), 20);
	
	if (showAnalysis)
	{
		const int y = editorHeight;

		const int w = showInput ? getWidth() / 2 : getWidth();
		const int h = showFFT ? getHeight() / 4 : getHeight() / 2;

		const int xOffset = showInput ? w : 0;

		inDisplay->setVisible(showInput);
		inFFTDisplay->setVisible(showInput && showFFT);
		outFFTDisplay->setVisible(showFFT);
		outDisplay->setVisible(true);

		inDisplay->setBounds(0, y, w, h);
		inFFTDisplay->setBounds(0, y + h, w, h);

		outDisplay->setBounds(xOffset, y, w, h);
		outFFTDisplay->setBounds(xOffset, y + h, w, h);
	}
	else
	{
		inDisplay->setVisible(false);
		inFFTDisplay->setVisible(false);
		outDisplay->setVisible(false);
		outFFTDisplay->setVisible(false);
	}

}

void MainContentComponent::buttonClicked(Button* /*b*/)
{
	
}

void MainContentComponent::getAllCommands(Array<CommandID>& commands)
{
	const NativeJITCommandID id[] = {
		NativeJITCommandID::FileNew,
		NativeJITCommandID::FileSave,
		NativeJITCommandID::FileLoad,
		NativeJITCommandID::ViewShowAnalysis,
		NativeJITCommandID::ViewShowFFT,
		NativeJITCommandID::ViewShowInput,
		NativeJITCommandID::TestSignalNoise,
		NativeJITCommandID::TestSignalSine,
		NativeJITCommandID::TestSignalSaw,
		NativeJITCommandID::TestSignalSquare,
		NativeJITCommandID::TestSignalDirac,
		NativeJITCommandID::Compile
	};

	commands.addArray(id, numElementsInArray(id));
}

void MainContentComponent::getCommandInfo(CommandID commandID, ApplicationCommandInfo &result)
{
	NativeJITCommandID cid = (NativeJITCommandID)commandID;

	switch (cid)
	{
	case MainContentComponent::NativeJITCommandID::FileNew:
		setCommandTarget(result, "New File", true, false, 'N', true, ModifierKeys::commandModifier);
		break;
	case MainContentComponent::NativeJITCommandID::FileSave:
		setCommandTarget(result, "Save File", true, false, 'S', true, ModifierKeys::commandModifier);
		break;
	case MainContentComponent::NativeJITCommandID::FileLoad:
		setCommandTarget(result, "Load File", true, false, 'L', true, ModifierKeys::commandModifier);
		break;
	case MainContentComponent::NativeJITCommandID::TestSignalNoise:
		setCommandTarget(result, "Create Noise Test Signal", true, false, 'X', false);
		break;
	case MainContentComponent::NativeJITCommandID::TestSignalSine:
		setCommandTarget(result, "Create Sine Test Signal", true, false, 'X', false);
		break;
	case MainContentComponent::NativeJITCommandID::TestSignalSaw:
		setCommandTarget(result, "Create Saw Test Signal", true, false, 'X', false);
		break;
	case MainContentComponent::NativeJITCommandID::TestSignalDirac:
		setCommandTarget(result, "Create Dirac Test Signal", true, false, 'X', false);
		break;
	case MainContentComponent::NativeJITCommandID::TestSignalSquare:
		setCommandTarget(result, "Create Square Test Signal", true, false, 'X', false);
		break;
	case MainContentComponent::NativeJITCommandID::ViewShowAnalysis:
		setCommandTarget(result, "Show Test Signal Display", true, showAnalysis, 'X', false);
		break;
	case MainContentComponent::NativeJITCommandID::ViewShowInput:
		setCommandTarget(result, "Show Input Test Signal", true, showInput, 'X', false);
		break;
	case MainContentComponent::NativeJITCommandID::ViewShowFFT:
		setCommandTarget(result, "Show Test Signal FFT", true, showFFT, 'X', false);
		break;
	case MainContentComponent::NativeJITCommandID::Compile:
		setCommandTarget(result, "Compile", true, false, 'x', false);
		result.addDefaultKeypress(KeyPress::F5Key, ModifierKeys::noModifiers);
		break;
	case MainContentComponent::NativeJITCommandID::numCommands:
		break;
	default:
		break;
	}
}



bool MainContentComponent::perform(const InvocationInfo &info)
{
	NativeJITCommandID cid = (NativeJITCommandID)info.commandID;

	switch (cid)
	{
	case MainContentComponent::NativeJITCommandID::FileNew:
		editor->getDocument().replaceAllContent(String(emptyText));
		return true;
	case MainContentComponent::NativeJITCommandID::FileSave:
		break;
	case MainContentComponent::NativeJITCommandID::FileLoad:
		break;
	case MainContentComponent::NativeJITCommandID::TestSignalNoise:
		currentSignal = BufferDisplay::Noise;
		calcFFTAndSetBuffers();
		return true;
	case MainContentComponent::NativeJITCommandID::TestSignalSine:
		currentSignal = BufferDisplay::Sine;
		calcFFTAndSetBuffers();
		return true;
	case MainContentComponent::NativeJITCommandID::TestSignalSaw:
		currentSignal = BufferDisplay::Saw;
		calcFFTAndSetBuffers();
		return true;
	case MainContentComponent::NativeJITCommandID::TestSignalDirac:
		currentSignal = BufferDisplay::DiracTrain;
		calcFFTAndSetBuffers();
		return true;
	case MainContentComponent::NativeJITCommandID::TestSignalSquare:
		currentSignal = BufferDisplay::Square;
		calcFFTAndSetBuffers();
		return true;
	case MainContentComponent::NativeJITCommandID::Compile:
		compileAndRun();
		return true;
	case MainContentComponent::NativeJITCommandID::numCommands:
		break;
	case MainContentComponent::NativeJITCommandID::ViewShowAnalysis:
		showAnalysis = !showAnalysis;
		commandManager->commandStatusChanged();
		resized();
		return true;
	case MainContentComponent::NativeJITCommandID::ViewShowInput:
		showInput = !showInput;
		commandManager->commandStatusChanged();
		resized();
		return true;
	case MainContentComponent::NativeJITCommandID::ViewShowFFT:
		showFFT = !showFFT;
		commandManager->commandStatusChanged();
		resized();
		return true;
	default:
		break;
	}

	return false;
}


void MainContentComponent::calcFFTAndSetBuffers(bool input/*=true*/)
{
	

	if (input)
	{
		BufferDisplay::fillBufferWithTestSignal(inBuffer, currentSignal, (double)currentFreq);
		createFFT(true);
		inFFTDisplay->setBuffer(inFFTBuffer);
		inDisplay->setBuffer(inBuffer);
		run();
	}
	else
	{
		createFFT(false);
		outFFTDisplay->setBuffer(outFFTBuffer);
		outDisplay->setBuffer(outBuffer);
	}

	
}

void MainContentComponent::compileAndRun()
{
	const String code = doc->getAllContent();

	compiler = new NativeJITCompiler(code);
	module = new NativeJITDspModule(compiler);

	if (!compiler->wasCompiledOK())
	{
		stopTimer();
		messageBox->setText(compiler->getErrorMessage(), dontSendNotification);
		return;
	}

	startTimer(500);

	module->enableOverflowCheck(true);

	run();
}

void MainContentComponent::run()
{
	if (compiler == nullptr)
	{
		return;
	}

	if (!module->allOK())
	{
		messageBox->setText("Not all functions are defined.", dontSendNotification);
		return;
	}

	FloatVectorOperations::copy(outBuffer.getWritePointer(0), inBuffer.getReadPointer(0), inBuffer.getNumSamples());

	try
	{
		module->init();

		module->prepareToPlay(44100.0, outBuffer.getNumSamples());

		double start = Time::getMillisecondCounterHiRes();

		module->processBlock(outBuffer.getWritePointer(0), outBuffer.getNumSamples());

		double end = Time::getMillisecondCounterHiRes();
		double delta = end - start;

		double timeframeMs = 1000.0 * (double)outBuffer.getNumSamples() / 44100.0;


		calcFFTAndSetBuffers(false);

		messageBox->setText("Compiled OK. Performance: " + String(timeframeMs / delta, 1) + "x realtime", dontSendNotification);
	}
	catch (String e)
	{
		messageBox->setText(e, dontSendNotification);
	}
}
