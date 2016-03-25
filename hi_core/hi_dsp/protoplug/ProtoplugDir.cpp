#include "ProtoplugDir.h"

ProtoplugDir* ProtoplugDir::pInstance = 0; 

// private contructor called once
ProtoplugDir::ProtoplugDir()
{
	found = true;
	#if JUCE_WINDOWS
		dir = File::getSpecialLocation(File::currentExecutableFile).getSiblingFile("ProtoplugFiles");
	#endif
	#if JUCE_MAC
		dir = File::getSpecialLocation(File::currentApplicationFile).getSiblingFile("ProtoplugFiles");
		//if (ret.getFullPathName().endsWith("/Contents/MacOS")) // assume OSX bundle format
		//pluginLocation = pluginLocation.getSiblingFile("../../../");
	#endif
	#if JUCE_LINUX
		dir = File("/usr/share/ProtoplugFiles");
	#endif
	if (dir.exists())
		return;

	// compatibility with old version
	dir = dir.getSiblingFile("protoplug");
	if (dir.exists())
		return;

	dir = File::getSpecialLocation(File::userHomeDirectory).getSiblingFile("ProtoplugFiles");
	if (dir.exists())
		return;

	File appDataDir = File::getSpecialLocation(File::userApplicationDataDirectory).getChildFile("Protoplug");
	if (!appDataDir.exists())
		appDataDir.createDirectory();
	dirTextFile = appDataDir.getChildFile("ProtoplugFiles.txt");
	String protoPath = dirTextFile.loadFileAsString();
	if (protoPath.isNotEmpty() && File::isAbsolutePath(protoPath))
		dir = File(protoPath);
	if (!dir.exists())
		// ProtoplugFiles not found
		found = false;
}

bool ProtoplugDir::checkDir(File _dir, String &missing)
{
	bool valid = true;
	StringArray requiredFiles;
	requiredFiles.add("effects");
	requiredFiles.add("generators");
	requiredFiles.add("include");
	requiredFiles.add("effects/default.lua");
	requiredFiles.add("generators/default.lua");
	for (int i = 0; i < requiredFiles.size(); ++i)
	{
		if (!_dir.getChildFile(requiredFiles[i]).exists()) {
			valid = false;
			missing = requiredFiles[i];
			break;
		}
	}
	return valid;
}

void ProtoplugDir::setDir(File _dir)
{
	found = true;
	dir = _dir;
}

File ProtoplugDir::getDir()
{
	return dir;
}

File ProtoplugDir::getDirTextFile()
{
	return dirTextFile;
}

File ProtoplugDir::getScriptsDir()
{
	return getDir().getChildFile(SCRIPTS_DIR);
}

File ProtoplugDir::getLibDir()
{
	return getDir().getChildFile("lib");
}

ProtoplugDir* ProtoplugDir::Instance()
{
	if (!pInstance)
		pInstance = new ProtoplugDir;

	return pInstance;
}
