cd "$(dirname "$0")"
cd ..
cd ..


# This is the project folder for the Standalone app
standalone_folder="projects/standalone"

chmod +x "tools/Projucer/Projucer.app/Contents/MacOS/Projucer"

"tools/Projucer/Projucer.app/Contents/MacOS/Projucer" --resave "projects/standalone/HISE Standalone.jucer"

echo "Compiling Standalone App..."

xcodebuild -project "$standalone_folder/Builds/MacOSX/HISE Standalone.xcodeproj" -configuration "CI" | xcpretty || exit 1

if [ $? != 0 ];
then
	echo "========================================================================"
	echo "Error at compiling. Aborting..."
    exit 1
fi

echo "OK"
echo "Running unit tests..."

hise_path="projects/standalone/Builds/MacOSX/build/CI/HISE.app/Contents/MacOS/HISE"

$hise_path run_unit_tests

if [ $? != 0 ];
then
	echo "========================================================================"
	echo "Error at unit testing. Aborting..."
    exit 1
fi

echo "OK"

echo "Exporting demo project..."

project_folder="$PWD"/extras/demo_project

echo $project_folder

$hise_path set_project_folder -p:"$project_folder"

$hise_path export_ci "XmlPresetBackups/Demo.xml" -t:standalone -a:x64

"$project_folder/Binaries/batchCompileOSX"

if [ $? != 0 ];
then
	echo "========================================================================"
	echo "Error at project export. Aborting..."
    exit 1
fi

echo "OK"
