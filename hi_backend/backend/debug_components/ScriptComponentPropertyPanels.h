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
*   which must be separately licensed for cloused source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

#ifndef SCRIPTCOMPONENTPROPERTYPANELS_H_INCLUDED
#define SCRIPTCOMPONENTPROPERTYPANELS_H_INCLUDED



/** A base class for all property panels that edit a selection of script components. 
*
*	Just add a component, register it as listener and call updateComponentProperties for the entire selection.
*/
class HiPropertyComponent : public PropertyComponent
{
public:

	HiPropertyComponent(const Identifier& id, ScriptComponentEditPanel* panel);

	virtual ~HiPropertyComponent() {};

	virtual Identifier getId() const { return propertyId; }
	
protected:

	/** Returns the value from the first selected component. */
	const var& getCurrentPropertyValue(bool returnUndefinedWhenMultipleSelection=true) const;

	PopupLookAndFeel plaf;

	Component::SafePointer<ScriptComponentEditPanel> panel;

private:

	Identifier propertyId;
};



class HiSliderPropertyComponent : public HiPropertyComponent,
								  public Slider::Listener,
								  public Label::Listener

{
public:

	HiSliderPropertyComponent(const Identifier& id, ScriptComponentEditPanel* panel);

	void sliderValueChanged(Slider *s) override;

	void labelTextChanged(Label* labelThatHasChanged) override;

	void refresh() override;

	void updateRange();

	

private:

	void updateInternal(const var& newValue);

	struct Comp : public Component
	{
		Comp(HiSliderPropertyComponent* parent);

		void resized() override
		{
			auto b = getLocalBounds();

			editor.setBounds(b.removeFromLeft(60));
			slider.setBounds(b);
		}

		Label editor;
		Slider slider;
	};

	Comp comp;
	

};


class HiChoicePropertyComponent : public HiPropertyComponent,
								  public ComboBox::Listener
{
public:

	HiChoicePropertyComponent(const Identifier& id, ScriptComponentEditPanel* panel);

	void comboBoxChanged(ComboBox* comboBoxThatHasChanged) override;

	void refresh() override;

private:

	ComboBox comboBox;
};


class HiTogglePropertyComponent : public HiPropertyComponent,
								  public ButtonListener
	
{
public:

	HiTogglePropertyComponent(const Identifier& id, ScriptComponentEditPanel* panel);

	void refresh() override;

	void buttonClicked(Button *) override;

	TextButton button;
};



class HiTextPropertyComponent : public HiPropertyComponent,
								public TextEditor::Listener
{
public:

	HiTextPropertyComponent(const Identifier& id, ScriptComponentEditPanel* panel, bool isMultiLine);

	void refresh() override;

	void textEditorReturnKeyPressed(TextEditor&) override;

	void textEditorFocusLost(TextEditor&) override;

private:

	void update();

	TextEditor editor;

	bool useNumberMode;
};

class HiColourPropertyComponent : public HiPropertyComponent
{
public:

	HiColourPropertyComponent(const Identifier& id, ScriptComponentEditPanel* panel);

	void refresh() override;

private:

	

	struct ColourComp : public Component,
						public Label::Listener,
						public ChangeListener
	{
		struct Popup : public Component
		{
			Popup(ColourComp* parent);

			void resized() override;

			ColourSelector selector;
		};

		ColourComp();

		void setDisplayedColour(Colour& c);

		void mouseDown(const MouseEvent& event);

		void changeListenerCallback(ChangeBroadcaster* b);

		void resized() override
		{
			l.setBounds(getLocalBounds().withWidth(80));
		}

		void labelTextChanged(Label* labelThatHasChanged) override;

		void paint(Graphics& g) override;

	private:

		void updateColour(Colour c);

		Label l;

		Colour colour;
	};

	ColourComp comp;
};

class HiFilePropertyComponent : 
	public HiPropertyComponent,
	public ButtonListener,
	public ComboBoxListener
{
public:

	HiFilePropertyComponent(const Identifier& id, ScriptComponentEditPanel* panel);

	void refresh() override;

	void buttonClicked(Button *) override;

	void comboBoxChanged(ComboBox *) override;

private:

	class CombinedComponent : public Component
	{
	public:
		CombinedComponent();

		void resized()
		{
			button.setBounds(getWidth() - 30, 0, 30, getHeight());
			box.setBounds(0, 0, getWidth() - 30, getHeight());
		}

		ComboBox box;
		TextButton button;

	private:

		


	};

	CombinedComponent combinedComponent;
};



#endif  // SCRIPTCOMPONENTPROPERTYPANELS_H_INCLUDED
