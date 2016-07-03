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

#ifndef ATKCONVOLUTION_H_INCLUDED
#define ATKCONVOLUTION_H_INCLUDED



#include <list>
#include <vector>

struct IppBuffer
{
	IppBuffer(int size=0):
		data(nullptr)
	{
		if (size != 0)
		{
			data = ippsMalloc_8u(size*sizeof(float));
		}
	}

	~IppBuffer()
	{
		if (data != nullptr)
		{
			ippFree(data);
			data = nullptr;
		}
	}

	Ipp8u *data;
};

//#include <ATK/Core/TypedBaseFilter.h>
//#include <ATK/Utility/FFT.h>

/// A zero-delay convolution filter based on FFT

class ConvolutionFilter
{
public:
	/// Simplify parent calls
	
	/// Current amount of data in the buffer
	mutable uint64 split_position;
	
	int input_delay;

	/// Size of the individual FFTs that are processed
	int splitSize;

	IppFFT processor;

	/// Impulse convolved with the input signal
	std::vector<float> impulse;

	AudioSampleBuffer impulseBuffer;

	AudioSampleBuffer partialFrequencyInputBuffer;

	AudioSampleBuffer partialFrequencyImpulseBuffer;

	AudioSampleBuffer tempBuffer;

	AudioSampleBuffer resultBuffer;

	AudioSampleBuffer invFFTBuffer;

	AudioSampleBuffer bruteBuffer;

	IppBuffer bruteBuffer2;

	std::list<AudioSampleBuffer> partialFrequencyInputBufferList;

	/// This buffer contains the head of the last convolution (easier to have 2 parts)
	mutable std::vector<float> temp_out_buffer;

	/// Called partial convolutions, but actually contains the former temp_in_buffers
	mutable std::list<std::vector<std::complex<float> > > partial_frequency_input;
	/// Copied so that it's not reallocated each time
	mutable std::vector<std::complex<float> > result;

	/// The impulse is stored here in a unique vector, split in split_size FFTs, one after the other
	std::vector<std::complex<float> > partial_frequency_impulse;

	/// Compute the partial convolutions
	void calculatePartialConvolutions();
	/// Create a new chunk and compute the convolution
	void pushNewChunk(float *inputData, int numSamples);

	/// Process the first split_size elements of the convolution
	void processBruteConvolution(float *input, int numSamples);

public:
	/// Build a new convolution filter
	ConvolutionFilter();

	/**
	* @brief Set the impulse for the convolution
	* @param impulse is the impulse for the convolution
	*/
	void setImpulse(float *impulseData, int impulseSize);

	/*!
	* @brief Set the split size
	* @param split_size is the size of the individual FFTs
	*/
	void setSplitSize(int split_size);

	void processBlock(float *data, int numSamples);

	void setup();

};



#endif  // ATKCONVOLUTION_H_INCLUDED
