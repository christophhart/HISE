/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   HISE is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licenses for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licensing:
*
*   http://www.hise.audio/
*
*   HISE is based on the JUCE library,
*   which must be separately licensed for closed source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

#if 0

namespace hise { using namespace juce;

class ReferenceSearcher
{
public:

	/** Iterates the file and stores all samplemap file paths into an array. */
	void search() {	if(ok) searchInternal(preset); }

	/** strips the ValueTree (replaces all filenames with the ID) and returns a copy.
	*	It also saves a XML file at C:\\export\preset.xml for debugging purpose.
	*/
	ValueTree getStrippedPreset();

	/** returns the list of all file names. */
	const StringArray &getFileNames() const { return fileNames; };

	static void writeValueTreeToFile(const ValueTree &valueTree, File &presetFile)
	{
		presetFile.deleteFile();
		presetFile.create();

		FileOutputStream fos(presetFile);

		valueTree.writeToStream(fos);
	}

protected:

	
	/** Creates a SampleMapSearcher that searches the given File.
	*
	*	@param presetFile a File containing a ValueTree that holds the preset data. */
	ReferenceSearcher(const File &presetFile);

	/** Creates a SampleMapSearcher that searches the preset. 
	*
	*	It creates a copy of the preset, so you don't have to bother about any modifications to the original ValueTree. */
	ReferenceSearcher(const ValueTree &preset);
    
    virtual ~ReferenceSearcher(){};

	/** checks if the ValueTree corresponds to a valid preset file (a SynthChain). */
	bool checkValidPreset();

	/** iterates the value tree and either saves the filenames into a list or strips the path from the filenames. */
	virtual void searchInternal(ValueTree &treeToSearch, bool stripPath=false) = 0;

	StringArray fileNames;

private:

	bool ok;
	
	ValueTree preset;


};

class ExternalResourceCollector : public DialogWindowWithBackgroundThread
{
public:

	ExternalResourceCollector(MainController *mc_);

public:

	void run() override;

	void threadFinished() override;

private:

	int numTotalSamples;
	int numTotalAudioFiles;
	int numTotalImages;

	int numSamplesCopied;
	int numImagesCopied;
	int numAudioFilesCopied;

	MainController *mc;
};


/** A SampleMapSearcher searches a preset files for samplemaps and stores them in a list for the SampleExporter. */
class SampleMapSearcher: public ReferenceSearcher
{
public:

	/** Creates a SampleMapSearcher that searches the given File.
	*
	*	@param presetFile a File containing a ValueTree that holds the preset data. */
	SampleMapSearcher(const File &presetFile):
		ReferenceSearcher(presetFile)
	{};

	/** Creates a SampleMapSearcher that searches the preset. 
	*
	*	It creates a copy of the preset, so you don't have to bother about any modifications to the original ValueTree. */
	SampleMapSearcher(const ValueTree &preset):
		ReferenceSearcher(preset)
	{};

private:

	/** iterates the value tree and either saves the filenames into a list or strips the path from the filenames. */
	void searchInternal(ValueTree &treeToSearch, bool stripPath=false) final override;

};

class ModulatorSynthChain;

} // namespace hise

#endif  // SAMPLEEXPORTER_H_INCLUDED
