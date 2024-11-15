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
	StyleSheetLookAndFeel::StyleSheetLookAndFeel(CSSRootComponent& root_):
		root(root_)
	{
		
	}

void StyleSheetLookAndFeel::drawButtonBackground(Graphics& g, Button& tb, const Colour& colour, bool cond, bool cond1)
{
	if(auto ed = tb.findParentComponentOfClass<CSSRootComponent>())
	{
		if(auto ss = root.css.getForComponent(&tb))
		{
			Renderer r(&tb, root.stateWatcher);
			ss->setDefaultColour("background-color", tb.findColour(TextButton::buttonColourId));

			auto currentState = Renderer::getPseudoClassFromComponent(&tb);
			ed->stateWatcher.checkChanges(&tb, ss, currentState);
			r.drawBackground(g, tb.getLocalBounds().toFloat(), ss);

			return;
		}
	}

	GlobalHiseLookAndFeel::drawButtonBackground(g, tb, colour, cond, cond1);
}

bool StyleSheetLookAndFeel::drawButtonText(Graphics& g, Button* b)
{
	if(auto ed = b->findParentComponentOfClass<CSSRootComponent>())
	{
		if(auto ss = root.css.getForComponent(b))
		{
			Renderer r(b, root.stateWatcher);
			ss->setDefaultColour("color", b->findColour(TextButton::ColourIds::textColourOffId));
			r.renderText(g, b->getLocalBounds().toFloat(), b->getButtonText(), ss);
			return true;
		}
		
	}

	return false;
}

bool StyleSheetLookAndFeel::drawImageOnComponent(Graphics& g, Component* c, const Image& img)
{
	if(auto ss = root.css.getWithAllStates(c, Selector(ElementType::Image)))
	{
		Renderer r(c, root.stateWatcher);

		auto state = r.getPseudoClassFromComponent(c);
		root.stateWatcher.checkChanges(c, ss, state);
			
		r.drawImage(g, img, c->getLocalBounds().toFloat(), ss, true);
		return true;
	}

	return false;
}

void StyleSheetLookAndFeel::drawButtonText(Graphics& g, TextButton& tb, bool over, bool down)
{
	if(!drawButtonText(g, &tb))
		AlertWindowLookAndFeel::drawButtonText(g, tb, over, down);
}

void StyleSheetLookAndFeel::drawToggleButton(Graphics& g, ToggleButton& tb, bool shouldDrawButtonAsHighlighted,
	bool shouldDrawButtonAsDown)
{
	if(auto ed = tb.findParentComponentOfClass<CSSRootComponent>())
	{
		if(auto ss = root.css.getForComponent(&tb))
		{
			drawButtonBackground(g, tb, {}, shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
			drawButtonText(g, &tb);
			return;
		}
	}

	GlobalHiseLookAndFeel::drawToggleButton(g, tb, shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
}

void StyleSheetLookAndFeel::fillTextEditorBackground(Graphics& g, int width, int height, TextEditor& textEditor)
{
	if(auto ed = textEditor.findParentComponentOfClass<CSSRootComponent>())
	{
		if(auto ss = root.css.getForComponent(&textEditor))
		{
			Renderer r(&textEditor, root.stateWatcher);

			auto currentState = Renderer::getPseudoClassFromComponent(&textEditor);
			ed->stateWatcher.checkChanges(&textEditor, ss, currentState);

			ss->setDefaultColour("background-color", textEditor.findColour(TextEditor::backgroundColourId));
			ss->setDefaultColour("color", textEditor.findColour(TextEditor::textColourId));
			
			r.drawBackground(g, textEditor.getLocalBounds().toFloat(), ss);
		}
		else
		{
			GlobalHiseLookAndFeel::fillTextEditorBackground(g, width, height, textEditor);
			GlobalHiseLookAndFeel::drawTextEditorOutline(g, width, height, textEditor);
		}
	}
}

void StyleSheetLookAndFeel::drawLinearSlider(Graphics& g, int x, int y, int width, int height, float sliderPos,
	float minSliderPos, float maxSliderPos, const Slider::SliderStyle sliderStyle, Slider& slider)
{
	if(auto ss = root.css.getForComponent(&slider))
	{
		auto nr = NormalisableRange<double>(slider.getRange());
		nr.skew = slider.getSkewFactor();

		auto normPos = nr.convertTo0to1(slider.getValue());
			
		ss->setPropertyVariable("value", String(normPos, 4));
		
		auto text = slider.getTextFromValue(slider.getValue());
		
		Renderer r(&slider, root.stateWatcher);

		int currentState = 0;

		if(auto sp = slider.findParentComponentOfClass<SliderPack>())
		{
			currentState = sp->getHoverStateForSlider(&slider);
			r.setPseudoClassState(currentState, true);
		}
		else
			currentState = Renderer::getPseudoClassFromComponent(&slider);

		root.stateWatcher.checkChanges(&slider, ss, currentState);

		r.drawBackground(g, slider.getLocalBounds().toFloat(), ss);
		r.renderText(g, slider.getLocalBounds().toFloat(), text, ss, PseudoElementType::None, Justification(0), false);
	}
	else
	{
		GlobalHiseLookAndFeel::drawLinearSlider(g, x, y, width, height, sliderPos, minSliderPos, maxSliderPos,
		                                        sliderStyle, slider);
	}
}

void StyleSheetLookAndFeel::drawRotarySlider(Graphics& graphics, int x, int y, int width, int height,
	float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle, Slider& slider)
{
	if(auto ss = root.css.getForComponent(&slider))
	{
		auto normPos = NormalisableRange<double>(slider.getRange()).convertTo0to1(slider.getValue());
			
		ss->setPropertyVariable("value", String(normPos, 4));

		Renderer r(&slider, root.stateWatcher);

		auto currentState = Renderer::getPseudoClassFromComponent(&slider);
		root.stateWatcher.checkChanges(&slider, ss, currentState);

		r.drawBackground(graphics, slider.getLocalBounds().toFloat(), ss);
		auto t = slider.getTextFromValue(slider.getValue());
		r.renderText(graphics, slider.getLocalBounds().toFloat(), t, ss);
	}
	else
	{
		GlobalHiseLookAndFeel::drawRotarySlider(graphics, x, y, width, height, sliderPosProportional, rotaryStartAngle,
	                                        rotaryEndAngle, slider);
	}
	
}

void StyleSheetLookAndFeel::drawGenericComponentText(Graphics& g, const String& text, Component* c, Selector s)
{
	if(auto ss = s ? root.css.getWithAllStates(c, s) : root.css.getForComponent(c))
	{
		Renderer r(c, root.stateWatcher);
		r.renderText(g, c->getLocalBounds().toFloat(), text, ss);
	}
}

bool StyleSheetLookAndFeel::drawComponentBackground(Graphics& g, Component* c, Selector s)
{
	if(auto ss = s ? root.css.getWithAllStates(c, s) : root.css.getForComponent(c))
	{
		Renderer r(c, root.stateWatcher);

		auto state = r.getPseudoClassFromComponent(c);
		root.stateWatcher.checkChanges(c, ss, state);
		r.drawBackground(g, c->getLocalBounds().toFloat(), ss);
		return true;
	}

	return false;
}

bool StyleSheetLookAndFeel::drawListBoxRow(int rowNumber, Graphics& g, const String& text, Component* lb, int width,
	int height, bool rowIsSelected, bool rowIsHovered)
{
	if(lb == nullptr)
		return false;

	if(auto ss = root.css.getWithAllStates(lb, Selector(ElementType::TableRow)))
	{

		Renderer r(lb, root.stateWatcher);

		int state = 0;

		if(rowIsSelected)
			state |= (int)PseudoClassType::Checked;

		if(rowIsHovered)
			state |= (int)PseudoClassType::Hover;

		if(auto lbParent = lb->findParentComponentOfClass<ListBox>())
		{
			if(rowIsHovered && lbParent->isMouseButtonDown(true))
				state |= (int)PseudoClassType::Active;
		}
				
		r.setPseudoClassState(state, true);
			
		root.stateWatcher.checkChanges(lb, ss, state);

		Rectangle<float> bounds(0.0f, 0.0f, (float)width, (float)height);

		r.drawBackground(g, bounds, ss);
		r.renderText(g, bounds, text, ss);

		return true;
	}

	return false;
}

Font StyleSheetLookAndFeel::getPopupMenuFont()
{
	if(auto ss = getBestPopupStyleSheet(true))
	{
		return ss->getFont({}, {});
	}

	return GlobalHiseLookAndFeel::getPopupMenuFont();
}

void StyleSheetLookAndFeel::drawPopupMenuBackgroundWithOptions(Graphics& g, int width, int height,
	const PopupMenu::Options& o)
{
	if(auto ss = getBestPopupStyleSheet(false))
	{
		Renderer r(nullptr, root.stateWatcher);
		r.drawBackground(g, { (float)width, (float)height }, ss);
	}
	else
		GlobalHiseLookAndFeel::drawPopupMenuBackgroundWithOptions(g, width, height, o);
}

void StyleSheetLookAndFeel::drawPopupMenuItem(Graphics& g, Rectangle<float> area, int flags, const String& text,
	bool isSeparator)
{
	if(auto ss = getBestPopupStyleSheet(true))
	{
		Renderer r(nullptr, root.stateWatcher);

		r.setPseudoClassState(flags);

		r.drawBackground(g, area.toFloat(), ss);

		if(isSeparator)
		{
			auto sep = ss->getArea(area.toFloat(), { "padding", flags});

			if(auto rss = root.css.getWithAllStates(nullptr, ElementType::Ruler))
			{
				sep = rss->getArea(sep, { "margin", flags});
				sep = rss->getArea(sep, { "padding", flags});
				auto f = rss->getPixelValue(area.toFloat(), { "border-width", flags});
				sep = sep.withSizeKeepingCentre(sep.getWidth(), f);
				r.setCurrentBrush(g, rss, sep, {"border-color", flags }, Colours::black);
				auto radius = rss->getPixelValue(sep, { "border-top-left-radius", flags }, 0.0);
				g.fillRoundedRectangle(sep, radius);
			}
			else
			{
				r.setCurrentBrush(g, ss, sep, {"color", flags }, Colours::black);
				sep = sep.withSizeKeepingCentre(sep.getWidth(), 1.0f);
				g.fillRect(sep);
			}
		}
		else
		{
			r.renderText(g, area.toFloat(), text, ss);
		}
	}
	else
	{
		jassertfalse;
	}
}

void StyleSheetLookAndFeel::drawPopupMenuItemWithOptions(Graphics& g, const Rectangle<int>& area, bool isHighlighted,
	const PopupMenu::Item& item, const PopupMenu::Options& options)
{
	if(auto ss = getBestPopupStyleSheet(true))
	{
		int flags = 0;

		if(isHighlighted && !(item.isSeparator || item.isSectionHeader))
			flags |= (int)PseudoClassType::Hover;

		if(item.isTicked)
			flags |= (int)PseudoClassType::Active;

		if(!item.isEnabled)
			flags |= (int)PseudoClassType::Disabled;

		if(item.subMenu != nullptr)
			flags |= (int)PseudoClassType::Root;

		drawPopupMenuItem(g, area.toFloat(), flags, item.text, item.isSeparator);
	}
	else
	{
		GlobalHiseLookAndFeel::drawPopupMenuItemWithOptions(g, area, isHighlighted, item, options);
	}
}

void StyleSheetLookAndFeel::drawScrollbar(Graphics& g, ScrollBar& scrollbar, int x, int y, int width, int height,
	bool isScrollbarVertical, int thumbStartPosition, int thumbSize, bool isMouseOver, bool isMouseDown)
{
	if(auto ss = root.css.getWithAllStates(&scrollbar, Selector(ElementType::Scrollbar)))
	{
		Renderer r(&scrollbar, root.stateWatcher);

		int state = 0;

		if(isMouseOver || isMouseDown)
			state |= (int)PseudoClassType::Hover;

		if(isMouseDown)
			state |= (int)PseudoClassType::Active;
			
		r.setPseudoClassState(state, true);

		root.stateWatcher.checkChanges(&scrollbar, ss, state);

		Rectangle<float> b;

		if(isScrollbarVertical)
			b = { (float)x, (float)(y + thumbStartPosition), (float)width, (float)thumbSize };
		else
			b = { (float)(x + thumbStartPosition), (float)y, (float)thumbSize, (float)height };

		r.drawBackground(g, b, ss);
		return;
	}

	fallback.drawScrollbar(g, scrollbar, x, y, width, height, isScrollbarVertical, thumbStartPosition, thumbSize, isMouseOver, isMouseDown);
}

void StyleSheetLookAndFeel::initComponent(Component* c, Selector s)
{
	if(auto ss = s ? root.css.getWithAllStates(c, s) : root.css.getForComponent(c))
	{
		ss->setupComponent(&root, c, 0);
	}
}

void StyleSheetLookAndFeel::drawProgressBar(Graphics& g, ProgressBar& progressBar, int width, int height,
                                            double progress, const String& textToShow)
{
	if(auto ss = root.css.getForComponent(&progressBar))
	{
		Renderer r(&progressBar, root.stateWatcher);

		auto currentState = Renderer::getPseudoClassFromComponent(&progressBar);
		root.stateWatcher.checkChanges(&progressBar, ss, currentState);

		ss->setPropertyVariable("progress", String(progress * 100) + "%");

		r.drawBackground(g, progressBar.getLocalBounds().toFloat(), ss);
		r.renderText(g, progressBar.getLocalBounds().toFloat(), textToShow, ss);
	}
	else
	{
		GlobalHiseLookAndFeel::drawProgressBar(g, progressBar, width, height, progress, textToShow);
	}
}

StyleSheet::Ptr StyleSheetLookAndFeel::getBestPopupStyleSheet(bool getItem)
{
	return root.css.getWithAllStates(nullptr, Selector::withClass(getItem ? "popup-item" : "popup"));
}

void StyleSheetLookAndFeel::getIdealPopupMenuItemSizeWithOptions(const String& text, bool isSeparator,
	int standardMenuItemHeight, int& idealWidth, int& idealHeight, const PopupMenu::Options& options)
{
	if(auto ss = getBestPopupStyleSheet(true))
	{
		auto f = getPopupMenuFont();

		auto textWidth = f.getStringWidthFloat(ss->getText(text, {}));
		auto h = f.getHeight();

		int state = 0;
		if(standardMenuItemHeight == -1)
			state = (int)PseudoClassType::Focus;

		auto hv = ss->getPropertyValueString({ "height", state });

		if(hv.isNotEmpty())
		{
			h = ExpressionParser::evaluate(hv, { false, {h, h}, f.getHeight() });
		}

		Rectangle<float> ta(textWidth, h);

		auto pw = ss->getPseudoArea(ta, state, PseudoElementType::Before).getWidth();
		pw += ss->getPseudoArea(ta, state, PseudoElementType::After).getWidth();

		if(pw != 0)
			ta = ta.withSizeKeepingCentre(ta.getWidth() + pw, ta.getHeight());
			
		ta = ss->expandArea(ta, { "padding", state });
		ta = ss->expandArea(ta, { "margin", state });
			
		idealWidth = roundToInt(ta.getWidth());
		idealHeight = roundToInt(ta.getHeight());
	}
	else
	{
		GlobalHiseLookAndFeel::getIdealPopupMenuItemSizeWithOptions(text, isSeparator, standardMenuItemHeight, idealWidth, idealHeight, options);
	}
}

void StyleSheetLookAndFeel::drawPopupMenuSectionHeaderWithOptions(Graphics& g, const Rectangle<int>& area,
	const String& sectionName, const PopupMenu::Options& options)
{
	drawPopupMenuItem(g, area.toFloat(), (int)PseudoClassType::Focus, sectionName, false);
}

void StyleSheetLookAndFeel::drawLabel(Graphics& g, Label& l)
{
	if(auto ss = root.css.getForComponent(&l))
	{
		Renderer r(&l, root.stateWatcher);

		auto state = r.getPseudoClassState();

		if(l.isBeingEdited())
			state |= (int)PseudoClassType::Focus;
		else
			state &= (0xFFFFFFFF ^ (int)PseudoClassType::Focus);


		r.setPseudoClassState(state, true);

		root.stateWatcher.checkChanges(&l, ss, r.getPseudoClassState());

		r.drawBackground(g, l.getLocalBounds().toFloat(), ss);

		if(!l.isBeingEdited())
			r.renderText(g, l.getLocalBounds().toFloat(), l.getText(), ss);
	}
	else
	{
		GlobalHiseLookAndFeel::drawLabel(g, l);
	}
}

Font StyleSheetLookAndFeel::getLabelFont(Label& label)
{
	if(auto ss = root.css.getForComponent(&label))
	{
		return ss->getFont({}, label.getLocalBounds().toFloat());
	}
	else
		return GlobalHiseLookAndFeel::getLabelFont(label);
}

void StyleSheetLookAndFeel::drawComboBox(Graphics& g, int width, int height, bool isButtonDown, int buttonX,
                                         int buttonY, int buttonW, int buttonH, ComboBox& cb)
{
	if(auto ss = root.css.getForComponent(&cb))
	{
		Renderer r(&cb, root.stateWatcher);
			
		root.stateWatcher.checkChanges(&cb, ss, r.getPseudoClassState());

		r.drawBackground(g, cb.getLocalBounds().toFloat(), ss);

		r.renderText(g, cb.getLocalBounds().toFloat(), cb.getText(), ss);
	}
	else
	{
		GlobalHiseLookAndFeel::drawComboBox(g, width, height, isButtonDown, buttonX, buttonY, buttonW, buttonH, cb);
	}
}

void StyleSheetLookAndFeel::positionComboBoxText(ComboBox& cb, Label& label)
{
	// position it but then hide it so that the popup menu size will be initialised correctly (wtf, 1 hour of debugging for this...)
	GlobalHiseLookAndFeel::positionComboBoxText(cb, label);
	label.setVisible(false);
}

void StyleSheetLookAndFeel::drawTableHeaderBackground(Graphics& graphics, TableHeaderComponent& tableHeaderComponent)
{
	
}

void StyleSheetLookAndFeel::drawTableHeaderColumn(Graphics& g, TableHeaderComponent& tableHeaderComponent,
	const String& columnName, int columnId, int width, int height, bool isMouseOver, bool isMouseDown, int columnFlags)
{
	if(auto ss = root.css.getForComponent(&tableHeaderComponent))
	{
		Renderer r(nullptr, root.stateWatcher);

		int flags = 0;

        auto columnIndex = tableHeaderComponent.getIndexOfColumnId(columnId, true);
        
        auto isFirst = columnIndex == 0;
        auto isLast = (tableHeaderComponent.getNumColumns(true)-1) == columnIndex;
        
        if(isFirst)
            flags |= (int)PseudoClassType::First;

        if(isLast)
            flags |= (int)PseudoClassType::Last;
        
		if(isMouseDown)
			flags |= (int)PseudoClassType::Active;

		if(isMouseOver)
			flags |= (int)PseudoClassType::Hover;
        
        uint32 firstLastMask = 0xFFFFFFFF;
        firstLastMask ^= (uint32)PseudoClassType::First;
        firstLastMask ^= (uint32)PseudoClassType::Last;

		root.stateWatcher.checkChanges(&tableHeaderComponent, ss, flags & firstLastMask);

		r.setPseudoClassState(flags);

		auto area = Rectangle<float>(0.0f, 0.0f, (float)width, (float)height);

		r.drawBackground(g, area, ss);
		r.renderText(g, area, columnName, ss);
	}
	else
	{
		GlobalHiseLookAndFeel::drawTableHeaderColumn(g, tableHeaderComponent, columnName, columnId, width, height,
	                                      isMouseOver, isMouseDown, columnFlags);
	}

	
}
}
}


