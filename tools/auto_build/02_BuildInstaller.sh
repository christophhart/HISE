
installer_project="hise_nightly_build.pkgproj"

echo "Enter version number with underscores"
read version


# Building Installer

echo "Building Installer..."

cd "$(dirname "$0")"

/usr/local/bin/packagesbuild $installer_project

mv build/HISE.pkg Output/HISE_$version.pkg