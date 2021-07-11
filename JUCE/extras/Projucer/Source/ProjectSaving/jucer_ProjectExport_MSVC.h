/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2020 - Raw Material Software Limited

   JUCE is an open source library subject to commercial or open-source
   licensing.

   By using JUCE, you agree to the terms of both the JUCE 6 End-User License
   Agreement and JUCE Privacy Policy (both effective as of the 16th June 2020).

   End User License Agreement: www.juce.com/juce-6-licence
   Privacy Policy: www.juce.com/juce-privacy-policy

   Or: You may also use this code under the terms of the GPL v3 (see
   www.gnu.org/licenses).

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

#pragma once


//==============================================================================
class MSVCProjectExporterBase   : public ProjectExporter
{
public:
    MSVCProjectExporterBase (Project& p, const ValueTree& t, String folderName)
        : ProjectExporter (p, t),
          IPPLibraryValue       (settings, Ids::IPPLibrary,                   getUndoManager()),
          platformToolsetValue  (settings, Ids::toolset,                      getUndoManager()),
          targetPlatformVersion (settings, Ids::windowsTargetPlatformVersion, getUndoManager()),
          manifestFileValue     (settings, Ids::msvcManifestFile,             getUndoManager())
    {
        targetLocationValue.setDefault (getDefaultBuildsRootFolder() + folderName);
    }

    virtual int getVisualStudioVersion() const = 0;

    virtual String getSolutionComment() const = 0;
    virtual String getToolsVersion() const = 0;
    virtual String getDefaultToolset() const = 0;
    virtual String getDefaultWindowsTargetPlatformVersion() const = 0;

    //==============================================================================
    String getIPPLibrary() const                      { return IPPLibraryValue.get(); }
    String getPlatformToolset() const                 { return platformToolsetValue.get(); }
    String getWindowsTargetPlatformVersion() const    { return targetPlatformVersion.get(); }

    //==============================================================================
    void addToolsetProperty (PropertyListBuilder& props, const char** names, const var* values, int num)
    {
        props.add (new ChoicePropertyComponent (platformToolsetValue, "Platform Toolset",
                                                StringArray (names, num), { values, num }),
                   "Specifies the version of the platform toolset that will be used when building this project.");
    }

    void create (const OwnedArray<LibraryModule>&) const override
    {
        createResourcesAndIcon();
        createPackagesConfigFile();

        for (int i = 0; i < targets.size(); ++i)
            if (auto* target = targets[i])
                target->writeProjectFile();

        build_tools::writeStreamToFile (getSLNFile(), [&] (MemoryOutputStream& mo)
        {
            writeSolutionFile (mo, "11.00", getSolutionComment());
        });
    }

    //==============================================================================
    void updateDeprecatedSettings() override
    {
        {
            auto oldStylePrebuildCommand = getSettingString (Ids::prebuildCommand);
            settings.removeProperty (Ids::prebuildCommand, nullptr);

            if (oldStylePrebuildCommand.isNotEmpty())
                for (ConfigIterator config (*this); config.next();)
                    dynamic_cast<MSVCBuildConfiguration&> (*config).getValue (Ids::prebuildCommand) = oldStylePrebuildCommand;
        }

        {
            auto oldStyleLibName = getSettingString ("libraryName_Debug");
            settings.removeProperty ("libraryName_Debug", nullptr);

            if (oldStyleLibName.isNotEmpty())
                for (ConfigIterator config (*this); config.next();)
                    if (config->isDebug())
                        config->getValue (Ids::targetName) = oldStyleLibName;
        }

        {
            auto oldStyleLibName = getSettingString ("libraryName_Release");
            settings.removeProperty ("libraryName_Release", nullptr);

            if (oldStyleLibName.isNotEmpty())
                for (ConfigIterator config (*this); config.next();)
                    if (! config->isDebug())
                        config->getValue (Ids::targetName) = oldStyleLibName;
        }

        for (ConfigIterator i (*this); i.next();)
            dynamic_cast<MSVCBuildConfiguration&> (*i).updateOldLTOSetting();
    }

    void initialiseDependencyPathValues() override
    {
        vstLegacyPathValueWrapper.init ({ settings, Ids::vstLegacyFolder, nullptr },
                                        getAppSettings().getStoredPath (Ids::vstLegacyPath, TargetOS::windows), TargetOS::windows);

        aaxPathValueWrapper.init ({ settings, Ids::aaxFolder, nullptr },
                                  getAppSettings().getStoredPath (Ids::aaxPath,  TargetOS::windows), TargetOS::windows);

        rtasPathValueWrapper.init ({ settings, Ids::rtasFolder, nullptr },
                                   getAppSettings().getStoredPath (Ids::rtasPath, TargetOS::windows), TargetOS::windows);
    }

    //==============================================================================
    class MSVCBuildConfiguration  : public BuildConfiguration,
                                    private Value::Listener
    {
    public:
        MSVCBuildConfiguration (Project& p, const ValueTree& settings, const ProjectExporter& e)
            : BuildConfiguration (p, settings, e),
              warningLevelValue              (config, Ids::winWarningLevel,            getUndoManager(), 4),
              warningsAreErrorsValue         (config, Ids::warningsAreErrors,          getUndoManager(), false),
              prebuildCommandValue           (config, Ids::prebuildCommand,            getUndoManager()),
              postbuildCommandValue          (config, Ids::postbuildCommand,           getUndoManager()),
              generateDebugSymbolsValue      (config, Ids::alwaysGenerateDebugSymbols, getUndoManager(), false),
              generateManifestValue          (config, Ids::generateManifest,           getUndoManager(), true),
              enableIncrementalLinkingValue  (config, Ids::enableIncrementalLinking,   getUndoManager(), false),
              useRuntimeLibDLLValue          (config, Ids::useRuntimeLibDLL,           getUndoManager(), true),
              multiProcessorCompilationValue (config, Ids::multiProcessorCompilation,  getUndoManager(), true),
              intermediatesPathValue         (config, Ids::intermediatesPath,          getUndoManager()),
              characterSetValue              (config, Ids::characterSet,               getUndoManager()),
              architectureTypeValue          (config, Ids::winArchitecture,            getUndoManager(), get64BitArchName()),
              fastMathValue                  (config, Ids::fastMath,                   getUndoManager()),
              debugInformationFormatValue    (config, Ids::debugInformationFormat,     getUndoManager(), isDebug() ? "ProgramDatabase" : "None"),
              pluginBinaryCopyStepValue      (config, Ids::enablePluginBinaryCopyStep, getUndoManager(), false),
              vstBinaryLocation              (config, Ids::vstBinaryLocation,          getUndoManager()),
              vst3BinaryLocation             (config, Ids::vst3BinaryLocation,         getUndoManager()),
              rtasBinaryLocation             (config, Ids::rtasBinaryLocation,         getUndoManager()),
              aaxBinaryLocation              (config, Ids::aaxBinaryLocation,          getUndoManager()),
              unityPluginBinaryLocation      (config, Ids::unityPluginBinaryLocation,  getUndoManager(), {})
        {
            setPluginBinaryCopyLocationDefaults();
            optimisationLevelValue.setDefault (isDebug() ? optimisationOff : optimiseFull);

            architectureValueToListenTo = architectureTypeValue.getPropertyAsValue();
            architectureValueToListenTo.addListener (this);
        }

        //==============================================================================
        int getWarningLevel() const                       { return warningLevelValue.get(); }
        bool areWarningsTreatedAsErrors() const           { return warningsAreErrorsValue.get(); }

        String getPrebuildCommandString() const           { return prebuildCommandValue.get(); }
        String getPostbuildCommandString() const          { return postbuildCommandValue.get(); }
        String getVSTBinaryLocationString() const         { return vstBinaryLocation.get(); }
        String getVST3BinaryLocationString() const        { return vst3BinaryLocation.get(); }
        String getRTASBinaryLocationString() const        { return rtasBinaryLocation.get();}
        String getAAXBinaryLocationString() const         { return aaxBinaryLocation.get();}
        String getUnityPluginBinaryLocationString() const { return unityPluginBinaryLocation.get(); }
        String getIntermediatesPathString() const         { return intermediatesPathValue.get(); }
        String getCharacterSetString() const              { return characterSetValue.get(); }
        String get64BitArchName() const                   { return "x64"; }
        String get32BitArchName() const                   { return "Win32"; }
        String getArchitectureString() const              { return architectureTypeValue.get(); }
        String getDebugInformationFormatString() const    { return debugInformationFormatValue.get(); }

        bool shouldGenerateDebugSymbols() const           { return generateDebugSymbolsValue.get(); }
        bool shouldGenerateManifest() const               { return generateManifestValue.get(); }
        bool shouldLinkIncremental() const                { return enableIncrementalLinkingValue.get(); }
        bool isUsingRuntimeLibDLL() const                 { return useRuntimeLibDLLValue.get(); }
        bool shouldUseMultiProcessorCompilation() const   { return multiProcessorCompilationValue.get(); }
        bool is64Bit() const                              { return getArchitectureString() == get64BitArchName(); }
        bool isFastMathEnabled() const                    { return fastMathValue.get(); }
        bool isPluginBinaryCopyStepEnabled() const        { return pluginBinaryCopyStepValue.get(); }

        //==============================================================================
        String createMSVCConfigName() const
        {
            return getName() + "|" + (is64Bit() ? "x64" : "Win32");
        }

        String getOutputFilename (const String& suffix, bool forceSuffix, bool forceUnityPrefix) const
        {
            auto target = File::createLegalFileName (getTargetBinaryNameString (forceUnityPrefix).trim());

            if (forceSuffix || ! target.containsChar ('.'))
                return target.upToLastOccurrenceOf (".", false, false) + suffix;

            return target;
        }

        void createConfigProperties (PropertyListBuilder& props) override
        {
            if (project.isAudioPluginProject())
                addVisualStudioPluginInstallPathProperties (props);

            props.add (new ChoicePropertyComponent (architectureTypeValue, "Architecture",
                                                    { get32BitArchName(), get64BitArchName() },
                                                    { get32BitArchName(), get64BitArchName() }),
                       "Whether to use a 32-bit or 64-bit architecture.");


            props.add (new ChoicePropertyComponentWithEnablement (debugInformationFormatValue,
                                                                  isDebug() ? isDebugValue : generateDebugSymbolsValue,
                                                                  "Debug Information Format",
                                                                  { "None", "C7 Compatible (/Z7)", "Program Database (/Zi)", "Program Database for Edit And Continue (/ZI)" },
                                                                  { "None", "OldStyle",            "ProgramDatabase",        "EditAndContinue" }),
                       "The type of debugging information created for your program for this configuration."
                       " This will always be used in a debug configuration and will be used in a release configuration"
                       " with forced generation of debug symbols.");

            props.add (new ChoicePropertyComponent (fastMathValue, "Relax IEEE Compliance"),
                       "Enable this to use FAST_MATH non-IEEE mode. (Warning: this can have unexpected results!)");

            props.add (new ChoicePropertyComponent (optimisationLevelValue, "Optimisation",
                                                    { "Disabled (/Od)", "Minimise size (/O1)", "Maximise speed (/O2)", "Full optimisation (/Ox)" },
                                                    { optimisationOff,  optimiseMinSize,       optimiseMaxSpeed,       optimiseFull }),
                       "The optimisation level for this configuration");

            props.add (new TextPropertyComponent (intermediatesPathValue, "Intermediates Path", 2048, false),
                       "An optional path to a folder to use for the intermediate build files. Note that Visual Studio allows "
                       "you to use macros in this path, e.g. \"$(TEMP)\\MyAppBuildFiles\\$(Configuration)\", which is a handy way to "
                       "send them to the user's temp folder.");

            props.add (new ChoicePropertyComponent (warningLevelValue, "Warning Level",
                                                    { "Low", "Medium", "High" },
                                                    { 2,     3,        4 }),
                       "The compilation warning level to use.");

            props.add (new ChoicePropertyComponent (warningsAreErrorsValue, "Treat Warnings as Errors"),
                       "Enable this to treat compilation warnings as errors.");

            props.add (new ChoicePropertyComponent (useRuntimeLibDLLValue, "Runtime Library",
                                                    { "Use static runtime", "Use DLL runtime" },
                                                    { false,                true }),
                       "If the static runtime is selected then your app/plug-in will not be dependent upon users having Microsoft's redistributable "
                       "C++ runtime installed. However, if you are linking libraries from different sources you must select the same type of runtime "
                       "used by the libraries.");

            props.add (new ChoicePropertyComponent (multiProcessorCompilationValue, "Multi-Processor Compilation",
                                                    { "Enabled", "Disabled" },
                                                    { true,      false }),
                       "Allows the compiler to use of all the available processors, which can reduce compilation time. "
                       "This is enabled by default and should only be disabled if you know what you are doing.");

            props.add (new ChoicePropertyComponent (enableIncrementalLinkingValue, "Incremental Linking"),
                       "Enable to avoid linking from scratch for every new build. "
                       "Disable to ensure that your final release build does not contain padding or thunks.");

            if (! isDebug())
            {
                props.add (new ChoicePropertyComponent (generateDebugSymbolsValue, "Force Generation of Debug Symbols"),
                           "Enable this to force generation of debug symbols in a release configuration.");
            }

            props.add (new TextPropertyComponent (prebuildCommandValue,  "Pre-build Command",  2048, true),
                       "Some command that will be run before a build starts.");

            props.add (new TextPropertyComponent (postbuildCommandValue, "Post-build Command", 2048, true),
                       "Some command that will be run after a build starts.");

            props.add (new ChoicePropertyComponent (generateManifestValue, "Generate Manifest"),
                       "Enable this to generate a Manifest file.");

            props.add (new ChoicePropertyComponent (characterSetValue, "Character Set",
                                                    { "MultiByte", "Unicode" },
                                                    { "MultiByte", "Unicode" }),
                       "Specifies the character set used when building.");
        }

        String getModuleLibraryArchName() const override
        {
            String result ("$(Platform)\\");
            result += isUsingRuntimeLibDLL() ? "MD" : "MT";

            if (isDebug())
                result += "d";

            return result;
        }

        void updateOldLTOSetting()
        {
            if (! isDebug() && config.getPropertyAsValue ("wholeProgramOptimisation", nullptr) != Value())
                linkTimeOptimisationValue = (static_cast<int> (config ["wholeProgramOptimisation"]) == 0);
        }

    private:
        ValueWithDefault warningLevelValue, warningsAreErrorsValue, prebuildCommandValue, postbuildCommandValue, generateDebugSymbolsValue,
                         generateManifestValue, enableIncrementalLinkingValue, useRuntimeLibDLLValue, multiProcessorCompilationValue,
                         intermediatesPathValue, characterSetValue, architectureTypeValue, fastMathValue, debugInformationFormatValue,
                         pluginBinaryCopyStepValue;

        ValueWithDefault vstBinaryLocation, vst3BinaryLocation, rtasBinaryLocation, aaxBinaryLocation, unityPluginBinaryLocation;

        Value architectureValueToListenTo;

        //==============================================================================
        void addVisualStudioPluginInstallPathProperties (PropertyListBuilder& props)
        {
            auto isBuildingAnyPlugins = (project.shouldBuildVST() || project.shouldBuildVST3() || project.shouldBuildRTAS()
                                          || project.shouldBuildAAX() || project.shouldBuildUnityPlugin());

            if (isBuildingAnyPlugins)
                props.add (new ChoicePropertyComponent (pluginBinaryCopyStepValue, "Enable Plugin Copy Step"),
                           "Enable this to copy plugin binaries to a specified folder after building.");

            if (project.shouldBuildVST3())
                props.add (new TextPropertyComponentWithEnablement (vst3BinaryLocation, pluginBinaryCopyStepValue, "VST3 Binary Location",
                                                                    1024, false),
                           "The folder in which the compiled VST3 binary should be placed.");

            if (project.shouldBuildRTAS())
                props.add (new TextPropertyComponentWithEnablement (rtasBinaryLocation, pluginBinaryCopyStepValue, "RTAS Binary Location",
                                                                    1024, false),
                           "The folder in which the compiled RTAS binary should be placed.");

            if (project.shouldBuildAAX())
                props.add (new TextPropertyComponentWithEnablement (aaxBinaryLocation, pluginBinaryCopyStepValue, "AAX Binary Location",
                                                                    1024, false),
                           "The folder in which the compiled AAX binary should be placed.");

            if (project.shouldBuildUnityPlugin())
                props.add (new TextPropertyComponentWithEnablement (unityPluginBinaryLocation, pluginBinaryCopyStepValue, "Unity Binary Location",
                                                                    1024, false),
                           "The folder in which the compiled Unity plugin binary and associated C# GUI script should be placed.");

            if (project.shouldBuildVST())
                props.add (new TextPropertyComponentWithEnablement (vstBinaryLocation, pluginBinaryCopyStepValue, "VST (Legacy) Binary Location",
                                                                    1024, false),
                           "The folder in which the compiled legacy VST binary should be placed.");

        }

        void setPluginBinaryCopyLocationDefaults()
        {
            vstBinaryLocation.setDefault  ((is64Bit() ? "%ProgramW6432%" : "%programfiles(x86)%") + String ("\\Steinberg\\Vstplugins"));

            auto prefix = is64Bit() ? "%CommonProgramW6432%"
                                    : "%CommonProgramFiles(x86)%";

            vst3BinaryLocation.setDefault (prefix + String ("\\VST3"));
            rtasBinaryLocation.setDefault (prefix + String ("\\Digidesign\\DAE\\Plug-Ins"));
            aaxBinaryLocation.setDefault  (prefix + String ("\\Avid\\Audio\\Plug-Ins"));
        }

        void valueChanged (Value&) override
        {
            setPluginBinaryCopyLocationDefaults();
        }
    };

    //==============================================================================
    class MSVCTargetBase : public build_tools::ProjectType::Target
    {
    public:
        MSVCTargetBase (build_tools::ProjectType::Target::Type targetType, const MSVCProjectExporterBase& exporter)
            : build_tools::ProjectType::Target (targetType), owner (exporter)
        {
            projectGuid = createGUID (owner.getProject().getProjectUIDString() + getName());
        }

        virtual ~MSVCTargetBase() {}

        String getProjectVersionString() const     { return "10.00"; }
        String getProjectFileSuffix() const        { return ".vcxproj"; }
        String getFiltersFileSuffix() const        { return ".vcxproj.filters"; }
        String getTopLevelXmlEntity() const        { return "Project"; }

        //==============================================================================
        void fillInProjectXml (XmlElement& projectXml) const
        {
            projectXml.setAttribute ("DefaultTargets", "Build");
            projectXml.setAttribute ("ToolsVersion", getOwner().getToolsVersion());
            projectXml.setAttribute ("xmlns", "http://schemas.microsoft.com/developer/msbuild/2003");

            {
                auto* configsGroup = projectXml.createNewChildElement ("ItemGroup");
                configsGroup->setAttribute ("Label", "ProjectConfigurations");

                for (ConstConfigIterator i (owner); i.next();)
                {
                    auto& config = dynamic_cast<const MSVCBuildConfiguration&> (*i);
                    auto* e = configsGroup->createNewChildElement ("ProjectConfiguration");
                    e->setAttribute ("Include", config.createMSVCConfigName());
                    e->createNewChildElement ("Configuration")->addTextElement (config.getName());
                    e->createNewChildElement ("Platform")->addTextElement (config.is64Bit() ? config.get64BitArchName()
                                                                                            : config.get32BitArchName());
                }
            }

            {
                auto* globals = projectXml.createNewChildElement ("PropertyGroup");
                globals->setAttribute ("Label", "Globals");
                globals->createNewChildElement ("ProjectGuid")->addTextElement (getProjectGuid());
            }

            {
                auto* imports = projectXml.createNewChildElement ("Import");
                imports->setAttribute ("Project", "$(VCTargetsPath)\\Microsoft.Cpp.Default.props");
            }

            for (ConstConfigIterator i (owner); i.next();)
            {
                auto& config = dynamic_cast<const MSVCBuildConfiguration&> (*i);

                auto* e = projectXml.createNewChildElement ("PropertyGroup");
                setConditionAttribute (*e, config);
                e->setAttribute ("Label", "Configuration");
                e->createNewChildElement ("ConfigurationType")->addTextElement (getProjectType());
                e->createNewChildElement ("UseOfMfc")->addTextElement ("false");
                e->createNewChildElement ("WholeProgramOptimization")->addTextElement (config.isLinkTimeOptimisationEnabled() ? "true"
                                                                                                                              : "false");

                auto charSet = config.getCharacterSetString();

                if (charSet.isNotEmpty())
                    e->createNewChildElement ("CharacterSet")->addTextElement (charSet);

                if (config.shouldLinkIncremental())
                    e->createNewChildElement ("LinkIncremental")->addTextElement ("true");

                e->createNewChildElement ("PlatformToolset")->addTextElement (owner.getPlatformToolset());

                addWindowsTargetPlatformToConfig (*e);

                auto ippLibrary = owner.getIPPLibrary();

                if (ippLibrary.isNotEmpty())
                    e->createNewChildElement ("UseIntelIPP")->addTextElement (ippLibrary);
            }

            {
                auto* e = projectXml.createNewChildElement ("Import");
                e->setAttribute ("Project", "$(VCTargetsPath)\\Microsoft.Cpp.props");
            }

            {
                auto* e = projectXml.createNewChildElement ("ImportGroup");
                e->setAttribute ("Label", "ExtensionSettings");
            }

            {
                auto* e = projectXml.createNewChildElement ("ImportGroup");
                e->setAttribute ("Label", "PropertySheets");
                auto* p = e->createNewChildElement ("Import");
                p->setAttribute ("Project", "$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props");
                p->setAttribute ("Condition", "exists('$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props')");
                p->setAttribute ("Label", "LocalAppDataPlatform");
            }

            {
                auto* props = projectXml.createNewChildElement ("PropertyGroup");
                props->createNewChildElement ("_ProjectFileVersion")->addTextElement ("10.0.30319.1");
                props->createNewChildElement ("TargetExt")->addTextElement (getTargetSuffix());

                for (ConstConfigIterator i (owner); i.next();)
                {
                    auto& config = dynamic_cast<const MSVCBuildConfiguration&> (*i);

                    if (getConfigTargetPath (config).isNotEmpty())
                    {
                        auto* outdir = props->createNewChildElement ("OutDir");
                        setConditionAttribute (*outdir, config);
                        outdir->addTextElement (build_tools::windowsStylePath (getConfigTargetPath (config)) + "\\");
                    }

                    {
                        auto* intdir = props->createNewChildElement ("IntDir");
                        setConditionAttribute (*intdir, config);

                        auto intermediatesPath = getIntermediatesPath (config);
                        if (! intermediatesPath.endsWithChar (L'\\'))
                            intermediatesPath += L'\\';

                        intdir->addTextElement (build_tools::windowsStylePath (intermediatesPath));
                    }


                    {
                        auto* targetName = props->createNewChildElement ("TargetName");
                        setConditionAttribute (*targetName, config);
                        targetName->addTextElement (config.getOutputFilename ("", false, type == UnityPlugIn));
                    }

                    {
                        auto* manifest = props->createNewChildElement ("GenerateManifest");
                        setConditionAttribute (*manifest, config);
                        manifest->addTextElement (config.shouldGenerateManifest() ? "true" : "false");
                    }

                    if (type != SharedCodeTarget)
                    {
                        auto librarySearchPaths = getLibrarySearchPaths (config);
                        if (librarySearchPaths.size() > 0)
                        {
                            auto* libPath = props->createNewChildElement ("LibraryPath");
                            setConditionAttribute (*libPath, config);
                            libPath->addTextElement ("$(LibraryPath);" + librarySearchPaths.joinIntoString (";"));
                        }
                    }
                }
            }

            for (ConstConfigIterator i (owner); i.next();)
            {
                auto& config = dynamic_cast<const MSVCBuildConfiguration&> (*i);

                bool isDebug = config.isDebug();

                auto* group = projectXml.createNewChildElement ("ItemDefinitionGroup");
                setConditionAttribute (*group, config);

                {
                    auto* midl = group->createNewChildElement ("Midl");
                    midl->createNewChildElement ("PreprocessorDefinitions")->addTextElement (isDebug ? "_DEBUG;%(PreprocessorDefinitions)"
                                                                                                     : "NDEBUG;%(PreprocessorDefinitions)");
                    midl->createNewChildElement ("MkTypLibCompatible")->addTextElement ("true");
                    midl->createNewChildElement ("SuppressStartupBanner")->addTextElement ("true");
                    midl->createNewChildElement ("TargetEnvironment")->addTextElement ("Win32");
                    midl->createNewChildElement ("HeaderFileName");
                }

                bool isUsingEditAndContinue = false;

                {
                    auto* cl = group->createNewChildElement ("ClCompile");

                    cl->createNewChildElement ("Optimization")->addTextElement (getOptimisationLevelString (config.getOptimisationLevelInt()));

                    if (isDebug || config.shouldGenerateDebugSymbols())
                    {
                        cl->createNewChildElement ("DebugInformationFormat")
                            ->addTextElement (config.getDebugInformationFormatString());
                    }

                    auto includePaths = getOwner().getHeaderSearchPaths (config);
                    includePaths.addArray (getExtraSearchPaths());
                    includePaths.add ("%(AdditionalIncludeDirectories)");

                    cl->createNewChildElement ("AdditionalIncludeDirectories")->addTextElement (includePaths.joinIntoString (";"));
                    cl->createNewChildElement ("PreprocessorDefinitions")->addTextElement (getPreprocessorDefs (config, ";") + ";%(PreprocessorDefinitions)");

                    cl->createNewChildElement ("RuntimeLibrary")->addTextElement (config.isUsingRuntimeLibDLL() ? (isDebug ? "MultiThreadedDebugDLL" : "MultiThreadedDLL")
                                                                                                                : (isDebug ? "MultiThreadedDebug"    : "MultiThreaded"));
                    cl->createNewChildElement ("RuntimeTypeInfo")->addTextElement ("true");
                    cl->createNewChildElement ("PrecompiledHeader")->addTextElement ("NotUsing");
                    cl->createNewChildElement ("AssemblerListingLocation")->addTextElement ("$(IntDir)\\");
                    cl->createNewChildElement ("ObjectFileName")->addTextElement ("$(IntDir)\\");
                    cl->createNewChildElement ("ProgramDataBaseFileName")->addTextElement ("$(IntDir)\\");
                    cl->createNewChildElement ("WarningLevel")->addTextElement ("Level" + String (config.getWarningLevel()));
                    cl->createNewChildElement ("SuppressStartupBanner")->addTextElement ("true");
                    cl->createNewChildElement ("MultiProcessorCompilation")->addTextElement (config.shouldUseMultiProcessorCompilation() ? "true" : "false");

                    if (config.isFastMathEnabled())
                        cl->createNewChildElement ("FloatingPointModel")->addTextElement ("Fast");

                    auto extraFlags = getOwner().replacePreprocessorTokens (config, getOwner().getExtraCompilerFlagsString()).trim();
                    if (extraFlags.isNotEmpty())
                        cl->createNewChildElement ("AdditionalOptions")->addTextElement (extraFlags + " %(AdditionalOptions)");

                    if (config.areWarningsTreatedAsErrors())
                        cl->createNewChildElement ("TreatWarningAsError")->addTextElement ("true");

                    auto cppStandard = owner.project.getCppStandardString();

                    if (cppStandard == "11") // VS doesn't support the C++11 flag so we have to bump it to C++14
                        cppStandard = "14";

                    cl->createNewChildElement ("LanguageStandard")->addTextElement ("stdcpp" + cppStandard);
                }

                {
                    auto* res = group->createNewChildElement ("ResourceCompile");
                    res->createNewChildElement ("PreprocessorDefinitions")->addTextElement (isDebug ? "_DEBUG;%(PreprocessorDefinitions)"
                                                                                                    : "NDEBUG;%(PreprocessorDefinitions)");
                }

                auto externalLibraries = getExternalLibraries (config, getOwner().getExternalLibrariesString());
                auto additionalDependencies = type != SharedCodeTarget && externalLibraries.isNotEmpty()
                                                        ? getOwner().replacePreprocessorTokens (config, externalLibraries).trim() + ";%(AdditionalDependencies)"
                                                        : String();

                auto librarySearchPaths = config.getLibrarySearchPaths();
                auto additionalLibraryDirs = type != SharedCodeTarget && librarySearchPaths.size() > 0
                                                       ? getOwner().replacePreprocessorTokens (config, librarySearchPaths.joinIntoString (";")) + ";%(AdditionalLibraryDirectories)"
                                                       : String();

                {
                    auto* link = group->createNewChildElement ("Link");
                    link->createNewChildElement ("OutputFile")->addTextElement (getOutputFilePath (config, type == UnityPlugIn));
                    link->createNewChildElement ("SuppressStartupBanner")->addTextElement ("true");
                    link->createNewChildElement ("IgnoreSpecificDefaultLibraries")->addTextElement (isDebug ? "libcmt.lib; msvcrt.lib;;%(IgnoreSpecificDefaultLibraries)"
                                                                                                            : "%(IgnoreSpecificDefaultLibraries)");
                    link->createNewChildElement ("GenerateDebugInformation")->addTextElement ((isDebug || config.shouldGenerateDebugSymbols()) ? "true" : "false");
                    link->createNewChildElement ("ProgramDatabaseFile")->addTextElement (getOwner().getIntDirFile (config, config.getOutputFilename (".pdb", true, type == UnityPlugIn)));
                    link->createNewChildElement ("SubSystem")->addTextElement (type == ConsoleApp ? "Console" : "Windows");

                    if (! config.is64Bit())
                        link->createNewChildElement ("TargetMachine")->addTextElement ("MachineX86");

                    if (isUsingEditAndContinue)
                        link->createNewChildElement ("ImageHasSafeExceptionHandlers")->addTextElement ("false");

                    if (! isDebug)
                    {
                        link->createNewChildElement ("OptimizeReferences")->addTextElement ("true");
                        link->createNewChildElement ("EnableCOMDATFolding")->addTextElement ("true");
                    }

                    if (additionalLibraryDirs.isNotEmpty())
                        link->createNewChildElement ("AdditionalLibraryDirectories")->addTextElement (additionalLibraryDirs);

                    link->createNewChildElement ("LargeAddressAware")->addTextElement ("true");

                    if (config.isLinkTimeOptimisationEnabled())
                        link->createNewChildElement ("LinkTimeCodeGeneration")->addTextElement ("UseLinkTimeCodeGeneration");

                    if (additionalDependencies.isNotEmpty())
                        link->createNewChildElement ("AdditionalDependencies")->addTextElement (additionalDependencies);

                    auto extraLinkerOptions = getOwner().getExtraLinkerFlagsString();
                    if (extraLinkerOptions.isNotEmpty())
                        link->createNewChildElement ("AdditionalOptions")->addTextElement (getOwner().replacePreprocessorTokens (config, extraLinkerOptions).trim()
                                                                                           + " %(AdditionalOptions)");

                    auto delayLoadedDLLs = getDelayLoadedDLLs();
                    if (delayLoadedDLLs.isNotEmpty())
                        link->createNewChildElement ("DelayLoadDLLs")->addTextElement (delayLoadedDLLs);

                    auto moduleDefinitionsFile = getModuleDefinitions (config);
                    if (moduleDefinitionsFile.isNotEmpty())
                        link->createNewChildElement ("ModuleDefinitionFile")
                            ->addTextElement (moduleDefinitionsFile);
                }

                {
                    auto* bsc = group->createNewChildElement ("Bscmake");
                    bsc->createNewChildElement ("SuppressStartupBanner")->addTextElement ("true");
                    bsc->createNewChildElement ("OutputFile")->addTextElement (getOwner().getIntDirFile (config, config.getOutputFilename (".bsc", true, type == UnityPlugIn)));
                }

                if (type != SharedCodeTarget)
                {
                    auto* lib = group->createNewChildElement ("Lib");

                    if (additionalDependencies.isNotEmpty())
                        lib->createNewChildElement ("AdditionalDependencies")->addTextElement (additionalDependencies);

                    if (additionalLibraryDirs.isNotEmpty())
                        lib->createNewChildElement ("AdditionalLibraryDirectories")->addTextElement (additionalLibraryDirs);
                }

                auto manifestFile = getOwner().getManifestPath();
                if (manifestFile.getRoot() != build_tools::RelativePath::unknown)
                {
                    auto* bsc = group->createNewChildElement ("Manifest");
                    bsc->createNewChildElement ("AdditionalManifestFiles")
                       ->addTextElement (manifestFile.rebased (getOwner().getProject().getFile().getParentDirectory(),
                                                               getOwner().getTargetFolder(),
                                                               build_tools::RelativePath::buildTargetFolder).toWindowsStyle());
                }

                if (getTargetFileType() == staticLibrary && ! config.is64Bit())
                {
                    auto* lib = group->createNewChildElement ("Lib");
                    lib->createNewChildElement ("TargetMachine")->addTextElement ("MachineX86");
                }

                auto preBuild = getPreBuildSteps (config);
                if (preBuild.isNotEmpty())
                    group->createNewChildElement ("PreBuildEvent")
                         ->createNewChildElement ("Command")
                         ->addTextElement (preBuild);

                auto postBuild = getPostBuildSteps (config);
                if (postBuild.isNotEmpty())
                    group->createNewChildElement ("PostBuildEvent")
                         ->createNewChildElement ("Command")
                         ->addTextElement (postBuild);
            }

            std::unique_ptr<XmlElement> otherFilesGroup (new XmlElement ("ItemGroup"));

            {
                auto* cppFiles    = projectXml.createNewChildElement ("ItemGroup");
                auto* headerFiles = projectXml.createNewChildElement ("ItemGroup");

                writePrecompiledHeaderFiles (*cppFiles);

                for (int i = 0; i < getOwner().getAllGroups().size(); ++i)
                {
                    auto& group = getOwner().getAllGroups().getReference (i);

                    if (group.getNumChildren() > 0)
                        addFilesToCompile (group, *cppFiles, *headerFiles, *otherFilesGroup);
                }
            }

            if (getOwner().iconFile.existsAsFile())
            {
                auto* e = otherFilesGroup->createNewChildElement ("None");
                e->setAttribute ("Include", prependDot (getOwner().iconFile.getFileName()));
            }

            if (getOwner().packagesConfigFile.existsAsFile())
            {
                auto* e = otherFilesGroup->createNewChildElement ("None");
                e->setAttribute ("Include", getOwner().packagesConfigFile.getFileName());
            }

            if (otherFilesGroup->getFirstChildElement() != nullptr)
                projectXml.addChildElement (otherFilesGroup.release());

            if (type != SharedCodeTarget && getOwner().hasResourceFile())
            {
                auto* rcGroup = projectXml.createNewChildElement ("ItemGroup");
                auto* e = rcGroup->createNewChildElement ("ResourceCompile");
                e->setAttribute ("Include", prependDot (getOwner().rcFile.getFileName()));
            }

            {
                auto* e = projectXml.createNewChildElement ("Import");
                e->setAttribute ("Project", "$(VCTargetsPath)\\Microsoft.Cpp.targets");
            }

            {
                auto* importGroup = projectXml.createNewChildElement ("ImportGroup");
                importGroup->setAttribute ("Label", "ExtensionTargets");

                if (owner.shouldAddWebView2Package())
                {
                    auto packageTargetsPath = "packages\\" + getWebView2PackageName() + "." + getWebView2PackageVersion()
                                            + "\\build\\native\\" + getWebView2PackageName() + ".targets";

                    auto* e = importGroup->createNewChildElement ("Import");
                    e->setAttribute ("Project", packageTargetsPath);
                    e->setAttribute ("Condition", "Exists('" + packageTargetsPath + "')");
                }
            }
        }

        String getProjectType() const
        {
            auto targetFileType = getTargetFileType();

            if (targetFileType == executable)     return "Application";
            if (targetFileType == staticLibrary)  return "StaticLibrary";

            return "DynamicLibrary";
        }

        //==============================================================================
        static void setSourceFilePCHSettings (XmlElement& element, const File& pchFile, const String& option, const BuildConfiguration& config)
        {
            auto setConfigConditionAttribute = [&config] (XmlElement* elementToSet) -> XmlElement*
            {
                setConditionAttribute (*elementToSet, config);
                return elementToSet;
            };

            setConfigConditionAttribute (element.createNewChildElement ("PrecompiledHeader"))->addTextElement (option);
            setConfigConditionAttribute (element.createNewChildElement ("PrecompiledHeaderFile"))->addTextElement (pchFile.getFileName());
            setConfigConditionAttribute (element.createNewChildElement ("PrecompiledHeaderOutputFile"))->addTextElement ("$(Platform)\\$(Configuration)\\JucePrecompiledHeader.pch");
            setConfigConditionAttribute (element.createNewChildElement ("ForcedIncludeFiles"))->addTextElement (pchFile.getFileName());
        }

        void writePrecompiledHeaderFiles (XmlElement& cpps) const
        {
            for (ConstConfigIterator config (owner); config.next();)
            {
                if (config->shouldUsePrecompiledHeaderFile())
                {
                    auto pchFileContent = config->getPrecompiledHeaderFileContent();

                    if (pchFileContent.isNotEmpty())
                    {
                        auto pchFile = owner.getTargetFolder().getChildFile (config->getPrecompiledHeaderFilename()).withFileExtension (".h");

                        build_tools::writeStreamToFile (pchFile, [&] (MemoryOutputStream& mo)
                        {
                            mo << pchFileContent;
                        });

                        auto pchSourceFile = pchFile.withFileExtension (".cpp");

                        build_tools::writeStreamToFile (pchSourceFile, [this] (MemoryOutputStream& mo)
                        {
                            mo.setNewLineString (owner.getNewLineString());

                            writeAutoGenWarningComment (mo);

                            mo << "    This is an empty source file generated by JUCE required for Visual Studio PCH." << newLine
                                << newLine
                                << "*/" << newLine
                                << newLine;
                        });

                        auto* pchSourceElement = cpps.createNewChildElement ("ClCompile");
                        pchSourceElement->setAttribute ("Include", prependDot (pchSourceFile.getFileName()));
                        setSourceFilePCHSettings (*pchSourceElement, pchFile, "Create", *config);
                    }
                }
            }
        }

        void addFilesToCompile (const Project::Item& projectItem, XmlElement& cpps, XmlElement& headers, XmlElement& otherFiles) const
        {
            auto targetType = (getOwner().getProject().isAudioPluginProject() ? type : SharedCodeTarget);

            if (projectItem.isGroup())
            {
                for (int i = 0; i < projectItem.getNumChildren(); ++i)
                    addFilesToCompile (projectItem.getChild (i), cpps, headers, otherFiles);
            }
            else if (projectItem.shouldBeAddedToTargetProject() && projectItem.shouldBeAddedToTargetExporter (getOwner())
                     && getOwner().getProject().getTargetTypeFromFilePath (projectItem.getFile(), true) == targetType)
            {
                build_tools::RelativePath path (projectItem.getFile(), getOwner().getTargetFolder(), build_tools::RelativePath::buildTargetFolder);

                jassert (path.getRoot() == build_tools::RelativePath::buildTargetFolder);

                if (path.hasFileExtension (cOrCppFileExtensions) || path.hasFileExtension (asmFileExtensions))
                {
                    auto* e = cpps.createNewChildElement ("ClCompile");
                    e->setAttribute ("Include", path.toWindowsStyle());

                    if (shouldUseStdCall (path))
                        e->createNewChildElement ("CallingConvention")->addTextElement ("StdCall");

                    if (projectItem.shouldBeCompiled())
                    {
                        auto extraCompilerFlags = owner.compilerFlagSchemesMap[projectItem.getCompilerFlagSchemeString()].get().toString();

                        if (extraCompilerFlags.isNotEmpty())
                            e->createNewChildElement ("AdditionalOptions")->addTextElement (extraCompilerFlags + " %(AdditionalOptions)");

                        if (! projectItem.shouldSkipPCH())
                        {
                            for (ConstConfigIterator i (owner); i.next();)
                            {
                                if (i->shouldUsePrecompiledHeaderFile())
                                {
                                    auto pchFile = owner.getTargetFolder().getChildFile (i->getPrecompiledHeaderFilename()).withFileExtension (".h");

                                    if (pchFile.existsAsFile())
                                        setSourceFilePCHSettings (*e, pchFile, "Use", *i);
                                }
                            }
                        }
                    }
                    else
                    {
                        e->createNewChildElement ("ExcludedFromBuild")->addTextElement ("true");
                    }
                }
                else if (path.hasFileExtension (headerFileExtensions))
                {
                    headers.createNewChildElement ("ClInclude")->setAttribute ("Include", path.toWindowsStyle());
                }
                else if (! path.hasFileExtension (objCFileExtensions))
                {
                    otherFiles.createNewChildElement ("None")->setAttribute ("Include", path.toWindowsStyle());
                }
            }
        }

        static void setConditionAttribute (XmlElement& xml, const BuildConfiguration& config)
        {
            auto& msvcConfig = dynamic_cast<const MSVCBuildConfiguration&> (config);
            xml.setAttribute ("Condition", "'$(Configuration)|$(Platform)'=='" + msvcConfig.createMSVCConfigName() + "'");
        }

        //==============================================================================
        void addFilterGroup (XmlElement& groups, const String& path) const
        {
            auto* e = groups.createNewChildElement ("Filter");
            e->setAttribute ("Include", path);
            e->createNewChildElement ("UniqueIdentifier")->addTextElement (createGUID (path + "_guidpathsaltxhsdf"));
        }

        void addFileToFilter (const build_tools::RelativePath& file, const String& groupPath,
                              XmlElement& cpps, XmlElement& headers, XmlElement& otherFiles) const
        {
            XmlElement* e = nullptr;

            if (file.hasFileExtension (headerFileExtensions))
                e = headers.createNewChildElement ("ClInclude");
            else if (file.hasFileExtension (sourceFileExtensions))
                e = cpps.createNewChildElement ("ClCompile");
            else
                e = otherFiles.createNewChildElement ("None");

            jassert (file.getRoot() == build_tools::RelativePath::buildTargetFolder);
            e->setAttribute ("Include", file.toWindowsStyle());
            e->createNewChildElement ("Filter")->addTextElement (groupPath);
        }

        bool addFilesToFilter (const Project::Item& projectItem, const String& path,
                               XmlElement& cpps, XmlElement& headers, XmlElement& otherFiles, XmlElement& groups) const
        {
            auto targetType = (getOwner().getProject().isAudioPluginProject() ? type : SharedCodeTarget);

            if (projectItem.isGroup())
            {
                bool filesWereAdded = false;

                for (int i = 0; i < projectItem.getNumChildren(); ++i)
                    if (addFilesToFilter (projectItem.getChild(i),
                                          (path.isEmpty() ? String() : (path + "\\")) + projectItem.getChild(i).getName(),
                                          cpps, headers, otherFiles, groups))
                        filesWereAdded = true;

                if (filesWereAdded)
                    addFilterGroup (groups, path);

                return filesWereAdded;
            }
            else if (projectItem.shouldBeAddedToTargetProject()
                     && projectItem.shouldBeAddedToTargetExporter (getOwner())
                     && getOwner().getProject().getTargetTypeFromFilePath (projectItem.getFile(), true) == targetType)
            {
                build_tools::RelativePath relativePath (projectItem.getFile(),
                                                        getOwner().getTargetFolder(),
                                                        build_tools::RelativePath::buildTargetFolder);

                jassert (relativePath.getRoot() == build_tools::RelativePath::buildTargetFolder);

                addFileToFilter (relativePath, path.upToLastOccurrenceOf ("\\", false, false), cpps, headers, otherFiles);
                return true;
            }

            return false;
        }

        void fillInFiltersXml (XmlElement& filterXml) const
        {
            filterXml.setAttribute ("ToolsVersion", getOwner().getToolsVersion());
            filterXml.setAttribute ("xmlns", "http://schemas.microsoft.com/developer/msbuild/2003");

            auto* groupsXml  = filterXml.createNewChildElement ("ItemGroup");
            auto* cpps       = filterXml.createNewChildElement ("ItemGroup");
            auto* headers    = filterXml.createNewChildElement ("ItemGroup");
            std::unique_ptr<XmlElement> otherFilesGroup (new XmlElement ("ItemGroup"));

            for (int i = 0; i < getOwner().getAllGroups().size(); ++i)
            {
                auto& group = getOwner().getAllGroups().getReference(i);

                if (group.getNumChildren() > 0)
                    addFilesToFilter (group, group.getName(), *cpps, *headers, *otherFilesGroup, *groupsXml);
            }

            if (getOwner().iconFile.existsAsFile())
            {
                auto* e = otherFilesGroup->createNewChildElement ("None");
                e->setAttribute ("Include", prependDot (getOwner().iconFile.getFileName()));
                e->createNewChildElement ("Filter")->addTextElement (ProjectSaver::getJuceCodeGroupName());
            }

            if (getOwner().packagesConfigFile.existsAsFile())
            {
                auto* e = otherFilesGroup->createNewChildElement ("None");
                e->setAttribute ("Include", getOwner().packagesConfigFile.getFileName());
            }

            if (otherFilesGroup->getFirstChildElement() != nullptr)
                filterXml.addChildElement (otherFilesGroup.release());

            if (type != SharedCodeTarget && getOwner().hasResourceFile())
            {
                auto* rcGroup = filterXml.createNewChildElement ("ItemGroup");
                auto* e = rcGroup->createNewChildElement ("ResourceCompile");
                e->setAttribute ("Include", prependDot (getOwner().rcFile.getFileName()));
                e->createNewChildElement ("Filter")->addTextElement (ProjectSaver::getJuceCodeGroupName());
            }
        }

        const MSVCProjectExporterBase& getOwner() const { return owner; }
        const String& getProjectGuid() const            { return projectGuid; }

        //==============================================================================
        void writeProjectFile()
        {
            {
                XmlElement projectXml (getTopLevelXmlEntity());
                fillInProjectXml (projectXml);
                writeXmlOrThrow (projectXml, getVCProjFile(), "UTF-8", 10);
            }

            {
                XmlElement filtersXml (getTopLevelXmlEntity());
                fillInFiltersXml (filtersXml);
                writeXmlOrThrow (filtersXml, getVCProjFiltersFile(), "UTF-8", 100);
            }
        }

        String getSolutionTargetPath (const BuildConfiguration& config) const
        {
            auto binaryPath = config.getTargetBinaryRelativePathString().trim();
            if (binaryPath.isEmpty())
                return "$(SolutionDir)$(Platform)\\$(Configuration)";

            build_tools::RelativePath binaryRelPath (binaryPath, build_tools::RelativePath::projectFolder);

            if (binaryRelPath.isAbsolute())
                return binaryRelPath.toWindowsStyle();

            return prependDot (binaryRelPath.rebased (getOwner().projectFolder,
                                                      getOwner().getTargetFolder(),
                                                      build_tools::RelativePath::buildTargetFolder)
                               .toWindowsStyle());
        }

        String getConfigTargetPath (const BuildConfiguration& config) const
        {
            auto solutionTargetFolder = getSolutionTargetPath (config);
            return solutionTargetFolder + "\\" + getName();
        }

        String getIntermediatesPath (const MSVCBuildConfiguration& config) const
        {
            auto intDir = (config.getIntermediatesPathString().isNotEmpty() ? config.getIntermediatesPathString()
                                                                            : "$(Platform)\\$(Configuration)");

            if (! intDir.endsWithChar (L'\\'))
                intDir += L'\\';

            return intDir + getName();
        }

        static const char* getOptimisationLevelString (int level)
        {
            switch (level)
            {
                case optimiseMinSize:   return "MinSpace";
                case optimiseMaxSpeed:  return "MaxSpeed";
                case optimiseFull:      return "Full";
                default:                return "Disabled";
            }
        }

        String getTargetSuffix() const
        {
            auto fileType = getTargetFileType();

            if (fileType == executable)          return ".exe";
            if (fileType == staticLibrary)       return ".lib";
            if (fileType == sharedLibraryOrDLL)  return ".dll";

            if (fileType == pluginBundle)
            {
                if (type == VST3PlugIn)  return ".vst3";
                if (type == AAXPlugIn)   return ".aaxdll";
                if (type == RTASPlugIn)  return ".dpm";

                return ".dll";
            }

            return {};
        }

        XmlElement* createToolElement (XmlElement& parent, const String& toolName) const
        {
            auto* e = parent.createNewChildElement ("Tool");
            e->setAttribute ("Name", toolName);
            return e;
        }

        String getPreprocessorDefs (const BuildConfiguration& config, const String& joinString) const
        {
            auto defines = getOwner().msvcExtraPreprocessorDefs;
            defines.set ("WIN32", "");
            defines.set ("_WINDOWS", "");

            if (config.isDebug())
            {
                defines.set ("DEBUG", "");
                defines.set ("_DEBUG", "");
            }
            else
            {
                defines.set ("NDEBUG", "");
            }

            defines = mergePreprocessorDefs (defines, getOwner().getAllPreprocessorDefs (config, type));
            addExtraPreprocessorDefines (defines);

            if (getTargetFileType() == staticLibrary || getTargetFileType() == sharedLibraryOrDLL)
                defines.set("_LIB", "");

            StringArray result;

            for (int i = 0; i < defines.size(); ++i)
            {
                auto def = defines.getAllKeys()[i];
                auto value = defines.getAllValues()[i];
                if (value.isNotEmpty())
                    def << "=" << value;

                result.add (def);
            }

            return result.joinIntoString (joinString);
        }

        //==============================================================================
        build_tools::RelativePath getAAXIconFile() const
        {
            build_tools::RelativePath aaxSDK (owner.getAAXPathString(), build_tools::RelativePath::projectFolder);
            build_tools::RelativePath projectIcon ("icon.ico", build_tools::RelativePath::buildTargetFolder);

            if (getOwner().getTargetFolder().getChildFile ("icon.ico").existsAsFile())
                return projectIcon.rebased (getOwner().getTargetFolder(),
                                            getOwner().getProject().getProjectFolder(),
                                            build_tools::RelativePath::projectFolder);

            return aaxSDK.getChildFile ("Utilities").getChildFile ("PlugIn.ico");
        }

        String getExtraPostBuildSteps (const MSVCBuildConfiguration& config) const
        {
            if (type == AAXPlugIn)
            {
                build_tools::RelativePath aaxSDK (owner.getAAXPathString(), build_tools::RelativePath::projectFolder);
                build_tools::RelativePath aaxLibsFolder = aaxSDK.getChildFile ("Libs");
                build_tools::RelativePath bundleScript  = aaxSDK.getChildFile ("Utilities").getChildFile ("CreatePackage.bat");
                build_tools::RelativePath iconFilePath  = getAAXIconFile();

                auto outputFilename = config.getOutputFilename (".aaxplugin", true, false);
                auto bundleDir      = getOwner().getOutDirFile (config, outputFilename);
                auto bundleContents = bundleDir + "\\Contents";
                auto archDir        = bundleContents + String ("\\") + (config.is64Bit() ? "x64" : "Win32");
                auto executable     = archDir + String ("\\") + outputFilename;

                auto pkgScript = String ("copy /Y ") + getOutputFilePath (config, false).quoted() + String (" ") + executable.quoted() + String ("\r\ncall ")
                                     + createRebasedPath (bundleScript) + String (" ") + archDir.quoted() + String (" ") + createRebasedPath (iconFilePath);

                if (config.isPluginBinaryCopyStepEnabled())
                    return pkgScript + "\r\n" + "xcopy " + bundleDir.quoted() + " "
                               + String (config.getAAXBinaryLocationString() + "\\" + outputFilename + "\\").quoted() + " /E /H /K /R /Y";

                return pkgScript;
            }
            else if (type == UnityPlugIn)
            {
                build_tools::RelativePath scriptPath (config.project.getGeneratedCodeFolder().getChildFile (config.project.getUnityScriptName()),
                                                    getOwner().getTargetFolder(),
                                                    build_tools::RelativePath::projectFolder);

                auto pkgScript = String ("copy /Y ") + scriptPath.toWindowsStyle().quoted() + " \"$(OutDir)\"";

                if (config.isPluginBinaryCopyStepEnabled())
                {
                    auto copyLocation = config.getUnityPluginBinaryLocationString();

                    pkgScript += "\r\ncopy /Y \"$(OutDir)$(TargetFileName)\" " +  String (copyLocation + "\\$(TargetFileName)").quoted();
                    pkgScript += "\r\ncopy /Y " + String ("$(OutDir)" + config.project.getUnityScriptName()).quoted() + " " + String (copyLocation + "\\" + config.project.getUnityScriptName()).quoted();
                }

                return pkgScript;
            }
            else if (config.isPluginBinaryCopyStepEnabled())
            {
                auto copyScript = String ("copy /Y \"$(OutDir)$(TargetFileName)\"") + String (" \"$COPYDIR$\\$(TargetFileName)\"");

                if (type == VSTPlugIn)     return copyScript.replace ("$COPYDIR$", config.getVSTBinaryLocationString());
                if (type == VST3PlugIn)    return copyScript.replace ("$COPYDIR$", config.getVST3BinaryLocationString());
                if (type == RTASPlugIn)    return copyScript.replace ("$COPYDIR$", config.getRTASBinaryLocationString());
            }

            return {};
        }

        String getExtraPreBuildSteps (const MSVCBuildConfiguration& config) const
        {
            if (type == AAXPlugIn)
            {
                String script;

                auto bundleDir      = getOwner().getOutDirFile (config, config.getOutputFilename (".aaxplugin", false, false));
                auto bundleContents = bundleDir + "\\Contents";
                auto archDir        = bundleContents + String ("\\") + (config.is64Bit() ? "x64" : "Win32");

                for (auto& folder : StringArray { bundleDir, bundleContents, archDir })
                    script += String ("if not exist \"") + folder + String ("\" mkdir \"") + folder + String ("\"\r\n");

                return script;
            }

            return {};
        }

        String getPostBuildSteps (const MSVCBuildConfiguration& config) const
        {
            auto postBuild = config.getPostbuildCommandString().replace ("\n", "\r\n");;
            auto extraPostBuild = getExtraPostBuildSteps (config);

            return postBuild + String (postBuild.isNotEmpty() && extraPostBuild.isNotEmpty() ? "\r\n" : "") + extraPostBuild;
        }

        String getPreBuildSteps (const MSVCBuildConfiguration& config) const
        {
            auto preBuild = config.getPrebuildCommandString().replace ("\n", "\r\n");;
            auto extraPreBuild = getExtraPreBuildSteps (config);

            return preBuild + String (preBuild.isNotEmpty() && extraPreBuild.isNotEmpty() ? "\r\n" : "") + extraPreBuild;
        }

        void addExtraPreprocessorDefines (StringPairArray& defines) const
        {
            if (type == AAXPlugIn)
            {
                auto aaxLibsFolder = build_tools::RelativePath (owner.getAAXPathString(), build_tools::RelativePath::projectFolder).getChildFile ("Libs");
                defines.set ("JucePlugin_AAXLibs_path", createRebasedPath (aaxLibsFolder));
            }
            else if (type == RTASPlugIn)
            {
                build_tools::RelativePath rtasFolder (owner.getRTASPathString(), build_tools::RelativePath::projectFolder);
                defines.set ("JucePlugin_WinBag_path", createRebasedPath (rtasFolder.getChildFile ("WinBag")));
            }
        }

        String getExtraLinkerFlags() const
        {
            if (type == RTASPlugIn)
                return "/FORCE:multiple";

            return {};
        }

        StringArray getExtraSearchPaths() const
        {
            StringArray searchPaths;
            if (type == RTASPlugIn)
            {
                build_tools::RelativePath rtasFolder (owner.getRTASPathString(), build_tools::RelativePath::projectFolder);

                static const char* p[] = { "AlturaPorts/TDMPlugins/PluginLibrary/EffectClasses",
                                           "AlturaPorts/TDMPlugins/PluginLibrary/ProcessClasses",
                                           "AlturaPorts/TDMPlugins/PluginLibrary/ProcessClasses/Interfaces",
                                           "AlturaPorts/TDMPlugins/PluginLibrary/Utilities",
                                           "AlturaPorts/TDMPlugins/PluginLibrary/RTASP_Adapt",
                                           "AlturaPorts/TDMPlugins/PluginLibrary/CoreClasses",
                                           "AlturaPorts/TDMPlugins/PluginLibrary/Controls",
                                           "AlturaPorts/TDMPlugins/PluginLibrary/Meters",
                                           "AlturaPorts/TDMPlugins/PluginLibrary/ViewClasses",
                                           "AlturaPorts/TDMPlugins/PluginLibrary/DSPClasses",
                                           "AlturaPorts/TDMPlugins/PluginLibrary/Interfaces",
                                           "AlturaPorts/TDMPlugins/common",
                                           "AlturaPorts/TDMPlugins/common/Platform",
                                           "AlturaPorts/TDMPlugins/common/Macros",
                                           "AlturaPorts/TDMPlugins/SignalProcessing/Public",
                                           "AlturaPorts/TDMPlugIns/DSPManager/Interfaces",
                                           "AlturaPorts/SADriver/Interfaces",
                                           "AlturaPorts/DigiPublic/Interfaces",
                                           "AlturaPorts/DigiPublic",
                                           "AlturaPorts/Fic/Interfaces/DAEClient",
                                           "AlturaPorts/NewFileLibs/Cmn",
                                           "AlturaPorts/NewFileLibs/DOA",
                                           "AlturaPorts/AlturaSource/PPC_H",
                                           "AlturaPorts/AlturaSource/AppSupport",
                                           "AvidCode/AVX2sdk/AVX/avx2/avx2sdk/inc",
                                           "xplat/AVX/avx2/avx2sdk/inc" };

                for (auto* path : p)
                    searchPaths.add (createRebasedPath (rtasFolder.getChildFile (path)));
            }

            return searchPaths;
        }

        String getBinaryNameWithSuffix (const MSVCBuildConfiguration& config, bool forceUnityPrefix) const
        {
            return config.getOutputFilename (getTargetSuffix(), true, forceUnityPrefix);
        }

        String getOutputFilePath (const MSVCBuildConfiguration& config, bool forceUnityPrefix) const
        {
            return getOwner().getOutDirFile (config, getBinaryNameWithSuffix (config, forceUnityPrefix));
        }

        StringArray getLibrarySearchPaths (const BuildConfiguration& config) const
        {
            auto librarySearchPaths = config.getLibrarySearchPaths();

            if (type != SharedCodeTarget)
                if (auto* shared = getOwner().getSharedCodeTarget())
                    librarySearchPaths.add (shared->getConfigTargetPath (config));

            return librarySearchPaths;
        }

        String getExternalLibraries (const MSVCBuildConfiguration& config, const String& otherLibs) const
        {
            StringArray libraries;

            if (otherLibs.isNotEmpty())
                libraries.add (otherLibs);

            auto moduleLibs = getOwner().getModuleLibs();
            if (! moduleLibs.isEmpty())
                libraries.addArray (moduleLibs);

            if (type != SharedCodeTarget)
                if (auto* shared = getOwner().getSharedCodeTarget())
                    libraries.add (shared->getBinaryNameWithSuffix (config, false));

            return libraries.joinIntoString (";");
        }

        String getDelayLoadedDLLs() const
        {
            auto delayLoadedDLLs = getOwner().msvcDelayLoadedDLLs;

            if (type == RTASPlugIn)
                delayLoadedDLLs += "DAE.dll; DigiExt.dll; DSI.dll; PluginLib.dll; "
                    "DSPManager.dll; DSPManager.dll; DSPManagerClientLib.dll; RTASClientLib.dll";

            return delayLoadedDLLs;
        }

        String getModuleDefinitions (const MSVCBuildConfiguration& config) const
        {
            auto moduleDefinitions = config.config [Ids::msvcModuleDefinitionFile].toString();

            if (moduleDefinitions.isNotEmpty())
                return moduleDefinitions;

            if (type == RTASPlugIn)
            {
                auto& exp = getOwner();

                auto moduleDefPath
                    = build_tools::RelativePath (exp.getPathForModuleString ("juce_audio_plugin_client"), build_tools::RelativePath::projectFolder)
                         .getChildFile ("juce_audio_plugin_client").getChildFile ("RTAS").getChildFile ("juce_RTAS_WinExports.def");

                return prependDot (moduleDefPath.rebased (exp.getProject().getProjectFolder(),
                                                          exp.getTargetFolder(),
                                                          build_tools::RelativePath::buildTargetFolder).toWindowsStyle());
            }

            return {};
        }

        File getVCProjFile() const                                   { return getOwner().getProjectFile (getProjectFileSuffix(), getName()); }
        File getVCProjFiltersFile() const                            { return getOwner().getProjectFile (getFiltersFileSuffix(), getName()); }

        String createRebasedPath (const build_tools::RelativePath& path) const { return getOwner().createRebasedPath (path); }

        void addWindowsTargetPlatformToConfig (XmlElement& e) const
        {
            auto target = owner.getWindowsTargetPlatformVersion();

            if (target == "Latest")
            {
                auto* child = e.createNewChildElement ("WindowsTargetPlatformVersion");

                child->setAttribute ("Condition", "'$(WindowsTargetPlatformVersion)' == ''");
                child->addTextElement ("$([Microsoft.Build.Utilities.ToolLocationHelper]::GetLatestSDKTargetPlatformVersion('Windows', '10.0'))");
            }
            else
            {
                e.createNewChildElement ("WindowsTargetPlatformVersion")->addTextElement (target);
            }
        }

    protected:
        const MSVCProjectExporterBase& owner;
        String projectGuid;
    };

    //==============================================================================
    bool usesMMFiles() const override                        { return false; }
    bool canCopeWithDuplicateFiles() override                { return false; }
    bool supportsUserDefinedConfigurations() const override  { return true; }

    bool isXcode() const override                            { return false; }
    bool isVisualStudio() const override                     { return true; }
    bool isCodeBlocks() const override                       { return false; }
    bool isMakefile() const override                         { return false; }
    bool isAndroidStudio() const override                    { return false; }
    bool isCLion() const override                            { return false; }

    bool isAndroid() const override                          { return false; }
    bool isWindows() const override                          { return true; }
    bool isLinux() const override                            { return false; }
    bool isOSX() const override                              { return false; }
    bool isiOS() const override                              { return false; }

    bool supportsPrecompiledHeaders() const override         { return true; }

    String getNewLineString() const override                 { return "\r\n"; }

    bool supportsTargetType (build_tools::ProjectType::Target::Type type) const override
    {
        switch (type)
        {
        case build_tools::ProjectType::Target::StandalonePlugIn:
        case build_tools::ProjectType::Target::GUIApp:
        case build_tools::ProjectType::Target::ConsoleApp:
        case build_tools::ProjectType::Target::StaticLibrary:
        case build_tools::ProjectType::Target::SharedCodeTarget:
        case build_tools::ProjectType::Target::AggregateTarget:
        case build_tools::ProjectType::Target::VSTPlugIn:
        case build_tools::ProjectType::Target::VST3PlugIn:
        case build_tools::ProjectType::Target::AAXPlugIn:
        case build_tools::ProjectType::Target::RTASPlugIn:
        case build_tools::ProjectType::Target::UnityPlugIn:
        case build_tools::ProjectType::Target::DynamicLibrary:
            return true;
        case build_tools::ProjectType::Target::AudioUnitPlugIn:
        case build_tools::ProjectType::Target::AudioUnitv3PlugIn:
        case build_tools::ProjectType::Target::unspecified:
        default:
            break;
        }

        return false;
    }

    //==============================================================================
    build_tools::RelativePath getManifestPath() const
    {
        auto path = manifestFileValue.get().toString();

        return path.isEmpty() ? build_tools::RelativePath()
                              : build_tools::RelativePath (path, build_tools::RelativePath::projectFolder);
    }

    //==============================================================================
    bool launchProject() override
    {
       #if JUCE_WINDOWS
        return getSLNFile().startAsProcess();
       #else
        return false;
       #endif
    }

    bool canLaunchProject() override
    {
       #if JUCE_WINDOWS
        return true;
       #else
        return false;
       #endif
    }

    void createExporterProperties (PropertyListBuilder& props) override
    {
        props.add (new TextPropertyComponent (manifestFileValue, "Manifest file", 8192, false),
            "Path to a manifest input file which should be linked into your binary (path is relative to jucer file).");

        props.add (new ChoicePropertyComponent (IPPLibraryValue, "Use IPP Library",
                                                { "No",  "Yes (Default Linking)",  "Multi-Threaded Static Library", "Single-Threaded Static Library", "Multi-Threaded DLL", "Single-Threaded DLL" },
                                                { var(), "true",                   "Parallel_Static",               "Sequential",                     "Parallel_Dynamic",   "Sequential_Dynamic" }),
                   "Enable this to use Intel's Integrated Performance Primitives library.");

        {
            auto isWindows10SDK = getVisualStudioVersion() > 14;

            props.add (new TextPropertyComponent (targetPlatformVersion, "Windows Target Platform", 20, false),
                       String ("Specifies the version of the Windows SDK that will be used when building this project. ")
                       + (isWindows10SDK ? "Leave this field empty to use the latest Windows 10 SDK installed on the build machine."
                                         : "The default value for this exporter is " + getDefaultWindowsTargetPlatformVersion()));
        }
    }

    enum OptimisationLevel
    {
        optimisationOff = 1,
        optimiseMinSize = 2,
        optimiseFull = 3,
        optimiseMaxSpeed = 4
    };

    //==============================================================================
    void addPlatformSpecificSettingsForProjectType (const build_tools::ProjectType& type) override
    {
        msvcExtraPreprocessorDefs.set ("_CRT_SECURE_NO_WARNINGS", "");

        if (type.isCommandLineApp())
            msvcExtraPreprocessorDefs.set("_CONSOLE", "");

        callForAllSupportedTargets ([this] (build_tools::ProjectType::Target::Type targetType)
                                    {
                                        if (targetType != build_tools::ProjectType::Target::AggregateTarget)
                                            targets.add (new MSVCTargetBase (targetType, *this));
                                    });

        // If you hit this assert, you tried to generate a project for an exporter
        // that does not support any of your targets!
        jassert (targets.size() > 0);
    }

    const MSVCTargetBase* getSharedCodeTarget() const
    {
        for (auto target : targets)
            if (target->type == build_tools::ProjectType::Target::SharedCodeTarget)
                return target;

        return nullptr;
    }

    bool hasTarget (build_tools::ProjectType::Target::Type type) const
    {
        for (auto target : targets)
            if (target->type == type)
                return true;

        return false;
    }

    static void createRCFile (const Project& p, const File& iconFile, const File& rcFile)
    {
        build_tools::ResourceRcOptions resourceRc;
        resourceRc.version = p.getVersionString();
        resourceRc.companyName = p.getCompanyNameString();
        resourceRc.companyCopyright = p.getCompanyCopyrightString();
        resourceRc.projectName = p.getProjectNameString();
        resourceRc.icon = iconFile;

        resourceRc.write (rcFile);
    }

private:
    //==============================================================================
    String createRebasedPath (const build_tools::RelativePath& path) const
    {
        auto rebasedPath = rebaseFromProjectFolderToBuildTarget (path).toWindowsStyle();

        return getVisualStudioVersion() < 10  // (VS10 automatically adds escape characters to the quotes for this definition)
                                          ? CppTokeniserFunctions::addEscapeChars (rebasedPath.quoted())
                                          : CppTokeniserFunctions::addEscapeChars (rebasedPath).quoted();
    }

protected:
    //==============================================================================
    mutable File rcFile, iconFile, packagesConfigFile;
    OwnedArray<MSVCTargetBase> targets;

    ValueWithDefault IPPLibraryValue, platformToolsetValue, targetPlatformVersion, manifestFileValue;

    File getProjectFile (const String& extension, const String& target) const
    {
        auto filename = project.getProjectFilenameRootString();

        if (target.isNotEmpty())
            filename += String ("_") + target.removeCharacters (" ");

        return getTargetFolder().getChildFile (filename).withFileExtension (extension);
    }

    File getSLNFile() const     { return getProjectFile (".sln", String()); }

    static String prependIfNotAbsolute (const String& file, const char* prefix)
    {
        if (File::isAbsolutePath (file) || file.startsWithChar ('$'))
            prefix = "";

        return prefix + build_tools::windowsStylePath (file);
    }

    String getIntDirFile (const BuildConfiguration& config, const String& file) const  { return prependIfNotAbsolute (replacePreprocessorTokens (config, file), "$(IntDir)\\"); }
    String getOutDirFile (const BuildConfiguration& config, const String& file) const  { return prependIfNotAbsolute (replacePreprocessorTokens (config, file), "$(OutDir)\\"); }

    BuildConfiguration::Ptr createBuildConfig (const ValueTree& v) const override
    {
        return *new MSVCBuildConfiguration (project, v, *this);
    }

    StringArray getHeaderSearchPaths (const BuildConfiguration& config) const
    {
        auto searchPaths = extraSearchPaths;
        searchPaths.addArray (config.getHeaderSearchPaths());
        return getCleanedStringArray (searchPaths);
    }

    String getSharedCodeGuid() const
    {
        String sharedCodeGuid;

        for (int i = 0; i < targets.size(); ++i)
            if (auto* target = targets[i])
                if (target->type == build_tools::ProjectType::Target::SharedCodeTarget)
                    return target->getProjectGuid();

        return {};
    }

    //==============================================================================
    void writeProjectDependencies (OutputStream& out) const
    {
        auto sharedCodeGuid = getSharedCodeGuid();

        for (int addingOtherTargets = 0; addingOtherTargets < (sharedCodeGuid.isNotEmpty() ? 2 : 1); ++addingOtherTargets)
        {
            for (int i = 0; i < targets.size(); ++i)
            {
                if (auto* target = targets[i])
                {
                    if (sharedCodeGuid.isEmpty() || (addingOtherTargets != 0) == (target->type != build_tools::ProjectType::Target::StandalonePlugIn))
                    {
                        out << "Project(\"{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}\") = \"" << projectName << " - "
                            << target->getName() << "\", \""
                            << target->getVCProjFile().getFileName() << "\", \"" << target->getProjectGuid() << '"' << newLine;

                        if (sharedCodeGuid.isNotEmpty() && target->type != build_tools::ProjectType::Target::SharedCodeTarget)
                            out << "\tProjectSection(ProjectDependencies) = postProject" << newLine
                                << "\t\t" << sharedCodeGuid << " = " << sharedCodeGuid << newLine
                                << "\tEndProjectSection" << newLine;

                        out << "EndProject" << newLine;
                    }
                }
            }
        }
    }

    void writeSolutionFile (OutputStream& out, const String& versionString, String commentString) const
    {
        if (commentString.isNotEmpty())
            commentString += newLine;

        out << "Microsoft Visual Studio Solution File, Format Version " << versionString << newLine
            << commentString << newLine;

        writeProjectDependencies (out);

        out << "Global" << newLine
            << "\tGlobalSection(SolutionConfigurationPlatforms) = preSolution" << newLine;

        for (ConstConfigIterator i (*this); i.next();)
        {
            auto& config = dynamic_cast<const MSVCBuildConfiguration&> (*i);
            auto configName = config.createMSVCConfigName();
            out << "\t\t" << configName << " = " << configName << newLine;
        }

        out << "\tEndGlobalSection" << newLine
            << "\tGlobalSection(ProjectConfigurationPlatforms) = postSolution" << newLine;

        for (auto& target : targets)
            for (ConstConfigIterator i (*this); i.next();)
            {
                auto& config = dynamic_cast<const MSVCBuildConfiguration&> (*i);
                auto configName = config.createMSVCConfigName();

                for (auto& suffix : { "ActiveCfg", "Build.0" })
                    out << "\t\t" << target->getProjectGuid() << "." << configName << "." << suffix << " = " << configName << newLine;
            }

        out << "\tEndGlobalSection" << newLine
            << "\tGlobalSection(SolutionProperties) = preSolution" << newLine
            << "\t\tHideSolutionNode = FALSE" << newLine
            << "\tEndGlobalSection" << newLine;

        out << "EndGlobal" << newLine;
    }

    //==============================================================================
    bool hasResourceFile() const
    {
        return ! projectType.isStaticLibrary();
    }

    void createResourcesAndIcon() const
    {
        if (hasResourceFile())
        {
            iconFile = getTargetFolder().getChildFile ("icon.ico");
            build_tools::writeWinIcon (getIcons(), iconFile);
            rcFile = getTargetFolder().getChildFile ("resources.rc");
            createRCFile (project, iconFile, rcFile);
        }
    }

    bool shouldAddWebView2Package() const
    {
        return project.getEnabledModules().isModuleEnabled ("juce_gui_extra")
              && project.isConfigFlagEnabled ("JUCE_USE_WIN_WEBVIEW2", false);
    }

    static String getWebView2PackageName()     { return "Microsoft.Web.WebView2"; }
    static String getWebView2PackageVersion()  { return "0.9.488"; }

    void createPackagesConfigFile() const
    {
        if (shouldAddWebView2Package())
        {
            packagesConfigFile = getTargetFolder().getChildFile ("packages.config");

            build_tools::writeStreamToFile (packagesConfigFile, [] (MemoryOutputStream& mo)
            {
                mo.setNewLineString ("\r\n");

                mo << "<?xml version=\"1.0\" encoding=\"utf-8\"?>"                   << newLine
                   << "<packages>"                                                   << newLine
                   << "\t" << "<package id=" << getWebView2PackageName().quoted()
                           << " version="    << getWebView2PackageVersion().quoted()
                           << " />"                                                  << newLine
                   << "</packages>"                                                  << newLine;
            });
        }
    }

    static String prependDot (const String& filename)
    {
        return build_tools::isAbsolutePath (filename) ? filename
                                                      : (".\\" + filename);
    }

    static bool shouldUseStdCall (const build_tools::RelativePath& path)
    {
        return path.getFileNameWithoutExtension().startsWithIgnoreCase ("include_juce_audio_plugin_client_RTAS_");
    }

    StringArray getModuleLibs() const
    {
        StringArray result;

        for (auto& lib : windowsLibs)
            result.add (lib + ".lib");

        return result;
    }

    JUCE_DECLARE_NON_COPYABLE (MSVCProjectExporterBase)
};

//==============================================================================
class MSVCProjectExporterVC2015  : public MSVCProjectExporterBase
{
public:
    MSVCProjectExporterVC2015 (Project& p, const ValueTree& t)
        : MSVCProjectExporterBase (p, t, getTargetFolderName())
    {
        name = getDisplayName();

        targetPlatformVersion.setDefault (getDefaultWindowsTargetPlatformVersion());
        platformToolsetValue.setDefault (getDefaultToolset());
    }

    static String getDisplayName()        { return "Visual Studio 2015"; }
    static String getValueTreeTypeName()  { return "VS2015"; }
    static String getTargetFolderName()   { return "VisualStudio2015"; }

    int getVisualStudioVersion() const override                      { return 14; }
    String getSolutionComment() const override                       { return "# Visual Studio 2015"; }
    String getToolsVersion() const override                          { return "14.0"; }
    String getDefaultToolset() const override                        { return "v140"; }
    String getDefaultWindowsTargetPlatformVersion() const override   { return "8.1"; }

    static MSVCProjectExporterVC2015* createForSettings (Project& projectToUse, const ValueTree& settingsToUse)
    {
        if (settingsToUse.hasType (getValueTreeTypeName()))
            return new MSVCProjectExporterVC2015 (projectToUse, settingsToUse);

        return nullptr;
    }

    void createExporterProperties (PropertyListBuilder& props) override
    {
        static const char* toolsetNames[] = { "v140", "v140_xp", "CTP_Nov2013" };
        const var toolsets[]              = { "v140", "v140_xp", "CTP_Nov2013" };
        addToolsetProperty (props, toolsetNames, toolsets, numElementsInArray (toolsets));

        MSVCProjectExporterBase::createExporterProperties (props);
    }

    JUCE_DECLARE_NON_COPYABLE (MSVCProjectExporterVC2015)
};

//==============================================================================
class MSVCProjectExporterVC2017  : public MSVCProjectExporterBase
{
public:
    MSVCProjectExporterVC2017 (Project& p, const ValueTree& t)
        : MSVCProjectExporterBase (p, t, getTargetFolderName())
    {
        name = getDisplayName();

        targetPlatformVersion.setDefault (getDefaultWindowsTargetPlatformVersion());
        platformToolsetValue.setDefault (getDefaultToolset());
    }

    static String getDisplayName()        { return "Visual Studio 2017"; }
    static String getValueTreeTypeName()  { return "VS2017"; }
    static String getTargetFolderName()   { return "VisualStudio2017"; }

    int getVisualStudioVersion() const override                      { return 15; }
    String getSolutionComment() const override                       { return "# Visual Studio 2017"; }
    String getToolsVersion() const override                          { return "15.0"; }
    String getDefaultToolset() const override                        { return "v141"; }
    String getDefaultWindowsTargetPlatformVersion() const override   { return "Latest"; }

    static MSVCProjectExporterVC2017* createForSettings (Project& projectToUse, const ValueTree& settingsToUse)
    {
        if (settingsToUse.hasType (getValueTreeTypeName()))
            return new MSVCProjectExporterVC2017 (projectToUse, settingsToUse);

        return nullptr;
    }

    void createExporterProperties (PropertyListBuilder& props) override
    {
        static const char* toolsetNames[] = { "v140", "v140_xp", "v141", "v141_xp" };
        const var toolsets[]              = { "v140", "v140_xp", "v141", "v141_xp" };
        addToolsetProperty (props, toolsetNames, toolsets, numElementsInArray (toolsets));

        MSVCProjectExporterBase::createExporterProperties (props);
    }

    JUCE_DECLARE_NON_COPYABLE (MSVCProjectExporterVC2017)
};

//==============================================================================
class MSVCProjectExporterVC2019  : public MSVCProjectExporterBase
{
public:
    MSVCProjectExporterVC2019 (Project& p, const ValueTree& t)
        : MSVCProjectExporterBase (p, t, getTargetFolderName())
    {
        name = getDisplayName();

        targetPlatformVersion.setDefault (getDefaultWindowsTargetPlatformVersion());
        platformToolsetValue.setDefault (getDefaultToolset());
    }

    static String getDisplayName()        { return "Visual Studio 2019"; }
    static String getValueTreeTypeName()  { return "VS2019"; }
    static String getTargetFolderName()   { return "VisualStudio2019"; }

    int getVisualStudioVersion() const override                      { return 16; }
    String getSolutionComment() const override                       { return "# Visual Studio 2019"; }
    String getToolsVersion() const override                          { return "16.0"; }
    String getDefaultToolset() const override                        { return "v142"; }
    String getDefaultWindowsTargetPlatformVersion() const override   { return "10.0"; }

    static MSVCProjectExporterVC2019* createForSettings (Project& projectToUse, const ValueTree& settingsToUse)
    {
        if (settingsToUse.hasType (getValueTreeTypeName()))
            return new MSVCProjectExporterVC2019 (projectToUse, settingsToUse);

        return nullptr;
    }

    void createExporterProperties (PropertyListBuilder& props) override
    {
        static const char* toolsetNames[] = { "v140", "v140_xp", "v141", "v141_xp", "v142" };
        const var toolsets[]              = { "v140", "v140_xp", "v141", "v141_xp", "v142" };
        addToolsetProperty (props, toolsetNames, toolsets, numElementsInArray (toolsets));

        MSVCProjectExporterBase::createExporterProperties (props);
    }

    JUCE_DECLARE_NON_COPYABLE (MSVCProjectExporterVC2019)
};
