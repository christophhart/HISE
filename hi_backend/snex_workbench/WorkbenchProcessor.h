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

	DspNetworkCompileExporter(Component* editor, BackendProcessor* bp);

	void run() override;

	void threadFinished() override;

	File getBuildFolder() const override;

private:

	enum CppFileLocationType
	{
		UnknownFileType,
		CompiledNetworkFile,
		ThirdPartyFile,
		ThirdPartySourceFile,
		EmbeddedDataFile
	};

	void writeDebugFileAndShowSolution();

	CppFileLocationType getLocationType(const File& f) const;

	DspNetwork* getNetwork();

	static Array<File> getIncludedNetworkFiles(const File& networkFile);
	
	BackendDllManager* getDllManager();

	Component* editor;

	ErrorCodes ok = ErrorCodes::UserAbort;

	File getFolder(BackendDllManager::FolderSubType t) const
	{
		return BackendDllManager::getSubFolder(getMainController(), t);
	}

	static bool isInterpretedDataFile(const File& f);

	void createIncludeFile(const File& sourceDir);

	void createProjucerFile();

	String errorMessage;

	Array<File> includedFiles;
	Array<File> includedThirdPartyFiles;
	

	File getSourceDirectory(bool isDllMainFile) const;

	void createMainCppFile(bool isDllMainFile);

	snex::cppgen::CustomNodeProperties nodeProperties;
};


}
