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
#ifndef MAINMENUCOMPONENT_H_INCLUDED
#define MAINMENUCOMPONENT_H_INCLUDED



#define MAIN_MENU_ITEM_HEIGHT 50

class MainMenuItem : public Component
{
public:
	MainMenuItem(ApplicationCommandManager *manager_, int commandId_);

	void mouseUp(const MouseEvent &event);

	void triggerMenuItem();

	MainMenuItem *getItemUnderMouse(const MouseEvent &event);

	void mouseDrag(const MouseEvent& event);

	void mouseDown(const MouseEvent& event);

	void clearOnState()
	{
		on = false;
		repaint();
	}


	void paint(Graphics& g);

private:

	bool on;

	int commandId;
	BackendProcessorEditor *editor;
	ApplicationCommandManager *manager;
};

class MainMenuContainer : public Component
{
public:

	MainMenuContainer(ApplicationCommandManager *manager, const Array<int> &ids);

	void clearOnStateForAllItems();

	

	void resized();

private:

	OwnedArray<MainMenuItem> items;
};



#endif  // MAINMENUCOMPONENT_H_INCLUDED
