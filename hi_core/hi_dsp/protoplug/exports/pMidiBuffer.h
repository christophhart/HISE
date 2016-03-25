/*
  ==============================================================================

    pMidiBuffer.h
    Created: 24 Mar 2014 5:41:49pm
    Author:  pac

  ==============================================================================
*/

#pragma once

#include "typedefs.h"


PROTO_API uint8 *MidiBuffer_getDataPointer(pMidiBuffer mb)
{
	return mb.m->data.getRawDataPointer();
}

PROTO_API int MidiBuffer_getDataSize(pMidiBuffer mb)
{
	return mb.m->data.size();
}

PROTO_API void MidiBuffer_resizeData(pMidiBuffer mb, int size)
{
	return mb.m->data.resize(size);
}
