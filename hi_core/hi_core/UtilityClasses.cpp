/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software : you can redistribute it and / or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   HISE is distributed in the hope that it will be useful,
*but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with HISE.If not, see < http://www.gnu.org/licenses/>.
*
*   Commercial licences for using HISE in an closed source project are
*   available on request.Please visit the project's website to get more
*   information about commercial licencing :
*
*   http ://www.hartinstruments.net/hise/
*
*   HISE is based on the JUCE library,
*which also must be licenced for commercial applications :
*
*   http ://www.juce.com
*
* == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == =
*/

#if  JUCE_MAC

struct FileLimitInitialiser
{
	FileLimitInitialiser()
	{
		rlimit lim;

		getrlimit(RLIMIT_NOFILE, &lim);
		lim.rlim_cur = lim.rlim_max = 200000;
		setrlimit(RLIMIT_NOFILE, &lim);
	}
};



static FileLimitInitialiser fileLimitInitialiser;
#endif


double ScopedGlitchDetector::maxMilliSeconds = 3.0;
Identifier ScopedGlitchDetector::lastPositiveId = Identifier();


void AutoSaver::timerCallback()
{
	Processor *mainSynthChain = mc->getMainSynthChain();

	File backupFile = getAutoSaveFile();

	ValueTree v = mainSynthChain->exportAsValueTree();

	v.setProperty("BuildVersion", BUILD_SUB_VERSION, nullptr);
	FileOutputStream fos(backupFile);
	v.writeToStream(fos);

	debugToConsole(mainSynthChain, "Autosaving as " + backupFile.getFileName());
}

File AutoSaver::getAutoSaveFile()
{
	Processor *mainSynthChain = mc->getMainSynthChain();

	File presetDirectory = GET_PROJECT_HANDLER(mainSynthChain).getSubDirectory(ProjectHandler::SubDirectories::Presets);

	if (presetDirectory.isDirectory())
	{
		if (fileList.size() == 0)
		{
			fileList.add(presetDirectory.getChildFile("Autosave_1.hip"));
			fileList.add(presetDirectory.getChildFile("Autosave_2.hip"));
			fileList.add(presetDirectory.getChildFile("Autosave_3.hip"));
			fileList.add(presetDirectory.getChildFile("Autosave_4.hip"));
			fileList.add(presetDirectory.getChildFile("Autosave_5.hip"));
		}

		File toReturn = fileList[currentAutoSaveIndex];

		if (toReturn.existsAsFile()) toReturn.deleteFile();

		currentAutoSaveIndex = (currentAutoSaveIndex + 1) % 5;

		return toReturn;
	}
	else
	{
		return File::nonexistent;
	}
}
