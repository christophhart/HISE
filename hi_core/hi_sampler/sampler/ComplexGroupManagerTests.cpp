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

namespace hise { using namespace juce;

struct ComplexGroupManagerTests: public UnitTest
{
	using Bitmask = SynthSoundWithBitmask::Bitmask;
	using LogicType = ComplexGroupManager::LogicType;
	using Manager = ComplexGroupManager;
	static constexpr uint8 IgnoreFlag = ComplexGroupManager::IgnoreFlag;
	using Flags = ComplexGroupManager::Flags;

	struct TestInstance
	{
		TestInstance(UnitTest* t, const String& name):
		  parent(t)
		{
			parent->beginTest(name);
			reset();
		}

		struct TestDummy: public SynthSoundWithBitmask
		{
			TestDummy(const ValueTree& sample)
			{
				jassert(sample.getType() == Identifier("sample"));

				auto loKey = (int)sample[SampleIds::LoKey];
				auto hikey = (int)sample[SampleIds::HiKey];
				sampleName = sample[SampleIds::FileName].toString();
				noteMap.setRange(loKey, hikey - loKey, true);

				setBitmask((Bitmask)(int64)sample[SampleIds::RRGroup]);
			}

			bool appliesToChannel (int midiChannel) override { return true; }

			bool appliesToNote(int m) override { return noteMap[m]; }
			bool appliesToVelocity(int) override { return true; }

			Range<int> getNoteRange() const override
			{
				auto l = noteMap.findNextSetBit(0);
				auto h = noteMap.getHighestBit();
				return { l, h + 1 };
			}

			String sampleName;
			BigInteger noteMap;

			JUCE_DECLARE_WEAK_REFERENCEABLE(TestDummy);
		};

		struct LayoutGenerator: public ComplexGroupManager::ScopedUpdateDelayer
		{
			LayoutGenerator(TestInstance& m):
			  ScopedUpdateDelayer(*m.gm)
			{};

			void addLegatoLayer()
			{
				addLayer("Legato", LogicType::LegatoInterval, { "50", "72" }, Flags::FlagIgnorable );
			}

			void addRRLayer(const StringArray& items)
			{
				addLayer("RR", LogicType::RoundRobin, items, Flags::FlagIgnorable);
			}

			void addKeyswitchLayer(const StringArray& items)
			{
				addLayer("Keyswitch", LogicType::Keyswitch, items, Flags::FlagCached | Flags::FlagPurgable);
			}

			void addLayer(const Identifier& id, LogicType lt, const StringArray& items, int flags)
			{
				ValueTree v(groupIds::Layer);
				v.setProperty(groupIds::id, id.toString(), nullptr);
				v.setProperty(groupIds::type, ComplexGroupManagerComponent::Helpers::getLogicTypeName(lt), nullptr);
				v.setProperty(groupIds::tokens, items.joinIntoString(","), nullptr);
				v.setProperty(groupIds::cached, ComplexGroupManager::Helpers::shouldBeCached(flags), nullptr);
				v.setProperty(groupIds::ignorable, ComplexGroupManager::Helpers::canBeIgnored(flags), nullptr);
				v.setProperty(groupIds::purgable, ComplexGroupManager::Helpers::canBePurged(flags), nullptr);

				try
				{
					gm.getDataTree().addChild(v, -1, nullptr);
				}
				catch(Result& r)
				{
					DBG(r.getErrorMessage());
					jassertfalse;
				}
			}
		};

		/** Use this to programatically generate samplemaps.
		 *
		 *  It assumes the single note number as first file token and
		 *	then the order of the tokens to match exactly the order and amount of
		 *	layers defined by the Layout Generator.
		 */
		struct SampleMapGenerator
		{
			SampleMapGenerator(TestInstance& m_):
			  m(m_)
			{}

			~SampleMapGenerator()
			{
				ValueTree sampleMap("samplemap");

				for(auto s: filenames)
				{
					ValueTree sample("sample");
					
					auto noteName = s.upToFirstOccurrenceOf("_", false, false);

					for(int i = 0; i < 128; i++)
					{
						if(MidiMessage::getMidiNoteName(i, true, true, 3) == noteName)
						{
							sample.setProperty(SampleIds::LoKey, i, nullptr);
							sample.setProperty(SampleIds::HiKey, i+1, nullptr);
						}
					}

					sample.setProperty(SampleIds::FileName, s, nullptr);
					auto bm = m.gm->parseBitmask(s, 1);
					sample.setProperty(SampleIds::RRGroup, (int64)bm, nullptr);

					sampleMap.addChild(sample, -1, nullptr);
				}

				m.loadSampleMap(sampleMap);
			}

			void addSample(const String& newSample)
			{
				filenames.add(newSample);
			}

			StringArray filenames;

			TestInstance& m;
		};

		void loadLayout(const ValueTree& v)
		{
			jassert(v.getType() == groupIds::Layers);

			gm = new ComplexGroupManager(&soundList);

			for(auto c: v)
				gm->getDataTree().addChild(c.createCopy(), -1, nullptr);
		}

		void loadSampleMap(const ValueTree& v)
		{
			for(auto c: v)
				soundList.add(new TestDummy(c));
			
			gm->refreshCache();
		}

		void setPurged(const Identifier& layer, const String& token, bool shouldBePurged)
		{
			try
			{
				gm->setPurged(layer, token, shouldBePurged);
			}
			catch(Result& r)
			{
				DBG(r.getErrorMessage());
				jassertfalse;
			}
		}

		void setFilter(const Identifier& id, const String& tokenValue)
		{
			try
			{
				gm->applyFilter(id, tokenValue, dontSendNotification);
			}
			catch(Result& r)
			{
				DBG(r.getErrorMessage());
				jassertfalse;
			}
		}

		int getNumSamplesWithFilter(const Identifier& groupId, uint8 tokenValue)
		{
			auto idx = gm->getLayerIndex(groupId);
			return gm->getFilteredSelection(idx, tokenValue, true).size();
		}

		void resetPlayingState()
		{
			gm->resetPlayingState();
		}

		void expectSingleSampleAtNoteOn(const String& noteName, const std::string& sampleName, const String& errorMessage = {})
		{
			auto list = onNoteOn(noteName);
			jassert(list.size() == 1);
			jassert(list[0] == sampleName);
			parent->expectEquals<std::string>(list[0], sampleName, errorMessage);
		}

		void expectNumSamplesAtNoteOn(const String& noteName, int numSamples, const String& errorMessage = {})
		{
			auto list = onNoteOn(noteName);
			jassert(list.size() == numSamples);
			parent->expectEquals<size_t>(list.size(), numSamples, errorMessage);
		}

		std::vector<std::string> onNoteOn(const String& noteName)
		{
			std::vector<std::string> samples;

			for(int i = 0; i < 128; i++)
			{
				if(MidiMessage::getMidiNoteName(i, true, true, 3) == noteName)
				{
					HiseEvent no(HiseEvent::Type::NoteOn, i, 127, 1);
					UnorderedStack<ModulatorSynthSound*> sounds;

					gm->handleNoteOn(no);
					gm->collectSounds(no, sounds);

					for(auto s: sounds)
						samples.push_back(dynamic_cast<TestDummy*>(s)->sampleName.toStdString());

					break;
				}
			}

			return samples;
		}

		void reset(const String& name=String())
		{
			if(name.isNotEmpty())
				parent->beginTest(name);
			soundList.clear();
			gm = new ComplexGroupManager(&soundList);
		}

		float getXFadeValue(const String& groupId, const String& sampleName)
		{
			auto idx = gm->getLayerIndex(groupId);

			if(idx == -1)
			{
				parent->expect(false, "Can't find layer with ID " + groupId);
				return -1.0f;
			}

			for(auto s: soundList)
			{
				auto typed = dynamic_cast<TestDummy*>(s);
				if(typed->sampleName == sampleName)
				{
					return gm->getXFadeValue(idx, typed);
				}
			}

			parent->expect(false, "Can't find sample " + sampleName);
			return -1.0f;
		}

	private:

		UnitTest* parent;
		ReferenceCountedArray<SynthesiserSound> soundList;
		ScopedPointer<ComplexGroupManager> gm;
	};

	ComplexGroupManagerTests():
	  UnitTest("Group manager", "Sampler")
	{};

	void runTest() override
	{
		testLegatoInstance();
		testInstance();

		testFirst();

		testXFade();
		testLegato();
		
		testIgnoreFlag();
		testKeyswitches();
		
		testBigPatch();

		testPurge();
	}

	void testFirst()
	{
		TestInstance test(this, "Testing group manager with 5 RRs, 2 XFADE & 2 Types");

		{
			TestInstance::LayoutGenerator l(test);
			l.addRRLayer({ "RR1", "RR2", "RR3", "RR4", "RR5" });
			l.addLayer("XF", LogicType::XFade, { "p", "f" }, 0);
			l.addKeyswitchLayer({ "sus", "trem"});
		}

		{
			TestInstance::SampleMapGenerator s(test);

			s.addSample("D#3_RR1_p_sus"); // { {RR, 1}, {XF, 2}, { TP, 1} });
			s.addSample("D#3_RR2_p_sus"); // { {RR, 2}, {XF, 2}, { TP, 1} });
			s.addSample("D#3_RR3_p_sus"); // { {RR, 3}, {XF, 2}, { TP, 1} });
			s.addSample("D#3_RR1_f_sus"); // { {RR, 1}, {XF, 2}, { TP, 1} });
			s.addSample("D#3_RR2_f_sus"); // { {RR, 2}, {XF, 2}, { TP, 1} });
			s.addSample("D#3_RR3_f_sus"); // { {RR, 3}, {XF, 2}, { TP, 1} });
			s.addSample("D#3_RR1_p_trem"); // { {RR, 1}, {XF, 2}, { TP, 1} });
			s.addSample("D#3_RR2_p_trem"); // { {RR, 2}, {XF, 2}, { TP, 1} });
			s.addSample("D#3_RR3_p_trem"); // { {RR, 3}, {XF, 2}, { TP, 1} });
			s.addSample("D#3_RR1_f_trem"); // { {RR, 1}, {XF, 2}, { TP, 1} });
			s.addSample("D#3_RR2_f_trem"); // { {RR, 2}, {XF, 2}, { TP, 1} });
			s.addSample("D#3_RR3_f_trem"); // { {RR, 3}, {XF, 2}, { TP, 1} });
		}
		
		expectEquals(test.getNumSamplesWithFilter("RR", 1), 4, "size doesn't match");

		expectEquals(test.getNumSamplesWithFilter("XF", 1), 6, "size doesn't match");
		
		// only start tremolo
		test.setFilter("Keyswitch", "trem");

		test.expectNumSamplesAtNoteOn("D#3", 2, "note doesn't work");
		test.expectNumSamplesAtNoteOn("D#3", 2, "note doesn't work");
		test.expectNumSamplesAtNoteOn("D#3", 2, "note doesn't work");
		test.expectNumSamplesAtNoteOn("D#3", 0, "note doesn't work");
		test.expectNumSamplesAtNoteOn("D#3", 0, "note doesn't work");
		test.expectNumSamplesAtNoteOn("D#3", 2, "note doesn't work");
		test.expectNumSamplesAtNoteOn("D#3", 2, "note doesn't work");
		test.expectNumSamplesAtNoteOn("D#3", 2, "note doesn't work");
		test.expectNumSamplesAtNoteOn("D#3", 0, "note doesn't work");
		test.expectNumSamplesAtNoteOn("D#3", 0, "note doesn't work");
	}

	void testKeyswitches()
	{
		TestInstance test(this, "testing keyswitches");

		{
			TestInstance::LayoutGenerator l(test);
			l.addKeyswitchLayer({ "pizz", "spicc", "stacc" });
		}

		{
			TestInstance::SampleMapGenerator s(test);
			s.addSample("C2_pizz"); // {{ KS, 1 }});
			s.addSample("C2_pizz"); // {{ KS, 1 }});
			s.addSample("C2_pizz"); // {{ KS, 1 }});
			s.addSample("C2_pizz"); // {{ KS, 1 }});
			s.addSample("C2_spicc"); // {{ KS, 2 }});
			s.addSample("C2_spicc"); // {{ KS, 2 }});
			s.addSample("C2_spicc"); // {{ KS, 2 }});
			s.addSample("C2_stacc"); // {{ KS, 3 }});
			s.addSample("C2_stacc"); // {{ KS, 3 }});
			s.addSample("C2_stacc"); // {{ KS, 3 }});
			s.addSample("C2_stacc"); // {{ KS, 3 }});
			s.addSample("C2_stacc"); // {{ KS, 3 }});
			s.addSample("C2_stacc"); // {{ KS, 3 }});
		}
		
		expectEquals(test.getNumSamplesWithFilter("Keyswitch", 1), 4, "not equal");
		expectEquals(test.getNumSamplesWithFilter("Keyswitch", 2), 3, "not equal");
		expectEquals(test.getNumSamplesWithFilter("Keyswitch", 3), 6, "not equal");

		test.setFilter("Keyswitch", "pizz");
		test.expectNumSamplesAtNoteOn("C2", 4, "not equal");

		test.setFilter("Keyswitch", "spicc");
		test.expectNumSamplesAtNoteOn("C2", 3, "not equal");

		test.setFilter("Keyswitch", "stacc");
		test.expectNumSamplesAtNoteOn("C2", 6, "not equal");
	}

	void testLegato()
	{
		TestInstance test(this, "Testing minimal legato setup");

		{
			TestInstance::LayoutGenerator l(test);
			l.addLegatoLayer();
		}

		{
			TestInstance::SampleMapGenerator s(test);

			s.addSample("C2_all"); // {{ "LG", 0xFF }}); // sustain (ignore legato)
			s.addSample("C2_D2"); // {{ "LG", 55 }});   // legato at start 55
		}

		test.setFilter("Legato", "all");
		test.expectNumSamplesAtNoteOn("C2", 1, "only sustain plays");

		test.setFilter("Legato", "D2");
		test.expectNumSamplesAtNoteOn("C2", 2, "legato plays");

		test.setFilter("Legato", "E2");
		test.expectNumSamplesAtNoteOn("C2", 1, "only sustain (legato doesn't match)");

		test.reset("Testing legato interval system");

		{
			TestInstance::LayoutGenerator l(test);
			l.addKeyswitchLayer({ "sus", "spicc" });
			l.addLegatoLayer();
		}
		
		{
			TestInstance::SampleMapGenerator s(test);

			s.addSample("C2_sus_all"); // { { TP, 1 }, { LG, 0xFF } });
			s.addSample("C2_sus_E2"); // { { TP, 1 }, { LG, 55 }});
			s.addSample("C2_sus_D2"); // { { TP, 1 }, { LG, 56 }});
			s.addSample("C2_spicc_all"); // { { TP, 2 }, { LG, 0xFF } });
			s.addSample("C2_spicc_all"); // { { TP, 2 }, { LG, 0xFF } });
			s.addSample("C2_spicc_all"); // { { TP, 2 }, { LG, 0xFF } });
			s.addSample("C2_spicc_all"); // { { TP, 2 }, { LG, 0xFF } });
			s.addSample("C2_spicc_D2"); // { { TP, 2 }, { LG, 56 }});
			s.addSample("C2_spicc_D2"); // { { TP, 2 }, { LG, 56 }});
			s.addSample("C2_spicc_D2"); // { { TP, 2 }, { LG, 56 }});
		}

		// set articulation to sustain
		test.setFilter("Keyswitch", "sus");
		
		// reset legato
		test.setFilter("Legato", "all");
		test.expectNumSamplesAtNoteOn("C2", 1, "only play sustain samples");

		// set legato to key 55
		test.setFilter("Legato", "D2");
		test.expectNumSamplesAtNoteOn("C2", 2, "play legato + sustain");

		test.setFilter("Legato", "F6");
		test.expectNumSamplesAtNoteOn("C2", 1, "non existing legato + sustain");

		// set articulation to spicc
		test.setFilter("Keyswitch", "spicc");

		// clear legato
		test.setFilter("Legato", "all");
		test.expectNumSamplesAtNoteOn("C2", 4, "4 spiccato non legato");

		test.setFilter("Legato", "D2");
		test.expectNumSamplesAtNoteOn("C2", 7, "4 spiccato + 3 legato");
	}

	void testBigPatch()
	{
		TestInstance test(this, "Testing big patch");

		StringArray rrTokens({ "RR1", "RR2", "RR3", "RR4", "RR5" });
		StringArray xfTokens({ "pp", "mp", "mf", "ff" });
		StringArray ksTokens({ "sus", "trem", "stac", "spicc"});
		StringArray sdLayer({ "normal", "sordino" });

		{
			TestInstance::LayoutGenerator l(test);

			l.addKeyswitchLayer(ksTokens);
			l.addLayer("SD", LogicType::Custom, sdLayer, 0);
			l.addRRLayer(rrTokens);
			l.addLayer("XF", LogicType::TableFade, xfTokens, 0);
		}

		{
			TestInstance::SampleMapGenerator s(test);

			for(int i = 48; i < 72; i++)
			{
				auto nn = MidiMessage::getMidiNoteName(i, true, true, 3);

				for(auto ks: ksTokens)
				{
					String ksName = nn + "_" + ks + "_";

					for(auto sd: sdLayer)
					{
						String sdName = ksName + sd + "_";

						if(ks == "stac" || ks == "spicc")
						{
							for(auto rr: rrTokens)
								s.addSample(sdName + rr + "_all");
						}
						else
						{
							for(auto xf: xfTokens)
								s.addSample(sdName + "all_" + xf);
						}
					}
				}
			}
		}

		auto ln = MidiMessage::getMidiNoteName(48, true, true, 3);
		auto un = MidiMessage::getMidiNoteName(71, true, true, 3);
		auto on = MidiMessage::getMidiNoteName(90, true, true, 3);

		test.setFilter("Keyswitch", ksTokens[0]); // sustain
		test.setFilter("SD", sdLayer[0]); // normal
		
		test.expectNumSamplesAtNoteOn(on, 0, "outside note, zero matches");
		test.expectNumSamplesAtNoteOn(ln, xfTokens.size(), "all xfade sustains");
		test.expectNumSamplesAtNoteOn(un, xfTokens.size(), "all xfade sustains");

		test.setFilter("Keyswitch", ksTokens[3]); // spicc

		test.resetPlayingState();

		test.expectSingleSampleAtNoteOn(ln, "C2_spicc_normal_RR1_all", "single staccato sample");
		test.expectSingleSampleAtNoteOn(ln, "C2_spicc_normal_RR2_all", "single staccato sample");
		test.expectSingleSampleAtNoteOn(ln, "C2_spicc_normal_RR3_all", "single staccato sample");
		test.expectSingleSampleAtNoteOn(ln, "C2_spicc_normal_RR4_all", "single staccato sample");
		test.expectSingleSampleAtNoteOn(ln, "C2_spicc_normal_RR5_all", "single staccato sample");
		test.expectSingleSampleAtNoteOn(ln, "C2_spicc_normal_RR1_all", "single staccato sample");
	}

	void testIgnoreFlag()
	{
		static constexpr uint8 IgnoreFlag = 0xff;

		TestInstance test(this, "testing ignore flag");

		{
			TestInstance::LayoutGenerator l(test);

			l.addLayer("RR", LogicType::RoundRobin, { "RR1", "RR2", "RR3 "}, Flags::FlagIgnorable);
			l.addLayer("TP", LogicType::Keyswitch, { "stacc", "sustain"}, Flags::FlagCached);
		}

		{
			TestInstance::SampleMapGenerator s(test);

			s.addSample("C2_RR1_stacc");   // {{ "RR", 1 },          { "TP", 1}});
			s.addSample("C2_all_stacc");   // {{ "RR", IgnoreFlag }, { "TP", 1} });
			s.addSample("C2_all_sustain"); // {{ "RR", IgnoreFlag }, { "TP", 2} });
		}
		
		test.setFilter("TP", "stacc");
		test.expectNumSamplesAtNoteOn("C2", 2, "RR1 should have two elements");
		test.expectNumSamplesAtNoteOn("C2", 1, "RR2 should have one match");

		test.setFilter("TP", "sustain");
		test.expectNumSamplesAtNoteOn("C2", 1, "sustain should ignore the RR flag");

		test.reset("testing two ignorable layers");

		{
			TestInstance::LayoutGenerator l(test);
			l.addLayer("L1", LogicType::Custom, { "X1", "X2", "X3" }, Flags::FlagIgnorable);
			l.addLayer("L2", LogicType::Custom, { "Y1", "Y2", "Y3" }, Flags::FlagIgnorable);
		}

		{
			TestInstance::SampleMapGenerator s(test);

			s.addSample("C2_X1_Y1");  // { { "L1", 1}, { "L2", 1 } });
			s.addSample("C2_X2_Y1");  // { { "L1", 2}, { "L2", 1 } });
			s.addSample("C2_X1_Y2");  // { { "L1", 1}, { "L2", 2 } });
			s.addSample("C2_X2_Y2");  // { { "L1", 2}, { "L2", 2 } });
			s.addSample("C2_all_Y1"); // { { "L1", 0xFF}, { "L2", 1 } });
			s.addSample("C2_X1_all"); // { { "L1", 1},    { "L2", 0xFF } });
			s.addSample("C2_all_Y2"); // { { "L1", 0xFF}, { "L2", 0xFF } });
		}

		test.setFilter("L1", "X1");
		test.setFilter("L2", "Y1");
		test.expectNumSamplesAtNoteOn("C2", 3, "expect 4 notes to play");

		test.setFilter("L1", "X3");
		test.expectNumSamplesAtNoteOn("C2", 1, "expect 1 notes to play (X1_all)");

		test.setFilter("L1", "all");
		test.setFilter("L2", "all");
		test.expectNumSamplesAtNoteOn("C2", 0, "setting both ignore flags should play no samples");
	}

	void testXFade()
	{
		TestInstance test(this, "testing xfade");

		{
			TestInstance::LayoutGenerator l(test);
			l.addLayer("RT", LogicType::Custom, { "sus", "rel" }, Flags::FlagCached);
			l.addLayer("XF", LogicType::XFade, { "pp", "mp", "f" }, 0);
		}
		
		{
			TestInstance::SampleMapGenerator s(test);

			s.addSample("C2_sus_mp");
			s.addSample("C2_rel_all");
		}

		auto v1 = test.getXFadeValue("XF", "C2_sus_mp");
		auto v2 = test.getXFadeValue("XF", "C2_rel_all");

		expectEquals(v1, 0.5f);
		expectEquals(v2, 1.0f);
	}

	void testInstance()
	{
		TestInstance test(this, "testing instance object");

		{
			TestInstance::LayoutGenerator l(test);

			l.addRRLayer({"RR1", "RR2", "RR3", "RR4"});
			l.addKeyswitchLayer({"sus", "trem", "spicc"});
		}

		{
			TestInstance::SampleMapGenerator s(test);

			s.addSample("C2_RR1_sus");
			s.addSample("C2_RR2_sus");
			s.addSample("C2_RR3_sus");
		}
		
		test.setFilter("Keyswitch", "sus");

		test.resetPlayingState();

		test.expectSingleSampleAtNoteOn("C2", "C2_RR1_sus", "RR filter works");
		test.expectSingleSampleAtNoteOn("C2", "C2_RR2_sus", "RR filter works");
		test.expectSingleSampleAtNoteOn("C2", "C2_RR3_sus", "RR filter works");
		test.expectNumSamplesAtNoteOn("C2", 0, "RR4 empty works");
		test.expectSingleSampleAtNoteOn("C2", "C2_RR1_sus", "RR filter works");
		test.expectSingleSampleAtNoteOn("C2", "C2_RR2_sus", "RR filter works");

		test.resetPlayingState();
		test.expectSingleSampleAtNoteOn("C2", "C2_RR1_sus", "reset RR works");

		test.expectNumSamplesAtNoteOn("C#2", 0, "Note mapping doesn't work");
		
		{
			TestInstance::SampleMapGenerator s(test);
			s.addSample("C2_RR1_trem");
			s.addSample("C2_RR1_trem");
		}

		test.resetPlayingState();
		test.setFilter("Keyswitch", "trem");

		test.expectNumSamplesAtNoteOn("C2", 2, "two notes at tremolo RR1");
	}

	void testLegatoInstance()
	{
		TestInstance test(this, "testing simple legato");

		{
			TestInstance::LayoutGenerator l(test);
			l.addLegatoLayer();
			l.addKeyswitchLayer({ "sus", "trem" });
		}
		
		{
			TestInstance::SampleMapGenerator s(test);
			// Add two sustain samples (the all token will set the ignore flag on the legato layer)
			s.addSample("C2_all_sus");
			s.addSample("D2_all_sus");

			// Add a interval from C2 to D2 (the order in the filename is reversed because the target
			// note has to be the first token.
			s.addSample("D2_C2_sus");
		}
		
		test.setFilter("Keyswitch", "sus");

		// "Press" the first note
		auto firstEvent = test.onNoteOn("C2");

		// It should play the first sustain note from C2
		expect(firstEvent.size() == 1, "not a single note");
		expect(firstEvent[0] == "C2_all_sus");

		// Now we set the legato filter to C2 so that it plays the legato intervals
		// starting from C2 (this would be called in the onNoteOn callback of your script)
		test.setFilter("Legato", "C2");
		
		auto secondEvent = test.onNoteOn("D2");

		expect(secondEvent.size() == 2); // Now there are two notes that should be played:
		expect(secondEvent[0] == "D2_all_sus"); // and the sustain from D2
		expect(secondEvent[1] == "D2_C2_sus");  // the legato interval
	}

	void testPurge()
	{
		TestInstance test(this, "testing purge flag");

		{
			TestInstance::LayoutGenerator l(test);
			l.addKeyswitchLayer({ "sustain", "tremolo", "pizz" });
		}

		{
			TestInstance::SampleMapGenerator s(test);
			s.addSample("C2_sustain");
			s.addSample("C2_tremolo");
			s.addSample("C2_pizz");
			s.addSample("C2_pizz");
			s.addSample("C2_pizz");
			s.addSample("C2_pizz");
		}
		
		test.setFilter("Keyswitch", "sustain");
		test.expectNumSamplesAtNoteOn("C2", 1);

		test.setFilter("Keyswitch", "tremolo");
		test.expectNumSamplesAtNoteOn("C2", 1);

		test.setFilter("Keyswitch", "pizz");
		test.expectNumSamplesAtNoteOn("C2", 4);

		test.setPurged("Keyswitch", "pizz", true);
		test.expectNumSamplesAtNoteOn("C2", 0);

		test.setPurged("Keyswitch", "pizz", false);
		test.expectNumSamplesAtNoteOn("C2", 4);

		test.setPurged("Keyswitch", "tremolo", true);
		test.expectNumSamplesAtNoteOn("C2", 4);
	}

	
};

static ComplexGroupManagerTests ct;

} // namespace hise
