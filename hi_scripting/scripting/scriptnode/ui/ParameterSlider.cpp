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
 *   which also must be licensed for commercial applications:
 *
 *   http://www.juce.com
 *
 *   ===========================================================================
 */

namespace scriptnode
{
using namespace juce;
using namespace hise;




ParameterSlider::ParameterSlider(NodeBase* node_, int index) :
	SimpleTimer(node_->getScriptProcessor()->getMainController_()->getGlobalUIUpdater()),
	parameterToControl(node_->getParameter(index)),
	node(node_),
	pTree(node_->getParameter(index)->data)
{
	setName(pTree[PropertyIds::ID].toString());

	connectionListener.setCallback(pTree,
		{ PropertyIds::Connection, PropertyIds::ModulationTarget, PropertyIds::OpType },
		valuetree::AsyncMode::Coallescated,
		BIND_MEMBER_FUNCTION_2(ParameterSlider::checkEnabledState));

	rangeListener.setCallback(pTree, RangeHelpers::getRangeIds(),
		valuetree::AsyncMode::Coallescated,
		BIND_MEMBER_FUNCTION_2(ParameterSlider::updateRange));

	valueListener.setCallback(pTree, { PropertyIds::Value },
		valuetree::AsyncMode::Asynchronously,
		[this](Identifier, var newValue)
	{
		setValue(newValue, dontSendNotification);
		repaint();
	});

	addListener(this);
	setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
	setTextBoxStyle(Slider::TextBoxBelow, false, 100, 18);
	setLookAndFeel(&laf);

	if (auto nl = dynamic_cast<ParameterKnobLookAndFeel::SliderLabel*>(getTextBox()))
	{
		nl->updateText();
	}

	setScrollWheelEnabled(false);
}
    
ParameterSlider::~ParameterSlider()
{
    removeListener(this);
}


void ParameterSlider::checkEnabledState(Identifier, var)
{
	bool isModulated = (bool)pTree[PropertyIds::ModulationTarget];
	bool isConnectedToMacro = pTree[PropertyIds::Connection].toString().isNotEmpty();
	bool isReadOnly = pTree[PropertyIds::OpType].toString() == OperatorIds::SetValue.toString();

	modulationActive = isModulated || isConnectedToMacro;

	if (modulationActive)
	{
		setEnabled(!isReadOnly);
		start();
	}
	else
	{
		stop();
		setEnabled(true);
	}

	if (auto g = findParentComponentOfClass<DspNetworkGraph>())
		g->repaint();
}


void ParameterSlider::updateRange(Identifier, var)
{
	auto range = RangeHelpers::getDoubleRange(pTree);

	setRange(range.getRange(), range.interval);
	setSkewFactor(range.skew);

	repaint();
}


void ParameterSlider::timerCallback()
{
	if (parameterToControl != nullptr)
	{
		modulationActive = true;
		auto modValue = parameterToControl->getModValue();

		if (modValue != lastModValue)
		{
			lastModValue = modValue;
			repaint();
		}
	}
}


bool ParameterSlider::isInterestedInDragSource(const SourceDetails& details)
{
	if (details.sourceComponent == this)
		return false;

	return !modulationActive;
}

void ParameterSlider::paint(Graphics& g)
{
	Slider::paint(g);

	if (macroHoverIndex != -1)
	{
		g.setColour(Colour(SIGNAL_COLOUR));
		g.drawRect(getLocalBounds(), 1);
	}
}


void ParameterSlider::itemDragEnter(const SourceDetails& dragSourceDetails)
{
	macroHoverIndex = 1;
	repaint();
}


void ParameterSlider::itemDragExit(const SourceDetails& dragSourceDetails)
{
	macroHoverIndex = -1;
	repaint();
}



void ParameterSlider::itemDropped(const SourceDetails& dragSourceDetails)
{
	macroHoverIndex = -1;
	parameterToControl->addConnectionTo(dragSourceDetails.description);
	repaint();	
}

void ParameterSlider::mouseDown(const MouseEvent& e)
{
	if (e.mods.isRightButtonDown())
	{
		auto pe = new MacroPropertyEditor(node, pTree);

		pe->setName("Edit Parameter");
		findParentComponentOfClass<FloatingTile>()->showComponentInRootPopup(pe, this, getLocalBounds().getCentre());
	}
	else
		Slider::mouseDown(e);
}




void ParameterSlider::sliderDragStarted(Slider*)
{
	if (auto nl = dynamic_cast<ParameterKnobLookAndFeel::SliderLabel*>(getTextBox()))
	{
		nl->startDrag();
	}
}

void ParameterSlider::sliderDragEnded(Slider*)
{
	if (auto nl = dynamic_cast<ParameterKnobLookAndFeel::SliderLabel*>(getTextBox()))
	{
		nl->endDrag();
	}
}

void ParameterSlider::sliderValueChanged(Slider*)
{
	if (parameterToControl != nullptr)
		parameterToControl->setValueAndStoreAsync(getValue());

	if (auto nl = dynamic_cast<ParameterKnobLookAndFeel::SliderLabel*>(getTextBox()))
	{
		nl->updateText();
	}
}


juce::String ParameterSlider::getTextFromValue(double value)
{
	if (parameterToControl->valueNames.isEmpty())
		return Slider::getTextFromValue(value);

	int index = (int)value;
	return parameterToControl->valueNames[index];
}

double ParameterSlider::getValueFromText(const String& text)
{
	if (parameterToControl->valueNames.contains(text))
		return (double)parameterToControl->valueNames.indexOf(text);

	return Slider::getValueFromText(text);
}

ParameterKnobLookAndFeel::ParameterKnobLookAndFeel()
{
	cachedImage_smalliKnob_png = ImageProvider::getImage(ImageProvider::ImageType::KnobEmpty); // ImageCache::getFromMemory(BinaryData::knob_empty_png, BinaryData::knob_empty_pngSize);
	cachedImage_knobRing_png = ImageProvider::getImage(ImageProvider::ImageType::KnobUnmodulated); // ImageCache::getFromMemory(BinaryData::ring_unmodulated_png, BinaryData::ring_unmodulated_pngSize);
}


juce::Font ParameterKnobLookAndFeel::getLabelFont(Label&)
{
	return GLOBAL_BOLD_FONT();
}

    

juce::Label* ParameterKnobLookAndFeel::createSliderTextBox(Slider& slider)
{
	auto l = new SliderLabel(slider);
	l->refreshWithEachKey = false;

	l->setJustificationType(Justification::centred);
	l->setKeyboardType(TextInputTarget::decimalKeyboard);

	l->setColour(Label::textColourId, Colours::white);
	l->setColour(Label::backgroundColourId, Colours::transparentBlack);
	l->setColour(Label::outlineColourId, Colours::transparentBlack);
	l->setColour(TextEditor::textColourId, Colours::white);
	l->setColour(TextEditor::backgroundColourId, Colours::transparentBlack);
	l->setColour(TextEditor::outlineColourId, Colours::transparentBlack);
	l->setColour(TextEditor::highlightColourId, Colour(SIGNAL_COLOUR).withAlpha(0.5f));
	l->setColour(TextEditor::ColourIds::focusedOutlineColourId, Colour(SIGNAL_COLOUR));
	l->setColour(Label::ColourIds::outlineWhenEditingColourId, Colour(SIGNAL_COLOUR));

	return l;
}

void ParameterKnobLookAndFeel::drawRotarySlider(Graphics& g, int x, int y, int width, int height, float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle, Slider& s)
{
	height = s.getHeight();
	width = s.getWidth();

	int xOffset = (s.getWidth() - 48) / 2;

	const int filmstripHeight = cachedImage_smalliKnob_png.getHeight() / 128;

	{
		const double value = s.getValue();
		const double normalizedValue = (value - s.getMinimum()) / (s.getMaximum() - s.getMinimum());
		const double proportion = pow(normalizedValue, s.getSkewFactor());
		const int stripIndex = (int)(proportion * 127);


		const int offset = stripIndex * filmstripHeight;

		Image clip = cachedImage_smalliKnob_png.getClippedImage(Rectangle<int>(0, offset, filmstripHeight, filmstripHeight));



		g.setColour(Colours::black.withAlpha(s.isEnabled() ? 1.0f : 0.5f));
		g.drawImage(clip, xOffset, 3, 48, 48, 0, 0, filmstripHeight, filmstripHeight);
	}

	{
		auto ps = dynamic_cast<ParameterSlider*>(&s);

		double value = ps->modulationActive ? ps->lastModValue : ps->getValue();


		const double normalizedValue = (value - s.getMinimum()) / (s.getMaximum() - s.getMinimum());
		const double proportion = jlimit(0.0, 1.0, pow(normalizedValue, s.getSkewFactor()));
		const int stripIndex = (int)(proportion * 127);

		Image *imageToUse = &cachedImage_knobRing_png;

		Image clipRing = imageToUse->getClippedImage(Rectangle<int>(0, (int)(stripIndex * filmstripHeight), filmstripHeight, filmstripHeight));

		g.setColour(Colours::black.withAlpha(s.isEnabled() ? 1.0f : 0.5f));
		g.drawImage(clipRing, xOffset, 3, 48, 48, 0, 0, filmstripHeight, filmstripHeight);
	}

}

MacroParameterSlider::MacroParameterSlider(NodeBase* node, int index) :
	slider(node, index)
{
	addAndMakeVisible(slider);
}

void MacroParameterSlider::resized()
{
	auto b = getLocalBounds();
	b.removeFromBottom(10);
	slider.setBounds(b);
}

void MacroParameterSlider::mouseDrag(const MouseEvent& event)
{
	if (editEnabled)
	{
		if (auto container = DragAndDropContainer::findParentDragContainerFor(this))
		{
			DynamicObject::Ptr details = new DynamicObject();

			details->setProperty(PropertyIds::ID, slider.node->getId());
			details->setProperty(PropertyIds::ParameterId, slider.parameterToControl->getId());

			container->startDragging(var(details), &slider);
		}
	}
}

void MacroParameterSlider::paintOverChildren(Graphics& g)
{
	if (editEnabled)
	{
		g.setColour(Colour(SIGNAL_COLOUR).withAlpha(0.2f));
		g.fillRoundedRectangle(getLocalBounds().reduced(2).toFloat(), 3.0f);
	}
}

void MacroParameterSlider::setEditEnabled(bool shouldBeEnabled)
{
	slider.setEnabled(!shouldBeEnabled);
	editEnabled = shouldBeEnabled;

	if (editEnabled)
		slider.addMouseListener(this, true);
	else
		slider.removeMouseListener(this);

	repaint();
}

}

