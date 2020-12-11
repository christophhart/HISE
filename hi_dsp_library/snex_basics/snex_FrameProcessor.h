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

private:

	friend class SnexObjectDatabase;
	template <int NumChannels> friend class ProcessData;
	friend class ProcessDataDyn;

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


struct dyn_indexes
{
	struct wrapped : public index_base<wrapped>
	{
		wrapped(int maxSize) :
			index_base<wrapped>(0),
			size(maxSize)
		{}

		wrapped& operator=(int v) { value = v; return *this; };

		operator int() const
		{
			return value % size;
		}

	private:

		const int size = 0;
	};

	struct zeroed : public index_base<zeroed>
	{
		zeroed(int maxSize) :
			index_base<zeroed>(0),
			size(maxSize)
		{}

		zeroed& operator=(int v) { value = v; return *this; };

		operator int() const
		{
			if (isPositiveAndBelow(value, size))
				return value;

			return 0;
		}

	private:

		const int size = 0;
	};

	struct clamped : public index_base<clamped>
	{
		clamped(int maxSize) :
			index_base<clamped>(0),
			size(jmax(0, maxSize - 1))
		{}

		clamped& operator=(int v) { value = v; return *this; };

		operator int() const
		{
			return jlimit(0, size, value);
		}

	private:

		int size = 0;
	};

	struct unsafe : public index_base<unsafe>
	{
		unsafe(int unused, int initValue) :
			index_base<unsafe>(initValue)
		{}

		operator int() const
		{
			return value;
		}
	};
};

struct IndexType
{
	

	template <int I> static auto wrapped(const FrameProcessor<I>& f)
	{
		return typename span<float, I>::wrapped(0);
	}

	template <int I> static auto clamped(const FrameProcessor<I>& f)
	{
		return typename span<float, I>::clamped(0);
	}

	template <typename E, int I> static auto clamped(const span<E, I>& obj)
	{
		return typename span<E, I>::clamped(0);
	}

	template <typename E, int I> static auto wrapped(const span<E, I>& obj)
	{
		return typename span<E, I>::wrapped(0);
	}

	template <typename T> static auto clamped(T& obj)
	{
		return dyn_indexes::clamped(obj.size());
	}

	template <typename T> static auto wrapped(T& obj)
	{
		return dyn_indexes::wrapped(obj.size());
	}



#if 0
	template <typename E> static auto clamped(const dyn<E>& obj)
	{
		return typename dyn<E>::<clamped>(obj, 0);
	}

	

	template <typename E> static auto wrapped(const dyn<E>& obj)
	{
		return typename dyn<E>::wrapped(obj, 0);
	}
#endif
};


}
}