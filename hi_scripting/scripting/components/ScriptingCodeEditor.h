/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   HISE is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licenses for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licensing:
*
*   http://www.hise.audio/
*
*   HISE is based on the JUCE library,
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/
#ifndef SCRIPTINGCODEEDITOR_H_INCLUDED
#define SCRIPTINGCODEEDITOR_H_INCLUDED

namespace hise { using namespace juce;


class DebugConsoleTextEditor : public TextEditor,
	public TextEditor::Listener,
	public GlobalScriptCompileListener
{
public:

	DebugConsoleTextEditor(const String& name, Processor* p);;

	~DebugConsoleTextEditor();

	void scriptWasCompiled(JavascriptProcessor *jp);

	bool keyPressed(const KeyPress& k) override;

	void mouseDown(const MouseEvent& e);
	void mouseDoubleClick(const MouseEvent& e) override;

	void gotoText();

	void addToHistory(const String& s);

	void textEditorReturnKeyPressed(TextEditor& /*t*/);

    void setOK(bool isOK)
    {
		pending = false;
        ok = isOK;
        repaint();
    }
    
	void startCompilation()
	{
		pending = true;
		repaint();
	}

private:

    String fullErrorMessage;
    
	bool pending = false;

    bool ok = true;
    
    struct LAF: public LookAndFeel_V2
    {
        void fillTextEditorBackground (Graphics& g, int width, int height, TextEditor& t)
        {
            if(auto d = dynamic_cast<DebugConsoleTextEditor*>(&t))
            {
                auto ta = Rectangle<float>(0.0f, 0.0f, (float)width, (float)height);
                
                auto br = JUCE_LIVE_CONSTANT_OFF(1.3f);
                
                Colour resultColours[2] = { Colour(HISE_ERROR_COLOUR).withMultipliedBrightness(br),
                                            Colour(HISE_OK_COLOUR).withMultipliedBrightness(br) };
                
                g.setColour(Colours::white.withAlpha(0.4f));
                
                auto circleWidth = JUCE_LIVE_CONSTANT_OFF(10.0f);
                auto margin = JUCE_LIVE_CONSTANT_OFF(5);
                auto padding = JUCE_LIVE_CONSTANT_OFF(2);
                float noAlpha = JUCE_LIVE_CONSTANT_OFF(0.35f);
                auto xMargin = JUCE_LIVE_CONSTANT_OFF(8.5f);
                
                ta.removeFromRight(xMargin);
                auto c2 = ta.removeFromRight(circleWidth).withSizeKeepingCentre(circleWidth, circleWidth);
                ta.removeFromRight(margin);
                auto c1 = ta.removeFromRight(circleWidth).withSizeKeepingCentre(circleWidth, circleWidth);
                
                auto isError = !d->ok;
                
				auto isPending = d->pending;

                g.setColour(resultColours[0].withAlpha(isError && !isPending ? 0.7f : noAlpha));
                g.drawEllipse(c1, 1.0f);
                g.setColour(resultColours[1].withAlpha(!isError && !isPending ? 0.7f : noAlpha));
                g.drawEllipse(c2, 1.0f);
                
                g.setColour(resultColours[0].withAlpha(isError && !isPending ? 1.0f : noAlpha));
                g.fillEllipse(c1.reduced(padding));
                g.setColour(resultColours[1].withAlpha(!isError && !isPending ? 1.0f : noAlpha));
                g.fillEllipse(c2.reduced(padding));
                
            }
        }
        
    } laf2;

	WeakReference<Processor> processor;

	StringArray history;
	int currentHistoryIndex = 0;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DebugConsoleTextEditor)
};





} // namespace hise
#endif  // SCRIPTINGCODEEDITOR_H_INCLUDED
