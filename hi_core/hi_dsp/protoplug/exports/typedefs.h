/*
  ==============================================================================

    typedefs.h
    Created: 7 Mar 2014 5:57:09pm
    Author:  pac

  ==============================================================================
*/

#pragma once

#include "JuceHeader.h"


// adapted from juce_StandardHeader.h
#if JUCE_MSVC
  #define PROTO_API __declspec (dllexport)
  #pragma warning (disable: 4251)
 #ifdef __INTEL_COMPILER
  #pragma warning (disable: 1125) // (virtual override warning)
 #endif
#else
 #define PROTO_API __attribute__ ((visibility("default")))
#endif

// pointer-structs :
// this is equivalent passing the pointers directly 
// and can be handled as such by the importing program
// it's necessary for proper extern C

struct pGraphics
{ Graphics *g; };

struct pFillType
{ FillType *f; };

struct pColourGradient
{ ColourGradient *c; };

struct pImage
{ Image *i; };

struct pAudioFormatReader
{ 
	AudioFormatReader *a;
	double sampleRate;
	unsigned int bitsPerSample;
	int64 lengthInSamples;
	unsigned int numChannels;
	bool usesFloatingPointData;
};

struct pFont
{ Font *f; };

struct pPath
{ Path *p; };

struct pComponent
{
	Component *c;
	pComponent(Component *_c) : c(_c) { }
};

struct pMidiBuffer
{ MidiBuffer *m; };

struct pAudioPlayHead
{ AudioPlayHead *a; };




struct exLagrangeInterpolator
{
    float lastInputSamples[5];
    double subSamplePos;
};

struct exPoint_int
{
	int x, y;
	exPoint_int (const Point<int> &p) :x(p.x), y(p.y) { }
	const Point<int> toJucePoint() { return Point<int>(x,y); }
};

struct exPoint_float
{
	float x, y;
	exPoint_float (const Point<float> &p) :x(p.x), y(p.y) { }
	Point<float> toJucePoint() { return Point<float>(x,y); }
};

struct exLine_int
{
	int startx, starty;
	int endx, endy;
	const Line<int> toJuceLine()
	{
		return Line<int>(startx, starty, endx, endy);
	}
};

struct exLine_float
{
	float startx, starty;
	float endx, endy;
	const Line<float> toJuceLine()
	{
		return Line<float>(startx, starty, endx, endy);
	}
	exLine_float(Line<float> in)
		:startx(in.getStart().getX()), starty(in.getStart().getY()), 
		endx(in.getEnd().getX()), endy(in.getEnd().getY())
	{}
};

struct exRectangle_int
{
	int x, y, w, h;
	const Rectangle<int> toJuceRect()
	{ return Rectangle<int>(x, y, w, h); }
	exRectangle_int(const Rectangle<int> &in)
		:x(in.getX()), y(in.getY()), w(in.getWidth()), h(in.getHeight())
	{ }
};

struct exRectangle_float
{
	float x, y, w, h;
	const Rectangle<float> toJuceRect()
	{ return Rectangle<float>(x, y, w, h); }
	exRectangle_float(const Rectangle<float> &in)
		:x(in.getX()), y(in.getY()), w(in.getWidth()), h(in.getHeight())
	{ }
};

struct exColour
{
	uint32 c;
};

struct exKeyPress
{
	int keyCode;
	int mods;
	juce_wchar textCharacter;
};

exKeyPress KeyPress2Struct(const KeyPress& _k)
{
	exKeyPress k = {
		_k.getKeyCode(),
		_k.getModifiers().getRawFlags(),
		_k.getTextCharacter()
	};
	return k;
}

struct exAffineTransform
{
	float mat00, mat01, mat02;
	float mat10, mat11, mat12;
	const AffineTransform toJuceAff()
	{
		return AffineTransform(
			mat00, mat01, mat02,
			mat10, mat11, mat12);
	}
	exAffineTransform (AffineTransform in)
	: mat00(in.mat00), mat01(in.mat01), mat02(in.mat02),
	mat10(in.mat10), mat11(in.mat11), mat12(in.mat12)
	{}
};

struct exPathStrokeType
{
	float thickness;
	int jointStyle;
	int endStyle;
	const PathStrokeType toJuceType()
	{
		return PathStrokeType(thickness, 
			(juce::PathStrokeType::JointStyle)jointStyle, 
			(juce::PathStrokeType::EndCapStyle)endStyle);
	}
};

struct exTime
{
	int64 millisSinceEpoch;
	exTime (const Time &t) : millisSinceEpoch(t.toMilliseconds()) { }
};

// warning in VS because of VS bug 488660 "Improper issuance of C4610"
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4510)
#pragma warning(disable:4610)
#endif
struct exMouseEvent
{
	int x, y;
	int mods;
	pComponent eventComponent;
	pComponent originalComponent;
	exTime eventTime;
	exTime mouseDownTime;
	exPoint_int mouseDownPos;
	uint8 numberOfClicks, wasMovedSinceMouseDown;
};
#ifdef _MSC_VER
#pragma warning(pop)
#endif

exMouseEvent MouseEvent2Struct (const MouseEvent &_e)
{
	exMouseEvent e = 
	{
		_e.x,
		_e.y,
		_e.mods.getRawFlags(),
		_e.eventComponent,
		_e.originalComponent,
		_e.eventTime,
		_e.mouseDownTime,
		_e.getMouseDownPosition(),
		(uint8)_e.getNumberOfClicks(),
		_e.getDistanceFromDragStart()!=0
	};
	return e;
}

struct exMouseWheelDetails
{
	float deltaX;
	float deltaY;
	bool isReversed;
	bool isSmooth;
};

exMouseWheelDetails MouseWheelDetails2Struct (const MouseWheelDetails& _d)
{
	exMouseWheelDetails d = 
	{
		_d.deltaX,
		_d.deltaY,
		_d.isReversed,
		_d.isSmooth
	};
	return d;
}
