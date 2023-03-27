#!/bin/bash
cd -- "$(dirname "$BASH_SOURCE")"

echo *==========================*
echo HISE PROJECT VERSION UPDATER
echo *==========================*

echo Current tag version:

git describe --abbrev=0

echo Standalone project version:

./tools/projucer/Projucer.app/Contents/MacOS/Projucer --get-version ./projects/standalone/HISE\ Standalone.jucer

echo Please enter the new semantic version number

read versioninput

./tools/projucer/Projucer.app/Contents/MacOS/Projucer --set-version $versioninput ./projects/plugin/HISE.jucer
./tools/projucer/Projucer.app/Contents/MacOS/Projucer --set-version $versioninput ./projects/standalone/HISE\ Standalone.jucer

echo New version written into project files:

./tools/projucer/Projucer.app/Contents/MacOS/Projucer --get-version ./projects/standalone/HISE\ Standalone.jucer
