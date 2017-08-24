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
*   which must be separately licensed for cloused source applications:
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


class SharedPoolBase : public RestorableObject,
					   public SafeChangeBroadcaster
{
public:

	SharedPoolBase(MainController* mc_) :
		mc(mc_)
	{};


	virtual ~SharedPoolBase() {};

	virtual Identifier getFileTypeName() const = 0;

	virtual int getNumLoadedFiles() const = 0;

	virtual void storeItemInValueTree(ValueTree& childItem, int index) const = 0;
	virtual void restoreItemFromValueTree(ValueTree& childItem) = 0;

	virtual void clearData() = 0;

	virtual Identifier getIdForIndex(int index) const = 0;
	
	virtual String getFileNameForId(Identifier identifier) const = 0;

	virtual StringArray getTextDataForId(int index) const = 0;

	void notifyTable();

	File getFileFromFileNameString(const String& fileName);

	Identifier getIdForFileName(const String &absoluteFileName) const;

	ValueTree exportAsValueTree() const override;
	void restoreFromValueTree(const ValueTree &v) override;

	ProjectHandler& getProjectHandler();

	const ProjectHandler& getProjectHandler() const;

protected:

	template <class DataType> struct PoolEntry
	{
		PoolEntry() :
			fileName(String()),
			id(Identifier()),
			data(DataType())
		{};

		PoolEntry(const Identifier& id_) :
			id(id_),
			data(DataType())
		{}

		StringArray getTextData(const Identifier& typeName) const
		{
			StringArray info;
			info.add(id.toString());
			info.add("100kB"); // because no one cares actually.
			info.add(typeName.toString());
			info.add("1");

			return info;
		}

		bool operator==(const PoolEntry& other) const
		{
			return this->id == other.id;
		};

		Identifier id;
		String fileName;
		DataType data;
		var additionalData;
	};

	MainController* mc;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SharedPoolBase)
};


/** A pool for audio samples
*
*	This is used to embed impulse responses into the binary file and load it from there instead of having the impulse file as seperate audio file.
*	In order to use it, create a object, call loadExternalFilesFromValueTree() and the convolution effect checks automatically if it should load the file from disk or from the pool.
*/
class AudioSampleBufferPool : public SharedPoolBase
{
public:

	typedef SharedPoolBase::PoolEntry<AudioSampleBuffer> BufferEntry;

	AudioSampleBufferPool(MainController* mc);;
	~AudioSampleBufferPool();

	Identifier getFileTypeName() const override;

	int getNumLoadedFiles() const override
	{
		return loadedSamples.size();
	}

	Identifier getIdForIndex(int index) const override;

	String getFileNameForId(Identifier identifier) const override;
	StringArray getTextDataForId(int index) const;

	void clearData() override;

	void storeItemInValueTree(ValueTree& child, int i) const override;
	void restoreItemFromValueTree(ValueTree& child) override;

	AudioThumbnailCache *getCache() { return cache; }

	AudioSampleBuffer loadFileIntoPool(const String& fileName);

	double getSampleRateForFile(const Identifier& id);

private:

	ScopedPointer<AudioThumbnailCache> cache;

	void loadFromStream(BufferEntry& ne, InputStream* ownedStream);

	AudioFormatManager afm;

	Array<BufferEntry> loadedSamples;

};



/** A pool for images. */
class ImagePool: public SharedPoolBase
{
public:

	typedef SharedPoolBase::PoolEntry<Image> ImageEntry;

	ImagePool(MainController* mc_);;

	~ImagePool();

	int getNumLoadedFiles() const override;
	Identifier getIdForIndex(int index) const override;
	String getFileNameForId(Identifier identifier) const override;
	void clearData() override;
	StringArray getTextDataForId(int index) const;

	void storeItemInValueTree(ValueTree& child, int i) const override;
	void restoreItemFromValueTree(ValueTree& child) override;

	Identifier getFileTypeName() const override;
	static Identifier getStaticIdentifier() { RETURN_STATIC_IDENTIFIER("ImagePool") };
	size_t getFileSize(const Image *image) const;

	StringArray getFileNameList() const;

	Image loadFileIntoPool(const String& fileName);

	static Image getEmptyImage(int width, int height);
	static Image loadImageFromReference(MainController* mc, const String referenceToImage);

protected:

	

	Array<ImageEntry> loadedImages;
};

#endif  // EXTERNALFILEPOOL_H_INCLUDED
