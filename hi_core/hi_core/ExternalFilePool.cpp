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


ProjectHandler::SubDirectories PoolHelpers::getSubDirectoryType(const AudioSampleBuffer& emptyData)
{
	jassert(emptyData.getNumSamples() == 0);

	return ProjectHandler::SubDirectories::AudioFiles;
}



ProjectHandler::SubDirectories PoolHelpers::getSubDirectoryType(const Image& emptyImage)
{
	jassert(emptyImage.isNull());

	return ProjectHandler::SubDirectories::Images;
}



hise::ProjectHandler::SubDirectories PoolHelpers::getSubDirectoryType(const ValueTree& emptyTree)
{
	return ProjectHandler::SubDirectories::SampleMaps;
}

void PoolHelpers::loadData(AudioFormatManager& afm, InputStream* ownedStream, int64 hashCode, AudioSampleBuffer& data, var* additionalData)
{
	ScopedPointer<AudioFormatReader> reader = afm.createReaderFor(ownedStream);

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

		*additionalData = var(meta);
	}
}

void PoolHelpers::loadData(AudioFormatManager& afm, InputStream* ownedStream, int64 hashCode, Image& data, var* additionalData)
{
	ScopedPointer<InputStream> inputStream = ownedStream;

	data = ImageFileFormat::loadFrom(*inputStream);
	ImageCache::addImageToCache(data, hashCode);

	

}

void PoolHelpers::loadData(AudioFormatManager& afm, InputStream* ownedStream, int64 hashCode, ValueTree& data, var* additionalData)
{
	ScopedPointer<InputStream> inputStream = ownedStream;

	if (auto fis = dynamic_cast<FileInputStream*>(inputStream.get()))
	{
		if (ScopedPointer<XmlElement> xml = XmlDocument::parse(fis->getFile()))
		{
			data = ValueTree::fromXml(*xml);
		}
	}
	else
	{
		data = ValueTree::readFromStream(*inputStream);
	}

	
}

void PoolHelpers::fillMetadata(AudioSampleBuffer& data, var* additionalData)
{

}

void PoolHelpers::fillMetadata(Image& data, var* additionalData)
{
	DynamicObject::Ptr meta = new DynamicObject();

	if (additionalData->isObject())
		meta = additionalData->getDynamicObject();

	meta->setProperty("Size", String(data.getWidth()) + " px x " + String(data.getHeight()) + " px");

	*additionalData = var(meta);
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

	*additionalData = var(meta);
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



PoolHelpers::Reference::Reference(const MainController* mc, const String& referenceString, ProjectHandler::SubDirectories directoryType_) :
	directoryType(directoryType_)
{
	parseReferenceString(mc, referenceString);

	hashCode = reference.hashCode64();
}

PoolHelpers::Reference::Reference(MemoryBlock& mb, const String& referenceString, ProjectHandler::SubDirectories directoryType_)
{
	m = EmbeddedResource;

	memoryLocation = mb.getData();
	memorySize = mb.getSize();

	reference = referenceString;
	directoryType = directoryType_;
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
	jassert(isValid() && !isEmbeddedReference());
	jassert(f.existsAsFile());

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
		BACKEND_ONLY(jassertfalse);
		return new MemoryInputStream(memoryLocation, memorySize, false);
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

bool PoolHelpers::Reference::isValid() const
{
	if (m == AbsolutePath)
		return f.existsAsFile();

	return m != Invalid;
}

hise::ProjectHandler::SubDirectories PoolHelpers::Reference::getFileType() const
{
	return directoryType;
}

var PoolHelpers::Reference::createDragDescription() const
{
	DynamicObject::Ptr obj = new DynamicObject();
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

void PoolHelpers::Reference::parseReferenceString(const MainController* mc, const String& input)
{
	if (input.isEmpty())
	{
		m = Invalid;
		reference = "";
		f = File();
		return;
	}

	

	static const String projectFolderWildcard("{PROJECT_FOLDER}");

	if (ProjectHandler::isAbsolutePathCrossPlatform(input))
	{
		f = File(input);

		auto expansionFolder = mc->getExpansionHandler().getExpansionFolder();


		if (f.isAChildOf(expansionFolder))
		{
			m = ExpansionPath;

			auto relativePath = f.getRelativePathFrom(expansionFolder).replace("\\", "/");
			auto expansionName = relativePath.upToFirstOccurrenceOf("/", false, false);
			auto subDirectoryName = ProjectHandler::getIdentifier(directoryType);
			relativePath = relativePath.fromFirstOccurrenceOf(subDirectoryName, false, false);
			reference = "{EXP::" + expansionName + "}" + relativePath;
			return;
		}

#if USE_BACKEND
		auto projectFolder = mc->getSampleManager().getProjectHandler().getWorkDirectory();

		if (f.isAChildOf(projectFolder))
		{
			m = ProjectPath;

			auto relativePath = f.getRelativePathFrom(projectFolder).replace("\\", "/");
			auto subDirectoryName = ProjectHandler::getIdentifier(directoryType);
			relativePath = relativePath.fromFirstOccurrenceOf(subDirectoryName, false, false);

			if (directoryType == FileHandlerBase::SampleMaps)
				reference = relativePath.upToLastOccurrenceOf(".xml", false, false);
			else
				reference = projectFolderWildcard + relativePath;
			
			return;
		}
#endif

		m = AbsolutePath;

		f = File(input);
		reference = input;
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
            m = LinkToEmbeddedResource;
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

	if (auto e = mc->getExpansionHandler().getExpansionForWildcardReference(input))
	{
		m = ExpansionPath;

		reference = input;
		f = e->getSubDirectory(directoryType).getChildFile(reference.fromFirstOccurrenceOf("}", false, false));
	}
}


} // namespace hise