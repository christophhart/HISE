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
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

namespace hise { using namespace juce;

#undef GET_SCRIPT_PROPERTY
#undef GET_OBJECT_COLOUR

#define GET_SCRIPT_PROPERTY(id) (getScriptComponent()->getScriptObjectProperty(ScriptingApi::Content::ScriptComponent::Properties::id))




#define GET_OBJECT_COLOUR(id) (ScriptingApi::Content::Helpers::getCleanedObjectColour(GET_SCRIPT_PROPERTY(id)))


Array<Identifier> ScriptComponentPropertyTypeSelector::toggleProperties = Array<Identifier>();
Array<Identifier> ScriptComponentPropertyTypeSelector::sliderProperties = Array<Identifier>();
Array<Identifier> ScriptComponentPropertyTypeSelector::colourProperties = Array<Identifier>();
Array<Identifier> ScriptComponentPropertyTypeSelector::choiceProperties = Array<Identifier>();
Array<Identifier> ScriptComponentPropertyTypeSelector::multilineProperties = Array<Identifier>();
Array<Identifier> ScriptComponentPropertyTypeSelector::fileProperties = Array<Identifier>();
Array<Identifier> ScriptComponentPropertyTypeSelector::codeProperties = Array<Identifier>();
Array<ScriptComponentPropertyTypeSelector::SliderRange> ScriptComponentPropertyTypeSelector::sliderRanges = Array<ScriptComponentPropertyTypeSelector::SliderRange>();


ScriptCreatedComponentWrapper::~ScriptCreatedComponentWrapper()
{
	if (auto c = getComponent())
		c->setLookAndFeel(nullptr);

	currentPopup = nullptr;
}

void ScriptCreatedComponentWrapper::updateComponent(int propertyIndex, var newValue)
{
	if (propertyIndex >= ScriptingApi::Content::ScriptComponent::Properties::numProperties)
		return;

	auto propIndex = (ScriptingApi::Content::ScriptComponent::Properties)propertyIndex;

	switch (propIndex)
	{
	case hise::ScriptingApi::Content::ScriptComponent::visible: contentComponent->updateComponentVisibility(this); break;
	case hise::ScriptingApi::Content::ScriptComponent::enabled: contentComponent->updateComponentVisibility(this); break;
	case hise::ScriptingApi::Content::ScriptComponent::x:
	case hise::ScriptingApi::Content::ScriptComponent::y:
	case hise::ScriptingApi::Content::ScriptComponent::width:
	case hise::ScriptingApi::Content::ScriptComponent::height: contentComponent->updateComponentPosition(this); break;
	case hise::ScriptingApi::Content::ScriptComponent::parentComponent: contentComponent->updateComponentParent(this); break;
	default:
		break;
	}
}

void ScriptCreatedComponentWrapper::changed(var newValue)
{
	getScriptComponent()->value = newValue;

	dynamic_cast<ProcessorWithScriptingContent*>(getProcessor())->controlCallback(getScriptComponent(), newValue);
}

void ScriptCreatedComponentWrapper::asyncValueTreePropertyChanged(ValueTree& v, const Identifier& id)
{
	if (v != getScriptComponent()->getPropertyValueTree())
		return;

	jassert(v == getScriptComponent()->getPropertyValueTree());

	auto idIndex = getScriptComponent()->getIndexForProperty(id);
	auto value = v.getProperty(id);

	if (idIndex == -1)
	{
		debugError(getProcessor(), "invalid property " + id.toString() + " with value: '" + value.toString() + "'");
	}

	updateComponent(idIndex, value);
}

void ScriptCreatedComponentWrapper::valueTreeParentChanged(ValueTree& /*v*/)
{
	contentComponent->updateComponentParent(this);
}

ScriptCreatedComponentWrapper::ScriptCreatedComponentWrapper(ScriptContentComponent *content, int index_) :
	AsyncValueTreePropertyListener(content->contentData->getComponent(index_)->getPropertyValueTree(), content->contentData->getUpdateDispatcher()),
	valuePopupHandler(*this),
	contentComponent(content),
	index(index_)
{
	scriptComponent = content->contentData->getComponent(index_);
}

ScriptCreatedComponentWrapper::ScriptCreatedComponentWrapper(ScriptContentComponent* content, ScriptComponent* sc):
	AsyncValueTreePropertyListener(sc->getPropertyValueTree(), content->contentData->getUpdateDispatcher()),
	valuePopupHandler(*this),
	contentComponent(content),
	index(-1),
	scriptComponent(sc)
{
	
}

Processor * ScriptCreatedComponentWrapper::getProcessor()
{
	return contentComponent->p.get();
}

ScriptingApi::Content * ScriptCreatedComponentWrapper::getContent()
{
	return contentComponent->contentData;
}

void ScriptCreatedComponentWrapper::initAllProperties()
{
	auto sc = getScriptComponent();

	for (int i = 0; i < sc->getNumIds(); i++)
	{
		auto v = sc->getScriptObjectProperty(i);

		if (i == ScriptingApi::Content::ScriptComponent::Properties::parentComponent)
			continue;

		updateComponent(i, v);
	}
}



ScriptCreatedComponentWrappers::SliderWrapper::SliderWrapper(ScriptContentComponent *content, ScriptingApi::Content::ScriptSlider *sc, int index) :
ScriptCreatedComponentWrapper(content, index)
{
	HiSlider *s;

	s = new HiSlider(sc->name.toString());

	
	s->addListener(this);
	s->setValue(sc->value, dontSendNotification);

	s->setup(getProcessor(), getIndex(), sc->name.toString());

	component = s;

	initAllProperties();

	s->updateValue(dontSendNotification);
}

void ScriptCreatedComponentWrappers::SliderWrapper::updateFilmstrip()
{
	HiSlider *s = dynamic_cast<HiSlider*>(getComponent());



	ScriptingApi::Content::ScriptSlider* sc = dynamic_cast<ScriptingApi::Content::ScriptSlider*>(getScriptComponent());

	if (s == nullptr || sc == nullptr)
		return;

	if (sc->getImage().isValid())
	{
		String thisFilmStrip = sc->getScriptObjectProperty(ScriptingApi::Content::ScriptSlider::Properties::filmstripImage);

		int thisStrips = sc->getScriptObjectProperty(ScriptingApi::Content::ScriptSlider::numStrips);

		auto thisScaleFactor = (double)sc->getScriptObjectProperty(ScriptingApi::Content::ScriptSlider::scaleFactor);

		if (thisFilmStrip != filmStripName || thisStrips != numStrips || scaleFactor != thisScaleFactor)
		{
			filmStripName = thisFilmStrip;
			numStrips = thisStrips;
			scaleFactor = thisScaleFactor;

			FilmstripLookAndFeel *fslaf = new FilmstripLookAndFeel();

			fslaf->setFilmstripImage(sc->getImage(),
				sc->getScriptObjectProperty(ScriptingApi::Content::ScriptSlider::numStrips),
				sc->getScriptObjectProperty(ScriptingApi::Content::ScriptSlider::isVertical));

			fslaf->setScaleFactor((float)thisScaleFactor);

			s->setTextBoxStyle(Slider::NoTextBox, true, 0, 0);

			s->setLookAndFeelOwned(fslaf);

			s->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
		}
	}
}




void ScriptCreatedComponentWrappers::SliderWrapper::updateColours(HiSlider * s)
{
	s->setColour(Slider::backgroundColourId, GET_OBJECT_COLOUR(bgColour));
	s->setColour(Slider::thumbColourId, GET_OBJECT_COLOUR(itemColour));
	s->setColour(Slider::trackColourId, GET_OBJECT_COLOUR(itemColour2));

	s->setColour(HiseColourScheme::ComponentOutlineColourId, GET_OBJECT_COLOUR(bgColour));
	s->setColour(HiseColourScheme::ComponentFillTopColourId, GET_OBJECT_COLOUR(itemColour));
	s->setColour(HiseColourScheme::ComponentFillBottomColourId, GET_OBJECT_COLOUR(itemColour2));

	s->setColour(Slider::textBoxTextColourId, GET_OBJECT_COLOUR(textColour));
}




void ScriptCreatedComponentWrappers::SliderWrapper::updateSensitivity(ScriptingApi::Content::ScriptSlider* sc, HiSlider * s)
{
	double sensitivityScaler = sc->getScriptObjectProperty(ScriptingApi::Content::ScriptSlider::mouseSensitivity);

	if (sensitivityScaler != 1.0)
	{
		double sensitivity = jmax<double>(1.0, 250.0 * sensitivityScaler);
		s->setMouseDragSensitivity((int)sensitivity);
	}
}

void ScriptCreatedComponentWrappers::SliderWrapper::updateSliderRange(ScriptingApi::Content::ScriptSlider* sc, HiSlider * s)
{
    double min = sc->getScriptObjectProperty(ScriptingApi::Content::ScriptComponent::min);
    double max = sc->getScriptObjectProperty(ScriptingApi::Content::ScriptComponent::max);
	const double stepsize = sc->getScriptObjectProperty(ScriptingApi::Content::ScriptSlider::stepSize);
	const double middlePos = sc->getScriptObjectProperty(ScriptingApi::Content::ScriptSlider::middlePosition);

	Range<double> r(min, max);

	if (sc->m == HiSlider::Mode::TempoSync)
	{
        min = jmax(min, 0.0);
        max = jmin(max, (double)((int)TempoSyncer::Tempo::numTempos-1));
        
		s->setMode(HiSlider::Mode::TempoSync, min, max, min + (max-min)/2, 1);
		return;
	}

	const String suffix = sc->getScriptObjectProperty(ScriptingApi::Content::ScriptSlider::suffix);

	if (min >= max || stepsize <= 0.0 || min < -100000.0 || max > 100000.0)
	{
		s->setMode(HiSlider::Mode::Linear, 0.0, 1.0);
		s->setSkewFactor(1.0);
		s->setEnabled(false);
	}
	else
	{
		s->setSkewFactor(1.0);
		s->setMode(sc->m, min, max);
		s->setRange(min, max, stepsize);
		if (middlePos != min && r.contains(middlePos)) s->setSkewFactorFromMidPoint(middlePos);
		if (sc->m == HiSlider::Mode::Linear) s->setTextValueSuffix(suffix);
	}

	const double defaultValue = sc->getScriptObjectProperty(ScriptingApi::Content::ScriptSlider::defaultValue);

	if (defaultValue >= min && defaultValue <= max)
	{
		s->setDoubleClickReturnValue(true, defaultValue);
	}
}

void ScriptCreatedComponentWrappers::SliderWrapper::updateSliderStyle(ScriptingApi::Content::ScriptSlider* sc, HiSlider * s)
{
	if (sc->styleId == Slider::RotaryHorizontalVerticalDrag)
	{
		String direction = sc->getScriptObjectProperty(ScriptingApi::Content::ScriptSlider::dragDirection);

		if (direction == "Horizontal") s->setSliderStyle(Slider::RotaryHorizontalDrag);
		else if (direction == "Vertical") s->setSliderStyle(Slider::RotaryVerticalDrag);
		else s->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
	}
	else
	{
		s->setSliderStyle(sc->styleId);
	}

	if (sc->styleId == Slider::TwoValueHorizontal)
	{
		s->setTextBoxStyle(Slider::NoTextBox, false, 0, 0);
	}

	if (sc->styleId == Slider::LinearBar || sc->styleId == Slider::LinearBarVertical)
	{
		auto showTextBox = (bool)sc->getScriptObjectProperty(ScriptingApi::Content::ScriptSlider::showTextBox);

		if (!showTextBox)
			s->setColour(Slider::textBoxOutlineColourId, Colours::transparentBlack);

		s->setTextBoxStyle(showTextBox ? Slider::TextBoxAbove : Slider::NoTextBox, !showTextBox, s->getWidth(), s->getHeight());
	}

}

void ScriptCreatedComponentWrappers::SliderWrapper::updateComponent()
{
	jassertfalse;

	HiSlider *s = dynamic_cast<HiSlider*>(getComponent());

	jassert(s != nullptr);

	s->setUseUndoManagerForEvents(GET_SCRIPT_PROPERTY(useUndoManager));

	s->setTooltip(GET_SCRIPT_PROPERTY(tooltip));
	s->setName(GET_SCRIPT_PROPERTY(text));
	s->enableMacroControlledComponent(GET_SCRIPT_PROPERTY(enabled));

	ScriptingApi::Content::ScriptSlider* sc = dynamic_cast<ScriptingApi::Content::ScriptSlider*>(getScriptComponent());

	updateSensitivity(sc, s);


	updateFilmstrip();
	
	updateSliderRange(sc, s);


	

	s->setValue(sc->value, dontSendNotification);

	const bool usesFilmStrip = sc->getImage().isValid();

	if (!usesFilmStrip)
	{
		updateSliderStyle(sc, s);


		updateColours(s);

	}

	

	s->repaint();
}

#define PROPERTY_CASE case hise::ScriptingApi::Content

void ScriptCreatedComponentWrappers::SliderWrapper::updateComponent(int propertyIndex, var newValue)
{
	ScriptCreatedComponentWrapper::updateComponent(propertyIndex, newValue);
	
	HiSlider *s = dynamic_cast<HiSlider*>(getComponent());
	auto sc = dynamic_cast<ScriptingApi::Content::ScriptSlider*>(getScriptComponent());

	switch (propertyIndex)
	{
		PROPERTY_CASE::ScriptComponent::useUndoManager:	s->setUseUndoManagerForEvents(GET_SCRIPT_PROPERTY(useUndoManager)); break;
		PROPERTY_CASE::ScriptComponent::text:			s->setName(GET_SCRIPT_PROPERTY(text)); break;
		PROPERTY_CASE::ScriptComponent::enabled:		s->enableMacroControlledComponent(GET_SCRIPT_PROPERTY(enabled)); break;
		PROPERTY_CASE::ScriptComponent::tooltip :		s->setTooltip(GET_SCRIPT_PROPERTY(tooltip)); break;
		PROPERTY_CASE::ScriptComponent::saveInPreset:   s->setCanBeMidiLearned(newValue); break;
		PROPERTY_CASE::ScriptComponent::bgColour:
		PROPERTY_CASE::ScriptComponent::itemColour :
		PROPERTY_CASE::ScriptComponent::itemColour2 :
		PROPERTY_CASE::ScriptComponent::textColour :	updateColours(s); break;
		PROPERTY_CASE::ScriptSlider::dragDirection:
		PROPERTY_CASE::ScriptSlider::showTextBox:
		PROPERTY_CASE::ScriptSlider::Style:				updateSliderStyle(sc, s); break;
		PROPERTY_CASE::ScriptSlider::Mode:
		PROPERTY_CASE::ScriptComponent::min :
		PROPERTY_CASE::ScriptComponent::max :
		PROPERTY_CASE::ScriptSlider::stepSize :
		PROPERTY_CASE::ScriptSlider::middlePosition :
		PROPERTY_CASE::ScriptSlider::defaultValue :
		PROPERTY_CASE::ScriptSlider::suffix :			updateSliderRange(sc, s); break;
		PROPERTY_CASE::ScriptSlider::filmstripImage:
		PROPERTY_CASE::ScriptSlider::numStrips:
		PROPERTY_CASE::ScriptSlider::isVertical :
		PROPERTY_CASE::ScriptSlider::scaleFactor :		updateFilmstrip(); break;
		PROPERTY_CASE::ScriptSlider::mouseSensitivity:		updateSensitivity(sc, s); break;
		PROPERTY_CASE::ScriptSlider::showValuePopup:
		PROPERTY_CASE::ScriptSlider::numProperties:
	default:
		break;
	}
}

void ScriptCreatedComponentWrappers::SliderWrapper::updateValue(var newValue)
{
	if (auto s = dynamic_cast<HiSlider*>(getComponent()))
	{
		s->updateValue(dontSendNotification);
		updateTooltip(s);
	}
	
}

void ScriptCreatedComponentWrappers::SliderWrapper::sliderValueChanged(Slider *s)
{
	if (s->getSliderStyle() == Slider::TwoValueHorizontal)
	{
		if (s->getThumbBeingDragged() == 1)
		{
			dynamic_cast<ScriptingApi::Content::ScriptSlider*>(getScriptComponent())->setMinValue(s->getMinValue());
			changed(s->getMinValue());
		}
		else
		{
			dynamic_cast<ScriptingApi::Content::ScriptSlider*>(getScriptComponent())->setMaxValue(s->getMaxValue());
			changed(s->getMaxValue());
		}
	}
	else
	{
		/*changed(s->getValue());*/ // setInternalAttribute handles this for .
	}
}

void ScriptCreatedComponentWrappers::SliderWrapper::updateTooltip(Slider * s)
{
	auto tooltip = GET_SCRIPT_PROPERTY(tooltip).toString();

	static const String valueWildCard("{VALUE}");

	if (tooltip.isNotEmpty() && tooltip.contains(valueWildCard))
	{
		tooltip = tooltip.replace(valueWildCard, s->getTextFromValue(s->getValue()));
		s->setTooltip(tooltip);
	}
}

void ScriptCreatedComponentWrapper::showValuePopup()
{
	auto c = getComponent();

	auto parentTile = c->findParentComponentOfClass<FloatingTile>(); // always in a tile...

	if (parentTile == nullptr)
	{
		// Ouch...
		jassertfalse;
		return;
	}

	parentTile->addAndMakeVisible(currentPopup = new ValuePopup(*this));

	

	currentPopup->setFont(parentTile->getMainController()->getFontFromString("Default", 14.0f));

	

	currentPopup->setAlwaysOnTop(true);
	
	updatePopupPosition();
}

void ScriptCreatedComponentWrapper::updatePopupPosition()
{
	if (currentPopup != nullptr)
	{
		auto c = getComponent();

		auto parentTile = c->findParentComponentOfClass<FloatingTile>(); // always in a tile...

		if (parentTile == nullptr)
		{
			// Ouch...
			jassertfalse;
			return;
		}

		auto l = parentTile->getLocalArea(c, c->getLocalBounds());
		currentPopup->setTopLeftPosition(getValuePopupPosition(l));
	}
}

void ScriptCreatedComponentWrapper::closeValuePopupAfterDelay()
{
	valuePopupHandler.startTimer(200);
}

void ScriptCreatedComponentWrappers::SliderWrapper::sliderDragStarted(Slider* s)
{
	auto direction = getScriptComponent()->getScriptObjectProperty(ScriptingApi::Content::ScriptSlider::Properties::showValuePopup).toString();

	if (direction == "No")
		return;

	if (auto c = getComponent())
	{
		showValuePopup();

		if (s->getSliderStyle() == Slider::LinearBar ||
			s->getSliderStyle() == Slider::LinearBarVertical)
		{
			currentPopup->itemColour = Colour(0xFF222222);
			currentPopup->itemColour2 = Colour(0xFF111111);
			currentPopup->textColour = Colour(0xFFCCCCCC);
			currentPopup->bgColour = Colour(0xFFCCCCCC);
		}
		else
		{
			currentPopup->itemColour = GET_OBJECT_COLOUR(itemColour);
			currentPopup->itemColour2 = GET_OBJECT_COLOUR(itemColour2);
			currentPopup->textColour = GET_OBJECT_COLOUR(textColour);
			currentPopup->bgColour = GET_OBJECT_COLOUR(bgColour);
		}
	}
}

void ScriptCreatedComponentWrappers::SliderWrapper::sliderDragEnded(Slider* /*s*/)
{
	closeValuePopupAfterDelay();
}

juce::Point<int> ScriptCreatedComponentWrappers::SliderWrapper::getValuePopupPosition(Rectangle<int> l) const
{
	enum Direction
	{
		No,
		Above,
		Below,
		Left,
		Right,
		numDirections
	};

	auto s = dynamic_cast<const Slider*>(getComponent());

	Direction d = numDirections;

	if (auto sc = getScriptComponent())
	{
		auto svp = sc->getScriptObjectProperty(ScriptingApi::Content::ScriptSlider::Properties::showValuePopup).toString();

		if (svp == "No") d = No;
		if (svp == "Above") d = Above;
		if (svp == "Below") d = Below;
		if (svp == "Left") d = Left;
		if (svp == "Right") d = Right;

		jassert(d != numDirections);
	}

	jassert(d != No);
	
	int x = 0;
	int y = 0;

	switch (d)
	{
	case Above:
	{
		int xP = l.getCentreX();
		int wP = currentPopup->getWidth();
		x = xP - wP / 2;

		y = l.getY() - 25;
		break;
	}
	case numDirections:
	case Below:
	{
		int xP = l.getCentreX();
		int wP = currentPopup->getWidth();
		x = xP - wP / 2;

		y = l.getBottom();

		if (s != nullptr &&
			(s->getSliderStyle() == Slider::LinearBar ||
			 s->getSliderStyle() == Slider::LinearBarVertical))
		{
			y += 10;
		}
			

		break;
	}

	case Left:
	{
		x = l.getX() - currentPopup->getWidth() - 10;
		y = l.getCentreY() - currentPopup->getHeight() / 2;
		break;
	}
	case Right:
	{
		x = l.getRight() + 10;
		y = l.getCentreY() - currentPopup->getHeight() / 2;
		break;
	}
	default:
		break;
	}

	return { x, y };
}

juce::String ScriptCreatedComponentWrappers::SliderWrapper::getTextForValuePopup()
{
	if (auto slider = dynamic_cast<Slider*>(getComponent()))
	{
		return slider->getTextFromValue(slider->getValue());;
	}
	
	return "";
}

ScriptCreatedComponentWrappers::ComboBoxWrapper::ComboBoxWrapper(ScriptContentComponent *content, ScriptingApi::Content::ScriptComboBox *scriptComboBox, int index) :
ScriptCreatedComponentWrapper(content, index)
{
	HiComboBox *cb = new HiComboBox(scriptComboBox->name.toString());

	cb->setup(getProcessor(), getIndex(), scriptComboBox->name.toString());
	cb->addListener(this);
	//cb->setLookAndFeel(&plaf);

	component = cb;

	initAllProperties();

	cb->updateValue(dontSendNotification);
}

void ScriptCreatedComponentWrappers::ComboBoxWrapper::updateComponent()
{

	

}

void ScriptCreatedComponentWrappers::ComboBoxWrapper::updateComponent(int propertyIndex, var newValue)
{
	ScriptCreatedComponentWrapper::updateComponent(propertyIndex, newValue);
	
	HiComboBox *cb = dynamic_cast<HiComboBox*>(component.get());
	
	switch (propertyIndex)
	{
		PROPERTY_CASE::ScriptComponent::tooltip :		cb->setTooltip(newValue); break;
		PROPERTY_CASE::ScriptComponent::useUndoManager: cb->setUseUndoManagerForEvents(newValue); break;
		PROPERTY_CASE::ScriptComponent::enabled:		cb->enableMacroControlledComponent(newValue); break;
		PROPERTY_CASE::ScriptComponent::text:			cb->setTextWhenNothingSelected(newValue); break;
		PROPERTY_CASE::ScriptComponent::saveInPreset:   cb->setCanBeMidiLearned(newValue); break;
		PROPERTY_CASE::ScriptComponent::bgColour:
		PROPERTY_CASE::ScriptComponent::itemColour :
			PROPERTY_CASE::ScriptComponent::itemColour2 :
			PROPERTY_CASE::ScriptComponent::textColour : updateColours(cb); break;
		PROPERTY_CASE::ScriptComboBox::Items:			 updateItems(cb); break;
		PROPERTY_CASE::ScriptComboBox::FontName:
		PROPERTY_CASE::ScriptComboBox::FontSize :
		PROPERTY_CASE::ScriptComboBox::FontStyle : updateFont(getScriptComponent()); break;
		PROPERTY_CASE::ScriptSlider::numProperties :
	default:
		break;
	}
}

void ScriptCreatedComponentWrappers::ComboBoxWrapper::updateFont(ScriptComponent* cb)
{
	const String fontName = cb->getScriptObjectProperty(ScriptingApi::Content::ScriptComboBox::FontName).toString();
	const String fontStyle = cb->getScriptObjectProperty(ScriptingApi::Content::ScriptComboBox::FontStyle).toString();
	const float fontSize = (float)cb->getScriptObjectProperty(ScriptingApi::Content::ScriptComboBox::FontSize);

	if (fontName == "Oxygen" || fontName == "Default")
	{
		if (fontStyle == "Bold")
			plaf.setComboBoxFont(GLOBAL_BOLD_FONT().withHeight(fontSize));
		else
		{
			plaf.setComboBoxFont(GLOBAL_FONT().withHeight(fontSize));
		}
	}
	else if (fontName == "Source Code Pro")
	{
		plaf.setComboBoxFont(GLOBAL_MONOSPACE_FONT().withHeight(fontSize));
	}
	else
	{
		const juce::Typeface::Ptr typeface = dynamic_cast<const Processor*>(contentComponent->getScriptProcessor())->getMainController()->getFont(fontName);

		if (typeface != nullptr)
		{
			Font font = Font(typeface).withHeight(fontSize);
			plaf.setComboBoxFont(font);
		}
		else
		{
			Font font(fontName, fontStyle, fontSize);
			plaf.setComboBoxFont(font);
		}
	}

	getComponent()->resized();
	getComponent()->repaint();
}

void ScriptCreatedComponentWrappers::ComboBoxWrapper::updateItems(HiComboBox * cb)
{
	cb->clear(dontSendNotification);

	cb->addItemList(dynamic_cast<ScriptingApi::Content::ScriptComboBox*>(getScriptComponent())->getItemList(), 1);
}

void ScriptCreatedComponentWrappers::ComboBoxWrapper::updateColours(HiComboBox * cb)
{
	cb->setColour(HiseColourScheme::ComponentOutlineColourId, GET_OBJECT_COLOUR(bgColour));
	cb->setColour(HiseColourScheme::ComponentFillTopColourId, GET_OBJECT_COLOUR(itemColour));
	cb->setColour(HiseColourScheme::ComponentFillBottomColourId, GET_OBJECT_COLOUR(itemColour2));
	cb->setColour(HiseColourScheme::ComponentTextColourId, GET_OBJECT_COLOUR(textColour));
}

void ScriptCreatedComponentWrappers::ComboBoxWrapper::updateValue(var newValue)
{
	HiComboBox *cb = dynamic_cast<HiComboBox*>(component.get());
	cb->updateValue(dontSendNotification);
}

ScriptCreatedComponentWrappers::ButtonWrapper::ButtonWrapper(ScriptContentComponent *content, ScriptingApi::Content::ScriptButton *sb, int index) :
ScriptCreatedComponentWrapper(content, index)
{
	HiToggleButton *b = new HiToggleButton(sb->name.toString());
	
	b->addListener(this);
	
	b->setup(getProcessor(), getIndex(), sb->name.toString());

	if (sb->getPopupData().isObject())
	{
        auto r = sb->getPopupPosition();
		b->setPopupData(sb->getPopupData(), r);
	}

	b->updateValue(dontSendNotification);


	component = b;

	initAllProperties();

}


ScriptCreatedComponentWrappers::ButtonWrapper::~ButtonWrapper()
{
	
}

void ScriptCreatedComponentWrappers::ButtonWrapper::updateFilmstrip(HiToggleButton* b, ScriptingApi::Content::ScriptButton* sb)
{
	if (sb->getImage().isValid())
	{
		b->setLookAndFeel(nullptr);
		FilmstripLookAndFeel *fslaf = new FilmstripLookAndFeel();

		fslaf->setFilmstripImage(sb->getImage(),
			sb->getScriptObjectProperty(ScriptingApi::Content::ScriptButton::numStrips).toString().getIntValue(),
			sb->getScriptObjectProperty(ScriptingApi::Content::ScriptButton::isVertical));

		fslaf->setScaleFactor(sb->getScriptObjectProperty(ScriptingApi::Content::ScriptButton::scaleFactor));

		b->setLookAndFeelOwned(fslaf);
	}
}

void ScriptCreatedComponentWrappers::ButtonWrapper::updateComponent()
{
	jassertfalse;

	HiToggleButton *b = dynamic_cast<HiToggleButton*>(component.get());

	b->enableMacroControlledComponent(GET_SCRIPT_PROPERTY(enabled));

	b->setUseUndoManagerForEvents(GET_SCRIPT_PROPERTY(useUndoManager));
    
    FilmstripLookAndFeel *fslaf = dynamic_cast<FilmstripLookAndFeel*>(&b->getLookAndFeel());
    
    if(fslaf != nullptr)
    {
        fslaf->setScaleFactor(getScriptComponent()->getScriptObjectProperty(ScriptingApi::Content::ScriptButton::scaleFactor));
    }

	b->setIsMomentary(getScriptComponent()->getScriptObjectProperty(ScriptingApi::Content::ScriptButton::isMomentary));
	updateColours(b);

	b->setButtonText(GET_SCRIPT_PROPERTY(text));
	b->setToggleState((bool)getScriptComponent()->value, dontSendNotification);
	b->setRadioGroupId(getScriptComponent()->getScriptObjectProperty(ScriptingApi::Content::ScriptButton::radioGroup));
}


void ScriptCreatedComponentWrappers::ButtonWrapper::updateComponent(int propertyIndex, var newValue)
{
	ScriptCreatedComponentWrapper::updateComponent(propertyIndex, newValue);
	
	HiToggleButton *b = dynamic_cast<HiToggleButton*>(getComponent());
	auto sc = dynamic_cast<ScriptingApi::Content::ScriptButton*>(getScriptComponent());

	switch (propertyIndex)
	{
		PROPERTY_CASE::ScriptComponent::saveInPreset:   b->setCanBeMidiLearned(newValue); break;
		PROPERTY_CASE::ScriptComponent::useUndoManager:	b->setUseUndoManagerForEvents(GET_SCRIPT_PROPERTY(useUndoManager)); break;
		PROPERTY_CASE::ScriptComponent::text:			b->setButtonText(GET_SCRIPT_PROPERTY(text)); break;
		PROPERTY_CASE::ScriptComponent::enabled:		b->enableMacroControlledComponent(GET_SCRIPT_PROPERTY(enabled)); break;
		PROPERTY_CASE::ScriptComponent::tooltip :		b->setTooltip(GET_SCRIPT_PROPERTY(tooltip)); break;
		PROPERTY_CASE::ScriptComponent::bgColour:
		PROPERTY_CASE::ScriptComponent::itemColour :
		PROPERTY_CASE::ScriptComponent::itemColour2 :
		PROPERTY_CASE::ScriptComponent::textColour :	updateColours(b); break;
		PROPERTY_CASE::ScriptButton::filmstripImage:
		PROPERTY_CASE::ScriptButton::numStrips :
		PROPERTY_CASE::ScriptButton::scaleFactor :		updateFilmstrip(b, sc); break;
		PROPERTY_CASE::ScriptButton::radioGroup:		b->setRadioGroupId(getScriptComponent()->getScriptObjectProperty(ScriptingApi::Content::ScriptButton::radioGroup)); break;
		PROPERTY_CASE::ScriptButton::isMomentary :		b->setIsMomentary(getScriptComponent()->getScriptObjectProperty(ScriptingApi::Content::ScriptButton::isMomentary)); break;
			PROPERTY_CASE::ScriptSlider::numProperties :
	default:
		break;
	}
}

void ScriptCreatedComponentWrappers::ButtonWrapper::updateColours(HiToggleButton * b)
{
	b->setColour(HiseColourScheme::ComponentTextColourId, GET_OBJECT_COLOUR(textColour));
	b->setColour(HiseColourScheme::ComponentOutlineColourId, GET_OBJECT_COLOUR(bgColour));
	b->setColour(HiseColourScheme::ComponentFillTopColourId, GET_OBJECT_COLOUR(itemColour));
	b->setColour(HiseColourScheme::ComponentFillBottomColourId, GET_OBJECT_COLOUR(itemColour2));
}

void ScriptCreatedComponentWrappers::ButtonWrapper::updateValue(var newValue)
{
	HiToggleButton *b = dynamic_cast<HiToggleButton*>(getComponent());
	b->updateValue(dontSendNotification);
}

ScriptCreatedComponentWrappers::LabelWrapper::LabelWrapper(ScriptContentComponent *content, ScriptingApi::Content::ScriptLabel *sl, int index):
ScriptCreatedComponentWrapper(content, index)
{
	auto l = new MultilineLabel(sl->name.toString());
	
	
	component = l;

	l->addListener(this);

	initAllProperties();

	updateValue(sl->getValue());
}

void ScriptCreatedComponentWrappers::LabelWrapper::updateComponent()
{
	jassertfalse;

	MultilineLabel *l = dynamic_cast<MultilineLabel*>(component.get());
	
	ScriptingApi::Content::ScriptLabel *sl = dynamic_cast<ScriptingApi::Content::ScriptLabel*>(getScriptComponent());

	l->setTooltip(GET_SCRIPT_PROPERTY(tooltip));

	updateFont(sl, l);


	
	updateColours(l);

	
	updateEditability(sl, l);

}

void ScriptCreatedComponentWrappers::LabelWrapper::updateComponent(int propertyIndex, var newValue)
{
	if (propertyIndex < ScriptingApi::Content::ScriptComponent::Properties::numProperties)
	{
		ScriptCreatedComponentWrapper::updateComponent(propertyIndex, newValue);
	}

	MultilineLabel *l = dynamic_cast<MultilineLabel*>(component.get());
	auto sc = dynamic_cast<ScriptingApi::Content::ScriptLabel*>(getScriptComponent());

	switch (propertyIndex)
	{
		PROPERTY_CASE::ScriptComponent::tooltip :		l->setTooltip(GET_SCRIPT_PROPERTY(tooltip)); break;
		PROPERTY_CASE::ScriptComponent::bgColour:
		PROPERTY_CASE::ScriptComponent::itemColour :
		PROPERTY_CASE::ScriptComponent::itemColour2 :
		PROPERTY_CASE::ScriptComponent::textColour : updateColours(l); break;
		PROPERTY_CASE::ScriptLabel::FontName:
		PROPERTY_CASE::ScriptLabel::FontSize :
		PROPERTY_CASE::ScriptLabel::FontStyle :
		PROPERTY_CASE::ScriptLabel::Alignment :		updateFont(sc, l); break;
		PROPERTY_CASE::ScriptLabel::Editable:		 updateEditability(sc, l); break;
		PROPERTY_CASE::ScriptLabel::Multiline:		l->setMultiline(newValue); break;
		PROPERTY_CASE::ScriptSlider::numProperties :
	default:
		break;
	}
}

void ScriptCreatedComponentWrappers::LabelWrapper::updateEditability(ScriptingApi::Content::ScriptLabel * sl, MultilineLabel * l)
{
	bool editable = sl->getScriptObjectProperty(ScriptingApi::Content::ScriptLabel::Editable);
	bool multiline = sl->getScriptObjectProperty(ScriptingApi::Content::ScriptLabel::Multiline);

	l->setText(getScriptComponent()->getValue().toString(), dontSendNotification);

	l->setInterceptsMouseClicks(editable, editable);
	l->setEditable(editable);
	l->setMultiline(multiline);
}

void ScriptCreatedComponentWrappers::LabelWrapper::updateFont(ScriptingApi::Content::ScriptLabel * sl, MultilineLabel * l)
{
	const String fontName = sl->getScriptObjectProperty(ScriptingApi::Content::ScriptLabel::FontName).toString();
	const String fontStyle = sl->getScriptObjectProperty(ScriptingApi::Content::ScriptLabel::FontStyle).toString();
	const float fontSize = (float)sl->getScriptObjectProperty(ScriptingApi::Content::ScriptLabel::FontSize);

	if (fontName == "Oxygen" || fontName == "Default")
	{
		if (fontStyle == "Bold")
		{
			l->setFont(GLOBAL_BOLD_FONT().withHeight(fontSize));
		}
		else
		{
			l->setFont(GLOBAL_FONT().withHeight(fontSize));
		}
	}
	else if (fontName == "Source Code Pro")
	{
		l->setFont(GLOBAL_MONOSPACE_FONT().withHeight(fontSize));
	}
	else
	{
		const juce::Typeface::Ptr typeface = dynamic_cast<const Processor*>(contentComponent->getScriptProcessor())->getMainController()->getFont(fontName);

		if (typeface != nullptr)
		{
			Font font = Font(typeface).withHeight(fontSize);
			l->setFont(font);
		}
		else
		{
			Font font(fontName, fontStyle, fontSize);
			l->setFont(font);
		}
	}

	l->setUsePasswordCharacter(fontStyle == "Password");

	l->setJustificationType(sl->getJustification());
}

void ScriptCreatedComponentWrappers::LabelWrapper::updateColours(MultilineLabel * l)
{
	l->setColour(Label::ColourIds::textColourId, GET_OBJECT_COLOUR(textColour));
	l->setColour(Label::ColourIds::backgroundColourId, GET_OBJECT_COLOUR(bgColour));
	l->setColour(Label::ColourIds::backgroundWhenEditingColourId, GET_OBJECT_COLOUR(bgColour));
	l->setColour(Label::ColourIds::textWhenEditingColourId, GET_OBJECT_COLOUR(textColour));
	l->setColour(TextEditor::ColourIds::highlightColourId, GET_OBJECT_COLOUR(textColour));
	l->setColour(TextEditor::ColourIds::highlightedTextColourId, GET_OBJECT_COLOUR(textColour).contrasting());
	l->setColour(TextEditor::ColourIds::focusedOutlineColourId, GET_OBJECT_COLOUR(itemColour));
	l->setColour(CaretComponent::ColourIds::caretColourId, GET_OBJECT_COLOUR(textColour));
	l->setColour(Label::ColourIds::outlineColourId, GET_OBJECT_COLOUR(itemColour));
}

void ScriptCreatedComponentWrappers::LabelWrapper::labelTextChanged(Label *l)
{
	auto sc = getScriptComponent();

	sc->setValue(l->getText());

	dynamic_cast<ProcessorWithScriptingContent*>(getProcessor())->controlCallback(getScriptComponent(), sc->getValue());
}

void ScriptCreatedComponentWrappers::LabelWrapper::updateValue(var newValue)
{
	MultilineLabel *l = dynamic_cast<MultilineLabel*>(component.get());

	l->setText(newValue.toString(), dontSendNotification);
}

ScriptCreatedComponentWrappers::TableWrapper::TableWrapper(ScriptContentComponent *content, ScriptingApi::Content::ScriptTable *table, int index) :
ScriptCreatedComponentWrapper(content, index)
{
	auto mc = getContent()->getScriptProcessor()->getMainController_();

	TableEditor *t = new TableEditor(mc->getControlUndoManager(), table->getTable());

	t->setName(table->name.toString());
	t->popupFunction = BIND_MEMBER_FUNCTION_2(TableWrapper::getTextForTablePopup);

	auto slaf = &mc->getGlobalLookAndFeel();

	if (auto s = dynamic_cast<TableEditor::LookAndFeelMethods*>(slaf))
	{
		t->setTableLookAndFeel(s, true);
	}

	table->broadcaster.addChangeListener(t);

	component = t;
	
	t->addListener(this);

	updateConnectedTable(t);

	initAllProperties();
}

ScriptCreatedComponentWrappers::TableWrapper::~TableWrapper()
{
	if (auto table = dynamic_cast<ScriptingApi::Content::ScriptTable*>(getScriptComponent()))
	{
		if (auto te = dynamic_cast<TableEditor*>(component.get()))
		{
			table->broadcaster.removeChangeListener(te);
			te->removeListener(this);
		}
	}
}

void ScriptCreatedComponentWrappers::TableWrapper::updateComponent()
{
	jassertfalse;

	TableEditor *t = dynamic_cast<TableEditor*>(component.get());

	updateConnectedTable(t);

}

void ScriptCreatedComponentWrappers::TableWrapper::updateComponent(int propertyIndex, var newValue)
{
	ScriptCreatedComponentWrapper::updateComponent(propertyIndex, newValue);
	
	ScriptingApi::Content::ScriptTable *st = dynamic_cast<ScriptingApi::Content::ScriptTable*>(getScriptComponent());
	TableEditor *t = dynamic_cast<TableEditor*>(component.get());
	
	switch (propertyIndex)
	{
		PROPERTY_CASE::ScriptComponent::bgColour: t->setColour(TableEditor::ColourIds::bgColour, GET_OBJECT_COLOUR(bgColour)); t->repaint(); break;
		PROPERTY_CASE::ScriptComponent::itemColour: t->setColour(TableEditor::ColourIds::fillColour, GET_OBJECT_COLOUR(itemColour)); t->repaint(); break;
		PROPERTY_CASE::ScriptComponent::itemColour2: t->setColour(TableEditor::ColourIds::lineColour, GET_OBJECT_COLOUR(itemColour2)); t->repaint(); break;
		PROPERTY_CASE::ScriptComponent::tooltip: t->setTooltip(GET_SCRIPT_PROPERTY(tooltip)); break;
		PROPERTY_CASE::ScriptComponent::processorId:
		PROPERTY_CASE::ScriptTable::Properties::TableIndex : updateConnectedTable(t); break;
		PROPERTY_CASE::ScriptTable::Properties::customColours: t->setUseFlatDesign(newValue); break;
		PROPERTY_CASE::ScriptComponent::parameterId: t->setSnapValues(st->snapValues);
													  break;
	default:
		break;
	}
}

void ScriptCreatedComponentWrappers::TableWrapper::updateConnectedTable(TableEditor * t)
{
	ScriptingApi::Content::ScriptTable *st = dynamic_cast<ScriptingApi::Content::ScriptTable*>(getScriptComponent());
	LookupTableProcessor *ltp = st->getTableProcessor();
	t->connectToLookupTableProcessor(dynamic_cast<Processor*>(ltp));

	t->setSnapValues(st->snapValues);

	Table *oldTable = t->getEditedTable();
	Table *newTable = st->getTable();

	if (oldTable != newTable) t->setEditedTable(newTable);
}

juce::String ScriptCreatedComponentWrappers::TableWrapper::getTextForTablePopup(float x, float y)
{
	TableEditor *t = dynamic_cast<TableEditor*>(component.get());
	ScriptingApi::Content::ScriptTable *st = dynamic_cast<ScriptingApi::Content::ScriptTable*>(getScriptComponent());

	if (HiseJavascriptEngine::isJavascriptFunction(st->tableValueFunction))
	{
		if (auto jp = dynamic_cast<JavascriptProcessor*>(st->getScriptProcessor()))
		{
			var data[2] = { var(x), var(y) };
			var::NativeFunctionArgs args(st, data, 2);
			Result r = Result::ok();

			auto text = jp->getScriptEngine()->callExternalFunction(st->tableValueFunction, args, &r, true);

			if (r.wasOk())
			{
				return text;
			}
		}
	}

	return t->getPopupString(x, y);
}

void ScriptCreatedComponentWrappers::TableWrapper::pointDragStarted(juce::Point<int> position, float pointIndex, float value)
{
	localPopupPosition = position.withY(position.getY() - 20);;

	popupText = getTextForTablePopup(pointIndex, value);
	//popupText = String(pointIndex) + " | " + String(roundToInt(value*100.0f)) + "%";

	if (auto st = dynamic_cast<ScriptingApi::Content::ScriptTable*>(getScriptComponent()))
	{
		//auto xName = st->getTable()->getXValueText(pointIndex);
		//auto yName = st->getTable()->getYValueText(value);

		//popupText = xName + " | " + yName;

		showValuePopup();
	}
}

void ScriptCreatedComponentWrappers::TableWrapper::pointDragEnded()
{
	closeValuePopupAfterDelay();
}

void ScriptCreatedComponentWrappers::TableWrapper::pointDragged(juce::Point<int> position, float pointIndex, float value)
{
	if (auto st = dynamic_cast<ScriptingApi::Content::ScriptTable*>(getScriptComponent()))
	{
		popupText = getTextForTablePopup(pointIndex, value);
		showValuePopup();
	}

	localPopupPosition = position.withY(position.getY() - 20);;
	updatePopupPosition();
}

void ScriptCreatedComponentWrappers::TableWrapper::curveChanged(juce::Point<int> position, float curveValue)
{
	localPopupPosition = position;

	popupText = String(roundToInt(curveValue * 100.0f)) + "%";

	showValuePopup();
	closeValuePopupAfterDelay();
}

juce::Point<int> ScriptCreatedComponentWrappers::TableWrapper::getValuePopupPosition(Rectangle<int> l) const
{
	return { l.getX() + localPopupPosition.getX(), l.getY() + localPopupPosition.getY() };
}



class DummyComponent: public Component
{
public:

	DummyComponent()
	{
		setSize(4000, 4000);
	}

	void paint(Graphics& g)
	{
		g.setGradientFill(ColourGradient(Colours::white.withAlpha(0.2f), 0.0f, 0.0f, Colours::black.withAlpha(0.2f), (float)getWidth(), (float)getHeight(), false));

		g.fillAll();
		g.setColour(Colours::white.withAlpha(0.6f));
		g.drawRect(getLocalBounds(), 1);
		
	}
};

ScriptCreatedComponentWrappers::ViewportWrapper::ViewportWrapper(ScriptContentComponent* content, ScriptingApi::Content::ScriptedViewport* viewport, int index):
	ScriptCreatedComponentWrapper(content, index)
{
	

	shouldUseList = (bool)viewport->getScriptObjectProperty(ScriptingApi::Content::ScriptedViewport::Properties::useList);

	if (!shouldUseList)
	{
		Viewport* vp = new Viewport();

		vp->setName(viewport->name.toString());

		vp->setViewedComponent(new DummyComponent(), true);

		auto mc = viewport->getScriptProcessor()->getMainController_();

		if (mc->getCurrentScriptLookAndFeel() != nullptr)
		{
			slaf = new ScriptingObjects::ScriptedLookAndFeel::Laf(mc);
			vp->setLookAndFeel(slaf);
		}

		component = vp;
	}
	else
	{
		model = new ColumnListBoxModel(this);
		auto table = new ListBox();

		table->setModel(model);

		table->setMultipleSelectionEnabled(false);

		table->setColour(ListBox::ColourIds::backgroundColourId, Colours::white.withAlpha(0.15f));
		table->setRowHeight(30);
		table->setWantsKeyboardFocus(true);

		if (HiseDeviceSimulator::isMobileDevice())
			table->setRowSelectedOnMouseDown(false);


		table->getViewport()->setScrollOnDragEnabled(true);

		component = table;
	}
	
	initAllProperties();
	updateValue(viewport->value);
}


ScriptCreatedComponentWrappers::ViewportWrapper::~ViewportWrapper()
{
	component = nullptr;
	model = nullptr;
}

void ScriptCreatedComponentWrappers::ViewportWrapper::updateComponent()
{
	auto vpc = dynamic_cast<ScriptingApi::Content::ScriptedViewport*>(getScriptComponent());

	if (shouldUseList)
	{
		updateFont(vpc);

		updateColours();
		updateItems(vpc);

	}
	else
	{
		auto vp = dynamic_cast<Viewport*>(component.get());

		vp->setScrollBarThickness(vpc->getScriptObjectProperty(ScriptingApi::Content::ScriptedViewport::Properties::scrollbarThickness));
		vp->setColour(ScrollBar::ColourIds::thumbColourId, GET_OBJECT_COLOUR(itemColour));
	}
}



void ScriptCreatedComponentWrappers::ViewportWrapper::updateComponent(int propertyIndex, var newValue)
{
	if (propertyIndex < ScriptingApi::Content::ScriptComponent::Properties::numProperties)
	{
		ScriptCreatedComponentWrapper::updateComponent(propertyIndex, newValue);
	}

	auto vpc = dynamic_cast<ScriptingApi::Content::ScriptedViewport*>(getScriptComponent());
	auto vp = dynamic_cast<Viewport*>(component.get());

	if (shouldUseList)
	{
		switch (propertyIndex)
		{
			PROPERTY_CASE::ScriptedViewport::Properties::FontName:
			PROPERTY_CASE::ScriptedViewport::Properties::FontSize :
			PROPERTY_CASE::ScriptedViewport::Properties::FontStyle :
			PROPERTY_CASE::ScriptedViewport::Properties::Alignment : updateFont(vpc); break;
			PROPERTY_CASE::ScriptComponent::bgColour:
			PROPERTY_CASE::ScriptComponent::itemColour :
			PROPERTY_CASE::ScriptComponent::itemColour2 :
			PROPERTY_CASE::ScriptComponent::textColour : updateColours(); break;
			PROPERTY_CASE::ScriptedViewport::Properties::Items: updateItems(vpc); break;
		}
	}
	else
	{
		switch (propertyIndex)
		{
			PROPERTY_CASE::ScriptedViewport::Properties::scrollbarThickness: vp->setScrollBarThickness(newValue); break;
			PROPERTY_CASE::ScriptComponent::itemColour: vp->setColour(ScrollBar::ColourIds::thumbColourId, GET_OBJECT_COLOUR(itemColour)); break;
		}
	}
}

void ScriptCreatedComponentWrappers::ViewportWrapper::updateValue(var newValue)
{
	auto listBox = dynamic_cast<ListBox*>(component.get());

	if (listBox != nullptr)
	{
		int viewportIndex = (int)newValue;
		listBox->selectRow(viewportIndex);
	}
}

void ScriptCreatedComponentWrappers::ViewportWrapper::updateItems(ScriptingApi::Content::ScriptedViewport * vpc)
{
	auto listBox = dynamic_cast<ListBox*>(component.get());

	if (listBox != nullptr)
	{
		if (model->shouldUpdate(vpc->getItemList()))
		{
			
			model->setItems(vpc->getItemList());
			listBox->deselectAllRows();
			listBox->repaint();
		}

		listBox->updateContent();

		//updateValue(getScriptComponent()->getValue());
	}
}



void ScriptCreatedComponentWrappers::ViewportWrapper::updateColours()
{
	auto listBox = dynamic_cast<ListBox*>(component.get());

	if (listBox != nullptr)
	{

		auto itemColour1 = GET_OBJECT_COLOUR(itemColour);
		auto itemColour2 = GET_OBJECT_COLOUR(itemColour2);
		auto bgColour = GET_OBJECT_COLOUR(bgColour);
		auto textColour = GET_OBJECT_COLOUR(textColour);


		model->itemColour1 = itemColour1;
		model->itemColour2 = itemColour2;
		model->bgColour = bgColour;
		model->textColour = textColour;



		listBox->getViewport()->setColour(ScrollBar::ColourIds::thumbColourId, itemColour1);

		listBox->setColour(ListBox::ColourIds::backgroundColourId, bgColour);
		listBox->setColour(ListBox::ColourIds::outlineColourId, itemColour2);
		listBox->repaint();

	}
}

void ScriptCreatedComponentWrappers::ViewportWrapper::updateFont(ScriptingApi::Content::ScriptedViewport * vpc)
{
	auto listBox = dynamic_cast<ListBox*>(component.get());

	if (listBox != nullptr)
	{
		Font f;

		const String fontName = vpc->getScriptObjectProperty(ScriptingApi::Content::ScriptedViewport::FontName).toString();
		const String fontStyle = vpc->getScriptObjectProperty(ScriptingApi::Content::ScriptedViewport::FontStyle).toString();
		const float fontSize = (float)vpc->getScriptObjectProperty(ScriptingApi::Content::ScriptedViewport::FontSize);

		if (fontName == "Oxygen" || fontName == "Default")
		{
			if (fontStyle == "Bold")
			{
				f = GLOBAL_BOLD_FONT().withHeight(fontSize);
			}
			else
			{
				f = GLOBAL_FONT().withHeight(fontSize);
			}
		}
		else if (fontName == "Source Code Pro")
		{
			f = GLOBAL_MONOSPACE_FONT().withHeight(fontSize);
		}
		else
		{
			const juce::Typeface::Ptr typeface = dynamic_cast<const Processor*>(contentComponent->getScriptProcessor())->getMainController()->getFont(fontName);

			if (typeface != nullptr)
			{
				f = Font(typeface).withHeight(fontSize);
			}
			else
			{
				f = Font(fontName, fontStyle, fontSize);
			}
		}

		model->font = f;
		model->justification = vpc->getJustification();
		listBox->setRowHeight((int)f.getHeight() + 15);
		listBox->repaint();
	}
}


ScriptCreatedComponentWrappers::ImageWrapper::ImageWrapper(ScriptContentComponent *content, ScriptingApi::Content::ScriptImage *img, int index):
ScriptCreatedComponentWrapper(content, index)
{
	ImageComponentWithMouseCallback *i = new ImageComponentWithMouseCallback();

	i->setName(img->name.toString());

	//i->setInterceptsMouseClicks(false, false);

    
    i->addMouseCallbackListener(this);
    
	component = i;

	initAllProperties();

}

void ScriptCreatedComponentWrappers::ImageWrapper::updateComponent()
{
	ImageComponentWithMouseCallback *ic = dynamic_cast<ImageComponentWithMouseCallback*>(component.get());
	ScriptingApi::Content::ScriptImage *si = dynamic_cast<ScriptingApi::Content::ScriptImage*>(getScriptComponent());

	if (si->getImage().isValid())
	{
		const StringArray sa = si->getItemList();

		ic->setAllowCallback(si->getScriptObjectProperty(ScriptingApi::Content::ScriptImage::AllowCallbacks).toString());
        ic->setInterceptsMouseClicks(true, false);
        ic->setPopupMenuItems(si->getItemList());
		ic->setUseRightClickForPopup(si->getScriptObjectProperty(ScriptingApi::Content::ScriptImage::PopupOnRightClick));

        ic->setBounds(si->getPosition());
		ic->setImage(si->getImage());
        ic->setOffset(si->getScriptObjectProperty(ScriptingApi::Content::ScriptImage::Offset));
        ic->setScale(si->getScriptObjectProperty(ScriptingApi::Content::ScriptImage::Scale));
		ic->setAlpha(si->getScriptObjectProperty(ScriptingApi::Content::ScriptImage::Alpha));
	}
	else
	{
		ic->setBounds(si->getPosition());
		ic->setImage(PoolHelpers::getEmptyImage(ic->getWidth(), ic->getHeight()));
	}

	contentComponent->repaint();
}

void ScriptCreatedComponentWrappers::ImageWrapper::updateComponent(int propertyIndex, var newValue)
{
	ScriptCreatedComponentWrapper::updateComponent(propertyIndex, newValue);

	ImageComponentWithMouseCallback *ic = dynamic_cast<ImageComponentWithMouseCallback*>(component.get());
	ScriptingApi::Content::ScriptImage *si = dynamic_cast<ScriptingApi::Content::ScriptImage*>(getScriptComponent());

	switch (propertyIndex)
	{
		PROPERTY_CASE::ScriptImage::AllowCallbacks:
		PROPERTY_CASE::ScriptImage::PopupMenuItems :
		PROPERTY_CASE::ScriptImage::PopupOnRightClick : updatePopupMenu(si, ic); break;
		PROPERTY_CASE::ScriptImage::FileName:
		PROPERTY_CASE::ScriptImage::Offset :
		PROPERTY_CASE::ScriptImage::Scale :
		PROPERTY_CASE::ScriptImage::Alpha : updateImage(ic, si); break;
	}
}

void ScriptCreatedComponentWrappers::ImageWrapper::updateImage(ImageComponentWithMouseCallback * ic, ScriptingApi::Content::ScriptImage * si)
{
	ic->setImage(si->getImage());
	ic->setOffset(si->getScriptObjectProperty(ScriptingApi::Content::ScriptImage::Offset));
	ic->setScale(si->getScriptObjectProperty(ScriptingApi::Content::ScriptImage::Scale));
	ic->setAlpha(si->getScriptObjectProperty(ScriptingApi::Content::ScriptImage::Alpha));
}

void ScriptCreatedComponentWrappers::ImageWrapper::updatePopupMenu(ScriptingApi::Content::ScriptImage * si, ImageComponentWithMouseCallback * ic)
{
	const StringArray sa = si->getItemList();
	ic->setAllowCallback(si->getScriptObjectProperty(ScriptingApi::Content::ScriptImage::AllowCallbacks).toString());
	ic->setInterceptsMouseClicks(true, false);
	ic->setPopupMenuItems(si->getItemList());
	ic->setUseRightClickForPopup(si->getScriptObjectProperty(ScriptingApi::Content::ScriptImage::PopupOnRightClick));
}

void ScriptCreatedComponentWrappers::ImageWrapper::mouseCallback(const var &mouseInformation)
{
    changed(mouseInformation);
}

ScriptCreatedComponentWrappers::PanelWrapper::PanelWrapper(ScriptContentComponent *content, ScriptingApi::Content::ScriptPanel *panel, int index) :
ScriptCreatedComponentWrapper(content, index)
{
	initPanel(panel);

}

ScriptCreatedComponentWrappers::PanelWrapper::PanelWrapper(ScriptContentComponent* content, ScriptingApi::Content::ScriptPanel* panel):
	ScriptCreatedComponentWrapper(content, panel)
{
	initPanel(panel);
}

void ScriptCreatedComponentWrappers::PanelWrapper::updateComponent()
{
	jassertfalse;

	BorderPanel *bpc = dynamic_cast<BorderPanel*>(component.get());
	auto sc = dynamic_cast<ScriptingApi::Content::ScriptPanel*>(getScriptComponent());

	updateColourAndBorder(bpc);

	bpc->setPopupMenuItems(sc->getItemList());

	bpc->setOpaque(sc->getScriptObjectProperty(ScriptingApi::Content::ScriptPanel::opaque));
	bpc->setActivePopupItem((int)getScriptComponent()->getScriptObjectProperty(ScriptingApi::Content::ScriptPanel::selectedPopupIndex));
	bpc->setUseRightClickForPopup(sc->getScriptObjectProperty(ScriptingApi::Content::ScriptPanel::PopupOnRightClick));
	bpc->alignPopup(sc->getScriptObjectProperty(ScriptingApi::Content::ScriptPanel::popupMenuAlign));

	bpc->setTooltip(GET_SCRIPT_PROPERTY(tooltip));

	

	bpc->setTouchEnabled(sc->getScriptObjectProperty(ScriptingApi::Content::ScriptPanel::holdIsRightClick));

	// TODO: in updateValue
	bpc->setJSONPopupData(sc->getJSONPopupData(), sc->getPopupSize());


	updateRange(bpc);


	bpc->setInterceptsMouseClicks(sc->getScriptObjectProperty(ScriptingApi::Content::ScriptPanel::enabled), true);

	bpc->repaint();

	bpc->setAllowCallback(getScriptComponent()->getScriptObjectProperty(ScriptingApi::Content::ScriptPanel::allowCallbacks).toString());

	contentComponent->repaint();

	
}

void ScriptCreatedComponentWrappers::PanelWrapper::updateComponent(int propertyIndex, var newValue)
{
	ScriptCreatedComponentWrapper::updateComponent(propertyIndex, newValue);

	BorderPanel *bpc = dynamic_cast<BorderPanel*>(component.get());
	auto sc = dynamic_cast<ScriptingApi::Content::ScriptPanel*>(getScriptComponent());

	switch (propertyIndex)
	{
		PROPERTY_CASE::ScriptComponent::bgColour:
		PROPERTY_CASE::ScriptComponent::itemColour :
		PROPERTY_CASE::ScriptComponent::itemColour2 :
		PROPERTY_CASE::ScriptPanel::borderRadius :
		PROPERTY_CASE::ScriptPanel::borderSize :
		PROPERTY_CASE::ScriptPanel::textColour : updateColourAndBorder(bpc); break;
		PROPERTY_CASE::ScriptPanel::enableMidiLearn: bpc->setMidiLearnEnabled(newValue); break;
		PROPERTY_CASE::ScriptPanel::PopupMenuItems: bpc->setPopupMenuItems(sc->getItemList()); break;
		PROPERTY_CASE::ScriptPanel::opaque: bpc->setOpaque(newValue); break;
		PROPERTY_CASE::ScriptPanel::selectedPopupIndex: bpc->setActivePopupItem((int)newValue); break;
		PROPERTY_CASE::ScriptPanel::PopupOnRightClick: bpc->setUseRightClickForPopup(newValue); break;
		PROPERTY_CASE::ScriptPanel::popupMenuAlign: bpc->alignPopup(newValue); break;
		PROPERTY_CASE::ScriptPanel::tooltip: bpc->setTooltip(newValue); break;
		PROPERTY_CASE::ScriptPanel::holdIsRightClick: bpc->setTouchEnabled(newValue); break;
		PROPERTY_CASE::ScriptComponent::min:
		PROPERTY_CASE::ScriptComponent::max : 
		PROPERTY_CASE::ScriptPanel::stepSize : updateRange(bpc); break;
		PROPERTY_CASE::ScriptPanel::enabled: break;
		PROPERTY_CASE::ScriptPanel::allowCallbacks: bpc->setAllowCallback(newValue.toString()); break;
	}
}

void ScriptCreatedComponentWrappers::PanelWrapper::updateRange(BorderPanel * bpc)
{
	const double min = GET_SCRIPT_PROPERTY(min);
	const double max = GET_SCRIPT_PROPERTY(max);
	const double stepSize = getScriptComponent()->getScriptObjectProperty(ScriptingApi::Content::ScriptPanel::stepSize);

	NormalisableRange<double> r(min, max);
	r.interval = stepSize;

	bpc->setRange(r);
}

void ScriptCreatedComponentWrappers::PanelWrapper::updateColourAndBorder(BorderPanel * bpc)
{
	bpc->c1 = GET_OBJECT_COLOUR(itemColour);
	bpc->c2 = GET_OBJECT_COLOUR(itemColour2);
	bpc->borderColour = GET_OBJECT_COLOUR(textColour);
	bpc->borderRadius = getScriptComponent()->getScriptObjectProperty(ScriptingApi::Content::ScriptPanel::borderRadius);
	bpc->borderSize = getScriptComponent()->getScriptObjectProperty(ScriptingApi::Content::ScriptPanel::borderSize);
	bpc->repaint();
};

void ScriptCreatedComponentWrappers::PanelWrapper::updateValue(var newValue)
{
	BorderPanel *bpc = dynamic_cast<BorderPanel*>(component.get());
	auto sc = dynamic_cast<ScriptingApi::Content::ScriptPanel*>(getScriptComponent());

	if (sc->isModal())
	{
		contentComponent->setModalPopup(this, sc->isShowing(true));
	}
	else
	{
		bpc->setVisible(sc->isShowing(false));
		bpc->repaint();
	}
}

void ScriptCreatedComponentWrappers::PanelWrapper::mouseCallback(const var &mouseInformation)
{
	auto sp = dynamic_cast<ScriptingApi::Content::ScriptPanel*>(getScriptComponent());

	if(sp != nullptr)
		sp->mouseCallback(mouseInformation);
}

void ScriptCreatedComponentWrappers::PanelWrapper::boundsChanged(const Rectangle<int> &newBounds)
{
	auto sc = dynamic_cast<ScriptingApi::Content::ScriptPanel*>(getScriptComponent());

	static const Identifier x("x");
	static const Identifier y("y");

	sc->setScriptObjectPropertyWithChangeMessage(x, newBounds.getX());
	sc->setScriptObjectPropertyWithChangeMessage(y, newBounds.getY());
}

ScriptCreatedComponentWrappers::PanelWrapper::~PanelWrapper()
{
	if (auto c = getScriptComponent())
		c->removeSubComponentListener(this);

	BorderPanel *bpc = dynamic_cast<BorderPanel*>(component.get());

	bpc->removeCallbackListener(this);
	
}

void ScriptCreatedComponentWrappers::PanelWrapper::subComponentAdded(ScriptComponent* newComponent)
{
	BorderPanel *bpc = dynamic_cast<BorderPanel*>(component.get());
	auto sc = dynamic_cast<ScriptingApi::Content::ScriptPanel*>(getScriptComponent());

	for (int i = 0; i < sc->getNumSubPanels(); i++)
	{
		if (auto sp = sc->getSubPanel(i))
		{
			if (newComponent == sp)
			{
				childPanelWrappers.add(new PanelWrapper(contentComponent, sp));
				bpc->addAndMakeVisible(childPanelWrappers.getLast()->getComponent());
			}
		}
	}
}

void ScriptCreatedComponentWrappers::PanelWrapper::subComponentRemoved(ScriptComponent* componentAboutToBeRemoved)
{
	BorderPanel *bpc = dynamic_cast<BorderPanel*>(component.get());
	
	for (int i = 0; i < childPanelWrappers.size(); i++)
	{
		if (childPanelWrappers[i]->getScriptComponent() == componentAboutToBeRemoved)
		{
			bpc->removeChildComponent(childPanelWrappers[i]->getComponent());
			childPanelWrappers.remove(i--);
		}
	}
}

void ScriptCreatedComponentWrappers::PanelWrapper::initPanel(ScriptingApi::Content::ScriptPanel* panel)
{
	BorderPanel *bp = new BorderPanel(panel->getDrawActionHandler());

	panel->addSubComponentListener(this);

	bp->setName(panel->name.toString());

#if HISE_INCLUDE_RLOTTIE
	bp->setAnimation(panel->getAnimation());
#endif

	bp->addMouseCallbackListener(this);
	bp->setDraggingEnabled(panel->getScriptObjectProperty(ScriptingApi::Content::ScriptPanel::allowDragging));
	bp->setDragBounds(panel->getDragBounds(), this);
	bp->setOpaque(panel->getScriptObjectProperty(ScriptingApi::Content::ScriptPanel::opaque));
	bp->isPopupPanel = panel->getScriptObjectProperty(ScriptingApi::Content::ScriptPanel::isPopupPanel);
	bp->setJSONPopupData(panel->getJSONPopupData(), panel->getPopupSize());
	bp->setup(getProcessor(), getIndex(), panel->name.toString());
	bp->isUsingCustomImage = panel->isUsingCustomPaintRoutine() || panel->isUsingClippedFixedImage();

	auto cursor = panel->getMouseCursorPath();

	if (!cursor.path.isEmpty())
	{
		auto s = 80;

		Image icon(Image::ARGB, s, s, true);
		Graphics g(icon);

		PathFactory::scalePath(cursor.path, { 0.0f, 0.0f, (float)s, (float)s });

		g.setColour(cursor.c);
		g.fillPath(cursor.path);
		MouseCursor c(icon, roundToInt(cursor.hitPoint.x * s), roundToInt(cursor.hitPoint.y * s));
		bp->setMouseCursor(c);
	}

	component = bp;

	initAllProperties();

	rebuildChildPanels();

	panel->repaint();

}

void ScriptCreatedComponentWrappers::PanelWrapper::rebuildChildPanels()
{
	BorderPanel *bpc = dynamic_cast<BorderPanel*>(component.get());
	auto sc = dynamic_cast<ScriptingApi::Content::ScriptPanel*>(getScriptComponent());

	for (int i = 0; i < sc->getNumSubPanels(); i++)
	{
		if (auto sp = sc->getSubPanel(i))
		{
			childPanelWrappers.add(new PanelWrapper(contentComponent, sp));
			bpc->addAndMakeVisible(childPanelWrappers.getLast()->getComponent());
		}
	}
}

ScriptCreatedComponentWrappers::SliderPackWrapper::SliderPackWrapper(ScriptContentComponent *content, ScriptingApi::Content::ScriptSliderPack *pack, int index) :
ScriptCreatedComponentWrapper(content, index)
{
	SliderPack *sp = new SliderPack(pack->getSliderPackData());

	sp->addListener(this);
	sp->setName(pack->name.toString());

	sp->setSliderWidths(pack->widthArray);

	component = sp;

	initAllProperties();
}

void ScriptCreatedComponentWrappers::SliderPackWrapper::updateComponent()
{
	SliderPack *sp = dynamic_cast<SliderPack*>(component.get());
	
	updateColours(sp);
}

void ScriptCreatedComponentWrappers::SliderPackWrapper::updateComponent(int propertyIndex, var newValue)
{
	ScriptCreatedComponentWrapper::updateComponent(propertyIndex, newValue);

	SliderPack *sp = dynamic_cast<SliderPack*>(component.get());
	ScriptingApi::Content::ScriptSliderPack *ssp = dynamic_cast<ScriptingApi::Content::ScriptSliderPack*>(getScriptComponent());

	switch (propertyIndex)
	{
		PROPERTY_CASE::ScriptComponent::itemColour :
		PROPERTY_CASE::ScriptComponent::itemColour2 :
		PROPERTY_CASE::ScriptComponent::bgColour :
		PROPERTY_CASE::ScriptComponent::textColour : updateColours(sp); break;
		PROPERTY_CASE::ScriptSliderPack::Properties::FlashActive: sp->setFlashActive(newValue); break;
		PROPERTY_CASE::ScriptSliderPack::Properties::ShowValueOverlay : sp->setShowValueOverlay(newValue); break;
		PROPERTY_CASE::ScriptSliderPack::Properties::StepSize:
		PROPERTY_CASE::ScriptComponent::Properties::min :
		PROPERTY_CASE::ScriptComponent::Properties::max : updateRange(ssp->getSliderPackData());
	}
}

void ScriptCreatedComponentWrappers::SliderPackWrapper::updateColours(SliderPack * sp)
{
	sp->setColourForSliders(Slider::thumbColourId, GET_OBJECT_COLOUR(itemColour));
	sp->setColour(Slider::ColourIds::textBoxOutlineColourId, GET_OBJECT_COLOUR(itemColour2));
	sp->setColour(Slider::backgroundColourId, GET_OBJECT_COLOUR(bgColour));
	sp->setColourForSliders(Slider::trackColourId, GET_OBJECT_COLOUR(textColour));

	sp->repaint();
}

void ScriptCreatedComponentWrappers::SliderPackWrapper::updateRange(SliderPackData* data)
{
	ScriptingApi::Content::ScriptSliderPack *ssp = dynamic_cast<ScriptingApi::Content::ScriptSliderPack*>(getScriptComponent());

	double min = GET_SCRIPT_PROPERTY(min);
	double max = GET_SCRIPT_PROPERTY(max);
	double stepSize = ssp->getScriptObjectProperty(ScriptingApi::Content::ScriptSliderPack::Properties::StepSize);

	if (!ssp->getConnectedProcessor())
	{
		data->setRange(min, max, stepSize);
		SliderPack *sp = dynamic_cast<SliderPack*>(component.get());
		sp->updateSliders();
	}
}

void ScriptCreatedComponentWrappers::SliderPackWrapper::updateValue(var newValue)
{
	SliderPack *sp = dynamic_cast<SliderPack*>(component.get());
	ScriptingApi::Content::ScriptSliderPack *ssp = dynamic_cast<ScriptingApi::Content::ScriptSliderPack*>(getScriptComponent());

	jassert(ssp->getSliderPackData() == sp->getData());

    sp->setSliderWidths(ssp->widthArray);
    
	if (sp->getNumSliders() != ssp->getSliderPackData()->getNumSliders())
	{
		if (ssp->getNumSliders() > 0)
		{
			sp->setNumSliders(ssp->getNumSliders());
		}
		else
		{
			// Somehow, the slider amount got zeroed...
			jassertfalse;
		}
	}
	else
	{
		sp->updateSliders();
		sp->repaint();
	}
}

class ScriptCreatedComponentWrappers::AudioWaveformWrapper::SamplerListener : public SafeChangeListener,
																			  public SampleMap::Listener
{
public:

	SamplerListener(ModulatorSampler* s_, SamplerSoundWaveform* waveform_) :
		s(s_),
		samplemap(s->getSampleMap()),
		waveform(waveform_)
	{
		samplemap->addListener(this);
		s->addChangeListener(this);

		if (auto v = s->getLastStartedVoice())
			lastSound = v->getCurrentlyPlayingSound();
	}

	~SamplerListener()
	{
		lastSound = nullptr;

		if (s != nullptr)
			s->removeChangeListener(this);

		if (samplemap.get() != nullptr)
			samplemap->removeListener(this);
	}

	void refreshAfterSampleMapChange()
	{
		if (displayedIndex != -1)
		{
			if (auto newSound = s->getSound(displayedIndex))
			{
				waveform->setSoundToDisplay(dynamic_cast<ModulatorSamplerSound*>(newSound), 0);
				lastSound = newSound;
			}
			else
			{
				waveform->setSoundToDisplay(nullptr, 0);
				lastSound = nullptr;
			}
		}
	}

	void sampleMapWasChanged(PoolReference ) override
	{
		refreshAfterSampleMapChange();
	}

	void sampleAmountChanged() override 
	{
		refreshAfterSampleMapChange();
	};

	void sampleMapCleared() override 
	{
		refreshAfterSampleMapChange();
	}

    void setActive(bool shouldBeActive)
    {
        active = shouldBeActive;
    }
    
	void changeListenerCallback(SafeChangeBroadcaster* /*b*/) override
	{
        if(!active)
            return;
        
		if (auto v = s->getLastStartedVoice())
		{
			auto thisSound = v->getCurrentlyPlayingSound();

			if (thisSound != lastSound)
			{
				lastSound = thisSound;

				waveform->setSoundToDisplay(dynamic_cast<ModulatorSamplerSound*>(thisSound.get()), 0);
			}
		}
	}


	int displayedIndex = -1;

    bool active = true;
	WeakReference<ModulatorSampler> s;
	WeakReference<SampleMap> samplemap;
	Component::SafePointer<SamplerSoundWaveform> waveform;
	SynthesiserSound::Ptr lastSound;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SamplerListener);
};



ScriptCreatedComponentWrappers::AudioWaveformWrapper::AudioWaveformWrapper(ScriptContentComponent *content, ScriptingApi::Content::ScriptAudioWaveform *form, int index):
ScriptCreatedComponentWrapper(content, index)
{
	if (auto s = form->getSampler())
	{
		SamplerSoundWaveform* ssw = new SamplerSoundWaveform(s);
		ssw->setName(form->name.toString());

		ssw->getSampleArea(SamplerSoundWaveform::PlayArea)->setAreaEnabled(false);

		ssw->setIsOnInterface(true);

		component = ssw;

		samplerListener = new SamplerListener(s, ssw);
	}
	else if ((saf = form->getScriptAudioFile()))
	{
		auto asb = new ScriptAudioFileBufferComponent(saf);
		asb->setName(form->name.toString());
		component = asb;
	}
	else
	{
		auto asb = new AudioSampleProcessorBufferComponent(form->getConnectedProcessor());

		asb->setName(form->name.toString());
		asb->addAreaListener(this);

		component = asb;
	}

	initAllProperties();
}



ScriptCreatedComponentWrappers::AudioWaveformWrapper::~AudioWaveformWrapper()
{
	samplerListener = nullptr;
	saf = nullptr;
}

void ScriptCreatedComponentWrappers::AudioWaveformWrapper::updateComponent()
{
	
}

void ScriptCreatedComponentWrappers::AudioWaveformWrapper::updateComponent(int propertyIndex, var newValue)
{
	ScriptCreatedComponentWrapper::updateComponent(propertyIndex, newValue);

	auto form = dynamic_cast<ScriptingApi::Content::ScriptAudioWaveform *>(getScriptComponent());

	if (auto adc = dynamic_cast<AudioDisplayComponent*>(component.get()))
	{
		switch (propertyIndex)
		{
			PROPERTY_CASE::ScriptComponent::enabled: adc->getSampleArea(0)->setEnabled((bool)newValue); break;
			PROPERTY_CASE::ScriptAudioWaveform::Properties::opaque:	adc->setOpaque((bool)newValue); break;
			PROPERTY_CASE::ScriptAudioWaveform::Properties::itemColour3:
			PROPERTY_CASE::ScriptComponent::visible : newValue ? startTimer(50) : stopTimer(); break;
			PROPERTY_CASE::ScriptComponent::itemColour :
			PROPERTY_CASE::ScriptComponent::itemColour2 :
			PROPERTY_CASE::ScriptComponent::bgColour :
			PROPERTY_CASE::ScriptComponent::textColour : updateColours(adc); break;
			PROPERTY_CASE::ScriptAudioWaveform::Properties::showLines: adc->getThumbnail()->setDrawHorizontalLines((bool)newValue); break;
            PROPERTY_CASE::ScriptAudioWaveform::Properties::sampleIndex: updateSampleIndex(form, adc, newValue); break;
		}

		if (auto asb = dynamic_cast<AudioSampleProcessorBufferComponent*>(component.get()))
		{
			switch (propertyIndex)
			{
				PROPERTY_CASE::ScriptComponent::processorId: asb->setAudioSampleProcessor(form->getConnectedProcessor()); break;
				PROPERTY_CASE::ScriptAudioWaveform::Properties::showFileName: asb->setShowFileName((bool)newValue); break;
			}
		}
	}
}

void ScriptCreatedComponentWrappers::AudioWaveformWrapper::updateSampleIndex(ScriptingApi::Content::ScriptAudioWaveform *form, AudioDisplayComponent* asb, int newValue)
{
    if (auto s = form->getSampler())
    {
        if(auto ssw = dynamic_cast<SamplerSoundWaveform*>(asb))
        {
            if(samplerListener != nullptr)
            {
                samplerListener->setActive(newValue == -1);
				samplerListener->displayedIndex = newValue;
            }
            
            if(newValue != -1 && lastIndex != newValue)
            {
                ssw->setSoundToDisplay(dynamic_cast<ModulatorSamplerSound*>(s->getSound(newValue)), 0);
                lastIndex = newValue;
            }
        }
    }
}
    
void ScriptCreatedComponentWrappers::AudioWaveformWrapper::rangeChanged(AudioDisplayComponent *broadcaster, int changedArea)
{

	ScriptingApi::Content::ScriptAudioWaveform * form = dynamic_cast<ScriptingApi::Content::ScriptAudioWaveform *>(getScriptComponent());

	if (form != nullptr)
	{
		Range<int> newRange = broadcaster->getSampleArea(changedArea)->getSampleRange();

		//form->getAudioProcessor()->setLoadedFile(dynamic_cast<AudioSampleBufferComponent*>(broadcaster)->getCurrentlyLoadedFileName());
		form->getAudioProcessor()->setRange(newRange);
	}
}

void ScriptCreatedComponentWrappers::AudioWaveformWrapper::timerCallback()
{
	auto asb = dynamic_cast<AudioSampleProcessorBufferComponent*>(component.get());

	if(asb != nullptr)
		asb->updatePlaybackPosition();

	
}

void ScriptCreatedComponentWrappers::AudioWaveformWrapper::updateColours(AudioDisplayComponent* asb)
{
	auto tn = asb->getThumbnail();

	asb->setColour(AudioDisplayComponent::ColourIds::bgColour, GET_OBJECT_COLOUR(bgColour));
	tn->setColour(AudioDisplayComponent::ColourIds::outlineColour, GET_OBJECT_COLOUR(itemColour));
	tn->setColour(AudioDisplayComponent::ColourIds::fillColour, GET_OBJECT_COLOUR(itemColour2));
	tn->setColour(AudioDisplayComponent::ColourIds::textColour, GET_OBJECT_COLOUR(textColour));


	asb->repaint();
	tn->repaint();
}


ScriptCreatedComponentWrappers::FloatingTileWrapper::FloatingTileWrapper(ScriptContentComponent *content, ScriptingApi::Content::ScriptFloatingTile *floatingTile, int index):
	ScriptCreatedComponentWrapper(content, index)
{
	auto mc = const_cast<MainController*>(dynamic_cast<const Processor*>(content->getScriptProcessor())->getMainController());

	auto ft = new FloatingTile(mc, nullptr);
	ft->setIsFloatingTileOnInterface();
	component = ft;

	ft->setName(floatingTile->name.toString());
	ft->setOpaque(false);
	ft->setContent(floatingTile->getContentData());
	ft->refreshRootLayout();
}



void ScriptCreatedComponentWrappers::FloatingTileWrapper::updateComponent()
{
}

void ScriptCreatedComponentWrappers::FloatingTileWrapper::updateComponent(int propertyIndex, var newValue)
{
	ScriptCreatedComponentWrapper::updateComponent(propertyIndex, newValue);

	auto sft = dynamic_cast<ScriptingApi::Content::ScriptFloatingTile*>(getScriptComponent());
	auto ft = dynamic_cast<FloatingTile*>(component.get());

	auto ftc = ft->getCurrentFloatingPanel();

	if (ftc == nullptr)
		return;

	switch (propertyIndex)
	{
	PROPERTY_CASE::ScriptComponent::itemColour: 
	PROPERTY_CASE::ScriptComponent::itemColour2: 
	PROPERTY_CASE::ScriptComponent::bgColour: 
	PROPERTY_CASE::ScriptComponent::textColour: 
	PROPERTY_CASE::ScriptFloatingTile::Properties::Font:
	PROPERTY_CASE::ScriptFloatingTile::Properties::FontSize:
	PROPERTY_CASE::ScriptFloatingTile::Properties::Data :
	PROPERTY_CASE::ScriptFloatingTile::Properties::ContentType: ft->setContent(sft->getContentData()); break;
	}

#if USE_BACKEND

	// This will cause the properties to update and show the properties in the edit panel
	if (propertyIndex == ScriptingApi::Content::ScriptFloatingTile::ContentType)
	{
		sft->fillScriptPropertiesWithFloatingTile(ft);
	}
#endif
}

void ScriptCreatedComponentWrappers::FloatingTileWrapper::updateValue(var newValue)
{
	auto sft = dynamic_cast<ScriptingApi::Content::ScriptFloatingTile*>(getScriptComponent());

	auto ft = dynamic_cast<FloatingTile*>(component.get());

	const bool updateAfterInit = (bool)sft->getScriptObjectProperty(ScriptingApi::Content::ScriptFloatingTile::Properties::updateAfterInit);

	if (updateAfterInit)
	{
		ft->setContent(sft->getContentData());
		ft->refreshRootLayout();
		
	}
}

typedef ScriptingApi::Content::ScriptComponent ScriptedComponent;

ScriptedControlAudioParameter::ScriptedControlAudioParameter(ScriptingApi::Content::ScriptComponent *newComponent, AudioProcessor *parentProcessor_, ScriptBaseMidiProcessor *scriptProcessor_, int index_) :
  AudioProcessorParameterWithID(newComponent->getName().toString(), 
								getNameForComponent(newComponent)),
  id(newComponent->getName()),
  parentProcessor(parentProcessor_),
  type(getType(newComponent)),
  scriptProcessor(scriptProcessor_),
  componentIndex(index_),
  suffix(String()),
  deactivated(false)
{
	setControlledScriptComponent(newComponent);
}

void ScriptedControlAudioParameter::setControlledScriptComponent(ScriptingApi::Content::ScriptComponent *newComponent)
{
	ScriptedComponent *c = newComponent;

	if (c != nullptr)
	{
		const float min = c->getScriptObjectProperty(ScriptingApi::Content::ScriptComponent::Properties::min);
		const float max = c->getScriptObjectProperty(ScriptingApi::Content::ScriptComponent::Properties::max);

		range = NormalisableRange<float>(min, max);

        isMeta = c->getScriptObjectProperty(ScriptingApi::Content::ScriptComponent::Properties::isMetaParameter);
        
		switch (type)
		{
		case ScriptedControlAudioParameter::Type::Slider:
		{
			range.interval = c->getScriptObjectProperty(ScriptingApi::Content::ScriptSlider::Properties::stepSize);

			const float midPoint = (float)c->getScriptObjectProperty(ScriptingApi::Content::ScriptSlider::Properties::middlePosition);

			if (range.getRange().contains(midPoint))
			{
				range.skew = (float)HiSlider::getSkewFactorFromMidPoint((double)min, (double)max, (double)midPoint);

				if (range.skew == 0.0f)
				{
					// You have some weird ranges going on here...
					jassertfalse;
					range.skew = 1.0f;
				}
			}

			suffix = c->getScriptObjectProperty(ScriptingApi::Content::ScriptSlider::Properties::suffix);
			break;
		}
		case ScriptedControlAudioParameter::Type::Button:
			range.interval = 1.0f;
            if((int)c->getScriptObjectProperty(ScriptingApi::Content::ScriptButton::radioGroup) != 0)
                isMeta = true;
			break;
		case ScriptedControlAudioParameter::Type::ComboBox:
			range.interval = 1.0f;
			itemList = dynamic_cast<ScriptingApi::Content::ScriptComboBox*>(c)->getItemList();
			break;
		case ScriptedControlAudioParameter::Type::Panel:
			range.interval = jmax<float>(0.001f, c->getScriptObjectProperty(ScriptingApi::Content::ScriptPanel::Properties::stepSize));
			break;
		case ScriptedControlAudioParameter::Type::Unsupported:
			// This should be taken care of before creation of this object...
			jassertfalse;
			break;
		default:
			break;
		}
	}
}

float ScriptedControlAudioParameter::getValue() const
{
	if (scriptProcessor.get() != nullptr)
	{
		const float value = jlimit<float>(0.0f, 1.0f, range.convertTo0to1(scriptProcessor->getAttribute(componentIndex)));

		return value;
		
	}
	else
	{
		jassertfalse;
		return 0.0f;
	}
}

void ScriptedControlAudioParameter::setValue(float newValue)
{
	if (scriptProcessor.get() != nullptr)
	{
		bool *enableUpdate = &dynamic_cast<MainController*>(parentProcessor)->getPluginParameterUpdateState();

		if (enableUpdate)
		{
			ScopedValueSetter<bool> setter(*enableUpdate, false, true);

			const float convertedValue = range.convertFrom0to1(newValue);
			const float snappedValue = range.snapToLegalValue(convertedValue);

			if (!lastValueInitialised || lastValue != snappedValue)
			{
				lastValue = snappedValue;
				lastValueInitialised = true;
				scriptProcessor->setAttribute(componentIndex, snappedValue, sendNotification);
			}
		}
	}
	else
	{
		jassertfalse;
	}
}

float ScriptedControlAudioParameter::getDefaultValue() const
{
	if (scriptProcessor.get() != nullptr && type == Type::Slider)
	{
		const float v = range.convertTo0to1(scriptProcessor->getDefaultValue(componentIndex));

		return jlimit<float>(0.0f, 1.0f, v);
	}
	else
	{
		return 0.0f;
	}
}



String ScriptedControlAudioParameter::getLabel() const
{
	if (type == Type::Slider)
	{
		return suffix;
	}
	else return String();
}

String ScriptedControlAudioParameter::getText(float value, int) const
{
	switch (type)
	{
	case ScriptedControlAudioParameter::Type::Slider:

		return String(range.convertFrom0to1(jlimit(0.0f, 1.0f, value)), 1);
		break;
	case ScriptedControlAudioParameter::Type::Button:
		return value > 0.5f ? "On" : "Off";
		break;
	case ScriptedControlAudioParameter::Type::ComboBox:
	{
		const int index = jlimit<int>(0, itemList.size() - 1, (int)(value*(float)itemList.size()));

		return itemList[index];
		break;
	}
	case ScriptedControlAudioParameter::Type::Panel:
	{
		return String((int)range.convertFrom0to1(jlimit(0.0f, 1.0f, value)));
	}
		
	case ScriptedControlAudioParameter::Type::Unsupported:
	default:
		jassertfalse;
		break;
	}

	return String();
}

float ScriptedControlAudioParameter::getValueForText(const String &text) const
{
	switch (type)
	{
	case ScriptedControlAudioParameter::Type::Slider:
		return text.getFloatValue();
		break;
	case ScriptedControlAudioParameter::Type::Button:
		return text == "On" ? 1.0f : 0.0f;
		break;
	case ScriptedControlAudioParameter::Type::ComboBox:
		return (float)itemList.indexOf(text);
		break;
	case ScriptedControlAudioParameter::Type::Panel:
		return (float)text.getIntValue();
	case ScriptedControlAudioParameter::Type::Unsupported:
		break;
	default:
		break;
	}

	return 0.0f;
}

int ScriptedControlAudioParameter::getNumSteps() const
{
	switch (type)
	{
	case ScriptedControlAudioParameter::Type::Slider:
		return (int)((float)range.getRange().getLength() / range.interval);
		break;
	case ScriptedControlAudioParameter::Type::Button:
		return 2;
		break;
	case ScriptedControlAudioParameter::Type::ComboBox:
		return itemList.size();
	case ScriptedControlAudioParameter::Type::Panel:

		return range.interval != 0.0 ? (int)((float)range.getRange().getLength() / range.interval) :
									   (int)range.getRange().getLength();
	case ScriptedControlAudioParameter::Type::Unsupported:
		break;
	default:
		break;
	}

	return parentProcessor->getDefaultNumParameterSteps();
}

bool ScriptedControlAudioParameter::isMetaParameter() const
{
    return isMeta;
}
    
void ScriptedControlAudioParameter::setParameterNotifyingHost(int index, float newValue)
{
	auto mc = dynamic_cast<MainController*>(parentProcessor);

	if (mc->getKillStateHandler().getCurrentThread() == MainController::KillStateHandler::AudioThread)
	{
		indexForHost = index;
		valueForHost = newValue;
		triggerAsyncUpdate();
	}
	else
		setParameterNotifyingHostInternal(index, newValue);
}

void ScriptedControlAudioParameter::setParameterNotifyingHostInternal(int index, float newValue)
{
	ScopedValueSetter<bool> setter(dynamic_cast<MainController*>(parentProcessor)->getPluginParameterUpdateState(), false, true);

	auto sanitizedValue = jlimit<float>(range.start, range.end, newValue);

	parentProcessor->setParameterNotifyingHost(index, range.convertTo0to1(sanitizedValue));
}



ScriptedControlAudioParameter::Type ScriptedControlAudioParameter::getType(ScriptingApi::Content::ScriptComponent *component)
{
	if (dynamic_cast<ScriptingApi::Content::ScriptSlider*>(component)) return Type::Slider;
	else if (dynamic_cast<ScriptingApi::Content::ScriptComboBox*>(component)) return Type::ComboBox;
	else if (dynamic_cast<ScriptingApi::Content::ScriptButton*>(component)) return Type::Button;
	else if (dynamic_cast<ScriptingApi::Content::ScriptPanel*>(component)) return Type::Panel;
	else return Type::Unsupported;
}

ScriptCreatedComponentWrappers::ViewportWrapper::ColumnListBoxModel::ColumnListBoxModel(ViewportWrapper* parent_):
	parent(parent_),
	font(GLOBAL_BOLD_FONT()),
	justification(Justification::centredLeft)
{

}

int ScriptCreatedComponentWrappers::ViewportWrapper::ColumnListBoxModel::getNumRows()
{
	return list.size();
}

void ScriptCreatedComponentWrappers::ViewportWrapper::ColumnListBoxModel::listBoxItemClicked(int row, const MouseEvent &)
{
	parent->changed(row);
}

void ScriptCreatedComponentWrappers::ViewportWrapper::ColumnListBoxModel::paintListBoxItem(int rowNumber, Graphics &g, int width, int height, bool rowIsSelected)
{
	if (rowNumber < list.size())
	{
		auto text = list[rowNumber];

		Rectangle<int> area(0, 1, width, height - 2);

		g.setColour(rowIsSelected ? itemColour1 : bgColour);
		g.fillRect(area);
		g.setColour(itemColour2);
		if (rowIsSelected) g.drawRect(area, 1);

		g.setColour(textColour);
		g.setFont(font);
		g.drawText(text, 10, 0, width - 20, height, justification);
	}
}

void ScriptCreatedComponentWrappers::ViewportWrapper::ColumnListBoxModel::returnKeyPressed(int row)
{
	parent->changed(row);
}


void ScriptCreatedComponentWrapper::ValuePopup::updateText()
{
	auto thisText = parent.getTextForValuePopup();

	Properties::Ptr p = parent.contentComponent->getValuePopupProperties();

	if (p != nullptr && thisText != currentText)
	{
		currentText = thisText;

		int margin = (int)p->getLayoutData().margin;

		int newWidth = p->getFont().getStringWidth(currentText) + 2 * margin + 5;

		setSize(newWidth, (int)p->getFont().getHeight() + 2*margin);


		repaint();
	}
}

void ScriptCreatedComponentWrapper::ValuePopup::paint(Graphics& g)
{
	Properties::Ptr p = parent.contentComponent->getValuePopupProperties();

	

	if (p != nullptr)
	{
		auto l = p->getLayoutData();

		auto ar = getLocalBounds().toFloat().reduced(l.lineThickness * 0.5f);

		g.setGradientFill(ColourGradient(p->getColour(Properties::itemColour), 0.0f, 0.0f, 
										 p->getColour(Properties::itemColour2), 0.0f, (float)getHeight(), false));

		g.fillRoundedRectangle(ar, l.radius);

		g.setColour(p->getColour(Properties::bgColour));
		g.drawRoundedRectangle(ar, l.radius, l.lineThickness);

		g.setFont(p->getFont());
		g.setColour(p->getColour(Properties::textColour));
		g.drawText(currentText, getLocalBounds(), Justification::centred);
	}

	
}

} // namespace hise
