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



#pragma once

namespace juce { using namespace hise;

#ifndef RETURN_STATIC_IDENTIFIER
#define RETURN_STATIC_IDENTIFIER(x) const static Identifier id_(x); return id_;
#endif

#define CHECK_CONDITION(condition, errorMessage) if(!(condition)) throw String(errorMessage);
#define CHECK_CONDITION_WITH_LOCATION(condition, errorMessage) if(!(condition)) location.throwError(errorMessage);



/** A buffer of floating point data for use in scripted environments.
*
*	This class can be wrapped into a var and used like a primitive value.
*	It is a one dimensional float array - if you want multiple channels, use an Array<var> with multiple variant buffers.
*
*	It contains some handy overload operators to make working with this type more convenient:
*
*		b * 2.0f			// Applies the gain factor to all samples in the buffer
*		b * otherBuffer		// Multiplies the values of the buffers and store them into 'b'
*		b + 2.0f			// adds 2.0f to all samples
*		b + otherBuffer		// adds the other buffer
*
*	Important: Because all operations are inplace, these statements are aquivalent:
*
*		(b = b * 2.0f) == (b *= 2.0f) == (b *2.0f);
*
*	For copying and filling buffers, the '<<' and '>>' operators are used.
*
*		0.5f >> b			// fills the buffer with 0.5f (shovels 0.5f into the buffer...)
*		b << 0.5f			// same as 0.5f >> b
*		a >> b				// copies the buffer a into b;
*		a << b				// copies the buffer b into a;
*
*	If the Intel IPP library is used, the data will be allocated using the IPP allocators for aligned data
*
*/
class VariantBuffer : public DynamicObject
{
public:

	// ================================================================================================================

	/** Creates a buffer using preallocated data. */
	VariantBuffer(float *externalData, int size_);

	/** Creates a buffer that operates on another buffer. */
	VariantBuffer(VariantBuffer *otherBuffer, int offset = 0, int numSamples = -1);

	/** Creates a new buffer with the given sample size. The data will be initialised to 0.0f. */
	VariantBuffer(int samples);

	~VariantBuffer();

	static Identifier getName() { RETURN_STATIC_IDENTIFIER("Buffer") };

	/** Removes Nan numbers from a array of float data. */
    static void sanitizeFloatArray(float** channels, int numChannels, int numSamples);

	// ================================================================================================================

	void referToOtherBuffer(VariantBuffer *b, int offset = 0, int numSamples = -1);
	void referToData(float *data, int numSamples);

	void addSum(const VariantBuffer &a, const VariantBuffer &b);
	void addMul(const VariantBuffer &a, const VariantBuffer &b);

	VariantBuffer& operator *(const VariantBuffer &b);
	VariantBuffer& operator *=(const VariantBuffer &b);
	VariantBuffer& operator *(float gain);
	VariantBuffer& operator *=(float gain);
	VariantBuffer& operator +(const VariantBuffer &b);
	VariantBuffer& operator +=(const VariantBuffer &b);
	VariantBuffer& operator +(float gain);
	VariantBuffer& operator +=(float gain);

	VariantBuffer& operator -(float value);
	VariantBuffer& operator -=(float value);

	VariantBuffer& operator -(const VariantBuffer &b);
	VariantBuffer& operator -=(const VariantBuffer &b);

	VariantBuffer& operator <<(const VariantBuffer &b);
	VariantBuffer& operator << (float f);
	void operator >>(VariantBuffer &destinationBuffer) const;
	
	
	const float &operator [](int sampleIndex) const;

	float &operator [](int sampleIndex);

	var getSample(int sampleIndex);
	void setSample(int sampleIndex, float newValue);

	String toDebugString() const;
	
	
	
	class Factory : public DynamicObject
	{
	public:

		Factory(int stackSize_);
		~Factory();

		static var create(const var::NativeFunctionArgs &args);
		static var referTo(const var::NativeFunctionArgs &args);

	private:

		VariantBuffer *getFreeVariantBuffer();

		const int stackSize;

		ReferenceCountedArray<VariantBuffer> sectionBufferStack;
	};

	typedef ReferenceCountedObjectPtr<VariantBuffer> Ptr;

	AudioSampleBuffer buffer;
	int size;
	VariantBuffer::Ptr referencedBuffer;

	private:

	void addMethods();

	JUCE_DECLARE_WEAK_REFERENCEABLE(VariantBuffer);
};

void operator >> (float f, VariantBuffer &b);
} // namespace juce
