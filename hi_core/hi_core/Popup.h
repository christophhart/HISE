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
*   which must be separately licensed for closed source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

#pragma once

namespace hise { using namespace juce;

class BaseDebugArea;
class BackendProcessorEditor;
class AboutPage;

class TooltipBar: public Component,
				  public Timer
{
public:

	enum ColourIds
	{
		backgroundColour = 0x100,
		textColour = 0x010,
		iconColour = 0x001
	};

	TooltipBar();

    ~TooltipBar();

	struct LookAndFeelMethods
    {
        LookAndFeelMethods();

        virtual ~LookAndFeelMethods();

        Path icon;
        
        virtual void drawTooltipBar(Graphics& g, TooltipBar& bar, float alpha, const String& text);
    };
    
    struct DefaultLookAndFeel: public LookAndFeel_V3,
                               public LookAndFeelMethods
    {
        
    };
    
    DefaultLookAndFeel laf;
    
	void paint(Graphics &g);

	void timerCallback();
	

	void setText(const String &t);

	void clearText();

	void setShowInfoIcon(bool shouldShowIcon);

	void mouseDown(const MouseEvent &e);

    void setFont(Font f);

private:

    Font font;
    
	float alpha = 0.0f;

	bool showIcon = true;
	bool isFadingOut = false;


	int counterSinceLastTextChange;

	String currentText;
	bool isClear;

	Point<float> lastMousePosition;
	bool newPosition;

	
};

} // namespace hise
