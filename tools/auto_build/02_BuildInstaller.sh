cd "$(dirname "$0")"

installer_project="hise_nightly_build.pkgproj"

function abort_if_not_exist {
	echo 
if [ -e "$1" ]
then
    echo "$1 was found./"
else
    echo "$1 was not found"
    exit 1
fi
}

plugin_au=/Library/Audio/Plug-Ins/Components/HISE.component/
plugin_vst=/Library/Audio/Plug-Ins/VST/HISE.vst/
standalone_app=./../../projects/standalone/Builds/MacOSX/build/Release/HISE.app

abort_if_not_exist $plugin_au
abort_if_not_exist $plugin_vst
abort_if_not_exist $standalone_app

echo "Enter Apple-ID for codesigning:"
read apple_id

echo "Codesigning AU plugin..."
codesign -s "$apple_id" $plugin_au

if [ $? != "0" ];
then
	echo "========================================================================"
	echo "Error at codesigning. Aborting..."
    #exit
fi
echo "OK."

echo "Codesigning VST plugin..."
codesign -s "$apple_id" $plugin_vst

if [ $? != "0" ];
then
	echo "========================================================================"
	echo "Error at codesigning. Aborting..."
    #exit
fi
echo "OK."

echo "Codesigning Standalone App..."
codesign -s "$apple_id" $standalone_app

if [ $? != "0" ];
then
	echo "========================================================================"
	echo "Error at codesiging. Aborting..."
    #exit
fi
echo "OK."

echo "Enter version number with underscores"
read version

# Building Installer

echo "Building Installer..."

/usr/local/bin/packagesbuild $installer_project

mv build/HISE.pkg Output/HISE_$version.pkg

echo "Codesigning Installer..."
codesign -s "$apple_id" ./Output/HISE_$version.pkg
echo "OK."


