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
namespace jit {
using namespace juce;
using namespace asmjit;


#if INCLUDE_BUFFERS

struct BufferHolder
{
	BufferHolder(juce::VariantBuffer* buffer) :
		b(buffer)
	{};

	BufferHolder():
		b(nullptr)
	{}

	BufferHolder(int size) :
		b(new juce::VariantBuffer(size))
	{}

	~BufferHolder()
	{
		b = nullptr;
	}

	void setBuffer(juce::VariantBuffer* newBuffer)
	{
		b = newBuffer;
	}

	juce::VariantBuffer::Ptr b;

	int overflowError = -1;

private:

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BufferHolder);
};

typedef BufferHolder Buffer;


struct BufferOperations
{
	static float getSample(Buffer* b, int index)
	{
		if (b->b.get() == nullptr)
			return 0.0f;

		auto& vb = *b->b.get();

		if (isPositiveAndBelow(index, vb.size))
		{
			return vb.buffer.getReadPointer(0)[index];
		}
		else
		{
			b->overflowError = index;
			return 0.0f;
		}
	};

	static float getSampleRaw(Buffer*b, int index)
	{
		return b->b->buffer.getReadPointer(0)[index];
	}

	template <typename DummyReturn> static DummyReturn setSampleRaw(Buffer* b, int index, float value)
	{
		b->b->buffer.getWritePointer(0)[index] = value;
		return DummyReturn();
	}

	template <typename DummyReturn> static DummyReturn setSample(Buffer* b, int index, float value)
	{
		if (b->b.get() == nullptr)
			return DummyReturn();

		auto& vb = *b->b.get();

		if (isPositiveAndBelow(index, vb.size))
		{
			vb.buffer.getWritePointer(0)[index] = value;
		}
		else
		{
			b->overflowError = index;
		}

		return DummyReturn();
	}

	template <typename DummyReturn> static DummyReturn setSize(Buffer* b, int newSize)
	{
		b->b = new juce::VariantBuffer(newSize);

		return DummyReturn();
	}

	
};

#endif


} // end namespace jit
} // end namespace snex




