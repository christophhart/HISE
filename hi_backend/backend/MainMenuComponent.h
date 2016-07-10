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
	
	MainMenuItem() :
		on(false)
	{};

	void mouseUp(const MouseEvent &event);

	void triggerMenuItem();

	/** Overwrite this method and do your stuff.
	*/
	virtual void perform() = 0;

	virtual String getName() = 0;
	
	virtual bool isPlaceHolder() { return false; };

	void setupSize();

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

};

class CommandMenuItem : public MainMenuItem
{
public:

	CommandMenuItem(ApplicationCommandManager *manager_, int commandId_);

	void perform() override;

	String getName() override;

	bool isPlaceHolder() override { return commandId == 0; }

	int commandId;
	BackendProcessorEditor *editor;
	ApplicationCommandManager *manager;
};

class FileMenuItem : public MainMenuItem
{
public:

	enum class Action
	{
		OpenProject = 0,
		OpenXmlPreset,
		OpenPreset
	};

	FileMenuItem(const File &file_, Action actionToDo_, BackendProcessorEditor *bpe_) :
		file(file_),
		actionToDo(actionToDo_),
		bpe(bpe_)
	{
		setupSize();
	};

	void perform() override;

	String getName() override { return file.getFileName(); }

private:

	BackendProcessorEditor *bpe;
	File file;
	Action actionToDo;
};

class MainMenuContainer : public Component
{
public:

	~MainMenuContainer() {}

	void addCommandIds(ApplicationCommandManager *manager, const Array<int> &ids);

	void addFileIds(const Array<File> &files, BackendProcessorEditor *bpe);

	void clearOnStateForAllItems();

	void resized();

private:

	OwnedArray<MainMenuItem> items;
};

class MainMenuWithViewPort : public Component
{
public:

	MainMenuWithViewPort()
	{
		addAndMakeVisible(viewport = new Viewport());

		viewport->setViewedComponent(menuContainer = new MainMenuContainer());
		viewport->setScrollBarsShown(true, false, false, false);
		viewport->setScrollBarThickness(30);

		setSize(900 - 32, 768);
	}

	~MainMenuWithViewPort()
	{
		viewport->setViewedComponent(nullptr);
		menuContainer = nullptr;
		viewport = nullptr;
		
	}

	MainMenuContainer *getContainer() { return menuContainer.getComponent(); }

	void resized()
	{
		viewport->setBounds(getLocalBounds());
	}

private:

	ScopedPointer<Viewport> viewport;
	Component::SafePointer<MainMenuContainer> menuContainer;
};


#endif  // MAINMENUCOMPONENT_H_INCLUDED
