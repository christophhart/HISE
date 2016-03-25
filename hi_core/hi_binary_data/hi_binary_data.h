/*
  ==============================================================================

    hi_binary_data.h
    Created: 12 Jun 2015 8:50:19pm
    Author:  Christoph

  ==============================================================================
*/

#ifndef HI_BINARY_DATA_H_INCLUDED
#define HI_BINARY_DATA_H_INCLUDED

namespace HiBinaryData
{

#if USE_BACKEND

#else
#include "FrontendBinaryData.h"
#endif
#include "ProcessorEditorHeaderBinaryData.h"
#include "LookAndFeelBinaryData.h"

}

#endif  // HI_BINARY_DATA_H_INCLUDED
