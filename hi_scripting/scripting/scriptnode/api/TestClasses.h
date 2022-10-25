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

#pragma once

namespace scriptnode {
using namespace juce;
using namespace hise;
using namespace snex;
using namespace snex::ui;

struct ScriptnodeCompileHandlerBase : public snex::ui::WorkbenchData::CompileHandler
{
	ScriptnodeCompileHandlerBase(WorkbenchData* d, DspNetwork* network_);

	WorkbenchData::CompileResult compile(const String& codeToCompile) override;

	void processTestParameterEvent(int parameterIndex, double value) override;

	Result prepareTest(PrepareSpecs ps, const Array<ParameterEvent>& initialParameters) override;

	void initExternalData(ExternalDataHolder* h) override;

	void postCompile(ui::WorkbenchData::CompileResult& lastResult) override;

	Result runTest(ui::WorkbenchData::CompileResult& lastResult) override;

	void processTest(ProcessDataDyn& data);

	bool shouldProcessEventsManually() const;

	void processHiseEvent(HiseEvent& e);;

	virtual PrepareSpecs getPrepareSpecs() const = 0;

	WeakReference<DspNetwork> network;
	Result lastTestResult;
};

struct Buffer2Ascii
{
	enum class WaveformCharacters
	{
		ZeroChar = 0,
		UpChar,
		DownChar,
		FillChar,
		ZeroLineChar,
		VoidChar,
		UpperBounds,
		LowerBounds,
		QuarterMarker,
		numWaveformCharacters
	};

	Buffer2Ascii(var data_, int numLines_);

	void setCharacterSet(const String& newCharacterSet);
	String toString() const;
	Result sanityCheck() const;

private:

	juce_wchar getChannelChar(int channelIndex) const;
	juce_wchar get(WaveformCharacters s) const;

	int getNumSamples() const;;
	int getNumChannels() const;

	VariantBuffer::Ptr getChannel(int index) const;
	bool isMultiChannel() const { return data.isArray(); }
	static String rotateString(String s);
	String printBufferSlice(VariantBuffer::Ptr b, int i, int numSamplesPerLine, int numCols) const;

	int numLines;
	String characterSet;
	var data;
};

/** A test object for a DSP Network. */
struct ScriptNetworkTest : public hise::ConstScriptingObject
{
	struct CProvider : public WorkbenchData::CodeProvider
	{
		CProvider(WorkbenchData::Ptr wb, DspNetwork* n) :
			WorkbenchData::CodeProvider(wb.get()),
			id(n->getId())
		{};

		String loadCode() const override { return {}; }
		bool providesCode() const override { return false; }
		bool saveCode(const String&) override { return true; }
		Identifier getInstanceId() const override { return id; }
		Identifier id;
	};

	struct CHandler : public ScriptnodeCompileHandlerBase
	{
		CHandler(WorkbenchData::Ptr wb, DspNetwork* n);

		PrepareSpecs getPrepareSpecs() const override { return ps; }

		Result prepareTest(PrepareSpecs ps, const Array<ParameterEvent>& initialParameters) override;

		void processTest(ProcessDataDyn& data) override;

		PrepareSpecs ps;

		void addRuntimeFunction(const var& f, int timestamp);

		int waitTimeMs = 0;

	private:

		struct RuntimeFunction
		{
			RuntimeFunction(DspNetwork* n, const var& f_, int timestamp);;
			WeakCallbackHolder f;
			int timestamp;

			JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RuntimeFunction);
		};

		int currentTimestamp = 0;
		OwnedArray<RuntimeFunction> runtimeFunctions;
	};

	ScriptNetworkTest(DspNetwork* n, var testData);;

	Identifier getObjectName() const override { RETURN_STATIC_IDENTIFIER("NetworkTest"); }

	// ================================================================================= API Methods

	/** runs the test and returns the buffer. */
	var runTest();

	/** Sets a test property. */
	void setTestProperty(String id, var value);

	/** Set the processing specifications for the test run. */
	void setProcessSpecs(int numChannels, double sampleRate, int blockSize);

	/** Compares the two data types and returns a error message if they don't match. */
	var expectEquals(var data1, var data2, float errorDb);

	/** Sets a time to wait between calling prepare and processing the data. */
	void setWaitingTime(int timeToWaitMs);

	/** Sets a function that will be executed at the given time to simulate live user input. */
	void addRuntimeFunction(var f, int timestamp);

	/** Creates a XML representation of the current network. */
	String dumpNetworkAsXml();

	/** Creates a string that vaguely represents the buffer data content. */
	String createBufferContentAsAsciiArt(var buffer, int numLines);

	/** Creates a ASCII diff with 'X' as error when the datas don't match. */
	String createAsciiDiff(var data1, var data2, int numLines);

	/** Returns the exception that was caused by the last test run (or empty if fine). */
	String getLastTestException() const;

	/** Returns the list of all compiled nodes. */
	var getListOfCompiledNodes();

	/** Returns the list of all nodes that can be compiled. */
	var getListOfAllCompileableNodes();

	/** Checks whether the hash code of all compiled nodes match their network file. */
	var checkCompileHashCodes();

	/** Returns an object containing the information about the project dll. */
	var getDllInfo();

	// ================================================================================= API Methods

private:

	struct Wrapper;

	snex::ui::WorkbenchData::Ptr wb;

	ScopedPointer<CProvider> cProv;
};



}
