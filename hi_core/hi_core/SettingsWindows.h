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


#ifndef SETTINGSWINDOWS_H_INCLUDED
#define SETTINGSWINDOWS_H_INCLUDED

namespace hise { using namespace juce;



/** Contains all Setting windows that can popup and edit a specified XML file. */
struct SettingWindows: public Component,
		public ButtonListener,
		public QuasiModalComponent,
		public TextEditor::Listener,
		private ValueTree::Listener,
		private SafeChangeListener
{
public:

	enum ColourValues
	{
		bgColour = 0xFF444444,
		barColour = 0xFF2B2B2B,
		tabBgColour = 0xFF333333,

		overColour = 0xFF464646
	};

	using SettingList = Array<Identifier>;

	SettingWindows(HiseSettings::Data& data, const Array<Identifier>& menusToShow = {});

	

	~SettingWindows();

	void buttonClicked(Button* b) override;

	void resized() override;

	void paint(Graphics& g) override;

	void textEditorTextChanged(TextEditor&) override;

	void activateSearchBox()
	{
		fuzzySearchBox.grabKeyboardFocus();
	}

private:

	void changeListenerCallback(SafeChangeBroadcaster* /*b*/)
	{
		setContent(currentList);
	}

	void valueTreePropertyChanged(ValueTree& treeWhosePropertyHasChanged,
		const Identifier& property);

	void valueTreeChildAdded(ValueTree& , ValueTree& ) override;;

	void valueTreeChildRemoved(ValueTree& , ValueTree& , int ) override {};

	void valueTreeChildOrderChanged(ValueTree& ,int , int ) override {};

	void valueTreeParentChanged(ValueTree& ) override {}

	void fillPropertyPanel(const Identifier& s, PropertyPanel& panel, const String& searchText);

	void addProperty(ValueTree& c, Array<PropertyComponent*>& props);

	String getSettingNameToDisplay(const Identifier& s) const;

	ValueTree getValueTree(const Identifier& s) const;

	void save(const Identifier& s);


	class TabButtonLookAndFeel : public LookAndFeel_V3
	{
		void drawToggleButton(Graphics& g, ToggleButton& b, bool isMouseOverButton, bool isButtonDown) override;
	};

	HiseSettings::Data& dataObject;

	TabButtonLookAndFeel tblaf;

	BlackTextButtonLookAndFeel blaf;

	class Content;
	void setContent(SettingList s);

	ProjectHandler* handler;

	ScopedPointer<LookAndFeel> alaf;

	ToggleButton projectSettings;
	ToggleButton developmentSettings;
	ToggleButton docSettings;
	ToggleButton snexSettings;
	ToggleButton audioSettings;
	ToggleButton allSettings;

	Array<Identifier> settingsToShow;

	TextButton applyButton;
	TextButton cancelButton;
	TextButton undoButton;

	ScopedPointer<Content> currentContent;

	SettingList currentList;

	TextEditor fuzzySearchBox;

	bool saveOnDestroy = false;

	UndoManager undoManager;
	
};

} // namespace hise

#endif  // SETTINGSWINDOWS_H_INCLUDED
