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
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

#include "AppConfig.h"

#if HI_RUN_UNIT_TESTS

#include  "JuceHeader.h"

using namespace hise;


template <int RampLength> class AlignedSSERamper
{
	
	AlignedSSERamper(float* data_):
		data(data_)
	{
		jassert(dsp::SIMDRegister<float>::isSIMDAligned(data));
	}

	void ramp(float startValue, float endValue)
	{
		constexpr float ratio = 1.0f / (float)RampLength;
		const float delta1 = (endValue - startValue)*ratio;
		
		constexpr int numSSE = dsp::SIMDRegister<float>::SIMDRegisterSize / sizeof(float);
		constexpr int numLoop = RampLength / numSSE;

		__m128 delta = { delta1, delta1*2.0f, delta1*3.0f, delta1*4.0f };
		__m128 step = _mm_set1_ps(delta1 * 4.0f);

		int i = 0;

		for (int i = 0, i < numLoop; i++)
		{
			auto d = _mm_load_ps(data);

			auto thisStep = _mm_mul_ps(step, _mm_set1_ps((float)i));


				_mm_add_ps(d, step);
			_mm

				data += numSSE;
		}

		while (--numLoop >= 0)
		{
			
		}
	}


	float* data;
};

class DspUnitTests : public UnitTest
{
public:

	DspUnitTests():
		UnitTest("Testing Scripting DSP classes")
	{
		
	}

	void runTest() override
	{
		testVariantBuffer();

		testVariantBufferWithCorruptValues();

		testDspInstances();

		testCircularBuffers();
	}

	void testCircularBuffers()
	{
		beginTest("Testing circular audio buffers");

		int numSamples = r.nextInt({ 258, 512 });

		AudioSampleBuffer input(1, numSamples);

		fillFloatArrayWithRandomNumbers(input.getWritePointer(0), numSamples);

		AudioSampleBuffer output(1, numSamples);

		CircularAudioSampleBuffer b(1, 1024);

		expectEquals<int>(b.getNumAvailableSamples(), 0, "Num Samples available");

		b.writeSamples(input, 0, numSamples);

		expectEquals<int>(b.getNumAvailableSamples(), numSamples, "Num Samples available");

		b.readSamples(output, 0, numSamples);
		
		expectEquals<int>(b.getNumAvailableSamples(), 0, "Num Samples available");
		expect(checkBuffersEqual(input, output), "Basic Read/Write operation");

		


		beginTest("Testing MIDI circular Buffers");

		CircularAudioSampleBuffer mb(1, 1024);

		testMidiWrite(mb, 1000, 600);
		testMidiWrite(mb, 1000, 600);

		
		testMidiWrite(mb, r.nextInt({ 0, 512 }));
		testMidiWrite(mb, r.nextInt({ 0, 512 }));
		testMidiWrite(mb, r.nextInt({ 0, 512 }));
		testMidiWrite(mb, r.nextInt({ 0, 512 }));
		testMidiWrite(mb, r.nextInt({ 0, 512 }));
		testMidiWrite(mb, r.nextInt({ 0, 512 }));


	}

	void testMidiWrite(CircularAudioSampleBuffer& mb, int numThisTime, int timeStamp=-1)
	{

		MidiBuffer mInput;
		MidiBuffer mOutput;

		
		if(timeStamp == -1)
			timeStamp = r.nextInt({ 0, numThisTime });

		const int noteNumber = r.nextInt({ 0, 127 });

		String s;

		s << "Buffersize: " << numThisTime << ", Timestamp: " << timeStamp << ", NoteNumber: " << noteNumber;

		logMessage(s);

		mInput.addEvent(MidiMessage::noteOn(1, noteNumber, 1.0f), timeStamp);

		mb.writeMidiEvents(mInput, 0, numThisTime);

		mb.readMidiEvents(mOutput, 0, numThisTime);

		MidiBuffer::Iterator iter(mOutput);

		MidiMessage m;
		int pos;

		iter.getNextEvent(m, pos);

		jassert(timeStamp == pos);

		expect(m.getNoteNumber() == noteNumber, "Wrong event.");
		expectEquals<int>(pos, timeStamp, "Wrong timestamp.");
		expect(mb.getNumMidiEvents() == 0, "Buffer should be empty. Size: " + String(mb.getNumMidiEvents()));
	}

	

	void testVariantBuffer()
	{
		beginTest("Testing VariantBuffer class");

		VariantBuffer b(256);

		expectEquals<int>(b.buffer.getNumSamples(), 256, "VariantBuffer size");
		expectEquals<int>(b.size, 256, "VariantBuffer size 2");

		for (int i = 0; i < 256; i++)
		{
			expect((float)b.getSample(i) == 0.0f, "Clear sample at index " + String(i));
			expect(b.buffer.getSample(0, i) == 0.0f, "Clear sample at index " + String(i));
		}

		float otherData[128];

		fillFloatArrayWithRandomNumbers(otherData, 128);

		b.referToData(otherData, 128);

		for (int i = 0; i < 128; i++)
		{
			expectEquals<float>(otherData[i], b[i], "Sample at index " + String(i));
		}
		
		beginTest("Starting VariantBuffer operators");

		const float random1 = r.nextFloat();

		random1 >> b;

		for (int i = 0; i < b.size; i++)
		{
			expectEquals<float>(b[i], random1, "Testing fill operator");
		}
	}
	
	void testDspInstances()
	{
		DspFactory::Handler handler;

		DspFactory::Handler::registerStaticFactory<HiseCoreDspFactory>(&handler);
		
		DspFactory* coreFactory = handler.getFactory("core", "");

		expect(coreFactory != nullptr, "Creating the core factory");


		var sm = coreFactory->createModule("stereo");
		DspInstance* stereoModule = dynamic_cast<DspInstance*>(sm.getObject());
		expect(stereoModule != nullptr, "Stereo Module creation");

		VariantBuffer::Ptr lData = new VariantBuffer(256);
		VariantBuffer::Ptr rData = new VariantBuffer(256);

		Array<var> channels;

		channels.add(var(lData));
		channels.add(var(rData));

		try
		{
			stereoModule->processBlock(channels);
		}
		catch (String message)
		{
			expectEquals<String>(message, "stereo: prepareToPlay must be called before processing buffers.");
		}
		

	}


	void testVariantBufferWithCorruptValues()
	{
		VariantBuffer b(6);

		b.setSample(0, INFINITY);
		b.setSample(1, FLT_MIN / 20.0f);
		b.setSample(2, FLT_MIN / -14.0f);
		b.setSample(3, NAN);
		b.setSample(4, 24.0f);
		b.setSample(5, 0.0052f);

		expectEquals<float>(b[0], 0.0f, "Storing Infinity");
		expectEquals<float>(b[1], 0.0f, "Storing Denormal");
		expectEquals<float>(b[2], 0.0f, "Storing Negative Denormal");
		expectEquals<float>(b[3], 0.0f, "Storing NaN");
		expectEquals<float>(b[4], 24.0f, "Storing Normal Number");
		expectEquals<float>(b[5], 0.0052f, "Storing Small Number");

		1.0f >> b;
		b * INFINITY;
		expectEquals<float>(b.getSample(0), 0.0f, "Multiplying with infinity");

		1.0f >> b;
		b * (FLT_MIN / 20.0f);
		expectEquals<float>(b.getSample(0), 0.0f, "Multiplying with positive denormal");

		1.0f >> b;
		b * (FLT_MIN / -14.0f);
		expectEquals<float>(b.getSample(0), 0.0f, "Multiplying with negative denormal");

		1.0f >> b;
		b * NAN;
		expectEquals<float>(b.getSample(0), 0.0f, "Multiplying with NaN");

		VariantBuffer a(6);

		float* aData = a.buffer.getWritePointer(0);

		aData[0] = INFINITY;
		aData[1] = FLT_MIN / 20.0f;
		aData[2] = FLT_MIN / -14.0f;
		aData[3] = NAN;
		aData[4] = 24.0f;
		aData[5] = 0.0052f;

		a >> b;

		expectEquals<float>(b[0], 0.0f, "Copying Infinity");
		expectEquals<float>(b[1], 0.0f, "Copying Denormal");
		expectEquals<float>(b[2], 0.0f, "Copying Negative Denormal");
		expectEquals<float>(b[3], 0.0f, "Copying NaN");
		expectEquals<float>(b[4], 24.0f, "Copying Normal Number");
		expectEquals<float>(b[5], 0.0052f, "Copying Small Number");
	}



	void fillFloatArrayWithRandomNumbers(float *data, int numSamples)
	{
		for (int i = 0; i < numSamples; i++)
		{
			data[i] = r.nextFloat();
		}
	}

	bool checkBuffersEqual(const AudioSampleBuffer& first, const AudioSampleBuffer& second)
	{
		if (first.getNumSamples() != second.getNumSamples())
			return false;

		auto r1 = first.getReadPointer(0);
		auto r2 = second.getReadPointer(0);

		for (int i = 0; i < first.getNumSamples(); i++)
		{
			if (fabsf(*r2 - *r1) > 0.0001f)
				return false;

			r1++;
			r2++;
		}

		return true;
	}


	Random r;
	


};

static DspUnitTests dspUnitTest;


class CustomContainerTest : public UnitTest
{
public:

	struct DummyStruct
	{
		DummyStruct(int index_) :
			index(index_)
		{};

		DummyStruct() :
			index(0)
		{}

		int index;
	};

	struct DummyStruct2
	{
		DummyStruct2(int a1, int a2, int a3, int a4) :
			a({ a1, a2, a3, a4 })
		{

		}

		DummyStruct2() :
			a({ 0, 0, 0, 0 })
		{}


		int getSum() const
		{
			int sum = 0;
			for (auto a_ : a)
				sum += a_;

			return sum;
		}

	private:

		std::vector<int> a;

	};

	CustomContainerTest() :
		UnitTest("Testing custom containers")
	{

	}

	void runTest() override
	{
		testingUnorderedStack();

		testingLockFreeQueue();

		testBlockDivider();
	}

	

private:

	void testBlockDivider()
	{
		beginTest("Testing Block Divider");
		using SSEType = dsp::SIMDRegister<float>;

		int blockSize = 67;

		int numBlocks = 5;

		int totalLength = blockSize * numBlocks;

		ModulatorChain::Buffer totalData;
		totalData.setMaxSize(totalLength);

		ModulatorChain::Buffer blockData;
		blockData.setMaxSize(blockSize);

		auto data = blockData.scratchBuffer;

		FloatVectorOperations::fill(data, 0.5f, totalLength);

		auto startData = totalData.scratchBuffer;
		int startLength = totalLength;

		int counter = 0;
		
		constexpr int DividerBlockSize = 32;

		BlockDivider<DividerBlockSize> divider;

		int numProcessed = 0;
		int blockOffset = 0;

		while (totalLength > 0)
		{
			data = blockData.scratchBuffer;
			FloatVectorOperations::fill(data, 0.5, blockSize);
			
			counter = blockSize;

			while (counter > 0)
			{
				bool newBlock;
				int subBlockSize = divider.cutBlock(counter, newBlock, data);

				if (subBlockSize == 0)
				{
					expect(SSEType::isSIMDAligned(data));

					FloatVectorOperations::fill(data, 0.0f, DividerBlockSize);
					data[0] = 1.0f;

					numProcessed += DividerBlockSize;

					data += DividerBlockSize;
				}
				else
				{
					FloatVectorOperations::fill(data, 0.0f, subBlockSize);

					if (newBlock)
						data[0] = 1.0f;

					data += subBlockSize;
					numProcessed += subBlockSize;
				}
			}

			totalLength -= blockSize;
			FloatVectorOperations::copy(startData + blockOffset, blockData.scratchBuffer, blockSize);
			blockOffset += blockSize;
		}

		for (int i = 0; i < startLength; i++)
		{
			const float expected = i % DividerBlockSize == 0 ? 1.0f : 0.0f;

			expectEquals<float>(startData[i], expected, "Position: " + String(i));
		}

		blockData.clear();
		totalData.clear();
	}

	void testingUnorderedStack()
	{
		beginTest("Testing Unordered Stack: basic functions with ints");

		UnorderedStack<int> intStack;

		intStack.insert(1);
		intStack.insert(2);
		intStack.insert(3);

		expectEquals<int>(intStack.size(), 3, "Size after insertion");

		intStack.remove(2);

		expectEquals<int>(intStack.size(), 2, "Size after deletion");

		intStack.insert(4);

		expectEquals<int>(intStack[0], 1, "First Element");
		expectEquals<int>(intStack[1], 3, "Second Element");
		expectEquals<int>(intStack[2], 4, "Third Element");
		expectEquals<int>(intStack[3], 0, "Default Element");

		expect(intStack.contains(4), "Contains int 1");
		expect(!intStack.contains(5), "Contains int 1");

		const float data[5] = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f };

		beginTest("Testing Unordered Stack: functions with float pointer");

		UnorderedStack<const float*> fpStack;

		expect(fpStack[0] == nullptr, "Null pointer");

		fpStack.insert(data);
		fpStack.insert(data + 1);
		fpStack.insert(data + 2);
		fpStack.insert(data + 3);
		fpStack.insert(data + 4);

		expectEquals<int>(fpStack.size(), 5);

		expect(fpStack[5] == nullptr, "Null pointer 2");

		expectEquals<float>(*fpStack[0], data[0], "Float pointer elements");
		expectEquals<float>(*fpStack[1], data[1], "Float pointer elements");
		expectEquals<float>(*fpStack[2], data[2], "Float pointer elements");
		expectEquals<float>(*fpStack[3], data[3], "Float pointer elements");
		expectEquals<float>(*fpStack[4], data[4], "Float pointer elements");

		expect(fpStack.contains(data + 2), "Contains float* 1");

		float d2 = 2.0f;

		expect(!fpStack.contains(&d2), "Contains not float 2");
		expect(!fpStack.contains(nullptr), "No null");

		fpStack.remove(data + 2);

		fpStack.insert(data + 2);

		expectEquals<float>(*fpStack[0], data[0], "Float pointer elements after shuffle 1");
		expectEquals<float>(*fpStack[1], data[1], "Float pointer elements after shuffle 2");
		expectEquals<float>(*fpStack[2], data[4], "Float pointer elements after shuffle 3");
		expectEquals<float>(*fpStack[3], data[3], "Float pointer elements after shuffle 4");
		expectEquals<float>(*fpStack[4], data[2], "Float pointer elements after shuffle 5");

		beginTest("Testing Unordered Stack: with dummy struct");

		OwnedArray<DummyStruct> elements;

		UnorderedStack<DummyStruct*> elementStack;

		for (int i = 0; i < UNORDERED_STACK_SIZE; i++)
		{
			elements.add(new DummyStruct(i));
		}

		Random r;

		for (int i = 0; i < 1000; i++)
		{
			const int indexToInsert = r.nextInt(Range<int>(0, elements.size() - 1));

			auto ds = elements[indexToInsert];

			if (!elements.contains(ds))
			{
				elementStack.insert(ds);
			}
			else
			{
				elementStack.remove(ds);
			}
		}

		for (int i = 0; i < elementStack.size(); i++)
		{
			expect(elementStack[i] != nullptr);
		}

		for (int i = elementStack.size(); i < UNORDERED_STACK_SIZE; i++)
		{
			expect(elementStack[i] == nullptr);
		}
	}

	void testingLockFreeQueue()
	{

		testLockFreeQueueWithInt();
		testLockFreeQueueWithDummyStruct();
		testLockFreeQueueWithLambda();


	}

	void testLockFreeQueueWithInt()
	{
		beginTest("Testing Lockfree Queue with ints");

		LockfreeQueue<int> q(1024);

		expect(q.push(1), "Push first element");
		expect(q.push(2), "Push second element");
		expect(q.push(3), "Push third element");

		expectEquals(q.size(), 3, "Size");

		int result;

		expect(q.pop(result), "Pop First Element");
		expectEquals<int>(result, 1, "first element value");

		expect(q.pop(result), "Pop Second Element");
		expectEquals<int>(result, 2, "second element value");

		expect(q.pop(result), "Pop Third Element");
		expectEquals<int>(result, 3, "third element value");

		expect(q.isEmpty(), "Queue is empty");
		expect(q.pop(result) == false, "Return false when empty");


		q.push(1);
		q.push(2);
		q.push(3);

		int sum = 0;

		LockfreeQueue<int>::ElementFunction makeSum = [&sum](int& i)->bool { sum += i; return i != 0; };

		expect(q.callForEveryElementInQueue(makeSum), "Call function for element");

		expect(q.isEmpty(), "Empty after call");
		expectEquals<int>(sum, 6, "Function call result");


	};

	void testLockFreeQueueWithDummyStruct()
	{
		beginTest("Testing Lockfree Queue with dummy struct");

		LockfreeQueue<DummyStruct2> q2(3);

		expect(q2.push(DummyStruct2(1, 2, 3, 4)), "Pushing first dummy struct");
		expect(q2.push(DummyStruct2(2, 3, 4, 5)), "Pushing second dummy struct");
		expect(q2.push(DummyStruct2(3, 4, 5, 6)), "Pushing third dummy struct");
		expect(q2.push(DummyStruct2(4, 5, 6, 7)) == false, "Pushing forth dummy struct");
		expect(q2.push(DummyStruct2(4, 5, 6, 7)) == false, "Pushing fifth dummy struct");

		DummyStruct2 result;


		expect(q2.pop(result), "Pop first element");
		expectEquals<int>(result.getSum(), 1 + 2 + 3 + 4, "Sum of first element");

		int sum = 0;

		LockfreeQueue<DummyStruct2>::ElementFunction makeSum = [&sum](DummyStruct2& s)->bool{ sum += s.getSum(); return true; };

		const int expectedSum = 2 + 3 + 4 + 5 + 3 + 4 + 5 + 6;

		q2.callForEveryElementInQueue(makeSum);

		expect(q2.isEmpty(), "Queue is empty");
		expectEquals<int>(sum, expectedSum, "Accumulate function calls");

	}

	void testLockFreeQueueWithLambda()
	{
		beginTest("Testing Lockfree Queue with lambda");

		using TestFunction = std::function<bool()>;

		LockfreeQueue<TestFunction> q(100);

		int sum = 0;

		TestFunction addTwo = [&sum]() { sum += 2; return true; };

		LockfreeQueue<TestFunction>::ElementFunction call = [](TestFunction& f) {f(); return true; };

		call(addTwo);
		call(addTwo);
		call(addTwo);

		expectEquals<int>(sum, 6, "lambda is working");

		sum = 0;

		q.push([&sum]() {sum += 1; return true; });
		q.push([&sum]() {sum += 2; return true; });
		q.push([&sum]() {sum += 3; return true; });
		
		q.callForEveryElementInQueue(call);

		expect(q.isEmpty(), "Queue empty after calling");

		expectEquals<int>(sum, 6, "Result after calling");

		sum = 0;

		q.push([&sum]() {sum += 2; return true; });
		q.push([&sum]() {sum += 3; return true; });
		q.push([&sum]() {sum += 4; return true; });

		expect(q.callEveryElementInQueue(), "Direct call");
		expectEquals<int>(sum, 9, "Result after direct call");
	}

};


static CustomContainerTest unorderedStackTest;



#endif