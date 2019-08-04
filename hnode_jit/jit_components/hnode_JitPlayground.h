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


namespace hnode {
namespace jit {
using namespace juce;

struct SnexPathFactory: public hise::PathFactory
{
    String getId() const override { return "Snex"; }
    Path createPath(const String& id) const override;
};
    
struct Graph : public Component
{
	void setBuffer(AudioSampleBuffer& b);

	void paint(Graphics& g) override;

	Path p;
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

class JitPlayground : public Component,
	public ComboBox::Listener,
	public Compiler::DebugHandler
{
public:

    static String getDefaultCode()
    {
        auto emitCommentLine = [](String& code, const String& comment)
        {
            code << "/** " << comment << " */\n" ;
        };
        
        String s;
        String nl = "\n";
        String emptyBracket;
        emptyBracket << "{" << nl << "\t" << nl << "}" << nl << nl;
        
        emitCommentLine(s, "Variable definitions");
        s << "double sr = 0.0;" << nl;
        s << "int c = 0;" << nl << nl;
        
        emitCommentLine(s, "Initialise the processing here.");
        s << "void prepare(double sampleRate, int blockSize, int numChannels)" << nl;
        s << "{" << nl;
        s << "\tsr = sampleRate;" << nl;
        s << "\tc = numChannels;" << nl;
        s << "}" << nl << nl;
        
        emitCommentLine(s, "Reset the processing pipeline");
        s << "void reset()" << nl;
        s << emptyBracket;
        
        emitCommentLine(s, "Mono processing callback");
        s << "float processSample(float input)" << nl;
        s << "{" << nl << "\treturn input;" << nl << "}" << nl << nl;
        
        emitCommentLine(s, "Multichannel processing callback");
        s << "void processFrame(block frame)" << nl;
        s << "{" << nl;
        s << "\t// Clear first channel" << nl;
        s << "\tframe[0] = 0.0f;" << nl;
        s << "\t// Clear second channel" << nl;
        s << "\tframe[1] = 0.0f;" << nl;
        s << "}";
        
        return s;
    }
    
	JitPlayground();

	void paint(Graphics& g) override;
	void resized() override;

	void comboBoxChanged(ComboBox* b) override
	{
		recalculate();
	}


	bool keyPressed(const KeyPress& k) override;
	void createTestSignal();

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

		void drawButtonText(Graphics&g, TextButton& b, bool over, bool down)
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

    hise::PopupLookAndFeel laf;

	AudioSampleBuffer b;
	Graph graph;
	ComboBox testSignal;
	juce::CPlusPlusCodeTokeniser tokeniser;
	CodeDocument doc;
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
	TextButton compileButton;

};

}
}
