cd "$(dirname "$0")"

#echo User: $USER
#echo Fetching Apple Code Signing ID
#echo If this fails, you need to add your certificate as $USER keychain password with the ID dev_certificate_id
#APPLE_CERTIFICATE_ID=$(security find-generic-password -a "$USER" -s "dev_certificate_id" -w)
#echo The certificate was found: $APPLE_CERTIFICATE_ID
#echo Fetching Apple Installer Code Signing ID
#echo If this fails, you need to add your installer certificate as $USER keychain password with the ID dev_installer_id
#APPLE_CERTIFICATE_ID_INSTALLER=$(security find-generic-password -a "$USER" -s "dev_installer_id" -w)
#echo The installer certificate was found: $APPLE_CERTIFICATE_ID_INSTALLER

tag_version=$(git describe --abbrev=0)
tag_underscore=$(echo "$tag_version" | tr . _)

echo $tag_version
echo $tag_underscore

installer_project="hise_nightly_build.pkgproj"

sed -i '' "s/TAG_VERSION/$tag_version/" $installer_project

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

plugin_path=./../../projects/plugin/Builds/MacOSX/build/Release

plugin_au="$plugin_path/HISE.component/"
plugin_vst="$plugin_path/HISE.vst/"
standalone_app=./../../projects/standalone/Builds/MacOSX/build/Release/HISE.app

abort_if_not_exist $plugin_au
abort_if_not_exist $plugin_vst
abort_if_not_exist $standalone_app

# Building Installer

echo "Building Installer..."

/usr/local/bin/packagesbuild --verbose "$installer_project" PROJECT_VERSION=$tag_version

installer_name=./Output/HISE\ $tag_version.pkg

cp ./build/HISE.pkg "$installer_name"

echo "OK."


