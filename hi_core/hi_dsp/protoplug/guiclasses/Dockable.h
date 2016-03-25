/*
  ==============================================================================

    Dockable.h
    Created: 13 Apr 2014 3:42:04pm
    Author:  pac

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "../PluginProcessor.h"

class Dockable;

class DockablePopout  :	public DocumentWindow
{
public:
	DockablePopout (	Dockable *_dad,
					const String& name,
                    Colour backgroundColour,
                    int requiredButtons,
                    bool addToDesktop = true)
		:DocumentWindow(name, backgroundColour, requiredButtons, addToDesktop)
	{ dad = _dad; }

	void closeButtonPressed();
private:
	Dockable *dad;
};


class Dockable : public Component
{
public:
	Dockable(Component *_content, String _name, LuaProtoplugJuceAudioProcessor* _processor)
	{
		content = _content;
		name = _name;
		processor = _processor;
		addAndMakeVisible(content);
	}

	void paint (Graphics& g)
	{
		g.fillAll (Colours::white);
		if (docwin==0) return;
		g.fillAll();
		g.setColour(Colours::grey);
		g.drawText(name + " window popped out !", g.getClipBounds(), Justification::centred, false);
	}

	void resized()
	{
		if (docwin==0)
			content->setBounds(0, 0, getWidth(), getHeight());
	}

	void handleCommandMessage(int com)
	{
		if (com==1 && docwin==0)
			popOut();
		else if (com==1 && docwin!=0)
			popIn();
	}

	void popOut()
	{
		docwin = new DockablePopout(this, name, Colours::white, DocumentWindow::allButtons, true);
		docwin->setAlwaysOnTop(processor->alwaysontop);
		docwin->setResizable(true, false);
		docwin->setUsingNativeTitleBar(true);
		docwin->setContentNonOwned(content, true);
		//processor->popout = true;
		//docwin->setContentComponentSize(processor->lastUIWidth, processor->lastUIHeight);
		docwin->setTopLeftPosition(processor->lastPopoutX, processor->lastPopoutY);
		//content.setPoppedOut(true);
		docwin->setVisible(true);
		//setSize (280, 130);
		//yank.setVisible(true);
		//popin.setVisible(true);
		//content.takeFocus();
		resized();
	}

	void popIn()
	{
		//processor->popout = false;
		//int w=processor->lastUIWidth, h=processor->lastUIHeight;
		addAndMakeVisible(content);
		//content.setPoppedOut(false);
		//setSize (w,h);
		content->setSize (getWidth(), getHeight());
		docwin = 0;
		//yank.setVisible(false);
		//content.takeFocus();
		//popin.setVisible(false);
		resized();
	}
	void setAlwaysOnTop(bool aot)
	{
		if (docwin==0) return;
		docwin->setAlwaysOnTop(aot);
	}
	bool isPoppedOut()
	{
		return (docwin!=0);
	}
	void bringWindowToFront()
	{
		if (docwin==0) return;
		docwin->toFront(true);
	}
private:
	Component *content;
	ScopedPointer<DockablePopout> docwin;
	String name;
	LuaProtoplugJuceAudioProcessor *processor;
};
