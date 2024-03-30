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
	void CSSRootComponent::InfoOverlay::Item::draw(Graphics& g)
	{
		String text;

		for(const auto& s: selectors)
			text << s.toString() << " ";

        auto hue = 0.27f;
		
		auto colour = Colour::fromHSL(hue, 0.7f, 0.8f, 1.0f);
		g.setColour(colour.withAlpha(0.4f));
		g.drawRect(globalBounds, 1.0f);

		g.setColour(colour);
		g.setFont(GLOBAL_MONOSPACE_FONT());

		auto tb = textBounds.withWidth(GLOBAL_MONOSPACE_FONT().getStringWidthFloat(text) + 6.0f);
		
		g.drawText(text, tb.reduced(3), Justification::topLeft);
	}

	CSSRootComponent::InfoOverlay::InfoOverlay(CSSRootComponent& parent_):
	  parent(parent_)
	{
		auto root = dynamic_cast<Component*>(&parent);
		root->addAndMakeVisible(this);
		setBounds(0, 0, root->getWidth(), root->getHeight());
		toFront(false);

		setInterceptsMouseClicks(false, false);
		rebuild();
	}

	void CSSRootComponent::InfoOverlay::rebuild()
	{
		items.clear();

		auto root = dynamic_cast<Component*>(&parent);

		Component::callRecursive<Component>(root, [&](Component* child)
		{
			if(!child->isShowing())
				return false;

			auto ni = new Item();

			ni->selectors.addArray(FlexboxComponent::Helpers::getClassSelectorFromComponentClass(child));

			if(auto id = FlexboxComponent::Helpers::getIdSelectorFromComponentClass(child))
				ni->selectors.add(id);

			ni->globalBounds = root->getLocalArea(child, child->getLocalBounds()).toFloat();
			ni->textBounds = ni->globalBounds;
			ni->ss = parent.css.getForComponent(child);
			ni->c = child;

			if(*ni)
				items.add(ni);

			return false;
		});

		for(auto ni: items)
		{
			for(int i = 0; i < items.size(); i++)
			{
				if(items[i] == ni)
					break;

				if(items[i]->globalBounds.getTopLeft() == ni->globalBounds.getTopLeft())
					ni->textBounds.removeFromTop(18.0f);
			}
		}

		repaint();
	}

	Positioner::Positioner(StyleSheet::Collection styleSheet, Rectangle<float> totalArea_, bool applyMargin_):
	css(styleSheet),
	totalArea(totalArea_),
	applyMargin(applyMargin_)
{
	if(auto ss = css.getWithAllStates(Selector(ElementType::Body)))
	{
		if(applyMargin)
			totalArea = ss->getArea(totalArea, { "margin", 0});

		bodyArea = totalArea;

		totalArea = ss->getArea(totalArea, { "padding", 0});
	}
	else
		bodyArea = totalArea;
}

Rectangle<int> Positioner::getLocalBoundsFromText(const Array<Selector>& s, const String& text,
	Rectangle<int> defaultBounds)
{
	if(auto ss = css.getWithAllStates(s.getFirst()))
	{
		return ss->getLocalBoundsFromText(text).toNearestInt();
	}

	return defaultBounds;
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

Renderer::Renderer(Component* c, StateWatcher& state_):
	ScopedComponentSetter(c)  ,
	currentComponent(c),
	state(state_)
{}

int Renderer::getPseudoClassFromComponent(Component* c)
{
	int state = 0;

	if(c == nullptr)
		return 0;

	auto isHover = c->isMouseOverOrDragging(true);
	auto isDown = c->isMouseButtonDown(false);
	auto isFocus = c->hasKeyboardFocus(false);

	if(auto b = dynamic_cast<Button*>(c))
	{
		if(b->getToggleState())
			state |= (int)PseudoClassType::Checked;
	}

	if(c->getProperties()["first-child"])
		state |= (int)PseudoClassType::First;

	if(c->getProperties()["last-child"])
		state |= (int)PseudoClassType::Last;

	if(!c->isEnabled())
		state |= (int)PseudoClassType::Disabled;
	else
	{
		if(isHover)
			state |= (int)PseudoClassType::Hover;

		if(isDown)
			state |= (int)PseudoClassType::Active;

		if(isFocus)
			state |= (int)PseudoClassType::Focus;
	}

	return state;
}



void Renderer::drawBackground(Graphics& g, Rectangle<float> area, StyleSheet::Ptr ss, PseudoElementType type)
{
	if(ss == nullptr)
		return;
	
	auto stateFlag = currentComponent != nullptr ? getPseudoClassFromComponent(currentComponent) : pseudoClassState;
	auto defaultState = PseudoState(stateFlag).withElement(type);

	if(ss->getPropertyValueString({ "display", defaultState}) == "none")
		return;

	auto ma = applyMargin ? ss->getArea(area, {"margin", defaultState}) : area;
    auto transform = ss->getTransform(ma, defaultState);

	if(!transform.isIdentity())
		g.addTransform(transform);
	
	auto p = ss->getBorderPath(ma, defaultState);

	Path bp;
	Path sp;

	auto borderSize = ss->getPixelValue(ma, {"border-width", defaultState});

	auto useBorderBox = ss->getPropertyValueString({ "box-sizing", defaultState}) == "border-box";
	
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

			auto v = ss->getPropertyValueString({"content", PseudoState().withElement(PseudoElementType::Before)});

			if(v.isNotEmpty())
				renderText(g, beforeArea, v, ss, PseudoElementType::Before);
		}

		auto afterArea = ss->getPseudoArea(ma, stateFlag, PseudoElementType::After);

		if(!afterArea.isEmpty())
		{
			Graphics::ScopedSaveState sss(g);
			drawBackground(g, afterArea, ss, PseudoElementType::After);

			auto v = ss->getPropertyValueString({"content", PseudoState().withElement(PseudoElementType::After)});

			if(v.isNotEmpty())
				renderText(g, afterArea, v, ss, PseudoElementType::After);
		}
	}
}

void Renderer::renderText(Graphics& g, Rectangle<float> area, const String& text, StyleSheet::Ptr ss, PseudoElementType type)
{
	auto currentState = PseudoState(getPseudoClassState()).withElement(type);

	auto totalArea = area;
		
	totalArea = ss->getArea(totalArea, { "margin", currentState });
	totalArea = ss->getArea(totalArea, { "padding", currentState });


	if(type == PseudoElementType::None)
		totalArea = ss->truncateBeforeAndAfter(totalArea, currentState.stateFlag);
	
	g.setFont(ss->getFont(currentState, totalArea));

	auto textToDraw = ss->getText(text, currentState);

	auto j = ss->getJustification(currentState);

	state.renderShadow(g, std::make_tuple(textToDraw, j, totalArea), currentComponent, ss->getShadow(totalArea, { "text-shadow", currentState}, false), false);	

	setCurrentBrush(g, ss, totalArea, {"color", currentState }, Colours::black);
	
	g.drawText(textToDraw, totalArea, j);
}

void Renderer::setPseudoClassState(int state)
{
	pseudoClassState = state;
}
}
}


