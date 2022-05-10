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
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

namespace hise { using namespace juce;

ModulatorPeakMeter::ModulatorPeakMeter(Modulator *m) :
mod(m)
{
	addAndMakeVisible(vuMeter = new VuMeter());

	vuMeter->setType(VuMeter::MonoVertical);

	vuMeter->setColour(VuMeter::ledColour, Colour(0x88dddddd));

	startTimer(40);

	vuMeter->addMouseListener(this, true);
}

ScriptContentComponent::ScriptContentComponent(ProcessorWithScriptingContent *p_) :
	AsyncValueTreePropertyListener(p_->getScriptingContent()->getContentProperties(), p_->getScriptingContent()->getUpdateDispatcher()),
	contentRebuildNotifier(*this),
	modalOverlay(*this),
	processor(p_),
	p(dynamic_cast<Processor*>(p_))
{
	processor->getScriptingContent()->addRebuildListener(this);
	processor->getScriptingContent()->addScreenshotListener(this);

    setNewContent(processor->getScriptingContent());

	setInterceptsMouseClicks(false, true);

    setWantsKeyboardFocus(true);
    
	p->addDeleteListener(this);

	p->addChangeListener(this);
	p->getMainController()->addScriptListener(this, true);

	addChildComponent(modalOverlay);
}

ScriptContentComponent::~ScriptContentComponent()
{
	if (contentData.get() != nullptr)
	{
		for (int i = 0; i < contentData->getNumComponents(); i++)
		{
			contentData->getComponent(i)->removeChangeListener(this);
		}

		contentData->removeRebuildListener(this);
		contentData->addScreenshotListener(this);
	}

	if (p.get() != nullptr)
	{
		p->getMainController()->removeScriptListener(this);
		p->removeChangeListener(this);
		p->removeDeleteListener(this);
	};

	componentWrappers.clear();
}



void ScriptContentComponent::refreshMacroIndexes()
{
	if (p == nullptr)
		return;

	MacroControlBroadcaster *mcb = p->getMainController()->getMacroManager().getMacroChain();

	for(int i = 0; i < componentWrappers.size(); i++)
	{
		int macroIndex = mcb->getMacroControlIndexForProcessorParameter(p, i);

		if(macroIndex != -1)
		{
			MacroControlBroadcaster::MacroControlledParameterData * pData = mcb->getMacroControlData(macroIndex)->getParameterWithProcessorAndIndex(p, i);

			// Check if the name matches
			if(pData->getParameterName() != componentWrappers[i]->getComponent()->getName())
			{
				const String x = pData->getParameterName();

				mcb->getMacroControlData(macroIndex)->removeParameter(x);

				p->getMainController()->getMacroManager().getMacroChain()->sendChangeMessage();

				debugToConsole(p, "Index mismatch: Removed Macro Control for " + x);
			}

		}
	}
}


String ScriptContentComponent::getContentTooltip() const
{
	if (contentData != nullptr)
	{
		return contentData->tooltip;
	}
	else return String();
}

Colour ScriptContentComponent::getContentColour()
{
	if (contentData != nullptr)
	{
		return contentData->colour;
	}
	else
	{
		return Colour(0xff777777);
	}
}

void ScriptContentComponent::updateValue(int i)
{
	MacroControlledObject *o = dynamic_cast<MacroControlledObject*>(componentWrappers[i]->getComponent());

	if (o != nullptr)
	{
		o->updateValue(dontSendNotification);
	}

	if (Slider *s = dynamic_cast<Slider*>(componentWrappers[i]->getComponent()))
	{
		if (s->getSliderStyle() == Slider::TwoValueHorizontal)
		{
			const double min = dynamic_cast<ScriptingApi::Content::ScriptSlider*>(contentData->components[i].get())->getMinValue();
			const double max = dynamic_cast<ScriptingApi::Content::ScriptSlider*>(contentData->components[i].get())->getMaxValue();

			s->setMinValue(min, dontSendNotification);
			s->setMaxValue(max, dontSendNotification);
		}
	}
}

void ScriptContentComponent::updateValues()
{
	for (int i = 0; i < componentWrappers.size(); i++)
	{
		updateValue(i);
	}
}

void ScriptContentComponent::changeListenerCallback(SafeChangeBroadcaster *b)
{
	if (contentData.get() == nullptr) return;

	if (p.get() == nullptr)
	{
		setEnabled(false);
	}

	if (p == b)
	{
#if USE_BACKEND
		updateValues();
#endif
	}
	
	else if (auto sc = dynamic_cast<ScriptingApi::Content::ScriptComponent*>(b))
	{
		auto index = contentData->getComponentIndex(sc->name);

		if (index == -1)
			return;

		if (auto w = componentWrappers[index])
		{
			w->updateValue(sc->getValue());
		}
	}
	else
	{
		updateContent();
	}
}

void ScriptContentComponent::asyncValueTreePropertyChanged(ValueTree& /*v*/, const Identifier& /*id*/)
{
	
}

void ScriptContentComponent::valueTreeChildAdded(ValueTree& /*parent*/, ValueTree& /*child*/)
{

}

void ScriptContentComponent::updateComponent(int i)
{
    if(componentWrappers[i]->getComponent() == nullptr)
    {
        jassertfalse;
        return;
    }
    
	updateComponentVisibility(componentWrappers[i]);
	
	updateComponentParent(componentWrappers[i]);

	updateComponentPosition(componentWrappers[i]);

	//componentWrappers[i]->updateComponent();
	updateValue(i);
}

void ScriptContentComponent::updateContent(ScriptingApi::Content::ScriptComponent* componentToUpdate/*=nullptr*/)
{
	if (contentData.get() == nullptr) return;

	if (componentToUpdate == nullptr)
	{

		jassert(contentData->components.size() == componentWrappers.size());

		for (int i = 0; i < contentData->components.size(); i++)
		{
			updateComponent(i);
		}

		//resized();
	}
	else
	{

		jassert(contentData->components.size() == componentWrappers.size());

		auto index = contentData->components.indexOf(componentToUpdate);


		if (index >= 0)
			updateComponent(index);
		else
			jassertfalse;
	}
}

void ScriptContentComponent::updateComponentPosition(ScriptCreatedComponentWrapper* wrapper)
{
	auto c = wrapper->getComponent();
	auto sc = wrapper->getScriptComponent();

	const Rectangle<int> localBounds = c->getBoundsInParent();
	const Rectangle<int> currentPosition = sc->getPosition();

	if (localBounds != currentPosition)
	{
		const bool isInViewport = dynamic_cast<Viewport*>(c->getParentComponent()) != nullptr;
		const bool sizeChanged = localBounds.getWidth() != currentPosition.getWidth() || localBounds.getHeight() != currentPosition.getHeight();

		if (!isInViewport || sizeChanged)
			c->setBounds(sc->getPosition());

	}
}

void ScriptContentComponent::updateComponentVisibility(ScriptCreatedComponentWrapper* wrapper)
{
	auto sc = wrapper->getScriptComponent();
	auto c = wrapper->getComponent();

	const bool v = sc->isShowing(false);
	c->setVisible(v);

	if (!v && !sc->isShowing(true))
	{
		return;
	}

	const bool e = sc->getScriptObjectProperty(ScriptingApi::Content::ScriptComponent::enabled);
	wrapper->getComponent()->setEnabled(e);
	wrapper->getComponent()->setInterceptsMouseClicks(sc->isClickable(), true);

}

void ScriptContentComponent::updateComponentParent(ScriptCreatedComponentWrapper* wrapper)
{
	auto c = wrapper->getComponent();

	if (c == nullptr || c->getParentComponent() == nullptr)
	{
		return;
	}

	auto sc = wrapper->getScriptComponent();

	const Rectangle<int> currentPosition = sc->getPosition();



	auto currentParentId = c->getParentComponent()->getName();
	auto newParentId = sc->getScriptObjectProperty(ScriptingApi::Content::ScriptComponent::Properties::parentComponent).toString();

	if (currentParentId != newParentId)
	{
		auto componentToRemove = c;

		if (newParentId.isEmpty())
		{
			componentToRemove->getParentComponent()->removeChildComponent(componentToRemove);

			addChildComponent(componentToRemove);
			componentToRemove->setBounds(currentPosition);
		}
		else
		{
			for (int cIndex = 0; cIndex < componentWrappers.size(); cIndex++)
			{
				if (componentWrappers[cIndex]->getComponent()->getName() == newParentId)
				{
					auto newParent = componentWrappers[cIndex]->getComponent();
					componentToRemove->getParentComponent()->removeChildComponent(componentToRemove);
					newParent->addChildComponent(componentToRemove);
					componentToRemove->setBounds(currentPosition);
					break;
				}
			}
		}

		updateComponentPosition(wrapper);
	}
}

void ScriptContentComponent::resized()
{
	modalOverlay.setBounds(getLocalBounds());

	if (!contentValid())
	{
		return;
	}

	for (int i = 0; i < componentWrappers.size(); i++)
	{
		Component *c = componentWrappers[i]->getComponent();

		if (c->getLocalBounds() != contentData->components[i]->getPosition())
		{
			c->setBounds(contentData->components[i]->getPosition());
		}
	}
}

void ScriptContentComponent::setModalPopup(ScriptCreatedComponentWrapper* wrapper, bool shouldShow)
{
	if (shouldShow)
	{
		modalOverlay.showFor(wrapper);
	}
	else
		modalOverlay.closeModalPopup();
}

void ScriptContentComponent::scriptWasCompiled(JavascriptProcessor *jp)
{
	if (jp == getScriptProcessor())
	{
		contentRebuildNotifier.notify(processor->getScriptingContent());
	}
}

void ScriptContentComponent::makeScreenshot(const File& target, Rectangle<float> area)
{
	WeakReference<ScriptContentComponent> safeThis(this);

	auto f = [safeThis, target, area]()
	{
		if (safeThis != nullptr)
		{
			ScriptingObjects::ScriptShader::ScopedScreenshotRenderer ssr;

			auto sf = UnblurryGraphics::getScaleFactorForComponent(safeThis.get());

			safeThis->repaint();

			auto img = safeThis->createComponentSnapshot(area.toNearestInt(), true, sf);

			juce::PNGImageFormat png;

			target.deleteFile();

			FileOutputStream fos(target);

			auto ok = png.writeImageToStream(img, fos);

			if (ok)
			{
				debugToConsole(dynamic_cast<Processor*>(safeThis->processor), "Screenshot exported as " + target.getFullPathName());
			}
		}
	};

	MessageManager::callAsync(f);
}

void ScriptContentComponent::visualGuidesChanged()
{
	Component::SafePointer<Component> safeThis(this);

	auto f = [safeThis]()
	{
		if(safeThis != nullptr)
			safeThis->repaint();
	};

	MessageManager::callAsync(f);
}

void ScriptContentComponent::prepareScreenshot()
{
	MessageManagerLock mm;
	repaint();
}

void ScriptContentComponent::contentWasRebuilt()
{
	contentRebuildNotifier.notify(processor->getScriptingContent());
}



void ScriptContentComponent::setNewContent(ScriptingApi::Content *c)
{
	if (c == nullptr) return;

	contentData = c;

	deleteAllScriptComponents();

	valuePopupProperties = new ScriptCreatedComponentWrapper::ValuePopup::Properties(p->getMainController(), c->getValuePopupProperties());

	for (int i = 0; i < contentData->components.size(); i++)
	{
		auto sc = contentData->components[i].get();
		
		sc->addChangeListener(this);

		componentWrappers.add(sc->createComponentWrapper(this, i));

		auto newComponent = componentWrappers.getLast()->getComponent();

		if (auto scParent = sc->getParentScriptComponent())
		{
			auto pOfSc = getComponentFor(scParent);

			if (auto vp = dynamic_cast<Viewport*>(pOfSc))
			{
				vp->setViewedComponent(newComponent, false);
			}
			else
			{
				pOfSc->addChildComponent(newComponent);
				newComponent->setVisible(sc->isShowing(false));
			}
		}
		else
		{
			addChildComponent(newComponent);
			newComponent->setVisible(sc->isShowing(false));
		}
	}

	refreshMacroIndexes();
	refreshContentButton();

	if (getWidth() != 0) setSize(getWidth(), getContentHeight());

	updateContent();
    
    addMouseListenersForComponentWrappers();
	repaint();
}

void ScriptContentComponent::addMouseListenersForComponentWrappers()
{
    if (getParentComponent() != nullptr)
    {
        for (int i = 0; i < componentWrappers.size(); i++)
        {
            componentWrappers[i]->getComponent()->addMouseListener(getParentComponent(), true);
        }
    }
}

void ScriptContentComponent::deleteAllScriptComponents()
{
	for (auto w : componentWrappers)
	{
		w->getScriptComponent()->removeChangeListener(this);
	}

	componentWrappers.clear();
}

void ScriptContentComponent::refreshContentButton()
{
#if USE_BACKEND

	if(ScriptingEditor *e = dynamic_cast<ScriptingEditor*>(getParentComponent()))
	{
		e->checkContent();
	}

#endif

}

bool ScriptContentComponent::keyPressed(const KeyPress &/*key*/)
{
	return false;
}

void ScriptContentComponent::processorDeleted(Processor* /*deletedProcessor*/)
{
	contentData->resetContentProperties();
	deleteAllScriptComponents();
}

void ScriptContentComponent::paint(Graphics &g)
{
	if(findParentComponentOfClass<FloatingTilePopup>() == nullptr)
		g.fillAll(JUCE_LIVE_CONSTANT_OFF(Colour(0xff252525)));
}

void ScriptContentComponent::paintOverChildren(Graphics& g)
{
#if USE_BACKEND

	if (p.get() == nullptr)
		return;
	
	const auto& guides = processor->getScriptingContent()->guides;

	if (!guides.isEmpty() && !ScriptingObjects::ScriptShader::isRenderingScreenshot())
	{
		UnblurryGraphics ug(g, *this, true);

		for (const auto& vg : guides)
		{
			g.setColour(vg.c);

			if (vg.t == ScriptingApi::Content::VisualGuide::Type::HorizontalLine)
				ug.draw1PxHorizontalLine(vg.area.getY(), vg.area.getX(), vg.area.getRight());
			if (vg.t == ScriptingApi::Content::VisualGuide::Type::VerticalLine)
				ug.draw1PxVerticalLine(vg.area.getX(), vg.area.getY(), vg.area.getBottom());
			if (vg.t == ScriptingApi::Content::VisualGuide::Type::Rectangle)
				ug.draw1PxRect(vg.area);
		}
	}
#endif

	if (isRebuilding)
	{
		g.fillAll(Colours::black.withAlpha(0.8f));
		g.setColour(Colours::white);
		g.setFont(GLOBAL_BOLD_FONT());
		g.drawText("Rebuilding...", 0, 0, getWidth(), getHeight(), Justification::centred, false);
	}
}

ScriptingApi::Content::ScriptComponent * ScriptContentComponent::getScriptComponentFor(Point<int> pos)
{
	for (int i = componentWrappers.size() - 1; i >= 0; --i)
	{
		Component* c = componentWrappers[i]->getComponent();

		if (!c->isVisible()) continue;

		Component* parentOfC = c->getParentComponent();

		if (getLocalArea(parentOfC, c->getBounds()).contains(pos))
		{
			return contentData->getComponent(i);
		}
	}

	return nullptr;
}

ScriptingApi::Content::ScriptComponent* ScriptContentComponent::getScriptComponentFor(Component* component)
{
	for (int i = 0; i < componentWrappers.size(); i++)
	{
		if (contentData.get() != nullptr && componentWrappers[i]->getComponent() == component)
		{
			return contentData->getComponent(i);
		}
	}

	return nullptr;
}

Component* ScriptContentComponent::getComponentFor(ScriptingApi::Content::ScriptComponent* sc)
{
	if (sc == nullptr)
		return nullptr;

	if (contentData != nullptr)
	{
		auto index = contentData->getComponentIndex(sc->getName());

		if (index != -1)
		{
			auto cw = componentWrappers[index];
			if (cw != nullptr)
			{
				return cw->getComponent();
			}
		}
	}

	return nullptr;
}

void ScriptContentComponent::getScriptComponentsFor(Array<ScriptingApi::Content::ScriptComponent*> &arrayToFill, Point<int> pos)
{
	for (int i = componentWrappers.size() - 1; i >= 0; --i)
	{
		Component* c = componentWrappers[i]->getComponent();

		if (componentWrappers[i]->getScriptComponent()->isLocked())
			continue;

		Component* parentOfC = c->getParentComponent();

		if (getLocalArea(parentOfC, c->getBounds()).contains(pos))
		{
			arrayToFill.add(contentData->getComponent(i));
		}
	}
}

void ScriptContentComponent::getScriptComponentsFor(Array<ScriptingApi::Content::ScriptComponent*> &arrayToFill, const Rectangle<int> area)
{
	arrayToFill.clear();

	for (int i = componentWrappers.size() - 1; i >= 0; --i)
	{
		auto sc = contentData->getComponent(i);

		Component* c = componentWrappers[i]->getComponent();

		if (sc == nullptr || !sc->isShowing())
			continue;

		Component* parentOfC = c->getParentComponent();

		auto cBounds = getLocalArea(parentOfC, c->getBounds());

		if (sc->isLocked())
			continue;

		if (cBounds.contains(area))
			continue;

		auto pBounds = parentOfC->getBounds();

		if (area.intersects(cBounds) && area.intersects(pBounds))
		{
			arrayToFill.addIfNotAlreadyThere(sc);
		}
	}
}


MarkdownPreviewPanel::MarkdownPreviewPanel(FloatingTile* parent) :
	FloatingTileContent(parent)
{
	setDefaultPanelColour(PanelColourId::bgColour, Colours::transparentBlack);
	setDefaultPanelColour(PanelColourId::itemColour1, Colour(SIGNAL_COLOUR));
	setDefaultPanelColour(PanelColourId::itemColour2, Colours::slategrey);
	setDefaultPanelColour(PanelColourId::textColour, Colours::white);
	setDefaultPanelColour(PanelColourId::itemColour3, Colour(0xFF222222));
}

void MarkdownPreviewPanel::initPanel()
{
	if (preview != nullptr)
		return;

	MarkdownDatabaseHolder* holder;
	bool isProjectDoc = false;

#if USE_BACKEND
	if (!getParentShell()->isOnInterface() && !getMainController()->isFlakyThreadingAllowed())
	{
		holder = dynamic_cast<BackendProcessor*>(getMainController())->getDocProcessor();
		isProjectDoc = false;
	}
	else
	{
		holder = getMainController()->getProjectDocHolder();
		isProjectDoc = true;
	}
#else
	holder = getMainController()->getProjectDocHolder();
	isProjectDoc = true;

#endif

	addAndMakeVisible(preview = new HiseMarkdownPreview(*holder));

    options = (int)MarkdownPreview::ViewOptions::Edit;
    
    if (showSearch || showBack)
        options |= (int)MarkdownPreview::ViewOptions::Topbar;
    
    if (showSearch)
        options |= (int)MarkdownPreview::ViewOptions::Search;
    
    if (showBack)
        options |= (int)MarkdownPreview::ViewOptions::Back;
    
    if (showToc)
        options |= (int)MarkdownPreview::ViewOptions::Toc;
    
	preview->setViewOptions(options);
	preview->toc.fixWidth = fixWidth;
	preview->toc.setBgColour(findPanelColour(PanelColourId::itemColour3));
	preview->renderer.setCreateFooter(holder->getDatabase().createFooter);
	preview->renderer.setStyleData(sd);

	preview->setStyleData(sd);

	getMainController()->setCurrentMarkdownPreview(preview);

	if (customContent.isNotEmpty())
		preview->setNewText(customContent, File(), true);
	else if (isProjectDoc)
	{
		holder->rebuildDatabase();
		preview->renderer.gotoLink(MarkdownLink(holder->getDatabaseRootDirectory(), startURL));
		BACKEND_ONLY(preview->editingEnabled = true);
		
	}

	visibilityChanged();
	resized();
}

Component* ScriptContentComponent::SimpleTraverser::getDefaultComponent(Component* parentComponent)
{
	if(parentComponent->getWantsKeyboardFocus())
		return parentComponent;

	return nullptr;
}

Component* ScriptContentComponent::SimpleTraverser::getNextComponent(Component* current)
{
	return nullptr;
}

Component* ScriptContentComponent::SimpleTraverser::getPreviousComponent(Component* current)
{
	return nullptr;
}

std::vector<Component*> ScriptContentComponent::SimpleTraverser::getAllComponents(Component* parentComponent)
{
	return {};
}

} // namespace hise
