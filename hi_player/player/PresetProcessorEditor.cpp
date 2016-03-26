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
*   along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
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

PresetProcessorEditor::PresetProcessorEditor(PresetProcessor *pp_) :
AudioProcessorEditor(pp_),
pp(pp_)
{

	addAndMakeVisible(header = new PresetHeader(pp_));

	addAndMakeVisible(interfaceComponent = new ScriptContentContainer(pp->getMainSynthChain()));

	interfaceComponent->checkInterfaces();

	interfaceComponent->refreshContentBounds();
	interfaceComponent->setIsFrontendContainer(true);

	setSize(700, getComponentHeight());
}

PresetProcessorEditor::~PresetProcessorEditor()
{
	header = nullptr;

	interfaceComponent = nullptr;
}

void PresetProcessorEditor::resized()
{
	header->setBounds(0, 0, 700, 35);

	const int interfaceWidth = interfaceComponent->getContentWidth() != -1 ? interfaceComponent->getContentWidth() : 700 - 6;

	const int interfaceX = jmax<int>(3, (getWidth() - interfaceWidth) / 2);

	interfaceComponent->setVisible(!pp->getMainSynthChain()->getEditorState(Processor::Folded));

	interfaceComponent->setBounds(interfaceX, 30, interfaceWidth, interfaceComponent->getContentHeight());
	
}

void PresetProcessorEditor::paint(Graphics &g)
{
	
	ProcessorEditorLookAndFeel::drawBackground(g, 700, getHeight(), Colour(0xFF444444), false, 2);
}

int PresetProcessorEditor::getComponentHeight() const
{
	if (pp->getMainSynthChain()->getEditorState(Processor::Folded))
	{
		return 35;
	}
	else
	{
		return interfaceComponent->getContentHeight() + 36;
	}
}
