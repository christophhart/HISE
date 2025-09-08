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

#if HISE_INCLUDE_CSS_DEBUG_TOOLS

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

	//setInterceptsMouseClicks(false, false);
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

void CSSRootComponent::InspectorData::draw(Graphics& g, Rectangle<float> lb, StyleSheet::Collection& css) const
{
	if(!first.isEmpty())
	{
		auto cb = first;
		auto b = lb;
		auto left = b.removeFromLeft(cb.getX());
		auto right = b.removeFromRight(b.getRight() - cb.getRight());
		auto top = b.removeFromTop(cb.getY());
		auto bottom = b.removeFromBottom(b.getBottom() - cb.getBottom());
		            
		g.setColour(Colours::black.withAlpha(0.5f));
		g.fillRect(top);
		g.fillRect(left);
		g.fillRect(right);
		g.fillRect(bottom);
		            
		g.setColour(Colour(SIGNAL_COLOUR).withAlpha(0.3f));
		g.drawRect(first, 1.0f);
		g.setColour(Colour(SIGNAL_COLOUR));
		auto f = GLOBAL_MONOSPACE_FONT();
		g.setFont(f);
		            
		auto tb = first.withSizeKeepingCentre(f.getStringWidthFloat(second), first.getHeight() + 40).constrainedWithin(lb);
		            
		if(first.getY() > 20)
			g.drawText(second, tb, Justification::centredTop);
		else
			g.drawText(second, tb, Justification::centredBottom);

		if(c.getComponent() == nullptr)
			return;

		if(auto ss = css.getForComponent(c.getComponent()))
		{
			auto b = first;

			auto mb = ss->getArea(b, { "margin", {}});
			auto pb = ss->getArea(mb, { "padding", {}});

			Colour paddingColour(0xFFB8C37F);
			Colour marginColour(0xFFB08354);

			{
				Graphics::ScopedSaveState sss(g);
				g.reduceClipRegion(b.toNearestInt());
				g.excludeClipRegion(mb.toNearestInt());
				g.fillAll(marginColour.withAlpha(0.33f));
			}

			{
				Graphics::ScopedSaveState sss(g);
				g.reduceClipRegion(mb.toNearestInt());
				g.excludeClipRegion(pb.toNearestInt());
				g.fillAll(paddingColour.withAlpha(0.33f));
			}

			g.setColour(paddingColour);
			g.drawRect(mb, 1);
			g.drawRect(pb, 1);
			g.setColour(marginColour);
			g.drawRect(b, 1);
		}
	}
}

CSSRootComponent::CSSDebugger::CSSDebugger(CSSRootComponent& componentToDebug, bool start):
	root(&componentToDebug),
	codeDoc(doc),
	editor(codeDoc),
	powerButton("debug-css", nullptr, *this),
	editButton("edit", nullptr, *this)
{
	doc.setDisableUndo(true);
	setName("CSS Inspector");
	addAndMakeVisible(editor);
	        
	editor.tokenCollection = new mcl::TokenCollection("CSS");
	editor.tokenCollection->setUseBackgroundThread(false);
	editor.setLanguageManager(new simple_css::LanguageManager(codeDoc));
	
	setSize(450, 800);
	setOpaque(true);

	setEditMode(false);

	GlobalHiseLookAndFeel::setDefaultColours(hierarchy);
	hierarchy.setLookAndFeel(&laf);
	addAndMakeVisible(hierarchy);
	addAndMakeVisible(powerButton);
	powerButton.setToggleModeWithColourChange(true);
	hierarchy.setTextWhenNothingSelected("Select parent component");
	addAndMakeVisible(powerButton);

	hierarchy.onChange = [&]()
	{
		auto pd = parentData[hierarchy.getSelectedItemIndex()];
		updateWithInspectorData(pd);
	};

	powerButton.onClick = [this]()
	{
		setActive(powerButton.getToggleState());
	};

	editButton.onClick = [&]()
	{
		this->setEditMode(editButton.getToggleState());
	};

	editSelector.onChange = [&]()
	{
		updateEditorText(currentlyEditedData, true);
	};

	editButton.setToggleModeWithColourChange(true);
	addAndMakeVisible(editButton);

	addAndMakeVisible(overlayMode);
	overlayMode.addItemList({ "None", "Hovered", "Selectors"}, 1);
	overlayMode.setSelectedItemIndex(0, dontSendNotification);
	overlayMode.setLookAndFeel(&laf);
	GlobalHiseLookAndFeel::setDefaultColours(overlayMode);

	addAndMakeVisible(editSelector);
	editSelector.setLookAndFeel(&laf);
	editSelector.setTextWhenNoChoicesAvailable("Selector");
	editSelector.setTextWhenNothingSelected("Selector");
	GlobalHiseLookAndFeel::setDefaultColours(editSelector);

	overlayMode.onChange = [&]()
	{
		auto om = (OverlayMode)overlayMode.getSelectedItemIndex();

		switch(om)
		{
		case OverlayMode::None:
			root->setCurrentInspectorData({});
			root->showInfo(false);
			break;
		case OverlayMode::Hovered:
			root->setCurrentInspectorData(hoverData);
			root->showInfo(false);
			break;
		case OverlayMode::Selectors:
			root->setCurrentInspectorData({});
			root->showInfo(true);
			break;
		default: ;
		}
	};

	getRootComponent()->addAndMakeVisible(this);
	setBounds(getRootComponent()->getLocalBounds());
	toFront(false);

	editButton.setTooltip("Edit the compiled stylesheet for the current UI element");
	overlayMode.setTooltip("Choose the overlay type for the CSS inspection");
	editSelector.setTooltip("Choose the selector that is used for editing the current CSS element");
	powerButton.setTooltip("Enable hover to select UI elements for CSS inspection");
	hierarchy.setTooltip("Select a parent UI element for CSS inspection");

	setActive(start);
}

CSSRootComponent::CSSDebugger::SelectListener::SelectListener(CSSDebugger& parent_):
	parent(parent_)
{
	

	if(auto pc = parent.getRootComponent())
		pc->addMouseListener(this, true);
}

CSSRootComponent::CSSDebugger::SelectListener::~SelectListener()
{
	if(auto pc = parent.getRootComponent())
		pc->removeMouseListener(this);
}

void CSSRootComponent::CSSDebugger::SelectListener::mouseMove(const MouseEvent& e)
{
	auto* target = e.eventComponent;

	if(target == parent.getRootComponent())
		return;

	auto hd = parent.createInspectorData(target);

	while(!hd && target->getParentComponent() != nullptr && target != parent.getRootComponent())
	{
		target = target->getParentComponent();
		hd = parent.createInspectorData(target);
	}

	if(hd != parent.hoverData)
	{
		parent.updateWithInspectorData(hd);
		parent.hoverData = hd;
		parent.repaint();
	}
}

void CSSRootComponent::CSSDebugger::SelectListener::mouseDown(const MouseEvent& e)
{
	parent.check();

	auto p = &parent;
	MessageManager::callAsync([p]()
	{
		p->selectListener = nullptr;
		p->powerButton.setToggleStateAndUpdateIcon(false);
		p->root->setCurrentInspectorData({});
		p->overlayMode.setSelectedItemIndex(0, sendNotificationAsync);
	});
}

bool CSSRootComponent::CSSDebugger::keyPressed(const KeyPress& key)
{
	if(key == KeyPress::F5Key && editMode)
	{
		auto code = doc.getAllContent();

		Parser p(code);

		auto ok = p.parse();

		if(ok.wasOk())
		{
			editor.clearWarningsAndErrors();
			auto css = p.getCSSValues();

			root->css.copyStyleSheetsFrom(currentlyEditedData.c.getComponent(), css);

			callRecursive<Component>(getRootComponent(), [&](Component* c)
			{
				if(auto fb = dynamic_cast<FlexboxComponent*>(c))
				{
					fb->setCSS(root->css);
				}

				c->resized();
				c->repaint();
				return false;
			});

		}
		else
		{
			editor.setError(ok.getErrorMessage());
		}
	}

	return false;
}

void CSSRootComponent::CSSDebugger::clear()
{
	hoverData = {};
	selectListener = nullptr;

	if(root != nullptr)
		root->setCurrentInspectorData({});
}

void CSSRootComponent::CSSDebugger::setEditMode(bool shouldBeEnabled)
{
	if(editMode != shouldBeEnabled)
	{
		editMode = shouldBeEnabled;
		auto fontSize = shouldBeEnabled ? 15.0f : 12.0f;
		editor.setFont(GLOBAL_MONOSPACE_FONT().withHeight(fontSize));
		doc.setDisableUndo(!shouldBeEnabled);
		doc.clearUndoHistory();
		editor.setReadOnly(!shouldBeEnabled);

		updateEditorText(currentlyEditedData, true);
	}
}

void CSSRootComponent::CSSDebugger::updateEditorText(const InspectorData& d, bool force)
{
	if(force || (d != currentlyEditedData))
	{
		currentlyEditedData = d;

		if(editMode)
		{
			if(auto c = currentlyEditedData.c.getComponent())
			{
				if(auto ss = root->css.getForComponent(c))
				{
					auto properties = ss->toStringForDefinedStates();
					auto code = FlexboxComponent::Helpers::toCSSCode(*c, properties);

					auto es = FlexboxComponent::Helpers::getElementSelector(*c).toString();

					auto lastTag = editSelector.getText();
					code = code.replace(es, lastTag);

					doc.clearUndoHistory();
					doc.replaceAllContent(code);
				}
				else
				{
					String code = editSelector.getText();

					code << " {\n  \n}\n";
					doc.clearUndoHistory();
					doc.replaceAllContent(code);
				}
			}
		}
		else
		{
			auto s = root->css.getDebugLogForComponent(currentlyEditedData.c.getComponent());
	        
			if(doc.getAllContent() != s)
				doc.replaceAllContent(s);
		}
	}
}

void CSSRootComponent::CSSDebugger::setActive(bool shouldBeActive)
{
	powerButton.setToggleStateAndUpdateIcon(shouldBeActive, false);

	clear();

	if(shouldBeActive)
	{
		overlayMode.setSelectedItemIndex((int)OverlayMode::Hovered, sendNotificationAsync);
		selectListener = new SelectListener(*this);
	}
		
}

Path CSSRootComponent::CSSDebugger::createPath(const String& url) const
{
	Path p;
	LOAD_EPATH_IF_URL("debug-css", ColumnIcons::debugCSS);
	LOAD_EPATH_IF_URL("bypass", HiBinaryData::ProcessorEditorHeaderIcons::bypassShape);
	LOAD_EPATH_IF_URL("edit", EditorIcons::penShape);
	return p;
}

void CSSRootComponent::CSSDebugger::paint(Graphics& g)
{
	g.fillAll(Colour(0xFF222222));

	if(hoverData)
	{
		auto b = getLocalBounds().removeFromTop(24);
		b.removeFromLeft(b.getHeight() + 10);
		g.setFont(GLOBAL_MONOSPACE_FONT());
		g.setColour(Colours::white.withAlpha(0.5f));
		g.drawText(hoverData.second, b.toFloat(), Justification::left);
	}
}

CSSRootComponent::InspectorData CSSRootComponent::CSSDebugger::createInspectorData(Component* c)
{
	auto b = getRootComponent()->getLocalArea(c, c->getLocalBounds()).toFloat();
	auto data = simple_css::FlexboxComponent::Helpers::dump(*c);

	InspectorData id;
	id.first = b;
	id.second = data;
	id.c = c;

	return id;
}

void CSSRootComponent::CSSDebugger::check()
{
	if(root == nullptr)
		return;
	        
	auto* target = Desktop::getInstance().getMainMouseSource().getComponentUnderMouse();

	bool change = false;

	if(target != nullptr && target->findParentComponentOfClass<simple_css::CSSRootComponent>() == root)
	{
		currentTarget = target;
		change = true;
	}
	        
	if(currentTarget.getComponent() != nullptr && change)
	{
		auto hd = createInspectorData(currentTarget.getComponent());
		auto tc = hd.c.getComponent();

		StringArray items;

		parentData.clear();

		while(tc != nullptr)
		{
			if(dynamic_cast<CSSRootComponent*>(tc) != nullptr)
				break;

			if(auto id = createInspectorData(tc))
			{
				if(!hd)
					hd = id;
				else
					parentData.addIfNotAlreadyThere(id);
			}

			tc = tc->getParentComponent();
		}

		int idx = 1;

		if(!parentData.contains(hd))
			parentData.insert(0, hd);

		hierarchy.clear(dontSendNotification);

		for(const auto& pd: parentData)
			hierarchy.addItem(pd.second, idx++);

		hierarchy.setText("", dontSendNotification);

		updateWithInspectorData(hd);
	}
}

void CSSRootComponent::CSSDebugger::updateWithInspectorData(const InspectorData& id)
{
	root->setCurrentInspectorData(id);

	editSelector.clear(dontSendNotification);

	auto selectors = StringArray::fromTokens(id.second, ",", "");

	selectors.trim();
	selectors.removeDuplicates(false);
	selectors.removeEmptyStrings();

	editSelector.addItemList(selectors, 1);
	editSelector.setSelectedItemIndex(0, dontSendNotification);

	updateEditorText(id, false);
}

void CSSRootComponent::CSSDebugger::resized()
{
	auto b = getLocalBounds();
	auto topArea = b.removeFromTop(24);

	auto cb = b.removeFromTop(32);

	overlayMode.setBounds(cb.removeFromLeft(120));
	editSelector.setBounds(cb.removeFromRight(110));
	hierarchy.setBounds(cb);

	powerButton.setBounds(topArea.removeFromLeft(topArea.getHeight()).reduced(2));
	editButton.setBounds(topArea.removeFromRight(topArea.getHeight()).reduced(2));
	        
	editor.setBounds(b);
}

#endif

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

	if(c.isGradient())
		g.setGradientFill(c.second);
	else
		g.setColour(c.first);
}
}

int Renderer::getPseudoClassState() const
{
	if(forceOverwriteState)
		return pseudoClassState;

	return currentComponent.first != nullptr ? getPseudoClassFromComponent(currentComponent.first) : pseudoClassState;
}

CodeGenerator::CodeGenerator(StyleSheet::Ptr ss_):
	ss(ss_)
{
	String nl = "\n";

	code << "drawBackground(Graphics& g, Rectangle<float> fullArea, PseudoElementType type=PseudoElementType::None)" << nl;

	code << "{" << nl;
	code << "\t" << nl;

	static const StringArray pseudoElementTypes({ "None", "Before", "After", "Before2", "After2" });

	for(int i = 0; i < (int)PseudoElementType::All; i++)
	{
		auto ps = PseudoState().withElement((PseudoElementType)i);

		if(ss->hasNonLayoutProperties((PseudoElementType)i))
		{
			code << "if(type == PseudoElementType::" + pseudoElementTypes[i] << ") {";

			for(const auto& ra: ss->getCodeGeneratorArea("fullArea", { "margin", ps }))
			{
				appendLine(ra);
			}

			auto c = ss->getCodeGeneratorColour("fullArea", { "background", {} });

			auto btl = ss->getCodeGeneratorPixelValueString("fullArea", { "border-top-left-radius", ps });
			auto btr = ss->getCodeGeneratorPixelValueString("fullArea", { "border-top-right-radius", ps });
			auto bbl = ss->getCodeGeneratorPixelValueString("fullArea", { "border-bottom-left-radius", ps });
			auto bbr = ss->getCodeGeneratorPixelValueString("fullArea", { "border-bottom-right-radius", ps });

			appendLine("g.setColour(" + c + ");");

			if(btl == btr && btl == bbl && btl == bbr)
			{
				if(btl.getFloatValue() == 0.0)
				{
					appendLine("g.fillRect(fullArea);");
				}
				else
				{
					appendLine("g.fillRoundedRectangle(fullArea, " + btl + ");");
				}
			}
			else
			{
				appendLine("g.fillPath(borderPath)");
			}
			
			code << "}";
		}
	}

	code << "};" << nl;
}

Renderer::Renderer(Component* c, StateWatcher& state_, int subComponentIndex):
	ScopedComponentSetter({c, subComponentIndex, {}})  ,
	currentComponent({c, subComponentIndex, {}}),
	state(state_)
{}

int Renderer::getPseudoClassFromComponent(Component* c)
{
	if(c == nullptr)
		return 0;

	int state = FlexboxComponent::Helpers::getManualPseudoState(*c);

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

	if(!scopy.isEmpty() && !pathToStroke.getBounds().isEmpty())
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
		auto hc = CSSRootComponent::find(*currentComponent.first);
		ScopedPointer<StyleSheet::Collection::DataProvider> dp = hc->createDataProvider();

		if(dp != nullptr)
		{
			auto img = dp->loadImage(imageURL);
			drawImage(g, img, area, ss, false);
		}
	}
	else
	{
		state.renderShadow(g, pathToFill, currentComponent, ss->getShadow(ma, { "box-shadow", defaultState}, false), false);
		setCurrentBrush(g, ss, ma, {"background-color", defaultState});
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
				setCurrentBrush(g, ss, ma, {"border-color", defaultState});
				g.strokePath(pathToStroke, PathStrokeType(borderSize));
			} 
		}
	}
	
	if(type == PseudoElementType::None && renderPseudoElements)
	{
		auto beforeAbsolute = ss->getPropertyValue({"position", PseudoState(0).withElement(PseudoElementType::Before)}).toString() == "absolute";
		auto beforeArea = ss->getPseudoArea(beforeAbsolute ? area : ma, stateFlag, PseudoElementType::Before);

		if(!beforeArea.isEmpty())
		{
			ScopedValueSetter<PseudoElementType> svs(currentlyRenderedPseudoElement, PseudoElementType::Before);
			Graphics::ScopedSaveState sss(g);
			drawBackground(g, beforeArea, ss, PseudoElementType::Before);

			if(renderTextWithBackground)
			{
				auto v = ss->getPropertyValueString({"content", PseudoState().withElement(PseudoElementType::Before)});

				if(v.isNotEmpty())
					renderText(g, beforeArea, v, ss, PseudoElementType::Before);
			}
            
            if(!beforeAbsolute)
            {
                if(beforeArea.getX() == ma.getX())
                    ma.removeFromLeft(beforeArea.getWidth());
                else if (beforeArea.getRight() == ma.getRight())
                    ma.removeFromRight(beforeArea.getWidth());
            }
                
		}
        
        auto beforeAbsolute2 = ss->getPropertyValue({"position", PseudoState(0).withElement(PseudoElementType::Before2)}).toString() == "absolute";
        auto beforeArea2 = ss->getPseudoArea(beforeAbsolute2 ? area : ma, stateFlag, PseudoElementType::Before2);
        
        if(!beforeArea2.isEmpty())
        {
            ScopedValueSetter<PseudoElementType> svs(currentlyRenderedPseudoElement, PseudoElementType::Before2);
            Graphics::ScopedSaveState sss(g);
            drawBackground(g, beforeArea2, ss, PseudoElementType::Before2);

			if(renderTextWithBackground)
			{
				auto v = ss->getPropertyValueString({"content", PseudoState().withElement(PseudoElementType::Before2)});

	            if(v.isNotEmpty())
	                renderText(g, beforeArea2, v, ss, PseudoElementType::Before2);
			}
        }


		auto afterAbsolute = ss->getPropertyValue({"position", PseudoState(0).withElement(PseudoElementType::After)}).toString() == "absolute";
		
		auto afterArea = ss->getPseudoArea(afterAbsolute ? area : ma, stateFlag, PseudoElementType::After);
		
		if(!afterArea.isEmpty())
		{
			ScopedValueSetter<PseudoElementType> svs(currentlyRenderedPseudoElement, PseudoElementType::After);
			Graphics::ScopedSaveState sss(g);
			drawBackground(g, afterArea, ss, PseudoElementType::After);

			if(renderTextWithBackground)
			{
				auto v = ss->getPropertyValueString({"content", PseudoState().withElement(PseudoElementType::After)});

				if(v.isNotEmpty())
					renderText(g, afterArea, v, ss, PseudoElementType::After);
			}
			
            
            if(!afterAbsolute)
            {
                if(afterArea.getX() == ma.getX())
                    ma.removeFromLeft(afterArea.getWidth());
                else if (afterArea.getRight() == ma.getRight())
                    ma.removeFromRight(afterArea.getWidth());
            }
		}
        
        auto afterAbsolute2 = ss->getPropertyValue({"position", PseudoState(0).withElement(PseudoElementType::After2)}).toString() == "absolute";
        
        auto afterArea2 = ss->getPseudoArea(afterAbsolute ? area : ma, stateFlag, PseudoElementType::After2);
        
        if(!afterArea2.isEmpty())
        {
            ScopedValueSetter<PseudoElementType> svs(currentlyRenderedPseudoElement, PseudoElementType::After2);
            Graphics::ScopedSaveState sss(g);
            drawBackground(g, afterArea2, ss, PseudoElementType::After2);

			if(renderTextWithBackground)
			{
				auto v = ss->getPropertyValueString({"content", PseudoState().withElement(PseudoElementType::After2)});

	            if(v.isNotEmpty())
	                renderText(g, afterArea2, v, ss, PseudoElementType::After2);
			}

			if(!afterAbsolute2)
            {
                if(afterArea2.getX() == ma.getX())
                    ma.removeFromLeft(afterArea2.getWidth());
                else if (afterArea2.getRight() == ma.getRight())
                    ma.removeFromRight(afterArea2.getWidth());
            }
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

		if(currentlyRenderedPseudoElement == PseudoElementType::None)
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
        drawBackground(g, clipBounds, ss, PseudoElementType::Before2);
        drawBackground(g, clipBounds, ss, PseudoElementType::After);
		drawBackground(g, clipBounds, ss, PseudoElementType::After2);
	}
}

void Renderer::renderText(Graphics& g, Rectangle<float> area, const String& text, StyleSheet::Ptr ss, PseudoElementType type, Justification jToUse, bool truncateBeforeAfter)
{
	auto currentState = PseudoState(getPseudoClassState()).withElement(type);

	auto textToDraw = ss->getText(text, currentState);

	if(textToDraw.isEmpty())
		return;

	auto totalArea = area;
		
	totalArea = ss->getArea(totalArea, { "margin", currentState });
	totalArea = ss->getArea(totalArea, { "padding", currentState });

	if(type == PseudoElementType::None && truncateBeforeAfter)
		totalArea = ss->truncateBeforeAndAfter(totalArea, currentState.stateFlag);
	
	g.setFont(ss->getFont(currentState, totalArea));

	auto j = jToUse.getFlags() ? jToUse : ss->getJustification(currentState);

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


