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

	BackgroundCompileThread(ui::WorkbenchData::Ptr data_) :
		Thread("SnexPlaygroundThread"),
		CompileHandler(data_)
	{
		getParent()->getGlobalScope().getBreakpointHandler().setExecutingThread(this);

		setPriority(4);
	}

	~BackgroundCompileThread()
	{
		stopThread(3000);
	}

	ui::WorkbenchData::CompileResult compile(const String& s) override;

	void postCompile(ui::WorkbenchData::CompileResult& lastResult) override;

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


	ScopedPointer<JitFileTestCase> lastTest;
};



class SnexPlayground : public ui::WorkbenchComponent,
	public ui::WorkbenchData::CodeProvider,
	public CodeDocument::Listener,
	public BreakpointHandler::Listener,
	public mcl::GutterComponent::BreakpointListener
{
public:

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

	SnexPlayground(ui::WorkbenchData* data, bool addDebugComponents=false);

	~SnexPlayground();

	void paint(Graphics& g) override;
	void resized() override;

	void mouseDown(const MouseEvent& event) override;

	bool keyPressed(const KeyPress& k) override;
	void createTestSignal();

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

	String loadCode() const override;

	bool preprocess(String& code) override
	{
		if(testMode)
			return editor.injectBreakpointCode(code);

		return false;
	}

	bool saveCode(const String& s) override;

	

private:

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

		assemblyDoc.replaceAllContent({});
		consoleContent.replaceAllContent({});
		consoleContent.clearUndoHistory();
		editor.setCurrentBreakline(-1);
	}

	/** Don't change the workbench in the editor. */
	void workbenchChanged(ui::WorkbenchData::Ptr, ui::WorkbenchData::Ptr) {}

	void recompiled(ui::WorkbenchData::Ptr p) override;

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

	Compiler::Tokeniser consoleTokeniser;
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
};

}
}

#endif