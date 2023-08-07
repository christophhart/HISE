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

	addMethods();
}

VariantBuffer::VariantBuffer(VariantBuffer *otherBuffer, int offset /*= 0*/, int numSamples /*= -1*/)
{
	referToOtherBuffer(otherBuffer, offset, numSamples);
	addMethods();
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

	addMethods();
}

VariantBuffer::~VariantBuffer()
{
	referencedBuffer = nullptr;
}

void VariantBuffer::sanitizeFloatArray(float** channels, int numChannels, int numSamples)
{
	if(numChannels == 2)
	{
		float* l = channels[0];
		float* r = channels[1];
            
		Range<float> l_r = FloatVectorOperations::findMinAndMax(l, numSamples);
		Range<float> r_r = FloatVectorOperations::findMinAndMax(r, numSamples);
            
		if(std::isnan(l_r.getStart()) || std::isnan(l_r.getEnd()))
		{
			FloatVectorOperations::clear(l, numSamples);
		}
            
		if(std::isnan(r_r.getStart()) || std::isnan(r_r.getEnd()))
		{
			FloatVectorOperations::clear(r, numSamples);
		}
	}
	{
		while(--numSamples >= 0)
		{
                
		}
	}
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
	CHECK_CONDITION(isPositiveAndBelow(sampleIndex, buffer.getNumSamples()), toDebugString() + " Error: Invalid get sample index: " + String(sampleIndex));
	return (*this)[sampleIndex];
}

void VariantBuffer::setSample(int sampleIndex, float newValue)
{
	CHECK_CONDITION(isPositiveAndBelow(sampleIndex, buffer.getNumSamples()), toDebugString() + " Error: Invalid set sample index: " + String(sampleIndex));

	buffer.setSample(0, sampleIndex, FloatSanitizers::sanitizeFloatNumber(newValue));
}

String VariantBuffer::toDebugString() const
{
	String description;

	description << "Buffer (size: " << size << ")";

	if (buffer.getNumSamples() == 0)
		return description;

	description << ", Max: " << String(buffer.getMagnitude(0, size), 3);
	description << ", RMS: " << String(buffer.getRMSLevel(0, 0, size), 3);

	return description;
}

void VariantBuffer::addMethods()
{
	setMethod("normalise", [](const var::NativeFunctionArgs& n)
	{
		if (auto bf = n.thisObject.getBuffer())
		{
			auto ptr = bf->buffer.getWritePointer(0, 0);

			auto mag = bf->buffer.getMagnitude(0, bf->size);

			float gain = 1.0f;

			if (n.numArguments > 0)
				gain = jlimit<float>(0.0f, 1.0f, Decibels::decibelsToGain(gain));

			if (mag > 0.0f)
				gain /= mag;

			FloatVectorOperations::multiply(ptr, gain, bf->size);
		}

		return var(0);
	});
    
    setMethod("toCharString", [](const var::NativeFunctionArgs& n)
    {
        if(auto bf = n.thisObject.getBuffer())
        {
            int numChars = bf->size;
            
            if(n.numArguments > 0)
                numChars = jmax(1, (int)n.arguments[0]);
            
            float min = 0.0f;
            float max = 1.0f;
            
            if(n.numArguments > 1)
            {
                min = (float)n.arguments[1][0];
                max = (float)n.arguments[1][1];
            }
             
            int samplesPerChar = bf->size / numChars;
            
            String s;
            s.preallocateBytes(numChars * 2);
            
            for(int i = 0; i < bf->size; i += samplesPerChar)
            {
                int numToCheck = jmin(samplesPerChar, bf->size-i);
                auto r = bf->buffer.findMinMax(0, i, numToCheck);
                
                float v = r.getEnd();
                
                if(std::abs(r.getStart()) > v)
                    v = r.getStart();
                
                v = jlimit(min, max, v);
                
                auto safe_offset = (float)(int)('(');
                auto safe_limit = 124.0f;
                
                auto v2 = (v - min) / (max - min) * (safe_limit - safe_offset);
                
                char msb = (char)(int)(v2) + (char)safe_offset;
                
                auto min2 = std::floor(v2);
                auto max2 = min2 + 1.0f;
                
                auto v3 = (v2 - min2) / (max2 - min2) * (safe_limit - safe_offset);
                
                char lsb = (char)(int)(v3) + (char)safe_offset;
                
                if(msb >= '\\')
                    msb++;
                
                if(lsb >= '\\')
                    lsb++;
                
                s << msb;
                s << lsb;
                
            }
            
            return var(s);
        }
        
        return var();
    });

#if HISE_INCLUDE_PITCH_DETECTION
	setMethod("detectPitch", [](const var::NativeFunctionArgs& n)
	{
		if (auto bf = n.thisObject.getBuffer())
		{
			if (n.numArguments == 0)
			{
				throw String("samplerate expected as first argument");
			}

			auto sampleRate = (double)n.arguments[0];

			int numSamples = bf->buffer.getNumSamples();

			if (n.numArguments > 2)
				numSamples = jmin(numSamples, (int)n.arguments[2]);

			int offset = 0;

			if (n.numArguments > 1)
				offset = jmin((int)n.arguments[1], bf->buffer.getNumSamples() - numSamples);

			return var(PitchDetection::detectPitch(bf->buffer, offset, numSamples, sampleRate));
		}

		return var(0);
	});
#endif

	setMethod("indexOfPeak", [](const var::NativeFunctionArgs& n)
	{
		if (auto bf = n.thisObject.getBuffer())
		{
			int numSamples = bf->buffer.getNumSamples();

			if (n.numArguments > 1)
				numSamples = jmin(numSamples, (int)n.arguments[1]);

			int offset = 0;

			if (n.numArguments > 0)
				offset = jmin((int)n.arguments[0], bf->buffer.getNumSamples() - numSamples);

			auto ptr = bf->buffer.getReadPointer(0, offset);

			int index = 0;
			auto maxValue = 0.0f;

			for (int i = 0; i < numSamples; i++)
			{
				auto thisValue = std::abs(ptr[i]);
                
				if (thisValue > maxValue)
				{
					maxValue = thisValue;
					index = i;
				}
			}

			return var(index + offset);
		}

		return var(0);
	});

	setMethod("toBase64", [](const var::NativeFunctionArgs& n)
	{
		if (auto bf = n.thisObject.getBuffer())
		{
			juce::MemoryBlock mb(bf->buffer.getReadPointer(0), sizeof(float) * bf->size);

			String b = "Buffer";
			b << mb.toBase64Encoding();
			return var(b);
		}

		return var(0);
	});

	setMethod("fromBase64", [](const var::NativeFunctionArgs& n)
	{
		if (auto bf = n.thisObject.getBuffer())
		{
			if (n.numArguments == 0)
				throw String("expected string");

			auto s = n.arguments[0].toString();
			MemoryBlock mb;

			if (!s.startsWith("Buffer"))
				return var(false);

			if (!mb.fromBase64Encoding(s.substring(6)))
				return var(false);

			auto newSize = mb.getSize() / sizeof(float);

			if (newSize > 44100)
				throw String("Too big");

			bf->buffer.setSize(1, (int)newSize);
			bf->size = (int)newSize;

			auto asFloatArray = reinterpret_cast<float*>(mb.getData());

			FloatVectorOperations::copy(bf->buffer.getWritePointer(0), asFloatArray, (int)newSize);

			return var(true);
		}

		return var(0);
	});

	setMethod("getMagnitude", [](const var::NativeFunctionArgs& n)
	{
		if (auto bf = n.thisObject.getBuffer())
		{
			int numSamples = bf->buffer.getNumSamples();

			if (numSamples == 0)
				return var(0.0f);

			if (n.numArguments > 1)
				numSamples = jlimit(0, numSamples, (int)n.arguments[1]);

			int offset = 0;

			if (n.numArguments > 0)
				offset = jlimit(0, jmax(0, bf->buffer.getNumSamples() - numSamples), (int)n.arguments[0]);

			return var(bf->buffer.getMagnitude(0, offset, numSamples));
		}
			

		return var(0);
	});
	
	setMethod("getRMSLevel", [](const var::NativeFunctionArgs& n)
	{
		if (auto bf = n.thisObject.getBuffer())
		{
			int numSamples = bf->buffer.getNumSamples();

			if (n.numArguments > 1)
				numSamples = jmin(numSamples, (int)n.arguments[1]);

			int offset = 0;

			if (n.numArguments > 0)
				offset = jmin((int)n.arguments[0], bf->buffer.getNumSamples() - numSamples);

			return var(bf->buffer.getRMSLevel(0, offset, numSamples));
		}
			

		return var(0);
	});

    setMethod("trim", [](const var::NativeFunctionArgs& n)
    {
        if(auto bf = n.thisObject.getBuffer())
        {
            int numToTrimFromStart = 0;
            int numToTrimFromEnd = 0;
            
            if(n.numArguments > 0)
                numToTrimFromStart = jlimit(0, bf->size - 1, (int)n.arguments[0]);
            if(n.numArguments > 1)
                numToTrimFromEnd = jlimit(0, bf->size - numToTrimFromStart, (int)n.arguments[1]);
            
            auto ptr = bf->buffer.getWritePointer(0, numToTrimFromStart);
            auto newSize = bf->size - numToTrimFromStart - numToTrimFromEnd;
            
            auto newBuffer = new VariantBuffer(newSize);
            
            FloatVectorOperations::copy(newBuffer->buffer.getWritePointer(0), ptr, newSize);
            
            return var(newBuffer);
        }
        
        return var();
    });
    
	setMethod("getPeakRange", [](const var::NativeFunctionArgs& n)
	{
		Array<var> range;

		if (auto bf = n.thisObject.getBuffer())
		{
			int numSamples = bf->buffer.getNumSamples();

			if (n.numArguments > 1)
				numSamples = jmin(numSamples, (int)n.arguments[1]);

			int offset = 0;

			if (n.numArguments > 0)
				offset = jmin((int)n.arguments[0], bf->buffer.getNumSamples() - numSamples);

			auto r = bf->buffer.findMinMax(0, offset, numSamples);
			range.add(r.getStart());
			range.add(r.getEnd());
		}
		else
		{
			range.add(0);
			range.add(0);
		}
			
		return var(range);
	});
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

VariantBuffer& VariantBuffer::operator+(float gain)
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

VariantBuffer& VariantBuffer::operator+(const VariantBuffer &b)
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

VariantBuffer& VariantBuffer::operator*(float gain)
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

VariantBuffer& VariantBuffer::operator*(const VariantBuffer &b)
{
	CHECK_CONDITION((b.size >= size), "second buffer too small: " + String(b.size));

	FloatVectorOperations::multiply(buffer.getWritePointer(0), b.buffer.getReadPointer(0), size);

	return *this;
}

float &VariantBuffer::operator [](int sampleIndex)
{
	CHECK_CONDITION(isPositiveAndBelow(sampleIndex, buffer.getNumSamples()), toDebugString() + " Error: Invalid sample index: " + String(sampleIndex));
	return buffer.getWritePointer(0)[sampleIndex];
}

const float & VariantBuffer::operator[](int sampleIndex) const
{
	CHECK_CONDITION(isPositiveAndBelow(sampleIndex, buffer.getNumSamples()), toDebugString() + " Error: Invalid sample index: " + String(sampleIndex));
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
