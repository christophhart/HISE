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

namespace scriptnode {
using namespace juce;
using namespace hise;

struct ScriptNetworkTest::Wrapper
{
	API_METHOD_WRAPPER_0(ScriptNetworkTest, runTest);
	API_VOID_METHOD_WRAPPER_2(ScriptNetworkTest, setTestProperty);
	API_VOID_METHOD_WRAPPER_3(ScriptNetworkTest, setProcessSpecs);
	API_METHOD_WRAPPER_3(ScriptNetworkTest, expectEquals);
	API_METHOD_WRAPPER_0(ScriptNetworkTest, dumpNetworkAsXml);
	API_VOID_METHOD_WRAPPER_1(ScriptNetworkTest, setWaitingTime);
	API_METHOD_WRAPPER_0(ScriptNetworkTest, getLastTestException);
	API_METHOD_WRAPPER_2(ScriptNetworkTest, createBufferContentAsAsciiArt);
	API_METHOD_WRAPPER_0(ScriptNetworkTest, getListOfCompiledNodes);
	API_METHOD_WRAPPER_0(ScriptNetworkTest, getListOfAllCompileableNodes);
	API_METHOD_WRAPPER_0(ScriptNetworkTest, checkCompileHashCodes);
	API_METHOD_WRAPPER_3(ScriptNetworkTest, createAsciiDiff);
	API_METHOD_WRAPPER_0(ScriptNetworkTest, getDllInfo);
	API_VOID_METHOD_WRAPPER_2(ScriptNetworkTest, addRuntimeFunction);
};

ScriptNetworkTest::ScriptNetworkTest(DspNetwork* n, var testData) :
	ConstScriptingObject(n->getScriptProcessor(), 0),
	wb(new snex::ui::WorkbenchData())
{
	wb->setCompileHandler(new CHandler(wb, n));
	cProv = new CProvider(wb, n);
	wb->setCodeProvider(cProv);
	wb->getTestData().fromJSON(testData, dontSendNotification);

	ADD_API_METHOD_0(runTest);
	ADD_API_METHOD_2(setTestProperty);
	ADD_API_METHOD_3(setProcessSpecs);
	ADD_API_METHOD_3(expectEquals);
	ADD_API_METHOD_0(dumpNetworkAsXml);
	ADD_API_METHOD_1(setWaitingTime);
	ADD_API_METHOD_0(getLastTestException);
	ADD_API_METHOD_2(createBufferContentAsAsciiArt);
	ADD_API_METHOD_3(createAsciiDiff);
	ADD_API_METHOD_0(getListOfCompiledNodes);
	ADD_API_METHOD_0(getListOfAllCompileableNodes);
	ADD_API_METHOD_0(checkCompileHashCodes);
	ADD_API_METHOD_0(getDllInfo);
	ADD_API_METHOD_2(addRuntimeFunction);
}

String ScriptNetworkTest::getLastTestException() const
{
	if (auto b = dynamic_cast<CHandler*>(wb->getCompileHandler()))
	{
		return b->lastTestResult.getErrorMessage();
	}

	jassertfalse;
	return {};
}

var ScriptNetworkTest::getListOfCompiledNodes()
{
	auto mg = dynamic_cast<BackendProcessor*>(getScriptProcessor()->getMainController_())->dllManager;

	Array<var> list;

	if (mg != nullptr && mg->projectDll != nullptr)
	{
		int numNodes = mg->projectDll->getNumNodes();

		for (int i = 0; i < numNodes; i++)
		{
			list.add(var(mg->projectDll->getNodeId(i)));
		}
	}

	return var(list);
}

juce::var ScriptNetworkTest::getListOfAllCompileableNodes()
{
	Array<var> list;

	auto mg = dynamic_cast<BackendProcessor*>(getScriptProcessor()->getMainController_())->dllManager;

	if (mg != nullptr)
	{
		auto fileList = mg->getNetworkFiles(getScriptProcessor()->getMainController_(), false);

		for (auto f : fileList)
		{
			list.add(var(f.getFileNameWithoutExtension()));
		}

	}

	return var(list);
}

var ScriptNetworkTest::checkCompileHashCodes()
{
	auto mg = dynamic_cast<BackendProcessor*>(getScriptProcessor()->getMainController_())->dllManager;

	if (mg == nullptr)
		return var("No DLL manager can be found");

	if (mg != nullptr)
	{
		auto fileList = mg->getNetworkFiles(getScriptProcessor()->getMainController_(), false);

		if (mg->projectDll == nullptr)
			return var("The DLL was not loaded");

		if (mg->projectDll != nullptr)
		{
			int numNodes = mg->projectDll->getNumNodes();

			for (int i = 0; i < numNodes; i++)
			{
				auto id = mg->projectDll->getNodeId(i);
				auto fileHash = mg->getHashForNetworkFile(getScriptProcessor()->getMainController_(), id);

				auto compileHash = mg->projectDll->getHash(i);

				if (fileHash != compileHash)
				{
					String errorMessage;
					errorMessage << "Hash mismatch for node " << id << ": " << String(fileHash) << " -> " << String(compileHash);
					return var(errorMessage);
				}
			}
		}
	}

	return var(0);
}

juce::var ScriptNetworkTest::getDllInfo()
{
	auto mg = dynamic_cast<BackendProcessor*>(getScriptProcessor()->getMainController_())->dllManager;

	return mg->getStatistics();
}

juce::var ScriptNetworkTest::runTest()
{
	wb->triggerRecompile();
	auto& td = wb->getTestData();

	if (auto b = dynamic_cast<CHandler*>(wb->getCompileHandler()))
	{
		if (b->lastTestResult.wasOk() && !b->network->getExceptionHandler().isOk())
		{
			// A runtime error occured!
			b->lastTestResult = Result::fail(b->network->getExceptionHandler().getErrorMessage());
		}
	}

	auto numChannels = td.testOutputData.getNumChannels();

	if (numChannels == 1)
	{

		auto v = new VariantBuffer(td.testOutputData.getNumSamples());
		FloatVectorOperations::copy(v->buffer.getWritePointer(0), td.testOutputData.getReadPointer(0), v->size);
		return var(v);
	}
	else
	{
		Array<var> channels;

		for (int i = 0; i < numChannels; i++)
		{
			auto v = new VariantBuffer(td.testOutputData.getNumSamples());
			FloatVectorOperations::copy(v->buffer.getWritePointer(0), td.testOutputData.getReadPointer(i), v->size);

			channels.add(var(v));
		}

		return var(channels);
	}
}

String ScriptNetworkTest::dumpNetworkAsXml()
{
	auto v = dynamic_cast<CHandler*>(wb->getCompileHandler())->network->getValueTree();
	auto copy = v.createCopy();

#if USE_BACKEND
	DspNetworkListeners::PatchAutosaver::removeDanglingConnections(copy);
	DspNetworkListeners::PatchAutosaver::stripValueTree(copy);
#endif

	auto xml = copy.createXml();

	return xml->createDocument("");
}




String ScriptNetworkTest::createBufferContentAsAsciiArt(var buffer, int numLines)
{
	Buffer2Ascii bf(buffer, numLines);

	auto r = bf.sanityCheck();

	if (r.failed())
	{
		reportScriptError(r.getErrorMessage());
		RETURN_IF_NO_THROW({});
	}

	return bf.toString();
}

String ScriptNetworkTest::createAsciiDiff(var data1, var data2, int numLines)
{
	Buffer2Ascii b1(data1, numLines);
	Buffer2Ascii b2(data2, numLines);

	b1.setCharacterSet("O/\\ - ==|");
	b2.setCharacterSet("O/\\ - ==|");

	auto r = b1.sanityCheck();

	if (!r.wasOk())
	{
		reportScriptError(r.getErrorMessage());
		RETURN_IF_NO_THROW({});
	}

	r = b2.sanityCheck();

	if (!r.wasOk())
	{
		reportScriptError(r.getErrorMessage());
		RETURN_IF_NO_THROW({});
	}

	auto s1 = b1.toString();
	auto s2 = b2.toString();

	if (s1.compare(s2) == 0)
	{
		return s1;
	}
	else
	{
		auto p1 = s1.begin();
		auto p2 = s2.begin();

		int l = s1.length();
		int l2 = s2.length();

        ignoreUnused(l2);
		jassert(l == l2);

		String newC;
		newC.preallocateBytes(l);

		for (int i = 0; i < l; i++)
		{
			if (*p1 != *p2)
				newC << 'X';
			else
				newC << *p1;

			++p1;
			++p2;
		}

		return newC;
	}

}

void ScriptNetworkTest::setTestProperty(String id, var value)
{
	auto v = wb->getTestData().toJSON();

	if (auto obj = v.getDynamicObject())
		obj->setProperty(Identifier(id), value);

	wb->getTestData().fromJSON(v, dontSendNotification);
}

void ScriptNetworkTest::setProcessSpecs(int numChannels, double sampleRate, int blockSize)
{
	auto c = dynamic_cast<CHandler*>(wb->getCompileHandler());
	c->ps.numChannels = numChannels;
	c->ps.blockSize = blockSize;
	c->ps.sampleRate = sampleRate;
}

juce::var ScriptNetworkTest::expectEquals(var data1, var data2, float errorDb)
{
	auto isNumeric = [](const var& v)
	{
		return v.isInt() || v.isDouble() || v.isInt64() || v.isBool();
	};

	if (data1.isArray() && (data2.isArray() || isNumeric(data2)))
	{
		auto size1 = data1.size();
		auto size2 = isNumeric(data2) ? data2.size() : size1;

		if (size1 != size2)
			return var("Array size mismatch");

		for (int i = 0; i < size1; i++)
		{
			auto v = isNumeric(data2) ? data2 : data2[i];
			auto ok = expectEquals(data1[i], v, errorDb);

			if (ok.isString())
				return ok;
		}

		return 0;
	}
	if (data1.isBuffer() && (data2.isBuffer() || isNumeric(data2)))
	{
		auto p1 = data1.getBuffer()->buffer.getReadPointer(0);
		auto p2 = !isNumeric(data2) ? data2.getBuffer()->buffer.getReadPointer(0) : nullptr;
		auto size1 = data1.getBuffer()->size;
		auto size2 = !isNumeric(data2) ? data2.getBuffer()->size : size1;

		if (size1 != size2)
			return var("Buffer size mismatch");

		for (int i = 0; i < size1; i++)
		{
			float v = p2 != nullptr ? p2[i] : (float)data2;
			auto result = expectEquals(p1[i], v, errorDb);

			if (result.isString())
				return result;
		}

		return 0;
	}
	if (isNumeric(data1) && isNumeric(data2))
	{
		auto v1 = (float)data1;
		auto v2 = (float)data2;

		auto diff = hmath::abs(v1 - v2);

		if (diff > Decibels::decibelsToGain(errorDb))
		{
			String error;
			error << "Value error: " << String(Decibels::gainToDecibels(diff), 1) << " dB";
			return var(error);
		}

		return 0;
	}

	return var("unsupported type");
}

void ScriptNetworkTest::setWaitingTime(int timeToWaitMs)
{
	dynamic_cast<CHandler*>(wb->getCompileHandler())->waitTimeMs = jlimit(0, 3000, timeToWaitMs);
}

void ScriptNetworkTest::addRuntimeFunction(var f, int timestamp)
{
	if (HiseJavascriptEngine::isJavascriptFunction(f))
	{
		dynamic_cast<CHandler*>(wb->getCompileHandler())->addRuntimeFunction(f, timestamp);
	}
}

ScriptNetworkTest::CHandler::CHandler(WorkbenchData::Ptr wb, DspNetwork* n) :
	ScriptnodeCompileHandlerBase(wb.get(), n)
{
	ps.voiceIndex = n->getPolyHandler();
	ps.numChannels = wb->getTestData().testSourceData.getNumChannels();
	ps.blockSize = 512;
	ps.numChannels = 1;
	ps.sampleRate = 44100.0;
}

Result ScriptNetworkTest::CHandler::prepareTest(PrepareSpecs ps, const Array<ParameterEvent>& initialParameters)
{
	lastTestResult = ScriptnodeCompileHandlerBase::prepareTest(ps, initialParameters);

	if (waitTimeMs > 0)
	{
		HiseJavascriptEngine::TimeoutExtender te(dynamic_cast<JavascriptProcessor*>(network->getScriptProcessor())->getScriptEngine());
		Thread::getCurrentThread()->wait(waitTimeMs);
	}

	currentTimestamp = 0;

	return lastTestResult;
}

void ScriptNetworkTest::CHandler::processTest(ProcessDataDyn& data)
{
	Range<int> timestampRange(currentTimestamp, currentTimestamp + data.getNumSamples());

	for (auto f : runtimeFunctions)
	{
		if (timestampRange.contains(f->timestamp))
		{
			f->f.callSync(nullptr, 0);
		}
	}

	ScriptnodeCompileHandlerBase::processTest(data);

	currentTimestamp += data.getNumSamples();
}

ScriptNetworkTest::CHandler::RuntimeFunction::RuntimeFunction(DspNetwork* n, const var& f_, int timestamp_) :
	f(n->getScriptProcessor(), nullptr, f_, 0),
	timestamp(timestamp_)
{
	f.incRefCount();
	f.setThisObject(n);
}

void ScriptNetworkTest::CHandler::addRuntimeFunction(const var& f, int timestamp)
{
	runtimeFunctions.add(new RuntimeFunction(network, f, timestamp));
}

ScriptnodeCompileHandlerBase::ScriptnodeCompileHandlerBase(WorkbenchData* d, DspNetwork* network_) :
	CompileHandler(d),
	network(network_),
	lastTestResult(Result::ok())
{

}

snex::ui::WorkbenchData::CompileResult ScriptnodeCompileHandlerBase::compile(const String& codeToCompile)
{
	return {};
}

void ScriptnodeCompileHandlerBase::processTestParameterEvent(int parameterIndex, double value)
{
	SimpleReadWriteLock::ScopedReadLock sl(network->getConnectionLock());
	network->getCurrentParameterHandler()->setParameter(parameterIndex, value);
}

Result ScriptnodeCompileHandlerBase::prepareTest(PrepareSpecs ps, const Array<ParameterEvent>& initialParameters)
{
	network->getExceptionHandler().removeError(nullptr);

	network->setNumChannels(ps.numChannels);
	network->prepareToPlay(ps.sampleRate, ps.blockSize);

	for (auto pe : initialParameters)
	{
		if (pe.timeStamp == 0)
			processTestParameterEvent(pe.parameterIndex, pe.valueToUse);
	}

	if (!network->getExceptionHandler().isOk())
		return Result::fail(network->getExceptionHandler().getErrorMessage(nullptr));

	network->reset();

	return Result::ok();
}

void ScriptnodeCompileHandlerBase::initExternalData(ExternalDataHolder* h)
{

}

void ScriptnodeCompileHandlerBase::postCompile(ui::WorkbenchData::CompileResult& lastResult)
{
	lastTestResult = runTest(lastResult);
}

Result ScriptnodeCompileHandlerBase::runTest(ui::WorkbenchData::CompileResult& lastResult)
{
	auto& td = getParent()->getTestData();

	auto cs = getPrepareSpecs();

	if (cs.sampleRate <= 0.0 || cs.blockSize == 0)
	{
		cs.sampleRate = 44100.0;
		cs.blockSize = 512;
	}

	td.initProcessing(cs);

	return td.processTestData(getParent());
}

void ScriptnodeCompileHandlerBase::processTest(ProcessDataDyn& data)
{
	network->process(data);
}

bool ScriptnodeCompileHandlerBase::shouldProcessEventsManually() const
{
	return false;
}

void ScriptnodeCompileHandlerBase::processHiseEvent(HiseEvent& e)
{
	jassertfalse;
}


Buffer2Ascii::Buffer2Ascii(var data_, int numLines_) :
	data(data_),
	characterSet("O/\\:- ==|"),
	numLines(numLines_)
{

}

void Buffer2Ascii::setCharacterSet(const String& newCharacterSet)
{
	characterSet = newCharacterSet;
}

String Buffer2Ascii::toString() const
{
	String s;

	s << "\n";

	auto numSamples = getNumSamples();

	auto numSamplesPerLine = numSamples / numLines;
	int numCols = isMultiChannel() ? 9 : 13;

	snex::Types::span<Range<int>, 5> eqRanges;

	eqRanges[0] = Range<int>(0, numSamplesPerLine / 2);
	eqRanges[1] = Range<int>(numSamples / 4 - numSamplesPerLine / 2, numSamples / 4 + numSamplesPerLine / 2);
	eqRanges[2] = Range<int>(numSamples / 2 - numSamplesPerLine / 2, numSamples / 2 + numSamplesPerLine / 2);
	eqRanges[3] = Range<int>(3 * numSamples / 4 - numSamplesPerLine / 2, 3 * numSamples / 4 + numSamplesPerLine / 2);
	eqRanges[4] = Range<int>(numSamples - numSamplesPerLine / 2, numSamples + numSamplesPerLine / 2);

	if (isMultiChannel())
	{
		for (int i = 0; i < getNumChannels(); i++)
		{
			s << get(WaveformCharacters::QuarterMarker);

			for (int c = 0; c < numCols / 2; c++)
				s << get(WaveformCharacters::VoidChar);

			s << getChannelChar(getNumChannels() - 1 - i);

			for (int c = 0; c < numCols / 2; c++)
				s << get(WaveformCharacters::VoidChar);

			s << get(WaveformCharacters::QuarterMarker);
		}
	}

	for (int i = 0; i < numSamples; i += numSamplesPerLine)
	{
		String lineChars;
		lineChars << get(WaveformCharacters::LowerBounds);
		lineChars << get(WaveformCharacters::UpperBounds);

		for (const auto& r : eqRanges)
		{
			if (r.contains(i))
			{
				lineChars = {};
				lineChars << get(WaveformCharacters::QuarterMarker);
				lineChars << get(WaveformCharacters::QuarterMarker);
				break;
			}
		}

		s << "\n";

		for (int c = getNumChannels() - 1; c >= 0; c--)
		{
			s << lineChars[0];
			s << printBufferSlice(getChannel(c), i, numSamplesPerLine, numCols);
			s << lineChars[1];
		}
	}

	return rotateString(s);
}

Result Buffer2Ascii::sanityCheck() const
{
	if (numLines == 0)
		return Result::fail("numLines must not be zero");

	if (getNumSamples() == 0)
		return Result::fail("Buffer must not have zero length");

	if (isMultiChannel())
	{
		for (int i = getNumChannels() - 1; i >= 0; i--)
		{
			auto c = getChannel(i);
			if (c == nullptr)
				return Result::fail("channel " + String(i + 1) + " is not a buffer");

			if (c->size != getNumSamples())
				return Result::fail("channel size mismatch at channel " + String(i + 1));
		}
	}

	return Result::ok();
}

juce::juce_wchar Buffer2Ascii::getChannelChar(int channelIndex) const
{
	if (getNumChannels() == 2)
		return channelIndex == 0 ? 'L' : 'R';
	else
		return String(channelIndex + 1)[0];
}

juce::juce_wchar Buffer2Ascii::get(WaveformCharacters s) const
{
	return characterSet[(int)s];
}

int Buffer2Ascii::getNumSamples() const
{
	return getChannel(0)->size;
}

int Buffer2Ascii::getNumChannels() const
{
	return isMultiChannel() ? data.size() : 1;
}

juce::VariantBuffer::Ptr Buffer2Ascii::getChannel(int index) const
{
	if (!isMultiChannel())
		return data.getBuffer();
	else
	{
		return data[index].getBuffer();
	}
}

String Buffer2Ascii::rotateString(String s)
{
	auto rows = StringArray::fromLines(s);

	rows.removeEmptyStrings();

	int numCols = rows[0].length();

	StringArray rotated;

	for (int i = numCols - 1; i >= 0; i--)
	{
		String newRow;
		newRow.preallocateBytes(rows.size());

		for (const auto& r : rows)
		{
			newRow << r[i];
		}

		rotated.add(newRow);
	}

	auto r = "\n" + rotated.joinIntoString("\n");

	return r;
}

String Buffer2Ascii::printBufferSlice(VariantBuffer::Ptr b, int i, int numSamplesPerLine, int numCols) const
{
	String s;
	s.preallocateBytes(numCols);

	int numThisTime = jmin(numSamplesPerLine, b->size - i);

	auto magRange = b->buffer.findMinMax(0, i, numThisTime);


	char SignalChar;

	float delta = 0.0f;

	auto ptr = b->buffer.getReadPointer(0, i);

	for (int j = 1; j < numThisTime; j++)
		delta += ptr[j] - ptr[j - 1];

	float cDelta = 1.0f / ((float)numCols - 1.0f);

	if (hmath::abs(delta) < cDelta * JUCE_LIVE_CONSTANT_OFF(0.25))
		SignalChar = get(WaveformCharacters::ZeroLineChar);
	else if (delta > 0.0)
		SignalChar = get(WaveformCharacters::UpChar);
	else
		SignalChar = get(WaveformCharacters::DownChar);

	auto absStart = hmath::abs(magRange.getStart());
	auto absEnd = hmath::abs(magRange.getEnd());

	float mag = 0.0f;

	if (absStart > absEnd)
		mag = magRange.getStart();
	if (absStart < absEnd)
		mag = magRange.getEnd();
	if (absStart == absEnd)
		mag = magRange.getStart() + magRange.getLength() * 0.5f;

	mag = jlimit(-1.0f, 1.0f, mag);

	mag = (mag + 1.0f) * 0.5f;

	auto thisColIndex = roundToInt(mag * (numCols - 1));

	auto halfCol = numCols / 2;

	for (int i = 0; i < numCols; i++)
	{
		if (i == thisColIndex)
			s << SignalChar;
		else
		{
			if (i == halfCol)
			{
				s << get(WaveformCharacters::ZeroLineChar);
			}
			else if (i < halfCol)
			{
				if (i < thisColIndex)
					s << get(WaveformCharacters::VoidChar);
				else
					s << get(WaveformCharacters::FillChar);
			}
			else
			{
				if (i > thisColIndex)
					s << get(WaveformCharacters::VoidChar);
				else
					s << get(WaveformCharacters::FillChar);
			}
		}
	}

	return s;
}

}
