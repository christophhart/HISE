/*
  ==============================================================================

    VuMeter.h
    Created: 7 Aug 2014 5:10:33pm
    Author:  Christoph

  ==============================================================================
*/

#ifndef VUMETER_H_INCLUDED
#define VUMETER_H_INCLUDED


/** A Slider-style component that displays peak values.
*	@ingroup components
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

	/** Creates a new VuMeter. */
	VuMeter(float leftPeak=0.0f, float rightPeak=0.0f, Type t = MonoHorizontal);

	~VuMeter() {};

	/** Change the colour of the VuMeter. */
	void setColour(ColourId id, Colour newColour) {	colours[id] = newColour; };

	void paint(Graphics &g) override;;

	

	/** Change the Type of the VuMeter. */
	void setType(Type newType);;

	/** sets a new peak level. 
	*
	*	For stereo meters there is a peak logic with decibel conversion and decreasing level, 
	*	and for monophonic VuMeters it simply displays the 'left' value. */
	void setPeak(float left, float right=0.0f);;

	void setPeakMultiChannel(float *numbers, int numChannels);

private:

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



class WaveformComponent: public Component
{
public:

	enum WaveformType
	{
		Sine = 1,
		Triangle,
		Saw,
		Square,
		Noise,
		Custom,
		numWaveformTypes
	};

	WaveformComponent():
		type(Sine)
	{};

	void paint(Graphics &g);

	void setType(int t)
	{
		type = (WaveformType)t;
		repaint();
	}

private:

	WaveformType type;

};

#endif  // VUMETER_H_INCLUDED
