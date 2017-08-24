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

template <class FileType>
void Pool<FileType>::restoreFromValueTree(const ValueTree &v)
{
	data.clear();

	for (int i = 0; i < v.getNumChildren(); i++)
	{
		ValueTree child = v.getChild(i);

		String fileName = child.getProperty("FileName", String()).toString();

		jassert(fileName.isNotEmpty());

		Identifier id = Identifier(child.getProperty("ID", String()).toString());

		if (getDataForId(id) != nullptr) continue;

		var x = child.getProperty("Data", var::undefined());

		MemoryBlock *mb = x.getBinaryData();

		jassert(mb != nullptr);

		MemoryInputStream *mis = new MemoryInputStream(*mb, false);

		addData(id, fileName, mis);
	}
}

template <class FileType>
ValueTree Pool<FileType>::exportAsValueTree() const
{
	ValueTree v(getFileTypeName());

	for (int i = 0; i < data.size(); i++)
	{
		ValueTree child("PoolData");

		PoolData *d = data[i];

		child.setProperty("ID", d->id.toString(), nullptr);
		child.setProperty("FileName", d->fileName, nullptr);

		

		FileInputStream fis(d->fileName);

		MemoryBlock mb = MemoryBlock();
		MemoryOutputStream out(mb, false);
		out.writeFromInputStream(fis, fis.getTotalLength());

		child.setProperty("Data", var(mb.getData(), mb.getSize()), nullptr);

		v.addChild(child, -1, nullptr);
	}

	return v;
}


template <class FileType>
Identifier Pool<FileType>::getIdForIndex(int index)
{
	if (index < data.size())
	{
		return data[index]->id;
	}

	return Identifier();
}

template <class FileType>
int Pool<FileType>::getIndexForId(Identifier id) const
{
	for (int i = 0; i < data.size(); i++)
	{
		if (id == data[i]->id) return i;
	}

	return -1;
}

template <class FileType>
StringArray Pool<FileType>::getFileNameList() const
{
	StringArray sa;

	for (int i = 0; i < data.size(); i++)
	{
		sa.add(data[i]->fileName);
	}

	return sa;
}

template <class FileType>
StringArray Pool<FileType>::getTextDataForId(int i)
{
	if (i < data.size())
	{
		PoolData *d = data[i];

		StringArray info;
		info.add(d->id.toString());
		info.add(String((int)getFileSize(d->data) / 1024) + String(" kB"));
		info.add(getFileTypeName().toString());
		info.add(String(d->refCount));

		return info;
	}

	else
	{
		return StringArray();
	}
}

template <class FileType>
void Pool<FileType>::clearData()
{

	for (int i = 0; i < data.size(); i++)
	{
		if (data[i]->refCount == 0) data.remove(i--);
	}
	sendChangeMessage();
}

template <class FileType>
void Pool<FileType>::releasePoolData(const FileType *b)
{
	if (b == nullptr) return;

	for (int i = 0; i < data.size(); i++)
	{
		if ((data[i]->data) == b)
		{
			data[i]->refCount--;

			sendChangeMessage();
			return;
		}
	}
}

template <class FileType>
void Pool<FileType>::setPropertyForData(const Identifier &id, const Identifier &propertyName, var propertyValue) const
{
	for (int i = 0; i < data.size(); i++)
	{
		if (data[i]->id == id)
		{
			data[i]->properties.set(propertyName, propertyValue);
		}
	}
}

template <class FileType>
var Pool<FileType>::getPropertyForData(const Identifier &id, const Identifier &propertyName) const
{
	for (int i = 0; i < data.size(); i++)
	{
		if (data[i]->id == id)
		{
			return data[i]->properties[propertyName];
		}
	}

	return var::undefined();
}



template <class FileType>
const FileType * Pool<FileType>::loadFileIntoPool(const String &fileName, bool forceReload/*=false*/)
{
	Identifier idForFileName = getIdForFileName(fileName);

	const FileType *existingData = getDataForId(idForFileName);

	if (existingData != nullptr)
	{
		const int index = getIndexForId(idForFileName);

		jassert(index != -1);

		if (forceReload)
		{
			reloadData(*data[index]->data, fileName);
		}

		data[index]->refCount++;
		sendChangeMessage();
		return existingData;
	}
	else
	{
#if USE_FRONTEND && DONT_EMBED_FILES_IN_FRONTEND
        
        
#if HISE_IOS
        const File directory = ProjectHandler::Frontend::getResourcesFolder().getChildFile((getFileTypeName().toString()));
#else
		const File directory = ProjectHandler::Frontend::getAppDataDirectory().getChildFile(getFileTypeName().toString());
#endif

		// This should have been taken care of during installation...
		jassert(directory.isDirectory());

		File f = directory.getChildFile(fileName);

#else
		File f(fileName);
#endif

		if (f.existsAsFile())
		{
			FileInputStream *fis = new FileInputStream(f);
			addData(idForFileName, fileName, fis);

			sendChangeMessage();
			return data.getLast()->data;
		}
		else

			jassertfalse;
		return nullptr;
	}
}

template <class FileType>
void Pool<FileType>::addData(Identifier id, const String &fileName, InputStream *inputStream)
{
	PoolData *d = new PoolData;

	d->id = id;
	d->fileName = fileName;

	

	d->refCount = 1;
	data.add(d);
	
	d->data = loadDataFromStream(inputStream);
}

template <class FileType>
const FileType * Pool<FileType>::getDataForId(Identifier &id) const
{
	for (int i = 0; i < data.size(); i++)
	{
		if (data[i]->id == id)
		{
			
			return data[i]->data;
		}
	}

	return nullptr;
}

template <class FileType>
const String Pool<FileType>::getFileNameForId(Identifier identifier)
{
	for (int i = 0; i < data.size(); i++)
	{
		if (data[i]->id == identifier) return data[i]->fileName;
	}

	return String();
}

template <class FileType>
Identifier Pool<FileType>::getIdForFileName(const String &absoluteFileName) const
{
#if USE_FRONTEND

	return Identifier(absoluteFileName.upToFirstOccurrenceOf(".", false, false)); // .removeCharacters(" \n\t"));

#else

	const File root = getProjectHandler().getSubDirectory((ProjectHandler::SubDirectories)directoryType);

    if(root.isDirectory() && File(absoluteFileName).isAChildOf(root))
    {
        const String id = File(absoluteFileName).getRelativePathFrom(root).upToFirstOccurrenceOf(".", false, false).replaceCharacter('\\', '/');
        
        if(id.isEmpty()) return Identifier();
        else return Identifier(id);
    }
    else
    {
        String id = absoluteFileName.upToFirstOccurrenceOf(".", false, false);
        id = id.replaceCharacter('\\', '/');
        id = id.fromLastOccurrenceOf("/", false, false);
        
        if(id.isEmpty()) return Identifier();
        else return Identifier(id);
    }
    
    

#endif
}

template class Pool < Image > ;
template class Pool < AudioSampleBuffer > ;

#if 0
AudioSampleBufferPool::AudioSampleBufferPool(ProjectHandler *handler) :
Pool<AudioSampleBuffer>(handler, (int)ProjectHandler::SubDirectories::AudioFiles),
sampleRateIdentifier("SampleRate")
{

	cache = new AudioThumbnailCache(128);
}

Identifier AudioSampleBufferPool::getFileTypeName() const
{
	return getProjectHandler().getIdentifier(ProjectHandler::SubDirectories::AudioFiles);
}

AudioSampleBuffer * AudioSampleBufferPool::loadDataFromStream(InputStream *inputStream) const
{
	AudioSampleBuffer *buffer = new AudioSampleBuffer();
	ScopedPointer<AudioFormatReader> reader;
	WavAudioFormat waf;

	AudioFormatManager afm;

	afm.registerBasicFormats();
	afm.registerFormat(new hlac::HiseLosslessAudioFormat(), false);

	reader = afm.createReaderFor(inputStream);

	jassert(reader != nullptr);

	if (reader != nullptr)
	{
		//reader = waf.createReaderFor(inputStream, true);
		buffer->setSize(reader->numChannels, (int)reader->lengthInSamples);

		reader->read(buffer, 0, (int)reader->lengthInSamples, 0, true, true);

		if (dynamic_cast<FileInputStream*>(inputStream) != nullptr)
		{
			String fileName = dynamic_cast<FileInputStream*>(inputStream)->getFile().getFullPathName();

			setPropertyForData(getIdForFileName(fileName), sampleRateIdentifier, reader->sampleRate);
		}
	}



	return buffer;
}

void AudioSampleBufferPool::reloadData(AudioSampleBuffer &dataToBeOverwritten, const String &fileName)
{
	FileInputStream *fis = new FileInputStream(fileName);

	AudioSampleBuffer *newBuffer = loadDataFromStream(fis);

	dataToBeOverwritten.setSize(newBuffer->getNumChannels(), newBuffer->getNumSamples(), true, false, true);

	for (int i = 0; i < dataToBeOverwritten.getNumChannels(); i++)
	{
		FloatVectorOperations::copy(dataToBeOverwritten.getWritePointer(i, 0), newBuffer->getReadPointer(i, 0), dataToBeOverwritten.getNumSamples());
	}
}
#endif

Identifier ImagePool::getFileTypeName() const
{
	return ProjectHandler::getIdentifier(ProjectHandler::SubDirectories::Images);
}

Image ImagePool::getEmptyImage(int width, int height)
{
	static Image i(Image::PixelFormat::ARGB, width, height, true);

	if (i.isValid())
		return i;

	Graphics g(i);

	g.setColour(Colours::grey);

	g.fillAll();

	g.setColour(Colours::black);
	g.drawRect(0, 0, width, height);
	g.setFont(GLOBAL_BOLD_FONT());
	g.drawText("Missing", 1, 1, width - 2, height - 2, Justification::centred, true);

	return i;
}

Image ImagePool::loadImageFromReference(MainController* mc, const String referenceToImage)
{
#if USE_BACKEND
	auto file = GET_PROJECT_HANDLER(mc->getMainSynthChain()).getFilePath(referenceToImage, ProjectHandler::SubDirectories::Images);
	auto img = mc->getSampleManager().getImagePool()->loadFileIntoPool(file, false);

#else
	String poolName = ProjectHandler::Frontend::getSanitiziedFileNameForPoolReference(referenceToImage);

	auto img = mc->getSampleManager().getImagePool()->loadFileIntoPool(poolName, false);

	jassert(img.isValid());
#endif

	

	if (img.isValid())
		return img;

	return Image();
}

File SharedPoolBase::getFileFromFileNameString(const String& fileName)
{
#if USE_FRONTEND && DONT_EMBED_FILES_IN_FRONTEND

#if HISE_IOS
	const File directory = ProjectHandler::Frontend::getResourcesFolder().getChildFile((getFileTypeName().toString()));
#else
	const File directory = ProjectHandler::Frontend::getAppDataDirectory().getChildFile(getFileTypeName().toString());
#endif

	// This should have been taken care of during installation...
	jassert(directory.isDirectory());

	File f = directory.getChildFile(fileName);

#else
	File f(fileName);
#endif

	return f;
}

Identifier SharedPoolBase::getIdForFileName(const String &absoluteFileName) const
{
#if USE_FRONTEND
	return Identifier(absoluteFileName.upToFirstOccurrenceOf(".", false, false)); // .removeCharacters(" \n\t"));
#else

	const File root = getProjectHandler().getSubDirectory(ProjectHandler::SubDirectories::Images);

	if (root.isDirectory() && File(absoluteFileName).isAChildOf(root))
	{
		const String id = File(absoluteFileName).getRelativePathFrom(root).upToFirstOccurrenceOf(".", false, false).replaceCharacter('\\', '/');

		if (id.isEmpty()) return Identifier();
		else return Identifier(id);
	}
	else
	{
		String id = absoluteFileName.upToFirstOccurrenceOf(".", false, false);
		id = id.replaceCharacter('\\', '/');
		id = id.fromLastOccurrenceOf("/", false, false);

		if (id.isEmpty()) return Identifier();
		else return Identifier(id);
	}



#endif
}

ValueTree SharedPoolBase::exportAsValueTree() const
{
	ValueTree v(getFileTypeName());

	for (int i = 0; i < getNumLoadedFiles(); i++)
	{
		ValueTree child("PoolData");

		storeItemInValueTree(child, i);

		v.addChild(child, -1, nullptr);
	}

	return v;
}

void SharedPoolBase::restoreFromValueTree(const ValueTree &v)
{
	clearData();

	for (int i = 0; i < v.getNumChildren(); i++)
	{
		ValueTree child = v.getChild(i);

		restoreItemFromValueTree(child);
	}

#if USE_BACKEND
	sendChangeMessage();
#endif
}

ProjectHandler& SharedPoolBase::getProjectHandler()
{
	return GET_PROJECT_HANDLER(mc->getMainSynthChain());
}

const ProjectHandler& SharedPoolBase::getProjectHandler() const
{
	return GET_PROJECT_HANDLER(mc->getMainSynthChain());
}

Identifier AudioSampleBufferPool::getFileTypeName() const
{
	return ProjectHandler::getIdentifier(ProjectHandler::SubDirectories::AudioFiles);
}
