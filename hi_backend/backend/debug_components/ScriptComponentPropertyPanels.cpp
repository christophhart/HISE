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

#if 0
class JavascriptColourSelector : public Component,
	public ScriptComponentEditListener
{
public:

	JavascriptColourSelector(ReferenceCountedObject* editedComponent, const Identifier& colourId) :
		ScriptComponentEditListener(dynamic_cast<ScriptingApi::Content::ScriptComponent*>(editedComponent)->getScriptProcessor()->getMainController_()),
		selector(ColourSelector::showAlphaChannel | ColourSelector::showColourspace | ColourSelector::showSliders)
	{
		addAndMakeVisible(selector);



		//auto currentVar = editedComponent->getScriptObjectProperty(editedComponent->getIdFor(colourId));

		//selector.setCurrentColour(Colour((int)currentVar));

		selector.setColour(ColourSelector::ColourIds::backgroundColourId, Colours::transparentBlack);
		selector.setColour(ColourSelector::ColourIds::labelTextColourId, Colours::white);

		setSize(300, 300);
	}

	void scriptComponentChanged(ReferenceCountedObject *componentThatWasChanged, Identifier idThatWasChanged) override
	{
		delete this;
	}

	void scriptComponentUpdated(Identifier idThatWasChanged, var newValue) override
	{
		if (idThatWasChanged == colourId)
		{
			selector.setCurrentColour(Colour((int)newValue), dontSendNotification);
		}
	}

	void resized() override
	{
		selector.setBounds(getLocalBounds());
	}

private:

	Identifier colourId;


	ColourSelector selector;

};
#endif

HiPropertyComponent::HiPropertyComponent(const Identifier& id, ScriptComponentEditPanel* panel_):
	PropertyComponent(id.toString()),
	propertyId(id),
	panel(panel_)
{
	
}

const var& HiPropertyComponent::getCurrentPropertyValue(int indexInSelection/*=0*/) const
{
	auto first = panel->getScriptComponentEditBroadcaster()->getFirstFromSelection();
	return first->getScriptObjectProperties()->getProperty(getId());
}

HiSliderPropertyComponent::HiSliderPropertyComponent(const Identifier& id, ScriptComponentEditPanel* panel) :
	HiPropertyComponent(id, panel)
{
	addAndMakeVisible(slider = new Slider(id.toString()));

	slider->setTextBoxStyle(Slider::TextEntryBoxPosition::NoTextBox, false, 80, 20);

	slider->setColour(Slider::backgroundColourId, Colour(0xfb282828));
	slider->setColour(Slider::thumbColourId, Colour(0xff777777));
	slider->setColour(Slider::trackColourId, Colour(0xff222222));
	slider->setColour(Slider::textBoxTextColourId, Colours::white);
	slider->setColour(Slider::textBoxOutlineColourId, Colour(0x45ffffff));

	slider->setScrollWheelEnabled(false);

	slider->addListener(this);

	refresh();

}



void HiSliderPropertyComponent::sliderValueChanged(Slider *s)
{
	var newValue(s->getValue());

	panel->getScriptComponentEditBroadcaster()->setScriptComponentPropertyForSelection(getId(), newValue, sendNotification);
}

void HiSliderPropertyComponent::refresh()
{
	updateRange();

	auto newValue = (double)getCurrentPropertyValue();

	if (slider->getValue() != newValue)
	{
		slider->setValue(newValue, dontSendNotification);
	}
}



void HiSliderPropertyComponent::updateRange()
{
	int oldRange = (double)slider->getMaximum();

	static const Identifier x("x");
	static const Identifier y("y");
	static const Identifier w("width");
	static const Identifier h("height");

	const bool isSize = getId() == w || getId() == h;

	ScriptComponent* sc = panel->getScriptComponentEditBroadcaster()->getFirstFromSelection();

	int maxWidth = sc->parent->getContentWidth();
	int maxHeight = sc->parent->getContentHeight();

	auto parent = sc->parent->getComponent(sc->getParentComponentIndex());

	if (parent != nullptr)
	{
		maxWidth = parent->getScriptObjectProperty(ScriptComponent::Properties::width);
		maxHeight = parent->getScriptObjectProperty(ScriptComponent::Properties::height);
	}

	int range;

	if (getId() == w)
	{
		range = maxWidth - (int)sc->getScriptObjectProperty(ScriptComponent::Properties::x);
	}
	else if (getId() == h)
	{
		range = maxHeight - (int)sc->getScriptObjectProperty(ScriptComponent::Properties::y);
	}
	else if (getId() == x)
	{
		range = maxWidth - (int)sc->getScriptObjectProperty(ScriptComponent::Properties::width);
	}
	else
	{
		range = maxHeight - (int)sc->getScriptObjectProperty(ScriptComponent::Properties::height);
	}

	if (oldRange != range)
	{
		slider->setRange(0.0, (double)range, 1);
		slider->repaint();
	}
}

HiChoicePropertyComponent::HiChoicePropertyComponent(const Identifier& id, ScriptComponentEditPanel* panel):
	HiPropertyComponent(id, panel)
{
	addAndMakeVisible(comboBox);

	comboBox.addListener(this);

	setLookAndFeel(&plaf);

	refresh();
}


void HiChoicePropertyComponent::comboBoxChanged(ComboBox* comboBoxThatHasChanged)
{
	auto b = panel->getScriptComponentEditBroadcaster();

	var newValue = comboBoxThatHasChanged->getText();
	b->setScriptComponentPropertyForSelection(getId(), newValue, sendNotification);
}

void HiChoicePropertyComponent::refresh()
{
	auto sc = panel->getScriptComponentEditBroadcaster()->getFirstFromSelection();

	if (sc != nullptr)
	{
		comboBox.clear(dontSendNotification);
		auto sa = sc->getOptionsFor(getId());
		comboBox.addItemList(sa, 1);
		auto selectedText = getCurrentPropertyValue().toString();
		comboBox.setText(selectedText, dontSendNotification);
	}
}



HiTogglePropertyComponent::HiTogglePropertyComponent(const Identifier& id, ScriptComponentEditPanel* panel):
	HiPropertyComponent(id, panel)
{

	addAndMakeVisible(button);

	button.setLookAndFeel(&plaf);

	button.setConnectedEdges(Button::ConnectedOnLeft | Button::ConnectedOnRight);
	button.setColour(TextButton::buttonColourId, Colour(0x4c4b4b4b));
	button.setColour(TextButton::buttonOnColourId, Colours::white.withAlpha(0.7f));
	button.setColour(TextButton::textColourOnId, Colour(0xaa000000));
	button.setColour(TextButton::textColourOffId, Colour(0x99ffffff));

	button.addListener(this);

	
}

void HiTogglePropertyComponent::refresh()
{
	const bool on = (bool)getCurrentPropertyValue();

	button.setButtonText(on ? "Enabled" : "Disabled");

	button.setToggleState(on, dontSendNotification);
}

void HiTogglePropertyComponent::buttonClicked(Button *)
{
	auto b = panel->getScriptComponentEditBroadcaster();

	b->setScriptComponentPropertyForSelection(getId(), !button.getToggleState(), sendNotification);
}


HiTextPropertyComponent::HiTextPropertyComponent(const Identifier& id, ScriptComponentEditPanel* panel, bool isMultiline):
	HiPropertyComponent(id, panel),
	useNumberMode(false)
{
	addAndMakeVisible(editor);

	if (getId() == Identifier("min") || getId() == Identifier("max"))
	{
		useNumberMode = true;
	}

	editor.setMultiLine(isMultiline);
	editor.addListener(this);

	setLookAndFeel(&plaf);
}


void HiTextPropertyComponent::refresh()
{
	editor.setText(getCurrentPropertyValue().toString());
}

void HiTextPropertyComponent::textEditorFocusLost(TextEditor&)
{
	update();
}


void HiTextPropertyComponent::textEditorReturnKeyPressed(TextEditor&)
{
	update();
}


void HiTextPropertyComponent::update()
{
	auto b = panel->getScriptComponentEditBroadcaster();

	if (useNumberMode)
	{
		var number(editor.getText().getDoubleValue());
		b->setScriptComponentPropertyForSelection(getId(), number, sendNotification);
	}
	else
	{
		var text(editor.getText());
		b->setScriptComponentPropertyForSelection(getId(), text, sendNotification);
	}
}


HiFilePropertyComponent::HiFilePropertyComponent(const Identifier& id, ScriptComponentEditPanel* panel):
	HiPropertyComponent(id, panel)
{
	addAndMakeVisible(combinedComponent);
	combinedComponent.setLookAndFeel(&plaf);
	combinedComponent.box.setLookAndFeel(&plaf);
	combinedComponent.box.addListener(this);
	combinedComponent.button.addListener(this);
}

void HiFilePropertyComponent::refresh()
{
	auto sc = panel->getScriptComponentEditBroadcaster()->getFirstFromSelection();

	if (sc != nullptr)
	{
		combinedComponent.box.clear(dontSendNotification);
		auto sa = sc->getOptionsFor(getId());
		combinedComponent.box.addItemList(sa, 1);
		auto selectedText = getCurrentPropertyValue().toString();
		combinedComponent.box.setText(selectedText, dontSendNotification);
	}
}

void HiFilePropertyComponent::buttonClicked(Button *)
{
	FileChooser fc("Load File", GET_PROJECT_HANDLER(GET_BACKEND_ROOT_WINDOW(this)->getMainSynthChain()).getSubDirectory(ProjectHandler::SubDirectories::Images));

	if (fc.browseForFileToOpen())
	{
		String currentFile = GET_PROJECT_HANDLER(GET_BACKEND_ROOT_WINDOW(this)->getMainSynthChain()).getFileReference(fc.getResult().getFullPathName(), ProjectHandler::SubDirectories::Images);

		auto b = panel->getScriptComponentEditBroadcaster();

		b->setScriptComponentPropertyForSelection(getId(), currentFile, sendNotification);
	}
}

void HiFilePropertyComponent::comboBoxChanged(ComboBox *)
{
	auto b = panel->getScriptComponentEditBroadcaster();
	b->setScriptComponentPropertyForSelection(getId(), combinedComponent.box.getText(), sendNotification);
}

HiFilePropertyComponent::CombinedComponent::CombinedComponent() :
	box("FileNames"),
	button("Open")
{
	addAndMakeVisible(box);
	addAndMakeVisible(button);

	button.setConnectedEdges(Button::ConnectedOnLeft | Button::ConnectedOnRight);
	button.setColour(TextButton::buttonColourId, Colour(0x4c4b4b4b));
	button.setColour(TextButton::buttonOnColourId, Colours::white.withAlpha(0.7f));
	button.setColour(TextButton::textColourOnId, Colour(0xaa000000));
	button.setColour(TextButton::textColourOffId, Colour(0x99ffffff));
}


#if 0

 HiFilePropertyComponent::HiFilePropertyComponent(DynamicObject *properties_, Identifier id_, const StringArray &options, ScriptComponentEditPanel *panel_) :
	PropertyComponent(id_.toString()),
	 HiPropertyComponent(properties_, id_, panel_),
	component(this)
{
	addAndMakeVisible(component);

	

	pooledFiles.addArray(options, 1);

	component.box.addItemList(pooledFiles, 1);

}

void  HiFilePropertyComponent::refresh()
{
	currentFile = propertiesObject->getProperty(id);

	const int currentIndex = pooledFiles.indexOf(currentFile);

	component.box.setSelectedItemIndex(currentIndex, dontSendNotification);
}

void  HiFilePropertyComponent::buttonClicked(Button *)
{
	
}

void  HiFilePropertyComponent::comboBoxChanged(ComboBox *)
{
	const String fileName = component.box.getItemText(component.box.getSelectedItemIndex());

	currentFile = GET_PROJECT_HANDLER(GET_BACKEND_ROOT_WINDOW(this)->getMainSynthChain()).getFileReference(fileName, ProjectHandler::SubDirectories::Images);

	sendSynchronousChangeMessage();
}



 HiFilePropertyComponent::CombinedComponent::CombinedComponent(HiFilePropertyComponent *parent) :
	

#endif
	 
