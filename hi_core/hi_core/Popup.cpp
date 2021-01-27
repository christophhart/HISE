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

namespace hise { using namespace juce;

TooltipBar::TooltipBar():
counterSinceLastTextChange(0),
currentText(""),
isClear(true),
lastMousePosition(),
newPosition(true),
font(GLOBAL_BOLD_FONT())
{
#if JUCE_DEBUG
	startTimer(50);
#else
	startTimer(30);
#endif

	setColour(backgroundColour, Colour(0xFF383838));
	setColour(iconColour, Colours::white.withAlpha(0.2f));
	setColour(textColour, Colours::white.withAlpha(0.8f));
}

void TooltipBar::paint(Graphics &g)
{
	if (currentText.isEmpty())
	{
		return;
	}
		
	const float thisAlpha = jmin<float>(alpha, 1.0f);

	int offset = 0;

	g.setColour(findColour(backgroundColour).withMultipliedAlpha(thisAlpha));

	g.fillRect(0.0f, 0.0f, (float)getWidth(), (float)getHeight());

	if (showIcon)
	{
		static const unsigned char pathData[] = { 110, 109, 0, 0, 12, 67, 46, 183, 84, 68, 98, 229, 174, 239, 66, 46, 183, 84, 68, 254, 255, 206, 66, 11, 205, 88, 68, 254, 255, 206, 66, 46, 215, 93, 68, 98, 254, 255, 206, 66, 82, 225, 98, 68, 228, 174, 239, 66, 46, 247, 102, 68, 0, 0, 12, 67, 46, 247, 102, 68, 98, 142, 40, 32, 67, 46, 247, 102, 68, 1, 128, 48, 67, 81, 225,
			98, 68, 1, 128, 48, 67, 46, 215, 93, 68, 98, 1, 128, 48, 67, 10, 205, 88, 68, 142, 40, 32, 67, 46, 183, 84, 68, 0, 0, 12, 67, 46, 183, 84, 68, 99, 109, 0, 0, 12, 67, 46, 65, 101, 68, 98, 31, 62, 247, 66, 46, 65, 101, 68, 255, 175, 220, 66, 106, 239, 97, 68, 255, 175, 220, 66, 46, 215, 93, 68, 98, 255, 175, 220, 66, 242, 190,
			89, 68, 31, 62, 247, 66, 46, 109, 86, 68, 0, 0, 12, 67, 46, 109, 86, 68, 98, 240, 96, 28, 67, 46, 109, 86, 68, 1, 168, 41, 67, 242, 190, 89, 68, 1, 168, 41, 67, 46, 215, 93, 68, 98, 1, 168, 41, 67, 106, 239, 97, 68, 241, 96, 28, 67, 46, 65, 101, 68, 0, 0, 12, 67, 46, 65, 101, 68, 99, 109, 0, 112, 7, 67, 46, 71, 89, 68, 108, 0, 144,
			16, 67, 46, 71, 89, 68, 108, 0, 144, 16, 67, 46, 143, 91, 68, 108, 0, 112, 7, 67, 46, 143, 91, 68, 108, 0, 112, 7, 67, 46, 71, 89, 68, 99, 109, 0, 32, 21, 67, 46, 103, 98, 68, 108, 0, 224, 2, 67, 46, 103, 98, 68, 108, 0, 224, 2, 67, 46, 67, 97, 68, 108, 0, 112, 7, 67, 46, 67, 97, 68, 108, 0, 112, 7, 67, 46, 215, 93, 68, 108, 0, 224,
			2, 67, 46, 215, 93, 68, 108, 0, 224, 2, 67, 46, 179, 92, 68, 108, 0, 144, 16, 67, 46, 179, 92, 68, 108, 0, 144, 16, 67, 46, 67, 97, 68, 108, 0, 32, 21, 67, 46, 67, 97, 68, 108, 0, 32, 21, 67, 46, 103, 98, 68, 99, 101, 0, 0 };

		Path path;
		path.loadPathFromData(pathData, sizeof(pathData));

		path.scaleToFit(4.0f, 4.0f, (float)(getHeight() - 8), (float)(getHeight() - 8), true);

		g.setColour(findColour(iconColour).withMultipliedAlpha(thisAlpha));

		g.fillPath(path);

		offset = 24;

	}

	

	g.setColour(findColour(textColour).withMultipliedAlpha(thisAlpha));
	g.setFont(font);
	g.drawText(currentText, offset + 4, 0, getWidth() - offset, getHeight(), Justification::centredLeft, true);
}

void TooltipBar::timerCallback()
{
	Desktop& desktop = Desktop::getInstance();
	const MouseInputSource mouseSource (desktop.getMainMouseSource());

	juce::Point<float> thisPosition = mouseSource.getScreenPosition();

	const bool positionHasChanged = thisPosition != lastMousePosition;

	lastMousePosition = thisPosition;

	Component* const newComp = mouseSource.isMouse() ? mouseSource.getComponentUnderMouse() : nullptr;

    Component *parent = dynamic_cast<Component*>(findParentComponentOfClass<ModalBaseWindow>());
    
    //jassert(parent != nullptr);
    
	// Deactivate tooltips for multiple instances!
	if (parent == nullptr || !parent->isParentOf(newComp))
		return;

	TooltipClient *client = dynamic_cast<TooltipClient*>(newComp);

#if USE_BACKEND

	Component *focusComponent = Component::getCurrentlyFocusedComponent();

	TooltipClient *keyClient = dynamic_cast<JavascriptCodeEditor*>(focusComponent);

#else 
	TooltipClient *keyClient = nullptr;

#endif

	if(!positionHasChanged && keyClient != nullptr)
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

} // namespace hise
