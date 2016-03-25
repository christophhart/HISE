/*
  ==============================================================================

    pImage.h
    Created: 15 Mar 2014 5:17:51pm
    Author:  pac

  ==============================================================================
*/

#pragma once

#include "typedefs.h"

PROTO_API pImage Image_new()
{
	pImage i = { new Image() };
	return i;
}

PROTO_API pImage Image_new2(int pixelFormat, int imageWidth, int imageHeight, bool clearImage)
{
	pImage i = { new Image((Image::PixelFormat)pixelFormat, imageWidth, imageHeight, clearImage) };
	return i;
}

PROTO_API void Image_delete(pImage i)
{ delete i.i; }

PROTO_API bool Image_isValid(pImage i)
{ 
	if (i.i)
		return i.i->isValid();
	else
		return false;
}