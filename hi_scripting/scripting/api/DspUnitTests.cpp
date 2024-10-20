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

		channels.add(var(lData.get()));
		channels.add(var(rData.get()));

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

//static DspUnitTests dspUnitTest;


class ModulationTests : public UnitTest
{
public:

	using ScopedProcessor = ScopedPointer<BackendProcessor>;

	ModulationTests() :
		UnitTest("Modulation Tests")
	{

	}

	void runTest() override
	{
		runBasicTest();
	}

private:

	

	void runBasicTest()
	{
		ScopedValueSetter<bool> s(MainController::unitTestMode, true);



		testResuming(false);
		testResuming(true);

		testSimpleEnvelopeWithoutAttack(true);
		testSimpleEnvelopeWithoutAttack(false);

		testSimpleEnvelopeWithAttack(false);
		testSimpleEnvelopeWithAttack(true);

		testAhdsrSustain(true);
		testAhdsrSustain(false);

		testConstantModulator(false);
		testConstantModulator(true);

		testLFOSeq(false);
		testLFOSeq(true);

		testModulatorCombo(false);
		testModulatorCombo(true);

		testPanModulation(false);
		testPanModulation(true);

		testSynthGroup();

		testGlobalModulators(false);
		testGlobalModulators(true);
		
		testGlobalModulatorIntensity(false);
		testGlobalModulatorIntensity(true);
	
		testScriptPitchFade(false);
		testScriptPitchFade(true);
	}

	void testPanModulation(bool useGroup)
	{
		beginTestWithOptionalGroup("Testing pan modulation ", useGroup);

		// Init
		ScopedProcessor bp = Helpers::createWithOptionalGroup(NoiseSynth::DC, useGroup);

		Helpers::setAttribute<SimpleEnvelope>(bp, SimpleEnvelope::Attack, 0.0f);
		Helpers::setAttribute<SimpleEnvelope>(bp, SimpleEnvelope::Release, 0.0f);

		// Setup

		Helpers::addVoiceEffectToOptionalGroup<StereoEffect>(bp);

		auto lfo = Helpers::addTimeModulator<StereoEffect, LfoModulator>(bp, 0);

		Helpers::setLfoToDefaultSquare(lfo);

		// Process

		auto testData = Helpers::createTestDataWithOneSecondNote(1024);
		Helpers::process(bp, testData, 512);

		// Tests

		expect(testData.isWithinErrorRange(10000, sqrt(2.0f), 0), "Left channel 1");
		expect(testData.isWithinErrorRange(10000, 0.0f, 1), "Right channel 1");
		expect(testData.isWithinErrorRange(15000, 0.0f, 0), "Left channel 2");
		expect(testData.isWithinErrorRange(15000, sqrt(2.0f), 1), "Left channel 2");

		bp = nullptr;
	}

	void testScriptPitchFade(bool useGroup)
	{
		beginTestWithOptionalGroup("Testing script pitch fade", useGroup);

		// Init
		ScopedProcessor bp = Helpers::createWithOptionalGroup(NoiseSynth::DiracTrain, useGroup);

		Helpers::setAttribute<SimpleEnvelope>(bp, SimpleEnvelope::Attack, 0.0f);
		Helpers::setAttribute<SimpleEnvelope>(bp, SimpleEnvelope::Release, 0.0f);

		// Setup
		
		const String pitchMidiProcessor = R"(function onNoteOn(){Synth.addPitchFade(Message.getEventId(),500,12,0);})" \
			R"(function onNoteOff(){}function onController(){}function onTimer(){}function onControl(number, value){})";

		auto jp = new JavascriptMidiProcessor(bp, "scripter");

		auto mpc = dynamic_cast<MidiProcessorChain*>(bp->getMainSynthChain()->getChildProcessor(ModulatorSynth::MidiProcessor));
		jp->setOwnerSynth(bp->getMainSynthChain());
		jp->parseSnippetsFromString(pitchMidiProcessor, true);
		mpc->getHandler()->add(jp, nullptr);

		// Process

		
		auto testData = Helpers::createTestDataWithOneSecondNote();
		Helpers::process(bp, testData, 512);

		Helpers::DiracIterator di(testData.audioBuffer, 0, false);
		expectResult(di.scan(), "Dirac scan");

		auto& data = di.getData();

		expectResult(data.matchesStartAndEnd(256, 128), "Start and end");
		expectResult(data.isWithinRange({ 128, 256 }), "Outside range");

		// Tests

		bp = nullptr;
	}

	void testGlobalModulatorIntensity(bool useGroup)
	{
		constexpr int sampleRate = 44100;

		beginTestWithOptionalGroup("Testing global modulator intensity ", useGroup);

		// Init
		ScopedProcessor bp = Helpers::createWithOptionalGroup(NoiseSynth::DC, useGroup);

		Helpers::setAttribute<SimpleEnvelope>(bp, SimpleEnvelope::Attack, 10.0f);
		Helpers::setAttribute<SimpleEnvelope>(bp, SimpleEnvelope::Release, 20.0f);

		// Setup

		Helpers::addGlobalContainer(bp, useGroup);

		auto sender = Helpers::addTimeModulator<GlobalModulatorContainer, ControlModulator>(bp, ModulatorSynth::GainModulation);
		auto receiver = Helpers::addTimeModulatorToOptionalGroup<GlobalTimeVariantModulator>(bp, ModulatorSynth::GainModulation);
		auto checkMod = Helpers::addTimeModulatorToOptionalGroup<ControlModulator>(bp, ModulatorSynth::GainModulation);

		bp->prepareToPlay(sampleRate, 512);

		sender->setAttribute(ControlModulator::SmoothTime, 0.0f, dontSendNotification);
		sender->setAttribute(ControlModulator::DefaultValue, 0.0f, dontSendNotification);
		sender->setIntensity(0.75f);

		checkMod->setAttribute(ControlModulator::SmoothTime, 0.0f, dontSendNotification);
		checkMod->setAttribute(ControlModulator::DefaultValue, 0.0f, dontSendNotification);
		checkMod->setIntensity(0.75f);

		receiver->setBypassed(true);
		checkMod->setBypassed(true);


		receiver->connectToGlobalModulator("Container:" + sender->getId());

		expect(receiver->isConnected(), "Connection failed");

		// Process

		receiver->setBypassed(true);
		checkMod->setBypassed(false);
		
		Random r;

		const int startOffset = r.nextInt(4096);
		const int stopOffset = r.nextInt({ 8192, 32768 });

		const float nextIntensity = r.nextFloat();


		auto testData = Helpers::createTestDataWithOneSecondNote(startOffset, stopOffset);
		Helpers::process(bp, testData, 512, 16384);

		checkMod->setIntensity(nextIntensity);

		Helpers::resumeProcessing(bp, testData, 512, -1, 16384);

		receiver->setBypassed(false);
		checkMod->setBypassed(true);

		auto testData2 = Helpers::createTestDataWithOneSecondNote(startOffset, stopOffset);
		Helpers::process(bp, testData2, 512, 16384);

		sender->setIntensity(nextIntensity);

		Helpers::resumeProcessing(bp, testData2, 512, -1, 16384);


		// Tests

		expect(testData == testData2, "Compare static intensity");

		bp = nullptr;
	}

	void testGlobalModulators(bool useGroup)
	{
		beginTestWithOptionalGroup("Testing global modulators", useGroup);

		// Init
		ScopedProcessor bp = Helpers::createWithOptionalGroup(NoiseSynth::DiracTrain, useGroup);

		Helpers::setAttribute<SimpleEnvelope>(bp, SimpleEnvelope::Attack, 0.0f);
		Helpers::setAttribute<SimpleEnvelope>(bp, SimpleEnvelope::Release, 0.0f);

		// Setup

		Helpers::addGlobalContainer(bp, useGroup);
		
		auto sender = Helpers::addVoiceModulator<GlobalModulatorContainer, VelocityModulator>(bp, ModulatorSynth::GainModulation);
		auto receiver = Helpers::addVoiceModulatorToOptionalGroup<GlobalVoiceStartModulator>(bp, ModulatorSynth::GainModulation);

		receiver->connectToGlobalModulator("Container:" + sender->getId());

		expect(receiver->isConnected(), "Connection failed");

		// Process

		auto testData = Helpers::createTestDataWithOneSecondNote();
		Helpers::process(bp, testData, 512);

		// Tests

		bp = nullptr;
	}

	void testSynthGroup()
	{
		beginTest("Testing SynthGroup modulators");

		// Init
		ScopedProcessor bp = Helpers::createAndInitialiseProcessorWithGroup(NoiseSynth::DiracTrain);
		Helpers::setAttribute<SimpleEnvelope>(bp, SimpleEnvelope::Attack, 0.0f);
		Helpers::setAttribute<SimpleEnvelope>(bp, SimpleEnvelope::Release, 0.0f);

		// Setup

		auto group = ProcessorHelpers::getFirstProcessorWithType<ModulatorSynthGroup>(bp->getMainSynthChain());
		group->setAttribute(ModulatorSynthGroup::SpecialParameters::UnisonoVoiceAmount, 2.0f, dontSendNotification);
		group->setAttribute(ModulatorSynthGroup::SpecialParameters::UnisonoDetune, 6.0f, dontSendNotification);

		auto sineSynth = new SineSynth(bp, "FM", NUM_POLYPHONIC_VOICES);
		sineSynth->setAttribute(ModulatorSynth::Gain, 0.0f, dontSendNotification);

		group->getHandler()->add(sineSynth, nullptr);

		group->setAttribute(ModulatorSynthGroup::SpecialParameters::EnableFM, false, dontSendNotification);
		group->setAttribute(ModulatorSynthGroup::SpecialParameters::CarrierIndex, 1, dontSendNotification);
		group->setAttribute(ModulatorSynthGroup::SpecialParameters::ModulatorIndex, 2, dontSendNotification);

		

		auto constantPitch = Helpers::addVoiceModulator<ModulatorSynthGroup, ConstantModulator>(bp, ModulatorSynth::PitchModulation);

		const float pf = 1.5f;
		auto npf = Modulation::PitchConverters::pitchFactorToNormalisedRange(pf);
		constantPitch->setIntensity(npf);

		// Process

		const int offset = 128;

		auto testData = Helpers::createTestDataWithOneSecondNote(offset);
		Helpers::process(bp, testData, 512);

		group->setAttribute(ModulatorSynthGroup::SpecialParameters::EnableFM, true, dontSendNotification);
		expect(group->fmIsCorrectlySetup(), "FM not Working");
		
		auto testData2 = Helpers::createTestDataWithOneSecondNote(offset);
		Helpers::process(bp, testData2, 512);

		// Test

		expect(testData.isWithinErrorRange(offset, -1.0f, 0), "First sample left");
		expect(testData.isWithinErrorRange(offset, -1.0f, 1), "First sample right");

		float pitchFactorRight = pf * sqrt(2.0f); // 6 semitones down
		float pitchFactorLeft = 0.5f * pitchFactorRight;

		const int leftNextDirac = offset + roundToInt(ceil(256.0 / pitchFactorLeft));
		const int rightNextDirac = offset + roundToInt(ceil(256.0 / pitchFactorRight));

		expect(testData.isWithinErrorRange(leftNextDirac - 1, 0.0f, 0), "leftNextDirac - 1");
		expect(testData.isWithinErrorRange(rightNextDirac - 1, 0.0f, 1), "rightNextDirac - 1");

		expect(testData.isWithinErrorRange(leftNextDirac, 1.0f, 0), "leftNextDirac + 1");
		expect(testData.isWithinErrorRange(rightNextDirac, 1.0f, 1), "rightNextDirac - 1");

		expect(testData.isWithinErrorRange(leftNextDirac + 1, 0.0f, 0), "leftNextDirac + 1");
		expect(testData.isWithinErrorRange(rightNextDirac + 1, 0.0f, 1), "rightNextDirac - 1");

		expect(testData == testData2, "Detune for FM");

		bp = nullptr;
	}

	void testModulatorCombo(bool useGroup)
	{
		beginTestWithOptionalGroup("Test modulator combination", useGroup);

		// Init

		ScopedProcessor bp = Helpers::createWithOptionalGroup(NoiseSynth::DC, useGroup);

		// Setup


		const int attackSamples = 2048;
		const float attackMs = (float)attackSamples / (float)44100 * 1000.0f;

		Helpers::setAttribute<SimpleEnvelope>(bp, SimpleEnvelope::Attack, attackMs);
		Helpers::setAttribute<SimpleEnvelope>(bp, SimpleEnvelope::Release, 20.0f);
		auto constantMod = Helpers::addVoiceModulatorToOptionalGroup<ConstantModulator>(bp, ModulatorSynth::GainModulation);
		const float cModValue = 0.3f;

		constantMod->setIntensity(cModValue);

		auto lfoMod = Helpers::addTimeModulatorToOptionalGroup<LfoModulator>(bp, ModulatorSynth::GainModulation);
		Helpers::setLfoToDefaultSquare(lfoMod);
		const float lfoModValue = 0.25f;

		lfoMod->setIntensity(lfoModValue);
		
		const float gainFactorWhenLFO = (1.0f - cModValue) * (1.0f - lfoModValue);
		const float gainFactorWhenNoLFO = (1.0f - cModValue);

		// Process

		auto testData = Helpers::createTestDataWithOneSecondNote();
		Helpers::process(bp, testData, 512);

		// Test

		expect(testData.isWithinErrorRange(0, 0.0f), "First sample");

		float halfEnvelopeSample = 0.5f;
		int halfEnvelopeIndex = attackSamples / 2;

		expect(testData.isWithinErrorRange(halfEnvelopeIndex, halfEnvelopeSample * gainFactorWhenNoLFO), "Half Envelope");

		float fullEnvelopeSample = 1.0f;

		expect(testData.isWithinErrorRange(attackSamples, fullEnvelopeSample * gainFactorWhenNoLFO), "Full Envelope");

		const int sampleIndexWhenNoLFO = 2205;
		const int sampleIndexWhenLFO = 8802;

		expect(testData.isWithinErrorRange(sampleIndexWhenLFO, fullEnvelopeSample * gainFactorWhenLFO), "LFO On");
		expect(testData.isWithinErrorRange(sampleIndexWhenNoLFO, fullEnvelopeSample * gainFactorWhenNoLFO), "LFO Off");

	}

	void testResuming(bool useGroup)
	{
		beginTestWithOptionalGroup("Testing resuming", useGroup);

		// Init
		ScopedProcessor bp = Helpers::createWithOptionalGroup(NoiseSynth::DC, useGroup);

		Helpers::setAttribute<SimpleEnvelope>(bp, SimpleEnvelope::Attack, 0.0f);
		Helpers::setAttribute<SimpleEnvelope>(bp, SimpleEnvelope::Release, 0.0f);

		// Setup

		// Process

		auto testData = Helpers::createTestDataWithOneSecondNote();
		Helpers::process(bp, testData, 512);

		auto testData2 = Helpers::createTestDataWithOneSecondNote();
		Helpers::process(bp, testData2, 512, 16384);
		Helpers::resumeProcessing(bp, testData2, 512, -1, 16384);

		// Tests

		expect(testData == testData2, "Resume OK");



		bp = nullptr;
	}


	void testSimpleEnvelopeWithoutAttack(bool useGroup)
	{
		beginTestWithOptionalGroup("Testing simple envelope without attack", useGroup);

		// Init
		ScopedProcessor bp = Helpers::createWithOptionalGroup(NoiseSynth::DiracTrain, useGroup);

		Helpers::setAttribute<SimpleEnvelope>(bp, SimpleEnvelope::Attack, 0.0f);
		Helpers::setAttribute<SimpleEnvelope>(bp, SimpleEnvelope::Release, 0.0f);

		// Setup

		// Process

		Random r;

		const int stopOffset = r.nextInt({ 20000, 40000 });

		logMessage("Offset: " + String(stopOffset));

		auto testData = Helpers::createTestDataWithOneSecondNote(0, stopOffset);
		Helpers::process(bp, testData, 512);

		// Tests

		expectEquals<float>(testData.audioBuffer.getSample(0, 0), -1.0f, "First Dirac doesn't match");
		expectEquals<float>(testData.audioBuffer.getSample(0, 256), 1.0f, "Second Dirac doesn't match");

		double rasterValue = (double)stopOffset / (double)HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR;

		int rasteredTimestamp = (int)(floor(rasterValue) * (double)HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR);

		auto diracBeforeNoteOff = (int)(floor((double)rasteredTimestamp / 256.0) * 256.0);
		auto diracAfterNoteOff = diracBeforeNoteOff + 256;

		expectEquals<float>(testData.audioBuffer.getSample(0, diracBeforeNoteOff), 1.0f, "Dirac before note off");
		expectEquals<float>(testData.audioBuffer.getSample(0, diracAfterNoteOff), 0.0f, "Dirac after note off");

		bp = nullptr;
	}


	void testSimpleEnvelopeWithAttack(bool useGroup)
	{
		beginTestWithOptionalGroup("Testing simple envelope without attack", useGroup);

		ScopedProcessor bp = Helpers::createWithOptionalGroup(NoiseSynth::DC, useGroup);

		auto testData = Helpers::createTestDataWithOneSecondNote();

		float attackSamples = 512;
		float attackMs = (float)attackSamples / (float)44100 * 1000.0f;

		Helpers::setAttribute<SimpleEnvelope>(bp, SimpleEnvelope::Attack, attackMs);
		Helpers::process(bp, testData, 512);

		expectEquals<float>(testData.audioBuffer.getSample(0, 0), 0, "First Dirac doesn't match");
		expectEquals<float>(testData.audioBuffer.getSample(0, (int)attackSamples/2), 0.5f, "First Dirac doesn't match");
		expectEquals<float>(testData.audioBuffer.getSample(0, (int)attackSamples), 1.0f, "First Dirac doesn't match");
	}

	void testConstantModulator(bool useGroup)
	{
		beginTestWithOptionalGroup("Testing constant modulator", useGroup);

		// Init

		ScopedProcessor bp = Helpers::createWithOptionalGroup(NoiseSynth::DiracTrain, useGroup);

		Helpers::setAttribute<SimpleEnvelope>(bp, SimpleEnvelope::Attack, 0.0f);
		Helpers::setAttribute<SimpleEnvelope>(bp, SimpleEnvelope::Release, 0.0f);

		// Setup

		// Gain Mod -> 0.5
		// Pan Mod -> 25 R
		// Pitch Mod -1 octave => 512 Dirac

		const float balance = 0.25f;
		const float gain = 0.5f;

		auto constantGainMod = Helpers::addVoiceModulatorToOptionalGroup<ConstantModulator>(bp, ModulatorSynth::GainModulation);
		auto constantPitchMod = Helpers::addVoiceModulatorToOptionalGroup<ConstantModulator>(bp, ModulatorSynth::PitchModulation);

		auto stereoEffect = Helpers::addVoiceEffectToOptionalGroup<StereoEffect>(bp);
		stereoEffect->setAttribute(StereoEffect::Pan, 100.0f, dontSendNotification);
		
		auto constantPanMod = Helpers::addVoiceModulator<StereoEffect, ConstantModulator>(bp, StereoEffect::BalanceChain);

		constantGainMod->setIntensity(gain);
		constantPitchMod->setIntensityFromSlider(-12.0f);
		constantPanMod->setIntensity(balance);

		// Process

		auto testData = Helpers::createTestDataWithOneSecondNote();
		Helpers::process(bp, testData, 512);

		// Test

		Helpers::DiracIterator di(testData.audioBuffer, 0, false);

		di.scan();

		di.dump(this);

		Helpers::DiracIterator di2(testData.audioBuffer, 1, false);

		di2.scan();

		di2.dump(this);

		auto expectedFirstValue = -1.0f;
		auto gain_l = BalanceCalculator::getGainFactorForBalance(balance * 100, true) * gain;
		auto gain_r = BalanceCalculator::getGainFactorForBalance(balance * 100, false) * gain;

		expectResult(testData.isWithinErrorRange(0, expectedFirstValue * gain_l, 0), "Gain + Pan");
		expectResult(testData.isWithinErrorRange(0, expectedFirstValue * gain_r, 1), "Gain + Pan");

		auto expectedHalfValue = 0.0f;

		expectResult(testData.isWithinErrorRange(256, expectedHalfValue, 0), "Test pitch");
		expectResult(testData.isWithinErrorRange(256, expectedHalfValue, 1), "Test pitch");

		auto expectedFullValue = 1.0f;

		expectResult(testData.isWithinErrorRange(512, expectedFullValue * gain_l, 0), "Test pitch");
		expectResult(testData.isWithinErrorRange(512, expectedFullValue * gain_r, 1), "Test pitch");
	}

	void testAhdsrSustain(bool useGroup)
	{
		beginTestWithOptionalGroup("Testing AHDSR envelope sustain value", useGroup);

		ScopedProcessor bp = Helpers::createWithOptionalGroup(NoiseSynth::DC, useGroup);

		Helpers::get<SimpleEnvelope>(bp)->setBypassed(true);

		Helpers::addVoiceModulatorToOptionalGroup<AhdsrEnvelope>(bp, ModulatorSynth::GainModulation);

		const float sustainLevel = 0.25f;

		Helpers::setAttribute<AhdsrEnvelope>(bp, AhdsrEnvelope::Attack, 0.0f);
		Helpers::setAttribute<AhdsrEnvelope>(bp, AhdsrEnvelope::Hold, 0.0f);
		Helpers::setAttribute<AhdsrEnvelope>(bp, AhdsrEnvelope::Decay, 100.0f);
		Helpers::setAttribute<AhdsrEnvelope>(bp, AhdsrEnvelope::Sustain, Decibels::gainToDecibels(sustainLevel));

		auto testData = Helpers::createTestDataWithOneSecondNote();

		Helpers::process(bp, testData, 512);

		expectResult(testData.isWithinErrorRange(22050, sustainLevel), "Sustain value");
	}

	void testLFOSeq(bool useGroup)
	{
		beginTestWithOptionalGroup("Testing LFO Seq", useGroup);

		// Init
		ScopedProcessor bp = Helpers::createWithOptionalGroup(NoiseSynth::DC, useGroup);

		Helpers::setAttribute<SimpleEnvelope>(bp, SimpleEnvelope::Attack, 0.0f);
		Helpers::setAttribute<SimpleEnvelope>(bp, SimpleEnvelope::Release, 0.0f);

		// Setup

		auto squareLFO = Helpers::addTimeModulatorToOptionalGroup<LfoModulator>(bp, ModulatorSynth::GainModulation);
		auto seqLFO = Helpers::addTimeModulatorToOptionalGroup<LfoModulator>(bp, ModulatorSynth::GainModulation);

		squareLFO->setAttribute(LfoModulator::Parameters::TempoSync, 1.0f, dontSendNotification);
		seqLFO->setAttribute(LfoModulator::Parameters::TempoSync, 1.0f, dontSendNotification);
		squareLFO->setAttribute(LfoModulator::Frequency, (float)(int)TempoSyncer::Eighth, dontSendNotification);
		seqLFO->setAttribute(LfoModulator::Frequency, (float)(int)TempoSyncer::Sixteenth, dontSendNotification);

		squareLFO->setAttribute(LfoModulator::SmoothingTime, 0.0f, dontSendNotification);
		seqLFO->setAttribute(LfoModulator::SmoothingTime, 0.0f, dontSendNotification);

		squareLFO->setAttribute(LfoModulator::WaveFormType, LfoModulator::Waveform::Square, dontSendNotification);
		seqLFO->setAttribute(LfoModulator::WaveFormType, LfoModulator::Waveform::Steps, dontSendNotification);

		squareLFO->setAttribute(LfoModulator::Parameters::FadeIn, 0.0f, dontSendNotification);
		seqLFO->setAttribute(LfoModulator::Parameters::FadeIn, 0.0f, dontSendNotification);

		seqLFO->getSliderPack(0)->setNumSliders(2);
		seqLFO->getSliderPack(0)->setValue(1, 0.0f, dontSendNotification);

		// Process
		squareLFO->setBypassed(true);
		seqLFO->setBypassed(false);

		auto seqData = Helpers::createTestDataWithOneSecondNote(56);
		Helpers::process(bp, seqData, 512);

		squareLFO->setBypassed(false);
		seqLFO->setBypassed(true);

		auto squareData = Helpers::createTestDataWithOneSecondNote(56);
		Helpers::process(bp, squareData, 512);
		// Tests

		expectResult(seqData.matches(squareData, this, -3.0f), "Step doesn't match Square");

		bp = nullptr;
	}

	void expectResult(Result r, String errorMessage)
	{
		expect(r.wasOk(), errorMessage + " - " + r.getErrorMessage());
	}

	void beginTestWithOptionalGroup(const String& testName, bool useGroup)
	{
		beginTest(testName + String(useGroup ? " in group" : ""));
	}

	struct Helpers
	{
		class DiracIterator
		{
		public:

			DiracIterator(AudioSampleBuffer& b, int channel, bool strictMode_):
				ptr(b.getReadPointer(channel)),
				strictMode(strictMode_),
				numSamples(b.getNumSamples())
			{
				
			}

			Result scan()
			{
				int lastDiracIndex = -1;
				int currentIndex = 0;

				while (--numSamples >= 0)
				{
					const float value = *ptr++;

					const bool isOne = strictMode ? (value == 1.0f) : (value > 0.01f);
					const bool isMinusOne = strictMode ? (value == -11.0f) : (value < -0.01f);

					if (isMinusOne)
					{
						if (lastDiracIndex != -1)
						{
							return Result::fail("Negative dirac at non-start");
						}

						data.firstIsNegative = true;

						if(currentIndex != 0)
							data.sampleDistances.add(currentIndex);

						lastDiracIndex = currentIndex;
					}
					else if (isOne)
					{
						data.sampleDistances.add(currentIndex - lastDiracIndex);
						lastDiracIndex = currentIndex;
					}
					else if (strictMode)
					{
						return Result::fail("Intermediate value found: " + String(value, 3) + " at " + String(currentIndex));
					}

					currentIndex++;
				}

				return Result::ok();
			}

			struct Data
			{
				Result matchesStartAndEnd(int start, int end) const
				{
					if (sampleDistances.getFirst() != start)
						return Result::fail("Start doesn't match. Expected: " + String(start) + ", actual: " + String(sampleDistances.getFirst()));

					if (sampleDistances.getLast() != end)
						return Result::fail("End doesn't match. Expected: " + String(end) + ", actual: " + String(sampleDistances.getLast()));

					return Result::ok();
				};

				Result isWithinRange(Range<int> r) const
				{
					for (int i = 0; i < sampleDistances.size(); i++)
					{
						auto v = sampleDistances[i];

						if (v < r.getStart() || v > r.getEnd())
						{
							return Result::fail("Outside of range: " + String(v) + " at Sample index " + String(i));
						}
					}

					return Result::ok();
				}

				Array<int> sampleDistances;
				bool firstIsNegative = false;
			};

			const Data& getData() const
			{
				return data;
			}

			void dump(UnitTest* test)
			{
				String d;

				d << "Dirac dump:\n";
				
				int row = 0;

				for (const auto& da : data.sampleDistances)
				{
					d << da << ", ";

					row++;

					if (row >= 8)
					{
						d << "\n";
						row = 0;
					}

				}
				
				test->logMessage(d);
			}

		private:

			const float* ptr;
			bool strictMode;
			int numSamples;

			Data data;

		};

		static void copyToClipboard(BackendProcessor* bp)
		{
            jassertfalse;
			//BackendCommandTarget::Actions::exportFileAsSnippet(bp);
		}

		template <class ProcessorType> static ProcessorType* get(BackendProcessor* bp)
		{
			return ProcessorHelpers::getFirstProcessorWithType<ProcessorType>(bp->getMainSynthChain());
		}

		template <class ProcessorType> static void reset(BackendProcessor* bp)
		{
			auto p = get<ProcessorType>(bp);

			auto numParameters = p->getNumParameters();

			for (int i = 0; i < numParameters; i++)
			{
				p->setAttribute(i, p->getDefaultValue(i), dontSendNotification);
			}

		}

		template <class ProcessorType> static ProcessorType* addVoiceModulatorToOptionalGroup(BackendProcessor* bp, int chainToInsert)
		{
			bool isGroup = ProcessorHelpers::is<ModulatorSynthGroup>(bp->getMainSynthChain()->getHandler()->getProcessor(0));

			if (isGroup)
				return addVoiceModulator<ModulatorSynthGroup, ProcessorType>(bp, chainToInsert);
			else
				return addVoiceModulator<NoiseSynth, ProcessorType>(bp, chainToInsert);
		}

		static void setLfoToDefaultSquare(LfoModulator* lfo)
		{
			lfo->setAttribute(LfoModulator::TempoSync, true, dontSendNotification);
			lfo->setAttribute(LfoModulator::WaveFormType, LfoModulator::Square, dontSendNotification);
			lfo->setAttribute(LfoModulator::Frequency, TempoSyncer::Eighth, dontSendNotification);
			lfo->setAttribute(LfoModulator::FadeIn, 0.0f, dontSendNotification);
			lfo->setAttribute(LfoModulator::SmoothingTime, 0.0f, dontSendNotification);
		}

		template <class ParentType, class ProcessorType> static ProcessorType* addVoiceModulator(BackendProcessor* bp, int chainToInsert)
		{
			auto modChain = dynamic_cast<ModulatorChain*>(get<ParentType>(bp)->getChildProcessor(chainToInsert));

			Random r;
			auto id = String(r.nextInt());

			auto newMod = new ProcessorType(bp, id, NUM_POLYPHONIC_VOICES, modChain->getMode());

			modChain->getHandler()->add(newMod, nullptr);

			return newMod;
		}

		static ModulatorSynth* getMainSynth(BackendProcessor* bp, bool useGroup)
		{
			if (useGroup)
			{
				return dynamic_cast<ModulatorSynth*>(ProcessorHelpers::getFirstProcessorWithType<ModulatorSynthGroup>(bp->getMainSynthChain()));
			}
			else
			{
				return dynamic_cast<ModulatorSynth*>(ProcessorHelpers::getFirstProcessorWithType<NoiseSynth>(bp->getMainSynthChain()));
			}
		}

		static void addGlobalContainer(BackendProcessor* bp, bool useGroup)
		{
			auto mainSynth = Helpers::getMainSynth(bp, useGroup);

			auto container = new GlobalModulatorContainer(bp, "Container", NUM_POLYPHONIC_VOICES);

			bp->getMainSynthChain()->getHandler()->add(container, mainSynth);
		}

		template <class ProcessorType> static ProcessorType* addTimeModulatorToOptionalGroup(BackendProcessor* bp, int chainToInsert)
		{
			bool isGroup = ProcessorHelpers::is<ModulatorSynthGroup>(bp->getMainSynthChain()->getHandler()->getProcessor(0));

			if (isGroup)
				return addTimeModulator<ModulatorSynthGroup, ProcessorType>(bp, chainToInsert);
			else
				return addTimeModulator<NoiseSynth, ProcessorType>(bp, chainToInsert);
		}

		template <class ParentType, class ProcessorType> static ProcessorType* addTimeModulator(BackendProcessor* bp, int chainToInsert)
		{
			auto modChain = dynamic_cast<ModulatorChain*>(get<ParentType>(bp)->getChildProcessor(chainToInsert));

			Random r;
			auto id = String(r.nextInt());

			auto newMod = new ProcessorType(bp, id, modChain->getMode());

			modChain->getHandler()->add(newMod, nullptr);

			return newMod;
		}

		template <class ProcessorType> static ProcessorType* addVoiceEffectToOptionalGroup(BackendProcessor* bp)
		{
			bool isGroup = ProcessorHelpers::is<ModulatorSynthGroup>(bp->getMainSynthChain()->getHandler()->getProcessor(0));

			if (isGroup)
				return addVoiceEffect<ModulatorSynthGroup, ProcessorType>(bp);
			else
				return addVoiceEffect<NoiseSynth, ProcessorType>(bp);
		}

		template <class ParentType, class ProcessorType> static ProcessorType* addVoiceEffect(BackendProcessor* bp)
		{
			auto fxChain = dynamic_cast<EffectProcessorChain*>(get<ParentType>(bp)->getChildProcessor(ModulatorSynth::EffectChain));

			Random r;
			auto id = String(r.nextInt());

			auto newEffect = new ProcessorType(bp, id, NUM_POLYPHONIC_VOICES);
	
			fxChain->getHandler()->add(newEffect, nullptr);

			return newEffect;
		}

		template <class ProcessorType> static void setAttribute(BackendProcessor* bp, int index, float value)
		{
			auto p = get<ProcessorType>(bp);

			p->setAttribute(index, value, dontSendNotification);
		}


		struct TestData
		{
			float getSample(int sampleIndex) const
			{
				return audioBuffer.getSample(0, sampleIndex);
			}

			float getSample(int channelIndex, int sampleIndex) const
			{
				return audioBuffer.getSample(channelIndex, sampleIndex);
			}

			bool operator==(const TestData& other) const
			{
				if (audioBuffer.getNumSamples() != other.audioBuffer.getNumSamples())
					return false;

				int size = audioBuffer.getNumSamples();

				float error = 0.0f;

				for (int i = 0; i < size; i++)
				{
					error = jmax(error, fabsf(getSample(0, i) - other.getSample(0, i)));
					error = jmax(error, fabsf(getSample(1, i) - other.getSample(1, i)));
				}

				const float errorDb = Decibels::gainToDecibels(error);

				if (errorDb > -80.0f)
					return false;

				return true;

			}

			Result matches(const TestData& otherData, UnitTest* test, float errorLevelDecibels)
			{
				if (audioBuffer.getNumSamples() != otherData.audioBuffer.getNumSamples())
					return Result::fail("Size mismatch");

				int size = audioBuffer.getNumSamples();

				float maxError = -100.0f;

				for (int i = 0; i < size; i++)
				{
					auto otherL = otherData.getSample(0, i);
					auto otherR = otherData.getSample(1, i);
					
					maxError = jmax<float>(maxError, getError(getSample(0, i), otherL));
					maxError = jmax<float>(maxError, getError(getSample(1, i), otherR));

					auto rl = isWithinErrorRange(i, otherL, 0, errorLevelDecibels);
					auto rr = isWithinErrorRange(i, otherR, 1, errorLevelDecibels);
					
					if (rl.failed())
					{
						test->logMessage("Error at sample " + String(i));

						dump(audioBuffer, "dumpExpected.wav");
						dump(otherData.audioBuffer, "dumpActual.wav");
						return rl;
					}
						

					if (rr.failed())
					{
						test->logMessage("Error at sample " + String(i));

						dump(audioBuffer, "dumpExpected.wav");
						dump(otherData.audioBuffer, "dumpActual.wav");
						return rr;
					}
						
				}

				test->logMessage("Max error: " + String(maxError, 1) + " dB");

				return Result::ok();
			}

			static float getError(float firstSample, float secondSample)
			{
				auto diff = fabsf(firstSample - secondSample);

				auto error = Decibels::gainToDecibels(diff);

				return error;
			}

			Result isWithinErrorRange(int sampleIndex, float expected, int channelIndex = 0, float errorInDecibels=-60.0f) const
			{
				auto actual = audioBuffer.getSample(channelIndex, sampleIndex);

				auto error = getError(expected, actual);

				if (error > errorInDecibels)
				{
					return Result::fail("Error: " + String(error, 1) + " dB");
				}
				
				return Result::ok();
				
			}

			MidiBuffer midiBuffer;
			AudioSampleBuffer audioBuffer;
		};

		static BackendProcessor* createWithOptionalGroup(NoiseSynth::TestSignal signalType, bool useGroup)
		{
			if (useGroup)
				return createAndInitialiseProcessorWithGroup(signalType);
			else
				return createAndInitialiseProcessor(signalType);
		}


		static BackendProcessor* createAndInitialiseProcessorWithGroup(NoiseSynth::TestSignal signalType)
		{
			ScopedPointer<BackendProcessor> bp = new BackendProcessor(nullptr, nullptr);
			ScopedPointer<ModulatorSynthGroup> gr = new ModulatorSynthGroup(bp, "Group", NUM_POLYPHONIC_VOICES);
			ScopedPointer<NoiseSynth> noiseSynth = new NoiseSynth(bp, "TestProcessor", NUM_POLYPHONIC_VOICES);

			noiseSynth->setAttribute(ModulatorSynth::Parameters::Gain, 1.0f, dontSendNotification);
			noiseSynth->setTestSignal(signalType);

			gr->getHandler()->add(noiseSynth.release(), nullptr);
			gr->addProcessorsWhenEmpty();
			gr->setAttribute(ModulatorSynth::Parameters::Gain, 1.0f, dontSendNotification);

			bp->getMainSynthChain()->getHandler()->add(gr.release(), nullptr);

			return bp.release();
		}

		static BackendProcessor* createAndInitialiseProcessor(NoiseSynth::TestSignal signalType)
		{
			ScopedPointer<BackendProcessor> bp = new BackendProcessor(nullptr, nullptr);

			ScopedPointer<NoiseSynth> noiseSynth = new NoiseSynth(bp, "TestProcessor", NUM_POLYPHONIC_VOICES);

			noiseSynth->addProcessorsWhenEmpty();
			noiseSynth->setAttribute(ModulatorSynth::Parameters::Gain, 1.0f, dontSendNotification);
			noiseSynth->setTestSignal(signalType);

			bp->getMainSynthChain()->getHandler()->add(noiseSynth.release(), nullptr);


			return bp.release();
		}

		static TestData createTestDataWithOneSecondNote(int startOffset = 0, int stopOffset=-1)
		{
			TestData d;

			d.audioBuffer.setSize(2, 44100 * 2);
			d.audioBuffer.clear();
			d.midiBuffer.addEvent(MidiMessage::noteOn(1, 64, 1.0f), startOffset);

			if (stopOffset == -1)
				stopOffset = roundToInt(44100*0.7);

			d.midiBuffer.addEvent(MidiMessage::noteOff(1, 64), stopOffset);

			return d;
		}

		static void process(BackendProcessor* bp, TestData& data, int blockSize, int numToProcess=-1)
		{
			bp->prepareToPlay((double)44100, blockSize);

			resumeProcessing(bp, data, blockSize, numToProcess, 0);
		}

		static void resumeProcessing(BackendProcessor* bp, TestData& data, int blockSize, int numToProcess, int sampleOffset)
		{
			int numSamplesTotal = numToProcess > 0 ? numToProcess : data.audioBuffer.getNumSamples();

			numSamplesTotal = jmin<int>(numSamplesTotal, data.audioBuffer.getNumSamples()-sampleOffset);

			int offset = sampleOffset;

			while (numSamplesTotal > 0)
			{
				int numThisTime = jmin<int>(numSamplesTotal, blockSize);

				auto l = data.audioBuffer.getWritePointer(0, offset);
				auto r = data.audioBuffer.getWritePointer(1, offset);

				float* d[2] = { l, r };

				AudioSampleBuffer subAudio(d, 2, numThisTime);
				MidiBuffer subMidi;
				subMidi.addEvents(data.midiBuffer, offset, numThisTime, -offset);

				bp->processBlock(subAudio, subMidi);
				numSamplesTotal -= numThisTime;
				offset += numThisTime;
			}
		}


		static void dump(const AudioSampleBuffer& b, const String& fileName)
		{
			hlac::CompressionHelpers::dump(b, fileName);
		}

	};

};


//static ModulationTests modulationTests;

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

		testBlockDivider<32>(64, 32, 1);
		testBlockDivider<32>(96, 0, 0);
		testBlockDivider<32>(256, 4, 32);
		testBlockDivider<16>(17, 4, 1);
		testBlockDivider<64>(64, 4, 1);
		testBlockDivider<16>(53, 12, 1);
		testBlockDivider<32>(512, 13, 8);

		testBlockDividedRamping<32>(512, 0, 0);
		testBlockDividedRamping<32>(512, 4, 32);
		testBlockDividedRamping<32>(512, 17, 8);
		testBlockDividedRamping<32>(256, 18, 8);
		testBlockDividedRamping<32>(74, 16, 1);

	}

	

private:

	template <int RampLength> void testBlockDividedRamping(int averageBlockSize, int maxVariationFactor, int variationSize)
	{
		beginTest("Testing Rampers with length " + String(RampLength) + ", average block size " + String(averageBlockSize) + " and variation " + String(maxVariationFactor) + "*" + String(variationSize));

		Random r;

		const int totalLength = averageBlockSize * r.nextInt({ 8, 20 });

		int numSamples = totalLength;

		ModulatorChain::ModChainWithBuffer::Buffer b;
		b.setMaxSize(totalLength);

		auto data = b.scratchBuffer;

		BlockDivider<RampLength> blockDivider;

		float value = 2.0f;
		float delta = 1.0f / (float)numSamples;

		int numProcessed = 0;

		BlockDividerStatistics::resetStatistics();

		while (numSamples > 0)
		{
			int thisBlockSize = averageBlockSize;

			if (maxVariationFactor > 0)
			{
				thisBlockSize -= r.nextInt({ 0, maxVariationFactor }) *variationSize;
			}

			thisBlockSize = jlimit<int>(0, numSamples, thisBlockSize);

			const int numThisBlockConst = thisBlockSize;

			// Reset it here to get rid of rounding errors...
			value = 2.0f + numProcessed / (float)totalLength;

			while (thisBlockSize > 0)
			{
				bool newBlock;
				int blockSize = blockDivider.cutBlock(thisBlockSize, newBlock, data);

				if (blockSize == 0)
				{
					AlignedSSERamper<RampLength> ramper(data);
					ramper.ramp(value, delta);
					value += delta * (float)RampLength;
					data += RampLength;
					numProcessed += RampLength;
				}
				else
				{
					FallbackRamper ramper(data, blockSize);
					value = ramper.ramp(value, delta);
					data += blockSize;
					numProcessed += blockSize;
				}
			}

			numSamples -= numThisBlockConst;
			
		}

		expectEquals<int>(numProcessed, totalLength, "NumProcessed");

		int alignedCallPercentage = BlockDividerStatistics::getAlignedCallPercentage();

		if (averageBlockSize % RampLength == 0 && (variationSize % RampLength == 0))
		{
			expect(alignedCallPercentage == 100, "All calls must be aligned");
		}
		else
		{
			logMessage("Aligned call percentage: " + String(alignedCallPercentage) + "%");
		}


		auto check = b.scratchBuffer;

		for (int i = 0; i < totalLength; i++)
		{
			float expected = 2.0f + (float)i / (float)totalLength;
			float thisValue = check[i];

			float delta2 = Decibels::gainToDecibels(fabsf(expected - thisValue));

			expect(delta2 < -96.0f, "Value at " + String(i) + "Expected" + String(expected) + ", Actual: " + String(thisValue));
		};

	}

	template <int RampLength> void testBlockDivider(int averageBlockSize, int maxVariationFactor, int variationSize)
	{
		beginTest("Testing Block Divider with average block size " + String(averageBlockSize) + " and variation " + String(maxVariationFactor) + "*" + String(variationSize));
		
		try
		{
			using SSEType = dsp::SIMDRegister<float>;

			Random r;

			int numBlocks = r.nextInt({ 8, 32 });

			int totalLength = averageBlockSize * numBlocks;

			ModulatorChain::ModChainWithBuffer::Buffer totalData;
			totalData.setMaxSize(totalLength);

			ModulatorChain::ModChainWithBuffer::Buffer blockData;
			blockData.setMaxSize(averageBlockSize);

			auto data = blockData.scratchBuffer;

			FloatVectorOperations::fill(data, 1.0f, averageBlockSize);

			auto startData = totalData.scratchBuffer;
			int startLength = totalLength;

			FloatVectorOperations::fill(startData, 0.5f, totalLength);

			int counter = 0;

			BlockDivider<RampLength> divider;

			int numProcessed = 0;
			int blockOffset = 0;

			

			BlockDividerStatistics::resetStatistics();

			while (totalLength > 0)
			{
				int thisBlockSize = averageBlockSize;

				if (maxVariationFactor > 0)
					thisBlockSize = averageBlockSize - r.nextInt({ 0, maxVariationFactor }) * variationSize;
				else
					thisBlockSize = averageBlockSize;

				thisBlockSize = jmin<int>(thisBlockSize, totalLength);

				data = blockData.scratchBuffer;
				FloatVectorOperations::fill(data, 0.5, thisBlockSize);

				counter = thisBlockSize;

				while (counter > 0)
				{
					bool newBlock;
					int subBlockSize = divider.cutBlock(counter, newBlock, data);

					if (subBlockSize == 0)
					{
						expect(SSEType::isSIMDAligned(data), "Pointer alignment");

						FloatVectorOperations::fill(data, 0.0f, RampLength);
						data[0] = 1.0f;

						numProcessed += RampLength;

						data += RampLength;
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

				totalLength -= thisBlockSize;
				FloatVectorOperations::copy(startData + blockOffset, blockData.scratchBuffer, thisBlockSize);
				blockOffset += thisBlockSize;
			}

			expectEquals<int>(numProcessed, startLength, "NumProcessed");

			

			for (int i = 0; i < startLength; i++)
			{
				const float expected = i % RampLength == 0 ? 1.0f : 0.0f;

				expectEquals<float>(startData[i], expected, "Position: " + String(i));
			}

			int alignedCallPercentage = BlockDividerStatistics::getAlignedCallPercentage();

			if (averageBlockSize % RampLength == 0 && (variationSize % RampLength == 0))
			{
				expect(alignedCallPercentage == 100, "All calls must be aligned");
			}
			else
			{
				logMessage("Aligned call percentage: " + String(alignedCallPercentage) + "%");
			}


			blockData.clear();
			totalData.clear();
		}
		catch (String& m)
		{
			ignoreUnused(m);
			DBG(m);
			jassertfalse;
		}

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
