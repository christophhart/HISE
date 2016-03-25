/*
  ==============================================================================

    exFillType.h
    Created: 25 Feb 2014 6:08:09pm
    Author:  pac

  ==============================================================================
*/

#pragma once

#include "typedefs.h"

PROTO_API pFillType FillType_new(exColour c)
{
	pFillType f = { new FillType(Colour(c.c)) };
	return f;
}

PROTO_API pFillType FillType_new2(pColourGradient c)
{
	pFillType f = { new FillType(*c.c) };
	return f;
}

PROTO_API void FillType_delete(pFillType f)
{
	delete f.f;
}

PROTO_API void FillType_setOpacity (pFillType f, float newOpacity)
{
	f.f->setOpacity(newOpacity);
}