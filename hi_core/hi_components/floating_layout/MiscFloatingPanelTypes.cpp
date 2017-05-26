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


PanelWithProcessorConnection::PanelWithProcessorConnection(FloatingTile* parent) :
	FloatingTileContent(parent)
{
	rootWindow = getParentShell()->getRootWindow();

	addAndMakeVisible(connectionSelector = new ComboBox());
	connectionSelector->addListener(this);
	getMainSynthChain()->getMainController()->skin(*connectionSelector);

	connectionSelector->setColour(MacroControlledObject::HiBackgroundColours::upperBgColour, Colours::transparentBlack);
	connectionSelector->setColour(MacroControlledObject::HiBackgroundColours::lowerBgColour, Colours::transparentBlack);
	connectionSelector->setColour(MacroControlledObject::HiBackgroundColours::outlineBgColour, Colours::transparentBlack);
	connectionSelector->setTextWhenNothingSelected("Disconnected");

	addAndMakeVisible(indexSelector = new ComboBox());
	indexSelector->addListener(this);
	getMainSynthChain()->getMainController()->skin(*indexSelector);

	indexSelector->setColour(MacroControlledObject::HiBackgroundColours::upperBgColour, Colours::transparentBlack);
	indexSelector->setColour(MacroControlledObject::HiBackgroundColours::lowerBgColour, Colours::transparentBlack);
	indexSelector->setColour(MacroControlledObject::HiBackgroundColours::outlineBgColour, Colours::transparentBlack);
	indexSelector->setTextWhenNothingSelected("Disconnected");

	

	rootWindow->getMainPanel()->getModuleListNofifier().addChangeListener(this);
}

PanelWithProcessorConnection::~PanelWithProcessorConnection()
{
	content = nullptr;

	auto mp = rootWindow->getMainPanel();
	
	if(mp != nullptr)
		mp->getModuleListNofifier().removeChangeListener(this);

	rootWindow = nullptr;
}

void PanelWithProcessorConnection::paint(Graphics& g)
{

	const bool connected = getProcessor() != nullptr && (!hasSubIndex() || currentIndex != -1);

	g.setColour(Colour(0xFF3D3D3D));
	g.fillRect(0, 16, getWidth(), 18);
	g.setColour(connected ? Colours::white.withAlpha(0.9f) : Colours::white.withAlpha(0.1f));
	
	Path p;
	p.loadPathFromData(ColumnIcons::connectionIcon, sizeof(ColumnIcons::connectionIcon));
	p.scaleToFit(2.0, 18.0, 14.0, 14.0, true);
	g.fillPath(p);

}

void PanelWithProcessorConnection::resized()
{
	if (!listInitialised)
	{
		// Do this here the first time to avoid pure virtual function call...
		refreshConnectionList();
		listInitialised = true;
	}
		

	connectionSelector->setVisible(!getParentShell()->isFolded());
	connectionSelector->setBounds(18, 16, 128, 18);

	indexSelector->setVisible(!getParentShell()->isFolded() && hasSubIndex());
	indexSelector->setBounds(connectionSelector->getRight() + 5, 16, 128, 18);

	if (content != nullptr)
	{
		if (getHeight() > 18)
		{
			content->setVisible(true);
			content->setBounds(0, 16+18, getWidth(), jmax<int>(0, getHeight() - 18-16));
		}
		else
			content->setVisible(false);
	}
}

void PanelWithProcessorConnection::comboBoxChanged(ComboBox* comboBoxThatHasChanged)
{
	if (comboBoxThatHasChanged == connectionSelector)
	{
		indexSelector->clear(dontSendNotification);
		setConnectionIndex(-1);

		if (connectionSelector->getSelectedId() == 1)
		{
			setCurrentProcessor(nullptr);
			refreshContent();
		}
		else
		{
			const String id = comboBoxThatHasChanged->getText();

			auto p = ProcessorHelpers::getFirstProcessorWithName(getMainSynthChain(), id);
		
			connectedProcessor = p;

			if (hasSubIndex())
			{
				refreshIndexList();
				setContentWithUndo(p, 0);
			}
			else
			{
				setConnectionIndex(-1);
				setContentWithUndo(p, -1);
			}
		}
	}
	else if (comboBoxThatHasChanged == indexSelector)
	{
		int newIndex = -1;

		if (indexSelector->getSelectedId() != 1)
		{
			newIndex = indexSelector->getSelectedId() - 2;
			setContentWithUndo(connectedProcessor.get(), newIndex);
		}
		else
		{
			setConnectionIndex(newIndex);
			refreshContent();
		}
	}
}

void PanelWithProcessorConnection::refreshConnectionList()
{
	String currentId = connectionSelector->getText();

	connectionSelector->clear(dontSendNotification);

	StringArray items;

	fillModuleList(items);

	int index = items.indexOf(currentId);

	connectionSelector->addItem("Disconnect", 1);
	connectionSelector->addItemList(items, 2);

	if (index != -1)
	{
		connectionSelector->setSelectedId(index + 2, dontSendNotification);
	}
}

void PanelWithProcessorConnection::refreshIndexList()
{
	String currentId = indexSelector->getText();

	indexSelector->clear(dontSendNotification);

	StringArray items;

	fillIndexList(items);

	int index = items.indexOf(currentId);

	indexSelector->addItem("Disconnect", 1);
	indexSelector->addItemList(items, 2);

	if (index != -1)
	{
		indexSelector->setSelectedId(index + 2, dontSendNotification);
	}

	indexSelector->setEnabled(true);
}

ModulatorSynthChain* PanelWithProcessorConnection::getMainSynthChain()
{
	return rootWindow->getMainSynthChain();
}

const ModulatorSynthChain* PanelWithProcessorConnection::getMainSynthChain() const
{
	return rootWindow->getMainSynthChain();
}

void PanelWithProcessorConnection::setContentWithUndo(Processor* newProcessor, int newIndex)
{
	auto undoManager = rootWindow->getBackendProcessor()->getViewUndoManager();
	
	String undoText;

	StringArray indexes;
	fillIndexList(indexes);

	undoText << (currentProcessor.get() != nullptr ? currentProcessor->getId() : "Disconnected") << ": " << indexes[currentIndex] << " -> ";
	undoText << (newProcessor != nullptr ? newProcessor->getId() : "Disconnected") << ": " << indexes[newIndex] << " -> ";

	undoManager->beginNewTransaction(undoText);
	undoManager->perform(new ProcessorConnection(this, newProcessor, newIndex));
}


Component* CodeEditorPanel::createContentComponent(int index)
{
	auto p = dynamic_cast<JavascriptProcessor*>(getProcessor());

	auto pe = new PopupIncludeEditor(p, p->getSnippet(index)->getCallbackName());

	getProcessor()->getMainController()->setLastActiveEditor(pe->getEditor(), CodeDocument::Position());

	pe->getEditor()->grabKeyboardFocus();

	return pe;
}

void CodeEditorPanel::fillIndexList(StringArray& indexList)
{
	auto p = dynamic_cast<JavascriptProcessor*>(getConnectedProcessor());

	if (p != nullptr)
	{
		for (int i = 0; i < p->getNumSnippets(); i++)
		{
			indexList.add(p->getSnippet(i)->getCallbackName().toString());
		}
	}
}

Component* ScriptContentPanel::createContentComponent(int /*index*/)
{
	auto pwsc = dynamic_cast<ProcessorWithScriptingContent*>(getConnectedProcessor());

	return new ScriptContentComponent(pwsc);
}

Component* ScriptWatchTablePanel::createContentComponent(int /*index*/)
{
	auto swt = new ScriptWatchTable(getRootWindow());

	swt->setScriptProcessor(dynamic_cast<JavascriptProcessor*>(getConnectedProcessor()), nullptr);

	return swt;
}
