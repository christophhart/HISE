/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option any later version.
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

#pragma once

namespace snex {
    

    
    
    
    
    
    
    
namespace Types {
using namespace juce;




/** A simple helper class that provides interleaved iteration over a multichannel signal.
    @ingroup snex_helpers

	Some processing algorithms require interleaved processing so that you have all samples 
	for all channels available. 

	The most simple example would be a MS-Decoder

	
	@code
	// Mid-Side processing

	float m = (l + r) * 0.5f;
	float s = (l - r) * 0.5f;

	l = m + stereo_amount * s;
	r = m - stereo_amount * s;
	@endcode

	In scriptnode / SNEX, the signal data is not interleaved by default, which means that the data
	for each channel is stored consecutively in memory. This has the advantage that most operations
	that do not require frame processing can use SIMD operations. 

	However if your algorithm requires frame based processing, you can use this class which tucks
	away most of the interleaving boilerplate code.

	
	@code
	// This is the callback function required by SNEX / scriptnode
	void process(ProcessData<2>& data)
	{
		// Create a FrameProcessor object from the data
		auto frame = data.toFrameData();

		// Iterate through the frames and call the other function below
		while(frame.next())
			processFrame(frame);
	}

	// This callback will be called whenever the processing environment
	// is using frame-based processing
	void processFrame(span<float, 2>& frame)
	{
		auto& l = frame[0];
		auto& r = frame[1];

		float m = (l + r) * 0.5f;
		float s = (l - r) * 0.5f;

		l = m + stereo_amount * s;
		r = m - stereo_amount * s;
	}
	@endcode

	Since the function will be most likely inlined by the compiler, there is no performance overhead,
	and you don't need to implement the algorithm twice for compatibility with frame-based calls.

	> Note the implicit cast to the span type in the function call.
*/
template <int NumChannels> struct FrameProcessor
{
	/** The underlying span type. This class is designed so that it forwards almost every
		operation to this type so you can use it like a native span:
		
		- access using the subscript operator
		- implicitely castable for passing it into functions
		- iteratable
	*/
	using FrameType = span<float, NumChannels>;

	/** @internal forwards the const []-operator to the span type. This is templated so you can use
		the index types of the span class for wrapped / clamped / unsafe access. */
	template <typename IndexType> float& operator[](IndexType index)
	{
		return frameData[index];
	}

	/** forwards the []-operator to the span type. This is templated so you can use
	    the index types of the span class for wrapped / clamped / unsafe access. 
		
		@code{.cpp}

		auto f = processData.toFrameData();

		span<float, 2>::wrapped index;
		index = 1259; // => will be wrapped to '1'
		f[index] = 12.0f;

		@endcode
	*/
	template <typename IndexType> const float& operator[](IndexType index) const
	{
		return frameData[index];
	}

	/** Loads the next frame and returns false if the end of the signal is reached. 
	
		You can use this as condition of a while loop in order to iterate over the
		entire signal:
		
		@code{.cpp}

		while(f.next())
		{
			// do something with this frame...
			f[0] = 2.0f;
		}

		@endcode
	*/
	int next();

	static constexpr int size() { return NumChannels; }

	/** @internal Implicitely casts this object to the span type. */
	operator FrameType&();
	operator const FrameType&() const;

	/** @internal Forwards the range-based iterator of the span class. */
	float* begin() const;
	/** @internal Forwards the range-based iterator of the span class. */
	float* end() const;

	FrameType& toSpan() { return frameData; }

	static constexpr bool hasCompileTimeSize() { return true; }

private:

	friend struct SnexObjectDatabase;
	template <int C> friend struct ProcessData;
	friend struct ProcessDataDyn;

	/** @internal: only used by ProcessData<NumChannels>::toFrameData(). */
	FrameProcessor(float** processDataPointers, int numSamples);

	/** @internal */
	void loadFrame();
	/** @internal */
	void writeLastFrame();
	/** @internal (also used as JIT function pointer by SNEX) */
	static int nextFrame(void* obj);

	span<float*, NumChannels>& channels; // 8 byte
	int frameLimit = 0;					 // 4 byte
	int frameIndex = 0;				     // 4 byte
	FrameType frameData;				 // sizeof(FrameData)

};

/** This data structure is useful if you're writing any kind of oscillator.
    @ingroup snex_data_structures

	It contains the buffer that the signal is supposed to be added to as well
	as the pitch information and current state. @ref snex_data_structures

	It has an overloaded ++-operator that will bump up the uptime.

    If you create an oscillator using the `snex_osc` node, this will take in the
    two parameters Frequency and FreqRatio into account and calculates a
    per sample counter that can be used to generate any kind of signal. If you
    eg. want to generate a (naive) saw waveform, all you have to do is
 
    @code
    void process(OscProcessData& d)
    {
        for (auto& s : d.data)
        {
            s = Math.fmod(d++, 1.0);
        }
    }
    @endcode
*/
struct OscProcessData
{
    /** A monophonic audio buffer that needs to be filled with your signal. */
	dyn<float> data;		// 16 bytes
    
    /** The uptime since the voice start. */
	double uptime = 0.0;    // 8 bytes
    
    /** The delta that is added to the uptime for each sample. This is automatically calculated using the samplerate, frequency and ratio! */
	double delta = 0.0;     // 8 bytes
	
    /** This bumps the uptime with the delta value and returns the previous uptime value (post incrementor). */
	double operator++()
	{
		auto v = uptime;
		uptime += delta;
		return v;
	}
};


}
}
