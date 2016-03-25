#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

#ifdef _PROTOGEN
#define SCRIPTS_DIR "generators"
#else
#define SCRIPTS_DIR "effects"
#endif

class ProtoplugDir
{
public:
	static ProtoplugDir* Instance();
	File getDir();
	void setDir(File _dir);
	bool checkDir(File dir, String &missing);

	File getScriptsDir();
	File getLibDir();
	File getDirTextFile();
	bool found;
	

private:
	File dir;
	File dirTextFile;

	// i still consider singletons an anti-pattern, but here you go...
	ProtoplugDir();
	ProtoplugDir(ProtoplugDir const&){};
	ProtoplugDir& operator=(ProtoplugDir const&){};
	static ProtoplugDir* pInstance;
};
