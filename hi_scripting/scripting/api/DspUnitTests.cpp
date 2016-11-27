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

#include "JuceHeader.h"

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