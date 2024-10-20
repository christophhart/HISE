
REM This is the local folder where the DMG images are created.
SET nightly_build_folder="D:\Development\Installer\Windows\nightly_builds"

REM This is the project folder for the Standalone app
SET standalone_project="projects\standalone\Builds\VisualStudio2022\HISE Standalone.sln"

SET projucerPath="tools\projucer\Projucer.exe"

SET standalone_projucer_project="projects\standalone\HISE Standalone.jucer"

SET plugin_projucer_project="projects\plugin\HISE.jucer"

SET multipagecreator_projucer_project="tools\multipagecreator\multipagecreator.jucer"

SET hise_ci="projects\standalone\Builds\VisualStudio2022\x64\CI\App\HISE.exe"

SET multipagecreator_project="tools\multipagecreator\Builds\VisualStudio2022\multipagecreator.sln"

set multipage_binary="tools\multipagecreator\Builds\VisualStudio2022\x64\Release\App\multipagecreator.exe"

REM This is the project folder of the plugin project
SET plugin_project="projects\plugin\Builds\VisualStudio2022\HISE.sln"

REM This is the path to the ISS Installer compiler
SET installer_command="C:\Program Files (x86)\Inno Setup 6\ISCC.exe"