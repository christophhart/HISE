/*
  ==============================================================================

    FloatingInterfaceBuilder.cpp
    Created: 15 May 2017 7:58:54pm
    Author:  Christoph

  ==============================================================================
*/



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
			c->getComponent(i)->getLayoutData().currentSize = sizes[i];
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
			c->getComponent(i)->getLayoutData().currentSize = foldStates[i];
		}

		if (shouldUpdateLayout == sendNotification)
			c->refreshLayout();
	}
	else
		jassertfalse;
}

void FloatingInterfaceBuilder::setAbsoluteSize(int index, Array<bool> absoluteState, NotificationType shouldUpdateLayout /*= sendNotification*/)
{
	if (auto c = getTileManager(index))
	{
		if (absoluteState.size() != c->getNumComponents())
		{
			jassertfalse;
			return;
		}

		for (int i = 0; i < c->getNumComponents(); i++)
		{
			c->getComponent(i)->getLayoutData().isAbsolute = absoluteState[i];
		}

		if (shouldUpdateLayout == sendNotification)
			c->refreshLayout();
	}
	else
		jassertfalse;
}

void FloatingInterfaceBuilder::setLocked(int index, Array<bool> lockedStates, NotificationType shouldUpdateLayout/*=sendNotification*/)
{
	if (auto c = getTileManager(index))
	{
		if (lockedStates.size() != c->getNumComponents())
		{
			jassertfalse;
			return;
		}

		for (int i = 0; i < c->getNumComponents(); i++)
		{
			c->getComponent(i)->getLayoutData().isLocked = lockedStates[i];
		}

		if (shouldUpdateLayout == sendNotification)
			c->refreshLayout();
	}
	else
		jassertfalse;
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

void FloatingInterfaceBuilder::removeFirstChildOfNewContainer(FloatingTile* panel)
{
	if (auto c = dynamic_cast<FloatingTileContainer*>(panel->getCurrentFloatingPanel()))
		c->removeFloatingTile(c->getComponent(0));
}


FloatingTile* FloatingInterfaceBuilder::finalizeAndReturnRoot(bool isReadOnly)
{
	createdComponents.getFirst()->resized();

	FloatingTile::Iterator<HorizontalTile> it(createdComponents.getFirst());

	while (auto c = it.getNextPanel())
	{
		c->refreshLayout();
	}

	return createdComponents.getFirst();
}