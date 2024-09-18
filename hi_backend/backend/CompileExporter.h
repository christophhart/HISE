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

#ifndef COMPILEEXPORTER_H_INCLUDED


namespace hise { using namespace juce;

class BaseExporter
{
public:

	
	virtual ~BaseExporter() {};

	ModulatorSynthChain* chainToExport;

	HiseSettings::Data& dataObject;

	virtual File getBuildFolder() const = 0;

protected:

	ValueTree exportEmbeddedFiles();
	ValueTree exportUserPresetFiles();
	ValueTree exportPresetFile();
	
	BaseExporter(ModulatorSynthChain* chainToExport_);
	
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
	*	A = OS (0 = Linux / 1 = Windows / 2 = OSX / 4 = iPad, 8=iPhone, 12 = iPad/iPhone)
	*	B = type (1 = Standalone, 2 = Instrument, 4 = Effect, 8 = MidiFX)
	*	C = platform (0 = void, 1 = VST, 2 = AU, 4 = VST / AU, 8 = AAX);
	*	D = bit (1 = 32bit, 2 = 64bit, 4 = both) 
	*/
	enum BuildOption
	{
		Cancelled = 0,
		StandaloneLinux = 0x0104,
		VSTiLinux = 0x0214,
		VSTLinux = 0x0414,
		MidiFXLinux = 0x0814,
		VSTWindowsx86 = 0x1411,
		VSTWindowsx64 = 0x1412,
		VSTWindowsx64x86 = 0x1414,
		VSTiWindowsx86 = 0x1211,
		VSTiWindowsx64 = 0x1212,
		VSTiWindowsx64x86 = 0x1214,
		MidiFXWindowsx86 = 0x1811,
		MidiFXWIndowsx64 = 0x1812,
		MidiFXWindowsx64x86 = 0x1814,
		AUmacOS = 0x2422,
		VSTmacOS = 0x2412,
		VSTAUmacOS = 0x2442,
		AUimacOS = 0x2222,
		VSTimacOS = 0x2212,
		VSTiAUimacOS = 0x2242,
		AUMIDImacOS = 0x2822,
		VSTMIDImacOS = 0x2812,
		VSTAUMIDImacOS = 0x2842,
		AAXWindowsx86 = 0x1281,
		AAXWindowsx64 = 0x1282,
		AAXWindowsx86x64 = 0x1284,
		AAXmacOS = 0x2284,
		HeadlessLinuxVST = 0x0484,
		HeadlessLinuxVSTi = 0x0284,
		StandaloneWindowsx86 = 0x1101,
		StandaloneWindowsx64 = 0x1102,
		StandaloneWindowsx64x86 = 0x1104,
		StandaloneiOS = 0xC104,
		StandaloneiPad = 0x4104,
		StandaloneiPhone = 0x8104,
		StandalonemacOS = 0x2104,
        AllPluginFormatsFX = 0x10404,
        AllPluginFormatsInstrument = 0x10204,
		AllPluginFormatsMidiFX = 0x10804,
		numBuildOptions
	};

	struct BuildOptionHelpers
	{
        static bool isVST(BuildOption option) { return ((option & 0x10000) != 0) || (option & 0x0010) != 0 || (option & 0x0040) != 0; };
		static bool isAU(BuildOption option) { return ((option & 0x10000) != 0) || (option & 0x0020) != 0 || (option & 0x0040) != 0; };
		static bool isAAX(BuildOption option) { return ((option & 0x10000) != 0) || (option & 0x0080) != 0; };
		static bool is32Bit(BuildOption option) { return false; };
		static bool is64Bit(BuildOption option) { return (option & 0x0002) != 0 || (option & 0x0004) != 0; };
		static bool isIOS(BuildOption option) { return (option & 0xC000) != 0; };
		static bool isIPhone(BuildOption option) { return (option & 0x8000) != 0; };
		static bool isIPad(BuildOption option) { return (option & 0x4000) != 0; };
		static bool isWindows(BuildOption option) { return (option & 0x1000) != 0; };
		static bool isLinux(BuildOption option) { return (option & 0x3000) == 0; };
		static bool isOSX(BuildOption option) { return (option & 0x2000) != 0; }
		static bool isStandalone(BuildOption option) { return (option & 0x0100) != 0; }
		static bool isInstrument(BuildOption option) { return (option & 0x0200) != 0; }
		static bool isEffect(BuildOption option) { return (option & 0x0400) != 0; }
		static bool isHeadlessLinuxPlugin(BuildOption option) { return isLinux(option) && (option & 0x0080) != 0; };
		static bool isMidiEffect(BuildOption option) { return (option & 0x0800) != 0; };
		static void runUnitTests();
	};

	enum class TargetTypes
	{
		InstrumentPlugin,
		EffectPlugin,
		MidiEffectPlugin,
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
		CorruptedPoolFiles,
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
	ErrorCodes exportMainSynthChainAsMidiFx(BuildOption option=BuildOption::Cancelled);

    static NamedValueSet getTemporaryDefinitions(const String& commandLine)
    {
        NamedValueSet list;
        
        auto argsString = commandLine.fromFirstOccurrenceOf(" ", false, false);
        auto sa = StringArray::fromTokens(argsString, true);
        
        for(const auto& s: sa)
        {
            if(s.startsWith("-D:"))
            {
                auto def = s.substring(3).trim();
                auto key = def.upToFirstOccurrenceOf("=", false, false).trim();
                auto value = def.fromFirstOccurrenceOf("=", false, false).trim();
                
                list.set(Identifier(key), var(value));
            }
        }
        
        return list;
    }
    
	static ErrorCodes compileFromCommandLine(const String& commandLine, String& pluginFile);

	BuildOption getBuildOptionFromCommandLine(StringArray &args);

	File hisePath;
	
    bool useIpp;
    
    bool legacyCpuSupport = false;

	bool rawMode = false;

	void setRawExportMode(bool useRawMode)
	{
		rawMode = useRawMode;
	}

	void printErrorMessage(const String& title, const String &message);

	static String getCompileResult(ErrorCodes result);

	File getBuildFolder() const override;
	
	void writeValueTreeToTemporaryFile(const ValueTree& v, const String &tempFolder, const String& childFile, bool compress=false);

	template <class ProviderType> Result compressValueTree(const ValueTree& v, const String& tempFolder, const String& childFile)
	{
		File tf(tempFolder);
		File target = tf.getChildFile(childFile);

		zstd::ZCompressor<ProviderType> compressor;
		return compressor.compress(v, target);
	}

	int getBuildOptionPart(const String& argument);

	static void setExportingFromCommandLine()
	{
		globalCommandLineExport = true;
	};

	static void setExportUsingCI(bool shouldUseCIMode)
	{
		useCIMode = shouldUseCIMode;
	}

	static bool isUsingCIMode() { return useCIMode; }

	static bool isExportingFromCommandLine() { return globalCommandLineExport; }

    bool shouldBeSilent() const { return isExportingFromCommandLine() || silentMode; }
    
	struct BatchFileCreator
	{
		static void createBatchFile(CompileExporter* exporter, BuildOption buildOption, TargetTypes types);
		static File getBatchFile(CompileExporter* exporter);
	};

protected:

    bool noLto = false;
    
    bool silentMode = false;
    
	String configurationName = "Release";

	static bool globalCommandLineExport;
	
	static bool useCIMode;

	static int forcedVSTVersion;

	struct HelperClasses
	{
		static String getFileNameForCompiledPlugin(const HiseSettings::Data& dataObject, ModulatorSynthChain* chain, BuildOption option);

		static bool isUsingVisualStudio2017(const HiseSettings::Data& dataObject);

		static ErrorCodes saveProjucerFile(String templateProject, CompileExporter* exporter);
	};

	ErrorCodes exportInternal(TargetTypes type, BuildOption option);

	bool checkSanity(TargetTypes type, BuildOption option);

	BuildOption showCompilePopup(TargetTypes type);

	ErrorCodes compileSolution(BuildOption buildOption, TargetTypes types);

	ErrorCodes createPluginDataHeaderFile(const String &solutionDirectory, const String &publicKey, bool iOSAUv3);

	ErrorCodes createResourceFile(const String &solutionDirectory, const String & uniqueName, const String &version);

	ErrorCodes createPluginProjucerFile(TargetTypes type, BuildOption option, ModulatorSynthChain* chainToExport);

	struct ProjectTemplateHelpers
	{
		static void handleCompilerWarnings(String& templateProject);

		static void handleCompilerInfo(CompileExporter* exporter, String& templateProject);
		static void handleCompanyInfo(CompileExporter* exporter, String& templateProject);
		static void handleVisualStudioVersion(const HiseSettings::Data& dataObject, String& templateProject);
		static void handleAdditionalSourceCode(CompileExporter* exporter, String &templateProject, BuildOption option);
		static void handleCopyProtectionInfo(CompileExporter* exporter, String &templateProject, BuildOption option);
		static String getTargetFamilyString(BuildOption option);
		static String getPluginChannelAmount(ModulatorSynthChain* chain);
	};

	struct HeaderHelpers
	{
		static void addBasicIncludeLines(CompileExporter* exporter, String& pluginDataHeaderFile, bool isIOS=false);
		static void addAdditionalSourceCodeHeaderLines(CompileExporter* exporter, String& pluginDataHeaderFile);
		static void addStaticDspFactoryRegistration(String& pluginDataHeaderFile, CompileExporter* exporter);
		static void addCopyProtectionHeaderLines(const String &publicKey, String& pluginDataHeaderFile);
		static void addFullExpansionTypeSetter(CompileExporter* exporter, String& pluginDataHeaderFile);
		static void addProjectInfoLines(CompileExporter* exporter, String& pluginDataHeaderFile);

		static void writeHeaderFile(const String & solutionDirectory, const String& pluginDataHeaderFile);
	};

	ErrorCodes copyHISEImageFiles();

	File getProjucerProjectFile();
	
	ErrorCodes createStandaloneAppHeaderFile(const String& solutionDirectory, const String& uniqueId, const String& version, String publicKey);
	CompileExporter::ErrorCodes createStandaloneAppProjucerFile(BuildOption option);

	
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



} // namespace hise

#endif  // COMPILEEXPORTER_H_INCLUDED
