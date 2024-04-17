cd "$(dirname "$0")"

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
plugin_vst="$plugin_path/HISE.vst3/"
standalone_app=./../../projects/standalone/Builds/MacOSX/build/Release/HISE.app

abort_if_not_exist $plugin_au
abort_if_not_exist $plugin_vst
abort_if_not_exist $standalone_app

# Building Installer

echo "Building Installer..."

/usr/local/bin/packagesbuild --verbose "$installer_project" PROJECT_VERSION=$tag_version

installer_name="./Output/HISE $tag_version_underscore.pkg"

echo "Write to $installer_name"

cd "$(dirname "$0")"
mkdir Output
cp ./build/HISE.pkg "$installer_name"

echo "OK."