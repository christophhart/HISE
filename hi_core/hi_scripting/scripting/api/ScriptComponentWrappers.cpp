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
*   Commercial licences for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licencing:
*
*   http://www.hartinstruments.net/hise/
*
*   HISE is based on the JUCE library,
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

BorderPanel::BorderPanel() :
borderColour(Colours::black),
c1(Colours::white),
c2(Colours::white),
borderRadius(0.0f),
borderSize(1.0f),
allowCallback(false)
{

}



void BorderPanel::paint(Graphics &g)
{
	ColourGradient grad = ColourGradient(c1, 0.0f, 0.0f, c2, 0.0f, (float)getHeight(), false);

	Rectangle<float> fillR(borderSize, borderSize, getWidth() - 2 * borderSize, getHeight() - 2 * borderSize);

	fillR.expand(borderSize * 0.5f, borderSize * 0.5f);

	if (fillR.isEmpty() || fillR.getX() < 0 || fillR.getY() < 0) return;

	g.setGradientFill(grad);
	g.fillRoundedRectangle(fillR, borderRadius);



	g.setColour(borderColour);

	g.drawRoundedRectangle(fillR, borderRadius, borderSize);
}



void BorderPanel::sendMessage(const MouseEvent &event)
{
	if (!allowCallback) return;

	DynamicObject::Ptr obj = new DynamicObject();

	obj->setProperty(Identifier("x"), event.getPosition().getX());
	obj->setProperty(Identifier("y"), event.getPosition().getY());
	obj->setProperty(Identifier("clicked"), event.mouseWasClicked());
	obj->setProperty(Identifier("drag"), event.getDistanceFromDragStart() > 4);
    obj->setProperty(Identifier("dragX"), event.getDistanceFromDragStartX());
    obj->setProperty(Identifier("dragY"), event.getDistanceFromDragStartY());

    obj->setProperty(Identifier("mouseDownX"), event.getEventRelativeTo(getParentComponent()).getMouseDownX());
    obj->setProperty(Identifier("mouseDownY"), event.getEventRelativeTo(getParentComponent()).getMouseDownY());
    
	var clickInformation(obj);

	for (int i = 0; i < listenerList.size(); i++)
	{
		if (listenerList[i].get() != nullptr)
		{
			listenerList[i]->mouseCallback(clickInformation);
		}
	}
}



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

class MultilineLabel : public Label
{
public:

	MultilineLabel(const String &name) :
		Label(name),
		multiline(false)
	{};

	virtual ~MultilineLabel() {};

	void setMultiline(bool shouldBeMultiline)
	{
		multiline = shouldBeMultiline;
	};

protected:

	TextEditor *createEditorComponent() override
	{
		TextEditor *editor = Label::createEditorComponent();

		editor->setMultiLine(multiline, true);

		editor->setReturnKeyStartsNewLine(multiline);

		return editor;
	};

private:

	bool multiline;
};


void ScriptCreatedComponentWrapper::changed(var newValue)
{
	getScriptComponent()->value = newValue;

	dynamic_cast<ScriptBaseProcessor*>(getProcessor())->controlCallback(getScriptComponent(), newValue);
}

Processor * ScriptCreatedComponentWrapper::getProcessor()
{
	return contentComponent->processor;
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

	if (sc->getImage() != nullptr)
	{
		FilmstripLookAndFeel *fslaf = new FilmstripLookAndFeel();

		fslaf->setFilmstripImage(*sc->getImage(),
			sc->getScriptObjectProperty(ScriptingApi::Content::ScriptSlider::numStrips),
			sc->getScriptObjectProperty(ScriptingApi::Content::ScriptSlider::isVertical));

		s->setTextBoxStyle(Slider::NoTextBox, true, 0, 0);

		s->setLookAndFeelOwned(fslaf);
	}

	if (sc->m == HiSlider::Linear)
	{
		double min = GET_SCRIPT_PROPERTY(min);
		double max = GET_SCRIPT_PROPERTY(max);
		double step = sc->getScriptObjectProperty(ScriptingApi::Content::ScriptSlider::stepSize);
		double middle = sc->getScriptObjectProperty(ScriptingApi::Content::ScriptSlider::middlePosition);

		s->setMode(sc->m, min, max, middle);

		s->setRange(min, max, step);

		//if(sc->middlePosition != DBL_MAX) s->setSkewFactorFromMidPoint(sc->middlePosition);

	}
	else
	{
		s->setMode(sc->m);
	}

	s->updateValue();

	component = s;
}

void ScriptCreatedComponentWrappers::SliderWrapper::updateComponent()
{
	HiSlider *s = dynamic_cast<HiSlider*>(getComponent());

	jassert(s != nullptr);

	s->setTooltip(GET_SCRIPT_PROPERTY(tooltip));

	ScriptingApi::Content::ScriptSlider* sc = dynamic_cast<ScriptingApi::Content::ScriptSlider*>(getScriptComponent());

	s->setName(GET_SCRIPT_PROPERTY(text));

	s->setSliderStyle(sc->styleId);

	

	s->enableMacroControlledComponent(GET_SCRIPT_PROPERTY(enabled));

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

	cb->setTooltip(GET_SCRIPT_PROPERTY(tooltip));

	cb->enableMacroControlledComponent(GET_SCRIPT_PROPERTY(enabled));

	cb->setTextWhenNothingSelected(GET_SCRIPT_PROPERTY(text));

    cb->setColour(MacroControlledObject::HiBackgroundColours::outlineBgColour, GET_OBJECT_COLOUR(bgColour));
    cb->setColour(MacroControlledObject::HiBackgroundColours::upperBgColour, GET_OBJECT_COLOUR(itemColour));
    cb->setColour(MacroControlledObject::HiBackgroundColours::lowerBgColour, GET_OBJECT_COLOUR(itemColour2));
    
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

	b->updateValue();

	if (sb->getImage() != nullptr)
	{
		FilmstripLookAndFeel *fslaf = new FilmstripLookAndFeel();

		fslaf->setFilmstripImage(*sb->getImage(),
			2,
			sb->getScriptObjectProperty(ScriptingApi::Content::ScriptButton::isVertical));

		b->setLookAndFeelOwned(fslaf);
	}

	component = b;
}


void ScriptCreatedComponentWrappers::ButtonWrapper::updateComponent()
{
	HiToggleButton *b = dynamic_cast<HiToggleButton*>(component.get());

	b->enableMacroControlledComponent(GET_SCRIPT_PROPERTY(enabled));

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

	Font font(sl->getScriptObjectProperty(ScriptingApi::Content::ScriptLabel::FontName).toString(),
		sl->getScriptObjectProperty(ScriptingApi::Content::ScriptLabel::FontStyle).toString(),
		(float)sl->getScriptObjectProperty(ScriptingApi::Content::ScriptLabel::FontSize));

	l->setFont(font);
	l->setJustificationType(sl->getJustification());
	l->setColour(Label::ColourIds::textColourId, GET_OBJECT_COLOUR(textColour));
	l->setColour(Label::ColourIds::backgroundColourId, GET_OBJECT_COLOUR(bgColour));
	l->setColour(Label::ColourIds::backgroundWhenEditingColourId, GET_OBJECT_COLOUR(bgColour).contrasting());
	l->setColour(Label::ColourIds::textWhenEditingColourId, GET_OBJECT_COLOUR(textColour).contrasting());
	l->setColour(Label::ColourIds::outlineColourId, GET_OBJECT_COLOUR(itemColour));

	bool editable = sl->getScriptObjectProperty(ScriptingApi::Content::ScriptLabel::Editable);
	bool multiline = sl->getScriptObjectProperty(ScriptingApi::Content::ScriptLabel::Multiline);
	const String labelText = GET_SCRIPT_PROPERTY(text);

	l->setText(labelText.isEmpty() ? getScriptComponent()->value.toString() : labelText, dontSendNotification);

	l->setInterceptsMouseClicks(editable, editable);
	l->setEditable(editable);
	l->setMultiline(multiline);
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

ScriptCreatedComponentWrappers::PlotterWrapper::PlotterWrapper(ScriptContentComponent *content, ScriptingApi::Content::ScriptedPlotter *p, int index):
ScriptCreatedComponentWrapper(content, index)
{
	Plotter *pl = new Plotter(nullptr);

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
	ImageComponent *i = new ImageComponent();

	i->setName(img->name.toString());

	i->setInterceptsMouseClicks(false, false);

	component = i;
}

void ScriptCreatedComponentWrappers::ImageWrapper::updateComponent()
{
	ImageComponent *ic = dynamic_cast<ImageComponent*>(component.get());
	ScriptingApi::Content::ScriptImage *si = dynamic_cast<ScriptingApi::Content::ScriptImage*>(getScriptComponent());

	if (si->getImage() != nullptr)
	{
		ic->setImage(*si->getImage());
		ic->setAlpha(si->getScriptObjectProperty(ScriptingApi::Content::ScriptImage::Alpha));
	}
	else
	{
		ic->setBounds(si->getPosition());
		ic->setImage(ImagePool::getEmptyImage(ic->getWidth(), ic->getHeight()));
	}

	contentComponent->repaint();
}

ScriptCreatedComponentWrappers::PanelWrapper::PanelWrapper(ScriptContentComponent *content, ScriptingApi::Content::ScriptPanel *panel, int index) :
ScriptCreatedComponentWrapper(content, index)
{
	BorderPanel *bp = new BorderPanel();

	bp->setName(panel->name.toString());

	bp->addMouseCallbackListener(this);

	component = bp;
}

void ScriptCreatedComponentWrappers::PanelWrapper::updateComponent()
{
	BorderPanel *bpc = dynamic_cast<BorderPanel*>(component.get());

	bpc->c1 = GET_OBJECT_COLOUR(itemColour);
	bpc->c2 = GET_OBJECT_COLOUR(itemColour2);
	bpc->borderColour = GET_OBJECT_COLOUR(textColour);
	bpc->borderRadius = getScriptComponent()->getScriptObjectProperty(ScriptingApi::Content::ScriptPanel::borderRadius);
	bpc->borderSize = getScriptComponent()->getScriptObjectProperty(ScriptingApi::Content::ScriptPanel::borderSize);

	bpc->setAllowCallback(getScriptComponent()->getScriptObjectProperty(ScriptingApi::Content::ScriptPanel::allowCallbacks));

	

	contentComponent->repaint();
}

void ScriptCreatedComponentWrappers::PanelWrapper::mouseCallback(const var &mouseInformation)
{
	changed(mouseInformation);
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

	if (sp->getNumSliders() != ssp->getSliderPackData()->getNumSliders())
	{
		sp->setNumSliders(ssp->getNumSliders());
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
	AudioSampleBufferComponent *asb = new AudioSampleBufferComponent(*const_cast<ScriptProcessor*>(content->getScriptProcessor())->getMainController()->getSampleManager().getAudioSampleBufferPool()->getCache());

	asb->setName(form->name.toString());

	AudioSampleProcessor *asp = form->getProcessor();

	if (asp != nullptr)
	{
		asb->setAudioSampleBuffer(asp->getBuffer(), asp->getFileName());

		asb->addAreaListener(this);
		asb->addChangeListener(asp);
	}

	component = asb;
}

void ScriptCreatedComponentWrappers::AudioWaveformWrapper::updateComponent()
{
	ScriptingApi::Content::ScriptAudioWaveform *form = dynamic_cast<ScriptingApi::Content::ScriptAudioWaveform*>(getScriptComponent());

	AudioSampleProcessor *asp = form->getProcessor();

	if (asp != nullptr)

	{
		dynamic_cast<Processor*>(form->getProcessor())->sendSynchronousChangeMessage();

		//asb->setAudioSampleBuffer(asp->getBuffer(), asp->getFileName());

		//asb->addAreaListener(this);
		//asb->addChangeListener(asp);
	}
}

void ScriptCreatedComponentWrappers::AudioWaveformWrapper::rangeChanged(AudioDisplayComponent *broadcaster, int changedArea)
{

	ScriptingApi::Content::ScriptAudioWaveform * form = dynamic_cast<ScriptingApi::Content::ScriptAudioWaveform *>(getScriptComponent());

	if (form != nullptr)
	{
		Range<int> newRange = broadcaster->getSampleArea(changedArea)->getSampleRange();

		form->getProcessor()->setLoadedFile(dynamic_cast<AudioSampleBufferComponent*>(broadcaster)->getCurrentlyLoadedFileName());
		form->getProcessor()->setRange(newRange);
	}
	

}

