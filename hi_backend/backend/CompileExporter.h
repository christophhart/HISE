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


class BaseExporter
{
public:

	
	virtual ~BaseExporter() {};

	ModulatorSynthChain* chainToExport;

protected:

	ValueTree exportReferencedImageFiles();
	ValueTree exportReferencedAudioFiles();
	ValueTree exportEmbeddedFiles(bool includeSampleMaps);
	ValueTree exportUserPresetFiles();
	ValueTree exportPresetFile();
	
	BaseExporter(ModulatorSynthChain* chainToExport_) : chainToExport(chainToExport_) {}
	
private:

	ValueTree collectAllSampleMapsInDirectory();

};


class CompileExporter: public BaseExporter
{
public:

	CompileExporter(ModulatorSynthChain* chainToExport);

	~CompileExporter() { globalCommandLineExport = false; }

	/** 0xABCD
	*
	*	A = OS (1 = Windows / 2 = OSX / 4 = iOS)
	*	B = type (1 = Standalone, 2 = Instrument, 4 = Effect)
	*	C = platform (0 = void, 1 = VST, 2 = AU, 4 = VST / AU, 8 = AAX);
	*	D = bit (1 = 32bit, 2 = 64bit, 4 = both) 
	*/
	enum BuildOption
	{
		Cancelled = 0,
		VSTWindowsx86 = 0x1411,
		VSTWindowsx64 = 0x1412,
		VSTWindowsx64x86 = 0x1414,
		VSTiWindowsx86 = 0x1211,
		VSTiWindowsx64 = 0x1212,
		VSTiWindowsx64x86 = 0x1214,
		AUmacOS = 0x2422,
		VSTmacOS = 0x2412,
		VSTAUmacOS = 0x2442,
		AUimacOS = 0x2222,
		VSTimacOS = 0x2212,
		VSTiAUimacOS = 0x2242,
		AAXWindowsx86 = 0x1281,
		AAXWindowsx64 = 0x1282,
		AAXWindowsx86x64 = 0x1284,
		AAXmacOS = 0x2284,
		StandaloneWindowsx86 = 0x1101,
		StandaloneWindowsx64 = 0x1102,
		StandaloneWindowsx64x86 = 0x1104,
		StandaloneiOS = 0x4104,
		StandalonemacOS = 0x2104,
        AllPluginFormatsFX = 0x10404,
        AllPluginFormatsInstrument = 0x10204,
		numBuildOptions
	};

	struct BuildOptionHelpers
	{
        static bool isVST(BuildOption option) { return ((option & 0x10000) != 0) || (option & 0x0010) != 0 || (option & 0x0040) != 0; };
		static bool isAU(BuildOption option) { return ((option & 0x10000) != 0) || (option & 0x0020) != 0 || (option & 0x0040) != 0; };
		static bool isAAX(BuildOption option) { return ((option & 0x10000) != 0) || (option & 0x0080) != 0; };
		static bool is32Bit(BuildOption option) { return (option & 0x0001) != 0 || (option & 0x0004) != 0; };
		static bool is64Bit(BuildOption option) { return (option & 0x0002) != 0 || (option & 0x0004) != 0; };
		static bool isIOS(BuildOption option) { return (option & 0x4000) != 0; };
		static bool isWindows(BuildOption option) { return (option & 0x1000) != 0; };
		static bool isOSX(BuildOption option) { return (option & 0x2000) != 0; }
		static bool isStandalone(BuildOption option) { return (option & 0x0100) != 0; }
		static bool isInstrument(BuildOption option) { return (option & 0x0200) != 0; }
		static bool isEffect(BuildOption option) { return (option & 0x0400) != 0; }
		static void runUnitTests();
	};

	enum class TargetTypes
	{
		InstrumentPlugin,
		EffectPlugin,
		StandaloneApplication,
		numTargetTypes
	};

	enum ErrorCodes
	{
		OK = 0,
        SanityCheckFailed,
		PresetIsInvalid,
		ProjectXmlInvalid,
		HISEImageDirectoryNotFound,
		IntrojucerNotFound,
		UserAbort,
		MissingArguments,
		BuildOptionInvalid,
		CompileError,
		VSTSDKMissing,
		AAXSDKMissing,
		ASIOSDKMissing,
		HISEPathNotSpecified,
		numErrorCodes
	};

	/** Exports the main synthchain all samples, external files into a ValueTree file which can be included in a compiled FrontEndProcessor. */
	ErrorCodes exportMainSynthChainAsInstrument(BuildOption option=BuildOption::Cancelled);
	ErrorCodes exportMainSynthChainAsFX(BuildOption option=BuildOption::Cancelled);
	ErrorCodes exportMainSynthChainAsStandaloneApp(BuildOption option=BuildOption::Cancelled);

	static ErrorCodes compileFromCommandLine(const String& commandLine);

	BuildOption getBuildOptionFromCommandLine(StringArray &args);

	File hisePath;
	
    bool useIpp;

	void printErrorMessage(const String& title, const String &message);

	static String getCompileResult(ErrorCodes result);

	void writeValueTreeToTemporaryFile(const ValueTree& v, const String &tempFolder, const String& childFile, bool compress=false);

	int getBuildOptionPart(const String& argument);

	static void setExportingFromCommandLine()
	{
		globalCommandLineExport = true;
	};

	static bool isExportingFromCommandLine() { return globalCommandLineExport; }

private:

	static bool globalCommandLineExport;
	

	struct HelperClasses
	{
		static bool isUsingVisualStudio2015();

		static ErrorCodes saveProjucerFile(String templateProject, CompileExporter* exporter);
	};

	ErrorCodes exportInternal(TargetTypes type, BuildOption option);

	bool checkSanity(BuildOption option);

	BuildOption showCompilePopup(TargetTypes type);

	void convertTccScriptsToCppClasses();

	void createCppFileFromTccScript(File& targetDirectory, const File &f, Array<File>& convertedList);

	StringArray getTccSection(const StringArray &cLines, const String &sectionName);

	ErrorCodes compileSolution(BuildOption buildOption, TargetTypes types);

	ErrorCodes createPluginDataHeaderFile(const String &solutionDirectory, const String &publicKey);

	ErrorCodes createResourceFile(const String &solutionDirectory, const String & uniqueName, const String &version);

	ErrorCodes createPluginProjucerFile(TargetTypes type, BuildOption option);

	struct ProjectTemplateHelpers
	{
		static void handleCompilerInfo(CompileExporter* exporter, String& templateProject);
		static void handleCompanyInfo(CompileExporter* exporter, String& templateProject);
		static void handleVisualStudioVersion(String& templateProject);
		static void handleAdditionalSourceCode(CompileExporter* exporter, String &templateProject);
		static void handleCopyProtectionInfo(CompileExporter* exporter, String &templateProject);
	};

	struct HeaderHelpers
	{
		static void addBasicIncludeLines(String& pluginDataHeaderFile);
		static void addAdditionalSourceCodeHeaderLines(CompileExporter* exporter, String& pluginDataHeaderFile);
		static void addStaticDspFactoryRegistration(String& pluginDataHeaderFile, CompileExporter* exporter);
		static void addCopyProtectionHeaderLines(const String &publicKey, String& pluginDataHeaderFile);
		static void addCustomToolbarRegistration(CompileExporter* exporter, String& pluginDataHeaderFile);
		static void addProjectInfoLines(CompileExporter* exporter, String& pluginDataHeaderFile);

		static void writeHeaderFile(const String & solutionDirectory, const String& pluginDataHeaderFile);
	};

	ErrorCodes copyHISEImageFiles();

	File getProjucerProjectFile();
	
	ErrorCodes createStandaloneAppHeaderFile(const String& solutionDirectory, const String& uniqueId, const String& version, String publicKey);
	CompileExporter::ErrorCodes createStandaloneAppProjucerFile();

	struct BatchFileCreator
	{
		static void createBatchFile(CompileExporter* exporter, BuildOption buildOption, TargetTypes types);
		static File getBatchFile(CompileExporter* exporter);
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
