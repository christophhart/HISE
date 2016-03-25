/*
  ==============================================================================

    exFont.h
    Created: 25 Feb 2014 7:37:37pm
    Author:  pac

  ==============================================================================
*/

#pragma once

#include "typedefs.h"

PROTO_API pFont Font_new(const char *typefaceName, float fontHeight, int styleFlags, bool hinted)
{
	String stypefaceName = typefaceName;
	if (hinted)
		stypefaceName += "_hinted_";
	pFont f = { new Font(stypefaceName, fontHeight, styleFlags) };
	return f;
}

PROTO_API void Font_delete(pFont f)
{
	delete f.f;
}

