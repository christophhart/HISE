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
	Positioner::Positioner(StyleSheet::Collection styleSheet, Rectangle<float> totalArea_, bool applyMargin_):
		css(styleSheet),
		totalArea(totalArea_),
		applyMargin(applyMargin_)
	{
		if(auto ss = css[Selector(ElementType::Body)])
		{
			if(applyMargin)
				totalArea = ss->getArea(totalArea, { "margin", 0});

			bodyArea = totalArea;

			totalArea = ss->getArea(totalArea, { "padding", 0});
		}
		else
			bodyArea = totalArea;
	}

	

	

void Renderer::setCurrentBrush(Graphics& g, StyleSheet::Ptr ss, Rectangle<float> area,
	                               const PropertyKey& key, Colour defaultColour)
{
	if (ss != nullptr)
	{
		auto c = ss->getColourOrGradient(area, key, defaultColour);

		if(c.second.getNumColours() > 0)
			g.setGradientFill(c.second);
		else
			g.setColour(c.first);
        
        auto op = ss->getOpacity(key.state.stateFlag);
        
        if(op != 1.0f)
            g.setOpacity(jlimit(0.0f, 1.0f, op));
	}
}

int Renderer::getPseudoClassFromComponent(Component* c)
{
	int state = 0;

	if(c == nullptr)
		return 0;

	auto isHover = c->isMouseOverOrDragging(true);
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



void Renderer::drawBackground(Graphics& g, Rectangle<float> area, StyleSheet::Ptr ss, PseudoElementType type)
{
	if(ss == nullptr)
		return;

	auto stateFlag = currentComponent != nullptr ? getPseudoClassFromComponent(currentComponent) : pseudoClassState;
	auto defaultState = PseudoState(stateFlag).withElement(type);

	auto ma = ss->getArea(area, {"margin", defaultState});
    auto transform = ss->getTransform(ma, defaultState);

	if(!transform.isIdentity())
		g.addTransform(transform);

	

	

	auto p = ss->getBorderPath(ma, defaultState);

	Path bp;
	Path sp;

	auto borderSize = ss->getPixelValue(ma, {"border-width", defaultState});

	auto useBorderBox = false;

	if(auto bs = ss->getPropertyValue({ "box-sizing", defaultState}))
	{
		useBorderBox = bs.valueAsString == "border-box";
	}

	auto pathToFill = p;
	auto pathToStroke = p;

	auto scopy = ma.reduced(borderSize * 0.5f);

	if(!scopy.isEmpty())
		pathToStroke.scaleToFit(scopy.getX(), scopy.getY(), scopy.getWidth(), scopy.getHeight(), false);
	else
		pathToStroke = {};

	if(useBorderBox)
	{
		auto copy = ma.reduced(borderSize);
		if(!copy.isEmpty())
			pathToFill.scaleToFit(copy.getX(), copy.getY(), copy.getWidth(), copy.getHeight(), false);
		else
			pathToFill = {};
	}

	state.renderShadow(g, pathToFill, currentComponent, ss->getShadow(ma, { "box-shadow", defaultState}, false), false);

	setCurrentBrush(g, ss, ma, {"background", defaultState});
	g.fillPath(pathToFill);

	state.renderShadow(g, bp.isEmpty() ? p : bp, currentComponent, ss->getShadow(ma, { "box-shadow", defaultState}, true), true);

	if(auto nub = ss->getNonUniformBorder(ma, defaultState))
	{
		nub.draw(g);
	}
	else
	{
		if(borderSize > 0.0f)
		{
			setCurrentBrush(g, ss, ma, {"border", defaultState});
			g.strokePath(pathToStroke, PathStrokeType(borderSize));
		} 
	}

	if(type == PseudoElementType::None)
	{
		auto beforeArea = ss->getPseudoArea(ma, stateFlag, PseudoElementType::Before);

		if(!beforeArea.isEmpty())
		{
			Graphics::ScopedSaveState sss(g);
			drawBackground(g, beforeArea, ss, PseudoElementType::Before);
		}

		auto afterArea = ss->getPseudoArea(ma, stateFlag, PseudoElementType::After);

		if(!afterArea.isEmpty())
		{
			Graphics::ScopedSaveState sss(g);
			drawBackground(g, afterArea, ss, PseudoElementType::After);
		}
	}

#if 0
	auto beforeArea = ss->getPseudoArea(ma, stateFlag, PseudoElementType::Before);
	if(!beforeArea.isEmpty())
	{
		PseudoState ps(stateFlag);
		ps.element = PseudoElementType::Before;

		Graphics::ScopedSaveState sss(g);

		auto transform = ss->getTransform(beforeArea, ps);
		if(!transform.isIdentity())
			g.addTransform(transform);
		
		auto bp = ss->getBorderPath(beforeArea, ps);

		state.renderShadow(g, bp, currentComponent, ss->getShadow(beforeArea, { "box-shadow", ps}, false), false);

		setCurrentBrush(g, ss, beforeArea, { "background", ps });
		g.fillPath(bp);

		state.renderShadow(g, bp, currentComponent, ss->getShadow(beforeArea, { "box-shadow", ps}, true), true);

		auto beforeStroke = ss->getPixelValue(beforeArea, {"border-width", ps});

		if(beforeStroke > 0.0f)
		{
			setCurrentBrush(g, ss, beforeArea, {"border", ps});
			g.strokePath(bp, PathStrokeType(beforeStroke));
		}
	}

	auto afterArea = ss->getPseudoArea(ma, stateFlag, PseudoElementType::After);
	if(!afterArea.isEmpty())
	{
		PseudoState ps(stateFlag);
		ps.element = PseudoElementType::After;

		Graphics::ScopedSaveState sss(g);

		auto transform = ss->getTransform(afterArea, ps);
		if(!transform.isIdentity())
			g.addTransform(transform);
		
		auto bp = ss->getBorderPath(afterArea, ps);
		setCurrentBrush(g, ss, afterArea, { "background", ps });
		
		g.fillPath(bp);

		auto afterStroke = ss->getPixelValue(afterArea, {"border-width", ps});

		if(afterStroke > 0.0f)
		{
			setCurrentBrush(g, ss, afterArea, {"border", ps});
			g.strokePath(bp, PathStrokeType(afterStroke));
		}
	}
#endif
}

void Renderer::renderText(Graphics& g, Rectangle<float> area, const String& text, StyleSheet::Ptr ss)
{
	auto currentState = getPseudoClassState();
	auto totalArea = area;
		
	totalArea = ss->getArea(totalArea, { "margin", currentState });
	totalArea = ss->getArea(totalArea, { "padding", currentState });


	totalArea = ss->truncateBeforeAndAfter(totalArea, currentState);
	
	g.setFont(ss->getFont(currentState, totalArea));

	auto textToDraw = ss->getText(text, currentState);

	auto j = ss->getJustification(currentState);

	state.renderShadow(g, std::make_tuple(textToDraw, j, totalArea), currentComponent, ss->getShadow(totalArea, { "text-shadow", currentState}, false), false);	

	setCurrentBrush(g, ss, totalArea, {"color", currentState }, Colours::black);
	
	g.drawText(textToDraw, totalArea, j);
}
}
}


