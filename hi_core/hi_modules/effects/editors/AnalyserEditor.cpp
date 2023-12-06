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


#include "AnalyserEditor.h"



//==============================================================================
AnalyserEditor::AnalyserEditor (ProcessorEditor *p):
	ProcessorEditorBody(p),
	updater(*this)
{
	addAndMakeVisible(typeSelector = new HiComboBox("Type"));

	addAndMakeVisible(bufferSize = new HiComboBox("BufferSize"));

	bufferSize->setTextWhenNothingSelected("Analyser Buffer Size");
	bufferSize->addItem("4096 Samples", 4096);
	bufferSize->addItem("8192 Samples", 8192);
	bufferSize->addItem("16384 Samples", 16384);
	bufferSize->addItem("32768 Samples", 32768);

	typeSelector->setTextWhenNothingSelected("Analyser Type");
	typeSelector->addItem("Nothing", 1);
	typeSelector->addItem("Goniometer", 2);
	typeSelector->addItem("Oscilloscope", 3);
	typeSelector->addItem("Spectral Analyser", 4);

	typeSelector->setup(getProcessor(), AnalyserEffect::Parameters::PreviewType, "Analyser Type");
	bufferSize->setup(getProcessor(), AnalyserEffect::Parameters::BufferSize, "Buffer Size");

    setSize (800, 300);

	h = getHeight();

}

AnalyserEditor::~AnalyserEditor()
{
	typeSelector = nullptr;
	bufferSize = nullptr;
}

void AnalyserEditor::updateGui()
{
	typeSelector->updateValue();
	bufferSize->updateValue();

	int currentIndex = typeSelector->getSelectedId();

	if (currentIndex != getIdForCurrentType())
	{
		switch (currentIndex)
		{
		case 1: analyser = nullptr; break;
		case 2: addAndMakeVisible(analyser = new Goniometer(getProcessor())); break;
		case 3: addAndMakeVisible(analyser = new Oscilloscope(getProcessor())); break;
		case 4: addAndMakeVisible(analyser = new FFTDisplay(getProcessor())); break;
		}

		refreshBodySize();
		resized();
	}
}

//==============================================================================
void AnalyserEditor::paint (Graphics& /*g*/)
{

}

void AnalyserEditor::resized()
{
	auto area = getLocalBounds();

	auto top = area.removeFromTop(32);
	top.removeFromLeft(10);

	typeSelector->setBounds(top.removeFromLeft(128));
	top.removeFromLeft(20);
	bufferSize->setBounds(top.removeFromLeft(128));

	if (analyser != nullptr)
	{
		analyser->setBounds(area.reduced(10));
	}
}

}