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

#include  "JuceHeader.h"

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



	Random r;
	


};

static DspUnitTests dspUnitTest;