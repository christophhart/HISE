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

struct CallbackCollection
{
	struct Listener
	{
		virtual ~Listener() {};

		virtual void initialised(const CallbackCollection& c) = 0;

		virtual void prepare(double samplerate, int blockSize, int numChannels) = 0;

		JUCE_DECLARE_WEAK_REFERENCEABLE(Listener);
	};

	enum ProcessType
	{
		FrameProcessing,
		BlockProcessing,
		numProcessTypes
	};

	enum ActiveCallback
	{
		Channel,
		Frame,
		Sample,
		numActiveCallbacks,
		Inactive
	};

	void setupCallbacks()
	{
		StringArray cIds = { "processChannel", "processFrame", "processSample" };

		using namespace snex::Types;

		prepareFunction = obj["prepare"];

		if (!prepareFunction.matchesArgumentTypes(ID::Void, { ID::Double, ID::Integer, ID::Integer }))
			prepareFunction = {};

		resetFunction = obj["reset"];

		if (!resetFunction.matchesArgumentTypes(ID::Void, {}))
			resetFunction = {};

		callbacks[Sample] = obj[cIds[Sample]];

		if (!callbacks[Sample].matchesArgumentTypes(ID::Float, { ID::Float }))
			callbacks[Sample] = {};

		callbacks[Frame] = obj[cIds[Frame]];

		if (!callbacks[Frame].matchesArgumentTypes(ID::Void, { ID::Block }))
			callbacks[Frame] = {};

		callbacks[Channel] = obj[cIds[Channel]];

		if (!callbacks[Channel].matchesArgumentTypes(ID::Void, { ID::Block, ID::Integer }))
			callbacks[Channel] = {};

		bestCallback[FrameProcessing] = getBestCallback(FrameProcessing);
		bestCallback[BlockProcessing] = getBestCallback(BlockProcessing);

		if (listener != nullptr)
			listener->initialised(*this);
	}

	ActiveCallback getBestCallback(int processType) const
	{
		if (processType == FrameProcessing)
		{
			if (callbacks[Frame])
				return Frame;
			if (callbacks[Sample])
				return Sample;
			if (callbacks[Channel])
				return Channel;

			return Inactive;
		}
		else
		{
			if (callbacks[Channel])
				return Channel;
			if (callbacks[Frame])
				return Frame;
			if (callbacks[Sample])
				return Sample;

			return Inactive;
		}
	}

	void prepare(double sampleRate, int blockSize, int numChannels)
	{
		if (prepareFunction)
			prepareFunction.callVoid(sampleRate, blockSize, numChannels);

		if (resetFunction)
			resetFunction.callVoid();

		if(listener != nullptr)
			listener->prepare(sampleRate, blockSize, numChannels);
	}

	void setListener(Listener* l)
	{
		listener = l;
	}

	ActiveCallback bestCallback[numProcessTypes];

	snex::jit::JitObject obj;

	FunctionData callbacks[numActiveCallbacks];

	FunctionData resetFunction;
	FunctionData prepareFunction;
	FunctionData eventFunction;
	FunctionData modFunction;

	WeakReference<Listener> listener;
};

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

	String getCallbackName() const
	{
		switch (currentCallback)
		{
		case CallbackCollection::Channel:	return "processChannel()";
		case CallbackCollection::Frame:		return "processFrame()";
		case CallbackCollection::Sample:	return "processSample()";
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
		String s;
		s << "**Samplerate**: " << String(samplerate, 1) << "  ";
		s << "**Blocksize**: " << String(blockSize) << "  ";
		s << "**NumChannels**: " << String(numChannels) << "  ";
		s << "**Frame processing**: " << (frameProcessing ? "Yes" : "No") << "  ";
		s << "**Used Callback**: `" << getCallbackName() << "`";

		r.setNewText(s);
		r.getHeightForWidth(getWidth() + 10, true);
		setSize(getWidth(), 130);

		repaint();
	}

	void prepare(double samplerate_, int blockSize_, int numChannels_) override
	{
		samplerate = samplerate_;
		blockSize = blockSize_;
		numChannels = numChannels_;
		
		rebuild();
	}

	String processSpecs;

	double samplerate = 0.0;
	int blockSize = 0;
	int numChannels = 0;

	CallbackCollection::ActiveCallback frameCallback;
	CallbackCollection::ActiveCallback blockCallback;

	CallbackCollection::ActiveCallback currentCallback = CallbackCollection::Inactive;
	bool frameProcessing = false;
	bool active = false;

	MarkdownRenderer r;
};

struct SnexPathFactory: public hise::PathFactory
{
    String getId() const override { return "Snex"; }
    Path createPath(const String& id) const override;
};
    
struct Graph : public Component
{
	int boxWidth = 128;

	Graph() :
		testSignal("TestSignal"),
		channelMode("ChannelMode")
	{
		addAndMakeVisible(testSignal);
		skin(testSignal);
		testSignal.setTextWhenNothingSelected("Select test signal");
		testSignal.addItemList({ "Noise", "Ramp", "Fast Ramp" }, 1);

		addAndMakeVisible(processingMode);
		skin(processingMode);
		processingMode.setTextWhenNothingSelected("Select processing");
		processingMode.addItemList({ "Frame", "Block"}, 1);

		addAndMakeVisible(channelMode);
		skin(channelMode);
		channelMode.setTextWhenNothingSelected("Select channel mode");
		channelMode.addItemList({ "Mono", "Stereo" }, 1);

		addAndMakeVisible(bufferLength);
		skin(bufferLength);
		bufferLength.setTextWhenNothingSelected("Select Buffer size");
		bufferLength.addItemList({ "16", "512", "44100" }, 1);
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

	void setBuffer(AudioSampleBuffer& b);

	void paint(Graphics& g) override;

	void resized() override
	{
		auto boxBounds = getLocalBounds().removeFromLeft(boxWidth);

		testSignal.setBounds(boxBounds.removeFromTop(30));
		channelMode.setBounds(boxBounds.removeFromTop(30));
		bufferLength.setBounds(boxBounds.removeFromTop(30));
		processingMode.setBounds(boxBounds.removeFromTop(30));
	}

	void calculatePath(Path& p, AudioSampleBuffer& b, int channel);

	Range<float> leftPeaks;
	Range<float> rightPeaks;
	bool stereoMode = false;

	ComboBox testSignal;
	ComboBox channelMode;
	ComboBox bufferLength;
	ComboBox processingMode;
	Path l;
	Path r;
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

struct ParameterHelpers
{
	static FunctionData getFunction(const String& parameterName, JitObject& obj)
	{
		Identifier id("set" + parameterName);

		auto f = obj[id];

		if (f.matchesArgumentTypes(Types::ID::Void, { Types::ID::Double }))
			return f;

		return {};
	}

	static StringArray getParameterNames(JitObject& obj)
	{
		auto ids = obj.getFunctionIds();
		StringArray sa;

		for (int i = 0; i < ids.size(); i++)
		{
			auto fName = ids[i].toString();

			if (fName.startsWith("set"))
				sa.add(fName.fromFirstOccurrenceOf("set", false, false));
		}

		return sa;
	}
};

class SnexPlayground : public Component,
	public ComboBox::Listener,
	public DebugHandler,
	public CodeDocument::Listener
{
public:

	void codeDocumentTextInserted(const String& , int ) override
	{
		auto lineToShow = jmax(0, consoleContent.getNumLines() - console.getNumLinesOnScreen());
		console.scrollToLine(lineToShow);
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
				f.callVoid(slider->getValue());
			}

			findParentComponentOfClass<SnexPlayground>()->recalculate();
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
    
    static String getDefaultCode();
    
	SnexPlayground(Value externalCodeValue);

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
		Spacer(const String& n) :
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

	void logMessage(const String& m) override
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
	
	CodeEditorComponent editor;
	AssemblyTokeniser assemblyTokeniser;
	CodeDocument assemblyDoc;
	CodeEditorComponent assembly;

	Label resultLabel;

	Compiler::Tokeniser consoleTokeniser;
	CodeDocument consoleContent;
	CodeEditorComponent console;
    
    SnexPathFactory factory;
    Path snexIcon;

	TextButton showAssembly;
	TextButton showSignal;
	TextButton showConsole;
	TextButton showParameters;
	TextButton compileButton;
	TextButton showInfo;
    
	Spacer spacerAssembly;
	Spacer spacerInfo;
	Spacer spacerParameters;
	Spacer spacerConsole;
	Spacer spacerSignal;

    ParameterList sliders;
	jit::GlobalScope memory;
	
	CallbackCollection cData;
	CallbackStateComponent stateViewer;

};

}
}
