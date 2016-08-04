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

#ifndef EXTERNALFILEPOOL_H_INCLUDED
#define EXTERNALFILEPOOL_H_INCLUDED

class ProjectHandler;


/** A base class for all objects that can be saved as value tree. 
*	@ingroup core
*/
class RestorableObject
{
public:
    
    virtual ~RestorableObject() {};

	/** Overwrite this method and return a representation of the object as ValueTree.
	*
	*	It's best practice to only store variables that are not internal (eg. states ...)
	*/
	virtual ValueTree exportAsValueTree() const = 0;

	/** Overwrite this method and restore the properties of this object using the referenced ValueTree.
	*/
	virtual void restoreFromValueTree(const ValueTree &previouslyExportedState) = 0;
};


/** A template class which handles the logic of a pool of external file references.
*
*	Features:
*	- load and stores data from files
*	- caches them so that they are referenced multiple times
*	- reference counting and clearing of unused data (but it must be handled manually)
*	- export / import the whole pool as ValueTree.
*/
template <class FileType> class Pool: public SafeChangeBroadcaster,
								public RestorableObject
{
public:

	Pool(ProjectHandler *handler_, int directory) :
		handler(handler_),
		directoryType(directory)
	{};

	virtual ~Pool() {};

	void restoreFromValueTree(const ValueTree &v) override;;

	ValueTree exportAsValueTree() const override;

	/** Returns a non-whitespace version of the file as id (check for collisions!) */
	Identifier getIdForFileName(const String &absoluteFileName) const;

	const String getFileNameForId(Identifier identifier);;

	Identifier getIdForIndex(int index);

	int getIndexForId(Identifier id) const;

	int getNumLoadedFiles() const {return data.size();};
	
	StringArray getFileNameList() const;

	StringArray getTextDataForId(int i);

	/** Clears the data. */
	void clearData();

	/** Call this whenever you don't need the buffer anymore. */
	void releasePoolData(const FileType *b);

	/** This loads the file into the pool. If it is already in the pool, it will be referenced. */
	const FileType *loadFileIntoPool(const String &fileName, bool forceReload=false);;

	/** Overwrite this method and return a file type identifier that will be used for the ValueTree id and stuff. */
	virtual Identifier getFileTypeName() const = 0;

protected:

	/** Set a property for the data. */
	void setPropertyForData(const Identifier &id, const Identifier &propertyName, var propertyValue) const;

	/** returns the property for the data */
	var getPropertyForData(const Identifier &id, const Identifier &propertyName) const;

	/** Overwrite this method and return a data object which will be owned by the pool from then. 
	*
	*	You also must delete the stream object when the job is finished (a bit ugly because The AudioFormatReader class does this automatically)
	*/
	virtual FileType *loadDataFromStream(InputStream *inputStream) const = 0;

	/** Overwrite this method and reload the data from the file. The object must not be deleted, because there may be references to it,
	*	so make sure it will replace the data internally.
	*
	*	You can use loadDataFromStream to load the data and then copy the internal data to the existing data. */
	virtual void reloadData(FileType &dataToBeOverwritten, const String &fileName) = 0;

	/** Overwrite this method and return its file size. */
	virtual size_t getFileSize(const FileType *data) const = 0;

	const ProjectHandler &getProjectHandler() const { return *handler; }

	// The subdirectory type for the project handler
	int directoryType;

private:

	void addData(Identifier id, const String &fileName, InputStream *inputStream);;

	const FileType *getDataForId(Identifier &id) const;

	struct PoolData
	{
		Identifier id;
		ScopedPointer<FileType> data;
		String fileName;
		NamedValueSet properties;
		int refCount;
	};

	OwnedArray<PoolData> data;

	ProjectHandler *handler;
	

	
};


/** A pool for audio samples
*
*	This is used to embed impulse responses into the binary file and load it from there instead of having the impulse file as seperate audio file.
*	In order to use it, create a object, call loadExternalFilesFromValueTree() and the convolution effect checks automatically if it should load the file from disk or from the pool.
*/
class AudioSampleBufferPool: public Pool<AudioSampleBuffer>
{
public:

	AudioSampleBufferPool(ProjectHandler *handler);

	Identifier getFileTypeName() const override;

	virtual size_t getFileSize(const AudioSampleBuffer *data) const override
	{
		return (size_t)data->getNumSamples() * (size_t)data->getNumChannels() * sizeof(float);
	}

	AudioThumbnailCache *getCache() { return cache; }

	double getSampleRateForFile(const Identifier &id)
	{
		return (double)getPropertyForData(id, sampleRateIdentifier);
	}

protected:

	AudioSampleBuffer *loadDataFromStream(InputStream *inputStream) const override;

	void reloadData(AudioSampleBuffer &dataToBeOverwritten, const String &fileName) override;;	


private:

	Identifier sampleRateIdentifier;

	ScopedPointer<AudioThumbnailCache> cache;
	
};


/** A pool for images. */
class ImagePool: public Pool<Image>
{
public:

	ImagePool(ProjectHandler *handler);

	Identifier getFileTypeName() const override;;

	virtual size_t getFileSize(const Image *data) const override
	{
		return (size_t)data->getWidth() * (size_t)data->getHeight() * sizeof(uint32);
	}

	static Image getEmptyImage(int width, int height);

protected:


	void reloadData(Image &image, const String &fileName) override;;	

	Image *loadDataFromStream(InputStream *inputStream) const override;;
};

#endif  // EXTERNALFILEPOOL_H_INCLUDED
