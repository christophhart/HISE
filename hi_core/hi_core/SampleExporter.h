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
*   Commercial licences for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licencing:
*
*   http://www.hartinstruments.net/hise/
*
*   HISE is based on the JUCE library,
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

#ifndef SAMPLEEXPORTER_H_INCLUDED
#define SAMPLEEXPORTER_H_INCLUDED


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

class ExternalResourceCollector : public ThreadWithAsyncProgressWindow
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

/** A SampleMapExporter writes all samples that are referenced by a SampleMap into one big audio file. */
class SampleMapExporter
{
public:

	/** Creates a SampleMapExporter.
	*
	*	@param sampleMapPaths a list of all file paths of all samplemaps. This can be obtained by SampleMapSearcher::getFileNamesOfAllSampleMaps()
	*/
	SampleMapExporter(const StringArray &sampleMapPaths_):
		sampleMapPaths(StringArray(sampleMapPaths_)),
		exportToGlobalFolder(false)
	{};

	SampleMapExporter(const StringArray fileNames_, bool exportToGlobalFolder_, PresetPlayerHandler::FolderType type_) :
        exportToGlobalFolder(exportToGlobalFolder_),
		fileNames(fileNames_),
        folderType(type_)
	{};


	SampleMapExporter(ValueTree &sampleMapTree) :
		exportToGlobalFolder(false),
        sampleMaps(sampleMapTree)
	{};

	/** This exports all samplemaps. 
	*
	*	It writes one data file with an ValueTree containing all samplemaps
	*	and one wave data collection file per sample map.
	*
	*	@param path the directory path where the preset file (and the samplemap collection file) is stored. This is not the target directory for the samples,
	*		        it will use PresetHandler::getDataFolder() to determine the location of the sample folder.
	*	@param uniqueProductIdentifier the name of the product. Don't use whitespace or weird characters.
	*	@param useBackgroundThread if true, then the writing of the samples will be done in a background thread (with a progressbar popup).
	*/
	bool exportSamples(const String &directoryPath, const String &uniqueProductIdentifier, bool useBackgroundThread=false);

private:

	bool useOldMode() const { return sampleMapPaths.size() != 0; };

	void writeWaveFile(ValueTree &sampleMapData, const String &directoryPath, const String &uniqueProductIdentifier, const String &name);

	void writeSampleIntoMonolith(ValueTree &sampleMapData, int sampleIndex, String *message = nullptr);

	void updateSampleMap(ValueTree &sampleMapData, const String &uniqueProductIdentifier, const String &name);	

	void setupFilesAndFolders(const ValueTree &sampleMapData, const String &targetDirectory, const String &uniqueProductIdentifier, const String &name);

	void setupInputOutput(const ValueTree &sampleMapData);

	class SampleWriter: public ThreadWithProgressWindow
	{
	public:

		SampleWriter(SampleMapExporter *exporter_, ValueTree &sampleMap, const String &directoryPath_, const String &productId_, const String &sampleMapId_): 
			ThreadWithProgressWindow("Writing sample files for " + sampleMapId_ + " into monolith", true, true),
			exporter(exporter_),
			v(sampleMap),
			exportToGlobalFolder(false),
            productId(productId_),
            directoryPath(directoryPath_),
            sampleMapId(sampleMapId_)
		{
			getAlertWindow()->setLookAndFeel(&laf);
		};

		SampleWriter(SampleMapExporter *exporter_, const StringArray &fileNames_, const String &packageName_, PresetPlayerHandler::FolderType type_, ValueTree &invalidTree) :
			ThreadWithProgressWindow("Copying samples to global folder", true, true),
   			fileNames(fileNames_),
			exporter(exporter_),
			exportToGlobalFolder(true),
			subFolderName(packageName_),
			folderType(type_),
			v(invalidTree)
		{
			getAlertWindow()->setLookAndFeel(&laf);
		}

		void run();

	private:

		AlertWindowLookAndFeel laf;

		StringArray fileNames;
        
        ValueTree &v;
        
        SampleMapExporter *exporter;
		bool exportToGlobalFolder;

		String subFolderName;

		PresetPlayerHandler::FolderType folderType;

		const String productId;
		const String directoryPath;
		const String sampleMapId;

	};

	ScopedPointer<MemoryMappedAudioFormatReader> reader;
	ScopedPointer<AudioFormatWriter> writer;

	struct WriteData
	{
		String sampleFolder;
		File monolithData;
		String samplePath;

		int64 pos;
		int sampleRate;
		int bitsPerSample;
		int channels;

		StringArray fileNames;

	};

	

	StringArray sampleMapPaths;

	StringArray fileNames;

	bool exportToGlobalFolder;

	PresetPlayerHandler::FolderType folderType;

	ValueTree sampleMaps;

	WriteData writeData;

	bool createSampleMapValueTree();

	void writeDataFile(const String &directoryPath, const String uniqueProductIdentifier);

	ValueTree createValueTree(const String &sampleMapFile);
};


#endif  // SAMPLEEXPORTER_H_INCLUDED
