
// ====================================================================================================================

PatchBrowser::PatchBrowser(BaseDebugArea *area, BackendProcessorEditor *editor_) :
SearchableListComponent(area),
editor(editor_),
showChains(true)
{
	setName("Patch Browser");

	setShowEmptyCollections(true);

	editor->getModuleListNofifier().addChangeListener(this);

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
	editor->getModuleListNofifier().removeChangeListener(this);

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


			BetterProcessorEditorContainer *rootContainer = findParentComponentOfClass<BackendProcessorEditor>()->getRootContainer();

			jassert(rootContainer != nullptr);

			BetterProcessorEditor *editorOfParent = nullptr;
			BetterProcessorEditor *editorOfChain = nullptr;

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
	Processor::Iterator<ModulatorSynth> iter(editor.getComponent()->getMainSynthChain());

	ModulatorSynth *synth = nullptr;

	int i = 0;

	while ((synth = iter.getNextProcessor()))
	{
		i++;
	}

	return i;
}

SearchableListComponent::Collection * PatchBrowser::createCollection(int index)
{
	Processor::Iterator<ModulatorSynth> iter(editor.getComponent()->getMainSynthChain(), true);

	ModulatorSynth *synth = nullptr;

	Array<ModulatorSynth*> synths;
	Array<int> hierarchies;

	while ((synth = iter.getNextProcessor()))
	{
		synths.add(synth);
		hierarchies.add(iter.getHierarchyForCurrentProcessor());
	}

	jassert(index < synths.size());

	return new PatchCollection(synths[index], hierarchies[index], showChains);

}

void PatchBrowser::paint(Graphics &g)
{
	g.fillAll(Colour(0xff636363));
	
	const Colour shadowColour = Colour(0x22000000);

	g.setGradientFill(ColourGradient(shadowColour,
		0.0f, 0.0f,
		Colour(0),
		4.0f, 0.0f,
		false));
	g.fillRect(0, 0, 4, getHeight());

	g.setGradientFill(ColourGradient(shadowColour,
		(float)getWidth(), 0.0f,
		Colour(0),
		(float)getWidth() - 4.0f, 0.0f,
		false));
	g.fillRect(getWidth() - 4, 0, 4, getHeight());


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

		g.setColour(Colour(0xFF333333));

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

	AutoPopupDebugComponent *c = findParentComponentOfClass<BackendProcessorEditor>()->getDebugComponent(true, CombinedDebugArea::AreaIds::ProcessorCollection);

	if (c != nullptr)
	{
		c->showComponentInDebugArea(showChains);
	}

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
    
    startTimer(3000);
}



void PatchBrowser::ModuleDragTarget::timerCallback()
{
	if (getProcessor() == nullptr || !dynamic_cast<Component*>(this)->isVisible()) return;

	bool repaintFlag = false;

	if (getProcessor()->isBypassed() != bypassed)
	{
		bypassed = getProcessor()->isBypassed();
		repaintFlag = true;
	}
	else if (getProcessor()->getId() != id)
	{
		id = getProcessor()->getId();
		repaintFlag = true;
	}
	else if (getProcessor()->getColour() != colour)
	{
		colour = getProcessor()->getColour();
	}

	if (repaintFlag) dynamic_cast<Component*>(this)->repaint();
}

void PatchBrowser::ModuleDragTarget::buttonClicked(Button *b)
{
	BackendProcessorEditor *editor = dynamic_cast<Component*>(this)->findParentComponentOfClass<BackendProcessorEditor>();

	if (b == soloButton)
	{
		const bool isSolo = getProcessor()->getEditorState(Processor::EditorState::Solo);

		if (!isSolo) editor->addProcessorToPanel(getProcessor());
		else		 editor->removeProcessorFromPanel(getProcessor());

		refreshButtonState(soloButton, !isSolo);
	}

	if (b == hideButton)
	{
		const bool isHidden = getProcessor()->getEditorState(Processor::EditorState::Visible);

		getProcessor()->setEditorState(Processor::Visible, !isHidden, sendNotification);
		getProcessor()->sendChangeMessage();

		editor->getRootContainer()->refreshSize(false);
		
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
	}

	g.fillRoundedRectangle(area, 2.0f);

}

// ====================================================================================================================

PatchBrowser::PatchCollection::PatchCollection(ModulatorSynth *synth, int hierarchy_, bool showChains) :
root(synth),
hierarchy(hierarchy_)
{
	addAndMakeVisible(foldButton = new ShapeButton("Fold Overview", Colour(0xFF333333), Colours::white.withAlpha(0.4f), Colour(0xFF333333)));

	foldButton->addListener(this);

	refreshFoldButton();

	Processor::Iterator<Processor> iter(synth, true);

	Processor *p = nullptr;

	p = iter.getNextProcessor(); // skip itself...

	while ((p = iter.getNextProcessor()))
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
		findParentComponentOfClass<BackendProcessorEditor>()->setRootProcessorWithUndo(root);
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

	const bool isRoot = findParentComponentOfClass<BackendProcessorEditor>()->getMainSynthChain()->getRootProcessor() == root;

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

	g.setColour(Colour(0xFF333333));

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
		const bool folded = getProcessor()->getEditorState(getProcessor()->getEditorStateForIndex(ModulatorSynth::OverviewFolded));

		getProcessor()->setEditorState(getProcessor()->getEditorStateForIndex(ModulatorSynth::OverviewFolded), !folded);

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
lastId(p->getId()),
hierarchy(hierarchy_)
{
	setSize(380 - 16, ITEM_HEIGHT);

	setUsePopupMenu(true);
	setRepaintsOnMouseActivity(true);
}

PatchBrowser::PatchItem::~PatchItem()
{
	
}

void PatchBrowser::PatchItem::mouseDoubleClick(const MouseEvent& )
{
	if (processor.get() != nullptr)
	{
		findParentComponentOfClass<BackendProcessorEditor>()->setRootProcessorWithUndo(processor);
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

	m.addSeparator();

	const bool isRoot = findParentComponentOfClass<BackendProcessorEditor>()->getMainSynthChain()->getRootProcessor() == getProcessor();
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

	BackendProcessorEditor *editor = dynamic_cast<Component*>(this)->findParentComponentOfClass<BackendProcessorEditor>();

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
		editor->getRootContainer()->refreshSize(false);	
		break;
	case PatchBrowser::ModuleDragTarget::ViewSettings::Solo:
		getProcessor()->toggleEditorState(Processor::Solo, sendNotification);

		if (getProcessor()->getEditorState(Processor::EditorState::Solo))
			editor->addProcessorToPanel(getProcessor());
		else
			editor->removeProcessorFromPanel(getProcessor());
		break;
	case PatchBrowser::ModuleDragTarget::ViewSettings::Root:
		findParentComponentOfClass<BackendProcessorEditor>()->setRootProcessorWithUndo(processor);
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

		const bool isRoot = findParentComponentOfClass<BackendProcessorEditor>()->getMainSynthChain()->getRootProcessor() == processor;

		

		g.setGradientFill(ColourGradient(isRoot ? Colours::white.withAlpha(0.3f) : Colours::black.withAlpha(0.2f), 0.0f, 0.0f,
										 isRoot ? Colours::white.withAlpha(0.2f) : Colours::black.withAlpha(0.3f), 0.0f, (float)getHeight(), false));
		
		g.fillRoundedRectangle(1.0f + xOffset, 0.0f, (float)getWidth() - 2.0f - xOffset, (float)getHeight() - 2.0f, 2.0f);

		if (isMouseOver(true))
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
		g.setFont(GLOBAL_BOLD_FONT());
		g.drawText(processor->getId(), getHeight() + 4 + (int)xOffset, 0, getWidth() - (int)xOffset, getHeight(), Justification::centredLeft, false);
		lastId = processor->getId();

		drawDragStatus(g, Rectangle<float>(1.0f + xOffset, 0.0f, (float)getWidth() - 2.0f - xOffset, (float)getHeight() - 2.0f));

		if (getProcessor()->isBypassed())
		{
			g.setColour(Colours::black.withAlpha(0.2f));

			g.fillRoundedRectangle(1.0f + xOffset, 0.0f, (float)getWidth() - 2.0f - xOffset, (float)getHeight() - 2.0f, 2.0f);
		}
	}
}
