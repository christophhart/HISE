/*
  ==============================================================================

    ProcessorPopupList.h
    Created: 9 Jul 2015 8:25:31pm
    Author:  Chrisboy

  ==============================================================================
*/

#ifndef PROCESSORPOPUPLIST_H_INCLUDED
#define PROCESSORPOPUPLIST_H_INCLUDED

class ProcessorEditorPanel;
class PopupWindow;
class BackendProcessor;
class BackendProcessorEditor;

/** A small representation of a Processor with some buttons to change the view configuration
*	@ingroup views
*
*	You can open a list of every Processor of the current patch and bypass it, set it as root processor or add it to the panel.
*	There is also the option for a popup of the ProcessorEditor for quick changes
*/
class ProcessorPopupItem : public PopupMenu::CustomComponent,
	public ButtonListener,
	public Timer
{
public:

	ProcessorPopupItem(ModulatorSynth *synth, BackendProcessorEditor *mainEditor);;

	void timerCallback() { };

	void mouseDown(const MouseEvent &m) override;
    
	void buttonClicked(Button *b) override;

	void initButtons();

    void mouseEnter(const MouseEvent &) override
    {
        repaint();
    }
    
	ProcessorPopupItem(Processor *parentProcessor, Processor *p_, BackendProcessorEditor *mainEditor);;

	void setUsedInList()
	{
		usedInList = true;
	}

	void getIdealSize(int &idealWidth, int &idealHeight) override
	{
		idealWidth = 300;
		idealHeight = 30;
	};

	void resized() override;

	void paint(Graphics &g) override;

private:

	const bool isHeadline;
	bool isSolo;

	BackendProcessorEditor *editor;

	ScopedPointer<ShapeButton> setAsRootButton;
	ScopedPointer<ShapeButton> bypassButton;
	ScopedPointer<ShapeButton> popupButton;
	ScopedPointer<ShapeButton> soloButton;
	ScopedPointer<ShapeButton> visibleButton;

	Processor *p;
	Processor *parent;

	bool usedInList;

};


class StupidRectangle : public Component
{
public:

	StupidRectangle();

	void paint(Graphics &g);

	// Sets the text that will be displayed at the top. 
	void setText(const String &p) noexcept {	path = p; }

	void resized();

private:

	ScopedPointer<ShapeButton> closeButton;

	String path;
};



class ProcessorList : public Component,
					  public SafeChangeListener
{
public:

	ProcessorList(BackendProcessorEditor *editor);

	void changeListenerCallback(SafeChangeBroadcaster *)
	{
		refreshList();
	}

	void refreshList();

	void paint(Graphics &g) override
	{
		g.fillAll(Colour(0xff333333));

	}

	void resized()
	{
		int y = 0;

		for (int i = 0; i < items.size(); i++)
		{
			items[i]->setBounds(0, y, getWidth(), 30);

			y += 30;
		}
	}

	int getHeightOfAllItems()
	{
		return items.size() * 30;
	};

private:

	BackendProcessorEditor *editor;
	WeakReference<Processor> mainSynthChain;
	

	OwnedArray<ProcessorPopupItem> items;
};



#endif  // PROCESSORPOPUPLIST_H_INCLUDED
