
#pragma once

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4190) // C function returns C-incompatible UDT.
#endif

#include "exports/typedefs.h"

#include "exports/pAudioFormatReader.h"
#include "exports/pColourGradient.h"
#include "exports/pComponent.h"
#include "exports/pFillType.h"
#include "exports/pFont.h"
#include "exports/pGraphics.h"
#include "exports/pImage.h"
#include "exports/pImageFileFormat.h"
#include "exports/pLagrangeInterpolator.h"
#include "exports/pMidiBuffer.h"
#include "exports/pPath.h"


PROTO_API     bool AudioPlayHead_getCurrentPosition(pAudioPlayHead self, AudioPlayHead::CurrentPositionInfo& result) 
{ return self.a->getCurrentPosition(result); }

#ifdef _MSC_VER
#pragma warning(pop)
#endif

/*
the protojuce api looks somewhat consistent on the outside, but on the inside 
it's not so pretty. I didn't use any of the c++ to Lua autowrapping libraries, 
instead i rolled my own wrappers and used LuaJIT's FFI. This may or may not 
have been the easier route, either way the end product is a little faster.

In a nutshell :
1. The required C++ juce functionality is wrapped into a C API and exported by 
   protoplug. This is done by the headers in the "Exports" folder.

2. This API is then imported into Lua using LuaJIT's FFI feature.

3. The "protojuce" lua module then takes this (somewhat messy) C API and 
   re-wraps it into a nicer lua API.

JUCE's c++ classes are handled in different ways :

classes that need to be managed by JUCE :
	export constuctor/destructor (always ffi.gc)
		ColourGradient
		FillType
		Path
		RectangleList
		Image
		Font
	do not export constructor/destructor
		Component
		Graphics
classes that convert to structs and need not export anything:
	Rectangle<>
	MouseEvent
	Time
	Point<>
	MouseWheelDetails
	Colour
	AffineTransform
	PathStrokeType
	Line<>
classes that convert to something else :
	String (to const char*)
	ModifierKeys (to int)
	Justification (to int)
	RectanglePlacement (to int)
classes that are opaque pointers and only referred to :
	MouseInputSource
*/


