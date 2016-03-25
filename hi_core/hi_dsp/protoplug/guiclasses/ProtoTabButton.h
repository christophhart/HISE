
#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

class ProtoTabButton : public Button
{
public:
	ProtoTabButton(String _name) :
	  Button(_name)
	{
		name = _name;
	}
	void paintButton (Graphics &g, bool isMouseOverButton, bool isButtonDown) override
	{
		if (isButtonDown)
			g.fillAll (Colour(0xffbebeff)); // deeper blue ish
		else if (isMouseOverButton)
			g.fillAll (Colour(0xffe2e2ff)); // light blue ish
		else
			g.fillAll (Colour(0xffd4d4f3)); // medium blue ish
		g.setColour(Colours::black);
		g.drawText(name, getBounds().withPosition(0,0), Justification::centred, false);
	}
    class Listener
    {
    public:
        virtual ~Listener()  {}
        virtual void tabButtonClicked (ProtoTabButton*) = 0;
        virtual void tabButtonDoubleClicked (ProtoTabButton*) = 0;
    };
	void clicked ()
	{ if (listener) listener->tabButtonClicked(this); }
	void mouseDoubleClick (const MouseEvent &)
	{ if (listener) listener->tabButtonDoubleClicked(this); }
    void setListener (Listener* newListener)
	{ listener = newListener; }

private:
	String name;
	Listener *listener;
};

