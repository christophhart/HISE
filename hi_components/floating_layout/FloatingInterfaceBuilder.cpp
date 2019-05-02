/*
  ==============================================================================

    FloatingInterfaceBuilder.cpp
    Created: 15 May 2017 7:58:54pm
    Author:  Christoph

  ==============================================================================
*/

namespace hise { using namespace juce;

void FloatingInterfaceBuilder::setSizes(int index, Array<double> sizes, NotificationType shouldUpdateLayout/*=sendNotification*/)
{
	if (auto c = getTileManager(index))
	{
		if (sizes.size() != c->getNumComponents())
		{
			jassertfalse;
			return;
		}

		for (int i = 0; i < c->getNumComponents(); i++)
		{
			c->getComponent(i)->getLayoutData().setCurrentSize(sizes[i]);
		}

		if (shouldUpdateLayout == sendNotification)
			c->refreshLayout();
	}
	else
		jassertfalse;
}

void FloatingInterfaceBuilder::setFolded(int index, Array<bool> foldStates, NotificationType shouldUpdateLayout /*= sendNotification*/)
{
	if (auto c = getTileManager(index))
	{
		if (foldStates.size() != c->getNumComponents())
		{
			jassertfalse;
			return;
		}

		for (int i = 0; i < c->getNumComponents(); i++)
		{
			c->getComponent(i)->getLayoutData().setFoldState(foldStates[i]);
		}

		if (shouldUpdateLayout == sendNotification)
			c->refreshLayout();
	}
	else
		jassertfalse;
}


void FloatingInterfaceBuilder::setVisibility(int index, bool shouldBeVisible, Array<bool> childVisibleStates, NotificationType shouldUpdateLayout /*= dontSendNotification*/)
{
	getPanel(index)->getLayoutData().setVisible(shouldBeVisible);

	if (!childVisibleStates.isEmpty())
	{
		if (auto c = getTileManager(index)) // Tabs don't need to toggle the visibility of their children
		{
			if (childVisibleStates.size() != c->getNumComponents())
			{
				jassertfalse;
				return;
			}

			for (int i = 0; i < c->getNumComponents(); i++)
				c->getComponent(i)->getLayoutData().setVisible(childVisibleStates[i]);
		}
		else
			jassertfalse;
	}

	if (shouldUpdateLayout)
		getPanel(index)->refreshRootLayout();
}

void FloatingInterfaceBuilder::setFoldable(int index, bool isFoldable, Array<bool> childFoldableStates, NotificationType /*= dontSendNotification*/)
{
	getPanel(index)->setCanBeFolded(isFoldable);

	if (auto c = getContainer(index))
	{
		if (childFoldableStates.size() != c->getNumComponents())
		{
			jassertfalse;
			return;
		}

		for (int i = 0; i < c->getNumComponents(); i++)
			c->getComponent(i)->setCanBeFolded(childFoldableStates[i]);
	}
	else
		jassertfalse;
}

void FloatingInterfaceBuilder::setDynamic(int index, bool shouldBeDynamic)
{
	getContainer(index)->setIsDynamic(shouldBeDynamic);
}

void FloatingInterfaceBuilder::setCustomName(int index, const String& name, Array<String> names)
{
	if (auto p = getPanel(index))
		p->getCurrentFloatingPanel()->setCustomTitle(name);
	else
		jassertfalse;

	if (names.size() > 0)
	{
		if (auto c = getContainer(index))
		{
			if (names.size() != c->getNumComponents())
			{
				jassertfalse;
				return;
			}

			for (int i = 0; i < c->getNumComponents(); i++)
				c->getComponent(i)->getCurrentFloatingPanel()->setCustomTitle(names[i]);
		}
		else
			jassertfalse;
	}

}


void FloatingInterfaceBuilder::setCustomPanels(int toggleBarIndex, Array<int> panels)
{
	auto tb = getContent<VisibilityToggleBar>(toggleBarIndex);

	jassert(tb != nullptr);

	for (int i = 0; i < panels.size(); i++)
	{
		tb->addCustomPanel(getPanel(panels[i]));
	}

	tb->refreshButtons();
}

void FloatingInterfaceBuilder::setId(int index, const String& newID)
{
	getPanel(index)->getLayoutData().setId(newID);
}



int FloatingInterfaceBuilder::addChild(int index, const Identifier& panelId)
{
	auto c = createdComponents[index].getComponent();

	if (c != nullptr)
	{
		if (auto container = dynamic_cast<FloatingTileContainer*>(c->getCurrentFloatingPanel()))
		{
			auto newPanel = new FloatingTile(container->getParentShell()->getMainController(), container);

			container->addFloatingTile(newPanel);

			createdComponents.add(newPanel);

			newPanel->setNewContent(panelId);

			removeFirstChildOfNewContainer(newPanel);

			return createdComponents.size() - 1;
		}
	}

	return -1;
}

FloatingTile* FloatingInterfaceBuilder::getPanel(int index)
{
	return createdComponents[index].getComponent();
}

FloatingTileContainer* FloatingInterfaceBuilder::getContainer(int index)
{
	if (auto p = getPanel(index))
		return dynamic_cast<FloatingTileContainer*>(p->getCurrentFloatingPanel());

	return nullptr;
}

ResizableFloatingTileContainer* FloatingInterfaceBuilder::getTileManager(int index)
{
	if (auto p = getPanel(index))
		return dynamic_cast<ResizableFloatingTileContainer*>(p->getCurrentFloatingPanel());

	return nullptr;
}

FloatingTileContent* FloatingInterfaceBuilder::getContent(int index)
{
	return getPanel(index)->getCurrentFloatingPanel();
}

void FloatingInterfaceBuilder::removeFirstChildOfNewContainer(FloatingTile* panel)
{
	if (auto c = dynamic_cast<FloatingTileContainer*>(panel->getCurrentFloatingPanel()))
		c->removeFloatingTile(c->getComponent(0));
}


FloatingTile* FloatingInterfaceBuilder::finalizeAndReturnRoot()
{
	createdComponents.getFirst()->resized();

	FloatingTile::Iterator<HorizontalTile> it(createdComponents.getFirst());

	while (auto c = it.getNextPanel())
	{
		c->refreshLayout();
	}

	return createdComponents.getFirst();
}

} // namespace hise