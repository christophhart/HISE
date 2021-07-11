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


namespace juce { using namespace hise;

VariantBuffer::VariantBuffer(float *externalData, int size_) :
size((externalData != nullptr) ? size_ : 0)
{
	if (externalData != nullptr)
	{
		float *data[1] = { externalData };
		buffer.setDataToReferTo(data, 1, size_);
	}
}

VariantBuffer::VariantBuffer(VariantBuffer *otherBuffer, int offset /*= 0*/, int numSamples /*= -1*/)
{
	referToOtherBuffer(otherBuffer, offset, numSamples);
}

VariantBuffer::VariantBuffer(int samples) :
size(samples),
buffer()
{
	if (samples > 0)
	{
		buffer.setSize(1, jmax<int>(0, samples));
		buffer.clear();
	}
}

VariantBuffer::~VariantBuffer()
{
	referencedBuffer = nullptr;
}

void VariantBuffer::referToOtherBuffer(VariantBuffer *b, int offset /*= 0*/, int numSamples /*= -1*/)
{
	referencedBuffer = b;

	size = (numSamples == -1) ? b->size : numSamples;

	referToData(b->buffer.getWritePointer(0, offset), size);
}

void VariantBuffer::referToData(float *data, int numSamples)
{
	if (data != nullptr)
	{
		buffer.setDataToReferTo(&data, 1, numSamples);
	}
	else
	{
		buffer.setSize(0, 0);
	}
	
	size = numSamples;
}

void VariantBuffer::addSum(const VariantBuffer &a, const VariantBuffer &b)
{
	CHECK_CONDITION((size >= a.size && size >= b.size && a.size == b.size), "Wrong buffer sizes for addSum");

	FloatVectorOperations::add(buffer.getWritePointer(0), a.buffer.getReadPointer(0), b.buffer.getReadPointer(0), size);
}

void VariantBuffer::addMul(const VariantBuffer &a, const VariantBuffer &b)
{
	CHECK_CONDITION((size >= a.size && size >= b.size && a.size == b.size), "Wrong buffer sizes for addSum");

	FloatVectorOperations::addWithMultiply(buffer.getWritePointer(0), a.buffer.getReadPointer(0), b.buffer.getReadPointer(0), size);
}

var VariantBuffer::getSample(int sampleIndex)
{
	CHECK_CONDITION(isPositiveAndBelow(sampleIndex, buffer.getNumSamples()), getName() + ": Invalid sample index" + String(sampleIndex));
	return (*this)[sampleIndex];
}

void VariantBuffer::setSample(int sampleIndex, float newValue)
{
	CHECK_CONDITION(isPositiveAndBelow(sampleIndex, buffer.getNumSamples()), getName() + ": Invalid sample index" + String(sampleIndex));

	buffer.setSample(0, sampleIndex, FloatSanitizers::sanitizeFloatNumber(newValue));
}

String VariantBuffer::toDebugString() const
{
	String description;

	description << "Buffer (size: " << size << ")";

	description << ", Max: " << String(buffer.getMagnitude(0, size), 3);
	description << ", RMS: " << String(buffer.getRMSLevel(0, 0, size), 3);

	return description;
}

void VariantBuffer::operator>>(VariantBuffer &destinationBuffer) const
{
	CHECK_CONDITION((destinationBuffer.size >= size), "destination buffer too small: " + String(size));

	FloatVectorOperations::copy(destinationBuffer.buffer.getWritePointer(0), buffer.getReadPointer(0), size);

	FloatSanitizers::sanitizeArray(destinationBuffer.buffer.getWritePointer(0), destinationBuffer.size);
}

VariantBuffer& VariantBuffer::operator<<(float f)
{
	FloatVectorOperations::fill(buffer.getWritePointer(0), FloatSanitizers::sanitizeFloatNumber(f), size);

	return *this;
}




VariantBuffer& VariantBuffer::operator<<(const VariantBuffer &b)
{
	CHECK_CONDITION((size >= b.size), "second buffer too small: " + String(size));

	FloatVectorOperations::copy(buffer.getWritePointer(0), b.buffer.getReadPointer(0), size);

	FloatSanitizers::sanitizeArray(buffer.getWritePointer(0), size);

	return *this;
}


VariantBuffer& VariantBuffer::operator+=(float gain)
{
	FloatVectorOperations::add(buffer.getWritePointer(0), FloatSanitizers::sanitizeFloatNumber(gain), buffer.getNumSamples());

	return *this;
}

VariantBuffer VariantBuffer::operator+(float gain)
{
	FloatVectorOperations::add(buffer.getWritePointer(0), FloatSanitizers::sanitizeFloatNumber(gain), buffer.getNumSamples());

	return *this;
}

VariantBuffer& VariantBuffer::operator+=(const VariantBuffer &b)
{
	CHECK_CONDITION((b.buffer.getNumSamples() >= buffer.getNumSamples()), "second buffer too small: " + String(buffer.getNumSamples()));

	FloatVectorOperations::add(buffer.getWritePointer(0), b.buffer.getReadPointer(0), buffer.getNumSamples());

	return *this;
}

VariantBuffer VariantBuffer::operator+(const VariantBuffer &b)
{
	CHECK_CONDITION((b.size >= size), "second buffer too small: " + String(size));

	FloatVectorOperations::add(buffer.getWritePointer(0), b.buffer.getReadPointer(0), size);

	return *this;
}

VariantBuffer & VariantBuffer::operator-(float value)
{
	FloatSanitizers::sanitizeFloatNumber(value);
	FloatVectorOperations::add(buffer.getWritePointer(0), -1.0f * value, buffer.getNumSamples());

	return *this;
}


VariantBuffer & VariantBuffer::operator-=(float value)
{
	FloatSanitizers::sanitizeFloatNumber(value);
	FloatVectorOperations::add(buffer.getWritePointer(0), -1.0f * value, buffer.getNumSamples());
	
	return *this;
}

VariantBuffer & VariantBuffer::operator-(const VariantBuffer &b)
{
	CHECK_CONDITION((b.size >= size), "second buffer too small: " + String(size));

	FloatVectorOperations::subtract(buffer.getWritePointer(0), b.buffer.getReadPointer(0), size);

	return *this;
}

VariantBuffer & VariantBuffer::operator-=(const VariantBuffer &b)
{
	CHECK_CONDITION((b.size >= size), "second buffer too small: " + String(size));

	FloatVectorOperations::subtract(buffer.getWritePointer(0), b.buffer.getReadPointer(0), size);

	return *this;
}


VariantBuffer& VariantBuffer::operator*=(float gain)
{
	buffer.applyGain(FloatSanitizers::sanitizeFloatNumber(gain));

	return *this;
}

VariantBuffer VariantBuffer::operator*(float gain)
{
	buffer.applyGain(FloatSanitizers::sanitizeFloatNumber(gain));

	return *this;
}

VariantBuffer& VariantBuffer::operator*=(const VariantBuffer &b)
{
	CHECK_CONDITION((b.size >= size), "second buffer too small: " + String(b.size));

	FloatVectorOperations::multiply(buffer.getWritePointer(0), b.buffer.getReadPointer(0), size);

	return *this;
}

VariantBuffer VariantBuffer::operator*(const VariantBuffer &b)
{
	CHECK_CONDITION((b.size >= size), "second buffer too small: " + String(b.size));

	FloatVectorOperations::multiply(buffer.getWritePointer(0), b.buffer.getReadPointer(0), size);

	return *this;
}

float &VariantBuffer::operator [](int sampleIndex)
{
	CHECK_CONDITION(isPositiveAndBelow(sampleIndex, buffer.getNumSamples()), getName() + ": Invalid sample index" + String(sampleIndex));
	return buffer.getWritePointer(0)[sampleIndex];
}

const float & VariantBuffer::operator[](int sampleIndex) const
{
	CHECK_CONDITION(isPositiveAndBelow(sampleIndex, buffer.getNumSamples()), getName() + ": Invalid sample index" + String(sampleIndex));
	return buffer.getReadPointer(0)[sampleIndex];
}

VariantBuffer::Factory::Factory(int stackSize_) :
stackSize(stackSize_)
{
	sectionBufferStack.ensureStorageAllocated(stackSize);

	for (int i = 0; i < stackSize; i++)
	{
		sectionBufferStack.add(new VariantBuffer(0));
	}

	setMethod("create", create);
	setMethod("referTo", referTo);
}

VariantBuffer::Factory::~Factory()
{
	sectionBufferStack.clear();
}

var VariantBuffer::Factory::create(const var::NativeFunctionArgs &args)
{
	if (args.numArguments == 1)
	{
		VariantBuffer *b = new VariantBuffer((int)args.arguments[0]);
		return var(b);
	}
	else
	{
		VariantBuffer *b = new VariantBuffer(0);
		return var(b);
	}
}

var VariantBuffer::Factory::referTo(const var::NativeFunctionArgs &args)
{
	Factory *f = dynamic_cast<VariantBuffer::Factory*>(args.thisObject.getObject());

	CHECK_CONDITION((f != nullptr), "Factory Object is wrong");

	const var *firstArgs = args.arguments; // nice...

	CHECK_CONDITION((firstArgs->isBuffer()), "Referenced object is not a buffer");

	if (!firstArgs->isBuffer()) return var::undefined();

	VariantBuffer *b = f->getFreeVariantBuffer();

	// Don't wrap this into CHECK_CONDITION, because it could happen with working scripts...
	if (b == nullptr)
        throw String("Buffer stack size reached!");

	switch (args.numArguments)
	{
	case 1: b->referToOtherBuffer(firstArgs->getBuffer()); break;
	case 2: b->referToOtherBuffer(args.arguments[0].getBuffer(), (int)args.arguments[1]); break;
	case 3:	b->referToOtherBuffer(args.arguments[0].getBuffer(), (int)args.arguments[1], (int)args.arguments[2]); break;
	}

	return var(b);
}

VariantBuffer * VariantBuffer::Factory::getFreeVariantBuffer()
{
	for (int i = 0; i < stackSize; i++)
	{
		const int thisRefCount = sectionBufferStack.getUnchecked(i)->getReferenceCount();
		if (thisRefCount == 2)
		{
			VariantBuffer::Ptr p(sectionBufferStack[i]);

			return p.get();
		}
	}

	return nullptr;
}

void operator>>(float f, VariantBuffer &b)
{
	FloatVectorOperations::fill(b.buffer.getWritePointer(0), f, b.size);
}

} // namespace juce
