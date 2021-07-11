/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option any later version.
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

#pragma once

namespace snex {
namespace jit {
using namespace juce;

class FunctionClass;
class NamespaceHandler;
struct AssemblyMemory;
struct SubTypeConstructData;
struct FunctionData;
struct InlineData;

struct ComplexType : public ReferenceCountedObject
{
	static int numInstances;

	struct InitData
	{
		enum class Type
		{
			Constructor,
			Desctructor,
			numTypes
		};

		Type t = Type::numTypes;
		AssemblyMemory* asmPtr = nullptr;
		InlineData* functionTree = nullptr;
		void* dataPointer = nullptr;
		InitialiserList::Ptr initValues;
		bool callConstructor = false;
	};

	ComplexType()
	{
		numInstances++;
	};

	virtual ~ComplexType() { numInstances--; };

	static void* getPointerWithOffset(void* data, size_t byteOffset)
	{
		return reinterpret_cast<void*>((uint8*)data + byteOffset);
	}

	static void writeNativeMemberTypeToAsmStack(const ComplexType::InitData& d, int initIndex, int offsetInBytes, int size);

	static void writeNativeMemberType(void* dataPointer, int byteOffset, const VariableStorage& initValue);

	using Ptr = ReferenceCountedObjectPtr<ComplexType>;
	using WeakPtr = WeakReference<ComplexType>;

	using TypeFunction = std::function<bool(Ptr, void* dataPointer)>;

	/** Override this and return the size of the object. It will be used by the allocator to create the memory. */
	virtual size_t getRequiredByteSize() const = 0;

	virtual size_t getRequiredAlignment() const = 0;

	/** Override this and optimise the alignment. After this call the data structure must not be changed. */
	virtual void finaliseAlignment() { finalised = true; };

	/** finalised the type and returns a ref counted pointer. */
	Ptr finaliseAndReturn();

	virtual void dumpTable(juce::String& s, int& intentLevel, void* dataStart, void* complexTypeStartPointer) const = 0;

	virtual Result initialise(InitData d) = 0;

	template <typename StorageType> Result initialiseObjectStorage(StorageType& c)
	{
		c.setSize(jmax<int>(1, (int)getRequiredByteSize()));

		InitData d;
		d.callConstructor = hasConstructor();
		d.dataPointer = c.getObjectPtr();
		d.t = ComplexType::InitData::Type::Constructor;
		d.initValues = makeDefaultInitialiserList();

		return initialise(d);
	}

	virtual InitialiserList::Ptr makeDefaultInitialiserList() const = 0;

	

	
	FunctionData getDestructor();

	virtual bool hasDestructor();

	virtual bool hasConstructor();

	virtual bool hasDefaultConstructor();

	virtual void registerExternalAtNamespaceHandler(NamespaceHandler* handler, const juce::String& description);

	virtual Types::ID getRegisterType(bool allowSmallObjectOptimisation) const
	{
		ignoreUnused(allowSmallObjectOptimisation);
		return Types::ID::Pointer;
	}

	/** Override this, check if the type matches and call the function for itself and each member recursively and abort if t returns true. */
	virtual bool forEach(const TypeFunction& t, Ptr typePtr, void* dataPointer) = 0;

	/** Override this and return a function class object containing methods that are performed on this type. The object returned by this function must be owned by the caller (because keeping a member object will most likely create a cyclic reference).
	*/
	virtual FunctionClass* getFunctionClass() { return nullptr; };

	FunctionData getNonOverloadedFunction(const Identifier& id);

	/** Returns the node callback with the given channel amount. */
	FunctionData getNodeCallback(const Identifier& id, int numChannels, bool checkProcessFunctions=true);

	virtual var getInternalProperty(const Identifier& id, const var& defaultValue) { return defaultValue; }

	bool isFinalised() const { return finalised; }

	bool operator ==(const ComplexType& other) const
	{
		return matchesOtherType(other);
	}

	virtual bool isValidCastSource(Types::ID nativeSourceType, ComplexType::Ptr complexSourceType) const;

	virtual bool isValidCastTarget(Types::ID nativeTargetType, ComplexType::Ptr complexTargetType) const;

	virtual ComplexType::Ptr createSubType(SubTypeConstructData*) { return nullptr; }



	int hash() const
	{
		return (int)toString().hash();
	}

	void setAlias(const NamespacedIdentifier& newAlias)
	{
		usingAlias = newAlias;
	}

	NamespacedIdentifier getAlias() const
	{
		jassert(hasAlias());
		return usingAlias;
	}

	juce::String toString() const
	{
		if (hasAlias())
			return usingAlias.toString();
		else
			return toStringInternal();
	}

	bool hasAlias() const
	{
		return usingAlias.isValid();
	}

	virtual bool matchesOtherType(const ComplexType& other) const
	{
		if (usingAlias.isValid() && other.usingAlias == usingAlias)
			return true;

		if (toStringInternal() == other.toStringInternal())
			return true;

		return false;
	}

	bool matchesId(const NamespacedIdentifier& id) const
	{
		if (id == usingAlias)
			return true;

		if (toStringInternal() == id.toString())
			return true;

		return false;
	}

	juce::String getActualTypeString() const
	{
		return toStringInternal();
	}


	/** Todo: clean this up so that the paths are the same for stack initialisation and root data construction (just like destructor logic). */
	Result callConstructor(InitData& d);

	virtual Result callDestructor(InitData& d);

private:

	/** Override this and create a string representation. This must be "unique" in a sense that pointers to types with the same string can be interchanged. */
	virtual juce::String toStringInternal() const = 0;

	bool finalised = false;
	NamespacedIdentifier usingAlias;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ComplexType);
	JUCE_DECLARE_WEAK_REFERENCEABLE(ComplexType);
};



}
}