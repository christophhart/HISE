/*
  ==============================================================================

    pImageFileFormat.h
    Created: 23 Mar 2014 7:29:52pm
    Author:  pac

  ==============================================================================
*/

#pragma once

#include "typedefs.h"
#include "../ProtoplugDir.h"

PROTO_API pImage ImageFileFormat_loadFrom2(const char *filename)
{ 
	pImage i = { new Image() };
	File f = ProtoplugDir::Instance()->getDir().getChildFile(filename);
	if (f != File::nonexistent)
		*i.i = ImageFileFormat::loadFrom(f);
	return i;
}