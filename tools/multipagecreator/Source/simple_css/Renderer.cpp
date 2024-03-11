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

namespace hise
{
namespace simple_css
{

	
void Renderer::setCurrentBrush(Graphics& g, StyleSheet::Ptr ss, Rectangle<float> area,
	const StyleSheet::PropertyKey& key, Colour defaultColour)
{
	if (ss != nullptr)
	{
		auto c = ss->getColourOrGradient(area, key, defaultColour);

		if(c.second.getNumColours() > 0)
			g.setGradientFill(c.second);
		else
			g.setColour(c.first);
        
        auto op = ss->getOpacity(key.state);
        
        if(op != 1.0f)
            g.setOpacity(jlimit(0.0f, 1.0f, op));
	}
}

int Renderer::getPseudoClassFromComponent(Component* c)
{
	int state = 0;

	auto isHover = c->isMouseOverOrDragging(false);
	auto isDown = c->isMouseButtonDown(false);
	auto isFocus = c->hasKeyboardFocus(false);
		
	if(isHover)
		state |= (int)PseudoClassType::Hover;

	if(isDown)
		state |= (int)PseudoClassType::Active;

	if(isFocus)
		state |= (int)PseudoClassType::Focus;

	return state;
}



void Renderer::drawBackground(Graphics& g, Rectangle<float> area, StyleSheet::Ptr ss)
{
	if(ss == nullptr)
		return;

	auto stateFlag = getPseudoClassFromComponent(&currentComponent);

	auto ma = ss->getArea(area, {"margin", stateFlag});

    auto opacity = ss->getOpacity(stateFlag);
    
    auto transform = ss->getTransform(ma, stateFlag);

	if(!transform.isIdentity())
		g.addTransform(transform);

	auto p = ss->getBorderPath(ma, stateFlag);


	auto& sw = ComponentWithCSS::find(currentComponent)->stateWatcher;

	sw.renderShadow(g, p, &currentComponent, ss->getBoxShadow(ma, stateFlag, false), false);

	setCurrentBrush(g, ss, ma, {"background", stateFlag});
	g.fillPath(p);

	sw.renderShadow(g, p, &currentComponent, ss->getBoxShadow(ma, stateFlag, true), true);

	auto strokeSize = ss->getPixelValue(ma, {"border-width", stateFlag});

	if(strokeSize > 0.0f)
	{
		setCurrentBrush(g, ss, ma, {"border", stateFlag});
		g.strokePath(p, PathStrokeType(strokeSize));
	}
}
}
}


