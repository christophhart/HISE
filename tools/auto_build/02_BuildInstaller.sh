#!/bin/bash
set -euo pipefail

script_dir="$(cd "$(dirname "$0")" && pwd)"
repo_root="$(cd "$script_dir/../.." && pwd)"
cd "$repo_root"

tag_version="$(git describe --abbrev=0)"
package_version="${tag_version#v}"
tag_underscore="${tag_version//./_}"
standalone_app="$repo_root/projects/standalone/Builds/MacOSX/build/Release/HISE.app"
output_dir="$script_dir/Output"
package_id="com.hise.HISE"
package_root="$script_dir/package-root"

if [ ! -d "$standalone_app" ]; then
	echo "HISE Standalone app not found: $standalone_app"
	echo "Build it with 01_BuildHISE.sh first."
	exit 1
fi

rm -rf "$package_root"
mkdir -p "$package_root/Applications" "$output_dir"
cp -R "$standalone_app" "$package_root/Applications/HISE.app"

pkgbuild \
	--root "$package_root" \
	--install-location "/" \
	--identifier "$package_id" \
	--version "$package_version" \
	"$output_dir/HISE_${tag_underscore}_macOS.pkg"

rm -rf "$package_root"
echo "Installer created: $output_dir/HISE_${tag_underscore}_macOS.pkg"
