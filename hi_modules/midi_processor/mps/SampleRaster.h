/*
  ==============================================================================

    SampleRaster.h
    Created: 5 Jul 2014 6:01:13pm
    Author:  Chrisboy

  ==============================================================================
*/

#ifndef SAMPLERASTER_H_INCLUDED
#define SAMPLERASTER_H_INCLUDED


/** The SampleRaster is a small utility processor that quantisizes timestamps of midi data to a specific step size.
*
*	If a midi message has the timestamp '15', and the step size is 4, the new timestamp will be '12'.
*
*	This allows assumptions about buffersizes for later calculations (which can be useful for eg. SIMD acceleration).
*/
class SampleRaster: public MidiProcessor
{
public:

	SET_PROCESSOR_NAME("SampleRaster", "Sample Raster");

	/** Creates a SampleRaster. The initial step size is 4. */
	SampleRaster(MainController *mc, const String &id):
		MidiProcessor(mc, id),
		stepSize(8)	{};

	/** This sets the quantisation size. It is supposed to be a power of two and smaller than the buffer size. */
	void setRaster(int newStepSize) noexcept
	{
		jassert(isPowerOfTwo(newStepSize));
		jassert(stepSize < getBlockSize());
		stepSize = newStepSize;
	};

	

	float getAttribute(int) const override { return 1.0f; };

	void setInternalAttribute(int, float) override { 	};

protected:

	/** Applies the raster to the timestamp. */
	void processMidiMessage(MidiMessage &m) override
	{
		const int t = (int)m.getTimeStamp();
		m.setTimeStamp(roundToRaster(t));
	};

private:

	/** This quantisizes the value to the raster. */
	int roundToRaster(int valueToRound) const noexcept
	{
		const int delta = (valueToRound % stepSize);

		const int rastered = valueToRound - delta;

		return rastered;
	}

	int stepSize;

};




#endif  // SAMPLERASTER_H_INCLUDED
