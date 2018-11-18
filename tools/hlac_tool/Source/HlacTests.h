/*  HISE Lossless Audio Codec
*	�2017 Christoph Hart
*
*	Redistribution and use in source and binary forms, with or without modification,
*	are permitted provided that the following conditions are met:
*
*	1. Redistributions of source code must retain the above copyright notice,
*	   this list of conditions and the following disclaimer.
*
*	2. Redistributions in binary form must reproduce the above copyright notice,
*	   this list of conditions and the following disclaimer in the documentation
*	   and/or other materials provided with the distribution.
*
*	3. All advertising materials mentioning features or use of this software must
*	   display the following acknowledgement:
*	   This product includes software developed by Hart Instruments
*
*	4. Neither the name of the copyright holder nor the names of its contributors may be used
*	   to endorse or promote products derived from this software without specific prior written permission.
*
*	THIS SOFTWARE IS PROVIDED BY CHRISTOPH HART "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
*	BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
*	DISCLAIMED. IN NO EVENT SHALL COPYRIGHT HOLDER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
*	SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
*	GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
*	THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
*	ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*/

#ifndef HLACTESTS_H_INCLUDED
#define HLACTESTS_H_INCLUDED

#include "JuceHeader.h"

using namespace hlac;

#if HLAC_INCLUDE_TEST_SUITE

struct BitCompressors::UnitTests : public UnitTest
{
	UnitTests() :
		UnitTest("Bit reduction unit tests")
	{}

	void runTest() override;
	void fillDataWithAllowedBitRange(int16* data, int size, int bitRange);
	void testCompressor(Base* compressor);

	void testAutomaticCompression(uint8 maxBitSize);

};

struct CodecTest : public UnitTest
{
	CodecTest();

	enum class SignalType
	{
		Empty,
		Static,
		FullNoise,
		SineOnly,
		MixedSine,
		RampDown,
		DecayingSineWithHarmonic,
		NastyDiracTrain,
		numSignalTypes
	};

	enum class Option
	{
		WholeBlock,
		Diff,
		numCompressorOptions
	};

	void runTest() override;

	void testIntegerBuffers();

	void testCodec(SignalType type, Option option, bool testStereo);

	void testCopyWithNormalisation();

	void testHiseSampleBuffer();

	void testNormalisation();

	static AudioSampleBuffer createTestSignal(int numSamples, int numChannels, SignalType type, float maxAmplitude);

	HlacEncoder::CompressorOptions options[(int)Option::numCompressorOptions];
	
	String getNameForOption(Option o) const;
	String getNameForSignal(SignalType s) const;

private:
	void testHiseSampleBufferMinNormalisation();
	void testHiseSampleBufferClearing();
};


#endif

#endif  // HLACTESTS_H_INCLUDED
