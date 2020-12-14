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

class JitFileTestCase : public DebugHandler
{
public:

	JitFileTestCase(UnitTest* t_, GlobalScope& memory_, const File& f);
	JitFileTestCase(GlobalScope& memory_, const juce::String& s);
	~JitFileTestCase();

	HiseEventBuffer parseEventData(const String& s);
	HiseEvent parseHiseEvent(const var& eventObject);

	

	void logMessage(int level, const juce::String& s) override;

	static File getTestFileDirectory();

	void setTypeForDynamicFunction(Types::ID t, const String& originalCode);

	void initCompiler();

	Result compileWithoutTesting(bool dumpBeforeTest = false);

	Result testAfterCompilation(bool dumpBeforeTest = false);

	Result test(bool dumpBeforeTest = false);

	bool save();

	juce::String assembly;
	DebugHandler* debugHandler = nullptr;
	GlobalScope& memory;
	Compiler c;
	JitObject obj;

private:

	JitCompiledNode::Ptr nodeToTest;

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
		case Types::ID::Block: { auto b = inputs[0].toBlock(); return function.call<R>(&b); break; }
		case Types::ID::Pointer:
		{
			jassert(function.args[0].typeInfo.getTypedComplexType<StructType>()->id == NamespacedIdentifier("ProcessData"));
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
	File fileToBeWritten;
	Result r;
	juce::String code;
	FunctionData function;

	Identifier nodeId;


	bool isProcessDataTest = false;
	bool outputWasEmpty = false;
	int numChannels = -1;

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
	int expectedLoopCount = -1;

};

}
}