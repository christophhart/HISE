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


namespace hise {
using namespace juce;


MPEModulatorEditor::MPEModulatorEditor(ProcessorEditor* parent) :
	ProcessorEditorBody(parent)
{
	addAndMakeVisible(tableEditor = new TableEditor(getProcessor()->getMainController()->getControlUndoManager(), dynamic_cast<LookupTableProcessor*>(getProcessor())->getTable(0)));
	addAndMakeVisible(typeSelector = new HiComboBox("Type"));
	addAndMakeVisible(smoothingTime = new HiSlider("SmoothingTime"));


	tableEditor->connectToLookupTableProcessor(getProcessor());

	typeSelector->setup(getProcessor(), MPEModulator::SpecialParameters::GestureCC, "Gesture Type");
	typeSelector->addItem("Press", MPEModulator::Gesture::Press);
	typeSelector->addItem("Slide", MPEModulator::Gesture::Slide);
	typeSelector->addItem("Glide", MPEModulator::Gesture::Glide);
	typeSelector->addItem("Stroke", MPEModulator::Gesture::Stroke);
	typeSelector->addItem("Lift", MPEModulator::Gesture::Lift);

	smoothingTime->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
	smoothingTime->setTextBoxStyle(Slider::TextBoxRight, true, 80, 20);
	smoothingTime->setup(getProcessor(), MPEModulator::SpecialParameters::SmoothingTime, "Smoothing Time");
	smoothingTime->setMode(HiSlider::Time, 0.0, 2000.0, 100.0, 0.1);

	addAndMakeVisible(mpePanel = new MPEKeyboard(getProcessor()->getMainController()->getKeyboardState()));

	mpePanel->setColour(MPEKeyboard::ColourIds::bgColour, Colour(0x11000000));
}

void MPEModulatorEditor::resized()
{
	auto area = getLocalBounds();

	area = area.withSizeKeepingCentre(650, area.getHeight() - 12);

	area = area.reduced(8);

	auto keyboardArea = area.removeFromBottom(80);

	

	auto tablePanel = area.removeFromLeft(450);

	mpePanel->setBounds(keyboardArea.removeFromBottom(72));

	tableEditor->setBounds(tablePanel);

	area.removeFromTop(50);

	typeSelector->setBounds(area.removeFromTop(40).reduced(6));
	smoothingTime->setBounds(area.removeFromTop(60).reduced(6));
}

void MPEModulatorEditor::paint(Graphics& g)
{
	auto area = getLocalBounds();

	area = area.withSizeKeepingCentre(650, area.getHeight() - 12);

	g.setColour(Colour(0x30000000));
	g.fillRoundedRectangle(FLOAT_RECTANGLE(area), 3.0f);
	g.setColour(Colours::white);

	area.reduce(8, 8);
	area.removeFromLeft(450);

	g.setFont(GLOBAL_BOLD_FONT().withHeight(24.0f));
	g.drawText("MPE", area, Justification::topRight);
}

}

