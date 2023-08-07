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

#ifndef VUMETER_H_INCLUDED
#define VUMETER_H_INCLUDED

namespace hise { using namespace juce;

/** A level meter that for showing peak values.
*	@ingroup hise_ui
*
*	The best practice for using one of those is a Timer that regularly
*	calls setPeak() in its timerCallback().
*
*/
class VuMeter: public Component,
			   public SettableTooltipClient
{
public:

	/** The Style of the vu meter. */
	enum Type
	{
		MonoHorizontal = 0, ///< a non segmented mono meter with linear range (0.0 - 1.0)
		MonoVertical, ///< a vertical version of the MonoHorizontal Style (not yet implemented)
		StereoHorizontal, ///< a segmented stereo meter with logarithmic range (-100dB - 0dB)
		StereoVertical, ///< a vertical version of StereoHorizontal
		MultiChannelVertical,
		MultiChannelHorizontal,
		numTypes
	};

	/** The ColourIds that can be changed with setColour. */
	enum ColourId
	{
		outlineColour = 0, ///< the outline colour
		ledColour, ///< the colour of the segmented leds or the mono-bar 
		backgroundColour, ///< the background colour
		numColours
	};

	struct LookAndFeelMethods
	{
		virtual ~LookAndFeelMethods();;

		virtual void drawMonoMeter2(Graphics& g, VuMeter& v, VuMeter::Type type, float value);

		virtual void drawStereoMeter2(Graphics& g, VuMeter& v, VuMeter::Type type, float vL, float vR);

	};

	/** Creates a new VuMeter. */
	VuMeter(float leftPeak=0.0f, float rightPeak=0.0f, Type t = MonoHorizontal);

	~VuMeter();;

	/** Change the colour of the VuMeter. */
	void setColour(ColourId id, Colour newColour);;

	void paint(Graphics &g) override;;

	/** Change the Type of the VuMeter. */
	void setType(Type newType);;

	/** sets a new peak level. 
	*
	*	For stereo meters there is a peak logic with decibel conversion and decreasing level, 
	*	and for monophonic VuMeters it simply displays the 'left' value. */
	void setPeak(float left, float right=0.0f);;

	void setPeakMultiChannel(float *numbers, int numChannels);

	void setInvertMode(bool shouldInvertRange);

	void setForceLinear(bool shouldForceLinear);

private:

	LookAndFeelMethods* getLaf();

	LookAndFeelMethods defaultLaf;

	bool forceLinear = false;

	bool invertMode = false;

	void drawMonoMeter(Graphics &g);
	void drawStereoMeter(Graphics &g);;

	float previousValue;

	Colour colours[numColours];

	float l;
	float r;
	Type type;

	float multiChannels[NUM_MAX_CHANNELS];
	int numChannels;

};

} // namespace hise

#endif  // VUMETER_H_INCLUDED
