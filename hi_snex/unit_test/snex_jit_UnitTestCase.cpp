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
namespace jit {
using namespace juce;

namespace MetaIds
{
	static const juce::String BEGIN_TEST_DATA("BEGIN_TEST_DATA");
	static const Identifier f("f");
	static const Identifier ret("ret");
	static const Identifier args("args");
	static const Identifier input("input");
	static const Identifier output("output");
	static const Identifier error("error");
	static const Identifier voiceindex("voiceindex");
	static const Identifier filename("filename");
	static const Identifier events("events");
	static const Identifier loop_count("loop_count");
	static const Identifier compile_flags("compile_flags");
	static const Identifier voice_index("voice_index");
	static const juce::String END_TEST_DATA("END_TEST_DATA");
}

struct JitFileTestCase::Helpers
{
	static File getAudioFile(const var& v)
	{
		auto filename = parseQuotedString(v);
		return getTestFileDirectory().getChildFile("wave_files").getChildFile(filename);
	}

	static Result testAssemblyLoopCount(const String& assemblyCode, int expectedLoopCount)
	{
		if (expectedLoopCount == -1)
			return Result::ok();

		int numFound = 0;

		for (int i = 0; i < assemblyCode.length(); i++)
		{
			static const String l("loop {");
			if (assemblyCode.substring(i, i + l.length()) == l)
			{
				numFound++;
			}
		}

		if (numFound != expectedLoopCount)
		{
			String f;
			f << "Loop optimisation fail: Expected: " << String(expectedLoopCount) << ", Actual: " << String(numFound);
			return Result::fail(f);
		}

		return Result::ok();
	}

	static AudioSampleBuffer loadFile(const var& v)
	{
		auto f = getAudioFile(v);

		double s = 0.0;
		auto b = hlac::CompressionHelpers::loadFile(f, s);
		return b;
	}

	static Result compareBuffers(AudioSampleBuffer& e, AudioSampleBuffer& a)
	{
		int numChannels = e.getNumChannels();
		jassert(e.getNumSamples() == a.getNumSamples());
		jassert(e.getNumChannels() == a.getNumChannels());
		jassert(numChannels == a.getNumChannels());

		int numSamples = a.getNumSamples();

		for (int i = 0; i < numChannels; i++)
		{
			block a_(a.getWritePointer(i), numSamples);
			block e_(e.getWritePointer(i), numSamples);

			auto r = Helpers::compareBlocks(e_, a_);

			if (!r.wasOk())
				return Result::fail("Channel " + String(i + 1) + ": " + r.getErrorMessage());
		}

		return Result::ok();
	}

	static Result compareBlocks(const block& e, const block& a)
	{
		if (e.size() != a.size())
			return Result::fail(juce::String("size mismatch. Expected: ") + juce::String(e.size()) + ", Actual: " + juce::String(a.size()));

		for (int i = 0; i < e.size(); i++)
		{
			auto ev = e[i];
			auto av = a[i];

			auto error = Decibels::gainToDecibels(std::abs(ev - av));

			if (error > -80.0f)
			{
				juce::String e;
				e << "Delta at [" << juce::String(i) << "]: " << juce::String(error, 1) << " dB. ";
				e << "Actual value: " << juce::String(av, 4) << ", ";
				e << "Expected value: " << juce::String(ev, 4);
				return Result::fail(e);
			}
		}

		return Result::ok();
	}

	static juce::String parseQuotedString(const var& s)
	{
		return s.toString().trim().trimCharactersAtEnd("\"").trimCharactersAtStart("\"");
	}

	static StringArray getStringArray(const juce::String& s, const juce::String& token = juce::String())
	{
		StringArray sa;

		NewLine nl;

		if (token.isEmpty())
			sa = StringArray::fromTokens(s, nl.getDefault(), "[]");
		else
			sa = StringArray::fromTokens(s, token, "\"[]");

		sa.removeEmptyStrings();
		return sa;
	}
};

JitFileTestCase::JitFileTestCase(UnitTest* t_, GlobalScope& memory_, const File& f) :
	code(f.loadFileAsString()),
	file(f),
	r(Result::ok()),
	t(t_),
	memory(memory_),
	c(memory),
	polyHandler(true)
{
	parseFunctionData();
}

JitFileTestCase::JitFileTestCase(GlobalScope& memory_, const juce::String& s) :
	code(s),
	r(Result::ok()),
	t(nullptr),
	memory(memory_),
	c(memory),
	polyHandler(true)
{
	parseFunctionData();
}

JitFileTestCase::~JitFileTestCase()
{

}

hise::HiseEventBuffer JitFileTestCase::parseEventData(const String& s)
{
	HiseEventBuffer e;

	auto v = JSON::parse(s);

	if (auto ar = v.getArray())
	{
		for (auto& o : *ar)
		{
			e.addEvent(parseHiseEvent(o));
		}
	}

	return e;
}

hise::HiseEvent JitFileTestCase::parseHiseEvent(const var& eventObject)
{
	auto type = eventObject["Type"].toString();
	auto channel = (int)eventObject["Channel"];
	auto value1 = (int)eventObject["Value1"];
	auto value2 = (int)eventObject["Value2"];
	auto timestamp = (int)eventObject["Timestamp"];

	HiseEvent e;

	if (type == "NoteOn")
		e = HiseEvent(HiseEvent::Type::NoteOn, value1, value2, channel);
	if (type == "NoteOff")
		e = HiseEvent(HiseEvent::Type::NoteOff, value1, value2, channel);
	if (type == "Controller")
		e = HiseEvent(HiseEvent::Type::Controller, value1, value2, channel);

	if (timestamp % HISE_EVENT_RASTER != 0)
		throw String("Unaligned event: " + JSON::toString(eventObject));

	e.setTimeStamp(timestamp);

	return e;
}

juce::var JitFileTestCase::getJSONData(const HiseEvent& e)
{
	auto eventObject = new DynamicObject();

	auto type = e.getType();


	switch (type)
	{
	case hise::HiseEvent::Type::Empty:
	case hise::HiseEvent::Type::NoteOn: eventObject->setProperty("Type", "NoteOn"); break;
	case hise::HiseEvent::Type::NoteOff: eventObject->setProperty("Type", "NoteOff"); break;
	case hise::HiseEvent::Type::Controller: eventObject->setProperty("Type", "Controller"); break;
	case hise::HiseEvent::Type::PitchBend: eventObject->setProperty("Type", "PitchBend"); break;
	case hise::HiseEvent::Type::Aftertouch:
	case hise::HiseEvent::Type::AllNotesOff:
	case hise::HiseEvent::Type::SongPosition:
	case hise::HiseEvent::Type::MidiStart:
	case hise::HiseEvent::Type::MidiStop:
	case hise::HiseEvent::Type::VolumeFade:
	case hise::HiseEvent::Type::PitchFade:
	case hise::HiseEvent::Type::TimerEvent:
	case hise::HiseEvent::Type::ProgramChange:
	case hise::HiseEvent::Type::numTypes:
		break;
	default:
		break;
	}

	eventObject->setProperty("Channel", e.getChannel());
	eventObject->setProperty("Value1", e.getNoteNumber());
	eventObject->setProperty("Value2", e.getVelocity());
	eventObject->setProperty("Timestamp", e.getTimeStamp());


	return var(eventObject);
}

void JitFileTestCase::logMessage(int level, const juce::String& s)
{
	if (level == BaseCompiler::Error)
		r = Result::fail(s);
}

juce::File JitFileTestCase::getTestFileDirectory()
{
	auto p = File::getSpecialLocation(File::currentApplicationFile);

	p = p.getParentDirectory();

	while (!p.isRoot() && p.isDirectory() && !p.getChildFile("JUCE").isDirectory())
		p = p.getParentDirectory();

	return p.getChildFile("tools/snex_playground/test_files");
}

void JitFileTestCase::setTypeForDynamicFunction(Types::ID t, const String& originalCode)
{
	code = {};
	code << "using T = " << Types::Helpers::getCppTypeName(t) << ";\n";
	code << originalCode;

	function.returnType = TypeInfo(t);

	for (auto& a : function.args)
		a.typeInfo = TypeInfo(t);
}

void JitFileTestCase::initCompiler()
{
	if (voiceIndex != -1)
		memory.setPolyphonic(true);

	c.reset();
	c.setDebugHandler(debugHandler);

	memory.setDebugMode(true);

	if (!nodeId.isValid())
    {
        Types::SnexObjectDatabase::registerObjects(c, 2);
    }
}

juce::Result JitFileTestCase::compileWithoutTesting(bool dumpBeforeTest /*= false*/)
{
	initCompiler();

	if (nodeId.isValid())
	{
		nodeToTest = new JitCompiledNode(c, code, nodeId.toString(), numChannels);

		if (!nodeToTest->r.wasOk())
			return nodeToTest->r;

		if (dumpBeforeTest)
		{
			DBG("code");
			DBG(code);
			DBG(c.dumpSyntaxTree());
			DBG("symbol tree");
			DBG(c.dumpNamespaceTree());
			DBG("assembly");
			DBG(c.getAssemblyCode());
			DBG("data dump");
			DBG(obj.dumpTable());
		}

        assembly = nodeToTest->getAssembly();
        
		return nodeToTest->r;
	}
	else
	{
		obj = c.compileJitObject(code);

		r = c.getCompileResult();
        
		assembly = c.getAssemblyCode();

		if (dumpBeforeTest)
		{
			DBG("code");
			DBG(code);
			DBG(c.dumpSyntaxTree());
			DBG("symbol tree");
			DBG(c.dumpNamespaceTree());
			DBG("assembly");
			DBG(assembly);
			DBG("data dump");
			DBG(obj.dumpTable());
		}

    

		if (r.failed())
			return r;

		auto r = Helpers::testAssemblyLoopCount(assembly, expectedLoopCount);
		if (r.failed() && !memory.getOptimizationPassList().contains(OptimizationIds::AutoVectorisation))
		{
			if (t != nullptr)
				t->expect(false, r.getErrorMessage());

			return r;
		}

		return Result::ok();
	}
}

juce::Result JitFileTestCase::testAfterCompilation(bool dumpBeforeTest /*= false*/)
{
	if (nodeToTest != nullptr)
	{
		inputBuffer = Helpers::loadFile(inputFile);

		Types::PrepareSpecs ps;
		ps.numChannels = inputBuffer.getNumChannels();
		ps.sampleRate = 44100.0;
		ps.blockSize = inputBuffer.getNumSamples();
		ps.voiceIndex = memory.getPolyHandler();

		nodeToTest->prepare(ps);
		nodeToTest->reset();

		Types::ProcessDataDyn d(inputBuffer.getArrayOfWritePointers(), inputBuffer.getNumSamples(), numChannels);
		d.setEventBuffer(eventBuffer);

		auto before = Time::getMillisecondCounterHiRes();

		{
			PolyHandler::ScopedVoiceSetter svs(*memory.getPolyHandler(), voiceIndex);
			nodeToTest->process(d);
		}

		auto after = Time::getMillisecondCounterHiRes();
		auto delta = (after - before) * 0.001;

		auto calculatedSeconds = (double)ps.blockSize / ps.sampleRate;

		cpuUsage = delta / calculatedSeconds;

		auto r = Helpers::testAssemblyLoopCount(assembly, expectedLoopCount);
		if (r.failed() && !memory.getOptimizationPassList().contains(OptimizationIds::AutoVectorisation))
			return r;

		if (outputWasEmpty)
		{
			// Write the output as reference for the next tests...

			DBG("New file created");

			hlac::CompressionHelpers::dump(inputBuffer, outputBufferFile.getFullPathName());

			outputBufferFile.revealToUser();

			return Result::ok();
		}
		else
		{
			auto ok = Helpers::compareBuffers(inputBuffer, outputBuffer);

			if (!ok.wasOk())
			{
				auto f = outputBufferFile.getSiblingFile(outputBufferFile.getFileNameWithoutExtension() + "_error").getNonexistentSibling(false).withFileExtension(".wav");

				//hlac::CompressionHelpers::dump(inputBuffer, f.getFullPathName());
				//f.revealToUser();
			}

			return ok;
		}
	}
	else
	{
		auto compiledF = obj[function.id];

		PolyHandler::ScopedVoiceSetter svs(*memory.getPolyHandler(), voiceIndex);

		if (function.args[0].typeInfo.getType() == Types::ID::Block)
		{
			function.function = compiledF.function;
			auto b = inputs[0].toBlock();

			if (b.begin() != nullptr)
			{
				auto r = function.call<int>(&b);
                jassert(r == 1);
				ignoreUnused(r);
				actualResult = VariableStorage(b);
			}
			else
			{
				r = Result::fail("Can't open input block");
				return r;
			}


		}
		else
		{
			if (!compiledF.matchesArgumentTypes(function))
			{
				r = Result::fail("Compiled function doesn't match test data");
				return r;
			}

			PolyHandler::ScopedVoiceSetter svs(polyHandler, polyVoiceIndex);

			if (auto prep = obj["prepare"])
			{
				PrepareSpecs ps;
				ps.voiceIndex = &polyHandler;

				prep.callVoid(&ps);
			}

			function = compiledF;

			switch (function.returnType.getType())
            {
            case Types::ID::Integer: actualResult = call<int>(); break;
            case Types::ID::Float:   actualResult = call<float>(); break;
            case Types::ID::Double:  actualResult = call<double>(); break;
            default: jassertfalse;
            }

			expectedResult = VariableStorage(function.returnType.getType(), var(expectedResult.toDouble()));
		}

		if (memory.checkRuntimeErrorAfterExecution())
		{
			if (t != nullptr)
				t->expect(false, memory.getRuntimeError().getErrorMessage());

			return memory.getRuntimeError();
		}

		if (dumpBeforeTest)
		{
			DBG("data dump after execution");
			DBG(obj.dumpTable());
		}

		if (expectedFail.isNotEmpty())
			return expectCompileFail(expectedFail);
		else
			return expectValueMatch();
	}
}

juce::Result JitFileTestCase::test(bool dumpBeforeTest /*= false*/)
{
	if (nodeId.isNull() && function.returnType == Types::ID::Dynamic)
	{
		initCompiler();

		String originalCode = code;



		setTypeForDynamicFunction(Types::ID::Integer, originalCode);
		auto ok = test(dumpBeforeTest);

		if (ok.failed())
			return ok;

		setTypeForDynamicFunction(Types::ID::Float, originalCode);
		ok = test(dumpBeforeTest);

		if (ok.failed())
			return ok;

		setTypeForDynamicFunction(Types::ID::Double, originalCode);
		ok = test(dumpBeforeTest);

		if (ok.failed())
			return ok;

		return Result::ok();
	}

	if (r.failed())
		return r;

	if (!requiredCompileFlags.isEmpty())
	{
		auto activeOptimizations = memory.getOptimizationPassList();

		for (auto r : requiredCompileFlags)
		{
			if (!activeOptimizations.contains(r))
			{
				return Result::ok();
			}
		}
	}

	r = compileWithoutTesting(dumpBeforeTest);

	if (!r.wasOk())
		return expectCompileFail(expectedFail);

	return testAfterCompilation(dumpBeforeTest);
}

File JitFileTestCase::save()
{
	if (!r.wasOk())
	{
		AlertWindow::showMessageBox(AlertWindow::WarningIcon, "Can't save file because of error", r.getErrorMessage());
		return File();
	}

	if (fileToBeWritten == File())
	{
		AlertWindow::showMessageBox(AlertWindow::WarningIcon, "Can't save file", "Unspecified name");
		return File();
	}

	if (!fileToBeWritten.existsAsFile() || AlertWindow::showYesNoCancelBox(AlertWindow::QuestionIcon, "Replace test file", "Do you want to replace the test file " + fileToBeWritten.getFullPathName()))
	{
		bool showMessageAfter = !fileToBeWritten.existsAsFile();

		fileToBeWritten.create();
		auto ok = fileToBeWritten.replaceWithText(code);

		if (ok)
			return fileToBeWritten;

		if (showMessageAfter)
			AlertWindow::showMessageBox(AlertWindow::InfoIcon, "Created new test file", fileToBeWritten.getFullPathName());
	}


	return File();
}

void JitFileTestCase::parseFunctionData()
{
	using namespace MetaIds;

	/** BEGIN_TEST_DATA
		f: test
		ret: int
		args: int
		input: 12
		output: 12
		voiceindex: -1
		error: "Line 12: expected result"
		END_TEST_DATA
	*/

	try
	{
		auto metadata = code.fromFirstOccurrenceOf(BEGIN_TEST_DATA, false, false)
			.upToFirstOccurrenceOf(END_TEST_DATA, false, false);

		auto lines = Helpers::getStringArray(metadata);

		if (lines.isEmpty())
			throwError("Can't find metadata");

		NamedValueSet s;

		for (auto l : lines)
		{
			auto t = Helpers::getStringArray(l, ":");

			if (t.size() != 2)
				throwError(l + ": Illegal line");

			auto key = Identifier(t[0].trim());
			auto value = t[1].trim();

			s.set(key, var(value));
		}

		{
			// Parse function id

			auto fId = s[f].toString();

			if (fId.startsWith("{"))
			{
				nodeId = Identifier(fId.removeCharacters("{} \t"));
				isProcessDataTest = true;

				inputFile = Helpers::parseQuotedString(s[input]);

				inputBuffer = Helpers::loadFile(inputFile);

				numChannels = inputBuffer.getNumChannels();

				auto v = Helpers::parseQuotedString(s[output]);
				outputBufferFile = Helpers::getAudioFile(v);

				try
				{
					outputBuffer = Helpers::loadFile(s[output]);
				}
				catch (String& )
				{
					outputBuffer.makeCopyOf(inputBuffer);
					outputWasEmpty = true;
				}

				expectedResult = block(outputBuffer.getWritePointer(0), outputBuffer.getNumSamples());
			}
			else
				function.id = NamespacedIdentifier::fromString(fId);
		}


		if (nodeId.isNull())
		{
			{
				// Parse return type

				auto retType = s[ret].toString();

				if (retType == "T")
					function.returnType = TypeInfo(Types::ID::Dynamic);
				else
					function.returnType = TypeInfo(Types::Helpers::getTypeFromTypeName(retType));

				if (function.returnType == Types::ID::Void)
					throwError("Can't find return type in metadata");
			}

			{
				// Parse arguments
				auto arg = Helpers::getStringArray(s[args], " ");

				int index = 1;

				for (auto a : arg)
				{
					if (a.trim().startsWith("ProcessData"))
					{
						isProcessDataTest = true;
						numChannels = a.fromFirstOccurrenceOf("<", false, false).getIntValue();

						Identifier id("data");

						Types::SnexObjectDatabase::registerObjects(c, numChannels);
						TemplateParameter::List tp;
						tp.add(TemplateParameter(numChannels, TemplateParameter::Single));

						function.addArgs("data", TypeInfo(c.getComplexType(NamespacedIdentifier("ProcessData"), tp)));
					}
					else
					{
						if (a == "T")
						{
							Identifier id(juce::String("p") + juce::String(index));
							function.addArgs(id, TypeInfo(Types::ID::Dynamic));
						}
						else
						{
							auto type = Types::Helpers::getTypeFromTypeName(a.trim());

							if (type == Types::ID::Void)
								throwError("Can't parse argument type " + a);

							Identifier id(juce::String("p") + juce::String(index));
							function.addArgs(id, TypeInfo(type));
						}
					}
				}
			}
			{
				// Parse inputs

				auto inp = Helpers::getStringArray(s[input], " ");

				if (inp.size() != function.args.size())
					throwError("Input amount mismatch");

				for (int i = 0; i < inp.size(); i++)
				{
					auto v = inp[i];

					auto t = function.args[i].typeInfo.getType();

					switch (t)
					{
					case Types::ID::Integer: inputs.add(v.getIntValue());    break;
					case Types::ID::Float:   inputs.add(v.getFloatValue());  break;
					case Types::ID::Double:  inputs.add(v.getDoubleValue()); break;
					case Types::ID::Dynamic: inputs.add(v.getDoubleValue()); break;
					case Types::ID::Block:
					case Types::ID::Pointer:
					{
						try
						{
							inputBuffer = Helpers::loadFile(v);
						}
						catch (String& m)
						{
                            inputBuffer = AudioSampleBuffer();
							throwError(m);
						}

						if (isProcessDataTest && inputBuffer.getNumChannels() != numChannels)
							throwError("Input Channel mismatch: " + String(inputBuffer.getNumChannels()));

						inputs.add(block(inputBuffer.getWritePointer(0), inputBuffer.getNumSamples()));
						break;
					}
					default:				 jassertfalse;
					}
				}
			}
		}

		{
			// Parse error
			expectedFail = Helpers::parseQuotedString(s[error]);
		}

		{
			// Parse voiceIndex;
			if(s.contains(voiceindex))
				polyVoiceIndex = (int)s[voiceindex];
		}

		{
			auto obj = s[events].toString();

			try
			{
				auto buffer = parseEventData(obj);
				eventBuffer.addEvents(buffer);
			}
			catch (String& e)
			{
				t->expect(false, e);
			}
		}

		{
			// Parse compile flags;
			requiredCompileFlags = Helpers::getStringArray(s[compile_flags].toString(), " ");
		}

		{
			auto l = s[loop_count].toString();

			if (!l.isEmpty())
			{
				expectedLoopCount = l.getIntValue();
				requiredCompileFlags.addIfNotAlreadyThere(OptimizationIds::LoopOptimisation);
			}
		}

		if (!file.existsAsFile())
		{
			// Parse output file

			auto outputFileName = Helpers::parseQuotedString(s[filename]);

			if (outputFileName.isNotEmpty())
			{
				if (outputFileName.contains("."))
					throwError("Don't append file extension in output file name");

				fileToBeWritten = getTestFileDirectory().getChildFile(outputFileName).withFileExtension("h");
			}
		}

		if (nodeId.isNull() && expectedFail.isEmpty())
		{
			// Parse output
			auto v = Helpers::parseQuotedString(s[output]);
			auto t = function.returnType.getType();

			if (isProcessDataTest)
				t = Types::ID::Block;
            if(function.args[0].typeInfo.getType() == Types::ID::Block)
                t = Types::ID::Block;

			switch (t)
			{
			case Types::ID::Integer: expectedResult = v.getIntValue();    break;
			case Types::ID::Float:   expectedResult = v.getFloatValue();  break;
			case Types::ID::Double:  expectedResult = v.getDoubleValue(); break;
			case Types::ID::Dynamic: expectedResult = v.getDoubleValue(); break;
			case Types::ID::Block:
			{
				try
				{
					outputBuffer = Helpers::loadFile(v);
					expectedResult = block(outputBuffer.getWritePointer(0), outputBuffer.getNumSamples());
				}
				catch (juce::String& )
				{
					if (inputBuffer.getNumChannels() == 0)
						throwError("Must supply input buffer");

					outputBuffer.makeCopyOf(inputBuffer);
					outputWasEmpty = true;

					outputBufferFile = Helpers::getAudioFile(v);
				}
				break;
			}
			default:				 jassertfalse;
			}
		}
	}
	catch (juce::String& m)
	{
		r = Result::fail(m);
	}
}

juce::Result JitFileTestCase::expectBufferOrProcessDataOK()
{
	if (outputWasEmpty && outputBufferFile != File())
	{
		hlac::CompressionHelpers::dump(inputBuffer, outputBufferFile.getFullPathName());

		if (t != nullptr)
			return r;
		else
			return Result::fail(outputBufferFile.getFullPathName() + " was created");
	}

	if (isProcessDataTest)
	{
		if (actualResult.toInt() != 0)
			return Result::fail("Return value not zero: " + String(actualResult.toInt()));

		return Helpers::compareBuffers(inputBuffer, outputBuffer);
	}
	else
	{
		return Helpers::compareBlocks(expectedResult.toBlock(), actualResult.toBlock());
	}
}

juce::Result JitFileTestCase::expectValueMatch()
{
	if (t != nullptr)
	{
		if (actualResult.getType() == Types::ID::Block || isProcessDataTest)
		{
			auto r = expectBufferOrProcessDataOK();
			t->expect(r.wasOk(), file.getFileName() + ": " + r.getErrorMessage());
		}
		else
		{
			t->expectEquals(Types::Helpers::getCppValueString(actualResult), Types::Helpers::getCppValueString(expectedResult), file.getFileName());
		}

		return r;
	}
	else
	{
		if (r.failed())
			return r;

		if (actualResult.getType() == Types::ID::Block || isProcessDataTest)
		{
			return expectBufferOrProcessDataOK();
		}
		else
		{
			if (!(actualResult == expectedResult))
			{
				juce::String e;

				e << "FAIL: Expected: " << Types::Helpers::getCppValueString(expectedResult);
				e << ", Actual: " << Types::Helpers::getCppValueString(actualResult);
				return Result::fail(e);
			}
		}

		return Result::ok();
	}
}

juce::Result JitFileTestCase::expectCompileFail(const juce::String& errorMessage)
{
	if (t != nullptr)
	{
		t->expectEquals(r.getErrorMessage(), errorMessage, file.getFileName());
		return Result::ok();
	}
	else
	{
		if (r.getErrorMessage() != errorMessage)
		{
			if (errorMessage.isEmpty())
			{
				return r;
			}

			juce::String e;

			e << "FAIL: Expected message: " << errorMessage << "\n";
			e << "Actual message: " << r.getErrorMessage();

			if (r.wasOk())
				e << "[No error message]";

			return Result::fail(e);
		}

		return Result::ok();
	}
}

String JitFileTestCase::HeaderBuilder::operator()()
{
	/*
	BEGIN_TEST_DATA
	  f: {processor}
	  ret: int
	  args: int
	  input: "zero.wav"
	  output: "sine.wav"
	  error: ""
	  filename: "node/node_sine2"
	END_TEST_DATA
	*/

	using namespace MetaIds;

	String s;
	String nl = "\n";

    int numChannels = 2;
	auto id = v[scriptnode::PropertyIds::ID].toString();

	String inputFile;

	inputFile << "zero";
	if (numChannels != 1)
		inputFile << numChannels;
	inputFile << ".wav";

	String outputFile;

	outputFile << "valuetree_nodes/" << id << ".wav";

	String codeFile;

	codeFile << "valuetree_nodes/" << id;

	s << "/*" << nl;
	s << BEGIN_TEST_DATA << nl;
	addDefinition(s, f, "{processor}");
	addDefinition(s, ret, "int");
	addDefinition(s, args, "int");
	addDefinition(s, input, inputFile, true);
	addDefinition(s, output, outputFile, true);
	addDefinition(s, error, "", true);
	addDefinition(s, filename, codeFile, true);
	s << END_TEST_DATA << nl;
	s << "*/" << nl;

	return s;
}

}
}
