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


namespace snex {
using namespace juce;

DECLARE_PARAMETER_RANGE(FreqRange, 20.0, 20000.0);

String ui::WorkbenchData::getDefaultNodeTemplate(const Identifier& mainClass)
{
	auto emitCommentLine = [](juce::String& code, const juce::String& comment)
	{
		code << "/** " << comment << " */\n";
	};

	String s;
	String nl = "\n";
	String emptyBody = "\t{\n\t\t\n\t}\n\t\n";

	s << "struct " << mainClass << nl;

	s << "{" << nl;
	s << "\t" << "void prepare(PrepareSpecs ps)" << nl;
	s << emptyBody;
	s << "\t" << "void reset()" << nl;
	s << emptyBody;
	s << "\t" << "template <typename ProcessDataType> void process(ProcessDataType& data)" << nl;
	s << emptyBody;
	s << "\t" << "template <int C> void processFrame(span<float, C>& data)" << nl;
	s << emptyBody;
	s << "\t" << "void handleEvent(HiseEvent& e)" << nl;
	s << emptyBody;

	s << "};" << nl;

	return s;
}

String ui::WorkbenchData::convertToLogMessage(int level, const String& s)
{
	juce::String m;

	switch (level)
	{
	case jit::BaseCompiler::Error:  m << "ERROR: "; break;
	case jit::BaseCompiler::Warning:  m << "WARNING: "; break;
	case jit::BaseCompiler::PassMessage: return {};// m << "PASS: "; break;
	case jit::BaseCompiler::ProcessMessage: return {};// m << "- "; break;
	case jit::BaseCompiler::VerboseProcessMessage: m << "-- "; break;
	case jit::BaseCompiler::AsmJitMessage: m << "OUTPUT "; break;
	case jit::BaseCompiler::Blink:	m << "BLINK at line ";
	default: break;
	}

	m << getInstanceId() << ":";

	m << s;

	return m;
}

void ui::WorkbenchData::handleBlinks()
{
	for (auto p : pendingBlinks)
	{
		for (auto l : listeners)
		{
			if (l.get() != nullptr)
				l->logMessage(this, BaseCompiler::MessageType::Blink, String(p));
		}
	}

	pendingBlinks.clearQuick();
}

bool ui::WorkbenchData::handleCompilation()
{
	if (getGlobalScope().getBreakpointHandler().shouldAbort())
		return true;

	if (compileHandler != nullptr)
	{
		auto s = getCode();

		if (codeProvider != nullptr)
			codeProvider->preprocess(s);

		for (auto l : listeners)
		{
			if (l != nullptr)
				l->preprocess(s);
		}

		if (getGlobalScope().getBreakpointHandler().shouldAbort())
			return true;

		lastCompileResult = compileHandler->compile(s);

		MessageManager::callAsync(BIND_MEMBER_FUNCTION_0(WorkbenchData::postCompile));

		compileHandler->postCompile(lastCompileResult);
		
		// Might get deleted in the meantime...
		if (compileHandler != nullptr)
		{
			MessageManager::callAsync(BIND_MEMBER_FUNCTION_0(WorkbenchData::postPostCompile));
		}
	}

	return true;
}

void ui::WorkbenchManager::workbenchChanged(WorkbenchData::Ptr oldWorkBench, WorkbenchData::Ptr newWorkbench)
{
	jassert(oldWorkBench.get() == currentWb.get());

	currentWb = newWorkbench.get();

	if (currentWb != nullptr)
		logMessage(currentWb.get(), jit::BaseCompiler::VerboseProcessMessage, "Switched to " + currentWb->getInstanceId());
}



void ui::ValueTreeCodeProvider::timerCallback()
{
	auto f = snex::JitFileTestCase::getTestFileDirectory().getChildFile("node.xml");

	if (ScopedPointer<XmlElement> xml = XmlDocument::parse(f))
	{
		ValueTree v = ValueTree::fromXml(*xml);

		if (!lastTree.isEquivalentTo(v))
		{
			customCode = {};
			lastTree = v;
			getParent()->triggerRecompile();
		}
	}
}

juce::Identifier ui::ValueTreeCodeProvider::getInstanceId() const
{
	return Identifier(lastTree[scriptnode::PropertyIds::ID].toString());
}

void ui::ValueTreeCodeProvider::rebuild() const
{
	snex::cppgen::ValueTreeBuilder vb(lastTree, cppgen::ValueTreeBuilder::Format::JitCompiledInstance);
	lastResult = vb.createCppCode();
}

namespace TestDataIds
{
#define DECLARE_ID(x) const Identifier x(#x);
	DECLARE_ID(SignalType);
	DECLARE_ID(SignalLength);
	DECLARE_ID(NodeId);
	DECLARE_ID(ParameterEvents);
	DECLARE_ID(HiseEvents);
	DECLARE_ID(InputFile);
	DECLARE_ID(OutputFile);

#undef DECLARE_ID
}

juce::var ui::WorkbenchData::TestData::toJSON() const
{
	DynamicObject::Ptr obj = new DynamicObject();

	obj->setProperty(TestDataIds::NodeId, parent.getInstanceId().toString());
	obj->setProperty(TestDataIds::SignalType, getSignalTypeList()[(int)currentTestSignalType]);
	obj->setProperty(TestDataIds::SignalLength, testSignalLength);
	obj->setProperty(TestDataIds::InputFile, testInputFile.getFullPathName());
	obj->setProperty(TestDataIds::OutputFile, testOutputFile.getFullPathName());

	Array<var> eventList;

	for (auto e : hiseEvents)
		eventList.add(JitFileTestCase::getJSONData(e));

	Array<var> parameterList;

	for (auto p : parameterEvents)
		parameterList.add(p.toJson());

	ExternalData::forEachType([&](ExternalData::DataType t)
	{
		Array<var> data;

		auto numObjects = getNumDataObjects(t);

		for (int i = 0; i < numObjects; i++)
		{
			String b64;

			if (t == ExternalData::DataType::Table && isPositiveAndBelow(i, tables.size()))
				b64 = tables[i]->exportData();
			if (t == ExternalData::DataType::SliderPack && isPositiveAndBelow(i, sliderPacks.size()))
				b64 = sliderPacks[i]->toBase64();

			if (t == ExternalData::DataType::AudioFile)
				jassertfalse; // soon

			if(b64.isNotEmpty())
				data.add(b64);
		}

		String id = ExternalData::getDataTypeName(t);
		id << "s";

		obj->setProperty(id, data);
	});


	obj->setProperty(TestDataIds::HiseEvents, eventList);
	obj->setProperty(TestDataIds::ParameterEvents, parameterList);

	return var(obj);
}

bool ui::WorkbenchData::TestData::fromJSON(const var& jsonData)
{
	if (auto obj = jsonData.getDynamicObject())
	{
		auto& o = obj->getProperties();
		auto id = Identifier(o.getWithDefault(TestDataIds::NodeId, "").toString());

		if (parent.getInstanceId() == id)
		{
			clear();

			testSignalLength = o.getWithDefault(TestDataIds::SignalLength, 1024);

			auto signalName = o.getWithDefault(TestDataIds::SignalType, "Empty").toString();
			auto index = getSignalTypeList().indexOf(signalName);
			index = jmax(0, index);
			currentTestSignalType = (TestSignalMode)index;
			testInputFile = File(o.getWithDefault(TestDataIds::InputFile, ""));
			testOutputFile = File(o.getWithDefault(TestDataIds::OutputFile, ""));

			auto e = o.getWithDefault(TestDataIds::HiseEvents, var());
			auto p = o.getWithDefault(TestDataIds::ParameterEvents, var());

			ExternalData::forEachType([&](ExternalData::DataType t)
			{
				String id = ExternalData::getDataTypeName(t);
				id << "s";

				auto v = o.getWithDefault(id, var());

				if (auto vList = v.getArray())
				{
					for (auto b64_ : *vList)
					{
						auto b64 = b64_.toString();

						if (b64.isNotEmpty())
						{
							int numExisting = getNumDataObjects(t);

							if (t == ExternalData::DataType::Table)
							{
								auto table = getTable(numExisting);
								table->restoreData(b64);
							}
							if (t == ExternalData::DataType::SliderPack)
							{
								auto sp = getSliderPack(numExisting);
								sp->fromBase64(b64);
							}
						}
					}
				}
			});

			if (auto eList = e.getArray())
			{
				for (auto ev : *eList)
					hiseEvents.addEvent(JitFileTestCase::parseHiseEvent(ev));
			}

			if (auto pList = p.getArray())
			{
				for (auto pe : *pList)
					parameterEvents.add(ParameterEvent(pe));
			}

			sendMessageToListeners(true);
			rebuildTestSignal(sendNotification);

			return true;
		}
	}

	return false;
}

void ui::WorkbenchData::TestData::rebuildTestSignal(NotificationType triggerTest)
{
	int size = testSignalLength;

	testSourceData.setSize(2, size);

	testSourceData.clear();

	switch (currentTestSignalType)
	{
	case TestSignalMode::Empty:
		break;
	case TestSignalMode::DC1:
	{
		FloatVectorOperations::fill(testSourceData.getWritePointer(0), 1.0f, size);
		break;
	}
	case TestSignalMode::FastRamp:
	case TestSignalMode::Ramp:
	{
		float delta = 1.0f / (float)size;

		if (currentTestSignalType == TestSignalMode::FastRamp)
			delta *= 32.0f;

		for (int i = 0; i < size; i++)
			testSourceData.setSample(0, i, hmath::fmod(delta * (float)i, 1.0f));
		break;
	}
	case TestSignalMode::Impulse:
	{
		testSourceData.setSample(0, 0, 1.0f);
		break;
	}
	case TestSignalMode::OneKhzSine:
	case TestSignalMode::OneKhzSaw:
	{
		scriptnode::core::oscillator osc;
		
		auto psCopy = ps;
		psCopy.numChannels = 1;
		osc.prepare(psCopy);
		osc.reset();

		if (currentTestSignalType == TestSignalMode::OneKhzSaw)
			osc.getWrappedObject().setMode(1.0);

		osc.getWrappedObject().setFrequency(1000.0);
		ProcessDataDyn d(testSourceData.getArrayOfWritePointers(), size, 1);
		osc.process(d);
		break;
	}
	case TestSignalMode::SineSweep:
	{
		using namespace scriptnode;
		using PType = parameter::from0To1<core::oscillator, 1, FreqRange>;
		using ProcessorType = container::chain<parameter::empty, wrap::fix<1, core::ramp>, wrap::mod<PType, core::peak>, math::clear, core::oscillator>;

		wrap::frame<1, ProcessorType> fObj;

		auto& obj = fObj.getObject();
		obj.get<1>().connect<0>(obj.get<3>());

		auto psCopy = ps;
		psCopy.blockSize = testSourceData.getNumSamples();
		psCopy.numChannels = 1;
		fObj.prepare(psCopy);
		fObj.reset();
		obj.get<0>().setPeriodTime(1000.0 * (double)psCopy.blockSize / ps.sampleRate);

		ProcessData<1> d(testSourceData.getArrayOfWritePointers(), size, 1);
		fObj.process(d);
		break;
	}
	case TestSignalMode::Noise:
	{
		for (int i = 0; i < size; i++)
			testSourceData.setSample(0, i, hmath::random() * 2.0f - 1.0f);

		break;
	}
	case TestSignalMode::CustomFile:
	{
		if (!testInputFile.existsAsFile())
		{
			FileChooser fc("Load test file", testRootDirectory, "*.wav", true);

			if (fc.browseForFileToOpen())
				testInputFile = fc.getResult();
		}

		if (testInputFile.existsAsFile())
		{
			double speed = 0.0;
			auto b = hlac::CompressionHelpers::loadFile(testInputFile, speed);

			if (b.getNumSamples() != 0)
				testSourceData.makeCopyOf(b);
		}

		break;
	}
	case TestSignalMode::numTestSignals:
		break;
	default:
		break;
	}

	if(currentTestSignalType != TestSignalMode::CustomFile)
		FloatVectorOperations::copy(testSourceData.getWritePointer(1), testSourceData.getReadPointer(0), size);

	for (auto l : listeners)
	{
		if (l != nullptr)
			l->testSignalChanged();
	}

	if (triggerTest != dontSendNotification)
	{
		parent.triggerPostCompileActions();
	}
}

void ui::WorkbenchData::TestData::processTestData(WorkbenchData::Ptr data)
{
	auto compileHandler = data->getCompileHandler();
	auto nodeToTest = data->getLastResult().lastNode;

	// if this fails, the compile handler didn't put the node to the CompileResult::lastNode variable...
	//jassert(!data->getLastResult().compiledOk() || nodeToTest != nullptr);

	if(nodeToTest != nullptr)
		nodeToTest->setExternalDataHolder(this);

	jassert(ps.sampleRate > 0.0);
	ps.numChannels = testSourceData.getNumChannels();

	compileHandler->prepareTest(ps);

	testOutputData.makeCopyOf(testSourceData);

	Types::ProcessDataDyn d(testOutputData.getArrayOfWritePointers(), testOutputData.getNumSamples(), testOutputData.getNumChannels());
	d.setEventBuffer(hiseEvents);

	auto before = Time::getMillisecondCounterHiRes();

	{
		ChunkableProcessData<ProcessDataDyn, true> cd(d);

		int samplePos = 0;
		int numLeft = cd.getNumLeft();
		int thisParameterIndex = -1;

		while (numLeft > 0)
		{
			auto numThisTime = jmin(ps.blockSize, numLeft);

			Range<int> r(samplePos, samplePos + numThisTime);

			ParameterEvent p;

			thisParameterIndex = getParameterInSampleRange(r, thisParameterIndex, p);

			while(thisParameterIndex != -1)
			{
				auto numBeforeParam = p.timeStamp - samplePos;

				if (numBeforeParam > 0)
				{
					auto sc = cd.getChunk(numBeforeParam);
					compileHandler->processTest(sc.toData());

					samplePos = p.timeStamp;
					numThisTime -= numBeforeParam;
				}

				compileHandler->processTestParameterEvent(p.parameterIndex, p.valueToUse);

				r = { samplePos, samplePos + numThisTime };

				thisParameterIndex = getParameterInSampleRange(r, thisParameterIndex + 1, p);
			}
			
			if (numThisTime > 0)
			{
				auto sc = cd.getChunk(numThisTime);
				compileHandler->processTest(sc.toData());
				samplePos += numThisTime;
			}

			numLeft = testOutputData.getNumSamples() - samplePos;
		}

		jassert(cd.getNumLeft() == 0);
	}

	auto after = Time::getMillisecondCounterHiRes();
	auto delta = (after - before) * 0.001;

	auto calculatedSeconds = (double)testOutputData.getNumSamples() / ps.sampleRate;

	cpuUsage = delta / calculatedSeconds;
}

}
