/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option any later version.
*
*   HISE is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licences for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licencing:
*
*   http://www.hartinstruments.net/hise/
*
*   HISE is based on the JUCE library,
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/



#pragma once

#if USE_BACKEND || SNEX_STANDALONE_PLAYGROUND

namespace snex {
namespace jit {
using namespace juce;


struct CallbackStateComponent : public Component,
								public CallbackCollection::Listener
{
	CallbackStateComponent() :
		r("")
	{
		r.getStyleData().fontSize = 14.0f;
	}

	void initialised(const CallbackCollection& c) override
	{
		frameCallback = c.getBestCallback(CallbackCollection::FrameProcessing);
		blockCallback = c.getBestCallback(CallbackCollection::BlockProcessing);
		currentCallback = frameProcessing ? frameCallback : blockCallback;

		rebuild();
	}

	void paint(Graphics& g) override
	{
		g.setColour(Colours::white.withAlpha(0.8f));
		g.setFont(GLOBAL_BOLD_FONT());

		r.draw(g, getLocalBounds().reduced(5).toFloat());
	}

	juce::String getCallbackName() const
	{
		switch (currentCallback)
		{
		case CallbackTypes::Channel:	return "processChannel()";
		case CallbackTypes::Frame:		return "processFrame()";
		case CallbackTypes::Sample:	return "processSample()";
		default:							return "inactive";
		}
	}

	void setFrameProcessing(int mode)
	{
		frameProcessing = mode == CallbackCollection::ProcessType::FrameProcessing;
		currentCallback = frameProcessing ? frameCallback : blockCallback;

		rebuild();
	}

	void rebuild()
	{
		juce::String s;
		s << "**Samplerate**: " << juce::String(samplerate, 1) << "  ";
		s << "**Blocksize**: " << juce::String(blockSize) << "  ";
		s << "**NumChannels**: " << juce::String(numChannels) << "  ";
		s << "**Frame processing**: " << (frameProcessing ? "Yes" : "No") << "  ";
		s << "**Used Callback**: `" << getCallbackName() << "`";

		r.setNewText(s);
		auto h = r.getHeightForWidth((float)(getWidth() + 10), true);

		setSize(getWidth(), h + 10);

		repaint();
	}

	void prepare(double samplerate_, int blockSize_, int numChannels_) override
	{
		samplerate = samplerate_;
		blockSize = blockSize_;
		numChannels = numChannels_;
		
		MessageManager::callAsync([this]
		{
			rebuild();
		});
	}

	juce::String processSpecs;

	double samplerate = 0.0;
	int blockSize = 0;
	int numChannels = 0;

	int frameCallback = CallbackTypes::Inactive;
	int blockCallback = CallbackTypes::Inactive;

	int currentCallback = CallbackTypes::Inactive;
	bool frameProcessing = false;
	bool active = false;

	BreakpointHandler* handler = nullptr;
	MarkdownRenderer r;
};

struct SnexPathFactory: public hise::PathFactory
{
	juce::String getId() const override { return "Snex"; }
    Path createPath(const juce::String& id) const override;
};
  


/** Quick and dirty assembly syntax highlighter.

Definitely not standard conform (don't know nothing about assembly lol).
*/
class AssemblyTokeniser : public juce::CodeTokeniser
{
public:

	enum Tokens
	{
		Unknown,
		Comment,
		Location,
		Number,
		Label,
		Instruction
	};

	static SparseSet<int> applyDiff(const String& oldAsm, String& newAsm);

	int readNextToken(CodeDocument::Iterator& source) override;

	CodeEditorComponent::ColourScheme getDefaultColourScheme() override;
};

class BreakpointDataProvider : public ApiProviderBase,
							   public ApiProviderBase::Holder
{
public:

	BreakpointDataProvider(GlobalScope& m) :
		handler(m.getBreakpointHandler()),
		scope(m)
	{};

	ApiProviderBase* getProviderBase() override { return this; }

	int getNumDebugObjects() const override
	{
		return infos.size();
	}

	DebugInformationBase* getDebugInformation(int index)
	{
		return infos[index];
	}

	void getColourAndLetterForType(int type, Colour& colour, char& letter) override
	{
		ApiHelpers::getColourAndLetterForType(type, colour, letter);
	}

	void rebuild() override;

	OwnedArray<DebugInformationBase> infos;
	GlobalScope& scope;
	BreakpointHandler& handler;
};


/** A simple background thread that does the compilation. */
class BackgroundCompileThread: public Thread,
	public ui::WorkbenchData::CompileHandler
{
public:

	struct OwnedDataHolder : public ExternalDataHolder
	{
		OwnedDataHolder(PooledUIUpdater* updater_) :
			updater(updater_)
		{

		}

		Table* getTable(int index) override
		{
			if (isPositiveAndBelow(index, tables.size()))
			{
				return tables[index];
			}

			tables.add(new SampleLookupTable());
			return tables.getLast();
		}

		SliderPackData* getSliderPack(int index) override
		{
			if (isPositiveAndBelow(index, sliderPacks.size()))
			{
				return sliderPacks[index];
			}

			sliderPacks.add(new SliderPackData(nullptr, updater));
			return sliderPacks.getLast();
		}

		MultiChannelAudioBuffer* getAudioFile(int index) override
		{
			if (isPositiveAndBelow(index, buffers.size()))
			{
				return buffers[index];
			}

			buffers.add(new MultiChannelAudioBuffer(updater, AudioSampleBuffer()));
			return buffers.getLast();
		}

		int getNumDataObjects(ExternalData::DataType t) const override
		{
			if (t == ExternalData::DataType::SliderPack)
				return sliderPacks.size();
			if (t == ExternalData::DataType::Table)
				return tables.size();
			if (t == ExternalData::DataType::AudioFile)
				return buffers.size();
		}

	private:

		PooledUIUpdater* updater = nullptr;

		ReferenceCountedArray<Table> tables;
		ReferenceCountedArray<SliderPackData> sliderPacks;
		ReferenceCountedArray<MultiChannelAudioBuffer> buffers;
	};

	BackgroundCompileThread(ui::WorkbenchData::Ptr data_, PooledUIUpdater* updater) :
		Thread("SnexPlaygroundThread"),
		CompileHandler(data_)
	{
		if(updater != nullptr)
			ownHolder = new OwnedDataHolder(updater);

		getParent()->getGlobalScope().getBreakpointHandler().setExecutingThread(this);
		setPriority(4);
	}

	~BackgroundCompileThread()
	{
		stopThread(3000);
	}

	virtual JitCompiledNode::Ptr getLastNode() = 0;

	ui::WorkbenchData::CompileResult compile(const String& s) override
	{
		// You need to override that method...
		jassertfalse;
		return { };
	}

	void postCompile(ui::WorkbenchData::CompileResult& lastResult) override
	{
		// You need to override that method...
		jassertfalse;
	}

	

	bool triggerCompilation() override
	{
		getParent()->getGlobalScope().getBreakpointHandler().abort();

		if (isThreadRunning())
			stopThread(3000);

		startThread();

		return false;
	}

	void run() override
	{
		try
		{
			getParent()->handleCompilation();
		}
		catch (...)
		{
			jassertfalse;
		}
	}

protected:


	ScopedPointer<OwnedDataHolder> ownHolder;
};


class TestCompileThread : public BackgroundCompileThread
{
public:

	TestCompileThread(ui::WorkbenchData::Ptr data) :
		BackgroundCompileThread(data, nullptr)
	{
		updater = new PooledUIUpdater();
		ownHolder = new OwnedDataHolder(updater);

		updater->startTimer(30);
	}

	ui::WorkbenchData::CompileResult compile(const String& s) override;

	void postCompile(ui::WorkbenchData::CompileResult& lastResult) override;

	JitCompiledNode::Ptr getLastNode() override
	{
		if (lastTest != nullptr)
		{
			return lastTest->nodeToTest;
		}

		return nullptr;
	}

	void processTestParameterEvent(int parameterIndex, double value) override
	{
		jassert(lastTest != nullptr);
		jassert(lastTest->nodeToTest != nullptr);

		lastTest->nodeToTest->setParameterDynamic(parameterIndex, value);
	}

	void processTest(ProcessDataDyn& data) override
	{
		jassert(lastTest != nullptr);
		jassert(lastTest->nodeToTest != nullptr);

		lastTest->nodeToTest->process(data);
	}

	void prepareTest(PrepareSpecs ps) override
	{
		jassert(lastTest != nullptr);
		jassert(lastTest->nodeToTest != nullptr);

		lastTest->nodeToTest->prepare(ps);
		lastTest->nodeToTest->reset();
	}

private:

	AudioSampleBuffer empty;

	ScopedPointer<PooledUIUpdater> updater;
	ScopedPointer<JitFileTestCase> lastTest;
};


class JitNodeCompileThread : public BackgroundCompileThread
{
public:

	JitNodeCompileThread(ui::WorkbenchData::Ptr d, PooledUIUpdater* updater) :
		BackgroundCompileThread(d, updater)
	{
		
	};

#if 0
	void setTestBuffer(const AudioSampleBuffer& newBuffer)
	{
		auto& testBuffer = getParent()->getTestData().b;

		if (newBuffer.getNumChannels() != testBuffer.getNumChannels())
			getParent()->triggerRecompile();

		testBuffer.makeCopyOf(newBuffer);

		getParent()->triggerPostCompileActions();
	}
#endif

	void prepareTest(PrepareSpecs ps)
	{
		if (lastNode != nullptr)
			lastNode->prepare(ps);
	}

	void processTestParameterEvent(int parameterIndex, double value) override
	{
		if (lastNode != nullptr)
			lastNode->setParameterDynamic(parameterIndex, value);
	}

	void processTest(ProcessDataDyn& data) override
	{
		if (lastNode != nullptr)
			lastNode->process(data);
	}

	ui::WorkbenchData::CompileResult compile(const String& code)
	{
		auto p = getParent();

		lastResult = {};

		auto instanceId = p->getInstanceId();

		if (instanceId.isValid())
		{
			Compiler::Ptr cc = createCompiler();

			auto numChannels = getParent()->getTestData().testSourceData.getNumChannels();

			if (numChannels == 0)
				numChannels = 2;

			lastNode = new JitCompiledNode(*cc, code, instanceId.toString(), numChannels);

			lastResult.assembly = cc->getAssemblyCode();
			lastResult.compileResult = lastNode->r;
			lastResult.obj = lastNode->getJitObject();

			for (auto p : lastNode->getParameterList())
			{
				ui::WorkbenchData::CompileResult::DynamicParameterData d;
				d.data = p.data;
				d.f = (void(*)(double))p.function;
				lastResult.parameters.add(d);
			}

			return lastResult;
		}

		lastResult.compileResult = Result::fail("Didn't specify file");
		return lastResult;
	}

	void postCompile(ui::WorkbenchData::CompileResult& lastResult) override
	{
		if (lastNode != nullptr && lastResult.compiledOk())
		{
			lastNode->setExternalDataHolder(ownHolder);

			auto& testData = getParent()->getTestData();
			
			if (testData.shouldRunTest())
			{
				testData.initProcessing(512, 44100.0);
				testData.processTestData(getParent());

#if 0
				auto ps = testData.createPrepareSpecs();

				auto pd = testData.createProcessData();


				lastNode->prepare(ps);
				lastNode->reset();

				DBG("POST");

				ProcessDataDyn data(processedTestBuffer.getArrayOfWritePointers(), ps.blockSize, ps.numChannels);
				lastNode->process(data);
#endif
			}
		}
	}

	JitCompiledNode::Ptr getLastNode() override { return lastNode; }

private:

	ui::WorkbenchData::CompileResult lastResult;

	
	JitCompiledNode::Ptr lastNode;
};


class SnexPlayground : public ui::WorkbenchComponent,
	public CodeDocument::Listener,
	public BreakpointHandler::Listener,
	public mcl::GutterComponent::BreakpointListener
{
public:

	struct TestCodeProvider : public ui::WorkbenchData::CodeProvider
	{
		TestCodeProvider(SnexPlayground& p, const File& f_):
			CodeProvider(p.getWorkbench()),
			parent(p),
			f(f_)
		{}

		static String getTestTemplate();

		String loadCode() const override;

		bool saveCode(const String& s) override;

		Identifier getInstanceId() const override 
		{ 
			if (f.existsAsFile())
				return Identifier(f.getFileNameWithoutExtension());

			return Identifier("UnsavedTest");
		}

		SnexPlayground& parent;

		File f;
	};

	struct PreprocessorUpdater: public Timer,
								public CodeDocument::Listener,
		public snex::DebugHandler
	{
		PreprocessorUpdater(SnexPlayground& parent_):
			parent(parent_)
		{
			parent.doc.addListener(this);
		}

		void timerCallback() override;

		void codeDocumentTextInserted(const juce::String&, int) override
		{
			startTimer(1200);
		}

		void logMessage(int level, const juce::String& s);

		void codeDocumentTextDeleted(int, int) override
		{
			startTimer(1200);
		}

		~PreprocessorUpdater()
		{
			parent.doc.removeListener(this);
		}

		SparseSet<int> lastRange;

		SnexPlayground& parent;
	};

	void codeDocumentTextInserted(const juce::String& , int ) override
	{
		auto lineToShow = jmax(0, consoleContent.getNumLines() - console.getNumLinesOnScreen());
		console.scrollToLine(lineToShow);
	}

	void codeDocumentTextDeleted(int , int ) override
	{

	}

	void breakpointsChanged(mcl::GutterComponent& g) override
	{
		getWorkbench()->triggerRecompile();
	}

	void setFullTokenProviders();

	SnexPlayground(ui::WorkbenchData* data, bool addDebugComponents=false);

	~SnexPlayground();

	void paint(Graphics& g) override;
	void resized() override;

	void mouseDown(const MouseEvent& event) override;

	bool keyPressed(const KeyPress& k) override;
	void createTestSignal();

	bool preprocess(String& code) override
	{
		return editor.injectBreakpointCode(code);
	}

	struct Spacer : public Component
	{
		Spacer(const juce::String& n) :
			Component(n)
		{};

		void paint(Graphics& g) override
		{
			g.setColour(Colours::black.withAlpha(0.4f));
			g.fillAll();
			g.setColour(Colours::white);
			g.setFont(GLOBAL_BOLD_FONT());
			g.drawText(getName(), getLocalBounds().toFloat(), Justification::centred);
		}
	};

	void setReadOnly(bool shouldBeReadOnly)
	{
		isReadOnly = shouldBeReadOnly;
		editor.setReadOnly(!isReadOnly);
	}

	void updateTextFromCodeProvider()
	{
		auto c = getWorkbench()->getCode();
		doc.replaceAllContent(c);
		doc.clearUndoHistory();
	}

	
private:

	bool isReadOnly = false;

	ScopedPointer<ui::WorkbenchData::CodeProvider> previousProvider;

	File currentTestFile;

	int currentBreakpointLine = -1;

	juce::String currentParameter;
	std::function<void(void)> pendingParam;

	bool dirty = false;

	void recalculateInternal();

	struct ButtonLaf : public LookAndFeel_V3
	{
		void drawButtonBackground(Graphics& g, Button& b, const Colour& , bool over, bool down)
		{
			float alpha = 0.0f;

			if (over)
				alpha += 0.2f;
			if (down)
				alpha += 0.2f;

			if (b.getToggleState())
			{
				g.setColour(Colours::white.withAlpha(0.5f));
				g.fillRoundedRectangle(b.getLocalBounds().toFloat(), 3.0f);
			}

			g.setColour(Colours::white.withAlpha(alpha));
			g.fillRoundedRectangle(b.getLocalBounds().toFloat(), 3.0f);
		}

		void drawButtonText(Graphics&g, TextButton& b, bool , bool )
		{
			auto c = !b.getToggleState() ? Colours::white : Colours::black;
			g.setColour(c.withAlpha(0.8f));
			g.setFont(GLOBAL_BOLD_FONT());
			g.drawText(b.getButtonText(), b.getLocalBounds().toFloat(), Justification::centred);
		}
	} blaf;

	void logMessage(ui::WorkbenchData::Ptr d, int level, const juce::String& s) override;

	void eventHappened(BreakpointHandler* handler, BreakpointHandler::EventType type) override
	{
		currentBreakpointLine = *handler->getLineNumber();

		if (type == BreakpointHandler::Resume)
			currentBreakpointLine = -1;

		editor.setCurrentBreakline(currentBreakpointLine);

		bpProvider.rebuild();
	}

	void preCompile() override
	{
		auto& ed = editor.editor;
		ed.clearWarningsAndErrors();

		if (isReadOnly)
		{
			doc.replaceAllContent(getWorkbench()->getCode());
			doc.clearUndoHistory();
		}
		else
		{
			assemblyDoc.replaceAllContent({});
			consoleContent.replaceAllContent({});
			consoleContent.clearUndoHistory();
			editor.setCurrentBreakline(-1);
		}
	}

	/** Don't change the workbench in the editor. */
	void workbenchChanged(ui::WorkbenchData::Ptr, ui::WorkbenchData::Ptr) {}

	void recompiled(ui::WorkbenchData::Ptr p) override;

	void postPostCompile(ui::WorkbenchData::Ptr wb) override;

	const bool testMode;

	CodeDocument doc;
	mcl::TextDocument mclDoc;
	PreprocessorUpdater conditionUpdater;
	juce::CPlusPlusCodeTokeniser tokeniser;
	BreakpointDataProvider bpProvider;
	mcl::FullEditor editor;
	AssemblyTokeniser assemblyTokeniser;
	CodeDocument assemblyDoc;
	CodeEditorComponent assembly;
	bool saveTest = false;

	juce::Label resultLabel;

	DebugHandler::Tokeniser consoleTokeniser;
	CodeDocument consoleContent;
	CodeEditorComponent console;

    SnexPathFactory factory;
    Path snexIcon;

	TextButton showAssembly;
	TextButton showConsole;
	TextButton compileButton;
	TextButton resumeButton;
	TextButton showInfo;

	std::atomic<int> currentSampleIndex = { 0 };

	Spacer spacerAssembly;
	Spacer spacerInfo;
	Spacer spacerConsole;

	ScopedPointer<Component> stateViewer;

	Array<Range<int>> scopeRanges;
	ScopedPointer<TestCodeProvider> testProvider;
};

}
}

#endif