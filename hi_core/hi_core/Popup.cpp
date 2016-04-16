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


TooltipBar::TooltipBar():
counterSinceLastTextChange(0),
currentText(""),
isClear(true),
lastMousePosition(),
newPosition(true)
{
#if JUCE_DEBUG
	startTimer(150);
#else
	startTimer(30);
#endif

	setColour(backgroundColour, Colours::black.withAlpha(0.2f));
	setColour(iconColour, Colours::white.withAlpha(0.2f));
	setColour(textColour, Colours::white.withAlpha(0.8f));

	

}

void TooltipBar::paint(Graphics &g)
{
	static const unsigned char pathData[] = { 110, 109, 0, 0, 12, 67, 46, 183, 84, 68, 98, 229, 174, 239, 66, 46, 183, 84, 68, 254, 255, 206, 66, 11, 205, 88, 68, 254, 255, 206, 66, 46, 215, 93, 68, 98, 254, 255, 206, 66, 82, 225, 98, 68, 228, 174, 239, 66, 46, 247, 102, 68, 0, 0, 12, 67, 46, 247, 102, 68, 98, 142, 40, 32, 67, 46, 247, 102, 68, 1, 128, 48, 67, 81, 225,
		98, 68, 1, 128, 48, 67, 46, 215, 93, 68, 98, 1, 128, 48, 67, 10, 205, 88, 68, 142, 40, 32, 67, 46, 183, 84, 68, 0, 0, 12, 67, 46, 183, 84, 68, 99, 109, 0, 0, 12, 67, 46, 65, 101, 68, 98, 31, 62, 247, 66, 46, 65, 101, 68, 255, 175, 220, 66, 106, 239, 97, 68, 255, 175, 220, 66, 46, 215, 93, 68, 98, 255, 175, 220, 66, 242, 190,
		89, 68, 31, 62, 247, 66, 46, 109, 86, 68, 0, 0, 12, 67, 46, 109, 86, 68, 98, 240, 96, 28, 67, 46, 109, 86, 68, 1, 168, 41, 67, 242, 190, 89, 68, 1, 168, 41, 67, 46, 215, 93, 68, 98, 1, 168, 41, 67, 106, 239, 97, 68, 241, 96, 28, 67, 46, 65, 101, 68, 0, 0, 12, 67, 46, 65, 101, 68, 99, 109, 0, 112, 7, 67, 46, 71, 89, 68, 108, 0, 144,
		16, 67, 46, 71, 89, 68, 108, 0, 144, 16, 67, 46, 143, 91, 68, 108, 0, 112, 7, 67, 46, 143, 91, 68, 108, 0, 112, 7, 67, 46, 71, 89, 68, 99, 109, 0, 32, 21, 67, 46, 103, 98, 68, 108, 0, 224, 2, 67, 46, 103, 98, 68, 108, 0, 224, 2, 67, 46, 67, 97, 68, 108, 0, 112, 7, 67, 46, 67, 97, 68, 108, 0, 112, 7, 67, 46, 215, 93, 68, 108, 0, 224,
		2, 67, 46, 215, 93, 68, 108, 0, 224, 2, 67, 46, 179, 92, 68, 108, 0, 144, 16, 67, 46, 179, 92, 68, 108, 0, 144, 16, 67, 46, 67, 97, 68, 108, 0, 32, 21, 67, 46, 67, 97, 68, 108, 0, 32, 21, 67, 46, 103, 98, 68, 99, 101, 0, 0 };

	Path path;
	path.loadPathFromData(pathData, sizeof(pathData));

	path.scaleToFit(4.0f, 4.0f, (float)(getHeight() - 8), (float)(getHeight() - 8), true);


	g.setColour(findColour(backgroundColour));

	g.fillRoundedRectangle(0.0f, 0.0f, (float)getWidth(), (float)getHeight(), 3.0f);

	g.setColour(findColour(iconColour));

	g.fillPath(path);

	g.setColour(findColour(textColour));
	g.setFont(GLOBAL_BOLD_FONT());
	g.drawText(currentText, 28, 0, getWidth() - 24, getHeight(), Justification::centredLeft, true);
}

void TooltipBar::timerCallback()
{
	Desktop& desktop = Desktop::getInstance();
	const MouseInputSource mouseSource (desktop.getMainMouseSource());

	Point<float> thisPosition = mouseSource.getScreenPosition();

	const bool newPosition = thisPosition != lastMousePosition;

	lastMousePosition = thisPosition;

	Component* const newComp = mouseSource.isMouse() ? mouseSource.getComponentUnderMouse() : nullptr;

    Component *parentComponent = nullptr;
    
    
#if USE_BACKEND
    
    parentComponent = findParentComponentOfClass<BackendProcessorEditor>();
    
    if(parentComponent == nullptr)
    {
        parentComponent = findParentComponentOfClass<PluginPreviewWindow>();
    }
    
#else
    
    parentComponent = findParentComponentOfClass<FrontendProcessorEditor>();
    
#endif
    
    jassert(parentComponent != nullptr);
    
    
	// Deactivate tooltips for multiple instances!
	if (parentComponent == nullptr || !parentComponent->isParentOf(newComp)) return;

	TooltipClient *client = dynamic_cast<TooltipClient*>(newComp);

#if USE_BACKEND

	Component *focusComponent = Component::getCurrentlyFocusedComponent();

	TooltipClient *keyClient = dynamic_cast<JavascriptCodeEditor*>(focusComponent);

#else 
	TooltipClient *keyClient = nullptr;

#endif

	if(counterSinceLastTextChange++ >= 100) 
	{
		clearText();
	}

	if(!newPosition && keyClient != nullptr)
	{
		setText(keyClient->getTooltip());
	}
	else if(client != nullptr)
	{
		setText(client->getTooltip());
	}
		
	else
	{
		clearText();
	}

}

void TooltipBar::mouseDown(const MouseEvent &)
{
	
}
