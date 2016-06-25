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

#ifndef MODULATORSAMPLERDATA_H_INCLUDED
#define MODULATORSAMPLERDATA_H_INCLUDED

class ModulatorSampler;
class ModulatorSamplerSound;

/** A background thread which loads sample data into the preload buffer of a StreamingSamplerSound
*	@ingroup sampler
*
*	Whenever you need to change the preloadSize, create an instance of this
*/
class SoundPreloadThread: public ThreadWithQuasiModalProgressWindow
{
public:

	/** Creates a preload thread which preloads all sounds from the supplied sampler. */
	SoundPreloadThread(ModulatorSampler *s);;

	/** Creates a preload thread which preloads all sounds from the pointer array. 
	*
	*	This can be used to only preload a selection (which is the case when changing sample starts).
	*/
	SoundPreloadThread(ModulatorSampler *s, Array<ModulatorSamplerSound*> soundsToPreload_);;

	/** preloads either all sounds from the sampler or the list of sounds that was passed in the constructor. */
	void run() override;

	void preloadSample(StreamingSamplerSound * s, const int preloadSize);

private:

	AlertWindowLookAndFeel laf;

	Array<ModulatorSamplerSound*> soundsToPreload;

	ModulatorSampler *sampler;
};

/** Handles all thumbnail related stuff
*	@ingroup sampler
*
*	Simply call loadThumbNails whenever you change the directory and it takes care of everything.
*/
class ThumbnailHandler: public ThreadWithQuasiModalProgressWindow
{
public:

	/** This loads the thumbnails into the sampler.
	*
	*	It looks for a previously saved file and creates a new file if nothing is found.
	*/
	static void loadThumbnails(ModulatorSampler *sampler, const File &directory)
	{
		if(ThumbnailHandler::getThumbnailFile(sampler).existsAsFile())
		{
			ThumbnailHandler::loadThumbnailsIntoSampler(sampler);
		}
		else
		{
			ThumbnailHandler::generateThumbnailData(sampler, directory);
		}
	}

	static void saveNewThumbNails(ModulatorSampler *sampler, const StringArray &newAudioFiles);

private:

	ThumbnailHandler(const File &directoryToLoad, ModulatorSampler *s);;

	ThumbnailHandler(const File &directoryToLoad, const StringArray &fileNames, ModulatorSampler *s);

	static File getThumbnailFile(ModulatorSampler *sampler);;

	static void loadThumbnailsIntoSampler(ModulatorSampler *sampler );

	/** This generates a thumbnail file in the specified directory and loads it into the sampler. */
	static void generateThumbnailData(ModulatorSampler *sampler, const File &directoryToLoad)
	{
		new ThumbnailHandler(directoryToLoad, sampler);
	}

	void saveThumbnail(AudioThumbnailCache *cache, AudioFormatManager &afm, const File &file)
	{
		AudioThumbnail thumb(256, afm, *cache);
		ScopedPointer<AudioFormatReader> afr = afm.createReaderFor(new FileInputStream(file));
		
        if(afr != nullptr)
        {
            AudioSampleBuffer buffer(afr->numChannels, (int)afr->lengthInSamples);
            
            thumb.reset(afr->numChannels, afr->sampleRate, afr->lengthInSamples);
            afr->read(&buffer, 0, (int)afr->lengthInSamples, 0, true, true);
            thumb.addBlock(0, buffer, 0, buffer.getNumSamples());
            cache->storeThumb(thumb, file.hashCode64());
        }
        else
        {
            jassertfalse;
        }
        
	}

	void run() override;

	const bool addThumbNailsToExistingCache;

	ScopedPointer<AudioThumbnailCache> writeCache;

	AlertWindowLookAndFeel laf;

	File directory;

	StringArray fileNamesToLoad;

	ModulatorSampler *sampler;
};

/** Writes all samples that are loaded into the sampler to the specified filenames.
*	@ingroup sampler
*
*	The number of loaded samples must be the same as the size of the filename array.
*/
class SampleWriter: public ThreadWithProgressWindow
{
public:
	SampleWriter(ModulatorSampler *sampler, const StringArray &fileNames);

	void run() override;

private:

	ModulatorSampler *sampler;
	StringArray fileNames;
};

/** A SampleMap is a data structure that encapsulates all data loaded into an ModulatorSampler. 
*	@ingroup sampler
*
*	It saves / loads all sampler data (modulators, effects) as well as all loaded sound files.
*
*	It supports two saving modes (monolithic and file-system based).
*	It only accesses the sampler data when saved or loaded, and uses a ChangeListener to check if a sound has changed.
*/
class SampleMap: public RestorableObject,
				 public SafeChangeListener
{
public:

	/** A SamplerMap can be saved in multiple modes. */
	enum SaveMode
	{
		/** The default mode, until the map gets saved. */
		Undefined = 0,
		/** Saves all data using this file structure:
		*
		*	- the sample map will be saved as .xml file
		*	- the thumbnail data will be saved as thumbnail.dat
		*	- the samples will be saved into a '/samples' subfolder and replaced by relative file references.
		*	- the sampler data (modulators) will be stored as preset file (*.hip) containing a reference to the samplerMap
		*/
		MultipleFiles,
		/** Saves everything into a big file which contains all data. */
		Monolith,
		/** Saves everything into a big file and encrypts the header data using a RSA Key 
		*	which can be used to handle serial numbers
		*/
		MonolithEncrypted,
		numSaveModes
	};
	
	SampleMap(ModulatorSampler *sampler_);

	~SampleMap()
	{
		if(!monolith) saveIfNeeded();
	};

	/** Checks if the samplemap was changed and deletes it. */
	void saveIfNeeded();

	/** writes all loaded samples into the '/samples' subdirectory. */
	void saveAllSoundsToSubdirectory();
	
	void changeListenerCallback(SafeChangeBroadcaster *b);

	/** Checks if any ModulatorSamplerSound was changed since the last save. 
	*
	*	This feature is currently disabled.
	*	It does not check if any other ModulatorSampler Properties were changed.
	*/
	bool hasUnsavedChanges() const
	{
		return false; //fileOnDisk == File::nonexistent || changed;
	}

	/** Returns the file on the disk. */
	File getFile() const {return fileOnDisk;};

	/** loads a XML file from disk. 
	*
	*	It parses the file and calls restoreFromValue().
	*/
	void load(const File &f);

	/** Restores the samplemap from the ValueTree.
	*
	*	If the files are saved in relative mode, the references are replaced
	*	using the parent directory of the sample map before they are loaded.
	*	If the files are saved as monolith, it assumes the files are already loaded and simply adds references to this samplemap.
	*/
	void restoreFromValueTree(const ValueTree &v) override;
	
	/** Exports the SampleMap as ValueTree.
	*
	*	If the relative mode is enabled, it writes the files to the subdirectory '/samples',
	*	if they don't exist yet.
	*/
	ValueTree exportAsValueTree() const override;	

	void replaceFileReferences(ValueTree &soundTree) const;

	/** Saves all data with the mode depending on the file extension. */
	void save(SaveMode m= SaveMode::Undefined);

	/** returns the default sample directory (the sample map directory + '/samples'. */
	String getSampleDirectory() const
	{
		if(fileOnDisk == File::nonexistent) return String::empty;

		return fileOnDisk.getParentDirectory().getFullPathName() + "/samples/";
	};

	/** Clears the sample map. */
    void clear();
	

	void replaceReferencesWithGlobalFolder();

    void setId(Identifier newIdentifier)
    {
        sampleMapId = newIdentifier;
    }
    
    Identifier getId() const { return sampleMapId; };
    
private:

	void resolveMissingFiles(ValueTree &treeToUse);

	void loadSamplesFromDirectory(const ValueTree &v);

	void loadSamplesFromMonolith(const ValueTree &v);

	ModulatorSampler *sampler;

	SaveMode mode;

	File fileOnDisk;
	bool changed;
	bool monolith;

    Identifier sampleMapId;
    
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SampleMap)
		
};

/** A data container which stores information about the amount of round robin groups for each notenumber / velocity combination.
*
*	The information is precalculated so that the query is a very fast look up operation (O(1)). In order to use it, create one, and
*	call addSample() for every ModulatorSamplerSound you need.
*	You can query the rr group later with getRRGroupsForMessage().
*/
class RoundRobinMap
{
public:

	RoundRobinMap();

	/** Clears the map */
	void clear();

	/** adds the information of the sample to the map. It checks for every notenumber / velocity combination if it is the biggest group. */
	void addSample(const ModulatorSamplerSound *sample);

	/** returns the biggest group index for the given MIDI information. This is very fast. */
	int getRRGroupsForMessage(int noteNumber, int velocity);

private:

	char internalData[128][128];

};

#endif  // MODULATORSAMPLERDATA_H_INCLUDED
