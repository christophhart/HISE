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


Note::Note(FloatingTile* parent) :
	FloatingTileContent(parent)
{
	addAndMakeVisible(editor = new TextEditor());
	editor->setFont(GLOBAL_BOLD_FONT());
	editor->setColour(TextEditor::ColourIds::backgroundColourId, Colours::transparentBlack);
	editor->setColour(TextEditor::ColourIds::textColourId, Colours::white.withAlpha(0.8f));
	editor->setColour(TextEditor::ColourIds::focusedOutlineColourId, Colours::white.withAlpha(0.5f));
	editor->setColour(CaretComponent::ColourIds::caretColourId, Colours::white);
	editor->addListener(this);
	editor->setReturnKeyStartsNewLine(true);
	editor->setMultiLine(true, true);

	editor->setLookAndFeel(&plaf);
}

void Note::resized()
{
	editor->setBounds(getLocalBounds().withTrimmedTop(16));
}

ValueTree Note::exportAsValueTree() const
{
	ValueTree v = FloatingTileContent::exportAsValueTree();
	v.setProperty("Text", editor->getText(), nullptr);

	return v;
}

void Note::restoreFromValueTree(const ValueTree& v)
{
	editor->setText(v.getProperty("Text", editor->getText()), dontSendNotification);
}

void Note::labelTextChanged(Label* )
{

}

void EmptyComponent::paint(Graphics& g)
{
	g.fillAll(c);
}

void EmptyComponent::mouseDown(const MouseEvent& event)
{
    if (event.mods.isRightButtonDown())
	{
		auto e2 = event.getEventRelativeTo(getParentShell()->getRootComponent());

		getParentShell()->getRootComponent()->setSelector(this, e2.getMouseDownPosition());
	}
}

ConsolePanel::ConsolePanel(FloatingTile* parent) :
	FloatingTileContent(parent)
{
	setInterceptsMouseClicks(false, true);
	addAndMakeVisible(console = new Console(parent->findParentComponentOfClass<BackendRootWindow>()->getBackendProcessor()));
}

MidiKeyboardPanel::MidiKeyboardPanel(FloatingTile* parent) :
	FloatingTileContent(parent)
{
	setInterceptsMouseClicks(false, true);

	addAndMakeVisible(keyboard = new CustomKeyboard(parent->getRootWindow()->getBackendProcessor()->getKeyboardState()));

	keyboard->setLowestVisibleKey(12);
}

void ApplicationCommandButtonPanel::setCommand(int commandID)
{
	Path p = BackendCommandIcons::getIcon(commandID);

	b->setCommandToTrigger(getParentShell()->getRootWindow()->getBackendProcessor()->getCommandManager(), commandID, true);
	b->setShape(p, false, true, true);
	b->setVisible(true);
}


ModulatorSynthChain* PanelWithProcessorConnection::getMainSynthChain()
{
	return getParentShell()->getRootWindow()->getMainSynthChain();
}

const ModulatorSynthChain* PanelWithProcessorConnection::getMainSynthChain() const
{
	return getParentShell()->getRootWindow()->getMainSynthChain();
}
