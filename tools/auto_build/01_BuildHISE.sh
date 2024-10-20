cd "$(dirname "$0")"
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

echo Write team development ID

echo Fetching Apple Code Signing ID
echo If this fails, you need to add your certificate as $USER keychain password with the ID dev_certificate_id
APPLE_CERTIFICATE_ID=$(security find-generic-password -a "$USER" -s "dev_certificate_id" -w)
echo The certificate was found: $APPLE_CERTIFICATE_ID
echo Fetching Apple Installer Code Signing ID
echo If this fails, you need to add your installer certificate as $USER keychain password with the ID dev_installer_id
APPLE_CERTIFICATE_ID_INSTALLER=$(security find-generic-password -a "$USER" -s "dev_installer_id" -w)
echo The installer certificate was found: $APPLE_CERTIFICATE_ID_INSTALLER

# regex the actual team ID
[[ $APPLE_CERTIFICATE_ID_INSTALLER =~ .*\((.*)\) ]] && team_id=${BASH_REMATCH[1]}

echo The team ID is: $team_id

echo Fetching App specific password for notarizing
echo If this fails, you need to create an app specific password in iCloud and add it as $USER keychain password with the ID notarize_app_password

notarize_app_password=$(security find-generic-password -a "$USER" -s "notarize_app_password" -w)

echo $notarize_app_password

sed -i -e "s/iosDevelopmentTeamID=\"\"/iosDevelopmentTeamID=\"$team_id\"/g" "projects/plugin/HISE.jucer"
sed -i -e "s/iosDevelopmentTeamID=\"\"/iosDevelopmentTeamID=\"$team_id\"/g" "projects/standalone/HISE Standalone.jucer"
sed -i -e "s/iosDevelopmentTeamID=\"\"/iosDevelopmentTeamID=\"$team_id\"/g" "tools/multipagecreator/multipagecreator.jucer"

"tools/Projucer/Projucer.app/Contents/MacOS/Projucer" --resave "projects/plugin/HISE.jucer"
"tools/Projucer/Projucer.app/Contents/MacOS/Projucer" --resave "projects/standalone/HISE Standalone.jucer"
"tools/Projucer/Projucer.app/Contents/MacOS/Projucer" --resave "tools/multipagecreator/multipagecreator.jucer"

echo "Compiling Multipage Creator App..."

xcodebuild -project "tools/multipagecreator/Builds/MacOSX/multipagecreator.xcodeproj" -configuration "Release" | xcpretty

if [ $? != "0" ];
then
	echo "========================================================================"
	echo "Error at compiling. Aborting..."
    exit
fi

echo "OK"

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


au_project=$PWD/$plugin_folder/Builds/MacOSX/build/Release/HISE.component
vst3_project=$PWD/$plugin_folder/Builds/MacOSX/build/Release/HISE.vst3
standalone_project=$PWD/$standalone_folder/Builds/MacOSX/build/Release/HISE.app
mp_binary="$PWD/tools/multipagecreator/Builds/MacOSX/build/Release/multipagecreator.app"
installer_binary="$PWD/tools/auto_build/installer/Hise Installer.app"

echo "Compiling HISE installer"



$mp_binary/Contents/MacOS/multipagecreator --export:"$PWD/tools/auto_build/installer/hise_installer.json" --hisepath:"$PWD" --teamid:$team_id

chmod +x tools/auto_build/Installer/Binaries/batchCompileOSX
tools/auto_build/Installer/Binaries/batchCompileOSX

echo "Code signing"

echo "Signing VST & AU"
codesign --deep --force --options runtime -s "$APPLE_CERTIFICATE_ID" "$au_project" --timestamp
codesign -dv --verbose=4 "$au_project"
codesign --deep --force --options runtime -s "$APPLE_CERTIFICATE_ID" "$vst3_project" --timestamp
codesign -dv --verbose=4 "$vst3_project"

echo "Signing Standalone App"
codesign --deep --force --options runtime -s "$APPLE_CERTIFICATE_ID" "$standalone_project" --timestamp
codesign -dv --verbose=4 "$standalone_project"
codesign --deep --force --options runtime -s "$APPLE_CERTIFICATE_ID" "$mp_binary" --timestamp
codesign -dv --verbose=4 "$mp_binary"

echo "Signing Installer"

codesign --deep --force --options runtime -s "$APPLE_CERTIFICATE_ID" "$installer_binary" --timestamp
codesign -dv --verbose=4 "$mp_binary"




