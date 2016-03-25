/*
  ==============================================================================

    exColourGradient.h
    Created: 25 Feb 2014 6:06:46pm
    Author:  pac

  ==============================================================================
*/

#pragma once

#include "typedefs.h"

PROTO_API pColourGradient ColourGradient_new (exColour c1, float x1, float y1, exColour c2, float x2, float y2, bool isRadial)
{
	pColourGradient c = {new ColourGradient(Colour(c1.c), x1, y1, Colour(c2.c), x2, y2, isRadial)};
	return c;
}

PROTO_API void ColourGradient_delete (pColourGradient c)
{
	delete c.c;
}

PROTO_API int ColourGradient_addColour (pColourGradient self, double proportionAlongGradient,
                   exColour colour)
{
	return self.c->addColour (proportionAlongGradient, Colour(colour.c));
}

PROTO_API void ColourGradient_removeColour (pColourGradient self, int index)
{
	self.c->removeColour (index);
}

PROTO_API void ColourGradient_multiplyOpacity (pColourGradient self, float multiplier)

{
	self.c->multiplyOpacity (multiplier);
}

PROTO_API int ColourGradient_getNumColours(pColourGradient self)
{
	return self.c->getNumColours();
}

PROTO_API exColour ColourGradient_getColour (pColourGradient self, int index)
{
	exColour c = {self.c->getColour (index).getARGB()};
	return c;
}

PROTO_API void ColourGradient_setColour (pColourGradient self, int index, exColour newColour)
{
	self.c->setColour (index, Colour(newColour.c));
}

PROTO_API exColour ColourGradient_getColourAtPosition (pColourGradient self, double position)
{
	exColour c = {self.c->getColourAtPosition (position).getARGB()};
	return c;
}