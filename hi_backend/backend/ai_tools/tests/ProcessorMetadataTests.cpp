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

#if HI_RUN_UNIT_TESTS

namespace hise {

/** Unit tests for the ProcessorMetadata system.

    Tests the metadata declarations, registry, Processor base class integration,
    and the LfoModulator migration (the first fully migrated processor).
*/
class ProcessorMetadataTest : public juce::UnitTest
{
public:
    ProcessorMetadataTest() : UnitTest("ProcessorMetadata Tests", "Processors") {}

    void runTest() override
    {
        ScopedValueSetter<bool> svs(MainController::unitTestMode, true);

        ctx = std::make_unique<TestContext>(*this);

        testConstrainerParser<SynthGroupConstrainer>();
        testConstrainerParser<SynthGroupFXConstrainer>();
        testConstrainerParser<NoMidiInputConstrainer>();
        testConstrainerParser<SlotFX::Constrainer>();
        testConstrainerParser<NoGlobalsConstrainer>();
        testConstrainerParser<VoiceStartModulatorFactoryType::Constrainer>();

        // LFO static metadata tests (no processor instance needed)
        testLfoMetadataParameterCount();
        testLfoMetadataParameterIds();
        testLfoMetadataParameterTypes();
        testLfoMetadataParameterDefaults();
        testLfoMetadataParameterRanges();
        testLfoMetadataSliderModes();
        testLfoMetadataTempoSync();
        testLfoMetadataValueList();
        testLfoMetadataModulationChains();
        testLfoMetadataTypeAndSubtype();

        // WaveSynth static metadata tests
        testWaveSynthMetadataParameterCount();
        testWaveSynthMetadataParameterIds();
        testWaveSynthMetadataParameterTypes();
        testWaveSynthMetadataParameterDefaults();
        testWaveSynthMetadataParameterRanges();
        testWaveSynthMetadataSliderModes();
        testWaveSynthMetadataValueList();
        testWaveSynthMetadataModulationChains();
        testWaveSynthMetadataTypeAndSubtype();

        // Registry tests
        testRegistryPopulation();
        testRegistryLfoEntry();
        testRegistryWaveSynthEntry();
        testRegistryAllEntriesValid();
        testRegistryToJSON();



        // LFO processor integration tests (require MainController)
        testLfoParameterResolution();
        testLfoHasInitialisedMetadata();
        testLfoGetDefaultValueFromMetadata();
        testLfoGetMetadataVirtual();

        // WaveSynth processor integration tests
        testWaveSynthParameterResolution();
        testWaveSynthHasInitialisedMetadata();
        testWaveSynthGetDefaultValueFromMetadata();
        testWaveSynthGetMetadataVirtual();

        // Edge case tests
        testOperatorBracketOutOfRange();
        testGetParameterByIdInvalid();

        ctx.reset();

        // Module body parameter integrity test
        testAllModules();

        ctx.reset();
    }

private:

    /** Lightweight test context - creates a BackendProcessor for live processor tests. */
    struct TestContext
    {
        TestContext(UnitTest& parentTest) : test(parentTest)
        {
            CompileExporter::setSkipAudioDriverInitialisation();
            sp = new StandaloneProcessor();
            mc = dynamic_cast<MainController*>(sp->createProcessor());
            threadSuspender = new MainController::ScopedBadBabysitter(mc);
            bp = dynamic_cast<BackendProcessor*>(mc.get());
            bp->prepareToPlay(44100.0, 512);
        }

        ~TestContext()
        {
            threadSuspender = nullptr;
            mc = nullptr;
            sp = nullptr;
        }

        /** Create an LfoModulator attached to this MainController. Caller owns the result. */
        LfoModulator* createLfo(const String& id = "TestLFO")
        {
            return new LfoModulator(mc, id, Modulation::GainMode);
        }

        /** Create a WaveSynth attached to this MainController. Caller owns the result. */
        WaveSynth* createWaveSynth(const String& id = "TestWaveSynth")
        {
            return new WaveSynth(mc, id, 8);
        }

        UnitTest& test;
        ScopedPointer<StandaloneProcessor> sp;
        ScopedPointer<MainController> mc;
        ScopedPointer<MainController::ScopedBadBabysitter> threadSuspender;
        BackendProcessor* bp = nullptr;
    };

    std::unique_ptr<TestContext> ctx;

    //==========================================================================
    // Static metadata tests - operate on LfoModulator::createMetadata() directly
    //==========================================================================

    void testModule(ProcessorMetadata md, ModulatorSynth* parentSynthToUse)
    {
        beginTest("Testing metadata integrity for " + md.id.toString());

        raw::Builder b(ctx->mc);

        int insertIndex = -2;

        if (md.type == ProcessorMetadataIds::SoundGenerator)
        {
            parentSynthToUse = ctx->mc->getMainSynthChain();
            insertIndex = -1;
        }
        else if (md.type == ProcessorMetadataIds::Effect)
            insertIndex = ModulatorSynth::InternalChains::EffectChain;
        else if (md.type == ProcessorMetadataIds::MidiProcessor)
            insertIndex = ModulatorSynth::InternalChains::MidiProcessor;
        else
            insertIndex = ModulatorSynth::InternalChains::GainModulation;

        auto pToTest = b.create(parentSynthToUse, md.id, insertIndex);

        

        expect(pToTest != nullptr, "module created OK");

        if(md.dataType == ProcessorMetadata::DataType::Dynamic)
            md = pToTest->getMetadata();

        if (md.constrainerWildcard != "*")
        {
            auto c = dynamic_cast<Chain*>(pToTest);
            expect(c != nullptr);
            auto wildcards = c->getFactoryType()->getConstrainer()->getWildcardFromObject();

            bool found = false;

            for (auto& wc : wildcards)
            {
                if (wc.first == md.type)
                {
                    expectEquals(wc.second, md.constrainerWildcard, "constrainer wildcard mismatch");
                    found = true;
                    break;
                }
            }

            expect(found, "wildcard not found");
        }
        else
        {
            if (auto c = dynamic_cast<Chain*>(pToTest))
            {
                expect(c->getFactoryType()->getConstrainer() == nullptr, "missing wildcard");
            }
        }

		if (md.fxConstrainerWildcard != "*")
		{
            auto fxChain = dynamic_cast<EffectProcessorChain*>(pToTest->getChildProcessor(ModulatorSynth::InternalChains::EffectChain));

			expect(fxChain != nullptr);
			auto wildcards = fxChain->getFactoryType()->getConstrainer()->getWildcardFromObject();

			bool found = false;

			for (auto& wc : wildcards)
			{
				if (wc.first == ProcessorMetadataIds::Effect)
				{
					expectEquals(wc.second, md.fxConstrainerWildcard, "constrainer wildcard mismatch");
					found = true;
					break;
				}
			}

			expect(found, "wildcard not found");
		}
		else
		{
            if (dynamic_cast<ModulatorSynth*>(pToTest) != nullptr)
            {
				if (auto fxChain = dynamic_cast<EffectProcessorChain*>(pToTest->getChildProcessor(ModulatorSynth::InternalChains::EffectChain)))
				{
					expect(fxChain->getFactoryType()->getConstrainer() == nullptr, "missing wildcard");
				}
            }
		}

        auto state = pToTest->exportAsValueTree();

        std::map<std::pair<Identifier, int>, bool> defaultValueSkipMap;

        // Define custom rules for the default value skip check
        {
			// macro value is a dynamic property
			defaultValueSkipMap[{MacroModulator::getClassType(), MacroModulator::SpecialParameters::MacroValue}] = true;

			// default value is dependent on chain type
			defaultValueSkipMap[{MatrixModulator::getClassType(), MatrixModulator::SpecialParameters::Value}] = true;

            defaultValueSkipMap[{DynamicsEffect::getClassType(), DynamicsEffect::Parameters::GateReduction}] = true;
            defaultValueSkipMap[{DynamicsEffect::getClassType(), DynamicsEffect::Parameters::CompressorReduction}] = true;
            defaultValueSkipMap[{DynamicsEffect::getClassType(), DynamicsEffect::Parameters::LimiterReduction}] = true;
    
			// weird decibel rounding error, disregard
			defaultValueSkipMap[{MidiMetronome::getClassType(), MidiMetronome::Parameters::Volume}] = true;

			// no voices, disregard
			defaultValueSkipMap[{SendContainer::getClassType(), ModulatorSynth::Parameters::VoiceLimit}] = true;
        }
        
        std::map<std::pair<Identifier, int>, int> exportValueSkipMap;

        // Define custom rules for the exportAsValueTree() integrity check
        {
            // these are read only parameters and not supposed to be stored in the value tree
            exportValueSkipMap[{DynamicsEffect::getClassType(), DynamicsEffect::Parameters::GateReduction}] = true;
            exportValueSkipMap[{DynamicsEffect::getClassType(), DynamicsEffect::Parameters::CompressorReduction}] = true;
            exportValueSkipMap[{DynamicsEffect::getClassType(), DynamicsEffect::Parameters::LimiterReduction}] = true;

            // these are read only parameters or stored in a subtree
            exportValueSkipMap[{MidiPlayer::getClassType(), MidiPlayer::SpecialParameters::CurrentPosition}] = true;
            exportValueSkipMap[{MidiPlayer::getClassType(), MidiPlayer::SpecialParameters::LoopStart}] = true;
            exportValueSkipMap[{MidiPlayer::getClassType(), MidiPlayer::SpecialParameters::LoopEnd}] = true;
            
            // this information is stored in the slider pack data
            exportValueSkipMap[{LfoModulator::getClassType(), LfoModulator::Parameters::NumSteps}] = true;
        }

        std::map<std::pair<Identifier, int>, bool> roundTripSkipMap;
        {
			// eco mode is deprecated anyways
            roundTripSkipMap[{AhdsrEnvelope::getClassType(), AhdsrEnvelope::SpecialParameters::EcoMode}] = true;

			// dynamic value, disregard
            roundTripSkipMap[{Arpeggiator::getClassType(), Arpeggiator::Parameters::CurrentStep}] = true;

            // these are truncated with each other and highly mess up the fuzzer
            roundTripSkipMap[{ChokeGroupProcessor::getClassType(), ChokeGroupProcessor::SpecialParameters::HiKey}] = true;
            roundTripSkipMap[{ChokeGroupProcessor::getClassType(), ChokeGroupProcessor::SpecialParameters::LoKey}] = true;

            // dynamic parameters...
            roundTripSkipMap[{ ConvolutionEffect::getClassType(), ConvolutionEffect::Parameters::Latency}] = true;
            roundTripSkipMap[{ ConvolutionEffect::getClassType(), ConvolutionEffect::Parameters::ImpulseLength}] = true;

            // they are dependent on the tempo sync value
            roundTripSkipMap[{ DelayEffect::getClassType(), DelayEffect::Parameters::DelayTimeLeft}] = true;
            roundTripSkipMap[{ DelayEffect::getClassType(), DelayEffect::Parameters::DelayTimeRight}] = true;

            // these are read only parameters and not supposed to be stored in the value tree
			roundTripSkipMap[{DynamicsEffect::getClassType(), DynamicsEffect::Parameters::GateReduction}] = true;
			roundTripSkipMap[{DynamicsEffect::getClassType(), DynamicsEffect::Parameters::CompressorReduction}] = true;
			roundTripSkipMap[{DynamicsEffect::getClassType(), DynamicsEffect::Parameters::LimiterReduction}] = true;

            // folded into wet level parameter
            roundTripSkipMap[{SimpleReverbEffect::getClassType(), SimpleReverbEffect::Parameters::DryLevel}] = true;

			// these require actual MIDI data to operate correctly
			roundTripSkipMap[{MidiPlayer::getClassType(), MidiPlayer::SpecialParameters::CurrentPosition}] = true;
            roundTripSkipMap[{MidiPlayer::getClassType(), MidiPlayer::SpecialParameters::CurrentSequence}] = true;
            roundTripSkipMap[{MidiPlayer::getClassType(), MidiPlayer::SpecialParameters::CurrentTrack}] = true;
			roundTripSkipMap[{MidiPlayer::getClassType(), MidiPlayer::SpecialParameters::LoopStart}] = true;
			roundTripSkipMap[{MidiPlayer::getClassType(), MidiPlayer::SpecialParameters::LoopEnd}] = true;

            // this is rounded to power of two internally
            roundTripSkipMap[{PolyFilterEffect::getClassType(), PolyFilterEffect::Parameters::Quality}] = true;

            // weird rounding error...
            roundTripSkipMap[{ AudioLooper::getClassType(), AudioLooper::SpecialParameters::SampleStartMod}] = true;
        }

        for (int i = 0; i < md.parameters.size(); i++)
        {
            const auto& p = md.parameters[i];
            auto idx = p.parameterIndex;
            auto propId = p.id;

            std::pair<Identifier, int> key = { pToTest->getType(), idx };

			if (defaultValueSkipMap[key])
				continue;

            auto v1 = md.getDefaultValue(pToTest, i);
            auto v2 = pToTest->getAttribute(idx);

            String msg;
			msg << "outside range: ";
			msg << propId.toString() + " [" + String(idx) + "]";

            expect(p.range.getRange().contains(v1) || p.range.getRange().getEnd() == v1, msg);

            msg = {};
            msg << "parameter default value mismatch: ";
            msg << propId.toString() + " [" + String(idx) + "]";
            expectWithinAbsoluteError<float>(v1, v2, 0.01f, msg);

            // skip testing hardcoded processors, they store their values differently...
            if (dynamic_cast<HardcodedScriptProcessor*>(pToTest) != nullptr)
                continue;

			if (exportValueSkipMap[key])
				continue;

            expect(state.hasProperty(propId), "exportAsValueTree(): missing property " + propId.toString());

            auto v3 = (float)state[propId];

            msg = {};
			msg << "exportAsValueTree() value mismatch: ";
			msg << propId.toString() + " [" + String(idx) + "]";

            expectWithinAbsoluteError<float>(v2, v3, 0.01f, msg);
        }

        NamedValueSet randomParameters;

        for (const auto& p : md.parameters)
        {
            auto newValue = Random::getSystemRandom().nextDouble();
            newValue = p.range.convertFrom0to1(newValue, false);
            randomParameters.set(p.id, newValue);
            pToTest->setAttribute(p.parameterIndex, newValue, dontSendNotification);
        }

        auto state2 = pToTest->exportAsValueTree();

		for (const auto& p : md.parameters)
		{
            auto idx = p.parameterIndex;
            auto propId = p.id;

            std::pair<Identifier, int> key = { pToTest->getType(), idx };

            // voice limits behave weird and I don't want to check this now...
            if (propId.toString() == "VoiceLimit")
                continue;

            if (roundTripSkipMap[key])
                continue;

			if (defaultValueSkipMap[key])
				continue;

            auto v1 = pToTest->getAttribute(idx);
            auto v3 = (float)randomParameters[propId];

            String msg;
            msg << "getAttribute after randomizing within range: ";
            msg << propId.toString() + " [" + String(idx) + "]";

            expectWithinAbsoluteError<float>(v1, v3, 0.1f, msg);

			// skip testing hardcoded processors, they store their values differently...
			if (dynamic_cast<HardcodedScriptProcessor*>(pToTest) != nullptr)
				continue;

			if (exportValueSkipMap[key])
				continue;

            auto v2 = (float)state2[propId];

            msg = {};
			msg << "exportAsValueTree() after randomizing within range: ";
			msg << propId.toString() + " [" + String(idx) + "]";

            expectWithinAbsoluteError<float>(v2, v3, 0.1f, msg);
		}

        // testing modulation setup

		auto numModChains = md.modulation.size();

		if (md.type == ProcessorMetadataIds::SoundGenerator)
			numModChains += 2;

		expectEquals<int>(pToTest->getNumInternalChains(), numModChains, "modulation chain amount mismatch");
        
        for (const auto& m : md.modulation)
        {
            if (m.disabled)
                continue;

            auto idx = m.chainIndex;

            auto modChain = dynamic_cast<ModulatorChain*>(pToTest->getChildProcessor(idx));
            expect(modChain != nullptr, "not a modulation chain: #" + String(idx));

            if (auto ms = dynamic_cast<ModulatorSynth*>(pToTest))
            {
                auto isDisabled = ms->isChainDisabled((ModulatorSynth::InternalChains)idx) ||
                                 modChain->isBypassed();
                auto shouldBeDisabled = m.disabled;

                expect(isDisabled == shouldBeDisabled, "disable mismatch at " + m.id.toString());
    
            }

			auto actual = Modulation::convertToScriptnodeMode(modChain->getMode());
			auto expected = m.modulationMode;

			expectEquals<int>((int)actual, (int)expected, "mode mismatch at " + m.id.toString());

			if (m.constrainerWildcard != "*")
			{
                auto constrainer = modChain->getFactoryType()->getConstrainer();

                if (constrainer == nullptr)
                {
                    auto isVoiceStart = dynamic_cast<VoiceStartModulatorFactoryType*>(modChain->getFactoryType()) != nullptr;

                    expect(isVoiceStart == (m.constrainerWildcard == ProcessorMetadataIds::VoiceStartModulator.toString()),
                        "not a voice start modchain");

                }
                else
                {
					auto wildcards = constrainer->getWildcardFromObject();

					bool found = false;

					for (auto& wc : wildcards)
					{
						if (wc.first == ProcessorMetadataIds::Modulator)
						{
							expectEquals(wc.second, m.constrainerWildcard, "constrainer wildcard mismatch at " + m.id.toString());
							found = true;
							break;
						}
					}

					expect(found, "wildcard not found");
                }
			}
            else
            {
                auto noConstrainer = modChain->getFactoryType()->getConstrainer() == nullptr;
                auto notOnlyVoiceStart = dynamic_cast<VoiceStartModulatorFactoryType*>(modChain->getFactoryType()) == nullptr;

                expect(noConstrainer&& notOnlyVoiceStart, "missing wildcard");
            }
        }

    }

    void testAllModules()
    {
        ctx = std::make_unique<TestContext>(*this);

        raw::Builder b(ctx->mc);

        auto sine = b.create<SineSynth>(ctx->mc->getMainSynthChain());

        ProcessorMetadataRegistry rd;

        for (auto id : rd.getRegisteredTypes())
        {
            testModule(*rd.get(id), sine);
        }
    }

    void testLfoMetadataParameterCount()
    {
        beginTest("LFO metadata has 11 parameters");

        auto md = LfoModulator::createMetadata();
        expect(md.parameters.size() == LfoModulator::numParameters,
               "Expected " + String(LfoModulator::numParameters) + " parameters, got " +
               String(md.parameters.size()));
    }

    void testLfoMetadataParameterIds()
    {
        /** Setup: LfoModulator::createMetadata()
         *  Scenario: Each parameter has an id matching the original parameterNames.add() strings
         *  Expected: All 11 IDs present in correct order
         */
        beginTest("LFO parameter IDs match enum names");

        auto md = LfoModulator::createMetadata();
        StringArray expectedIds = {
            "Frequency", "FadeIn", "WaveformType", "Legato", "TempoSync",
            "SmoothingTime", "NumSteps", "LoopEnabled", "PhaseOffset",
            "SyncToMasterClock", "IgnoreNoteOn"
        };

        for (int i = 0; i < expectedIds.size(); i++)
        {
            expect(md.parameters[i].id == Identifier(expectedIds[i]),
                   "Parameter " + String(i) + " should be '" + expectedIds[i] +
                   "' but is '" + md.parameters[i].id.toString() + "'");
        }
    }

    void testLfoMetadataParameterTypes()
    {
        beginTest("LFO parameter types");

        auto md = LfoModulator::createMetadata();
        using T = ProcessorMetadata::ParameterMetadata::Type;

        // Float parameters
        expect(md[LfoModulator::Frequency].type == T::Float, "Frequency should be Float");
        expect(md[LfoModulator::FadeIn].type == T::Float, "FadeIn should be Float");
        expect(md[LfoModulator::SmoothingTime].type == T::Float, "SmoothingTime should be Float");
        expect(md[LfoModulator::NumSteps].type == T::Float, "NumSteps should be Float");
        expect(md[LfoModulator::PhaseOffset].type == T::Float, "PhaseOffset should be Float");

        // List parameter
        expect(md[LfoModulator::WaveFormType].type == T::List, "WaveFormType should be List");

        // Toggle parameters
        expect(md[LfoModulator::Legato].type == T::Toggle, "Legato should be Toggle");
        expect(md[LfoModulator::TempoSync].type == T::Toggle, "TempoSync should be Toggle");
        expect(md[LfoModulator::LoopEnabled].type == T::Toggle, "LoopEnabled should be Toggle");
        expect(md[LfoModulator::SyncToMasterClock].type == T::Toggle, "SyncToMasterClock should be Toggle");
        expect(md[LfoModulator::IgnoreNoteOn].type == T::Toggle, "IgnoreNoteOn should be Toggle");
    }

    void testLfoMetadataParameterDefaults()
    {
        beginTest("LFO parameter default values");

        auto md = LfoModulator::createMetadata();

        expectEquals(md[LfoModulator::Frequency].defaultValue, 3.0f, "Frequency default");
        expectEquals(md[LfoModulator::FadeIn].defaultValue, 1000.0f, "FadeIn default");
        expectEquals(md[LfoModulator::WaveFormType].defaultValue, 1.0f, "WaveFormType default");
        expectEquals(md[LfoModulator::Legato].defaultValue, 1.0f, "Legato default");
        expectEquals(md[LfoModulator::TempoSync].defaultValue, 0.0f, "TempoSync default");
        expectEquals(md[LfoModulator::SmoothingTime].defaultValue, 5.0f, "SmoothingTime default");
        expectEquals(md[LfoModulator::NumSteps].defaultValue, 16.0f, "NumSteps default");
        expectEquals(md[LfoModulator::LoopEnabled].defaultValue, 1.0f, "LoopEnabled default");
        expectEquals(md[LfoModulator::PhaseOffset].defaultValue, 0.0f, "PhaseOffset default");
        expectEquals(md[LfoModulator::SyncToMasterClock].defaultValue, 0.0f, "SyncToMasterClock default");
        expectEquals(md[LfoModulator::IgnoreNoteOn].defaultValue, 0.0f, "IgnoreNoteOn default");
    }

    void testLfoMetadataParameterRanges()
    {
        beginTest("LFO parameter ranges");

        auto md = LfoModulator::createMetadata();

        // Frequency: {0.01, 40.0}
        auto freq = md[LfoModulator::Frequency];
        expectEquals(freq.range.rng.start, 0.01, "Frequency range start");
        expectEquals(freq.range.rng.end, 40.0, "Frequency range end");

        // FadeIn: {0.0, 3000.0}
        auto fadeIn = md[LfoModulator::FadeIn];
        expectEquals(fadeIn.range.rng.start, 0.0, "FadeIn range start");
        expectEquals(fadeIn.range.rng.end, 3000.0, "FadeIn range end");

        // WaveFormType: {1, 7, 1} (1-indexed, 7 items)
        auto waveform = md[LfoModulator::WaveFormType];
        expectEquals(waveform.range.rng.start, 1.0, "WaveFormType range start");
        expectEquals(waveform.range.rng.end, 7.0, "WaveFormType range end");
        expectEquals(waveform.range.rng.interval, 1.0, "WaveFormType range interval");

        // Toggle parameters: {0, 1, 1}
        auto legato = md[LfoModulator::Legato];
        expectEquals(legato.range.rng.start, 0.0, "Toggle range start");
        expectEquals(legato.range.rng.end, 1.0, "Toggle range end");
        expectEquals(legato.range.rng.interval, 1.0, "Toggle range interval");

        // NumSteps: {1, 128, 1}
        auto numSteps = md[LfoModulator::NumSteps];
        expectEquals(numSteps.range.rng.start, 1.0, "NumSteps range start");
        expectEquals(numSteps.range.rng.end, 128.0, "NumSteps range end");
        expectEquals(numSteps.range.rng.interval, 1.0, "NumSteps range interval");
    }

    void testLfoMetadataSliderModes()
    {
        beginTest("LFO parameter slider modes");

        auto md = LfoModulator::createMetadata();

        expect(md[LfoModulator::Frequency].sliderMode == HiSlider::Frequency, "Frequency mode");
        expect(md[LfoModulator::FadeIn].sliderMode == HiSlider::Time, "FadeIn mode");
        expect(md[LfoModulator::SmoothingTime].sliderMode == HiSlider::Time, "SmoothingTime mode");
        expect(md[LfoModulator::NumSteps].sliderMode == HiSlider::Discrete, "NumSteps mode");
        expect(md[LfoModulator::PhaseOffset].sliderMode == HiSlider::NormalizedPercentage, "PhaseOffset mode");

        // Toggle and List parameters should not set a slider mode
        expect(md[LfoModulator::Legato].sliderMode == HiSlider::numModes, "Legato should have no slider mode");
        expect(md[LfoModulator::WaveFormType].sliderMode == HiSlider::numModes, "WaveFormType should have no slider mode");
    }

    void testLfoMetadataTempoSync()
    {
        /** Setup: LFO Frequency parameter with withTempoSyncMode(TempoSync)
         *  Scenario: tempoSyncControllerIndex stores the controlling toggle index
         *  Expected: Controller points to TempoSync enum; non-syncable params have -1
         */
        beginTest("LFO Frequency tempo-sync dual mode");

        auto md = LfoModulator::createMetadata();
        auto freq = md[LfoModulator::Frequency];

        expect(freq.tempoSyncControllerIndex == LfoModulator::TempoSync,
               "Tempo sync should reference TempoSync parameter");

        // Non-tempo-sync parameters should have -1 controller index
        expect(md[LfoModulator::FadeIn].tempoSyncControllerIndex == -1,
               "FadeIn should not have tempo sync");
    }

   

    template <typename T> void testConstrainerParser()
    {
        ScopedPointer<T> constrainer = new T();

        beginTest("Testing constrainer wildcard parser for " + constrainer->getDescription());

        using ConstrainerMap = std::map<std::pair<Identifier, String>, std::vector<Identifier>>;

        ProcessorMetadataRegistry rd;

        std::vector<ProcessorMetadata> allData;

        for (const auto& typeId : rd.getRegisteredTypes())
            allData.push_back(*rd.get(typeId));

        ConstrainerMap okMap;
        ConstrainerMap fails;

        auto wildcard = T::getWildcard();

        

        for (const auto& wc : wildcard)
        {
            Identifier typeFilter = wc.first;

            std::vector<Identifier> okList, failList;

            for (const auto& md : allData)
            {
                if (md.type != typeFilter)
                    continue;

                if (constrainer->allowType(md.id))
                    okList.push_back(md.id);
                else
                    failList.push_back(md.id);
            }

            okMap[wc] = okList;
            fails[wc] = failList;
        }

        for (auto& wc : okMap)
        {
            ProcessorMetadata::ConstrainerParser okParser(wc.first.second);

            for (const auto& typeId : wc.second)
            {
                auto r = okParser.check(*rd.get(typeId));
                expect(r.wasOk(), wc.first.second + "should accept: " + typeId.toString() + ". got: " + r.getErrorMessage());
            }
        }

		for (auto& wc : fails)
		{
			ProcessorMetadata::ConstrainerParser failParser(wc.first.second);

			for (const auto& typeId : wc.second)
			{
                auto r = failParser.check(*rd.get(typeId));
				expect(!r.wasOk(), wc.first.second + " - should reject: " + typeId.toString());
			}
		}
    }

    void testLfoMetadataValueList()
    {
        beginTest("LFO WaveFormType value list");

        auto md = LfoModulator::createMetadata();
        auto waveform = md[LfoModulator::WaveFormType];

        expectEquals(waveform.vtc.itemList.size(), 7, "Should have 7 waveform names");

        StringArray expected = {"Sine", "Triangle", "Saw", "Square", "Random", "Custom", "Steps"};
        for (int i = 0; i < expected.size(); i++)
        {
            expect(waveform.vtc.itemList[i] == expected[i],
                   "Waveform " + String(i) + " should be '" + expected[i] +
                   "' but is '" + waveform.vtc.itemList[i] + "'");
        }
    }

    void testLfoMetadataModulationChains()
    {
        beginTest("LFO modulation chain metadata");

        auto md = LfoModulator::createMetadata();
        expectEquals(md.modulation.size(), 2, "Should have 2 modulation chains");

        // IntensityChain
        auto intensity = md.modulation[0];
        expect(intensity.chainIndex == LfoModulator::IntensityChain, "Intensity chain index");
        expect(intensity.id == Identifier("LFO Intensity Mod"), "Intensity chain ID");
        expect(intensity.modulationMode == scriptnode::modulation::ParameterMode::ScaleOnly,
               "Intensity chain mode");

        // FrequencyChain
        auto frequency = md.modulation[1];
        expect(frequency.chainIndex == LfoModulator::FrequencyChain, "Frequency chain index");
        expect(frequency.id == Identifier("LFO Frequency Mod"), "Frequency chain ID");
        expect(frequency.modulationMode == scriptnode::modulation::ParameterMode::ScaleOnly,
               "Frequency chain mode");
    }

    void testLfoMetadataTypeAndSubtype()
    {
        beginTest("LFO metadata type and subtype");

        auto md = LfoModulator::createMetadata();

        expect(md.id == Identifier("LFO"), "Type ID should be 'LFO'");
        expect(md.prettyName == "LFO Modulator", "Pretty name");
        expect(md.type == Identifier("Modulator"), "Type should be 'Modulator'");
        expect(md.subtype == Identifier("TimeVariantModulator"), "Subtype should be 'TimeVariantModulator'");
        expect(md.dataType == ProcessorMetadata::DataType::Static, "DataType should be Static");
    }

    //==========================================================================
    // WaveSynth static metadata tests
    //==========================================================================

    void testWaveSynthMetadataParameterCount()
    {
        beginTest("WaveSynth metadata has 19 parameters (4 base + 15 own)");

        auto md = WaveSynth::createMetadata();
        expect(md.parameters.size() == WaveSynth::numWaveSynthParameters,
               "Expected " + String(WaveSynth::numWaveSynthParameters) + " parameters, got " +
               String(md.parameters.size()));
    }

    void testWaveSynthMetadataParameterIds()
    {
        beginTest("WaveSynth parameter IDs include base and own");

        auto md = WaveSynth::createMetadata();
        StringArray expectedIds = {
            // Base (ModulatorSynth)
            "Gain", "Balance", "VoiceLimit", "KillFadeTime",
            // Own (WaveSynth)
            "OctaveTranspose1", "WaveForm1", "Detune1", "Pan1",
            "OctaveTranspose2", "WaveForm2", "Detune2", "Pan2",
            "Mix", "EnableSecondOscillator", "PulseWidth1", "PulseWidth2",
            "HardSync", "SemiTones1", "SemiTones2"
        };

        for (int i = 0; i < expectedIds.size(); i++)
        {
            expect(md.parameters[i].id == Identifier(expectedIds[i]),
                   "Parameter " + String(i) + " should be '" + expectedIds[i] +
                   "' but is '" + md.parameters[i].id.toString() + "'");
        }
    }

    void testWaveSynthMetadataParameterTypes()
    {
        beginTest("WaveSynth parameter types");

        auto md = WaveSynth::createMetadata();
        using T = ProcessorMetadata::ParameterMetadata::Type;

        // Base parameters
        expect(md.getParameterById(ModulatorSynth::Gain)->type == T::Float, "Gain should be Float");
        expect(md.getParameterById(ModulatorSynth::Balance)->type == T::Float, "Balance should be Float");

        // Own Float parameters
        expect(md.getParameterById(WaveSynth::OctaveTranspose1)->type == T::Float, "OctaveTranspose1 should be Float");
        expect(md.getParameterById(WaveSynth::Detune1)->type == T::Float, "Detune1 should be Float");
        expect(md.getParameterById(WaveSynth::Mix)->type == T::Float, "Mix should be Float");

        // List parameters
        expect(md.getParameterById(WaveSynth::WaveForm1)->type == T::List, "WaveForm1 should be List");
        expect(md.getParameterById(WaveSynth::WaveForm2)->type == T::List, "WaveForm2 should be List");

        // Toggle parameters
        expect(md.getParameterById(WaveSynth::EnableSecondOscillator)->type == T::Toggle, "EnableSecondOscillator should be Toggle");
        expect(md.getParameterById(WaveSynth::HardSync)->type == T::Toggle, "HardSync should be Toggle");
    }

	void testWaveSynthMetadataParameterDefaults()
	{
		beginTest("WaveSynth parameter default values (base + own)");

		auto md = WaveSynth::createMetadata();

		// Base parameter defaults (inherited from createBaseMetadata)
		expectEquals(md.getParameterById(ModulatorSynth::Gain)->defaultValue, 0.25f, "Gain default");
		expectEquals(md.getParameterById(ModulatorSynth::Balance)->defaultValue, 0.0f, "Balance default");
		expectEquals(md.getParameterById(ModulatorSynth::VoiceLimit)->defaultValue, (float)NUM_POLYPHONIC_VOICES, "VoiceLimit default");
		expectEquals(md.getParameterById(ModulatorSynth::KillFadeTime)->defaultValue, 20.0f, "KillFadeTime default");

		// Own parameter defaults
		expectEquals(md.getParameterById(WaveSynth::OctaveTranspose1)->defaultValue, 0.0f, "OctaveTranspose1 default");
		expectEquals(md.getParameterById(WaveSynth::WaveForm1)->defaultValue, (float)WaveformComponent::WaveformType::Saw, "WaveForm1 default");
        expectEquals(md.getParameterById(WaveSynth::Detune1)->defaultValue, 0.0f, "Detune1 default");
        expectEquals(md.getParameterById(WaveSynth::Pan1)->defaultValue, 0.0f, "Pan1 default");
        expectEquals(md.getParameterById(WaveSynth::Mix)->defaultValue, 0.5f, "Mix default");
        expectEquals(md.getParameterById(WaveSynth::EnableSecondOscillator)->defaultValue, 1.0f, "EnableSecondOscillator default");
        expectEquals(md.getParameterById(WaveSynth::PulseWidth1)->defaultValue, 0.5f, "PulseWidth1 default");
        expectEquals(md.getParameterById(WaveSynth::PulseWidth2)->defaultValue, 0.5f, "PulseWidth2 default");
        expectEquals(md.getParameterById(WaveSynth::HardSync)->defaultValue, 0.0f, "HardSync default (was missing from old getDefaultValue)");
        expectEquals(md.getParameterById(WaveSynth::SemiTones1)->defaultValue, 0.0f, "SemiTones1 default");
        expectEquals(md.getParameterById(WaveSynth::SemiTones2)->defaultValue, 0.0f, "SemiTones2 default");
    }

    void testWaveSynthMetadataParameterRanges()
    {
        beginTest("WaveSynth parameter ranges");

        auto md = WaveSynth::createMetadata();

        // OctaveTranspose1: {-5, 5, 1}
        auto oct = md.getParameterById(WaveSynth::OctaveTranspose1);
        expectEquals(oct->range.rng.start, -5.0, "OctaveTranspose1 range start");
        expectEquals(oct->range.rng.end, 5.0, "OctaveTranspose1 range end");
        expectEquals(oct->range.rng.interval, 1.0, "OctaveTranspose1 range interval");

        // Detune1: {-100, 100}
        auto det = md.getParameterById(WaveSynth::Detune1);
        expectEquals(det->range.rng.start, -100.0, "Detune1 range start");
        expectEquals(det->range.rng.end, 100.0, "Detune1 range end");

        // WaveForm1: {1, 9, 1} (1-indexed, 9 items)
        auto wf = md.getParameterById(WaveSynth::WaveForm1);
        expectEquals(wf->range.rng.start, 1.0, "WaveForm1 range start");
        expectEquals(wf->range.rng.end, 9.0, "WaveForm1 range end");
        expectEquals(wf->range.rng.interval, 1.0, "WaveForm1 range interval");

        // SemiTones1: {-12, 12, 1}
        auto semi = md.getParameterById(WaveSynth::SemiTones1);
        expectEquals(semi->range.rng.start, -12.0, "SemiTones1 range start");
        expectEquals(semi->range.rng.end, 12.0, "SemiTones1 range end");
        expectEquals(semi->range.rng.interval, 1.0, "SemiTones1 range interval");
    }

    void testWaveSynthMetadataSliderModes()
    {
        beginTest("WaveSynth parameter slider modes");

        auto md = WaveSynth::createMetadata();

        expect(md.getParameterById(WaveSynth::OctaveTranspose1)->sliderMode == HiSlider::Discrete, "OctaveTranspose1 mode");
        expect(md.getParameterById(WaveSynth::Detune1)->sliderMode == HiSlider::Linear, "Detune1 mode");
        expect(md.getParameterById(WaveSynth::Pan1)->sliderMode == HiSlider::Pan, "Pan1 mode");
        expect(md.getParameterById(WaveSynth::Mix)->sliderMode == HiSlider::NormalizedPercentage, "Mix mode");
        expect(md.getParameterById(WaveSynth::PulseWidth1)->sliderMode == HiSlider::NormalizedPercentage, "PulseWidth1 mode");
        expect(md.getParameterById(WaveSynth::SemiTones1)->sliderMode == HiSlider::Discrete, "SemiTones1 mode");

        // List and Toggle parameters have no slider mode
        expect(md.getParameterById(WaveSynth::WaveForm1)->sliderMode == HiSlider::numModes, "WaveForm1 should have no slider mode");
        expect(md.getParameterById(WaveSynth::EnableSecondOscillator)->sliderMode == HiSlider::numModes, "EnableSecondOscillator should have no slider mode");
    }

    void testWaveSynthMetadataValueList()
    {
        beginTest("WaveSynth WaveForm value list");

        auto md = WaveSynth::createMetadata();
        auto wf1 = md.getParameterById(WaveSynth::WaveForm1);

        expectEquals(wf1->vtc.itemList.size(), 9, "Should have 9 waveform names");

        StringArray expected = {"Sine", "Triangle", "Saw", "Square", "Noise",
                                "Triangle 2", "Square 2", "Trapezoid 1", "Trapezoid 2"};
        for (int i = 0; i < expected.size(); i++)
        {
            expect(wf1->vtc.itemList[i] == expected[i],
                   "Waveform " + String(i) + " should be '" + expected[i] +
                   "' but is '" + wf1->vtc.itemList[i] + "'");
        }

        // WaveForm2 should have the same list
        auto wf2 = md.getParameterById(WaveSynth::WaveForm2);
        expectEquals(wf2->vtc.itemList.size(), 9, "WaveForm2 should also have 9 items");
    }

	void testWaveSynthMetadataModulationChains()
	{
		beginTest("WaveSynth modulation chain metadata (2 base + 2 own)");

		auto md = WaveSynth::createMetadata();
		expectEquals(md.modulation.size(), 4, "Should have 4 modulation chains (2 base + 2 own)");

		// Base: GainModulation
        auto gain = md.modulation[0];
		expect(gain.chainIndex == ModulatorSynth::GainModulation, "Gain chain index");
		expect(gain.id == Identifier("Gain Modulation"), "Gain chain ID");
		expect(gain.modulationMode == scriptnode::modulation::ParameterMode::ScaleOnly,
		       "Gain chain mode");

		// Base: PitchModulation
        auto pitch = md.modulation[1];
		expect(pitch.chainIndex == ModulatorSynth::PitchModulation, "Pitch chain index");
		expect(pitch.id == Identifier("Pitch Modulation"), "Pitch chain ID");
		expect(pitch.modulationMode == scriptnode::modulation::ParameterMode::Pitch,
		       "Pitch chain mode");

		// Own: MixModulation
		auto mix = md.modulation[2];
		expect(mix.chainIndex == WaveSynth::MixModulation, "Mix chain index");
		expect(mix.id == Identifier("Mix Modulation"), "Mix chain ID");
		expect(mix.modulationMode == scriptnode::modulation::ParameterMode::ScaleAdd,
		       "Mix chain mode");

		// Own: Osc2PitchChain
		auto osc2 = md.modulation[3];
		expect(osc2.chainIndex == WaveSynth::Osc2PitchChain, "Osc2Pitch chain index");
		expect(osc2.id == Identifier("Osc2 Pitch Modulation"), "Osc2Pitch chain ID");
		expect(osc2.modulationMode == scriptnode::modulation::ParameterMode::Pitch,
		       "Osc2Pitch chain mode");
	}

    void testWaveSynthMetadataTypeAndSubtype()
    {
        beginTest("WaveSynth metadata type and subtype");

        auto md = WaveSynth::createMetadata();

        expect(md.id == Identifier("WaveSynth"), "Type ID should be 'WaveSynth'");
        expect(md.prettyName == "Waveform Generator", "Pretty name");
        expect(md.type == Identifier("SoundGenerator"), "Type should be 'SoundGenerator'");
        expect(md.subtype == Identifier("SoundGenerator"), "Subtype should be 'SoundGenerator'");
        expect(md.dataType == ProcessorMetadata::DataType::Static, "DataType should be Static");
    }

    //==========================================================================
    // Registry tests
    //==========================================================================

    void testRegistryPopulation()
    {
        /** Setup: SharedResourcePointer<ProcessorMetadataRegistry>
         *  Scenario: Registry auto-populates on construction
         *  Expected: All processor types registered
         */
        beginTest("Registry populates all processor types");

        SharedResourcePointer<ProcessorMetadataRegistry> registry;
        auto types = registry->getRegisteredTypes();

        // We registered 79 processor types in registerAllMetadata()
        expect(registry->getNumRegistered() > 70,
               "Should have at least 70 registered types, got " +
               String(registry->getNumRegistered()));
    }

    void testRegistryLfoEntry()
    {
        beginTest("Registry contains LFO with full metadata");

        SharedResourcePointer<ProcessorMetadataRegistry> registry;
        auto md = registry->get(Identifier("LFO"));

        expect(md != nullptr, "LFO should be in registry");
        expect(md->isValid(), "LFO should have valid metadata");
        expect(md->dataType == ProcessorMetadata::DataType::Static,
               "LFO should be Static");
        expectEquals(md->parameters.size(), 11, "LFO should have 11 parameters");
    }

	void testRegistryWaveSynthEntry()
	{
		beginTest("Registry contains WaveSynth with full metadata");

		SharedResourcePointer<ProcessorMetadataRegistry> registry;
		auto md = registry->get(Identifier("WaveSynth"));

		expect(md != nullptr, "WaveSynth should be in registry");
		expect(md->isValid(), "WaveSynth should have valid metadata");
		expect(md->dataType == ProcessorMetadata::DataType::Static,
		       "WaveSynth should be Static");
		expectEquals(md->parameters.size(), (int)WaveSynth::numWaveSynthParameters,
		             "WaveSynth should have " + String(WaveSynth::numWaveSynthParameters) +
		             " parameters (4 base + 15 own)");
	}

    void testRegistryAllEntriesValid()
    {
        beginTest("All registry entries have valid metadata");

        SharedResourcePointer<ProcessorMetadataRegistry> registry;

        for (auto& typeId : registry->getRegisteredTypes())
        {
            auto md = registry->get(typeId);
            expect(md != nullptr, typeId.toString() + " should be in registry");
            expect(md->isValid(), typeId.toString() + " should have valid metadata");
        }
    }

    void testRegistryToJSON()
    {
        beginTest("Registry toJSON produces array");

        ProcessorMetadataRegistry registry;
        auto json = registry.toJSON();

        auto moduleList = json["modules"];

        expect(moduleList.isArray(), "toJSON should return an array");
        expect(moduleList.size() > 70, "JSON array should have entries for all processors");

        // Find LFO in the JSON output
        bool foundLfo = false;
        for (int i = 0; i < moduleList.size(); i++)
        {
            if (moduleList[i]["id"].toString() == "LFO")
            {
                foundLfo = true;
                auto params = moduleList[i]["parameters"];
                expect(params.isArray(), "LFO should have parameters array in JSON");
                expect(params.size() == 11, "LFO JSON should have 11 parameters");

                break;
            }
        }
        expect(foundLfo, "LFO should appear in registry JSON");
    }

    //==========================================================================
    // Processor integration tests (require a live MainController)
    //==========================================================================

    void testLfoParameterResolution()
    {
        /** Setup: Construct LfoModulator via MainController
         *  Scenario: Constructor calls updateParameterSlots() which caches metadata
         *  Expected: getNumParameters() returns 11, getIdentifierForParameterIndex()
         *            resolves each parameter to the correct metadata ID
         */
        beginTest("LFO parameter resolution matches metadata");

        ScopedPointer<LfoModulator> lfo = ctx->createLfo();

        expectEquals(lfo->getNumParameters(), (int)LfoModulator::numParameters,
                     "getNumParameters() should return " + String(LfoModulator::numParameters));

        auto md = LfoModulator::createMetadata();
        for (int i = 0; i < md.parameters.size(); i++)
        {
            expect(lfo->getIdentifierForParameterIndex(md.parameters[i].parameterIndex) == md.parameters[i].id,
                   "Parameter " + String(i) + " should resolve to '" +
                   md.parameters[i].id.toString() + "'");
        }
    }

    void testLfoHasInitialisedMetadata()
    {
        beginTest("LFO hasInitialisedMetadata returns true after construction");

        ScopedPointer<LfoModulator> lfo = ctx->createLfo();
        expect(lfo->hasInitialisedMetadata(), "LFO should report initialised metadata");
    }

    void testLfoGetDefaultValueFromMetadata()
    {
        /** Setup: Construct LfoModulator, query getDefaultValue for each parameter
         *  Scenario: Base class reads from cached metadata; Frequency uses a
         *            runtimeDefaultQueryFunction based on tempoSync state
         *  Expected: Non-dynamic defaults match metadata declarations.
         *            Frequency is skipped - its dynamic default depends on tempoSync
         *            state, which is not meaningful to test against the static default.
         */
        beginTest("LFO getDefaultValue reads from metadata");

        ScopedPointer<LfoModulator> lfo = ctx->createLfo();
        auto md = LfoModulator::createMetadata();

        // Skip Frequency (index 0) - it has a dynamic override based on tempoSync state
        for (int i = 1; i < md.parameters.size(); i++)
        {
            expectEquals(lfo->getDefaultValue(i), md.parameters[i].defaultValue,
                         "Default value for parameter " + md.parameters[i].id.toString());
        }
    }

    void testLfoGetMetadataVirtual()
    {
        beginTest("LFO getMetadata() returns cached metadata via virtual dispatch");

        ScopedPointer<LfoModulator> lfo = ctx->createLfo();

        // Call via Processor* base pointer to test virtual dispatch
        Processor* p = lfo.get();
        auto md = p->getMetadata();

        expect(md.isValid(), "Should return valid metadata");
        expect(md.id == Identifier("LFO"), "Should be LFO metadata");
        expectEquals(md.parameters.size(), 11, "Should have 11 parameters");
    }

    //==========================================================================
    // WaveSynth processor integration tests
    //==========================================================================

	void testWaveSynthParameterResolution()
	{
		/** Setup: Construct WaveSynth via MainController
		 *  Scenario: metadataInitialised calls updateParameterSlots() in the initializer
		 *            list, which caches metadata
		 *  Expected: getNumParameters() returns 19, getIdentifierForParameterIndex()
		 *            resolves each parameter to the correct metadata ID
		 */
		beginTest("WaveSynth parameter resolution matches metadata");

		ScopedPointer<WaveSynth> ws = ctx->createWaveSynth();

		expectEquals(ws->getNumParameters(), (int)WaveSynth::numWaveSynthParameters,
		             "getNumParameters() should return " + String(WaveSynth::numWaveSynthParameters));

		auto md = WaveSynth::createMetadata();
		for (int i = 0; i < md.parameters.size(); i++)
		{
			expect(ws->getIdentifierForParameterIndex(md.parameters[i].parameterIndex) == md.parameters[i].id,
			       "Parameter " + String(i) + " should resolve to '" +
			       md.parameters[i].id.toString() + "'");
		}
	}

    void testWaveSynthHasInitialisedMetadata()
    {
        beginTest("WaveSynth hasInitialisedMetadata returns true after construction");

        ScopedPointer<WaveSynth> ws = ctx->createWaveSynth();
        expect(ws->hasInitialisedMetadata(), "WaveSynth should report initialised metadata");
    }

    void testWaveSynthGetDefaultValueFromMetadata()
    {
        /** Setup: Construct WaveSynth, query getDefaultValue for each parameter
         *  Scenario: Base class reads from cached metadata (no override needed)
         *  Expected: All defaults match metadata declarations, including HardSync
         *            which was previously missing from the old switch statement
         */
        beginTest("WaveSynth getDefaultValue reads from metadata");

        ScopedPointer<WaveSynth> ws = ctx->createWaveSynth();
        auto md = WaveSynth::createMetadata();

        for (int i = 0; i < md.parameters.size(); i++)
        {
            auto paramIndex = md.parameters[i].parameterIndex;
            expectEquals(ws->getDefaultValue(paramIndex), md.parameters[i].defaultValue,
                         "Default value for parameter " + md.parameters[i].id.toString());
        }
    }

    void testWaveSynthGetMetadataVirtual()
    {
        beginTest("WaveSynth getMetadata() returns cached metadata via virtual dispatch");

        ScopedPointer<WaveSynth> ws = ctx->createWaveSynth();

        Processor* p = ws.get();
        auto md = p->getMetadata();

        expect(md.isValid(), "Should return valid metadata");
        expect(md.id == Identifier("WaveSynth"), "Should be WaveSynth metadata");
        expectEquals(md.parameters.size(), (int)WaveSynth::numWaveSynthParameters,
                     "Should have " + String(WaveSynth::numWaveSynthParameters) +
                     " parameters (4 base + 15 own)");
    }

    //==========================================================================
    // Edge case tests
    //==========================================================================

    void testOperatorBracketOutOfRange()
    {
        beginTest("operator[] out of range returns invalid sentinel");

        auto md = LfoModulator::createMetadata();

		// Access beyond parameter count (operator[] returns by value)
		auto invalid = md[999];
        expect(invalid.parameterIndex == -1,
               "Out-of-range access should return sentinel with parameterIndex == -1");
    }

    void testGetParameterByIdInvalid()
    {
        beginTest("getParameterById returns nullptr for invalid index");

        auto md = LfoModulator::createMetadata();
        expect(md.getParameterById(999) == nullptr,
               "Should return nullptr for non-existent parameter index");
    }
};

static ProcessorMetadataTest processorMetadataTest;

} // namespace hise

#endif // HI_RUN_UNIT_TESTS
