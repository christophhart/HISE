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

namespace hise {
namespace simple_css
{
using namespace juce;

/** A look and feel class that applies CSS styling to most stock JUCE controls.
 *
 *  Currently supported:
 *
 *	- text buttons
 *	- text editors
 *	- comboboxes
 *	- popup menus
 *
 */
struct StyleSheetLookAndFeel: public LookAndFeel_V3
{
	/** Creates a style sheet LAF using a compiled CSS collection and a state watcher that
	 *  will track the pseudo class state of each component. */
	StyleSheetLookAndFeel(CSSRootComponent& root_):
	  root(root_)
	{
		setColour(PopupMenu::backgroundColourId, Colours::transparentBlack);
	};

	/** Uses the selector "button [#id]". */
	void drawButtonBackground(Graphics& g, Button& tb, const Colour&, bool, bool) override;

	bool drawButtonText(Graphics& g, Button* b);

	/** Uses the selector "button [#id]". */
	void drawButtonText(Graphics& g, TextButton& tb, bool over, bool down) override;

	void drawToggleButton(Graphics& g, ToggleButton& tb, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

	/** Uses the selector "input [#id]". */
	void fillTextEditorBackground (Graphics&, int width, int height, TextEditor&) override;

	/** Uses the selector "input [#id]". */
	void drawTextEditorOutline (Graphics&, int width, int height, TextEditor&) override {};

	/** Uses the selector ".popup and .popup-item". */
	Font getPopupMenuFont() override;

	/** Uses the selector ".popup". */
	void drawPopupMenuBackgroundWithOptions (Graphics& g, int width, int height, const PopupMenu::Options& o) override;

	/** Uses the selector ".popup-item". Supports the pseudo classes:
	 *  - "hover" (highlighted)
	 *  - "active" (ticked)
	 *  - "focus" (header)
	 *	- "root" (submenu)
	 */
	void drawPopupMenuItem(Graphics& g, Rectangle<float> area, int flags, const String& text, bool isSeparator);

    void drawPopupMenuItemWithOptions (Graphics& g, const Rectangle<int>& area,
                                               bool isHighlighted,
                                               const PopupMenu::Item& item,
                                               const PopupMenu::Options&) override;

	void drawProgressBar(Graphics& g, ProgressBar& pb, int width, int height, double progress, const String& textToShow) override;

	bool isProgressBarOpaque(ProgressBar& b) override { return false; }

	StyleSheet::Ptr getBestPopupStyleSheet(bool getItem);

	void getIdealPopupMenuItemSizeWithOptions (const String& text,
	                                                   bool isSeparator,
	                                                   int standardMenuItemHeight,
	                                                   int& idealWidth,
	                                                   int& idealHeight,
	                                                   const PopupMenu::Options& options) override;

	void drawPopupMenuSectionHeaderWithOptions (Graphics& g, const Rectangle<int>& area,
	                                            const String& sectionName,
	                                            const PopupMenu::Options&) override;

	/** Uses the selector "select". */
	void drawComboBox (Graphics& g, int width, int height, bool isButtonDown,
	                   int buttonX, int buttonY, int buttonW, int buttonH,
	                   ComboBox& cb) override;

	Font getComboBoxFont (ComboBox&) override { return getPopupMenuFont(); }
	void positionComboBoxText (ComboBox& cb, Label& label) override;
	void drawComboBoxTextWhenNothingSelected (Graphics&, ComboBox&, Label&) override { }

	void drawTableHeaderBackground (Graphics&, TableHeaderComponent&) override;

    void drawTableHeaderColumn (Graphics&, TableHeaderComponent&, const String& columnName, int columnId, int width, int height, bool isMouseOver, bool isMouseDown, int columnFlags) override;

private:

	CSSRootComponent& root;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StyleSheetLookAndFeel);
};

	
} // namespace simple_css
} // namespace hise
