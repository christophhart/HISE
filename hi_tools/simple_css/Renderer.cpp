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
	if(auto ss = css.getWithAllStates(nullptr, Selector(ElementType::Body)))
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
	if(auto ss = css.getWithAllStates(nullptr, s.getFirst()))
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

    auto op = ss->getOpacity(key.state.stateFlag);
    
    if(op != 1.0f)
    {
        op = jlimit(0.0f, 1.0f, op);
        
        if(c.second.getNumColours() > 0)
            c.second.multiplyOpacity(op);
        else
            c.first = c.first.withMultipliedAlpha(op);
    }

	if(c.second.getNumColours() > 0)
		g.setGradientFill(c.second);
	else
		g.setColour(c.first);
}
}

int Renderer::getPseudoClassState() const
{
	if(forceOverwriteState)
		return pseudoClassState;

	return currentComponent != nullptr ? getPseudoClassFromComponent(currentComponent) : pseudoClassState;
}

CodeGenerator::CodeGenerator(StyleSheet::Ptr ss_):
	ss(ss_)
{
	String nl = "\n";

	code << "drawBackground(Graphics& g, Rectangle<float> fullArea, PseudoElementType type=PseudoElementType::None)" << nl;

	code << "{" << nl;
	code << "\t" << nl;

	for(const auto& ra: ss->getCodeGeneratorArea("fullArea", { "margin", {}}))
	{
		appendLine(ra);
	}

	auto c = ss->getCodeGeneratorColour("fullArea", { "background", {} });

	appendLine("g.setColour(" + c + ");");
	appendLine("g.fillRect(fullArea);");

	code << "};" << nl;
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
	
	auto stateFlag = getPseudoClassState();
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

	auto imageURL = ss->getURLFromProperty({ "background-image", defaultState});

	if(imageURL.isNotEmpty())
	{
		auto hc = CSSRootComponent::find(*currentComponent);
		ScopedPointer<StyleSheet::Collection::DataProvider> dp = hc->createDataProvider();
		auto img = dp->loadImage(imageURL);
		drawImage(g, img, area, ss, false);
	}
	else
	{
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
	}
	
	if(type == PseudoElementType::None)
	{
		auto beforeAbsolute = ss->getPropertyValue({"position", PseudoState(0).withElement(PseudoElementType::Before)}).toString() == "absolute";
		auto beforeArea = ss->getPseudoArea(beforeAbsolute ? area : ma, stateFlag, PseudoElementType::Before);

		if(!beforeArea.isEmpty())
		{
			Graphics::ScopedSaveState sss(g);
			drawBackground(g, beforeArea, ss, PseudoElementType::Before);

			auto v = ss->getPropertyValueString({"content", PseudoState().withElement(PseudoElementType::Before)});

			if(v.isNotEmpty())
				renderText(g, beforeArea, v, ss, PseudoElementType::Before);
		}

		auto afterAbsolute = ss->getPropertyValue({"position", PseudoState(0).withElement(PseudoElementType::After)}).toString() == "absolute";
		
		auto afterArea = ss->getPseudoArea(afterAbsolute ? area : ma, stateFlag, PseudoElementType::After);
		
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

void Renderer::drawImage(Graphics& g, const juce::Image& img, Rectangle<float> area, StyleSheet::Ptr ss, bool isContent)
{
	Rectangle<float> clipBounds;

	{
		Graphics::ScopedSaveState sss(g);

		auto currentState = PseudoState(getPseudoClassState());
		auto totalArea = area;

		totalArea = ss->getArea(totalArea, { "margin", currentState });

		if(isContent)
			totalArea = ss->getArea(totalArea, { "padding", currentState });

		totalArea = ss->truncateBeforeAndAfter(totalArea, currentState.stateFlag);

		g.setColour(Colours::black.withAlpha(ss->getOpacity(currentState.stateFlag)));

		// { "fill", "contain", "cover", "none", "scale-down" };

		enum class CSSPlacement
		{
			fill,
			contain,
			cover,
			none,
			scaledown,
			numPlacements
		};
		
		auto placement = ss->getAsEnum({ isContent ? "object-fit" : "background-size", currentState}, CSSPlacement::fill);
		
		Rectangle<float> imageBounds = img.getBounds().toFloat();

		auto sourceWidth = imageBounds.getWidth();
		auto sourceHeight = imageBounds.getHeight();
		auto targetWidth = totalArea.getWidth();
		auto targetHeight = totalArea.getHeight();

		auto minScale = jmin(targetWidth / sourceWidth, targetHeight / sourceHeight);
		auto maxScale = jmax(targetWidth / sourceWidth, targetHeight / sourceHeight);
		
		switch(placement)
		{
		case CSSPlacement::fill:
			imageBounds = totalArea;
			break;
		case CSSPlacement::contain:
			imageBounds = totalArea.withSizeKeepingCentre(sourceWidth * minScale, sourceHeight * minScale);
			break;
		case CSSPlacement::cover:
			imageBounds = totalArea.withSizeKeepingCentre(sourceWidth * maxScale, sourceHeight * maxScale);
			break;
		case CSSPlacement::none:
			imageBounds = totalArea.withSizeKeepingCentre(sourceWidth, sourceHeight);
			break;
		case CSSPlacement::scaledown:
			imageBounds = totalArea.withSizeKeepingCentre(sourceWidth * jmax(1.0f, maxScale), sourceHeight * jmax(1.0f, maxScale));
			break;
		case CSSPlacement::numPlacements: break;
		}

		clipBounds = imageBounds.constrainedWithin(totalArea);
		auto transform = ss->getTransform(clipBounds, currentState);

		if(!transform.isIdentity())
			g.addTransform(transform);

		auto borderSize = ss->getPixelValue(clipBounds, {"border-width", currentState});

		auto bp = ss->getBorderPath(clipBounds, currentState);

		auto scopy = clipBounds.reduced(borderSize * 0.5f);

		auto originalPath = bp;

		if(!scopy.isEmpty())
			bp.scaleToFit(scopy.getX(), scopy.getY(), scopy.getWidth(), scopy.getHeight(), false);
		else
			bp = {};

		state.renderShadow(g, bp, currentComponent, ss->getShadow(clipBounds, { "box-shadow", currentState}, false), false);

		{
			Graphics::ScopedSaveState sss(g);

			if(!originalPath.isEmpty())
				g.reduceClipRegion(bp, {});

			g.drawImage(img, imageBounds);
		}
		
		state.renderShadow(g, bp, currentComponent, ss->getShadow(clipBounds, { "box-shadow", currentState}, true), true);

		if(auto nub = ss->getNonUniformBorder(clipBounds, currentState))
		{
			nub.draw(g);
		}
		else
		{
			if(borderSize > 0.0f)
			{
				setCurrentBrush(g, ss, clipBounds, {"border", currentState});
				g.strokePath(bp, PathStrokeType(borderSize));
			} 
		}
	}

	if(isContent)
	{
		ScopedValueSetter<bool> svs(applyMargin, false);

		drawBackground(g, clipBounds, ss, PseudoElementType::Before);
		drawBackground(g, clipBounds, ss, PseudoElementType::After);
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

void Renderer::setPseudoClassState(int state, bool shouldForceOverwrite)
{
	forceOverwriteState |= shouldForceOverwrite;
	pseudoClassState = state;
}
}
}


