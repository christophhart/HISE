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


#include "hi_tools/hi_standalone_components/CodeEditorApiBase.h"

namespace snex {
using namespace juce;

using namespace scriptnode;
DECLARE_PARAMETER_RANGE(FreqRange, 20.0, 20000.0);

String ui::WorkbenchData::getDefaultNodeTemplate(const Identifier& mainClass)
{
#if 0
	auto emitCommentLine = [](juce::String& code, const juce::String& comment)
	{
		code << "/** " << comment << " */\n";
	};

	String s;
	String nl = "\n";
	String emptyBody = "\t{\n\t\t\n\t}\n\t\n";

	s << "template <int NumVoices> struct " << mainClass << nl;

	s << "{" << nl;
	s << "\t" << "void prepare(PrepareSpecs ps)" << nl;
	s << emptyBody;
	s << "\t" << "void reset()" << nl;
	s << emptyBody;
	s << "\t" << "template <typename ProcessDataType> void process(ProcessDataType& data)" << nl;
	s << emptyBody;
	s << "\t" << "template <int C> void processFrame(span<float, C>& data)" << nl;
	s << emptyBody;
	s << "\t" << "void handleHiseEvent(HiseEvent& e)" << nl;
	s << emptyBody;

	s << "};" << nl;
#endif

	using namespace cppgen;

	Base c(Base::OutputType::AddTabs);

	TemplateParameter::List tp;
	tp.add(TemplateParameter(NamespacedIdentifier("NV"), 0, false));

	Struct st(c, mainClass, {}, tp);

	String ld;
	ld << "SNEX_NODE(" << mainClass << ");";
	c << ld;
	c.addEmptyLine();

	c.addComment("Initialise the processing specs here", cppgen::Base::CommentType::Raw);
	c << "void prepare(PrepareSpecs ps)";
	{
		cppgen::StatementBlock sb(c); c.addEmptyLine();
	}

	c.addEmptyLine();
	c.addComment("Reset the processing pipeline here", cppgen::Base::CommentType::Raw);
	c << "void reset()";
	{
		cppgen::StatementBlock sb(c); c.addEmptyLine();
	}

	c.addEmptyLine();
	c.addComment("Process the signal here", cppgen::Base::CommentType::Raw);
	c << "template <typename ProcessDataType> void process(ProcessDataType& data)";
	{
		cppgen::StatementBlock sb(c); c.addEmptyLine();
	}

	c.addEmptyLine();
	c.addComment("Process the signal as frame here", cppgen::Base::CommentType::Raw);
	c << "template <int C> void processFrame(span<float, C>& data)";
	{
		cppgen::StatementBlock sb(c);
	}

	c.addEmptyLine();
	c.addComment("Process the MIDI events here", cppgen::Base::CommentType::Raw);
	c << "void handleHiseEvent(HiseEvent& e)";
	{
		cppgen::StatementBlock sb(c); c.addEmptyLine();
	}

	c.addEmptyLine();
	c.addComment("Use this function to setup the external data", cppgen::Base::CommentType::Raw);
	c << "void setExternalData(const ExternalData& d, int index)";
	{
		cppgen::StatementBlock sb(c); c.addEmptyLine();
	}

	c.addEmptyLine();
	c.addComment("Set the parameters here", cppgen::Base::CommentType::Raw);
	c << "template <int P> void setParameter(double v)";
	{
		cppgen::StatementBlock sb(c); c.addEmptyLine();
	}

	st.flushIfNot();

	return c.toString();
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

void ui::WorkbenchData::callAsyncWithSafeCheck(const std::function<void(WorkbenchData* d)>& f, bool callSyncIfMessageThread)
{
	if (callSyncIfMessageThread && MessageManager::getInstanceWithoutCreating()->isThisTheMessageThread())
	{
		f(this);
		return;
	}
		
	WeakReference<WorkbenchData> safePtr(this);

	MessageManager::callAsync([safePtr, f]()
	{
		if (safePtr.get() != nullptr)
			f(safePtr.get());
	});
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

		callAsyncWithSafeCheck([](WorkbenchData* d) { d->postCompile(); });

		// Might get deleted in the meantime...
		if (compileHandler != nullptr)
		{
            compileHandler->postCompile(lastCompileResult);
            
			callAsyncWithSafeCheck([](WorkbenchData* d) { d->postPostCompile(); });
		}
	}

	return true;
}



snex::ui::WorkbenchData::Ptr ui::WorkbenchManager::getWorkbenchDataForCodeProvider(WorkbenchData::CodeProvider* p, bool ownCodeProvider)
{
	ScopedPointer<WorkbenchData::CodeProvider> owned = p;

	for (auto w : data)
	{
		if (*w == p)
		{
			setCurrentWorkbench(w, true);

			if (!ownCodeProvider)
				owned.release();

			return w;
		}
	}

	WorkbenchData::Ptr w = new WorkbenchData();

	w->setCodeProvider(p, dontSendNotification);

	if (ownCodeProvider)
		codeProviders.add(owned.release());

	data.add(w);
	setCurrentWorkbench(w, true);

	return w;
}

void ui::WorkbenchManager::setCurrentWorkbench(WorkbenchData::Ptr newWorkbench, bool setAsRoot)
{
	if (setAsRoot)
		rootWb = newWorkbench;

	if (currentWb.get() != newWorkbench.get())
	{
		if (newWorkbench == nullptr)
		{
			for (int i = 0; i < codeProviders.size(); i++)
			{
				if (codeProviders[i]->getParent() == currentWb.get())
					codeProviders.remove(i--);
			}

			for (int i = 0; i < data.size(); i++)
			{
				if (data[i].get() == currentWb.get())
					data.remove(i--);
			}
		}

		currentWb = newWorkbench.get();

		triggerAsyncUpdate();
	}
}

void ui::WorkbenchManager::handleAsyncUpdate()
{
	for (auto l : listeners)
	{
		if (l.get() != nullptr)
			l->workbenchChanged(currentWb);
	}
}

void ui::ValueTreeCodeProvider::timerCallback()
{
	auto f = snex::JitFileTestCase::getTestFileDirectory().getChildFile("node.xml");

	if (auto xml = XmlDocument::parse(f))
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

void ui::WorkbenchData::TestData::saveCurrentTestOutput()
{
	auto id = parent.getInstanceId();

	testOutputFile = getTestRootDirectory().getChildFile(id.toString()).withFileExtension("wav");

	if (testOutputFile.existsAsFile())
	{
		AlertWindowLookAndFeel alaf;
		if (!AlertWindow::showOkCancelBox(AlertWindow::QuestionIcon, "Replace file", "Do you want to replace the output file " + testOutputFile.getFullPathName()))
			return;
	}

	hlac::CompressionHelpers::dump(testOutputData, testOutputFile.getFullPathName());
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

	return var(obj.get());
}

bool ui::WorkbenchData::TestData::fromJSON(const var& jsonData, NotificationType runTests)
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

			if (runTests != dontSendNotification)
			{
				sendMessageToListeners(true);
				rebuildTestSignal(sendNotification);
			}
			
			return true;
		}
	}

	return false;
}

void ui::WorkbenchData::TestData::rebuildTestSignal(NotificationType triggerTest)
{
	int size = testSignalLength;

    if(ps.numChannels == 0)
        return;
    
	testSourceData.setSize(ps.numChannels, size);
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
		scriptnode::core::oscillator<1> osc;
		
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
		using PType = parameter::from0To1<core::oscillator<1>, 1, FreqRange>;
		using ProcessorType = container::chain<parameter::empty, wrap::fix<1, core::ramp<1, false>>, wrap::mod<PType, core::peak>, math::clear<1>, core::oscillator<1>>;

		wrap::frame<1, ProcessorType> fObj;

		auto& obj = fObj.getObject();
		obj.get<1>().connect<0>(obj.get<3>());

		auto psCopy = ps;
		psCopy.blockSize = testSourceData.getNumSamples();
		psCopy.numChannels = 1;
		fObj.prepare(psCopy);
		fObj.reset();


		obj.get<0>().setGate(1.0);
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
    {
        for(int i = 1; i < testSourceData.getNumChannels(); i++)
        {
            FloatVectorOperations::copy(testSourceData.getWritePointer(i), testSourceData.getReadPointer(0), size);
        }
    }
		

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

void ui::WorkbenchData::TestData::processInChunks(const std::function<void()>& f)
{

}

Result ui::WorkbenchData::TestData::processTestData(WorkbenchData::Ptr data)
{
	auto tester = customTester.get();

	if (tester == nullptr)
		tester = dynamic_cast<TestRunnerBase*>(data->getCompileHandler());

	auto nodeToTest = data->getLastResult().lastNode;

	// if this fails, the compile handler didn't put the node to the CompileResult::lastNode variable...
	//jassert(!data->getLastResult().compiledOk() || nodeToTest != nullptr);

	tester->initExternalData(this);

	jassert(ps.sampleRate > 0.0);
	ps.numChannels = testSourceData.getNumChannels();

	

	auto r = tester->prepareTest(ps, parameterEvents);
    
    if(!r.wasOk())
        return r;

	testOutputData.makeCopyOf(testSourceData);

	Types::ProcessDataDyn d(testOutputData.getArrayOfWritePointers(), testOutputData.getNumSamples(), testOutputData.getNumChannels());
	d.setEventBuffer(hiseEvents);

	auto before = Time::getMillisecondCounterHiRes();

	{
		ChunkableProcessData<ProcessDataDyn, true> cd(d);

		int samplePos = 0;
		int numLeft = cd.getNumLeft();
		int thisParameterIndex = -1;
		int thisEventIndex = -1;

		while (numLeft > 0)
		{
			auto numThisTime = jmin(ps.blockSize, numLeft);

			auto bufferStart = samplePos;

			Range<int> r(samplePos, samplePos + numThisTime);

			HiseEvent e;
			ParameterEvent p;
			thisParameterIndex = getParameterInSampleRange(r, thisParameterIndex, p);

			thisEventIndex = tester->shouldProcessEventsManually() ? getHiseEventInSampleRange(r, thisEventIndex, e) : -1;

			while(thisParameterIndex != -1 || thisEventIndex != -1)
			{
				auto numBeforeParam = p.timeStamp - samplePos;
				
				if (numBeforeParam > 0 && thisParameterIndex != -1)
				{
					auto sc = cd.getChunk(numBeforeParam);
					tester->processTest(sc.toData());
					samplePos = p.timeStamp;
					numThisTime -= numBeforeParam;
				}

				if (thisParameterIndex != -1 && p.timeStamp != 0)
					tester->processTestParameterEvent(p.parameterIndex, p.valueToUse);

				auto numBeforeEvent = e.getTimeStamp() - samplePos;
				
				if (numBeforeEvent > 0 && thisEventIndex != -1)
				{
					auto sc = cd.getChunk(numBeforeEvent);
					tester->processTest(sc.toData());
					samplePos = e.getTimeStamp();
					numThisTime -= numBeforeEvent;
				}

				if (thisEventIndex != -1)
				{
					e.addToTimeStamp(-bufferStart);
					tester->processHiseEvent(e);
				}

				r = { samplePos, samplePos + numThisTime };

				thisParameterIndex = getParameterInSampleRange(r, thisParameterIndex + 1, p);
				thisEventIndex = tester->shouldProcessEventsManually() ? getHiseEventInSampleRange(r, thisEventIndex, e) : -1;
			}
			
			if (numThisTime > 0)
			{
				auto sc = cd.getChunk(numThisTime);
				tester->processTest(sc.toData());
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
    
    return Result::ok();
}

int ui::WorkbenchData::CompileResult::getNumDebugObjects() const
{
#if SNEX_MIR_BACKEND && SNEX_STANDALONE_PLAYGROUND
	return obj.getNumVariables();
#else
	if (dataPtr == nullptr)
		return 0;

	if (auto st = dynamic_cast<StructType*>(mainClassPtr.get()))
	{
		return st->getNumMembers();
	}

	return 0;
#endif
	
}

struct ValueTreeDebugInfo: public hise::DebugInformationBase
{
    ValueTreeDebugInfo(const ValueTree& v_):
      v(v_)
    {
        
    }
    
    int getNumChildElements() const override
    {
        return v.getNumChildren();
    }
    
    virtual int getType() const
    {
        if (auto obj = getObject())
            return obj->getTypeNumber();

        return 0;
    };

    Ptr getChildElement(int index)
    {
        return new ValueTreeDebugInfo(v.getChild(index));
    }

    virtual String getTextForName() const
    {
        return v["ObjectId"].toString();
    }

    virtual String getCategory() const
    {
        return "";
    }

    virtual DebugableObjectBase::Location getLocation() const
    {
        return DebugableObjectBase::Location();
    }

    virtual String getTextForType() const { return "unknown"; }

    virtual String getTextForDataType() const
    {
        return v["type"].toString();
    }

    virtual String getTextForValue() const
    {
        return v["Value"].toString();
    }

    virtual bool isWatchable() const
    {
        return true;
    }

    virtual bool isAutocompleteable() const
    {
        return false;
    }

    virtual String getCodeToInsert() const
    {
        return "";
    };

    virtual AttributedString getDescription() const
    {
        return AttributedString();
    }
    
    ValueTree v;
};

hise::DebugInformationBase::Ptr ui::WorkbenchData::CompileResult::getDebugInformation(int index)
{
#if SNEX_MIR_BACKEND && SNEX_STANDALONE_PLAYGROUND
	auto c = obj.getDataLayout(index); 
    return new ValueTreeDebugInfo(c);
#else
	if (dataPtr == nullptr)
		return nullptr;

	if (auto st = dynamic_cast<StructType*>(mainClassPtr.get()))
	{
		if (isPositiveAndBelow(index, st->getNumMembers()))
		{
			auto id = st->getMemberName(index);
			auto ptr = reinterpret_cast<uint8*>(dataPtr) + st->getMemberOffset(index);
			return new CompileResult::DataEntry(st->getMemberTypeInfo(id), ptr, id);
		}
	}

	return nullptr;
#endif
}

int ui::WorkbenchData::CompileResult::DataEntry::getType() const
{
	if (type.getTypedIfComplexType<DynType>())
		return (int)DataTypes::Dyn;

	if (type.getTypedIfComplexType<SpanType>())
		return (int)DataTypes::Span;

	if (type.getTypedIfComplexType<StructType>())
		return (int)DataTypes::Struct;

	if (type.getType() == Types::ID::Integer)
		return (int)DataTypes::Integer;

	if (type.getType() == Types::ID::Double)
		return (int)DataTypes::Double;

	if (type.getType() == Types::ID::Float)
		return (int)DataTypes::Float;

	return 0;
}

String ui::WorkbenchData::CompileResult::DataEntry::getTextForValue() const
{
	if (type.isComplexType())
	{
		if (type.toString() == "HiseEvent")
		{
			auto t = static_cast<HiseEvent*>(data)->toDebugString();
			return t.replace("Number", "N").replace("Value", "V").replace("Channel", "C");
		}

		String s;
		s << "0x" << String::toHexString((uint64_t)data).toUpperCase();
		return s;
	}

	return Types::Helpers::getStringFromDataPtr(type.getType(), data);
}

int ui::WorkbenchData::CompileResult::DataEntry::getNumChildElements() const
{
	if (auto st = type.getTypedIfComplexType<StructType>())
	{
		if (st->id == NamespacedIdentifier("HiseEvent"))
		{
			return 0;
		}

		return st->getNumMembers();
	}

	if (auto dyn = type.getTypedIfComplexType<DynType>())
	{
		auto ptr = reinterpret_cast<uint8*>(data);
		auto size = *reinterpret_cast<int*>(ptr + 4);
		return jmin(1024, size);
	}

	if (auto sp = type.getTypedIfComplexType<SpanType>())
	{
		return jmin(1024, sp->getNumElements());
	}

	return 0;
}

hise::DebugInformationBase::Ptr ui::WorkbenchData::CompileResult::DataEntry::getChildElement(int index)
{
	if (auto st = type.getTypedIfComplexType<StructType>())
	{
		if (isPositiveAndBelow(index, st->getNumMembers()))
		{
			auto d = reinterpret_cast<uint8*>(data) + st->getMemberOffset(index);

			String eid = id.toString();

			auto mid = st->getMemberName(index);

			eid << "." << mid;
			return new DataEntry(st->getMemberTypeInfo(mid), d, eid);
		}
	}

	if (auto dyn = type.getTypedIfComplexType<DynType>())
	{
		auto ptr = reinterpret_cast<uint8*>(data);
		auto dynPtr = *reinterpret_cast<uint8**>(ptr + 8);

		auto d = dynPtr + index * dyn->getElementType().getRequiredByteSizeNonZero();

		String eid = id.toString();
		eid << "[" << String(index) << "]";

		return new DataEntry(dyn->getElementType(), d, Identifier(eid));
	}

	if (auto sp = type.getTypedIfComplexType<SpanType>())
	{
		if (isPositiveAndBelow(index, sp->getNumElements()))
		{
			auto d = reinterpret_cast<uint8*>(data) + index * sp->getElementSize();

			String eid = id.toString();
			eid << "[" << String(index) << "]";

			return new DataEntry(sp->getElementType(), d, Identifier(eid));
			
		}
	}

	return nullptr;
}

}
