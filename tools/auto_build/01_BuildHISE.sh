cd "$(dirname "$0")"
cd ..
cd ..

rm -rf "$plugin_folder/Builds/MacOSX/build/"

# This is the project folder for the Standalone app
standalone_folder="projects/standalone"

# This is the project folder of the plugin project
plugin_folder="projects/plugin"

echo "Compiling VST / AU Plugins..."

echo "Compiling stereo version..."

xcodebuild -project "$plugin_folder/Builds/MacOSX/HISE.xcodeproj" -configuration "Release" clean

xcodebuild -project "$plugin_folder/Builds/MacOSX/HISE.xcodeproj" -configuration "Release" | xcpretty

if [ $? != "0" ];
then
	echo "========================================================================"
	echo "Error at compiling. Aborting..."
    exit
fi

auval -v aumu Hise Hain

if [ $? != "0" ];
then
	echo "========================================================================"
	echo "Error at validating AU. Aborting..."
    exit
fi


echo "OK"

echo "Compiling Standalone App..."

xcodebuild -project "$standalone_folder/Builds/MacOSX/HISE Standalone.xcodeproj" -configuration "Release" clean

xcodebuild -project "$standalone_folder/Builds/MacOSX/HISE Standalone.xcodeproj" -configuration "Release" | xcpretty

if [ $? != "0" ];
then
	echo "========================================================================"
	echo "Error at compiling. Aborting..."
    exit
fi

echo "OK"

