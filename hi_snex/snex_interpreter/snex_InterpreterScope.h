#pragma once

namespace snex {
namespace interpreter {

struct Scope : public ReferenceCountedObject
{
	enum class ScopeType { Unknown, Root, Class, Function, Block };

	using Ptr = ReferenceCountedObjectPtr<Scope>;
	using List = ReferenceCountedArray<Scope>;
	using WeakPtr = WeakReference<Scope>;

	Scope(ScopeType st, Scope* parent_, Engine* e, const NamespacedIdentifier& id_);

	struct Entry
	{
		jit::Symbol symbol;
		size_t offset = 0;
		size_t size = 0;
	};

	Result addSymbol(const jit::Symbol& symbol, size_t slotSize, size_t alignment);
	Result finalise();

	void clear();
	void* getDataPointer() const { return dataStart; }
	size_t getDataSize() const { return totalSize; }

	const Entry* find(const Identifier& symbolId) const;
	void* getAddress(const Identifier& symbolId, size_t bytesNeeded) const;
	Array<jit::Symbol> getSymbols() const;
	Scope* getScope(const jit::NamespacedIdentifier& scopeId);

	Scope* getParentScopeWithType(ScopeType sp)
	{
		if (scopeType == sp)
			return this;

		return parent->getParentScopeWithType(sp);
	}

	Scope* getRootScope() 
	{
		if (parent == nullptr)
			return this;

		return parent->getRootScope();
	}

	List children;
	WeakPtr parent = nullptr;
	Engine* engine = nullptr;
	jit::NamespacedIdentifier id;

	HeapBlock<uint8> scopeData;
	void* thisPtr = nullptr;

	const ScopeType scopeType;

	std::map<Identifier, Entry> entries;
	uint8* dataStart = nullptr;
	size_t totalSize = 0;
	size_t maxAlignment = 1;

	JUCE_DECLARE_WEAK_REFERENCEABLE(Scope);
};

} // namespace interpreter
} // namespace snex
