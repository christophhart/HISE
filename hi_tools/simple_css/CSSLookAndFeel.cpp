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

void StyleSheetLookAndFeel::drawButtonBackground(Graphics& g, Button& tb, const Colour& colour, bool cond, bool cond1)
{
	if(auto ed = tb.findParentComponentOfClass<CSSRootComponent>())
	{
		Renderer r(&tb, root.stateWatcher);
		
		if(auto ss = root.css.getForComponent(&tb))
		{
			ss->setDefaultColour("background-color", tb.findColour(TextButton::buttonColourId));

			auto currentState = Renderer::getPseudoClassFromComponent(&tb);
			ed->stateWatcher.checkChanges(&tb, ss, currentState);
			r.drawBackground(g, tb.getLocalBounds().toFloat(), ss);
		}
		else
		{
			LookAndFeel_V3::drawButtonBackground(g, tb, colour, cond, cond1);
		}
	}
}

bool StyleSheetLookAndFeel::drawButtonText(Graphics& g, Button* b)
{
	if(auto ed = b->findParentComponentOfClass<CSSRootComponent>())
	{
		Renderer r(b, root.stateWatcher);
			
		if(auto ss = root.css.getForComponent(b))
		{
			ss->setDefaultColour("color", b->findColour(TextButton::ColourIds::textColourOffId));
			r.renderText(g, b->getLocalBounds().toFloat(), b->getButtonText(), ss);
			return true;
		}
	}

	return false;
}

void StyleSheetLookAndFeel::drawButtonText(Graphics& g, TextButton& tb, bool over, bool down)
{
	if(!drawButtonText(g, &tb))
		LookAndFeel_V3::drawButtonText(g, tb, over, down);
}

void StyleSheetLookAndFeel::drawToggleButton(Graphics& g, ToggleButton& tb, bool shouldDrawButtonAsHighlighted,
	bool shouldDrawButtonAsDown)
{
	drawButtonBackground(g, tb, {}, shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
	drawButtonText(g, &tb);
}

void StyleSheetLookAndFeel::fillTextEditorBackground(Graphics& g, int width, int height, TextEditor& textEditor)
{
	if(auto ed = textEditor.findParentComponentOfClass<CSSRootComponent>())
	{
		Renderer r(&textEditor, root.stateWatcher);

		if(auto ss = root.css.getForComponent(&textEditor))
		{
			auto currentState = Renderer::getPseudoClassFromComponent(&textEditor);
			ed->stateWatcher.checkChanges(&textEditor, ss, currentState);

			ss->setDefaultColour("background-color", textEditor.findColour(TextEditor::backgroundColourId));
			ss->setDefaultColour("color", textEditor.findColour(TextEditor::textColourId));
			
			r.drawBackground(g, textEditor.getLocalBounds().toFloat(), ss);
		}
		else
		{
			LookAndFeel_V3::fillTextEditorBackground(g, width, height, textEditor);
			LookAndFeel_V3::drawTextEditorOutline(g, width, height, textEditor);
		}
	}
}

Font StyleSheetLookAndFeel::getPopupMenuFont()
{
	if(auto ss = getBestPopupStyleSheet(true))
	{
		return ss->getFont({}, {});
	}

	return LookAndFeel_V3::getPopupMenuFont();
}

void StyleSheetLookAndFeel::drawPopupMenuBackgroundWithOptions(Graphics& g, int width, int height,
	const PopupMenu::Options& o)
{
	Renderer r(nullptr, root.stateWatcher);

	if(auto ss = getBestPopupStyleSheet(false))
	{
		DBG(ss->toString());
		r.drawBackground(g, { (float)width, (float)height }, ss);
	}
		
	else
		LookAndFeel_V3::drawPopupMenuBackgroundWithOptions(g, width, height, o);
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
}

void StyleSheetLookAndFeel::drawPopupMenuItemWithOptions(Graphics& g, const Rectangle<int>& area, bool isHighlighted,
	const PopupMenu::Item& item, const PopupMenu::Options& options)
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

	DBG(flags);

	drawPopupMenuItem(g, area.toFloat(), flags, item.text, item.isSeparator);
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
}

StyleSheet::Ptr StyleSheetLookAndFeel::getBestPopupStyleSheet(bool getItem)
{
	if(getItem)
		return root.css.getWithAllStates(nullptr, Selector::withClass("popup-item"));

	return root.css.getWithAllStates(nullptr, Selector::withClass("popup"));
}

void StyleSheetLookAndFeel::getIdealPopupMenuItemSizeWithOptions(const String& text, bool isSeparator,
	int standardMenuItemHeight, int& idealWidth, int& idealHeight, const PopupMenu::Options& options)
{
	auto f = getPopupMenuFont();

	if(auto ss = getBestPopupStyleSheet(true))
	{
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
}

void StyleSheetLookAndFeel::drawPopupMenuSectionHeaderWithOptions(Graphics& g, const Rectangle<int>& area,
	const String& sectionName, const PopupMenu::Options& options)
{
	drawPopupMenuItem(g, area.toFloat(), (int)PseudoClassType::Focus, sectionName, false);
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
		LookAndFeel_V3::drawComboBox(g, width, height, isButtonDown, buttonX, buttonY, buttonW, buttonH, cb);
	}
}

void StyleSheetLookAndFeel::positionComboBoxText(ComboBox& cb, Label& label)
{
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
		LookAndFeel_V3::drawTableHeaderColumn(g, tableHeaderComponent, columnName, columnId, width, height,
	                                      isMouseOver, isMouseDown, columnFlags);
	}

	
}
}
}


