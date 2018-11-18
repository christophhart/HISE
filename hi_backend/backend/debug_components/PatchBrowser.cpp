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

// ====================================================================================================================

PatchBrowser::PatchBrowser(BackendRootWindow *window) :
SearchableListComponent(window),
editor(window->getMainPanel()),
rootWindow(window),
showChains(false)
{
	setName("Patch Browser");

	setShowEmptyCollections(true);

	window->getModuleListNofifier().addProcessorChangeListener(this);

	addAndMakeVisible(addButton = new ShapeButton("Show chains", Colours::white.withAlpha(0.6f), Colours::white, Colours::white));
	
	addButton->setTooltip("Show internal chains in list");
	Path addPath;
	addPath.loadPathFromData(HiBinaryData::ProcessorEditorHeaderIcons::addIcon, sizeof(HiBinaryData::ProcessorEditorHeaderIcons::addIcon));
	addButton->setShape(addPath, true, true, false);
	addButton->addListener(this);

	addCustomButton(addButton);
	
	
	addAndMakeVisible(foldButton = new ShapeButton("Fold all", Colours::white.withAlpha(0.6f), Colours::white, Colours::white));

	Path foldPath;
	foldPath.loadPathFromData(HiBinaryData::ProcessorEditorHeaderIcons::foldedIcon, sizeof(HiBinaryData::ProcessorEditorHeaderIcons::foldedIcon));
	foldButton->setShape(foldPath, false, true, false);
	foldButton->setTooltip("Fold all sound generators");

	foldButton->addListener(this);

	addCustomButton(foldButton);

	setOpaque(true);
}

PatchBrowser::~PatchBrowser()
{
	if(rootWindow != nullptr)
		rootWindow->getModuleListNofifier().removeProcessorChangeListener(this);

	addButton = nullptr;
}



bool PatchBrowser::isInterestedInDragSource(const SourceDetails& dragSourceDetails)
{
	return !dragSourceDetails.description.isUndefined();
}

void PatchBrowser::itemDragEnter(const SourceDetails& dragSourceDetails)
{
	for (int i = 0; i < getNumCollections(); i++)
	{
		dynamic_cast<PatchCollection*>(getCollection(i))->checkDragState(dragSourceDetails);
	}
}

void PatchBrowser::itemDragExit(const SourceDetails& /*dragSourceDetails*/)
{
	for (int i = 0; i < getNumCollections(); i++)
	{
		dynamic_cast<PatchCollection*>(getCollection(i))->resetDragState();
	}
}

void PatchBrowser::itemDragMove(const SourceDetails& dragSourceDetails)
{
	ModuleDragTarget *dragTarget = dynamic_cast<ModuleDragTarget*>(getComponentAt(dragSourceDetails.localPosition));

	if (dragTarget == nullptr) return;

	if (lastTarget != nullptr && dynamic_cast<ModuleDragTarget*>(lastTarget.getComponent()) != dragTarget)
	{
		dynamic_cast<ModuleDragTarget*>(lastTarget.getComponent())->setDraggingOver(false);
	}

	dragTarget->setDraggingOver(true);
	lastTarget = dynamic_cast<Component*>(dragTarget);
}

void PatchBrowser::itemDropped(const SourceDetails& dragSourceDetails)
{
	ModuleDragTarget *dragTarget = dynamic_cast<ModuleDragTarget*>(getComponentAt(dragSourceDetails.localPosition));

	if (dragTarget != nullptr && dragTarget->getDragState() == ModuleDragTarget::DragState::Allowed)
	{
		Chain *c = dynamic_cast<Chain*>(dragTarget->getProcessor());

		if (c != nullptr)
		{
			Identifier type = Identifier(dragSourceDetails.description.toString().upToFirstOccurrenceOf("::", false, true));
			String name = dragSourceDetails.description.toString().fromLastOccurrenceOf("::", false, true);

			Processor *newProcessor = MainController::createProcessor(c->getFactoryType(), type, name);

			c->getHandler()->add(newProcessor, nullptr);

			dynamic_cast<Processor*>(c)->setEditorState(Processor::EditorState::Visible, true, sendNotification);


			ProcessorEditorContainer *rootContainer = GET_BACKEND_ROOT_WINDOW(this)->getMainPanel()->getRootContainer();

			jassert(rootContainer != nullptr);

			ProcessorEditor *editorOfParent = nullptr;
			ProcessorEditor *editorOfChain = nullptr;

			if (ProcessorHelpers::is<ModulatorSynth>(dragTarget->getProcessor()))
			{
				editorOfParent = rootContainer->getFirstEditorOf(dragTarget->getProcessor());
				editorOfChain = editorOfParent;
			}
			else
			{
				editorOfParent = rootContainer->getFirstEditorOf(ProcessorHelpers::findParentProcessor(dragTarget->getProcessor(), true));
				editorOfChain = rootContainer->getFirstEditorOf(dragTarget->getProcessor());
			}


			if (editorOfParent != nullptr)
			{
				editorOfParent->getChainBar()->refreshPanel();
				editorOfParent->sendResizedMessage();
				editorOfChain->changeListenerCallback(editorOfChain->getProcessor());
				editorOfChain->childEditorAmountChanged();
			}

			GET_BACKEND_ROOT_WINDOW(this)->sendRootContainerRebuildMessage(false);
		}
	}

	for (int i = 0; i < getNumCollections(); i++)
	{
		dynamic_cast<ModuleDragTarget*>(getCollection(i))->resetDragState();
	}

	rebuildModuleList(true);
}


int PatchBrowser::getNumCollectionsToCreate() const
{
    Processor::Iterator<ModulatorSynth> iter(rootWindow.getComponent()->getMainSynthChain());

	int i = 0;

	while (iter.getNextProcessor())
	{
		i++;
	}

	return i;
}

SearchableListComponent::Collection * PatchBrowser::createCollection(int index)
{
	Processor::Iterator<ModulatorSynth> iter(rootWindow.getComponent()->getMainSynthChain(), true);

	Array<ModulatorSynth*> synths;
	Array<int> hierarchies;

	while (ModulatorSynth *synth = iter.getNextProcessor())
	{
		synths.add(synth);
		hierarchies.add(iter.getHierarchyForCurrentProcessor());
	}

	jassert(index < synths.size());

	return new PatchCollection(synths[index], hierarchies[index], showChains);

}

void PatchBrowser::paint(Graphics &g)
{
	g.fillAll(Colour(0xff383838));

	SearchableListComponent::paint(g);

	Point<int> startPointInParent;

	int numCollections = getNumCollections();

	for (int i = 1; i < numCollections; i++) // skip the Master Chain
	{
		PatchCollection *c = dynamic_cast<PatchCollection*>(getCollection(i));

		if (!c->hasVisibleItems()) continue;

		Processor *p = c->getProcessor();

		if (p == nullptr) return;

		Processor *root = ProcessorHelpers::findParentProcessor(p, true);

		for (int j = 0; j < numCollections; j++)
		{
			PatchCollection *possibleRootCollection = dynamic_cast<PatchCollection*>(getCollection(j));

			if (possibleRootCollection->getProcessor() == root)
			{
				Point<int> startPoint = possibleRootCollection->getPointForTreeGraph(true);
				startPointInParent = getLocalPoint(possibleRootCollection, startPoint);
				break;
			}
		}

		Point<int> endPoint = c->getPointForTreeGraph(false);
		Point<int> endPointInParent = getLocalPoint(c, endPoint);

		g.setColour(Colour(0xFF222222));

		g.drawLine((float)startPointInParent.getX(), (float)startPointInParent.getY(), (float)startPointInParent.getX(), (float)endPointInParent.getY(), 2.0f);
		g.drawLine((float)startPointInParent.getX(), (float)endPointInParent.getY(), (float)endPointInParent.getX(), (float)endPointInParent.getY(), 2.0f);

	}
}

void PatchBrowser::toggleFoldAll()
{
	foldAll = true;

	for (int i = 0; i < getNumCollections(); i++)
	{
		getCollection(i)->setFolded(foldAll);
		dynamic_cast<PatchCollection*>(getCollection(i))->getProcessor()->setEditorState(ModulatorSynth::OverviewFolded, foldAll, sendNotification);
		dynamic_cast<PatchCollection*>(getCollection(i))->refreshFoldButton();
	}

	refreshDisplayedItems();
}

void PatchBrowser::toggleShowChains()
{
	showChains = !showChains;

	addButton->setColours(showChains ? Colours::white : Colours::white.withAlpha(0.6f), Colours::white, Colours::white);
	addButton->setToggleState(showChains, dontSendNotification);

	rebuildModuleList(true);
}

void PatchBrowser::buttonClicked(Button *b)
{
	if (b == foldButton)
	{
		toggleFoldAll();
	}
	else if (b == addButton)
	{
		toggleShowChains();
	}
}

// ====================================================================================================================

PatchBrowser::ModuleDragTarget::ModuleDragTarget() :
dragState(DragState::Inactive),
isOver(false)
{
	soloButton = new ShapeButton("Solo Processor", Colours::white.withAlpha(0.2f), Colours::white.withAlpha(0.5f), Colours::white);

	static Path soloPath;

	soloPath.loadPathFromData(BackendBinaryData::PopupSymbols::soloShape, sizeof(BackendBinaryData::PopupSymbols::soloShape));
	soloButton->setShape(soloPath, false, true, false);
	soloButton->addListener(this);

	hideButton = new ShapeButton("Hide Processor", Colours::white.withAlpha(0.2f), Colours::white.withAlpha(0.5f), Colours::white);

	static Path hidePath;

	hidePath.loadPathFromData(BackendBinaryData::ToolbarIcons::viewPanel, sizeof(BackendBinaryData::ToolbarIcons::viewPanel));
	hideButton->setShape(hidePath, false, true, false);
	hideButton->addListener(this);
    
}

void PatchBrowser::ModuleDragTarget::buttonClicked(Button *b)
{
	auto *mainEditor = GET_BACKEND_ROOT_WINDOW(dynamic_cast<Component*>(this))->getMainPanel();

	if (b == soloButton)
	{
		const bool isSolo = getProcessor()->getEditorState(Processor::EditorState::Solo);

		if (!isSolo) mainEditor->addProcessorToPanel(getProcessor());
		else		 mainEditor->removeProcessorFromPanel(getProcessor());

		refreshButtonState(soloButton, !isSolo);
	}

	if (b == hideButton)
	{
		const bool isHidden = getProcessor()->getEditorState(Processor::EditorState::Visible);

		getProcessor()->setEditorState(Processor::Visible, !isHidden, sendNotification);
		getProcessor()->sendChangeMessage();

		mainEditor->getRootContainer()->refreshSize(false);
		
		refreshButtonState(hideButton, !isHidden);
	}
}

void PatchBrowser::ModuleDragTarget::refreshAllButtonStates()
{
	refreshButtonState(soloButton, getProcessor()->getEditorState(Processor::EditorState::Solo));
	refreshButtonState(hideButton, getProcessor()->getEditorState(Processor::EditorState::Visible));
}

void PatchBrowser::ModuleDragTarget::refreshButtonState(ShapeButton *button, bool on)
{
	if (on)
	{
		button->setColours(Colours::white.withAlpha(0.7f), Colours::white, Colours::white);
	}
	else
	{
		button->setColours(Colours::black.withAlpha(0.2f), Colours::white.withAlpha(0.5f), Colours::white);
	}
}

void PatchBrowser::ModuleDragTarget::setDraggingOver(bool shouldBeOver)
{
	isOver = shouldBeOver;
	
	dynamic_cast<Component*>(this)->repaint();
}

void PatchBrowser::ModuleDragTarget::checkDragState(const SourceDetails& dragSourceDetails)
{
	if (ProcessorHelpers::is<Chain>(getProcessor()))
	{
		Identifier t = Identifier(dragSourceDetails.description.toString().upToFirstOccurrenceOf("::", false, true));

		const bool allowed = dynamic_cast<const Chain*>(getProcessor())->getFactoryType()->allowType(t);

		setDragState(allowed ? DragState::Allowed : DragState::Forbidden);

		dynamic_cast<Component*>(this)->repaint();
	}
	else
	{
		setDragState(ModuleDragTarget::DragState::Forbidden);
		dynamic_cast<Component*>(this)->repaint();
	}
}

void PatchBrowser::ModuleDragTarget::resetDragState()
{
	dragState = DragState::Inactive;
	dynamic_cast<Component*>(this)->repaint();
}

// ====================================================================================================================

void PatchBrowser::ModuleDragTarget::drawDragStatus(Graphics &g, Rectangle<float> area)
{
	switch (getDragState())
	{
	case ModuleDragTarget::DragState::Allowed:
		g.setColour(Colours::green.withMultipliedBrightness(isOver ? 1.5f : 0.6f).withAlpha(0.3f)); break;
	case ModuleDragTarget::DragState::Forbidden:
		g.setColour(Colours::red.withMultipliedBrightness(isOver ? 1.5f : 0.6f).withAlpha(0.3f)); break;
	case ModuleDragTarget::DragState::Inactive:
		g.setColour(Colours::transparentBlack); break;
    case ModuleDragTarget::DragState::numDragStates: break;
	}

	g.fillRoundedRectangle(area, 2.0f);

}

// ====================================================================================================================

PatchBrowser::PatchCollection::PatchCollection(ModulatorSynth *synth, int hierarchy_, bool showChains) :
root(synth),
hierarchy(hierarchy_)
{
	addAndMakeVisible(foldButton = new ShapeButton("Fold Overview", Colour(0xFF222222), Colours::white.withAlpha(0.4f), Colour(0xFF222222)));

	foldButton->addListener(this);

	refreshFoldButton();

	Processor::Iterator<Processor> iter(synth, true);

	iter.getNextProcessor(); // skip itself...

	while (Processor *p = iter.getNextProcessor())
	{
		if (ProcessorHelpers::is<ModulatorSynth>(p)) break;

		ModulatorSynth *parentSynth = dynamic_cast<ModulatorSynth*>(ProcessorHelpers::findParentProcessor(p, true));

		if (ProcessorHelpers::is<Chain>(p))
		{
			if (!showChains) continue;


			bool skip = false;

			for (int i = 0; i < parentSynth->getNumInternalChains(); i++)
			{
				if (parentSynth->getChildProcessor(i) == p && parentSynth->isChainDisabled((ModulatorSynth::InternalChains)i))
				{
					skip = true;
					break;
				}
			}

			if (skip) continue;
		}

		String searchTerm;

		if (p != nullptr)
		{
			searchTerm << p->getId() << " " << p->getType();
		}

		if (parentSynth != nullptr)
		{
			searchTerm << parentSynth->getId() << " " << parentSynth->getType();
		}

		items.add(new PatchItem(p, parentSynth, iter.getHierarchyForCurrentProcessor(), searchTerm));
		addAndMakeVisible(items.getLast());
	}

	setRepaintsOnMouseActivity(true);
}


PatchBrowser::PatchCollection::~PatchCollection()
{
}


void PatchBrowser::PatchCollection::mouseDoubleClick(const MouseEvent& )
{
	if (getProcessor() != nullptr)
	{
		GET_BACKEND_ROOT_WINDOW(this)->getMainPanel()->setRootProcessorWithUndo(root);
		findParentComponentOfClass<SearchableListComponent>()->repaint();
	}
}

void PatchBrowser::PatchCollection::paint(Graphics &g)
{
	if (getProcessor() == nullptr) return;

	ModulatorSynth *synth = dynamic_cast<ModulatorSynth*>(root.get());

	float xOffset = getIntendation();

	if (root->isBypassed())
	{
		g.setOpacity(0.4f);
	}

	const bool isRoot = GET_BACKEND_ROOT_WINDOW(this)->getMainSynthChain()->getRootProcessor() == root;

	g.setFont(GLOBAL_BOLD_FONT().withHeight(16.0f));

	if (isMouseOver(false))
	{
		g.setColour(Colours::white.withAlpha(0.2f));
		g.fillRoundedRectangle(xOffset + 8.0f, 8.0f, (float)getWidth() - 8.0f - xOffset, 24.0f, 2.0f);
	}

	if (isRoot)
	{
		g.setColour(Colours::white.withAlpha(0.2f));
		g.fillRoundedRectangle(xOffset + 8.0f, 8.0f, (float)getWidth() - 8.0f - xOffset, 24.0f, 2.0f);
	}

	g.setGradientFill(ColourGradient(synth->getIconColour().withMultipliedBrightness(1.1f), 0.0f, 7.0f,
		synth->getIconColour().withMultipliedBrightness(0.9f), 0.0f, 35.0f, false));

	g.fillRoundedRectangle(xOffset + 7.0f, 7.0f, 26.0f, 26.0f, 2.0f);

	g.setColour(Colour(0xFF222222));

	g.drawRoundedRectangle(xOffset + 7.0f, 7.0f, 26.0f, 26.0f, 2.0f, 2.0f);

	g.setColour(isMouseOver() ? Colours::white : Colours::white.withAlpha(0.7f));

	g.drawText(root->getId(), (int)xOffset + 40, 10, getWidth(), 20, Justification::centredLeft, false);

	drawDragStatus(g, Rectangle<float>(xOffset + 7.0f, 7.0f, (float)getWidth() - 7.0f - xOffset, 26.0f));
}


void PatchBrowser::PatchCollection::refreshFoldButton()
{
	Path foldShape;
	foldShape.loadPathFromData(HiBinaryData::ProcessorEditorHeaderIcons::foldedIcon, sizeof(HiBinaryData::ProcessorEditorHeaderIcons::foldedIcon));

	setFolded(getProcessor()->getEditorState(getProcessor()->getEditorStateForIndex(ModulatorSynth::OverviewFolded)));

	if (!isFolded()) foldShape.applyTransform(AffineTransform::rotation(3.147f / 2.0f));

	foldButton->setShape(foldShape, true, true, false);

	SearchableListComponent *p = findParentComponentOfClass<SearchableListComponent>();

	if (p != nullptr)
	{
		p->refreshDisplayedItems();
	}

	resized();
}


void PatchBrowser::PatchCollection::resized()
{
	SearchableListComponent::Collection::resized();

	if (getProcessor() == getProcessor()->getMainController()->getMainSynthChain())
	{
		foldButton->setBounds(15, 15, 10, 10);
	}
	else
	{
		foldButton->setBounds((int)getIntendation() - 14, 15, 10, 10);
	}
}

void PatchBrowser::PatchCollection::buttonClicked(Button *b)
{
	if (b == foldButton)
	{
		const bool wasFolded = getProcessor()->getEditorState(getProcessor()->getEditorStateForIndex(ModulatorSynth::OverviewFolded));

		getProcessor()->setEditorState(getProcessor()->getEditorStateForIndex(ModulatorSynth::OverviewFolded), !wasFolded);

		refreshFoldButton();
	}
	else ModuleDragTarget::buttonClicked(b);
	
}

Point<int> PatchBrowser::PatchCollection::getPointForTreeGraph(bool getStartPoint) const
{
	return getStartPoint ? Point<int>((int)getIntendation() + 20, 32) : Point<int>((int)getIntendation() + 7, 20);
}

void PatchBrowser::PatchCollection::checkDragState(const SourceDetails& dragSourceDetails)
{
	ModuleDragTarget::checkDragState(dragSourceDetails);

	for (int i = 0; i < items.size(); i++)
	{
		dynamic_cast<ModuleDragTarget*>(items[i])->checkDragState(dragSourceDetails);
	}
}

void PatchBrowser::PatchCollection::resetDragState()
{
	ModuleDragTarget::resetDragState();

	for (int i = 0; i < items.size(); i++)
	{
		dynamic_cast<ModuleDragTarget*>(items[i])->resetDragState();
	}
}

void PatchBrowser::PatchCollection::toggleShowChains()
{
}

// ====================================================================================================================

PatchBrowser::PatchItem::PatchItem(Processor *p, Processor *parent_, int hierarchy_, const String &searchTerm) :
Item(searchTerm.toLowerCase()),
processor(p),
parent(parent_),
lastId(String()),
hierarchy(hierarchy_),
lastMouseDown(0)
{
    addAndMakeVisible(idLabel = new Label());
    
    //idLabel->setEditable(false);
    //idLabel->addMouseListener(this, true);
    
	idLabel->setInterceptsMouseClicks(false, true);

    idLabel->setColour(Label::ColourIds::textColourId, Colours::white);
	idLabel->setColour(Label::ColourIds::textWhenEditingColourId, Colours::white);
	idLabel->setColour(Label::ColourIds::outlineWhenEditingColourId, Colour(SIGNAL_COLOUR));
	idLabel->setColour(TextEditor::ColourIds::highlightColourId, Colour(SIGNAL_COLOUR));
	idLabel->setColour(TextEditor::ColourIds::highlightedTextColourId, Colours::black);
	idLabel->setColour(CaretComponent::ColourIds::caretColourId, Colours::white);

	setSize(380 - 16, ITEM_HEIGHT);

	setUsePopupMenu(true);
	setRepaintsOnMouseActivity(true);
    
    idLabel->addListener(this);
}

PatchBrowser::PatchItem::~PatchItem()
{
	
}

void PatchBrowser::PatchItem::mouseDoubleClick(const MouseEvent& )
{
	if (processor.get() != nullptr)
	{
		GET_BACKEND_ROOT_WINDOW(this)->getMainPanel()->setRootProcessorWithUndo(processor);
		findParentComponentOfClass<SearchableListComponent>()->repaint();
	}
}


void PatchBrowser::PatchItem::fillPopupMenu(PopupMenu &m)
{
	repaint();

	m.addSectionHeader(getProcessor()->getId());

	m.addItem((int)ModuleDragTarget::ViewSettings::Bypassed, "Bypass module", true, getProcessor()->isBypassed());
	m.addItem((int)ModuleDragTarget::ViewSettings::Copy, "Copy module to clipboard", true, false);
	m.addItem((int)ModuleDragTarget::ViewSettings::CreateScriptVariableDeclaration, "Create script variable declaration", true, false);

	if (Chain *c = dynamic_cast<Chain*>(getProcessor()))
	{
		FactoryType* t = c->getFactoryType();

		String clipBoardName = PresetHandler::getProcessorNameFromClipboard(t);

		if (clipBoardName != String())
		{
			m.addItem((int)ModuleDragTarget::ViewSettings::PasteProcessorFromClipboard, "Paste " + clipBoardName + " from clipboard", true, false);
		}
	}

	m.addSeparator();

	const bool isRoot = GET_BACKEND_ROOT_WINDOW(this)->getMainSynthChain()->getRootProcessor() == getProcessor();
	m.addItem((int)ModuleDragTarget::ViewSettings::Root, "Set Fullscreen", true, isRoot);
	m.addItem((int)ModuleDragTarget::ViewSettings::Visible, "Show module", true, getProcessor()->getEditorState(Processor::Visible));
	m.addItem((int)ModuleDragTarget::ViewSettings::Solo, "Add module to Root", true, getProcessor()->getEditorState(Processor::Solo));

	m.addSeparator();
	
	m.addSectionHeader("Global settings");

	m.addItem((int)ModuleDragTarget::ViewSettings::ToggleFoldAll, "Fold all", true, false);
	m.addItem((int)ModuleDragTarget::ViewSettings::ShowChains, "Show chains in list", true, false);
}


void PatchBrowser::PatchItem::popupCallback(int menuIndex)
{
	ViewSettings setting = (ViewSettings)menuIndex;

	auto *mainEditor = GET_BACKEND_ROOT_WINDOW(dynamic_cast<Component*>(this))->getMainPanel();

	switch (setting)
	{
	case PatchBrowser::ModuleDragTarget::ViewSettings::ToggleFoldAll:
		findParentComponentOfClass<PatchBrowser>()->toggleFoldAll();
		break;
	case PatchBrowser::ModuleDragTarget::ViewSettings::ShowChains:
		findParentComponentOfClass<PatchBrowser>()->toggleShowChains();
		break;
	case PatchBrowser::ModuleDragTarget::ViewSettings::Visible:
		getProcessor()->toggleEditorState(Processor::Visible, sendNotification);
		getProcessor()->sendChangeMessage();
		mainEditor->getRootContainer()->refreshSize(false);	
		break;
	case PatchBrowser::ModuleDragTarget::ViewSettings::Solo:
		getProcessor()->toggleEditorState(Processor::Solo, sendNotification);

		if (getProcessor()->getEditorState(Processor::EditorState::Solo))
			mainEditor->addProcessorToPanel(getProcessor());
		else
			mainEditor->removeProcessorFromPanel(getProcessor());
		break;
	case PatchBrowser::ModuleDragTarget::ViewSettings::Root:
		mainEditor->setRootProcessorWithUndo(processor);
		findParentComponentOfClass<SearchableListComponent>()->repaint();
		break;
	case PatchBrowser::ModuleDragTarget::ViewSettings::Bypassed:
		getProcessor()->setBypassed(!getProcessor()->isBypassed());
		getProcessor()->sendChangeMessage();
		break;
	case PatchBrowser::ModuleDragTarget::ViewSettings::Copy:
		PresetHandler::copyProcessorToClipboard(getProcessor());
		break;
	case PatchBrowser::ModuleDragTarget::ViewSettings::CreateScriptVariableDeclaration:
		ProcessorHelpers::getScriptVariableDeclaration(getProcessor(), true);
		break;
	case PatchBrowser::ModuleDragTarget::ViewSettings::PasteProcessorFromClipboard:
	{
		Chain* c = dynamic_cast<Chain*>(getProcessor());

		Processor* newProcessor = PresetHandler::createProcessorFromClipBoard(getProcessor());

		c->getHandler()->add(newProcessor, nullptr);

		ProcessorEditorContainer *rootContainer = mainEditor->getRootContainer();

		jassert(rootContainer != nullptr);

		ProcessorEditor *editorOfParent = nullptr;
		ProcessorEditor *editorOfChain = nullptr;

		if (ProcessorHelpers::is<ModulatorSynth>(getProcessor()))
		{
			editorOfParent = rootContainer->getFirstEditorOf(getProcessor());
			editorOfChain = editorOfParent;
		}
		else
		{
			editorOfParent = rootContainer->getFirstEditorOf(ProcessorHelpers::findParentProcessor(getProcessor(), true));
			editorOfChain = rootContainer->getFirstEditorOf(getProcessor());
		}


		if (editorOfParent != nullptr)
		{
			editorOfParent->getChainBar()->refreshPanel();
			editorOfParent->sendResizedMessage();
			editorOfChain->changeListenerCallback(editorOfChain->getProcessor());
			editorOfChain->childEditorAmountChanged();
		}

		findParentComponentOfClass<PatchBrowser>()->rebuildModuleList(true);

		break;
	}
	case PatchBrowser::ModuleDragTarget::ViewSettings::numViewSettings:
		break;
	default:
		break;
	}

	repaint();
}



void PatchBrowser::PatchItem::paint(Graphics& g)
{
	if (processor.get() != nullptr)
	{
		float xOffset = (float)hierarchy * 10.0f + 10.0f;

		

		xOffset += findParentComponentOfClass<PatchCollection>()->getIntendation();

		const bool isRoot = GET_BACKEND_ROOT_WINDOW(this)->getMainSynthChain()->getRootProcessor() == processor;

		

		g.setGradientFill(ColourGradient(isRoot ? Colours::white.withAlpha(0.3f) : Colours::black.withAlpha(0.2f), 0.0f, 0.0f,
										 isRoot ? Colours::white.withAlpha(0.2f) : Colours::black.withAlpha(0.3f), 0.0f, (float)getHeight(), false));
		
		g.fillRoundedRectangle(1.0f + xOffset, 0.0f, (float)getWidth() - 2.0f - xOffset, (float)getHeight() - 2.0f, 2.0f);


        
		if (isMouseOver(true) || idLabel->isMouseOver(true))
		{
			g.setGradientFill(ColourGradient(Colours::white.withAlpha(0.3f), 0.0f, 0.0f,
											 Colours::white.withAlpha(0.2f), 0.0f, (float)getHeight(), false));

			g.fillRoundedRectangle(1.0f + xOffset, 0.0f, (float)getWidth() - 2.0f - xOffset, (float)getHeight() - 2.0f, 2.0f);
		}
		
		g.setColour(processor->getColour());

		g.fillRoundedRectangle(1.0f + xOffset, 1.0f, (float)getHeight() - 4.0f, (float)getHeight() - 4.0f, 2.0f);

		g.setColour(Colour(0xFF222222));

		g.drawRoundedRectangle(1.0f + xOffset, 1.0f, (float)getHeight() - 4.0f, (float)getHeight() - 4.0f, 2.0f, 2.0f);

		g.setColour(ProcessorHelpers::is<Chain>(processor) ? Colours::black.withAlpha(0.6f) : Colours::black);
		
        if(lastId != processor->getId())
        {
            idLabel->setFont(GLOBAL_BOLD_FONT());
            idLabel->setJustificationType(Justification::centredLeft);
            idLabel->setBounds(getHeight() + 4 + (int)xOffset, 0, getWidth() - (int)xOffset, getHeight());
            
            lastId = processor->getId();
            idLabel->setText(lastId, dontSendNotification);
        }
        
		drawDragStatus(g, Rectangle<float>(1.0f + xOffset, 0.0f, (float)getWidth() - 2.0f - xOffset, (float)getHeight() - 2.0f));

		if (getProcessor()->isBypassed())
		{
			g.setColour(Colours::black.withAlpha(0.2f));

			g.fillRoundedRectangle(1.0f + xOffset, 0.0f, (float)getWidth() - 2.0f - xOffset, (float)getHeight() - 2.0f, 2.0f);
		}
	}
}

} // namespace hise