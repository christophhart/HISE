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

namespace hise {
using namespace juce;




struct BufferTestResult
{
	static bool isAlmostEqual(float a, float b, float errorDb = -80.0f)
	{
		return std::abs(a - b) < Decibels::decibelsToGain(errorDb);
	}

	BufferTestResult(const AudioSampleBuffer& a, const AudioSampleBuffer& b) :
		r(Result::ok())
	{
		if (a.getNumChannels() != b.getNumChannels())
			r = Result::fail("Channel amount mismatch");

		if (a.getNumSamples() != b.getNumSamples())
			r = Result::fail("Sample amount mismatch");

		if (!r.wasOk())
			return;

		for (int c = 0; c < a.getNumChannels(); c++)
		{
			BigInteger sampleStates;
			sampleStates.setBit(a.getNumSamples() - 1, true);
			sampleStates.clearBit(a.getNumSamples() - 1);

			for (int i = 0; i < a.getNumSamples(); i++)
			{
				auto as = a.getSample(c, i);
				auto es = b.getSample(c, i);

				if (!isAlmostEqual(as, es))
				{
					sampleStates.setBit(i, true);

					if (r.wasOk())
					{
						String d;
						d << "data[" << String(c) << "][" << String(i) << "]: value mismatch";

						r = Result::fail(d);
					}
				}
			}

			errorRanges.add(sampleStates);
		}
	}

	Result r;
	Array<BigInteger> errorRanges;
};



class DspNetworkCompileExporter : public hise::DialogWindowWithBackgroundThread,
	public ControlledObject,
	public CompileExporter
{
public:

	enum class DspNetworkErrorCodes
	{
		OK,
		NoNetwork,
		NonCompiledInclude,
        CppGenError,
		UninitialisedProperties
	};

	enum CppFileLocationType
	{
		UnknownFileType,
		CompiledNetworkFile,
		ThirdPartyFile,
		ThirdPartySourceFile,
		EmbeddedDataFile
	};

	/** Plain non-UI context object holding all data the static export pipeline
	    operates on. Constructed on any thread - no UI base class. The instance
	    `run()` builds one from its members; `NetworkCompiler::execute` builds
	    one directly to avoid creating the UI-bound exporter on a worker thread. */
	struct Context
	{
		// Inputs
		BackendProcessor* bp = nullptr;
		CompileExporter* exporter = nullptr;        // required - provides hisePath/useIpp/chainToExport/dataObject
		ChildProcessManager* managerToUse = nullptr;
		bool skipCompilation = false;
		std::function<void(const String&)> logCallback;
		std::function<void(double)> progressCallback;

		// Working state (populated during runStatic)
		StringArray nodesToCompile;
		StringArray cppFilesToCompile;
		Array<File> includedFiles;
		Array<File> includedThirdPartyFiles;

		// Outputs
		CompileExporter::ErrorCodes ok = CompileExporter::ErrorCodes::UserAbort;
		String errorMessage;

		MainController* getMainController() const;
		File getFolder(BackendDllManager::FolderSubType t) const;
		void logMessage(const String& m) const;
		void setProgress(double p) const;
		Result getCompilationResult() const { return ok == CompileExporter::ErrorCodes::OK ? Result::ok() : Result::fail(errorMessage); }
	};

	DspNetworkCompileExporter(Component* editor, BackendProcessor* bp, bool skipCompilation_=false);

	void run() override;

	void threadFinished() override;

	File getBuildFolder() const override;

	ChildProcessManager* managerToUse = nullptr;

	ErrorCodes getErrorCode() const { return ok; }

	StringArray nodesToCompile;
	StringArray cppFilesToCompile;

	bool skipCompilation = false;

	DspNetwork* getNetwork();

	BackendDllManager* getDllManager();

	Result getCompilationResult() const { return getErrorCode() == ErrorCodes::OK ? Result::ok() : Result::fail(errorMessage); }

	/** Runs the source-generation + projucer-file pipeline against the supplied
	    context. Does NOT invoke `compileSolution()` - callers that want the DLL
	    built must do so themselves on the supplied exporter. */
	static void runStatic(Context& ctx);

	static void createProjucerFile(Context& ctx);
	static void createIncludeFile(Context& ctx, const File& sourceDir);
	static void createMainCppFile(Context& ctx, bool isDllMainFile);
	static File getSourceDirectory(const Context& ctx, bool isDllMainFile);
	static CppFileLocationType getLocationType(const Context& ctx, const File& f);
	static DspNetwork* getNetwork(const Context& ctx);
	static BackendDllManager* getDllManager(const Context& ctx);

private:

	void writeDebugFileAndShowSolution();

	static Array<File> getIncludedNetworkFiles(const File& networkFile);

	Component* editor;

	ErrorCodes ok = ErrorCodes::UserAbort;

	File getFolder(BackendDllManager::FolderSubType t) const
	{
		return BackendDllManager::getSubFolder(getMainController(), t);
	}

	static bool isInterpretedDataFile(const File& f);

	void logMessage(const String& m)
	{
		if(managerToUse != nullptr)
		{
			managerToUse->logMessage("> " + m + "\n");
			return;
		}

		if(MessageManager::getInstance()->isThisTheMessageThread())
			showStatusMessage(m);
		else
			Logger::writeToLog(m);
	}

	String errorMessage;

	Array<File> includedFiles;
	Array<File> includedThirdPartyFiles;

	snex::cppgen::CustomNodeProperties nodeProperties;
};


}
