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

HeaderButton::HeaderButton(const String &name, const unsigned char *path, size_t pathSize, ProcessorEditorHeader *parentHeader_) :
parentHeader(parentHeader_)
{
	addAndMakeVisible(button = new ShapeButton(name, Colours::white, Colours::white, Colours::white));
	Path p;
	p.loadPathFromData(path, pathSize);
	button->setShape(p, true, true, true);
	button->setToggleState(true, dontSendNotification);
	refresh();
    
    button->setWantsKeyboardFocus(false);

	setWantsKeyboardFocus(false);

	button->addListener(parentHeader);
	button->addListener(this);
}


void HeaderButton::buttonClicked(Button *)
{
	auto p = findParentComponentOfClass<ProcessorEditorHeader>()->getProcessor();

	if (p->getMainController()->getScriptComponentEditBroadcaster()->getCurrentlyLearnedComponent() != nullptr)
	{
		Learnable::LearnData d;
		d.processorId = p->getId();

		d.parameterId = PresetHandler::showYesNoWindow("Create a inverted connection", "Do you want to create a inverted connection (so that pressing the button on the interface will enable this module?  \nPress no in order to create a normal bypass connection") ? "Enabled" : "Bypass";
		d.range = { 0.0, 1.0 };
		d.value = p->isBypassed();
		d.name = d.parameterId;

		p->getMainController()->getScriptComponentEditBroadcaster()->setLearnData(d);
	}

	refresh();
}

void HeaderButton::refresh()
{
	button->setTooltip(getTooltip());

	bool off = !button->getToggleState();

	Colour buttonColour = parentHeader->isHeaderOfModulatorSynth() ? Colours::black.withAlpha(0.8f) : Colours::white;

	buttonColour = Colours::white;

	Colour shadowColour = parentHeader->isHeaderOfModulatorSynth() ? Colours::cyan.withAlpha(0.25f) : Colours::white.withAlpha(0.6f);

	shadowColour = Colours::white.withAlpha(0.7f);

	DropShadowEffect *shadow = dynamic_cast<DropShadowEffect*>(button->getComponentEffect());

	jassert(shadow != nullptr);

	shadow->setShadowProperties(DropShadow(off ? Colours::transparentBlack : shadowColour, 3, Point<int>()));


	buttonColour = off ? Colours::grey.withAlpha(0.3f) : buttonColour;

	buttonColour = buttonColour.withMultipliedAlpha(isEnabled() ? 1.0f : 0.2f);

	button->setColours(buttonColour, buttonColour, buttonColour);
	button->repaint();
	repaint();
}


void HeaderButton::paint(Graphics &g)
{
	float alpha = .8f;

	if (!isEnabled()) alpha *= 0.6f;

	if (!isMouseOver(true)) alpha *= 0.8f;

	if (!button->getToggleState()) alpha *= 0.5f;

	Colour bg = Colours::grey.withBrightness(0.5f).withAlpha(alpha);

	g.setGradientFill(ColourGradient(bg.withMultipliedAlpha(0.65f), 0.0f, 0.0f,
		bg, 0.0f, (float)getHeight(), false));

	g.fillRoundedRectangle(0.0f, 0.0f, (float)getWidth(), (float)getHeight(), 5.0f);
}


void IntensitySlider::mouseDrag(const MouseEvent& e)
{
	auto p = findParentComponentOfClass<ProcessorEditorHeader>()->getProcessor();

	if (p->getMainController()->getScriptComponentEditBroadcaster()->getCurrentlyLearnedComponent() != nullptr)
	{
		Learnable::LearnData d;
		d.processorId = p->getId();

		d.parameterId = "Intensity";
		d.range = { 0.0, 1.0 };
		d.value = getValue();
		d.name = d.parameterId;

		p->getMainController()->getScriptComponentEditBroadcaster()->setLearnData(d);
	}

	Slider::mouseDrag(e);
}

ChainIcon::ChainIcon(Processor *p_)
{
	p = p_;

	if (p->getId() == "Midi Processor")  chainType = ModulatorSynth::MidiProcessor;
	else if (p->getId() == "GainModulation")  chainType = ModulatorSynth::GainModulation;
	else if (p->getId() == "PitchModulation") chainType = ModulatorSynth::PitchModulation;
	else if (p->getId() == "FX")			  chainType = ModulatorSynth::EffectChain;
	else if (p->getId() == "Sample Start")	  chainType = ModulatorSampler::SampleStartModulation;
	else if (dynamic_cast<const CurveEq*>(p) != nullptr)
	{
		chainType = Filter;
		addAndMakeVisible(filterGraph = new FilterGraph(0, FilterGraph::Icon));
		
	}
	else if (dynamic_cast<const MasterEffectProcessor*>(p) != nullptr)
	{
		chainType = MasterEffect;
	}
	else if (dynamic_cast<const MonophonicEffectProcessor*>(p) != nullptr)
	{
		chainType = MonophonicEffect;
	}
	else if (dynamic_cast<const VoiceEffectProcessor*>(p) != nullptr)
	{
		chainType = PolyphonicEffect;
	}
	else if (dynamic_cast<const MacroModulator*>(p) != nullptr)
	{
		chainType = MacroMod;
	}
	else if (dynamic_cast<const VoiceStartModulator*>(p) != nullptr)
	{
		chainType = VoiceModulator;
	}
	else if (dynamic_cast<const TimeVariantModulator*>(p) != nullptr)
	{
		chainType = TimeModulator;
	}
	else if (dynamic_cast<const EnvelopeModulator*>(p) != nullptr)
	{
		chainType = Envelope;
	}
	else if (dynamic_cast<const JavascriptProcessor*>(p) != nullptr)
	{
		chainType = ScriptingProcessor;
	}
	else if (dynamic_cast<const ModulatorSynth*>(p) != nullptr)
	{
		chainType = ChainIcons::ModulatorSynthIcon;

	}



	else chainType = -1;
};

ChainIcon::~ChainIcon()
{
}

class ColourSelectorWithRecentList: public ColourSelector
{
public:

	ColourSelectorWithRecentList(BackendProcessorEditor *editor_, Colour initialColour=Colours::transparentBlack):
		ColourSelector(showColourspace),
		editor(editor_)
	{
		editor->restoreSwatchColours(swatchColours);

		

		Random r;

		r.setSeed(Time::getCurrentTime().getApproximateMillisecondCounter());

		Colour randomColour((uint8)(r.nextFloat() * 255.0f), (uint8)(r.nextFloat() * 255.0f), (uint8)(r.nextFloat() * 255.0f));

		setName("background");
		setCurrentColour(initialColour.isTransparent() ? randomColour : initialColour);
		setColour(ColourSelector::backgroundColourId, Colours::transparentBlack);

		setSize(300, 280);

		setLookAndFeel(&plaf);
	}

	~ColourSelectorWithRecentList()
	{
		editor->storeSwatchColours(swatchColours);
	}

	int getNumSwatches() const override { return 8; }
	
	void setSwatchColour(int index, const Colour& newColour) override
	{
		swatchColours[index] = newColour;
	}

	Colour getSwatchColour(int index) const override { return swatchColours[index]; }

private:

	PopupLookAndFeel plaf;

	BackendProcessorEditor *editor;

	mutable Colour swatchColours[8];
};
										

void ChainIcon::mouseDown(const MouseEvent &)
{
	if (chainType == ModulatorSynthIcon)
	{
		Colour iconColour = dynamic_cast<ModulatorSynth*>(p)->getIconColour();
		
		auto editor = GET_BACKEND_ROOT_WINDOW(this);

		ColourSelectorWithRecentList *colourSelector = new ColourSelectorWithRecentList(editor->getMainPanel(), iconColour);

		colourSelector->addChangeListener(this);

		CallOutBox::launchAsynchronously(std::unique_ptr<Component>(colourSelector), getScreenBounds(), nullptr);
	}
}


void ChainIcon::changeListenerCallback(ChangeBroadcaster *b)
{
	ColourSelector *s = dynamic_cast<ColourSelector*>(b);

	if (s != nullptr)
	{
		if (ModulatorSynth *ms = dynamic_cast<ModulatorSynth*>(p))
		{
			ms->setIconColour(s->getCurrentColour());
		}


	}

	repaint();
}


void ChainIcon::paint(Graphics &g)
{

	float w = (float)getWidth();
	float h = (float)getHeight();

	Path path;

	Colour c = Colours::white;

	if (dynamic_cast<Chain*>(p) != nullptr ||
		dynamic_cast<ModulatorSynth*>(p) != nullptr)
		c = Colours::black;

	if (chainType == ChainIcon::ModulatorSynthIcon)
	{
		g.setColour(Colours::grey);
		g.drawRoundedRectangle(1.0f, 1.0f, w - 2.0f, h - 2.0f, 3.0f, 1.0f);

		Colour iconColour = dynamic_cast<ModulatorSynth*>(p)->getIconColour();

		if (iconColour == Colours::transparentBlack) return;


		ColourGradient grad(iconColour.withAlpha(0.7f), 0.0f, 0.0f, iconColour, 0.0f, h, false);

		g.setGradientFill(grad);
		g.fillRoundedRectangle(1.0f, 1.0f, w - 2.0f, h - 2.0f, 3.0f);




		return;

	}

	if (chainType == ModulatorSynth::GainModulation)
	{
		path.loadPathFromData(HiBinaryData::ProcessorIcons::gainModulation, SIZE_OF_PATH(HiBinaryData::ProcessorIcons::gainModulation));

	}
	else if (chainType == ModulatorSynth::PitchModulation)
	{
		path.loadPathFromData(HiBinaryData::ProcessorIcons::pitchModulation, SIZE_OF_PATH(HiBinaryData::ProcessorIcons::pitchModulation));

	}
	else if (chainType == ModulatorSynth::EffectChain)
	{
		path.loadPathFromData(HiBinaryData::ProcessorIcons::effectChain, SIZE_OF_PATH(HiBinaryData::ProcessorIcons::effectChain));
	}

	else if (chainType == ModulatorSampler::SampleStartModulation)
	{
		path.loadPathFromData(HiBinaryData::ProcessorIcons::sampleStartModulation, SIZE_OF_PATH(HiBinaryData::ProcessorIcons::sampleStartModulation));

	}
	else if (chainType == Filter)
	{
		return;
	}
	else
	{
		path = p->getSymbol();
	}

	path.scaleToFit(.0f, .0f, (float)getWidth(), (float)getHeight(), true);

	g.setColour(c.withAlpha(JUCE_LIVE_CONSTANT_OFF(0.4f)));

	g.fillPath(path);




}


void ChainIcon::resized()
{
	if (filterGraph != nullptr) filterGraph->setBounds(getLocalBounds());
}

Drawable * ButtonShapes::createSymbol(Symbol s, bool on, bool enabled)
{
	switch (s)
	{
	case Debug:  return bipolarShape(on, enabled);
	case Fold:	 return foldShape(on, enabled);
	case Bypass: return bypassShape(on, enabled);
	case Delete: return deleteShape(on, enabled);
	case Add:	 return addShape(on, enabled);
	case Plot:	 return plotShape(on, enabled);
	case Routing: return routingShape(on, enabled);
	default:	 jassertfalse; return nullptr;
	}
}

Drawable * ButtonShapes::foldShape(bool on, bool enabled)
{
	DrawablePath *dp = new DrawablePath();

	Path internalPath1;

	internalPath1.clear();
	internalPath1.startNewSubPath(0.0f, 0.0f);
	internalPath1.lineTo(1.0000f, 0.5000f);
	internalPath1.lineTo(0.0f, 1.0000f);
	internalPath1.closeSubPath();

	if (on) internalPath1.applyTransform(AffineTransform::rotation(float_Pi / 2));

	dp->setPath(internalPath1);

	dp->setFill(FillType(Colours::white.withAlpha(enabled ? 1.0f : 0.4f)));

	return dp;
}

Drawable * ButtonShapes::deleteShape(bool /*on*/, bool enabled)
{
	DrawableImage *di = new DrawableImage();
	Image b(Image::PixelFormat::ARGB, 20, 20, true);
	Graphics g(b);

	Path internalPath1;
	Path internalPath2;

	internalPath1.clear();
	internalPath1.startNewSubPath(18.0f, 2.0f);
	internalPath1.lineTo(2.0f, 18.0f);
	internalPath1.closeSubPath();

	internalPath2.clear();
	internalPath2.startNewSubPath(18.0f, 18.0f);
	internalPath2.lineTo(2.0f, 2.0f);
	internalPath2.closeSubPath();

	g.setColour(Colours::white.withAlpha(enabled ? 1.0f : 0.4f));

	g.strokePath(internalPath1, PathStrokeType(4.000f, PathStrokeType::mitered, PathStrokeType::square));
	g.strokePath(internalPath2, PathStrokeType(4.000f, PathStrokeType::mitered, PathStrokeType::square));

	DropShadow d(Colours::white.withAlpha(0.5f), 5, Point<int>());

	d.drawForPath(g, internalPath1);
	d.drawForPath(g, internalPath2);


	di->setImage(b);

	return di;
}

Drawable * ButtonShapes::plotShape(bool /*on*/, bool enabled)
{
	DrawableImage *di = new DrawableImage();
	Image b(Image::PixelFormat::ARGB, 20, 20, true);
	Graphics g(b);

	Path internalPath1;

	internalPath1.clear();
	internalPath1.startNewSubPath(2.0f, 10.0f);
	internalPath1.cubicTo(10.0f, static_cast<float> (-17), 10.0f, 36.0f, 18.0f, 10.0f);

	g.setColour(Colours::white.withAlpha(enabled ? 1.0f : 0.4f));

	g.strokePath(internalPath1, PathStrokeType(4.000f, PathStrokeType::mitered, PathStrokeType::square));

	di->setImage(b);

	return di;
}

Drawable * ButtonShapes::addShape(bool /*on*/, bool enabled)
{
	DrawableImage *di = new DrawableImage();
	Image b(Image::PixelFormat::ARGB, 20, 20, true);
	Graphics g(b);

	Path internalPath1;
	Path internalPath2;

	internalPath1.clear();
	internalPath1.startNewSubPath(10., 2.0f);
	internalPath1.lineTo(10.0f, 18.0f);
	internalPath1.closeSubPath();

	internalPath2.clear();
	internalPath2.startNewSubPath(2.0f, 10.0f);
	internalPath2.lineTo(18.0f, 10.0f);
	internalPath2.closeSubPath();

	g.setColour(Colours::white.withAlpha(enabled ? 1.0f : 0.4f));

	g.strokePath(internalPath1, PathStrokeType(5.000f, PathStrokeType::mitered, PathStrokeType::square));
	g.strokePath(internalPath2, PathStrokeType(5.000f, PathStrokeType::mitered, PathStrokeType::square));

	di->setImage(b);

	return di;
}

Drawable * ButtonShapes::bypassShape(bool /*on*/, bool enabled)
{
	DrawableImage *di = new DrawableImage();
	Image b(Image::PixelFormat::ARGB, 20, 20, true);
	Graphics g(b);

	Path internalPath1;
	Path internalPath2;

	internalPath1.clear();
	internalPath1.startNewSubPath(10.0f, 2.0f);
	internalPath1.cubicTo(14.0f, 2.0f, 18.0f, 6.0f, 18.0f, 10.0f);
	internalPath1.cubicTo(18.0f, 14.0f, 14.0f, 18.0f, 10.0f, 18.0f);
	internalPath1.startNewSubPath(2.0f, 10.0f);
	internalPath1.cubicTo(2.0f, 6.0f, 6.0f, 2.0f, 10.0f, 2.0f);

	internalPath2.clear();
	internalPath2.startNewSubPath(10, 10);
	internalPath2.lineTo(2.0f, 18);
	internalPath2.closeSubPath();

	g.setColour(Colours::white.withAlpha(enabled ? 1.0f : 0.4f));
	g.addTransform(AffineTransform::rotation(float_Pi * 3.0f / 4.0f, 10, 10));

	g.strokePath(internalPath1, PathStrokeType(4.000f, PathStrokeType::mitered, PathStrokeType::square));
	g.strokePath(internalPath2, PathStrokeType(4.000f, PathStrokeType::mitered, PathStrokeType::square));


	di->setImage(b);

	return di;
}

Drawable * ButtonShapes::bipolarShape(bool , bool enabled)
{
	static const unsigned char pathData[] = { 110, 109, 0, 0, 7, 67, 46, 151, 47, 68, 108, 0, 0, 250, 66, 46, 151, 47, 68, 108, 0, 0, 12, 67, 46, 215, 43, 68, 108, 0, 0, 27, 67, 46, 151, 47, 68, 108, 0, 0, 17, 67, 46, 151, 47, 68, 108, 0, 0, 17, 67, 46, 23, 50, 68, 108, 0, 0, 27, 67, 46, 23, 50, 68, 108, 0, 0, 12, 67, 46, 215, 53, 68, 108, 0, 0, 250, 66, 46, 23, 50, 68, 108, 0,
		0, 7, 67, 46, 23, 50, 68, 99, 101, 0, 0 };

	DrawableImage *di = new DrawableImage();
	Image b(Image::PixelFormat::ARGB, 20, 20, true);
	Graphics g(b);

	Path p;

	p.loadPathFromData(pathData, sizeof(pathData));

	g.setColour(Colours::white.withAlpha(enabled ? 1.0f : 0.4f));


	g.fillPath(p);

	di->setImage(b);

	return di;
}

Drawable * ButtonShapes::routingShape(bool, bool enabled)
{
	DrawableImage *di = new DrawableImage();
	Image b(Image::PixelFormat::ARGB, 20, 20, true);
	Graphics g(b);

	Path p;

	p.loadPathFromData(HiBinaryData::SpecialSymbols::routingIcon, sizeof(HiBinaryData::SpecialSymbols::routingIcon));

	g.setColour(Colours::white.withAlpha(enabled ? 1.0f : 0.4f));
	

	g.fillPath(p);
	
	di->setImage(b);

	return di;
}


} // namespace hise