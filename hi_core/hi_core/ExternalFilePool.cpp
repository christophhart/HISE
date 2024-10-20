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

#if JUCE_WINDOWS
#include "ExternalFilePool_impl.h"
#endif

namespace hise { using namespace juce;

ProjectHandler::SubDirectories PoolHelpers::getSubDirectoryType(const AudioSampleBuffer& emptyData)
{
	ignoreUnused(emptyData);
	jassert(emptyData.getNumSamples() == 0);

	return ProjectHandler::SubDirectories::AudioFiles;
}



ProjectHandler::SubDirectories PoolHelpers::getSubDirectoryType(const Image& emptyImage)
{
	ignoreUnused(emptyImage);
	jassert(emptyImage.isNull());

	return ProjectHandler::SubDirectories::Images;
}



hise::ProjectHandler::SubDirectories PoolHelpers::getSubDirectoryType(const ValueTree& emptyTree)
{
	ignoreUnused(emptyTree);
	return ProjectHandler::SubDirectories::SampleMaps;
}

hise::ProjectHandler::SubDirectories PoolHelpers::getSubDirectoryType(const MidiFileReference& emptyTree)
{
	ignoreUnused(emptyTree);
	return ProjectHandler::SubDirectories::MidiFiles;
}


hise::ProjectHandler::SubDirectories PoolHelpers::getSubDirectoryType(const AdditionalDataReference& /*emptyTree*/)
{
	return ProjectHandler::SubDirectories::AdditionalSourceCode;
}

void PoolHelpers::loadData(AudioFormatManager& afm, InputStream* ownedStream, int64 /*hashCode*/, AudioSampleBuffer& data, var* additionalData)
{
	ScopedPointer<AudioFormatReader> reader = afm.createReaderFor(std::unique_ptr<InputStream>(ownedStream));

	if (reader != nullptr)
	{
		data = AudioSampleBuffer(reader->numChannels, (int)reader->lengthInSamples);
		reader->read(&data, 0, (int)reader->lengthInSamples, 0, true, true);

		DynamicObject::Ptr meta = new DynamicObject();
		
		if (additionalData->isObject())
			meta = additionalData->getDynamicObject();

		meta->setProperty(MetadataIDs::SampleRate, reader->sampleRate);
		meta->setProperty(MetadataIDs::LoopEnabled, false);
		meta->setProperty(MetadataIDs::LoopStart, 0);
		meta->setProperty(MetadataIDs::LoopEnd, 0);

		Range<int> sampleRange = { 0, (int)reader->lengthInSamples };

		auto metadata = reader->metadataValues;

		const String format = metadata.getValue("MetaDataSource", "");

		auto getConstrainedLoopValue = [sampleRange](String md)
		{
			return jlimit<int>(sampleRange.getStart(), sampleRange.getEnd(), md.getIntValue());
		};
		
		auto isEmptyOrZero = [](String value) { return value.isEmpty() || value == "0"; };

		if (format == "AIFF")
		{
			meta->setProperty(MetadataIDs::LoopEnabled, isEmptyOrZero(metadata.getValue("Loop0Type", "0")));

			const int loopStartId = metadata.getValue("Loop0StartIdentifier", "-1").getIntValue();
			const int loopEndId = metadata.getValue("Loop0EndIdentifier", "-1").getIntValue();

			int loopStartIndex = -1;
			int loopEndIndex = -1;

			const int numCuePoints = metadata.getValue("NumCuePoints", "0").getIntValue();

			for (int i = 0; i < numCuePoints; i++)
			{
				const String idTag = "CueLabel" + String(i) + "Identifier";

				if (metadata.getValue(idTag, "-2").getIntValue() == loopStartId)
				{
					loopStartIndex = i;
					meta->setProperty(MetadataIDs::LoopStart, getConstrainedLoopValue(metadata.getValue("Cue" + String(i) + "Offset", "")));
				}
				else if (metadata.getValue(idTag, "-2").getIntValue() == loopEndId)
				{
					loopEndIndex = i;
					meta->setProperty(MetadataIDs::LoopEnd, getConstrainedLoopValue(metadata.getValue("Cue" + String(i) + "Offset", "")));
				}
			}

			if (meta->getProperty(MetadataIDs::LoopStart) == meta->getProperty(MetadataIDs::LoopEnd))
				meta->setProperty(MetadataIDs::LoopEnabled, false);
		}
		else if (format == "WAV")
		{
			meta->setProperty(MetadataIDs::LoopStart, getConstrainedLoopValue(metadata.getValue("Loop0Start", "")));
			meta->setProperty(MetadataIDs::LoopEnd, getConstrainedLoopValue(metadata.getValue("Loop0End", "")));

			const bool loopEnabled = meta->getProperty(MetadataIDs::LoopStart) != meta->getProperty(MetadataIDs::LoopEnd) &&
									 (int)meta->getProperty(MetadataIDs::LoopEnd) != 0;

			meta->setProperty(MetadataIDs::LoopEnabled, loopEnabled);
		}

		*additionalData = var(meta.get());
	}
}

void PoolHelpers::loadData(AudioFormatManager& /*afm*/, InputStream* ownedStream, int64 hashCode, Image& data, var* additionalData)
{
	ScopedPointer<InputStream> inputStream = ownedStream;

	data = ImageFileFormat::loadFrom(*inputStream);
	ImageCache::addImageToCache(data, hashCode);

	fillMetadata(data, additionalData);
}

void PoolHelpers::loadData(AudioFormatManager& /*afm*/, InputStream* ownedStream, int64 /*hashCode*/, ValueTree& data, var* additionalData)
{
	ScopedPointer<InputStream> inputStream = ownedStream;

	if (auto fis = dynamic_cast<FileInputStream*>(inputStream.get()))
	{
		if (auto xml = XmlDocument::parse(fis->getFile()))
		{
			data = ValueTree::fromXml(*xml);
		}
	}
	else
	{
		data = ValueTree::readFromStream(*inputStream);
	}

	fillMetadata(data, additionalData);
}

void PoolHelpers::loadData(AudioFormatManager& /*afm*/, InputStream* ownedStream, int64 /*hashCode*/, MidiFileReference& data, var* additionalData)
{
	ScopedPointer<InputStream> inputStream = ownedStream;
	data.getFile().readFrom(*inputStream);
	fillMetadata(data, additionalData);
}

void PoolHelpers::loadData(AudioFormatManager& /*afm*/, InputStream* ownedStream, int64 /*hashCode*/, AdditionalDataReference& data, var* additionalData)
{
	ScopedPointer<InputStream> inputStream = ownedStream;
	data.getFile() = inputStream->readEntireStreamAsString();
	fillMetadata(data, additionalData);
}

void PoolHelpers::fillMetadata(AudioSampleBuffer& /*data*/, var* /*additionalData*/)
{

}

void PoolHelpers::fillMetadata(Image& data, var* additionalData)
{
	DynamicObject::Ptr meta = new DynamicObject();

	if (additionalData->isObject())
		meta = additionalData->getDynamicObject();

	meta->setProperty("Size", String(data.getWidth()) + " px x " + String(data.getHeight()) + " px");

	if (data.getWidth() % 2 == 0 && data.getHeight() % 2 == 0)
	{
		meta->setProperty("Non-retina size: ", String(data.getWidth() / 2) + " px x " + String(data.getHeight() / 2) + " px");
	}

	*additionalData = var(meta.get());
}

void PoolHelpers::fillMetadata(ValueTree& data, var* additionalData)
{
	DynamicObject::Ptr meta = new DynamicObject();

	if (additionalData->isObject())
		meta = additionalData->getDynamicObject();

	meta->setProperty("ID", data.getProperty("ID"));
	meta->setProperty("Round Robin Groups", data.getProperty("RRGroupAmount"));
	meta->setProperty("Sample Mode", (int)data.getProperty("SaveMode") == (int)SampleMap::SaveMode::Monolith ? "Monolith" : "Single files");
	meta->setProperty("Mic Positions", data.getProperty("MicPositions"));
	meta->setProperty("Samples", data.getNumChildren());

	*additionalData = var(meta.get());
}

void PoolHelpers::fillMetadata(MidiFileReference& data, var* additionalData)
{
	DynamicObject::Ptr meta = new DynamicObject();

	if (additionalData->isObject())
		meta = additionalData->getDynamicObject();

	meta->setProperty("ID", data.getId().toString());

	*additionalData = var(meta.get());
}

void PoolHelpers::fillMetadata(AdditionalDataReference& /*data*/, var* additionalData)
{
	DynamicObject::Ptr meta = new DynamicObject();

	*additionalData = var(meta.get());
}

size_t PoolHelpers::getDataSize(const Image* img)
{
	return img ? img->getWidth() * img->getHeight() * 4 : 0;
}

size_t PoolHelpers::getDataSize(const AudioSampleBuffer* buffer)
{
	return buffer ? buffer->getNumChannels() * buffer->getNumSamples() * sizeof(float) : 0;
}

size_t PoolHelpers::getDataSize(const ValueTree* v)
{
	return v->getNumChildren();
}

size_t PoolHelpers::getDataSize(const MidiFileReference* midiFile)
{
	auto f = midiFile->getFile();
	auto ticksPerQuarter = f.getTimeFormat() > 0 ? (int)f.getTimeFormat() : 96;
	return (size_t)(4 * (int)f.getLastTimestamp() / ticksPerQuarter);
}

size_t PoolHelpers::getDataSize(const AdditionalDataReference* stringContent)
{
	return stringContent->getFile().length();
}

bool PoolHelpers::isValid(const AudioSampleBuffer* buffer)
{
	return buffer ? buffer->getNumChannels() != 0 && buffer->getNumSamples() != 0 : false;
}

bool PoolHelpers::isValid(const Image* image)
{
	return image ? image->isValid() : false;
}

bool PoolHelpers::isValid(const ValueTree* v)
{
	return v ? v->isValid() : false;
}

bool PoolHelpers::isValid(const MidiFileReference* file)
{
	return file ? file->isValid() : false;
}

bool PoolHelpers::isValid(const AdditionalDataReference* file)
{
	return file->getFile().isNotEmpty();
}

juce::Image PoolHelpers::getEmptyImage(int width, int height)
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

bool PoolHelpers::isStrong(LoadingType t)
{
	return t == LoadAndCacheStrong || t == ForceReloadStrong || t == SkipPoolSearchStrong || t == LoadIfEmbeddedStrong;
}

bool PoolHelpers::throwIfNotLoaded(LoadingType t)
{
	return t != LoadIfEmbeddedStrong && t != LoadIfEmbeddedWeak;
}

bool PoolHelpers::shouldSearchInPool(LoadingType t)
{
	return t == LoadAndCacheStrong || 
		t == LoadAndCacheWeak || 
		t == ForceReloadStrong || 
		t == ForceReloadWeak || 
		t == DontCreateNewEntry ||
		t == LoadIfEmbeddedStrong ||
		t == LoadIfEmbeddedWeak;
}

bool PoolHelpers::shouldForceReload(LoadingType t)
{
	return t == ForceReloadStrong || t == ForceReloadWeak;
}

Identifier PoolHelpers::getPrettyName(const AudioSampleBuffer*)
{ RETURN_STATIC_IDENTIFIER("AudioFilePool"); }

Identifier PoolHelpers::getPrettyName(const Image*)
{ RETURN_STATIC_IDENTIFIER("ImagePool"); }

Identifier PoolHelpers::getPrettyName(const ValueTree*)
{ RETURN_STATIC_IDENTIFIER("SampleMapPool"); }

Identifier PoolHelpers::getPrettyName(const MidiFileReference*)
{ RETURN_STATIC_IDENTIFIER("MidiFilePool"); }

Identifier PoolHelpers::getPrettyName(const AdditionalDataReference*)
{ RETURN_STATIC_IDENTIFIER("AdditionalDataPool"); }

int PoolHelpers::Reference::Comparator::compareElements(const Reference& first, const Reference& second)
{
	return first.reference.compare(second.reference);
}

PoolHelpers::Reference::operator bool() const
{
	return isValid();
}

PoolHelpers::Reference::Mode PoolHelpers::Reference::getMode() const
{ return m; }

void PoolHelpers::sendErrorMessage(MainController* mc, const String& errorMessage)
{
    mc->sendOverlayMessage(DeactiveOverlay::State::CriticalCustomErrorMessage, errorMessage);
}

PoolHelpers::Reference::Reference(const MainController* mc, const String& referenceString, ProjectHandler::SubDirectories directoryType_) :
	directoryType(directoryType_)
{
	parseReferenceString(mc, referenceString);

	hashCode = reference.hashCode64();
}


PoolHelpers::Reference::Reference():
	m(Mode::Invalid)
{

}

PoolHelpers::Reference::Reference(const var& dragDescription)
{
	parseDragDescription(dragDescription);
}

PoolHelpers::Reference::Reference(PoolBase* pool_, const String& embeddedReference, FileHandlerBase::SubDirectories type):
	pool(pool_),
	directoryType(type)
{
	reference = embeddedReference;
	hashCode = reference.hashCode64();
	m = EmbeddedResource;
}

PoolHelpers::Reference PoolHelpers::Reference::withFileHandler(FileHandlerBase* handler)
{
	if (m == ExpansionPath)
		return *this;

	jassert(m == ProjectPath);

	if (handler->getMainController()->getExpansionHandler().isEnabled())
	{
		if (auto exp = dynamic_cast<Expansion*>(handler))
		{
			auto path = reference.fromFirstOccurrenceOf("{PROJECT_FOLDER}", false, false);
			return exp->createReferenceForFile(path, directoryType);
		}
	}

	ignoreUnused(handler);
	return Reference(*this);
}

juce::String PoolHelpers::Reference::getReferenceString() const
{
	return reference;
}

juce::Identifier PoolHelpers::Reference::getId() const
{
	return id;
}

juce::File PoolHelpers::Reference::getFile() const
{
	jassert(isValid(true) && !isEmbeddedReference());
	
	return f;
}

bool PoolHelpers::Reference::isRelativeReference() const
{
	return m == ExpansionPath || m == ProjectPath;
}

bool PoolHelpers::Reference::isAbsoluteFile() const
{
	return m == AbsolutePath;
}

bool PoolHelpers::Reference::isEmbeddedReference() const
{
	return m == EmbeddedResource;
}

File PoolHelpers::Reference::resolveFile(FileHandlerBase* handler, FileHandlerBase::SubDirectories type) const
{
	if (isEmbeddedReference())
	{
		auto id = Expansion::Helpers::getExpansionIdFromReference(reference);

		auto typeRoot = handler->getRootFolder();
		typeRoot = typeRoot.getChildFile(handler->getIdentifier(type));

		auto refToUse = reference;

		if (refToUse.containsChar('}'))
			refToUse = refToUse.fromFirstOccurrenceOf("}", false, false);

		if (type == FileHandlerBase::SampleMaps)
			refToUse << ".xml";

		return typeRoot.getChildFile(refToUse);
	}

	return f;
}

bool PoolHelpers::Reference::operator==(const Reference& other) const
{
	return other.hashCode == hashCode;
}

bool PoolHelpers::Reference::operator!=(const Reference& other) const
{
	return other.hashCode != hashCode;
}



juce::InputStream* PoolHelpers::Reference::createInputStream() const
{
	switch (m)
	{
	case Mode::AbsolutePath:
	case Mode::ExpansionPath:
	case Mode::ProjectPath:
	{
		ScopedPointer<FileInputStream> fis = new FileInputStream(f);
		if (fis->openedOk())
		{
			return fis.release();
		}

		return nullptr;
	}
	case Mode::EmbeddedResource:
		return pool->getDataProvider()->createInputStream(reference);
	case Mode::LinkToEmbeddedResource:
		jassertfalse;
	case Mode::numModes_:
		break;
	default:
		break;
	}

	return nullptr;
}

juce::int64 PoolHelpers::Reference::getHashCode() const
{
	return hashCode;
}

bool PoolHelpers::Reference::isValid(bool allowNonExistentAbsolutePaths) const
{
	if (m == AbsolutePath)
		return f.existsAsFile() || allowNonExistentAbsolutePaths;

	return m != Invalid;
}

hise::ProjectHandler::SubDirectories PoolHelpers::Reference::getFileType() const
{
	return directoryType;
}

var PoolHelpers::Reference::createDragDescription() const
{
	auto obj = new DynamicObject();
	obj->setProperty("HashCode", hashCode);
	obj->setProperty("Mode", (int)m);
	obj->setProperty("Reference", reference);
	obj->setProperty("Type", directoryType);
	obj->setProperty("File", f.getFullPathName());

	return var(obj);
}

void PoolHelpers::Reference::parseDragDescription(const var& v)
{
	if (auto obj = v.getDynamicObject())
	{
		hashCode = obj->getProperty("HashCode");
		m = (Mode)(int)obj->getProperty("Mode");
		reference = obj->getProperty("Reference").toString();
		directoryType = (FileHandlerBase::SubDirectories)(int)obj->getProperty("Type");
		f = File(obj->getProperty("File").toString());
	}
	else
	{
		jassertfalse;
		m = Invalid;
		reference = "";
		f = File();
		return;
	}
}

void PoolHelpers::Reference::parseReferenceString(const MainController* mc, const String& input_)
{
	String input = input_;

	if (input.isEmpty())
	{
		m = Invalid;
		reference = "";
		f = File();
		return;
	}

	

	static const String projectFolderWildcard("{PROJECT_FOLDER}");
	static const String sampleFolderWildcard("{SAMPLE_FOLDER}");

	if (FullInstrumentExpansion::isEnabled(mc))
	{
		if (directoryType == FileHandlerBase::SampleMaps)
		{
			m = EmbeddedResource;
			reference = input;
			f = File();
			return;
		}
		else if (input.startsWith(projectFolderWildcard))
		{
			if (auto e = mc->getExpansionHandler().getCurrentExpansion())
			{
				input = input.replace(projectFolderWildcard, e->getWildcard());
			}
		}
		else if (input.startsWith(sampleFolderWildcard))
		{
			if (auto e = mc->getExpansionHandler().getCurrentExpansion())
			{
				input = input.replace(sampleFolderWildcard, e->getSubDirectory(FileHandlerBase::Samples).getFullPathName() + "/");
			}
		}
	}

#if USE_RELATIVE_PATH_FOR_AUDIO_FILES

	static const String rpWildcard = "{AUDIO_FILES}";

	if (directoryType == FileHandlerBase::AudioFiles && input.startsWith(rpWildcard))
	{
		m = Mode::AbsolutePath;
		auto root = FrontendHandler::getAdditionalAudioFilesDirectory();
		reference = input;
		f = root.getChildFile(input.fromFirstOccurrenceOf(rpWildcard, false, false));
		return;
	}
#endif
	if (ProjectHandler::isAbsolutePathCrossPlatform(input))
	{
		f = File(input);

		auto expansionFolder = mc->getExpansionHandler().getExpansionFolder();

		if (mc->getExpansionHandler().isEnabled() && f.isAChildOf(expansionFolder))
		{
			m = ExpansionPath;

			auto relativePath = f.getRelativePathFrom(expansionFolder).replace("\\", "/");
			auto eFolder = expansionFolder.getChildFile(relativePath.upToFirstOccurrenceOf("/", false, false));

			String expansionName;

			if (auto e = mc->getExpansionHandler().getExpansionFromRootFile(eFolder))
			{
				expansionName = e->getProperty(ExpansionIds::Name);
			}
			else
			{
				auto eInfoFile = Expansion::Helpers::getExpansionInfoFile(eFolder, Expansion::FileBased);
				jassert(eInfoFile.existsAsFile());
				auto xml = XmlDocument::parse(eInfoFile);
				jassert(xml != nullptr);
				expansionName = xml->getStringAttribute(ExpansionIds::Name.toString());
			}

			jassert(expansionName.isNotEmpty());

			auto subDirectoryName = ProjectHandler::getIdentifier(directoryType);
			relativePath = relativePath.fromFirstOccurrenceOf(subDirectoryName, false, false);

			if (directoryType == FileHandlerBase::SampleMaps)
				relativePath = relativePath.upToLastOccurrenceOf(".xml", false, false);

			reference = "{EXP::" + expansionName + "}" + relativePath;
			return;
		}

#if USE_BACKEND
		auto subFolder = mc->getCurrentFileHandler().getSubDirectory(directoryType);

		if (f.isAChildOf(subFolder))
		{
			m = ProjectPath;

			auto relativePath = f.getRelativePathFrom(subFolder).replace("\\", "/");

			if (directoryType == FileHandlerBase::SampleMaps)
				reference = relativePath.upToLastOccurrenceOf(".xml", false, false);
			else
				reference = projectFolderWildcard + relativePath;

			return;
		}
		else
		{
			auto globalScriptPath = dynamic_cast<const GlobalSettingManager*>(mc)->getSettingsObject().getSetting(HiseSettings::Scripting::GlobalScriptPath);
			File globalScriptFolder = File(globalScriptPath.toString());
			
			if (f.isAChildOf(globalScriptFolder))
			{
				auto filePath = f.getFullPathName().replace(globalScriptPath.toString() + "/", "").replace("\\", "/");
				reference = "{GLOBAL_SCRIPT_FOLDER}" + filePath;
				return;
			}
		}
#endif

		if (directoryType == FileHandlerBase::AudioFiles)
		{
			auto sampleDirectory = mc->getCurrentFileHandler().getSubDirectory(FileHandlerBase::Samples);

			if (f.isAChildOf(sampleDirectory))
			{
				m = ProjectPath;
				auto relativePath = f.getRelativePathFrom(sampleDirectory).replace("\\", "/");
				reference = sampleFolderWildcard + relativePath;
				return;
			}
		}

		m = AbsolutePath;

		f = File(input);
		reference = input;
		return;
	}

	if (auto e = mc->getExpansionHandler().getExpansionForWildcardReference(input))
	{
		if (e->getExpansionType() == Expansion::FileBased || directoryType == FileHandlerBase::Samples)
		{
			m = ExpansionPath;

			reference = input;
			f = e->getSubDirectory(directoryType).getChildFile(reference.fromFirstOccurrenceOf("}", false, false));
			return;
		}
		else
		{
			m = EmbeddedResource;
			reference = input;
			f = File();
			return;
		}
	}
	
	if (input.startsWith(sampleFolderWildcard) && directoryType == FileHandlerBase::AudioFiles)
	{
		reference = input;
		m = ProjectPath;

		auto relativePath = input.replace("\\", "/").replace(sampleFolderWildcard, "");
		auto& projectHandler = mc->getSampleManager().getProjectHandler();
		f = projectHandler.getSubDirectory(FileHandlerBase::Samples).getChildFile(relativePath);
		return;
	}


	if (input.startsWith(projectFolderWildcard) || directoryType == FileHandlerBase::SampleMaps)
	{
		reference = input;

#if USE_BACKEND
		m = ProjectPath;


		auto relativePath = input.replace("\\", "/").replace(projectFolderWildcard, "");

		if (directoryType == FileHandlerBase::SampleMaps)
			relativePath.append(".xml", 5);

		auto& projectHandler = mc->getSampleManager().getProjectHandler();

		f = projectHandler.getSubDirectory(directoryType).getChildFile(relativePath);
#else
        
        if(directoryType != FileHandlerBase::Samples)
        {
            m = EmbeddedResource;
        }
        else
        {
            m = ProjectPath;
            
            
            auto relativePath = input.replace("\\", "/").replace(projectFolderWildcard, "");

            auto& projectHandler = mc->getSampleManager().getProjectHandler();
            
            f = projectHandler.getSubDirectory(directoryType).getChildFile(relativePath);
        }
        
		// An embedded resource must be created using an memory input stream...
		

#endif

		return;
	}

	
}


bool PoolBase::DataProvider::isEmbeddedResource(PoolReference r)
{
	return r.isEmbeddedReference() || hashCodes.contains(r.getHashCode());
}

hise::PoolReference PoolBase::DataProvider::getEmbeddedReference(PoolReference other)
{
	return PoolReference(pool, other.getReferenceString(), other.getFileType());
}

juce::Result PoolBase::DataProvider::restorePool(InputStream* ownedInputStream)
{
	pool->clearData();

	input = ownedInputStream;
	int64 metadataSize = input->readInt64();

	if (metadataSize == 0)
		return Result::ok();

	MemoryBlock metadataBlock;

	input->readIntoMemoryBlock(metadataBlock, (size_t)metadataSize);

	jassert((int64)metadataBlock.getSize() == metadataSize);

	zstd::ZDefaultCompressor mDecomp;
	mDecomp.expand(metadataBlock, metadata);

	jassert(metadata.isValid());
	jassert(metadata.getType() == Identifier("PoolData"));

	static const Identifier hc("HashCode");

	for (const auto& item : metadata)
	{
		hashCodes.add(item.getProperty(hc));
	}

	metadataOffset = input->getPosition();

	embeddedSize = input->getTotalLength();

	return Result::ok();
}

juce::MemoryInputStream* PoolBase::DataProvider::createInputStream(const String& referenceString)
{
	if (metadata.isValid())
	{
		auto item = metadata.getChildWithProperty("ID", referenceString);

		if (item.isValid())
		{
			auto offset = (int64)item.getProperty("ChunkStart");
			auto end = (int64)item.getProperty("ChunkEnd");

			if (input != nullptr && (input->getTotalLength() > offset + metadataOffset))
			{
				input->setPosition(offset + metadataOffset);

				MemoryBlock mb;
				input->readIntoMemoryBlock(mb, (size_t)(end - offset));

				return new MemoryInputStream(mb, true);
			}
		}
		else
		{
			for (auto i : metadata)
				DBG(i.getProperty("ID").toString());
		}

        DBG("WARNING: Not found: " + referenceString);
		return nullptr;
	}
	else
	{
		jassertfalse;
		return nullptr;
	}
}

juce::Result PoolBase::DataProvider::writePool(OutputStream* ownedOutputStream, double* progress/*=nullptr*/)
{
	ScopedPointer<OutputStream> output = ownedOutputStream;
	
	MemoryOutputStream dataOutputStream;

	metadata = ValueTree("PoolData");

	for (int i = 0; i < pool->getNumLoadedFiles(); i++)
	{
		if (progress != nullptr)
		{
			double total = (double)pool->getNumLoadedFiles();
			*progress = (double)i / total;
		}

		if (Thread::currentThreadShouldExit())
			return Result::fail("Aborted");

		auto ref = pool->getReference(i);
		auto additionalData = pool->getAdditionalData(ref);

		ValueTree child = ValueTreeConverters::convertDynamicObjectToValueTree(additionalData, "Item");

		const String message = "Writing " + ref.getReferenceString() + " ... " + String(dataOutputStream.getPosition() / 1024) + " kB";

		if(auto l = Logger::getCurrentLogger())
			l->writeToLog(message);

		child.setProperty("ID", ref.getReferenceString(), nullptr);
		child.setProperty("HashCode", ref.getHashCode(), nullptr);

		MemoryOutputStream itemData;

		pool->writeItemToOutput(itemData, ref);

		DBG(message);

		child.setProperty("ChunkStart", dataOutputStream.getPosition(), nullptr);
		dataOutputStream.write(itemData.getData(), itemData.getDataSize());
		child.setProperty("ChunkEnd", dataOutputStream.getPosition(), nullptr);

		metadata.addChild(child, -1, nullptr);
	}

	if (Thread::currentThreadShouldExit())
		return Result::fail("Aborted");


	MemoryBlock compressedMetadata;

	zstd::ZDefaultCompressor mComp;

	auto result = mComp.compress(metadata, compressedMetadata);

	if (result.failed())
	{
		jassertfalse;
		DBG(result.getErrorMessage());
		return result;
	}



	MemoryOutputStream metadataOutputStream;

	metadataOutputStream.write(compressedMetadata.getData(), compressedMetadata.getSize());

	int64 size = (int64)metadataOutputStream.getDataSize();

	output->writeInt64(size);
	output->write(metadataOutputStream.getData(), metadataOutputStream.getDataSize());
	output->write(dataOutputStream.getData(), dataOutputStream.getDataSize());

	output->flush();

	return Result::ok();
}

var PoolBase::DataProvider::createAdditionalData(PoolReference r)
{
	auto item = metadata.getChildWithProperty("ID", r.getReferenceString());

	if (item.isValid())
	{
		var data = ValueTreeConverters::convertValueTreeToDynamicObject(item);
		
		if (auto obj = data.getDynamicObject())
		{
			obj->removeProperty("ID");
			obj->removeProperty("HashCode");
		}

		return data;
	}

	return var();
}

Array<hise::PoolReference> PoolBase::DataProvider::getListOfAllEmbeddedReferences() const
{
	Array<PoolReference> references;

	for (const auto& c : metadata)
	{
		auto rString = c.getProperty("ID").toString();

		references.add(PoolReference(pool, rString, pool->getFileType()));
	}

	return references;
}

PoolBase::ScopedNotificationDelayer::ScopedNotificationDelayer(PoolBase& parent_, EventType type):
	parent(parent_),
	t(type)
{
	parent.skipNotification = true;
}

PoolBase::ScopedNotificationDelayer::~ScopedNotificationDelayer()
{
	parent.skipNotification = false;
	parent.sendPoolChangeMessage(t, sendNotificationAsync);
}

PoolBase::DataProvider::Compressor::~Compressor()
{}

PoolBase::DataProvider::DataProvider(PoolBase* pool_):
	pool(pool_),
	metadataOffset(-1),
	compressor(new Compressor())
{}

PoolBase::DataProvider::~DataProvider()
{}

const PoolBase::DataProvider::Compressor* PoolBase::DataProvider::getCompressor() const
{ return compressor; }

void PoolBase::DataProvider::setCompressor(Compressor* newCompressor)
{ compressor = newCompressor; }

size_t PoolBase::DataProvider::getSizeOfEmbeddedReferences() const
{ return embeddedSize; }

PoolBase::Listener::~Listener()
{}

void PoolBase::Listener::poolEntryAdded()
{}

void PoolBase::Listener::poolEntryRemoved()
{}

void PoolBase::Listener::poolEntryChanged(PoolReference referenceThatWasChanged)
{}

void PoolBase::Listener::poolEntryReloaded(PoolReference referenceThatWasChanged)
{}

void PoolBase::sendPoolChangeMessage(EventType t, NotificationType notify, PoolReference r)
{
	if (skipNotification && notify == sendNotificationAsync)
		return;

	lastType = t;
	lastReference = r;

	if (notify == sendNotificationAsync)
		notifier.triggerAsyncUpdate();
	else
		notifier.handleAsyncUpdate();
}

void PoolBase::addListener(Listener* l)
{
	listeners.addIfNotAlreadyThere(l);
}

void PoolBase::removeListener(Listener* l)
{
	listeners.removeAllInstancesOf(l);
}

void PoolBase::setDataProvider(DataProvider* newDataProvider)
{
	dataProvider = newDataProvider;
}

PoolBase::DataProvider* PoolBase::getDataProvider()
{ return dataProvider; }

const PoolBase::DataProvider* PoolBase::getDataProvider() const
{ return dataProvider; }

FileHandlerBase::SubDirectories PoolBase::getFileType() const
{
	return type;
}

void PoolBase::setUseSharedPool(bool shouldUse)
{
	useSharedCache = shouldUse;
}

FileHandlerBase* PoolBase::getFileHandler() const
{ return parentHandler; }

PoolBase::PoolBase(MainController* mc, FileHandlerBase* handler):
	ControlledObject(mc),
	notifier(*this),
	type(FileHandlerBase::SubDirectories::numSubDirectories),
	dataProvider(new DataProvider(this)),
	parentHandler(handler)
{

}

PoolBase::Notifier::Notifier(PoolBase& parent_):
	parent(parent_)
{}

PoolBase::Notifier::~Notifier()
{
	cancelPendingUpdate();
}

void PoolBase::Notifier::handleAsyncUpdate()
{
	ScopedLock sl(parent.listeners.getLock());
			
	for (auto& l : parent.listeners)
	{
		if (l != nullptr)
		{
			switch (parent.lastType)
			{
			case Added: l->poolEntryAdded(); break;
			case Removed: l->poolEntryRemoved(); break;
			case Changed: l->poolEntryChanged(parent.lastReference); break;
			case Reloaded: l->poolEntryReloaded(parent.lastReference); break;
			default:
				break;
			}
		}
	}
}

void PoolBase::DataProvider::Compressor::write(OutputStream& output, const ValueTree& data, const File& /*originalFile*/) const
{
	zstd::ZCompressor<SampleMapDictionaryProvider> comp;
	MemoryBlock mb;
	comp.compress(data, mb);
	output.write(mb.getData(), mb.getSize());
	
#if 0
	GZIPCompressorOutputStream zipper(&output, 9);
	data.writeToStream(zipper);
	zipper.flush();
#endif
}

void PoolBase::DataProvider::Compressor::write(OutputStream& output, const Image& data, const File& originalFile) const
{
	const bool isValidImage = ImageFileFormat::loadFrom(originalFile).isValid();

	int originalFileSize = 0;

	if (isValidImage)
	{
		originalFileSize = (int)originalFile.getSize();
	}

	MemoryOutputStream newlyCompressedImage;

	PNGImageFormat format;
	format.writeImageToStream(data, newlyCompressedImage);
	auto newSize = newlyCompressedImage.getDataSize();

	if (isValidImage && originalFileSize < (int64)newSize)
	{
		FileInputStream fis(originalFile);
		output.writeFromInputStream(fis, fis.getTotalLength());
	}
	else
	{
		output.write(newlyCompressedImage.getData(), newlyCompressedImage.getDataSize());
	}
}


void PoolBase::DataProvider::Compressor::write(OutputStream& output, const AudioSampleBuffer& data, const File& /*originalFile*/) const
{
	FlacAudioFormat format;
	
	MemoryBlock mb;
	MemoryOutputStream* tempStream = new MemoryOutputStream(mb, true);

	if (ScopedPointer<AudioFormatWriter> writer = format.createWriterFor(tempStream, 44100.0, data.getNumChannels(), 24, StringPairArray(), 9))
	{
		writer->writeFromAudioSampleBuffer(data, 0, data.getNumSamples());

		// We need to destruct the writer before the next line in order to make sure it flushes the last padded block correctly.
		writer = nullptr;

		output.write(mb.getData(), mb.getSize());
	}
}

void PoolBase::DataProvider::Compressor::write(OutputStream& output, const MidiFileReference& data, const File& /*originalFile*/) const
{
	data.getFile().writeTo(output);
}

void PoolBase::DataProvider::Compressor::write(OutputStream& output, const AdditionalDataReference& data, const File& /*originalFile*/) const
{
	output.writeString(data.getFile());
}

void PoolBase::DataProvider::Compressor::create(MemoryInputStream* mis, ValueTree* data) const
{
	ScopedPointer<MemoryInputStream> scopedInput = mis;
	
	static zstd::ZCompressor<SampleMapDictionaryProvider> dec;
	MemoryBlock mb;
	mis->readIntoMemoryBlock(mb);
	dec.expand(mb, *data);
	jassert(data->isValid());

#if 0
		ScopedPointer<MemoryInputStream> scopedInput = mis;

	*data = ValueTree::readFromGZIPData(mis->getData(), mis->getDataSize());

	jassert(data->isValid())

	scopedInput = nullptr;
#endif
}

void PoolBase::DataProvider::Compressor::create(MemoryInputStream* mis, Image* data) const
{
	ScopedPointer<MemoryInputStream> scopedInput = mis;

	if (auto ff = ImageFileFormat::findImageFormatForStream(*mis))
	{
		*data = ff->decodeImage(*mis);
	}
}

void PoolBase::DataProvider::Compressor::create(MemoryInputStream* mis, AudioSampleBuffer* data) const
{
	FlacAudioFormat format;

	if (ScopedPointer<AudioFormatReader> reader = format.createReaderFor(mis, false))
	{
		*data = AudioSampleBuffer(reader->numChannels, (int)reader->lengthInSamples);
		reader->read(data, 0, (int)reader->lengthInSamples, 0, true, true);
	}
		
}

void PoolBase::DataProvider::Compressor::create(MemoryInputStream* mis, MidiFileReference* data) const
{
	ScopedPointer<MemoryInputStream> scopedInput = mis;
	data->getFile().readFrom(*mis);
}

void PoolBase::DataProvider::Compressor::create(MemoryInputStream* mis, AdditionalDataReference* data) const
{
	ScopedPointer<MemoryInputStream> scopedInput = mis;

	auto d = mis->readEntireStreamAsString();

	data->getFile().swapWith(d);
}


EncryptedCompressor::EncryptedCompressor(BlowFish* ownedKey) :
	key(ownedKey)
{

}

void EncryptedCompressor::encrypt(MemoryBlock&& mb, OutputStream& output) const
{
	key->encrypt(mb);
	output.write(mb.getData(), mb.getSize());
}

void EncryptedCompressor::write(OutputStream& output, const ValueTree& data, const File& originalFile) const
{
	MemoryBlock mb;

	zstd::ZDefaultCompressor comp;
	auto result = comp.compress(data, mb);

	if (result.failed())
	{
		DBG(result.getErrorMessage());
		jassertfalse;
	}

	key->encrypt(mb);
	output.write(mb.getData(), mb.getSize());
}

void EncryptedCompressor::create(MemoryInputStream* mis, AdditionalDataReference* data) const
{
	ScopedPointer<MemoryInputStream> ownedStream = mis;

	MemoryBlock mb;
	mis->readIntoMemoryBlock(mb);
	key->decrypt(mb);

	ownedStream = new MemoryInputStream(mb, false);

	Compressor::create(ownedStream.release(), data);
}

void EncryptedCompressor::create(MemoryInputStream* mis, MidiFileReference* data) const
{
	ScopedPointer<MemoryInputStream> ownedStream = mis;

	MemoryBlock mb;
	mis->readIntoMemoryBlock(mb);
	key->decrypt(mb);

	ownedStream = new MemoryInputStream(mb, false);

	Compressor::create(ownedStream.release(), data);
}

void EncryptedCompressor::write(OutputStream& output, const AdditionalDataReference& data, const File& originalFile) const
{
	MemoryOutputStream mos;
	Compressor::write(mos, data, originalFile);
	encrypt(mos.getMemoryBlock(), output);
}

void EncryptedCompressor::create(MemoryInputStream* mis, AudioSampleBuffer* data) const
{
	ScopedPointer<MemoryInputStream> ownedStream = mis;

	MemoryBlock mb;
	mis->readIntoMemoryBlock(mb);
	key->decrypt(mb);

	ownedStream = new MemoryInputStream(mb, false);

	Compressor::create(ownedStream.release(), data);
}

void EncryptedCompressor::write(OutputStream& output, const MidiFileReference& data, const File& originalFile) const
{
	MemoryOutputStream mos;
	Compressor::write(mos, data, originalFile);
	encrypt(mos.getMemoryBlock(), output);
}

void EncryptedCompressor::create(MemoryInputStream* mis, Image* data) const
{
	Compressor::create(mis, data);
}

void EncryptedCompressor::write(OutputStream& output, const AudioSampleBuffer& data, const File& originalFile) const
{
	MemoryOutputStream mos;
	Compressor::write(mos, data, originalFile);
	encrypt(mos.getMemoryBlock(), output);
}

void EncryptedCompressor::create(MemoryInputStream* mis, ValueTree* data) const
{
	ScopedPointer<MemoryInputStream> ownedStream = mis;

	MemoryBlock mb;
	mis->readIntoMemoryBlock(mb);
	key->decrypt(mb);
	zstd::ZDefaultCompressor comp;
	comp.expand(mb, *data);

	jassert(data->isValid());
}

void EncryptedCompressor::write(OutputStream& output, const Image& data, const File& originalFile) const
{
	Compressor::write(output, data, originalFile);
}

PoolCollection::PoolCollection(MainController* mc, FileHandlerBase* handler) :
	ControlledObject(mc),
	parentHandler(handler)
{
	for (int i = 0; i < (int)ProjectHandler::SubDirectories::numSubDirectories; i++)
	{
		switch ((ProjectHandler::SubDirectories)i)
		{
		case ProjectHandler::SubDirectories::AdditionalSourceCode:
			if (mc->getExpansionHandler().isEnabled())
				dataPools[i] = new AdditionalDataPool(mc, parentHandler);
			else
				dataPools[i] = nullptr;
			break;
		case ProjectHandler::SubDirectories::AudioFiles:
			dataPools[i] = new AudioSampleBufferPool(mc, parentHandler);
			break;
		case ProjectHandler::SubDirectories::Images:
			dataPools[i] = new ImagePool(mc, parentHandler);
			break;
		case ProjectHandler::SubDirectories::Samples:
			dataPools[i] = new ModulatorSamplerSoundPool(mc, parentHandler);
			break;
		case ProjectHandler::SubDirectories::SampleMaps:
			dataPools[i] = new SampleMapPool(mc, parentHandler);
			break;
		case ProjectHandler::SubDirectories::MidiFiles:
			dataPools[i] = new MidiFilePool(mc, parentHandler);
			break;
		default:
			dataPools[i] = nullptr;
		}
	}

#if USE_FRONTEND
	// This makes plugins use one global pool of images in order to save memory
	dataPools[ProjectHandler::SubDirectories::Images]->setUseSharedPool(true);

	// Since memory is super tight on AUv3, we also share the audio files here...
	if (HiseDeviceSimulator::isAUv3())
		dataPools[ProjectHandler::SubDirectories::AudioFiles]->setUseSharedPool(true);
#endif
}

PoolCollection::~PoolCollection()
{
	for (int i = 0; i < (int)ProjectHandler::SubDirectories::numSubDirectories; i++)
	{
		if (dataPools[i] != nullptr)
		{
			delete dataPools[i];
			dataPools[i] = nullptr;
		}
	}
}

void PoolCollection::clear()
{
	for (int i = 0; i < (int)ProjectHandler::SubDirectories::numSubDirectories; i++)
	{
		if (dataPools[i] != nullptr)
		{
			dataPools[i]->clearData();
		}
	}
}

const hise::AudioSampleBufferPool& PoolCollection::getAudioSampleBufferPool() const
{
	return *getPool<AudioSampleBuffer>();
}

hise::AudioSampleBufferPool& PoolCollection::getAudioSampleBufferPool()
{
	return *getPool<AudioSampleBuffer>();
}

const hise::ImagePool& PoolCollection::getImagePool() const
{
	return *getPool<Image>();
}

hise::ImagePool& PoolCollection::getImagePool()
{
	return *getPool<Image>();
}

hise::AdditionalDataPool& PoolCollection::getAdditionalDataPool()
{
	return *dynamic_cast<SharedPoolBase<AdditionalDataReference>*>(getPoolBase(FileHandlerBase::AdditionalSourceCode));
}

const hise::AdditionalDataPool& PoolCollection::getAdditionalDataPool() const
{
	return *dynamic_cast<const SharedPoolBase<AdditionalDataReference>*>(dataPools[FileHandlerBase::AdditionalSourceCode]);
}

const hise::SampleMapPool& PoolCollection::getSampleMapPool() const
{
	return *getPool<ValueTree>();
}

hise::SampleMapPool& PoolCollection::getSampleMapPool()
{
	return *getPool<ValueTree>();
}

const MidiFilePool& PoolCollection::getMidiFilePool() const
{
	return *getPool<MidiFileReference>();
}

MidiFilePool& PoolCollection::getMidiFilePool()
{
	return *getPool<MidiFileReference>();
}

const ModulatorSamplerSoundPool* PoolCollection::getSamplePool() const
{
	return static_cast<const ModulatorSamplerSoundPool*>(dataPools[FileHandlerBase::Samples]);
}

ModulatorSamplerSoundPool* PoolCollection::getSamplePool()
{
	return static_cast<ModulatorSamplerSoundPool*>(dataPools[FileHandlerBase::Samples]);
}

PooledAudioFileDataProvider::PooledAudioFileDataProvider(MainController* mc):
	ControlledObject(mc)
{}

void PooledAudioFileDataProvider::setRootDirectory(const File& rootDirectory)
{
	customDefaultFolder = rootDirectory;
}

hise::MultiChannelAudioBuffer::SampleReference::Ptr PooledAudioFileDataProvider::loadFile(const String& reference)
{
	MultiChannelAudioBuffer::SampleReference::Ptr lr;

	if (reference.isEmpty())
		return lr;

	PoolReference ref(getMainController(), reference, FileHandlerBase::AudioFiles);

	lastHandler = getFileHandlerBase(reference);

	if (auto dataPtr = lastHandler->pool->getAudioSampleBufferPool().loadFromReference(ref, PoolHelpers::LoadAndCacheWeak))
	{
		lr = new MultiChannelAudioBuffer::SampleReference();

		auto metadata = dataPtr->additionalData;
		
		lr->sampleRate = metadata.getProperty(MetadataIDs::SampleRate, 0.0);

		if (metadata.getProperty(MetadataIDs::LoopEnabled, false))
		{
			// add 1 because of the offset
			lr->loopRange = { (int)metadata.getProperty(MetadataIDs::LoopStart, 0), (int)metadata.getProperty(MetadataIDs::LoopEnd, 0) + 1 };
		}

		lr->buffer = dataPtr->data;
		lr->reference = ref.getReferenceString();
	}
	
	return lr;
}

File PooledAudioFileDataProvider::parseFileReference(const String& b64) const
{
	if(b64.isEmpty())
		return File();

	PoolReference ref(getMainController(), b64, FileHandlerBase::AudioFiles);

	return ref.getFile();
}

juce::File PooledAudioFileDataProvider::getRootDirectory()
{
	if (customDefaultFolder.isDirectory())
		return customDefaultFolder;

	if (lastHandler == nullptr)
		lastHandler = getMainController()->getActiveFileHandler();

	if(lastHandler != nullptr)
	{
		return lastHandler->getSubDirectory(FileHandlerBase::AudioFiles);
	}

	return {};
}

hise::FileHandlerBase* PooledAudioFileDataProvider::getFileHandlerBase(const String& refString)
{
	if (auto e = getMainController()->getExpansionHandler().getExpansionForWildcardReference(refString))
		return e;

	return &getMainController()->getSampleManager().getProjectHandler();
}

} // namespace hise
