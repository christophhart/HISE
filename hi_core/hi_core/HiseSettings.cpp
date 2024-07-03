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



namespace hise {
using namespace juce;


Array<juce::Identifier> HiseSettings::SettingFiles::getAllIds()
{
	Array<Identifier> ids;

#if USE_BACKEND 

#if IS_MARKDOWN_EDITOR
	ids.add(DocSettings);
#else
	ids.add(ProjectSettings);
	ids.add(UserSettings);
	ids.add(CompilerSettings);
	ids.add(GeneralSettings);
	ids.add(ExpansionSettings);
	ids.add(ScriptingSettings);
	ids.add(DocSettings);
	ids.add(SnexWorkbenchSettings);
#endif

#endif

	ids.add(OtherSettings);

	ids.add(AudioSettings);
	ids.add(MidiSettings);

	return ids;
}

Array<juce::Identifier> HiseSettings::Project::getAllIds()
{
	Array<Identifier> ids;

	ids.add(Name);
	ids.add(Version);
	ids.add(Description);
	ids.add(BundleIdentifier);
	ids.add(PluginCode);
	ids.add(EmbedAudioFiles);
	ids.add(EmbedImageFiles);
	ids.add(SupportFullDynamicsHLAC);
	ids.add(AdditionalDspLibraries);
	ids.add(OSXStaticLibs);
	ids.add(WindowsStaticLibFolder);
	ids.add(ExtraDefinitionsWindows);
	ids.add(ExtraDefinitionsOSX);
	ids.add(ExtraDefinitionsIOS);
    ids.add(ExtraDefinitionsLinux);
	ids.add(ExtraDefinitionsNetworkDll);
	ids.add(AppGroupID);
	ids.add(RedirectSampleFolder);
	ids.add(AAXCategoryFX);
    ids.add(VST3Category);
	ids.add(SupportMonoFX);
	ids.add(EnableMidiInputFX);
    ids.add(EnableMidiOut);
	ids.add(EnableSoundGeneratorsFX);
	ids.add(VST3Support);
	ids.add(UseRawFrontend);
	ids.add(ExpansionType);
	ids.add(EncryptionKey);
	ids.add(LinkExpansionsToProject);
	ids.add(ReadOnlyFactoryPresets);
    ids.add(ForceStereoOutput);
	ids.add(AdminPermissions);
	ids.add(EmbedUserPresets);
	ids.add(OverwriteOldUserPresets);
	ids.add(EnableGlobalPreprocessor);
    ids.add(UseGlobalAppDataFolderWindows);
    ids.add(UseGlobalAppDataFolderMacOS);
	ids.add(DefaultUserPreset);
	ids.add(CompileWithPerfetto);
	ids.add(CompileWithDebugSymbols);
	ids.add(IncludeLorisInFrontend);

	return ids;
}

Array<juce::Identifier> HiseSettings::Compiler::getAllIds()
{
	Array<Identifier> ids;

	ids.add(HisePath);
	ids.add(VisualStudioVersion);
	ids.add(UseIPP);
	ids.add(LegacyCPUSupport);
	ids.add(RebuildPoolFiles);
	ids.add(Support32BitMacOS);
	ids.add(CustomNodePath);
	ids.add(FaustPath);
    ids.add(FaustExternalEditor);
    ids.add(EnableLoris);

	return ids;
}

Array<juce::Identifier> HiseSettings::User::getAllIds()
{
	Array<Identifier> ids;

	ids.add(Company);
	ids.add(CompanyCode);
	ids.add(CompanyURL);
	ids.add(CompanyCopyright);
	ids.add(TeamDevelopmentID);

	return ids;
}

Array<juce::Identifier> HiseSettings::ExpansionSettings::getAllIds()
{
	Array<Identifier> ids;

	ids.add(UUID);
	ids.add(Tags);
	ids.add(Description);

	return ids;
}

Array<juce::Identifier> HiseSettings::Scripting::getAllIds()
{
	Array<Identifier> ids;

	ids.add(EnableCallstack);
	ids.add(EnableOptimizations);
	ids.add(GlobalScriptPath);
	ids.add(CompileTimeout);
	ids.add(CodeFontSize);
	ids.add(EnableDebugMode);
	ids.add(SaveConnectedFilesOnCompile);
	ids.add(EnableMousePositioning);
    ids.add(WarnIfUndefinedParameters);

	return ids;
}

Array<juce::Identifier> HiseSettings::Other::getAllIds()
{
	Array<Identifier> ids;

	ids.add(UseOpenGL);
	ids.add(GlobalSamplePath);
	ids.add(EnableAutosave);
	ids.add(AutosaveInterval);
	ids.add(AudioThreadGuardEnabled);
	ids.add(ExternalEditorPath);
    ids.add(AutoShowWorkspace);
	ids.add(EnableShaderLineNumbers);

	return ids;
}

juce::Array<juce::Identifier> HiseSettings::Documentation::getAllIds()
{
	Array<Identifier> ids;

	ids.add(DocRepository);
	ids.add(RefreshOnStartup);

	return ids;
}

Array<juce::Identifier> HiseSettings::Midi::getAllIds()
{
	Array<Identifier> ids;

	ids.add(MidiInput);
	ids.add(MidiChannels);

	return ids;
}

Array<juce::Identifier> HiseSettings::Audio::getAllIds()
{
	Array<Identifier> ids;

	ids.add(Driver);
	ids.add(Device);
	ids.add(Output);
	ids.add(Samplerate);
	ids.add(BufferSize);

	return ids;
}

Array<juce::Identifier> HiseSettings::SnexWorkbench::getAllIds()
{
	Array<Identifier> ids;

	ids.add(PlayOnRecompile);
	ids.add(AddFade);
	
	return ids;
}


#define P(p) if (prop == p) { s << "### " << ConversionHelpers::getUncamelcasedId(p) << "\n";
#define D(x) s << x << "\n";
#define P_() return s; } 

	String HiseSettings::SettingDescription::getDescription(const Identifier& prop)
	{
		String s;

		P(HiseSettings::Project::Name);
		D("The name of the project. This will be also the name of the plugin binaries");
		P_();

		P(HiseSettings::Project::Version);
		D("The version number of the project. Try using semantic versioning (`1.0.0`) for this.  ");
		D("The version number will be used to handle the user preset backward compatibility.");
		D("> Be aware that some hosts (eg. Logic) are very picky when they detect different plugin binaries with the same version.");
		P_();

		P(HiseSettings::Project::BundleIdentifier);
		D("This is a unique identifier used by Apple OS to identify the app. It must be formatted as reverse domain like this:");
		D("> `com.your-company.product`");
		P_();

		P(HiseSettings::Project::PluginCode);
		D("The code used to identify the plugins. This has to be four characters with the first one being uppercase like this:");
		D("> `Abcd`");
		P_();

		P(HiseSettings::Project::EmbedAudioFiles);
		D("If this is **enabled**, it will embed all audio files (impulse responses & loops) **as well as images** into the plugin.");
		D("This will not affect samples - they will always be streamed.  ");
		D("If it's **disabled**, it will use the resource files found in the app data directory and you need to make sure that your installer");
		D("copies them to the right location:");
		D("> **Windows:** `%APPDATA%\\Company\\Product\\`");
		D("> **macOS:** `~/Library/Application Support/Company/Product/`");
		D("Normally you would try to embed them into the binary, however if you have a lot of audio files (> 50MB)");
		D("the compiler will crash with an **out of heap space** error, so in this case you're better off not embedding them.");
		P_();

		P(HiseSettings::Project::CompileWithPerfetto);
		D("If enabled, the project will be compiled with the Perfetto Tracing SDK.");
		D("> This allows you to profile & track down issues and performance hotspots, during development or troubleshooting.");
		P_();

		P(HiseSettings::Project::CompileWithDebugSymbols);
		D("If enabled, the project will be compiled with the debug symbols for better trouble shooting.");
		D("> With this setting, the crash reports will contain valid source code locations which might be helpful for debugging crashes, but you obviously have to turn this off for a production release!.");
		P_();

		P(HiseSettings::Project::EmbedImageFiles);
		D("If this is **enabled**, it will embed all audio files (impulse responses & loops) into the plugin.");
		D("If it's **disabled**, it will use the resource files found in the app data directory and you need to make sure that your installer");
		D("copies them to the right location:");
		D("> **Windows:** `%APPDATA%\\Company\\Product\\`");
		D("> **macOS:** `~/Library/Application Support/Company/Product/`");
		D("Normally you would try to embed them into the binary, however if you have a lot of images (> 50MB)");
		D("the compiler will crash with an **out of heap space** error, so in this case you're better off not embedding them.");
		P_();

		P(HiseSettings::Project::SupportFullDynamicsHLAC);
		D("If enabled, the user can extract the sample monolith files to support the full dynamic range of 24 bit.");
		D("The HLAC codec is 16bit only, but with this feature enabled, it normalises the audio data in chunks of 1024 samples in order to recreate higher bit depths. This results in a lower compression ratio, but removes the quantisation noise that can occur under certain circumstances:");
		D("For normal sample libraries without heavy dynamics processing this feature is not required, but for projects that heavily process the dynamic range (eg. drum libraries that squash the samples with a compressor) the quantisation noise floor of -96dB might get attenuated into the audible range. So: If you start to hear quantisation noise, enable this, otherwise enjoy the low disk usage and performance of 16bit samples.");
		D("> The end user can still choose whether he wants to use the samples in the full dynamics range. However in order to make this work, create the sample archive with the **Support Full Dynamics** option set to true.");
		P_();

		P(HiseSettings::Project::AdditionalDspLibraries);
		D("If you have written custom DSP objects that you want to embed statically, you have to supply the class names of each DspModule class here");
		P_();

		P(HiseSettings::Project::WindowsStaticLibFolder);
		D("If you need to link a static library on Windows, supply the absolute path to the folder here. Unfortunately, relative paths do not work well with the VS Linker");
		P_();

		P(HiseSettings::Project::OSXStaticLibs);
		D("If you need to link a static library on macOS, supply the path to the .a library file here.");
        D("> This can also be used to set additional linker flags for additional frameworks (eg. iLok).");
		P_();

		P(HiseSettings::Project::ExtraDefinitionsWindows);
		D("This field can be used to add preprocessor definitions for Windows builds. Use it to tailor the compile options for HISE for the project.");
		D("#### Examples");
		D("```javascript");
		D("ENABLE_ALL_PEAK_METERS=0");
		D("NUM_POLYPHONIC_VOICES=100");
		D("```\n");
		P_();

        P(HiseSettings::Project::ExtraDefinitionsLinux);
        D("This field can be used to add preprocessor definitions  for Linux builds. Use it to tailor the compile options for HISE for the project.");
        D("#### Examples");
        D("```javascript");
        D("ENABLE_ALL_PEAK_METERS=0");
        D("NUM_POLYPHONIC_VOICES=100");
        D("```\n");
        P_();
        
		P(HiseSettings::Project::ExtraDefinitionsOSX);
		D("This field can be used to add preprocessor definitions for macOS builds. Use it to tailor the compile options for HISE for the project.");
		D("#### Examples");
		D("```javascript");
		D("ENABLE_ALL_PEAK_METERS=0");
		D("NUM_POLYPHONIC_VOICES=100");
		D("```\n");
		P_();

		P(HiseSettings::Project::ExtraDefinitionsIOS);
		D("This field can be used to add preprocessor definitions for iOS builds. Use it to tailor the compile options for HISE for the project.");
		D("#### Examples");
		D("```javascript");
		D("ENABLE_ALL_PEAK_METERS=0");
		D("NUM_POLYPHONIC_VOICES=100");
		D("```\n");
		P_();

		P(HiseSettings::Project::ExtraDefinitionsIOS);
		D("This field can be used to add preprocessor definitions to the compiled network DLL. Use it to tailor the compile options for HISE for the project.");
		D("#### Examples");
		D("```javascript");
		D("ENABLE_ALL_PEAK_METERS=0");
		D("NUM_POLYPHONIC_VOICES=100");
		D("```\n");
		D("> Be aware that these fields are only added to the compiled network DLL. If you want to add them to the exported project, add every property to the other extra definitions.");
		P_();

		P(HiseSettings::Project::EmbedUserPresets);
		D("If disabled, the user presets will not be part of the binary and are not extracted automatically on first plugin launch");
		D("> This is useful if you're running your own preset management or the user preset collection gets too big to be embedded in the plugin");
		P_();

		P(HiseSettings::Project::OverwriteOldUserPresets);
		D("If true, then the plugin will silently overwrite user presets with the same name but an older version number.  ");
		D("This will also overwrite user-modified factory presets but will not modify or delete user-created user presets (with the exception of a name collision).");
		P_();

		P(HiseSettings::Project::AppGroupID);
		D("If you're compiling an iOS app, you need to add an App Group to your Apple ID for this project and supply the name here.");
		D("App Group IDs must have reverse-domain format and start with group, like:");
		D("> `group.company.product`");
		P_();

		P(HiseSettings::Project::ExpansionType);
		D("Sets the expansion type you want to use for this project. You can choose between:  \n");
		D("- Disabled / no expansions (default)");
		D("- Unencrypted file-based expansions (just creates a mini-project-folder inside the expansion)");
		D("- Script-encrypted expansions");
		D("- Full expansions that contain the entire instrument");
		D("- Custom expansions that uses a custom C++ class");
		D("> If you use a custom expansion, you will need to implement `ExpansionHandler::createCustomExpansion()` in your project's C++ code");
		P_();

		P(HiseSettings::Project::EncryptionKey);
		D("Sets the BlowFish encryption key (up to 72 characters) that will be used to encrypt the intermediate expansions.");
		D("> If you're using the **Full** expansion type you will need to set the key here, otherwise, you can call `ExpansionHandler.setEncryptionKey()` for the same effect.");
		D("Make sure you restart HISE after changing this setting in order to apply the change.");
		P_();

		P(HiseSettings::Project::LinkExpansionsToProject);
		D("Redirects the expansion folder in the app data directory of the compiled plugin back to your project folder's expansion folder.  ");
		D("A compiled plugin will look in its app data folder for the expansions, so during development, you might want it to link back to the subfolder of the project directory so that you can see the expansions that you have created.  ");
		D("If you do not enable this, the expansion list in a compiled project will be empty unless you manually copy the Expansion folder from the project folder to the app data folder.");
		D("> Be aware that this will only work on the development machine and has nothing to do with proper distribution of the expansions to the end user");
		D("Be aware that this is a system-specific setting so if you load a project from another machine, make sure to tick / untick this box in order to create the expansion folder on this machine");
		P_();

		P(HiseSettings::Project::IncludeLorisInFrontend);
		D("If enabled, this will include the Loris library in the compiled plugin.");
		D("> Be aware that the Loris library is licensed under the GPL license, so you must not enable this flag in a proprietary project!");
		P_();

		P(HiseSettings::Project::RedirectSampleFolder);
		D("You can use another location for your sample files. This is useful if you have limited space on your hard drive and need to separate the samples.");
		D("> HISE will create a file called `LinkWindows` / `LinkOSX` in the samples folder that contains the link to the real folder.");
		P_();

		P(HiseSettings::Project::EnableGlobalPreprocessor);
		D("If this flag is enabled, it will use the C-Preprocessor to process all HiseScript files on compilation");
		D("If you disable this method, you can still enable preprocessing for individual files if they start with the directive `#on`");
		D("> This setting will not have an effect on compiled plugins as the preprocessor will already be evaluated on export");
		P_();

        P(HiseSettings::Project::VST3Category);
        D("The category the VST3 plugin will appear in");
        P_();
        
		P(HiseSettings::Project::AAXCategoryFX);
		D("If you export an effect plugin, you can specify the category it will show up in ProTools here");
		
		D("| ID | Description |");
		D("| ------ | ---- |");
		D("| AAX_ePlugInCategory_EQ | Equalization |");
		D("| AAX_ePlugInCategory_Dynamics | Compressor, expander, limiter, etc. |");
		D("| AAX_ePlugInCategory_PitchShift | Pitch processing |");
		D("| AAX_ePlugInCategory_Reverb | Reverberation and room/space simulation |");
		D("| AAX_ePlugInCategory_Delay | Delay and echo |");
		D("| AAX_ePlugInCategory_Modulation | Phasing, flanging, chorus, etc. |");
		D("| AAX_ePlugInCategory_Harmonic | Distortion, saturation, and harmonic enhancement |");
		D("| AAX_ePlugInCategory_NoiseReduction | Noise reduction |");
		D("| AAX_ePlugInCategory_Dither | Dither, noise shaping, etc. |");
		D("| AAX_ePlugInCategory_SoundField | Pan, auto-pan, upmix and downmix, and surround handling |");
		D("| AAX_EPlugInCategory_Effect | Special effects |");
		D("> This setting will have no effect for virtual instruments.");
		P_();

		P(HiseSettings::Project::SupportMonoFX);
		D("If enabled, the effect plugin will also be compatible to mono channel tracks.");
		D("> This setting will have no effect for virtual instruments.");
		P_();

		P(HiseSettings::Project::EnableMidiInputFX);
		D("If enabled, the effect plugin will process incoming MIDI messages (if the host supports FX plugins with MIDI input");
		P_();
        
        P(HiseSettings::Project::EnableMidiOut);
        D("If enabled, the instrument plugin can send out MIDI messages that you forward using `Message.sendToMidiOut()`");
        P_();
        
		P(HiseSettings::Project::EnableSoundGeneratorsFX);
		D("If enabled, the effect plugin will also process child sound generators (eg. Global modulation containers and Macro modulation sources");
		D("> Be aware that the sound output of the child sound generators will not be used, so this is only useful with \"quiet\" sound generators");
		P_();


		P(HiseSettings::Project::ReadOnlyFactoryPresets);
		D("If enabled, the user presets that are part of the compiled plugin (and expansions) can not be overriden by the end user.");
		D("> It will grey out the save button for all factory presets");
		P_();

        P(HiseSettings::Scripting::WarnIfUndefinedParameters);
        D("If enabled, it will print a warning with a callstack if you try to call a function  on a dynamic object reference with an undefined function.");
        D("> This only works if you haven't set `HISE_WARN_UNDEFINED_PARAMETER_CALLS` to 0, then it will just abort execution and throw an error");
        P_();
        
		P(HiseSettings::Project::VST3Support);
		D("If enabled, the exported plugins will use the VST3 SDK standard instead of the VST 2.x SDK. Until further notice, this is a experimental feature so proceed with caution.");
		D("> Be aware that Steinberg stopped support for the VST 2.4 SDK in October 2018 so if you don't have a valid VST2 license agreement in place, you must use the VST3 SDK.");
		P_();
				
		P(HiseSettings::Project::UseRawFrontend);
		D("If enabled, the project will not use the preset structure and scripted user interface and lets you use HISE as C++ framework.");
		D("You will have to implement a custom C++ class in the `AdditionalSourceCode` subfolder.");
		P_();

        P(HiseSettings::Project::ForceStereoOutput);
        D("If you export a plugin with HISE it will create as much channels as the routing matrix of the master container requires.");
        D(" If you don't want this behaviour, you can enable this option to force the plugin to just use a stereo channel configuration");
        P_();
    
				P(HiseSettings::Project::AdminPermissions);
				D("If enabled, standalone builds on Windows will prompt the user for administrator privileges.");
				D(" This is neccessary for tasks that access restricted locations such as the user's VST3 directory.");
				P_();
		    
        P(HiseSettings::Project::UseGlobalAppDataFolderWindows);
        D("If enabled, this will use the global app data folder (C:/ProgramData/Common Files) for the app data");
        D("> This setting will write the `HISE_USE_SYSTEM_APP_DATA_FOLDER` flag when exporting the plugin");
        P_();
        
        P(HiseSettings::Project::UseGlobalAppDataFolderMacOS);
        D("If enabled, this will use the global app data folder on macOS (/Library/Application Support)");
        D("> This setting will write the `HISE_USE_SYSTEM_APP_DATA_FOLDER` flag when exporting the plugin");
        P_();
        
		P(HiseSettings::Project::DefaultUserPreset);
		D("The relative path to the user preset that is supposed to be the initialisation state. If non-empty, this will be used ");
		D("in order to initialise the plugin as well as set the default states and select it in the preset browser");
		P_();

		P(HiseSettings::User::Company);
		D("Your company name. This will be used for the path to the app data directory so make sure you don't use weird characters here");
		P_();

		P(HiseSettings::User::CompanyCode);
		D("The unique code to identify your company. This must be 4 characters with the first one being uppercase like this:");
		D("> `Abcd`");
		P_();

		P(HiseSettings::User::TeamDevelopmentID);
		D("If you have a Apple Developer Account, enter the Developer ID here in order to code sign your app / plugin after compilation");
		P_();

		P(HiseSettings::Compiler::VisualStudioVersion);
		D("Set the VS version that you've installed. Make sure you always use the latest one, since I need to regularly deprecate the oldest version");
		P_();

		P(HiseSettings::Compiler::HisePath);
		D("This is the path to the source code of HISE. It must be the root folder of the repository (so that the folders `hi_core`, `hi_modules` etc. are immediate child folders.  ");
		D("This will be used for the compilation of the exported plugins and also contains all necessary SDKs (ASIO, VST, etc).");
		D("> Always make sure you are using the **exact** same source code that was used to build HISE or there will be unpredicatble issues.");
		P_();

		P(HiseSettings::Compiler::CustomNodePath);
		D("This is the path to the directory where the additional nodes are stored. If you want to use this feature, recompile HISE with the HI_ENABLE_CUSTOM_NODES flag.");
		P_();

		P(HiseSettings::Compiler::UseIPP);
		D("If enabled, HISE uses the FFT routines from the Intel Performance Primitive library (which can be downloaded for free) in order ");
		D("to speed up the convolution reverb");
		D("> If you use the convolution reverb in your project, this is almost mandatory, but there are a few other places that benefit from having this library");
		P_();

		P(HiseSettings::Compiler::FaustPath);
		D("Set the path to your Faust installation here. ");
		D("It will be used to look up the standard faust libraries on platforms which don't have a default path. ");
		D("There should be at least the following directories inside: \"share\", \"lib\", \"include\"");
		P_();

		P(HiseSettings::Compiler::ExportSetup);
		D("If this is ticked the system is ready for export.  ");
		D("Starting with HISE 4.0.1 this will be deactivated by default until the export setup wizard has been executed once.");
		D("> Nobody prevents you from ticking the box here in order to bypass the export wizard...");
		P_();

        P(HiseSettings::Compiler::FaustExternalEditor);
        D("If enabled, the edit button in the faust node will launch an external editor for ");
        D("editing the faust source files. If disabled, it will use a FaustCodeEditor floating tile");
        P_();
        
        P(HiseSettings::Compiler::EnableLoris);
        D("If you want to use the Loris toolkit in HISE, you need to enable this setting and download and copy the Loris DLL to the expected location");
        D("> The repository can be found here: `https://github.com/christophhart/loris-tools/`");
        P_();
        
        P(HiseSettings::Compiler::LegacyCPUSupport);
		D("If enabled, then all SSE instructions are replaced by their native implementation. This can be used to compile a version that runs on legacy CPU models."); 
		P_();

        
        
		P(HiseSettings::Compiler::RebuildPoolFiles);
		D("If enabled, the pool files for SampleMaps, AudioFiles and Images are deleted and rebuild everytime you export a plugin.");
		D("You can turn this off in order to speed up compilation times, however be aware that in this case you need to delete them manually");
		D("whenever you change the referenced data in any way or it will use the deprecated cached files.");
		P_();

		P(HiseSettings::Scripting::CodeFontSize);
		D("Changes the default font size for the console, all code editors and the script watch table");
		D("> You can temporarily change the font size for individual elements using Cmd+Scrollwheel, however this will not be persistent.");
		P_();

		P(HiseSettings::Scripting::EnableOptimizations);
		D("Enables some compiler optimizations like constant folding or dead code removal for the HiseScript compiler");
		D("> This setting is baked into a plugin when you compile it");
		P_();

		P(HiseSettings::Scripting::EnableMousePositioning);
		D("Sets the default value of whether the interface designer should allow dragging UI components with the mouse");
		D("> This was always enabled, but on larger projects it's easy to accidentally drag UI elements when you really just wanted to select them so this gives you the option to remove the dragging.");
		D("Note that you can always choose to enable / disable dragging in the interface designer menu bar, and this only sets the default value. It's still enabled by default so the HISE forum doesn't get swamped with bug reports that the interface designer stopped working...");
		P_();

		P(HiseSettings::Compiler::Support32BitMacOS);
		D("If enabled (which is still the default), the compiler will build both 32bit and 64bit versions as universal binary on macOS. However since 32bit binaries are deprecated in the most recent versions of macOS / XCode, you can tell the exporter to just generate 64bit binaries by disabling this flag. If you see this error messag in the compile terminal:");
		D("> error: The i386 architecture is deprecated. You should update your ARCHS build setting to remove the i386 architecture.");
		D("Just disable this flag and try again.");
		P_();

		P(HiseSettings::Scripting::EnableCallstack);
		D("This enables a stacktrace that shows the order of function calls that lead to the error (or breakpoint).");
		D("#### Example: ")
			D("```javascript");
		D("Interface: Breakpoint 1 was hit ");
		D(":  someFunction() - Line 5, column 18");
		D(":  onNoteOn() - Line 3, column 2");
		D("```\n");
		D("A breakpoint was set on the function `someFunction` You can see in the stacktrace that it was called in the `onNoteOn` callback.  ");
		D("Double clicking on the line in the console jumps to each location.");
		P_();

		P(HiseSettings::Scripting::CompileTimeout);
		D("Sets the timeout for the compilation of a script in **seconds**. Whenever the compilation takes longer, it will abort and show a error message.");
		D("This prevents hanging if you accidentally create endless loops like this:\n");
		D("```javascript");
		D("while(true)");
		D(" x++;");
		D("");
		D("```\n");
		P_();

		P(HiseSettings::Scripting::SaveConnectedFilesOnCompile);
		D("If this is enabled, it will save a connected script file everytime the script is compiled. By default this is disabled, but if you want to apply changes to a connected script file, you will have to enable this setting");
		P_();

		P(HiseSettings::ExpansionSettings::UUID);
		D("A unique Identifier that will be used when this project is exported as full instrument expansion");
		P_();

		P(HiseSettings::ExpansionSettings::Tags);
		D("A comma-separated list of strings that will be used as tags for full instrument expansions");
		P_();

		P(HiseSettings::ExpansionSettings::Description);
		D("A markdown formatted text that will be written into the metadata of the full instrument expansion");
		P_();

		P(HiseSettings::Scripting::GlobalScriptPath);
		D("There is a folder that can be used to store global script files like additional API functions or generic UI Component definitions.");
		D("By default, this folder is stored in the application data folder, but you can choose to redirect it to another location, which may be useful if you want to put it under source control.");
		D("You can include scripts that are stored in this location by using the `{GLOBAL_SCRIPT_FOLDER}` wildcard:");
		D("```javascript");
		D("// Includes 'File.js'");
		D("include(\"{GLOBAL_SCRIPT_FOLDER}File.js\");");
		D("```\n");
		P_();

		P(HiseSettings::Scripting::EnableDebugMode);
		D("This enables the debug logger which creates a log file containing performance issues and system specifications.");
		D("It's the same functionality as found in the compiled plugins.");
		P_();

		P(HiseSettings::Other::UseOpenGL);
		D("Enable this in order to use OpenGL for the UI rendering of the HISE app. This might drastically accelerate the UI performance, so if you have a laggy UI in HISE, try this option");
		D("> Be aware that this does not affect whether your compiled project uses OpenGL (as this can be defined separately).");
		P_();

		P(HiseSettings::Other::GlobalSamplePath);
		D("If you want to redirect all sample locations to a global sample path (eg. on a dedicated hard drive or the Dropbox folder), you can set it here.")
		D("Then you can just put a redirection file using the `{GLOBAL_SAMPLE_FOLDER}` wildcard into each sample folder that you want to redirect");
		P_();

		P(HiseSettings::Other::ExternalEditorPath);
		D("You can specifiy the executable of an audio editor here and then use the button in the sample editor to open the currently selected files in the editor");
		D("> You can use any editor that accepts filenames as command-line argument");
		P_();

		P(HiseSettings::Other::EnableAutosave);
		D("The autosave function will store up to 5 archive files called `AutosaveXXX.hip` in the archive folder of the project.");
		D("In a rare and almost never occuring event of a crash, this might be your saviour...");
		P_();

		P(HiseSettings::Other::AutosaveInterval);
		D("The interval for the autosaver in minutes. This must be a number between `1` and `30`.");
		P_();

        P(HiseSettings::Other::AutoShowWorkspace);
        D("If this is activated, clicking on a workspace icon (or loading a new patch) will ensure that the workspace is visible (so if it's folded, it will be unfolded.");
        D("> Disable this setting if you are using a custom workspace environment with a second window.");
        P_();
        
		P(HiseSettings::Other::AudioThreadGuardEnabled);
		D("Watches for illegal calls in the audio thread. Use this during script development to catch allocations etc.");
		P_();

		P(HiseSettings::Other::EnableShaderLineNumbers);
		D("Enables proper support for line numbers when editing GLSL shader files. This injects a `#line` preprocessor before your code so that the line numbers will be displayed correctly.     \n> Old graphic cards (eg. the integrated Intel HD ones) do not support this, so if you get a weird GLSL compile error, untick this line.");
		P_();

		P(HiseSettings::Documentation::DocRepository);
		D("The folder of the `hise_documentation` repository. If you want to contribute to the documentation you can setup this folder.");
		D("Otherwise it will use the cached version that was downloaded from the HISE doc server");
		P_();

		P(HiseSettings::Documentation::RefreshOnStartup);
		D("If enabled, HISE will download the latest documentation files from the server when you start HISE. It needs an internet connection for this");
		D("It will download two files, `Content.dat` and `Images.dat`, which contain a compressed version of the HISE documentation");
		P_();

		P(HiseSettings::Audio::BufferSize);
		D("The buffer size in samples that will be used to render the audio. The best numbers are power-of two numbers, which should be preffered if possible.");
		P_();

		P(HiseSettings::Audio::Device);
		D("The sound card that is used to output the sound");
		P_();

		P(HiseSettings::Audio::Driver);
		D("The driver type. On Windows you should choose ASIO (on macOS it's CoreAudio by default)");
		P_();

		P(HiseSettings::Audio::Output);
		D("The output channel if your audio interface has multple outputs");
		P_();

		P(HiseSettings::Audio::Samplerate);
		D("The samplerate that HISE uses for rendering the audio. Make sure that this matches the sample rate on your audio interface, or you will get artifacts.");
		P_();

		P(HiseSettings::Midi::MidiChannels);
		D("A list of all MIDI channels that should be processed. By default all channels are processed, but you might want to filter out some channels depending on your setup.");
		P_();

		P(HiseSettings::Midi::MidiInput);
		D("The MIDI input device list. It should get updated automatically if you connect a new MIDI device - if not try restarting HISE");
		P_();
			
		P(HiseSettings::SnexWorkbench::PlayOnRecompile);
		D("Plays the selected testfile when you compile a script");
		P_();

		return s;

	};

#undef P
#undef D
#undef P_


HiseSettings::Data::Data(MainController* mc_) :
	mc(mc_),
	data("SettingRoot")
{
	for (const auto& id : SettingFiles::getAllIds())
		data.addChild(ValueTree(id), -1, nullptr);

	loadDataFromFiles();
}

juce::File HiseSettings::Data::getFileForSetting(const Identifier& id) const
{
	
	auto appDataFolder = NativeFileHandler::getAppDataDirectory(nullptr);

	if (id == SettingFiles::AudioSettings)		return appDataFolder.getChildFile("DeviceSettings.xml");
	else if (id == SettingFiles::MidiSettings)		return appDataFolder.getChildFile("DeviceSettings.xml");
	else if (id == SettingFiles::GeneralSettings)	return appDataFolder.getChildFile("GeneralSettings.xml");

#if USE_BACKEND

	auto handler_ = &GET_PROJECT_HANDLER(mc->getMainSynthChain());

	auto wd = handler_->getWorkDirectory();

	if (wd.isDirectory())
	{
		if (id == SettingFiles::ProjectSettings)	return handler_->getWorkDirectory().getChildFile("project_info.xml");
		else if (id == SettingFiles::UserSettings)	return handler_->getWorkDirectory().getChildFile("user_info.xml");
		else if (id == SettingFiles::ExpansionSettings) return handler_->getWorkDirectory().getChildFile("expansion_info.xml");
	}

	if (id == SettingFiles::CompilerSettings)	return appDataFolder.getChildFile("compilerSettings.xml");
	else if (id == SettingFiles::ScriptingSettings)	return appDataFolder.getChildFile("ScriptSettings.xml");
	else if (id == SettingFiles::OtherSettings)		return appDataFolder.getChildFile("OtherSettings.xml");
	else if (id == SettingFiles::DocSettings)		return appDataFolder.getChildFile("DocSettings.xml");
	else if (id == SettingFiles::SnexWorkbenchSettings) return appDataFolder.getChildFile("SnexWorkbench.xml");

#endif
	
	return File();
}

void HiseSettings::Data::loadDataFromFiles()
{
	for (const auto& id : SettingFiles::getAllIds())
		loadSettingsFromFile(id);
}

void HiseSettings::Data::refreshProjectData()
{
	loadSettingsFromFile(SettingFiles::ProjectSettings);
	loadSettingsFromFile(SettingFiles::UserSettings);
    loadSettingsFromFile(SettingFiles::ExpansionSettings);
}

void HiseSettings::Data::loadSettingsFromFile(const Identifier& id)
{
	auto f = getFileForSetting(id);

	ValueTree v = ConversionHelpers::loadValueTreeFromFile(f, id);

	if (!v.isValid())
		v = ValueTree(id);

	data.removeChild(data.getChildWithName(id), nullptr);
	data.addChild(v, -1, nullptr);

	addMissingSettings(v, id);
}

File HiseSettings::Data::getFaustPath() const
{
#if JUCE_MAC
    auto hisePath = File(getSetting(HiseSettings::Compiler::HisePath).toString());
    return hisePath.getChildFile("tools").getChildFile("faust");
#else
	return File(getSetting(HiseSettings::Compiler::FaustPath).toString());
#endif
}

var HiseSettings::Data::getSetting(const Identifier& id) const
{
	for (const auto& c : data)
	{
		auto prop = c.getChildWithName(id);

		static const Identifier va("value");

		if (prop.isValid())
		{
			auto value = prop.getProperty(va);

			if (value == "Yes")
				return var(true);

			if (value == "No")
				return var(false);

			return value;
		}
	}

	auto value = getDefaultSetting(id);
    
    if(value == "Yes")
        return var(true);
    else if (value == "No")
        return var(false);
    else
        return value;
}

var HiseSettings::Data::getExtraDefinitionsAsObject() const
{
#if JUCE_WINDOWS
    
    auto defName = Project::ExtraDefinitionsWindows;
#elif JUCE_MAC
    auto defName = Project::ExtraDefinitionsOSX;
#elif JUCE_LINUX
    auto defName = Project::ExtraDefinitionsLinux;
#endif

    auto s = getSetting(defName).toString();

    StringArray items;

    if (s.contains(","))
        items = StringArray::fromTokens(s, ",", "");
    else if (s.contains(";"))
        items = StringArray::fromTokens(s, ";", "");
    else
        items = StringArray::fromLines(s);

    DynamicObject::Ptr obj = new DynamicObject();

    for (auto i : items)
    {
        i = i.trim();
        
        if(i.isEmpty())
            continue;
        
        obj->setProperty(i.upToFirstOccurrenceOf("=", false, false).trim(), i.fromFirstOccurrenceOf("=", false, false).trim());
    }
    
    for(const auto& sp: temporaryExtraDefinitions)
        obj->setProperty(sp.name, sp.value);
    
    return var(obj.get());
}

String HiseSettings::Data::getTemporaryDefinitionsAsString() const
{
    String s;
    
    if(temporaryExtraDefinitions.isEmpty())
        return s;
    
    for(const auto& p: temporaryExtraDefinitions)
        s << "\n" << p.name.toString() << "=" << p.value.toString();
    
    return s;
}

void HiseSettings::Data::addSetting(ValueTree& v, const Identifier& id)
{
	if (v.getChildWithName(id).isValid())
		return;

	ValueTree child(id);
	child.setProperty("value", getDefaultSetting(id), nullptr);
	v.addChild(child, -1, nullptr);
}

juce::StringArray HiseSettings::Data::getOptionsFor(const Identifier& id)
{
	if (id == Project::EmbedAudioFiles ||
		id == Project::EmbedImageFiles ||
		id == Project::EmbedUserPresets ||
		id == Project::OverwriteOldUserPresets ||
		id == Compiler::UseIPP ||
        id == Compiler::LegacyCPUSupport ||
        id == Compiler::EnableLoris ||
		id == Scripting::EnableCallstack ||
		id == Other::EnableAutosave ||
		id == Scripting::EnableDebugMode ||
		id == Scripting::EnableOptimizations ||
		id == Other::AudioThreadGuardEnabled ||
		id == Other::UseOpenGL ||
        id == Other::AutoShowWorkspace ||
		id == Other::EnableShaderLineNumbers ||
		id == Compiler::RebuildPoolFiles ||
		id == Compiler::Support32BitMacOS ||
        id == Compiler::FaustExternalEditor ||
		id == Compiler::ExportSetup ||
		id == Project::SupportMonoFX ||
		id == Project::EnableMidiInputFX ||
        id == Project::EnableMidiOut ||
		id == Project::EnableSoundGeneratorsFX ||
		id == Project::VST3Support ||
		id == Project::UseRawFrontend ||
		id == Project::LinkExpansionsToProject ||
		id == Project::SupportFullDynamicsHLAC ||
		id == Project::ReadOnlyFactoryPresets ||
        id == Project::ForceStereoOutput ||
		id == Project::AdminPermissions ||
		id == Project::EnableGlobalPreprocessor ||
        id == Project::UseGlobalAppDataFolderWindows ||
        id == Project::UseGlobalAppDataFolderMacOS ||
		id == Project::CompileWithPerfetto ||
		id == Project::CompileWithDebugSymbols ||
		id == Project::IncludeLorisInFrontend ||
		id == Documentation::RefreshOnStartup ||
		id == SnexWorkbench::PlayOnRecompile ||
		id == SnexWorkbench::AddFade ||
		id == Scripting::SaveConnectedFilesOnCompile ||
        id == Scripting::WarnIfUndefinedParameters ||
		id == Scripting::EnableMousePositioning)

	    return { "Yes", "No" };

	if (id == Compiler::VisualStudioVersion)
		return { "Visual Studio 2017", "Visual Studio 2022" };

	if (id == Project::ExpansionType)
	{
		return { "Disabled", "FilesOnly", "Encrypted", "Full", "Custom" };
	}

    if(id == Project::VST3Category)
    {
        return {
            "Analyzer",
            "Delay",
            "Distortion",
            "Dynamics",
            "EQ",
            "Filter",
            "Generator",
            "Mastering",
            "Modulation",
            "Pitch Shift",
            "Restoration",
            "Reverb",
            "Spatial",
            "Surround",
            "Tools",
            "Drum",
            "Synth",
            "Sampler"
        };
    }
	if (id == Project::AAXCategoryFX)
		return {
			"AAX_ePlugInCategory_EQ",
			"AAX_ePlugInCategory_Dynamics",
			"AAX_ePlugInCategory_PitchShift",
			"AAX_ePlugInCategory_Reverb",
			"AAX_ePlugInCategory_Delay",
			"AAX_ePlugInCategory_Modulation",
			"AAX_ePlugInCategory_Harmonic",
			"AAX_ePlugInCategory_NoiseReduction",
			"AAX_ePlugInCategory_Dither",
			"AAX_ePlugInCategory_SoundField",
			"AAX_EPlugInCategory_Effect"
		};

#if IS_STANDALONE_APP
	else if (Audio::getAllIds().contains(id))
	{
		auto manager = dynamic_cast<AudioProcessorDriver*>(mc)->deviceManager;


		if (manager == nullptr)
			return {};

		StringArray sa;

		if (id == Audio::Driver)
		{
			

			const auto& list = manager->getAvailableDeviceTypes();
			for (auto l : list)
				sa.add(l->getTypeName());

		}
		else if (id == Audio::Device)
		{
			const auto currentDevice = manager->getCurrentDeviceTypeObject();
			return currentDevice->getDeviceNames();
		}
		else if (id == Audio::BufferSize)
		{
			const auto currentDevice = manager->getCurrentAudioDevice();
			const auto& bs = ConversionHelpers::getBufferSizesForDevice(currentDevice);

			for (auto l : bs)
				sa.add(String(l));
		}
		else if (id == Audio::Samplerate)
		{
			const auto currentDevice = manager->getCurrentAudioDevice();
			const auto& bs = ConversionHelpers::getSampleRates(currentDevice);

			for (auto l : bs)
				sa.add(String(roundToInt(l)));
		}
		else if (id == Audio::Output)
		{
			const auto currentDevice = manager->getCurrentAudioDevice();
			return ConversionHelpers::getChannelPairs(currentDevice);
		}
		return sa;
	}
	else if (id == Midi::MidiInput)
	{
		return MidiInput::getDevices();
	}
#endif
	else if (id == Midi::MidiChannels)
	{
		return ConversionHelpers::getChannelList();
	}


	return {};
}

bool HiseSettings::Data::isFileId(const Identifier& id)
{
	return id == Compiler::HisePath || 
		   id == Scripting::GlobalScriptPath || 
		   id == Project::RedirectSampleFolder ||
		   id == Compiler::CustomNodePath ||
		   id == Compiler::FaustPath ||
		   id == Other::GlobalSamplePath ||
		   id == Other::ExternalEditorPath ||
		   id == Documentation::DocRepository;
}


bool HiseSettings::Data::isToggleListId(const Identifier& id)
{
	return id == Midi::MidiInput;
}

void HiseSettings::Data::addMissingSettings(ValueTree& v, const Identifier &id)
{
	Array<Identifier> ids;

	if (id == SettingFiles::ProjectSettings)		ids = Project::getAllIds();
	else if (id == SettingFiles::UserSettings)		ids = User::getAllIds();
	else if (id == SettingFiles::CompilerSettings)	ids = Compiler::getAllIds();
	else if (id == SettingFiles::ScriptingSettings) ids = Scripting::getAllIds();
	else if (id == SettingFiles::OtherSettings)		ids = Other::getAllIds();
	else if (id == SettingFiles::ExpansionSettings) ids = ExpansionSettings::getAllIds();
	else if (id == SettingFiles::DocSettings)		ids = Documentation::getAllIds();
	else if (id == SettingFiles::SnexWorkbenchSettings) ids = SnexWorkbench::getAllIds();

	for (const auto& id_ : ids)
		addSetting(v, id_);
}

juce::AudioDeviceManager* HiseSettings::Data::getDeviceManager()
{
	return dynamic_cast<AudioProcessorDriver*>(mc)->deviceManager;
}

void HiseSettings::Data::initialiseAudioDriverData(bool forceReload/*=false*/)
{
	ignoreUnused(forceReload);

#if IS_STANDALONE_APP
	static const Identifier va("value");

	auto v = data.getChildWithName(SettingFiles::AudioSettings);
	
	for (const auto& id : Audio::getAllIds())
	{
		if (forceReload)
			v.getChildWithName(id).setProperty(va, getDefaultSetting(id), nullptr);
		else
			addSetting(v, id);
	}

	auto v2 = data.getChildWithName(SettingFiles::MidiSettings);
	
	for (const auto& id : Midi::getAllIds())
	{
		if (forceReload)
			v2.getChildWithName(id).setProperty(va, getDefaultSetting(id), nullptr);
		else
			addSetting(v2, id);
	}

#endif
}

var HiseSettings::Data::getDefaultSetting(const Identifier& id) const
{
	BACKEND_ONLY(auto& handler_ = GET_PROJECT_HANDLER(mc->getMainSynthChain()));

	if (id == Project::Name)
	{
		BACKEND_ONLY(return handler_.getWorkDirectory().getFileName());
	}
	else if (id == Project::Version)			    return "1.0.0";
	else if (id == Project::BundleIdentifier)	    return "com.myCompany.product";
	else if (id == Project::PluginCode)			    return "Abcd";
	else if (id == Project::EmbedAudioFiles)		return "Yes";
	else if (id == Project::EmbedImageFiles)		return "Yes";
	else if (id == Project::EmbedUserPresets)		return "Yes";
	else if (id == Project::OverwriteOldUserPresets)    return "No";
	else if (id == Project::SupportFullDynamicsHLAC)	return "No";
	else if (id == Project::RedirectSampleFolder)	BACKEND_ONLY(return handler_.isRedirected(ProjectHandler::SubDirectories::Samples) ? handler_.getSubDirectory(ProjectHandler::SubDirectories::Samples).getFullPathName() : "");
    else if (id == Project::AAXCategoryFX)			return "AAX_ePlugInCategory_Modulation";           
    else if (id == Project::VST3Category)           return "";
	else if (id == Project::SupportMonoFX)			return "No";
	else if (id == Project::EnableMidiInputFX)		return "No";
    else if (id == Project::EnableMidiOut)          return "No";
	else if (id == Project::EnableSoundGeneratorsFX) return "No";
	else if (id == Project::ReadOnlyFactoryPresets) return "No";
    else if (id == Project::ForceStereoOutput)      return "No";
		else if (id == Project::AdminPermissions) return "No";
	else if (id == Project::VST3Support)			return "No";
	else if (id == Project::UseRawFrontend)			return "No";
	else if (id == Project::CompileWithPerfetto)	return "No";
	else if (id == Compiler::ExportSetup)			return "No";
	else if (id == Project::CompileWithDebugSymbols) return "No";
	else if (id == Project::ExpansionType)			return "Disabled";
	else if (id == Project::LinkExpansionsToProject)       return "No";
	else if (id == Project::EnableGlobalPreprocessor)      return "No";
    else if (id == Project::UseGlobalAppDataFolderWindows) return "No";
    else if (id == Project::UseGlobalAppDataFolderMacOS)   return "No";
	else if (id == Project::IncludeLorisInFrontend) return "No"; // return "Yes"; everybody straight to jail...
	else if (id == Other::UseOpenGL)				return "No";
	else if (id == Other::EnableAutosave)			return "Yes";
	else if (id == Other::AutosaveInterval)			return 5;
	else if (id == Other::AudioThreadGuardEnabled)  return "Yes";
    else if (id == Other::AutoShowWorkspace)        return "Yes";
	else if (id == Other::ExternalEditorPath)		return "";
	else if (id == Documentation::DocRepository)	return "";
	else if (id == Documentation::RefreshOnStartup) return "Yes";
	else if (id == Scripting::CodeFontSize)			return 17.0;
	else if (id == Scripting::EnableCallstack)		return "No";
	else if (id == Scripting::EnableOptimizations)	return "No";
	else if (id == Scripting::EnableMousePositioning) return "Yes";
	else if (id == Scripting::CompileTimeout)		return 5.0;
	else if (id == Scripting::SaveConnectedFilesOnCompile) return "No";
#if HISE_USE_VS2022
	else if (id == Compiler::VisualStudioVersion)	return "Visual Studio 2022";
#else
	else if (id == Compiler::VisualStudioVersion)	return "Visual Studio 2017";
#endif
	else if (id == Compiler::UseIPP)				return "Yes";
	else if (id == Compiler::LegacyCPUSupport) 		return "No";
	else if (id == Compiler::RebuildPoolFiles)		return "Yes";
	else if (id == Compiler::Support32BitMacOS)		return "Yes";
    else if (id == Compiler::FaustExternalEditor)   return "No";
    else if (id == Compiler::EnableLoris)           return "No";
	else if (id == SnexWorkbench::AddFade)			return "Yes";
	else if (id == SnexWorkbench::PlayOnRecompile)  return "Yes";
	else if (id == User::CompanyURL)				return "http://yourcompany.com";
	else if (id == User::CompanyCopyright)			return "(c)2017, Company";
	else if (id == User::CompanyCode)				return "Abcd";
	else if (id == User::Company)					return "My Company";
	else if (id == Other::GlobalSamplePath)         return "";
	else if (id == Scripting::GlobalScriptPath)		
	{
		FRONTEND_ONLY(jassertfalse);

		File scriptFolder = File(NativeFileHandler::getAppDataDirectory(nullptr)).getChildFile("scripts");
		if (!scriptFolder.isDirectory())
			scriptFolder.createDirectory();

		return scriptFolder.getFullPathName();
	}
	else if (id == Scripting::EnableDebugMode)		return mc->getDebugLogger().isLogging() ? "Yes" : "No";
    else if (id == Scripting::WarnIfUndefinedParameters) return "Yes";
	else if (id == Audio::Driver)					return const_cast<Data*>(this)->getDeviceManager()->getCurrentAudioDeviceType();
	else if (id == Audio::Device)
	{
		auto device = dynamic_cast<AudioProcessorDriver*>(mc)->deviceManager->getCurrentAudioDevice();
		return device != nullptr ? device->getName() : "No Device";
	}
	else if (id == Audio::Output)
	{
		auto device = dynamic_cast<AudioProcessorDriver*>(mc)->deviceManager->getCurrentAudioDevice();

		return ConversionHelpers::getCurrentOutputName(device);
	}
	else if (id == Audio::Samplerate)				return dynamic_cast<AudioProcessorDriver*>(mc)->getCurrentSampleRate();
	else if (id == Audio::BufferSize)				return dynamic_cast<AudioProcessorDriver*>(mc)->getCurrentBlockSize();
	else if (id == Midi::MidiInput)					return dynamic_cast<AudioProcessorDriver*>(mc)->getMidiInputState().toInt64();
	else if (id == Midi::MidiChannels)
	{
		auto state = BigInteger(dynamic_cast<AudioProcessorDriver*>(mc)->getChannelData());
		auto firstSetBit = state.getHighestBit();
		return ConversionHelpers::getChannelList()[firstSetBit];
	}

	return var();
}

juce::Result HiseSettings::Data::checkInput(const Identifier& id, const var& newValue)
{
	if (id == Other::AutosaveInterval && !TestFunctions::isValidNumberBetween(newValue, { 1.0f, 30.0f }))
		return Result::fail("The autosave interval must be between 1 and 30 minutes");

	if (id == Project::Version)
	{
		const String version = newValue.toString();
		SemanticVersionChecker versionChecker(version, version);

		if (!versionChecker.newVersionNumberIsValid())
		{
			return Result::fail("The version number is not a valid semantic version number. Use something like 1.0.0.\n " \
				"This is required for the user presets to detect whether it should ask for updating the presets after a version bump.");
		};
	}

	if (id == Project::AppGroupID || id == Project::BundleIdentifier)
	{
		const String wildcard = (id == HiseSettings::Project::BundleIdentifier) ?
			R"(com\.[\w_]+\.[\w_]+$)" :
			R"(group\.[\w_]+\.[\w_]+$)";

		if (!RegexFunctions::matchesWildcard(wildcard, newValue.toString()))
		{
			return Result::fail(id.toString() + " doesn't match the required format.");
		}
	}

	if (id == Project::PluginCode || id == User::CompanyCode)
	{
		const String pluginCode = newValue.toString();
		const String codeWildcard = "[A-Z][a-z][a-z][a-z]";

		if (pluginCode.length() != 4 || !RegexFunctions::matchesWildcard(codeWildcard, pluginCode))
		{
			return Result::fail("The code doesn't match the required formula. Use something like 'Abcd'\n" \
				"This is required for exported AU plugins to pass the AU validation.");
		};
	}

	if (id == Project::Name || id == User::Company)
	{
		const String name = newValue.toString();

		if (!name.containsOnly("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890 _-"))
		{
			return Result::fail("Illegal Project name\n" \
				"The Project name must not contain exotic characters");
		}

		if (name.isEmpty())
			return Result::fail("The project name / company name must not be empty");
	}

	if (id == Compiler::HisePath)
	{
		File f = File(newValue.toString());

		if (!f.isDirectory())
			return Result::fail("The HISE path is not a valid directory");

		if (!f.getChildFile("hi_core").isDirectory())
			return Result::fail("The HISE path does not contain the HISE source code");
	}

	if (id == Scripting::GlobalScriptPath && !File(newValue.toString()).isDirectory())
		return Result::fail("The global script folder is not a valid directory");

	return Result::ok();
}

void HiseSettings::Data::settingWasChanged(const Identifier& id, const var& newValue)
{
	if (id == Project::RedirectSampleFolder)
	{
#if USE_BACKEND
		auto& handler_ = GET_PROJECT_HANDLER(mc->getMainSynthChain());

		if (File::isAbsolutePath(newValue.toString()))
			handler_.createLinkFile(ProjectHandler::SubDirectories::Samples, File(newValue.toString()));
		else
			ProjectHandler::getLinkFile(handler_.getWorkDirectory().getChildFile("Samples")).deleteFile();
#endif
	}

	if (id == Scripting::EnableCallstack)
		mc->updateCallstackSettingForExistingScriptProcessors();

	else if (id == Scripting::CodeFontSize)
		mc->getFontSizeChangeBroadcaster().sendMessage(sendNotification, (float)newValue);
	else if (id == Other::UseOpenGL)
		PresetHandler::showMessageWindow("Reopen HISE window", "Restart HISE (or reopen this window) in order to apply the new Graphics setting", PresetHandler::IconType::Info);
	else if (id == Other::EnableAutosave || id == Other::AutosaveInterval)
		mc->getAutoSaver().updateAutosaving();
	else if (id == Other::AudioThreadGuardEnabled)
		mc->getKillStateHandler().enableAudioThreadGuard(newValue);
	else if (id == Scripting::EnableOptimizations)
		mc->compileAllScripts();
	else if (id == Scripting::EnableDebugMode)
		newValue ? mc->getDebugLogger().startLogging() : mc->getDebugLogger().stopLogging();
	else if (id == Audio::Samplerate)
		dynamic_cast<AudioProcessorDriver*>(mc)->setCurrentSampleRate(newValue.toString().getDoubleValue());
	else if (id == Audio::BufferSize)
		dynamic_cast<AudioProcessorDriver*>(mc)->setCurrentBlockSize(newValue.toString().getIntValue());
	else if (id == Audio::Driver)
	{
		if (newValue.toString().isNotEmpty())
		{
			auto driver = dynamic_cast<AudioProcessorDriver*>(mc);
			driver->deviceManager->setCurrentAudioDeviceType(newValue.toString(), true);
			auto device = driver->deviceManager->getCurrentAudioDevice();

			if (device == nullptr)
			{
				PresetHandler::showMessageWindow("Error initialising driver", "The audio driver could not be opened. The default settings will be loaded.", PresetHandler::IconType::Error);
				driver->resetToDefault();
			}

			initialiseAudioDriverData(true);
			sendChangeMessage();
		}
	}
	else if (id == Audio::Output)
	{
		if (newValue.toString().isNotEmpty())
		{
			auto driver = dynamic_cast<AudioProcessorDriver*>(mc);
			auto device = driver->deviceManager->getCurrentAudioDevice();
			auto list = ConversionHelpers::getChannelPairs(device);
			auto outputIndex = list.indexOf(newValue.toString());

			if (outputIndex != -1)
			{
				AudioDeviceManager::AudioDeviceSetup config;
				driver->deviceManager->getAudioDeviceSetup(config);

				auto& original = config.outputChannels;

				original.clear();
				original.setBit(outputIndex * 2, 1);
				original.setBit(outputIndex * 2 + 1, 1);

				config.useDefaultOutputChannels = false;

				driver->deviceManager->setAudioDeviceSetup(config, true);
			}
		}
	}
	else if (id == Audio::Device)
	{
		if (newValue.toString().isNotEmpty())
		{
			auto driver = dynamic_cast<AudioProcessorDriver*>(mc);

			driver->setAudioDevice(newValue.toString());

			auto device = driver->deviceManager->getCurrentAudioDevice();

			if (device == nullptr)
			{
				PresetHandler::showMessageWindow("Error initialising driver", "The audio driver could not be opened. The default settings will be loaded.", PresetHandler::IconType::Error);
				driver->resetToDefault();
			}

			initialiseAudioDriverData(true);
			sendChangeMessage();
		}
	}
	else if (id == Midi::MidiInput)
	{
		auto state = BigInteger((int64)newValue);
		auto driver = dynamic_cast<AudioProcessorDriver*>(mc);

		auto midiNames = MidiInput::getDevices();

		for (int i = 0; i < midiNames.size(); i++)
			driver->toggleMidiInput(midiNames[i], state[i]);
	}
	else if (id == Project::LinkExpansionsToProject)
	{
		auto shouldRedirect = (bool)newValue;

		auto company = getSetting(User::Company).toString();
		auto project = getSetting(Project::Name).toString();

		auto expFolder = ProjectHandler::getAppDataRoot(mc).getChildFile(company).getChildFile(project).getChildFile("Expansions");

		auto expRoot = mc->getExpansionHandler().getExpansionFolder();

		if (shouldRedirect)
			ProjectHandler::createLinkFileInFolder(expFolder, expRoot);
		else
			ProjectHandler::createLinkFileInFolder(expFolder, {});
	}

	else if (id == Midi::MidiChannels)
	{
		auto sa = HiseSettings::ConversionHelpers::getChannelList();
		auto index = sa.indexOf(newValue.toString());

		BigInteger s = 0;
		s.setBit(index, true);

		auto intValue = s.toInteger();
		auto channelData = mc->getMainSynthChain()->getActiveChannelData();

		channelData->restoreFromData(intValue);
	}
}

bool HiseSettings::Data::TestFunctions::isValidNumberBetween(var value, Range<float> range)
{
	auto number = value.toString().getFloatValue();

	if (std::isnan(number))
		return false;

	if (std::isinf(number))
		return false;

	number = FloatSanitizers::sanitizeFloatNumber(number);

	return range.contains(number);
}

juce::String HiseSettings::ConversionHelpers::getUncamelcasedId(const Identifier& id)
{
	auto n = id.toString();
	String pretty;
	auto ptr = n.getCharPointer();
	bool lastWasUppercase = true;

	while (!ptr.isEmpty())
	{
		if (ptr.isUpperCase() && !lastWasUppercase)
			pretty << " ";

		lastWasUppercase = ptr.isUpperCase();
		pretty << ptr.getAddress()[0];
		ptr++;
	}

	return pretty;
}

juce::StringArray HiseSettings::ConversionHelpers::getChannelPairs(AudioIODevice* currentDevice)
{
	if (currentDevice != nullptr)
	{
		StringArray items = currentDevice->getOutputChannelNames();

		StringArray pairs;

		for (int i = 0; i < items.size(); i += 2)
		{
			const String& name = items[i];

			if (i + 1 >= items.size())
				pairs.add(name.trim());
			else
				pairs.add(getNameForChannelPair(name, items[i + 1]));
		}

		return pairs;
	}

	return StringArray();
}

juce::String HiseSettings::ConversionHelpers::getNameForChannelPair(const String& name1, const String& name2)
{
	String commonBit;

	for (int j = 0; j < name1.length(); ++j)
		if (name1.substring(0, j).equalsIgnoreCase(name2.substring(0, j)))
			commonBit = name1.substring(0, j);

	// Make sure we only split the name at a space, because otherwise, things
	// like "input 11" + "input 12" would become "input 11 + 2"
	while (commonBit.isNotEmpty() && !CharacterFunctions::isWhitespace(commonBit.getLastCharacter()))
		commonBit = commonBit.dropLastCharacters(1);

	return name1.trim() + " + " + name2.substring(commonBit.length()).trim();
}

juce::String HiseSettings::ConversionHelpers::getCurrentOutputName(AudioIODevice* currentDevice)
{
	if(currentDevice != nullptr)
	{
		auto list = getChannelPairs(currentDevice);
		const int thisOutputName = (currentDevice->getActiveOutputChannels().getHighestBit() - 1) / 2;

		return list[thisOutputName];
	}

	return "";
}

}
