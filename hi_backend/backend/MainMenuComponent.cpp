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


void MainMenuItem::mouseUp(const MouseEvent &event)
{
	if (isPlaceHolder()) return;
	on = false;
	repaint();

	if (MainMenuItem *up = getItemUnderMouse(event))
	{
		up->perform();
		triggerMenuItem();
	}
}

void MainMenuItem::triggerMenuItem()
{
	if (isPlaceHolder()) return;
	findParentComponentOfClass<BackendProcessorEditor>()->clearPopup();
}

void MainMenuItem::setupSize()
{
	if (isPlaceHolder())
	{
		setSize(900 - 32 - 40, 20);
	}
	else
	{
		setSize(900 - 32 - 40, MAIN_MENU_ITEM_HEIGHT);
	}
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

	return nullptr;
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

void MainMenuItem::mouseDown(const MouseEvent& )
{
	if(isPlaceHolder()) return;

	on = true;
	repaint();
}

void MainMenuItem::paint(Graphics& g)
{
	if (isPlaceHolder())
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

		const String name = getName();

		g.setColour(Colours::white);

		g.drawText(name, getLocalBounds().reduced(5), Justification::centred);
	}
	
}


// ============================================================================================

void MainMenuContainer::addCommandIds(ApplicationCommandManager *manager, const Array<int> &ids)
{
	items.clear();

	for (int i = 0; i < ids.size(); i++)
	{
		CommandMenuItem *item = new CommandMenuItem(manager, ids[i]);

		addAndMakeVisible(item);

		items.add(item);
	}

	setSize(900 - 32, 768);
}

void MainMenuContainer::addFileIds(const Array<File> &files, BackendProcessorEditor *bpe)
{
	items.clear();

	int y = 20;

	for (int i = 0; i < files.size(); i++)
	{
		FileMenuItem::Action actionToDo;

		if (files[i].isDirectory()) actionToDo = FileMenuItem::Action::OpenProject;
		else if (files[i].hasFileExtension(".hip")) actionToDo = FileMenuItem::Action::OpenPreset;
		else actionToDo = FileMenuItem::Action::OpenXmlPreset;

		FileMenuItem *item = new FileMenuItem(files[i], actionToDo, bpe);

		addAndMakeVisible(item);

		y += item->getHeight();

		items.add(item);
	}

	setSize(900 - 32, y);
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

CommandMenuItem::CommandMenuItem(ApplicationCommandManager *manager_, int commandId_):
MainMenuItem(),
manager(manager_),
commandId(commandId_)
{
	setupSize();
}

void CommandMenuItem::perform()
{
	manager->invokeDirectly(commandId, true);
}

String CommandMenuItem::getName()
{
	return manager->getCommandForID(commandId)->shortName;
}

void FileMenuItem::perform()
{
	switch (actionToDo)
	{
	case FileMenuItem::Action::OpenProject:
		GET_PROJECT_HANDLER(bpe->getMainSynthChain()).setWorkingProject(file, bpe);
		break;
	case FileMenuItem::Action::OpenXmlPreset:
		BackendCommandTarget::Actions::openFileFromXml(bpe, file);
		break;
	case FileMenuItem::Action::OpenPreset:
		bpe->loadNewContainer(file);
		break;
	default:
		break;
	}
}
