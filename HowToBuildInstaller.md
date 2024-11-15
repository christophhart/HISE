1. Run set_version.bat OLD_VERSION NEW_VERSION, commit the version number updates & git hash
2. Add the tag to the development branch
3. Merge this into master
4. Run the DMG builder on the macOS build server
5. Upload the HISE Windows binary (unsigned)