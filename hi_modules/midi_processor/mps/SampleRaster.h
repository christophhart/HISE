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
*   Commercial licences for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licencing:
*
*   http://www.hartinstruments.net/hise/
*
*   HISE is based on the JUCE library,
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
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
	void processHiseEvent(HiseEvent& m) override
	{
		const int t = (int)m.getTimeStamp();
		m.setTimeStamp((uint16)roundToRaster(t));
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
