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

    Factory f;
    
    addAndMakeVisible(addButton = new HiseShapeButton("add", this, f));
    addButton->setToggleModeWithColourChange(true);
	addButton->setTooltip("Edit Module Tree");
    addButton->setToggleStateAndUpdateIcon(false);
	
	addCustomButton(addButton);
	
	
#if 0
	addAndMakeVisible(foldButton = new ShapeButton("Fold all", Colours::white.withAlpha(0.6f), Colours::white, Colours::white));

	Path foldPath;
	foldPath.loadPathFromData(HiBinaryData::ProcessorEditorHeaderIcons::foldedIcon, sizeof(HiBinaryData::ProcessorEditorHeaderIcons::foldedIcon));
	foldButton->setShape(foldPath, false, true, false);
	foldButton->setTooltip("Fold all sound generators");

	foldButton->addListener(this);

	addCustomButton(foldButton);
#endif

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



void PatchBrowser::refreshBypassState()
{
	Component::callRecursive<Component>(this, [](Component* c)
	{
		auto somethingBypassed = false;

		Processor* p = nullptr;

		if (auto pc = dynamic_cast<PatchCollection*>(c))
			p = pc->getProcessor();
		else if (auto pi = dynamic_cast<PatchItem*>(c))
			p = pi->getProcessor();
		else
			return false;
		
		somethingBypassed |= p->isBypassed();

		while (!somethingBypassed && p != nullptr)
		{
			somethingBypassed |= p->isBypassed();
			p = p->getParentProcessor(false);
		}

		dynamic_cast<ModuleDragTarget*>(c)->bypassed = somethingBypassed;
		c->repaint();

		return false;
	}, true);
}

void PatchBrowser::processorChanged(PatchBrowser& pb, Processor* oldProcessor, Processor* newProcessor)
{
	pb.popupProcessors.removeAllInstancesOf(oldProcessor);
	pb.popupProcessors.addIfNotAlreadyThere(newProcessor);

	pb.refreshPopupState();

}

void PatchBrowser::showProcessorInPopup(Component* c, const MouseEvent& e, Processor* p)
{
	auto bp = c->findParentComponentOfClass<PatchBrowser>();
	auto ft = GET_BACKEND_ROOT_WINDOW(c)->getRootFloatingTile();

	auto shouldShow = !bp->popupProcessors.contains(p);

	if (shouldShow)
	{
		if (!e.mods.isCommandDown())
		{
			ft->forEachDetachedPopup([p](FloatingTilePopup* ftp)
				{
					if (auto pe = ftp->getContent<ProcessorEditorContainer>())
					{
						ftp->deleteAndClose();
					}
				});
		}

		auto b = c->getLocalBounds();
		b = bp->getLocalArea(c, b);
		auto pe = dynamic_cast<ProcessorEditorContainer*>(DebugableObject::Helpers::showProcessorEditorPopup(e, c, p));
		
		Component::SafePointer<FloatingTilePopup> safePopup = ft->showComponentAsDetachedPopup(pe, bp, { b.getCentreX(), b.getY() + 30 }, true);

		pe->rootBroadcaster.addListener(*bp, PatchBrowser::processorChanged);

		auto newC = new BreadcrumbComponent(pe);
		newC->setSize(100, 28);

		safePopup->addFixComponent(newC);

		dynamic_cast<ProcessorEditorContainer*>(pe)->deleteCallback = [safePopup]()
		{
			if (safePopup.getComponent())
				safePopup->deleteAndClose();
		};
	}
	else
	{
		ft->forEachDetachedPopup([p](FloatingTilePopup* ftp)
		{
			if (auto pe = ftp->getContent<ProcessorEditorContainer>())
			{
				if(pe->getRootEditor()->getProcessor() == p)
					ftp->deleteAndClose();
			}
		});
	}
}

void PatchBrowser::refreshPopupState()
{
	Component::callRecursive<PatchCollection>(this, [this](PatchCollection* pc)
	{
		pc->setInPopup(popupProcessors.contains(pc->getProcessor()));
		return false;
	});

	Component::callRecursive<PatchItem>(this, [this](PatchItem* item)
	{
		item->setInPopup(popupProcessors.contains(item->getProcessor()));
		return false;
	});
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
	SearchableListComponent::paint(g);
    
    auto b = getLocalBounds();
    b.removeFromTop(25);
    
    g.setColour(Colour(0xFF353535));
    g.fillRect(b);
    
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
    
    if(showChains)
    {
        Colour lineColour = Colours::white;

        UnblurryGraphics ug(g, *this, true);

        auto mulAlpha = 1.0f - jlimit(0.0f, 1.0f, (1.0f / 3.0f * ug.getPixelSize()));

        if (mulAlpha > 0.1f)
        {
            for (int x = 10; x < getWidth(); x += 10)
            {
                float alpha = (x % 100 == 0) ? 0.12f : 0.05f;
                alpha *= mulAlpha;
                g.setColour(lineColour.withAlpha(alpha));
                ug.draw1PxVerticalLine(x, b.getY(), (float)getHeight());
            }

            for (int y = b.getY() + 10; y < b.getBottom(); y += 10)
            {
                float alpha = (y % 100 == 0) ? 0.12f : 0.05f;
                alpha *= mulAlpha;
                g.setColour(lineColour.withAlpha(alpha));
                ug.draw1PxHorizontalLine(y, 0.0f, (float)getWidth());
            }
        }
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
	rebuildModuleList(true);
    repaint();
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

HiseShapeButton* PatchBrowser::skinWorkspaceButton(Processor* processor)
{
	if (processor != nullptr)
	{
		auto isWorkspaceTarget = dynamic_cast<JavascriptProcessor*>(processor) || dynamic_cast<ModulatorSampler*>(processor);

		if (!isWorkspaceTarget)
		{
            return nullptr;
			
		}

        Factory f;
        
        auto b = new HiseShapeButton("workspace", nullptr, f);
        
		
        b->setToggleModeWithColourChange(true);
		b->setTooltip("Open " + processor->getId() + " in workspace");

		WeakReference<Processor> safeP(processor);

		b->onClick = [safeP, b]()
		{
			auto rootWindow = GET_BACKEND_ROOT_WINDOW((b));
            
            rootWindow->gotoIfWorkspace(safeP);
		};
        
        return b;
	}
    
    return nullptr;
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
hierarchy(hierarchy_),
peak(synth)
{
	synth->addBypassListener(this);
	addAndMakeVisible(peak);
	addAndMakeVisible(foldButton = new ShapeButton("Fold Overview", Colour(0xFF222222), Colours::white.withAlpha(0.4f), Colour(0xFF222222)));

	setRepaintsOnMouseActivity(true);
    
	gotoWorkspace = PatchBrowser::skinWorkspaceButton(synth);

	if (gotoWorkspace != nullptr)
	{
		addAndMakeVisible(gotoWorkspace);
		gotoWorkspace->addMouseListener(this, true);
	}
        
    
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
	if(getProcessor() != nullptr)
		getProcessor()->removeBypassListener(this);
}

void PatchBrowser::PatchCollection::mouseDown(const MouseEvent& e)
{
	if (e.eventComponent == gotoWorkspace)
		return;

    if(auto pb = findParentComponentOfClass<PatchBrowser>())
    {
		if(e.mods.isRightButtonDown())
        {
            auto p = getProcessor();
            
            if(auto c = dynamic_cast<Chain*>(p))
                ProcessorEditor::createProcessorFromPopup(this, p, nullptr);
            else
                ProcessorEditor::createProcessorFromPopup(this, p->getParentProcessor(false), p);
            return;
        }
		else if (getProcessor() != nullptr)
			PatchBrowser::showProcessorInPopup(this, e, getProcessor());
    }

	
}

void PatchBrowser::PatchCollection::paint(Graphics &g)
{
	if (getProcessor() == nullptr) return;

	ModulatorSynth *synth = dynamic_cast<ModulatorSynth*>(root.get());

	float xOffset = getIntendation();

	g.setFont(GLOBAL_BOLD_FONT().withHeight(16.0f));

	auto b = getLocalBounds().removeFromTop(40).toFloat();

	b.removeFromLeft(xOffset);

	

    g.setGradientFill(ColourGradient(JUCE_LIVE_CONSTANT_OFF(Colour(0xff303030)), 0.0f, 0.0f,
                                     JUCE_LIVE_CONSTANT_OFF(Colour(0xff212121)), 0.0f, (float)b.getHeight(), false));
    

    
    
	auto iconSpace2 = b.reduced(7.0f);
	auto iconSpace = iconSpace2.removeFromLeft(iconSpace2.getHeight());

    
    g.fillRoundedRectangle(iconSpace2, 2.0f);
    
    g.setColour(Colours::white.withAlpha(0.1f));
    g.drawRoundedRectangle(iconSpace2.reduced(2.0f), 1.0f, 1.0f);
    
	g.setGradientFill(ColourGradient(synth->getIconColour().withMultipliedBrightness(1.1f), 0.0f, 7.0f,
		synth->getIconColour().withMultipliedBrightness(0.9f), 0.0f, 35.0f, false));

	g.fillRoundedRectangle(iconSpace, 2.0f);

	g.setColour(Colour(0xFF222222));

	g.drawRoundedRectangle(iconSpace, 2.0f, 2.0f);

    if (isMouseOver(false) || (gotoWorkspace != nullptr && gotoWorkspace->isMouseOver(true)))
    {
        g.setColour(Colour(SIGNAL_COLOUR).withAlpha(0.5f));
        g.drawRoundedRectangle(iconSpace2, 2.0f, 1.0f);
		
    }
    
	if (inPopup)
	{
		g.setColour(Colour(SIGNAL_COLOUR).withAlpha(0.1f));
		g.fillRoundedRectangle(iconSpace2, 2.0f);
		g.setColour(Colour(SIGNAL_COLOUR).withAlpha(0.6f));
		g.drawRoundedRectangle(iconSpace2, 2.0f, 1.0f);
	}

	g.setColour(Colours::white.withAlpha(bypassed ? 0.2f : 0.8f));
	g.drawText(root->getId(), (int)iconSpace.getRight() + 18, 10, getWidth(), 20, Justification::centredLeft, false);
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


void PatchBrowser::PatchCollection::applyLayout()
{
    auto b = getLocalBounds().removeFromTop(40);

    auto iconSpace = b.reduced(7);
    iconSpace = iconSpace.removeFromLeft(iconSpace.getHeight());
    
    foldButton->setBorderSize(BorderSize<int>(JUCE_LIVE_CONSTANT_OFF(10)));
    foldButton->setBounds(iconSpace.expanded(4));
    peak.setBounds(iconSpace.removeFromRight(9).translated(12 + getIntendation(), 0));

    if (gotoWorkspace != nullptr)
    {
        gotoWorkspace->setBorderSize(BorderSize<int>(JUCE_LIVE_CONSTANT_OFF(12)));
        gotoWorkspace->setBounds(b.removeFromRight(b.getHeight()));
    }
}

void PatchBrowser::PatchCollection::resized()
{
    SearchableListComponent::Collection::resized();

    if(getParentComponent() != nullptr)
        applyLayout();
    
	
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
lastMouseDown(0),
peak(p),
closeButton("close", nullptr, f),
createButton("create", nullptr, f)
{
    createButton.onClick = [this]()
    {
        if (auto c = dynamic_cast<Chain*>(processor.get()))
            ProcessorEditor::createProcessorFromPopup(this, processor, nullptr);
        else
            ProcessorEditor::createProcessorFromPopup(this, processor->getParentProcessor(false), processor);
    };
    
	addAndMakeVisible(closeButton);
	addAndMakeVisible(createButton);
	p->addBypassListener(this);

    addAndMakeVisible(idLabel = new Label());
	addAndMakeVisible(gotoWorkspace);
	addAndMakeVisible(peak);

    gotoWorkspace = PatchBrowser::skinWorkspaceButton(getProcessor());

	if (gotoWorkspace != nullptr)
	{
		addAndMakeVisible(gotoWorkspace);
		gotoWorkspace->addMouseListener(this, true);
	}
        
    

	setRepaintsOnMouseActivity(true);

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
	if(getProcessor() != nullptr)
		getProcessor()->removeBypassListener(this);
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

void PatchBrowser::PatchItem::mouseDown(const MouseEvent& e)
{
	if (e.eventComponent == gotoWorkspace)
		return;

	auto canBeBypassed = dynamic_cast<Chain*>(getProcessor()) == nullptr;
	canBeBypassed |= dynamic_cast<ModulatorSynth*>(getProcessor()) != nullptr;

	if (bypassArea.contains(e.getPosition()) && canBeBypassed)
	{
		bool shouldBeBypassed = !getProcessor()->isBypassed();
		getProcessor()->setBypassed(shouldBeBypassed, sendNotification);
		return;
	}

    const bool isEditable = dynamic_cast<Chain*>(processor.get()) == nullptr ||
		dynamic_cast<ModulatorSynth*>(processor.get()) != nullptr;

	if (isEditable && e.mods.isShiftDown())
	{
		idLabel->showEditor();
		return;
	}

    if(auto pb = findParentComponentOfClass<PatchBrowser>())
    {
		if (e.mods.isRightButtonDown())
		{
			if (auto c = dynamic_cast<Chain*>(processor.get()))
				ProcessorEditor::createProcessorFromPopup(this, processor, nullptr);
			else
				ProcessorEditor::createProcessorFromPopup(this, processor->getParentProcessor(false), processor);
		}
		else
		{
			if (processor.get() != nullptr)
				PatchBrowser::showProcessorInPopup(this, e, processor);
		}
    }
}

void PatchBrowser::PatchItem::applyLayout()
{
    auto b = getLocalBounds();

    b.removeFromLeft(hierarchy * 10 + 10);
    b.removeFromLeft(b.getHeight() + 2);

    b.removeFromLeft(findParentComponentOfClass<PatchCollection>()->getIntendation());
    peak.setBounds(b.removeFromLeft(b.getHeight() - JUCE_LIVE_CONSTANT_OFF(8)).toNearestInt());

    idLabel->setFont(GLOBAL_BOLD_FONT());
    idLabel->setJustificationType(Justification::centredLeft);
    idLabel->setBounds(b.toNearestInt());
    
    idLabel->setText(getProcessor()->getId(), dontSendNotification);
    
    //peak.setVisible(dynamic_cast<MidiProcessor*>(getProcessor()) == nullptr);

    auto canBeDeleted = dynamic_cast<Chain*>(getProcessor()) == nullptr;
    canBeDeleted |= dynamic_cast<ModulatorSynth*>(getProcessor()) != nullptr;
    canBeDeleted &= getProcessor() != getProcessor()->getMainController()->getMainSynthChain();

    closeButton.setVisible(canBeDeleted && findParentComponentOfClass<PatchBrowser>()->showChains);
    
    if (closeButton.isVisible())
    {
        closeButton.setBorderSize(BorderSize<int>(2));
        closeButton.setBounds(b.removeFromRight(getHeight()));
    }
   
        
        
    if (dynamic_cast<Chain*>(getProcessor()) != nullptr)
    {
        createButton.setBorderSize(BorderSize<int>(2));
        createButton.setBounds(b.removeFromRight(getHeight()));
    }

    if (gotoWorkspace != nullptr)
    {
        gotoWorkspace->setBorderSize(BorderSize<int>(3, 13, 3, 3));
        gotoWorkspace->setBounds(b.removeFromRight(getHeight() + 10));
    }
        
}

void PatchBrowser::PatchItem::resized()
{
    if(getParentComponent() != nullptr)
        applyLayout();
}

void PatchBrowser::rebuilt()
{
    if(auto root = findParentComponentOfClass<BackendRootWindow>())
    {
        Component::callRecursive<ModuleDragTarget>(this, [root](ModuleDragTarget* d)
        {
            if(d->gotoWorkspace != nullptr)
            {
                root->workspaceListeners.addListener(*d, ModuleDragTarget::setWorkspace);
            }
            
            d->applyLayout();
            
            return false;
        });
        
#if 0
        for(int i = 0; i < getNumCollections(); i++)
        {
            auto c = dynamic_cast<PatchCollection*>(getCollection(i));
            
            
            
            for(int j = 0; j < c->getNumItems(false); j++)
            {
                auto item = dynamic_cast<PatchItem*>(c->getItem(j));
                
                if(item->gotoWorkspace != nullptr)
                {
                    root->workspaceListeners.addListener(*item, PatchItem::setWorkspace);
                }
            }
        }
#endif
    }

	refreshPopupState();
}

void PatchBrowser::PatchItem::paint(Graphics& g)
{
	idLabel->setColour(Label::ColourIds::textColourId, Colours::white.withAlpha(bypassed ? 0.2f : 0.8f));

	if (processor.get() != nullptr)
	{
		auto b = getLocalBounds().toFloat();

		b.removeFromLeft(hierarchy * 10.0f + 10.0f);
		b.removeFromLeft(findParentComponentOfClass<PatchCollection>()->getIntendation());

		auto pb = findParentComponentOfClass<PatchBrowser>();

		if (pb->showChains)
		{
			b.removeFromRight(getHeight());
		}

		const bool isRoot = GET_BACKEND_ROOT_WINDOW(this)->getMainSynthChain()->getRootProcessor() == processor;

        g.setGradientFill(ColourGradient(JUCE_LIVE_CONSTANT_OFF(Colour(0xff303030)), 0.0f, 0.0f,
                                         JUCE_LIVE_CONSTANT_OFF(Colour(0xff282828)), 0.0f, (float)b.getHeight(), false));
        
        

		g.fillRoundedRectangle(b, 2.0f);

        g.setColour(Colours::white.withAlpha(0.1f));
        g.drawRoundedRectangle(b.reduced(1.0f), 2.0f, 1.0f);
        
		if (isMouseOver(true))
		{
			g.setColour(Colour(SIGNAL_COLOUR).withAlpha(0.5f));
			g.drawRoundedRectangle(b.reduced(1.0f), 2.0f, 1.0f);
		}
		
		if (inPopup)
		{
			g.setColour(Colour(SIGNAL_COLOUR).withAlpha(0.1f));
			g.fillRoundedRectangle(b.reduced(1.0f), 2.0f);
			g.setColour(Colour(SIGNAL_COLOUR).withAlpha(0.6f));
			g.drawRoundedRectangle(b.reduced(1.0f), 2.0f, 1.0f);
		}

		g.setColour(processor->getColour().withAlpha(!bypassed ? 1.0f : 0.5f));

		auto iconSpace = b.removeFromLeft(b.getHeight()).reduced(2.0f);

		bypassArea = iconSpace.toNearestInt();

        auto canBeBypassed = dynamic_cast<Chain*>(getProcessor()) == nullptr;
        canBeBypassed |= dynamic_cast<ModulatorSynth*>(getProcessor()) != nullptr;
        
        if(!canBeBypassed)
        {
            g.drawRoundedRectangle(iconSpace.reduced(1.0f), 2.0f, 2.0f);
            g.setColour(processor->getColour().withAlpha(0.2f));
        }
        
        g.fillRoundedRectangle(iconSpace, 2.0f);

		g.setColour(Colour(0xFF222222));

		g.drawRoundedRectangle(iconSpace, 2.0f, 2.0f);

		g.setColour(ProcessorHelpers::is<Chain>(processor) ? Colours::black.withAlpha(0.6f) : Colours::black);
		
	}
}

struct PlotterPopup: public Component
{
	PlotterPopup(Processor* m_):
		m(m_),
		resizer(this, nullptr)
	{
		dynamic_cast<Modulation*>(m.get())->setPlotter(&p);
		addAndMakeVisible(p);
		addAndMakeVisible(resizer);

		setName("Plotter: " + m_->getId());
		setSize(280, 200);
		p.setOpaque(false);
		p.setColour(Plotter::ColourIds::backgroundColour, Colour(0));
		
	}

	void resized() override
	{
		p.setBounds(getLocalBounds().reduced(20));
		resizer.setBounds(getLocalBounds().removeFromRight(15).removeFromBottom(15));
	}

	~PlotterPopup()
	{
		if(m != nullptr)
			dynamic_cast<Modulation*>(m.get())->setPlotter(nullptr);
	}

	WeakReference<Processor> m;
	Plotter p;
	juce::ResizableCornerComponent resizer;
};

void PatchBrowser::MiniPeak::mouseDown(const MouseEvent& e)
{
	auto pl = new PlotterPopup(p);
	
	

	GET_BACKEND_ROOT_WINDOW(this)->getRootFloatingTile()->showComponentInRootPopup(pl, getParentComponent(), { 100, 35 }, false);
}

void PatchBrowser::MiniPeak::paint(Graphics& g)
{
    if(p.get() == nullptr)
        return;
    
	auto area = getLocalBounds().toFloat().reduced(1.0f);

	switch (type)
	{
	case ProcessorType::Audio:
	{
		area = area.reduced(0.0f, 3.0f);

		Rectangle<float> la, ra;

		ra = area.removeFromRight(3.0f);
		area.removeFromRight(1.0f);
		la = area.removeFromRight(3.0f);

		g.setColour(Colours::white.withAlpha(0.2f));

		g.fillRect(la);
		g.fillRect(ra);

		g.setColour(Colours::white.withAlpha(0.7f));

		auto skew = JUCE_LIVE_CONSTANT_OFF(0.4f);

		auto lpeak = std::pow(jlimit(0.0f, 1.0f, l), skew);
		auto rpeak = std::pow(jlimit(0.0f, 1.0f, r), skew);

		la = la.removeFromBottom(lpeak * la.getHeight());
		ra = ra.removeFromBottom(rpeak * ra.getHeight());

		g.fillRect(la);
		g.fillRect(ra);

		break;
	}
	case ProcessorType::Mod:
	{
		if (auto c = dynamic_cast<Chain*>(p.get()))
		{
			if (c->getHandler()->getNumProcessors() == 0)
				return;
		}

		auto b = getLocalBounds().withSizeKeepingCentre(getWidth(), getWidth()).reduced(JUCE_LIVE_CONSTANT(3));
		g.setColour(Colours::black.withAlpha(0.4f));
		g.fillEllipse(b.toFloat());

		g.setColour(p->getColour().withAlpha(l));
		g.fillEllipse(b.toFloat().reduced(1.0f));
		break;
	}
	case ProcessorType::Midi:
	{
		Path mp;
		mp.loadPathFromData(HiBinaryData::SpecialSymbols::midiData, sizeof(HiBinaryData::SpecialSymbols::midiData));
		PathFactory::scalePath(mp, getLocalBounds().toFloat().reduced(1.0f));
		g.setColour(Colours::white.withAlpha(0.2f));
		g.fillPath(mp);
		g.setColour(p->getColour());

		if (auto synth = dynamic_cast<ModulatorSynth*>(p->getParentProcessor(true)))
		{
			g.setColour(Colours::white.withAlpha(l));
			g.fillPath(mp);
		}

		break;
	}
	}
}

void PatchBrowser::MiniPeak::timerCallback()
{
	if (p == nullptr)
		return;

	switch (type)
	{
	case ProcessorType::Mod:
	{
		auto newValue = getModValue();

		if (l != newValue)
		{
			l = newValue;
			repaint();
		}

		break;
	}
	case ProcessorType::Midi:
	{
		auto newValue = dynamic_cast<ModulatorSynth*>(p->getParentProcessor(true))->getMidiInputFlag();

		if (l != newValue)
		{
			l = newValue;
			repaint();
		}
		
		break;
	}
	case ProcessorType::Audio:
	{
		const auto& v = p->getDisplayValues();

		if (v.outL != l || v.outR != r)
		{
			auto decay = JUCE_LIVE_CONSTANT_OFF(0.7f);
			l = jmax(l *= decay, v.outL);
			r = jmax(r *= decay, v.outR);

			if (l < 0.001f)
				l = 0.0f;
			if (r < 0.001f)
				r = 0.0f;

			repaint();
		}
	}
	}
}

} // namespace hise
