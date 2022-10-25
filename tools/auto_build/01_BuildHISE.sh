cd "$(dirname "$0")"
cd ..
cd ..


cd tools/SDK

tar -xf sdk.zip

cd ..
cd ..

# This is the project folder for the Standalone app
standalone_folder="projects/standalone"

# This is the project folder of the plugin project
plugin_folder="projects/plugin"

chmod +x "tools/Projucer/Projucer.app/Contents/MacOS/Projucer"

"tools/Projucer/Projucer.app/Contents/MacOS/Projucer" --resave "projects/plugin/HISE.jucer"
"tools/Projucer/Projucer.app/Contents/MacOS/Projucer" --resave "projects/standalone/HISE Standalone.jucer"

echo "Compiling VST / AU Plugins..."

echo "Compiling stereo version..."

# Skip this for now
xcodebuild -project "$plugin_folder/Builds/MacOSX/HISE.xcodeproj" -configuration "Release" | xcpretty

if [ $? != "0" ];
then
	echo "========================================================================"
	echo "Error at compiling. Aborting..."
    exit
fi

if [ $? != "0" ];
then
	echo "========================================================================"
	echo "Error at validating AU. Aborting..."
    exit
fi

echo "OK"

echo "Compiling Standalone App..."

xcodebuild -project "$standalone_folder/Builds/MacOSX/HISE Standalone.xcodeproj" -configuration "Release" | xcpretty

if [ $? != "0" ];
then
	echo "========================================================================"
	echo "Error at compiling. Aborting..."
    exit
fi

echo "OK"

