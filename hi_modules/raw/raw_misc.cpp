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

namespace hise {
using namespace juce;

namespace raw
{

void Pool::allowLoadingOfUnusedResources()
{
	allowUnusedSources = true;
}

juce::AudioSampleBuffer Pool::loadAudioFile(const String& id)
{
	auto pool = getMainController()->getCurrentAudioSampleBufferPool();

	PoolReference ref(pool, "{PROJECT_FOLDER}" + id, ProjectHandler::AudioFiles);

	auto entry = pool->loadFromReference(ref, allowUnusedSources ? PoolHelpers::LoadAndCacheStrong :
		PoolHelpers::DontCreateNewEntry
	);

	if (auto b = entry.getData())
	{
		return AudioSampleBuffer(*b);
	}

	jassertfalse;
	return {};
}


juce::Image Pool::loadImage(const String& id)
{
	auto pool = getMainController()->getCurrentImagePool();

	String idToUse = id;

	if (!idToUse.startsWith("{PROJECT_FOLDER}"))
		idToUse = "{PROJECT_FOLDER}" + id;

	PoolReference ref(pool, idToUse, ProjectHandler::Images);

	auto entry = pool->loadFromReference(ref, allowUnusedSources ? PoolHelpers::LoadAndCacheStrong :
		PoolHelpers::DontCreateNewEntry
	);

	if (auto img = entry.getData())
	{
		return Image(*img);
	}

	jassertfalse;
	return {};
}

hise::PoolReference Pool::createSampleMapReference(const String& referenceString)
{
	return PoolReference(getMainController(), referenceString, FileHandlerBase::SampleMaps);
}

hise::PoolReference Pool::createMidiFileReference(const String& referenceString)
{
	return PoolReference(getMainController(), referenceString, FileHandlerBase::MidiFiles);
}

juce::StringArray Pool::getListOfEmbeddedResources(FileHandlerBase::SubDirectories directory, bool useExpansionPool)
{
	auto references = getListOfReferences(directory, &getMainController()->getCurrentFileHandler());

	if (useExpansionPool)
	{
		auto& expHandler = getMainController()->getExpansionHandler();

		for (int i = 0; i < expHandler.getNumExpansions(); i++)
		{
			auto expansion = expHandler.getExpansion(i);
			references.addArray(getListOfReferences(directory, expansion));
		}
	}

	StringArray sa;

	for (auto r : references)
		sa.add(r.getReferenceString());

	return sa;
}

juce::Array<hise::PoolReference> Pool::getListOfReferences(FileHandlerBase::SubDirectories directory, FileHandlerBase* handler)
{
	Array<PoolReference> references;

	switch (directory)
	{
	case hise::FileHandlerBase::AudioFiles:
		references = handler->pool->getAudioSampleBufferPool().getListOfAllReferences(true);
		break;
	case hise::FileHandlerBase::Images:
		references = handler->pool->getImagePool().getListOfAllReferences(true);
		break;
	case hise::FileHandlerBase::SampleMaps:
		references = handler->pool->getSampleMapPool().getListOfAllReferences(true);
		break;
	case hise::FileHandlerBase::MidiFiles:
		references = handler->pool->getMidiFilePool().getListOfAllReferences(true);
		break;
	case hise::FileHandlerBase::UserPresets:
	case hise::FileHandlerBase::Samples:
	case hise::FileHandlerBase::Scripts:
	case hise::FileHandlerBase::Binaries:
	case hise::FileHandlerBase::Presets:
	case hise::FileHandlerBase::XMLPresetBackups:
	case hise::FileHandlerBase::AdditionalSourceCode:
	case hise::FileHandlerBase::numSubDirectories:
	default:
		jassertfalse;
		break;
	}

	return references;
}

void FloatingTileProperties::set(FloatingTile& floatingTile, const std::initializer_list<Property>& list)
{
	auto content = floatingTile.getCurrentFloatingPanel();

	DynamicObject::Ptr obj = new DynamicObject();

	for (const auto& p : list)
	{
		auto id = content->getDefaultablePropertyId(p.id);
		obj->setProperty(id, p.value);
	}

	content->fromDynamicObject(var(obj.get()));
}

FloatingTileProperties::Property::Property(int id_, const var& value_) :
	id(id_),
	value(value_)
{

}

}

} // namespace hise;