/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   HISE is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licenses for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licensing:
*
*   http://www.hise.audio/
*
*   HISE is based on the JUCE library,
*   which must be separately licensed for closed source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

#ifndef HI_BINARY_DATA_H_INCLUDED
#define HI_BINARY_DATA_H_INCLUDED

#include "FrontendBinaryData.h"
#include "ProcessorEditorHeaderBinaryData.h"
#include "LookAndFeelBinaryData.h"
#include "BinaryDataDictionaries.h"


#if !USE_FRONTEND
#include "BackendBinaryData.h"
#endif


namespace EditorIcons
{
static const unsigned char compileIcon[] = { 110,109,0,0,22,67,15,2,192,67,108,0,0,27,67,54,88,195,67,98,0,0,27,67,54,88,195,67,110,3,35,67,195,174,188,67,0,0,37,67,232,171,188,67,98,0,0,37,67,232,171,188,67,110,99,39,67,232,171,188,67,0,0,42,67,232,171,188,67,98,183,253,39,67,232,171,188,67,0,
0,27,67,93,174,198,67,0,0,27,67,93,174,198,67,108,0,0,17,67,15,2,192,67,99,101,0,0 };

static const unsigned char cancelIcon[] = { 110,109,116,110,45,66,184,32,152,67,108,215,146,59,66,43,92,150,67,108,0,0,102,66,208,169,155,67,108,148,54,136,66,43,92,150,67,108,198,72,143,66,184,32,152,67,108,99,36,116,66,93,110,157,67,108,198,72,143,66,2,188,162,67,108,148,54,136,66,143,128,164,
67,108,0,0,102,66,233,50,159,67,108,215,146,59,66,142,128,164,67,108,116,110,45,66,2,188,162,67,108,157,219,87,66,93,110,157,67,99,101,0,0 };

static const unsigned char undoIcon[] = { 110,109,0,93,96,67,64,87,181,67,98,169,116,87,67,119,74,181,67,238,53,75,67,247,66,184,67,128,173,59,67,64,229,191,67,108,0,0,47,67,64,46,186,67,108,0,0,47,67,64,174,203,67,108,0,0,82,67,64,174,203,67,108,0,86,71,67,128,123,197,67,98,221,255,111,67,79,
174,178,67,128,164,101,67,210,215,207,67,128,228,102,67,64,179,210,67,98,201,215,119,67,101,133,198,67,205,117,117,67,136,117,181,67,0,93,96,67,64,87,181,67,99,101,0,0 };

static const unsigned char redoIcon[] = { 110,109,90,186,64,67,64,87,181,67,98,176,162,73,67,118,74,181,67,108,225,85,67,247,66,184,67,218,105,101,67,64,229,191,67,108,90,23,114,67,64,46,186,67,108,90,23,114,67,64,174,203,67,108,90,23,79,67,64,174,203,67,108,90,193,89,67,128,123,197,67,98,125,
23,49,67,79,174,178,67,218,114,59,67,211,215,207,67,218,50,58,67,64,179,210,67,98,145,63,41,67,101,133,198,67,141,161,43,67,136,117,181,67,90,186,64,67,64,87,181,67,99,101,0,0 };
};

#endif  // HI_BINARY_DATA_H_INCLUDED
