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

#include "PatchBrowser.h"
#include "PatchBrowser.h"

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

	window->getBackendProcessor()->getLockFreeDispatcher().addPresetLoadListener(this);

	
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

	newHisePresetLoaded();
}

PatchBrowser::~PatchBrowser()
{
	if(rootWindow != nullptr)
	{
		rootWindow->getBackendProcessor()->getLockFreeDispatcher().removePresetLoadListener(this);
		rootWindow->getModuleListNofifier().removeProcessorChangeListener(this);
	}

	addButton = nullptr;
}


#if 0
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
	
}

void PatchBrowser::itemDropped(const SourceDetails& dragSourceDetails)
{
	
}
#endif


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
		
		if (p == nullptr)
			return true;

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
		
		Component::SafePointer<FloatingTilePopup> safePopup = ft->showComponentAsDetachedPopup(pe, bp, { b.getRight() + 50 + (CONTAINER_WIDTH)/2, b.getY() -10 }, true);

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

void PatchBrowser::mouseMove(const MouseEvent& e)
{
	if (!showChains)
		return;

	Processor* thisHover = nullptr;

	if (auto m = e.eventComponent->findParentComponentOfClass<ModuleDragTarget>())
	{
		thisHover = m->getProcessor();

		if (auto c = dynamic_cast<Chain*>(thisHover))
		{
			if(c->getHandler()->getNumProcessors() > 0)
			{
				thisHover = c->getHandler()->getProcessor(c->getHandler()->getNumProcessors() - 1);

				while (thisHover->getNumChildProcessors() != 0)
				{
					thisHover = thisHover->getChildProcessor(thisHover->getNumChildProcessors() - 1);
				}
			}
		}
		else
		{
			auto pc = dynamic_cast<Chain*>(thisHover->getParentProcessor(false));

			if (pc->getHandler()->getNumProcessors() > 1)
			{
				for (int i = 0; i < pc->getHandler()->getNumProcessors() - 1; i++)
				{
					if (pc->getHandler()->getProcessor(i + 1) == thisHover)
					{
						thisHover = pc->getHandler()->getProcessor(i);
						break;
					}
				}
			}
			else
				thisHover = dynamic_cast<Processor*>(pc);
		}
	}

	if (insertHover.get() != thisHover)
	{
		insertHover = thisHover;
		repaint();
	}
	
}

void PatchBrowser::mouseExit(const MouseEvent& e)
{
	if (auto m = e.eventComponent->findParentComponentOfClass<ModuleDragTarget>())
	{
		if (insertHover != nullptr)
		{
			insertHover = nullptr;
			repaint();
		}
	}
}

int PatchBrowser::getNumCollectionsToCreate() const
{
    Processor::Iterator<ModulatorSynth> iter(rootWindow.getComponent()->getMainSynthChain());

	int i = 0;

	while (iter.getNextProcessor())
	{
		i++;
	}

	if (rootWindow.getComponent()->getMainController()->getGlobalRoutingManager() != nullptr)
		i++;

	return i;
}

struct GlobalCableCollection : public SearchableListComponent::Collection,
						       public ControlledObject,
							   public PooledUIUpdater::SimpleTimer
{
	struct CableItem : public SearchableListComponent::Item
	{
		PatchBrowser::Factory f;

		CableItem(scriptnode::routing::GlobalRoutingManager::SlotBase::Ptr p) :
			Item(p->id),
			slot(p),
			gotoButton("workspace", nullptr, f)
		{
			addAndMakeVisible(gotoButton);

			scriptnode::routing::GlobalRoutingManager::Helpers::addGotoTargetCallback(&gotoButton, slot.get());

			setSize(380 - 16, ITEM_HEIGHT);
		};

		void resized() override
		{
			gotoButton.setBounds(getLocalBounds().removeFromRight(getHeight()).reduced(3));
		}

		void paint(Graphics &g) override
		{
			auto b = getLocalBounds().toFloat();
			b.removeFromLeft(32.0f);

			paintItemBackground(g, b);

			auto led = b.removeFromLeft(b.getHeight()).reduced(6.0f);

			auto c = scriptnode::routing::GlobalRoutingManager::Helpers::getColourFromId(searchKeywords);

			g.setColour(c);
			g.drawEllipse(led, 1.0f);

			if (auto cable = dynamic_cast<scriptnode::routing::GlobalRoutingManager::Cable*>(slot.get()))
			{
				g.setColour(c.withAlpha((float)jlimit(0.0, 1.0, cable->lastValue)));
				g.fillEllipse(led.reduced(2.0f));
			}

			g.setColour(Colours::white.withAlpha(0.7f));
			g.setFont(GLOBAL_BOLD_FONT());

			b.removeFromLeft(5.0f);

			String s;
			s << searchKeywords;
			s << " (" << String(slot->getTargetList().size()) << ")";

			g.drawText(s, b, Justification::left);
		}

		scriptnode::routing::GlobalRoutingManager::SlotBase::Ptr slot;
		HiseShapeButton gotoButton;
	};

	GlobalCableCollection(var m, MainController* mc) :
		Collection(),
		ControlledObject(mc),
		SimpleTimer(mc->getGlobalUIUpdater()),
		manager(dynamic_cast<scriptnode::routing::GlobalRoutingManager*>(m.getObject())),
		foldButton("folded", nullptr, f, "unfolded")
	{
		auto type = scriptnode::routing::GlobalRoutingManager::SlotBase::SlotType::Cable;

		manager->listUpdater.addListener(*this, rebuildList, false);

		foldButton.setToggleModeWithColourChange(true);
		foldButton.onColour = foldButton.offColour;
		foldButton.refreshButtonColours();
		addAndMakeVisible(foldButton);
		foldButton.onClick = BIND_MEMBER_FUNCTION_0(GlobalCableCollection::toggleFold);

		auto cableList = manager->getIdList(type);

		for (auto c : cableList)
		{
			items.add(new CableItem(manager->getSlotBase(c, type)));
			addAndMakeVisible(items.getLast());
		}
	};

	static void rebuildList(GlobalCableCollection& c, scriptnode::routing::GlobalRoutingManager::SlotBase::SlotType t, StringArray idList)
	{
		SafeAsyncCall::call<GlobalCableCollection>(c, [](GlobalCableCollection& c)
		{
			c.findParentComponentOfClass<PatchBrowser>()->rebuildModuleList(true);
		});
	}

	void toggleFold()
	{
		setFolded(foldButton.getToggleState());
		findParentComponentOfClass<PatchBrowser>()->refreshDisplayedItems();
	}

	void timerCallback() override
	{
		if (!foldButton.getToggleState())
		{
			for (auto i : items)
				i->repaint();
		}
	}

	void paint(Graphics &g) override
	{
		g.setFont(GLOBAL_BOLD_FONT().withHeight(16.0f));

		auto b = getLocalBounds().removeFromTop(40).toFloat();

		

		g.setGradientFill(ColourGradient(JUCE_LIVE_CONSTANT_OFF(Colour(0xff303030)), 0.0f, 0.0f,
			JUCE_LIVE_CONSTANT_OFF(Colour(0xff212121)), 0.0f, (float)b.getHeight(), false));

		auto iconSpace2 = b.reduced(7.0f);
		auto iconSpace = iconSpace2.removeFromLeft(iconSpace2.getHeight());

		g.fillRoundedRectangle(iconSpace2.reduced(2.0f), 2.0f);

		g.setColour(Colours::white.withAlpha(0.1f));
		g.drawRoundedRectangle(iconSpace2.reduced(2.0f), 1.0f, 1.0f);

		auto c = JUCE_LIVE_CONSTANT_OFF(Colour(0xff828282));

		g.setGradientFill(ColourGradient(c.withMultipliedBrightness(1.1f), 0.0f, 7.0f,
			c.withMultipliedBrightness(0.9f), 0.0f, 35.0f, false));

		g.setColour(Colour(0xFF222222));
		g.drawRoundedRectangle(iconSpace.reduced(2.0f), 2.0f, 1.0f);
		g.setColour(Colours::white.withAlpha(0.7f));
		g.drawText("Global Cables", iconSpace2.reduced(10.0f, 0.0f), Justification::left);
	}

	void resized() override
	{
		Collection::resized();
		foldButton.setBounds(getLocalBounds().removeFromTop(40).removeFromLeft(40).reduced(12));
	}

	PatchBrowser::Factory f;

	HiseShapeButton foldButton;

	scriptnode::routing::GlobalRoutingManager::Ptr manager;

	JUCE_DECLARE_WEAK_REFERENCEABLE(GlobalCableCollection);
};

SearchableListComponent::Collection * PatchBrowser::createCollection(int index)
{
	auto mc = rootWindow.getComponent()->getMainController();

	if (auto obj = mc->getGlobalRoutingManager())
	{
		if (index == 0)
			return new GlobalCableCollection(var(obj), mc);
		else
			index -= 1;
	}
	
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

	for (int i = 0; i < numCollections; i++)
	{
		PatchCollection *c = dynamic_cast<PatchCollection*>(getCollection(i));

        if(i == 0) // skip the Master Chain
            continue;
        
		if (!c->hasVisibleItems()) continue;

        Processor *p = c->getProcessor();

		if (p == nullptr) return;

		Processor *root = ProcessorHelpers::findParentProcessor(p, true);

		for (int j = 0; j < numCollections; j++)
		{
			if (auto possibleRootCollection = dynamic_cast<PatchCollection*>(getCollection(j)))
			{
				if (possibleRootCollection->getProcessor() == root)
				{
					Point<int> startPoint = possibleRootCollection->getPointForTreeGraph(true);
					startPointInParent = getLocalPoint(possibleRootCollection, startPoint);
					break;
				}
			}
		}

		auto endPoint = c->getPointForTreeGraph(false).toFloat();
		auto endPointInParent = getLocalPoint(c, endPoint);

        bool paintUniform = false;
        
        if(auto ms = dynamic_cast<ModulatorSynth*>(c->getProcessor()))
        {
            if(ms->isUsingUniformVoiceHandler())
                paintUniform = true;
            
            if(auto msc = dynamic_cast<ModulatorSynthChain*>(ms))
            {
                if(msc->isUniformVoiceHandlerRoot())
                    paintUniform = false;
            }
        }
        
		g.setColour(paintUniform ? Colour(0xFF888888) :
            Colour(0xFF222222));

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
    
    struct GlobalModCablePin
    {
        Processor* p = nullptr;
        Point<float> point;
        Colour c;
    };
    
    Array<GlobalModCablePin> sources;
    Array<std::tuple<GlobalModCablePin, GlobalModCablePin>> connections;
    
    Component::callRecursive<PatchItem>(this, [&](PatchItem* pi)
    {
        
        if(!pi->isVisible() || pi->getProcessor() == nullptr)
            return false;
        
        if(dynamic_cast<GlobalModulatorContainer*>(pi->getProcessor()->getParentProcessor(true)))
        {
            GlobalModCablePin nd;
            nd.p = pi->getProcessor();
            nd.c = pi->getProcessor()->getColour();
            nd.point = getLocalPoint(pi, pi->bypassArea.getCentre()).toFloat();
            
            sources.add(nd);
        }
        
        if(auto gm = dynamic_cast<GlobalModulator*>(pi->getProcessor()))
        {
            if(auto om = gm->getOriginalModulator())
            {
                for(const auto& s: sources)
                {
                    if(s.p == om)
                    {
                        GlobalModCablePin nd;;
                        nd.p = pi->getProcessor();
                        nd.c = nd.p->getColour();
                        
                        
                        
                        nd.point = getLocalPoint(pi, pi->bypassArea.getCentre()).toFloat();
                        
                        connections.add({s, nd});
                        break;
                    }
                }
            }
        }
        
        return false;
    });
    
    auto x = 2.0f;
    
    for(const auto& c: connections)
    {
        auto startPoint = std::get<0>(c).point;
        auto endPoint = std::get<1>(c).point;
        
        Path gc;
        gc.startNewSubPath(startPoint);
        gc.lineTo(x, startPoint.getY() + x);
        gc.lineTo(x, endPoint.getY());
        gc.lineTo(endPoint);
        
        g.setColour(std::get<1>(c).c.withAlpha(JUCE_LIVE_CONSTANT_OFF(0.7f)));
        g.strokePath(gc, PathStrokeType(1.0f));
        
        x += 2.0f;
    }
}

void PatchBrowser::paintOverChildren(Graphics& g)
{
	if (insertHover != nullptr)
	{
		Component::callRecursive<ModuleDragTarget>(this, [this, &g](ModuleDragTarget* d)
		{
			if (d->getProcessor() == insertHover)
			{
				auto c = dynamic_cast<Component*>(d);
				auto b = getLocalArea(c, c->getLocalBounds()).toFloat();
				g.setColour(Colours::white.withAlpha(JUCE_LIVE_CONSTANT_OFF(0.6f)));
				b = b.removeFromBottom(0).withSizeKeepingCentre(16, 16).withX(0);

				Path arrow;
				arrow.addArrow(Line<float>(b.getX(), b.getCentreY(), b.getRight(), b.getCentreY()), b.getHeight() / 4, b.getHeight() / 2, b.getWidth() / 2);

				g.fillPath(arrow);
				return true;
			}

			return false;
		});
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
	SUSPEND_GLOBAL_DISPATCH(rootWindow->getBackendProcessor(), "toggle patch browser edit mode");
	
	showChains = !showChains;

	addButton->setToggleStateAndUpdateIcon(showChains);
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

void PatchBrowser::rebuilt()
{
	if (auto root = findParentComponentOfClass<BackendRootWindow>())
	{
		Component::callRecursive<ModuleDragTarget>(this, [root, this](ModuleDragTarget* d)
		{
			d->createButton.addMouseListener(this, true);

			if (d->gotoWorkspace != nullptr)
			{
				root->getBackendProcessor()->workspaceBroadcaster.addListener(*d, ModuleDragTarget::setWorkspace);
			}

			d->applyLayout();

			return false;
		});
	}

	refreshPopupState();
}

void PatchBrowser::newHisePresetLoaded()
{
	Processor::Iterator<Processor> iter(rootWindow->getBackendProcessor()->getMainSynthChain());

	int counter = 0;

	while(iter.getNextProcessor())
		counter++;

	auto shouldBeEditable = counter <= 6;

	if(showChains != shouldBeEditable)
	{
		toggleShowChains();
	}
}

// ====================================================================================================================

PatchBrowser::ModuleDragTarget::ModuleDragTarget(Processor* p_) :
BypassListener(p_->getMainController()->getRootDispatcher()),
p(p_),
peak(p_),
dragState(DragState::Inactive),
closeButton("close", nullptr, f),
createButton("create", nullptr, f),
isOver(false),
idUpdater(p->getMainController()->getRootDispatcher(), *this, BIND_MEMBER_FUNCTION_1(ModuleDragTarget::onNameOrColourUpdate))
{
	p->addBypassListener(this, dispatch::sendNotificationAsync);

	createButton.onClick = [this]()
	{
		auto p = getProcessor();
		auto c = dynamic_cast<Component*>(this);

		if (dynamic_cast<Chain*>(p) != nullptr)
			ProcessorEditor::createProcessorFromPopup(c, p, nullptr);
		else
			ProcessorEditor::createProcessorFromPopup(c, p->getParentProcessor(false), p);

		auto pb = createButton.findParentComponentOfClass<PatchBrowser>();
		pb->insertHover = nullptr;
		pb->repaint();
	};

	closeButton.setTooltip("Delete " + getProcessor()->getId());

    String type;
    
    if(dynamic_cast<ModulatorSynth*>(getProcessor()))
        type = "Sound generator";
    if(dynamic_cast<Modulator*>(getProcessor()))
        type = "Modulator";
    if(dynamic_cast<EffectProcessor*>(getProcessor()))
        type = "Effect";
    if(dynamic_cast<MidiProcessor*>(getProcessor()))
        type = "MIDI Processor";
    
	if (dynamic_cast<Chain*>(getProcessor()) != nullptr)
		createButton.setTooltip("Add a new " + type + " to this chain");
	else
		createButton.setTooltip("Add a new " + type + " before " + getProcessor()->getId());
	
	closeButton.onClick = [this]()
	{
		auto brw = GET_BACKEND_ROOT_WINDOW((&closeButton));
		brw->getRootFloatingTile()->clearAllPopups();

		auto p = getProcessor();
		auto c = dynamic_cast<Component*>(this);

		if(p != nullptr)
			ProcessorEditor::deleteProcessorFromUI(c, p);
	};

	soloButton = new ShapeButton("Solo Processor", Colours::white.withAlpha(0.2f), Colours::white.withAlpha(0.5f), Colours::white);

	static Path soloPath;

	soloPath.loadPathFromData(BackendBinaryData::PopupSymbols::soloShape, SIZE_OF_PATH(BackendBinaryData::PopupSymbols::soloShape));
	soloButton->setShape(soloPath, false, true, false);
	soloButton->addListener(this);

	hideButton = new ShapeButton("Hide Processor", Colours::white.withAlpha(0.2f), Colours::white.withAlpha(0.5f), Colours::white);

	static Path hidePath;

	hidePath.loadPathFromData(BackendBinaryData::ToolbarIcons::viewPanel, SIZE_OF_PATH(BackendBinaryData::ToolbarIcons::viewPanel));
	hideButton->setShape(hidePath, false, true, false);
	hideButton->addListener(this);
 
	idLabel.setInterceptsMouseClicks(false, true);
	idLabel.setColour(Label::ColourIds::textColourId, Colours::white);
	idLabel.setColour(Label::ColourIds::textWhenEditingColourId, Colours::white);
	idLabel.setColour(Label::ColourIds::outlineWhenEditingColourId, Colour(SIGNAL_COLOUR));
	idLabel.setColour(TextEditor::ColourIds::highlightColourId, Colour(SIGNAL_COLOUR));
	idLabel.setColour(TextEditor::ColourIds::highlightedTextColourId, Colours::black);
	idLabel.setColour(CaretComponent::ColourIds::caretColourId, Colours::white);

	idLabel.setFont(GLOBAL_BOLD_FONT());
	idLabel.setJustificationType(Justification::centredLeft);
	idLabel.setText(getProcessor()->getId(), dontSendNotification);
	idLabel.addListener(this);
    
	bypassed = getProcessor()->isBypassed();

	getProcessor()->addDeleteListener(this);
	getProcessor()->addNameAndColourListener(&idUpdater, dispatch::sendNotificationAsync);
}

PatchBrowser::ModuleDragTarget::~ModuleDragTarget()
{
	if(getProcessor() == nullptr)
	{
		return;
	}

	getProcessor()->removeDeleteListener(this);
	getProcessor()->removeBypassListener(this);
	getProcessor()->removeNameAndColourListener(&idUpdater);
}

void PatchBrowser::ModuleDragTarget::buttonClicked(Button *b)
{
	auto *mainEditor = GET_BACKEND_ROOT_WINDOW(dynamic_cast<Component*>(this))->getMainPanel();

	if (b == soloButton)
	{
		const bool isSolo = getProcessor()->getEditorState(Processor::EditorState::Solo);
		refreshButtonState(soloButton, !isSolo);
	}

	if (b == hideButton)
	{
		const bool isHidden = getProcessor()->getEditorState(Processor::EditorState::Visible);

		getProcessor()->setEditorState(Processor::Visible, !isHidden, sendNotification);

		mainEditor->getRootContainer()->refreshSize(false);
		
		refreshButtonState(hideButton, !isHidden);
	}
}

void PatchBrowser::ModuleDragTarget::refreshAllButtonStates()
{
	refreshButtonState(soloButton, getProcessor()->getEditorState(Processor::EditorState::Solo));
	refreshButtonState(hideButton, getProcessor()->getEditorState(Processor::EditorState::Visible));
}

bool PatchBrowser::ModuleDragTarget::startDrag(const MouseEvent& e)
{
	if(e.mods.isRightButtonDown() || e.mods.isAnyModifierKeyDown() || dragging)
		return false;

	auto pb = e.eventComponent->findParentComponentOfClass<PatchBrowser>();

	if(!pb->showChains)
		return false;

	if(canBeDragged())
	{
		String path;
		path << getProcessor()->getType() << "::" << getProcessor()->getId();

		Image img2(Image::PixelFormat::ARGB, 200, jmin(32, dynamic_cast<Component*>(this)->getHeight()), true);

		Graphics g(img2);

		auto b = img2.getBounds().toFloat();

		g.setColour(Colour(0xFF282828).withAlpha(0.7f));
		g.fillRoundedRectangle(b.reduced(1), 2.0);
		g.setColour(Colours::white.withAlpha(0.7f));
		g.drawRoundedRectangle(b.reduced(1.0f), 2.0f, 1.0f);
		auto ib = b.removeFromLeft(b.getHeight()).reduced(3.0f);
		b.removeFromLeft(10);
		g.setFont(GLOBAL_BOLD_FONT());
		g.drawText(getProcessor()->getId(), b, Justification::left);
		g.setColour(getProcessor()->getColour());
		g.fillRoundedRectangle(ib, 2.0f);

		Path p;
		p.loadPathFromData(EditorIcons::resizeIcon, SIZE_OF_PATH(EditorIcons::resizeIcon));
		PathFactory::scalePath(p, ib.reduced(4.0f));
		g.setColour(Colours::white);
		g.fillPath(p);

		auto sf = UnblurryGraphics::getScaleFactorForComponent(dynamic_cast<Component*>(this), false);

		juce::ScaledImage img(img2, sf);

		dragging = true;

		PatchBrowser::showProcessorInPopup(e.eventComponent, e, getProcessor());

		pb->startDragging(var(path), e.eventComponent, img);
		e.eventComponent->repaint();
		return true;
	}

	return false;
}

void PatchBrowser::ModuleDragTarget::refreshButtonState(ShapeButton *button, bool on)
{
	if (on)
		button->setColours(Colours::white.withAlpha(0.7f), Colours::white, Colours::white);
	else
		button->setColours(Colours::black.withAlpha(0.2f), Colours::white.withAlpha(0.5f), Colours::white);
}

void PatchBrowser::ModuleDragTarget::setDraggingOver(bool shouldBeOver)
{
	isOver = shouldBeOver;
	resetDragState();
	
	dynamic_cast<Component*>(this)->repaint();
}

void PatchBrowser::ModuleDragTarget::checkDragState(const SourceDetails& dragSourceDetails)
{
	auto c = dynamic_cast<Chain*>(getProcessor());

	if(c == nullptr)
		c = dynamic_cast<Chain*>(getProcessor()->getParentProcessor(false));

	if (c != nullptr)
	{
		auto sourceProcessor = dynamic_cast<ModuleDragTarget*>(dragSourceDetails.sourceComponent.get())->getProcessor();
		auto targetProcessor = dynamic_cast<Processor*>(c);
		
		Identifier t = Identifier(dragSourceDetails.description.toString().upToFirstOccurrenceOf("::", false, true));

		const bool allowed = c->getFactoryType()->allowType(t);

		setDragState(allowed ? DragState::Allowed : DragState::Forbidden);

		// Modulators can't change their Mode so you can't drag a modulator eg. from a pitch chain to a gain chain...
		if(auto mc = dynamic_cast<ModulatorChain*>(c))
		{
			auto mod = dynamic_cast<Modulation*>(sourceProcessor);
			
			if(mod != nullptr && mc->getMode() != mod->getMode())
				setDragState(DragState::Forbidden);
		}

		// Check that you don't drag a parent into its child...
		auto newParent = targetProcessor;

		while(newParent != nullptr)
		{
			if(newParent == sourceProcessor)
			{
				setDragState(DragState::Forbidden);
				break;
			}
			newParent = newParent->getParentProcessor(false, false);
		}

		auto pb = dynamic_cast<Component*>(this)->findParentComponentOfClass<PatchBrowser>();

		if(allowed)
		{
			pb->insertHover = getProcessor();
		}
		else
		{
			pb->insertHover = nullptr;
		}

		pb->repaint();

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

bool PatchBrowser::ModuleDragTarget::isInterestedInDragSource(const SourceDetails& dragSourceDetails)
{
	return dynamic_cast<const ModuleDragTarget*>(dragSourceDetails.sourceComponent.get()) != nullptr;
}



void PatchBrowser::ModuleDragTarget::itemDropped(const SourceDetails& dragSourceDetails)
{
	auto pb = dynamic_cast<Component*>(this)->findParentComponentOfClass<PatchBrowser>();
	pb->insertHover = nullptr;
	pb->repaint();

	if(getDragState() == DragState::Forbidden)
	{
		resetDragState();
		return;
	}

	if(dragSourceDetails.sourceComponent == dynamic_cast<Component*>(this))
	{
		resetDragState();
		return;
	}

	Chain *c = dynamic_cast<Chain*>(getProcessor());

	auto p = dynamic_cast<ModuleDragTarget*>(dragSourceDetails.sourceComponent.get())->getProcessor();

	auto oldChain = dynamic_cast<Chain*>(p->getParentProcessor(false));

	if(p->getParentProcessor(false) == getProcessor())
	{
		resetDragState();
		return;
	}

	if(c == nullptr)
		c = dynamic_cast<Chain*>(getProcessor()->getParentProcessor(false));

	if (c != nullptr)
	{
		Identifier type = Identifier(dragSourceDetails.description.toString().upToFirstOccurrenceOf("::", false, true));
		String name = dragSourceDetails.description.toString().fromLastOccurrenceOf("::", false, true);

		int index = -1;

		for(int i = 0; i < c->getHandler()->getNumProcessors(); i++)
		{
			if(c->getHandler()->getProcessor(i) == p)
			{
				index = i;
				break;
			}
		}

		p->getMainController()->allNotesOff();

		auto sibling = index == -1 ? nullptr : c->getHandler()->getProcessor(index);

		oldChain->getHandler()->remove(p, false);

		c->getHandler()->add(p, sibling);

		//if(index != -1)
		//	c->getHandler()->moveProcessor(p, -1);

		auto root = p->getMainController()->getMainSynthChain();
		root->prepareToPlay(root->getSampleRate(), root->getLargestBlockSize());

		resetDragState();
		pb->rebuildModuleList(true);
		pb->repaint();
	}
}

void PatchBrowser::ModuleDragTarget::handleRightClick(bool isInEditMode)
{
	auto p = getProcessor();

	auto comp = dynamic_cast<Component*>(this);

	if (isInEditMode)
	{
		if (auto c = dynamic_cast<Chain*>(p))
			ProcessorEditor::createProcessorFromPopup(comp, p, nullptr);
		else
			ProcessorEditor::createProcessorFromPopup(comp, p->getParentProcessor(false), p);
	}
	else
	{
		ProcessorEditor::showContextMenu(comp, p);
	}
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
ModuleDragTarget(synth),
hierarchy(hierarchy_)
{
	addAndMakeVisible(peak);
	addAndMakeVisible(idLabel);
	addAndMakeVisible(foldButton = new ShapeButton("Fold Overview", Colour(0xFF222222), Colour(0xFF888888), Colour(0xFF222222)));

	foldButton->setVisible(true);

    setTooltip("Show " + synth->getId() + " editor");
    
	idLabel.setFont(GLOBAL_BOLD_FONT().withHeight(JUCE_LIVE_CONSTANT_OFF(16.0f)));

	if (dynamic_cast<Chain*>(synth) != nullptr)
		addAndMakeVisible(createButton);

	if (synth->getMainController()->getMainSynthChain() != synth)
		addAndMakeVisible(closeButton);

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
			searchTerm << p->getId() << ";" << p->getType();
		}

		if (parentSynth != nullptr)
		{
			searchTerm << ";" << parentSynth->getId() << ";" << parentSynth->getType();
		}

		items.add(new PatchItem(p, parentSynth, iter.getHierarchyForCurrentProcessor(), searchTerm));
		addAndMakeVisible(items.getLast());
	}

	setRepaintsOnMouseActivity(true);
}


PatchBrowser::PatchCollection::~PatchCollection()
{
	
}

void PatchBrowser::PatchCollection::mouseDown(const MouseEvent& e)
{
	if (e.eventComponent == gotoWorkspace)
		return;

	auto canBeBypassed = getProcessor()->getMainController()->getMainSynthChain() != getProcessor();
	

	if (iconArea.contains(e.getPosition()) && canBeBypassed)
	{
		bool shouldBeBypassed = !getProcessor()->isBypassed();
		getProcessor()->setBypassed(shouldBeBypassed, sendNotification);
		return;
	}

	if (e.mods.isShiftDown())
	{
		idLabel.showEditor();
		return;
	}

    if(auto pb = findParentComponentOfClass<PatchBrowser>())
    {
		if(e.mods.isRightButtonDown())
        {
			handleRightClick(pb->showChains);
            return;
        }
		else if (getProcessor() != nullptr)
			PatchBrowser::showProcessorInPopup(this, e, getProcessor());
    }
}

void PatchBrowser::PatchCollection::paint(Graphics &g)
{
	if (getProcessor() == nullptr) return;

	ModulatorSynth *synth = dynamic_cast<ModulatorSynth*>(p.get());

	float xOffset = getIntendation();

	g.setFont(GLOBAL_BOLD_FONT().withHeight(16.0f));

	auto b = getLocalBounds().removeFromTop(40).toFloat();

	b.removeFromLeft(xOffset);

	

    g.setGradientFill(ColourGradient(JUCE_LIVE_CONSTANT_OFF(Colour(0xff303030)), 0.0f, 0.0f,
                                     JUCE_LIVE_CONSTANT_OFF(Colour(0xff212121)), 0.0f, (float)b.getHeight(), false));

	auto isRoot = synth == synth->getMainController()->getMainSynthChain();
    
	auto iconSpace2 = b.reduced(7.0f);

	if(closeButton.isVisible())
		iconSpace2 = iconSpace2.withRight(closeButton.getX());
	if(createButton.isVisible() && createButton.getParentComponent() != nullptr)
		iconSpace2 = iconSpace2.withRight(createButton.getX());

	auto iconSpace = iconSpace2.removeFromLeft(iconSpace2.getHeight());

	if(isRoot && createButton.isVisible())
	{
		iconSpace2.removeFromLeft(7);
	}
	

	g.fillRoundedRectangle(iconSpace2.reduced(2.0f), 2.0f);
    
	g.setColour(Colours::white.withAlpha(0.1f));
	g.drawRoundedRectangle(iconSpace2.reduced(2.0f), 1.0f, 1.0f);

	auto c = synth->getIconColour();

	if (c.isTransparent() && getProcessor()->getMainController()->getMainSynthChain() != getProcessor())
		c = JUCE_LIVE_CONSTANT_OFF(Colour(0xff828282));

	if (getProcessor()->isBypassed())
		c = c.withMultipliedAlpha(0.4f);

	if(!isRoot)
	{
		g.setGradientFill(ColourGradient(c.withMultipliedBrightness(1.1f), 0.0f, 7.0f,
		c.withMultipliedBrightness(0.9f), 0.0f, 35.0f, false));

		g.fillRoundedRectangle(iconSpace.reduced(2.0f), 2.0f);

		iconArea = iconSpace.toNearestInt();

		g.setColour(Colour(0xFF222222));

		g.drawRoundedRectangle(iconSpace.reduced(2.0f), 2.0f,  1.0f);
	}

	

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

	idLabel.setColour(Label::ColourIds::textColourId, Colours::white.withAlpha(bypassed ? 0.2f : 0.8f));
    
    if(auto ms = dynamic_cast<ModulatorSynthChain*>(getProcessor()))
    {
        if(ms->isUniformVoiceHandlerRoot())
        {
            
            g.setFont(GLOBAL_BOLD_FONT().withHeight(10.0f));
            
            
            
            auto b = iconSpace2.removeFromRight(30.0f);
            
            g.setColour(JUCE_LIVE_CONSTANT_OFF(Colour(0x14FFFFFF)));
            g.fillRoundedRectangle(b.reduced(3.0f), 2.0f);
            
            g.setColour(Colour(0xFF888888));
            g.drawText("UVH", b, Justification::centred);
        }
        
    }

	auto ds = getDragState();

	if(ds != DragState::Inactive)
	{
		g.setColour(Colour(ds == DragState::Forbidden ? HISE_ERROR_COLOUR : HISE_OK_COLOUR).withAlpha(0.4f));
		g.fillRoundedRectangle(b, 2.0f);
	}
	else if (dragging)
	{
		g.setColour(Colour(HISE_WARNING_COLOUR).withAlpha(0.2f));
		g.fillRoundedRectangle(b, 2.0f);
	}
}


void PatchBrowser::PatchCollection::refreshFoldButton()
{
	Path foldShape;
	foldShape.loadPathFromData(HiBinaryData::ProcessorEditorHeaderIcons::foldedIcon, SIZE_OF_PATH(HiBinaryData::ProcessorEditorHeaderIcons::foldedIcon));

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

    auto rectSpace = b.reduced(0, 7);
	rectSpace.removeFromLeft(7);
	rectSpace.removeFromRight(JUCE_LIVE_CONSTANT_OFF(3));
    auto iconSpace = rectSpace.removeFromLeft(rectSpace.getHeight());
    
    foldButton->setBorderSize(BorderSize<int>(JUCE_LIVE_CONSTANT_OFF(10)));
    foldButton->setBounds(iconSpace.expanded(4));
	rectSpace.removeFromLeft(JUCE_LIVE_CONSTANT_OFF(4) + getIntendation());

    peak.setBounds(rectSpace.removeFromLeft(peak.getPreferredWidth()));

	auto showAddClose = findParentComponentOfClass<PatchBrowser>()->showChains;

	createButton.setVisible(showAddClose);
	closeButton.setVisible(showAddClose);

	if (closeButton.isVisible() && closeButton.getParentComponent() == this)
	{
		closeButton.setBorderSize(BorderSize<int>(JUCE_LIVE_CONSTANT_OFF(4)));
		closeButton.setBounds(rectSpace.removeFromRight(rectSpace.getHeight()));
	}

	if (createButton.isVisible() && createButton.getParentComponent() == this)
	{
		createButton.setBorderSize(BorderSize<int>(JUCE_LIVE_CONSTANT_OFF(4)));
		createButton.setBounds(rectSpace.removeFromRight(rectSpace.getHeight()));
	}

    if (gotoWorkspace != nullptr)
    {
        gotoWorkspace->setBorderSize(BorderSize<int>(JUCE_LIVE_CONSTANT_OFF(12)));
        gotoWorkspace->setBounds(rectSpace.removeFromRight(b.getHeight()).expanded(0, b.getHeight() - rectSpace.getHeight()));
    }

	rectSpace.removeFromLeft(JUCE_LIVE_CONSTANT_OFF(5));
	idLabel.setBounds(rectSpace.toNearestInt());

	repaint();
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
	
}

void PatchBrowser::PatchCollection::resetDragState()
{
	ModuleDragTarget::resetDragState();
	
}

void PatchBrowser::PatchCollection::toggleShowChains()
{
}

// ====================================================================================================================

PatchBrowser::PatchItem::PatchItem(Processor *p, Processor *parent_, int hierarchy_, const String &searchTerm) :
Item(searchTerm.toLowerCase()),
ModuleDragTarget(p),
parent(parent_),
lastId(String()),
hierarchy(hierarchy_),
lastMouseDown(0)
{
    setTooltip("Show " + p->getId() + " editor");
    
	addAndMakeVisible(closeButton);
	addAndMakeVisible(createButton);

    addAndMakeVisible(idLabel);
	addAndMakeVisible(gotoWorkspace);
	addAndMakeVisible(peak);

    gotoWorkspace = PatchBrowser::skinWorkspaceButton(getProcessor());

	if (gotoWorkspace != nullptr)
	{
		addAndMakeVisible(gotoWorkspace);
		gotoWorkspace->addMouseListener(this, true);
	}
        
	closeButton.addMouseListener(this, true);
	createButton.addMouseListener(this, true);
	peak.addMouseListener(this, true);

	setRepaintsOnMouseActivity(true);

    //idLabel->setEditable(false);
    //idLabel->addMouseListener(this, true);
    
	

	setSize(380 - 16, ITEM_HEIGHT);

	setUsePopupMenu(true);
	setRepaintsOnMouseActivity(true);
    
    
}

PatchBrowser::PatchItem::~PatchItem()
{
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

	m.addItem((int)ModuleDragTarget::ViewSettings::Visible, "Show module", true, getProcessor()->getEditorState(Processor::Visible));

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
		mainEditor->getRootContainer()->refreshSize(false);	
		break;
	case PatchBrowser::ModuleDragTarget::ViewSettings::Solo:
		getProcessor()->toggleEditorState(Processor::Solo, sendNotification);

		break;
	case PatchBrowser::ModuleDragTarget::ViewSettings::Root:
		mainEditor->setRootProcessor(p);
		findParentComponentOfClass<SearchableListComponent>()->repaint();
		break;
	case PatchBrowser::ModuleDragTarget::ViewSettings::Bypassed:
		getProcessor()->setBypassed(!getProcessor()->isBypassed());
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
			editorOfChain->otherChange(editorOfChain->getProcessor());
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
	if (e.eventComponent != this)
		return;

	auto canBeBypassed = dynamic_cast<Chain*>(getProcessor()) == nullptr;
	canBeBypassed |= dynamic_cast<ModulatorSynth*>(getProcessor()) != nullptr;

	if (bypassArea.contains(e.getPosition()) && canBeBypassed)
	{
		bool shouldBeBypassed = !getProcessor()->isBypassed();
		getProcessor()->setBypassed(shouldBeBypassed, sendNotification);
		return;
	}

    const bool isEditable = dynamic_cast<Chain*>(p.get()) == nullptr ||
		dynamic_cast<ModulatorSynth*>(p.get()) != nullptr;

	if (isEditable && e.mods.isShiftDown())
	{
		idLabel.showEditor();
		return;
	}

    if(auto pb = findParentComponentOfClass<PatchBrowser>())
    {
		if (e.mods.isRightButtonDown())
		{
			handleRightClick(pb->showChains);
		}
		else
		{
			if (p.get() != nullptr)
				PatchBrowser::showProcessorInPopup(this, e, p);
		}
    }
}

void PatchBrowser::PatchItem::applyLayout()
{
	if(getProcessor() == nullptr)
		return;

    auto b = getLocalBounds();

    b.removeFromLeft(hierarchy * 10 + 10);
    b.removeFromLeft(b.getHeight() + 2);

    b.removeFromLeft(findParentComponentOfClass<PatchCollection>()->getIntendation());
	auto peakBounds = b.removeFromLeft(peak.getPreferredWidth()).toNearestInt();
    peak.setBounds(peakBounds);

    auto canBeDeleted = dynamic_cast<Chain*>(getProcessor()) == nullptr;
    canBeDeleted |= dynamic_cast<ModulatorSynth*>(getProcessor()) != nullptr;
    canBeDeleted &= getProcessor() != getProcessor()->getMainController()->getMainSynthChain();
    canBeDeleted &= dynamic_cast<SlotFX*>(getProcessor()->getParentProcessor(false, true)) == nullptr;
    
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
        
	
	idLabel.setBounds(b.toNearestInt());
	

	repaint();
}

void PatchBrowser::PatchItem::paint(Graphics& g)
{
	idLabel.setColour(Label::ColourIds::textColourId, Colours::white.withAlpha(bypassed ? 0.2f : 0.8f));

	Colour pColour = Colours::grey;
	


	if (p.get() != nullptr)
		pColour = p.get()->getColour();

	auto b = getLocalBounds().toFloat();

	b.removeFromLeft(hierarchy * 10.0f + 10.0f);
	b.removeFromLeft(findParentComponentOfClass<PatchCollection>()->getIntendation());

	auto pb = findParentComponentOfClass<PatchBrowser>();

	if (pb->showChains)
	{
		b.removeFromRight(getHeight());
	}

	paintItemBackground(g, b);

	if (isMouseOver(false))
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

	g.setColour(pColour.withAlpha(!bypassed ? 1.0f : 0.5f));

	auto iconSpace = b.removeFromLeft(b.getHeight()).reduced(2.0f);

	bypassArea = iconSpace.toNearestInt();

	auto canBeBypassed = dynamic_cast<Chain*>(getProcessor()) == nullptr;
	canBeBypassed |= dynamic_cast<ModulatorSynth*>(getProcessor()) != nullptr;

	if (!canBeBypassed)
	{
		g.drawRoundedRectangle(iconSpace.reduced(1.0f), 2.0f, 2.0f);
		g.setColour(pColour.withAlpha(0.2f));
	}

	g.fillRoundedRectangle(iconSpace, 2.0f);

	g.setColour(Colour(0xFF222222));

	g.drawRoundedRectangle(iconSpace, 2.0f, 2.0f);

	g.setColour(ProcessorHelpers::is<Chain>(p.get()) ? Colours::black.withAlpha(0.6f) : Colours::black);

	auto ds = getDragState();

	if(ds != DragState::Inactive)
	{
		g.setColour(Colour(ds == DragState::Forbidden ? HISE_ERROR_COLOUR : HISE_OK_COLOUR).withAlpha(0.4f));
		g.fillRoundedRectangle(b, 2.0f);
	}
	else if (dragging)
	{
		g.setColour(Colour(HISE_WARNING_COLOUR).withAlpha(0.2f));
		g.fillRoundedRectangle(b, 2.0f);
	}

}

void PatchBrowser::PatchItem::resized()
{
    if(getParentComponent() != nullptr)
        applyLayout();
}





struct PlotterPopup: public Component
{
	struct VoiceStartPopup: public Component,
							public PooledUIUpdater::SimpleTimer
	{
		VoiceStartPopup(Processor* m_, PooledUIUpdater* updater):
		  SimpleTimer(updater),
		  voiceMod(dynamic_cast<Modulator*>(m_)),
		  synth(dynamic_cast<ModulatorSynth*>(m_->getParentProcessor(true))),
		  modChain(dynamic_cast<ModulatorChain*>(m_->getParentProcessor(false)))
		{
			
		}

		void paint(Graphics& g) override
		{
			auto b = getLocalBounds().toFloat().reduced(15.0f);


			g.setColour(Colours::white.withAlpha(0.05f));

			g.drawRect(b, 1.0f);

			auto minText = modChain->getTableValueConverter()(0.0f);
			auto maxText = modChain->getTableValueConverter()(1.0f);

			g.setFont(GLOBAL_BOLD_FONT());

			g.setColour(Colours::white.withAlpha(0.25f));

			g.drawText(maxText, getLocalBounds().toFloat(), Justification::topLeft);
			g.drawText(minText, getLocalBounds().toFloat(), Justification::bottomLeft);

			g.setColour(Colours::white.withAlpha(0.8f));



			g.strokePath(p, PathStrokeType(2.0f));

			for(const auto& i: info)
				i.draw(g);
		}

		void timerCallback() override
		{
			info.clearQuick();
			p.clear();

			auto b = getLocalBounds().toFloat().reduced(15.0f);
			
			auto numVoices = (float)synth->getNumActiveVoices();

			p.startNewSubPath(b.getBottomLeft());

			using VoiceInfo = std::pair<int, float>;
			Array<VoiceInfo> voiceValues;

			voiceValues.ensureStorageAllocated(numVoices);
			info.ensureStorageAllocated(numVoices);

			for(const auto& av: synth->activeVoices)
			{
				auto vi = av->getVoiceIndex();
				auto voiceModValue = dynamic_cast<VoiceStartModulator*>(voiceMod.get())->getVoiceStartValue(vi);
				
				voiceValues.add({vi, voiceMod->getValueForTextConverter(voiceModValue)});
			}

			struct Sorter
			{
				static int compareElements(const VoiceInfo& v1, const VoiceInfo& v2)
				{
					if(v1.first > v2.first)
						return 1;
					if(v1.first < v2.first)
						return -1;

					return 0;
				}
			} sorter;

			voiceValues.sort(sorter);

			int idx = 0;

			

			for(const auto& v: voiceValues)
			{
				auto offset = b.getX() + b.getWidth() / (numVoices) * 0.5f;
				auto x = offset + ((float)idx++ / numVoices) * b.getWidth();
				auto y = b.getY() + (1.0f - v.second) * b.getHeight();

				p.lineTo(x, y);

				p.addEllipse(x-2.0f, y-2.0f, 4.0f, 4.0f);

				if(auto voice = dynamic_cast<ModulatorSynthVoice*>(synth->getVoice(v.first)))
				{
					Info newInfo;
					newInfo.voiceIndex = v.first;
					newInfo.modValue = modChain->getTableValueConverter()(v.second);
					newInfo.pos = { x, y };
					newInfo.event = voice->getCurrentHiseEvent();

					info.add(newInfo);
				}
			}

			p.lineTo(b.getBottomRight());

			repaint();
		}

		struct Info
		{
			Point<float> pos;
			int voiceIndex;
			String modValue;
			HiseEvent event;

			void draw(Graphics& g) const
			{
				Rectangle<float> area(pos, pos);

				String m;
				m << "#" << String(event.getEventId()) << "(" << MidiMessage::getMidiNoteName(event.getNoteNumberIncludingTransposeAmount(), true, true, 2) <<  "): ";
				m << modValue;

				auto f = GLOBAL_BOLD_FONT();


				area = area.withSizeKeepingCentre(f.getStringWidthFloat(m) + 10.0f, 18.0f);

				area = area.translated(0.0f, -10.0f);

				g.setColour(Colours::black.withAlpha(0.8f));
				g.setFont(f);

				g.fillRoundedRectangle(area, area.getHeight() * 0.5f);
				g.setColour(Colours::white.withAlpha(0.5f));
				g.drawText(m, area, Justification::centred);
			}
		};

		Array<Info> info;

		Path p;

		ModulatorChain* modChain;
		WeakReference<Modulator> voiceMod;
		WeakReference<ModulatorSynth> synth;
	};

	PlotterPopup(Processor* m_):
		m(m_),
		p(),
		resizer(this, nullptr)
	{
		auto updater = m_->getMainController()->getGlobalUIUpdater();

		if(auto vc = dynamic_cast<VoiceStartModulator*>(m.get()))
		{
			p = new VoiceStartPopup(m_, updater);
		}
		else
		{
			auto np = new Plotter(updater);
			p = np;
			dynamic_cast<Modulation*>(m.get())->setPlotter(np);
		}
		
		addAndMakeVisible(p);
		addAndMakeVisible(resizer);

		setName("Plotter: " + m_->getId());
		setSize(280, 200);
		p->setOpaque(false);
		p->setColour(Plotter::ColourIds::backgroundColour, Colour(0));
		
	}

	void resized() override
	{

		p->setBounds(getLocalBounds().reduced(getPlotter() ? 20 : 5));
		resizer.setBounds(getLocalBounds().removeFromRight(15).removeFromBottom(15));
	}

	Plotter* getPlotter()
	{
		return dynamic_cast<Plotter*>(p.get());
	}

	~PlotterPopup()
	{
		if(m != nullptr)
		{
			dynamic_cast<Modulation*>(m.get())->setPlotter(nullptr);
		}
	}

	WeakReference<Processor> m;

	ScopedPointer<Component> p;

	
	juce::ResizableCornerComponent resizer;
};

PatchBrowser::MiniPeak::MiniPeak(Processor* p_) :
	PooledUIUpdater::SimpleTimer(p_->getMainController()->getGlobalUIUpdater()),
	p(p_),
	isMono(dynamic_cast<Modulator*>(p_) != nullptr)
{
	FloatVectorOperations::clear(channelValues, NUM_MAX_CHANNELS);

	setRepaintsOnMouseActivity(true);

	if (dynamic_cast<MidiProcessor*>(p_) != nullptr)
	{
		type = ProcessorType::Midi;
		numChannels = 0;
		setTooltip("Click to open event list viewer");
	}
	else if (dynamic_cast<Modulator*>(p_) != nullptr)
	{
		type = ProcessorType::Mod;
		numChannels = 1;
		setTooltip("Click to open Plotter");
	}
	else
	{
		type = ProcessorType::Audio;
		
		if (auto rp = dynamic_cast<RoutableProcessor*>(p_))
		{
			numChannels = rp->getMatrix().getNumSourceChannels();
			rp->getMatrix().addChangeListener(this);
            
			Array<int> channelIndexes;

			for (int i = 0; i < numChannels; i++)
				channelIndexes.add(i);

            if(numChannels != 2)
                rp->getMatrix().setEditorShown(channelIndexes, true);
		}
			
		else
			numChannels = 2;

		setTooltip("Click to edit channel routing");
	}
	
	setInterceptsMouseClicks(true, false);
}

PatchBrowser::MiniPeak::~MiniPeak()
{
	if (auto rp = dynamic_cast<RoutableProcessor*>(p.get()))
	{
		rp->getMatrix().removeChangeListener(this);
        

		Array<int> channelIndexes;

		for (int i = 0; i < numChannels; i++)
			channelIndexes.add(i);

        if(numChannels != 2)
            rp->getMatrix().setEditorShown(channelIndexes, false);
	}
}

void PatchBrowser::MiniPeak::mouseDown(const MouseEvent& e)
{
	auto root = GET_BACKEND_ROOT_WINDOW(this)->getRootFloatingTile();

	

	if (type == ProcessorType::Audio)
	{
		if(auto rp = dynamic_cast<RoutableProcessor*>(p.get()))
		{
			if(root->setTogglePopupFlag(*this, clicked))
			{
				rp->editRouting(this);
			}
		}
			
	}
    if(type == ProcessorType::Midi)
    {
		if(root->setTogglePopupFlag(*this, clicked))
		{
			auto pl = dynamic_cast<MidiProcessor*>(p.get())->createEventLogComponent();
			root->showComponentInRootPopup(pl, getParentComponent(), { 100, 35 }, false);
		}
    }
	if (type == ProcessorType::Mod)
	{
		if(root->setTogglePopupFlag(*this, clicked))
		{
			auto pl = new PlotterPopup(p);
			root->showComponentInRootPopup(pl, getParentComponent(), { 100, 35 }, false);
		}
	}
}

void PatchBrowser::MiniPeak::paint(Graphics& g)
{
    if(p.get() == nullptr)
        return;
    
	if (isMouseOver(true))
	{
		g.setColour(Colour(SIGNAL_COLOUR).withAlpha(0.2f));
		g.fillRect(getLocalBounds().reduced(0, 1));
	}

	auto area = getLocalBounds().toFloat().reduced(1.0f);

	switch (type)
	{
	case ProcessorType::Audio:
	{
		area = area.reduced(0.0f, 3.0f);

		float suspendAlpha = 0.0f;

		if (auto mfx = dynamic_cast<EffectProcessor*>(p.get()))
		{
			if (mfx->isCurrentlySuspended())
			{
				suspendAlpha = -0.1f;
			}
		}

		for (int i = 0; i < numChannels; i++)
		{
			auto a = area.removeFromLeft(3.0f);
			area.removeFromLeft(1.0f);
			
			float alpha1 = 0.2f;
			float alpha2 = 0.7f;

			if (auto rp = dynamic_cast<RoutableProcessor*>(p.get()))
			{
				if (rp->getMatrix().getConnectionForSourceChannel(i) == -1)
				{
					alpha1 = 0.1f;
					alpha2 = 0.2f;
				}
					
			}

			alpha1 += suspendAlpha;

			g.setColour(Colours::white.withAlpha(alpha1));
			g.fillRect(a);

			if (suspendAlpha == 0.0f)
			{
				g.setColour(Colours::white.withAlpha(alpha2));

				auto skew = JUCE_LIVE_CONSTANT_OFF(0.4f);
				auto v = std::pow(jlimit(0.0f, 1.0f, channelValues[i]), skew);
				a = a.removeFromBottom(v * a.getHeight());

				g.fillRect(a);
			}
		}

		if (suspendAlpha != 0.0f)
		{
			g.setColour(Colours::white.withAlpha(0.2f));
			g.setFont(GLOBAL_BOLD_FONT());
			g.drawText("S", getLocalBounds().toFloat(), Justification::centred);
		}

		break;
	}
	case ProcessorType::Mod:
	{
		if (auto c = dynamic_cast<Chain*>(p.get()))
		{
			if (c->getHandler()->getNumProcessors() == 0)
				return;
		}

		auto b = getLocalBounds().withSizeKeepingCentre(getWidth(), getWidth()).reduced(JUCE_LIVE_CONSTANT_OFF(2));
		g.setColour(Colours::black.withAlpha(0.4f));
		g.fillEllipse(b.toFloat());

		float value = channelValues[0];
		FloatSanitizers::sanitizeFloatNumber(value);
		value = jlimit(0.0f, 1.0f, value);

		g.setColour(p->getColour().withAlpha(value));
		g.fillEllipse(b.toFloat().reduced(1.0f));
		break;
	}
	case ProcessorType::Midi:
	{
		Path mp;
		mp.loadPathFromData(HiBinaryData::SpecialSymbols::midiData, SIZE_OF_PATH(HiBinaryData::SpecialSymbols::midiData));
		PathFactory::scalePath(mp, getLocalBounds().toFloat().reduced(1.0f));
		g.setColour(Colours::white.withAlpha(0.2f));
		g.fillPath(mp);
		g.setColour(p->getColour());

		if (auto synth = dynamic_cast<ModulatorSynth*>(p->getParentProcessor(true, false)))
		{
			g.setColour(Colours::white.withAlpha(channelValues[0]));
			g.fillPath(mp);
		}

		break;
	}
	}
}

float PatchBrowser::MiniPeak::getModValue()
{
	auto v = p->getDisplayValues().outL;

	if (dynamic_cast<Modulation*>(p.get())->getMode() == Modulation::PitchMode)
	{
		if (v != 0.0f)
			v = log2(v) * 0.5f + 0.5f;
	}

	v = dynamic_cast<Modulation*>(p.get())->calcIntensityValue(v);
	v = jlimit(0.0f, 1.0f, v);

	return v;
}

int PatchBrowser::MiniPeak::getPreferredWidth() const
{
	if (type == ProcessorType::Audio)
	{
		return numChannels * 4 + 1;
	}

	return JUCE_LIVE_CONSTANT_OFF(12);
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

		if (channelValues[0] != newValue)
		{
			channelValues[0] = newValue;
			repaint();
		}

		break;
	}
	case ProcessorType::Midi:
	{
		if (auto pp = p->getParentProcessor(true, false))
		{
			auto newValue = dynamic_cast<ModulatorSynth*>(pp)->getMidiInputFlag();

			if (channelValues[0] != newValue)
			{
				channelValues[0] = newValue;
				repaint();
			}
		}

		
		
		break;
	}
	case ProcessorType::Audio:
	{
		int thisNumChannels = 2;
		float thisData[NUM_MAX_CHANNELS];
		bool somethingChanged = false;

		if (auto mfx = dynamic_cast<EffectProcessor*>(p.get()))
		{
			auto thisSuspended = mfx->isCurrentlySuspended();

			auto change = thisSuspended != suspended;

			somethingChanged |= change;

			suspended = thisSuspended;
		}
		
		if (auto rp = dynamic_cast<RoutableProcessor*>(p.get()))
		{
			auto& mat = rp->getMatrix();
			thisNumChannels = mat.getNumSourceChannels();
			
			somethingChanged |= numChannels != thisNumChannels;

			if (thisNumChannels == 2)
			{
				thisData[0] = p->getDisplayValues().outL;
				thisData[1] = p->getDisplayValues().outR;
			}
			else
			{
				for (int i = 0; i < thisNumChannels; i++)
					thisData[i] = mat.getGainValue(i, true);
			}
		}
		else
		{
			thisData[0] = p->getDisplayValues().outL;
			thisData[1] = p->getDisplayValues().outR;
		}
		
		if (thisNumChannels != numChannels)
		{
			numChannels = thisNumChannels;
			findParentComponentOfClass<ModuleDragTarget>()->applyLayout();
			
		}
			
		for(int i = 0; i < thisNumChannels; i++)
		{
			auto decay = JUCE_LIVE_CONSTANT_OFF(0.7f);
			thisData[i] = jmax(thisData[i], channelValues[i] * decay);
            
			if (FloatSanitizers::isSilence(thisData[i]))
				thisData[i] = 0.0f;

			somethingChanged |= (thisData[i] != channelValues[i]);
		}

		if (somethingChanged)
		{
			

			numChannels = thisNumChannels;
			memcpy(channelValues, thisData, sizeof(float) * thisNumChannels);
			repaint();
		}
	}
	}
}

juce::Path PatchBrowser::Factory::createPath(const String& url) const
{
	Path p;
	LOAD_EPATH_IF_URL("add", EditorIcons::penShape);
	LOAD_PATH_IF_URL("workspace", ColumnIcons::openWorkspaceIcon);
	LOAD_EPATH_IF_URL("close", SampleMapIcons::deleteSamples);
	LOAD_EPATH_IF_URL("create", HiBinaryData::ProcessorEditorHeaderIcons::addIcon);
	LOAD_EPATH_IF_URL("folded", HiBinaryData::ProcessorEditorHeaderIcons::foldedIcon);
	LOAD_EPATH_IF_URL("unfolded", HiBinaryData::ProcessorEditorHeaderIcons::foldedIcon);

	if (url == "unfolded")
		p.applyTransform(AffineTransform::rotation(float_Pi * 0.5f));

	return p;
}

AutomationDataBrowser::AutomationCollection::ConnectionItem::ConnectionItem(AutomationData::Ptr d_, AutomationData::ConnectionBase::Ptr c_) :
	Item(d_->id),
	d(d_),
	c(c_)
{
	if (auto pc = dynamic_cast<AutomationData::ProcessorConnection*>(c.get()))
	{
		if (pc->connectedProcessor != nullptr)
			updater = new Updater(*this, pc->connectedProcessor);
	}

	setSize(380 - 16, ITEM_HEIGHT);
}

AutomationDataBrowser::AutomationCollection::ConnectionItem::~ConnectionItem()
{
	updater = nullptr;
}

void AutomationDataBrowser::AutomationCollection::ConnectionItem::paint(Graphics& g)
{
	String id = c->getDisplayString();
	auto pValue = c->getLastValue();

	if (d->lastValue != pValue)
		id << " (*)";

	Item::paintItemBackground(g, getLocalBounds().toFloat());

	auto nv = d->range.convertTo0to1(pValue);

	auto b = getLocalBounds().toFloat().reduced(4.0f);
	b = b.removeFromLeft(b.getWidth() * nv);

	g.setColour(Colours::white.withAlpha(0.1f));
	g.fillRect(b);

	g.setColour(Colours::white.withAlpha(d->lastValue != pValue ? 0.9f : 0.4f));
	g.setFont(GLOBAL_BOLD_FONT());
	g.drawText(id, getLocalBounds().toFloat().reduced(10, 0), Justification::left);
}

void AutomationDataBrowser::AutomationCollection::paint(Graphics& g)
{
	g.setColour(Colours::white);
	g.setFont(GLOBAL_MONOSPACE_FONT());

	auto v = data->range.convertTo0to1(data->lastValue);
	auto top = getLocalBounds().toFloat().removeFromTop(COLLECTION_HEIGHT).reduced(2.0f, 10.0f);

	String s;

	s << "#" << String(index) << " " << data->id << ": " << String(data->lastValue);

	g.drawText(s, top.reduced(10.0f, 0.0f), Justification::left);

	g.setColour(Colours::white.withAlpha(0.2f));
	g.drawRoundedRectangle(top, top.getHeight() / 2.0f, 1.0f);

	

	top = top.reduced(2.0f);

	auto copy = top;
	

	top = top.removeFromLeft(v * top.getWidth());
	
	g.fillRoundedRectangle(top, top.getHeight() * 0.5f);

	

	g.setColour(Colours::white);

	copy = copy.reduced(1.0f);

	Factory f;

	if (hasComponentConnection)
	{
		auto p = f.createPath("component");
		f.scalePath(p, copy.removeFromRight(copy.getHeight()));
		g.fillPath(p);
		copy.removeFromRight(5.0f);
	}

	if (hasMidiConnection)
	{
		auto p = f.createPath("midi");
		f.scalePath(p, copy.removeFromRight(copy.getHeight()));
		g.fillPath(p);
	}
}

AutomationDataBrowser::AutomationCollection::AutomationCollection(MainController* mc, AutomationData::Ptr data_, int index_) :
	ControlledObject(mc),
	SimpleTimer(mc->getGlobalUIUpdater()),
	Collection(),
	data(data_),
	NEW_AUTOMATION_WITH_COMMA(listener(mc->getRootDispatcher(), *this, [this](int, double){ this->repaint();}))
	index(index_)
{
	for (auto c_ : data->connectionList)
	{
		auto c = new ConnectionItem(data, c_);
		items.add(c);
		addAndMakeVisible(items.getLast());
	}

#if USE_OLD_AUTOMATION_DISPATCH
	data->asyncListeners.addListener(*this, [](AutomationCollection& c, int index, float v)
	{
		c.repaint();
	}, false);
#endif

	IF_NEW_AUTOMATION_DISPATCH(data->dispatcher.addValueListener(&listener, true, dispatch::DispatchType::sendNotificationAsync));

	checkIfChanged(false);
}

void AutomationDataBrowser::AutomationCollection::checkIfChanged(bool rebuildIfChanged)
{
	auto hasMidiConnectionNow = data->isConnectedToMidi();
	auto hasComponentConnectionNow = data->isConnectedToComponent();

	if (hasComponentConnection != hasComponentConnectionNow ||
		hasMidiConnection != hasMidiConnectionNow)
	{
		hasComponentConnection = hasComponentConnectionNow;
		hasMidiConnection = hasMidiConnectionNow;
		repaint();

		if (rebuildIfChanged)
		{
			if (auto p = findParentComponentOfClass<AutomationDataBrowser>())
			{
				if (p->midiButton->getToggleState() ||
					p->componentButton->getToggleState())
				{
					SafeAsyncCall::call<AutomationDataBrowser>(*p, [](AutomationDataBrowser& b)
						{
							b.rebuildModuleList(true);
						});
				}
			}

			return;
		}
	}
}

void AutomationDataBrowser::AutomationCollection::timerCallback()
{
	checkIfChanged(true);
}

AutomationDataBrowser::AutomationDataBrowser(BackendRootWindow* bw) :
	ControlledObject(bw->getBackendProcessor()),
	SearchableListComponent(bw)
{
	setFuzzyness(0.8);

	addAndMakeVisible(midiButton = new HiseShapeButton("midi", this, factory));
	midiButton->setToggleModeWithColourChange(true);
	midiButton->setTooltip("Show only MIDI learned data");
	midiButton->setToggleStateAndUpdateIcon(false);

	addCustomButton(midiButton);

	addAndMakeVisible(componentButton = new HiseShapeButton("component", this, factory));
	componentButton->setToggleModeWithColourChange(true);
	componentButton->setTooltip("Show only automation with connection to a script component");
	componentButton->setToggleStateAndUpdateIcon(false);

	addCustomButton(componentButton);

	updateList(*this, true);
}

void AutomationDataBrowser::buttonClicked(Button* b)
{
	rebuildModuleList(true);
}

int AutomationDataBrowser::getNumCollectionsToCreate() const
{
	auto numTotal = getMainController()->getUserPresetHandler().getNumCustomAutomationData();

	auto someFilterActive = midiButton->getToggleState() || componentButton->getToggleState();

	

	if (someFilterActive)
	{
		const auto numToSearch = numTotal;

		for (int i = 0; i < numToSearch; i++)
		{
			if (auto ptr = getMainController()->getUserPresetHandler().getCustomAutomationData(i))
			{
				if ((!ptr->isConnectedToMidi() && midiButton->getToggleState()) ||
					(!ptr->isConnectedToComponent() && componentButton->getToggleState()))
				{
					numTotal--;
				}
			}
		}
	}

	return numTotal;
}

hise::SearchableListComponent::Collection* AutomationDataBrowser::createCollection(int index)
{
	auto someFilterActive = midiButton->getToggleState() || componentButton->getToggleState();

	if (someFilterActive)
	{
		auto numTotal = getMainController()->getUserPresetHandler().getNumCustomAutomationData();

		int realIndex = -1;

		for (int i = 0; i < numTotal; i++)
		{
			if (auto ptr = getMainController()->getUserPresetHandler().getCustomAutomationData(i))
			{
				// check if it's included in the filter
				if ((ptr->isConnectedToMidi() || !midiButton->getToggleState()) &&
					(ptr->isConnectedToComponent() || !componentButton->getToggleState()))
				{
					realIndex++;
				}

				if(index == realIndex)
					return new AutomationCollection(getMainController(), ptr, i);
			}
		}

		// must not happen...
		jassertfalse;
		return nullptr;
	}
	else
	{
		if (auto ptr = getMainController()->getUserPresetHandler().getCustomAutomationData(index))
		{
			return new AutomationCollection(getMainController(), ptr, index);
		}
	}
    
    jassertfalse;
    return nullptr;
}

juce::Path AutomationDataBrowser::Factory::createPath(const String& url) const
{
	Path p;

	LOAD_EPATH_IF_URL("component", HiBinaryData::SpecialSymbols::macros);
	LOAD_EPATH_IF_URL("midi", HiBinaryData::SpecialSymbols::midiData);

	return p;
}

} // namespace hise
