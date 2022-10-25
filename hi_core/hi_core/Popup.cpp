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
    setLookAndFeel(&laf);
    
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
		return;
	
    if(auto tlaf = dynamic_cast<LookAndFeelMethods*>(&getLookAndFeel()))
        tlaf->drawTooltipBar(g, *this, alpha, currentText);
}

void TooltipBar::timerCallback()
{
	Desktop& desktop = Desktop::getInstance();
	const MouseInputSource mouseSource (desktop.getMainMouseSource());

	Point<float> thisPosition = mouseSource.getScreenPosition();

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


