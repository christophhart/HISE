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

MainMenuItem::MainMenuItem(ApplicationCommandManager *manager_, int commandId_) :
manager(manager_),
commandId(commandId_),
on(false)
{
	if (commandId == 0)
	{
		setSize(900 - 32 - 40, 20);
	}
	else
	{
		setSize(900 - 32 - 40, MAIN_MENU_ITEM_HEIGHT);
	}

	
}

void MainMenuItem::mouseUp(const MouseEvent &event)
{
	if (commandId == 0) return;
	on = false;
	repaint();

	if (MainMenuItem *up = getItemUnderMouse(event))
	{
		manager->invokeDirectly(up->commandId, false);
		triggerMenuItem();
	}
}

void MainMenuItem::triggerMenuItem()
{
	if (commandId == 0) return;
	findParentComponentOfClass<BackendProcessorEditor>()->clearPopup();
}

MainMenuItem * MainMenuItem::getItemUnderMouse(const MouseEvent &event)
{
	Component *c = getParentComponent();

	if (c != nullptr)
	{
		Point<int> mouseUpPointInParent = c->getLocalPoint(this, event.getPosition());

		Component* upComponent = c->getComponentAt(mouseUpPointInParent);

		return dynamic_cast<MainMenuItem*>(upComponent);
	}
}

void MainMenuItem::mouseDrag(const MouseEvent& event)
{
	if (MainMenuItem *item = getItemUnderMouse(event))
	{
		findParentComponentOfClass<MainMenuContainer>()->clearOnStateForAllItems();
		item->on = true;
		item->repaint();
	}
}

void MainMenuItem::mouseDown(const MouseEvent& event)
{
	if (commandId == 0) return;
	on = true;
	repaint();
}

void MainMenuItem::paint(Graphics& g)
{
	if (commandId == 0)
	{
		g.setGradientFill(ColourGradient(Colours::black.withAlpha(0.1f), 0.0f, 0.0f,
			Colours::transparentBlack, 0.0f, 16.0f, false));


		g.fillRect(0, 0, getWidth(), 16);
	}
	else
	{
		g.setGradientFill(ColourGradient(Colour(0xFF404040).withMultipliedBrightness(on ? 1.2f : 1.0f), 0.0f, 0.0f,
			Colour(0xFF353535).withMultipliedBrightness(on ? 1.1f : 1.0f), 0.0f, MAIN_MENU_ITEM_HEIGHT, false));

		g.fillAll();

		g.setColour(Colours::white.withAlpha(0.1f));
		g.drawLine(0.0f, 0.0f, (float)getWidth(), 0.0f);

		g.setColour(Colours::black.withAlpha(0.1f));
		g.drawLine(0.0f, (float)getHeight() - 1.0f, (float)getWidth(), (float)getHeight() - 1.0f);

		g.setFont(GLOBAL_BOLD_FONT().withHeight(18.0f));

		const String name = manager->getCommandForID(commandId)->shortName;

		g.setColour(Colours::white);

		g.drawText(name, getLocalBounds().reduced(5), Justification::centred);
	}
	
}


// ============================================================================================

MainMenuContainer::MainMenuContainer(ApplicationCommandManager *manager, const Array<int> &ids)
{
	for (int i = 0; i < ids.size(); i++)
	{
		MainMenuItem *item = new MainMenuItem(manager, ids[i]);

		addAndMakeVisible(item);

		items.add(item);
	}

	setSize(900 - 32, 40 + items.size() * MAIN_MENU_ITEM_HEIGHT);
}

void MainMenuContainer::clearOnStateForAllItems()
{
	for (int i = 0; i < items.size(); i++)
	{
		items[i]->clearOnState();
	}
}

void MainMenuContainer::resized()
{
	int y = 20;

	for (int i = 0; i < items.size(); i++)
	{
		items[i]->setTopLeftPosition(20, y);
		y += items[i]->getHeight();
	}
}
