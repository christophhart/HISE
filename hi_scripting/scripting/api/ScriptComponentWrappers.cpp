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

	s->updateValue(dontSendNotification);

	component = s;
}

void ScriptCreatedComponentWrappers::SliderWrapper::updateComponent()
{
	HiSlider *s = dynamic_cast<HiSlider*>(getComponent());

	jassert(s != nullptr);

    FilmstripLookAndFeel *fslaf = dynamic_cast<FilmstripLookAndFeel*>(&s->getLookAndFeel());

    ScriptingApi::Content::ScriptSlider* sc = dynamic_cast<ScriptingApi::Content::ScriptSlider*>(getScriptComponent());
    
    if(fslaf != nullptr)
    {
        fslaf->setScaleFactor(sc->getScriptObjectProperty(ScriptingApi::Content::ScriptSlider::scaleFactor));
    }
    
	s->setTooltip(GET_SCRIPT_PROPERTY(tooltip));

	s->setName(GET_SCRIPT_PROPERTY(text));

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

	double sensitivityScaler = sc->getScriptObjectProperty(ScriptingApi::Content::ScriptSlider::mouseSensitivity);
	
	if (sensitivityScaler != 1.0)
	{
		double sensitivity = jmax<double>(1.0, 250.0 * sensitivityScaler);
		s->setMouseDragSensitivity((int)sensitivity);
	}

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

	s->setValue(sc->value, dontSendNotification);

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

    
    FilmstripLookAndFeel *fslaf = dynamic_cast<FilmstripLookAndFeel*>(&b->getLookAndFeel());
    
    if(fslaf != nullptr)
    {
        fslaf->setScaleFactor(getScriptComponent()->getScriptObjectProperty(ScriptingApi::Content::ScriptButton::scaleFactor));
    }

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

	if (si->getImage() != nullptr)
	{
		const StringArray sa = si->getItemList();

		ic->setAllowCallback(si->getScriptObjectProperty(ScriptingApi::Content::ScriptImage::AllowCallbacks).toString());

		const bool allowCallbacks = ic->getCallbackLevel() != MouseCallbackComponent::CallbackLevel::NoCallbacks;
		const bool showPopupMenu = sa.size() != 0;

        ic->setInterceptsMouseClicks(allowCallbacks || showPopupMenu, false);
        
		ic->setPopupMenuItems(si->getItemList());
		ic->setUseRightClickForPopup(si->getScriptObjectProperty(ScriptingApi::Content::ScriptImage::PopupOnRightClick));

        ic->setBounds(si->getPosition());
		ic->setImage(*si->getImage());
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

	bp->addMouseCallbackListener(this);
	bp->setDraggingEnabled(panel->getScriptObjectProperty(ScriptingApi::Content::ScriptPanel::allowDragging));
	bp->setDragBounds(panel->getDragBounds(), this);

    bp->setOpaque(panel->getScriptObjectProperty(ScriptingApi::Content::ScriptPanel::opaque));
    
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
	bpc->isUsingCustomImage = sc->isUsingCustomPaintRoutine();
	bpc->setPopupMenuItems(sc->getItemList());
	bpc->setUseRightClickForPopup(sc->getScriptObjectProperty(ScriptingApi::Content::ScriptPanel::PopupOnRightClick));
	bpc->alignPopup(sc->getScriptObjectProperty(ScriptingApi::Content::ScriptPanel::popupMenuAlign));
	

	bpc->setInterceptsMouseClicks(sc->getScriptObjectProperty(ScriptingApi::Content::ScriptPanel::enabled), true);

	bpc->repaint();

	bpc->setAllowCallback(getScriptComponent()->getScriptObjectProperty(ScriptingApi::Content::ScriptPanel::allowCallbacks).toString());

	contentComponent->repaint();
}

void ScriptCreatedComponentWrappers::PanelWrapper::mouseCallback(const var &mouseInformation)
{
	auto sp = dynamic_cast<ScriptingApi::Content::ScriptPanel*>(getScriptComponent());

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


ScriptCreatedComponentWrappers::PluginEditorWrapper::PluginEditorWrapper(ScriptContentComponent *content, ScriptingApi::Content::ScriptPluginEditor *editor, int index):
ScriptCreatedComponentWrapper(content, index)
{
	AudioProcessorWrapper *wrapper = editor->getProcessor();

	WrappedAudioProcessorEditorContent *apew = new WrappedAudioProcessorEditorContent(wrapper);

	if (wrapper != nullptr)
	{
		apew->setAudioProcessor(editor->getProcessor()->getWrappedAudioProcessor());
		apew->setOpaque(false);
	}

	component = apew;
}


void ScriptCreatedComponentWrappers::PluginEditorWrapper::updateComponent()
{

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
			}

			suffix = c->getScriptObjectProperty(ScriptingApi::Content::ScriptSlider::Properties::suffix);
			break;
		}
		case ScriptedControlAudioParameter::Type::Button:
			break;
		case ScriptedControlAudioParameter::Type::ComboBox:
			range.interval = 1.0f;
			itemList = dynamic_cast<ScriptingApi::Content::ScriptComboBox*>(c)->getItemList();
			break;
		case ScriptedControlAudioParameter::Type::Panel:
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

			scriptProcessor->setAttribute(componentIndex, range.convertFrom0to1(newValue), sendNotification);
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
		return range.convertTo0to1(scriptProcessor->getDefaultValue(componentIndex));
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
		return (int)range.end +1 ;

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
