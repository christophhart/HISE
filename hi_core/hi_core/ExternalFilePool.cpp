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

namespace hise { using namespace juce;

ImagePool::ImagePool(MainController* mc_) :
	SharedPoolBase(mc_)
{

}

ImagePool::~ImagePool()
{
	clearData();
}

int ImagePool::getNumLoadedFiles() const
{
	return loadedImages.size();
}

Identifier ImagePool::getIdForIndex(int index) const
{
	return loadedImages[index].id;
}

String ImagePool::getFileNameForId(Identifier identifier) const
{
	auto index = loadedImages.indexOf(identifier);
	if (index != -1)
		return loadedImages[index].fileName;

	return String();
}

void ImagePool::clearData()
{
	loadedImages.clear();
}

StringArray ImagePool::getTextDataForId(int index) const
{
	return loadedImages[index].getTextData(getFileTypeName());
}

void ImagePool::storeItemInValueTree(ValueTree& child, int i) const
{
	auto e = loadedImages[i];

	child.setProperty("ID", e.id.toString(), nullptr);
	child.setProperty("FileName", e.fileName, nullptr);

	FileInputStream fis(e.fileName);

	MemoryBlock mb = MemoryBlock();
	MemoryOutputStream out(mb, false);
	out.writeFromInputStream(fis, fis.getTotalLength());

	child.setProperty("Data", var(mb.getData(), mb.getSize()), nullptr);
}

void ImagePool::restoreItemFromValueTree(ValueTree& child)
{
	Identifier id = Identifier(child.getProperty("ID", String()).toString());

	if (loadedImages.indexOf(id) != -1)
		return;

	var x = child.getProperty("Data", var::undefined());

	MemoryBlock *mb = x.getBinaryData();

	jassert(mb != nullptr);

	ScopedPointer<MemoryInputStream> mis = new MemoryInputStream(*mb, false);

	ImageEntry ne;

	ne.fileName = child.getProperty("FileName", String()).toString();
	jassert(ne.fileName.isNotEmpty());

	ne.id = id;
	ne.data = ImageFileFormat::loadFrom(mis->getData(), mis->getDataSize());

	mis = nullptr;

	ImageCache::addImageToCache(ne.data, ne.fileName.hashCode64());

	notifyTable();

	loadedImages.add(ne);
}

Identifier ImagePool::getFileTypeName() const
{
	return ProjectHandler::getIdentifier(ProjectHandler::SubDirectories::Images);
}

size_t ImagePool::getFileSize(const Image *image) const
{
	return (size_t)image->getWidth() * (size_t)image->getHeight() * sizeof(uint32);
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

Image ImagePool::loadImageFromReference(MainController* mc, const String referenceToImage)
{
#if USE_BACKEND
	auto file = GET_PROJECT_HANDLER(mc->getMainSynthChain()).getFilePath(referenceToImage, ProjectHandler::SubDirectories::Images);
	auto img = mc->getSampleManager().getImagePool()->loadFileIntoPool(file);

#else
	String poolName = ProjectHandler::Frontend::getSanitiziedFileNameForPoolReference(referenceToImage);

	auto img = mc->getSampleManager().getImagePool()->loadFileIntoPool(poolName);

	jassert(img.isValid());
#endif

	

	if (img.isValid())
		return img;

	return Image();
}

StringArray ImagePool::getFileNameList() const
{
	StringArray sa;

	for (auto e : loadedImages)
		sa.add(e.fileName);

	return sa;
}

Image ImagePool::loadFileIntoPool(const String& fileName)
{
	Identifier idForFileName = getIdForFileName(fileName);

	const int existingIndex = loadedImages.indexOf(idForFileName);

	if (existingIndex != -1)
	{
		return loadedImages[existingIndex].data;
	}

	ImageEntry ne;
	ne.id = idForFileName;
	ne.fileName = fileName;

	File f = getFileFromFileNameString(fileName);

	ne.data = ImageCache::getFromFile(f);

	loadedImages.add(ne);

	return ne.data;
}

void SharedPoolBase::notifyTable()
{
#if USE_BACKEND
	sendChangeMessage();
#endif
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

	return f;

#else

#if USE_RELATIVE_PATH_FOR_AUDIO_FILES

	if (File::isAbsolutePath(fileName))
		return File(fileName);
	else
		return ProjectHandler::Frontend::getAudioFileForRelativePath(fileName);

#else

	if (File::isAbsolutePath(fileName))
		return File(fileName);
	else
		return File();
#endif

#endif

	
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

AudioSampleBufferPool::AudioSampleBufferPool(MainController* mc) :
	SharedPoolBase(mc)
{
	cache = new AudioThumbnailCache(128);

	afm.registerBasicFormats();
}

AudioSampleBufferPool::~AudioSampleBufferPool()
{
	clearData();
}

Identifier AudioSampleBufferPool::getFileTypeName() const
{
	return ProjectHandler::getIdentifier(ProjectHandler::SubDirectories::AudioFiles);
}

Identifier AudioSampleBufferPool::getIdForIndex(int index) const
{
	return loadedSamples[index].id;
}

String AudioSampleBufferPool::getFileNameForId(Identifier identifier) const
{
	auto index = loadedSamples.indexOf(identifier);
	if (index != -1)
		return loadedSamples[index].fileName;

	return String();
}

StringArray AudioSampleBufferPool::getTextDataForId(int index) const
{
	return loadedSamples[index].getTextData(getFileTypeName());
}

void AudioSampleBufferPool::clearData()
{
	loadedSamples.clear();
}

void AudioSampleBufferPool::storeItemInValueTree(ValueTree& child, int i) const
{
	auto e = loadedSamples[i];

	child.setProperty("ID", e.id.toString(), nullptr);
	child.setProperty("FileName", e.fileName, nullptr);

	FileInputStream fis(e.fileName);

	MemoryBlock mb = MemoryBlock();
	MemoryOutputStream out(mb, false);
	out.writeFromInputStream(fis, fis.getTotalLength());

	child.setProperty("Data", var(mb.getData(), mb.getSize()), nullptr);
	child.setProperty("AdditionalData", e.additionalData, nullptr);
}

void AudioSampleBufferPool::restoreItemFromValueTree(ValueTree& child)
{
	Identifier id = Identifier(child.getProperty("ID", String()).toString());

	if (loadedSamples.indexOf(id) != -1)
		return;

	var x = child.getProperty("Data", var::undefined());

	MemoryBlock *mb = x.getBinaryData();

	jassert(mb != nullptr);

	MemoryInputStream* mis = new MemoryInputStream(*mb, false);

	BufferEntry ne;

	ne.fileName = child.getProperty("FileName", String()).toString();
	ne.additionalData = child.getProperty("AdditionalData");
	jassert(ne.fileName.isNotEmpty());

	ne.id = id;

	loadFromStream(ne, mis);

	loadedSamples.add(ne);
}

AudioSampleBuffer AudioSampleBufferPool::loadFileIntoPool(const String& fileName)
{
	Identifier idForFileName = getIdForFileName(fileName);

	const int existingIndex = loadedSamples.indexOf(idForFileName);

	if (existingIndex != -1)
	{
		return loadedSamples[existingIndex].data;
	}

	BufferEntry be;
	be.id = idForFileName;
	be.fileName = fileName;

	File f = getFileFromFileNameString(fileName);

	if(f.existsAsFile())
		loadFromStream(be, new FileInputStream(f));

	loadedSamples.add(be);

	notifyTable();

	return be.data;
}

double AudioSampleBufferPool::getSampleRateForFile(const Identifier& id)
{
	auto index = loadedSamples.indexOf(id);

	if (index != -1)
	{
		auto be = loadedSamples[index];

		return (double)be.additionalData;
	}

	return 0.0;
}

void AudioSampleBufferPool::loadFromStream(BufferEntry& ne, InputStream* ownedStream)
{
	ScopedPointer<AudioFormatReader> reader = afm.createReaderFor(ownedStream);

	if (reader != nullptr)
	{
		ne.data.setSize(reader->numChannels, (int)reader->lengthInSamples, false, false, false);

		reader->read(&(ne.data), 0, (int)reader->lengthInSamples, 0, true, true);

		ne.additionalData = reader->sampleRate;

	}
}

} // namespace hise