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

#pragma once

namespace snex {
namespace jit {
using namespace juce;

struct CppGenerator
{
	static void wrapInNamespace(String& code, const Identifier& ns, const String& suffix)
	{
		String s;
		s << "namespace " << ns << suffix << "{\n";
		s << code;
		s << "\n}\n";
		
		std::swap(s, code);
	}
};

class JitFileTestCase : public DebugHandler
{
public:

	JitFileTestCase(UnitTest* t_, GlobalScope& memory_, const File& f);
	JitFileTestCase(GlobalScope& memory_, const juce::String& s);
	~JitFileTestCase();

	HiseEventBuffer parseEventData(const String& s);
	static HiseEvent parseHiseEvent(const var& eventObject);

	static var getJSONData(const HiseEvent& e);

	struct HeaderBuilder
	{
		HeaderBuilder(const ValueTree& v_) :
			v(v_)
		{};

		String operator()();

	private:

		void addDefinition(String& s, const Identifier& key, const String& value, bool quoted = false)
		{
			s << "  " << key.toString() << ": ";
			
			if (quoted)
				s << value.quoted();
			else
				s << value;
			
			s << "\n";
		}

		ValueTree v;
	};
	

	void logMessage(int level, const juce::String& s) override;

	static File getTestFileDirectory();

	void setTypeForDynamicFunction(Types::ID t, const String& originalCode);

	void initCompiler();

	NamespacedIdentifier getCppPath()
	{
		auto path = file.getRelativePathFrom(getTestFileDirectory()).replaceCharacter('\\', '/');

		path = path.replace("00 ", "");
		path = path.removeCharacters(" ");
		
		if (path.getIntValue() != 0)
			path = path.fromFirstOccurrenceOf("_", false, false);


		
		return NamespacedIdentifier::fromString(path.replace("/", "_test::").upToFirstOccurrenceOf(".h", false, false));
	}

	bool canBeTestedAsCpp()
	{
		if (file.getFileName().startsWith("0"))
			return false;

		if (expectedFail.isNotEmpty())
			return false;

		for (int i = 0; i < function.args.size(); i++)
		{
			auto iType = function.args[i].typeInfo.getType();

			if (iType == Types::ID::Block ||
				iType == Types::ID::Dynamic)
				return false;
		}

		auto oType = expectedResult.getType();
		
		return oType != Types::ID::Block && oType != Types::ID::Dynamic;
	}

	String convertToCppTestCode()
	{
		if (!canBeTestedAsCpp())
			return {};

		auto id = getCppPath();

		String s;

		s << 

		s << "        expectEquals(" << id.toString() << "::" << function.id.toString() << "(";

		for (int i = 0; i < inputs.size(); i++)
		{
			s << Types::Helpers::getCppValueString(inputs[i]);

			if (i != inputs.size() - 1)
				s << ", ";
		}

		s << "), " << Types::Helpers::getCppValueString(expectedResult) << ", \"";
		
		s << file.getRelativePathFrom(getTestFileDirectory()).replaceCharacter('\\', '/') << "\");\n";

		return s;
	}

	String convertToIncludeableCpp()
	{
		if (!canBeTestedAsCpp())
			return {};

		auto id = getCppPath();

		auto className = id.getIdentifier();

		auto namespaces = id.getParent();

		String c;

		c << "namespace " << className << "\n{";
		c << code.fromFirstOccurrenceOf("*/", false, false);
		c << "\n};\n";

		for (auto& ns : id.getParent().getIdList())
		{
			CppGenerator::wrapInNamespace(c, ns, "");
		}
		
		return c;
	}

	Result compileWithoutTesting(bool dumpBeforeTest = false);

	Result testAfterCompilation(bool dumpBeforeTest = false);

	Result test(bool dumpBeforeTest = false);

	AudioSampleBuffer getBuffer(bool getProcessed) const
	{
		return inputBuffer;// getProcessed ? outputBuffer : inputBuffer;
	}

	File save();

	juce::String assembly;
	DebugHandler* debugHandler = nullptr;
	GlobalScope& memory;
	Compiler c;
	JitObject obj;
    
    JitCompiledNode::Ptr nodeToTest;

	double cpuUsage = 0.0;

private:

	PolyHandler polyHandler;

	String inputFile;
	

	void parseFunctionData();

	struct Helpers;
	

	Result expectBufferOrProcessDataOK();

	Result expectValueMatch();

	Result expectCompileFail(const juce::String& errorMessage);

	template <typename R> R call()
	{
		switch (function.args.size())
		{
		case 0: return function.call<R>();
		case 1: return call1<R>();
		case 2: return call2<R>();
		default: jassertfalse; return R();
		}
	}

	template <typename R> R call1()
	{
		switch (function.args[0].typeInfo.getType())
		{
		case Types::ID::Integer: return function.call<R>(inputs[0].toInt()); break;
		case Types::ID::Float:   return function.call<R>(inputs[0].toFloat()); break;
		case Types::ID::Double:  return function.call<R>(inputs[0].toDouble()); break;
		case Types::ID::Block: 
		{ 
			jassertfalse;
		}
		case Types::ID::Pointer:
		{
			jassert(inputBuffer.getNumSamples() > 0);
			jassert(inputBuffer.getNumChannels() == numChannels);

			auto channels = inputBuffer.getArrayOfWritePointers();

			Types::ProcessDataDyn p(channels, inputBuffer.getNumSamples(), numChannels);

			p.setEventBuffer(eventBuffer);

			return function.call<R>(&p);

		}
		default: jassertfalse; return R();
		}
	}

	template <typename R> R call2()
	{
		switch (function.args[0].typeInfo.getType())
		{
		case Types::ID::Integer: return call2With1<R, int>(inputs[0].toInt()); break;
		case Types::ID::Float:   return call2With1<R, float>(inputs[0].toFloat()); break;
		case Types::ID::Double:  return call2With1<R, double>(inputs[0].toDouble()); break;
		default: jassertfalse; return R();
		}
	}

	template <typename R, typename A1> R call2With1(A1 arg1)
	{
		switch (function.args[1].typeInfo.getType())
		{
		case Types::ID::Integer: return function.call<R>(arg1, inputs[1].toInt()); break;
		case Types::ID::Float:   return function.call<R>(arg1, inputs[1].toFloat()); break;
		case Types::ID::Double:  return function.call<R>(arg1, inputs[1].toDouble()); break;
		default: jassertfalse; return R();
		}
	}

	void throwError(const juce::String& s)
	{
		throw s;
	}

	File file;
	File fileToBeWritten ;
	Result r;
	juce::String code;
	FunctionData function;

	Identifier nodeId;


	bool isProcessDataTest = false;
	bool outputWasEmpty = false;
	int numChannels = -1;
	int voiceIndex = -1;

	UnitTest* t;
	Array<VariableStorage> inputs;
	VariableStorage expectedResult;
	VariableStorage actualResult;
	juce::String expectedFail;
	AudioSampleBuffer inputBuffer;
	AudioSampleBuffer outputBuffer;
	File outputBufferFile;
	HiseEventBuffer eventBuffer;
	StringArray requiredCompileFlags;
	int polyVoiceIndex = -1;
	int expectedLoopCount = -1;

};

}
}
