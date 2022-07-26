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

namespace hise { using namespace juce;

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

HiPropertyComponent::HiPropertyComponent(const Identifier& id, ScriptComponentEditPanel* panel_) :
	PropertyComponent(id.toString()),
	propertyId(id),
	panel(panel_),
	overlay(this)
{
	setLookAndFeel(&plaf);

	if (!checkOverwrittenProperty())
	{
		addAndMakeVisible(overlay);
		overlay.setAlwaysOnTop(true);
	}
		
}

void HiPropertyComponent::resized()
{
	PropertyComponent::resized();

	if (auto c = getChildComponent(0))
	{
		if (overlay.isVisible())
			overlay.setBounds(c->getBoundsInParent());
	}
	else
	{
		if (overlay.isVisible())
			overlay.setBounds(getLocalBounds());
	}

	

	
}

var HiPropertyComponent::getCurrentPropertyValue(bool returnUndefinedWhenMultipleSelection/*=true*/) const
{
	

	auto b = panel->getScriptComponentEditBroadcaster();

	auto first = b->getFirstFromSelection();

	if (first == nullptr)
		return {};

	const var& firstValue = first->getScriptObjectProperty(getId());
	
	if (returnUndefinedWhenMultipleSelection && (b->getNumSelected() > 1))
	{
		ScriptComponentEditBroadcaster::Iterator iter(b);

		while (auto sc = iter.getNextScriptComponent())
		{
			auto nextValue = sc->getScriptObjectProperty(getId());
			if (nextValue != firstValue)
				return var::undefined();
		}
	}

	return firstValue;
}

bool HiPropertyComponent::checkOverwrittenProperty()
{
	ScriptComponentEditBroadcaster::Iterator iter(panel->getScriptComponentEditBroadcaster());

	while (auto sc = iter.getNextScriptComponent())
	{
		if (sc->isPropertyOverwrittenByScript(getId()))
			return false;
	}

	return true;
}

HiSliderPropertyComponent::HiSliderPropertyComponent(const Identifier& id, ScriptComponentEditPanel* panel) :
	HiPropertyComponent(id, panel),
	comp(this)
{
	addAndMakeVisible(comp);


	refresh();

}


HiSliderPropertyComponent::Comp::Comp(HiSliderPropertyComponent* parent)
{
	addAndMakeVisible(editor);
	addAndMakeVisible(slider);

	slider.setTextBoxStyle(Slider::TextEntryBoxPosition::NoTextBox, false, 80, 20);
	slider.setColour(Slider::backgroundColourId, Colour(0xfb282828));
	slider.setColour(Slider::thumbColourId, Colour(0xff777777));
	slider.setColour(Slider::trackColourId, Colour(0xff222222));
	slider.setColour(Slider::textBoxTextColourId, Colours::white);
	slider.setColour(Slider::textBoxOutlineColourId, Colour(0x45ffffff));
	slider.setScrollWheelEnabled(false);

	editor.addListener(parent);
	editor.setFont(GLOBAL_BOLD_FONT());
	editor.setColour(Label::ColourIds::backgroundColourId, Colours::white);
	editor.setColour(Label::ColourIds::backgroundWhenEditingColourId, Colours::white);
	editor.setColour(Label::ColourIds::outlineWhenEditingColourId, Colour(SIGNAL_COLOUR));
	editor.setColour(TextEditor::ColourIds::highlightColourId, Colour(SIGNAL_COLOUR));

	editor.setEditable(true);

	slider.addListener(parent);
}



void HiSliderPropertyComponent::sliderValueChanged(Slider *s)
{
	var newValue(s->getValue());

	updateInternal(newValue);
}


void HiSliderPropertyComponent::labelTextChanged(Label* l)
{
	auto d = l->getText().getDoubleValue();

	var newValue(d);

	updateInternal(newValue);

}

void HiSliderPropertyComponent::updateInternal(const var& newValue)
{
	panel->getScriptComponentEditBroadcaster()->setScriptComponentPropertyForSelection(getId(), newValue, sendNotification);
}

void HiSliderPropertyComponent::refresh()
{
	auto newVar = getCurrentPropertyValue();

	if (newVar.isUndefined())
	{
		comp.slider.setEnabled(false);

		comp.editor.setText("*", dontSendNotification);
	}
	else
	{
		comp.slider.setEnabled(true);

		updateRange();

		auto newValue = (double)newVar;

		if (comp.slider.getValue() != newValue)
		{
			comp.slider.setValue(newValue, dontSendNotification);
		}

		if (!comp.editor.isBeingEdited())
		{
			auto nAsInt = (int)newValue;

			if ((double)nAsInt == newValue)
			{
				comp.editor.setText(String(nAsInt), dontSendNotification);
			}
			else
			{
				comp.editor.setText(String(newValue, 2), dontSendNotification);
			}
		}
	}

	repaint();
}



void HiSliderPropertyComponent::updateRange()
{
	int oldRange = (int)comp.slider.getMaximum();

	static const Identifier x("x");
	static const Identifier y("y");
	static const Identifier w("width");
	static const Identifier h("height");

	static const Array<Identifier> posIds = { x, y, w, h };


	if (!posIds.contains(getId()))
	{
		SharedResourcePointer<hise::ScriptComponentPropertyTypeSelector> ptr;
		auto r = ptr->getRangeForId(getId());
		comp.slider.setRange(r.min, r.max, r.interval);
		return;
	}

	if (auto sc = panel->getScriptComponentEditBroadcaster()->getFirstFromSelection())
	{
		int maxWidth = sc->parent->getContentWidth();
		int maxHeight = sc->parent->getContentHeight();

		auto parent = sc->getParentScriptComponent();

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
			if(range > 0.0)
				comp.slider.setRange(0.0, (double)range, 1);

			comp.slider.repaint();
		}
	}
}


HiChoicePropertyComponent::HiChoicePropertyComponent(const Identifier& id, ScriptComponentEditPanel* panel):
	HiPropertyComponent(id, panel)
{
	addAndMakeVisible(comboBox);
	comboBox.setColour(HiseColourScheme::ComponentTextColourId, Colours::white);
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

		auto newVar = getCurrentPropertyValue();

		if (newVar.isUndefined())
		{
			comboBox.setText("*", dontSendNotification);
		}
		else
		{
			auto selectedText = getCurrentPropertyValue().toString();
			comboBox.setText(selectedText, dontSendNotification);
		}
	}

	repaint();
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
	auto newVar = getCurrentPropertyValue();

	if (newVar.isUndefined())
	{
		button.setButtonText("*");

		const bool on = (bool)getCurrentPropertyValue(false);

		button.setToggleState(on, dontSendNotification);
	}
	else
	{
		const bool on = (bool)newVar;

		button.setButtonText(on ? "Enabled" : "Disabled");

		button.setToggleState(on, dontSendNotification);
	}

	repaint();
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
	editor.setReturnKeyStartsNewLine(isMultiline);
	editor.addListener(this);

	if (isMultiline)
		setPreferredHeight(200);

	setLookAndFeel(&plaf);
}


void HiTextPropertyComponent::refresh()
{
	auto newVar = getCurrentPropertyValue();

	if (newVar.isUndefined())
	{
		editor.setText("*", dontSendNotification);
	}
	else
	{
		editor.setText(newVar.toString(), dontSendNotification);
	}

	repaint();
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

HiCodeEditorPropertyComponent::HiCodeEditorPropertyComponent(const Identifier& id, ScriptComponentEditPanel* panel) :
	HiPropertyComponent(id, panel)
{
	empty = new DynamicObject();

	addAndMakeVisible(editor = new JSONEditor(empty));

	setPreferredHeight(350);

	auto b = panel->getScriptComponentEditBroadcaster();

	JSONEditor::F5Callback cb = [b, id](const var& newValue)
	{
		b->setScriptComponentPropertyForSelection(id, JSON::toString(newValue, dontSendNotification, DOUBLE_TO_STRING_DIGITS), sendNotification);
	};

	editor->setCallback(cb);

	editor->setEditable(true);

	refresh();
}

void HiCodeEditorPropertyComponent::refresh()
{
	auto d = getCurrentPropertyValue();

	auto v = JSON::parse(d.toString());

	editor->setDataToEdit(v.isUndefined() ? empty : v);


}

HiColourPropertyComponent::HiColourPropertyComponent(const Identifier& id, ScriptComponentEditPanel* panel) :
	HiPropertyComponent(id, panel)
{
	addAndMakeVisible(comp);
	refresh();
}

void HiColourPropertyComponent::refresh()
{
	auto v = getCurrentPropertyValue();

	if (v.isUndefined())
	{
		v = getCurrentPropertyValue(false);

	}
	
	Colour c;

	if (v.isString())
	{
		c = Colour((uint32)v.toString().getLargeIntValue());
	}
	else if (v.isInt() || v.isInt64())
	{
		c = Colour((uint32)(int64)v);
	}

	comp.setDisplayedColour(c);

	repaint();
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
	if (auto sc = panel->getScriptComponentEditBroadcaster()->getFirstFromSelection())
	{
		auto newVar = getCurrentPropertyValue();

		combinedComponent.box.clear(dontSendNotification);
		auto sa = sc->getOptionsFor(getId());
		combinedComponent.box.addItemList(sa, 1);

		if (newVar.isUndefined())
			combinedComponent.box.setText("*", dontSendNotification);
		else
			combinedComponent.box.setText(newVar.toString(), dontSendNotification);
	}
	
	repaint();
}

void HiFilePropertyComponent::updateFile(const String& absoluteFilePath)
{
	String currentFile = GET_PROJECT_HANDLER(GET_BACKEND_ROOT_WINDOW(this)->getMainSynthChain()).getFileReference(absoluteFilePath, ProjectHandler::SubDirectories::Images);

	auto b = panel->getScriptComponentEditBroadcaster();

	b->setScriptComponentPropertyForSelection(getId(), currentFile, sendNotification);
}



void HiFilePropertyComponent::buttonClicked(Button *)
{
	FileChooser fc("Load File", GET_PROJECT_HANDLER(GET_BACKEND_ROOT_WINDOW(this)->getMainSynthChain()).getSubDirectory(ProjectHandler::SubDirectories::Images));

	if (fc.browseForFileToOpen())
	{
		auto path = fc.getResult().getFullPathName();
		updateFile(path);
	}
}

void HiFilePropertyComponent::comboBoxChanged(ComboBox *)
{
	auto path = combinedComponent.box.getText();
	updateFile(path);
}

HiFilePropertyComponent::CombinedComponent::CombinedComponent() :
	box("FileNames"),
	button("Open")
{
	addAndMakeVisible(box);
	addAndMakeVisible(button);

	box.setColour(HiseColourScheme::ComponentTextColourId, Colours::white);
	box.setColour(HiseColourScheme::ComponentFillTopColourId, Colours::transparentBlack);
	box.setColour(HiseColourScheme::ComponentFillBottomColourId, Colours::transparentBlack);
	box.setColour(HiseColourScheme::ComponentOutlineColourId, Colours::transparentBlack);


	button.setConnectedEdges(Button::ConnectedOnLeft | Button::ConnectedOnRight);
	button.setColour(TextButton::buttonColourId, Colour(0x4c4b4b4b));
	button.setColour(TextButton::buttonOnColourId, Colours::white.withAlpha(0.7f));
	button.setColour(TextButton::textColourOnId, Colour(0xaa000000));
	button.setColour(TextButton::textColourOffId, Colour(0x99ffffff));
}

HiColourPropertyComponent::ColourComp::ColourComp()
{
	addAndMakeVisible(l);

	l.setColour(Label::ColourIds::backgroundColourId, Colours::transparentBlack);
	l.setColour(Label::ColourIds::outlineColourId, Colours::transparentBlack);
	l.setColour(Label::ColourIds::outlineWhenEditingColourId, Colour(SIGNAL_COLOUR));
	l.addListener(this);
	l.setFont(GLOBAL_BOLD_FONT());
	l.setEditable(true);
}

void HiColourPropertyComponent::ColourComp::setDisplayedColour(Colour& c)
{
	colour = c;

	Colour textColour = Colours::white;

	l.setColour(Label::ColourIds::textColourId, Colours::white);
	l.setColour(Label::ColourIds::textWhenEditingColourId, Colours::white);
	l.setColour(TextEditor::ColourIds::highlightColourId, textColour.contrasting(0.5f));
	l.setColour(TextEditor::ColourIds::highlightedTextColourId, textColour);

	l.setText("#" + colour.toDisplayString(true), dontSendNotification);

	repaint();
}

void HiColourPropertyComponent::ColourComp::mouseDown(const MouseEvent& /*event*/)
{
	auto p = new Popup(this);

	auto root = findParentComponentOfClass<FloatingTile>()->getRootFloatingTile();

	CallOutBox::launchAsynchronously(std::unique_ptr<Component>(p), root->getLocalArea(this, getLocalBounds()), root);
}

void HiColourPropertyComponent::ColourComp::changeListenerCallback(ChangeBroadcaster* b)
{
	auto selector = dynamic_cast<ColourSelector*>(b);

	updateColour(selector->getCurrentColour());
}

void HiColourPropertyComponent::ColourComp::labelTextChanged(Label* /*labelThatHasChanged*/)
{
	const String t = l.getText().trimCharactersAtStart("#");

	auto c = Colour::fromString(t);

	updateColour(c);
}


void HiColourPropertyComponent::ColourComp::updateColour(Colour c)
{
	auto prop = findParentComponentOfClass<HiPropertyComponent>();

	auto b = findParentComponentOfClass<ScriptComponentEditListener>()->getScriptComponentEditBroadcaster();

	var newValue = var((int64)c.getARGB());

	b->setScriptComponentPropertyForSelection(prop->getId(), newValue, sendNotification);
}


void HiColourPropertyComponent::ColourComp::paint(Graphics& g)
{
	auto r = getLocalBounds();
	r.removeFromLeft(80);

	g.fillCheckerBoard(r.toFloat(), 10, 10, Colour(0xFF888888), Colour(0xFF444444));

	g.setColour(colour);
	g.fillRect(r);

	g.setColour(Colours::white.withAlpha(0.5f));
	g.drawRect(r, 1);

}

HiColourPropertyComponent::ColourComp::Popup::Popup(ColourComp* parent) :
	selector(ColourSelector::ColourSelectorOptions::showAlphaChannel |
		ColourSelector::ColourSelectorOptions::showColourspace |
		ColourSelector::ColourSelectorOptions::showSliders)
{
	setLookAndFeel(&laf);

	selector.setColour(ColourSelector::ColourIds::backgroundColourId, Colours::transparentBlack);
	selector.setColour(ColourSelector::ColourIds::labelTextColourId, Colours::white);
	selector.setColour(ColourSelector::ColourIds::labelTextColourId, Colours::white);

	juce::Component::callRecursive<Component>(&selector, [](Component* s)
	{
		s->setColour(Slider::ColourIds::textBoxTextColourId, Colours::white.withAlpha(0.8f));
		s->setColour(Slider::ColourIds::backgroundColourId, Colours::black.withAlpha(0.3f));
		s->setColour(Slider::ColourIds::thumbColourId, Colours::white.withAlpha(0.8f));
		s->setColour(Slider::ColourIds::trackColourId, Colours::white.withAlpha(0.5f));
		return false;
	});

	selector.setCurrentColour(parent->colour);
	

	addAndMakeVisible(selector);

	selector.addChangeListener(parent);

	setSize(300, 300);
}

void HiColourPropertyComponent::ColourComp::Popup::resized()
{
	selector.setBounds(getLocalBounds().reduced(10));
}

void HiPropertyComponent::Overlay::buttonClicked(Button* /*b*/)
{
	Identifier id = findParentComponentOfClass<HiPropertyComponent>()->getId();

	ScriptComponentEditBroadcaster* br = findParentComponentOfClass<ScriptComponentEditPanel>()->getScriptComponentEditBroadcaster();

	if (auto sc = br->getFirstFromSelection())
		ScriptingApi::Content::Helpers::recompileAndSearchForPropertyChange(sc, id);

	
}



} // namespace hise
