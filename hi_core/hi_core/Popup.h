/*
  ==============================================================================

    SingletonPopup.h
    Created: 17 Jun 2014 1:22:21pm
    Author:  Chrisboy

  ==============================================================================
*/

#ifndef SINGLETONPOPUP_H_INCLUDED
#define SINGLETONPOPUP_H_INCLUDED


class BaseDebugArea;

class AutoPopupDebugComponent
{
public:

	void showComponentInDebugArea(bool shouldBeVisible);

	virtual ~AutoPopupDebugComponent() {};

protected:

	AutoPopupDebugComponent(BaseDebugArea *area) :
		parentArea(area)
	{};

private:

	BaseDebugArea *parentArea;
};


/** A small utility class that allows popups for console and plotter. 
*
*	@ingroup utility
*/
class PopupWindow: public DocumentWindow
{
public:

	/** Creates a basic Popup window with all Buttons .
	*
	*	@param t the Popup title.
	*/
	PopupWindow(const String &t): DocumentWindow(t, Colours::lightgrey, DocumentWindow::allButtons, true)
	{
		
	};

	virtual ~PopupWindow()
	{ };

	void setDeleteOnClose(bool shouldBeDeleted)
	{
		deleteWhenClosed = shouldBeDeleted;
	};

	/** Removes the instance from the desktop. You have to take care about ownership elsewhere! */
	virtual void closeButtonPressed()
	{ 
		deleteWhenClosed ? delete this : removeFromDesktop(); 
	};

private: 

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PopupWindow)

	

	bool deleteWhenClosed;
};

class AboutPage;

class TooltipBar: public Component,
				  public Timer
{
public:

	enum ColourIds
	{
		backgroundColour = 0x100,
		textColour = 0x010,
		iconColour = 0x001
	};

	TooltipBar();

	void paint(Graphics &g);

	void timerCallback();
	

	void setText(const String &t)
	{
		if(t != currentText)
		{
			isClear = false;
			counterSinceLastTextChange = 0;
			currentText = t;
			repaint();
		}
		
	}

	void clearText()
	{
		if(!isClear)
		{
			isClear = true;
			currentText = String::empty;
			counterSinceLastTextChange = 0;
			repaint();
		}
	}

	void mouseDown(const MouseEvent &e);

private:

	int counterSinceLastTextChange;

	String currentText;
	bool isClear;

	Point<float> lastMousePosition;
	bool newPosition;

	
};


#endif  // SINGLETONPOPUP_H_INCLUDED
