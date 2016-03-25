
template <class FileType>
void Pool<FileType>::restoreFromValueTree(const ValueTree &v)
{
	data.clear();

	for (int i = 0; i < v.getNumChildren(); i++)
	{
		ValueTree child = v.getChild(i);

		String fileName = child.getProperty("FileName", String::empty).toString();

		jassert(fileName.isNotEmpty());

		Identifier id = Identifier(child.getProperty("ID", String::empty).toString());

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
		if (File(fileName).existsAsFile())
		{
			FileInputStream *fis = new FileInputStream(fileName);
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

	return String::empty;
}

template class Pool < Image > ;
template class Pool < AudioSampleBuffer > ;

AudioSampleBuffer * AudioSampleBufferPool::loadDataFromStream(InputStream *inputStream) const
{
	AudioSampleBuffer *buffer = new AudioSampleBuffer();
	ScopedPointer<AudioFormatReader> reader;
	WavAudioFormat waf;

	AudioFormatManager afm;

	afm.registerBasicFormats();

	reader = afm.createReaderFor(inputStream);

	jassert(reader != nullptr);

	if (reader != nullptr)
	{
		//reader = waf.createReaderFor(inputStream, true);
		buffer->setSize(reader->numChannels, (int)reader->lengthInSamples);

		reader->read(buffer, 0, (int)reader->lengthInSamples, 0, true, true);
	}

	if (dynamic_cast<FileInputStream*>(inputStream) != nullptr)
	{
		String fileName = dynamic_cast<FileInputStream*>(inputStream)->getFile().getFullPathName();

		setPropertyForData(getIdForFileName(fileName), sampleRateIdentifier, reader->sampleRate);
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

Image ImagePool::getEmptyImage(int width, int height)
{
	Image i(Image::PixelFormat::ARGB, width, height, true);

	Graphics g(i);

	g.setColour(Colours::grey);

	g.fillAll();

	g.setColour(Colours::black);
	g.drawRect(0, 0, width, height);
	g.setFont(GLOBAL_BOLD_FONT());
	g.drawText("Missing", 1, 1, width - 2, height - 2, Justification::centred, true);

	return i;
}

void ImagePool::reloadData(Image &image, const String &fileName)
{
	Image newImage = ImageFileFormat::loadFrom(File(fileName));
	image = newImage;

	image.duplicateIfShared();
}

Image * ImagePool::loadDataFromStream(InputStream *inputStream) const
{
	Image *image = new Image(ImageFileFormat::loadFrom(*inputStream));

	delete inputStream;

	return image;
}
