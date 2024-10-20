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


#ifndef HISESETTINGS_H_INCLUDED
#define HISESETTINGS_H_INCLUDED

namespace hise {
using namespace juce;

namespace HiseSettings {

#define DECLARE_ID(x) const juce::Identifier x(#x);

namespace SettingFiles
{
DECLARE_ID(ProjectSettings);
DECLARE_ID(UserSettings);
DECLARE_ID(CompilerSettings);
DECLARE_ID(GeneralSettings);
DECLARE_ID(ExpansionSettings);
DECLARE_ID(AudioSettings);
DECLARE_ID(MidiSettings);
DECLARE_ID(ScriptingSettings);
DECLARE_ID(OtherSettings);
DECLARE_ID(DocSettings);
DECLARE_ID(SnexWorkbenchSettings);

Array<Identifier> getAllIds();

}

namespace Project
{
DECLARE_ID(Name);
DECLARE_ID(Version);
DECLARE_ID(Description);
DECLARE_ID(BundleIdentifier);
DECLARE_ID(PluginCode);
DECLARE_ID(EmbedAudioFiles);
DECLARE_ID(EmbedImageFiles);
DECLARE_ID(SupportFullDynamicsHLAC);
DECLARE_ID(AdditionalDspLibraries);
DECLARE_ID(OSXStaticLibs);
DECLARE_ID(WindowsStaticLibFolder);
DECLARE_ID(ExtraDefinitionsWindows);
DECLARE_ID(ExtraDefinitionsOSX);
DECLARE_ID(ExtraDefinitionsIOS);
DECLARE_ID(ExtraDefinitionsLinux);
DECLARE_ID(ExtraDefinitionsNetworkDll);
DECLARE_ID(AppGroupID);
DECLARE_ID(RedirectSampleFolder);
DECLARE_ID(AAXCategoryFX);
DECLARE_ID(VST3Category);
DECLARE_ID(SupportMonoFX);
DECLARE_ID(EnableSoundGeneratorsFX);
DECLARE_ID(EnableMidiInputFX);
DECLARE_ID(EnableMidiOut);
DECLARE_ID(EnableGlobalPreprocessor);
DECLARE_ID(UseRawFrontend);
DECLARE_ID(VST3Support);
DECLARE_ID(ExpansionType);
DECLARE_ID(EncryptionKey);
DECLARE_ID(LinkExpansionsToProject);
DECLARE_ID(ReadOnlyFactoryPresets);
DECLARE_ID(ForceStereoOutput);
DECLARE_ID(AdminPermissions);
DECLARE_ID(EmbedUserPresets);
DECLARE_ID(OverwriteOldUserPresets);
DECLARE_ID(UseGlobalAppDataFolderWindows);
DECLARE_ID(UseGlobalAppDataFolderMacOS);
DECLARE_ID(DefaultUserPreset);
DECLARE_ID(CompileWithPerfetto);
DECLARE_ID(CompileWithDebugSymbols);
DECLARE_ID(IncludeLorisInFrontend);

Array<Identifier> getAllIds();

} // Project

namespace Compiler
{
DECLARE_ID(HisePath);
DECLARE_ID(VisualStudioVersion);
DECLARE_ID(UseIPP);
DECLARE_ID(LegacyCPUSupport);
DECLARE_ID(RebuildPoolFiles);
DECLARE_ID(Support32BitMacOS);
DECLARE_ID(CustomNodePath);
DECLARE_ID(FaustPath);
DECLARE_ID(FaustExternalEditor);
DECLARE_ID(EnableLoris);
DECLARE_ID(ExportSetup);
DECLARE_ID(DefaultProjectFolder);

Array<Identifier> getAllIds();

} // Compiler

namespace ExpansionSettings
{
DECLARE_ID(UUID);
DECLARE_ID(Tags);
DECLARE_ID(Description);

Array<Identifier> getAllIds();
}

namespace User
{
DECLARE_ID(Company);
DECLARE_ID(CompanyCode);
DECLARE_ID(CompanyURL);
DECLARE_ID(CompanyCopyright);
DECLARE_ID(TeamDevelopmentID);

Array<Identifier> getAllIds();

} // User

namespace Scripting
{
DECLARE_ID(EnableCallstack);
DECLARE_ID(GlobalScriptPath);
DECLARE_ID(CompileTimeout);
DECLARE_ID(CodeFontSize);
DECLARE_ID(EnableOptimizations);
DECLARE_ID(EnableDebugMode);
DECLARE_ID(WarnIfUndefinedParameters);
DECLARE_ID(SaveConnectedFilesOnCompile);
DECLARE_ID(EnableMousePositioning);

Array<Identifier> getAllIds();

} // Scripting

namespace Other
{
DECLARE_ID(GlobalSamplePath);
DECLARE_ID(UseOpenGL);
DECLARE_ID(ShowWelcomeScreen);
DECLARE_ID(GlobalHiseScaleFactor);
DECLARE_ID(EnableShaderLineNumbers);
DECLARE_ID(EnableAutosave);
DECLARE_ID(AutosaveInterval);
DECLARE_ID(AudioThreadGuardEnabled);
DECLARE_ID(ExternalEditorPath);
DECLARE_ID(AutoShowWorkspace);

Array<Identifier> getAllIds();

} // Other

namespace Documentation
{
DECLARE_ID(DocRepository);
DECLARE_ID(RefreshOnStartup);

Array<Identifier> getAllIds();
}

namespace Audio
{
DECLARE_ID(Driver);
DECLARE_ID(Device);
DECLARE_ID(Output);
DECLARE_ID(Samplerate);
DECLARE_ID(BufferSize);

Array<Identifier> getAllIds();

} // Audio

namespace Midi
{
DECLARE_ID(MidiInput);
DECLARE_ID(MidiChannels);

Array<Identifier> getAllIds();

} // Midi

namespace SnexWorkbench
{
DECLARE_ID(PlayOnRecompile);
DECLARE_ID(AddFade);

Array<Identifier> getAllIds();
}

#undef DECLARE_ID


struct Data: public SafeChangeBroadcaster
{
	Data(MainController* mc_);

	File getFileForSetting(const Identifier& id) const;

    File getFaustPath() const;
    
	void loadDataFromFiles();
	void refreshProjectData();
	void loadSettingsFromFile(const Identifier& id);

    var getExtraDefinitionsAsObject() const;
    
	var getSetting(const Identifier& id) const;

	void initialiseAudioDriverData(bool forceReload = false);

	StringArray getOptionsFor(const Identifier& id);

	static bool isFileId(const Identifier& id);

	static bool isToggleListId(const Identifier& id);

	MainController* getMainController() { return mc; }
	const MainController* getMainController() const { return mc; }

	var getDefaultSetting(const Identifier& id) const;

	ValueTree data;

	static Result checkInput(const Identifier& id, const var& newValue);

	void settingWasChanged(const Identifier& id, const var& newValue);

    String getTemporaryDefinitionsAsString() const;
    
    void addTemporaryDefinitions(const NamedValueSet& list)
    {
        temporaryExtraDefinitions = list;
    }
    
private:

	struct TestFunctions
	{
		static bool isValidNumberBetween(var value, Range<float> range);
	};

    void addSetting(ValueTree& v, const Identifier& id);
	void addMissingSettings(ValueTree& v, const Identifier &id);

	AudioDeviceManager* getDeviceManager();
	MainController* mc;
    NamedValueSet temporaryExtraDefinitions;
};



#undef DECLARE_ID


struct ConversionHelpers
{
	static ValueTree loadValueTreeFromFile(const File& f, const Identifier& settingid);
	static ValueTree loadValueTreeFromXml(XmlElement* xml, const Identifier& settingId);
	static XmlElement* getConvertedXml(const ValueTree& v);
	static Array<int> getBufferSizesForDevice(AudioIODevice* currentDevice);
	static Array<double> getSampleRates(AudioIODevice* currentDevice);

	static String getUncamelcasedId(const Identifier& id);


	static StringArray getChannelPairs(AudioIODevice* currentDevice);

	static String getNameForChannelPair(const String& name1, const String& name2);

	static String getCurrentOutputName(AudioIODevice* currentDevice);

	static StringArray getChannelList();
};

struct SettingDescription
{
	static String getDescription(const Identifier& id);
};

} // SettingIds



}

#endif
