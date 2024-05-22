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
namespace multipage {

using namespace juce;

namespace Templates
{
#if JUCE_WINDOWS
	static const char* BatchFile = R"(@echo off
set project=%PROJECT%
set build_path=%ROOT_DIR%
set msbuild="C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MsBuild.exe"
set vs_args=/p:Configuration="Release" /verbosity:minimal
set PreferredToolArchitecture=x64
set VisualStudioVersion=17.0

"%HISE_PATH%\tools\Projucer\Projucer.exe" --resave "%build_path%\%PROJECT%.jucer"

echo Compiling 64bit FX plugin %project% ...
set Platform=X64
%msbuild% "%build_path%\Builds\VisualStudio2022\%project%.sln" %vs_args%

pause
)";
#elif JUCE_MAC
static const char* BatchFile = R"(
    chmod +x "%HISE_PATH%/tools/Projucer/Projucer.app/Contents/MacOS/Projucer"
    cd "`dirname "$0"`"
    "%HISE_PATH%/tools/Projucer/Projucer.app/Contents/MacOS/Projucer" --resave "%PROJECT%.jucer"

    set -o pipefail
    echo Compiling %PROJECT% ...
    xcodebuild -project "Builds/MacOSX/%PROJECT%.xcodeproj" -configuration "Release" -jobs "10" | xcpretty
)";
#elif JUCE_LINUX
static const char* BatchFile = R"(
    TODO!!!"
)";
#endif

	static const char* Main_Cpp = R"(#include <JuceHeader.h>
#include "Dialog.h"

//==============================================================================
class MainWrapper: public JUCEApplication
{
public:
    //==============================================================================
    MainWrapper() {}

    const String getApplicationName() override       { return ProjectInfo::projectName; }
    const String getApplicationVersion() override    { return ProjectInfo::versionString; }
    bool moreThanOneInstanceAllowed() override       { return false; }

    //==============================================================================
    void initialise (const String& commandLine) override
    {
        mainWindow.reset (new MainWindow (getApplicationName()));
    }

    void shutdown() override
	{
        mainWindow = nullptr;
    }

    void systemRequestedQuit() override
    {
        quit();
    }

    void anotherInstanceStarted (const String& commandLine) override {}

    class MainWindow    : public DocumentWindow,
						  public hise::multipage::ComponentWithSideTab
    {
    public:
        MainWindow (String name)  : DocumentWindow (name, Colour(0xFF333333), DocumentWindow::closeButton | DocumentWindow::minimiseButton)
        {
            setUsingNativeTitleBar (true);
            auto nc = new DialogClass();
			nc->setOnCloseFunction(BIND_MEMBER_FUNCTION_0(MainWindow::closeButtonPressed));

            state = &nc->state;

            setContentOwned (nc, true);
            setResizable(false, false);
            centreWithSize (getWidth(), getHeight());
            setVisible (true);
        }

        void closeButtonPressed() override
        {
            JUCEApplication::getInstance()->systemRequestedQuit();
        }

        multipage::State* getMainState() override { return state; }

		bool setSideTab(multipage::State* dialogState, multipage::Dialog* newDialog) override
        {
	        delete dialogState;
            delete newDialog;
            return false;
        }

		void refreshDialog() override {};

    private:

        multipage::State* state;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainWindow)
    };

private:
    std::unique_ptr<MainWindow> mainWindow;
};

START_JUCE_APPLICATION (MainWrapper)

)";
	static const char* Projucer_Xml = R"(<?xml version="1.0" encoding="UTF-8"?>

<JUCERPROJECT id="L2bkCQ" name="%PROJECT%" projectType="guiapp" cppLanguageStandard="17"
              displaySplashScreen="0" reportAppUsage="0" splashScreenColour="Dark"
              version="%VERSION%" bundleIdentifier="com.%COMPANY%.%PROJECT%" includeBinaryInAppConfig="1"
              companyCopyright="" headerPath="%HISE_PATH%"
              jucerFormatVersion="1">
  <MAINGROUP id="V5l5vh" name="%PROJECT%">
    <GROUP id="{AC16B56A-5346-1AB4-D4BD-30CF29DD2DFA}" name="Source">
      <GROUP id="{AE8C0D3C-F782-7C53-F11E-959CE393BF6B}" name="Source">
        <FILE id="BBBM94" name="Assets.cpp" compile="1" resource="0" file="Source/Assets.cpp"/>
        <FILE id="Mj9PcC" name="Assets.h" compile="0" resource="0" file="Source/Assets.h"/>
        <FILE id="MOympr" name="Dialog.cpp" compile="1" resource="0" file="Source/Dialog.cpp"/>
        <FILE id="ilgAKy" name="Dialog.h" compile="0" resource="0" file="Source/Dialog.h"/>
        <FILE id="VfEEh3" name="Main.cpp" compile="1" resource="0" file="Source/Main.cpp"/>
      </GROUP>
	  %FILE_OBJECT_LINE%	
    </GROUP>
  </MAINGROUP>
  <EXPORTFORMATS>
    <XCODE_MAC targetFolder="Builds/MacOSX" extraDefs="USE_IPP=0&#10;PERFETTO=0&#10;USE_BACKEND=1"
               extraCompilerFlags="-Wno-reorder -Wno-inconsistent-missing-override -mpopcnt -faligned-allocation -Wno-switch"
               xcodeValidArchs="x86_64" smallIcon="%ICON_REF%" bigIcon="%ICON_REF%" iosDevelopmentTeamID="%TEAM_ID%">
      <CONFIGURATIONS>
        <CONFIGURATION isDebug="1" name="Debug" enablePluginBinaryCopyStep="1" osxArchitecture="64BitIntel"
                       macOSDeploymentTarget="10.15" osxCompatibility="10.15 SDK"  targetName="%BINARY_NAME%"
                       binaryPath="../"/>
        <CONFIGURATION isDebug="0" name="Release" enablePluginBinaryCopyStep="1" osxArchitecture="64BitIntel"
                       headerPath="" libraryPath="" linkTimeOptimisation="0" optimisation="2"
                       stripLocalSymbols="1"  targetName="%BINARY_NAME%"
                       binaryPath="../"/>
      </CONFIGURATIONS>
      <MODULEPATHS>
        <MODULEPATH id="juce_audio_basics" path="%HISE_PATH%/JUCE/modules"/>
        <MODULEPATH id="juce_audio_devices" path="%HISE_PATH%/JUCE/modules"/>
        <MODULEPATH id="juce_audio_formats" path="%HISE_PATH%/JUCE/modules"/>
        <MODULEPATH id="juce_audio_processors" path="%HISE_PATH%/JUCE/modules"/>
        <MODULEPATH id="juce_core" path="%HISE_PATH%/JUCE/modules"/>
        <MODULEPATH id="juce_data_structures" path="%HISE_PATH%/JUCE/modules"/>
        <MODULEPATH id="juce_events" path="%HISE_PATH%/JUCE/modules"/>
        <MODULEPATH id="juce_graphics" path="%HISE_PATH%/JUCE/modules"/>
        <MODULEPATH id="juce_gui_basics" path="%HISE_PATH%/JUCE/modules"/>
        <MODULEPATH id="juce_opengl" path="%HISE_PATH%/JUCE/modules"/>
        <MODULEPATH id="hi_zstd" path="%HISE_PATH%/"/>
        <MODULEPATH id="hi_lac" path="%HISE_PATH%/"/>
        <MODULEPATH id="juce_cryptography" path="%HISE_PATH%/JUCE/modules"/>
        <MODULEPATH id="juce_gui_extra" path="%HISE_PATH%/JUCE/modules"/>
        <MODULEPATH id="hi_snex" path="%HISE_PATH%/"/>
        <MODULEPATH id="hi_dsp_library" path="%HISE_PATH%/"/>
        <MODULEPATH id="juce_dsp" path="%HISE_PATH%/JUCE/modules"/>
        <MODULEPATH id="hi_rlottie" path="%HISE_PATH%/"/>
        <MODULEPATH id="hi_tools" path="%HISE_PATH%/"/>
        <MODULEPATH id="juce_product_unlocking" path="%HISE_PATH%/JUCE/modules"/>
        <MODULEPATH id="melatonin_blur" path="%HISE_PATH%"/>
      </MODULEPATHS>
    </XCODE_MAC>
    <VS2022 targetFolder="Builds/VisualStudio2022" extraCompilerFlags="/bigobj /wd&quot;4100&quot; /wd&quot;4661&quot; /wd&quot;4456&quot; /wd&quot;4244&quot; /wd&quot;4457&quot; /wd&quot;4458&quot; /wd&quot;4127&quot; /Zc:__cplusplus /permissive-"
            extraDefs="HI_RUN_UNIT_TESTS=1&#10;USE_BACKEND=1" extraLinkerFlags="/MANIFESTUAC:level='requireAdministrator'" smallIcon="%ICON_REF%" bigIcon="%ICON_REF%">
      <CONFIGURATIONS>
        <CONFIGURATION isDebug="1" name="Debug" defines="PERFETTO=1&#10;NOMINMAX=1 &#10;WIN32_LEAN_AND_MEAN=1" 
                       targetName="%BINARY_NAME%" binaryPath="../"/>
        <CONFIGURATION isDebug="0" name="Release" optimisation="2" linkTimeOptimisation="0" 
                       targetName="%BINARY_NAME%" binaryPath="../"/>
      </CONFIGURATIONS>
      <MODULEPATHS>
        <MODULEPATH id="juce_opengl" path="%HISE_PATH%\JUCE\modules"/>
        <MODULEPATH id="juce_gui_extra" path="%HISE_PATH%\JUCE\modules"/>
        <MODULEPATH id="juce_gui_basics" path="%HISE_PATH%\JUCE\modules"/>
        <MODULEPATH id="juce_graphics" path="%HISE_PATH%\JUCE\modules"/>
        <MODULEPATH id="juce_events" path="%HISE_PATH%\JUCE\modules"/>
        <MODULEPATH id="juce_dsp" path="%HISE_PATH%\JUCE\modules"/>
        <MODULEPATH id="juce_data_structures" path="%HISE_PATH%\JUCE\modules"/>
        <MODULEPATH id="juce_cryptography" path="%HISE_PATH%\JUCE\modules"/>
        <MODULEPATH id="juce_core" path="%HISE_PATH%\JUCE\modules"/>
        <MODULEPATH id="juce_audio_processors" path="%HISE_PATH%\JUCE\modules"/>
        <MODULEPATH id="juce_audio_formats" path="%HISE_PATH%\JUCE\modules"/>
        <MODULEPATH id="juce_audio_devices" path="%HISE_PATH%\JUCE\modules"/>
        <MODULEPATH id="juce_audio_basics" path="%HISE_PATH%\JUCE\modules"/>
        <MODULEPATH id="hi_zstd" path="%HISE_PATH%"/>
        <MODULEPATH id="hi_snex" path="%HISE_PATH%"/>
        <MODULEPATH id="hi_lac" path="%HISE_PATH%"/>
        <MODULEPATH id="hi_dsp_library" path="%HISE_PATH%"/>
        <MODULEPATH id="hi_rlottie" path="%HISE_PATH%"/>
        <MODULEPATH id="hi_tools" path="%HISE_PATH%"/>
        <MODULEPATH id="juce_product_unlocking" path="%HISE_PATH%\JUCE\modules"/>
        <MODULEPATH id="melatonin_blur" path="%HISE_PATH%"/>
      </MODULEPATHS>
    </VS2022>
  </EXPORTFORMATS>
  <MODULES>
    <MODULE id="hi_dsp_library" showAllCode="1" useLocalCopy="0" useGlobalPath="0"/>
    <MODULE id="hi_lac" showAllCode="1" useLocalCopy="0" useGlobalPath="0"/>
    <MODULE id="hi_rlottie" showAllCode="1" useLocalCopy="0" useGlobalPath="0"/>
    <MODULE id="hi_snex" showAllCode="1" useLocalCopy="0" useGlobalPath="0"/>
    <MODULE id="hi_tools" showAllCode="1" useLocalCopy="0" useGlobalPath="0"/>
    <MODULE id="hi_zstd" showAllCode="1" useLocalCopy="0" useGlobalPath="0"/>
    <MODULE id="juce_audio_basics" showAllCode="1" useLocalCopy="0" useGlobalPath="0"/>
    <MODULE id="juce_audio_devices" showAllCode="1" useLocalCopy="0" useGlobalPath="0"/>
    <MODULE id="juce_audio_formats" showAllCode="1" useLocalCopy="0" useGlobalPath="0"/>
    <MODULE id="juce_audio_processors" showAllCode="1" useLocalCopy="0" useGlobalPath="0"/>
    <MODULE id="juce_core" showAllCode="1" useLocalCopy="0" useGlobalPath="0"/>
    <MODULE id="juce_cryptography" showAllCode="1" useLocalCopy="0" useGlobalPath="0"/>
    <MODULE id="juce_data_structures" showAllCode="1" useLocalCopy="0" useGlobalPath="0"/>
    <MODULE id="juce_dsp" showAllCode="1" useLocalCopy="0" useGlobalPath="0"/>
    <MODULE id="juce_events" showAllCode="1" useLocalCopy="0" useGlobalPath="0"/>
    <MODULE id="juce_graphics" showAllCode="1" useLocalCopy="0" useGlobalPath="0"/>
    <MODULE id="juce_gui_basics" showAllCode="1" useLocalCopy="0" useGlobalPath="0"/>
    <MODULE id="juce_gui_extra" showAllCode="1" useLocalCopy="0" useGlobalPath="0"/>
    <MODULE id="juce_opengl" showAllCode="1" useLocalCopy="0" useGlobalPath="0"/>
    <MODULE id="juce_product_unlocking" showAllCode="1" useLocalCopy="0"
            useGlobalPath="0"/>
    <MODULE id="melatonin_blur" showAllCode="1" useLocalCopy="0" useGlobalPath="0"/>
  </MODULES>
  <LIVE_SETTINGS>
    <WINDOWS/>
    <OSX/>
    <LINUX/>
  </LIVE_SETTINGS>
  <JUCEOPTIONS JUCE_LOAD_CURL_SYMBOLS_LAZILY="1" HI_EXPORT_DSP_LIBRARY="0" HISE_INCLUDE_RLOTTIE="0"
               SNEX_STANDALONE_PLAYGROUND="0" HISE_INCLUDE_SNEX="0" HISE_INCLUDE_PERFETTO="0"
               JUCE_USE_WIN_WEBVIEW2="0" JUCE_ENABLE_LIVE_CONSTANT_EDITOR="0"
               HISE_INCLUDE_LORIS="0" JUCE_USE_BETTER_MACHINE_IDS="1"/>
</JUCERPROJECT>
)";
}

struct Helpers
{
    static void callRecursive(const var& d, const std::function<void(const var&)>& f)
    {
        f(d);
        
        auto l = d[mpid::Children];
        
        if(auto ar = l.getArray())
        {
            for(const auto& a: *ar)
                callRecursive(a, f);
        }
    }
};


void CodeGenerator::write(OutputStream& x, FileType t, State::Job* job) const
{
	x << getNewLine();

	if(t == FileType::BatchFile || t == FileType::ProjucerFile)
	{
		String temp(t == FileType::BatchFile ? Templates::BatchFile : Templates::Projucer_Xml);

		temp = temp.replace("%PROJECT%", className);
		temp = temp.replace("%ROOT_DIR%", rootDirectory.getChildFile("Binaries").getFullPathName());
		temp = temp.replace("%COMPANY%", company);
		temp = temp.replace("%VERSION%", version);
		temp = temp.replace("%HISE_PATH%", hisePath);
		temp = temp.replace("%BINARY_NAME%", data[mpid::Properties][mpid::BinaryName].toString());
        temp = temp.replace("%TEAM_ID%", teamId);
        
		File iconFile;

		auto iconId = data[mpid::Properties][mpid::Icon].toString().removeCharacters("${}");

		if(iconId.isNotEmpty())
		{
			if(auto ar = data[mpid::Assets].getArray())
			{
				for(auto& v: *ar)
				{
					auto assetID =  v[mpid::ID].toString();

					if(assetID == iconId)
					{
						auto fn = v[mpid::Filename].toString();

						if(File::isAbsolutePath(fn))
							iconFile = File(fn);
						else
							iconFile = rootDirectory.getChildFile(fn);

						break;
					}
				}
			}
		}

		String fileObjectLine = "";
		String iconRef = "";

		if(iconFile.existsAsFile())
		{
			fileObjectLine = String(R"(<FILE id="t5fgDK" name="%ICON_NAME%" compile="0" resource="1" file="%RELATIVE%"/>)");
			iconRef = "t5fgDK";

			fileObjectLine = fileObjectLine.replace("%RELATIVE%", iconFile.getRelativePathFrom(rootDirectory.getChildFile("Binaries")));
			fileObjectLine = fileObjectLine.replace("%ICON_NAME%", iconFile.getFileName());
		}

		temp = temp.replace("%ICON_REF%", iconRef);
		temp = temp.replace("%FILE_OBJECT_LINE%", fileObjectLine);
		
		x << temp;
	}
	if(t == FileType::MainCpp)
	{
		x << Templates::Main_Cpp;
	}
	if(t == FileType::AssetHeader)
	{
		x << getNewLine() << "#pragma once";
		x << getNewLine() << "#include <JuceHeader.h>";

		if(auto al = data[mpid::Assets].getArray())
		{
			x << getNewLine() << "namespace " << className << "_Assets {";

			x << getNewLine() << "using namespace hise::multipage;";

			x << getNewLine() << "Asset::List createAssets();";
			x << getNewLine() << "} // namespace " << className << "_Assets";
			x << getNewLine();
		}
	}
	else if (t == FileType::AssetData && job != nullptr)
	{
		if(auto al = data[mpid::Assets].getArray())
		{
			x << getNewLine() << "#include \"Assets.h\"";
			x << getNewLine() << "namespace " << className << "_Assets {";
			x << getNewLine() << "using namespace hise::multipage;";

			{
				ScopedTabSetter t0(*this);

				for(const auto& a: *al)
				{
					job->setMessage("Embedding " + a[mpid::ID].toString() + "...");
					auto asset = Asset::fromVar(a, rootDirectory);
					asset->writeCppLiteral(x, getNewLine(), job);
				}
			}

			x << getNewLine();
			x << getNewLine() << "Asset::List createAssets()";
			x << getNewLine() << "{";
			{
				ScopedTabSetter t0(*this);
				x << getNewLine() << "Asset::List list;";

				for(auto& a: *al)
				{
					
					x << getNewLine() << "MULTIPAGE_ADD_ASSET_TO_LIST(" << a[mpid::ID].toString() << ");";
				}

				x << getNewLine() << "return list;";
			}
			x << getNewLine() << "}";

			x << getNewLine() << "} // namespace " << className << "_Assets";
			x << getNewLine();
		}
	}
	else if (t == FileType::DialogHeader)
	{
        if(!rawMode)
        {
            x << getNewLine() << "#pragma once";
            x << getNewLine() << "#include <JuceHeader.h>";
        }

		x << getNewLine() << "namespace hise {";
		x << getNewLine() << "namespace multipage {";
        
        if(rawMode)
            x << getNewLine() << "namespace library {";
        
		x << getNewLine() << "using namespace juce;";

		x << getNewLine() << "struct " << className << ": public HardcodedDialogWithState";
        
        if(rawMode)
            x << "," << getNewLine() << "                                     public hise::QuasiModalComponent";
        
		x << getNewLine() << "{";
		{
			ScopedTabSetter to(*this);

            if(rawMode)
            {
                Helpers::callRecursive(data, [&](const var& f)
                {
                    auto id = f[mpid::ID].toString();
                    
                    lambdaLocalIds.addIfNotAlreadyThere(id);
                    
                    auto type = f[mpid::Type].toString();
                    
                    if(type == "LambdaTask")
                        lambdaIds.add(f[mpid::Function].toString());
                });
                
                for(auto& lt: lambdaIds)
                    x << getNewLine() << "var " << lt << "(State::Job& t, const var& state);";
            }
            

            
            x << getNewLine() << className << "()";
            x << getNewLine() << "{";
            
            {
                ScopedTabSetter st(*this);
                
                if(rawMode)
                    x << getNewLine() << "closeFunction = BIND_MEMBER_FUNCTION_0(" << className << "::destroy);";
                      

                
                x << getNewLine() << "setSize(";
                
                x << String((int)data[mpid::LayoutData]["DialogWidth"]) << ", ";
                x << String((int)data[mpid::LayoutData]["DialogHeight"]) << ");";

                
            }
            
            x << getNewLine() << "}";
            
            x << getNewLine();
            x << getNewLine() << "Dialog* createDialog(State& state) override;";
            x << getNewLine();
		}
		
		x << getNewLine() << "};";
        
        if(rawMode)
            x << getNewLine() << "} // namespace library";
        
		x << getNewLine() << "} // namespace multipage";
		x << getNewLine() << "} // namespace hise";

        if(!rawMode)
        {
            x << getNewLine();
            x << getNewLine() << "using DialogClass = hise::multipage::" << className << ";";
        }
	}
	else if (t == FileType::DialogImplementation)
	{
        if(!rawMode)
        {
            x << getNewLine() << "#include \"Dialog.h\"";
            x << getNewLine() << "#include \"Assets.h\"";
        }

		x << getNewLine() << "namespace hise {";
		x << getNewLine() << "namespace multipage {";
        
        if(rawMode)
            x << getNewLine() << "namespace library {";
        
		x << getNewLine() << "using namespace juce;";

        if(rawMode)
        {
            for(const auto& lt: lambdaIds)
            {
                x << getNewLine() << "var " << className << "::" << lt << "(State::Job& t, const var& state)";
                x << getNewLine() << "{";
                
                {
                    ScopedTabSetter to(*this);
                    
                    x << getNewLine() << "// All variables: ";
                    
                    for(const auto& id: lambdaLocalIds)
                    {
                        if(id.isEmpty() || id == lt)
                            continue;
                        
                        x << getNewLine() << "auto " << id << " = state[" << id.quoted() << "];";
                    }
                    
                    x << getNewLine();
                    x << getNewLine() << "// ADD CODE here...";
                    x << getNewLine();
                    
                    x << getNewLine() << "return var(\"\"); // return a error";
                }
                
                x << getNewLine() << "}";
            }

        }
        
		x << getNewLine() << "Dialog* " << className << "::createDialog(State& state)";
		x << getNewLine() << "{";

		{
			ScopedTabSetter t0(*this);

            if(!rawMode)
                x << getNewLine() << "state.assets = " << className << "_Assets::createAssets();";

			x << getNewLine() << "DynamicObject::Ptr fullData = new DynamicObject();";

            if(!rawMode)
                x << getNewLine() << "fullData->setProperty(mpid::StyleData, JSON::parse(R\"(" << JSON::toString(data[mpid::StyleData], true) << ")\"));";
            
			x << getNewLine() << "fullData->setProperty(mpid::LayoutData, JSON::parse(R\"(" << JSON::toString(data[mpid::LayoutData], true) << ")\"));";
            
			x << getNewLine() << "fullData->setProperty(mpid::Properties, JSON::parse(R\"(" << JSON::toString(data[mpid::Properties], true) << ")\"));";

			x << getNewLine() << "using namespace factory;";
			x << getNewLine() << "auto mp_ = new Dialog(var(fullData.get()), state, false);";
			x << getNewLine() << "auto& mp = *mp_;";
			
			if(data[mpid::Children].isArray())
			{
				for(const auto& page: *data[mpid::Children].getArray())
				{
					x << createAddChild("mp", page, "Page", true);
				}
			}

			x << getNewLine() << "return mp_;";
		}

		x << getNewLine() << "}";

        if(rawMode)
            x << getNewLine() << "} // namespace library";
        
		x << getNewLine() << "} // namespace multipage";
		x << getNewLine() << "} // namespace hise";
	}
}

String CodeGenerator::generateRandomId(const String& prefix) const
{
	String x = prefix;

	if(x.isEmpty())
		x = "element";
	
	x << "_";
	x << String(idCounter++);
	
	return x;
}

String CodeGenerator::getNewLine() const
{
	String x = "\n";

	for(int i = 0; i < numTabs; i++)
		x << "\t";

	return x;
}

String CodeGenerator::arrayToCommaString(const var& value)
{
	String s;

	for(int i = 0; i < value.size(); i++)
	{
		s << value[i].toString();

		if(i < (value.size() - 1))
			s << ", ";
	}

	return s.quoted();
}

String CodeGenerator::createAddChild(const String& parentId, const var& childData, const String& itemType, bool attachCustomFunction) const
{
	auto id = childData[mpid::ID].toString();
	
	if(id.isEmpty())
		id = childData[mpid::Type].toString();

	id = generateRandomId(id);

	while(existingVariables.contains(id))
		id = generateRandomId(id);

	existingVariables.add(id);

	String x = getNewLine();

	auto typeId = childData[mpid::Type].toString();

    auto canBeAnonymous = childData[mpid::Children].size() == 0 &&
                          typeId != "LambdaTask";
    
    if(!canBeAnonymous)
        x << "auto& " << id << " = ";

    const auto& prop = childData.getDynamicObject()->getProperties();

    if(typeId == "Placeholder")
        typeId << "<" << prop[mpid::ContentType].toString() << ">";

    x << parentId << ".add" << itemType << "<" << typeId << ">({";
    
	String cp;

    NamedValueSet defaultValues;
    defaultValues.set(mpid::Trigger, false);
    defaultValues.set(mpid::Help, "");
    defaultValues.set(mpid::Class, "");
    defaultValues.set(mpid::Style, "");
    defaultValues.set(mpid::Text, "LabelText");
    defaultValues.set(mpid::UseInitValue, false);
    defaultValues.set(mpid::InitValue, "");
    defaultValues.set(mpid::Required, false);
    defaultValues.set(mpid::UseOnValue, false);
    defaultValues.set(mpid::Enabled, true);
    defaultValues.set(mpid::Foldable, false);
    defaultValues.set(mpid::Folded, false);
    defaultValues.set(mpid::UseChildState, false);
    defaultValues.set(mpid::EmptyText, "");
    defaultValues.set(mpid::ParseArray, false);
    defaultValues.set(mpid::Multiline, false);
    defaultValues.set(mpid::EventTrigger, "OnPageLoad");
    
    static const Array<Identifier> deprecatedIds = { Identifier("Padding"),
        Identifier("LabelPosition"),
        Identifier("UseFilter"),
        Identifier("Visible"),
        Identifier("Comment"),
        Identifier("ManualAction"),
        Identifier("CallOnNext")};
    
	for(auto& nv: prop)
	{
		if(nv.name == mpid::Type || nv.name == mpid::ContentType || nv.name == mpid::Children)
			continue;

        if(deprecatedIds.contains(nv.name))
            continue;
        
		if(nv.value.toString().isEmpty())
			continue;

        if(nv.name == mpid::Code)
        {
            if(prop.contains(mpid::UseOnValue) && !prop[mpid::UseOnValue])
                continue;

            
        }
        
        if(defaultValues.contains(nv.name) && nv.value == defaultValues[nv.name])
            continue;
        
		cp << getNewLine() << "  { mpid::" << nv.name << ", ";

		if(nv.value.isString())
		{
            auto asString = nv.value.toString();

            if(asString.containsAnyOf("\n\"\\"))
            {
	            cp << "R\"(" << asString;
				cp << ")\"";
            }
            else
            {
	            cp << asString.quoted();
            }
		}
		else if (nv.value.isArray())
		{
			cp << arrayToCommaString(nv.value);
		}
		else
			cp << (int)(nv.value);

		cp << " }";

		cp << ", ";
	}

	x << cp.upToLastOccurrenceOf(", ", false, false);

	x << getNewLine() << "});\n";

	if(typeId == "LambdaTask")
	{
		auto bindFunctionName = childData[mpid::Function].toString();

		if(bindFunctionName.isEmpty())
			x << getNewLine() << "// lambda task for " << id;
		else
			x << getNewLine() << "// TODO: add var " << bindFunctionName << "(State::Job& t, const var& stateObject) to class";

		x << getNewLine() << id << ".setLambdaAction(state, ";
		
		if(bindFunctionName.isNotEmpty())
		{
			x << "BIND_MEMBER_FUNCTION_2(" << className << "::" << bindFunctionName << ")";
		}
		else
		{
			x << "[](State::Job& t, const var& stateObject)";
            x << getNewLine() << "{";
            x << getNewLine() << "\treturn var();" << getNewLine() << "}";
		}
		
		x << ");" << getNewLine();
	}

	auto children = childData[mpid::Children];

	if(children.isArray())
	{
		for(const auto& v: *children.getArray())
		{
			x << createAddChild(id, v, "Child", false);
		}
	}

	if(attachCustomFunction)
	{
		x << getNewLine() << "// Custom callback for page " << id;
		x << getNewLine() << id << ".setCustomCheckFunction([](Dialog::PageBase* b, const var& obj)";
		x << "{\n" << getNewLine() << "\treturn Result::ok();\n" << getNewLine() << "});" << getNewLine();
	}

	return x;
}


} // multipage
} // hise
