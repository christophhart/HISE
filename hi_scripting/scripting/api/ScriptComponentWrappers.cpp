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

#define GET_SCRIPT_PROPERTY(id) (getContent()->getComponent(getIndex())->getScriptObjectProperty(ScriptingApi::Content::ScriptComponent::Properties::id))

#define GET_OBJECT_COLOUR(id) (Colour((uint32)(int64)GET_SCRIPT_PROPERTY(id)))


Array<Identifier> ScriptComponentPropertyTypeSelector::toggleProperties = Array<Identifier>();
Array<Identifier> ScriptComponentPropertyTypeSelector::sliderProperties = Array<Identifier>();
Array<Identifier> ScriptComponentPropertyTypeSelector::colourProperties = Array<Identifier>();
Array<Identifier> ScriptComponentPropertyTypeSelector::choiceProperties = Array<Identifier>();
Array<Identifier> ScriptComponentPropertyTypeSelector::multilineProperties = Array<Identifier>();
Array<Identifier> ScriptComponentPropertyTypeSelector::fileProperties = Array<Identifier>();
Array<ScriptComponentPropertyTypeSelector::SliderRange> ScriptComponentPropertyTypeSelector::sliderRanges = Array<ScriptComponentPropertyTypeSelector::SliderRange>();


void ScriptCreatedComponentWrapper::changed(var newValue)
{
	getScriptComponent()->value = newValue;

	dynamic_cast<ProcessorWithScriptingContent*>(getProcessor())->controlCallback(getScriptComponent(), newValue);
}

Processor * ScriptCreatedComponentWrapper::getProcessor()
{
	return contentComponent->p.get();
}

ScriptingApi::Content * ScriptCreatedComponentWrapper::getContent()
{
	return contentComponent->contentData;
}

ScriptCreatedComponentWrappers::SliderWrapper::SliderWrapper(ScriptContentComponent *content, ScriptingApi::Content::ScriptSlider *sc, int index) :
ScriptCreatedComponentWrapper(content, index)
{
	HiSlider *s;

	s = new HiSlider(sc->name.toString());

	
	

	s->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
	s->setTextBoxStyle(Slider::TextBoxRight, true, 80, 20);
	s->addListener(this);
	s->setValue(sc->value, dontSendNotification);

	s->setup(getProcessor(), getIndex(), sc->name.toString());

	component = s;

	updateFilmstrip();

	double min = GET_SCRIPT_PROPERTY(min);
	double max = GET_SCRIPT_PROPERTY(max);
	double step = sc->getScriptObjectProperty(ScriptingApi::Content::ScriptSlider::stepSize);
	double middle = sc->getScriptObjectProperty(ScriptingApi::Content::ScriptSlider::middlePosition);



	s->setMode(sc->m, min, max, middle, step);

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

		

		if (thisFilmStrip != filmStripName || thisStrips != numStrips)
		{
			filmStripName = thisFilmStrip;
			numStrips = thisStrips;

			FilmstripLookAndFeel *fslaf = new FilmstripLookAndFeel();

			fslaf->setFilmstripImage(sc->getImage(),
				sc->getScriptObjectProperty(ScriptingApi::Content::ScriptSlider::numStrips),
				sc->getScriptObjectProperty(ScriptingApi::Content::ScriptSlider::isVertical));

			s->setTextBoxStyle(Slider::NoTextBox, true, 0, 0);

			s->setLookAndFeelOwned(fslaf);

			s->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
		}
	}
}



void ScriptCreatedComponentWrappers::SliderWrapper::updateComponent()
{
	HiSlider *s = dynamic_cast<HiSlider*>(getComponent());

	jassert(s != nullptr);

	s->setUseUndoManagerForEvents(GET_SCRIPT_PROPERTY(useUndoManager));

	s->setTooltip(GET_SCRIPT_PROPERTY(tooltip));
	s->setName(GET_SCRIPT_PROPERTY(text));
	s->enableMacroControlledComponent(GET_SCRIPT_PROPERTY(enabled));

	ScriptingApi::Content::ScriptSlider* sc = dynamic_cast<ScriptingApi::Content::ScriptSlider*>(getScriptComponent());

	double sensitivityScaler = sc->getScriptObjectProperty(ScriptingApi::Content::ScriptSlider::mouseSensitivity);

	if (sensitivityScaler != 1.0)
	{
		double sensitivity = jmax<double>(1.0, 250.0 * sensitivityScaler);
		s->setMouseDragSensitivity((int)sensitivity);
	}

	String currentFilmStrip = sc->getScriptObjectProperty(ScriptingApi::Content::ScriptSlider::Properties::filmstripImage);
	
	updateFilmstrip();
	
	const double min = sc->getScriptObjectProperty(ScriptingApi::Content::ScriptComponent::min);
	const double max = sc->getScriptObjectProperty(ScriptingApi::Content::ScriptComponent::max);
	const double stepsize = sc->getScriptObjectProperty(ScriptingApi::Content::ScriptSlider::stepSize);
	const double middlePos = sc->getScriptObjectProperty(ScriptingApi::Content::ScriptSlider::middlePosition);
	const double defaultValue = sc->getScriptObjectProperty(ScriptingApi::Content::ScriptSlider::defaultValue);
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
		if (middlePos != -1.0) s->setSkewFactorFromMidPoint(middlePos);
		if (sc->m == HiSlider::Mode::Linear) s->setTextValueSuffix(suffix);
	}

	if (defaultValue >= min && defaultValue <= max)
	{
		s->setDoubleClickReturnValue(true, defaultValue);
	}

	s->setValue(sc->value, dontSendNotification);

	const bool usesFilmStrip = sc->getImage().isValid();

	if (usesFilmStrip)
	{
		FilmstripLookAndFeel *fslaf = dynamic_cast<FilmstripLookAndFeel*>(&s->getLookAndFeel());

		

		if (fslaf != nullptr)
		{
			fslaf->setScaleFactor(sc->getScriptObjectProperty(ScriptingApi::Content::ScriptSlider::scaleFactor));
		}
	}
	else
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

		s->setColour(Slider::backgroundColourId, GET_OBJECT_COLOUR(bgColour));
		s->setColour(Slider::thumbColourId, GET_OBJECT_COLOUR(itemColour));
		s->setColour(Slider::trackColourId, GET_OBJECT_COLOUR(itemColour2));

		s->setColour(MacroControlledObject::HiBackgroundColours::outlineBgColour, GET_OBJECT_COLOUR(bgColour));
		s->setColour(MacroControlledObject::HiBackgroundColours::upperBgColour, GET_OBJECT_COLOUR(itemColour));
		s->setColour(MacroControlledObject::HiBackgroundColours::lowerBgColour, GET_OBJECT_COLOUR(itemColour2));

		s->setColour(Slider::textBoxTextColourId, GET_OBJECT_COLOUR(textColour));
	}

	

	s->repaint();
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
		/*changed(s->getValue());*/ // setInternalAttribute handles this for HiWidgets.
	}
}

void ScriptCreatedComponentWrappers::SliderWrapper::sliderDragStarted(Slider* s)
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

	if (d == No)
		return;

	if (auto c = getComponent())
	{
		auto parentTile = c->findParentComponentOfClass<FloatingTile>(); // always in a tile...

		if (parentTile == nullptr)
		{
			// Ouch...
			jassertfalse;
			return;
		}

		parentTile->addAndMakeVisible(currentPopup = new ValuePopup(c));

		currentPopup->setAlwaysOnTop(true);

		currentPopup->itemColour = GET_OBJECT_COLOUR(itemColour);
		currentPopup->itemColour2 = GET_OBJECT_COLOUR(itemColour2);
		currentPopup->textColour = GET_OBJECT_COLOUR(textColour);
		currentPopup->bgColour = GET_OBJECT_COLOUR(bgColour);

		auto l = parentTile->getLocalArea(c, c->getLocalBounds());

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

		currentPopup->setTopLeftPosition(x, y);
	}
}

void ScriptCreatedComponentWrappers::SliderWrapper::sliderDragEnded(Slider* s)
{
	if (auto c = getComponent())
	{
		Desktop::getInstance().getAnimator().fadeOut(currentPopup, 200);

		auto parentTile = c->findParentComponentOfClass<FloatingTile>(); // always in a tile...

		if (parentTile == nullptr)
		{
			// Ouch...
			jassertfalse;
			return;
		}

		parentTile->removeChildComponent(currentPopup);
		currentPopup = nullptr;
	}
}

ScriptCreatedComponentWrappers::ComboBoxWrapper::ComboBoxWrapper(ScriptContentComponent *content, ScriptingApi::Content::ScriptComboBox *scriptComboBox, int index) :
ScriptCreatedComponentWrapper(content, index)
{
	HiComboBox *cb = new HiComboBox(scriptComboBox->name.toString());

	cb->addItemList(scriptComboBox->getItemList(), 1);

	cb->setup(getProcessor(), getIndex(), scriptComboBox->name.toString());

	cb->updateValue();

	cb->addListener(this);

	component = cb;
}

void ScriptCreatedComponentWrappers::ComboBoxWrapper::updateComponent()
{
	HiComboBox *cb = dynamic_cast<HiComboBox*>(component.get());

	cb->setUseUndoManagerForEvents(GET_SCRIPT_PROPERTY(useUndoManager));

	cb->setTooltip(GET_SCRIPT_PROPERTY(tooltip));

	cb->enableMacroControlledComponent(GET_SCRIPT_PROPERTY(enabled));

	cb->setTextWhenNothingSelected(GET_SCRIPT_PROPERTY(text));

    cb->setColour(MacroControlledObject::HiBackgroundColours::outlineBgColour, GET_OBJECT_COLOUR(bgColour));
    cb->setColour(MacroControlledObject::HiBackgroundColours::upperBgColour, GET_OBJECT_COLOUR(itemColour));
    cb->setColour(MacroControlledObject::HiBackgroundColours::lowerBgColour, GET_OBJECT_COLOUR(itemColour2));
    cb->setColour(MacroControlledObject::HiBackgroundColours::textColour, GET_OBJECT_COLOUR(textColour));
    
	cb->clear(dontSendNotification);

	cb->addItemList(dynamic_cast<ScriptingApi::Content::ScriptComboBox*>(getScriptComponent())->getItemList(), 1);
}

ScriptCreatedComponentWrappers::ButtonWrapper::ButtonWrapper(ScriptContentComponent *content, ScriptingApi::Content::ScriptButton *sb, int index) :
ScriptCreatedComponentWrapper(content, index)
{
	HiToggleButton *b = new HiToggleButton(sb->name.toString());
	b->setButtonText(sb->name.toString());
	b->addListener(this);
	b->setToggleState((bool)sb->value, dontSendNotification);

	b->setup(getProcessor(), getIndex(), sb->name.toString());

	if (sb->getPopupData().isObject())
	{
        auto r = sb->getPopupPosition();
		b->setPopupData(sb->getPopupData(), r);
	}

	b->updateValue();

	if (sb->getImage().isValid())
	{
		FilmstripLookAndFeel *fslaf = new FilmstripLookAndFeel();

		fslaf->setFilmstripImage(sb->getImage(),
			sb->getScriptObjectProperty(ScriptingApi::Content::ScriptButton::numStrips).toString().getIntValue(),
			sb->getScriptObjectProperty(ScriptingApi::Content::ScriptButton::isVertical));

		b->setLookAndFeelOwned(fslaf);
	}

	component = b;
}


void ScriptCreatedComponentWrappers::ButtonWrapper::updateComponent()
{
	HiToggleButton *b = dynamic_cast<HiToggleButton*>(component.get());

	b->enableMacroControlledComponent(GET_SCRIPT_PROPERTY(enabled));

	b->setUseUndoManagerForEvents(GET_SCRIPT_PROPERTY(useUndoManager));
    
    FilmstripLookAndFeel *fslaf = dynamic_cast<FilmstripLookAndFeel*>(&b->getLookAndFeel());
    
    if(fslaf != nullptr)
    {
        fslaf->setScaleFactor(getScriptComponent()->getScriptObjectProperty(ScriptingApi::Content::ScriptButton::scaleFactor));
    }

	b->setIsMomentary(getScriptComponent()->getScriptObjectProperty(ScriptingApi::Content::ScriptButton::isMomentary));

    b->setColour(MacroControlledObject::HiBackgroundColours::outlineBgColour, GET_OBJECT_COLOUR(bgColour));
    b->setColour(MacroControlledObject::HiBackgroundColours::upperBgColour, GET_OBJECT_COLOUR(itemColour));
    b->setColour(MacroControlledObject::HiBackgroundColours::lowerBgColour, GET_OBJECT_COLOUR(itemColour2));
    
	b->setTooltip(GET_SCRIPT_PROPERTY(tooltip));
	b->setButtonText(GET_SCRIPT_PROPERTY(text));
	b->setToggleState((bool)getScriptComponent()->value, dontSendNotification);
	b->setRadioGroupId(getScriptComponent()->getScriptObjectProperty(ScriptingApi::Content::ScriptButton::radioGroup));
}


ScriptCreatedComponentWrappers::LabelWrapper::LabelWrapper(ScriptContentComponent *content, ScriptingApi::Content::ScriptLabel *sl, int index):
ScriptCreatedComponentWrapper(content, index)
{
	Label *l = new MultilineLabel(sl->name.toString());
	l->setText(sl->value.toString(), dontSendNotification);
	l->setFont(GLOBAL_FONT());

	bool editable = sl->getScriptObjectProperty(ScriptingApi::Content::ScriptLabel::Editable);
	l->setInterceptsMouseClicks(editable, editable);
	l->setEditable(editable);

	

	l->addListener(this);

	component = l;
}

void ScriptCreatedComponentWrappers::LabelWrapper::updateComponent()
{
	MultilineLabel *l = dynamic_cast<MultilineLabel*>(component.get());
	
	ScriptingApi::Content::ScriptLabel *sl = dynamic_cast<ScriptingApi::Content::ScriptLabel*>(getScriptComponent());

	l->setTooltip(GET_SCRIPT_PROPERTY(tooltip));

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
		ScriptContentComponent* content = l->findParentComponentOfClass<ScriptContentComponent>();
		const juce::Typeface::Ptr typeface = dynamic_cast<const Processor*>(content->getScriptProcessor())->getMainController()->getFont(fontName);

		

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

	l->setJustificationType(sl->getJustification());
	l->setColour(Label::ColourIds::textColourId, GET_OBJECT_COLOUR(textColour));
	l->setColour(Label::ColourIds::backgroundColourId, GET_OBJECT_COLOUR(bgColour));
	l->setColour(Label::ColourIds::backgroundWhenEditingColourId, GET_OBJECT_COLOUR(bgColour));
	l->setColour(Label::ColourIds::textWhenEditingColourId, GET_OBJECT_COLOUR(textColour));
	l->setColour(TextEditor::ColourIds::highlightColourId, GET_OBJECT_COLOUR(textColour));
	l->setColour(TextEditor::ColourIds::highlightedTextColourId, GET_OBJECT_COLOUR(textColour).contrasting());
	l->setColour(TextEditor::ColourIds::focusedOutlineColourId, GET_OBJECT_COLOUR(itemColour));
	l->setColour(CaretComponent::ColourIds::caretColourId, GET_OBJECT_COLOUR(textColour));
	l->setColour(Label::ColourIds::outlineColourId, GET_OBJECT_COLOUR(itemColour));
	
	bool editable = sl->getScriptObjectProperty(ScriptingApi::Content::ScriptLabel::Editable);
	bool multiline = sl->getScriptObjectProperty(ScriptingApi::Content::ScriptLabel::Multiline);
	
	l->setText(getScriptComponent()->getValue().toString(), dontSendNotification);

	l->setInterceptsMouseClicks(editable, editable);
	l->setEditable(editable);
	l->setMultiline(multiline);
}

void ScriptCreatedComponentWrappers::LabelWrapper::labelTextChanged(Label *l)
{
	auto sc = getScriptComponent();

	sc->setValue(l->getText());

	dynamic_cast<ProcessorWithScriptingContent*>(getProcessor())->controlCallback(getScriptComponent(), sc->getValue());
}

ScriptCreatedComponentWrappers::TableWrapper::TableWrapper(ScriptContentComponent *content, ScriptingApi::Content::ScriptTable *table, int index) :
ScriptCreatedComponentWrapper(content, index)
{
	TableEditor *t = new TableEditor(table->getTable());

	t->setName(table->name.toString());

	component = t;
	
}

void ScriptCreatedComponentWrappers::TableWrapper::updateComponent()
{
	TableEditor *t = dynamic_cast<TableEditor*>(component.get());

	t->setTooltip(GET_SCRIPT_PROPERTY(tooltip));

	ScriptingApi::Content::ScriptTable *st = dynamic_cast<ScriptingApi::Content::ScriptTable*>(getScriptComponent());
	LookupTableProcessor *ltp = st->getTableProcessor();
	t->connectToLookupTableProcessor(dynamic_cast<Processor*>(ltp));

	Table *oldTable = t->getEditedTable();
	Table *newTable = st->getTable();

	if (oldTable != newTable) t->setEditedTable(newTable);
}

ScriptCreatedComponentWrappers::ModulatorMeterWrapper::ModulatorMeterWrapper(ScriptContentComponent *content, ScriptingApi::Content::ModulatorMeter *m, int index) :
ScriptCreatedComponentWrapper(content, index)
{
	ModulatorPeakMeter *pm = new ModulatorPeakMeter(m->targetMod);

	pm->setName(m->name.toString());

	pm->setColour(Colour(0x22ffffff));

	component = pm;

}

void ScriptCreatedComponentWrappers::ModulatorMeterWrapper::updateComponent()
{
	dynamic_cast<SettableTooltipClient*>(component.get())->setTooltip(GET_SCRIPT_PROPERTY(tooltip));
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
				ScriptContentComponent* content = listBox->findParentComponentOfClass<ScriptContentComponent>();
				const juce::Typeface::Ptr typeface = dynamic_cast<const Processor*>(content->getScriptProcessor())->getMainController()->getFont(fontName);



				if (typeface != nullptr)
				{
					f = Font(typeface).withHeight(fontSize);
				}
				else
				{
					f = Font(fontName, fontStyle, fontSize);
				}
			}

			auto itemColour1 = GET_OBJECT_COLOUR(itemColour);
			auto itemColour2 = GET_OBJECT_COLOUR(itemColour2);
			auto bgColour = GET_OBJECT_COLOUR(bgColour);
			auto textColour = GET_OBJECT_COLOUR(textColour);

			model->font = f;
			model->itemColour1 = itemColour1;
			model->itemColour2 = itemColour2;
			model->bgColour = bgColour;
			model->textColour = textColour;
			
			listBox->setRowHeight((int)f.getHeight() + 15);

			listBox->getViewport()->setColour(ScrollBar::ColourIds::thumbColourId, itemColour1);

			listBox->setColour(ListBox::ColourIds::backgroundColourId, bgColour);
			listBox->setColour(ListBox::ColourIds::outlineColourId, itemColour2);

			model->justification = vpc->getJustification();

			if (model->shouldUpdate(vpc->getItemList()))
			{
				model->setItems(vpc->getItemList());
			}

			int viewportIndex = vpc->getValue();

			listBox->selectRow(viewportIndex);

			listBox->updateContent();
		}
	}
	else
	{
		auto vp = dynamic_cast<Viewport*>(component.get());

		vp->setScrollBarThickness(vpc->getScriptObjectProperty(ScriptingApi::Content::ScriptedViewport::Properties::scrollbarThickness));
		vp->setColour(ScrollBar::ColourIds::thumbColourId, GET_OBJECT_COLOUR(itemColour));
	}
}

ScriptCreatedComponentWrappers::PlotterWrapper::PlotterWrapper(ScriptContentComponent *content, ScriptingApi::Content::ScriptedPlotter *p, int index):
ScriptCreatedComponentWrapper(content, index)
{
	Plotter *pl = new Plotter();

	pl->setName(p->name.toString());

	for (int i = 0; i < p->mods.size(); i++)
	{
		pl->addPlottedModulator(p->mods[i]);
	}

	component = pl;
}

void ScriptCreatedComponentWrappers::PlotterWrapper::updateComponent()
{
	dynamic_cast<SettableTooltipClient*>(component.get())->setTooltip(GET_SCRIPT_PROPERTY(tooltip));

	component.get()->setColour(Plotter::pathColour, GET_OBJECT_COLOUR(itemColour));
	component.get()->setColour(Plotter::pathColour2, GET_OBJECT_COLOUR(itemColour2));
	component.get()->setColour(Plotter::backgroundColour, GET_OBJECT_COLOUR(bgColour));
}

ScriptCreatedComponentWrappers::ImageWrapper::ImageWrapper(ScriptContentComponent *content, ScriptingApi::Content::ScriptImage *img, int index):
ScriptCreatedComponentWrapper(content, index)
{
	ImageComponentWithMouseCallback *i = new ImageComponentWithMouseCallback();

	i->setName(img->name.toString());

	//i->setInterceptsMouseClicks(false, false);

    
    i->addMouseCallbackListener(this);
    
	component = i;
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
		ic->setImage(ImagePool::getEmptyImage(ic->getWidth(), ic->getHeight()));
	}

	contentComponent->repaint();
}

void ScriptCreatedComponentWrappers::ImageWrapper::mouseCallback(const var &mouseInformation)
{
    changed(mouseInformation);
}

ScriptCreatedComponentWrappers::PanelWrapper::PanelWrapper(ScriptContentComponent *content, ScriptingApi::Content::ScriptPanel *panel, int index) :
ScriptCreatedComponentWrapper(content, index)
{
	BorderPanel *bp = new BorderPanel();

	bp->setName(panel->name.toString());

    panel->getRepaintNotifier()->addChangeListener(bp);
    
	bp->addMouseCallbackListener(this);
	bp->setDraggingEnabled(panel->getScriptObjectProperty(ScriptingApi::Content::ScriptPanel::allowDragging));
	bp->setDragBounds(panel->getDragBounds(), this);

    bp->setOpaque(panel->getScriptObjectProperty(ScriptingApi::Content::ScriptPanel::opaque));
    
	bp->setup(getProcessor(), getIndex(), panel->name.toString());

	component = bp;
}

void ScriptCreatedComponentWrappers::PanelWrapper::updateComponent()
{
	BorderPanel *bpc = dynamic_cast<BorderPanel*>(component.get());
	auto sc = dynamic_cast<ScriptingApi::Content::ScriptPanel*>(getScriptComponent());

	bpc->c1 = GET_OBJECT_COLOUR(itemColour);
	bpc->c2 = GET_OBJECT_COLOUR(itemColour2);
	bpc->borderColour = GET_OBJECT_COLOUR(textColour);
	bpc->borderRadius = getScriptComponent()->getScriptObjectProperty(ScriptingApi::Content::ScriptPanel::borderRadius);
	bpc->borderSize = getScriptComponent()->getScriptObjectProperty(ScriptingApi::Content::ScriptPanel::borderSize);
	bpc->image = dynamic_cast<ScriptingApi::Content::ScriptPanel*>(getScriptComponent())->getImage();
	
	bpc->isUsingCustomImage = sc->isUsingCustomPaintRoutine() || sc->isUsingClippedFixedImage();
	bpc->setPopupMenuItems(sc->getItemList());
	bpc->setOpaque(sc->getScriptObjectProperty(ScriptingApi::Content::ScriptPanel::opaque));
	bpc->setActivePopupItem((int)getScriptComponent()->getScriptObjectProperty(ScriptingApi::Content::ScriptPanel::selectedPopupIndex));
	bpc->setUseRightClickForPopup(sc->getScriptObjectProperty(ScriptingApi::Content::ScriptPanel::PopupOnRightClick));
	bpc->alignPopup(sc->getScriptObjectProperty(ScriptingApi::Content::ScriptPanel::popupMenuAlign));

	bpc->setTooltip(GET_SCRIPT_PROPERTY(tooltip));

	bpc->setMidiLearnEnabled(sc->getScriptObjectProperty(ScriptingApi::Content::ScriptPanel::enableMidiLearn));

	bpc->setTouchEnabled(sc->getScriptObjectProperty(ScriptingApi::Content::ScriptPanel::holdIsRightClick));

	bpc->setJSONPopupData(sc->getJSONPopupData(), sc->getPopupSize());

	const double min = GET_SCRIPT_PROPERTY(min);
	const double max = GET_SCRIPT_PROPERTY(max);
	const double stepSize = getScriptComponent()->getScriptObjectProperty(ScriptingApi::Content::ScriptPanel::stepSize);

	NormalisableRange<double> r(min, max);
	r.interval = stepSize;

	bpc->setRange(r);

	bpc->setInterceptsMouseClicks(sc->getScriptObjectProperty(ScriptingApi::Content::ScriptPanel::enabled), true);

	bpc->repaint();

	bpc->setAllowCallback(getScriptComponent()->getScriptObjectProperty(ScriptingApi::Content::ScriptPanel::allowCallbacks).toString());

	

	contentComponent->repaint();
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
	BorderPanel *bpc = dynamic_cast<BorderPanel*>(component.get());

    auto panel = dynamic_cast<ScriptingApi::Content::ScriptPanel*>(getScriptComponent());
    
	if (panel != nullptr)
	{
		panel->getRepaintNotifier()->removeChangeListener(bpc);
	}

	bpc->removeCallbackListener(this);
	
}

ScriptCreatedComponentWrappers::SliderPackWrapper::SliderPackWrapper(ScriptContentComponent *content, ScriptingApi::Content::ScriptSliderPack *pack, int index) :
ScriptCreatedComponentWrapper(content, index)
{
	SliderPack *sp = new SliderPack(pack->getSliderPackData());

	sp->addListener(this);
	sp->setName(pack->name.toString());

	component = sp;
}

void ScriptCreatedComponentWrappers::SliderPackWrapper::updateComponent()
{
	SliderPack *sp = dynamic_cast<SliderPack*>(component.get());
	ScriptingApi::Content::ScriptSliderPack *ssp = dynamic_cast<ScriptingApi::Content::ScriptSliderPack*>(getScriptComponent());

	jassert(ssp->getSliderPackData() == sp->getData());

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
		sp->setColour(Slider::thumbColourId, GET_OBJECT_COLOUR(itemColour));
		sp->setColour(Slider::ColourIds::textBoxOutlineColourId, GET_OBJECT_COLOUR(itemColour2));
		sp->setColour(Slider::backgroundColourId, GET_OBJECT_COLOUR(bgColour));
		sp->setColour(Slider::trackColourId, GET_OBJECT_COLOUR(textColour));

		sp->updateSliders();

		sp->repaint();
	}
}

ScriptCreatedComponentWrappers::AudioWaveformWrapper::AudioWaveformWrapper(ScriptContentComponent *content, ScriptingApi::Content::ScriptAudioWaveform *form, int index):
ScriptCreatedComponentWrapper(content, index)
{
	// Ugly as fuck
	AudioThumbnailCache* cache = const_cast<Processor*>(dynamic_cast<const Processor*>(content->getScriptProcessor()))->getMainController()->getSampleManager().getAudioSampleBufferPool()->getCache();

	AudioSampleBufferComponent *asb = new AudioSampleBufferComponent(*cache);

	asb->setName(form->name.toString());
	asb->setOpaque(false);

	AudioSampleProcessor *asp = form->getAudioProcessor();

	if (asp != nullptr)
	{
		asb->setAudioSampleBuffer(asp->getBuffer(), asp->getFileName());

		asb->setRange(asp->getRange());

		asb->addAreaListener(this);
		asb->addChangeListener(asp);
	}

	component = asb;
}



ScriptCreatedComponentWrappers::AudioWaveformWrapper::~AudioWaveformWrapper()
{
	if (auto form = dynamic_cast<ScriptingApi::Content::ScriptAudioWaveform*>(getScriptComponent()))
	{
		if (auto asp = form->getAudioProcessor())
		{
			if (auto asb = dynamic_cast<AudioSampleBufferComponent*>(getComponent()))
			{
				asb->removeChangeListener(asp);
			}
		}
	}
}

void ScriptCreatedComponentWrappers::AudioWaveformWrapper::updateComponent()
{
	ScriptingApi::Content::ScriptAudioWaveform *form = dynamic_cast<ScriptingApi::Content::ScriptAudioWaveform*>(getScriptComponent());

	AudioSampleProcessor *asp = form->getAudioProcessor();

	AudioSampleBufferComponent *asb = dynamic_cast<AudioSampleBufferComponent*>(component.get());

	if (asp != nullptr)

	{
		dynamic_cast<Processor*>(form->getAudioProcessor())->sendSynchronousChangeMessage();

		if (asb != nullptr)
		{
			asb->setAudioSampleBuffer(asp->getBuffer(), asp->getFileName());
			asb->setRange(asp->getRange());
		}
	}
}

void ScriptCreatedComponentWrappers::AudioWaveformWrapper::rangeChanged(AudioDisplayComponent *broadcaster, int changedArea)
{

	ScriptingApi::Content::ScriptAudioWaveform * form = dynamic_cast<ScriptingApi::Content::ScriptAudioWaveform *>(getScriptComponent());

	if (form != nullptr)
	{
		Range<int> newRange = broadcaster->getSampleArea(changedArea)->getSampleRange();

		form->getAudioProcessor()->setLoadedFile(dynamic_cast<AudioSampleBufferComponent*>(broadcaster)->getCurrentlyLoadedFileName());
		form->getAudioProcessor()->setRange(newRange);
	}
}



ScriptCreatedComponentWrappers::FloatingTileWrapper::FloatingTileWrapper(ScriptContentComponent *content, ScriptingApi::Content::ScriptFloatingTile *floatingTile, int index):
	ScriptCreatedComponentWrapper(content, index)
{
	auto mc = const_cast<MainController*>(dynamic_cast<const Processor*>(content->getScriptProcessor())->getMainController());

	auto ft = new FloatingTile(mc, nullptr);

	ft->setName(floatingTile->name.toString());
	ft->setOpaque(false);

	const bool updateAfterInit = (bool)floatingTile->getScriptObjectProperty(ScriptingApi::Content::ScriptFloatingTile::Properties::updateAfterInit);

	if (!updateAfterInit)
	{
		ft->setContent(floatingTile->getContentData());
		ft->refreshRootLayout();
	}

	component = ft;
}



void ScriptCreatedComponentWrappers::FloatingTileWrapper::updateComponent()
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
			break;
		case ScriptedControlAudioParameter::Type::ComboBox:
			range.interval = 1.0f;
			itemList = dynamic_cast<ScriptingApi::Content::ScriptComboBox*>(c)->getItemList();
			break;
		case ScriptedControlAudioParameter::Type::Panel:
			range.interval = c->getScriptObjectProperty(ScriptingApi::Content::ScriptPanel::Properties::stepSize);
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

void ScriptedControlAudioParameter::setParameterNotifyingHost(int index, float newValue)
{
	ScopedValueSetter<bool> setter(dynamic_cast<MainController*>(parentProcessor)->getPluginParameterUpdateState(), false, true);
	parentProcessor->setParameterNotifyingHost(index, range.convertTo0to1(newValue));
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


} // namespace hise
