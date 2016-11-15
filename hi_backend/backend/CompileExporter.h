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

#ifndef COMPILEEXPORTER_H_INCLUDED



class CompileExporter
{
public:

	enum BuildOption
	{
		Cancelled = 0,
		VSTx86,
		VSTx64,
		VSTx64x86,
		AU,
		numBuildOptions
	};

	enum ErrorCodes
	{
		OK = 0,
		ProjectXmlInvalid,
		HISEImageDirectoryNotFound,
		IntrojucerNotFound,
		numErrorCodes
	};

	/** Exports the main synthchain all samples, external files into a ValueTree file which can be included in a compiled FrontEndProcessor. */
	static void exportMainSynthChainAsPackage(ModulatorSynthChain *chainToExport);

	static void exportMainSynthChainAsFX(ModulatorSynthChain* chainToExport);

private:

	static bool checkSanity(ModulatorSynthChain *chainToExport);

	static BuildOption showCompilePopup(String &publicKey, String &uniqueId, String &version, String &solutionDirectory);

	static void writeReferencedImageFiles(ModulatorSynthChain * chainToExport, const String directoryPath);

	static void writeReferencedAudioFiles(ModulatorSynthChain * chainToExport, const String directoryPath);

	static void writeEmbeddedFiles(ModulatorSynthChain * chainToExport, const String &directoryPath);

	static void writeUserPresetFiles(ModulatorSynthChain * chainToExport, const String &directoryPath);

	static void writePresetFile(ModulatorSynthChain *chainToExport, const String directoryPath, const String &uniqueName);

	static void convertTccScriptsToCppClasses(ModulatorSynthChain* chainToExport);

	static void createCppFileFromTccScript(File& targetDirectory, const File &f, Array<File>& convertedList);

	static StringArray getTccSection(const StringArray &cLines, const String &sectionName);

	static ErrorCodes compileSolution(ModulatorSynthChain *chainToExport, BuildOption buildOption);

	static ErrorCodes createPluginDataHeaderFile(ModulatorSynthChain* chainToExport, const String &solutionDirectory, const String &uniqueName, const String &version, const String &publicKey);

	static ErrorCodes createResourceFile(const String &solutionDirectory, const String & uniqueName, const String &version);

	static ErrorCodes createIntrojucerFile(ModulatorSynthChain *chainToExport, bool createFX=false);

	static void handleAdditionalSourceCode(ModulatorSynthChain * chainToExport, String &templateProject);

	static ErrorCodes copyHISEImageFiles(ModulatorSynthChain *chainToExport);

	static File getIntrojucerProjectFile(ModulatorSynthChain *chainToExport);
	static ValueTree collectAllSampleMapsInDirectory(ModulatorSynthChain * chainToExport);

	struct BatchFileCreator
	{
		static void createBatchFile(ModulatorSynthChain *chainToExport, BuildOption buildOption);

		static File getBatchFile(ModulatorSynthChain *chainToExport);
	};

};

/** A cheap rip-off of Juce's Binary Builder to convert the exported valuetrees into a cpp file. */
class CppBuilder
{
public:

	static int exportValueTreeAsCpp(const File &sourceDirectory, const File &targetDirectory, String &targetClassName);

private:

	static int addFile(const File& file, const String& classname, OutputStream& headerStream, OutputStream& cppStream);
	static bool isHiddenFile(const File& f, const File& root);
};



#define COMPILEEXPORTER_H_INCLUDED





#endif  // COMPILEEXPORTER_H_INCLUDED
