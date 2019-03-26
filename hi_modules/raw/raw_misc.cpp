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

	PoolReference ref(pool, "{PROJECT_FOLDER}" + id, ProjectHandler::Images);

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

juce::StringArray Pool::getSampleMapList() const
{
	auto pool = getMainController()->getCurrentSampleMapPool();

	StringArray sampleMapNames;
	auto references = pool->getListOfAllReferences(true);
	PoolReference::Comparator comparator;
	references.sort(comparator);

	sampleMapNames.ensureStorageAllocated(references.size());

	for (auto r : references)
		sampleMapNames.add(r.getReferenceString());

	return sampleMapNames;
}

hise::PoolReference Pool::createSampleMapReference(const String& referenceString)
{
	return PoolReference(getMainController(), referenceString, FileHandlerBase::SampleMaps);
}

hise::PoolReference Pool::createMidiFileReference(const String& referenceString)
{
	return PoolReference(getMainController(), referenceString, FileHandlerBase::MidiFiles);
}

}

} // namespace hise;