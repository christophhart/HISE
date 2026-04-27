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

#ifndef PROCESSOR_METADATA_REGISTRY_H
#define PROCESSOR_METADATA_REGISTRY_H

namespace hise {
using namespace juce;

/** Central registry for ProcessorMetadata, accessed via SharedResourcePointer.
*
*	Holds one ProcessorMetadata per processor type (keyed by the type Identifier
*	from SET_PROCESSOR_NAME). Population happens via registerAllMetadata() which
*	mirrors the existing fillTypeNameList() pattern.
*
*	Usage:
*	  ProcessorMetadataRegistry registry;
*	  auto md = registry.get(processor->getType());
*/
class ProcessorMetadataRegistry
{
public:

	ProcessorMetadataRegistry();

	/** Look up metadata by processor type identifier (e.g. "LFO", "SimpleGain").
	*	Returns a pointer to the stored metadata, or nullptr if not registered.
	*/
	const ProcessorMetadata* get(const Identifier& typeId) const;

	/** Register a metadata entry. Asserts if the typeId is already registered. */
	void add(const Identifier& typeId, const ProcessorMetadata& md);

	/** Register a metadata entry, associating it with the processor's own type ID. */
	void add(const ProcessorMetadata& md);

	/** Get all registered type IDs. */
	Array<Identifier> getRegisteredTypes() const;

	/** Get the total number of registered entries. */
	int getNumRegistered() const;

	/** Convert all registered metadata to a JSON array. */
	var toJSON() const;

private:

	/** Called from constructor. Registers metadata for all known processor types
	*	via their createMetadata() static.
	*/
	void registerAllMetadata();

	struct Data
	{
		void init();

		bool initialised = false;
		std::map<Identifier, ProcessorMetadata> entries;
		DynamicObject::Ptr categories;
	};

	SharedResourcePointer<Data> data;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProcessorMetadataRegistry)
};

} // namespace hise

#endif // PROCESSOR_METADATA_REGISTRY_H
