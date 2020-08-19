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

		setSize(getWidth(), roundToInt(h + 10.f));

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
    
struct Graph : public Component
{
	bool barebone = false;
	int boxWidth = 128;

	struct InternalGraph : public Component,
						   public Timer
	{
		void paint(Graphics& g) override;
		
		void timerCallback() override
		{
			stopTimer();
			repaint();
		}

		void setBuffer(AudioSampleBuffer& b);

		void calculatePath(Path& p, AudioSampleBuffer& b, int channel);

		void mouseMove(const MouseEvent& e) override
		{
			currentPoint = e.getPosition();
			startTimer(1200);
			repaint();
		}

		void mouseExit(const MouseEvent&) override
		{
			stopTimer();
			currentPoint = {};
			repaint();
		}

		void mouseWheelMove(const MouseEvent& e, const MouseWheelDetails& wheel) override
		{
			if (e.mods.isAnyModifierKeyDown())
			{
				zoomFactor = jlimit(1.0f, 32.0f, zoomFactor + (float)wheel.deltaY * 5.0f);
				findParentComponentOfClass<Graph>()->resized();
				setBuffer(lastBuffer);
			}
			else
				getParentComponent()->mouseWheelMove(e, wheel);
		}

		bool isHiresMode() const
		{
			return pixelsPerSample > 10.0f;
		}

		int getYPixelForSample(int sample)
		{
			auto x = (float)getPixelForSample(sample);
			Line<float> line(x, 0.0f, x, (float)getHeight());
			return roundToInt(l.getClippedLine(line, true).getEndY());
		}

		int getPixelForSample(int sample)
		{
			if (lastBuffer.getNumSamples() == 0)
				return 0;

			Path::Iterator iter(l);

			auto asFloat = (float)sample / (float)lastBuffer.getNumSamples();
			asFloat *= (float)getWidth();

			while (iter.next())
			{
				if (iter.x1 >= asFloat)
					return roundToInt(iter.x1);
			}

			return 0;
		}

		AudioSampleBuffer lastBuffer;

		float pixelsPerSample = 1;
		
		Point<int> currentPoint;

		int numSamples = 0;
		int currentPosition = 0;
		Path l;
		Path r;

		Range<float> leftPeaks;
		Range<float> rightPeaks;
		bool stereoMode = false;
		
		float zoomFactor = 1.0f;

	} internalGraph;

	

	Viewport viewport;

	Graph(bool barebone_=false) :
		testSignal("TestSignal"),
		channelMode("ChannelMode"),
		barebone(barebone_),
		boxWidth(barebone ? 0 : 128)
	{
		if (!barebone)
		{
			addAndMakeVisible(testSignal);
			skin(testSignal);
			testSignal.setTextWhenNothingSelected("Select test signal");
			testSignal.addItemList({ "Noise", "Ramp", "Fast Ramp" }, 1);

			addAndMakeVisible(processingMode);
			skin(processingMode);
			processingMode.setTextWhenNothingSelected("Select processing");
			processingMode.addItemList({ "Frame", "Block" }, 1);

			addAndMakeVisible(channelMode);
			skin(channelMode);
			channelMode.setTextWhenNothingSelected("Select channel mode");
			channelMode.addItemList({ "Mono", "Stereo" }, 1);

			addAndMakeVisible(bufferLength);
			skin(bufferLength);
			bufferLength.setTextWhenNothingSelected("Select Buffer size");
			bufferLength.addItemList({ "16", "512", "44100" }, 1);

			viewport.setViewedComponent(&internalGraph, false);

			addAndMakeVisible(viewport);
		}
	}

	void paint(Graphics& g)
	{
		auto b = getLocalBounds().removeFromRight(50);

		b = b.removeFromTop(viewport.getMaximumVisibleHeight());

		g.setColour(Colours::white);

		if (internalGraph.stereoMode)
		{
			auto left = b.removeFromTop(b.getHeight() / 2).toFloat();
			auto right = b.toFloat();

			auto lMax = left.removeFromTop(18);
			auto lMin = left.removeFromBottom(18);

			g.drawText(juce::String(internalGraph.leftPeaks.getStart(), 1), lMin, Justification::left);
			g.drawText(juce::String(internalGraph.leftPeaks.getEnd(), 1), lMax, Justification::left);

			auto rMax = right.removeFromTop(18);
			auto rMin = right.removeFromBottom(18);

			g.drawText(juce::String(internalGraph.rightPeaks.getStart(), 1), rMin, Justification::left);
			g.drawText(juce::String(internalGraph.rightPeaks.getEnd(), 1), rMax, Justification::left);
		}
		else
		{
			auto left = b.removeFromTop(b.getHeight()).toFloat();

			auto lMax = left.removeFromTop(18);
			auto lMin = left.removeFromBottom(18);

			g.drawText(juce::String(internalGraph.leftPeaks.getStart(), 1), lMin, Justification::left);
			g.drawText(juce::String(internalGraph.leftPeaks.getEnd(), 1), lMax, Justification::left);
		}
	}

	void skin(ComboBox& b)
	{
		b.setColour(hise::HiseColourScheme::ComponentBackgroundColour, Colour(0));
		b.setColour(hise::HiseColourScheme::ComponentFillTopColourId, Colour(0));
		b.setColour(hise::HiseColourScheme::ComponentFillBottomColourId, Colour(0));
		b.setColour(hise::HiseColourScheme::ComponentOutlineColourId, Colour(0));
		b.setColour(hise::HiseColourScheme::ComponentTextColourId, Colours::white);
		b.setColour(ComboBox::ColourIds::backgroundColourId, Colour(0xFF444444));
		b.setColour(ComboBox::ColourIds::textColourId, Colours::white);
		b.setColour(ComboBox::ColourIds::arrowColourId, Colours::white);
		b.setColour(ComboBox::ColourIds::outlineColourId, Colours::transparentBlack);
	}

	void resized() override
	{
		auto b = getLocalBounds();
		b.removeFromRight(60);

		if (!barebone)
		{
			auto boxBounds = b.removeFromLeft(boxWidth);

			testSignal.setBounds(boxBounds.removeFromTop(30));
			channelMode.setBounds(boxBounds.removeFromTop(30));
			bufferLength.setBounds(boxBounds.removeFromTop(30));
			processingMode.setBounds(boxBounds.removeFromTop(30));
		}

		internalGraph.setBounds(0, 0, roundToInt((float)viewport.getWidth() * internalGraph.zoomFactor), viewport.getMaximumVisibleHeight());
		viewport.setBounds(b);
		internalGraph.setBounds(0, 0, roundToInt((float)viewport.getWidth() * internalGraph.zoomFactor), viewport.getMaximumVisibleHeight());

		repaint();
	}

	void setBuffer(AudioSampleBuffer& b)
	{
		resized();
		internalGraph.setBuffer(b);
	}
	
	void setCurrentPosition(int newPos)
	{
		internalGraph.currentPosition = newPos;
		repaint();
	}

	ComboBox testSignal;
	ComboBox channelMode;
	ComboBox bufferLength;
	ComboBox processingMode;
};

/** Quick and dirty assembly syntax highlighter.

Definitely not standard conform (don't know nothing about assembly lol).
*/
class AssemblyTokeniser : public juce::CodeTokeniser
{
	enum Tokens
	{
		Unknown,
		Comment,
		Location,
		Number,
		Label,
		Instruction
	};

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

class SnexPlayground : public Component,
	public ComboBox::Listener,
	public DebugHandler,
	public ApiProviderBase::Holder,
	public CodeDocument::Listener,
	public BreakpointHandler::Listener
{
public:

	ValueTree createApiTree() override 
	{ 
		return cData.obj.createValueTree();
	}

	void eventHappened(BreakpointHandler* handler, BreakpointHandler::EventType type) override
	{
		currentBreakpointLine = (int)*handler->getLineNumber();

		if (type == BreakpointHandler::Resume)
			currentBreakpointLine = -1;

		resumeButton.setEnabled(type == BreakpointHandler::Break);
		graph.setBuffer(b);
		graph.setCurrentPosition(currentSampleIndex);

		juce::String s;

		s << "Line " << currentBreakpointLine;
		s << ": Execution paused at ";

		if (currentParameter.isNotEmpty())
			s << "parameter callback " << currentParameter;
		else
			s << juce::String(currentSampleIndex.load());

		resultLabel.setText(s, dontSendNotification);
		editor.repaint();
		bpProvider.rebuild();
		resized();
	}

	ApiProviderBase* getProviderBase() override { return &cData.obj; }

	void codeDocumentTextInserted(const juce::String& , int ) override
	{
		auto lineToShow = jmax(0, consoleContent.getNumLines() - console.getNumLinesOnScreen());
		console.scrollToLine(lineToShow);
	}

	void handleBreakpoints(const Identifier& /*codeFile*/, Graphics& g, Component* /*c*/) override
	{
		if (currentBreakpointLine > 0)
		{
			int firstLine = editor.getFirstLineOnScreen();
			int lastLine = firstLine + editor.getNumLinesOnScreen();

			if (currentBreakpointLine >= firstLine && currentBreakpointLine <= lastLine)
			{
				auto lineHeight = editor.getLineHeight();
				auto b = editor.getLocalBounds();

				int x = 0;
				int y = lineHeight * (currentBreakpointLine - firstLine - 1);
				int w = editor.getWidth();
				int h = lineHeight;

				g.setColour(Colours::red.withAlpha(0.1f));
				g.fillRect( x, y, w, h );
			}
		}
	}

	void codeDocumentTextDeleted(int , int ) override
	{

	}

    struct ParameterList: public Component,
						  public SliderListener
    {
		Array<FunctionData> functions;

		void updateFromJitObject(JitObject& obj)
		{
			StringArray names = ParameterHelpers::getParameterNames(obj);
			
			functions.clear();
			sliders.clear();

			for (int i = 0; i < names.size(); i++)
			{
				auto s = new juce::Slider(names[i]);
				s->setLookAndFeel(&laf);
				s->setRange(0.0, 1.0, 0.01);
				s->setSliderStyle(Slider::SliderStyle::RotaryHorizontalVerticalDrag);
				s->setColour(HiseColourScheme::ComponentFillTopColourId, Colour(0x66333333));
				s->setColour(HiseColourScheme::ComponentFillBottomColourId, Colour(0xfb111111));
				s->setColour(HiseColourScheme::ComponentOutlineColourId, Colours::white.withAlpha(0.3f));
				s->setColour(HiseColourScheme::ComponentTextColourId, Colours::white);
				s->addListener(this);

				functions.add(ParameterHelpers::getFunction(names[i], obj));

				addAndMakeVisible(s);
				s->setSize(128, 48);
				sliders.add(s);
			}

			auto numColumns = jmax(1, getWidth() / 150);
			auto numRows = sliders.size() / numColumns + 1;

			setSize(getWidth(), numRows * 60);
			resized();
		}

		void sliderValueChanged(Slider* slider) override
		{
			auto index = sliders.indexOf(slider);

			if (auto f = functions[index])
			{
				auto value = slider->getValue();

				auto parent = findParentComponentOfClass<SnexPlayground>();

				parent->currentParameter = getName();
				parent->pendingParam = [f, value]()
				{
					f.callVoid(value);
				};
			}
		}
       
        
        void resized() override
        {
            auto numColumns = jmax(1, getWidth() / 150);
            auto numRows = sliders.size() / numColumns + 1;
            
            int x = 0;
            int y = 0;
            int i = 0;
            
            for(int row = 0; row < numRows; row++)
            {
                x = 0;
                
                for(int column = 0; column < numColumns; column++)
                {
                    if(auto s = sliders[i])
                    {
                        sliders[i++]->setTopLeftPosition(x, y + 5);
                        x += 150;
                    }
                    else
                        break;
                }
                
                y += 50;
            }
        }
        
        hise::GlobalHiseLookAndFeel laf;
        juce::OwnedArray<juce::Slider> sliders;
    };
    
    static juce::String getDefaultCode(bool getTestCode=false);
    
	SnexPlayground(Value externalCodeValue, BufferHandler* bufferHandlerToUse=nullptr);

	~SnexPlayground();

	void paint(Graphics& g) override;
	void resized() override;

	void comboBoxChanged(ComboBox* ) override
	{
		recalculate();
	}

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

private:

	AudioSampleBuffer loadedFile;

	int currentBreakpointLine = -1;

	struct RunThread : public Thread
	{
		RunThread(SnexPlayground& parent) :
			Thread("SnexPlaygroundThread"),
			p(parent)
		{
			setPriority(4);
			startThread();
		}

		SnexPlayground& p;

		void run() override
		{
			while (!threadShouldExit())
			{
				if (p.pendingParam)
				{
					p.pendingParam();
					p.pendingParam = {};
					p.currentParameter = "";
					p.dirty = true;
				}

				if (p.dirty)
				{
					p.recalculateInternal();
					p.dirty = false;
				}

				yield();
			}
		}

	} runThread;

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

	void logMessage(const juce::String& m) override
	{
		consoleContent.insertText(consoleContent.getNumCharacters(), m);
		consoleContent.clearUndoHistory();
	}

	void recalculate();

	void recompile();

    hise::PopupLookAndFeel laf;

	Value externalCodeValue;

	CodeDocument doc;
	AudioSampleBuffer b;
	Graph graph;
	
	juce::CPlusPlusCodeTokeniser tokeniser;
	jit::GlobalScope memory;
	BreakpointDataProvider bpProvider;

	JavascriptCodeEditor editor;
	AssemblyTokeniser assemblyTokeniser;
	CodeDocument assemblyDoc;
	CodeEditorComponent assembly;
	bool saveTest = false;

	struct PlaygroundBufferHandler : public BufferHandler
	{
		PlaygroundBufferHandler()
		{
			reset();
		}

		void registerExternalItems() override
		{
			registerTable(0, { audioFile, 512 });
		}

		float audioFile[512];

	};

	juce::Label resultLabel;

	Compiler::Tokeniser consoleTokeniser;
	CodeDocument consoleContent;
	CodeEditorComponent console;
	ScriptWatchTable watchTable;

    SnexPathFactory factory;
    Path snexIcon;

	TextButton showTable;
	TextButton showAssembly;
	TextButton showSignal;
	TextButton showConsole;
	TextButton showParameters;
	TextButton compileButton;
	TextButton resumeButton;
	TextButton showInfo;

	bool testMode = true;
    
	std::atomic<int> currentSampleIndex = { 0 };

	Spacer spacerAssembly;
	Spacer spacerInfo;
	Spacer spacerParameters;
	Spacer spacerTable;
	Spacer spacerConsole;
	Spacer spacerSignal;

    ParameterList sliders;
	
	
	CallbackCollection cData;
	CallbackStateComponent stateViewer;

};

}
}
