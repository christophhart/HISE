#!/bin/bash
set -euo pipefail

script_dir="$(cd "$(dirname "$0")" && pwd)"
repo_root="$(cd "$script_dir/../.." && pwd)"
cd "$repo_root"

bash tools/auto_build/check_macos_signing.sh

if [[ ! -f "tools/SDK/sdk.zip" ]]; then
	echo "tools/SDK/sdk.zip not found."
	exit 1
fi

(cd tools/SDK && tar -xf sdk.zip)

projucer="JUCE/projucer/Projucer.app/Contents/MacOS/Projucer"
standalone_project="projects/standalone/HISE Standalone.jucer"
standalone_xcodeproj="projects/standalone/Builds/MacOSX/HISE Standalone.xcodeproj"
standalone_app="projects/standalone/Builds/MacOSX/build/Release/HISE.app"

chmod +x "$projucer"
"$projucer" --resave "$standalone_project"

set -o pipefail
xcodebuild -project "$standalone_xcodeproj" -configuration "Release" | ./tools/projucer/xcbeautify --renderer github-actions

if [[ ! -d "$standalone_app" ]]; then
	echo "HISE Standalone app not found after build: $standalone_app"
	exit 1
fi

codesign --deep --force --options runtime --timestamp \
	--sign "$APPLE_APPLICATION_SIGNING_IDENTITY" "$standalone_app"
codesign --verify --deep --strict --verbose=2 "$standalone_app"

echo "Signed HISE Standalone app: $standalone_app"
