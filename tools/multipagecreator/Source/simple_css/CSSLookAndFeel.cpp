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
		Renderer r(&tb, state);

		Selector s_id(SelectorType::ID, tb.getName().toLowerCase());
		Selector s_type(ElementType::Button);

		if(auto ss = css.getOrCreateCascadedStyleSheet({s_type, s_id}))
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

void StyleSheetLookAndFeel::drawButtonText(Graphics& g, TextButton& tb, bool over, bool down)
{
	if(auto ed = tb.findParentComponentOfClass<CSSRootComponent>())
	{
		Renderer r(&tb, state);

		

		Selector s_id(SelectorType::ID, tb.getName().toLowerCase());
		Selector s_type(ElementType::Button);

		if(auto ss = css.getOrCreateCascadedStyleSheet({s_type, s_id}))
		{
			ss->setDefaultColour("color", tb.findColour(TextButton::ColourIds::textColourOffId));

			r.renderText(g, tb.getLocalBounds().toFloat(), tb.getButtonText(), ss);
		}
		else
		{
			LookAndFeel_V3::drawButtonText(g, tb, over, down);
		}
	}
		
}

void StyleSheetLookAndFeel::fillTextEditorBackground(Graphics& g, int width, int height, TextEditor& textEditor)
{
	if(auto ed = textEditor.findParentComponentOfClass<CSSRootComponent>())
	{
		Renderer r(&textEditor, state);

		Selector s_id(SelectorType::ID, textEditor.getName().toLowerCase());
		Selector s_type(ElementType::TextInput);

		if(auto ss = css.getOrCreateCascadedStyleSheet({s_type, s_id}))
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
		return ss->getFont(0, {});
	}

	return LookAndFeel_V3::getPopupMenuFont();
}

void StyleSheetLookAndFeel::drawPopupMenuBackgroundWithOptions(Graphics& g, int width, int height,
	const PopupMenu::Options& o)
{
	Renderer r(nullptr, state);

	if(auto ss = getBestPopupStyleSheet(false))
		r.drawBackground(g, { (float)width, (float)height }, ss);
	else
		LookAndFeel_V3::drawPopupMenuBackgroundWithOptions(g, width, height, o);
}

void StyleSheetLookAndFeel::drawPopupMenuItem(Graphics& g, Rectangle<float> area, int flags, const String& text,
	bool isSeparator)
{
	if(auto ss = getBestPopupStyleSheet(true))
	{
		Renderer r(nullptr, state);

		r.setPseudoClassState(flags);

		r.drawBackground(g, area.toFloat(), ss);

		if(isSeparator)
		{
			auto sep = ss->getArea(area.toFloat(), { "padding", flags});

			if(auto rss = css[ElementType::Ruler])
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

StyleSheet::Ptr StyleSheetLookAndFeel::getBestPopupStyleSheet(bool getItem)
{
	if(getItem)
	{
		if(auto c = css.getOrCreateCascadedStyleSheet({ Selector::withClass("popup"), Selector::withClass("popup-item")}))
			return c;
	}

	return css[Selector::withClass("popup")];
}

void StyleSheetLookAndFeel::getIdealPopupMenuItemSizeWithOptions(const String& text, bool isSeparator,
	int standardMenuItemHeight, int& idealWidth, int& idealHeight, const PopupMenu::Options& options)
{
	auto f = getPopupMenuFont();

	if(auto ss = getBestPopupStyleSheet(true))
	{
		auto textWidth = f.getStringWidthFloat(ss->getText(text, 0));
		auto h = f.getHeight();

		int state = 0;
		if(standardMenuItemHeight == -1)
			state = (int)PseudoClassType::Focus;

		if(auto v = ss->getPropertyValue({ "height", state }))
		{
			h = ExpressionParser::evaluate(v.valueAsString, { false, {h, h}, f.getHeight() });
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
	if(auto ss = css[ElementType::Selector])
	{
		Renderer r(&cb, state);
			
		state.checkChanges(&cb, ss, r.getPseudoClassState());

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
#if 0
		if(auto ss = css[ElementType::Selector])
		{
			auto state = Renderer::getPseudoClassFromComponent(&cb);

			auto area = cb.getLocalBounds().toFloat();

			area = ss->getArea(area, { "margin", state });
			area = ss->getArea(area, { "padding", state });

			area = ss->truncateBeforeAndAfter(area, state);


			label.setBounds (area.toNearestInt());
			label.setFont (ss->getFont(state, area));
			label.setColour(Label::ColourIds::textColourId, ss->getColourOrGradient(area, { "color", state }, label.findColour(Label::textColourId)).first);
			label.setJustificationType(ss->getJustification(state, Justification::left));
		}
		else
		{
			LookAndFeel_V3::positionComboBoxText(cb, label);
		}
#endif
}
}
}


