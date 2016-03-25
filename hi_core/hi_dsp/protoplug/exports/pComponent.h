/*
  ==============================================================================

    exComponent.h
    Created: 25 Feb 2014 6:03:51pm
    Author:  pac

  ==============================================================================
*/

#pragma once

#include "typedefs.h"



PROTO_API     void Component_repaint(pComponent self) 
{
	self.c->repaint();
	/* this doesn't even catch segfaults :(
	bool caught = false;
	try {
		self.c->repaint();
	} catch (...) {
		caught = true;
	}
	if (caught) {
		juce::AlertWindow(
			"protoplug error", 
			"An exception occured in a protoplug script. The script is probably passing a null pointer to an ffi function ! "
			+String("\r\nThe host program might crash because of this."), 
			AlertWindow::AlertIconType::QuestionIcon);
	}*/
}


PROTO_API     void Component_repaint2(pComponent self, int x, int y, int width, int height) 
{
	self.c->repaint(x, y, width, height);
}


PROTO_API     void Component_repaint3(pComponent self, exRectangle_int area) 
{
	self.c->repaint(area.toJuceRect());
}


PROTO_API     pImage Component_createComponentSnapshot (pComponent self, 
												exRectangle_int areaToGrab,
												bool clipImageToComponentBounds = true,
												float scaleFactor = 1.0f)
{
	pImage i = { new Image() };
	*i.i = self.c->createComponentSnapshot(areaToGrab.toJuceRect(), clipImageToComponentBounds, scaleFactor);
	return i;
}